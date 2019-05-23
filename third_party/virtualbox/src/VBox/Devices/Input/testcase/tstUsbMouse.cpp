/* $Id: tstUsbMouse.cpp $ */
/** @file
 * tstUsbMouse.cpp - testcase USB mouse and tablet devices.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "VBoxDD.h"
#include <VBox/vmm/pdmdrv.h>
#include <iprt/alloc.h>
#include <iprt/stream.h>
#include <iprt/test.h>
#include <iprt/uuid.h>

/** Test mouse driver structure. */
typedef struct DRVTSTMOUSE
{
    /** The USBHID structure. */
    struct USBHID              *pUsbHid;
    /** The base interface for the mouse driver. */
    PDMIBASE                    IBase;
    /** Our mouse connector interface. */
    PDMIMOUSECONNECTOR          IConnector;
    /** The base interface of the attached mouse port. */
    PPDMIBASE                   pDrvBase;
    /** The mouse port interface of the attached mouse port. */
    PPDMIMOUSEPORT              pDrv;
    /** Is relative mode currently supported? */
    bool                        fRel;
    /** Is absolute mode currently supported? */
    bool                        fAbs;
    /** Is multi-touch mode currently supported? */
    bool                        fMT;
} DRVTSTMOUSE;
typedef DRVTSTMOUSE *PDRVTSTMOUSE;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static PDMUSBHLP   g_tstUsbHlp;
/** Global mouse driver variable.
 * @todo To be improved some time. */
static DRVTSTMOUSE g_drvTstMouse;


/** @interface_method_impl{PDMUSBHLPR3,pfnVMSetErrorV} */
static DECLCALLBACK(int) tstVMSetErrorV(PPDMUSBINS pUsbIns, int rc,
                                        RT_SRC_POS_DECL, const char *pszFormat,
                                        va_list va)
{
    RT_NOREF(pUsbIns);
    RTPrintf("Error: %s:%u:%s:", RT_SRC_POS_ARGS);
    RTPrintfV(pszFormat, va);
    return rc;
}

/** @interface_method_impl{PDMUSBHLPR3,pfnDriverAttach} */
/** @todo We currently just take the driver interface from the global
 * variable.  This is sufficient for a unit test but still a bit sad. */
static DECLCALLBACK(int) tstDriverAttach(PPDMUSBINS pUsbIns, RTUINT iLun, PPDMIBASE pBaseInterface,
                                         PPDMIBASE *ppBaseInterface, const char *pszDesc)
{
    RT_NOREF3(pUsbIns, iLun, pszDesc);
    g_drvTstMouse.pDrvBase = pBaseInterface;
    g_drvTstMouse.pDrv = PDMIBASE_QUERY_INTERFACE(pBaseInterface, PDMIMOUSEPORT);
    *ppBaseInterface = &g_drvTstMouse.IBase;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) tstMouseQueryInterface(PPDMIBASE pInterface,
                                                   const char *pszIID)
{
    PDRVTSTMOUSE pUsbIns = RT_FROM_MEMBER(pInterface, DRVTSTMOUSE, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pUsbIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMOUSECONNECTOR, &pUsbIns->IConnector);
    return NULL;
}


/**
 * @interface_method_impl{PDMIMOUSECONNECTOR,pfnReportModes}
 */
static DECLCALLBACK(void) tstMouseReportModes(PPDMIMOUSECONNECTOR pInterface,
                                              bool fRel, bool fAbs, bool fMT)
{
    PDRVTSTMOUSE pDrv = RT_FROM_MEMBER(pInterface, DRVTSTMOUSE, IConnector);
    pDrv->fRel = fRel;
    pDrv->fAbs = fAbs;
    pDrv->fMT  = fMT;
}


