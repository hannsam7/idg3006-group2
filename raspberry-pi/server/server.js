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
    let rawLine = data.toString();
try {
  const obj = JSON.parse(rawLine);
  if (obj && typeof obj === "object" && "raw" in obj) rawLine = String(obj.raw);
} catch {}

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

app.post("/api/inject", express.json(), (req, res) => {
    const rawLine = String(req.body.raw || "");
    if (!rawLine.trim()) return res.status(400).json({ error: "raw required" });
    // Reuse presence logic
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
  console.log(`HTTP+WS server listening on :${PORT} (path ${WS_PATH})`);
});