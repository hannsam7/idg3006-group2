#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#define RX_PIN 16
#define TX_PIN 17

// WiFi credentials (SET REAL VALUES)
const char* ssid     = "Hanna banna";        // 2.4 GHz SSID
const char* password = "12345678";    // Wi-Fi password

// WebSocket server (Mac while testing)
WebSocketsClient webSocket;
const char* wsHost = "10.24.11.99";           // your Mac's IP
const uint16_t wsPort = 8085;
const char* wsPath = "/ws";                   // <-- missing semicolon fixed

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
