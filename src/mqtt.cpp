#include "mqtt.h"

#include <LittleFS.h>

#include "storage.h"
#include "wifi.h"

WiFiClient MQTTClient::espClient;
PubSubClient MQTTClient::mqttClient(MQTTClient::espClient);
const String MQTTClient::DEVICE_ID = "touran";
const String MQTTClient::NODE_ID = "touran/dpf";
const String MQTTClient::AVAILIBILITY_TOPIC = NODE_ID + "/status";
const String MQTTClient::STATE_TOPIC = NODE_ID + "/state";
const String MQTTClient::LOG_TOPIC = NODE_ID + "/log";
const String MQTTClient::UPLOAD_TOPIC = NODE_ID + "/upload";

// Upload confirmation (marker echoed back by the broker)
static String s_uploadMarker;
static volatile bool s_uploadConfirmed = false;

bool MQTTClient::connect() {
    Serial.printf("[MQTT] Connecting to the MQTT server %s:%d", mqttcfg.server.c_str(), mqttcfg.port);
    mqttClient.setServer(mqttcfg.server.c_str(), mqttcfg.port);
    // Small messages go through the buffer, large ones (discovery, log file) are streamed
    mqttClient.setBufferSize(750);
    mqttClient.setCallback(MQTTClient::onMessage);

    // Empty credentials -> anonymous connection
    const char* user = mqttcfg.username.isEmpty() ? nullptr : mqttcfg.username.c_str();
    const char* pass = mqttcfg.password.isEmpty() ? nullptr : mqttcfg.password.c_str();

    int mqttAttempts = 0;
    while (!mqttClient.connected() && mqttAttempts < 10) {
        if (mqttClient.connect(DEVICE_ID.c_str(), user, pass,
                               AVAILIBILITY_TOPIC.c_str(), 0, true, "offline"))
            break;
        Serial.print(".");
        delay(500);
        mqttAttempts++;
    }
    Serial.println();

    if (!mqttClient.connected()) {
        Serial.printf("[MQTT] connection failed (state=%d), skipping send\n", mqttClient.state());
        return false;
    }

    mqttClient.publish(AVAILIBILITY_TOPIC.c_str(), "online", true);

    Serial.println("[MQTT] connected");
    return true;
}

bool MQTTClient::disconnect() {
    if (mqttClient.connected()) {
        // A clean disconnect does not trigger the LWT, so publish "offline" explicitly
        mqttClient.publish(AVAILIBILITY_TOPIC.c_str(), "offline", true);
        mqttClient.loop();
    }
    mqttClient.disconnect();
    return true;
}

bool MQTTClient::isConnected() {
    return mqttClient.connected();
}

bool MQTTClient::loop() {
    return mqttClient.loop();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::vector<String> MQTTClient::split(const String& line) {
    std::vector<String> out;
    int start = 0, end;
    while ((end = line.indexOf('\t', start)) != -1) {
        out.push_back(line.substring(start, end));
        start = end + 1;
    }
    out.push_back(line.substring(start));
    for (auto& s : out) s.trim();
    return out;
}

void MQTTClient::addDevice(JsonDocument& doc) {
    JsonObject device = doc["device"].to<JsonObject>();
    device["name"] = "Touran - DPF indicator";
    device["identifiers"].to<JsonArray>().add("touran_dpf_01");
    device["model"] = "Touran";
    device["manufacturer"] = "VW";
    device["configuration_url"] = String("http://") + WifiClient::getIP();
    device["hw_version"] = ESP.getChipModel();
}

// Publish JSON of any size - streamed, so it is not limited by the client buffer
bool MQTTClient::publishJson(const String& topic, const JsonDocument& doc, bool retained) {
    if (!mqttClient.beginPublish(topic.c_str(), measureJson(doc), retained)) return false;
    serializeJson(doc, mqttClient);
    const bool ok = mqttClient.endPublish();
    if (!ok) Serial.printf("[MQTT] Publish to %s failed (state=%d)\n", topic.c_str(), mqttClient.state());
    return ok;
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
    if (unit == "hPa") return "pressure";
    if (unit == "W") return "power";
    if (unit == "kWh") return "energy";
    return "";  // default: no device_class
}

// ---------------------------------------------------------------------------
// Home Assistant discovery
// ---------------------------------------------------------------------------

bool MQTTClient::publishAutoDiscovery(const std::vector<String>& headers) {
    bool ok = true;
    // Home Assistant expects topic: homeassistant/sensor/<node>/<object>/config
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
        if (unit != "") doc["unit_of_measurement"] = unit;
        if (devClass != "") doc["device_class"] = devClass;
        if (sensorName == "dpf_distance_driven") {
            // Cumulative value - enables correct long-term statistics
            doc["state_class"] = "total_increasing";
        } else if (unit != "") {
            doc["state_class"] = "measurement";
        }
        if (sensorName == "dpf_soot_mass_calculated" ||
            sensorName == "dpf_soot_mass_measured" ||
            sensorName == "dpf_oil_ash_residue") {
            doc["suggested_display_precision"] = 2;
        }
        addDevice(doc);

        ok &= publishJson(discoveryTopic, doc, true);
        mqttClient.loop();
    }

    Serial.printf("[MQTT] HASS sensor discovery published (%u columns)\n", headers.size());
    return ok;
}

