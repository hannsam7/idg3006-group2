import fs from "fs";
import { WebSocket } from "ws";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname  = path.dirname(__filename);

const url = "ws://localhost:3000/ws";
const samplePath = path.join(__dirname, "sample-data.txt");

if (!fs.existsSync(samplePath)) {
  fs.writeFileSync(samplePath, `targets=0
targets=2
{"targets":3,"distance":1.5}
targets=1
targets=0
`, "utf8");
  console.log("Created default sample-data.txt");
}

const lines = fs.readFileSync(samplePath, "utf8")
  .split(/\r?\n/)
  .map(l => l.trim())
  .filter(l => l.length);

if (!lines.length) {
  console.error("No lines to replay"); process.exit(1);
}

const ws = new WebSocket(url);
ws.on("open", () => {
  console.log("Replaying", lines.length, "lines");
  let i = 0;
  const tick = () => {
    if (i >= lines.length) { console.log("Done"); ws.close(); return; }
    const json = JSON.stringify({ sensorId: "sensor1", raw: lines[i] });
    ws.send(json);
    console.log("Sent:", json);
    i++;
    setTimeout(tick, 1000);
  };
  tick();
});
ws.on("error", err => console.error("WS error:", err.message));