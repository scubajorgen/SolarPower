/**************************************************************************************************\
*
* PulseCounter.cpp
*
* Pulse meter readout and processing
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "PulseCounter.h"

/******************************************************************************\
* Variables
\******************************************************************************/

/******************************************************************************\
* Friend methods
\******************************************************************************/

/******************************************************************************\
* Private methods
\******************************************************************************/

/******************************************************************************\
*
* The method implements the statemachine for counting pulses. It should
* be called on a regular basis (every 10 ms=0.01 s).
*
* IDLE -> LOWSTATE -> WAITFORHIGH -> HIGHSTATE -> WAITFORLOW -
*           ^  ^        |                  ^          |       |
*           |  |________|                  |__________|       |
*           |_________________________________________________|
* 
* A signal must be high/low for at least 2 periods (0.02s) before it is
* accepted. One period is not sufficient.
*
\******************************************************************************/
void PulseCounter::process()
{
    if (meterUsage!=USAGE_NOTUSED)  // If not used, don't waste energy
    {
        pulseValue=ioPins->getPulse(pulseId);

        switch (countState)
        {
        case COUNTSTATE_IDLE:
            if (pulseValue==false)
            {
                countState=COUNTSTATE_LOWSTATE;
            }
            break;

        case COUNTSTATE_LOWSTATE:
            if (pulseValue==true)
            {
                countState=COUNTSTATE_WAITFORHIGH;
            }
            break;

        case COUNTSTATE_WAITFORHIGH:
            if (pulseValue==true)
            {
                // YES! WE'VE GOT ONE!!!
                pulseReceived();
                countState=COUNTSTATE_HIGHSTATE;
            }
            else
            {
                countState=COUNTSTATE_LOWSTATE;
                ghostPulseCount++;
                logger.logWarning("To short high (pulse)");
            }
            break;

        case COUNTSTATE_HIGHSTATE:
            if (pulseValue==false)
            {
                countState=COUNTSTATE_WAITFORLOW;
            }
            break;

        case COUNTSTATE_WAITFORLOW:
            if (pulseValue==false)
            {
                countState=COUNTSTATE_LOWSTATE;
            }
            else
            {
                countState=COUNTSTATE_HIGHSTATE;
                ghostDipCount++;
                logger.logWarning("To short low (^pulse)");
            }
            break;
        }
    }
}


/******************************************************************************\
*
* The method calculates the power in Watt based on this and previous pulse
*
\******************************************************************************/
INT32 PulseCounter::calculatePower()
{
    INT32 thePower;

    thePower=-1;

    timeDiff  = pulseTime.hour  * 3600L * 100L;
    timeDiff += pulseTime.minute* 60L * 100L;
    timeDiff += pulseTime.second* 100L;
    timeDiff += pulseTime.centisecond;
    timeDiff -= previousPulseTime.hour  * 3600L * 100L;
    timeDiff -= previousPulseTime.minute* 60L * 100L;
    timeDiff -= previousPulseTime.second* 100L;
    timeDiff -= previousPulseTime.centisecond;

    // day boundary crossed...
    if (timeDiff<0)
    {
        timeDiff+=24L * 3600L * 100L;
    }

    // DIRTY, VERY DIRTY HACK
    // the time between pulses should be larger than 10 cs (100 ms: equivalent of 18 kW)
    // to prevent 'ghost pulses' to be counted
    // Without ghost pulses, this value should be 0 cs
    if (timeDiff>10)
    {
        // calculate the instantaneous power value
        thePower=(3600L*100L/timeDiff)*WATTHOUR_PER_KILOWATTHOUR/configuration->getPulsesPerKwh(pulseId);

    }
    return thePower;

}

/******************************************************************************\
*
* The method calculates at the end of the interval the average interval power
* in dWatt
*
\******************************************************************************/
INT32 PulseCounter::calculateAverageIntervalPower()
{
    // Retrieve pulses per kWh, it could have been changed...
    int pulsesPerKwh=configuration->getPulsesPerKwh(pulseId);
    INT32 thePower=(60/MEASUREMENT_INTERVAL)*WATTHOUR_PER_KILOWATTHOUR*DECIWATT_PER_WATT*currentPulseCounter/pulsesPerKwh;
    return thePower;
}

