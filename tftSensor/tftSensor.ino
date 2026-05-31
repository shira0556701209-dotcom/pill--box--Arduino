/*
 * tftSensor.ino
 * -------------
 * דוגמת שימוש בספריית tftSensor
 * מסך TFT 2.8" עם מגע – ממשק משתמש לקופסת תרופות חכמה.
 *
 * חיבור:
 *   TFT CS  → פין 10   Touch CS  → פין 6
 *   TFT DC  → פין 9    Touch IRQ → פין 5
 *   TFT RST → פין 8    MISO      → פין 12
 *   MOSI    → פין 11   BL        → פין 7
 *   SCK     → פין 13
 */

#include "tftSensor.h"

// כפתורי ממשק ראשי
TFTButton btnStatus;
TFTButton btnConfirm;

void setup() {
    Serial.begin(9600);

    // אתחול מסך בכיוון פורטרט
    tft_init(0);
    tft_setBacklight(true);

    Serial.println("TFT initialized");

    // ── הצגת מסך בית ────────────────────────────────────────
    tft_showHomeScreen("08:30");
    delay(2000);

    // ── הצגת תזכורת תרופה ───────────────────────────────────
    tft_showReminderScreen("Metformin", "500mg x2");
    delay(3000);

    // ── בניית ממשק עם כפתורים ───────────────────────────────
    tft_fillScreen(TFT_BLACK);

    tft_setTextColor(TFT_WHITE, TFT_BLACK);
    tft_setTextSize(2);
    tft_setCursor(10, 10);
    tft_println("Smart Pill Box");

    tft_buttonInit(&btnStatus,  20, 80,  200, 60, TFT_BLUE,  TFT_WHITE, "INVENTORY");
    tft_buttonInit(&btnConfirm, 20, 180, 200, 60, TFT_GREEN, TFT_BLACK, "TOOK PILL");

    tft_buttonDraw(&btnStatus);
    tft_buttonDraw(&btnConfirm);
}

void loop() {
    // ── בדיקת מגע ───────────────────────────────────────────
    TouchPoint tp;
    if (tft_getTouch(&tp)) {
        Serial.print("Touch at: ");
        Serial.print(tp.x); Serial.print(", "); Serial.println(tp.y);

        // כפתור מלאי
        if (tft_buttonHit(&btnStatus, tp.x, tp.y)) {
            Serial.println(">> Inventory button pressed");
            // דוגמה: תא 0 ריק, שאר מלאים
            bool cells[4] = {false, true, true, true};
            tft_showInventoryScreen(cells);
            delay(3000);
            // חזרה לממשק ראשי
            tft_fillScreen(TFT_BLACK);
            tft_setTextColor(TFT_WHITE, TFT_BLACK);
            tft_setTextSize(2);
            tft_setCursor(10, 10);
            tft_println("Smart Pill Box");
            tft_buttonDraw(&btnStatus);
            tft_buttonDraw(&btnConfirm);
        }

        // כפתור אישור נטילה
        if (tft_buttonHit(&btnConfirm, tp.x, tp.y)) {
            Serial.println(">> Confirm button pressed");
            tft_showConfirmScreen(true);  // true = נטל, false = פספס
            tft_fillScreen(TFT_BLACK);
            tft_setCursor(10, 10);
            tft_setTextSize(2);
            tft_println("Smart Pill Box");
            tft_buttonDraw(&btnStatus);
            tft_buttonDraw(&btnConfirm);
        }

        delay(300); // debounce מגע
    }
}
