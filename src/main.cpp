#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "DeviceConfig/DeviceConfig.h"
#include "Webserver/Webserver.h"
#include "globals.h"

#include "main.h"

IPAddress myIP;
DNSServer dnsServer;
WiFiClient client;

DeviceConfig deviceConfig;
Webserver webserver(&deviceConfig);

unsigned long lastCheck = 0;

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
    return;
  }

  if (millis() - lastCheck > 30000) { // Check every 30 seconds
    handleConnection();
    lastCheck = millis();
  }
}

void setupAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
  myIP = WiFi.softAPIP();
  dnsServer.start(53, "*", myIP);
}

void handleConnection() {
  if (hasInternetConnection()) {
    return;
  }

  unsigned int failedAttempts = 0;
  while (failedAttempts < 3) {
    reconnectWifi();

    if (hasInternetConnection()) {
      return;
    }

    failedAttempts++;
  }

  // After 3 failed attempts, switch to AP mode
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