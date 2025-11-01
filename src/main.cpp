#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include "DeviceConfig/DeviceConfig.h"
#include "Webserver/Webserver.h"
#include "Sync/Sync.h"
#include "Sensor/Sensor.h"

#include "globals.h"

#include "main.h"

#include "PinManager/PinManager.h"
#include <LittleFS.h>

DNSServer dnsServer;
WiFiClient client;

DeviceConfig deviceConfig;
Sync sync;
Webserver webserver;
Sensor sensor;
PinManager pinManager;

unsigned long lastCheck = 0;
unsigned long lastSyncCheck = 0;
unsigned long apModeStartTime = 0;
unsigned int syncTimeoutCount = 0;
unsigned long lastSensorStatusSent = 0; // Controle para evitar envios muito frequentes

const char* AP_SSID = "Portatec-Setup";

void setup() {
  delay(1000);

  // Initialize Serial only if DEBUG is enabled
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println();
  DEBUG_PRINTLN("=== ESP PORTATEC DEBUG MODE ===");
  DEBUG_PRINTLN("Device starting up...");
#endif

  if (!LittleFS.begin()) {
    DEBUG_PRINTLN("An Error has occurred while mounting LittleFS");
    return;
  }

  pinManager.begin();

  pinMode(deviceConfig.getPulsePin(), OUTPUT);
  digitalWrite(deviceConfig.getPulsePin(), deviceConfig.getPulseInverted() ? HIGH : LOW);

  sensor.init();

  DEBUG_PRINTLN("Pulse and sensor pins configured");

  WiFi.mode(WIFI_AP);
  setupAPMode();
}

void loop() {
  webserver.handleClient();
  dnsServer.processNextRequest();

  sync.handle();

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
  WiFi.softAP(AP_SSID, NULL);
  myIP = WiFi.softAPIP();
  DEBUG_PRINT("AP mode started. SSID: ");
  DEBUG_PRINT(AP_SSID);
  DEBUG_PRINT(", IP: ");
  DEBUG_PRINTLN(myIP);
  dnsServer.start(53, "*", myIP);
}
