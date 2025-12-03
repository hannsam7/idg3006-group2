#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char* ssid = "ESP32_mmwave_AP";
const char* pass = "password123";

AsyncWebServer httpServer(80);
WebSocketsServer wsServer(81);

volatile bool presence = false;
volatile bool motion = false;
volatile int distanceCm = 0;

void wsBroadcast() {
  StaticJsonDocument<128> doc;
  doc["presence"] = presence;
  doc["motion"] = motion;
  doc["distance"] = distanceCm;
  String payload;
  serializeJson(doc, payload);
  wsServer.broadcastTXT(payload);
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    wsBroadcast();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // AP mode so Mac can connect to 192.168.4.1
  WiFi.softAP(ssid, pass);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // typically 192.168.4.1

  wsServer.begin();
  wsServer.onEvent(onWsEvent);
  wsServer.enableHeartbeat(15000, 3000, 2);

  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/plain", "ESP32 mmWave WS server running");
  });
  httpServer.begin();
}

void loop() {
  wsServer.loop();

  // TODO: read mmWave sensor and update presence/motion/distanceCm
  // Example dummy data
  static uint32_t last = millis();
  if (millis() - last > 1000) {
    last = millis();
    presence = !presence;
    motion = presence;
    distanceCm = presence ? 75 : 0;
    wsBroadcast();
  }
}