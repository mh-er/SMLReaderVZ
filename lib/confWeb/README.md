# confWeb Configurable web interface
##  Description

**confWeb** is a collection of classes that provide a configurable web interface for ESP8266.  
They are modifications of a subset of IotWebConf which is an ESP8266/ESP32 non blocking WiFi/AP web configuration library for Arduino, see
https://github.com/prampec/IotWebConf

## Features
- provides a WiFi access point at http://192.168.4.1 (or the specified network name) and a configuration page after boot (AP mode)  
- stores configuration in EEPROM  
- connects after a timeout of 30sec to a local WiFi if configured and the configuration page is not accessed (STA mode); accessible by MY_WIFI_AP_SSID (or IP address provided by your DHCP server)  
- supports a force to AP mode with *MY_WIFI_AP_DEFAULT_PASSWORD* in case of forgotten password by connecting *WEBCONF_AP_MODE_CONFIG_PIN* to ground  
- allows access to the configuration page from STA mode by login as *admin* and the configured AP password.
- change of CONFIG_VERSION invalidates the stored configuration in EEPROM.

## Usage 
An example main program is provide to show the usage.

#### Declarations/Definitions:
``` bash
DNSServer dnsServer;  
AsyncWebServer server(80);  
char wifiConfig[2];  

IotWebConf confWeb(MY_WIFI_AP_SSID, &dnsServer, &server, MY_WIFI_AP_DEFAULT_PASSWORD, CONFIG_VERSION);  
// custom configuration parameter  
NumberParameter wifiModeParam = NumberParameter("WiFi mode", "wifiMode", wifiConfig, sizeof(wifiConfig), nullptr, wifiConfig);  
ParameterGroup paramGroup = ParameterGroup("WiFi mode Settings", "");  
```

#### Setup:  
``` bash
// config pin for force mode - PIN to be pulled to ground during init()  
confWeb.setConfigPin(WEBCONF_AP_MODE_CONFIG_PIN);  
	// custom parameter  
paramGroup.addItem(&wifiModeParam);  
confWeb.addParameterGroup(&paramGroup);  
	// handler for web configuration  
confWeb.setConfigSavedCallback(&configSaved);  
confWeb.setWifiConnectionCallback(&wifiConnected);  
	// init  
boolean validConfig = confWeb.init();  
```
#### Loop:

``` bash
confWeb.doLoop();  
```
	
#### Handler:  
// call configuration page  
void onConfiguration(AsyncWebServerRequest \*request) {  
confWeb.handleConfig(reinterpret_cast<WebRequestWrapper\*>(request));  
}  


## Implementation Details

### Modules
confWeb consists of six files:  

	Configuration classes: confWeb.cpp, confWeb.h  
	Configuration HTML page: confWebParameter.cpp, confWebParameter.h  
	Wrapper class: confWebServerWrapper.h  
	Define settings: confWebSettings.h  

### Modification of IOTWebConf library
The original IOTWebConf library uses the ESP8266Webserver that allows only one client at a time.  
The modified confWeb interface uses ESP AsyncWebServer instead that supports multiple clients at a time.  
Main motivation of the modification was the usage of ESP-Dash that is bases on AsyncWebServer.

ESP8266Webserver includes both the server and the request methods in its class 
(The request methods perform the http operations).
IOTWebConf provides separated interfaces for server and requests by means of wrapper classes. However, the separation is not complete on implementation level because still ESP8266WebServer is to be used.
The data for ONE http transmission request are written in a kind of stream to a send buffer by different methods.

ESPAsyncWebServer separates the server and the request handling in two classes. Due to the asynchronous mode, the http request data is put in a *response* structure. It would be still possible to distribute this accross different methods as in the ESP8266WebServer case but changes in several methods would be necessary.

As the IOTWebConf library uses a wrapper around the web server and request class, it was possible to keep most code unchanged by doing three basic modifications:
- #define WebServer AsyncWebServer in *confWeb.h*
- derive class WebRequestWrapper from AsyncWebServerRequest in *WebServerWrapper.h*
- modify class StandardWebRequestWrapper in *confWeb.h* to use AsyncWebServerRequest instead of WebServer to handle http requests.

Only few methods with modified interfaces needed to be added, especially for the render-methods in *confWebParameter.cpp*.  
Additionally, few lines of code had to be changed. The biggest change is in *handleConfig()* within *confWeb.cpp*. 
Details can be found in the code by searching *IOT_OLD*

### confWebSettings.h
This file contains *#define* statements internally used by IOTWebConf, mainly field length and debug configuration.

### Configuration class in confWeb.cpp/h
Here, the configuration and the transition between WiFi AP and STA mode is handled.

**init()** checks the config PIN if AP mode is to be forced. Then it loads the configuration from the EEPROM, if available. A valid configuration is detected from the version information that is stored in the first 4 words of the EEPROM block.  

**loop()** implements a state machine for WiFi connection (NetworkState).


