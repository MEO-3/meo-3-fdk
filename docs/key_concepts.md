# Key Concepts

This library follows the MEO 3 open-service contract and stays focused on getting an ESP32 device
provisioned and online. Command/reading exchange over MQTT is a separate, not-yet-wired concern
(see "What's not here yet" below).

## Device lifecycle

- `MeoDevice(model)` creates one device instance
- Declare capabilities with `addCapability(id)` in `setup()`, before `begin()`
- `begin()` starts storage, BLE, and provisioning; connects to Wi-Fi if credentials are already stored
- `loop()` drives provisioning and detects when Wi-Fi comes up
- `isProvisioned()` / `isWifiConnected()` report status

## API reference

### Construction & identity

- `MeoDevice()` / `MeoDevice(model)`
- `setDeviceInfo(model, manufacturer)`
- `setFirmwareVersion(version)` — reported in the capability payload, default `"0.0.0"`

### Logging

- `setLogger(fn)` — receives `(level, message)`
- `setDebugTags(csv)` — e.g. `"DEVICE,PROV"` to enable `DEBUG`-level logs for those tags only

### Capabilities

- `addCapability(id)` — declare one capability from `lib/meo/define/Meo3_Cmd.h`. Call once per
  capability, before `begin()`. Duplicates are ignored; the declared set is fixed-size (32 entries).
- `buildCapabilityPayload(out, cap)` — serializes the declared set into the capability
  characteristic payload. Called internally by `begin()`; exposed mainly for testing.

### Wi-Fi & lifecycle

- `beginWifi(ssid, pass)` — connect directly, bypassing BLE provisioning (local development only)
- `begin()` → `bool` — false only if storage or BLE init fails
- `loop()` — call on every `loop()` iteration
- `isProvisioned()`, `isWifiConnected()`

## Provisioning

When Wi-Fi is missing, the device advertises the MEO provisioning BLE service and walks the
gateway through MAC → capabilities → Wi-Fi config → status notify. The exact GATT contract
(UUIDs, payload formats, status states) is documented once, shared with the gateway, in
`meo-3-open-service/docs/firmware_development_guide.md` — treat that file as the source of truth
rather than this one.

## Capabilities

A device does not register a per-product profile. It declares which entries from the shared,
generic command catalog (`lib/meo/define/Meo3_Cmd.h`) it implements, and reports that set to the
gateway during provisioning via `addCapability()`. The gateway's `MeoCmd` catalog must stay in
sync with this file value-for-value — see the firmware development guide's "Capability Reporting"
section for the full contract and payload shape.

## Identity

- The board MAC is the default stable identity.
- Students should not need to type device IDs or MQTT credentials.

## What's not here yet

Runtime command dispatch and sensor reading publish over MQTT are not wired into `MeoDevice`.
`MeoMqttClient` (`lib/meo/mqtt/Meo3_Mqtt.h`) exists as a standalone transport wrapper, but nothing
in `MeoDevice` calls it yet. `examples/` still reference an older `onCommand` / `sendReading` /
`sendEvent` API that predates the capability-based model above and is due for an update — until
then, `src/main.cpp` is the sketch that matches the current library.
