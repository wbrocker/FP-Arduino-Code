// This is the 1st Sensor Unit
// Running on: ESP8266


#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include "OTA.h"
#include <TelnetStream.h>
#include "config.h"

#define LED 2

// WebServer server(80);
StaticJsonDocument<512> jsonDocument;
char buffer[512];


String serverName = "192.168.1.35";             // Server address for pictures
// String serverPath = "/api/upload/";             // Upload URL for pictures
String serverPath = "/devices/register/";       // Registration Endpoint
const int Id = 1;                         // This is the camera identifier.
const int serverPort = 8000;                    // Server Port 
String hostName = "ESP2-Sensor";                   // Setting the Device Hostname
String firmware = "0.1";
int counter = 0;
bool ledStatus = false;

WiFiClient client;

bool alarm = false;

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("Wifi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  setup_wifi();

  pinMode(LED, OUTPUT);

  setupOTA("ESP-SEN1", ssid, password);

  // // Start Wifi
  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // TelnetStream.begin();
  // TelnetStream.println("WiFi Connected!");
  // Serial.println("WiFi Connected!");
  // TelnetStream.println(WiFi.localIP());
  // ip_addr = WiFi.localIP().toString();
  // //Serial.print(WiFi.localIP());
  // Serial.println("' to connect");

  // updateLocaleVariables();                  // Push local vars to Controller.

  // Connecting to the Server
  if (client.connect(serverName, serverPort)) {
    Serial.println("Connected to the Server!");

    // REST Request
    // Serial.print(String("GET ") + serverPath + " HTTP/1.1\r\n" +
    //               "Host: " + serverName + "\r\n" +
    //               "Connection: close\r\n\r\n");

    String parameters = "?ip=" + WiFi.localIP().toString() + "&type=SEN&host=" + hostName;

    client.print(String("GET ") + serverPath + parameters +" HTTP/1.1\r\n" +
              "Host: " + serverName + "\r\n" +
              "Connection: close\r\n\r\n");

    // Wait for the server response
    while (client.available()) {
      Serial.println("Getting Device Configs");
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    Serial.println("Response Received.");
    client.stop();
  } else {
    Serial.println("Connection to the server failed!");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Waiting: ");
  Serial.println(String(counter));
  counter++;
  delay(1000);
  ledStatus = !ledStatus;
  digitalWrite(LED, ledStatus);



}

// void updateLocaleVariables() {
//   HTTPClient http;

//   Serial.print("[HTTP] registration...\n");
//   TelnetStream.print("[HTTP] registration...\n");

//   // String url = "http://";
//   // url += serverName + ":";
//   // url += String() + serverPort;
//   // url += "/devices/register";

//   // url += "?ip=" + ip_addr;
//   // url += "&host=" + hostname;
//   // url += "&type=CAM";

//   // TelnetStream.println(url);
//   String url = "http://192.168.1.35:8000/devices/register/?ip=192.168.1.32&host=test&type=SEN";

//   http.begin(url);

//   Serial.print("[HTTP GET...\n");
//   int httpCode = http.GET();

//   if(httpCode > 0) {
//     Serial.printf("[HTTP] GET... code: %d\n", httpCode);

//     if (httpCode == HTTP_CODE_OK) {
//       // Parse JSON Response
//       const size_t capacity = JSON_OBJECT_SIZE(6) + 150;
//       DynamicJsonDocument doc(capacity);
//       DeserializationError error = deserializeJson(doc, http.getString());
//       if (error) {
//         Serial.print("deserializeJson() failed: ");
//         TelnetStream.print("deserializeJson() failed: ");
//         Serial.println(error.f_str());
//         http.end();
//         return;
//       }

//       // Extract JSON
//       alarm = doc["alarm"];
//       // useLed = doc["flash"];
//       // picInterval = doc["picInterval"];
//       // camStatus = doc["camStatus"];
//       // sleepMode = doc["sleep"];
//       } else {
//         TelnetStream.print("HTTP request failed with error: ");
//         TelnetStream.println(httpCode);
//       }
//     http.end();
//   }
// }
