#pragma once

// This cmd combine to the byte payload
// Payload format: [cmd] [number_define (uint_8)] [value (2 byte)]
// Block cmd contain 2 bytes, number define unsigned 1 byte, and value is 2 byte, value base on command to be signed or unsigned
#define MEO_CMD_GENERIC             0x0001
#define MEO_CMD_WRITE               0x0002
#define MEO_CMD_READ                0x0003
#define MEO_CMD_EXECUTE             0x0004
#define MEO_CMD_EXECUTE_WITH_VAR    0x0005
#define MEO_CMD_STOP                0x0006

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
