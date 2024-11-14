# MEO 3 Firmware Developement Kit

This repository contains the firmware development kit for the MEO 3.0 project. MEO 3 Firmware Developement Kit aims to simplify the development of the firmware for MEO 3 and provide a set of tools to help the developers to create and test the firmware.

## Getting Started

To use the MEO 3 FDK, you first need to clone this repository or download and move the FDK files to your Arduino project folder. The FDK is composed of the following files:

- `MeoConnect`: MeoConnect provide a set of functions that allow developers to setup WiFi connection, MQTT connection faster and easier.
- `MeoMessage`: MeoMessage help developers to create and format messages more straightforward and faster.

## Dependencies

The MEO 3 FDK depends on the following libraries:
- [ArduinoJson](https://arduinojson.org/): ArduinoJson is a JSON library for Arduino and embedded C++. It allows developers to parse and generate JSON messages.
- [PubSubClient](https://pubsubclient.knolleary.net/): PubSubClient is a MQTT library for Arduino. It allows developers to connect to a MQTT broker and publish/subscribe to topics.
- [WiFi](https://www.arduino.cc/en/Reference/WiFi): WiFi library allows developers to connect to a WiFi network.

## Manual

### MeoConnect

1. Setup WiFi connection
```cpp
MeoConnect::setupWiFiConfig("SSID", "PASSWORD");
```
- `SSID` is the name of the WiFi network.
- `PASSWORD` is the password of the WiFi network.
- This function will set the WiFi configuration for the device.

2. Setup MQTT connection
```cpp
MeoConnect::setupMQTTConfig("BROKER", "PORT");
```
- `BROKER` is the IP address of the MQTT broker.
- `PORT` is the port of the MQTT broker.
- This function will set the MQTT configuration for the device.

3. Initialize the configuration
```cpp
MeoConnect::initConfig();
```
- This function will initialize the WiFi and MQTT by getting configuration from setup functions above, then connect to the WiFi network and MQTT broker.

4. Reconnect to MQTT broker
```cpp
MeoConnect::reconnect();
```
- This function will reconnect to the MQTT broker.

5. MeoConnect client
```cpp
MeoConnect meo_con = MeoConnect();
meo_con.client
```
- `meo_con.client` is an object inherited from PubSubClient object. All methods from PubSubClient can be used with `meo_con.client`.

6. Send message and JsonMessage
```cpp

meo_me.textMessageSetter("Hello NodeRED from MeoFDK!");
meo_con.pubMessageToTopic(meo_me.messageStorage, "meo3/message");
meo_me.reset();

meo_me.jsonMessage["temperature"] = 25;
meo_me.jsonMessage["humidity"] = 50;
meo_me.jsonMessageSetter();
meo_con.pubMessageToTopic(meo_me.messageStorage, "meo3/message");
meo_me.reset();
```

- This is an example of how to send a text message and a JSON message to a topic.
- `meo_con.pubMessageToTopic(meo_me.messageStorage, "meo3/message");` is a function to publish a message to a topic.

Currently in the early version 1, this FDK only supports one-way communication, which is sending messages to the MQTT broker, the callback is default Serial print. In the future, we will support two-way communication, which is sending and receiving messages into variable from the MQTT broker, by now it's recommended to write your own callback function.

### MeoMessage

1. Text message
```cpp
MeoMessage meo_me = MeoMessage();
meo_me.textMessageSetter("Hello NodeRED from MeoFDK!");
```
- This function will set the text message to the message storage.

2. JSON message
```cpp
MeoMessage meo_me = MeoMessage();
meo_me.jsonMessage["temperature"] = 25;
meo_me.jsonMessage["humidity"] = 50;
meo_me.jsonMessageSetter();
```
- This function will set the JSON message to the message storage.

3. Reset message
```cpp
meo_me.reset();
```
- This function will reset the message storage. Developers should call this function after sending the message to the MQTT broker.

4. Access message storage
```cpp
meo_me.messageStorage;
meo_me.jsonMessageStorage;
```
- `meo_me.messageStorage` is a string variable that stores the message.
- `meo_me.jsonMessageStorage` is a string variable that stores the JSON message.
- Developers can access these variables to get the message and JSON message.


## Versioning

- Version 1.0.0: Initial version of the MEO 3 FDK.
- Version 1.1.0: Overload function for textMessageSetter. (Current version)