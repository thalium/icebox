/* $Id: PDMDevice.cpp $ */
/** @file
 * PDM - Pluggable Device and Driver Manager, Device parts.
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
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/apic.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/vmm/vmm.h>

#include <VBox/version.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/alloc.h>
#include <iprt/alloca.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/path.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Internal callback structure pointer.
 * The main purpose is to define the extra data we associate
 * with PDMDEVREGCB so we can find the VM instance and so on.
 */
typedef struct PDMDEVREGCBINT
{
    /** The callback structure. */
    PDMDEVREGCB     Core;
    /** A bit of padding. */
    uint32_t        u32[4];
    /** VM Handle. */
    PVM             pVM;
    /** Pointer to the configuration node the registrations should be
     * associated with.  Can be NULL. */
    PCFGMNODE       pCfgNode;
} PDMDEVREGCBINT;
/** Pointer to a PDMDEVREGCBINT structure. */
typedef PDMDEVREGCBINT *PPDMDEVREGCBINT;
/** Pointer to a const PDMDEVREGCBINT structure. */
typedef const PDMDEVREGCBINT *PCPDMDEVREGCBINT;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)    pdmR3DevReg_Register(PPDMDEVREGCB pCallbacks, PCPDMDEVREG pReg);
static int                  pdmR3DevLoadModules(PVM pVM);
static int                  pdmR3DevLoad(PVM pVM, PPDMDEVREGCBINT pRegCB, const char *pszFilename, const char *pszName);




/**
 * This function will initialize the devices for this VM instance.
 *
 *
 * First of all this mean loading the builtin device and letting them
 * register themselves. Beyond that any additional device modules are
 * loaded and called for registration.
 *
 * Then the device configuration is enumerated, the instantiation order
 * is determined, and finally they are instantiated.
 *
 * After all devices have been successfully instantiated the primary
 * PCI Bus device is called to emulate the PCI BIOS, i.e. making the
 * resource assignments. If there is no PCI device, this step is of course
 * skipped.
 *
 * Finally the init completion routines of the instantiated devices
 * are called.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
int pdmR3DevInit(PVM pVM)
{
    LogFlow(("pdmR3DevInit:\n"));

    AssertRelease(!(RT_OFFSETOF(PDMDEVINS, achInstanceData) & 15));
    AssertRelease(sizeof(pVM->pdm.s.pDevInstances->Internal.s) <= sizeof(pVM->pdm.s.pDevInstances->Internal.padding));

    /*
     * Load device modules.
     */
    int rc = pdmR3DevLoadModules(pVM);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_USB
    /* ditto for USB Devices. */
    rc = pdmR3UsbLoadModules(pVM);
    if (RT_FAILURE(rc))
        return rc;
