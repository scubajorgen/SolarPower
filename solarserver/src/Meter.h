/**************************************************************************************************\
*
* Meter.h
*
* Abstract class representing a meter
*
\**************************************************************************************************/
#if !defined(METER_H)
#define METER_H

#include "MeasurementStorage.h"

class Meter
{
public:
    virtual void            process                         ()=0;                               // Give processing time, every SAMPLE_TIME
    virtual void            startMeasurement                ()=0;                               // Reset/start measurement
    virtual void            retrieveAndRestartMeasurement   (measurement_t *measurement)=0;     // Retrieve measurement and restart
    virtual INT32           getCurrentImportPower           ()=0;                               // Retrieves import power
    virtual INT32           getCurrentExportPower           ()=0;                               // Retrieves export power
};

#endif