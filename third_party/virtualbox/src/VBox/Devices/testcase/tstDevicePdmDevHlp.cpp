/* $Id: tstDevicePdmDevHlp.cpp $ */
/** @file
 * tstDevice - Test framework for PDM devices/drivers, PDM helper implementation.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT /** @todo */
#include <VBox/types.h>
#include <VBox/version.h>
#include <VBox/vmm/pdmpci.h>

#include <iprt/assert.h>
#include <iprt/mem.h>

#include "tstDeviceInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** @def PDMDEV_ASSERT_DEVINS
 * Asserts the validity of the device instance.
 */
#ifdef VBOX_STRICT
# define PDMDEV_ASSERT_DEVINS(pDevIns)   \
    do { \
        AssertPtr(pDevIns); \
        Assert(pDevIns->u32Version == PDM_DEVINS_VERSION); \
        Assert(pDevIns->CTX_SUFF(pvInstanceData) == (void *)&pDevIns->achInstanceData[0]); \
    } while (0)
#else
# define PDMDEV_ASSERT_DEVINS(pDevIns)   do { } while (0)
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/



/** @interface_method_impl{PDMDEVHLPR3,pfnIOPortRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_IOPortRegister(PPDMDEVINS pDevIns, RTIOPORT Port, RTIOPORT cPorts, RTHCPTR pvUser, PFNIOMIOPORTOUT pfnOut, PFNIOMIOPORTIN pfnIn,
                                                    PFNIOMIOPORTOUTSTRING pfnOutStr, PFNIOMIOPORTINSTRING pfnInStr, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_IOPortRegister: caller='%s'/%d: Port=%#x cPorts=%#x pvUser=%p pfnOut=%p pfnIn=%p pfnOutStr=%p pfnInStr=%p p32_tszDesc=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, Port, cPorts, pvUser, pfnOut, pfnIn, pfnOutStr, pfnInStr, pszDesc, pszDesc));

    /** @todo Verify there is no overlapping. */

    RT_NOREF(pszDesc);
    int rc = VINF_SUCCESS;
    PRTDEVDUTIOPORT pIoPort = (PRTDEVDUTIOPORT)RTMemAllocZ(sizeof(RTDEVDUTIOPORT));
    if (RT_LIKELY(pIoPort))
    {
        pIoPort->PortStart   = Port;
        pIoPort->cPorts      = cPorts;
        pIoPort->pvUserR3    = pvUser;
        pIoPort->pfnOutR3    = pfnOut;
        pIoPort->pfnInR3     = pfnIn;
        pIoPort->pfnOutStrR3 = pfnOutStr;
        pIoPort->pfnInStrR3  = pfnInStr;
        RTListAppend(&pDevIns->Internal.s.pDut->LstIoPorts, &pIoPort->NdIoPorts);
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlow(("pdmR3DevHlp_IOPortRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnIOPortRegisterRC} */
static DECLCALLBACK(int) pdmR3DevHlp_IOPortRegisterRC(PPDMDEVINS pDevIns, RTIOPORT Port, RTIOPORT cPorts, RTRCPTR pvUser,
                                                      const char *pszOut, const char *pszIn,
                                                      const char *pszOutStr, const char *pszInStr, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_IOPortRegisterRC: caller='%s'/%d: Port=%#x cPorts=%#x pvUser=%p pszOut=%p:{%s} pszIn=%p:{%s} pszOutStr=%p:{%s} pszInStr=%p:{%s} pszDesc=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, Port, cPorts, pvUser, pszOut, pszOut, pszIn, pszIn, pszOutStr, pszOutStr, pszInStr, pszInStr, pszDesc, pszDesc));

    /** @todo */
    int rc = VINF_SUCCESS;
    //AssertFailed();

    LogFlow(("pdmR3DevHlp_IOPortRegisterRC: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnIOPortRegisterR0} */
static DECLCALLBACK(int) pdmR3DevHlp_IOPortRegisterR0(PPDMDEVINS pDevIns, RTIOPORT Port, RTIOPORT cPorts, RTR0PTR pvUser,
                                                      const char *pszOut, const char *pszIn,
                                                      const char *pszOutStr, const char *pszInStr, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_IOPortRegisterR0: caller='%s'/%d: Port=%#x cPorts=%#x pvUser=%p pszOut=%p:{%s} pszIn=%p:{%s} pszOutStr=%p:{%s} pszInStr=%p:{%s} pszDesc=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, Port, cPorts, pvUser, pszOut, pszOut, pszIn, pszIn, pszOutStr, pszOutStr, pszInStr, pszInStr, pszDesc, pszDesc));

    PTSTDEVDUTINT pThis = pDevIns->Internal.s.pDut;
    PRTDEVDUTIOPORT pIoPort;
    int rc = VERR_NOT_FOUND;
    RTListForEach(&pThis->LstIoPorts, pIoPort, RTDEVDUTIOPORT, NdIoPorts)
    {
        /** @todo Support overlapping port ranges. */
        if (   pIoPort->PortStart == Port
            && pIoPort->cPorts == cPorts)
        {
            rc = VINF_SUCCESS;
            pIoPort->pvUserR0 = (void *)pvUser;
            if (pszOut)
                rc = tstDevPdmLdrGetSymbol(pThis, pDevIns->pReg->szR0Mod, TSTDEVPDMMODTYPE_R0, pszOut, (PFNRT *)&pIoPort->pfnOutR0);
            if (RT_SUCCESS(rc) && pszIn)
                rc = tstDevPdmLdrGetSymbol(pThis, pDevIns->pReg->szR0Mod, TSTDEVPDMMODTYPE_R0, pszIn, (PFNRT *)&pIoPort->pfnInR0);
            if (RT_SUCCESS(rc) && pszOutStr)
                rc = tstDevPdmLdrGetSymbol(pThis, pDevIns->pReg->szR0Mod, TSTDEVPDMMODTYPE_R0, pszOutStr, (PFNRT *)&pIoPort->pfnOutStrR0);
            if (RT_SUCCESS(rc) && pszInStr)
                rc = tstDevPdmLdrGetSymbol(pThis, pDevIns->pReg->szR0Mod, TSTDEVPDMMODTYPE_R0, pszInStr, (PFNRT *)&pIoPort->pfnInStrR0);
            break;
        }
    }

    LogFlow(("pdmR3DevHlp_IOPortRegisterR0: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnIOPortDeregister} */
static DECLCALLBACK(int) pdmR3DevHlp_IOPortDeregister(PPDMDEVINS pDevIns, RTIOPORT Port, RTIOPORT cPorts)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_IOPortDeregister: caller='%s'/%d: Port=%#x cPorts=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, Port, cPorts));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_IOPortDeregister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnMMIORegister} */
static DECLCALLBACK(int) pdmR3DevHlp_MMIORegister(PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTHCPTR pvUser,
                                                  PFNIOMMMIOWRITE pfnWrite, PFNIOMMMIOREAD pfnRead, PFNIOMMMIOFILL pfnFill,
                                                  uint32_t fFlags, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIORegister: caller='%s'/%d: GCPhysStart=%RGp cbRange=%RGp pvUser=%p pfnWrite=%p pfnRead=%p pfnFill=%p fFlags=%#x pszDesc=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhysStart, cbRange, pvUser, pfnWrite, pfnRead, pfnFill, pszDesc, fFlags, pszDesc));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIORegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnMMIORegisterRC} */
static DECLCALLBACK(int) pdmR3DevHlp_MMIORegisterRC(PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTRCPTR pvUser,
                                                    const char *pszWrite, const char *pszRead, const char *pszFill)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIORegisterRC: caller='%s'/%d: GCPhysStart=%RGp cbRange=%RGp pvUser=%p pszWrite=%p:{%s} pszRead=%p:{%s} pszFill=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhysStart, cbRange, pvUser, pszWrite, pszWrite, pszRead, pszRead, pszFill, pszFill));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIORegisterRC: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}

/** @interface_method_impl{PDMDEVHLPR3,pfnMMIORegisterR0} */
static DECLCALLBACK(int) pdmR3DevHlp_MMIORegisterR0(PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTR0PTR pvUser,
                                                    const char *pszWrite, const char *pszRead, const char *pszFill)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIORegisterHC: caller='%s'/%d: GCPhysStart=%RGp cbRange=%RGp pvUser=%p pszWrite=%p:{%s} pszRead=%p:{%s} pszFill=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhysStart, cbRange, pvUser, pszWrite, pszWrite, pszRead, pszRead, pszFill, pszFill));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIORegisterR0: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnMMIODeregister} */
static DECLCALLBACK(int) pdmR3DevHlp_MMIODeregister(PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIODeregister: caller='%s'/%d: GCPhysStart=%RGp cbRange=%RGp\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhysStart, cbRange));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIODeregister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMIO2Register
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMIO2Register(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS cb,
                                                   uint32_t fFlags, void **ppv, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIO2Register: caller='%s'/%d: pPciDev=%p (%#x) iRegion=%#x cb=%#RGp fFlags=%RX32 ppv=%p pszDescp=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion,
             cb, fFlags, ppv, pszDesc, pszDesc));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 == pDevIns, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIO2Register: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @interface_method_impl{PDMDEVHLPR3,pfnMMIOExPreRegister}
 */
static DECLCALLBACK(int)
pdmR3DevHlp_MMIOExPreRegister(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS cbRegion, uint32_t fFlags,
                              const char *pszDesc,
                              RTHCPTR pvUser, PFNIOMMMIOWRITE pfnWrite, PFNIOMMMIOREAD pfnRead, PFNIOMMMIOFILL pfnFill,
                              RTR0PTR pvUserR0, const char *pszWriteR0, const char *pszReadR0, const char *pszFillR0,
                              RTRCPTR pvUserRC, const char *pszWriteRC, const char *pszReadRC, const char *pszFillRC)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIOExPreRegister: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x cbRegion=%#RGp fFlags=%RX32 pszDesc=%p:{%s}\n"
             "                               pvUser=%p pfnWrite=%p pfnRead=%p pfnFill=%p\n"
             "                               pvUserR0=%p pszWriteR0=%s pszReadR0=%s pszFillR0=%s\n"
             "                               pvUserRC=%p pszWriteRC=%s pszReadRC=%s pszFillRC=%s\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion, cbRegion,
             fFlags, pszDesc, pszDesc,
             pvUser, pfnWrite, pfnRead, pfnFill,
             pvUserR0, pszWriteR0, pszReadR0, pszFillR0,
             pvUserRC, pszWriteRC, pszReadRC, pszFillRC));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 == pDevIns, VERR_INVALID_PARAMETER);

    /*
     * Resolve the functions.
     */
    AssertLogRelReturn(   (!pszWriteR0 && !pszReadR0 && !pszFillR0)
                       || (pDevIns->pReg->szR0Mod[0] && (pDevIns->pReg->fFlags & PDM_DEVREG_FLAGS_R0)),
                       VERR_INVALID_PARAMETER);
    AssertLogRelReturn(   (!pszWriteRC && !pszReadRC && !pszFillRC)
                       || (pDevIns->pReg->szRCMod[0] && (pDevIns->pReg->fFlags & PDM_DEVREG_FLAGS_RC)),
                       VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIOExPreRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMIOExDeregister
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMIOExDeregister(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIOExDeregister: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion));

    AssertReturn(iRegion <= UINT8_MAX || iRegion == UINT32_MAX, VERR_INVALID_PARAMETER);
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 == pDevIns, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIOExDeregister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMIOExMap
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMIOExMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS GCPhys)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIOExMap: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x GCPhys=%#RGp\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion, GCPhys));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 != NULL, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIOExMap: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMIOExUnmap
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMIOExUnmap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS GCPhys)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIOExUnmap: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x GCPhys=%#RGp\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion, GCPhys));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 != NULL, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIOExUnmap: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMIOExReduce
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMIOExReduce(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS cbRegion)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIOExReduce: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x cbRegion=%RGp\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion, cbRegion));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 != NULL, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIOExReduce: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMHyperMapMMIO2
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMHyperMapMMIO2(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS off,
                                                     RTGCPHYS cb, const char *pszDesc, PRTRCPTR pRCPtr)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMHyperMapMMIO2: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x off=%RGp cb=%RGp pszDesc=%p:{%s} pRCPtr=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion, off, cb, pszDesc, pszDesc, pRCPtr));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 == pDevIns, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMHyperMapMMIO2: caller='%s'/%d: returns %Rrc *pRCPtr=%RRv\n", pDevIns->pReg->szName, pDevIns->iInstance, rc, *pRCPtr));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnMMIO2MapKernel
 */
