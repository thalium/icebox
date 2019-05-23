/** @file
 * PDM - Pluggable Device Manager, Queues.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_vmm_pdmqueue_h
#define ___VBox_vmm_pdmqueue_h

#include <VBox/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_queue     The PDM Queues API
 * @ingroup grp_pdm
 * @{
 */

/** Pointer to a PDM queue. Also called PDM queue handle. */
typedef struct PDMQUEUE *PPDMQUEUE;

/** Pointer to a PDM queue item core. */
typedef struct PDMQUEUEITEMCORE *PPDMQUEUEITEMCORE;

/**
 * PDM queue item core.
 */
typedef struct PDMQUEUEITEMCORE
{
    /** Pointer to the next item in the pending list - R3 Pointer. */
    R3PTRTYPE(PPDMQUEUEITEMCORE)    pNextR3;
    /** Pointer to the next item in the pending list - R0 Pointer. */
    R0PTRTYPE(PPDMQUEUEITEMCORE)    pNextR0;
    /** Pointer to the next item in the pending list - RC Pointer. */
    RCPTRTYPE(PPDMQUEUEITEMCORE)    pNextRC;
#if HC_ARCH_BITS == 64
    RTRCPTR                         Alignment0;
#endif
} PDMQUEUEITEMCORE;


/**
 * Queue consumer callback for devices.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDevIns     The device instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks The device critical section will NOT be entered before calling the
 *          callback.  No locks will be held, but for now it's safe to assume
 *          that only one EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACK(bool) FNPDMQUEUEDEV(PPDMDEVINS pDevIns, PPDMQUEUEITEMCORE pItem);
/** Pointer to a FNPDMQUEUEDEV(). */
typedef FNPDMQUEUEDEV *PFNPDMQUEUEDEV;

/**
 * Queue consumer callback for USB devices.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDevIns     The USB device instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACK(bool) FNPDMQUEUEUSB(PPDMUSBINS pUsbIns, PPDMQUEUEITEMCORE pItem);
/** Pointer to a FNPDMQUEUEUSB(). */
typedef FNPDMQUEUEUSB *PFNPDMQUEUEUSB;

/**
 * Queue consumer callback for drivers.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDrvIns     The driver instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACK(bool) FNPDMQUEUEDRV(PPDMDRVINS pDrvIns, PPDMQUEUEITEMCORE pItem);
/** Pointer to a FNPDMQUEUEDRV(). */
typedef FNPDMQUEUEDRV *PFNPDMQUEUEDRV;

/**
 * Queue consumer callback for internal component.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pVM         The cross context VM structure.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACK(bool) FNPDMQUEUEINT(PVM pVM, PPDMQUEUEITEMCORE pItem);
/** Pointer to a FNPDMQUEUEINT(). */
typedef FNPDMQUEUEINT *PFNPDMQUEUEINT;

/**
 * Queue consumer callback for external component.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pvUser      User argument.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 * @remarks No locks will be held, but for now it's safe to assume that only one
 *          EMT will do queue callbacks at any one time.
 */
typedef DECLCALLBACK(bool) FNPDMQUEUEEXT(void *pvUser, PPDMQUEUEITEMCORE pItem);
/** Pointer to a FNPDMQUEUEEXT(). */
typedef FNPDMQUEUEEXT *PFNPDMQUEUEEXT;

#ifdef VBOX_IN_VMM
VMMR3_INT_DECL(int)  PDMR3QueueCreateDevice(PVM pVM, PPDMDEVINS pDevIns, size_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                            PFNPDMQUEUEDEV pfnCallback, bool fRZEnabled, const char *pszName, PPDMQUEUE *ppQueue);
VMMR3_INT_DECL(int)  PDMR3QueueCreateDriver(PVM pVM, PPDMDRVINS pDrvIns, size_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                            PFNPDMQUEUEDRV pfnCallback, const char *pszName, PPDMQUEUE *ppQueue);
VMMR3_INT_DECL(int)  PDMR3QueueCreateInternal(PVM pVM, size_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                              PFNPDMQUEUEINT pfnCallback, bool fGCEnabled, const char *pszName, PPDMQUEUE *ppQueue);
VMMR3_INT_DECL(int)  PDMR3QueueCreateExternal(PVM pVM, size_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                              PFNPDMQUEUEEXT pfnCallback, void *pvUser, const char *pszName, PPDMQUEUE *ppQueue);
VMMR3_INT_DECL(int)  PDMR3QueueDestroy(PPDMQUEUE pQueue);
VMMR3_INT_DECL(int)  PDMR3QueueDestroyDevice(PVM pVM, PPDMDEVINS pDevIns);
VMMR3_INT_DECL(int)  PDMR3QueueDestroyDriver(PVM pVM, PPDMDRVINS pDrvIns);
VMMR3_INT_DECL(void) PDMR3QueueFlushAll(PVM pVM);
#endif /* VBOX_IN_VMM */

VMMDECL(PPDMQUEUEITEMCORE)    PDMQueueAlloc(PPDMQUEUE pQueue);
VMMDECL(void)                 PDMQueueInsert(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem);
VMMDECL(void)                 PDMQueueInsertEx(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem, uint64_t NanoMaxDelay);
VMMDECL(RCPTRTYPE(PPDMQUEUE)) PDMQueueRCPtr(PPDMQUEUE pQueue);
VMMDECL(R0PTRTYPE(PPDMQUEUE)) PDMQueueR0Ptr(PPDMQUEUE pQueue);
VMMDECL(bool)                 PDMQueueFlushIfNecessary(PPDMQUEUE pQueue);

/** @} */

RT_C_DECLS_END

#endif

