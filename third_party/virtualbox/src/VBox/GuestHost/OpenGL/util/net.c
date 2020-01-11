/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <memory.h>
#include <signal.h>

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <process.h>
#else
#include <unistd.h>
#endif

#include "cr_mem.h"
#include "cr_error.h"
#include "cr_string.h"
#include "cr_net.h"
#include "cr_netserver.h"
#include "cr_pixeldata.h"
#include "cr_bufpool.h"
#include "cr_threads.h"
#include "net_internals.h"


#define CR_MINIMUM_MTU 1024

#define CR_INITIAL_RECV_CREDITS ( 1 << 21 ) /* 2MB */

/* Allow up to four processes per node. . . */
#define CR_QUADRICS_LOWEST_RANK  0
#define CR_QUADRICS_HIGHEST_RANK 3

static struct {
    int                  initialized; /* flag */
    CRNetReceiveFuncList *recv_list;  /* what to do with arriving packets */
    CRNetCloseFuncList   *close_list; /* what to do when a client goes down */

    /* Number of connections using each type of interface: */
    int                  use_hgcm;

    int                  num_clients; /* total number of clients (unused?) */

    CRmutex              mutex;
} cr_net;



/**
 * Helper routine used by both crNetConnectToServer() and crNetAcceptClient().
 * Call the protocol-specific Init() and Connection() functions.
 *
 */
static void
InitConnection(CRConnection *conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
        )
{
#ifdef VBOX_WITH_HGCM
    cr_net.use_hgcm++;
    crVBoxHGCMInit(cr_net.recv_list, cr_net.close_list);
    crVBoxHGCMConnection(conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                         , pHgsmi
#endif
                         );
#endif
}



/**
 * Establish a connection with a server.
 * \param server  the server to connect to, in the form
 *                "protocol://servername:port" where the port specifier
 *                is optional and if the protocol is missing it is assumed
 *                to be "tcpip".
 * \param default_port  the port to connect to, if port not specified in the
 *                      server URL string.
 * \param mtu  desired maximum transmission unit size (in bytes)
 * \param broker  either 1 or 0 to indicate if connection is brokered through
 *                the mothership
 */
CRConnection * crNetConnectToServer( const char *server, int mtu, int broker
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
)
{
    CRConnection *conn;

    crDebug( "In crNetConnectToServer( \"%s\", mtu=%d, broker=%d )",
                     server, mtu, broker );

    CRASSERT( cr_net.initialized );

    if (mtu < CR_MINIMUM_MTU)
    {
        crError( "You tried to connect to server \"%s\" with an mtu of %d, "
                         "but the minimum MTU is %d", server, mtu, CR_MINIMUM_MTU );
    }

    conn = (CRConnection *) crCalloc( sizeof(*conn) );
    if (!conn)
        return NULL;

    /* init the non-zero fields */
    conn->type               = CR_NO_CONNECTION; /* we don't know yet */
    conn->recv_credits       = CR_INITIAL_RECV_CREDITS;
    conn->mtu                = mtu;
    conn->buffer_size        = mtu;
    conn->broker             = broker;

    crInitMessageList(&conn->messageList);

    /* now, just dispatch to the appropriate protocol's initialization functions. */
    InitConnection(conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , pHgsmi
#endif
            );

    if (!crNetConnect( conn ))
    {
        crDebug("crNetConnectToServer() failed, freeing the connection");
        crFreeMutex( &conn->messageList.lock );
        conn->Disconnect(conn);
        crFree( conn );
        return NULL;
    }

    crDebug( "Done connecting to %s (swapping=%d)", server, conn->swap );
    return conn;
}

/**
 * Send a message to the receiver that another connection is needed.
 * We send a CR_MESSAGE_NEWCLIENT packet, then call crNetServerConnect.
 */
void crNetNewClient( CRNetServer *ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
)
{
    crNetServerConnect( ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , pHgsmi
#endif
);
}


