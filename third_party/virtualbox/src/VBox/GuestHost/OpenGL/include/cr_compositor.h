/* $Id: cr_compositor.h $ */
/** @file
 * Compositor API.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___cr_compositor_h
#define ___cr_compositor_h

#include "cr_vreg.h"
#include "cr_blitter.h"


/* Compositor with Stretching & Cached Rectangles info */

RT_C_DECLS_BEGIN

struct VBOXVR_SCR_COMPOSITOR_ENTRY;
struct VBOXVR_SCR_COMPOSITOR;

typedef DECLCALLBACK(void) FNVBOXVRSCRCOMPOSITOR_ENTRY_RELEASED(const struct VBOXVR_SCR_COMPOSITOR *pCompositor,
                                                                struct VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry,
                                                                struct VBOXVR_SCR_COMPOSITOR_ENTRY *pReplacingEntry);
typedef FNVBOXVRSCRCOMPOSITOR_ENTRY_RELEASED *PFNVBOXVRSCRCOMPOSITOR_ENTRY_RELEASED;


typedef struct VBOXVR_SCR_COMPOSITOR_ENTRY
{
    VBOXVR_COMPOSITOR_ENTRY Ce;
    RTRECT Rect;
    uint32_t fChanged;
    uint32_t fFlags;
    uint32_t cRects;
    PRTRECT paSrcRects;
    PRTRECT paDstRects;
    PRTRECT paDstUnstretchedRects;
    PFNVBOXVRSCRCOMPOSITOR_ENTRY_RELEASED pfnEntryReleased;
    PCR_TEXDATA pTex;
} VBOXVR_SCR_COMPOSITOR_ENTRY;
typedef VBOXVR_SCR_COMPOSITOR_ENTRY *PVBOXVR_SCR_COMPOSITOR_ENTRY;
typedef VBOXVR_SCR_COMPOSITOR_ENTRY const *PCVBOXVR_SCR_COMPOSITOR_ENTRY;

typedef struct VBOXVR_SCR_COMPOSITOR
{
    VBOXVR_COMPOSITOR Compositor;
    RTRECT Rect;
#ifndef IN_RING0
    float StretchX;
    float StretchY;
#endif
    uint32_t fFlags;
    uint32_t cRects;
    uint32_t cRectsBuffer;
    PRTRECT paSrcRects;
    PRTRECT paDstRects;
    PRTRECT paDstUnstretchedRects;
} VBOXVR_SCR_COMPOSITOR;
typedef VBOXVR_SCR_COMPOSITOR *PVBOXVR_SCR_COMPOSITOR;
typedef VBOXVR_SCR_COMPOSITOR const *PCVBOXVR_SCR_COMPOSITOR;


typedef DECLCALLBACK(bool) FNVBOXVRSCRCOMPOSITOR_VISITOR(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                         void *pvVisitor);
typedef FNVBOXVRSCRCOMPOSITOR_VISITOR *PFNVBOXVRSCRCOMPOSITOR_VISITOR;

DECLINLINE(void) CrVrScrCompositorEntryInit(PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry, PCRTRECT pRect, CR_TEXDATA *pTex,
                                            PFNVBOXVRSCRCOMPOSITOR_ENTRY_RELEASED pfnEntryReleased)
{
    memset(pEntry, 0, sizeof (*pEntry));
    VBoxVrCompositorEntryInit(&pEntry->Ce);
    pEntry->Rect = *pRect;
    pEntry->pfnEntryReleased = pfnEntryReleased;
    if (pTex)
    {
        CrTdAddRef(pTex);
        pEntry->pTex = pTex;
    }
}

DECLINLINE(void) CrVrScrCompositorEntryCleanup(PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    if (pEntry->pTex)
    {
        CrTdRelease(pEntry->pTex);
        pEntry->pTex = NULL;
    }
}

DECLINLINE(bool) CrVrScrCompositorEntryIsUsed(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    return VBoxVrCompositorEntryIsInList(&pEntry->Ce);
}

DECLINLINE(void) CrVrScrCompositorEntrySetChanged(PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry, bool fChanged)
{
    pEntry->fChanged = !!fChanged;
}

DECLINLINE(void) CrVrScrCompositorEntryTexSet(PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry, CR_TEXDATA *pTex)
{
    if (pEntry->pTex)
        CrTdRelease(pEntry->pTex);

    if (pTex)
        CrTdAddRef(pTex);

    pEntry->pTex = pTex;
}

DECLINLINE(CR_TEXDATA *) CrVrScrCompositorEntryTexGet(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    return pEntry->pTex;
}

DECLINLINE(bool) CrVrScrCompositorEntryIsChanged(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    return !!pEntry->fChanged;
}

DECLINLINE(bool) CrVrScrCompositorIsEmpty(PCVBOXVR_SCR_COMPOSITOR pCompositor)
{
    return VBoxVrCompositorIsEmpty(&pCompositor->Compositor);
}

