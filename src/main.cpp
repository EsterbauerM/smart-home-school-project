#include <Arduino.h>
#include <iostream>
#include <string>
//#include <WiFi.h>
//#include <AsyncTCP.h>
//#include <SPIFFS.h>
#include <Servo.h>
#include <Wire.h>
#include <analogWrite.h>
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522Debug.h>
#include <MFRC522DriverPinSimple.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <DHT_U.h>
#include "OneButton.h"
#include "pitches.h"

#include "wifi_credentials.h"

using namespace std;

const unsigned int 
 ledPin = 12,
 buttonPins[2] = {16,27},           // left, right button
 tempHumiSensPin = 17,            // temp. & humidity
 steamPin = 34,
 fanMPin = 18, fanPPin = 19, // fan Minus & Plus
 pirPin = 33,                // IS ON ANALOG PIN NOW  
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
String correct_p = "_";  //The correct password for the password door

const unsigned int passcards_maxAmount = 10;
String tempUid;
String passcards[passcards_maxAmount];  
unsigned int passcards_index = 0;
void cardSetup();

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

bool doorState = false;

AsyncWebServer server(80);

const char* PARAM_MESSAGE = "message";


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


int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

void playSong(){
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);

    //1.30 kleiner machen für weniger delay zwischen den noten
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    noTone(buzzerPin);
  }
}


void gas(){
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAS DETECTED!");
  servos[0].write(0);

  do{
    tone(buzzerPin, NOTE_A5, 250);
    delay(400);
    noTone(buzzerPin);
    delay(400);
  }while (!digitalRead(gasPin));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(doorState ? "Open" : "enter passcode:");
}


void click1(){
  if(!doorState){
    password = password + '.';
    lcd.setCursor(0,1);
    Serial.println(password);
    lcd.print(password);
  }
}

void longPress1(){
  if(!doorState){
    password = password + '_';
    lcd.setCursor(0, 1);
    Serial.println(password);
    lcd.print(password);
  }
}

void click2() {
  if(password == correct_p) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("passage granted");
    servos[1].write(180);  // Open door if psw is correct
    doorState = true;
    playSong();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (password != "")
      lcd.print("wrong.");
    servos[1].write(0);  // Make sure door is closed on wrong psw
    doorState = false;
  }
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(doorState ? "Open" : "enter passcode:");
  password = "";
}

bool keyExists(const String &str) {
  for (int i = 0; i < passcards_maxAmount; i++) {
    if (passcards[i] == str) {
      return true;
    }
  }
  return false;
}

void addToStringArray(const String &str) {
  if (passcards_index < passcards_maxAmount) {
    passcards[passcards_index] = str;
    passcards_index++;
    return;
  }
}

/*
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
*/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Wire.begin();
  mfrc.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc, Serial);
  lcd.init();
  lcd.backlight();

  pinMode(steamPin, INPUT);

  pinMode(ledPin, OUTPUT);
  pinMode(pirPin, INPUT);
  pinMode(gasPin, INPUT);

  btn1.attachClick(click1);
  btn1.attachLongPressStop(longPress1);
  btn2.attachClick(click2);

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
  /*
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  
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
  
  lcd.setCursor(0, 0);
  lcd.print("nfc setup");
  lcd.setCursor(0, 1);
  lcd.print("Done? Press btn");

  cardSetup();
  //readCardData();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("enter passcode:");
}

void cardSetup(){
  while (digitalRead(buttonPins[0])){
    if ( mfrc.PICC_IsNewCardPresent() || mfrc.PICC_ReadCardSerial()) {
      for (byte i = 0; i < mfrc.uid.size; i++) {
        tempUid = tempUid + mfrc.uid.uidByte[i];
      }
      
      if(!keyExists(tempUid)){
        Serial.println(tempUid);
        addToStringArray(tempUid);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("+ key");
        delay(1000);
        lcd.clear();
        lcd.print("nfc setup");
        lcd.setCursor(0, 1);
        lcd.print("Done? Press btn");
      }
    }
    tempUid = "";
  }
  Serial.println("==");
  for(int i=0; i<passcards_index; i++){
    Serial.println(passcards[i]);
  }
  Serial.println("==");
  delay(1000);
}
  
void loop() {
  if ( mfrc.PICC_IsNewCardPresent() || mfrc.PICC_ReadCardSerial()) {
    tempUid = "";
    for (byte i = 0; i < mfrc.uid.size; ++i) {
      tempUid = tempUid + mfrc.uid.uidByte[i];
    }

    Serial.println(tempUid);
    if(keyExists(tempUid) && !doorState){
      servos[1].write(180);
      lcd.clear();
      lcd.print("door open");
      doorState = true;
      //playSong();

    } else if (!keyExists(tempUid)){
      lcd.clear();
      lcd.print("wrong key");
      servos[1].write(0);
      delay(1000);
      lcd.clear();
      lcd.print("enter passcode:");
      doorState = false;
    }
    Serial.println("in array:");
    for(int i=0; i<passcards_index; i++){
      Serial.println(passcards[i]);
    }
    Serial.println("==");
  }

  int water_val = analogRead(steamPin);
  if(water_val > 3000){
    if(servos[0].attached()){
      servos[0].attach(5);
    }
    servos[0].write(0);
  }else 
    servos[0].write(112);
  

  int pir_val = analogRead(pirPin);
  if(pir_val > 1500)
    digitalWrite(ledPin, HIGH);
  else
    digitalWrite(ledPin, LOW);

  int gasVal = digitalRead(gasPin);
  boolean gasDetected = false;
  
  if(!gasVal){
    gas();
  }

  btn1.tick();
  btn2.tick();
}


/*
Temperatur abhäging maybe??

#include <LiteLED.h>
//#define LED_TYPE        LED_STRIP_WS2812
#define LED_TYPE        LED_STRIP_SK6812

#define LED_TYPE_IS_RGBW 0   // if the LED is an RGBW type, change the 0 to 1

#define LED_GPIO 26     // change this number to be the GPIO pin connected to the LED

#define LED_BRIGHT 10   // sets how bright the LED is. O is off; 255 is burn your eyeballs out (not recommended)

static const crgb_t L_RED = 0xff0000;
static const crgb_t L_GREEN = 0x00ff00;
static const crgb_t L_BLUE = 0x0000ff;
static const crgb_t L_WHITE = 0xe0e0e0;

LiteLED myLED( LED_TYPE, LED_TYPE_IS_RGBW );    // create the LiteLED object; we're calling it "myLED"

void setup() {
    myLED.begin( LED_GPIO, 4 );         // initialze the myLED object. Here we have 1 LED attached to the LED_GPIO pin
    myLED.brightness( LED_BRIGHT );     // set the LED photon intensity level
    myLED.setPixel(0, L_WHITE, 1 );    // set the LED colour and show it
    myLED.setPixel(1, L_WHITE, 1 );    // set the LED colour and show it
    myLED.setPixel(2, L_WHITE, 1 );    // set the LED colour and show it
    myLED.setPixel(3, L_WHITE, 1 );    // set the LED colour and show it
}

void loop() {
    myLED.brightness( LED_BRIGHT, 1 );
} 
*/