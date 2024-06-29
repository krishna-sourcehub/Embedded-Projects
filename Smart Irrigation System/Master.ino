#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino.h>
#include <DS1307.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3c
// void timeout1(uint8_t minute, uint8_t start1);
// void timeout2(uint8_t minute, uint8_t start2);



#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const char *ssid = "POCOM2Pro";
const char *password = "00001111";

const char *apSSID = "ESP8266";

// dual core
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;
TaskHandle_t Task5;

// KeyPad

#include <Keypad.h>
char key;

#define ROWS 4
#define COLS 4

char keyMap[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

uint8_t rowPins[ROWS] = { 13, 12, 14, 27 };  // GIOP13, GIOP12, GIOP14, GIOP27
uint8_t colPins[COLS] = { 26, 25, 33, 32 };  // GIOP26, GIOP25, GIOP33, GIOP32

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS);

String modes = "NONE";
String motor = "false";
String valve1 = "false";
String valve2 = "false";


char value;


// RTC
uint8_t sec, minute, hour, day, month;
uint16_t year;
DS1307 rtc;
uint8_t currenthour;

// Sensor Data and Weather API
int temperature1 = 100;
int temperature2 = 100;
int moisture1 = 20;
int moisture2 = 20;
int targetHour = 13;  // Example: Search for data for 12:00 PM
uint8_t current_time;
bool ispossible;
String payload;
JsonArray forecastday;
uint8_t currentDate;
int apidate;

String fetechData;

//  today rain fall and hour temperature
float rainfall = 0;
float currentTemp;



float customTemp;
int customMoisture;
bool isIrrigration1;
bool isIrrigration2;



// Recive the Client Data
const char *client1Moisture = "http://192.168.4.2/moisture";
const char *client1Temp = "http://192.168.4.2/temperature";
const char *client2Moisture = "http://192.168.4.3/moisture";
const char *client2Temp = "http://192.168.4.3/temperature";

String client1MoistureResponse;
String client1TemperatureResponse;
String client2MoistureResponse;
String client2TemperatureResponse;
int client1Moisture_data;
float client1Temperature_data;
int client2Moisture_data;
float client2Temperature_data;

// time out
uint8_t start1;
uint8_t start2;
bool isConnected;







#define SOFT_INTERRUPT_INTERVAL_MS 100  // Adjust this value as needed

void task2(void *pvParameters);
// Task functions
void task1(void *pvParameters);
void task3(void *pvParameters);
void task4(void *pvParameters);
void task5(void *pvParameters);


TaskHandle_t task2Handle;
TaskHandle_t task1Handle;
TaskHandle_t task3Handle;
TaskHandle_t task4Handle;
TaskHandle_t task5Handle;


void accessPoint() {
  Serial.println("Setting AP (Access Point)....");
  WiFi.softAP(apSSID);
  Serial.println("WIFI AP/hotspot ON");
  Serial.print("AP IP ADDRESS         : ");
  Serial.println(WiFi.softAPIP());
}


void api() {
  // api code

  // Make HTTP GET request
  HTTPClient http;
  http.begin("https://api.weatherapi.com/v1/forecast.json?key=8473949565a9482da0870228241402&q=11.713843484355516,%2078.08535799429197&days=1&aqi=no&alerts=yes");

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    payload = http.getString();
    Serial.println("Response:");
    Serial.println(payload);
    fetechData = "true";

  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();  // Close connection

  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(10) + 440;
  DynamicJsonDocument doc(capacity);

  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject location = doc["location"];
  String localtime = location["localtime"];
  apidate = extractDate(localtime);
  Serial.print("api date :");
  Serial.println(apidate);
}

void connectwifi() {
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi....");

  isConnected = false;  // Flag to track if the connection is successful
  int i = 0;

  // Check if already connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi!");
    // isNet="true";
    return;
  }



  while (!(WiFi.status() == WL_CONNECTED)) {
    Serial.println(" - ");
    delay(500);
    i++;
    if (i >= 5)  // 10 seconds delay, as we're delaying 500 ms each time
    {
      break;  // Break the loop and return if 10 seconds elapsed
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    isConnected = true;
    Serial.println("WiFi connected");

    Serial.print("WiFi IP ADDRESS       : ");
    Serial.println(WiFi.localIP());

    Serial.print("WiFi Strength         : ");
    Serial.println(WiFi.RSSI());

    Serial.print("ESP32 MAC address     : ");
    Serial.println(WiFi.macAddress());

    Serial.println("*******************************************************");
  }

  if (!isConnected) {
    Serial.println("Failed to connect to WiFi within 10 seconds.");
  }
}





