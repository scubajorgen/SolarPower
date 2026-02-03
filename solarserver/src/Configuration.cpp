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
        else if (strcmp(tag, "smartmeter_time")==0)
        {
            strncpy(timeRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_import_kwh_low")==0)
        {
            strncpy(importLowKwhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_import_kwh_high")==0)
        {
            strncpy(importHighKwhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_export_kwh_low")==0)
        {
            strncpy(exportLowKwhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_export_kwh_high")==0)
        {
            strncpy(exportHighKwhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_tariff")==0)
        {
            strncpy(tariffRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_import_kw")==0)
        {
            strncpy(importKwRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_export_kw")==0)
        {
            strncpy(exportKwRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_powerfailures")==0)
        {
            strncpy(powerFailuresRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_longpowerfailures")==0)
        {
            strncpy(longPowerFailuresRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_sags_l1")==0)
        {
            strncpy(sagsL1Regexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_sags_l2")==0)
        {
            strncpy(sagsL2Regexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_sags_l3")==0)
        {
            strncpy(sagsL3Regexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_swells_l1")==0)
        {
            strncpy(swellsL1Regexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_swells_l2")==0)
        {
            strncpy(swellsL2Regexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_swells_l3")==0)
        {
            strncpy(swellsL3Regexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_voltage_l1")==0)
        {
            strncpy(voltageL1mVRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_voltage_l2")==0)
        {
            strncpy(voltageL2mVRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_voltage_l3")==0)
        {
            strncpy(voltageL3mVRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_current_l1")==0)
        {
            strncpy(currentL1ARegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_current_l2")==0)
        {
            strncpy(currentL2ARegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_current_l3")==0)
        {
            strncpy(currentL3ARegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_importact_l1_kw")==0)
        {
            strncpy(activeImportL1WhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_importact_l2_kw")==0)
        {
            strncpy(activeImportL2WhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_importact_l3_kw")==0)
        {
            strncpy(activeImportL3WhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_exportact_l1_kw")==0)
        {
            strncpy(activeExportL1WhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_exportact_l2_kw")==0)
        {
            strncpy(activeExportL2WhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_exportact_l3_kw")==0)
        {
            strncpy(activeExportL3WhRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_gas")==0)
        {
            strncpy(gasImportRegexp, value, MAXREGEXP-1);
        }
        else if (strcmp(tag, "smartmeter_gas_time")==0)
        {
            strncpy(gasTimeRegexp, value, MAXREGEXP-1);
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
* Return regexp value for P1 time string
*
\******************************************************************************/
char* Configuration::getTimeRegexp()
{
    return timeRegexp;
}

/******************************************************************************\
*
* Return regexp value for import low kWh
*
\******************************************************************************/
char* Configuration::getImportLowKwhRegexp()
{
    return importLowKwhRegexp;
}

/******************************************************************************\
*
* Return regexp value for import high kWh
*
\******************************************************************************/
char* Configuration::getImportHighKwhRegexp()
{
    return importHighKwhRegexp;
}

/******************************************************************************\
*
* Return regexp value for export low kWh
*
\******************************************************************************/
char* Configuration::getExportLowKwhRegexp()
{
    return exportLowKwhRegexp;
}

/******************************************************************************\
*
* Return regexp value for export high kWh
*
\******************************************************************************/
char* Configuration::getExportHighKwhRegexp()
{
    return exportHighKwhRegexp;
}

/******************************************************************************\
*
* Return regexp value for import kW
*
\******************************************************************************/
char* Configuration::getImportKwRegexp()
{
    return importKwRegexp;
}

/******************************************************************************\
*
* Return regexp value for export kW
*
\******************************************************************************/
char* Configuration::getExportKwRegexp()
{
    return exportKwRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getTariffRegexp()
{
    return tariffRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getPowerFailuresRegexp()
{
    return powerFailuresRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getLongPowerFailuresRegexp()
{
    return longPowerFailuresRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getSagsL1Regexp()
{
    return sagsL1Regexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getSagsL2Regexp()
{
    return sagsL2Regexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getSagsL3Regexp()
{
    return sagsL3Regexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getSwellsL1Regexp()
{
    return swellsL1Regexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getSwellsL2Regexp()
{
    return swellsL2Regexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getSwellsL3Regexp()
{
    return swellsL3Regexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getVoltageL1mVRegexp()
{
    return voltageL1mVRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getVoltageL2mVRegexp()
{
    return voltageL2mVRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getVoltageL3mVRegexp()
{
    return voltageL3mVRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getCurrentL1ARegexp()
{
    return currentL1ARegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getCurrentL2ARegexp()
{
    return currentL2ARegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getCurrentL3ARegexp()
{
    return currentL3ARegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getActiveImportL1WhRegexp()
{
    return activeImportL1WhRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getActiveImportL2WhRegexp()
{
    return activeImportL2WhRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getActiveImportL3WhRegexp()
{
    return activeImportL3WhRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getActiveExportL1WhRegexp()
{
    return activeExportL1WhRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getActiveExportL2WhRegexp()
{
    return activeExportL2WhRegexp;
}

/******************************************************************************\
*
* Return regexp value
*
\******************************************************************************/
char* Configuration::getActiveExportL3WhRegexp()
{
    return activeExportL3WhRegexp;
}

/******************************************************************************\
*
* Return regexp value for gas
*
\******************************************************************************/
char* Configuration::getGasImportRegexp()
{
    return gasImportRegexp;
}

/******************************************************************************\
*
* Return regexp value for gas
*
\******************************************************************************/
char* Configuration::getGasTimeRegexp()
{
    return gasTimeRegexp;
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
    logger.logInfo("time regexp                 %s", timeRegexp);
    logger.logInfo("Import Low  kWh regexp      %s", importLowKwhRegexp);
    logger.logInfo("Import High kWh regexp      %s", importHighKwhRegexp);
    logger.logInfo("Export Low  kWh regexp      %s", exportLowKwhRegexp);
    logger.logInfo("Export High kWh regexp      %s", exportHighKwhRegexp);
    logger.logInfo("Tariff regexp               %s", tariffRegexp);
    logger.logInfo("Import kW regexp            %s", importKwRegexp);
    logger.logInfo("Export kW regexp            %s", exportKwRegexp);
    logger.logInfo("Power Failures regexp       %s", powerFailuresRegexp);
    logger.logInfo("Long Power Failures regexp  %s", longPowerFailuresRegexp);
    logger.logInfo("Sags L1 regexp              %s", sagsL1Regexp);
    logger.logInfo("Sags L2 regexp              %s", sagsL2Regexp);
    logger.logInfo("Sags L3 regexp              %s", sagsL3Regexp);
    logger.logInfo("Swells L1 regexp            %s", swellsL1Regexp);
    logger.logInfo("Swells L2 regexp            %s", swellsL2Regexp);
    logger.logInfo("Swells L3 regexp            %s", swellsL3Regexp);
    logger.logInfo("Voltage L1 regexp           %s", voltageL1mVRegexp);
    logger.logInfo("Voltage L2 regexp           %s", voltageL2mVRegexp);
    logger.logInfo("Voltage L3 regexp           %s", voltageL3mVRegexp);
    logger.logInfo("Current L1 regexp           %s", currentL1ARegexp);
    logger.logInfo("Current L2 regexp           %s", currentL2ARegexp);
    logger.logInfo("Current L3 regexp           %s", currentL3ARegexp);
    logger.logInfo("Act Power Import L1 regexp  %s", activeImportL1WhRegexp);
    logger.logInfo("Act Power Import L2 regexp  %s", activeImportL2WhRegexp);
    logger.logInfo("Act Power Import L3 regexp  %s", activeImportL3WhRegexp);
    logger.logInfo("Act Power Export L1 regexp  %s", activeExportL1WhRegexp);
    logger.logInfo("Act Power Export L2 regexp  %s", activeExportL2WhRegexp);
    logger.logInfo("Act Power Export L3 regexp  %s", activeExportL3WhRegexp);
    logger.logInfo("Gas regexp                  %s", gasImportRegexp);
    logger.logInfo("Gas time regexp             %s", gasTimeRegexp);
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