/**
 * Accept a connection from a client.
 * \param protocol  the protocol to use (such as "tcpip" or "gm")
 * \param hostname  optional hostname of the expected client (may be NULL)
 * \param port  number of the port to accept on
 * \param mtu  maximum transmission unit
 * \param broker  either 1 or 0 to indicate if connection is brokered through
 *                the mothership
 * \return  new CRConnection object, or NULL
 */
CRConnection *
crNetAcceptClient( const char *protocol, const char *hostname,
                                     unsigned short port, unsigned int mtu, int broker )
{
    CRConnection *conn;

    RT_NOREF(hostname);
    CRASSERT( cr_net.initialized );

    conn = (CRConnection *) crCalloc( sizeof( *conn ) );
    if (!conn)
        return NULL;

    /* init the non-zero fields */
    conn->type               = CR_NO_CONNECTION; /* we don't know yet */
    conn->recv_credits       = CR_INITIAL_RECV_CREDITS;
    conn->mtu                = mtu;
    conn->buffer_size        = mtu;
    conn->broker             = broker;

    crInitMessageList(&conn->messageList);

    /* now, just dispatch to the appropriate protocol's initialization functions. */
    crDebug("In crNetAcceptClient( protocol=\"%s\" port=%d mtu=%d )",
                    protocol, (int) port, (int) mtu);

    /* call the protocol-specific init routines */
      InitConnection(conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , NULL
#endif
              );

    crNetAccept(conn);
    return conn;
}


/**
 * Close and free given connection.
 */
void
crNetFreeConnection(CRConnection *conn)
{
    conn->Disconnect(conn);
    crFreeMutex( &conn->messageList.lock );
    crFree(conn);
}


/**
 * Start the ball rolling.  give functions to handle incoming traffic
 * (usually placing blocks on a queue), and a handler for dropped
 * connections.
 */
void crNetInit( CRNetReceiveFunc recvFunc, CRNetCloseFunc closeFunc )
{
    CRNetReceiveFuncList *rfl;
    CRNetCloseFuncList *cfl;

    if ( cr_net.initialized )
    {
        /*crDebug( "Networking already initialized!" );*/
    }
    else
    {
        cr_net.use_hgcm    = 0;
        cr_net.num_clients = 0;
        crInitMutex(&cr_net.mutex);

        cr_net.initialized = 1;
        cr_net.recv_list = NULL;
        cr_net.close_list = NULL;
    }

    if (recvFunc != NULL)
    {
        /* check if function is already in the list */
        for (rfl = cr_net.recv_list ; rfl ; rfl = rfl->next )
        {
            if (rfl->recv == recvFunc)
            {
                /* we've already seen this function -- do nothing */
                break;
            }
        }
        /* not in list, so insert at the head */
        if (!rfl)
        {
            rfl = (CRNetReceiveFuncList *) crAlloc( sizeof (*rfl ));
            rfl->recv = recvFunc;
            rfl->next = cr_net.recv_list;
            cr_net.recv_list = rfl;
        }
    }

    if (closeFunc != NULL)
    {
        /* check if function is already in the list */
        for (cfl = cr_net.close_list ; cfl ; cfl = cfl->next )
        {
            if (cfl->close == closeFunc)
            {
                /* we've already seen this function -- do nothing */
                break;
            }
        }
        /* not in list, so insert at the head */
        if (!cfl)
        {
            cfl = (CRNetCloseFuncList *) crAlloc( sizeof (*cfl ));
            cfl->close = closeFunc;
            cfl->next = cr_net.close_list;
            cr_net.close_list = cfl;
        }
    }
}

/* Free up stuff */
void crNetTearDown(void)
{
    CRNetReceiveFuncList *rfl;
    CRNetCloseFuncList *cfl;
    void *tmp;

    if (!cr_net.initialized) return;

    crLockMutex(&cr_net.mutex);

    /* Note, other protocols used by chromium should free up stuff too,
     * but VBox doesn't use them, so no other checks.
     */  
    if (cr_net.use_hgcm)
        crVBoxHGCMTearDown();

    for (rfl = cr_net.recv_list ; rfl ; rfl = (CRNetReceiveFuncList *) tmp )
    {
        tmp = rfl->next;
        crFree(rfl);
    }

    for (cfl = cr_net.close_list ; cfl ; cfl = (CRNetCloseFuncList *) tmp )
    {
        tmp = cfl->next;
        crFree(cfl);
    }

    cr_net.initialized = 0;

    crUnlockMutex(&cr_net.mutex);
    crFreeMutex(&cr_net.mutex);
}

