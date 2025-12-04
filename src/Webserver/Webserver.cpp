#include "Arduino.h"

#include "Webserver.h"
#include "../Sync/Sync.h"
#include <ctime> // For time_t, gmtime, strftime
#include <LittleFS.h>

// Initialize static instance pointer
Webserver* Webserver::instance = nullptr;

Webserver::Webserver(): server(80) {
  instance = this;  // Set the instance pointer
  LittleFS.begin();

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
  sendHtml("/config.html", [](String html) -> String {
    html.replace("%CHIP_ID%", String(ESP.getChipId(), HEX));
    html.replace("%DEVICE_NAME%", String(deviceConfig.getDeviceName()));
    html.replace("%PASSWORD%", String(deviceConfig.getPassword()));
    html.replace("%PULSE_PIN%", String(deviceConfig.getPulsePin()));
    html.replace("%PIN%", String(deviceConfig.getPin()));
    html.replace("%PULSE_INVERTED_CHECK%", deviceConfig.getPulseInverted() ? " checked" : "");
    html.replace("%SENSOR_PIN%", deviceConfig.getSensorPin() == DeviceConfig::UNCONFIGURED_PIN ? "" : String(deviceConfig.getSensorPin()));
    html.replace("%WIFI_SSID%", String(deviceConfig.getWifiSSID()));
    html.replace("%WIFI_PASS%", String(deviceConfig.getWifiNetworkPass()));
    return html;
  });
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
      delay(3000); // Add 3-second delay for incorrect PIN
      instance->server.send(401, "application/json", "{\"success\":false,\"message\":\"PIN incorreto!\"}");
    }
  } else {
    instance->server.send(400, "text/plain", "PIN required");
  }
}

void Webserver::handleRoot() {
  sendHtml("/index.html", [](String html) -> String {
    html.replace("%DEVICE_NAME%", String(deviceConfig.getDeviceName()));
    
    if (deviceConfig.getSensorPin() != DeviceConfig::UNCONFIGURED_PIN) {
      bool sensorState = digitalRead(deviceConfig.getSensorPin());
      String statusHtml = "<div class='status-display " + String(sensorState ? \"status-closed\" : \"status-open\") + "'>";
      statusHtml += "Status: " + String(sensorState ? "FECHADO" : "ABERTO");
      statusHtml += "</div>";
      html.replace("%SENSOR_STATUS_HTML%", statusHtml);
    } else {
       html.replace("%SENSOR_STATUS_HTML%", "");
    }
    return html;
  });
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
    html += "<span class='info-value'>" + String(rssi) + " dBm (" + String(signalPercent) + "%) ";
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

void Webserver::sendHtml(const String& path, std::function<String(const String&)> processor) {
  if (!LittleFS.exists(path)) {
    instance->server.send(404, "text/plain", "File not found: " + path);
    return;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    instance->server.send(500, "text/plain", "Failed to open file: " + path);
    return;
  }

  String html = file.readString();
  file.close();

  if (processor) {
    html = processor(html);
  }

  instance->server.send(200, "text/html", html);
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
