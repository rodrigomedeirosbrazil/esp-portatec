#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "DeviceConfig.h"

const char DeviceConfig::defaultDeviceName[] = "ESP-PORTATEC";
const char DeviceConfig::defaultPassword[] = "123456789";
#ifdef FIRMWARE_VERSION_STR
const char* DeviceConfig::FIRMWARE_VERSION = FIRMWARE_VERSION_STR;
#else
const char* DeviceConfig::FIRMWARE_VERSION = "dev";
#endif

// Legacy struct for migration from binary format
#pragma pack(push, 1)
struct LegacyConfig {
    uint32_t signature;
    uint8_t version;
    char deviceName[32];
    char wifiPassword[32];
    char wifiSSID[32];
    char wifiNetworkPass[32];
    uint8_t pulsePin;
    uint8_t sensorPin;
    bool pulseInverted;
    char pin[7];
};
#pragma pack(pop)

DeviceConfig::DeviceConfig() : configured(false) {
    EEPROM.begin(512);
    loadConfig();
}

void DeviceConfig::initDefaultConfig() {
    strcpy(deviceName, defaultDeviceName);
    strcpy(wifiPassword, defaultPassword);
    wifiSSID[0] = '\0';
    wifiNetworkPass[0] = '\0';
    pulsePin = 3;
    sensorPin = UNCONFIGURED_PIN;
    pulseInverted = false;
    strcpy(pin, "123456");
    strcpy(mqttHost, "portatec.medeirostec.com.br");  // Default broker
    mqttPort = 1883;
    mqttUser[0] = '\0';
    mqttPassword[0] = '\0';
}

void DeviceConfig::loadConfig() {
    uint8_t firstByte = EEPROM.read(CONFIG_ADDRESS);

    // New format: JSON starts with '{'
    if (firstByte == '{') {
        char jsonBuffer[CONFIG_MAX_SIZE];
        for (int i = 0; i < CONFIG_MAX_SIZE - 1; i++) {
            jsonBuffer[i] = EEPROM.read(CONFIG_ADDRESS + i);
            if (jsonBuffer[i] == '\0') break;
        }
        jsonBuffer[CONFIG_MAX_SIZE - 1] = '\0';

        DynamicJsonDocument doc(CONFIG_MAX_SIZE);
        DeserializationError error = deserializeJson(doc, jsonBuffer);

        if (error) {
            initDefaultConfig();
            configured = false;
            return;
        }

        const char* v;
        v = doc["deviceName"].as<const char*>(); strncpy(deviceName, v ? v : defaultDeviceName, sizeof(deviceName) - 1);
        deviceName[sizeof(deviceName) - 1] = '\0';
        v = doc["wifiPassword"].as<const char*>(); strncpy(wifiPassword, v ? v : defaultPassword, sizeof(wifiPassword) - 1);
        wifiPassword[sizeof(wifiPassword) - 1] = '\0';
        v = doc["wifiSSID"].as<const char*>(); strncpy(wifiSSID, v ? v : "", sizeof(wifiSSID) - 1);
        wifiSSID[sizeof(wifiSSID) - 1] = '\0';
        v = doc["wifiNetworkPass"].as<const char*>(); strncpy(wifiNetworkPass, v ? v : "", sizeof(wifiNetworkPass) - 1);
        wifiNetworkPass[sizeof(wifiNetworkPass) - 1] = '\0';
        pulsePin = doc.containsKey("pulsePin") ? (int)doc["pulsePin"] : 3;
        sensorPin = doc.containsKey("sensorPin") ? (int)doc["sensorPin"] : UNCONFIGURED_PIN;
        pulseInverted = doc["pulseInverted"].as<bool>();
        v = doc["pin"].as<const char*>(); strncpy(pin, v ? v : "123456", sizeof(pin) - 1);
        pin[sizeof(pin) - 1] = '\0';
        v = doc["mqttHost"].as<const char*>(); strncpy(mqttHost, v ? v : "", sizeof(mqttHost) - 1);
        mqttHost[sizeof(mqttHost) - 1] = '\0';
        mqttPort = doc.containsKey("mqttPort") ? (int)doc["mqttPort"] : 1883;
        v = doc["mqttUser"].as<const char*>(); strncpy(mqttUser, v ? v : "", sizeof(mqttUser) - 1);
        mqttUser[sizeof(mqttUser) - 1] = '\0';
        v = doc["mqttPassword"].as<const char*>(); strncpy(mqttPassword, v ? v : "", sizeof(mqttPassword) - 1);
        mqttPassword[sizeof(mqttPassword) - 1] = '\0';

        configured = (strlen(wifiSSID) > 0);
        return;
    }

    // Legacy format: binary struct
    LegacyConfig legacy;
    EEPROM.get(CONFIG_ADDRESS, legacy);

    if (legacy.signature != CONFIG_SIGNATURE || legacy.version != CONFIG_VERSION_STRUCT) {
        initDefaultConfig();
        configured = false;
        return;
    }

    // Migrate to JSON format
    strncpy(deviceName, legacy.deviceName, sizeof(deviceName) - 1);
    deviceName[sizeof(deviceName) - 1] = '\0';
    strncpy(wifiPassword, legacy.wifiPassword, sizeof(wifiPassword) - 1);
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';
    strncpy(wifiSSID, legacy.wifiSSID, sizeof(wifiSSID) - 1);
    wifiSSID[sizeof(wifiSSID) - 1] = '\0';
    strncpy(wifiNetworkPass, legacy.wifiNetworkPass, sizeof(wifiNetworkPass) - 1);
    wifiNetworkPass[sizeof(wifiNetworkPass) - 1] = '\0';
    pulsePin = legacy.pulsePin;
    sensorPin = legacy.sensorPin;
    pulseInverted = legacy.pulseInverted;
    strncpy(pin, legacy.pin, sizeof(pin) - 1);
    pin[sizeof(pin) - 1] = '\0';
    strcpy(mqttHost, "portatec.medeirostec.com.br");  // Default for migrated devices
    mqttPort = 1883;
    mqttUser[0] = '\0';
    mqttPassword[0] = '\0';

    configured = true;
    saveConfig();  // Save in new JSON format
}

