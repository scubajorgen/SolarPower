/******************************************************************************\
*
* Server.cpp
* The Server function allowing connections from SolarClient. Includes
* processing of all commands
*
\******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include<arpa/inet.h> //inet_addr

#include "Server.h"
#include "Clock.h"
#include "Scheduler.h"
#include "Configuration.h"
#include "SolarPublish.h"

#define MAX(a,b) (((a)>(b))?(a):(b))

/******************************************************************************\
* Variables
\******************************************************************************/

Server* Server::theInstance=NULL;

/******************************************************************************\
*
* Receive task
* Usually a thread is created for any incoming connection; since we expect
* just one connection, we handle the connection in this one single thread
*
\******************************************************************************/
void* receiveTask(void* param)
{
    int                 retval;
    int                 acceptSocket;
    bool                exitLoop;
    char                clientIp[17];
    struct sockaddr_in  clAddr;                     //holds clients ip and port number


    Server* server=Server::getInstance();

    server->logger.logInfo("Server task started");

    pthread_mutex_lock(&server->mutex);
    server->taskRunning=true;
    bool localCloseTask=server->closeTask;
    pthread_mutex_unlock(&server->mutex);

    while(!localCloseTask)
    {

        // Wait for a connection
        acceptSocket=-1;
        while ((acceptSocket<0) && !localCloseTask)
        {
            //listen for incomming client connections
            listen(server->serverSocket, 3);

            //accept
            int length      =sizeof(struct sockaddr_in);
            acceptSocket    = accept(server->serverSocket, (struct sockaddr*)&clAddr, (socklen_t*)&length);
            pthread_mutex_lock(&server->mutex);
            localCloseTask=server->closeTask;
            pthread_mutex_unlock(&server->mutex);
        }

        // we now have a valid client socket or the we should close the task
        if (acceptSocket >= 0)
        {
            server->disableTcpNagleAlgorithm(acceptSocket);
            inet_ntop(AF_INET,&clAddr.sin_addr, clientIp, sizeof(clientIp));
            server->logger.logInfo("Connection to server from %s", clientIp);

            exitLoop=false;
            while(!exitLoop && !localCloseTask)
            {
                //recv. After connection, within 1 second a message is expected

                retval = recv( acceptSocket,
                                server->chrbuf,
                                RECVQUEUE,
                                0);


                if(retval <= 0)
                {
                    exitLoop=true;
                }
                else
                {
                    server->processCommand(acceptSocket, server->chrbuf, retval);
                }
                pthread_mutex_lock(&server->mutex);
                localCloseTask=server->closeTask;
                pthread_mutex_unlock(&server->mutex);
            }
            retval = close( acceptSocket );
            server->logger.logInfo("Connection lost");
        }

        pthread_mutex_lock(&server->mutex);
        localCloseTask=server->closeTask;
        pthread_mutex_unlock(&server->mutex);

        // sleep a while to give control to other processes...
        usleep(100000);
    }

    pthread_mutex_lock(&server->mutex);
    server->taskRunning=false;
    pthread_mutex_unlock(&server->mutex);

    // end of excercise
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
Server::Server()
{
    taskRunning =false;
    closeTask   =false;
    scheduler=Scheduler::getInstance();
    smartMeter=SmartMeter::getInstance();
    measurementStorage=MeasurementStorage::getInstance();
}


/******************************************************************************
* Disable nagle
******************************************************************************/
void Server::disableTcpNagleAlgorithm(int sd)
{
    /* Disable the Nagle (TCP No Delay) algorithm */
    int flag    = 1;
    int ret     = setsockopt( sd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
    if (ret == -1)
    {
        logger.logError("Unable to disable nagle algorithm");
        exit( EXIT_FAILURE );
    }
}

/******************************************************************************\
*
* This method processes the data from the clients
*
\******************************************************************************/
void Server::processCommand(int socket, char* data, int blockSize)
{
    command_t   command;

    if (blockSize>=1)
    {
        command=(command_t)data[0];
        switch (command)
        {
        case COMMAND_SENDINSTANTMAX:
            logger.logInfo("COMMAND: send instant power maximum");
            sendInstantMax(socket);
            break;
        case COMMAND_RESETINSTANTMAX:
            logger.logInfo("COMMAND: reset instant power maximum");
            resetInstantMax(socket);
            break;
        case COMMAND_GETTIME:
            logger.logInfo("COMMAND: send time");
            sendTime(socket);
            break;
        case COMMAND_ADJUSTTIME:
            logger.logInfo("COMMAND: adjust time");
            synchroniseTime(socket, data+1, blockSize-1);
            break;
        case COMMAND_CALIBRATEPULSE:
            logger.logInfo("COMMAND: calibrate pulse counters");
            calibratePulseCounters(socket, data+1, blockSize-1);
            break;
        case COMMAND_TRANSFERMEASUREMENTS:
            logger.logInfo("COMMAND: Transfer available measurements");
            transferMeasurements(socket);
            break;
        case COMMAND_TRANSFERPOWERMAXS:
            logger.logInfo("COMMAND: Transfer available max power values");
            transferPowerMaxs(socket);
            break;
        case COMMAND_ACKMEASUREMENTTRANSFER:
            logger.logInfo("COMMAND: Acknowledge measurement transfer");
            ackMeasurementTransfer(socket);
            break;
        case COMMAND_ACKPOWERMAXTRANSFER:
            logger.logInfo("COMMAND: Acknowledge daily maximum power transfer");
            ackPowerMaxsTransfer(socket);
            break;
        case COMMAND_SENDSTORAGEINFO:
            logger.logInfo("COMMAND: send storage size information");
            sendStorageSizes(socket);
            break;
        case COMMAND_SENDVERSIONINFO:
            logger.logInfo("COMMAND: send version information (%s)", VERSION);
            sendVersion(socket);
            break;

        case COMMAND_SENDALLSTOREDMAXS:
            logger.logError("COMMAND: send all stored instant power maximums - OBSOLETE, no function");
            break;
        case COMMAND_SENDLASTSTOREDMAX:
            logger.logError("COMMAND: send last stored instant power maximum - OBSOLETE, no function");
            break;
        case COMMAND_SENDALLDATA:
            logger.logError("COMMAND: send all measurement data - OBSOLETE, no function");
            break;
        case COMMAND_SENDLASTDATA:
            logger.logError("COMMAND: send last measurement data - OBSOLETE, no function");
            break;
        case COMMAND_SENDLASTTENDATA:
            logger.logError("COMMAND: send last 10 measurement data - OBSOLETE, no function");
            break;
        default:
            logger.logError("COMMAND: unknown command %d", command);
            break;

        }
    }
}

/******************************************************************************\
*
* This method calibrates the pulse counters, i.e. sets pulses per kWh
*
\******************************************************************************/
void Server::calibratePulseCounters (int socket, char* data, int dataLength)
{
    Configuration* configuration;

    configuration=Configuration::getInstance();
    if (dataLength==3*sizeof(int))
    {
        configuration->setPulsesPerKwh(0, *(INT32*)(data  ));
        configuration->setPulsesPerKwh(1, *(INT32*)(data+4));
        configuration->setPulsesPerKwh(2, *(INT32*)(data+8));

        sendBuffer[0]=COMMAND_ACK;
        send(socket, sendBuffer, 1, 0);
    }
}

/******************************************************************************\
*
* This method synchronises the time based on the received data
*
\******************************************************************************/
void Server::synchroniseTime(int socket, char* timeData, int dataLength)
{
    solarTime_t*    newTime;
    Clock*          clock;

    if (dataLength==sizeof(solarTime_t))
    {
        newTime=(solarTime_t*)timeData;
        clock=Clock::getInstance();
        clock->setTime(newTime);

        sendBuffer[0]=COMMAND_ACK;
        send(socket, sendBuffer, 1, 0);
    }
}

/******************************************************************************\
*
* This method synchronises the time based on the received data
*
\******************************************************************************/
void Server::sendTime (int socket)
{
    Clock*          clock;
    solarTime_t*    timePtr;

    clock           =Clock::getInstance();
    clock->getTime(&solarTime);

    timePtr         =(solarTime_t*)(sendBuffer+1);
    *timePtr        =solarTime;


    sendBuffer[0]   =COMMAND_RECEIVETIME;
    send(socket, sendBuffer, 1+sizeof(solarTime_t), 0);
}

/******************************************************************************\
*
* This method writes a measurement into the send buffer
*
\******************************************************************************/
void Server::sendMeasurement(int socket, Measurement_t measurement)
{
    // send each measurment as a separate TCP block

    sendBuffer[0]                   =COMMAND_RECEIVEDATA;
    int  recordSize                 =sizeof(char)+4*sizeof(INT16)+17*sizeof(INT32)+sizeof(solarTime_t);
    *(INT32*)(sendBuffer+ 1)        =measurement.timeIndex;
    *(INT16*)(sendBuffer+ 5)        =measurement.year;
    *(INT16*)(sendBuffer+ 7)        =measurement.pulse[0];
    *(INT16*)(sendBuffer+ 9)        =measurement.pulse[1];
    *(INT16*)(sendBuffer+11)        =measurement.pulse[2];
    *(INT32*)(sendBuffer+13)        =measurement.pulsePower[0];
    *(INT32*)(sendBuffer+17)        =measurement.pulsePower[1];
    *(INT32*)(sendBuffer+21)        =measurement.pulsePower[2];
    *(INT32*)(sendBuffer+25)        =measurement.pulseMaxPower[0];
    *(INT32*)(sendBuffer+29)        =measurement.pulseMaxPower[1];
    *(INT32*)(sendBuffer+33)        =measurement.pulseMaxPower[2];
    *(INT32*)(sendBuffer+37)        =measurement.pulseMeter[0];
    *(INT32*)(sendBuffer+41)        =measurement.pulseMeter[1];
    *(INT32*)(sendBuffer+45)        =measurement.pulseMeter[2];

    *(INT32*)(sendBuffer+49)        =measurement.electricityImportLow;
    *(INT32*)(sendBuffer+53)        =measurement.electricityImportNormal;
    *(INT32*)(sendBuffer+57)        =measurement.electricityExportLow;
    *(INT32*)(sendBuffer+61)        =measurement.electricityExportNormal;
    *(INT32*)(sendBuffer+65)        =measurement.gasImport;

    *(INT32*)(sendBuffer+69)        =measurement.grossPower;
    *(INT32*)(sendBuffer+73)        =measurement.netPower;

    *(solarTime_t*)(sendBuffer+77)  =measurement.datetime;

    send(socket, sendBuffer, recordSize, 0);
}


/******************************************************************************\
*
* This method writes a measurement into the send buffer
*
\******************************************************************************/
void Server::sendInstantMaxPower(int socket, MaxPower_t maxPower)
{
    // send each measurment as a separate TCP block
    // C T1 T1 T1 T1 P1 P1 P1 P1 I1 I1 I1 I1 I1 I1 I1 
    //   T2 T2 T2 T2 P2 P2 P2 P2 I2 I2 I2 I2 I2 I2 I2 
    //   T3 T3 T3 T3 P3 P3 P3 P3 I3 I3 I3 I3 I3 I3 I3
    // C - Command byte (COMMAND_RECEIVEINSTANTMAX)
    // T - Power max time difference between two adjacent pulses
    // P - Power value in Wh
    // I - Time stamp (solarTime_t struct)
    sendBuffer[0]=COMMAND_RECEIVEINSTANTMAX;

    int recordSize=2*sizeof(INT32)+sizeof(solarTime_t);
    *(INT32*)      (sendBuffer+recordSize*0+1)      =maxPower.maxPowerTimeDiff[0];
    *(INT32*)      (sendBuffer+recordSize*0+5)      =maxPower.maxPower[0];
    *(solarTime_t*)(sendBuffer+recordSize*0+9)      =maxPower.maxPowerTime[0];
    *(INT32*)      (sendBuffer+recordSize*1+1)      =maxPower.maxPowerTimeDiff[1];
    *(INT32*)      (sendBuffer+recordSize*1+5)      =maxPower.maxPower[1];
    *(solarTime_t*)(sendBuffer+recordSize*1+9)      =maxPower.maxPowerTime[1];
    *(INT32*)      (sendBuffer+recordSize*2+1)      =maxPower.maxPowerTimeDiff[2];
    *(INT32*)      (sendBuffer+recordSize*2+5)      =maxPower.maxPower[2];
    *(solarTime_t*)(sendBuffer+recordSize*2+9)      =maxPower.maxPowerTime[2];

    send(socket, sendBuffer, (sizeof(char)+MAX_PULSE_COUNTERS*(recordSize)), 0);
}

/******************************************************************************\
*
* This method sends the current instant power maximum values
*
\******************************************************************************/
void Server::sendInstantMax(int socket)
{
    scheduler->getCurrentPowerMax(&maxPower);
    sendInstantMaxPower(socket, maxPower);
}


/******************************************************************************\
*
* This method resets the instant power maximum value
*
\******************************************************************************/
void Server::resetInstantMax(int socket)
{
    // reset the instantaneous power maximum values
    scheduler->resetCurrentPowerMax();

    // send an acknowledge
    sendBuffer[0]=COMMAND_ACK;
    send(socket, sendBuffer, 1, 0);


}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance (Singleton)
*
\******************************************************************************/
Server* Server::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new Server();
    }

    return theInstance;
}


