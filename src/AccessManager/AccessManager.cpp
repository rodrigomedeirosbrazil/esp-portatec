#include "AccessManager.h"
#include "../globals.h"

AccessManager::AccessManager() {
}

void AccessManager::handlePinAction(String action, int id, String code, unsigned long start, unsigned long end) {
    DEBUG_PRINT("[AccessManager] Handling action: ");
    DEBUG_PRINT(action);
    DEBUG_PRINT(" for ID: ");
    DEBUG_PRINTLN(id);

    if (action == "create") {
        createPin(id, code, start, end);
    } else if (action == "update") {
        updatePin(id, code, start, end);
    } else if (action == "delete") {
        deletePin(id);
    } else {
        DEBUG_PRINTLN("[AccessManager] Unknown action");
    }
}

void AccessManager::createPin(int id, String code, unsigned long start, unsigned long end) {
    // Check if already exists, if so, update
    for (auto& pin : pins) {
        if (pin.id == id) {
            DEBUG_PRINTLN("[AccessManager] Pin ID already exists, updating instead.");
            pin.code = code;
            pin.start = start;
            pin.end = end;
            return;
        }
    }
    
    AccessPin newPin = {id, code, start, end};
    pins.push_back(newPin);
    DEBUG_PRINTLN("[AccessManager] Pin created.");
}

void AccessManager::updatePin(int id, String code, unsigned long start, unsigned long end) {
    for (auto& pin : pins) {
        if (pin.id == id) {
            pin.code = code;
            pin.start = start;
            pin.end = end;
            DEBUG_PRINTLN("[AccessManager] Pin updated.");
            return;
        }
    }
    DEBUG_PRINTLN("[AccessManager] Pin ID not found for update.");
}

void AccessManager::deletePin(int id) {
    for (auto it = pins.begin(); it != pins.end(); ++it) {
        if (it->id == id) {
            pins.erase(it);
            DEBUG_PRINTLN("[AccessManager] Pin deleted.");
            return;
        }
    }
    DEBUG_PRINTLN("[AccessManager] Pin ID not found for deletion.");
}

int AccessManager::validate(String inputCode) {
    // 1. Check Master PIN
    if (inputCode == deviceConfig.getPin()) {
        DEBUG_PRINTLN("[AccessManager] Validated Master PIN");
        return -1;
    }

    // 2. Check Temporary PINs
    unsigned long currentUnixTime = systemClock.getUnixTime();
    
    // If clock is not synced (e.g. 0 or very low), we might deny access or allow?
    // Usually we deny if time dependent.
    if (currentUnixTime < 1000000) {
         DEBUG_PRINTLN("[AccessManager] System clock not synced, cannot validate temp pins reliably.");
         return 0; 
    }

    for (const auto& pin : pins) {
        if (pin.code == inputCode) {
            if (currentUnixTime >= pin.start && currentUnixTime <= pin.end) {
                DEBUG_PRINT("[AccessManager] Validated Temp PIN ID: ");
                DEBUG_PRINTLN(pin.id);
                return pin.id;
            } else {
                DEBUG_PRINT("[AccessManager] PIN found but time invalid. ID: ");
                DEBUG_PRINTLN(pin.id);
            }
        }
    }

    DEBUG_PRINTLN("[AccessManager] Invalid PIN");
    return 0;
}

void AccessManager::cleanup() {
    unsigned long currentUnixTime = systemClock.getUnixTime();
    if (currentUnixTime < 1000000) return; // Don't cleanup if time is wrong

    auto it = pins.begin();
    while (it != pins.end()) {
        if (it->end < currentUnixTime) {
            DEBUG_PRINT("[AccessManager] Removing expired PIN ID: ");
            DEBUG_PRINTLN(it->id);
            it = pins.erase(it);
        } else {
            ++it;
        }
    }
}
