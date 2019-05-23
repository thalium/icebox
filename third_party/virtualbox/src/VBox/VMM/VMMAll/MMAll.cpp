/* $Id: MMAll.cpp $ */
/** @file
 * MM - Memory Manager - Any Context.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MM_HYPER
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vmm.h>
#include "MMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/hm.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/string.h>



/**
 * Lookup a host context ring-3 address.
 *
 * @returns Pointer to the corresponding lookup record.
 * @returns NULL on failure.
 * @param   pVM     The cross context VM structure.
 * @param   R3Ptr   The host context ring-3 address to lookup.
 * @param   poff    Where to store the offset into the HMA memory chunk.
 */
DECLINLINE(PMMLOOKUPHYPER) mmHyperLookupR3(PVM pVM, RTR3PTR R3Ptr, uint32_t *poff)
{
    /** @todo cache last lookup, this stuff ain't cheap! */
    PMMLOOKUPHYPER  pLookup = (PMMLOOKUPHYPER)((uint8_t *)pVM->mm.s.CTX_SUFF(pHyperHeap) + pVM->mm.s.offLookupHyper);
    for (;;)
    {
        switch (pLookup->enmType)
        {
            case MMLOOKUPHYPERTYPE_LOCKED:
            {
                const RTR3UINTPTR off = (RTR3UINTPTR)R3Ptr - (RTR3UINTPTR)pLookup->u.Locked.pvR3;
                if (off < pLookup->cb)
                {
                    *poff = off;
                    return pLookup;
                }
                break;
            }

            case MMLOOKUPHYPERTYPE_HCPHYS:
            {
                const RTR3UINTPTR off = (RTR3UINTPTR)R3Ptr - (RTR3UINTPTR)pLookup->u.HCPhys.pvR3;
                if (off < pLookup->cb)
                {
                    *poff = off;
                    return pLookup;
                }
                break;
            }

            case MMLOOKUPHYPERTYPE_GCPHYS:  /* (for now we'll not allow these kind of conversions) */
            case MMLOOKUPHYPERTYPE_MMIO2:
            case MMLOOKUPHYPERTYPE_DYNAMIC:
                break;

            default:
                AssertMsgFailed(("enmType=%d\n", pLookup->enmType));
                break;
        }

        /* next */
        if (pLookup->offNext ==  (int32_t)NIL_OFFSET)
            break;
        pLookup = (PMMLOOKUPHYPER)((uint8_t *)pLookup + pLookup->offNext);
    }

    AssertMsgFailed(("R3Ptr=%RHv is not inside the hypervisor memory area!\n", R3Ptr));
    return NULL;
}


/**
 * Lookup a host context ring-0 address.
 *
 * @returns Pointer to the corresponding lookup record.
 * @returns NULL on failure.
 * @param   pVM     The cross context VM structure.
 * @param   R0Ptr   The host context ring-0 address to lookup.
 * @param   poff    Where to store the offset into the HMA memory chunk.
 */
DECLINLINE(PMMLOOKUPHYPER) mmHyperLookupR0(PVM pVM, RTR0PTR R0Ptr, uint32_t *poff)
{
    AssertCompile(sizeof(RTR0PTR) == sizeof(RTR3PTR));

    /** @todo cache last lookup, this stuff ain't cheap! */
    PMMLOOKUPHYPER  pLookup = (PMMLOOKUPHYPER)((uint8_t *)pVM->mm.s.CTX_SUFF(pHyperHeap) + pVM->mm.s.offLookupHyper);
    for (;;)
    {
        switch (pLookup->enmType)
        {
            case MMLOOKUPHYPERTYPE_LOCKED:
            {
                const RTR0UINTPTR off = (RTR0UINTPTR)R0Ptr - (RTR0UINTPTR)pLookup->u.Locked.pvR0;
                if (off < pLookup->cb && pLookup->u.Locked.pvR0)
                {
                    *poff = off;
                    return pLookup;
                }
                break;
            }

            case MMLOOKUPHYPERTYPE_HCPHYS:
            {
                const RTR0UINTPTR off = (RTR0UINTPTR)R0Ptr - (RTR0UINTPTR)pLookup->u.HCPhys.pvR0;
                if (off < pLookup->cb && pLookup->u.HCPhys.pvR0)
                {
                    *poff = off;
                    return pLookup;
                }
                break;
            }

            case MMLOOKUPHYPERTYPE_GCPHYS:  /* (for now we'll not allow these kind of conversions) */
            case MMLOOKUPHYPERTYPE_MMIO2:
            case MMLOOKUPHYPERTYPE_DYNAMIC:
                break;

            default:
                AssertMsgFailed(("enmType=%d\n", pLookup->enmType));
                break;
        }

        /* next */
        if (pLookup->offNext ==  (int32_t)NIL_OFFSET)
            break;
        pLookup = (PMMLOOKUPHYPER)((uint8_t *)pLookup + pLookup->offNext);
    }

    AssertMsgFailed(("R0Ptr=%RHv is not inside the hypervisor memory area!\n", R0Ptr));
    return NULL;
}


