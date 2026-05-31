/*
 * laserSensor.ino
 * ---------------
 * דוגמת שימוש בספריית laserSensor
 * בודק זמינות כדורים ב-4 תאי הקופסה באמצעות חיישני לייזר.
 *
 * חיבור:
 *   תא 0 – דיוד מקלט → פין 2
 *   תא 1 – דיוד מקלט → פין 3
 *   תא 2 – דיוד מקלט → פין 4
 *   תא 3 – דיוד מקלט → פין 5
 *   (כל פין מוגדר INPUT_PULLUP בתוך הספריה)
 */

#include "laserSensor.h"

// פינים של מקלטי הלייזר – אחד לכל תא
int laserPins[NUM_CELLS] = {13, 12, 14, 27};
laserSensor laser(laserPins);

void setup() {
    Serial.begin(9600);
    laser.begin();
    Serial.println("laserSensor initialized");
}

void loop() {
    // עדכון מצב פנימי (חובה בכל סבב)
    laser.laserSensorUpdate();

    // ── קריאת כל התאים ──────────────────────────────────────
    int states[NUM_CELLS];
    laser.readAll(states);

    Serial.println("=== Cell Status ===");
    for (int i = 0; i < NUM_CELLS; i++) {
        Serial.print("Cell ");
        Serial.print(i);
        Serial.print(": ");
        if (states[i] == CELL_OCCUPIED) {
            Serial.println("OCCUPIED (pills present)");
        } else if (states[i] == CELL_EMPTY) {
            Serial.println("EMPTY – refill needed!");
        } else {
            Serial.println("ERROR");
        }
    }

    // ── בדיקת תא בודד ───────────────────────────────────────
    if (laser.isCellEmpty(0)) {
        Serial.println(">> Cell 0 is empty – alert!");
    }

    // ── זיהוי שינוי (כדור נלקח) ─────────────────────────────
    for (int i = 0; i < NUM_CELLS; i++) {
        if (laser.stateChanged(i)) {
            Serial.print(">> State changed in cell ");
            Serial.println(i);
        }
    }

    // ── סיכום ───────────────────────────────────────────────
    Serial.print("Empty cells: ");
    Serial.println(laser.emptyCellCount());

    if (laser.allEmpty()) {
        Serial.println(">> ALL CELLS EMPTY – please refill!");
    }

    Serial.println();
    delay(1000);
}
