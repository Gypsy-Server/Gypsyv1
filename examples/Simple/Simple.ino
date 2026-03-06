#include "Gypsy.h"

#define WIFI_SSID  "YourNetwork"
#define WIFI_PASS  "YourPassword"
#define DEVICE_ID  "GYPSY-A00e"
#define AUTH_TOKEN "changeme123"

Gypsy node(WIFI_SSID, WIFI_PASS, DEVICE_ID, AUTH_TOKEN);

int readSensors(SensorReading* out, int maxCount) {
    out[0] = { "uptime_seconds", (float)(millis() / 1000), "s", 0, 9999999 };
    return 1;
}

void setup() {
    node.registerSensor(readSensors);
    node.begin();
}

void loop() {
    node.handle();
}
