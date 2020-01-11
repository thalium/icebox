/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_threads.h"
#include "cr_hash.h"
#include "cr_mem.h"
#include "cr_error.h"

#include <iprt/list.h>

#define CR_MAXUINT             ((GLuint) 0xFFFFFFFF)
#define CR_HASH_ID_MIN ((GLuint)1)
#define CR_HASH_ID_MAX CR_MAXUINT

#define CR_NUM_BUCKETS 1047

typedef struct FreeElemRec {
    RTLISTNODE Node;
    GLuint min;
    GLuint max;
} FreeElem;

struct CRHashIdPool {
    RTLISTNODE freeList;
    GLuint min;
    GLuint max;
};

typedef struct CRHashNode {
        unsigned long key;
        void *data;
        struct CRHashNode *next;
} CRHashNode;

struct CRHashTable {
    unsigned int num_elements;
    CRHashNode *buckets[CR_NUM_BUCKETS];
    CRHashIdPool   *idPool;
    CRmutex mutex;
};


CRHashIdPool *crAllocHashIdPoolEx( GLuint min, GLuint max )
{
    CRHashIdPool *pool;
    FreeElem *elem;
    if (min < CR_HASH_ID_MIN || max > CR_HASH_ID_MAX || min >= max)
    {
        crWarning("invalid min man vals");
        return NULL;
    }
    pool = (CRHashIdPool *) crCalloc(sizeof(CRHashIdPool));
    elem = (FreeElem *) crCalloc(sizeof(FreeElem));
    RTListInit(&pool->freeList);
    elem->min = min;
    elem->max = max;
    RTListAppend(&pool->freeList, &elem->Node);
    pool->min = min;
    pool->max = max;
    return pool;
}

CRHashIdPool *crAllocHashIdPool( void )
{
    return crAllocHashIdPoolEx( CR_HASH_ID_MIN, CR_HASH_ID_MAX );
}

void crFreeHashIdPool( CRHashIdPool *pool )
{
    FreeElem *i, *next;
    RTListForEachSafe(&pool->freeList, i, next, FreeElem, Node)
    {
        crFree(i);
    }

    crFree(pool);
}

#ifdef DEBUG_misha
static void crHashIdPoolDbgCheckConsistency(CRHashIdPool *pool)
{
    FreeElem *i;
    GLuint min = 0;

    /* null is a special case, it is always treated as allocated */
    Assert(!crHashIdPoolIsIdFree(pool, 0));

    /* first ensure entries have correct values */
    RTListForEach(&pool->freeList, i, FreeElem, Node)
    {
        Assert(i->min >= pool->min);
        Assert(i->max <= pool->max);
        Assert(i->min < i->max);
    }

    /* now ensure entries do not intersect */
    /* and that they are sorted */
    RTListForEach(&pool->freeList, i, FreeElem, Node)
    {
        Assert(min < i->min);
        min = i->max;
    }
}

static void crHashIdPoolDbgCheckUsed( const CRHashIdPool *pool, GLuint start, GLuint count, GLboolean fUsed )
{
    GLuint i;
    CRASSERT(count);
    CRASSERT(start >= pool->min);
    CRASSERT(start + count <= pool->max);
    CRASSERT(start + count >  start);
    for (i = 0; i < count; ++i)
    {
        Assert(!fUsed == !!crHashIdPoolIsIdFree( pool, start + i ));
    }
}

# define CR_HASH_IDPOOL_DBG_CHECK_USED(_p, _start, _count, _used) do { \
        crHashIdPoolDbgCheckConsistency((_p)); \
        crHashIdPoolDbgCheckUsed( (_p), (_start), (_count), (_used) ); \
    } while (0)

# define CR_HASH_IDPOOL_DBG_CHECK_CONSISTENCY(_p) do { crHashIdPoolDbgCheckConsistency((_p)); } while (0)
#else
# define CR_HASH_IDPOOL_DBG_CHECK_USED(_p, _start, _count, _used) do { } while (0)
# define CR_HASH_IDPOOL_DBG_CHECK_CONSISTENCY(_p) do { } while (0)
#endif

