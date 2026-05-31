#ifndef SMART_CAM_H
#define SMART_CAM_H

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

class SmartCam {
  public:
    // בנאי - מאתחל את האובייקט
    SmartCam();

    // פונקציית האתחול הראשית - מתחברת ל-WiFi ומדליקה מצלמה
    bool begin(const char* ssid, const char* password);

    // מתחילה את שידור הוידאו לרשת
    void startStreamServer();

    // מחזירה את כתובת ה-IP של המצלמה כטקסט
    String getIPAddress();

    // --- תוספות מיוחדות לפרויקט הקופסה החכמה ---
    // שליטה בפלאש כדי לאפשר זיהוי פנים בחושך!
    void setFlash(bool state);

  private:
    // הפעלת החומרה של המצלמה (מוסתר מהמשתמש)
    bool initCameraHardware();
    
    // משתנה פנימי לשמירת השרת
    httpd_handle_t _stream_httpd;
};

#endif // SMART_CAM_H
