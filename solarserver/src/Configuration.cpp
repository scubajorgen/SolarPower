/**************************************************************************************************\
*
* Configuration.cpp
*
* Reads configuration from file, usually config.ini
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Configuration.h"
#include "Toolbox.h"
#include "IoPins.h"

/******************************************************************************\
* Variables
\******************************************************************************/

Configuration* Configuration::theInstance=NULL;
char Configuration::configFileName[256]="config.ini";

/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/
Configuration::Configuration()
{
    int counterNo;
    
    // Default log file name
    strncpy(logFileName, LOGFILE, MAXFILENAME-1);
    strncpy(pidFileName, SERVERPIDFILE, MAXFILENAME-1);
    
    counterNo=0;
    while (counterNo<MAX_PULSE_COUNTERS)
    {
        pulsesPerKwh[counterNo]=DEFAULT_PULSES_PER_KILOWATTHOUR;
        counterNo++;
    }
    
    readConfigFile();
}

/******************************************************************************\
*
* Parses one tag-value line
*
\******************************************************************************/
void Configuration::parseLine(char* line)
{
    char* tag;
    char* value;
    
    tag=new char[MAXCONFIGSTRING];
    value=new char[MAXCONFIGSTRING];
    
    if (line[0]!='#')
    {
    
        sscanf(line, "%s %s\n", tag, value);
        if (strcmp(tag, "server_port")==0)
        {
            serverPort=atoi(value);
        }
        else if (strcmp(tag, "pidfile")==0)
        {
            strncpy(pidFileName, value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "sim_meter_file")==0)
        {
            strncpy(simMeterFileName, value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "logfile")==0)
        {
            strncpy(logFileName, value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "pulsemeter_file1")==0)
        {
            strncpy(pulseMeterFileName[0], value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "pulsemeter_file2")==0)
        {
            strncpy(pulseMeterFileName[1], value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "pulsemeter_file3")==0)
        {
            strncpy(pulseMeterFileName[2], value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "pulse1_pulsesperkwh")==0)
        {
            pulsesPerKwh[0]=atoi(value);
        }
        else if (strcmp(tag, "pulse2_pulsesperkwh")==0)
        {
            pulsesPerKwh[1]=atoi(value);
        }
        else if (strcmp(tag, "pulse3_pulsesperkwh")==0)
        {
            pulsesPerKwh[2]=atoi(value);
        }

        else if (strcmp(tag, "pulse1_usage")==0)
        {
            pulseMeterUsage[0]=(pulseMeterUsage_t)atoi(value);
        }
        else if (strcmp(tag, "pulse2_usage")==0)
        {
            pulseMeterUsage[1]=(pulseMeterUsage_t)atoi(value);
        }
        else if (strcmp(tag, "pulse3_usage")==0)
        {
            pulseMeterUsage[2]=(pulseMeterUsage_t)atoi(value);
        }

        else if (strcmp(tag, "pulse1_gpio_input")==0)
        {
            strncpy(gpioPulse[0], value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "pulse2_gpio_input")==0)
        {
            strncpy(gpioPulse[1], value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "pulse3_gpio_input")==0)
        {
            strncpy(gpioPulse[2], value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "pulse1_gpio_output")==0)
        {
            strncpy(gpioLed[0], value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "pulse2_gpio_output")==0)
        {
            strncpy(gpioLed[1], value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "pulse3_gpio_output")==0)
        {
            strncpy(gpioLed[2], value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "heartbeat_gpio_output")==0)
        {
            strncpy(gpioHeartbeatLed, value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "serial_port_device")==0)
        {
            strncpy(serialPortDevice, value, MAXFILENAME-1);
        }
        else if (strcmp(tag, "serial_gpio_invert")==0)
        {
            strncpy(serialGpioInvert, value, MAXPINNAME-1);
        }
        else if (strcmp(tag, "serial_invert")==0)
        {
            serialInvert=atoi(value);
        }
        else if (strcmp(tag, "serial_baudrate")==0)
        {
            serialBaudrate=atoi(value);
        }
        else if (strcmp(tag, "serial_bits")==0)
        {
            serialBits=atoi(value);
        }
        else if (strcmp(tag, "serial_stop_bits")==0)
        {
            serialStopBits=atoi(value);
        }
        else if (strcmp(tag, "serial_parity")==0)
        {
            serialParity=atoi(value);
        }
        else if (strcmp(tag, "dsmr")==0)
        {
            strncpy(dsmr, value, MAXDSMR-1);
        }
        else if (strcmp(tag, "simulation_mode")==0)
        {
            simulationMode=atoi(value);
        }
        else if (strcmp(tag, "pulsemeter_persist_interval")==0)
        {
            pulseMeterPersistInterval=(PulseMeterPersistInterval_t)atoi(value);
        }
#ifdef PUBLISH_AMQP
        else if (strcmp(tag, "amqp_host")==0)
        {
            strncpy(amqpHost, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "amqp_port")==0)
        {
            amqpPort=atoi(value);
        }
        else if (strcmp(tag, "amqp_exchange")==0)
        {
            strncpy(amqpExchange, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "amqp_user")==0)
        {
            strncpy(amqpUser, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "amqp_password")==0)
        {
            strncpy(amqpPassword, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "amqp_routingkey")==0)
        {
            strncpy(amqpRoutingKey, value, MAXCONFIGSTRING-1);
        }
        else if (strcmp(tag, "amqp_vhost")==0)
        {
            strncpy(amqpVHost, value, MAXCONFIGSTRING-1);
        }
#endif
    }
    delete[] tag;
    delete[] value;
}