/*
 * Allocate a block of <count> IDs.  Return index of first one.
 * Return 0 if we fail.
 */
GLuint crHashIdPoolAllocBlock( CRHashIdPool *pool, GLuint count )
{
    FreeElem *f, *next;
    GLuint ret;

    CRASSERT(count > 0);
    RTListForEachSafe(&pool->freeList, f, next, FreeElem, Node)
    {
        Assert(f->max > f->min);
        if (f->max - f->min >= (GLuint) count)
        {
            /* found a sufficiently large enough block */
            ret = f->min;
            f->min += count;
            if (f->min == f->max)
            {
                RTListNodeRemove(&f->Node);
                crFree(f);
            }

            CR_HASH_IDPOOL_DBG_CHECK_USED(pool, ret, count, GL_TRUE);
            return ret;
        }
    }

    /* failed to find free block */
    crWarning("crHashIdPoolAllocBlock failed");
    CR_HASH_IDPOOL_DBG_CHECK_CONSISTENCY(pool);
    return 0;
}


/*
 * Free a block of <count> IDs starting at <first>.
 */
void crHashIdPoolFreeBlock( CRHashIdPool *pool, GLuint first, GLuint count )
{
    FreeElem *f;
    GLuint last;
    GLuint newMax;
    FreeElem *cur, *curNext;

    /* null is a special case, it is always treated as allocated */
    if (!first)
    {
        Assert(!crHashIdPoolIsIdFree(pool, 0));
        ++first;
        --count;
        if (!count)
            return;
    }

    last = first + count;
    CRASSERT(count > 0);
    CRASSERT(last > first);
    CRASSERT(first >= pool->min);
    CRASSERT(last <= pool->max);

    /* the id list is sorted, first find a place to insert */
    RTListForEach(&pool->freeList, f, FreeElem, Node)
    {
        Assert(f->max > f->min);

        if (f->max < first)
            continue;

        if (f->min > last)
        {
            /* we are here because first is > than prevEntry->max
             * otherwise the previous loop iterations should handle that */
            FreeElem *elem = (FreeElem *) crCalloc(sizeof(FreeElem));
            elem->min = first;
            elem->max = last;
            RTListNodeInsertBefore(&f->Node, &elem->Node);
            CR_HASH_IDPOOL_DBG_CHECK_USED(pool, first, count, GL_FALSE);
            return;
        }

        /* now we have f->min <= last and f->max >= first,
         * so we have either intersection */

        if (f->min > first)
            f->min = first; /* first is guaranteed not to touch any prev regions */

        newMax = last;

        if (f->max >= last)
        {
            CR_HASH_IDPOOL_DBG_CHECK_USED(pool, first, count, GL_FALSE);
            return;
        }

        for (cur = RTListNodeGetNext(&f->Node, FreeElem, Node),
                curNext = RT_FROM_MEMBER(cur->Node.pNext, FreeElem, Node);
             !RTListNodeIsDummy(&pool->freeList, cur, FreeElem, Node);
             cur = curNext,
                     curNext = RT_FROM_MEMBER((cur)->Node.pNext, FreeElem, Node) )
        {
            if (cur->min > last)
                break;

            newMax = cur->max;
            RTListNodeRemove(&cur->Node);
            crFree(cur);

            if (newMax >= last)
                break;
        }

        f->max = newMax;
        CR_HASH_IDPOOL_DBG_CHECK_USED(pool, first, count, GL_FALSE);
        return;
    }

    /* we are here because either the list is empty or because all list rande elements have smaller values */
    f = (FreeElem *) crCalloc(sizeof(FreeElem));
    f->min = first;
    f->max = last;
    RTListAppend(&pool->freeList, &f->Node);
    CR_HASH_IDPOOL_DBG_CHECK_USED(pool, first, count, GL_FALSE);
}



/*
 * Mark the given Id as being allocated.
 */
