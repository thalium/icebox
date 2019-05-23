/* $Id: vboxext.h $ */
/** @file
 * VBox extension to Wine D3D
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBOXEXT_H__
#define ___VBOXEXT_H__

#ifdef VBOX_WINE_WITHOUT_LIBWINE
# include <iprt/win/windows.h>
#endif

#include <iprt/list.h>

HRESULT VBoxExtCheckInit(void);
HRESULT VBoxExtCheckTerm(void);
#if defined(VBOX_WINE_WITH_SINGLE_CONTEXT) || defined(VBOX_WINE_WITH_SINGLE_SWAPCHAIN_CONTEXT)
# ifndef VBOX_WITH_WDDM
/* Windows destroys HDC created by a given thread when the thread is terminated
 * this leads to a mess-up in Wine & Chromium code in some situations, e.g.
 * D3D device is created in one thread, then the thread is terminated,
 * then device is started to be used in another thread */
HDC VBoxExtGetDC(HWND hWnd);
int VBoxExtReleaseDC(HWND hWnd, HDC hDC);
# endif
/* We need to do a VBoxTlsRefRelease for the current thread context on thread exit to avoid memory leaking
 * Calling VBoxTlsRefRelease may result in a call to context dtor callback, which is supposed to be run under wined3d lock.
 * We can not acquire a wined3d lock in DllMain since this would result in a lock order violation, which may result in a deadlock.
 * In other words, wined3d may internally call Win32 API functions which result in a DLL lock acquisition while holding wined3d lock.
 * So lock order should always be "wined3d lock" -> "dll lock".
 * To avoid possible deadlocks we make an asynchronous call to a worker thread to make a context release from there. */
struct wined3d_context;
void VBoxExtReleaseContextAsync(struct wined3d_context *context);
#endif

/* API for creating & destroying windows */
HRESULT VBoxExtWndDestroy(HWND hWnd, HDC hDC);
HRESULT VBoxExtWndCreate(DWORD width, DWORD height, HWND *phWnd, HDC *phDC);


/* hashmap */
typedef DECLCALLBACK(uint32_t) FNVBOXEXT_HASHMAP_HASH(void *pvKey);
typedef FNVBOXEXT_HASHMAP_HASH *PFNVBOXEXT_HASHMAP_HASH;

typedef DECLCALLBACK(bool) FNVBOXEXT_HASHMAP_EQUAL(void *pvKey1, void *pvKey2);
typedef FNVBOXEXT_HASHMAP_EQUAL *PFNVBOXEXT_HASHMAP_EQUAL;

struct VBOXEXT_HASHMAP;
struct VBOXEXT_HASHMAP_ENTRY;
typedef DECLCALLBACK(bool) FNVBOXEXT_HASHMAP_VISITOR(struct VBOXEXT_HASHMAP *pMap, void *pvKey, struct VBOXEXT_HASHMAP_ENTRY *pValue, void *pvVisitor);
typedef FNVBOXEXT_HASHMAP_VISITOR *PFNVBOXEXT_HASHMAP_VISITOR;

typedef struct VBOXEXT_HASHMAP_ENTRY
{
    RTLISTNODE ListNode;
    void *pvKey;
    uint32_t u32Hash;
} VBOXEXT_HASHMAP_ENTRY, *PVBOXEXT_HASHMAP_ENTRY;

typedef struct VBOXEXT_HASHMAP_BUCKET
{
    RTLISTNODE EntryList;
} VBOXEXT_HASHMAP_BUCKET, *PVBOXEXT_HASHMAP_BUCKET;

#define VBOXEXT_HASHMAP_NUM_BUCKETS 29

typedef struct VBOXEXT_HASHMAP
{
    PFNVBOXEXT_HASHMAP_HASH pfnHash;
    PFNVBOXEXT_HASHMAP_EQUAL pfnEqual;
    uint32_t cEntries;
    VBOXEXT_HASHMAP_BUCKET aBuckets[VBOXEXT_HASHMAP_NUM_BUCKETS];
} VBOXEXT_HASHMAP, *PVBOXEXT_HASHMAP;

