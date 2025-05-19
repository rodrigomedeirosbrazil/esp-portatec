#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

void setupAPMode();
void handleConnection();
bool hasInternetConnection();
void waitForWifiConnection();
void reconnectWifi();

#endif // MAIN_H