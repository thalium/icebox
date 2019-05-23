/* $Id: DrvChar.cpp $ */
/** @file
 * Driver that adapts PDMISTREAM into PDMICHARCONNECTOR / PDMICHARPORT.
 *
 * Converts synchronous calls (PDMICHARCONNECTOR::pfnWrite, PDMISTREAM::pfnRead)
 * into asynchronous ones.
 *
 * Note that we don't use a send buffer here to be able to handle
 * dropping of bytes for xmit at device level.
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
#define LOG_GROUP LOG_GROUP_DRV_CHAR
#include <VBox/vmm/pdmdrv.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/poll.h>
#include <iprt/stream.h>
#include <iprt/critsect.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Char driver instance data.
 *
 * @implements PDMICHARCONNECTOR
 */
typedef struct DRVCHAR
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS                  pDrvIns;
    /** Pointer to the char port interface of the driver/device above us. */
    PPDMICHARPORT               pDrvCharPort;
    /** Pointer to the stream interface of the driver below us. */
    PPDMISTREAM                 pDrvStream;
    /** Our char interface. */
    PDMICHARCONNECTOR           ICharConnector;
    /** Flag to notify the receive thread it should terminate. */
    volatile bool               fShutdown;
    /** I/O thread. */
    PPDMTHREAD                  pThrdIo;
    /** Thread to relay read data to the device above without
     * blocking send operations.
     * @todo: This has to go but needs changes in the interface
     *        between device and driver.
     */
    PPDMTHREAD                  pThrdRead;
    /** Event semaphore for the read relay thread. */
    RTSEMEVENT                  hEvtSemRead;
    /** Critical section protection the send part. */
    RTCRITSECT                  CritSectSend;

    /** Internal send FIFO queue */
    uint8_t volatile            u8SendByte;
    bool volatile               fSending;
    uint8_t                     Alignment[2];

    /** Receive buffer. */
    uint8_t                     abBuffer[256];
    /** Number of bytes remaining in the receive buffer. */
    volatile size_t             cbRemaining;
    /** Current position into the read buffer. */
    uint8_t                     *pbBuf;

    /** Read/write statistics */
    STAMCOUNTER                 StatBytesRead;
    STAMCOUNTER                 StatBytesWritten;
} DRVCHAR, *PDRVCHAR;
AssertCompileMemberAlignment(DRVCHAR, StatBytesRead, 8);




/* -=-=-=-=- IBase -=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvCharQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS  pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVCHAR    pThis = PDMINS_2_DATA(pDrvIns, PDRVCHAR);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMICHARCONNECTOR, &pThis->ICharConnector);
    return NULL;
}


/* -=-=-=-=- ICharConnector -=-=-=-=- */

/**
 * @interface_method_impl{PDMICHARCONNECTOR,pfnWrite}
 */
static DECLCALLBACK(int) drvCharWrite(PPDMICHARCONNECTOR pInterface, const void *pvBuf, size_t cbWrite)
{
    PDRVCHAR pThis = RT_FROM_MEMBER(pInterface, DRVCHAR, ICharConnector);
    const char *pbBuffer = (const char *)pvBuf;
    int rc = VINF_SUCCESS;

    LogFlow(("%s: pvBuf=%#p cbWrite=%d\n", __FUNCTION__, pvBuf, cbWrite));

    RTCritSectEnter(&pThis->CritSectSend);

    for (uint32_t i = 0; i < cbWrite && RT_SUCCESS(rc); i++)
    {
        if (!ASMAtomicXchgBool(&pThis->fSending, true))
        {
            pThis->u8SendByte = pbBuffer[i];
            pThis->pDrvStream->pfnPollInterrupt(pThis->pDrvStream);
            STAM_COUNTER_INC(&pThis->StatBytesWritten);
        }
        else
            rc = VERR_BUFFER_OVERFLOW;
    }

    RTCritSectLeave(&pThis->CritSectSend);
    return rc;
}


/**
 * @interface_method_impl{PDMICHARCONNECTOR,pfnSetParameters}
 */
static DECLCALLBACK(int) drvCharSetParameters(PPDMICHARCONNECTOR pInterface, unsigned Bps, char chParity,
                                              unsigned cDataBits,  unsigned cStopBits)
{
    RT_NOREF(pInterface, Bps, chParity, cDataBits, cStopBits);

    LogFlow(("%s: Bps=%u chParity=%c cDataBits=%u cStopBits=%u\n", __FUNCTION__, Bps, chParity, cDataBits, cStopBits));
    return VINF_SUCCESS;
}


/* -=-=-=-=- I/O thread -=-=-=-=- */

/**
 * Send thread loop - pushes data down thru the driver chain.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The char driver instance.
 * @param   pThread     The worker thread.
 */
