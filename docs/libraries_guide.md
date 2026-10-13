# Справочник библиотек проекта ESP32 Front Sani Bike

Проект использует ESP32 (board `esp32dev`) с Arduino-фреймворком. В `platformio.ini` подключены две внешние библиотеки и используются стандартные Arduino-библиотеки.

---

## 1. `TFT_eSPI` — графическая библиотека для TFT/LCD-дисплеев

**Пакет:** `bodmer/TFT_eSPI`  
**Репозиторий:** <https://github.com/bodmer/TFT_eSPI>  
**Лицензия:** GPL-3.0  
**Код-сниппетов в доке:** 90 | Репутация: High

### Назначение

Оптимизированная библиотека для отрисовки графики на TFT-дисплеях (ST7735, ILI9341, ILI9163, ST7789 и др.) под ESP32, ESP8266, RP2040, STM32. Поддерживает SPI, 8-битный параллельный интерфейс, DMA на ESP32-S3, сглаженные шрифты, BMP-изображения.

### Подключение в PlatformIO

```ini
lib_deps = bodmer/TFT_eSPI
```

### Конфигурация через `build_flags`

Вместо редактирования `User_Setup.h` внутри библиотеки, проект использует макросы-флаги:

```ini
build_flags =
    -D USER_SETUP_LOADED=1
    -D ST7735_DRIVER=1
    -D ST7735_BLACKTAB=1
    -D TFT_WIDTH=128
    -D TFT_HEIGHT=160
    -D TFT_MOSI=23
    -D TFT_SCLK=18
    -D TFT_MISO=19
    -D TFT_CS=5
    -D TFT_DC=2
    -D TFT_RST=4
    -D LOAD_GLCD=1
    -D LOAD_FONT2=1
    -D LOAD_FONT4=1
    -D LOAD_FONT6=1
    -D LOAD_FONT7=1
    -D LOAD_FONT8=1
    -D LOAD_GFXFF=1
    -D SMOOTH_FONT=1
    -D SPI_FREQUENCY=27000000
```

### Основные API-функции

```cpp
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();  // Инициализация экземпляра

void setup() {
    tft.init();              // Инициализация дисплея
    tft.setRotation(0);      // Поворот 0–3 (0 = портрет, 1 = ландшафт)
    tft.fillScreen(TFT_BLACK); // Очистка экрана
}
```

#### Отрисовка примитивов

```cpp
tft.drawRect(x, y, w, h, color);    // Прямоугольник (контур)
tft.fillRect(x, y, w, h, color);    // Заполненный прямоугольник
tft.drawCircle(x, y, r, color);     // Окружность
tft.fillCircle(x, y, r, color);     // Заполненная окружность
tft.drawLine(x0, y0, x1, y1, color); // Линия
tft.drawSmoothArc(x, y, rx, ry, startAngle, endAngle, color, bgColor, roundedEnds); // Дуга
```

#### Текст и шрифты

```cpp
tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Цвет текста и фона
tft.setTextSize(1);                       // Размер (0 = системный)
tft.setCursor(x, y);                      // Позиция курсора
tft.println("Hello");                     // Вывод строки
tft.setTextWrap(true);                    // Перенос строк
```

#### Цвета

```cpp
// Предопределённые цвета
TFT_BLACK, TFT_NAVY, TFT_DARKGREEN, TFT_DARKCYAN, TFT_MAROON,
TFT_PURPLE, TFT_OLIVE, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLUE,
TFT_GREEN, TFT_CYAN, TFT_RED, TFT_MAGENTA, TFT_YELLOW, TFT_WHITE,
TFT_ORANGE, TFT_GREENYELLOW, TFT_PINK

// RGB565
tft.color565(r, g, b);
```

#### Изображения из PROGMEM

```cpp
tft.drawRGBBitmap(x, y, bitmap, w, h);  // Рисование битмапа из flash-памяти
```

### Особенности в этом проекте

Дисплей 128×160 (ST7735), SPI-шина, 27 МГц, сглаженные шрифты. Изображение `Saninokia` хранится в PROGMEM как массив `uint16_t` (RGB565).

---

## 2. `mcp_can` — библиотека для CAN-контроллера MCP2515

