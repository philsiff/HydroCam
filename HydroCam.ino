#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <StringArray.h>
#include <SPIFFS.h>
#include <FS.h>

const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";

WiFiClient client;

String serverName = "10.0.0.80";
String serverPath = "/postimg";
const int serverPort = 2023;

// set flash vars
const int flashPin = 4;
const int freq = 5000;
const int ledChannel = 2;
const int reso = 8;

#define FILE_PHOTO "/photo.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Deep Sleep Variables
#define uS_TO_S_FACTOR 1000000LL
#define TIME_TO_SLEEP 5

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  delay(1000);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  } else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
  
  SPIFFS.remove(FILE_PHOTO);

  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  rtc_gpio_hold_dis(GPIO_NUM_4);
  ledcSetup(ledChannel, freq, reso);
  ledcAttachPin(flashPin, ledChannel);

  // Take picture
  Serial.println("Awake, taking picture");
  delay(1000);
  sendPhoto();
  delay(1000);

  esp_sleep_enable_timer_wakeup(uint64_t(28800) * uS_TO_S_FACTOR);

  // Deep Sleep GPIO settings
  pinMode(flashPin, OUTPUT);
  digitalWrite(flashPin, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4);
  gpio_deep_sleep_hold_en();
  
  // Go to sleep
  Serial.println("Going to sleep");
  delay(1000);
  Serial.flush();

  esp_deep_sleep_start();
}

void loop() {

}

void sendPhoto() {
  String get_all;
  String get_body;

  capturePhotoSaveSpiffs();
  File file = SPIFFS.open(FILE_PHOTO);

  Serial.println("Connecting to Server: " + serverName);
  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection Successful");
    String head = "--HydroCam\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--HydroCam--\r\n";

    uint32_t image_len = file.size();
    uint32_t extra_len = head.length() + tail.length();
    uint32_t total_len = image_len + extra_len;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(total_len));
    client.println("Content-Type: multipart/form-data; boundary=HydroCam");
    client.println();
    client.print(head);

    size_t file_len = file.size();
    Serial.println(file_len);
    for (size_t n = 0; n < file_len; n = n + 1024) {
      if (n + 1024 < file_len) {
        byte file_buf[1024];
        file.readBytes((char*)file_buf, 1024);
        client.write(file_buf, 1024);
      } else if (file_len % 1024 > 0) {
        size_t remainder = file_len % 1024;
        byte file_buf[remainder];
        file.readBytes((char*)file_buf, remainder);
        client.write(file_buf, remainder);
      }
    }
    client.print(tail);
    file.close();

    int timeout_timer = 10000;
    long start_timer = millis();
    boolean state = false;

    while ((start_timer + timeout_timer) > millis()) {
      Serial.print(".");
      delay(100);
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (get_all.length() == 0) {
            state = true;
          }
          get_all = "";
        } else if (c != '\r') {
          get_all += String(c);
        }
        if (state == true) {
          get_body += String(c);
        }
        start_timer = millis();
      }
      if (get_body.length() > 0) {
        break;
      }
    }
    Serial.println();
    client.stop();
    Serial.println(get_body);
  } else {
    Serial.println("Connection to " + serverName + " failed.");
  }
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    ledcWrite(ledChannel, 5);
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      ledcWrite(ledChannel, 0);
      return;
    }
    ledcWrite(ledChannel, 0);

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Open Write Failed");
    }
    else {
      file.write(fb->buf, fb->len);
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}
