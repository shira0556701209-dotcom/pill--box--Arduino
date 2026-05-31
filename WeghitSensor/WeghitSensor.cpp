#include "WeghitSensor.h"

// ============================================================
//  HX711 - מימוש ידני של פרוטוקול סדרתי (Bit-Banging)
//  השבב לא עובד ב-I2C רגיל. הוא דורש 24 פעימות שעון כדי לשחרר קריאת משקל באורך 24 ביט.
// ============================================================
long WeghitSensor::hx711Read() {
    // נמתין עד שפין הנתונים (DOUT) ירד למצב LOW - זה הסימן של השבב שהמשקל סיים חישוב פנימי והנתונים מוכנים לקריאה
    unsigned long timeout = millis() + 200;
    while (digitalRead(doutPin) == HIGH) {
        if (millis() > timeout) return 0;  // הגנה: אם הרכיב מנותק, לא נתקע בלולאה אינסופית
    }

    long data = 0;

    // לולאה לקריאת 24 ביטים בזה אחר זה (מהביט הכי חשיב - MSB, לביט הכי פחות חשיב)
    for (int i = 0; i < 24; i++) {
        digitalWrite(sckPin, HIGH);     // מעלים את קו השעון - השבב מוציא ביט מידע אחד לקו הנתונים
        delayMicroseconds(1);
        
        // הזזת ביטים: דוחפים את המידע הקיים שמאלה, ומכניסים בביט הימני ביותר את הקריאה מהפין (0 או 1)
        data = (data << 1) | digitalRead(doutPin);
        
        digitalWrite(sckPin, LOW);      // מורידים את קו השעון - נערכים לביט הבא
        delayMicroseconds(1);
    }

    //  פעימה מספר 25 (פעימת שעון אחרונה חובה):
    // אומרת לשבב HX711 שבפעם הבאה נרצה לקרוא שוב מערוץ A בהגברה מקסימלית של 128 (Gain)
    digitalWrite(sckPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(sckPin, LOW);
    delayMicroseconds(1);

    // המרת מספר משלים ל-2 (Sign Extension):
    // שבב ה-HX711 מחזיר מספר עם סימן פלוס/מינוס באורך 24 ביט.
    // מכיוון שמשתנה מסוג long ב-ESP32 תופס 32 ביט, אם הביט ה-24 הוא '1' (מספר שלילי), 
    // אנו חייבים למלא את כל 8 הביטים העליונים הנותרים ב-'1' (0xFF000000) כדי שהמחשב יבין שהערך שלילי.
    if (data & 0x800000) data |= 0xFF000000;

    return data;
}

// כיול האפס: קורא 10 פעמים את המשקל הנוכחי ומחשב ממוצע. הערך הזה ייחשב כמשקל של כוס ריקה.
void WeghitSensor::hx711Tare() {
    zeroOffset = hx711ReadAverage(10); // קביעת נקודת האפס
    Serial.print("[InventoryModule] Tare (אפס): ");
    Serial.println(zeroOffset);
}

// פונקציה  קוראת את החיישן מספר פעמים ומחזירה ממוצע
long WeghitSensor::hx711ReadAverage(int times) {
    long sum = 0;
    for (int i = 0; i < times; i++) {
        sum += hx711Read(); // קריאה בודדת
        delay(5);           // הפסקה קטנטנה לחיישן להתייצב בין קריאות
    }
    return sum / times;     // החזרת הממוצע
}

// החזרת המשקל הנוכחי בגרמים נטו
float WeghitSensor::getWeightGrams() {
    // ממוצע של 5 שקילות - כדי לקבל מידע יציב יותר
    long raw = hx711ReadAverage(5); 
    
    // הפחתת משקל הכוס (zeroOffset) וחלוקה במקדם הכיול
    return (float)(raw - zeroOffset) / calibFactor;
}

// ============================================================
//  בנאי
// ============================================================
WeghitSensor::WeghitSensor(int pinDout, int pinSck) {

    doutPin = pinDout;
    sckPin = pinSck;
    // ערכי כיול התחלתיים
    calibFactor = -494.14f;  
    zeroOffset  = 0;
}
// ============================================================
//  אתחול - הגדרת מצבי הפינים בחומרה וביצוע איפוס ראשוני
// ============================================================
void WeghitSensor::begin() {

    // הגדרת פינים עבור מד המשקל
    pinMode(doutPin, INPUT);
    pinMode(sckPin,  OUTPUT);
    digitalWrite(sckPin, LOW); // נקודת מוצא כבויה
    hx711Tare();  // מריצים טארה אוטומטית בהדלקה
    Serial.println("wheight sensor begins");
}
// ============================================================
//  isCupEmpty - בדיקה האם המטופל פיזית אסף את התרופה שלו
// ============================================================
bool WeghitSensor::isCupEmpty() {
    float weight = getWeightGrams(); // קריאת המשקל העדכני בגרמים
    
    // אם המשקל שנמדד נמוך מ-1.0 גרם, המערכת מסיקה שהכוס ריקה לחלוטין והתרופה נלקחה
    bool empty = (weight < 0.7f);
    
    Serial.print("[InventoryModule] משקל כוס: ");
    Serial.print(weight);
    Serial.println(empty ? "g - ריקה ✓" : "g - לא ריקה");
    return empty;
}