void VBoxExtHashInit(PVBOXEXT_HASHMAP pMap, PFNVBOXEXT_HASHMAP_HASH pfnHash, PFNVBOXEXT_HASHMAP_EQUAL pfnEqual);
PVBOXEXT_HASHMAP_ENTRY VBoxExtHashPut(PVBOXEXT_HASHMAP pMap, void *pvKey, PVBOXEXT_HASHMAP_ENTRY pEntry);
PVBOXEXT_HASHMAP_ENTRY VBoxExtHashGet(PVBOXEXT_HASHMAP pMap, void *pvKey);
PVBOXEXT_HASHMAP_ENTRY VBoxExtHashRemove(PVBOXEXT_HASHMAP pMap, void *pvKey);
void* VBoxExtHashRemoveEntry(PVBOXEXT_HASHMAP pMap, PVBOXEXT_HASHMAP_ENTRY pEntry);
void VBoxExtHashVisit(PVBOXEXT_HASHMAP pMap, PFNVBOXEXT_HASHMAP_VISITOR pfnVisitor, void *pvVisitor);
void VBoxExtHashCleanup(PVBOXEXT_HASHMAP pMap, PFNVBOXEXT_HASHMAP_VISITOR pfnVisitor, void *pvVisitor);

DECLINLINE(uint32_t) VBoxExtHashSize(PVBOXEXT_HASHMAP pMap)
{
    return pMap->cEntries;
}

DECLINLINE(void*) VBoxExtHashEntryKey(PVBOXEXT_HASHMAP_ENTRY pEntry)
{
    return pEntry->pvKey;
}

struct VBOXEXT_HASHCACHE_ENTRY;
typedef DECLCALLBACK(void) FNVBOXEXT_HASHCACHE_CLEANUP_ENTRY(void *pvKey, struct VBOXEXT_HASHCACHE_ENTRY *pEntry);
typedef FNVBOXEXT_HASHCACHE_CLEANUP_ENTRY *PFNVBOXEXT_HASHCACHE_CLEANUP_ENTRY;

typedef struct VBOXEXT_HASHCACHE_ENTRY
{
    VBOXEXT_HASHMAP_ENTRY MapEntry;
    uint32_t u32Usage;
} VBOXEXT_HASHCACHE_ENTRY, *PVBOXEXT_HASHCACHE_ENTRY;

typedef struct VBOXEXT_HASHCACHE
{
    VBOXEXT_HASHMAP Map;
    uint32_t cMaxElements;
    PFNVBOXEXT_HASHCACHE_CLEANUP_ENTRY pfnCleanupEntry;
} VBOXEXT_HASHCACHE, *PVBOXEXT_HASHCACHE;

#define VBOXEXT_HASHCACHE_FROM_MAP(_pMap) RT_FROM_MEMBER((_pMap), VBOXEXT_HASHCACHE, Map)
#define VBOXEXT_HASHCACHE_ENTRY_FROM_MAP(_pEntry) RT_FROM_MEMBER((_pEntry), VBOXEXT_HASHCACHE_ENTRY, MapEntry)

DECLINLINE(void) VBoxExtCacheInit(PVBOXEXT_HASHCACHE pCache, uint32_t cMaxElements,
        PFNVBOXEXT_HASHMAP_HASH pfnHash,
        PFNVBOXEXT_HASHMAP_EQUAL pfnEqual,
        PFNVBOXEXT_HASHCACHE_CLEANUP_ENTRY pfnCleanupEntry)
{
    VBoxExtHashInit(&pCache->Map, pfnHash, pfnEqual);
    pCache->cMaxElements = cMaxElements;
    pCache->pfnCleanupEntry = pfnCleanupEntry;
}

DECLINLINE(PVBOXEXT_HASHCACHE_ENTRY) VBoxExtCacheGet(PVBOXEXT_HASHCACHE pCache, void *pvKey)
{
    PVBOXEXT_HASHMAP_ENTRY pEntry = VBoxExtHashRemove(&pCache->Map, pvKey);
    return VBOXEXT_HASHCACHE_ENTRY_FROM_MAP(pEntry);
}

DECLINLINE(void) VBoxExtCachePut(PVBOXEXT_HASHCACHE pCache, void *pvKey, PVBOXEXT_HASHCACHE_ENTRY pEntry)
{
    PVBOXEXT_HASHMAP_ENTRY pOldEntry = VBoxExtHashPut(&pCache->Map, pvKey, &pEntry->MapEntry);
    PVBOXEXT_HASHCACHE_ENTRY pOld;
    if (!pOldEntry)
        return;
    pOld = VBOXEXT_HASHCACHE_ENTRY_FROM_MAP(pOldEntry);
    if (pOld != pEntry)
        pCache->pfnCleanupEntry(pvKey, pOld);
}

void VBoxExtCacheCleanup(PVBOXEXT_HASHCACHE pCache);

DECLINLINE(void) VBoxExtCacheTerm(PVBOXEXT_HASHCACHE pCache)
{
    VBoxExtCacheCleanup(pCache);
}

#ifdef VBOX_WINE_WITH_PROFILE

#include <iprt/time.h>

void vboxWDbgPrintF(char * szString, ...);

