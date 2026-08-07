#include "mpu.h"

#include <Adafruit_MPU6050.h>
#include <Wire.h>

#include "../pins.h"

static Adafruit_MPU6050 mpu;
static bool ready = false;

bool mpuBegin() {
    ready = mpu.begin(MPU6050_ADDR, &Wire);
    return ready;
}

bool mpuRead(MpuReading &out) {
    if (!ready) {
        return false;
    }

    sensors_event_t accel, gyro, temp;
    if (!mpu.getEvent(&accel, &gyro, &temp)) {
        return false;
    }

    out.ax = accel.acceleration.x;
    out.ay = accel.acceleration.y;
    out.az = accel.acceleration.z;
    out.gx = gyro.gyro.x;
    out.gy = gyro.gyro.y;
    out.gz = gyro.gyro.z;
    out.tempC = temp.temperature;
    return true;
}
