#include "SmartCam.h"

// יצירת אובייקט המצלמה
SmartCam myCam;

// הגדרות הרשת שלך
// const char* ssid = "my-wifi";
// const char* password = "my- password";


void setup() {
  Serial.begin(115200);
  
  // אתחול המצלמה והחיבור לרשת
  if (myCam.begin(ssid, password)) {
    
    // הפעלת שידור הוידאו
    myCam.startStreamServer();
    
    // הדפסת הכתובת לצפייה
    Serial.print("Camera is ready! Watch live stream at: http://");
    Serial.println(myCam.getIPAddress());
    
    myCam.setFlash(true);
    delay(1000);
    myCam.setFlash(false);
  } else {
    Serial.println("System failed to initialize.");
  }
}

void loop() {
  // הלולאה ריקה - השרת רץ ברקע בצורה אוטומטית!
  // את יכולה להוסיף כאן לוגיקה שמדליקה את הפלאש בשעות הלילה אם צריך.
  delay(1000);
}
