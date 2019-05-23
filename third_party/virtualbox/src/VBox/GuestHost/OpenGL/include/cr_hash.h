/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_HASH_H
#define CR_HASH_H

#include "chromium.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CRHashIdPool CRHashIdPool;
typedef struct CRHashTable CRHashTable;

/* Callback function used for freeing/deleting table entries */
typedef void (*CRHashtableCallback)(void *data);
typedef void (*CRHashtableCallbackEx)(void *data1, void *data2);
/* Callback function used for walking through table entries */
typedef void (*CRHashtableWalkCallback)(unsigned long key, void *data1, void *data2);
/* Callback function used for walking through allocated keys */
typedef void (*CRHashIdWalkKeys)(unsigned long firstKey, unsigned long count, void *data);

DECLEXPORT(CRHashIdPool *) crAllocHashIdPool( void );
DECLEXPORT(CRHashIdPool *) crAllocHashIdPoolEx( GLuint min, GLuint max );
DECLEXPORT(void) crFreeHashIdPool( CRHashIdPool *pool );
DECLEXPORT(GLboolean) crHashIdPoolIsIdFree( const CRHashIdPool *pool, GLuint id );
DECLEXPORT(GLuint) crHashIdPoolAllocBlock( CRHashIdPool *pool, GLuint count );
DECLEXPORT(void) crHashIdPoolFreeBlock( CRHashIdPool *pool, GLuint first, GLuint count );
/* @return GL_TRUE if the id is allocated, and GL_FALSE if the id was already allocated */
DECLEXPORT(GLboolean) crHashIdPoolAllocId( CRHashIdPool *pool, GLuint id );
DECLEXPORT(void) crHashIdWalkKeys( CRHashIdPool *pool, CRHashIdWalkKeys walkFunc , void *data);

DECLEXPORT(CRHashTable *) crAllocHashtable( void );
DECLEXPORT(CRHashTable *) crAllocHashtableEx( GLuint min, GLuint max );
DECLEXPORT(void) crFreeHashtable( CRHashTable *hash, CRHashtableCallback deleteCallback );
DECLEXPORT(void) crFreeHashtableEx( CRHashTable *hash, CRHashtableCallbackEx deleteCallback, void *pData );
DECLEXPORT(void) crHashtableAdd( CRHashTable *h, unsigned long key, void *data );
/* to ensure hash table pool id consistency, there is no crHashTableFreeKeys/UnregisterKey,
 * one should call crHashtableDelete to free unneeded keys, 
 * which will also ensure there is no entry with the specified key left in the table */
DECLEXPORT(GLuint) crHashtableAllocKeys( CRHashTable *h, GLsizei range );
/* @return GL_TRUE if the id is allocated, and GL_FALSE if the id was already allocated */
DECLEXPORT(GLboolean) crHashtableAllocRegisterKey( CRHashTable *h,  GLuint key);
DECLEXPORT(void) crHashtableDelete( CRHashTable *h, unsigned long key, CRHashtableCallback deleteCallback );
DECLEXPORT(void) crHashtableDeleteEx( CRHashTable *h, unsigned long key, CRHashtableCallbackEx deleteCallback, void *pData );
DECLEXPORT(void) crHashtableDeleteBlock( CRHashTable *h, unsigned long key, GLsizei range, CRHashtableCallback deleteFunc );
DECLEXPORT(void *) crHashtableSearch( const CRHashTable *h, unsigned long key );
DECLEXPORT(void) crHashtableReplace( CRHashTable *h, unsigned long key, void *data, CRHashtableCallback deleteFunc);
DECLEXPORT(unsigned int) crHashtableNumElements( const CRHashTable *h) ;
DECLEXPORT(GLboolean) crHashtableIsKeyUsed( const CRHashTable *h, GLuint id );
DECLEXPORT(void) crHashtableWalk( CRHashTable *hash, CRHashtableWalkCallback walkFunc , void *data);
/* walk the hashtable w/o holding the table lock */
DECLEXPORT(void) crHashtableWalkUnlocked( CRHashTable *hash, CRHashtableWalkCallback walkFunc , void *data);
/*Returns GL_TRUE if given hashtable hold the data, pKey is updated with key value for data in this case*/
DECLEXPORT(GLboolean) crHashtableGetDataKey(CRHashTable *pHash, void *pData, unsigned long *pKey);
DECLEXPORT(void) crHashtableLock(CRHashTable *h);
DECLEXPORT(void) crHashtableUnlock(CRHashTable *h);
DECLEXPORT(void) crHashtableWalkKeys(CRHashTable *hash, CRHashIdWalkKeys walkFunc , void *data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_HASH_H */
