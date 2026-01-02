/**************************************************************************************************\
*
* SolarPublishWebsocket.h
*
* Publishing real-time data using websockets
*
\**************************************************************************************************/
#ifndef SOLARPUBLISHWEBSOCKET_H

#define SOLARPUBLISHWEBSOCKET_H
 
#include <stdio.h>
#include <stdlib.h> 


#include "libwebsockets.h"


#include "Toolbox.h"
#include "Log.h"
#include "Clock.h" 
#include "SolarPublish.h" 

#define LOCAL_RESOURCE_PATH "./html"

#define QUEUE_LENGTH 200


/* solarpower protocol */

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 * for this example protocol we use it to individualize the count for each
 * connection.
 */

typedef struct  
{
    int     messageQueueTail;
} perSessionDataSolarPower_t;


typedef struct
{
    int             reading;
    solarTime_t     time;
    double          value;
} message_t;

struct serveable 
{
    const char *urlpath;
    const char *mimetype;
}; 
 
class SolarPublishWebsocket:SolarPublish
{
private:
    message_t               messageQueue[QUEUE_LENGTH];
    int                     messageQueueHead;
    int                     messageQueueTail;
                            SolarPublishWebsocket                       ();
    friend void*            websocketServerTask                         (void* param);

    friend  int             callback_http               (struct lws *wsi,
                                                         enum lws_callback_reasons reason, void *user,
                                                         void *in, size_t len);
    friend int              callback_solarpower         (struct lws *wsi,
                                                         enum lws_callback_reasons reason,
                                                         void *user, void *in, size_t len);                                          

    friend void             logFunction                 (int level, const char* line);
    int                     startWebSocketServer        ();
    void                    closeWebSocketServer        ();
    void                    websocketSend               (char* messageText);

public:
    static SolarPublish*                    getInstance ();

    void                                    postMessage                 (solarTime_t time, int reading, double value);
   
    void                                    start                       ();
    void                                    stop                        ();

                                            ~SolarPublishWebsocket      (); 
};
 
#endif