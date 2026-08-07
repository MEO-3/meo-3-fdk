#pragma once

// DHT11 temperature + humidity on DHT11_PIN.

void dhtBegin();

// NAN when the sensor fails to answer. The DHT11 only refreshes every ~2s;
// the driver caches within that window, so calling these often is free.
float dhtTemperature(); // degC
float dhtHumidity();    // %RH
