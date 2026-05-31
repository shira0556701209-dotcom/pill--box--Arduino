#include "ledSensor.h"

// ============================================================
//   שימוש ב-ledSensor
//  התראות וחזותיות
// ============================================================

ledSensor led(27, 14, 12);

void setup() {
    Serial.begin(115200);
    led.begin();
    // כיבוי הכל
    led.turnOffAll();
}

void loop() {
      
    // התראת תרופה
    led.activateLED(ALERT_MEDICATION_TIME);
    delay(1000);

    // שינוי צבע LED לירוק (נטילה הושלמה)
    led.setLightStatus(COLOR_GREEN);
    delay(2000);

    // הבהוב 3 פעמים
    led.blinkLED(3);
}
