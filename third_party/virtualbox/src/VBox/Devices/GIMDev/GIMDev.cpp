/* $Id: GIMDev.cpp $ */
/** @file
 * Guest Interface Manager Device.
 */

/*
 * Copyright (C) 2014-2016 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_GIM
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/gim.h>
#include <VBox/vmm/vm.h>

#include "VBoxDD.h"
#include <iprt/alloc.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>

#define GIMDEV_DEBUG_LUN                998

/**
 * GIM device.
 */
typedef struct GIMDEV
{
    /** Pointer to the device instance - R3 Ptr. */
    PPDMDEVINSR3                    pDevInsR3;
    /** Pointer to the device instance - R0 Ptr. */
    PPDMDEVINSR0                    pDevInsR0;
    /** Pointer to the device instance - RC Ptr. */
    PPDMDEVINSRC                    pDevInsRC;
    /** Alignment. */
    RTRCPTR                         Alignment0;

    /** LUN\#998: The debug interface. */
    PDMIBASE                        IDbgBase;
    /** LUN\#998: The stream port interface. */
    PDMISTREAM                      IDbgStreamPort;
    /** Pointer to the attached base debug driver. */
    R3PTRTYPE(PPDMIBASE)            pDbgDrvBase;
    /** The debug receive thread. */
    RTTHREAD                        hDbgRecvThread;
    /** Flag to indicate shutdown of the debug receive thread. */
    bool volatile                   fDbgRecvThreadShutdown;
    /** The debug setup parameters. */
    GIMDEBUGSETUP                   DbgSetup;
    /** The debug transfer struct. */
    GIMDEBUG                        Dbg;
} GIMDEV;
/** Pointer to the GIM device state. */
typedef GIMDEV *PGIMDEV;
AssertCompileMemberAlignment(GIMDEV, IDbgBase, 8);

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

#ifdef IN_RING3