void screen2() {
  // checkrainandtemp();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 0);
  display.print("TIME: ");
  display.print(hour, DEC);
  display.print(":");
  display.print(minute, DEC);
  display.print(":");
  display.print(sec, DEC);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 15);
  display.print("TODAY WEATHER");
  display.setCursor(10, 30);
  display.print("TEMP: ");
  display.print(currentTemp);
  display.println(" C");
  display.setCursor(10, 45);
  display.print("RAIN: ");
  display.print(rainfall);
  display.println(" mm");
  // Update the display
  display.display();
}

void mentor() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(44, 0);
  display.print("MENTOR");
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(18, 20);
  display.print("DR.D.ASHOKARAJU");
  display.setCursor(10, 40);
  display.print("ASSOCIATE PROFESSOR");
  // Update the display
  display.display();
}

void TeamLeader() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(34, 0);
  display.print("TEAM LEADER");
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(46, 30);
  display.print("ASHIF");
  // display.setCursor(10, 40);
  // display.print("Associate Professor");
  // Update the display
  display.display();
}

void team() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(46, 0);
  display.print("TEAM");
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  // display.setCursor(8, 10);
  // display.print("ASIF");
  display.setCursor(16, 15);
  display.print("1. YASWANTH");
  display.setCursor(16, 30);
  display.print("2. PRANAV");
  display.setCursor(16, 45);
  display.print("3. SATHISH");
  // Update the display
  display.display();
}

void Status() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(20, 0);
  display.print("   INTERNET");
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(12, 32);
  if (isConnected == true) {
    display.print("    CONNECTED");
  }
  if (isConnected == false) {
    display.print("   DISCONNECTED");
  }
  // Update the display
  display.display();
}


void intro() {
  display.clearDisplay();

  testdrawroundrect();
  delay(2000);
  display.clearDisplay();

  testfillroundrect();
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(12, 32);
  display.print("SMART IRRIGRATION");
  // display.setTextSize(1);
  // display.setTextColor(SH110X_WHITE);
  // display.setCursor(8, 10);
  // Update the display
  display.display();
}



void testdrawroundrect(void) {
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, SH110X_WHITE);
    display.display();
    delay(1);
  }
}

void testfillroundrect(void) {
  uint8_t color = SH110X_WHITE;
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.fillRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, color);
    if (color == SH110X_WHITE) color = SH110X_BLACK;
    else color = SH110X_WHITE;
    display.display();
    delay(1);
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(19, OUTPUT);
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(15, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(19, HIGH);
  fetechData = "false";
  modes = "NONE";

  display.begin(i2c_Address, true);  // Address 0x3C default
  isIrrigration1 = false;
  isIrrigration2 = false;
  display.display();
  delay(2000);
  // Clear the buffer.
  display.clearDisplay();
  accessPoint();
  intro();
  // RTC Begin
  rtc.begin();
  rtc.start();

  // Connect to WiFi network
  connectwifi();

  api();
  RTC();

  rtc.get(&sec, &minute, &hour, &day, &month, &year);
  currenthour = hour;
  Serial.println("Setting up ESP32 in Station and Access Point mode...");
  //  // Create Task 1 with priority 1
  //   xTaskCreatePinnedToCore(
  //       task1,      // Task function
  //       "Task1",    // Task name
  //       10000,      // Stack size (bytes)
  //       NULL,       // Task parameters
  //       2,          // Priority
  //       NULL,       // Task handle
  //       1);         // Core ID (Core 0)


  // Create Task 2
  xTaskCreatePinnedToCore(
    task2,         // Task function
    "Task2",       // Task name
    10000,         // Stack size (bytes)
    NULL,          // Task parameters
    1,             // Priority
    &task2Handle,  // Task handle
    1              // Core ID (Core 1)
  );

  //  Create Task 1
  // xTaskCreatePinnedToCore(
  //   task1,      // Task function
  //   "Task1",    // Task name
  //   10000,      // Stack size (bytes)
  //   NULL,       // Task parameters
  //   1,          // Priority
  //   &task1Handle, // Task handle
  //   0           // Core ID (Core 1)
  // );

  xTaskCreatePinnedToCore(
    task3,         // Task function
    "Task3",       // Task name
    10000,         // Stack size (bytes)
    NULL,          // Task parameters
    2,             // Priority
    &task3Handle,  // Task handle
    0              // Core ID (Core 1)
  );

  //  xTaskCreatePinnedToCore(
  //   task4,      // Task function
  //   "Task4",    // Task name
  //   10000,      // Stack size (bytes)
  //   NULL,       // Task parameters
  //   1,          // Priority
  //   &task4Handle, // Task handle
  //   0           // Core ID (Core 1)
  // );

  // Create Task 5
  xTaskCreatePinnedToCore(
    task5,         // Task function
    "Task5",       // Task name
    10000,         // Stack size (bytes)
    NULL,          // Task parameters
    1,             // Priority
    &task5Handle,  // Task handle
    0              // Core ID (Core 1)
  );

  xTaskCreatePinnedToCore(
    task4,         // Task function
    "Task4",       // Task name
    10000,         // Stack size (bytes)
    NULL,          // Task parameters
    2,             // Priority
    &task4Handle,  // Task handle
    1              // Core ID (Core 1)
  );

  checkWeather();
}

