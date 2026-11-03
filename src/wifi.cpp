#include "wifi.h"

#include "config.h"
#include "measurement.h"
#include "storage.h"

AsyncWebServer WifiServer::m_server(80);

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
  <a href="deletelog">Delete the log file</a><br><br>

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
    Serial.printf("Local IP address: %s\n", ip.toString());

    m_server.begin();

    // Route for root / web page
    m_server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", WifiServer::m_index.c_str(), WifiServer::table);
    });

    m_server.on("/delete", HTTP_GET, [](AsyncWebServerRequest* request) {
        size_t params = request->params();

        Serial.printf("%d params sent in\n", params);
        for (size_t i = 0; i < params; i++) {
            const AsyncWebParameter* p = request->getParam(i);
            Serial.printf("_GET[%s]: %s", p->name().c_str(), p->value().c_str());
        }
        String logid = "[]";
        if (request->hasParam("id")) {
            logid = request->getParam("id")->value();
            Storage::deleteLine(Measurements::MEASUREMENTS_LOG, logid.toInt() + 1);
        }

        String redirect =
            "<html>"
            "<head>"
            "<meta http-equiv=\"refresh\" content=\"2;url=/\" />"
            "</head>"
            "<body>"
            "Record " +
            logid +
            " deleted..."
            "</body>"
            "</html>";

        request->send_P(200, "text/html", redirect.c_str(), NULL);
    });

    m_server.on("/deletelog", HTTP_GET, [](AsyncWebServerRequest* request) {
        Storage::remove(Measurements::MEASUREMENTS_LOG);

        String redirect =
            "<html>"
            "<head>"
            "<meta http-equiv=\"refresh\" content=\"2;url=/\" />"
            "</head>"
            "<body>"
            "Log file deleted..."
            "</body>"
            "</html>";

        request->send_P(200, "text/html", redirect.c_str(), NULL);
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

bool WifiServer::getIP() {
    return WiFi.softAPIP().toString();
}

void WifiServer::loop() {}

String WifiServer::table(const String& var) {
    if (var == "TABLEPLACEHOLDER") {
        String table = "<table>";
        size_t i = 0;
        while (true) {
            String line = Storage::readLine(Measurements::MEASUREMENTS_LOG);

            if (line.isEmpty()) break;

            if (line.startsWith("1")) {
                // Regeneration on
                line.replace("\t", "</td><td>");
                table += "<tr style=\"background-color:#CCCC00\"><td>" + String(i) + "</td><td>" + line +
                         "</td><td><a href=\"delete?id=" + i + "\">delete</a></td></tr>\n";
                i++;
            } else if (line.startsWith("0")) {
                // Regeneration off
                line.replace("\t", "</td><td>");
                table += "<tr style=\"background-color:#66CC00\"><td>" + String(i) + "</td><td>" + line +
                         "</td><td><a href=\"delete?id=" + i + "\">delete</a></td></tr>\n";
                i++;
            } else if (table.length() == strlen("<table>")) {
                // Header line
                line.replace("\t", "</th><th>");
                table += "<tr><th>#</th><th>" + line + "</th><th></th></tr>\n";
            } else {
                // Other line
                line.replace("\t", "</td><td>");
                table += "<tr><td></td><td>" + line + "</td><td></td></tr>\n";
            }
        }
        table += "</table>";
        return table;
    }
    return String();
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
        attempts++;
    }
    Serial.println("\nWiFi client init end");
    return isConnected();
}

bool WifiClient::isConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Wi-Fi client connected (%s)\n", WiFi.localIP().toString().c_str());
        return true;
    }
    return false;
}

bool WifiClient::getIP() {
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