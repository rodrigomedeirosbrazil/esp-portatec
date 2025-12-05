#ifndef SYNC_H
#define SYNC_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "globals.h"

class Sync {
  private:
    unsigned long lastPing;
    unsigned long lastSuccessfulSync;
    String deviceId;
    String channelName;
    WebSocketsClient webSocket;
    bool connected;
    bool subscribed;

    void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    void subscribeToChannel();
    void sendPong();
    void sendDeviceStatus();
    void handlePusherMessage(String message);
    void pulse();
    void sendCommandAck(String commandName, uint8_t gpio);
    void updateFirmware();
    void sendDiagnosticInfo(String event);
    uint32_t optimizeMemoryForOTA();
    void processPinAction(JsonVariant data);

  public:
    Sync();
    void handle();
    void connect();
    bool isConnected();
    bool isSyncing();
    unsigned long getLastSuccessfulSync();
    void sendSensorStatus(int value);
    void sendPinUsage(int pinId);
};

#endif
