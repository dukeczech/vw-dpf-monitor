#include "mqtt.h"

#include <ArduinoJson.h>

#include "storage.h"
#include "wifi.h"

WiFiClient MQTTClient::espClient;
PubSubClient MQTTClient::mqttClient(MQTTClient::espClient);
const String MQTTClient::DEVICE_ID = "touran";
const String MQTTClient::NODE_ID = "touran/dpf";
const String MQTTClient::AVAILIBILITY_TOPIC = NODE_ID + "/status";
const String MQTTClient::STATE_TOPIC = NODE_ID + "/state";

bool MQTTClient::connect() {
    mqttClient.setServer(mqttcfg.server.c_str(), mqttcfg.port);
    // Increase the mqtt client buffer size (the state message has ~500 characters)
    mqttClient.setBufferSize(750);

    int mqttAttempts = 0;
    while (!mqttClient.connected() && mqttAttempts < 10) {
        // if (mqttClient.connect(DEVICE_ID.c_str(), mqttcfg.username.c_str(), mqttcfg.password.c_str()))
        if (mqttClient.connect(DEVICE_ID.c_str(),
                               (mqttcfg.username.isEmpty()) ? "" : mqttcfg.username.c_str(),
                               (mqttcfg.password.isEmpty()) ? "" : mqttcfg.password.c_str(),
                               AVAILIBILITY_TOPIC.c_str(), 0, true, "offline"))
            break;
        Serial.print(".");
        delay(500);
        mqttAttempts++;
    }

    if (!mqttClient.connected()) {
        Serial.println("MQTT connection failed, skipping send");
        return false;
    }

    mqttClient.publish(AVAILIBILITY_TOPIC.c_str(), "online", true);

    Serial.println("MQTT connected");
    return true;
}

bool MQTTClient::disconnect() {
    mqttClient.disconnect();
    return true;
}

void MQTTClient::publishAutoDiscovery(const std::vector<String>& headers) {
    // Home Assistant expects topic: homeassistant/sensor/<unique_id>/config
    for (size_t i = 0; i < headers.size(); i++) {
        const String sensorName = "dpf_" + normalizeHeader(extractName(headers[i]));
        const String discoveryTopic = "homeassistant/sensor/" + DEVICE_ID + "/" + sensorName + "/config";
        const String unit = extractUnit(headers[i]);
        const String devClass = deviceClassFromUnit(unit);

        JsonDocument doc;
        doc["name"] = extractName(headers[i]);
        doc["unique_id"] = sensorName;
        doc["state_topic"] = STATE_TOPIC;
        doc["value_template"] = String("{{ value_json.") + sensorName + " }}";
        doc["unit_of_measurement"] = unit;
        doc["force_update"] = "true";
        if (devClass != "") {
            doc["device_class"] = devClass;
        }
        if (sensorName == "dpf_distance_driven") {
            // This tells HA:
            // This is cumulative
            // Don’t interpret deltas as noise
            // Enables correct statistics
            doc["state_class"] = "total_increasing";
        }
        if (sensorName == "dpf_soot_mass_calculated" ||
            sensorName == "dpf_soot_mass_measured" ||
            sensorName == "dpf_oil_ash_residue") {
            doc["suggested_display_precision"] = 2;
        }

        // Add device info
        JsonObject device = doc.createNestedObject("device");
        device["name"] = "Touran - DPF indicator";
        device["identifiers"] = JsonArray();
        device["identifiers"].add("touran_dpf_01");
        device["model"] = "Touran";
        device["manufacturer"] = "VW";
        device["configuration_url"] = String("http://") + WifiServer::getIP();
        device["hw_version"] = ESP.getChipModel();
        // device["sw_version"] = "500+TZSKT31BS101";

        // Serialize to a string
        String payload;
        serializeJson(doc, payload);

        // --- Debug log before publishing ---
        Serial.print("[MQTT] Publishing to autodiscovery topic ");
        Serial.println(discoveryTopic);
        Serial.print("[MQTT] Payload: ");
        Serial.println(payload);

        mqttClient.publish(discoveryTopic.c_str(), payload.c_str(), true);
    }

    Serial.println("Home Assistant auto-discovery published for all columns");
}

