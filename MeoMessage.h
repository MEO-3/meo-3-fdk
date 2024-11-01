#ifndef MeoMessage_h
#define MeoMessage_h

#include <Arduino.h>

#include <ArduinoJson.h>

class MeoMessage {
public:
  MeoMessage();
  void textMessageSetter(char* input);
  void textMessageSetter(String input);
  void jsonMessageSetter();
  void reset();

  char* messageStorage;
  char jsonMessageStorage[500];
  StaticJsonDocument<500> jsonMessage;
private:
  int MSG_BUFFER_SIZE = 256;
  int JSON_BUFFER_SIZE = 500;
};

#endif