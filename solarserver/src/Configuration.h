/**************************************************************************************************\
*
* Configuration.h
*
* Reads configuration from file, usually config.ini
*
\**************************************************************************************************/
#ifndef CONFIGURATION_H

#define CONFIGURATION_H

#include "common.h"
#include "Log.h"

#define MAXCONFIGSTRING 128
#define MAXLINE         512
#define MAXFILENAME     256
#define MAXREGEXP       128
#define MAXPINNAME      10

typedef enum
{
    PERSISTINTERVAL_NONE    =0,
    PERSISTINTERVAL_MINUTE  =1,
    PERSISTINTERVAL_HOUR    =2,
    PERSISTINTERVAL_DAY     =3
} PulseMeterPersistInterval_t;

class Configuration
{
    static  Configuration*      theInstance;
    static  char                configFileName[256];

    Log                         logger {"config"};
    int                         serverPort;
    char                        gpioHeartbeatLed[MAXPINNAME];

    char                        logFileName[MAXFILENAME];
    char                        pidFileName[MAXFILENAME];
    char                        simMeterFileName[MAXFILENAME];

    int                         pulsesPerKwh        [MAX_PULSE_COUNTERS];
    char                        gpioPulse           [MAX_PULSE_COUNTERS][MAXPINNAME];
    char                        gpioLed             [MAX_PULSE_COUNTERS][MAXPINNAME];
    char                        pulseMeterFileName  [MAX_PULSE_COUNTERS][MAXFILENAME];
    pulseMeterUsage_t           pulseMeterUsage     [MAX_PULSE_COUNTERS];
    
    char                        serialPortDevice[MAXFILENAME];
    char                        serialGpioInvert[MAXPINNAME];
    int                         serialInvert;
    int                         serialBaudrate;
    int                         serialBits;
    int                         serialStopBits;
    int                         serialParity;
    
    char                        importLowKwhRegexp[MAXREGEXP];
    char                        importHighKwhRegexp[MAXREGEXP];
    char                        exportLowKwhRegexp[MAXREGEXP];
    char                        exportHighKwhRegexp[MAXREGEXP];
    char                        importKwRegexp[MAXREGEXP];
    char                        exportKwRegexp[MAXREGEXP];
    char                        tariffRegexp[MAXREGEXP];
    char                        gasImportRegexp[MAXREGEXP];
    char                        gasTimeRegexp[MAXREGEXP];

    char                        timeRegexp[MAXREGEXP];
    char                        powerFailuresRegexp[MAXREGEXP];
    char                        longPowerFailuresRegexp[MAXREGEXP];
    char                        sagsL1Regexp[MAXREGEXP];
    char                        sagsL2Regexp[MAXREGEXP];
    char                        sagsL3Regexp[MAXREGEXP];
    char                        swellsL1Regexp[MAXREGEXP];
    char                        swellsL2Regexp[MAXREGEXP];
    char                        swellsL3Regexp[MAXREGEXP];
    char                        voltageL1mVRegexp[MAXREGEXP];
    char                        voltageL2mVRegexp[MAXREGEXP];
    char                        voltageL3mVRegexp[MAXREGEXP];
    char                        currentL1ARegexp[MAXREGEXP];
    char                        currentL2ARegexp[MAXREGEXP];
    char                        currentL3ARegexp[MAXREGEXP];
    char                        activeImportL1WhRegexp[MAXREGEXP];
    char                        activeImportL2WhRegexp[MAXREGEXP];
    char                        activeImportL3WhRegexp[MAXREGEXP];
    char                        activeExportL1WhRegexp[MAXREGEXP];
    char                        activeExportL2WhRegexp[MAXREGEXP];
    char                        activeExportL3WhRegexp[MAXREGEXP];
#ifdef PUBLISH_AMQP
    char                        amqpHost[MAXCONFIGSTRING];
    int                         amqpPort;
    char                        amqpExchange[MAXCONFIGSTRING];
    char                        amqpUser[MAXCONFIGSTRING];
    char                        amqpPassword[MAXCONFIGSTRING];
    char                        amqpRoutingKey[MAXCONFIGSTRING];
    char                        amqpVHost[MAXCONFIGSTRING];
#endif

    int                         simulationMode;
    PulseMeterPersistInterval_t pulseMeterPersistInterval;


                                Configuration();
    void                        parseLine(char* line);
    bool                        readConfigFile();

    const char                  usageString[3][12]={"NOT USED", "CONSUMPTION", "PRODUCTION"};
    
public:
    static  Configuration*      getInstance(); 

    static void                 setConfigFile(char* configFileName);

    void                        setPulsesPerKwh(int counter, int value);
    int                         getPulsesPerKwh(int counter);
    int                         getGpioPulse(int counter);
    int                         getGpioLed(int counter);
    char*                       getPulseMeterFileName(int counter);

    int                         getGpioHeartbeatLed();
    pulseMeterUsage_t           getPulseMeterUsage(int counter);
    int                         getServerPort();
    void                        setServerPort(int value);
    char*                       getLogFileName();
    void                        setLogFileName(char* fileName);
    char*                       getPidFileName();
    void                        setPidFileName(char* fileName);
    char*                       getSimMeterFileName();

    char*                       getSerialPortDevice();
    int                         getSerialBaudrate();
    int                         getSerialGpioInvert();
    int                         getSerialInvert();
    int                         getSerialBits();
    int                         getSerialStopBits();
    int                         getSerialParity();

    char*                       getTimeRegexp();
    char*                       getImportLowKwhRegexp();
    char*                       getImportHighKwhRegexp();
    char*                       getExportLowKwhRegexp();
    char*                       getExportHighKwhRegexp();
    char*                       getTariffRegexp();
    char*                       getImportKwRegexp();
    char*                       getExportKwRegexp();
    char*                       getPowerFailuresRegexp();
    char*                       getLongPowerFailuresRegexp();
    char*                       getSagsL1Regexp();
    char*                       getSagsL2Regexp();
    char*                       getSagsL3Regexp();
    char*                       getSwellsL1Regexp();
    char*                       getSwellsL2Regexp();
    char*                       getSwellsL3Regexp();
    char*                       getVoltageL1mVRegexp();
    char*                       getVoltageL2mVRegexp();
    char*                       getVoltageL3mVRegexp();
    char*                       getCurrentL1ARegexp();
    char*                       getCurrentL2ARegexp();
    char*                       getCurrentL3ARegexp();
    char*                       getActiveImportL1WhRegexp();
    char*                       getActiveImportL2WhRegexp();
    char*                       getActiveImportL3WhRegexp();
    char*                       getActiveExportL1WhRegexp();
    char*                       getActiveExportL2WhRegexp();
    char*                       getActiveExportL3WhRegexp();
    char*                       getGasImportRegexp();
    char*                       getGasTimeRegexp();

    char*                       getAmqpHost();
    int                         getAmqpPort();
    char*                       getAmqpVHost();
    char*                       getAmqpExchange();
    char*                       getAmqpRoutingKey();
    char*                       getAmqpUser();
    char*                       getAmqpPassword();

    int                         getSimulationMode();
    PulseMeterPersistInterval_t getPulseMeterPersistInterval();

    void                        dumpConfig();
};
#endif


