
/**************************************************************************************************\
*
* log.cpp
*
* Simple logger, to vary logging output according to log level
*
\**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "Log.h"
#include "Clock.h"


/**************************************************************************************************\
* VARIABLES
\**************************************************************************************************/

LogLevel_t      Log::theLogLevel                            =LOGLEVEL_INFO;
bool            Log::filePrinting                           =false;
char            Log::logFileName[MAX_LOGFILENAME_STRING];
FILE*           Log::fp;

/**************************************************************************************************\
*
* Helper, generic log message printing routine
*
\**************************************************************************************************/
void Log::printLogMessage(const char* message, va_list args, const char* level)
{
    Clock* clock=Clock::getInstance();
    clock->getTimeString(timeString);

    vsprintf(logBuffer, message, args);
    if (filePrinting)
    {
        if (fp==NULL)
        {
           fp = fopen(logFileName, "w");
        }
        if (fp!=NULL)
        {
            fprintf(fp, "%-12s %s, %-7s: %s\n", theModule, timeString, level, logBuffer);
            fflush(fp);
        }
    }
    else
    {
        printf("%-12s %s, %-7s: %s\n", theModule, timeString, level, logBuffer);
    }
}

/**************************************************************************************************\
*
* Constructor
*
\**************************************************************************************************/
Log::Log(const char* module)
{
    strncpy(theModule, module, MAX_LOGMODULENAME-1);
}

/**************************************************************************************************\
*
* Destructor
*
\**************************************************************************************************/
Log::~Log()
{
}

/**************************************************************************************************\
*
* Sets the log level
*
\**************************************************************************************************/
void Log::setLogLevel(LogLevel_t logLevel)
{
    theLogLevel=logLevel;
}

/**************************************************************************************************\
*
* Debug level logging
*
\**************************************************************************************************/
void Log::logDebug(const char* message, ...)
{
    if (theLogLevel<=LOGLEVEL_DEBUG)
    {
        va_list args;
        va_start(args, message);
        printLogMessage(message, args, "DEBUG");
    }
}

/**************************************************************************************************\
*
* Info level logging
*
\**************************************************************************************************/
void Log::logInfo(const char* message, ...)
{
    if (theLogLevel<=LOGLEVEL_INFO)
    {
        va_list args;
        va_start(args, message);
        printLogMessage(message, args, "INFO");
    }
}

/**************************************************************************************************\
*
* Warning level logging
*
\**************************************************************************************************/
void Log::logWarning(const char* message, ...)
{
    if (theLogLevel<=LOGLEVEL_WARNING)
    {
        va_list args;
        va_start(args, message);
        printLogMessage(message, args, "WARNING");
    }
}

/**************************************************************************************************\
*
* Info level logging
*
\**************************************************************************************************/
void Log::logError(const char* message, ...)
{
    if (theLogLevel<=LOGLEVEL_ERROR)
    {
        va_list args;
        va_start(args, message);
        printLogMessage(message, args, "ERROR");
    }
}

/**************************************************************************************************\
*
* Fatal level logging and say goodbye
*
\**************************************************************************************************/
void Log::logFatal(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    printLogMessage(message, args, "FATAL");
    exit(0);
}

/**************************************************************************************************\
*
* Reporting, always show
*
\**************************************************************************************************/
void Log::logReport(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    printLogMessage(message, args, "REPORT");
}

/**************************************************************************************************\
*
* This method redirects logging to file (true) or screen (false)
*
\**************************************************************************************************/
void Log::logPrintToFile(bool enable, char* fileName)
{
    if (fileName!=NULL)
    {
        strncpy(logFileName, fileName, MAX_LOGFILENAME_STRING-1);
    }

    filePrinting=enable;
}

/**************************************************************************************************\
*
* Closes the log file
*
\**************************************************************************************************/
void Log::logClose()
{
    if (fp!=NULL)
    {
        fclose(fp);
        fp=NULL;
    }
}