CRConnection** crNetDump( int* num )
{
    CRConnection **c;

#ifdef VBOX_WITH_HGCM
    c = crVBoxHGCMDump( num );
    if ( c ) return c;
#endif

    *num = 0;
    return NULL;
}


/*
 * Allocate a network data buffer.  The size will be the mtu size specified
 * earlier to crNetConnectToServer() or crNetAcceptClient().
 *
 * Buffers that will eventually be transmitted on a connection
 * *must* be allocated using this interface.  This way, we can
 * automatically pin memory and tag blocks, and we can also use
 * our own buffer pool management.
 */
void *crNetAlloc( CRConnection *conn )
{
    CRASSERT( conn );
    return conn->Alloc( conn );
}


/**
 * This returns a buffer (which was obtained from crNetAlloc()) back
 * to the network layer so that it may be reused.
 */
void crNetFree( CRConnection *conn, void *buf )
{
    conn->Free( conn, buf );
}


void
crInitMessageList(CRMessageList *list)
{
    list->head = list->tail = NULL;
    list->numMessages = 0;
    crInitMutex(&list->lock);
    crInitCondition(&list->nonEmpty);
}


/**
 * Add a message node to the end of the message list.
 * \param list  the message list
 * \param msg   points to start of message buffer
 * \param len   length of message, in bytes
 * \param conn  connection associated with message (may be NULL)
 */
void 
crEnqueueMessage(CRMessageList *list, CRMessage *msg, unsigned int len,
                                 CRConnection *conn)
{
    CRMessageListNode *node;

    crLockMutex(&list->lock);

    node = (CRMessageListNode *) crAlloc(sizeof(CRMessageListNode));
    node->mesg = msg;
    node->len = len;
    node->conn = conn;
    node->next = NULL;

    /* insert at tail */
    if (list->tail)
        list->tail->next = node;
    else
        list->head = node;
    list->tail = node;

    list->numMessages++;

    crSignalCondition(&list->nonEmpty);
    crUnlockMutex(&list->lock);
}


/**
 * Remove first message node from message list and return it.
 * Don't block.
 * \return 1 if message was dequeued, 0 otherwise.
 */
static int
crDequeueMessageNoBlock(CRMessageList *list, CRMessage **msg,
                                                unsigned int *len, CRConnection **conn)
{
    int retval;

    crLockMutex(&list->lock);

    if (list->head) {
        CRMessageListNode *node = list->head;

        /* unlink the node */
        list->head = node->next;
        if (!list->head) {
            /* empty list */
            list->tail = NULL;
        }

        *msg = node->mesg;
        *len = node->len;
        if (conn)
            *conn = node->conn;

        list->numMessages--;

        crFree(node);
        retval = 1;
    }
    else {
        *msg = NULL;
        *len = 0;
        retval = 0;
    }

    crUnlockMutex(&list->lock);
    return retval;
}


/**
 * Remove message from tail of list.  Block until non-empty if needed.
 * \param list  the message list
 * \param msg   returns start of message
 * \param len   returns message length, in bytes
 * \param conn  returns connection associated with message (may be NULL)
 */
void
crDequeueMessage(CRMessageList *list, CRMessage **msg, unsigned int *len,
                                 CRConnection **conn)
{
    CRMessageListNode *node;

    crLockMutex(&list->lock);

    while (!list->head) {
        crWaitCondition(&list->nonEmpty, &list->lock);
    }

    node = list->head;

    /* unlink the node */
    list->head = node->next;
    if (!list->head) {
        /* empty list */
        list->tail = NULL;
    }

    *msg = node->mesg;
    CRASSERT((*msg)->header.type);
    *len = node->len;
    if (conn)
        *conn = node->conn;

    list->numMessages--;

    crFree(node);
    crUnlockMutex(&list->lock);
}