bool MQTTClient::isConnected() {
    return mqttClient.connected();
}

String MQTTClient::normalizeHeader(const String& h) {
    String out;
    out.reserve(h.length());

    for (size_t i = 0; i < h.length(); i++) {
        char c = h[i];
        if (isalnum(c)) {
            out += (char)tolower(c);
        } else if (c == ' ' || c == '_' || c == '-') {
            out += '_';
        }
        // drop everything else: (), %, °, etc.
    }

    // Remove duplicate underscores
    while (out.indexOf("__") >= 0) {
        out.replace("__", "_");
    }

    // Trim leading/trailing underscores
    if (out.startsWith("_")) out.remove(0, 1);
    if (out.endsWith("_")) out.remove(out.length() - 1);

    // Serial.printf("Header converted %s -> %s\n", h.c_str(), out.c_str());
    return out;
}

String MQTTClient::extractUnit(const String& h) {
    int start = h.indexOf('(');
    int end = h.indexOf(')');
    if (start >= 0 && end > start) {
        String unit = h.substring(start + 1, end);
        // Replace ` with ° for degrees
        unit.replace("`", "°");
        return unit;
    }
    return "";  // No unit found
}

String MQTTClient::extractName(const String& h) {
    int start = h.indexOf('(');
    if (start >= 0) {
        String s = h.substring(0, start);
        s.trim();
        return s;
    }
    return h;
}

String MQTTClient::deviceClassFromUnit(const String& unit) {
    if (unit == "°C") return "temperature";
    if (unit == "km") return "distance";
    if (unit == "g") return "weight";
    if (unit == "min") return "duration";
    if (unit == "%") return "humidity";  // optional mapping
    if (unit == "hPa") return "pressure";
    if (unit == "mg/str") return "weight";  // approximate
    if (unit == "W") return "power";
    if (unit == "kWh") return "energy";
    return "";  // default: no device_class
}

void MQTTClient::sendLogFile(const String& path) {
    // publishState();

    // First line = headers
    String headerLine = Storage::readLine(path);
    headerLine.trim();
    if (headerLine.length() == 0) return;

    // Split headers
    std::vector<String> headers;
    int start = 0, end;
    while ((end = headerLine.indexOf('\t', start)) != -1) {
        headers.push_back(headerLine.substring(start, end));
        start = end + 1;
    }
    headers.push_back(headerLine.substring(start));

    // Publish auto-discovery for Home Assistant
    publishAutoDiscovery(headers);

    // Read data lines
    while (true) {
        String line = Storage::readLine(path);
        line.trim();
        if (line.length() == 0) break;

        // Split columns
        std::vector<String> values;
        start = 0;
        while ((end = line.indexOf('\t', start)) != -1) {
            values.push_back(line.substring(start, end));
            start = end + 1;
        }
        values.push_back(line.substring(start));

        JsonDocument doc;
        for (size_t i = 0; i < headers.size(); i++) {
            const String sensorName = "dpf_" + normalizeHeader(extractName(headers[i]));
            const float value = (i < values.size()) ? values[i].toFloat() : 0;

            doc[sensorName.c_str()] = value;
        }

        String payload;
        serializeJson(doc, payload);

        // --- Debug log before publishing ---
        Serial.print("[MQTT] Publishing to topic ");
        Serial.println(STATE_TOPIC);
        Serial.print("[MQTT] Payload: ");
        Serial.println(payload);

        mqttClient.publish(STATE_TOPIC.c_str(), payload.c_str(), true);

        // Flush buffer
        loop();
    }

    Serial.println("Finished sending log file to MQTT");
}

bool MQTTClient::loop() {
    return mqttClient.loop();
}