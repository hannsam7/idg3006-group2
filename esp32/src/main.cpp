#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#define RX_PIN 16
#define TX_PIN 17

// ===== Normal Wi-Fi (WPA2-PSK) =====
const char* ssid = "Omni_9122A0";           // ← Change to your Wi-Fi name
const char* password = "under6597";   // ← Change to your Wi-Fi password

// WebSocket server
const char* wsHost = "192.168.39.101";  // ← Change to your PC's IP on same Wi-Fi
const uint16_t wsPort = 3000;
const char* wsPath = "/ws";

WebSocketsClient webSocket;
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
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    mmwaveSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    
    Serial.println("\n=================================");
    Serial.println("ESP32 WiFi (WPA2-PSK) Connection");
    Serial.println("=================================");
    
    // Reset WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    WiFi.mode(WIFI_STA);
    delay(500);
    
    Serial.print("Connecting to: ");
    Serial.println(ssid);
    
    // Simple WPA2-PSK connect (no EAP)
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.println();
            Serial.print("Status (");
            Serial.print(attempts);
            Serial.print("s): ");
            switch(WiFi.status()) {
                case WL_NO_SSID_AVAIL:
                    Serial.println("Can't find network");
                    break;
                case WL_CONNECT_FAILED:
                    Serial.println("Connection failed - check password");
                    break;
                case WL_DISCONNECTED:
                    Serial.println("Disconnected");
                    break;
                default:
                    Serial.println(WiFi.status());
            }
            Serial.print("Retrying");
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓✓✓ SUCCESS ✓✓✓");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("MAC: ");
        Serial.println(WiFi.macAddress());
        
        // Initialize WebSocket
        webSocket.begin(wsHost, wsPort, wsPath);
        webSocket.onEvent(onWebSocketEvent);
        webSocket.setReconnectInterval(5000);
        webSocket.enableHeartbeat(15000, 3000, 2);
        Serial.println("✓ WebSocket ready");
        
    } else {
        Serial.println("\n✗ FAILED to connect");
        Serial.println("\nCheck:");
        Serial.println("  - SSID is correct");
        Serial.println("  - Password is correct");
        Serial.println("  - Wi-Fi is in range");
        Serial.print("\nYour MAC: ");
        Serial.println(WiFi.macAddress());
    }
}

void loop() {
    webSocket.loop();
    
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, reconnecting...");
            WiFi.begin(ssid, password);
        }
    }
    
    // Read mmWave sensor data
    if (mmwaveSerial.available()) {
        String sensorData = mmwaveSerial.readStringUntil('\n');
        sensorData.trim();
        
        if (sensorData.length() > 0) {
            String json = "{\"sensorId\":\"sensor1\",\"raw\":\"" + sensorData + "\"}";
            Serial.println("→ " + json);
            
            if (webSocket.isConnected()) {
                webSocket.sendTXT(json);
            }
        }
    }
    
    delay(10);
}