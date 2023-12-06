#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <SPIFFS.h>

#include "wifi_credentials.h"

#define BUILTIN_LED 18

AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);
  Serial.println("Booted");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting....");
    delay(5000);
    ESP.restart();
  }

  SPIFFS.begin();

  MDNS.begin("homeserver");

  Serial.print("Wifi connected, IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(BUILTIN_LED, OUTPUT);

  File file = SPIFFS.open("/test.txt");
  if(!file){
    Serial.println("Failed to open file!");
    return;
  }

  Serial.println("Content of file:");
  Serial.write(file.read());
  file.close();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.addHandler(new AsyncCallbackJsonWebHandler("/led", [](AsyncWebServerRequest *request, JsonVariant &json) {
    const JsonObject &jsonObj = json.as<JsonObject>();
    if (jsonObj["on"])
    {
      Serial.println("Turn on LED");
      digitalWrite(BUILTIN_LED, HIGH);
    }
    else
    {
      Serial.println("Turn off LED");
      digitalWrite(BUILTIN_LED, LOW);
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