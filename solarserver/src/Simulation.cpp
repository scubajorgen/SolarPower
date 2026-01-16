
/**************************************************************************************************\
*
* Simulation.h
*
* Simulates power usage and production
*
\**************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Configuration.h"
#include "Simulation.h"

Simulation* Simulation::theInstance=NULL;

// Simulated powers, per quarter of an hour
Sim_t Simulation::powers[QUARTERS_PER_DAY]=
{
    {0, 0, 0, {0, 100, 200}, 600, 500},
    {0, 15, 0, {0, 100, 200}, 600, 500},
    {0, 30, 0, {0, 100, 200}, 700, 500},
    {0, 45, 0, {0, 100, 200}, 700, 500},
    {1, 0, 0, {0, 100, 200}, 600, 500},
    {1, 15, 0, {0, 100, 200}, 600, 500},
    {1, 30, 0, {0, 100, 200}, 700, 500},
    {1, 45, 0, {0, 100, 200}, 700, 500},
    {2, 0, 0, {0, 100, 200}, 600, 500},
    {2, 15, 0, {0, 100, 200}, 600, 500},
    {2, 30, 0, {0, 100, 200}, 700, 500},
    {2, 45, 0, {0, 100, 200}, 700, 500},
    {3, 0, 0, {0, 100, 200}, 600, 500},
    {3, 15, 0, {0, 100, 200}, 600, 500},
    {3, 30, 0, {0, 100, 200}, 700, 500},
    {3, 45, 0, {0, 100, 200}, 700, 500},
    {4, 0, 0, {0, 100, 200}, 600, 500},
    {4, 15, 0, {0, 100, 200}, 600, 500},
    {4, 30, 0, {0, 100, 200}, 700, 500},
    {4, 45, 0, {0, 100, 200}, 700, 500},
    {5, 0, 0, {0, 100, 200}, 600, 500},
    {5, 15, 0, {0, 100, 200}, 600, 500},
    {5, 30, 0, {0, 100, 200}, 700, 500},
    {5, 45, 0, {0, 100, 200}, 700, 500},
    {6, 0, 0, {0, 0, 200}, 600, 500},
    {6, 15, 0, {377, 0, 200}, 600, 500},
    {6, 30, 0, {740, 0, 200}, 700, 500},
    {6, 45, 0, {1087, 0, 200}, 700, 500},
    {7, 0, 0, {1420, 0, 200}, 800, 5000},
    {7, 15, 0, {1738, 0, 200}, 800, 5000},
    {7, 30, 0, {2041, 0, 200}, 900, 1000},
    {7, 45, 0, {2330, 0, 200}, 900, 1000},
    {8, 0, 0, {2604, 0, 200}, 800, 1000},
    {8, 15, 0, {2862, 0, 200}, 800, 1000},
    {8, 30, 0, {3107, 0, 200}, 900, 1000},
    {8, 45, 0, {3336, 0, 200}, 900, 1000},
    {9, 0, 0, {3550, 0, 200}, 600, 1000},
    {9, 15, 0, {3750, 0, 200}, 600, 1000},
    {9, 30, 0, {3935, 0, 200}, 700, 1000},
    {9, 45, 0, {4105, 0, 200}, 700, 1000},
    {10, 0, 0, {4260, 0, 200}, 600, 1000},
    {10, 15, 0, {4401, 0, 200}, 600, 1000},
    {10, 30, 0, {4527, 0, 200}, 700, 1000},
    {10, 45, 0, {4638, 0, 200}, 700, 1000},
    {11, 0, 0, {4734, 0, 200}, 600, 1000},
    {11, 15, 0, {4815, 0, 200}, 600, 1000},
    {11, 30, 0, {4882, 0, 200}, 700, 1000},
    {11, 45, 0, {4933, 0, 200}, 700, 1000},
    {12, 0, 0, {4970, 0, 200}, 600, 1000},
    {12, 15, 0, {4993, 0, 200}, 600, 1000},
    {12, 30, 0, {5000, 0, 200}, 700, 1000},
    {12, 45, 0, {4993, 0, 200}, 700, 1000},
    {13, 0, 0, {4970, 0, 200}, 600, 1000},
    {13, 15, 0, {4933, 0, 200}, 600, 1000},
    {13, 30, 0, {4882, 0, 200}, 700, 1000},
    {13, 45, 0, {4815, 0, 200}, 700, 1000},
    {14, 0, 0, {4734, 0, 200}, 600, 1000},
    {14, 15, 0, {4638, 0, 200}, 600, 1000},
    {14, 30, 0, {4527, 0, 200}, 700, 1000},
    {14, 45, 0, {4401, 0, 200}, 700, 1000},
    {15, 0, 0, {4260, 0, 200}, 600, 1000},
    {15, 15, 0, {4105, 0, 200}, 600, 1000},
    {15, 30, 0, {3935, 0, 200}, 700, 1000},
    {15, 45, 0, {3750, 0, 200}, 700, 1000},
    {16, 0, 0, {3550, 0, 200}, 600, 1000},
    {16, 15, 0, {3336, 0, 200}, 600, 1000},
    {16, 30, 0, {3107, 0, 200}, 700, 1000},
    {16, 45, 0, {2862, 0, 200}, 700, 1000},
    {17, 0, 0, {2604, 0, 200}, 3300, 1000},
    {17, 15, 0, {2330, 0, 200}, 3300, 1000},
    {17, 30, 0, {2041, 0, 200}, 3400, 1000},
    {17, 45, 0, {1738, 0, 200}, 3400, 1000},
    {18, 0, 0, {1420, 0, 200}, 800, 3000},
    {18, 15, 0, {1087, 0, 200}, 800, 3000},
    {18, 30, 0, {740, 0, 200}, 900, 3000},
    {18, 45, 0, {377, 0, 200}, 900, 3000},
    {19, 0, 0, {0, 0, 200}, 800, 1000},
    {19, 15, 0, {0, 100, 200}, 800, 1000},
    {19, 30, 0, {0, 100, 200}, 900, 1000},
    {19, 45, 0, {0, 100, 200}, 900, 1000},
    {20, 0, 0, {0, 100, 200}, 800, 1000},
    {20, 15, 0, {0, 100, 200}, 800, 1000},
    {20, 30, 0, {0, 100, 200}, 900, 1000},
    {20, 45, 0, {0, 100, 200}, 900, 1000},
    {21, 0, 0, {0, 100, 200}, 800, 500},
    {21, 15, 0, {0, 100, 200}, 800, 500},
    {21, 30, 0, {0, 100, 200}, 900, 500},
    {21, 45, 0, {0, 100, 200}, 900, 500},
    {22, 0, 0, {0, 100, 200}, 800, 500},
    {22, 15, 0, {0, 100, 200}, 800, 500},
    {22, 30, 0, {0, 100, 200}, 900, 500},
    {22, 45, 0, {0, 100, 200}, 900, 500},
    {23, 0, 0, {0, 100, 200}, 600, 500},
    {23, 15, 0, {0, 100, 200}, 600, 500},
    {23, 30, 0, {0, 100, 200}, 700, 500},
    {23, 45, 0, {0, 100, 200}, 700, 500}
};

char Simulation::simulationReading[]=
                "/Ene5\\T210-D ESMR5.0\n"
                "\n"
                "1-3:0.2.8(50)\n"
                "0-0:1.0.0(260103085949W)\n"
                "0-0:96.1.1(4530303438303030303238353430383138)\n"
                "1-0:1.8.1(009645.859*kWh)\n"
                "1-0:1.8.2(009196.033*kWh)\n"
                "1-0:2.8.1(006595.405*kWh)\n"
                "1-0:2.8.2(014587.408*kWh)\n"
                "0-0:96.14.0(0001)\n"
                "1-0:1.7.0(00.762*kW)\n"
                "1-0:2.7.0(00.000*kW)\n"
                "0-0:96.7.21(01063)\n"
                "0-0:96.7.9(00026)\n"
                "1-0:99.97.0(8)(0-0:96.7.19)(250828124710S)(0000011426*s)(221017143729S)(0000004864*s)(220521164328S)(0000005946*s)(201020103607S)"
                "(0000003494*s)(200216132044W)(0000013110*s)(200216093038W)(0000027019*s)(191111115906W)(0000003380*s)(190612174024S)(0000008060*s)\n"
                "1-0:32.32.0(00009)\n"
                "1-0:52.32.0(00010)\n"
                "1-0:72.32.0(00010)\n"
                "1-0:32.36.0(00000)\n"
                "1-0:52.36.0(00000)\n"
                "1-0:72.36.0(00000)\n"
                "0-0:96.13.0()\n"
                "1-0:32.7.0(228.0*V)\n"
                "1-0:52.7.0(228.0*V)\n"
                "1-0:72.7.0(230.0*V)\n"
                "1-0:31.7.0(000*A)\n"
                "1-0:51.7.0(002*A)\n"
                "1-0:71.7.0(000*A)\n"
                "1-0:21.7.0(00.120*kW)\n"
                "1-0:41.7.0(00.562*kW)\n"
                "1-0:61.7.0(00.079*kW)\n"
                "1-0:22.7.0(00.000*kW)\n"
                "1-0:42.7.0(00.000*kW)\n"
                "1-0:62.7.0(00.000*kW)\n"
                "0-1:24.1.0(003)\n"
                "0-1:96.1.0(4730303732303034303539333133373230)\n"
                "0-1:24.2.1(260103085500W)(04717.700*m3)\n"
                "!09D9";

/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
Simulation::Simulation()
{
    logger.logInfo("Simulation Started");
    config              =Configuration::getInstance();
    clock               =Clock::getInstance();

    // Resent kWh & gas meters to 0, then try to read from file
    energyImportLow     =0.0;
    energyImportNormal  =0.0;
    energyExportLow     =0.0;
    energyExportNormal  =0.0;
    volumeGas           =0.0;
    readSimulatedMeterFile();

    powersCurrent.pulseMeterPower[0]=0;
    powersCurrent.pulseMeterPower[1]=0;
    powersCurrent.pulseMeterPower[2]=0;
    powersCurrent.grossPowerUsage   =0;
    powersCurrent.gasFlow           =0;

    previousPowerIndex  =-1;
    clock->getTime(&previousTime);
}

/******************************************************************************\
*
* Destructor
*
\******************************************************************************/
Simulation::~Simulation()
{
    logger.logInfo("Teminating simulation");
    writeSimulatedMeterFile();
}

