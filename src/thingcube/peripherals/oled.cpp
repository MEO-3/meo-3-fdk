#include "oled.h"

#include <Adafruit_SH110X.h>
#include <Wire.h>

#include "../pins.h"

static Adafruit_SH1106G oled(128, 64, &Wire, -1); // no reset pin
static bool ready = false;

bool oledBegin() {
    ready = oled.begin(SH1106_ADDR, true);
    return ready;
}

void oledText(const char *l1, const char *l2, const char *l3, const char *l4) {
    if (!ready) {
        return;
    }

    const char *lines[] = {l1, l2, l3, l4};
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SH110X_WHITE);
    for (int i = 0; i < 4; i++) {
        if (lines[i]) {
            oled.setCursor(0, i * 16);
            oled.println(lines[i]);
        }
    }
    oled.display();
}
