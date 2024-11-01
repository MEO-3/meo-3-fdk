#include "MeoMessage.h"

#include <ArduinoJson.h>

MeoMessage::MeoMessage() {}

void MeoMessage::textMessageSetter(char* input) {
  this->messageStorage = input;
}

void MeoMessage::textMessageSetter(String input) {
  int str_len = input.length() + 1;
  this->messageStorage = (char *) malloc(str_len);
  input.toCharArray(this->messageStorage, str_len);
}

void MeoMessage::jsonMessageSetter() {
  serializeJson(jsonMessage, this->jsonMessageStorage);
}

void MeoMessage::reset() {
  textMessageSetter("");
  this->jsonMessage.clear();
}