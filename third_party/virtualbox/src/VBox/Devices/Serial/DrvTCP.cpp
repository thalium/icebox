/* $Id: DrvTCP.cpp $ */
/** @file
 * TCP socket driver implementing the IStream interface.
 */

/*
 * Contributed by Alexey Eromenko (derived from DrvNamedPipe).
 *
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
#define LOG_GROUP LOG_GROUP_DRV_TCP
#include <VBox/vmm/pdmdrv.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/stream.h>
#include <iprt/alloc.h>
#include <iprt/pipe.h>
#include <iprt/poll.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/socket.h>
#include <iprt/tcp.h>
#include <iprt/uuid.h>
#include <stdlib.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

#define DRVTCP_POLLSET_ID_SOCKET 0
#define DRVTCP_POLLSET_ID_WAKEUP 1

#define DRVTCP_WAKEUP_REASON_EXTERNAL       0
#define DRVTCP_WAKEUP_REASON_NEW_CONNECTION 1


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * TCP driver instance data.
 *
 * @implements PDMISTREAM
 */
typedef struct DRVTCP
{
    /** The stream interface. */
    PDMISTREAM          IStream;
    /** Pointer to the driver instance. */
    PPDMDRVINS          pDrvIns;
    /** Pointer to the TCP server address:port or port only. (Freed by MM) */
    char                *pszLocation;
    /** Flag whether VirtualBox represents the server or client side. */
    bool                fIsServer;

    /** Handle of the TCP server for incoming connections. */
    PRTTCPSERVER        hTcpServ;
    /** Socket handle of the TCP socket connection. */
    RTSOCKET            hTcpSock;

    /** Poll set used to wait for I/O events. */
    RTPOLLSET           hPollSet;
    /** Reading end of the wakeup pipe. */
    RTPIPE              hPipeWakeR;
    /** Writing end of the wakeup pipe. */
    RTPIPE              hPipeWakeW;
    /** Flag whether the socket is in the pollset. */
    bool                fTcpSockInPollSet;
    /** Flag whether the send buffer is full nad it is required to wait for more
     * space until there is room again. */
    bool                fXmitBufFull;

    /** Thread for listening for new connections. */
    RTTHREAD            ListenThread;
    /** Flag to signal listening thread to shut down. */
    bool volatile       fShutdown;
} DRVTCP, *PDRVTCP;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/**
 * Kicks any possibly polling thread to get informed about changes.
 *
 * @returns VBOx status code.
 * @param   pThis                  The TCP driver instance.
 * @param   bReason                The reason code to handle.
 */
static int drvTcpPollerKick(PDRVTCP pThis, uint8_t bReason)
{
    size_t cbWritten = 0;
    return RTPipeWrite(pThis->hPipeWakeW, &bReason, 1, &cbWritten);
}


