#include "Meo3_Device.h"
#include <string.h>
#include <stdarg.h>
#include <esp_system.h>

MeoDevice::MeoDevice()
    : _model("MEO Device"),
      _manufacturer("ThingAI"),
      _fwVersion("0.0.0"),
      _wifiSsid(nullptr),
      _wifiPass(nullptr) {}

MeoDevice::MeoDevice(const char* model)
    : _model(model && model[0] ? model : "MEO Device"),
      _manufacturer("ThingAI"),
      _fwVersion("0.0.0"),
      _wifiSsid(nullptr),
      _wifiPass(nullptr) {}

void MeoDevice::setDeviceInfo(const char* model, const char* manufacturer) {
    if (model && model[0]) _model = model;
    _manufacturer = manufacturer;
}

void MeoDevice::setFirmwareVersion(const char* version) {
    if (version && version[0]) _fwVersion = version;
}

void MeoDevice::addCapability(uint16_t capabilityId) {
    if (_capabilityCount >= MEO_MAX_CAPABILITIES) {
        logw("DEVICE", "Capability list full; ignoring 0x%04X", capabilityId);
        return;
    }
    for (uint8_t i = 0; i < _capabilityCount; ++i) {
        if (_capabilities[i] == capabilityId) return;  // already declared
    }
    _capabilities[_capabilityCount++] = capabilityId;
}

size_t MeoDevice::buildCapabilityPayload(char* out, size_t cap) const {
    if (!out || cap == 0) return 0;

    int n = snprintf(out, cap,
                     "{\"model\":\"%s\",\"fw\":\"%s\",\"capabilities\":[",
                     _model ? _model : "",
                     _fwVersion ? _fwVersion : "");
    if (n < 0 || (size_t)n >= cap) return 0;
    size_t len = (size_t)n;

    for (uint8_t i = 0; i < _capabilityCount; ++i) {
        int m = snprintf(out + len, cap - len, "%s%u",
                         i == 0 ? "" : ",", _capabilities[i]);
        if (m < 0 || len + (size_t)m >= cap) return 0;
        len += (size_t)m;
    }

    int t = snprintf(out + len, cap - len, "]}");
    if (t < 0 || len + (size_t)t >= cap) return 0;
    return len + (size_t)t;
}

bool MeoDevice::onCommand(uint16_t cap, MeoMessaging::MeoWriteHandler fn) {
    // MEO_CMD_* generic commands (below 0x1000) are implicit — every firmware
    // supports them, they are never declared during provisioning.
    if (cap >= 0x1000) addCapability(cap);
    return _messaging.onCommand(cap, fn);
}

bool MeoDevice::onRead(uint16_t cap, MeoMessaging::MeoReadHandler fn) {
    if (cap >= 0x1000) addCapability(cap);
    return _messaging.onRead(cap, fn);
}

bool MeoDevice::sendReading(uint16_t cap, double value) {
    return _messaging.sendEvent(cap, value);
}

bool MeoDevice::sendEvent(uint16_t cap, double value) {
    return _messaging.sendEvent(cap, value);
}

void MeoDevice::setBroker(const char* host, uint16_t port) {
    _brokerHostOverride = host;
    _brokerPortOverride = port;
}

void MeoDevice::beginWifi(const char* ssid, const char* pass) {
    _wifiSsid = ssid;
    _wifiPass = pass;
    logi("DEVICE", "Connecting WiFi SSID=%s", ssid ? ssid : "");
    WiFi.mode(WIFI_STA);
    WiFi.begin(_wifiSsid, _wifiPass);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(100);
    }
    _wifiReady = (WiFi.status() == WL_CONNECTED);
    if (_wifiReady) {
        logi("DEVICE", "WiFi connected");
    } else {
        loge("DEVICE", "WiFi failed");
    }
}

bool MeoDevice::begin() {
    if (!_storage.begin()) {
        loge("DEVICE", "Storage init failed");
        return false;
    }

    std::string setupName = "MEO-Setup-";
    setupName += _model ? _model : "Device";
    if (!_ble.begin(setupName.c_str())) {
        loge("DEVICE", "BLE init failed");
        return false;
    }

    char capPayload[384];
    if (buildCapabilityPayload(capPayload, sizeof(capPayload)) == 0) capPayload[0] = '\0';
    _prov.setCapabilities(capPayload);

    if (!_prov.begin(&_ble, &_storage, _model)) {
        loge("DEVICE", "BLE provisioning init failed");
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
        logi("DEVICE", "Already provisioned [%s]; skipping BLE advertising", _deviceId.c_str());
    } else {
        _prov.startAdvertising();
        _bleActive = true;
        logi("DEVICE", "BLE provisioning started; waiting for Wi-Fi config");
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
        logi("DEVICE", "Provisioned [%s]; BLE advertising stopped", _deviceId.c_str());
    }

    // Once online, start MQTT messaging (one attempt) and keep driving it
    if (_wifiReady && !_messagingStarted) {
        _startMessaging();
    }
    if (_messagingActive) {
        _messaging.loop();
    }
}

// Wire the MQTT transport + messaging once Wi-Fi is up. Broker comes from the
// setBroker() override (development) or storage, written during BLE
// provisioning. Missing broker info leaves messaging off — the device still
// runs, so a re-provision can fix it.
void MeoDevice::_startMessaging() {
    _messagingStarted = true;

    const char* host = _brokerHostOverride;
    uint16_t port = _brokerPortOverride;
    if (!host) {
        if (!_storage.loadString("mq_host", _storedBrokerHost) || _storedBrokerHost.empty()) {
            logw("DEVICE", "No broker info stored; messaging disabled (re-provision or setBroker)");
            return;
        }
        host = _storedBrokerHost.c_str();
        int16_t storedPort = 0;
        port = _storage.loadShort("mq_port", storedPort) ? (uint16_t)storedPort : 1883;
    }
    _mqtt.configure(host, port);
    _mqtt.setCredentials(_deviceId.c_str(), nullptr); // MAC as MQTT client id; no auth yet
    if (!_messaging.begin(&_mqtt, _deviceId.c_str())) {
        loge("DEVICE", "Messaging init failed");
        return;
    }

    _messagingActive = true;
    logi("DEVICE", "Messaging starting (broker %s:%u)", host, (unsigned)port);
}

bool MeoDevice::isProvisioned() const {
    return _wifiReady && _deviceId.length() > 0;
}

bool MeoDevice::_tryConnectStoredWifi() {
    if (_wifiSsid && _wifiPass) return false;  // caller already set credentials via beginWifi()

    std::string ssid, pass;
    if (!_storage.loadString("wifi_ssid", ssid) || ssid.empty()) return false;
    _storage.loadString("wifi_pass", pass);

    logi("DEVICE", "Connecting stored WiFi SSID=%s", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(100);
    }
    bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected) {
        logi("DEVICE", "Stored WiFi connected");
    } else {
        logw("DEVICE", "Stored WiFi failed");
    }
    return connected;
}

void MeoDevice::_ensureMacIdentity() {
    if (_deviceId.length()) return;
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        esp_read_mac(mac, ESP_MAC_ETH);
    }
    // Lowercase hex, no separators — directly usable in MQTT topics
    // (mqtt_messaging.md "Identities")
    char buf[13];
    snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    _deviceId = buf;
}

