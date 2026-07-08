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
    MeoDevice(const char* deviceName);

    // Logging
    void setLogger(MeoLogFunction logger);
    // CSV of tags to enable DEBUG logs for (e.g. "DEVICE,PROV")
    void setDebugTags(const char* tagsCsv);

    // Device metadata exposed via BLE provisioning characteristics
    void setDeviceInfo(const char* model, const char* manufacturer);

    // Override Wi-Fi upfront (development / bypass provisioning)
    void beginWifi(const char* ssid, const char* pass);

    // Lifecycle
    bool begin();   // Init storage, BLE, provisioning service; connect if already provisioned
    void loop();    // Drive provisioning; detect Wi-Fi connect; stop BLE when online

    // Status
    bool isProvisioned() const;      // Wi-Fi connected and MAC identity set
    bool isWifiConnected() const { return _wifiReady; }

private:
    const char* _deviceName;
    const char* _model;
    const char* _manufacturer;

    const char* _wifiSsid;
    const char* _wifiPass;

    std::string _deviceId;   // Wi-Fi MAC — stable device identity

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
