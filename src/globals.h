#ifndef GLOBALS_H
#define GLOBALS_H

#include <IPAddress.h>

// Debug flag - set to true to enable debug logging
#define DEBUG true

extern IPAddress myIP;
extern String API_KEY;

// Debug helper macro
#define DEBUG_PRINT(x) if(DEBUG) { Serial.print(x); }
#define DEBUG_PRINTLN(x) if(DEBUG) { Serial.println(x); }

#endif