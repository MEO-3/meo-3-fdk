#pragma once

#include <Arduino.h>

// [LEVEL] [TAG] message. tag must be a string literal — it is concatenated at
// compile time, so there is no runtime formatting cost for it.
#define logd(tag, fmt, ...) Serial.printf("[DEBUG] [" tag "] " fmt "\n", ##__VA_ARGS__)
#define logi(tag, fmt, ...) Serial.printf("[INFO] [" tag "] " fmt "\n", ##__VA_ARGS__)
#define logw(tag, fmt, ...) Serial.printf("[WARN] [" tag "] " fmt "\n", ##__VA_ARGS__)
#define loge(tag, fmt, ...) Serial.printf("[ERROR] [" tag "] " fmt "\n", ##__VA_ARGS__)
