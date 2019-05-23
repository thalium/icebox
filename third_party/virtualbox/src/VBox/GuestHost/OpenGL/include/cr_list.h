#ifndef CR_LIST_H
#define CR_LIST_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CRList CRList;
typedef struct CRListIterator CRListIterator;
typedef int ( *CRListCompareFunc ) ( const void *element1, const void *element2 );
typedef void ( *CRListApplyFunc ) ( void *element, void *arg );

DECLEXPORT(CRList *) crAllocList( void );
DECLEXPORT(void) crFreeList( CRList *l );

DECLEXPORT(unsigned) crListSize( const CRList *l );
DECLEXPORT(int) crListIsEmpty( const CRList *l );

DECLEXPORT(void) crListInsert( CRList *l, CRListIterator *iter, void *elem );
DECLEXPORT(void) crListErase( CRList *l, CRListIterator *iter );
DECLEXPORT(void) crListClear( CRList *l );

DECLEXPORT(void) crListPushBack( CRList *l, void *elem );
DECLEXPORT(void) crListPushFront( CRList *l, void *elem );

DECLEXPORT(void) crListPopBack( CRList *l );
DECLEXPORT(void) crListPopFront( CRList *l );

DECLEXPORT(void *) crListFront( CRList *l );
DECLEXPORT(void *) crListBack( CRList *l );

DECLEXPORT(CRListIterator *) crListBegin( CRList *l );
DECLEXPORT(CRListIterator *) crListEnd( CRList *l );

DECLEXPORT(CRListIterator *) crListNext( CRListIterator *iter );
DECLEXPORT(CRListIterator *) crListPrev( CRListIterator *iter );
DECLEXPORT(void *) crListElement( CRListIterator *iter );

DECLEXPORT(CRListIterator *) crListFind( CRList *l, void *element, CRListCompareFunc compare );
DECLEXPORT(void) crListApply( CRList *l, CRListApplyFunc apply, void *arg );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_LIST_H */
