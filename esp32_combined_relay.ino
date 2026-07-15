// ============================================================
//  ESP32 – 4 Channel Relay
//  ✅ SinricPro  – Alexa / Google Voice Control
//  ✅ Web UI     – Browser control on local IP
//  College Project | GPIO 12, 13, 14, 27
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

// ─── WiFi (Home Router) ──────────────────────────────────────
#define WIFI_SSID    "YOUR_4G_HOTSPOT_NAME"      // <-- change this
#define WIFI_PASS    "YOUR_HOTSPOT_PASSWORD"  // <-- change this

// ─── SinricPro Keys ──────────────────────────────────────────
#define APP_KEY     "YOUR_SINRIC_APP_KEY"
#define APP_SECRET  "YOUR_SINRIC_APP_SECRET"

// ─── SinricPro Device IDs ────────────────────────────────────
#define BEDROOM_ID  "YOUR_BEDROOM_DEVICE_ID"  // Relay 1 – GPIO 12
#define KITCHEN_ID  "YOUR_KITCHEN_DEVICE_ID"  // Relay 2 – GPIO 13
#define ROOM_ID     "YOUR_ROOM_DEVICE_ID"  // Relay 3 – GPIO 14
// Add a 4th device in SinricPro dashboard and paste ID here:
#define RELAY4_ID   "ADD-4TH-DEVICE-ID-HERE"    // Relay 4 – GPIO 27

// ─── Relay GPIO Pins ─────────────────────────────────────────
const int RELAY_PINS[4] = {12, 13, 14, 27};

// ─── Device → Pin mapping ────────────────────────────────────
struct DeviceRelay { const char* id; int pin; };
DeviceRelay deviceMap[4] = {
  { BEDROOM_ID, 12 },
  { KITCHEN_ID, 13 },
  { ROOM_ID,    14 },
  { RELAY4_ID,  27 },
};

// ─── Shared relay state (used by both Web UI & SinricPro) ────
bool relayState[4] = { false, false, false, false };

// ─── Web Server on port 80 ───────────────────────────────────
WebServer server(80);

// ─────────────────────────────────────────────────────────────
//  Core relay function – single source of truth
// ─────────────────────────────────────────────────────────────
void setRelay(int index, bool on) {
  relayState[index] = on;
  // Active-LOW module: LOW = ON, HIGH = OFF
  digitalWrite(RELAY_PINS[index], on ? LOW : HIGH);
  Serial.printf("[Relay %d] → %s (GPIO %d)\n", index + 1, on ? "ON" : "OFF", RELAY_PINS[index]);
}

// ─────────────────────────────────────────────────────────────
//  JSON state builder for Web UI responses
// ─────────────────────────────────────────────────────────────
String buildStateJSON() {
  String json = "{\"states\":[";
  for (int i = 0; i < 4; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < 3) json += ",";
  }
  json += "]}";
  return json;
}

