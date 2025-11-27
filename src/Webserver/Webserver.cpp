#include "Arduino.h"

#include "Webserver.h"
#include "../Sync/Sync.h"
#include <ctime> // For time_t, gmtime, strftime

// Initialize static instance pointer
Webserver* Webserver::instance = nullptr;

Webserver::Webserver(): server(80) {
  instance = this;  // Set the instance pointer

  server.on("/config", handleConfig);
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);
  server.on("/info", handleInfo);

  if (deviceConfig.isConfigured()) {
    server.on("/", handleRoot);
  } else {
    server.on("/", handleConfig);
  }

  server.on("/pulse", handlePulse);
  server.onNotFound(handleNotFound);
  server.begin();
}

void Webserver::handleConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset=\"UTF-8\">";
  html += "<title>ESP-PORTATEC Configuration</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "input { padding: 10px; margin: 10px; width: 80%; max-width: 300px; }";
  html += "button { padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background-color: #45a049; }";
  html += ".section { margin: 20px 0; padding: 20px; border: 1px solid #ddd; border-radius: 4px; }";
  html += "h2 { margin-top: 0; }";
  html += ".chip-id { font-size: 14px; color: #666; margin-bottom: 20px; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>ESP-PORTATEC Configuration</h1>";
  html += "<div class='chip-id'>Chip ID: " + String(ESP.getChipId(), HEX) + "</div>";
  html += "<form action='/saveconfig' method='POST'>";

  // Device Configuration Section
  html += "<div class='section'>";
  html += "<h2>Device Configuration</h2>";
  html += "<input type='text' name='devicename' placeholder='Device Name' value='" + String(deviceConfig.getDeviceName()) + "' required><br>";
  html += "<input type='password' name='password' placeholder='WiFi Password' value='" + String(deviceConfig.getPassword()) + "' required><br>";
  html += "<input type='number' name='pulsepin' placeholder='Pulse Pin' value='" + String(deviceConfig.getPulsePin()) + "' required><br>";
  html += "<input type='password' name='pin' placeholder='PIN' value='" + String(deviceConfig.getPin()) + "' required><br>";
  html += String("<input type='checkbox' name='pulseinverted' id='pulseinverted' value='true'") + (deviceConfig.getPulseInverted() ? " checked" : "") + "><label for='pulseinverted'>Invert Pulse</label><br>";
  html += "<input type='number' name='sensorpin' placeholder='Sensor Pin' value='" + (deviceConfig.getSensorPin() == DeviceConfig::UNCONFIGURED_PIN ? "" : String(deviceConfig.getSensorPin())) + "'><br>";
  html += "</div>";

  // WiFi Network Configuration Section
  html += "<div class='section'>";
  html += "<h2>WiFi Network Configuration</h2>";
  html += "<input type='text' name='wifissid' placeholder='WiFi Network Name (SSID)' value='" + String(deviceConfig.getWifiSSID()) + "'><br>";
  html += "<input type='password' name='wifipass' placeholder='WiFi Network Password' value='" + String(deviceConfig.getWifiNetworkPass()) + "'><br>";
  html += "</div>";

  html += "<button type='submit'>Save Configuration</button>";
  html += "</form></body></html>";
  instance->server.send(200, "text/html", html);
}

void Webserver::handleSaveConfig() {
  if (
    instance->server.hasArg("devicename")
    && instance->server.hasArg("password")
    && instance->server.hasArg("pulsepin")
    && instance->server.hasArg("sensorpin")
    && instance->server.hasArg("wifissid")
    && instance->server.hasArg("wifipass")
    && instance->server.hasArg("pin")
  ) {
    String deviceName = instance->server.arg("devicename");
    String password = instance->server.arg("password");
    String pulsePinStr = instance->server.arg("pulsepin");
    String pulseInvertedStr = instance->server.arg("pulseinverted");
    String sensorPinStr = instance->server.arg("sensorpin");
    String wifiSSID = instance->server.arg("wifissid");
    String wifiPass = instance->server.arg("wifipass");
    String pin = instance->server.arg("pin");

    if (
      deviceName.length() > 0
      && password.length() > 0
      && pulsePinStr.length() > 0
      && pin.length() > 0
    ) {
      deviceConfig.setDeviceName(deviceName.c_str());
      deviceConfig.setPassword(password.c_str());
      deviceConfig.setPulsePin(pulsePinStr.toInt());
      deviceConfig.setPulseInverted(pulseInvertedStr == "true");
      deviceConfig.setPin(pin.c_str());

      if (sensorPinStr.length() > 0) {
        deviceConfig.setSensorPin(sensorPinStr.toInt());
      } else {
        deviceConfig.setSensorPin(DeviceConfig::UNCONFIGURED_PIN);
      }

      // Set WiFi network configuration if provided
      if (wifiSSID.length() > 0) {
        deviceConfig.setWifiSSID(wifiSSID.c_str());
        deviceConfig.setWifiNetworkPass(wifiPass.c_str());
      }

      deviceConfig.saveConfig();

      // Restart ESP to apply new configuration
      instance->server.send(200, "text/html", "<script>setTimeout(function(){ window.location.href='/'; }, 3000);</script>Configuration saved! Restarting...");
      delay(3000);
      ESP.restart();
    }
  }
  instance->server.send(400, "text/plain", "Invalid configuration");
}

