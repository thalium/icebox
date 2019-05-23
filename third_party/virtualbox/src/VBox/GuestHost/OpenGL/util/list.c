#include "cr_list.h"
#include "cr_error.h"
#include "cr_mem.h"

#ifndef CR_TESTING_LIST			/* vbox */
# define CR_TESTING_LIST 0      	/* vbox */
#endif  				/* vbox */
#if CR_TESTING_LIST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

struct CRListIterator {
	void *element;
	CRListIterator *prev;
	CRListIterator *next;
};

struct CRList {
	CRListIterator *head;
	CRListIterator *tail;
	unsigned size;
};

CRList *crAllocList( void )
{
	CRList *l = crAlloc( sizeof( CRList ) );
	CRASSERT( l );

	l->head = crAlloc( sizeof( CRListIterator ) );
	CRASSERT( l->head );

	l->tail = crAlloc( sizeof( CRListIterator ) );
	CRASSERT( l->tail );

	l->head->prev = NULL;
	l->head->next = l->tail;

	l->tail->prev = l->head;
	l->tail->next = NULL;

	l->size = 0;

	return l;
}

void crFreeList( CRList *l )
{
	CRListIterator *t1;

	CRASSERT( l != NULL );
	t1 = l->head;
	while ( t1 != NULL )
	{
		CRListIterator *t2 = t1;
		t1 = t1->next;
		t2->prev = NULL;
		t2->next = NULL;
		t2->element = NULL;
		crFree( t2 );
	}
	l->size = 0;
	crFree( l );
}

unsigned crListSize( const CRList *l )
{
	return l->size;
}

int crListIsEmpty( const CRList *l )
{
	CRASSERT( l != NULL );
	return l->size == 0;
}

void crListInsert( CRList *l, CRListIterator *iter, void *elem )
{
	CRListIterator *p;

	CRASSERT( l != NULL );
	CRASSERT( iter != NULL );
	CRASSERT( iter != l->head );

	p = crAlloc( sizeof( CRListIterator ) );
	CRASSERT( p != NULL );
	p->prev = iter->prev;
	p->next = iter;
	p->prev->next = p;
	iter->prev = p;

	p->element = elem;
	l->size++;
}

void crListErase( CRList *l, CRListIterator *iter )
{
	CRASSERT( l != NULL );
	CRASSERT( iter != NULL );
	CRASSERT( iter != l->head );
	CRASSERT( iter != l->tail );
	CRASSERT( l->size > 0 );

	iter->next->prev = iter->prev;
	iter->prev->next = iter->next;

	iter->prev = NULL;
	iter->next = NULL;
	iter->element = NULL;
	crFree( iter );

	l->size--;
}

void crListClear( CRList *l )
{
	CRASSERT( l != NULL );
	while ( !crListIsEmpty( l ) )
	{
		crListPopFront( l );
	}
}

void crListPushBack( CRList *l, void *elem )
{
	CRASSERT( l != NULL );
	crListInsert( l, l->tail, elem );
}

void crListPushFront( CRList *l, void *elem )
{
	CRASSERT( l != NULL );
	crListInsert( l, l->head->next, elem );
}

void crListPopBack( CRList *l )
{
	CRASSERT( l != NULL );
	CRASSERT( l->size > 0 );
	crListErase( l, l->tail->prev );
}

void crListPopFront( CRList *l )
{
	CRASSERT( l != NULL );
	CRASSERT( l->size > 0 );
	crListErase( l, l->head->next );
}

void *crListFront( CRList *l )
{
	CRASSERT( l != NULL );
	CRASSERT( l->size > 0 );
	CRASSERT( l->head != NULL );
	CRASSERT( l->head->next != NULL );
	return l->head->next->element;
}

void *crListBack( CRList *l )
{
	CRASSERT( l != NULL );
	CRASSERT( l->size > 0 );
	CRASSERT( l->tail != NULL );
	CRASSERT( l->tail->prev != NULL );
	return l->tail->prev->element;
}

CRListIterator *crListBegin( CRList *l )
{
	CRASSERT( l != NULL );
	CRASSERT( l->head != NULL );
	CRASSERT( l->head->next != NULL );
	return l->head->next;
}

CRListIterator *crListEnd( CRList *l )
{
	CRASSERT( l != NULL );
	CRASSERT( l->tail != NULL );
	return l->tail;
}

CRListIterator *crListNext( CRListIterator *iter )
{
	CRASSERT( iter != NULL );
	CRASSERT( iter->next != NULL );
	return iter->next;
}

CRListIterator *crListPrev( CRListIterator *iter )
{
	CRASSERT( iter != NULL );
	CRASSERT( iter->prev != NULL );
	return iter->prev;
}

void *crListElement( CRListIterator *iter )
{
	CRASSERT( iter != NULL );
	return iter->element;
}

CRListIterator *crListFind( CRList *l, void *element, CRListCompareFunc compare )
{
	CRListIterator *iter;

	CRASSERT( l != NULL );
	CRASSERT( compare );

	for ( iter = crListBegin( l ); iter != crListEnd( l ); iter = crListNext( iter ) )
	{
		if ( compare( element, iter->element ) == 0 )
		{
			return iter;
		}
	}
	return NULL;
}

void crListApply( CRList *l, CRListApplyFunc apply, void *arg )
{
	CRListIterator *iter;

	CRASSERT( l != NULL );
	for ( iter = crListBegin( l ); iter != crListEnd( l ); iter = crListNext( iter ) )
	{
		apply( iter->element, arg );
	}
}

#if CR_TESTING_LIST

static void printElement( void *elem, void *arg )
{
	char *s = elem;
	FILE *fp = arg;

	CRASSERT( s != NULL );
	CRASSERT( fp != NULL );
	fprintf( fp, "%s ", s );
}

static void printList( CRList *l )
{
	CRASSERT( l != NULL );
	crListApply( l, printElement, stderr );
	fprintf( stderr, "\n" );
}

static int elementCompare( void *a, void *b )
{
	return strcmp( a, b );
}

int main( void )
{
	char *names[] = { "a", "b", "c", "d", "e", "f" };
	CRList *l;
	CRListIterator *iter;
	int i, n;

	n = sizeof( names ) / sizeof( names[0] );
	fprintf( stderr, "n=%d\n", n );

	l = crAllocList(  );
	for ( i = 0; i < n; ++i )
	{
		crListPushBack( l, names[i] );
	}
	printList( l );

	crListPushFront( l, "x" );
	printList( l );

	crListPushBack( l, "y" );
	printList( l );

	crListPopFront( l );
	printList( l );

	crListPopBack( l );
	printList( l );

	iter = crListFind( l, "c", elementCompare );
	CRASSERT( iter != NULL );
	crListInsert( l, iter, "z" );
	printList( l );

	iter = crListFind( l, "d", elementCompare );
	CRASSERT( iter != NULL );
	crListErase( l, iter );
	printList( l );

	crListClear( l );
	printList( l );
	fprintf( stderr, "size: %u\n", crListSize( l ) );
	fprintf( stderr, "is empty: %d\n", crListIsEmpty( l ) );

	crListPushBack( l, "w" );
	crListPushBack( l, "t" );
	crListPushBack( l, "c" );
	printList( l );

	fprintf( stderr, "front: %s\n", ( char * ) crListFront( l ) );
	fprintf( stderr, "back: %s\n", ( char * ) crListBack( l ) );
	fprintf( stderr, "size: %u\n", crListSize( l ) );
	fprintf( stderr, "is empty: %d\n", crListIsEmpty( l ) );

	crFreeList( l );
	return 0;
}

#endif /* CR_TESTING_LIST */