#endif

    /*
     * Get the RC & R0 devhlps and create the devhlp R3 task queue.
     */
    PCPDMDEVHLPRC pHlpRC = NIL_RTRCPTR;
    if (!HMIsEnabled(pVM))
    {
        rc = PDMR3LdrGetSymbolRC(pVM, NULL, "g_pdmRCDevHlp", &pHlpRC);
        AssertReleaseRCReturn(rc, rc);
    }

    PCPDMDEVHLPR0 pHlpR0;
    rc = PDMR3LdrGetSymbolR0(pVM, NULL, "g_pdmR0DevHlp", &pHlpR0);
    AssertReleaseRCReturn(rc, rc);

    rc = PDMR3QueueCreateInternal(pVM, sizeof(PDMDEVHLPTASK), 8, 0, pdmR3DevHlpQueueConsumer, true, "DevHlp",
                                  &pVM->pdm.s.pDevHlpQueueR3);
    AssertRCReturn(rc, rc);
    pVM->pdm.s.pDevHlpQueueR0 = PDMQueueR0Ptr(pVM->pdm.s.pDevHlpQueueR3);
    pVM->pdm.s.pDevHlpQueueRC = PDMQueueRCPtr(pVM->pdm.s.pDevHlpQueueR3);


    /*
     *
     * Enumerate the device instance configurations
     * and come up with a instantiation order.
     *
     */
    /* Switch to /Devices, which contains the device instantiations. */
    PCFGMNODE pDevicesNode = CFGMR3GetChild(CFGMR3GetRoot(pVM), "Devices");

    /*
     * Count the device instances.
     */
    PCFGMNODE pCur;
    PCFGMNODE pInstanceNode;
    unsigned cDevs = 0;
    for (pCur = CFGMR3GetFirstChild(pDevicesNode); pCur; pCur = CFGMR3GetNextChild(pCur))
        for (pInstanceNode = CFGMR3GetFirstChild(pCur); pInstanceNode; pInstanceNode = CFGMR3GetNextChild(pInstanceNode))
            cDevs++;
    if (!cDevs)
    {
        Log(("PDM: No devices were configured!\n"));
        return VINF_SUCCESS;
    }
    Log2(("PDM: cDevs=%u\n", cDevs));

    /*
     * Collect info on each device instance.
     */
    struct DEVORDER
    {
        /** Configuration node. */
        PCFGMNODE   pNode;
        /** Pointer to device. */
        PPDMDEV     pDev;
        /** Init order. */
        uint32_t    u32Order;
        /** VBox instance number. */
        uint32_t    iInstance;
    } *paDevs = (struct DEVORDER *)alloca(sizeof(paDevs[0]) * (cDevs + 1)); /* (One extra for swapping) */
    Assert(paDevs);
    unsigned i = 0;
    for (pCur = CFGMR3GetFirstChild(pDevicesNode); pCur; pCur = CFGMR3GetNextChild(pCur))
    {
        /* Get the device name. */
        char szName[sizeof(paDevs[0].pDev->pReg->szName)];
        rc = CFGMR3GetName(pCur, szName, sizeof(szName));
        AssertMsgRCReturn(rc, ("Configuration error: device name is too long (or something)! rc=%Rrc\n", rc), rc);

        /* Find the device. */
        PPDMDEV pDev = pdmR3DevLookup(pVM, szName);
        AssertLogRelMsgReturn(pDev, ("Configuration error: device '%s' not found!\n", szName), VERR_PDM_DEVICE_NOT_FOUND);

        /* Configured priority or use default based on device class? */
        uint32_t u32Order;
        rc = CFGMR3QueryU32(pCur, "Priority", &u32Order);
        if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        {
            uint32_t u32 = pDev->pReg->fClass;
            for (u32Order = 1; !(u32 & u32Order); u32Order <<= 1)
                /* nop */;
        }
        else
            AssertMsgRCReturn(rc, ("Configuration error: reading \"Priority\" for the '%s' device failed rc=%Rrc!\n", szName, rc), rc);

        /* Enumerate the device instances. */
        uint32_t const iStart = i;
        for (pInstanceNode = CFGMR3GetFirstChild(pCur); pInstanceNode; pInstanceNode = CFGMR3GetNextChild(pInstanceNode))
        {
            paDevs[i].pNode = pInstanceNode;
            paDevs[i].pDev = pDev;
            paDevs[i].u32Order = u32Order;

            /* Get the instance number. */
            char szInstance[32];
            rc = CFGMR3GetName(pInstanceNode, szInstance, sizeof(szInstance));
            AssertMsgRCReturn(rc, ("Configuration error: instance name is too long (or something)! rc=%Rrc\n", rc), rc);
            char *pszNext = NULL;
            rc = RTStrToUInt32Ex(szInstance, &pszNext, 0, &paDevs[i].iInstance);
            AssertMsgRCReturn(rc, ("Configuration error: RTStrToInt32Ex failed on the instance name '%s'! rc=%Rrc\n", szInstance, rc), rc);
            AssertMsgReturn(!*pszNext, ("Configuration error: the instance name '%s' isn't all digits. (%s)\n", szInstance, pszNext), VERR_INVALID_PARAMETER);

            /* next instance */
            i++;
        }

        /* check the number of instances */
        if (i - iStart > pDev->pReg->cMaxInstances)
            AssertLogRelMsgFailedReturn(("Configuration error: Too many instances of %s was configured: %u, max %u\n",
                                         szName, i - iStart, pDev->pReg->cMaxInstances),
                                        VERR_PDM_TOO_MANY_DEVICE_INSTANCES);
    } /* devices */
    Assert(i == cDevs);

    /*
     * Sort (bubble) the device array ascending on u32Order and instance number
     * for a device.
     */
    unsigned c = cDevs - 1;
    while (c)
    {
        unsigned j = 0;
        for (i = 0; i < c; i++)
            if (   paDevs[i].u32Order > paDevs[i + 1].u32Order
                || (   paDevs[i].u32Order  == paDevs[i + 1].u32Order
                    && paDevs[i].iInstance >  paDevs[i + 1].iInstance
                    && paDevs[i].pDev      == paDevs[i + 1].pDev) )
            {
                paDevs[cDevs] = paDevs[i + 1];
                paDevs[i + 1] = paDevs[i];
                paDevs[i] = paDevs[cDevs];
                j = i;
            }
        c = j;
    }


    /*
     *
     * Instantiate the devices.
     *
     */
    for (i = 0; i < cDevs; i++)
    {
        /*
         * Gather a bit of config.
         */
        /* trusted */
        bool fTrusted;
        rc = CFGMR3QueryBool(paDevs[i].pNode, "Trusted", &fTrusted);
        if (rc == VERR_CFGM_VALUE_NOT_FOUND)
            fTrusted = false;
        else if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("configuration error: failed to query boolean \"Trusted\", rc=%Rrc\n", rc));
            return rc;
        }
        /* config node */
        PCFGMNODE pConfigNode = CFGMR3GetChild(paDevs[i].pNode, "Config");
        if (!pConfigNode)
        {
            rc = CFGMR3InsertNode(paDevs[i].pNode, "Config", &pConfigNode);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("Failed to create Config node! rc=%Rrc\n", rc));
                return rc;
            }
        }
        CFGMR3SetRestrictedRoot(pConfigNode);

        /*
         * Allocate the device instance and critical section.
         */
        AssertReturn(paDevs[i].pDev->cInstances < paDevs[i].pDev->pReg->cMaxInstances, VERR_PDM_TOO_MANY_DEVICE_INSTANCES);
        size_t cb = RT_OFFSETOF(PDMDEVINS, achInstanceData[paDevs[i].pDev->pReg->cbInstance]);
        cb = RT_ALIGN_Z(cb, 16);
        PPDMDEVINS pDevIns;
        if (paDevs[i].pDev->pReg->fFlags & (PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0))
            rc = MMR3HyperAllocOnceNoRel(pVM, cb, 0, MM_TAG_PDM_DEVICE, (void **)&pDevIns);
        else
            rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_DEVICE, cb, (void **)&pDevIns);
        AssertLogRelMsgRCReturn(rc,
                                ("Failed to allocate %d bytes of instance data for device '%s'. rc=%Rrc\n",
                                cb, paDevs[i].pDev->pReg->szName, rc),
                                rc);
        PPDMCRITSECT pCritSect;
        if (paDevs[i].pDev->pReg->fFlags & (PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0))
            rc = MMHyperAlloc(pVM, sizeof(*pCritSect), 0, MM_TAG_PDM_DEVICE, (void **)&pCritSect);
        else
            rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_DEVICE, sizeof(*pCritSect), (void **)&pCritSect);
        AssertLogRelMsgRCReturn(rc, ("Failed to allocate a critical section for the device (%Rrc)\n",  rc), rc);

        /*
         * Initialize it.
         */
        pDevIns->u32Version                     = PDM_DEVINS_VERSION;
        pDevIns->iInstance                      = paDevs[i].iInstance;
        //pDevIns->Internal.s.pNextR3             = NULL;
        //pDevIns->Internal.s.pPerDeviceNextR3    = NULL;
        pDevIns->Internal.s.pDevR3              = paDevs[i].pDev;
        pDevIns->Internal.s.pVMR3               = pVM;
        pDevIns->Internal.s.pVMR0               = pVM->pVMR0;
        pDevIns->Internal.s.pVMRC               = pVM->pVMRC;
        //pDevIns->Internal.s.pLunsR3             = NULL;
        pDevIns->Internal.s.pCfgHandle          = paDevs[i].pNode;
        //pDevIns->Internal.s.pHeadPciDevR3       = NULL;
        //pDevIns->Internal.s.pHeadPciDevR0       = 0;
        //pDevIns->Internal.s.pHeadPciDevRC       = 0;
        pDevIns->Internal.s.fIntFlags           = PDMDEVINSINT_FLAGS_SUSPENDED;
        //pDevIns->Internal.s.uLastIrqTag         = 0;
        pDevIns->pHlpR3                         = fTrusted ? &g_pdmR3DevHlpTrusted : &g_pdmR3DevHlpUnTrusted;
        pDevIns->pHlpRC                         = pHlpRC;
        pDevIns->pHlpR0                         = pHlpR0;
        pDevIns->pReg                           = paDevs[i].pDev->pReg;
        pDevIns->pCfg                           = pConfigNode;
        //pDevIns->IBase.pfnQueryInterface        = NULL;
        //pDevIns->fTracing                       = 0;
        pDevIns->idTracing                      = ++pVM->pdm.s.idTracingDev;
        pDevIns->pvInstanceDataR3               = &pDevIns->achInstanceData[0];
        pDevIns->pvInstanceDataRC               = pDevIns->pReg->fFlags & PDM_DEVREG_FLAGS_RC
                                                ? MMHyperR3ToRC(pVM, pDevIns->pvInstanceDataR3) : NIL_RTRCPTR;
        pDevIns->pvInstanceDataR0               = pDevIns->pReg->fFlags & PDM_DEVREG_FLAGS_R0
                                                ? MMHyperR3ToR0(pVM, pDevIns->pvInstanceDataR3) : NIL_RTR0PTR;

        pDevIns->pCritSectRoR3                  = pCritSect;
        pDevIns->pCritSectRoRC                  = pDevIns->pReg->fFlags & PDM_DEVREG_FLAGS_RC
                                                ? MMHyperR3ToRC(pVM, pCritSect) : NIL_RTRCPTR;
        pDevIns->pCritSectRoR0                  = pDevIns->pReg->fFlags & PDM_DEVREG_FLAGS_R0
                                                ? MMHyperR3ToR0(pVM, pCritSect) : NIL_RTR0PTR;

        rc = pdmR3CritSectInitDeviceAuto(pVM, pDevIns, pCritSect, RT_SRC_POS,
                                         "%s#%uAuto", pDevIns->pReg->szName, pDevIns->iInstance);
        AssertLogRelRCReturn(rc, rc);

        /*
         * Link it into all the lists.
         */
        /* The global instance FIFO. */
        PPDMDEVINS pPrev1 = pVM->pdm.s.pDevInstances;
        if (!pPrev1)
            pVM->pdm.s.pDevInstances = pDevIns;
        else
        {
            while (pPrev1->Internal.s.pNextR3)
                pPrev1 = pPrev1->Internal.s.pNextR3;
            pPrev1->Internal.s.pNextR3 = pDevIns;
        }

        /* The per device instance FIFO. */
        PPDMDEVINS pPrev2 = paDevs[i].pDev->pInstances;
        if (!pPrev2)
            paDevs[i].pDev->pInstances = pDevIns;
        else
        {
            while (pPrev2->Internal.s.pPerDeviceNextR3)
                pPrev2 = pPrev2->Internal.s.pPerDeviceNextR3;
            pPrev2->Internal.s.pPerDeviceNextR3 = pDevIns;
        }

        /*
         * Call the constructor.
         */
        paDevs[i].pDev->cInstances++;
        Log(("PDM: Constructing device '%s' instance %d...\n", pDevIns->pReg->szName, pDevIns->iInstance));
        rc = pDevIns->pReg->pfnConstruct(pDevIns, pDevIns->iInstance, pDevIns->pCfg);
        if (RT_FAILURE(rc))
        {
            LogRel(("PDM: Failed to construct '%s'/%d! %Rra\n", pDevIns->pReg->szName, pDevIns->iInstance, rc));
            paDevs[i].pDev->cInstances--;
            /* Because we're damn lazy, the destructor will be called even if
               the constructor fails.  So, no unlinking. */
            return rc == VERR_VERSION_MISMATCH ? VERR_PDM_DEVICE_VERSION_MISMATCH : rc;
        }
    } /* for device instances */