/******************************************************************************\
*
* Runs the simulation process, exectue each ~second
*
\******************************************************************************/
void Simulation::process()
{
    // Estimate current electricity powers and gas flow
    solarTime_t currentTime;
    clock->getTime(&currentTime);
    double secondsPassed    =currentTime.epoch-previousTime.epoch;
    findSimValueByTime(currentTime.hour, currentTime.minute, currentTime.second);

    // Current net power usage
    double production=0;
    for(int counterNo=0; counterNo<MAX_PULSE_COUNTERS; counterNo++)
    {
        if (config->getPulseMeterUsage(counterNo)==USAGE_PRODUCTION)
        {
            production+=powersCurrent.pulseMeterPower[counterNo];
        }
    }
    powerNetPower=powersCurrent.grossPowerUsage-production;

    // Get net electricity import and export power (at least one of them is 0 Wh)
    powerImport=powerNetPower>=0? powerNetPower: 0; 
    powerExport=powerNetPower<0 ?-powerNetPower: 0; 

    // Calculate kWh and m3 energy meters and create 
    // the P1 smart meter message that corresponds to current situation
    bool normalTariff=currentTime.hour>=NORMALTARIFF_MINHOUR && currentTime.hour<NORMALTARIFF_MAXHOUR;
    
    if (normalTariff)
    {
        energyImportNormal  +=powerImport*secondsPassed/SECONDS_PER_HOUR/WATT_PER_KILOWATT;
        energyExportNormal  +=powerExport*secondsPassed/SECONDS_PER_HOUR/WATT_PER_KILOWATT;
    }
    else
    {
        energyImportLow     +=powerImport*secondsPassed/SECONDS_PER_HOUR/WATT_PER_KILOWATT;
        energyExportLow     +=powerExport*secondsPassed/SECONDS_PER_HOUR/WATT_PER_KILOWATT;
    }

    volumeGas               +=powersCurrent.gasFlow*secondsPassed/SECONDS_PER_HOUR/LITER_PER_M3;
    updateSmartMeterMessage();

    previousTime            =currentTime;
    //dumpCurrentPowersAndEnergies();
}