void task5(void *pvParameters) {
  Serial.print("Task5 running on core ");
  Serial.println(xPortGetCoreID());


  for (;;) {
    Status();
    delay(3000);
    clientData(key);
    delay(3000);
    // if (WiFi.softAPgetStationNum() >= 1) {
    //   // Serial.println("Both Devices are Connected");
    //   clientData(key);
    //   delay(5000);
    // }
    // RTC();
    retrivedata();
    delay(5000);
    screen2();
    delay(5000);
    Irrigation();
    delay(5000);
    mentor();
    delay(5000);
    TeamLeader();
    delay(3000);
    team();
    delay(3000);
  }
}



// Task 1 function
// void task1(void *pvParameters) {
//   while (true) {
//     Serial.println("Task 1 running with priority 1");
//     retrivedata();
//     delay(1000); // Add some delay for demonstration
//   }
// }

// Task 3 function
void task4(void *pvParameters) {
  while (true) {
    Serial.println("Task 4 running with priority 1");
    Irrigation1();
  }
}
void task3(void *pvParameters) {
  while (true) {
    Serial.println("Task 3 running with priority 2");
    countClient();
  }
}

// Task 4 function
// void task4(void *pvParameters) {
//   while (true) {
//     Serial.println("Task 4 running with priority 3");
//     Irrigation();
//   }
// }


void loop() {
  // Other loop code...

  // Soft interrupt emulation
  vTaskDelay(SOFT_INTERRUPT_INTERVAL_MS / portTICK_PERIOD_MS);
  // Trigger Task 2
  xTaskNotifyGive(task2Handle);


  Serial.print("NO OF DEVICES CONNECTED  : ");
  Serial.println(WiFi.softAPgetStationNum());
  //check internet status
  bool success = Ping.ping("www.google.com", 3);
  if (!success) {
    Serial.println("Ping failed");
  } else {
    Serial.println("Ping succesful.");
  }


  countClient();
  // RTC();
  if (!(WiFi.status() == WL_CONNECTED)) {
    connectwifi();
    Serial.print("Reconnecting to Wifi ");
  }
  // timeout1(minute, start1);
  // timeout2(minute, start2);
  RTC();
}

void task2(void *pvParameters) {
  while (true) {
    // Wait for notification from loop()
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Execute the required actions
    char key = keypad.getKey();
    if (key) {
      display.clearDisplay();
      clientData(key);
      delay(1000);
    }
  }
}