static int tstMouseConstruct(int iInstance, const char *pcszMode,
                             uint8_t u8CoordShift, PPDMUSBINS *ppThis,
                             uint32_t uInstanceVersion = PDM_USBINS_VERSION)
{
    int rc = VERR_NO_MEMORY;
    PPDMUSBINS pUsbIns = (PPDMUSBINS)RTMemAllocZ(  sizeof(*pUsbIns)
                                               + g_UsbHidMou.cbInstance);
    PCFGMNODE pCfg = NULL;
    if (pUsbIns)
    pCfg = CFGMR3CreateTree(NULL);
    if (pCfg)
        rc = CFGMR3InsertString(pCfg, "Mode", pcszMode);
    if (RT_SUCCESS(rc))
        rc = CFGMR3InsertInteger(pCfg, "CoordShift", u8CoordShift);
    if (RT_SUCCESS(rc))
    {
        g_drvTstMouse.pDrv     = NULL;
        g_drvTstMouse.pDrvBase = NULL;
        pUsbIns->u32Version = uInstanceVersion;
        pUsbIns->iInstance  = iInstance;
        pUsbIns->pHlpR3     = &g_tstUsbHlp;
        rc = g_UsbHidMou.pfnConstruct(pUsbIns, iInstance, pCfg, NULL);
        if (RT_SUCCESS(rc))
        {
           *ppThis = pUsbIns;
           return rc;
        }
    }
    /* Failure */
    if (pCfg)
        CFGMR3DestroyTree(pCfg);
    if (pUsbIns)
        RTMemFree(pUsbIns);
    return rc;
}


static void testConstructAndDestruct(RTTEST hTest)
{
    RTTestSub(hTest, "simple construction and destruction");

    /*
     * Normal check first.
     */
    PPDMUSBINS pUsbIns = NULL;
    RTTEST_CHECK_RC(hTest, tstMouseConstruct(0, "relative", 1, &pUsbIns), VINF_SUCCESS);
    if (pUsbIns)
        g_UsbHidMou.pfnDestruct(pUsbIns);

    /*
     * Modify the dev hlp version.
     */
    static struct
    {
        int         rc;
        uint32_t    uInsVersion;
        uint32_t    uHlpVersion;
    } const s_aVersionTests[] =
    {
        {  VERR_PDM_USBHLPR3_VERSION_MISMATCH, PDM_USBINS_VERSION, 0 },
        {  VERR_PDM_USBHLPR3_VERSION_MISMATCH, PDM_USBINS_VERSION, PDM_USBHLP_VERSION - PDM_VERSION_MAKE(0, 1, 0) },
        {  VERR_PDM_USBHLPR3_VERSION_MISMATCH, PDM_USBINS_VERSION, PDM_USBHLP_VERSION + PDM_VERSION_MAKE(0, 1, 0) },
        {  VERR_PDM_USBHLPR3_VERSION_MISMATCH, PDM_USBINS_VERSION, PDM_USBHLP_VERSION + PDM_VERSION_MAKE(0, 1, 1) },
        {  VERR_PDM_USBHLPR3_VERSION_MISMATCH, PDM_USBINS_VERSION, PDM_USBHLP_VERSION + PDM_VERSION_MAKE(1, 0, 0) },
        {  VERR_PDM_USBHLPR3_VERSION_MISMATCH, PDM_USBINS_VERSION, PDM_USBHLP_VERSION - PDM_VERSION_MAKE(1, 0, 0) },
        {  VINF_SUCCESS,                       PDM_USBINS_VERSION, PDM_USBHLP_VERSION + PDM_VERSION_MAKE(0, 0, 1) },
        {  VERR_PDM_USBINS_VERSION_MISMATCH,   PDM_USBINS_VERSION - PDM_VERSION_MAKE(0, 1, 0), PDM_USBHLP_VERSION },
        {  VERR_PDM_USBINS_VERSION_MISMATCH,   PDM_USBINS_VERSION + PDM_VERSION_MAKE(0, 1, 0), PDM_USBHLP_VERSION },
        {  VERR_PDM_USBINS_VERSION_MISMATCH,   PDM_USBINS_VERSION + PDM_VERSION_MAKE(0, 1, 1), PDM_USBHLP_VERSION },
        {  VERR_PDM_USBINS_VERSION_MISMATCH,   PDM_USBINS_VERSION + PDM_VERSION_MAKE(1, 0, 0), PDM_USBHLP_VERSION },
        {  VERR_PDM_USBINS_VERSION_MISMATCH,   PDM_USBINS_VERSION - PDM_VERSION_MAKE(1, 0, 0), PDM_USBHLP_VERSION },
        {  VINF_SUCCESS,                       PDM_USBINS_VERSION + PDM_VERSION_MAKE(0, 0, 1), PDM_USBHLP_VERSION },
        {  VINF_SUCCESS,
           PDM_USBINS_VERSION + PDM_VERSION_MAKE(0, 0, 1),         PDM_USBHLP_VERSION + PDM_VERSION_MAKE(0, 0, 1) },
    };
    bool const fSavedMayPanic = RTAssertSetMayPanic(false);
    bool const fSavedQuiet    = RTAssertSetQuiet(true);
    for (unsigned i = 0; i < RT_ELEMENTS(s_aVersionTests); i++)
    {
        g_tstUsbHlp.u32Version = g_tstUsbHlp.u32TheEnd = s_aVersionTests[i].uHlpVersion;
        pUsbIns = NULL;
        RTTEST_CHECK_RC(hTest, tstMouseConstruct(0, "relative", 1, &pUsbIns, s_aVersionTests[i].uInsVersion),
                        s_aVersionTests[i].rc);
    }
    RTAssertSetMayPanic(fSavedMayPanic);
    RTAssertSetQuiet(fSavedQuiet);

    g_tstUsbHlp.u32Version = g_tstUsbHlp.u32TheEnd = PDM_USBHLP_VERSION;
}


