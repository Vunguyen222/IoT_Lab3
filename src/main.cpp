#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "DHT.h"
#include "Shared_Attribute_Callback.h"
#include "Shared_Attribute_Update.h"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
// const char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";

// use to connect wifi
const char WIFI_SSID[] = "Galaxy A327A68";
const char WIFI_PASSWORD[] = "quangvu8888";

// use to connect thingsboard
const char THINGSBOARD_SERVER[] = "app.coreiot.io";
const uint16_t THINGSBOARD_PORT = 1883;
const char TOKEN[] = "q8hm31kl8fvuv1vhzbzr";

// define max size of the message
const uint32_t MAX_MESSAGE_SIZE = 1024;

// define object instantiation to communicate with thingsboar server
WiFiClient wifiClient; 
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard thingsBoard(mqttClient, MAX_MESSAGE_SIZE);

// DHT20 dht20;

void initWifi()
{
  //establish a connection to wifi
  Serial.println("Start connecting to Access Point");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  while(WiFi.status() != WL_CONNECTED){
    // waiting until connecting to wifi successfully
    delay(500);
    Serial.print("."); 
  }
  Serial.println("Connected to Access Point");
}

void taskReconnectWifi(void *pvParameters){
  (void)pvParameters; 

  while(true){
    if(WiFi.status() != WL_CONNECTED){
      // establish a connection
      initWifi();
    }
    else Serial.println("Wifi already connected"); 
    // check every 10s
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void taskConnectCoreIoT(void* pvParameters){
  (void)pvParameters; 
  while(true){
    if(!thingsBoard.connected()){
      if (!thingsBoard.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT))
      {
        Serial.println("Failed to connect");
      }
      else 
        Serial.println("Connecting CoreIoT Successfully");
    }
    else 
      Serial.println("CoreIoT already connected"); 
    vTaskDelay(8000 / portTICK_PERIOD_MS);
  }
}


void sendDatatoTelemetry(void* pvParameters){
  (void)pvParameters; 
  while(true){

    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
}

// void readSensor(void* pvParameters){
//   (void)pvParameters; 
//   while(true){
//     dht20.read();

//     // float temperature = dht20.getTemperature();
//     // float humidity = dht20.getHumidity();
//     float temperature = 25;
//     float humidity = 55;
// switch (status)
// {
//   case DHT20_OK:
//   Serial.println("OK");
//   // String payload = "{\"temperature\": " + String(temperature) +
//   //                  ", \"humidity\": " + String(humidity) + "}";
//   // thingsBoard.
// if(thingsBoard.sendTelemetryData("temperature", temperature)){
//   Serial.println("Send data success");
// }
//   // thingsBoard.sendTelemetryData("humidity", humidity);
//   break;
// case DHT20_ERROR_CHECKSUM:
//   Serial.println("Checksum error");
//   break;
// case DHT20_ERROR_CONNECT:
//   Serial.println("Connect error");
//   break;
// case DHT20_MISSING_BYTES:
//   Serial.println("Missing bytes");
//   break;
// case DHT20_ERROR_BYTES_ALL_ZERO:
//   Serial.println("All bytes read zero");
//   break;
// case DHT20_ERROR_READ_TIMEOUT:
//   Serial.println("Read time out");
//   break;
// case DHT20_ERROR_LASTREAD:
//   Serial.println("Read too fast");
//   break;
// default:
//   Serial.println("Unknown error");
//   break;
// }
//     if (isnan(temperature) || isnan(humidity)) {
//       Serial.println("Failed to read from DHT20 sensor!");
//     } else {
//       Serial.print("Temperature: ");
//       Serial.print(temperature);
//       Serial.print(" °C, Humidity: ");
//       Serial.print(humidity);
//       Serial.println(" %");
//     }
//     vTaskDelay(2000 / portTICK_PERIOD_MS); 
//   }
// }

void taskReadSensor(void *pvParameters)
{
  while (1)
  {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if(isnan(temperature) || isnan(humidity)) {
      Serial.println("Error: Can not read data from DHT !");
    }
    else {

      Serial.print("DHT20 Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");

      Serial.print("DHT20 Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      bool success = thingsBoard.sendTelemetryData("temperature", temperature) 
                    && thingsBoard.sendTelemetryData("humidity", humidity);
      if(success)
        Serial.println("Send data to CoreIoT successfully");
    }
    Serial.println(); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello, World from ESP32!");
  // Wire.begin();
  // dht20.begin();
  dht.begin(); 
  // initWifi();
  xTaskCreate(taskReconnectWifi, "reconnectWifi", 4096, NULL, 1, NULL);
  xTaskCreate(taskConnectCoreIoT, "connectCoreIoT", 4096, NULL, 1, NULL);
  xTaskCreate(taskReadSensor, "readSensor", 2048, NULL, 1, NULL);
}

void loop()
{
}
