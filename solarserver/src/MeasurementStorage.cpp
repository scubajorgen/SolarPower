/**************************************************************************************************\
*
* MeasurementStorage.cpp
*
* Storage for measurements and daily maxima
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
    endOfArray              =0;

    maxPowerStartOfArray    =0;
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
    int number;

    pthread_mutex_lock(&mutex);  
    number=endOfArray-startOfArray;
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
    int number;
    
    pthread_mutex_lock(&mutex); 
    number=maxPowerEndOfArray-maxPowerStartOfArray;
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
void MeasurementStorage::appendMeasurement(Measurement_t* measurement)
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

    pthread_mutex_unlock(&mutex);  

    logger.logInfo("Measurement - Time %d/%d (%02d-%02d-%04d %02d:%02d:%02d.%02d),",                         
                       measurement->year, measurement->timeIndex,
                       measurement->datetime.day, measurement->datetime.month, 2000+measurement->datetime.year,
                       measurement->datetime.hour, measurement->datetime.minute, measurement->datetime.second, measurement->datetime.centisecond);
    logger.logInfo("              P1 %d (%d/%d Watt, %d Wh), P2 %d (%d/%d Watt, %d Wh), P3 %d (%d/%d Watt, %d Wh), ",
                       measurement->pulse[0],measurement->pulsePower[0]/10, measurement->pulseMaxPower[0],measurement->pulseMeter[0],
                       measurement->pulse[1],measurement->pulsePower[1]/10, measurement->pulseMaxPower[1],measurement->pulseMeter[1],
                       measurement->pulse[2],measurement->pulsePower[2]/10, measurement->pulseMaxPower[2],measurement->pulseMeter[2]);
    logger.logInfo("              Eimp low %d, Eimp normal %d, Eexp low %d, Eexp normal %d, Gas %d",
                       measurement->electricityImportLow, measurement->electricityImportNormal,
                       measurement->electricityExportLow, measurement->electricityExportNormal,
                       measurement->gasImport);
    logger.logInfo("              Gross power %d W, net power %d W",
                       measurement->grossPower/10, measurement->netPower/10);
                    }

/******************************************************************************\
*
*  This method returns a specific measurement. Number indicates
*  the nth measurement with respect to the startOfArray. If it is negative
*  the last stored measurement is returned.
*
\******************************************************************************/
bool MeasurementStorage::getMeasurement(int number, Measurement_t* measurement)
{
    int     index;
    bool    error;
    int     recordsInStore;
    
    error=true;

    pthread_mutex_lock(&mutex); 
    
    // i.s.o. calling getNumberOfMeasurementRecords, just calculate the number
    // saves an expensive mutex lock
    recordsInStore=endOfArray-startOfArray;
    if (recordsInStore<0)
    {
        recordsInStore+=MEASUREMENTSTORAGESIZE;
    }

    if (recordsInStore>0)
    {
        // If number<0, just return the last measurement
        if (number<0)
        {
            number=recordsInStore-1;
        }
        
        if (number<recordsInStore)
        {
    
            index=startOfArray+number;
            if (index>=MEASUREMENTSTORAGESIZE)
            {
                index-=MEASUREMENTSTORAGESIZE;
            }
            *measurement=measurements[index];
            error=false;
        }
        else
        {
            logger.logError("Requesting non existing measurement");
        }
    }
    pthread_mutex_unlock(&mutex); 

    return error;
}

/******************************************************************************\
*
*  This method returns the last measured value
*
\******************************************************************************/
bool MeasurementStorage::getLastMeasurement(Measurement_t* measurement)
{
    return getMeasurement(-1, measurement);
}



/******************************************************************************\
*
* Add a measurement to the measurement array
*
\******************************************************************************/
void MeasurementStorage::appendMaxPower(MaxPower_t* maxPower)
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
    pthread_mutex_unlock(&mutex); 
}

/******************************************************************************\
*
*  This method returns the last max power values
*
\******************************************************************************/
bool MeasurementStorage::getPowerMaxValue(int number, MaxPower_t* maxPower)
{
    int     index;
    bool    error;
    int     recordsInStore;
    
    error=true;

    pthread_mutex_lock(&mutex); 
    
    // i.s.o. calling getNumberOfMeasurementRecords, just calculate the number
    // saves an expensive mutex lock
    recordsInStore=maxPowerEndOfArray-maxPowerStartOfArray;
    if (recordsInStore<0)
    {
        recordsInStore+=MAXPOWERSTORAGESIZE;
    }

    if (recordsInStore>0)
    {
        if (number<0)
        {
            number=recordsInStore-1;
        }
        if (number<recordsInStore)
        {
            index=maxPowerStartOfArray+number;
            if (index>=MAXPOWERSTORAGESIZE)
            {
                index-=MAXPOWERSTORAGESIZE;
            }
            *maxPower=maxPowers[index];
            error=false;    
        }
        else
        {

        }
    }
    pthread_mutex_unlock(&mutex);
     
    return error;
}

/******************************************************************************\
*
*  This method returns the last measured value
*
\******************************************************************************/
bool MeasurementStorage::getLastPowerMaxValue(MaxPower_t* maxPower)
{
    return getPowerMaxValue(-1, maxPower);
}

