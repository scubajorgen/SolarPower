
/**************************************************************************************************\
*
* Connection.cpp
*
* Conenction to the SolarServer
*
\**************************************************************************************************/
#include "Log.h"

typedef int SOCKET;

#define SOCKET_ERROR		-1
#define INVALID_SOCKET		-1

enum sockErr_t
{
    SOCKERR_OK=0,
    SOCKERR_INITIALISEERROR=1,
    SOCKERR_BINDERROR=2,
    SOCKERR_CONNECTERROR=3,
    SOCKERR_LISTENERROR=4,
    SOCKERR_ADDRESSERROR=5,
    SOCKERR_WRITEERROR=6,
	SOCKERR_CONNECTIONCLOSED=7,
	SOCKERR_READERROR=8,
	SOCKERR_DATAERROR
};

class Connection
{
private:
    Log                     logger {"connnection"};
    static Connection*      theInstance;
    char                    ipAddress[100];
    int                     port;
    sockErr_t               sockErr;
    SOCKET                  sock;
    bool                    socketActive;

                            Connection          ();
public:

    static Connection*      getInstance         ();    
    void                    connectToServer     ();
    void                    disconnectFromServer();
    void                    sendData            (char* data, int length);
    void                    waitForData         (char* data, int* length, int maxDataLength);
};

