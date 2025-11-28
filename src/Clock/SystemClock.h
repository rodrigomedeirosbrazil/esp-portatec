#ifndef SYSTEMCLOCK_H
#define SYSTEMCLOCK_H

#include <Arduino.h> // For millis() and unsigned long
#include <time.h>

class SystemClock {
public:
    SystemClock();
    void sync(unsigned long unix_time);
    unsigned long getUnixTime();
    void setupNtp();
    void loop();

private:
    unsigned long _lastSyncUnixTime;
    unsigned long _lastSyncMillis;
    unsigned long _lastNtpSyncMillis;
};

#endif // SYSTEMCLOCK_H
