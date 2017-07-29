#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>


#include <SPI.h>
#include <SD.h>
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h
#include "FS.h"

#define BBC 0
#define ESP 1

 char ssid[] = "";
 char password[] = "";

ESP8266WebServer server ( 80 );

File sdFile;
//fs::File fsUploadFile;


int NextFree=511; //Used to find first free ssd in mmb
String CurrentMMB; //Currently selected MMB file


void setup() {
  // put your setup code here, to run once:
  WiFi.hostname("BeebSD-");// + String(ESP.getChipId(), HEX));
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  hostSelect(BBC);


  Serial.begin ( 115200 );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );

  if ( MDNS.begin ( "beebsd" ) ) {
    Serial.println ( "MDNS responder started" );
  }

 SPIFFS.begin();                           // Start the SPI Flash Files System

 serverStart(); // Start the web server




  
  Serial.print("Initializing SD card...");
  hostSelect(ESP);
  //delay(10);
  if (!SD.begin()) {
    Serial.println("initialization failed!");
    hostSelect(BBC);
    return;
  }
  Serial.println("initialization done.");
/*
  if (SD.exists("BEEB.MMB")) {
    Serial.println("BEEB.MMB exists.");
    readMMBDir();
  }
  else {
    Serial.println("BEEB.MMB doesn't exist.");
  }
*/
  hostSelect(BBC);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}

void hostSelect(boolean host){
  delay(10);
  if(host==BBC){
    digitalWrite(D2,LOW);
    digitalWrite(D1,HIGH);
  }
  if(host==ESP){
    digitalWrite(D1,LOW);
    digitalWrite(D2,HIGH);
  }
  delay(10);
}

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password)>0?"********":"<no password>");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}


