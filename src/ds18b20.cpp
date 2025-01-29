#include <Arduino.h>
#include <math.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <DallasTemperature.h>
#include "config.h"
#include "ds18b20.h"
#include "network.h"
#include "utils.h"

#include <ESP8266WiFi.h> // temporary for WiFi
//#include <WiFi.h> // temporary for WiFi

extern bool verbose;
extern AsyncMqttClient mqttClient;

//#define MQTT_PUB_TEMP "esp/ds18b20/temperature"
//#define MQTT_PUB_JSON "esp/ds18b20/json"

#define MQTT_PUB_TEMP "esp/%s/temp" // will the run-time substituted with local node IP address
#define MQTT_PUB_SENS "esp/%s/sensors"
#define MQTT_PUB_JSON "esp/%s/sensors/json"
#define MQTT_PUB_IPSTRLEN 16 // '123.567.9AB.DEF' so an IP address c-string is max 16 characters (including '\0')
#define MQTT_PUB_BUFSIZ (sizeof(MQTT_PUB_JSON)/sizeof(char) - 2 + MQTT_PUB_IPSTRLEN) // subtract len of the 2 byte token %s
char mqtt_pub_str[MQTT_PUB_BUFSIZ]; // universal buffer for any and all dynamically generated publish strings

enum stat_types { MIN_TEMP, MAX_TEMP, DELTA, AVG_TEMP, STD_DEV, MAX_STAT };

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorAddresses[MAX_SENSORS]; // Array to store sensor addresses
float offsets[MAX_SENSORS];
float results[MAX_SENSORS]; 
float std_devs[MAX_SENSORS]; 
float std_dev; 
int sensorCount;

static float findMin(float readings[], int count) {
  float min = readings[0];
  for (int i = 1; i < count; i++) {
    if (readings[i] < min) {
      min = readings[i];
    }
  }
  return min;
}

static float findMax(float readings[], int count) {
  float max = readings[0];
  for (int i = 1; i < count; i++) {
    if (readings[i] > max) {
      max = readings[i];
    }
  }
  return max;
}

static float calculateAverage(float readings[], int count) {
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += readings[i];
  }
  return sum / count;
}

static float calculateStdDev(float readings[], int count, float avg) {
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += pow(readings[i] - avg, 2);
  }
  return sqrt(sum / count);
}

static void print_array(float array[], int count) {
  for (int i = 0; i < count; i++) {
    Serial.print(array[i]);
    if (i < count - 1) {
      Serial.print(",");
    }
  }
}

static void calc_stats(float stats[], float data[], int data_len, char id) {
  stats[MIN_TEMP] = findMin(data, data_len);
  stats[MAX_TEMP] = findMax(data, data_len);
  stats[DELTA] = stats[MAX_TEMP] - stats[MIN_TEMP];
  stats[AVG_TEMP] = calculateAverage(data, data_len);
  stats[STD_DEV] = calculateStdDev(data, data_len, stats[AVG_TEMP]);
  if (verbose) {
    Serial.print("S");
    Serial.print(id);
    Serial.print(":");
    //print_array(data, data_len);
    //Serial.print(" - ");
    print_array(stats, MAX_STAT);
    Serial.println();
  }
}

static void printSensorAddress(DeviceAddress sensorAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (sensorAddress[i] < 16) {
      Serial.print("0");
    }
    Serial.print(sensorAddress[i], HEX);
  }
}

static void formatSensorAddress(DeviceAddress sensorAddress, char * dest, int size) {
  if (size > 15) {
    for (int i = 0; i < 8; i++) {
      snprintf(&dest[i*2], size, "%02X", sensorAddress[i]);
    }
    dest[16] = '\0';
  }
}

static void format1WireAddress(DeviceAddress sensorAddress, char * dest, int size) {
  if (size > 14) {
    sprintf(dest, "%02X-", sensorAddress[0]);
    for (int i = 1; i < 7; i++) {
      snprintf(&dest[i*2+1], size, "%02X", sensorAddress[7 - i]);
    }
  }
}

static float sample_sensors(bool init_offsets) {  // Collect temperature readings multiple times for each sensor
  float sensorReadings[NUM_SAMPLES][MAX_SENSORS];  // 2D array for storing readings (rows = readings, columns = sensors)
  float stats[MAX_STAT];
  unsigned long startMillis;
  unsigned long endMillis;

  startMillis = millis();
  // Oversample every sensor which was detected during setup
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sensors.requestTemperatures();
    for (int j = 0; j < sensorCount; j++) {
      sensorReadings[i][j] = sensors.getTempC(sensorAddresses[j]);
    }
    yield();
  }
  endMillis = millis();
  if (verbose) {
    Serial.print("Conversation time: ");
    Serial.print((endMillis - startMillis));
    Serial.print(" = time per sensor: ");
    Serial.println((endMillis - startMillis) / NUM_SAMPLES);
  }

  startMillis = millis();
  // Calculate and print statistics for each sensor (column in 2D array)
  for (int j = 0; j < sensorCount; j++) {
    float sensorData[NUM_SAMPLES];
    // Extract the data for the current sensor j (column), rows i are the samples
    for (int i = 0; i < NUM_SAMPLES; i++) {
      sensorData[i] = sensorReadings[i][j];
    }
    // Calculate min, max, delta, avg, and std dev for this sensor (column)
    calc_stats(stats, sensorData, NUM_SAMPLES, '0' + j);
    std_devs[j] = stats[STD_DEV];
    if (init_offsets) {
      results[j] = stats[AVG_TEMP];
    } else {
      results[j] = stats[AVG_TEMP] - offsets[j];
    }
  }
  endMillis = millis();
  if (verbose) {
    Serial.print("Sensors stats calc time: ");
    Serial.println((endMillis - startMillis));
  }
  startMillis = millis();
  // Calculate the resulting statistics across all the sensor averages 
  calc_stats(stats, results, sensorCount, '*');
  std_dev = stats[STD_DEV];
  endMillis = millis();
  if (init_offsets) {
    for (int i = 0; i < sensorCount; i++) {
      offsets[i] = results[i] - stats[AVG_TEMP];
    }
    if (verbose) {
      Serial.print("offsets: ");
      print_array(offsets, sensorCount);
      Serial.println();
    }
  }
  if (verbose) {
    Serial.print("Totals stat calc time: ");
    Serial.println((endMillis - startMillis));
  }
  return stats[AVG_TEMP];
}

