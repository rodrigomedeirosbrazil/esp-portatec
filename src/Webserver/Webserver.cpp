#include "Arduino.h"

#include "Webserver.h"
#include "../Sync/Sync.h"

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
  ) {
    String deviceName = instance->server.arg("devicename");
    String password = instance->server.arg("password");
    String pulsePinStr = instance->server.arg("pulsepin");
    String pulseInvertedStr = instance->server.arg("pulseinverted");
    String sensorPinStr = instance->server.arg("sensorpin");
    String wifiSSID = instance->server.arg("wifissid");
    String wifiPass = instance->server.arg("wifipass");

    if (
      deviceName.length() > 0
      && password.length() > 0
      && pulsePinStr.length() > 0
    ) {
      deviceConfig.setDeviceName(deviceName.c_str());
      deviceConfig.setPassword(password.c_str());
      deviceConfig.setPulsePin(pulsePinStr.toInt());
      deviceConfig.setPulseInverted(pulseInvertedStr == "true");

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
  uint8_t pin = deviceConfig.getPulsePin();
  bool inverted = deviceConfig.getPulseInverted();
  digitalWrite(pin, inverted ? LOW : HIGH);
  delay(500);
  digitalWrite(pin, inverted ? HIGH : LOW);
  instance->server.send(200, "text/plain", "GPIO " + String(pin) + " toggled");
}

void Webserver::handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset=\"UTF-8\">";
  html += "<meta http-equiv='refresh' content='10'>";  // Auto refresh every 10 seconds
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
  html += "</style></head>";
  html += "<body>";
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
  html += "<button id='pulseButton' onclick='pulseGpio()'>Abrir</button>";
  html += "<button onclick=\"window.location.href='/info'\">Informações</button>";
  html += "</div>";
  html += "<script>";
  html += "function pulseGpio() {";
  html += "  const button = document.getElementById('pulseButton');";
  html += "  button.disabled = true;";
  html += "  button.classList.add('working');";
  html += "  button.textContent = 'Abrindo...';";
  html += "  fetch('/pulse')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      console.log(data);";
  html += "      setTimeout(() => {";
  html += "        button.disabled = false;";
  html += "        button.classList.remove('working');";
  html += "        button.textContent = 'Abrir';";
  html += "      }, 500);";
  html += "    });";
  html += "}";
  html += "</script></body></html>";
  instance->server.send(200, "text/html", html);
}

void Webserver::handleInfo() {
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

  // Device Information
  html += "<div class='section'>";
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

  // WiFi Information
  html += "<div class='section'>";
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

  // Access Point Information
  html += "<div class='section'>";
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

  instance->server.send(200, "text/html", html);
}

void Webserver::handleClient() {
  server.handleClient();
}