void retrivedata() {

  client1MoistureResponse = httpGETRequest(client1Moisture);
  client1TemperatureResponse = httpGETRequest(client1Temp);
  client2MoistureResponse = httpGETRequest(client2Moisture);
  client2TemperatureResponse = httpGETRequest(client2Temp);

  client1Moisture_data = client1MoistureResponse.toInt();
  client1Temperature_data = client1TemperatureResponse.toFloat();
  client2Moisture_data = client2MoistureResponse.toInt();
  client2Temperature_data = client2TemperatureResponse.toFloat();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(32, 0);
  display.print("SENSOR DATA");
  display.setCursor(4, 20);
  display.print("M1: ");
  display.print(client1Moisture_data);
  display.println(" %");
  display.setCursor(68, 20);
  display.print("M2: ");
  display.print(client2Moisture_data);
  display.println(" %");
  display.setCursor(4, 40);
  display.print("T1:");
  display.print(client1Temperature_data);
  display.println("C");
  display.setCursor(68, 40);
  display.print("T2:");
  display.print(client2Temperature_data);
  display.println("C");
  // Update the display
  display.display();
  Serial.print("Client 1 Moisture: ");
  Serial.print(client1Moisture_data);
  Serial.println(" *%");

  Serial.print("Client 1 Temperature: ");
  Serial.print(client1Temperature_data);
  Serial.println(" *C");

  Serial.print("Client 2 Moisture: ");
  Serial.print(client2Moisture_data);
  Serial.println(" *%");

  Serial.print("Client 2 Temperature: ");
  Serial.print(client2Temperature_data);
  Serial.println(" *C");
}


void clientData(char key) {
  display.clearDisplay();
  switch (key) {
    case '1':
      display.clearDisplay();
      modes = "WATER MELON";
      customTemp = 28;
      customMoisture = 75;
      Serial.println("One");
      break;
    case '2':
      display.clearDisplay();
      modes = "MUSK MELON";
      Serial.println("Two");
      customTemp = 30;
      customMoisture = 65;
      break;
    case '3':
      display.clearDisplay();
      modes = "BANANA";
      Serial.println("Three");
      customTemp = 30;
      customMoisture = 65;
      break;
    case '4':
      display.clearDisplay();
      modes = "ONION";
      Serial.println("Three");
      customTemp = 32;
      customMoisture = 50;
      break;
    case '5':
      display.clearDisplay();
      modes = "TOMATO";
      Serial.println("Three");
      customTemp = 30;
      customMoisture = 65;
      break;
    case '6':
      display.clearDisplay();
      modes = "CHILLI";
      Serial.println("Three");
      customTemp = 32;
      customMoisture = 40;
      break;
    case '7':
      display.clearDisplay();
      modes = "CUCUMBER";
      Serial.println("Three");
      customTemp = 27;
      customMoisture = 80;
      break;
    case '8':
      display.clearDisplay();
      modes = "GRAPES";
      Serial.println("Three");
      customTemp = 28;
      customMoisture = 65;
      break;
    case '9':
      display.clearDisplay();
      modes = "CAULIFLOWER";
      Serial.println("Three");
      customTemp = 30;
      customMoisture = 60;
      break;
    case '0':
      display.clearDisplay();
      modes = "GARDEN";
      Serial.println("Three");
      customTemp = 30;
      customMoisture = 62;
      break;
    // case 'A':
    //   display.clearDisplay();
    //   // modes = "Apple";
    //   // Serial.println("Three");
    //   // customTemp = 99;
    //   // customMoisture = 99;
    //   break;
    // case 'B':
    //   display.clearDisplay();
    //   // modes = "ABOUT";
    //   Serial.println("Three");
    //   break;
    // case 'C':
    //   display.clearDisplay();
    //   // modes = "";
    //   Serial.println("Three");
    //   break;
    // case 'D':
    //   display.clearDisplay();
    //   // modes = "";
    //   Serial.println("Three");
    //   break;
    case '*':
      display.clearDisplay();
      modes = "MANUAL";
      Serial.println("Three");
      display.setCursor(10, 40);
      display.print("IRRIGATION TIMING");
      display.setCursor(10, 55);
      display.print("6 AM, 6 PM, 3 PM");
      break;
    // case '#':
    //   display.clearDisplay();
    //   // modes = "";
    //   Serial.println("Three");
    //   break;
    default:
      Serial.println("Number not recognized");
      break;
  }


  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 0);
  display.print("MODE OF IRRIGATION");
  display.setCursor(10, 20);
  display.print("MODE: ");
  display.println(modes);
  if (modes == "NONE") {
    display.setCursor(10, 40);
    display.print("SELECT THE MODE");
  }
  display.display();
}




