#include "Dispenser.h"

// ============================================================
// Dispenser
//  הוצאת כדור מתא ונעילה חזרה
// ============================================================

Dispenser dispenser(13);

void setup() {
    Serial.begin(115200);
    dispenser.begin();
    // הוצאת כדור מתא 0
    dispenser.releasePill(0);
    delay(2000);

    // נעילת הסרוו
    dispenser.lockServo();
}

void loop() {
    dispenser.releasePill(0);
    delay(2000);
    dispenser.releasePill(1);
    delay(2000);
    dispenser.releasePill(2);
    delay(2000);
    dispenser.releasePill(3);
    delay(2000);

    // נעילת הסרוו
    dispenser.lockServo();
}
