#include "TimeManager.h"

// הגדרת הפינים
const uint8_t RTC_CLK = 5;
const uint8_t RTC_DAT = 18;
const uint8_t RTC_RST = 19;

TimeManager timeManager(RTC_CLK, RTC_DAT, RTC_RST);

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    timeManager.begin();
    // ==========================================
    // קוד כיוון השעון (לצריבה חד-פעמית!)
    // ==========================================
    DateTime configTime;
    configTime.second = 0;        // שניות (0 עד 59)
    configTime.minute = 49;       // דקות (0 עד 59)
    configTime.hour   = 19;       // שעה בפורמט 24 שעות (19 פירושו 7 בערב)
    configTime.day    = 27;       // יום בחודש (1 עד 31)
    configTime.month  = 5;        // חודש (1 עד 12, מאי = 5)
    configTime.year   = 2026;     // שנה מלאה (4 ספרות)

    // שליחת הנתונים לשבב ה-DS1302 כדי שישמור אותם בזיכרון שלו
    timeManager.setTime(configTime);
    Serial.println("השעון כוון בהצלחה!");
    // ==========================================
    
    Serial.println("--- מערכת ניהול זמן תרופות הופעלה ---");
    
}

void loop() {
    // 1. בדיקה האם הגיע זמן התרופה המתוזמן (למשל: בשעה 08:00 בבוקר)
    if (timeManager.isScheduledTime(19, 50)) {
        Serial.println("!!! הגיע זמן תרופת הבוקר (08:00) !!!");
        
        // כאן יבוא הקוד של הפעלת המנוע/הסרוו הפיזי שמפיל את התרופה בפועל
        // ...
        
        // לאחר שהתרופה יצאה בהצלחה, מתעדים את הזמן כדי להפעיל את חסימת ה-millis:
        timeManager.recordDispenseTime();
        
        // השהייה ממושכת של דקה וחצי (90 שניות) כדי שהלולאה לא תתקע ותזהה 
        // את השעה 08:00 שוב ושוב באותה הדקה ותוציא מנות כפולות.
        delay(90000); 
    }

    // 2. הדפסת שעון קבועה בכל 10 שניות רק כדי שנוכל לראות ב-Serial שהכל עובד קשורה
    DateTime now = timeManager.getCurrentTime();
    Serial.print("[זמן רץ] ");
    if (now.hour < 10) Serial.print("0");
    Serial.print(now.hour);
    Serial.print(":");
    if (now.minute < 10) Serial.print("0");
    Serial.println(now.minute);

    delay(10000); // ממתין 10 שניות לפני הבדיקה החוזרת בלופ
}
