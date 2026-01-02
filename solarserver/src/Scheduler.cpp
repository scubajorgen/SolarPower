/******************************************************************************\
*
* Scheduler.cpp
* Scheduler for periodic tasks
*
\******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Scheduler.h"
#include "Configuration.h"

/******************************************************************************\
* Variables
\******************************************************************************/

Scheduler*  Scheduler::theInstance=NULL;
char        teststr[100];

/******************************************************************************\
* Friend methods
\******************************************************************************/
/******************************************************************************\
*
* The task function
*
\******************************************************************************/
void* schedulerTask(void* param)
{
    Scheduler* scheduler            =Scheduler::getInstance();

    // Signal the task is running
    pthread_mutex_lock(&scheduler->mutex);
    scheduler->taskRunning          =true;
    pthread_mutex_unlock(&scheduler->mutex);

    scheduler->logger.logInfo("Scheduler task started");

    // Some local variables
    bool localCloseTask             = false;
    int  tenMillisecondPeriodCounter= 0;
    int  publishIntervalCounter     = 0;

    // the task loop
    while (!localCloseTask)
    {
        // every cycle (1/100 s) give processing power to each meter
        scheduler->processMeters();

        // Every second, execute the measurement state machine
        if (tenMillisecondPeriodCounter==ONESECOND)
        {
            // publish last received pulses
            publishIntervalCounter++;
            if (publishIntervalCounter>=PUBLISHINTERVAL)
            {
                scheduler->publishCounters();
                publishIntervalCounter=0;
            }

            // toggle led1
            if (scheduler->led1State)
            {
                (scheduler->ioPins)->setLed(IOPINS_LED_HEARTBEAT, 0);
            }
            else
            {
                (scheduler->ioPins)->setLed(IOPINS_LED_HEARTBEAT, 1);
            }
            scheduler->led1State=!scheduler->led1State;


            // execute the measurement statemachine
            scheduler->measureStateMachine();

            // reset tenmillisecond counter
            tenMillisecondPeriodCounter=0;
        }
        else
        {
            tenMillisecondPeriodCounter++;
        }

        // Sleep 10 ms = 10000 us
        usleep(SAMPLE_TIME);

        // Check whether the task needs to be killed
        pthread_mutex_lock(&scheduler->mutex);
        localCloseTask=scheduler->closeTask;
        pthread_mutex_unlock(&scheduler->mutex);
    }

    // Signal the task bailing out
    pthread_mutex_lock(&scheduler->mutex);
    scheduler->taskRunning=false;
    pthread_mutex_unlock(&scheduler->mutex);

    // end of excercise
    pthread_exit(NULL);
    return  NULL;
}



/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/
Scheduler::Scheduler()
{
    int                     err;
    pthread_mutexattr_t     attr;

    solarClock              =Clock::getInstance();

    measurementStorage      =MeasurementStorage::getInstance();

    smartMeter              =SmartMeter::getInstance();

    // initialise the measuring state machine
    measureState            =MEASURESTATE_IDLE;

    // initialise the counting state machine
    ioPins                  =IoPins::getInstance();

    led1State               =false;

    // create counters
    Configuration* configuration=Configuration::getInstance();
    counters[0]             =new PulseCounter(PULSE1, true , configuration->getPulseMeterFileName1());    // Solar production
    counters[1]             =new PulseCounter(PULSE2, false, configuration->getPulseMeterFileName2());    // Solar consumption
    counters[2]             =new PulseCounter(PULSE3, true , configuration->getPulseMeterFileName3());    // Not used

    // prepare the empty measurement
    
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        emptyMeasurement.pulse[i]           =INVALID_MEASUREMENT;
        emptyMeasurement.pulsePower[i]      =INVALID_MEASUREMENT;
        emptyMeasurement.pulseMaxPower[i]   =INVALID_MEASUREMENT;
        emptyMeasurement.pulseMeter[i]      =INVALID_MEASUREMENT;
    }
    emptyMeasurement.electricityImportLow   =INVALID_MEASUREMENT;
    emptyMeasurement.electricityImportNormal=INVALID_MEASUREMENT;
    emptyMeasurement.electricityExportLow   =INVALID_MEASUREMENT;
    emptyMeasurement.electricityExportNormal=INVALID_MEASUREMENT;
    emptyMeasurement.gasImport              =INVALID_MEASUREMENT;
    emptyMeasurement.grossPower             =INVALID_MEASUREMENT;
    emptyMeasurement.netPower               =INVALID_MEASUREMENT;

    // initialise the task
    taskRunning             =false;

    solarPublish=SolarPublish::getInstance();

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
*
* This function processes the meters: it gives processing time to each
* meter
*
\******************************************************************************/
void Scheduler::processMeters()
{
    // process the pulse meters
    pthread_mutex_lock(&mutex);
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        counters[i]->process();
    }
    pthread_mutex_unlock(&mutex);

    // process the smart meter
    smartMeter->process();
}

