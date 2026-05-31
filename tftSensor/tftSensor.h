#ifndef TFT_SENSOR_H
#define TFT_SENSOR_H

#include <Arduino.h>

// =========================================================
//  הגדרות מסך TFT 2.8" (ILI9341 / ST7789 – SPI)
//  חיבור SPI רגיל: SCK, MOSI, MISO דרך חומרה
// =========================================================

// פינים
#define TFT_CS_PIN    22   // Chip Select
#define TFT_DC_PIN    13   // Data/Command
#define TFT_RST_PIN   23   // Reset
#define TFT_BL_PIN    27   // Backlight (אופציונלי, -1 לביטול)

// רזולוציית מסך
#define TFT_WIDTH    240
#define TFT_HEIGHT   320

// =========================================================
//  הגדרות מגע (Touch) – XPT2046 / מגע נגד-עמיד
// =========================================================
#define TOUCH_CS_PIN   33   // פין CS של ה-touch controller
#define TOUCH_IRQ_PIN  34   // פין IRQ (אופציונלי, -1 לביטול)

// כיול מגע – ערכים גולמיים מ-ADC
#define TOUCH_X_MIN   200
#define TOUCH_X_MAX  3800
#define TOUCH_Y_MIN   200
#define TOUCH_Y_MAX  3800

// =========================================================
//  צבעים בסיסיים (RGB565)
// =========================================================
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_ORANGE  0xFD20
#define TFT_GRAY    0x8410

// =========================================================
//  מבנים
// =========================================================

// נקודת מגע
struct TouchPoint {
    int16_t x;       // עמודה (0..TFT_WIDTH-1)
    int16_t y;       // שורה  (0..TFT_HEIGHT-1)
    bool    pressed; // האם נגע בפועל?
};

// כפתור פשוט על המסך
struct TFTButton {
    int16_t  x, y;          // פינה שמאלית-עליונה
    uint16_t w, h;          // רוחב וגובה
    uint16_t colorBg;       // צבע רקע
    uint16_t colorText;     // צבע טקסט
    char     label[20];     // תווית
    bool     enabled;       // האם פעיל?
};

// =========================================================
//  אתחול
// =========================================================

/**
 * מאתחל מסך TFT ומודול מגע
 * @param rotation סיבוב מסך: 0=פורטרט, 1=לנדסקייפ, 2,3 הפוך
 */
void tft_init(uint8_t rotation);

/**
 * מכבה / מדליק תאורת רקע
 * @param on true = דלוק
 */
void tft_setBacklight(bool on);

// =========================================================
//  ציור בסיסי
// =========================================================

/** מנקה את המסך בצבע נתון */
void tft_fillScreen(uint16_t color);

/** מצייר פיקסל בודד */
void tft_drawPixel(int16_t x, int16_t y, uint16_t color);

/** מצייר קו */
void tft_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/** מצייר מלבן ריק */
void tft_drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color);

/** מצייר מלבן מלא */
void tft_fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color);

/** מצייר עיגול ריק */
void tft_drawCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color);

/** מצייר עיגול מלא */
void tft_fillCircle(int16_t x0, int16_t y0, uint16_t r, uint16_t color);

/** מצייר מלבן עם פינות מעוגלות */
void tft_fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);

// =========================================================
//  טקסט
// =========================================================

/**
 * מגדיר מיקום הדפסת טקסט
 */
void tft_setCursor(int16_t x, int16_t y);

/**
 * מגדיר גודל טקסט (1=קטן, 2=בינוני, 3=גדול)
 */
void tft_setTextSize(uint8_t size);

/**
 * מגדיר צבע טקסט ורקע
 */
void tft_setTextColor(uint16_t color, uint16_t bgColor);

/**
 * מדפיס מחרוזת על המסך
 */
void tft_print(const char* text);

/**
 * מדפיס מחרוזת עם שינוי שורה
 */
void tft_println(const char* text);

/**
 * מדפיס מספר שלם
 */
void tft_printInt(int32_t value);

/**
 * מדפיס טקסט ממורכז בתוך אזור
 */
void tft_printCentered(const char* text, int16_t x, int16_t y, uint16_t w, uint16_t h,
                        uint8_t textSize, uint16_t color);

// =========================================================
//  כפתורים
// =========================================================

/**
 * מאתחל מבנה כפתור
 */
void tft_buttonInit(TFTButton* btn, int16_t x, int16_t y, uint16_t w, uint16_t h,
                    uint16_t colorBg, uint16_t colorText, const char* label);

/**
 * מצייר כפתור על המסך
 */
void tft_buttonDraw(const TFTButton* btn);

/**
 * בודק אם נקודת מגע נמצאת בתוך הכפתור
 */
bool tft_buttonHit(const TFTButton* btn, int16_t tx, int16_t ty);

// =========================================================
//  מגע
// =========================================================

/**
 * קורא נקודת מגע גולמית מה-ADC ומתרגם לקואורדינטות מסך
 * @param tp פלט – מבנה TouchPoint
 * @return true אם יש מגע
 */
bool tft_getTouch(TouchPoint* tp);

/**
 * ממתין למגע ומחזיר את הנקודה
 * חוסם עד שיש מגע
 */
TouchPoint tft_waitForTouch();

// =========================================================
//  מסכים מוכנים לפרוייקט
// =========================================================

/**
 * מציג מסך בית (שם פרוייקט + שעה)
 * @param timeStr מחרוזת שעה, למשל "08:00"
 */
void tft_showHomeScreen(const char* timeStr);

/**
 * מציג הודעת תזכורת לתרופה
 * @param medicineName שם התרופה
 * @param dose מינון
 */
void tft_showReminderScreen(const char* medicineName, const char* dose);

/**
 * מציג אישור נטילה
 * @param ok true = נטלו, false = לא נטלו
 */
void tft_showConfirmScreen(bool ok);

/**
 * מציג מצב מלאי (4 תאים)
 * @param cellStatus מערך 4 bool: true=יש כדורים, false=ריק
 */
void tft_showInventoryScreen(bool cellStatus[4]);

/**
 * מציג הודעת שגיאה
 */
void tft_showError(const char* message);

#endif // TFT_SENSOR_H
