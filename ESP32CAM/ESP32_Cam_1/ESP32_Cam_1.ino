// This is for the ESP32-Cam 1 Module
// This is the Cam in the Grey Casing 
// with external Antenna

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "base64.h"
#include <ArduinoJson.h>
#include <WebServer.h>
#include "OTA.h"
#include <TelnetStream.h>
#include "config.h"                            // Config.h keeps secret items like
                                                 // my WiFi Credentials.

#define CAMERA_MODEL_AI_THINKER
#define LED_BUILTIN 4                           // Builtin LED on GPIO4

#include "camerapins.h"

WebServer server(80);
StaticJsonDocument<512> jsonDocument;
char buffer[512];


String serverName = "192.168.1.35";             // Server address for pictures
String serverPath = "/api/upload/";             // Upload URL for pictures
const int cameraId = 1;                         // This is the camera identifier.
const int serverPort = 8000;                    // Server Port 

unsigned long lastPicTaken = 0;                 // Variable for when last pic was taken
unsigned long picInterval = 1000;               // Min interval between pictures (in ms)

WiFiClient client;

// Pushbutton to test Photo Capture
const int pirInput = 12;                        // PIR using GPIO12 -
int pirState = LOW;                             // State is used for motion
int val = 0;
bool useLed = true;                             // Using LED
bool camStatus = true;                          // Camera enabled/disabled
const String firmwareVersion = "0.13";             // Firmware Version

String csrfToken = "";                          // Placeholder for Django csrfToken (Not used currently)

// Function Declarations
void setup_routing(void);                       // Routing for API's on Webserver
void create_json(char *tag, float value);       // Creating JSON
void add_json_object(char *tag, float value);   // Adding objects to JSON
void getStatus(void);                           // Get Camera Status variables
void takePic(void);                             // Force a PIC to be taken
void setData(void);                             // POST request to set data on the device


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  setupOTA("ESPCAM1", ssid, password);
  
  // Configure the Pushbutton as Input.
  // This will now be used to trigger the camera.
  pinMode(pirInput, INPUT_PULLDOWN);

  // Configure Builtin LED as Output
  pinMode(LED_BUILTIN, OUTPUT);

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
  // for larger pre-allocated frame buffer.
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

  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 0);

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

  setup_routing();
}

void loop() {

  #ifdef defined(ESP32_RTOS) && defined(ESP32)
  #else
    ArduinoOTA.handle();
  #endif

  motionDetect();       // Check PIR for motion
  delay(100);

  // Handle incoming Web Requests
  server.handleClient();

}


// Function to test PIR Sensor.
void motionDetect() {
  val = digitalRead(pirInput);                // Read the Input
  Serial.println(val);
  if (val == HIGH) {                          // Confirm if Input is HIGH
    if (pirState == LOW) {
      // Motion was detected now.
      TelnetStream.println("Motion detected!");
      Serial.println("Motion Detected!");
      if ((millis() - lastPicTaken >= picInterval) && camStatus) {
        if (useLed) {   // Enable Flash
          // getCSRF();
          digitalWrite(LED_BUILTIN, HIGH);    // Enable the Flash
          delay(200);
          sendPhoto();                        // Call the function to take picture and Send Photo
          digitalWrite(LED_BUILTIN, LOW);     // Disable the Flash
        } else {
          // Take picture without the Flash
          sendPhoto();       
        }

        // Update when last Picture was taken.
        lastPicTaken = millis();
      }
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

// Function to take picture and send photo.
String sendPhoto() {
  String getAll;
  String getBody;
  HTTPClient http;

  String boundary = "FinalProject";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    TelnetStream.println("Capture failed!!!");
    delay(100);
  }

  TelnetStream.println("Connecting to server...");
  String cameraIdString = String(cameraId);

  if (client.connect(serverName.c_str(), serverPort)) {
    TelnetStream.println("Connection successful!");
    String csrf = "csrftoken=";
    csrf += csrfToken;

    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http.addHeader("Cookie", csrf); 
    http.addHeader("X-CSRFToken", csrf);

    
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"image\"; filename=\"esp32-cam1.jpg\"\r\n";
    head += "Content-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--" + boundary + "\r\n";
    tail += "Content-Disposition: form-data; name=\"cameraId\"\r\n\r\n";
    tail += cameraIdString + "\r\n";      //"5\r\n";
    tail += "--" + boundary + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=FinalProject");
    client.println();
    client.print(head);

    uint8_t *fbBuf = fb->buf;
      size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
  while ((startTimer + timoutTimer) > millis()) {
    Serial.print(".");
    delay(100);      
    while (client.available()) {
      char c = client.read();
      if (c == '\n') {
        if (getAll.length()==0) { state=true; }
        getAll = "";
      }
      else if (c != '\r') { getAll += String(c); }
      if (state==true) { getBody += String(c); }
      startTimer = millis();
    }
      if (getBody.length()>0) { break; }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
    else {
      getBody = "Connection to " + serverName +  " failed.";
      Serial.println(getBody);
    }
    return getBody;
}



// Function to retrieve the CSRF Token
void getCSRF() {
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");

  http.begin("http://192.168.1.35:8000/api/upload/");

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


void setup_routing() {
  server.on("/getstatus", getStatus);           // Retrieve device data
  server.on("/takepic", takePic);               // Force pic
  server.on("/setdata", HTTP_POST, setData);    // Set Data
  
  server.begin();
}


void create_json(char *tag, float value) {
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;

  serializeJson(jsonDocument, buffer);
}

void add_json_object(char *tag, float value) {
  JsonObject obj = jsonDocument.createNestedObject();
  // obj["type"] = tag;
  // obj["value"] = value;
  obj[tag] = value;
}


void takePic() {
  TelnetStream.println("Force Picture");
  getCSRF();
  sendPhoto();

  server.send(200, "application/json", "{}");
}

void getStatus() {
  TelnetStream.println("Retrieve all settings");
  jsonDocument.clear();
  jsonDocument["cameraid"] = cameraId;
  jsonDocument["flash"] = useLed;
  jsonDocument["picInterval"] = picInterval;
  jsonDocument["camStatus"] = camStatus;
  jsonDocument["firmware"] = firmwareVersion;

  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

// POST request to set data on the device
void setData() {
  TelnetStream.println("Set Data Function called!");
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  useLed = jsonDocument["flash"];
  picInterval = jsonDocument["picInterval"];
  camStatus = jsonDocument["camStatus"];

  server.send(200, "application/json", "{}");
}
