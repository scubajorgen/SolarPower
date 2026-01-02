#include <stdio.h>
#include <string.h> 

#include "common.h"

#include "Toolbox.h"
#include "Configuration.h"

/******************************************************************************\
* Variables
\******************************************************************************/

char        Toolbox::logFileName[MAX_FILENAME_STRING];
char        Toolbox::messageString[MAX_MESSAGE_STRING];
char        Toolbox::timeString[MAX_TIME_STRING];

Clock*      Toolbox::clock;

bool        Toolbox::debugPrinting=true;
bool        Toolbox::infoPrinting=true;
bool        Toolbox::errorPrinting=true;
bool        Toolbox::filePrinting=false;


FILE*       Toolbox::fp=NULL;
   



/******************************************************************************\
*
* This method outputs the message to stdio or file
*
\******************************************************************************/

void Toolbox::outputString(const char* outString)
{
    if (filePrinting)
    {
        if (fp==NULL)
        {
           fp = fopen(logFileName, "w");
        }
        if (fp!=NULL)
        {
            fprintf(fp, outString);
            fflush(fp);
        }
    }
    else
    {
        printf(outString);
    }
}

      


/******************************************************************************\
*
* This method redirects logging to file (true) or screen (false)
*
\******************************************************************************/

void Toolbox::printToFile(bool enable, char* fileName)
{
    if (fileName!=NULL)
    {
        strncpy(logFileName, fileName, MAX_FILENAME_STRING-1);
    }
    
    filePrinting=enable;
}


/******************************************************************************\
*
* This method enables/disables debug level logging
*
\******************************************************************************/

void Toolbox::setDebugPrinting     (bool enable)
{
    debugPrinting=enable;
}

/******************************************************************************\
*
* This method enables/disables info level logging
*
\******************************************************************************/

void Toolbox::setInfoPrinting     (bool enable)
{
    infoPrinting=enable;
}

/******************************************************************************\
*
* This method enables/disables error level logging (should always be enabled...)
*
\******************************************************************************/

void Toolbox::setErrorPrinting    (bool enable)
{
    errorPrinting=enable;
}




/******************************************************************************\
*
* This method prints an error message
*
\******************************************************************************/
void Toolbox::printError(const char* message)
{

    clock=Clock::getInstance();

    clock->getTimeString(timeString);

    if (errorPrinting)
    {
        snprintf(messageString, MAX_MESSAGE_STRING, "%s ERROR: %s", timeString, message);
        outputString(messageString);
    }
}


/******************************************************************************\
*
* This method prints info
*
\******************************************************************************/
void Toolbox::printInfo(const char* message)
{

    clock=Clock::getInstance();

    clock->getTimeString(timeString);

    if (infoPrinting)
    {
        snprintf(messageString, MAX_MESSAGE_STRING, "%s INFO: %s", timeString, message);
        outputString(messageString);
    }
}

/******************************************************************************\
*
* This method prints debug info
*
\******************************************************************************/
void Toolbox::printDebug(const char* message)
{

    clock=Clock::getInstance();

    clock->getTimeString(timeString);

    if (debugPrinting)
    {
        snprintf(messageString, MAX_MESSAGE_STRING, "%s DEBUG: %s", timeString, message);
        outputString(messageString);
    }
}

/******************************************************************************\
*
* Close the log (in case of file logging: close the file)
*
\******************************************************************************/

void Toolbox::closeLog()
{
    if (fp!=NULL)
    {
        fclose(fp);
        fp=NULL;
    }
}


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
