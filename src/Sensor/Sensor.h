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
    static const unsigned long SENSOR_CHECK_INTERVAL = 80;   // leitura a cada ~80 ms para debounce efetivo
    static const unsigned long DEBOUNCE_DELAY = 200;        // valor estável por 200 ms antes de confirmar
    static const int DEBOUNCE_COUNT = 3;

public:
    Sensor();
    void init();
    bool hasChanged();
    int getValue();
};

#endif
