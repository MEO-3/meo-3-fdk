# MEO 3 Arduino Library

MEO 3 Arduino is a beginner-friendly SDK for K12 Arduino projects that connect to the MEO 3 open service.

## What it does

- Starts the device with a simple Arduino API
- Handles BLE provisioning using the MEO service contract
- Uses the board MAC as the default device identity
- Lets you send readings and receive commands without dealing with MQTT topics

## Install

- Open the library in Arduino IDE or PlatformIO
- Include `Meo3.h`
- Give your device a friendly name

## Quick start

```cpp
#include <Meo3.h>

MeoDevice meo("Classroom Light");

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
```

## Beginner API

- `MeoDevice(name)`
- `begin()`
- `loop()`
- `onCommand(name, callback)`
- `sendReading(name, value)`
- `sendEvent(name)`
- `isOnline()`

## Provisioning

The device advertises the MEO provisioning service when Wi-Fi is missing.

Required BLE contract:

- Service UUID: `7f5a0000-0f23-4b6a-9f5e-3c2a9f7e0100`
- MAC read: `7f5a0001-0f23-4b6a-9f5e-3c2a9f7e0100`
- Wi-Fi config write: `7f5a0002-0f23-4b6a-9f5e-3c2a9f7e0100`
- Provision status: `7f5a0003-0f23-4b6a-9f5e-3c2a9f7e0100`

Wi-Fi is written as JSON:

```json
{"ssid":"Classroom WiFi","password":"secret"}
```

Status values:

- `received`
- `connecting`
- `connected`
- `failed`

## Examples

- `examples/01_blink_command`
- `examples/02_button_event`
- `examples/03_temperature_reading`

## Advanced use

The library still exposes lower-level callbacks for feature calls and MQTT-facing behavior, but the beginner API should be the default for student projects.
