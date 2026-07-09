# Contributing

This is the ESP32 firmware library that runs on MEO 3 devices. It's one of four independent repos
in the MEO 3 workspace (Java gateway, this Arduino library, Node-RED nodes, Node-RED fork) — see
the workspace-level `AGENTS.md` for how they fit together.

## Repo layout

- `lib/meo/` — the library itself, one subfolder per concern:
  - `Meo3_Device.{h,cpp}` — `MeoDevice`, the main entry point (lifecycle, capability declaration)
  - `ble/` — thin NimBLE wrapper (`MeoBle`)
  - `provision/` — BLE provisioning GATT service and state machine (`MeoBleProvision`)
  - `mqtt/` — standalone MQTT transport wrapper (`MeoMqttClient`); not yet wired into `MeoDevice`
  - `storage/` — persisted Wi-Fi credentials (`MeoStorage`)
  - `define/Meo3_Cmd.h` — the shared capability/command catalog (see below)
- `src/main.cpp` — a manual smoke-test sketch for the provisioning flow (excluded from the
  published library via `library.json`'s `export.exclude`)
- `examples/` — sketches shipped with the library for end users
- `docs/key_concepts.md` — user-facing mental model and API reference

## Build, flash, monitor

PlatformIO, targeting `esp32-c3-devkitc-02` (see `platformio.ini`):

```bash
pio run -e esp32-c3-devkitc-02            # build
pio run -e esp32-c3-devkitc-02 -t upload  # flash
pio device monitor -b 115200              # serial monitor
```

There's no test suite checked in yet (`test/` is the PlatformIO Unit Testing placeholder). If you
add one, wire it into `pio test`.

## Adding or changing a capability

Capability IDs in `lib/meo/define/Meo3_Cmd.h` are a **cross-repo contract** with the gateway's
`org.thingai.app.meo.define.MeoCmd` (in `meo-3-open-service`). The two catalogs must match
value-for-value — there's no shared build-time check, so:

1. Add the constant to `Meo3_Cmd.h` here.
2. Add the matching constant to `MeoCmd` in `meo-3-open-service`, same value.
3. Commit both (separately — these are independent git repos).

An ID the gateway doesn't recognize is displayed as "unknown" rather than dropped, so a
one-sided change won't break anything immediately — but it also won't do anything useful until
both sides agree.

## Conventions

- `lib/meo/` uses 4-space indent, matches the rest of the codebase.
- `examples/` use 2-space indent and stick to the simple `MeoDevice` API surface — they're
  read by students, so avoid introducing advanced/internal APIs there.
- The BLE provisioning GATT contract (UUIDs, payload formats, status states) is documented once,
  in `meo-3-open-service/docs/firmware_development_guide.md`. Don't duplicate it in this repo's
  README or docs — link to it instead, so the two don't drift apart.
- Commits: small and scoped; Conventional Commits style (`feat:`, `fix:`, `refactor:`, `docs:`)
  matches existing history.
- Don't commit Wi-Fi credentials, device keys, or other runtime config.
