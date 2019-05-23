/* $Id: PDMR0Device.cpp $ */
/** @file
 * PDM - Pluggable Device and Driver Manager, R0 Device parts.
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
#define LOG_GROUP LOG_GROUP_PDM_DEVICE
#define PDMPCIDEV_INCLUDE_PRIVATE  /* Hack to get pdmpcidevint.h included at the right point. */
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/apic.h>

#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/vmm/gvmm.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/string.h>

#include "dtrace/VBoxVMM.h"
#include "PDMInline.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
extern DECLEXPORT(const PDMDEVHLPR0)    g_pdmR0DevHlp;
extern DECLEXPORT(const PDMPICHLPR0)    g_pdmR0PicHlp;
extern DECLEXPORT(const PDMIOAPICHLPR0) g_pdmR0IoApicHlp;
extern DECLEXPORT(const PDMPCIHLPR0)    g_pdmR0PciHlp;
extern DECLEXPORT(const PDMHPETHLPR0)   g_pdmR0HpetHlp;
extern DECLEXPORT(const PDMPCIRAWHLPR0) g_pdmR0PciRawHlp;
extern DECLEXPORT(const PDMDRVHLPR0)    g_pdmR0DrvHlp;
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static bool pdmR0IsaSetIrq(PVM pVM, int iIrq, int iLevel, uint32_t uTagSrc);



/** @name Ring-0 Device Helpers
 * @{
 */

/** @interface_method_impl{PDMDEVHLPR0,pfnPCIPhysRead} */
static DECLCALLBACK(int) pdmR0DevHlp_PCIPhysRead(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, RTGCPHYS GCPhys,
                                                 void *pvBuf, size_t cbRead)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    if (!pPciDev) /* NULL is an alias for the default PCI device. */
        pPciDev = pDevIns->Internal.s.pHeadPciDevR0;
    AssertReturn(pPciDev, VERR_PDM_NOT_PCI_DEVICE);

#ifndef PDM_DO_NOT_RESPECT_PCI_BM_BIT
    /*
     * Just check the busmaster setting here and forward the request to the generic read helper.
     */
    if (PCIDevIsBusmaster(pPciDev))
    { /* likely */ }
    else
    {
        Log(("pdmRCDevHlp_PCIPhysRead: caller=%p/%d: returns %Rrc - Not bus master! GCPhys=%RGp cbRead=%#zx\n",
             pDevIns, pDevIns->iInstance, VERR_PDM_NOT_PCI_BUS_MASTER, GCPhys, cbRead));
        memset(pvBuf, 0xff, cbRead);
        return VERR_PDM_NOT_PCI_BUS_MASTER;
    }
#endif

    return pDevIns->pHlpR0->pfnPhysRead(pDevIns, GCPhys, pvBuf, cbRead);
}


/** @interface_method_impl{PDMDEVHLPR0,pfnPCIPhysWrite} */
static DECLCALLBACK(int) pdmR0DevHlp_PCIPhysWrite(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, RTGCPHYS GCPhys,
                                                  const void *pvBuf, size_t cbWrite)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    if (!pPciDev) /* NULL is an alias for the default PCI device. */
        pPciDev = pDevIns->Internal.s.pHeadPciDevR0;
    AssertReturn(pPciDev, VERR_PDM_NOT_PCI_DEVICE);

#ifndef PDM_DO_NOT_RESPECT_PCI_BM_BIT
    /*
     * Just check the busmaster setting here and forward the request to the generic read helper.
     */
    if (PCIDevIsBusmaster(pPciDev))
    { /* likely */ }
    else
    {
        Log(("pdmRCDevHlp_PCIPhysWrite: caller=%p/%d: returns %Rrc - Not bus master! GCPhys=%RGp cbWrite=%#zx\n",
             pDevIns, pDevIns->iInstance, VERR_PDM_NOT_PCI_BUS_MASTER, GCPhys, cbWrite));
        return VERR_PDM_NOT_PCI_BUS_MASTER;
    }
#endif

    return pDevIns->pHlpR0->pfnPhysWrite(pDevIns, GCPhys, pvBuf, cbWrite);
}


