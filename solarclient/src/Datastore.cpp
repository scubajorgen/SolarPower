/**************************************************************************************************\
*
* Datastore.cpp
*
* All about storing persistently the data and processing where required
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#include "common.h"
#include "Datastore.h"
#include "Configuration.h"

#define FIVEMINUTEINDEX_IX                      0
#define FIVEMINUTEINDEX_DATETIME                1
#define FIVEMINUTEINDEX_TIMEINDEX               2
#define FIVEMINUTEINDEX_YEAR                    3
#define FIVEMINUTEINDEX_PULSE1                  4
#define FIVEMINUTEINDEX_PULSEPOWER1             5
#define FIVEMINUTEINDEX_PULSEMAXPOWER1          6
#define FIVEMINUTEINDEX_PULSEMETER1             7
#define FIVEMINUTEINDEX_PULSE2                  8
#define FIVEMINUTEINDEX_PULSEPOWER2             9
#define FIVEMINUTEINDEX_PULSEMAXPOWER2          10
#define FIVEMINUTEINDEX_PULSEMETER2             11
#define FIVEMINUTEINDEX_PULSE3                  12
#define FIVEMINUTEINDEX_PULSEPOWER3             13
#define FIVEMINUTEINDEX_PULSEMAXPOWER3          14
#define FIVEMINUTEINDEX_PULSEMETER3             15
#define FIVEMINUTEINDEX_ELECTRICITYIMPORTLOW    16
#define FIVEMINUTEINDEX_ELECTRICITYIMPORTNORMAL 17
#define FIVEMINUTEINDEX_ELECTRICITYEXPORTLOW    18
#define FIVEMINUTEINDEX_ELECTRICITYEXPORTNORMAL 19
#define FIVEMINUTEINDEX_GASIMPORT               20
#define FIVEMINUTEINDEX_GROSSPOWER              21
#define FIVEMINUTEINDEX_NETPOWRE                22


#define DAYVALUEINDEX_IX                        0
#define DAYVALUEINDEX_DATE                      1
#define DAYVALUEINDEX_ENERGY1                   2
#define DAYVALUEINDEX_MAXPOWER1                 3
#define DAYVALUEINDEX_MAXPOWERINDEX1            4
#define DAYVALUEINDEX_INSTANTMAXPOWER1          5
#define DAYVALUEINDEX_INSTANTMAXPOWERTIME1      6
#define DAYVALUEINDEX_MINUTESACTIVE1            7
#define DAYVALUEINDEX_ENERGY2                   8
#define DAYVALUEINDEX_MAXPOWER2                 9
#define DAYVALUEINDEX_MAXPOWERINDEX2            10
#define DAYVALUEINDEX_INSTANTMAXPOWER2          11
#define DAYVALUEINDEX_INSTANTMAXPOWERTIME2      12
#define DAYVALUEINDEX_MINUTESACTIVE2            13
#define DAYVALUEINDEX_ENERGY3                   14
#define DAYVALUEINDEX_MAXPOWER3                 15
#define DAYVALUEINDEX_MAXPOWERINDEX3            16
#define DAYVALUEINDEX_INSTANTMAXPOWER3          17
#define DAYVALUEINDEX_INSTANTMAXPOWERTIME3      18
#define DAYVALUEINDEX_MINUTESACTIVE3            19




/******************************************************************************\
* Variables
\******************************************************************************/
DataStore* DataStore::theInstance=NULL;


/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
DataStore::DataStore()
{
    handle=NULL;
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of this class (Singleton)
*
\******************************************************************************/
DataStore* DataStore::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new DataStore();
    }
    return theInstance;
}

/*************************************************************
* Checks whether mysql server is alive
*************************************************************/
bool DataStore::checkMysql()
{
  int               result;
  bool              error;

  result=mysql_ping(&database);

  if (result!=0)
  {
    logger.logError("Mysql server appears to be down");
    error=true;
  }
  else
  {
      error=false;
  }
  return error;
}

