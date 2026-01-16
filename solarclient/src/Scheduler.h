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
#include "Client.h"
#include "Log.h"


class Scheduler
{
private:
    Log                     logger {"scheduler"};
    static Scheduler*       theInstance;                    // the one and only instance of this class
    Client*                 client;

    pthread_t               threadId;                         // we use a thread for the scheduled task
    pthread_mutex_t         mutex;                           // and a mutex
    
    
    // mutex guarded data
    bool                    taskRunning;                    // flag indicating the thread is active
    bool                    exitTask;                        // flag indicating the thread should stop
    // end mutex guarded data


    Configuration*          configuration;



                            Scheduler                       ();
    void                    start                           ();
    void                    stop                            ();
    void                    preScheduleIntialise            ();
    // The thread function
    friend void*            schedulerTask                   (void* param);
    
public:
    static Scheduler*       getInstance         ();
                            ~Scheduler          ();           
};


