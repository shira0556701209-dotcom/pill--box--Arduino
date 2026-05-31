#include "tftSensor.h"

// =========================================================
//  תקשורת SPI בסיסית (bit-bang תוכנתי - ללא ספריית SPI)
// =========================================================

// פינים SPI (ניתן לשינוי)
#define SPI_MOSI_PIN 12
#define SPI_SCK_PIN  14

static inline void _spiBeginTransaction() {
    // ---- אם רוצים SPI חומרתי - להחליף בהתאם ----
    // כאן: SPI תוכנתי פשוט (bit-bang)
}

static void _spiSendByte(uint8_t data) {
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (data & 0x80) {
            digitalWrite(SPI_MOSI_PIN, HIGH);
        } else {
            digitalWrite(SPI_MOSI_PIN, LOW);
        }
        digitalWrite(SPI_SCK_PIN, HIGH);
        digitalWrite(SPI_SCK_PIN, LOW);
        data <<= 1;
    }
}

static void _spiSend16(uint16_t data) {
    _spiSendByte(data >> 8);
    _spiSendByte(data & 0xFF);
}

// =========================================================
//  שליחת פקודות ל-ILI9341
// =========================================================

static void _tftWriteCommand(uint8_t cmd) {
    digitalWrite(TFT_DC_PIN, LOW);   // Command mode
    digitalWrite(TFT_CS_PIN, LOW);
    _spiSendByte(cmd);
    digitalWrite(TFT_CS_PIN, HIGH);
}

static void _tftWriteData(uint8_t data) {
    digitalWrite(TFT_DC_PIN, HIGH);  // Data mode
    digitalWrite(TFT_CS_PIN, LOW);
    _spiSendByte(data);
    digitalWrite(TFT_CS_PIN, HIGH);
}

static void _tftWriteData16(uint16_t data) {
    digitalWrite(TFT_DC_PIN, HIGH);
    digitalWrite(TFT_CS_PIN, LOW);
    _spiSend16(data);
    digitalWrite(TFT_CS_PIN, HIGH);
}

// =========================================================
//  הגדרת חלון ציור
// =========================================================

static void _tftSetWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    // Column Address Set
    _tftWriteCommand(0x2A);
    _tftWriteData(x0 >> 8); _tftWriteData(x0 & 0xFF);
    _tftWriteData(x1 >> 8); _tftWriteData(x1 & 0xFF);
    // Page Address Set
    _tftWriteCommand(0x2B);
    _tftWriteData(y0 >> 8); _tftWriteData(y0 & 0xFF);
    _tftWriteData(y1 >> 8); _tftWriteData(y1 & 0xFF);
    // Memory Write
    _tftWriteCommand(0x2C);
}

// =========================================================
//  מצב פנימי
// =========================================================

static uint8_t  _rotation   = 0;
static int16_t  _cursorX    = 0;
static int16_t  _cursorY    = 0;
static uint8_t  _textSize   = 1;
static uint16_t _textColor  = TFT_WHITE;
static uint16_t _textBgColor = TFT_BLACK;

// =========================================================
//  אתחול
// =========================================================

