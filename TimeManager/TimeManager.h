#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

// ============================================================
// כתובות הרגיסטרים (תאי הזיכרון) בתוך שבב ה-DS1302
// ============================================================
#define DS1302_SEC_WRITE   0x80
#define DS1302_SEC_READ    0x81
#define DS1302_MIN_WRITE   0x82
#define DS1302_MIN_READ    0x83
#define DS1302_HOUR_WRITE  0x84
#define DS1302_HOUR_READ   0x85
#define DS1302_DATE_WRITE  0x86
#define DS1302_DATE_READ   0x87
#define DS1302_MON_WRITE   0x88
#define DS1302_MON_READ    0x89
#define DS1302_YEAR_WRITE  0x8C
#define DS1302_YEAR_READ   0x8D
#define DS1302_WP_WRITE    0x8E // מפתח לשינויים

struct DateTime {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

class TimeManager {
private:
    // משתנים פרטיים לשמירת מספרי הפינים שהמשתמש יבחר
    uint8_t clkPin;
    uint8_t datPin;
    uint8_t rstPin;

    unsigned long lastDispenseMillis;

    // פונקציות פנימיות לפרוטוקול
    void writeByte(uint8_t value);
    uint8_t readByte();
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);

    uint8_t bcdToDec(uint8_t bcd);
    uint8_t decToBcd(uint8_t dec);

public:
    // הבנאי מקבל את מספרי הפינים כפרמטרים
    TimeManager(uint8_t clPin, uint8_t daPin, uint8_t rsPin);

    // פונקציית האתחול
    void begin();

    DateTime getCurrentTime();
    void setTime(DateTime dt);
    
    bool isSafetyIntervalPassed(int minWaitMinutes);
    void recordDispenseTime();
    bool isScheduledTime(int scheduledHour, int scheduledMinute);
};

#endif // TIME_MANAGER_H