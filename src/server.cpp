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

WifiServer::WifiServer() {}

bool WifiServer::init(const String& ssid, const String& password) {
    Serial.println("Setting the wifi AP");

    WiFi.mode(WIFI_AP);
    // Remove the password parameter, if you want the AP to be open
    WiFi.softAP(ssid, password);
    WiFi.softAPsetHostname(ssid.c_str());

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("Local IP address: %s\n", ip.toString());

    m_server.begin();

    // Route for root / web page
    m_server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", WifiServer::m_index.c_str(), WifiServer::table);
    });

    m_server.on("/deletelog", HTTP_GET, [](AsyncWebServerRequest* request) {
        Storage::remove(Measurements::MEASUREMENTS_LOG);

        String redirect =
            "<html>"
            "<head>"
            "<meta http-equiv=\"refresh\" content=\"3;url=/\" />"
            "</head>"
            "<body>"
            "Log deleted..."
            "</body>"
            "</html>";

        request->send_P(200, "text/html", redirect.c_str(), NULL);
    });

    return true;
}

bool WifiServer::hasClient() {
    return WiFi.softAPgetStationNum() > 0;
}

void WifiServer::loop() {}

String WifiServer::table(const String& var) {
    if (var == "TABLEPLACEHOLDER") {
        String table = "<table>";
        while (true) {
            String line = Storage::readLine(Measurements::MEASUREMENTS_LOG);

            if (line.isEmpty()) break;

            line.replace("\t", "</td><td>");
            table += "<tr><td>" + line + "</td></tr>\n";
        }
        table += "</table>";
        return table;
    }
    return String();
}