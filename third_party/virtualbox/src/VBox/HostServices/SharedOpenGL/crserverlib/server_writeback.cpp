/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"

void SERVER_DISPATCH_APIENTRY crServerDispatchWriteback( GLint *writeback )
{
	(void) writeback;
	crServerWriteback( );
}

void crServerWriteback(void)
{
	CRMessageWriteback *wb = (CRMessageWriteback *) crAlloc( sizeof( *wb ) );
	wb->header.type = CR_MESSAGE_WRITEBACK;
	CRDBGPTR_PRINTWB(cr_server.curClient->conn->u32ClientID, &cr_server.writeback_ptr);
	CRDBGPTR_CHECKNZ(&cr_server.writeback_ptr);
	crMemcpy( &(wb->writeback_ptr), &(cr_server.writeback_ptr), sizeof( wb->writeback_ptr ) );
	crNetSend( cr_server.curClient->conn, NULL, wb, sizeof( *wb ) );
	CRDBGPTR_SETZ(&cr_server.writeback_ptr);
	crFree( wb );
}
