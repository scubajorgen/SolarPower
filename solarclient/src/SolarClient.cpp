/**************************************************************************************************\
*
* SolarClient.cpp
*
* The Solar Client programm
*
\**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#include "Client.h"
#include "Scheduler.h"
#include "Log.h"
#include "Connection.h"
#include "Configuration.h"

Scheduler*      scheduler;
Configuration*  configuration;

char            timeString[256];
char            optionString[128];
char            fileName[256];

int             pidFileHandle;

Log             logger("SolarClient");

/******************************************************************************\
*
* This method initialises the software
*
\******************************************************************************/

void initialiseClient()
{
    logger.logReport("SOLAR POWER MONITOR CLIENT 6.0");
    configuration->dumpConfig();
    scheduler=Scheduler::getInstance();
}

/******************************************************************************\
*
* This method cleans up the software
*
\******************************************************************************/
void deinitialiseClient()
{
    logger.logReport("Exit SolarClient");
    delete scheduler;
}

/******************************************************************************\
*
* This method runs the software as console application
*
\******************************************************************************/
void runAsConsoleApplication()
{
    char            userInput[100];
    char            substring[100];
    int             year;


    initialiseClient();
    Client*         client=Client::getInstance();

    // wait for user input
    while (strcmp(userInput, "quit\n")!=0)
    {
        fgets(userInput, 99, stdin);


        if (strcmp(userInput, "max\n")==0)
        {
            client->requestInstantMax();
        }
        else if (strcmp(userInput, "time\n")==0)
        {
            client->requestTime();
        }

        else if (strcmp(userInput, "resetmax\n")==0)
        {
            client->resetInstantMax();
        }

        else if (strcmp(userInput, "sync\n")==0)
        {
            client->syncTime();
        }
        else if (strcmp(userInput, "storage\n")==0)
        {
            client->requestStorageInfo();
        }
        else if (strcmp(userInput, "help\n")==0)
        {
            printf("max                 get instant max\n");
            printf("time                compare local time and server time\n");
            printf("sync                sync time - deprecated, use NTP to sync time\n");
            printf("resetmax            reset instant max\n");
            printf("storage             get storage info\n");
            printf("process year [yyyy] calculate and store statistics for given year \n");
            printf("help                the help page\n");
            printf("quit                bail out\n");
        }
        else if (strcmp(userInput, "quit\n")==0)
        {
        }
        else
        {
            memcpy(substring, userInput, 13);
            substring[13]='\0';
            if (strcmp(substring, "process year ")==0)
            {
                memcpy(substring, userInput+13, 4);
                substring[4]='\0';
                year=atoi(substring);
                if (year>=2000 && year <2099)
                {
                    client->processAllDays(year);
                }
                else
                {
                    printf("No valid year: %d\n", year);
                }
            }
            else
            {

                printf("Mmmmm.... I'm not really digging that.\n");
            }
        }


    }
    deinitialiseClient();
}


/******************************************************************************\
*
* This method shuts down the daemon
*
\******************************************************************************/
void daemonShutdown()
{

    deinitialiseClient();

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


    logger.logReport("Starting SolarClient as daemon");

    /* Check if parent process id is set */
    if (getppid() == 1)
    {
        /* PPID exists, therefore we are already a daemon */
        logger.logError("Daemon already running\n");
        return;
    }


    // Initialise the logging
    logger.setLogLevel(LOGLEVEL_DEBUG);
    logger.logPrintToFile(true, configuration->getLogFileName());


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
        logger.logClose();
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
        logger.logClose();
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
            logger.logClose();
            exit(EXIT_FAILURE);
    }

    // Create PID file
    /* Ensure only one copy */
    pidFileHandle = open(configuration->getPidFileName(), O_RDWR|O_CREAT, 0600);

    if (pidFileHandle == -1 )
    {
        /* Couldn't open lock file */
        logger.logError("Could not open PID lock file, exiting");
        logger.logClose();
        exit(EXIT_FAILURE);
    }

    /* Try to lock file */
    if (lockf(pidFileHandle,F_TLOCK,0) == -1)
    {
        /* Couldn't get lock on lock file */
        logger.logError("Could not lock PID lock file %s, exiting");
        logger.logClose();
        exit(EXIT_FAILURE);
    }

    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    write(pidFileHandle, str, strlen(str));

    initialiseClient();

    logger.logInfo("Running the application as deamon\n");

    /* The Big Loop */
    while (1)
    {
       /* Nothing to be done here, everyting runs in threads... */
       sleep(30); /* wait 30 seconds */
    }
}


/******************************************************************************\
*
* The main loop
*
\******************************************************************************/
int main(int argc,char *argv[])
{
    Log::setLogLevel(LOGLEVEL_DEBUG);
    // Default values
    bool runAsDeamon=false;

    // default config file
    strcpy(fileName, "config.ini");

    // Parse the input options
    char option;
    do
    {
        option=getopt(argc, argv, "df");

        switch (option)
        {
            case 'd':
                runAsDeamon=true;
                break;
            case 'f':
                strncpy(fileName, argv[optind], 255);
                Configuration::setConfigFile(fileName);
                break;
            case EOF:
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
    while (option!=EOF);

    // Get the configuration for the 1st time. This reads the config file
    configuration=Configuration::getInstance();

    if (runAsDeamon)
    {
        runAsDaemon();
    }
    else
    {
        runAsConsoleApplication();
    }

    return 0;
}