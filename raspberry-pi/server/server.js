import express from "express";
import { WebSocketServer, WebSocket } from "ws";
import http from "http";
import path from "path";
import { fileURLToPath } from "url";

const PORT = 8085;
const WS_PATH = "/ws";

// In-memory state
let presence = false;
let lastRaw = "";
let lastUpdate = null;

// Derive presence from a raw line (adjust to your sensor output)
function derivePresence(raw) {
  const line = raw.trim().toLowerCase();
  if (!line) return false;
  const tokens = ["person", "human", "target", "presence", "occupied"];
  return tokens.some(t => line.includes(t));
}

const app = express();

// Use absolute path for static files (public next to server.js)
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
app.use(express.static(path.join(__dirname, "public")));

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
  console.log("WS client connected:", req.socket.remoteAddress);
  ws.send(JSON.stringify({ type: "state", presence, lastRaw, lastUpdate }));

  ws.on("message", data => {
    // Accept either JSON {"sensorId","raw"} or plain text
    let rawLine = data.toString();
    try {
      const obj = JSON.parse(rawLine);
      if (obj && typeof obj === "object" && "raw" in obj) {
        rawLine = String(obj.raw);
      }
    } catch {
      // not JSON; keep as-is
    }

    const raw = rawLine.trim();
    if (!raw) return;

    lastRaw = raw;
    const newPresence = derivePresence(raw);
    if (newPresence !== presence) {
      presence = newPresence;
      lastUpdate = new Date().toISOString();
      broadcast({ type: "state", presence, lastRaw, lastUpdate });
      console.log("Presence changed:", presence, "raw:", raw);
    } else {
      // Optional: comment out if too chatty
      broadcast({ type: "raw", raw, timestamp: new Date().toISOString() });
    }
  });

  ws.on("close", () => console.log("WS client disconnected"));
});

// Optional heartbeat to keep connections alive
setInterval(() => {
  broadcast({ type: "heartbeat", ts: Date.now() });
}, 15000);

server.listen(PORT, () => {
  console.log(`HTTP+WS server listening on :${PORT} (path ${WS_PATH})`);
});