#define VBOXWINEPROFILE_GET_TIME_NANO() RTTimeNanoTS()
#define VBOXWINEPROFILE_GET_TIME_MILLI() RTTimeMilliTS()

# define PRLOG(_m) do {\
        vboxWDbgPrintF _m ; \
    } while (0)

typedef struct VBOXWINEPROFILE_ELEMENT
{
    uint64_t u64Time;
    uint32_t cu32Calls;
} VBOXWINEPROFILE_ELEMENT, *PVBOXWINEPROFILE_ELEMENT;

typedef struct VBOXWINEPROFILE_HASHMAP_ELEMENT
{
    VBOXEXT_HASHMAP_ENTRY MapEntry;
    VBOXWINEPROFILE_ELEMENT Data;
} VBOXWINEPROFILE_HASHMAP_ELEMENT, *PVBOXWINEPROFILE_HASHMAP_ELEMENT;

#define VBOXWINEPROFILE_HASHMAP_ELEMENT_FROMENTRY(_p) ((PVBOXWINEPROFILE_HASHMAP_ELEMENT)(((uint8_t*)(_p)) - RT_OFFSETOF(VBOXWINEPROFILE_HASHMAP_ELEMENT, MapEntry)))

#define VBOXWINEPROFILE_ELEMENT_DUMP(_p, _pn) do { \
        PRLOG(("%s: t(%u);c(%u)\n", \
            (_pn), \
            (uint32_t)((_p)->u64Time / 1000000), \
            (_p)->cu32Calls \
            )); \
    } while (0)

#define VBOXWINEPROFILE_ELEMENT_RESET(_p) do { \
        memset(_p, 0, sizeof (*(_p))); \
    } while (0)

#define VBOXWINEPROFILE_ELEMENT_STEP(_p, _t) do { \
        (_p)->u64Time += (_t); \
        ++(_p)->cu32Calls; \
    } while (0)

#define VBOXWINEPROFILE_HASHMAP_ELEMENT_CREATE()  ( (PVBOXWINEPROFILE_HASHMAP_ELEMENT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (VBOXWINEPROFILE_HASHMAP_ELEMENT)) )

#define VBOXWINEPROFILE_HASHMAP_ELEMENT_TERM(_pe) do { \
        HeapFree(GetProcessHeap(), 0, (_pe)); \
    } while (0)

DECLINLINE(PVBOXWINEPROFILE_HASHMAP_ELEMENT) vboxWineProfileHashMapElementGet(PVBOXEXT_HASHMAP pMap, void *pvKey)
{
    PVBOXEXT_HASHMAP_ENTRY pEntry = VBoxExtHashGet(pMap, pvKey);
    if (pEntry)
    {
        return VBOXWINEPROFILE_HASHMAP_ELEMENT_FROMENTRY(pEntry);
    }
    else
    {
        PVBOXWINEPROFILE_HASHMAP_ELEMENT pElement = VBOXWINEPROFILE_HASHMAP_ELEMENT_CREATE();
        Assert(pElement);
        if (pElement)
            VBoxExtHashPut(pMap, pvKey, &pElement->MapEntry);
        return pElement;
    }
}

#define VBOXWINEPROFILE_HASHMAP_ELEMENT_STEP(_pm, _pk, _t) do { \
        PVBOXWINEPROFILE_HASHMAP_ELEMENT pElement = vboxWineProfileHashMapElementGet(_pm, _pk); \
        VBOXWINEPROFILE_ELEMENT_STEP(&pElement->Data, _t); \
    } while (0)

static DECLCALLBACK(bool) vboxWineProfileElementResetCb(struct VBOXEXT_HASHMAP *pMap, void *pvKey, struct VBOXEXT_HASHMAP_ENTRY *pValue, void *pvVisitor)
{
    PVBOXWINEPROFILE_HASHMAP_ELEMENT pElement = VBOXWINEPROFILE_HASHMAP_ELEMENT_FROMENTRY(pValue);
    VBOXWINEPROFILE_ELEMENT_RESET(&pElement->Data);
    return true;
}

static DECLCALLBACK(bool) vboxWineProfileElementDumpCb(struct VBOXEXT_HASHMAP *pMap, void *pvKey, struct VBOXEXT_HASHMAP_ENTRY *pValue, void *pvVisitor)
{
    PVBOXWINEPROFILE_HASHMAP_ELEMENT pElement = VBOXWINEPROFILE_HASHMAP_ELEMENT_FROMENTRY(pValue);
    char *pName = (char*)pvVisitor;
    PRLOG(("%s[%d]:", pName, (uint32_t)pvKey));
    VBOXWINEPROFILE_ELEMENT_DUMP(&pElement->Data, "");
    return true;
}


