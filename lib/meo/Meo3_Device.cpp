#include "Meo3_Device.h"
#include <ArduinoJson.h>
#include <string.h>
#include <string>
#include <stdarg.h>
#include <esp_system.h>

MeoDevice::MeoDevice()
    : _deviceName("MEO Device"),
      _profileId("meo-profile-generic-v1"),
      _model("MEO Device"),
      _manufacturer("ThingAI"),
      _wifiSsid(nullptr),
      _wifiPass(nullptr),
      _gatewayHost("meo-open-service.local") {}

MeoDevice::MeoDevice(const char* deviceName, const char* profileId)
    : _deviceName(deviceName && deviceName[0] ? deviceName : "MEO Device"),
      _profileId(profileId && profileId[0] ? profileId : "meo-profile-generic-v1"),
      _model(_deviceName),
      _manufacturer("ThingAI"),
      _wifiSsid(nullptr),
      _wifiPass(nullptr),
      _gatewayHost("meo-open-service.local") {}

void MeoDevice::setLogger(MeoLogFunction logger) {
    _logger = logger;
    // Forward logger to submodules
    _mqtt.setLogger(logger);
    _prov.setLogger(logger);
}

void MeoDevice::setDebugTags(const char* tagsCsv) {
    if (!tagsCsv) { _debugTags[0] = '\0'; return; }
    strncpy(_debugTags, tagsCsv, sizeof(_debugTags) - 1);
    _debugTags[sizeof(_debugTags) - 1] = '\0';
    // Forward to submodules
    _mqtt.setDebugTags(tagsCsv);
    _prov.setDebugTags(tagsCsv);
}

void MeoDevice::setDeviceInfo(const char* model,
                              const char* manufacturer) {
    _model = model;
    if (model && model[0]) _deviceName = model;
    _manufacturer = manufacturer;
    _logf("DEBUG", "DEVICE", "Device info set: model=%s manufacturer=%s", 
        _model ? _model : "", _manufacturer ? _manufacturer : "");
    
}

void MeoDevice::setProfileId(const char* profileId) {
    _profileId = profileId && profileId[0] ? profileId : "meo-profile-generic-v1";
    _prov.setProfileId(_profileId);
    _logf("DEBUG", "DEVICE", "Profile ID set: %s", _profileId ? _profileId : "");
}

void MeoDevice::beginWifi(const char* ssid, const char* pass) {
    _wifiSsid = ssid;
    _wifiPass = pass;

    _logf("INFO", "DEVICE", "Connecting WiFi SSID=%s", ssid ? ssid : "");
    WiFi.mode(WIFI_STA);
    WiFi.begin(_wifiSsid, _wifiPass);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(100);
    }
    _wifiReady = (WiFi.status() == WL_CONNECTED);
    _logf(_wifiReady ? "INFO" : "ERROR", "DEVICE", "WiFi %s", _wifiReady ? "connected" : "failed");
}

void MeoDevice::setGateway(const char* host, uint16_t mqttPort) {
    _gatewayHost = host;
    _mqttPort = mqttPort;
    _logf("INFO", "DEVICE", "Gateway set: %s:%u", host ? host : "", mqttPort);
}

bool MeoDevice::addFeatureEvent(const char* name) {
    if (!name || !*name || _eventCount >= MEO_MAX_FEATURE_EVENTS) return false;
    _eventNames[_eventCount++] = name;
    if (_logger && _debugTagEnabled("DEVICE")) {
        _logf("DEBUG", "DEVICE", "Feature event added: %s", name);
    }
    return true;
}

bool MeoDevice::addFeatureMethod(const char* name, MeoFeatureCallback cb) {
    if (!name || !*name || !cb || _methodCount >= MEO_MAX_FEATURE_METHODS) return false;
    _methodNames[_methodCount]    = name;
    _methodHandlers[_methodCount] = cb;
    _methodCount++;
    if (_logger && _debugTagEnabled("DEVICE")) {
        _logf("DEBUG", "DEVICE", "Feature method added: %s", name);
    }
    return true;
}

