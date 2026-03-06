#include "Gypsy.h"

// ---------------------------------------------------------------
//  Static instance pointer (needed for static route trampolines)
// ---------------------------------------------------------------
Gypsy* Gypsy::_instance = nullptr;

// ---------------------------------------------------------------
//  Constructor
// ---------------------------------------------------------------
Gypsy::Gypsy(const char* ssid,
             const char* password,
             const char* deviceId,
             const char* authToken)
    : _ssid(ssid)
    , _password(password)
    , _deviceId(deviceId)
    , _authToken(authToken)
    , _server(80)
    , _sensorCb(nullptr)
    , _loopCb(nullptr)
    , _lastLog(0)
{
    _instance = this;
}

// ---------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------
void Gypsy::registerSensor(GypsynSensorCallback cb) {
    _sensorCb = cb;
}

void Gypsy::onLoop(void (*cb)()) {
    _loopCb = cb;
}

void Gypsy::begin() {
    Serial.begin(115200);
    Serial.printf("\n[GYPSY] Device: %s  FW: %s\n", _deviceId, GYPSY_FW_VERSION);

    _connectWiFi();
    _setupOTA();

    // HTTP OTA endpoint
    _updater.setup(&_server, "/ota/update");

    _setupRoutes();
    _server.begin();
    Serial.println("[GYPSY] HTTP server started");
}

void Gypsy::handle() {
    ArduinoOTA.handle();
    MDNS.update();
    _server.handleClient();

    if (_loopCb) _loopCb();

    // Serial log every 3 s
    if (millis() - _lastLog > 3000) {
        _lastLog = millis();

        if (_sensorCb) {
            SensorReading readings[GYPSY_MAX_SENSORS];
            int count = _sensorCb(readings, GYPSY_MAX_SENSORS);

            Serial.printf("[GYPSY] Up=%lus  Heap=%u  WiFi=%s\n",
                millis() / 1000, ESP.getFreeHeap(),
                connected() ? "OK" : "LOST");

            for (int i = 0; i < count; i++) {
                Serial.printf("  %-16s %.1f %s\n",
                    readings[i].name, readings[i].value, readings[i].unit);
            }
        } else {
            Serial.printf("[GYPSY] Up=%lus  Heap=%u  (no sensors registered)\n",
                millis() / 1000, ESP.getFreeHeap());
        }
    }
}

// ---------------------------------------------------------------
//  Private – WiFi
// ---------------------------------------------------------------
void Gypsy::_connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    Serial.printf("[GYPSY] Connecting to \"%s\"", _ssid);

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
        delay(500);
        Serial.print(".");
        tries++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[GYPSY] WiFi failed – restarting in 2 s");
        delay(2000);
        ESP.restart();
    }

    Serial.printf("\n[GYPSY] IP: %s\n", WiFi.localIP().toString().c_str());

    if (MDNS.begin(_deviceId)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[GYPSY] mDNS: http://%s.local\n", _deviceId);
    }
}

// ---------------------------------------------------------------
//  Private – OTA
// ---------------------------------------------------------------
void Gypsy::_setupOTA() {
    ArduinoOTA.setHostname(_deviceId);
    ArduinoOTA.setPassword(_authToken);   // reuse auth token as OTA password

    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Starting update...");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Done.");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] %u%%\r", progress * 100 / total);
    });
    ArduinoOTA.onError([](ota_error_t err) {
        Serial.printf("[OTA] Error[%u]\n", err);
    });

    ArduinoOTA.begin();
    Serial.println("[GYPSY] OTA ready");
}

// ---------------------------------------------------------------
//  Private – routes
// ---------------------------------------------------------------
void Gypsy::_setupRoutes() {
    _server.on("/",           HTTP_GET,     _hRoot);
    _server.on("/ping",       HTTP_GET,     _hPing);
    _server.on("/data",       HTTP_GET,     _hData);
    _server.on("/info",       HTTP_GET,     _hInfo);
    _server.on("/ota/update", HTTP_OPTIONS, _hOptions);
    _server.onNotFound(_h404);
}

void Gypsy::_addCORS() {
    _server.sendHeader("Access-Control-Allow-Origin",  "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Gypsy-Token");
}

// ---------------------------------------------------------------
//  Private – JSON builders
// ---------------------------------------------------------------
String Gypsy::_buildPingJson() {
    JsonDocument doc;
    doc["id"]      = _deviceId;
    doc["version"] = GYPSY_FW_VERSION;
    doc["uptime"]  = millis() / 1000;
    String out;
    serializeJson(doc, out);
    return out;
}

String Gypsy::_buildDataJson() {
    JsonDocument doc;
    doc["device_id"] = _deviceId;
    doc["timestamp"] = millis();
    doc["uptime"]    = millis() / 1000;

    JsonObject data = doc["data"].to<JsonObject>();

    if (_sensorCb) {
        SensorReading readings[GYPSY_MAX_SENSORS];
        int count = _sensorCb(readings, GYPSY_MAX_SENSORS);

        for (int i = 0; i < count; i++) {
            JsonObject obj = data[readings[i].name].to<JsonObject>();
            obj["value"] = round(readings[i].value * 10) / 10.0;
            obj["unit"]  = readings[i].unit;
            obj["min"]   = readings[i].minVal;
            obj["max"]   = readings[i].maxVal;
        }
    }

    String out;
    serializeJson(doc, out);
    return out;
}

String Gypsy::_buildInfoJson() {
    JsonDocument doc;
    doc["device_id"]  = _deviceId;
    doc["version"]    = GYPSY_FW_VERSION;
    doc["uptime"]     = millis() / 1000;
    doc["chip_id"]    = String(ESP.getChipId(), HEX);
    doc["flash_size"] = ESP.getFlashChipSize();
    doc["free_heap"]  = ESP.getFreeHeap();
    doc["ssid"]       = WiFi.SSID();
    // NOTE: auth token intentionally NOT included here for security
    String out;
    serializeJson(doc, out);
    return out;
}

// ---------------------------------------------------------------
//  Static trampolines → instance methods
// ---------------------------------------------------------------
void Gypsy::_hRoot() {
    _instance->_addCORS();
    String html =
        "<html><body style='font-family:monospace;background:#07080f;"
        "color:#00ffd5;padding:30px'>"
        "<h2>GYPSY NODE</h2>"
        "<p>Device: " + String(_instance->_deviceId) + "</p>"
        "<p>Firmware v" + String(GYPSY_FW_VERSION) + "</p>"
        "<p><a href='/data'  style='color:#00ffd5'>/data</a></p>"
        "<p><a href='/info'  style='color:#00ffd5'>/info</a></p>"
        "<p><a href='/ping'  style='color:#00ffd5'>/ping</a></p>"
        "</body></html>";
    _instance->_server.send(200, "text/html", html);
}

void Gypsy::_hPing() {
    _instance->_addCORS();
    _instance->_server.send(200, "application/json", _instance->_buildPingJson());
}

void Gypsy::_hData() {
    _instance->_addCORS();
    _instance->_server.send(200, "application/json", _instance->_buildDataJson());
}

void Gypsy::_hInfo() {
    _instance->_addCORS();
    _instance->_server.send(200, "application/json", _instance->_buildInfoJson());
}

void Gypsy::_hOptions() {
    _instance->_addCORS();
    _instance->_server.send(204);
}

void Gypsy::_h404() {
    _instance->_addCORS();
    _instance->_server.send(404, "application/json", "{\"error\":\"not found\"}");
}
