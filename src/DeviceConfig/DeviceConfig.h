#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

class DeviceConfig {
private:
    static const int CONFIG_ADDRESS = 0;
    static const int CONFIG_MAX_SIZE = 480;
    static const uint32_t CONFIG_SIGNATURE = 0x504F5254;  // "PORT"
    static const uint8_t CONFIG_VERSION_STRUCT = 5;
    static const uint8_t CONFIG_VERSION_JSON = 6;

    static const char defaultDeviceName[];
    static const char defaultPassword[];

    bool configured;

public:
    static const uint8_t UNCONFIGURED_PIN = 255;
    static const char* FIRMWARE_VERSION;

    // In-memory config (loaded from JSON)
    char deviceName[32];
    char wifiPassword[32];
    char wifiSSID[32];
    char wifiNetworkPass[32];
    uint8_t pulsePin;
    uint8_t sensorPin;
    bool pulseInverted;
    char pin[7];
    char mqttHost[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[32];

    DeviceConfig();
    void begin();
    bool isConfigured() const { return configured; }
    const char* getDeviceName() const { return deviceName; }
    const char* getPassword() const { return wifiPassword; }
    const char* getWifiSSID() const { return wifiSSID; }
    const char* getWifiNetworkPass() const { return wifiNetworkPass; }
    uint8_t getPulsePin() const { return pulsePin; }
    uint8_t getSensorPin() const { return sensorPin; }
    bool getPulseInverted() const { return pulseInverted; }
    const char* getPin() const { return pin; }
    const char* getMqttHost() const { return mqttHost; }
    uint16_t getMqttPort() const { return mqttPort; }
    const char* getMqttUser() const { return mqttUser; }
    const char* getMqttPassword() const { return mqttPassword; }
    void setDeviceName(const char* name);
    void setPassword(const char* password);
    void setWifiSSID(const char* ssid);
    void setWifiNetworkPass(const char* password);
    void setPulsePin(uint8_t pin);
    void setSensorPin(uint8_t pin);
    void setPulseInverted(bool inverted);
    void setPin(const char* pin);
    void setMqttHost(const char* host);
    void setMqttPort(uint16_t port);
    void setMqttUser(const char* user);
    void setMqttPassword(const char* password);
    void initDefaultConfig();
    void loadConfig();
    void saveConfig();
};

#endif
