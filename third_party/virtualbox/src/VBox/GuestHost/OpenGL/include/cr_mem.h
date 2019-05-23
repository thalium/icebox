/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_MEM_H
#define CR_MEM_H

#include <iprt/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_MEM 0

void *crAllocDebug( unsigned int nbytes, const char *file, int line );
void *crCallocDebug( unsigned int nbytes, const char *file, int line );
#if DEBUG_MEM
#define crAlloc(N) crAllocDebug(N, __FILE__, __LINE__)
#define crCalloc(N) crCallocDebug(N, __FILE__, __LINE__)
#else
extern DECLEXPORT(void *) crAlloc( unsigned int nbytes );
extern DECLEXPORT(void *) crCalloc( unsigned int nbytes );
#endif
extern DECLEXPORT(void) crRealloc( void **ptr, unsigned int bytes );
extern DECLEXPORT(void) crFree( void *ptr );
extern DECLEXPORT(void) crMemcpy( void *dst, const void *src, unsigned int bytes );
extern DECLEXPORT(void) crMemset( void *ptr, int value, unsigned int bytes );
extern DECLEXPORT(void) crMemZero( void *ptr, unsigned int bytes );
extern DECLEXPORT(int)  crMemcmp( const void *p1, const void *p2, unsigned int bytes );

#ifdef __cplusplus
}
#endif

#endif /* CR_MEM_H */
