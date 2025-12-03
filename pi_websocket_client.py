import asyncio
import websockets
import json
from aiohttp import web
import threading
from datetime import datetime

# ESP32 Configuration
ESP32_IP = "192.168.4.1"  # Default IP of ESP32 AP
ESP32_WS_PORT = 81

# Store latest sensor data
sensor_data = {
    "presence": False,
    "motion": False,
    "distance": 0,
    "timestamp": None,
    "connected": False
}

# Connected web clients
web_clients = set()

async def connect_to_esp32():
    """Connect to ESP32 WebSocket and receive sensor data"""
    uri = f"ws://{ESP32_IP}:{ESP32_WS_PORT}"
    
    while True:
        try:
            print(f"Connecting to ESP32 at {uri}...")
            async with websockets.connect(uri) as websocket:
                print("✓ Connected to ESP32!")
                sensor_data["connected"] = True
                
                # Broadcast connection status to web clients
                await broadcast_to_clients(json.dumps(sensor_data))
                
                async for message in websocket:
                    # Parse sensor data from ESP32
                    data = json.loads(message)
                    sensor_data.update(data)
                    sensor_data["timestamp"] = datetime.now().isoformat()
                    sensor_data["connected"] = True
                    
                    print(f"Sensor: Presence={data.get('presence')}, "
                          f"Motion={data.get('motion')}, "
                          f"Distance={data.get('distance')}cm")
                    
                    # Forward to web clients
                    await broadcast_to_clients(json.dumps(sensor_data))
                    
        except Exception as e:
            print(f"✗ ESP32 connection error: {e}")
            sensor_data["connected"] = False
            await broadcast_to_clients(json.dumps(sensor_data))
            await asyncio.sleep(5)  # Wait before reconnecting

async def broadcast_to_clients(message):
    """Send data to all connected web clients"""
    if web_clients:
        disconnected = set()
        for ws in web_clients:
            try:
                await ws.send_str(message)
            except:
                disconnected.add(ws)
        
        # Remove disconnected clients
        web_clients.difference_update(disconnected)

async def websocket_handler(request):
    """Handle WebSocket connections from web browsers"""
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    
    web_clients.add(ws)
    print(f"✓ Web client connected (total: {len(web_clients)})")
    
    # Send current sensor data to new client
    await ws.send_str(json.dumps(sensor_data))
    
    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                print(f"Received from client: {msg.data}")
            elif msg.type == web.WSMsgType.ERROR:
                print(f"WebSocket error: {ws.exception()}")
    finally:
        web_clients.discard(ws)
        print(f"✗ Web client disconnected (total: {len(web_clients)})")
    
    return ws

