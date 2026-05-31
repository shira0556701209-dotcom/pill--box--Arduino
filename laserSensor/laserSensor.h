#ifndef LASER_SENSOR_H
#define LASER_SENSOR_H

#include <Arduino.h>

// מספר התאים בקופסה
#define NUM_CELLS 4

// מצב תא
#define CELL_EMPTY    0   // קרן הלייזר עוברת - תא ריק
#define CELL_OCCUPIED 1   // קרן הלייזר חסומה - יש כדורים
#define CELL_ERROR    -1  // שגיאת קריאה

// מבנה המייצג חיישן לייזר לתא אחד
class laserSensor {
private:
    int sensorPin[NUM_CELLS];   // פין חיישן (מקלט)
    // int cellIndex[NUM_CELLS];   // אינדקס התא (0-3)
    bool lastState[NUM_CELLS];   // מצב קודם (לזיהוי שינוי)
    // =========================================================
    //  קריאה גולמית מפין אחד
    //=========================================================
    bool readRaw(int cellIndex);
public:
// =========================================================
//  אתחול
// =========================================================

/**
 * בנאי
 * @param pin פין המקלט
 */
laserSensor(int pins[NUM_CELLS]);
/**
 * מאתחל את  חיישן הלייזר
 */
void begin();
// =========================================================
//  קריאה
// =========================================================

/**
 * קורא מצב תא בודד
 * @param cellIndex אינדקס התא (0–3)
 * @return CELL_OCCUPIED אם יש כדורים, CELL_EMPTY אם ריק, CELL_ERROR אם שגיאה
 */
int readCell(int cellIndex);

/**
 * בודק אם תא מסוים ריק (אין כדורים)
 * @param cellIndex אינדקס התא (0–3)
 * @return true אם ריק
 */
bool isCellEmpty(int cellIndex);

/**
 * בודק אם כל 4 התאים ריקים
 * @return true אם כולם ריקים
 */
bool allEmpty();
/**
 * @param results[NUM_CELLS] מערך בו יושמו התוצאות
 * קורא את מצב כל 4 התאים
 */
void readAll(int results[NUM_CELLS]);

/**
 * בודק אם חל שינוי במצב תא מאז הקריאה האחרונה
 * @param cellIndex אינדקס התא (0–3)
 * @return true אם השתנה
 */
bool stateChanged(int cellIndex);

/**
 * מחזיר את מספר התאים הריקים כרגע
 * @return מספר בין 0 ל-4
 */
int emptyCellCount();

// =========================================================
//  עדכון מצב פנימי (קריאה בלופ)
// =========================================================

/**
 * מעדכן את המצב הפנימי של כל החיישנים - יש לקרוא בכל סבב loop()
 */
void laserSensorUpdate();

};


#endif // LASER_SENSOR_H