String httpGETRequest(const char *serverName) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);

  int httpResponseCode = http.GET();

  String payload = "--";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  return payload;
}



void RTC() {
  rtc.get(&sec, &minute, &hour, &day, &month, &year);
  current_time = hour;
  currenthour = hour;
  Serial.print("\nTime: ");
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(sec, DEC);

  Serial.print("\nDate: ");
  Serial.print(day, DEC);
  Serial.print(".");
  Serial.print(month, DEC);
  Serial.print(".");
  Serial.print(year, DEC);
  Serial.println("");
  /*wait a second*/
  // delay(1000);
}



void countClient() {

  if (WiFi.softAPgetStationNum() == 2) {
    Serial.println("Both Devices are Connected");
    digitalWrite(0, HIGH);
    digitalWrite(2, LOW);
    digitalWrite(15, HIGH);
    // clientData(key);
  }

  else if (WiFi.softAPgetStationNum() == 1) {
    Serial.println("One Devices are Connected");
    digitalWrite(0, HIGH);
    digitalWrite(2, HIGH);
    digitalWrite(15, LOW);

  } else if (WiFi.softAPgetStationNum() == 0) {
    digitalWrite(0, HIGH);
    digitalWrite(2, HIGH);
    digitalWrite(15, LOW);
    Serial.println("No Devices are Connected");
  }
}

void controlIrrigation( int tempData1, int moistureData1, int tempData2, int moistureData2, const String &mode, int customTemp, int customMoisture) {
  if (mode != "MANUAL") {
    // if (isIrrigration2 == false && isIrrigration1 == false) {
    //       digitalWrite(4, HIGH);
    //     }
    // Automatic mode: control irrigation based on temperature and moisture data
    if (tempData1 == 0 && moistureData1 == 0) {
      digitalWrite(16, HIGH);
      isIrrigration1 = false;
      if (isIrrigration1 == false && isIrrigration2 == false) {
        digitalWrite(4, HIGH);
      }

    } else {
      if (tempData1 >= customTemp && moistureData1 <= customMoisture) {
        checkWeather();
        if (!ispossible) {
          digitalWrite(16, LOW);
          delay(200);
          digitalWrite(4, LOW);
          isIrrigration1 = true;
        }
      }
      if (tempData1 < customTemp && moistureData1 > customMoisture) {


        if (isIrrigration2 == false) {
          digitalWrite(4, HIGH);
        }
        delay(200);
        digitalWrite(16, HIGH);
        isIrrigration1 = false;
      }
    }



    if (tempData2 == 0 && moistureData2 == 0) {
      digitalWrite(17, HIGH);
      isIrrigration2 = false;
      if (isIrrigration1 == false && isIrrigration2 == false) {
        digitalWrite(4, HIGH);
      }

    } else {
      if (tempData2 >= customTemp && moistureData2 <= customMoisture) {

        checkWeather();
        if (!ispossible) {
          digitalWrite(17, LOW);
          delay(200);
          digitalWrite(4, LOW);
          isIrrigration2 = true;
        }
      }
      if (tempData2 < customTemp && moistureData2 > customMoisture) {

        if (isIrrigration1 == false) {
          digitalWrite(4, HIGH);
        }
        delay(200);
        digitalWrite(17, HIGH);
        isIrrigration2 = false;
      }
    }
  }
  else {
    // Manual mode: control irrigation based on specified times
    if (current_time == 6 || current_time == 15 || current_time == 18 || current_time == 16) {
      digitalWrite(16, LOW);
      digitalWrite(17, LOW);
      delay(200);
      digitalWrite(4, LOW);
      isIrrigration1 = true;
      isIrrigration2 = true;

    } else {
      digitalWrite(4, HIGH);
      delay(200);
      digitalWrite(16, HIGH);
      digitalWrite(17, HIGH);
      isIrrigration1 = false;
      isIrrigration2 = false;
    }
  }
}

void Irrigation1() {
  RTC();
  if (WiFi.softAPgetStationNum() >= 1) {
    if (modes != "NONE") {
      controlIrrigation( client1Temperature_data, client1Moisture_data, client2Temperature_data, client2Moisture_data, modes,  customTemp, customMoisture);
      // controlIrrigation( client1Temperature_data, client1Moisture_data, client2Temperature_data, client2Moisture_data, isIrrigration2, modes,  customTemp, customMoisture);
    }
  }
}


