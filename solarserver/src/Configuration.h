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

class Configuration
{
    static  Configuration*  theInstance;
    static  char            configFileName[256];

    Log                     logger {"config"};
    int                     serverPort;
    int                     pulsesPerKwh[MAX_PULSE_COUNTERS];
    char                    gpioPulse[MAX_PULSE_COUNTERS][MAXPINNAME];
    char                    gpioLed[MAX_PULSE_COUNTERS][MAXPINNAME];
    char                    gpioHeartbeatLed[MAXPINNAME];
    char                    logFileName[MAXFILENAME];
    char                    pidFileName[MAXFILENAME];
    char                    pulseMeterFileName1[MAXFILENAME];
    char                    pulseMeterFileName2[MAXFILENAME];
    char                    pulseMeterFileName3[MAXFILENAME];
    
    char                    serialPortDevice[MAXFILENAME];
    char                    serialGpioInvert[MAXPINNAME];
    int                     serialInvert;
    int                     serialBaudrate;
    int                     serialBits;
    int                     serialStopBits;
    int                     serialParity;
    
    char                    importLowKwhRegexp[MAXREGEXP];
    char                    importHighKwhRegexp[MAXREGEXP];
    char                    exportLowKwhRegexp[MAXREGEXP];
    char                    exportHighKwhRegexp[MAXREGEXP];
    char                    importKwRegexp[MAXREGEXP];
    char                    exportKwRegexp[MAXREGEXP];
    char                    gasRegexp[MAXREGEXP];
#ifdef PUBLISH_AMQP
    char                    amqpHost[MAXCONFIGSTRING];
    int                     amqpPort;
    char                    amqpExchange[MAXCONFIGSTRING];
    char                    amqpUser[MAXCONFIGSTRING];
    char                    amqpPassword[MAXCONFIGSTRING];
    char                    amqpRoutingKey[MAXCONFIGSTRING];
    char                    amqpVHost[MAXCONFIGSTRING];
#endif

                            Configuration();
    void                    parseLine(char* line);
    bool                    readConfigFile();   
    
public:
    static  Configuration*  getInstance();    

    static void             setConfigFile(char* configFileName);

    void                    setPulsesPerKwh(int counter, int value);
    int                     getPulsesPerKwh(int counter);
    
    int                     getGpioPulse(int counter);
//    void                    setGpioPulse(int counter, int value);
    int                     getGpioLed(int counter);
//    void                    setGpioLed(int counter, int value);
    int                     getGpioHeartbeatLed();
//    void                    setGpioHeartbeatLed(int value);
    int                     getServerPort();
    void                    setServerPort(int value);
    char*                   getLogFileName();
    void                    setLogFileName(char* fileName);
    char*                   getPidFileName();
    void                    setPidFileName(char* fileName);
    char*                   getPulseMeterFileName1();
    char*                   getPulseMeterFileName2();
    char*                   getPulseMeterFileName3();


    char*                   getSerialPortDevice();
    int                     getSerialBaudrate();
    int                     getSerialGpioInvert();
    int                     getSerialInvert();
    int                     getSerialBits();
    int                     getSerialStopBits();
    int                     getSerialParity();

    char*                   getImportLowKwhRegexp();
    char*                   getImportHighKwhRegexp();
    char*                   getExportLowKwhRegexp();
    char*                   getExportHighKwhRegexp();
    char*                   getImportKwRegexp();
    char*                   getExportKwRegexp();
    char*                   getGasRegexp();

    char*                   getAmqpHost();
    int                     getAmqpPort();
    char*                   getAmqpVHost();
    char*                   getAmqpExchange();
    char*                   getAmqpRoutingKey();
    char*                   getAmqpUser();
    char*                   getAmqpPassword();
    
    void                    dumpConfig();
};
#endif