void tft_init(uint8_t rotation) {
    _rotation = rotation;

    // הגדרת פינים
    pinMode(TFT_CS_PIN,  OUTPUT);
    pinMode(TFT_DC_PIN,  OUTPUT);
    pinMode(TFT_RST_PIN, OUTPUT);
    pinMode(SPI_MOSI_PIN, OUTPUT);
    pinMode(SPI_SCK_PIN,  OUTPUT);

    if (TFT_BL_PIN >= 0) {
        pinMode(TFT_BL_PIN, OUTPUT);
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    // Reset חומרתי
    digitalWrite(TFT_RST_PIN, HIGH); delay(10);
    digitalWrite(TFT_RST_PIN, LOW);  delay(20);
    digitalWrite(TFT_RST_PIN, HIGH); delay(150);

    // רצף אתחול ILI9341
    _tftWriteCommand(0x01); delay(150);  // Software Reset
    _tftWriteCommand(0x11); delay(500);  // Sleep Out

    _tftWriteCommand(0xCF);              // Power Control B
    _tftWriteData(0x00); _tftWriteData(0xC1); _tftWriteData(0x30);

    _tftWriteCommand(0xED);              // Power on sequence control
    _tftWriteData(0x64); _tftWriteData(0x03); _tftWriteData(0x12); _tftWriteData(0x81);

    _tftWriteCommand(0xE8);              // Driver timing A
    _tftWriteData(0x85); _tftWriteData(0x00); _tftWriteData(0x78);

    _tftWriteCommand(0xCB);              // Power Control A
    _tftWriteData(0x39); _tftWriteData(0x2C); _tftWriteData(0x00);
    _tftWriteData(0x34); _tftWriteData(0x02);

    _tftWriteCommand(0xF7); _tftWriteData(0x20); // Pump ratio control

    _tftWriteCommand(0xEA);              // Driver timing B
    _tftWriteData(0x00); _tftWriteData(0x00);

    _tftWriteCommand(0xC0); _tftWriteData(0x23); // Power Control 1
    _tftWriteCommand(0xC1); _tftWriteData(0x10); // Power Control 2

    _tftWriteCommand(0xC5);              // VCOM Control 1
    _tftWriteData(0x3E); _tftWriteData(0x28);
    _tftWriteCommand(0xC7); _tftWriteData(0x86); // VCOM Control 2

    // Memory Access Control (סיבוב)
    _tftWriteCommand(0x36);
    switch (rotation & 3) {
        case 0: _tftWriteData(0x48); break; // פורטרט
        case 1: _tftWriteData(0x28); break; // לנדסקייפ
        case 2: _tftWriteData(0x88); break; // פורטרט הפוך
        case 3: _tftWriteData(0xE8); break; // לנדסקייפ הפוך
    }

    _tftWriteCommand(0x3A); _tftWriteData(0x55); // Pixel Format 16bit

    _tftWriteCommand(0xB1);              // Frame Rate
    _tftWriteData(0x00); _tftWriteData(0x18);

    _tftWriteCommand(0xB6);              // Display Function Control
    _tftWriteData(0x08); _tftWriteData(0x82); _tftWriteData(0x27);

    _tftWriteCommand(0xF2); _tftWriteData(0x00); // 3Gamma off

    _tftWriteCommand(0x26); _tftWriteData(0x01); // Gamma Set

    // Positive Gamma Correction
    _tftWriteCommand(0xE0);
    uint8_t posGamma[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00};
    for (uint8_t i = 0; i < 15; i++) _tftWriteData(posGamma[i]);

    // Negative Gamma Correction
    _tftWriteCommand(0xE1);
    uint8_t negGamma[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F};
    for (uint8_t i = 0; i < 15; i++) _tftWriteData(negGamma[i]);

    _tftWriteCommand(0x11); delay(120); // Sleep Out
    _tftWriteCommand(0x29);             // Display ON

    tft_fillScreen(TFT_BLACK);
}

void tft_setBacklight(bool on) {
    if (TFT_BL_PIN >= 0) {
        digitalWrite(TFT_BL_PIN, on ? HIGH : LOW);
    }
}

// =========================================================
//  ציור בסיסי
// =========================================================

void tft_fillScreen(uint16_t color) {
    tft_fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void tft_drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT) return;
    _tftSetWindow(x, y, x, y);
    _tftWriteData16(color);
}

void tft_fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH  - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    _tftSetWindow(x, y, x + w - 1, y + h - 1);
    uint32_t pixels = (uint32_t)w * h;
    digitalWrite(TFT_DC_PIN, HIGH);
    digitalWrite(TFT_CS_PIN, LOW);
    while (pixels--) {
        _spiSend16(color);
    }
    digitalWrite(TFT_CS_PIN, HIGH);
}

void tft_drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
    tft_drawLine(x,       y,       x+w-1, y,       color);
    tft_drawLine(x,       y+h-1,   x+w-1, y+h-1,   color);
    tft_drawLine(x,       y,       x,     y+h-1,   color);
    tft_drawLine(x+w-1,  y,       x+w-1, y+h-1,   color);
}

