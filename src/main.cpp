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

bool checkInternetConnection() {
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

void setup() {
  delay(1000);

  // Initialize pulse pin regardless of configuration
  pinMode(deviceConfig.getPulsePin(), OUTPUT);
  digitalWrite(deviceConfig.getPulsePin(), LOW);

  // Try to connect to WiFi if configured
  if (deviceConfig.isConfigured() && strlen(deviceConfig.getWifiSSID()) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());

    // Wait for connection with timeout
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
      delay(500);
      timeout++;
    }
  }

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
  } else {
    // Check internet connection periodically
    static unsigned long lastCheck = 0;
    static int failedAttempts = 0;

    if (millis() - lastCheck > 30000) { // Check every 30 seconds
      bool hasInternet = checkInternetConnection();

      if (!hasInternet) {
        failedAttempts++;

        if (failedAttempts < 3) {
          // Try to reconnect to WiFi
          WiFi.disconnect();
          WiFi.begin(deviceConfig.getWifiSSID(), deviceConfig.getWifiNetworkPass());

          // Wait for connection with timeout
          int timeout = 0;
          while (WiFi.status() != WL_CONNECTED && timeout < 20) {
            delay(500);
            timeout++;
          }
        } else {
          // After 3 failed attempts, switch to AP mode
          setupAPMode();
          failedAttempts = 0; // Reset counter
        }
      } else {
        // If internet is working, reset the failed attempts counter
        failedAttempts = 0;
      }

      lastCheck = millis();
    }
  }
}

void setupAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
  myIP = WiFi.softAPIP();
  dnsServer.start(53, "*", myIP);
}