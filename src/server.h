#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

class WifiServer {
   public:
    WifiServer();

    static bool init(const String& ssid, const String& password);
    static bool hasClient();
    static void loop();

    static String table(const String& var);

   protected:
    static AsyncWebServer m_server;
    static const String m_index;
};