static DECLCALLBACK(int) pdmR3DevHlp_MMIO2MapKernel(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS off,
                                                    RTGCPHYS cb,const char *pszDesc, PRTR0PTR pR0Ptr)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMIO2MapKernel: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%#x off=%RGp cb=%RGp pszDesc=%p:{%s} pR0Ptr=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev ? pPciDev->uDevFn : UINT32_MAX, iRegion, off, cb, pszDesc, pszDesc, pR0Ptr));
    //AssertReturn(!pPciDev || pPciDev->Int.s.pDevInsR3 == pDevIns, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_MMIO2MapKernel: caller='%s'/%d: returns %Rrc *pR0Ptr=%RHv\n", pDevIns->pReg->szName, pDevIns->iInstance, rc, *pR0Ptr));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnROMRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_ROMRegister(PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, uint32_t cbRange,
                                                 const void *pvBinary, uint32_t cbBinary, uint32_t fFlags, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_ROMRegister: caller='%s'/%d: GCPhysStart=%RGp cbRange=%#x pvBinary=%p cbBinary=%#x fFlags=%#RX32 pszDesc=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhysStart, cbRange, pvBinary, cbBinary, fFlags, pszDesc, pszDesc));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_ROMRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnROMProtectShadow} */
static DECLCALLBACK(int) pdmR3DevHlp_ROMProtectShadow(PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, uint32_t cbRange, PGMROMPROT enmProt)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_ROMProtectShadow: caller='%s'/%d: GCPhysStart=%RGp cbRange=%#x enmProt=%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhysStart, cbRange, enmProt));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_ROMProtectShadow: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnSSMRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_SSMRegister(PPDMDEVINS pDevIns, uint32_t uVersion, size_t cbGuess, const char *pszBefore,
                                                 PFNSSMDEVLIVEPREP pfnLivePrep, PFNSSMDEVLIVEEXEC pfnLiveExec, PFNSSMDEVLIVEVOTE pfnLiveVote,
                                                 PFNSSMDEVSAVEPREP pfnSavePrep, PFNSSMDEVSAVEEXEC pfnSaveExec, PFNSSMDEVSAVEDONE pfnSaveDone,
                                                 PFNSSMDEVLOADPREP pfnLoadPrep, PFNSSMDEVLOADEXEC pfnLoadExec, PFNSSMDEVLOADDONE pfnLoadDone)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_SSMRegister: caller='%s'/%d: uVersion=%#x cbGuess=%#x pszBefore=%p:{%s}\n"
             "    pfnLivePrep=%p pfnLiveExec=%p pfnLiveVote=%p pfnSavePrep=%p pfnSaveExec=%p pfnSaveDone=%p pszLoadPrep=%p pfnLoadExec=%p pfnLoadDone=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, uVersion, cbGuess, pszBefore, pszBefore,
             pfnLivePrep, pfnLiveExec, pfnLiveVote,
             pfnSavePrep, pfnSaveExec, pfnSaveDone,
             pfnLoadPrep, pfnLoadExec, pfnLoadDone));

    /** @todo */
    int rc = VINF_SUCCESS;
    //AssertFailed();

    LogFlow(("pdmR3DevHlp_SSMRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnTMTimerCreate} */
static DECLCALLBACK(int) pdmR3DevHlp_TMTimerCreate(PPDMDEVINS pDevIns, TMCLOCK enmClock, PFNTMTIMERDEV pfnCallback, void *pvUser,
                                                   uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_TMTimerCreate: caller='%s'/%d: enmClock=%d pfnCallback=%p pvUser=%p fFlags=%#x pszDesc=%p:{%s} ppTimer=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, enmClock, pfnCallback, pvUser, fFlags, pszDesc, pszDesc, ppTimer));

    int rc = VINF_SUCCESS;
    PTMTIMERR3 pTimer = (PTMTIMERR3)RTMemAllocZ(sizeof(TMTIMER));
    if (RT_LIKELY(pTimer))
    {
        pTimer->pVmmCallbacks  = pDevIns->Internal.s.pVmmCallbacks;
        pTimer->enmClock       = enmClock;
        pTimer->pfnCallbackDev = pfnCallback;
        pTimer->pvUser         = pvUser;
        pTimer->fFlags         = fFlags;
        RTListAppend(&pDevIns->Internal.s.pDut->LstTimers, &pTimer->NdDevTimers);
        *ppTimer = pTimer;
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlow(("pdmR3DevHlp_TMTimerCreate: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnTMUtcNow} */
static DECLCALLBACK(PRTTIMESPEC) pdmR3DevHlp_TMUtcNow(PPDMDEVINS pDevIns, PRTTIMESPEC pTime)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_TMUtcNow: caller='%s'/%d: pTime=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pTime));

    AssertFailed();

    LogFlow(("pdmR3DevHlp_TMUtcNow: caller='%s'/%d: returns %RU64\n", pDevIns->pReg->szName, pDevIns->iInstance, RTTimeSpecGetNano(pTime)));
    return pTime;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnTMTimeVirtGet} */
static DECLCALLBACK(uint64_t) pdmR3DevHlp_TMTimeVirtGet(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_TMTimeVirtGet: caller='%s'/%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    uint64_t u64Time = 0;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_TMTimeVirtGet: caller='%s'/%d: returns %RU64\n", pDevIns->pReg->szName, pDevIns->iInstance, u64Time));
    return u64Time;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnTMTimeVirtGetFreq} */
static DECLCALLBACK(uint64_t) pdmR3DevHlp_TMTimeVirtGetFreq(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_TMTimeVirtGetFreq: caller='%s'/%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    uint64_t u64Freq = 0;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_TMTimeVirtGetFreq: caller='%s'/%d: returns %RU64\n", pDevIns->pReg->szName, pDevIns->iInstance, u64Freq));
    return u64Freq;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnTMTimeVirtGetNano} */
static DECLCALLBACK(uint64_t) pdmR3DevHlp_TMTimeVirtGetNano(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_TMTimeVirtGetNano: caller='%s'/%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    uint64_t u64Nano = 0;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_TMTimeVirtGetNano: caller='%s'/%d: returns %RU64\n", pDevIns->pReg->szName, pDevIns->iInstance, u64Nano));
    return u64Nano;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetSupDrvSession} */
static DECLCALLBACK(PSUPDRVSESSION) pdmR3DevHlp_GetSupDrvSession(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_GetSupDrvSession: caller='%s'/%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    PTSTDEVDUTINT pThis = TSTDEV_PDMDEVINS_2_DUT(pDevIns);
    PSUPDRVSESSION pSession = TSTDEV_PTSTDEVSUPDRVSESSION_2_PSUPDRVSESSION(&pThis->SupSession);

    LogFlow(("pdmR3DevHlp_GetSupDrvSession: caller='%s'/%d: returns %#p\n", pDevIns->pReg->szName, pDevIns->iInstance, pSession));
    return pSession;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnQueryGenericUserObject} */
static DECLCALLBACK(void *) pdmR3DevHlp_QueryGenericUserObject(PPDMDEVINS pDevIns, PCRTUUID pUuid)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_QueryGenericUserObject: caller='%s'/%d: pUuid=%p:%RTuuid\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pUuid, pUuid));

    void *pvRet = NULL;

    LogRel(("pdmR3DevHlp_QueryGenericUserObject: caller='%s'/%d: returns %#p for %RTuuid\n",
            pDevIns->pReg->szName, pDevIns->iInstance, pvRet, pUuid));
    return pvRet;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysRead} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysRead(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, void *pvBuf, size_t cbRead)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysRead: caller='%s'/%d: GCPhys=%RGp pvBuf=%p cbRead=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhys, pvBuf, cbRead));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    Log(("pdmR3DevHlp_PhysRead: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysWrite} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysWrite(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, const void *pvBuf, size_t cbWrite)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysWrite: caller='%s'/%d: GCPhys=%RGp pvBuf=%p cbWrite=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhys, pvBuf, cbWrite));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    Log(("pdmR3DevHlp_PhysWrite: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysGCPhys2CCPtr} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysGCPhys2CCPtr(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint32_t fFlags, void **ppv, PPGMPAGEMAPLOCK pLock)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysGCPhys2CCPtr: caller='%s'/%d: GCPhys=%RGp fFlags=%#x ppv=%p pLock=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhys, fFlags, ppv, pLock));
    AssertReturn(!fFlags, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    Log(("pdmR3DevHlp_PhysGCPhys2CCPtr: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysGCPhys2CCPtrReadOnly} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysGCPhys2CCPtrReadOnly(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint32_t fFlags, const void **ppv, PPGMPAGEMAPLOCK pLock)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysGCPhys2CCPtrReadOnly: caller='%s'/%d: GCPhys=%RGp fFlags=%#x ppv=%p pLock=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhys, fFlags, ppv, pLock));
    AssertReturn(!fFlags, VERR_INVALID_PARAMETER);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    Log(("pdmR3DevHlp_PhysGCPhys2CCPtrReadOnly: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysReleasePageMappingLock} */
static DECLCALLBACK(void) pdmR3DevHlp_PhysReleasePageMappingLock(PPDMDEVINS pDevIns, PPGMPAGEMAPLOCK pLock)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysReleasePageMappingLock: caller='%s'/%d: pLock=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pLock));

    AssertFailed();

    Log(("pdmR3DevHlp_PhysReleasePageMappingLock: caller='%s'/%d: returns void\n", pDevIns->pReg->szName, pDevIns->iInstance));
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysReadGCVirt} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysReadGCVirt(PPDMDEVINS pDevIns, void *pvDst, RTGCPTR GCVirtSrc, size_t cb)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysReadGCVirt: caller='%s'/%d: pvDst=%p GCVirt=%RGv cb=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pvDst, GCVirtSrc, cb));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PhysReadGCVirt: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));

    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysWriteGCVirt} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysWriteGCVirt(PPDMDEVINS pDevIns, RTGCPTR GCVirtDst, const void *pvSrc, size_t cb)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysWriteGCVirt: caller='%s'/%d: GCVirtDst=%RGv pvSrc=%p cb=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCVirtDst, pvSrc, cb));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PhysWriteGCVirt: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));

    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPhysGCPtr2GCPhys} */
