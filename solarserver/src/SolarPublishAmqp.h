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
#define MAX_AMQPMESSAGE_SIZE   128

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

    char                            messageText[MAX_AMQPMESSAGE_SIZE];

    amqp_connection_state_t         connection;

                                    SolarPublishAmqp        ();

    friend void*                    sendTask                (void* param);
    bool                            amqpSend                (message_t* message);

    bool                            openConnection          ();
    bool                            sendTheMessage          (message_t* message);
    bool                            closeConnection         ();

public:
    static SolarPublish*            getInstance ();

    void                            start                   ();

                                    ~SolarPublishAmqp       (); 
};
 
#endif