**Пакет:** `coryjfowler/mcp_can@^1.5.1`  
**Репозиторий:** <https://github.com/coryjfowler/MCP_CAN_lib>  
**Лицензия:** GPL-3.0  
**Код-сниппетов в доке:** 12 | Репутация: High

### Назначение

Библиотека для работы с CAN-контроллером MCP2515 (также совместима с MCP25625) через SPI-интерфейс. MCP2515 — отдельный чип CAN 2.0B, подключаемый к микроконтроллеру по SPI. Используется для приёма и отправки CAN-сообщений.

### Архитектура MCP2515

MCP2515 — это самостоятельный CAN-контроллер с собственным процессором, который работает независимо от основного микроконтроллера. Ключевые особенности:

- **Интерфейс:** SPI (макс. 10 МГц)
- **CAN-версия:** 2.0B (стандартные 11-битные и расширенные 29-битные ID)
- **Буферы:** 3 трансмиттерных + 2 приёмных буфера
- **Прерывания:** вывод INT для уведомления о событиях
- **Кварц:** внешний резонатор 8/16/20 МГц
- **Питание:** 2.5–5.5 В (5 В tolerant на логических входах)

### Подключение в PlatformIO

```ini
lib_deps = coryjfowler/mcp_can@^1.5.1
```

### Пины подключения (в проекте)

```cpp
#define MCP2515_CS_PIN    17   // SPI Chip Select
#define MCP2515_INT_PIN   16   // Прерывание (принято сообщение)
```

Также требуются стандартные SPI-пины ESP32 (MOSI=23, MISO=19, SCK=18).

### Инициализация

```cpp
#include <SPI.h>
#include <mcp_can.h>

MCP_CAN CAN(MCP2515_CS_PIN);

// begin(mode, bitRate, clock)
// mode: MCP_ANY — принимать все стандартные и расширенные ID
// bitRate: CAN_500KBPS, CAN_250KBPS, CAN_125KBPS и т.д.
// clock: MCP_8MHZ, MCP_16MHZ, MCP_20MHZ — частота кварца на модуле
if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN initialized");
} else {
    Serial.println("CAN init failed");
}
```

> **Важно:** После `begin()` контроллер автоматически переходит в режим `MCP_LOOPBACK`. Для работы с реальной шиной необходимо переключить в `MCP_NORMAL`.

### Режимы работы

```cpp
// MCP_NORMAL — нормальная работа (отправка и приём)
CAN.setMode(MCP_NORMAL);

// MCP_LOOPBACK — петлевой режим (отправленные сообщения возвращаются на приём)
CAN.setMode(MCP_LOOPBACK);

// MCP_LISTENONLY — только приём, игнорирование ошибок
CAN.setMode(MCP_LISTENONLY);

// MCP_SLEEP — энергосбережение
CAN.setMode(MCP_SLEEP);
```

### Фильтры и маски

MCP2515 имеет 2 маски (Mask 0, Mask 1) и 6 фильтров (Filt 0–5). Фильтры работают в паре с масками:

```cpp
// init_Mask(num, ext, mask)
// num: 0 или 1 — номер маски
// ext: MCP_STD (0) или MCP_EXT (1) — тип ID
// mask: маска для фильтрации
CAN.init_Mask(0, MCP_STD, 0x000);  // Принимать все стандартные ID
CAN.init_Mask(1, MCP_STD, 0x000);

// init_Filt(num, ext, filter)
// num: 0–5 — номер фильтра
// ext: MCP_STD или MCP_EXT
// filter: конкретный ID для фильтрации
CAN.init_Filt(0, MCP_STD, 0x001);  // Фильтр для ID 0x001
CAN.init_Filt(1, MCP_STD, 0x002);  // Фильтр для ID 0x002
CAN.init_Filt(2, MCP_STD, 0x003);
CAN.init_Filt(3, MCP_STD, 0x004);
CAN.init_Filt(4, MCP_STD, 0x000);  // Запасной
CAN.init_Filt(5, MCP_STD, 0x000);  // Запасной
```

**Принцип работы:** сообщение принимается, если `(received_ID & mask) == (filter & mask)`. Установка маски в `0x000` означает «принимать всё».

### Отправка сообщений

