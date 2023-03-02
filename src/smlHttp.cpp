#include <string.h>
#include <ESP8266HTTPClient.h>
#include "config.h"
#include "smlHttp.h"
#include "smlDebug.h"

/* *** smlHttp.cpp

transfer data to and from a web server

2023-02-27 mh
- split up input for server url
- not transmission, if uuid = VZ_UUID_NO_SEND

2023-02-05 mh
- rename obisValue to UuidValueName

2023-01-26 mh
- added setServerName(), removed VZ_SERVER and VZ_UUID_*
- added init(SmlHttpConfig &config)
- added testHttp(): test channel to database
- rename from MyHttp.cpp/.h to smlHttp.cpp/.h and class to SmlHttp because strong dependance on smlLib.

2022-12-08 mh
- use double value for postHttp to avoid loose of accurancy of SML meter data

2022-11-30 mh
- added method publish to evaluate and transfer SML message data to Volkszaehler
- based on https://github.com/mruettgers/SMLReader changing the mqtt publish method

2022-11-09	M. Herbert
- based on https://randomnerdtutorials.com/esp8266-nodemcu-http-get-post-arduino/ (see Copyright notice below)
- modified for own use


Copyright (C) M. Herbert, 2022-2023.
Licensed under the GNU General Public License v3.0

*** end change log *** */

/* ***
# Description of class SmlHttp #
http transfer of a data tupel (timestamp,value) of a sensor channel to the Volkszaehler data base via middleware.php  
- timestamp is Unix epoch time in seconds.
- sensor channel is defined by its data base UUID.

This class replaces class MqttPublisher that was used in https://github.com/mruettgers/SMLReader  as a http server is used instead of an MQTT broker.  
The http implementation follows the example in https://RandomNerdTutorials.com/esp8266-nodemcu-http-get-post-arduino/N_UUID_VALUE.  

## Usage ##
```bash
myHttp.init(SmlHttpConfig &config)              // initialize class with server name and channel UUIDs
myHttp.postHttp(vzUUID, s_timeStamp, value);    // post value to Volkszaehler
myHttp.publish(sensor, file);                   // evaluate and filter SML file messages and call postHttp()
myHttp.testHttp();                              // create test output and call postHttp()
myHttp.getTimeStamp();                          // returns TimeStamp string
myHttp.getValue(UuidValueName _select);             // returns selected Obis value of an SML message, valid only with publish()
```
Server name and Volkszaehler channel UUIDs are provided via struct SmlHttpConfig.

## Implementation ##

postHttp():  
For transfer to volkszaehler, the http transfer should look like this:
```bash
// http://volks-raspi/middleware.php/data.json?uuid=ae53c580-5549-11ed-84a0-cfe6bdf4d646&operation=add&ts=1666801000000&value=22
```

Implementation is done using class HTTPClient.

publish():  
The publish() method evaluates the SML messages of the SML file structure extracting Obis name of channels and the data.  
The timestamp is created locally based on the system time.  
Sensor is only used to extract configuration data (name of meter, numeric flag).

*** end description *** */

/*
  License notice Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-http-get-post-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  
  Code compatible with ESP8266 Boards Version 3.0.0 or above 
  (see in Tools > Boards > Boards Manager > ESP8266)
*/

String baseTopic="";      // was used as root for MQTT
WiFiClient _client;
HTTPClient http;
// const char* _serverName="http://volks-raspi/middleware.php/data.json";

SmlHttp::SmlHttp()
{
  uint16_t i;
  for (i=0;i<N_UUID_VALUE;i++)
  {
    _value[i]=0.;
  }
}

void SmlHttp::init(SmlHttpConfig &config) {
  _serverName = String(config.vzServer);
  _middlewareName = String(config.vzMiddleware);
  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"vzServer: %s",_serverName.c_str());
  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"vzMiddleware: %s",_middlewareName.c_str());

  uint16_t i;
  for (i=0;i<N_UUID_VALUE;i++)
  {
    _uuid[i] = &config.uuidValue[i][0];
    DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"uuid[%d] = %s",i,_uuid[i]);
  }

};
void SmlHttp::setServerName(String serverName) {
  _serverName = serverName;
};

void SmlHttp::setMiddlewareName(String middlewareName)
{
  _middlewareName = middlewareName;
};

int SmlHttp::postHttp(String vzUUID, String timeStamp, double value)
{
    //For transfer to volkszaehler, the http transfer should look like this:
    // http://volks-raspi/middleware.php/data.json?uuid=ae53c580-1234-5678-90ab-cdefghijklmn&operation=add&ts=1666801000000&value=22

  if(!strcmp(vzUUID.c_str(),VZ_UUID_NO_SEND))
  {
    return -99;
  }
  String vzUrl = "http://";
  vzUrl += _serverName + "/" + _middlewareName;
  vzUrl += "/" + String(VZ_DATA_JSON);

  if(!http.begin(_client, vzUrl))
  {
    DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"No connection to %s",_serverName.c_str());
    return 404;
  };

  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  this->_TimeStamp = timeStamp + "000";    // store internally in ms

  // HTTP request with a content type: x-www-form-urlencoded
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  //construct the message body
  //example for data to be sent: uuid=ae53c580-1234-5678-90ab-cdefghijklmn&operation=add&ts=1666801000000&value=22

   String httpRequestData = "uuid=" + vzUUID;
   String httpOperation = "&operation=add";
   
   String s_timestamp = "&ts=";
   s_timestamp += timeStamp;
   s_timestamp += "000";              // convert seconds to milli seconds
   
   String s_value = "&value="; 
   s_value += String(value);
   
   httpRequestData = httpRequestData + s_timestamp + s_value;

  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"Post message: %s",httpRequestData.c_str());

  int httpResponseCode = http.POST(httpRequestData);

  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"HTTP Response code: %d",httpResponseCode);
      
  // Free resources
  http.end();
  return httpResponseCode;
};

