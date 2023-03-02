// main.cpp WebIF - a demo program to use confWeb and ESP-Dash
//
// configuration of a web interface and realization of a dash board
//
// 2023-01-24 mh
// - first version
// - derived from  SMLReader, see https://github.com/mruettgers/SMLReader
//
// Copyright (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
//

/*
# WebIF - configurable web interface and dash board for ESP8266
##  Description

The program provides a Web Interface based on [IotWebConf](https://github.com/prampec/IotWebConf).
This library has been modified to support ESPAsyncWebServer instead of ESP8266WebServer.
Thus, multiple clients can be served asynchronously and in parallel.

# Features

## Web Interface
The program provides a Web Interface based on [IotWebConf](https://github.com/prampec/IotWebConf).
This library has been modified to support ESPAsyncWebServer instead of ESP8266WebServer.
Thus, multiple clients can be served asynchronously and in parallel.

### Web Access Point (WiFI AP mode)
The program provides a Web Access Point at boot time via http://192.168.4.1 using the configured password.
It offers a configuration page both for the Access Point and a local WLAN for SSID name and password.
Configuration is stored in EEPROM.
At initial boot, the defined default password is used for AP mode access.

If no client connects before the timeout (configured to 30sec), the device will automatically continue in STA (station) mode
and connect to a local WLAN if configured.

### Web Interface in a local WLAN (WiFi STA mode)
If the device has already been configured, the web interface can be reached via the IP address obtained from your local network's DHCP server.
A small Home page is provided which offers access to the configuration page as well.
To login, provide the user *admin* and the configured AP *password*.

## Dash Board
A demo dash board is acessible both from AP and STA mode.
It is based on https://github.com/ayushsharma82/ESP-DASH, with only minor modification.

- the root path was modified from "/" to "/dash"
- a link "return to home page" with referende to "/" was added (replacing the link to "espdash.pro").

  *** end description *** */
#include <Arduino.h>
#include "EEPROM.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <DNSServer.h>

#include "main.h"
#include "config.h"
#include "confWeb.h"
#include "confWebParameter.h"
#include <FormattingSerialDebug.h>    // part of MicroDebug
#include "timeControl.h"

#define WEB_CONF_MODE_AP 0
#define WEB_CONF_MODE_STA 1
#define WEB_CONF_MODE_AP_STA 2

// select which mode to use
//int webConfMode = WEB_CONF_MODE_AP;
//int webConfMode = WEB_CONF_MODE_STA;
int webConfMode = WEB_CONF_MODE_AP_STA;
boolean inSTAmode = false;

// callback handler and html page functions
void connectSTA();
void wifiConnected();
void configSaved();
void startHtml(AsyncWebServerRequest *request);
void onConfiguration(AsyncWebServerRequest *request);
void onReset(AsyncWebServerRequest *request);
void onConnect(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);
void handleRoot(AsyncWebServerRequest *request);
void onDashFrontend(AsyncWebServerRequest *request);
void homePage(String& content);

boolean needReset = false;
boolean closeAPrequested = false;
boolean traceLoop = true;   // used to send loop trace output only once 

// workaround for missing String method
String toStringIp(IPAddress ip);

DNSServer dnsServer;
AsyncWebServer server(80);

// WiFiClient net;		// ??
String currentHtmlPage;     // sub-headline for different modes
String currentSSID = "unknown";
String currentIP   = "unknown";
char c_wifiIP[40];

timeControl Clock(NTP_SERVER_IP,TIMEZONE,7200);
uint16_t Year = 2000;
uint8_t Month = 0;
uint8_t Day = 0;
uint8_t Hour = 0;
uint8_t Minute = 0;

uint64_t epochtime;   // in seconds - not ms to avoid overrun
String s_timeStamp;   // in seconds
String s_DateTime;

uint32_t currentMillisTime;
String s_currentMillisTime;
uint32_t lastDateUpdate;

char wifiConfig[2];   // buffer for config entry


// class for WiFi and webserver configuration page, connects to WiFi in AP or STA mode
// note: constructor does some presets
IotWebConf confWeb(MY_WIFI_AP_SSID, &dnsServer, &server, MY_WIFI_AP_DEFAULT_PASSWORD, CONFIG_VERSION);

NumberParameter wifiModeParam = NumberParameter("WiFi mode", "wifiMode", wifiConfig, sizeof(wifiConfig), nullptr, wifiConfig);
ParameterGroup paramGroup = ParameterGroup("WiFi mode Settings", "");

boolean switch2WifiConfigMode = true;

