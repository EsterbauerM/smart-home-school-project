#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <SPIFFS.h>

#include "wifi_credentials.h"

#define LED_PIN 18
#define BUTTON_LED 0

AsyncWebServer server(80);

int led_state = LOW;    // the current state of LED
int button_state;       // the current state of button
int last_button_state;

void setup()
{
  Serial.begin(115200);
  Serial.println("Booted");

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_LED, INPUT_PULLUP);

  if (!WiFi.softAP(SSID, PASSWORD)) {
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");

  SPIFFS.begin();

  MDNS.begin("demo-server");

  pinMode(LED_PIN, OUTPUT);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.addHandler(new AsyncCallbackJsonWebHandler("/led", [](AsyncWebServerRequest *request, JsonVariant &json) {
    const JsonObject &jsonObj = json.as<JsonObject>();
    if (jsonObj["on"])
    {
      Serial.println("Turn on LED");
      digitalWrite(LED_PIN, HIGH);
    }
    else
    {
      Serial.println("Turn off LED");
      digitalWrite(LED_PIN, LOW);
    }
    request->send(200, "OK");
  }));

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      Serial.println("Not found");
      request->send(404, "Not found");
    }
  });

  server.begin();
}

void loop()
{
  // put your main code here, to run repeatedly:
}