void ds18b20_show(void) {
  for (uint8_t i = 0; i < sensorCount; i++) {
    printSensorAddress(sensorAddresses[i]);
    Serial.print(": ");
    Serial.print(offsets[i]);
    Serial.println();
  }
}

void ds18b20_load(void) { // the sensors default userdata value was 21760 = 170 degrees C
  for (uint8_t i = 0; i < sensorCount; i++) {
    int16_t data = sensors.getUserData(sensorAddresses[i]);
    offsets[i] = sensors.rawToCelsius(data);
  }
  ds18b20_show();
}

void ds18b20_save(void) {
  for (uint8_t i = 0; i < sensorCount; i++) {
    int16_t data = sensors.celsiusToRaw(offsets[i]);
    sensors.setUserData(sensorAddresses[i], data);
    printSensorAddress(sensorAddresses[i]);
    Serial.print(": ");
    Serial.print(offsets[i]);
    Serial.print(" -> ");
    Serial.print(data);
    Serial.println();
  }
}
/*
void ds18b20_samples(void) {
  float avg = sample_sensors(false);
  snprintf(mqtt_pub_str, MQTT_PUB_BUFSIZ, MQTT_PUB_TEMP, WiFi.localIP().c_str());
  uint16_t packetIdPub1 = mqttClient.publish(mqtt_pub_str, 1, true, String(avg).c_str());
  if (verbose) {
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", mqtt_pub_str, packetIdPub1);
//    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \r\n", avg);
  }
}
 */
void ds18b20_publish(void) {
  uint16_t packetIdPub1;
  JsonDocument doc;
  String json;
  char dest[17];
  if (sensorCount > 0) {

    float avg = sample_sensors(false);

  //  snprintf(mqtt_pub_str, MQTT_PUB_BUFSIZ, MQTT_PUB_TEMP, WiFi.localIP().toString().c_str());
  //  packetIdPub1 = mqttClient.publish(mqtt_pub_str, 1, true, String(avg).c_str());
  //  if (verbose) {
  //    Serial.printf("Published on topic %s at QoS 1, packetId: %i ", mqtt_pub_str, packetIdPub1);
  //    Serial.printf("Message: %.2f \r\n", avg);
  //  }
    doc["type"] = "esp8266";
    doc["addr"] = get_macAddress();
    doc["time"] = _get_unix_time();
    doc["size"] = sensorCount;
    doc["vari"] = std_dev;
    JsonArray sensors = doc["sens"].to<JsonArray>();
    for (uint8_t i = 0; i < sensorCount; i++) {
      JsonObject sensors_0 = sensors.add<JsonObject>();
      //formatSensorAddress(sensorAddresses[i], dest, sizeof(dest));
      format1WireAddress(sensorAddresses[i], dest, sizeof(dest));
      sensors_0["type"] = "ds18b20";
      sensors_0["addr"] = dest;
      sensors_0["data"] = results[i];
      sensors_0["offs"] = offsets[i];
      sensors_0["vari"] = std_devs[i];
    }
    doc.shrinkToFit();  // optional
    if (verbose) {
  //    serializeJsonPretty(doc, Serial);
      serializeJson(doc, Serial);
      Serial.println();
    }
    serializeJson(doc, json);
  //  mqttClient.publish(MQTT_PUB_JSON, 1, true, json.c_str());
    snprintf(mqtt_pub_str, MQTT_PUB_BUFSIZ, MQTT_PUB_JSON, WiFi.localIP().toString().c_str());
    packetIdPub1 = mqttClient.publish(mqtt_pub_str, 1, true, json.c_str());

  // stats[AVG_TEMP]
  }
}

void ds18b20_init(void) {
  sensors.begin();
  sensorCount = sensors.getDeviceCount();
  Serial.print("Number of sensors found: ");
  Serial.println(sensorCount);
  if (sensorCount > 0) {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors.getAddress(sensorAddresses[i], i)) {
        Serial.print("Sensor ");
        Serial.print(i);
        Serial.print(": ");
        printSensorAddress(sensorAddresses[i]);
        Serial.println();
        yield();
      }
    }
    if (sensorCount > MAX_SENSORS) {
      sensorCount = MAX_SENSORS;
      Serial.print("To many sensors, capping at ");
      Serial.print(sensorCount);
      Serial.println();
    }
    sample_sensors(true);
  } else {
    Serial.println("No sensors found!");
  }
}
