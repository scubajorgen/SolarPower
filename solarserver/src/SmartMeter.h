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

#define MAXP1MESSAGESIZE            8192
#define P1TIMESTAMPSIZE             12

// The number of seconds between subsequent simulated messages.
#define SIMULATION_INTERVAL_SECONDS 1
// The number of sample intervals between subsequent simulated messages.
#define SIMULATION_INTERVALS        (SIMULATION_INTERVAL_SECONDS*MICROSECONDS_PER_SECOND/SAMPLE_TIME)

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

    char*                                   dsmr;
    meterReading_t                          currentReading;
    meterReading_t                          startReading;

    Simulation*                             simulation;
    bool                                    simulationMode;         // Indicates if the system runs in simulation mode
    unsigned int                            simPointer;             // Pointer to the next char in simulationReading
    int                                     simCounter;             // Counts the sample intervals
    char*                                   simMeterMessage;        // The simulated P1 datagram
    size_t                                  simMeterMessageLength;  // The length of the simulated P1 datagram

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
    void    initializeObisCodes             ();
    void    deinitializeSerialPort          ();
    bool    validateP1Datagram              (const char *datagram);
    bool    processMessage                  ();
    bool    processMessageDsmr5             ();
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