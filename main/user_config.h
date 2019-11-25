#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_


//#define GLOBAL_DEBUG_ON                  // Enable serial debug info

//#define CFG_HOLDER          0x20170312   // Change this value to load default configurations

#define PROJECT "esp32-mqtt"

#ifdef CONFIG_PRIVATE_USER_CONFIG 
#include "private_user_config.h"
#endif

#ifndef _PRIVATE_USER_CONFIG_H_

// Wifi
#define STA_SSID            "DEF_SSID"
#define STA_PASS            "DEF_PASS"
#define STA_TYPE            AUTH_WPA2_PSK

// Ota
#define OTA_URL             "http://192.168.1.30:80/api/"PROJECT"/user1.bin"

// MQTT
#define MQTT_HOST           "mqtt://192.168.1.30"
#define MQTT_PORT           1883
#define MQTT_SECURITY    	0

#define MQTT_CLIENT_ID      "DVES_%s"  // Also fall back topic using Chip Id = last 6 characters of MAC address
#define MQTT_USER           "DVES_USER"
#define MQTT_PASS           "DVES_PASS"

#define MQTT_CLEANSESSION	1			 // cleansession by default

#endif

#define MQTT_KEEPALIVE      120          // seconds

#define MQTT_GRPTOPIC       PROJECT"s"   // Group topic
#define MQTT_TOPIC          PROJECT
#define SUB_PREFIX          "cmnd"
#define PUB_PREFIX          "stat"

// Application
#define MQTT_SUBTOPIC       "power"
#define APP_TIMEZONE        "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"            // CET DST
#define APP_POWER           0            // Saved power state Off

#define APP_PULSE			0			 // 0 = permanent, else on pulse duration in seconds-tenth

#endif
