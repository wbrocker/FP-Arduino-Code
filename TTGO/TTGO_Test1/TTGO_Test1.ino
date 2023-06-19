#include "esp_camera.h"
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


#define CAMERA_MODEL_TTGO

#include "camerapins.h"

const char* ssid="Brocker";
const char* password = "3134R3dd3rsburg";

String serverName = "192.168.1.81";
String serverPath = "/api/upload/";
const int cameraId = 3;                 // This is the camera identifier.

const int serverPort = 8000;

WiFiClient client;

const int pushButton = 34;
boolean takePicture = false;    // Test Flag to trigger picture

int pirSensor = 33;
int state = LOW;
int val = 0;


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Disable Brownout
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  
  // Configure the Pushbutton as Input.
  // This will now be used to trigger the camera.
  pinMode(pushButton, INPUT);

  // PIR
  pinMode(pirSensor, INPUT);   // Init the PIR Sensor
  

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

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera Init
  esp_err_t err = esp_camera_init(&config);

  // Start Wifi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected!");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  // delay(5000);
  int pushButtonState = digitalRead(pushButton);

  if (pushButtonState == LOW ) {
    Serial.println("Button LOW!");
    takePicture = true;
  }

  if (takePicture) {
    sendPhoto();
    takePicture = false;
  }

  val = digitalRead(pirSensor);
  if (val == HIGH) { 
    delay(100);
    if (state = LOW) {
      
      Serial.println("Motion Detected!");
      sendPhoto();
      state = HIGH;
    }
  } else {
    delay(200);

    if (state == HIGH) {
      Serial.println("Motion Stopped.");
      state = LOW;
    }
  }
}

String sendPhoto() {

  const char *server = "192.168.1.81";
  if (!client.connect(server, 8000)) {
    Serial.println("Connection Failed!");
  } else {
    camera_fb_t *fb = esp_camera_fb_get();

    Serial.println("Connection Successful");
    String boundary = "boundary";
    String FPProj = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"cameraId\"" + "\r\n\r\n" + String(cameraId);

    String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"image\";filename=\"image" + String(cameraId) + ".jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLength = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);

    // Content Length
    uint32_t contentLength = FPProj.length() + totalLength;

    // Sending the POST Header
    client.println("Content-Length: " + String(contentLength));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println();
    char charBufferKey[FPProj.length() + 1];
    FPProj.toCharArray(charBufferKey, FPProj.length() + 1);
    client.write(charBufferKey);
    client.println();
    client.print(head);
    
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    // int timeout = 10000;
    // long startTimer = millis();
    // boolean state = false;

    // while ((startTimer + timeout) > millis()) {
    //   Serial.print(".");
    //   delay(100);
    //   while (client.available()) {
    //     char c = client.read();
    //     if (c == '\n') {
    //       if (getAll.length() == 0)
    //     }
    //   }
    // }

    client.stop();
    // Serial.println(getBody);
  }

}

String sendPhoto_old() {
  String getAll;
  String getBody;

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Capture Failed!");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connecting to Server: " + serverName);

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connected");
    String head = "Content-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-com.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--FinalProject--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data;");
    client.println();
    client.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    int timeoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timeoutTimer) > millis()) {
      Serial.print(".");
      delay(100);
      while (client.available()) { 
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state == true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state == true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length() > 0) { break; }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to " + serverName + " failed.";
    Serial.println(getBody);
  }
  return getBody;

}
