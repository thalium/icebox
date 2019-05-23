/** @file
 * PDM - Pluggable Device Manager, Critical Sections.
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

#ifndef ___VBox_vmm_pdmcritsect_h
#define ___VBox_vmm_pdmcritsect_h

#include <VBox/types.h>
#include <iprt/critsect.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_critsect      The PDM Critical Section API
 * @ingroup grp_pdm
 * @{
 */

/**
 * A PDM critical section.
 * Initialize using PDMDRVHLP::pfnCritSectInit().
 */
typedef union PDMCRITSECT
{
    /** Padding. */
    uint8_t padding[HC_ARCH_BITS == 32 ? 0x80 : 0xc0];
#ifdef PDMCRITSECTINT_DECLARED
    /** The internal structure (not normally visible). */
    struct PDMCRITSECTINT s;
#endif
} PDMCRITSECT;

VMMR3_INT_DECL(int)     PDMR3CritSectBothTerm(PVM pVM);
VMMR3_INT_DECL(void)    PDMR3CritSectLeaveAll(PVM pVM);
VMM_INT_DECL(void)      PDMCritSectBothFF(PVMCPU pVCpu);


VMMR3DECL(uint32_t) PDMR3CritSectCountOwned(PVM pVM, char *pszNames, size_t cbNames);

VMMR3DECL(int)      PDMR3CritSectInit(PVM pVM, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL,
                                      const char *pszNameFmt, ...) RT_IPRT_FORMAT_ATTR(6, 7);
VMMR3DECL(int)      PDMR3CritSectEnterEx(PPDMCRITSECT pCritSect, bool fCallRing3);
VMMR3DECL(bool)     PDMR3CritSectYield(PPDMCRITSECT pCritSect);
VMMR3DECL(const char *) PDMR3CritSectName(PCPDMCRITSECT pCritSect);
VMMR3DECL(int)      PDMR3CritSectDelete(PPDMCRITSECT pCritSect);
#if defined(IN_RING0) || defined(IN_RING3)
VMMDECL(int)        PDMHCCritSectScheduleExitEvent(PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal);
#endif

VMMDECL(int)        PDMCritSectEnter(PPDMCRITSECT pCritSect, int rcBusy);
VMMDECL(int)        PDMCritSectEnterDebug(PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL);
VMMDECL(int)        PDMCritSectTryEnter(PPDMCRITSECT pCritSect);
VMMDECL(int)        PDMCritSectTryEnterDebug(PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL);
VMMDECL(int)        PDMCritSectLeave(PPDMCRITSECT pCritSect);

VMMDECL(bool)       PDMCritSectIsOwner(PCPDMCRITSECT pCritSect);
VMMDECL(bool)       PDMCritSectIsOwnerEx(PCPDMCRITSECT pCritSect, PVMCPU pVCpu);
VMMDECL(bool)       PDMCritSectIsInitialized(PCPDMCRITSECT pCritSect);
VMMDECL(bool)       PDMCritSectHasWaiters(PCPDMCRITSECT pCritSect);
VMMDECL(uint32_t)   PDMCritSectGetRecursion(PCPDMCRITSECT pCritSect);

VMMR3DECL(PPDMCRITSECT)             PDMR3CritSectGetNop(PVM pVM);
VMMR3DECL(R0PTRTYPE(PPDMCRITSECT))  PDMR3CritSectGetNopR0(PVM pVM);
VMMR3DECL(RCPTRTYPE(PPDMCRITSECT))  PDMR3CritSectGetNopRC(PVM pVM);

/* Strict build: Remap the two enter calls to the debug versions. */
#ifdef VBOX_STRICT
# ifdef ___iprt_asm_h
#  define PDMCritSectEnter(pCritSect, rcBusy)   PDMCritSectEnterDebug((pCritSect), (rcBusy), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
#  define PDMCritSectTryEnter(pCritSect)        PDMCritSectTryEnterDebug((pCritSect), (uintptr_t)ASMReturnAddress(), RT_SRC_POS)
# else
#  define PDMCritSectEnter(pCritSect, rcBusy)   PDMCritSectEnterDebug((pCritSect), (rcBusy), 0, RT_SRC_POS)
#  define PDMCritSectTryEnter(pCritSect)        PDMCritSectTryEnterDebug((pCritSect), 0, RT_SRC_POS)
# endif
#endif

/** @} */

RT_C_DECLS_END

#endif