static void testSendPositionRel(RTTEST hTest)
{
    PPDMUSBINS pUsbIns = NULL;
    VUSBURB Urb;
    RTTestSub(hTest, "sending a relative position event");
    int rc = tstMouseConstruct(0, "relative", 1, &pUsbIns);
    RT_ZERO(Urb);
    if (RT_SUCCESS(rc))
        rc = g_UsbHidMou.pfnUsbReset(pUsbIns, false);
    if (RT_SUCCESS(rc) && !g_drvTstMouse.pDrv)
        rc = VERR_PDM_MISSING_INTERFACE;
    RTTEST_CHECK_RC_OK(hTest, rc);
    if (RT_SUCCESS(rc))
    {
        g_drvTstMouse.pDrv->pfnPutEvent(g_drvTstMouse.pDrv, 123, -16, 1, -1, 3);
        Urb.EndPt = 0x01;
        rc = g_UsbHidMou.pfnUrbQueue(pUsbIns, &Urb);
    }
    if (RT_SUCCESS(rc))
    {
        PVUSBURB pUrb = g_UsbHidMou.pfnUrbReap(pUsbIns, 0);
        if (pUrb)
        {
            if (pUrb == &Urb)
            {
                if (   Urb.abData[0] != 3    /* Buttons */
                    || Urb.abData[1] != 123  /* x */
                    || Urb.abData[2] != 240  /* 256 - y */
                    || Urb.abData[3] != 255  /* z */)
                    rc = VERR_GENERAL_FAILURE;
            }
            else
                rc = VERR_GENERAL_FAILURE;
        }
        else
            rc = VERR_GENERAL_FAILURE;
    }
    RTTEST_CHECK_RC_OK(hTest, rc);
    if (pUsbIns)
        g_UsbHidMou.pfnDestruct(pUsbIns);
}


