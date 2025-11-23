#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#define RX_PIN 16
#define TX_PIN 17

// Replace with your network information
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// WebSocket server address (Raspberry Pi)
WebSocketsClient webSocket;
const char* wsHost = "192.168.1.50";
const uint16_t wsPort = 8085;
const char* wsPath = "/ws";  // WebSocket endpoint

HardwareSerial mmwaveSerial(2); // UART2 for mmWave sensor

void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_CONNECTED:
            Serial.println("Connected to WebSocket server");
            break;
        case WStype_DISCONNECTED:
            Serial.println("Disconnected from WebSocket server");
            break;
        case WStype_TEXT:
            Serial.printf("Message from server: %s\n", payload);
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

    Serial.println("\nWiFi connected!");
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
        Serial.println("Sensor: " + sensorData);

        // Send to backend
        webSocket.sendTXT(sensorData);
    }
}
