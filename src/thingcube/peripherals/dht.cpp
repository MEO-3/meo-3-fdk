#include "dht.h"

#include <DHT.h>

#include "../pins.h"

static DHT dht(DHT11_PIN, DHT11);

void dhtBegin() {
    dht.begin();
}

float dhtTemperature() {
    return dht.readTemperature();
}

float dhtHumidity() {
    return dht.readHumidity();
}
