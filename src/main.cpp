#include <TFT_eSPI.h>
#include "Saninokia.h"
#include "protocol_can.h"
#include <Arduino.h>
#include <stdio.h>

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

// --- Обработка ошибок ---
#define CAN_TIMEOUT_MS    500


// --- Кнопки выбора батареи ---
#define BAT1_SELECT_PIN   26
#define BAT2_SELECT_PIN   27

uint32_t canLastRxMs = 0;
bool dataReceived = false;
uint8_t selectedBattery = 1; // 1 = Bat1, 2 = Bat2

StatusData currentStatus;
MainLineData currentMainLine;
Battery1Data currentBat1;
Battery2Data currentBat2;

enum ErrorState : uint8_t {
    ERR_OK = 0,
    ERR_WARNING,
    ERR_CRITICAL
};
ErrorState errorState = ERR_OK;
uint32_t lastErrorUpdateMs = 0;
bool errorBlinkState = false;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft); // Спрайт на весь экран
bool useSprite = false; // true если спрайт создан успешно

// Макрос: рисовать на спрайте если он есть, иначе напрямую на tft
#define DRAW (useSprite ? spr : tft)

// --- Цвет по статусу модуля ---
uint32_t getStatusColor(uint8_t status) {
    if (status == INA226_OK || status == BAT_ADC_OK || status == APU_OK) return TFT_GREEN;
    if (status == INA226_ERR_I2C || status == INA226_ERR_OV || status == INA226_ERR_OC ||
        status == BAT_ADC_ERR_OPEN || status == BAT_ADC_ERR_OVER || status == APU_ERR) return TFT_RED;
    return TFT_YELLOW;
}

// --- Пороги защиты батарей ---
#define BAT_UNDER_VOLTAGE  300  // 3.00 В * 100
#define BAT_OVER_VOLTAGE   520  // 4.20 В * 100

// --- Отрисовка индикатора батареи 1 ---
void drawBattery1(const uint32_t color) {
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.drawRect(5, 10, 30, 20, color);
    DRAW.setTextSize(0);
    DRAW.setCursor(9, 16);
    DRAW.println("BAT1");
}

// --- Отрисовка стрелки приоритета батареи 1 ---
void drawBattery1Prime() {
    DRAW.drawLine(10, 35, 10, 44, TFT_WHITE);
    DRAW.drawLine(10, 35, 13, 38, TFT_WHITE);
    DRAW.drawLine(10, 35, 7, 38, TFT_WHITE);
}

// --- Отрисовка стрелки приоритета батареи 2 ---
void drawBattery2Prime() {
    DRAW.drawLine(80, 35, 80, 44, TFT_WHITE);
    DRAW.drawLine(80, 35, 83, 38, TFT_WHITE);
    DRAW.drawLine(80, 35, 77, 38, TFT_WHITE);
}

// --- Отрисовка линии от батареи 1 к шине ---
void drawBattery1ToMainLineVoltage(const uint32_t color) {
    DRAW.drawLine(30, 30, 30, 44, color);
}

// --- Отрисовка индикатора батареи 2 ---
void drawBattery2(const uint32_t color) {
    DRAW.drawRect(55, 10, 30, 20, color);
    DRAW.setTextSize(0);
    DRAW.setCursor(59, 16);
    DRAW.println("BAT2");
}

// --- Отрисовка линии от батареи 2 к шине ---
void drawBattery2ToMainLineVoltage(const uint32_t color) {
    DRAW.drawLine(60, 30, 60, 44, color);
}



// --- Отрисовка напряжения главной шины ---
void drawMainLineVoltage(const char* value, const uint32_t color) {
    DRAW.fillRect(23, 45, 43, 20, TFT_BLACK);
    DRAW.drawRect(23, 45, 43, 20, color);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setTextSize(0);
    DRAW.setCursor(27, 51);
    DRAW.println(value);
}

// --- Отрисовка тока главной шины ---
void drawMainLineCurrent(const char* value, const uint32_t color) {
    DRAW.fillRect(23, 65, 43, 20, TFT_BLACK);
    DRAW.drawRect(23, 65, 43, 20, color);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setCursor(27, 71);
    DRAW.println(value);
}

// --- Отрисовка индикатора APU ---
void drawApu(const uint32_t color) {
    DRAW.fillRect(30, 100, 30, 20, TFT_BLACK);
    DRAW.drawLine(45, 85, 45, 99, color);
    DRAW.drawRect(30, 100, 30, 20, color);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setTextSize(0);
    DRAW.setCursor(36, 106);
    DRAW.println("APU");
}