/******************************************************************************\
*
* Returns the one and only instance of this class
*
\******************************************************************************/
Simulation* Simulation::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new (Simulation);
    }
    return theInstance;
}

/******************************************************************************\
*
* Helper: Returns the current active simulation value
*
\******************************************************************************/
void Simulation::findSimValueByTime(int hour, int minute, int second)
{
    int daySeconds                      =hour*SECONDS_PER_HOUR+minute*SECONDS_PER_MINUTE+second;
    int found                           =-1;
    for (unsigned int i=0; i<sizeof(powers) && found<0; i++)
    {
        int simSeconds      =powers[i].hour*SECONDS_PER_HOUR+powers[i].minute*SECONDS_PER_MINUTE+powers[i].seconds;
        if (daySeconds<=simSeconds)
        {
            found=i-1;
        }
    }
    if (found<0)
    {
        logger.logFatal("Error estimating simulation value");
    }

    // Interpolate power values
    int     i1                          =found;
    int     i2                          =(found+1)%sizeof(powers);
    int     seconds1                    =powers[i1].hour*SECONDS_PER_HOUR+powers[i1].minute*SECONDS_PER_MINUTE+powers[i1].seconds;
    int     seconds2                    =powers[i2].hour*SECONDS_PER_HOUR+powers[i2].minute*SECONDS_PER_MINUTE+powers[i2].seconds;
    powersCurrent.grossPowerUsage       =powers[i1].grossPowerUsage+
                                         (powers[i2].grossPowerUsage-powers[i1].grossPowerUsage)*(daySeconds-seconds1)/(seconds2-seconds1);
    for (int j=0; j<MAX_PULSE_COUNTERS; j++)
    {
        powersCurrent.pulseMeterPower[j]=powers[i1].pulseMeterPower[j]+
                                         (powers[i2].pulseMeterPower[j]-powers[i1].pulseMeterPower[j])*(daySeconds-seconds1)/(seconds2-seconds1);
    }
    powersCurrent.gasFlow               =powers[i1].gasFlow+
                                         (powers[i2].gasFlow-powers[i1].gasFlow)*(daySeconds-seconds1)/(seconds2-seconds1);
    powersCurrent.hour                  =hour;
    powersCurrent.minute                =minute;
    powersCurrent.seconds               =second;

    if (previousPowerIndex!=found)
    {
        logger.logInfo("Simulating: %02d:%02d:%02d pulse 1 %d W, pulse 2 %d W, pulse 3 %d W, gross power usage %d W, gas usage %d l/h",
                         powers[i1].hour, powers[i1].minute, powers[i1].seconds,
                         powersCurrent.pulseMeterPower[0], powersCurrent.pulseMeterPower[1], powersCurrent.pulseMeterPower[2], powersCurrent.grossPowerUsage, powersCurrent.gasFlow);
        previousPowerIndex=found;
    }
}