/** @interface_method_impl{PDMDEVHLPR0,pfnPCISetIrq} */
static DECLCALLBACK(void) pdmR0DevHlp_PCISetIrq(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    if (!pPciDev) /* NULL is an alias for the default PCI device. */
        pPciDev = pDevIns->Internal.s.pHeadPciDevR0;
    AssertReturnVoid(pPciDev);
    LogFlow(("pdmR0DevHlp_PCISetIrq: caller=%p/%d: pPciDev=%p:{%#x} iIrq=%d iLevel=%d\n",
             pDevIns, pDevIns->iInstance, pPciDev, pPciDev->uDevFn, iIrq, iLevel));
    PVM          pVM     = pDevIns->Internal.s.pVMR0;
    PPDMPCIBUS   pPciBus = pPciDev->Int.s.pPdmBusR0;

    pdmLock(pVM);
    uint32_t uTagSrc;
    if (iLevel & PDM_IRQ_LEVEL_HIGH)
    {
        pDevIns->Internal.s.uLastIrqTag = uTagSrc = pdmCalcIrqTag(pVM, pDevIns->idTracing);
        if (iLevel == PDM_IRQ_LEVEL_HIGH)
            VBOXVMM_PDM_IRQ_HIGH(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));
        else
            VBOXVMM_PDM_IRQ_HILO(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));
    }
    else
        uTagSrc = pDevIns->Internal.s.uLastIrqTag;

    if (    pPciBus
        &&  pPciBus->pDevInsR0)
    {
        pPciBus->pfnSetIrqR0(pPciBus->pDevInsR0, pPciDev, iIrq, iLevel, uTagSrc);

        pdmUnlock(pVM);

        if (iLevel == PDM_IRQ_LEVEL_LOW)
            VBOXVMM_PDM_IRQ_LOW(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));
    }
    else
    {
        pdmUnlock(pVM);

        /* queue for ring-3 execution. */
        PPDMDEVHLPTASK pTask = (PPDMDEVHLPTASK)PDMQueueAlloc(pVM->pdm.s.pDevHlpQueueR0);
        AssertReturnVoid(pTask);

        pTask->enmOp = PDMDEVHLPTASKOP_PCI_SET_IRQ;
        pTask->pDevInsR3 = PDMDEVINS_2_R3PTR(pDevIns);
        pTask->u.PciSetIRQ.iIrq = iIrq;
        pTask->u.PciSetIRQ.iLevel = iLevel;
        pTask->u.PciSetIRQ.uTagSrc = uTagSrc;
        pTask->u.PciSetIRQ.pPciDevR3 = MMHyperR0ToR3(pVM, pPciDev);

        PDMQueueInsertEx(pVM->pdm.s.pDevHlpQueueR0, &pTask->Core, 0);
    }

    LogFlow(("pdmR0DevHlp_PCISetIrq: caller=%p/%d: returns void; uTagSrc=%#x\n", pDevIns, pDevIns->iInstance, uTagSrc));
}


/** @interface_method_impl{PDMDEVHLPR0,pfnISASetIrq} */
static DECLCALLBACK(void) pdmR0DevHlp_ISASetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_ISASetIrq: caller=%p/%d: iIrq=%d iLevel=%d\n", pDevIns, pDevIns->iInstance, iIrq, iLevel));
    PVM pVM = pDevIns->Internal.s.pVMR0;

    pdmLock(pVM);
    uint32_t uTagSrc;
    if (iLevel & PDM_IRQ_LEVEL_HIGH)
    {
        pDevIns->Internal.s.uLastIrqTag = uTagSrc = pdmCalcIrqTag(pVM, pDevIns->idTracing);
        if (iLevel == PDM_IRQ_LEVEL_HIGH)
            VBOXVMM_PDM_IRQ_HIGH(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));
        else
            VBOXVMM_PDM_IRQ_HILO(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));
    }
    else
        uTagSrc = pDevIns->Internal.s.uLastIrqTag;

    bool fRc = pdmR0IsaSetIrq(pVM, iIrq, iLevel, uTagSrc);

    if (iLevel == PDM_IRQ_LEVEL_LOW && fRc)
        VBOXVMM_PDM_IRQ_LOW(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));
    pdmUnlock(pVM);
    LogFlow(("pdmR0DevHlp_ISASetIrq: caller=%p/%d: returns void; uTagSrc=%#x\n", pDevIns, pDevIns->iInstance, uTagSrc));
}