/******************************************************************************\
*
* This method creates a measurement storage record and stores it
*
\******************************************************************************/
void Scheduler::resetMeasurement()
{
    // reset the current pulsecounter for the next 5 minutes
    pthread_mutex_lock(&mutex);
    for(int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        // Reset counters for first interval
        counters[i]->startMeasurement();
    }
    smartMeter->startMeasurement();
    pthread_mutex_unlock(&mutex);
}

/******************************************************************************\
*
* This method creates a measurement storage record and stores it
*
\******************************************************************************/
void Scheduler::storeAndResetMeasurement()
{
    int recordsToBeAdded;

    // Check if there is a gap in the measurements
    recordsToBeAdded    =timeIndex-previousTimeIndex;
    // Fill the gap with INVALID_MEASUREMENTS
    for (int recordCount=0; recordCount<(recordsToBeAdded-1L); recordCount++)
    {
        measurementStorage->appendMeasurement(&emptyMeasurement);
    }

    // Now, build the measurement
    pthread_mutex_lock(&mutex);
    // Fill the measurement
    measurement.timeIndex   =previousTimeIndex; // start of interval
    measurement.year        =previousYear;

    // Add the GMT time
    solarClock->getTime(&measurement.datetime);

    // Retrieve pulse meter measurements and calculate production
    INT32 production=0;
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        counters[i]->retrieveAndRestartMeasurement(&measurement);
        if (counters[i]->isProductionMeter())
        {
            production+=measurement.pulsePower[i];
        }
    }

    // Retrieve the smart meter measurements
    smartMeter->retrieveAndRestartMeasurement(&measurement);

    // Calculate the average gross power usage for the interval
    measurement.grossPower=measurement.netPower+production;
    pthread_mutex_unlock(&mutex);

    measurementStorage->appendMeasurement(&measurement);
}

/******************************************************************************\
*
* This method creates a measurement storage record and stores it
*
\******************************************************************************/
void Scheduler::storeAndResetMaxPowerValues()
{
    pthread_mutex_lock(&mutex);
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        // store the maximum value of the day
        counters[i]->getCurrentPowerMax(&(maxPower.maxPowerTimeDiff[i]), &(maxPower.maxPower[i]), &(maxPower.maxPowerTime[i]));
        // reset the value for the next day.
        counters[i]->resetCurrentPowerMax();
    }
    pthread_mutex_unlock(&mutex);

    measurementStorage->appendMaxPower(&maxPower);
}

/******************************************************************************\
*
* This method implements the measurement state machine.
* MEASURESTATE_IDLE     : waiting for the 1st 5 minute boundary
* MEASURESTATE_WAITING  : waiting for next 5 minute boundaries
* MEASURESTATE_COLLECTED: boundary passed and measurement made, waiting for the
*                         next minute
*
\******************************************************************************/
void Scheduler::measureStateMachine()
{
    solarClock->getTime(&pulseTime);
    year=pulseTime.year+2000;

    switch (measureState)
    {
    case MEASURESTATE_IDLE:
        if (((pulseTime.minute%MEASUREMENT_INTERVAL)==0) && (pulseTime.second<2))
        {
            // Now the start of the 1st measurement period has passed!!!
            // This means: record the time index of this interval and start counting!

            // calculate the index in the year of the 1st measurement
            startTimeIndex      =Clock::calculateYearTimeIndex(&pulseTime);
            previousTimeIndex   =startTimeIndex;
            previousYear        =year;

            resetMeasurement();

            // Indicates the startTimeIndex (index of first measurement interval in the array)
            measurementStorage->setStartTimeIndex(startTimeIndex);
            logger.logInfo("First 5 min boundary encountered. Started measuring...");
            measureState=MEASURESTATE_COLLECTED;
        }
        break;

    case MEASURESTATE_WAITING:
        // The state waits for a measurment interval boundary. Eg. if the interval is 5 minutes
        // it triggers at 0:00, 0:05, 0:10 hr
        // It is executed only at the start of this minute between 0:00:00-0:00:02, etc
        if ((pulseTime.minute%MEASUREMENT_INTERVAL)==0)
        {
            // In fact, timeIndex is the time index of NEXT period
            timeIndex=Clock::calculateYearTimeIndex(&pulseTime);
            storeAndResetMeasurement();
            previousTimeIndex   =timeIndex;
            previousYear        =year;

            // After the final interval of previous day has been stored (starting at 23:55, ending at 00:00)
            // store and reset the instant max counter
            if ((pulseTime.hour==00) && (pulseTime.minute==00))
            {
                storeAndResetMaxPowerValues();
            }
            measureState=MEASURESTATE_COLLECTED;
        }
        break;

    case MEASURESTATE_COLLECTED:
        // Stay insensitive till measurement minute has passed
        if ((pulseTime.minute%MEASUREMENT_INTERVAL)!=0)
        {
            measureState=MEASURESTATE_WAITING;
        }
        break;
    }
}


