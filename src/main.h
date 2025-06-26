#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

void setupAPMode();
void handleConnection();
bool hasInternetConnection();
void waitForWifiConnection();
void reconnectWifi();
void handleApMode();

// Funções para gerenciar eventos do sensor
void initSensorEvents();
void checkSensorEvents();
void onSensorChange(int currentValue, int previousValue);

#endif // MAIN_H