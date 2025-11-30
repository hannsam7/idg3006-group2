// #include <Arduino.h>
// #include <WiFi.h>
// #include <WebSocketsClient.h>
// #include "esp_wpa2.h"

// #define RX_PIN 16
// #define TX_PIN 17

// // ===== NTNU eduroam credentials (based on wpa_supplicant config) =====
// const char* ssid = "Omni_9122A0";

// // Format from wpa_supplicant: identity="[name.surname]@ntnu.no"
// const char* EAP_IDENTITY = "";     // ← YOUR full NTNU email
// const char* EAP_PASSWORD = "under6597";    // ← Plain text (ESP32 doesn't use hash)

// // WebSocket server
// const char* wsHost = "10.22.110.94";  // ← Your PC's IP
// const uint16_t wsPort = 3000;
// const char* wsPath = "/ws";

// WebSocketsClient webSocket;
// HardwareSerial mmwaveSerial(2);

// void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
//     switch (type) {
//         case WStype_CONNECTED:
//             Serial.println("✓ WebSocket connected");
//             break;
//         case WStype_DISCONNECTED:
//             Serial.println("✗ WebSocket disconnected");
//             break;
//         case WStype_TEXT:
//             Serial.printf("← Server: %s\n", payload);
//             break;
//         default:
//             break;
//     }
// }

// void setup() {
//     Serial.begin(115200);
//     delay(2000);
    
//     mmwaveSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    
//     Serial.println("\n=================================");
//     Serial.println("ESP32 NTNU eduroam Connection");
//     Serial.println("Using PEAP/MSCHAPv2");
//     Serial.println("=================================");
    
//     // Reset WiFi
//     WiFi.disconnect(true);
//     WiFi.mode(WIFI_OFF);
//     delay(500);
//     WiFi.mode(WIFI_STA);
//     delay(500);
    
//     Serial.print("Username: ");
//     Serial.println(EAP_IDENTITY);
//     Serial.println("Configuring PEAP/MSCHAPv2...");
    
//     // Configure based on wpa_supplicant settings
//     esp_wifi_sta_wpa2_ent_set_identity(
//         (uint8_t *)EAP_IDENTITY, 
//         strlen(EAP_IDENTITY)
//     );
    
//     esp_wifi_sta_wpa2_ent_set_username(
//         (uint8_t *)EAP_IDENTITY, 
//         strlen(EAP_IDENTITY)
//     );
    
//     esp_wifi_sta_wpa2_ent_set_password(
//         (uint8_t *)EAP_PASSWORD, 
//         strlen(EAP_PASSWORD)
//     );
    
//     // Disable time check (like phase1 settings)
//     esp_wifi_sta_wpa2_ent_set_disable_time_check(true);
    
//     // Enable enterprise auth
//     esp_wifi_sta_wpa2_ent_enable();
    
//     Serial.print("Connecting to wifi");
//     WiFi.begin(ssid, EAP_PASSWORD);
    
//     int attempts = 0;
//     while (WiFi.status() != WL_CONNECTED && attempts < 60) {
//         delay(1000);
//         Serial.print(".");
//         attempts++;
        
//         if (attempts % 10 == 0) {
//             Serial.println();
//             Serial.print("Status (");
//             Serial.print(attempts);
//             Serial.print("s): ");
//             switch(WiFi.status()) {
//                 case WL_NO_SSID_AVAIL:
//                     Serial.println("Can't find eduroam network");
//                     break;
//                 case WL_CONNECT_FAILED:
//                     Serial.println("Connection failed - check credentials");
//                     break;
//                 case WL_DISCONNECTED:
//                     Serial.println("Disconnected - authentication issue?");
//                     break;
//                 default:
//                     Serial.println(WiFi.status());
//             }
//             Serial.print("Retrying");
//         }
//     }
    
//     Serial.println();
    
//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("\n✓✓✓ SUCCESS ✓✓✓");
//         Serial.println("Connected to NTNU eduroam!");
//         Serial.print("IP: ");
//         Serial.println(WiFi.localIP());
//         Serial.print("Signal: ");
//         Serial.print(WiFi.RSSI());
//         Serial.println(" dBm");
//         Serial.print("MAC: ");
//         Serial.println(WiFi.macAddress());
        
//         // Initialize WebSocket
//         webSocket.begin(wsHost, wsPort, wsPath);
//         webSocket.onEvent(onWebSocketEvent);
//         webSocket.setReconnectInterval(5000);
//         webSocket.enableHeartbeat(15000, 3000, 2);
//         Serial.println("✓ WebSocket ready");
        
//     } else {
//         Serial.println("\n✗ FAILED");
//         Serial.println("\nTry these username formats:");
//         Serial.println("  hanns.sam@ntnu.no");
//         Serial.println("  hanns.sam@student.ntnu.no");
//         Serial.println("  hannsam@ntnu.no");
//         Serial.print("\nYour MAC (for IT support): ");
//         Serial.println(WiFi.macAddress());
//     }
// }

// void loop() {
//     webSocket.loop();
    
//     static unsigned long lastCheck = 0;
//     if (millis() - lastCheck > 10000) {
//         lastCheck = millis();
//         if (WiFi.status() != WL_CONNECTED) {
//             Serial.println("Reconnecting...");
//             WiFi.begin(ssid);
//         }
//     }
    
//     if (mmwaveSerial.available()) {
//         String sensorData = mmwaveSerial.readStringUntil('\n');
//         sensorData.trim();
        
//         if (sensorData.length() > 0) {
//             String json = "{\"sensorId\":\"sensor1\",\"raw\":\"" + sensorData + "\"}";
//             Serial.println("→ " + json);
            
//             if (webSocket.isConnected()) {
//                 webSocket.sendTXT(json);
//             }
//         }
//     }
    
//     delay(10);
// }