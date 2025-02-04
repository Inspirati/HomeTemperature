#include <math.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <DallasTemperature.h>
#include "../include/config.h"
#include "../include/ds18b20.h"
#include "../include/network.h"
#include "../include/utils.h"

#include <ESP8266WiFi.h> // temporary for WiFi

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
float raw_tmps[MAX_SENSORS];  // raw sensor results averaged across oversampled readings
float adj_tmps[MAX_SENSORS];  // adjusted results by applying calibration offset to raw
float std_devs[MAX_SENSORS];  // the standard deviation of each sensors results from oversampled readings
float off_sets[MAX_SENSORS];
//float results[MAX_SENSORS]; 
float avg_tmp; // across all sensors - for calibration, calculation of offset data
float std_dev; // between all sensors - for calibration across the array
int sensorCount;
bool calMode = true;

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

static inline void formatSensorAddress(DeviceAddress sensorAddress, char * dest, int size) {
  if (size > 15) {
    for (int i = 0; i < 8; i++) {
      snprintf(&dest[i*2], size, "%02X", sensorAddress[i]);
    }
    dest[16] = '\0';
  }
}

static inline void format1WireAddress(DeviceAddress sensorAddress, char * dest, int size) {
  if (size > 14) {
    sprintf(dest, "%02X-", sensorAddress[0]);
    for (int i = 1; i < 7; i++) {
      snprintf(&dest[i*2+1], size, "%02X", sensorAddress[7 - i]);
    }
  }
}

static float sample_sensors(bool init_off_sets) {  // Collect temperature readings multiple times for each sensor
  float sensorReadings[NUM_SAMPLES][MAX_SENSORS];  // 2D array for storing readings (rows = readings, columns = sensors)
  float stats[MAX_STAT];

  profiling_begin();
  // Oversample every sensor which was detected during setup
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sensors.requestTemperatures();
    for (int j = 0; j < sensorCount; j++) {
      sensorReadings[i][j] = sensors.getTempC(sensorAddresses[j]);
    }
    yield();
  }
  profiling_print("ds18b20 sampling time: ");
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
    raw_tmps[j] = stats[AVG_TEMP];
    adj_tmps[j] = stats[AVG_TEMP] - off_sets[j];
//    if (init_off_sets) {
//      results[j] = stats[AVG_TEMP];
//    } else {
//      results[j] = stats[AVG_TEMP] - off_sets[j];
//    }
  }
  profiling_print("Sensors stat calc time: ");
  // Calculate the resulting statistics across all the sensor averages 
//  calc_stats(stats, results, sensorCount, '*');
  calc_stats(stats, raw_tmps, sensorCount, '*');
  std_dev = stats[STD_DEV];
  avg_tmp = stats[AVG_TEMP];
  if (init_off_sets) {
    for (int i = 0; i < sensorCount; i++) {
//      off_sets[i] = results[i] - stats[AVG_TEMP];
      off_sets[i] = raw_tmps[i] - stats[AVG_TEMP];
    }
    if (verbose) {
      Serial.print("off_sets: ");
      print_array(off_sets, sensorCount);
      Serial.println();
    }
  }
  profiling_print("Totals stat calc time: ");
  return stats[AVG_TEMP];
}

void ds18b20_show(void) {
  for (uint8_t i = 0; i < sensorCount; i++) {
    printSensorAddress(sensorAddresses[i]);
    Serial.print(": ");
    Serial.print(off_sets[i]);
    Serial.println();
  }
}

void ds18b20_load(void) { // the sensors default userdata value was 21760 = 170 degrees C
  for (uint8_t i = 0; i < sensorCount; i++) {
    int16_t data = sensors.getUserData(sensorAddresses[i]);
    off_sets[i] = sensors.rawToCelsius(data);
  }
  ds18b20_show();
}

void ds18b20_save(void) {
  for (uint8_t i = 0; i < sensorCount; i++) {
    int16_t data = sensors.celsiusToRaw(off_sets[i]);
    sensors.setUserData(sensorAddresses[i], data);
    printSensorAddress(sensorAddresses[i]);
    Serial.print(": ");
    Serial.print(off_sets[i]);
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
    (void)avg;
    doc["dev"] = "esp8266";
    doc["mac"] = get_macAddress();
//    doc["ip4"] = WiFi.localIP().toString().c_str();
    doc["sec"] = _get_unix_time();
    doc["cnt"] = sensorCount;
    if (calMode) {
      doc["val"] = avg_tmp;
      doc["var"] = std_dev;
    }
    JsonArray sensors = doc["data"].to<JsonArray>();
    for (uint8_t i = 0; i < sensorCount; i++) {
      JsonObject sensors_0 = sensors.add<JsonObject>();
      //formatSensorAddress(sensorAddresses[i], dest, sizeof(dest));
      format1WireAddress(sensorAddresses[i], dest, sizeof(dest));
      sensors_0["dev"] = "ds18b20";
      sensors_0["adr"] = dest;
      sensors_0["raw"] = raw_tmps[i];
      sensors_0["adj"] = adj_tmps[i];
      sensors_0["ofs"] = off_sets[i];
      sensors_0["var"] = std_devs[i];
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
    (void)packetIdPub1;
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

