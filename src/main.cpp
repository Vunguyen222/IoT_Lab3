#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <Shared_Attribute_Update.h>
#include "DHT20.h"
#include "DHT.h"

#define LEDPIN 5
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// use to connect wifi
const char WIFI_SSID[] = "Cutom 2.4Ghz";
const char WIFI_PASSWORD[] = "Cutom0104";

// use to connect thingsboard
const char THINGSBOARD_SERVER[] = "app.coreiot.io";
const uint16_t THINGSBOARD_PORT = 1883;
const char TOKEN[] = "q8hm31kl8fvuv1vhzbzr";

// define max size of the message
const uint32_t MAX_MESSAGE_SIZE = 1024;

// parameter to subscribe shared attribute
// declare function
void processSharedAttributeUpdate(const JsonObjectConst &data);

constexpr uint8_t MAX_SHARED_ATTR = 5U;
Shared_Attribute_Update<1U, MAX_SHARED_ATTR> sharedAttributeUpdate;

const char ledState[] = "turnOn";
constexpr std::array<const char *, 1U> SUBSCRIBED_SHARED_ATTRIBUTES = {ledState};
const Shared_Attribute_Callback<MAX_SHARED_ATTR> callback(&processSharedAttributeUpdate, SUBSCRIBED_SHARED_ATTRIBUTES);

// RPC callback
void processGetValidationVal(const JsonVariantConst &data, JsonDocument &response);

const char RPC_TRUE_METHOD[] = "rpcTrueCommand";
const char RPC_FALSE_METHOD[] = "rpcFalseCommand";

constexpr uint8_t MAX_RPC_RESPONSE = 5U;
Server_Side_RPC<2U, MAX_RPC_RESPONSE> rpc;

const std::array<RPC_Callback, 2U> RPCcallbacks = {
    // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
    RPC_Callback{RPC_TRUE_METHOD, processGetValidationVal},
    RPC_Callback{RPC_FALSE_METHOD, processGetValidationVal},
};

const std::array<IAPI_Implementation *, 2U> apis = {
    &sharedAttributeUpdate,
    &rpc,
};
// define object instantiation to communicate with thingsboar server
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard thingsBoard(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

DHT20 dht20;
bool sharedAttrSubscribed = false;
bool RPCSubscribed = false;

void initWifi()
{
  // establish a connection to wifi
  Serial.println("Start connecting to Access Point");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    // waiting until connecting to wifi successfully
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Access Point");
}

void taskReconnectWifi(void *pvParameters)
{
  (void)pvParameters;

  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      // establish a connection
      initWifi();
    }
    else
      Serial.println("Wifi already connected");
    // check every 10s
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void taskConnectCoreIoT(void *pvParameters)
{
  (void)pvParameters;
  while (true)
  {
    if (!thingsBoard.connected())
    {
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

void taskSubSharedAttr(void *pvParameters)
{
  (void)pvParameters;
  while (true)
  {
    if (!sharedAttrSubscribed)
    {
      if (!sharedAttributeUpdate.Shared_Attributes_Subscribe(callback))
      {
        Serial.println("Failed to subscribe for shared attribute updates");
        return;
      }
      Serial.println("Subscribe shared attribute done");
      sharedAttrSubscribed = true;
    }
    vTaskDelay(9000 / portTICK_PERIOD_MS);
  }
}

void taskSubRPCCallback(void *pvParameters)
{
  (void)pvParameters;
  while (true)
  {
    if (!RPCSubscribed)
    {
      Serial.println("Subscribing for RPC...");
      if (!rpc.RPC_Subscribe(RPCcallbacks.cbegin(), RPCcallbacks.cend()))
      {
        Serial.println("Failed to subscribe for RPC");
        return;
      }

      Serial.println("Subscribe RPC done");
      RPCSubscribed = true;
    }
    vTaskDelay(9000 / portTICK_PERIOD_MS);
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
    float temperature = 100;
    // float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity))
    {
      Serial.println("Error: Can not read data from DHT !");
    }
    else
    {

      Serial.print("DHT20 Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");

      Serial.print("DHT20 Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      StaticJsonDocument<256> jsonBuffer;
      jsonBuffer["temperature"] = temperature;
      jsonBuffer["humidity"] = humidity;

      bool sent = thingsBoard.sendTelemetryJson(jsonBuffer, measureJson(jsonBuffer));
      if (sent)
      {
        Serial.println("Send data successfully");
      }
    }
    Serial.println();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  // Wire.begin();
  // dht20.begin();
  dht.begin();
  initWifi();
  delay(2000);
  xTaskCreate(taskReconnectWifi, "reconnectWifi", 4096, NULL, 1, NULL);
  xTaskCreate(taskConnectCoreIoT, "connectCoreIoT", 4096, NULL, 1, NULL);
  xTaskCreate(taskSubSharedAttr, "subSharedAttr", 2048, NULL, 1, NULL);
  xTaskCreate(taskSubRPCCallback, "taskSubRPCCallback", 2048, NULL, 1, NULL);
  xTaskCreate(taskReadSensor, "readSensor", 2048, NULL, 1, NULL);
}

void loop()
{
  thingsBoard.loop();
  delay(1000);
}

bool turnOn = false;
void processSharedAttributeUpdate(const JsonObjectConst &data)
{
  for (auto it = data.begin(); it != data.end(); ++it)
  {
    Serial.println(it->key().c_str());
    if (strcmp(it->key().c_str(), "turnOn") == 0)
    {
      turnOn = it->value().as<bool>();
      digitalWrite(LEDPIN, turnOn);

      const size_t jsonSize = Helper::Measure_Json(data);
      char buffer[jsonSize];
      serializeJson(data, buffer, jsonSize);
      Serial.println(buffer);

      if (turnOn)
        Serial.println("turn on the led");
      else
        Serial.println("turn off the led");
    }
  }
}

void processGetValidationVal(const JsonVariantConst &data, JsonDocument &response)
{
  // Process data
  const bool isValid = data["isValid"];
  if (!isValid)
    Serial.println("DHT20 data is not valid");
  else
    Serial.println("DHT20 data is valid");

  // const size_t jsonSize = Helper::Measure_Json(data);
  // char buffer[jsonSize];
  // serializeJson(data, buffer, jsonSize);
  // Serial.println(buffer);
  // Ensure to only pass values do not store by copy, or if they do increase the MaxRPC
  // template parameter accordingly to ensure that the value can be deserialized.RPC_Callback.
  response["isValid"] = isValid;
}