/* -=-=-=-=-=-=-=-=- PDMIBASE on LUN#GIMDEV_DEBUG_LUN -=-=-=-=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) gimdevR3QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PGIMDEV pThis = RT_FROM_MEMBER(pInterface, GIMDEV, IDbgBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IDbgBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMISTREAM, &pThis->IDbgStreamPort);
    return NULL;
}


static DECLCALLBACK(int) gimDevR3DbgRecvThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF1(hThreadSelf);

    /*
     * Validate.
     */
    PPDMDEVINS pDevIns = (PPDMDEVINS)pvUser;
    AssertReturn(pDevIns, VERR_INVALID_PARAMETER);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    PGIMDEV pThis = PDMINS_2_DATA(pDevIns, PGIMDEV);
    AssertReturn(pThis, VERR_INVALID_POINTER);
    AssertReturn(pThis->DbgSetup.cbDbgRecvBuf, VERR_INTERNAL_ERROR);
    AssertReturn(pThis->Dbg.hDbgRecvThreadSem != NIL_RTSEMEVENTMULTI, VERR_INTERNAL_ERROR_2);
    AssertReturn(pThis->Dbg.pvDbgRecvBuf, VERR_INTERNAL_ERROR_3);

    PVM pVM = PDMDevHlpGetVM(pDevIns);
    AssertReturn(pVM, VERR_INVALID_POINTER);

    PPDMISTREAM pDbgDrvStream = pThis->Dbg.pDbgDrvStream;
    AssertReturn(pDbgDrvStream, VERR_INVALID_POINTER);

    for (;;)
    {
        /*
         * Read incoming debug data.
         */
        size_t cbRead = pThis->DbgSetup.cbDbgRecvBuf;
        int rc = pDbgDrvStream->pfnRead(pDbgDrvStream, pThis->Dbg.pvDbgRecvBuf, &cbRead);
        if (   RT_SUCCESS(rc)
            && cbRead > 0)
        {
            /*
             * Notify the consumer thread.
             */
            if (ASMAtomicReadBool(&pThis->Dbg.fDbgRecvBufRead) == false)
            {
                if (pThis->DbgSetup.pfnDbgRecvBufAvail)
                    pThis->DbgSetup.pfnDbgRecvBufAvail(pVM);
                pThis->Dbg.cbDbgRecvBufRead = cbRead;
                RTSemEventMultiReset(pThis->Dbg.hDbgRecvThreadSem);
                ASMAtomicWriteBool(&pThis->Dbg.fDbgRecvBufRead, true);
            }

            /*
             * Wait until the consumer thread has acknowledged reading of the
             * current buffer or we're asked to shut down.
             *
             * It is important that we do NOT re-invoke 'pfnRead' before the
             * current buffer is consumed, otherwise we risk data corruption.
             */
            while (   ASMAtomicReadBool(&pThis->Dbg.fDbgRecvBufRead) == true
                   && !pThis->fDbgRecvThreadShutdown)
            {
                RTSemEventMultiWait(pThis->Dbg.hDbgRecvThreadSem, RT_INDEFINITE_WAIT);
            }
        }
#ifdef RT_OS_LINUX
        else if (rc == VERR_NET_CONNECTION_REFUSED)
        {
            /*
             * With the current, simplistic PDMISTREAM interface, this is the best we can do.
             * Even using RTSocketSelectOne[Ex] on Linux returns immediately with 'ready-to-read'
             * on localhost UDP sockets that are not connected on the other end.
             */
            /** @todo Fix socket waiting semantics on localhost Linux unconnected UDP sockets. */
            RTThreadSleep(400);
        }
#endif
        else if (   rc != VINF_TRY_AGAIN
                 && rc != VERR_TRY_AGAIN
                 && rc != VERR_NET_CONNECTION_RESET_BY_PEER)
        {
            LogRel(("GIMDev: Debug thread terminating with rc=%Rrc\n", rc));
            break;
        }

        if (pThis->fDbgRecvThreadShutdown)
        {
            LogRel(("GIMDev: Debug thread shutting down\n"));
            break;
        }
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) gimdevR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF2(iInstance, pCfg);
    Assert(iInstance == 0);
    PGIMDEV pThis = PDMINS_2_DATA(pDevIns, PGIMDEV);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Initialize relevant state bits.
     */
    pThis->pDevInsR3  = pDevIns;
    pThis->pDevInsR0  = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC  = PDMDEVINS_2_RCPTR(pDevIns);

    /*
     * Get debug setup requirements from GIM.
     */
    PVM pVM = PDMDevHlpGetVM(pDevIns);
    int rc = GIMR3GetDebugSetup(pVM, &pThis->DbgSetup);
    if (   RT_SUCCESS(rc)
        && pThis->DbgSetup.cbDbgRecvBuf > 0)
    {
        /*
         * Attach the stream driver for the debug connection.
         */
        PPDMISTREAM pDbgDrvStream = NULL;
        pThis->IDbgBase.pfnQueryInterface = gimdevR3QueryInterface;
        rc = PDMDevHlpDriverAttach(pDevIns, GIMDEV_DEBUG_LUN, &pThis->IDbgBase, &pThis->pDbgDrvBase, "GIM Debug Port");
        if (RT_SUCCESS(rc))
        {
            pDbgDrvStream = PDMIBASE_QUERY_INTERFACE(pThis->pDbgDrvBase, PDMISTREAM);
            if (pDbgDrvStream)
                LogRel(("GIMDev: LUN#%u: Debug port configured\n", GIMDEV_DEBUG_LUN));
            else
            {
                LogRel(("GIMDev: LUN#%u: No unit\n", GIMDEV_DEBUG_LUN));
                rc = VERR_INTERNAL_ERROR_2;
            }
        }
        else
        {
            pThis->pDbgDrvBase = NULL;
            LogRel(("GIMDev: LUN#%u: No debug port configured! rc=%Rrc\n", GIMDEV_DEBUG_LUN, rc));
        }

        if (!pDbgDrvStream)
        {
            Assert(rc != VINF_SUCCESS);
            return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                       N_("Debug port configuration expected when GIM configured with debugging support"));
        }

        void *pvDbgRecvBuf = RTMemAllocZ(pThis->DbgSetup.cbDbgRecvBuf);
        if (RT_UNLIKELY(!pvDbgRecvBuf))
        {
            LogRel(("GIMDev: Failed to alloc %u bytes for debug receive buffer\n", pThis->DbgSetup.cbDbgRecvBuf));
            return VERR_NO_MEMORY;
        }

        /*
         * Update the shared debug struct.
         */
        pThis->Dbg.pDbgDrvStream    = pDbgDrvStream;
        pThis->Dbg.pvDbgRecvBuf     = pvDbgRecvBuf;
        pThis->Dbg.cbDbgRecvBufRead = 0;
        pThis->Dbg.fDbgRecvBufRead  = false;

        /*
         * Create the sempahore and the debug receive thread itself.
         */
        rc = RTSemEventMultiCreate(&pThis->Dbg.hDbgRecvThreadSem);
        if (RT_SUCCESS(rc))
        {
            rc = RTThreadCreate(&pThis->hDbgRecvThread, gimDevR3DbgRecvThread, pDevIns, 0 /*cbStack*/, RTTHREADTYPE_IO,
                                RTTHREADFLAGS_WAITABLE, "GIMDebugRecv");
            if (RT_FAILURE(rc))
            {
                RTSemEventMultiDestroy(pThis->Dbg.hDbgRecvThreadSem);
                pThis->Dbg.hDbgRecvThreadSem = NIL_RTSEMEVENTMULTI;

                RTMemFree(pThis->Dbg.pvDbgRecvBuf);
                pThis->Dbg.pvDbgRecvBuf = NULL;
                return rc;
            }
        }
        else
            return rc;
    }

    /*
     * Register this device with the GIM component.
     */
    GIMR3GimDeviceRegister(pVM, pDevIns, pThis->DbgSetup.cbDbgRecvBuf ? &pThis->Dbg : NULL);

    /*
     * Get the MMIO2 regions from the GIM provider.
     */
    uint32_t cRegions = 0;
    PGIMMMIO2REGION pRegionsR3 = GIMR3GetMmio2Regions(pVM, &cRegions);
    if (   cRegions
        && pRegionsR3)
    {
        /*
         * Register the MMIO2 regions.
         */
        PGIMMMIO2REGION pCur = pRegionsR3;
        for (uint32_t i = 0; i < cRegions; i++, pCur++)
        {
            Assert(!pCur->fRegistered);
            rc = PDMDevHlpMMIO2Register(pDevIns, NULL, pCur->iRegion, pCur->cbRegion, 0 /* fFlags */, &pCur->pvPageR3,
                                        pCur->szDescription);
            if (RT_FAILURE(rc))
                return rc;

            pCur->fRegistered = true;

#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE)
            RTR0PTR pR0Mapping = 0;
            rc = PDMDevHlpMMIO2MapKernel(pDevIns, NULL, pCur->iRegion, 0 /* off */, pCur->cbRegion, pCur->szDescription,
                                         &pR0Mapping);
            AssertLogRelMsgRCReturn(rc, ("PDMDevHlpMapMMIO2IntoR0(%#x,) -> %Rrc\n", pCur->cbRegion, rc), rc);
            pCur->pvPageR0 = pR0Mapping;
#else
            pCur->pvPageR0 = (RTR0PTR)pCur->pvPageR3;
#endif

            /*
             * Map into RC if required.
             */
            if (pCur->fRCMapping)
            {
                RTRCPTR pRCMapping = 0;
                rc = PDMDevHlpMMHyperMapMMIO2(pDevIns, NULL, pCur->iRegion, 0 /* off */, pCur->cbRegion, pCur->szDescription,
                                              &pRCMapping);
                AssertLogRelMsgRCReturn(rc, ("PDMDevHlpMMHyperMapMMIO2(%#x,) -> %Rrc\n", pCur->cbRegion, rc), rc);
                pCur->pvPageRC = pRCMapping;
            }
            else
                pCur->pvPageRC = NIL_RTRCPTR;

            LogRel(("GIMDev: Registered %s\n", pCur->szDescription));
        }
    }

    /** @todo Register SSM: PDMDevHlpSSMRegister(). */
    /** @todo Register statistics: STAM_REG(). */
    /** @todo Register DBGFInfo: PDMDevHlpDBGFInfoRegister(). */

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) gimdevR3Destruct(PPDMDEVINS pDevIns)
{
    PGIMDEV  pThis    = PDMINS_2_DATA(pDevIns, PGIMDEV);
    PVM      pVM      = PDMDevHlpGetVM(pDevIns);
    uint32_t cRegions = 0;

    PGIMMMIO2REGION pCur = GIMR3GetMmio2Regions(pVM, &cRegions);
    for (uint32_t i = 0; i < cRegions; i++, pCur++)
    {
        int rc = PDMDevHlpMMIOExDeregister(pDevIns, NULL, pCur->iRegion);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Signal and wait for the debug thread to terminate.
     */
    if (pThis->hDbgRecvThread != NIL_RTTHREAD)
    {
        pThis->fDbgRecvThreadShutdown = true;
        if (pThis->Dbg.hDbgRecvThreadSem != NIL_RTSEMEVENT)
            RTSemEventMultiSignal(pThis->Dbg.hDbgRecvThreadSem);

        int rc = RTThreadWait(pThis->hDbgRecvThread, 20000, NULL /*prc*/);
        if (RT_SUCCESS(rc))
            pThis->hDbgRecvThread = NIL_RTTHREAD;
        else
        {
            LogRel(("GIMDev: Debug thread did not terminate, rc=%Rrc!\n", rc));
            return VERR_RESOURCE_BUSY;
        }
    }

    /*
     * Now clean up the semaphore & buffer now that the thread is gone.
     */
    if (pThis->Dbg.hDbgRecvThreadSem != NIL_RTSEMEVENT)
    {
        RTSemEventMultiDestroy(pThis->Dbg.hDbgRecvThreadSem);
        pThis->Dbg.hDbgRecvThreadSem = NIL_RTSEMEVENTMULTI;
    }
    if (pThis->Dbg.pvDbgRecvBuf)
    {
        RTMemFree(pThis->Dbg.pvDbgRecvBuf);
        pThis->Dbg.pvDbgRecvBuf = NULL;
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) gimdevR3Relocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    NOREF(pDevIns);
    NOREF(offDelta);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) gimdevR3Reset(PPDMDEVINS pDevIns)
{
    NOREF(pDevIns);
    /* We do not deregister any MMIO2 regions as the regions are expected to be static. */
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceGIMDev =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "GIMDev",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "VirtualBox GIM Device",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_R0 | PDM_DEVREG_FLAGS_RC,
    /* fClass */
    PDM_DEVREG_CLASS_MISC,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(GIMDEV),
    /* pfnConstruct */
    gimdevR3Construct,
    /* pfnDestruct */
    gimdevR3Destruct,
    /* pfnRelocate */
    gimdevR3Relocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    gimdevR3Reset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};
#endif /* IN_RING3 */

#endif  /* VBOX_DEVICE_STRUCT_TESTCASE */

