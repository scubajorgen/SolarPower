/**************************************************************************************************\
*
* PulseCounter.h
*
* Pulse meter readout and processing. 
*
\**************************************************************************************************/
#include "IoPins.h"
#include "Clock.h"

#include "pthread.h"

#include "common.h"
#include "Log.h"
#include "Configuration.h"
#include "Meter.h"


#define COUNTTASK_STACKSIZE     1024         // in words


#define TENSECONDS              1000         // number of 10 ms periods in 10 seconds
#define ONESECOND               100          // number of 10 ms periods in 1 second

#define PULSE1                  0
#define PULSE2                  1
#define PULSE3                  2

typedef enum
{
    COUNTSTATE_IDLE,
    COUNTSTATE_LOWSTATE,
    COUNTSTATE_WAITFORHIGH,
    COUNTSTATE_HIGHSTATE,
    COUNTSTATE_WAITFORLOW
} countState_t;

class PulseCounter : Meter
{
private:
    Log                 logger {"pulsecount"};
    Configuration*      configuration;

    pulseMeterUsage_t   meterUsage;                         // Indicates whether pulse meter is a production meter (export) or consumption (import) or not used

    INT32               maxPower;                           // Maximum power measured during day
    INT32               maxPowerTimeDiff;
    solarTime_t         maxPowerTime;                       // Time of maximum power per day
    INT32               maxIntervalPower;                   // Maximum power measured during interval

    INT32               energyMeter;                        // Simulated energy meter, in Wh
    INT32               energyMeterCounts;                  // Pulses counted since previous reset
    INT32               energyMeterBase;                    // Value of energy meter during reset
    char*               energyMeterFileName;                // File to which meter value is stored

    int                 pulseId;                            // ID of the pulse meter; also used as array index; 0, 1, 2...

    countState_t        countState;                         // State machine state
    IoPins*             ioPins;
    int                 pulseValue;                         // Value of the pulse input
    bool                ledState;                           // state of the LED

    int                 currentPulseCounter;                // Interval pulse couter value

    INT32               timeDiff;
    INT32               power;                              // Power calculated when last pulse was received
    INT32               publishPower;                       // Power that was published. Corrected (asymptotically to 0) when no pulses are received

    bool                firstPulseReceived;                 // Bool indicating that the first pulse has been received

    Clock*              solarClock;
    solarTime_t         pulseTime;
    solarTime_t         previousPulseTime;
    solarTime_t         now;
    INT32               timeIndex;
    INT32               recordCount;
    INT32               recordsToBeAdded;

    long                pulseCount;
    long                ghostPulseCount;
    long                ghostDipCount;


    INT32                   calculatePower                  ();
    INT32                   calculateAverageIntervalPower   ();
    void                    estimateCurrentPower            ();
    void                    pulseReceived                   ();
    void                    initialiseEnergyMeter           ();
    void                    processEnergyMeter              ();
    void                    persistEnergyMeter              ();

public:
                            PulseCounter                    (int pulseId, pulseMeterUsage_t meterUsage, char* meterFile);

                            ~PulseCounter                   ();

    void                    setPulsesPerKwh                 (int pulsesPerKwh);
    void                    process                         () override;

    void                    startMeasurement                () override;
    void                    retrieveAndRestartMeasurement   (measurement_t *measurement) override;
    int                     getCounterValue                 ();

    void                    getCurrentPowerMax              (INT32 *timeDiff, INT32* power, solarTime_t *pulseTime);
    void                    resetCurrentPowerMax            ();

    INT32                   getPublishPower                 ();
    INT32                   getCurrentImportPower           () override;
    INT32                   getCurrentExportPower           () override;

    bool                    isProductionMeter               ();
    pulseMeterUsage_t       getMeterUsage                   ();
    void                    logStatus                       ();
};