void SmlHttp::publish(Sensor *sensor, sml_file *file)
{

    for (int i = 0; i < file->messages_len; i++)
    {
      sml_message *message = file->messages[i];
      if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE)
      {
        sml_list *entry;
        sml_get_list_response *body;
        body = (sml_get_list_response *)message->message_body->data;
        for (entry = body->val_list; entry != NULL; entry = entry->next)
        {
          if (!entry->value)
          { // do not crash on null value
            continue;
          }

          char obisIdentifier[32];
          char buffer[255];
          String s_timestamp;
          String _vzUUID;

          sprintf(obisIdentifier, "%d-%d:%d.%d.%d*%d",              // adapted to VZ, original was: "%d-%d:%d.%d.%d/%d"
                  entry->obj_name->str[0], entry->obj_name->str[1],
                  entry->obj_name->str[2], entry->obj_name->str[3],
                  entry->obj_name->str[4], entry->obj_name->str[5]);

          // construction of MQTT path - currently used only for DEBUG
          String entryTopic = baseTopic + "sensor/" + (sensor->config->name) + "/obis/" + obisIdentifier + "/";

          // check for time stamp or use local time
          // Note: my meter does not send time, therefore we use local time
#if 0
          struct timeval tv;      // defined in time.h
          if (entry->val_time) { /* use time from meter */
            tv.tv_sec = *entry->val_time->data.timestamp;
            tv.tv_usec = 0;
            DEBUG("Use meter time %ldsec %ldus",tv.tv_sec,tv.tv_usec);
          } else {
            gettimeofday(&tv, NULL); /* use local time */
            DEBUG("Use local time %ldsec %ldus",tv.tv_sec,tv.tv_usec);
          }
#endif
          struct timeval tv;                      // defined in time.h
          gettimeofday(&tv, NULL);                // use local time; note that usec also contains ms --> divide by 1000 to get ms
          s_timestamp = String(tv.tv_sec);        // timestamp with resolution of 1 sec

          if (((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_INTEGER) ||
              ((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_UNSIGNED))
          {
            double value = sml_value_to_double(entry->value);
            int scaler = (entry->scaler) ? *entry->scaler : 0;
            int prec = -scaler;
            if (prec < 0)
              prec = 0;
            value = value * pow(10, scaler);
            sprintf(buffer, "%.*f", prec, value);
            DEBUG("%s: %s",entryTopic.c_str(), buffer);   // buffer contains the value as string in float format

//            publish(entryTopic + "value", buffer);   /* old, for MQTT */

            // we publish only numeric data, other parts below are kept for future use
            // we are interested only in specific data

            if( 0 == strcmp(obisIdentifier,OBIS_ID_ENERGY_IN))
            {
              _vzUUID = String(_uuid[vzENERGY_IN]);
              this->postHttp(_vzUUID, s_timestamp, value);
              // this->_TimeStamp = s_timestamp; done in postHttp()
              this->_value[vzENERGY_IN] = value;
            }
            else if( 0 == strcmp(obisIdentifier,OBIS_ID_ENERGY_OUT))
            {
              _vzUUID = String(_uuid[vzENERGY_OUT]);
              this->postHttp(_vzUUID, s_timestamp, value);
              // this->_TimeStamp = s_timestamp;  done in postHttp()
              this->_value[vzENERGY_OUT] = value;
            }
            else if( 0 == strcmp(obisIdentifier,OBIS_ID_POWER_IN))
            {
              _vzUUID = String(_uuid[vzPOWER_IN]);
              this->postHttp(_vzUUID, s_timestamp, value);
              // this->_TimeStamp = s_timestamp;  // done in postHttp()
              this->_value[vzPOWER_IN] = value;
            }
            else
            {
              /* do nothing */
            }
          }
          else if (!sensor->config->numeric_only)
          {
            if (entry->value->type == SML_TYPE_OCTET_STRING)
            {
              char *value;
              sml_value_to_strhex(entry->value, &value, true);
//             publish(entryTopic + "value", value);
              DEBUG("%s: %s",entryTopic.c_str(), value);

              free(value);
            }
            else if (entry->value->type == SML_TYPE_BOOLEAN)
            {
//              publish(entryTopic + "value", entry->value->data.boolean ? "true" : "false");
              DEBUG("%s: %s",entryTopic.c_str(), entry->value->data.boolean ? "true" : "false");
            }
          }
        }
      }
    }
}

String SmlHttp::getTimeStamp()
{
  return _TimeStamp;
}
double SmlHttp::getValue(UuidValueName _select)
{
  return _value[_select];
}
void SmlHttp::testHttp()
//
// 2023-01-26 mh
// - test channel to database, use system time as test data
{
  static uint32_t lastSendTime=0;
  uint32_t currentTime;
  currentTime = millis();
  if((currentTime-lastSendTime) > MY_TEST_SEND_UPDATE)
  {
    lastSendTime = currentTime;
    String _vzUUID = String(_uuid[vzTEST]);
      struct timeval tv;                      // defined in time.h
      gettimeofday(&tv, NULL);                // use local time; note that usec also contains ms --> divide by 1000 to get ms
      String s_timestamp = String(tv.tv_sec);        // timestamp with resolution of 1 sec

    this->postHttp(_vzUUID, s_timestamp, double(currentTime/1000.));
    // this->_TimeStamp = s_timestamp;  // done in postHttp()
    this->_value[vzTEST] = double(currentTime/1000.);
  }
}