// --- Отрисовка спидометра ---
void drawSpeedometer(const char* speedValue, const uint32_t frameColor) {
    DRAW.fillRect(85, 88, 75, 40, TFT_BLACK);
    DRAW.drawRect(85, 88, 75, 40, frameColor);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setCursor(91, 98);
    DRAW.setTextSize(3);
    DRAW.print(speedValue);
    DRAW.setTextSize(0);
    DRAW.println("km/h");
}

// --- Отрисовка ошибок (белый текст, цветная рамка) ---
void drawError(const char* message, const uint32_t frameColor) {
    DRAW.fillRect(95, 0, 65, 40, TFT_BLACK);
    DRAW.drawRect(95, 0, 65, 40, frameColor);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setTextSize(0);
    DRAW.setCursor(99, 10);
    DRAW.println(message);
}
// --- Отрисовка времени от gps ---
void drawTime(const char* message, const uint32_t frameColor) {
    DRAW.fillRect(95, 41, 65, 40, TFT_BLACK);
    DRAW.drawRect(95, 41, 65, 40, frameColor);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setTextSize(0);
    DRAW.setCursor(99, 51);
    DRAW.println(message);
}

// --- Револвер ошибок ---
// Список всех возможных ошибок с приоритетом (критические первые)
struct ErrorEntry {
    const char* message;
    ErrorState state;
    bool active;
};

ErrorEntry errorList[] = {
    {"CAN TIMEOUT",       ERR_CRITICAL, false},
    {"INA226 ERR",        ERR_CRITICAL, false},
    {"BAT1 OPEN",         ERR_WARNING,  false},
    {"BAT1 OVER",         ERR_WARNING,  false},
    {"BAT2 OPEN",         ERR_WARNING,  false},
    {"BAT2 OVER",         ERR_WARNING,  false},
    {"APU ERR",           ERR_WARNING,  false},
};
constexpr size_t kErrorCount = sizeof(errorList) / sizeof(errorList[0]);

uint8_t currentErrorIndex = 0;
uint32_t lastErrorSwitchMs = 0;
constexpr uint32_t ERROR_SWITCH_INTERVAL_MS = 3000;

// --- Проверка ошибок батарей по статусу ---
// Возвращает true, если есть активная ошибка батареи
bool checkBatteryErrors(uint8_t batStatus, uint8_t errOpenIdx, uint8_t errOverIdx) {
    if (batStatus == BAT_ADC_ERR_OPEN) {
        errorList[errOpenIdx].active = true;
        return true;
    } else if (batStatus == BAT_ADC_ERR_OVER) {
        errorList[errOverIdx].active = true;
        return true;
    }
    return false;
}

// --- Обновление списка активных ошибок на основе статусов ---
void refreshErrorList() {
    // Сбрасываем все ошибки
    for (size_t i = 0; i < kErrorCount; i++) {
        errorList[i].active = false;
    }

    // CAN timeout
    if (!dataReceived) {
        errorList[0].active = true;
    }

    // INA226
    if (currentStatus.status_main_line != INA226_OK) {
        errorList[1].active = true;
    }

    // Battery 1 — проверка статуса по CAN
    checkBatteryErrors(currentStatus.status_battery1, 2, 3);

    // Battery 2 — проверка статуса по CAN
    checkBatteryErrors(currentStatus.status_battery2, 4, 5);

    // APU
    if (currentStatus.status_apu == APU_ERR) {
        errorList[6].active = true;
    }
}

// --- Поиск следующей активной ошибки (циклический перебор всех активных) ---
int8_t findNextActiveError(int8_t currentIdx) {
    // Собираем все активные ошибки в порядке массива
    uint8_t activeIndices[kErrorCount];
    uint8_t activeCount = 0;
    for (size_t i = 0; i < kErrorCount; i++) {
        if (errorList[i].active) {
            activeIndices[activeCount++] = i;
        }
    }
    if (activeCount == 0) return -1;

    // Находим текущую в списке и возвращаем следующую
    for (uint8_t i = 0; i < activeCount; i++) {
        if (activeIndices[i] == currentIdx) {
            return activeIndices[(i + 1) % activeCount];
        }
    }
    // Если текущая не активна — возвращаем первую из списка
    return activeIndices[0];
}