static DECLCALLBACK(int) pdmR3DevHlp_PhysGCPtr2GCPhys(PPDMDEVINS pDevIns, RTGCPTR GCPtr, PRTGCPHYS pGCPhys)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PhysGCPtr2GCPhys: caller='%s'/%d: GCPtr=%RGv pGCPhys=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPtr, pGCPhys));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PhysGCPtr2GCPhys: caller='%s'/%d: returns %Rrc *pGCPhys=%RGp\n", pDevIns->pReg->szName, pDevIns->iInstance, rc, *pGCPhys));

    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnMMHeapAlloc} */
static DECLCALLBACK(void *) pdmR3DevHlp_MMHeapAlloc(PPDMDEVINS pDevIns, size_t cb)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMHeapAlloc: caller='%s'/%d: cb=%#x\n", pDevIns->pReg->szName, pDevIns->iInstance, cb));

    PTSTDEVDUTINT pThis = TSTDEV_PDMDEVINS_2_DUT(pDevIns);

    void *pv = NULL;
    PTSTDEVMMHEAPALLOC pHeapAlloc = (PTSTDEVMMHEAPALLOC)RTMemAllocZ(cb + sizeof(TSTDEVMMHEAPALLOC) - 1);
    if (pHeapAlloc)
    {
        /* Poison */
        memset(&pHeapAlloc->abAlloc[0], 0xfe, cb);
        pHeapAlloc->cbAlloc       = cb;
        pHeapAlloc->pVmmCallbacks = pDevIns->Internal.s.pVmmCallbacks;
        pHeapAlloc->pDut          = pThis;

        tstDevDutLockExcl(pThis);
        RTListAppend(&pThis->LstMmHeap, &pHeapAlloc->NdMmHeap);
        tstDevDutUnlockExcl(pThis);

        pv = &pHeapAlloc->abAlloc[0];
    }

    LogFlow(("pdmR3DevHlp_MMHeapAlloc: caller='%s'/%d: returns %p\n", pDevIns->pReg->szName, pDevIns->iInstance, pv));
    return pv;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnMMHeapAllocZ} */
static DECLCALLBACK(void *) pdmR3DevHlp_MMHeapAllocZ(PPDMDEVINS pDevIns, size_t cb)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMHeapAllocZ: caller='%s'/%d: cb=%#x\n", pDevIns->pReg->szName, pDevIns->iInstance, cb));

    void *pv = pdmR3DevHlp_MMHeapAlloc(pDevIns, cb);
    if (VALID_PTR(pv))
        memset(pv, 0, cb);

    LogFlow(("pdmR3DevHlp_MMHeapAllocZ: caller='%s'/%d: returns %p\n", pDevIns->pReg->szName, pDevIns->iInstance, pv));
    return pv;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnMMHeapFree} */
