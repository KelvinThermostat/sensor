#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WebServer.h>
#include <Wire.h>

float temperature, humidity, pressure;
const char *ssid = "BaconNet";
const char *password = "5AlandaleClose//";

Adafruit_BME280 bme;
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
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;

  server.send(200, "application/json", sendData(temperature, humidity, pressure));
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  bme.begin(0x76);

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
