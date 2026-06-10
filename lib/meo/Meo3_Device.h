#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <string>

#include "Meo3_Type.h"   // MeoFeatureCall, MeoEventPayload, MeoFeatureCallback, MeoConnectionType, MeoLogFunction
#include "storage/Meo3_Storage.h"
#include "ble/Meo3_Ble.h"
#include "provision/Meo3_BleProvision.h"
#include "mqtt/Meo3_Mqtt.h"              // MeoMqttClient transport

#ifndef MEO_MAX_FEATURE_EVENTS
#define MEO_MAX_FEATURE_EVENTS 8
#endif
#ifndef MEO_MAX_FEATURE_METHODS
#define MEO_MAX_FEATURE_METHODS 8
#endif

class MeoDevice {
public:
    MeoDevice();
    MeoDevice(const char* deviceName, const char* profileId);

    // Logging
    void setLogger(MeoLogFunction logger);
    // CSV of tags to enable DEBUG logs for (e.g. "DEVICE,MQTT,PROV")
    void setDebugTags(const char* tagsCsv);

    // Device info for declare and BLE RO fields
    void setDeviceInfo(const char* model, const char* manufacturer);
    void setProfileId(const char* profileId);

    // Optional: provide WiFi upfront; otherwise BLE provisioning can set it
    void beginWifi(const char* ssid, const char* pass);

    // MQTT broker (gateway)
    void setGateway(const char* host, uint16_t mqttPort = 1883);

    // Features (simple API)
    bool addFeatureEvent(const char* name);
    bool addFeatureMethod(const char* name, MeoFeatureCallback cb);
    bool onCommand(const char* name, MeoSimpleCommandCallback cb);
    bool onCommand(const char* name, MeoFeatureCallback cb);

    // Lifecycle
    bool start();    // Load creds; BLE provisioning if needed; MQTT connect; declare
    bool begin();
    void loop();     // BLE status, MQTT loop, lazy reconnect

    // Publish helpers
    bool publishEvent(const char* eventName,
                      const char* const* keys,
                      const char* const* values,
                      uint8_t count);
    bool publishEvent(const char* eventName, const MeoEventPayload& payload);
    bool sendEvent(const char* eventName);
    bool sendReading(const char* name, int value);
    bool sendReading(const char* name, float value);
    bool sendReading(const char* name, const char* value);

    // Send feature response
    bool sendFeatureResponse(const char* featureName,
                             bool success,
                             const char* message);
    bool sendFeatureResponse(const MeoFeatureCall& call,
                             bool success,
                             const char* message);

    // Status
    bool hasCredentials() const { return _deviceId.length(); }
    bool isMqttConnected() { return _mqtt.isConnected(); }
    bool isOnline() { return isMqttConnected(); }

private:
    // Config
    const char* _deviceName;
    const char* _profileId;
    const char* _model;
    const char* _manufacturer;

    const char* _wifiSsid;
    const char* _wifiPass;
    const char* _gatewayHost;
    uint16_t    _mqttPort = 1883;

    // Identity (from BLE/app)
    std::string  _deviceId;
    std::string  _transmitKey;

    // Registries (simple arrays)
    const char* _eventNames[MEO_MAX_FEATURE_EVENTS];
    uint8_t     _eventCount = 0;

    const char*        _methodNames[MEO_MAX_FEATURE_METHODS];
    MeoFeatureCallback _methodHandlers[MEO_MAX_FEATURE_METHODS];
    uint8_t            _methodCount = 0;

    const char*              _simpleCommandNames[MEO_MAX_FEATURE_METHODS];
    MeoSimpleCommandCallback _simpleCommandHandlers[MEO_MAX_FEATURE_METHODS];
    uint8_t                  _simpleCommandCount = 0;

    // Modules
    MeoStorage      _storage;
    MeoBle          _ble;
    MeoBleProvision _prov;
    MeoMqttClient   _mqtt;

    // State
    bool _wifiReady = false;
    uint32_t _nextMqttRetryAtMs = 0;

    // Logging
    MeoLogFunction _logger = nullptr;
    char           _debugTags[96] = {0}; // CSV list of enabled DEBUG tags

    // Internals
    void _updateBleStatus();
    bool _connectMqttAndDeclare();
    bool _publishDeclare();
    void _ensureMacIdentity();

    // MQTT message adapter: parse invoke and dispatch MeoFeatureCall
    static void _mqttThunk(const char* topic, const uint8_t* payload, unsigned int length, void* ctx);
    void _dispatchInvoke(const char* topic, const uint8_t* payload, unsigned int length);

    // Logging helpers
    bool _debugTagEnabled(const char* tag) const;
    void _log(const char* level, const char* tag, const char* msg) const;
    void _logf(const char* level, const char* tag, const char* fmt, ...) const;
};
