/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_MEM_H
#define CR_MEM_H

#include "cr_error.h"

#include <stdlib.h>
#include <iprt/types.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLINLINE(void) crMemcpy( void *dst, const void *src, unsigned int bytes )
{
	CRASSERT(dst || 0==bytes);
	CRASSERT(src || 0==bytes);
	(void) memcpy( dst, src, bytes );
}

DECLINLINE(void) crMemset( void *ptr, int value, unsigned int bytes )
{
	CRASSERT(ptr);
	memset( ptr, value, bytes );
}

DECLINLINE(void) crMemZero( void *ptr, unsigned int bytes )
{
	CRASSERT(ptr);
	memset( ptr, 0, bytes );
}

DECLINLINE(int) crMemcmp( const void *p1, const void *p2, unsigned int bytes )
{
	CRASSERT(p1);
	CRASSERT(p2);
	return memcmp( p1, p2, bytes );
}

extern DECLEXPORT(void *) crAlloc( unsigned int nbytes );
extern DECLEXPORT(void *) crCalloc( unsigned int nbytes );
extern DECLEXPORT(void) crRealloc( void **ptr, unsigned int bytes );
extern DECLEXPORT(void) crFree( void *ptr );

#ifdef __cplusplus
}
#endif

#endif /* CR_MEM_H */
