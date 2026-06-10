#include <Arduino.h>
#include <Meo3.h>

#define LED_PIN 7

MeoDevice meo("Classroom Light", "meo-profile-light-v1");

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    meo.setLogger([](const char* level, const char* message) {
        Serial.printf("[%s] %s\n", level, message);
    });

    meo.onCommand("turn_on", []() {
        digitalWrite(LED_PIN, HIGH);
    });

    meo.onCommand("turn_off", []() {
        digitalWrite(LED_PIN, LOW);
    });

    meo.begin();
}

void loop() {
    meo.loop();

    static uint32_t last = 0;
    if (millis() - last > 5000) {
        last = millis();
        meo.sendReading("light_level", 1);
    }
}
