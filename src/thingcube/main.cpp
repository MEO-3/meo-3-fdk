// ThingCube — DHT11 + MPU6050 + SH1106 on a MEO device.
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
        logw("CUBE", "MPU6050 not found - check wiring/address");
    }
    if (!oledBegin()) {
        logw("CUBE", "SH1106 not found - check wiring/address");
    }
    oledText("ThingCube", "starting...");

    if (!meo.begin()) {
        loge("CUBE", "begin() failed - halting");
        while (true) {
            delay(1000);
        }
    }

    logi("CUBE", "%s", meo.isProvisioned() ? "Provisioned - waiting for commands"
                                           : "Not provisioned - BLE advertising");
}

void loop() {
    meo.loop();

    static unsigned long last = 0;
    if (millis() - last < 2000) {
        return;
    }
    last = millis();

    // Sensors stay idle until the gateway has provisioned the device.
    if (!meo.isProvisioned()) {
        oledText("ThingCube", "Not provisioned", "BLE pairing...");
        return;
    }

    // Wiring check until the capabilities are wired up: dump both sensors.
    float t = dhtTemperature();
    float h = dhtHumidity();
    MpuReading m{};
    mpuRead(m);

    logi("DHT", "%.1fC %.1f%%", t, h);
    logi("MPU", "a=%.2f,%.2f,%.2f g=%.2f,%.2f,%.2f %.1fC", m.ax, m.ay, m.az, m.gx, m.gy, m.gz,
         m.tempC);

    char temp[24], humid[24], accel[24];
    snprintf(temp, sizeof(temp), "Temp  %.1f C", t);
    snprintf(humid, sizeof(humid), "Humid %.1f %%", h);
    snprintf(accel, sizeof(accel), "Acc %.1f %.1f %.1f", m.ax, m.ay, m.az);
    oledText("ThingCube", temp, humid, accel);
}
