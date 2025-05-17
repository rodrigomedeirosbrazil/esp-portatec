#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <DNSServer.h>

#include "main.h"

// EEPROM configuration structure
struct Config {
  uint32_t signature;
  uint8_t version;
  char deviceName[32];
  char wifiPassword[32];
};

Config config;

// Constantes
const int CONFIG_ADDRESS = 0;
const uint32_t CONFIG_SIGNATURE = 0x504F5254;
const uint8_t CONFIG_VERSION = 1;

// Default values
const char *defaultDeviceName = "ESP-PORTATEC";
const char *defaultPassword = "123456789";

bool configured = false;

IPAddress myIP;
DNSServer dnsServer;
ESP8266WebServer server(80);

// Function to initialize default configuration
void initDefaultConfig() {
  config.signature = CONFIG_SIGNATURE;
  config.version = CONFIG_VERSION;
  strcpy(config.deviceName, defaultDeviceName);
  strcpy(config.wifiPassword, defaultPassword);
}

// Function to load configuration from EEPROM
void loadConfig() {
  EEPROM.get(CONFIG_ADDRESS, config);

  // Verifica se a assinatura e versão são válidas
  if (config.signature != CONFIG_SIGNATURE || config.version != CONFIG_VERSION) {
    initDefaultConfig();
    saveConfig();
    configured = false;
    return;
  }

  configured = true;
}

// Function to save configuration to EEPROM
void saveConfig() {
  config.signature = CONFIG_SIGNATURE;
  config.version = CONFIG_VERSION;
  EEPROM.put(CONFIG_ADDRESS, config);
  EEPROM.commit();
}

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
  html += "<input type='text' name='devicename' placeholder='Device Name' value='" + String(config.deviceName) + "' required><br>";
  html += "<input type='password' name='password' placeholder='WiFi Password' value='" + String(config.wifiPassword) + "' required><br>";
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
      deviceName.toCharArray(config.deviceName, 32);
      password.toCharArray(config.wifiPassword, 32);
      saveConfig();

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

void handleGPIO() {
  if (!server.hasArg("gpio")) {
    server.send(400, "text/plain", "GPIO parameter required");
    return;
  }

  int gpio = server.arg("gpio").toInt();
  if (gpio < 0 || gpio > 3) {
    server.send(400, "text/plain", "Invalid GPIO number");
    return;
  }

  digitalWrite(gpio, HIGH);
  delay(1000);
  digitalWrite(gpio, LOW);
  server.send(200, "text/plain", "GPIO" + String(gpio) + " toggled");
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
  html += ".processing { background-color: #ff9800 !important; }";
  html += ".button-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin: 20px 0; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>ESP-PORTATEC Control</h1>";
  html += "<p>Dispositivo: " + String(config.deviceName) + "</p>";
  html += "<div class='button-container'>";
  html += "<button id='gpio0Button' onclick='toggleGPIO(0)'>Toggle GPIO0</button>";
  html += "<button id='gpio1Button' onclick='toggleGPIO(1)'>Toggle GPIO1</button>";
  html += "<button id='gpio2Button' onclick='toggleGPIO(2)'>Toggle GPIO2</button>";
  html += "<button id='gpio3Button' onclick='toggleGPIO(3)'>Toggle GPIO3</button>";
  html += "</div>";
  html += "<script>";
  html += "function toggleGPIO(gpio) {";
  html += "  const button = document.getElementById('gpio' + gpio + 'Button');";
  html += "  button.disabled = true;";
  html += "  button.classList.add('processing');";
  html += "  button.textContent = 'Processando...';";
  html += "  fetch('/gpio?gpio=' + gpio)";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      console.log(data);";
  html += "      setTimeout(() => {";
  html += "        button.disabled = false;";
  html += "        button.classList.remove('processing');";
  html += "        button.textContent = 'Toggle GPIO' + gpio;";
  html += "      }, 1000);";
  html += "    });";
  html += "}";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  delay(1000);

  // Initialize EEPROM
  EEPROM.begin(512);
  loadConfig();

  // Start AP with configured or default values
  WiFi.softAP(config.deviceName, config.wifiPassword);
  myIP = WiFi.softAPIP();

  // Configure all GPIOs as outputs
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  // Initialize all GPIOs to LOW
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);

  // Route configuration endpoints
  server.on("/config", handleConfig);
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);

  // Only show main page if configured
  if (configured) {
    server.on("/", handleRoot);
  } else {
    server.on("/", handleConfig);
  }

  server.on("/gpio", handleGPIO);
  server.onNotFound(handleNotFound);
  server.begin();
  dnsServer.start(53, "*", myIP);
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
}
