/**************************************************************************************************\
*
* Clock.cpp
*
* Time and timing functions
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Clock.h"

/******************************************************************************\
* Variables
\******************************************************************************/
Clock*  Clock::theInstance=NULL;

INT16   Clock::monthDays[12]={31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor
*
\******************************************************************************/
Clock::Clock()
{
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
*  This method returns the one and only instance (Singleton)
*
\******************************************************************************/
Clock* Clock::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new Clock();
    }
    return theInstance;
}

/******************************************************************************\
*
* The destructor
*
\******************************************************************************/
Clock::~Clock()
{
}

/******************************************************************************\
* Old, Server functions
\******************************************************************************/
/******************************************************************************\
*
* This method updates the time in the I2C clock
*
\******************************************************************************/
void Clock::setTime(solarTime_t* newSolarTime)
{
    logger.logWarning("Deprecated function call clock::setTime - not effective");
}

/******************************************************************************\
*
* This method fetches the time from the I2C clock and returns it in the time
* variable. It returns GMT
*
\******************************************************************************/
void Clock::getTime(solarTime_t* solarTime)
{
    struct timespec now;
    struct tm *     nowGmt;
    
    clock_gettime(CLOCK_REALTIME, &now);
    
    nowGmt                  = gmtime(&(now.tv_sec));
    
    currentTime.year        =nowGmt->tm_year+1900;          //
    currentTime.month       =nowGmt->tm_mon+1;              // tm_mon [0..11], currentTime.month [1..12]
    currentTime.day         =nowGmt->tm_mday;
    currentTime.hour        =nowGmt->tm_hour;
    currentTime.minute      =nowGmt->tm_min;
    currentTime.second      =nowGmt->tm_sec;
    currentTime.centisecond =(now.tv_nsec/1e7L);
    currentTime.epoch       =now.tv_sec+(double)now.tv_nsec/1.0E9;
    *solarTime=currentTime;
}

/******************************************************************************\
*
* This method calculates the unique index of the given time in a year.
* The year is split in 5 minute periods. A leap year (366 days) is assumed.
*
\******************************************************************************/
INT32 Clock::calculateYearTimeIndex(solarTime_t* solarTime)
{
    // calculate full days
    int     days    =0;
    int     month   =solarTime->month-1;            // time->month is counted from 1, month is counted from 0
    for (int i=0; i<month; i++)
    {
        days+=monthDays[i];
    }
    days+=(solarTime->day-1);

    INT32   index   = (INT32)days*(INT32)INTERVALS_PER_DAY;
    index           +=(INT32)solarTime->hour*INTERVALS_PER_HOUR;
    index           +=(INT32)solarTime->minute/(INT32)MEASUREMENT_INTERVAL;

    return index;
}

/******************************************************************************\
*
* This method prints the current time into a string. The string should
* be passed and should be at least 23 characters long.
*
\******************************************************************************/
void Clock::getTimeString(char* timeString)
{
    getTime(&currentTime);

    sprintf(timeString, "%02d-%02d-%4d %02d:%02d:%02d.%02d",
                        currentTime.day,
                        currentTime.month,
                        currentTime.year,
                        currentTime.hour,
                        currentTime.minute,
                        currentTime.second,
                        currentTime.centisecond);
}

/******************************************************************************\
* New, Client functions
\******************************************************************************/
/******************************************************************************\
*
* This method retrieves the system time. It is stored in 'daytime' for use
* It is the GMT, hence not corrected for summertime/wintertime (it is always
* wintertime). The reason for this is that it is a constant timebase, the graphs
* will always be with respect to the same time (and not shift one hour in summer)
* The sun does not keep track of summer and wintertime....
*
\******************************************************************************/
tm* Clock::getTime()
{
    time_t timer=time(NULL);
    
    // get the greenwich mean time (GMT)
    struct tm* daytime=gmtime(&timer);

    return daytime;
}  

/******************************************************************************\
*
* Calculate the time index of the given time
* @param day      Day of the month. Ranges from 1-31!
* @param month    Mont. Ranges from 1-12!
* @param hour     Hour. Ranges from 0-23
* @param minute   Minute. Ranges from 0-59
*
\******************************************************************************/
int Clock::calculateYearTimeIndex(int day, int month, int hour, int minute)
{
    int index;

    // calculate full days
    int days=0;
    for (int i=0; i<month-1; i++)
    {
        days+=monthDays[i];
    }
    days+=(day-1);

    index =days*INTERVALS_PER_DAY;
    index+=hour*INTERVALS_PER_HOUR;
    index+=minute/MEASUREMENT_INTERVAL;

    return index;
}

/******************************************************************************\
*
* Calculate the time index of the given date
*
\******************************************************************************/
int Clock::calculateYearTimeIndex(int day, int month)
{
    return calculateYearTimeIndex(day, month, 0, 0);
}

/******************************************************************************\
*
* Calculate the time from the given time index
*
\******************************************************************************/
void Clock::calculateTime(int timeIndex, int* day, int* month, int* hour, int* minute)
{
    int days                =timeIndex/INTERVALS_PER_DAY;
    int remainingIntervals  =timeIndex%INTERVALS_PER_DAY;
    
    *month                  =1;
    int monthIndex          =0;
    bool exit                   =false;
    while ((monthIndex<12) && (!exit))
    {
        if (days>=monthDays[monthIndex])
        {
            days-=monthDays[monthIndex];
            *month+=1;
        }
        else
        {
            exit=true;
        }
        monthIndex++;
    }
    *day=days+1;
    
    *hour  =remainingIntervals/INTERVALS_PER_HOUR;
    *minute=MEASUREMENT_INTERVAL*(remainingIntervals%INTERVALS_PER_HOUR);
}

/******************************************************************************\
*
* Substract one day from the timeindex
*
\******************************************************************************/
int Clock::substractDay(int timeIndex)
{
    timeIndex-=INTERVALS_PER_DAY;
    if (timeIndex<0)
    {
        timeIndex+=DAYS_PER_YEAR*INTERVALS_PER_DAY;
    }
    return timeIndex;
}