async def index_handler(request):
    """Serve the main HTML page"""
    html = """
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>mmWave Sensor Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 600px;
            width: 100%;
        }
        h1 { 
            color: #667eea;
            text-align: center;
            margin-bottom: 30px;
            font-size: 2em;
        }
        .status-card {
            background: #f8f9fa;
            border-radius: 15px;
            padding: 30px;
            margin: 20px 0;
            text-align: center;
            transition: all 0.3s ease;
        }
        .status-card.detected {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            transform: scale(1.05);
            box-shadow: 0 10px 30px rgba(102, 126, 234, 0.4);
        }
        .status-icon {
            font-size: 80px;
            margin-bottom: 15px;
        }
        .status-text {
            font-size: 32px;
            font-weight: bold;
            margin-bottom: 10px;
        }
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 20px;
        }
        .info-box {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
        }
        .info-label {
            font-size: 14px;
            color: #6c757d;
            margin-bottom: 5px;
        }
        .info-value {
            font-size: 24px;
            font-weight: bold;
            color: #667eea;
        }
        .connection-status {
            text-align: center;
            padding: 10px;
            border-radius: 10px;
            margin-bottom: 20px;
            font-weight: bold;
        }
        .connected {
            background: #d4edda;
            color: #155724;
        }
        .disconnected {
            background: #f8d7da;
            color: #721c24;
        }
        .timestamp {
            text-align: center;
            color: #6c757d;
            font-size: 14px;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌊 mmWave Sensor Dashboard</h1>
        
        <div id="connection" class="connection-status disconnected">
            Connecting to ESP32...
        </div>
        
        <div id="statusCard" class="status-card">
            <div class="status-icon" id="statusIcon">👤</div>
            <div class="status-text" id="statusText">NO PRESENCE</div>
        </div>
        
        <div class="info-grid">
            <div class="info-box">
                <div class="info-label">Motion</div>
                <div class="info-value" id="motionValue">-</div>
            </div>
            <div class="info-box">
                <div class="info-label">Distance</div>
                <div class="info-value" id="distanceValue">-</div>
            </div>
        </div>
        
        <div class="timestamp" id="timestamp">Waiting for data...</div>
    </div>

    <script>
        const ws = new WebSocket('ws://' + window.location.host + '/ws');
        
        ws.onopen = () => {
            console.log('Connected to Raspberry Pi WebSocket');
        };
        
        ws.onclose = () => {
            console.log('Disconnected from WebSocket');
            document.getElementById('connection').textContent = '✗ Disconnected';
            document.getElementById('connection').className = 'connection-status disconnected';
        };
        
        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            console.log('Received:', data);
            
            // Update connection status
            const connEl = document.getElementById('connection');
            if (data.connected) {
                connEl.textContent = '✓ Connected to ESP32';
                connEl.className = 'connection-status connected';
            } else {
                connEl.textContent = '✗ ESP32 Disconnected';
                connEl.className = 'connection-status disconnected';
            }
            
            // Update main status card
            const statusCard = document.getElementById('statusCard');
            const statusIcon = document.getElementById('statusIcon');
            const statusText = document.getElementById('statusText');
            
            if (data.presence) {
                statusCard.className = 'status-card detected';
                if (data.motion) {
                    statusIcon.textContent = '🏃';
                    statusText.textContent = 'MOVING';
                } else {
                    statusIcon.textContent = '🧍';
                    statusText.textContent = 'PRESENT';
                }
            } else {
                statusCard.className = 'status-card';
                statusIcon.textContent = '👤';
                statusText.textContent = 'NO PRESENCE';
            }
            
            // Update info boxes
            document.getElementById('motionValue').textContent = 
                data.motion ? 'YES' : 'NO';
            document.getElementById('distanceValue').textContent = 
                data.distance + ' cm';
            
            // Update timestamp
            if (data.timestamp) {
                const date = new Date(data.timestamp);
                document.getElementById('timestamp').textContent = 
                    'Last update: ' + date.toLocaleTimeString();
            }
        };
        
        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
        };
    </script>
</body>
</html>
    """
    return web.Response(text=html, content_type='text/html')

async def start_web_server():
    """Start the web server for the UI"""
    app = web.Application()
    app.router.add_get('/', index_handler)
    app.router.add_get('/ws', websocket_handler)
    
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', 8080)
    await site.start()
    
    print("\n" + "="*60)
    print("🌐 Raspberry Pi WebSocket Server Started")
    print("="*60)
    print(f"📱 Open browser to: http://localhost:8080")
    print(f"🔗 Or from other device: http://{get_pi_ip()}:8080")
    print("="*60 + "\n")

def get_pi_ip():
    """Get Raspberry Pi's IP address"""
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "localhost"

async def main():
    """Main function to run both servers"""
    # Start web server
    await start_web_server()
    
    # Connect to ESP32 and receive data
    await connect_to_esp32()

if __name__ == "__main__":
    print("\n" + "="*60)
    print("mmWave Sensor - Raspberry Pi Client")
    print("="*60)
    print(f"ESP32 AP IP: {ESP32_IP}")
    print(f"ESP32 WebSocket Port: {ESP32_WS_PORT}")
    print("="*60)
    
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n\n✓ Shutting down gracefully...")