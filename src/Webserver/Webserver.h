#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include "DeviceConfig/DeviceConfig.h"
#include "../globals.h"

// Forward declaration
class Sync;

class Webserver {
    private:
        ESP8266WebServer server;
        DeviceConfig *deviceConfig;
        Sync *sync;
        static Webserver* instance;  // Static instance pointer

    public:
        Webserver(DeviceConfig *deviceConfig, Sync *sync);
        void handleClient();

        // Static handler functions
        static void handleConfig();
        static void handleSaveConfig();
        static void handleNotFound();
        static void handlePulse();
        static void handleRoot();
        static void handleInfo();
};

#endif