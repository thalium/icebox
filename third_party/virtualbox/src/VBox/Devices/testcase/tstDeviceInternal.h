/** @file
 * tstDevice: Shared definitions between the framework and the shim library.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___tstDeviceInternal_h
#define ___tstDeviceInternal_h

#include <VBox/types.h>
#include <iprt/assert.h>
#include <iprt/list.h>
#include <iprt/semaphore.h>

#include "tstDevicePlugin.h"
#include "tstDeviceVMMInternal.h"

RT_C_DECLS_BEGIN


/** Converts PDM device instance to the device under test structure. */
#define TSTDEV_PDMDEVINS_2_DUT(a_pDevIns) ((a_pDevIns)->Internal.s.pDut)

/**
 * PDM module descriptor type.
 */
typedef enum TSTDEVPDMMODTYPE
{
    /** Invalid module type. */
    TSTDEVPDMMODTYPE_INVALID = 0,
    /** Ring 3 module. */
    TSTDEVPDMMODTYPE_R3,
    /** Ring 0 module. */
    TSTDEVPDMMODTYPE_R0,
    /** Raw context module. */
    TSTDEVPDMMODTYPE_RC,
    /** 32bit hack. */
    TSTDEVPDMMODTYPE_32BIT_HACK = 0x7fffffff
} TSTDEVPDMMODTYPE;

/**
 * Registered I/O port access handler.
 */
typedef struct RTDEVDUTIOPORT
{
    /** Node for the list of registered handlers. */
    RTLISTNODE                      NdIoPorts;
    /** Start I/O port the handler is for. */
    RTIOPORT                        PortStart;
    /** Number of ports handled. */
    RTIOPORT                        cPorts;
    /** Opaque user data - R3. */
    void                            *pvUserR3;
    /** Out handler - R3. */
    PFNIOMIOPORTOUT                 pfnOutR3;
    /** In handler - R3. */
    PFNIOMIOPORTIN                  pfnInR3;
    /** Out string handler - R3. */
    PFNIOMIOPORTOUTSTRING           pfnOutStrR3;
    /** In string handler - R3. */
    PFNIOMIOPORTINSTRING            pfnInStrR3;

    /** Opaque user data - R0. */
    void                            *pvUserR0;
    /** Out handler - R0. */
    PFNIOMIOPORTOUT                 pfnOutR0;
    /** In handler - R0. */
    PFNIOMIOPORTIN                  pfnInR0;
    /** Out string handler - R0. */
    PFNIOMIOPORTOUTSTRING           pfnOutStrR0;
    /** In string handler - R0. */
    PFNIOMIOPORTINSTRING            pfnInStrR0;

#ifdef TSTDEV_SUPPORTS_RC
    /** Opaque user data - RC. */
    void                            *pvUserRC;
    /** Out handler - RC. */
    PFNIOMIOPORTOUT                 pfnOutRC;
    /** In handler - RC. */
    PFNIOMIOPORTIN                  pfnInRC;
    /** Out string handler - RC. */
    PFNIOMIOPORTOUTSTRING           pfnOutStrRC;
    /** In string handler - RC. */
    PFNIOMIOPORTINSTRING            pfnInStrRC;
#endif
} RTDEVDUTIOPORT;
/** Pointer to a registered I/O port handler. */
typedef RTDEVDUTIOPORT *PRTDEVDUTIOPORT;
/** Pointer to a const I/O port handler. */
typedef const RTDEVDUTIOPORT *PCRTDEVDUTIOPORT;

/**
 * The Support Driver session state.
 */
typedef struct TSTDEVSUPDRVSESSION
{
    /** Pointer to the owning device under test instance. */
    PTSTDEVDUTINT                   pDut;
    /** List of event semaphores. */
    RTLISTANCHOR                    LstSupSem;
} TSTDEVSUPDRVSESSION;
/** Pointer to the Support Driver session state. */
typedef TSTDEVSUPDRVSESSION *PTSTDEVSUPDRVSESSION;

/** Converts a Support Driver session handle to the internal state. */
#define TSTDEV_PSUPDRVSESSION_2_PTSTDEVSUPDRVSESSION(a_pSession) ((PTSTDEVSUPDRVSESSION)(a_pSession))
/** Converts the internal session state to a Support Driver session handle. */
#define TSTDEV_PTSTDEVSUPDRVSESSION_2_PSUPDRVSESSION(a_pSession) ((PSUPDRVSESSION)(a_pSession))

/**
 * Support driver event semaphore.
 */
