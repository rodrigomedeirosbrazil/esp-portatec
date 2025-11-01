#ifndef SYNC_H
#define SYNC_H

#include "../globals.h"

class Sync {
  private:
    unsigned long lastSyncMillis;
    unsigned long syncInterval; // Interval in milliseconds
    String deviceId;

    // Performs the synchronization with the server
    void syncNow();

  public:
    Sync();

    // Should be called in the main loop
    void handle();
};

#endif
