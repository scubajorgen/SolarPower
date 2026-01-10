
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

Sim_t Simulation::powers[24]=
{
    { 0,  0,  0, { 100,  100,  100},  150, 0},
    { 1,  0,  0, { 200,  100,  100},  150, 0},
    { 2,  0,  0, { 300,  100,  100},  150, 0},
    { 3,  0,  0, { 400,  100,  100},  150, 0},
    { 4,  0,  0, { 500,  100,  100},  150, 0},
    { 5,  0,  0, { 600,  100,  100},  150, 0},
    { 6,  0,  0, { 700,  100,  100},  250, 0},
    { 7,  0,  0, { 800,  100,  100},  350, 0},
    { 8,  0,  0, { 900,  100,  100},  450, 0},
    { 9,  0,  0, {1000,  100,  100},  550, 0},
    {10,  0,  0, {1100,  100,  100},  450, 0},
    {11,  0,  0, {1200,  100,  100},  350, 0},
    {12,  0,  0, {1300,  100,  100},  350, 0},
    {13,  0,  0, {1200,  100,  100},  350, 0},
    {14,  0,  0, {1100,  100,  100},  350, 0},
    {15,  0,  0, {1000,  100,  100},  450, 0},
    {16,  0,  0, { 900,  100,  100},  550, 0},
    {17,  0,  0, { 800,  100,  100}, 1050, 0},
    {18,  0,  0, { 700,  100,  100}, 1150, 0},
    {19,  0,  0, { 800,  100,  100},  550, 0},
    {20,  0,  0, { 900,  100,  100},  450, 0},
    {21,  0,  0, { 000,  100,  100},  450, 0},
    {22,  0,  0, { 300,  100,  100},  450, 0},
    {23,  0,  0, { 200,  100,  100},  250, 0}
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
    previousPowerIndex  =-1;
    energyImportLow     =0.0;
    energyImportNormal  =0.0;
    energyExportLow     =0.0;
    energyExportNormal  =0.0;
    readSimulatedMeterFile();
    clock->getTime(&previousTime);
    previousTimeEpoch   =clock->getLastTimeAsEpoch();   
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
* Runs the simulation process, exectue each
*
\******************************************************************************/
void Simulation::process()
{
    solarTime_t currentTime;
    clock->getTime(&currentTime);
    double currentTimeEpoch =clock->getLastTimeAsEpoch();   
    double secondsPassed    =currentTimeEpoch-previousTimeEpoch;
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

    powerImport=powerNetPower>=0? powerNetPower: 0; 
    powerExport=powerNetPower<0 ?-powerNetPower: 0; 

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
    updateSmartMeterMessage();

    previousTime            =currentTime;
    previousTimeEpoch       =currentTimeEpoch;
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
    int daySeconds=hour*SECONDS_PER_HOUR+minute*SECONDS_PER_MINUTE+second;
    int found=-1;
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
    powersCurrent.hour                  =hour;
    powersCurrent.minute                =minute;
    powersCurrent.seconds               =second;

    if (previousPowerIndex!=found)
    {
        logger.logInfo("Simulating: pulse 1 %d W, pulse 2 %d W, pulse 3 %d W, gross power usage %d W",
                         powersCurrent.pulseMeterPower[0], powersCurrent.pulseMeterPower[1], powersCurrent.pulseMeterPower[2], powersCurrent.grossPowerUsage);
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
        fscanf (fptr, "%lf/n", &energyImportNormal);
        fscanf (fptr, "%lf/n", &energyImportLow);
        fscanf (fptr, "%lf/n", &energyExportNormal);
        fscanf (fptr, "%lf/n", &energyExportLow);
        fscanf (fptr, "%lf/n", &volumeGas);
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
    FILE *fptr;
    // Open file in writing mode
    fptr = fopen(filename, "w");
    if (fptr!=NULL)
    {
        // Write some text to the file
        fprintf(fptr, "%lf", energyImportNormal);
        fprintf(fptr, "%lf", energyImportLow);
        fprintf(fptr, "%lf", energyExportNormal);
        fprintf(fptr, "%lf", energyExportLow);
        fprintf(fptr, "%lf", volumeGas);
        // Close the file
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