// it is possible to add additional configuration parameters, e.g.
/**
TextParameter mqttServerParam = TextParameter("MQTT server", "mqttServer", mqttConfig.server, sizeof(mqttConfig.server), nullptr, mqttConfig.server);
NumberParameter mqttPortParam = NumberParameter("MQTT port", "mqttPort", mqttConfig.port, sizeof(mqttConfig.port), nullptr, mqttConfig.port);
PasswordParameter mqttPasswordParam = PasswordParameter("MQTT password", "mqttPassword", mqttConfig.password, sizeof(mqttConfig.password), nullptr, mqttConfig.password);
ParameterGroup paramGroup = ParameterGroup("MQTT Settings", "");

add the parameter in setup() before init()
	paramGroup.addItem(&mqttServerParam);
	paramGroup.addItem(&mqttPortParam);
	paramGroup.addItem(&mqttPasswordParam);

	confWeb.addParameterGroup(&paramGroup);

**/

// Dash board and dash board card definitions
ESPDash dashboard(&server);
Card card_Time(&dashboard, GENERIC_CARD, "Date & Time");
Card card_TimeStamp(&dashboard, GENERIC_CARD, "Time Stamp (s)");
Card card_status(&dashboard, STATUS_CARD, "Loop Status", "empty");

void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);                         // Pin2 = D4  = LED_BUILTIN, LOW means ON
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  confWeb.setConfigPin(WEBCONF_AP_MODE_CONFIG_PIN);  // used to enter config mode if PIN is pulled to ground during init()

#ifdef DEBUG
	// Delay for getting a serial console attached in time
	delay(2000);
#endif

  Serial.begin(115200);       // start serial output to monitor
  Serial.println(" ");
  Serial.println("Hello");


  //--- Start AsyncWebServer + Handler --------------------------------------------------------------
//  server.onNotFound([](AsyncWebServerRequest *request) {notFound();});
  server.onNotFound(notFound);

  server.on("/", handleRoot);
  server.on("/index.html", startHtml);
  server.on("/start", startHtml);
  server.on("/config", onConfiguration);
  server.on("/connect", onConnect);
  server.on("/reset", onReset);
  // note: "/dash" is handles by ESP-Dash
  server.on("/dashfrontend", onDashFrontend);

//  WiFi event handler: Subscribe to specific event and get event information as an argument to the callback
/**     WiFiEventHandler onStationModeConnected(std::function<void(const WiFiEventStationModeConnected&)>);
        WiFiEventHandler onStationModeDisconnected(std::function<void(const WiFiEventStationModeDisconnected&)>);
        WiFiEventHandler onStationModeAuthModeChanged(std::function<void(const WiFiEventStationModeAuthModeChanged&)>);
        WiFiEventHandler onStationModeGotIP(std::function<void(const WiFiEventStationModeGotIP&)>);
        WiFiEventHandler onStationModeDHCPTimeout(std::function<void(void)>);
        WiFiEventHandler onSoftAPModeStationConnected(std::function<void(const WiFiEventSoftAPModeStationConnected&)>);
        WiFiEventHandler onSoftAPModeStationDisconnected(std::function<void(const WiFiEventSoftAPModeStationDisconnected&)>);
        WiFiEventHandler onSoftAPModeProbeRequestReceived(std::function<void(const WiFiEventSoftAPModeProbeRequestReceived&)>);
        WiFiEventHandler onWiFiModeChange(std::function<void(const WiFiEventModeChange&)>);
        Example
        WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
        publisher.disconnect();
      });
**/

  WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& event)
  {
    currentSSID = MY_WIFI_AP_SSID;
    currentIP = toStringIp(WiFi.softAPIP());
    inSTAmode = false;
  });

  WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event)
  {
    currentSSID = WiFi.SSID();
        currentIP = toStringIp(WiFi.localIP());
    inSTAmode = true;
  });

    WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
          //        here we can do some clean up.
          inSTAmode = false;
      });

    // own config parameter group
    paramGroup.addItem(&wifiModeParam);
    confWeb.addParameterGroup(&paramGroup);

    // handler for web configuration
    confWeb.setConfigSavedCallback(&configSaved);
    confWeb.setWifiConnectionCallback(&wifiConnected);

    // -- Define how to handle updateServer calls.
    //    confWeb.setupUpdateServer([](const char *updatePath) { httpUpdater.setup(&server, updatePath); },
    //                                 [](const char *userName, char *password){ httpUpdater.updateCredentials(userName, password); });

  if(webConfMode == WEB_CONF_MODE_AP)
  {
// --- provide a local access point ------------------------------------------------------
    DEBUG_TRACE(VERBOSE_LEVEL_WLAN,"setting up: %s",MY_WIFI_AP_SSID);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(MY_WIFI_AP_SSID, MY_WIFI_AP_DEFAULT_PASSWORD);
    currentSSID = MY_WIFI_AP_SSID;
    currentIP = toStringIp(WiFi.softAPIP());
    inSTAmode = false;
    DEBUG_TRACE(VERBOSE_LEVEL_WLAN,"Established. IP address: %s",currentIP.c_str());

    server.begin();
  }
