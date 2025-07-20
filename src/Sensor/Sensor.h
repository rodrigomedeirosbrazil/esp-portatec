#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "../globals.h"

class Sensor {
private:
    int lastSensorValue;
    unsigned long lastSensorCheck;
    static const unsigned long SENSOR_CHECK_INTERVAL = 100;

public:
    Sensor();
    void init();
    bool hasChanged();
    int getValue();
};

#endif
