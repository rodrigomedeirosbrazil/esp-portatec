#ifndef SYNC_H
#define SYNC_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "globals.h"

class Sync {
  private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt;
    unsigned long lastSuccessfulSync;
    unsigned long lastHeartbeat;
    String deviceId;
    String topicCommand;
    String topicAccessCodesSync;
    String topicAck;
    String topicStatus;
    String topicEvent;
    String topicAccessCodesAck;
    bool connected;
    String clientId;

    void subscribeToTopics();
    void sendDeviceStatus();
    void handleCommand(JsonObject data);
    void handleAccessCodesSync(JsonObject data);
    void executeRelay(const char* action, const char* commandId);
    void sendCommandAck(String action, uint8_t gpio, const char* commandId);
    void updateFirmware(const char* commandId);
    uint32_t optimizeMemoryForOTA();
    bool reconnect();

  public:
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    Sync();
    void handle();
    void connect();
    bool isConnected();
    bool isSyncing();
    unsigned long getLastSuccessfulSync();
    void sendSensorStatus(int value);
    void sendPinUsage(int pinId);
    void sendAccessEvent(const char* code, const char* result, unsigned long timestamp);
};

#endif
