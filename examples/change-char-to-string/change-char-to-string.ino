#include "MeoConnect.h"
#include "MeoMessage.h"
#include <DHT.h>

#define DHTTYPE DHT11
#define DHTPIN 2

MeoConnect meo_con = MeoConnect();
MeoMessage meo_me = MeoMessage();
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  dht.begin();

  meo_con.setWifiConfig("Ngoi Nha Bat On", "ngoinhabaton");
  meo_con.setMqttConfig("192.168.100.121 ", 1883);//ip của máy kèm port 
  meo_con.initConfig();
}

void loop() {
  if(!meo_con.client.connected()) {
    meo_con.reconnect();
  }
  //config dữ liệu về nhiệt độ - lấy heatindex(chỉ số nóng bức)
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);

  //check nếu có bất cứ dữ liệu nào trong 3 cái fail thì reconnect
  if (isnan(t) || isnan(h) || isnan(f)) {
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  std::string tempStr = String(t, 2).c_str();     // temperature
  std::string humidStr = String(h, 2).c_str();    // humidity
  std::string heatIndexStr = String(hic, 2).c_str(); // heat index

  meo_me.textMessageSetter(tempStr);
  meo_con.pubMessageToTopic(meo_me.messageStorage.c_str(), "meo3/Temp");
  meo_me.reset();

  meo_me.textMessageSetter(humidStr);
  meo_con.pubMessageToTopic(meo_me.messageStorage.c_str(), "meo3/Humid");
  meo_me.reset();
  delay(2000);

  meo_me.textMessageSetter(heatIndexStr);
  meo_con.pubMessageToTopic(meo_me.messageStorage.c_str(), "meo3/HeatIndex");
  meo_me.reset();
  delay(2000);

  /* meo_me.jsonMessage["device"] = "ThingBot";
  meo_me.jsonMessage["sensor"] = "DHT11";
  meo_me.jsonMessage["message"] = "HeatIndex";
  meo_me.jsonMessageSetter();
  meo_con.pubMessageToTopic(meo_me.jsonMessageStorage, "meo3/json/message");
  meo_me.reset();
  delay(100); */

  meo_con.client.loop();
}
