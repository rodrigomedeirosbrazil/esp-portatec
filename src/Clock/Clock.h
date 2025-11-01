#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>

class Clock {
public:
    // Returns the single instance of the Clock class
    static Clock& getInstance();

    // Sets the current time from a Unix epoch
    void setTime(uint32_t epoch);

    // Gets the current estimated time as a Unix epoch
    uint32_t now();

private:
    // Private constructor for the singleton pattern
    Clock();

    uint32_t lastSyncEpoch;   // Last known time from server
    uint32_t lastSyncMillis;  // Millis() value at the last sync

    // Prevent copying
    Clock(Clock const&) = delete;
    void operator=(Clock const&) = delete;
};

#endif // CLOCK_H
