/*
 * distansSensor.ino
 * -----------------
 * שימוש בספריית distansSensor
 * מזהה נטילת תרופה: יד שנכנסת לכוס ויוצאת.
 *
 * חיבור (HC-SR04):
 *   VCC  → 5V
 *   GND  → GND
 */

#include "distansSensor.h"

#define TRIG_PIN 5
#define ECHO_PIN 18

DistansSensor dist(5, 18);

void setup() {
    Serial.begin(9600);
    dist.begin();
    Serial.println("distansSensor initialized");
    Serial.print("Hand detection threshold: ");
    Serial.print(HAND_DETECT_THRESHOLD_CM);
    Serial.println(" cm");
}

void loop() {
    // ── עדכון מכונת המצבים (חובה בכל סבב) ──────────────────
    dist.distansUpdate();

    // ── קריאת מרחק נוכחי ────────────────────────────────────
    float cm = 0;
    int8_t status = dist.Measure(&cm);

    if (status == DISTANS_OK) {
        Serial.print("Distance: ");
        Serial.print(cm);
        Serial.println(" cm");
    } else if (status == DISTANS_ERR_TIMEOUT) {
        Serial.println("Measurement timeout – no echo");
    } else {
        Serial.println("Measurement out of range");
    }

    // ── בדיקת יד בכוס ───────────────────────────────────────
    if (dist.isHandDetected()) {
        Serial.println(">> HAND DETECTED in cup!");
    }

    // ── אישור נטילת תרופה ────────────────────────────────────
    if (dist.isMedicineTaken()) {
        Serial.println(">> MEDICINE TAKEN – confirmed!");
        // כאן: עדכן לוג, שלח התראה, הדלק נורה וכו'
        dist.resetTaken();  // איפוס לנטילה הבאה
    }

    // ── מדידת ממוצע (אופציונלי, מדויק יותר) ────────────────
    // float avgCm = 0;
    // if (distans_measureAvg(&avgCm) == DISTANS_OK) {
    //     Serial.print("Avg distance: "); Serial.println(avgCm);
    // }

    delay(2000);
}
