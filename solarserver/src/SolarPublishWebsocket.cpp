/**************************************************************************************************\
*
* SolarPublishWebsocket.cpp
*
* Publishing real-time data using websockets
*
\**************************************************************************************************/
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "SolarPublishWebsocket.h"

#include "pthread.h" 

#define PROTOCOLINDEX_HTTP       0
#define PROTOCOLINDEX_SOLARPOWER 1

/*
 * We take a strict whitelist approach to stop ../ attacks
 */

const struct serveable whitelist[] = 
{
    { "/favicon.ico", "image/x-icon" },
    { "/libwebsockets.org-logo.png", "image/png" },

    /* last one is the default served if no match */
    { "/test.html", "text/html" },
};

struct lws_context             *context;

 int callback_http(struct lws *wsi,
                            enum lws_callback_reasons reason, void *user,
                            void *in, size_t len);

int callback_solarpower(struct lws *wsi,
                               enum lws_callback_reasons reason,
                               void *user, void *in, size_t len);

/* list of supported protocols and callbacks */
struct lws_protocols protocols[3] = 
{
    /* first protocol must always be HTTP handler */
    {
        "http-only",    // name
        callback_http,  // callback
        0,              // per_session_data_size
        0,              // max frame size / rx buffer
        1,              // ID 
        NULL,           // user
        0               // tx_packet_size
    },
    {
        "solarpower-protocol",
        callback_solarpower,
        sizeof(perSessionDataSolarPower_t),
        128,
        2,
        NULL, 
        0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
};

Log socketLogger("pubwebsock");

/******************************************************************************\
* Friend methods
\******************************************************************************/
/******************************************************************************\
*
* The task function for servicing the websockets
*
\******************************************************************************/
void* websocketServerTask(void* param)
{
    SolarPublishWebsocket*  solarPublish;
    bool                    localCloseTask;
   
    solarPublish=(SolarPublishWebsocket*)SolarPublishWebsocket::getInstance();
    
    // Signal the task is running
    pthread_mutex_lock(&solarPublish->mutex);    
    solarPublish->taskRunning=true;
    pthread_mutex_unlock(&solarPublish->mutex);    

    socketLogger.logInfo("Websocket server task started");

    // Some local variables
    localCloseTask                  = false;
    
    
    // the task loop
    while (!localCloseTask)
    {
        // Hand some processing to the webserver for 100 ms
        // it blocks when no websockets need to be serviced        
        lws_service(context, 100);

        // Fetch the guarded variables
        pthread_mutex_lock(&solarPublish->mutex);    
        localCloseTask=solarPublish->closeTask;
        pthread_mutex_unlock(&solarPublish->mutex);    
    } 
    
    // Signal the thread stopped...
    pthread_mutex_lock(&solarPublish->mutex);    
    localCloseTask=solarPublish->taskRunning=false;
    pthread_mutex_unlock(&solarPublish->mutex);        
    
    return NULL;
}

/******************************************************************************\
*
* The HTTP protocol callback function
*
\******************************************************************************/