#ifdef VBOX_WITH_USB
    /* ditto for USB Devices. */
    rc = pdmR3UsbInstantiateDevices(pVM);
    if (RT_FAILURE(rc))
        return rc;
#endif

    LogFlow(("pdmR3DevInit: returns %Rrc\n", VINF_SUCCESS));
    return VINF_SUCCESS;
}


/**
 * Performs the init complete callback after ring-0 and raw-mode has been
 * initialized.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
int pdmR3DevInitComplete(PVM pVM)
{
    int rc;

    /*
     * Iterate thru the device instances and work the callback.
     */
    for (PPDMDEVINS pDevIns = pVM->pdm.s.pDevInstances; pDevIns; pDevIns = pDevIns->Internal.s.pNextR3)
    {
        if (pDevIns->pReg->pfnInitComplete)
        {
            PDMCritSectEnter(pDevIns->pCritSectRoR3, VERR_IGNORED);
            rc = pDevIns->pReg->pfnInitComplete(pDevIns);
            PDMCritSectLeave(pDevIns->pCritSectRoR3);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("InitComplete on device '%s'/%d failed with rc=%Rrc\n",
                                 pDevIns->pReg->szName, pDevIns->iInstance, rc));
                return rc;
            }
        }
    }

#ifdef VBOX_WITH_USB
    rc = pdmR3UsbVMInitComplete(pVM);
    if (RT_FAILURE(rc))
    {
        Log(("pdmR3DevInit: returns %Rrc\n", rc));
        return rc;
    }