/*************************************************************
* Opens the database
*************************************************************/
bool DataStore::openDatabase(void)
{
    bool           error;
    Configuration* config;

    config=Configuration::getInstance();

    error=false;

    newHandle=mysql_init(&database);
    if (newHandle==NULL)
    {
        logger.logError("Error creating database handle");
        error=true;
    }

    if (newHandle!=&database)
    {
        logger.logError("Database error: Another handle");
        error=true;
    }

    handle=mysql_real_connect(&database,
                              config->getDatabaseHost(),
                              config->getDatabaseUser(),
                              config->getDatabasePassword(),
                              config->getDatabaseName(), 0, NULL, 0);

    if (handle==NULL)
    {
        logger.logError("Error opening database");
        error=true;
    }
    else
    {
        error=checkMysql();
    }

    if (handle!=&database)
    {
        logger.logError("Database error: Another handle");
    }

    return error;
}

/************************************************************\
*
\************************************************************/
void DataStore::getDatabaseVersion()
{
    unsigned long   version;

    if (handle!=NULL)
    {
        version=mysql_get_server_version(&database);
        logger.logInfo("Mysql server version: %ld.%ld.%ld",
               version/10000, (version%10000)/100, version%100);
    }
}

/************************************************************\
*
\************************************************************/
unsigned int DataStore::query(char* queryString)
{
    int result;
    unsigned int fields;

    if (handle!=NULL)
    {
        result=mysql_real_query(handle, queryString, (unsigned int) strlen(queryString));

        if (result==0)
        {
            queryResult=mysql_store_result(handle);
            if (queryResult==NULL)
            {
                fields=0;
            }
            else
            {
                fields=mysql_field_count(handle);
            }
        }
        else
        {
            logger.logError("Error querying");
            fields=0;
        }
    }
    else
    {
        logger.logError("Database not open when querying");
        fields=0;
    }

    return fields;

}



/************************************************************\
*
\************************************************************/
void DataStore::closeDatabase(void)
{
    if (handle!=NULL)
    {
        mysql_close(handle);
        handle=NULL;
    }
}


/******************************************************************************\
*
* This method stores a measurement
*
\******************************************************************************/
bool DataStore::storeFiveMinuteValue(fiveMinuteMeasurement_t* mmt)
{
    MYSQL_ROW       row;
    bool            error;

    sprintf(queryString, "SELECT * FROM solarenergyfiveminutes WHERE timeindex=%d AND year=%d", mmt->timeIndex, mmt->year);

    query(queryString);

    row=mysql_fetch_row(queryResult);

    if (row==0)
    {
        logger.logDebug("Inserting 5 minute measurement for %04d-%02d-%02d %02d:%02d:%02d", 
                         mmt->datetime.year+2000, mmt->datetime.month, mmt->datetime.day, mmt->datetime.hour, mmt->datetime.minute, mmt->datetime.second);
        snprintf(queryString, QUERYSTRINGSIZE,
                 "INSERT INTO solarenergyfiveminutes (datetime, timeindex, year, "
                 "pulse1, pulsepower1, pulsemaxpower1, pulsemeter1, "
                 "pulse2, pulsepower2, pulsemaxpower2, pulsemeter2, "
                 "pulse3, pulsepower3, pulsemaxpower3, pulsemeter3, "
                 "electricityimportlow, electricityimportnormal, electricityexportlow, electricityexportnormal, gasimport, "
                 "grosspower, netpower) VALUES ("
                 "'%04d-%02d-%02d %02d:%02d:%02d', %d, %d,"
                 "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
                 "%d, %d, %d, %d, %d, %d, %d"
                 ")",
                mmt->datetime.year+2000, mmt->datetime.month, mmt->datetime.day, mmt->datetime.hour, mmt->datetime.minute, mmt->datetime.second, mmt->timeIndex, mmt->year,
                mmt->pulse[0], mmt->pulsePower[0], mmt->pulseMaxPower[0], mmt->pulseMeter[0],
                mmt->pulse[1], mmt->pulsePower[1], mmt->pulseMaxPower[1], mmt->pulseMeter[1],
                mmt->pulse[2], mmt->pulsePower[2], mmt->pulseMaxPower[2], mmt->pulseMeter[2],
                mmt->electricityImportLow, mmt->electricityImportNormal, mmt->electricityExportLow, mmt->electricityExportNormal, mmt->gasImport,
                mmt->grossPower, mmt->netPower
                );
        query(queryString);
        error=false;
    }
    else
    {
        // Aparently the record already exists. In that case it is not necessary to update it...
        error=true;
    }

    return error;
}

