#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "../globals.h"

class Sensor {
private:
    int lastSensorValue;
    unsigned long lastSensorCheck;
    static const unsigned long SENSOR_CHECK_INTERVAL = 100;

    void onSensorChange(int currentValue, int previousValue);

public:
    Sensor();
    void init();
    void handle();
};

#endif
