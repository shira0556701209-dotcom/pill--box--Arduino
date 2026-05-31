#include <Dispenser.h>

// ============================================================
//  pillbox.ino  –  ESP32 ראשי  (כל החיישנים)
//
//  ארכיטקטורה:
//    ESP32 ראשי  קובץ זה  (buzzer, servo×2, מרחק, לייזר×4, LED RGB, משקל, RTC)
//    ESP32-CAM מחובר בנפרד, מקבל פקודות דרך HTTP GET
//    ESP32 מחובר בנפרד, מקבל פקודות דרך HTTP GET
//    אפליקציית React תקשורת עם ה-ESP32 הראשי דרך HTTP
// ============================================================

// --- ספריות ---
#include "BuzzerSensor.h"
#include "Dispenser.h"
#include "distansSensor.h"
#include "laserSensor.h"
#include "ledSensor.h"
#include "WeghitSensor.h"
#include "TimeManager.h"

#include <WiFi.h>
#include <WebServer.h>    // שרת HTTP שמאזין לאפליקציה
#include <HTTPClient.h>   // לקוח HTTP לשלוח פקודות למצלמה ולמסך
#include <ArduinoJson.h>  // לבניית תגובות JSON לאפליקציה

// --- פרטי רשת WiFi ---
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASS";

// --- כתובות IP של הרכיבים האחרים ---
const char* CAM_IP = "192.168.1.101";
const char* SCREEN_IP = "192.168.1.102";

// --- הגדרת פינים ---
#define BUZZER_PIN 4        // GPIO של הזמזם (Buzzer)
#define MAIN_SERVO_PIN 13   // GPIO של הסרוו שמסובב את הדיסק (בחירת תא)
#define OPEN_SERVO_PIN 15   // GPIO של הסרוו שפותח את שער ההוצאה
#define DIST_TRIG_PIN 5     // GPIO של Trigger לחיישן מרחק אולטרסוני
#define DIST_ECHO_PIN 18    // GPIO של Echo לחיישן מרחק
#define LASER_0_PIN 34      // GPIO של מקלט לייזר תא 0
#define LASER_1_PIN 35      // GPIO של מקלט לייזר תא 1
#define LASER_2_PIN 36      // GPIO של מקלט לייזר תא 2
#define LASER_3_PIN 39      // GPIO של מקלט לייזר תא 3
#define LED_RED_PIN 27      // GPIO אדום של LED RGB
#define LED_GREEN_PIN 14    // GPIO ירוק של LED RGB
#define LED_BLUE_PIN 12     // GPIO כחול של LED RGB
#define WEIGHT_DOUT_PIN 32  // GPIO DOUT של מד-משקל HX711
#define WEIGHT_SCK_PIN 33   // GPIO SCK של מד-משקל HX711
#define RTC_CLK_PIN 22      // GPIO CLK של שבב שעון RTC DS1302
#define RTC_DAT_PIN 21      // GPIO DAT של שבב שעון RTC DS1302
#define RTC_RST_PIN 19      // GPIO RST של שבב שעון RTC DS1302

// --- קבועי לוגיקה ---
#define NUM_CELLS 4                    // מספר תאי התרופות בקופסה
#define DOSE_CONFIRM_TIMEOUT 600000UL  // 10 שניות למשתמש לאשר נטילה
#define PAIN_KILLER_WAIT_MIN 360       // דקות המינימום בין משכך כאבים (6 שעות)
#define LOW_STOCK_THRESHOLD 2          // פחות מ-X תאים מלאים = מלאי נמוך

// --- לוח זמנים לתרופות ---
// כל שורה: {שעה, דקות, מספר תא (0-3), שם תרופה}
struct ScheduledDose {
  int hour;
  int minute;
  int cellID;
  const char* medicineName;
};

