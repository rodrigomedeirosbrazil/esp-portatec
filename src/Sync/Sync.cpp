#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

#include "Sync.h"
#include "../globals.h"

Sync::Sync(DeviceConfig *deviceConfig) {
  this->deviceConfig = deviceConfig;
  lastPing = 0;
  lastSuccessfulSync = 0;
  connected = false;
  subscribed = false;
  deviceId = String(ESP.getChipId(), HEX);
  channelName = "device-sync." + deviceId;

  // Configure WebSocket
  webSocket.begin("localhost", 8888, "/app/" + API_KEY + "?protocol=7&client=js&version=7.0.0&flash=false");
  webSocket.onEvent([this](WStype_t type, uint8_t * payload, size_t length) {
    this->onWebSocketEvent(type, payload, length);
  });
  webSocket.setReconnectInterval(5000);
}

void Sync::handle() {
  webSocket.loop();

  // Try to connect if not connected
  if (!connected) {
    if (millis() - lastPing > 5000) {
      connect();
      lastPing = millis();
    }
  }

  // Subscribe to channel if connected but not subscribed
  if (connected && !subscribed) {
    subscribeToChannel();
  }
}

void Sync::connect() {
  if (!connected) {
    webSocket.begin("localhost", 8888, "/app/" + API_KEY + "?protocol=7&client=js&version=7.0.0&flash=false");
  }
}

bool Sync::isConnected() {
  return connected;
}

bool Sync::isSyncing() {
  return connected && (millis() - lastSuccessfulSync < 30000);
}

void Sync::onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      connected = false;
      subscribed = false;
      break;

    case WStype_CONNECTED:
      connected = true;
      subscribed = false;
      lastSuccessfulSync = millis();
      break;

    case WStype_TEXT:
      handlePusherMessage(String((char*)payload));
      break;

    default:
      break;
  }
}

void Sync::subscribeToChannel() {
  if (!connected) return;

  DynamicJsonDocument doc(512);
  doc["event"] = "pusher:subscribe";
  JsonObject data = doc.createNestedObject("data");
  data["channel"] = channelName;

  String message;
  serializeJson(doc, message);

  webSocket.sendTXT(message);
  subscribed = true;
}

void Sync::handlePusherMessage(String message) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    return;
  }

  String event = doc["event"];

  if (event == "pusher:ping") {
    sendPong();
    lastSuccessfulSync = millis();
    return;
  }

  if (event == "pusher:connection_established") {
    lastSuccessfulSync = millis();
    return;
  }

  if (event == "pusher_internal:subscription_succeeded") {
    subscribed = true;
    lastSuccessfulSync = millis();
    return;
  }

  if (event == "pulse") {
    pulse();
    lastSuccessfulSync = millis();
    return;
  }

  if (event == "update-firmware") {
    updateFirmware();
    lastSuccessfulSync = millis();
    return;
  }
}

void Sync::sendPong() {
  DynamicJsonDocument doc(256);
  doc["event"] = "pusher:pong";
  doc["data"] = "{}";

  String message;
  serializeJson(doc, message);

  webSocket.sendTXT(message);
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
