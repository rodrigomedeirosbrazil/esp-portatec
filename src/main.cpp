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

  pinMode(deviceConfig.getPulsePin(), OUTPUT);
  digitalWrite(deviceConfig.getPulsePin(), LOW);

  // Try to connect to WiFi if configured
  if (deviceConfig.isConfigured() && strlen(deviceConfig.getWifiSSID()) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());

  }

  waitForWifiConnection();

  // If not configured or couldn't connect, start AP mode
  if (!deviceConfig.isConfigured() || WiFi.status() != WL_CONNECTED) {
    setupAPMode();
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
  apModeStartTime = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
  myIP = WiFi.softAPIP();
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

  if (!sync.isSyncing()) {
    syncTimeoutCount++;
    if (syncTimeoutCount >= 3) { // 90 seconds without sync
      syncTimeoutCount = 0;
      setupAPMode();
      return;
    }
  } else {
    syncTimeoutCount = 0;
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    unsigned int wifiAttempts = 0;
    while (wifiAttempts < 3) {
      reconnectWifi();
      if (WiFi.status() == WL_CONNECTED) {
        break;
      }
      wifiAttempts++;
    }

    if (wifiAttempts >= 3) {
      setupAPMode();
      return;
    }
  }

  // Check internet connection
  unsigned int internetAttempts = 0;
  while (internetAttempts < 3) {
    if (hasInternetConnection()) {
      return;
    }
    internetAttempts++;
    delay(1000);
  }

  // If we get here, we couldn't establish internet connection
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
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
      delay(1000);
      timeout++;
  }
}

void reconnectWifi() {
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
    ESP.restart();
  }
}
