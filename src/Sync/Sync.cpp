#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "Sync.h"

Sync::Sync(DeviceConfig *deviceConfig) {
  this->deviceConfig = deviceConfig;
  lastSync = 0;
  lastSuccessfulSync = 0;
  deviceId = String(ESP.getChipId(), HEX);
}

void Sync::handle() {
  if (millis() - lastSync > 4000) {
    sync();
    lastSync = millis();
  }
}

void Sync::sync() {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  // Ignore SSL certificate validation
  client->setInsecure();

  //create an HTTPClient instance
  HTTPClient https;

  if (https.begin(*client, "https://portatec.medeirostec.com.br/api/sync/" + deviceId)) {
    https.addHeader("Content-Type", "application/json");

    String postData = "{\"millis\":" + String(millis()) + "}";

    int httpCode = https.POST(postData);

    if (httpCode != 200) {
      if (httpCode == 204) {
        lastSuccessfulSync = millis();
      }
      return;
    }

    uint8_t pin = deviceConfig->getPulsePin();
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);

    https.end();
  }
}