#include "Sensor.h"
#include "../globals.h"

Sensor::Sensor() {
    this->lastSensorValue = -1;
    this->lastSensorCheck = 0;
}

void Sensor::init() {
    if (deviceConfig.getSensorPin() == DeviceConfig::UNCONFIGURED_PIN) return;
    pinMode(deviceConfig.getSensorPin(), INPUT);
    DEBUG_PRINTLN("Initializing sensor event system...");
    lastSensorValue = digitalRead(deviceConfig.getSensorPin());
    lastSensorCheck = millis();
    DEBUG_PRINT("Initial sensor value: ");
    DEBUG_PRINTLN(lastSensorValue);
}

bool Sensor::hasChanged() {
    if (deviceConfig.getSensorPin() == DeviceConfig::UNCONFIGURED_PIN) return false;
    if (millis() - lastSensorCheck < SENSOR_CHECK_INTERVAL) {
        return false;
    }

    lastSensorCheck = millis();

    int currentSensorValue = digitalRead(deviceConfig.getSensorPin());

    if (currentSensorValue != lastSensorValue) {
        DEBUG_PRINT("Sensor change detected! Previous: ");
        DEBUG_PRINT(lastSensorValue);
        DEBUG_PRINT(", Current: ");
        DEBUG_PRINTLN(currentSensorValue);

        lastSensorValue = currentSensorValue;
        return true;
    }

    return false;
}

int Sensor::getValue() {
    return this->lastSensorValue;
}