// --- Обновление отображения ошибок (револвер) ---
void updateErrorDisplay() {
    refreshErrorList();

    const uint32_t nowMs = millis();

    // Находим активную ошибку
    int8_t activeIdx = findNextActiveError(currentErrorIndex);

    if (activeIdx < 0) {
        // Нет ошибок — очищаем область
        DRAW.fillRect(95, 0, 65, 40, TFT_BLACK);
        errorState = ERR_OK;
        return;
    }

    // Если текущая ошибка не активна — переключаемся на первую найденную
    if (!errorList[currentErrorIndex].active) {
        currentErrorIndex = activeIdx;
        lastErrorSwitchMs = nowMs;
    }

    // Переключаем ошибки каждые ERROR_SWITCH_INTERVAL_MS
    if (nowMs - lastErrorSwitchMs >= ERROR_SWITCH_INTERVAL_MS) {
        lastErrorSwitchMs = nowMs;
        // Ищем следующую ошибку после текущей
        int8_t nextIdx = findNextActiveError(currentErrorIndex);
        if (nextIdx >= 0) {
            currentErrorIndex = nextIdx;
        }
    }

    // Отображаем текущую ошибку
    ErrorEntry& entry = errorList[currentErrorIndex];
    uint32_t color = (entry.state == ERR_CRITICAL) ? TFT_RED : TFT_YELLOW;
    errorState = entry.state;
    drawError(entry.message, color);

    if (useSprite) spr.pushSprite(0, 0);
}

// --- Полная отрисовка статичных элементов (вызывается один раз) ---
void drawStaticScreen() {
    DRAW.fillScreen(TFT_BLACK);

    uint32_t bat1Color = getStatusColor(currentStatus.status_battery1);
    uint32_t bat2Color = getStatusColor(currentStatus.status_battery2);

    drawBattery1(bat1Color);
    drawBattery1ToMainLineVoltage(bat1Color);
    drawBattery1Prime();

    drawBattery2(bat2Color);
    drawBattery2ToMainLineVoltage(bat2Color);
    drawBattery2Prime();

    drawApu(getStatusColor(currentStatus.status_apu));
    drawSpeedometer("??", TFT_RED);
    drawTime("--:--", TFT_RED);

    if (useSprite) spr.pushSprite(0, 0);
}

// --- Обновление только данных (без полной очистки) ---
void updateDynamicScreen() {
    uint32_t mainLineColor = getStatusColor(currentStatus.status_main_line);
    uint32_t dataColor = dataReceived ? TFT_GREEN : TFT_RED;

    // Обновляем напряжение и ток
    char mainVoltage[16], mainCurrent[16];
    snprintf(mainVoltage, sizeof(mainVoltage), "%.2fV", currentMainLine.voltage / 100.0f);
    snprintf(mainCurrent, sizeof(mainCurrent), "%.2fA", currentMainLine.current / 100.0f);
    drawMainLineVoltage(mainVoltage, mainLineColor);
    drawMainLineCurrent(mainCurrent, mainLineColor);

    // Обновляем цвета батарей если изменились
    static uint32_t lastBat1Color = 0, lastBat2Color = 0;
    uint32_t bat1Color = getStatusColor(currentStatus.status_battery1);
    uint32_t bat2Color = getStatusColor(currentStatus.status_battery2);

    if (bat1Color != lastBat1Color) {
        drawBattery1(bat1Color);
        drawBattery1ToMainLineVoltage(bat1Color);
        lastBat1Color = bat1Color;
    }
    if (bat2Color != lastBat2Color) {
        drawBattery2(bat2Color);
        drawBattery2ToMainLineVoltage(bat2Color);
        lastBat2Color = bat2Color;
    }

    // Обновляем стрелки приоритета
    static uint8_t lastSelectedBattery = 0;
    if (selectedBattery != lastSelectedBattery) {
        if (selectedBattery == 1) {
            drawBattery1Prime();
            DRAW.fillRect(75, 35, 10, 10, TFT_BLACK);
        } else {
            drawBattery2Prime();
            DRAW.fillRect(5, 35, 10, 10, TFT_BLACK);
        }
        lastSelectedBattery = selectedBattery;
    }

    // Обновляем APU если изменился
    static uint32_t lastApuColor = 0;
    uint32_t apuColor = getStatusColor(currentStatus.status_apu);
    if (apuColor != lastApuColor) {
        drawApu(apuColor);
        lastApuColor = apuColor;
    }

    // Обновляем цвет спидометра/времени
    static uint32_t lastDataColor = 0;
    if (dataColor != lastDataColor) {
        drawSpeedometer("??", dataColor);
        drawTime("--:--", dataColor);
        lastDataColor = dataColor;
    }

    if (useSprite) spr.pushSprite(0, 0);
}

