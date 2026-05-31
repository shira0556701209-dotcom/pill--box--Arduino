// ============================================================
//  pillbox_cam.ino  -  ESP32-CAM
//
//  תפקיד: מצלמה שמסתכלת על הכוס.
//  מתחבר ל-WiFi, מקבל פקודות GET מה-ESP32 הראשי,
//  מספק Stream וידאו חי + צילום לתיעוד.
//
//  פינים: AI Thinker ESP32-CAM (קבועים, אסור לשנות!)
// ============================================================

#include "esp_camera.h"
#include "img_converters.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"   // לשמירת תמונות בזיכרון הפלאש

// --- WiFi  ---
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASS";

// --- פינים של AI Thinker ESP32-CAM ---
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22
#define FLASH_LED_PIN   4  // נורת הפלאש המובנית

WebServer server(80);

// --- גבול ה-Multipart Stream ---
#define PART_BOUNDARY "pillbox_frame"
static const char* STREAM_CONTENT_TYPE =
  "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART =
  "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ============================================================
//  אתחול חומרת המצלמה
// ============================================================
bool initCamera() {
  camera_config_t cfg;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0   = Y2_GPIO_NUM;  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2   = Y4_GPIO_NUM;  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4   = Y6_GPIO_NUM;  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6   = Y8_GPIO_NUM;  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk = XCLK_GPIO_NUM;
  cfg.pin_pclk = PCLK_GPIO_NUM;
  cfg.pin_vsync    = VSYNC_GPIO_NUM;
  cfg.pin_href     = HREF_GPIO_NUM;
  cfg.pin_sccb_sda = SIOD_GPIO_NUM;
  cfg.pin_sccb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn     = PWDN_GPIO_NUM;
  cfg.pin_reset    = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;

  // אם יש PSRAM - איכות גבוהה יותר
  if (psramFound()) {
    cfg.frame_size   = FRAMESIZE_VGA;
    cfg.jpeg_quality = 10;
    cfg.fb_count     = 2;
  } else {
    cfg.frame_size   = FRAMESIZE_QVGA;
    cfg.jpeg_quality = 15;
    cfg.fb_count     = 1;
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  Serial.println("מצלמה אותחלה בהצלחה");
  return true;
}

// ============================================================
//  GET /stream  -  שידור וידאו חי (MJPEG)
//  האפליקציה מציגה את ה-stream הזה באמצעות תג <img>
// ============================================================
void handleStream() {
  // שולחים header של multipart stream
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(STREAM_CONTENT_TYPE);
  client.println("Access-Control-Allow-Origin: *");
  client.println();

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { Serial.println("Frame capture failed"); break; }

    uint8_t* buf  = fb->buf;
    size_t   len  = fb->len;

    // המרה ל-JPEG אם צריך
    uint8_t* jpgBuf = NULL;
    size_t   jpgLen = 0;
    bool converted  = false;
    if (fb->format != PIXFORMAT_JPEG) {
      converted = frame2jpg(fb, 80, &jpgBuf, &jpgLen);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!converted) break;
      buf = jpgBuf; len = jpgLen;
    }

    // שליחת frame
    client.print(STREAM_BOUNDARY);
    char hdr[64];
    snprintf(hdr, sizeof(hdr), STREAM_PART, len);
    client.print(hdr);
    client.write(buf, len);

    if (fb)     esp_camera_fb_return(fb);
    if (jpgBuf) free(jpgBuf);
    delay(33); // ~30 FPS
  }
}

// ============================================================
//  GET /capture  -  צילום תמונה בודדת (לתיעוד נטילה)
//  ה-ESP32 הראשי קורא לזה בכל פעם שיוצאת תרופה
// ============================================================
void handleCapture() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  // הדלק פלאש לרגע
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(100);

  camera_fb_t* fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }

  // שמירה ב-SPIFFS עם שם לפי millis
  if (SPIFFS.begin(true)) {
    String filename = "/dose_" + String(millis()) + ".jpg";
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (file) {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.printf("תמונה נשמרה: %s\n", filename.c_str());
    }
  }

  // מחזירים את התמונה ישירות בתגובה
  server.sendHeader("Content-Type", "image/jpeg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ============================================================
//  GET /snapshot  -  תמונה בודדת (עבור האפליקציה)
// ============================================================
void handleSnapshot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { server.send(500, "text/plain", "Failed"); return; }

  server.sendHeader("Content-Type", "image/jpeg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// ============================================================
//  setup
// ============================================================
void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // ביטול reset בנפילות מתח

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!initCamera()) {
    Serial.println("מצלמה נכשלה - עוצר");
    while (true) delay(1000);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("מתחבר ל-WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.printf("\nESP32-CAM מחובר! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("** רשמי את ה-IP הזה בקובץ pillbox.ino כ-CAM_IP **");

  server.on("/stream",   HTTP_GET, handleStream);
  server.on("/capture",  HTTP_GET, handleCapture);
  server.on("/snapshot", HTTP_GET, handleSnapshot);
  server.begin();
  Serial.println("שרת מצלמה פעיל");
}

void loop() {
  server.handleClient();
  delay(1);
}
