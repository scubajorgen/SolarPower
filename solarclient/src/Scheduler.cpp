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

    // WORK AROUND!!!! THE SOCKETS WON'T WORK IF THE Connection INSTANCE
    // IS CREATED AFTER THE 1ST DATABASE ACCESS
    Connection::getInstance();

    // Get and display the Mysql version
    scheduler->dataStore=DataStore::getInstance();
    (scheduler->dataStore)->openDatabase();
    (scheduler->dataStore)->getDatabaseVersion();
    (scheduler->dataStore)->closeDatabase();

    // Before starting sync the time
    scheduler->requestTime();
    scheduler->syncTime();

    // Send pulse calibration
    scheduler->calibratePulses();

    // Request all available data
    scheduler->requestData(REQUESTDATA_ALL);

    // Since requesting ALL data may take longer than 5min,
    // a new measurement may have done. Fetch the last
    // data to make sure we don't miss this measurement.
    scheduler->requestData(REQUESTDATA_LASTTEN);

    // Request all available instant power max values
    scheduler->requestStoredInstantMaxValues(false);

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
                scheduler->syncTime();
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
                scheduler->requestStoredInstantMaxValues(true);
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
                scheduler->requestData(REQUESTDATA_LAST);
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
* This method sends the pulse calibration
*
\******************************************************************************/
void Scheduler::calibratePulses()
{
    logger.logInfo("Calibrating pulse counters");

    Configuration* configuration                    =Configuration::getInstance();

    sendReceiveBuffer[0]=COMMAND_CALIBRATEPULSE;
    *(INT32*)(sendReceiveBuffer+1                )  =configuration->getPulsesPerKwh(0);
    *(INT32*)(sendReceiveBuffer+1+  sizeof(INT32))  =configuration->getPulsesPerKwh(1);
    *(INT32*)(sendReceiveBuffer+1+2*sizeof(INT32))  =configuration->getPulsesPerKwh(2);;

    // send it
    Connection* connection=Connection::getInstance();
    connection->connectToServer();
    connection->sendData(sendReceiveBuffer, 3*sizeof(int)+1);

    // wait for acknowledge
    int receiveLength=0;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if ((receiveLength==1) && (sendReceiveBuffer[0]==COMMAND_ACK))
    {
        logger.logInfo("Calibration successful");
    }
    else
    {
        logger.logError("Error synchronising time");
    }

    connection->disconnectFromServer();
}


