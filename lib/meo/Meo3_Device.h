#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <string>

#include "Meo3_Type.h"
#include "storage/Meo3_Storage.h"
#include "ble/Meo3_Ble.h"
#include "provision/Meo3_BleProvision.h"

class MeoDevice {
public:
    MeoDevice();
    MeoDevice(const char* model);

    // Logging
    void setLogger(MeoLogFunction logger);
    // CSV of tags to enable DEBUG logs for (e.g. "DEVICE,PROV")
    void setDebugTags(const char* tagsCsv);

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

    // Override Wi-Fi upfront (development / bypass provisioning)
    void beginWifi(const char* ssid, const char* pass);

    // Lifecycle
    bool begin();   // Init storage, BLE, provisioning service; connect if already provisioned
    void loop();    // Drive provisioning; detect Wi-Fi connect; stop BLE when online

    // Status
    bool isProvisioned() const;      // Wi-Fi connected and MAC identity set
    bool isWifiConnected() const { return _wifiReady; }

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

    bool _wifiReady  = false;
    bool _bleActive  = false;

    MeoLogFunction _logger = nullptr;
    char           _debugTags[96] = {0};

    bool _tryConnectStoredWifi();
    void _ensureMacIdentity();
    bool _debugTagEnabled(const char* tag) const;
    void _log(const char* level, const char* tag, const char* msg) const;
    void _logf(const char* level, const char* tag, const char* fmt, ...) const;
};
