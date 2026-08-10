#pragma once

// DHT11 temperature + humidity on DHT11_PIN.

void dhtBegin();

// False until the first successful read; both getters return NAN before that.
bool dhtReady();

// Last good sample — a failed read returns the previous value, never NAN once
// dhtReady(). The DHT11 only refreshes every ~2s; the driver caches within that
// window, so calling these often is free.
float dhtTemperature(); // degC
float dhtHumidity();    // %RH