// ─────────────────────────────────────────────────────────────
//  HTML Web Page
// ─────────────────────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>ESP32 Relay Control</title>
<link href="https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@400;700;900&display=swap" rel="stylesheet"/>
<style>
  :root {
    --bg:       #0a0e1a;
    --panel:    #0f1628;
    --border:   #1e3a5f;
    --accent:   #00d4ff;
    --accent2:  #ff6b35;
    --on:       #00ff88;
    --off:      #ff3366;
    --text:     #c8dff5;
    --dim:      #4a6080;
    --glow-on:  0 0 20px #00ff8866, 0 0 40px #00ff8833;
    --glow-off: 0 0 20px #ff336666, 0 0 40px #ff336633;
    --glow-acc: 0 0 15px #00d4ff55;
  }
  * { margin:0; padding:0; box-sizing:border-box; }
  body {
    background: var(--bg);
    font-family: 'Share Tech Mono', monospace;
    color: var(--text);
    min-height: 100vh;
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    padding: 20px; overflow-x: hidden;
  }
  body::before {
    content:''; position:fixed; inset:0;
    background-image:
      linear-gradient(rgba(0,212,255,0.03) 1px, transparent 1px),
      linear-gradient(90deg, rgba(0,212,255,0.03) 1px, transparent 1px);
    background-size:40px 40px; pointer-events:none; z-index:0;
  }
  body::after {
    content:''; position:fixed; top:-50%; left:-50%;
    width:200%; height:200%;
    background:radial-gradient(ellipse at center, rgba(0,50,100,0.15) 0%, transparent 60%);
    pointer-events:none; z-index:0;
    animation:pulse-bg 6s ease-in-out infinite;
  }
  @keyframes pulse-bg {
    0%,100%{opacity:.5;transform:scale(1);}
    50%{opacity:1;transform:scale(1.05);}
  }
  .container { position:relative; z-index:1; width:100%; max-width:720px; }
  header { text-align:center; margin-bottom:40px; }
  .logo {
    font-family:'Orbitron',sans-serif;
    font-size:clamp(22px,5vw,38px); font-weight:900;
    letter-spacing:4px; color:var(--accent);
    text-shadow:var(--glow-acc); text-transform:uppercase;
  }
  .logo span { color:var(--accent2); }
  .subtitle { font-size:11px; letter-spacing:6px; color:var(--dim); margin-top:6px; text-transform:uppercase; }
  .status-bar {
    display:flex; align-items:center; justify-content:center;
    gap:8px; margin-top:14px; font-size:11px; color:var(--dim); letter-spacing:2px;
  }
  .status-dot {
    width:8px; height:8px; border-radius:50%;
    background:var(--on); box-shadow:var(--glow-on);
    animation:blink 2s ease-in-out infinite;
  }
  @keyframes blink { 0%,100%{opacity:1;} 50%{opacity:.3;} }
  .voice-badge {
    display:inline-flex; align-items:center; gap:6px;
    background:rgba(0,212,255,0.08); border:1px solid var(--border);
    border-radius:20px; padding:5px 14px; margin-top:10px;
    font-size:10px; letter-spacing:2px; color:var(--accent);
  }
  .relay-grid { display:grid; grid-template-columns:repeat(2,1fr); gap:20px; }
  @media(max-width:480px){ .relay-grid{grid-template-columns:1fr;} }
  .relay-card {
    background:var(--panel); border:1px solid var(--border);
    border-radius:12px; padding:28px 24px; position:relative;
    overflow:hidden; transition:border-color .3s,box-shadow .3s; cursor:pointer;
  }
  .relay-card::before {
    content:''; position:absolute; top:0; left:0; right:0; height:2px;
    background:linear-gradient(90deg,transparent,var(--accent),transparent);
    opacity:0; transition:opacity .3s;
  }
  .relay-card:hover::before { opacity:1; }
  .relay-card.active {
    border-color:var(--on);
    box-shadow:0 0 0 1px #00ff8822, 0 8px 32px #00ff8822;
  }
  .relay-card.active::before {
    background:linear-gradient(90deg,transparent,var(--on),transparent); opacity:1;
  }
  .relay-card::after {
    content:attr(data-index); position:absolute; top:12px; right:16px;
    font-family:'Orbitron',sans-serif; font-size:28px; font-weight:900;
    color:rgba(0,212,255,0.06); line-height:1; pointer-events:none;
  }
  .relay-header { display:flex; align-items:center; gap:10px; margin-bottom:20px; }
  .relay-icon {
    width:36px; height:36px; border-radius:8px;
    background:rgba(0,212,255,0.08); border:1px solid var(--border);
    display:flex; align-items:center; justify-content:center; font-size:16px;
  }
  .relay-label { font-family:'Orbitron',sans-serif; font-size:12px; font-weight:700; letter-spacing:2px; color:var(--accent); }
  .relay-name  { font-size:10px; color:var(--accent2); letter-spacing:1px; margin-top:1px; }
  .relay-pin   { font-size:10px; color:var(--dim); letter-spacing:1px; margin-top:1px; }
  .toggle-wrap { display:flex; align-items:center; justify-content:space-between; margin-top:10px; }
  .state-label { font-family:'Orbitron',sans-serif; font-size:14px; font-weight:700; letter-spacing:3px; transition:color .3s; }
  .state-label.on  { color:var(--on);  text-shadow:var(--glow-on); }
  .state-label.off { color:var(--off); }
  .toggle { position:relative; width:64px; height:32px; flex-shrink:0; }
  .toggle input { display:none; }
  .toggle-track {
    position:absolute; inset:0; border-radius:16px;
    background:#1a2535; border:1px solid var(--border);
    transition:background .3s,border-color .3s,box-shadow .3s; cursor:pointer;
  }
  .toggle-thumb {
    position:absolute; top:4px; left:4px; width:22px; height:22px;
    border-radius:50%; background:var(--dim);
    transition:transform .3s cubic-bezier(.34,1.56,.64,1),background .3s,box-shadow .3s;
  }
  input:checked ~ .toggle-track { background:rgba(0,255,136,.12); border-color:var(--on); box-shadow:var(--glow-on); }
  input:checked ~ .toggle-track .toggle-thumb { transform:translateX(32px); background:var(--on); box-shadow:0 0 12px var(--on); }
  .wave-bars { display:flex; align-items:flex-end; gap:3px; height:20px; margin-top:18px; }
  .wave-bar  { flex:1; border-radius:2px; background:var(--border); transition:height .3s,background .3s; }
  .relay-card.active .wave-bar { background:var(--on); animation:wave-anim 1.2s ease-in-out infinite alternate; }
  .wave-bar:nth-child(1) {height:30%;} .wave-bar:nth-child(2) {height:70%;}
  .wave-bar:nth-child(3) {height:50%;} .wave-bar:nth-child(4) {height:90%;}
  .wave-bar:nth-child(5) {height:40%;} .wave-bar:nth-child(6) {height:60%;}
  .wave-bar:nth-child(7) {height:30%;} .wave-bar:nth-child(8) {height:80%;}
  .wave-bar:nth-child(9) {height:55%;} .wave-bar:nth-child(10){height:35%;}
  @keyframes wave-anim { from{opacity:.5;} to{opacity:1;} }
  .master-panel {
    margin-top:28px; background:var(--panel); border:1px solid var(--border);
    border-radius:12px; padding:20px 24px;
    display:flex; align-items:center; justify-content:space-between; gap:12px; flex-wrap:wrap;
  }
  .master-title { font-family:'Orbitron',sans-serif; font-size:10px; letter-spacing:4px; color:var(--dim); text-transform:uppercase; }
  .master-btns { display:flex; gap:12px; flex-wrap:wrap; }
  .btn {
    font-family:'Orbitron',sans-serif; font-size:10px; letter-spacing:3px; text-transform:uppercase;
    padding:10px 20px; border-radius:6px; border:1px solid; cursor:pointer;
    transition:all .2s; position:relative; overflow:hidden;
  }
  .btn::after { content:''; position:absolute; inset:0; background:white; opacity:0; transition:opacity .15s; }
  .btn:active::after { opacity:.08; }
  .btn-on  { background:rgba(0,255,136,.08); border-color:var(--on);  color:var(--on);  }
  .btn-on:hover  { background:rgba(0,255,136,.18); box-shadow:var(--glow-on);  }
  .btn-off { background:rgba(255,51,102,.08); border-color:var(--off); color:var(--off); }
  .btn-off:hover { background:rgba(255,51,102,.18); box-shadow:var(--glow-off); }
  footer { text-align:center; margin-top:28px; font-size:10px; color:var(--dim); letter-spacing:3px; }
  #toast {
    position:fixed; bottom:30px; left:50%;
    transform:translateX(-50%) translateY(80px);
    background:var(--panel); border:1px solid var(--accent); color:var(--accent);
    padding:10px 24px; border-radius:30px; font-size:11px; letter-spacing:3px;
    transition:transform .4s cubic-bezier(.34,1.56,.64,1);
    z-index:100; white-space:nowrap; box-shadow:var(--glow-acc);
  }
  #toast.show { transform:translateX(-50%) translateY(0); }
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="logo">ESP<span>32</span> RELAY</div>
    <div class="subtitle">4-Channel Controller &nbsp;///&nbsp; Smart Home</div>
    <div class="status-bar">
      <div class="status-dot"></div>
      <span id="ipLabel">CONNECTED</span>
    </div>
    <div style="text-align:center;margin-top:8px;">
      <span class="voice-badge">🎙 ALEXA &amp; GOOGLE ENABLED</span>
    </div>
  </header>

  <div class="relay-grid" id="relayGrid"></div>

  <div class="master-panel">
    <div class="master-title">Master Control</div>
    <div class="master-btns">
      <button class="btn btn-on"  onclick="setAll(1)">&#9646; All On</button>
      <button class="btn btn-off" onclick="setAll(0)">&#9646; All Off</button>
    </div>
  </div>

  <footer>ESP32 &nbsp;///&nbsp; SINRICPRO + WEB UI &nbsp;///&nbsp; 4CH</footer>
