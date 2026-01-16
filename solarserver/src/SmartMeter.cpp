/******************************************************************************\
*
* SmartMeter.cpp
* Readout for the P1 port of the smart meter
*
\******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <regex.h>

#include "SmartMeter.h"
#include "Configuration.h"
#include "common.h"
#include "wiringSerial.h"
#include "wiringPi.h"

SmartMeter* SmartMeter::theInstance=NULL;


/******************************************************************************\
* Friend methods
\******************************************************************************/


/******************************************************************************\
* Private methods
\******************************************************************************/


/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
SmartMeter::SmartMeter()
{
    clock       =Clock::getInstance();

    // Get the current time
    clock->getTime(&currentReading.dateTime);
    currentReading.electricityImportLowWh   =INVALID_MEASUREMENT;
    currentReading.electricityImportNormalWh=INVALID_MEASUREMENT;
    currentReading.electricityExportLowWh   =INVALID_MEASUREMENT;
    currentReading.electricityExportNormalWh=INVALID_MEASUREMENT;
    currentReading.electricityImportW       =INVALID_MEASUREMENT;
    currentReading.electricityExportW       =INVALID_MEASUREMENT;
    currentReading.gasImport                =INVALID_MEASUREMENT;

    serialPortResetCounter                  =0;
    messageIndex                            =0;
    messageCount                            =0L;
    parseErrors                             =0;
    Configuration* conf                     =Configuration::getInstance();
    simulationMode                          =conf->getSimulationMode();
    if (simulationMode)
    {
        logger.logInfo("Starting Smart Meter in simulation mode");
        simulation                          =Simulation::getInstance();
        //readSimFile("./SmartMeterMessage.txt");
        simMeterMessage                     =simulation->getSmartMeterMessage();
        simPointer                          =strlen(simMeterMessage);
        simCounter                          =SIMULATION_INTERVALS;
        serialPortEnable                    =true;
        serialPortResetCounter              =1;

    }
    else
    {
        logger.logInfo("Starting Smart Meter with serial port");
        initializeSerialPort();
    }
    initializeRegexp();
}

/******************************************************************************\
*
* Destructor
*
\******************************************************************************/
SmartMeter::~SmartMeter()
{
    if (!simulationMode)
    {
        deinitializeSerialPort();
    }
}

/******************************************************************************\
*
* Setup the serial port. If it does not succeed, it sets serialPortEnable=false,
* disabling the processing of the serial port
*
\******************************************************************************/
bool SmartMeter::initializeSerialPort()
{
    struct termios  options ;

    serialPortEnable                    =true;
    bool                        error   =false;
    // Get the configured serial port setting
    Configuration* config               =Configuration::getInstance();
    char* device                        =config->getSerialPortDevice();
    int baudrate                        =config->getSerialBaudrate();
    int invertPin                       =config->getSerialGpioInvert();
    int invert                          =config->getSerialInvert();
    int bits                            =config->getSerialBits();
    int stopBits                        =config->getSerialStopBits();
    int parity                          =config->getSerialParity();

    // Set signal inversion, if required
    pinMode(invertPin, OUTPUT);
    if (invert)
    {
        digitalWrite(invertPin, LOW);
    }
    else
    {
        digitalWrite(invertPin, HIGH);
    }

    // Configure the serial port
    skipFirstMessage        =true;
    firstMessageProcessed   =false;
    serial=serialOpen(device, baudrate);
    serialFlush(serial);
    if (serial>=0)
    {
        tcgetattr (serial, &options) ;   // Read current options
        options.c_cflag &= ~CSIZE ;      // Mask out size
        switch (bits)
        {
        case 5:
            logger.logInfo("Serial port: setting 5 bits");
            options.c_cflag |= CS5 ;
            break;
        case 6:
            logger.logInfo("Serial port: setting 6 bits");
            options.c_cflag |= CS6 ;
            break;
        case 7:
            logger.logInfo("Serial port: setting 7 bits");
            options.c_cflag |= CS7 ;
            break;
        default:
            logger.logInfo("Serial port: setting 8 bits");
            options.c_cflag |= CS8 ;
            break;
        }
        switch (parity)
        {
        case -1:
            logger.logInfo("Serial port: setting odd parity");
            options.c_cflag |= PARENB ;  // Enable Parity - even by default
            options.c_cflag |= PARODD ;  // Switch to odd
            break;
        case 1:
            logger.logInfo("Serial port: setting even parity");
            options.c_cflag |= PARENB ;  // Enable Parity - even by default
            options.c_cflag &= ~PARODD ; // Disable odd
            break;
        case 0:
        default:
            logger.logInfo("Serial port: setting no parity");
            options.c_cflag &= ~PARENB ; // Disable Parity
            break;
        }
        switch (stopBits)
        {
        case 2:
            logger.logInfo("Serial port: setting two stop bit");
            options.c_cflag |= CSTOPB ;  // Two stop bits
            break;
        default:
            logger.logInfo("Serial port: setting one stop bit");
            options.c_cflag &= ~CSTOPB ; // One stop bits
            break;

        }
        tcsetattr (serial, TCSANOW, &options) ;   // Set new options
        serialFlush(serial);
        logger.logDebug("Serial port initialized, descriptor %d", serial);
    }
    else
    {
        logger.logError("Unable to open serial port: %s", strerror(errno));
        serialPortEnable    =false;
        error               =true;
    }
    serialPortResetCounter++;
    return error;
}