/** @interface_method_impl{PDMDEVHLPR0,pfnIoApicSendMsi} */
static DECLCALLBACK(void) pdmR0DevHlp_IoApicSendMsi(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint32_t uValue)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_IoApicSendMsi: caller=%p/%d: GCPhys=%RGp uValue=%#x\n", pDevIns, pDevIns->iInstance, GCPhys, uValue));
    PVM pVM = pDevIns->Internal.s.pVMR0;

    uint32_t uTagSrc;
    pDevIns->Internal.s.uLastIrqTag = uTagSrc = pdmCalcIrqTag(pVM, pDevIns->idTracing);
    VBOXVMM_PDM_IRQ_HILO(VMMGetCpu(pVM), RT_LOWORD(uTagSrc), RT_HIWORD(uTagSrc));

    if (pVM->pdm.s.IoApic.pDevInsR0)
        pVM->pdm.s.IoApic.pfnSendMsiR0(pVM->pdm.s.IoApic.pDevInsR0, GCPhys, uValue, uTagSrc);
    else
        AssertFatalMsgFailed(("Lazy bastards!"));

    LogFlow(("pdmR0DevHlp_IoApicSendMsi: caller=%p/%d: returns void; uTagSrc=%#x\n", pDevIns, pDevIns->iInstance, uTagSrc));
}


/** @interface_method_impl{PDMDEVHLPR0,pfnPhysRead} */
static DECLCALLBACK(int) pdmR0DevHlp_PhysRead(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, void *pvBuf, size_t cbRead)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_PhysRead: caller=%p/%d: GCPhys=%RGp pvBuf=%p cbRead=%#x\n",
             pDevIns, pDevIns->iInstance, GCPhys, pvBuf, cbRead));

    VBOXSTRICTRC rcStrict = PGMPhysRead(pDevIns->Internal.s.pVMR0, GCPhys, pvBuf, cbRead, PGMACCESSORIGIN_DEVICE);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); /** @todo track down the users for this bugger. */

    Log(("pdmR0DevHlp_PhysRead: caller=%p/%d: returns %Rrc\n", pDevIns, pDevIns->iInstance, VBOXSTRICTRC_VAL(rcStrict) ));
    return VBOXSTRICTRC_VAL(rcStrict);
}


/** @interface_method_impl{PDMDEVHLPR0,pfnPhysWrite} */
static DECLCALLBACK(int) pdmR0DevHlp_PhysWrite(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, const void *pvBuf, size_t cbWrite)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_PhysWrite: caller=%p/%d: GCPhys=%RGp pvBuf=%p cbWrite=%#x\n",
             pDevIns, pDevIns->iInstance, GCPhys, pvBuf, cbWrite));

    VBOXSTRICTRC rcStrict = PGMPhysWrite(pDevIns->Internal.s.pVMR0, GCPhys, pvBuf, cbWrite, PGMACCESSORIGIN_DEVICE);
    AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict))); /** @todo track down the users for this bugger. */

    Log(("pdmR0DevHlp_PhysWrite: caller=%p/%d: returns %Rrc\n", pDevIns, pDevIns->iInstance, VBOXSTRICTRC_VAL(rcStrict) ));
    return VBOXSTRICTRC_VAL(rcStrict);
}


/** @interface_method_impl{PDMDEVHLPR0,pfnA20IsEnabled} */
static DECLCALLBACK(bool) pdmR0DevHlp_A20IsEnabled(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_A20IsEnabled: caller=%p/%d:\n", pDevIns, pDevIns->iInstance));

    bool fEnabled = PGMPhysIsA20Enabled(VMMGetCpu(pDevIns->Internal.s.pVMR0));

    Log(("pdmR0DevHlp_A20IsEnabled: caller=%p/%d: returns %RTbool\n", pDevIns, pDevIns->iInstance, fEnabled));
    return fEnabled;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnVMState} */
static DECLCALLBACK(VMSTATE) pdmR0DevHlp_VMState(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    VMSTATE enmVMState = pDevIns->Internal.s.pVMR0->enmVMState;

    LogFlow(("pdmR0DevHlp_VMState: caller=%p/%d: returns %d\n", pDevIns, pDevIns->iInstance, enmVMState));
    return enmVMState;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnVMSetError} */
