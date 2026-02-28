#include "AccessManager.h"
#include "../globals.h"
#include <cstdio>
#include <cstring>

// Parse ISO 8601 "2026-01-01T00:00:00" or "2026-01-01T00:00:00Z" or "2026-01-01T00:00:00+00:00"
// Returns Unix timestamp (UTC), or 0 on parse error
static unsigned long parseIso8601ToUnix(const char* str) {
  if (!str || strlen(str) < 19) return 0;
  int year, month, day, hour, min, sec;
  if (sscanf(str, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec) != 6)
    return 0;
  if (year < 1970 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31)
    return 0;
  if (hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
    return 0;

  // Gregorian to Unix timestamp (UTC)
  if (month <= 2) {
    month += 12;
    year--;
  }
  unsigned long days = (unsigned long)day + (153 * (unsigned long)month - 457) / 5
    + 365 * (unsigned long)year + (unsigned long)year / 4
    - (unsigned long)year / 100 + (unsigned long)year / 400 - 719469;
  return days * 86400UL + (unsigned long)hour * 3600UL + (unsigned long)min * 60UL + (unsigned long)sec;
}

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

void AccessManager::syncFromBackend(JsonArray accessCodes) {
    pins.clear();
    int id = 0;
    for (JsonVariant v : accessCodes) {
        JsonObject obj = v.as<JsonObject>();
        const char* code = obj["pin"].as<const char*>();
        if (!code || strlen(code) == 0) continue;

        unsigned long startUnix;
        unsigned long endUnix;

        if (obj.containsKey("start_unix") && obj.containsKey("end_unix")) {
            startUnix = obj["start_unix"].as<unsigned long>();
            endUnix = obj["end_unix"].as<unsigned long>();
        } else if (obj.containsKey("start") && obj.containsKey("end")) {
            const char* startStr = obj["start"].as<const char*>();
            const char* endStr = obj["end"].as<const char*>();
            startUnix = parseIso8601ToUnix(startStr);
            endUnix = parseIso8601ToUnix(endStr);
            if (startUnix == 0 || endUnix == 0) {
                DEBUG_PRINTLN("[AccessManager] Skipping access code - invalid ISO 8601 date");
                continue;
            }
        } else {
            DEBUG_PRINTLN("[AccessManager] Skipping access code - missing start/end");
            continue;
        }

        createPin(id++, String(code), startUnix, endUnix);
    }
    DEBUG_PRINT("[AccessManager] Synced ");
    DEBUG_PRINT(pins.size());
    DEBUG_PRINTLN(" access codes from backend.");
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