bool MQTTClient::publishStatusDiscovery(const std::vector<String>& headers, const std::vector<String>& sample) {
    bool ok = true;

    // 1) Log table sensor. State = number of data rows, attribute "rows" = newest
    //    HA_LOG_ROWS rows (newest first). The parsing template is generated from
    //    the file header, so adding/reordering columns needs no change in HA.
    String tpl =
        "{% set lines = value.strip().splitlines()[1:][-" + String(HA_LOG_ROWS) + ":] %}"
        "{% set ns = namespace(rows=[]) %}"
        "{% for l in lines | reverse %}"
        "{% set c = l.split() %}"
        "{% if c | count >= " + String(headers.size()) + " %}"
        "{% set ns.rows = ns.rows + [{";
    for (size_t i = 0; i < headers.size(); i++) {
        // Integer column when the sample value has no decimal point
        const bool isInt = i < sample.size() && sample[i].indexOf('.') < 0;
        if (i) tpl += ",";
        tpl += "'" + normalizeHeader(extractName(headers[i])) + "':c[" + String(i) + "]|" + (isInt ? "int(0)" : "float(0)");
    }
    tpl += "}] %}{% endif %}{% endfor %}{{ {'rows': ns.rows} | tojson }}";

    {
        JsonDocument doc;
        doc["name"] = "Log";
        doc["unique_id"] = "dpf_log";
        doc["state_topic"] = LOG_TOPIC;
        doc["value_template"] = "{{ value.strip().splitlines() | count - 1 }}";
        doc["unit_of_measurement"] = "rows";
        doc["json_attributes_topic"] = LOG_TOPIC;
        doc["json_attributes_template"] = tpl;
        doc["icon"] = "mdi:table";
        addDevice(doc);
        ok &= publishJson("homeassistant/sensor/" + DEVICE_ID + "/dpf_log/config", doc, true);
        mqttClient.loop();
    }

    // 2) Regeneration active (taken from the last row)
    {
        JsonDocument doc;
        doc["name"] = "Regeneration active";
        doc["unique_id"] = "dpf_regeneration_active";
        doc["state_topic"] = STATE_TOPIC;
        doc["value_template"] = "{{ 'ON' if value_json.dpf_regeneration | int(0) == 1 else 'OFF' }}";
        doc["icon"] = "mdi:fire";
        addDevice(doc);
        ok &= publishJson("homeassistant/binary_sensor/" + DEVICE_ID + "/dpf_regeneration_active/config", doc, true);
        mqttClient.loop();
    }

    // 3) Device connected to the broker (online/offline + LWT)
    {
        JsonDocument doc;
        doc["name"] = "Connected";
        doc["unique_id"] = "dpf_connected";
        doc["state_topic"] = AVAILIBILITY_TOPIC;
        doc["payload_on"] = "online";
        doc["payload_off"] = "offline";
        doc["device_class"] = "connectivity";
        doc["entity_category"] = "diagnostic";
        addDevice(doc);
        ok &= publishJson("homeassistant/binary_sensor/" + DEVICE_ID + "/dpf_connected/config", doc, true);
        mqttClient.loop();
    }

    // 4) Last confirmed upload - HA stamps the time when the marker arrives
    {
        JsonDocument doc;
        doc["name"] = "Last upload";
        doc["unique_id"] = "dpf_last_upload";
        doc["state_topic"] = UPLOAD_TOPIC;
        doc["value_template"] = "{{ now().isoformat() }}";
        doc["device_class"] = "timestamp";
        doc["entity_category"] = "diagnostic";
        addDevice(doc);
        ok &= publishJson("homeassistant/sensor/" + DEVICE_ID + "/dpf_last_upload/config", doc, true);
        mqttClient.loop();
    }

    Serial.println("[MQTT] HASS status discovery published");
    return ok;
}

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