void tft_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Bresenham's line algorithm
    int16_t dx = abs(x1-x0), dy = -abs(y1-y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy, e2;
    while (true) {
        tft_drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void tft_drawCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color) {
    int16_t x = r, y = 0, err = 0;
    while (x >= y) {
        tft_drawPixel(x0+x, y0+y, color); tft_drawPixel(x0+y, y0+x, color);
        tft_drawPixel(x0-y, y0+x, color); tft_drawPixel(x0-x, y0+y, color);
        tft_drawPixel(x0-x, y0-y, color); tft_drawPixel(x0-y, y0-x, color);
        tft_drawPixel(x0+y, y0-x, color); tft_drawPixel(x0+x, y0-y, color);
        y++;
        if (err <= 0) { err += 2*y+1; }
        else { x--; err += 2*(y-x)+1; }
    }
}

void tft_fillCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color) {
    tft_fillRect(x0-r, y0-r, 2*r, 2*r, TFT_BLACK); // פשטני – לציור מלא
    for (int16_t y = -r; y <= r; y++) {
        int16_t xspan = (int16_t)sqrt((float)(r*r - y*y));
        tft_fillRect(x0 - xspan, y0 + y, 2*xspan, 1, color);
    }
}

void tft_fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color) {
    tft_fillRect(x+r, y,   w-2*r, h,   color);
    tft_fillRect(x,   y+r, w,     h-2*r, color);
    tft_fillCircle(x+r,     y+r,     r, color);
    tft_fillCircle(x+w-r-1, y+r,     r, color);
    tft_fillCircle(x+r,     y+h-r-1, r, color);
    tft_fillCircle(x+w-r-1, y+h-r-1, r, color);
}

// =========================================================
//  טקסט – גופן 5x7 מובנה (מקטע בסיסי ASCII 32-126)
// =========================================================