/******************************************************************************\
*
* Start de server
*
\******************************************************************************/

void Server::startServer(void)
{
    int                 retval;
    int                 err;
    pthread_mutexattr_t attr;
    Configuration*      config;


    config          =Configuration::getInstance();
    serverPort      =config->getServerPort();


    // create the socket

    serverSocket    = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        logger.logError("Could not create socket");
    }
    memset(&addr, '0', sizeof(addr));
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = INADDR_ANY;
    addr.sin_port           = htons(serverPort);

    logger.logInfo("Starting server on port %d", serverPort);

    retval=bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr));

    if(retval < 0)
    {
        logger.logError("TCPserver: Socket bind failed");
        close(serverSocket);
    }

   //************************************************************************
   //create a mutex for synchronized tcp send at stdout
   //************************************************************************
    // create the mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

    err=pthread_mutex_init(&mutex, &attr);

    if (err)
    {
        logger.logError("Unable to create mutex");
    }

    //**************************************************************************
    //create and start tcp stdio receiver task
    //**************************************************************************

    if (!err)
    {
        err=pthread_create(&threadId, NULL, receiveTask, (void *)this);
    }

    if(err!=0)
    {
        logger.logError("Creating receive task failed");
        close(serverSocket);
    }
}

/***************************************************\
*
* Stop server
*
\***************************************************/
void Server::stopServer(void)
{
    bool localTaskRunning;

    logger.logInfo("Stopping server task");

    //closing the main listening socket

    pthread_mutex_lock(&mutex);
    closeTask=true;
    localTaskRunning=taskRunning;
    pthread_mutex_unlock(&mutex);

    shutdown(serverSocket, SHUT_RDWR);

    while (localTaskRunning)
    {
        usleep(10000);
        pthread_mutex_lock(&mutex);
        localTaskRunning=taskRunning;
        pthread_mutex_unlock(&mutex);
    }

    logger.logInfo("Server stopped");
}


