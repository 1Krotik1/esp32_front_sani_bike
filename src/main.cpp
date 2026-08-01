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

// --- Данные для дашборда (вычисляются из CAN) ---
float currentSpeed = 0.0f; // Текущая скорость км/ч (0 — не реализовано)
char timeStr[9] = "--:--"; // Время HH:MM (заглушка)

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

// --- Пороги защиты батарей (12В — 3S Li-ion) ---
#define BAT_LOW_VOLTAGE      1050  // 10.50 В * 100 — предупреждение
#define BAT_CRIT_VOLTAGE      900  // 9.00 В * 100 — критическое
#define BAT_HIGH_VOLTAGE     1300  // 13.00 В * 100 — предупреждение
#define BAT_OVER_VOLTAGE     1400  // 14.00 В * 100 — критическое

// --- Цвет по статусу модуля ---
uint32_t getStatusColor(uint8_t status) {
    if (status == INA226_OK || status == BAT_ADC_OK || status == APU_OK) return TFT_GREEN;
    if (status == INA226_ERR_I2C || status == INA226_ERR_OV || status == INA226_ERR_OC ||
        status == BAT_ADC_ERR_OPEN || status == BAT_ADC_ERR_OVER || status == APU_ERR) return TFT_RED;
    return TFT_YELLOW;
}

// --- Цвет батареи: учитывает статус ADC и напряжение ---
uint32_t getBatteryColor(uint8_t status, uint16_t voltage) {
    if (status == BAT_ADC_ERR_OPEN || status == BAT_ADC_ERR_OVER) return TFT_RED;
    if (status != BAT_ADC_OK) return TFT_YELLOW;
    if (voltage < BAT_CRIT_VOLTAGE || voltage > BAT_OVER_VOLTAGE) return TFT_RED;
    if (voltage < BAT_LOW_VOLTAGE || voltage > BAT_HIGH_VOLTAGE) return TFT_YELLOW;
    return TFT_GREEN;
}

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

// --- Отрисовка ошибок ---
void drawError(const char* message, const uint32_t frameColor) {
    DRAW.fillRect(88, 0, 72, 40, TFT_BLACK);
    DRAW.drawRect(88, 0, 72, 40, frameColor);
    DRAW.setTextColor(TFT_WHITE, TFT_BLACK);
    DRAW.setTextSize(0);
    DRAW.setCursor(91, 10);
    DRAW.println(message);
}

// --- Отрисовка времени ---
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
    // Критические — связь и аппаратные ошибки
    {"MCP2515 INIT",      ERR_CRITICAL, false},  // 0  — MCP2515 не инициализировался
    {"CAN TIMEOUT",       ERR_CRITICAL, false},  // 1  — нет данных от Nano >500мс
    {"INA226 I2C",        ERR_CRITICAL, false},  // 2  — INA226 не отвечает по I2C
    {"INA226 OV",         ERR_CRITICAL, false},  // 3  — перенапряжение главной шины
    {"INA226 OC",         ERR_CRITICAL, false},  // 4  — перегруз по току главной шины
    {"INA226 TEMP",       ERR_CRITICAL, false},  // 5  — перегрев INA226 >85°C
    {"BAT1 OPEN",         ERR_CRITICAL, false},  // 6  — обрыв цепи батареи 1
    {"BAT1 OVER",         ERR_CRITICAL, false},  // 7  — перегруз входа ADC батареи 1
    {"BAT1 CRIT",         ERR_CRITICAL, false},  // 8  — критическое напряжение батареи 1
    {"BAT2 OPEN",         ERR_CRITICAL, false},  // 9  — обрыв цепи батареи 2
    {"BAT2 OVER",         ERR_CRITICAL, false},  // 10 — перегруз входа ADC батареи 2
    {"BAT2 CRIT",         ERR_CRITICAL, false},  // 11 — критическое напряжение батареи 2
    // Предупреждения — значения вне диапазона
    {"BAT1 LOW",          ERR_WARNING,  false},  // 12 — низкое напряжение батареи 1
    {"BAT1 HIGH",         ERR_WARNING,  false},  // 13 — высокое напряжение батареи 1
    {"BAT2 LOW",          ERR_WARNING,  false},  // 14 — низкое напряжение батареи 2
    {"BAT2 HIGH",         ERR_WARNING,  false},  // 15 — высокое напряжение батареи 2
    {"APU ERR",           ERR_WARNING,  false},  // 16 — ошибка APU
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

    // MCP2515 — не инициализировался
    if (!canInitialized) {
        errorList[0].active = true;
    }

    // CAN timeout — нет данных >500мс
    if (!dataReceived) {
        errorList[1].active = true;
        // При таймауте CAN остальные статусы недостоверны — не показываем их
        return;
    }

    // INA226 — раздельные ошибки по коду статуса
    switch (currentStatus.status_main_line) {
        case INA226_ERR_I2C:  errorList[2].active = true; break;
        case INA226_ERR_OV:   errorList[3].active = true; break;
        case INA226_ERR_OC:   errorList[4].active = true; break;
        case INA226_ERR_TEMP: errorList[5].active = true; break;
    }

    // Battery 1 — аппаратные ошибки ADC
    checkBatteryErrors(currentStatus.status_battery1, 6, 7);
    // Battery 1 — напряжение (статус приходит с Nano)
    switch (currentStatus.volt_status_bat1) {
        case BAT_VOLT_CRIT: errorList[8].active = true;  break;  // критическое
        case BAT_VOLT_LOW:  errorList[12].active = true; break;  // низкое
        case BAT_VOLT_HIGH: errorList[13].active = true; break;  // высокое
        case BAT_VOLT_OVER: errorList[8].active = true;  break;  // критическое
    }

    // Battery 2 — аппаратные ошибки ADC
    checkBatteryErrors(currentStatus.status_battery2, 9, 10);
    // Battery 2 — напряжение (статус приходит с Nano)
    switch (currentStatus.volt_status_bat2) {
        case BAT_VOLT_CRIT: errorList[11].active = true; break;  // критическое
        case BAT_VOLT_LOW:  errorList[14].active = true; break;  // низкое
        case BAT_VOLT_HIGH: errorList[15].active = true; break;  // высокое
        case BAT_VOLT_OVER: errorList[11].active = true; break;  // критическое
    }

    // APU
    if (currentStatus.status_apu == APU_ERR) {
        errorList[16].active = true;
    }
}