ScheduledDose schedule[] = {
  { 8, 0, 0, "Aspirin" },
  { 14, 0, 1, "Vitamin D" },
  { 20, 0, 2, "Omega 3" },
  { 22, 30, 3, "Melatonin" },
};
const int SCHEDULE_COUNT = sizeof(schedule) / sizeof(schedule[0]);

// --- מצב המערכת ---
struct SystemState {
  bool cellFull[NUM_CELLS];   // האם כל תא מלא
  bool waitingForTake;        // האם הקופסה מחכה שהמשתמש ייקח את התרופה
  int lastDispensedCell;      // איזה תא הוצא לאחרונה
  bool medicineConfirmed;     // האם נטילת התרופה אושרה
  int scheduledHour;          // שעת המינון הבא
  int scheduledMinute;        // דקות המינון הבא
  char lastMedicineName[32];  // שם התרופה שהוצאה לאחרונה
  float cupWeightGrams;       // משקל הכוס הנוכחי
  bool painKillerAllowed;     // האם מותר לקחת משכך כאבים
  char currentTime[12];       // שעה נוכחית כטקסט "HH:MM:SS"
  bool waitingForAuth;        // האם אנחנו מחכים לזיהוי פנים
  unsigned long authStart;    // מתי התחלנו לחכות לזיהוי
  int pendingCellID;          // איזה תא מיועד לצאת אחרי הזיהוי
  int pendingScheduleIndex;   
  char pendingMedName[32];    // שם התרופה שמיועדת לצאת
} state;

// --- אובייקטים ---
BuzzerSensor buzzer(BUZZER_PIN);
Dispenser mainDispenser(MAIN_SERVO_PIN);
Dispenser openDispenser(OPEN_SERVO_PIN);
DistansSensor dist(DIST_TRIG_PIN, DIST_ECHO_PIN);
int laserPins[NUM_CELLS] = { LASER_0_PIN, LASER_1_PIN, LASER_2_PIN, LASER_3_PIN };
laserSensor laser(laserPins);
ledSensor led(LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN);
WeghitSensor weight(WEIGHT_DOUT_PIN, WEIGHT_SCK_PIN);
TimeManager rtc(RTC_CLK_PIN, RTC_DAT_PIN, RTC_RST_PIN);
WebServer server(80);

// --- משתנים פנימיים ---
unsigned long lastScheduleCheck = 0;
unsigned long doseWaitStart = 0;
bool dispensedThisCycle = false;

// ============================================================
//  פונקציות עזר - תקשורת עם ESP32-CAM ו-ESP32-מסך
// ============================================================

