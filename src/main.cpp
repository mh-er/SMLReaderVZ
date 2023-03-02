/* *** main.cpp to receive SML data from a (electrical) meter and send it to the Volkszaehler middleware.

2023-02-19 mh
- add missing update of date/time in loop

2023-02-17 mh
- remove usage of timeControl
- split up input for server url, configuration page updated
- not transmission, if uuid = VZ_UUID_NO_SEND

2023-02-05 mh
- use timeControl 1.1.0 (built-in ntp timer, error corrections)
- add heartbeat channel

2023-01-28 mh
- added confWeb for the configuration interface in WiFi AP mode
- added struct MyHttpConfig for the configuration of VZ server and UUID
- added Home Page and handler
- added in class MyHttp: _uuid\[\], method setServerName(), testHttp(), initHttp()
- rename MyHttp.cpp/.h to smlHttp.cpp/.h and class to SmlHttp because strong dependance on smlLib.
- use lib timeControl instead of local time.cpp, update to new interface
- use own (modified) ESP-Dash @ 4.0.1

- clean up cascaded include files
- removed MicroDebug lib, defined Macros in respective include files.

2022-12-30   mh
- adapted for own use based on https://github.com/mruettgers/SMLReader
- modularization of code (own cpp instead of monolithic include of code into main.cpp)
- removal of jled use
- use ESP-Dash instead of IOTwebserver
- remove MQTT interface and add http interface to Volkszaehler middleware instead

Copyright (C) M. Herbert, 2022-2023.
Licensed under the GNU General Public License v3.0

*** end change log *** */

/* ***
# SMLReader #
Receive SML data from an (electrical) meter and send it to the Volkszaehler middleware.  
Adapted for own use based on https://github.com/mruettgers/SMLReader.  

### Changes
- modularization of code (own cpp instead of monolithic include of code into main.cpp)
- removal of jled use
- use ESP-Dash (with ESPAsyncWebserver) instead of IOTwebserver
- remove MQTT interface and add http interface to Volkszaehler middleware instead  

# Description
## Overview
Receive SML data from an (electrical) meter, evaluate it and send selected data to the middleware
of Volkszaehler (addresses and channel UUIDs hard coded).

Evaluation is done in periods defined in structure *SensorConfig SENSOR_CONFIGS*.  
A webserver dashboard is available to show the instantaneous information.  

The build in LED is used for signalling:  
- Setup()
  - On at begin
  - Off before start of conf.init and sensor.init
  - on at end
- Loop()  
  - no switch
- wifi_connected()  
  - Off at begin
- process_message (SML data)  
  - On at begin
  - Off at end

Serial monitor output is available as configured during the build.

## Configuration

### Web Interface
The program provides a Web Interface for configuration and a small home page.

### Web Access Point (WiFI AP mode)
The program provides a Web Access Point at boot time via http://192.168.4.1 (or the configured SSID name) using the configured password.  
It offers a configuration page both for the Access Point and a local WLAN for SSID name and password.
Additional customer parameters are supported.  
Configuration is stored in EEPROM.  
At initial boot, the defined default password *MY_WIFI_AP_DEFAULT_PASSWORD* is used for AP mode access.

If no client connects before the timeout (configured to 30sec), the device will automatically continue in STA (station) mode and connect to a local WLAN if configured.

### Web Interface in a local WLAN (WiFi STA mode)
If the device has already been configured, it will automatically connect to the local WiFi.  
The device web interface can be reached via the IP address obtained from your local network's DHCP server or the configured SSID name.  
A small Home page is provided which offers access to the configuration page as well.  
To login into configuration, provide the user *admin* and the configured AP *password*.

## Usage
- Just switch on the board.
- Connect to the access point if you need to configure your local WiFi net:  IP address 192.168.4.1 (or the specified network name) using your configured AP password.
- A force to AP mode is provided in case of forgotten password:  
Connect *WEBCONF_AP_MODE_CONFIG_PIN* to ground and use *MY_WIFI_AP_DEFAULT_PASSWORD*.  
- Wait for AP timeout and access the home page in your local network:  
accessible by MY_WIFI_AP_SSID (or IP address provided by your DHCP server).
- You can access the configuration page in STA mode by login as *admin* with the configured AP password.


## Dash Board
A dash board is acessible both from AP and STA mode.
It is based on https://github.com/ayushsharma82/ESP-DASH, with only minor modification:

- the root path was modified from "/" to "/dash"
- a link "return to home page" with referende to "/" was added (replacing the link to "espdash.pro").

## Implementation
Using classes  
**Sensor:**      receive data and put it into a buffer  
**SmlHttp:**     transfers data to Volkszaehler data base  
**smlDebug:**    functions for output of sml messages to serial monitor [3]  

Used own libs:  
**confWeb**             configurable web server (derived from [1])  
**timeControl**         provides date, time and epoch time stamp (derived from time.cpp [2])  
**libSML**              parse and evaluate SML messages (error correction of [4])  
**ESP-Dash**            Dash Board (small modification of [5])  

Used 3rd party libs:  
**ArduinoJson**  
**ESP AsyncWebServer**  provide web server on D1 mini  
**ESPAsyncTCP**  
 **NTPClient**           access of NTP server  
**TimeLib**             low level date/time functions  

*** end description *** */
// c and cpp
#include <list>
#include <stdio.h>
#include <string.h>
#include <time.h>
// Arduino + ESP framework
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <TimeLib.h>
// own libs
#include <confWeb.h>
#include <confWebParameter.h>
#include <sml/sml_file.h>
// local dir
#include "main.h"
#include "config.h"
#include "smlDebug.h"
#include "Sensor.h"
#include "smlHttp.h"

