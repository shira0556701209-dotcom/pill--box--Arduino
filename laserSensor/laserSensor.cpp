#include "laserSensor.h"
//האם המערכת אותחלה
static bool initialized = false;

// =========================================================
// בנאי
// =========================================================

laserSensor::laserSensor(int pins[NUM_CELLS]) {
    for (int i = 0; i < NUM_CELLS; i++) {
        sensorPin[i] = pins[i];
        // cellIndex[i] = i;
        lastState[i] = false;
    }
}
// =========================================================
//  אתחול
// =========================================================

void laserSensor::begin() {
    for (int i = 0; i < NUM_CELLS; i++) {
        // הגדרת הפין כקלט עם עמידות pull-up פנימית
        // (כאשר קרן הלייזר פגעת בדיוד - הפין LOW; חסומה - HIGH)
        pinMode(sensorPin[i], INPUT_PULLUP);
    }
    initialized = true;
    Serial.println("laser sensor begin!");
}
// =========================================================
//  קריאה גולמית מפין אחד
// =========================================================
bool laserSensor::readRaw(int cellIndex) {
    // HIGH = קרן חסומה (יש כדורים), LOW = קרן פתוחה (ריק)
    // עם INPUT_PULLUP: כשדיוד לא מקבל אור -> HIGH
    return (digitalRead(sensorPin[cellIndex]) == HIGH);
}

/**
 * קורא מצב תא בודד
 * @param cellIndex אינדקס התא (0–3)
 * @return CELL_OCCUPIED אם יש כדורים, CELL_EMPTY אם ריק, CELL_ERROR אם שגיאה
 */
int laserSensor::readCell(int cellIndex) {
    if (!initialized || cellIndex >= NUM_CELLS) {
        return CELL_ERROR;
    }
    return readRaw(cellIndex) ? CELL_OCCUPIED : CELL_EMPTY;
}
void laserSensor::readAll(int results[NUM_CELLS]) {
    for (uint8_t i = 0; i < NUM_CELLS; i++) {
        results[i] = readCell(i);
    }
}
/**
 * בודק אם תא מסוים ריק (אין כדורים)
 * @param cellIndex אינדקס התא (0–3)
 * @return true אם ריק
 */
bool laserSensor::isCellEmpty(int cellIndex) {
    if (!initialized || cellIndex >= NUM_CELLS) return false;
    return (readRaw(cellIndex) == false); // קרן פתוחה = ריק
}
/**
 * בודק אם כל 4 התאים ריקים
 * @return true אם כולם ריקים
 */
bool laserSensor::allEmpty() {
    for (uint8_t i = 0; i < NUM_CELLS; i++) {
        if (!isCellEmpty(i)) return false;
    }
    return true;
}
/**
 * בודק אם חל שינוי במצב תא מאז הקריאה האחרונה
 * @param cellIndex אינדקס התא (0–3)
 * @return true אם השתנה
 */
bool laserSensor::stateChanged(int cellIndex) {
    if (!initialized || cellIndex >= NUM_CELLS) return false;
    bool current = readRaw(cellIndex);
    return (current != lastState[cellIndex]);
}
/**
 * מחזיר את מספר התאים הריקים כרגע
 * @return מספר בין 0 ל-4
 */
int laserSensor::emptyCellCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < NUM_CELLS; i++) {
        if (isCellEmpty(i)) count++;
    }
    return count;
}
/**
 * מעדכן את המצב הפנימי של כל החיישנים - יש לקרוא בכל סבב loop()
 */
void laserSensor::laserSensorUpdate() {
    if (!initialized) return;
    for (uint8_t i = 0; i < NUM_CELLS; i++) {
        lastState[i] = readRaw(i);
    }
} 
// #include "laserSensor.h"

// // משתניפ פנימיים
// static bool initialized = false;
// laserSensor cells [NUM_CELLS]

// // =========================================================
// //  אתחול
// // =========================================================

// laserSensor::laserSensor(int pins[NUM_CELLS]) {
//     sensorPin  = pin;
//     cellIndex  = (i++) % NUM_CELLS;
//     lastState  = false;
//     pinMode(pin, INPUT_PULLUP);
//     initialized = true;
// }

// // =========================================================
// //  קריאה גולמית מפין אחד
// // =========================================================

// static bool readRaw(uint8_t cellIndex) {
//     // HIGH = קרן חסומה (יש כדורים) LOW = קרן פתוחה (ריק)
//     // עם INPUT_PULLUP: כשדיוד לא מקבל אור HIGH
//     return (digitalRead(_cells[cellIndex].sensorPin) == HIGH);
// }

// // =========================================================
// //  פונקציות ציבוריות
// // =========================================================

// int8_t laserSensor_readCell(uint8_t cellIndex) {
//     if (!_initialized || cellIndex >= NUM_CELLS) {
//         return CELL_ERROR;
//     }
//     return _readRaw(cellIndex) ? CELL_OCCUPIED : CELL_EMPTY;
// }

// void laserSensor_readAll(int8_t results[NUM_CELLS]) {
//     for (uint8_t i = 0; i < NUM_CELLS; i++) {
//         results[i] = laserSensor_readCell(i);
//     }
// }

// bool laserSensor_isCellEmpty(uint8_t cellIndex) {
//     if (!_initialized || cellIndex >= NUM_CELLS) return false;
//     return (_readRaw(cellIndex) == false); // קרן פתוחה = ריק
// }

// bool laserSensor_allEmpty() {
//     for (uint8_t i = 0; i < NUM_CELLS; i++) {
//         if (!laserSensor_isCellEmpty(i)) return false;
//     }
//     return true;
// }

// bool laserSensor_stateChanged(uint8_t cellIndex) {
//     if (!_initialized || cellIndex >= NUM_CELLS) return false;
//     bool current = _readRaw(cellIndex);
//     return (current != _cells[cellIndex].lastState);
// }

// uint8_t laserSensor_emptyCellCount() {
//     uint8_t count = 0;
//     for (uint8_t i = 0; i < NUM_CELLS; i++) {
//         if (laserSensor_isCellEmpty(i)) count++;
//     }
//     return count;
// }

// void laserSensor_update() {
//     if (!_initialized) return;
//     for (uint8_t i = 0; i < NUM_CELLS; i++) {
//         _cells[i].lastState = _readRaw(i);
//     }
// }