/** @interface_method_impl{PDMISTREAM,pfnPoll} */
static DECLCALLBACK(int) drvTcpPoll(PPDMISTREAM pInterface, uint32_t fEvts, uint32_t *pfEvts, RTMSINTERVAL cMillies)
{
    int rc = VINF_SUCCESS;
    PDRVTCP pThis = RT_FROM_MEMBER(pInterface, DRVTCP, IStream);

    if (pThis->hTcpSock != NIL_RTSOCKET)
    {
        if (!pThis->fTcpSockInPollSet)
        {
            rc = RTPollSetAddSocket(pThis->hPollSet, pThis->hTcpSock,
                                    fEvts, DRVTCP_POLLSET_ID_SOCKET);
            if (RT_SUCCESS(rc))
            {
                pThis->fTcpSockInPollSet = true;
                pThis->fXmitBufFull = false;
            }
        }
        else
        {
            /*
             * Just return if the send buffer wasn't full till now and
             * the caller wants to check whether writing is possible with
             * the event set.
             *
             * On Windows the write event is only posted after a send operation returned
             * WSAEWOULDBLOCK. So without this we would block in the poll call below waiting
             * for an event which would never happen if the buffer has space left.
             */
            if (   (fEvts & RTPOLL_EVT_WRITE)
                && !pThis->fXmitBufFull)
            {
                *pfEvts = RTPOLL_EVT_WRITE;
                return VINF_SUCCESS;
            }

            /* Always include error event. */
            fEvts |= RTPOLL_EVT_ERROR;
            rc = RTPollSetEventsChange(pThis->hPollSet, DRVTCP_POLLSET_ID_SOCKET, fEvts);
            AssertRC(rc);
        }
    }

    if (RT_SUCCESS(rc))
    {
        while (RT_SUCCESS(rc))
        {
            uint32_t fEvtsRecv = 0;
            uint32_t idHnd = 0;

            rc = RTPoll(pThis->hPollSet, cMillies, &fEvtsRecv, &idHnd);
            if (RT_SUCCESS(rc))
            {
                if (idHnd == DRVTCP_POLLSET_ID_WAKEUP)
                {
                    /* We got woken up, drain the pipe and return. */
                    uint8_t bReason;
                    size_t cbRead = 0;
                    rc = RTPipeRead(pThis->hPipeWakeR, &bReason, 1, &cbRead);
                    AssertRC(rc);

                    if (bReason == DRVTCP_WAKEUP_REASON_EXTERNAL)
                        rc = VERR_INTERRUPTED;
                    else if (bReason == DRVTCP_WAKEUP_REASON_NEW_CONNECTION)
                    {
                        Assert(!pThis->fTcpSockInPollSet);
                        rc = RTPollSetAddSocket(pThis->hPollSet, pThis->hTcpSock,
                                                fEvts, DRVTCP_POLLSET_ID_SOCKET);
                        if (RT_SUCCESS(rc))
                            pThis->fTcpSockInPollSet = true;
                    }
                    else
                        AssertMsgFailed(("Unknown wakeup reason in pipe %u\n", bReason));
                }
                else
                {
                    Assert(idHnd == DRVTCP_POLLSET_ID_SOCKET);

                    /* On error we close the socket here. */
                    if (fEvtsRecv & RTPOLL_EVT_ERROR)
                    {
                        rc = RTPollSetRemove(pThis->hPollSet, DRVTCP_POLLSET_ID_SOCKET);
                        AssertRC(rc);

                        if (pThis->fIsServer)
                            RTTcpServerDisconnectClient2(pThis->hTcpSock);
                        else
                            RTSocketClose(pThis->hTcpSock);
                        pThis->hTcpSock = NIL_RTSOCKET;
                        pThis->fTcpSockInPollSet = false;
                        /* Continue with polling. */
                    }
                    else
                    {
                        if (fEvtsRecv & RTPOLL_EVT_WRITE)
                            pThis->fXmitBufFull = false;
                        *pfEvts = fEvtsRecv;
                        break;
                    }
                }
            }
        }
    }

    return rc;
}


/** @interface_method_impl{PDMISTREAM,pfnPollInterrupt} */
static DECLCALLBACK(int) drvTcpPollInterrupt(PPDMISTREAM pInterface)
{
    PDRVTCP pThis = RT_FROM_MEMBER(pInterface, DRVTCP, IStream);
    return drvTcpPollerKick(pThis, DRVTCP_WAKEUP_REASON_EXTERNAL);
}