```cpp
// Вариант 1: стандартный ID
long unsigned int txId = 0x123;
byte txLen = 8;
byte txBuf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
CAN.sendMsgBuf(txId, txLen, txBuf);

// Вариант 2: с флагом extended
boolean ext = false;
CAN.sendMsgBuf(txId, ext, txLen, txBuf);

// Расширенный ID (29 бит)
CAN.sendMsgBuf(txId | 0x80000000, txLen, txBuf);

// Remote Request (запрос данных без payload)
CAN.sendMsgBuf(txId | 0x40000000, txLen, txBuf);
```

### Приём сообщений

```cpp
// Проверка наличия сообщения
if (CAN_MSGAVAIL == CAN.checkReceive()) {
    long unsigned int rxId;
    byte rxLen = 0;
    byte rxBuf[8];

    // Чтение: ID, длина, данные
    CAN.readMsgBuf(&rxId, &rxLen, rxBuf);

    // Проверка типа ID
    if (rxId & 0x80000000) {
        // Расширенный ID
    }
    if (rxId & 0x40000000) {
        // Remote Request
    }
}
```

### Проверка ошибок

```cpp
// Проверка ошибок приёма/передачи
unsigned char err = CAN.checkError();
if (err == 0) {
    // Ошибок нет
}

// Коды ошибок (битовая маска):
// 0x01 — EWARN: Error Warning (счётчик ошибок > 96)
// 0x02 — RXWAR: RX Error Warning (счётчик > 96)
// 0x04 — TXWAR: TX Error Warning (счётчик > 96)
// 0x08 — RXEP: RX Error Passive
// 0x10 — TXEP: TX Error Passive
// 0x20 — TXBO: TX Bus Off (контроллер отключён от шины)
```

### One-Shot режим

В режиме One-Shot контроллер пытается отправить сообщение только один раз (без повторных попыток при коллизии):

```cpp
CAN.enOneShotTX();   // Включить
CAN.disOneShotTX();  // Выключить (по умолчанию)
```

### Прерывания

MCP2515 генерирует сигнал на выводе INT при:
- Приёме сообщения
- Возникновении ошибки
- Изменении состояния ошибок

```cpp
// Настройка прерывания
attachInterrupt(digitalPinToInterrupt(MCP2515_INT_PIN), canInterrupt, FALLING);

void canInterrupt() {
    // Обработка прерывания
    // Можно проверить статус через CAN.checkError()
}

// Отключение прерывания
detachInterrupt(digitalPinToInterrupt(MCP2515_INT_PIN));
```

### Sleep/Wakeup

```cpp
// Включение прерывания пробуждения от CAN-шины
CAN.setSleepWakeup(1);

// Выключение (по умолчанию)
CAN.setSleepWakeup(0);
```

### Структуры данных CAN (в проекте)

Проект определяет свои структуры CAN-сообщений через `protocol_can.h`:

```cpp
enum CanId : uint32_t {
    CAN_ID_STATUS_DATA    = 0x001,  // Статус всех компонентов
    CAN_ID_MAIN_LINE_DATA = 0x002,  // Напряжение и ток главной шины
    CAN_ID_BATTERY1_DATA  = 0x003,  // Напряжение батареи 1
    CAN_ID_BATTERY2_DATA  = 0x004,  // Напряжение батареи 2
};

#pragma pack(push, 1)
struct StatusData {
    uint8_t status_main_line; // Код статуса главной шины
    uint8_t status_battery1;  // Код статуса батареи 1
    uint8_t status_battery2;  // Код статуса батареи 2
    uint8_t status_apu;       // Код статуса ВСУ
};
struct MainLineData {
    uint16_t voltage; // Напряжение в вольтах (×100)
    uint16_t current; // Ток в амперах (×100)
};
struct Battery1Data {
    uint16_t voltage; // Напряжение в вольтах (×100)
};
struct Battery2Data {
    uint16_t voltage; // Напряжение в вольтах (×100)
};
#pragma pack(pop)
```

### Полная инициализация (как в проекте)

