#include "Sensor.h"
#include "../globals.h"

Sensor::Sensor() {
    this->lastSensorValue = -1;
    this->lastSensorCheck = 0;
    this->lastStableValue = 0;
    this->stableValue = -1;
}

void Sensor::init() {
    if (deviceConfig.getSensorPin() == DeviceConfig::UNCONFIGURED_PIN) return;
    pinMode(deviceConfig.getSensorPin(), INPUT_PULLUP); // Adiciona pull-up interno
    DEBUG_PRINTLN("Initializing sensor event system...");
    lastSensorValue = digitalRead(deviceConfig.getSensorPin());
    stableValue = lastSensorValue;
    lastSensorCheck = millis();
    lastStableValue = millis();
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
        lastStableValue = millis();
        lastSensorValue = currentSensorValue;
        DEBUG_PRINT("Sensor value changed to: ");
        DEBUG_PRINTLN(currentSensorValue);
        return false;
    }


    if (millis() - lastStableValue >= DEBOUNCE_DELAY) {
        if (currentSensorValue != stableValue) {
            DEBUG_PRINT("Sensor change confirmed! Previous: ");
            DEBUG_PRINT(stableValue);
            DEBUG_PRINT(", Current: ");
            DEBUG_PRINTLN(currentSensorValue);

            stableValue = currentSensorValue;
            return true;
        }
    }

    return false;
}

int Sensor::getValue() {
    return this->stableValue;
}
