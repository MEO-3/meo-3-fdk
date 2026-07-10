#include "Meo3_Messaging.h"
#include "../define/Meo3_CmdErr.h"
#include <ArduinoJson.h>
#include <stdarg.h>
#include <string.h>

void MeoMessaging::setLogger(MeoLogFunction logger) {
    _logger = logger;
}

void MeoMessaging::setDebugTags(const char* tagsCsv) {
    if (!tagsCsv) { _debugTags[0] = '\0'; return; }
    strncpy(_debugTags, tagsCsv, sizeof(_debugTags) - 1);
    _debugTags[sizeof(_debugTags) - 1] = '\0';
}

bool MeoMessaging::onCommand(uint16_t cap, MeoWriteHandler fn) {
    if (!fn) return false;
    for (uint8_t i = 0; i < _writeCount; ++i) {
        if (_writeCaps[i] == cap) { _writeFns[i] = fn; return true; }
    }
    if (_writeCount >= MEO_MAX_HANDLERS) {
        _logf("WARN", "MSG", "Command handler table full; ignoring 0x%04X", cap);
        return false;
    }
    _writeCaps[_writeCount] = cap;
    _writeFns[_writeCount] = fn;
    _writeCount++;
    return true;
}

bool MeoMessaging::onRead(uint16_t cap, MeoReadHandler fn) {
    if (!fn) return false;
    for (uint8_t i = 0; i < _readCount; ++i) {
        if (_readCaps[i] == cap) { _readFns[i] = fn; return true; }
    }
    if (_readCount >= MEO_MAX_HANDLERS) {
        _logf("WARN", "MSG", "Read handler table full; ignoring 0x%04X", cap);
        return false;
    }
    _readCaps[_readCount] = cap;
    _readFns[_readCount] = fn;
    _readCount++;
    return true;
}

bool MeoMessaging::begin(MeoMqttClient* mqtt, const char* deviceId) {
    if (!mqtt || !deviceId || !deviceId[0]) return false;
    _mqtt = mqtt;
    snprintf(_topicCommand, sizeof(_topicCommand), "meo/v1/device/%s/command", deviceId);
    snprintf(_topicReply, sizeof(_topicReply), "meo/v1/device/%s/reply", deviceId);
    snprintf(_topicEvent, sizeof(_topicEvent), "meo/v1/device/%s/event", deviceId);
    _mqtt->setMessageHandler(&MeoMessaging::_onMessageStatic, this);
    return true;
}

void MeoMessaging::loop() {
    if (!_mqtt) return;

    if (!_mqtt->isConnected()) {
        _subscribed = false;
        uint32_t now = millis();
        if (_lastConnectAttempt != 0 && now - _lastConnectAttempt < RECONNECT_INTERVAL_MS) return;
        _lastConnectAttempt = now;
        if (!_mqtt->connect()) return;
    }

    if (!_subscribed) {
        // Clean session on every connect, so (re)subscribe each time
        _subscribed = _mqtt->subscribe(_topicCommand, 1);
        if (_subscribed) _logf("INFO", "MSG", "Messaging online (%s)", _topicCommand);
    }

    _mqtt->loop();
}

bool MeoMessaging::isConnected() {
    return _mqtt && _mqtt->isConnected() && _subscribed;
}

bool MeoMessaging::sendEvent(uint16_t cap, double value) {
    if (!isConnected()) return false;
    StaticJsonDocument<96> doc;
    doc["cap"] = cap;
    doc["value"] = value;
    char buf[96];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len == 0) return false;
    return _mqtt->publish(_topicEvent, buf, false);
}

void MeoMessaging::_onMessageStatic(const char* topic, const uint8_t* payload, unsigned int length, void* ctx) {
    MeoMessaging* self = reinterpret_cast<MeoMessaging*>(ctx);
    if (!self || !topic || strcmp(topic, self->_topicCommand) != 0) return;
    self->_handleCommand(payload, length);
}

void MeoMessaging::_handleCommand(const uint8_t* payload, unsigned int length) {
    StaticJsonDocument<192> doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        _log("WARN", "MSG", "Dropping malformed command JSON");
        return;
    }

    const char* requestId = doc["requestId"] | "";
    if (!requestId[0]) {
        _log("WARN", "MSG", "Dropping command without requestId");
        return;
    }

    uint16_t cap = doc["cap"] | 0;
    if (cap == 0) {
        _replyError(requestId, MEO_ERR_BAD_REQUEST);
        return;
    }

    for (uint8_t i = 0; i < _readCount; ++i) {
        if (_readCaps[i] == cap) {
            _replyOkDouble(requestId, cap, _readFns[i]());
            return;
        }
    }

    for (uint8_t i = 0; i < _writeCount; ++i) {
        if (_writeCaps[i] == cap) {
            int32_t value = doc["value"] | 0;
            if (_writeFns[i](value)) {
                _replyOkInt(requestId, cap, value);
            } else {
                _replyError(requestId, MEO_ERR_HANDLE_FAILED);
            }
            return;
        }
    }

    _logf("WARN", "MSG", "No handler for cap 0x%04X", cap);
    _replyError(requestId, MEO_ERR_UNKNOWN_CAP);
}

void MeoMessaging::_replyOkInt(const char* requestId, uint16_t cap, int32_t value) {
    StaticJsonDocument<192> doc;
    doc["requestId"] = requestId;
    doc["ok"] = true;
    doc["cap"] = cap;
    doc["value"] = value;
    char buf[192];
    if (serializeJson(doc, buf, sizeof(buf)) == 0) return;
    _mqtt->publish(_topicReply, buf, false);
}

void MeoMessaging::_replyOkDouble(const char* requestId, uint16_t cap, double value) {
    StaticJsonDocument<192> doc;
    doc["requestId"] = requestId;
    doc["ok"] = true;
    doc["cap"] = cap;
    doc["value"] = value;
    char buf[192];
    if (serializeJson(doc, buf, sizeof(buf)) == 0) return;
    _mqtt->publish(_topicReply, buf, false);
}

void MeoMessaging::_replyError(const char* requestId, int error) {
    StaticJsonDocument<192> doc;
    doc["requestId"] = requestId;
    doc["ok"] = false;
    doc["error"] = error;
    char buf[192];
    if (serializeJson(doc, buf, sizeof(buf)) == 0) return;
    _mqtt->publish(_topicReply, buf, false);
}

bool MeoMessaging::_debugTagEnabled(const char* tag) const {
    if (!_debugTags[0]) return false;
    const char* p = strstr(_debugTags, tag);
    if (!p) return false;
    bool leftOk  = (p == _debugTags) || (*(p - 1) == ',');
    const char* end = p + strlen(tag);
    bool rightOk = (*end == '\0') || (*end == ',');
    return leftOk && rightOk;
}

void MeoMessaging::_log(const char* level, const char* tag, const char* msg) const {
    if (!_logger) return;
    char buf[256];
    snprintf(buf, sizeof(buf), "[%s] %s", tag ? tag : "MSG", msg ? msg : "");
    _logger(level, buf);
}

void MeoMessaging::_logf(const char* level, const char* tag, const char* fmt, ...) const {
    if (!_logger) return;
    char msg[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    _log(level, tag, msg);
}
