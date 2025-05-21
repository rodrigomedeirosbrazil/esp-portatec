#include "Arduino.h"

#include "Webserver.h"

// Initialize static instance pointer
Webserver* Webserver::instance = nullptr;

Webserver::Webserver(DeviceConfig *deviceConfig): server(80) {
  this->deviceConfig = deviceConfig;
  instance = this;  // Set the instance pointer

  server.on("/config", handleConfig);
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);

  if (deviceConfig->isConfigured()) {
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
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
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
  html += "<input type='text' name='devicename' placeholder='Device Name' value='" + String(instance->deviceConfig->getDeviceName()) + "' required><br>";
  html += "<input type='password' name='password' placeholder='WiFi Password' value='" + String(instance->deviceConfig->getPassword()) + "' required><br>";
  html += "<input type='number' name='pulsepin' placeholder='Pulse Pin' value='" + String(instance->deviceConfig->getPulsePin()) + "' required><br>";
  html += "</div>";

  // WiFi Network Configuration Section
  html += "<div class='section'>";
  html += "<h2>WiFi Network Configuration</h2>";
  html += "<input type='text' name='wifissid' placeholder='WiFi Network Name (SSID)' value='" + String(instance->deviceConfig->getWifiSSID()) + "'><br>";
  html += "<input type='password' name='wifipass' placeholder='WiFi Network Password' value='" + String(instance->deviceConfig->getWifiNetworkPass()) + "'><br>";
  html += "</div>";

  html += "<button type='submit'>Save Configuration</button>";
  html += "</form></body></html>";
  instance->server.send(200, "text/html", html);
}

void Webserver::handleSaveConfig() {
  if (instance->server.hasArg("devicename") && instance->server.hasArg("password") && instance->server.hasArg("pulsepin")) {
    String deviceName = instance->server.arg("devicename");
    String password = instance->server.arg("password");
    String pulsePinStr = instance->server.arg("pulsepin");
    String wifiSSID = instance->server.arg("wifissid");
    String wifiPass = instance->server.arg("wifipass");

    if (deviceName.length() > 0 && password.length() > 0 && pulsePinStr.length() > 0) {
      instance->deviceConfig->setDeviceName(deviceName.c_str());
      instance->deviceConfig->setPassword(password.c_str());
      instance->deviceConfig->setPulsePin(pulsePinStr.toInt());

      // Set WiFi network configuration if provided
      if (wifiSSID.length() > 0) {
        instance->deviceConfig->setWifiSSID(wifiSSID.c_str());
        instance->deviceConfig->setWifiNetworkPass(wifiPass.c_str());
      }

      instance->deviceConfig->saveConfig();

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
  uint8_t pin = instance->deviceConfig->getPulsePin();
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
  instance->server.send(200, "text/plain", "GPIO " + String(pin) + " toggled");
}

void Webserver::handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP-PORTATEC Control</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "button { padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }";
  html += "button:hover { background-color: #45a049; }";
  html += "button:disabled { background-color: #cccccc; cursor: not-allowed; }";
  html += ".working { background-color: #ff9800 !important; }";
  html += ".button-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin: 20px 0; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>ESP-PORTATEC Control</h1>";
  html += "<p>Dispositivo: " + String(instance->deviceConfig->getDeviceName()) + "</p>";
  html += "<p>Chip ID: " + String(ESP.getChipId(), HEX) + "</p>";
  html += "<div class='button-container'>";
  html += "<button id='pulseButton' onclick='pulseGpio()'>Abrir</button>";
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

void Webserver::handleClient() {
  server.handleClient();
}

