/**************************************************************************************************\
*
* Toolbox.h
*
* Some come in handy functions
*
\**************************************************************************************************/
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

public:
    
    static  int         convertPulsesToPower(int* pulses, int pulsesPerKiloWattHour);
};

#endif