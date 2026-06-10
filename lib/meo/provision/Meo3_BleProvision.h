#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>
#include "../storage/Meo3_Storage.h"
#include "../ble/Meo3_Ble.h"
#include "../Meo3_Type.h" // MeoLogFunction

// MEO open-service provisioning contract.
#define MEO_BLE_PROV_SERV_UUID      "7f5a0000-0f23-4b6a-9f5e-3c2a9f7e0100"
#define CH_UUID_DEVICE_MAC          "7f5a0001-0f23-4b6a-9f5e-3c2a9f7e0100"
#define CH_UUID_WIFI_CONFIG         "7f5a0002-0f23-4b6a-9f5e-3c2a9f7e0100"
#define CH_UUID_PROVISION_STATUS    "7f5a0003-0f23-4b6a-9f5e-3c2a9f7e0100"
#define CH_UUID_PROFILE_ID          "7f5a0004-0f23-4b6a-9f5e-3c2a9f7e0100"

class MeoBleProvision {
public:
    MeoBleProvision() = default;

    // Logging
    void setLogger(MeoLogFunction logger);
    void setDebugTags(const char* tagsCsv); // enables DEBUG for "PROV" when tag present

    bool begin(MeoBle* ble, MeoStorage* storage, const char* deviceName, const char* profileId);

    // Start/stop advertising through base BLE
    void startAdvertising();
    void stopAdvertising();

    // Call regularly to refresh status and handle optional scheduled reboot
    void loop();

    void setProfileId(const char* profileId);
    void setProvisionState(const char* state);

private:
    MeoBle*            _ble      = nullptr;
    MeoStorage*        _storage  = nullptr;

    std::string        _deviceName;
    std::string        _profileId;
    std::string        _macAddress;
    std::string        _pendingSsid;
    std::string        _pendingPassword;

    NimBLEService*         _svc      = nullptr;
    NimBLECharacteristic*  _chMac = nullptr;
    NimBLECharacteristic*  _chWifiConfig = nullptr;
    NimBLECharacteristic*  _chStatus = nullptr;
    NimBLECharacteristic*  _chProfileId = nullptr;

    char                _statusBuf[96] = {0};
    bool                _wifiConfigPending = false;
    bool                _wifiConnectRunning = false;

    // Logging
    MeoLogFunction _logger = nullptr;
    char           _debugTags[96] = {0};

    // Internal lifecycle
    bool _createServiceAndCharacteristics();
    void _bindWriteHandlers();
    void _loadInitialValues();
    void _connectPendingWifi();
    void _setStatusJson(const char* state, const char* message = nullptr);
    std::string _readMacAddress() const;
    bool _debugTagEnabled(const char* tag) const;

    // Write callbacks
    static void _onWriteStatic(NimBLECharacteristic* ch, void* ctx);
    void _onWrite(NimBLECharacteristic* ch);
};
