#include "Meo3_BleProvision.h"
#include "Meo3_Logger.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_system.h>
#include <stdarg.h>
#include <string>

void MeoBleProvision::setLogger(MeoLogFunction logger) {
    _logger = logger;
}

void MeoBleProvision::setDebugTags(const char* tagsCsv) {
    if (!tagsCsv) { _debugTags[0] = '\0'; return; }
    strncpy(_debugTags, tagsCsv, sizeof(_debugTags) - 1);
    _debugTags[sizeof(_debugTags) - 1] = '\0';
}

bool MeoBleProvision::begin(MeoBle* ble, MeoStorage* storage, const char* deviceName) {
    _ble = ble;
    _storage = storage;
    _deviceName = deviceName && deviceName[0] ? deviceName : "MEO Device";

    if (!_ble || !_storage || !_storage->begin()) return false;
    _macAddress = _readMacAddress();
    if (!_createServiceAndCharacteristics()) return false;
    _bindWriteHandlers();
    _svc->start();
    _loadInitialValues();
    _setStatusJson(WiFi.status() == WL_CONNECTED ? "connected" : "received");
    return true;
}

void MeoBleProvision::setProvisionState(const char* state) {
    _setStatusJson(state ? state : "received");
}

void MeoBleProvision::setCapabilities(const char* payload) {
    _capabilities = payload ? payload : "";
}

bool MeoBleProvision::_createServiceAndCharacteristics() {
    _svc = _ble->createService(MEO_BLE_PROV_SERV_UUID);
    if (!_svc) return false;

    _chMac = _ble->createCharacteristic(_svc, CH_UUID_DEVICE_MAC, NIMBLE_PROPERTY::READ);
    _chWifiConfig = _ble->createCharacteristic(_svc, CH_UUID_WIFI_CONFIG, NIMBLE_PROPERTY::WRITE);
    _chStatus = _ble->createCharacteristic(_svc, CH_UUID_PROVISION_STATUS, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _chCapabilities = _ble->createCharacteristic(_svc, CH_UUID_DEVICE_CAPABILITIES, NIMBLE_PROPERTY::READ);

    return _chMac && _chWifiConfig && _chStatus && _chCapabilities;
}

void MeoBleProvision::_bindWriteHandlers() {
    _ble->setCharWriteHandler(_chWifiConfig, &MeoBleProvision::_onWriteStatic, this);
}

void MeoBleProvision::startAdvertising() {
    if (_ble) _ble->startAdvertising();
}

void MeoBleProvision::stopAdvertising() {
    if (_ble) _ble->stopAdvertising();
}

void MeoBleProvision::loop() {
    if (_wifiConfigPending && !_wifiConnectRunning) {
        _connectPendingWifi();
    }
}

void MeoBleProvision::_loadInitialValues() {
    if (_chMac) _chMac->setValue(_macAddress);
    if (_chCapabilities) _chCapabilities->setValue(_capabilities);
}

void MeoBleProvision::_connectPendingWifi() {
    _wifiConfigPending = false;
    _wifiConnectRunning = true;

    _setStatusJson("connecting");
    MeoLogf("INFO", "PROV", "Connecting Wi-Fi SSID=%s", _pendingSsid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(_pendingSsid.c_str(), _pendingPassword.c_str());

    uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 15000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        _setStatusJson("connected");
        MeoLog("INFO", "PROV", "Wi-Fi connected");
        stopAdvertising();
    } else {
        _setStatusJson("failed", "Wi-Fi connection failed");
        MeoLog("ERROR", "PROV", "Wi-Fi connection failed");
        startAdvertising();
    }

    _wifiConnectRunning = false;
}

void MeoBleProvision::_setStatusJson(const char* state, const char* message) {
    StaticJsonDocument<160> doc;
    doc["state"] = state && state[0] ? state : "received";
    if (message && message[0]) doc["message"] = message;

    size_t len = serializeJson(doc, _statusBuf, sizeof(_statusBuf));
    if (len == 0) {
        strncpy(_statusBuf, "{\"state\":\"failed\"}", sizeof(_statusBuf) - 1);
        _statusBuf[sizeof(_statusBuf) - 1] = '\0';
    }

    if (_chStatus) {
        _chStatus->setValue((uint8_t*)_statusBuf, strlen(_statusBuf));
        _chStatus->notify();
    }
}

std::string MeoBleProvision::_readMacAddress() const {
    uint8_t mac[6] = {0};
    esp_err_t r = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (r != ESP_OK) {
        r = esp_read_mac(mac, ESP_MAC_ETH);
    }

    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

void MeoBleProvision::_onWriteStatic(NimBLECharacteristic* ch, void* ctx) {
    reinterpret_cast<MeoBleProvision*>(ctx)->_onWrite(ch);
}

void MeoBleProvision::_onWrite(NimBLECharacteristic* ch) {
    if (!ch || !ch->getUUID().equals(NimBLEUUID(CH_UUID_WIFI_CONFIG))) return;

    std::string payload = ch->getValue();
    StaticJsonDocument<384> doc;
    DeserializationError err = deserializeJson(doc, payload.c_str(), payload.size());
    if (err) {
        _setStatusJson("failed", "Invalid network config JSON");
        MeoLog("ERROR", "PROV", "Invalid network config JSON");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* password = doc["password"] | "";
    if (!ssid || !ssid[0]) {
        _setStatusJson("failed", "Wi-Fi SSID is required");
        MeoLog("ERROR", "PROV", "Wi-Fi SSID is required");
        return;
    }

    const char* brokerHost = doc["brokerHost"] | "";
    uint16_t brokerPort = doc["brokerPort"] | 1883;
    if (!brokerHost[0]) {
        _setStatusJson("failed", "broker host is required");
        MeoLog("ERROR", "PROV", "broker host is required");
        return;
    }

    _pendingSsid = ssid;
    _pendingPassword = password ? password : "";
    if (!_storage->saveString("wifi_ssid", _pendingSsid) ||
        !_storage->saveString("wifi_pass", _pendingPassword) ||
        !_storage->saveString("mq_host", std::string(brokerHost)) ||
        !_storage->saveShort("mq_port", (int16_t)brokerPort)) {
        _setStatusJson("failed", "Could not save network config");
        MeoLog("ERROR", "PROV", "Could not save network config");
        return;
    }

    _setStatusJson("received");
    _wifiConfigPending = true;
    MeoLogf("INFO", "PROV", "Network config received (broker %s:%u)", brokerHost, (unsigned)brokerPort);
}

bool MeoBleProvision::_debugTagEnabled(const char* tag) const {
    if (!_debugTags[0]) return false;
    const char* p = strstr(_debugTags, tag);
    if (!p) return false;
    bool leftOk  = (p == _debugTags) || (*(p - 1) == ',');
    const char* end = p + strlen(tag);
    bool rightOk = (*end == '\0') || (*end == ',');
    return leftOk && rightOk;
}
