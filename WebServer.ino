void serverStart(){

  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                    // Receive and save the file
  );

  server.on ("/", handleRoot);
  server.on ("/index.htm", handleRoot);
  server.on ("/beeb.js",[]() {
    handleFileRead("/beeb.js");
  });
  server.on ("/config.htm",[]() {
    handleFileRead("/config.htm");
  }); 
  server.on ("/beebowl.png",[]() {
    handleFileRead("/beebowl.png");
  });
  server.on ("/mmbdata", mmbFileList);
  server.on ("/ssddata", ssdData);
  server.on ("/setconfig", updateSettings);
  server.on ("/getconfig",getSettings);
  server.on("/update",httpUpdate);
  server.on("/download",ssdDownload);
  
 
  server.onNotFound (handleNotFound);
  server.begin();
  Serial.println ("HTTP server started");
}


void handleRoot() {
  if(!handleFileRead("/index.htm")) server.send(404, "text/plain", "FileNotFound");
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
 
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  //else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".png")) return "image/png";
  //else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    fs::File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the SPIFFS
 
  HTTPUpload& upload = server.upload();
  //Serial.println("OK...");
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    //TODO validate file name & size

    checkNextFree();
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    filename=CurrentMMB;
    if(!filename.startsWith("/")) filename = "/"+filename;
    
    hostSelect(ESP8266);
    sdFile = SD.open(filename,FILE_WRITE);            // Open the file for writing in SD
    if(!sdFile){
      Serial.println("SD Error");
      hostSelect(BBC);
      return;
    }
    if(!sdFile.seek((NextFree*SSDSIZE)+8192)){
      Serial.println("Seek Error");
      hostSelect(BBC);
      return; 
      
    }
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(sdFile)
      sdFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(sdFile) {  // If the file was successfully created
      Serial.println("Complete");
      long padding=SSDSIZE-upload.totalSize;//Pad out ssd to 200k
      char buf[1024];
      while(padding>1024){
        sdFile.write(buf,1024);
        padding-=1024;
      }
      if(padding){
        sdFile.write(buf,padding);
      }
      sdFile.close();// Close the file again
      hostSelect(BBC);
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/index.htm");      // Redirect the client to the success page
      server.send(303);
      updateMMBTitle();
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
      hostSelect(BBC);
    }
  }
}

void updateSettings() //ssid,pswd etc.
{
  //TODO
  if(server.args()){
    for(int count=0;count<server.args();count++){
      if(server.argName(count)=="ssid"){
        
      }
      
    }
  }
  
}

void getSettings()
{
  String JSON="[";
  JSON+="\""+String(ssid)+"\",";
  JSON+="\""+String(upurl)+"\"]";
  
  server.send(200,"text/plain",JSON);
}

void httpUpdate()//Update code & spiffs
{
  //TODO serve update page with timeout to load index.htm 
  handleNotFound();
  
  server.stop();
  delay(10);
  ESPhttpUpdate.rebootOnUpdate(false);
  Serial.println("Update SPIFFS...");
  t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(String(upurl)+"BBC_WiFi_SDCard.spiffs.bin");
  if(ret == HTTP_UPDATE_OK) {
    Serial.println("Update sketch...");
    ret = ESPhttpUpdate.update(String(upurl)+"BBC_WiFi_SDCard.ino.bin");
    switch(ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;
      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        delay(50);
        ESP.restart();
        break;
    }
  }
}

void ssdDownload()
{
  //TODO calculate the actual size of the image, rather just SDDSIZE
  if(server.args()){
    if(server.argName(0)=="ssd"){
      Serial.print("Download: ");
      Serial.println(server.arg(0));
      long ssd=server.arg(0).toInt();
      char buf[1024];
      server.setContentLength(SSDSIZE);
      server.sendHeader("content-disposition","attachment; filename=\""+GetTitle(ssd)+".ssd\"");
      server.send(200,"application/octet-stream","");
      hostSelect(ESP8266);
      sdFile=SD.open(CurrentMMB,FILE_READ);
      sdFile.seek((ssd*SSDSIZE)+8192);
      for(int count=0;count<SSDSIZE;count+=1024){
        sdFile.read(buf,1024);
        server.sendContent_P(buf,1024);
      }
      sdFile.close();
      hostSelect(BBC);
    }
  }
}

