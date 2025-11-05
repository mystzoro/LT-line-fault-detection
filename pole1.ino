#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "EmonLib.h"
#include <ArduinoJson.h>

// -------------------- CONFIG --------------------
const char* ssid = "Airtel_Bachelor";
const char* password = "Dypiu_fckoff01";

#define ADC_PIN 34
#define RELAY_PIN 23

// UART to Pole2 (UART1)
#define POLE2_RX_PIN 16 
#define POLE2_TX_PIN 17 
HardwareSerial pole2Serial(1); // UART1 for Pole2

// EmonLib
EnergyMonitor emon1;

// thresholds, timing
const float CURRENT_THRESHOLD = 1.0f; // A
const float HYSTERESIS = 0.08f;       // avoids chattering
const int SAMPLE_COUNT = 1480;
const unsigned long MEASURE_INTERVAL_MS_DEFAULT = 1000; 

// -------------------- STATE --------------------
volatile float lastIrmsPole1 = 0.0f;
volatile float lastIrmsPole2 = 0.0f;
volatile bool relayState = false;

AsyncWebServer server(80);

// timing
unsigned long lastMeasureMs = 0;
unsigned long measureIntervalMs = MEASURE_INTERVAL_MS_DEFAULT;
unsigned long lastWifiCheck = 0;