bool MeoDevice::onCommand(const char* name, MeoSimpleCommandCallback cb) {
    if (!name || !*name || !cb) return false;
    if (_simpleCommandCount >= MEO_MAX_FEATURE_METHODS) return false;
    if (_methodCount >= MEO_MAX_FEATURE_METHODS) return false;

    _simpleCommandNames[_simpleCommandCount] = name;
    _simpleCommandHandlers[_simpleCommandCount] = cb;
    _simpleCommandCount++;

    _methodNames[_methodCount] = name;
    _methodHandlers[_methodCount] = nullptr;
    _methodCount++;
    return true;
}

bool MeoDevice::onCommand(const char* name, MeoFeatureCallback cb) {
    return addFeatureMethod(name, cb);
}

bool MeoDevice::start() {
    // Storage
    if (!_storage.begin()) {
        _log("ERROR", "DEVICE", "Storage init failed");
        return false;
    }

    // BLE provisioning uses the open-service contract and profile ID.
    std::string setupName = "MEO-Setup-";
    setupName += _deviceName ? _deviceName : "Device";
    _ble.begin(setupName.c_str());
    _prov.setLogger(_logger);
    _prov.setDebugTags(_debugTags);
    if (!_prov.begin(&_ble, &_storage, _deviceName, _profileId)) {
        _log("ERROR", "DEVICE", "BLE provisioning init failed");
        return false;
    }
    _prov.startAdvertising();
    _log("INFO", "DEVICE", "BLE provisioning started");

    // If WiFi not configured up-front, try load from storage (set via BLE)
    if (!_wifiReady && (!_wifiSsid || !_wifiPass)) {
        std::string ssid, pass;
        if (_storage.loadString("wifi_ssid", ssid) && _storage.loadString("wifi_pass", pass)) {
            _logf("INFO", "DEVICE", "WiFi creds loaded from storage: SSID=%s", ssid.c_str());
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), pass.c_str());
            uint32_t start = millis();
            while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
                delay(100);
            }
            _wifiReady = (WiFi.status() == WL_CONNECTED);
        }
    }

    _ensureMacIdentity();

    _logf("INFO", "DEVICE", "Device identity %s", _deviceId.c_str());

    // Only proceed if Wi-Fi and a MAC-derived identity are ready.
    if (!_wifiReady || !hasCredentials()) {
        _prov.setProvisionState(_wifiReady ? "connected" : "received");
        _log("WARN", "DEVICE", "Waiting for WiFi/credentials via BLE provisioning");
        return true;
    }
    _log("INFO", "DEVICE", "WiFi connected and identity available; connecting MQTT");

    // PATCH: stop BLE advertising once WiFi is connected (if BLE was already advertising)
    if (_wifiReady && hasCredentials()) {
        _prov.stopAdvertising();
        _log("INFO", "DEVICE", "WiFi connected; stopped BLE advertising");
    }

    // MQTT connect + declare
    if (!_connectMqttAndDeclare()) {
        _nextMqttRetryAtMs = millis() + 5000;
        _log("WARN", "DEVICE", "MQTT unavailable; will retry in loop()");
    }
    return true;
}

bool MeoDevice::begin() {
    return start();
}

