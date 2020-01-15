/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_error.h"

#include <iprt/types.h>
#include <iprt/mem.h>


DECLEXPORT(void) *crAlloc( unsigned int nbytes )
{
	void *ret = RTMemAlloc( nbytes );
	if (!ret) {
		crError( "Out of memory trying to allocate %d bytes!", nbytes );
		abort();
	}
	return ret;
}

DECLEXPORT(void) *crCalloc( unsigned int nbytes )
{
	void *ret = RTMemAlloc( nbytes );
	if (!ret) {
		crError( "Out of memory trying to (c)allocate %d bytes!", nbytes );
		abort();
	}
	crMemset( ret, 0, nbytes );
	return ret;
}

DECLEXPORT(void) crRealloc( void **ptr, unsigned int nbytes )
{
	if ( *ptr == NULL )
		*ptr = crAlloc( nbytes );
	else
	{
		*ptr = RTMemRealloc( *ptr, nbytes );
		if (*ptr == NULL)
			crError( "Couldn't realloc %d bytes!", nbytes );
	}
}

DECLEXPORT(void) crFree( void *ptr )
{
	if (ptr)
		RTMemFree(ptr);
}
