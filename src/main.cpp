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
Sync sync(&deviceConfig);
Webserver webserver(&deviceConfig, &sync);

unsigned long lastCheck = 0;
unsigned long lastSyncCheck = 0;
unsigned long apModeStartTime = 0;
unsigned int syncTimeoutCount = 0;


int lastSensorValue = -1;
unsigned long lastSensorCheck = 0;
const unsigned long SENSOR_CHECK_INTERVAL = 100;

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
  digitalWrite(deviceConfig.getPulsePin(), LOW);

  pinMode(deviceConfig.getSensorPin(), INPUT);

  DEBUG_PRINTLN("Pulse and sensor pins configured");

  // Inicializar sistema de eventos do sensor
  initSensorEvents();

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
}

void loop() {
  webserver.handleClient();
  dnsServer.processNextRequest();

  handleConnection();

  sync.handle();

  checkSensorEvents();
}

void setupAPMode() {
  DEBUG_PRINTLN("Setting up AP mode...");
  WiFi.softAP(deviceConfig.getDeviceName(), deviceConfig.getPassword());
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

void initSensorEvents() {
  DEBUG_PRINTLN("Initializing sensor event system...");
  lastSensorValue = digitalRead(deviceConfig.getSensorPin());
  lastSensorCheck = millis();
  DEBUG_PRINT("Initial sensor value: ");
  DEBUG_PRINTLN(lastSensorValue);
}

void checkSensorEvents() {
  if (millis() - lastSensorCheck < SENSOR_CHECK_INTERVAL) {
    return;
  }

  lastSensorCheck = millis();

  int currentSensorValue = digitalRead(deviceConfig.getSensorPin());

  if (currentSensorValue != lastSensorValue) {
    DEBUG_PRINT("Sensor change detected! Previous: ");
    DEBUG_PRINT(lastSensorValue);
    DEBUG_PRINT(", Current: ");
    DEBUG_PRINTLN(currentSensorValue);

    onSensorChange(currentSensorValue, lastSensorValue);

    lastSensorValue = currentSensorValue;
  }
}

void onSensorChange(int currentValue, int previousValue) {
  DEBUG_PRINTLN("=== SENSOR EVENT TRIGGERED ===");
  DEBUG_PRINT("Sensor changed from ");
  DEBUG_PRINT(previousValue);
  DEBUG_PRINT(" to ");
  DEBUG_PRINTLN(currentValue);

  if (currentValue == HIGH && previousValue == LOW) {
    DEBUG_PRINTLN("Sensor activated (LOW -> HIGH)");
  } else if (currentValue == LOW && previousValue == HIGH) {
    DEBUG_PRINTLN("Sensor deactivated (HIGH -> LOW)");
  }

  DEBUG_PRINTLN("=== END SENSOR EVENT ===");
}

