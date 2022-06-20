/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-plot-readings-charts-multiple/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

//#include <Arduino.h>
//#include <WiFi.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include "SPIFFS.h"
//#include <Arduino_JSON.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>

//#include <EEPROM.h>
#include <SHT21.h>  // include SHT21 library
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include <Wire.h>

SHT21 sht; 

// Replace with your network credentials
const char* ssid = "xxxx";
const char* password = "xxxx";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

AsyncWebSocket ws("/ws");
String message = "";
float temp      = 0;
float humidity  = 0;

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  temp      = sht.getTemperature();
  humidity  = sht.getHumidity();
  readings["temperature"] =  String(temp);
  readings["humidity"]    =  String(humidity);
  bool printit = true;
  if (printit) {  
    Serial.print("Temp ");
    Serial.print(temp);
    Serial.print(" C; Humidity ");
    Serial.print(humidity);
    Serial.println("%");
  }

  String jsonString = JSON.stringify(readings);
  return jsonString;

  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.println(WiFi.localIP());
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Wire.begin();
  initWiFi();
  initFS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
}
