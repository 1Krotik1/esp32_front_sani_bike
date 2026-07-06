#include <TFT_eSPI.h>
#include "Saninokia.h"
#include "protocol_can.h"
TFT_eSPI tft = TFT_eSPI();



// --- Отрисовка индикатора батареи 1 ---
void drawBattery1(const uint32_t color) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRect(5, 10, 30, 20, color);
    tft.setTextSize(0);
    tft.setCursor(9, 16);
    tft.println("BAT1");

    

    
}

void drawBattery1Prime() {
    tft.drawLine(10, 35, 10, 44, TFT_WHITE);
    tft.drawLine(10, 35, 13, 38, TFT_WHITE);
    tft.drawLine(10, 35, 7, 38, TFT_WHITE);
}

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

void drawBattery2ToMainLineVoltage(const uint32_t color ) {
    tft.drawLine(60, 30, 60, 44, color);
}

void drawBattery2Prime() {
    tft.drawLine(80, 35, 80, 44, TFT_WHITE);
    tft.drawLine(80, 35, 83, 38, TFT_WHITE);
    tft.drawLine(80, 35, 77, 38, TFT_WHITE);
}
// --- Отрисовка напряжения ---
void drawMainLineVoltage(const char* value) {
    tft.drawRect(23, 45, 43, 20, TFT_GREEN);
    tft.setTextSize(0);
    tft.setCursor(27, 51);
    tft.println(value);
}

// --- Отрисовка тока ---
void drawMainLineCurrent(const char* value) {
    tft.drawRect(23, 65, 43, 20, TFT_GREEN);
    tft.setTextSize(0);
    tft.setCursor(27, 71);
    tft.println(value);
}

// --- Отрисовка линии от тока к APU ---
void drawMainLineCurrentToApuLine(const uint32_t color) {
    tft.drawLine(45, 85, 45, 99, color);
}

// --- Отрисовка индикатора APU ---
void drawApu(const uint32_t color) {
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
    drawMainLineCurrentToApuLine(TFT_RED);
    drawApu(TFT_RED);
    drawSpeedometer("??");
    drawCriticalError("critical error");
    drawWarning("warning");
}

void setup() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // tft.setSwapBytes(true); // ВАЖНО: меняет порядок байт (Big/Little Endian)
    // tft.pushImage(0, 0, Saninokia_w, Saninokia_h, Saninokia);
    // delay(10000); // Пауза для просмотра изображения
    // tft.fillScreen(TFT_BLACK);
    // tft.setSwapBytes(false); // Вернуть к нормальному порядку байт для текста

    drawScreen();
}

void loop() {
    // Пустой цикл, изображение уже выведено в setup()
    delay(1000);
}
