#include "config.h"

bool testMode = false;
eRunningState state = Init;

// Use specific code for T-display-s3
#if 1
Arduino_ESP32PAR8Q bus(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_ST7789 gfx(&bus, 5, 0, true, 170, 320, 35, 0, 35, 0);
#endif

BTAdapter& serialBT = BTAdapter::GetInstance();
Preferences prefs;
WifiConfig wificfg;
MqttConfig mqttcfg;

void ApplicationConfig::load() {
    Serial.println("=== Preferences wifi ===");

    if (prefs.begin("wifi", true)) {
        wificfg.ssid = prefs.getString("ssid", "");
        wificfg.password = prefs.getString("pass", "");

        Serial.print("ssid: ");
        Serial.println(wificfg.ssid.length() ? wificfg.ssid : "<empty>");

        Serial.print("pass: ");
        Serial.println(wificfg.password.length() ? "<set>" : "<empty>");

        prefs.end();
    } else {
        Serial.println("wifi namespace not found");
    }

    Serial.println();
    Serial.println("=== Preferences mqtt ===");

    if (prefs.begin("mqtt", true)) {
        mqttcfg.server = prefs.getString("server", "");
        mqttcfg.port = prefs.getUShort("port", 1883);
        mqttcfg.username = prefs.getString("user", "");
        mqttcfg.password = prefs.getString("pass", "");
        mqttcfg.topic = prefs.getString("topic", "");

        Serial.print("server: ");
        Serial.println(mqttcfg.server.length() ? mqttcfg.server : "<empty>");

        Serial.print("port: ");
        Serial.println(mqttcfg.port);

        Serial.print("user: ");
        Serial.println(mqttcfg.username.length() ? mqttcfg.username : "<empty>");

        Serial.print("pass: ");
        Serial.println(mqttcfg.password.length() ? "<set>" : "<empty>");

        Serial.print("topic: ");
        Serial.println(mqttcfg.topic.length() ? mqttcfg.topic : "<empty>");

        prefs.end();
    } else {
        Serial.println("mqtt namespace not found");
    }

    Serial.println("==========================");
}

void ApplicationConfig::save() {
    prefs.begin("wifi", false);
    prefs.putString("ssid", wificfg.ssid);
    prefs.putString("pass", wificfg.password);
    prefs.end();

    prefs.begin("mqtt", false);
    prefs.putString("server", mqttcfg.server);
    prefs.putUShort("port", mqttcfg.port);
    prefs.putString("user", mqttcfg.username);
    prefs.putString("pass", mqttcfg.password);
    prefs.putString("topic", mqttcfg.topic);
    prefs.end();
}
