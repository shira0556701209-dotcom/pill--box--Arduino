#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <Arduino.h>

// // פינים עבור חיישן המרחק
// #define TRIG_PIN  25           // out
// #define ECHO_PIN  26           // in

// פינים עבור חיישן המשקל 
// #define HX711_DOUT 32          // out
// #define HX711_SCK  33          // in

class WeghitSensor {
private:
    int doutPin;
    int sckPin;
    long   zeroOffset;         // משתנה השומר את משקל הבסיס של הכוס כשהיא ריקה 
    float  calibFactor;        // מקדם כיול- כמה יחידות דיגיטליות של החיישן שוות לגרם אחד אמיתי

    // --- HX711 ידני ---
    // פונקציה פנימית שקוראת אות דיגיטלי גולמי באורך 24 ביט משבב המשקל
    long hx711Read();

    // פונקציית איפוס משקל: שוקלת את הכוס הריקה ושומרת את המשקל הזה כ"אפס" של המערכת
    void hx711Tare();

    // פונקצית כיול - כדי שהערך של המשקל יהיה יציב מחשבים ממוצע של מספר שקילות
    long hx711ReadAverage(int times);

public:
    // בנאי
    WeghitSensor(int pinDout, int pinSck);
    //אתחול
    void begin();
    // פונקציה המחשבת את המשקל האמיתי הנוכחי בגרמים
    float getWeightGrams();
    // פונקציית המחזירה אמת אם הכוס ריקה, או שקר אם הכדור עדיין מחכה שם
    bool isCupEmpty();

};

#endif // WEIGHT_SENSOR_H
