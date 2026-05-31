#include "BuzzerSensor.h"

// ============================================================
//   שימוש ב-BuzzerSensor
//  התראות קוליות וחזותיות
// ============================================================

BuzzerSensor buzzer(4);

void setup() {
    Serial.begin(115200);
    buzzer.begin();
    buzzer.stopBeep();

}

void loop() {
      
    // התראת תרופה
    buzzer.activateBuzzer(ALERT_MEDICATION_TIME);
    delay(1000);
}
