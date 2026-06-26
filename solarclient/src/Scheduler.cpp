/**************************************************************************************************\
*
* Scheduler.cpp
*
* Exwecutes periodic functions
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <pthread.h>


#include "Scheduler.h"
#include "Connection.h"
#include "Configuration.h"

Scheduler* Scheduler::theInstance=NULL;

/******************************************************************************\
*
* The scheduler task
*
\******************************************************************************/
void* schedulerTask(void* param)
{
    tm*         daytime;

    Scheduler* scheduler    =(Scheduler*)param;

    bool timeSynced         =false;
    bool dataRetrieved      =false;
    bool dayProcessed       =false;
    bool weekProcessed      =false;

    scheduler->logger.logInfo("Scheduler task started");

    pthread_mutex_lock(&scheduler->mutex);
    scheduler->taskRunning  =true;
    bool localExitTask      =scheduler->exitTask;
    pthread_mutex_unlock(&scheduler->mutex);

    scheduler->preScheduleIntialise();

    // Now run the scheduler loop
    while (!localExitTask)
    {
        daytime=Clock::getTime();

        // Synchronise the time of the server at 3:12:30 o'clock
        // Original     : By that time the summer/wintertime correction should have been made
        // Later        : Time is not depending on summer/winter time.
        // Even later   : No effect since timesync is done by the device using NTP
        if ((daytime->tm_hour==3) && (daytime->tm_min==12) && (daytime->tm_sec==30))
        {
            if (!timeSynced)
            {
                // sync time
                scheduler->client->syncTime();
                timeSynced=true;
            }
        }
        else
        {
            timeSynced=false;
        }

        // Update the day values per day at 00:02:30 GMT
        // This is 2.5 minutes after the last 5 minute value of the (previous) day has been received
        if ((daytime->tm_hour==0) && (daytime->tm_min==2) && (daytime->tm_sec==30))
        {
            if (!dayProcessed)
            {
                //  Request yesterdays' instant max value
                scheduler->logger.logInfo("Requesting stored instant max values");
                bool error=scheduler->client->requestStoredInstantMaxValues();
                if (!error)
                {
                    scheduler->client->acknowledgeInstantMaxValues();
                }
                dayProcessed=true;
            }
        }
        else
        {
            dayProcessed=false;
        }

        // Update the week values every sunday at 00:02:30 GMT
        // This is 2.5 minutes after the last 5 minute value of the (previous) day has been received
        if ((daytime->tm_hour==0) && (daytime->tm_min==2) && (daytime->tm_sec==30) && (daytime->tm_wday==0))
        {
            if (!weekProcessed)
            {
                //  Request meter readings
                scheduler->logger.logInfo("Processing Week");
                //scheduler->requestMeterReadings();
                weekProcessed=true;
            }
        }
        else
        {
            weekProcessed=false;
        }

        // Get last measured data 10 seconds after the interval passes
        if ((((daytime->tm_min)%MEASUREMENT_INTERVAL)==0) && (daytime->tm_sec==10))
        {
            if (!dataRetrieved)
            {
                bool error=scheduler->client->requestMeasurements();
                if (!error)
                {
                    scheduler->client->acknowledgeMeasurements();
                }
                if (daytime->tm_hour==23 && daytime->tm_min==55)
                {
                    // TO DO: processDay(); not it is in client requestData()...
                    // However, it needs to be there, when processing in bulk
                }
                dataRetrieved=true;
            }
        }
        else
        {
            dataRetrieved=false;
        }

        usleep(200000);                                // sleep one 0.2 second second

        pthread_mutex_lock(&scheduler->mutex);
        localExitTask=scheduler->exitTask;
        pthread_mutex_unlock(&scheduler->mutex);
    }

    pthread_mutex_lock(&scheduler->mutex);
    scheduler->taskRunning=false;
    pthread_mutex_unlock(&scheduler->mutex);

    pthread_exit(NULL);
    return  NULL;
}


/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
Scheduler::Scheduler()
{
    taskRunning     =false;
    exitTask        =false;
    configuration   =Configuration::getInstance();
}

/******************************************************************************\
*
* Start scheduler task
*
\******************************************************************************/
void Scheduler::start()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

    int err=pthread_mutex_init(&mutex, &attr);
    if (!err)
    {
        err=pthread_create(&threadId, NULL, schedulerTask, (void *)this);

        if (err)
        {
            logger.logFatal("Could not start scheduler");
        }
    }
    else
    {
        logger.logFatal("Unable to create mutex");
    }
}

/******************************************************************************\
*
* This method stops the scheduler task
*
\******************************************************************************/
void Scheduler::stop()
{
    pthread_mutex_lock(&mutex);
    exitTask                =true;                      // sign task to bail out
    bool localTaskRunning   =taskRunning;
    pthread_mutex_unlock(&mutex);

    while(localTaskRunning)                             // wait till the task bails out
    {
        usleep(1000);
        pthread_mutex_lock(&mutex);
        localTaskRunning=taskRunning;
        pthread_mutex_unlock(&mutex);
    }
    logger.logInfo("Scheduler stopped");
}


/******************************************************************************\
*
* This method performs initialising before starting the scheduled tasks
*
\******************************************************************************/
void Scheduler::preScheduleIntialise()
{
    // WORK AROUND!!!! THE SOCKETS WON'T WORK IF THE Connection INSTANCE
    // IS CREATED AFTER THE 1ST DATABASE ACCESS
    Connection::getInstance();

    // Get and display the Mysql version
    DataStore* dataStore=DataStore::getInstance();
    bool error=dataStore->openDatabase();
    if (!error)
    {
        dataStore->getDatabaseVersion();
        dataStore->closeDatabase();
    }
    else
    {
        logger.logFatal("Database not available");
    }

    // The communication channel
    client=Client::getInstance();


    // Before starting sync the time
    client->requestTime();
    client->syncTime();

    // Send pulse calibration
    client->calibratePulses();

    // Get info on how much is in storage
    client->requestStorageInfo();

    // Request all available data
    error=client->requestMeasurements();
    if (!error)
    {
        client->acknowledgeMeasurements();
    }
    // Request all available instant power max values
    error=client->requestStoredInstantMaxValues();
    if (!error)
    {
        client->acknowledgeInstantMaxValues();
    }
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of the class (Singleton)
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
