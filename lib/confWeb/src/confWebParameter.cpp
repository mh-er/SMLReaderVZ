// confWebParameter.cpp - configuration page for WLAN access access using AsyncWebServer
//
// 2023-01-21 mh
// - derived from IotWebConfParameter.cpp
// - use ESPAsyncWebServer instead of ESP8266WebServer
// - original code part is framed by #if IOT_OLD ... #endif
//
// Copyright (C) 2023 Manfred Herbert

/**
 * IotWebConfParameter.cpp -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#if IOT_OLD
#include <Arduino.h>
#include <functional>
#include <IotWebConfSettings.h>
#include <IotWebConfWebServerWrapper.h>

#ifdef IOTWEBCONF_ENABLE_JSON
# include <ArduinoJson.h>
#endif

#else  // IOT_OLD
#include "confWebParameter.h"
#endif  // IOT_OLD

#if IOT_OLD
namespace iotwebconf
{
#endif

ParameterGroup::ParameterGroup(
  const char* id, const char* label) :
  ConfigItem(id)
{
  this->label = label;
}

void ParameterGroup::addItem(ConfigItem* configItem)
{
  if (configItem->_parentItem != nullptr)
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("Item must not be added two times.: ");
    Serial.println(configItem->_id);
#endif
    return; // Item must not be added two times.
  }
  if (this->_firstItem == nullptr)
  {
    this->_firstItem = configItem;
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("First item added: ");
    Serial.println(configItem->_id);
#endif
    return;
  }
  ConfigItem* current = this->_firstItem;
  while (current->_nextItem != nullptr)
  {
        current = current->_nextItem;
  }
  current->_nextItem = configItem;
  configItem->_parentItem = this;
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("Next item added: ");
    Serial.println(configItem->_id);
#endif
}

int ParameterGroup::getStorageSize()
{
  int size = 0;
  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    size += current->getStorageSize();
    current = current->_nextItem;
  }
  return size;
}
void ParameterGroup::applyDefaultValue()
{
  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    current->applyDefaultValue();
    current = current->_nextItem;
  }
}

void ParameterGroup::storeValue(
  std::function<void(SerializationData* serializationData)> doStore)
{
  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    current->storeValue(doStore);
    current = current->_nextItem;
  }
}
void ParameterGroup::loadValue(
  std::function<void(SerializationData* serializationData)> doLoad)
{
  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    current->loadValue(doLoad);
    current = current->_nextItem;
  }
}
void ParameterGroup::renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper)
{
    if (this->label != nullptr)
    {
      String content = getStartTemplate();
      content.replace("{b}", this->label);
      content.replace("{i}", this->getId());
      webRequestWrapper->sendContent(content);
    }
    ConfigItem* current = this->_firstItem;
    while (current != nullptr)
    {
      if (current->visible)
      {
        current->renderHtml(dataArrived, webRequestWrapper);
      }
      current = current->_nextItem;
    }
    if (this->label != nullptr)
    {
      String content = getEndTemplate();
      content.replace("{b}", this->label);
      content.replace("{i}", this->getId());
      webRequestWrapper->sendContent(content);
    }
}
#if IOT_OLD
#else
// new version for AsyncWebServer - not necessary, as not used.
void ParameterGroup::renderHtml(bool dataArrived, String& content)
{
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.println("Entering renderHtml. ");
#endif     

    if (this->label != nullptr)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Label: ");
      Serial.println(this->label);
#endif  
      String _content = getStartTemplate();
      _content.replace("{b}", this->label);
      _content.replace("{i}", this->getId());
      content +=_content;
    }
    ConfigItem* current = this->_firstItem;
    while (current != nullptr)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Rendering: ");
      Serial.println(current->_id);
#endif 
      if (current->visible)
      {
        current->renderHtml(dataArrived, content);
      }
      current = current->_nextItem;
    }
    if (this->label != nullptr)
    {
      String _content = getEndTemplate();
      _content.replace("{b}", this->label);
      _content.replace("{i}", this->getId());
      content +=_content;
    }
}
// new version 2 for AsyncWebServer: this is used (polymorphic)
void ParameterGroup::renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper, String& content)
{
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.println("Entering renderHtml-2. ");
#endif     

    if (this->label != nullptr)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Label: ");
      Serial.println(this->label);
#endif  

      String _content = getStartTemplate();
      _content.replace("{b}", this->label);
      _content.replace("{i}", this->getId());
      content +=_content;
    }
    ConfigItem* current = this->_firstItem;
    while (current != nullptr)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print("Rendering: ");
      Serial.println(current->_id);
#endif 
      if (current->visible)
      {
        current->renderHtml(dataArrived, webRequestWrapper, content);
      }
      current = current->_nextItem;
    }
    if (this->label != nullptr)
    {
      String _content = getEndTemplate();
      _content.replace("{b}", this->label);
      _content.replace("{i}", this->getId());
      content +=_content;
    }
}
#endif // IOT_OLD
void ParameterGroup::update(WebRequestWrapper* webRequestWrapper)
{
  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    current->update(webRequestWrapper);
    current = current->_nextItem;
  }
}
void ParameterGroup::clearErrorMessage()
{
  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    current->clearErrorMessage();
    current = current->_nextItem;
  }
}
void ParameterGroup::debugTo(Stream* out)
{
  out->print('[');
  out->print(this->getId());
  out->println(']');

  // -- Here is some overcomplicated logic to have nice debug output.
  bool ownItem = false;
  bool lastItem = false;
  PrefixStreamWrapper stream =
    PrefixStreamWrapper(
      out,
      [&](Stream* out1)
    {
      if (ownItem)
      {
        ownItem = false;
        return (size_t)0;
      }
      if (lastItem)
      {
        return out1->print("    ");
      }
      else
      {
        return out1->print("|   ");
      }
    });

  ConfigItem* current = this->_firstItem;
  while (current != nullptr)
  {
    if (current->_nextItem == nullptr)
    {
      out->print("\\-- ");
    }
    else
    {
      out->print("|-- ");
    }
    ownItem = true;
    lastItem = (current->_nextItem == nullptr);
    current->debugTo(&stream);
    current = current->_nextItem;
  }
}

#ifdef IOTWEBCONF_ENABLE_JSON
void ParameterGroup::loadFromJson(JsonObject jsonObject)
{
  if (jsonObject.containsKey(this->getId()))
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(F("Applying values from JSON for groupId: "));
  Serial.println(this->getId());
#endif
    JsonObject myObject = jsonObject[this->getId()];
    ConfigItem* current = this->_firstItem;
    while (current != nullptr)
    {
      current->loadFromJson(myObject);
      current = current->_nextItem;
    }
  }
  else
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print(F("Group data not found in JSON. Skipping groupId: "));
    Serial.println(this->getId());
#endif
  }
}
#endif

///////////////////////////////////////////////////////////////////////////////

Parameter::Parameter(
  const char* label, const char* id, char* valueBuffer, int length,
  const char* defaultValue) :
  ConfigItem(id)
{
  this->label = label;
  this->valueBuffer = valueBuffer;
  this->_length = length;
  this->defaultValue = defaultValue;

  this->errorMessage = nullptr;
}
int Parameter::getStorageSize()
{
  return this->_length;
}
void Parameter::applyDefaultValue()
{
  if (defaultValue != nullptr)
  {
    strncpy(this->valueBuffer, this->defaultValue, this->getLength());
  }
  else
  {
    this->valueBuffer[0] = '\0';
  }
}
void Parameter::storeValue(
  std::function<void(SerializationData* serializationData)> doStore)
{
  SerializationData serializationData;
  serializationData.length = this->_length;
  serializationData.data = (byte*)this->valueBuffer;
  doStore(&serializationData);
}
void Parameter::loadValue(
  std::function<void(SerializationData* serializationData)> doLoad)
{
  SerializationData serializationData;
  serializationData.length = this->_length;
  serializationData.data = (byte*)this->valueBuffer;
  doLoad(&serializationData);
}
void Parameter::update(WebRequestWrapper* webRequestWrapper)
{
  if (webRequestWrapper->hasArg(this->getId()))
  {
    String newValue = webRequestWrapper->arg(this->getId());
    this->update(newValue);
  }
}
void Parameter::clearErrorMessage()
{
    this->errorMessage = nullptr;
}
#ifdef IOTWEBCONF_ENABLE_JSON
void Parameter::loadFromJson(JsonObject jsonObject)
{
  if (jsonObject.containsKey(this->getId()))
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(F("Applying value from JSON for parameterId: "));
  Serial.println(this->getId());
#endif
    const char* value = jsonObject[this->getId()];
    this->update(String(value));
  }
  else
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(F("No value found in JSON for parameterId: "));
  Serial.println(this->getId());
#endif
  }
}
#endif


///////////////////////////////////////////////////////////////////////////////

TextParameter::TextParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  const char* defaultValue,
  const char* placeholder,
  const char* customHtml)
  : Parameter(label, id, valueBuffer, length, defaultValue)
{
  this->placeholder = placeholder;
  this->customHtml = customHtml;
}

void TextParameter::renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper)
{
  String content = this->renderHtml(
    dataArrived,
    webRequestWrapper->hasArg(this->getId()),
    webRequestWrapper->arg(this->getId()));
  webRequestWrapper->sendContent(content);
}
#if IOT_OLD
#else
// new for AsyncWebServer - that is the important one. The content is delivered by the called method.
void TextParameter::renderHtml(bool dataArrived, WebRequestWrapper* webRequestWrapper, String& content)
{
   content += this->renderHtml(
    dataArrived,
    webRequestWrapper->hasArg(this->getId()),       // check if a post message with this Id has beeen received
    webRequestWrapper->arg(this->getId()));         // deliver the posted value
}
// not used
void TextParameter::renderHtml(bool dataArrived, String& content)
{

}
#endif // IOT_OLD
String TextParameter::renderHtml(bool dataArrived, bool hasValueFromPost, String valueFromPost)
{
  return this->renderHtml("text", hasValueFromPost, valueFromPost);
}
// this method is called by the other parameter type methods. It does the work of composing the content string.
String TextParameter::renderHtml(const char* type, bool hasValueFromPost, String valueFromPost)
{
  TextParameter* current = this;
  char parLength[12];

  String pitem = getHtmlTemplate();

  pitem.replace("{b}", current->label);
  pitem.replace("{t}", type);
  pitem.replace("{i}", current->getId());
  pitem.replace("{p}", current->placeholder == nullptr ? "" : current->placeholder);
  snprintf(parLength, 12, "%d", current->getLength()-1);
  pitem.replace("{l}", parLength);
  if (hasValueFromPost)
  {
    // -- Value from previous submit
    pitem.replace("{v}", valueFromPost);
  }
  else
  {
    // -- Value from config
    pitem.replace("{v}", current->valueBuffer);
  }
  pitem.replace(
      "{c}", current->customHtml == nullptr ? "" : current->customHtml);
  pitem.replace(
      "{s}",
      current->errorMessage == nullptr ? "" : "de"); // Div style class.
  pitem.replace(
      "{e}",
      current->errorMessage == nullptr ? "" : current->errorMessage);

  return pitem;
}

void TextParameter::update(String newValue)
{
  newValue.toCharArray(this->valueBuffer, this->getLength());
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(this->getId());
  Serial.print(": ");
  Serial.println(this->valueBuffer);
#endif
}

void TextParameter::debugTo(Stream* out)
{
  Parameter* current = this;
  out->print("'");
  out->print(current->getId());
  out->print("' with value: '");
  out->print(current->valueBuffer);
  out->println("'");
}

///////////////////////////////////////////////////////////////////////////////

NumberParameter::NumberParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  const char* defaultValue,
  const char* placeholder,
  const char* customHtml)
  : TextParameter(label, id, valueBuffer, length, defaultValue,
  placeholder, customHtml)
{
}

String NumberParameter::renderHtml(bool dataArrived, bool hasValueFromPost, String valueFromPost)
{
  return TextParameter::renderHtml("number", hasValueFromPost, valueFromPost);
}

///////////////////////////////////////////////////////////////////////////////

PasswordParameter::PasswordParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  const char* defaultValue,
  const char* placeholder,
  const char* customHtml)
  : TextParameter(label, id, valueBuffer, length, defaultValue,
  placeholder, customHtml)
{
}

String PasswordParameter::renderHtml(bool dataArrived, bool hasValueFromPost, String valueFromPost)
{
#if IOT_OLD
  return TextParameter::renderHtml("password", true, String(" "));
#else
  return TextParameter::renderHtml("password", true, String(""));    // mh: empty string to avoid fill of value field in browser
#endif
}

void PasswordParameter::debugTo(Stream* out)
{
  Parameter* current = this;
  out->print("'");
  out->print(current->getId());
  out->print("' with value: ");
#ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
  out->print("'");
  out->print(current->valueBuffer);
  out->println("'");
#else
  out->println(F("<hidden>"));
#endif
}

void PasswordParameter::update(String newValue)
{
  Parameter* current = this;
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
  Serial.print(this->getId());
  Serial.print(": ");
#endif
  if (newValue.length() > 0)
  {
    // -- Value was set.
    newValue.toCharArray(current->valueBuffer, current->getLength());
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
# ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
    Serial.println(current->valueBuffer);
# else
    Serial.println("<updated>");
# endif
#endif
  }
  else
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.println("<was not changed>");
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////

CheckboxParameter::CheckboxParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    bool defaultValue)
  : TextParameter(label, id, valueBuffer, length, defaultValue ? "selected" : nullptr,
  nullptr, nullptr)
{
}

String CheckboxParameter::renderHtml(bool dataArrived, bool hasValueFromPost, String valueFromPost)
{
  bool checkSelected = false;
  if (dataArrived)
  {
    if (hasValueFromPost && valueFromPost.equals("selected"))
    {
      checkSelected = true;
    }
  }
  else
  {
    if (this->isChecked())
    {
      checkSelected = true;
    }
  }

  if (checkSelected)
  {
    this->customHtml = CheckboxParameter::_checkedStr;
  }
  else
  {
    this->customHtml = nullptr;
  }
  
  
  return TextParameter::renderHtml("checkbox", true, "selected");
}

void CheckboxParameter::update(WebRequestWrapper* webRequestWrapper)
{
  if (webRequestWrapper->hasArg(this->getId()))
  {
    String newValue = webRequestWrapper->arg(this->getId());
    return TextParameter::update(newValue);
  }
  else if (this->visible)
  {
    // HTML will not post back unchecked checkboxes.
    return TextParameter::update("");
  }
}

///////////////////////////////////////////////////////////////////////////////

OptionsParameter::OptionsParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t nameLength,
    const char* defaultValue)
  : TextParameter(label, id, valueBuffer, length, defaultValue,
  nullptr, nullptr)
{
  this->_optionValues = optionValues;
  this->_optionNames = optionNames;
  this->_optionCount = optionCount;
  this->_nameLength = nameLength;
}

///////////////////////////////////////////////////////////////////////////////

SelectParameter::SelectParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t nameLength,
    const char* defaultValue)
  : OptionsParameter(label, id, valueBuffer, length, optionValues, optionNames,
  optionCount, nameLength, defaultValue)
{
}

String SelectParameter::renderHtml(bool dataArrived, bool hasValueFromPost, String valueFromPost)
{
  TextParameter* current = this;

  String options = "";

  for (size_t i=0; i<this->_optionCount; i++)
  {
    const char *optionValue = (this->_optionValues + (i*this->getLength()) );
    const char *optionName = (this->_optionNames + (i*this->_nameLength) );
    String oitem = FPSTR(IOTWEBCONF_HTML_FORM_OPTION);
    oitem.replace("{v}", optionValue);
//    if (sizeof(this->_optionNames) > i)
    {
      oitem.replace("{n}", optionName);
    }
//    else
//    {
//      oitem.replace("{n}", "?");
//    }
    if ((hasValueFromPost && (valueFromPost == optionValue)) ||
      (strncmp(current->valueBuffer, optionValue, this->getLength()) == 0))
    {
      // -- Value from previous submit
      oitem.replace("{s}", " selected");
    }
    else
    {
      // -- Value from config
      oitem.replace("{s}", "");
    }

    options += oitem;
  }

  String pitem = FPSTR(IOTWEBCONF_HTML_FORM_SELECT_PARAM);

  pitem.replace("{b}", current->label);
  pitem.replace("{i}", current->getId());
  pitem.replace(
      "{c}", current->customHtml == nullptr ? "" : current->customHtml);
  pitem.replace(
      "{s}",
      current->errorMessage == nullptr ? "" : "de"); // Div style class.
  pitem.replace(
      "{e}",
      current->errorMessage == nullptr ? "" : current->errorMessage);
  pitem.replace("{o}", options);

  return pitem;
}

///////////////////////////////////////////////////////////////////////////////

PrefixStreamWrapper::PrefixStreamWrapper(
  Stream* originalStream,
  std::function<size_t(Stream* stream)> prefixWriter)
{
  this->_originalStream = originalStream;
  this->_prefixWriter = prefixWriter;
}
size_t PrefixStreamWrapper::write(uint8_t data)
{
  size_t sizeOut = checkNewLine();
  sizeOut += this->_originalStream->write(data);
  if (data == 10) // NewLine
  {
    this->_newLine = true;
  }
  return sizeOut;
}
size_t PrefixStreamWrapper::write(const uint8_t *buffer, size_t size)
{
  size_t sizeOut = checkNewLine();
  sizeOut += this->_originalStream->write(buffer, size);
  if (*(buffer + size-1) == 10) // Ends with new line
  {
    this->_newLine = true;
  }
  return sizeOut;
}
size_t PrefixStreamWrapper::checkNewLine()
{
  if (this->_newLine)
  {
    this->_newLine = false;
    return this->_prefixWriter(this->_originalStream);
  }
  return 0;
}

#if IOT_OLD
} // end namespace
#endif