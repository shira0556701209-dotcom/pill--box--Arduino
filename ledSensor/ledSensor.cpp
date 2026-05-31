#include "ledSensor.h"

// -------------------------------------------------------
// בנאי
// -------------------------------------------------------
ledSensor::ledSensor(int rPin, int gPin, int bPin) {
    redPin = rPin;
    greenPin = gPin;
    bluePin = bPin;
}
//--------------------------------------------------------
//אתחול
//--------------------------------------------------------
void ledSensor::begin() {
    pinMode(redPin,   OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin,  OUTPUT);
    currentColor = COLOR_OFF;
    turnOffAll();
    Serial.println("led begins");
}
// -------------------------------------------------------
// ledSensor שרות 
// -------------------------------------------------------
void ledSensor::activateLED(AlertType type) {
    switch (type) {
        case ALERT_MEDICATION_TIME:
            Serial.println("[AlertSystem] הגיע זמן תרופה!");
            setLightStatus(COLOR_RED);
            break;

        case ALERT_PAIN_REQUEST:
            Serial.println("[AlertSystem] בקשת משכך כאבים");
            setLightStatus(COLOR_BLUE);
            break;

        case ALERT_STOCK_LOW:
            Serial.println("[AlertSystem] מלאי נמוך!");
            setLightStatus(COLOR_YELLOW);
            break;

        case ALERT_MISSED_DOSE:
            Serial.println("[AlertSystem] החמצת מנה!");
            setLightStatus(COLOR_RED);
            break;
    }
}

// -------------------------------------------------------
// setLightStatus - שליטה ישירה ב-GPIO של LED RGB
// -------------------------------------------------------
void ledSensor::setLightStatus(LightColor color) {
    // כיבוי הכל תחילה
    digitalWrite(redPin,   LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin,  LOW);
    currentColor = color;

    switch (color) {
        case COLOR_RED:    digitalWrite(redPin,   HIGH); break;
        case COLOR_GREEN:  digitalWrite(greenPin, HIGH); break;
        case COLOR_BLUE:   digitalWrite(bluePin,  HIGH); break;
        case COLOR_YELLOW:
            // צהוב = אדום + ירוק
            digitalWrite(redPin,   HIGH);
            digitalWrite(greenPin, HIGH);
            break;
        case COLOR_OFF:
        default: break;
    }
}

// -------------------------------------------------------
// blinkLED  הבהוב אדום
// -------------------------------------------------------
void ledSensor::blinkLED(int times) {
    for (int i = 0; i < times; i++) {
        setLightStatus(COLOR_RED);
        delay(300);
        setLightStatus(COLOR_OFF);
        delay(300);
    }
}

// -------------------------------------------------------
// turnOffAll כיבוי הכל
// -------------------------------------------------------
void ledSensor::turnOffAll() {
    digitalWrite(redPin,   LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin,  LOW);
    currentColor = COLOR_OFF;
}