| Old State          | Event                                                 | New State                     |
|:-------------------|:-----------------------------------------------------:|------------------------------:|
| 0 Boot             | boot, startup = APmode                                | 2 APmode                      |  
| 0 Boot             | boot, update                                          | 3 Connecting                  |  
| 2 APmode           | checkConnection, setupAP                              | 2 APmode, AP mode             |  
| 2 APmode           | must use default password                             | 1 notConfigured               |  
| 2 APmode           | AP timeout, startup == Connecting                     | 3 Connecting                  |  
| 2 APmode           | startup == Offline                                    | 5 Offline, Disconnect         |  
| 3 Connecting       | checkWifConnection == 1                               | 4 Online                      |  
| 3 Connecting       | checkWifConnection == 0                               | 3 Connecting                  |  
| 4 Online           | not connected                                         | 3 Connecting                  |  
| 4 Online           | connected                                             | 4 Online, STA mode            |  
| 3 Connecting       | checkWifiConnection == 0                              | 3 Connecting                  |  
| 1 notConfigured    | AP timeout                                            | 3 Connecting                  |  
| 1 notConfigured    | checkConnection, setupAP                              | 2 APmode, AP mode             |  
|____________________|_______________________________________________________|_______________________________|

**handleConfig()** displays the configuration form page by calling **renderHtml()*. If it gets a response triggered by the submit button of the form (delivering an argument "iotSave"), it validates the length of the passwords (>=8) and saves the configuration to EEPROM.

**checkConnection()** checks if an AP client has connected. There is a small state machine ApConnectionState.

| Old State          | Event                                                 | New State                     |
|:-------------------|:-----------------------------------------------------:|------------------------------:|
| NoConnections      | checkConnection(), ApStationNum > 0                   | HasConnections                |  
| HasConnections     | checkConnection(), ApStationNum == 0                  | Disconnected                  |  
|____________________|_______________________________________________________|_______________________________|

Note: When an AP client connects the ApStationNum becomes > 0 and state changes to *HasConnections*. If the pathword is wrong, connection is refused. With the next check, state becomes *Disconnected*, the AP mode is stopped und switched to STA mode. Even if your clients prompts for a correct password, you will not be able to connect because this switch happens very fast.
Thus, when you change the AP login password, make sure to replace the password memory of your client for this WiFi connection because typing in would be to slow.


### ConfigItem and parameter classes in confWebParameter.cpp

The classes in *confWebParameter.cpp* handle the configuration parameters by linked lists of *ConfigItems*.
*ConfigItem* is the base class from which different parameter classes are derived.  

	ConfigItem
	|-- Parameter
	|   |-- TextParameter
	|   |   |-- NumberParameter
	|   |   |-- PasswordParameter
	|   |   |-- CheckboxParameter
	|   |   |-- OptionsParameter
	|   |   |	|-- SelectParameter
	|-- ParameterGroup


**renderHtml()** is the method of all classes (derived by means of overriding and overloading) that creates the content for the HTML configurtion page. Basically, the polymorphic character of the derived methods is used on top level to apply the method matching the object type.

**TextParameter::renderHtml**(const char* type, bool hasValueFromPost, String valueFromPost)  
is the method finally called by all others (in a loop for each parameter). If it is called with *hasValueFromPost*, the configuration form was submitted and its data is used for rendering the configuration page. Otherwise, loadConfig data from the EEPROM is used, accessing *valueBuf* of the object.

### Callback functions and handlers


URL-not-found web request handler. Used for handling captive portal request.

	void handleNotFound(WebRequestWrapper* webRequestWrapper);
	void handleNotFound()
	{
	  WebRequestWrapper* webRequestWrapper;
	  handleNotFound(webRequestWrapper);
	}

Specify a callback method, that will be called upon WiFi connection success.
Should be called before init()!

	void setWifiConnectionCallback(std::function<void()> func);

Specify a callback method, that will be called when settings is being changed.
This is very handy if you have other routines, that are modifying the "EEPROM"
parallel to IotWebConf, now this is the time to disable these routines.
Should be called before init()!

	void setConfigSavingCallback(std::function<void(int size)> func);

Specify a callback method, that will be called when settings have been changed.
All pending EEPROM manipulations are done by the time this method is called.
Should be called before init()!

	void setConfigSavedCallback(std::function<void()> func);

Specify a callback method, that will be called when form validation is required.
If the method will return false, the configuration will not be saved.
Should be called before init()!

	void setFormValidator(std::function<bool(WebRequestWrapper* webRequestWrapper)> func);

Specify your custom Access Point connection handler. Please use IotWebConf::connectAp() as
reference when implementing your custom solution.

	void setApConnectionHandler(
	  std::function<bool(const char* apName, const char* password)> func)
	{
	  _apConnectionHandler = func;
	}

Specify your custom WiFi connection handler. Please use IotWebConf::connectWifi() as
reference when implementing your custom solution.
Your method will be called when IotWebConf trying to establish
connection to a WiFi network.

	void setWifiConnectionHandler(
	  std::function<void(const char* ssid, const char* password)> func)
	{
	  _wifiConnectionHandler = func;
	}

# License
Copyright (C) M. Herbert, 2023.  
Licensed under the GNU General Public License v3.0

# References
[1] https://github.com/prampec/IotWebConf  





