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

void Sensor::handle() {
    if (deviceConfig.getSensorPin() == DeviceConfig::UNCONFIGURED_PIN) return;
    if (millis() - lastSensorCheck < SENSOR_CHECK_INTERVAL) {
        return;
    }

    lastSensorCheck = millis();

    int currentSensorValue = digitalRead(deviceConfig.getSensorPin());

    if (currentSensorValue != lastSensorValue) {
        DEBUG_PRINT("Sensor change detected! Previous: ");
        DEBUG_PRINT(lastSensorValue);
        DEBUG_PRINT(", Current: ");
        DEBUG_PRINTLN(currentSensorValue);

        onSensorChange(currentSensorValue, lastSensorValue);

        lastSensorValue = currentSensorValue;
    }
}

void Sensor::onSensorChange(int currentValue, int previousValue) {
    DEBUG_PRINTLN("=== SENSOR EVENT TRIGGERED ===");
    DEBUG_PRINT("Sensor changed from ");
    DEBUG_PRINT(previousValue);
    DEBUG_PRINT(" to ");
    DEBUG_PRINTLN(currentValue);

    if (sync.isConnected()) {
        sync.sendSensorStatus(currentValue);
    }

    if (currentValue == HIGH && previousValue == LOW) {
        DEBUG_PRINTLN("Sensor activated (LOW -> HIGH)");
    } else if (currentValue == LOW && previousValue == HIGH) {
        DEBUG_PRINTLN("Sensor deactivated (HIGH -> LOW)");
    }

    DEBUG_PRINTLN("=== END SENSOR EVENT ===");
}
