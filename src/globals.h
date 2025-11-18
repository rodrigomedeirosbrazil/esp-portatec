#ifndef GLOBALS_H
#define GLOBALS_H

#include <IPAddress.h>
#include "Clock/Clock.h" // Include Clock.h

// Debug flag - comment/uncomment the line below to disable/enable debug logging
// This uses compile-time preprocessing instead of runtime checks for better performance
// #define DEBUG

#include "DeviceConfig/DeviceConfig.h"
#include "Sensor/Sensor.h"
#include "Sync/Sync.h"
#include "Webserver/Webserver.h"

class DeviceConfig;
class Sensor;
class Sync;
class Webserver;

extern IPAddress myIP;
extern String API_KEY;

extern DeviceConfig deviceConfig;
extern Sensor sensor;
extern Sync sync;
extern Webserver webserver;
extern Clock clock; // Declare global Clock instance

// Debug helper macros
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#endif
