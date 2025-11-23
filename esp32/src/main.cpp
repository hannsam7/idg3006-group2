#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#define RX_PIN 16
#define TX_PIN 17

// WiFi credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// WebSocket server (Raspberry Pi)
WebSocketsClient webSocket;
const char* wsHost = "192.168.1.50";
const uint16_t wsPort = 8085;
const char* wsPath = "/ws";

HardwareSerial mmwaveSerial(2); // UART2

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("WebSocket connected");
            break;
        case WStype_DISCONNECTED:
            Serial.println("WebSocket disconnected");
            break;
        case WStype_TEXT:
            Serial.printf("Server message: %s\n", payload);
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    mmwaveSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    webSocket.begin(wsHost, wsPort, wsPath);
    webSocket.onEvent(onWebSocketEvent);
    webSocket.setReconnectInterval(5000);
    webSocket.enableHeartbeat(15000, 3000, 2);
}

void loop() {
    webSocket.loop();

    if (mmwaveSerial.available()) {
        String sensorData = mmwaveSerial.readStringUntil('\n');
        sensorData.trim();
        if (sensorData.length()) {
            String json = String("{\"sensorId\":\"sensor1\",\"raw\":\"") + sensorData + "\"}";
            Serial.println("Sending: " + json);
            webSocket.sendTXT(json);
        }
    }
}
