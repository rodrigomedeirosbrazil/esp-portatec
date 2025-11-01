#include "Clock.h"

// Private constructor
Clock::Clock() : lastSyncEpoch(0), lastSyncMillis(0) {}

// Static method to get the single instance
Clock& Clock::getInstance() {
    static Clock instance; // Guaranteed to be destroyed, instantiated on first use.
    return instance;
}

// Set the official time
void Clock::setTime(uint32_t epoch) {
    lastSyncEpoch = epoch;
    lastSyncMillis = millis();
}

// Get the current estimated time
uint32_t Clock::now() {
    if (lastSyncEpoch == 0) {
        // If time has never been set, return 0 or an invalid time marker.
        // This indicates that the clock is not synchronized.
        return 0;
    }
    // Calculate elapsed seconds since last sync and add to the epoch
    return lastSyncEpoch + ((millis() - lastSyncMillis) / 1000);
}