GLboolean crHashIdPoolAllocId( CRHashIdPool *pool, GLuint id )
{
    FreeElem *f, *next;

    if (!id)
    {
        /* null is a special case, it is always treated as allocated */
        Assert(!crHashIdPoolIsIdFree(pool, 0));
        return GL_FALSE;
    }

/*    Assert(id != 2);*/

    RTListForEachSafe(&pool->freeList, f, next, FreeElem, Node)
    {
        if (f->max <= id)
            continue;
        if (f->min > id)
        {
            CR_HASH_IDPOOL_DBG_CHECK_USED(pool, id, 1, GL_TRUE);
            return GL_FALSE;
        }

        /* f->min <= id && f->max > id */
        if (id > f->min)
        {
            if (id + 1 < f->max)
            {
                FreeElem *elem = (FreeElem *) crCalloc(sizeof(FreeElem));
                elem->min = id + 1;
                elem->max = f->max;
                RTListNodeInsertAfter(&f->Node, &elem->Node);
            }
            f->max = id;

            CR_HASH_IDPOOL_DBG_CHECK_USED(pool, id, 1, GL_TRUE);
        }
        else
        {
            Assert(id == f->min);
            if (id + 1 < f->max)
            {
                f->min = id + 1;
                CR_HASH_IDPOOL_DBG_CHECK_USED(pool, id, 1, GL_TRUE);
            }
            else
            {
                RTListNodeRemove(&f->Node);
                crFree(f);
                CR_HASH_IDPOOL_DBG_CHECK_USED(pool, id, 1, GL_TRUE);
            }
        }

        CR_HASH_IDPOOL_DBG_CHECK_USED(pool, id, 1, GL_TRUE);
        return GL_TRUE;
    }

    /* if we get here, the ID was already allocated - that's OK */
    CR_HASH_IDPOOL_DBG_CHECK_USED(pool, id, 1, GL_TRUE);
    return GL_FALSE;
}


/*
 * Determine if the given id is free.  Return GL_TRUE if so.
 */
GLboolean crHashIdPoolIsIdFree( const CRHashIdPool *pool, GLuint id )
{
    FreeElem *f;
    CRASSERT(id <= pool->max);

    RTListForEach(&pool->freeList, f, FreeElem, Node)
    {
        if (f->max <= id)
            continue;
        if (f->min > id)
            return GL_FALSE;
        return GL_TRUE;
    }
    return GL_FALSE;
}

void crHashIdWalkKeys( CRHashIdPool *pool, CRHashIdWalkKeys walkFunc , void *data)
{
    FreeElem *prev = NULL, *f;

    RTListForEach(&pool->freeList, f, FreeElem, Node)
    {
        if (prev)
        {
            Assert(prev->max < f->min);
            walkFunc(prev->max+1, f->min - prev->max, data);
        }
        else if (f->min > pool->min)
        {
            walkFunc(pool->min, f->min - pool->min, data);
        }

        prev = f;
    }

    Assert(prev->max <= pool->max);

    if (prev->max < pool->max)
    {
        walkFunc(prev->max+1, pool->max - prev->max, data);
    }
}

CRHashTable *crAllocHashtableEx( GLuint min, GLuint max )
{
    int i;
    CRHashTable *hash = (CRHashTable *) crCalloc( sizeof( CRHashTable )) ;
    hash->num_elements = 0;
    for (i = 0 ; i < CR_NUM_BUCKETS ; i++)
    {
        hash->buckets[i] = NULL;
    }
    hash->idPool = crAllocHashIdPoolEx( min, max );
    crInitMutex(&hash->mutex);
    return hash;
}

CRHashTable *crAllocHashtable( void )
{
    return crAllocHashtableEx(CR_HASH_ID_MIN, CR_HASH_ID_MAX);
}

void crFreeHashtable( CRHashTable *hash, CRHashtableCallback deleteFunc )
{
    int i;
    CRHashNode *entry, *next;

    if ( !hash) return;

    crLockMutex(&hash->mutex);
    for ( i = 0; i < CR_NUM_BUCKETS; i++ )
    {
        entry = hash->buckets[i];
        while (entry)
        {
            next = entry->next;
            /* Clear the key in case crHashtableDelete() is called
             * from this callback.
             */
            entry->key = 0;
            if (deleteFunc && entry->data)
            {
                (*deleteFunc)(entry->data);
            }
            crFree(entry);
            entry = next;

        }
    }
    crFreeHashIdPool( hash->idPool );
    crUnlockMutex(&hash->mutex);
    crFreeMutex(&hash->mutex);
    crFree( hash );
}

