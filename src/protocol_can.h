#pragma once
#include <Arduino.h>

enum CanId: uint32_t {
    CAN_ID_HEARTBEAT = 0x001,
    CAN_ID_ERROR_DATA = 0x002,
    CAN_ID_MAIN_LINE_DATA = 0x003,
    CAN_ID_BATTERY1_DATA = 0x004,
    CAN_ID_BATTERY2_DATA = 0x005,
    CAN_ID_STATUS_BATTERY1 = 0x006,
    CAN_ID_STATUS_BATTERY2 = 0x007,
    // Add more CAN IDs as needed
};
#pragma pack(push, 1)
struct HeartbeatData {
    uint8_t status; // Status code
};
struct ErrorData {
    uint8_t error_no_signal_main_line; // Error code for no signal on main line
    uint8_t error_no_signal_battery1; // Error code for no signal on battery 1
    uint8_t error_no_signal_battery2; // Error code for no signal on battery 2
};
struct MainLineData {
    uint16_t voltage; // Voltage in volts
    uint16_t current; // Current in amperes
};
struct Battery1Data {
    uint16_t voltage; // Voltage in volts
};
struct Battery2Data {
    uint16_t voltage; // Voltage in volts
};
struct StatusBattery1 {
    uint8_t status; // Status code
};
struct StatusBattery2 {
    uint8_t status; // Status code
};

#pragma pack(pop)