// void Irrigation1() {

//   if (WiFi.softAPgetStationNum() >= 1) {

//   if (modes !="NONE") {
//     if(modes !="MANUAL"){
//     Serial.println(" current_time ");
//     Serial.println(targetHour);
//     Serial.println(current_time);
//     Serial.print("customMoisture :");
//     Serial.println(customMoisture);
//     Serial.print("Custom Temperature :");
//     Serial.println(customTemp);


//     Serial.println(" current_time is matched");
//     if (client1Temperature_data >= customTemp && client1Moisture_data <= customMoisture) {
//       if (isIrrigration1 == false) {
//         checkWeather();
//         if (ispossible == "false") {
//           digitalWrite(16, LOW);
//           delay(200);
//           digitalWrite(4, LOW);
//           Serial.println("Irrigation is started for client 1");
//           isIrrigration1 = true;
//         }
//       }
//     }

//     if (client1Temperature_data < customTemp && client1Moisture_data > customMoisture) {
//       if (isIrrigration1 == true) {
//           digitalWrite(16, HIGH);
//           delay(200);
//           digitalWrite(4, HIGH);
//           Serial.println("Irrigation is started for client 1");
//           isIrrigration1 = "false";

//       }
//     }
//     checkWeather();
//     if (client2Temperature_data >= customTemp && client2Moisture_data <= customMoisture) {
//       if (isIrrigration2 == false) {
//         if (ispossible == "false") {
//           digitalWrite(17, LOW);
//           delay(200);
//           digitalWrite(4, LOW);
//           Serial.println("Irrigation is started for client 2");
//           isIrrigration2 =true;
//         }
//       }
//     }
//     if (client2Temperature_data < customTemp && client2Moisture_data > customMoisture) {
//       if (isIrrigration2 == true) {
//           digitalWrite(17, HIGH);
//           delay(200);
//           digitalWrite(4, HIGH);
//           Serial.println("Irrigation is started for client 2");
//           isIrrigration2 = false;
//       }
//     }


//   }}}


//  if (modes == "MANUAL") {
//     // Correctly check if current_time matches the specified times
//     if (current_time == 6 || current_time == 15 || current_time == 18 || current_time == 21) {
//         // Activate irrigation for client 1
//         digitalWrite(16, LOW);
//         delay(200);
//         digitalWrite(4, LOW);
//         Serial.println("Irrigation is started for client 1");
//         isIrrigration1 =true;

//         // Activate irrigation for client 2
//         digitalWrite(17, LOW);
//         delay(200);
//         digitalWrite(4, LOW);
//         Serial.println("Irrigation is started for client 2");
//         isIrrigration2 =true;
//     }

//     // Correctly check if current_time does not match any of the specified times
//     if (current_time != 6 && current_time != 15 && current_time != 18 && current_time != 22) {
//         // Deactivate irrigation for client 1
//         digitalWrite(16, HIGH);
//         delay(200);
//         digitalWrite(4, HIGH);
//         Serial.println("Irrigation is stopped for client 1");
//         isIrrigration1 = false;

//         // Deactivate irrigation for client 2
//         digitalWrite(17, HIGH);
//         delay(200);
//         digitalWrite(4, HIGH);
//         Serial.println("Irrigation is stopped for client 2");
//         isIrrigration2 = false;
//     }
// }

// }



