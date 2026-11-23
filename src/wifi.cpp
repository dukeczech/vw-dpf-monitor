#include "wifi.h"

#include "config.h"
#include "measurement.h"
#include "storage.h"

#include <esp_task_wdt.h>

#include <memory>

AsyncWebServer WifiServer::m_server(80);

static const char TABLE_PLACEHOLDER[] = "%TABLEPLACEHOLDER%";

// State of one streamed index page response
struct IndexStream {
    enum Stage { HEAD, ROWS, TAIL, DONE };

    Stage stage = HEAD;
    File file;
    String pending;      // generated, not yet sent part of the page
    size_t pos = 0;      // how much of pending has been sent
    size_t row = 0;      // log record number shown in the table
    bool header = false; // table header row already generated
};

const String WifiServer::m_index = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>DPF indicator</title>
  <style>
    html {font-family: monospace; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {margin:0px auto; padding-bottom: 25px;}
    .config-link {
      font-size: 0.8rem;
      color: #888;
      text-decoration: none;
    }
    .config-link:hover {
      color: #444;
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <h2>DPF indicator log table</h2>
  %TABLEPLACEHOLDER%
  <br></br>
  <br></br>
  <a href="download">Download the log file</a><br><br>
  <a href="#" onclick="return delLog()">Delete the log file</a><br><br>

  <script>
    // Deleting is a POST request after a confirmation, so a stray click or a link
    // prefetch cannot delete the data
    function post(url, body) {
      fetch(url, {method: 'POST', body: body})
        .then(r => r.text().then(t => { alert(t); location.reload(); }))
        .catch(e => alert('Request failed: ' + e));
      return false;
    }
    function del(id) {
      if (!confirm('Delete record ' + id + '?')) return false;
      return post('delete', new URLSearchParams({id: id}));
    }
    function delLog() {
      if (!confirm('Delete the whole log file? This cannot be undone.')) return false;
      return post('deletelog');
    }
  </script>

  <div style="margin-top:40px;">
    <a class="config-link" href="/config">configuration</a>
  </div>
</body>
</html>
)rawliteral";

const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>MQTT + Wi-Fi Config</title>
</head>
<body>
  <h2>Configuration</h2>
  <form action="/config/save" method="post">

  <h3>MQTT Settings</h3>
Server:<br><input name="server" value="%SERVER%"><br>
Port:<br><input name="port" value="%PORT%"><br>
Username:<br><input name="user" value="%USER%"><br>
Password:<br><input name="pass" type="password" value="%PASS%"><br>
Topic:<br><input name="topic" value="%TOPIC%"><br><br>

  <h3>Wi-Fi Settings</h3>
SSID:<br><input name="ssid" value="%WIFI_SSID%"><br>
Password:<br><input name="wifipass" type="password" value="%WIFI_PASS%"><br><br>

<input type="submit" value="Save">
  </form>
</body>
</html>
)rawliteral";

bool WifiServer::m_up = false;
bool WifiServer::m_connected = false;

WifiServer::WifiServer() {}

bool WifiServer::init(const String& ssid, const String& password) {
    Serial.println("Setting the wifi AP");

    WiFi.mode(WIFI_AP);
    // Remove the password parameter, if you want the AP to be open
    WiFi.softAP(ssid, password);
    WiFi.softAPsetHostname(ssid.c_str());
    WiFi.onEvent(WifiServer::wifiEvent);

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("Local IP address: %s\n", ip.toString().c_str());

    m_server.begin();

    // Route for root / web page
    m_server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // The state is shared with the filler and released (file closed) when the response ends
        auto stream = std::make_shared<IndexStream>();

        AsyncWebServerResponse* response = request->beginChunkedResponse(
            "text/html", [stream](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                size_t written = 0;
                while (written < maxLen) {
                    if (stream->pos >= stream->pending.length()) {
                        // Everything generated so far is sent, generate the next part
                        stream->pending = "";
                        stream->pos = 0;
                        if (!WifiServer::nextIndexChunk(*stream)) break;
                        continue;
                    }
                    const size_t len = min(maxLen - written, stream->pending.length() - stream->pos);
                    memcpy(buffer + written, stream->pending.c_str() + stream->pos, len);
                    written += len;
                    stream->pos += len;
                }
                // Returning 0 ends the response
                return written;
            });
        request->send(response);
    });

    // Deleting is POST only (sent by the index page after a confirmation)
    m_server.on("/delete", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("id", true)) {
            request->send(400, "text/plain", "Missing record id");
            return;
        }

        const String logid = request->getParam("id", true)->value();
        const long id = logid.toInt();
        if (id < 0 || (id == 0 && logid != "0")) {
            request->send(400, "text/plain", "Invalid record id");
            return;
        }

        // Line 0 is the table heading
        if (Storage::deleteLine(Measurements::MEASUREMENTS_LOG, id + 1)) {
            request->send(200, "text/plain", "Record " + logid + " deleted");
        } else {
            request->send(500, "text/plain", "Failed to delete record " + logid);
        }
    });

    m_server.on("/deletelog", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (Storage::remove(Measurements::MEASUREMENTS_LOG)) {
            request->send(200, "text/plain", "Log file deleted");
        } else {
            request->send(500, "text/plain", "Failed to delete the log file");
        }
    });

    m_server.serveStatic("/download", LittleFS, Measurements::MEASUREMENTS_LOG.c_str());

    WifiServer::m_server.on("/config", HTTP_GET,
                            [](AsyncWebServerRequest* request) {
                                // Load all the variables from the persist storage
                                ApplicationConfig::load();

                                String html = CONFIG_HTML;  // copy from PROGMEM to RAM
                                html.replace("%SERVER%", mqttcfg.server);
                                html.replace("%PORT%", String(mqttcfg.port));
                                html.replace("%USER%", mqttcfg.username);
                                html.replace("%PASS%", mqttcfg.password);
                                html.replace("%TOPIC%", mqttcfg.topic);

                                html.replace("%WIFI_SSID%", wificfg.ssid);
                                html.replace("%WIFI_PASS%", wificfg.password);

                                request->send(200, "text/html", html);
                            });

    WifiServer::m_server.on("/config/save", HTTP_POST,
                            [](AsyncWebServerRequest* request) {
                                // MQTT settings
                                if (request->hasParam("server", true)) mqttcfg.server = request->getParam("server", true)->value();
                                if (request->hasParam("port", true)) mqttcfg.port = request->getParam("port", true)->value().toInt();
                                if (request->hasParam("user", true)) mqttcfg.username = request->getParam("user", true)->value();
                                if (request->hasParam("pass", true)) mqttcfg.password = request->getParam("pass", true)->value();
                                if (request->hasParam("topic", true)) mqttcfg.topic = request->getParam("topic", true)->value();

                                // Wi-Fi settings
                                if (request->hasParam("ssid", true)) wificfg.ssid = request->getParam("ssid", true)->value();
                                if (request->hasParam("wifipass", true)) wificfg.password = request->getParam("wifipass", true)->value();

                                // Save the new values to the persistent storage
                                ApplicationConfig::save();

                                request->send(200, "text/html",
                                              "<html><body><h3>Saved. Rebooting…</h3></body></html>");

                                delay(1000);
                                ESP.restart();
                            });

    m_up = true;
    return true;
}

