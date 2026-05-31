#include "WeghitSensor.h"

// ============================================================
//  דוגמת שימוש ב-InventoryModule
//  בדיקת מלאי ואישור נטילה
// ============================================================

WeghitSensor weghit(32, 33);

void setup() {
    Serial.begin(9600);
    weghit.begin();
    // בדיקה אם הכוס ריקה
    if (weghit.isCupEmpty()) {
        Serial.println("הכוס ריקה - התרופה נלקחה");
    }
   
}

void loop() {
    if (weghit.isCupEmpty()) {
        Serial.println("הכוס ריקה - התרופה נלקחה");
    }
    delay(3000);
}