/******************************************************************************\
*
* The method handles the reception of a valid pulse.
*
\******************************************************************************/
void PulseCounter::pulseReceived()
{
    // Always toggle the led
    ioPins->setPulseLed(pulseId, ledState);            // toggle the led
    ledState=!ledState;

    // Get the current time
    solarClock->getTime(&pulseTime);


    if (firstPulseReceived)
    {
        power=calculatePower();

        if (power>=0)
        {
            // Count the pulse
            currentPulseCounter++;

            // check if we've got a new instantaneous power max value
            if (timeDiff<maxPowerTimeDiff)
            {
                maxPower            =power;
                maxPowerTimeDiff    =timeDiff;
                maxPowerTime        =pulseTime;
            }

            if (power>maxIntervalPower)
            {
                maxIntervalPower    =power;
            }
            energyMeterCounts++;
            processEnergyMeter();
            logger.logInfo("Counter %d - Pulse: dT=%d cs P=%d W max: dT=%d cs P=%d W interval max: %d W",
                                 pulseId, timeDiff, power, maxPowerTimeDiff, maxPower, maxIntervalPower);
        }
        else
        {
            if (timeDiff==0)
            {
                logger.logError("Division by zero");
            }
            else
            {
                logger.logError("GHOST PULSE RECEIVED!!!");
            }
        }

    }
    else
    {
        logger.logInfo("Counter %d: First pulse received!", pulseId);
        firstPulseReceived=true;
    }
    previousPulseTime=pulseTime;
    pulseCount++;
}


/******************************************************************************\
*
* The method estimates the current power.
* This is the last measured power or the power that would result
* if at this very moment a pulse would have been received, whichever is lower.
* The trouble is, if the power suddely drops to a low value (say: zero), pulses
* cease to occur. When publishing, the last measured power is going to be
* repeated, which does not make sense. Therefore, we calculate the power
* if now a pulse is received. If this value is lower, we publish this value.
* In this way the published value exponentially goes to the low value.
*
\******************************************************************************/
void PulseCounter::estimateCurrentPower()
{
    // If some publishing instance defined, publish the pulse (only for 1st counter)
    if (power>=0)
    {
        // Get the current time
        solarClock->getTime(&pulseTime);

        // The power if right here, right now a pulse would have been received.
        INT32 whatIfPower=calculatePower();

        // Compare to last measured real power
        if ((whatIfPower>=0) && (whatIfPower<power))
        {
            publishPower=whatIfPower;
        }
        else
        {
            publishPower=power;
        }
    }
    else
    {
        publishPower=-1;
    }
}

/******************************************************************************\
*
* Start the measurements; Resets the pulse counter
*
\******************************************************************************/
void PulseCounter::startMeasurement()
{
    currentPulseCounter =0;
    maxIntervalPower    =0;
}

/******************************************************************************\
*
* Collect the measrured values and restart the measurement
*
\******************************************************************************/
void PulseCounter::retrieveAndRestartMeasurement(Measurement_t *measurement)
{
    measurement->pulse[pulseId]         =currentPulseCounter;
    measurement->pulsePower[pulseId]    =calculateAverageIntervalPower();
    measurement->pulseMaxPower[pulseId] =maxIntervalPower;
    measurement->pulseMeter[pulseId]    =energyMeter;
    startMeasurement();
}