/** @interface_method_impl{PDMISTREAM,pfnRead} */
static DECLCALLBACK(int) drvTcpRead(PPDMISTREAM pInterface, void *pvBuf, size_t *pcbRead)
{
    int rc = VINF_SUCCESS;
    PDRVTCP pThis = RT_FROM_MEMBER(pInterface, DRVTCP, IStream);
    LogFlow(("%s: pvBuf=%p *pcbRead=%#x (%s)\n", __FUNCTION__, pvBuf, *pcbRead, pThis->pszLocation));

    Assert(pvBuf);

    if (pThis->hTcpSock != NIL_RTSOCKET)
    {
        size_t cbRead;
        size_t cbBuf = *pcbRead;
        rc = RTSocketReadNB(pThis->hTcpSock, pvBuf, cbBuf, &cbRead);
        if (RT_SUCCESS(rc))
        {
            if (!cbRead && rc != VINF_TRY_AGAIN)
            {
                rc = RTPollSetRemove(pThis->hPollSet, DRVTCP_POLLSET_ID_SOCKET);
                AssertRC(rc);

                if (pThis->fIsServer)
                    RTTcpServerDisconnectClient2(pThis->hTcpSock);
                else
                    RTSocketClose(pThis->hTcpSock);
                pThis->hTcpSock = NIL_RTSOCKET;
                pThis->fTcpSockInPollSet = false;
                rc = VINF_SUCCESS;
            }
            *pcbRead = cbRead;
        }
    }
    else
    {
        RTThreadSleep(100);
        *pcbRead = 0;
    }

    LogFlow(("%s: *pcbRead=%zu returns %Rrc\n", __FUNCTION__, *pcbRead, rc));
    return rc;
}


/** @interface_method_impl{PDMISTREAM,pfnWrite} */
static DECLCALLBACK(int) drvTcpWrite(PPDMISTREAM pInterface, const void *pvBuf, size_t *pcbWrite)
{
    int rc = VINF_SUCCESS;
    PDRVTCP pThis = RT_FROM_MEMBER(pInterface, DRVTCP, IStream);
    LogFlow(("%s: pvBuf=%p *pcbWrite=%#x (%s)\n", __FUNCTION__, pvBuf, *pcbWrite, pThis->pszLocation));

    Assert(pvBuf);
    if (pThis->hTcpSock != NIL_RTSOCKET)
    {
        size_t cbBuf = *pcbWrite;
        rc = RTSocketWriteNB(pThis->hTcpSock, pvBuf, cbBuf, pcbWrite);
        if (rc == VINF_TRY_AGAIN)
        {
            Assert(*pcbWrite == 0);
            pThis->fXmitBufFull = true;
            rc = VERR_TIMEOUT;
        }
    }
    else
        *pcbWrite = 0;

    LogFlow(("%s: returns %Rrc *pcbWrite=%zu\n", __FUNCTION__, rc, *pcbWrite));
    return rc;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvTCPQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS      pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVTCP         pThis   = PDMINS_2_DATA(pDrvIns, PDRVTCP);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMISTREAM, &pThis->IStream);
    return NULL;
}


/* -=-=-=-=- listen thread -=-=-=-=- */

/**
 * Receive thread loop.
 *
 * @returns VINF_SUCCESS
 * @param   hThreadSelf Thread handle to this thread.
 * @param   pvUser      User argument.
 */
static DECLCALLBACK(int) drvTCPListenLoop(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    PDRVTCP pThis = (PDRVTCP)pvUser;

    while (RT_LIKELY(!pThis->fShutdown))
    {
        RTSOCKET hTcpSockNew = NIL_RTSOCKET;
        int rc = RTTcpServerListen2(pThis->hTcpServ, &hTcpSockNew);
        if (RT_SUCCESS(rc))
        {
            if (pThis->hTcpSock != NIL_RTSOCKET)
            {
                LogRel(("DrvTCP%d: only single connection supported\n", pThis->pDrvIns->iInstance));
                RTTcpServerDisconnectClient2(hTcpSockNew);
            }
            else
            {
                pThis->hTcpSock = hTcpSockNew;
                /* Inform the poller about the new socket. */
                drvTcpPollerKick(pThis, DRVTCP_WAKEUP_REASON_NEW_CONNECTION);
            }
        }
    }

    return VINF_SUCCESS;
}

/* -=-=-=-=- PDMDRVREG -=-=-=-=- */

/**
 * Common worker for drvTCPPowerOff and drvTCPDestructor.
 *
 * @param   pThis               The instance data.
 */
static void drvTCPShutdownListener(PDRVTCP pThis)
{
    /*
     * Signal shutdown of the listener thread.
     */
    pThis->fShutdown = true;
    if (    pThis->fIsServer
        &&  pThis->hTcpServ != NULL)
    {
        int rc = RTTcpServerShutdown(pThis->hTcpServ);
        AssertRC(rc);
        pThis->hTcpServ = NULL;
    }
}


