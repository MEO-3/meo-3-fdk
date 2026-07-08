# Key Concepts

This library follows the MEO 3 open-service contract and is meant to stay simple for student projects.

## Beginner model

- `MeoDevice(name)` creates one device instance
- `begin()` starts provisioning and connectivity
- `loop()` keeps the device alive
- `onCommand()` registers simple actions
- `sendReading()` publishes sensor values
- `sendEvent()` publishes one-off events

## Provisioning

When Wi-Fi is missing, the device advertises the MEO provisioning service.

Required BLE service:

- `7f5a0000-0f23-4b6a-9f5e-3c2a9f7e0100`

Required characteristics:

- Device MAC read
- Wi-Fi config write
- Provision status read/notify

Wi-Fi is written as JSON:

```json
{"ssid":"Classroom WiFi","password":"secret"}
```

## Identity

- The board MAC is the default stable identity.
- Students should not need to type device IDs or MQTT credentials.

## Advanced API

Advanced feature callbacks still exist for users who want parameter maps and richer device behavior, but examples should prefer the simple API first.
