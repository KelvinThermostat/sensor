#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <AsyncElegantOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "networkclient.h"

const unsigned long DEFAULT_READ_DELAY = 30000;
unsigned long _lastSensorReadCheck = 0;
float _temperature = 0;
float _pressure = 0;
byte _humidity = 0;

Adafruit_BME280 bme;
AsyncWebServer server(80);
NetworkClient net;

void endpointNotFound(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", F("Not found"));
}

void readSensor()
{
    if (_lastSensorReadCheck > 0 && (millis() - _lastSensorReadCheck) < DEFAULT_READ_DELAY)
    {
        return;
    }

    _lastSensorReadCheck = millis();

    _temperature = bme.readTemperature();
    _humidity = bme.readHumidity();
    _pressure = bme.readPressure() / 100.0F;
}

double round1(double value)
{
    return (int)(value * 10 + 0.5) / 10.0;
}

void endpointStatus(AsyncWebServerRequest *request)
{
    String jsonResult = "{";
    jsonResult += "\"temperature\": ";
    jsonResult += round1(_temperature);
    jsonResult += ",";
    jsonResult += "\"humidity\": ";
    jsonResult += _humidity;
    jsonResult += ",";
    jsonResult += "\"pressure\": ";
    jsonResult += round1(_pressure);
    jsonResult += "}";

    request->send(200, "application/json", jsonResult);
}

void setup()
{
    const String mDnsHost = "kelvin-sensor-" + String(ESP.getChipId());

    Serial.begin(9600);

    net.connect();
    net.registerMdnsHost(mDnsHost);

    Serial.println(F("Searching for BME280I2C sensor"));

    if (!bme.begin(0x76))
    {
        Serial.println(F("Could not find a valid BME280 sensor"));
        while (1)
            delay(10);
    }
    else
    {
        Serial.println(F("BME280 sensor connected"));
    }

    server.on("/api/status", HTTP_GET, endpointStatus);
    server.onNotFound(endpointNotFound);

    AsyncElegantOTA.begin(&server);
    server.begin();

    Serial.println(F("HTTP server started"));
}

void loop()
{
    net.check();

    readSensor();
}
