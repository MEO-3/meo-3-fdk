#include "Meo3_Messaging.h"
#include "../define/Meo3_CmdErr.h"
#include <stdarg.h>
#include <string.h>

// Frame sizes per mqtt_messaging.md — fixed, little-endian, no framing.
static const unsigned int COMMAND_FRAME_SIZE = 8;
static const unsigned int REPLY_FRAME_SIZE = 10;
static const unsigned int EVENT_FRAME_SIZE = 6;

static void writeU16LE(uint8_t* buf, uint16_t v) {
    buf[0] = (uint8_t)(v & 0xFF);
    buf[1] = (uint8_t)((v >> 8) & 0xFF);
}

static uint16_t readU16LE(const uint8_t* buf) {
    return (uint16_t)(buf[0] | (buf[1] << 8));
}

static void writeI32LE(uint8_t* buf, int32_t v) {
    uint32_t u = (uint32_t)v;
    buf[0] = (uint8_t)(u & 0xFF);
    buf[1] = (uint8_t)((u >> 8) & 0xFF);
    buf[2] = (uint8_t)((u >> 16) & 0xFF);
    buf[3] = (uint8_t)((u >> 24) & 0xFF);
}

static int32_t readI32LE(const uint8_t* buf) {
    uint32_t u = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
                 ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    return (int32_t)u;
}

static void writeF32LE(uint8_t* buf, float v) {
    uint32_t bits;
    memcpy(&bits, &v, sizeof(bits));
    writeI32LE(buf, (int32_t)bits);
}

static float readF32LE(const uint8_t* buf) {
    uint32_t bits = (uint32_t)readI32LE(buf);
    float v;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

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
    uint8_t buf[EVENT_FRAME_SIZE];
    writeU16LE(buf, cap);
    writeF32LE(buf + 2, (float)value);
    return _mqtt->publish(_topicEvent, buf, sizeof(buf), false);
}

void MeoMessaging::_onMessageStatic(const char* topic, const uint8_t* payload, unsigned int length, void* ctx) {
    MeoMessaging* self = reinterpret_cast<MeoMessaging*>(ctx);
    if (!self || !topic || strcmp(topic, self->_topicCommand) != 0) return;
    self->_handleCommand(payload, length);
}

void MeoMessaging::_handleCommand(const uint8_t* payload, unsigned int length) {
    if (length != COMMAND_FRAME_SIZE) {
        _logf("WARN", "MSG", "Dropping command: bad length %u", length);
        return;
    }

    uint16_t requestId = readU16LE(payload);
    uint16_t cap = readU16LE(payload + 2);
    int32_t value = readI32LE(payload + 4);

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

void MeoMessaging::_replyOkInt(uint16_t requestId, uint16_t cap, int32_t value) {
    uint8_t buf[REPLY_FRAME_SIZE];
    writeU16LE(buf, requestId);
    buf[2] = 1;
    writeU16LE(buf + 3, cap);
    writeI32LE(buf + 5, value);
    buf[9] = 0;
    _mqtt->publish(_topicReply, buf, sizeof(buf), false);
}

void MeoMessaging::_replyOkDouble(uint16_t requestId, uint16_t cap, double value) {
    uint8_t buf[REPLY_FRAME_SIZE];
    writeU16LE(buf, requestId);
    buf[2] = 1;
    writeU16LE(buf + 3, cap);
    writeF32LE(buf + 5, (float)value);
    buf[9] = 0;
    _mqtt->publish(_topicReply, buf, sizeof(buf), false);
}

void MeoMessaging::_replyError(uint16_t requestId, int error) {
    uint8_t buf[REPLY_FRAME_SIZE];
    writeU16LE(buf, requestId);
    buf[2] = 0;
    writeU16LE(buf + 3, 0);
    writeI32LE(buf + 5, 0);
    buf[9] = (uint8_t)error;
    _mqtt->publish(_topicReply, buf, sizeof(buf), false);
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
