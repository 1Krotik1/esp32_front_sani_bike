#include <TFT_eSPI.h>
#include "Saninokia.h"
#include "protocol_can.h"
#include <Arduino.h>

// --- MCP2515 CAN ---
#define MCP2515_CS_PIN    17
#define MCP2515_INT_PIN   16

#include <SPI.h>
#include <mcp_can.h>
MCP_CAN CAN(MCP2515_CS_PIN);
bool canInitialized = false;
uint32_t canRxCount = 0;
uint32_t canLastDiagMs = 0;
uint32_t canLastTxTestMs = 0;
constexpr bool kEnableCanTxTest = false;

TFT_eSPI tft = TFT_eSPI();



// --- Отрисовка индикатора батареи 1 ---
void drawBattery1(const uint32_t color) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRect(5, 10, 30, 20, color);
    tft.setTextSize(0);
    tft.setCursor(9, 16);
    tft.println("БАТ1");

    

    
}
// --- Отрисовка стрелки приоритета батареи 1 ---
void drawBattery1Prime() {
    tft.drawLine(10, 35, 10, 44, TFT_WHITE);
    tft.drawLine(10, 35, 13, 38, TFT_WHITE);
    tft.drawLine(10, 35, 7, 38, TFT_WHITE);
}
// --- Отрисовка линии от батареи 1 к шине ---
void drawBattery1ToMainLineVoltage(const uint32_t color) {
    tft.drawLine(30, 30, 30, 44, color);
}

// --- Отрисовка индикатора батареи 2 ---
void drawBattery2(const uint32_t color) {
    tft.drawRect(55, 10, 30, 20, color);
    tft.setTextSize(0);
    tft.setCursor(59, 16);
    tft.println("БАТ2");

    

    
}

// --- Отрисовка линии от батареи 2 к шине ---
void drawBattery2ToMainLineVoltage(const uint32_t color ) {
    tft.drawLine(60, 30, 60, 44, color);
}

// --- Отрисовка стрелки приоритета батареи 2 ---
void drawBattery2Prime() {
    tft.drawLine(80, 35, 80, 44, TFT_WHITE);
    tft.drawLine(80, 35, 83, 38, TFT_WHITE);
    tft.drawLine(80, 35, 77, 38, TFT_WHITE);
}
// --- Отрисовка напряжения главной шины ---
void drawMainLineVoltage(const char* value) {
    tft.drawRect(23, 45, 43, 20, TFT_GREEN);
    tft.setTextSize(0);
    tft.setCursor(27, 51);
    tft.println(value);
}

// --- Отрисовка тока главной шины ---
void drawMainLineCurrent(const char* value) {
    tft.drawRect(23, 65, 43, 20, TFT_GREEN);
    tft.setTextSize(0);
    tft.setCursor(27, 71);
    tft.println(value);
}


// --- Отрисовка индикатора APU ---
void drawApu(const uint32_t color) {
    tft.drawLine(45, 85, 45, 99, color);
    tft.drawRect(30, 100, 30, 20, color);
    tft.setTextSize(0);
    tft.setCursor(36, 106);
    tft.println("ВСУ");
}

// --- Отрисовка спидометра ---
void drawSpeedometer(const char* speedValue) {
    tft.drawRect(85, 88, 75, 40, TFT_RED);
    tft.setCursor(91, 98);
    tft.setTextSize(3);
    tft.print(speedValue);
    tft.setTextSize(0);
    tft.println("км/ч");
}

// --- Отрисовка критической ошибки ---
void drawCriticalError(const char* message) {
    tft.drawRect(95, 0, 65, 40, TFT_RED);
    tft.setViewport(99, 0, 55, 40);
    tft.setTextWrap(true, false);
    tft.setCursor(99, 0);
    tft.setTextSize(0);
    tft.println(message);
    tft.resetViewport();
}

// --- Отрисовка предупреждения ---
void drawWarning(const char* message) {
    tft.drawRect(95, 41, 65, 40, TFT_YELLOW);
    tft.setViewport(99, 46, 55, 40);
    tft.setTextWrap(true, true);
    tft.setCursor(99, 52);
    tft.setTextSize(0);
    tft.println(message);
    tft.resetViewport();
}

// --- Отрисовка всего экрана ---
void drawScreen() {
    drawBattery1(TFT_RED);
    drawBattery1ToMainLineVoltage(TFT_RED);
    drawBattery1Prime();
    drawBattery2(TFT_GREEN);
    drawBattery2ToMainLineVoltage(TFT_GREEN);
    drawBattery2Prime();
    drawMainLineVoltage("12.50V");
    drawMainLineCurrent("1.25A");
    drawApu(TFT_RED);
    drawSpeedometer("??");
    drawCriticalError("критическая ошибка");
    drawWarning("предупреждение");
}

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

