#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <functional>
#include "../globals.h"

class Webserver {
    private:
        ESP8266WebServer server;
        static Webserver* instance;  // Static instance pointer

        // Helper static function
        static String formatUnixTime(unsigned long unix_timestamp);
        static void sendHtml(const String& path, std::function<String(const String&)> processor);

        // Static handler functions
        static void handleConfig();
        static void handleSaveConfig();
        static void handleNotFound();
        static void handlePulse();
        static void handleIndex();
        static void handleInfo();

    public:
        Webserver();
        void begin();
        void handleClient();
};

#endif