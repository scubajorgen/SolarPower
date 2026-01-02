/**************************************************************************************************\
*
* log.h
*
* Simple logger, to vary logging output according to log level
*
\**************************************************************************************************/
#ifndef LOG_H
#define LOG_H

#define MAX_LOGFILENAME_STRING  256
#define MAX_LOGBUFFER           2048
#define MAX_LOGMODULENAME       20
#define MAX_LOGTIME_STRING      25

typedef enum
{
    LOGLEVEL_DEBUG  =0,
    LOGLEVEL_INFO   =1,
    LOGLEVEL_WARNING=2,
    LOGLEVEL_ERROR  =3,
    LOGLEVEL_FATAL  =4
} LogLevel_t;

class Log
{
private:
    static LogLevel_t       theLogLevel;
    static bool             filePrinting;
    static char             logFileName[MAX_LOGFILENAME_STRING];
    static FILE*            fp;
    char                    logBuffer[MAX_LOGBUFFER];
    char                    theModule[MAX_LOGMODULENAME];
    char                    timeString[MAX_LOGTIME_STRING];

    void printLogMessage                (const char* message, va_list args, const char* level);
public:
    Log(const char* module);
    ~Log();
    static void setLogLevel             (LogLevel_t logLevel);
    static void logPrintToFile          (bool enable, char* fileName);
    static void logClose                ();
    void logDebug                       (const char* message, ...);
    void logInfo                        (const char* message, ...);
    void logWarning                     (const char* message, ...);
    void logError                       (const char* message, ...);
    void logFatal                       (const char* message, ...);
    void logReport                      (const char* message, ...);
};

#endif