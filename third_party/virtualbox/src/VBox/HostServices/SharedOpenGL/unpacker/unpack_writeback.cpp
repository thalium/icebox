/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include <stdio.h>

void crUnpackExtendWriteback(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, CRNetworkPointer);

	/* This copies the unpack buffer's CRNetworkPointer to writeback_ptr */
	SET_WRITEBACK_PTR(pState, 8 );
	pState->pDispatchTbl->Writeback( NULL );
}

#if 0
void crUnpackWriteback(void)
{
	crError( "crUnpackWriteback should never be called" );
}
#endif