 int callback_http(struct lws *wsi,
                            enum lws_callback_reasons reason, void *user,
                            void *in, size_t len)
{
#if 0
    char client_name[128];
    char client_ip[128];
#endif
    char        buf[256];
    unsigned    int n;
#ifdef EXTERNAL_POLL
    int m;
    int fd = (int)(long)user;
#endif

    switch (reason) 
    {
    case LWS_CALLBACK_HTTP:
        for (n = 0; n < (sizeof(whitelist) / sizeof(whitelist[0]) - 1); n++)
            if (in && strcmp((const char *)in, whitelist[n].urlpath) == 0)
                break;

        sprintf(buf, LOCAL_RESOURCE_PATH"%s", whitelist[n].urlpath);

        if (lws_serve_http_file(wsi, buf, whitelist[n].mimetype, NULL, 0))
            return 1; /* through completion or error, close the socket */

        /*
         * notice that the sending of the file completes asynchronously,
         * we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when
         * it's done
         */

        break;

    case LWS_CALLBACK_HTTP_FILE_COMPLETION:
//      lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen");
        /* kill the connection after we sent one file */
        return 1;

    /*
     * callback for confirming to continue with client IP appear in
     * protocol 0 callback since no websocket protocol has been agreed
     * yet.  You can just ignore this if you won't filter on client IP
     * since the default uhandled callback return is 0 meaning let the
     * connection continue.
     */

    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
#if 0
        libwebsockets_get_peer_addresses(context, wsi, (int)(long)user, client_name,
                 sizeof(client_name), client_ip, sizeof(client_ip));

        fprintf(stderr, "Received network connect from %s (%s)\n",
                            client_name, client_ip);
#endif
        /* if we returned non-zero from here, we kill the connection */
        break;

#ifdef EXTERNAL_POLL
    /*
     * callbacks for managing the external poll() array appear in
     * protocol 0 callback
     */

    case LWS_CALLBACK_ADD_POLL_FD:

        if (count_pollfds >= max_poll_elements) {
            lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
            return 1;
        }

        fd_lookup[fd] = count_pollfds;
        pollfds[count_pollfds].fd = fd;
        pollfds[count_pollfds].events = (int)(long)len;
        pollfds[count_pollfds++].revents = 0;
        break;

    case LWS_CALLBACK_DEL_POLL_FD:
        if (!--count_pollfds)
            break;
        m = fd_lookup[fd];
        /* have the last guy take up the vacant slot */
        pollfds[m] = pollfds[count_pollfds];
        fd_lookup[pollfds[count_pollfds].fd] = m;
        break;

    case LWS_CALLBACK_SET_MODE_POLL_FD:
        pollfds[fd_lookup[fd]].events |= (int)(long)len;
        break;

    case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
        pollfds[fd_lookup[fd]].events &= ~(int)(long)len;
        break;
#endif

    default:
        break;
    }

    return 0;
}

/******************************************************************************\
 * this is just an example of parsing handshake headers, you don't need this
 * in your code unless you will filter allowing connections by the header
 * content
\******************************************************************************/
static void dump_handshake_info(struct lws *wsi)
{
    int n;
    static const char *token_names[WSI_TOKEN_COUNT] = {
        /*[WSI_TOKEN_GET_URI]       =*/ "GET URI",
        /*[WSI_TOKEN_HOST]          =*/ "Host",
        /*[WSI_TOKEN_CONNECTION]    =*/ "Connection",
        /*[WSI_TOKEN_KEY1]          =*/ "key 1",
        /*[WSI_TOKEN_KEY2]          =*/ "key 2",
        /*[WSI_TOKEN_PROTOCOL]      =*/ "Protocol",
        /*[WSI_TOKEN_UPGRADE]       =*/ "Upgrade",
        /*[WSI_TOKEN_ORIGIN]        =*/ "Origin",
        /*[WSI_TOKEN_DRAFT]         =*/ "Draft",
        /*[WSI_TOKEN_CHALLENGE]     =*/ "Challenge",

        /* new for 04 */
        /*[WSI_TOKEN_KEY]           =*/ "Key",
        /*[WSI_TOKEN_VERSION]       =*/ "Version",
        /*[WSI_TOKEN_SWORIGIN]      =*/ "Sworigin",

        /* new for 05 */
        /*[WSI_TOKEN_EXTENSIONS]    =*/ "Extensions",

        /* client receives these */
        /*[WSI_TOKEN_ACCEPT]        =*/ "Accept",
        /*[WSI_TOKEN_NONCE]         =*/ "Nonce",
        /*[WSI_TOKEN_HTTP]          =*/ "Http",
        /*[WSI_TOKEN_MUXURL]        =*/ "MuxURL",
    };
    char buf[256];

    for (n = 0; n < WSI_TOKEN_COUNT; n++) 
    {
        if (!lws_hdr_total_length(wsi, (lws_token_indexes)n))
            continue;
        lws_hdr_copy(wsi, buf, sizeof buf, (lws_token_indexes)n);
        fprintf(stderr, "    %s = %s\n", token_names[n], buf);
    }
}

/******************************************************************************\
*
* The solar power websocket protocol callback function
*
\******************************************************************************/
int callback_solarpower(struct lws *wsi,
                               enum lws_callback_reasons reason,
                               void *user, void *in, size_t len)
{
    int                     n;
    SolarPublishWebsocket*  solarPublish;
    
    
    unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 +
                          LWS_SEND_BUFFER_POST_PADDING];
    unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