// גופן 5x7 מינימלי (כל תו = 5 bytes, bit0=שורה עליונה)
static const uint8_t FONT5x7[][5] PROGMEM = {
  {0x00,0x00,0x00,0x00,0x00}, // 32 space
  {0x00,0x00,0x5F,0x00,0x00}, // 33 !
  {0x00,0x07,0x00,0x07,0x00}, // 34 "
  {0x14,0x7F,0x14,0x7F,0x14}, // 35 #
  {0x24,0x2A,0x7F,0x2A,0x12}, // 36 $
  {0x23,0x13,0x08,0x64,0x62}, // 37 %
  {0x36,0x49,0x55,0x22,0x50}, // 38 &
  {0x00,0x05,0x03,0x00,0x00}, // 39 '
  {0x00,0x1C,0x22,0x41,0x00}, // 40 (
  {0x00,0x41,0x22,0x1C,0x00}, // 41 )
  {0x14,0x08,0x3E,0x08,0x14}, // 42 *
  {0x08,0x08,0x3E,0x08,0x08}, // 43 +
  {0x00,0x50,0x30,0x00,0x00}, // 44 ,
  {0x08,0x08,0x08,0x08,0x08}, // 45 -
  {0x00,0x60,0x60,0x00,0x00}, // 46 .
  {0x20,0x10,0x08,0x04,0x02}, // 47 /
  {0x3E,0x51,0x49,0x45,0x3E}, // 48 0
  {0x00,0x42,0x7F,0x40,0x00}, // 49 1
  {0x42,0x61,0x51,0x49,0x46}, // 50 2
  {0x21,0x41,0x45,0x4B,0x31}, // 51 3
  {0x18,0x14,0x12,0x7F,0x10}, // 52 4
  {0x27,0x45,0x45,0x45,0x39}, // 53 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 54 6
  {0x01,0x71,0x09,0x05,0x03}, // 55 7
  {0x36,0x49,0x49,0x49,0x36}, // 56 8
  {0x06,0x49,0x49,0x29,0x1E}, // 57 9
  {0x00,0x36,0x36,0x00,0x00}, // 58 :
  {0x00,0x56,0x36,0x00,0x00}, // 59 ;
  {0x08,0x14,0x22,0x41,0x00}, // 60 <
  {0x14,0x14,0x14,0x14,0x14}, // 61 =
  {0x00,0x41,0x22,0x14,0x08}, // 62 >
  {0x02,0x01,0x51,0x09,0x06}, // 63 ?
  {0x32,0x49,0x79,0x41,0x3E}, // 64 @
  {0x7E,0x11,0x11,0x11,0x7E}, // 65 A
  {0x7F,0x49,0x49,0x49,0x36}, // 66 B
  {0x3E,0x41,0x41,0x41,0x22}, // 67 C
  {0x7F,0x41,0x41,0x22,0x1C}, // 68 D
  {0x7F,0x49,0x49,0x49,0x41}, // 69 E
  {0x7F,0x09,0x09,0x09,0x01}, // 70 F
  {0x3E,0x41,0x49,0x49,0x7A}, // 71 G
  {0x7F,0x08,0x08,0x08,0x7F}, // 72 H
  {0x00,0x41,0x7F,0x41,0x00}, // 73 I
  {0x20,0x40,0x41,0x3F,0x01}, // 74 J
  {0x7F,0x08,0x14,0x22,0x41}, // 75 K
  {0x7F,0x40,0x40,0x40,0x40}, // 76 L
  {0x7F,0x02,0x0C,0x02,0x7F}, // 77 M
  {0x7F,0x04,0x08,0x10,0x7F}, // 78 N
  {0x3E,0x41,0x41,0x41,0x3E}, // 79 O
  {0x7F,0x09,0x09,0x09,0x06}, // 80 P
  {0x3E,0x41,0x51,0x21,0x5E}, // 81 Q
  {0x7F,0x09,0x19,0x29,0x46}, // 82 R
  {0x46,0x49,0x49,0x49,0x31}, // 83 S
  {0x01,0x01,0x7F,0x01,0x01}, // 84 T
  {0x3F,0x40,0x40,0x40,0x3F}, // 85 U
  {0x1F,0x20,0x40,0x20,0x1F}, // 86 V
  {0x3F,0x40,0x38,0x40,0x3F}, // 87 W
  {0x63,0x14,0x08,0x14,0x63}, // 88 X
  {0x07,0x08,0x70,0x08,0x07}, // 89 Y
  {0x61,0x51,0x49,0x45,0x43}, // 90 Z
  {0x00,0x7F,0x41,0x41,0x00}, // 91 [
  {0x02,0x04,0x08,0x10,0x20}, // 92 backslash
  {0x00,0x41,0x41,0x7F,0x00}, // 93 ]
  {0x04,0x02,0x01,0x02,0x04}, // 94 ^
  {0x40,0x40,0x40,0x40,0x40}, // 95 _
  {0x00,0x01,0x02,0x04,0x00}, // 96 `
  {0x20,0x54,0x54,0x54,0x78}, // 97 a
  {0x7F,0x48,0x44,0x44,0x38}, // 98 b
  {0x38,0x44,0x44,0x44,0x20}, // 99 c
  {0x38,0x44,0x44,0x48,0x7F}, // 100 d
  {0x38,0x54,0x54,0x54,0x18}, // 101 e
  {0x08,0x7E,0x09,0x01,0x02}, // 102 f
  {0x0C,0x52,0x52,0x52,0x3E}, // 103 g
  {0x7F,0x08,0x04,0x04,0x78}, // 104 h
  {0x00,0x44,0x7D,0x40,0x00}, // 105 i
  {0x20,0x40,0x44,0x3D,0x00}, // 106 j
  {0x7F,0x10,0x28,0x44,0x00}, // 107 k
  {0x00,0x41,0x7F,0x40,0x00}, // 108 l
  {0x7C,0x04,0x18,0x04,0x78}, // 109 m
  {0x7C,0x08,0x04,0x04,0x78}, // 110 n
  {0x38,0x44,0x44,0x44,0x38}, // 111 o
  {0x7C,0x14,0x14,0x14,0x08}, // 112 p
  {0x08,0x14,0x14,0x18,0x7C}, // 113 q
  {0x7C,0x08,0x04,0x04,0x08}, // 114 r
  {0x48,0x54,0x54,0x54,0x20}, // 115 s
  {0x04,0x3F,0x44,0x40,0x20}, // 116 t
  {0x3C,0x40,0x40,0x20,0x7C}, // 117 u
  {0x1C,0x20,0x40,0x20,0x1C}, // 118 v
  {0x3C,0x40,0x30,0x40,0x3C}, // 119 w
  {0x44,0x28,0x10,0x28,0x44}, // 120 x
  {0x0C,0x50,0x50,0x50,0x3C}, // 121 y
  {0x44,0x64,0x54,0x4C,0x44}, // 122 z
};

static void _drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 122) c = '?';
    uint8_t idx = c - 32;

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t colData = pgm_read_byte(&FONT5x7[idx][col]);
        for (uint8_t row = 0; row < 7; row++) {
            uint16_t pColor = (colData & (1 << row)) ? color : bg;
            if (size == 1) {
                tft_drawPixel(x + col, y + row, pColor);
            } else {
                tft_fillRect(x + col*size, y + row*size, size, size, pColor);
            }
        }
    }
}

