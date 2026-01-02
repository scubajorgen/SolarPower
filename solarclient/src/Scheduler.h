/**************************************************************************************************\
*
* Scheduler.cpp
*
* Exwecutes periodic functions
*
\**************************************************************************************************/
#include "pthread.h"

#include "common.h"
#include "Datastore.h"
#include "Configuration.h"
#include "Log.h"

#define SENDRECEIVEBUFFERSIZE   100000

typedef enum
{
    COMMAND_NOCOMMAND           =0,
    COMMAND_ACK                 =1,
    COMMAND_ADJUSTTIME          =2,
    COMMAND_SENDALLDATA         =3,
    COMMAND_SENDLASTDATA        =4,
    COMMAND_RECEIVEDATA         =5,
    COMMAND_ENDOFDATA           =6,
    COMMAND_SENDINSTANTMAX      =7,
    COMMAND_RECEIVEINSTANTMAX   =8,
    COMMAND_RESETINSTANTMAX     =9,
    COMMAND_SENDALLSTOREDMAXS   =10,
    COMMAND_SENDLASTSTOREDMAX   =11,
    COMMAND_SENDLASTTENDATA     =12,
    COMMAND_GETTIME             =13,
    COMMAND_RECEIVETIME         =14,
    COMMAND_CALIBRATEPULSE      =15,
    COMMAND_GETBUFFERINFO       =16,
    COMMAND_RECEIVEBUFFERINFO   =17   
} command_t;


typedef enum
{
    REQUESTDATA_LAST        =0,
    REQUESTDATA_LASTTEN     =1,
    REQUESTDATA_ALL         =2
} requestdata_t;

class Scheduler
{
private:
    Log                     logger {"scheduler"};
    static Scheduler*       theInstance;					// the one and only instance of this class
        
    pthread_t               threadId; 						// we use a thread for the scheduled task
    pthread_mutex_t         mutex;   						// and a mutex
    
    struct tm*              daytime;						// the current time
    
    // mutex guarded data
    bool                    taskRunning;					// flag indicating the thread is active
    bool                    exitTask;						// flag indicating the thread should stop
    solarTime_t             currentTime;					// 
    
    char                    sendReceiveBuffer[SENDRECEIVEBUFFERSIZE];		// Buffer used for sending and receiving serial data
    
    char                    debugString[256];
    DataStore*              dataStore;
    
    // end mutex guarded data

    instantMax_t            maxs;   

    Configuration*          configuration;

    fiveMinuteStats_t       statistics;
    dayRecord_t             dayRecord;  
    

    
        
                            Scheduler                       ();
    void                    start                           ();
    void                    stop                            ();
    
    void                    requestData                     (requestdata_t typeOfRequest);		// request measurements from the server
    void                    requestStoredInstantMaxValues   (bool onlyLastData);
    void                    printStatistics                 ();
    void                    printInstantMaxValues           ();
    void                    processDay                      (int day, int month, int year, bool databaseIsOpen);
    

    
    void                    convertStatistics               (int timeIndex);    
    
    // The thread function
    friend void*            schedulerTask                   (void* param);
    
    
    
    
                
public:

    static Scheduler*       getInstance         ();
                            ~Scheduler          ();           

    void                    requestInstantMax   ();
    void                    resetInstantMax     ();
    void                    syncTime            ();
    void                    requestTime         ();
    void                    calibratePulses     ();
    void                    processAllDays      (int year);
    
};


