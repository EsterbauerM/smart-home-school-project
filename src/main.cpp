#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "ezButton.h"

#include "wifi_credentials.h"

const int ledPin = 2;
ezButton button(0);

String ledState;
String buttonState;

AsyncWebServer server(80);

String processor(const String& var){
  Serial.println("processor: "+var);
  if(var == "LED_STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }

  if(var == "BUTTON_STATE"){
    if(button.isPressed())
      buttonState = "pressed";

    else
      buttonState = " ";

    Serial.println(buttonState);
    return buttonState;
  }
  return String();
}
 
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH or LOW
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.begin();

  button.setDebounceTime(40);
}
 
void loop(){
  button.loop();

  if(button.isPressed()){
    Serial.println("button pressed");
    Serial.println(ledState);
    ledState= ledState=="OFF"? "ON" : "OFF";
    digitalWrite(ledPin, (ledState=="OFF"? LOW : HIGH));
  }
}