void MeoDevice::loop() {
    _prov.loop();
    _mqtt.loop();

    if (!_wifiReady && WiFi.status() == WL_CONNECTED) {
        _wifiReady = true;
        _ensureMacIdentity();
        _prov.setProvisionState("connected");
        _log("INFO", "DEVICE", "WiFi connected after provisioning");
    }

    // Update BLE status on change
    static wl_status_t lastWifi = WL_IDLE_STATUS;
    wl_status_t nowWifi = WiFi.status();
    if (nowWifi != lastWifi) {
        _prov.setProvisionState(nowWifi == WL_CONNECTED ? "connected" : "received");
        lastWifi = nowWifi;
        if (_logger && _debugTagEnabled("DEVICE")) {
            _logf("DEBUG", "DEVICE", "Status WiFi=%s MQTT=%s",
                  nowWifi == WL_CONNECTED ? "connected" : "disconnected",
                  _mqtt.isConnected() ? "connected" : "disconnected");
        }
    }

    // Lazy reconnect when WiFi + creds available
    if (!_mqtt.isConnected() && _wifiReady && hasCredentials() && millis() >= _nextMqttRetryAtMs) {
        _log("WARN", "DEVICE", "MQTT disconnected; attempting reconnect");
        if (!_connectMqttAndDeclare()) {
            _nextMqttRetryAtMs = millis() + 5000;
        }
    }
}

bool MeoDevice::publishEvent(const char* eventName,
                             const char* const* keys,
                             const char* const* values,
                             uint8_t count) {
    if (!_mqtt.isConnected()) return false;
    std::string base = "meo/";
    std::string topic = base + _deviceId + "/event";

    StaticJsonDocument<512> doc;
    for (uint8_t i = 0; i < count; ++i) {
        doc[keys[i]] = values[i];
    }

    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len == 0) return false;

    if (_logger && _debugTagEnabled("DEVICE")) {
        _logf("DEBUG", "DEVICE", "Publish event %s len=%u", eventName, (unsigned)len);
    }
    return _mqtt.publish(topic.c_str(), (const uint8_t*)buf, len, false);
}

bool MeoDevice::publishEvent(const char* eventName, const MeoEventPayload& payload) {
    if (!_mqtt.isConnected()) return false;
    std::string base = "meo/";
    std::string topic = base + _deviceId + "/event/" + eventName;

    StaticJsonDocument<512> doc;
    for (const auto& kv : payload) {
        doc[kv.first] = kv.second;
    }

    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len == 0) return false;

    if (_logger && _debugTagEnabled("DEVICE")) {
        _logf("DEBUG", "DEVICE", "Publish event %s len=%u", eventName, (unsigned)len);
    }
    return _mqtt.publish(topic.c_str(), (const uint8_t*)buf, len, false);
}

bool MeoDevice::sendEvent(const char* eventName) {
    MeoEventPayload payload;
    return publishEvent(eventName, payload);
}

bool MeoDevice::sendReading(const char* name, int value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    return sendReading(name, buf);
}

bool MeoDevice::sendReading(const char* name, float value) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%.2f", value);
    return sendReading(name, buf);
}

bool MeoDevice::sendReading(const char* name, const char* value) {
    if (!name || !*name) return false;
    MeoEventPayload payload;
    payload["value"] = value ? value : "";
    return publishEvent(name, payload);
}

bool MeoDevice::sendFeatureResponse(const char* featureName,
                                    bool success,
                                    const char* message) {
    if (!_mqtt.isConnected()) return false;
    std::string base = "meo/";
    std::string topic = base + _deviceId + "/event/feature_response";

    StaticJsonDocument<512> doc;
    doc["feature_name"] = featureName;
    doc["device_id"]    = _deviceId.c_str();
    doc["success"]      = success;
    if (message) doc["message"] = message;

    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len == 0) return false;

    if (_logger && _debugTagEnabled("DEVICE")) {
        _logf("DEBUG", "DEVICE", "Publish feature_response for %s", featureName);
    }
    return _mqtt.publish(topic.c_str(), (const uint8_t*)buf, len, false);
}

bool MeoDevice::sendFeatureResponse(const MeoFeatureCall& call,
                                    bool success,
                                    const char* message) {
    return sendFeatureResponse(call.featureName.c_str(), success, message);
}

void MeoDevice::_updateBleStatus() {
    _prov.setProvisionState(WiFi.status() == WL_CONNECTED ? "connected" : "received");
}

