#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

#include "Sync.h"
#include "../globals.h"
#include "../AccessManager/AccessManager.h"

static Sync* s_syncInstance = nullptr;

static void mqttCallbackStatic(char* topic, byte* payload, unsigned int length) {
  if (s_syncInstance) {
    s_syncInstance->mqttCallback(topic, payload, length);
  }
}

Sync::Sync() : mqttClient(wifiClient) {
  lastReconnectAttempt = 0;
  lastSuccessfulSync = 0;
  lastHeartbeat = 0;
  connected = false;
  deviceId = String(ESP.getChipId(), HEX);
  clientId = "esp-" + deviceId;
  topicCommand = "device/" + deviceId + "/command";
  topicAccessCodesSync = "device/" + deviceId + "/access-codes/sync";
  topicAck = "device/" + deviceId + "/ack";
  topicStatus = "device/" + deviceId + "/status";
  topicEvent = "device/" + deviceId + "/event";
  topicAccessCodesAck = "device/" + deviceId + "/access-codes/ack";

  s_syncInstance = this;
  mqttClient.setCallback(mqttCallbackStatic);
  mqttClient.setBufferSize(512);

  DEBUG_PRINT("Initializing Sync for device ID: ");
  DEBUG_PRINTLN(deviceId);
  DEBUG_PRINTLN("MQTT topics configured");
}

void Sync::handle() {
  if (!mqttClient.connected()) {
    connected = false;
    if (WiFi.status() == WL_CONNECTED && strlen(deviceConfig.getMqttHost()) > 0) {
      if (millis() - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = millis();
        reconnect();
      }
    }
  } else {
    mqttClient.loop();
    connected = true;

    // Heartbeat: send status periodically (every 60s)
    if (millis() - lastHeartbeat > 60000) {
      sendDeviceStatus();
      lastHeartbeat = millis();
      lastSuccessfulSync = millis();
    }

    // Connection timeout (5 min)
    if (millis() - lastSuccessfulSync > 300000) {
      DEBUG_PRINTLN("[MQTT] Connection timeout, will reconnect...");
      mqttClient.disconnect();
      connected = false;
    }
  }
}

void Sync::connect() {
  if (strlen(deviceConfig.getMqttHost()) == 0) {
    DEBUG_PRINTLN("[MQTT] No MQTT host configured, skipping connection");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("[MQTT] Cannot connect - WiFi not connected");
    return;
  }
  reconnect();
}

bool Sync::reconnect() {
  mqttClient.setServer(deviceConfig.getMqttHost(), deviceConfig.getMqttPort());
  if (strlen(deviceConfig.getMqttUser()) > 0) {
    mqttClient.connect(clientId.c_str(), deviceConfig.getMqttUser(), deviceConfig.getMqttPassword());
  } else {
    mqttClient.connect(clientId.c_str());
  }

  if (mqttClient.connected()) {
    DEBUG_PRINTLN("[MQTT] Connected to broker");
    subscribeToTopics();
    sendDeviceStatus();
    lastSuccessfulSync = millis();
    lastHeartbeat = millis();
    return true;
  }
  DEBUG_PRINT("[MQTT] Connection failed, rc=");
  DEBUG_PRINTLN(mqttClient.state());
  return false;
}

void Sync::subscribeToTopics() {
  mqttClient.subscribe(topicCommand.c_str());
  mqttClient.subscribe(topicAccessCodesSync.c_str());
  DEBUG_PRINTLN("[MQTT] Subscribed to command and access-codes/sync topics");
}

void Sync::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length >= 512) return;
  char buffer[512];
  memcpy(buffer, payload, length);
  buffer[length] = '\0';

  DEBUG_PRINT("[MQTT] Message on ");
  DEBUG_PRINT(topic);
  DEBUG_PRINT(": ");
  DEBUG_PRINTLN((char*)buffer);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, buffer);
  if (error) {
    DEBUG_PRINTLN("[MQTT] JSON parse error");
    return;
  }

  JsonObject data = doc.as<JsonObject>();
  String topicStr = String(topic);

  if (topicStr == topicCommand) {
    handleCommand(data);
  } else if (topicStr == topicAccessCodesSync) {
    handleAccessCodesSync(data);
  }
}

void Sync::handleCommand(JsonObject data) {
  const char* action = data["action"].as<const char*>();
  if (!action) return;

  const char* cmdId = data["command_id"].as<const char*>();
  String commandId = (cmdId && strlen(cmdId) > 0) ? String(cmdId) : "local";

  lastSuccessfulSync = millis();

  if (strcmp(action, "pulse") == 0 || strcmp(action, "toggle") == 0 || strcmp(action, "push_button") == 0) {
    executeRelay(action, commandId.c_str());
  } else if (strcmp(action, "update_firmware") == 0) {
    updateFirmware(commandId.c_str());
  } else {
    sendCommandAck(String(action), 255, commandId.c_str());
  }
}

void Sync::handleAccessCodesSync(JsonObject data) {
  const char* action = data["action"].as<const char*>();
  if (!action || strcmp(action, "sync_access_codes") != 0) return;

  lastSuccessfulSync = millis();

  JsonArray accessCodes = data["access_codes"].as<JsonArray>();
  if (!accessCodes.isNull()) {
    accessManager.syncFromBackend(accessCodes);
  }

  // Send ACK
  DynamicJsonDocument ackDoc(128);
  ackDoc["command_id"] = data["command_id"];
  ackDoc["action"] = "sync_access_codes";
  String ackMsg;
  serializeJson(ackDoc, ackMsg);
  mqttClient.publish(topicAccessCodesAck.c_str(), ackMsg.c_str());
}

