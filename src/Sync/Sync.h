#ifndef SYNC_H
#define SYNC_H

#include "DeviceConfig/DeviceConfig.h"
class Sync {
  private:
    unsigned long lastSync;
    unsigned long lastSuccessfulSync;
    DeviceConfig *deviceConfig;
    String deviceId;
  public:
    Sync(DeviceConfig *deviceConfig);
    void handle();
    void sync();
    bool isSyncing();
    void pulse();
    void updateFirmware();
};

#endif
