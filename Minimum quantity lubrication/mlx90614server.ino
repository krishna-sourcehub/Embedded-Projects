#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Adafruit_MLX90614.h>
float Ambient_temperature;
float Object_temperature;
float g;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
//int outputpin = A0;
// #include <Wire.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>

// Set your access point network credentials
const char* ssid = "ESP8266";
const char* password = "123456789";

/*#include <SPI.h>
#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 5*/

//Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readTemp() {
  Serial.print("Ambient temperature ="); 
  Serial.print(mlx.readAmbientTempC());
  Ambient_temperature =mlx.readAmbientTempC();
  Serial.print("°C");      
  Serial.print("   ");
  Serial.print("Object temperature = "); 
  Serial.print(mlx.readObjectTempC()); 
  Object_temperature =mlx.readObjectTempC();
  Serial.println("°C");
  Serial.print("Ambient temperature = ");
  Serial.print(mlx.readAmbientTempF());
  Serial.print("°F");      
  Serial.print("   ");
  Serial.print("Object temperature = "); 
  Serial.print(mlx.readObjectTempF()); 
  Serial.println("°F");
  return String(Ambient_temperature);
 // return String(Object_temperature);

  // int analogValue = analogRead(outputpin);
// float millivolts = (analogValue/1024.0) * 3300; //3300 is the voltage provided by NodeMCU
// float celsius = millivolts/10;
// Serial.print("in DegreeC=   ");
//  Serial.println(celsius);
//---------- Here is the calculation for Fahrenheit ----------//
// float fahrenheit = ((celsius * 9)/5 + 32);
// Serial.print(" in Farenheit=   ");
// Serial.println(fahrenheit);
// return String(fahrenheit);
// return String(celsius);
  // return String(bme.readTemperature());
  //return String(1.8 * bme.readTemperature() + 32);
}

 String readHumi() {
   //return String(bme.readHumidity());
   return String(Object_temperature);
 }

// String readPres() {
//   return String(bme.readPressure() / 100.0F);
// }

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(D3,OUTPUT);
  pinMode(D4,OUTPUT);
  digitalWrite(D4, HIGH);
  Serial.println();
  
  // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (!mlx.begin()) {
     Serial.println("Error connecting to MLX sensor. Check wiring.");
     while (1);
  };
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTemp().c_str());
  });
   server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readHumi().c_str());
  });
  // server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readPres().c_str());
  // });
  
  // bool status;

  // // default settings
  // // (you can also pass in a Wire library object like &Wire2)
  // //status = bme.begin(0x76);  
  // if (!status) {
  //   Serial.println("Could not find a valid BME280 sensor, check wiring!");
  //   while (1);
  // }
  
  // Start server
  server.begin();
}
 
void loop(){
    g=mlx.readAmbientTempC();
    if (WiFi.softAPgetStationNum()==1) {
    digitalWrite(D3, HIGH);  // Turn on the LED
  }
  if (WiFi.softAPgetStationNum() == 0) {
    digitalWrite(D3, LOW);  // Turn on the LED
  } 
  // else {
  //   digitalWrite(D3, LOW);   // Turn off the LED
  // }
    //Serial.print(g);
}