static DECLCALLBACK(void) pdmR3DevHlp_MMHeapFree(PPDMDEVINS pDevIns, void *pv)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_MMHeapFree: caller='%s'/%d: pv=%p\n", pDevIns->pReg->szName, pDevIns->iInstance, pv));

    MMR3HeapFree(pv);

    LogFlow(("pdmR3DevHlp_MMHeapAlloc: caller='%s'/%d: returns void\n", pDevIns->pReg->szName, pDevIns->iInstance));
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMState} */
static DECLCALLBACK(VMSTATE) pdmR3DevHlp_VMState(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    VMSTATE enmVMState = VMSTATE_CREATING;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_VMState: caller='%s'/%d: returns %d\n", pDevIns->pReg->szName, pDevIns->iInstance,
             enmVMState));
    return enmVMState;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMTeleportedAndNotFullyResumedYet} */
static DECLCALLBACK(bool) pdmR3DevHlp_VMTeleportedAndNotFullyResumedYet(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    bool fRc = false;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_VMState: caller='%s'/%d: returns %RTbool\n", pDevIns->pReg->szName, pDevIns->iInstance,
             fRc));
    return fRc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMSetError} */
static DECLCALLBACK(int) pdmR3DevHlp_VMSetError(PPDMDEVINS pDevIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, ...)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(rc, pszFile, iLine, pszFunction, pszFormat);
    rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMSetErrorV} */
static DECLCALLBACK(int) pdmR3DevHlp_VMSetErrorV(PPDMDEVINS pDevIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(rc, pszFile, iLine, pszFunction, pszFormat, va);
    rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMSetRuntimeError} */
static DECLCALLBACK(int) pdmR3DevHlp_VMSetRuntimeError(PPDMDEVINS pDevIns, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, ...)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(fFlags, pszErrorId, pszFormat);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMSetRuntimeErrorV} */
static DECLCALLBACK(int) pdmR3DevHlp_VMSetRuntimeErrorV(PPDMDEVINS pDevIns, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, va_list va)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(fFlags, pszErrorId, pszFormat, va);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDBGFStopV} */
static DECLCALLBACK(int) pdmR3DevHlp_DBGFStopV(PPDMDEVINS pDevIns, const char *pszFile, unsigned iLine, const char *pszFunction, const char *pszFormat, va_list args)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
#ifdef LOG_ENABLED
    va_list va2;
    va_copy(va2, args);
    LogFlow(("pdmR3DevHlp_DBGFStopV: caller='%s'/%d: pszFile=%p:{%s} iLine=%d pszFunction=%p:{%s} pszFormat=%p:{%s} (%N)\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pszFile, pszFile, iLine, pszFunction, pszFunction, pszFormat, pszFormat, pszFormat, &va2));
    va_end(va2);
#endif

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DBGFStopV: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDBGFInfoRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_DBGFInfoRegister(PPDMDEVINS pDevIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDEV pfnHandler)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DBGFInfoRegister: caller='%s'/%d: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pszName, pszName, pszDesc, pszDesc, pfnHandler));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DBGFInfoRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDBGFRegRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_DBGFRegRegister(PPDMDEVINS pDevIns, PCDBGFREGDESC paRegisters)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DBGFRegRegister: caller='%s'/%d: paRegisters=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, paRegisters));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DBGFRegRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDBGFTraceBuf} */
static DECLCALLBACK(RTTRACEBUF) pdmR3DevHlp_DBGFTraceBuf(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RTTRACEBUF hTraceBuf = NULL;
    AssertFailed();
    LogFlow(("pdmR3DevHlp_DBGFTraceBuf: caller='%s'/%d: returns %p\n", pDevIns->pReg->szName, pDevIns->iInstance, hTraceBuf));
    return hTraceBuf;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnSTAMRegister} */
static DECLCALLBACK(void) pdmR3DevHlp_STAMRegister(PPDMDEVINS pDevIns, void *pvSample, STAMTYPE enmType, const char *pszName,
                                                   STAMUNIT enmUnit, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pvSample, enmType, pszName, enmUnit, pszDesc);
    AssertFailed();
}



/** @interface_method_impl{PDMDEVHLPR3,pfnSTAMRegisterF} */
static DECLCALLBACK(void) pdmR3DevHlp_STAMRegisterF(PPDMDEVINS pDevIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                    STAMUNIT enmUnit, const char *pszDesc, const char *pszName, ...)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName);
    AssertFailed();
}


/** @interface_method_impl{PDMDEVHLPR3,pfnSTAMRegisterV} */
static DECLCALLBACK(void) pdmR3DevHlp_STAMRegisterV(PPDMDEVINS pDevIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                    STAMUNIT enmUnit, const char *pszDesc, const char *pszName, va_list args)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, args);
    AssertFailed();
}


/**
 * @interface_method_impl{PDMDEVHLPR3,pfnPCIRegister}
 */
