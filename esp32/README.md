# ESP32 Firmware

This directory contains the firmware responsible for:
- Reading human presence data from the mmWave radar sensor
- Parsing the sensor values
- Connecting to Wi-Fi
- Sending live occupancy data to the Raspberry Pi WebSocket backend

## Requirements
- ESP32 DevKit
- mmWave Sensor (e.g., MR24HPB1 / LD2410 / IWR6843)
- PlatformIO or Arduino IDE

## Running the Code

### 1. Configure Wi-Fi
Edit `main.cpp`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
