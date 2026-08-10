#include "dht.h"

#include <Arduino.h>
#include <DHT.h>

#include "../pins.h"

static DHT dht(DHT11_PIN, DHT11);

// The DHT11 drops reads routinely and the MEO reply frame has no "no value"
// encoding, so a NAN would ship to the gateway as a float32 NaN. Hold the last
// good sample instead.
static float lastT = NAN;
static float lastH = NAN;

void dhtBegin() {
    dht.begin();
}

bool dhtReady() {
    return !isnan(lastT) || !isnan(lastH);
}

float dhtTemperature() {
    float v = dht.readTemperature();
    if (!isnan(v)) {
        lastT = v;
    }
    return lastT;
}

float dhtHumidity() {
    float v = dht.readHumidity();
    if (!isnan(v)) {
        lastH = v;
    }
    return lastH;
}