/******************************************************************************\
*
* Reads the config file
*
\******************************************************************************/
void Configuration::setConfigFile(char* newConfigFileName)
{
    strncpy(configFileName, newConfigFileName, 255);
}

/******************************************************************************\
*
* Reads the config file
*
\******************************************************************************/
bool Configuration::readConfigFile()
{
    char* readLine;   
    FILE *file; 
    bool error;
 
    readLine=new char[MAXLINE];
    error=false;
    
    logger.logInfo("Reading configuration from: %s", configFileName);
    file = fopen(configFileName, "r");
 
    if(file==NULL) 
    {
        logger.logFatal("Error: can't open file %s", configFileName);
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
* Get the one and only instance of this class
*
\******************************************************************************/
Configuration* Configuration::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance = new Configuration();
    }
    return theInstance;
}

/******************************************************************************\
*
* Set the pulses per kWh value
*
\******************************************************************************/
void Configuration::setPulsesPerKwh(int counter, int value)
{
    if(counter>=0 && counter<MAX_PULSE_COUNTERS)
    {
        logger.logInfo("Calibrating pulse counter %d: %d pulses per kWh", counter, value);
        pulsesPerKwh[counter]=value;
    }
    else
    {
        logger.logError("Invalid pulse id while setting the pulses per kWh");
    }

}


/******************************************************************************\
*
* Get the pulses per kWh value
*
\******************************************************************************/
int  Configuration::getPulsesPerKwh(int counter)
{
    int value;
    
    if(counter>=0 && counter<MAX_PULSE_COUNTERS)
    {
        value=pulsesPerKwh[counter];
    }
    else
    {
        value=INVALID_MEASUREMENT;
        logger.logError("Invalid pulse id while getting the pulses per kWh");
    }
    
    
    return value;
}


/******************************************************************************\
*
* Get the GPIO pin number to be used as pulse input for indicated counter
*
\******************************************************************************/
int  Configuration::getGpioPulse(int counter)
{
    int value;
    
    if(counter>=0 && counter<MAX_PULSE_COUNTERS)
    {
        value=IoPins::convertPinNameToPin(gpioPulse[counter]);
    }
    else
    {
        value=INVALID_MEASUREMENT;
        logger.logError("Invalid pulse  while getting  pulse GPIO input pin");
    }
    
    
    return value;
}

