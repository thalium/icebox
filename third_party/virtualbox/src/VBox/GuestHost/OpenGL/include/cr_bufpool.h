/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_BUFPOOL_H
#define CR_BUFPOOL_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CRBufferPool_t CRBufferPool;
typedef void (*CRBufferPoolDeleteCallback)(void *data);

DECLEXPORT(CRBufferPool *) crBufferPoolInit( unsigned int maxBuffers );
DECLEXPORT(void) crBufferPoolFree( CRBufferPool *pool );
DECLEXPORT(void) crBufferPoolCallbackFree(CRBufferPool *pool, CRBufferPoolDeleteCallback pfnDelete);

DECLEXPORT(void)   crBufferPoolPush( CRBufferPool *pool, void *buf, unsigned int bytes );
DECLEXPORT(void *) crBufferPoolPop( CRBufferPool *pool, unsigned int bytes );

DECLEXPORT(int) crBufferPoolGetNumBuffers( CRBufferPool *pool );
DECLEXPORT(int) crBufferPoolGetMaxBuffers( CRBufferPool *pool );

#ifdef __cplusplus
}
#endif

#endif /* CR_BUFPOOL_H */