/******************************************************************************\
*
* Publish the last measured power for each counter to websocket, message queue,
* etc
*
\******************************************************************************/
void Scheduler::publishCounters()
{
    if (solarPublish!=NULL)
    {
        pthread_mutex_lock(&mutex);
        for(int pulseId=0; pulseId<MAX_PULSE_COUNTERS; pulseId++)
        {
            INT32 power=counters[pulseId]->getPublishPower();
            if (power>=0)
            {
                solarPublish->postMessage(pulseTime, pulseId, power);
            }
        }
        pthread_mutex_unlock(&mutex);

        INT32 netPower          =smartMeter->getCurrentNetPower();
        INT32 productionPower   =0;
        for (int id=0; id<MAX_PULSE_COUNTERS; id++)
        {
            productionPower+=counters[id]->getCurrentExportPower();
        }

        solarPublish->postMessage(pulseTime,
                                    NETPOWER,
                                    netPower);
        solarPublish->postMessage(pulseTime,
                                GROSSPOWER,
                                netPower+productionPower);
    }
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
*  This method returns the one and only instance (Singleton)
*
\******************************************************************************/
Scheduler* Scheduler::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new Scheduler();
        theInstance->start();
    }
    return theInstance;
}

/******************************************************************************\
*
* Destructor
*
\******************************************************************************/
Scheduler::~Scheduler()
{
    stop();
}

/******************************************************************************\
*
*  This method starts the task
*
\******************************************************************************/
void Scheduler::start()
{
    closeTask               =false;
    int err=pthread_create(&threadId, NULL, schedulerTask, (void *)this);

    if (err)
    {
        logger.logFatal("Could not start scheduler");
    }
}

/******************************************************************************\
*
*  This method kills the task
*
\******************************************************************************/
void Scheduler::stop()
{
    bool localTaskRunning;

    pthread_mutex_lock(&mutex);
    closeTask       =true;
    localTaskRunning=taskRunning;
    pthread_mutex_unlock(&mutex);

    while (localTaskRunning)
    {
        usleep(10000);
        pthread_mutex_lock(&mutex);
        localTaskRunning=taskRunning;
        pthread_mutex_unlock(&mutex);
    }
    logger.logInfo("Counting task stopped");
}

/******************************************************************************\
*
*  This method returns the values of the instantaneous max power
*  The instantaneous max power is the max power as defined by the time
*  between two adjacent pulses
*
\******************************************************************************/
void Scheduler::getCurrentPowerMax(MaxPower_t* maxPower)
{
    int i;

    pthread_mutex_lock(&mutex);
    i=0;
    while (i<MAX_PULSE_COUNTERS)
    {
        // store the maximum value of the day
        counters[i]->getCurrentPowerMax(&(maxPower->maxPowerTimeDiff[i]), &(maxPower->maxPower[i]), &(maxPower->maxPowerTime[i]));
        i++;
    }
    pthread_mutex_unlock(&mutex);
}

/******************************************************************************\
*
*  This method resets the max power values
*  The instantaneous max power is the max power as defined by the time
*  between two adjacent pulses
*
\******************************************************************************/
void Scheduler::resetCurrentPowerMax()
{
    pthread_mutex_lock(&mutex);
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        // store the maximum value of the day
        counters[i]->resetCurrentPowerMax();
    }
    pthread_mutex_unlock(&mutex);
}