// local function declaration


// sensor stuff
std::list<Sensor *> *sensors = new std::list<Sensor *>();
// callback for sensor, main processing function
void process_message(byte *buffer, size_t len, Sensor *sensor, State sensorState);

// callback handler and html page functions
void wifiConnected();
void configSaved();
void notFound(AsyncWebServerRequest *request);

void homePage(String& content);
void handleRoot(AsyncWebServerRequest *request);
void startHtml(AsyncWebServerRequest *request);
void onConfiguration(AsyncWebServerRequest *request);

void onReset(AsyncWebServerRequest *request);
boolean needReset = false;

String currentHtmlPage ="";     // sub-headline for different modes
String currentSSID = "unknown";
String currentIP   = "unknown";
char c_wifiIP[40];
String vzServerIP = "unknown";

// time stuff

uint32_t  lastDateUpdate;
uint16_t  Year = 1960;
uint8_t   Month = 0;
uint8_t   Day = 0;
uint8_t   Hour = 0;
uint8_t   Minute = 0;
int       Timezone = TIMEZONE;  
char      s_TimezoneOffset[4] = TIMEZONE_DEFAULT;

time_t    getLocalTime();                 // callback for TimeLib
time_t    getEpochTime();                 // UNIX epoch time in sec
void      getDateTime(char* s_DateTime);  // update date and time, return char string

uint64_t  epochtime;   // in seconds - not ms to avoid overrun
uint64_t  timeStamp;   // in ms
String    s_timeStamp;   // in seconds
String    s_epochtime;   // in seconds
char      s_DateTime[NMAX_DATE_TIME] = "1960-01-01 00:00";

// volkszaehler stuff
SmlHttpConfig myHttpConfig;
SmlHttp       my_http;

// server and WiFi stuff
// class for WiFi and webserver configuration page, connects to WiFi in AP or STA mode
// note: constructor does some presets
DNSServer dnsServer;
AsyncWebServer server(80);

IotWebConf confWeb(WIFI_AP_SSID, &dnsServer, &server, WIFI_AP_DEFAULT_PASSWORD, WIFI_AP_CONFIG_VERSION);
boolean b_WiFi_connected = false;
boolean b_WiFi_firstTime = true;

