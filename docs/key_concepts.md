# Key Concepts

This library follows the MEO 3 open-service contract: it gets an ESP32 device provisioned over
BLE, then online over MQTT — receiving commands, replying, and publishing readings/events per
`meo-3-open-service/docs/mqtt_messaging.md`.

## Device lifecycle

- `MeoDevice(model)` creates one device instance
- Register command/read handlers with `onCommand(cap, fn)` / `onRead(cap, fn)` in `setup()`,
  before `begin()` — registering a handler also declares the capability
- `begin()` starts storage, BLE, and provisioning; connects to Wi-Fi if credentials are already stored
- `loop()` drives provisioning, detects when Wi-Fi comes up, then runs MQTT messaging (broker
  host/port come from storage, written during BLE provisioning)
- `isProvisioned()` / `isWifiConnected()` / `isMqttConnected()` report status

## API reference

### Construction & identity

- `MeoDevice()` / `MeoDevice(model)`
- `setDeviceInfo(model, manufacturer)`
- `setFirmwareVersion(version)` — reported in the capability payload, default `"0.0.0"`

### Logging

- `setLogger(fn)` — receives `(level, message)`
- `setDebugTags(csv)` — e.g. `"DEVICE,PROV"` to enable `DEBUG`-level logs for those tags only

### Capabilities & handlers

- `onCommand(cap, fn)` — register `bool fn(int32_t value)` for an actuator (`MEO_WRITE_*`) or a
  generic command (`MEO_CMD_*`). Called when the gateway sends that capability; return `false` to
  reply with an error. Also declares the capability (except the implicit `MEO_CMD_*` range).
- `onRead(cap, fn)` — register `double fn()` for a sensor (`MEO_READ_*`); the returned value is
  carried in the reply. Also declares the capability.
- `addCapability(id)` — declare a capability without a handler (e.g. an event-only `MEO_EVENT_*`).
  Duplicates are ignored; the declared set is fixed-size (32 entries).
- All registration happens in `setup()`, before `begin()`.
- `buildCapabilityPayload(out, cap)` — serializes the declared set into the capability
  characteristic payload. Called internally by `begin()`; exposed mainly for testing.

### Readings & events

- `sendReading(cap, value)` / `sendEvent(cap, value)` — publish `{"cap":..,"value":..}` to the
  device event topic. Two names, same topic; pick whichever reads better. Returns `false` while
  messaging is offline.

### Wi-Fi & lifecycle

- `beginWifi(ssid, pass)` — connect directly, bypassing BLE provisioning (local development only)
- `setBroker(host, port)` — override the MQTT broker, bypassing stored provisioning data (local
  development only; pairs with `beginWifi`)
- `begin()` → `bool` — false only if storage or BLE init fails
- `loop()` — call on every `loop()` iteration
- `isProvisioned()`, `isWifiConnected()`, `isMqttConnected()`

## Provisioning

When Wi-Fi is missing, the device advertises the MEO provisioning BLE service and walks the
gateway through MAC → capabilities → Wi-Fi config → status notify. The exact GATT contract
(UUIDs, payload formats, status states) is documented once, shared with the gateway, in
`meo-3-open-service/docs/firmware_development_guide.md` — treat that file as the source of truth
rather than this one.

## Capabilities

A device does not register a per-product profile. It declares which entries from the shared,
generic command catalog (`lib/meo/define/Meo3_Cmd.h`) it implements — normally implicitly, by
registering handlers via `onCommand()` / `onRead()` — and reports that set to the gateway during
provisioning. The gateway's `MeoCmd` catalog must stay in sync with this file value-for-value —
see the firmware development guide's "Capability Reporting" section for the full contract and
payload shape.

## Messaging

Once Wi-Fi is up, `MeoDevice` connects to the gateway broker (host/port stored during BLE
provisioning, keys `mq_host`/`mq_port`), subscribes to its command topic, dispatches commands to
the registered handlers, and auto-publishes a reply for every command. Topic and payload shapes
are documented once, shared with the gateway, in `meo-3-open-service/docs/mqtt_messaging.md` —
treat that file as the source of truth. Reconnects retry every 5 s; replies and events are QoS 0
(PubSubClient publish limitation, acknowledged in the contract doc).

## Identity

- The board MAC is the default stable identity.
- Students should not need to type device IDs or MQTT credentials.

## What's not here yet

Broker authentication (the broker is an open local listener for now) and device online/offline
presence are deliberately out of the messaging contract — see `mqtt_messaging.md`. `examples/`
still reference an older API that predates the capability-based model above and is due for an
update — until then, `src/main.cpp` is the sketch that matches the current library.