static DECLCALLBACK(int) drvCharIoLoop(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    RT_NOREF(pDrvIns);
    PDRVCHAR pThis = (PDRVCHAR)pThread->pvUser;

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        uint32_t fEvts = 0;

        if (   !pThis->cbRemaining
            && pThis->pDrvStream->pfnRead)
            fEvts |= RTPOLL_EVT_READ;
        if (pThis->fSending)
            fEvts |= RTPOLL_EVT_WRITE;

        uint32_t fEvtsRecv = 0;
        int rc = pThis->pDrvStream->pfnPoll(pThis->pDrvStream, fEvts, &fEvtsRecv, RT_INDEFINITE_WAIT);
        if (RT_SUCCESS(rc))
        {
            if (fEvtsRecv & RTPOLL_EVT_WRITE)
            {
                RTCritSectEnter(&pThis->CritSectSend);
                Assert(pThis->fSending);

                size_t cbProcessed = 1;
                uint8_t ch = pThis->u8SendByte;
                rc = pThis->pDrvStream->pfnWrite(pThis->pDrvStream, &ch, &cbProcessed);
                if (RT_SUCCESS(rc))
                {
                    ASMAtomicXchgBool(&pThis->fSending, false);
                    Assert(cbProcessed == 1);
                }
                else if (rc == VERR_TIMEOUT)
                {
                    /* Normal case, just means that the stream didn't accept a new
                     * character before the timeout elapsed. Just retry. */

                    /* do not change the rc status here, otherwise the (rc == VERR_TIMEOUT) branch
                     * in the wait above will never get executed */
                    /* rc = VINF_SUCCESS; */
                }
                else
                {
                    LogRel(("Write failed with %Rrc; skipping\n", rc));
                    break;
                }
                RTCritSectLeave(&pThis->CritSectSend);
            }

            if (fEvtsRecv & RTPOLL_EVT_READ)
            {
                AssertPtr(pThis->pDrvStream->pfnRead);
                Assert(!pThis->cbRemaining);

                size_t cbRead = sizeof(pThis->abBuffer);
                rc = pThis->pDrvStream->pfnRead(pThis->pDrvStream, &pThis->abBuffer[0], &cbRead);
                if (RT_FAILURE(rc))
                {
                    LogFlow(("Read failed with %Rrc\n", rc));
                    break;
                }
                pThis->pbBuf = &pThis->abBuffer[0];
                ASMAtomicWriteZ(&pThis->cbRemaining, cbRead);
                RTSemEventSignal(pThis->hEvtSemRead); /* Wakeup relay thread to continue. */
            }
        }
        else if (rc != VERR_INTERRUPTED)
            LogRelMax(10, ("Char#%d: Polling failed with %Rrc\n", pDrvIns->iInstance, rc));
    }

    return VINF_SUCCESS;
}


/**
 * Unblock the send worker thread so it can respond to a state change.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The char driver instance.
 * @param   pThread     The worker thread.
 */
static DECLCALLBACK(int) drvCharIoLoopWakeup(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    PDRVCHAR pThis = (PDRVCHAR)pThread->pvUser;

    RT_NOREF(pDrvIns);
    return pThis->pDrvStream->pfnPollInterrupt(pThis->pDrvStream);
}


static DECLCALLBACK(int) drvCharReadRelayLoop(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    RT_NOREF(pDrvIns);
    PDRVCHAR pThis = (PDRVCHAR)pThread->pvUser;

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;
    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        size_t cbRem = ASMAtomicReadZ(&pThis->cbRemaining);

        /* Block as long as there is nothing to relay. */
        if (!pThis->cbRemaining)
            rc = RTSemEventWait(pThis->hEvtSemRead, RT_INDEFINITE_WAIT);

        if (pThread->enmState != PDMTHREADSTATE_RUNNING)
            break;

        cbRem = ASMAtomicReadZ(&pThis->cbRemaining);
        if (cbRem)
        {
            /* Send data to guest. */
            size_t cbProcessed = cbRem;
            rc = pThis->pDrvCharPort->pfnNotifyRead(pThis->pDrvCharPort, pThis->pbBuf, &cbProcessed);
            if (RT_SUCCESS(rc))
            {
                Assert(cbProcessed);
                pThis->pbBuf += cbProcessed;

                /* Wake up the I/o thread so it can read new data to process. */
                cbRem = ASMAtomicSubZ(&pThis->cbRemaining, cbProcessed);
                if (cbRem == cbProcessed)
                    pThis->pDrvStream->pfnPollInterrupt(pThis->pDrvStream);
                STAM_COUNTER_ADD(&pThis->StatBytesRead, cbProcessed);
            }
            else if (rc == VERR_TIMEOUT)
            {
                /* Normal case, just means that the guest didn't accept a new
                 * character before the timeout elapsed. Just retry. */
                rc = VINF_SUCCESS;
            }
            else
            {
                LogFlow(("NotifyRead failed with %Rrc\n", rc));
                break;
            }
        }
    }

    return VINF_SUCCESS;
}


/**
 * Unblock the read relay worker thread so it can respond to a state change.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The char driver instance.
 * @param   pThread     The worker thread.
 */
static DECLCALLBACK(int) drvCharReadRelayLoopWakeup(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    PDRVCHAR pThis = (PDRVCHAR)pThread->pvUser;

    RT_NOREF(pDrvIns);
    return RTSemEventSignal(pThis->hEvtSemRead);
}


