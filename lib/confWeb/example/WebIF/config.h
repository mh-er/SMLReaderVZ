#ifndef CONFIG_H
#define CONFIG_H


#define VERSION "1.0.1"

// AP mode for configuration
#define CONFIG_VERSION "1.0.2" // Modifying the config version will probably cause a loss of the existig configuration in EEPROM, 4 bytes are significant
#define MY_WIFI_AP_SSID "yourAP_SSID"
#define MY_WIFI_AP_DEFAULT_PASSWORD "yourAP_DefaultPassword"

#define WEBCONF_AP_MODE_CONFIG_PIN D3

//mh own WLAN
#define MY_SSID "yourWLAN_SSID"
#define MY_PASSWORD ""
#define MY_HOSTNAME "yourAP_SSID"
#define WEBCONF_DEFAULT_STA_MODE_TIMEOUT_SECS 30

//  NTP
#define NTP_SERVER_IP "de.pool.ntp.org"
#define TIMEZONE +1 // Central europe

// build in LED is inverted for Wemos D1 mini
#define LED_BUILTIN_ON  0
#define LED_BUILTIN_OFF 1

// Verbose Level
#define VERBOSE_LEVEL_WLAN  1
#define VERBOSE_LEVEL_HTTP  0
#define VERBOSE_LEVEL_MeterData  1
#define VERBOSE_LEVEL_Setup  1
#define VERBOSE_LEVEL_Loop 0

#define DATE_UPDATE_INTERVAL 60000 // in ms

#endif  // CONFIG_H