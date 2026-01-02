/******************************************************************************\
*
* SmartMeter.cpp
* Readout for the P1 port of the smart meter
*
\******************************************************************************/
#if !defined(SMARTMETER_H)
#define SMARTMETER_H

#include <regex.h>
#include "SolarPublish.h"
#include "Clock.h"
#include "Log.h"
#include "Meter.h"

#define MAXP1MESSAGESIZE 8192
#define MAXMATCH         128
#define MAXERRORMSG      256

typedef struct
{
    solarTime_t dateTime;
    INT32       electricityImportLowWh;
    INT32       electricityImportNormalWh;
    INT32       electricityExportLowWh;
    INT32       electricityExportNormalWh;
    INT32       electricityImportW;
    INT32       electricityExportW;
    INT32       gasImport;
}
MeterReading_t;

class SmartMeter : Meter
{
private:
    // The one and only instance of this class
    Log                                     logger {"smartmeter"};
    static SmartMeter*                      theInstance;
    int                                     serial;
    bool                                    skipFirstMessage;
    bool                                    firstMessageProcessed;
    char                                    message[MAXP1MESSAGESIZE];
    char                                    errorMessage[MAXERRORMSG];
    int                                     messageIndex;
    regex_t                                 importLowKwh;
    regex_t                                 importHighKwh;
    regex_t                                 exportLowKwh;
    regex_t                                 exportHighKwh;
    regex_t                                 importKw;
    regex_t                                 exportKw;
    regex_t                                 gas;
    MeterReading_t                          currentReading;
    MeterReading_t                          startReading;
    char                                    matchResult[MAXMATCH];

    Clock*                                  clock;

    SmartMeter                              ();
    void    initialize                      ();
    void    deinitialize                    ();
    bool    processMatch                    (regex_t* regex, INT32* var);
    void    compileRegex                    (regex_t * r, const char * regex_text);
    char*   matchRegex                      (regex_t* r, const char* to_match);
    bool    processMessage                  ();
    void    dumpCurrentReading              ();
public:
    static SmartMeter* getInstance          ();
    void    getMeterReading                 (MeterReading_t* reading);
    void    process                         () override;
    INT32   getCurrentNetPower              ();
    void    startMeasurement                () override;
    void    retrieveAndRestartMeasurement   (Measurement_t *measurement) override;
    INT32   getCurrentImportPower           () override;
    INT32   getCurrentExportPower           () override;
};




#endif