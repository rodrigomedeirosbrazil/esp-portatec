#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266httpUpdate.h>

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

    String postData = "{\"millis\":" + String(millis()) + ", \"firmware_version\":\"" + String(DeviceConfig::FIRMWARE_VERSION) + "\"}";

    int httpCode = https.POST(postData);

    if (httpCode != 200) {
      if (httpCode == 204) {
        lastSuccessfulSync = millis();
      }
      https.end();
      return;
    }

    lastSuccessfulSync = millis();

    String payload = https.getString();

    if (payload.equals("pulse")) {
      https.end();
      pulse();
      return;
    }

    if (payload.equals("update-firmware")) {
      https.end();
      updateFirmware();
      return;
    }

    https.end();
  }
}

bool Sync::isSyncing() {
  return millis() - lastSuccessfulSync < 10000;
}

void Sync::pulse() {
  uint8_t pin = deviceConfig->getPulsePin();
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}

void Sync::updateFirmware() {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  String url = "https://portatec.medeirostec.com.br/api/firmware/?deviceId=" + deviceId + "&version=" + DeviceConfig::FIRMWARE_VERSION;
  t_httpUpdate_return ret = ESPhttpUpdate.update(*client, url);

  if (ret == HTTP_UPDATE_OK) {
    ESP.restart();
  }
}
