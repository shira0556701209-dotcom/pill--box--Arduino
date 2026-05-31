// ============================================================
//  pillbox_screen.ino  -  ESP32 מסך TFT
//
//  תפקיד: מציג ממשק גרפי למשתמש.
//  מתחבר ל-WiFi, מאזין לפקודות GET מה-ESP32 הראשי
//  ומעדכן את המסך בהתאם.
//
//  ** לא מחוברים כאן חיישנים - רק מסך ומגע **
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include "tftSensor.h"   // כל הגרפיקה של המסך מכאן

// --- פרטי WiFi ---
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASS";

WebServer server(80);

// --- מצב המסך הנוכחי ---
// בהתאם למצב, ה-loop מצייר את המסך המתאים
enum ScreenMode {
  SCREEN_HOME,       // מסך בית - שעה
  SCREEN_REMINDER,   // תזכורת לנטילה
  SCREEN_CONFIRM,    // אישור / החמצה
  SCREEN_INVENTORY,  // מצב מלאי
  SCREEN_ERROR       // שגיאה
};

ScreenMode currentScreen = SCREEN_HOME;
unsigned long screenTimeout = 0;  // כמה זמן להישאר במסך הנוכחי

// משתנים לפרמטרים שמגיעים מהאפליקציה/מהמכשיר הראשי
char medicineName[32] = "Medicine";
char doseText[32]     = "1 tablet";
char timeText[12]     = "00:00";
bool confirmOK        = true;
bool cellStatus[4]    = {true, true, true, true};

// ============================================================
//  Handlers - מגיבים לפקודות מה-ESP32 הראשי
// ============================================================

// GET /showHome?time=08:30:00  - מסך בית
void handleShowHome() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("time")) {
    strncpy(timeText, server.arg("time").c_str(), sizeof(timeText)-1);
  }
  currentScreen = SCREEN_HOME;
  tft_showHomeScreen(timeText);
  server.send(200, "text/plain", "ok");
}

// GET /showReminder?med=Aspirin&dose=1tab  - מסך תזכורת נטילה
void handleShowReminder() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("med"))  strncpy(medicineName, server.arg("med").c_str(), sizeof(medicineName)-1);
  if (server.hasArg("dose")) strncpy(doseText,     server.arg("dose").c_str(), sizeof(doseText)-1);

  currentScreen  = SCREEN_REMINDER;
  screenTimeout  = millis() + 60000; // נשאר 60 שניות
  tft_showReminderScreen(medicineName, doseText);
  server.send(200, "text/plain", "ok");
}

// GET /showConfirm?ok=1  - מסך אישור (ירוק) / החמצה (אדום)
void handleShowConfirm() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  confirmOK = (!server.hasArg("ok") || server.arg("ok") == "1");

  currentScreen = SCREEN_CONFIRM;
  screenTimeout = millis() + 3000; // 3 שניות ואז חזרה לבית
  tft_showConfirmScreen(confirmOK);
  server.send(200, "text/plain", "ok");
}

// GET /showInventory?c0=1&c1=1&c2=0&c3=1  - מסך מלאי
void handleShowInventory() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  cellStatus[0] = (!server.hasArg("c0") || server.arg("c0") == "1");
  cellStatus[1] = (!server.hasArg("c1") || server.arg("c1") == "1");
  cellStatus[2] = (!server.hasArg("c2") || server.arg("c2") == "1");
  cellStatus[3] = (!server.hasArg("c3") || server.arg("c3") == "1");

  currentScreen = SCREEN_INVENTORY;
  screenTimeout = millis() + 10000; // 10 שניות
  tft_showInventoryScreen(cellStatus);
  server.send(200, "text/plain", "ok");
}

// GET /showError?msg=SomeError  - מסך שגיאה
void handleShowError() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  const char* msg = server.hasArg("msg") ? server.arg("msg").c_str() : "Unknown Error";
  currentScreen = SCREEN_ERROR;
  screenTimeout = millis() + 5000;
  tft_showError(msg);
  server.send(200, "text/plain", "ok");
}

// ============================================================
//  setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // אתחול מסך TFT - rotation=1 = לנדסקייפ
  tft_init(1);
  tft_showHomeScreen("00:00:00");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("מתחבר ל-WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.printf("\nESP32-מסך מחובר! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("** רשמי את ה-IP הזה בקובץ pillbox.ino כ-SCREEN_IP **");

  server.on("/showHome",      HTTP_GET, handleShowHome);
  server.on("/showReminder",  HTTP_GET, handleShowReminder);
  server.on("/showConfirm",   HTTP_GET, handleShowConfirm);
  server.on("/showInventory", HTTP_GET, handleShowInventory);
  server.on("/showError",     HTTP_GET, handleShowError);
  server.begin();
  Serial.println("שרת מסך פעיל");
}

// ============================================================
//  loop
// ============================================================
void loop() {
  server.handleClient();

  // אחרי ה-timeout חוזרים למסך הבית
  if (screenTimeout > 0 && millis() > screenTimeout) {
    screenTimeout = 0;
    currentScreen = SCREEN_HOME;
    tft_showHomeScreen(timeText);
  }

  delay(10);
}
