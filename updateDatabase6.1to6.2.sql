/**************************************************************************************************\
*
* updatedatabase6.1to6.2.sql
*
* Updates the database when upgrading from version 6.1 to  6.2.
*
\**************************************************************************************************/

ALTER TABLE solarenergyfiveminutes ADD p1time                CHAR(20) AFTER pulsemeter3;

ALTER TABLE solarenergyfiveminutes ADD tariff                INT AFTER electricityexportnormal;
ALTER TABLE solarenergyfiveminutes ADD powerfailures         INT AFTER tariff;
ALTER TABLE solarenergyfiveminutes ADD powerfailureslong     INT AFTER powerfailures;
ALTER TABLE solarenergyfiveminutes ADD sagsl1                INT AFTER powerfailureslong;
ALTER TABLE solarenergyfiveminutes ADD sagsl2                INT AFTER sagsl1;
ALTER TABLE solarenergyfiveminutes ADD sagsl3                INT AFTER sagsl2;
ALTER TABLE solarenergyfiveminutes ADD swellsl1              INT AFTER sagsl3;
ALTER TABLE solarenergyfiveminutes ADD swellsl2              INT AFTER swellsl1;
ALTER TABLE solarenergyfiveminutes ADD swellsl3              INT AFTER swellsl2;
ALTER TABLE solarenergyfiveminutes ADD voltagel1             INT AFTER swellsl3;
ALTER TABLE solarenergyfiveminutes ADD voltagel2             INT AFTER voltagel1;
ALTER TABLE solarenergyfiveminutes ADD voltagel3             INT AFTER voltagel2;
ALTER TABLE solarenergyfiveminutes ADD currentl1             INT AFTER voltagel3;
ALTER TABLE solarenergyfiveminutes ADD currentl2             INT AFTER currentl1;
ALTER TABLE solarenergyfiveminutes ADD currentl3             INT AFTER currentl2;
ALTER TABLE solarenergyfiveminutes ADD actpowerimportl1      INT AFTER currentl3;
ALTER TABLE solarenergyfiveminutes ADD actpowerimportl2      INT AFTER actpowerimportl1;
ALTER TABLE solarenergyfiveminutes ADD actpowerimportl3      INT AFTER actpowerimportl2;
ALTER TABLE solarenergyfiveminutes ADD actpowerexportl1      INT AFTER actpowerimportl3;
ALTER TABLE solarenergyfiveminutes ADD actpowerexportl2      INT AFTER actpowerexportl1;
ALTER TABLE solarenergyfiveminutes ADD actpowerexportl3      INT AFTER actpowerexportl2;

ALTER TABLE solarenergyfiveminutes CHANGE COLUMN gasimport gasimport INT(32)    AFTER netpower;
ALTER TABLE solarenergyfiveminutes ADD gastime               CHAR(20)    AFTER gasimport;


ALTER TABLE solarenergyfiveminutes CHANGE COLUMN timeindex      timeindex INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN year           year INT;

ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulse1         pulse1 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsepower1    pulsepower1 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsemaxpower1 pulsemaxpower1 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsemeter1    pulsemeter1 INT;

ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulse2         pulse2 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsepower2    pulsepower2 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsemaxpower2 pulsemaxpower2 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsemeter2    pulsemeter2 INT;

ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulse3         pulse3 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsepower3    pulsepower3 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsemaxpower3 pulsemaxpower3 INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN pulsemeter3    pulsemeter3 INT;

ALTER TABLE solarenergyfiveminutes CHANGE COLUMN electricityimportlow       electricityimportlow INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN electricityimportnormal    electricityimportnormal INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN electricityexportlow       electricityexportlow INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN electricityexportnormal    electricityexportnormal INT;

ALTER TABLE solarenergyfiveminutes CHANGE COLUMN grosspower                 grosspower INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN netpower                   netpower INT;
ALTER TABLE solarenergyfiveminutes CHANGE COLUMN gasimport                  gasimport INT;