void tft_setCursor(int16_t x, int16_t y) { _cursorX = x; _cursorY = y; }
void tft_setTextSize(uint8_t size)        { _textSize = (size < 1) ? 1 : size; }
void tft_setTextColor(uint16_t color, uint16_t bgColor) {
    _textColor = color; _textBgColor = bgColor;
}

void tft_print(const char* text) {
    while (*text) {
        if (*text == '\n') {
            _cursorY += 8 * _textSize;
            _cursorX = 0;
        } else {
            _drawChar(_cursorX, _cursorY, *text, _textColor, _textBgColor, _textSize);
            _cursorX += 6 * _textSize;
        }
        text++;
    }
}

void tft_println(const char* text) {
    tft_print(text);
    _cursorY += 8 * _textSize;
    _cursorX = 0;
}

void tft_printInt(int32_t value) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%ld", (long)value);
    tft_print(buf);
}

void tft_printCentered(const char* text, int16_t x, int16_t y, uint16_t w, uint16_t h,
                        uint8_t textSize, uint16_t color) {
    uint16_t len    = strlen(text);
    uint16_t tW     = len * 6 * textSize;
    uint16_t tH     = 8 * textSize;
    int16_t  startX = x + (w - tW) / 2;
    int16_t  startY = y + (h - tH) / 2;

    tft_setCursor(startX, startY);
    tft_setTextSize(textSize);
    tft_setTextColor(color, _textBgColor);
    tft_print(text);
}

// =========================================================
//  כפתורים
// =========================================================

void tft_buttonInit(TFTButton* btn, int16_t x, int16_t y, uint16_t w, uint16_t h,
                    uint16_t colorBg, uint16_t colorText, const char* label) {
    btn->x = x; btn->y = y; btn->w = w; btn->h = h;
    btn->colorBg   = colorBg;
    btn->colorText = colorText;
    btn->enabled   = true;
    strncpy(btn->label, label, sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label)-1] = '\0';
}

void tft_buttonDraw(const TFTButton* btn) {
    uint16_t bg = btn->enabled ? btn->colorBg : TFT_GRAY;
    tft_fillRoundRect(btn->x, btn->y, btn->w, btn->h, 6, bg);
    tft_drawRect(btn->x, btn->y, btn->w, btn->h, TFT_WHITE);
    tft_printCentered(btn->label, btn->x, btn->y, btn->w, btn->h, 2, btn->colorText);
}

bool tft_buttonHit(const TFTButton* btn, int16_t tx, int16_t ty) {
    if (!btn->enabled) return false;
    return (tx >= btn->x && tx < btn->x + (int16_t)btn->w &&
            ty >= btn->y && ty < btn->y + (int16_t)btn->h);
}

// =========================================================
//  מגע – XPT2046 bit-bang
// =========================================================

#define TOUCH_CMD_X  0xD0
#define TOUCH_CMD_Y  0x90

static uint16_t _touchReadRaw(uint8_t cmd) {
    uint16_t result = 0;
    digitalWrite(TOUCH_CS_PIN, LOW);
    // שלח פקודה
    for (int8_t i = 7; i >= 0; i--) {
        digitalWrite(SPI_MOSI_PIN, (cmd >> i) & 1);
        digitalWrite(SPI_SCK_PIN, HIGH);
        digitalWrite(SPI_SCK_PIN, LOW);
    }
    // קבל 12 ביט
    for (int8_t i = 11; i >= 0; i--) {
        digitalWrite(SPI_SCK_PIN, HIGH);
        if (digitalRead(12)) result |= (1 << i); // MISO
        digitalWrite(SPI_SCK_PIN, LOW);
    }
    digitalWrite(TOUCH_CS_PIN, HIGH);
    return result;
}

bool tft_getTouch(TouchPoint* tp) {
    if (TOUCH_IRQ_PIN >= 0 && digitalRead(TOUCH_IRQ_PIN) == HIGH) {
        tp->pressed = false;
        return false;
    }

    uint16_t rawX = _touchReadRaw(TOUCH_CMD_X);
    uint16_t rawY = _touchReadRaw(TOUCH_CMD_Y);

    // כיול: מיפוי ADC → פיקסלים
    int16_t px = map(rawX, TOUCH_X_MIN, TOUCH_X_MAX, 0, TFT_WIDTH  - 1);
    int16_t py = map(rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, TFT_HEIGHT - 1);

    px = constrain(px, 0, TFT_WIDTH  - 1);
    py = constrain(py, 0, TFT_HEIGHT - 1);

    tp->x = px;
    tp->y = py;
    tp->pressed = true;
    return true;
}