/******************************************************************************\
*
* This method retrieves a five minute measurement from the database
* NOT USED...
*
\******************************************************************************/
void DataStore::getFiveMinuteValue(int timeIndex, int year, fiveMinuteMeasurement_t* measurement)
{
    MYSQL_ROW       row;
//    MYSQL_RES*      res;
    int             numRows;

    snprintf(queryString, QUERYSTRINGSIZE, "SELECT * FROM solarenergyfiveminutes WHERE timeindex=%d and year=%d;", timeIndex, year);

    query(queryString);

    /*res=*/mysql_use_result(handle);

    measurement->timeIndex                  =timeIndex;
    measurement->year                       =year;
    measurement->pulse[0]                   =INVALID_MEASUREMENT;
    measurement->pulsePower[0]              =INVALID_MEASUREMENT;
    measurement->pulse[1]                   =INVALID_MEASUREMENT;
    measurement->pulsePower[1]              =INVALID_MEASUREMENT;
    measurement->pulse[2]                   =INVALID_MEASUREMENT;
    measurement->pulsePower[2]              =INVALID_MEASUREMENT;
    measurement->electricityImportLow       =INVALID_MEASUREMENT;
    measurement->electricityImportNormal    =INVALID_MEASUREMENT;
    measurement->electricityExportLow       =INVALID_MEASUREMENT;
    measurement->electricityExportNormal    =INVALID_MEASUREMENT;
    measurement->gasImport                  =INVALID_MEASUREMENT;

    numRows=mysql_num_rows(queryResult);
    if (numRows==1)
    {
        row=mysql_fetch_row(queryResult);
        if (row!=0)
        {
            measurement->timeIndex              =atoi(row[FIVEMINUTEINDEX_TIMEINDEX]);
            measurement->year                   =atoi(row[FIVEMINUTEINDEX_YEAR]);
            measurement->pulse[0]               =atoi(row[FIVEMINUTEINDEX_PULSE1]);
            measurement->pulsePower[0]          =atoi(row[FIVEMINUTEINDEX_PULSEPOWER1]);
            measurement->pulse[1]               =atoi(row[FIVEMINUTEINDEX_PULSE2]);
            measurement->pulsePower[1]          =atoi(row[FIVEMINUTEINDEX_PULSEPOWER2]);
            measurement->pulse[2]               =atoi(row[FIVEMINUTEINDEX_PULSE3]);
            measurement->pulsePower[2]          =atoi(row[FIVEMINUTEINDEX_PULSEPOWER3]);
            measurement->electricityImportLow   =atoi(row[FIVEMINUTEINDEX_ELECTRICITYIMPORTLOW]);
            measurement->electricityImportNormal=atoi(row[FIVEMINUTEINDEX_ELECTRICITYIMPORTNORMAL]);
            measurement->electricityExportLow   =atoi(row[FIVEMINUTEINDEX_ELECTRICITYEXPORTLOW]);
            measurement->electricityExportNormal=atoi(row[FIVEMINUTEINDEX_ELECTRICITYEXPORTNORMAL]);
            measurement->gasImport              =atoi(row[FIVEMINUTEINDEX_GASIMPORT]);
        }
        else
        {
            logger.logError("Error processing five minute data from datase");
        }
    }
    else
    {
        logger.logError("Error processing five minute data from datase: to many rows");
    }
}

