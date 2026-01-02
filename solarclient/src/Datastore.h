#include <mysql.h>
#include <time.h>

#include "common.h"
#include "Clock.h"
#include "Log.h"

#define QUERYSTRINGSIZE     2048

// Structure defining a 5 minute measurement value
typedef struct
{
/*
    int             timeIndex;                              // Time index of the interval with the year
    int             year;                                   // The year of the measurement
    int             pulses[MAX_PULSE_COUNTERS];             // Pulse counter: Number of pulses counted during the 5 min interval
    int             power[MAX_PULSE_COUNTERS];              // Pulse counter: Average power in 0.1 W 
    unsigned int    importlow;                              // Main meter reading, import at low tariff in Wh
    unsigned int    importnormal;                           // Main meter reading, import at normal tariff in Wh
    unsigned int    exportlow;                              // Main meter reading, export at low tariff in Wh
    unsigned int    exportnormal;                           // Main meter reading, export at normal tariff in Wh
    unsigned int    gasimport;                              // Main meter reading, gas import in liter (0.001 m3)
*/
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
} fiveMinuteMeasurement_t;

typedef struct
{
    INT32           timeDiff[MAX_PULSE_COUNTERS];
    INT32           power[MAX_PULSE_COUNTERS];
    solarTime_t     time[MAX_PULSE_COUNTERS];    
} instantMax_t;

/*
typedef struct
{
    int     day;
    int     month;
    int     year;
    double  maxPower[MAX_PULSE_COUNTERS];
    int     maxPowerIndex[MAX_PULSE_COUNTERS];
    int     energy[MAX_PULSE_COUNTERS];
    
} dayStatistics_t;
*/

// Structure defines the statistics about a bunch of 5 minute values
typedef struct
{
    int     year;                                               // the year
    int     pulseCountSum[MAX_PULSE_COUNTERS];                  // the sum of the counts
    int     pulsePowerSum[MAX_PULSE_COUNTERS];                  // the sum of average power values in dWatt
    int     numberOfRecordsInSum[MAX_PULSE_COUNTERS];           // number of records that added to the sum
    int     maxCount[MAX_PULSE_COUNTERS];                       // maximum count value per interal
    int     maxCountIndex[MAX_PULSE_COUNTERS];                  // year time index of the maximum (1st if there are more)
    int     maxPower[MAX_PULSE_COUNTERS];                       // maximum power value per interval in dWatt
    int     maxPowerIndex[MAX_PULSE_COUNTERS];                  // year time index of the maximum power (1st if there are more); = maxCountIndex
    int     numberOfActivityRecords[MAX_PULSE_COUNTERS];        // number of records that have a value>0
    bool    error;                                              // true if an error occurred
} fiveMinuteStats_t;


typedef struct
{
    int     day;
    int     month;
    int     year;
    double  energy[MAX_PULSE_COUNTERS];
    double  maxPower[MAX_PULSE_COUNTERS];
    int     maxPowerIndex[MAX_PULSE_COUNTERS];
    int     minutesActive[MAX_PULSE_COUNTERS];
} dayRecord_t;

class DataStore
{
private:
    Log                         logger {"datastore"};
    static DataStore*           theInstance;
    MYSQL                       database;
    MYSQL_RES*                  queryResult;  
    MYSQL*                      handle;
    MYSQL*                      newHandle;
    char                        queryString[QUERYSTRINGSIZE];
    char                        dateString[20];
    char                        timeString1[20];
    char                        timeString2[20];
    char                        timeString3[20];
    fiveMinuteMeasurement_t     measurements[INTERVALS_PER_DAY*DAYS_PER_YEAR];  
      
                                DataStore                   ();
    bool                        checkMysql                  ();   
    unsigned int                query                       (char* queryString);
    
                                
public:

    static DataStore*           getInstance                 ();
    
    bool                        storeFiveMinuteValue        (fiveMinuteMeasurement_t* measurement);
    
    void                        getFiveMinuteValue          (int timeIndex, int year, fiveMinuteMeasurement_t* measurement);
    void                        sumFiveMinuteValues         (int timeStartIndex, int year, int numberOfRecords, fiveMinuteStats_t* statistics);
                                                                        
    bool                        storeDayValue               (dayRecord_t* dayRecord);
    bool                        storeInstanteneousPowerMax  (instantMax_t* maxs);

    bool                        openDatabase                ();
    void                        closeDatabase               ();   
    void                        getDatabaseVersion          ();                      


};


