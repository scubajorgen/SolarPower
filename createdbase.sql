CREATE DATABASE energy;

USE energy;

CREATE TABLE energybill 
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  date                    datetime,
  emeter1                 double,
  emeter1_offset          double,
  emeter2                 double,
  emeter2_offset          double,
  emeter3                 double,
  emeter3_offset          double,
  emeter4                 double,
  emeter4_offset          double,
  solarmeter              double,
  solarmeter_offset       double,
  uchpproduction          double,
  uchpproduction_offset   double,
  uchpconsumption         double,
  uchpconsumption_offset  double,
  ebill                   double,
  gasmeter                double,
  gasmeter_offset         double,
  gasbill                 double,
  graaddagen              double,
  watermeter              double,
  watermeter_offset       double,
  waterbill               double,
  remarks                 text
);

CREATE TABLE meterreadings 
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  date                    datetime,
  emeter1                 double,
  emeter1_offset          double,
  emeter2                 double,
  emeter2_offset          double,
  emeter3                 double,
  emeter3_offset          double,
  emeter4                 double,
  emeter4_offset          double,
  solarmeter              double,
  solarmeter_offset       double,
  gasmeter                double,
  gasmeter_offset         double,
  watermeter              double,
  watermeter_offset       double,
  uchpproduction          double,
  uchpproduction_offset   double,
  uchpconsumption         double,
  uchpconsumption_offset  double,
  remarks                 text
);


CREATE TABLE solarenergyday
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  date                    datetime,

  energy1                 double,
  maxpower1               double,
  maxpowerindex1          int(11),
  instantmaxpower1        double,
  instantmaxpowertime1    time,

  minutesactive1          int(11),
  energy2                 double,
  maxpower2               double,
  maxpowerindex2          int(11),
  instantmaxpower2        double,
  instantmaxpowertime2    time,
  minutesactive2          int(11),

  energy3                 double,
  maxpower3               double,
  maxpowerindex3          int(11),
  instantmaxpower3        double,
  instantmaxpowertime3    time,
  minutesactive3       	  int(11)
);

CREATE TABLE solarenergyfiveminutes
(
  ix                      int(11) AUTO_INCREMENT PRIMARY KEY,
  datetime                datetime,
  timeindex               int(32),
  year                    int(16),
  pulse1                  int(16),
  pulsepower1             int(32),
  pulsemaxpower1          int(32),
  pulsemeter1             int(32),
  pulse2                  int(16),
  pulsepower2             int(32),
  pulsemaxpower2          int(32),
  pulsemeter2             int(32),
  pulse3                  int(16),
  pulsepower3             int(32),
  pulsemaxpower3          int(32),
  pulsemeter3             int(32),
  electricityimportlow    int(32),
  electricityimportnormal int(32),
  electricityexportlow    int(32),
  electricityexportnormal int(32),
  gasimport               int(32),
  grosspower              int(32),
  netpower                int(32)
);


ALTER TABLE solarenergyfiveminutes ADD UNIQUE INDEX dbtimeindex (year, timeindex);
ALTER TABLE solarenergyday ADD UNIQUE INDEX dbtimeindex (date);



