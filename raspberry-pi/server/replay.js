import fs from "fs";
import { WebSocket } from "ws";

const url = "ws://localhost:8085/ws";
const lines = fs.readFileSync("sample-data.txt","utf8").split(/\r?\n/).filter(l=>l.trim());

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