void Webserver::handleNotFound() {
  instance->server.sendHeader("Location", "http://" + myIP.toString(), true);
  instance->server.send(302, "text/plain", "");
}

void Webserver::handlePulse() {
  if (instance->server.hasArg("pin")) {
    String pin = instance->server.arg("pin");
    pin.trim(); // Remove any accidental whitespace
    if (pin == deviceConfig.getPin()) {
      uint8_t pin = deviceConfig.getPulsePin();
      bool inverted = deviceConfig.getPulseInverted();
      digitalWrite(pin, inverted ? LOW : HIGH);
      delay(500);
      digitalWrite(pin, inverted ? HIGH : LOW);
      instance->server.send(200, "text/plain", "GPIO " + String(pin) + " toggled");
    } else {
      // Debugging aid: show what was received vs expected
      String errorMsg = "Invalid PIN. Received: '" + pin + "', Expected: '" + String(deviceConfig.getPin()) + "'";
      instance->server.send(401, "text/plain", errorMsg);
    }
  } else {
    instance->server.send(400, "text/plain", "PIN required");
  }
}

void Webserver::handleRoot() {
  instance->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  instance->server.send(200, "text/html", "");

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset=\"UTF-8\">";
  // html += "<meta http-equiv='refresh' content='10'>";  // Auto refresh disabled to prevent modal issues
  html += "<title>ESP-PORTATEC Control</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "button { padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }";
  html += "button:hover { background-color: #45a049; }";
  html += "button:disabled { background-color: #cccccc; cursor: not-allowed; }";
  html += ".working { background-color: #ff9800 !important; }";
  html += ".button-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin: 20px 0; }";
  html += ".status-display { font-size: 18px; margin: 20px auto; padding: 15px; border-radius: 8px; max-width: 300px; width: 100%; }";
  html += ".status-closed { background-color: #f44336; color: white; }";
  html += ".status-open { background-color: #4CAF50; color: white; }";
  html += ".modal { display: none; position: fixed; z-index: 1; left: 0; top: 0; width: 100%; height: 100%; overflow: auto; background-color: rgba(0,0,0,0.4); }";
  html += ".modal-content { background-color: #fefefe; margin: 15% auto; padding: 20px; border: 1px solid #888; width: 80%; max-width: 300px; text-align: center; border-radius: 8px; }";
  html += ".pin-inputs { display: flex; justify-content: center; gap: 10px; margin: 20px 0; }";
  html += ".pin-inputs input { width: 40px; height: 40px; text-align: center; font-size: 20px; border: 1px solid #ddd; border-radius: 4px; }";
  html += "</style></head>";
  instance->server.sendContent(html);

  html = "<body>";
  html += "<h1>ESP-PORTATEC Control</h1>";
  html += "<p>Dispositivo: " + String(deviceConfig.getDeviceName()) + "</p>";

  // Sensor status display
  if (deviceConfig.getSensorPin() != DeviceConfig::UNCONFIGURED_PIN) {
    bool sensorState = digitalRead(deviceConfig.getSensorPin());
    html += "<div class='status-display " + String(sensorState ? "status-closed" : "status-open") + "'>";
    html += "Status: " + String(sensorState ? "FECHADO" : "ABERTO");
    html += "</div>";
  }

  html += "<div class='button-container'>";
  html += "<button id='pulseButton' onclick='openPinModal()'>Abrir</button>";
  html += "</div>";

  // PIN Modal
  html += "<div id='pinModal' class='modal'>";
  html += "<div class='modal-content'>";
  html += "<h2>Digite o PIN</h2>";
  html += "<div id='pinMessage' style='color: red; margin-bottom: 10px;'></div>";
  html += "<div class='pin-inputs'>";
  for (int i = 0; i < 6; i++) {
    html += "<input type='tel' inputmode='numeric' pattern='[0-9]*' maxlength='1' id='pin" + String(i) + "'>";
  }
  html += "</div>";
  html += "<button id='confirmPinButton' onclick='submitPin()'>Confirmar</button>";
  html += "</div>";
  html += "</div>";
  instance->server.sendContent(html);

  // Chunk 3: JS Functions (openPinModal, submitPin)
  html = "<script>";
  html += "function openPinModal() { ";
  html += "  document.getElementById('pinModal').style.display = 'block'; ";
  html += "  document.getElementById('pin0').focus(); ";
  html += "  document.getElementById('pinMessage').textContent = ''; ";
  html += "}";
  html += "function submitPin() {";
  html += "  let pin = '';";
  html += "  for (let i = 0; i < 6; i++) { pin += document.getElementById('pin' + i).value; }";
  html += "  pulseGpio(pin);";
  html += "}";
  instance->server.sendContent(html);

  // Chunk 4: JS Listeners
  html = "const pinInputs = document.querySelector('.pin-inputs');";
  html += "if (pinInputs) {"; // Added safety check
  html += "pinInputs.addEventListener('input', (e) => {";
  html += "  const target = e.target;";
  html += "  target.value = target.value.replace(/[^0-9]/g, '');";
  html += "  const next = target.nextElementSibling;";
  html += "  if (target.value && next) { next.focus(); }";
  html += "});";
  html += "pinInputs.addEventListener('keydown', (e) => {";
  html += "  const target = e.target;";
  html += "  const prev = target.previousElementSibling;";
  html += "  if (e.key === 'Backspace' && !target.value && prev) { prev.focus(); }";
  html += "});";
  html += "pinInputs.addEventListener('paste', (e) => {";
  html += "  e.preventDefault();";
  html += "  let paste = (e.clipboardData || window.clipboardData).getData('text');";
  html += "  paste = paste.replace(/[^0-9]/g, '');";
  html += "  const inputs = pinInputs.querySelectorAll('input');";
  html += "  for (let i = 0; i < Math.min(inputs.length, paste.length); i++) { inputs[i].value = paste[i]; }";
  html += "  if (paste.length > 0) { inputs[Math.min(inputs.length - 1, paste.length - 1)].focus(); }";
  html += "});";
  html += "}"; // End safety check
  instance->server.sendContent(html);
  
  // Chunk 5: pulseGpio function
  html = "function pulseGpio(pin) {";
  html += "  const button = document.getElementById('confirmPinButton');";
  html += "  const pinMessage = document.getElementById('pinMessage');";
  html += "  button.disabled = true;";
  html += "  button.classList.add('working');";
  html += "  const originalText = button.textContent;";
  html += "  button.textContent = 'Validando...';";
  html += "  fetch('/pulse?pin=' + pin)";
  html += "    .then(response => {";
  html += "      if (response.status !== 200) {";
  html += "        pinMessage.style.color = 'red';";
  html += "        pinMessage.textContent = 'PIN incorreto!';";
  html += "        const pin0 = document.getElementById('pin0');";
  html += "        if (pin0) {";
  html += "          for (let i = 0; i < 6; i++) {";
  html += "             const input = document.getElementById('pin' + i);";
  html += "             if (input) { input.select(); }";
  html += "          }";
  html += "          pin0.focus();";
  html += "        }";
  html += "      } else {";
  html += "        pinMessage.style.color = 'green';";
  html += "        pinMessage.textContent = 'Sucesso: Comando enviado!';";
  html += "        setTimeout(() => {";
  html += "          document.getElementById('pinModal').style.display = 'none';";
  html += "          for (let i = 0; i < 6; i++) { document.getElementById('pin' + i).value = ''; }";
  html += "        }, 1000);";
  html += "      }";
  html += "      return response.text();";
  html += "    })";
  html += "    .then(data => {";
  html += "      console.log(data);";
  html += "      button.disabled = false;";
  html += "      button.classList.remove('working');";
  html += "      button.textContent = originalText;";
  html += "    });";
  html += "}";
  html += "</script></body></html>";
  instance->server.sendContent(html);
}