TouchPoint tft_waitForTouch() {
    TouchPoint tp;
    tp.pressed = false;
    while (!tft_getTouch(&tp)) { delay(10); }
    return tp;
}

// =========================================================
//  מסכים מוכנים לפרוייקט
// =========================================================

void tft_showHomeScreen(const char* timeStr) {
    tft_fillScreen(TFT_BLACK);
    // כותרת
    tft_fillRect(0, 0, TFT_WIDTH, 50, TFT_BLUE);
    tft_printCentered("Smart Pill Box", 0, 0, TFT_WIDTH, 50, 2, TFT_WHITE);
    // שעה
    tft_printCentered(timeStr, 0, 70, TFT_WIDTH, 40, 3, TFT_YELLOW);
    // הנחיה
    tft_printCentered("Tap for menu", 0, 260, TFT_WIDTH, 40, 1, TFT_GRAY);
}

void tft_showReminderScreen(const char* medicineName, const char* dose) {
    tft_fillScreen(TFT_BLACK);
    // רצועת אזהרה
    tft_fillRect(0, 0, TFT_WIDTH, 60, TFT_ORANGE);
    tft_printCentered("TIME TO TAKE", 0, 0, TFT_WIDTH, 30, 2, TFT_BLACK);
    tft_printCentered("YOUR MEDICINE", 0, 30, TFT_WIDTH, 30, 2, TFT_BLACK);
    // שם תרופה
    tft_printCentered(medicineName, 0, 80, TFT_WIDTH, 40, 2, TFT_WHITE);
    // מינון
    tft_printCentered(dose, 0, 130, TFT_WIDTH, 30, 2, TFT_CYAN);
    // כפתור אישור
    TFTButton btn;
    tft_buttonInit(&btn, 20, 220, 200, 60, TFT_GREEN, TFT_BLACK, "I TOOK IT");
    tft_buttonDraw(&btn);
}

void tft_showConfirmScreen(bool ok) {
    tft_fillScreen(ok ? TFT_GREEN : TFT_RED);
    const char* msg  = ok ? "MEDICINE TAKEN" : "MISSED DOSE";
    const char* icon = ok ? ":)"            : "!";
    tft_printCentered(icon, 0, 80,  TFT_WIDTH, 80, 4, TFT_WHITE);
    tft_printCentered(msg,  0, 170, TFT_WIDTH, 40, 2, TFT_WHITE);
    delay(2000);
    tft_fillScreen(TFT_BLACK);
}

void tft_showInventoryScreen(bool cellStatus[4]) {
    tft_fillScreen(TFT_BLACK);
    tft_printCentered("INVENTORY", 0, 5, TFT_WIDTH, 30, 2, TFT_WHITE);

    const char* labels[4] = {"Cell 1","Cell 2","Cell 3","Cell 4"};
    uint16_t colors[4];
    for (uint8_t i = 0; i < 4; i++) {
        colors[i] = cellStatus[i] ? TFT_GREEN : TFT_RED;
    }

    // 2×2 grid
    uint16_t bw = 100, bh = 100, gap = 10;
    uint16_t startX = (TFT_WIDTH - 2*bw - gap) / 2;
    uint16_t startY = 45;

    for (uint8_t i = 0; i < 4; i++) {
        uint16_t cx = startX + (i % 2) * (bw + gap);
        uint16_t cy = startY + (i / 2) * (bh + gap);
        tft_fillRoundRect(cx, cy, bw, bh, 10, colors[i]);
        tft_printCentered(labels[i], cx, cy,     bw, 40, 1, TFT_WHITE);
        tft_printCentered(cellStatus[i] ? "OK" : "EMPTY", cx, cy+40, bw, 40, 2, TFT_WHITE);
    }
}

void tft_showError(const char* message) {
    tft_fillScreen(TFT_BLACK);
    tft_fillRect(0, 0, TFT_WIDTH, 50, TFT_RED);
    tft_printCentered("ERROR", 0, 0, TFT_WIDTH, 50, 3, TFT_WHITE);
    tft_printCentered(message, 10, 80, TFT_WIDTH-20, 160, 1, TFT_ORANGE);
}
