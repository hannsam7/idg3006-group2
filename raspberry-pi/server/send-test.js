import { WebSocket } from "ws";

const url = "ws://localhost:3000/ws";
const frames = [
  '{"sensorId":"sensor1","raw":"targets=0"}',
  '{"sensorId":"sensor1","raw":"targets=2"}',
  '{"sensorId":"sensor1","raw":"targets=1"}',
  '{"sensorId":"sensor1","raw":"targets=0"}'
];

const ws = new WebSocket(url);
ws.on("open", () => {
  console.log("Connected, sending test frames...");
  let i = 0;
  const h = setInterval(() => {
    if (i >= frames.length) { clearInterval(h); ws.close(); return; }
    ws.send(frames[i]);
    console.log("Sent:", frames[i]);
    i++;
  }, 1500);
});