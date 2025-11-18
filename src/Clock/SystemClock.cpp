#include "SystemClock.h"

SystemClock::SystemClock() : _lastSyncUnixTime(0), _lastSyncMillis(0) {
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
