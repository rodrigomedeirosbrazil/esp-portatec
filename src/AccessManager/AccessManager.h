#ifndef ACCESSMANAGER_H
#define ACCESSMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct AccessPin {
    int id;
    String code;
    unsigned long start;
    unsigned long end;
};

class AccessManager {
public:
    AccessManager();
    void handlePinAction(String action, int id, String code, unsigned long start, unsigned long end);
    void syncFromBackend(JsonArray accessCodes);
    bool validate(String inputCode);
    void cleanup();

private:
    std::vector<AccessPin> pins;
    void createPin(int id, String code, unsigned long start, unsigned long end);
    void updatePin(int id, String code, unsigned long start, unsigned long end);
    void deletePin(int id);
};

#endif
