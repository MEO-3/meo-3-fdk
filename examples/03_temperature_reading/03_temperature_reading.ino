#include <Meo3.h>

MeoDevice meo("Temperature Sensor", "meo-profile-temp-v1");

void setup() {
  meo.begin();
}

void loop() {
  meo.loop();

  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    float temperature = random(200, 310) / 10.0;
    meo.sendReading("temperature", temperature);
  }
}