#endif

    LogFlow(("pdmR3DevInit: returns %Rrc\n", VINF_SUCCESS));
    return VINF_SUCCESS;
}


/**
 * Lookups a device structure by name.
 * @internal
 */
PPDMDEV pdmR3DevLookup(PVM pVM, const char *pszName)
{
    size_t cchName = strlen(pszName);
    for (PPDMDEV pDev = pVM->pdm.s.pDevs; pDev; pDev = pDev->pNext)
        if (    pDev->cchName == cchName
            &&  !strcmp(pDev->pReg->szName, pszName))
            return pDev;
    return NULL;
}


/**
 * Loads the device modules.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
static int pdmR3DevLoadModules(PVM pVM)
{
    /*
     * Initialize the callback structure.
     */
    PDMDEVREGCBINT RegCB;
    RegCB.Core.u32Version  = PDM_DEVREG_CB_VERSION;
    RegCB.Core.pfnRegister = pdmR3DevReg_Register;
    RegCB.pVM              = pVM;
    RegCB.pCfgNode         = NULL;

    /*
     * Load the internal VMM APIC device.
     */
    int rc2 = pdmR3DevReg_Register(&RegCB.Core, &g_DeviceAPIC);
    AssertRCReturn(rc2, rc2);

    /*
     * Load the builtin module.
     */
    PCFGMNODE pDevicesNode = CFGMR3GetChild(CFGMR3GetRoot(pVM), "PDM/Devices");
    bool fLoadBuiltin;
    int rc = CFGMR3QueryBool(pDevicesNode, "LoadBuiltin", &fLoadBuiltin);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
        fLoadBuiltin = true;
    else if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: Querying boolean \"LoadBuiltin\" failed with %Rrc\n", rc));
        return rc;
    }
    if (fLoadBuiltin)
    {
        /* make filename */
        char *pszFilename = pdmR3FileR3("VBoxDD", true /*fShared*/);
        if (!pszFilename)
            return VERR_NO_TMP_MEMORY;
        rc = pdmR3DevLoad(pVM, &RegCB, pszFilename, "VBoxDD");
        RTMemTmpFree(pszFilename);
        if (RT_FAILURE(rc))
            return rc;

        /* make filename */
        pszFilename = pdmR3FileR3("VBoxDD2", true /*fShared*/);
        if (!pszFilename)
            return VERR_NO_TMP_MEMORY;
        rc = pdmR3DevLoad(pVM, &RegCB, pszFilename, "VBoxDD2");
        RTMemTmpFree(pszFilename);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Load additional device modules.
     */
    PCFGMNODE pCur;
    for (pCur = CFGMR3GetFirstChild(pDevicesNode); pCur; pCur = CFGMR3GetNextChild(pCur))
    {
        /*
         * Get the name and path.
         */
        char szName[PDMMOD_NAME_LEN];
        rc = CFGMR3GetName(pCur, &szName[0], sizeof(szName));
        if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
        {
            AssertMsgFailed(("configuration error: The module name is too long, cchName=%zu.\n", CFGMR3GetNameLen(pCur)));
            return VERR_PDM_MODULE_NAME_TOO_LONG;
        }
        else if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("CFGMR3GetName -> %Rrc.\n", rc));
            return rc;
        }

        /* the path is optional, if no path the module name + path is used. */
        char szFilename[RTPATH_MAX];
        rc = CFGMR3QueryString(pCur, "Path", &szFilename[0], sizeof(szFilename));
        if (rc == VERR_CFGM_VALUE_NOT_FOUND)
            strcpy(szFilename, szName);
        else if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("configuration error: Failure to query the module path, rc=%Rrc.\n", rc));
            return rc;
        }

        /* prepend path? */
        if (!RTPathHavePath(szFilename))
        {
            char *psz = pdmR3FileR3(szFilename, false /*fShared*/);
            if (!psz)
                return VERR_NO_TMP_MEMORY;
            size_t cch = strlen(psz) + 1;
            if (cch > sizeof(szFilename))
            {
                RTMemTmpFree(psz);
                AssertMsgFailed(("Filename too long! cch=%d '%s'\n", cch, psz));
                return VERR_FILENAME_TOO_LONG;
            }
            memcpy(szFilename, psz, cch);
            RTMemTmpFree(psz);
        }

        /*
         * Load the module and register it's devices.
         */
        RegCB.pCfgNode = pCur;
        rc = pdmR3DevLoad(pVM, &RegCB, szFilename, szName);
        if (RT_FAILURE(rc))
            return rc;
    }

    return VINF_SUCCESS;
}