/**
 * @callback_method_impl{PDMICHARCONNECTOR,pfnSetModemLines}
 */
static DECLCALLBACK(int) drvCharSetModemLines(PPDMICHARCONNECTOR pInterface, bool fRequestToSend, bool fDataTerminalReady)
{
    /* Nothing to do here. */
    RT_NOREF(pInterface, fRequestToSend, fDataTerminalReady);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{PDMICHARCONNECTOR,pfnSetBreak}
 */
static DECLCALLBACK(int) drvCharSetBreak(PPDMICHARCONNECTOR pInterface, bool fBreak)
{
    /* Nothing to do here. */
    RT_NOREF(pInterface, fBreak);
    return VINF_SUCCESS;
}


/* -=-=-=-=- driver interface -=-=-=-=- */

/**
 * Destruct a char driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that
 * any non-VM resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvCharDestruct(PPDMDRVINS pDrvIns)
{
    PDRVCHAR pThis = PDMINS_2_DATA(pDrvIns, PDRVCHAR);
    LogFlow(("%s: iInstance=%d\n", __FUNCTION__, pDrvIns->iInstance));
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    if (RTCritSectIsInitialized(&pThis->CritSectSend))
        RTCritSectDelete(&pThis->CritSectSend);

    if (pThis->hEvtSemRead != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(pThis->hEvtSemRead);
        pThis->hEvtSemRead = NIL_RTSEMEVENT;
    }
}


/**
 * Construct a char driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvCharConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(pCfg);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVCHAR pThis = PDMINS_2_DATA(pDrvIns, PDRVCHAR);
    LogFlow(("%s: iInstance=%d\n", __FUNCTION__, pDrvIns->iInstance));

    /*
     * Init basic data members and interfaces.
     */
    pThis->pDrvIns                          = pDrvIns;
    pThis->pThrdIo                          = NIL_RTTHREAD;
    pThis->pThrdRead                        = NIL_RTTHREAD;
    pThis->hEvtSemRead                      = NIL_RTSEMEVENT;
    /* IBase. */
    pDrvIns->IBase.pfnQueryInterface        = drvCharQueryInterface;
    /* ICharConnector. */
    pThis->ICharConnector.pfnWrite          = drvCharWrite;
    pThis->ICharConnector.pfnSetParameters  = drvCharSetParameters;
    pThis->ICharConnector.pfnSetModemLines  = drvCharSetModemLines;
    pThis->ICharConnector.pfnSetBreak       = drvCharSetBreak;

    int rc = RTCritSectInit(&pThis->CritSectSend);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Char#%d: Failed to create critical section"), pDrvIns->iInstance);

    /*
     * Get the ICharPort interface of the above driver/device.
     */
    pThis->pDrvCharPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMICHARPORT);
    if (!pThis->pDrvCharPort)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_PDM_MISSING_INTERFACE_ABOVE, RT_SRC_POS, N_("Char#%d has no char port interface above"), pDrvIns->iInstance);

    /*
     * Attach driver below and query its stream interface.
     */
    PPDMIBASE pBase;
    rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pBase);
    if (RT_FAILURE(rc))
        return rc; /* Don't call PDMDrvHlpVMSetError here as we assume that the driver already set an appropriate error */
    pThis->pDrvStream = PDMIBASE_QUERY_INTERFACE(pBase, PDMISTREAM);
    if (!pThis->pDrvStream)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_PDM_MISSING_INTERFACE_BELOW, RT_SRC_POS, N_("Char#%d has no stream interface below"), pDrvIns->iInstance);

    /* Don't start the receive relay thread if reading is not supported. */
    if (pThis->pDrvStream->pfnRead)
    {
        rc = PDMDrvHlpThreadCreate(pThis->pDrvIns, &pThis->pThrdRead, pThis, drvCharReadRelayLoop,
                                   drvCharReadRelayLoopWakeup, 0, RTTHREADTYPE_IO, "CharReadRel");
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, N_("Char#%d cannot create read relay thread"), pDrvIns->iInstance);

         rc = RTSemEventCreate(&pThis->hEvtSemRead);
         AssertRCReturn(rc, rc);
    }

    rc = PDMDrvHlpThreadCreate(pThis->pDrvIns, &pThis->pThrdIo, pThis, drvCharIoLoop,
                               drvCharIoLoopWakeup, 0, RTTHREADTYPE_IO, "CharIo");
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, N_("Char#%d cannot create send thread"), pDrvIns->iInstance);


    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatBytesWritten,    STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_BYTES, "Nr of bytes written",         "/Devices/Char%d/Written", pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatBytesRead,       STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_BYTES, "Nr of bytes read",            "/Devices/Char%d/Read", pDrvIns->iInstance);

    return VINF_SUCCESS;
}


/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvChar =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "Char",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Generic char driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_CHAR,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVCHAR),
    /* pfnConstruct */
    drvCharConstruct,
    /* pfnDestruct */
    drvCharDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};
