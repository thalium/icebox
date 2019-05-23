/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_error.h"

#include <stdlib.h>
#include <memory.h>

#include <iprt/types.h>
#include <iprt/mem.h>

#if DEBUG_MEM
#include <stdio.h>
#define THRESHOLD 100 * 1024

#undef crAlloc
#undef crCalloc

/* Need these stubs because util.def says we're always exporting crAlloc
 * and crCalloc.
 */
extern void *crAlloc( unsigned int bytes );
void *crAlloc( unsigned int bytes )
{
   return NULL;
}

extern void *crCalloc( unsigned int bytes );
void *crCalloc( unsigned int bytes )
{
   return NULL;
}
#endif /* DEBUG_MEM */



#if DEBUG_MEM
static void *_crAlloc( unsigned int nbytes )
#else
DECLEXPORT(void) *crAlloc( unsigned int nbytes )
#endif
{
#ifdef VBOX
	void *ret = RTMemAlloc( nbytes );
#else
	void *ret = malloc( nbytes );
#endif
	if (!ret) {
		crError( "Out of memory trying to allocate %d bytes!", nbytes );
		abort();
	}
	return ret;
}

void *crAllocDebug( unsigned int nbytes, const char *file, int line )
{
#if DEBUG_MEM
	if (nbytes >= THRESHOLD)
		fprintf(stderr, "crAllocDebug(%d bytes) in %s at %d\n", nbytes, file, line);
	return _crAlloc(nbytes);
#else
	RT_NOREF2(file, line);
	return crAlloc(nbytes);
#endif
}

#if DEBUG_MEM
static void *_crCalloc( unsigned int nbytes )
#else
DECLEXPORT(void) *crCalloc( unsigned int nbytes )
#endif
{
#ifdef VBOX
	void *ret = RTMemAlloc( nbytes );
#else
	void *ret = malloc( nbytes );
#endif
	if (!ret) {
		crError( "Out of memory trying to (c)allocate %d bytes!", nbytes );
		abort();
	}
	crMemset( ret, 0, nbytes );
	return ret;
}

void *crCallocDebug( unsigned int nbytes, const char *file, int line )
{
#if DEBUG_MEM
	if (nbytes >= THRESHOLD)
		fprintf(stderr, "crCallocDebug(%d bytes) in %s at %d\n", nbytes, file, line);
	return _crCalloc(nbytes);
#else
	RT_NOREF2(file, line);
	return crCalloc(nbytes);
#endif
}

DECLEXPORT(void) crRealloc( void **ptr, unsigned int nbytes )
{
	if ( *ptr == NULL )
	{
#if DEBUG_MEM
		*ptr = _crAlloc( nbytes );
#else
		*ptr = crAlloc( nbytes );
#endif
	}
	else
	{
#ifdef VBOX
		*ptr = RTMemRealloc( *ptr, nbytes );
#else
		*ptr = realloc( *ptr, nbytes );
#endif
		if (*ptr == NULL)
			crError( "Couldn't realloc %d bytes!", nbytes );
	}
}

DECLEXPORT(void) crFree( void *ptr )
{
#ifdef VBOX
	if (ptr)
		RTMemFree(ptr);
#else
	if (ptr)
		free(ptr);
#endif
}

DECLEXPORT(void) crMemcpy( void *dst, const void *src, unsigned int bytes )
{
	CRASSERT(dst || 0==bytes);
	CRASSERT(src || 0==bytes);
	(void) memcpy( dst, src, bytes );
}

DECLEXPORT(void) crMemset( void *ptr, int value, unsigned int bytes )
{
	CRASSERT(ptr);
	memset( ptr, value, bytes );
}

DECLEXPORT(void) crMemZero( void *ptr, unsigned int bytes )
{
	CRASSERT(ptr);
	memset( ptr, 0, bytes );
}

DECLEXPORT(int) crMemcmp( const void *p1, const void *p2, unsigned int bytes )
{
	CRASSERT(p1);
	CRASSERT(p2);
	return memcmp( p1, p2, bytes );
}

