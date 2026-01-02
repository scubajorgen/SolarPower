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

typedef struct
{
    solarTime_t datetime;                           // Datetime in UTC
    INT32       timeIndex;                          // Time index in the year, assuming CET=UTC+1
    INT16       year;                               // Year of the measurement
    INT16       pulse[MAX_PULSE_COUNTERS];          // Pulse counter values
    INT32       pulseMeter[MAX_PULSE_COUNTERS];     // Simulated meter value in Wh
    INT32       pulsePower[MAX_PULSE_COUNTERS];     // Average interval power measured by the pulse counters in dWatt 
    INT32       pulseMaxPower[MAX_PULSE_COUNTERS];  // Max instantanious power measured during interval in Watt
    INT32       electricityImportLow;               // Smart meter reading, in Wh
    INT32       electricityImportNormal;            // Smart meter reading, in Wh
    INT32       electricityExportLow;               // Smart meter reading, in Wh
    INT32       electricityExportNormal;            // Smart meter reading, in Wh
    INT32       gasImport;                          // Smart meter reading, in 0.001 m3=liter
    INT32       grossPower;                         // Average gross interval power in dWatt
    INT32       netPower;                           // Average net interval power in dWatt
}
Measurement_t;

typedef struct
{
    solarTime_t maxPowerTime[MAX_PULSE_COUNTERS];       // Time of maximum power value
    INT32       maxPowerTimeDiff[MAX_PULSE_COUNTERS];   // Timediff between two pulses in cs
    INT32       maxPower[MAX_PULSE_COUNTERS];           // Maximum power value in Watt
}
MaxPower_t;

class MeasurementStorage
{
    Log                             logger {"measurements"};
    static MeasurementStorage*      theInstance;

    Measurement_t                   measurements[MEASUREMENTSTORAGESIZE];
    int                             startOfArray;
    int                             endOfArray;
    INT32                           startTimeIndex;
    
    MaxPower_t                      maxPowers[MAXPOWERSTORAGESIZE];
    int                             maxPowerStartOfArray;
    int                             maxPowerEndOfArray;    
    
    pthread_mutex_t                 mutex;   						// and a mutex
    
    Configuration*                  configuration;
    
    
                                    MeasurementStorage              ();
    
public:
    static MeasurementStorage*      getInstance                     ();   
                                    ~MeasurementStorage             ();
                                    
    void                            setStartTimeIndex               (int startTimeIndex);                                    
    
    int                             getNumberOfMeasurementRecords   ();  
    void                            getMeasurementInfo              (int* number, INT32* startTimeIndex);
    void                            appendMeasurement               (Measurement_t* measurement);   
    bool                            getMeasurement                  (int number, Measurement_t* measurement);
    bool                            getLastMeasurement              (Measurement_t* measurement);
    
    int                             getNumberOfMaxPowerRecords      ();
    void                            appendMaxPower                  (MaxPower_t* maxPower); 
    bool                            getPowerMaxValue                (int number, MaxPower_t* maxPower);   
    bool                            getLastPowerMaxValue            (MaxPower_t* maxPower);   
    
};

#endif