// custom configurationparameter: TextParameter(label,id,valueBuffer,lengthValueBuffer,defaultValue,placeholder,customHtml)
// configuration input is in valueBuffer
TextParameter confVZserverParam = TextParameter("VZ Server", "vzServer", myHttpConfig.vzServer, sizeof(myHttpConfig.vzServer),
                                                   VZ_SERVER, nullptr, "vzServer");
TextParameter confVZmiddlewareParam = TextParameter("VZ Middleware", "vzMiddleware", myHttpConfig.vzMiddleware, sizeof(myHttpConfig.vzMiddleware),
                                                   VZ_MIDDLEWARE, nullptr, "vzMiddleware");
TextParameter confVZuuidPowInParam = TextParameter("UUID PowerIn", "UUID-PowerIn", &myHttpConfig.uuidValue[vzPOWER_IN][0], sizeOfUUID,
                                                   VZ_UUID_POWER_IN, nullptr, "UUID-PowerIn");
TextParameter confVZuuidEnInParam = TextParameter("UUID EnergyIn", "UUID-EnergyIn", &myHttpConfig.uuidValue[vzENERGY_IN][0], sizeOfUUID,
                                                   VZ_UUID_ENERGY_IN, nullptr, "UUID-EnergyIn");
TextParameter confVZuuidEnOutParam = TextParameter("UUID EnergyOut", "UUID-EnergyOut", &myHttpConfig.uuidValue[vzENERGY_OUT][0], sizeOfUUID,
                                                   VZ_UUID_ENERGY_OUT, nullptr, "UUID-EnergyOut");
TextParameter confVZuuidTestParam = TextParameter("UUID Test", "UUID-Test", &myHttpConfig.uuidValue[vzTEST][0], sizeOfUUID,
                                                   VZ_UUID_TEST, nullptr, "UUID-Test");
TextParameter confVZuuidSmlHeartBeatParam = TextParameter("UUID SmlHeartBeat", "UUID-SmlHeartBeat", &myHttpConfig.uuidValue[vzSML_HEART_BEAT][0], sizeOfUUID,
                                                   VZ_UUID_SML_HEART_BEAT, nullptr, "UUID-SmlHeartBeat");
NumberParameter confTimezoneParam = NumberParameter("TimezoneOffset[h]", "TimezoneOffset", s_TimezoneOffset, sizeof(s_TimezoneOffset),
                                                   TIMEZONE_DEFAULT, nullptr, "TimezoneOffset");
ParameterGroup paramGroup = ParameterGroup("VZ Settings", "VZ-Settings");

Parameter* thingName;                   // name set on configuration page, might override WIFI_AP_SSID
char wifiAPssid[IOTWEBCONF_WORD_LEN] = WIFI_AP_SSID;

// ESP-Dash
ESPDash dashboard(&server);

Card card_Title(&dashboard, GENERIC_CARD, "Title");
Card card_Time(&dashboard, GENERIC_CARD, "Date & Time");
Card card_power(&dashboard, GENERIC_CARD, "Power In (W)");
Card card_energy(&dashboard, GENERIC_CARD, "Energy In (kWh)");
Card card_energy2(&dashboard, GENERIC_CARD, "Energy2 Out (kWh)");
Card card_TimeStamp(&dashboard, GENERIC_CARD, "Time Stamp (ms)");
Card card_EpochTime(&dashboard, GENERIC_CARD, "Epoch Time (s)");
Card card_status(&dashboard, STATUS_CARD, "Loop Status", "empty");
Card card_SensorStatus(&dashboard, STATUS_CARD, "Sensor Status", "empty");

String s_loopCount;
char myStringBuf[80]; // emulate string conversion for uint64_t because old framework needs to be used.

double energyIn=0.0;
double energyOut=0.0;
double vzTestValue=0.0;
float powerIn=0.0;