/******************************************************************************\
*
* Get the GPIO pin number to be used as LED output for indicated counter
*
\******************************************************************************/
int  Configuration::getGpioLed(int counter)
{
    int value;
    
    if(counter>=0 && counter<MAX_PULSE_COUNTERS)
    {
        value=IoPins::convertPinNameToPin(gpioLed[counter]);
    }
    else
    {
        value=INVALID_MEASUREMENT;
        logger.logError("Invalid pulse  while getting  pulse GPIO output pin for LED");
    }
    return value;
}

/******************************************************************************\
*
* Get the GPIO pin number for the heartbeat led output
*
\******************************************************************************/
int Configuration::getGpioHeartbeatLed()
{
    return IoPins::convertPinNameToPin(gpioHeartbeatLed);
}

/******************************************************************************\
*
* Get the GPIO pin number for the heartbeat led output
*
\******************************************************************************/
int Configuration::getServerPort()
{
    return serverPort;
}

/******************************************************************************\
*
* Set the GPIO pin number for the heartbeat led output
*
\******************************************************************************/
void Configuration::setServerPort(int value)
{
    serverPort=value;
}

/******************************************************************************\
*
* Returns the serial port device, like '/dev/ttyAMA0' 
*
\******************************************************************************/
char* Configuration::getSerialPortDevice()
{
    return serialPortDevice;
} 

/******************************************************************************\
*
* Returns the serial port baudrate
*
\******************************************************************************/
int Configuration::getSerialBaudrate()
{
    return serialBaudrate;
}

/******************************************************************************\
*
* Returns the GPIO pin to be used for inverting the serial signal
* (Some smart meters use inverted serial)
*
\******************************************************************************/
int Configuration::getSerialGpioInvert()
{
    return IoPins::convertPinNameToPin(serialGpioInvert);
}


/******************************************************************************\
*
* Indicates whether the smart meter uses inverted signals
*
\******************************************************************************/
int Configuration::getSerialInvert()
{
    return serialInvert;
}

/******************************************************************************\
*
* Returns the number of bits
*
\******************************************************************************/
int Configuration::getSerialBits()
{
    return serialBits;
}

/******************************************************************************\
*
* Returns the number of stopbits
*
\******************************************************************************/
int Configuration::getSerialStopBits()
{
    return serialStopBits;
}

/******************************************************************************\
*
* Return the parity: -1 for odd, 0 for none, 1 for even
*
\******************************************************************************/
int Configuration::getSerialParity()
{
    return serialParity;
}

/******************************************************************************\
*
* Return DSMR version as string, like "5.0.2"
*
\******************************************************************************/
char* Configuration::getDsmr()
{
    return dsmr;
}

#ifdef PUBLISH_AMQP
/******************************************************************************\
*
* Return AMQP broker host
*
\******************************************************************************/
char* Configuration::getAmqpHost()
{
    return amqpHost;
}

/******************************************************************************\
*
* Return AMQP broker host port
*
\******************************************************************************/
int Configuration::getAmqpPort()
{
    return amqpPort;
}

/******************************************************************************\
*
* Return AMQP exchange name
*
\******************************************************************************/
char* Configuration::getAmqpExchange()
{
    return amqpExchange;
}

/******************************************************************************\
*
* Return username for the AMQP broker
*
\******************************************************************************/
char* Configuration::getAmqpUser()
{
    return amqpUser;
}

/******************************************************************************\
*
* Return password for the AMQP broker
*
\******************************************************************************/
char* Configuration::getAmqpPassword()
{
    return amqpPassword;
}

/******************************************************************************\
*
* Return routing key for the AMQP broker
*
\******************************************************************************/
char* Configuration::getAmqpRoutingKey()
{
    return amqpRoutingKey;
}

/******************************************************************************\
*
* Return VHost for the AMQP broker
*
\******************************************************************************/
char* Configuration::getAmqpVHost()
{
    return amqpVHost;
}

