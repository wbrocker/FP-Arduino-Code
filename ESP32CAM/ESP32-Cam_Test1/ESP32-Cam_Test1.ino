// This is for the ESP32-Cam Module

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
// #include <Arduino.h>
#include "base64.h"
#include <ArduinoJson.h>



#define CAMERA_MODEL_AI_THINKER

#include "camerapins.h"

const char* ssid="Brocker";
const char* password = "3134R3dd3rsburg";

String serverName = "192.168.1.81";
String serverPath = "/api/upload/";

const int serverPort = 8000;

WiFiClient client;

// Pushbutton to test Photo Capture
const int pushButton = 16;   // GPIO16

String csrfToken = "";
const char* boundary = "---WebKitBoundary";

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
  // Configure the Pushbutton as Input.
  // This will now be used to trigger the camera.
  pinMode(pushButton, INPUT);

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
  // delay(5000);

  // To test this, I will make a pushbutton that can 
  // trigger a Photo
  int pushButtonState = digitalRead(pushButton);

  if (pushButtonState == HIGH) {
    Serial.println("Button High!");
    //getCSRFToken();
    testHttp();
    sendPhoto7();
  } 
}

void sendPhoto7() {

  const char *server = "192.168.1.81"; // Server URL
if (!client.connect(server, 8000))
    Serial.println("Connection failed!");
else
{
    camera_fb_t *fb = esp_camera_fb_get();


    Serial.println("Connection successful!");
    String bound = "boundry";
    String FirstConfigNameProj = "--" + bound + "\r\nContent-Disposition: form-data; name=\"csrfmiddlewaretoken\"\r\n\r\n" + csrfToken + "\r\n"; //; name=\"cameraId\"" + "\r\n\r\n" + "1" + "";

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
    // client.println("Content-Type: multipart/form-data; name=\"csrfmiddlewaretoken\"\r\n\r\n");
    // client.println(String(csrfToken) + "\r\nboundary=" + bound);
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



void sendPhoto5() {
  camera_fb_t *fb = esp_camera_fb_get();

  if (fb) {
    Serial.println("Captured Image");
    uint8_t* image_data = fb->buf;
    int image_size = fb->len;

    // Create a dictionary for the image data
    DynamicJsonDocument doc(1024);
    doc["image"] = image_data;

    // Upload the image
    String url = "http://192.168.1.81:8000/api/upload/";
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-CSRFToken", csrfToken);
    http.POST(doc.as<String>());
    Serial.println(doc.as<String>());

    // Check the status of the request
    // int http_code = http.response().statusCode();
    // if (http_code == 200) {
    //   Serial.println("Image uploaded");
    // } else {
    //   Serial.println("Image upload failed");
    // }

    // Free the memory used by the image
     esp_camera_fb_return(fb);
  }
  delay(5000);
}



void sendPhoto3() {
  HTTPClient http;

  http.begin(serverName, serverPort, serverPath);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + String(boundary));
  http.addHeader("X-CSRFToken", csrfToken);
  // http.addHeader("Cookie", "csrftoken=" + String(csrfToken));

  // Take a photo with the camera
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Failed to capture image");
    return;
  }

  // Calculate the content length
  size_t contentLength = fb->len;

    // Create the request body
  String requestBody = "--" + String(boundary) + "\r\n";
  requestBody += "Content-Disposition: form-data; name=\"csrfmiddlewaretoken\"\r\n\r\n";
  requestBody += String(csrfToken) + "\r\n";
  requestBody += "--" + String(boundary) + "\r\n";
  requestBody += "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n";
  requestBody += "Content-Type: image/jpeg\r\n\r\n";

  // Send the headers and request body
  http.addHeader("Content-Length", String(requestBody.length() + contentLength + 6));  // +6 for the closing boundary

  // Send the headers and request body
  http.useHTTP10(true);
  http.sendRequest("POST", serverName);

  // Send the request body
  client.write((uint8_t*)requestBody.c_str(), requestBody.length());

  // Send the image data
  const uint8_t* imageData = fb->buf;
  size_t remainingBytes = contentLength;
  size_t chunkSize = 1024;
  while (remainingBytes > 0) {
    size_t bytesToWrite = min(chunkSize, remainingBytes);
    client.write(imageData, bytesToWrite);
    imageData += bytesToWrite;
    remainingBytes -= bytesToWrite;
  }

  
  // Send the closing boundary
  String closingBoundary = "\r\n--" + String(boundary) + "--\r\n";
  client.write((uint8_t*)closingBoundary.c_str(), closingBoundary.length());

  http.end();
  // int httpResponseCode = http.end();

  // // Read the response
  // int httpResponseCode = http.responseStatusCode();


  // if (httpResponseCode > 0) {
  //   Serial.print("HTTP Response code: ");
  //   Serial.println(httpResponseCode);
  // } else {
  //   Serial.println("Failed to make HTTP request");
  // }

  // http.end();
  esp_camera_fb_return(fb);


  delay(5000);

}