bool MQTTClient::publishLastRow(const std::vector<String>& headers, const String& line) {
    const std::vector<String> values = split(line);

    JsonDocument doc;
    for (size_t i = 0; i < headers.size(); i++) {
        const String sensorName = "dpf_" + normalizeHeader(extractName(headers[i]));
        doc[sensorName] = (i < values.size()) ? values[i].toFloat() : 0;
    }

    Serial.print("[MQTT] Last row: ");
    serializeJson(doc, Serial);
    Serial.println();

    return publishJson(STATE_TOPIC, doc, true);
}

// Stream the whole log file as one retained message
bool MQTTClient::publishLogFile(const String& path) {
    File f = LittleFS.open(path, "r");
    if (!f || f.isDirectory()) return false;

    const size_t size = f.size();
    if (!mqttClient.beginPublish(LOG_TOPIC.c_str(), size, true)) {
        f.close();
        return false;
    }

    uint8_t buf[256];
    size_t written = 0;
    while (f.available()) {
        const size_t n = f.read(buf, sizeof(buf));
        written += mqttClient.write(buf, n);
    }
    f.close();

    const bool ok = mqttClient.endPublish() && written == size;
    Serial.printf("[MQTT] Log file published (%u/%u B) %s\n", written, size, ok ? "OK" : "FAILED");
    return ok;
}

void MQTTClient::onMessage(char* topic, byte* payload, unsigned int length) {
    if (UPLOAD_TOPIC == topic &&
        length == s_uploadMarker.length() &&
        memcmp(payload, s_uploadMarker.c_str(), length) == 0) {
        s_uploadConfirmed = true;
    }
}

// Publish a unique marker and wait until the broker sends it back.
// Messages from one client are delivered in order, so the echo proves
// that everything published before it reached the broker.
bool MQTTClient::confirmDelivery(uint32_t timeoutMs) {
    s_uploadMarker = String(esp_random(), HEX);
    s_uploadConfirmed = false;

    if (!mqttClient.subscribe(UPLOAD_TOPIC.c_str())) return false;
    mqttClient.publish(UPLOAD_TOPIC.c_str(), s_uploadMarker.c_str(), false);

    const uint32_t start = millis();
    while (!s_uploadConfirmed && millis() - start < timeoutMs && mqttClient.connected()) {
        mqttClient.loop();
        delay(10);
    }
    mqttClient.unsubscribe(UPLOAD_TOPIC.c_str());
    return s_uploadConfirmed;
}

bool MQTTClient::sendLogFile(const String& path) {
    if (!mqttClient.connected()) return false;

    // Read the header and the last non-empty data line
    File f = LittleFS.open(path, "r");
    if (!f || f.isDirectory()) {
        Serial.println(F("[MQTT] Log file not found"));
        return false;
    }
    String headerLine = f.readStringUntil('\n');
    headerLine.trim();
    String lastLine;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length()) lastLine = line;
    }
    f.close();

    if (headerLine.isEmpty()) return false;

    const std::vector<String> headers = split(headerLine);
    const std::vector<String> sample = split(lastLine);

    bool ok = publishAutoDiscovery(headers);
    ok = publishStatusDiscovery(headers, sample) && ok;
    if (lastLine.length()) ok = publishLastRow(headers, lastLine) && ok;
    ok = publishLogFile(path) && ok;

    const bool confirmed = ok && confirmDelivery(3000);
    Serial.printf("[MQTT] Upload %s\n", confirmed ? "CONFIRMED by broker" : "FAILED");
    return confirmed;
}
