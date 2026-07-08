#include <Arduino.h>
#include <WiFi.h>
#include <Meo3.h>

// ESP32-C3-DevKitC-02 built-in RGB is GPIO8; adjust if your board differs
#define LED_PIN 8

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

void setup() {
    Serial.begin(115200);
    delay(500);  // let USB CDC enumerate before first print

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("\n=== MEO BLE Provisioning Test ===");

    meo.setLogger([](const char* level, const char* message) {
        Serial.printf("[%s] %s\n", level, message);
    });
    meo.setDebugTags("DEVICE,PROV");

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

    // First time we reach provisioned state
    static bool announced = false;
    if (!announced) {
        announced = true;
        Serial.printf("[INFO] Provisioned! IP: %s\n", WiFi.localIP().toString().c_str());
        blinkLed(5, 150);
        digitalWrite(LED_PIN, HIGH);
    }
}
