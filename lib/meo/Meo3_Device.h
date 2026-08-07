#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include "Meo3_Logger.h"

#include "storage/Meo3_Storage.h"
#include "ble/Meo3_Ble.h"
#include "provision/Meo3_BleProvision.h"
#include "mqtt/Meo3_Mqtt.h"
#include "messaging/Meo3_Messaging.h"

class MeoDevice {
public:
    MeoDevice();
    MeoDevice(const char* model);
    // CSV of tags to enable DEBUG logs for (e.g. "DEVICE,PROV")

    // Device model and manufacturer. The model is reported to the gateway in
    // the capability report; the human-facing device name lives on the gateway,
    // not on firmware.
    void setDeviceInfo(const char* model, const char* manufacturer);

    // Firmware version reported in the capability report (default "0.0.0")
    void setFirmwareVersion(const char* version);

    // Declare a capability this device supports, using a MEO_* constant from
    // define/Meo3_Cmd.h. The capability set is fixed per boot: call once per
    // capability in setup(), before begin(). Duplicates and any past
    // MEO_MAX_CAPABILITIES are ignored. The declared set is exposed to the
    // gateway over the BLE provisioning capability characteristic.
    void addCapability(uint16_t capabilityId);

    // Serialize the capability report served over the BLE provisioning
    // capability characteristic (see firmware_development_guide.md "Capability
    // Reporting") into out. Returns bytes written excluding the NUL terminator,
    // or 0 if out is too small.
    size_t buildCapabilityPayload(char* out, size_t cap) const;

    // Register a command handler for an actuator / generic capability
    // (MEO_WRITE_* / MEO_CMD_*). Call in setup(), before begin(). Also
    // declares the capability, except for the implicit MEO_CMD_* range.
    bool onCommand(uint16_t cap, MeoMessaging::MeoWriteHandler fn);

    // Register a read handler for a sensor capability (MEO_READ_*); its return
    // value is carried in the reply. Also declares the capability.
    bool onRead(uint16_t cap, MeoMessaging::MeoReadHandler fn);

    // Publish an unsolicited reading/event ({"cap":..,"value":..}). Returns
    // false while messaging is offline. Both names publish to the same topic.
    bool sendReading(uint16_t cap, double value);
    bool sendEvent(uint16_t cap, double value);

    // Override Wi-Fi upfront (development / bypass provisioning)
    void beginWifi(const char* ssid, const char* pass);

    // Override the MQTT broker upfront (development / bypass provisioning).
    // Normally the broker comes from storage, written during BLE provisioning.
    void setBroker(const char* host, uint16_t port = 1883);

    // Lifecycle
    bool begin();   // Init storage, BLE, provisioning service; connect if already provisioned
    void loop();    // Drive provisioning; detect Wi-Fi connect; stop BLE when online; run messaging

    // Status
    bool isProvisioned() const;      // Wi-Fi connected and MAC identity set
    bool isWifiConnected() const { return _wifiReady; }
    bool isMqttConnected() { return _messaging.isConnected(); }

private:
    const char* _model;
    const char* _manufacturer;
    const char* _fwVersion;

    const char* _wifiSsid;
    const char* _wifiPass;

    std::string _deviceId;   // Wi-Fi MAC — stable device identity

    // Declared capability set (fixed per boot), serialized into the capability report
    static const uint8_t MEO_MAX_CAPABILITIES = 32;
    uint16_t _capabilities[MEO_MAX_CAPABILITIES];
    uint8_t  _capabilityCount = 0;

    MeoStorage      _storage;
    MeoBle          _ble;
    MeoBleProvision _prov;
    MeoMqttClient   _mqtt;
    MeoMessaging    _messaging;

    // Development broker override (setBroker); normally loaded from storage
    const char* _brokerHostOverride = nullptr;
    uint16_t    _brokerPortOverride = 1883;
    std::string _storedBrokerHost;

    bool _wifiReady  = false;
    bool _bleActive  = false;
    bool _messagingStarted = false;  // start attempted (one-shot)
    bool _messagingActive  = false;  // started successfully; _messaging.loop() runs


    bool _tryConnectStoredWifi();
    void _ensureMacIdentity();
    void _startMessaging();
};
