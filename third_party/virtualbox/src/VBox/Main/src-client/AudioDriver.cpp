/* $Id: AudioDriver.cpp $ */
/** @file
 * VirtualBox audio base class for Main audio drivers.
 */

/*
 * Copyright (C) 2018 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include "LoggingNew.h"

#include <VBox/log.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmdrv.h>

#include "AudioDriver.h"
#include "ConsoleImpl.h"

AudioDriver::AudioDriver(Console *pConsole)
    : mpConsole(pConsole)
    , mfAttached(false)
{
}

AudioDriver::~AudioDriver(void)
{
}


/**
 * Initializes the audio driver with a certain (device) configuration.
 *
 * @returns VBox status code.
 * @param   pCfg                Audio driver configuration to use.
 */
int AudioDriver::InitializeConfig(AudioDriverCfg *pCfg)
{
    AssertPtrReturn(pCfg, VERR_INVALID_POINTER);

    /* Sanity. */
    AssertReturn(pCfg->strDev.isNotEmpty(),  VERR_INVALID_PARAMETER);
    AssertReturn(pCfg->uLUN != UINT8_MAX,    VERR_INVALID_PARAMETER);
    AssertReturn(pCfg->strName.isNotEmpty(), VERR_INVALID_PARAMETER);

    /* Apply configuration. */
    mCfg = *pCfg;

    return VINF_SUCCESS;
}


/**
 * Attaches the driver via EMT, if configured.
 *
 * @returns IPRT status code.
 * @param   pUVM                The user mode VM handle for talking to EMT.
 * @param   pAutoLock           The callers auto lock instance.  Can be NULL if
 *                              not locked.
 */
int AudioDriver::doAttachDriverViaEmt(PUVM pUVM, util::AutoWriteLock *pAutoLock)
{
    if (!isConfigured())
        return VINF_SUCCESS;

    PVMREQ pReq;
    int vrc = VMR3ReqCallU(pUVM, VMCPUID_ANY, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)attachDriverOnEmt, 1, this);
    if (vrc == VERR_TIMEOUT)
    {
        /* Release the lock before a blocking VMR3* call (EMT might wait for it, @bugref{7648})! */
        if (pAutoLock)
            pAutoLock->release();

        vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);

        if (pAutoLock)
            pAutoLock->acquire();
    }

    AssertRC(vrc);
    VMR3ReqFree(pReq);

    return vrc;
}


/**
 * Configures the audio driver (to CFGM) and attaches it to the audio chain.
 * Does nothing if the audio driver already is attached.
 *
 * @returns VBox status code.
 * @param   pThis               Audio driver to detach.
 */
/* static */
DECLCALLBACK(int) AudioDriver::attachDriverOnEmt(AudioDriver *pThis)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    Console::SafeVMPtrQuiet ptrVM(pThis->mpConsole);
    Assert(ptrVM.isOk());

    if (pThis->mfAttached) /* Already attached? Bail out. */
    {
        LogFunc(("%s: Already attached\n", pThis->mCfg.strName.c_str()));
        return VINF_SUCCESS;
    }

    AudioDriverCfg *pCfg = &pThis->mCfg;

    LogFunc(("strName=%s, strDevice=%s, uInst=%u, uLUN=%u\n",
             pCfg->strName.c_str(), pCfg->strDev.c_str(), pCfg->uInst, pCfg->uLUN));

    /* Detach the driver chain from the audio device first. */
    int rc = PDMR3DeviceDetach(ptrVM.rawUVM(), pCfg->strDev.c_str(), pCfg->uInst, pCfg->uLUN, 0 /* fFlags */);
    if (RT_SUCCESS(rc))
    {
        rc = pThis->configure(pCfg->uLUN, true /* Attach */);
        if (RT_SUCCESS(rc))
            rc = PDMR3DriverAttach(ptrVM.rawUVM(), pCfg->strDev.c_str(), pCfg->uInst, pCfg->uLUN, 0 /* fFlags */,
                                   NULL /* ppBase */);
    }

    if (RT_SUCCESS(rc))
    {
        pThis->mfAttached = true;
        LogRel2(("%s: Driver attached (LUN #%u)\n", pCfg->strName.c_str(), pCfg->uLUN));
    }
    else
        LogRel(("%s: Failed to attach audio driver, rc=%Rrc\n", pCfg->strName.c_str(), rc));

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}


/**
 * Detatches the driver via EMT, if configured.
 *
 * @returns IPRT status code.
 * @param   pUVM                The user mode VM handle for talking to EMT.
 * @param   pAutoLock           The callers auto lock instance.  Can be NULL if
 *                              not locked.
 */
int AudioDriver::doDetachDriverViaEmt(PUVM pUVM, util::AutoWriteLock *pAutoLock)
{
    if (!isConfigured())
        return VINF_SUCCESS;

    PVMREQ pReq;
    int vrc = VMR3ReqCallU(pUVM, VMCPUID_ANY, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)detachDriverOnEmt, 1, this);
    if (vrc == VERR_TIMEOUT)
    {
        /* Release the lock before a blocking VMR3* call (EMT might wait for it, @bugref{7648})! */
        if (pAutoLock)
            pAutoLock->release();

        vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);

        if (pAutoLock)
            pAutoLock->acquire();
    }

    AssertRC(vrc);
    VMR3ReqFree(pReq);

    return vrc;
}


/**
 * Detaches an already attached audio driver from the audio chain.
 * Does nothing if the audio driver already is detached or not attached.
 *
 * @returns VBox status code.
 * @param   pThis               Audio driver to detach.
 */
