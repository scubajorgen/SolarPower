/**************************************************************************************************\
*
* Clock.h
*
* Time and timing functions
*
\**************************************************************************************************/

#ifndef CLOCK_H

#define CLOCK_H
#include <time.h>

#include "common.h"
#include "Log.h"


// From Server
#define MAXDAYSPERMONTH             31


#define MAXYEARTIMEINDEX            (366*INTERVALS_PER_DAY)

// From client
#define DAYS_PER_YEAR               366
#define MINUTES_PER_DAY             (24*60)
#define MINUTES_PER_HOUR            60


#define CET_OFFSET 1;

typedef struct
{
    INT16       year;
    INT8        month;
    INT8        day;
    INT8        hour;
    INT8        minute;
    INT8        second;
    INT8        centisecond;
    FLOAT64     epoch;
} solarTime_t;

class Clock
{

private:
    Log             logger {"clock"};
    static Clock*   theInstance;

    static INT16    monthDays[12];

    solarTime_t     currentTime;

                    Clock                       ();

public:
    static Clock*   getInstance                 ();
                    ~Clock                      ();

    // Solar Server functions
    void            setTime                     (solarTime_t* newSolarTime);
    void            getTime                     (solarTime_t* solarTime);

    void            getTimeString               (char* timeString);

    static INT32    calculateYearTimeIndex      (solarTime_t* solarTime);
    
    // Solar Client functions
    static  tm*     getTime                     ();

    static INT128   getNanoSeconds              ();
    
    static  int     calculateYearTimeIndex      (int day, int month, int hour, int minute);
    static  int     calculateYearTimeIndex      (int day, int month);
    
    static  void    calculateTime               (int timeIndex, int* day, int* month, int* hour, int* minute);
    
    static  int     substractDay                (int timeIndex);
};
#endif