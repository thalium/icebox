#ifndef NET_INTERNALS_H
#define NET_INTERNALS_H

#include "cr_bufpool.h"
#include "cr_threads.h"

#ifndef WINDOWS
#include <sys/time.h>
#endif

/*
 * DevNull network interface
 */
extern void crDevnullInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crDevnullConnection( CRConnection *conn );
extern int crDevnullRecv( void );
extern CRConnection** crDevnullDump( int *num );


/*
 * File network interface
 */
extern void crFileInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crFileConnection( CRConnection *conn );
extern int crFileRecv( void );
extern CRConnection** crFileDump( int *num );


/*
 * TCP/IP network interface
 */
typedef enum {
    CRTCPIPMemory,
    CRTCPIPMemoryBig
} CRTCPIPBufferKind;

#define CR_TCPIP_BUFFER_MAGIC 0x89134532

typedef struct CRTCPIPBuffer {
    unsigned int          magic;
    CRTCPIPBufferKind     kind;
    unsigned int          len;
    unsigned int          allocated;
    unsigned int          pad;  /* may be clobbered by crTCPIPSend() */
} CRTCPIPBuffer;

typedef struct {
    int                  initialized;
    int                  num_conns;
    CRConnection         **conns;
    CRBufferPool         *bufpool;
#ifdef CHROMIUM_THREADSAFE
    CRmutex              mutex;
    CRmutex              recvmutex;
#endif
    CRNetReceiveFuncList *recv_list;
    CRNetCloseFuncList *close_list;
    CRSocket             server_sock;
} cr_tcpip_data;

extern cr_tcpip_data cr_tcpip;

extern void crTCPIPInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crTCPIPConnection( CRConnection *conn );
extern int crTCPIPRecv( void );
extern CRConnection** crTCPIPDump( int *num );
extern int crTCPIPDoConnect( CRConnection *conn );
extern void crTCPIPDoDisconnect( CRConnection *conn );
extern int crTCPIPErrno( void );
extern char *crTCPIPErrorString( int err );
extern void crTCPIPAccept( CRConnection *conn, const char *hostname, unsigned short port );
extern void crTCPIPWriteExact( CRConnection *conn, const void *buf, unsigned int len );
extern void crTCPIPFree( CRConnection *conn, void *buf );
extern void *crTCPIPAlloc( CRConnection *conn );
extern void crTCPIPReadExact( CRConnection *conn, void *buf, unsigned int len );
extern int __tcpip_write_exact( CRSocket sock, const void *buf, unsigned int len );
extern int __tcpip_read_exact( CRSocket sock, void *buf, unsigned int len );
extern void __tcpip_dead_connection( CRConnection *conn );
extern int __crSelect( int n, fd_set *readfds, int sec, int usec );


/*
 * UDP network interface
 */
extern void crUDPTCPIPInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crUDPTCPIPConnection( CRConnection *conn );
extern int crUDPTCPIPRecv( void );

/*
 * VirtualBox HGCM
 */
#ifdef VBOX_WITH_HGCM
extern void crVBoxHGCMInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crVBoxHGCMConnection( CRConnection *conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , struct VBOXUHGSMI *pHgsmi
#endif
        );
extern int crVBoxHGCMRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        CRConnection *conn
#else
        void
#endif
        );
#ifdef IN_GUEST
extern uint32_t crVBoxHGCMHostCapsGet(void);
#endif
extern CRConnection** crVBoxHGCMDump( int *num );
extern void crVBoxHGCMTearDown(void);
#endif

/*
 * TEAC network interface
 */
#ifdef TEAC_SUPPORT
extern void crTeacInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl,
            unsigned int mtu );
extern void crTeacConnection( CRConnection *conn );
extern int crTeacRecv( void );
extern void crTeacSetRank( int );
extern void crTeacSetContextRange( int, int );
extern void crTeacSetNodeRange( const char *, const char * );
extern void crTeacSetKey( const unsigned char *key, const int keyLength );
#endif /* TEAC_SUPPORT */


/*
 * Tcscomm network interface
 */
#ifdef TCSCOMM_SUPPORT
extern void crTcscommInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl,
                           unsigned int mtu );
extern void crTcscommConnection( CRConnection *conn );
extern int crTcscommRecv( void );
#endif /* TCSCOMM_SUPPORT */


/*
 * SDP network interface
 */
#ifdef SDP_SUPPORT
extern const char *crGetSDPHostnameSuffix(void);
extern void crSDPInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crSDPConnection( CRConnection *conn );
extern int crSDPRecv( void );
extern CRConnection** crSDPDump( int *num );
#endif /* SDP_SUPPORT */


/*
 * Infiniband network interface
 */
#ifdef IB_SUPPORT
extern void crIBInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crIBConnection( CRConnection *conn );
extern int crIBRecv( void );
extern CRConnection** crIBDump( int *num );
#endif /* IB_SUPPORT */


/*
 * GM network interface
 */
#ifdef GM_SUPPORT
extern void crGmInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu );
extern void crGmConnection( CRConnection *conn );
extern int crGmRecv( void );
extern CRConnection** crGmDump( int *num );
extern int crGmDoConnect( CRConnection *conn );
extern void crGmDoDisconnect( CRConnection *conn );
extern int crGmErrno( void );
extern char *crGmErrorString( int err );
extern void crGmAccept( CRConnection *conn, const char *hostname, unsigned short port );
extern void crGmSendExact( CRConnection *conn, const void *buf, unsigned int len );
extern void crGmFree( CRConnection *conn, void *buf );
extern void *crGmAlloc( CRConnection *conn );
extern void crGmReadExact( CRConnection *conn, void *buf, unsigned int len );
extern void crGmBogusRecv( CRConnection *conn, void *buf, unsigned int len );
extern void crGmHandleNewMessage( CRConnection *conn, CRMessage *msg, unsigned int len );
extern void crGmInstantReclaim( CRConnection *conn, CRMessage *msg );
extern unsigned int crGmNodeId( void );
extern unsigned int crGmPortNum( void );
#endif /* GM_SUPPORT */


extern CRConnection** crNetDump( int *num );

#endif /* NET_INTERNALS_H */
