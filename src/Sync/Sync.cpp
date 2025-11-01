#include "Sync.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "../globals.h"
#include "../Clock/Clock.h"
#include "../PinManager/PinManager.h"

// Forward declaration of the global pinManager instance
extern PinManager pinManager;

Sync::Sync() {
    lastSyncMillis = 0;
    syncInterval = 300000; // 5 minutes
    deviceId = String(ESP.getChipId(), HEX);
}

void Sync::handle() {
    // Only try to sync if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (millis() - lastSyncMillis > syncInterval) {
        syncNow();
        lastSyncMillis = millis();
    }
}

void Sync::syncNow() {
    WiFiClient client;
    HTTPClient http;

    String serverUrl = "http://your-server.com/api/sync"; // TODO: Make this configurable
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Create the request body
    String requestBody = pinManager.serializePins();

    int httpCode = http.POST(requestBody);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            // JSON parsing failed
            http.end();
            return;
        }

        // Update clock
        uint32_t serverEpoch = doc["epoch"].as<uint32_t>();
        Clock::getInstance().setTime(serverEpoch);

        // Update pins
        pinManager.clearAllPins();
        JsonArray pinsArray = doc["pins"].as<JsonArray>();
        for (JsonObject pinObj : pinsArray) {
            Pin p;
            p.code = pinObj["code"].as<String>();
            p.start_epoch = pinObj["start_epoch"].as<uint32_t>();
            p.end_epoch = pinObj["end_epoch"].as<uint32_t>();
            pinManager.addPin(p);
        }
        pinManager.savePins(); // Save the new set of pins to LittleFS

    } else if (httpCode == HTTP_CODE_NO_CONTENT) {
        // No changes, everything is up to date.
        // We can still update our clock from a header if the server provides it.
    } else {
        // HTTP error
    }

    http.end();
}