/**
 * Power off a TCP socket stream driver instance.
 *
 * This does most of the destruction work, to avoid ordering dependencies.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvTCPPowerOff(PPDMDRVINS pDrvIns)
{
    PDRVTCP pThis = PDMINS_2_DATA(pDrvIns, PDRVTCP);
    LogFlow(("%s: %s\n", __FUNCTION__, pThis->pszLocation));

    drvTCPShutdownListener(pThis);
}


/**
 * Destruct a TCP socket stream driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that
 * any non-VM resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvTCPDestruct(PPDMDRVINS pDrvIns)
{
    PDRVTCP pThis = PDMINS_2_DATA(pDrvIns, PDRVTCP);
    LogFlow(("%s: %s\n", __FUNCTION__, pThis->pszLocation));
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    drvTCPShutdownListener(pThis);

    /*
     * While the thread exits, clean up as much as we can.
     */
    if (pThis->hTcpSock != NIL_RTSOCKET)
    {
        int rc = RTPollSetRemove(pThis->hPollSet, DRVTCP_POLLSET_ID_SOCKET);
        AssertRC(rc);

        rc = RTSocketShutdown(pThis->hTcpSock, true /* fRead */, true /* fWrite */);
        AssertRC(rc);

        rc = RTSocketClose(pThis->hTcpSock);
        AssertRC(rc); RT_NOREF(rc);

        pThis->hTcpSock = NIL_RTSOCKET;
    }

    if (pThis->hPipeWakeR != NIL_RTPIPE)
    {
        int rc = RTPipeClose(pThis->hPipeWakeR);
        AssertRC(rc);

        pThis->hPipeWakeR = NIL_RTPIPE;
    }

    if (pThis->hPipeWakeW != NIL_RTPIPE)
    {
        int rc = RTPipeClose(pThis->hPipeWakeW);
        AssertRC(rc);

        pThis->hPipeWakeW = NIL_RTPIPE;
    }

    if (pThis->hPollSet != NIL_RTPOLLSET)
    {
        int rc = RTPollSetDestroy(pThis->hPollSet);
        AssertRC(rc);

        pThis->hPollSet = NIL_RTPOLLSET;
    }

    MMR3HeapFree(pThis->pszLocation);
    pThis->pszLocation = NULL;

    /*
     * Wait for the thread.
     */
    if (pThis->ListenThread != NIL_RTTHREAD)
    {
        int rc = RTThreadWait(pThis->ListenThread, 30000, NULL);
        if (RT_SUCCESS(rc))
            pThis->ListenThread = NIL_RTTHREAD;
        else
            LogRel(("DrvTCP%d: listen thread did not terminate (%Rrc)\n", pDrvIns->iInstance, rc));
    }
}


