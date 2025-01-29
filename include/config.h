#pragma once

#define SERIAL_SPEED    115200
#define WIFI_SSID       "Belong7F9C9D"
#define WIFI_PASSWORD   "wb2yezd36ka445bh"

// Mosquitto MQTT Broker
//#define MQTT_HOST IPAddress(192, 168, 1, XXX)
#define MQTT_HOST IPAddress(10, 0, 0, 10)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

//#define NUM_SAMPLES 20  // Number of temperature readings to collect for each sensor
#define NUM_SAMPLES  8   // Number of temperature readings to collect for each sensor
#define MAX_SENSORS  10  // Maximum number of sensors on the bus (adjust accordingly)
#define ONE_WIRE_BUS 2   // GPIO2 (D4 on many ESP8266 boards)

