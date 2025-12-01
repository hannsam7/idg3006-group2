import express from "express";
import { WebSocketServer, WebSocket } from "ws";
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";
import http from "http";
import path from "path";
import { fileURLToPath } from "url";

const PORT = 3000;
const WS_PATH = "/ws";

// ⚠️ IMPORTANT: Change this to match your mmWave sensor's COM port
// Find it in: Device Manager → Ports (COM & LPT)
const SERIAL_PORT = "COM3";  // Could be COM3, COM4, COM5, etc.
const BAUD_RATE = 115200;    // Match your sensor's baud rate

// In-memory state
let presence = false;
let lastRaw = "";
let lastUpdate = null;

// Derive presence from a raw line (same logic as raspberry-pi/server/server.js)
function derivePresence(raw) {
    const line = raw.trim();
    if (!line) return false;
    
    // Try JSON with numeric target fields
    try {
      const obj = JSON.parse(line);
      for (const k of ["targets","targetCount","count"]) {
        if (typeof obj[k] === "number") return obj[k] > 0;
      }
      if (typeof obj.presence === "boolean") return obj.presence;
      if (typeof obj.occupied === "boolean") return obj.occupied;
    } catch {}
    
    // key=value pattern: targets=1
    const kv = line.match(/\b(targets?|count)\s*=\s*(\d+)/i);
    if (kv) return parseInt(kv[2],10) > 0;
    
    // simple CSV: second field numeric count
    const parts = line.split(/[,;]/).map(p=>p.trim());
    if (parts.length >= 2 && /^\d+$/.test(parts[1])) return parseInt(parts[1],10) > 0;
    
    // fallback keywords
    return /\b(person|human|occupied|presence|target)\b/i.test(line);
}

const app = express();

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Serve web dashboard
app.use(express.static(path.join(__dirname, "web-dashboard/public")));

// REST endpoint for polling
app.get("/api/state", (_req, res) => {
  res.json({ presence, lastRaw, lastUpdate });
});

const server = http.createServer(app);

// WebSocket server
const wss = new WebSocketServer({ server, path: WS_PATH });

// Broadcast helper
function broadcast(obj) {
  const msg = JSON.stringify(obj);
  for (const client of wss.clients) {
    if (client.readyState === WebSocket.OPEN) client.send(msg);
  }
}

wss.on("connection", (ws, req) => {
  console.log("📱 Browser connected");
  ws.send(JSON.stringify({ type: "state", presence, lastRaw, lastUpdate }));

  ws.on("close", () => console.log("📱 Browser disconnected"));
});

// ========================================
// Serial Port Setup (mmWave Sensor)
// ========================================

console.log(`\n🔌 Attempting to open serial port: ${SERIAL_PORT} at ${BAUD_RATE} baud`);

const serialPort = new SerialPort({
  path: SERIAL_PORT,
  baudRate: BAUD_RATE,
});

const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

serialPort.on("open", () => {
  console.log(`✅ Serial port ${SERIAL_PORT} opened successfully!`);
  console.log(`📡 Reading data from mmWave sensor...\n`);
});

serialPort.on("error", (err) => {
  console.error("\n❌ Serial port error:", err.message);
  console.log("\n🔧 Troubleshooting:");
  console.log("1. Check mmWave sensor is connected via USB");
  console.log("2. Find the correct COM port:");
  console.log("   - Open Device Manager (Win + X → Device Manager)");
  console.log("   - Look under 'Ports (COM & LPT)'");
  console.log("   - Update SERIAL_PORT in windows-server.js");
  console.log("3. Run: npm run ports (to list all available ports)");
  console.log("4. Make sure no other program is using the port (Arduino IDE, PlatformIO, etc.)");
  console.log("5. Try unplugging and replugging the sensor\n");
});

parser.on("data", (line) => {
  let rawLine = line.trim();
  
  // Extract raw field if it's JSON wrapped from ESP32
  try {
    const obj = JSON.parse(rawLine);
    if (obj && typeof obj === "object" && "raw" in obj) {
      rawLine = String(obj.raw);
    }
  } catch {}
  
  if (!rawLine) return;
  
  // Log for debugging
  console.log("📥 Raw:", rawLine);
  
  lastRaw = rawLine;
  const newPresence = derivePresence(rawLine);
  
  if (newPresence !== presence) {
    presence = newPresence;
    lastUpdate = new Date().toISOString();
    broadcast({ type: "state", presence, lastRaw, lastUpdate });
    console.log(`🚨 Presence changed: ${presence ? "OCCUPIED ✓" : "VACANT ○"}\n`);
  } else {
    // Broadcast raw updates without state change
    broadcast({ type: "raw", raw: rawLine, timestamp: new Date().toISOString() });
  }
});

// Optional heartbeat to keep connections alive
setInterval(() => {
  broadcast({ type: "heartbeat", ts: Date.now() });
}, 15000);

// Manual injection endpoint (for testing without hardware)
app.post("/api/inject", express.json(), (req, res) => {
    const rawLine = String(req.body.raw || "");
    if (!rawLine.trim()) return res.status(400).json({ error: "raw required" });
    
    lastRaw = rawLine.trim();
    const newPresence = derivePresence(lastRaw);
    if (newPresence !== presence) {
      presence = newPresence;
      lastUpdate = new Date().toISOString();
      broadcast({ type: "state", presence, lastRaw, lastUpdate });
    } else {
      broadcast({ type: "raw", raw: lastRaw, timestamp: new Date().toISOString() });
    }
    res.json({ ok: true, presence });
});

server.listen(PORT, () => {
  console.log(`\n🌐 Server running at http://localhost:${PORT}`);
  console.log(`📊 Open your browser to see the dashboard`);
  console.log(`🔌 WebSocket path: ${WS_PATH}\n`);
});