/******************************************************************************\
*
* This method removes all measurements from the queue and sends them
*
\******************************************************************************/
void Server::transferMeasurements(int socket)
{
    int  count  =0;
    bool error  =measurementStorage->resetMeasurementNext();
    while (!error)
    {
        error=measurementStorage->getNextMeasurement(&measurement);
        if (!error)
        {
            sendMeasurement(socket, measurement);

            // very, very dirty work around to limit the
            // packet size that is being sent (or received at the client side):
            // every 16 packets wait for 0.1 sec
            if (!(count&0x0f))
            {
                usleep(100000);
            }
            // end of very, very dirty work around
            count++;
        }
    }
    sendBuffer[0]=COMMAND_ENDOFDATA;
    send(socket, sendBuffer, 1, 0);

    logger.logInfo("Stored measurements sent: %d; stored records %d", count, measurementStorage->getNumberOfMeasurementRecords());
}


/******************************************************************************\
*
* This method removes all power max values from the queue and sends them
*
\******************************************************************************/
void Server::transferPowerMaxs(int socket)
{
    int     count=0;
    bool    error=measurementStorage->resetPowerMaxNext();
    while (!error)
    {
        error   =measurementStorage->getNextPowerMaxValue(&maxPower);
        // send each measurment as a separate TCP block
        // C T1 T1 T1 T1 I1 I1 I1 I1 I1 I1 I1 T2 T2 T2 T2 I2 I2 I2 I2 I2 I2 I2 T3 T3 T3 T3 I3 I3 I3 I3 I3 I3 I3
        // C - Command byte (COMMAND_RECEIVEINSTANTMAX)
        // T - Power max time difference between two adjacent pulses
        // I - Time stamp (solarTime_t struct)
        if (!error)
        {
            sendInstantMaxPower(socket, maxPower);

            // very, very dirty work around to limit the
            // packet size that is being sent (or received at the client side):
            // every 8 packets wait for 0.1 sec
            if (!(count&0x07))
            {
                usleep(100000);
            }
            // end of very, very dirty work around
            count++;
        }
    }
    // Send last packet
    sendBuffer[0]=COMMAND_ENDOFDATA;
    send(socket, sendBuffer, 1, 0);
    // very, very dirty work around to limit the
    // packet size that is being sent (or received at the client side):
    // every 8 packets wait for 0.1 sec
    if (!(count&0x07))
    {
        usleep(100000);
    }
    
    logger.logInfo("Stored power max values sent: %d; records in storage %d", count, measurementStorage->getNumberOfMaxPowerRecords());
}