// --- connect to a WiFI network
  else if(webConfMode == WEB_CONF_MODE_STA)
  {
    connectSTA();
    server.begin();
  }
// --- we start in AP mode to allow configuration of access info and switch to STA mode after timeout.
  else
  {
    boolean validConfig = confWeb.init();
    if (!validConfig)
    {
      DEBUG("Missing or invalid config. Possibly unable to initialize all functionality.");
    }
    else
    {
      DEBUG("Setup: valid config.");
      // here we can do some setup stuff that requires valid configuration data
	  }
  }

  card_status.update("Starting");
  dashboard.sendUpdates();

// --- Start clock and ticker, inverter access 
  Clock.begin();

  delay(2000);   // ms

  Clock.getDate(&Year, &Month, &Day);
  Clock.getTime(&Hour, &Minute);
  s_DateTime = Clock.getDateTime();

//  Clock.getEpochTime(&epochtime);
//  s_timeStamp = String(epochtime);

  currentMillisTime = millis();
  s_timeStamp = String(currentMillisTime);
    card_status.update("1st get started");
    card_Time.update(s_DateTime);
    card_TimeStamp.update(s_timeStamp);
    dashboard.sendUpdates();

	DEBUG("Setup done.");
  DEBUG_TRACE(VERBOSE_LEVEL_Setup,"---  end of setup()  ---------------------------------------------");

  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
	lastDateUpdate = millis();
}

