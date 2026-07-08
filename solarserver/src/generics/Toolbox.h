#ifndef TOOLBOX_H

#define TOOLBOX_H

#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <regex.h>

#include "common.h"
#include "Log.h"

#define MAXMATCHSIZE            128
#define MAXMATCHNUMBER          1       // We expect one match
#define MAXGROUPNUMBER          1       // We expect one group per match
#define MAXREGEXERRORMSG        256
#define APPENDBUFFERSIZE        2048

class Toolbox
{
private:
    static Log          logger;
    static char         appendBuffer[APPENDBUFFERSIZE];
    static char         regexErrorMessage[MAXREGEXERRORMSG];
    static char         matchResult[MAXMATCHSIZE];
public:
    static  int         convertPulsesToPower(int* pulses, int pulsesPerKiloWattHour);

    static void         stringReset         (char* string);
    static void         stringAppend        (char* string, const char* message, ...);

    static uint16_t     crc16               (const uint8_t* data, size_t length);

    static void         compileRegex        (regex_t* regex, const char * regexText);
    static char*        matchRegex          (regex_t* regex, const char* toMatch);
    static bool         processMatchFloat   (char* message, regex_t* regex, INT32* var, float factor);
    static bool         processMatchInt     (char* message, regex_t* regex, INT32* var);
    static bool         processMatchString  (char* message, regex_t* regex, char* var, int length);

    static bool         getObisValueString  (char* message, const char* obisCode, int response, char* var , int length);
    static bool         getObisValueFloat   (char* message, const char* obisCode, int response, INT32* var, float factor);
    static bool         getObisValueInt     (char* message, const char* obisCode, int response, INT32* var);
};

#endif