</div>
<div id="toast"></div>

<script>
const relayConfig = [
  { label:'RELAY 1', name:'BEDROOM',       pin:'GPIO 12', icon:'🛏' },
  { label:'RELAY 2', name:'KITCHEN LIGHT', pin:'GPIO 13', icon:'💡' },
  { label:'RELAY 3', name:'ROOM LIGHT',    pin:'GPIO 14', icon:'🔆' },
  { label:'RELAY 4', name:'RELAY 4',       pin:'GPIO 27', icon:'⚡' },
];
let states = [false,false,false,false];

function buildGrid() {
  document.getElementById('relayGrid').innerHTML = relayConfig.map((r,i) => `
    <div class="relay-card ${states[i]?'active':'inactive'}" data-index="${i+1}" onclick="toggleRelay(${i})">
      <div class="relay-header">
        <div class="relay-icon">${r.icon}</div>
        <div>
          <div class="relay-label">${r.label}</div>
          <div class="relay-name">${r.name}</div>
          <div class="relay-pin">${r.pin}</div>
        </div>
      </div>
      <div class="toggle-wrap">
        <div class="state-label ${states[i]?'on':'off'}">${states[i]?'ON':'OFF'}</div>
        <label class="toggle">
          <input type="checkbox" ${states[i]?'checked':''} onchange="toggleRelay(${i})" onclick="event.stopPropagation()"/>
          <div class="toggle-track"><div class="toggle-thumb"></div></div>
        </label>
      </div>
      <div class="wave-bars">
        ${Array.from({length:10},(_,j)=>`<div class="wave-bar" style="animation-delay:${j*0.08}s"></div>`).join('')}
      </div>
    </div>`).join('');
}

