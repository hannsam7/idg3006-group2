# idg3006-group2
UI code

# Look for "IPv4 Address" under your Wi-Fi adapter
ipconfig

# 1 start the node.js server
cd "c:\Users\sanna\Desktop\idg3006-group2\idg3006-group2\raspberry-pi\servr"
npm install
node server.js

# 2 Verify server is listening (open new PowerShell):
netstat -ano | findstr :3000

# 3 Once server is running, watch ESP32 serial (it will auto-reconnect):
cd "C:\Users\sanna\Desktop\idg3006-group2\idg3006-group2"
python -m platformio run --target upload
python -m platformio device monitor -p COM3 -b 115200 

You should see:
✓ WebSocket connected

# 4 Verify mmWave data flow:
→ {"sensorId":"sensor1","raw":"...sensor data..."}

# to do 
I think there is something wrong with the ui. because when i open http://0.0.0.0:3000/ the site cant be reached. and when i open http://127.0.0.1:5501/web-dashboard/public/index.html nothing happens?