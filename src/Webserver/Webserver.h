#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include "../globals.h"

class Webserver {
    private:
        ESP8266WebServer server;
        static Webserver* instance;  // Static instance pointer

    public:
        Webserver();
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