static void testSendPositionAbs(RTTEST hTest)
{
    PPDMUSBINS pUsbIns = NULL;
    VUSBURB Urb;
    RTTestSub(hTest, "sending an absolute position event");
    int rc = tstMouseConstruct(0, "absolute", 1, &pUsbIns);
    RT_ZERO(Urb);
    if (RT_SUCCESS(rc))
    {
        rc = g_UsbHidMou.pfnUsbReset(pUsbIns, false);
    }
    if (RT_SUCCESS(rc))
    {
        if (g_drvTstMouse.pDrv)
            g_drvTstMouse.pDrv->pfnPutEventAbs(g_drvTstMouse.pDrv, 300, 200, 1,
                                               3, 3);
        else
            rc = VERR_PDM_MISSING_INTERFACE;
    }
    if (RT_SUCCESS(rc))
    {
        Urb.EndPt = 0x01;
        rc = g_UsbHidMou.pfnUrbQueue(pUsbIns, &Urb);
    }
    if (RT_SUCCESS(rc))
    {
        PVUSBURB pUrb = g_UsbHidMou.pfnUrbReap(pUsbIns, 0);
        if (pUrb)
        {
            if (pUrb == &Urb)
            {
                if (   Urb.abData[0] != 3                  /* Buttons */
                    || (int8_t)Urb.abData[1] != -1         /* dz */
                    || (int8_t)Urb.abData[2] != -3         /* dw */
                    || *(uint16_t *)&Urb.abData[4] != 150  /* x >> 1 */
                    || *(uint16_t *)&Urb.abData[6] != 100  /* y >> 1 */)
                    rc = VERR_GENERAL_FAILURE;
            }
            else
                rc = VERR_GENERAL_FAILURE;
        }
        else
            rc = VERR_GENERAL_FAILURE;
    }
    RTTEST_CHECK_RC_OK(hTest, rc);
    if (pUsbIns)
        g_UsbHidMou.pfnDestruct(pUsbIns);
}

#if 0
/** @todo PDM interface was updated. This is not working anymore. */
static void testSendPositionMT(RTTEST hTest)
{
    PPDMUSBINS pUsbIns = NULL;
    VUSBURB Urb;
    RTTestSub(hTest, "sending a multi-touch position event");
    int rc = tstMouseConstruct(0, "multitouch", 1, &pUsbIns);
    RT_ZERO(Urb);
    if (RT_SUCCESS(rc))
    {
        rc = g_UsbHidMou.pfnUsbReset(pUsbIns, false);
    }
    if (RT_SUCCESS(rc))
    {
        if (g_drvTstMouse.pDrv)
            g_drvTstMouse.pDrv->pfnPutEventMT(g_drvTstMouse.pDrv, 300, 200, 2,
                                              3);
        else
            rc = VERR_PDM_MISSING_INTERFACE;
    }
    if (RT_SUCCESS(rc))
    {
        Urb.EndPt = 0x01;
        rc = g_UsbHidMou.pfnUrbQueue(pUsbIns, &Urb);
    }
    if (RT_SUCCESS(rc))
    {
        PVUSBURB pUrb = g_UsbHidMou.pfnUrbReap(pUsbIns, 0);
        if (pUrb)
        {
            if (pUrb == &Urb)
            {
                if (   Urb.abData[0] != 1                  /* Report ID */
                    || Urb.abData[1] != 3                  /* Contact flags */
                    || *(uint16_t *)&Urb.abData[2] != 150  /* x >> 1 */
                    || *(uint16_t *)&Urb.abData[4] != 100  /* y >> 1 */
                    || Urb.abData[6] != 2                  /* Contact number */)
                    rc = VERR_GENERAL_FAILURE;
            }
            else
                rc = VERR_GENERAL_FAILURE;
        }
        else
            rc = VERR_GENERAL_FAILURE;
    }
    RTTEST_CHECK_RC_OK(hTest, rc);
    if (pUsbIns)
        g_UsbHidMou.pfnDestruct(pUsbIns);
}
#endif

int main()
{
    /*
     * Init the runtime, test and say hello.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstUsbMouse", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);
    /* Set up our faked PDMUSBHLP interface. */
    g_tstUsbHlp.u32Version      = PDM_USBHLP_VERSION;
    g_tstUsbHlp.pfnVMSetErrorV  = tstVMSetErrorV;
    g_tstUsbHlp.pfnDriverAttach = tstDriverAttach;
    g_tstUsbHlp.u32TheEnd       = PDM_USBHLP_VERSION;
    /* Set up our global mouse driver */
    g_drvTstMouse.IBase.pfnQueryInterface = tstMouseQueryInterface;
    g_drvTstMouse.IConnector.pfnReportModes = tstMouseReportModes;

    /*
     * Run the tests.
     */
    testConstructAndDestruct(hTest);
    testSendPositionRel(hTest);
    testSendPositionAbs(hTest);
    /* testSendPositionMT(hTest); */
    return RTTestSummaryAndDestroy(hTest);
}