#define VBOXWINEPROFILE_HASHMAP_RESET(_pm) do { \
        VBoxExtHashVisit((_pm), vboxWineProfileElementResetCb, NULL); \
    } while (0)

#define VBOXWINEPROFILE_HASHMAP_DUMP(_pm, _pn) do { \
        VBoxExtHashVisit((_pm), vboxWineProfileElementDumpCb, (_pn)); \
    } while (0)

static DECLCALLBACK(bool) vboxWineProfileElementCleanupCb(struct VBOXEXT_HASHMAP *pMap, void *pvKey, struct VBOXEXT_HASHMAP_ENTRY *pValue, void *pvVisitor)
{
    PVBOXWINEPROFILE_HASHMAP_ELEMENT pElement = VBOXWINEPROFILE_HASHMAP_ELEMENT_FROMENTRY(pValue);
    VBOXWINEPROFILE_HASHMAP_ELEMENT_TERM(pElement);
    return true;
}

#define VBOXWINEPROFILE_HASHMAP_TERM(_pm) do { \
        VBoxExtHashCleanup((_pm), vboxWineProfileElementCleanupCb, NULL); \
        VBoxExtHashVisit((_pm), vboxWineProfileElementResetCb, NULL); \
    } while (0)

typedef struct VBOXWINEPROFILE_DRAWPRIM
{
    uint64_t u64LoadLocationTime;
    uint64_t u64CtxAcquireTime;
    uint64_t u64PostProcess;
    VBOXEXT_HASHMAP MapDrawPrimSlowVs;
    VBOXEXT_HASHMAP MapDrawPrimSlow;
    VBOXEXT_HASHMAP MapDrawPrimStrided;
    VBOXEXT_HASHMAP MapDrawPrimFast;
    uint32_t cu32Calls;
} VBOXWINEPROFILE_DRAWPRIM, *PVBOXWINEPROFILE_DRAWPRIM;

#define VBOXWINEPROFILE_DRAWPRIM_RESET_NEXT(_p) do { \
        (_p)->u64LoadLocationTime = 0; \
        (_p)->u64CtxAcquireTime = 0; \
        (_p)->u64PostProcess = 0; \
        VBOXWINEPROFILE_HASHMAP_RESET(&(_p)->MapDrawPrimSlowVs); \
        VBOXWINEPROFILE_HASHMAP_RESET(&(_p)->MapDrawPrimSlow); \
        VBOXWINEPROFILE_HASHMAP_RESET(&(_p)->MapDrawPrimStrided); \
        VBOXWINEPROFILE_HASHMAP_RESET(&(_p)->MapDrawPrimFast); \
    } while (0)

static DECLCALLBACK(uint32_t) vboxWineProfileDrawPrimHashMapHash(void *pvKey)
{
    return (uint32_t)pvKey;
}

static DECLCALLBACK(bool) vboxWineProfileDrawPrimHashMapEqual(void *pvKey1, void *pvKey2)
{
    return ((uint32_t)pvKey1) == ((uint32_t)pvKey2);
}

#define VBOXWINEPROFILE_DRAWPRIM_INIT(_p) do { \
        memset((_p), 0, sizeof (*(_p))); \
        VBoxExtHashInit(&(_p)->MapDrawPrimSlowVs, vboxWineProfileDrawPrimHashMapHash, vboxWineProfileDrawPrimHashMapEqual); \
        VBoxExtHashInit(&(_p)->MapDrawPrimSlow, vboxWineProfileDrawPrimHashMapHash, vboxWineProfileDrawPrimHashMapEqual); \
        VBoxExtHashInit(&(_p)->MapDrawPrimStrided, vboxWineProfileDrawPrimHashMapHash, vboxWineProfileDrawPrimHashMapEqual); \
        VBoxExtHashInit(&(_p)->MapDrawPrimFast, vboxWineProfileDrawPrimHashMapHash, vboxWineProfileDrawPrimHashMapEqual); \
    } while (0)

#define VBOXWINEPROFILE_DRAWPRIM_TERM(_p) do { \
        memset((_p), 0, sizeof (*(_p))); \
        VBOXWINEPROFILE_HASHMAP_TERM(&(_p)->MapDrawPrimSlowVs); \
        VBOXWINEPROFILE_HASHMAP_TERM(&(_p)->MapDrawPrimSlow); \
        VBOXWINEPROFILE_HASHMAP_TERM(&(_p)->MapDrawPrimStrided); \
        VBOXWINEPROFILE_HASHMAP_TERM(&(_p)->MapDrawPrimFast); \
    } while (0)
#else
# define PRLOG(_m) do {} while (0)
#endif

#endif /* #ifndef ___VBOXEXT_H__*/
