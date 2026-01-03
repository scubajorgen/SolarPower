/******************************************************************************\
*
* Solar.cpp
* Main programm
*
\******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>


#include "common.h"
#include "Scheduler.h"
#include "Server.h"
#include "Clock.h"
#include "Toolbox.h"
#include "SolarPublish.h"
#include "SmartMeter.h"
#include "Configuration.h"
#include "Log.h"

#if defined(PUBLISH_AMQP) 

#include "SolarPublishAmqp.h"

#elif defined (PUBLISH_WEBSOCKETS)

#include "SolarPublishWebsocket.h"

#endif 

Log             logger("main");
Scheduler       *scheduler;
Server          *server;
SmartMeter      *smartMeter;
IoPins          *ioPins;
Clock           *solarClock;
SolarPublish    *solarPublish;
Configuration   *configuration;

char            c;
bool            exitLoop;

char            timeString[256];
char            optionString[128];
char            fileName[256];

int             pidFileHandle;


/******************************************************************************\
*
* This method initialises the software
*
\******************************************************************************/
void initialiseSolar()
{
    logger.logReport("SOLAR POWER MONITOR 6.0 FOR RASPBERRY PI") ;

    configuration->dumpConfig();

    solarClock=Clock::getInstance();

    // get the IO module; initializes the I/O pins (wiringPi)
    ioPins      =IoPins::getInstance();
    
    // Start the publishing send task. Must be prior to scheduler
#if defined(PUBLISH_AMQP) 
    solarPublish=SolarPublishAmqp::getInstance();
#elif defined (PUBLISH_WEBSOCKETS)
    solarPublish=SolarPublishWebsocket::getInstance();
#endif

    // get the smart meter; initializes the serial port
    smartMeter  =SmartMeter::getInstance();

    // start the scheduling & counting task
    scheduler   =Scheduler::getInstance();

    // start the server
    server      =Server::getInstance();
    server->startServer();

}


/******************************************************************************\
*
* This method cleans up the software
*
\******************************************************************************/
void deinitialiseSolar()
{
    // stop the server
    server->stopServer();
   // Clean up the mess
    delete server;
    delete scheduler;
    delete solarPublish;
    delete ioPins;
    delete solarClock;
    // Wave goodbye
    logger.logReport("Good Bye") ;
    Log::logClose();
}

/******************************************************************************\
*
* This method runs the application as a console application. It waits till
* the user enters 'quit'
*
\******************************************************************************/
void runAsConsoleApplication()
{
    Log::setLogLevel(LOGLEVEL_DEBUG);
    logger.logInfo("Starting as console app, enter 'quit' to bail out");
    char        userInput[100];
    initialiseSolar();
    // Wait for the user to quit
    // wait for user input
    while (strcmp(userInput, "quit\n")!=0)
    {
        fgets(userInput, 99, stdin);
    }
    deinitialiseSolar();
}

/******************************************************************************\
*
* This method shuts down the daemon
*
\******************************************************************************/
void daemonShutdown()
{
    deinitialiseSolar();
    close(pidFileHandle);
}

/******************************************************************************\
*
* Signal handler
*
\******************************************************************************/
void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGHUP:
            logger.logInfo("SIGHUP - Doing nothing");
            break;
        case SIGINT:
        case SIGTERM:
            logger.logInfo("SIGTERM - Daemon exiting");
            daemonShutdown();
            exit(EXIT_SUCCESS);
            break;
        default:
            logger.logInfo("Unhandled signal");
            break;
    }
}


