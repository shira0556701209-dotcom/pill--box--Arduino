#ifndef LED_SENSOR_H
#define LED_SENSOR_H

#include <Arduino.h>
#include <AlertType.h>

// ============================================================
//  ledSensor- LED RGB 
//  LED: שליטה ב-GPIO ישירות
// ============================================================

#define LED_RED_PIN    27
#define LED_GREEN_PIN  14
#define LED_BLUE_PIN   12

enum LightColor {
    COLOR_OFF, 
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW
};

class ledSensor {
private:
    int redPin;
    int greenPin;
    int bluePin;
    LightColor currentColor;
public:
    ledSensor(int rPin, int gPin, int bPin);
    void begin();//אתחול
    void activateLED(AlertType type); // שרות
    void setLightStatus(LightColor color); //שינוי צבע הלד
    void blinkLED(int times); // הבהוב
    void turnOffAll(); //כיבוי הכל
};

#endif // LED_SENSOR_H
