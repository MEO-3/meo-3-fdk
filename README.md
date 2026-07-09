# MEO 3 Arduino Library

MEO 3 Arduino is an ESP32 firmware library for MEO 3 devices. It handles BLE provisioning against
the MEO 3 open-service gateway so a device can join Wi-Fi and report what it can do, without the
sketch touching MQTT or BLE directly.

## What it does

- Advertises BLE provisioning and walks the MEO provisioning GATT contract
- Connects to Wi-Fi once the gateway writes credentials (or via `beginWifi()` for local development)
- Uses the board MAC as the stable device identity — no device IDs or MQTT credentials to configure
- Reports device model, firmware version, and declared capabilities to the gateway during provisioning

## Install

- Open the library in PlatformIO (or Arduino IDE)
- `#include <Meo3.h>`
- Give your device a name and declare its capabilities

## Quick start

```cpp
#include <Meo3.h>
#include "define/Meo3_Cmd.h"

MeoDevice meo("Classroom Weather Station");

void setup() {
  // Declare capabilities before begin() — the gateway reads them off the
  // BLE capability characteristic during provisioning.
  meo.addCapability(MEO_READ_TEMP);
  meo.addCapability(MEO_WRITE_MOTOR);

  meo.begin();
}

void loop() {
  meo.loop();
}
```

## API

- `MeoDevice()` / `MeoDevice(model)`
- `setLogger(fn)`, `setDebugTags(csv)` — optional logging
- `setDeviceInfo(model, manufacturer)`, `setFirmwareVersion(version)`
- `addCapability(id)` — declare a capability from `lib/meo/define/Meo3_Cmd.h`; call before `begin()`
- `beginWifi(ssid, pass)` — bypass BLE provisioning for local development
- `begin()` — init storage/BLE/provisioning, connect if already provisioned
- `loop()` — drive provisioning, detect Wi-Fi connect, stop BLE once online
- `isProvisioned()`, `isWifiConnected()`

Runtime command/reading exchange over MQTT (once the device is online) is not yet wired into
`MeoDevice` — see `docs/key_concepts.md`.

## Provisioning

The device advertises the MEO provisioning service when Wi-Fi is missing. The full GATT contract
(characteristics, payload formats, status states) is shared with the gateway and documented once,
in `meo-3-open-service/docs/firmware_development_guide.md` — that file is the source of truth, not
this README.

## Capabilities

A device doesn't register a per-product profile. It reports the generic capability IDs it supports
(from `lib/meo/define/Meo3_Cmd.h`) during provisioning; the gateway's `MeoCmd` catalog is kept in
sync with this file value-for-value. See the firmware development guide's "Capability Reporting"
section for the full contract.

## Examples

- `examples/01_blink_command`
- `examples/02_button_event`
- `examples/03_temperature_reading`

> These examples predate the capability-based API above and still call `onCommand` /
> `sendReading` / `sendEvent`, which no longer exist on `MeoDevice`. `src/main.cpp` is the sketch
> that matches the current library; the examples are due for an update to match.

## Docs

- `docs/key_concepts.md` — beginner mental model and full API reference
- `meo-3-open-service/docs/firmware_development_guide.md` — authoritative BLE provisioning contract