void Webserver::handleInfo() {
  instance->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  instance->server.send(200, "text/html", "");

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset=\"UTF-8\">";
  html += "<meta http-equiv='refresh' content='10'>";  // Auto refresh every 10 seconds
  html += "<title>ESP-PORTATEC Informações</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; }";
  html += ".section { margin: 20px 0; padding: 20px; border: 1px solid #ddd; border-radius: 8px; background-color: #f9f9f9; }";
  html += "h1 { text-align: center; color: #333; }";
  html += "h2 { margin-top: 0; color: #4CAF50; border-bottom: 2px solid #4CAF50; padding-bottom: 5px; }";
  html += ".info-row { display: flex; justify-content: space-between; margin: 10px 0; padding: 8px; background-color: white; border-radius: 4px; }";
  html += ".info-label { font-weight: bold; color: #555; }";
  html += ".info-value { color: #333; }";
  html += ".status-connected { color: #4CAF50; font-weight: bold; }";
  html += ".status-disconnected { color: #f44336; font-weight: bold; }";
  html += ".status-syncing { color: #2196F3; font-weight: bold; }";
  html += ".back-button { display: block; width: 200px; margin: 20px auto; padding: 10px; text-align: center; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".back-button:hover { background-color: #45a049; }";
  html += ".signal-bar { display: inline-block; width: 100px; height: 20px; background: linear-gradient(90deg, #f44336 0%, #ff9800 50%, #4CAF50 100%); border-radius: 10px; position: relative; }";
  html += ".signal-indicator { position: absolute; top: 0; left: 0; height: 100%; background-color: rgba(255,255,255,0.8); border-radius: 10px; }";
  html += "</style></head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>Informações do Sistema</h1>";
  instance->server.sendContent(html);

  // Chunk 2: Device Information
  html = "<div class='section'>";
  html += "<h2>Informações do Dispositivo</h2>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Nome do Dispositivo:</span>";
  html += "<span class='info-value'>" + String(deviceConfig.getDeviceName()) + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Chip ID:</span>";
  html += "<span class='info-value'>" + String(ESP.getChipId(), HEX) + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Versão do Firmware:</span>";
  html += "<span class='info-value'>" + String(DeviceConfig::FIRMWARE_VERSION) + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Tempo Ligado:</span>";
  unsigned long uptime = millis() / 1000;
  unsigned long days = uptime / 86400;
  unsigned long hours = (uptime % 86400) / 3600;
  unsigned long minutes = (uptime % 3600) / 60;
  unsigned long seconds = uptime % 60;
  html += "<span class='info-value'>" + String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Data e Hora Atual:</span>";
  html += "<span class='info-value'>" + formatUnixTime(systemClock.getUnixTime()) + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Pino Pulso:</span>";
  html += "<span class='info-value'>GPIO " + String(deviceConfig.getPulsePin()) + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Pino Sensor:</span>";
  if (deviceConfig.getSensorPin() != DeviceConfig::UNCONFIGURED_PIN) {
    html += "<span class='info-value'>GPIO " + String(deviceConfig.getSensorPin()) + " (" + (digitalRead(deviceConfig.getSensorPin()) ? "ALTO" : "BAIXO") + ")</span>";
  } else {
    html += "<span class='info-value'>N/A</span>";
  }
  html += "</div>";
  html += "</div>";
  instance->server.sendContent(html);

  // Chunk 3: WiFi Information
  html = "<div class='section'>";
  html += "<h2>Informações WiFi</h2>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Status WiFi:</span>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<span class='info-value status-connected'>Conectado</span>";
  } else {
    html += "<span class='info-value status-disconnected'>Desconectado</span>";
  }
  html += "</div>";

  if (WiFi.status() == WL_CONNECTED) {
    html += "<div class='info-row'>";
    html += "<span class='info-label'>Nome da Rede:</span>";
    html += "<span class='info-value'>" + WiFi.SSID() + "</span>";
    html += "</div>";
    html += "<div class='info-row'>";
    html += "<span class='info-label'>Endereço IP:</span>";
    html += "<span class='info-value'>" + WiFi.localIP().toString() + "</span>";
    html += "</div>";
    html += "<div class='info-row'>";
    html += "<span class='info-label'>Gateway:</span>";
    html += "<span class='info-value'>" + WiFi.gatewayIP().toString() + "</span>";
    html += "</div>";
    html += "<div class='info-row'>";
    html += "<span class='info-label'>DNS:</span>";
    html += "<span class='info-value'>" + WiFi.dnsIP().toString() + "</span>";
    html += "</div>";
    html += "<div class='info-row'>";
    html += "<span class='info-label'>Potência do Sinal:</span>";
    int32_t rssi = WiFi.RSSI();
    int signalPercent = constrain(map(rssi, -100, -30, 0, 100), 0, 100);
    html += "<span class='info-value'>" + String(rssi) + " dBm (" + String(signalPercent) + "%)";
    html += "<div class='signal-bar'><div class='signal-indicator' style='width:" + String(100-signalPercent) + "%'></div></div>";
    html += "</span>";
    html += "</div>";
  } else {
    html += "<div class='info-row'>";
    html += "<span class='info-label'>Rede Configurada:</span>";
    html += "<span class='info-value'>" + String(deviceConfig.getWifiSSID()) + "</span>";
  }
  html += "</div>";
  instance->server.sendContent(html);

  // Chunk 4: AP Info, Sync Info and Footer
  html = "<div class='section'>";
  html += "<h2>Ponto de Acesso</h2>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Nome do AP:</span>";
  html += "<span class='info-value'>" + String(deviceConfig.getDeviceName()) + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>IP do AP:</span>";
  html += "<span class='info-value'>" + WiFi.softAPIP().toString() + "</span>";
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Clientes Conectados:</span>";
  html += "<span class='info-value'>" + String(WiFi.softAPgetStationNum()) + "</span>";
  html += "</div>";
  html += "</div>";

  // Sync Information
  html += "<div class='section'>";
  html += "<h2>Informações de Sincronização</h2>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Status da Conexão:</span>";
  if (sync.isConnected()) {
    html += "<span class='info-value status-connected'>Conectado</span>";
  } else {
    html += "<span class='info-value status-disconnected'>Desconectado</span>";
  }
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Status da Sincronização:</span>";
  if (sync.isSyncing()) {
    html += "<span class='info-value status-syncing'>Sincronizando</span>";
  } else if (sync.isConnected()) {
    html += "<span class='info-value status-disconnected'>Parado</span>";
  } else {
    html += "<span class='info-value status-disconnected'>Offline</span>";
  }
  html += "</div>";
  html += "<div class='info-row'>";
  html += "<span class='info-label'>Última Sincronização:</span>";
  unsigned long lastSync = sync.getLastSuccessfulSync();
  if (lastSync > 0) {
    unsigned long timeSinceSync = (millis() - lastSync) / 1000;
    if (timeSinceSync < 60) {
      html += "<span class='info-value'>" + String(timeSinceSync) + " segundos atrás</span>";
    } else if (timeSinceSync < 3600) {
      html += "<span class='info-value'>" + String(timeSinceSync / 60) + " minutos atrás</span>";
    } else if (timeSinceSync < 86400) {
      html += "<span class='info-value'>" + String(timeSinceSync / 3600) + " horas atrás</span>";
    } else {
      html += "<span class='info-value'>" + String(timeSinceSync / 86400) + " dias atrás</span>";
    }
  } else {
    html += "<span class='info-value status-disconnected'>Nunca sincronizado</span>";
  }
  html += "</div>";
  html += "</div>";

  html += "<a href='/' class='back-button'>← Voltar</a>";
  html += "</div>";
  html += "</body></html>";

  instance->server.sendContent(html);
}

void Webserver::handleClient() {
  server.handleClient();
}

String Webserver::formatUnixTime(unsigned long unix_timestamp) {
  if (unix_timestamp == 0) return "N/A (Não sincronizado)";

  time_t rawtime = unix_timestamp;
  struct tm * ti;
  ti = localtime(&rawtime); // Use localtime for local time, or gmtime for UTC

  char buffer[64]; // Increased size to safely accommodate the formatted string
  // Example: 2025-11-26 14:30:00
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
          ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
          ti->tm_hour, ti->tm_min, ti->tm_sec);
  return String(buffer);
}
