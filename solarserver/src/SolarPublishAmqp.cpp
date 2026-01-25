/**************************************************************************************************\
*
* SolarPublishAmqp.cpp
*
* Publishing real-time data using AMQP broker. - NOT MAINTAINED
*
\**************************************************************************************************/
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>

#include "common.h"
#include "SolarPublishAmqp.h"
#include "Configuration.h"


/******************************************************************************\
* Friend methods
\******************************************************************************/
/******************************************************************************\
*
* The task function for AMQP message posting
*
\******************************************************************************/
void* sendTask(void* param)
{
    SolarPublishAmqp*   solarPublishAmqp;
    bool                localCloseTask;
    message_t*          message; 
    message_t           lastMessage;
    int                 timeCount;
    bool                success;

    solarPublishAmqp=(SolarPublishAmqp*)SolarPublishAmqp::getInstance();
    
    // Signal the task is running
	pthread_mutex_lock(&solarPublishAmqp->mutex);    
    solarPublishAmqp->taskRunning=true;
	pthread_mutex_unlock(&solarPublishAmqp->mutex);    

    solarPublishAmqp->logger.logInfo("AMQP send task started");

    // Some local variables
    localCloseTask                  = false;
    timeCount                       = 0;
    lastMessage.reading             = INVALID_READING;
    
    // the task loop
    while (!localCloseTask)
    {
        message                     =solarPublishAmqp->popQueue();
        if (message!=NULL)
        {
            success=solarPublishAmqp->openConnection();
            if (success)
            {
                // Send all the messages in the buffer in this session
                do
                {
                    success     =solarPublishAmqp->sendTheMessage(message);
                    lastMessage =*message;
                    message     =solarPublishAmqp->popQueue();
                }
                while (success && message!=NULL);
            }
            if (success)
            {
                solarPublishAmqp->closeConnection();
            }
            timeCount=0;
        }
        else
        {
            // Sleep 0.3 s 
            usleep(300000);
            timeCount++;
            // If 10 sec passed, repeat last message
            if (timeCount==33)
            {
                timeCount=0;
                if (lastMessage.reading!=INVALID_READING)
                {
                    solarPublishAmqp->amqpSend(&lastMessage);
                }
            }
        }
        

        // Check whether the task needs to be killed
        pthread_mutex_lock(&solarPublishAmqp->mutex);    
        localCloseTask=solarPublishAmqp->closeTask;
    	pthread_mutex_unlock(&solarPublishAmqp->mutex);    
    }
    // Signal the thread stopped...
    pthread_mutex_lock(&solarPublishAmqp->mutex);    
    localCloseTask=solarPublishAmqp->taskRunning=false;
	pthread_mutex_unlock(&solarPublishAmqp->mutex);                 
    
    return NULL;
}
 
/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/ 
SolarPublishAmqp::SolarPublishAmqp():SolarPublish()
{
    logger.logInfo("Starting AMQP client");

    Configuration* config=Configuration::getInstance();
    strncpy(hostname           , config->getAmqpHost()      , MAX_AMQPSETTING_LENGTH-1);
    strncpy(exchange           , config->getAmqpExchange()  , MAX_AMQPSETTING_LENGTH-1);
    strncpy(user               , config->getAmqpUser()      , MAX_AMQPSETTING_LENGTH-1);
    strncpy(password           , config->getAmqpPassword()  , MAX_AMQPSETTING_LENGTH-1);
    strncpy(routingkey         , config->getAmqpRoutingKey(), MAX_AMQPSETTING_LENGTH-1);
    strncpy(vHost              , config->getAmqpVHost()     , MAX_AMQPSETTING_LENGTH-1);
    port                    =config->getAmqpPort(); 

    messengerStarted        =false;
}
 
/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of this class
*
\******************************************************************************/
SolarPublish* SolarPublishAmqp::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new SolarPublishAmqp();
        theInstance->start();
    }
    return theInstance;
}

/******************************************************************************\
*
*  This method starts the task
*
\******************************************************************************/
void SolarPublishAmqp::start()
{
    SolarPublish::start();
    startThread(sendTask);
}

/******************************************************************************\
*
* Destructor, terminates the connection and cleans up
*
\******************************************************************************/
SolarPublishAmqp::~SolarPublishAmqp()
{
} 


/******************************************************************************\
*
* Amqp open connection to exchange on AMQP broker
*
\******************************************************************************/
bool SolarPublishAmqp::openConnection()
{
    amqp_socket_t           *socket   = NULL;
    amqp_rpc_reply_t        r;
    int                     status;
    bool                    success   =true;

    connection  = amqp_new_connection();
    socket      = amqp_tcp_socket_new(connection);
    if (socket)
    {
        status = amqp_socket_open(socket, hostname, port);
        if (status==AMQP_STATUS_OK)
        {
            r=amqp_login(connection, vHost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user, password);
            if (r.reply_type==AMQP_RESPONSE_NORMAL)
            {
                amqp_channel_open(connection, 1);
                r=amqp_get_rpc_reply(connection);
                if (r.reply_type!=AMQP_RESPONSE_NORMAL)
                {
                    logger.logError("Error");
                    success=false;
                }
            }
            else
            {
                logger.logError("AMQP broker login failed");
                success=false;
            }
        }
        else
        {
            logger.logError("Error opening TCP socket for AMQP");
            success=false;
        }
    }
    else
    {
        logger.logError("Error creating TCP socket");
        success=false;
    }
    return success;
}

/******************************************************************************\
*
* Send the message
*
\******************************************************************************/
bool SolarPublishAmqp::sendTheMessage(message_t* message)
{
    bool success            =true;
    int  status;

        // fetch next for sending
    sprintf(messageText, "%d %f %lf", message->reading,
                                      message->value, 
                                      message->time.epoch);
    amqp_basic_properties_t props;
    props._flags            = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type      = amqp_literal_bytes((void*)"text/plain");
    props.delivery_mode     = 2; /* persistent delivery mode */
    status=amqp_basic_publish(connection, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey), 0, 0, &props, amqp_cstring_bytes(messageText));
    if (status!=AMQP_STATUS_OK)
    {
        success=false;
    }
    return success;
}


/******************************************************************************\
*
* Close the connection to the broker
*
\******************************************************************************/
bool SolarPublishAmqp::closeConnection()
{
    bool                    success=true;
    amqp_rpc_reply_t        r;

    r=amqp_channel_close(connection, 1, AMQP_REPLY_SUCCESS);
    if (r.reply_type!=AMQP_RESPONSE_NORMAL)
    {
        logger.logError("Error closing AMQP channel");
        success=false;
    }

    r=amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
    if (r.reply_type!=AMQP_RESPONSE_NORMAL)
    {
        logger.logError("Error closing AMQP connection");
        success=false;
    }

    amqp_destroy_connection(connection);
    return success;
}


/******************************************************************************\
*
* Amqp send - Open connection, send, close connection
*
\******************************************************************************/

bool SolarPublishAmqp::amqpSend(message_t* messageText)
{
    bool success=openConnection();
    if (success)
    {
        success=sendTheMessage(messageText);
    }
    if (success)
    {
        closeConnection();
    }
    return success;
}
