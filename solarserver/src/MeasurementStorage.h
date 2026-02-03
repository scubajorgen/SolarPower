/**************************************************************************************************\
*
* MeasurementStorage.h
*
* Storage for measurements
*
\**************************************************************************************************/
#if !defined(MEASUREMENTSTORAGE_H)
#define MEASUREMENTSTORAGE_H

#include <pthread.h> 

#include "common.h"
#include "Clock.h"
#include "Log.h"
#include "Configuration.h"



// measurement buffer: enough for one month of 5 minute periods (1 vacant position)
#define MEASUREMENTSTORAGESIZE   (MAXDAYSPERMONTH*INTERVALS_PER_DAY+1)
#define MAXPOWERSTORAGESIZE      (MAXDAYSPERMONTH+1)
#define P1TIMESTAMPSIZE         12                  // From SmartMeter.h

typedef struct
{
    solarTime_t datetime;                           // Datetime in UTC
    INT32       timeIndex;                          // Time index in the year, assuming CET=UTC+1
    INT16       year;                               // Year of the measurement
    INT16       pulse[MAX_PULSE_COUNTERS];          // Pulse counter values
    INT32       pulseMeter[MAX_PULSE_COUNTERS];     // Simulated meter value in Wh
    INT32       pulsePower[MAX_PULSE_COUNTERS];     // Average interval power measured by the pulse counters in dWatt 
    INT32       pulseMaxPower[MAX_PULSE_COUNTERS];  // Max instantanious power measured during interval in Watt
    char        p1Time[P1TIMESTAMPSIZE+1];          // Timetamp of P1 datagram YYMMDDHHmmSS
    INT32       electricityImportLow;               // Smart meter reading, in Wh (tariff 1)
    INT32       electricityImportNormal;            // Smart meter reading, in Wh (tariff 2)
    INT32       electricityExportLow;               // Smart meter reading, in Wh (tariff 1)
    INT32       electricityExportNormal;            // Smart meter reading, in Wh (tariff 2)
    INT32       tariff;                             // Tariff indication
    INT32       grossPower;                         // Average gross interval power in dWatt
    INT32       netPower;                           // Average net interval power in dWatt
    INT32       powerFailures;                      // Power failues
    INT32       powerFailuresLong;
    INT32       sagsL1;                             // Sags, i.e. under voltage, per phase
    INT32       sagsL2;
    INT32       sagsL3;
    INT32       swellsL1;                           // Swells, i.e. over voltage, per phase
    INT32       swellsL2;
    INT32       swellsL3;
    INT32       voltageL1;                          // Voltage per phase, in mV
    INT32       voltageL2;
    INT32       voltageL3;
    INT32       currentL1;                          // Current per phase in A
    INT32       currentL2;
    INT32       currentL3;
    INT32       activeImportPowerL1;                // Active power per phase, import in W
    INT32       activeImportPowerL2;
    INT32       activeImportPowerL3;
    INT32       activeExportPowerL1;                // Active power per phase, export in W
    INT32       activeExportPowerL2;
    INT32       activeExportPowerL3;
    INT32       gasImport;                          // Smart meter reading, in 0.001 m3=liter
    char        gasTime[P1TIMESTAMPSIZE+1];         // Timestring of gas measurement, YYMMDDHHmmSS
}
measurement_t;

typedef struct
{
    solarTime_t maxPowerTime[MAX_PULSE_COUNTERS];       // Time of maximum power value
    INT32       maxPowerTimeDiff[MAX_PULSE_COUNTERS];   // Timediff between two pulses in cs
    INT32       maxPower[MAX_PULSE_COUNTERS];           // Maximum power value in Watt
}
maxPower_t;

class MeasurementStorage
{
    Log                             logger {"measurements"};
    static MeasurementStorage*      theInstance;

    measurement_t                   measurements[MEASUREMENTSTORAGESIZE];   // FIFO buffer or circular buffer
    int                             startOfArray;                           // Oldest element
    int                             startOfArrayNext;                       // Pointer for sending
    int                             endOfArray;                             // First vacant position
    
    maxPower_t                      maxPowers[MAXPOWERSTORAGESIZE];         // FIFO buffer or circular buffer
    int                             maxPowerStartOfArray;
    int                             maxPowerStartOfArrayNext;
    int                             maxPowerEndOfArray;    
    
    pthread_mutex_t                 mutex;                                  // and a mutex
    
    Configuration*                  configuration;
   
                                    MeasurementStorage              ();
    
public:
    static MeasurementStorage*      getInstance                     ();   
                                    ~MeasurementStorage             ();
    
    int                             getNumberOfMeasurementRecords   ();  
    void                            appendMeasurement               (measurement_t* measurement);
    bool                            resetMeasurementNext            ();
    bool                            getNextMeasurement              (measurement_t* measurement);
    bool                            purgeRetrievedMeasurements      ();
    
    int                             getNumberOfMaxPowerRecords      ();
    void                            appendMaxPower                  (maxPower_t* maxPower);
    bool                            resetPowerMaxNext               ();
    bool                            getNextPowerMaxValue            (maxPower_t* maxPower);
    bool                            purgeRetrievedPowerMaxValues    ();

    void                            logStatus                       ();
};

#endif