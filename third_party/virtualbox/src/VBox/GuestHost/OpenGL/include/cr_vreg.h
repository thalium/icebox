/* $Id: cr_vreg.h $ */
/** @file
 * Visible Regions processing API
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___cr_vreg_h_
#define ___cr_vreg_h_

#include <iprt/list.h>
#include <iprt/types.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/asm.h>

#ifndef IN_RING0
# define VBOXVREGDECL(_type) DECLEXPORT(_type)
#else
/** @todo r=bird: Using RTDECL is just SOO wrong!   */
# define VBOXVREGDECL(_type) RTDECL(_type)
#endif



RT_C_DECLS_BEGIN

typedef struct VBOXVR_LIST
{
    RTLISTANCHOR ListHead;
    uint32_t cEntries;
} VBOXVR_LIST;
typedef VBOXVR_LIST *PVBOXVR_LIST;
typedef VBOXVR_LIST const *PCVBOXVR_LIST;

DECLINLINE(int) VBoxRectCmp(PCRTRECT pRect1, PCRTRECT pRect2)
{
    return memcmp(pRect1, pRect2, sizeof(*pRect1));
}

#ifndef IN_RING0
# define CR_FLOAT_RCAST(_t, _v) ((_t)((float)(_v) + 0.5))

DECLINLINE(void) VBoxRectScale(PRTRECT pRect, float xScale, float yScale)
{
    pRect->xLeft   = CR_FLOAT_RCAST(int32_t, pRect->xLeft   * xScale);
    pRect->yTop    = CR_FLOAT_RCAST(int32_t, pRect->yTop    * yScale);
    pRect->xRight  = CR_FLOAT_RCAST(int32_t, pRect->xRight  * xScale);
    pRect->yBottom = CR_FLOAT_RCAST(int32_t, pRect->yBottom * yScale);
}

DECLINLINE(void) VBoxRectScaled(PCRTRECT pRect, float xScale, float yScale, PRTRECT pResult)
{
    *pResult = *pRect;
    VBoxRectScale(pResult, xScale, yScale);
}

DECLINLINE(void) VBoxRectUnscale(PRTRECT pRect, float xScale, float yScale)
{
    pRect->xLeft   = CR_FLOAT_RCAST(int32_t, pRect->xLeft   / xScale);
    pRect->yTop    = CR_FLOAT_RCAST(int32_t, pRect->yTop    / yScale);
    pRect->xRight  = CR_FLOAT_RCAST(int32_t, pRect->xRight  / xScale);
    pRect->yBottom = CR_FLOAT_RCAST(int32_t, pRect->yBottom / yScale);
}

DECLINLINE(void) VBoxRectUnscaled(PCRTRECT pRect, float xScale, float yScale, PRTRECT pResult)
{
    *pResult = *pRect;
    VBoxRectUnscale(pResult, xScale, yScale);
}

#endif /* IN_RING0 */

DECLINLINE(void) VBoxRectIntersect(PRTRECT pRect1, PCRTRECT pRect2)
{
    Assert(pRect1);
    Assert(pRect2);
    pRect1->xLeft   = RT_MAX(pRect1->xLeft,   pRect2->xLeft);
    pRect1->yTop    = RT_MAX(pRect1->yTop,    pRect2->yTop);
    pRect1->xRight  = RT_MIN(pRect1->xRight,  pRect2->xRight);
    pRect1->yBottom = RT_MIN(pRect1->yBottom, pRect2->yBottom);
    /* ensure the rect is valid */
    pRect1->xRight  = RT_MAX(pRect1->xRight,  pRect1->xLeft);
    pRect1->yBottom = RT_MAX(pRect1->yBottom, pRect1->yTop);
}

DECLINLINE(void) VBoxRectIntersected(PCRTRECT pRect1, PCRTRECT pRect2, PRTRECT pResult)
{
    *pResult = *pRect1;
    VBoxRectIntersect(pResult, pRect2);
}


DECLINLINE(void) VBoxRectTranslate(PRTRECT pRect, int32_t x, int32_t y)
{
    pRect->xLeft   += x;
    pRect->yTop    += y;
    pRect->xRight  += x;
    pRect->yBottom += y;
}