static DECLCALLBACK(int) pdmR0DevHlp_VMSetError(PPDMDEVINS pDevIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, ...)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    va_list args;
    va_start(args, pszFormat);
    int rc2 = VMSetErrorV(pDevIns->Internal.s.pVMR0, rc, RT_SRC_POS_ARGS, pszFormat, args); Assert(rc2 == rc); NOREF(rc2);
    va_end(args);
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnVMSetErrorV} */
static DECLCALLBACK(int) pdmR0DevHlp_VMSetErrorV(PPDMDEVINS pDevIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    int rc2 = VMSetErrorV(pDevIns->Internal.s.pVMR0, rc, RT_SRC_POS_ARGS, pszFormat, va); Assert(rc2 == rc); NOREF(rc2);
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnVMSetRuntimeError} */
static DECLCALLBACK(int) pdmR0DevHlp_VMSetRuntimeError(PPDMDEVINS pDevIns, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, ...)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    va_list va;
    va_start(va, pszFormat);
    int rc = VMSetRuntimeErrorV(pDevIns->Internal.s.pVMR0, fFlags, pszErrorId, pszFormat, va);
    va_end(va);
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnVMSetRuntimeErrorV} */
static DECLCALLBACK(int) pdmR0DevHlp_VMSetRuntimeErrorV(PPDMDEVINS pDevIns, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, va_list va)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    int rc = VMSetRuntimeErrorV(pDevIns->Internal.s.pVMR0, fFlags, pszErrorId, pszFormat, va);
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnPATMSetMMIOPatchInfo} */
static DECLCALLBACK(int) pdmR0DevHlp_PATMSetMMIOPatchInfo(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, RTGCPTR pCachedData)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_PATMSetMMIOPatchInfo: caller=%p/%d:\n", pDevIns, pDevIns->iInstance));

    AssertFailed();
    NOREF(GCPhys); NOREF(pCachedData); NOREF(pDevIns);

/*    return PATMSetMMIOPatchInfo(pDevIns->Internal.s.pVMR0, GCPhys, pCachedData); */
    return VINF_SUCCESS;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnGetVM} */
static DECLCALLBACK(PVM)  pdmR0DevHlp_GetVM(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_GetVM: caller='%p'/%d\n", pDevIns, pDevIns->iInstance));
    return pDevIns->Internal.s.pVMR0;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnGetVMCPU} */
static DECLCALLBACK(PVMCPU) pdmR0DevHlp_GetVMCPU(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_GetVMCPU: caller='%p'/%d\n", pDevIns, pDevIns->iInstance));
    return VMMGetCpu(pDevIns->Internal.s.pVMR0);
}


/** @interface_method_impl{PDMDEVHLPRC,pfnGetCurrentCpuId} */
static DECLCALLBACK(VMCPUID) pdmR0DevHlp_GetCurrentCpuId(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VMCPUID idCpu = VMMGetCpuId(pDevIns->Internal.s.pVMR0);
    LogFlow(("pdmR0DevHlp_GetCurrentCpuId: caller='%p'/%d for CPU %u\n", pDevIns, pDevIns->iInstance, idCpu));
    return idCpu;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnTMTimeVirtGet} */
static DECLCALLBACK(uint64_t) pdmR0DevHlp_TMTimeVirtGet(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_TMTimeVirtGet: caller='%p'/%d\n", pDevIns, pDevIns->iInstance));
    return TMVirtualGet(pDevIns->Internal.s.pVMR0);
}


/** @interface_method_impl{PDMDEVHLPR0,pfnTMTimeVirtGetFreq} */
static DECLCALLBACK(uint64_t) pdmR0DevHlp_TMTimeVirtGetFreq(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_TMTimeVirtGetFreq: caller='%p'/%d\n", pDevIns, pDevIns->iInstance));
    return TMVirtualGetFreq(pDevIns->Internal.s.pVMR0);
}


/** @interface_method_impl{PDMDEVHLPR0,pfnTMTimeVirtGetNano} */
static DECLCALLBACK(uint64_t) pdmR0DevHlp_TMTimeVirtGetNano(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_TMTimeVirtGetNano: caller='%p'/%d\n", pDevIns, pDevIns->iInstance));
    return TMVirtualToNano(pDevIns->Internal.s.pVMR0, TMVirtualGet(pDevIns->Internal.s.pVMR0));
}


/** @interface_method_impl{PDMDEVHLPR0,pfnDBGFTraceBuf} */
static DECLCALLBACK(RTTRACEBUF) pdmR0DevHlp_DBGFTraceBuf(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RTTRACEBUF hTraceBuf = pDevIns->Internal.s.pVMR0->hTraceBufR0;
    LogFlow(("pdmR3DevHlp_DBGFTraceBuf: caller='%p'/%d: returns %p\n", pDevIns, pDevIns->iInstance, hTraceBuf));
    return hTraceBuf;
}


/** @interface_method_impl{PDMDEVHLPR0,pfnCanEmulateIoBlock} */
static DECLCALLBACK(bool) pdmR0DevHlp_CanEmulateIoBlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR0DevHlp_GetVM: caller='%p'/%d\n", pDevIns, pDevIns->iInstance));
    return HMCanEmulateIoBlock(VMMGetCpu(pDevIns->Internal.s.pVMR0));
}


/**
 * The Ring-0 Device Helper Callbacks.
 */