/**
 * Lookup a raw-mode context address.
 *
 * @returns Pointer to the corresponding lookup record.
 * @returns NULL on failure.
 * @param   pVM     The cross context VM structure.
 * @param   RCPtr   The raw-mode context address to lookup.
 * @param   poff    Where to store the offset into the HMA memory chunk.
 */
DECLINLINE(PMMLOOKUPHYPER) mmHyperLookupRC(PVM pVM, RTRCPTR RCPtr, uint32_t *poff)
{
    /** @todo cache last lookup this stuff ain't cheap! */
    unsigned        offRC = (RTRCUINTPTR)RCPtr - (RTGCUINTPTR)pVM->mm.s.pvHyperAreaGC;
    PMMLOOKUPHYPER  pLookup = (PMMLOOKUPHYPER)((uint8_t *)pVM->mm.s.CTX_SUFF(pHyperHeap) + pVM->mm.s.offLookupHyper);
    for (;;)
    {
        const uint32_t off = offRC - pLookup->off;
        if (off < pLookup->cb)
        {
            switch (pLookup->enmType)
            {
                case MMLOOKUPHYPERTYPE_LOCKED:
                case MMLOOKUPHYPERTYPE_HCPHYS:
                    *poff = off;
                    return pLookup;
                default:
                    break;
            }
            AssertMsgFailed(("enmType=%d\n", pLookup->enmType));
            *poff = 0; /* shut up gcc */
            return NULL;
        }

        /* next */
        if (pLookup->offNext == (int32_t)NIL_OFFSET)
            break;
        pLookup = (PMMLOOKUPHYPER)((uint8_t *)pLookup + pLookup->offNext);
    }

    AssertMsgFailed(("RCPtr=%RRv is not inside the hypervisor memory area!\n", RCPtr));
    *poff = 0; /* shut up gcc */
    return NULL;
}


/**
 * Lookup a current context address.
 *
 * @returns Pointer to the corresponding lookup record.
 * @returns NULL on failure.
 * @param   pVM     The cross context VM structure.
 * @param   pv      The current context address to lookup.
 * @param   poff    Where to store the offset into the HMA memory chunk.
 */
DECLINLINE(PMMLOOKUPHYPER) mmHyperLookupCC(PVM pVM, void *pv, uint32_t *poff)
{
#ifdef IN_RC
    return mmHyperLookupRC(pVM, (RTRCPTR)pv, poff);
#elif defined(IN_RING0)
    return mmHyperLookupR0(pVM, pv, poff);
#else
    return mmHyperLookupR3(pVM, pv, poff);
#endif
}


/**
 * Calculate the host context ring-3 address of an offset into the HMA memory chunk.
 *
 * @returns the host context ring-3 address.
 * @param   pLookup     The HMA lookup record.
 * @param   off         The offset into the HMA memory chunk.
 */
DECLINLINE(RTR3PTR) mmHyperLookupCalcR3(PMMLOOKUPHYPER pLookup, uint32_t off)
{
    switch (pLookup->enmType)
    {
        case MMLOOKUPHYPERTYPE_LOCKED:
            return (RTR3PTR)((RTR3UINTPTR)pLookup->u.Locked.pvR3 + off);
        case MMLOOKUPHYPERTYPE_HCPHYS:
            return (RTR3PTR)((RTR3UINTPTR)pLookup->u.HCPhys.pvR3 + off);
        default:
            AssertMsgFailed(("enmType=%d\n", pLookup->enmType));
            return NIL_RTR3PTR;
    }
}


