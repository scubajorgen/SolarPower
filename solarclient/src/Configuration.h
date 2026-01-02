/**************************************************************************************************\
*
* Configuration.h
*
* Configuration from the config.ini file
*
\**************************************************************************************************/
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "common.h"
#include "Log.h"

#define MAXCONFIGSTRING 128
#define MAXLINE         512
#define MAXFILENAME     256

class Configuration
{
private:
    Log                         logger {"config"};
    static Configuration*       theInstance;
    static  char                configFileName[MAXFILENAME];    
    
    char                        ipAddress[MAXCONFIGSTRING];
    int                         port;
    char                        dbName[MAXCONFIGSTRING];
    char                        dbHost[MAXCONFIGSTRING];
    char                        dbUser[MAXCONFIGSTRING];
    char                        dbPassword[MAXCONFIGSTRING];
    int                         pulsesPerKWh[MAX_PULSE_COUNTERS];
    char                        logFileName[MAXFILENAME];
    char                        pidFileName[MAXFILENAME];


                                Configuration();
    void                        parseLine(char* line);
    bool                        readConfigFile();


public:
    static Configuration*       getInstance();
    static void                 setConfigFile(char* configFileName);
    
    char*                       getServerAddress();
    char*                       getDatabaseName();
    char*                       getDatabaseHost();
    char*                       getDatabaseUser();
    char*                       getDatabasePassword();
    int                         getServerPort();
    void                        dumpConfig();
    int                         getPulsesPerKwh(int pulseCounter);
    char*                       getLogFileName();
    void                        setLogFileName(char* fileName);            
    char*                       getPidFileName();
    void                        setPidFileName(char* fileName);   
};

#endif