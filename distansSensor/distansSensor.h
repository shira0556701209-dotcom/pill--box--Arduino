#ifndef DISTANS_SENSOR_H
#define DISTANS_SENSOR_H

#include <Arduino.h>

// =========================================================
//  קבועים
// =========================================================

// סף מרחק לזיהוי יד (ס"מ) - כשמדידה קטנה מסף → יד זוהתה
#define HAND_DETECT_THRESHOLD_CM   12

// מספר מדידות לממוצע (הפחתת רעש)
#define DISTANS_SAMPLE_COUNT        5

// זמן בין פולס ל-echo (מיקרו-שניות)
#define DISTANS_TRIGGER_PULSE_US   10

// מקסימום מרחק תקין (ס"מ) - מעל זה = שגיאה / אין מדידה
#define DISTANS_MAX_CM             50

// מינימום מרחק תקין (ס"מ)
#define DISTANS_MIN_CM              2

// timeout לecho (מיקרו-שניות) – כ-30ms
#define DISTANS_ECHO_TIMEOUT_US  30000UL

// =========================================================
//  קודי סטטוס
// =========================================================
#define DISTANS_OK           0
#define DISTANS_ERR_TIMEOUT -1
#define DISTANS_ERR_RANGE   -2


class DistansSensor {
private:
    int trigPin;
    int echoPin;
    float    lastDistance;     // מרחק אחרון שנמדד (ס"מ)
    bool     handDetected;     // האם יד זוהתה
    uint32_t lastDetectTime;   // מיל-שניות של זיהוי אחרון
    // ביצוע המדידה וחישוב המרחק בפועל
    int8_t singleMeasure(float* outCm);
public:  
// =========================================================
//  אתחול
// =========================================================

/**
 * בנאי
* @param trigPin //input
 * @param echoPin //output
 */
DistansSensor(int trigPin, int echoPin);
/**
 * מאתחל את חיישן המרחק
 */
 void begin();
// =========================================================
//  מדידה
// =========================================================

/**
 * מבצע מדידת מרחק בודדת
 * @param outCm מצביע לפלט המרחק בס"מ
 * @return DISTANS_OK, DISTANS_ERR_TIMEOUT, או DISTANS_ERR_RANGE
 */
int8_t Measure(float* outCm);

/**
 * מבצע מספר מדידות ומחזיר את הממוצע (הפחתת רעש)
 * @param outCm מצביע לפלט הממוצע בס"מ
 * @return DISTANS_OK אם הצליח, אחרת קוד שגיאה
 */
int8_t MeasureAvg(float* outCm);

// =========================================================
//  זיהוי יד
// =========================================================

/**
 * בודק אם יד מוחדרת לכוס (מרחק מתחת לסף)
 * @return true אם יד זוהתה
 */
bool isHandDetected();

/**
 * בודק אם נטילת התרופה אושרה (יד נכנסה ויצאה מהכוס)
 * עושה debounce: מחכה שהיד תסתלק לפני אישור
 * @return true אם נטילה הושלמה
 */
bool isMedicineTaken();

// =========================================================
//  עדכון (לקרוא בכל סבב loop)
// =========================================================

/**
 * מעדכן את המצב הפנימי – יש לקרוא בכל loop()
 */
void distansUpdate();

/**
 * מחזיר את המרחק האחרון שנמדד (ס"מ) ללא מדידה חדשה
 */
float getLastDistance();

/**
 * מאפס את דגל זיהוי הנטילה לאחר שטיפלנו בו
 */
void resetTaken();
  
};



#endif // DISTANS_SENSOR_H
