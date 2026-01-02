/**************************************************************************************************\
*
* Connection.cpp
*
* Conenction to the SolarServer
*
\**************************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <netinet/tcp.h>

#include "Connection.h"
#include "Configuration.h"

Connection* Connection::theInstance=NULL;

/******************************************************************************\
*
* Constructor
*
\******************************************************************************/
Connection::Connection()
{   
    this->socketActive=false;
}
    

/******************************************************************************\
*
* This method returns the one and only instance of this class (Singleton)
*
\******************************************************************************/
Connection*  Connection::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new Connection();
    }    
    return theInstance;
}

/******************************************************************************\
*
*  This method opens a socket connection to the server
*
\******************************************************************************/
void Connection::connectToServer()
{
    Configuration*      config;
    int                 result;
    unsigned long        dotIpAddress;
    struct hostent        *lphost;

    sockaddr_in            service;

    config=Configuration::getInstance();

    // create the socket
    sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


    // disable naggle algorithm
    int noDelay=1;
    result=setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay, sizeof(noDelay));
    if (result<0)
    {
        logger.logError("Could not disable nagle algorithm");
    }

    if (sock==INVALID_SOCKET)
    {
        logger.logError("Could not open socket");
    }

    strncpy(ipAddress, config->getServerAddress(), 16);
    port=config->getServerPort();

    dotIpAddress = inet_addr (ipAddress);
    if (dotIpAddress==INADDR_NONE)
    {
            lphost = gethostbyname(ipAddress);
            if(lphost!=NULL)
            {
                /* name was found */
                memcpy (&service.sin_addr.s_addr,lphost->h_addr,lphost->h_length);
            }
            else
            {
                sockErr=SOCKERR_CONNECTERROR;
            }

    }
    else
    {
        service.sin_addr.s_addr=dotIpAddress;    
    }

    service.sin_family=AF_INET;

    service.sin_port=htons(port);

    if (sockErr==SOCKERR_OK)
    {
        result=connect(sock, (sockaddr*)&service, sizeof(service));

        if (result==0)
        {
            socketActive=true;
        }
        else
        {
            logger.logError("Error connecting");
        }
    }

}

/******************************************************************************\
*
*  This method sends a block of data 
*
\******************************************************************************/
void Connection::sendData(char* data, int length)
{
    int writeResult=send(sock, data, length, MSG_NOSIGNAL);

    if (writeResult==SOCKET_ERROR)
    {
        logger.logError("Error sending data");
        disconnectFromServer();
    }
}

/******************************************************************************\
*
*  This method waits for data 
*
\******************************************************************************/
void Connection::waitForData(char* data, int* length, int maxDataLength)
{
    int     readResult=0;
    // Use the receive method in polling mode. Poll for at most 1 second
    bool exit				=false;
    int  tenthOfSecondCount	=0;
    while (!exit && tenthOfSecondCount<10)
    {
        readResult=recv(sock, data, maxDataLength, MSG_DONTWAIT);
        if (readResult==SOCKET_ERROR)
        {
            if (errno!=EAGAIN)
            {
                exit=true;
            }
        }
        else
        {
            exit=true;
        }
        usleep(100000);
        tenthOfSecondCount++;
        
    }
    
    if (readResult==0)
    {
        *length=0;
        logger.logError("Error receiving data");
        disconnectFromServer();
    }
    else if (readResult==SOCKET_ERROR)
    {
        *length=0;
        logger.logError("Error receiving data");
        disconnectFromServer();
    }
    else
    {
        *length=readResult;
    }
}

/******************************************************************************\
*
*  Abort connection
*
\******************************************************************************/
void Connection::disconnectFromServer()
{
    if (socketActive)
    {
        // indicate the socket is no longer active
        socketActive=false;
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
}

