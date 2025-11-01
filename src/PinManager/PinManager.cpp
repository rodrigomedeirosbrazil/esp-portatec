#include "PinManager.h"
#include "../Clock/Clock.h"
#include <LittleFS.h>

PinManager::PinManager() {
    // Constructor is kept light
}

void PinManager::begin() {
    // We assume LittleFS is already initialized in main setup()
    File pinsFile = LittleFS.open(pin_storage_path, "r");
    if (!pinsFile) {
        // File doesn't exist, start with an empty list
        return;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, pinsFile);
    pinsFile.close();

    if (error) {
        // Handle JSON parsing error, maybe log it
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        Pin p;
        p.code = obj["code"].as<String>();
        p.start_epoch = obj["start_epoch"].as<uint32_t>();
        p.end_epoch = obj["end_epoch"].as<uint32_t>();
        pins.push_back(p);
    }
}

bool PinManager::isPinValid(const String& pinCode) {
    uint32_t currentTime = Clock::getInstance().now();
    if (currentTime == 0) {
        // Clock not synced, deny all access
        return false;
    }

    for (const auto& pin : pins) {
        if (pin.code == pinCode) {
            // Check if the current time is within the valid window
            return (currentTime >= pin.start_epoch && currentTime < pin.end_epoch);
        }
    }
    return false; // Pin not found
}

void PinManager::purgeExpiredPins() {
    uint32_t currentTime = Clock::getInstance().now();
    if (currentTime == 0) return; // Cannot purge if clock is not set

    bool changed = false;
    auto it = pins.begin();
    while (it != pins.end()) {
        if (currentTime >= it->end_epoch) {
            it = pins.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (changed) {
        savePins();
    }
}

void PinManager::clearAllPins() {
    pins.clear();
    savePins();
}

void PinManager::addPin(const Pin& newPin) {
    // Optional: check for duplicates before adding
    pins.push_back(newPin);
}

void PinManager::savePins() {
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& pin : pins) {
        JsonObject obj = array.createNestedObject();
        obj["code"] = pin.code;
        obj["start_epoch"] = pin.start_epoch;
        obj["end_epoch"] = pin.end_epoch;
    }

    File pinsFile = LittleFS.open(pin_storage_path, "w");
    if (pinsFile) {
        serializeJson(doc, pinsFile);
        pinsFile.close();
    }
}

String PinManager::serializePins() {
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();

    for (const auto& pin : pins) {
        JsonObject obj = array.createNestedObject();
        obj["code"] = pin.code;
        obj["start_epoch"] = pin.start_epoch;
        obj["end_epoch"] = pin.end_epoch;
    }

    String output;
    serializeJson(doc, output);
    return output;
}
