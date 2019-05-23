/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif


#include "cr_error.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_bufpool.h"
#include "cr_net.h"
#include "cr_endian.h"
#include "cr_threads.h"
#include "net_internals.h"

typedef enum {
	CRFileMemory,
	CRFileMemoryBig
} CRFileBufferKind;

#define CR_FILE_BUFFER_MAGIC 0x89134539

#ifndef WINDOWS
#define O_BINARY 0
#endif

typedef struct CRFileBuffer {
	unsigned int          magic;
	CRFileBufferKind     kind;
	unsigned int          len;
	unsigned int          allocated;
	unsigned int          pad;
} CRFileBuffer;

static struct {
	int                  initialized;
	int                  num_conns;
	CRConnection         **conns;
	CRBufferPool         *bufpool;
#ifdef CHROMIUM_THREADSAFE
	CRmutex              mutex;
#endif
	CRNetReceiveFuncList *recv_list;
	CRNetCloseFuncList   *close_list;
} cr_file;

static void
crFileReadExact( CRConnection *conn, void *buf, unsigned int len )
{
	char *dst = (char *) buf;

	while ( len > 0 )
	{
		int num_read = read( conn->fd, buf, len );

		if ( num_read < 0 )
		{
			crError( "Bad bad bad file error!" );
		}
		if (num_read == 0)
		{
			crError( "END OF FILE!" );
		}

		dst += num_read;
		len -= num_read;
	}
}

static void
crFileWriteExact( CRConnection *conn, const void *buf, unsigned int len )
{
	int retval = write( conn->fd, buf, len );
	if ( retval < (int) len )
	{
		crError( "crFileWriteExact: %s (tried to write %d bytes, actually wrote %d)", conn->filename, len, retval );
	}
}

static void
crFileAccept( CRConnection *conn, const char *hostname, unsigned short port )
{
	(void)hostname;
	conn->file_direction = CR_FILE_READ;
	conn->fd = open( conn->filename, O_RDONLY | O_BINARY );
	if (conn->fd < 0)
	{
		crError( "Couldn't open %s for reading!", conn->filename );
	}
	(void) port;
}

static void
*crFileAlloc( CRConnection *conn )
{
	CRFileBuffer *buf;

#ifdef CHROMIUM_THREADSAFE
	crLockMutex(&cr_file.mutex);
#endif

	buf  = (CRFileBuffer *) crBufferPoolPop( cr_file.bufpool, conn->buffer_size );

	if ( buf == NULL )
	{
		crDebug( "Buffer pool was empty, so I allocated %d bytes", 
			(int)(sizeof(CRFileBuffer) + conn->buffer_size) );
		buf = (CRFileBuffer *) 
			crAlloc( sizeof(CRFileBuffer) + conn->buffer_size );
		buf->magic = CR_FILE_BUFFER_MAGIC;
		buf->kind  = CRFileMemory;
		buf->pad   = 0;
		buf->allocated = conn->buffer_size;
	}

#ifdef CHROMIUM_THREADSAFE
	crUnlockMutex(&cr_file.mutex);
#endif

	return (void *)( buf + 1 );
}

static void
crFileSingleRecv( CRConnection *conn, void *buf, unsigned int len )
{
	crFileReadExact( conn, buf, len );
}

static void
crFileSend( CRConnection *conn, void **bufp, const void *start, unsigned int len )
{
	CRFileBuffer *file_buffer;
	unsigned int      *lenp;

	if ( bufp == NULL )
	{
		/* we are doing synchronous sends from user memory, so no need
		 * to get fancy.  Simply write the length & the payload and
		 * return. */
		if (conn->swap)
		{
			len = SWAP32(len);
		}
		crFileWriteExact( conn, &len, sizeof(len) );
		crFileWriteExact( conn, start, len );
		return;
	}

	file_buffer = (CRFileBuffer *)(*bufp) - 1;

	CRASSERT( file_buffer->magic == CR_FILE_BUFFER_MAGIC );

	/* All of the buffers passed to the send function were allocated
	 * with crFileAlloc(), which includes a header with a 4 byte
	 * length field, to insure that we always have a place to write
	 * the length field, even when start == *bufp. */
	lenp = (unsigned int *) start - 1;
	*lenp = len;

	crFileWriteExact(conn, lenp, len + sizeof(int) );

	/* reclaim this pointer for reuse and try to keep the client from
		 accidentally reusing it directly */
#ifdef CHROMIUM_THREADSAFE
	crLockMutex(&cr_file.mutex);
#endif
	crBufferPoolPush( cr_file.bufpool, file_buffer, conn->buffer_size );
#ifdef CHROMIUM_THREADSAFE
	crUnlockMutex(&cr_file.mutex);
#endif
	*bufp = NULL;
}

static void
crFileFree( CRConnection *conn, void *buf )
{
	CRFileBuffer *file_buffer = (CRFileBuffer *) buf - 1;

	CRASSERT( file_buffer->magic == CR_FILE_BUFFER_MAGIC );
	conn->recv_credits += file_buffer->len;

	switch ( file_buffer->kind )
	{
		case CRFileMemory:
#ifdef CHROMIUM_THREADSAFE
			crLockMutex(&cr_file.mutex);
#endif
			crBufferPoolPush( cr_file.bufpool, file_buffer, conn->buffer_size );
#ifdef CHROMIUM_THREADSAFE
			crUnlockMutex(&cr_file.mutex);
#endif
			break;

		case CRFileMemoryBig:
			crFree( file_buffer );
			break;

		default:
			crError( "Weird buffer kind trying to free in crFileFree: %d", file_buffer->kind );
	}
}

