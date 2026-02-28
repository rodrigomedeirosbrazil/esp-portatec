#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "DeviceConfig/DeviceConfig.h"
#include "Webserver/Webserver.h"
#include "Sync/Sync.h"
#include "Sensor/Sensor.h"
#include "Clock/SystemClock.h" // Include SystemClock.h

#include "globals.h"

#include "main.h"

DNSServer dnsServer;
WiFiClient client;
IPAddress myIP;

DeviceConfig deviceConfig;
Sync sync;
Webserver webserver;
Sensor sensor;
SystemClock systemClock; // Instantiate global Clock instance
AccessManager accessManager;

unsigned long lastCheck = 0;
unsigned long lastSyncCheck = 0;
unsigned long apModeStartTime = 0;
unsigned int syncTimeoutCount = 0;
unsigned long lastSensorStatusSent = 0; // Controle para evitar envios muito frequentes

void setup() {
  delay(1000);

  // Initialize Serial only if DEBUG is enabled
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println();
  DEBUG_PRINTLN("=== ESP PORTATEC DEBUG MODE ===");
  DEBUG_PRINTLN("Device starting up...");
#endif

  pinMode(deviceConfig.getPulsePin(), OUTPUT);
  digitalWrite(deviceConfig.getPulsePin(), deviceConfig.getPulseInverted() ? HIGH : LOW);

  sensor.init();

  DEBUG_PRINTLN("Pulse and sensor pins configured");

  WiFi.mode(WIFI_AP_STA);
  setupAPMode();

  // Try to connect to WiFi if configured
  if (! deviceConfig.isConfigured()) {
    DEBUG_PRINTLN("Device not configured or no WiFi credentials");
    return;
  }

  DEBUG_PRINT("Device configured. Connecting to WiFi: ");
  DEBUG_PRINTLN(deviceConfig.getWifiSSID());

  WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());
  waitForWifiConnection();

  systemClock.setupNtp();
}

void loop() {
  webserver.handleClient();
  dnsServer.processNextRequest();

  handleConnection();

  sync.handle();
  systemClock.loop();
  accessManager.cleanup();

  if (sensor.hasChanged()) {
    if (sync.isConnected()) {
      if (millis() - lastSensorStatusSent >= 2000) {
        sync.sendSensorStatus(sensor.getValue());
        lastSensorStatusSent = millis();
        DEBUG_PRINTLN("[Main] Sensor status sent to server");
      } else {
        DEBUG_PRINTLN("[Main] Skipping sensor status send - too frequent");
      }
    }
  }
}

void setupAPMode() {
  DEBUG_PRINTLN("Setting up AP mode...");
  WiFi.softAP(deviceConfig.getDeviceName());
  myIP = WiFi.softAPIP();
  DEBUG_PRINT("AP mode started. SSID: ");
  DEBUG_PRINT(deviceConfig.getDeviceName());
  DEBUG_PRINT(", IP: ");
  DEBUG_PRINTLN(myIP);
  dnsServer.start(53, "*", myIP);
}

void handleConnection() {
  if (! deviceConfig.isConfigured()) {
    return;
  }

  if (millis() - lastCheck < 30000) {
    return;
  }

  lastCheck = millis();
  DEBUG_PRINTLN("Checking connection status...");

  if (sync.isSyncing()) {
    return;
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi disconnected. Attempting to reconnect...");
    reconnectWifi();
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTLN("WiFi reconnected successfully");
      return;
    }

    DEBUG_PRINTLN("WiFi cannot reconnect.");
    return;
  }
}

void waitForWifiConnection() {
    DEBUG_PRINT("Waiting for WiFi connection");
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
      delay(1000);
      timeout++;
      DEBUG_PRINT(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINT("WiFi connected successfully. IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    return;
  }

  DEBUG_PRINTLN(" Timeout!");
  return;
}

void reconnectWifi() {
  DEBUG_PRINTLN("Reconnecting to WiFi...");
  WiFi.disconnect();
  WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());
  waitForWifiConnection();
}
