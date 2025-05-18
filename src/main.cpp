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

  // Only start AP, webserver and DNS if device is not configured
  if (!deviceConfig.isConfigured()) {
    WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
    myIP = WiFi.softAPIP();
    dnsServer.start(53, "*", myIP);
  }
}

void loop() {
  // Only handle webserver and DNS if device is not configured
  if (!deviceConfig.isConfigured()) {
    webserver.handleClient();
    dnsServer.processNextRequest();
  }
}
