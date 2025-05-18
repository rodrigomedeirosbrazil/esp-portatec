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

DeviceConfig deviceConfig;
Webserver webserver(&deviceConfig);

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
    WiFi.mode(WIFI_AP);
    WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
    myIP = WiFi.softAPIP();
    dnsServer.start(53, "*", myIP);
  }
}

void loop() {
  // Only handle webserver and DNS if device is not configured or in AP mode
  if (!deviceConfig.isConfigured() || WiFi.getMode() == WIFI_AP) {
    webserver.handleClient();
    dnsServer.processNextRequest();
  }
}
