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
    int     err;
    pthread_mutexattr_t attr;

    // initialise the task
    taskRunning             =false;
    closeTask               =false;

    startOfQueue            =0;
    endOfQueue              =0;

    // create the mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

    err=pthread_mutex_init(&mutex, &attr);

    if (err)
    {
        logger.logFatal("Unable to create mutex");
    }

}


/******************************************************************************\
*
* This method pushes a message on the queue
*
\******************************************************************************/

void SolarPublish::pushQueue(char* item)
{
    int newStartOfQueue;

    // Check whether the task needs to be killed
    pthread_mutex_lock(&mutex);
    newStartOfQueue=startOfQueue+1;
    if (newStartOfQueue==QUEUEDEPTH)
    {
        newStartOfQueue=0;
    }

    if (newStartOfQueue!=endOfQueue)
    {
        strncpy(localMessageQueue[startOfQueue], item, MAXMESSAGELENGTH);
        startOfQueue=newStartOfQueue;
    }

    pthread_mutex_unlock(&mutex);
}

/******************************************************************************\
*
* This method pops a message from the queue. If the queue is empty it returns
* NULL.
*
\******************************************************************************/
char* SolarPublish::popQueue()
{
    char* returnString;

    // Check whether the task needs to be killed
    pthread_mutex_lock(&mutex);

    if (endOfQueue!=startOfQueue)
    {
        strncpy(outBuffer, localMessageQueue[endOfQueue], MAXMESSAGELENGTH-1);
        endOfQueue++;
        if (endOfQueue==QUEUEDEPTH)
        {
            endOfQueue=0;
        }
        returnString=outBuffer;
    }
    else
    {
        returnString=NULL;
    }

    pthread_mutex_unlock(&mutex);

    return returnString;
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of this class
*
\******************************************************************************/
SolarPublish* SolarPublish::getInstance   ()
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
    sprintf(inBuffer, "%d %f", reading, value);
    pushQueue(inBuffer);
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
       logger.logDebug("Posting message");

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
