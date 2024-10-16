#ifndef MeoConnect_h
#define MeoConnect_h

#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>

class MeoConnect {
public:
  MeoConnect();
  void setWifiConfig(char* ssid, char* wifiPassword);
  void setMqttConfig(char* mqttServer, int mqttPort);
  void initConfig();
  void pubMessageToTopic(char* message, char* topic);
  void reconnect();
  
  static void defaultCallback(char* topic, byte* payload, int length);

  WiFiClient espClient = WiFiClient();
  PubSubClient client = PubSubClient(espClient);
private:
  char* ssid;
  char* wifiPassword;
  char* mqttServer;
  int mqttPort;
};

#endif
