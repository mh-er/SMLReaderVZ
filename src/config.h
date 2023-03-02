#ifndef CONFIG_H
#define CONFIG_H

#include "Sensor.h"
#include <sml/sml_file.h>

# define MY_TEST 0                 // use test mode, no sensor data used, only test data is sent to VZ_UUID_TEST
#define MY_TEST_SEND_UPDATE 5000  // ms

// AP mode for configuration
// Identify configuration info in EEPROM, Modifying cause a loss of the existig configuration in EEPROM
// note: EEPROM configuration remains unchanged after firmware update; update main version count if you are using a new application
// otherwise the previous configuration is considered valid.
#define WIFI_AP_CONFIG_VERSION "2.2.1"      // 4 bytes are significant for check with EEPROM (IOTWEBCONF_CONFIG_VERSION_LENGTH in confWebSettings.h)

#define WIFI_AP_SSID "YourSMLReaderVZ"
#define WIFI_AP_IP "192.168.4.1"            // default address, set by the framework.
#define WIFI_AP_DEFAULT_PASSWORD "yourDefaultAPpassword"
#define WEBCONF_AP_MODE_CONFIG_PIN D3       // to force AP mode

//mh own WLAN
#define MY_SSID "yourWLANname"
#define MY_PASSWORD ""
#define MY_HOSTNAME "YourSMLReaderVZ"

//  NTP time
#define NMAX_DATE_TIME 20               // max length of date and time string
#define TIMEZONE +1                     // Central europe
#define TIMEZONE_DEFAULT "1"            // string default for configuration

// sensor config
static const SensorConfig SENSOR_CONFIGS[] = {
    {.pin = D2,                                 // input pin
     .name = "yourMeterName",                   // name of meter for debug and MQTT
     .numeric_only = false,
     .interval = 5}};                           // read out interval in sec, 0=no wait
const uint8_t NUM_OF_SENSORS = sizeof(SENSOR_CONFIGS) / sizeof(SensorConfig);


// build in LED is inverted for Wemos D1 mini
#define LED_BUILTIN_ON  0
#define LED_BUILTIN_OFF 1

// Special Heart Beat values
#define HEART_BEAT_RESET 1
#define HEART_BEAT_WIFI_CONFIG 2

// Verbose Level
#define VERBOSE_LEVEL_WLAN  1
#define VERBOSE_LEVEL_HTTP  0
#define VERBOSE_LEVEL_MeterData  1
#define VERBOSE_LEVEL_MeterProtocol  0
#define VERBOSE_LEVEL_Setup  1
#define VERBOSE_LEVEL_Loop 0
#define VERBOSE_LEVEL_TIME 0

#define DATE_UPDATE_INTERVAL 60000      // in ms; for Dash Board

// http transfer to data base
#define VZ_SERVER           "yourVolkszaehlerServer_name_or_IP"
#define VZ_MIDDLEWARE       "middleware.php"
#define VZ_DATA_JSON        "data.json"
#define VZ_UUID_NO_SEND     "null"        // use this uuid if you do not want to transmit data for a channel

// SMLReader channels: replace by your UUIDs created in VZ frontend
#define VZ_UUID_POWER_IN            "power-in"                              // 3 
#define VZ_UUID_ENERGY_OUT          "energy-out"                        	// 4
#define VZ_UUID_ENERGY_IN	        "energy-in"                            	// 5
#define VZ_UUID_TEST                "test"                                  // 13 for test
#define VZ_UUID_SML_HEART_BEAT      "sml-heart-beat"                        // 16 debug channel

// OBIS identifier for used channels
#define OBIS_ID_ENERGY_IN   "1-0:1.8.0*255"
#define OBIS_ID_ENERGY_OUT  "1-0:2.8.0*255"
#define OBIS_ID_POWER_IN    "1-0:16.7.0*255"

#endif