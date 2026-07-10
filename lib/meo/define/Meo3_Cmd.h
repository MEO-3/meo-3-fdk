#pragma once

// Shared command catalog — must stay value-for-value in sync with the gateway's
// MeoCmd.java. Capability ids are exchanged as plain integers (BLE capability
// report, MQTT command/event payloads — see mqtt_messaging.md).
//
// The id range encodes the action; there is no separate verb field:
// - MEO_CMD_*   generic device commands every firmware supports implicitly
//               (not declared during provisioning)
// - MEO_EVENT_* edge-triggered events the device emits unsolicited
// - MEO_READ_*  readable sensor values
// - MEO_WRITE_* actuator writes (single scalar value; multi-parameter
//               actuators pack into one int, e.g. LED_RGB takes 0xRRGGBB)
#define MEO_CMD_GENERIC             0x0001
#define MEO_CMD_WRITE               0x0002
#define MEO_CMD_READ                0x0003
#define MEO_CMD_EXECUTE             0x0004
#define MEO_CMD_EXECUTE_WITH_VAR    0x0005
#define MEO_CMD_STOP                0x0006

#define MEO_EVENT_GENERIC           0xE000
#define MEO_EVENT_BUTTON            0xE001

#define MEO_READ_GENERIC            0xF000
#define MEO_READ_TEMP               0xF001
#define MEO_READ_HUMID              0xF002
#define MEO_READ_PRESSURE           0xF003
#define MEO_READ_CO2                0xF004
#define MEO_READ_PM25               0xF005
#define MEO_READ_DISTANCE           0xF006

#define MEO_WRITE_GENERIC           0xFF00
#define MEO_WRITE_LED               0xFF01
#define MEO_WRITE_LED_RGB           0xFF02
#define MEO_WRITE_BUZZER            0xFF03
#define MEO_WRITE_MOTOR             0xFF04
#define MEO_WRITE_SERVO             0xFF05