DECLINLINE(void) VBoxRectTranslated(PCRTRECT pRect, int32_t x, int32_t y, PRTRECT pResult)
{
    *pResult = *pRect;
    VBoxRectTranslate(pResult, x, y);
}

DECLINLINE(void) VBoxRectInvertY(PRTRECT pRect)
{
    int32_t y = pRect->yTop;
    pRect->yTop = pRect->yBottom;
    pRect->yBottom = y;
}

DECLINLINE(void) VBoxRectInvertedY(PCRTRECT pRect, PRTRECT pResult)
{
    *pResult = *pRect;
    VBoxRectInvertY(pResult);
}

DECLINLINE(void) VBoxRectMove(PRTRECT pRect, int32_t x, int32_t y)
{
    int32_t cx = pRect->xRight  - pRect->xLeft;
    int32_t cy = pRect->yBottom - pRect->yTop;
    pRect->xLeft   = x;
    pRect->yTop    = y;
    pRect->xRight  = cx + x;
    pRect->yBottom = cy + y;
}

DECLINLINE(void) VBoxRectMoved(PCRTRECT pRect, int32_t x, int32_t y, PRTRECT pResult)
{
    *pResult = *pRect;
    VBoxRectMove(pResult, x, y);
}

DECLINLINE(bool) VBoxRectCovers(PCRTRECT pRect, PCRTRECT pCovered)
{
    AssertPtr(pRect);
    AssertPtr(pCovered);
    if (pRect->xLeft > pCovered->xLeft)
        return false;
    if (pRect->yTop > pCovered->yTop)
        return false;
    if (pRect->xRight < pCovered->xRight)
        return false;
    if (pRect->yBottom < pCovered->yBottom)
        return false;
    return true;
}

DECLINLINE(bool) VBoxRectIsZero(PCRTRECT pRect)
{
    return pRect->xLeft == pRect->xRight || pRect->yTop == pRect->yBottom;
}

DECLINLINE(bool) VBoxRectIsIntersect(PCRTRECT pRect1, PCRTRECT pRect2)
{
    return !(   (pRect1->xLeft < pRect2->xLeft && pRect1->xRight  <= pRect2->xLeft)
             || (pRect2->xLeft < pRect1->xLeft && pRect2->xRight  <= pRect1->xLeft)
             || (pRect1->yTop  < pRect2->yTop  && pRect1->yBottom <= pRect2->yTop)
             || (pRect2->yTop  < pRect1->yTop  && pRect2->yBottom <= pRect1->yTop) );
}

DECLINLINE(uint32_t) VBoxVrListRectsCount(PCVBOXVR_LIST pList)
{
    return pList->cEntries;
}

DECLINLINE(bool) VBoxVrListIsEmpty(PCVBOXVR_LIST pList)
{
    return !VBoxVrListRectsCount(pList);
}

DECLINLINE(void) VBoxVrListInit(PVBOXVR_LIST pList)
{
    RTListInit(&pList->ListHead);
    pList->cEntries = 0;
}

VBOXVREGDECL(void) VBoxVrListClear(PVBOXVR_LIST pList);

/* moves list data to pDstList and empties the pList */
VBOXVREGDECL(void) VBoxVrListMoveTo(PVBOXVR_LIST pList, PVBOXVR_LIST pDstList);

VBOXVREGDECL(void) VBoxVrListTranslate(PVBOXVR_LIST pList, int32_t x, int32_t y);

VBOXVREGDECL(int) VBoxVrListCmp(PCVBOXVR_LIST pList1, PCVBOXVR_LIST pList2);

