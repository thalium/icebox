/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_NET_H
#define CR_NET_H

#ifdef RT_OS_WINDOWS
# include <iprt/win/winsock.h>
#endif

#include "cr_protocol.h"
#include "cr_threads.h"

#include <iprt/types.h>
#include <iprt/thread.h>
#include <iprt/list.h>
    
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CRConnection CRConnection;

typedef enum {
    CR_NO_CONNECTION,
    CR_VBOXHGCM,
    CR_DROP_PACKETS
} CRConnectionType;

typedef void (*CRVoidFunc)( void );
typedef int (*CRNetReceiveFunc)( CRConnection *conn, CRMessage *msg, unsigned int len );
typedef int (*CRNetConnectFunc)( CRConnection *conn );
typedef void (*CRNetCloseFunc)( unsigned int sender_id );

typedef struct __recvFuncList {
    CRNetReceiveFunc recv;
    struct __recvFuncList *next;
} CRNetReceiveFuncList;

typedef struct __closeFuncList {
    CRNetCloseFunc close;
    struct __closeFuncList *next;
} CRNetCloseFuncList;

typedef struct __messageListNode {
    CRMessage *mesg;    /* the actual message (header + payload) */
    unsigned int len;   /* length of message (header + payload) */
    CRConnection *conn; /* some messages are assoc. with specific connections*/
    struct __messageListNode *next;  /* next in list */
} CRMessageListNode;

typedef struct {
    CRMessageListNode *head, *tail;
    int numMessages;
    CRmutex lock;
    CRcondition nonEmpty;
} CRMessageList;


/**
 * Used to accumulate CR_MESSAGE_MULTI_BODY/TAIL chunks into one big buffer.
 */
typedef struct CRMultiBuffer {
    unsigned int  len;  /* current length (<= max) (with sizeof_buffer_header) */
    unsigned int  max;  /* size in bytes of data buffer */
    void         *buf;  /* data buffer */
} CRMultiBuffer;

#ifdef VBOX_WITH_CRHGSMI
# ifdef IN_GUEST
typedef struct CRVBOXHGSMI_CLIENT {
    struct VBOXUHGSMI *pHgsmi;
    struct VBOXUHGSMI_BUFFER *pCmdBuffer;
    struct VBOXUHGSMI_BUFFER *pHGBuffer;
    void *pvHGBuffer;
    struct CRBufferPool_t *bufpool;
} CRVBOXHGSMI_CLIENT, *PCRVBOXHGSMI_CLIENT;
#endif /* IN_GUEST */
#endif /* #ifdef VBOX_WITH_CRHGSMI */
/**
 * Chromium network connection (bidirectional).
 */
struct CRConnection {
    int ignore;
    CRConnectionType type;
    unsigned int id;         /* obtained from the mothership (if brokered) */

    /* List of messages that we've received on the network connection but
     * nobody has yet consumed.
     */
    CRMessageList messageList;

    CRMultiBuffer multi;

    unsigned int mtu;        /* max transmission unit size (in bytes) */
    unsigned int buffer_size;
    unsigned int krecv_buf_size;
    int broker;              /* is connection brokered through mothership? */
    int threaded;            /* is this a threaded connection? */
    int swap;
    int actual_network;      /* is this a real network? */

    unsigned char *userbuf;
    int userbuf_len;

    /* To allocate a data buffer of size conn->buffer_size bytes */
    void *(*Alloc)( CRConnection *conn );
    /* To indicate the client's done with a data buffer */
    void  (*Free)( CRConnection *conn, void *buf );
    /* To send a data buffer.  If bufp is non-null, it must have been obtained
     * from Alloc() and it'll be freed when Send() returns.
     */
    void  (*Send)( CRConnection *conn, void **buf, const void *start, unsigned int len );
    /* To send a data buffer than can optionally be dropped on the floor */
    void  (*Barf)( CRConnection *conn, void **buf, const void *start, unsigned int len );
    /* To send 'len' bytes from buffer at 'start', no funny business */
    void  (*SendExact)( CRConnection *conn, const void *start, unsigned int len );
    /* To receive data.  'len' bytes will be placed into 'buf'. */
    void  (*Recv)( CRConnection *conn, void *buf, unsigned int len );
    /* To receive one message on the connection */
    void  (*RecvMsg)( CRConnection *conn );
    /* What's this??? */
    void  (*InstantReclaim)( CRConnection *conn, CRMessage *mess );
    /* Called when a full CR_MESSAGE_MULTI_HEAD/TAIL message has been received */
    void  (*HandleNewMessage)( CRConnection *conn, CRMessage *mess, unsigned int len );
    /* To accept a new connection from a client */
    void  (*Accept)( CRConnection *conn);
    /* To connect to a server (return 0 if error, 1 if success) */
    int  (*Connect)( CRConnection *conn );
    /* To disconnect from a server */
    void  (*Disconnect)( CRConnection *conn );

    unsigned int sizeof_buffer_header;

    /* logging */
    int total_bytes_sent;
    int total_bytes_recv;
    int recv_count;
    int opcodes_count;

    /* credits for flow control */
    int send_credits;
    int recv_credits;

