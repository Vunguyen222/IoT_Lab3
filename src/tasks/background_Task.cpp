
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
