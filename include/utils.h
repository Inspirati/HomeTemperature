#pragma once

void connectToWifi(void);
void connectToMqtt(void);
void connectToTime(void);

//void getTime(void);
//unsigned long get_time(void);
void _print_time(void);
unsigned long _get_unix_time(void);

String get_macAddress(void);


void wifi_init(void);
void mqtt_init(void);