function toggleRelay(i) { sendCommand(i+1, !states[i]?'on':'off'); }
function setAll(s)       { sendCommand('all', s?'on':'off'); }

function sendCommand(relay, state) {
  fetch(`/relay?ch=${relay}&state=${state}`)
    .then(r=>r.json())
    .then(d=>{ states=d.states; buildGrid();
      showToast(relay==='all'?`ALL ${state.toUpperCase()}`:`RELAY ${relay} ${state.toUpperCase()}`);
    }).catch(()=>showToast('CONNECTION ERROR'));
}

function fetchStatus() {
  fetch('/status').then(r=>r.json())
    .then(d=>{ states=d.states; buildGrid(); }).catch(()=>{});
}

function showToast(msg) {
  const t=document.getElementById('toast');
  t.textContent=msg; t.classList.add('show');
  setTimeout(()=>t.classList.remove('show'),2000);
}

buildGrid();
fetchStatus();
setInterval(fetchStatus, 4000);  // sync every 4s (picks up voice changes too)
</script>
</body>
</html>
)rawliteral";

// ─────────────────────────────────────────────────────────────
//  SinricPro callback – fired when Alexa/Google sends command
// ─────────────────────────────────────────────────────────────
bool onPowerState(const String& deviceId, bool& state) {
  for (int i = 0; i < 4; i++) {
    if (deviceId == deviceMap[i].id) {
      setRelay(i, state);
      Serial.printf("[SinricPro] %s → %s\n", deviceId.c_str(), state ? "ON" : "OFF");
      return true;
    }
  }
  return false;
}

// ─────────────────────────────────────────────────────────────
//  Web Server route handlers
// ─────────────────────────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buildStateJSON());
}

void handleRelay() {
  if (!server.hasArg("ch") || !server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"Missing params\"}");
    return;
  }
  String ch    = server.arg("ch");
  String state = server.arg("state");
  bool   on    = (state == "on");

  if (ch == "all") {
    for (int i = 0; i < 4; i++) setRelay(i, on);
  } else {
    int idx = ch.toInt() - 1;
    if (idx >= 0 && idx < 4) setRelay(idx, on);
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buildStateJSON());
}

// ─────────────────────────────────────────────────────────────
//  WiFi connect
// ─────────────────────────────────────────────────────────────
void setupWiFi() {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300); Serial.print(".");
  }
  Serial.printf("\n[WiFi] Connected!\n");
  Serial.printf("[WiFi] Open browser → http://%s\n", WiFi.localIP().toString().c_str());
}

// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 Smart Relay – Voice + Web UI ===");

  // Init relay pins – all OFF at boot
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH);  // Active-LOW: HIGH = OFF
  }

  // Connect to home WiFi
  setupWiFi();

  // Web server routes
  server.on("/",       HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/relay",  HTTP_GET, handleRelay);
  server.begin();
  Serial.println("[WebServer] Started ✓");

  // SinricPro
  for (int i = 0; i < 4; i++) {
    SinricProSwitch& sw = SinricPro[deviceMap[i].id];
    sw.onPowerState(onPowerState);
  }
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
  Serial.println("[SinricPro] Started ✓");
  Serial.println("==========================================");
  Serial.printf("  Web UI  → http://%s\n", WiFi.localIP().toString().c_str());
  Serial.println("  Voice   → \"Alexa, turn on bedroom\"");
  Serial.println("==========================================");
}

// ─────────────────────────────────────────────────────────────
//  LOOP – both run together
// ─────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();   // handles web browser requests
  SinricPro.handle();      // handles Alexa / Google commands
}