    // Not used here... 
    perSessionDataSolarPower_t *pss = (perSessionDataSolarPower_t *)user;

    solarPublish=(SolarPublishWebsocket*)SolarPublishWebsocket::getInstance();
    switch (reason) 
    {
    case LWS_CALLBACK_ESTABLISHED:
        lwsl_info("callback_dumb_increment: "
                  "LWS_CALLBACK_ESTABLISHED");
        pthread_mutex_lock(&solarPublish->mutex);
        // Get a local COPY of the current tail of the Queue to be used for this client
        // (each client gets its own tail).
        // Skip the oldest values, because they may be overwritten before we can send it 
        // (in case the buffer is full). We have at least 2 seconds to send the next oldest values
        pss->messageQueueTail=solarPublish->messageQueueTail;
        for (int i=0; i<MESSAGETYPES; i++)
        {
            if (pss->messageQueueTail!=solarPublish->messageQueueHead)
            {
                pss->messageQueueTail++;
                if (pss->messageQueueTail==QUEUEDEPTH)
                {
                    pss->messageQueueTail=0;
                }
            }
        }   
        pthread_mutex_unlock(&solarPublish->mutex);
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        pthread_mutex_lock(&solarPublish->mutex);
        n=0;
        // If anything to send in the buffer
        if (pss->messageQueueTail!=solarPublish->messageQueueHead)
        {
            message_t msg=solarPublish->messageQueue[pss->messageQueueTail];
            // fetch next for sending
            n = sprintf((char *)p, "%d %f %lf", msg.reading,
                                                msg.value, 
                                                msg.time.epoch);
            pss->messageQueueTail++;
            if (pss->messageQueueTail==QUEUEDEPTH)
            {
                pss->messageQueueTail=0;
            }
        }
        pthread_mutex_unlock(&solarPublish->mutex);

        if (n>0)
        {
            // send it
            n = lws_write(wsi, p, n, LWS_WRITE_TEXT);
            if (n < 0) 
            {
                lwsl_err("ERROR %d writing to socket", n);
                return 1;
            }
            lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
        }
        break;

    case LWS_CALLBACK_RECEIVE:
        lwsl_notice("Message received from client: %s", (char *)in);
        break;
    /*
     * this just demonstrates how to use the protocol filter. If you won't
     * study and reject connections based on header content, you don't need
     * to handle this callback
     */

    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        dump_handshake_info(wsi);
        /* you could return non-zero here and kill the connection */
        break;

    default:
        break;
    }

    return 0;
}


/******************************************************************************\
* Private methods
\******************************************************************************/
/******************************************************************************\
*
* The constructor. Initialises the instance
*
\******************************************************************************/
SolarPublishWebsocket::SolarPublishWebsocket():SolarPublish()
{
}
 
/******************************************************************************\
* Public methods
\******************************************************************************/
/******************************************************************************\
*
* This method returns the one and only instance of this class
*
\******************************************************************************/
SolarPublish* SolarPublishWebsocket::getInstance()
{
    if (theInstance==NULL)
    {
        theInstance=new SolarPublishWebsocket();
        theInstance->start();
    }
    return theInstance;
}