/******************************************************************************\
*
* Returns current pulse counter value
*
\******************************************************************************/
int PulseCounter::getCounterValue()
{
    return currentPulseCounter;
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/
PulseCounter::PulseCounter(int pulseId, PulseMeterUsage_t meterUsage, char* meterFile)
{
    configuration               = Configuration::getInstance();

    // Id of this pulse
    this->pulseId               = pulseId;
    this->meterUsage            = meterUsage;
    this->energyMeterFileName   = meterFile;

    // Signal no valid power value measured
    power                       = 0;
    publishPower                = 0;

    // Get the clock instance
    solarClock                  =Clock::getInstance();

    // Get the current time
    solarClock->getTime(&previousPulseTime);

    pulseCount                  =0L;
    ghostPulseCount             =0L;
    ghostDipCount               =0L;


    // initialise the counting state machine
    ioPins                      =IoPins::getInstance();
    ledState                    =false;
    countState                  =COUNTSTATE_IDLE;


    // first pulse received flag
    firstPulseReceived          =false;

    // Reset the daily power maximum
    resetCurrentPowerMax();

    // Initialise interval max
    maxIntervalPower            =0;

    initialiseEnergyMeter();
}

/******************************************************************************\
*
* Destructor
*
\******************************************************************************/
PulseCounter::~PulseCounter()
{
}

/******************************************************************************\
*
* Returns the measured power, in dWatt
* This is the currently measured or estimated power; -1 if not available
*
\******************************************************************************/

INT32 PulseCounter::getPublishPower()
{
    estimateCurrentPower();
    return publishPower;
}

/******************************************************************************\
*
* Returns the measured import power, in dWatt; is zero for production meter.
* This is the currently measured or estimated power, or -1 if not available
*
\******************************************************************************/
INT32 PulseCounter::getCurrentImportPower()
{
    INT32 power=0;
    switch (meterUsage)
    {
        case USAGE_PRODUCTION:
        case USAGE_NOTUSED:
            power=0;
            break;
        case USAGE_CONSUMPTION:
            estimateCurrentPower();
            power=publishPower;
            break;
    }
    return power;
}

/******************************************************************************\
*
* Returns the measured export power, in dWatt; is zero for consumption meters
* This is the currently measured or estimated power, or -1 if not available
*
\******************************************************************************/
INT32 PulseCounter::getCurrentExportPower()
{
    INT32 power=0;
    switch (meterUsage)
    {
        case USAGE_CONSUMPTION:
        case USAGE_NOTUSED:
            power=0;
            break;
        case USAGE_PRODUCTION:
            estimateCurrentPower();
            power=publishPower;
            break;
    }
    return power;
}

/******************************************************************************\
*
*  This method returns the values of the instantaneous max power
*  The instantaneous max power is the max power as defined by the time
*  between two adjacent pulses
*
\******************************************************************************/
void PulseCounter::getCurrentPowerMax(INT32 *timeDiff, INT32* power, solarTime_t *pulseTime)
{
    *timeDiff   =maxPowerTimeDiff;
    *pulseTime  =maxPowerTime;
    *power      =maxPower;
}

/******************************************************************************\
*
*  This method resets the max power values
*  The instantaneous max power is the max power as defined by the time
*  between two adjacent pulses
*  This function is typically called at programm startup or at start of day
*
\******************************************************************************/
void PulseCounter::resetCurrentPowerMax()
{
    solarClock->getTime(&now);

    // Make sure the date is set to today
    maxPowerTime.year       =now.year;
    maxPowerTime.month      =now.month;
    maxPowerTime.day        =now.day;
    maxPowerTime.hour       =0;
    maxPowerTime.minute     =0;
    maxPowerTime.second     =0;
    maxPowerTime.centisecond=0;

    maxPower                =-1;
    maxPowerTimeDiff        =10L*24L*3600L*100L;       // is 10 days
}

/******************************************************************************\
*
*  This function initialises the simulated Energy meter
*
\******************************************************************************/
void PulseCounter::initialiseEnergyMeter()
{
    FILE *fptr;
    // Open file in reading mode
    fptr = fopen(energyMeterFileName, "r");
    if (fptr!=NULL)
    {
        // Write some text to the file
        fscanf (fptr, "%d", &energyMeterBase);
        logger.logInfo("Read energy meter value %d Wh from file %s", energyMeterBase, energyMeterFileName);
        // Close the file
        fclose(fptr);
    }
    else
    {
        logger.logError("Unable to open pulse energy meter file %s", energyMeterFileName);
        energyMeterBase     =0;
    }
    energyMeter         =energyMeterBase;
    energyMeterCounts   =0;
}

/******************************************************************************\
*
*  This function stores the simulated energy meter value persistently
*
\******************************************************************************/
void PulseCounter::processEnergyMeter()
{
    int pulsesPerKwh    =configuration->getPulsesPerKwh(pulseId);
    energyMeter         =energyMeterBase+energyMeterCounts*WATTHOUR_PER_KILOWATTHOUR/pulsesPerKwh;
    switch (configuration->getPulseMeterPersistInterval())
    {
        case PERSISTINTERVAL_MINUTE:
            if (pulseTime.minute!=previousPulseTime.minute)
            {
                persistEnergyMeter();
            }
        break;
        case PERSISTINTERVAL_HOUR:
            if (pulseTime.hour!=previousPulseTime.hour)
            {
                persistEnergyMeter();
            }
            break;
        case PERSISTINTERVAL_DAY:
            if (pulseTime.day!=previousPulseTime.day)
            {
                persistEnergyMeter();
            }
            break;
        default:
            break;
    }
}

/******************************************************************************\
*
*  This function stores the simulated energy meter value persistently
*
\******************************************************************************/
void PulseCounter::persistEnergyMeter()
{
    if(energyMeter!=energyMeterBase)
    {
        logger.logInfo("Counter %d: Storing pulse energy meter value %d Wh to file %s", pulseId, energyMeter, energyMeterFileName);
        FILE *fptr;
        // Open file in writing mode
        fptr = fopen(energyMeterFileName, "w");
        if (fptr!=NULL)
        {
            // Write some text to the file
            fprintf(fptr, "%d", energyMeter);
            // Close the file
            fclose(fptr);
            energyMeterBase     =energyMeter;
            energyMeterCounts   =0;

        }
        else
        {
            logger.logError("Unable to open pulse energy meter file %s", energyMeterFileName);
        }
    }
}

/******************************************************************************\
*
* This function indicates if the pulse meter measures production (true)
* or consumption (false)
* DEPRECATED
*
\******************************************************************************/
bool PulseCounter::isProductionMeter()
{
    return (meterUsage==USAGE_PRODUCTION);
}

/******************************************************************************\
*
* This function indicates how the meter is used
*
\******************************************************************************/
PulseMeterUsage_t PulseCounter::getMeterUsage()
{
    return meterUsage;
}


/******************************************************************************\
*
* This function prints the status
*
\******************************************************************************/
void PulseCounter::logStatus()
{
    logger.logReport("Pulse meter %d: Pulses received %ld, ghost pulses %ld, ghost dips %ld", 
                    pulseId, pulseCount, ghostPulseCount, ghostDipCount);
}