VBOXVREGDECL(int) VBoxVrListRectsSet(PVBOXVR_LIST pList, uint32_t cRects, PCRTRECT paRects, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrListRectsAdd(PVBOXVR_LIST pList, uint32_t cRects, PCRTRECT paRects, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrListRectsSubst(PVBOXVR_LIST pList, uint32_t cRects, PCRTRECT paRects, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrListRectsGet(PVBOXVR_LIST pList, uint32_t cRects, PRTRECT paRects);

VBOXVREGDECL(int) VBoxVrListClone(PCVBOXVR_LIST pList, VBOXVR_LIST *pDstList);

/* NOTE: with the current implementation the VBoxVrListIntersect is faster than VBoxVrListRectsIntersect,
 * i.e. VBoxVrListRectsIntersect is actually a convenience function that create a temporary list and calls
 * VBoxVrListIntersect internally. */
VBOXVREGDECL(int) VBoxVrListRectsIntersect(PVBOXVR_LIST pList, uint32_t cRects, PCRTRECT paRects, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrListIntersect(PVBOXVR_LIST pList, PCVBOXVR_LIST pList2, bool *pfChanged);

VBOXVREGDECL(int) VBoxVrInit(void);
VBOXVREGDECL(void) VBoxVrTerm(void);

typedef struct VBOXVR_LIST_ITERATOR
{
    PVBOXVR_LIST pList;
    PRTLISTNODE pNextEntry;
} VBOXVR_LIST_ITERATOR;
typedef VBOXVR_LIST_ITERATOR *PVBOXVR_LIST_ITERATOR;
typedef VBOXVR_LIST_ITERATOR const *PCVBOXVR_LIST_ITERATOR;

DECLINLINE(void) VBoxVrListIterInit(PVBOXVR_LIST pList, PVBOXVR_LIST_ITERATOR pIter)
{
    pIter->pList = pList;
    pIter->pNextEntry = pList->ListHead.pNext;
}

typedef struct VBOXVR_REG
{
    RTLISTNODE ListEntry;
    RTRECT Rect;
} VBOXVR_REG;
typedef VBOXVR_REG *PVBOXVR_REG;
typedef VBOXVR_REG const *PCVBOXVR_REG;

#define PVBOXVR_REG_FROM_ENTRY(_pEntry)     RT_FROM_MEMBER(_pEntry, VBOXVR_REG, ListEntry)

DECLINLINE(PCRTRECT) VBoxVrListIterNext(PVBOXVR_LIST_ITERATOR pIter)
{
    PRTLISTNODE pNextEntry = pIter->pNextEntry;
    if (pNextEntry != &pIter->pList->ListHead)
    {
        PCRTRECT pRect = &PVBOXVR_REG_FROM_ENTRY(pNextEntry)->Rect;
        pIter->pNextEntry = pNextEntry->pNext;
        return pRect;
    }
    return NULL;
}

typedef struct VBOXVR_COMPOSITOR_ENTRY
{
    RTLISTNODE Node;
    VBOXVR_LIST Vr;
    uint32_t cRefs;
} VBOXVR_COMPOSITOR_ENTRY;
typedef VBOXVR_COMPOSITOR_ENTRY *PVBOXVR_COMPOSITOR_ENTRY;
typedef VBOXVR_COMPOSITOR_ENTRY const *PCVBOXVR_COMPOSITOR_ENTRY;

struct VBOXVR_COMPOSITOR;

typedef DECLCALLBACK(void) FNVBOXVRCOMPOSITOR_ENTRY_RELEASED(const struct VBOXVR_COMPOSITOR *pCompositor,
                                                             PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                             PVBOXVR_COMPOSITOR_ENTRY pReplacingEntry);
typedef FNVBOXVRCOMPOSITOR_ENTRY_RELEASED *PFNVBOXVRCOMPOSITOR_ENTRY_RELEASED;

typedef struct VBOXVR_COMPOSITOR
{
    RTLISTANCHOR List;
    PFNVBOXVRCOMPOSITOR_ENTRY_RELEASED pfnEntryReleased;
} VBOXVR_COMPOSITOR;
typedef VBOXVR_COMPOSITOR *PVBOXVR_COMPOSITOR;
typedef VBOXVR_COMPOSITOR const *PCVBOXVR_COMPOSITOR;

typedef DECLCALLBACK(bool) FNVBOXVRCOMPOSITOR_VISITOR(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                      void *pvVisitor);
typedef FNVBOXVRCOMPOSITOR_VISITOR *PFNVBOXVRCOMPOSITOR_VISITOR;

VBOXVREGDECL(void) VBoxVrCompositorInit(PVBOXVR_COMPOSITOR pCompositor, PFNVBOXVRCOMPOSITOR_ENTRY_RELEASED pfnEntryReleased);
VBOXVREGDECL(void) VBoxVrCompositorClear(PVBOXVR_COMPOSITOR pCompositor);
VBOXVREGDECL(void) VBoxVrCompositorRegionsClear(PVBOXVR_COMPOSITOR pCompositor, bool *pfChanged);
VBOXVREGDECL(void) VBoxVrCompositorEntryInit(PVBOXVR_COMPOSITOR_ENTRY pEntry);

DECLINLINE(bool) VBoxVrCompositorEntryIsInList(PCVBOXVR_COMPOSITOR_ENTRY pEntry)
{
    return !VBoxVrListIsEmpty(&pEntry->Vr);
}

#define CRBLT_F_LINEAR                  0x00000001
#define CRBLT_F_INVERT_SRC_YCOORDS      0x00000002
#define CRBLT_F_INVERT_DST_YCOORDS      0x00000004
#define CRBLT_F_INVERT_YCOORDS          (CRBLT_F_INVERT_SRC_YCOORDS | CRBLT_F_INVERT_DST_YCOORDS)
/* the blit operation with discard the source alpha channel values and set the destination alpha values to 1.0 */
#define CRBLT_F_NOALPHA                 0x00000010

#define CRBLT_FTYPE_XOR                 CRBLT_F_INVERT_YCOORDS
#define CRBLT_FTYPE_OR                  (CRBLT_F_LINEAR | CRBLT_F_NOALPHA)
#define CRBLT_FOP_COMBINE(_f1, _f2)     ((((_f1) ^ (_f2)) & CRBLT_FTYPE_XOR) | (((_f1) | (_f2)) & CRBLT_FTYPE_OR))

#define CRBLT_FLAGS_FROM_FILTER(_f)     ( ((_f) & GL_LINEAR) ? CRBLT_F_LINEAR : 0)
#define CRBLT_FILTER_FROM_FLAGS(_f)     (((_f) & CRBLT_F_LINEAR) ? GL_LINEAR : GL_NEAREST)

/* compositor regions changed */
#define VBOXVR_COMPOSITOR_CF_REGIONS_CHANGED                           0x00000001
/* other entries changed along while doing current entry modification
 * always comes with VBOXVR_COMPOSITOR_CF_ENTRY_REGIONS_CHANGED */
#define VBOXVR_COMPOSITOR_CF_OTHER_ENTRIES_REGIONS_CHANGED             0x00000002
/* only current entry regions changed
 * can come wither with VBOXVR_COMPOSITOR_CF_REGIONS_CHANGED or with VBOXVR_COMPOSITOR_CF_ENTRY_REPLACED */
#define VBOXVR_COMPOSITOR_CF_ENTRY_REGIONS_CHANGED                     0x00000004
/* the given entry has replaced some other entry, while overal regions did NOT change.
 * always comes with VBOXVR_COMPOSITOR_CF_ENTRY_REGIONS_CHANGED */
#define VBOXVR_COMPOSITOR_CF_ENTRY_REPLACED                            0x00000008


VBOXVREGDECL(bool) VBoxVrCompositorEntryRemove(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry);
VBOXVREGDECL(bool) VBoxVrCompositorEntryReplace(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                PVBOXVR_COMPOSITOR_ENTRY pNewEntry);
VBOXVREGDECL(int) VBoxVrCompositorEntryRegionsAdd(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                  uint32_t cRegions, PCRTRECT paRegions,
                                                  PVBOXVR_COMPOSITOR_ENTRY *ppReplacedEntry, uint32_t *pfChangeFlags);
VBOXVREGDECL(int) VBoxVrCompositorEntryRegionsSubst(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                    uint32_t cRegions, PCRTRECT paRegions, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrCompositorEntryRegionsSet(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                  uint32_t cRegions, PCRTRECT paRegions, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrCompositorEntryRegionsIntersect(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                        uint32_t cRegions, PCRTRECT paRegions, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrCompositorEntryListIntersect(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                     PCVBOXVR_LIST pList2, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrCompositorEntryRegionsIntersectAll(PVBOXVR_COMPOSITOR pCompositor, uint32_t cRegions, PCRTRECT paRegions,
                                                           bool *pfChanged);
VBOXVREGDECL(int) VBoxVrCompositorEntryListIntersectAll(PVBOXVR_COMPOSITOR pCompositor, PCVBOXVR_LIST pList2, bool *pfChanged);
VBOXVREGDECL(int) VBoxVrCompositorEntryRegionsTranslate(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ENTRY pEntry,
                                                        int32_t x, int32_t y, bool *pfChanged);
VBOXVREGDECL(void) VBoxVrCompositorVisit(PVBOXVR_COMPOSITOR pCompositor, PFNVBOXVRCOMPOSITOR_VISITOR pfnVisitor, void *pvVisitor);

DECLINLINE(bool) VBoxVrCompositorIsEmpty(PCVBOXVR_COMPOSITOR pCompositor)
{
    return RTListIsEmpty(&pCompositor->List);
}

typedef struct VBOXVR_COMPOSITOR_ITERATOR
{
    PVBOXVR_COMPOSITOR pCompositor;
    PRTLISTNODE pNextEntry;
} VBOXVR_COMPOSITOR_ITERATOR;
typedef VBOXVR_COMPOSITOR_ITERATOR *PVBOXVR_COMPOSITOR_ITERATOR;

typedef struct VBOXVR_COMPOSITOR_CONST_ITERATOR
{
    PCVBOXVR_COMPOSITOR pCompositor;
    PCRTLISTNODE pNextEntry;
} VBOXVR_COMPOSITOR_CONST_ITERATOR;
typedef VBOXVR_COMPOSITOR_CONST_ITERATOR *PVBOXVR_COMPOSITOR_CONST_ITERATOR;

DECLINLINE(void) VBoxVrCompositorIterInit(PVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_ITERATOR pIter)
{
    pIter->pCompositor = pCompositor;
    pIter->pNextEntry = pCompositor->List.pNext;
}

DECLINLINE(void) VBoxVrCompositorConstIterInit(PCVBOXVR_COMPOSITOR pCompositor, PVBOXVR_COMPOSITOR_CONST_ITERATOR pIter)
{
    pIter->pCompositor = pCompositor;
    pIter->pNextEntry = pCompositor->List.pNext;
}

#define VBOXVR_COMPOSITOR_ENTRY_FROM_NODE(_p)       RT_FROM_MEMBER(_p, VBOXVR_COMPOSITOR_ENTRY, Node)
#define VBOXVR_COMPOSITOR_CONST_ENTRY_FROM_NODE(_p) RT_FROM_MEMBER(_p, const VBOXVR_COMPOSITOR_ENTRY, Node)

DECLINLINE(PVBOXVR_COMPOSITOR_ENTRY) VBoxVrCompositorIterNext(PVBOXVR_COMPOSITOR_ITERATOR pIter)
{
    PRTLISTNODE pNextEntry = pIter->pNextEntry;
    if (pNextEntry != &pIter->pCompositor->List)
    {
        PVBOXVR_COMPOSITOR_ENTRY pEntry = VBOXVR_COMPOSITOR_ENTRY_FROM_NODE(pNextEntry);
        pIter->pNextEntry = pNextEntry->pNext;
        return pEntry;
    }
    return NULL;
}

DECLINLINE(PCVBOXVR_COMPOSITOR_ENTRY) VBoxVrCompositorConstIterNext(PVBOXVR_COMPOSITOR_CONST_ITERATOR pIter)
{
    PCRTLISTNODE pNextEntry = pIter->pNextEntry;
    if (pNextEntry != &pIter->pCompositor->List)
    {
        PCVBOXVR_COMPOSITOR_ENTRY pEntry = VBOXVR_COMPOSITOR_CONST_ENTRY_FROM_NODE(pNextEntry);
        pIter->pNextEntry = pNextEntry->pNext;
        return pEntry;
    }
    return NULL;
}

typedef struct VBOXVR_TEXTURE
{
    int32_t  width;
    int32_t  height;
    uint32_t target;
    uint32_t hwid;
} VBOXVR_TEXTURE;
typedef VBOXVR_TEXTURE *PVBOXVR_TEXTURE;
typedef VBOXVR_TEXTURE const *PCVBOXVR_TEXTURE;

RT_C_DECLS_END

#endif

