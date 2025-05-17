#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

class DeviceConfig {
private:
    static const int CONFIG_ADDRESS = 0;
    static const uint32_t CONFIG_SIGNATURE = 0x504F5254;
    static const uint8_t CONFIG_VERSION = 1;

    static const char defaultDeviceName[];
    static const char defaultPassword[];

    struct Config {
        uint32_t signature;
        uint8_t version;
        char deviceName[32];
        char wifiPassword[32];
        uint8_t pulsePin;
    };

    Config config;
    bool configured;

public:
    DeviceConfig();
    bool isConfigured() const { return configured; }
    const char* getDeviceName() const { return config.deviceName; }
    const char* getPassword() const { return config.wifiPassword; }
    uint8_t getPulsePin() const { return config.pulsePin; }
    void setDeviceName(const char* name);
    void setPassword(const char* password);
    void setPulsePin(uint8_t pin);
    void initDefaultConfig();
    void loadConfig();
    void saveConfig();
};

#endif