/******************************************************************************\
*
* Setup the serial port
*
\******************************************************************************/
void SmartMeter::initializeRegexp()
{
    Configuration* config   =Configuration::getInstance();
    char*          regexp;

    regexp=config->getImportLowKwhRegexp();
    compileRegex (&importLowKwh, regexp);
    regexp=config->getImportHighKwhRegexp();
    compileRegex (&importHighKwh, regexp);
    regexp=config->getExportLowKwhRegexp();
    compileRegex (&exportLowKwh, regexp);
    regexp=config->getExportHighKwhRegexp();
    compileRegex (&exportHighKwh, regexp);
    regexp=config->getImportKwRegexp();
    compileRegex (&importKw, regexp);
    regexp=config->getExportKwRegexp();
    compileRegex (&exportKw, regexp);
    regexp=config->getGasRegexp();
    compileRegex (&gas, regexp);
}

/******************************************************************************\
*
* Clean up the serial port
*
\******************************************************************************/
void SmartMeter::deinitializeSerialPort()
{
    serialClose(serial);
}

/******************************************************************************\
* Public methods
\******************************************************************************/

/******************************************************************************\
*
* Returns the one and only instance of this class
*
\******************************************************************************/
SmartMeter* SmartMeter::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new SmartMeter();
    }
    return theInstance;
}

/******************************************************************************\
*
* Returns most current meter reading
*
\******************************************************************************/
void SmartMeter::getMeterReading(MeterReading_t* reading)
{
    *reading=currentReading;
}

/******************************************************************************\
*
* Receive P1 port messages and convert to meter values
* In simulation mode, a simulated message will be used
*
\******************************************************************************/
void SmartMeter::process()
{
    if (serialPortEnable)
    {
        int  x=0;
        while ((x=dataAvailable())>0)
        {
            int c=getNextChar();
            if (c=='!')
            {
                message[messageIndex]='\0'; // terminate the string
                if (skipFirstMessage)
                {
                    skipFirstMessage=false;
                }
                else
                {
                    bool success=processMessage();
                    if (success)
                    {
                        if (!firstMessageProcessed)
                        {
                            logger.logInfo("First message succesfully processed");
                            firstMessageProcessed=true;
                        }
                    }
                }
                messageCount++;
                messageIndex=0;
            }
            else
            {
                message[messageIndex]=c;
                // prevent the index getting out of bounds
                if (messageIndex<MAXP1MESSAGESIZE-1)
                {
                    messageIndex++;
                }
                else
                {
                    logger.logError("Serial port buffer index out of bounds");
                }
            }
        }
        if (x<0)
        {
            // Best what we can do is reinitialize
            if (!simulationMode)
            {
                logger.logError("Error on serial port %d: %s", serial, strerror (errno));
                initializeSerialPort();
                logger.logInfo("Serial port reset; resets: %d", serialPortResetCounter);
            }
            else
            {
                serialPortResetCounter++;
                logger.logError("Error on simulated serial port; simulating reset %d", serialPortResetCounter);
            }
        }
        if (simulationMode && simCounter>0)
        {
            simCounter--;
        }
    }
}