/******************************************************************************\
*
* This method retrieves and sums a five minute measurement from the database
* timeStartIndex   Time Index of the records to sum
* numberOfRecords  Last time index to sum is defined by this: <timeStartIndex+numberOfRecords
* *statistics      Will hold the statistics
*
\******************************************************************************/
void DataStore::sumFiveMinuteValues(int timeStartIndex, int year, int numberOfRecords, fiveMinuteStats_t* statistics)
{
    MYSQL_ROW       row;
    int             numRows;
    bool            error;
    int             yearValue;
    int             timeIndexValue;

    snprintf(queryString, QUERYSTRINGSIZE,
                          "SELECT * FROM solarenergyfiveminutes WHERE timeindex>=%d AND timeindex<%d AND year=%d;",
                          timeStartIndex,
                          timeStartIndex+numberOfRecords,
                          year);

    query(queryString);

    numRows=(int)mysql_num_rows(queryResult);

    // Check number of rows >0 and not larger than numberOfRecords (in that case: double values!)
    if ((numRows<=0) || (numRows>numberOfRecords))
    {
        logger.logError("No records for %d %d-%d", year, timeStartIndex, timeStartIndex+numberOfRecords);
        error           =true;
    }
    else
    {
        logger.logDebug("Summing %d %d-%d", year, timeStartIndex, timeStartIndex+numberOfRecords);
        statistics->year=year;

        /************************************************************\
        // Reset statistics
        \************************************************************/
        for (int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
        {
            statistics->pulseCountSum[counterNo]           =0;
            statistics->pulsePowerSum[counterNo]           =0;
            statistics->numberOfRecordsInSum[counterNo]    =0;
            statistics->maxCount[counterNo]                =0;
            statistics->maxCountIndex[counterNo]           =-1;
            statistics->maxPower[counterNo]                =0;
            statistics->maxPowerIndex[counterNo]           =-1;
            statistics->numberOfActivityRecords[counterNo] =0;
        }

        /************************************************************\
        // first copy the records
        \************************************************************/
        for (int i=0; i<numRows; i++)
        {
            // get the row
            row=mysql_fetch_row(queryResult);
            // if the row is fetched, process it
            if (row!=0)
            {
                measurements[i].year            =atoi(row[FIVEMINUTEINDEX_YEAR]);
                measurements[i].pulse[0]        =atoi(row[FIVEMINUTEINDEX_PULSE1]);
                measurements[i].pulse[1]        =atoi(row[FIVEMINUTEINDEX_PULSE2]);
                measurements[i].pulse[2]        =atoi(row[FIVEMINUTEINDEX_PULSE3]);
                measurements[i].pulsePower[0]   =atoi(row[FIVEMINUTEINDEX_PULSEPOWER1]);
                measurements[i].pulsePower[1]   =atoi(row[FIVEMINUTEINDEX_PULSEPOWER2]);
                measurements[i].pulsePower[2]   =atoi(row[FIVEMINUTEINDEX_PULSEPOWER3]);
                measurements[i].timeIndex       =atoi(row[FIVEMINUTEINDEX_TIMEINDEX]);
            }
            else
            {
                logger.logError("Error fetching database row");
                measurements[i].year            =INVALID_YEAR;
                measurements[i].pulse[0]        =INVALID_MEASUREMENT;
                measurements[i].pulse[1]        =INVALID_MEASUREMENT;
                measurements[i].pulse[2]        =INVALID_MEASUREMENT;
                measurements[i].pulsePower[0]   =INVALID_MEASUREMENT;
                measurements[i].pulsePower[1]   =INVALID_MEASUREMENT;
                measurements[i].pulsePower[2]   =INVALID_MEASUREMENT;
                measurements[i].timeIndex       =-1;
            }
        }

        /************************************************************\
        // parse the records and count the pulses
        \************************************************************/
        error=false;
        for (int i=0; i<numRows && !error; i++)
        {
            yearValue       =measurements[i].year;
            timeIndexValue  =measurements[i].timeIndex;

            for (int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
            {
                INT32 pulseValue    =measurements[i].pulse[counterNo];
                INT32 pulsePower    =measurements[i].pulsePower[counterNo];


                // check if the pulse and year value are sensible values
                if ((yearValue!=INVALID_YEAR) && (pulseValue!=INVALID_MEASUREMENT))
                {
                    // Add the pulse value to the sum
                    statistics->pulseCountSum[counterNo]+=pulseValue;
                    statistics->pulsePowerSum[counterNo]+=pulsePower;
                    (statistics->numberOfRecordsInSum[counterNo])++;

                    // Request at least 2 or more pulses per time interval
                    // Was >0, but EnergyManagment plug usage is counted as well
                    if (pulseValue>1)
                    {
                        statistics->numberOfActivityRecords[counterNo]++;
                    }

                    // OBSOLETE
                    // Check if the maximum count value is exceeded. If so, update maximum
                    if (pulseValue>statistics->maxCount[counterNo])
                    {
                        statistics->maxCount[counterNo]     =pulseValue;
                        statistics->maxCountIndex[counterNo]=timeIndexValue;
                    }

                    // Check if the maximum power value is exceeded. If so, update maximum
                    if (pulsePower>statistics->maxPower[counterNo])
                    {
                        statistics->maxPower[counterNo]     =pulsePower;
                        statistics->maxPowerIndex[counterNo]=timeIndexValue;
                    }
                }
            }
        }
    }
    if (error)
    {
        statistics->error   =true;
    }
    else
    {
        for (int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
        {
            logger.logDebug("Stats %d %d: %d counts, %d dW, %d records", 
                             timeStartIndex, year, statistics->pulseCountSum[counterNo], statistics->pulsePowerSum[counterNo], statistics->numberOfRecordsInSum[counterNo]);
            if (statistics->maxCountIndex[counterNo]!=statistics->maxPowerIndex[counterNo])
            {
                logger.logError("INVALID INDICES");
            }
        }
        statistics->error   =false;
    }
}

/******************************************************************************\
*
* This method stores a day value, if it is not yet present; it updates 
* the record when present
*
\******************************************************************************/
bool DataStore::storeDayValue(dayRecord_t* dayRecord)
{
    int             numRows;
    bool            error;


    sprintf(dateString, "%4d-%02d-%02d",
                        dayRecord->year,
                        dayRecord->month,
                        dayRecord->day);

    snprintf(queryString, QUERYSTRINGSIZE, "SELECT * FROM solarenergyday WHERE date=\"%s\"", dateString);

    query(queryString);

    numRows=mysql_num_rows(queryResult);
    if (numRows==0)
    {
        logger.logDebug("Inserting solarenergyday: adding energy and interval max power for %s", dateString);
        snprintf(queryString, QUERYSTRINGSIZE,
                             "INSERT INTO solarenergyday  (date, "
                             "energy1, maxpower1, maxpowerindex1, minutesactive1, instantmaxpower1, instantmaxpowertime1, "
                             "energy2, maxpower2, maxpowerindex2, minutesactive2, instantmaxpower2, instantmaxpowertime2, "
                             "energy3, maxpower3, maxpowerindex3, minutesactive3, instantmaxpower3, instantmaxpowertime3) VALUES"
                             "('%s', %f, %f, %d, %d, -1, '00:00:00', %f, %f, %d, %d, -1, '00:00:00', %f, %f, %d, %d, -1, '00:00:00')",
                              dateString,
                              dayRecord->energy[0], dayRecord->maxPower[0], dayRecord->maxPowerIndex[0], dayRecord->minutesActive[0],
                              dayRecord->energy[1], dayRecord->maxPower[1], dayRecord->maxPowerIndex[1], dayRecord->minutesActive[1],
                              dayRecord->energy[2], dayRecord->maxPower[2], dayRecord->maxPowerIndex[2], dayRecord->minutesActive[2]);

        query(queryString);
        error=false;

    }
    else if (numRows==1)
    {
        logger.logDebug("Updating solarenergyday: adding energy and interval max power for %s", dateString);
        snprintf(queryString, QUERYSTRINGSIZE,
                             "UPDATE solarenergyday SET "
                             "energy1=%f, maxpower1=%f, maxpowerindex1=%d, minutesactive1=%d, "
                             "energy2=%f, maxpower2=%f, maxpowerindex2=%d, minutesactive2=%d, "
                             "energy3=%f, maxpower3=%f, maxpowerindex3=%d, minutesactive3=%d WHERE date='%s' ",
                              dayRecord->energy[0], dayRecord->maxPower[0], dayRecord->maxPowerIndex[0], dayRecord->minutesActive[0],
                              dayRecord->energy[1], dayRecord->maxPower[1], dayRecord->maxPowerIndex[1], dayRecord->minutesActive[1],
                              dayRecord->energy[2], dayRecord->maxPower[2], dayRecord->maxPowerIndex[2], dayRecord->minutesActive[2],
                              dateString);
        query(queryString);
        error=false;
    }
    else
    {
        error=true;
        logger.logError("Multiple day records found for %s", dateString);
    }
    return error;
}


/******************************************************************************\
*
* This method stores a the daily instanteneous power maximum
* In fact the minumum time between two adjacent pulses (in centiseconds) is stored
*
\******************************************************************************/
bool DataStore::storeInstanteneousPowerMax(instantMax_t* maxs)
{
    // Get the max power values; -1 if not available
    int power1          =maxs->power[0];
    int power2          =maxs->power[1];
    int power3          =maxs->power[2];
    sprintf(timeString1, "%02d:%02d:%02d", maxs->time[0].hour, maxs->time[0].minute, maxs->time[0].second);
    sprintf(timeString2, "%02d:%02d:%02d", maxs->time[1].hour, maxs->time[1].minute, maxs->time[1].second);
    sprintf(timeString3, "%02d:%02d:%02d", maxs->time[2].hour, maxs->time[2].minute, maxs->time[2].second);

    bool error              =false;
    // Take the date from the first max value.
    sprintf(dateString, "%4d-%02d-%02d", maxs->time[0].year+2000, maxs->time[0].month, maxs->time[0].day);

    snprintf(queryString, QUERYSTRINGSIZE, "SELECT * FROM solarenergyday WHERE date=\"%s\"", dateString);

    query(queryString);

    int numRows             =mysql_num_rows(queryResult);
    // Record not found: insert
    if (numRows==0)
    {
        logger.logDebug("Inserting solarenergyday: adding instantmaxpower for %s", dateString);
        snprintf(queryString, QUERYSTRINGSIZE,
                             "INSERT INTO solarenergyday  (date, "
                             "energy1, maxpower1, maxpowerindex1, minutesactive1, instantmaxpower1, instantmaxpowertime1, "
                             "energy2, maxpower2, maxpowerindex2, minutesactive2, instantmaxpower2, instantmaxpowertime2, "
                             "energy3, maxpower3, maxpowerindex3, minutesactive3, instantmaxpower3, instantmaxpowertime3) VALUES"
                             "('%s', "
                              "-1, -1, -1, -1, %d, '%s', -1, -1, -1, -1, %d, '%s', -1, -1, -1, -1, %d, '%s')",
                              dateString,
                              power1, timeString1, power2, timeString2, power3, timeString3);
        query(queryString);
        error=false;
    }
    // One record found: update
    else if (numRows==1)
    {
        // get the row
        MYSQL_ROW row       =mysql_fetch_row(queryResult);

        if((atof(row[DAYVALUEINDEX_INSTANTMAXPOWER1])==power1) &&
           (atof(row[DAYVALUEINDEX_INSTANTMAXPOWER2])==power2) &&
           (atof(row[DAYVALUEINDEX_INSTANTMAXPOWER3])==power3))
        {
            // The value has been stored before and has not been changed.
            error=true;
        }
        else
        {
            logger.logDebug("Updating solarenergyday: adding instantmaxpower for %s", dateString);
            snprintf(queryString, QUERYSTRINGSIZE,
                                "UPDATE solarenergyday SET "
                                "instantmaxpower1=%d, instantmaxpowertime1='%s', "
                                "instantmaxpower2=%d, instantmaxpowertime2='%s', "
                                "instantmaxpower3=%d, instantmaxpowertime3='%s' "
                                "WHERE date=\"%s\"",
                                power1, timeString1, power2, timeString2, power3, timeString3, dateString);
            query(queryString);
            error=false;
        }
    }
    else
    {
        logger.logError("Multiple day records exist for %s when storing instant max", dateString);
        error=true;
    }
    return error;
}
