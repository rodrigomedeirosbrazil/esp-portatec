#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

class DeviceConfig {
private:
    static const int CONFIG_ADDRESS = 0;
    static const uint32_t CONFIG_SIGNATURE = 0x504F5254;
    static const uint8_t CONFIG_VERSION = 3;

    static const char defaultDeviceName[];
    static const char defaultPassword[];

    struct Config {
        uint32_t signature;
        uint8_t version;
        char deviceName[32];
        char wifiPassword[32];
        char wifiSSID[32];      // SSID of the WiFi network to connect to
        char wifiNetworkPass[32]; // Password of the WiFi network to connect to
        uint8_t pulsePin;
    };

    Config config;
    bool configured;

public:
    static const char* FIRMWARE_VERSION;

    DeviceConfig();
    bool isConfigured() const { return configured; }
    const char* getDeviceName() const { return config.deviceName; }
    const char* getPassword() const { return config.wifiPassword; }
    const char* getWifiSSID() const { return config.wifiSSID; }
    const char* getWifiNetworkPass() const { return config.wifiNetworkPass; }
    uint8_t getPulsePin() const { return config.pulsePin; }
    void setDeviceName(const char* name);
    void setPassword(const char* password);
    void setWifiSSID(const char* ssid);
    void setWifiNetworkPass(const char* password);
    void setPulsePin(uint8_t pin);
    void initDefaultConfig();
    void loadConfig();
    void saveConfig();
};

#endif