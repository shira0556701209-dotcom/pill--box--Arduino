#ifndef BUZZER_SENSOR_H
#define BUZZER_SENSOR_H

#include <Arduino.h>
#include <AlertType.h>


// ============================================================
//  BuzzerSensor -  Buzzer 
//   צליל דרך LEDC ערוץ 1 (PWM)
// ============================================================

// #define BUZZER_PIN     4

#define BUZZ_CHANNEL   1      // ערוץ LEDC לזמזם
#define BUZZ_RES_BITS  8      // רזולוציה 8 ביט לזמזם

class BuzzerSensor {
private:
    int buzzerPin;
public:
    //בנאי
    BuzzerSensor(int buzzPin);
    // אתחול
    void begin();
    // צפצוף בתדר נתון לזמן נתון
    void beep(int freqHz, int durationMs);
    // עצירת הזמזם
    void stopBeep();
    void activateBuzzer(AlertType type); // שרות
};

#endif // ALERT_SYSTEM_H
