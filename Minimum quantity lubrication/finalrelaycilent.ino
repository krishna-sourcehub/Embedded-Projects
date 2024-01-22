#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
float value;
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
const char* ssid = "ESP8266";
const char* password = "123456789";
char c;
int count;
const char* serverNameTemp = "http://192.168.4.1/temperature";
const char* serverNameObjectTemp = "http://192.168.4.1/humidity";

int readpin;
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String Ambient_temperature;
String object_temperature;

unsigned long previousMillis = 0;
const long interval = 5000;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.clear();
  lcd.backlight();

  lcd.setCursor(0, 0);
  Serial.println();
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) {
    }
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  digitalWrite(D3, HIGH);
  digitalWrite(D4, LOW);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(D3, HIGH);
    digitalWrite(D4, LOW);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH);
    digitalWrite(D7, LOW);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi");
  digitalWrite(D4, HIGH);
  digitalWrite(D3, LOW);
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, LOW);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(D3, HIGH);
    digitalWrite(D4, LOW);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH);
    digitalWrite(D7, LOW);
    Serial.println("Wi-Fi disconnected. Reconnecting...");

    WiFi.disconnect();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(D3, HIGH);
      digitalWrite(D4, LOW);
      digitalWrite(D5, HIGH);
      digitalWrite(D6, HIGH);
      digitalWrite(D7, LOW);
      Serial.print(".");
    }

    Serial.println();
    Serial.print("Connected to Wi-Fi. IP address: ");
    digitalWrite(D4, HIGH);
    digitalWrite(D3, LOW);
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH);
    Serial.println(WiFi.localIP());
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      Ambient_temperature = httpGETRequest(serverNameTemp);
      object_temperature = httpGETRequest(serverNameObjectTemp);

      Serial.println("Ambient_temperature: " + Ambient_temperature + " *C");
      lcd.setCursor(0, 0);
      lcd.print("Room_temp:" + Ambient_temperature + "C");
      Serial.println("Object_Temperature: " + object_temperature + "*C");
      lcd.setCursor(0, 1);
      lcd.print("Bit_temp:" + object_temperature + "C");
      display.clearDisplay();
      int readpin = digitalRead(D8);
      Serial.print(readpin);

      if (readpin == 0) {
        digitalWrite(D5, LOW);
        digitalWrite(D6, HIGH);
        digitalWrite(D7, LOW);

        float temperaturevalue = object_temperature.toFloat();
        if (temperaturevalue > 70) {
          digitalWrite(D5, LOW);
          digitalWrite(D6, LOW);
        }
      }
      count = 0;
      if(readpin == 1){
         if (count == 0) {
        digitalWrite(D5, HIGH);
        digitalWrite(D6, HIGH);
        digitalWrite(D7, HIGH);
        count=1;}
      while (readpin == 1 && Serial.available() > 0) {
       
        String input = Serial.readStringUntil('\n');
        float value = input.toFloat();

        Serial.print("Input: ");
        Serial.println(input);

        Serial.print("Float value: ");
        Serial.println(value);

        if (value <= 70.00) {
          digitalWrite(D5, LOW);
          digitalWrite(D6, HIGH);
        }

        if (value > 70.00) {
          digitalWrite(D5, LOW);
          digitalWrite(D6, LOW);
        }

        if (readpin == 0) {
          Serial.print("\nreadpin" + readpin);
          break;
        }
      }
      }
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.print("T: ");
      display.print(" ");
      display.setTextSize(1);
      display.cp437(true);
      display.write(248);
      display.setTextSize(2);
      display.print("C");

      display.display();

      previousMillis = currentMillis;
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}

String httpGETRequest(const char* serverName) {
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