void Sync::executeRelay(const char* action, const char* commandId) {
  uint8_t pin = deviceConfig.getPulsePin();
  bool inverted = deviceConfig.getPulseInverted();

  DEBUG_PRINT("[Device] Executing relay on pin: ");
  DEBUG_PRINTLN(pin);
  digitalWrite(pin, inverted ? LOW : HIGH);
  delay(500);
  digitalWrite(pin, inverted ? HIGH : LOW);
  sendCommandAck(String(action), pin, commandId);
  DEBUG_PRINTLN("[Device] Relay pulse completed");
}

void Sync::sendCommandAck(String action, uint8_t gpio, const char* commandId) {
  DynamicJsonDocument doc(256);
  doc["action"] = action;
  if (gpio != 255) {
    doc["pin"] = gpio;
  }
  doc["command_id"] = commandId ? commandId : "local";

  String message;
  serializeJson(doc, message);
  mqttClient.publish(topicAck.c_str(), message.c_str());
}

void Sync::sendDeviceStatus() {
  DynamicJsonDocument doc(512);
  doc["chip-id"] = deviceId;
  doc["millis"] = millis();
  doc["wifi-strength"] = constrain(map(WiFi.RSSI(), -100, -30, 0, 100), 0, 100);
  doc["firmware-version"] = DeviceConfig::FIRMWARE_VERSION;
  doc["device-name"] = deviceConfig.getDeviceName();
  doc["wifi-ssid"] = deviceConfig.getWifiSSID();
  doc["pulse-pin"] = deviceConfig.getPulsePin();
  doc["sensor-pin"] = deviceConfig.getSensorPin();
  doc["pulse-inverted"] = deviceConfig.getPulseInverted();
  if (deviceConfig.getSensorPin() != DeviceConfig::UNCONFIGURED_PIN) {
    doc["sensor_value"] = sensor.getValue();
  }

  String message;
  serializeJson(doc, message);
  mqttClient.publish(topicStatus.c_str(), message.c_str());
}

void Sync::sendSensorStatus(int value) {
  DynamicJsonDocument doc(256);
  doc["chip-id"] = deviceId;
  doc["sensor_pin"] = deviceConfig.getSensorPin();
  doc["sensor_value"] = value;

  String message;
  serializeJson(doc, message);
  mqttClient.publish(topicStatus.c_str(), message.c_str());
}

void Sync::sendPinUsage(int pinId) {
  DEBUG_PRINT("[MQTT] sendPinUsage deprecated, use sendAccessEvent. pinId: ");
  DEBUG_PRINTLN(pinId);
  // Legacy: will be replaced by sendAccessEvent in Webserver
  DynamicJsonDocument doc(128);
  doc["pin_id"] = pinId;
  doc["timestamp_device"] = systemClock.getUnixTime();
  String message;
  serializeJson(doc, message);
  mqttClient.publish(topicEvent.c_str(), message.c_str());
}

void Sync::sendAccessEvent(const char* code, const char* result, unsigned long timestamp) {
  DynamicJsonDocument doc(256);
  doc["pin"] = code;
  doc["result"] = result;
  doc["timestamp_device"] = timestamp;

  String message;
  serializeJson(doc, message);
  mqttClient.publish(topicEvent.c_str(), message.c_str());
}

void Sync::updateFirmware(const char* commandId) {
  const char* ackCmdId = (commandId && strlen(commandId) > 0) ? commandId : "local";

  DEBUG_PRINTLN("[Firmware] Starting firmware update...");
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  client->setTimeout(30000);
  client->setBufferSizes(1024, 1024);

  uint32_t optimizedFreeHeap = optimizeMemoryForOTA();
  if (optimizedFreeHeap < 25000) {
    DEBUG_PRINTLN("[Firmware] ERROR: Not enough free heap for OTA update");
    sendCommandAck("update-firmware-error", 255, ackCmdId);
    return;
  }

  String url = "https://portatec.medeirostec.com.br/api/firmware/?chip-id=" + deviceId + "&version=" + DeviceConfig::FIRMWARE_VERSION;
  DEBUG_PRINT("[Firmware] Update URL: ");
  DEBUG_PRINTLN(url);

  ESPhttpUpdate.setLedPin(-1);
  ESPhttpUpdate.rebootOnUpdate(false);
  ESPhttpUpdate.onStart([]() { ESP.wdtFeed(); });
  ESPhttpUpdate.onProgress([](int cur, int total) {
    if (cur % 8192 == 0) { ESP.wdtFeed(); yield(); }
  });

  mqttClient.disconnect();
  delay(200);

  t_httpUpdate_return ret = ESPhttpUpdate.update(*client, url);

  // Reconnect to send ACK before restart
  if (reconnect()) {
    if (ret == HTTP_UPDATE_OK) {
      sendCommandAck("update-firmware-success", 255, ackCmdId);
    } else if (ret == HTTP_UPDATE_FAILED) {
      sendCommandAck("update-firmware-failed", 255, ackCmdId);
    } else if (ret == HTTP_UPDATE_NO_UPDATES) {
      sendCommandAck("update-firmware-no-update", 255, ackCmdId);
    } else {
      sendCommandAck("update-firmware-unknown", 255, ackCmdId);
    }
    delay(500);  // Allow message to be sent
  }

  delay(1000);
  ESP.restart();
}

uint32_t Sync::optimizeMemoryForOTA() {
  if (mqttClient.connected()) {
    mqttClient.disconnect();
    connected = false;
    delay(100);
  }
  ESP.wdtFeed();
  yield();
  delay(100);
  return ESP.getFreeHeap();
}

bool Sync::isConnected() {
  return mqttClient.connected();
}

bool Sync::isSyncing() {
  return mqttClient.connected() && (millis() - lastSuccessfulSync < 120000);
}

unsigned long Sync::getLastSuccessfulSync() {
  return lastSuccessfulSync;
}
