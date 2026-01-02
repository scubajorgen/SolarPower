/**************************************************************************************************\
*
* Toolbox.cpp
*
* Some come in handy functions
*
\**************************************************************************************************/
#include <stdio.h>
#include <string.h> 

#include "common.h"

#include "Toolbox.h"
#include "Configuration.h"

/******************************************************************************\
* Variables
\******************************************************************************/


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