```cpp
#include <SPI.h>
#include <mcp_can.h>

#define MCP2515_CS_PIN    17
#define MCP2515_INT_PIN   16

MCP_CAN CAN(MCP2515_CS_PIN);
bool canInitialized = false;

void setup() {
    pinMode(MCP2515_CS_PIN, OUTPUT);
    pinMode(MCP2515_INT_PIN, INPUT);

    // Инициализация CAN (500 кбит/с, кварц 8 МГц)
    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("CAN initialized");
        canInitialized = true;
    } else {
        Serial.println("CAN init failed");
        canInitialized = false;
    }

    if (canInitialized) {
        // Фильтры: принимать все стандартные 11-битные ID
        CAN.init_Mask(0, 0, 0x00000000);
        CAN.init_Mask(1, 0, 0x00000000);
        for (int i = 0; i < 6; i++) {
            CAN.init_Filt(i, 0, 0x00000000);
        }

        // Переключение в нормальный режим
        CAN.setMode(MCP_NORMAL);
    }
}

void loop() {
    if (canInitialized && CAN_MSGAVAIL == CAN.checkReceive()) {
        long unsigned int rxId;
        byte rxLen = 0;
        byte rxBuf[8];

        CAN.readMsgBuf(&rxId, &rxLen, rxBuf);
        // Обработка сообщения...
    }
}
```

### Отправка и приём пакетов с использованием структур

CAN-шина ограничивает payload до 8 байт. Для передачи структурированных данных используется приведение указателей (`reinterpret_cast`). Структуры должны быть упакованы (`#pragma pack(push, 1)`) для предотвращения выравнивания.

#### Отправка структуры

```cpp
#include "protocol_can.h"

// --- Отправка статуса ---
void sendStatusData(uint8_t mainLine, uint8_t bat1, uint8_t bat2, uint8_t apu) {
    StatusData data;
    data.status_main_line = mainLine;
    data.status_battery1  = bat1;
    data.status_battery2  = bat2;
    data.status_apu       = apu;

    // Приводим структуру к массиву байт
    const byte* pData = reinterpret_cast<const byte*>(&data);
    CAN.sendMsgBuf(CAN_ID_STATUS_DATA, sizeof(StatusData), pData);
    Serial.printf("TX STATUS: main=%u bat1=%u bat2=%u apu=%u\n",
                  mainLine, bat1, bat2, apu);
}

// --- Отправка данных главной шины ---
void sendMainLineData(uint16_t voltage, uint16_t current) {
    MainLineData data;
    data.voltage = voltage;
    data.current = current;

    const byte* pData = reinterpret_cast<const byte*>(&data);
    CAN.sendMsgBuf(CAN_ID_MAIN_LINE_DATA, sizeof(MainLineData), pData);
    Serial.printf("TX MAIN_LINE: voltage=%u current=%u\n", voltage, current);
}

// --- Отправка данных батареи ---
void sendBatteryData(uint32_t canId, uint16_t voltage) {
    // Используем универсальную структуру для батареи
    Battery1Data data;
    data.voltage = voltage;

    const byte* pData = reinterpret_cast<const byte*>(&data);
    CAN.sendMsgBuf(canId, sizeof(Battery1Data), pData);
    Serial.printf("TX BATTERY: voltage=%u\n", voltage);
}
```

#### Приём и разбор структуры

```cpp
void handleCanMessage(uint32_t id, const byte* data, uint8_t len) {
    switch (id) {
        case CAN_ID_STATUS_DATA: {
            if (len < sizeof(StatusData)) {
                Serial.printf("STATUS_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(StatusData));
                break;
            }
            // Приводим массив байт к структуре
            const StatusData* status = reinterpret_cast<const StatusData*>(data);
            Serial.printf("STATUS: main=%u bat1=%u bat2=%u apu=%u\n",
                          status->status_main_line,
                          status->status_battery1,
                          status->status_battery2,
                          status->status_apu);
            break;
        }

        case CAN_ID_MAIN_LINE_DATA: {
            if (len < sizeof(MainLineData)) {
                Serial.printf("MAIN_LINE_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(MainLineData));
                break;
            }
            const MainLineData* mainLine = reinterpret_cast<const MainLineData*>(data);
            Serial.printf("MAIN_LINE: voltage=%u current=%u\n",
                          mainLine->voltage, mainLine->current);
            break;
        }

        case CAN_ID_BATTERY1_DATA: {
            if (len < sizeof(Battery1Data)) {
                Serial.printf("BATTERY1_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(Battery1Data));
                break;
            }
            const Battery1Data* bat1 = reinterpret_cast<const Battery1Data*>(data);
            Serial.printf("BATTERY1: voltage=%u\n", bat1->voltage);
            break;
        }

        case CAN_ID_BATTERY2_DATA: {
            if (len < sizeof(Battery2Data)) {
                Serial.printf("BATTERY2_DATA: invalid len=%u expected=%u\n",
                              len, (unsigned)sizeof(Battery2Data));
                break;
            }
            const Battery2Data* bat2 = reinterpret_cast<const Battery2Data*>(data);
            Serial.printf("BATTERY2: voltage=%u\n", bat2->voltage);
            break;
        }

        default:
            Serial.printf("UNKNOWN CAN ID=0x%03lX len=%u\n", id, len);
            break;
    }
}
```