void DeviceConfig::saveConfig() {
    DynamicJsonDocument doc(CONFIG_MAX_SIZE);
    doc["deviceName"] = deviceName;
    doc["wifiPassword"] = wifiPassword;
    doc["wifiSSID"] = wifiSSID;
    doc["wifiNetworkPass"] = wifiNetworkPass;
    doc["pulsePin"] = pulsePin;
    doc["sensorPin"] = sensorPin;
    doc["pulseInverted"] = pulseInverted;
    doc["pin"] = pin;
    doc["mqttHost"] = mqttHost;
    doc["mqttPort"] = mqttPort;
    doc["mqttUser"] = mqttUser;
    doc["mqttPassword"] = mqttPassword;

    String output;
    serializeJson(doc, output);

    for (size_t i = 0; i < output.length() && i < CONFIG_MAX_SIZE - 1; i++) {
        EEPROM.write(CONFIG_ADDRESS + i, output[i]);
    }
    EEPROM.write(CONFIG_ADDRESS + output.length(), '\0');
    EEPROM.commit();
}

void DeviceConfig::setDeviceName(const char* name) {
    strncpy(deviceName, name, sizeof(deviceName) - 1);
    deviceName[sizeof(deviceName) - 1] = '\0';
}

void DeviceConfig::setPassword(const char* password) {
    strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';
}

void DeviceConfig::setPulsePin(uint8_t pinNum) {
    pulsePin = pinNum;
}

void DeviceConfig::setSensorPin(uint8_t pinNum) {
    sensorPin = pinNum;
}

void DeviceConfig::setPulseInverted(bool inverted) {
    pulseInverted = inverted;
}

void DeviceConfig::setWifiSSID(const char* ssid) {
    strncpy(wifiSSID, ssid, sizeof(wifiSSID) - 1);
    wifiSSID[sizeof(wifiSSID) - 1] = '\0';
}

void DeviceConfig::setWifiNetworkPass(const char* password) {
    strncpy(wifiNetworkPass, password, sizeof(wifiNetworkPass) - 1);
    wifiNetworkPass[sizeof(wifiNetworkPass) - 1] = '\0';
}

void DeviceConfig::setPin(const char* pinCode) {
    strncpy(pin, pinCode, sizeof(pin) - 1);
    pin[sizeof(pin) - 1] = '\0';
}

void DeviceConfig::setMqttHost(const char* host) {
    strncpy(mqttHost, host, sizeof(mqttHost) - 1);
    mqttHost[sizeof(mqttHost) - 1] = '\0';
}

void DeviceConfig::setMqttPort(uint16_t port) {
    mqttPort = port;
}

void DeviceConfig::setMqttUser(const char* user) {
    strncpy(mqttUser, user, sizeof(mqttUser) - 1);
    mqttUser[sizeof(mqttUser) - 1] = '\0';
}

void DeviceConfig::setMqttPassword(const char* password) {
    strncpy(mqttPassword, password, sizeof(mqttPassword) - 1);
    mqttPassword[sizeof(mqttPassword) - 1] = '\0';
}
