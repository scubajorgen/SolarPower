#ifndef TOOLBOX_H

#define TOOLBOX_H

#include <stdio.h>
#include <stdlib.h>

#include "Clock.h"

#define MAX_TIME_STRING     25
#define MAX_MESSAGE_STRING  256
#define MAX_FILENAME_STRING  256

class Toolbox
{
private:
    static bool         debugPrinting;
    static bool         infoPrinting;
    static bool         errorPrinting;
    static bool         filePrinting;
    static char         timeString[MAX_TIME_STRING];
    static char         messageString[MAX_MESSAGE_STRING];
    static char         logFileName[MAX_FILENAME_STRING];

    static Clock*       clock;
    
    static FILE*        fp;
    
    
    static void         outputString(const char* outString);
    

public:
    static void         printToFile         (bool enable, char* fileName);
    static void         setDebugPrinting    (bool enable);
    static void         setInfoPrinting     (bool enable);
    static void         setErrorPrinting    (bool enable);

    static void         printError          (const char* message);
    static void         printInfo           (const char* message);
    static void         printDebug          (const char* message);
    
    static void         closeLog            ();
    
    static  int         convertPulsesToPower(int* pulses, int pulsesPerKiloWattHour);
};

#endif