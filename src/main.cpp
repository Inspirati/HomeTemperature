#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp

//#include <WiFi.h>
//#include <AsyncMQTT_ESP32.h>
//#include <NTPClient.h>
//#include <WiFiUdp.h>
#include "ds18b20.h"
#include "config.h"
#include "network.h"
#include "utils.h"

void dht_setup(void);
void dht_loop(void);

unsigned long previousMillis = 0;  // Stores last time temperature was published
const long interval = 10000;       // Interval at which to publish sensor readings
bool verbose = true;

DHTesp dht;

void print_sketch_name(void) {  // output file name without leading path
  char file[] = __FILE__;
  int i;
  for (i = strlen(file); i > 0; i--)
    if ((file[i] == '\\') || (file[i] == '/')) {
      i++;
      break;
    }
  Serial.print(F("\n--- # "));
  Serial.println(&file[i]);
}

void enumerate(void) {
  JsonDocument doc;

  doc["sensor"] = "gps";
  doc["time"] = 1351824120;
  JsonArray data = doc["data"].to<JsonArray>();
  doc["data"][0] = 48.756080;
  doc["data"][1] = 2.302038;
  serializeJson(doc, Serial);
  Serial.println();
}

void test_alarm(void) {
}

void discover(void) {
}

unsigned long getTime(void) {
  time_t now;
//  struct tm timeinfo;
//  if (!getLocalTime(&timeinfo)) {
//    //Serial.println("Failed to obtain time");
//    return(0);
//  }
  time(&now);
  return now;
}

void get_time(void) {
  unsigned long epochTime = getTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  time_t curtime;
  struct tm *loc_time;
 
  curtime = time(NULL);
  loc_time = localtime(&curtime);
  char text[100]={0};
  snprintf(text, 100, "%s", asctime (loc_time));  // Displaying date and time in standard format
  Serial.print("time ");
  Serial.println(text);
}

void menu(void) {
  Serial.println(F("Select the test:"));
  Serial.println(F("  0. enumerate attached sensors"));
  Serial.println(F("  1. test_alarm"));
  Serial.println(F("  2. discover"));
  Serial.println(F("  l. load user data"));
  Serial.println(F("  d. show user data"));
  Serial.println(F("  s. save user data"));
  Serial.println(F("  t. call STP"));
  Serial.println(F("  u. unix time"));
  Serial.println(F("  p. print time"));
  Serial.println(F("  h. humidity"));
  Serial.println(F("  m. menu (this)"));
  Serial.println(F("  v. toogle verbose mode"));
  Serial.print(F("Your choice> "));
  Serial.flush();
  while (Serial.available())
    Serial.read();
}

void console(void) {
  if (Serial.available()) {
    int select = Serial.read();
//    Serial.write(select);
//    Serial.write('\n');
    Serial.flush();
    if (select == '0') return enumerate();
    if (select == '1') return test_alarm();
    if (select == '2') return discover();
    if (select == 'l') return ds18b20_load();
    if (select == 'd') return ds18b20_show();
    if (select == 's') return ds18b20_save();
    if (select == 'u') { _get_unix_time(); return; }
    if (select == 'p') { _print_time(); return; }
    if (select == 't') return get_time();
    if (select == 'h') return dht_loop();
    if (select == '?') return menu();
    if (select == 'v') { verbose = !verbose; if (verbose) Serial.println("verbose"); else Serial.println("silent"); }
  }
}

void setup() {
  //verbose = true;
  Serial.begin(SERIAL_SPEED);
  while (!Serial) {};
  print_sketch_name();
  wifi_init();
  mqtt_init();
  connectToWifi();
  ds18b20_init();
  Serial.println("type '?' for help (menu)");
  dht_setup();
}

void loop() {
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = x) we take temperature readdings and publish a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Save the last time a new reading was published
    ds18b20_publish();
  }
  console();
//  if (verbose) verbose--;
}

//#include <functional>
//typedef std::function<void(char* topic, float temp)> _OnSensorData;  // user callbacks
//void OnSensorData(char* topic, float temp) {}

void dht_setup(void)
{
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);

  // Autodetect is not working reliable, don't use the following line
  // dht.setup(17);
  // use this instead: 
  dht.setup(17, DHTesp::DHT22); // Connect DHT sensor to GPIO 17
}

void dht_loop(void)
{
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
}