bool WifiServer::isUp() {
    return m_up;
}

bool WifiServer::hasClient() {
    return m_connected;
}

String WifiServer::getIP() {
    return WiFi.softAPIP().toString();
}

void WifiServer::loop() {}

bool WifiServer::nextIndexChunk(IndexStream& stream) {
    const int placeholder = m_index.indexOf(TABLE_PLACEHOLDER);

    switch (stream.stage) {
        case IndexStream::HEAD:
            // Page up to the table
            stream.pending = m_index.substring(0, placeholder) + "<table>\n";
            stream.file = LittleFS.open(Measurements::MEASUREMENTS_LOG);
            if (stream.file && stream.file.isDirectory()) stream.file.close();
            stream.stage = IndexStream::ROWS;
            return true;

        case IndexStream::ROWS:
            // One table row per log line
            if (stream.file && stream.file.available()) {
                String line = stream.file.readStringUntil('\n');
                if (line.endsWith("\r")) line.remove(line.length() - 1);
                if (!line.isEmpty()) stream.pending = tableRow(line, stream);
                return true;
            }
            if (stream.file) stream.file.close();
            stream.pending = "</table>\n";
            stream.stage = IndexStream::TAIL;
            return true;

        case IndexStream::TAIL:
            // Rest of the page after the table
            stream.pending = m_index.substring(placeholder + strlen(TABLE_PLACEHOLDER));
            stream.stage = IndexStream::DONE;
            return true;

        default:
            return false;
    }
}

String WifiServer::tableRow(String line, IndexStream& stream) {
    if (line.startsWith("1") || line.startsWith("0")) {
        // Regeneration on (yellow) / off (green)
        const char* color = line.startsWith("1") ? "#CCCC00" : "#66CC00";
        const String id(stream.row++);
        line.replace("\t", "</td><td>");
        return "<tr style=\"background-color:" + String(color) + "\"><td>" + id + "</td><td>" + line +
               "</td><td><a href=\"#\" onclick=\"return del(" + id + ")\">delete</a></td></tr>\n";
    }

    if (!stream.header) {
        // Header line
        stream.header = true;
        line.replace("\t", "</th><th>");
        return "<tr><th>#</th><th>" + line + "</th><th></th></tr>\n";
    }

    // Other line
    line.replace("\t", "</td><td>");
    return "<tr><td></td><td>" + line + "</td><td></td></tr>\n";
}

void WifiServer::wifiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            // Report connect on client IP assigned
            m_connected = true;
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            m_connected = false;
            break;
        default:
            break;
    }
}

WifiClient::WifiClient() {
}

bool WifiClient::init(const String& ssid, const String& password) {
    Serial.printf("Setting the wifi client to: %s\n", ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // 10s max
        delay(500);
        Serial.print(".");
        esp_task_wdt_reset();  // Keep the loop watchdog fed while waiting
        attempts++;
    }
    Serial.println("\nWiFi client init end");
    if (isConnected()) {
        Serial.printf("Wi-Fi client connected (%s)\n", WiFi.localIP().toString().c_str());
        return true;
    }
    return false;
}

bool WifiClient::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WifiClient::getIP() {
    return WiFi.localIP().toString();
}

void WifiClient::loop() {
}

bool WifiClient::isWifiAvailable(const String& ssid, int timeout) {
    int32_t start = millis();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);

    delay(100);

    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == ssid) return true;
    }
    return false;
}