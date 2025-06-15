#ifndef GLOBALS_H
#define GLOBALS_H

#include <IPAddress.h>

// Debug flag - comment/uncomment the line below to disable/enable debug logging
// This uses compile-time preprocessing instead of runtime checks for better performance
// #define DEBUG

extern IPAddress myIP;
extern String API_KEY;

// Debug helper macros
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#endif