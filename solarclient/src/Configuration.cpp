/**************************************************************************************************\
*
* Configuration.cpp
*
* Configuration from the config.ini file
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Configuration.h"

/******************************************************************************\
* Variables
\******************************************************************************/

Configuration*  Configuration::theInstance                  =NULL;
char            Configuration::configFileName[MAXFILENAME]  ="config.ini";

/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
Configuration::Configuration()
{
    // Default log file name
    strncpy(logFileName, LOGFILE, MAXFILENAME);
    strncpy(pidFileName, CLIENTPIDFILE, MAXFILENAME);
    readConfigFile();
}

/******************************************************************************\
*
* Parses one tag-value line
*
\******************************************************************************/
void Configuration::parseLine(char* line)
{
    char* tag     =new char[MAXCONFIGSTRING];
    char* value   =new char[MAXCONFIGSTRING];

    if (line[0]!='#')
    {
        sscanf(line, "%s %s\n", tag, value);
        
        if (strcmp(tag, "port")==0)
        {
            port=atoi(value);
        }
        else if (strcmp(tag, "logfile")==0)
        {
            strncpy(logFileName, value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "pidfile")==0)
        {
            strncpy(pidFileName, value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "address")==0)
        {
            strncpy(ipAddress, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "database_name")==0)
        {
            strncpy(dbName, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "database_host")==0)
        {
            strncpy(dbHost, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "database_user")==0)
        {
            strncpy(dbUser, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "database_password")==0)
        {
            strncpy(dbPassword, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "pulse0_pulsesperkwh")==0)
        {
            pulsesPerKWh[0]=atoi(value);
        }
        else if (strcmp(tag, "pulse1_pulsesperkwh")==0)
        {
            pulsesPerKWh[1]=atoi(value);
        }
        else if (strcmp(tag, "pulse2_pulsesperkwh")==0)
        {
            pulsesPerKWh[2]=atoi(value);
        }
    }    
    
    delete[] tag;
    delete[] value;
}

/******************************************************************************\
*
* Reads the config file
*
\******************************************************************************/
bool Configuration::readConfigFile()
{
    char* readLine  =new char[MAXLINE];
    bool  error     =false;
    FILE* file      = fopen(configFileName, "r");
 
    if(file==NULL) 
    {
        logger.logError("Error: can't open file %s", configFileName);
        error=true;
    }
    else 
    {
        while(fgets(readLine, MAXLINE, file)!=NULL) 
        {
            parseLine(readLine);
        }
        fclose(file);
    }
    delete[] readLine;
    return error;
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of this class (Singleton)
*
\******************************************************************************/
Configuration* Configuration::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new Configuration();
    }
    return theInstance;
}


/******************************************************************************\
*
* Sets the config file name. Must be executed prior to first getInstance() call
*
\******************************************************************************/
void Configuration::setConfigFile(char* newConfigFileName)
{
    strncpy(configFileName, newConfigFileName, MAXFILENAME-1);
}

/******************************************************************************\
*
* This method returns the IP address of the server
*
\******************************************************************************/
char* Configuration::getServerAddress()
{
    return ipAddress;
}

/******************************************************************************\
*
* This method returns the server port to be used
*
\******************************************************************************/
int Configuration::getServerPort()
{
    return port;
}


/******************************************************************************\
*
* This method returns the name of the MySQL database
*
\******************************************************************************/
char* Configuration::getDatabaseName()
{
    return dbName;
}

/******************************************************************************\
*
* This method returns the host that runs the MySQL database
*
\******************************************************************************/
char* Configuration::getDatabaseHost()
{
    return dbHost;
}

/******************************************************************************\
*
* This method returns the the MySQL database user
*
\******************************************************************************/
char* Configuration::getDatabaseUser()
{
    return dbUser;
}

/******************************************************************************\
*
* This method returns the name of the MySQL database
*
\******************************************************************************/
char* Configuration::getDatabasePassword()
{
    return dbPassword;
}

/******************************************************************************\
*
* This method returns the number of pulses per kWh
*
\******************************************************************************/
int Configuration::getPulsesPerKwh(int pulseCounter)
{
    int value;
    
    if (pulseCounter>=0 && pulseCounter<MAX_PULSE_COUNTERS)
    {
        value=pulsesPerKWh[pulseCounter];
    }
    else
    {
        value=0;
    }
    return value;
}

/******************************************************************************\
*
* This method prints the configuration
*
\******************************************************************************/
void Configuration::dumpConfig()
{
    logger.logInfo("Config file       %s", configFileName);
    logger.logInfo("Log file          %s", logFileName);
    logger.logInfo("PID file          %s", pidFileName);
    logger.logInfo("Server address    %s", ipAddress);
    logger.logInfo("Server port       %d", port);
    logger.logInfo("Database Name     %s", dbName);
    logger.logInfo("Database Host     %s", dbHost);
    logger.logInfo("Databasee User    %s", dbUser);
    logger.logInfo("Database Passwd   %s", dbPassword);
    logger.logInfo("Pulse0: pulse/kWh %d", pulsesPerKWh[0]);
    logger.logInfo("Pulse1: pulse/kWh %d", pulsesPerKWh[1]);
    logger.logInfo("Pulse2: pulse/kWh %d", pulsesPerKWh[2]);
}

/******************************************************************************\
*
* Get the filename for the logfile
*
\******************************************************************************/
char* Configuration::getLogFileName()
{
    return logFileName;
}

/******************************************************************************\
*
* Set the filename for the logfile
*
\******************************************************************************/
void Configuration::setLogFileName(char* fileName)
{
    strncpy(logFileName, fileName, MAXFILENAME-1);
}

/******************************************************************************\
*
* Get the filename for the pid file
*
\******************************************************************************/
char* Configuration::getPidFileName()
{
    return pidFileName;
}

/******************************************************************************\
*
* Set the filename for the pid file
*
\******************************************************************************/
void Configuration::setPidFileName(char* fileName)
{
    strncpy(pidFileName, fileName, MAXFILENAME-1);
}