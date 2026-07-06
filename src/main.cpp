#include <TFT_eSPI.h>
#include "Saninokia.h"
#include "protocol_can.h"
TFT_eSPI tft = TFT_eSPI();



void setup() {
    tft.init();
    tft.setRotation(1);
    
    tft.fillScreen(TFT_BLACK);

    // tft.setSwapBytes(true); // ВАЖНО: меняет порядок байт (Big/Little Endian)
    // tft.pushImage(0, 0, Saninokia_w, Saninokia_h, Saninokia);
    // delay(10000); // Пауза для просмотра изображения
    // tft.fillScreen(TFT_BLACK);
    // tft.setSwapBytes(false); // Вернуть к нормальному порядку байт для текста
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRect(5, 10, 30, 20, TFT_RED);
    tft.setTextSize(0);
    tft.setCursor(9, 16);
    tft.println("BAT1");

    tft.drawLine(10, 35, 10, 44, TFT_WHITE);
    tft.drawLine(10, 35, 13, 38, TFT_WHITE);
    tft.drawLine(10, 35, 7, 38, TFT_WHITE);

    tft.drawLine(30, 30, 30, 44, TFT_RED);

    tft.drawRect(55, 10, 30, 20, TFT_GREEN);
    tft.setTextSize(0);
    tft.setCursor(59, 16);
    tft.println("BAT2");

    tft.drawLine(80, 35, 80, 44, TFT_WHITE);
    tft.drawLine(80, 35, 83, 38, TFT_WHITE);
    tft.drawLine(80, 35, 77, 38, TFT_WHITE);

    tft.drawLine(60, 30, 60, 44, TFT_GREEN);
    

    tft.drawRect(23, 45, 43, 20, TFT_RED);
    tft.setTextSize(0);
    tft.setCursor(27, 51);
    tft.println("12.50V");

    tft.drawRect(23, 65, 43, 20, TFT_RED);
    tft.setTextSize(0);
    tft.setCursor(27, 71);
    tft.println("1.25A");

    tft.drawLine(45, 85, 45, 99, TFT_RED);

    tft.drawRect(30, 100, 30, 20, TFT_RED);
    tft.setTextSize(0);
    tft.setCursor(36, 106);
    tft.println("APU");

    tft.drawRect(85, 88, 75, 40, TFT_RED);
    tft.setCursor(91, 98);
    tft.setTextSize(3);
    tft.print("??");
    tft.setTextSize(0);
    tft.println("km/h");

    tft.drawRect(95, 0, 65, 40, TFT_RED);
    tft.setViewport(99, 0, 55, 40);
    tft.setTextWrap(true, false);
    tft.setCursor(99, 0);
    tft.setTextSize(0);
    tft.println("critical error");
    tft.resetViewport();

    tft.drawRect(95, 41, 65, 40, TFT_YELLOW);
    tft.setViewport(99, 46, 55, 40);
    tft.setTextWrap(true, true);
    tft.setCursor(99, 52);
    tft.setTextSize(0);
    tft.println("warning");
    tft.resetViewport();
    

    
}
void loop() {
    // Пустой цикл, изображение уже выведено в setup()
    delay(1000);
}
