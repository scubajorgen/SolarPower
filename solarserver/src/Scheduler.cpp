/******************************************************************************\
*
* Scheduler.cpp
* Scheduler for periodic tasks
*
\******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "Scheduler.h"
#include "Configuration.h"

#define NANOSECONDS_PER_MILLISECOND 1000000LL
#define SLEEPINTERVALMICROSECONDS   1000
#define TICK_INTERVAL_NANOSECONDS   ((INT128)SAMPLE_TIME*(INT128)NANOSECONDS_PER_SECOND/(INT128)MICROSECONDS_PER_SECOND)

/******************************************************************************\
* Variables
\******************************************************************************/

Scheduler*  Scheduler::theInstance=NULL;

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

    // First start with the simulation process, to ensure that we have 
    // simulated values before starting the measurement process
    if (scheduler->simulationMode)
    {
        scheduler->simulation->process();
    }

    // The task loop. It is going to be executed at 10 ms boundaries (0 ms, 10 ms, 20 ms, ...)
    // Calculate the next tick boundary, i.e. the next 10 ms boundary
    INT128 tick                     =Clock::getNanoSeconds();
    INT128 nextTickBoundary         =((tick+TICK_INTERVAL_NANOSECONDS)/TICK_INTERVAL_NANOSECONDS)*TICK_INTERVAL_NANOSECONDS;
    if (nextTickBoundary<=tick)
    {
        nextTickBoundary+=TICK_INTERVAL_NANOSECONDS;
    }
    scheduler->logger.logInfo("Scheduler task started @ tick: %lld ms, first boundary: %lld ms, interval: %lld ms", 
                                   tick/NANOSECONDS_PER_MILLISECOND, nextTickBoundary/NANOSECONDS_PER_MILLISECOND, 
                                   TICK_INTERVAL_NANOSECONDS/NANOSECONDS_PER_MILLISECOND);
    while (!localCloseTask)
    {
        tick=Clock::getNanoSeconds();
        // The 10 ms process
        if (tick>=nextTickBoundary)
        {
            // every cycle/tick give processing power to each meter
            scheduler->processMeters();

            // Every second, execute the measurement state machine and other processes
            if (tenMillisecondPeriodCounter>=ONESECOND)
            {
                // Publish last received pulses (websocket, MQTT)
                if (publishIntervalCounter>=PUBLISHINTERVAL)
                {
                    scheduler->publishCounters();
                    publishIntervalCounter=0;
                }
                else
                {
                    publishIntervalCounter++;
                }

                // Toggle heartbeat LED to show activity
                if (scheduler->led1State)
                {
                    (scheduler->ioPins)->setLed(IOPINS_LED_HEARTBEAT, 0);
                }
                else
                {
                    (scheduler->ioPins)->setLed(IOPINS_LED_HEARTBEAT, 1);
                }
                scheduler->led1State=!scheduler->led1State;


                // Execute the measurement statemachine
                scheduler->measureStateMachine();

                // Every second, give processing to the simulation
                if (scheduler->simulationMode)
                {
                    scheduler->simulation->process();
                }

                // Reset tenmillisecond counter
                tenMillisecondPeriodCounter=0;
            }
            else
            {
                tenMillisecondPeriodCounter++;
            }

            INT128 prelockTick      =Clock::getNanoSeconds();
            // Check whether the task needs to be killed
            pthread_mutex_lock(&scheduler->mutex);
            localCloseTask=scheduler->closeTask;
            pthread_mutex_unlock(&scheduler->mutex);

            INT128 finishedTick     =Clock::getNanoSeconds();
            if (finishedTick-tick>TICK_INTERVAL_NANOSECONDS)
            {
                scheduler->logger.logInfo("Execution took to long: %lld us, max 10000 us; prelock %lld us, second counter %d", 
                                          (finishedTick-tick)/1000LL, (finishedTick-prelockTick)/1000LL, tenMillisecondPeriodCounter);
            }

            nextTickBoundary        +=TICK_INTERVAL_NANOSECONDS;
        }
        // Sleep 1 ms = 1000 us
        usleep(SLEEPINTERVALMICROSECONDS);
    }

    // Signal the task bailing out
    pthread_mutex_lock(&scheduler->mutex);
    scheduler->taskRunning=false;
    pthread_mutex_unlock(&scheduler->mutex);

    // End of excercise
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
    solarClock->getTime(&schedulerStartTime);

    measurementStorage      =MeasurementStorage::getInstance();

    smartMeter              =SmartMeter::getInstance();

    // initialise the measuring state machine
    measureState            =MEASURESTATE_IDLE;
    firstMeasurement        =true;

    // initialise the counting state machine
    ioPins                  =IoPins::getInstance();

    led1State               =false;

    // create counters, daily power max is reset as part of this
    Configuration* configuration=Configuration::getInstance();
    counters[0]             =new PulseCounter(PULSE1, configuration->getPulseMeterUsage(0), configuration->getPulseMeterFileName(0));    // Solar production
    counters[1]             =new PulseCounter(PULSE2, configuration->getPulseMeterUsage(1), configuration->getPulseMeterFileName(1));    // Solar consumption
    counters[2]             =new PulseCounter(PULSE3, configuration->getPulseMeterUsage(2), configuration->getPulseMeterFileName(2));    // Not used

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

    simulationMode          =configuration->getSimulationMode();
    simulation              =Simulation::getInstance();

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
* meter. Runs every SAMPLE_TIME microsecomds
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
    // process the smart meter
    smartMeter->process();
    pthread_mutex_unlock(&mutex);
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
        if (firstMeasurement)
        {
            // For the first measurement we need to correct the values, since the measurement
            // interval was shorter than the normal interval. We do this by scaling the values to the normal interval.
            int intervalSeconds=(int)(measurement.datetime.epoch-measuringStartTime.epoch+0.5);
            logger.logInfo("Scaling first measurement for pulse counter %d, measurement interval was %d seconds", i, intervalSeconds);
            counters[i]->scaleMeasurement(&measurement, intervalSeconds);
        }
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
    logger.logDebug("Store & Rest daily max power values");
    pthread_mutex_lock(&mutex);
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        // store the maximum value of the day
        counters[i]->getCurrentPowerMax(&(maxPower.maxPowerTimeDiff[i]), &(maxPower.maxPower[i]), &(maxPower.maxPowerTime[i]));

        // reset the value for the next day.
        counters[i]->resetCurrentPowerMax();
    }
    pthread_mutex_unlock(&mutex);

    // Store outside the mutex area, to prevent deadlock
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
    int remainingSeconds;
    solarClock->getTime(&pulseTime);
    year=pulseTime.year;

    switch (measureState)
    {
        case MEASURESTATE_IDLE:
            // We start if we have at least a minimum measurement interval size before the next measurement interval boundary. 
            // Prerequisite is that we have a valid reading from the smart meter, otherwise we cannot start measuring
            remainingSeconds        =(MEASUREMENT_INTERVAL-(pulseTime.minute%MEASUREMENT_INTERVAL))*SECONDS_PER_MINUTE-pulseTime.second;
            if (remainingSeconds>MINIMUM_INTERVAL_SIZE && smartMeter->hasReading())
            {
                // This means: record the time index of this interval and start counting/measuring for the 1st measurement!

                // calculate the index in the year of the 1st measurement
                startTimeIndex      =Clock::calculateYearTimeIndex(&pulseTime);
                previousTimeIndex   =startTimeIndex;
                previousYear        =year;
                measuringStartTime  =pulseTime;
                firstMeasurement    =true;

                resetMeasurement();

                logger.logInfo("Started measuring @ %02d:%02d:%02d, current timeIndex %d, remaining time for interval: %d seconds", 
                                pulseTime.hour, pulseTime.minute, pulseTime.second, startTimeIndex, remainingSeconds);
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
                if ((pulseTime.hour==0) && (pulseTime.minute==0))
                {
                    storeAndResetMaxPowerValues();
                }
                firstMeasurement    =false;
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
        pthread_mutex_unlock(&mutex);
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
    if (simulationMode)
    {
        delete(simulation);
    }
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
void Scheduler::getCurrentPowerMax(maxPower_t* maxPower)
{
    pthread_mutex_lock(&mutex);
    for (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        // store the maximum value of the day
        counters[i]->getCurrentPowerMax(&(maxPower->maxPowerTimeDiff[i]), &(maxPower->maxPower[i]), &(maxPower->maxPowerTime[i]));
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

/******************************************************************************\
*
* This function prints the status
*
\******************************************************************************/
void Scheduler::logStatus()
{
    pthread_mutex_lock(&mutex);
    logger.logReport("____________________ STATUS ____________________");
    logger.logReport("SolarServer version %s", VERSION);
    logger.logReport("Scheduler started %02d-%02d-%04d %02d:%02d:%02d GMT",
                    schedulerStartTime.day, schedulerStartTime.month, schedulerStartTime.year, 
                    schedulerStartTime.hour, schedulerStartTime.minute, schedulerStartTime.second);
    smartMeter->logStatus();
    for(int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        counters[i]->logStatus();
    }
    measurementStorage->logStatus();
    SolarPublish::getInstance()->logStatus();
    logger.logReport("________________________________________________");
    pthread_mutex_unlock(&mutex);
}