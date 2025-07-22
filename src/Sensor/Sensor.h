#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "../globals.h"

class Sensor {
private:
    int lastSensorValue;
    unsigned long lastSensorCheck;
    unsigned long lastStableValue;
    int stableValue;
    static const unsigned long SENSOR_CHECK_INTERVAL = 1000;
    static const unsigned long DEBOUNCE_DELAY = 500;
    static const int DEBOUNCE_COUNT = 3;

public:
    Sensor();
    void init();
    bool hasChanged();
    int getValue();
};

#endif