typedef struct TSTDEVSUPSEMEVENT
{
    /** Node for the event semaphore list. */
    RTLISTNODE                      NdSupSem;
    /** Flag whether this is multi event semaphore. */
    bool                            fMulti;
    /** Event smeaphore handles depending on the flag above. */
    union
    {
        RTSEMEVENT                  hSemEvt;
        RTSEMEVENTMULTI             hSemEvtMulti;
    } u;
} TSTDEVSUPSEMEVENT;
/** Pointer to a support event semaphore state. */
typedef TSTDEVSUPSEMEVENT *PTSTDEVSUPSEMEVENT;

/** Converts a Support event semaphore handle to the internal state. */
#define TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(a_pSupSemEvt) ((PTSTDEVSUPSEMEVENT)(a_pSupSemEvt))
/** Converts the internal session state to a Support event semaphore handle. */
#define TSTDEV_PTSTDEVSUPSEMEVENT_2_SUPSEMEVENT(a_pSupSemEvt) ((SUPSEMEVENT)(a_pSupSemEvt))

/**
 * The contex the device under test is currently in.
 */
typedef enum TSTDEVDUTCTX
{
    /** Invalid context. */
    TSTDEVDUTCTX_INVALID = 0,
    /** R3 context. */
    TSTDEVDUTCTX_R3,
    /** R0 context. */
    TSTDEVDUTCTX_R0,
    /** RC context. */
    TSTDEVDUTCTX_RC,
    /** 32bit hack. */
    TSTDEVDUTCTX_32BIT_HACK = 0x7fffffff
} TSTDEVDUTCTX;

/**
 * PCI region descriptor.
 */
typedef struct TSTDEVDUTPCIREGION
{
    /** Size of the region. */
    RTGCPHYS                        cbRegion;
    /** Address space type. */
    PCIADDRESSSPACE                 enmType;
    /** Region mapping callback. */
    PFNPCIIOREGIONMAP               pfnRegionMap;
} TSTDEVDUTPCIREGION;
/** Pointer to a PCI region descriptor. */
typedef TSTDEVDUTPCIREGION *PTSTDEVDUTPCIREGION;
/** Pointer to a const PCI region descriptor. */
typedef const TSTDEVDUTPCIREGION *PCTSTDEVDUTPCIREGION;

/**
 * Device under test instance data.
 */
typedef struct TSTDEVDUTINT
{
    /** Pointer to the testcase this device is part of. */
    PCTSTDEVTESTCASEREG             pTestcaseReg;
    /** Pointer to the PDM device instance. */
    PPDMDEVINS                      pDevIns;
    /** Current device context. */
    TSTDEVDUTCTX                    enmCtx;
    /** Critical section protecting the lists below. */
    RTCRITSECTRW                    CritSectLists;
    /** List of registered I/O port handlers. */
    RTLISTANCHOR                    LstIoPorts;
    /** List of timers registered. */
    RTLISTANCHOR                    LstTimers;
    /** List of registered MMIO regions. */
    RTLISTANCHOR                    LstMmio;
    /** List of MM Heap allocations. */
    RTLISTANCHOR                    LstMmHeap;
    /** List of PDM threads. */
    RTLISTANCHOR                    LstPdmThreads;
    /** The SUP session we emulate. */
    TSTDEVSUPDRVSESSION             SupSession;
    /** The VM state assoicated with this device. */
    PVM                             pVm;
    /** The registered PCI device instance if this is a PCI device. */
    PPDMPCIDEV                      pPciDev;
    /** PCI Region descriptors. */
    TSTDEVDUTPCIREGION              aPciRegions[VBOX_PCI_NUM_REGIONS];
} TSTDEVDUTINT;


DECLHIDDEN(int) tstDevPdmLdrGetSymbol(PTSTDEVDUTINT pThis, const char *pszMod, TSTDEVPDMMODTYPE enmModType,
                                      const char *pszSymbol, PFNRT *ppfn);


DECLINLINE(int) tstDevDutLockShared(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwEnterShared(&pThis->CritSectLists);
}

DECLINLINE(int) tstDevDutUnlockShared(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwLeaveShared(&pThis->CritSectLists);
}

DECLINLINE(int) tstDevDutLockExcl(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwEnterExcl(&pThis->CritSectLists);
}

DECLINLINE(int) tstDevDutUnlockExcl(PTSTDEVDUTINT pThis)
{
    return RTCritSectRwLeaveExcl(&pThis->CritSectLists);
}

RT_C_DECLS_END

#endif