/**
 * Loads one device module and call the registration entry point.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pRegCB          The registration callback stuff.
 * @param   pszFilename     Module filename.
 * @param   pszName         Module name.
 */
static int pdmR3DevLoad(PVM pVM, PPDMDEVREGCBINT pRegCB, const char *pszFilename, const char *pszName)
{
    /*
     * Load it.
     */
    int rc = pdmR3LoadR3U(pVM->pUVM, pszFilename, pszName);
    if (RT_SUCCESS(rc))
    {
        /*
         * Get the registration export and call it.
         */
        FNPDMVBOXDEVICESREGISTER *pfnVBoxDevicesRegister;
        rc = PDMR3LdrGetSymbolR3(pVM, pszName, "VBoxDevicesRegister", (void **)&pfnVBoxDevicesRegister);
        if (RT_SUCCESS(rc))
        {
            Log(("PDM: Calling VBoxDevicesRegister (%p) of %s (%s)\n", pfnVBoxDevicesRegister, pszName, pszFilename));
            rc = pfnVBoxDevicesRegister(&pRegCB->Core, VBOX_VERSION);
            if (RT_SUCCESS(rc))
                Log(("PDM: Successfully loaded device module %s (%s).\n", pszName, pszFilename));
            else
                AssertMsgFailed(("VBoxDevicesRegister failed with rc=%Rrc for module %s (%s)\n", rc, pszName, pszFilename));
        }
        else
        {
            AssertMsgFailed(("Failed to locate 'VBoxDevicesRegister' in %s (%s) rc=%Rrc\n", pszName, pszFilename, rc));
            if (rc == VERR_SYMBOL_NOT_FOUND)
                rc = VERR_PDM_NO_REGISTRATION_EXPORT;
        }
    }
    else
        AssertMsgFailed(("Failed to load %s %s!\n", pszFilename, pszName));
    return rc;
}


/**
 * @interface_method_impl{PDMDEVREGCB,pfnRegister}
 */
