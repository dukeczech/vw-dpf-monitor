#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"

class MQTTClient {
   public:
    static bool connect();
    static bool disconnect();
    static bool isConnected();
    static String normalizeHeader(const String& h);
    static String extractUnit(const String& h);
    static String extractName(const String& h);
    static String deviceClassFromUnit(const String& unit);
    static bool sendLogFile(const String& path);
    static bool loop();

   private:
    static WiFiClient espClient;
    static PubSubClient mqttClient;
    static const String DEVICE_ID;
    static const String NODE_ID;
    static const String AVAILIBILITY_TOPIC;
    static const String STATE_TOPIC;

    static void publishAutoDiscovery(const std::vector<String>& headers);
};