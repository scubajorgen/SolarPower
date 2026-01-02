/**************************************************************************************************\
*
* common.h
*
* Common defines
*
\**************************************************************************************************/

// This define defines the interval of measurement in minutes
// Note: an hour should be dividable by this amount
// Hence: 1, 2, 3, 4, 5, 6, 10, 12, ... minutes
// Probably 5 is the only one that works...
#define MEASUREMENT_INTERVAL            5

#define DAYS_PER_YEAR                   366
#define MINUTES_PER_DAY                 (24*60)
#define MINUTES_PER_HOUR                60

#define INTERVALS_PER_DAY               (MINUTES_PER_DAY/MEASUREMENT_INTERVAL)
#define INTERVALS_PER_HOUR              (MINUTES_PER_HOUR/MEASUREMENT_INTERVAL)


#define INVALID_MEASUREMENT             -1
#define INVALID_YEAR                    -1
#define INVALID_ENERGY                  -1.0;

// The number of pulses per watthour
#define DEFAULT_PULSES_PER_KILOWATTHOUR 2000

#define WATTHOUR_PER_KILOWATTHOUR       1000

#define DECIWATT_PER_WATT               10

// Sample time in microseconds. This is the time between two samples of the pulse
#define SAMPLE_TIME                     10000

#define MAX_PULSE_COUNTERS              3

#define INT8  char
#define INT16 short
#define INT32 int
#define INT64 long

#define RUNDIR          "/tmp/"
#define LOGFILE         "/var/log/Solar.log"
#define SERVERPIDFILE   "/tmp/SolarServer.pid"
#define CLIENTPIDFILE   "/tmp/SolarClient.pid"
