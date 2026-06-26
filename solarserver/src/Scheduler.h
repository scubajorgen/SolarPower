/******************************************************************************\
*
* Scheduler.h
* Scheduler for periodic tasks
*
\******************************************************************************/
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pthread.h"

#include "Log.h"
#include "Clock.h"
#include "IoPins.h"
#include "PulseCounter.h"
#include "MeasurementStorage.h"
#include "SmartMeter.h"
#include "SolarPublish.h"
#include "Simulation.h"


#define TENSECONDS              1000         // number of 10 ms periods in 10 seconds
#define ONESECOND               100          // number of 10 ms periods in 1 second

#define NETPOWER                100
#define GROSSPOWER              101

typedef enum
{
    MEASURESTATE_IDLE,
    MEASURESTATE_WAITING,
    MEASURESTATE_COLLECTED
} measureState_t;

class Scheduler
{
private:
    Log                 logger {"scheduler"};
    static Scheduler    *theInstance;

    pthread_t           threadId;                       // we use a thread for the scheduled task
    pthread_mutex_t     mutex;                          // and a mutex

    measureState_t      measureState;

    PulseCounter*       counters[MAX_PULSE_COUNTERS];

    bool                closeTask;
    bool                taskRunning;

    bool                led1State;

    IoPins*             ioPins;

    Clock*              solarClock;
    solarTime_t         pulseTime;
    solarTime_t         schedulerStartTime;
    solarTime_t         measuringStartTime;
    bool                firstMeasurement;

    INT32               startTimeIndex;
    INT32               previousTimeIndex;
    INT16               year;
    INT16               previousYear;

    // helpers measureStateMachine()
    INT32               timeIndex;
    bool                simulationMode;
    Simulation*         simulation;


    MeasurementStorage* measurementStorage;
    measurement_t       measurement;
    measurement_t       emptyMeasurement;
    maxPower_t          maxPower;

    SmartMeter*         smartMeter;
    meterReading_t      meterReading;

    SolarPublish*       solarPublish;

    friend void*            schedulerTask(void* param);

    Scheduler();

    void                    publishCounters                 ();
    void                    processMeters                   ();
    void                    measureStateMachine             ();

    void                    resetMeasurement                ();
    void                    storeAndResetMeasurement        ();
    void                    storeAndResetMaxPowerValues     ();

public:

    static Scheduler*       getInstance                     ();
                            ~Scheduler                      ();
    void                    start                           ();
    void                    stop                            ();

    void                    getCurrentPowerMax              (maxPower_t* maxPower);
    void                    resetCurrentPowerMax            ();
    void                    logStatus                       ();
};


#endif