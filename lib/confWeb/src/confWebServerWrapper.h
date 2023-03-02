// confWebServerWrapper.h - configuration page for WLAN access using AsyncWebServer
//
// 2023-01-21 mh
// - derived from IotWebConfWebServerWrapper.h
// - use ESPAsyncWebServer instead of ESP8266WebServer
// - original code part is framed by #if IOT_OLD ... #endif
//
// Copyright (C) 2023 Manfred Herbert


/**
 * IotWebConfWebServerWrapper.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef WebServerWrapper_h
#define WebServerWrapper_h

#include <Arduino.h>
#include <IPAddress.h>
#if IOT_OLD
#else
#include <ESPAsyncWebServer.h>
#endif

#if IOT_OLD
namespace iotwebconf
{

class WebRequestWrapper
{
public:
  virtual const String hostHeader() const;
  virtual IPAddress localIP();
  virtual uint16_t localPort();
  virtual const String uri() const;
  virtual bool authenticate(const char * username, const char * password);
  virtual void requestAuthentication();
  virtual bool hasArg(const String& name);
  virtual String arg(const String name);
  virtual void sendHeader(const String& name, const String& value, bool first = false);
  virtual void setContentLength(const size_t contentLength);
  virtual void send(int code, const char* content_type = nullptr, const String& content = String(""));
  virtual void sendContent(const String& content);
  virtual void stop();
};

#else // IOT_OLD
// here is the key change: WebRequestWrapper is derived from AsyncWebServerRequest.
// The IotWebConf class uses the WebRequestWrapper class to handle requests.
// This will use the methods of AsyncWebServerRequest now.
// Only few adaptations of IotWebConf and IotWebParameter are necessary.
class WebRequestWrapper: public AsyncWebServerRequest
{
  public:
  void doNothing() {};
  void sendContent(const String& content) {};  // for compatibility with IOT
  void sendHeader(const String& name, const String& value, bool first = false) {};
};

#endif // IOT_OLD

#if IOT_OLD
class WebRequestWrapper
{
public:
  virtual const String hostHeader() const;
  virtual IPAddress localIP();
  virtual uint16_t localPort();
  virtual const String uri() const;
  virtual bool authenticate(const char * username, const char * password);
  virtual void requestAuthentication();
  virtual bool hasArg(const String& name);
  virtual String arg(const String name);
  virtual void sendHeader(const String& name, const String& value, bool first = false);
  virtual void setContentLength(const size_t contentLength);
  virtual void send(int code, const char* content_type = nullptr, const String& content = String(""));
  virtual void sendContent(const String& content);
  virtual void stop();
};
#else
class WebServerWrapper
{
public:
//  virtual void handleClient();
  virtual void begin();
};
#endif // IOT_OLD

#if IOT_OLD
 } // end namespace
#endif

#endif