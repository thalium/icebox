/* $Id: PDMAll.cpp $ */
/** @file
 * PDM Critical Sections
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
#define LOG_GROUP LOG_GROUP_PDM
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/vmm/apic.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>

#include "PDMInline.h"
#include "dtrace/VBoxVMM.h"



/**
 * Gets the pending interrupt.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_APIC_INTR_MASKED_BY_TPR when an APIC interrupt is pending but
 *          can't be delivered due to TPR priority.
 * @retval  VERR_NO_DATA if there is no interrupt to be delivered (either APIC
 *          has been software-disabled since it flagged something was pending,
 *          or other reasons).
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pu8Interrupt    Where to store the interrupt.
 */
VMMDECL(int) PDMGetInterrupt(PVMCPU pVCpu, uint8_t *pu8Interrupt)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    /*
     * The local APIC has a higher priority than the PIC.
     */
    int rc = VERR_NO_DATA;
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC))
    {
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_APIC);
        uint32_t uTagSrc;
        rc = APICGetInterrupt(pVCpu, pu8Interrupt, &uTagSrc);
        if (RT_SUCCESS(rc))
        {
            if (rc == VINF_SUCCESS)
                VBOXVMM_PDM_IRQ_GET(pVCpu, RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc), *pu8Interrupt);
            return rc;
        }
        /* else if it's masked by TPR/PPR/whatever, go ahead checking the PIC. Such masked
           interrupts shouldn't prevent ExtINT from being delivered. */
    }

    pdmLock(pVM);

    /*
     * Check the PIC.
     */
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC))
    {
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_PIC);
        Assert(pVM->pdm.s.Pic.CTX_SUFF(pDevIns));
        Assert(pVM->pdm.s.Pic.CTX_SUFF(pfnGetInterrupt));
        uint32_t uTagSrc;
        int i = pVM->pdm.s.Pic.CTX_SUFF(pfnGetInterrupt)(pVM->pdm.s.Pic.CTX_SUFF(pDevIns), &uTagSrc);
        AssertMsg(i <= 255 && i >= 0, ("i=%d\n", i));
        if (i >= 0)
        {
            pdmUnlock(pVM);
            *pu8Interrupt = (uint8_t)i;
            VBOXVMM_PDM_IRQ_GET(pVCpu, RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc), i);
            return VINF_SUCCESS;
        }
    }

    /*
     * One scenario where we may possibly get here is if the APIC signaled a pending interrupt,
     * got an APIC MMIO/MSR VM-exit which disabled the APIC. We could, in theory, clear the APIC
     * force-flag from all the places which disables the APIC but letting PDMGetInterrupt() fail
     * without returning a valid interrupt still needs to be handled for the TPR masked case,
     * so we shall just handle it here regardless if we choose to update the APIC code in the future.
     */

    pdmUnlock(pVM);
    return rc;
}


/**
 * Sets the pending interrupt coming from ISA source or HPET.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   u8Irq           The IRQ line.
 * @param   u8Level         The new level.
 * @param   uTagSrc         The IRQ tag and source tracer ID.
 */
