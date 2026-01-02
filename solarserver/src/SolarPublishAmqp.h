/**************************************************************************************************\
*
* SolarPublishAmqp.cpp
*
* Publishing real-time data using AMQP broker.
*
\**************************************************************************************************/
#ifndef SOLARPUBLISHAMQP_H

#define SOLARPUBLISHAMQP_H
 
#include <stdio.h>
#include <stdlib.h> 

#include "SolarPublish.h"

#include "Toolbox.h"
#include "Log.h"
#include "Clock.h" 
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>
 
#define MAX_AMQPSETTING_LENGTH 128

class SolarPublishAmqp:SolarPublish
{
private:
    Log                             logger {"pubamqp"};
    static char const               *address;
    bool                            messengerStarted;

    char                            hostname[MAX_AMQPSETTING_LENGTH];
    int                             port; 
    char                            exchange[MAX_AMQPSETTING_LENGTH];
    char                            user[MAX_AMQPSETTING_LENGTH];
    char                            password[MAX_AMQPSETTING_LENGTH];
    char                            routingkey[MAX_AMQPSETTING_LENGTH];
    char                            vHost[MAX_AMQPSETTING_LENGTH];


    amqp_connection_state_t         connection;

                                    SolarPublishAmqp        ();

    friend void*                    sendTask                (void* param);
    void                            amqpSend                (char* messageText);

    bool                            openConnection          ();
    bool                            sendTheMessage          (char* messageText);
    bool                            closeConnection         ();

public:
    static SolarPublish*            getInstance ();

    void                            start                   ();

                                    ~SolarPublishAmqp       (); 
};
 
#endif