extern DECLEXPORT(const PDMDEVHLPR0) g_pdmR0DevHlp =
{
    PDM_DEVHLPR0_VERSION,
    pdmR0DevHlp_PCIPhysRead,
    pdmR0DevHlp_PCIPhysWrite,
    pdmR0DevHlp_PCISetIrq,
    pdmR0DevHlp_ISASetIrq,
    pdmR0DevHlp_IoApicSendMsi,
    pdmR0DevHlp_PhysRead,
    pdmR0DevHlp_PhysWrite,
    pdmR0DevHlp_A20IsEnabled,
    pdmR0DevHlp_VMState,
    pdmR0DevHlp_VMSetError,
    pdmR0DevHlp_VMSetErrorV,
    pdmR0DevHlp_VMSetRuntimeError,
    pdmR0DevHlp_VMSetRuntimeErrorV,
    pdmR0DevHlp_PATMSetMMIOPatchInfo,
    pdmR0DevHlp_GetVM,
    pdmR0DevHlp_CanEmulateIoBlock,
    pdmR0DevHlp_GetVMCPU,
    pdmR0DevHlp_GetCurrentCpuId,
    pdmR0DevHlp_TMTimeVirtGet,
    pdmR0DevHlp_TMTimeVirtGetFreq,
    pdmR0DevHlp_TMTimeVirtGetNano,
    pdmR0DevHlp_DBGFTraceBuf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PDM_DEVHLPR0_VERSION
};

/** @} */




/** @name PIC Ring-0 Helpers
 * @{
 */

/** @interface_method_impl{PDMPICHLPR0,pfnSetInterruptFF} */
static DECLCALLBACK(void) pdmR0PicHlp_SetInterruptFF(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM    pVM   = pDevIns->Internal.s.pVMR0;
    PVMCPU pVCpu = &pVM->aCpus[0];      /* for PIC we always deliver to CPU 0, MP use APIC */
    /** @todo r=ramshankar: Propagating rcRZ and make all callers handle it? */
    APICLocalInterrupt(pVCpu, 0 /* u8Pin */, 1 /* u8Level */, VINF_SUCCESS /* rcRZ */);
}


/** @interface_method_impl{PDMPICHLPR0,pfnClearInterruptFF} */
static DECLCALLBACK(void) pdmR0PicHlp_ClearInterruptFF(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM    pVM   = pDevIns->Internal.s.pVMR0;
    PVMCPU pVCpu = &pVM->aCpus[0];      /* for PIC we always deliver to CPU 0, MP use APIC */
    /** @todo r=ramshankar: Propagating rcRZ and make all callers handle it? */
    APICLocalInterrupt(pVCpu, 0 /* u8Pin */, 0 /* u8Level */, VINF_SUCCESS /* rcRZ */);
}


/** @interface_method_impl{PDMPICHLPR0,pfnLock} */
static DECLCALLBACK(int) pdmR0PicHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    return pdmLockEx(pDevIns->Internal.s.pVMR0, rc);
}


/** @interface_method_impl{PDMPICHLPR0,pfnUnlock} */
static DECLCALLBACK(void) pdmR0PicHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    pdmUnlock(pDevIns->Internal.s.pVMR0);
}


/**
 * The Ring-0 PIC Helper Callbacks.
 */
extern DECLEXPORT(const PDMPICHLPR0) g_pdmR0PicHlp =
{
    PDM_PICHLPR0_VERSION,
    pdmR0PicHlp_SetInterruptFF,
    pdmR0PicHlp_ClearInterruptFF,
    pdmR0PicHlp_Lock,
    pdmR0PicHlp_Unlock,
    PDM_PICHLPR0_VERSION
};

/** @} */


/** @name I/O APIC Ring-0 Helpers
 * @{
 */

/** @interface_method_impl{PDMIOAPICHLPR0,pfnApicBusDeliver} */
static DECLCALLBACK(int) pdmR0IoApicHlp_ApicBusDeliver(PPDMDEVINS pDevIns, uint8_t u8Dest, uint8_t u8DestMode,
                                                       uint8_t u8DeliveryMode, uint8_t uVector, uint8_t u8Polarity,
                                                       uint8_t u8TriggerMode, uint32_t uTagSrc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR0;
    LogFlow(("pdmR0IoApicHlp_ApicBusDeliver: caller=%p/%d: u8Dest=%RX8 u8DestMode=%RX8 u8DeliveryMode=%RX8 uVector=%RX8 u8Polarity=%RX8 u8TriggerMode=%RX8 uTagSrc=%#x\n",
             pDevIns, pDevIns->iInstance, u8Dest, u8DestMode, u8DeliveryMode, uVector, u8Polarity, u8TriggerMode, uTagSrc));
    return APICBusDeliver(pVM, u8Dest, u8DestMode, u8DeliveryMode, uVector, u8Polarity, u8TriggerMode, uTagSrc);
}


