#include "MeoConnect.h"
#include "MeoMessage.h"

MeoConnect meo_con = MeoConnect();
MeoMessage meo_me = MeoMessage();

void setup() {
  Serial.begin(115200);

  meo_con.setWifiConfig("Makerviet", "@abc1235");
  meo_con.setMqttConfig("192.168.0.195", 1883);
  meo_con.initConfig();
}

void loop() {
  if (!meo_con.client.connected()) {
    meo_con.reconnect();
    meo_con.client.subscribe("#");
  }

  meo_me.textMessageSetter("Hello NodeRED from MeoFDK!");
  meo_con.pubMessageToTopic(meo_me.messageStorage, "meo3/message");
  meo_me.reset();
  delay(3000);

  meo_me.jsonMessage["device"] = "ThingBot";
  meo_me.jsonMessage["message"] = "Hello NodeRED from MEOFDK!";
  meo_me.jsonMessageSetter();
  meo_con.pubMessageToTopic(meo_me.jsonMessageStorage, "meo3/json/message");
  meo_me.reset();
  delay(3000);

  meo_con.client.loop();
}