/**
 * Construct a TCP socket stream driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvTCPConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVTCP pThis = PDMINS_2_DATA(pDrvIns, PDRVTCP);

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                      = pDrvIns;
    pThis->pszLocation                  = NULL;
    pThis->fIsServer                    = false;

    pThis->hTcpServ                     = NULL;
    pThis->hTcpSock                     = NIL_RTSOCKET;

    pThis->hPollSet                     = NIL_RTPOLLSET;
    pThis->hPipeWakeR                   = NIL_RTPIPE;
    pThis->hPipeWakeW                   = NIL_RTPIPE;
    pThis->fTcpSockInPollSet            = false;

    pThis->ListenThread                 = NIL_RTTHREAD;
    pThis->fShutdown                    = false;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface    = drvTCPQueryInterface;
    /* IStream */
    pThis->IStream.pfnPoll              = drvTcpPoll;
    pThis->IStream.pfnPollInterrupt     = drvTcpPollInterrupt;
    pThis->IStream.pfnRead              = drvTcpRead;
    pThis->IStream.pfnWrite             = drvTcpWrite;

    /*
     * Validate and read the configuration.
     */
    PDMDRV_VALIDATE_CONFIG_RETURN(pDrvIns, "Location|IsServer", "");

    int rc = CFGMR3QueryStringAlloc(pCfg, "Location", &pThis->pszLocation);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Configuration error: querying \"Location\" resulted in %Rrc"), rc);
    rc = CFGMR3QueryBool(pCfg, "IsServer", &pThis->fIsServer);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Configuration error: querying \"IsServer\" resulted in %Rrc"), rc);

    rc = RTPipeCreate(&pThis->hPipeWakeR, &pThis->hPipeWakeW, 0 /* fFlags */);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("DrvTCP#%d: Failed to create wake pipe"), pDrvIns->iInstance);

    rc = RTPollSetCreate(&pThis->hPollSet);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("DrvTCP#%d: Failed to create poll set"), pDrvIns->iInstance);

    rc = RTPollSetAddPipe(pThis->hPollSet, pThis->hPipeWakeR,
                            RTPOLL_EVT_READ | RTPOLL_EVT_ERROR,
                            DRVTCP_POLLSET_ID_WAKEUP);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("DrvTCP#%d failed to add wakeup pipe for %s to poll set"),
                                   pDrvIns->iInstance, pThis->pszLocation);

    /*
     * Create/Open the socket.
     */
    if (pThis->fIsServer)
    {
        uint32_t uPort = 0;
        rc = RTStrToUInt32Ex(pThis->pszLocation, NULL, 10, &uPort);
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                       N_("DrvTCP#%d: The port part of the location is not a numerical value"),
                                       pDrvIns->iInstance);

        /** @todo Allow binding to distinct interfaces. */
        rc = RTTcpServerCreateEx(NULL, uPort, &pThis->hTcpServ);
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc,  RT_SRC_POS,
                                       N_("DrvTCP#%d failed to create server socket"), pDrvIns->iInstance);

        rc = RTThreadCreate(&pThis->ListenThread, drvTCPListenLoop, (void *)pThis, 0,
                            RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "DrvTCPStream");
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc,  RT_SRC_POS,
                                       N_("DrvTCP#%d failed to create listening thread"), pDrvIns->iInstance);
    }
    else
    {
        char *pszPort = strchr(pThis->pszLocation, ':');
        if (!pszPort)
            return PDMDrvHlpVMSetError(pDrvIns, VERR_NOT_FOUND, RT_SRC_POS,
                                       N_("DrvTCP#%d: The location misses the port to connect to"),
                                       pDrvIns->iInstance);

        *pszPort = '\0'; /* Overwrite temporarily to avoid copying the hostname into a temporary buffer. */
        uint32_t uPort = 0;
        rc = RTStrToUInt32Ex(pszPort + 1, NULL, 10, &uPort);
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                       N_("DrvTCP#%d: The port part of the location is not a numerical value"),
                                       pDrvIns->iInstance);

        rc = RTTcpClientConnect(pThis->pszLocation, uPort, &pThis->hTcpSock);
        *pszPort = ':'; /* Restore delimiter before checking the status. */
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                       N_("DrvTCP#%d failed to connect to socket %s"),
                                       pDrvIns->iInstance, pThis->pszLocation);

        rc = RTPollSetAddSocket(pThis->hPollSet, pThis->hTcpSock,
                                RTPOLL_EVT_READ | RTPOLL_EVT_WRITE | RTPOLL_EVT_ERROR,
                                DRVTCP_POLLSET_ID_SOCKET);
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                       N_("DrvTCP#%d failed to add socket for %s to poll set"),
                                       pDrvIns->iInstance, pThis->pszLocation);

        pThis->fTcpSockInPollSet = true;
    }

    LogRel(("DrvTCP: %s, %s\n", pThis->pszLocation, pThis->fIsServer ? "server" : "client"));
    return VINF_SUCCESS;
}


/**
 * TCP stream driver registration record.
 */
const PDMDRVREG g_DrvTCP =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "TCP",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "TCP serial stream driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_STREAM,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVTCP),
    /* pfnConstruct */
    drvTCPConstruct,
    /* pfnDestruct */
    drvTCPDestruct,
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
    drvTCPPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

