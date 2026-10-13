#include "can_app.h"

#include <mcp_can.h>

#include "protocol_can.h"

extern MCP_CAN* CAN;
extern uint32_t canLastRxMs;
extern bool dataReceived;
extern StatusData currentStatus;
extern MainLineData currentMainLine;
extern Battery1Data currentBat1;
extern Battery2Data currentBat2;
extern bool dataChanged;

void printCanErrorFlags(const uint8_t errFlags) {
    if (errFlags == 0) {
        Serial.println("CAN eflg: none");
        return;
    }

    if (errFlags & 0x20) Serial.println("CAN eflg: TXBO");
    if (errFlags & 0x10) Serial.println("CAN eflg: TXEP");
    if (errFlags & 0x08) Serial.println("CAN eflg: RXEP");
    if (errFlags & 0x04) Serial.println("CAN eflg: TXWAR");
    if (errFlags & 0x02) Serial.println("CAN eflg: RXWAR");
    if (errFlags & 0x01) Serial.println("CAN eflg: EWARN");
}

void handleCanMessage(uint32_t id, const byte* data, uint8_t len) {
    Serial.printf("CAN RX: id=0x%03lX len=%u\n", id, len);
    const CanId canId = static_cast<CanId>(id);

    switch (canId) {
        case CAN_ID_STATUS_DATA: {
            if (len < 4 || len > sizeof(StatusData)) {
                Serial.printf("CAN STATUS_DATA: invalid len=%u expected 4..%u\n",
                              len, (unsigned)sizeof(StatusData));
                break;
            }
            StatusData statusData = {};
            memcpy(&statusData, data, len);
            currentStatus = statusData;
            canLastRxMs = millis();
            dataReceived = true;
            dataChanged = true;
            Serial.printf("STATUS: main=%u bat1=%u bat2=%u can=%u\n",
                          statusData.status_main_line,
                          statusData.status_battery1,
                          statusData.status_battery2,
                          statusData.status_apu);
            break;
        }
        case CAN_ID_MAIN_LINE_DATA: {
            if (len < sizeof(MainLineData)) {
                Serial.printf("CAN MAIN_LINE_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(MainLineData));
                break;
            }
            const MainLineData* mainLineData = (const MainLineData*)data;
            currentMainLine = *mainLineData;
            canLastRxMs = millis();
            dataReceived = true;
            dataChanged = true;
            Serial.printf("MAIN_LINE: voltage=%u current=%u\n",
                          mainLineData->voltage, mainLineData->current);
            break;
        }
        case CAN_ID_BATTERY1_DATA: {
            if (len < sizeof(Battery1Data)) {
                Serial.printf("CAN BATTERY1_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(Battery1Data));
                break;
            }
            const Battery1Data* battery1Data = (const Battery1Data*)data;
            currentBat1 = *battery1Data;
            canLastRxMs = millis();
            dataReceived = true;
            dataChanged = true;
            Serial.printf("BATTERY1: voltage=%u\n", battery1Data->voltage);
            break;
        }
        case CAN_ID_BATTERY2_DATA: {
            if (len < sizeof(Battery2Data)) {
                Serial.printf("CAN BATTERY2_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(Battery2Data));
                break;
            }
            const Battery2Data* battery2Data = (const Battery2Data*)data;
            currentBat2 = *battery2Data;
            canLastRxMs = millis();
            dataReceived = true;
            dataChanged = true;
            Serial.printf("BATTERY2: voltage=%u\n", battery2Data->voltage);
            break;
        }
        default:
            Serial.printf("CAN unknown id=0x%03lX\n", id);
            break;
    }
}