/**
 * Calculate the host context ring-0 address of an offset into the HMA memory chunk.
 *
 * @returns the host context ring-0 address.
 * @param   pVM         The cross context VM structure.
 * @param   pLookup     The HMA lookup record.
 * @param   off         The offset into the HMA memory chunk.
 */
DECLINLINE(RTR0PTR) mmHyperLookupCalcR0(PVM pVM, PMMLOOKUPHYPER pLookup, uint32_t off)
{
    switch (pLookup->enmType)
    {
        case MMLOOKUPHYPERTYPE_LOCKED:
            if (pLookup->u.Locked.pvR0)
                return (RTR0PTR)((RTR0UINTPTR)pLookup->u.Locked.pvR0 + off);
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
            AssertMsg(!HMIsEnabled(pVM), ("%s\n", R3STRING(pLookup->pszDesc)));
#else
            AssertMsgFailed(("%s\n", R3STRING(pLookup->pszDesc))); NOREF(pVM);
#endif
            return NIL_RTR0PTR;

        case MMLOOKUPHYPERTYPE_HCPHYS:
            if (pLookup->u.HCPhys.pvR0)
                return (RTR0PTR)((RTR0UINTPTR)pLookup->u.HCPhys.pvR0 + off);
            AssertMsgFailed(("%s\n", R3STRING(pLookup->pszDesc)));
            return NIL_RTR0PTR;

        default:
            AssertMsgFailed(("enmType=%d\n", pLookup->enmType));
            return NIL_RTR0PTR;
    }
}


/**
 * Calculate the raw-mode context address of an offset into the HMA memory chunk.
 *
 * @returns the raw-mode context base address.
 * @param   pVM         The cross context VM structure.
 * @param   pLookup     The HMA lookup record.
 * @param   off         The offset into the HMA memory chunk.
 */
DECLINLINE(RTRCPTR) mmHyperLookupCalcRC(PVM pVM, PMMLOOKUPHYPER pLookup, uint32_t off)
{
    return (RTRCPTR)((RTRCUINTPTR)pVM->mm.s.pvHyperAreaGC + pLookup->off + off);
}


/**
 * Calculate the guest context address of an offset into the HMA memory chunk.
 *
 * @returns the guest context base address.
 * @param   pVM         The cross context VM structure.
 * @param   pLookup     The HMA lookup record.
 * @param   off         The offset into the HMA memory chunk.
 */
DECLINLINE(void *) mmHyperLookupCalcCC(PVM pVM, PMMLOOKUPHYPER pLookup, uint32_t off)
{
#ifdef IN_RC
    return (void *)mmHyperLookupCalcRC(pVM, pLookup, off);
#elif defined(IN_RING0)
    return mmHyperLookupCalcR0(pVM, pLookup, off);
#else
    NOREF(pVM);
    return mmHyperLookupCalcR3(pLookup, off);
#endif
}


