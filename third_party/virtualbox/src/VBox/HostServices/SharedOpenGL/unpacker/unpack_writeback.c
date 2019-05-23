/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include <stdio.h>

CRNetworkPointer *return_ptr = NULL, *writeback_ptr = NULL;

void crUnpackSetReturnPointer( CRNetworkPointer *ret )
{
	return_ptr = ret;
}

void crUnpackSetWritebackPointer( CRNetworkPointer *wri )
{
	writeback_ptr = wri;
}

void crUnpackExtendWriteback(void)
{
	/* This copies the unpack buffer's CRNetworkPointer to writeback_ptr */
	SET_WRITEBACK_PTR( 8 );
	cr_unpackDispatch.Writeback( NULL );
}

#if 0
void crUnpackWriteback(void)
{
	crError( "crUnpackWriteback should never be called" );
}
#endif