#### Полный цикл: отправка и приём

```cpp
#include <SPI.h>
#include <mcp_can.h>
#include "protocol_can.h"

#define MCP2515_CS_PIN    17
#define MCP2515_INT_PIN   16

MCP_CAN CAN(MCP2515_CS_PIN);
bool canInitialized = false;
uint32_t canRxCount = 0;

void setup() {
    Serial.begin(115200);
    pinMode(MCP2515_CS_PIN, OUTPUT);
    pinMode(MCP2515_INT_PIN, INPUT);

    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("CAN initialized");
        canInitialized = true;

        // Фильтры: принимать все стандартные ID
        CAN.init_Mask(0, 0, 0x00000000);
        CAN.init_Mask(1, 0, 0x00000000);
        for (int i = 0; i < 6; i++) {
            CAN.init_Filt(i, 0, 0x00000000);
        }
        CAN.setMode(MCP_NORMAL);
    }
}

void loop() {
    // --- Приём ---
    if (canInitialized && CAN_MSGAVAIL == CAN.checkReceive()) {
        long unsigned int rxId;
        byte rxLen = 0;
        byte rxBuf[8];

        CAN.readMsgBuf(&rxId, &rxLen, rxBuf);
        canRxCount++;

        handleCanMessage(rxId, rxBuf, rxLen);
    }

    // --- Отправка (каждые 100 мс) ---
    static uint32_t lastTxMs = 0;
    if (millis() - lastTxMs >= 100) {
        lastTxMs = millis();

        // Имитация данных
        static uint16_t volt = 1250;  // 12.50 В
        static uint16_t curr =  125;  // 1.25 А

        sendMainLineData(volt, curr);
        sendBatteryData(CAN_ID_BATTERY1_DATA, 1260);
        sendBatteryData(CAN_ID_BATTERY2_DATA, 1245);

        StatusData status = {1, 1, 1, 0};
        sendStatusData(status.status_main_line,
                       status.status_battery1,
                       status.status_battery2,
                       status.status_apu);
    }
}
```

#### Размер структур и ограничение CAN

CAN 2.0 ограничивает payload до **8 байт**. Проверяйте размеры структур:

```cpp
static_assert(sizeof(StatusData)    <= 8, "StatusData too large");
static_assert(sizeof(MainLineData)  <= 8, "MainLineData too large");
static_assert(sizeof(Battery1Data)  <= 8, "Battery1Data too large");
static_assert(sizeof(Battery2Data)  <= 8, "Battery2Data too large");
```

| Структура | Размер | Поля |
|---|---|---|
| `StatusData` | 4 байта | 4 × uint8_t |
| `MainLineData` | 4 байта | 2 × uint16_t |
| `Battery1Data` | 2 байта | 1 × uint16_t |
| `Battery2Data` | 2 байта | 1 × uint16_t |

#### Копирование данных из буфера (без приведения указателя)

Если нужно избежать strict aliasing, используйте `memcpy`:

```cpp
case CAN_ID_MAIN_LINE_DATA: {
    MainLineData data;
    memcpy(&data, rxBuf, sizeof(data));
    Serial.printf("voltage=%u current=%u\n", data.voltage, data.current);
    break;
}
```

