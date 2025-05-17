#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>


void initDefaultConfig();
void loadConfig();
void saveConfig();
void handleConfig();
void handleSaveConfig();
void handleRoot();
void handleGPIO();
void handleNotFound();

#endif // MAIN_H