/******************************************************************************\
*
* Execute regex match and process result
*
\******************************************************************************/
bool SmartMeter::processMatch(regex_t* regex, INT32* var)
{
    bool success=false;
    char* result=matchRegex(regex, message);
    if (strcmp("", result)!=0)
    {
        *var=(int)(1000.0*atof(result)+0.5);
        success=true;
    }
    return success;
}

/******************************************************************************\
*
* Process the P1 message. Extract meter values by regexping
*
\******************************************************************************/
bool SmartMeter::processMessage()
{
    char* result;

    // Get the current time
    clock->getTime(&currentReading.dateTime);

    bool success;
    success     =processMatch(&importLowKwh, &currentReading.electricityImportLowWh);
    if (success)
    {
        success =processMatch(&importHighKwh, &currentReading.electricityImportNormalWh);
    }
    if (success)
    {
        success =processMatch(&exportLowKwh, &currentReading.electricityExportLowWh);
    }
    if (success)
    {
        success =processMatch(&exportHighKwh, &currentReading.electricityExportNormalWh);
    }
    if (success)
    {
        success =processMatch(&importKw, &currentReading.electricityImportW);
    }
    if (success)
    {
        success =processMatch(&exportKw, &currentReading.electricityExportW);
    }
    if (success)
    {
        result=matchRegex(&gas, message);
        currentReading.gasImport=(int)(1000.0*atof(result)+0.5);
    }
    if (!success)
    {
        logger.logError("Error parsing meter message");
        parseErrors++;
    }
    //dumpCurrentReading();
    return success;
}

/******************************************************************************\
*
*
*
\******************************************************************************/
void SmartMeter::compileRegex (regex_t* r, const char* regex_text)
{
    int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
    if (status != 0)
    {
        regerror (status, r, errorMessage, MAXERRORMSG);
        logger.logError("Regex error compiling '%s': %s",regex_text, errorMessage);
    }
}

/******************************************************************************\
*
* Returns the match result or empty string if not found
*
\******************************************************************************/
char* SmartMeter::matchRegex (regex_t* r, const char* to_match)
{
    /* "P" is a pointer into the string which points to the end of the
       previous match. */
    const char* p       = to_match;
    /* "N_matches" is the maximum number of matches allowed. */
    const int n_matches = 10;
    /* "M" contains the matches found. */
    regmatch_t m[n_matches];

    int nomatch = regexec (r, p, n_matches, m, 0);
    if (nomatch)
    {
        matchResult[0]='\0';
        logger.logError("Parsing P1: No more matches.\n %s", to_match);
    }
    else
    {
        for (int i = 0; i < n_matches; i++)
        {
            int start;
            int finish;
            if (m[i].rm_so == -1)
            {
                break;
            }
            start   = m[i].rm_so + (p - to_match);
            finish  = m[i].rm_eo + (p - to_match);
//            printf ("'%.*s' (bytes %d:%d)\n", (finish - start),to_match + start, start, finish);
            sprintf(matchResult, "%.*s", (finish-start), to_match+start);
        }
        p += m[0].rm_eo;
    }
    return matchResult;
}

/******************************************************************************\
*
*
*
\******************************************************************************/
void SmartMeter::dumpCurrentReading()
{
    logger.logDebug("SMARTMETER MESSAGE");
    logger.logDebug("Import Wh low:    %d", currentReading.electricityImportLowWh);
    logger.logDebug("Import Wh normal: %d", currentReading.electricityImportNormalWh);
    logger.logDebug("Export Wh low:    %d", currentReading.electricityExportLowWh);
    logger.logDebug("Export Wh normal: %d", currentReading.electricityExportNormalWh);
    logger.logDebug("Import W:         %d", currentReading.electricityImportW);
    logger.logDebug("Export W:         %d", currentReading.electricityExportW);
    logger.logDebug("Gas Import l:     %d", currentReading.gasImport);
}

