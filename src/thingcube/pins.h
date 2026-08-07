#pragma once

// ThingCube pin map — ESP32-C3-DevKitC-02.

// Shared I2C bus: MPU6050 + SH1106 OLED. Core defaults for the C3.
// GPIO8 doubles as the onboard RGB LED — don't drive it while I2C is up.
#define SDA_PIN 8
#define SCL_PIN 9

#define MPU6050_ADDR 0x68 // AD0 low
#define SH1106_ADDR  0x3D

#define DHT11_PIN 3