// --- Обработка входящего CAN-сообщения ---
void handleCanMessage(uint32_t id, const byte* data, uint8_t len) {
    Serial.printf("CAN RX: id=0x%03lX len=%u\n", id, len);
    CanId canId = (CanId)id;
    switch (canId) {
        case CAN_ID_STATUS_DATA: {
            if (len < sizeof(StatusData)) {
                Serial.printf("CAN STATUS_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(StatusData));
                break;
            }
            const StatusData* statusData = (const StatusData*)data;
            Serial.printf(
                "STATUS: main=%u bat1=%u bat2=%u apu=%u\n",
                statusData->status_main_line,
                statusData->status_battery1,
                statusData->status_battery2,
                statusData->status_apu
            );
            break;
        }
        case CAN_ID_MAIN_LINE_DATA: {
            if (len < sizeof(MainLineData)) {
                Serial.printf("CAN MAIN_LINE_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(MainLineData));
                break;
            }
            const MainLineData* mainLineData = (const MainLineData*)data;
            Serial.printf("MAIN_LINE: voltage=%u current=%u\n", mainLineData->voltage, mainLineData->current);
            break;
        }
        case CAN_ID_BATTERY1_DATA: {
            if (len < sizeof(Battery1Data)) {
                Serial.printf("CAN BATTERY1_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(Battery1Data));
                break;
            }
            const Battery1Data* battery1Data = (const Battery1Data*)data;
            Serial.printf("BATTERY1: voltage=%u\n", battery1Data->voltage);
            break;
        }
        case CAN_ID_BATTERY2_DATA: {
            if (len < sizeof(Battery2Data)) {
                Serial.printf("CAN BATTERY2_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(Battery2Data));
                break;
            }
            const Battery2Data* battery2Data = (const Battery2Data*)data;
            Serial.printf("BATTERY2: voltage=%u\n", battery2Data->voltage);
            break;
        }
        default:
            Serial.printf("CAN unknown id=0x%03lX\n", id);
            break;
    }
}

void setup() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    Serial.begin(115200);
    Serial.println("MCP2515 CAN Bus Test (physical bus)");

    pinMode(MCP2515_CS_PIN, OUTPUT);
    pinMode(MCP2515_INT_PIN, INPUT);

    // Кристалл 8 МГц
    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("MCP2515 initialized successfully (500kbps, 8MHz clock)");
        canInitialized = true;
    } else {
        Serial.println("MCP2515 init failed!");
        canInitialized = false;
    }

    if (canInitialized) {
        // Принимаем все стандартные 11-битные ID
        CAN.init_Mask(0, 0, 0x00000000);
        CAN.init_Mask(1, 0, 0x00000000);
        for (int i = 0; i < 6; i++) {
            CAN.init_Filt(i, 0, 0x00000000);
        }

        if (CAN.setMode(MCP_NORMAL) == CAN_OK) {
            Serial.println("CAN mode set to MCP_NORMAL; waiting for physical bus traffic");
        } else {
            Serial.println("CAN mode change to MCP_NORMAL failed");
        }
    }

    drawScreen();
}

void loop() {
    if (!canInitialized) {
        delay(200);
        return;
    }

    // Обработка входящих CAN-сообщений
    byte rxStatus = CAN.checkReceive();
    if (CAN_MSGAVAIL == rxStatus) {
        unsigned long id = 0;
        uint8_t len = 0;
        byte buf[8] = {0};
        CAN.readMsgBuf(&id, &len, buf);
        canRxCount++;
        handleCanMessage(id, buf, len);
    }

    // Периодический вывод состояния шины для диагностики отсутствия RX-кадров
    const uint32_t nowMs = millis();
    if (nowMs - canLastDiagMs >= 2000) {
        canLastDiagMs = nowMs;
        uint8_t errFlags = CAN.getError();
        byte errStatus = CAN.checkError();
        Serial.printf(
            "CAN diag: rx=%lu INT=%u rxStatus=%u err=%u eflg=0x%02X\n",
            (unsigned long)canRxCount,
            (unsigned)digitalRead(MCP2515_INT_PIN),
            (unsigned)rxStatus,
            (unsigned)errStatus,
            (unsigned)errFlags
        );
        printCanErrorFlags(errFlags);
        byte txData[1] = {0xA5};
        byte txResult = CAN.sendMsgBuf(0x100, 0, 1, txData);
        Serial.printf("CAN TX test: id=0x100 result=%u\n", (unsigned)txResult);
    }

    // Тестовый TX-кадр отключён по умолчанию: на одиночной шине без ACK контроллер
    // быстро уходит в TXBO. Для реальной проверки нужен хотя бы ещё один узел CAN.
    if (kEnableCanTxTest && nowMs - canLastTxTestMs >= 1000) {
        canLastTxTestMs = nowMs;
        byte txData[1] = {0xA5};
        byte txResult = CAN.sendMsgBuf(0x100, 0, 1, txData);
        Serial.printf("CAN TX test: id=0x100 result=%u\n", (unsigned)txResult);
    }

    delay(10);
}