/**
 * Converts a ring-0 host context address in the Hypervisor memory region to a ring-3 host context address.
 *
 * @returns ring-3 host context address.
 * @param   pVM         The cross context VM structure.
 * @param   R0Ptr       The ring-0 host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTR3PTR) MMHyperR0ToR3(PVM pVM, RTR0PTR R0Ptr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupR0(pVM, R0Ptr, &off);
    if (pLookup)
        return mmHyperLookupCalcR3(pLookup, off);
    return NIL_RTR3PTR;
}


/**
 * Converts a ring-0 host context address in the Hypervisor memory region to a raw-mode context address.
 *
 * @returns raw-mode context address.
 * @param   pVM         The cross context VM structure.
 * @param   R0Ptr       The ring-0 host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTRCPTR) MMHyperR0ToRC(PVM pVM, RTR0PTR R0Ptr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupR0(pVM, R0Ptr, &off);
    if (pLookup)
        return mmHyperLookupCalcRC(pVM, pLookup, off);
    return NIL_RTRCPTR;
}


#ifndef IN_RING0
/**
 * Converts a ring-0 host context address in the Hypervisor memory region to a current context address.
 *
 * @returns current context address.
 * @param   pVM         The cross context VM structure.
 * @param   R0Ptr       The ring-0 host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(void *) MMHyperR0ToCC(PVM pVM, RTR0PTR R0Ptr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupR0(pVM, R0Ptr, &off);
    if (pLookup)
        return mmHyperLookupCalcCC(pVM, pLookup, off);
    return NULL;
}
#endif


/**
 * Converts a ring-3 host context address in the Hypervisor memory region to a ring-0 host context address.
 *
 * @returns ring-0 host context address.
 * @param   pVM         The cross context VM structure.
 * @param   R3Ptr       The ring-3 host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTR0PTR) MMHyperR3ToR0(PVM pVM, RTR3PTR R3Ptr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupR3(pVM, R3Ptr, &off);
    if (pLookup)
        return mmHyperLookupCalcR0(pVM, pLookup, off);
    AssertMsgFailed(("R3Ptr=%p is not inside the hypervisor memory area!\n", R3Ptr));
    return NIL_RTR0PTR;
}


/**
 * Converts a ring-3 host context address in the Hypervisor memory region to a guest context address.
 *
 * @returns guest context address.
 * @param   pVM         The cross context VM structure.
 * @param   R3Ptr       The ring-3 host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTRCPTR) MMHyperR3ToRC(PVM pVM, RTR3PTR R3Ptr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupR3(pVM, R3Ptr, &off);
    if (pLookup)
        return mmHyperLookupCalcRC(pVM, pLookup, off);
    AssertMsgFailed(("R3Ptr=%p is not inside the hypervisor memory area!\n", R3Ptr));
    return NIL_RTRCPTR;
}


#ifndef IN_RING3
/**
 * Converts a ring-3 host context address in the Hypervisor memory region to a current context address.
 *
 * @returns current context address.
 * @param   pVM         The cross context VM structure.
 * @param   R3Ptr       The ring-3 host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(void *) MMHyperR3ToCC(PVM pVM, RTR3PTR R3Ptr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupR3(pVM, R3Ptr, &off);
    if (pLookup)
        return mmHyperLookupCalcCC(pVM, pLookup, off);
    return NULL;
}
#endif


/**
 * Converts a raw-mode context address in the Hypervisor memory region to a ring-3 context address.
 *
 * @returns ring-3 host context address.
 * @param   pVM         The cross context VM structure.
 * @param   RCPtr       The raw-mode context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTR3PTR) MMHyperRCToR3(PVM pVM, RTRCPTR RCPtr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupRC(pVM, RCPtr, &off);
    if (pLookup)
        return mmHyperLookupCalcR3(pLookup, off);
    return NIL_RTR3PTR;
}


/**
 * Converts a raw-mode context address in the Hypervisor memory region to a ring-0 host context address.
 *
 * @returns ring-0 host context address.
 * @param   pVM         The cross context VM structure.
 * @param   RCPtr       The raw-mode context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTR0PTR) MMHyperRCToR0(PVM pVM, RTRCPTR RCPtr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupRC(pVM, RCPtr, &off);
    if (pLookup)
        return mmHyperLookupCalcR0(pVM, pLookup, off);
    return NIL_RTR0PTR;
}

#ifndef IN_RC
/**
 * Converts a raw-mode context address in the Hypervisor memory region to a current context address.
 *
 * @returns current context address.
 * @param   pVM         The cross context VM structure.
 * @param   RCPtr       The raw-mode host context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(void *) MMHyperRCToCC(PVM pVM, RTRCPTR RCPtr)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupRC(pVM, RCPtr, &off);
    if (pLookup)
        return mmHyperLookupCalcCC(pVM, pLookup, off);
    return NULL;
}
#endif

#ifndef IN_RING3
/**
 * Converts a current context address in the Hypervisor memory region to a ring-3 host context address.
 *
 * @returns ring-3 host context address.
 * @param   pVM         The cross context VM structure.
 * @param   pv          The current context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTR3PTR) MMHyperCCToR3(PVM pVM, void *pv)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupCC(pVM, pv, &off);
    if (pLookup)
        return mmHyperLookupCalcR3(pLookup, off);
    return NIL_RTR3PTR;
}
#endif

#ifndef IN_RING0
/**
 * Converts a current context address in the Hypervisor memory region to a ring-0 host context address.
 *
 * @returns ring-0 host context address.
 * @param   pVM         The cross context VM structure.
 * @param   pv          The current context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTR0PTR) MMHyperCCToR0(PVM pVM, void *pv)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupCC(pVM, pv, &off);
    if (pLookup)
        return mmHyperLookupCalcR0(pVM, pLookup, off);
    return NIL_RTR0PTR;
}
#endif


#ifndef IN_RC
/**
 * Converts a current context address in the Hypervisor memory region to a raw-mode context address.
 *
 * @returns guest context address.
 * @param   pVM         The cross context VM structure.
 * @param   pv          The current context address.
 *                      You'll be damned if this is not in the HMA! :-)
 * @thread  The Emulation Thread.
 */
