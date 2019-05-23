/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_idpool.h"


#define CR_MAXUINT             ((GLuint) 0xFFFFFFFF)

typedef struct FreeElemRec {
	GLuint min;
	GLuint max;
	struct FreeElemRec *next;
	struct FreeElemRec *prev;
} FreeElem;

struct CRIdPoolRec {
	FreeElem *freeList;
};



CRIdPool *crAllocIdPool( void )
{
	CRIdPool *pool = (CRIdPool *) crCalloc(sizeof(CRIdPool));
	pool->freeList = (FreeElem *) crCalloc(sizeof(FreeElem));
	pool->freeList->min = 1;
	pool->freeList->max = CR_MAXUINT;
	pool->freeList->next = NULL;
	pool->freeList->prev = NULL;
	return pool;
}

void crFreeIdPool( CRIdPool *pool )
{
	FreeElem *i, *next;
	for (i = pool->freeList; i; i = next)
	{
		next = i->next;
		crFree(i);
	}
	crFree(pool);
}

/*
 * Allocate a block of <count> IDs.  Return index of first one.
 * Return 0 if we fail.
 */
GLuint crIdPoolAllocBlock( CRIdPool *pool, GLuint count )
{
	FreeElem *f;
	GLuint ret;

	CRASSERT(count > 0);

	f = pool->freeList;
	while (f)
	{
		if (f->max - f->min + 1 >= (GLuint) count)
		{
			/* found a sufficiently large enough block */
			ret = f->min;
			f->min += count;

			if (f->min == f->max)
			{
				/* remove this block from linked list */
				if (f == pool->freeList) 
				{
					/* remove from head */
					pool->freeList = pool->freeList->next;
					pool->freeList->prev = NULL;
				}
				else 
				{
					/* remove from elsewhere */
					f->prev->next = f->next;
					f->next->prev = f->prev;
				}
				crFree(f);
			}

#ifdef DEBUG
			/* make sure the IDs really are allocated */
			{
				GLuint i;
				for (i = 0; i < count; i++)
				{
					CRASSERT(crIdPoolIsIdUsed(pool, ret + i));
				}
			}
#endif

			return ret;
		}
		else {
			f = f->next;
		}
	}

	/* failed to find free block */
	return 0;
}


/*
 * Free a block of <count> IDs starting at <first>.
 */
void crIdPoolFreeBlock( CRIdPool *pool, GLuint first, GLuint count )
{
	FreeElem *i;
	FreeElem *newelem;

	/*********************************/
	/* Add the name to the freeList  */
	/* Find the bracketing sequences */

	for (i = pool->freeList; i && i->next && i->next->min < first; i = i->next)
	{
		/* EMPTY BODY */
	}

	/* j will always be valid */
	if (!i)
		return;
	if (!i->next && i->max == first)
		return;

	/* Case:  j:(~,first-1) */
	if (i->max + 1 == first) 
	{
		i->max += count;
		if (i->next && i->max+1 >= i->next->min) 
		{
			/* Collapse */
			i->next->min = i->min;
			i->next->prev = i->prev;
			if (i->prev)
			{
				i->prev->next = i->next;
			}
			if (i == pool->freeList)
			{
				pool->freeList = i->next;
			}
			crFree(i);
		}
		return;
	}

	/* Case: j->next: (first+1, ~) */
	if (i->next && i->next->min - count == first) 
	{
		i->next->min -= count;
		if (i->max + 1 >= i->next->min) 
		{
			/* Collapse */
			i->next->min = i->min;
			i->next->prev = i->prev;
			if (i->prev)
			{
				i->prev->next = i->next;
			}
			if (i == pool->freeList) 
			{
				pool->freeList = i->next;
			}
			crFree(i);
		}
		return;
	}

	/* Case: j: (first+1, ~) j->next: null */
	if (!i->next && i->min - count == first) 
	{
		i->min -= count;
		return;
	}

	/* allocate a new FreeElem node */
	newelem = (FreeElem *) crCalloc(sizeof(FreeElem));
	newelem->min = first;
	newelem->max = first + count - 1;

	/* Case: j: (~,first-(2+))  j->next: (first+(2+), ~) or null */
	if (first > i->max) 
	{
		newelem->prev = i;
		newelem->next = i->next;
		if (i->next)
		{
			i->next->prev = newelem;
		}
		i->next = newelem;
		return;
	}

	/* Case: j: (first+(2+), ~) */
	/* Can only happen if j = t->freeList! */
	if (i == pool->freeList && i->min > first) 
	{
		newelem->next = i;
		newelem->prev = i->prev;
		i->prev = newelem;
		pool->freeList = newelem;
		return;
	}
}



/*
 * Mark the given Id as being allocated.
 */
void crIdPoolAllocId( CRIdPool *pool, GLuint id )
{
	FreeElem *f;

	CRASSERT(id > 0);

	f = pool->freeList;
	while (f)
	{
		if (id >= f->min && id <= f->max)
		{
			/* found the block */
			if (id == f->min)
			{
				f->min++;
			}
			else if (id == f->max)
			{
				f->max--;
			}
			else
			{
				/* somewhere in the middle - split the block */
				FreeElem *newelem = (FreeElem *) crCalloc(sizeof(FreeElem));
				newelem->min = id + 1;
				newelem->max = f->max;
				f->max = id - 1;
				newelem->next = f->next;
				if (f->next)
				   f->next->prev = newelem;
				newelem->prev = f;
				f->next = newelem;
			}
			return;
		}
		f = f->next;
	}

	/* if we get here, the ID was already allocated - that's OK */
}


/*
 * Determine if the given id is free.  Return GL_TRUE if so.
 */
GLboolean crIdPoolIsIdFree( const CRIdPool *pool, GLuint id )
{
	FreeElem *i;

	/* First find which region it fits in */
	for (i = pool->freeList; i && !(i->min <= id && id <= i->max); i=i->next)
	{
		/* EMPTY BODY */
	}

	if (i)
		return GL_TRUE;
	else
		return GL_FALSE;
}


/*
 * Determine if the given id is used.  Return GL_TRUE if so.
 */
GLboolean crIdPoolIsIdUsed( const CRIdPool *pool, GLuint id )
{
	return (GLboolean) !crIdPoolIsIdFree( pool, id);
}
