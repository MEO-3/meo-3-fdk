#pragma once

#include <Arduino.h>
#include "../mqtt/Meo3_Mqtt.h"
#include "../Meo3_Type.h" // MeoLogFunction

/**
 * MeoMessaging: runtime MQTT messaging per
 * meo-3-open-service/docs/mqtt_messaging.md.
 * - Owns the device topic namespace (command/reply/event)
 * - Parses/builds the fixed-size little-endian binary frames (no JSON)
 * - Dispatches incoming commands to registered capability handlers
 * - Builds and publishes the reply for every command
 * - Publishes unsolicited events/readings
 * MeoDevice wires it to MeoMqttClient once Wi-Fi is up; from then on this
 * class owns connect/reconnect and message handling.
 */
class MeoMessaging {
public:
    // Actuator / generic command handler: receives the command value,
    // returns false to reply with MEO_ERR_HANDLE_FAILED.
    typedef bool (*MeoWriteHandler)(int32_t value);
    // Sensor read handler: returns the value carried in the reply.
    typedef double (*MeoReadHandler)();

    // Logging
    void setLogger(MeoLogFunction logger);
    void setDebugTags(const char* tagsCsv); // enables DEBUG for "MSG" when tag present

    // Handler registration (call in setup(); fixed-size tables, registering a
    // cap again replaces its handler). Returns false when the table is full.
    bool onCommand(uint16_t cap, MeoWriteHandler fn);
    bool onRead(uint16_t cap, MeoReadHandler fn);

    // Bind transport + identity and build the topic strings. deviceId is the
    // lowercase-hex MAC (topic-ready). The mqtt client must already be
    // configured with broker host/port.
    bool begin(MeoMqttClient* mqtt, const char* deviceId);

    // Drive connect/reconnect (retry every 5s) and incoming dispatch.
    void loop();

    bool isConnected();

    // Publish the 6-byte event frame (cap + float32 value) to the event topic
    // (readings + events).
    bool sendEvent(uint16_t cap, double value);

private:
    static const uint8_t  MEO_MAX_HANDLERS = 32;
    static const uint32_t RECONNECT_INTERVAL_MS = 5000;

    MeoMqttClient* _mqtt = nullptr;

    char _topicCommand[48] = {0};
    char _topicReply[48] = {0};
    char _topicEvent[48] = {0};

    uint16_t        _writeCaps[MEO_MAX_HANDLERS];
    MeoWriteHandler _writeFns[MEO_MAX_HANDLERS];
    uint8_t         _writeCount = 0;

    uint16_t       _readCaps[MEO_MAX_HANDLERS];
    MeoReadHandler _readFns[MEO_MAX_HANDLERS];
    uint8_t        _readCount = 0;

    uint32_t _lastConnectAttempt = 0;
    bool     _subscribed = false;

    // Logging
    MeoLogFunction _logger = nullptr;
    char           _debugTags[96] = {0};

    static void _onMessageStatic(const char* topic, const uint8_t* payload, unsigned int length, void* ctx);
    void _handleCommand(const uint8_t* payload, unsigned int length);

    void _replyOkInt(uint16_t requestId, uint16_t cap, int32_t value);
    void _replyOkDouble(uint16_t requestId, uint16_t cap, double value);
    void _replyError(uint16_t requestId, int error);

    bool _debugTagEnabled(const char* tag) const;
    void _log(const char* level, const char* tag, const char* msg) const;
    void _logf(const char* level, const char* tag, const char* fmt, ...) const;
};
