/**************************************************************************************************\
*
* MeasurementStorage.cpp
*
* Storage for measurements and daily maxima; thead safe
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "MeasurementStorage.h"

MeasurementStorage* MeasurementStorage::theInstance=NULL;

/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
MeasurementStorage::MeasurementStorage()
{
    int                     err;
    pthread_mutexattr_t     attr;
    
    configuration           =Configuration::getInstance();
    
    // initialise measurement array
    startOfArray            =0;
    startOfArrayNext        =0;
    endOfArray              =0;

    maxPowerStartOfArray    =0;
    maxPowerStartOfArrayNext=0;
    maxPowerEndOfArray      =0; 
    
    // create the mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    err=pthread_mutex_init(&mutex, &attr);
    if (err)
    {
        logger.logFatal("Unable to create mutex");
    }  
}

/******************************************************************************\
* Public methods
\******************************************************************************/
MeasurementStorage::~MeasurementStorage()
{
}

/******************************************************************************\
*
* Returns the one and only instance of this class
*
\******************************************************************************/
MeasurementStorage* MeasurementStorage::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new MeasurementStorage();
    }
    return theInstance;
}

/******************************************************************************\
*
*  This method returns the number of records that have been stored
*  Threadsafe
*
\******************************************************************************/
int MeasurementStorage::getNumberOfMeasurementRecords()
{
    pthread_mutex_lock(&mutex);  
    int number=endOfArray-startOfArray;
    if (number<0)
    {
        number+=MEASUREMENTSTORAGESIZE;
    }
    pthread_mutex_unlock(&mutex);  
    return number;
}

/******************************************************************************\
*
*  This method returns the number of power max records that have been stored
*
\******************************************************************************/
int MeasurementStorage::getNumberOfMaxPowerRecords()
{
    pthread_mutex_lock(&mutex); 
    int number=maxPowerEndOfArray-maxPowerStartOfArray;
    if (number<0)
    {
        number+=MAXPOWERSTORAGESIZE;
    }
    pthread_mutex_unlock(&mutex);  
    return number;
}

/******************************************************************************\
*
* Add indicated measurement to the measurement array
* Threadsafe
*
\******************************************************************************/
void MeasurementStorage::appendMeasurement(measurement_t* measurement)
{

    pthread_mutex_lock(&mutex);  
    
    // append the current pulsecounter to end of the measurement array
    measurements[endOfArray]=*measurement;

    // increase the start position pointer of the circular buffer
    endOfArray++;
    if (endOfArray>=MEASUREMENTSTORAGESIZE)
    {
        endOfArray=0;
    }

    // if the start equals the end position, overwrite the end pos
    if (endOfArray==startOfArray)
    {
        startOfArray++;
        if (startOfArray>=MEASUREMENTSTORAGESIZE)
        {
            startOfArray=0;
        }
    }
    if (endOfArray==startOfArrayNext)
    {
        startOfArrayNext++;
        if (startOfArrayNext>=MEASUREMENTSTORAGESIZE)
        {
            startOfArrayNext=0;
        }
    }
    pthread_mutex_unlock(&mutex);  

    logger.logInfo("Measurement - Time %d/%d (%02d-%02d-%04d %02d:%02d:%02d.%02d) P1: %s,",                         
                       measurement->year, measurement->timeIndex,
                       measurement->datetime.day, measurement->datetime.month, measurement->datetime.year,
                       measurement->datetime.hour, measurement->datetime.minute, measurement->datetime.second, measurement->datetime.centisecond,
                       measurement->p1Time);
    logger.logInfo("              P1 %d (%d/%d Watt, %d Wh), P2 %d (%d/%d Watt, %d Wh), P3 %d (%d/%d Watt, %d Wh), ",
                       measurement->pulse[0],measurement->pulsePower[0]/10, measurement->pulseMaxPower[0],measurement->pulseMeter[0],
                       measurement->pulse[1],measurement->pulsePower[1]/10, measurement->pulseMaxPower[1],measurement->pulseMeter[1],
                       measurement->pulse[2],measurement->pulsePower[2]/10, measurement->pulseMaxPower[2],measurement->pulseMeter[2]);
    logger.logInfo("              Eimp low %d Wh, Eimp normal %d Wh, Eexp low %d Wh, Eexp normal %d Wh",
                       measurement->electricityImportLow, measurement->electricityImportNormal,
                       measurement->electricityExportLow, measurement->electricityExportNormal);
    logger.logInfo("              Gross power %d W, net power %d W, Tariff %d",
                       measurement->grossPower/10, measurement->netPower/10, measurement->tariff);
    logger.logInfo("              Power failues all: %d, long %d, sags: L1 %d L2 %d L3 %d swells L1 %d L2 %d L3 %d",
                       measurement->powerFailures, measurement->powerFailuresLong, 
                       measurement->sagsL1, measurement->sagsL2, measurement->sagsL3,
                       measurement->swellsL1, measurement->swellsL2, measurement->swellsL3);
    logger.logInfo("              Voltage          L1 %6d L2 %6d L3 %6d mV Current L1 %6d L2 %6d L3 %6d A",
                        measurement->voltageL1, measurement->voltageL2, measurement->voltageL3,
                        measurement->currentL1, measurement->currentL2, measurement->currentL3);
    logger.logInfo("              Act Power Import L1 %6d L2 %6d L3 %6d W  Export  L1 %6d L2 %6d L3 %6d W",
                        measurement->activeImportPowerL1, measurement->activeImportPowerL2, measurement->activeImportPowerL3,
                        measurement->activeExportPowerL1, measurement->activeExportPowerL2, measurement->activeExportPowerL3);
    logger.logInfo("              Gas %d l (%s)", measurement->gasImport, measurement->gasTime);
}