/******************************************************************************\
*
* This method runs the application as daemon
*
\******************************************************************************/
void runAsDaemon()
{
    /* Our process ID and Session ID */
    pid_t               pid, sid, i;
    struct sigaction    newSigAction;
    sigset_t            newSigSet;   
    char                str[10]; 
    

    logger.logReport("Starting Solar as daemon");

    /* Check if parent process id is set */
    if (getppid() == 1)
    {
        /* PPID exists, therefore we are already a daemon */
        logger.logReport("Daemon already running");
        return;
    }    

    
    // Initialise the logging
    Log::logPrintToFile(true, configuration->getLogFileName());
    Log::setLogLevel(LOGLEVEL_DEBUG);



    // Install the signal handler
    /* Set signal mask - signals we want to block */
    sigemptyset(&newSigSet);
    sigaddset(&newSigSet, SIGCHLD);  /* ignore child - i.e. we don't need to wait for it */
    sigaddset(&newSigSet, SIGTSTP);  /* ignore Tty stop signals */
    sigaddset(&newSigSet, SIGTTOU);  /* ignore Tty background writes */
    sigaddset(&newSigSet, SIGTTIN);  /* ignore Tty background reads */
    sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

    /* Set up a signal handler */
    newSigAction.sa_handler = signal_handler;
    sigemptyset(&newSigAction.sa_mask);
    newSigAction.sa_flags = 0;

    /* Signals to handle */
    sigaction(SIGHUP,  &newSigAction, NULL);     /* catch hangup signal */
    sigaction(SIGTERM, &newSigAction, NULL);    /* catch term signal */
    sigaction(SIGINT,  &newSigAction, NULL);     /* catch interrupt signal */    

    
    
    
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) 
    {
            exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) 
    {
        logger.logDebug("Child process created");
        Log::logClose();    
        exit(EXIT_SUCCESS);
    }
    
    /* Change the file mode mask */
    umask(027);
            
    /* Open any logs here */        
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) 
    {
        /* Log the failure */
        logger.logError("Cannot get SID for the child process");
        Log::logClose();   
        exit(EXIT_FAILURE);
    }
    
    
    /* close all descriptors */
    for (i = getdtablesize(); i >= 0; --i)
    {
        close(i);
    }    
    
   
   
    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);


    /* Change the current working directory */
    if ((chdir(RUNDIR)) < 0) 
    {
            /* Log the failure */
            logger.logError("Cannot change working directory");
            Log::logClose();
            exit(EXIT_FAILURE);
    }

    // Create PID file
    /* Ensure only one copy */
    pidFileHandle = open(configuration->getPidFileName(), O_RDWR|O_CREAT, 0600);

    if (pidFileHandle == -1 )
    {
        /* Couldn't open lock file */
        logger.logError("Could not open PID lock file, exiting");
        Log::logClose();
        exit(EXIT_FAILURE);
    }    
    
    /* Try to lock file */
    if (lockf(pidFileHandle,F_TLOCK,0) == -1)
    {
        /* Couldn't get lock on lock file */
        logger.logError("Could not lock PID lock file %s, exiting");
        Log::logClose();
        exit(EXIT_FAILURE);
    }

    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    write(pidFileHandle, str, strlen(str));    


    
    initialiseSolar();
    
    logger.logReport("Running the application as deamon\n");
    
    /* The Big Loop */
    while (1) 
    {
       /* Nothing to be done here, everyting runs in threads... */
       
       sleep(30); /* wait 30 seconds */
    }    
}



/******************************************************************************\
*
* This main program
*
\******************************************************************************/
int main (int argc, char *argv[])
{
    int     option=-1;
    // Default values
    bool runAsDeamon=false;
    strcpy(fileName, "config.ini");

    // Parse the options
    do 
    {
        int option=getopt(argc, argv, "df");
        
        switch (option)
        {
            case 'd':
                runAsDeamon=true;
                break;
            case 'f':
                strncpy(fileName, argv[optind], 255);
                logger.logReport("Filename: %s", fileName);
                Configuration::setConfigFile(fileName);
                break;
            case -1:
            case 255:
                break;
            default:
                logger.logReport("Usage: Solar [OPTION]");
                logger.logReport("       [OPTION] ");
                logger.logReport("       -d               Run as Daemon");
                logger.logReport("       -f <configfile>  Define config file name");
                exit(0);
                break;    
        }
    }
    while (option!=-1 && option!=255);
    // Note: Raspberry Pi acts differently. Normally the exit is EOF, here it is -1 and 255 

    // First of all, read the configuration (at least before going to daemon mode)
    configuration=Configuration::getInstance();    

    // Check whether to run as daemon or console app
    if (runAsDeamon)
    {    
        runAsDaemon();
    } 
    else
    {
        runAsConsoleApplication();
    }
    return 0 ;
}