// שולח פקודת GET לרכיב אחר ברשת
void sendGetRequest(const char* url) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code > 0) {
    Serial.printf("[HTTP] GET %s -> %d\n", url, code);
  } else {
    Serial.printf("[HTTP] שגיאה: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

// פקודה למסך: הצג מסך תזכורת
void screenShowReminder(const char* medName, const char* dose) {
  char url[200];
  snprintf(url, sizeof(url),
           "http://%s/showReminder?med=%s&dose=%s", SCREEN_IP, medName, dose);
  sendGetRequest(url);
}

// פקודה למסך: הצג מסך אישור נטילה
void screenShowConfirm(bool ok) {
  char url[100];
  snprintf(url, sizeof(url), "http://%s/showConfirm?ok=%d", SCREEN_IP, ok ? 1 : 0);
  sendGetRequest(url);
}

// פקודה למסך: הצג מלאי
void screenShowInventory() {
  char url[200];
  snprintf(url, sizeof(url),
           "http://%s/showInventory?c0=%d&c1=%d&c2=%d&c3=%d",
           SCREEN_IP,
           state.cellFull[0] ? 1 : 0, state.cellFull[1] ? 1 : 0,
           state.cellFull[2] ? 1 : 0, state.cellFull[3] ? 1 : 0);
  sendGetRequest(url);
}
//פקודה למצלימה : זיהוי פנים
bool verifyUserFace() {
  char url[100];
  snprintf(url, sizeof(url), "http://%s/recognize", CAM_IP);

  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  bool isVerified = false;

  if (code == 200) {
    String response = http.getString();
    StaticJsonDocument<200> doc;
    deserializeJson(doc, response);

    // נניח שהמצלמה מחזירה: {"status":"success", "user":"Shira"}
    if (doc["status"] == "success" && doc["user"] == "Shira") {
      isVerified = true;
    }
  }
  http.end();
  return isVerified;
}

// פקודה למצלמה: צלם תמונה ושמור לתיעוד
void camCapture() {
  char url[100];
  snprintf(url, sizeof(url), "http://%s/capture", CAM_IP);
  sendGetRequest(url);
}

// ============================================================
// הוצאת תרופה
// ============================================================

// מוציא תרופה מתא מסוים ומתחיל להמתין לאישור נטילה
void dispenseMedicine(int cellID, const char* medName) {
  Serial.printf("[Dispenser] מוציא תרופה מתא %d - %s\n", cellID, medName);

  state.lastDispensedCell = cellID;
  state.waitingForTake = true;
  state.medicineConfirmed = false;
  strncpy(state.lastMedicineName, medName, sizeof(state.lastMedicineName) - 1);
  doseWaitStart = millis();

  // סרוו ראשי מסובב לתא הנכון ומוציא כדור
  mainDispenser.releasePill(cellID);

  //פתיחת התחתית ליציאת הכדור
  openDispenser.setAngle(90);

  // LED אדום להתראה
  led.activateLED(ALERT_MEDICATION_TIME);

  //  זמזם
  buzzer.activateBuzzer(ALERT_MEDICATION_TIME);

  // עדכון מסך TFT
  screenShowReminder(medName, "1 tablet");

  //צילום לתיעוד
  camCapture();

  dispensedThisCycle = true;
}

// בדיקה האם הגיע זמן מינון לפי לוח הזמנים
void checkSchedule() {
  if (millis() - lastScheduleCheck < 60000) return;  // בדיקה פעם בדקה
  lastScheduleCheck = millis();

  // אם מחכים שהמשתמש ייקח תרופה או יזדהה, אל תבדוק שוב
  if (state.waitingForTake || state.waitingForAuth) return; 

  DateTime now = rtc.getCurrentTime();
  snprintf(state.currentTime, sizeof(state.currentTime),
           "%02d:%02d:%02d", now.hour, now.minute, now.second);

  for (int i = 0; i < SCHEDULE_COUNT; i++) {
    if (now.hour == schedule[i].hour && now.minute == schedule[i].minute && !dispensedThisCycle) {
      if (state.cellFull[schedule[i].cellID]) {

        // -- מתחילים תהליך זיהוי --
        state.waitingForAuth = true;
        state.authStart = millis();
        state.pendingCellID = schedule[i].cellID;
        state.pendingScheduleIndex = i; // שומרים את האינדקס
        strncpy(state.pendingMedName, schedule[i].medicineName, sizeof(state.pendingMedName) - 1);

        // עדכון המסך וצפצוף למשוך תשומת לב
        screenShowReminder(schedule[i].medicineName, "Look at Camera!");
        buzzer.beep(1000, 500); 
        dispensedThisCycle = true;
        
      } else {
        // התא ריק - התראת מלאי נמוך
        buzzer.activateBuzzer(ALERT_STOCK_LOW);
        led.activateLED(ALERT_STOCK_LOW);
        Serial.printf("[Schedule] תא %d ריק! לא ניתן להוציא %s\n",
                      schedule[i].cellID, schedule[i].medicineName);
      }
    }
  }
}
void checkAuth() {
  // אם לא מחכים להזדהות, יוצאים מיד
  if (!state.waitingForAuth) return;

  // בדיקת זיהוי כל שתי שניות כדי לא להציף את המצלמה
  static unsigned long lastAuthCheck = 0;
  if (millis() - lastAuthCheck > 2000) {
    lastAuthCheck = millis();
    
    Serial.println("[Auth] בודק פנים מול המצלמה...");
    if (verifyUserFace()) {
      Serial.println("[Auth] זיהוי בהצלחה! מוציא תרופה...");
      state.waitingForAuth = false; 
      
      // הוצאת התרופה
      dispenseMedicine(state.pendingCellID, state.pendingMedName);
      
      // בודקים מאיפה הגיעה הבקשה כדי לדעת מה לעדכן
      if (state.pendingScheduleIndex != -1) {
        // זו תרופה מהלו"ז הרגיל - מעדכנים את שעת המנה הבאה
        int next = (state.pendingScheduleIndex + 1) % SCHEDULE_COUNT;
        state.scheduledHour = schedule[next].hour;
        state.scheduledMinute = schedule[next].minute;
      } else {
        // זה משכך כאבים! נרשום בשעון את שעת ההוצאה כדי לנעול ל-6 שעות
        rtc.recordDispenseTime();
      }
      
      return; 
    }
  }
  
  // Timeout: אם עברו 10 דקות ואף אחד לא הזדהה מול הקופסה
  if (millis() - state.authStart > 600000UL) {
    Serial.println("[Auth] כשל בזיהוי: עבר הזמן.");
    state.waitingForAuth = false;
    screenShowConfirm(false); 
    buzzer.activateBuzzer(ALERT_MISSED_DOSE);
  }
}
// בדיקת אישור נטילה - האם המשתמש לקח את התרופה מהכוס
void checkMedicineTaken() {
  if (!state.waitingForTake) return;

  if (dist.isMedicineTaken()) {
    // המשתמש הכניס יד לכוס ויצא - אישור נטילה
    dist.resetTaken();
    state.waitingForTake = false;
    state.medicineConfirmed = true;

    led.setLightStatus(COLOR_GREEN);
    buzzer.beep(1500, 200);
    screenShowConfirm(true);
    // state.cellFull[state.lastDispensedCell] = false;

    Serial.printf("[System] נטילה אושרה: %s\n", state.lastMedicineName);
  }

  // timeout - עבר יותר מ-10 דקות, לא נלקחה תרופה
  if (millis() - doseWaitStart > DOSE_CONFIRM_TIMEOUT) {
    state.waitingForTake = false;
    buzzer.activateBuzzer(ALERT_MISSED_DOSE);
    led.blinkLED(5);
    screenShowConfirm(false);
    Serial.println("[System] החמצת מנה!");
  }
}

// עדכון מצב מלאי מחיישני הלייזר
void updateInventory() {
  int results[NUM_CELLS];
  laser.readAll(results);
  int emptyCells = 0;
  for (int i = 0; i < NUM_CELLS; i++) {
    state.cellFull[i] = (results[i] == CELL_OCCUPIED);
    if (!state.cellFull[i]) emptyCells++;
  }
  if (emptyCells >= NUM_CELLS - LOW_STOCK_THRESHOLD) {
    Serial.printf("[Inventory] אזהרה: %d תאים ריקים!\n", emptyCells);
  }
}

// ============================================================
//  Endpoints של שרת HTTP
//  האפליקציה מתקשרת עם ה-ESP32 דרך נקודות אלה
// ============================================================

void setCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// GET /status  - מחזיר JSON מצב המערכת
void handleStatus() {
  setCorsHeaders();
  state.cupWeightGrams = weight.getWeightGrams();
  updateInventory();

  StaticJsonDocument<512> doc;
  doc["time"] = state.currentTime;
  doc["waitingForTake"] = state.waitingForTake;
  doc["lastMedicine"] = state.lastMedicineName;
  doc["medicineConfirmed"] = state.medicineConfirmed;
  doc["nextDoseHour"] = state.scheduledHour;
  doc["nextDoseMinute"] = state.scheduledMinute;
  doc["cupWeight"] = state.cupWeightGrams;
  doc["painKillerAllowed"] = state.painKillerAllowed;

  JsonArray cells = doc.createNestedArray("cells");
  for (int i = 0; i < NUM_CELLS; i++) cells.add(state.cellFull[i]);

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// GET /dispense?cell=0  - הוצאת תרופה מהאפליקציה
void handleDispense() {
  setCorsHeaders();
  if (!server.hasArg("cell")) {
    server.send(400, "application/json", "{\"error\":\"missing cell\"}");
    return;
  }
  int cell = server.arg("cell").toInt();
  if (cell < 0 || cell >= NUM_CELLS) {
    server.send(400, "application/json", "{\"error\":\"invalid cell\"}");
    return;
  }
  if (!state.cellFull[cell]) {
    server.send(409, "application/json", "{\"error\":\"cell is empty\"}");
    return;
  }
  dispenseMedicine(cell, schedule[cell].medicineName);
  server.send(200, "application/json", "{\"status\":\"dispensed\"}");
}

// GET /painkiller  - בקשת משכך כאבים עם בדיקת מרווח זמן וזיהוי פנים
void handlePainKiller() {
  setCorsHeaders();
  
  // הגנה: אם המערכת כבר באמצע תהליך (מחכה לזיהוי או מחכה שהכדור יילקח)
  if (state.waitingForTake || state.waitingForAuth) {
    server.send(409, "application/json", "{\"error\":\"system busy\"}");
    return;
  }

  bool allowed = rtc.isSafetyIntervalPassed(PAIN_KILLER_WAIT_MIN);
  state.painKillerAllowed = allowed;

  if (allowed) {
    int numCellPK = -1;
    for (int i = 0; i < NUM_CELLS; i++) {
      // חיפוש התא של משכך הכאבים
      if (String(schedule[i].medicineName) == "Pain Killer") {
        numCellPK = i;
      }
    }
    
    if (numCellPK == -1) {
      server.send(400, "application/json", "{\"error\":\"out of stock\"}");
    } else if (!state.cellFull[numCellPK]) {
      server.send(409, "application/json", "{\"error\":\"cell is empty\"}");
    } else {
      // -- תהליך זיהוי --
      state.waitingForAuth = true;
      state.authStart = millis();
      state.pendingCellID = numCellPK;
      state.pendingScheduleIndex = -1; // ה-1- מסמן שזו תרופה יזומה ולא מהלו"ז
      strncpy(state.pendingMedName, "Pain Killer", sizeof(state.pendingMedName) - 1);

      // התראות למשתמש
      buzzer.activateBuzzer(ALERT_PAIN_REQUEST);
      led.activateLED(ALERT_PAIN_REQUEST);
      screenShowReminder("Pain Killer", "Look at Camera!");

      // עונים לאפליקציה שהבקשה התקבלה ועכשיו מחכים לזיהוי
      server.send(200, "application/json", "{\"status\":\"waiting_for_auth\"}");
    }
  } else {
    server.send(429, "application/json", "{\"error\":\"too soon\",\"waitMinutes\":360}");
  }
}

// GET /inventory  - מצב כל 4 התאים
void handleInventory() {
  setCorsHeaders();
  updateInventory();
  screenShowInventory();

  StaticJsonDocument<300> doc;
  JsonArray cells = doc.createNestedArray("cells");
  for (int i = 0; i < NUM_CELLS; i++) {
    JsonObject cell = cells.createNestedObject();
    cell["id"] = i;
    cell["full"] = state.cellFull[i];
    cell["name"] = schedule[i].medicineName;
    cell["hour"] = schedule[i].hour;
    cell["min"] = schedule[i].minute;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// GET /setschedule?cell=0&hour=8&minute=30  - עדכון לוח זמנים
void handleSetSchedule() {
  setCorsHeaders();
  if (!server.hasArg("cell") || !server.hasArg("hour") || !server.hasArg("minute")) {
    server.send(400, "application/json", "{\"error\":\"missing params\"}");
    return;
  }
  int cell = server.arg("cell").toInt();
  int hour = server.arg("hour").toInt();
  int minute = server.arg("minute").toInt();
  if (cell < 0 || cell >= SCHEDULE_COUNT) {
    server.send(400, "application/json", "{\"error\":\"invalid cell\"}");
    return;
  }
  schedule[cell].hour = hour;
  schedule[cell].minute = minute;
  Serial.printf("[Schedule] עודכן: תא %d -> %02d:%02d\n", cell, hour, minute);
  server.send(200, "application/json", "{\"status\":\"updated\"}");
}

// OPTIONS - CORS Preflight
void handleOptions() {
  setCorsHeaders();
  server.send(204);
}

// ============================================================
//  setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== Smart Pill Box - ESP32 ===");

  // אתחול חיישנים
  buzzer.begin();
  mainDispenser.begin();
  openDispenser.begin();
  dist.begin();
  laser.begin();
  led.begin();
  weight.begin();
  rtc.begin();

  // כיוון שעון -פעם אחת  (comment):
  // DateTime t; t.second=0; t.minute=0; t.hour=8; t.day=28; t.month=5; t.year=2026;
  // rtc.setTime(t);

  // חיבור WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("מתחבר ל-WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi מחובר! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("** רשמי את ה-IP הזה באפליקציה React **");
  } else {
    Serial.println("\nWiFi נכשל - ממשיך ללא רשת");
  }

  // רישום Endpoints
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/dispense", HTTP_GET, handleDispense);
  server.on("/painkiller", HTTP_GET, handlePainKiller);
  server.on("/inventory", HTTP_GET, handleInventory);
  server.on("/setschedule", HTTP_GET, handleSetSchedule);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/dispense", HTTP_OPTIONS, handleOptions);
  server.on("/painkiller", HTTP_OPTIONS, handleOptions);
  server.on("/inventory", HTTP_OPTIONS, handleOptions);
  server.on("/setschedule", HTTP_OPTIONS, handleOptions);
  server.onNotFound([]() {
    setCorsHeaders();
    server.send(404, "application/json", "{\"error\":\"not found\"}");
  });

  server.begin();
  Serial.println("שרת HTTP פעיל על פורט 80");

  // אתחול מצב
  memset(&state, 0, sizeof(state));
  state.scheduledHour = schedule[0].hour;
  state.scheduledMinute = schedule[0].minute;
  strncpy(state.lastMedicineName, "None", sizeof(state.lastMedicineName) - 1);

  // LED ירוק - מוכן
  led.setLightStatus(COLOR_GREEN);
  delay(1000);
  led.turnOffAll();

  Serial.println("מערכת מוכנה!");
}

// ============================================================
//  loop
// ============================================================
void loop() {
  server.handleClient();  // טיפול בבקשות HTTP מהאפליקציה

  dist.distansUpdate();       // עדכון חיישן מרחק (state machine פנימי)
  laser.laserSensorUpdate();  // עדכון מצב לייזרים

  // עדכון שעה (כל שנייה)
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 1000) {
    lastTimeUpdate = millis();
    DateTime now = rtc.getCurrentTime();
    snprintf(state.currentTime, sizeof(state.currentTime),
             "%02d:%02d:%02d", now.hour, now.minute, now.second);
  }

  checkSchedule();       // בדיקת לוח זמנים
  checkAuth();            // האם המשתמש זוהה
  checkMedicineTaken();  // בדיקת נטילה
  
  // עדכון מלאי (כל שעה)
  static unsigned long lastInvCheck = 0;
  if (millis() - lastInvCheck > 1000 * 60 * 60) {
    lastInvCheck = millis();
    updateInventory();
  }

  delay(10);
}
