#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include "config.h"
#include "network.h"
#include "utils.h"

extern bool verbose;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void foo(void) {
//  String hostname = WiFi.getHostname(); 
//  char hostname[15];
//  sprintf(hostname, "esp8266-%06x", WiFi.getChipId());
//  Serial.print("Current Hostname: "); 
//  Serial.println(hostname); 
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
//  WiFi.printDiag(Serial);
}

/*
class IPAddress
union {
	uint8_t bytes[4];  // IPv4 address
	uint32_t dword;
} _address;

IPAddress ESP8266WiFiSTAClass::localIP() {
    struct ip_info ip;
    wifi_get_ip_info(STATION_IF, &ip);
    return IPAddress(ip.ip.addr);
}

uint8_t* ipAddress(uint8_t* ip) {
//  IPAddress addr = WiFi.localIP();
//    struct ip_info ip;
//    wifi_get_ip_info(STATION_IF, &ip);
//    return IPAddress(ip.ip.addr);
    return ip;
}

String ESP8266WiFiSTAClass::macAddress(void) {
    uint8_t mac[6];
    char macStr[18] = { 0 };
    wifi_get_macaddr(STATION_IF, mac);

    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}
 */

void connectToWifi(void) {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  String foo = WiFi.macAddress();
//void ESP8266WiFiClass::printDiag(Print& p);
//String ESP8266WiFiSTAClass::hostname(void);
}

//void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
//void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  foo();
  connectToTime();
  connectToMqtt();
}

//void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
//  Serial.println("WiFi connected");
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());
//}

//void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
//  Serial.println("Disconnected from WiFi access point");
//  Serial.print("WiFi lost connection. Reason: ");
//  Serial.println(info.wifi_sta_disconnected.reason);
//  Serial.println("Trying to Reconnect");
//  WiFi.begin(ssid, password);
//void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt(void) {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  if (verbose) {
    Serial.print("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

void wifi_init(void) {
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
//  WiFi.onEvent(onWifiConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
//  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
//  WiFi.onEvent(onWifiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

void mqtt_init(void) {
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPLACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
}

String get_macAddress(void) { return WiFi.macAddress(); }
