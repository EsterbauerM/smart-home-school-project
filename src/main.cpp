#include <Arduino.h>
#include <ArduinoJson.h>
#include <iostream>
#include <fstream>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <Servo.h>
#include <Wire.h>
#include <analogWrite.h>
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522Debug.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include "OneButton.h"

#include "wifi_credentials.h"

const unsigned int 
 ledPin = 12,
 buttonPins[2] = {16,27},           // left, right button
 thSensPin = 17,            // temp. & humidity
 steamPin = 34,
 fanMPin = 18, fanPPin = 19, // fan Minus & Plus
 pirPin = 14,
 rgbPin = 26,
 gasPin = 23,
 buzzerPin = 25,
 servosPins[2]={5,13};      // 5 is Window, 13 is Door

const uint8_t customAddress = 0x28;
TwoWire &customI2C = Wire;
MFRC522DriverI2C driver{customAddress, customI2C};
MFRC522 mfrc{driver};

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* PARAM_INPUT_1 = "state";

OneButton btn1(buttonPins[0],true);
OneButton btn2(buttonPins[1],true);

String password = "";
String correct_p = "-.-";  //The correct password for the password door

Servo servos[2];

int servo_indow_pos = 0;     // Start angle of servos
int servo_door_pos = 0;     
int posDegrees = 0;          // 0 = Window is open, 90 (roughly) = Window is closed

int ledState = LOW;          // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int ledToggle = HIGH;         // led on or off

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 40;

AsyncWebServer server(80);

const char* PARAM_MESSAGE = "message";

void click1();
void click2();
void longPress1();
void longPress2();

String outputState(){
  if(digitalRead(ledPin)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

void click1(){
  Serial.println(".");
  password = password + '.';
  lcd.setCursor(0,1);
  lcd.print(password);
}

void longPress1(){
  Serial.println("-");
  password = password + '-';
  lcd.setCursor(0, 1);
  lcd.print(password);
}

void click2() {
  Serial.println(password);
  if(password == correct_p)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("open");
    servos[1].write(180);  // Open door if psw is correct
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("press 2 to close");

  }
  else if(password ==""){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("close");
    servos[1].write(0);  // Close door btn    
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("enter passcode:");

  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("wrong.");
    servos[1].write(0);  // Make sure door is closed on wrong psw
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("enter again");
  }
  password = "";
  Serial.println(password);
}

//btn2 long press being detected sometimes?
// void longPress2() {
//   Serial.println("btn 2 long press");
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("close");
//   servos[1].write(0);  //Close the door
//   delay(1000);
//   lcd.clear();
//   lcd.print("enter passcode:");
// }

void cardSetup(){
  DynamicJsonDocument doc(1024);

  String uid;
  //std::string uid[5];
  unsigned int uidSize = 0;

  lcd.setCursor(0,0);
  lcd.print("nfc setup");

  while(digitalRead(buttonPins[0])){
    if (!mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial()) {
      for (byte i = 0; i < mfrc.uid.size; i++) {
        //uid[uidSize] = mfrc.uid.uidByte[i];
        uid = uid + String(mfrc.uid.uidByte[i]);
      }
      uidSize++;
    }
    doc["uid"] = uid;
  }

  File file = SPIFFS.open("/carddata.json", "w");
  serializeJson(doc, file);
  file.close();

  lcd.clear();
}

void readCardData(){
  File file = SPIFFS.open("/carddata.json", "r");

  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print("Failed to parse JSON file: ");
    Serial.println(error.c_str());
    return;
  }

  const char* uid = doc["uid"];

  Serial.println("uid: ");
  Serial.println(uid);

  file.close();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Wire.begin();
  mfrc.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc, Serial);
  lcd.init();
  lcd.backlight();

  pinMode(buttonPins[0], INPUT);

  btn1.attachClick(click1);
  btn1.attachLongPressStop(longPress1);
  btn2.attachClick(click2);
  // btn2.attachLongPressStop(longPress2);

  for(int i = 0; i < 2; i++) {
    if(!servos[i].attach(servosPins[i])) {
      Serial.print("Servo ");
      Serial.print(i);
      Serial.println("attach error");
    } else {
      servos[i].attach(servosPins[i]);
      servos[i].write(0);
    }
  }
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  /*
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi..");
    delay(1000);
  }

  Serial.println("Connected: ");
  Serial.println(WiFi.localIP());

  
  // Route for root / web page
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/app.js", "text/javascript");
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      digitalWrite(ledPin, inputMessage.toInt());
      ledState = !ledState;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(digitalRead(ledPin)).c_str());
  });
  // Start server
  server.begin();
  */

  cardSetup();
  readCardData();

  lcd.setCursor(0, 0);
  lcd.print("enter passcode:");
}
  
void loop() {

  if ( mfrc.PICC_IsNewCardPresent() || mfrc.PICC_ReadCardSerial()) {
    MFRC522Debug::PICC_DumpToSerial(mfrc, Serial, &(mfrc.uid));
    
  }

  btn1.tick();
  btn2.tick();
}