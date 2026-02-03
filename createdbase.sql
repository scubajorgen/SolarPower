/**************************************************************************************************\
*
* createdb.sql
*
* Creates the required database. MySql/MariaDB
*
\**************************************************************************************************/

-- CREATE DATABASE energy;
-- USE energy;

CREATE TABLE energybill 
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  date                    DATETIME,
  emeter1                 DOUBLE,
  emeter1_offset          DOUBLE,
  emeter2                 DOUBLE,
  emeter2_offset          DOUBLE,
  emeter3                 DOUBLE,
  emeter3_offset          DOUBLE,
  emeter4                 DOUBLE,
  emeter4_offset          DOUBLE,
  solarmeter              DOUBLE,
  solarmeter_offset       DOUBLE,
  uchpproduction          DOUBLE,
  uchpproduction_offset   DOUBLE,
  uchpconsumption         DOUBLE,
  uchpconsumption_offset  DOUBLE,
  ebill                   DOUBLE,
  gasmeter                DOUBLE,
  gasmeter_offset         DOUBLE,
  gasbill                 DOUBLE,
  graaddagen              DOUBLE,
  watermeter              DOUBLE,
  watermeter_offset       DOUBLE,
  waterbill               DOUBLE,
  remarks                 TEXT
);

CREATE TABLE meterreadings 
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  date                    DATETIME,
  emeter1                 DOUBLE,
  emeter1_offset          DOUBLE,
  emeter2                 DOUBLE,
  emeter2_offset          DOUBLE,
  emeter3                 DOUBLE,
  emeter3_offset          DOUBLE,
  emeter4                 DOUBLE,
  emeter4_offset          DOUBLE,
  solarmeter              DOUBLE,
  solarmeter_offset       DOUBLE,
  gasmeter                DOUBLE,
  gasmeter_offset         DOUBLE,
  watermeter              DOUBLE,
  watermeter_offset       DOUBLE,
  uchpproduction          DOUBLE,
  uchpproduction_offset   DOUBLE,
  uchpconsumption         DOUBLE,
  uchpconsumption_offset  DOUBLE,
  remarks                 TEXT
);


CREATE TABLE solarenergyday
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  date                    datetime,

  energy1                 DOUBLE,
  maxpower1               DOUBLE,
  maxpowerindex1          INT,
  instantmaxpower1        DOUBLE,
  instantmaxpowertime1    time,

  minutesactive1          INT,
  energy2                 DOUBLE,
  maxpower2               DOUBLE,
  maxpowerindex2          INT,
  instantmaxpower2        DOUBLE,
  instantmaxpowertime2    time,
  minutesactive2          INT,

  energy3                 DOUBLE,
  maxpower3               DOUBLE,
  maxpowerindex3          INT,
  instantmaxpower3        DOUBLE,
  instantmaxpowertime3    TIME,
  minutesactive3       	  INT
);

CREATE TABLE solarenergyfiveminutes
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  datetime                datetime,
  timeindex               INT,
  year                    INT,
  pulse1                  INT,
  pulsepower1             INT,
  pulsemaxpower1          INT,
  pulsemeter1             INT,
  pulse2                  INT,
  pulsepower2             INT,
  pulsemaxpower2          INT,
  pulsemeter2             INT,
  pulse3                  INT,
  pulsepower3             INT,
  pulsemaxpower3          INT,
  pulsemeter3             INT,
  p1time                  CHAR(20),
  electricityimportlow    INT,
  electricityimportnormal INT,
  electricityexportlow    INT,
  electricityexportnormal INT,
  tariff                  INT,
  powerfailures           INT,
  powerfailureslong       INT,
  sagsl1                  INT,
  sagsl2                  INT,
  sagsl3                  INT,
  swellsl1                INT,
  swellsl2                INT,
  swellsl3                INT,
  voltagel1               INT,
  voltagel2               INT,
  voltagel3               INT,
  currentl1               INT,
  currentl2               INT,
  currentl3               INT,
  actpowerimportl1        INT,
  actpowerimportl2        INT,
  actpowerimportl3        INT,
  actpowerexportl1        INT,
  actpowerexportl2        INT,
  actpowerexportl3        INT,
  grosspower              INT,
  netpower                INT,
  gasimport               INT,
  gastime                 CHAR(20)
);


ALTER TABLE solarenergyfiveminutes ADD UNIQUE INDEX dbtimeindex (year, timeindex);
ALTER TABLE solarenergyday         ADD UNIQUE INDEX dbtimeindex (date);



