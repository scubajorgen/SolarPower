/**************************************************************************************************\
*
* Client.cpp
*
* The Client functions
*
\**************************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "Client.h"
#include "Connection.h"

Client* Client::theInstance;

/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
Client::Client()
{
    configuration=Configuration::getInstance();
}

/******************************************************************************\
*
* Destructor
*
\******************************************************************************/
Client::~Client()
{

}

/******************************************************************************\
*
* Get the one and only instance of this class (Singleton)
*
\******************************************************************************/

Client* Client::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new Client();
    }
    return theInstance;
}

/******************************************************************************\
*
* This method sends the time to the server to synchronise it
*
\******************************************************************************/
void Client::syncTime()
{
    logger.logInfo("Synchronising time");

    // Make sure we've got the current time
    daytime=Clock::getTime();

    // convert
    currentTime.year        =char(daytime->tm_year);        // struct tm indicates year since 1900
    currentTime.month       =daytime->tm_mon+1;             // struct tm counts months from 0
    currentTime.day         =daytime->tm_mday;
    currentTime.hour        =daytime->tm_hour;
    currentTime.minute      =daytime->tm_min;
    currentTime.second      =daytime->tm_sec;
    currentTime.centisecond =0;                             // yeah, skip the sub second stuff. 1 second accurate is accurate enough

    logger.logInfo("Sending: %02d:%02d:%02d %02d-%02d-%04d",
           currentTime.hour, currentTime.minute, currentTime.second,
           currentTime.day, currentTime.month, currentTime.year);

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
void Client::requestTime()
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
                        currentTime.day, currentTime.month, currentTime.year, currentTime.hour, currentTime.minute, currentTime.second);
    }
    else
    {
        logger.logError("Error requesting time");
    }

    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method sends the pulse calibration
