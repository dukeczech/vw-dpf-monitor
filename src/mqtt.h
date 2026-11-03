#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <vector>

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

    // Publishes HA discovery, the last row as sensor states, the whole file
    // for the log table and waits until the broker echoes an upload marker.
    // Returns true only when the broker confirmed the upload.
    static bool sendLogFile(const String& path);
    static bool loop();

   private:
    static WiFiClient espClient;
    static PubSubClient mqttClient;
    static const String DEVICE_ID;
    static const String NODE_ID;
    static const String AVAILIBILITY_TOPIC;
    static const String STATE_TOPIC;
    static const String LOG_TOPIC;
    static const String UPLOAD_TOPIC;

    // Number of the newest log rows exposed in the HA table attribute
    static const uint16_t HA_LOG_ROWS = 39;  // keeps the attribute under HA's 16 kB recorder limit

    static std::vector<String> split(const String& line);
    static void addDevice(JsonDocument& doc);
    static bool publishJson(const String& topic, const JsonDocument& doc, bool retained);
    static bool publishAutoDiscovery(const std::vector<String>& headers);
    static bool publishStatusDiscovery(const std::vector<String>& headers, const std::vector<String>& sample);
    static bool publishLastRow(const std::vector<String>& headers, const String& line);
    static bool publishLogFile(const String& path);
    static bool confirmDelivery(uint32_t timeoutMs);
    static void onMessage(char* topic, byte* payload, unsigned int length);
};
