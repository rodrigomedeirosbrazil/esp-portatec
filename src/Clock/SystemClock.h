#ifndef SYSTEMCLOCK_H
#define SYSTEMCLOCK_H

#include <Arduino.h> // For millis() and unsigned long

class SystemClock {
public:
    SystemClock();
    void sync(unsigned long unix_time);
    unsigned long getUnixTime();

private:
    unsigned long _lastSyncUnixTime;
    unsigned long _lastSyncMillis;
};

#endif // SYSTEMCLOCK_H
