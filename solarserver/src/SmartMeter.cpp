/******************************************************************************\
*
* SmartMeter.cpp
* Readout for the P1 port of the smart meter. The class is not thread safe.
*
\******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <cstdint>
#include <termios.h>
#include "wiringSerial.h"
#include "wiringPi.h"

#include "SmartMeter.h"
#include "Configuration.h"
#include "common.h"
#include "Toolbox.h"

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
    receiveState                            =RECEIVESTATE_IDLE;
    parseErrors                             =0;
    validationErrors                        =0;

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

    serialPortEnable       =true;
    bool  error            =false;
    // Get the configured serial port setting
    Configuration* config  =Configuration::getInstance();
    char* device           =config->getSerialPortDevice();
    int baudrate           =config->getSerialBaudrate();
    int invertPin          =config->getSerialGpioInvert();
    int invert             =config->getSerialInvert();
    int bits               =config->getSerialBits();
    int stopBits           =config->getSerialStopBits();
    int parity             =config->getSerialParity();

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
    firstMessageProcessed   =false;
    messageIndex            =0;
    receiveState            =RECEIVESTATE_MESSAGE;
    serial                  =serialOpen(device, baudrate);
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
* Initialize the regular expressions that are used to extract info from P1
* datagram
*
\******************************************************************************/
void SmartMeter::initializeRegexp()
{
    Configuration* config   =Configuration::getInstance();
    Toolbox::compileRegex (&regexTime               , config->getTimeRegexp());
    Toolbox::compileRegex (&regexImportLowKwh       , config->getImportLowKwhRegexp());
    Toolbox::compileRegex (&regexImportHighKwh      , config->getImportHighKwhRegexp());
    Toolbox::compileRegex (&regexExportLowKwh       , config->getExportLowKwhRegexp());
    Toolbox::compileRegex (&regexExportHighKwh      , config->getExportHighKwhRegexp());
    Toolbox::compileRegex (&regexTariff             , config->getTariffRegexp());
    Toolbox::compileRegex (&regexImportKw           , config->getImportKwRegexp());
    Toolbox::compileRegex (&regexExportKw           , config->getExportKwRegexp());
    Toolbox::compileRegex (&regexPowerFailures      , config->getPowerFailuresRegexp());
    Toolbox::compileRegex (&regexPowerFailuresLong  , config->getLongPowerFailuresRegexp());
    Toolbox::compileRegex (&regexSagsL1             , config->getSagsL1Regexp());
    Toolbox::compileRegex (&regexSagsL2             , config->getSagsL2Regexp());
    Toolbox::compileRegex (&regexSagsL3             , config->getSagsL3Regexp());
    Toolbox::compileRegex (&regexSwellsL1           , config->getSwellsL1Regexp());
    Toolbox::compileRegex (&regexSwellsL2           , config->getSwellsL2Regexp());
    Toolbox::compileRegex (&regexSwellsL3           , config->getSwellsL3Regexp());
    Toolbox::compileRegex (&regexVoltageL1          , config->getVoltageL1mVRegexp());
    Toolbox::compileRegex (&regexVoltageL2          , config->getVoltageL2mVRegexp());
    Toolbox::compileRegex (&regexVoltageL3          , config->getVoltageL3mVRegexp());
    Toolbox::compileRegex (&regexCurrentL1          , config->getCurrentL1ARegexp());
    Toolbox::compileRegex (&regexCurrentL2          , config->getCurrentL2ARegexp());
    Toolbox::compileRegex (&regexCurrentL3          , config->getCurrentL3ARegexp());
    Toolbox::compileRegex (&regexActiveImportL1     , config->getActiveImportL1WhRegexp());
    Toolbox::compileRegex (&regexActiveImportL2     , config->getActiveImportL2WhRegexp());
    Toolbox::compileRegex (&regexActiveImportL3     , config->getActiveImportL3WhRegexp());
    Toolbox::compileRegex (&regexActiveExportL1     , config->getActiveExportL1WhRegexp());
    Toolbox::compileRegex (&regexActiveExportL2     , config->getActiveExportL2WhRegexp());
    Toolbox::compileRegex (&regexActiveExportL3     , config->getActiveExportL3WhRegexp());
    Toolbox::compileRegex (&regexGasImport          , config->getGasImportRegexp());
    Toolbox::compileRegex (&regexGasTime            , config->getGasTimeRegexp());
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
void SmartMeter::getMeterReading(meterReading_t* reading)
{
    *reading=currentReading;
}

/******************************************************************************\
*
* Periodic processing of the smart meter. Called every SAMPLE_TIME micro seconds.
* Receive P1 port messages and convert to meter values.
* In simulation mode, a simulated message will be used.
* P1 datagram starts with '/' and ends with '!XXXX[CR][LF]' or '![CR][LF]'.
* '/' and '!' do not occur in the message except as start resp. end
*
\******************************************************************************/
void SmartMeter::process()
{
    if (serialPortEnable)
    {
        int x                               =0;
        while ((x=dataAvailable())>0)
        {
            int c=getNextChar();
            switch (receiveState)
            {
            case RECEIVESTATE_IDLE:
                if (c=='/')
                {
                    // Start of message!!
                    messageIndex            =0;
                    message[messageIndex]   =c;
                    receiveState            =RECEIVESTATE_MESSAGE;
                    messageIndex++;
                }
                break;
            case RECEIVESTATE_MESSAGE:
                if (c=='/')
                {
                    // Unexpected start of message: reset
                    messageIndex            =0;
                }
                else if (c=='!')
                {
                    // End of message, awaing message tail with CRC
                    receiveState=RECEIVESTATE_MESSAGETAIL;
                }
                message[messageIndex]       =c;
                messageIndex++;
                break;
            case RECEIVESTATE_MESSAGETAIL:
                if (c=='/')
                {
                    // Unexpected start of message: reset
                    messageIndex            =0;
                    receiveState            =RECEIVESTATE_MESSAGE;
                    message[messageIndex]   =c;
                    messageIndex++;
                }
                else if (c=='\r')
                {
                    message[messageIndex]   =c;
                    messageIndex++;
                    bool success=validateP1Datagram(message);
                    if (success)
                    {
                        success=processMessage();
                        if (success)
                        {
                            if (!firstMessageProcessed)
                            {
                                logger.logInfo("First message succesfully processed");
                                firstMessageProcessed=true;
                            }
                            messageCount++;
                        }
                    }
                    else
                    {
                        validationErrors++;
                    }
                    receiveState=RECEIVESTATE_IDLE;
                }
                else
                {
                    message[messageIndex]       =c;
                    messageIndex++;
                }
                break;
            }
            if (messageIndex>=MAXP1MESSAGESIZE)
            {
                receiveState=RECEIVESTATE_IDLE;
                logger.logError("P1 message buffer overflow; resetting");
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
* Validate DSMR P1 datagram
*
\******************************************************************************/
bool SmartMeter::validateP1Datagram(const char *datagram)
{
    // Check for start ('/') and end ('!')
    const char *start       = strchr(datagram, '/');
    const char *end         = strchr(datagram, '!');
    end++;
    if (!start || !end || end <= start) 
    {
        logger.logError("P1 validation: Invalid frame bounds");
        return false;
    }
    // Ensure 4 hex characters after '!'
    if (strlen(end) < 5) 
    {
        logger.logError("P1 validation: Missing CRC after '!'");
        return false;
    }
    if (strlen(end) > 8) 
    {
        logger.logError("P1 validation: More characters than expected");
        return false;
    }
    // Extract received checksum

    uint16_t receivedCrc    = 0;
    if (sscanf(end, "%4hx", &receivedCrc) != 1) 
    {
        logger.logError("P1 validation: Invalid CRC format");
        return false;
    }

    // Compute CRC over everything before '!'
    size_t      dataLen     = (size_t)(end - start);
    uint16_t    computedCrc = Toolbox::crc16((const uint8_t*)start, dataLen);
    if (computedCrc == receivedCrc)
    {
        return true;  // ✅ Valid
    }
    else 
    {
        logger.logError("P1 validation: CRC mismatch (computed %04X, received %04X)\n", computedCrc, receivedCrc);
        return false;
    }
}

/******************************************************************************\
*
* Process the P1 message. Extract meter values by regexping
* Processing takes about 6 ms on Raspberry Pi 2 B+
*
\******************************************************************************/
bool SmartMeter::processMessage()
{
    // Get the current time
    clock->getTime(&currentReading.dateTime);
    bool s;
    s =Toolbox::processMatchString(message, &regexTime              ,  currentReading.time                      , P1TIMESTAMPSIZE);
    s&=Toolbox::processMatchFloat (message, &regexImportLowKwh      , &currentReading.electricityImportLowWh    ,1000.0);
    s&=Toolbox::processMatchFloat (message, &regexImportHighKwh     , &currentReading.electricityImportNormalWh ,1000.0);
    s&=Toolbox::processMatchFloat (message, &regexExportLowKwh      , &currentReading.electricityExportLowWh    ,1000.0);
    s&=Toolbox::processMatchFloat (message, &regexExportHighKwh     , &currentReading.electricityExportNormalWh ,1000.0);
    s&=Toolbox::processMatchInt   (message, &regexTariff            , &currentReading.tariff);
    s&=Toolbox::processMatchFloat (message, &regexImportKw          , &currentReading.electricityImportW        ,1000.0);
    s&=Toolbox::processMatchFloat (message, &regexExportKw          , &currentReading.electricityExportW        ,1000.0);
    s&=Toolbox::processMatchInt   (message, &regexPowerFailures     , &currentReading.powerFailures);
    s&=Toolbox::processMatchInt   (message, &regexPowerFailuresLong , &currentReading.powerFailuresLong);
    s&=Toolbox::processMatchInt   (message, &regexSagsL1            , &currentReading.sagsL1);
    s&=Toolbox::processMatchInt   (message, &regexSagsL2            , &currentReading.sagsL2);
    s&=Toolbox::processMatchInt   (message, &regexSagsL3            , &currentReading.sagsL3);
    s&=Toolbox::processMatchInt   (message, &regexSwellsL1          , &currentReading.swellsL1);
    s&=Toolbox::processMatchInt   (message, &regexSwellsL2          , &currentReading.swellsL2);
    s&=Toolbox::processMatchInt   (message, &regexSwellsL3          , &currentReading.swellsL3);
    s&=Toolbox::processMatchFloat (message, &regexVoltageL1         , &currentReading.voltageL1mV              , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexVoltageL2         , &currentReading.voltageL2mV              , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexVoltageL3         , &currentReading.voltageL3mV              , 1000.0);
    s&=Toolbox::processMatchInt   (message, &regexCurrentL1         , &currentReading.currentL1A);
    s&=Toolbox::processMatchInt   (message, &regexCurrentL2         , &currentReading.currentL2A);
    s&=Toolbox::processMatchInt   (message, &regexCurrentL3         , &currentReading.currentL3A);
    s&=Toolbox::processMatchFloat (message, &regexActiveImportL1    , &currentReading.activeImportL1Wh         , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexActiveImportL2    , &currentReading.activeImportL2Wh         , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexActiveImportL3    , &currentReading.activeImportL3Wh         , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexActiveExportL1    , &currentReading.activeExportL1Wh         , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexActiveExportL2    , &currentReading.activeExportL2Wh         , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexActiveExportL3    , &currentReading.activeExportL3Wh         , 1000.0);
    s&=Toolbox::processMatchFloat (message, &regexGasImport         , &currentReading.gasImport                 ,1000.0);
    s&=Toolbox::processMatchString(message, &regexGasTime           ,  currentReading.gasTime                   , P1TIMESTAMPSIZE);
    if (!s)
    {
        logger.logError("Error parsing meter message");
        parseErrors++;
    }
    //dumpCurrentReading();
    return s;
}

/******************************************************************************\
*
*
*
\******************************************************************************/
void SmartMeter::dumpCurrentReading()
{
    meterReading_t r=currentReading;
    logger.logInfo("SMARTMETER MESSAGE");
    logger.logInfo("Timestring        : %s", r.time);
    logger.logInfo("Import (Wh) low   : %9d", r.electricityImportLowWh);
    logger.logInfo("Import (Wh) normal: %9d", r.electricityImportNormalWh);
    logger.logInfo("Export (Wh) low   : %9d", r.electricityExportLowWh);
    logger.logInfo("Export (Wh) normal: %9d", r.electricityExportNormalWh);
    logger.logInfo("Tariff            : %9d", r.tariff);
    logger.logInfo("Import (W)        : %6d L1 %6d L2 %6d L3 %6d", 
                                    r.electricityImportW, r.activeImportL1Wh, r.activeImportL2Wh, r.activeImportL3Wh);
    logger.logInfo("Export (W)        : %6d L1 %6d L2 %6d L3 %6d", 
                                    r.electricityExportW, r.activeExportL1Wh, r.activeExportL2Wh, r.activeExportL3Wh);
    logger.logInfo("Power failures    : all %d, long %d", r.powerFailures, r.powerFailuresLong);
    logger.logInfo("Sags              : L1 %6d L2 %6d L3 %6d", r.sagsL1, r.sagsL2, r.sagsL3);
    logger.logInfo("Swells            : L1 %6d L2 %6d L3 %6d", r.swellsL1, r.swellsL2, r.swellsL3);
    logger.logInfo("Voltage (mV)      : L1 %6d L2 %6d L3 %6d", r.voltageL1mV, r.voltageL2mV, r.voltageL3mV);
    logger.logInfo("Current (A)       : L1 %6d L2 %6d L3 %6d", r.currentL1A, r.currentL2A, r.currentL3A);
    logger.logInfo("Gas Import (l)    : %d (%s)", r.gasImport, r.gasTime);
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
* Start the first measurement: store current reading.
*
\******************************************************************************/
void SmartMeter::startMeasurement()
{
    startReading=currentReading;
}

/******************************************************************************\
*
* Copy the most recent measurements to measurements and restart measuring
*
\******************************************************************************/
void  SmartMeter::retrieveAndRestartMeasurement(measurement_t *measurement)
{
    strncpy(measurement->p1Time     , currentReading.time   , P1TIMESTAMPSIZE+1);
    strncpy(measurement->gasTime    , currentReading.gasTime, P1TIMESTAMPSIZE+1);
    measurement->electricityImportLow   =currentReading.electricityImportLowWh;
    measurement->electricityExportLow   =currentReading.electricityExportLowWh;
    measurement->electricityImportNormal=currentReading.electricityImportNormalWh;
    measurement->electricityExportNormal=currentReading.electricityExportNormalWh;
    measurement->tariff                 =currentReading.tariff;
    measurement->gasImport              =currentReading.gasImport;

    INT32 energy                        =(currentReading.electricityImportLowWh   -startReading.electricityImportLowWh   )+
                                         (currentReading.electricityImportNormalWh-startReading.electricityImportNormalWh)-
                                         (currentReading.electricityExportLowWh   -startReading.electricityExportLowWh   )-
                                         (currentReading.electricityExportNormalWh-startReading.electricityExportNormalWh);
    measurement->netPower               =DECIWATT_PER_WATT*energy*MINUTES_PER_HOUR/MEASUREMENT_INTERVAL;

    measurement->powerFailures          =currentReading.powerFailures;
    measurement->powerFailuresLong      =currentReading.powerFailuresLong;
    measurement->sagsL1                 =currentReading.sagsL1;
    measurement->sagsL2                 =currentReading.sagsL2;
    measurement->sagsL3                 =currentReading.sagsL3;
    measurement->swellsL1               =currentReading.swellsL1;
    measurement->swellsL2               =currentReading.swellsL2;
    measurement->swellsL3               =currentReading.swellsL3;
    measurement->voltageL1              =currentReading.voltageL1mV;
    measurement->voltageL2              =currentReading.voltageL2mV;
    measurement->voltageL3              =currentReading.voltageL3mV;
    measurement->currentL1              =currentReading.currentL1A;
    measurement->currentL2              =currentReading.currentL2A;
    measurement->currentL3              =currentReading.currentL3A;
    measurement->activeImportPowerL1    =currentReading.activeImportL1Wh;
    measurement->activeImportPowerL2    =currentReading.activeImportL2Wh;
    measurement->activeImportPowerL3    =currentReading.activeImportL3Wh;
    measurement->activeExportPowerL1    =currentReading.activeExportL1Wh;
    measurement->activeExportPowerL2    =currentReading.activeExportL2Wh;
    measurement->activeExportPowerL3    =currentReading.activeExportL3Wh;
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
    logger.logReport("Serial port: simulation mode %d, enabled %d, messages processed %d", 
                    simulationMode, serialPortEnable, messageCount);
    logger.logReport("             port resets %d, parse errors %d, validation errors %d", 
                    serialPortResetCounter, parseErrors, validationErrors);
}