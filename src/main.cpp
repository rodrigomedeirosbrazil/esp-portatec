#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "DeviceConfig/DeviceConfig.h"

#include "main.h"

#define PIN_PULSE 3

IPAddress myIP;
DNSServer dnsServer;
ESP8266WebServer server(80);

DeviceConfig deviceConfig;

// Configuration page handler
void handleConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP-PORTATEC Configuration</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "input { padding: 10px; margin: 10px; width: 80%; max-width: 300px; }";
  html += "button { padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background-color: #45a049; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>ESP-PORTATEC Configuration</h1>";
  html += "<form action='/saveconfig' method='POST'>";
  html += "<input type='text' name='devicename' placeholder='Device Name' value='" + String(deviceConfig.getDeviceName()) + "' required><br>";
  html += "<input type='password' name='password' placeholder='WiFi Password' value='" + String(deviceConfig.getPassword()) + "' required><br>";
  html += "<button type='submit'>Save Configuration</button>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

// Save configuration handler
void handleSaveConfig() {
  if (server.hasArg("devicename") && server.hasArg("password")) {
    String deviceName = server.arg("devicename");
    String password = server.arg("password");

    if (deviceName.length() > 0 && password.length() > 0) {
      deviceConfig.setDeviceName(deviceName.c_str());
      deviceConfig.setPassword(password.c_str());
      deviceConfig.saveConfig();

      // Restart ESP to apply new configuration
      server.send(200, "text/html", "<script>setTimeout(function(){ window.location.href='/'; }, 3000);</script>Configuration saved! Restarting...");
      delay(3000);
      ESP.restart();
    }
  }
  server.send(400, "text/plain", "Invalid configuration");
}

void handleNotFound() {
  server.sendHeader("Location", "http://" + myIP.toString(), true);
  server.send(302, "text/plain", "");
}

void handlePulse() {
  digitalWrite(PIN_PULSE, HIGH);
  delay(500);
  digitalWrite(PIN_PULSE, LOW);
  server.send(200, "text/plain", "GPIO " + String(PIN_PULSE) + " toggled");
}

void handleRoot() {
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
  html += "<p>Dispositivo: " + String(deviceConfig.getDeviceName()) + "</p>";
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
  server.send(200, "text/html", html);
}

void setup() {
  delay(1000);

  // Start AP with configured or default values
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
  myIP = WiFi.softAPIP();

  pinMode(PIN_PULSE, OUTPUT);
  digitalWrite(PIN_PULSE, LOW);

  // Route configuration endpoints
  server.on("/config", handleConfig);
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);

  // Only show main page if configured
  if (deviceConfig.isConfigured()) {
    server.on("/", handleRoot);
  } else {
    server.on("/", handleConfig);
  }

  server.on("/pulse", handlePulse);
  server.onNotFound(handleNotFound);
  server.begin();
  dnsServer.start(53, "*", myIP);
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
}