/******************************************************************************\
*
*  This method starts the task
*
\******************************************************************************/
void SolarPublishWebsocket::start()
{
    // Start the websocket server
    startWebSocketServer();
    SolarPublish::start();
    startThread(websocketServerTask);
}

/******************************************************************************\
*
*  This method kills the task
*
\******************************************************************************/
void SolarPublishWebsocket::stop()
{
    SolarPublish::stop();
    // Stop the websocket server
    closeWebSocketServer();
}

/******************************************************************************\
*
* Post message to the websockets exchange
*
\******************************************************************************/
void SolarPublishWebsocket::postMessage(solarTime_t time, int reading, double value)
{
    SolarPublish::postMessage(time, reading, value);
    
    // Signal something to send
    lws_callback_on_writable_all_protocol(context, &protocols[PROTOCOLINDEX_SOLARPOWER]); 
    // Causes a LWS_CALLBACK_EVENT_WAIT_CANCELLED in the lws service thread context.
    lws_cancel_service(context);
}

/******************************************************************************\
*
* Destructor, terminates the connection and cleans up
*
\******************************************************************************/
SolarPublishWebsocket::~SolarPublishWebsocket()
{
} 

/******************************************************************************\
*
* Log function. Used to log
*
\******************************************************************************/
void logFunction(int level, const char* line)
{
    socketLogger.logInfo(line);  
}

/******************************************************************************\
*
* Start the socket server
*
\******************************************************************************/
int SolarPublishWebsocket::startWebSocketServer()
{
    int use_ssl;
    int opts;
    int debug_level;    

//  char interface_name[128] = "";

    const char *iface = NULL;
#ifndef WIN32
    int syslog_options = LOG_PID | LOG_PERROR;
#endif
    struct lws_context_creation_info info;

    // Initialise the info array
    memset(&info, 0, sizeof info);
    
    // Port to listen on
    info.port = 7681;
    
    // Use ssl 1 or not 0
    use_ssl=0;
    

    opts=0;
    
    // debug level: 0 none - 7 max
    debug_level=7;


#ifndef WIN32
    /* we will only try to log things according to our debug_level */
    setlogmask(LOG_UPTO (LOG_DEBUG));
    openlog("lwsts", syslog_options, LOG_DAEMON);
#endif

    /* tell the library what debug level to emit and to send it to syslog */
    lws_set_log_level(debug_level, logFunction);


    lwsl_notice("libwebsockets - "
                "(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> - "
                "licensed under LGPL2.1");
#ifdef EXTERNAL_POLL
    max_poll_elements = getdtablesize();
    pollfds = malloc(max_poll_elements * sizeof (struct pollfd));
    fd_lookup = malloc(max_poll_elements * sizeof (int));
    if (pollfds == NULL || fd_lookup == NULL) {
        lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
        return -1;
    }
#endif

    info.iface = iface;
    info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
//  info.extensions = lws_get_internal_extensions();
#endif
    if (!use_ssl) 
    {
        info.ssl_cert_filepath = NULL;
        info.ssl_private_key_filepath = NULL;
    } else 
    {
        info.ssl_cert_filepath = LOCAL_RESOURCE_PATH"/libwebsockets-test-server.pem";
        info.ssl_private_key_filepath = LOCAL_RESOURCE_PATH"/libwebsockets-test-server.key.pem";
    }
    info.gid = -1;
    info.uid = -1;
    info.options = opts;

    context = lws_create_context(&info);
    if (context == NULL) 
    {
        lwsl_err("libwebsocket init failed");
        return -1;
    }

    return 0;
}

/******************************************************************************\
*
* Stop the socket server
*
\******************************************************************************/
void SolarPublishWebsocket::closeWebSocketServer()
{
    lws_context_destroy(context);

    lwsl_notice("libwebsockets-test-server exited cleanly");
    
#ifndef WIN32
    closelog();
#endif
}
