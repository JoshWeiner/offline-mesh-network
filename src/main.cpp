#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "LittleFS.h"
#include "painlessMesh.h"

#define LED_BUILTIN 2
#define MESH_PREFIX "Offline Chat Server"
#define MESH_PASSWORD "test"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

const char* ssid = "Offline Chat Server";
const char* password = "test";

ESP8266WebServer webServer(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;
IPAddress apIP(192, 168, 1, 1);

void sendMessage();

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String message = "Hello from";
  message += mesh.getNodeId();
  mesh.sendBroadcast(message);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if(!file){
    Serial.println("Failed to open file for reading");
    webServer.send(404, "text/html", "404 Not Found");
  }
  else {
    String htmlData = file.readString();
    file.close();
    webServer.send(200, "text/html", htmlData);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION ); 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
 
  
  // =========== BELOW: Working single node serving web pages ==============
  // pinMode(LED_BUILTIN, OUTPUT);

  // WiFi.mode(WIFI_AP);
  // WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  // WiFi.softAP(ssid);

  // if(!LittleFS.begin()){
  //   Serial.println("An Error has occurred while mounting SPIFFS");
  //   return;
  // }
  
  // dnsServer.start(DNS_PORT, "*", apIP);
  // webServer.onNotFound([]() {
  //   handleRoot();
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   delay(500);
  //   digitalWrite(LED_BUILTIN, LOW);
  // });

  // IPAddress ipAddr = WiFi.softAPIP();
  // Serial.print("IP address: ");
  // Serial.println(ipAddr);

  // webServer.begin();
}

void loop() {
  mesh.update();

  // =========== BELOW: Working single node serving web pages ==============
  if (WiFi.softAPgetStationNum() == 0) {
    delay(100);
    // digitalWrite(LED_BUILTIN, LOW);
  }
  else {
    delay(100);
    dnsServer.processNextRequest();
    // digitalWrite(LED_BUILTIN, HIGH);
    webServer.handleClient();
  }
}