VMMDECL(int) PDMIsaSetIrq(PVM pVM, uint8_t u8Irq, uint8_t u8Level, uint32_t uTagSrc)
{
    pdmLock(pVM);

    /** @todo put the IRQ13 code elsewhere to avoid this unnecessary bloat. */
    if (!uTagSrc && (u8Level & PDM_IRQ_LEVEL_HIGH)) /* FPU IRQ */
    {
        if (u8Level == PDM_IRQ_LEVEL_HIGH)
            VBOXVMM_PDM_IRQ_HIGH(VMMGetCpu(pVM), 0, 0);
        else
            VBOXVMM_PDM_IRQ_HILO(VMMGetCpu(pVM), 0, 0);
    }

    int rc = VERR_PDM_NO_PIC_INSTANCE;
    if (pVM->pdm.s.Pic.CTX_SUFF(pDevIns))
    {
        Assert(pVM->pdm.s.Pic.CTX_SUFF(pfnSetIrq));
        pVM->pdm.s.Pic.CTX_SUFF(pfnSetIrq)(pVM->pdm.s.Pic.CTX_SUFF(pDevIns), u8Irq, u8Level, uTagSrc);
        rc = VINF_SUCCESS;
    }

    if (pVM->pdm.s.IoApic.CTX_SUFF(pDevIns))
    {
        Assert(pVM->pdm.s.IoApic.CTX_SUFF(pfnSetIrq));

        /*
         * Apply Interrupt Source Override rules.
         * See ACPI 4.0 specification 5.2.12.4 and 5.2.12.5 for details on
         * interrupt source override.
         * Shortly, ISA IRQ0 is electically connected to pin 2 on IO-APIC, and some OSes,
         * notably recent OS X rely upon this configuration.
         * If changing, also update override rules in MADT and MPS.
         */
        /* ISA IRQ0 routed to pin 2, all others ISA sources are identity mapped */
        if (u8Irq == 0)
            u8Irq = 2;

        pVM->pdm.s.IoApic.CTX_SUFF(pfnSetIrq)(pVM->pdm.s.IoApic.CTX_SUFF(pDevIns), u8Irq, u8Level, uTagSrc);
        rc = VINF_SUCCESS;
    }

    if (!uTagSrc && u8Level == PDM_IRQ_LEVEL_LOW)
        VBOXVMM_PDM_IRQ_LOW(VMMGetCpu(pVM), 0, 0);
    pdmUnlock(pVM);
    return rc;
}


/**
 * Sets the pending I/O APIC interrupt.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   u8Irq       The IRQ line.
 * @param   u8Level     The new level.
 * @param   uTagSrc     The IRQ tag and source tracer ID.
 */
VMM_INT_DECL(int) PDMIoApicSetIrq(PVM pVM, uint8_t u8Irq, uint8_t u8Level, uint32_t uTagSrc)
{
    if (pVM->pdm.s.IoApic.CTX_SUFF(pDevIns))
    {
        Assert(pVM->pdm.s.IoApic.CTX_SUFF(pfnSetIrq));
        pVM->pdm.s.IoApic.CTX_SUFF(pfnSetIrq)(pVM->pdm.s.IoApic.CTX_SUFF(pDevIns), u8Irq, u8Level, uTagSrc);
        return VINF_SUCCESS;
    }
    return VERR_PDM_NO_PIC_INSTANCE;
}


/**
 * Broadcasts an EOI to the I/O APICs.
 *
 * @return VBox status code (incl. scheduling status codes).
 * @param   pVM             The cross context VM structure.
 * @param   uVector         The interrupt vector corresponding to the EOI.
 */
VMM_INT_DECL(int) PDMIoApicBroadcastEoi(PVM pVM, uint8_t uVector)
{
    /* At present, we support only a maximum of one I/O APIC per-VM. If we ever implement having
       multiple I/O APICs per-VM, we'll have to broadcast this EOI to all of the I/O APICs. */
    if (pVM->pdm.s.IoApic.CTX_SUFF(pDevIns))
    {
        Assert(pVM->pdm.s.IoApic.CTX_SUFF(pfnSetEoi));
        return pVM->pdm.s.IoApic.CTX_SUFF(pfnSetEoi)(pVM->pdm.s.IoApic.CTX_SUFF(pDevIns), uVector);
    }

    /* We shouldn't return failure if no I/O APIC is present. */
    return VINF_SUCCESS;
}


/**
 * Send a MSI to an I/O APIC.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   GCAddr      Request address.
 * @param   uValue      Request value.
 * @param   uTagSrc     The IRQ tag and source tracer ID.
 */
VMM_INT_DECL(int) PDMIoApicSendMsi(PVM pVM, RTGCPHYS GCAddr, uint32_t uValue, uint32_t uTagSrc)
{
    if (pVM->pdm.s.IoApic.CTX_SUFF(pDevIns))
    {
        Assert(pVM->pdm.s.IoApic.CTX_SUFF(pfnSendMsi));
        pVM->pdm.s.IoApic.CTX_SUFF(pfnSendMsi)(pVM->pdm.s.IoApic.CTX_SUFF(pDevIns), GCAddr, uValue, uTagSrc);
        return VINF_SUCCESS;
    }
    return VERR_PDM_NO_PIC_INSTANCE;
}