static DECLCALLBACK(int) pdmR3DevHlp_PCIRegister(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t idxDevCfg, uint32_t fFlags,
                                                 uint8_t uPciDevNo, uint8_t uPciFunNo, const char *pszName)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PCIRegister: caller='%s'/%d: pPciDev=%p:{.config={%#.256Rhxs} idxDevCfg=%d fFlags=%#x uPciDevNo=%#x uPciFunNo=%#x pszName=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev->abConfig, idxDevCfg, fFlags, uPciDevNo, uPciFunNo, pszName, pszName ? pszName : ""));

    /*
     * Validate input.
     */
    AssertLogRelMsgReturn(RT_VALID_PTR(pPciDev),
                          ("'%s'/%d: Invalid pPciDev value: %p\n", pDevIns->pReg->szName, pDevIns->iInstance, pPciDev),
                          VERR_INVALID_POINTER);
    AssertLogRelMsgReturn(PDMPciDevGetVendorId(pPciDev),
                          ("'%s'/%d: Vendor ID is not set!\n", pDevIns->pReg->szName, pDevIns->iInstance),
                          VERR_INVALID_POINTER);
    AssertLogRelMsgReturn(idxDevCfg < 256 || idxDevCfg == PDMPCIDEVREG_CFG_NEXT,
                          ("'%s'/%d: Invalid config selector: %#x\n", pDevIns->pReg->szName, pDevIns->iInstance, idxDevCfg),
                          VERR_OUT_OF_RANGE);
    AssertLogRelMsgReturn(   uPciDevNo < 32
                          || uPciDevNo == PDMPCIDEVREG_DEV_NO_FIRST_UNUSED
                          || uPciDevNo == PDMPCIDEVREG_DEV_NO_SAME_AS_PREV,
                          ("'%s'/%d: Invalid PCI device number: %#x\n", pDevIns->pReg->szName, pDevIns->iInstance, uPciDevNo),
                          VERR_INVALID_PARAMETER);
    AssertLogRelMsgReturn(   uPciFunNo < 8
                          || uPciFunNo == PDMPCIDEVREG_FUN_NO_FIRST_UNUSED,
                          ("'%s'/%d: Invalid PCI funcion number: %#x\n", pDevIns->pReg->szName, pDevIns->iInstance, uPciFunNo),
                          VERR_INVALID_PARAMETER);
    AssertLogRelMsgReturn(!(fFlags & ~PDMPCIDEVREG_F_VALID_MASK),
                          ("'%s'/%d: Invalid flags: %#x\n", pDevIns->pReg->szName, pDevIns->iInstance, fFlags),
                          VERR_INVALID_FLAGS);
    int rc = VINF_SUCCESS;
    pDevIns->Internal.s.pDut->pPciDev = pPciDev;

    LogFlow(("pdmR3DevHlp_PCIRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCIRegisterMsi} */
static DECLCALLBACK(int) pdmR3DevHlp_PCIRegisterMsi(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, PPDMMSIREG pMsiReg)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PCIRegisterMsi: caller='%s'/%d: pPciDev=%p:{%#x} pMsgReg=%p:{cMsiVectors=%d, cMsixVectors=%d}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev->uDevFn, pMsiReg, pMsiReg->cMsiVectors, pMsiReg->cMsixVectors));

    int rc = VERR_NOT_SUPPORTED; /** @todo */

    LogFlow(("pdmR3DevHlp_PCIRegisterMsi: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCIIORegionRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_PCIIORegionRegister(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                                         RTGCPHYS cbRegion, PCIADDRESSSPACE enmType, PFNPCIIOREGIONMAP pfnCallback)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PCIIORegionRegister: caller='%s'/%d: pPciDev=%p:{%#x} iRegion=%d cbRegion=%RGp enmType=%d pfnCallback=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev->uDevFn, iRegion, cbRegion, enmType, pfnCallback));

    AssertLogRelMsgReturn(iRegion < VBOX_PCI_NUM_REGIONS,
                          ("Region %u exceeds maximum region index %u\n", iRegion, VBOX_PCI_NUM_REGIONS),
                          VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;
    PTSTDEVDUTINT pThis = TSTDEV_PDMDEVINS_2_DUT(pDevIns);
    PTSTDEVDUTPCIREGION pRegion = &pThis->aPciRegions[iRegion];
    pRegion->cbRegion     = cbRegion;
    pRegion->enmType      = enmType;
    pRegion->pfnRegionMap = pfnCallback;

    LogFlow(("pdmR3DevHlp_PCIIORegionRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCISetConfigCallbacks} */
static DECLCALLBACK(void) pdmR3DevHlp_PCISetConfigCallbacks(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, PFNPCICONFIGREAD pfnRead, PPFNPCICONFIGREAD ppfnReadOld,
                                                            PFNPCICONFIGWRITE pfnWrite, PPFNPCICONFIGWRITE ppfnWriteOld)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PCISetConfigCallbacks: caller='%s'/%d: pPciDev=%p pfnRead=%p ppfnReadOld=%p pfnWrite=%p ppfnWriteOld=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pfnRead, ppfnReadOld, pfnWrite, ppfnWriteOld));

    /*
     * Validate input and resolve defaults.
     */
    AssertPtr(pfnRead);
    AssertPtr(pfnWrite);
    AssertPtrNull(ppfnReadOld);
    AssertPtrNull(ppfnWriteOld);
    AssertPtrNull(pPciDev);

    AssertFailed();

    LogFlow(("pdmR3DevHlp_PCISetConfigCallbacks: caller='%s'/%d: returns void\n", pDevIns->pReg->szName, pDevIns->iInstance));
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCIPhysRead} */
static DECLCALLBACK(int) pdmR3DevHlp_PCIPhysRead(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, RTGCPHYS GCPhys, void *pvBuf, size_t cbRead)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pPciDev);
    return pDevIns->pHlpR3->pfnPhysRead(pDevIns, GCPhys, pvBuf, cbRead);
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCIPhysWrite} */
static DECLCALLBACK(int)
pdmR3DevHlp_PCIPhysWrite(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, RTGCPHYS GCPhys, const void *pvBuf, size_t cbWrite)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pPciDev);
    return pDevIns->pHlpR3->pfnPhysWrite(pDevIns, GCPhys, pvBuf, cbWrite);
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCISetIrq} */
static DECLCALLBACK(void) pdmR3DevHlp_PCISetIrq(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PCISetIrq: caller='%s'/%d: pPciDev=%p:{%#x} iIrq=%d iLevel=%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciDev, pPciDev->uDevFn, iIrq, iLevel));

    AssertFailed();

    LogFlow(("pdmR3DevHlp_PCISetIrq: caller='%s'/%d: returns void\n", pDevIns->pReg->szName, pDevIns->iInstance));
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCISetIrqNoWait} */
static DECLCALLBACK(void) pdmR3DevHlp_PCISetIrqNoWait(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, int iIrq, int iLevel)
{
    pdmR3DevHlp_PCISetIrq(pDevIns, pPciDev, iIrq, iLevel);
}


/** @interface_method_impl{PDMDEVHLPR3,pfnISASetIrq} */
static DECLCALLBACK(void) pdmR3DevHlp_ISASetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_ISASetIrq: caller='%s'/%d: iIrq=%d iLevel=%d\n", pDevIns->pReg->szName, pDevIns->iInstance, iIrq, iLevel));

    //AssertFailed();

    LogFlow(("pdmR3DevHlp_ISASetIrq: caller='%s'/%d: returns void\n", pDevIns->pReg->szName, pDevIns->iInstance));
}


/** @interface_method_impl{PDMDEVHLPR3,pfnISASetIrqNoWait} */
static DECLCALLBACK(void) pdmR3DevHlp_ISASetIrqNoWait(PPDMDEVINS pDevIns, int iIrq, int iLevel)
{
    pdmR3DevHlp_ISASetIrq(pDevIns, iIrq, iLevel);
}


/** @interface_method_impl{PDMDEVHLPR3,pfnIoApicSendMsi} */
static DECLCALLBACK(void) pdmR3DevHlp_IoApicSendMsi(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint32_t uValue)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_IoApicSendMsi: caller='%s'/%d: GCPhys=%RGp uValue=%#x\n", pDevIns->pReg->szName, pDevIns->iInstance, GCPhys, uValue));

    /*
     * Validate input.
     */
    Assert(GCPhys != 0);
    Assert(uValue != 0);
    AssertFailed();

    LogFlow(("pdmR3DevHlp_IoApicSendMsi: caller='%s'/%d: returns void\n", pDevIns->pReg->szName, pDevIns->iInstance));
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDriverAttach} */
static DECLCALLBACK(int) pdmR3DevHlp_DriverAttach(PPDMDEVINS pDevIns, uint32_t iLun, PPDMIBASE pBaseInterface, PPDMIBASE *ppBaseInterface, const char *pszDesc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DriverAttach: caller='%s'/%d: iLun=%d pBaseInterface=%p ppBaseInterface=%p pszDesc=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, iLun, pBaseInterface, ppBaseInterface, pszDesc, pszDesc));

    /** @todo */
    int rc = VERR_PDM_NO_ATTACHED_DRIVER;
    //AssertFailed();

    LogFlow(("pdmR3DevHlp_DriverAttach: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDriverDetach} */
static DECLCALLBACK(int) pdmR3DevHlp_DriverDetach(PPDMDEVINS pDevIns, PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDMDEV_ASSERT_DEVINS(pDevIns); RT_NOREF_PV(pDevIns);
    LogFlow(("pdmR3DevHlp_DriverDetach: caller='%s'/%d: pDrvIns=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pDrvIns));

    RT_NOREF(fFlags);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DriverDetach: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnQueueCreate} */
static DECLCALLBACK(int) pdmR3DevHlp_QueueCreate(PPDMDEVINS pDevIns, size_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                                 PFNPDMQUEUEDEV pfnCallback, bool fRZEnabled, const char *pszName, PPDMQUEUE *ppQueue)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_QueueCreate: caller='%s'/%d: cbItem=%#x cItems=%#x cMilliesInterval=%u pfnCallback=%p fRZEnabled=%RTbool pszName=%p:{%s} ppQueue=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, cbItem, cItems, cMilliesInterval, pfnCallback, fRZEnabled, pszName, pszName, ppQueue));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_QueueCreate: caller='%s'/%d: returns %Rrc *ppQueue=%p\n", pDevIns->pReg->szName, pDevIns->iInstance, rc, *ppQueue));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCritSectInit} */
static DECLCALLBACK(int) pdmR3DevHlp_CritSectInit(PPDMDEVINS pDevIns, PPDMCRITSECT pCritSect, RT_SRC_POS_DECL,
                                                  const char *pszNameFmt, va_list va)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_CritSectInit: caller='%s'/%d: pCritSect=%p pszNameFmt=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pCritSect, pszNameFmt, pszNameFmt));

    RT_NOREF(RT_SRC_POS_ARGS, pszNameFmt, pCritSect, va);
    int rc = RTCritSectInit(&pCritSect->s.CritSect);
    if (RT_SUCCESS(rc))
        pCritSect->s.pVmmCallbacks = pDevIns->Internal.s.pVmmCallbacks;

    LogFlow(("pdmR3DevHlp_CritSectInit: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCritSectGetNop} */
static DECLCALLBACK(PPDMCRITSECT) pdmR3DevHlp_CritSectGetNop(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    AssertFailed();
    PPDMCRITSECT pCritSect = NULL;

    LogFlow(("pdmR3DevHlp_CritSectGetNop: caller='%s'/%d: return %p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pCritSect));
    return pCritSect;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCritSectGetNopR0} */
static DECLCALLBACK(R0PTRTYPE(PPDMCRITSECT)) pdmR3DevHlp_CritSectGetNopR0(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    R0PTRTYPE(PPDMCRITSECT) pCritSect = 0;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_CritSectGetNopR0: caller='%s'/%d: return %RHv\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pCritSect));
    return pCritSect;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCritSectGetNopRC} */
static DECLCALLBACK(RCPTRTYPE(PPDMCRITSECT)) pdmR3DevHlp_CritSectGetNopRC(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    RCPTRTYPE(PPDMCRITSECT) pCritSect = 0;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_CritSectGetNopRC: caller='%s'/%d: return %RRv\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pCritSect));
    return pCritSect;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnSetDeviceCritSect} */
static DECLCALLBACK(int) pdmR3DevHlp_SetDeviceCritSect(PPDMDEVINS pDevIns, PPDMCRITSECT pCritSect)
{
    /*
     * Validate input.
     *
     * Note! We only allow the automatically created default critical section
     *       to be replaced by this API.
     */
    PDMDEV_ASSERT_DEVINS(pDevIns);
    AssertPtrReturn(pCritSect, VERR_INVALID_POINTER);
    LogFlow(("pdmR3DevHlp_SetDeviceCritSect: caller='%s'/%d: pCritSect=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pCritSect));

    /** @todo Implement default atomatic critical section. */
    pDevIns->pCritSectRoR3 = pCritSect;

    LogFlow(("pdmR3DevHlp_SetDeviceCritSect: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, VINF_SUCCESS));
    return VINF_SUCCESS;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnThreadCreate} */
static DECLCALLBACK(int) pdmR3DevHlp_ThreadCreate(PPDMDEVINS pDevIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDEV pfnThread,
                                                  PFNPDMTHREADWAKEUPDEV pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_ThreadCreate: caller='%s'/%d: ppThread=%p pvUser=%p pfnThread=%p pfnWakeup=%p cbStack=%#zx enmType=%d pszName=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, ppThread, pvUser, pfnThread, pfnWakeup, cbStack, enmType, pszName, pszName));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_ThreadCreate: caller='%s'/%d: returns %Rrc *ppThread=%RTthrd\n", pDevIns->pReg->szName, pDevIns->iInstance,
            rc, *ppThread));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnSetAsyncNotification} */
static DECLCALLBACK(int) pdmR3DevHlp_SetAsyncNotification(PPDMDEVINS pDevIns, PFNPDMDEVASYNCNOTIFY pfnAsyncNotify)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_SetAsyncNotification: caller='%s'/%d: pfnAsyncNotify=%p\n", pDevIns->pReg->szName, pDevIns->iInstance, pfnAsyncNotify));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_SetAsyncNotification: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnAsyncNotificationCompleted} */
static DECLCALLBACK(void) pdmR3DevHlp_AsyncNotificationCompleted(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    AssertFailed();
}


/** @interface_method_impl{PDMDEVHLPR3,pfnRTCRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_RTCRegister(PPDMDEVINS pDevIns, PCPDMRTCREG pRtcReg, PCPDMRTCHLP *ppRtcHlp)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_RTCRegister: caller='%s'/%d: pRtcReg=%p:{.u32Version=%#x, .pfnWrite=%p, .pfnRead=%p} ppRtcHlp=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pRtcReg, pRtcReg->u32Version, pRtcReg->pfnWrite,
             pRtcReg->pfnWrite, ppRtcHlp));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_RTCRegister: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDMARegister} */
static DECLCALLBACK(int) pdmR3DevHlp_DMARegister(PPDMDEVINS pDevIns, unsigned uChannel, PFNDMATRANSFERHANDLER pfnTransferHandler, void *pvUser)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    RT_NOREF(uChannel, pfnTransferHandler, pvUser);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DMARegister: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDMAReadMemory} */
static DECLCALLBACK(int) pdmR3DevHlp_DMAReadMemory(PPDMDEVINS pDevIns, unsigned uChannel, void *pvBuffer, uint32_t off, uint32_t cbBlock, uint32_t *pcbRead)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DMAReadMemory: caller='%s'/%d: uChannel=%d pvBuffer=%p off=%#x cbBlock=%#x pcbRead=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, uChannel, pvBuffer, off, cbBlock, pcbRead));

    RT_NOREF(uChannel, pvBuffer, off, cbBlock, pcbRead);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DMAReadMemory: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDMAWriteMemory} */
static DECLCALLBACK(int) pdmR3DevHlp_DMAWriteMemory(PPDMDEVINS pDevIns, unsigned uChannel, const void *pvBuffer, uint32_t off, uint32_t cbBlock, uint32_t *pcbWritten)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DMAWriteMemory: caller='%s'/%d: uChannel=%d pvBuffer=%p off=%#x cbBlock=%#x pcbWritten=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, uChannel, pvBuffer, off, cbBlock, pcbWritten));

    RT_NOREF(uChannel, pvBuffer, off, cbBlock, pcbWritten);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DMAWriteMemory: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDMASetDREQ} */
static DECLCALLBACK(int) pdmR3DevHlp_DMASetDREQ(PPDMDEVINS pDevIns, unsigned uChannel, unsigned uLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DMASetDREQ: caller='%s'/%d: uChannel=%d uLevel=%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance, uChannel, uLevel));

    RT_NOREF(uChannel, uLevel);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DMASetDREQ: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}