//----------------------------------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
 
  confWeb.setConfigPin(WEBCONF_AP_MODE_CONFIG_PIN);  // used to enter config mode if PIN is pulled to ground during init()
  
	// Delay for getting a serial console attached in time
	delay(2000);

  // line feed - get out of monitor start garbage
  Serial.println();
  Serial.println();
  Serial.println("Hello");

  //--- Start AsyncWebServer + Handler --------------------------------------------------------------
  server.onNotFound(notFound);

  server.on("/", handleRoot);     // used by ESP-dash
  server.on("/index.html", startHtml);
  server.on("/start", handleRoot);
  server.on("/config", onConfiguration);
  server.on("/reset", onReset);

  // own config parameter group
  paramGroup.addItem(&confVZserverParam);
  paramGroup.addItem(&confVZmiddlewareParam);
  paramGroup.addItem(&confVZuuidPowInParam);
  paramGroup.addItem(&confVZuuidEnInParam);
  paramGroup.addItem(&confVZuuidEnOutParam);
  paramGroup.addItem(&confVZuuidSmlHeartBeatParam);
  paramGroup.addItem(&confVZuuidTestParam);
  paramGroup.addItem(&confTimezoneParam);
  confWeb.addParameterGroup(&paramGroup);

  // handler for web configuration
  confWeb.setConfigSavedCallback(&configSaved);
  confWeb.setWifiConnectionCallback(&wifiConnected);

  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

  // Start server
  //--- we start in AP mode to allow configuration and switch to STA mode after timeout.

    boolean validConfig = confWeb.init();

    if (!validConfig)
    {
      DEBUG("Missing or invalid config. Possibly unable to initialize all functionality.");
    }
    else
    {
      DEBUG("Setup: valid config.");
      // here we can do some setup stuff that requires valid configuration data
        // fetch name from configuration
      thingName =  confWeb.getThingNameParameter();
      strncpy(wifiAPssid, thingName->valueBuffer,IOTWEBCONF_WORD_LEN);  
      Timezone = atoi(s_TimezoneOffset);
      
      my_http.init(myHttpConfig);
	  }

  card_Title.update(wifiAPssid);
  card_status.update("Starting");
  dashboard.sendUpdates();

  // --- set time, callback for TimeLib to get the time, is called in given interval to sync
  setSyncInterval(300);             // note: not necessary as preset value of TimeLib is 300
  setSyncProvider(getLocalTime);    // callback for TimeLib, setSyncProvider() calls immediately getLocalTime() via now() in TimeLib


// --- Start clock and sensor
  delay(100);

// here we probably do not have a connection to NTP server yet. time is relative to boot until time from NTP server is received, typically after 40s

  getDateTime(s_DateTime);
  DEBUG_TRACE(true,"%s",s_DateTime);

    s_epochtime = String(getEpochTime());
    card_status.update("entering loop");
    card_Time.update(s_DateTime);
    card_EpochTime.update(s_epochtime);
    dashboard.sendUpdates();
    lastDateUpdate = millis();


  if(MY_TEST)
  {
    DEBUG("No Sensor used.");
  }
  else
  {
    // Setup reading heads
    DEBUG("Setting up %d configured sensors...", NUM_OF_SENSORS);
    const SensorConfig *config = SENSOR_CONFIGS;
    for (uint8_t i = 0; i < NUM_OF_SENSORS; i++, config++)
    {
      Sensor *sensor = new Sensor(config, process_message);
      sensors->push_back(sensor);
    }
    DEBUG("Sensor setup done.");
  }

  // start in AP mode
  currentSSID = String(wifiAPssid);
  currentIP = WIFI_AP_IP;

  DEBUG_TRACE(VERBOSE_LEVEL_Setup,"%s: Setup done.----------------------------------------------", s_DateTime);
    card_SensorStatus.update("setup done");
    card_status.update("Setup done");
    dashboard.sendUpdates();
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

}
//----------------------------------------------------------------------------------------------------

