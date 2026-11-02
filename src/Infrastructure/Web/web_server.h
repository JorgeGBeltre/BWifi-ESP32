#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SPIFFS.h"
#include "Infrastructure/Config/settings_menu.h"
#include "Infrastructure/Arcade/games_menu.h"

extern AsyncWebServer asyncServer;

inline void startWebServer() {
  if (!SPIFFS.begin(true))
    Serial.println("Error SPIFFS");

  uint64_t chipID = ESP.getEfuseMac();
  String apSSID = "BWESP-" + String((uint16_t)(chipID >> 32), DEC) + String((uint32_t)chipID, DEC);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str(), "12345678");

  asyncServer.reset();

  asyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>BWifiKill v3.5 Panel</title>
<style>
  body { background:#0d1117; color:#c9d1d9; font-family:'Courier New',monospace; margin:0; padding:16px; }
  .container { max-width:740px; margin:auto; border:1px solid #30363d; padding:18px; border-radius:8px; background:#161b22; box-shadow:0 4px 20px rgba(0,0,0,0.5); }
  h1 { color:#58a6ff; text-align:center; font-size:20px; border-bottom:1px solid #30363d; padding-bottom:10px; margin-top:0; }
  .stats { display:grid; grid-template-columns:repeat(2,1fr); gap:8px; margin-bottom:14px; font-size:12px; }
  .card { background:#0d1117; border:1px solid #30363d; border-radius:6px; padding:8px; }
  .tabs { display:flex; gap:6px; margin-bottom:10px; flex-wrap:wrap; }
  .tab { flex:1; padding:8px; background:#21262d; color:#c9d1d9; border:1px solid #30363d; border-radius:5px; cursor:pointer; text-align:center; font-weight:bold; font-size:11px; }
  .tab.active { background:#1f6feb; color:#fff; border-color:#58a6ff; }
  .log-box { background:#010409; padding:12px; border-radius:6px; border:1px solid #21262d; white-space:pre-wrap; height:240px; overflow-y:auto; color:#39d353; font-size:12px; }
  textarea { width:100%; height:180px; background:#010409; color:#58a6ff; border:1px solid #30363d; border-radius:6px; padding:8px; font-family:monospace; box-sizing:border-box; display:none; }
  .btns { display:grid; grid-template-columns:repeat(3,1fr); gap:8px; margin-top:12px; }
  .btn { padding:10px; border:none; border-radius:6px; cursor:pointer; font-weight:bold; text-align:center; text-decoration:none; font-size:11px; color:#fff; }
  .btn-dl { background:#238636; }
  .btn-pcap { background:#8957e5; }
  .btn-del { background:#da3633; }
</style></head><body>
<div class='container'>
  <h1>BWIFIKILL v3.5 COMMAND CENTER</h1>
  <div class='stats'>
    <div class='card'>Heap Libre: <b id='st-heap'>...</b></div>
    <div class='card'>SPIFFS: <b id='st-fs'>...</b></div>
    <div class='card'>Uptime: <b id='st-up'>...</b></div>
    <div class='card'>Perfil: <b id='st-prof'>...</b></div>
  </div>
  <div class='tabs'>
    <div class='tab active' id='t-logins' onclick="setTab('logins')">LOGINS</div>
    <div class='tab' id='t-wardrive' onclick="setTab('wardrive')">KISMET CSV</div>
    <div class='tab' id='t-trackers' onclick="setTab('trackers')">TRACKERS</div>
    <div class='tab' id='t-ducky' onclick="setTab('ducky')">DUCKY SCRIPT</div>
    <div class='tab' id='t-scores' onclick="setTab('scores')">LEADERBOARD</div>
  </div>
  <div id='log-content' class='log-box'>Cargando...</div>
  <textarea id='ducky-ed'></textarea>
  <div class='btns'>
    <a id='dl-link' href='/download-logs' class='btn btn-dl'>DESCARGAR ACTUAL</a>
    <a href='/download-pcap' class='btn btn-pcap'>BAJAR .PCAP</a>
    <a href='/download-screen' class='btn btn-pcap'>BAJAR BMP OLED</a>
  </div>
  <div class='btns'>
    <button onclick='saveDucky()' class='btn btn-dl'>GUARDAR DUCKY</button>
    <button onclick='takeBmp()' class='btn btn-pcap'>CAPTURAR OLED</button>
    <button onclick='clearLogs()' class='btn btn-del'>BORRAR LOGS</button>
  </div>
</div>
<script>
  let curTab = 'logins';
  function setTab(t) {
    curTab = t;
    ['logins','wardrive','trackers','ducky','scores'].forEach(k=>{
      document.getElementById('t-'+k).className = 'tab' + (t===k?' active':'');
    });
    document.getElementById('log-content').style.display = (t==='ducky')?'none':'block';
    document.getElementById('ducky-ed').style.display = (t==='ducky')?'block':'none';
    if (t==='ducky') {
      fetch('/get-ducky').then(r=>r.text()).then(d=>{ document.getElementById('ducky-ed').value = d; });
    } else {
      document.getElementById('dl-link').href = (t==='logins')?'/download-logs':((t==='wardrive')?'/download-wardrive':'/download-trackers');
      fetchLogs();
    }
  }
  function fetchStatus() {
    fetch('/api/status').then(r=>r.json()).then(d=>{
      document.getElementById('st-heap').innerText = d.heap + ' KB';
      document.getElementById('st-fs').innerText = d.fs_used + '/' + d.fs_total + ' KB';
      document.getElementById('st-up').innerText = d.uptime + ' s';
      document.getElementById('st-prof').innerText = d.profile;
    }).catch(()=>{});
  }
  function fetchLogs() {
    if (curTab==='ducky') return;
    let ep = (curTab==='logins')?'/get-logs':((curTab==='wardrive')?'/get-wardrive':((curTab==='trackers')?'/get-trackers':'/get-scores'));
    fetch(ep).then(r=>r.text()).then(d=>{
      document.getElementById('log-content').innerText = d || 'Sin registros.';
    });
  }
  function saveDucky() {
    let body = document.getElementById('ducky-ed').value;
    fetch('/save-ducky?script=' + encodeURIComponent(body)).then(()=>alert('Ducky Script guardado en /payload.txt'));
  }
  function takeBmp() {
    fetch('/snap-bmp').then(()=>alert('Captura OLED guardada en /screen.bmp'));
  }
  function clearLogs() {
    if(confirm('¿Borrar logs y capturas en SPIFFS?')) {
      fetch('/clear-logs').then(()=>fetchLogs());
    }
  }
  setInterval(()=>{ fetchStatus(); fetchLogs(); }, 4000);
  window.onload = ()=>{ fetchStatus(); fetchLogs(); };
</script></body></html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  asyncServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    const char *profNames[] = {"LAB", "AUDIT", "DEMO"};
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap() / 1024) + ",";
    json += "\"fs_used\":" + String(SPIFFS.usedBytes() / 1024) + ",";
    json += "\"fs_total\":" + String(SPIFFS.totalBytes() / 1024) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"profile\":\"" + String(profNames[g_settings.profileMode % 3]) + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  asyncServer.on("/get-scores", HTTP_GET, [](AsyncWebServerRequest *request) {
    const char *gNames[10] = {"1. SNAKE", "2. PONG", "3. FLAPPY", "4. INVADERS", "5. DINO RUN",
                              "6. BREAKOUT", "7. SIMON", "8. TETRIS", "9. ASTEROIDS", "10. 2048"};
    String out = "=== ARCADE LEADERBOARD (NVS) ===\n\n";
    for (int i = 0; i < 10; i++) {
      out += String(gNames[i]) + " : " + String(getArcadeHighScore(i)) + " pts\n";
    }
    request->send(200, "text/plain", out);
  });

  asyncServer.on("/get-ducky", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/payload.txt"))
      request->send(SPIFFS, "/payload.txt", "text/plain");
    else
      request->send(200, "text/plain", "REM Ducky Script\nDELAY 400\nSTRING Hola Mundo\nENTER\n");
  });

  asyncServer.on("/save-ducky", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("script")) {
      File f = SPIFFS.open("/payload.txt", FILE_WRITE);
      if (f) {
        f.print(request->getParam("script")->value());
        f.close();
      }
    }
    request->send(200, "text/plain", "OK");
  });

  asyncServer.on("/snap-bmp", HTTP_GET, [](AsyncWebServerRequest *request) {
    saveOledScreenshotBmp("/screen.bmp");
    request->send(200, "text/plain", "OK");
  });

  asyncServer.on("/get-logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/logins.txt"))
      request->send(SPIFFS, "/logins.txt", "text/plain");
    else
      request->send(200, "text/plain", "Sin registros en /logins.txt");
  });

  asyncServer.on("/download-logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/logins.txt"))
      request->send(SPIFFS, "/logins.txt", "text/plain", true);
    else
      request->send(404, "text/plain", "Sin archivo logins.txt");
  });

  asyncServer.on("/get-wardrive", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/wardrive.csv"))
      request->send(SPIFFS, "/wardrive.csv", "text/plain");
    else
      request->send(200, "text/plain", "Sin registros en /wardrive.csv");
  });

  asyncServer.on("/download-wardrive", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/wardrive.csv"))
      request->send(SPIFFS, "/wardrive.csv", "text/csv", true);
    else
      request->send(404, "text/plain", "Sin archivo wardrive.csv");
  });

  asyncServer.on("/get-trackers", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/trackers.txt"))
      request->send(SPIFFS, "/trackers.txt", "text/plain");
    else
      request->send(200, "text/plain", "Sin registros en /trackers.txt");
  });

  asyncServer.on("/download-trackers", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/trackers.txt"))
      request->send(SPIFFS, "/trackers.txt", "text/plain", true);
    else
      request->send(404, "text/plain", "Sin archivo trackers.txt");
  });

  asyncServer.on("/download-pcap", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/capture.pcap"))
      request->send(SPIFFS, "/capture.pcap", "application/octet-stream", true);
    else
      request->send(404, "text/plain", "Ejecuta primero 7. PCAP CAPTURE en WIFI AUDIT PRO");
  });

  asyncServer.on("/download-screen", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!SPIFFS.exists("/screen.bmp")) saveOledScreenshotBmp("/screen.bmp");
    request->send(SPIFFS, "/screen.bmp", "image/bmp", true);
  });

  asyncServer.on("/clear-logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    SPIFFS.remove("/logins.txt");
    SPIFFS.remove("/logins.txt.1");
    SPIFFS.remove("/wardrive.csv");
    SPIFFS.remove("/wardrive.csv.1");
    SPIFFS.remove("/trackers.txt");
    SPIFFS.remove("/trackers.txt.1");
    SPIFFS.remove("/capture.pcap");
    SPIFFS.remove("/capture.pcap.1");
    request->send(200, "text/plain", "OK");
  });

  asyncServer.begin();
}

#endif