/** @interface_method_impl{PDMDEVHLPR3,pfnDMAGetChannelMode} */
static DECLCALLBACK(uint8_t) pdmR3DevHlp_DMAGetChannelMode(PPDMDEVINS pDevIns, unsigned uChannel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DMAGetChannelMode: caller='%s'/%d: uChannel=%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance, uChannel));

    uint8_t u8Mode = 0;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DMAGetChannelMode: caller='%s'/%d: returns %#04x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, u8Mode));
    return u8Mode;
}

/** @interface_method_impl{PDMDEVHLPR3,pfnDMASchedule} */
static DECLCALLBACK(void) pdmR3DevHlp_DMASchedule(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DMASchedule: caller='%s'/%d\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    AssertFailed();
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCMOSWrite} */
static DECLCALLBACK(int) pdmR3DevHlp_CMOSWrite(PPDMDEVINS pDevIns, unsigned iReg, uint8_t u8Value)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_CMOSWrite: caller='%s'/%d: iReg=%#04x u8Value=%#04x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, iReg, u8Value));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_CMOSWrite: caller='%s'/%d: return %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCMOSRead} */
static DECLCALLBACK(int) pdmR3DevHlp_CMOSRead(PPDMDEVINS pDevIns, unsigned iReg, uint8_t *pu8Value)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_CMOSWrite: caller='%s'/%d: iReg=%#04x pu8Value=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, iReg, pu8Value));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_CMOSWrite: caller='%s'/%d: return %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnAssertEMT} */
static DECLCALLBACK(bool) pdmR3DevHlp_AssertEMT(PPDMDEVINS pDevIns, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pszFile, iLine, pszFunction);
    AssertFailed();
    return false;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnAssertOther} */
static DECLCALLBACK(bool) pdmR3DevHlp_AssertOther(PPDMDEVINS pDevIns, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    RT_NOREF(pszFile, iLine, pszFunction);
    AssertFailed();
    return false;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnLdrGetRCInterfaceSymbols} */
static DECLCALLBACK(int) pdmR3DevHlp_LdrGetRCInterfaceSymbols(PPDMDEVINS pDevIns, void *pvInterface, size_t cbInterface,
                                                              const char *pszSymPrefix, const char *pszSymList)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PDMLdrGetRCInterfaceSymbols: caller='%s'/%d: pvInterface=%p cbInterface=%zu pszSymPrefix=%p:{%s} pszSymList=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pvInterface, cbInterface, pszSymPrefix, pszSymPrefix, pszSymList, pszSymList));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PDMLdrGetRCInterfaceSymbols: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName,
             pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnLdrGetR0InterfaceSymbols} */
static DECLCALLBACK(int) pdmR3DevHlp_LdrGetR0InterfaceSymbols(PPDMDEVINS pDevIns, void *pvInterface, size_t cbInterface,
                                                              const char *pszSymPrefix, const char *pszSymList)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PDMLdrGetR0InterfaceSymbols: caller='%s'/%d: pvInterface=%p cbInterface=%zu pszSymPrefix=%p:{%s} pszSymList=%p:{%s}\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pvInterface, cbInterface, pszSymPrefix, pszSymPrefix, pszSymList, pszSymList));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PDMLdrGetR0InterfaceSymbols: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName,
             pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnCallR0} */
static DECLCALLBACK(int) pdmR3DevHlp_CallR0(PPDMDEVINS pDevIns, uint32_t uOperation, uint64_t u64Arg)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_CallR0: caller='%s'/%d: uOperation=%#x u64Arg=%#RX64\n",
             pDevIns->pReg->szName, pDevIns->iInstance, uOperation, u64Arg));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_CallR0: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName,
             pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMGetSuspendReason} */
static DECLCALLBACK(VMSUSPENDREASON) pdmR3DevHlp_VMGetSuspendReason(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VMSUSPENDREASON enmReason = VMSUSPENDREASON_INVALID;
    AssertFailed();
    LogFlow(("pdmR3DevHlp_VMGetSuspendReason: caller='%s'/%d: returns %d\n",
             pDevIns->pReg->szName, pDevIns->iInstance, enmReason));
    return enmReason;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMGetResumeReason} */