/** @interface_method_impl{PDMIOAPICHLPR0,pfnLock} */
static DECLCALLBACK(int) pdmR0IoApicHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    return pdmLockEx(pDevIns->Internal.s.pVMR0, rc);
}


/** @interface_method_impl{PDMIOAPICHLPR0,pfnUnlock} */
static DECLCALLBACK(void) pdmR0IoApicHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    pdmUnlock(pDevIns->Internal.s.pVMR0);
}


/**
 * The Ring-0 I/O APIC Helper Callbacks.
 */
extern DECLEXPORT(const PDMIOAPICHLPR0) g_pdmR0IoApicHlp =
{
    PDM_IOAPICHLPR0_VERSION,
    pdmR0IoApicHlp_ApicBusDeliver,
    pdmR0IoApicHlp_Lock,
    pdmR0IoApicHlp_Unlock,
    PDM_IOAPICHLPR0_VERSION
};

/** @} */




/** @name PCI Bus Ring-0 Helpers
 * @{
 */

/** @interface_method_impl{PDMPCIHLPR0,pfnIsaSetIrq} */
static DECLCALLBACK(void) pdmR0PciHlp_IsaSetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel, uint32_t uTagSrc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    Log4(("pdmR0PciHlp_IsaSetIrq: iIrq=%d iLevel=%d uTagSrc=%#x\n", iIrq, iLevel, uTagSrc));
    PVM pVM = pDevIns->Internal.s.pVMR0;

    pdmLock(pVM);
    pdmR0IsaSetIrq(pVM, iIrq, iLevel, uTagSrc);
    pdmUnlock(pVM);
}


/** @interface_method_impl{PDMPCIHLPR0,pfnIoApicSetIrq} */
static DECLCALLBACK(void) pdmR0PciHlp_IoApicSetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel, uint32_t uTagSrc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    Log4(("pdmR0PciHlp_IoApicSetIrq: iIrq=%d iLevel=%d uTagSrc=%#x\n", iIrq, iLevel, uTagSrc));
    PVM pVM = pDevIns->Internal.s.pVMR0;

    if (pVM->pdm.s.IoApic.pDevInsR0)
        pVM->pdm.s.IoApic.pfnSetIrqR0(pVM->pdm.s.IoApic.pDevInsR0, iIrq, iLevel, uTagSrc);
    else if (pVM->pdm.s.IoApic.pDevInsR3)
    {
        /* queue for ring-3 execution. */
        PPDMDEVHLPTASK pTask = (PPDMDEVHLPTASK)PDMQueueAlloc(pVM->pdm.s.pDevHlpQueueR0);
        if (pTask)
        {
            pTask->enmOp = PDMDEVHLPTASKOP_IOAPIC_SET_IRQ;
            pTask->pDevInsR3 = NIL_RTR3PTR; /* not required */
            pTask->u.IoApicSetIRQ.iIrq = iIrq;
            pTask->u.IoApicSetIRQ.iLevel = iLevel;
            pTask->u.IoApicSetIRQ.uTagSrc = uTagSrc;

            PDMQueueInsertEx(pVM->pdm.s.pDevHlpQueueR0, &pTask->Core, 0);
        }
        else
            AssertMsgFailed(("We're out of devhlp queue items!!!\n"));
    }
}


/** @interface_method_impl{PDMPCIHLPR0,pfnIoApicSendMsi} */
static DECLCALLBACK(void) pdmR0PciHlp_IoApicSendMsi(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint32_t uValue, uint32_t uTagSrc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    Log4(("pdmR0PciHlp_IoApicSendMsi: GCPhys=%p uValue=%d uTagSrc=%#x\n", GCPhys, uValue, uTagSrc));
    PVM pVM = pDevIns->Internal.s.pVMR0;
    if (pVM->pdm.s.IoApic.pDevInsR0)
        pVM->pdm.s.IoApic.pfnSendMsiR0(pVM->pdm.s.IoApic.pDevInsR0, GCPhys, uValue, uTagSrc);
    else
        AssertFatalMsgFailed(("Lazy bastards!"));
}


/** @interface_method_impl{PDMPCIHLPR0,pfnLock} */
static DECLCALLBACK(int) pdmR0PciHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    return pdmLockEx(pDevIns->Internal.s.pVMR0, rc);
}


