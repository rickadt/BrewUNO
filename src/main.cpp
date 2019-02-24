#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP_PLATFORM)
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#endif

#include <FS.h>
#include <WiFiSettingsService.h>
#include <WiFiStatus.h>
#include <WiFiScanner.h>
#include <APSettingsService.h>
#include <NTPSettingsService.h>
#include <NTPStatus.h>
#include <OTASettingsService.h>
#include <APStatus.h>

#include <PID_v1.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <MashSettingsService.h>
#include <BoilSettingsService.h>
#include <BrewSettingsService.h>
#include <BrewService.h>
#include <MashService.h>
#include <BoilService.h>
#include <TemperatureService.h>
#include <KettleHeaterService.h>
#include <ActiveStatus.h>

#define ONE_WIRE_BUS D6
#define SERIAL_BAUD_RATE 9600

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

AsyncWebServer server(80);

WiFiSettingsService wifiSettingsService = WiFiSettingsService(&server, &SPIFFS);
APSettingsService apSettingsService = APSettingsService(&server, &SPIFFS);
NTPSettingsService ntpSettingsService = NTPSettingsService(&server, &SPIFFS);
OTASettingsService otaSettingsService = OTASettingsService(&server, &SPIFFS);

WiFiScanner wifiScanner = WiFiScanner(&server);
WiFiStatus wifiStatus = WiFiStatus(&server);
NTPStatus ntpStatus = NTPStatus(&server);
APStatus apStatus = APStatus(&server);

// biabrewEx
TemperatureService temperatureService = TemperatureService(DS18B20);
BrewSettingsService brewSettingsService = BrewSettingsService(&server, &SPIFFS);
MashSettingsService mashSettings = MashSettingsService(&server, &SPIFFS);
BoilSettingsService boilSettingsService = BoilSettingsService(&server, &SPIFFS, &brewSettingsService);

ActiveStatus activeStatus = ActiveStatus(&SPIFFS);
KettleHeaterService kettleHeaterService = KettleHeaterService(&temperatureService);
MashService mashService = MashService(&SPIFFS, &temperatureService);
BoilService boilService = BoilService(&SPIFFS, &temperatureService);
BrewService brewService = BrewService(&server, &SPIFFS, &mashService, &boilService, &brewSettingsService, &kettleHeaterService, &activeStatus);

//java -jar EspStackTraceDecoder.jar C:\Users\bruno\.platformio\packages\toolchain-xtensa\bin\xtensa-lx106-elf-addr2line.exe G:\Projetos\biabrewex\.pioenvs\esp12e\firmware.elf exeption.txt
//\.platformio\packages\framework-arduinoespressif8266\tools\sdk\lwip\include\lwipopts.h
//#define TCP_LISTEN_BACKLOG              1

void setup()
{
  // Disable wifi config persistance
  WiFi.persistent(false);

  Serial.begin(SERIAL_BAUD_RATE);
  SPIFFS.begin();
  //SPIFFS.format();

  // start services
  ntpSettingsService.begin();
  otaSettingsService.begin();
  apSettingsService.begin();
  wifiSettingsService.begin();

  brewSettingsService.begin();

  brewService.begin();

  // Serving static resources from /www/
  server.serveStatic("/js/", SPIFFS, "/www/js/");
  server.serveStatic("/css/", SPIFFS, "/www/css/");
  server.serveStatic("/fonts/", SPIFFS, "/www/fonts/");
  server.serveStatic("/app/", SPIFFS, "/www/app/");
  server.serveStatic("/favicon.ico", SPIFFS, "/www/favicon.ico");

  // Serving all other get requests with "/www/index.htm"
  // OPTIONS get a straight up 200 response
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_GET)
    {
      request->send(SPIFFS, "/www/index.html");
    }
    else if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      request->send(404);
    }
  });

// Disable CORS if required
#if defined(ENABLE_CORS)
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
#endif

  server.begin();

  pinMode(D5, OUTPUT);
  mashService.TurnPumpOff();
}

void loop()
{
  apSettingsService.loop();
  ntpSettingsService.loop();
  otaSettingsService.loop();

  brewService.loop();
}