> **Примечание:** `memcpy` безопаснее с точки зрения стандарта C++, но `reinterpret_cast` быстрее и чаще используется в embedded-разработке. Оба варианта дают одинаковый машинный код на ESP32.

### Константы библиотеки

```cpp
// Частоты кварцевого резонатора
MCP_8MHZ, MCP_16MHZ, MCP_20MHZ

// Битрейты CAN
CAN_4K096BPS, CAN_5KBPS, CAN_10KBPS, CAN_20KBPS,
CAN_31K25BPS, CAN_40KBPS, CAN_50KBPS, CAN_80KBPS,
CAN_100KBPS, CAN_125KBPS, CAN_200KBPS, CAN_250KBPS,
CAN_500KBPS, CAN_1000KBPS

// Режимы ID
MCP_STD   // Стандартный 11-битный ID
MCP_EXT   // Расширенный 29-битный ID
MCP_STDEXT // Все типы ID

// Режимы работы
MCP_NORMAL, MCP_SLEEP, MCP_LOOPBACK, MCP_LISTENONLY

// Коды возврата begin()
CAN_OK          // 0 — успех
CAN_FAILINIT    // 1 — ошибка инициализации
CAN_FAILTX      // 2 — ошибка отправки
CAN_BUF_FULL    // 3 — буфер полон
CAN_FAILTX      // 4 — повторная попытка
CAN_FAILRX      // 5 — ошибка приёма
CAN_FAILBUS     // 6 — ошибка шины

// Коды checkReceive()
CAN_MSGAVAIL    // 1 — есть сообщение
CAN_NOMSG       // 0 — нет сообщения

// Коды checkError()
CAN_CTRLERROR   // 0x7F — контрольная ошибка
CAN_GETTXBFTIMEOUT // таймаут отправки
CAN_SENDMSGTIMEOUT // таймаут отправки
CAN_FAIL        // общая ошибка
```

### Типичные проблемы и решения

| Проблема | Причина | Решение |
|---|---|---|
| `CAN init failed` | Неправильная частота кварца | Проверьте маркировку на модуле (8/16/20 МГц) |
| Сообщения не приходят | Режим LOOPBACK | Вызовите `CAN.setMode(MCP_NORMAL)` |
| Нет прерываний | Не подключён INT пин | Проверьте подключение и `attachInterrupt` |
| Ошибка `TXBO` | Обрыв шины или отсутствие терминатора | Проверьте CANH/CANL, добавьте 120 Ом |
| Потеря сообщений | Фильтры слишком строгие | Установите маску `0x000` для приёма всего |

---

## 3. `SPI` — стандартная библиотека Arduino

**Пакет:** Входит в `espressif32`  
**Репозиторий:** <https://github.com/espressif/arduino-esp32>  
**Лицензия:** LGPL-2.1

### Назначение

Реализация протокола SPI (Serial Peripheral Interface) для Arduino. Используется для связи с периферийными устройствами: дисплеями, модулями CAN, картами памяти и др.

### Подключение

```cpp
#include <SPI.h>
```

### Основные API-функции

```cpp
// Инициализация SPI-шины
SPI.begin();

// Настройка параметров передачи
SPI.beginTransaction(SPISettings(27000000, MSBFIRST, SPI_MODE0));

// Передача одного байта
byte result = SPI.transfer(0xFF);

// Завершение транзакции
SPI.endTransaction();

// Отключение SPI
SPI.end();
```

### SPI-режимы

```cpp
SPI_MODE0  // CPOL=0, CPHA=0 — данные считываются по переднему фронту
SPI_MODE1  // CPOL=0, CPHA=1 — данные считываются по заднему фронту
SPI_MODE2  // CPOL=1, CPHA=0
SPI_MODE3  // CPOL=1, CPHA=1
```

### Особенности на ESP32

ESP32 поддерживает до 3 SPI-шин:
- `SPI` — HSPI (по умолчанию)
- `SPIClass spi2(VSPI)` — VSPI
- `SPIClass spi1(HSPI)` — HSPI

```cpp
SPIClass spi2(VSPI);  // Вторая SPI-шина
spi2.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, CS
```

---

## 4. `Arduino.h` — ядро Arduino

**Пакет:** `espressif32`  
**Репозиторий:** <https://github.com/espressif/arduino-esp32>  
**Лицензия:** LGPL-2.1

