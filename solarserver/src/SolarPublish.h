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

#include "Log.h"
#include "Clock.h" 
 
#define QUEUEDEPTH         10
#define MAXMESSAGELENGTH   100
 
#define MAX_READINGS        10 
 
/******************************************************************************\
*
* Class for real-time publishing of the solar power value.
*
\******************************************************************************/
 
 
class SolarPublish
{
private:
    Log                             logger {"solarpub"};
    char                            inBuffer[MAXMESSAGELENGTH];                            
    char                            outBuffer[MAXMESSAGELENGTH];                            

    pthread_t                       threadId; 						// we use a thread for the scheduled task


    char                            localMessageQueue[QUEUEDEPTH][MAXMESSAGELENGTH];
    int                             startOfQueue;
    int                             endOfQueue;              

protected:
    static SolarPublish*            theInstance;


    // Mutex to guard mutual data
    pthread_mutex_t                 mutex;   						// and a mutex
    
    // flag to indicate the thread to stop
    bool                            closeTask;
 
    // Flag indicating the thread is running.   
    bool                            taskRunning;

                                    SolarPublish        ();     
    void                            pushQueue           (char* item);    
    char*                           popQueue            ();    
    
    void                            startThread         (void *(*threadFunction) (void *));
 
public:
    static SolarPublish*            getInstance         ();
    
    void                            die                 (const char *file, int line, const char *message);


    virtual void                    postMessage         (solarTime_t time, int reading, double value);


    void                            testPublish         ();
    
    virtual void                    start               ();
    virtual void                    stop                ();

    virtual                         ~SolarPublish       (); 
 
};
 
#endif