/******************************************************************************\
*
* This method acknowledges the measurement transfer: remove measurements
* from storage
*
\******************************************************************************/
void Server::ackMeasurementTransfer(int socket)
{
    int recordsBefore   =measurementStorage->getNumberOfMeasurementRecords();
    measurementStorage->purgeRetrievedMeasurements();
    int recordsAfter    =measurementStorage->getNumberOfMeasurementRecords();
    logger.logInfo("Measurements purged; records before %d, after %d", recordsBefore, recordsAfter);

    // send an acknowledge
    sendBuffer[0]=COMMAND_ACK;
    send(socket, sendBuffer, 1, 0);
}

/******************************************************************************\
*
* This method acknowledges the power max  transfer: remove values
* from storage
*
\******************************************************************************/
void Server::ackPowerMaxsTransfer(int socket)
{
    int recordsBefore   =measurementStorage->getNumberOfMaxPowerRecords();
    measurementStorage->purgeRetrievedPowerMaxValues();
    int recordsAfter    =measurementStorage->getNumberOfMaxPowerRecords();
    logger.logInfo("Power maximums purged; records before %d, after %d", recordsBefore, recordsAfter);

    // send an acknowledge
    sendBuffer[0]=COMMAND_ACK;
    send(socket, sendBuffer, 1, 0);
}