/* static */
DECLCALLBACK(int) AudioDriver::detachDriverOnEmt(AudioDriver *pThis)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    if (!pThis->mfAttached) /* Not attached? Bail out. */
    {
        LogFunc(("%s: Not attached\n", pThis->mCfg.strName.c_str()));
        return VINF_SUCCESS;
    }

    Console::SafeVMPtrQuiet ptrVM(pThis->mpConsole);
    Assert(ptrVM.isOk());

    AudioDriverCfg *pCfg = &pThis->mCfg;

    Assert(pCfg->uLUN != UINT8_MAX);

    LogFunc(("strName=%s, strDevice=%s, uInst=%u, uLUN=%u\n",
             pCfg->strName.c_str(), pCfg->strDev.c_str(), pCfg->uInst, pCfg->uLUN));

    /* Destroy the entire driver chain for the specified LUN.
     *
     * Start with the "AUDIO" driver, as this driver serves as the audio connector between
     * the device emulation and the select backend(s). */
    int rc = PDMR3DriverDetach(ptrVM.rawUVM(), pCfg->strDev.c_str(), pCfg->uInst, pCfg->uLUN,
                               "AUDIO", 0 /* iOccurrence */,  0 /* fFlags */);
    if (RT_SUCCESS(rc))
        rc = pThis->configure(pCfg->uLUN, false /* Detach */);

    if (RT_SUCCESS(rc))
    {
        pThis->mfAttached = false;
        LogRel2(("%s: Driver detached\n", pCfg->strName.c_str()));
    }
    else
        LogRel(("%s: Failed to detach audio driver, rc=%Rrc\n", pCfg->strName.c_str(), rc));

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * Configures the audio driver via CFGM.
 *
 * @returns VBox status code.
 * @param   uLUN                LUN to attach driver to.
 * @param   fAttach             Whether to attach or detach the driver configuration to CFGM.
 *
 * @thread EMT
 */
int AudioDriver::configure(unsigned uLUN, bool fAttach)
{
    Console::SafeVMPtrQuiet ptrVM(mpConsole);
    Assert(ptrVM.isOk());

    PUVM pUVM = ptrVM.rawUVM();
    AssertPtr(pUVM);

    PCFGMNODE pRoot = CFGMR3GetRootU(pUVM);
    AssertPtr(pRoot);
    PCFGMNODE pDev0 = CFGMR3GetChildF(pRoot, "Devices/%s/%u/", mCfg.strDev.c_str(), mCfg.uInst);

    if (!pDev0) /* No audio device configured? Bail out. */
    {
        LogRel2(("%s: No audio device configured, skipping to attach driver\n", mCfg.strName.c_str()));
        return VINF_SUCCESS;
    }

    int rc = VINF_SUCCESS;

    PCFGMNODE pDevLun = CFGMR3GetChildF(pDev0, "LUN#%u/", uLUN);

    if (fAttach)
    {
        do
        {
            AssertMsgBreakStmt(pDevLun, ("%s: Device LUN #%u not found\n", mCfg.strName.c_str(), uLUN), rc = VERR_NOT_FOUND);

            LogRel2(("%s: Configuring audio driver (to LUN #%u)\n", mCfg.strName.c_str(), uLUN));

            CFGMR3RemoveNode(pDevLun); /* Remove LUN completely first. */

            /* Insert new LUN configuration and build up the new driver chain. */
            rc = CFGMR3InsertNodeF(pDev0, &pDevLun, "LUN#%u/", uLUN);                              AssertRCBreak(rc);
            rc = CFGMR3InsertString(pDevLun, "Driver", "AUDIO");                                   AssertRCBreak(rc);

            PCFGMNODE pLunCfg;
            rc = CFGMR3InsertNode(pDevLun, "Config", &pLunCfg);                                    AssertRCBreak(rc);

                rc = CFGMR3InsertStringF(pLunCfg, "DriverName",    "%s", mCfg.strName.c_str());    AssertRCBreak(rc);

                rc = CFGMR3InsertInteger(pLunCfg, "InputEnabled",  0); /* Play safe by default. */ AssertRCBreak(rc);
                rc = CFGMR3InsertInteger(pLunCfg, "OutputEnabled", 1);                             AssertRCBreak(rc);

            PCFGMNODE pAttachedDriver, pAttachedDriverCfg;
            rc = CFGMR3InsertNode(pDevLun, "AttachedDriver", &pAttachedDriver);                    AssertRCBreak(rc);
                rc = CFGMR3InsertStringF(pAttachedDriver, "Driver", "%s", mCfg.strName.c_str());   AssertRCBreak(rc);
                rc = CFGMR3InsertNode(pAttachedDriver, "Config", &pAttachedDriverCfg);             AssertRCBreak(rc);

                /* Call the (virtual) method for driver-specific configuration. */
                rc = configureDriver(pAttachedDriverCfg);                                          AssertRCBreak(rc);

        } while (0);
    }
    else /* Detach */
    {
        LogRel2(("%s: Unconfiguring audio driver\n", mCfg.strName.c_str()));
    }

#ifdef LOG_ENABLED
    LogFunc(("%s: fAttach=%RTbool\n", mCfg.strName.c_str(), fAttach));
    CFGMR3Dump(pDevLun);
#endif

    if (RT_FAILURE(rc))
        LogRel(("%s: %s audio driver failed with rc=%Rrc\n",
                mCfg.strName.c_str(), fAttach ? "Configuring" : "Unconfiguring", rc));

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}