/******************************************************************************\
*
* This function fills in the right values in the smart meter message
*
\******************************************************************************/
void Simulation::updateSmartMeterMessage()
{
    sprintf(printBuffer, "%06.3f", powerImport/WATT_PER_KILOWATT);
    memcpy(simulationReading+240, printBuffer, 6);
    sprintf(printBuffer, "%06.3f", powerExport/WATT_PER_KILOWATT);
    memcpy(simulationReading+261, printBuffer, 6);

    sprintf(printBuffer, "%010.3f", energyImportNormal);
    memcpy(simulationReading+144, printBuffer, 10);

    sprintf(printBuffer, "%010.3f", energyImportLow   );
    memcpy(simulationReading+118, printBuffer, 10);

    sprintf(printBuffer, "%010.3f", energyExportNormal);
    memcpy(simulationReading+196, printBuffer, 10);

    sprintf(printBuffer, "%010.3f", energyExportLow   );
    memcpy(simulationReading+170, printBuffer, 10);

    sprintf(printBuffer, "%09.3f", volumeGas   );
    memcpy(simulationReading+1032, printBuffer, 9);
}

/******************************************************************************\
*
* Read energy kWh counters from file
*
\******************************************************************************/
void Simulation::readSimulatedMeterFile()
{
    char* filename=config->getSimMeterFileName();
    logger.logInfo("Reading smart meter values from file %s", filename);
    FILE *fptr;
    // Open file in reading mode
    fptr = fopen(filename, "r");
    if (fptr!=NULL)
    {
        // Write some text to the file
        fscanf (fptr, "%lf", &energyImportNormal);
        fscanf (fptr, "%lf", &energyImportLow);
        fscanf (fptr, "%lf", &energyExportNormal);
        fscanf (fptr, "%lf", &energyExportLow);
        fscanf (fptr, "%lf", &volumeGas);
        logger.logInfo("Read energy kWh counters from file %s ", filename);
        logger.logInfo("%lf kWh, %lf kWh, %lf kWh, %lf kWh, %lf m3", 
                       energyImportNormal, energyImportLow, energyExportNormal, energyExportLow, volumeGas, filename);
        // Close the file
        fclose(fptr);
    }
    else
    {
        logger.logError("Unable to energy kWh counters from file %s", filename);
    }
}

