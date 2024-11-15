#include "server.h"

#include "measurement.h"
#include "storage.h"

AsyncWebServer WifiServer::m_server(80);

const String WifiServer::m_index = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DPF indicator</title>
  <style>
    html {font-family: monospace; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {margin:0px auto; padding-bottom: 25px;}
  </style>
</head>
<body>
  <h2>DPF indicator log table</h2>
  %TABLEPLACEHOLDER%
  <br></br>
  <br></br>
  <a href=deletelog>Delete the log file</a>
<script>
</script>
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
            AsyncWebParameter* p = request->getParam(i);
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

    m_up = true;
    return true;
}

bool WifiServer::hasClient() {
    return m_connected;
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
    // Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            m_connected = true;
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            m_connected = false;
            break;
        default:
            break;
    }
}
