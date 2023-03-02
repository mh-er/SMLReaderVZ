#ifndef SML_DEBUG_H
#define SML_DEBUG_H

#include <Arduino.h>
#include <sml/sml_file.h>
#include <sml/sml_value.h>

// from FormattingSerialDebug
#if (SERIAL_DEBUG)
    #define DEBUG(format, ...) printf(format, ##__VA_ARGS__); fflush(stdout); SERIAL_DEBUG_IMPL.println()
    #ifndef SERIAL_DEBUG_IMPL
        #define SERIAL_DEBUG_IMPL Serial
    #endif
#else
    #define DEBUG(format, ...)
#endif


//#ifdef DEBUG
//#define SERIAL_DEBUG true
//#define SERIAL_DEBUG_VERBOSE true
//#else
//#define SERIAL_DEBUG false
//#endif

void DEBUG_DUMP_BUFFER(byte *buf, int size);
void DEBUG_SML_FILE(sml_file *file);

#endif