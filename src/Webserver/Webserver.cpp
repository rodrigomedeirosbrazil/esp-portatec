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
    server.on("/", handleIndex);
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
    unsigned long timestamp = systemClock.getUnixTime();

    int authorizedId = accessManager.validate(pin);

    if (authorizedId != 0) {
      uint8_t pulsePin = deviceConfig.getPulsePin();
      bool inverted = deviceConfig.getPulseInverted();
      digitalWrite(pulsePin, inverted ? LOW : HIGH);
      delay(500);
      digitalWrite(pulsePin, inverted ? HIGH : LOW);

      sync.sendAccessEvent(pin.c_str(), "valid", timestamp);
      instance->server.send(200, "text/plain", "GPIO " + String(pulsePin) + " toggled");
    } else {
      sync.sendAccessEvent(pin.c_str(), "invalid", timestamp);
      delay(3000); // Add 3-second delay for incorrect PIN
      instance->server.send(401, "application/json", "{\"success\":false,\"message\":\"PIN incorreto!\"}");
    }
  } else {
    instance->server.send(400, "text/plain", "PIN required");
  }
}

void Webserver::handleIndex() {
  sendHtml("/index.html", [](String html) -> String {
    html.replace("%DEVICE_NAME%", String(deviceConfig.getDeviceName()));
    
    if (deviceConfig.getSensorPin() != DeviceConfig::UNCONFIGURED_PIN) {
      bool sensorState = digitalRead(deviceConfig.getSensorPin());
      String statusHtml = "<div class='status-display " + String(sensorState ? "status-closed" : "status-open") + "'>";
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
  sendHtml("/info.html", [](String html) -> String {
    // Device Info
    html.replace("%DEVICE_NAME%", String(deviceConfig.getDeviceName()));
    html.replace("%CHIP_ID%", String(ESP.getChipId(), HEX));
    html.replace("%FIRMWARE_VERSION%", String(DeviceConfig::FIRMWARE_VERSION));
    
    unsigned long uptime = millis() / 1000;
    unsigned long days = uptime / 86400;
    unsigned long hours = (uptime % 86400) / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    html.replace("%UPTIME%", String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s");
    
    html.replace("%CURRENT_TIME%", formatUnixTime(systemClock.getUnixTime()));
    html.replace("%PULSE_PIN%", String(deviceConfig.getPulsePin()));
    
    if (deviceConfig.getSensorPin() != DeviceConfig::UNCONFIGURED_PIN) {
        html.replace("%SENSOR_PIN_INFO%", "GPIO " + String(deviceConfig.getSensorPin()) + " (" + (digitalRead(deviceConfig.getSensorPin()) ? "ALTO" : "BAIXO") + ")");
    } else {
        html.replace("%SENSOR_PIN_INFO%", "N/A");
    }

    // WiFi Info
    if (WiFi.status() == WL_CONNECTED) {
        html.replace("%WIFI_STATUS_CLASS%", "status-connected");
        html.replace("%WIFI_STATUS_TEXT%", "Conectado");
        
        String wifiDetails = "<div class='info-row'><span class='info-label'>Nome da Rede:</span><span class='info-value'>" + WiFi.SSID() + "</span></div>";
        wifiDetails += "<div class='info-row'><span class='info-label'>Endereço IP:</span><span class='info-value'>" + WiFi.localIP().toString() + "</span></div>";
        wifiDetails += "<div class='info-row'><span class='info-label'>Gateway:</span><span class='info-value'>" + WiFi.gatewayIP().toString() + "</span></div>";
        wifiDetails += "<div class='info-row'><span class='info-label'>DNS:</span><span class='info-value'>" + WiFi.dnsIP().toString() + "</span></div>";
        
        int32_t rssi = WiFi.RSSI();
        int signalPercent = constrain(map(rssi, -100, -30, 0, 100), 0, 100);
        wifiDetails += "<div class='info-row'><span class='info-label'>Potência do Sinal:</span><span class='info-value'>" + String(rssi) + " dBm (" + String(signalPercent) + "%) ";
        wifiDetails += "<div class='signal-bar'><div class='signal-indicator' style='width:" + String(100-signalPercent) + "%'></div></div></span></div>";
        
        html.replace("%WIFI_DETAILS%", wifiDetails);
    } else {
        html.replace("%WIFI_STATUS_CLASS%", "status-disconnected");
        html.replace("%WIFI_STATUS_TEXT%", "Desconectado");
        html.replace("%WIFI_DETAILS%", "<div class='info-row'><span class='info-label'>Rede Configurada:</span><span class='info-value'>" + String(deviceConfig.getWifiSSID()) + "</span></div>");
    }

    // AP Info
    html.replace("%AP_SSID%", String(deviceConfig.getDeviceName()));
    html.replace("%AP_IP%", WiFi.softAPIP().toString());
    html.replace("%AP_STATIONS%", String(WiFi.softAPgetStationNum()));

    // Sync Info
    if (sync.isConnected()) {
        html.replace("%SYNC_CONNECTION_CLASS%", "status-connected");
        html.replace("%SYNC_CONNECTION_TEXT%", "Conectado");
    } else {
        html.replace("%SYNC_CONNECTION_CLASS%", "status-disconnected");
        html.replace("%SYNC_CONNECTION_TEXT%", "Desconectado");
    }

    if (sync.isSyncing()) {
        html.replace("%SYNC_STATUS_CLASS%", "status-syncing");
        html.replace("%SYNC_STATUS_TEXT%", "Sincronizando");
    } else if (sync.isConnected()) {
        html.replace("%SYNC_STATUS_CLASS%", "status-disconnected");
        html.replace("%SYNC_STATUS_TEXT%", "Parado");
    } else {
        html.replace("%SYNC_STATUS_CLASS%", "status-disconnected");
        html.replace("%SYNC_STATUS_TEXT%", "Offline");
    }

    unsigned long lastSync = sync.getLastSuccessfulSync();
    if (lastSync > 0) {
        unsigned long timeSinceSync = (millis() - lastSync) / 1000;
        String timeText;
        if (timeSinceSync < 60) {
            timeText = String(timeSinceSync) + " segundos atrás";
        } else if (timeSinceSync < 3600) {
            timeText = String(timeSinceSync / 60) + " minutos atrás";
        } else if (timeSinceSync < 86400) {
            timeText = String(timeSinceSync / 3600) + " horas atrás";
        } else {
            timeText = String(timeSinceSync / 86400) + " dias atrás";
        }
        html.replace("%LAST_SYNC_CLASS%", "");
        html.replace("%LAST_SYNC_TEXT%", timeText);
    } else {
        html.replace("%LAST_SYNC_CLASS%", "status-disconnected");
        html.replace("%LAST_SYNC_TEXT%", "Nunca sincronizado");
    }

    return html;
  });
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