/******************************************************************************\
*
* This method sends statistics about the measurement storage
*
\******************************************************************************/
void Server::sendStorageSizes(int socket)
{
    // send each measurment as a separate TCP block
    // C M M M M MU MU MU MU A A A A AU AU AU AU
    // C  - Command byte (COMMAND_RECEIVESTORAGEINFO)
    // T  - Power max time difference between two adjacent pulses
    // M  - Total storage capacity measurements
    // MU - Records in storage measurements
    // A  - Total storage capacity daily maximum values
    // AU - Records in storage daily maximum values

    sendBuffer[0]=COMMAND_RECEIVESTORAGEINFO;

    int recordSize=sizeof(char)+6*sizeof(INT32);
    *(INT32*)      (sendBuffer+recordSize*0+ 1)      =MEASUREMENTSTORAGESIZE;
    *(INT32*)      (sendBuffer+recordSize*0+ 5)      =measurementStorage->getNumberOfMeasurementRecords();
    *(INT32*)      (sendBuffer+recordSize*0+ 9)      =MAXPOWERSTORAGESIZE;
    *(INT32*)      (sendBuffer+recordSize*0+13)      =measurementStorage->getNumberOfMaxPowerRecords();
    *(INT32*)      (sendBuffer+recordSize*0+17)      =QUEUEDEPTH;
    *(INT32*)      (sendBuffer+recordSize*0+21)      =SolarPublish::getInstance()->getQueueSize();

    send(socket, sendBuffer, recordSize, 0);
}

/******************************************************************************\
*
* This method sends version info
*
\******************************************************************************/
void Server::sendVersion(int socket)
{
    char version[10];
    // send each measurment as a separate TCP block
    // C V V V V V V V V V V
    sendBuffer[0]=COMMAND_RECEIVEVERSIONINFO;

    int recordSize=sizeof(char)+10;
    sprintf(version, "%s", VERSION);
    strncpy((char*)(sendBuffer+1), version, 10);
    send(socket, sendBuffer, recordSize, 0);
}