    /* VBox HGCM */
    int      index;
    uint32_t u32ClientID;
    uint8_t  *pBuffer;
    uint32_t cbBuffer;
    uint8_t  *pHostBuffer;
    uint32_t cbHostBufferAllocated;
    uint32_t cbHostBuffer;
#ifdef IN_GUEST
    uint32_t u32InjectClientID;
# ifdef VBOX_WITH_CRHGSMI
    CRVBOXHGSMI_CLIENT HgsmiClient;
    struct VBOXUHGSMI *pExternalHgsmi;
# endif
#else
# ifdef VBOX_WITH_CRHGSMI
    struct _crclient *pClient; /* back reference, just for simplicity */
    CRVBOXHGSMI_CMDDATA CmdData;
# endif
    RTLISTANCHOR PendingMsgList;
#endif
    /* Used on host side to indicate that we are not allowed to store above pointers for later use
     * in crVBoxHGCMReceiveMessage. As those messages are going to be processed after the corresponding 
     * HGCM call is finished and memory is freed. So we have to store a copy.
     * This happens when message processing for client associated with this connection 
     * is blocked by another client, which has send us glBegin call and we're waiting to receive glEnd.
     */
    uint8_t  allow_redir_ptr;

    uint32_t vMajor, vMinor; /*Protocol version*/
};


/*
 * Network functions
 */
extern DECLEXPORT(int) crGetHostname( char *buf, unsigned int len );

extern DECLEXPORT(void) crNetInit( CRNetReceiveFunc recvFunc, CRNetCloseFunc closeFunc );
extern DECLEXPORT(void) crNetTearDown(void);

extern DECLEXPORT(void) *crNetAlloc( CRConnection *conn );
extern DECLEXPORT(void) crNetFree( CRConnection *conn, void *buf );

extern DECLEXPORT(void) crNetAccept( CRConnection *conn );
extern DECLEXPORT(int) crNetConnect( CRConnection *conn );
extern DECLEXPORT(void) crNetDisconnect( CRConnection *conn );
extern DECLEXPORT(void) crNetFreeConnection( CRConnection *conn );

extern DECLEXPORT(void) crNetSend( CRConnection *conn, void **bufp, const void *start, unsigned int len );
extern DECLEXPORT(void) crNetBarf( CRConnection *conn, void **bufp, const void *start, unsigned int len );
extern DECLEXPORT(void) crNetSendExact( CRConnection *conn, const void *start, unsigned int len );
extern DECLEXPORT(void) crNetSingleRecv( CRConnection *conn, void *buf, unsigned int len );
extern DECLEXPORT(unsigned int) crNetGetMessage( CRConnection *conn, CRMessage **message );
extern DECLEXPORT(unsigned int) crNetPeekMessage( CRConnection *conn, CRMessage **message );
extern DECLEXPORT(int) crNetNumMessages(CRConnection *conn);
extern DECLEXPORT(void) crNetReadline( CRConnection *conn, void *buf );
extern DECLEXPORT(int) crNetRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                CRConnection *conn
#else
                void
#endif
        );
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
#define CR_WRITEBACK_WAIT(_conn, _writeback) do { \
        while (_writeback) {     \
            RTThreadYield();     \
            crNetRecv(_conn);    \
        }                        \
    } while (0)
#else
#define CR_WRITEBACK_WAIT(_conn, _writeback) do { \
        while (_writeback) { \
            RTThreadYield(); \
            crNetRecv();     \
        }                    \
    } while (0)

#endif
#ifdef IN_GUEST
extern DECLEXPORT(uint32_t) crNetHostCapsGet(void);
#endif
extern DECLEXPORT(void) crNetDefaultRecv( CRConnection *conn, CRMessage *msg, unsigned int len );
extern DECLEXPORT(void) crNetDispatchMessage( CRNetReceiveFuncList *rfl, CRConnection *conn, CRMessage *msg, unsigned int len );

extern DECLEXPORT(CRConnection *) crNetConnectToServer( const char *server, int mtu, int broker
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , struct VBOXUHGSMI *pHgsmi
#endif
);
extern DECLEXPORT(CRConnection *) crNetAcceptClient( const char *protocol, const char *hostname, unsigned short port, unsigned int mtu, int broker );


extern DECLEXPORT(void) crInitMessageList(CRMessageList *list);
extern DECLEXPORT(void) crEnqueueMessage(CRMessageList *list, CRMessage *msg, unsigned int len, CRConnection *conn);
extern DECLEXPORT(void) crDequeueMessage(CRMessageList *list, CRMessage **msg, unsigned int *len, CRConnection **conn);

extern DECLEXPORT(void) crNetRecvReadPixels( const CRMessageReadPixels *rp, unsigned int len );


/*
 * Socket callback facility
 */
#define CR_SOCKET_CREATE 1
#define CR_SOCKET_DESTROY 2
typedef void (*CRSocketCallbackProc)(int mode, int socket);
extern DECLEXPORT(void) crRegisterSocketCallback(int mode, CRSocketCallbackProc proc);


#ifdef __cplusplus
}
#endif

#endif /* CR_NET_H */