static DECLCALLBACK(VMRESUMEREASON) pdmR3DevHlp_VMGetResumeReason(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VMRESUMEREASON enmReason = VMRESUMEREASON_INVALID;
    AssertFailed();
    LogFlow(("pdmR3DevHlp_VMGetResumeReason: caller='%s'/%d: returns %d\n",
             pDevIns->pReg->szName, pDevIns->iInstance, enmReason));
    return enmReason;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetUVM} */
static DECLCALLBACK(PUVM) pdmR3DevHlp_GetUVM(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    AssertFailed();
    LogFlow(("pdmR3DevHlp_GetUVM: caller='%s'/%d: returns %p\n", pDevIns->pReg->szName, pDevIns->iInstance, NULL));
    return NULL;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetVM} */
static DECLCALLBACK(PVM) pdmR3DevHlp_GetVM(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PTSTDEVDUTINT pThis = TSTDEV_PDMDEVINS_2_DUT(pDevIns);
    PVM pVM = pThis->pVm;
    LogFlow(("pdmR3DevHlp_GetVM: caller='%s'/%d: returns %p\n", pDevIns->pReg->szName, pDevIns->iInstance, NULL));
    return pVM;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetVMCPU} */
static DECLCALLBACK(PVMCPU) pdmR3DevHlp_GetVMCPU(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    AssertFailed();
    LogFlow(("pdmR3DevHlp_GetVMCPU: caller='%s'/%d for CPU %u\n", pDevIns->pReg->szName, pDevIns->iInstance, NULL));
    return NULL;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetCurrentCpuId} */
static DECLCALLBACK(VMCPUID) pdmR3DevHlp_GetCurrentCpuId(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VMCPUID idCpu = 0;
    AssertFailed();
    LogFlow(("pdmR3DevHlp_GetCurrentCpuId: caller='%s'/%d for CPU %u\n", pDevIns->pReg->szName, pDevIns->iInstance, idCpu));
    return idCpu;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPCIBusRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_PCIBusRegister(PPDMDEVINS pDevIns, PPDMPCIBUSREG pPciBusReg,
                                                    PCPDMPCIHLPR3 *ppPciHlpR3, uint32_t *piBus)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PCIBusRegister: caller='%s'/%d: pPciBusReg=%p:{.u32Version=%#x, .pfnRegisterR3=%p, .pfnIORegionRegisterR3=%p, "
             ".pfnSetIrqR3=%p, .pszSetIrqRC=%p:{%s}, .pszSetIrqR0=%p:{%s}} ppPciHlpR3=%p piBus=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPciBusReg, pPciBusReg->u32Version, pPciBusReg->pfnRegisterR3,
             pPciBusReg->pfnIORegionRegisterR3, pPciBusReg->pfnSetIrqR3, pPciBusReg->pszSetIrqRC, pPciBusReg->pszSetIrqRC,
             pPciBusReg->pszSetIrqR0, pPciBusReg->pszSetIrqR0, ppPciHlpR3, piBus));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PCIBusRegister: caller='%s'/%d: returns %Rrc *piBus=%u\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc, *piBus));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPICRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_PICRegister(PPDMDEVINS pDevIns, PPDMPICREG pPicReg, PCPDMPICHLPR3 *ppPicHlpR3)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_PICRegister: caller='%s'/%d: pPicReg=%p:{.u32Version=%#x, .pfnSetIrqR3=%p, .pfnGetInterruptR3=%p, .pszGetIrqRC=%p:{%s}, .pszGetInterruptRC=%p:{%s}, .pszGetIrqR0=%p:{%s}, .pszGetInterruptR0=%p:{%s} } ppPicHlpR3=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pPicReg, pPicReg->u32Version, pPicReg->pfnSetIrqR3, pPicReg->pfnGetInterruptR3,
             pPicReg->pszSetIrqRC, pPicReg->pszSetIrqRC, pPicReg->pszGetInterruptRC, pPicReg->pszGetInterruptRC,
             pPicReg->pszSetIrqR0, pPicReg->pszSetIrqR0, pPicReg->pszGetInterruptR0, pPicReg->pszGetInterruptR0,
             ppPicHlpR3));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PICRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnAPICRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_APICRegister(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_APICRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnIOAPICRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_IOAPICRegister(PPDMDEVINS pDevIns, PPDMIOAPICREG pIoApicReg, PCPDMIOAPICHLPR3 *ppIoApicHlpR3)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_IOAPICRegister: caller='%s'/%d: pIoApicReg=%p:{.u32Version=%#x, .pfnSetIrqR3=%p, .pszSetIrqRC=%p:{%s}, .pszSetIrqR0=%p:{%s}} ppIoApicHlpR3=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pIoApicReg, pIoApicReg->u32Version, pIoApicReg->pfnSetIrqR3,
             pIoApicReg->pszSetIrqRC, pIoApicReg->pszSetIrqRC, pIoApicReg->pszSetIrqR0, pIoApicReg->pszSetIrqR0, ppIoApicHlpR3));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_IOAPICRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnHPETRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_HPETRegister(PPDMDEVINS pDevIns, PPDMHPETREG pHpetReg, PCPDMHPETHLPR3 *ppHpetHlpR3)
{
    PDMDEV_ASSERT_DEVINS(pDevIns); RT_NOREF_PV(pDevIns);
    LogFlow(("pdmR3DevHlp_HPETRegister: caller='%s'/%d:\n", pDevIns->pReg->szName, pDevIns->iInstance));

    RT_NOREF(pHpetReg, ppHpetHlpR3);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_HPETRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnPciRawRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_PciRawRegister(PPDMDEVINS pDevIns, PPDMPCIRAWREG pPciRawReg, PCPDMPCIRAWHLPR3 *ppPciRawHlpR3)
{
    PDMDEV_ASSERT_DEVINS(pDevIns); RT_NOREF_PV(pDevIns);
    LogFlow(("pdmR3DevHlp_PciRawRegister: caller='%s'/%d:\n", pDevIns->pReg->szName, pDevIns->iInstance));

    RT_NOREF(pPciRawReg, ppPciRawHlpR3);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_PciRawRegister: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnDMACRegister} */
static DECLCALLBACK(int) pdmR3DevHlp_DMACRegister(PPDMDEVINS pDevIns, PPDMDMACREG pDmacReg, PCPDMDMACHLP *ppDmacHlp)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_DMACRegister: caller='%s'/%d: pDmacReg=%p:{.u32Version=%#x, .pfnRun=%p, .pfnRegister=%p, .pfnReadMemory=%p, .pfnWriteMemory=%p, .pfnSetDREQ=%p, .pfnGetChannelMode=%p} ppDmacHlp=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pDmacReg, pDmacReg->u32Version, pDmacReg->pfnRun, pDmacReg->pfnRegister,
             pDmacReg->pfnReadMemory, pDmacReg->pfnWriteMemory, pDmacReg->pfnSetDREQ, pDmacReg->pfnGetChannelMode, ppDmacHlp));

    RT_NOREF(pDmacReg, ppDmacHlp);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_DMACRegister: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @copydoc PDMDEVHLPR3::pfnRegisterVMMDevHeap
 */
static DECLCALLBACK(int) pdmR3DevHlp_RegisterVMMDevHeap(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, RTR3PTR pvHeap, unsigned cbHeap)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_RegisterVMMDevHeap: caller='%s'/%d: GCPhys=%RGp pvHeap=%p cbHeap=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, GCPhys, pvHeap, cbHeap));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_RegisterVMMDevHeap: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/**
 * @interface_method_impl{PDMDEVHLPR3,pfnFirmwareRegister}
 */
static DECLCALLBACK(int) pdmR3DevHlp_FirmwareRegister(PPDMDEVINS pDevIns, PCPDMFWREG pFwReg, PCPDMFWHLPR3 *ppFwHlp)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_FirmwareRegister: caller='%s'/%d: pFWReg=%p:{.u32Version=%#x, .pfnIsHardReset=%p, .u32TheEnd=%#x} ppFwHlp=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, pFwReg, pFwReg->u32Version, pFwReg->pfnIsHardReset, pFwReg->u32TheEnd, ppFwHlp));

    RT_NOREF(pFwReg, ppFwHlp);
    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_FirmwareRegister: caller='%s'/%d: returns %Rrc\n",
             pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMReset} */
