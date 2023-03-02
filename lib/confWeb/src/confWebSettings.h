#ifndef CONF_WEB_SETTINGS_H
#define CONF_WEB_SETTINGS_H


// confWebSettings.h - configuration page for WLAN access using AsyncWebServer
//
// 2023-01-21 mh
// - derived from IotWebConfSettings.h
// - use ESPAsyncWebServer instead of ESP8266WebServer
// - original code part is framed by #if IOT_OLD ... #endif
//
// Copyright (C) 2023 Manfred Herbert

/**
 * IotWebConfSettings.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */


#if IOT_OLD
#else
// map IOT to AsyncWebServer classes
#define WebServer AsyncWebServer
#endif

// -- We might want to place the config in the EEPROM in an offset.
#ifndef IOTWEBCONF_CONFIG_START
# define IOTWEBCONF_CONFIG_START 0
#endif

// -- Maximal length of any string used in IotWebConfig configuration (e.g.
// ssid).
#ifndef IOTWEBCONF_WORD_LEN
# define IOTWEBCONF_WORD_LEN 33
#endif
// -- Maximal length of password used in IotWebConfig configuration.
#ifndef IOTWEBCONF_PASSWORD_LEN
# define IOTWEBCONF_PASSWORD_LEN 33
#endif

// -- IotWebConf tries to connect to the local network for an amount of time
// before falling back to AP mode.
#ifndef IOTWEBCONF_DEFAULT_WIFI_CONNECTION_TIMEOUT_MS
# define IOTWEBCONF_DEFAULT_WIFI_CONNECTION_TIMEOUT_MS 30000
#endif

// -- Thing will stay in AP mode for an amount of time on boot, before retrying
// to connect to a WiFi network.
#ifndef IOTWEBCONF_DEFAULT_AP_MODE_TIMEOUT_SECS
# define IOTWEBCONF_DEFAULT_AP_MODE_TIMEOUT_SECS "30"
#endif

// -- mDNS should allow you to connect to this device with a hostname provided
// by the device. E.g. mything.local
// (This is not very likely to work, and MDNS is not very well documented.)
#ifndef IOTWEBCONF_CONFIG_DONT_USE_MDNS
# define IOTWEBCONF_CONFIG_USE_MDNS 80
#endif

// -- Logs progress information to Serial if enabled.
#ifndef IOTWEBCONF_DEBUG_DISABLED
# define IOTWEBCONF_DEBUG_TO_SERIAL
#endif

// -- Logs passwords to Serial if enabled.
//#define IOTWEBCONF_DEBUG_PWD_TO_SERIAL

// -- Helper define for serial debug
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
# define IOTWEBCONF_DEBUG_LINE(MSG) Serial.println(MSG)
#else
# define IOTWEBCONF_DEBUG_LINE(MSG)
#endif

// -- EEPROM config starts with a special prefix of length defined here.
#ifndef IOTWEBCONF_CONFIG_VERSION_LENGTH
# define IOTWEBCONF_CONFIG_VERSION_LENGTH 4
#endif

#ifndef IOTWEBCONF_DNS_PORT
# define IOTWEBCONF_DNS_PORT 53
#endif

#endif  // CONF_WEB_SETTINGS_H