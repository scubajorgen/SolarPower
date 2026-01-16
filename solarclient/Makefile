#
#
# Make file for SolarClient
#
#

OBJDIR = ./obj

CC = gcc


VERSION=1.0


CFLAGS		= -I. -I./src -I./src/generics -I/usr/local/include -I /usr/include/mysql -Wall -g -O2 -c -DOSA_POSIX 
LDFLAGS		= -Wl,-rpath,/usr/local/lib -lstdc++
LIBS		= -L/usr/lib -L /usr/lib/mysql -lpthread -l mysqlclient -l mysqlclient_r -l rt

_SRCOBJ		= 	SolarClient.o \
				Configuration.o \
				Scheduler.o \
				Toolbox.o \
				Client.o \
				Connection.o \
				Clock.o \
				Log.o \
				Datastore.o

SRCOBJ	= $(patsubst %,$(OBJDIR)/%,$(_SRCOBJ))

OBJ		= $(SRCOBJ) 

VPATH	= ./src ./src/generics

all: SolarClient

SolarClient: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(OBJDIR)/%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ $<



install:


clean:
	rm -f $(OBJDIR)/*.o Solar

