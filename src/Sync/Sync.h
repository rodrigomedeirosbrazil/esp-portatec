#ifndef SYNC_H
#define SYNC_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "DeviceConfig/DeviceConfig.h"

class Sync {
  private:
    unsigned long lastPing;
    unsigned long lastSuccessfulSync;
    DeviceConfig *deviceConfig;
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
    void sendCommandAck(String commandName);
    void updateFirmware();

  public:
    Sync(DeviceConfig *deviceConfig);
    void handle();
    void connect();
    bool isConnected();
    bool isSyncing();
};

#endif
