#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "DeviceConfig/DeviceConfig.h"
#include "Webserver/Webserver.h"
#include "Sync/Sync.h"

#include "globals.h"

#include "main.h"

DNSServer dnsServer;
WiFiClient client;

DeviceConfig deviceConfig;
Webserver webserver(&deviceConfig);
Sync sync(&deviceConfig);

unsigned long lastCheck = 0;
unsigned long lastSyncCheck = 0;
unsigned long apModeStartTime = 0;
unsigned int syncTimeoutCount = 0;

void setup() {
  delay(1000);

  // Initialize Serial only if DEBUG is enabled
  if (DEBUG) {
    Serial.begin(9600);
    Serial.println();
    DEBUG_PRINTLN("=== ESP PORTATEC DEBUG MODE ===");
    DEBUG_PRINTLN("Device starting up...");
  }

  pinMode(deviceConfig.getPulsePin(), OUTPUT);
  digitalWrite(deviceConfig.getPulsePin(), LOW);

  DEBUG_PRINTLN("Pulse pin configured");

  // Try to connect to WiFi if configured
  if (deviceConfig.isConfigured() && strlen(deviceConfig.getWifiSSID()) > 0) {
    DEBUG_PRINT("Device configured. Connecting to WiFi: ");
    DEBUG_PRINTLN(deviceConfig.getWifiSSID());

    WiFi.mode(WIFI_STA);
    WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());
  } else {
    DEBUG_PRINTLN("Device not configured or no WiFi credentials");
  }

  waitForWifiConnection();

  // If not configured or couldn't connect, start AP mode
  if (!deviceConfig.isConfigured() || WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("Starting AP mode");
    setupAPMode();
  } else {
    DEBUG_PRINT("WiFi connected successfully. IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
  }
}

void loop() {
  // Only handle webserver and DNS if device is not configured or in AP mode
  if (!deviceConfig.isConfigured() || WiFi.getMode() == WIFI_AP) {
    webserver.handleClient();
    dnsServer.processNextRequest();
  }

  handleConnection();
  handleApMode();

  sync.handle();
}

void setupAPMode() {
  DEBUG_PRINTLN("Setting up AP mode...");
  apModeStartTime = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
  myIP = WiFi.softAPIP();
  DEBUG_PRINT("AP mode started. SSID: ");
  DEBUG_PRINT(deviceConfig.getDeviceName());
  DEBUG_PRINT(", IP: ");
  DEBUG_PRINTLN(myIP);
  dnsServer.start(53, "*", myIP);
}

void handleConnection() {
  if (WiFi.getMode() == WIFI_AP) {
    return;
  }

  if (millis() - lastCheck < 30000) {
    return;
  }

  lastCheck = millis();
  DEBUG_PRINTLN("Checking connection status...");

  if (!sync.isSyncing()) {
    syncTimeoutCount++;
    DEBUG_PRINT("Sync timeout count: ");
    DEBUG_PRINTLN(syncTimeoutCount);
    if (syncTimeoutCount >= 3) { // 90 seconds without sync
      DEBUG_PRINTLN("Sync timeout reached. Switching to AP mode");
      syncTimeoutCount = 0;
      setupAPMode();
      return;
    }
  } else {
    if (syncTimeoutCount > 0) {
      DEBUG_PRINTLN("Sync restored. Resetting timeout count");
    }
    syncTimeoutCount = 0;
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi disconnected. Attempting to reconnect...");
    unsigned int wifiAttempts = 0;
    while (wifiAttempts < 3) {
      reconnectWifi();
      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi reconnected successfully");
        break;
      }
      wifiAttempts++;
      DEBUG_PRINT("WiFi reconnect attempt ");
      DEBUG_PRINT(wifiAttempts);
      DEBUG_PRINTLN(" failed");
    }

    if (wifiAttempts >= 3) {
      DEBUG_PRINTLN("All WiFi reconnect attempts failed. Switching to AP mode");
      setupAPMode();
      return;
    }
  }

  // Check internet connection
  DEBUG_PRINTLN("Checking internet connection...");
  unsigned int internetAttempts = 0;
  while (internetAttempts < 3) {
    if (hasInternetConnection()) {
      DEBUG_PRINTLN("Internet connection verified");
      return;
    }
    internetAttempts++;
    DEBUG_PRINT("Internet check attempt ");
    DEBUG_PRINT(internetAttempts);
    DEBUG_PRINTLN(" failed");
    delay(1000);
  }

  // If we get here, we couldn't establish internet connection
  DEBUG_PRINTLN("No internet connection available. Switching to AP mode");
  setupAPMode();
}

bool hasInternetConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // Try to connect to a reliable server (Google DNS)
  if (client.connect("8.8.8.8", 53)) {
    client.stop();
    return true;
  }
  return false;
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
    DEBUG_PRINTLN(" Connected!");
  } else {
    DEBUG_PRINTLN(" Timeout!");
  }
}

void reconnectWifi() {
  DEBUG_PRINTLN("Reconnecting to WiFi...");
  WiFi.disconnect();
  WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());
  waitForWifiConnection();
}

void handleApMode() {
  if (WiFi.getMode() != WIFI_AP) {
    return;
  }

  if (
    deviceConfig.isConfigured()
    && millis() - apModeStartTime > 300000 // 300000 = 5 minutes
  ) {
    DEBUG_PRINTLN("AP mode timeout reached (5 minutes). Restarting device...");
    ESP.restart();
  }
}
