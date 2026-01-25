/**************************************************************************************************\
*
* Client.h
*
* The Client functions
*
\**************************************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

#include "Configuration.h"
#include "Clock.h"
#include "Datastore.h"
#include "Log.h"


#define SENDRECEIVEBUFFERSIZE   100000

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


class Client
{
private:
    Log                     logger {"client"};
    static Client*          theInstance;                    // the one and only instance of this class

    Configuration*          configuration;

    struct tm*              daytime;                        // the current time
    solarTime_t             currentTime;                    // 
    instantMax_t            maxs;
    char                    sendReceiveBuffer[SENDRECEIVEBUFFERSIZE];        // Buffer used for sending and receiving serial data
    fiveMinuteStats_t       statistics;
    dayRecord_t             dayRecord;  


                            Client                          ();
    void                    printInstantMaxValues           ();
    void                    printStatistics                 ();
    void                    processDay                      (int day, int month, int year, bool databaseIsOpen);
    void                    convertStatistics               (int timeIndex);    

public:
                            ~Client                         ();
    static Client*          getInstance                     ();
    void                    syncTime                        ();
    void                    requestTime                     ();
    void                    requestInstantMax               ();
    void                    resetInstantMax                 ();
    void                    requestStorageInfo              ();
    void                    requestVersion                  ();

    void                    calibratePulses                 ();
    bool                    requestMeasurements             ();     // request measurements from the server
    void                    acknowledgeMeasurements         ();     // acknowledge proper receiption of measurements
    bool                    requestStoredInstantMaxValues   ();     // request stored daily maximum values
    void                    acknowledgeInstantMaxValues     ();     // acknowledge proper reception of daily max values
    void                    processAllDays                  (int year);
};


#endif