// --- Отрисовка всего экрана (для совместимости) ---
void drawScreen() {
    drawStaticScreen();
    updateDynamicScreen();
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
bool dataChanged = false; // флаг: пришли новые данные — нужно перерисовать экран

void handleCanMessage(uint32_t id, const byte* data, uint8_t len) {
    canLastRxMs = millis();
    dataReceived = true;
    dataChanged = true;
    Serial.printf("CAN RX: id=0x%03lX len=%u\n", id, len);
    CanId canId = (CanId)id;
    switch (canId) {
        case CAN_ID_STATUS_DATA: {
            if (len < sizeof(StatusData)) {
                Serial.printf("CAN STATUS_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(StatusData));
                break;
            }
            const StatusData* statusData = (const StatusData*)data;
            currentStatus = *statusData;
            Serial.printf("STATUS: main=%u bat1=%u bat2=%u can=%u\n",
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
            currentMainLine = *mainLineData;
            Serial.printf("MAIN_LINE: voltage=%u current=%u\n", mainLineData->voltage, mainLineData->current);
            break;
        }
        case CAN_ID_BATTERY1_DATA: {
            if (len < sizeof(Battery1Data)) {
                Serial.printf("CAN BATTERY1_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(Battery1Data));
                break;
            }
            const Battery1Data* battery1Data = (const Battery1Data*)data;
            currentBat1 = *battery1Data;
            Serial.printf("BATTERY1: voltage=%u\n", battery1Data->voltage);
            break;
        }
        case CAN_ID_BATTERY2_DATA: {
            if (len < sizeof(Battery2Data)) {
                Serial.printf("CAN BATTERY2_DATA: invalid len=%u expected=%u\n", len, (unsigned)sizeof(Battery2Data));
                break;
            }
            const Battery2Data* battery2Data = (const Battery2Data*)data;
            currentBat2 = *battery2Data;
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
    // При setRotation(1) реальные размеры: 160x128 (ширина x высота)
    if (spr.createSprite(160, 128) != nullptr) {
        useSprite = true;
        Serial.println("Sprite created OK (160x128)");
    } else {
        useSprite = false;
        Serial.println("Sprite FAILED — fallback to direct TFT drawing");
    }
    pinMode(BAT1_SELECT_PIN, INPUT_PULLUP);
    pinMode(BAT2_SELECT_PIN, INPUT_PULLUP);
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

    const uint32_t nowMs = millis();

    // Проверка таймаута (нет данных > 500 мс)
    if (dataReceived && nowMs - canLastRxMs >= CAN_TIMEOUT_MS) {
        dataReceived = false;
        errorState = ERR_CRITICAL;
        Serial.println("CAN TIMEOUT: no data from Nano!");
    }

    // Обновление отображения ошибок
    updateErrorDisplay();

    // Перерисовка экрана при изменении статуса приёма CAN (первый приход/таймаут)
    // или при получении новых данных
    static bool lastDataReceived = false;
    if (dataReceived != lastDataReceived || dataChanged) {
        lastDataReceived = dataReceived;
        dataChanged = false;
        updateDynamicScreen(); // частичное обновление без мерцания
    }

    // Периодический вывод состояния шины для диагностики отсутствия RX-кадров
    if (nowMs - canLastDiagMs >= 2000) {
        canLastDiagMs = nowMs;
        uint8_t errFlags = CAN.getError();
        byte errStatus = CAN.checkError();
        Serial.printf("CAN diag: rx=%lu INT=%u rxStatus=%u err=%u eflg=0x%02X\n",
            (unsigned long)canRxCount,
            (unsigned)digitalRead(MCP2515_INT_PIN),
            (unsigned)rxStatus,
            (unsigned)errStatus,
            (unsigned)errFlags
        );
        printCanErrorFlags(errFlags);
        
    }
// Обработка кнопок выбора батареи
    static uint8_t prevBat1 = 1, prevBat2 = 1;
    uint8_t curBat1 = digitalRead(BAT1_SELECT_PIN);
    uint8_t curBat2 = digitalRead(BAT2_SELECT_PIN);

    if (curBat1 == 0 && prevBat1 == 1) {
        selectedBattery = 1;
        updateDynamicScreen();
    }
    if (curBat2 == 0 && prevBat2 == 1) {
        selectedBattery = 2;
        updateDynamicScreen();
    }
    prevBat1 = curBat1;
    prevBat2 = curBat2;

    
    

    delay(10);
}
