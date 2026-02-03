/**************************************************************************************************\
*
* Simulation.h
*
* Simulates power usage and production
*
\**************************************************************************************************/
#ifndef SIMULATION_H
#define SIMULATION_H

#include "Log.h"
#include "Clock.h"

#define NORMALTARIFF_MINHOUR   6
#define NORMALTARIFF_MAXHOUR   18

typedef struct
{
    int hour;                                   // Start of sim intervaL
    int minute;
    int seconds;
    int pulseMeterPower[MAX_PULSE_COUNTERS];    // Pulse meter power in W per interval
    int grossPowerUsage;                        // Gross usage power in W per interval
    int gasFlow;                                // Gas flow in l per hour
} sim_t;



class Simulation
{
private:
    Log                 logger {"simulation"};
    static Simulation*  theInstance;
    static sim_t        powers[QUARTERS_PER_DAY];
    int                 previousPowerIndex;
    Configuration*      config;
    Clock*              clock;
    char                printBuffer[20];

    solarTime_t         previousTime;
    sim_t               powersCurrent;
    double              powerNetPower;
    double              powerImport;
    double              powerExport;
    double              energyImportLow;
    double              energyImportNormal;
    double              energyExportLow;
    double              energyExportNormal;
    double              volumeGas;
    bool                normalTariff;
    static char         simulationReading[];  // A simulated Smart Meter reading from P1



                        Simulation();
    void                findSimValueByTime(int hour, int minute, int second);
    void                updateSmartMeterMessage(solarTime_t* time);
    void                readSimulatedMeterFile();
    void                writeSimulatedMeterFile();

public:
                        ~Simulation();
    static Simulation*  getInstance();
    void                process();
    double              getPulsePower(int powerMeter);
    double              getGrossPowerUsage();
    double              getNetPowerUsage();
    double              getNetImportPower();
    double              getNetExportPower();
    double              getEnergyImportLow();
    double              getEnergyImportNormal();
    double              getEnergyExportLow();
    double              getEnergyExportNormal();
    bool                isNormalTariff();
    char*               getSmartMeterMessage();
    void                dumpCurrentPowersAndEnergies();

};

#endif