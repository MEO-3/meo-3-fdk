#include "Meo3_Device.h"
#include <string.h>
#include <stdarg.h>
#include <esp_system.h>

MeoDevice::MeoDevice()
    : _deviceName("MEO Device"),
      _profileId("meo-profile-generic-v1"),
      _model("MEO Device"),
      _manufacturer("ThingAI"),
      _wifiSsid(nullptr),
      _wifiPass(nullptr) {}

MeoDevice::MeoDevice(const char* deviceName, const char* profileId)
    : _deviceName(deviceName && deviceName[0] ? deviceName : "MEO Device"),
      _profileId(profileId && profileId[0] ? profileId : "meo-profile-generic-v1"),
      _model(_deviceName),
      _manufacturer("ThingAI"),
      _wifiSsid(nullptr),
      _wifiPass(nullptr) {}

void MeoDevice::setLogger(MeoLogFunction logger) {
    _logger = logger;
    _prov.setLogger(logger);
}

void MeoDevice::setDebugTags(const char* tagsCsv) {
    if (!tagsCsv) { _debugTags[0] = '\0'; return; }
    strncpy(_debugTags, tagsCsv, sizeof(_debugTags) - 1);
    _debugTags[sizeof(_debugTags) - 1] = '\0';
    _prov.setDebugTags(tagsCsv);
}

void MeoDevice::setDeviceInfo(const char* model, const char* manufacturer) {
    _model = model;
    if (model && model[0]) _deviceName = model;
    _manufacturer = manufacturer;
}

void MeoDevice::setProfileId(const char* profileId) {
    _profileId = profileId && profileId[0] ? profileId : "meo-profile-generic-v1";
    _prov.setProfileId(_profileId);
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

bool MeoDevice::begin() {
    if (!_storage.begin()) {
        _log("ERROR", "DEVICE", "Storage init failed");
        return false;
    }

    std::string setupName = "MEO-Setup-";
    setupName += _deviceName ? _deviceName : "Device";
    if (!_ble.begin(setupName.c_str())) {
        _log("ERROR", "DEVICE", "BLE init failed");
        return false;
    }

    _prov.setLogger(_logger);
    _prov.setDebugTags(_debugTags);
    if (!_prov.begin(&_ble, &_storage, _deviceName, _profileId)) {
        _log("ERROR", "DEVICE", "BLE provisioning init failed");
        return false;
    }

    // If Wi-Fi wasn't pre-set via beginWifi(), try stored credentials
    if (!_wifiReady) {
        _wifiReady = _tryConnectStoredWifi();
    }

    _ensureMacIdentity();

    if (_wifiReady) {
        _prov.setProvisionState("connected");
        _prov.stopAdvertising();
        _logf("INFO", "DEVICE", "Already provisioned [%s]; skipping BLE advertising", _deviceId.c_str());
    } else {
        _prov.startAdvertising();
        _bleActive = true;
        _log("INFO", "DEVICE", "BLE provisioning started; waiting for Wi-Fi config");
    }

    return true;
}

void MeoDevice::loop() {
    if (_bleActive) {
        _prov.loop();
    }

    // Wi-Fi came up from BLE provisioning write
    if (!_wifiReady && WiFi.status() == WL_CONNECTED) {
        _wifiReady = true;
        _ensureMacIdentity();
        _prov.setProvisionState("connected");
        _prov.stopAdvertising();
        _bleActive = false;
        _logf("INFO", "DEVICE", "Provisioned [%s]; BLE advertising stopped", _deviceId.c_str());
    }
}

bool MeoDevice::isProvisioned() const {
    return _wifiReady && _deviceId.length() > 0;
}

bool MeoDevice::_tryConnectStoredWifi() {
    if (_wifiSsid && _wifiPass) return false;  // caller already set credentials via beginWifi()

    std::string ssid, pass;
    if (!_storage.loadString("wifi_ssid", ssid) || ssid.empty()) return false;
    _storage.loadString("wifi_pass", pass);

    _logf("INFO", "DEVICE", "Connecting stored WiFi SSID=%s", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(100);
    }
    bool connected = (WiFi.status() == WL_CONNECTED);
    _logf(connected ? "INFO" : "WARN", "DEVICE", "Stored WiFi %s", connected ? "connected" : "failed");
    return connected;
}

void MeoDevice::_ensureMacIdentity() {
    if (_deviceId.length()) return;
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        esp_read_mac(mac, ESP_MAC_ETH);
    }
    char buf[13];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    _deviceId = buf;
}

bool MeoDevice::_debugTagEnabled(const char* tag) const {
    if (!_debugTags[0]) return false;
    const char* p = strstr(_debugTags, tag);
    if (!p) return false;
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
