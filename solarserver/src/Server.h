/******************************************************************************\
*
* Server.cpp
* The Server function allowing connections from SolarClient. Includes
* processing of all commands
*
\******************************************************************************/
#if !defined(SERVER_H)
#define SERVER_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"

#include "Log.h"
#include "Clock.h"
#include "SmartMeter.h"
#include "Scheduler.h"
#include "MeasurementStorage.h"
#include "Configuration.h"

#include "pthread.h"

/******************************************************************************
* Defines
******************************************************************************/
#define SERVER_PORT                   8002
#define RECVQUEUE                      100  /*character recv queue size*/
#define SENDQUEUE                      100
#define TASK_STACKSIZE                1024  /*Words*/

typedef enum
{
    COMMAND_NOCOMMAND               =0,
    COMMAND_ACK                     =1,
    COMMAND_ADJUSTTIME              =2,
    COMMAND_SENDALLDATA             =3, // obsolete
    COMMAND_SENDLASTDATA            =4, // obsolete
    COMMAND_RECEIVEDATA             =5,
    COMMAND_ENDOFDATA               =6,
    COMMAND_SENDINSTANTMAX          =7,
    COMMAND_RECEIVEINSTANTMAX       =8,
    COMMAND_RESETINSTANTMAX         =9,
    COMMAND_SENDALLSTOREDMAXS       =10, // obsolete
    COMMAND_SENDLASTSTOREDMAX       =11, // obsolete
    COMMAND_SENDLASTTENDATA         =12, // obsolete
    COMMAND_GETTIME                 =13,
    COMMAND_RECEIVETIME             =14,
    COMMAND_CALIBRATEPULSE          =15,
    COMMAND_GETBUFFERINFO           =16,
    COMMAND_RECEIVEBUFFERINFO       =17,
    COMMAND_TRANSFERMEASUREMENTS    =18,
    COMMAND_TRANSFERPOWERMAXS       =19,
    COMMAND_ACKMEASUREMENTTRANSFER  =20,
    COMMAND_ACKPOWERMAXTRANSFER     =21,
    COMMAND_SENDSTORAGEINFO         =22,
    COMMAND_RECEIVESTORAGEINFO      =23,
    COMMAND_SENDVERSIONINFO         =24,
    COMMAND_RECEIVEVERSIONINFO      =25
} command_t;


class Server
{
    Log                 logger {"server"};
    static Server*      theInstance;

    pthread_t           threadId;                       // we use a thread for the server task
    pthread_mutex_t     mutex;                          // and a mutex

    int                 serverSocket;               //global socket descriptor
    struct sockaddr_in  addr;

    Scheduler*          scheduler;
    SmartMeter*         smartMeter;
    MeterReading_t      reading;

    MeasurementStorage* measurementStorage;
    Measurement_t       measurement;
    MaxPower_t          maxPower;

    int                 serverPort;


    bool                taskRunning;
    bool                closeTask;

    char                sendBuffer[SENDQUEUE];

    INT32               maxPowerTimeDiff;
    solarTime_t         maxPowerTime;

    solarTime_t         solarTime;

    // receiveTask() helper variables
    char                chrbuf[RECVQUEUE];

private:
    Server();
    void                disableTcpNagleAlgorithm    (int sd);
    void                processCommand              (int socket, char* data, int blockSize);



    // Helpers
    void                sendMeasurement             (int socket, Measurement_t measurement);
    void                sendInstantMaxPower         (int socket, MaxPower_t    maxPower);

    // Responders
    void                calibratePulseCounters      (int socket, char* data, int dataLength);
    void                synchroniseTime             (int socket, char* timeData, int dataLength);
    void                sendTime                    (int socket);
    void                sendInstantMax              (int socket);
    void                resetInstantMax             (int socket);
    void                transferMeasurements        (int socket);
    void                transferPowerMaxs           (int socket);
    void                ackMeasurementTransfer      (int socket);
    void                ackPowerMaxsTransfer        (int socket);
    void                sendStorageSizes            (int socket);
    void                sendVersion                 (int socket);

public:
    static Server*      getInstance                 ();
    void                startServer                 ();
    void                stopServer                  ();
    friend void*        receiveTask                 (void* param);
};
#endif