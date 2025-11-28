#include "SystemClock.h"

SystemClock::SystemClock() : _lastSyncUnixTime(0), _lastSyncMillis(0), _lastNtpSyncMillis(0) {
    // Constructor initializes with 0, meaning time is not yet set.
}

void SystemClock::sync(unsigned long unix_time) {
    _lastSyncUnixTime = unix_time;
    _lastSyncMillis = millis();
}

unsigned long SystemClock::getUnixTime() {
    if (_lastSyncUnixTime == 0) {
        return 0; // Time not yet synchronized
    }
    unsigned long currentMillis = millis();
    unsigned long elapsedMillis = currentMillis - _lastSyncMillis;
    // millis() returns unsigned long, so direct subtraction handles overflow correctly.
    // Convert elapsed milliseconds to seconds.
    unsigned long elapsedSeconds = elapsedMillis / 1000;
    return _lastSyncUnixTime + elapsedSeconds;
}

void SystemClock::setupNtp() {
    // UTC-3 for Brazil, no daylight saving (0)
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void SystemClock::loop() {
    // Check every hour (3600000 ms)
    if (millis() - _lastNtpSyncMillis > 3600000 || _lastNtpSyncMillis == 0) {
        time_t now = time(nullptr);
        // Check if time is valid (e.g., > year 2020)
        if (now > 1577836800) {
            sync(now);
            _lastNtpSyncMillis = millis();
        }
    }
}