/******************************************************************************\
*
*  Reset the next measurement pointer to the oldest measurement
*
\******************************************************************************/
bool MeasurementStorage::resetMeasurementNext()
{
    pthread_mutex_lock(&mutex);  
    startOfArrayNext=startOfArray;
    pthread_mutex_unlock(&mutex);  
    return false;
}

/******************************************************************************\
*
*  Purge the retrieved measurements. Simply by setting the start of the
*  array to the first non retrieved element
*
\******************************************************************************/
bool MeasurementStorage::purgeRetrievedMeasurements()
{
    pthread_mutex_lock(&mutex);  
    startOfArray    =startOfArrayNext;
    pthread_mutex_unlock(&mutex);  
    return false;
}

/******************************************************************************\
*
*  This method removes and returns oldest measurement in the buffer
*
\******************************************************************************/
bool MeasurementStorage::getNextMeasurement(measurement_t* measurement)
{
    bool error=false;
    pthread_mutex_lock(&mutex); 
    if (startOfArrayNext!=endOfArray)
    {
        *measurement=measurements[startOfArrayNext];
        startOfArrayNext++;
        if (startOfArrayNext>=MEASUREMENTSTORAGESIZE)
        {
            startOfArrayNext-=MEASUREMENTSTORAGESIZE;
        }
    }
    else
    {
        error=true;
    }
    pthread_mutex_unlock(&mutex); 
    return error;
}

/******************************************************************************\
*
* Add a measurement to the measurement array
*
\******************************************************************************/
void MeasurementStorage::appendMaxPower(maxPower_t* maxPower)
{
    pthread_mutex_lock(&mutex); 
    // append the current pulsecounter to end of the measurement array
    maxPowers[maxPowerEndOfArray]   =*maxPower;

    // increase the start position pointer of the circular buffer
    maxPowerEndOfArray++;
    if (maxPowerEndOfArray>=MAXPOWERSTORAGESIZE)
    {
        maxPowerEndOfArray=0;
    }

    // if the start equals the end position, overwrite the end pos
    if (maxPowerEndOfArray==maxPowerStartOfArray)
    {
        maxPowerStartOfArray++;
        if (maxPowerStartOfArray>=MAXPOWERSTORAGESIZE)
        {
            maxPowerStartOfArray=0;
        }
    }
    if (maxPowerEndOfArray==maxPowerStartOfArrayNext)
    {
        maxPowerStartOfArrayNext++;
        if (maxPowerStartOfArrayNext>=MAXPOWERSTORAGESIZE)
        {
            maxPowerStartOfArrayNext=0;
        }
    }
    pthread_mutex_unlock(&mutex); 
}


/******************************************************************************\
*
*  Reset the next measurement pointer to the oldest measurement
*
\******************************************************************************/
bool MeasurementStorage::resetPowerMaxNext()
{
    pthread_mutex_lock(&mutex);  
    maxPowerStartOfArrayNext    =maxPowerStartOfArray;
    pthread_mutex_unlock(&mutex);  
    return false;
}


/******************************************************************************\
*
*  This method removes and returns oldest max power from the buffer
*
\******************************************************************************/
bool MeasurementStorage::purgeRetrievedPowerMaxValues()
{
    pthread_mutex_lock(&mutex);  
    maxPowerStartOfArray        =maxPowerStartOfArrayNext;
    pthread_mutex_unlock(&mutex);  
    return false;
}


/******************************************************************************\
*
*  This method removes and returns oldest max power from the buffer
*
\******************************************************************************/
bool MeasurementStorage::getNextPowerMaxValue(maxPower_t* maxPower)
{
    bool error=false;
    pthread_mutex_lock(&mutex); 
    if (maxPowerStartOfArrayNext!=maxPowerEndOfArray)
    {
        *maxPower=maxPowers[maxPowerStartOfArrayNext];
        maxPowerStartOfArrayNext++;
        if (maxPowerStartOfArrayNext>=MAXPOWERSTORAGESIZE)
        {
            maxPowerStartOfArrayNext-=MAXPOWERSTORAGESIZE;
        }
    }
    else
    {
        error=true;
    }
    pthread_mutex_unlock(&mutex); 
    return error;
}

/******************************************************************************\
*
*  This method prints the status of the storage
*
\******************************************************************************/
void MeasurementStorage::logStatus()
{
    logger.logReport("Storage usage: Measurements - %d/%d, Daily max %d/%d", 
                    getNumberOfMeasurementRecords(), MEASUREMENTSTORAGESIZE,
                    getNumberOfMaxPowerRecords(), MAXPOWERSTORAGESIZE);
}
