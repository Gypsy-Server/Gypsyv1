#pragma once
#ifndef GYPSY_H
#define GYPSY_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------
//  GYPSY  –  ESP8266 sensor-node framework
//  Version 1.0.0
//
//  What the library handles (you don't touch these):
//    • WiFi connect / auto-restart on failure
//    • mDNS  (<device_id>.local)
//    • ArduinoOTA  (password = auth token)
//    • HTTP OTA endpoint  POST /ota/update
//    • CORS headers on every response
//    • Routes: GET /  /ping  /data  /info  404
//    • Serial loop logging every 3 s
//
//  What YOU provide:
//    • SSID / password
//    • Device ID  (becomes the mDNS hostname)
//    • Auth token
//    • Sensor readings  – via registerSensor() callback
// ---------------------------------------------------------------

// Maximum number of sensors you can register
#ifndef GYPSY_MAX_SENSORS
#define GYPSY_MAX_SENSORS 8
#endif

// Firmware version reported in /info and /ping
#ifndef GYPSY_FW_VERSION
#define GYPSY_FW_VERSION "1.0.0"
#endif

// ------------------------------------------------------------------
//  SensorReading  –  one named value + metadata
// ------------------------------------------------------------------
struct SensorReading {
    const char* name;   // e.g. "temperature"
    float       value;
    const char* unit;   // e.g. "C"
    float       minVal;
    float       maxVal;
};

// ------------------------------------------------------------------
//  Callback type the user implements to fill in fresh readings.
//  The library passes an array and the max count; the callback
//  must return how many entries it actually filled.
//
//  Example:
//    int myRead(SensorReading* out, int maxCount) {
//        out[0] = { "temperature", dht.readTemperature(), "C", -10, 60 };
//        out[1] = { "humidity",    dht.readHumidity(),    "%",  0, 100 };
//        return 2;
//    }
// ------------------------------------------------------------------
typedef int (*GypsynSensorCallback)(SensorReading* out, int maxCount);

// ------------------------------------------------------------------
//  Gypsy  –  main class
// ------------------------------------------------------------------
class Gypsy {
public:
    // ---- constructor --------------------------------------------
    //  ssid        – WiFi network name
    //  password    – WiFi password
    //  deviceId    – unique node name; also used as mDNS hostname
    //  authToken   – bearer token checked on /info; also OTA password
    Gypsy(const char* ssid,
          const char* password,
          const char* deviceId,
          const char* authToken = "changeme");

    // ---- user API -----------------------------------------------

    // Register a callback that reads your sensors.
    // Call once in setup() before begin().
    void registerSensor(GypsynSensorCallback cb);

    // Optional: called every loop iteration (your sensor init,
    // display refresh, relay logic, etc.)
    void onLoop(void (*cb)());

    // Start everything (WiFi, mDNS, OTA, HTTP server).
    // Call at the end of setup().
    void begin();

    // Must be called every loop().
    void handle();

    // ---- accessors (read-only) ----------------------------------
    const char* deviceId()   const { return _deviceId; }
    const char* fwVersion()  const { return GYPSY_FW_VERSION; }
    bool        connected()  const { return WiFi.status() == WL_CONNECTED; }

private:
    // config
    const char* _ssid;
    const char* _password;
    const char* _deviceId;
    const char* _authToken;

    // internals
    ESP8266WebServer        _server;
    ESP8266HTTPUpdateServer _updater;

    GypsynSensorCallback _sensorCb;
    void (*_loopCb)();

    unsigned long _lastLog;

    // helpers
    void _connectWiFi();
    void _setupOTA();
    void _setupRoutes();
    void _addCORS();

    // JSON builders
    String _buildPingJson();
    String _buildDataJson();
    String _buildInfoJson();

    // route handlers (static trampolines → instance methods)
    static Gypsy* _instance;
    static void _hRoot();
    static void _hPing();
    static void _hData();
    static void _hInfo();
    static void _hOptions();
    static void _h404();
};

#endif // GYPSY_H