static DECLCALLBACK(int) pdmR3DevHlp_VMReset(PPDMDEVINS pDevIns, uint32_t fFlags)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_VMReset: caller='%s'/%d: fFlags=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, fFlags));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_VMReset: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMSuspend} */
static DECLCALLBACK(int) pdmR3DevHlp_VMSuspend(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_VMSuspend: caller='%s'/%d:\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_VMSuspend: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMSuspendSaveAndPowerOff} */
static DECLCALLBACK(int) pdmR3DevHlp_VMSuspendSaveAndPowerOff(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_VMSuspendSaveAndPowerOff: caller='%s'/%d:\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_VMSuspendSaveAndPowerOff: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMPowerOff} */
static DECLCALLBACK(int) pdmR3DevHlp_VMPowerOff(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_VMPowerOff: caller='%s'/%d:\n",
             pDevIns->pReg->szName, pDevIns->iInstance));

    int rc = VERR_NOT_IMPLEMENTED;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_VMPowerOff: caller='%s'/%d: returns %Rrc\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnA20IsEnabled} */
static DECLCALLBACK(bool) pdmR3DevHlp_A20IsEnabled(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);

    bool fRc = false;
    AssertFailed();

    LogFlow(("pdmR3DevHlp_A20IsEnabled: caller='%s'/%d: returns %d\n", pDevIns->pReg->szName, pDevIns->iInstance, fRc));
    return fRc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnA20Set} */
static DECLCALLBACK(void) pdmR3DevHlp_A20Set(PPDMDEVINS pDevIns, bool fEnable)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_A20Set: caller='%s'/%d: fEnable=%d\n", pDevIns->pReg->szName, pDevIns->iInstance, fEnable));
    AssertFailed();
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetCpuId} */
static DECLCALLBACK(void) pdmR3DevHlp_GetCpuId(PPDMDEVINS pDevIns, uint32_t iLeaf,
                                               uint32_t *pEax, uint32_t *pEbx, uint32_t *pEcx, uint32_t *pEdx)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3DevHlp_GetCpuId: caller='%s'/%d: iLeaf=%d pEax=%p pEbx=%p pEcx=%p pEdx=%p\n",
             pDevIns->pReg->szName, pDevIns->iInstance, iLeaf, pEax, pEbx, pEcx, pEdx));
    AssertPtr(pEax); AssertPtr(pEbx); AssertPtr(pEcx); AssertPtr(pEdx);

    AssertFailed();

    LogFlow(("pdmR3DevHlp_GetCpuId: caller='%s'/%d: returns void - *pEax=%#x *pEbx=%#x *pEcx=%#x *pEdx=%#x\n",
             pDevIns->pReg->szName, pDevIns->iInstance, *pEax, *pEbx, *pEcx, *pEdx));
}

const PDMDEVHLPR3 g_tstDevPdmDevHlpR3 =
{
    PDM_DEVHLPR3_VERSION,
    pdmR3DevHlp_IOPortRegister,
    pdmR3DevHlp_IOPortRegisterRC,
    pdmR3DevHlp_IOPortRegisterR0,
    pdmR3DevHlp_IOPortDeregister,
    pdmR3DevHlp_MMIORegister,
    pdmR3DevHlp_MMIORegisterRC,
    pdmR3DevHlp_MMIORegisterR0,
    pdmR3DevHlp_MMIODeregister,
    pdmR3DevHlp_MMIO2Register,
    pdmR3DevHlp_MMIOExPreRegister,
    pdmR3DevHlp_MMIOExDeregister,
    pdmR3DevHlp_MMIOExMap,
    pdmR3DevHlp_MMIOExUnmap,
    pdmR3DevHlp_MMIOExReduce,
    pdmR3DevHlp_MMHyperMapMMIO2,
    pdmR3DevHlp_MMIO2MapKernel,
    pdmR3DevHlp_ROMRegister,
    pdmR3DevHlp_ROMProtectShadow,
    pdmR3DevHlp_SSMRegister,
    pdmR3DevHlp_TMTimerCreate,
    pdmR3DevHlp_TMUtcNow,
    pdmR3DevHlp_PhysRead,
    pdmR3DevHlp_PhysWrite,
    pdmR3DevHlp_PhysGCPhys2CCPtr,
    pdmR3DevHlp_PhysGCPhys2CCPtrReadOnly,
    pdmR3DevHlp_PhysReleasePageMappingLock,
    pdmR3DevHlp_PhysReadGCVirt,
    pdmR3DevHlp_PhysWriteGCVirt,
    pdmR3DevHlp_PhysGCPtr2GCPhys,
    pdmR3DevHlp_MMHeapAlloc,
    pdmR3DevHlp_MMHeapAllocZ,
    pdmR3DevHlp_MMHeapFree,
    pdmR3DevHlp_VMState,
    pdmR3DevHlp_VMTeleportedAndNotFullyResumedYet,
    pdmR3DevHlp_VMSetError,
    pdmR3DevHlp_VMSetErrorV,
    pdmR3DevHlp_VMSetRuntimeError,
    pdmR3DevHlp_VMSetRuntimeErrorV,
    pdmR3DevHlp_DBGFStopV,
    pdmR3DevHlp_DBGFInfoRegister,
    pdmR3DevHlp_DBGFRegRegister,
    pdmR3DevHlp_DBGFTraceBuf,
    pdmR3DevHlp_STAMRegister,
    pdmR3DevHlp_STAMRegisterF,
    pdmR3DevHlp_STAMRegisterV,
    pdmR3DevHlp_PCIRegister,
    pdmR3DevHlp_PCIRegisterMsi,
    pdmR3DevHlp_PCIIORegionRegister,
    pdmR3DevHlp_PCISetConfigCallbacks,
    pdmR3DevHlp_PCIPhysRead,
    pdmR3DevHlp_PCIPhysWrite,
    pdmR3DevHlp_PCISetIrq,
    pdmR3DevHlp_PCISetIrqNoWait,
    pdmR3DevHlp_ISASetIrq,
    pdmR3DevHlp_ISASetIrqNoWait,
    pdmR3DevHlp_IoApicSendMsi,
    pdmR3DevHlp_DriverAttach,
    pdmR3DevHlp_DriverDetach,
    pdmR3DevHlp_QueueCreate,
    pdmR3DevHlp_CritSectInit,
    pdmR3DevHlp_CritSectGetNop,
    pdmR3DevHlp_CritSectGetNopR0,
    pdmR3DevHlp_CritSectGetNopRC,
    pdmR3DevHlp_SetDeviceCritSect,
    pdmR3DevHlp_ThreadCreate,
    pdmR3DevHlp_SetAsyncNotification,
    pdmR3DevHlp_AsyncNotificationCompleted,
    pdmR3DevHlp_RTCRegister,
    pdmR3DevHlp_PCIBusRegister,
    pdmR3DevHlp_PICRegister,
    pdmR3DevHlp_APICRegister,
    pdmR3DevHlp_IOAPICRegister,
    pdmR3DevHlp_HPETRegister,
    pdmR3DevHlp_PciRawRegister,
    pdmR3DevHlp_DMACRegister,
    pdmR3DevHlp_DMARegister,
    pdmR3DevHlp_DMAReadMemory,
    pdmR3DevHlp_DMAWriteMemory,
    pdmR3DevHlp_DMASetDREQ,
    pdmR3DevHlp_DMAGetChannelMode,
    pdmR3DevHlp_DMASchedule,
    pdmR3DevHlp_CMOSWrite,
    pdmR3DevHlp_CMOSRead,
    pdmR3DevHlp_AssertEMT,
    pdmR3DevHlp_AssertOther,
    pdmR3DevHlp_LdrGetRCInterfaceSymbols,
    pdmR3DevHlp_LdrGetR0InterfaceSymbols,
    pdmR3DevHlp_CallR0,
    pdmR3DevHlp_VMGetSuspendReason,
    pdmR3DevHlp_VMGetResumeReason,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    pdmR3DevHlp_GetUVM,
    pdmR3DevHlp_GetVM,
    pdmR3DevHlp_GetVMCPU,
    pdmR3DevHlp_GetCurrentCpuId,
    pdmR3DevHlp_RegisterVMMDevHeap,
    pdmR3DevHlp_FirmwareRegister,
    pdmR3DevHlp_VMReset,
    pdmR3DevHlp_VMSuspend,
    pdmR3DevHlp_VMSuspendSaveAndPowerOff,
    pdmR3DevHlp_VMPowerOff,
    pdmR3DevHlp_A20IsEnabled,
    pdmR3DevHlp_A20Set,
    pdmR3DevHlp_GetCpuId,
    pdmR3DevHlp_TMTimeVirtGet,
    pdmR3DevHlp_TMTimeVirtGetFreq,
    pdmR3DevHlp_TMTimeVirtGetNano,
    pdmR3DevHlp_GetSupDrvSession,
    pdmR3DevHlp_QueryGenericUserObject,
    PDM_DEVHLPR3_VERSION /* the end */
};