u32_t count;
u32_t count10000;
void loop()
{
	if (needReset)  // Doing a chip reset caused by config changes
	{
		DEBUG("Rebooting after 1 second.");
    needReset = false;
       // post to volkszaehler
    s_epochtime = String(getEpochTime());
    my_http.postHttp(String(confVZuuidSmlHeartBeatParam.valueBuffer), s_epochtime, HEART_BEAT_RESET);

		delay(1000);
		ESP.restart();
	}

  confWeb.doLoop();

  // need to wait until WiFi connection is established.
  if(b_WiFi_connected)
  {
    if(b_WiFi_firstTime)
    {
      if(millis() > 40000)  // we wait 40sec, then we should have a valid time
      {
        // first we need to get a valid time
        getDateTime(s_DateTime);
        epochtime = getEpochTime();
        s_epochtime = String(epochtime);
          card_Time.update(s_DateTime);
          card_EpochTime.update(s_epochtime);
          dashboard.sendUpdates();

        uint32_t waitForTime = 0;
        while( (waitForTime < 10000) && (epochtime < 1672531200ULL) )  // 1672531200ULL = 2023-01-01
        {
            epochtime = getEpochTime();
            waitForTime++;
        }

        my_http.postHttp(String(confVZuuidSmlHeartBeatParam.valueBuffer), s_epochtime, HEART_BEAT_WIFI_CONFIG);
        if(epochtime > 1672531200ULL)     // now we have a valid time
        {
          // here, we should have connection to ntp server and valid time
          b_WiFi_firstTime = false;
          setSyncProvider(getLocalTime);    // setting again will force TimeLib to sync with system time
          getDateTime(s_DateTime);
          card_Time.update(s_DateTime);
          card_EpochTime.update(s_epochtime);
          dashboard.sendUpdates();

          IPAddress result;
          if (WiFi.hostByName(myHttpConfig.vzServer, result))
          {
            vzServerIP = result.toString();
            DEBUG_TRACE(true,"vzServerIP = %s",vzServerIP.c_str());
          }
        }
      }
    }
    else
    {
      if(MY_TEST)
      {
        my_http.testHttp();
      }
      else
      {
        // Execute sensor state machines
        for (std::list<Sensor*>::iterator it = sensors->begin(); it != sensors->end(); ++it)
        {
          (*it)->loop();
        }
      }
    }
  }
	yield();

  // update dash board
  if((millis()- lastDateUpdate) > DATE_UPDATE_INTERVAL)  // diff of unsigned is positive, if millis() wraps around diff becomes big
  { 
    lastDateUpdate = millis();

    // update dashboard status
    count10000 = count/10000;
    s_loopCount = "in loop, #" + String(count10000);
    card_status.update(s_loopCount);
    getDateTime(s_DateTime);
    card_Time.update(s_DateTime);
    s_epochtime = String(getEpochTime());
    card_EpochTime.update(s_epochtime);
    dashboard.sendUpdates();

    if(MY_TEST)
    {
      vzTestValue = my_http.getValue(vzTEST);
      sprintf(myStringBuf,"%.5f",vzTestValue);
      card_energy.update(myStringBuf);
      dashboard.sendUpdates();
    }
    else
    {
      // heart beat post to volkszaehler
      if ((count10000 != HEART_BEAT_RESET) && (count10000 != HEART_BEAT_WIFI_CONFIG))
      {
        my_http.postHttp(String(confVZuuidSmlHeartBeatParam.valueBuffer), s_epochtime, count10000); // count10000 should fit into a float
      }
    }

  }

  if((millis()- lastDateUpdate)%5000 == 0)
  {
    String s_timestamp = String(getEpochTime());        // timestamp with resolution of 1 sec
    DEBUG_TRACE(MY_TEST,"loop timestamp = %s",s_timestamp.c_str() );
  }

  if(count%10000 == 0)
  {
    DEBUG_TRACE(VERBOSE_LEVEL_Loop,"loop_count=%d", count/10000);
  }
  count++;

    //delay(100);  // do not use delay as we loose sml messages
}   // loop()