/**
 * Send a set of commands on a connection.  Pretty straightforward, just
 * error checking, byte counting, and a dispatch to the protocol's
 * "send" implementation.
 * The payload will be prefixed by a 4-byte length field.
 *
 * \param conn  the network connection
 * \param bufp  if non-null the buffer was provided by the network layer
 *              and will be returned to the 'free' pool after it's sent.
 * \param start  points to first byte to send, which must point to a CRMessage
 *               object!
 * \param len  number of bytes to send
 */
void
crNetSend(CRConnection *conn, void **bufp, const void *start, unsigned int len)
{
    CRMessage *msg = (CRMessage *) start;

    CRASSERT( conn );
    CRASSERT( len > 0 );
    if ( bufp ) {
        /* The region from [start .. start + len - 1] must lie inside the
         * buffer pointed to by *bufp.
         */
        CRASSERT( start >= *bufp );
        CRASSERT( (unsigned char *) start + len <=
                            (unsigned char *) *bufp + conn->buffer_size );
    }

#ifdef DEBUG
    if ( conn->send_credits > CR_INITIAL_RECV_CREDITS )
    {
        crError( "crNetSend: send_credits=%u, looks like there is a leak (max=%u)",
                         conn->send_credits, CR_INITIAL_RECV_CREDITS );
    }
#endif

    conn->total_bytes_sent += len;

    msg->header.conn_id = conn->id;
    conn->Send( conn, bufp, start, len );
}


/**
 * Like crNetSend(), but the network layer is free to discard the data
 * if something goes wrong.  In particular, the UDP layer might discard
 * the data in the event of transmission errors.
 */
void crNetBarf( CRConnection *conn, void **bufp,
                    const void *start, unsigned int len )
{
    CRMessage *msg = (CRMessage *) start;
    CRASSERT( conn );
    CRASSERT( len > 0 );
    CRASSERT( conn->Barf );
    if ( bufp ) {
        CRASSERT( start >= *bufp );
        CRASSERT( (unsigned char *) start + len <=
                (unsigned char *) *bufp + conn->buffer_size );
    }

#ifdef DEBUG
    if ( conn->send_credits > CR_INITIAL_RECV_CREDITS )
    {
        crError( "crNetBarf: send_credits=%u, looks like there is a "
                "leak (max=%u)", conn->send_credits,
                CR_INITIAL_RECV_CREDITS );
    }
#endif

    conn->total_bytes_sent += len;

    msg->header.conn_id = conn->id;
    conn->Barf( conn, bufp, start, len );
}


/**
 * Send a block of bytes across the connection without any sort of
 * header/length information.
 * \param conn  the network connection
 * \param buf  points to first byte to send
 * \param len  number of bytes to send
 */
void crNetSendExact( CRConnection *conn, const void *buf, unsigned int len )
{
  CRASSERT(conn->SendExact);
    conn->SendExact( conn, buf, len );
}


/**
 * Connect to a server, as specified by the 'name' and 'buffer_size' fields
 * of the CRNetServer parameter.
 * When done, the CrNetServer's conn field will be initialized.
 */
void crNetServerConnect( CRNetServer *ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
)
{
    ns->conn = crNetConnectToServer( ns->name,
                                     ns->buffer_size, 0
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , pHgsmi
#endif
            );
}

/**
 * Actually set up the specified connection.
 * Apparently, this is only called from the crNetConnectToServer function.
 */
int crNetConnect( CRConnection *conn )
{
    return conn->Connect( conn );
}


/**
 * Tear down a network connection (close the socket, etc).
 */
void crNetDisconnect( CRConnection *conn )
{
    conn->Disconnect( conn );
    crFreeMutex( &conn->messageList.lock );
    crFree( conn );
}


