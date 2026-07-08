/**************************************************************************************************\
*
* common.h
*
* Common defines
*
\**************************************************************************************************/
#ifndef COMMON_H
#define COMMON_H

#define VERSION                         "6.3"

// This define defines the interval of measurement in minutes
// Note: an hour should be dividable by this amount
// Hence: 1, 2, 3, 4, 5, 6, 10, 12, ... minutes
// Probably 5 is the only one that works...
#define MEASUREMENT_INTERVAL            5

// Minimum interval size in seconds (5 - 299 (=measurement interval))
// The larger this period, the more accurate the first measurement will be, since it will be scaled to the normal measurement interval.S
#define MINIMUM_INTERVAL_SIZE           30

#define DAYS_PER_YEAR                   366
#define HOURS_PER_DAY                   24
#define MINUTES_PER_DAY                 (24*60)
#define QUARTERS_PER_DAY                96
#define MINUTES_PER_HOUR                60
#define SECONDS_PER_DAY                 (24*3600)
#define SECONDS_PER_HOUR                3600
#define SECONDS_PER_MINUTE              60
#define MILLISECONDS_PER_SECOND         1000
#define MICROSECONDS_PER_SECOND         1000000
#define NANOSECONDS_PER_SECOND          1000000000

#define INTERVALS_PER_DAY               (MINUTES_PER_DAY/MEASUREMENT_INTERVAL)
#define INTERVALS_PER_HOUR              (MINUTES_PER_HOUR/MEASUREMENT_INTERVAL)


#define INVALID_MEASUREMENT             -1
#define INVALID_YEAR                    -1
#define INVALID_ENERGY                  -1.0;

// The number of pulses per watthour
#define DEFAULT_PULSES_PER_KILOWATTHOUR 2000

#define WATTHOUR_PER_KILOWATTHOUR       1000

#define DECIWATT_PER_WATT               10
#define WATT_PER_KILOWATT               1000
#define LITER_PER_M3                    1000

// Sample time in microseconds. This is the time between two samples of the pulse
// All processing should be well within this period. Measured in simulation: < 1000 us
#define SAMPLE_TIME                     10000

#define MAX_PULSE_COUNTERS              3

#define PUBLISHINTERVAL                 2

#define INT8        char
#define INT16       short
#define INT32       int
#define INT64       long
#define INT128      long long
#define FLOAT32     float
#define FLOAT64     double
#define FLOAT128    long double

#define RUNDIR          "/tmp/"
#define LOGFILE         "/var/log/Solar.log"
#define SERVERPIDFILE   "/tmp/SolarServer.pid"
#define CLIENTPIDFILE   "/tmp/SolarClient.pid"

typedef enum
{
    USAGE_NOTUSED    =0,
    USAGE_CONSUMPTION=1,
    USAGE_PRODUCTION =2
} pulseMeterUsage_t;

#endif