// Control the built-in LED from the gateway.
//
// Declares MEO_WRITE_LED and drives LED_BUILTIN with it. The gateway sends the
// command over MQTT and waits for this device's reply:
//
//   curl -X POST http://<gateway>:7070/api/v1/devices/<deviceId>/command \
//        -H 'Content-Type: application/json' -d '{"cap":65281,"value":1}'
//
// cap 65281 is 0xFF01 (MEO_WRITE_LED); value 0 turns the LED off, non-zero on.
// deviceId is the device's Wi-Fi MAC, lowercase hex without separators.
//
// On first boot the device is unprovisioned and advertises over BLE; provision
// it from the gateway before sending commands. To skip provisioning while
// developing, uncomment the beginWifi()/setBroker() calls below.

#include <Arduino.h>
#include <Meo3.h>
#include "define/Meo3_Cmd.h"

#define LED_BUILTIN 8

MeoDevice meo("MEO LED Demo");

// MEO_WRITE_LED handler. Returning false replies MEO_ERR_HANDLE_FAILED to the
// gateway; here the write always succeeds.
static bool handleLed(int32_t value) {
    digitalWrite(LED_BUILTIN, value ? LOW : HIGH);
    Serial.printf("[LED] %s\n", value ? "on" : "off");
    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(500); // let USB CDC enumerate before the first print

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    meo.setLogger([](const char *level, const char *message)
                  { Serial.printf("[%s] %s\n", level, message); });
    meo.setDebugTags("DEVICE,PROV,MQTT,MSG");

    // Registering the handler also declares the capability, which is what the
    // gateway reads off the BLE capability characteristic while provisioning.
    // Must happen before begin().
    meo.onCommand(MEO_WRITE_LED, handleLed);

    // Development shortcut — bypasses BLE provisioning:
    // meo.beginWifi("your-ssid", "your-password");
    // meo.setBroker("192.168.1.10", 1883);

    if (!meo.begin())
    {
        Serial.println("[ERROR] begin() failed - halting");
        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(meo.isProvisioned()
                       ? "[INFO] Provisioned - waiting for commands"
                       : "[INFO] Not provisioned - BLE advertising");
}

void loop()
{
    meo.loop();
}