// --- Подсчёт активных ошибок ---
uint8_t activeErrorCount() {
    uint8_t count = 0;
    for (size_t i = 0; i < kErrorCount; i++) {
        if (errorList[i].active) count++;
    }
    return count;
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
// Возвращает true, если есть активные ошибки
bool updateErrorDisplay() {
    refreshErrorList();

    const uint32_t nowMs = millis();

    // Находим активную ошибку
    int8_t activeIdx = findNextActiveError(currentErrorIndex);

    if (activeIdx < 0) {
        errorState = ERR_OK;
        return false;
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

    errorState = errorList[currentErrorIndex].state;
    // Отображаем текущую ошибку
    ErrorEntry& entry = errorList[currentErrorIndex];
    uint32_t color = (entry.state == ERR_CRITICAL) ? TFT_RED : TFT_YELLOW;
    drawError(entry.message, color);
    if (useSprite) spr.pushSprite(0, 0);
    return true;
}

// --- Полная отрисовка статичных элементов ---
void drawStaticScreen() {
    DRAW.fillScreen(TFT_BLACK);

    uint32_t bat1Color = dataReceived ? getBatteryColor(currentStatus.status_battery1, currentBat1.voltage) : TFT_RED;
    uint32_t bat2Color = dataReceived ? getBatteryColor(currentStatus.status_battery2, currentBat2.voltage) : TFT_RED;

    drawBattery1(bat1Color);
    drawBattery1ToMainLineVoltage(bat1Color);
    drawBattery1Prime();

    drawBattery2(bat2Color);
    drawBattery2ToMainLineVoltage(bat2Color);
    drawBattery2Prime();

    drawApu(dataReceived ? getStatusColor(currentStatus.status_apu) : TFT_RED);
    drawSpeedometer("??", TFT_RED);
    drawTime("--:--", TFT_RED);

    if (useSprite) spr.pushSprite(0, 0);
}

// --- Обновление данных ---
void updateDynamicScreen() {
    uint32_t mainLineColor = dataReceived ? getStatusColor(currentStatus.status_main_line) : TFT_RED;

    char mainVoltage[16], mainCurrent[16];
    snprintf(mainVoltage, sizeof(mainVoltage), "%.2fV", currentMainLine.voltage / 100.0f);
    snprintf(mainCurrent, sizeof(mainCurrent), "%.2fA", currentMainLine.current / 100.0f);
    drawMainLineVoltage(mainVoltage, mainLineColor);
    drawMainLineCurrent(mainCurrent, mainLineColor);

    static uint32_t lastBat1Color = 0, lastBat2Color = 0;
    uint32_t bat1Color = dataReceived ? getBatteryColor(currentStatus.status_battery1, currentBat1.voltage) : TFT_RED;
    uint32_t bat2Color = dataReceived ? getBatteryColor(currentStatus.status_battery2, currentBat2.voltage) : TFT_RED;

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

    static uint32_t lastApuColor = 0;
    uint32_t apuColor = dataReceived ? getStatusColor(currentStatus.status_apu) : TFT_RED;
    if (apuColor != lastApuColor) {
        drawApu(apuColor);
        lastApuColor = apuColor;
    }

    static bool speedTimeDrawn = false;
    if (!speedTimeDrawn) {
        drawSpeedometer("??", TFT_RED);
        drawTime("--:--", TFT_RED);
        speedTimeDrawn = true;
    }

    if (useSprite) spr.pushSprite(0, 0);
}

// --- Отрисовка всего экрана ---
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

    // Обновление состояния ошибок (без отрисовки)
    updateErrorDisplay();

    // Перерисовка экрана при изменении статуса приёма CAN (первый приход/таймаут)
    // или при получении новых данных
    static bool lastDataReceived = false;
    if (dataReceived != lastDataReceived || dataChanged) {
        lastDataReceived = dataReceived;
        dataChanged = false;
        updateDynamicScreen(); // обновление динамических элементов
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
