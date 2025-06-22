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

  DEBUG_PRINT("Initializing Sync for device ID: ");
  DEBUG_PRINTLN(deviceId);
  DEBUG_PRINT("Channel name: ");
  DEBUG_PRINTLN(channelName);

  // Configure WebSocket
  connect();
  webSocket.onEvent([this](WStype_t type, uint8_t * payload, size_t length) {
    this->onWebSocketEvent(type, payload, length);
  });
  webSocket.setReconnectInterval(5000);

  DEBUG_PRINTLN("WebSocket configured with reconnect interval: 5000ms");
}

void Sync::handle() {
  webSocket.loop();

  // Try to connect if not connected
  if (!connected) {
    if (millis() - lastPing > 5000) {
      // Only try to connect if WiFi is connected and not in AP mode
      if (WiFi.status() == WL_CONNECTED) {
        connect();
      } else {
        DEBUG_PRINTLN("[WebSocket] Skipping connection attempt - WiFi not connected");
      }
      lastPing = millis();
    }
  }
}

void Sync::connect() {
  if (!connected) {
    // Additional safety check
    if (WiFi.status() != WL_CONNECTED) {
      DEBUG_PRINTLN("[WebSocket] Cannot connect - WiFi not connected");
      return;
    }

    DEBUG_PRINTLN("Attempting to connect to WebSocket...");
    // webSocket.begin("192.168.15.104", 8888, "/app/" + API_KEY + "?protocol=7&client=js&version=7.0.0&flash=false");
    webSocket.beginSSL(
      "portatec.medeirostec.com.br",
      443,
      String("/app/" + API_KEY + "?protocol=7&client=js&version=7.0.0&flash=false").c_str()
    );
  }
}

bool Sync::isConnected() {
  return connected;
}

bool Sync::isSyncing() {
  return connected && (millis() - lastSuccessfulSync < 120000);
}

unsigned long Sync::getLastSuccessfulSync() {
  return lastSuccessfulSync;
}

void Sync::onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      DEBUG_PRINTLN("[WebSocket] Disconnected");
      connected = false;
      subscribed = false;
      break;

    case WStype_CONNECTED:
      DEBUG_PRINT("[WebSocket] Connected to: ");
      DEBUG_PRINTLN((char*)payload);
      connected = false;
      subscribed = false;
      lastSuccessfulSync = millis();
      break;

    case WStype_TEXT:
      DEBUG_PRINT("[WebSocket] Received message: ");
      DEBUG_PRINTLN((char*)payload);
      handlePusherMessage(String((char*)payload));
      break;

    default:
      DEBUG_PRINT("[WebSocket] Unknown event type: ");
      DEBUG_PRINTLN(type);
      break;
  }
}

void Sync::subscribeToChannel() {
  if (!connected) return;

  DEBUG_PRINT("[Pusher] Subscribing to channel: ");
  DEBUG_PRINTLN(channelName);

  DynamicJsonDocument doc(512);
  doc["event"] = "pusher:subscribe";
  JsonObject data = doc.createNestedObject("data");
  data["channel"] = channelName;

  String message;
  serializeJson(doc, message);

  DEBUG_PRINT("[Pusher] Sending subscription message: ");
  DEBUG_PRINTLN(message);

  webSocket.sendTXT(message);
}

void Sync::handlePusherMessage(String message) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    DEBUG_PRINT("[Pusher] JSON deserialization failed: ");
    DEBUG_PRINTLN(error.c_str());
    return;
  }

  String event = doc["event"];
  DEBUG_PRINT("[Pusher] Handling event: ");
  DEBUG_PRINTLN(event);

  if (event == "pusher:ping") {
    DEBUG_PRINTLN("[Pusher] Received ping, sending pong");
    sendPong();
    sendDeviceStatus();
    lastSuccessfulSync = millis();
    return;
  }

  if (event == "pusher:connection_established") {
    DEBUG_PRINTLN("[Pusher] Connection established");
    connected = true;
    lastSuccessfulSync = millis();
    subscribeToChannel();
    return;
  }

  if (event == "pusher_internal:subscription_succeeded") {
    DEBUG_PRINTLN("[Pusher] Channel subscription successful");
    subscribed = true;
    lastSuccessfulSync = millis();
    sendDeviceStatus();
    return;
  }

  if (event == "pulse") {
    DEBUG_PRINTLN("[Pusher] Pulse command received, executing pulse");
    pulse();
    lastSuccessfulSync = millis();
    return;
  }

  if (event == "update-firmware") {
    DEBUG_PRINTLN("[Pusher] Firmware update command received");
    updateFirmware();
    lastSuccessfulSync = millis();
    return;
  }

  DEBUG_PRINT("[Pusher] Unknown event: ");
  DEBUG_PRINTLN(event);
}

void Sync::sendPong() {
  DEBUG_PRINTLN("[Pusher] Sending pong response");
  DynamicJsonDocument doc(256);
  doc["event"] = "pusher:pong";

  String message;
  serializeJson(doc, message);

  webSocket.sendTXT(message);
}

void Sync::sendDeviceStatus() {
  DEBUG_PRINT("[Pusher] Sending device status to channel: ");
  DEBUG_PRINTLN(channelName);

  DynamicJsonDocument doc(256);
  doc["event"] = "client-device-status";
  doc["channel"] = channelName;

  JsonObject data = doc.createNestedObject("data");
  data["chip-id"] = deviceId;
  data["millis"] = millis();
  data["wifi-strength"] = constrain(map(WiFi.RSSI(), -100, -30, 0, 100), 0, 100); // Convert RSSI to percentage (0-100%)
  data["firmware-version"] = DeviceConfig::FIRMWARE_VERSION;

  String message;
  serializeJson(doc, message);

  DEBUG_PRINT("[Pusher] Sending device status message: ");
  DEBUG_PRINTLN(message);

  webSocket.sendTXT(message);
}

void Sync::pulse() {
  uint8_t pin = deviceConfig->getPulsePin();
  DEBUG_PRINT("[Device] Executing pulse on pin: ");
  DEBUG_PRINTLN(pin);
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
  sendCommandAck("pulse");
  DEBUG_PRINTLN("[Device] Pulse completed");
}

void Sync::sendCommandAck(String commandName) {
  DEBUG_PRINT("[Pusher] Sending command ack for command: ");
  DEBUG_PRINTLN(commandName);

  DynamicJsonDocument doc(256);
  doc["event"] = "client-command-ack";
  doc["channel"] = channelName;

  JsonObject data = doc.createNestedObject("data");
  data["chip-id"] = deviceId;
  data["command"] = commandName;

  String message;
  serializeJson(doc, message);

  DEBUG_PRINT("[Pusher] Sending command ack message: ");
  DEBUG_PRINTLN(message);

  webSocket.sendTXT(message);
}

void Sync::updateFirmware() {
  DEBUG_PRINTLN("[Firmware] Starting firmware update...");
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  String url = "https://portatec.medeirostec.com.br/api/firmware/?chip-id=" + deviceId + "&version=" + DeviceConfig::FIRMWARE_VERSION;
  DEBUG_PRINT("[Firmware] Update URL: ");
  DEBUG_PRINTLN(url);

  t_httpUpdate_return ret = ESPhttpUpdate.update(*client, url);

  if (ret == HTTP_UPDATE_OK) {
    sendCommandAck("update-firmware");
    DEBUG_PRINTLN("[Firmware] Update successful, restarting...");
    ESP.restart();
  } else {
    DEBUG_PRINT("[Firmware] Update failed with error: ");
    DEBUG_PRINTLN(ret);
  }
}
