#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

struct IndexStream;

class WifiServer {
   public:
    WifiServer();

    static bool init(const String& ssid, const String& password);
    static bool isUp();
    static bool hasClient();
    static String getIP();
    static void loop();

    static void wifiEvent(WiFiEvent_t event);

   protected:
    // Streams the index page with the log table in chunks, so the page size is not
    // limited by the available RAM or by the Arduino String capacity (64 KB).
    static bool nextIndexChunk(IndexStream& stream);
    static String tableRow(String line, IndexStream& stream);

    static bool m_up;
    static bool m_connected;
    static AsyncWebServer m_server;
    static const String m_index;
};

class WifiClient {
   public:
    WifiClient();

    static bool init(const String& ssid, const String& password);
    static bool isConnected();
    static String getIP();
    static void loop();
    static bool isWifiAvailable(const String& ssid, int timeout = 2000);
};