#include <Arduino.h>
#include <BME280I2C.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>

const char *ssid = "BaconNet";
const char *password = "5AlandaleClose//";
const char *heaterHost = "192.168.68.200";
const int heaterPort = 80;
const int temperatureCheckDelay = 600000;

BME280I2C::Settings settings(
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::Mode_Forced,
    BME280::StandbyTime_1000ms,
    BME280::Filter_Off,
    BME280::SpiEnable_False,
    BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
);
BME280I2C bme(settings);
ESP8266WebServer server(80);

float temp(NAN), hum(NAN), pres(NAN);
float targetTemperature = 0.0;
ulong lastTemperatureCheck = 0;
bool heating = false;

void endpointBoost()
{
}

void endpointNotFound()
{
    server.send(404, "text/plain", "Not found");
}

void endpointSetTemperature()
{
    if (server.hasArg("target"))
    {
        targetTemperature = server.arg("target").toFloat();
        lastTemperatureCheck = 0;
        server.send(201);
    }
    else
    {
        server.send(400);
    }
}

void endpointStatus()
{
    String jsonResult = "{\n";
    jsonResult += "\"target_temperature\": ";
    jsonResult += targetTemperature;
    jsonResult += ",\n";
    jsonResult += "\"actual_temperature\": ";
    jsonResult += temp;
    jsonResult += ",\n";
    jsonResult += "\"humidity\": ";
    jsonResult += hum;
    jsonResult += ",\n";
    jsonResult += "\"heating\": ";
    jsonResult += heating ? "true" : "false";
    jsonResult += "\n}";

    server.send(200, "application/json", jsonResult);
}

void setHeatingState(bool state)
{
    String url = state ? "/on" : "/off";
    HTTPClient http;
    http.begin(heaterHost, heaterPort, url);
    int httpCode = http.GET();
    String success = "false";

    if (httpCode == 201)
    {
        heating = state;
    }
}

void checkTemperature()
{
    if (lastTemperatureCheck > 0 && millis() - lastTemperatureCheck < temperatureCheckDelay)
    {
        return;
    }

    lastTemperatureCheck = millis();
    Serial.println("Checking temperature.");

    if (!heating && targetTemperature > temp)
    {
        Serial.println("Starting heating.");
        setHeatingState(true);
    }
    else if (heating && targetTemperature < temp)
    {
        Serial.println("Stop heating.");
        setHeatingState(false);
    }
    else
    {
        Serial.println("No action needed. Next check in 10 minutes.");
    }
}

void readSensor()
{
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_hPa);

    bme.read(pres, temp, hum, tempUnit, presUnit);

    // Only one decimal place for temperature
    temp = roundf(temp * 10) / 10;

    // No decimal places for humidity
    hum = truncf(hum);
}

void setup()
{
    Serial.begin(9600);
    delay(100);

    Serial.println("Searching for BME280I2C sensor");
    Wire.begin();

    while (!bme.begin())
    {
        Serial.print(".");
        delay(1000);
    }

    switch (bme.chipModel())
    {
    case BME280::ChipModel_BME280:
        Serial.println("Found BME280 sensor! Success.");
        break;
    case BME280::ChipModel_BMP280:
        Serial.println("Found BMP280 sensor! No Humidity available.");
        break;
    default:
        Serial.println("Found UNKNOWN sensor! Error!");
    }

    // Change some settings before using.
    settings.tempOSR = BME280::OSR_X4;

    bme.setSettings(settings);

    Serial.println("Connecting to ");
    Serial.println(ssid);

    //connect to your local wi-fi network
    WiFi.begin(ssid, password);

    //check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected..!");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());

    server.on("/api/status", endpointStatus);
    server.on("/api/temperature", endpointSetTemperature);
    server.on("/api/boost", endpointBoost);
    server.onNotFound(endpointNotFound);

    server.begin();
    Serial.println("HTTP server started");

    setHeatingState(false);
}

void loop()
{
    readSensor();
    checkTemperature();
    server.handleClient();
}