// Pole2 UART buffer
String pole2Buf;

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // EmonLib calibration value for CT sensor
  emon1.current(ADC_PIN, 30.0);

  // UART for Pole2
  pole2Serial.begin(9600, SERIAL_8N1, POLE2_RX_PIN, POLE2_TX_PIN);

  // Wi-Fi
  WiFi.setHostname("pole-monitor-esp32");
  Serial.printf("Connecting to WiFi SSID: %s\n", ssid);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connect timed out — continuing (will retry in loop).");
  }

  // ---------- Web UI: root (dashboard) ----------
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Pole Monitor — Dashboard</title>
  <style>
    :root{
      --bg1:#0f1724;--bg2:#071020;--muted:#9fb0c8;--accent1:#06b6d4;--accent2:#7c3aed;--accent3:#ff7a59;--success:#22c55e;--danger:#ef4444;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Inter, Arial;
    }
    *{box-sizing:border-box}
    html,body{height:100%;margin:0;background:linear-gradient(180deg,var(--bg1),var(--bg2));color:#e7f0fb}
    .wrap{max-width:960px;margin:20px auto;padding:20px}
    header{display:flex;justify-content:space-between;align-items:center;gap:12px}
    .brand{display:flex;align-items:center;gap:10px}
    .logo{width:44px;height:44px;border-radius:10px;background:linear-gradient(135deg,var(--accent1),var(--accent2));display:flex;align-items:center;justify-content:center;font-weight:800}
    h1{margin:0;font-size:18px}
    .sub{color:var(--muted);font-size:12px}
    .grid{display:grid;grid-template-columns:repeat(12,1fr);gap:16px;margin-top:16px}
    .box{grid-column:span 6;background:linear-gradient(180deg,rgba(255,255,255,.03),rgba(255,255,255,.015));border:1px solid rgba(255,255,255,.06);border-radius:12px;padding:16px;box-shadow:0 8px 30px rgba(3,8,20,.6);transition:transform .2s ease,border-color .2s ease}
    .box:hover{transform:translateY(-2px);border-color:rgba(255,255,255,.12)}
    .box.full{grid-column:span 12}
    .box h3{margin:0;color:var(--muted);font-size:13px}
    .big{font-size:40px;font-weight:800;margin-top:6px}
    .strip{height:6px;border-radius:6px;margin-top:12px;background:linear-gradient(90deg,var(--accent1),var(--accent2));box-shadow:0 6px 18px rgba(124,58,237,.12), inset 0 -4px 12px rgba(0,0,0,.25);width:20%;transition:width .6s cubic-bezier(.2,.8,.2,1)}
    .relay-pill{display:inline-block;padding:8px 14px;border-radius:999px;font-weight:700;border:1px solid transparent}
    .on{background:linear-gradient(90deg, rgba(34,197,94,.12), rgba(34,197,94,.06));color:var(--success);border-color:rgba(34,197,94,.18)}
    .off{background:linear-gradient(90deg, rgba(239,68,68,.06), rgba(239,68,68,.03));color:var(--danger);border-color:rgba(239,68,68,.12)}
    .small{color:var(--muted);font-size:12px}
    .meta{display:flex;gap:10px;align-items:center;margin-top:10px;flex-wrap:wrap}
    .btn{padding:10px 14px;border-radius:10px;border:0;background:linear-gradient(90deg,var(--accent1),var(--accent2));color:white;font-weight:700;cursor:pointer;box-shadow:0 8px 24px rgba(6,182,212,.25)}
    .chart{width:100%;height:220px;display:block}
    .shimmer{position:relative;overflow:hidden}
    .shimmer::after{content:"";position:absolute;inset:0;transform:translateX(-100%);background:linear-gradient(90deg,transparent,rgba(255,255,255,.06),transparent);animation:sh 1.2s infinite}
    @keyframes sh{100%{transform:translateX(100%)}}
    @media(max-width:900px){.box{grid-column:span 12}.big{font-size:34px}}
  </style>
</head>
<body>
  <div class="wrap">
    <header>
      <div class="brand">
        <div class="logo">PM</div>
        <div>
          <h1>Pole Monitor</h1>
          <div class="sub">Live currents • Relay status</div>
        </div>
      </div>
      <button id="refresh" class="btn">Refresh</button>
    </header>

    <div class="grid">
      <div class="box">
        <div>
          <h3>Pole 1</h3>
          <div class="big shimmer" id="p1">0.00 A</div>
          <div class="small">RMS Current — local CT</div>
        </div>
        <div>
          <div class="meta"><div class="small">Updated: <span id="t1">--</span></div></div>
          <div class="strip" id="s1"></div>
        </div>
      </div>

      <div class="box">
        <div>
          <h3>Pole 2</h3>
          <div class="big shimmer" id="p2">0.00 A</div>
          <div class="small">RMS Current — UART</div>
        </div>
        <div>
          <div class="meta"><div class="small">Updated: <span id="t2">--</span></div></div>
          <div class="strip" id="s2" style="background:linear-gradient(90deg,var(--accent3),var(--accent2))"></div>
        </div>
      </div>

      <div class="box">
        <div>
          <h3>GPS</h3>
          <div class="big" id="gps">--</div>
          <div class="small">Hardcoded device location</div>
        </div>
        <div class="meta" style="justify-content:space-between">
          <div class="small">Place: DY Patil International University</div>
          <a id="mapsLink" class="btn" href="#" target="_blank" rel="noopener">Open in Maps</a>
        </div>
      </div>

      <div class="box full">
        <div style="display:flex;justify-content:space-between;gap:12px;align-items:center;flex-wrap:wrap">
          <div>
            <h3>Relay</h3>
            <div class="small">Hardware relay controlling load</div>
            <div style="height:10px"></div>
            <div id="relay" class="relay-pill off">OFF</div>
          </div>
          <div class="small">Last poll: <span id="lp">--</span></div>
        </div>
      </div>

      <div class="box full">
        <div>
          <h3>Live Graph</h3>
          <div class="small">Recent currents (last 2 minutes)</div>
        </div>
        <div style="margin-top:10px">
          <canvas id="chart" class="chart"></canvas>
        </div>
      </div>
    </div>

    <div class="small" style="margin-top:12px;text-align:center;color:var(--muted)">Data source: /data</div>
  </div>

  <script>
    let first = true; const THRESH=1.0;
    // DY Patil International University, Akurdi, Pune
    const LAT = 18.654358, LNG = 73.772883;
    const MAX_POINTS = 120; // 120s at 1Hz
    const hist1 = []; const hist2 = [];
    function drawChart(){
      const canvas = document.getElementById('chart');
      if(!canvas) return;
      const dpr = window.devicePixelRatio || 1;
      const rect = canvas.getBoundingClientRect();
      const W = Math.max(300, rect.width|0), H = Math.max(120, rect.height|0);
      if(canvas.width !== W*dpr || canvas.height !== H*dpr){ canvas.width = W*dpr; canvas.height = H*dpr; }
      const ctx = canvas.getContext('2d');
      ctx.setTransform(dpr,0,0,dpr,0,0);
      ctx.clearRect(0,0,W,H);
      // theme colors
      const styles = getComputedStyle(document.documentElement);
      const gridC = 'rgba(255,255,255,0.08)';
      const txtC = styles.getPropertyValue('--muted') || '#9fb0c8';
      const c1 = styles.getPropertyValue('--accent1') || '#06b6d4';
      const c2 = styles.getPropertyValue('--accent3') || '#ff7a59';
      // padding
      const pad = {l:36, r:10, t:10, b:20};
      const plotW = W - pad.l - pad.r; const plotH = H - pad.t - pad.b;
      // scale
      let maxVal = Math.max(THRESH, 1, ...hist1, ...hist2);
      // round up to nice step
      const steps = [0.5,1,2,5,10,20];
      let step = steps[0];
      for(let s of steps){ if(maxVal/ s <= 6){ step = s; break; } }
      maxVal = Math.ceil(maxVal/step)*step;
      // grid
      ctx.strokeStyle = gridC; ctx.lineWidth = 1; ctx.beginPath();
      const ySteps = Math.max(2, Math.min(6, Math.round(maxVal/step)));
      for(let i=0;i<=ySteps;i++){
        const y = pad.t + plotH - (i/ySteps)*plotH; ctx.moveTo(pad.l,y); ctx.lineTo(W-pad.r,y);
      }
      ctx.stroke();
      // labels
      ctx.fillStyle = txtC; ctx.font = '12px system-ui, -apple-system, Segoe UI, Roboto, Arial'; ctx.textAlign='right'; ctx.textBaseline='middle';
      for(let i=0;i<=ySteps;i++){
        const val = (i/ySteps)*maxVal; const y = pad.t + plotH - (i/ySteps)*plotH; ctx.fillText(val.toFixed(1)+' A', pad.l-6, y);
      }
      // series draw helper
      function draw(series, color){
        if(series.length<2) return;
        ctx.strokeStyle = color.trim(); ctx.lineWidth = 2; ctx.beginPath();
        const n = series.length; const dx = plotW/Math.max(1, MAX_POINTS-1);
        for(let i=0;i<n;i++){
          const x = pad.l + i*dx;
          const v = Math.max(0, Math.min(maxVal, series[i] || 0));
          const y = pad.t + plotH - (v/maxVal)*plotH;
          if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
        }
        ctx.stroke();
      }
      draw(hist1, c1 || '#06b6d4');
      draw(hist2, c2 || '#ff7a59');
    }
    async function updateData(){
      try{
        const r = await fetch('/data');
        const d = await r.json();
        const p1 = Number(d.pole1||0); const p2 = Number(d.pole2||0);
        document.getElementById('p1').textContent = p1.toFixed(2)+' A';
        document.getElementById('p2').textContent = p2.toFixed(2)+' A';
        document.getElementById('t1').textContent = new Date().toLocaleTimeString();
        document.getElementById('t2').textContent = new Date().toLocaleTimeString();
        document.getElementById('s1').style.width = Math.max(20, Math.min(150, (p1/THRESH)*100)) + '%';
        document.getElementById('s2').style.width = Math.max(20, Math.min(150, (p2/THRESH)*100)) + '%';
        const pill = document.getElementById('relay');
        if(d.relay){ pill.className='relay-pill on'; pill.textContent='ON'; } else { pill.className='relay-pill off'; pill.textContent='OFF'; }
        document.getElementById('lp').textContent = new Date().toLocaleTimeString();
        // update history and redraw
        hist1.push(p1); hist2.push(p2);
        if(hist1.length>MAX_POINTS) hist1.shift();
        if(hist2.length>MAX_POINTS) hist2.shift();
        drawChart();
        if(first){ document.querySelectorAll('.shimmer').forEach(e=>e.classList.remove('shimmer')); first=false; }
      }catch(e){ console.error(e); }
    }
    document.getElementById('refresh').addEventListener('click', updateData);
    setInterval(updateData,1000); updateData();

    // Initialize GPS UI (static)
    (function(){
      const gpsEl = document.getElementById('gps');
      const linkEl = document.getElementById('mapsLink');
      if(gpsEl){ gpsEl.textContent = LAT.toFixed(6)+', '+LNG.toFixed(6); }
      if(linkEl){ linkEl.href = 'https://www.google.com/maps?q='+LAT+','+LNG; }
    })();
  </script>
</body>
</html>
)rawliteral";
    request->send(200,"text/html",html);
  });

  // ---------- /data endpoint (JSON) ----------
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<128> doc;
    doc["pole1"] = lastIrmsPole1;
    doc["pole2"] = lastIrmsPole2;
    doc["relay"] = relayState;
    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.begin();
}