/** @interface_method_impl{PDMPCIHLPR0,pfnUnlock} */
static DECLCALLBACK(void) pdmR0PciHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    pdmUnlock(pDevIns->Internal.s.pVMR0);
}


/**
 * The Ring-0 PCI Bus Helper Callbacks.
 */
extern DECLEXPORT(const PDMPCIHLPR0) g_pdmR0PciHlp =
{
    PDM_PCIHLPR0_VERSION,
    pdmR0PciHlp_IsaSetIrq,
    pdmR0PciHlp_IoApicSetIrq,
    pdmR0PciHlp_IoApicSendMsi,
    pdmR0PciHlp_Lock,
    pdmR0PciHlp_Unlock,
    PDM_PCIHLPR0_VERSION, /* the end */
};

/** @} */




/** @name HPET Ring-0 Helpers
 * @{
 */
/* none */

/**
 * The Ring-0 HPET Helper Callbacks.
 */
extern DECLEXPORT(const PDMHPETHLPR0) g_pdmR0HpetHlp =
{
    PDM_HPETHLPR0_VERSION,
    PDM_HPETHLPR0_VERSION, /* the end */
};

/** @} */


/** @name Raw PCI Ring-0 Helpers
 * @{
 */
/* none */

/**
 * The Ring-0 PCI raw Helper Callbacks.
 */
extern DECLEXPORT(const PDMPCIRAWHLPR0) g_pdmR0PciRawHlp =
{
    PDM_PCIRAWHLPR0_VERSION,
    PDM_PCIRAWHLPR0_VERSION, /* the end */
};

/** @} */


/** @name Ring-0 Context Driver Helpers
 * @{
 */

/** @interface_method_impl{PDMDRVHLPR0,pfnVMSetError} */
static DECLCALLBACK(int) pdmR0DrvHlp_VMSetError(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, ...)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    va_list args;
    va_start(args, pszFormat);
    int rc2 = VMSetErrorV(pDrvIns->Internal.s.pVMR0, rc, RT_SRC_POS_ARGS, pszFormat, args); Assert(rc2 == rc); NOREF(rc2);
    va_end(args);
    return rc;
}


/** @interface_method_impl{PDMDRVHLPR0,pfnVMSetErrorV} */
static DECLCALLBACK(int) pdmR0DrvHlp_VMSetErrorV(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    int rc2 = VMSetErrorV(pDrvIns->Internal.s.pVMR0, rc, RT_SRC_POS_ARGS, pszFormat, va); Assert(rc2 == rc); NOREF(rc2);
    return rc;
}


/** @interface_method_impl{PDMDRVHLPR0,pfnVMSetRuntimeError} */
static DECLCALLBACK(int) pdmR0DrvHlp_VMSetRuntimeError(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                       const char *pszFormat, ...)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    va_list va;
    va_start(va, pszFormat);
    int rc = VMSetRuntimeErrorV(pDrvIns->Internal.s.pVMR0, fFlags, pszErrorId, pszFormat, va);
    va_end(va);
    return rc;
}


/** @interface_method_impl{PDMDRVHLPR0,pfnVMSetRuntimeErrorV} */
static DECLCALLBACK(int) pdmR0DrvHlp_VMSetRuntimeErrorV(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId,
                                                        const char *pszFormat, va_list va)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    int rc = VMSetRuntimeErrorV(pDrvIns->Internal.s.pVMR0, fFlags, pszErrorId, pszFormat, va);
    return rc;
}


/** @interface_method_impl{PDMDRVHLPR0,pfnAssertEMT} */
static DECLCALLBACK(bool) pdmR0DrvHlp_AssertEMT(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    if (VM_IS_EMT(pDrvIns->Internal.s.pVMR0))
        return true;

    RTAssertMsg1Weak("AssertEMT", iLine, pszFile, pszFunction);
    RTAssertPanic();
    return false;
}


/** @interface_method_impl{PDMDRVHLPR0,pfnAssertOther} */
static DECLCALLBACK(bool) pdmR0DrvHlp_AssertOther(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    if (!VM_IS_EMT(pDrvIns->Internal.s.pVMR0))
        return true;

    RTAssertMsg1Weak("AssertOther", iLine, pszFile, pszFunction);
    RTAssertPanic();
    return false;
}