// ##########################################################################################
// void process_message(byte *buffer, size_t len, Sensor *sensor, State sensorState)
// call back function for libSML
// process_message is a wrapper around the parse and publish method of class Sensor
//
// 2022-12-07 mh
// - sensor state to support update of dash board
//
// 2022-11-30 mh
// - adapted for http and dashboard update
//
void process_message(byte *buffer, size_t len, Sensor *sensor, State sensorState)
{
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

  if( sensorState == PROCESS_MESSAGE)
  {
    // Parse
    sml_file *file = sml_file_parse(buffer + 8, len - 16);

    if(VERBOSE_LEVEL_MeterProtocol) 
    {
      DEBUG_SML_FILE(file);     // output of received messages
    }
    my_http.publish(sensor, file);

    // free the malloc'd memory
    sml_file_free(file);

    // update dashboard
    s_timeStamp = my_http.getTimeStamp();
    powerIn = my_http.getValue(vzPOWER_IN);
    energyIn = my_http.getValue(vzENERGY_IN);
    energyOut = my_http.getValue(vzENERGY_OUT);

    card_status.update("data published");
    card_power.update(powerIn);
    sprintf(myStringBuf,"%.5f",energyIn/1000.);
    card_energy.update(myStringBuf);
    sprintf(myStringBuf,"%.5f",energyOut/1000.);
    card_energy2.update(myStringBuf);
    card_TimeStamp.update(s_timeStamp);
    dashboard.sendUpdates();

    if (VERBOSE_LEVEL_MeterData)
    {
      Serial.print("ts=");
      Serial.print(s_timeStamp);
      Serial.print("ms, ");
      Serial.print("P=");
      Serial.print(powerIn);
      Serial.print("W, ");
      Serial.print("E_in=");
      Serial.print(energyIn);
      Serial.print("Wh, ");
      Serial.print("E_out=");
      Serial.print(energyOut);
      Serial.println("Wh");
      Serial.flush();
    }
  }
  card_SensorStatus.update(sensorState);
  dashboard.sendUpdates();
      Serial.print("** Sensor State: ");
      Serial.println(sensorState);

  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
}



// ##########################################################################################
void configSaved()
//
// configSaved() callback handler for confWeb, when configuration was saved.
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
	DEBUG("Configuration was updated.");
	//needReset = true;   // mh: erstmal kein reset 
}
// ##########################################################################################

