#pragma once

#include "Meo3_Type.h"

class MeoRegistrationClient {
public:
    MeoRegistrationClient();

    void setGateway(const char* host, uint16_t port);
    void setLogger(MeoLogFunction logger);

    // Perform registration over TCP if no credentials exist.
    // Returns true on success and fills deviceId and transmitKey.
    bool registerIfNeeded(const MeoDeviceInfo& devInfo,
                          const MeoFeatureRegistry& features,
                          String& deviceIdOut,
                          String& transmitKeyOut);

private:
    String         _gatewayHost;
    uint16_t       _port;
    MeoLogFunction _logger;

    bool _sendRegistrationRequest(const MeoDeviceInfo& devInfo,
                                  const MeoFeatureRegistry& features,
                                  String& responseJson);
    bool _parseRegistrationResponse(const String& json,
                                    String& deviceIdOut,
                                    String& transmitKeyOut);
};