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

  // Start AP with configured or default values
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
  myIP = WiFi.softAPIP();

  pinMode(deviceConfig.getPulsePin(), OUTPUT);
  digitalWrite(deviceConfig.getPulsePin(), LOW);

  dnsServer.start(53, "*", myIP);
}

void loop() {
  webserver.handleClient();
  dnsServer.processNextRequest();
}
