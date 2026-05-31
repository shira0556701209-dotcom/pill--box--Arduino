#include "BuzzerSensor.h"

// -------------------------------------------------------
// בנאי
// -------------------------------------------------------
BuzzerSensor::BuzzerSensor(int buzzPin) {
    buzzerPin = buzzPin;
    
}
//----------------------------------
//אתחול 
//----------------------------------
void BuzzerSensor::begin(){
    pinMode(buzzerPin,    OUTPUT);
    stopBeep();
    Serial.println("buzzer אותחל - GPIO ידני");
}

// -------------------------------------------------------
// beep - הפקת צליל דרך LEDC
// -------------------------------------------------------
void BuzzerSensor::beep(int freqHz, int durationMs) {
    // הדרך החדשה: מחברים את הפין לתדר המבוקש ורזולוציה (למשל 8 ביט) בשורה אחת
    ledcAttach(buzzerPin, freqHz, 8);
    ledcWriteTone(buzzerPin, freqHz); // חיבור הפין לערוץ - רמת התדר
    delay(durationMs);
    stopBeep();
}

void BuzzerSensor::stopBeep() {
   // מנתקים את הפין מהבקר של ה PWM
    ledcDetach(buzzerPin);
    digitalWrite(buzzerPin, LOW);
}


// -------------------------------------------------------
// activateBuzzer שרות 
// -------------------------------------------------------
void BuzzerSensor::activateBuzzer(AlertType type) {
    switch (type) {
        case ALERT_MEDICATION_TIME:
            Serial.println("[AlertSystem] הגיע זמן תרופה!");
            beep(1000, 500); delay(150);
            beep(1000, 500);
            break;

        case ALERT_PAIN_REQUEST:
            Serial.println("[AlertSystem] בקשת משכך כאבים");
            beep(800, 300);
            break;

        case ALERT_STOCK_LOW:
            Serial.println("[AlertSystem] מלאי נמוך!");
            for (int i = 0; i < 3; i++) { beep(600, 200); delay(100); }
            break;

        case ALERT_MISSED_DOSE:
            Serial.println("[AlertSystem] החמצת מנה!");
            for (int i = 0; i < 5; i++) { beep(1200, 150); delay(100); }
            break;
    }
}
