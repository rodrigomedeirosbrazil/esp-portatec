#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

class DeviceConfig {
private:
    static const int CONFIG_ADDRESS = 0;
    static const uint32_t CONFIG_SIGNATURE = 0x504F5254;
    static const uint8_t CONFIG_VERSION = 5;

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
        uint8_t sensorPin;
        bool pulseInverted;
        char pin[7];
    };

    Config config;
    bool configured;

public:
    static const uint8_t UNCONFIGURED_PIN = 255;
    static const char* FIRMWARE_VERSION;

    DeviceConfig();
    bool isConfigured() const { return configured; }
    const char* getDeviceName() const { return config.deviceName; }
    const char* getPassword() const { return config.wifiPassword; }
    const char* getWifiSSID() const { return config.wifiSSID; }
    const char* getWifiNetworkPass() const { return config.wifiNetworkPass; }
    uint8_t getPulsePin() const { return config.pulsePin; }
    uint8_t getSensorPin() const { return config.sensorPin; }
    bool getPulseInverted() const { return config.pulseInverted; }
    const char* getPin() const { return config.pin; }
    void setDeviceName(const char* name);
    void setPassword(const char* password);
    void setWifiSSID(const char* ssid);
    void setWifiNetworkPass(const char* password);
    void setPulsePin(uint8_t pin);
    void setSensorPin(uint8_t pin);
    void setPulseInverted(bool inverted);
    void setPin(const char* pin);
    void initDefaultConfig();
    void loadConfig();
    void saveConfig();
};

#endif