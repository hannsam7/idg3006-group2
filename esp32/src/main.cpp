#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_wpa2.h" // Required for WPA2-Enterprise

#define RX_PIN 16
#define TX_PIN 17

// eduroam credentials - UPDATE THESE
const char* ssid = "eduroam";
const char* EAP_IDENTITY = "sannasj@stud.ntnu.no"; // Your university username
const char* EAP_PASSWORD = "KimNamjoon1994"; // Your university password

// WebSocket server (Raspberry Pi)
WebSocketsClient webSocket;
const char* wsHost = "192.168.1.50"; // UPDATE if needed
const uint16_t wsPort = 3000;
const char* wsPath = "/ws";

HardwareSerial mmwaveSerial(2);

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("✓ WebSocket connected");
            break;
        case WStype_DISCONNECTED:
            Serial.println("✗ WebSocket disconnected");
            break;
        case WStype_TEXT:
            Serial.printf("← Server: %s\n", payload);
            break;
        case WStype_ERROR:
            Serial.println("✗ WebSocket error");
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    
    mmwaveSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    
    Serial.println("\n=== ESP32 mmWave Sensor ===");
    Serial.println("Connecting to eduroam...");
    
    // Disconnect if previously connected
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    
    // Configure WPA2-Enterprise
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_wpa2_ent_enable();

    // Connect to eduroam
    WiFi.begin(ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ eduroam connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        // Initialize WebSocket
        webSocket.begin(wsHost, wsPort, wsPath);
        webSocket.onEvent(onWebSocketEvent);
        webSocket.setReconnectInterval(5000);
        webSocket.enableHeartbeat(15000, 3000, 2);
        
        Serial.println("WebSocket initialized");
    } else {
        Serial.println("\n✗ Failed to connect to eduroam");
        Serial.println("Please check:");
        Serial.println("  - Username/password are correct");
        Serial.println("  - eduroam is available");
        Serial.println("  - ESP32 is in range");
    }
}

void loop() {
    webSocket.loop();
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting reconnect...");
        WiFi.begin(ssid);
        delay(5000);
    }
    
    // Read mmWave sensor data
    if (mmwaveSerial.available()) {
        String sensorData = mmwaveSerial.readStringUntil('\n');
        sensorData.trim();
        
        if (sensorData.length() > 0) {
            // Create JSON payload
            String json = String("{\"sensorId\":\"sensor1\",\"raw\":\"") + sensorData + "\"}";
            
            Serial.println("→ Sending: " + json);
            
            if (webSocket.isConnected()) {
                webSocket.sendTXT(json);
            } else {
                Serial.println("✗ WebSocket not connected, data not sent");
            }
        }
    }
    
    delay(10); // Small delay to prevent watchdog issues
}
