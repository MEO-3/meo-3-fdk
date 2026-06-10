#include <Meo3.h>

MeoDevice meo("Classroom Light", "meo-profile-light-v1");

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  meo.onCommand("turn_on", []() {
    digitalWrite(LED_BUILTIN, HIGH);
  });

  meo.onCommand("turn_off", []() {
    digitalWrite(LED_BUILTIN, LOW);
  });

  meo.begin();
}

void loop() {
  meo.loop();
}
