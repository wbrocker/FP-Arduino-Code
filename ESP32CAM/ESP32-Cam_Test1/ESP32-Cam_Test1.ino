// This is for the ESP32-Cam Module

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
// #include <Arduino.h>
#include "base64.h"
#include <ArduinoJson.h>
#include "OTA.h"
#include <TelnetStream.h>




#define CAMERA_MODEL_AI_THINKER

#include "camerapins.h"

const char* ssid="Brocker";
const char* password = "3134R3dd3rsburg";

String serverName = "192.168.1.81";       // Server address for pictures
String serverPath = "/api/upload/";       // Upload URL for pictures
const int cameraId = 1;                   // This is the camera identifier.
const int serverPort = 8000;              // Server Port 

WiFiClient client;

// Pushbutton to test Photo Capture
//const int pushButton = 16;   // GPIO16
const int pirInput = 12;            // PIR using GPIO16 - Also for Wakeup Feature
int pirState = LOW;            // State is used for motion
int val = 0;

String csrfToken = "";                          // Placeholder for Django csrfToken (Not used currentyl)
const char* boundary = "---WebKitBoundary";

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  setupOTA("ESPCAM1", ssid, password);
  
  // Configure the Pushbutton as Input.
  // This will now be used to trigger the camera.
  //pinMode(pushButton, INPUT);
  pinMode(pirInput, INPUT_PULLDOWN);

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
  TelnetStream.begin();
  TelnetStream.println("WiFi Connected!");
  Serial.println("WiFi Connected!");
  TelnetStream.println(WiFi.localIP());
  //Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // delay(5000);

  #ifdef defined(ESP32_RTOS) && defined(ESP32)
  #else
    ArduinoOTA.handle();
  #endif

  // To test this, I will make a pushbutton that can 
  // trigger a Photo
  //int pushButtonState = digitalRead(pushButton);
  motionDetect();       // Check PIR for motion
  delay(100);

  //if (pushButtonState == HIGH) {
  //  Serial.println("Button High!");
  //  TelnetStream.println("Movement Detected!");
    //getCSRFToken();
  //  testHttp();
  //  sendPhoto7();
  //} 
}


// Function to test PIR Sensor.
void motionDetect() {
  val = digitalRead(pirInput);    // Read the Input
  Serial.println(val);
  if (val == HIGH) {              // Confirm if Input is HIGH
    if (pirState == LOW) {
      // Motion was detected now.
      TelnetStream.println("Motion detected!");
      Serial.println("Motion Detected!");
      pirState = HIGH;
    }
  } else {
    Serial.println("State is LOW");
    if (pirState == HIGH) {
      // Motion stopped now
      Serial.println("Motion Ended!");
      TelnetStream.println("Motion Ended!");
      pirState = LOW;
    }
  }
}


void sendPhoto7() {

  const char *server = "192.168.1.81"; // Server URL
if (!client.connect(server, 8000)) {
    TelnetStream.println("Connetion Failed!");
    Serial.println("Connection failed!");
} else
{
    TelnetStream.println("Capturing Photo");
    
    camera_fb_t *fb = esp_camera_fb_get();

    String crlf = "\r\n";

    Serial.println("Connection successful!");
    String bound = "boundry";

    String FinalProj = "--" + bound + "\r\nContent-Disposition: form-data; name=\"X-CSRFToken\"\r\n\r\n" + csrfToken + "\r\n";
    FinalProj.concat("--" + bound + "\r\nContent-Disposition: form-data; name=\"cameraId\"" + "\r\n\r\n" + String(cameraId));

    String head = "--" + bound + "\r\nContent-Disposition: form-data; name=\"image\"; filename=\"image2.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + bound + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);

    // content length
    uint32_t contentLength = FinalProj.length() + totalLen;

    // send post header

    client.println("Content-Length: " + String(contentLength));
    client.println("Content-Type: multipart/form-data; boundary=" + bound);
    client.println();
    char charBufKey[FinalProj.length() + 1];
    FinalProj.toCharArray(charBufKey, FinalProj.length() + 1);
    client.write(charBufKey);
    client.println();
    client.print(head);
    TelnetStream.println("Sending the Photo!");
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
        if (n + 1024 < fbLen)
        {
            client.write(fbBuf, 1024);
            fbBuf += 1024;
        }
        else if (fbLen % 1024 > 0)
        {
            size_t remainder = fbLen % 1024;
            client.write(fbBuf, remainder);
        }
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    client.stop();
    // Serial.println(getBody);
  }
}

// Working bit to take picture!
void sendPhoto6() {

  const char *server = "192.168.1.81"; // Server URL
if (!client.connect(server, 8000))
    Serial.println("Connection failed!");
else
{
    camera_fb_t *fb = esp_camera_fb_get();


    Serial.println("Connection successful!");
    String bound = "boundry";
    String FirstConfigNameProj = "--" + bound + "\r\nContent-Disposition: form-data; name=\"cameraId\"" + "\r\n\r\n" + "1" + "";

    String head = "--" + bound + "\r\nContent-Disposition: form-data; name=\"image\";filename=\"image.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + bound + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);

    // content length
    uint32_t contentLength = FirstConfigNameProj.length() + totalLen;

    // send post header

    client.println("Content-Length: " + String(contentLength));
    client.println("Content-Type: multipart/form-data; boundary=" + bound);
    client.println();
    char charBufKey[FirstConfigNameProj.length() + 1];
    FirstConfigNameProj.toCharArray(charBufKey, FirstConfigNameProj.length() + 1);
    client.write(charBufKey);
    client.println();
    client.print(head);
    Serial.println("second step");
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
        if (n + 1024 < fbLen)
        {
            client.write(fbBuf, 1024);
            fbBuf += 1024;
        }
        else if (fbLen % 1024 > 0)
        {
            size_t remainder = fbLen % 1024;
            client.write(fbBuf, remainder);
        }
    }
    client.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    client.stop();
    // Serial.println(getBody);
}
}




// Function to retrive the CSRF Token
void testHttp() {
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");

  http.begin("http://192.168.1.81:8000/api/upload/");

  Serial.print("[HTTP GET...\n");
  int httpCode = http.GET();

  if(httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      //Serial.println(payload);

      int startIndex = payload.indexOf("value=\"") + 7;
      int endIndex = payload.indexOf("\"", startIndex);
      csrfToken = payload.substring(startIndex, endIndex);
      Serial.print("CSRF Token: ");
      Serial.println(csrfToken);

    }
  } else {
    Serial.println("[HTTP] GET... failed, error:");
    // %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
