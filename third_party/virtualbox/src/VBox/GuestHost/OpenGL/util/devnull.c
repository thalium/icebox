/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_net.h"
#include "cr_mem.h"
#include "cr_error.h"
#include "net_internals.h"

static void
crDevnullWriteExact( CRConnection *conn, const void *buf, unsigned int len )
{
	(void) conn;
	(void) buf;
	(void) len;
}

static void *
crDevnullAlloc( CRConnection *conn )
{
	return crAlloc( conn->buffer_size );
}

static void
crDevnullSingleRecv( CRConnection *conn, void *buf, unsigned int len )
{
	crError( "You can't receive data on a devnull connection!" );
	(void) conn;
	(void) buf;
	(void) len;
}

static void
crDevnullFree( CRConnection *conn, void *buf )
{
	crFree( buf );
	(void) conn;
}

static void
crDevnullSend( CRConnection *conn, void **bufp,
				 const void *start, unsigned int len )
{
	
	if (bufp)
	{
		/* We're sending something we've allocated.  It's now ours. 
		 * If the callers wants to send something else, he'll allocate 
		 * something else. 
		 * 
		 * ENFORCE IT! */

		crDevnullFree( conn, *bufp );
	}
	(void) conn;
	(void) bufp;
	(void) start;
	(void) len;
}

int
crDevnullRecv( void )
{
	crError( "You can't receive data on a DevNull network, stupid." );
	return 0;
}

void
crDevnullInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu )
{
	(void) rfl;
	(void) cfl;
	(void) mtu;
}

static void
crDevnullAccept( CRConnection *conn, const char *hostname, unsigned short port )
{
	crError( "Well, you *could* accept a devnull client, but you'd be disappointed. ");
	(void) conn;
	(void) port;
        (void) hostname;
}

static int
crDevnullDoConnect( CRConnection *conn )
{
	(void) conn; 
	return 1;
}

static void
crDevnullDoDisconnect( CRConnection *conn )
{
	(void) conn;
}

void crDevnullConnection( CRConnection *conn )
{
	conn->type  = CR_DROP_PACKETS;
	conn->Alloc = crDevnullAlloc;
	conn->Send  = crDevnullSend;
	conn->SendExact  = crDevnullWriteExact;
	conn->Recv  = crDevnullSingleRecv;
	conn->Free  = crDevnullFree;
	conn->Accept = crDevnullAccept;
	conn->Connect = crDevnullDoConnect;
	conn->Disconnect = crDevnullDoDisconnect;
	conn->actual_network = 0;
}

CRConnection** crDevnullDump( int * num )
{
	*num = 0;
	return NULL;
}