/** @interface_method_impl{PDMDRVHLPR0,pfnFTSetCheckpoint} */
static DECLCALLBACK(int) pdmR0DrvHlp_FTSetCheckpoint(PPDMDRVINS pDrvIns, FTMCHECKPOINTTYPE enmType)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    return FTMSetCheckpoint(pDrvIns->Internal.s.pVMR0, enmType);
}


/**
 * The Ring-0 Context Driver Helper Callbacks.
 */
extern DECLEXPORT(const PDMDRVHLPR0) g_pdmR0DrvHlp =
{
    PDM_DRVHLPRC_VERSION,
    pdmR0DrvHlp_VMSetError,
    pdmR0DrvHlp_VMSetErrorV,
    pdmR0DrvHlp_VMSetRuntimeError,
    pdmR0DrvHlp_VMSetRuntimeErrorV,
    pdmR0DrvHlp_AssertEMT,
    pdmR0DrvHlp_AssertOther,
    pdmR0DrvHlp_FTSetCheckpoint,
    PDM_DRVHLPRC_VERSION
};

/** @} */




/**
 * Sets an irq on the PIC and I/O APIC.
 *
 * @returns true if delivered, false if postponed.
 * @param   pVM         The cross context VM structure.
 * @param   iIrq        The irq.
 * @param   iLevel      The new level.
 * @param   uTagSrc     The IRQ tag and source.
 *
 * @remarks The caller holds the PDM lock.
 */
static bool pdmR0IsaSetIrq(PVM pVM, int iIrq, int iLevel, uint32_t uTagSrc)
{
    if (RT_LIKELY(    (   pVM->pdm.s.IoApic.pDevInsR0
                       || !pVM->pdm.s.IoApic.pDevInsR3)
                  &&  (   pVM->pdm.s.Pic.pDevInsR0
                       || !pVM->pdm.s.Pic.pDevInsR3)))
    {
        if (pVM->pdm.s.Pic.pDevInsR0)
            pVM->pdm.s.Pic.pfnSetIrqR0(pVM->pdm.s.Pic.pDevInsR0, iIrq, iLevel, uTagSrc);
        if (pVM->pdm.s.IoApic.pDevInsR0)
            pVM->pdm.s.IoApic.pfnSetIrqR0(pVM->pdm.s.IoApic.pDevInsR0, iIrq, iLevel, uTagSrc);
        return true;
    }

    /* queue for ring-3 execution. */
    PPDMDEVHLPTASK pTask = (PPDMDEVHLPTASK)PDMQueueAlloc(pVM->pdm.s.pDevHlpQueueR0);
    AssertReturn(pTask, false);

    pTask->enmOp = PDMDEVHLPTASKOP_ISA_SET_IRQ;
    pTask->pDevInsR3 = NIL_RTR3PTR; /* not required */
    pTask->u.IsaSetIRQ.iIrq = iIrq;
    pTask->u.IsaSetIRQ.iLevel = iLevel;
    pTask->u.IsaSetIRQ.uTagSrc = uTagSrc;

    PDMQueueInsertEx(pVM->pdm.s.pDevHlpQueueR0, &pTask->Core, 0);
    return false;
}


/**
 * PDMDevHlpCallR0 helper.
 *
 * @returns See PFNPDMDEVREQHANDLERR0.
 * @param   pGVM    The global (ring-0) VM structure. (For validation.)
 * @param   pVM     The cross context VM structure. (For validation.)
 * @param   pReq    Pointer to the request buffer.
 */
VMMR0_INT_DECL(int) PDMR0DeviceCallReqHandler(PGVM pGVM, PVM pVM, PPDMDEVICECALLREQHANDLERREQ pReq)
{
    /*
     * Validate input and make the call.
     */
    int rc = GVMMR0ValidateGVMandVM(pGVM, pVM);
    if (RT_SUCCESS(rc))
    {
        AssertPtrReturn(pReq, VERR_INVALID_POINTER);
        AssertMsgReturn(pReq->Hdr.cbReq == sizeof(*pReq), ("%#x != %#x\n", pReq->Hdr.cbReq, sizeof(*pReq)), VERR_INVALID_PARAMETER);

        PPDMDEVINS pDevIns = pReq->pDevInsR0;
        AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);
        AssertReturn(pDevIns->Internal.s.pVMR0 == pVM, VERR_INVALID_PARAMETER);

        PFNPDMDEVREQHANDLERR0 pfnReqHandlerR0 = pReq->pfnReqHandlerR0;
        AssertPtrReturn(pfnReqHandlerR0, VERR_INVALID_POINTER);

        rc = pfnReqHandlerR0(pDevIns, pReq->uOperation, pReq->u64Arg);
    }
    return rc;
}