void crFreeHashtableEx(CRHashTable *hash, CRHashtableCallbackEx deleteFunc, void *pData)
{
    int i;
    CRHashNode *entry, *next;

    if (!hash) return;

    crLockMutex(&hash->mutex);
    for (i = 0; i < CR_NUM_BUCKETS; i++)
    {
        entry = hash->buckets[i];
        while (entry)
        {
            next = entry->next;
            /* Clear the key in case crHashtableDelete() is called
             * from this callback.
             */
            entry->key = 0;
            if (deleteFunc && entry->data)
            {
                (*deleteFunc)(entry->data, pData);
            }
            crFree(entry);
            entry = next;

        }
    }
    crFreeHashIdPool(hash->idPool);

    crUnlockMutex(&hash->mutex);
    crFreeMutex(&hash->mutex);
    crFree(hash);
}


void crHashtableLock(CRHashTable *h)
{
    crLockMutex(&h->mutex);
}

void crHashtableUnlock(CRHashTable *h)
{
    crUnlockMutex(&h->mutex);
}

void crHashtableWalkUnlocked( CRHashTable *hash, CRHashtableWalkCallback walkFunc , void *dataPtr2)
{
    int i;
    CRHashNode *entry, *next;

    for (i = 0; i < CR_NUM_BUCKETS; i++)
    {
        entry = hash->buckets[i];
        while (entry) 
        {
            /* save next ptr here, in case walkFunc deletes this entry */
            next = entry->next;
            if (entry->data && walkFunc) {
                (*walkFunc)( entry->key, entry->data, dataPtr2 );
            }
            entry = next;
        }
    }
}

void crHashtableWalk( CRHashTable *hash, CRHashtableWalkCallback walkFunc , void *dataPtr2)
{
    if (!hash)
        return;

    crLockMutex(&hash->mutex);
    crHashtableWalkUnlocked(hash, walkFunc , dataPtr2);
    crUnlockMutex(&hash->mutex);
}

static unsigned int crHash( unsigned long key )
{
    return key % CR_NUM_BUCKETS;
}

void crHashtableAdd( CRHashTable *h, unsigned long key, void *data )
{
    unsigned int index = crHash(key);
    CRHashNode *node = (CRHashNode *) crCalloc( sizeof( CRHashNode ) );

    crLockMutex(&h->mutex);
    node->key = key;
    node->data = data;
    node->next = h->buckets[index];
    h->buckets[index] = node;
    h->num_elements++;
    crHashIdPoolAllocId (h->idPool, key);
    crUnlockMutex(&h->mutex);
}

GLboolean crHashtableAllocRegisterKey( CRHashTable *h,  GLuint key)
{
    GLboolean fAllocated;

    crLockMutex(&h->mutex);
    fAllocated = crHashIdPoolAllocId (h->idPool, key);
    crUnlockMutex(&h->mutex);
    return fAllocated;
}

void crHashtableWalkKeys( CRHashTable *h, CRHashIdWalkKeys walkFunc , void *data)
{
    crLockMutex(&h->mutex);
    crHashIdWalkKeys(h->idPool, walkFunc , data);
    crUnlockMutex(&h->mutex);
}

GLuint crHashtableAllocKeys( CRHashTable *h,  GLsizei range)
{
    GLuint res;

    crLockMutex(&h->mutex);
    res = crHashIdPoolAllocBlock (h->idPool, range);
#ifdef DEBUG_misha
    {
        int i;
        Assert(res);
        for (i = 0; i < range; ++i)
        {
            void *search = crHashtableSearch( h, res+i );
            Assert(!search);
        }
    }
#endif
    crUnlockMutex(&h->mutex);
    return res;
}

