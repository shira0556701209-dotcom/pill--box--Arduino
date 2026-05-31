#include "distansSensor.h"

// =========================================================
//  משתנה פנימיים
// =========================================================
static bool initialized   = false;

// מצבים לזיהוי נטילה
// IDLE -> HAND_IN (יד נכנסה) -> HAND_OUT (יד יצאה = נטילה)
typedef enum {
    STATE_IDLE = 0,
    STATE_HAND_IN,
    STATE_HAND_OUT
} TakeState;

static TakeState takeState    = STATE_IDLE;
static bool      medicineTaken = false;
static uint32_t  handInTime   = 0;

// debounce: יד חייבת להיות בפנים לפחות N מ"ש לפני אישור
#define HAND_IN_DEBOUNCE_MS  300UL

// =========================================================
// בנאי
// =========================================================

DistansSensor::DistansSensor(int trPin, int ecPin) {
    trigPin = trPin;
    echoPin = ecPin;
    lastDistance   = 0.0f;
    handDetected   = false;
    lastDetectTime = 0;
    takeState    = STATE_IDLE;
    medicineTaken = false;
}

//אתחול
void DistansSensor::begin(){
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    digitalWrite(trigPin, LOW);
    Serial.println("חיישן מרחק הותחל");
    initialized  = true;
}
// =========================================================
//  שליחת פולס ומדידת 
// =========================================================

int8_t DistansSensor::singleMeasure(float* outCm) {
    // שלח פולס TRIGGER
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(DISTANS_TRIGGER_PULSE_US);
    digitalWrite(trigPin, LOW);

    // מדוד זמן echo
    uint32_t duration = pulseIn(echoPin, HIGH, DISTANS_ECHO_TIMEOUT_US);

    if (duration == 0) {
        return DISTANS_ERR_TIMEOUT;
    }

    // חישוב מרחק: מהירות קול ~343 / מ"ש → 29.1 מיקרו-שניות לס"מ הלוך-חזור
    float cm = (float)duration / 58.0f;
    Serial.print(" :המרחק שנצפה על ידי חיישן המרחק");
    Serial.println(cm);

    if (cm < DISTANS_MIN_CM || cm > DISTANS_MAX_CM) {
        return DISTANS_ERR_RANGE;
    }

    *outCm = cm;
    return DISTANS_OK;
}

// =========================================================
//  פונקציות ציבוריות
// =========================================================

/**
 * מבצע מדידת מרחק בודדת
 * @param outCm מצביע לפלט המרחק בס"מ
 * @return DISTANS_OK, DISTANS_ERR_TIMEOUT, או DISTANS_ERR_RANGE
 */
int8_t DistansSensor::Measure(float* outCm) {
    if (!initialized) return DISTANS_ERR_TIMEOUT;
    int8_t result = singleMeasure(outCm);
    if (result == DISTANS_OK) {
        lastDistance = *outCm;
    }
    return result;
}

/**
 * מבצע מספר מדידות ומחזיר את הממוצע (הפחתת רעש)
 * @param outCm מצביע לפלט הממוצע בס"מ
 * @return DISTANS_OK אם הצליח, אחרת קוד שגיאה
 */
int8_t DistansSensor::MeasureAvg(float* outCm) {
    if (!initialized) return DISTANS_ERR_TIMEOUT;

    float   sum   = 0.0f;
    uint8_t valid = 0;

    for (int i = 0; i < DISTANS_SAMPLE_COUNT; i++) {
        float sample = 0.0f;
        if (singleMeasure(&sample) == DISTANS_OK) {
            sum += sample;
            valid++;
        }
        delay(15); // המתן קצת בין מדידות
    }

    if (valid == 0) return DISTANS_ERR_TIMEOUT;

    *outCm = sum / (float)valid;
    lastDistance = *outCm;
    Serial.print(" :המרחק האחרון");
    Serial.println(lastDistance);
    return DISTANS_OK;
}

/**
 * בודק אם יד מוחדרת לכוס (מרחק מתחת לסף)
 * @return true אם יד זוהתה
 */
bool DistansSensor::isHandDetected() {
    return handDetected;
}

/**
 * מחזיר את המרחק האחרון שנמדד ללא מדידה חדשה
 */
float DistansSensor::getLastDistance() {
    Serial.print(" :המרחק האחרון");
    Serial.println("lastDistance");
    return lastDistance;
}

/**
 * מאפס את דגל זיהוי הנטילה לאחר שטיפלנו בו
 */
void DistansSensor::resetTaken() {
    medicineTaken = false;
}

/**
 * בודק אם נטילת התרופה אושרה (יד נכנסה ויצאה מהכוס)
 * עושה debounce: מחכה שהיד תסתלק לפני אישור
 * @return true אם נטילה הושלמה
 */
bool DistansSensor::isMedicineTaken() {
    return medicineTaken;
}

// =========================================================
//  עדכון מצבים (loop)
// =========================================================

/**
 * מעדכן את המצב הפנימי – יש לקרוא בכל loop()
 */
void DistansSensor::distansUpdate() {
    if (!initialized) return;

    float cm = 0.0f;
    int8_t status = singleMeasure(&cm);

    if (status != DISTANS_OK) {
        // לא הצלחנו למדוד - נחשב כאין יד
        handDetected = false;

        if (takeState == STATE_HAND_IN) {
            // היד הייתה ועכשיו אין מדידה התעלם
        }
        return;
    }

    lastDistance = cm;
    bool handNow = (cm < HAND_DETECT_THRESHOLD_CM);
    handDetected = handNow;

    if (handNow) {
        lastDetectTime = millis();
    }

    // מצבים
    switch (takeState) {

        case STATE_IDLE:
            if (handNow) {
                takeState  = STATE_HAND_IN;
                handInTime = millis();
            }
            break;

        case STATE_HAND_IN:
            if (!handNow) {
                // יד יצאה – בדוק שהייתה בפנים מספיק זמן
                uint32_t duration = millis() - handInTime;
                if (duration >= HAND_IN_DEBOUNCE_MS) {
                    takeState     = STATE_HAND_OUT;
                    medicineTaken = true;
                } else {
                    // רעש קצר – חזור ל-IDLE
                    takeState = STATE_IDLE;
                }
            }
            break;

        case STATE_HAND_OUT:
            // נשאר כאן עד שהקוד הראשי קורא distans_resetTaken()
            break;
    }

    // אם אחרי אישור הכניסה שוב יד  מאפס (נטילה חדשה)
    if (takeState == STATE_HAND_OUT && handNow) {
        takeState = STATE_HAND_IN;
        handInTime = millis();
    }
}