*
\******************************************************************************/
void Client::calibratePulses()
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
* This method requests the instant power max
*
\******************************************************************************/
void Client::requestInstantMax()
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

    int counterRecordLength=2*sizeof(INT32)+sizeof(solarTime_t);
    if (((unsigned)receiveLength==(sizeof(char)+MAX_PULSE_COUNTERS*counterRecordLength)) && (sendReceiveBuffer[0]==COMMAND_RECEIVEINSTANTMAX))
    {
        logger.logInfo("Received instant max");

        for (int i=0; i<MAX_PULSE_COUNTERS; i++)
        {
            maxs.timeDiff[i]    =*(int*)        (sendReceiveBuffer+1                +i*(counterRecordLength));
            maxs.power[i]       =*(int*)        (sendReceiveBuffer+1+1*sizeof(INT32)+i*(counterRecordLength));
            maxs.time[i]        =*(solarTime_t*)(sendReceiveBuffer+1+2*sizeof(INT32)+i*(counterRecordLength));

            logger.logInfo("Counter %d: Time %02d-%02d-%04d %02d:%02d:%02d.%02d, power max: dT=%d cs, P=%d Watt",
                            i+1,
                            maxs.time[i].day, maxs.time[i].month, maxs.time[i].year,
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
void Client::resetInstantMax()
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
bool Client::requestMeasurements()
{
    bool error=false;

    logger.logInfo("Requesting stored measumements");

    sendReceiveBuffer[0]=(char)COMMAND_TRANSFERMEASUREMENTS;

    // get the instance from the server connection and open the connection
    Connection* connection          =Connection::getInstance();
    connection->connectToServer();

    // open the database and ping it
    DataStore* dataStore            =DataStore::getInstance();
    error=dataStore->openDatabase();

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // now wait for an answer and process the answer data
    int  recordSize                 =sizeof(measurement_t)+1;
    bool bailOut                    =false;
    while (!bailOut && !error)
    {
        // wait for the data
        int receiveLength           =0;
        connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

        // if any sensible data received, process it
        if (receiveLength>=1)
        {
            int pointer             =0;
            bool moreDataInBlock    =true;
            while (moreDataInBlock)
            {
                if (sendReceiveBuffer[pointer]==COMMAND_ENDOFDATA)
                {
                    logger.logInfo("End of data");
                    moreDataInBlock =false;
                    bailOut         =true;
                }
                else if (sendReceiveBuffer[pointer]==COMMAND_RECEIVEDATA)
                {
                    if (receiveLength-pointer>=recordSize)
                    {
                        // get the values from the buffer
                        measurement_t mmt;
                        mmt                         =*(measurement_t*)(sendReceiveBuffer+pointer+ 1);
                        int timeIndex               =mmt.timeIndex;
                        int currentTimeIndex;
                        int measurementYear;
                        int day;
                        int month;
                        int hour;
                        int minute;
                        // If the measurement is valid (contains sensible data) store it in the database
                        if (timeIndex>=0)
                        {
                            // Make sure we've got the current time
                            daytime         =Clock::getTime();
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
                                logger.logInfo("Measurement - Time %d/%d (%02d-%02d-%04d %02d:%02d:%02d.%02d) P1: %s,",                         
                                                mmt.year, mmt.timeIndex,
                                                mmt.datetime.day, mmt.datetime.month, mmt.datetime.year,
                                                mmt.datetime.hour, mmt.datetime.minute, mmt.datetime.second, mmt.datetime.centisecond,
                                                mmt.p1Time);
                                logger.logInfo("              P1 %d (%d/%d Watt, %d Wh), P2 %d (%d/%d Watt, %d Wh), P3 %d (%d/%d Watt, %d Wh), ",
                                                mmt.pulse[0],mmt.pulsePower[0]/10, mmt.pulseMaxPower[0],mmt.pulseMeter[0],
                                                mmt.pulse[1],mmt.pulsePower[1]/10, mmt.pulseMaxPower[1],mmt.pulseMeter[1],
                                                mmt.pulse[2],mmt.pulsePower[2]/10, mmt.pulseMaxPower[2],mmt.pulseMeter[2]);
                                logger.logInfo("              Eimp low %d Wh, Eimp normal %d Wh, Eexp low %d Wh, Eexp normal %d Wh",
                                                mmt.electricityImportLow, mmt.electricityImportNormal,
                                                mmt.electricityExportLow, mmt.electricityExportNormal);
                                logger.logInfo("              Gross power %d W, net power %d W, Tariff %d",
                                                mmt.grossPower/10, mmt.netPower/10, mmt.tariff);
                                logger.logInfo("              Power failues all: %d, long %d, sags: L1 %d L2 %d L3 %d swells L1 %d L2 %d L3 %d",
                                                mmt.powerFailures, mmt.powerFailuresLong, 
                                                mmt.sagsL1, mmt.sagsL2, mmt.sagsL3,
                                                mmt.swellsL1, mmt.swellsL2, mmt.swellsL3);
                                logger.logInfo("              Voltage          L1 %6d L2 %6d L3 %6d mV Current L1 %6d L2 %6d L3 %6d A",
                                                    mmt.voltageL1, mmt.voltageL2, mmt.voltageL3,
                                                    mmt.currentL1, mmt.currentL2, mmt.currentL3);
                                logger.logInfo("              Act Power Import L1 %6d L2 %6d L3 %6d W  Export  L1 %6d L2 %6d L3 %6d W",
                                                    mmt.activeImportPowerL1, mmt.activeImportPowerL2, mmt.activeImportPowerL3,
                                                    mmt.activeExportPowerL1, mmt.activeExportPowerL2, mmt.activeExportPowerL3);
                                logger.logInfo("              Gas %d l (%s)", mmt.gasImport, mmt.gasTime);

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
                    pointer+=recordSize;
                    if (receiveLength-pointer<=0)
                    {
                        moreDataInBlock=false;
                    }
                }
                else
                {
                    logger.logError("Invalid response received: %d",sendReceiveBuffer[pointer]);
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
    return error;
}

/******************************************************************************\
*
* This method requests the stored instant power max values
*
\******************************************************************************/
bool Client::requestStoredInstantMaxValues()
{
    bool error                      =false;
    logger.logInfo("Requesting stored daily maximum values");
    int             recordLength    =sizeof(char)+MAX_PULSE_COUNTERS*(2*sizeof(INT32)+sizeof(solarTime_t));

    sendReceiveBuffer[0]            =(char)COMMAND_TRANSFERPOWERMAXS;

    // get the instance from the server connection and open the connection
    Connection*connection           =Connection::getInstance();
    connection->connectToServer();

    // open the database and ping it
    DataStore*dataStore             =DataStore::getInstance();
    error                           =dataStore->openDatabase();

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // now wait for an answer and process the answer data
    bool exit                       =false;
    while (!exit && !error)
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

                        if (!error)
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
                    logger.logError("Invalid response received");
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
    return error;
}

/******************************************************************************\
*
* This method requests the info about the storage on the SolarServer
*
\******************************************************************************/
void Client::acknowledgeMeasurements()
{
    logger.logInfo("Acknowledge measurement reception");
    int             recordLength    =sizeof(char);

    // get the instance from the server connection and open the connection
    Connection*connection           =Connection::getInstance();
    connection->connectToServer();

    sendReceiveBuffer[0]            =(char)COMMAND_ACKMEASUREMENTTRANSFER;

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // wait for the data
    int receiveLength;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if (receiveLength==recordLength)
    {
        if (sendReceiveBuffer[0]==COMMAND_ACK)
        {
            logger.logInfo("Reception of measurements acknowledged");
        }
        else
        {
            logger.logError("Error acknowledging measurement reception: wrong response received: %d", sendReceiveBuffer[0]);
        }
    }
    else
    {
        logger.logError("Error acknowledging measurement reception: wrong size: received %d, expected %d", receiveLength, recordLength);
    }
    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method requests the info about the storage on the SolarServer
*
\******************************************************************************/
void Client::acknowledgeInstantMaxValues()
{
    logger.logInfo("Acknowledge daily max value reception");
    int             recordLength    =sizeof(char);

    // get the instance from the server connection and open the connection
    Connection*connection           =Connection::getInstance();
    connection->connectToServer();

    sendReceiveBuffer[0]            =(char)COMMAND_ACKPOWERMAXTRANSFER;

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // wait for the data
    int receiveLength;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if (receiveLength==recordLength)
    {
        if (sendReceiveBuffer[0]==COMMAND_ACK)
        {
            logger.logInfo("Reception of daily max values acknowledged");
        }
        else
        {
            logger.logError("Error acknowledging daily max values reception: wrong response received: %d", sendReceiveBuffer[0]);
        }
    }
    else
    {
        logger.logError("Error acknowledging daily max values reception: wrong size: received %d, expected %d", receiveLength, recordLength);
    }
    connection->disconnectFromServer();
}


/******************************************************************************\
*
* This method requests the info about the storage on the SolarServer
*
\******************************************************************************/
void Client::requestStorageInfo()
{
    logger.logInfo("Requesting storage info");
    int             recordLength    =sizeof(char)+6*sizeof(INT32);

    // get the instance from the server connection and open the connection
    Connection*connection           =Connection::getInstance();
    connection->connectToServer();

    sendReceiveBuffer[0]            =(char)COMMAND_SENDSTORAGEINFO;

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // wait for the data
    int receiveLength;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if (receiveLength==recordLength)
    {
        if (sendReceiveBuffer[0]==COMMAND_RECEIVESTORAGEINFO)
        {
            INT32 measurementSize   =*(INT32*)(sendReceiveBuffer+ 1);
            INT32 measurementUsed   =*(INT32*)(sendReceiveBuffer+ 5);
            INT32 dailyMaxSize      =*(INT32*)(sendReceiveBuffer+ 9);
            INT32 dailyMaxUsed      =*(INT32*)(sendReceiveBuffer+13);
            INT32 messageQueueSize  =*(INT32*)(sendReceiveBuffer+17);
            INT32 messageQueueUsed  =*(INT32*)(sendReceiveBuffer+21);
            logger.logReport("Storage usage: measurements %d/%d, daily max %d/%d, message queue %d/%d",
                                measurementUsed, measurementSize, dailyMaxUsed, dailyMaxSize, messageQueueUsed, messageQueueSize);
        }
        else
        {
            logger.logError("Error requesting storage info: wrong response received: %d", sendReceiveBuffer[0]);
        }
    }
    else
    {
        logger.logError("Error requesting storage info: wrong size: received %d, expected %d", receiveLength, recordLength);
    }
    connection->disconnectFromServer();
}

/******************************************************************************\
*
* This method requests the info about the version of the SolarServer
*
\******************************************************************************/
void Client::requestVersion()
{
    logger.logInfo("Requesting version info");
    int             recordLength    =sizeof(char)+10;

    // get the instance from the server connection and open the connection
    Connection*connection           =Connection::getInstance();
    connection->connectToServer();

    sendReceiveBuffer[0]            =(char)COMMAND_SENDVERSIONINFO;

    // send the request to the server
    connection->sendData(sendReceiveBuffer, 1);

    // wait for the data
    int receiveLength;
    connection->waitForData(sendReceiveBuffer, &receiveLength, SENDRECEIVEBUFFERSIZE);

    if (receiveLength==recordLength)
    {
        if (sendReceiveBuffer[0]==COMMAND_RECEIVEVERSIONINFO)
        {
            logger.logReport("SolarServer version: %s, SolarClient version %s", (char*)(sendReceiveBuffer+1), VERSION);
        }
        else
        {
            logger.logError("Error requesting version info: wrong response received: %d", sendReceiveBuffer[0]);
        }
    }
    else
    {
        logger.logError("Error requesting version info: wrong size: received %d, expected %d", receiveLength, recordLength);
    }
    connection->disconnectFromServer();
}


/******************************************************************************\
*
* This method prints the instant max values
*
\******************************************************************************/
void Client::printInstantMaxValues()
{
    for  (int i=0; i<MAX_PULSE_COUNTERS; i++)
    {
        logger.logInfo("Counter %d: Time %02d-%02d-%04d %02d:%02d:%02d.%02d, power max: diff time %d, %d Watt, %d Watt",
                        i+1,
                        maxs.time[i].day, maxs.time[i].month, maxs.time[i].year,
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
void Client::convertStatistics(int timeIndex)
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
void Client::printStatistics()
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
void Client::processDay(int day, int month, int year, bool databaseIsOpen)
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
}

/******************************************************************************\
*
* This method processes the measurments on a daily bases. It stores the
* day values. It processes all days of this year up to now
*
\******************************************************************************/

void Client::processAllDays(int year)
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

