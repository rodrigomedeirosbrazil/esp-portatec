#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h> // For millis() and unsigned long

class Clock {
public:
    Clock();
    void sync(unsigned long unix_time);
    unsigned long getUnixTime();

private:
    unsigned long _lastSyncUnixTime;
    unsigned long _lastSyncMillis;
};

#endif // CLOCK_H