void crHashtableDelete( CRHashTable *h, unsigned long key, CRHashtableCallback deleteFunc )
{
    unsigned int index = crHash( key );
    CRHashNode *temp, *beftemp = NULL;

    crLockMutex(&h->mutex);
    for ( temp = h->buckets[index]; temp; temp = temp->next )
    {
        if ( temp->key == key )
            break;
        beftemp = temp;
    }
    if ( temp ) 
    {
        if ( beftemp )
            beftemp->next = temp->next;
        else
            h->buckets[index] = temp->next;
        h->num_elements--;
        if (temp->data && deleteFunc) {
            (*deleteFunc)( temp->data );
        }

        crFree( temp );
    }
    
    crHashIdPoolFreeBlock( h->idPool, key, 1 );
    crUnlockMutex(&h->mutex);
}

void crHashtableDeleteEx(CRHashTable *h, unsigned long key, CRHashtableCallbackEx deleteFunc, void *pData)
{
    unsigned int index = crHash( key );
    CRHashNode *temp, *beftemp = NULL;

    crLockMutex(&h->mutex);
    for (temp = h->buckets[index]; temp; temp = temp->next)
    {
        if (temp->key == key)
            break;
        beftemp = temp;
    }
    if (temp)
    {
        if (beftemp)
            beftemp->next = temp->next;
        else
            h->buckets[index] = temp->next;
        h->num_elements--;
        if (temp->data && deleteFunc) {
            (*deleteFunc)(temp->data, pData);
        }

        crFree(temp);
    }

    crHashIdPoolFreeBlock(h->idPool, key, 1);
    crUnlockMutex(&h->mutex);
}


void crHashtableDeleteBlock( CRHashTable *h, unsigned long key, GLsizei range, CRHashtableCallback deleteFunc )
{
    /* XXX optimize someday */
    GLuint i;
    for (i = 0; i < (GLuint)range; i++) {
        crHashtableDelete( h, key, deleteFunc );
    }
}

void *crHashtableSearch( const CRHashTable *h, unsigned long key )
{
    unsigned int index = crHash( key );
    CRHashNode *temp;

    crLockMutex((CRmutex *)&h->mutex);
    for ( temp = h->buckets[index]; temp; temp = temp->next )
    {
        if ( temp->key == key )
            break;
    }
    crUnlockMutex((CRmutex *)&h->mutex);
    if ( !temp )
    {
        return NULL;
    }
    return temp->data;
}

void crHashtableReplace( CRHashTable *h, unsigned long key, void *data,
                         CRHashtableCallback deleteFunc)
{
    unsigned int index = crHash( key );
    CRHashNode *temp;

    crLockMutex(&h->mutex);
    for ( temp = h->buckets[index]; temp; temp = temp->next )
    {
        if ( temp->key == key )
            break;
    }
    crUnlockMutex(&h->mutex);
    if ( !temp )
    {
        crHashtableAdd( h, key, data );
        return;
    }
    crLockMutex(&h->mutex);
    if ( temp->data && deleteFunc )
    {
        (*deleteFunc)( temp->data );
    }
    temp->data = data;
    crUnlockMutex(&h->mutex);
}

unsigned int crHashtableNumElements( const CRHashTable *h) 
{
    if (h)
        return h->num_elements;
    else
        return 0;
}

/*
 * Determine if the given key is used.  Return GL_TRUE if so.
 */
GLboolean crHashtableIsKeyUsed( const CRHashTable *h, GLuint id )
{
    return (GLboolean) !crHashIdPoolIsIdFree( h->idPool, id);
}

GLboolean crHashtableGetDataKey(CRHashTable *pHash, void *pData, unsigned long *pKey)
{
    int i;
    CRHashNode *entry;
    GLboolean rc = GL_FALSE;

    if (!pHash)
        return rc;

    crLockMutex(&pHash->mutex);
    for (i = 0; i<CR_NUM_BUCKETS && !rc; i++)
    {
        entry = pHash->buckets[i];
        while (entry) 
        {
            if (entry->data == pData) {
                if (pKey)
                    *pKey = entry->key;
                rc = GL_TRUE;
                break;              
            }
            entry = entry->next;
        }
    }

    crUnlockMutex(&pHash->mutex);
    return rc;
}
