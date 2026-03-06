// ---------------------------------------------------------------
//  GYPSY – DHT11/DHT22 example
//
//  Install Gypsy library, then edit the four lines in CONFIGURE
//  below. That's all you need to change.
// ---------------------------------------------------------------

#include <Gypsy.h>
#include <DHT.h>

// ================================================================
//  CONFIGURE  –  only edit this section
// ================================================================
#define WIFI_SSID   "YourNetwork"
#define WIFI_PASS   "YourPassword"
#define DEVICE_ID   "GYPSY-A00e"      // becomes http://GYPSY-A00e.local
#define AUTH_TOKEN  "changeme123"

#define DHT_PIN     4                  // GPIO4 = D2 on NodeMCU
#define DHT_TYPE    DHT11              // or DHT22
// ================================================================

DHT dht(DHT_PIN, DHT_TYPE);

// Create the node – library takes care of everything else
Gypsy node(WIFI_SSID, WIFI_PASS, DEVICE_ID, AUTH_TOKEN);

// ---------------------------------------------------------------
//  Sensor callback – called by the library whenever it needs data
//  Fill in SensorReading structs and return how many you provided.
// ---------------------------------------------------------------
int readSensors(SensorReading* out, int maxCount) {
    out[0] = { "temperature", dht.readTemperature(), "C",  -10, 60  };
    out[1] = { "humidity",    dht.readHumidity(),    "%",   0,  100 };
    return 2;
}

// ---------------------------------------------------------------
void setup() {
    dht.begin();
    node.registerSensor(readSensors);   // plug in your sensor callback
    node.begin();                       // starts WiFi, OTA, HTTP server
}

void loop() {
    node.handle();                      // that's it!
}