void Irrigation() {
  // RTC();
  // if (modes != "MANUAL") {
  //   Serial.println(" current_time ");
  //   Serial.println(targetHour);
  //   Serial.println(current_time);
  //   Serial.print("customMoisture :");
  //   Serial.println(customMoisture);
  //   Serial.print("Custom Temperature :");
  //   Serial.println(customTemp);


  //   Serial.println(" current_time is matched");
  //   if (client1Temperature_data >= customTemp && client1Moisture_data <= customMoisture) {
  //     if (isIrrigration1 == "false") {
  //       checkWeather();
  //       if (ispossible == "false") {
  //         start1 = minute;
  //         digitalWrite(16, LOW);
  //         delay(200);
  //         digitalWrite(4, LOW);
  //         Serial.println("Irrigation is started for client 1");
  //         isIrrigration1 = "true";
  //         timeout1(minute, start1);
  //       }
  //     }
  //   }
  //   checkWeather();
  //   if (client2Temperature_data >= customTemp && client2Moisture_data <= customMoisture) {
  //     if (isIrrigration2 == "false") {
  //       if (ispossible == "false") {
  //         start2 = minute;
  //         digitalWrite(17, LOW);
  //         delay(200);
  //         digitalWrite(4, LOW);
  //         Serial.println("Irrigation is started for client 2");
  //         isIrrigration2 = "true";
  //         timeout2(minute, start2);
  //       }
  //     }
  //   }
  // }


  // if (modes == "MANUAL") {
  //   // if (hour == 6 || 15 || 18) {
  //   //   start1 = minute;
  //   //   digitalWrite(16, LOW);
  //   //   delay(200);
  //   //   digitalWrite(4, LOW);
  //   //   Serial.println("Irrigation is started for client 1");
  //   //   isIrrigration1 = "true";
  //   //   timeout1(minute, start1);
  //   //   start2 = minute;
  //   //   digitalWrite(17, LOW);
  //   //   delay(200);
  //   //   digitalWrite(4, LOW);
  //   //   Serial.println("Irrigation is started for client 2");
  //   //   isIrrigration2 = "true";
  //   //   timeout2(minute, start2);
  //   // }


  // }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 0);
  display.print("IRRIGATION STATUS");
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);


  if (isIrrigration1 == false && isIrrigration2 == false) {
    display.setCursor(8, 20);
    display.print("MOTOR STATUS :");
    display.println(" OFF");
  }

  if (isIrrigration1 == true) {
    display.setCursor(8, 20);
    display.print("MOTOR STATUS :");
    display.println(" ON");
    display.setCursor(10, 40);
    display.print("V1: ");
    display.println("ON");
  }

  if (isIrrigration1 == false) {
    display.setCursor(10, 40);
    display.print("V1: ");
    display.println("OFF");
  }


  if (isIrrigration2 == true) {
    display.setCursor(8, 20);
    display.print("MOTOR STATUS :");
    display.println(" ON");
    display.setCursor(60, 40);
    display.print("V2: ");
    display.println("ON");
  }

  if (isIrrigration2 == false) {
    display.setCursor(60, 40);
    display.print("V2: ");
    display.println("OFF");
  }

  // Update the display
  display.display();
  Serial.println("Irrigation Status");

  Serial.println("Irrigation Status");
}

// void timeout1(uint8_t minute, uint8_t start1) {
//   if (isIrrigration1 == "true") {
//     Serial.println("timeout 1 started");
//     Serial.print("timeout start1:");
//     Serial.println(start1);

//     // Check if 1 minute has elapsed since the last function call
//     if ((minute - start1) >= 1) {

//       Serial.println("Time 1 Out");
//       Serial.println("One minute has elapsed. 1");
//       if (isIrrigration2 == "true") {
//         digitalWrite(4, LOW);
//       }
//       if (!(isIrrigration2 == "true")) { digitalWrite(4, HIGH); }
//       digitalWrite(16, HIGH);
//       isIrrigration1 = "false";
//     }
//   }
// }

// void timeout2(uint8_t minute, uint8_t start2) {
//   if (isIrrigration2 == "true") {
//     Serial.println("timeout 2 started");
//     Serial.print("timeout start2: ");
//     Serial.println(start2);
//     // Check if 1 minute has elapsed since the last function call
//     if ((minute - start2) >= 1) {
//       Serial.println("Time 2 Out");
//       Serial.println("One minute has elapsed.2");
//       if (isIrrigration1 == "true") {
//         digitalWrite(4, LOW);
//       }
//       if (!(isIrrigration1 == "true")) { digitalWrite(4, HIGH); }
//       digitalWrite(17, HIGH);
//       isIrrigration2 = "false";
//     }
//   }
// }



int extractHour(String timeString) {
  // Example: Extract the hour value from the time string
  int delimiterIndex = timeString.indexOf(' ');                                      // Find the index of the space character
  String hourString = timeString.substring(delimiterIndex + 1, delimiterIndex + 3);  // Extract hour substring
  return hourString.toInt();                                                         // Convert hour substring to integer
}