bool MeoDevice::_connectMqttAndDeclare() {
    // Configure transport (host/port + credentials)
    _mqtt.configure(_gatewayHost, _mqttPort);
    _mqtt.setCredentials(_deviceId.c_str(), _transmitKey.c_str());
    _mqtt.setLogger(_logger);
    _mqtt.setDebugTags(_debugTags);

    // LWT: status offline retained
    {
        std::string base = "meo/";
        std::string willTopic = base + _deviceId + "/status";
        _mqtt.setWill(willTopic.c_str(), "offline", 0, false);
    }

    if (!_mqtt.connect()) {
        _log("ERROR", "DEVICE", "MQTT connect failed");
        return false;
    }
    _log("INFO", "DEVICE", "MQTT connected");

    // Subscribe to feature invokes and wire handler
    {
        std::string base = "meo/";
        std::string topic = base + _deviceId + "/feature/+/invoke";
        _mqtt.subscribe(topic.c_str());
        _mqtt.setMessageHandler(&_mqttThunk, this);
        if (_logger && _debugTagEnabled("DEVICE")) {
            _logf("DEBUG", "DEVICE", "Subscribed to %s", topic.c_str());
        }
    }

    // Publish online status
    {
        std::string base = "meo/";
        std::string statusTopic = base + _deviceId + "/status";
        _mqtt.publish(statusTopic.c_str(), "online", true);
    }

    // Declare
    _publishDeclare();

    _updateBleStatus();
    _nextMqttRetryAtMs = 0;
    return true;
}

bool MeoDevice::_publishDeclare() {
    if (!_mqtt.isConnected()) return false;

    std::string base = "meo/";
    std::string topic = base + _deviceId + "/declare";
    StaticJsonDocument<1024> doc;

    JsonObject info = doc.createNestedObject("device_info");
    info["name"]         = _deviceName ? _deviceName : "";
    info["profile_id"]   = _profileId ? _profileId : "";
    info["model"]        = _model ? _model : "";
    info["manufacturer"] = _manufacturer ? _manufacturer : "";
    info["connection"]   = "LAN";

    JsonArray events = doc.createNestedArray("events");
    for (uint8_t i = 0; i < _eventCount; ++i) {
        events.add(_eventNames[i]);
    }

    JsonArray methods = doc.createNestedArray("methods");
    for (uint8_t i = 0; i < _methodCount; ++i) {
        methods.add(_methodNames[i]);
    }

    char buf[1024];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len == 0) return false;

    if (_logger && _debugTagEnabled("DEVICE")) {
        _logf("DEBUG", "DEVICE", "Publish declare len=%u", (unsigned)len);
    }
    return _mqtt.publish(topic.c_str(), (const uint8_t*)buf, len, false);
}

