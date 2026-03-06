// ---------------------------------------------------------------
//  GYPSY – multi-sensor example
//  Shows how to attach any combination of sensors.
//  Uses a BMP280 (pressure/temp) + a soil moisture ADC pin.
// ---------------------------------------------------------------

#include <Gypsy.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#define WIFI_SSID   "YourNetwork"
#define WIFI_PASS   "YourPassword"
#define DEVICE_ID   "GYPSY-garden1"
#define AUTH_TOKEN  "supersecret"

#define SOIL_PIN    A0   // analog soil moisture sensor

Adafruit_BMP280 bmp;
Gypsy node(WIFI_SSID, WIFI_PASS, DEVICE_ID, AUTH_TOKEN);

// ---------------------------------------------------------------
//  Sensor callback – return up to GYPSY_MAX_SENSORS (default 8)
// ---------------------------------------------------------------
int readSensors(SensorReading* out, int maxCount) {
    int n = 0;

    out[n++] = { "temperature", bmp.readTemperature(), "C",   -40, 85   };
    out[n++] = { "pressure",    bmp.readPressure(),    "Pa",    0, 110000 };
    out[n++] = { "altitude",    bmp.readAltitude(1013.25), "m", 0, 9000 };

    // soil: raw ADC 0-1023, map to 0-100 %
    float soilRaw = analogRead(SOIL_PIN);
    out[n++] = { "soil_moisture", map(soilRaw, 0, 1023, 100, 0), "%", 0, 100 };

    return n;
}

// ---------------------------------------------------------------
void setup() {
    Wire.begin();
    bmp.begin(0x76);

    node.registerSensor(readSensors);
    node.begin();
}

void loop() {
    node.handle();
}