/******************************************************************************\
*
* This method sends the time to the server to synchronise it
*
\******************************************************************************/
void Scheduler::syncTime()
{
    logger.logInfo("Synchronising time");

    // Make sure we've got the current time
    daytime=Clock::getTime();

    // convert
    currentTime.year        =char(daytime->tm_year-100);
    currentTime.month       =daytime->tm_mon+1;
    currentTime.day         =daytime->tm_mday;
    currentTime.hour        =daytime->tm_hour;
    currentTime.minute      =daytime->tm_min;
    currentTime.second      =daytime->tm_sec;
    currentTime.centisecond =0;                     // yeah, skip the sub second stuff. 1 second accurate is accurate enough

    logger.logInfo("Sending: %02d:%02d:%02d %02d-%02d-%04d",
           currentTime.hour, currentTime.minute, currentTime.second,
           currentTime.day, currentTime.month, 2000+currentTime.year);

    // copy to buffer
    sendReceiveBuffer[0]=COMMAND_ADJUSTTIME;
    memcpy(sendReceiveBuffer+1, (char*)&currentTime, sizeof(currentTime));

    // send it
    Connection* connection=Connection::getInstance();
    connection->connectToServer();
    connection->sendData(sendReceiveBuffer, sizeof(solarTime_t)+1);


    // wait for acknowledge
    int receiveLength=0;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if ((receiveLength==1) && (sendReceiveBuffer[0]==COMMAND_ACK))
    {
        logger.logInfo("Time synchronisation successful");
    }
    else
    {
        logger.logError("Error synchronising time");
    }

    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method request the server time and displays it.
*
\******************************************************************************/
void Scheduler::requestTime()
{
    logger.logInfo("Requesting time");

    // Make sure we've got the current time
    daytime=Clock::getTime();

    // copy to buffer
    sendReceiveBuffer[0]=COMMAND_GETTIME;

    // send it
    Connection* connection=Connection::getInstance();
    connection->connectToServer();
    connection->sendData(sendReceiveBuffer, 1);

    // wait for acknowledge
    int receiveLength=0;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if ((receiveLength==1+sizeof(solarTime_t)) && (sendReceiveBuffer[0]==COMMAND_RECEIVETIME))
    {
        logger.logInfo("Received time");
        solarTime_t* serverTime =(solarTime_t*)(sendReceiveBuffer+1);
        currentTime             =*serverTime;
        logger.logInfo("Local datetime: %02d-%02d-%4d %02d:%02d:%02d Server datetime: %02d-%02d-%04d %02d:%02d:%02d",
                        daytime->tm_mday, daytime->tm_mon+1, daytime->tm_year+1900, daytime->tm_hour, daytime->tm_min, daytime->tm_sec,
                        currentTime.day, currentTime.month, currentTime.year+2000, currentTime.hour, currentTime.minute, currentTime.second);
    }
    else
    {
        logger.logError("Error requesting time");
    }

    connection->disconnectFromServer();
}
/******************************************************************************\
*
* This method requests the instant power max
*
\******************************************************************************/
void Scheduler::requestInstantMax()
{
    logger.logInfo("Requesting instant max");

    // copy to buffer
    sendReceiveBuffer[0]=COMMAND_SENDINSTANTMAX;

    // send it
    Connection* connection=Connection::getInstance();
    connection->connectToServer();
    connection->sendData(sendReceiveBuffer, 1);


    // wait for acknowledge
    int receiveLength=0;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if ((receiveLength==(sizeof(char)+MAX_PULSE_COUNTERS*(sizeof(int)+sizeof(solarTime_t)))) && (sendReceiveBuffer[0]==COMMAND_RECEIVEINSTANTMAX))
    {
        logger.logInfo("Received instant max");

        for (int i=0; i<MAX_PULSE_COUNTERS; i++)
        {
            maxs.timeDiff[i]    = *(int*)(sendReceiveBuffer+1+i*(sizeof(int)+sizeof(solarTime_t)));
            maxs.time[i] =*(solarTime_t*)(sendReceiveBuffer+1+sizeof(int)+i*(sizeof(int)+sizeof(solarTime_t)));

            printf("Counter %d: Time %02d-%02d-%04d %02d:%02d:%02d.%02d, power max: dT=%d cs, P=%d Watt",
                    i+1,
                    maxs.time[i].day, maxs.time[i].month, maxs.time[i].year+2000,
                    maxs.time[i].hour, maxs.time[i].minute, maxs.time[i].second, maxs.time[i].centisecond,
                    maxs.timeDiff[i],
                    3600*100/maxs.timeDiff[i]*WATTHOUR_PER_KILOWATTHOUR/configuration->getPulsesPerKwh(i));
        }
    }
    else
    {
        logger.logError("Error requesting instant max");
    }

    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method resets the instant max value at the server
* NOT USED
*
\******************************************************************************/
void Scheduler::resetInstantMax()
{
    logger.logInfo("Resetting instant max");

    // copy command to buffer
    sendReceiveBuffer[0]=COMMAND_RESETINSTANTMAX;


    // send it
    Connection* connection=Connection::getInstance();
    connection->connectToServer();
    connection->sendData(sendReceiveBuffer, 1);


    // wait for acknowledge
    int receiveLength=0;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if ((receiveLength==1) && (sendReceiveBuffer[0]==COMMAND_ACK))
    {
        logger.logInfo("Instant max power value reset");
    }
    else
    {
        logger.logError("Error resetting instant power max");
    }

    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method requests the last data from the server
*
\******************************************************************************/
void Scheduler::requestData(requestdata_t typeOfRequest)
{
    // depending on onlyLastData whether ALL data or LAST data is requested
    switch (typeOfRequest)
    {
        case REQUESTDATA_LAST:
            sendReceiveBuffer[0]=(char)COMMAND_SENDLASTDATA;
            logger.logInfo("Requesting most recent measurement");
            break;
        case REQUESTDATA_LASTTEN:
            sendReceiveBuffer[0]=(char)COMMAND_SENDLASTTENDATA;
            logger.logInfo("Requesting most recent ten measurements");
            break;
        case REQUESTDATA_ALL:
            sendReceiveBuffer[0]=(char)COMMAND_SENDALLDATA;
            logger.logInfo("Requesting all stored measurements");
            break;
    }

    // get the instance from the server connection and open the connection
    Connection* connection      =Connection::getInstance();
    connection->connectToServer();

    // open the database
    DataStore* dataStore        =DataStore::getInstance();
    dataStore->openDatabase();

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // now wait for an answer and process the answer data
    bool bailOut=false;
    while (!bailOut)
    {
        // wait for the data
        int receiveLength=0;
        connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

        // if any sensible data received, process it
        if (receiveLength>=1)
        {
            int pointer         =0;
            bool moreDataInBlock=true;
            while (moreDataInBlock)
            {
                if (sendReceiveBuffer[pointer]==COMMAND_ENDOFDATA)
                {
                    logger.logInfo("End of data");
                    moreDataInBlock=false;
                    bailOut=true;
                }
                else if (sendReceiveBuffer[pointer]==COMMAND_RECEIVEDATA)
                {
                    if (receiveLength-pointer>=84)
                    {
                        // get the values from the buffer
                        fiveMinuteMeasurement_t mmt;
                        int timeIndex;
                        int currentTimeIndex;
                        int measurementYear;
                        int day;
                        int month;
                        int hour;
                        int minute;
                        timeIndex                   =*(INT32*)(sendReceiveBuffer+pointer+ 1);
                        mmt.year                    =*(INT16*)(sendReceiveBuffer+pointer+ 5);
                        mmt.pulse[0]                =*(INT16*)(sendReceiveBuffer+pointer+ 7);
                        mmt.pulse[1]                =*(INT16*)(sendReceiveBuffer+pointer+ 9);
                        mmt.pulse[2]                =*(INT16*)(sendReceiveBuffer+pointer+11);

                        mmt.electricityImportLow    =*(INT32*)(sendReceiveBuffer+pointer+13);
                        mmt.electricityImportNormal =*(INT32*)(sendReceiveBuffer+pointer+17);
                        mmt.electricityExportLow    =*(INT32*)(sendReceiveBuffer+pointer+21);
                        mmt.electricityExportNormal =*(INT32*)(sendReceiveBuffer+pointer+25);
                        mmt.gasImport               =*(INT32*)(sendReceiveBuffer+pointer+29);

                        mmt.datetime.day            =*(INT8*) (sendReceiveBuffer+pointer+33);
                        mmt.datetime.month          =*(INT8*) (sendReceiveBuffer+pointer+34);
                        mmt.datetime.year           =*(INT8*) (sendReceiveBuffer+pointer+35);
                        mmt.datetime.hour           =*(INT8*) (sendReceiveBuffer+pointer+36);
                        mmt.datetime.minute         =*(INT8*) (sendReceiveBuffer+pointer+37);
                        mmt.datetime.second         =*(INT8*) (sendReceiveBuffer+pointer+38);
                        mmt.datetime.centisecond    =*(INT8*) (sendReceiveBuffer+pointer+39);

                        mmt.grossPower              =*(INT32*)(sendReceiveBuffer+pointer+40);
                        mmt.netPower                =*(INT32*)(sendReceiveBuffer+pointer+44);
                        mmt.pulsePower[0]           =*(INT32*)(sendReceiveBuffer+pointer+48);
                        mmt.pulsePower[1]           =*(INT32*)(sendReceiveBuffer+pointer+52);
                        mmt.pulsePower[2]           =*(INT32*)(sendReceiveBuffer+pointer+56);
                        mmt.pulseMaxPower[0]        =*(INT32*)(sendReceiveBuffer+pointer+60);
                        mmt.pulseMaxPower[1]        =*(INT32*)(sendReceiveBuffer+pointer+64);
                        mmt.pulseMaxPower[2]        =*(INT32*)(sendReceiveBuffer+pointer+68);
                        mmt.pulseMeter[0]           =*(INT32*)(sendReceiveBuffer+pointer+72);
                        mmt.pulseMeter[1]           =*(INT32*)(sendReceiveBuffer+pointer+76);
                        mmt.pulseMeter[2]           =*(INT32*)(sendReceiveBuffer+pointer+80);


                        // If the measurement is valid (contains sensible data) store it in the database
                        if (timeIndex>=0)
                        {
                            // Make sure we've got the current time
                            daytime=Clock::getTime();
                            currentTimeIndex=Clock::calculateYearTimeIndex(daytime->tm_mday,
                                                                           daytime->tm_mon+1,
                                                                           daytime->tm_hour,
                                                                           daytime->tm_min);  // note: day and month range from 1

                            // if the time index from the measurements is larger than current time index
                            // it is most probably a measurement from previous year. If it is equal or
                            // smaller, it is from this year.
                            // Note: it would be more elegant to send the date with the measurement.

                            if (timeIndex>currentTimeIndex)
                            {
                                measurementYear=daytime->tm_year-1+1900; // tm_year is year since 1900
                            }
                            else
                            {
                                measurementYear=daytime->tm_year+1900;
                            }

                            if (mmt.year!=measurementYear)
                            {
                                logger.logFatal("Year of measurement appears to be wrong");
                            }

                            mmt.timeIndex=timeIndex;

                            Clock::calculateTime(mmt.timeIndex, &day, &month, &hour, &minute);

                            bool error=dataStore->storeFiveMinuteValue(&mmt);
                            // If the value has been stored (i.e. it did not already exist in the database) print it.
                            if (!error)
                            {
                                logger.logInfo("Measurement - Time %d/%d (%02d-%02d-%04d %02d:%02d:%02d.%02d),",
                                                mmt.year, mmt.timeIndex,
                                                mmt.datetime.day, mmt.datetime.month, 2000+mmt.datetime.year,
                                                mmt.datetime.hour, mmt.datetime.minute, mmt.datetime.second, mmt.datetime.centisecond);
                                logger.logInfo("              P1 %d (%d/%d Watt, %d Wh), P2 %d (%d/%d Watt, %d Wh), P3 %d (%d/%d Watt, %d Wh), ",
                                                mmt.pulse[0],mmt.pulsePower[0]/10, mmt.pulseMaxPower[0],mmt.pulseMeter[0],
                                                mmt.pulse[1],mmt.pulsePower[1]/10, mmt.pulseMaxPower[1],mmt.pulseMeter[1],
                                                mmt.pulse[2],mmt.pulsePower[2]/10, mmt.pulseMaxPower[2],mmt.pulseMeter[2]);
                                logger.logInfo("              Eimp low %d, Eimp normal %d, Eexp low %d, Eexp normal %d, Gas %d",
                                                mmt.electricityImportLow, mmt.electricityImportNormal,
                                                mmt.electricityExportLow, mmt.electricityExportNormal,
                                                mmt.gasImport);
                                logger.logInfo("              Gross power %d W, net power %d W",
                                                mmt.grossPower/10, mmt.netPower/10);


                                // TO DO: Remove
                                logger.logInfo("CHECK         timeIndex %d %04d-%02d-%02d %02d:%02d",
                                       timeIndex,
                                       mmt.year, month, day, hour, minute);


                                // When the last measurement value of one day is received, process the day.
                                if (((timeIndex+1)%INTERVALS_PER_DAY)==0)
                                {
                                    Clock::calculateTime(mmt.timeIndex, &day, &month, &hour, &minute);
                                    logger.logInfo("Last value of day %02d-%02d-%04d", day, month, mmt.year);
                                    // process the day. Indicate the database is open and should be left open
                                    processDay(day, month, mmt.year, true);
                                }
                            }

                        }
                    }
                    else
                    {
                        logger.logError("Insufficient data in block");
                        moreDataInBlock=false;
                    }
                    pointer+=84;
                    if (receiveLength-pointer<=0)
                    {
                        moreDataInBlock=false;
                    }
                }
                else
                {
                    logger.logError("Invalid measurement data received");
                    bailOut=true;
                    moreDataInBlock=false;
                }
            }
        }
        else
        {
            logger.logError("Error receiving measurements");
            bailOut=true;
        }
    }

    // close the database
    dataStore->closeDatabase();

    // disconnect from server
    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method requests the stored instant power max values
*
\******************************************************************************/
void Scheduler::requestStoredInstantMaxValues(bool onlyLastData)
{
    int             recordLength    =sizeof(char)+MAX_PULSE_COUNTERS*(2*sizeof(INT32)+sizeof(solarTime_t));

    // depending on onlyLastData whether ALL data or LAST data is requested
    if (onlyLastData)
    {
        sendReceiveBuffer[0]        =(char)COMMAND_SENDLASTSTOREDMAX;
        logger.logInfo("Requesting most recent instant max value");
    }
    else
    {
        sendReceiveBuffer[0]        =(char)COMMAND_SENDALLSTOREDMAXS;
        logger.logInfo("Requesting all stored instant max values");
    }

    // get the instance from the server connection and open the connection
    Connection*connection           =Connection::getInstance();
    connection->connectToServer();

    // open the database
    DataStore*dataStore             =DataStore::getInstance();
    dataStore->openDatabase();

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // now wait for an answer and process the answer data
    bool exit                       =false;
    while (!exit)
    {
        // wait for the data
        int receiveLength;
        connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

        // if any sensible data received, process it
        if (receiveLength>=1)
        {
            int     pointer         =0;
            bool    moreDataInBlock =true;

            while (moreDataInBlock)
            {
                if (sendReceiveBuffer[pointer]==COMMAND_ENDOFDATA)
                {
                    logger.logInfo("End of data");
                    moreDataInBlock=false;
                    exit=true;
                }
                else if (sendReceiveBuffer[pointer]==COMMAND_RECEIVEINSTANTMAX)
                {
                    if (receiveLength-pointer>=recordLength)
                    {
                        for (int i=0; i<MAX_PULSE_COUNTERS; i++)
                        {
                            maxs.timeDiff[i]    =*(int*)        (sendReceiveBuffer+pointer+1+i*(2*sizeof(INT32)+sizeof(solarTime_t)));;
                            maxs.power[i]       =*(int*)        (sendReceiveBuffer+pointer+5+i*(2*sizeof(INT32)+sizeof(solarTime_t)));;
                            maxs.time[i]        =*(solarTime_t*)(sendReceiveBuffer+pointer+9+i*(2*sizeof(INT32)+sizeof(solarTime_t)));
                        }

                        // If the power max value is valid (contains sensible data) store it in the database
                        bool error=dataStore->storeInstanteneousPowerMax(&maxs);

                        if (!error || 1)
                        {
                            // The values are actually stored, so let's print them
                            // An error occurs if there is an error while storing or when the data is already stored before.
                            printInstantMaxValues();
                        }
                        else
                        {
                            logger.logWarning("Error storing instant power max to database, it might have been stored earlier");
                        }
                        pointer+=recordLength;
                        if (receiveLength-pointer<=0)
                        {
                            moreDataInBlock=false;
                        }
                    }
                    else
                    {
                        logger.logError("Insufficient data in block");
                        moreDataInBlock=false;
                    }
                }
                else
                {
                    logger.logError("Invalid response receiveed");
                    exit=true;
                    moreDataInBlock=false;
                }
            }
        }
        else
        {
            logger.logError("Error receiving instant max values: no data in response");
            exit=true;
        }
    }

    // close the database
    dataStore->closeDatabase();

    // disconnect from server
    connection->disconnectFromServer();
}


/******************************************************************************\
*
* This method prints the instant max values
*
\******************************************************************************/
void Scheduler::printInstantMaxValues()
{
    for  (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        logger.logInfo("Counter %d: Time %02d-%02d-%04d %02d:%02d:%02d.%02d, power max: diff time %d, %d Watt, %d Watt",
                        i+1,
                        maxs.time[i].day, maxs.time[i].month, maxs.time[i].year+2000,
                        maxs.time[i].hour, maxs.time[i].minute, maxs.time[i].second, maxs.time[i].centisecond,
                        maxs.timeDiff[i],
                        3600*100/maxs.timeDiff[i]*WATTHOUR_PER_KILOWATTHOUR/configuration->getPulsesPerKwh(i),
                        maxs.power[i]);
    }
}

/******************************************************************************\
*
* This method converts the 5 minute statistics into a day record to be stored
* in the day value database
*
\******************************************************************************/
void Scheduler::convertStatistics(int timeIndex)
{
    int                 day;
    int                 month;
    int                 hour;
    int                 minute;

    // create the date: day month year
    Clock::calculateTime(timeIndex, &day, &month, &hour, &minute);

    dayRecord.day  =day;
    dayRecord.month=month;
    dayRecord.year =statistics.year;

    for (int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
    {
        // if at least 90% of the measurements are available, store the measurement in the day value database
        if (statistics.numberOfRecordsInSum[counterNo]>INTERVALS_PER_DAY*9/10)
        {
            // calculate the energy in Wh
            // E [Wh] = average P [dW]  * HOURS_PER_DAY / DECIWATT_PER_WATT
            dayRecord.energy[counterNo]         =(double)statistics.pulsePowerSum[counterNo]/statistics.numberOfRecordsInSum[counterNo]*
                                                 HOURS_PER_DAY/DECIWATT_PER_WATT;

            // calculate the max power in W. The index is relative to the start of day
            dayRecord.maxPower[counterNo]       =(double)statistics.maxPower[counterNo]/DECIWATT_PER_WATT;
            dayRecord.maxPowerIndex[counterNo]  =statistics.maxPowerIndex[counterNo]%INTERVALS_PER_DAY;

            // Calculate the number of minutes there was output of power
            dayRecord.minutesActive[counterNo]  =statistics.numberOfActivityRecords[counterNo]*MEASUREMENT_INTERVAL;
        }
        else
        {
            dayRecord.energy[counterNo]         =INVALID_ENERGY;
            dayRecord.maxPower[counterNo]       =INVALID_MEASUREMENT;
            dayRecord.maxPowerIndex[counterNo]  =INVALID_MEASUREMENT;
            dayRecord.minutesActive[counterNo]  =INVALID_MEASUREMENT;
        }
    }
}


/******************************************************************************\
*
* This method prints a fancy overview of the statistics that have been stored
*
\******************************************************************************/

void Scheduler::printStatistics()
{
    for (int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
    {
        logger.logInfo("Counter %d: %04d-%02d-%02d: energy %6.1f Wh, maxPower %5d W, active %4d min",
                counterNo+1,
                dayRecord.year, dayRecord.month, dayRecord.day,
                dayRecord.energy[counterNo], (int)dayRecord.maxPower[counterNo],
                dayRecord.minutesActive[counterNo]
                );
    }
}

/******************************************************************************\
*
* This method processes the measurments on a daily bases. It stores the
* day values. The date is the date of the day to process
*
\******************************************************************************/

void Scheduler::processDay(int day, int month, int year, bool databaseIsOpen)
{
    logger.logInfo("Calculating single day statistics");

    DataStore* dataStore=DataStore::getInstance();

    // If the database is not open, open it
    if (!databaseIsOpen)
    {
        // open the database
        dataStore->openDatabase();
    }

    // Get the day
    int timeIndex=Clock::calculateYearTimeIndex(day, month);

    dataStore->sumFiveMinuteValues(timeIndex, year, INTERVALS_PER_DAY, &statistics);

    if (!statistics.error)
    {
        convertStatistics(timeIndex);

        bool error=dataStore->storeDayValue(&dayRecord);

        if (!error)
        {
            // The values are actually stored, so let's print them
            // An error occurs if there is an error while storing or when the data is already stored before.
            printStatistics();
        }
        else
        {
            logger.logDebug("Nothing to be done: statistics probably already stored");
        }
    }
    else
    {
        logger.logError("Statistics error: no data found for %04d-%02d-%02d", year, month, day);
    }

    // If we opened the database (i.e. it was not on entering the function), close it
    if (!databaseIsOpen)
    {
        // close the database
        dataStore->closeDatabase();
    }

    // get the last instantaneous power maximum value
//    requestStoredInstantMaxValues(true);
}

/******************************************************************************\
*
* This method processes the measurments on a daily bases. It stores the
* day values. It processes all days of this year up to now
*
\******************************************************************************/

void Scheduler::processAllDays(int year)
{
    logger.logInfo("Calculating day statistics for this year %d", year);

    daytime=Clock::getTime();
    int day;
    int month;

    // define the end of the year
    if (year==daytime->tm_year+1900)
    {
        day     =daytime->tm_mday;         // day ranges from 1-31
        month   =daytime->tm_mon+1;        // month ranges from 1-12, whereas tm_mon ranges from 0-11
    }
    else if (year>daytime->tm_year+1900)
    {
        day     =31;
        month   =12;
    }
    else
    {
        day     =31;
        month   =12;
    }

    if (year<=daytime->tm_year+1900)
    {
        // open the database
        DataStore* dataStore=DataStore::getInstance();
        dataStore->openDatabase();

        // Time index for today
        int currentTimeIndex=Clock::calculateYearTimeIndex(day, month);

        for (int timeIndexCounter=0;timeIndexCounter<=currentTimeIndex; timeIndexCounter+=INTERVALS_PER_DAY)
        {
            dataStore->sumFiveMinuteValues(timeIndexCounter, year, INTERVALS_PER_DAY, &statistics);

            if (!statistics.error)
            {
                convertStatistics(timeIndexCounter);

                bool error=dataStore->storeDayValue(&dayRecord);

                if (!error)
                {
                    printStatistics();
                }
                else
                {
                    logger.logDebug("Nothing to be done: statistics probably already stored");
                }
            }
            else
            {
                logger.logError("Statistics error: no data found for %04d / %d", year, timeIndexCounter);
            }
        }
        // close the database
        dataStore->closeDatabase();
    }
    else
    {
        logger.logError("Cannot process the future");
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
