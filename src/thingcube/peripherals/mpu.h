#pragma once

// MPU6050 6-axis IMU on the shared I2C bus. Wire.begin() must run first.

struct MpuReading {
    float ax, ay, az; // m/s^2
    float gx, gy, gz; // rad/s
    float tempC;      // die temperature, not ambient — use the DHT11 for that
};

bool mpuBegin();
bool mpuRead(MpuReading &out);
