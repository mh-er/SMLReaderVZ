# Changelog #
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Released] ##

## [2.2.1] - 2023-02-19 ##
### Fixed
- add missing update of date/time in loop


## [2.2.0] - 2023-02-17 ##
### Changed ###
- split VZ_SERVER and VZ_MIDDLEWARE names, use pure UUID strings
- config UI: separate config of server and middleware; add TIMEZONE offset to support switch to daylight saving time
- no transmission for data with UUID = VZ_UUID_NO_SEND
- remove use of timeControl and NTPclient (use built-in timer function of Arduino)
- adapt home page
- use original ESP-Dash

## [2.1.0] - 2023-01-29 ##
### Changed ###
- use timeControl 1.1.0 (built-in ntp timer, error corrections)
- add heartbeat channel
- rename obisValue to UuidValueName in smlHttp
- add Title card on Dash Board
- remove webConfMode in main()

### Fixed ###
-

## [2.0.0] - 2023-01-29 ##
### Changed ###
- added confWeb for the configuration interface in WiFi AP mode
- added struct MyHttpConfig for the configuration of VZ server and UUID
- added Home Page and handler
- added in class MyHttp: _uuid\[\], method setServerName(), testHttp(), initHttp()
- rename MyHttp.cpp/.h to smlHttp.cpp/.h and class to SmlHttp because strong dependance on smlLib.
- use lib timeControl instead of local time.cpp, update to new interface
- use own (modified) ESP-Dash @ 4.0.1
- clean up cascaded include files
- removed MicroDebug lib, defined Macros in respective include files.
- remove usage of toggleBuiltinLED

### Fixed ###
- name collision of hexdump in libSML/sml_shared.c/.h: renamed to sml_hexdump;  
allows to use newest Arduino framework
- Sensor.cpp: disables namespace std; added std:: to unique_ptr<SoftwareSerial>
  reason: byte was ambiguous
- rename local debug.c/.h to smlDebug.c/.h due to collision with Arduino.h
- smlDebug.h: removed #include <FormattingSerialDebug.h>, defined Macros there
  reason: problem with #pragma statement.


## [1.0.0] - 2022-12-08 ##
### Changed ###
- first version
### Fixed ###
-

