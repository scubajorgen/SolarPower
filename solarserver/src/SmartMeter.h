/******************************************************************************\
*
* SmartMeter.cpp
* Readout for the P1 port of the smart meter
*
\******************************************************************************/
#if !defined(SMARTMETER_H)
#define SMARTMETER_H

#include "Clock.h"
#include "Log.h"
#include "Meter.h"
#include "Simulation.h"
#include "Toolbox.h"

#define MAXP1MESSAGESIZE        8192
#define P1TIMESTAMPSIZE         12
#define SIMULATION_INTERVALS    (1*MICROSECONDS_PER_SECOND/SAMPLE_TIME)

typedef enum
{
    RECEIVESTATE_IDLE,          // awaiting start of P1 message ('/')
    RECEIVESTATE_MESSAGE,       // receiving message, awaiting end of message ('!')
    RECEIVESTATE_MESSAGETAIL    // receiving message tail with CRC (or not)
} 
receiveState_t;

typedef struct
{
    solarTime_t dateTime;
    char        time[P1TIMESTAMPSIZE+1];
    INT32       electricityImportLowWh;
    INT32       electricityImportNormalWh;
    INT32       electricityExportLowWh;
    INT32       electricityExportNormalWh;
    INT32       tariff;
    INT32       electricityImportW;
    INT32       electricityExportW;

    INT32       powerFailures;
    INT32       powerFailuresLong;
    INT32       sagsL1;
    INT32       sagsL2;
    INT32       sagsL3;
    INT32       swellsL1;
    INT32       swellsL2;
    INT32       swellsL3;
    INT32       voltageL1mV;
    INT32       voltageL2mV;
    INT32       voltageL3mV;
    INT32       currentL1A;
    INT32       currentL2A;
    INT32       currentL3A;
    INT32       activeImportL1Wh;
    INT32       activeImportL2Wh;
    INT32       activeImportL3Wh;
    INT32       activeExportL1Wh;
    INT32       activeExportL2Wh;
    INT32       activeExportL3Wh;

    INT32       gasImport;
    char        gasTime[P1TIMESTAMPSIZE+1];
}
meterReading_t;


class SmartMeter : Meter
{
private:
    // The one and only instance of this class
    Log                                     logger {"smartmeter"};
    static SmartMeter*                      theInstance;
    Clock*                                  clock;
    int                                     serial;

    bool                                    firstMessageProcessed;
    receiveState_t                          receiveState;

    char                                    message[MAXP1MESSAGESIZE];
    int                                     messageIndex;

    regex_t                                 regexTime;
    regex_t                                 regexImportLowKwh;
    regex_t                                 regexImportHighKwh;
    regex_t                                 regexExportLowKwh;
    regex_t                                 regexExportHighKwh;
    regex_t                                 regexTariff;
    regex_t                                 regexImportKw;
    regex_t                                 regexExportKw;
    regex_t                                 regexPowerFailures;
    regex_t                                 regexPowerFailuresLong;
    regex_t                                 regexSagsL1;
    regex_t                                 regexSagsL2;
    regex_t                                 regexSagsL3;
    regex_t                                 regexSwellsL1;
    regex_t                                 regexSwellsL2;
    regex_t                                 regexSwellsL3;
    regex_t                                 regexVoltageL1;
    regex_t                                 regexVoltageL2;
    regex_t                                 regexVoltageL3;
    regex_t                                 regexCurrentL1;
    regex_t                                 regexCurrentL2;
    regex_t                                 regexCurrentL3;
    regex_t                                 regexActiveImportL1;
    regex_t                                 regexActiveImportL2;
    regex_t                                 regexActiveImportL3;
    regex_t                                 regexActiveExportL1;
    regex_t                                 regexActiveExportL2;
    regex_t                                 regexActiveExportL3;
    regex_t                                 regexGasImport;
    regex_t                                 regexGasTime;
    meterReading_t                          currentReading;
    meterReading_t                          startReading;

    Simulation*                             simulation;
    bool                                    simulationMode;     // Indicates if the system runs in simulation mode
    unsigned int                            simPointer;         // Pointer to the next char in simulationReading
    int                                     simCounter;         // Counts the sample intervals
    char*                                   simMeterMessage;    // 

    // TO DO: guard with mutex
    bool                                    serialPortEnable;
    long                                    messageCount;
    int                                     serialPortResetCounter;
    int                                     parseErrors;
    int                                     validationErrors;
    // End TO DO

    SmartMeter                              ();
    ~SmartMeter                             ();
    bool    initializeSerialPort            ();
    void    initializeRegexp                ();
    void    deinitializeSerialPort          ();
    bool    validateP1Datagram              (const char *datagram);
    bool    processMessage                  ();
    void    dumpCurrentReading              ();

    void    updateSimMessage                ();
    int     dataAvailable                   ();
    char    getNextChar                     ();
public:
    static SmartMeter* getInstance          ();
    void    getMeterReading                 (meterReading_t* reading);
    void    process                         () override;
    INT32   getCurrentNetPower              ();
    void    startMeasurement                () override;
    void    retrieveAndRestartMeasurement   (measurement_t *measurement) override;
    INT32   getCurrentImportPower           () override;
    INT32   getCurrentExportPower           () override;
    void    logStatus                       ();
    bool    hasReading                      ();
};




#endif