VBOXVREGDECL(int) CrVrScrCompositorEntryRectSet(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                PCRTRECT pRect);
VBOXVREGDECL(int) CrVrScrCompositorEntryTexAssign(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                  CR_TEXDATA *pTex);
VBOXVREGDECL(void) CrVrScrCompositorVisit(PVBOXVR_SCR_COMPOSITOR pCompositor, PFNVBOXVRSCRCOMPOSITOR_VISITOR pfnVisitor,
                                          void *pvVisitor);
VBOXVREGDECL(void) CrVrScrCompositorEntrySetAllChanged(PVBOXVR_SCR_COMPOSITOR pCompositor, bool fChanged);
DECLINLINE(bool) CrVrScrCompositorEntryIsInList(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    return VBoxVrCompositorEntryIsInList(&pEntry->Ce);
}
VBOXVREGDECL(int) CrVrScrCompositorEntryRegionsAdd(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                   PCRTPOINT pPos, uint32_t cRegions, PCRTRECT paRegions, bool fPosRelated,
                                                   VBOXVR_SCR_COMPOSITOR_ENTRY **ppReplacedScrEntry, uint32_t *pfChangeFlags);
VBOXVREGDECL(int) CrVrScrCompositorEntryRegionsSet(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                   PCRTPOINT pPos, uint32_t cRegions, PCRTRECT paRegions, bool fPosRelated,
                                                   bool *pfChanged);
VBOXVREGDECL(int) CrVrScrCompositorEntryListIntersect(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                      PCVBOXVR_LIST pList2, bool *pfChanged);
VBOXVREGDECL(int) CrVrScrCompositorEntryRegionsIntersect(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                         uint32_t cRegions, PCRTRECT paRegions, bool *pfChanged);
VBOXVREGDECL(int) CrVrScrCompositorEntryRegionsIntersectAll(PVBOXVR_SCR_COMPOSITOR pCompositor, uint32_t cRegions,
                                                            PCRTRECT paRegions, bool *pfChanged);
VBOXVREGDECL(int) CrVrScrCompositorEntryListIntersectAll(PVBOXVR_SCR_COMPOSITOR pCompositor, PCVBOXVR_LIST pList2,
                                                         bool *pfChanged);
VBOXVREGDECL(int) CrVrScrCompositorEntryPosSet(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                               PCRTPOINT pPos);
DECLINLINE(PCRTRECT) CrVrScrCompositorEntryRectGet(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    return &pEntry->Rect;
}

/* regions are valid until the next CrVrScrCompositor call */
VBOXVREGDECL(int) CrVrScrCompositorEntryRegionsGet(PCVBOXVR_SCR_COMPOSITOR pCompositor,
                                                   PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry, uint32_t *pcRegions,
                                                   PCRTRECT *ppaSrcRegions, PCRTRECT *ppaDstRegions,
                                                   PCRTRECT *ppaDstUnstretchedRects);
VBOXVREGDECL(int) CrVrScrCompositorEntryRemove(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry);
VBOXVREGDECL(bool) CrVrScrCompositorEntryReplace(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                 PVBOXVR_SCR_COMPOSITOR_ENTRY pNewEntry);
VBOXVREGDECL(void) CrVrScrCompositorEntryFlagsSet(PVBOXVR_SCR_COMPOSITOR_ENTRY pEntry, uint32_t fFlags);
VBOXVREGDECL(uint32_t) CrVrScrCompositorEntryFlagsCombinedGet(PCVBOXVR_SCR_COMPOSITOR pCompositor,
                                                              PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry);
DECLINLINE(uint32_t) CrVrScrCompositorEntryFlagsGet(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry)
{
    return pEntry->fFlags;
}

VBOXVREGDECL(void) CrVrScrCompositorInit(PVBOXVR_SCR_COMPOSITOR pCompositor, PCRTRECT pRect);
VBOXVREGDECL(int) CrVrScrCompositorRectSet(PVBOXVR_SCR_COMPOSITOR pCompositor, PCRTRECT pRect, bool *pfChanged);
DECLINLINE(PCRTRECT) CrVrScrCompositorRectGet(PCVBOXVR_SCR_COMPOSITOR pCompositor)
{
    return &pCompositor->Rect;
}

VBOXVREGDECL(void) CrVrScrCompositorClear(PVBOXVR_SCR_COMPOSITOR pCompositor);
VBOXVREGDECL(void) CrVrScrCompositorRegionsClear(PVBOXVR_SCR_COMPOSITOR pCompositor, bool *pfChanged);

typedef DECLCALLBACK(VBOXVR_SCR_COMPOSITOR_ENTRY*) FNVBOXVR_SCR_COMPOSITOR_ENTRY_FOR(PCVBOXVR_SCR_COMPOSITOR_ENTRY pEntry,
                                                                                     void *pvContext);