/******************************************************************************\
*
* Write current energy kWh counters to file
*
\******************************************************************************/
void Simulation::writeSimulatedMeterFile()
{
    char* filename=config->getSimMeterFileName();
    logger.logInfo("Storing energy kWh meter to file %s", filename);
    
    // Open file in writing mode
    FILE *fptr = fopen(filename, "w");
    if (fptr!=NULL)
    {
        // Write some text to the file
        fprintf(fptr, "%lf\n", energyImportNormal);
        fprintf(fptr, "%lf\n", energyImportLow);
        fprintf(fptr, "%lf\n", energyExportNormal);
        fprintf(fptr, "%lf\n", energyExportLow);
        fprintf(fptr, "%lf\n", volumeGas);
        // Close the file
        fflush(fptr);
        fclose(fptr);
    }
    else
    {
        logger.logError("Unable to open simulatior meter file %s", filename);
    }
}

/******************************************************************************\
*
* Returns the power measured by the inidicated pulse meter
*
\******************************************************************************/
double Simulation::getPulsePower(int powerMeter)
{
    return powersCurrent.pulseMeterPower[powerMeter];
}

/******************************************************************************\
*
* Returns the gross power usage of the household
*
\******************************************************************************/
double Simulation::getGrossPowerUsage()
{
    return powersCurrent.grossPowerUsage;
}

/******************************************************************************\
*
* Returns the net power usage of the household, i.e. gross power - production
*
\******************************************************************************/
double Simulation::getNetPowerUsage()
{
    return powerNetPower;
}

/******************************************************************************\
*
* Returns the current net import power
*
\******************************************************************************/
double Simulation::getNetImportPower()
{
    return powerImport;
}

/******************************************************************************\
*
* Returns the current net export power
*
\******************************************************************************/
double Simulation::getNetExportPower()
{
    return powerExport;
}

/******************************************************************************\
*
* Returns the current time is normal tariff (true) or low tariff (false)
*
\******************************************************************************/
bool Simulation::isNormalTariff()
{
    return normalTariff;
}

/******************************************************************************\
*
* Returns the import low counter in kWh
*
\******************************************************************************/
double Simulation::getEnergyImportLow()
{
    return energyImportLow;
}

/******************************************************************************\
*
* Returns the import normal counter in kWh
*
\******************************************************************************/
double Simulation::getEnergyImportNormal()
{
    return energyImportNormal;
}

/******************************************************************************\
*
* Returns the export low counter in kWh
*
\******************************************************************************/
double Simulation::getEnergyExportLow()
{
    return energyExportLow;
}

/******************************************************************************\
*
* Returns the export normal counter in kWh
*
\******************************************************************************/
double Simulation::getEnergyExportNormal()
{
    return energyExportNormal;
}

/******************************************************************************\
*
* Returns the export normal counter in kWh
*
\******************************************************************************/
char* Simulation::getSmartMeterMessage()
{
    return simulationReading;
}

/******************************************************************************\
*
* Print out current situation
*
\******************************************************************************/
void Simulation::dumpCurrentPowersAndEnergies()
{
    logger.logInfo("Powers: pulse 0: %d W 1: %d W 2 %d W",
                   powersCurrent.pulseMeterPower[0], powersCurrent.pulseMeterPower[1], powersCurrent.pulseMeterPower[2]);
    logger.logInfo("        gross %d W, net %lf W, import %lf W export %lf W",
                   powersCurrent.grossPowerUsage, powerNetPower, powerImport, powerExport);
    logger.logInfo("Energy: import normal %lf kWh, low %lf kWh", energyImportNormal, energyImportLow);
    logger.logInfo("        export normal %lf kWh, low %lf kWh", energyExportNormal, energyExportLow);
}