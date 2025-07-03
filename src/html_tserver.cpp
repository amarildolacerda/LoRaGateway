#ifdef WS
#include "html_tserver.h"
#include "logger.h"
#include <Arduino.h>
#include "config.h"
#ifdef DISPLAY_ENABLED
#include "displaymanager.h"
#endif
#include "systemstate.h"
#include "deviceinfo.h"
#include "loracom.h"
#ifndef __AVR__

#ifdef ESP32
#include <Update.h>
#endif

#include "ExtraQueue.h"
#include "WifiConn.h"

// Inicialização do membro estático
AsyncWebServer *HtmlServer::espServer = nullptr;

String HtmlServer::getCommonStyles()
{
    String styles = "  * { box-sizing: border-box; }";
    styles += "  body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; color: #333; }";
    styles += "  .container { max-width: 800px; margin: 0 auto; }";
    styles += "  h1 { text-align: center; color: #2c3e50; margin-bottom: 20px; }";
    styles += "  .card { background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); padding: 25px; margin-bottom: 25px; }";
    styles += "  h2 { color: #3498db; margin-top: 0; border-bottom: 1px solid #eee; padding-bottom: 10px; }";
    styles += "  .button-group { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 15px; }";
    styles += "  button, .btn { background: #3498db; color: white; border: none; padding: 12px 20px; border-radius: 6px; cursor: pointer; font-size: 16px; transition: all 0.3s; flex: 1 1 120px; text-decoration: none; display: inline-block; text-align: center; }";
    styles += "  button:hover, .btn:hover { background: #2980b9; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.1); }";
    styles += "  button:active, .btn:active { transform: translateY(0); }";
    styles += "  .status { color: #7f8c8d; font-size: 14px; margin-top: 5px; }";
    styles += "  .btn-danger { background: #e74c3c; }";
    styles += "  .btn-danger:hover { background: #c0392b; }";
    styles += "  .btn-success { background: #2ecc71; }";
    styles += "  .btn-success:hover { background: #27ae60; }";
    styles += "  .btn-warning { background: #f39c12; }";
    styles += "  .btn-warning:hover { background: #d35400; }";
    styles += "  .btn-info { background: #3498db; }";
    styles += "  .btn-info:hover { background: #2980b9; }";
    styles += "  @media (max-width: 600px) { .button-group { flex-direction: column; } button, .btn { width: 100%; } }";
    styles += "  .menu { background: #2c3e50; padding: 15px; margin-bottom: 20px; border-radius: 8px; display: flex; flex-wrap: wrap; gap: 10px; }";
    styles += "  .menu-item { color: white; text-decoration: none; padding: 8px 12px; border-radius: 4px; }";
    styles += "  .menu-item:hover { background: #3498db; }";
    styles += "  .upload-form { margin-top: 20px; }";
    styles += "  .progress { margin-top: 20px; width: 100%; background-color: #f1f1f1; border-radius: 4px; overflow: hidden; }";
    styles += "  .progress-bar { width: 0%; height: 30px; background-color: #4CAF50; text-align: center; line-height: 30px; color: white; transition: width 0.3s; }";
    styles += "  .device-card { display: inline-block; width: 200px; margin: 10px; padding: 15px; border: 1px solid #ddd; border-radius: 8px; text-align: center; background-color: #fff; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); }";
    styles += "  .device-card h3 { margin: 10px 0; font-size: 18px; color: #333; }";
    styles += "  .device-card button { margin-top: 10px; padding: 10px; background-color: #3498db; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    styles += "  .device-card button:hover { background-color: #2980b9; }";
    // Adicionando classes de status
    styles += "  .status-on { color: #2ecc71; font-weight: bold; }";
    styles += "  .status-off { color: #e74c3c; font-weight: bold; }";
    styles += "  .status-error { color: #f39c12; font-weight: bold; }";
    return styles;
}

String HtmlServer::generateMenu()
{
    // Menu
    String html = "<div class='menu'>";
    html += "<a href='/' class='menu-item'>Home</a>";
    html += "<a href='/ota' class='menu-item'>OTA Update</a>";
    html += "<a href='/restart' class='menu-item'>Reiniciar</a>";
    html += "<a href='/discovery' class='menu-item' id='discovery-btn'>+Add</a>";
    html += "<a href='/change' class='menu-item' id='discovery-btn'>Config</a>";
    html += "</div>";
    // Adiciona o script para controlar o modo de descoberta
    html += "<script>"
            "let discoveryActive = false;"
            "let discoveryTimeout;"
            "function toggleDiscovery() {"
            "  discoveryActive = !discoveryActive;"
            "  const btn = document.getElementById('discovery-btn');"
            "  if (discoveryActive) {"
            "    btn.style.backgroundColor = '#2ecc71';" // Verde quando ativo
            "    fetch('/discovery?enable=1', {method: 'POST'});"
            "    discoveryTimeout = setTimeout(() => {"
            "      discoveryActive = false;"
            "      btn.style.backgroundColor = '';"
            "    }, 60000);" // 1 minutos
            "  } else {"
            "    clearTimeout(discoveryTimeout);"
            "    btn.style.backgroundColor = '';"
            "    fetch('/discovery?enable=0', {method: 'POST'});"
            "  }"
            "}"
            "document.getElementById('discovery-btn').onclick = function(e) {"
            "  e.preventDefault();"
            "  toggleDiscovery();"
            "  return false;"
            "};"
            "</script>";
    return html;
}

void HtmlServer::generateHomePage(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='pt-BR'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Dispositivos Conectados</title>";
    html += "<style>";
    html += getCommonStyles();
    html += "  .device-actions { display: flex; gap: 5px; margin-top: 10px; }";
    html += "  .device-actions button { flex: 1; padding: 8px; font-size: 14px; }";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";
    html += generateMenu();

    html += "  <header>";
    html += "    <h1>Dispositivos Conectados</h1>";
    html += "  </header>";
    html += "  <div class='device-list'>";

    for (auto &data : deviceInfo.getDevices())
    {
        data.name.toUpperCase();
        html += "    <div class='device-card'>";
        html += "      <h3>";
        html += "      <div style='cursor:pointer;' onclick=\"window.location='/device?tid=" + String(data.tid) + "'\">" + String(data.name) + "</div></h3>";
        html += "      <hr/>";
        html += "      <p id='device-status-" + String(data.tid) + "' class='status'>...</p>";
        html += "      <div class='device-actions'>";
        html += "        <button onclick=\"controlDevice(" + String(data.tid) + ", 'on')\" class='btn-success'>Ligar</button>";
        html += "        <button onclick=\"controlDevice(" + String(data.tid) + ", 'off')\" class='btn-danger'>Desligar</button>";
        html += "      </div>";
        html += "    </div>";
    }

    html += "  </div>";
    html += "</div>";

    html += "<script>";
    html += "async function updateAllStatuses() {";
    html += "  try {";
    html += "    const res = await fetch('/batchStatus', { method: 'GET' });";
    html += "    if (res.ok) {";
    html += "      const data = await res.json();";
    html += "      data.devices.forEach(device => {";
    html += "        const statusElement = document.getElementById(`device-status-${device.tid}`);";
    html += "        if (statusElement) {";
    html += "          statusElement.innerText = device.status;";
    html += "          statusElement.className = ";
    html += "            device.status.includes('ON') ? 'status-on' :";
    html += "            device.status.includes('OFF') ? 'status-off' : 'status-error';";
    html += "        }";
    html += "      });";
    html += "    } else {";
    html += "      console.error('Failed to fetch status:', res.status);";
    html += "      setTimeout(updateAllStatuses, 500);";
    html += "    }";
    html += "  } catch (error) {";
    html += "    console.error('Error fetching status:', error);";
    html += "    setTimeout(updateAllStatuses, 1000); ";
    html += "  }";
    html += "}";
    html += "async function controlDevice(id, action) {";
    html += "  try {";
    html += "    await fetch(`/ctlDevice?tid=${id}&action=${action}`, { method: 'POST' });";
    html += "    const statusElement = document.getElementById(`device-status-${id}`);";
    html += "    if (statusElement) {";
    html += "      statusElement.innerText = action === 'on' ? 'Ligando...' : 'Desligando...';";
    html += "      statusElement.className = action === 'on' ? 'status-on' : 'status-off';";
    html += "    }";
    html += "    setTimeout(updateAllStatuses, 1000);";
    html += "  } catch (error) {";
    html += "    console.error('Error controlling device:', error);";
    html += "  }";
    html += "}";
    html += "updateAllStatuses();";
    html += "setInterval(updateAllStatuses, 1000);";
    html += "</script>";

    html += "</body></html>";

    request->send(200, "text/html", html.c_str());
}

void HtmlServer::generateDeviceDetailsPage(AsyncWebServerRequest *request, uint8_t tid)
{
    String html = "<!DOCTYPE html><html lang='pt-BR'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Detalhes do Dispositivo</title>";
    html += "<style>";
    html += getCommonStyles();
    html += "  .button-group { margin-top: 15px; }";
    html += "  .info-row { display: flex; justify-content: space-between; margin-bottom: 8px; }";
    html += "  .info-label { font-weight: bold; }";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";

    html += generateMenu();

    const uint8_t idx = deviceInfo.indexOf(tid);
    if (idx >= 0)
    {
        const DeviceData &rdata = deviceInfo.getDevices()[idx];

        html += "  <div class='card'>";
        html += "    <h2>" + rdata.name + "</h2>";
        html += "    <div class='info-row'><span class='info-label'>RSSI:</span><span id='device-rssi'>" + String(rdata.rssi) + " dBm</span></div>";
        html += "    <div class='info-row'><span class='info-label'>Última Atualização:</span><span id='device-lastseen'>" + String(deviceInfo.diffSeconds(rdata.lastSeen)) + "s</span></div>";
        html += "    <div class='info-row'><span class='info-label'>Estado:</span><span id='device-status' class='status-off'>Carregando...</span></div>";
        html += "    <div class='button-group'>";
        html += "      <button onclick=\"controlDevice('on')\" class='btn-success'>Ligar</button>";
        html += "      <button onclick=\"controlDevice('off')\" class='btn-danger'>Desligar</button>";
        html += "    </div>";
        html += "  </div>";

        html += "<script>";
        html += "async function updateDeviceInfo() {";
        html += "  try {";
        html += "    const res = await fetch('/batchStatus', { method: 'GET' });";
        html += "    if (res.ok) {";
        html += "      const data = await res.json();";
        html += "      const device = data.devices.find(d => d.tid == " + String(rdata.tid) + ");";
        html += "      if (device) {";
        html += "        document.getElementById('device-status').innerText = device.status;";
        html += "        document.getElementById('device-status').className = ";
        html += "          device.status.includes('ON') ? 'status-on' : ";
        html += "          device.status.includes('OFF') ? 'status-off' : 'status-error';";
        html += "        document.getElementById('device-rssi').innerText = device.rssi + ' dBm';";
        html += "        document.getElementById('device-lastseen').innerText = device.lastSeen + 's';";
        html += "      }";
        html += "    }";
        html += "  } catch (error) {";
        html += "    console.error('Error updating device info:', error);";
        html += "  }";
        html += "}";
        html += "async function controlDevice(action) {";
        html += "  try {";
        html += "    await fetch(`/ctlDevice?tid=" + String(rdata.tid) + "&action=${action}`, { method: 'POST' });";
        html += "    const statusElement = document.getElementById('device-status');";
        html += "    statusElement.innerText = action === 'on' ? 'Ligando...' : 'Desligando...';";
        html += "    statusElement.className = 'status-' + (action === 'on' ? 'on' : 'off');";
        html += "    setTimeout(updateDeviceInfo, 500);";
        html += "  } catch (error) {";
        html += "    console.error('Error controlling device:', error);";
        html += "  }";
        html += "}";
        html += "updateDeviceInfo();";
        html += "setInterval(updateDeviceInfo, 1000);";
        html += "</script>";
    }
    html += "  <a href='/' class='btn-info'>Voltar</a>";
    html += "</div>";
    html += "</body></html>";

    request->send(200, "text/html", html.c_str());
}

void HtmlServer::generateOTAPage(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='pt-BR'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>OTA Update</title>";
    html += "<style>";
    html += getCommonStyles();
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";

    html += generateMenu();

    html += "  <div class='card'>";
    html += "    <h2>OTA Update</h2>";
    html += "    <form class='upload-form' method='POST' action='/update' enctype='multipart/form-data'>";
    html += "      <input type='file' name='update' accept='.bin' style='margin-bottom: 15px; width: 100%; padding: 10px;'>";
    html += "      <button type='submit' class='btn-success'>Iniciar Atualização</button>";
    html += "    </form>";
    html += "    <div class='progress' style='display:none;'>";
    html += "      <div class='progress-bar' id='progress'>0%</div>";
    html += "    </div>";
    html += "  </div>";
    html += "</div>";

    html += "<script>";
    html += "document.querySelector('form').addEventListener('submit', function(e) {";
    html += "  e.preventDefault();";
    html += "  var formData = new FormData(this);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('POST', '/update', true);";
    html += "  xhr.upload.onprogress = function(e) {";
    html += "    if (e.lengthComputable) {";
    html += "      var percent = Math.round((e.loaded / e.total) * 100);";
    html += "      document.querySelector('.progress').style.display = 'block';";
    html += "      document.getElementById('progress').style.width = percent + '%';";
    html += "      document.getElementById('progress').innerHTML = percent + '%';";
    html += "    }";
    html += "  };";
    html += "  xhr.onload = function() {";
    html += "    if (this.status == 200) {";
    html += "      alert('Atualização concluída com sucesso! O dispositivo irá reiniciar.');";
    html += "      setTimeout(function() { window.location.href = '/'; }, 2000);";
    html += "    } else {";
    html += "      alert('Erro na atualização: ' + this.responseText);";
    html += "      document.querySelector('.progress').style.display = 'none';";
    html += "    }";
    html += "  };";
    html += "  xhr.send(formData);";
    html += "});";
    html += "</script>";

    html += "</body></html>";

    request->send(200, "text/html", html.c_str());
}

void HtmlServer::handleRootRequest(AsyncWebServerRequest *request)
{
    generateHomePage(request);
}

void HtmlServer::handleDeviceDetailsRequest(AsyncWebServerRequest *request)
{
    uint8_t tid = request->hasArg("tid") ? request->arg("tid").toInt() : 0xFF;
    uint8_t idx = deviceInfo.indexOf(tid);
    if (idx >= 0)
    {
        generateDeviceDetailsPage(request, tid);
    }
    else
    {
        request->send(404, "text/plain", "Dispositivo não encontrado");
    }
}

void HtmlServer::respStatus(AsyncWebServerRequest *request, uint8_t tid, String status)
{
    String response = "{ \"tid\": " + String(tid) +
                      ", \"status\": \"" + status + "\" }";
    request->send(200, "application/json", response);
}

void HtmlServer::handleToggleDevice(AsyncWebServerRequest *request)
{
    uint8_t tid = request->hasArg("tid") ? request->arg("tid").toInt() : 0xFF;
    String action = request->hasArg("action") ? request->arg("action") : "none";
    action.toLowerCase();

    const int16_t x = deviceInfo.indexOf(tid);

    if (action != EVT_STATUS)
    {
        txExtraQueue.push(tid, EVT_GPIO, action.c_str(), TERMINAL_ID, 3);
        Logger::info("WS %d %s", tid, action);
    }

    String status = "Aguardando";
    if (x >= 0)
    {
        const DeviceData &data = deviceInfo.getDevices()[x];

        int timeDiff = deviceInfo.diffSeconds(data.lastSeen);
        bool isOffline = (timeDiff == -1) || (timeDiff > 60 * 5);
        status = isOffline ? "Não Responde" : (data.state ? "ON" : "OFF");
    }
    respStatus(request, tid, status);
}

void HtmlServer::doOTAUpdate(AsyncWebServerRequest *request)
{
}

void HtmlServer::initWebServer(AsyncWebServer *server)
{
    espServer = server;

    // Rotas principais
    espServer->on("/", HTTP_GET, handleRootRequest);
    espServer->on("/home", HTTP_GET, handleRootRequest);
    espServer->on("/change", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
                    request->send(200, "text/plain", "Portal de config iniciado. Conecte-se ao AP ESP32-Config para alterar a rede.");
                    
                    wifiConn.changeNetwork(); });

    espServer->on("/device", HTTP_GET, handleDeviceDetailsRequest);
    espServer->on("/ctlDevice", HTTP_POST, [](AsyncWebServerRequest *request)
                  { handleToggleDevice(request); });

    espServer->on("/batchStatus", HTTP_GET, [](AsyncWebServerRequest *request)
                  { 
                      //Serial.println("Handling batch status request"); // Debug log
                      handleBatchStatusRequest(request); });
    espServer->on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
                      deviceInfo.clear();
                      request->redirect("/");
                      delay(100);
                      ESP.restart(); });
    espServer->on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
                      request->redirect("/");
                      delay(1000);
                      ESP.restart(); });
    espServer->on("/resetWifi", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
                      request->redirect("/");
                      delay(1000);
                      wifiConn.resetWifi();
                      ESP.restart(); });

    espServer->on("/discovery", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
                      if (request->hasArg("enable")) {
                          systemState.setDiscovering(request->arg("enable") == "1");
                          request->send(200, "text/plain", "OK");
                      } else {
                          request->send(400, "text/plain", "Bad Request");
                      } });

    // Rotas OTA
    espServer->on("/ota", HTTP_GET, generateOTAPage);
    espServer->on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
                  { doOTAUpdate(request); });
}

void HtmlServer::handleBatchStatusRequest(AsyncWebServerRequest *request)
{
    // Serial.println("Processing batch status request"); // Debug log

    DynamicJsonDocument doc(1024);
    JsonArray devices = doc.createNestedArray("devices");

    for (const auto &data : deviceInfo.getDevices())
    {
        if (data.tid > 0)
        {
            JsonObject device = devices.createNestedObject();
            device["tid"] = data.tid;

            int timeDiff = deviceInfo.diffSeconds(data.lastSeen);
            bool isOffline = (timeDiff == -1) || (timeDiff > 60 * 5);
            device["status"] = isOffline ? "Não Responde" : (data.state ? "ON" : "OFF");
            device["name"] = data.name;
            device["rssi"] = data.rssi;
            device["lastSeen"] = timeDiff;

            // Serial.printf("Device %d - Status: %s, RSSI: %d, LastSeen: %ds\n",
            //               data.tid, device["status"].as<const char *>(), data.rssi, timeDiff); // Debug log
        }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

bool HtmlServer::begin()
{
    espServer->begin();
    Serial.println("HTTP server started");
    return true;
}

void HtmlServer::process()
{
    // Processamento periódico se necessário
}

HtmlServer htmlServer;

#endif
#endif