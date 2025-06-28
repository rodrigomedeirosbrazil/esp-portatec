#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "../DeviceConfig/DeviceConfig.h"
#include "../Sync/Sync.h"

class Sensor {
private:
    DeviceConfig *deviceConfig;
    Sync *sync;
    int lastSensorValue;
    unsigned long lastSensorCheck;
    static const unsigned long SENSOR_CHECK_INTERVAL = 100;

    void onSensorChange(int currentValue, int previousValue);

public:
    Sensor(DeviceConfig *deviceConfig, Sync *sync);
    void init();
    void handle();
};

#endif