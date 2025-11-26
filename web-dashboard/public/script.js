import path from "path";
import { fileURLToPath } from "url";
const __filename = fileURLToPath(import.meta.url);
const __dirname  = path.dirname(__filename);
app.use(express.static(path.join(__dirname, "../../web-dashboard/public")));

const statusEl = document.getElementById("status");
const desks = [document.getElementById("desk1"), document.getElementById("desk2"), document.getElementById("desk3")];
const logEl = document.getElementById("log");

// Assume one sensor covers desk1; adapt mapping logic for multiple sensors or zones.
function applyPresence(present, raw) {
desks[0].classList.toggle("occupied", present);
desks[0].classList.toggle("vacant", !present);
statusEl.textContent = present ? "Presence detected" : "No presence";
if (raw) appendLog("STATE raw=" + raw);
}

function appendLog(line) {
const div = document.createElement("div");
div.textContent = new Date().toLocaleTimeString() + " " + line;
logEl.prepend(div);
}

function connectWS() {
const proto = location.protocol === "https:" ? "wss" : "ws";
const url = proto + "://" + location.host + "/ws";
const ws = new WebSocket(url);

ws.onopen = () => {
    statusEl.textContent = "Connected (waiting for data)";
};

ws.onmessage = evt => {
    const txt = evt.data;
    // Only parse probable JSON
    if (typeof txt === "string" && /^[{\[]/.test(txt.trim())) {
    try {
        const msg = JSON.parse(txt);
        if (msg.type === "state") {
        applyPresence(msg.presence, msg.lastRaw);
        } else if (msg.type === "raw") {
        appendLog("RAW " + msg.raw);
        }
    } catch (e) {
        appendLog("JSON parse error " + e);
    }
    } else {
    // Non-JSON frame; log or ignore
    appendLog("NON-JSON " + txt);
    }
};

ws.onclose = () => {
    statusEl.textContent = "Disconnected — retrying...";
    setTimeout(connectWS, 3000);
};
}

connectWS();