/**************************************************************************************************\
*
* IoPins.h
*
* I/O pin and LED control
*
\**************************************************************************************************/
#ifndef IOPINS
#define IOPINS


#include "Configuration.h"
#include "Log.h"
#include "common.h"

/*
  wiringPi pin numbers vs pinout

   WPI  PIN    TYPE              TYPE    PIN WPI
   ____ ____________________________________ ____
        3V3            1   2              5V
   8    GPIO2  SDA     3   4              5V
   9    GPIO3  SCL     5   6          Ground
   7    GPIO4  GPCLK0  7   8      TXD GPIO14  15
        GROUND         9  10      RXD GPIO15  16
   0    GPIO17         11 12  PCM_CLK GPIO18   1
   2    GPIO27         13 14          Ground
   3    GPIO22         15 16          GPIO23   4
        3V3            17 18          GPIO24   5
   12   GPIO10 MOSI    19 20          Ground
   13   GPIO9  MISO    21 22          GPIO25   6
   14   GPIO11 SCLK    23 24      CE0  GPIO8  10
        Ground         25 26      CE1  GPIO7  11
        GPIO0  ID_SD   27 28           GPIO1
        GPIO5          29 30          Ground
        GPIO6          31 32     PWM0 GPIO12
        GPIO13 PWM1    33 34          Ground
        GPIO19 PCM_FS  35 36          GPIO16
        GPIO26         37 38  PCM_DIN GPIO20
        Ground         39 40 PCM_DOUT GPIO21
*/



#define IOPINS_LED_HEARTBEAT    0
#define IOPINS_LED_NOACTIVITY   1
#define IOPINS_LED_ACTIVITY     2
#define IOPINS_LED_PULSETOGGLE  3

#define IOPINS_PULSE1           0
#define IOPINS_PULSE2           1
#define IOPINS_PULSE3           2

#define MAXPINNAME              10


typedef struct
{
    char    name[MAXPINNAME];
    INT32   id;
} PinConvert_t;


class IoPins
{
private:
    Log                 logger {"iopins"};
    Configuration*      configuration;

    static PinConvert_t pinConversion[];


    void initialise();
    static IoPins*      theInstance;

    bool testMode;
    int  testCount;
    int  testPulse;

    IoPins();
    int getPulseInputPin        (int pulseNo);
    int getPulseLedOutputPin    (int pulseNo);

public:
    static IoPins* getInstance  ();
    ~IoPins                     ();

    void setLed                 (int ledNr, int state);
    void definePulsePins        (int pulseNo);
    bool getPulse               (int pulseNo);
    void setPulseLed            (int pulseNo, int state);
    void setLed1                (int state);
    
    int  convertPinNumberToPin  (int pinNo);
    
    static int convertPinNameToPin(char* pinName);
    
};

#endif