/******************************************************************************\
*
* Returns the net power that currently is being used; 0 if  not available
*
\******************************************************************************/
INT32 SmartMeter::getCurrentNetPower()
{
    INT32 netPower=0;
    if (currentReading.electricityImportW!=INVALID_MEASUREMENT &&
        currentReading.electricityExportW!=INVALID_MEASUREMENT)
    {
        netPower=currentReading.electricityImportW-currentReading.electricityExportW;
    }    
    return netPower;
}

/******************************************************************************\
*
* Returns the current import power; 0 if not available
*
\******************************************************************************/
INT32 SmartMeter::getCurrentImportPower()
{
    INT32 power=0;
    if (currentReading.electricityImportW!=INVALID_MEASUREMENT)
    {
        power=currentReading.electricityImportW;
    }    
    return power;
}

/******************************************************************************\
*
* 
*
\******************************************************************************/
INT32 SmartMeter::getCurrentExportPower()
{
    INT32 power=0;
    if (currentReading.electricityExportW!=INVALID_MEASUREMENT)
    {
        power=currentReading.electricityExportW;
    }    
    return power;
}

/******************************************************************************\
*
* Start the first measurement
*
\******************************************************************************/
void SmartMeter::startMeasurement()
{
    startReading=currentReading;
}

/******************************************************************************\
*
* 
*
\******************************************************************************/
void  SmartMeter::retrieveAndRestartMeasurement(Measurement_t *measurement)
{
    measurement->electricityImportLow   =currentReading.electricityImportLowWh;
    measurement->electricityExportLow   =currentReading.electricityExportLowWh;
    measurement->electricityImportNormal=currentReading.electricityImportNormalWh;
    measurement->electricityExportNormal=currentReading.electricityExportNormalWh;
    measurement->gasImport              =currentReading.gasImport;

    INT32 energy                        =(currentReading.electricityImportLowWh   -startReading.electricityImportLowWh   )+
                                         (currentReading.electricityImportNormalWh-startReading.electricityImportNormalWh)-
                                         (currentReading.electricityExportLowWh   -startReading.electricityExportLowWh   )-
                                         (currentReading.electricityExportNormalWh-startReading.electricityExportNormalWh);
    measurement->netPower               =DECIWATT_PER_WATT*energy*MINUTES_PER_HOUR/MEASUREMENT_INTERVAL;

    startMeasurement(); // start next measurement
}



/******************************************************************************\
*
* This function returns the number of chars available for reading
*
\******************************************************************************/
int SmartMeter::dataAvailable()
{
    int dataAvailable;
    if (simulationMode)
    {
        // If simulation intervall passed and previous message has been read fuly...
        if (simPointer==strlen(simMeterMessage) && simCounter==0)
        {
            // Simulate new Smart Meter message
            simPointer          =0;
            simCounter          =SIMULATION_INTERVALS;
        }
        dataAvailable=strlen(simMeterMessage)-simPointer;
        // Simulate serial port error
        if (rand()%1000000==0)
        {
            logger.logInfo("Bad luck: simulating comport error :->");
            dataAvailable=-1;
        }
    }
    else
    {
        dataAvailable=serialDataAvail(serial);
    }
    return dataAvailable;
}

/******************************************************************************\
*
* This function returns next available char
*
\******************************************************************************/
char SmartMeter::getNextChar()
{
    char c;
    if (simulationMode)
    {
        if (simPointer<strlen(simMeterMessage))
        {
            c=simMeterMessage[simPointer];
            simPointer++;
        }
        else
        {
            c=-1;
        }
    }
    else
    {
        c=serialGetchar(serial);
    }
    return c;
}

/******************************************************************************\
*
* This function prints the status
*
\******************************************************************************/
void SmartMeter::logStatus()
{
    logger.logReport("Serial port: messages %ld, port resets %d, parse errors %d, enabled %d, sim %d", 
                    messageCount, serialPortResetCounter, parseErrors, serialPortEnable, simulationMode);
}



