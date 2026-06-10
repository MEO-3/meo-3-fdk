#include <Meo3.h>

const int buttonPin = 4;

MeoDevice meo("Classroom Button", "meo-profile-button-v1");

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  meo.begin();
}

void loop() {
  meo.loop();

  static int lastButton = HIGH;
  int button = digitalRead(buttonPin);
  if (button == LOW && lastButton == HIGH) {
    meo.sendEvent("button_pressed");
  }
  lastButton = button;
}
