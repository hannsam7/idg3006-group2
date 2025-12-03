#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>

// WiFi Access Point Configuration
const char* ap_ssid = "mmWave-Sensor";
const char* ap_password = "sensor123";  // Change if needed (min 8 chars)

// Serial communication with SEN0395
#define RXD2 16  // ESP32 RX pin (connect to sensor TX)
#define TXD2 17  // ESP32 TX pin (connect to sensor RX)
HardwareSerial SensorSerial(2);

// WebSocket server on port 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Web server for serving HTML page
AsyncWebServer server(80);

// Detection state
struct SensorData {
  bool presence = false;
  bool motion = false;
  int distance = 0;
  unsigned long lastUpdate = 0;
} sensorData;

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %s\n", num, ip.toString().c_str());
      
      // Send current state to newly connected client
      String json = "{\"presence\":" + String(sensorData.presence) + 
                    ",\"motion\":" + String(sensorData.motion) + 
                    ",\"distance\":" + String(sensorData.distance) + "}";
      webSocket.sendTXT(num, json);
      break;
    }
      
    case WStype_TEXT:
      Serial.printf("[%u] Received: %s\n", num, payload);
      break;
  }
}

// Parse SEN0395 data format: $JYBSS,presence,motion,distance,...
void parseSensorData(String data) {
  if (data.startsWith("$JYBSS")) {
    int idx1 = data.indexOf(',');
    int idx2 = data.indexOf(',', idx1 + 1);
    int idx3 = data.indexOf(',', idx2 + 1);
    int idx4 = data.indexOf(',', idx3 + 1);
    
    if (idx1 > 0 && idx2 > 0 && idx3 > 0) {
      String presenceStr = data.substring(idx1 + 1, idx2);
      String motionStr = data.substring(idx2 + 1, idx3);
      String distanceStr = data.substring(idx3 + 1, idx4);
      
      bool newPresence = (presenceStr == "1");
      bool newMotion = (motionStr == "1");
      int newDistance = distanceStr.toInt();
      
      // Check if state changed
      if (newPresence != sensorData.presence || 
          newMotion != sensorData.motion ||
          abs(newDistance - sensorData.distance) > 10) {
        
        sensorData.presence = newPresence;
        sensorData.motion = newMotion;
        sensorData.distance = newDistance;
        sensorData.lastUpdate = millis();
        
        // Broadcast to all connected WebSocket clients
        String json = "{\"presence\":" + String(sensorData.presence) + 
                      ",\"motion\":" + String(sensorData.motion) + 
                      ",\"distance\":" + String(sensorData.distance) + 
                      ",\"timestamp\":" + String(sensorData.lastUpdate) + "}";
        
        webSocket.broadcastTXT(json);
        
        // Debug output
        Serial.print("Detection: ");
        Serial.print(sensorData.presence ? "PRESENT" : "ABSENT");
        Serial.print(" | Motion: ");
        Serial.print(sensorData.motion ? "YES" : "NO");
        Serial.print(" | Distance: ");
        Serial.println(sensorData.distance);
      }
    }
  }
}

// HTML page served at root
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>mmWave Sensor</title>
  <style>
    body { 
      font-family: Arial; 
      text-align: center; 
      margin: 20px;
      background: #1a1a1a;
      color: #fff;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      padding: 20px;
      background: #2a2a2a;
      border-radius: 10px;
    }
    h1 { color: #4CAF50; }
    .status {
      font-size: 48px;
      margin: 30px 0;
      padding: 30px;
      border-radius: 10px;
      transition: all 0.3s;
    }
    .detected { 
      background: #4CAF50; 
      color: white;
      box-shadow: 0 0 30px #4CAF50;
    }
    .not-detected { 
      background: #333; 
      color: #666;
    }
    .info {
      font-size: 20px;
      margin: 10px 0;
      padding: 15px;
      background: #333;
      border-radius: 5px;
    }
    .connection {
      padding: 10px;
      border-radius: 5px;
      margin-bottom: 20px;
    }
    .connected { background: #4CAF50; }
    .disconnected { background: #f44336; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🌊 mmWave Sensor Monitor</h1>
    <div id="connection" class="connection disconnected">Connecting...</div>
    <div id="status" class="status not-detected">NO PRESENCE</div>
    <div id="motion" class="info">Motion: -</div>
    <div id="distance" class="info">Distance: -</div>
    <div id="timestamp" class="info">Last update: -</div>
  </div>

  <script>
    const ws = new WebSocket('ws://' + window.location.hostname + ':81');
    
    ws.onopen = () => {
      console.log('WebSocket connected');
      document.getElementById('connection').textContent = '✓ Connected';
      document.getElementById('connection').className = 'connection connected';
    };
    
    ws.onclose = () => {
      console.log('WebSocket disconnected');
      document.getElementById('connection').textContent = '✗ Disconnected';
      document.getElementById('connection').className = 'connection disconnected';
    };
    
    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      console.log('Received:', data);
      
      const statusEl = document.getElementById('status');
      if (data.presence) {
        statusEl.textContent = data.motion ? '🏃 MOVING' : '🧍 PRESENT';
        statusEl.className = 'status detected';
      } else {
        statusEl.textContent = 'NO PRESENCE';
        statusEl.className = 'status not-detected';
      }
      
      document.getElementById('motion').textContent = 
        'Motion: ' + (data.motion ? 'YES' : 'NO');
      document.getElementById('distance').textContent = 
        'Distance: ' + data.distance + ' cm';
      
      if (data.timestamp) {
        const date = new Date();
        document.getElementById('timestamp').textContent = 
          'Last update: ' + date.toLocaleTimeString();
      }
    };
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP32 mmWave WebSocket Server");
  Serial.println("==============================");
  
  // Initialize sensor serial
  SensorSerial.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("✓ Sensor serial initialized");
  
  // Create WiFi Access Point
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.println("\n✓ WiFi Access Point created");
  Serial.println("  SSID: " + String(ap_ssid));
  Serial.println("  Password: " + String(ap_password));
  Serial.println("  IP Address: " + IP.toString());
  Serial.println("\n🌐 Open browser to: http://" + IP.toString());
  Serial.println("==============================\n");
  
  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("✓ WebSocket server started on port 81");
  
  // Serve HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.begin();
  Serial.println("✓ Web server started on port 80\n");
}

void loop() {
  webSocket.loop();
  
  // Read data from sensor
  if (SensorSerial.available()) {
    String data = SensorSerial.readStringUntil('\n');
    data.trim();
    if (data.length() > 0) {
      parseSensorData(data);
    }
  }
  
  delay(10);
}