#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <Shared_Attribute_Update.h>
#include <Attribute_Request.h>
#include <OTA_Firmware_Update.h>
#include <Preferences.h>
#include <Espressif_Updater.h>
#include "esp_ota_ops.h"
#include "DHT20.h"
#include "DHT.h"

#define LEDPIN 5
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// use to connect wifi
const char WIFI_SSID[] = "Cutom 2.4Ghz";
const char WIFI_PASSWORD[] = "Cutom0104";
// const char WIFI_SSID[] = "ACLAB";
// const char WIFI_PASSWORD[] = "ACLAB2023";
// const char WIFI_SSID[] = "Hoang";
// const char WIFI_PASSWORD[] = "15220903";

// use to connect thingsboard
const char THINGSBOARD_SERVER[] = "app.coreiot.io";
const uint16_t THINGSBOARD_PORT = 1883;
const char TOKEN[] = "q8hm31kl8fvuv1vhzbzr";

// define max size of the message
const uint32_t MAX_MESSAGE_SIZE = 1024;

// SHARED ATTRIBUTE
// declare function
void processSharedAttributeUpdate(const JsonObjectConst &data);

constexpr uint8_t MAX_SHARED_ATTR = 5U;
Shared_Attribute_Update<1U, MAX_SHARED_ATTR> sharedAttributeUpdate;

const char ledState[] = "turnOn";
constexpr std::array<const char *, 2U> SUBSCRIBED_SHARED_ATTRIBUTES = {ledState};
const Shared_Attribute_Callback<MAX_SHARED_ATTR> callback(&processSharedAttributeUpdate, SUBSCRIBED_SHARED_ATTRIBUTES);

// REQUEST ATTRIBUTE
const char fwVersion[] = "fw_version";
uint64_t REQUEST_TIMEOUT_MICROSECONDS = 5000000ULL;
void requestTimedOut() {}

const std::vector<const char *> REQUESTED_CLIENT_ATTRIBUTES = {fwVersion};
Attribute_Request<2U, 5U> attr_request;
const Attribute_Request_Callback<5U> clientCallback(&processSharedAttributeUpdate, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut, REQUESTED_CLIENT_ATTRIBUTES);

// RPC CALLBACK
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

// OTA UPDATE
void update_starting_callback();
void finished_callback(const bool &success);
void progress_callback(const size_t &current, const size_t &total);

Espressif_Updater<> updater;

const char CURRENT_FIRMWARE_TITLE[] = "OTA for ESP32";
char CURRENT_FIRMWARE_VERSION[32];
char LASTEST_FIRMWARE_VERSION[32];

// Maximum amount of retries we attempt to download each firmware chunck over MQTT
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
// Size of each firmware chunck downloaded over MQTT,
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;

OTA_Firmware_Update<> ota;

const std::array<IAPI_Implementation *, 4U> apis = {
    &sharedAttributeUpdate,
    &rpc,
    &ota,
    &attr_request,
};

// define object instantiation to communicate with thingsboar server
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard thingsBoard(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

DHT20 dht20;
bool sharedAttrSubscribed = false;
bool RPCSubscribed = false;
bool currentFWSent = false;
bool updateRequestSent = false;
bool requestedShared = false;
/* -----------------------------------------------HELPER FUNCTION-------------------------------------------- */
Preferences prefs;

void readVersion()
{
  prefs.begin("firmware", true); // true = read-only
  String version = prefs.getString("version", "unknown").c_str();
  strncpy(CURRENT_FIRMWARE_VERSION, version.c_str(), sizeof(CURRENT_FIRMWARE_VERSION));
  CURRENT_FIRMWARE_VERSION[strlen(CURRENT_FIRMWARE_VERSION) + 1] = '\0';
  prefs.end();
}

void writeVersion()
{
  prefs.begin("firmware", false); // false = read+write
  prefs.putString("version", String(LASTEST_FIRMWARE_VERSION));
  prefs.end();
}

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

/* ----------------------------------------------TASKS-------------------------------------------------------- */

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

    if (!currentFWSent)
    {
      currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
    }

    if (!updateRequestSent)
    {
      Serial.println("Firwmare Update");
      Serial.print("Current Firmware Version: ");
      Serial.println(CURRENT_FIRMWARE_VERSION);
      const OTA_Update_Callback callback(
          CURRENT_FIRMWARE_TITLE,
          CURRENT_FIRMWARE_VERSION,
          &updater,
          &finished_callback,
          &progress_callback,
          &update_starting_callback,
          FIRMWARE_FAILURE_RETRIES,
          FIRMWARE_PACKET_SIZE);
      updateRequestSent = ota.Start_Firmware_Update(callback);
    }
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

    if (!requestedShared)
    {
      // Shared attributes we want to request from the server
      requestedShared = attr_request.Shared_Attributes_Request(clientCallback);
      if (!requestedShared)
      {
        Serial.println("Failed to request shared attributes");
      }
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

void taskReadSensor(void *pvParameters)
{
  while (1)
  {
    float temperature = dht.readTemperature();
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

  delay(2000);
  dht.begin();
  initWifi();
  // prefs.begin("firmware", false); // false = read+write
  // prefs.putString("version", "1");
  // prefs.end();
  readVersion();
  delay(2000);

  xTaskCreate(taskReconnectWifi, "reconnectWifi", 4096, NULL, 1, NULL);
  xTaskCreate(taskConnectCoreIoT, "connectCoreIoT", 4096, NULL, 1, NULL);
  xTaskCreate(taskSubSharedAttr, "subSharedAttr", 2048, NULL, 1, NULL);
  // xTaskCreate(taskSubRPCCallback, "taskSubRPCCallback", 2048, NULL, 1, NULL);
  // xTaskCreate(taskReadSensor, "readSensor", 2048, NULL, 1, NULL);
}

void loop()
{
  thingsBoard.loop();
  delay(1000);
}

/* -----------------------------------------CALLBACK FUNCTION-------------------------------------------------- */

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

      if (turnOn)
        Serial.println("turn on the led");
      else
        Serial.println("turn off the led");
    }

    if (strcmp(it->key().c_str(), fwVersion) == 0)
    {
      strncpy(LASTEST_FIRMWARE_VERSION, it->value().as<String>().c_str(), sizeof(LASTEST_FIRMWARE_VERSION));
      LASTEST_FIRMWARE_VERSION[strlen(LASTEST_FIRMWARE_VERSION) + 1] = '\0';
      writeVersion();
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

  response["isValid"] = isValid;
}

void update_starting_callback()
{
  // Nothing to do
}

void finished_callback(const bool &success)
{
  if (success)
  {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t &current, const size_t &total)
{
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}