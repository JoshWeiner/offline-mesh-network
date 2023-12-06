#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsServer.h>
#include "LittleFS.h"
#include "painlessMesh.h"

#define LED_BUILTIN 2
#define MESH_PREFIX "Chat App Alpha"
#define MESH_PASSWORD "joshisgreat"
#define MESH_PORT 5555

const int HTTP_PORT = 80;
const int WS_PORT = 1337;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

AsyncWebServer server(HTTP_PORT);
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);

// Serve index.html as our captive portal
void handleRoot(AsyncWebServerRequest *request) {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    request->send(404, "text/html", "404: Not Found");
  } else {
    String htmlData = file.readString();
    file.close();
    request->send(200, "text/html", htmlData);
  }
}

// Initialize the Mesh Network
painlessMesh mesh;

void sendMessage();

void sendMessageClient(String msg) {
  Serial.printf("Sending to node %u msg=%s\n", mesh.getNodeId(), msg.c_str());
  mesh.sendBroadcast(msg);
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  webSocket.broadcastTXT(msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
  webSocket.loop();
}

// Test Message for Node Handshake
void broadcastTestMessage() {
  String msg = "Hello from node " + String(mesh.getNodeId());
  mesh.sendBroadcast(msg);
  Serial.println("Test message sent: " + msg);
}

// Used Later with listNodes
unsigned long previousMillis = 0;
const long interval = 5000;

// List ids of nodes on the mesh network
void listNodes() {
  auto nodes = mesh.getNodeList();
  Serial.printf("The mesh has %u nodes:\n", nodes.size());
  for (auto &id : nodes) {
    Serial.printf("Node ID: %u\n", id);
  }
}

void StartMesh() {
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  broadcastTestMessage();
}

// Functions for WebSocket
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        // Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      webSocket.broadcastTXT(payload, length);
      sendMessageClient((char*)payload);
      break;

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

// Functions for LittleFS
void onIndexRequest(AsyncWebServerRequest *request) {
  IPAddress ip = request->client()->remoteIP();
  request->send(LittleFS, "/index.html", "text/html");
}

void onCSSRequest(AsyncWebServerRequest *request) {
  IPAddress ip = request->client()->remoteIP();
  request->send(LittleFS, "/styles.css", "text/css");
}

void onJSRequest(AsyncWebServerRequest *request) {
  IPAddress ip = request->client()->remoteIP();
  request->send(LittleFS, "/scripts.js", "text/js");
}

void onChatRequest(AsyncWebServerRequest *request) {
  IPAddress ip = request->client()->remoteIP();
  request->send(LittleFS, "/chat.html", "text/html");
}

void onChatJSRequest(AsyncWebServerRequest *request) {
  IPAddress ip = request->client()->remoteIP();
  request->send(LittleFS, "/chatScripts.js", "text/js");
}

void StartFilesystem() {
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
}

void ConnectToServer() {
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/styles.css", HTTP_GET, onCSSRequest);
  server.on("/scripts.js", HTTP_GET, onJSRequest);
  server.on("/chat.html", HTTP_GET, onChatRequest);
  server.on("/chatScripts.js", HTTP_GET, onChatJSRequest);
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false);
  });
  server.begin();
}

void ConnectToWebSocket() {
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

// Functions for MDNS
void StartMDNS() {
  if (!MDNS.begin("esp8266")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", HTTP_PORT);
}

// Initialize WiFi
void StartWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Chat AP", MESH_PASSWORD);
  IPAddress apIP = WiFi.softAPIP();
  // Serial.print("Connecting to WiFi");
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(apIP);
  Serial.print("Connect to: ");
  Serial.println("Chat AP");
  digitalWrite(LED_BUILTIN, HIGH);
}

// On node startup
void setup() {
  Serial.begin(115200);
  
  StartFilesystem();
  StartMesh();
  // StartWiFi();
  ConnectToServer();
  ConnectToWebSocket();
  StartMDNS();

  dnsServer.start(DNS_PORT, "*", apIP);

  
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

// Recurring function calls
void loop() {
  mesh.update();
  webSocket.loop();
  dnsServer.processNextRequest();

  digitalWrite(LED_BUILTIN, HIGH);

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    listNodes();
  }

  // =========== BELOW: Working single node serving web pages ==============
  // if (WiFi.softAPgetStationNum() == 0) {
  //   delay(100);
  //   // digitalWrite(LED_BUILTIN, LOW);
  // }
  // else {
  //   delay(100);
  //   dnsServer.processNextRequest();
  //   // digitalWrite(LED_BUILTIN, HIGH);
  //   webServer.handleClient();
  // }
}
