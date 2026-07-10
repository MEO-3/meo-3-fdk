#include <Arduino.h>
#include <WiFi.h>
#include <Meo3.h>
#include "define/Meo3_Cmd.h"

// ESP32-C3-DevKitC-02 built-in RGB is GPIO8; adjust if your board differs
#define LED_PIN 8

// How often to publish the periodic temperature reading (ms)
#define READING_INTERVAL_MS 10000

MeoDevice meo("MEO Test Device");

// Blink LED n times at the given on/off period (ms)
static void blinkLed(int times, int periodMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(periodMs / 2);
        digitalWrite(LED_PIN, LOW);
        delay(periodMs / 2);
    }
}

// MEO_WRITE_LED handler: 0 = off, anything else = on
static bool handleLed(int32_t value) {
    digitalWrite(LED_PIN, value ? HIGH : LOW);
    return true;
}

// MEO_READ_TEMP handler: no real sensor on the devkit — fake a slow drift
static double readTemperature() {
    return 20.0 + (millis() % 10000) / 1000.0;
}

void setup() {
    Serial.begin(115200);
    delay(500);  // let USB CDC enumerate before first print

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("\n=== MEO Provisioning + Messaging Test ===");

    meo.setLogger([](const char* level, const char* message) {
        Serial.printf("[%s] %s\n", level, message);
    });
    meo.setDebugTags("DEVICE,PROV,MQTT,MSG");

    // Handlers registered before begin() double as capability declarations —
    // the gateway reads the declared set off the BLE capability characteristic
    // during provisioning, and commands for these caps are dispatched here.
    meo.onCommand(MEO_WRITE_LED, handleLed);
    meo.onRead(MEO_READ_TEMP, readTemperature);

    bool ok = meo.begin();
    if (!ok) {
        Serial.println("[ERROR] begin() failed — halting");
        while (true) {
            blinkLed(3, 200);
            delay(1000);
        }
    }

    if (meo.isProvisioned()) {
        Serial.printf("[INFO] Already provisioned. IP: %s\n", WiFi.localIP().toString().c_str());
        blinkLed(2, 300);
    } else {
        Serial.println("[INFO] Not provisioned — BLE advertising. Waiting for gateway...");
    }
}

void loop() {
    meo.loop();

    // Slow blink while waiting for provisioning
    if (!meo.isProvisioned()) {
        blinkLed(1, 1000);
        return;
    }

    // First time we reach provisioned state; from here the LED belongs to
    // MEO_WRITE_LED commands
    static bool announced = false;
    if (!announced) {
        announced = true;
        Serial.printf("[INFO] Provisioned! IP: %s\n", WiFi.localIP().toString().c_str());
        blinkLed(5, 150);
    }

    // Periodic reading once messaging is online
    static uint32_t lastReadingAt = 0;
    if (meo.isMqttConnected() && millis() - lastReadingAt >= READING_INTERVAL_MS) {
        lastReadingAt = millis();
        meo.sendReading(MEO_READ_TEMP, readTemperature());
    }
}
