#
#
# Make file for Solar
#
#

CC = g++
OBJDIR = ./obj

# NONE, AMQP or WEBSOCKETS
# PUBLISH = PUBLISH_NONE
PUBLISH = PUBLISH_WEBSOCKETS
# PUBLISH = PUBLISH_AMQP

VERSION		=6.0

CFLAGS		= -D${PUBLISH} -I./src -I./src/generics -I /usr/include/mysql -I /usr/include/rabbitmq-c -Wall -g -O2 -c -DOSA_POSIX 
LDFLAGS		= -Wl,-rpath,/usr/local/lib -lstdc++
PRELIBS		= -L/usr/lib -L /usr/local/lib -L /usr/lib/mysql  -l pthread  -l rt -lwiringPi -lwiringPiDev 

ifeq (${PUBLISH}, PUBLISH_AMQP)
  LIBS = $(PRELIBS) -l:librabbitmq.a
endif

ifeq (${PUBLISH}, PUBLISH_WEBSOCKETS)
  LIBS = $(PRELIBS) -l websockets
endif

ifeq (${PUBLISH}, PUBLISH_NONE)
  LIBS = $(PRELIBS)
endif

_SRCOBJ	=	Scheduler.o \
			Configuration.o \
			MeasurementStorage.o \
			IoPins.o \
			Meter.o \
			SmartMeter.o \
			PulseCounter.o \
			Clock.o \
			Toolbox.o \
			Solar.o \
			Server.o \
			SolarPublish.o \
			Simulation.o \
			Log.o

_SRCOBJAMQP    = SolarPublishAmqp.o

_SRCOBJWEBSOCK = SolarPublishWebsocket.o

SRCOBJ              = $(patsubst %,$(OBJDIR)/%,$(_SRCOBJ))
SRCOBJAMQP          = $(patsubst %,$(OBJDIR)/%,$(_SRCOBJAMQP))
SRCOBJWEBSOCK       = $(patsubst %,$(OBJDIR)/%,$(_SRCOBJWEBSOCK))


ifeq (${PUBLISH}, PUBLISH_AMQP)
   OBJ = $(SRCOBJ) $(SRCOBJAMQP) 
endif

ifeq (${PUBLISH}, PUBLISH_WEBSOCKETS)
   OBJ = $(SRCOBJ) $(SRCOBJWEBSOCK)
endif

ifeq (${PUBLISH}, PUBLISH_NONE)
   OBJ = $(SRCOBJ) 
endif


VPATH = ./src ./src/generics

all: Solar

Solar: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

#$(OBJDIR)/$(OBJ):%.o:%.cpp
$(OBJDIR)/%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ $<


install:


clean:
	rm -f $(OBJDIR)/*.o Solar

