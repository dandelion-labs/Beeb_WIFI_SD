
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h
#include "FS.h"
#include <EEPROM.h>
#include <SPI.h> 
#include <SD.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define BBC 0
#define ESP8266 1
#define connectTries 20
#define SSDSIZE 204800

char ssid[33];
char password[51];
char upurl[201];
char hname[12];
byte mac[6];
ESP8266WebServer server (80);

File sdFile;



int NextFree=511; //Used to find first free ssd in mmb
String CurrentMMB; //Currently selected MMB file


void setup() {
    // put your setup code here, to run once:
  //Setup SD Card select

  
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  hostSelect(BBC);
  
  Serial.begin ( 115200 );

  
  //Hostname
  WiFi.macAddress(mac);
  String temp="BeebSD-" + String(mac[0],HEX) + String(mac[1],HEX);
  temp.toUpperCase();
  Serial.println(temp);
  temp.toCharArray(hname,12);
  WiFi.disconnect();  //Clear WiFi credentials
  delay(100);
  WiFi.hostname(hname);
  
  loadCredentials();

  //Save credentials - debug only
  //saveCredentials();
 
  WiFi.mode(WIFI_STA);
  WiFi.begin ( ssid, password );

  // Wait for connection
  int count=0;
  while ( WiFi.status() != WL_CONNECTED ) { //STA Mode
    delay ( 500 );
    Serial.print (".");
    count++;
    if(count>connectTries) { //STA Mode failed, switch to AP Mode
      //SoftAP default IP 192.168.4.1
      Serial.println();
      Serial.println("Switching to AP Mode");
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);
      delay(100);
      WiFi.softAP(hname);
      break;
    }
  }
  if(count<=connectTries){ //Connected in STA Mode
    Serial.println ();
    Serial.print ("Connected to ");
    Serial.println ( ssid );
    Serial.print ("IP address: ");
    Serial.println (WiFi.localIP());
  } else {  //Connected in AP Mode
    Serial.println ();
    Serial.print ("IP address: ");
    Serial.println (WiFi.softAPIP());
  }
  
  //Start mDNS
  if (MDNS.begin ("beebsd")) {
    Serial.println ("MDNS responder started");
  }

  SPIFFS.begin();                           // Start the SPI Flash Files System

  serverStart(); // Start the web server

  Serial.print("Initializing SD card...");
  hostSelect(ESP8266);
  
  if (!SD.begin()) {
    Serial.println("initialization failed!");
    hostSelect(BBC);
    return;
  }
  Serial.println("initialization done.");
  
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
  if(host==ESP8266){
    digitalWrite(D1,LOW);
    digitalWrite(D2,HIGH);
  }
  delay(10);
}

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  Serial.println("Load credentials...");
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  EEPROM.get(0+sizeof(ssid)+sizeof(password),upurl);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(upurl), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password)>0?"********":"<no password>");
  Serial.println(upurl);
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  EEPROM.put(0+sizeof(ssid)+sizeof(password),upurl);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(ssid)+sizeof(password)+sizeof(upurl), ok);
  EEPROM.commit();
  EEPROM.end();
}


