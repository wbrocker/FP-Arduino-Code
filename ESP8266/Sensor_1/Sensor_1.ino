// This is the 1st Sensor Unit
// Running on: ESP8266


#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include "OTA.h"
#include <TelnetStream.h>
#include "config.h"
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <PubSubClient.h>
#include "ClickButton.h"


// Declaration of MQTT Topics
#define MQTT_TEMP "esp2/temp"
#define MQTT_HUM "esp2/hum"
#define MQTT_ALARM "alarm"
#define MQTT_ALARM_TRIG "alarmtrigger"
#define MQTT_BUTTON "esp2/button"

#define LED 2                                   // Onboard LED
#define TEMP_HUM_PIN D1                         // DHT Pin
#define BUTTON D2                               // Button Pin
#define ALARM_LED D5                            // Alarm LED
#define BUZZER D3                               // Buzzer Pin

ClickButton button(BUTTON, HIGH);               // Declare the Button instance

// WebServer server(80);
StaticJsonDocument<512> jsonDocument;
char buffer[512];


String serverName = "192.168.1.35";             // Server address for pictures
String serverPath = "/devices/register/";       // Registration Endpoint
const int Id = 1;                               // This is the Sensor identifier.
const int serverPort = 8000;                    // Server Port 
String hostName = "ESP2-Sensor";                // Setting the Device Hostname
String firmware = "0.2";
int counter = 0;
bool updatedHost = false;                       // Indicate if Controller have been notified

bool alarmArmed = false;                        // This variable show if the alarm is armed
bool alarmTriggered = false;                    // Variable if Alarm is triggered
bool alarmUseLed = true;                        // Using Visual Alarm
bool alarmUseBuzzer = false;                    // Using Audible alarm
bool alarmLedState = false;                     // Alarm LED
bool alarmBuzzerState = false;

const unsigned long ledInterval = 1000;         // Blink ledInterval for alarmArmed LED
const unsigned long ledAlarmInterval = 300;     // Blink Interval for Alarm LED
unsigned long previousMillis = 0;               // Store the last time for LED Blink
unsigned long previousMillisAlarm = 0;          // Last time for Alarm Indication
bool ledState = false;


WiFiClient espClient;
HTTPClient http;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];



// Initialise DHT22 Component
DHT dht(TEMP_HUM_PIN, DHT22);

bool alarm = false;             // Is the overall Alarm sounding

String temp_String;
char tempString[20];            // Placeholder for MQTT Messages

// Variables for sensors
int extTemp = 0;
int hum = 0;
int temperature = 0;
int humidity = 0;


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


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  // Check if it matches any topic
  if (strcmp(topic, MQTT_ALARM) == 0) {     // Alarm Topic Raised
    if (message.toInt() == 1) {             // Raise Alarm
      alarmArmed = true;
      // digitalWrite(ALARM_LED, HIGH);
    } else {
      alarmArmed = false;
      // digitalWrite(LED, LOW);
    }
  } else if (strcmp(topic, MQTT_ALARM_TRIG) == 0) {  // Alarm is Triggered
    if (message.toInt() == 1) {
      alarmTriggered = true;
    } else {
      alarmTriggered = false;
    }
  }
}

// Reconnect to MQTT
void reconnect() {
  // Loop until we are connected
  while (!client.connected()) {
    Serial.println("Attempting to connect to MQTT");
    // Create random clientid
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected!");
      // Publish announcement
      client.publish("esp/announce", "Hello ESP2");
      // Subscribe to topics
      client.subscribe("inTopic");
      client.subscribe("alarm");
      client.subscribe("alarmtrigger");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds...");
      delay(5000);
    }
  }
}

int registerDevice() {
  String parameters = "?ip=" + WiFi.localIP().toString() + "&type=SEN&host=" + hostName + "&firmware=" + firmware;
  String url = "http://" + String(serverName) + ":" + serverPort + serverPath + parameters;

  Serial.println(url + parameters);

  http.useHTTP10(true);
  http.begin(espClient, url);
  int httpCode = http.GET();

  if (httpCode == 200) {

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getStream());

    Serial.println(doc["alarm"].as<long>());
    Serial.println(doc["sensorid"].as<long>());
  }
  http.end();

  return httpCode;
}

// Function Declarations
void readTempHum(void);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Setup PIN Modes
  pinMode(LED, OUTPUT);
  pinMode(ALARM_LED, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(BUZZER, OUTPUT);

  // Initialise default values
  digitalWrite(LED, LOW);
  digitalWrite(ALARM_LED, LOW);
  digitalWrite(BUZZER, LOW);

  button.debounceTime = 20;
  button.multiclickTime = 250;
  button.longClickTime = 1000;


  dht.begin();

  setupOTA("ESP-SEN1", ssid, password);

  // Register this device with the controller
  if (!updatedHost) {
    Serial.println("Updating local variables");
    if (registerDevice() == 200) {
      Serial.println("Registration Done");
      updatedHost = true;
    }
  }

    // Check WiFi Connection
  if ((WiFi.status() != WL_CONNECTED)) {
    // Host need to re-register
    updatedHost = false;
    Serial.println("Lost Connection!");
    // WiFi.reconnect();
    WiFi.disconnect();
    
    WiFi.reconnect();
    delay(10000);
  }
}

void loop() {
  // Update the Button State
  button.Update();

  unsigned long currentMillis = millis();   // Get the current time

  // If the Alarm is armed, we need to flash the 
  // Blue LED to indicate it.
  if (alarmArmed) {
    if (currentMillis - previousMillis >= ledInterval) {
      previousMillis = currentMillis;     // Save current time as last update

      // Toggle the LED state
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
    }

    // Other actions for Alarm... If Alarm Triggered
    if (alarmTriggered) {
      if (alarmUseLed) {
        if (currentMillis - previousMillisAlarm >= ledAlarmInterval) {
          previousMillisAlarm = currentMillis;

          // Toggle the LED
          if (alarmLedState == LOW) {
            alarmLedState = HIGH;
          } else {
            alarmLedState = LOW;
          }
        }
      }

      if (alarmUseBuzzer) {
        alarmBuzzerState = HIGH;
      }
    } else {
      alarmLedState = LOW;
      alarmBuzzerState = LOW;
    }

  } else {
    ledState = HIGH;
  }

  // Update the Blue LED
  digitalWrite(LED, ledState);
  // Update RED Alarm LED
  digitalWrite(ALARM_LED, alarmLedState);
  digitalWrite(BUZZER, alarmBuzzerState);


  if (button.clicks == 1) {
    Serial.println("Button Clicked!");
    client.publish(MQTT_BUTTON, "1");        // 1 = 1 click
  }

  if (button.clicks == 2) {
    Serial.println("Double Clicked");
    client.publish(MQTT_BUTTON, "2");        // 2 = Double Click
  }

  // Read Temperature and Humidity
  readTempHum();

  // Confirm MQTT Connection
  if (!client.connected()) {
    reconnect();    
  }
  client.loop();

}

// Read the temperature and humidity values
void readTempHum() {
  // Set the Global Variables
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Serial.print("Temp: ");
  // Serial.println(temperature);

  // Determine if it changed and only publish them
  if (temperature != extTemp) {
    Serial.println("Publish Temperature!!!");
    extTemp = temperature;
    temp_String = String(extTemp);
    temp_String.toCharArray(tempString, temp_String.length() + 1);
    client.publish(MQTT_TEMP, tempString);
  }

  if (humidity != hum) {
    hum = humidity;
    temp_String = String(hum);
    temp_String.toCharArray(tempString, temp_String.length() + 1);
    client.publish(MQTT_HUM, tempString);    
  }
}