void wifiConnected()
{
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  currentSSID = WiFi.SSID();
  currentIP = WiFi.localIP().toString();
	DEBUG("WiFi connection established, SSID=%s, IP=%s",currentSSID.c_str(),currentIP.c_str());
  b_WiFi_connected = true;
}
// ##########################################################################################
void notFound(AsyncWebServerRequest *request)
{
  String _content = "<div style='padding-top:25px;'><b>NotFoundHandler: Page not found</b></div>";
  _content += "<div style='padding-top:25px;'><a href='/'>Return to Start Page '/start'</a></div>";
  request->send(404, "text/html", _content);
}
// ##########################################################################################
// request handler for /reset
void onReset(AsyncWebServerRequest *request)
//
// onReset() request handler for /reset
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  needReset = true;
  request->send(200, "text/html; charset=UTF-8", "Rebooting after 1 sec.");
}
// ##########################################################################################
// request handler for /start
void startHtml(AsyncWebServerRequest *request)
//
// indexHtml() provides an index page as anchor for a webpage
//
// 2022-12-30	mh
// - first version
//
// (C) M. Herbert, 2022.
// Licensed under the GNU General Public License v3.0
{
  String	content;
  if(b_WiFi_connected)
  {
    currentHtmlPage = "Local Net Start Page";
  }
  else
  {
    currentHtmlPage = "Access Point Start Page";    
  }

  homePage(content);
  request->send(200, "text/html; charset=UTF-8", content);
}
// ##########################################################################################
void onConfiguration(AsyncWebServerRequest *request)
//
// onConfiguration() handler for configuration page /config
//
// 2023-02-05 mh
// - remove webConfMode
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  
    DEBUG("onConfiguration:calling handleConfig()");
    confWeb.handleConfig(reinterpret_cast<WebRequestWrapper*>(request));
    //confWeb.handleConfig();
  
}
// ##########################################################################################
void handleRoot(AsyncWebServerRequest *request)
//
// handleRoot() handler for root page /
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  String	content;

  if(b_WiFi_connected)
  {
    currentHtmlPage = "Local Net Start Page";
  }
  else
  {
    currentHtmlPage = "Access Point Start Page";    
  }

  homePage(content);
  request->send(200, "text/html; charset=UTF-8", content);
}
// ##########################################################################################
void homePage(String& _content)
//
// homePage() compose a html home page
//
// 2023-01-26 mh
// - modified for SMLReader
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
//
// global variables used:
//  currentSSID, currentIP, currentHtmlPage, wifiAPssid
{
  #define MY_HTML_HEAD 		"<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{t}</title>"
  //#define MY_HTML_HEAD 		"<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>SMLReaderT</title>"
  #define MY_HTML_HEAD_END 	"</head><body>"
  #define MY_HTML_END 		"</div></body></html>"
  #define MY_HTML_HEADLINE		"<h1 style='padding-top:25px;'>Welcome to {h} Site</h1></a></div>"
  #define MY_HTML_START		"<div style='padding-top:25px;'><a href='/start'>Return to Start Page</a></div>"
  #define MY_HTML_CONFIG	"<div style='padding-top:25px;'><a href='/config'>Configuration Page</a></div>"
  #define MY_HTML_DASH		"<div style='padding-top:25px;'><a href='/'>Dash Board</a></div>"
  #define MY_RESET_HTML		"<div style='padding-top:25px;'><a href='/reset'>Reset ESP</a></div>\n"
  #define MY_HTML_CONFIG_VER "<div style='padding-top:25px;font-size: .6em;'>Version {v} {d}</div>"

  _content = MY_HTML_HEAD;
  _content += MY_HTML_HEAD_END;
  _content.replace("{t}", wifiAPssid);
  _content += MY_HTML_HEADLINE;
  _content.replace("{h}", wifiAPssid);
  _content += "You are connected to <b>" + currentSSID + "</b> with IP " + currentIP;
  _content += "<p> VZ-Server <b>" + String(myHttpConfig.vzServer) + "</b> with IP " + vzServerIP +"</p>";
  _content += "<h2 style='padding-top:25px;'>" + currentHtmlPage + "</h2></a></div>";
  _content += MY_HTML_START;
  _content += MY_HTML_CONFIG;
  _content += MY_HTML_DASH;
  _content += MY_RESET_HTML;
  _content += MY_HTML_CONFIG_VER;
  _content.replace("{v}", WIFI_AP_CONFIG_VERSION);
  _content.replace("{d}", MY_VERSION_TYPE);
  _content += MY_HTML_END;

}
// ##########################################################################################
// time helper functions

time_t getLocalTime()
//
// assumption is, that the system time tv is GMT, i.e. no timezones and no dayligth saving time offsets are set.
// time offset is applied manually based on configuration parameter
//
{
    timeval tv;
    DEBUG_TRACE_(VERBOSE_LEVEL_TIME,"Sync to Unix Timestamp: ");
    gettimeofday(&tv, NULL);                // use system time; note that usec also contains ms --> divide by 1000 to get ms
    DEBUG_TRACE(VERBOSE_LEVEL_TIME,"%lld",tv.tv_sec);
    return(tv.tv_sec+Timezone*3600);
}
time_t getEpochTime()
//
// return UNIX epoch time
{
    timeval tv;
    DEBUG_TRACE_(VERBOSE_LEVEL_Loop,"Unix Timestamp: ");
    gettimeofday(&tv, NULL);                // use local time; note that usec also contains ms --> divide by 1000 to get ms
    DEBUG_TRACE(VERBOSE_LEVEL_Loop,"%lld",tv.tv_sec);
    return(tv.tv_sec);
}
void getDateTime(char* s_DateTime)
//
// return a char string with the current date and time in the format
//    yyyy-mm-dd hh:mm
// set global variables Year,Month,Day,Hour,Minute
// uses TimeLib
//
// 2023-02-14 mh
// - first version 
{
  Year = year();
  Month = month();
  Day = day();
  Hour = hour();
  Minute = minute();
  sprintf(s_DateTime,"%4d-%02d-%02d %02d:%02d",Year,Month,Day,Hour,Minute);
}