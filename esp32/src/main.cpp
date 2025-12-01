#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_system.h"

#define RX_PIN 16
#define TX_PIN 17

// SET REAL VALUES
const char* ssid     = "Hanna banna";      // 2.4 GHz SSID
const char* password = "12345678";  // Wi‑Fi password
const char* wsHost   = "172.20.10.5";       // Your Mac IP (ipconfig getifaddr en0)
const uint16_t wsPort = 8085;
const char* wsPath   = "/ws";

HardwareSerial mmwaveSerial(2);
WebSocketsClient webSocket;

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) Serial.println("WebSocket connected");
  else if (type == WStype_DISCONNECTED) Serial.println("WebSocket disconnected");
  else if (type == WStype_TEXT) Serial.printf("WS msg: %s\n", payload);
}

static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    // See wifi_err_reason_t for codes; printing numeric is enough for debugging
    Serial.printf("\nWiFi disconnected, reason=%d\n", info.wifi_sta_disconnected.reason);
  } else {
    Serial.printf("\nWiFi event: %d\n", event);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("Reset reason: %d\n", (int)esp_reset_reason());
  mmwaveSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  mmwaveSerial.println("PING");
  mmwaveSerial.println("start"); 

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onWiFiEvent); 
  Serial.println("Connecting to WiFi...");
  WiFi.disconnect(true, true);  
  delay(200);

  int n = WiFi.scanNetworks();
Serial.printf("Found %d networks:\n", n);
for (int i = 0; i < n; i++) {
  Serial.printf("  %2d: %s (ch %d RSSI %d)\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
}

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\nWiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  WiFi.setSleep(false);

  webSocket.begin(wsHost, wsPort, wsPath);
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);

  Serial.println("ESP32 mmWave -> WS bridge ready");
}

unsigned long lastSensor = 0;

void loop() {
  webSocket.loop();

  if (mmwaveSerial.available()) {
    String sensorData = mmwaveSerial.readStringUntil('\n');
    sensorData.trim();
    if (sensorData.length()) {
      Serial.println("RAW: " + sensorData);
      String json = String("{\"sensorId\":\"sensor1\",\"raw\":\"") + sensorData + "\"}";
      webSocket.sendTXT(json);
      lastSensor = millis();
    }
    while (mmwaveSerial.available()) {
        int b = mmwaveSerial.read();
        Serial.printf("0x%02X ", b);
      }
  }

  // Optional: periodic notice if no sensor data yet
  if (!lastSensor && millis() > 5000) {
    Serial.println("Waiting for sensor data (check wiring / baud)...");
    lastSensor = 1; // prevent spamming
  }

  // Forward commands from USB Serial to sensor
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.length()) {
      Serial.println("TX->Sensor: " + command);
      mmwaveSerial.println(command);
    }
  }

  delay(5);
}