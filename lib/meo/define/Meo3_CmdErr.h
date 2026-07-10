#pragma once

// Reply error codes for MQTT command handling (see mqtt_messaging.md "Reply").
// Mirror of the gateway's MeoCmdErrCode.java — must stay value-for-value in sync.
#define MEO_ERR_BAD_REQUEST         1
#define MEO_ERR_UNKNOWN_CAP         2
#define MEO_ERR_HANDLE_FAILED       3
