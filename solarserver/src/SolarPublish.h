/******************************************************************************\
*
* SolarPublish.h
* Takes care of publishing real-time values
*
\******************************************************************************/
#ifndef SOLARPUBLISH_H

#define SOLARPUBLISH_H
 
#include <stdio.h>
#include <stdlib.h> 

#include "common.h"
#include "Log.h"
#include "Clock.h" 

// Defines how much time can be stored in the message queue
#define BUFFERMINUTES       60
#define MESSAGETYPES        5
#define QUEUEDEPTH          (MESSAGETYPES*BUFFERMINUTES*SECONDS_PER_MINUTE/PUBLISHINTERVAL)
 
#define MAX_READINGS        10 
#define INVALID_READING     -1


typedef struct
{
    int             reading;
    solarTime_t     time;
    double          value;
} message_t;

/******************************************************************************\
*
* Class for real-time publishing of the solar power value.
*
\******************************************************************************/
class SolarPublish
{
private:
    Log                             logger {"solarpub"};
    pthread_t                       threadId;                                           // we use a thread for the scheduled task

protected:
    message_t                       messageQueue[QUEUEDEPTH];       // FIFO cache
    int                             messageQueueHead;               // First empty place
    int                             messageQueueTail;               // Oldest element in Queue

    static SolarPublish*            theInstance;

    // Mutex to guard mutual data
    pthread_mutex_t                 mutex;                                              // and a mutex
    
    // flag to indicate the thread to stop
    bool                            closeTask;
 
    // Flag indicating the thread is running.   
    bool                            taskRunning;

                                    SolarPublish        ();     
    void                            pushQueue           (message_t* message);    
    message_t*                      popQueue            ();    
    
    void                            startThread         (void *(*threadFunction) (void *));
 
public:
    static SolarPublish*            getInstance         ();
    
    void                            die                 (const char *file, int line, const char *message);
    int                             getQueueSize        ();
    void                            logStatus           ();


    virtual void                    postMessage         (solarTime_t time, int reading, double value);


    void                            testPublish         ();
    
    virtual void                    start               ();
    virtual void                    stop                ();

    virtual                         ~SolarPublish       (); 
 
};
 
#endif