/**
 * Actually set up the specified connection.
 * Apparently, this is only called from the crNetConnectToServer function.
 */
void crNetAccept( CRConnection *conn)
{
    conn->Accept( conn);
}


/**
 * Do a blocking receive on a particular connection.  This only
 * really works for TCPIP, but it's really only used (right now) by
 * the mothership client library.
 * Read exactly the number of bytes specified (no headers/prefixes).
 */
void crNetSingleRecv( CRConnection *conn, void *buf, unsigned int len )
{
    /** @todo Remove after users have been identified and eliminated .*/
    crError( "Can't do a crNetSingleReceive on anything other than TCPIP." );
    conn->Recv( conn, buf, len );
}


/**
 * Receive a chunk of a CR_MESSAGE_MULTI_BODY/TAIL transmission.
 * \param conn  the network connection
 * \param msg  the incoming multi-part message
 * \param len  number of bytes in the message
 */
static void
crNetRecvMulti( CRConnection *conn, CRMessageMulti *msg, unsigned int len )
{
    CRMultiBuffer *multi = &(conn->multi);
    unsigned char *src, *dst;

    CRASSERT( len > sizeof(*msg) );
    len -= sizeof(*msg);

    /* Check if there's enough room in the multi-buffer to append 'len' bytes */
    if ( len + multi->len > multi->max )
    {
        if ( multi->max == 0 )
        {
            multi->len = conn->sizeof_buffer_header;
            multi->max = 8192;  /* arbitrary initial size */
        }
        /* grow the buffer by 2x until it's big enough */
        while ( len + multi->len > multi->max )
        {
            multi->max <<= 1;
        }
        crRealloc( &multi->buf, multi->max );
    }

    dst = (unsigned char *) multi->buf + multi->len;
    src = (unsigned char *) msg + sizeof(*msg);
    crMemcpy( dst, src, len );
    multi->len += len;

    if (msg->header.type == CR_MESSAGE_MULTI_TAIL)
    {
        /* OK, we've collected the last chunk of the multi-part message */
        conn->HandleNewMessage(
                conn,
                (CRMessage *) (((char *) multi->buf) + conn->sizeof_buffer_header),
                multi->len - conn->sizeof_buffer_header );

        /* clean this up before calling the user */
        multi->buf = NULL;
        multi->len = 0;
        multi->max = 0;
    }

    /* Don't do this too early! */
    conn->InstantReclaim( conn, (CRMessage *) msg );
}


/**
 * Increment the connection's send_credits by msg->credits.
 */
static void
crNetRecvFlowControl( CRConnection *conn,   CRMessageFlowControl *msg,
                                            unsigned int len )
{
    CRASSERT( len == sizeof(CRMessageFlowControl) );
    conn->send_credits += msg->credits;
    conn->InstantReclaim( conn, (CRMessage *) msg );
}

#ifdef IN_GUEST
/**
 * Called by the main receive function when we get a CR_MESSAGE_WRITEBACK
 * message.  Writeback is used to implement glGet*() functions.
 */
static void
crNetRecvWriteback( CRMessageWriteback *wb )
{
    int *writeback;
    crMemcpy( &writeback, &(wb->writeback_ptr), sizeof( writeback ) );
    (*writeback)--;
}


/**
 * Called by the main receive function when we get a CR_MESSAGE_READBACK
 * message.  Used to implement glGet*() functions.
 */
static void
crNetRecvReadback( CRMessageReadback *rb, unsigned int len )
{
    /* minus the header, the destination pointer,
     * *and* the implicit writeback pointer at the head. */

    int payload_len = len - sizeof( *rb );
    int *writeback;
    void *dest_ptr;
    crMemcpy( &writeback, &(rb->writeback_ptr), sizeof( writeback ) );
    crMemcpy( &dest_ptr, &(rb->readback_ptr), sizeof( dest_ptr ) );

    (*writeback)--;
    crMemcpy( dest_ptr, ((char *)rb) + sizeof(*rb), payload_len );
}
#endif

