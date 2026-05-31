#include "TimeManager.h"

// -------------------------------------------------------
// בנאי
// -------------------------------------------------------
TimeManager::TimeManager(uint8_t clPin, uint8_t daPin, uint8_t rsPin) {
    clkPin = clPin;
    datPin = daPin;
    rstPin = rsPin;
    lastDispenseMillis = 0;
}

// -------------------------------------------------------
// begin - אתחול 
// -------------------------------------------------------
void TimeManager::begin() {
    pinMode(clkPin, OUTPUT);
    pinMode(rstPin, OUTPUT);
    
    digitalWrite(clkPin, LOW);
    digitalWrite(rstPin, LOW);
    
    Serial.println("[TimeManager] הפינים אותחלו בהצלחה בתוך begin()");
}

// -------------------------------------------------------
// המרות BCD ידניות
// -------------------------------------------------------
uint8_t TimeManager::bcdToDec(uint8_t bcd) {
    return (bcd / 16 * 10) + (bcd % 16);
}

uint8_t TimeManager::decToBcd(uint8_t dec) {
    return (dec / 10 * 16) + (dec % 10);
}

// -------------------------------------------------------
// writeByte - דחיפת 8 ביטים לקו הנתונים
// -------------------------------------------------------
void TimeManager::writeByte(uint8_t value) {
    pinMode(datPin, OUTPUT); 
    
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(datPin, (value & 0x01) ? HIGH : LOW);
        
        digitalWrite(clkPin, HIGH);
        delayMicroseconds(1); 
        digitalWrite(clkPin, LOW);
        delayMicroseconds(1);
        
        value >>= 1; 
    }
}

// -------------------------------------------------------
// readByte - דגימת 8 ביטים מקו הנתונים
// -------------------------------------------------------
uint8_t TimeManager::readByte() {
    pinMode(datPin, INPUT); 
    uint8_t value = 0;
    
    for (uint8_t i = 0; i < 8; i++) {
        if (digitalRead(datPin) == HIGH) {
            value |= (1 << i);
        }
        
        digitalWrite(clkPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(clkPin, LOW);
        delayMicroseconds(1);
    }
    return value;
}

// -------------------------------------------------------
// writeRegister - ביצוע כתיבה לרגיסטר
// -------------------------------------------------------
void TimeManager::writeRegister(uint8_t reg, uint8_t value) {
    digitalWrite(rstPin, HIGH); 
    delayMicroseconds(4);
    
    writeByte(reg);   
    writeByte(value); 
    
    digitalWrite(rstPin, LOW); 
    delayMicroseconds(4);
}

// -------------------------------------------------------
// readRegister - ביצוע קריאה מרגיסטר
// -------------------------------------------------------
uint8_t TimeManager::readRegister(uint8_t reg) {
    digitalWrite(rstPin, HIGH); 
    delayMicroseconds(4);
    
    writeByte(reg);          
    uint8_t value = readByte(); 
    
    digitalWrite(rstPin, LOW);  
    delayMicroseconds(4);
    
    return value;
}

// -------------------------------------------------------
// getCurrentTime - קריאת הזמן
// -------------------------------------------------------
DateTime TimeManager::getCurrentTime() {
    DateTime dt;
    
    dt.second = bcdToDec(readRegister(DS1302_SEC_READ)  & 0x7F);
    dt.minute = bcdToDec(readRegister(DS1302_MIN_READ)  & 0x7F);
    dt.hour   = bcdToDec(readRegister(DS1302_HOUR_READ) & 0x3F); 
    dt.day    = bcdToDec(readRegister(DS1302_DATE_READ) & 0x3F);
    dt.month  = bcdToDec(readRegister(DS1302_MON_READ)  & 0x1F);
    dt.year   = bcdToDec(readRegister(DS1302_YEAR_READ)) + 2000;
    
    return dt;
}

// -------------------------------------------------------
// setTime - כיוון השעון
// -------------------------------------------------------
void TimeManager::setTime(DateTime dt) {
    writeRegister(DS1302_WP_WRITE, 0x00); // "פתיחת ה- "מפתח
    
    writeRegister(DS1302_SEC_WRITE,  decToBcd(dt.second));
    writeRegister(DS1302_MIN_WRITE,  decToBcd(dt.minute));
    writeRegister(DS1302_HOUR_WRITE, decToBcd(dt.hour));
    writeRegister(DS1302_DATE_WRITE, decToBcd(dt.day));
    writeRegister(DS1302_MON_WRITE,  decToBcd(dt.month));
    writeRegister(DS1302_YEAR_WRITE, decToBcd(dt.year % 100)); 
    
    writeRegister(DS1302_WP_WRITE, 0x80);
    
    Serial.println("[TimeManager] הזמן הוגדר בהצלחה");
}

// -------------------------------------------------------
// פונקציות לוגיות
// -------------------------------------------------------
// האם עבר מספיק זמן מאז שהמשכך האחרון נלקח
bool TimeManager::isSafetyIntervalPassed(int minWaitMinutes) {
    if (lastDispenseMillis == 0) return true;  
    unsigned long waitMs  = (unsigned long)minWaitMinutes * 60UL * 1000UL;
    unsigned long elapsed = millis() - lastDispenseMillis;

    if (elapsed >= waitMs) return true;

    int minutesLeft = (waitMs - elapsed) / 60000;
    Serial.print("[TimeManager] חסימה! יש להמתין עוד ");
    Serial.print(minutesLeft);
    Serial.println(" דקות");
    return false; 
}
// תיעוד לקיחת משכך
void TimeManager::recordDispenseTime() {
    lastDispenseMillis = millis();
    Serial.println("[TimeManager] זמן הנפקה נשמר");
}
/**
*האם הגיע הזמן לקחת תרופה 
*/
bool TimeManager::isScheduledTime(int scheduledHour, int scheduledMinute) {
    DateTime now = getCurrentTime();
    int nowTotal   = now.hour * 60 + now.minute;
    int schedTotal = scheduledHour * 60 + scheduledMinute;
    return abs(nowTotal - schedTotal) <= 1;
}