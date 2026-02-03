#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"

#include "Toolbox.h"
#include "Configuration.h"

/******************************************************************************\
* Variables
\******************************************************************************/
char    Toolbox::appendBuffer[APPENDBUFFERSIZE];
char    Toolbox::regexErrorMessage[MAXREGEXERRORMSG];
char    Toolbox::matchResult[MAXMATCHSIZE];
Log     Toolbox::logger {"Toolbox"};

/******************************************************************************\
*
* Convert pulses to average power, power in 0.1 W (deciWatt)
*
\******************************************************************************/
int Toolbox::convertPulsesToPower  (int* pulses, int pulsesPerKiloWattHour)
{
    long power;
    
    if (*pulses>=0)
    {
        power=DECIWATT_PER_WATT * *pulses * INTERVALS_PER_HOUR*WATTHOUR_PER_KILOWATTHOUR/pulsesPerKiloWattHour;
    }
    else
    {
        *pulses=INVALID_MEASUREMENT;
        power=(float)INVALID_MEASUREMENT;
    }   
    return (int)power; 
}

/**************************************************************************************************\
*
* String append
*
\**************************************************************************************************/
void Toolbox::stringAppend(char* string, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vsprintf(appendBuffer, message, args);
    char* pointer=string+strlen(string);
    sprintf(pointer, "%s", appendBuffer);
}

/**************************************************************************************************\
*
* String append
*
\**************************************************************************************************/
void Toolbox::stringReset(char* string)
{
    string[0]='\0';
}

/******************************************************************************\
*
* Compute CRC16 (used by DSMR) CRC16-IBM/CRC-16-ARC, inverted 0xA001 ipv 0x8005
* All characters from / to ! (inclusive), start value 0, 
* Polyniomal(0xA001) ((x^{16}+x^{15}+x^{2}+1))
*
\******************************************************************************/
uint16_t Toolbox::crc16(const uint8_t* data, size_t length)
{
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < length; i++) 
    {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) 
         {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/******************************************************************************\
*
* Execute regex match and process result
*
\******************************************************************************/
bool Toolbox::processMatchFloat(char* message, regex_t* regex, INT32* var, float factor)
{
    bool success=false;
    matchRegex(regex, message);
    if (strcmp("", matchResult)!=0)
    {
        *var=(int)(factor*atof(matchResult)+0.5);
        success=true;
    }
    return success;
}

/******************************************************************************\
*
* Execute regex match and process result
*
\******************************************************************************/
bool Toolbox::processMatchInt(char* message, regex_t* regex, INT32* var)
{
    bool success=false;
    char* result=matchRegex(regex, message);
    if (strcmp("", result)!=0)
    {
        *var=atoi(result);
        success=true;
    }
    return success;
}

/******************************************************************************\
*
* Execute regex match and process result
*
\******************************************************************************/
bool Toolbox::processMatchString(char* message, regex_t* regex, char* var, int length)
{
    bool success=false;
    char* result=matchRegex(regex, message);
    if (strcmp("", result)!=0)
    {
        strncpy(var, result, length);
        success=true;
    }
    return success;
}

/******************************************************************************\
*
*
*
\******************************************************************************/
void Toolbox::compileRegex (regex_t* regex, const char* regexText)
{
    int status = regcomp (regex, regexText, REG_EXTENDED|REG_NEWLINE);
    if (status != 0)
    {
        regerror (status, regex, regexErrorMessage, MAXREGEXERRORMSG);
        logger.logError("Regex error compiling '%s': %s",regexText, regexErrorMessage);
    }
}

/******************************************************************************\
*
* Returns the match result or empty string if not found
* This function tries find at most MAXMATCHNUMBER matches and returns the
* latest match found. It tries to find at most MAXGROUPNUMBER-1 groups and 
* returns the latest found group
*
\******************************************************************************/
char* Toolbox::matchRegex (regex_t* regex, const char* toMatch)
{
    /* "P" is a pointer into the string which points to the end of the
       previous match. */
    const char* nextMatch               = toMatch;
    // "maxNumberOfMatches" is the maximum number of matches allowed.
    // "groups" contains the matches found:
    //   groups[0] entire match 
    //   groups[1, 2, ...] the individual groups
    regmatch_t groups[MAXGROUPNUMBER+1];

    bool error=false;
    // try to match MAXMATCHNUMBER of times
    for (int m = 0; m < MAXMATCHNUMBER && !error; m++)
    {
        // We try to find MAXGROUPNUMBER+1 substrings
        // First is the match, subsequent are the group values
        int nomatch = regexec (regex, nextMatch, MAXGROUPNUMBER+1, groups, 0);
        if (nomatch)
        {
            matchResult[0]              ='\0';
            error                       =true;
            logger.logError("Parsing P1: No match found. %s", toMatch);
        }
        else
        {
            for (int gr = 1; gr < MAXGROUPNUMBER+1 && !error; gr++)
            {
                if (groups[gr].rm_so >=0)
                {
                    int start   = groups[gr].rm_so;  // Offset with respect to nextMach
                    int finish  = groups[gr].rm_eo;  // End with respect to nextMatch
                    sprintf(matchResult, "%.*s", (finish-start), nextMatch+start);
                }
                else
                {
                    error               =true;
                    logger.logError("Parsing P1: No group in match.\n");
                }
            }
            nextMatch += groups[0].rm_eo;
        }
    }
    return matchResult;
}