/**
 * This is used by the SPUs that do packing (such as Pack, Tilesort and
 * Replicate) to process ReadPixels messages.  We can't call this directly
 * from the message loop below because the SPU's have other housekeeping
 * to do for ReadPixels (such as decrementing counters).
 */
void
crNetRecvReadPixels( const CRMessageReadPixels *rp, unsigned int len )
{
   int payload_len = len - sizeof( *rp );
   char *dest_ptr;
   const char *src_ptr = (const char *) rp + sizeof(*rp);

   /* set dest_ptr value */
   crMemcpy( &(dest_ptr), &(rp->pixels), sizeof(dest_ptr));

   /* store pixel data in app's memory */
   if (rp->alignment == 1 &&
       rp->skipRows == 0 &&
       rp->skipPixels == 0 &&
       (rp->rowLength == 0 || rp->rowLength == rp->width)) {
      /* no special packing is needed */
      crMemcpy( dest_ptr, src_ptr, payload_len );
   }
   else {
      /* need special packing */
      CRPixelPackState packing;
      packing.skipRows = rp->skipRows;
      packing.skipPixels = rp->skipPixels;
      packing.alignment = rp->alignment;
      packing.rowLength = rp->rowLength;
      packing.imageHeight = 0;
      packing.skipImages = 0;
      packing.swapBytes = GL_FALSE;
      packing.psLSBFirst = GL_FALSE;
      crPixelCopy2D( rp->width, rp->height,
             dest_ptr, rp->format, rp->type, &packing,
             src_ptr, rp->format, rp->type, /*unpacking*/NULL);
   }
}



/**
 * If an incoming message is not consumed by any of the connection's
 * receive callbacks, this function will get called.
 *
 * XXX Make this function static???
 */
void
crNetDefaultRecv( CRConnection *conn, CRMessage *msg, unsigned int len )
{
    CRMessage *pRealMsg;

    pRealMsg = (msg->header.type!=CR_MESSAGE_REDIR_PTR) ? msg : (CRMessage*) msg->redirptr.pMessage;

    switch (pRealMsg->header.type)
    {
        case CR_MESSAGE_GATHER:
            break;
        case CR_MESSAGE_MULTI_BODY:
        case CR_MESSAGE_MULTI_TAIL:
            crNetRecvMulti( conn, &(pRealMsg->multi), len );
            return;
        case CR_MESSAGE_FLOW_CONTROL:
            crNetRecvFlowControl( conn, &(pRealMsg->flowControl), len );
            return;
        case CR_MESSAGE_OPCODES:
        case CR_MESSAGE_OOB:
            break;
        case CR_MESSAGE_READ_PIXELS:
            WARN(( "Can't handle read pixels" ));
            return;
        case CR_MESSAGE_WRITEBACK:
#ifdef IN_GUEST
            crNetRecvWriteback( &(pRealMsg->writeback) );
#else
            WARN(("CR_MESSAGE_WRITEBACK not expected\n"));
#endif
            return;
        case CR_MESSAGE_READBACK:
#ifdef IN_GUEST
            crNetRecvReadback( &(pRealMsg->readback), len );
#else
            WARN(("CR_MESSAGE_READBACK not expected\n"));
#endif
            return;
        case CR_MESSAGE_CRUT:
            /* nothing */
            break;
        default:
            /* We can end up here if anything strange happens in
             * the GM layer.  In particular, if the user tries to
             * send unpinned memory over GM it gets sent as all
             * 0xAA instead.  This can happen when a program exits
             * ungracefully, so the GM is still DMAing memory as
             * it is disappearing out from under it.  We can also
             * end up here if somebody adds a message type, and
             * doesn't put it in the above case block.  That has
             * an obvious fix. */
            {
                char string[128];
                crBytesToString( string, sizeof(string), msg, len );
                WARN(("crNetDefaultRecv: received a bad message: type=%d buf=[%s]\n"
                                "Did you add a new message type and forget to tell "
                                "crNetDefaultRecv() about it?\n",
                                msg->header.type, string ));
            }
    }

    /* If we make it this far, it's not a special message, so append it to
     * the end of the connection's list of received messages.
     */
    crEnqueueMessage(&conn->messageList, msg, len, conn);
}


