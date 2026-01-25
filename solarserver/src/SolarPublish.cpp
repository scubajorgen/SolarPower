/******************************************************************************\
*
* SolarPublish.h
* Takes care of publishing real-time values, queue based
*
\******************************************************************************/
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "SolarPublish.h"

#include "pthread.h"

SolarPublish* SolarPublish::theInstance=NULL;

/******************************************************************************\
* Friend methods
\******************************************************************************/

/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/
SolarPublish::SolarPublish()
{
    pthread_mutexattr_t attr;

    // initialise the task
    taskRunning             =false;
    closeTask               =false;

    messageQueueHead        =0;
    messageQueueTail        =0;

    // create the mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

    int err=pthread_mutex_init(&mutex, &attr);
    if (err)
    {
        logger.logFatal("Unable to create mutex");
    }
}

/******************************************************************************\
*
* This method pushes a message on the queue. If the queue is full, oldest
* item is overwritten
*
\******************************************************************************/
void SolarPublish::pushQueue(message_t* message)
{
    pthread_mutex_lock(&mutex);
    messageQueue[messageQueueHead]=*message;
    messageQueueHead++;
    if (messageQueueHead==QUEUEDEPTH)
    {
        messageQueueHead=0;
    }
    // If the head passes the tail, remove the tail (=oldest value)
    if (messageQueueHead==messageQueueTail)
    {
        messageQueueTail++;
        if (messageQueueTail==QUEUEDEPTH)
        {
            messageQueueTail=0;
        }
    }
    pthread_mutex_unlock(&mutex);
}

/******************************************************************************\
*
* This method pops a message from the queue. If the queue is empty it returns
* NULL.
*
\******************************************************************************/
message_t* SolarPublish::popQueue()
{
    message_t* msg;
    pthread_mutex_lock(&mutex);
    // Check if anything in the queue
    if (messageQueueHead!=messageQueueTail)
    {
        msg=&messageQueue[messageQueueTail];
        messageQueueTail++;
        if (messageQueueTail==QUEUEDEPTH)
        {
            messageQueueTail=0;
        }
    }
    else
    {
        msg=NULL;
    }
    pthread_mutex_unlock(&mutex);
    return msg;
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of this class
*
\******************************************************************************/
SolarPublish* SolarPublish::getInstance()
{
    // This method should not initialise theInstance. Instead a subclass method.
    return theInstance;
}

/******************************************************************************\
*
*  This method starts the task
*
\******************************************************************************/
void SolarPublish::startThread(void *(*threadFunction) (void *))
{
    int err;
    err=pthread_create(&threadId, NULL, threadFunction, (void *)this);
    if (err)
    {
        logger.logError("Could not start thread function");
    }
    else
    {
        logger.logInfo("Publishing function started");
    }
}

/******************************************************************************\
*
*  This method starts the task
*
\******************************************************************************/
void SolarPublish::start()
{
    closeTask               =false;
}

/******************************************************************************\
*
*  This method kills the task
*
\******************************************************************************/
void SolarPublish::stop()
{
    bool localTaskRunning;

    // Stop all running threads
    pthread_mutex_lock(&mutex);
    closeTask=true;
    localTaskRunning=taskRunning;
    pthread_mutex_unlock(&mutex);

    while (localTaskRunning)
    {
        usleep(10000);

        pthread_mutex_lock(&mutex);
        localTaskRunning=taskRunning;
        pthread_mutex_unlock(&mutex);
    }
    logger.logInfo("Publishing function stopped");
}


/******************************************************************************\
*
* This method bails out...
*
\******************************************************************************/
void SolarPublish::die(const char *file, int line, const char *message)
{
    fprintf(stderr, "%s:%i: %s\n", file, line, message);  //exit(1);
}

/******************************************************************************\
*
* Post message to the AMQP/RabbitMQ exchange
*
\******************************************************************************/
void SolarPublish::postMessage(solarTime_t time, int reading, double value)
{
    message_t msg;
    msg.reading =reading;
    msg.time    =time;
    msg.value   =value;
    pushQueue(&msg);
}

/******************************************************************************\
*
* Test function: posts messages
*
\******************************************************************************/
void SolarPublish::testPublish()
{
    int             i;
    Clock*          clock;
    solarTime_t     time;

    clock=Clock::getInstance();

    i=0;
    while (i<1000000)
    {
       logger.logDebug("Posting test message");
       // Get the current time
        clock->getTime(&time);
        postMessage(time, 0, (i*100)%1500);
        usleep(1000000);
        i++;
    }
}

/******************************************************************************\
*
* Destructor, terminates the connection and cleans up
*
\******************************************************************************/
SolarPublish::~SolarPublish()
{
    stop();
}

/******************************************************************************\
*
* Number of cached values in the Queue
*
* Example QUEUEDEPTH=10
*
* 0123456789
* --XXXXX---
*   ^    ^
*   T    H
* size=H-T=7-2=5
*
* XXX-----XX
*    ^    ^
*    H    T
* size=QUEUEDEPTH-T+H=10-8+3=5
*
\******************************************************************************/
int SolarPublish::getQueueSize()
{
    int size;
    pthread_mutex_lock(&mutex);
    if (messageQueueHead>=messageQueueTail)
    {
        size=messageQueueHead-messageQueueTail;
    }
    else
    {
        size=QUEUEDEPTH-messageQueueTail+messageQueueHead;
    }
    pthread_mutex_unlock(&mutex);
    return size;
}

/******************************************************************************\
*
* Status logging
*
\******************************************************************************/
void SolarPublish::logStatus()
{
    logger.logReport("Publish cache: Messages - %d/%d", 
                    getQueueSize(), QUEUEDEPTH);
}