#endif
/******************************************************************************\
*
* Dump the configuration to the screen
*
\******************************************************************************/
void Configuration::dumpConfig()
{
    logger.logInfo("Log filename                %s", logFileName);
    logger.logInfo("PID filename                %s", pidFileName);
    logger.logInfo("Server port                 %d", serverPort);
    logger.logInfo("Heartbeat led               %s (%d)", gpioHeartbeatLed, getGpioHeartbeatLed());
    
    for (int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
    {
        logger.logInfo("PULSE METER %d"                      , counterNo+1);
        logger.logInfo("Usage                       %s"      , usageString[pulseMeterUsage[counterNo]]);
        logger.logInfo("input                       %s (%d)" , gpioPulse[counterNo], getGpioPulse(counterNo));
        logger.logInfo("LED output                  %s (%d)" , gpioLed[counterNo], getGpioLed(counterNo));
        logger.logInfo("pulses per kWh              %d"      , pulsesPerKwh[counterNo]);
        logger.logInfo("Pulse energy filename       %s"      , pulseMeterFileName[counterNo]);
    }

    logger.logInfo("Serial port                 %s", serialPortDevice);
    logger.logInfo("Serial invert GPIO          %s (%d)", serialGpioInvert, IoPins::convertPinNameToPin(serialGpioInvert));
    logger.logInfo("Serial invert               %d", serialInvert);
    logger.logInfo("Serial baudrate             %d", serialBaudrate);
    logger.logInfo("Serial bits                 %d", serialBits);
    logger.logInfo("Serial stop bits            %d", serialStopBits);
    logger.logInfo("Serial parity               %d (-1=odd, 0=no, 1=even)", serialParity);
    logger.logInfo("DSMR version                %s", dsmr);

#ifdef PUBLISH_AMQP
    logger.logInfo("AMQP Host                   %s", amqpHost);
    logger.logInfo("AMQP Port                   %d", amqpPort);
    logger.logInfo("AMQP Exchange               %s", amqpExchange);
    logger.logInfo("AMQP User                   %s", amqpUser);
    logger.logInfo("AMQP Password               %s", amqpPassword);
    logger.logInfo("AMQP Routing key            %s", amqpRoutingKey);
    logger.logInfo("AMQP VHost                  %s", amqpVHost);
#endif
    logger.logInfo("Simulation mode             %d", simulationMode);
    logger.logInfo("Sumulation meter file       %s", simMeterFileName);
    logger.logInfo("Pulsemeter persist interval %d", pulseMeterPersistInterval);
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
* Get the filename for the PID file
*
\******************************************************************************/
char* Configuration::getPidFileName()
{
    return pidFileName;
}

/******************************************************************************\
*
* Set the filename for the PID file
*
\******************************************************************************/
void Configuration::setPidFileName(char* fileName)
{
    strncpy(pidFileName, fileName, MAXFILENAME-1);
}

/******************************************************************************\
*
* Get the filename for the pulse meter file
*
\******************************************************************************/
char* Configuration::getPulseMeterFileName(int counter)
{
    return pulseMeterFileName[counter];
}

/******************************************************************************\
*
* Get the usage of the pulse meter 0 - not used, 1 - consumption, 2 - production
*
\******************************************************************************/
pulseMeterUsage_t Configuration::getPulseMeterUsage(int counter)
{
    return pulseMeterUsage[counter];
}

/******************************************************************************\
*
* Get the simulation mode value 0 - normal operation, 1 - simulation
*
\******************************************************************************/
int Configuration::getSimulationMode()
{
    return simulationMode;
}

/******************************************************************************\
*
* Get the simulation smart meter reading file
*
\******************************************************************************/
char* Configuration::getSimMeterFileName()
{
    return simMeterFileName;
}


/******************************************************************************\
*
* Get the pulse meter persist interval: per minute, per hour, per day
*
\******************************************************************************/
PulseMeterPersistInterval_t Configuration::getPulseMeterPersistInterval()
{
    return pulseMeterPersistInterval;
}

