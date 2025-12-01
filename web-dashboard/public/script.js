const statusEl = document.getElementById("status");
const desks = [
  document.getElementById("desk1"), 
  document.getElementById("desk2"), 
  document.getElementById("desk3")
];
const logEl = document.getElementById("log");

// Apply presence state to desk visualization
function applyPresence(present, raw) {
  desks[0].classList.toggle("occupied", present);
  desks[0].classList.toggle("vacant", !present);
  statusEl.textContent = present ? "Presence detected" : "No presence";
  if (raw) appendLog("STATE raw=" + raw);
}

// Add log entry
function appendLog(line) {
  const div = document.createElement("div");
  div.textContent = new Date().toLocaleTimeString() + " " + line;
  logEl.prepend(div);
  
  // Keep log manageable (max 50 entries)
  while (logEl.children.length > 50) {
    logEl.removeChild(logEl.lastChild);
  }
}

// Connect to WebSocket server
function connectWS() {
  const proto = location.protocol === "https:" ? "wss" : "ws";
  const url = proto + "://" + location.host + "/ws";
  const ws = new WebSocket(url);

  ws.onopen = () => {
    statusEl.textContent = "Connected (waiting for data)";
    appendLog("WebSocket connected");
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
        } else if (msg.type === "heartbeat") {
          // Silently handle heartbeats
        }
      } catch (e) {
        appendLog("JSON parse error: " + e.message);
      }
    } else {
      // Non-JSON frame
      appendLog("NON-JSON " + txt);
    }
  };

  ws.onerror = (err) => {
    console.error("WebSocket error:", err);
    appendLog("WebSocket error");
  };

  ws.onclose = () => {
    statusEl.textContent = "Disconnected — retrying in 3s...";
    appendLog("WebSocket closed, reconnecting...");
    setTimeout(connectWS, 3000);
  };
}

// Start connection
connectWS();