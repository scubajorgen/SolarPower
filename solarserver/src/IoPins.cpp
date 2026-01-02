/**************************************************************************************************\
*
* IoPins.cpp
*
* I/O pin and LED control
*
\**************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"


#include "wiringPi.h"
#include "IoPins.h"

#define PORT 0x600

/******************************************************************************\
* Variables
\******************************************************************************/

IoPins* IoPins::theInstance=NULL;

PinConvert_t IoPins::pinConversion[]=
{
  {"GPIO0" , -1},
  {"GPIO1" , -1},
  {"GPIO2" ,  8},
  {"GPIO3" ,  9},
  {"GPIO4" ,  7},
  {"GPIO5" , -1},
  {"GPIO6" , -1},
  {"GPIO7" , 11},
  {"GPIO8" , 10},
  {"GPIO9" , 13},
  {"GPIO10", 12},
  {"GPIO11", 14},
  {"GPIO12", -1},
  {"GPIO13", -1},
  {"GPIO14", 15},
  {"GPIO15", 16},
  {"GPIO16", -1},
  {"GPIO17",  0},
  {"GPIO18",  1},
  {"GPIO19", -1},
  {"GPIO20", -1},
  {"GPIO21", -1},
  {"GPIO22",  3},
  {"GPIO23",  4},
  {"GPIO24",  5},
  {"GPIO25",  6},
  {"GPIO26", -1},
  {"GPIO27",  2}
};


/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/
IoPins::IoPins()
{
    configuration       =Configuration::getInstance();

    // Variables for testmode. Set testMode to true for testmode. 
    // When in testmode, the system simulates incoming pulses. One pulse
    // cycle per 2 seconds (=900 Watt)
    testMode            =false;
    testPulse           =0;
    testCount           =0;
    
    // Initialise the I/O
    this->initialise();
}

/******************************************************************************\
*
*  Initialisation
*
\******************************************************************************/
void IoPins::initialise()
{
    wiringPiSetup ();
    pinMode(getPulseInputPin(IOPINS_PULSE1), INPUT);  
    pinMode(getPulseLedOutputPin(IOPINS_PULSE1), OUTPUT);  
    pinMode(getPulseInputPin(IOPINS_PULSE2), INPUT);  
    pinMode(getPulseLedOutputPin(IOPINS_PULSE2), OUTPUT);  
    pinMode(getPulseInputPin(IOPINS_PULSE3), INPUT);  
    pinMode(getPulseLedOutputPin(IOPINS_PULSE3), OUTPUT);  
    
    pinMode(convertPinNumberToPin(configuration->getGpioHeartbeatLed()), OUTPUT);  
    
}

/******************************************************************************\
*
*  Returns the input pin used for indicated pulse
*
\****int**************************************************************************/
int IoPins::getPulseInputPin(int pulseNo)
{
    int pin;
    
    
    if (pulseNo>=0 && pulseNo<MAX_PULSE_COUNTERS)
    {
        pin=convertPinNumberToPin(configuration->getGpioPulse(pulseNo));
    }
    else
    {
        logger.logError("Error requesting pulse input pin");
        pin=-1;
    }  
    return pin;

}

/******************************************************************************\
*
*  Returns the output pin to be used for the led for indicated pulse
*
\******************************************************************************/
int IoPins::getPulseLedOutputPin(int pulseNo)
{
    int pin;
  
    if (pulseNo>=0 && pulseNo<MAX_PULSE_COUNTERS)
    {
        pin=convertPinNumberToPin(configuration->getGpioLed(pulseNo));
    }
    else
    {
        logger.logError("Error requesting LED output pin");
        pin=-1;
    }  
    return pin;
}

/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
*  This method returns the one and only instance (Singleton)
*
\******************************************************************************/
IoPins* IoPins::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new IoPins();
    }
    return theInstance;
}

/******************************************************************************\
*
*  Destructor
*
\******************************************************************************/
IoPins::~IoPins()
{
}

/******************************************************************************\
*
*  Set a LED to the desired state
*
\******************************************************************************/
void IoPins::setLed(int ledNr, int state)
{
    int pin;

    pin=-1;
    

    switch (ledNr)
    {
        case IOPINS_LED_HEARTBEAT:
            pin=convertPinNumberToPin(configuration->getGpioHeartbeatLed());
            break;
        default:
            logger.logError("Invalid led addressed\n");
            break;
    }

    if (pin>=0)
    {
        if (state)
        {
            digitalWrite(pin, LOW); 
        }
        else
        {
            digitalWrite(pin, HIGH); 
        }
//        gpio->setPin(pin, state);
    }
}

/******************************************************************************\
*
*  Set the state of LED1
*
\******************************************************************************/
void IoPins::setLed1(int state)
{
    logger.logError("Deprecated function call setLed1 - not effective\n");
}

/******************************************************************************\
*
*  Get the state of the pulse input
*
\******************************************************************************/
bool IoPins::getPulse(int pulseNo)
{
	bool value;
	int  ioValue;

    if (!testMode)
    {
        // Read pin 
//        ioValue=gpio->pollPin(getPulseInputPin(pulseNo));
        ioValue=digitalRead(getPulseInputPin(pulseNo));      
    }
    else
    {
        if (testCount==(1000000/SAMPLE_TIME-2))
        {
            testCount=0;
            if (testPulse==0)
            {
                testPulse=1;
            }
            else
            {   
                testPulse=0;
            }
        }
        else
        {
            testCount++;
        }
       

        ioValue=testPulse;
        
    }    
    
    if (ioValue)	// 7th iopin
    {
        value=true;
    }
    else
    {
        value=false;
    }
    
    return value;
}

/******************************************************************************\
*
*  Set the state of the led of indicated pulse
*
\******************************************************************************/
void IoPins::setPulseLed(int pulseNo, int state)
{
    int pin;

    pin=-1;

    pin=getPulseLedOutputPin(pulseNo);

    if (pin>=0)
    {
//        gpio->setPin(pin, state);
        if (state)
        {
            digitalWrite(pin, LOW);
        }
        else
        {
            digitalWrite(pin, HIGH);
        }
    }
}

/******************************************************************************\
*
*  Convert pin number to pin ID. 1 -> GPIO_01
*
\******************************************************************************/
int  IoPins::convertPinNumberToPin(int pinNo)
{
    int pinId;
    
    // Quick but dirty....
    pinId=pinNo;
    
    return pinId;
}

/******************************************************************************\
*
*  Convert pin name to pin ID
*
\******************************************************************************/
int IoPins::convertPinNameToPin(char* pinName)
{
    int pin=-1;
    for (unsigned int i=0; i<sizeof(pinConversion) && pin<0; i++)
    {
        if (strncmp(pinConversion[i].name, pinName, MAXPINNAME)==0)
        {
            pin=pinConversion[i].id;
        }
    }
    return pin;
}