VMMDECL(RTRCPTR) MMHyperCCToRC(PVM pVM, void *pv)
{
    uint32_t off;
    PMMLOOKUPHYPER pLookup = mmHyperLookupCC(pVM, pv, &off);
    if (pLookup)
        return mmHyperLookupCalcRC(pVM, pLookup, off);
    return NIL_RTRCPTR;
}
#endif


/**
 * Gets the string name of a memory tag.
 *
 * @returns name of enmTag.
 * @param   enmTag      The tag.
 */
const char *mmGetTagName(MMTAG enmTag)
{
    switch (enmTag)
    {
        #define TAG2STR(tag) case MM_TAG_##tag: return #tag

        TAG2STR(CFGM);
        TAG2STR(CFGM_BYTES);
        TAG2STR(CFGM_STRING);
        TAG2STR(CFGM_USER);

        TAG2STR(CPUM_CTX);
        TAG2STR(CPUM_CPUID);
        TAG2STR(CPUM_MSRS);

        TAG2STR(CSAM);
        TAG2STR(CSAM_PATCH);

        TAG2STR(DBGF);
        TAG2STR(DBGF_AS);
        TAG2STR(DBGF_INFO);
        TAG2STR(DBGF_LINE);
        TAG2STR(DBGF_LINE_DUP);
        TAG2STR(DBGF_MODULE);
        TAG2STR(DBGF_OS);
        TAG2STR(DBGF_REG);
        TAG2STR(DBGF_STACK);
        TAG2STR(DBGF_SYMBOL);
        TAG2STR(DBGF_SYMBOL_DUP);
        TAG2STR(DBGF_TYPE);

        TAG2STR(EM);

        TAG2STR(IEM);

        TAG2STR(IOM);
        TAG2STR(IOM_STATS);

        TAG2STR(MM);
        TAG2STR(MM_LOOKUP_GUEST);
        TAG2STR(MM_LOOKUP_PHYS);
        TAG2STR(MM_LOOKUP_VIRT);
        TAG2STR(MM_PAGE);

        TAG2STR(PARAV);

        TAG2STR(PATM);
        TAG2STR(PATM_PATCH);

        TAG2STR(PDM);
        TAG2STR(PDM_DEVICE);
        TAG2STR(PDM_DEVICE_DESC);
        TAG2STR(PDM_DEVICE_USER);
        TAG2STR(PDM_DRIVER);
        TAG2STR(PDM_DRIVER_DESC);
        TAG2STR(PDM_DRIVER_USER);
        TAG2STR(PDM_USB);
        TAG2STR(PDM_USB_DESC);
        TAG2STR(PDM_USB_USER);
        TAG2STR(PDM_LUN);
        TAG2STR(PDM_QUEUE);
        TAG2STR(PDM_THREAD);
        TAG2STR(PDM_ASYNC_COMPLETION);
#ifdef VBOX_WITH_NETSHAPER
        TAG2STR(PDM_NET_SHAPER);
#endif /* VBOX_WITH_NETSHAPER */

        TAG2STR(PGM);
        TAG2STR(PGM_CHUNK_MAPPING);
        TAG2STR(PGM_HANDLERS);
        TAG2STR(PGM_HANDLER_TYPES);
        TAG2STR(PGM_MAPPINGS);
        TAG2STR(PGM_PHYS);
        TAG2STR(PGM_POOL);

        TAG2STR(REM);

        TAG2STR(SELM);

        TAG2STR(SSM);

        TAG2STR(STAM);

        TAG2STR(TM);

        TAG2STR(TRPM);

        TAG2STR(VM);
        TAG2STR(VM_REQ);

        TAG2STR(VMM);

        TAG2STR(HM);

        #undef TAG2STR

        default:
        {
            AssertMsgFailed(("Unknown tag %d! forgot to add it to the switch?\n", enmTag));
#ifdef IN_RING3
            static char sz[48];
            RTStrPrintf(sz, sizeof(sz), "%d", enmTag);
            return sz;
#else
            return "unknown tag!";
#endif
        }
    }
}

