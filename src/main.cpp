#include <TFT_eSPI.h>
#include "Saninokia.h"
#include "protocol_can.h"
#include <Arduino.h>

#define CAN_2515
#include <SPI.h>
#include <mcp2515_can.h>
mcp2515_can CAN(MCP2515_CS_PIN);

TFT_eSPI tft = TFT_eSPI();

// --- MCP2515 CAN ---
#define MCP2515_CS_PIN    17
#define MCP2515_INT_PIN   16



// --- Отрисовка индикатора батареи 1 ---
void drawBattery1(const uint32_t color) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRect(5, 10, 30, 20, color);
    tft.setTextSize(0);
    tft.setCursor(9, 16);
    tft.println("BAT1");

    

    
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
    tft.println("BAT2");

    

    
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
    tft.println("APU");
}

// --- Отрисовка спидометра ---
void drawSpeedometer(const char* speedValue) {
    tft.drawRect(85, 88, 75, 40, TFT_RED);
    tft.setCursor(91, 98);
    tft.setTextSize(3);
    tft.print(speedValue);
    tft.setTextSize(0);
    tft.println("km/h");
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
    drawCriticalError("critical error");
    drawWarning("warning");
}

// --- Обработка входящего CAN-сообщения ---
void handleCanMessage(uint32_t id, const byte* data, uint8_t len) {
    CanId canId = (CanId)id;
    switch (canId) {
        case CAN_ID_STATUS_DATA: {
            const StatusData* statusData = (const StatusData*)data;
            // Обработка данных статуса
            break;
        }
        case CAN_ID_MAIN_LINE_DATA: {
            const MainLineData* mainLineData = (const MainLineData*)data;
            // Обработка данных главной шины
            break;
        }
        case CAN_ID_BATTERY1_DATA: {
            const Battery1Data* battery1Data = (const Battery1Data*)data;
            // Обработка данных батареи 1
            break;
        }
        case CAN_ID_BATTERY2_DATA: {
            const Battery2Data* battery2Data = (const Battery2Data*)data;
            // Обработка данных батареи 2
            break;
        }
        default:
            break;
    }
}

void setup() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // --- Инициализация MCP2515 ---
    Serial.begin(115200);
    Serial.println("MCP2515 CAN Bus Test");

    // Настройка пинов
    pinMode(MCP2515_CS_PIN, OUTPUT);
    pinMode(MCP2515_INT_PIN, INPUT);

    // Инициализация CAN (500 kbps)
    for (int i = 0; i < 10; i++) {
        if (CAN_OK == CAN.begin(CAN_500KBPS)) {
            Serial.println("MCP2515 initialized successfully!");
            break;
        }
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
    // Настройка фильтров (приём всех сообщений)
    for (int i = 0; i < 6; i++) {
        CAN.init_Mask(i, 0, 0x7FF);
        CAN.init_Filt(i, 0, 0x000);
    }

    drawScreen();
}

void loop() {
    // Обработка входящих CAN-сообщений
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
        uint32_t id;
        uint8_t len;
        byte buf[8];
        CAN.readMsgBuf(&len, buf);
        id = CAN.getCanId();
        handleCanMessage(id, buf, len);
    }

    delay(10);
}
