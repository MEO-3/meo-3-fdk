#include "MeoMessage.h"

#include <ArduinoJson.h>

MeoMessage::MeoMessage() {}

void MeoMessage::textMessageSetter(char* input) {
  this->messageStorage = input;
}

void MeoMessage::jsonMessageSetter() {
  serializeJson(jsonMessage, this->jsonMessageStorage);
}

void MeoMessage::reset() {
  textMessageSetter("");
  this->jsonMessage.clear();
}