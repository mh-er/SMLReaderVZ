#ifndef MAIN_H
#define MAIN_H

#ifndef DEBUG_TRACE
    #define DEBUG_TRACE(trace, format, ...) if(trace) {printf(format, ##__VA_ARGS__); fflush(stdout); Serial.println();}
#endif
#ifndef DEBUG_TRACE_
    #define DEBUG_TRACE_(trace, format, ...) if(trace) {printf(format, ##__VA_ARGS__); fflush(stdout);}
#endif


#if(SERIAL_DEBUG)
    #define MY_VERSION_TYPE "Debug"
#else
    #define MY_VERSION_TYPE "Release"
#endif
enum WebConfMode {UNKNOW, WEB_CONF_MODE_AP, WEB_CONF_MODE_STA, WEB_CONF_MODE_AP_STA};

#ifdef IPv6  // IPv6 requires adaptation of ESPAsyncTCP
#include <AddrList.h>
#include <lwip/dns.h>
#endif

#endif  // MAIN_H