/**
 * Returns the presence of an IO-APIC.
 *
 * @returns true if an IO-APIC is present.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(bool) PDMHasIoApic(PVM pVM)
{
    return pVM->pdm.s.IoApic.CTX_SUFF(pDevIns) != NULL;
}


/**
 * Returns the presence of an APIC.
 *
 * @returns true if an APIC is present.
 * @param   pVM         The cross context VM structure.
 */
VMM_INT_DECL(bool) PDMHasApic(PVM pVM)
{
    return pVM->pdm.s.Apic.CTX_SUFF(pDevIns) != NULL;
}


/**
 * Locks PDM.
 * This might call back to Ring-3 in order to deal with lock contention in GC and R3.
 *
 * @param   pVM     The cross context VM structure.
 */
void pdmLock(PVM pVM)
{
#ifdef IN_RING3
    int rc = PDMCritSectEnter(&pVM->pdm.s.CritSect, VERR_IGNORED);
#else
    int rc = PDMCritSectEnter(&pVM->pdm.s.CritSect, VERR_GENERAL_FAILURE);
    if (rc == VERR_GENERAL_FAILURE)
        rc = VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_PDM_LOCK, 0);
#endif
    AssertRC(rc);
}


/**
 * Locks PDM but don't go to ring-3 if it's owned by someone.
 *
 * @returns VINF_SUCCESS on success.
 * @returns rc if we're in GC or R0 and can't get the lock.
 * @param   pVM     The cross context VM structure.
 * @param   rc      The RC to return in GC or R0 when we can't get the lock.
 */
int pdmLockEx(PVM pVM, int rc)
{
    return PDMCritSectEnter(&pVM->pdm.s.CritSect, rc);
}


/**
 * Unlocks PDM.
 *
 * @param   pVM     The cross context VM structure.
 */
void pdmUnlock(PVM pVM)
{
    PDMCritSectLeave(&pVM->pdm.s.CritSect);
}


/**
 * Converts ring 3 VMM heap pointer to a guest physical address
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pv              Ring-3 pointer.
 * @param   pGCPhys         GC phys address (out).
 */
VMM_INT_DECL(int) PDMVmmDevHeapR3ToGCPhys(PVM pVM, RTR3PTR pv, RTGCPHYS *pGCPhys)
{
    if (RT_LIKELY(pVM->pdm.s.GCPhysVMMDevHeap != NIL_RTGCPHYS))
    {
        RTR3UINTPTR const offHeap = (RTR3UINTPTR)pv - (RTR3UINTPTR)pVM->pdm.s.pvVMMDevHeap;
        if (RT_LIKELY(offHeap < pVM->pdm.s.cbVMMDevHeap))
        {
            *pGCPhys = pVM->pdm.s.GCPhysVMMDevHeap + offHeap;
            return VINF_SUCCESS;
        }

        /* Don't assert here as this is called before we can catch ring-0 assertions. */
        Log(("PDMVmmDevHeapR3ToGCPhys: pv=%p pvVMMDevHeap=%p cbVMMDevHeap=%#x\n",
             pv, pVM->pdm.s.pvVMMDevHeap, pVM->pdm.s.cbVMMDevHeap));
    }
    else
        Log(("PDMVmmDevHeapR3ToGCPhys: GCPhysVMMDevHeap=%RGp (pv=%p)\n", pVM->pdm.s.GCPhysVMMDevHeap, pv));
    return VERR_PDM_DEV_HEAP_R3_TO_GCPHYS;
}


/**
 * Checks if the vmm device heap is enabled (== vmm device's pci region mapped)
 *
 * @returns dev heap enabled status (true/false)
 * @param   pVM             The cross context VM structure.
 */
VMM_INT_DECL(bool) PDMVmmDevHeapIsEnabled(PVM pVM)
{
    return pVM->pdm.s.GCPhysVMMDevHeap != NIL_RTGCPHYS;
}