### Назначение

Основное заголовочное файл Arduino-фреймворка для ESP32. Содержит объявления стандартных функций, типов и констант.

### Основные функции

```cpp
// Пины
pinMode(pin, MODE);           // INPUT, OUTPUT, INPUT_PULLUP
digitalWrite(pin, VALUE);     // HIGH, LOW
int digitalRead(pin);

// Аналоговые пины
analogRead(pin);              // 0-4095 (12-bit)
analogWrite(pin, value);      // ШИМ (0-255)
analogWriteFreq(freq);        // Частота ШИМ

// Задержки
delay(ms);                    // Задержка в миллисекундах
delayMicroseconds(us);        // Задержка в микросекундах
millis();                     // Миллисекунды с запуска
micros();                     // Микросекунды с запуска

// Прерывания
attachInterrupt(pin, handler, mode);  // FALLING, RISING, CHANGE
detachInterrupt(pin);

// Serial
Serial.begin(baud);
Serial.println(data);
Serial.printf(format, args);
```

### Типы данных

```cpp
uint8_t   // 8-bit unsigned (0-255)
uint16_t  // 16-bit unsigned (0-65535)
uint32_t  // 32-bit unsigned (0-4294967295)
int8_t    // 8-bit signed (-128 to 127)
int16_t   // 16-bit signed (-32768 to 32767)
int32_t   // 32-bit signed
float     // 32-bit float
double    // 32-bit float (на ESP32 = float)
```

---

## 5. `Saninokia.h` — локальный заголовок с изображением

**Пакет:** Локальный файл в `src/`  
**Формат:** BMP-битмап, RGB565, 160×128 пикселей

### Назначение

Содержит массив пикселей логотипа "Saninokia" для отображения на TFT-дисплее. Данные хранятся в flash-памяти (PROGMEM) для экономии RAM.

### Использование

```cpp
#include "Saninokia.h"

// Отрисовка логотипа
tft.drawRGBBitmap(0, 0, Saninokia, Saninokia_w, Saninokia_h);
```

### Параметры

```cpp
const uint16_t Saninokia_w = 160;  // Ширина в пикселях
const uint16_t Saninokia_h = 128;  // Высота в пикселях
const uint16_t Saninokia[] PROGMEM = { ... };  // Массив пикселей RGB565
```

---

## Сводная таблица зависимостей

| Библиотека | Пакет | Версия | Назначение |
|---|---|---|---|
| `TFT_eSPI` | `bodmer/TFT_eSPI` | последняя | Графика на TFT-дисплее |
| `mcp_can` | `coryjfowler/mcp_can` | ^1.5.1 | CAN-шина через MCP2515 |
| `SPI` | `espressif32` | встроенная | SPI-интерфейс |
| `Arduino.h` | `espressif32` | встроенная | Ядро Arduino |
| `Saninokia.h` | локальный | — | Логотип в PROGMEM |

---

## Схемы подключения

### TFT-дисплей (ST7735, 128×160)

| Пин дисплея | Пин ESP32 | Функция |
|---|---|---|
| SCLK | 18 | SPI clock |
| MOSI | 23 | SPI data out |
| MISO | 19 | SPI data in (не используется) |
| CS | 5 | Chip select |
| DC | 2 | Data/Command select |
| RST | 4 | Reset |
| VCC | 3.3V | Питание |
| GND | GND | Земля |

### MCP2515 CAN-модуль

| Пин модуля | Пин ESP32 | Функция |
|---|---|---|
| SCLK | 18 | SPI clock |
| MOSI | 23 | SPI data out |
| MISO | 19 | SPI data in |
| CS | 17 | Chip select |
| INT | 16 | Прерывание |
| VCC | 5V | Питание (5V tolerant) |
| GND | GND | Земля |
| CANH | — | CAN high |
| CANL | — | CAN low |

---

## Полезные ссылки

- [TFT_eSPI GitHub](https://github.com/bodmer/TFT_eSPI)
- [MCP_CAN_lib GitHub](https://github.com/coryjfowler/MCP_CAN_lib)
- [ESP32 Arduino API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api.html)
- [PlatformIO Documentation](https://docs.platformio.org/)
