#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "ArduinoJson.h"

// Data structure for a single PIN
struct Pin {
    String code;
    uint32_t start_epoch;
    uint32_t end_epoch;
};

class PinManager {
public:
    PinManager();

    // Initialize the manager and load PINs from LittleFS
    void begin();

    // Check if a given PIN is valid at the current time
    bool isPinValid(const String& pinCode);

    // Removes pins that have expired
    void purgeExpiredPins();

    // Clears all pins from memory and storage
    void clearAllPins();

    // Adds a new pin to the manager
    void addPin(const Pin& newPin);

    // Saves the current list of PINs to LittleFS
    // To be called after a batch of changes
    void savePins();

    // Serializes the current list of active pins to a JSON string
    String serializePins();

private:
    std::vector<Pin> pins; // In-memory cache of PINs
    const char* pin_storage_path = "/pins.json";
};

#endif // PIN_MANAGER_H
