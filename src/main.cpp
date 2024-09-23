#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "mqtt_client.h"
#include <DHT.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// Define the pins for the sensors
#define SOIL_MOISTURE_PIN A0
#define DHT_PIN 2
#define DHT_TYPE DHT22

// Initialize the DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Variables to store sensor data
float soilMoisture;
float temperature;
float humidity;

const char *ssid = "OPPO_636C83_2.4G";
const char *password = "et9m4JH9";

// MQTT Broker
const char *mqttBroker = "your_mqtt_broker";

// OpenWeather API credentials
const char *openWeatherApiKey = getenv("OPENWEATHER_API_KEY");
const char *city = "Johannesburg";
const char *country = "ZA";

// OpenWeather API URL
String openWeatherUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "," + String(country) + "&appid=" + String(openWeatherApiKey) + "&units=metric";

// Sentinel Hub API credentials
const char *sentinelHubApiKey = getenv("SENTINEL_HUB_CLIENT_SECRET");

// Sentinel Hub API URL
String sentinelHubUrl = "https://services.sentinel-hub.com/api/v1/process";

// Arduino IoT Cloud variables
float cloudSoilMoisture;
float cloudTemperature;
float cloudHumidity;

void initProperties()
{
  ArduinoCloud.addProperty(cloudSoilMoisture, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(cloudTemperature, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(cloudHumidity, READ, ON_CHANGE, NULL);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(ssid, password);

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(1500);

  // Initialize the DHT sensor
  dht.begin();

  // Initialize the MQTT client
  initMQTTClient();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Initialize Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  initProperties();
}

void loop()
{
  ArduinoCloud.update();

  // Read data from the soil moisture sensor
  soilMoisture = analogRead(SOIL_MOISTURE_PIN);

  // Read data from the DHT sensor
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Print the sensor data to the serial monitor
  Serial.print("Soil Moisture: ");
  Serial.println(soilMoisture);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);

  // Update cloud variables
  cloudSoilMoisture = soilMoisture;
  cloudTemperature = temperature;
  cloudHumidity = humidity;

  // Fetch weather data from OpenWeather API
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(openWeatherUrl);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON response
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      float weatherTemp = doc["main"]["temp"];
      float weatherHumidity = doc["main"]["humidity"];
      String weatherDescription = doc["weather"][0]["description"];

      // Print weather data to the serial monitor
      Serial.print("Weather Temperature: ");
      Serial.println(weatherTemp);
      Serial.print("Weather Humidity: ");
      Serial.println(weatherHumidity);
      Serial.print("Weather Description: ");
      Serial.println(weatherDescription);

      // Example data to send
      const char *topic = "sensor/data";
      String mqttPayload = "{\"soilMoisture\": " + String(soilMoisture) + ", \"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + ", \"weatherTemp\": " + String(weatherTemp) + ", \"weatherHumidity\": " + String(weatherHumidity) + ", \"weatherDescription\": \"" + weatherDescription + "\"}";

      // Send data to the cloud
      sendDataToCloud(topic, mqttPayload.c_str());
    }
    else
    {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }

  // Wait for 10 seconds before sending the next data
  delay(10000);
}