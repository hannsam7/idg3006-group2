import express from "express";
import { WebSocketServer, WebSocket } from "ws";
import http from "http";
import path from "path";
import { fileURLToPath } from "url";

const PORT = 8085;
const WS_PATH = "/ws";

const __filename = fileURLToPath(import.meta.url);
const __dirname  = path.dirname(__filename);

const app = express();
app.use(express.static(path.join(__dirname, "../../web-dashboard/public")));

let presence = false;
let lastRaw = "";
let lastUpdate = null;

function derivePresence(raw) {
    const line = raw.trim();
    if (!line) return false;
    try {
      const obj = JSON.parse(line);
      for (const k of ["targets","targetCount","count"]) {
        if (typeof obj[k] === "number") return obj[k] > 0;
      }
      if (typeof obj.presence === "boolean") return obj.presence;
      if (typeof obj.occupied === "boolean") return obj.occupied;
    } catch {}
    const kv = line.match(/\b(targets?|count)\s*=\s*(\d+)/i);
    if (kv) return parseInt(kv[2],10) > 0;
    return /\b(person|human|occupied|presence|target)\b/i.test(line);
  }

app.get("/api/state", (_req, res) => {
  res.json({ presence, lastRaw, lastUpdate });
});

const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: WS_PATH });

function broadcast(obj) {
  const msg = JSON.stringify(obj);
  for (const c of wss.clients) if (c.readyState === WebSocket.OPEN) c.send(msg);
}

wss.on("connection", ws => {
    ws.send(JSON.stringify({ type: "state", presence, lastRaw, lastUpdate }));
    ws.on("message", data => {
      let rawLine = data.toString();
      try {
        const obj = JSON.parse(rawLine);
        if (obj && obj.raw !== undefined) rawLine = String(obj.raw);
      } catch {}
      rawLine = rawLine.trim();
      if (!rawLine) return;
      lastRaw = rawLine;
      const newPresence = derivePresence(rawLine);
      if (newPresence !== presence) {
        presence = newPresence;
        lastUpdate = new Date().toISOString();
        broadcast({ type: "state", presence, lastRaw, lastUpdate });
      } else {
        broadcast({ type: "raw", raw: lastRaw, ts: Date.now() });
      }
    });
  });

server.listen(PORT, () => {
  console.log(`HTTP+WS server listening on :${PORT} (path ${WS_PATH})`);
});