// -------------------- Pole2 UART parsing --------------------
void handlePole2Serial() {
  while (pole2Serial.available()) {
    char c = (char)pole2Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = pole2Buf;
      pole2Buf = "";
      line.trim();
      if (line.length() > 0) {
        bool valid = true;
        int dots = 0;
        for (size_t i = 0; i < line.length(); ++i) {
          char ch = line[i];
          if (!(isDigit(ch) || ch=='-' || ch=='+' || ch=='.')) { valid = false; break; }
          if (ch == '.') dots++;
          if (dots > 1) { valid = false; break; }
        }
        if (valid) {
          float f = line.toFloat();
          if (f >= 0.0f && f < 200.0f) {
            lastIrmsPole2 = f;
          }
        }
      }
    } else {
      pole2Buf += c;
      if (pole2Buf.length() > 64) pole2Buf = ""; // protect
    }
  }
}

// -------------------- LOOP --------------------
void loop() {
  unsigned long now = millis();

  if (now - lastMeasureMs >= measureIntervalMs) {
    lastMeasureMs = now;

    float measured = emon1.calcIrms(SAMPLE_COUNT);
    if (!(measured > 0.005f && measured < 200.0f)) measured = 0.0f;

    // ✅ Force Pole1 = 0 if below 1A
    if (measured < 1.0f) {
      lastIrmsPole1 = 0.0f;
    } else {
      lastIrmsPole1 = measured;
    }

    if (!relayState && lastIrmsPole1 >= (CURRENT_THRESHOLD + HYSTERESIS)) {
      digitalWrite(RELAY_PIN, HIGH);
      relayState = true;
    } else if (relayState && lastIrmsPole1 <= (CURRENT_THRESHOLD - HYSTERESIS)) {
      digitalWrite(RELAY_PIN, LOW);
      relayState = false;
    }

    // ✅ Clean status print
    Serial.printf("[STATUS] Pole1: %.3f A | Pole2: %.3f A | Relay: %s\n",
                  lastIrmsPole1,
                  lastIrmsPole2,
                  relayState ? "ON" : "OFF");
  }

  handlePole2Serial();

  if (WiFi.status() != WL_CONNECTED && (now - lastWifiCheck > 5000)) {
    lastWifiCheck = now;
    Serial.println("WiFi disconnected — attempting reconnect...");
    WiFi.reconnect();
  }

  yield();
}