void MeoDevice::_ensureMacIdentity() {
    if (_deviceId.length()) return;

    uint8_t mac[6] = {0};
    esp_err_t r = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (r != ESP_OK) {
        esp_read_mac(mac, ESP_MAC_ETH);
    }

    char macbuf[13];
    snprintf(macbuf, sizeof(macbuf), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    _deviceId = macbuf;
}

// Static -> instance adapter
void MeoDevice::_mqttThunk(const char* topic, const uint8_t* payload, unsigned int length, void* ctx) {
    MeoDevice* self = reinterpret_cast<MeoDevice*>(ctx);
    if (!self) return;
    self->_dispatchInvoke(topic, payload, length);
}

void MeoDevice::_dispatchInvoke(const char* topic, const uint8_t* payload, unsigned int length) {
    // Two supported invoke forms:
    // 1) Topic-encoded: meo/{deviceId}/feature/{featureName}/invoke
    // 2) Payload-encoded: meo/{deviceId}/feature with JSON { "feature"|"feature_name": "name", "params": {...} }

    char featureName[64] = {0};
    bool featureFromTopic = false;

    const char* featureMarker = strstr(topic, "/feature/");
    const char* invokeMarker  = strstr(topic, "/invoke");
    if (featureMarker && invokeMarker && invokeMarker > featureMarker) {
        featureMarker += 9; // strlen("/feature/")
        size_t nameLen = (size_t)(invokeMarker - featureMarker);
        if (nameLen > 0 && nameLen < sizeof(featureName)) {
            memcpy(featureName, featureMarker, nameLen);
            featureName[nameLen] = '\0';
            featureFromTopic = true;
        }
    }

    // Parse minimal JSON regardless of form to extract params (and possibly feature name)
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    bool jsonOk = (err == DeserializationError::Ok);
    if (!jsonOk && !featureFromTopic) return; // if no JSON and feature not in topic, nothing to do

    // If payload provides feature name, accept keys "feature" or "feature_name"
    if (!featureFromTopic && jsonOk) {
        if (doc.containsKey("feature") && doc["feature"].is<const char*>()) {
            strncpy(featureName, doc["feature"].as<const char*>(), sizeof(featureName)-1);
        } else if (doc.containsKey("feature_name") && doc["feature_name"].is<const char*>()) {
            strncpy(featureName, doc["feature_name"].as<const char*>(), sizeof(featureName)-1);
        }
    }

    if (featureName[0] == '\0') return; // no feature name discovered

    // Build MeoFeatureCall
    MeoFeatureCall call;
    call.deviceId    = _deviceId;
    call.featureName = featureName;

    // Extract params: prefer explicit "params" object, otherwise include other top-level keys except feature keys
    if (jsonOk) {
        if (doc.containsKey("params") && doc["params"].is<JsonObject>()) {
            for (JsonPair kv : doc["params"].as<JsonObject>()) {
                call.params[kv.key().c_str()] = kv.value().as<const char*>();
            }
        } else {
            for (JsonPair kv : doc.as<JsonObject>()) {
                const char* k = kv.key().c_str();
                if (strcmp(k, "feature") == 0 || strcmp(k, "feature_name") == 0) continue;
                call.params[k] = kv.value().as<const char*>();
            }
        }
    }

    // Dispatch to advanced handlers first.
    for (uint8_t i = 0; i < _methodCount; ++i) {
        if (_methodHandlers[i] && strcmp(featureName, _methodNames[i]) == 0) {
            if (_logger && _debugTagEnabled("DEVICE")) {
                _logf("DEBUG", "DEVICE", "Invoke %s with %u params", featureName, (unsigned)call.params.size());
            }
            _methodHandlers[i](call);
            return;
        }
    }

    // Then dispatch beginner no-argument commands.
    for (uint8_t i = 0; i < _simpleCommandCount; ++i) {
        if (_simpleCommandHandlers[i] && strcmp(featureName, _simpleCommandNames[i]) == 0) {
            if (_logger && _debugTagEnabled("DEVICE")) {
                _logf("DEBUG", "DEVICE", "Invoke simple command %s", featureName);
            }
            _simpleCommandHandlers[i]();
            return;
        }
    }

    // No handler: optionally negative response
    sendFeatureResponse(call, false, "No handler registered");
}

bool MeoDevice::_debugTagEnabled(const char* tag) const {
    if (!_debugTags[0]) return false; // no debug tags -> no DEBUG logs
    // simple substring match in CSV
    const char* p = strstr(_debugTags, tag);
    if (!p) return false;
    // ensure token boundary (start or preceded by comma) and followed by comma or end
    bool leftOk  = (p == _debugTags) || (*(p - 1) == ',');
    const char* end = p + strlen(tag);
    bool rightOk = (*end == '\0') || (*end == ',');
    return leftOk && rightOk;
}

void MeoDevice::_log(const char* level, const char* tag, const char* msg) const {
    if (!_logger) return;
    char buf[256];
    snprintf(buf, sizeof(buf), "[%s] %s", tag ? tag : "DEVICE", msg ? msg : "");
    _logger(level, buf);
}

void MeoDevice::_logf(const char* level, const char* tag, const char* fmt, ...) const {
    if (!_logger) return;
    char msg[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    _log(level, tag, msg);
}
