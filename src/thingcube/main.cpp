// ThingCub
#include <Arduino.h>
#include <Meo3.h>
#include <Wire.h>
#include "define/Meo3_Cmd.h"
#include "peripherals/dht.h"
#include "peripherals/mpu.h"
#include "peripherals/oled.h"
#include "pins.h"

MeoDevice meo("ThingCube");

void setup() {
    Serial.begin(115200);
    delay(500); // let USB CDC enumerate before the first print

    meo.setDeviceInfo("ThingCube", "MEO");

    // One shared bus for the MPU6050 and the OLED.
    Wire.begin(SDA_PIN, SCL_PIN);
    dhtBegin();
    if (!mpuBegin()) {
        Serial.println("[WARN] MPU6050 not found - check wiring/address");
    }
    if (!oledBegin()) {
        Serial.println("[WARN] SH1106 not found - check wiring/address");
    }
    oledText("ThingCube", "starting...");

    if (!meo.begin()) {
        Serial.println("[ERROR] begin() failed - halting");
        while (true) {
            delay(1000);
        }
    }

    Serial.println(meo.isProvisioned()
                       ? "[INFO] Provisioned - waiting for commands"
                       : "[INFO] Not provisioned - BLE advertising");
}

void loop() {
    meo.loop();

    // Wiring check until the capabilities are wired up: dump both sensors.
    static unsigned long last = 0;
    if (millis() - last > 2000) {
        last = millis();
        Serial.printf("[DHT] %.1fC %.1f%%\n", dhtTemperature(), dhtHumidity());

        MpuReading m{};
        if (mpuRead(m)) {
            Serial.printf("[MPU] a=%.2f,%.2f,%.2f g=%.2f,%.2f,%.2f %.1fC\n", m.ax, m.ay, m.az,
                          m.gx, m.gy, m.gz, m.tempC);
        }

        char temp[24], humid[24], accel[24];
        snprintf(temp, sizeof(temp), "Temp  %.1f C", dhtTemperature());
        snprintf(humid, sizeof(humid), "Humid %.1f %%", dhtHumidity());
        snprintf(accel, sizeof(accel), "Acc %.1f %.1f %.1f", m.ax, m.ay, m.az);
        oledText("ThingCube", temp, humid, accel);
    }
}
