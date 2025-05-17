#include <Arduino.h>
#include <EEPROM.h>
#include "DeviceConfig.h"

const char DeviceConfig::defaultDeviceName[] = "ESP-PORTATEC";
const char DeviceConfig::defaultPassword[] = "123456789";

DeviceConfig::DeviceConfig() : configured(false) {
    EEPROM.begin(512);
    loadConfig();
}

void DeviceConfig::initDefaultConfig() {
  config.signature = CONFIG_SIGNATURE;
  config.version = CONFIG_VERSION;
  strcpy(config.deviceName, defaultDeviceName);
  strcpy(config.wifiPassword, defaultPassword);
}

void DeviceConfig::loadConfig() {
  EEPROM.get(CONFIG_ADDRESS, config);

  if (config.signature != CONFIG_SIGNATURE || config.version != CONFIG_VERSION) {
    initDefaultConfig();
    saveConfig();
    configured = false;
    return;
  }

  configured = true;
}

void DeviceConfig::saveConfig() {
  config.signature = CONFIG_SIGNATURE;
  config.version = CONFIG_VERSION;
  EEPROM.put(CONFIG_ADDRESS, config);
  EEPROM.commit();
}

void DeviceConfig::setDeviceName(const char* name) {
    strncpy(config.deviceName, name, sizeof(config.deviceName) - 1);
    config.deviceName[sizeof(config.deviceName) - 1] = '\0';
}

void DeviceConfig::setPassword(const char* password) {
    strncpy(config.wifiPassword, password, sizeof(config.wifiPassword) - 1);
    config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
}