/**
 * Default handler for receiving data.  Called via crNetRecv().
 * Typically, the various implementations of the network layer call this.
 * \param msg this is the address of the message (of <len> bytes) the
 *            first part of which is a CRMessage union.
 */
void
crNetDispatchMessage( CRNetReceiveFuncList *rfl, CRConnection *conn,
                                            CRMessage *msg, unsigned int len )
{
    for ( ; rfl ; rfl = rfl->next)
    {
        if (rfl->recv( conn, msg, len ))
        {
            /* Message was consumed by somebody (maybe a SPU).
             * All done.
             */
            return;
        }
    }
    /* Append the message to the connection's message list.  It'll be
     * consumed later (by crNetPeekMessage or crNetGetMessage and
     * then freed with a call to crNetFree()).  At this point, the buffer
     * *must* have been allocated with crNetAlloc!
     */
    crNetDefaultRecv( conn, msg, len );
}


/**
 * Return number of messages queued up on the given connection.
 */
int
crNetNumMessages(CRConnection *conn)
{
    return conn->messageList.numMessages;
}


/**
 * Get the next message in the connection's message list.  These are
 * message that have already been received.  We do not try to read more
 * bytes from the network connection.
 *
 * The crNetFree() function should be called when finished with the message!
 *
 * \param conn  the network connection
 * \param message  returns a pointer to the next message
 * \return length of message (header + payload, in bytes)
 */
unsigned int
crNetPeekMessage( CRConnection *conn, CRMessage **message )
{
    unsigned int len;
    CRConnection *dummyConn = NULL;
    if (crDequeueMessageNoBlock(&conn->messageList, message, &len, &dummyConn))
        return len;
    else
        return 0;
}


/**
 * Get the next message from the given network connection.  If there isn't
 * one already in the linked list of received messages, call crNetRecv()
 * until we get something.
 *
 * \param message returns pointer to the message
 * \return total length of message (header + payload, in bytes)
 */
unsigned int
crNetGetMessage( CRConnection *conn, CRMessage **message )
{
    /* Keep getting work to do */
    for (;;)
    {
        int len = crNetPeekMessage( conn, message );
        if (len)
            return len;
        crNetRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                conn
#endif
                );
    }

#if !defined(WINDOWS) && !defined(IRIX) && !defined(IRIX64)
    /* silence compiler */
    return 0;
#endif
}


/**
 * Read a \n-terminated string from a connection.  Replace the \n with \0.
 * Useful for reading from the mothership.
 * \note This is an extremely inefficient way to read a string!
 *
 * \param conn  the network connection
 * \param buf  buffer in which to place results
 */
void crNetReadline( CRConnection *conn, void *buf )
{
    RT_NOREF(buf);

    if (!conn || conn->type == CR_NO_CONNECTION)
        return;

    /** @todo Remove after users have been found and eliminated. */
    crError( "Can't do a crNetReadline on anything other than TCPIP (%d).",conn->type );
}

#ifdef IN_GUEST
uint32_t crNetHostCapsGet(void)
{
#ifdef VBOX_WITH_HGCM
    if ( cr_net.use_hgcm )
        return crVBoxHGCMHostCapsGet();
#endif
    WARN(("HostCaps supportted for HGCM only!"));
    return 0;
}
#endif

/**
 * The big boy -- call this function to see (non-blocking) if there is
 * any pending work.  If there is, the networking layer's "work received"
 * handler will be called, so this function only returns a flag.  Work
 * is assumed to be placed on queues for processing by the handler.
 */
int crNetRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        CRConnection *conn
#else
        void
#endif
        )
{
    int found_work = 0;

#ifdef VBOX_WITH_HGCM
    if ( cr_net.use_hgcm )
        found_work += crVBoxHGCMRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                    conn
#endif
                );
#endif

    return found_work;
}