void loop()
{
  Clock.loop();

	if (needReset)
	{
		// Doing a chip reset caused by config changes
		DEBUG("Rebooting after 1 second.");
    needReset = false;
		delay(1000);
		ESP.restart();
	}
  if(webConfMode == WEB_CONF_MODE_AP_STA)
  {
    confWeb.doLoop();
  }
  else
  {
    if (closeAPrequested)
    {
      delay(2000);

      DEBUG("stop server and access point");
      delay(2000);
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      DEBUG("Ready to connect to network");
      connectSTA();
      closeAPrequested = false;
      webConfMode = WEB_CONF_MODE_STA;
    }
  }

  // do something in loop
	uint32_t count;
	count = 0;
	for (int i=0;i<1000; i++)
	{
		count++;
	}
  if( webConfMode == WEB_CONF_MODE_AP_STA)
  {
    if(WiFi.status() != WL_CONNECTED)
    {
      // we are probably in AP mode
      currentSSID = MY_WIFI_AP_SSID;
      currentIP = toStringIp(WiFi.softAPIP());
      inSTAmode = false;
    }
    else
    {
      currentSSID = WiFi.SSID();
      currentIP = toStringIp(WiFi.localIP());
      inSTAmode = true;
    }
  }
  else if(webConfMode == WEB_CONF_MODE_STA)
  {
    if(WiFi.status() != WL_CONNECTED)
    {
      DEBUG("Loop: Waiting for WiFi connection ");
      // wait 5 seconds for connection:
      delay(5000);
    }
  }
  else
  { 
      delay(5000);
  }
  if(traceLoop)
  {
    DEBUG_TRACE(VERBOSE_LEVEL_WLAN,"--- in Loop, webConfMode = %d",webConfMode);
    traceLoop = false;
  }

  // check update counter
  currentMillisTime = millis();
  if((currentMillisTime - lastDateUpdate > 5000) || (currentMillisTime < lastDateUpdate)) // 5s update or when overrun
  {
    if( inSTAmode )
    {
      // check if time from ntp server received, otherwise update
      if(Year < 2000u)
      {
        Clock.forceNTPupdate();
        Clock.getDate(&Year, &Month, &Day);
        Clock.getTime(&Hour, &Minute);
      }
      s_DateTime = Clock.getDateTime();
      card_Time.update(s_DateTime);
    }
    lastDateUpdate = currentMillisTime;
    s_currentMillisTime = String(currentMillisTime/1000);
      card_status.update("In Loop");
      card_TimeStamp.update(s_currentMillisTime);
      dashboard.sendUpdates();
  }
	yield();
}
// ##########################################################################################
void connectSTA()
//
// connectSTA() connect to WiFi in STA mode
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
// --- establish WiFi connection to a network ------------------------------------------------------
  DEBUG_TRACE(VERBOSE_LEVEL_WLAN,"Connecting to: %s",MY_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(MY_SSID, MY_PASSWORD);
  WiFi.hostname(MY_HOSTNAME);

  uint8_t wifiCnt=0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (VERBOSE_LEVEL_WLAN) {Serial.print(".");}
    wifiCnt++;
    if (wifiCnt == 254)
    {
      delay(120000); // 120s
      ESP.restart(); // Restart ESP
    }
    delay(500);
  }
  currentSSID = WiFi.SSID();
  currentIP = toStringIp(WiFi.localIP());
  inSTAmode = true;
  DEBUG_TRACE(VERBOSE_LEVEL_WLAN,"\n Connected. IP address: %s",currentIP.c_str());
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
	DEBUG("WiFi connection established.");
}
// ##########################################################################################

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "NotFoundHandler: Page not found");
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
void onConnect(AsyncWebServerRequest *request)
//
// onConnect() handler for page /connect in STA mode
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  if( webConfMode == WEB_CONF_MODE_AP)
  {
  	DEBUG("Connection to network requested.");
    request->send(200, "text/html; charset=UTF-8", "Closing Access Point and connecting to Web-Site.");
    closeAPrequested = true;
    webConfMode = WEB_CONF_MODE_STA;
  }
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
  currentHtmlPage = "Start";

  homePage(content);
  request->send(200, "text/html; charset=UTF-8", content);

}
// ##########################################################################################
void onConfiguration(AsyncWebServerRequest *request)
//
// onConfiguration() handler for configuration page /config
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  if( webConfMode == WEB_CONF_MODE_AP_STA)
  {
    DEBUG("onConfiguration:calling handleConfig()");
    confWeb.handleConfig(reinterpret_cast<WebRequestWrapper*>(request));
    //confWeb.handleConfig();
  }
  else
  {
    String content;
    homePage(content);
    request->send(200, "text/html; charset=UTF-8", content);
  }
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

  if(webConfMode == WEB_CONF_MODE_AP)
  {
    currentHtmlPage = "Access Point Start Page";
  }
  else if(webConfMode == WEB_CONF_MODE_STA)
  {
    currentHtmlPage = "Local Net Start Page";
  }
  else
  {
    currentHtmlPage = "Access Point to Local Net Start Page";
  }

  homePage(content);
  request->send(200, "text/html; charset=UTF-8", content);
}
// ##########################################################################################
void homePage(String& _content)
//
// homePage() compose a html home page
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
//
// global variables used:
//  currentSSID, currentIP, currentHtmlPage
{
  #define MY_HTML_HEAD 		"<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>WebIF</title>"
  #define MY_HTML_HEAD_END 	"</head><body>"
  #define MY_HTML_END 		"</div></body></html>"
  #define MY_HEADLINE_HTML		"<h1 style='padding-top:25px;'>Welcome to WebIF Test Side</h1></a></div>"
  #define MY_START_HTML		"<div style='padding-top:25px;'><a href='/'>Return to Start Page</a></div>"
  #define MY_CONFIG_HTML	"<div style='padding-top:25px;'><a href='/config'>Configuration Page</a></div>"
  #define MY_DASH_HTML		"<div style='padding-top:25px;'><a href='/dash'>Dash Board</a></div>"
  #define MY_CONNECT_HTML		"<div style='padding-top:25px;'><a href='/connect'>Connect to network</a></div>"
  #define MY_RESET_HTML		"<div style='padding-top:25px;'><a href='/reset'>Reset ESP</a></div>"

  _content = MY_HTML_HEAD;
  _content += MY_HTML_HEAD_END;
  _content += MY_HEADLINE_HTML;
  _content += "You are connected to " + currentSSID + " with IP " + currentIP;
  _content += "<h2 style='padding-top:25px;'>" + currentHtmlPage + "</h2></a></div>";
  _content += MY_START_HTML;
  _content += MY_CONFIG_HTML;
  _content += MY_DASH_HTML;
  _content += MY_CONNECT_HTML;
  _content += MY_RESET_HTML;
  _content += MY_HTML_END;

}
// ##########################################################################################
void onDashFrontend(AsyncWebServerRequest *request)
//
// onDashFrontend() handler to display a web page in gzip format located in program memory
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
     // respond with the compressed frontend
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", DASH_HTML, sizeof(DASH_HTML));
    response->addHeader("Content-Encoding","gzip");
    request->send(response);     
}

String toStringIp(IPAddress ip)
//
// toStringIp() converts data type IPAddress to a string
//
// 2023-01-22	mh
// - version copied from confWeb.cpp
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}