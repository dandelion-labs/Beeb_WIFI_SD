

void mmbFileList()
{
 String JSON="[";
 String FileName="";
 Serial.println("MMBData");
 hostSelect(ESP);
 File dir=SD.open("/");
 dir.rewindDirectory();
 while(true){
  File entry=dir.openNextFile();
  if(!entry) break;
  FileName=entry.name();
  if(FileName.endsWith(".MMB")){
    if(JSON!="[") JSON+=",";
    JSON+="\"";
    JSON+=FileName;
    JSON+="\"";
  }
  entry.close();
 }
 JSON+="]";
 dir.close();
 hostSelect(BBC);
 server.send(200,"text/plain",JSON);
}

void ssdData()
{
  String MMBFile;
  String JSON;
  int pos=16;
  char buf[16];
  Serial.println("SSDData");
  Serial.println("Server Args");
  Serial.println(server.args());
  
  if(server.args()>0){
    Serial.println(server.arg(0));
    MMBFile=server.arg(0);
  }
  if(!MMBFile.endsWith(".MMB")){
    handleNotFound();
    return;
  }
  hostSelect(ESP);
  sdFile=SD.open(MMBFile,FILE_READ);
  //TODO check for file open error
  CurrentMMB=MMBFile;
  JSON="[";
  for(int count1=0;count1<510;count1++){
    
    sdFile.seek(pos);
    sdFile.read(buf,16);
    pos+=16;
    if(buf[15]<240){//bit 15 -Status bit 0=Readonly,15=Read/write,240=unformatted,255=invalid
      if(JSON!="[") JSON+=",";
      JSON+="\""+String(count1)+"\",";
      JSON+="\"";
      for(int count=0;count<12;count++){
        if(buf[count]!=0) JSON+=buf[count];
      }
      JSON+="\"";
    }
    
  }
  JSON+="]";
  sdFile.close();
  hostSelect(BBC);
  server.send(200,"text/plain",JSON);
}

void updateMMBTitle(){
  Serial.println("Update MMB Title");
  Serial.println(NextFree);
  Serial.println((NextFree*204800)+8192);
  char DiskTitle1[8];
  char DiskTitle2[4];
  hostSelect(ESP);
  sdFile=SD.open(CurrentMMB,FILE_READ);
  sdFile.seek((NextFree*204800)+8192);
  sdFile.read(DiskTitle1,8);
  sdFile.seek((NextFree*204800)+8448);
  sdFile.read(DiskTitle2,4);
  sdFile.close();
  sdFile=SD.open(CurrentMMB,FILE_WRITE);
  sdFile.seek((NextFree*16)+16);
  sdFile.write(DiskTitle1,8);
  sdFile.write(DiskTitle2,4);
  sdFile.seek((NextFree*16)+31);
  sdFile.write(byte(0)); //Read Only
  sdFile.close();
  hostSelect(BBC);
}

void checkNextFree()
{
  int pos=16;
  char buf[16];
  NextFree=511;
  hostSelect(ESP);
  sdFile=SD.open(CurrentMMB,FILE_READ);
  for(int count1=0;count1<510;count1++){
    sdFile.seek(pos);
    sdFile.read(buf,16);
    pos+=16;
    if(buf[15]==240 && NextFree==511){//Find first unformatted ssd
      NextFree=count1; 
      break;
    }
  }
  sdFile.close();
  hostSelect(BBC);
  Serial.print("Next Free ssd: ");
  Serial.println(NextFree);
}