String printWeatherData(JsonObject hour) {
  Serial.print("Time: ");
  Serial.println(hour["time"].as<String>());
  Serial.print("Temperature: ");
  Serial.println(hour["temp_c"].as<float>());
  Serial.print("Relative Humidity: ");
  Serial.println(hour["humidity"].as<int>());
  Serial.print("Rainfall: ");
  Serial.println(hour["precip_mm"].as<float>());
  Serial.print("Chance of Rainfall: ");
  Serial.println(hour["chance_of_rain"].as<float>());
  Serial.println();
  Serial.print("Time: ");
  Serial.println(hour["time"].as<String>());
  Serial.print("Temperature: ");
  Serial.println(hour["time"].as<String>());
  Serial.print("Temperature: ");
  Serial.println(hour["temp_c"].as<float>());
  Serial.print("Relative Humidity: ");
  Serial.println(hour["humidity"].as<int>());
  Serial.print("Rainfall: ");
  Serial.println(hour["precip_mm"].as<float>());
  Serial.print("Chance of Rainfall: ");
  Serial.println(hour["chance_of_rain"].as<float>());
  Serial.println();
  if (hour["precip_mm"] > rainfall) {
    rainfall = hour["precip_mm"];
  }
  Serial.print(" Highesh rain fall of today in mm ");
  Serial.println(rainfall);


  if (hour["chance_of_rain"].as<float>() == 0 && hour["precip_mm"].as<float>() == 0) {
    Serial.println("chance rain fall is matched ");
    return "True";
  } else {
    return "False";
  }
}


int extractDate(String localtime) {
  int spaceIndex = localtime.indexOf(' ');  // Find the index of the space character
  if (spaceIndex != -1) {
    String dateString = localtime.substring(0, spaceIndex);  // Extract substring before the space
    int day = dateString.substring(8).toInt();               // Extract day and convert to int
    return day;                                              // Return date as int (YYYYMMDD format)
  }
  return -1;  // Return -1 if space is not found (error)
}

String checkWeather() {

  if (WiFi.status() != WL_CONNECTED) {
    ispossible = false;
    // If not connected to WiFi, return an error message or handle it as needed
    return "Not connected to WiFi";

  }

  else {

    Serial.print("api and rtc date");
    Serial.println(day);
    if (fetechData == "false" || apidate != day) {

      api();
    }
    // Parse JSON
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(10) + 440;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return "error";
    }
    JsonObject location = doc["location"];
    String localtime = location["localtime"];
    int apidate = extractDate(localtime);
    Serial.print("api date :");
    Serial.println(apidate);

    JsonObject forecast = doc["forecast"];
    JsonArray forecastday = forecast["forecastday"];

    // Iterate over forecast days
    for (JsonObject day : forecastday) {
      // Print day's date
      Serial.print("Date: ");
      Serial.println(day["date"].as<String>());

      // Print hourly data
      Serial.println("Hourly data:");
      JsonArray hourly = day["hour"];
      for (JsonObject hour : hourly) {
        // Extract hour value from the time string
        int hourValue = extractHour(hour["time"].as<String>());
        Serial.print(" hourValue value is ");
        Serial.println(hourValue);
        // Check if the hour matches the target hour
        if (hourValue == currenthour) {
          // Print weather data for the current hour
          String result = printWeatherData(hour);
          Serial.println(" hourValue is matched");
          currentTemp = hour["temp_c"].as<float>();

          // Check the result of weather data for the next 4 hours
          for (int i = hourValue; i <= hourValue + 4; i++) {
            JsonObject nextHour = hourly[i];  // Get the next hour's data
            if (printWeatherData(nextHour) == "True") {
              ispossible = false;
            }
          }
        }
        // if(hourValue == 0){
        //   for (int i =hourValue; i <=23; i++) {
        //     JsonObject nextHour = hourly[i];  // Get the next hour's data
        //     printWeatherData(nextHour);
        //   }

        // }
        // Serial.print("current hour from rtc ");
        // Serial.println(currenthour);
        // Serial.print("current hour from api ");
        // Serial.println(hourValue);
      }
      return "complete";
    }
  }
}