static DECLCALLBACK(int) pdmR3DevReg_Register(PPDMDEVREGCB pCallbacks, PCPDMDEVREG pReg)
{
    /*
     * Validate the registration structure.
     */
    Assert(pReg);
    AssertMsgReturn(pReg->u32Version == PDM_DEVREG_VERSION,
                    ("Unknown struct version %#x!\n", pReg->u32Version),
                    VERR_PDM_UNKNOWN_DEVREG_VERSION);

    AssertMsgReturn(    pReg->szName[0]
                    &&  strlen(pReg->szName) < sizeof(pReg->szName)
                    &&  pdmR3IsValidName(pReg->szName),
                    ("Invalid name '%.*s'\n", sizeof(pReg->szName), pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(   !(pReg->fFlags & PDM_DEVREG_FLAGS_RC)
                    || (   pReg->szRCMod[0]
                        && strlen(pReg->szRCMod) < sizeof(pReg->szRCMod)),
                    ("Invalid GC module name '%s' - (Device %s)\n", pReg->szRCMod, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(   !(pReg->fFlags & PDM_DEVREG_FLAGS_R0)
                    || (   pReg->szR0Mod[0]
                        && strlen(pReg->szR0Mod) < sizeof(pReg->szR0Mod)),
                    ("Invalid R0 module name '%s' - (Device %s)\n", pReg->szR0Mod, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_HOST_BITS_MASK) == PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT,
                    ("Invalid host bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_HOST_BITS);
    AssertMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK),
                    ("Invalid guest bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->fClass,
                    ("No class! (Device %s)\n", pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->cMaxInstances > 0,
                    ("Max instances %u! (Device %s)\n", pReg->cMaxInstances, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->cbInstance <= (uint32_t)(pReg->fFlags & (PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0)  ? 96 * _1K : _1M),
                    ("Instance size %d bytes! (Device %s)\n", pReg->cbInstance, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->pfnConstruct,
                    ("No constructor! (Device %s)\n", pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK) == PDM_DEVREG_FLAGS_GUEST_BITS_DEFAULT,
                          ("PDM: Rejected device '%s' because it didn't match the guest bits.\n", pReg->szName),
                          VERR_PDM_INVALID_DEVICE_GUEST_BITS);
    AssertLogRelMsg(pReg->u32VersionEnd == PDM_DEVREG_VERSION,
                    ("u32VersionEnd=%#x, expected %#x. (szName=%s)\n",
                     pReg->u32VersionEnd, PDM_DEVREG_VERSION, pReg->szName));

    /*
     * Check for duplicate and find FIFO entry at the same time.
     */
    PCPDMDEVREGCBINT pRegCB = (PCPDMDEVREGCBINT)pCallbacks;
    PPDMDEV pDevPrev = NULL;
    PPDMDEV pDev = pRegCB->pVM->pdm.s.pDevs;
    for (; pDev; pDevPrev = pDev, pDev = pDev->pNext)
        AssertMsgReturn(strcmp(pDev->pReg->szName, pReg->szName),
                        ("Device '%s' already exists\n", pReg->szName),
                        VERR_PDM_DEVICE_NAME_CLASH);

    /*
     * Allocate new device structure, initialize and insert it into the list.
     */
    int rc;
    pDev = (PPDMDEV)MMR3HeapAlloc(pRegCB->pVM, MM_TAG_PDM_DEVICE, sizeof(*pDev));
    if (pDev)
    {
        pDev->pNext      = NULL;
        pDev->cInstances = 0;
        pDev->pInstances = NULL;
        pDev->pReg       = pReg;
        pDev->cchName    = (uint32_t)strlen(pReg->szName);
        rc = CFGMR3QueryStringAllocDef(    pRegCB->pCfgNode, "RCSearchPath", &pDev->pszRCSearchPath, NULL);
        if (RT_SUCCESS(rc))
            rc = CFGMR3QueryStringAllocDef(pRegCB->pCfgNode, "R0SearchPath", &pDev->pszR0SearchPath, NULL);
        if (RT_SUCCESS(rc))
        {
            if (pDevPrev)
                pDevPrev->pNext = pDev;
            else
                pRegCB->pVM->pdm.s.pDevs = pDev;
            Log(("PDM: Registered device '%s'\n", pReg->szName));
            return VINF_SUCCESS;
        }

        MMR3HeapFree(pDev);
    }
    else
        rc = VERR_NO_MEMORY;
    return rc;
}


/**
 * Locates a LUN.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pszDevice       Device name.
 * @param   iInstance       Device instance.
 * @param   iLun            The Logical Unit to obtain the interface of.
 * @param   ppLun           Where to store the pointer to the LUN if found.
 * @thread  Try only do this in EMT...
 */
int pdmR3DevFindLun(PVM pVM, const char *pszDevice, unsigned iInstance, unsigned iLun, PPPDMLUN ppLun)
{
    /*
     * Iterate registered devices looking for the device.
     */
    size_t cchDevice = strlen(pszDevice);
    for (PPDMDEV pDev = pVM->pdm.s.pDevs; pDev; pDev = pDev->pNext)
    {
        if (    pDev->cchName == cchDevice
            &&  !memcmp(pDev->pReg->szName, pszDevice, cchDevice))
        {
            /*
             * Iterate device instances.
             */
            for (PPDMDEVINS pDevIns = pDev->pInstances; pDevIns; pDevIns = pDevIns->Internal.s.pPerDeviceNextR3)
            {
                if (pDevIns->iInstance == iInstance)
                {
                    /*
                     * Iterate luns.
                     */
                    for (PPDMLUN pLun = pDevIns->Internal.s.pLunsR3; pLun; pLun = pLun->pNext)
                    {
                        if (pLun->iLun == iLun)
                        {
                            *ppLun = pLun;
                            return VINF_SUCCESS;
                        }
                    }
                    return VERR_PDM_LUN_NOT_FOUND;
                }
            }
            return VERR_PDM_DEVICE_INSTANCE_NOT_FOUND;
        }
    }
    return VERR_PDM_DEVICE_NOT_FOUND;
}


/**
 * Attaches a preconfigured driver to an existing device instance.
 *
 * This is used to change drivers and suchlike at runtime.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pszDevice       Device name.
 * @param   iInstance       Device instance.
 * @param   iLun            The Logical Unit to obtain the interface of.
 * @param   fFlags          Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @param   ppBase          Where to store the base interface pointer. Optional.
 * @thread  EMT
 */
VMMR3DECL(int) PDMR3DeviceAttach(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, uint32_t fFlags, PPPDMIBASE ppBase)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    VM_ASSERT_EMT(pVM);
    LogFlow(("PDMR3DeviceAttach: pszDevice=%p:{%s} iInstance=%d iLun=%d fFlags=%#x ppBase=%p\n",
             pszDevice, pszDevice, iInstance, iLun, fFlags, ppBase));

    /*
     * Find the LUN in question.
     */
    PPDMLUN pLun;
    int rc = pdmR3DevFindLun(pVM, pszDevice, iInstance, iLun, &pLun);
    if (RT_SUCCESS(rc))
    {
        /*
         * Can we attach anything at runtime?
         */
        PPDMDEVINS pDevIns = pLun->pDevIns;
        if (pDevIns->pReg->pfnAttach)
        {
            if (!pLun->pTop)
            {
                PDMCritSectEnter(pDevIns->pCritSectRoR3, VERR_IGNORED);
                rc = pDevIns->pReg->pfnAttach(pDevIns, iLun, fFlags);
                PDMCritSectLeave(pDevIns->pCritSectRoR3);
            }
            else
                rc = VERR_PDM_DRIVER_ALREADY_ATTACHED;
        }
        else
            rc = VERR_PDM_DEVICE_NO_RT_ATTACH;

        if (ppBase)
            *ppBase = pLun->pTop ? &pLun->pTop->IBase : NULL;
    }
    else if (ppBase)
        *ppBase = NULL;

    if (ppBase)
        LogFlow(("PDMR3DeviceAttach: returns %Rrc *ppBase=%p\n", rc, *ppBase));
    else
        LogFlow(("PDMR3DeviceAttach: returns %Rrc\n", rc));
    return rc;
}


/**
 * Detaches a driver chain from an existing device instance.
 *
 * This is used to change drivers and suchlike at runtime.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pszDevice       Device name.
 * @param   iInstance       Device instance.
 * @param   iLun            The Logical Unit to obtain the interface of.
 * @param   fFlags          Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @thread  EMT
 */
VMMR3DECL(int) PDMR3DeviceDetach(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, uint32_t fFlags)
{
    return PDMR3DriverDetach(pUVM, pszDevice, iInstance, iLun, NULL, 0, fFlags);
}


/**
 * References the critical section associated with a device for the use by a
 * timer or similar created by the device.
 *
 * @returns Pointer to the critical section.
 * @param   pVM             The cross context VM structure.
 * @param   pDevIns         The device instance in question.
 *
 * @internal
 */
VMMR3_INT_DECL(PPDMCRITSECT) PDMR3DevGetCritSect(PVM pVM, PPDMDEVINS pDevIns)
{
    VM_ASSERT_EMT(pVM); RT_NOREF_PV(pVM);
    VM_ASSERT_STATE(pVM, VMSTATE_CREATING);
    AssertPtr(pDevIns);

    PPDMCRITSECT pCritSect = pDevIns->pCritSectRoR3;
    AssertPtr(pCritSect);
    pCritSect->s.fUsedByTimerOrSimilar = true;

    return pCritSect;
}


/**
 * Attaches a preconfigured driver to an existing device or driver instance.
 *
 * This is used to change drivers and suchlike at runtime.  The driver or device
 * at the end of the chain will be told to attach to whatever is configured
 * below it.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pszDevice       Device name.
 * @param   iInstance       Device instance.
 * @param   iLun            The Logical Unit to obtain the interface of.
 * @param   fFlags          Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @param   ppBase          Where to store the base interface pointer. Optional.
 *
 * @thread  EMT
 */
VMMR3DECL(int) PDMR3DriverAttach(PUVM pUVM, const char *pszDevice, unsigned iInstance, unsigned iLun, uint32_t fFlags, PPPDMIBASE ppBase)
{
    LogFlow(("PDMR3DriverAttach: pszDevice=%p:{%s} iInstance=%d iLun=%d fFlags=%#x ppBase=%p\n",
             pszDevice, pszDevice, iInstance, iLun, fFlags, ppBase));
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    VM_ASSERT_EMT(pVM);

    if (ppBase)
        *ppBase = NULL;

    /*
     * Find the LUN in question.
     */
    PPDMLUN pLun;
    int rc = pdmR3DevFindLun(pVM, pszDevice, iInstance, iLun, &pLun);
    if (RT_SUCCESS(rc))
    {
        /*
         * Anything attached to the LUN?
         */
        PPDMDRVINS pDrvIns = pLun->pTop;
        if (!pDrvIns)
        {
            /* No, ask the device to attach to the new stuff. */
            PPDMDEVINS pDevIns = pLun->pDevIns;
            if (pDevIns->pReg->pfnAttach)
            {
                PDMCritSectEnter(pDevIns->pCritSectRoR3, VERR_IGNORED);
                rc = pDevIns->pReg->pfnAttach(pDevIns, iLun, fFlags);
                if (RT_SUCCESS(rc) && ppBase)
                    *ppBase = pLun->pTop ? &pLun->pTop->IBase : NULL;
                PDMCritSectLeave(pDevIns->pCritSectRoR3);
            }
            else
                rc = VERR_PDM_DEVICE_NO_RT_ATTACH;
        }
        else
        {
            /* Yes, find the bottom most driver and ask it to attach to the new stuff. */
            while (pDrvIns->Internal.s.pDown)
                pDrvIns = pDrvIns->Internal.s.pDown;
            if (pDrvIns->pReg->pfnAttach)
            {
                rc = pDrvIns->pReg->pfnAttach(pDrvIns, fFlags);
                if (RT_SUCCESS(rc) && ppBase)
                    *ppBase = pDrvIns->Internal.s.pDown
                            ? &pDrvIns->Internal.s.pDown->IBase
                            : NULL;
            }
            else
                rc = VERR_PDM_DRIVER_NO_RT_ATTACH;
        }
    }

    if (ppBase)
        LogFlow(("PDMR3DriverAttach: returns %Rrc *ppBase=%p\n", rc, *ppBase));
    else
        LogFlow(("PDMR3DriverAttach: returns %Rrc\n", rc));
    return rc;
}


/**
 * Detaches the specified driver instance.
 *
 * This is used to replumb drivers at runtime for simulating hot plugging and
 * media changes.
 *
 * This is a superset of PDMR3DeviceDetach.  It allows detaching drivers from
 * any driver or device by specifying the driver to start detaching at.  The
 * only prerequisite is that the driver or device above implements the
 * pfnDetach callback (PDMDRVREG / PDMDEVREG).
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pszDevice       Device name.
 * @param   iDevIns         Device instance.
 * @param   iLun            The Logical Unit in which to look for the driver.
 * @param   pszDriver       The name of the driver which to detach.  If NULL
 *                          then the entire driver chain is detatched.
 * @param   iOccurance      The occurrence of that driver in the chain.  This is
 *                          usually 0.
 * @param   fFlags          Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @thread  EMT
 */
VMMR3DECL(int) PDMR3DriverDetach(PUVM pUVM, const char *pszDevice, unsigned iDevIns, unsigned iLun,
                                 const char *pszDriver, unsigned iOccurance, uint32_t fFlags)
{
    LogFlow(("PDMR3DriverDetach: pszDevice=%p:{%s} iDevIns=%u iLun=%u pszDriver=%p:{%s} iOccurance=%u fFlags=%#x\n",
             pszDevice, pszDevice, iDevIns, iLun, pszDriver, pszDriver, iOccurance, fFlags));
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    VM_ASSERT_EMT(pVM);
    AssertPtr(pszDevice);
    AssertPtrNull(pszDriver);
    Assert(iOccurance == 0 || pszDriver);
    Assert(!(fFlags & ~(PDM_TACH_FLAGS_NOT_HOT_PLUG)));

    /*
     * Find the LUN in question.
     */
    PPDMLUN pLun;
    int rc = pdmR3DevFindLun(pVM, pszDevice, iDevIns, iLun, &pLun);
    if (RT_SUCCESS(rc))
    {
        /*
         * Locate the driver.
         */
        PPDMDRVINS pDrvIns = pLun->pTop;
        if (pDrvIns)
        {
            if (pszDriver)
            {
                while (pDrvIns)
                {
                    if (!strcmp(pDrvIns->pReg->szName, pszDriver))
                    {
                        if (iOccurance == 0)
                            break;
                        iOccurance--;
                    }
                    pDrvIns = pDrvIns->Internal.s.pDown;
                }
            }
            if (pDrvIns)
                rc = pdmR3DrvDetach(pDrvIns, fFlags);
            else
                rc = VERR_PDM_DRIVER_INSTANCE_NOT_FOUND;
        }
        else
            rc = VINF_PDM_NO_DRIVER_ATTACHED_TO_LUN;
    }

    LogFlow(("PDMR3DriverDetach: returns %Rrc\n", rc));
    return rc;
}


/**
 * Runtime detach and reattach of a new driver chain or sub chain.
 *
 * This is intended to be called on a non-EMT thread, this will instantiate the
 * new driver (sub-)chain, and then the EMTs will do the actual replumbing.  The
 * destruction of the old driver chain will be taken care of on the calling
 * thread.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pszDevice       Device name.
 * @param   iDevIns         Device instance.
 * @param   iLun            The Logical Unit in which to look for the driver.
 * @param   pszDriver       The name of the driver which to detach and replace.
 *                          If NULL then the entire driver chain is to be
 *                          reattached.
 * @param   iOccurance      The occurrence of that driver in the chain.  This is
 *                          usually 0.
 * @param   fFlags          Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @param   pCfg            The configuration of the new driver chain that is
 *                          going to be attached.  The subtree starts with the
 *                          node containing a Driver key, a Config subtree and
 *                          optionally an AttachedDriver subtree.
 *                          If this parameter is NULL, then this call will work
 *                          like at a non-pause version of PDMR3DriverDetach.
 * @param   ppBase          Where to store the base interface pointer to the new
 *                          driver.  Optional.
 *
 * @thread  Any thread. The EMTs will be involved at some point though.
 */
VMMR3DECL(int)  PDMR3DriverReattach(PUVM pUVM, const char *pszDevice, unsigned iDevIns, unsigned iLun,
                                    const char *pszDriver, unsigned iOccurance, uint32_t fFlags,
                                    PCFGMNODE pCfg, PPPDMIBASE ppBase)
{
    NOREF(pUVM); NOREF(pszDevice); NOREF(iDevIns); NOREF(iLun); NOREF(pszDriver); NOREF(iOccurance);
    NOREF(fFlags); NOREF(pCfg); NOREF(ppBase);
    return VERR_NOT_IMPLEMENTED;
}