typedef FNVBOXVR_SCR_COMPOSITOR_ENTRY_FOR *PFNVBOXVR_SCR_COMPOSITOR_ENTRY_FOR;

VBOXVREGDECL(int) CrVrScrCompositorClone(PCVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR pDstCompositor,
                                         PFNVBOXVR_SCR_COMPOSITOR_ENTRY_FOR pfnEntryFor, void *pvEntryFor);
VBOXVREGDECL(int) CrVrScrCompositorIntersectList(PVBOXVR_SCR_COMPOSITOR pCompositor, PCVBOXVR_LIST pVr, bool *pfChanged);
VBOXVREGDECL(int) CrVrScrCompositorIntersectedList(PCVBOXVR_SCR_COMPOSITOR pCompositor, PCVBOXVR_LIST pVr,
                                                   PVBOXVR_SCR_COMPOSITOR pDstCompositor,
                                                   PFNVBOXVR_SCR_COMPOSITOR_ENTRY_FOR pfnEntryFor, void *pvEntryFor, bool *pfChanged);
#ifndef IN_RING0
VBOXVREGDECL(void) CrVrScrCompositorSetStretching(PVBOXVR_SCR_COMPOSITOR pCompositor, float StretchX, float StretchY);
DECLINLINE(void) CrVrScrCompositorGetStretching(PCVBOXVR_SCR_COMPOSITOR pCompositor, float *pStretchX, float *pStretchY)
{
    if (pStretchX)
        *pStretchX = pCompositor->StretchX;

    if (pStretchY)
        *pStretchY = pCompositor->StretchY;
}
#endif
/* regions are valid until the next CrVrScrCompositor call */
VBOXVREGDECL(int) CrVrScrCompositorRegionsGet(PCVBOXVR_SCR_COMPOSITOR pCompositor, uint32_t *pcRegions,
                                              PCRTRECT *ppaSrcRegions, PCRTRECT *ppaDstRegions, PCRTRECT *ppaDstUnstretchedRects);

#define VBOXVR_SCR_COMPOSITOR_ENTRY_FROM_ENTRY(_p)          RT_FROM_MEMBER(_p, VBOXVR_SCR_COMPOSITOR_ENTRY, Ce)
#define VBOXVR_SCR_COMPOSITOR_CONST_ENTRY_FROM_ENTRY(_p)    RT_FROM_MEMBER(_p, const VBOXVR_SCR_COMPOSITOR_ENTRY, Ce)
#define VBOXVR_SCR_COMPOSITOR_FROM_COMPOSITOR(_p)           RT_FROM_MEMBER(_p, VBOXVR_SCR_COMPOSITOR, Compositor)

typedef struct VBOXVR_SCR_COMPOSITOR_ITERATOR
{
    VBOXVR_COMPOSITOR_ITERATOR Base;
} VBOXVR_SCR_COMPOSITOR_ITERATOR;
typedef VBOXVR_SCR_COMPOSITOR_ITERATOR *PVBOXVR_SCR_COMPOSITOR_ITERATOR;

typedef struct VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR
{
    VBOXVR_COMPOSITOR_CONST_ITERATOR Base;
} VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR;
typedef VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR *PVBOXVR_SCR_COMPOSITOR_CONST_ITERATOR;

DECLINLINE(void) CrVrScrCompositorIterInit(PVBOXVR_SCR_COMPOSITOR pCompositor, PVBOXVR_SCR_COMPOSITOR_ITERATOR pIter)
{
    VBoxVrCompositorIterInit(&pCompositor->Compositor, &pIter->Base);
}

DECLINLINE(void) CrVrScrCompositorConstIterInit(PCVBOXVR_SCR_COMPOSITOR pCompositor,
                                                PVBOXVR_SCR_COMPOSITOR_CONST_ITERATOR pIter)
{
    VBoxVrCompositorConstIterInit(&pCompositor->Compositor, &pIter->Base);
}

DECLINLINE(PVBOXVR_SCR_COMPOSITOR_ENTRY) CrVrScrCompositorIterNext(PVBOXVR_SCR_COMPOSITOR_ITERATOR pIter)
{
    PVBOXVR_COMPOSITOR_ENTRY pCe = VBoxVrCompositorIterNext(&pIter->Base);
    if (pCe)
        return VBOXVR_SCR_COMPOSITOR_ENTRY_FROM_ENTRY(pCe);
    return NULL;
}

DECLINLINE(PCVBOXVR_SCR_COMPOSITOR_ENTRY) CrVrScrCompositorConstIterNext(PVBOXVR_SCR_COMPOSITOR_CONST_ITERATOR pIter)
{
    PCVBOXVR_COMPOSITOR_ENTRY pCe = VBoxVrCompositorConstIterNext(&pIter->Base);
    if (pCe)
        return VBOXVR_SCR_COMPOSITOR_CONST_ENTRY_FROM_ENTRY(pCe);
    return NULL;
}

RT_C_DECLS_END

#endif

