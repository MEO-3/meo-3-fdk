#pragma once

// SH1106 128x64 OLED on the shared I2C bus. Wire.begin() must run first.

bool oledBegin();

// Replace the whole screen with up to 4 lines of text. Pass nullptr to leave
// a line blank.
void oledText(const char *l1, const char *l2 = nullptr, const char *l3 = nullptr,
              const char *l4 = nullptr);