void sendPhoto2() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Convert image data to base64
  String base64Image = base64::encode(fb->buf, fb->len);

  // Setup the headers

  // POST Request
  HTTPClient http;
  http.begin(serverName, serverPort, serverPath);

  // Set the content type to multipart/form-data
  http.addHeader("Content-Type", "multipart/form-data; boundary=---------------------------1234567890");
  http.addHeader("X-CSRFToken", csrfToken);

  // Construct the multipart/form-data payload
  String requestBody = "-----------------------------1234567890\r\n";
  requestBody += "Content-Disposition: form-data; name=\"csrfmiddlwaretoken\"\r\n";
  requestBody += csrfToken + "\r\n\r\n";
  requestBody += "-----------------------------1234567890\r\n";
  requestBody += "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n";
  requestBody += "Content-Type: image/jpeg\r\n\r\n";
  requestBody += base64Image + "\r\n";
  requestBody += "-----------------------------1234567890--\r\n";

  Serial.print("RequestBody:");
  Serial.println(requestBody);

  // Send the POST request with the constructed payload
  int httpResponseCode = http.sendRequest("POST", requestBody);

  // Confirm the response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.println("HTTP Request failed");
  }

  http.end();
  esp_camera_fb_return(fb);
}

void sendPhoto1() {

  camera_fb_t* fb = NULL;

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Convert image data to base64
  String base64Image = base64::encode(fb->buf, fb->len);

  // Setup the headers.

  // POST Request
  HTTPClient http;
  http.begin(serverName, serverPort, serverPath);
  http.addHeader("Content-Type", "multipart/form-data");
  http.addHeader("Cookie", "csrftoken=" + csrfToken);
  http.addHeader("Content-Type", "multipart/form-data; boundary=FinalProject");   // Setting the Boundary

  String requestBody = "FinalProject\r\n";
  requestBody += "Content-Disposition: form-data; name=\"csrfmiddlwaretoken\"\r\n\r\n";
  requestBody += csrfToken + "\r\n";
  requestBody += "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n";
  requestBody += "Content-Type: image/jpg\r\n\r\n";
  requestBody += base64Image;
  requestBody += "\r\nFinalProject\r\n";
  requestBody += "Content-Disposition: form-data; name=\"id_image\"\r\n\r\n";
  requestBody += "cam1\r\n";  // Replace "id_image" with the actual ID of the ESP32-Cam

  // String requestBody = "{\"image\": \"" + base64Image + "\"}";

  // Send the POST request
  int httpResponseCode = http.POST(requestBody);

  // Confirm the response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  esp_camera_fb_return(fb);
}


String sendPhoto() {
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
    // Include the CSRF Token here...:
    String head = "FinalProject\r\nContent-Disposition: form-data; name=\"image\"; filename=\"esp32-com.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\nFinalProject\r\n";
    Serial.println(head);
    Serial.println(tail);

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=FinalProject\r\n");
    client.println("Cookie: csrftoken=" + csrfToken + "\r\n");
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

void getCSRFToken() {

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connected for CSRF Token");

    // GET Request
    client.println("GET /api/upload/ HTTP/1.1");
    client.println("Host: " + String(serverName));
    client.println("Connection: close");
    client.println();

    // Get the server response
    bool startedResponse = false;
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);

      // if (!startedResponse) {
      //   // Skip the headers until empty line
      //   Serial.println(line);

      //   if (line == "\r") {
      //     startedResponse = true;
      //   }
      // } else {
      //   // Get the CSRF Token
      //   Serial.println("Starting with Token!");
      //   if (line.startsWith("<input type=\"hidden\" name=\"csrfmiddlewaretoken\"")) {
      //     int valStartInd = line.indexOf("value=\"") + 7;
      //     int valEndInd = line.indexOf("\"", valStartInd);
      //     Serial.println("StartIndex: " + String(valStartInd));
      //     csrfToken = line.substring(valStartInd, valEndInd);
      //     Serial.println(line.substring(valStartInd, valEndInd));
      //     break;  // Exit the loop if we have the CSRF token
      //   }
      // }
    }

    // Closing the connection
    client.stop();

    // Debugging - Print the csrf token
    Serial.println("CSRF: " + csrfToken);
  } else {
    Serial.println("Connection Failed");
  }
}

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
