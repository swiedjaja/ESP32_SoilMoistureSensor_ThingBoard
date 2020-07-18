/*
Function:
  - Sense soil moisture sensor with capacitive sensing method
  - Read temperature and humidity from DHT11 or DHT22
  - Read lux from TSL2561
  - Connect to ThingBoard
  - Read DeepSleep Period from ThingBoard
  - Send to Thingboard
  - Going to DeepSleep for 1 - 10 minutes

Board Name: "WeMos" WiFi & Bluetooth Battery or generic esp32
References: https://www.instructables.com/id/ESP32-WiFi-SOIL-MOISTURE-SENSOR/
github: https://github.com/JJSlabbert/Higrow-ESP32-WiFi-Soil-Moisture-Sensor

Library used:
- DHTEsp
- ThingBoard

  Progress:
*/

#include <Arduino.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <ThingsBoard.h>
#include "ntputils.h"
#include "wifi_id.h"

#define PIN_SOIL_MOISTURE_SENSOR 32
#define PIN_DHT 22

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  (30)        /* Time ESP32 will go to sleep (in seconds) */

#define THINGSBOARD_TOKEN   "bJAnSmrowVEKYTg06MXE"
// ThingsBoard server instance.
#define THINGSBOARD_SERVER  "demo.thingsboard.io"

DHTesp dht;
WiFiClient espClient;
ThingsBoard tb(espClient);

// Set to true if application is subscribed for the RPC messages.
bool fThingsBoardSubscribed = false;

RTC_DATA_ATTR int bootCount = 0; // if hardware reset occur, bootCount will set to 0
RTC_DATA_ATTR int nDeepSleepInterval=60;
float g_SoilMoist=0;

void print_wakeup_reason();
void ReadSensor();

/*
// Processes function for RPC call "setValue"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processDelayChange(const RPC_Data &data)
{
  Serial.println("Received the set delay RPC method");

  // Process data

  nDeepSleepInterval = data;

  Serial.print("Set new delay: ");
  Serial.println(nDeepSleepInterval);

  return RPC_Response(NULL, nDeepSleepInterval);
}

// Processes function for RPC call "getValue"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processGetDelay(const RPC_Data &data)
{
  Serial.println("Received the get value method");

  return RPC_Response(NULL, nDeepSleepInterval);
}

// RPC handlers
RPC_Callback callbacks[] = {
  { "setValue", processDelayChange },
  { "getValue", processGetDelay },
};
*/

inline float ReadSoilMoistureSensor()
{ 
  g_SoilMoist = (0.95*g_SoilMoist+0.05*analogRead(PIN_SOIL_MOISTURE_SENSOR));
  return g_SoilMoist;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  dht.setup(PIN_DHT, DHTesp::DHT11);
  g_SoilMoist = analogRead(PIN_SOIL_MOISTURE_SENSOR);
  delay(100);
  Serial.println("Boot number: " + String(bootCount++));

  //Print the wakeup reason for ESP32
  // print_wakeup_reason();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  NtpInit();
  Serial.print("System connected with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print(getFormattedDateTime()+"\t");
  ReadSensor();
  for (int i=0; i<1000; i++)
  {
    tb.loop();
    delay(1);
  }

  // Serial.printf("DHT minimum sampling period: %d\n", dht.getMinimumSamplingPeriod()); // 1000 mS
  Serial.printf("System going to sleep for %d sec\n", TIME_TO_SLEEP);
  esp_deep_sleep_start();
}

int nCount =0;
void loop() {
  // put your main code here, to run repeatedly:
  // delay(dht.getMinimumSamplingPeriod());
  if (nCount++==30000)
  {
    nCount = 0;
    ReadSensor();
  }
  tb.loop();
  delay(1);
}

void ReadSensor()
{
  if (dht.getStatus()==DHTesp::ERROR_NONE){
    
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();  
    ReadSoilMoistureSensor();

    Serial.print(humidity, 1);
    Serial.print("\t");
    Serial.print(temperature, 1);
    Serial.print("\t");
    Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
    Serial.print("\t");
    Serial.println(ReadSoilMoistureSensor(), 1);
    if (tb.connect(THINGSBOARD_SERVER, THINGSBOARD_TOKEN)){
      Serial.println("Sending data to thingsboard...");
      tb.sendTelemetryFloat("Temperature", temperature);
      tb.sendTelemetryFloat("Humidity", humidity);
      tb.sendTelemetryFloat("SoilMoisture", g_SoilMoist);
    }
    else
    {
        Serial.println("Failed to connect thingsBoard");
    }
  }
}
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}