int
crFileRecv( void )
{
	CRMessage *msg;
	int i;

	if (!cr_file.num_conns)
	{
		return 0;
	}
	for ( i = 0; i < cr_file.num_conns; i++ )
	{
		CRFileBuffer *file_buffer;
		unsigned int   len;
		CRConnection  *conn = cr_file.conns[i];

		crFileReadExact( conn, &len, sizeof( len ) );

		CRASSERT( len > 0 );

		if ( len <= conn->buffer_size )
		{
			file_buffer = (CRFileBuffer *) crFileAlloc( conn ) - 1;
		}
		else
		{
			file_buffer = (CRFileBuffer *) 
				crAlloc( sizeof(*file_buffer) + len );
			file_buffer->magic = CR_FILE_BUFFER_MAGIC;
			file_buffer->kind  = CRFileMemoryBig;
			file_buffer->pad   = 0;
		}

		file_buffer->len = len;

		crFileReadExact( conn, file_buffer + 1, len );

		conn->recv_credits -= len;

		msg = (CRMessage *) (file_buffer + 1);
		crNetDispatchMessage( cr_file.recv_list, conn, msg, len );

		/* CR_MESSAGE_OPCODES is freed in
		 * crserverlib/server_stream.c 
		 *
		 * OOB messages are the programmer's problem.  -- Humper 12/17/01 */
		if (msg->header.type != CR_MESSAGE_OPCODES && msg->header.type != CR_MESSAGE_OOB)
		{
			crFileFree( conn, file_buffer + 1 );
		}
	}

	return 1;
}

static void
crFileHandleNewMessage( CRConnection *conn, CRMessage *msg,
		unsigned int len )
{
	CRFileBuffer *buf = ((CRFileBuffer *) msg) - 1;

	/* build a header so we can delete the message later */
	buf->magic = CR_FILE_BUFFER_MAGIC;
	buf->kind  = CRFileMemory;
	buf->len   = len;
	buf->pad   = 0;

	crNetDispatchMessage( cr_file.recv_list, conn, msg, len );
}

static void
crFileInstantReclaim( CRConnection *conn, CRMessage *mess )
{
	crFileFree( conn, mess );
}

void
crFileInit( CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu )
{
	(void) mtu;

	cr_file.recv_list = rfl;
	cr_file.close_list = cfl;

	if (cr_file.initialized)
	{
		return;
	}

	cr_file.num_conns = 0;
	cr_file.conns     = NULL;
	
#ifdef CHROMIUM_THREADSAFE
	crInitMutex(&cr_file.mutex);
#endif
	cr_file.bufpool = crBufferPoolInit(16);


	cr_file.initialized = 1;
}

static int
crFileDoConnect( CRConnection *conn )
{
	conn->file_direction = CR_FILE_WRITE;
	/* NOTE: the third parameter (file permissions) is only used/required when
	 * we specify O_CREAT as part of the flags.  The permissions will be
	 * masked according to the effective user's umask setting.
	 */
#ifdef WINDOWS
	conn->fd = open( conn->filename, O_CREAT | O_WRONLY | O_BINARY |
									 S_IREAD | S_IWRITE);
#else
	conn->fd = open( conn->filename, O_CREAT | O_WRONLY | O_BINARY,
								     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (conn->fd < 0)
	{
		crWarning( "Couldn't open %s for writing!", conn->filename );
		return 0;
	}
	return 1;
}

static void
crFileDoDisconnect( CRConnection *conn )
{
	close( conn->fd );
	conn->type = CR_NO_CONNECTION;
	crMemcpy( cr_file.conns + conn->index, cr_file.conns + conn->index+1, 
		(cr_file.num_conns - conn->index - 1)*sizeof(*(cr_file.conns)) );
	cr_file.num_conns--;
}

void
crFileConnection( CRConnection *conn )
{
	int n_bytes;

	CRASSERT( cr_file.initialized );

	conn->type  = CR_FILE;
	conn->Alloc = crFileAlloc;
	conn->Send  = crFileSend;
	conn->SendExact  = crFileWriteExact;
	conn->Recv  = crFileSingleRecv;
	conn->Free  = crFileFree;
	conn->Accept = crFileAccept;
	conn->Connect = crFileDoConnect;
	conn->Disconnect = crFileDoDisconnect;
	conn->InstantReclaim = crFileInstantReclaim;
	conn->HandleNewMessage = crFileHandleNewMessage;
	conn->index = cr_file.num_conns;
	conn->sizeof_buffer_header = sizeof( CRFileBuffer );
	conn->actual_network = 0;

	conn->filename = crStrdup( conn->hostname );

	n_bytes = ( cr_file.num_conns + 1 ) * sizeof(*cr_file.conns);
	crRealloc( (void **) &cr_file.conns, n_bytes );

	cr_file.conns[cr_file.num_conns++] = conn;
}

CRConnection**
crFileDump( int* num )
{
	*num = cr_file.num_conns;

	return cr_file.conns;
}

