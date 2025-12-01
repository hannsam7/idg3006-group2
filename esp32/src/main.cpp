#include <Arduino.h>

#define RX_PIN 16
#define TX_PIN 17

HardwareSerial mmwaveSerial(2);

void setup() {
    // USB Serial to PC
    Serial.begin(115200);
    delay(2000);
    
    // Serial to mmWave sensor (same wiring as before)
    mmwaveSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    
    Serial.println("ESP32 Serial Bridge Ready");
    Serial.println("Reading from mmWave sensor on pins 16/17...");
}

void loop() {
    // Forward data from mmWave sensor to PC via USB
    if (mmwaveSerial.available()) {
        String sensorData = mmwaveSerial.readStringUntil('\n');
        sensorData.trim();
        
        if (sensorData.length() > 0) {
            // Send directly (server will parse it)
            Serial.println(sensorData);
        }
    }
    
    // Optional: Forward commands from PC to sensor
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        mmwaveSerial.println(command);
    }
    
    delay(10);
}