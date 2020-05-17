#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <BME280I2C.h>
#include <Wire.h>

float temperature, humidity, pressure;
const char *ssid = "BaconNet";
const char *password = "5AlandaleClose//";

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

String sendData(float temperature, float humidity, float pressure)
{
    String ptr = "{\n";
    ptr += "\"temperature\": ";
    ptr += temperature;
    ptr += ",\n";
    ptr += "\"humidity\": ";
    ptr += humidity;
    ptr += ",\n";
    ptr += "\"pressure\": ";
    ptr += pressure;
    ptr += "\n}";

    return ptr;
}

void handle_OnConnect()
{
    float temp(NAN), hum(NAN), pres(NAN);

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_hPa);

    bme.read(pres, temp, hum, tempUnit, presUnit);

    server.send(200, "application/json", sendData(temp, hum, pres));
}

void handle_NotFound()
{
    server.send(404, "text/plain", "Not found");
}

void setup()
{
    Serial.begin(9600);
    delay(100);

    while (!Serial)
    {
    } // Wait

    Wire.begin();
    while (!bme.begin())
    {
        Serial.println("Could not find BME280I2C sensor!");
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

    server.on("/", handle_OnConnect);
    server.onNotFound(handle_NotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    server.handleClient();
}
