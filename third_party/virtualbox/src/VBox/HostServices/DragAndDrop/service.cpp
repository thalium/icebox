/* $Id: service.cpp $ */
/** @file
 * Drag and Drop Service.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @page pg_svc_guest_control   Drag and drop HGCM Service
 *
 * TODO
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef LOG_GROUP
 #undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND

#include <algorithm>
#include <list>
#include <map>

#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/Service.h>
#include <VBox/HostServices/DragAndDropSvc.h>

#include "dndmanager.h"

using namespace DragAndDropSvc;


/*********************************************************************************************************************************
*   Service class declaration                                                                                                    *
*********************************************************************************************************************************/

class DragAndDropClient : public HGCM::Client
{
public:

    DragAndDropClient(uint32_t uClientId, VBOXHGCMCALLHANDLE hHandle = NULL,
                      uint32_t uMsg = 0, uint32_t cParms = 0, VBOXHGCMSVCPARM aParms[] = NULL)
        : HGCM::Client(uClientId, hHandle, uMsg, cParms, aParms)
        , m_fDeferred(false)
    {
        RT_ZERO(m_SvcCtx);
    }

    virtual ~DragAndDropClient(void)
    {
        disconnect();
    }

public:

    void complete(VBOXHGCMCALLHANDLE hHandle, int rcOp);
    void completeDeferred(int rcOp);
    void disconnect(void);
    bool isDeferred(void) const { return m_fDeferred; }
    void setDeferred(VBOXHGCMCALLHANDLE hHandle, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    void setSvcContext(const HGCM::VBOXHGCMSVCTX &SvcCtx) { m_SvcCtx = SvcCtx; }

protected:

    /** The HGCM service context this client is bound to. */
    HGCM::VBOXHGCMSVCTX m_SvcCtx;
    /** Flag indicating whether this client currently is deferred mode,
     *  meaning that it did not return to the caller yet. */
    bool                m_fDeferred;
};

/** Map holding pointers to drag and drop clients. Key is the (unique) HGCM client ID. */
typedef std::map<uint32_t, DragAndDropClient*> DnDClientMap;

/** Simple queue (list) which holds deferred (waiting) clients. */
typedef std::list<uint32_t> DnDClientQueue;

/**
 * Specialized drag & drop service class.
 */
class DragAndDropService : public HGCM::AbstractService<DragAndDropService>
{
public:

    explicit DragAndDropService(PVBOXHGCMSVCHELPERS pHelpers)
        : HGCM::AbstractService<DragAndDropService>(pHelpers)
        , m_pManager(NULL) {}

protected:

    int  init(VBOXHGCMSVCFNTABLE *pTable);
    int  uninit(void);
    int  clientConnect(uint32_t u32ClientID, void *pvClient);
    int  clientDisconnect(uint32_t u32ClientID, void *pvClient);
    void guestCall(VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID, void *pvClient, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int  hostCall(uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);

    int modeSet(uint32_t u32Mode);
    inline uint32_t modeGet(void) const { return m_u32Mode; };

protected:

    static DECLCALLBACK(int) progressCallback(uint32_t uStatus, uint32_t uPercentage, int rc, void *pvUser);

protected:

    /** Pointer to our DnD manager instance. */
    DnDManager                        *m_pManager;
    /** Map of all connected clients.
     *  The primary key is the (unique) client ID, the secondary value
     *  an allocated pointer to the DragAndDropClient class, managed
     *  by this service class. */
    DnDClientMap                       m_clientMap;
    /** List of all clients which are queued up (deferred return) and ready
     *  to process new commands. The key is the (unique) client ID. */
    DnDClientQueue                     m_clientQueue;
    /** Current drag and drop mode. */
    uint32_t                           m_u32Mode;
};


/*********************************************************************************************************************************
*   Client implementation                                                                                                        *
*********************************************************************************************************************************/

/**
 * Completes the call by returning the control back to the guest
 * side code.
 */
void DragAndDropClient::complete(VBOXHGCMCALLHANDLE hHandle, int rcOp)
{
    LogFlowThisFunc(("uClientID=%RU32\n", m_uClientId));

    if (   m_SvcCtx.pHelpers
        && m_SvcCtx.pHelpers->pfnCallComplete)
    {
        m_SvcCtx.pHelpers->pfnCallComplete(hHandle, rcOp);
    }
}

/**
 * Completes a deferred call by returning the control back to the guest
 * side code.
 */
void DragAndDropClient::completeDeferred(int rcOp)
{
    AssertMsg(m_fDeferred, ("Client %RU32 is not in deferred mode\n", m_uClientId));
    Assert(m_hHandle != NULL);

    LogFlowThisFunc(("uClientID=%RU32\n", m_uClientId));

    complete(m_hHandle, rcOp);
    m_fDeferred = false;
}

/**
 * Called when the HGCM client disconnected on the guest side.
 * This function takes care of the client's data cleanup and also lets the host
 * know that the client has been disconnected.
 *
 */
void DragAndDropClient::disconnect(void)
{
    LogFlowThisFunc(("uClient=%RU32\n", m_uClientId));

    if (isDeferred())
        completeDeferred(VERR_INTERRUPTED);

    /*
     * Let the host know.
     */
    VBOXDNDCBDISCONNECTMSGDATA data;
    RT_ZERO(data);
    /** @todo Magic needed? */
    /** @todo Add context ID. */

    if (m_SvcCtx.pfnHostCallback)
    {
        int rc2 = m_SvcCtx.pfnHostCallback(m_SvcCtx.pvHostData, GUEST_DND_DISCONNECT, &data, sizeof(data));
        if (RT_FAILURE(rc2))
            LogFlowFunc(("Warning: Unable to notify host about client %RU32 disconnect, rc=%Rrc\n", m_uClientId, rc2));
        /* Not fatal. */
    }
}

/**
 * Set the client's status to deferred, meaning that it does not return to the caller
 * on the guest side yet.
 */
void DragAndDropClient::setDeferred(VBOXHGCMCALLHANDLE hHandle, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    LogFlowThisFunc(("uClient=%RU32\n", m_uClientId));

    AssertMsg(m_fDeferred == false, ("Client already in deferred mode\n"));
    m_fDeferred = true;

    m_hHandle = hHandle;
    m_uMsg    = u32Function;
    m_cParms  = cParms;
    m_paParms = paParms;
}


/*********************************************************************************************************************************
*   Service class implementation                                                                                                 *
*********************************************************************************************************************************/

int DragAndDropService::init(VBOXHGCMSVCFNTABLE *pTable)
{
    /* Register functions. */
    pTable->pfnHostCall          = svcHostCall;
    pTable->pfnSaveState         = NULL;  /* The service is stateless, so the normal */
    pTable->pfnLoadState         = NULL;  /* construction done before restoring suffices */
    pTable->pfnRegisterExtension = svcRegisterExtension;

    /* Drag'n drop mode is disabled by default. */
    modeSet(VBOX_DRAG_AND_DROP_MODE_OFF);

    int rc = VINF_SUCCESS;

    try
    {
        m_pManager = new DnDManager(&DragAndDropService::progressCallback, this);
    }
    catch(std::bad_alloc &)
    {
        rc = VERR_NO_MEMORY;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int DragAndDropService::uninit(void)
{
    LogFlowFuncEnter();

    if (m_pManager)
    {
        delete m_pManager;
        m_pManager = NULL;
    }

    DnDClientMap::iterator itClient =  m_clientMap.begin();
    while (itClient != m_clientMap.end())
    {
        delete itClient->second;
        m_clientMap.erase(itClient);
        itClient = m_clientMap.begin();
    }

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

int DragAndDropService::clientConnect(uint32_t u32ClientID, void *pvClient)
{
    RT_NOREF1(pvClient);
    if (m_clientMap.size() >= UINT8_MAX) /* Don't allow too much clients at the same time. */
    {
        AssertMsgFailed(("Maximum number of clients reached\n"));
        return VERR_MAX_PROCS_REACHED;
    }

    int rc = VINF_SUCCESS;

    /*
     * Add client to our client map.
     */
    if (m_clientMap.find(u32ClientID) != m_clientMap.end())
        rc = VERR_ALREADY_EXISTS;

    if (RT_SUCCESS(rc))
    {
        try
        {
            DragAndDropClient *pClient = new DragAndDropClient(u32ClientID);
            pClient->setSvcContext(m_SvcCtx);
            m_clientMap[u32ClientID] = pClient;
        }
        catch(std::bad_alloc &)
        {
            rc = VERR_NO_MEMORY;
        }

        if (RT_SUCCESS(rc))
        {
            /*
             * Clear the message queue as soon as a new clients connect
             * to ensure that every client has the same state.
             */
            if (m_pManager)
                m_pManager->clear();
        }
    }

    LogFlowFunc(("Client %RU32 connected, rc=%Rrc\n", u32ClientID, rc));
    return rc;
}

int DragAndDropService::clientDisconnect(uint32_t u32ClientID, void *pvClient)
{
    RT_NOREF1(pvClient);

    /* Client not found? Bail out early. */
    DnDClientMap::iterator itClient =  m_clientMap.find(u32ClientID);
    if (itClient == m_clientMap.end())
        return VERR_NOT_FOUND;

    /*
     * Remove from waiters queue.
     */
    m_clientQueue.remove(u32ClientID);

    /*
     * Remove from client map and deallocate.
     */
    AssertPtr(itClient->second);
    delete itClient->second;

    m_clientMap.erase(itClient);

    LogFlowFunc(("Client %RU32 disconnected\n", u32ClientID));
    return VINF_SUCCESS;
}

int DragAndDropService::modeSet(uint32_t u32Mode)
{
#ifndef VBOX_WITH_DRAG_AND_DROP_GH
    if (   u32Mode == VBOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST
        || u32Mode == VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL)
    {
        m_u32Mode = VBOX_DRAG_AND_DROP_MODE_OFF;
        return VERR_NOT_SUPPORTED;
    }
#endif

    switch (u32Mode)
    {
        case VBOX_DRAG_AND_DROP_MODE_OFF:
        case VBOX_DRAG_AND_DROP_MODE_HOST_TO_GUEST:
        case VBOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST:
        case VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL:
            m_u32Mode = u32Mode;
            break;

        default:
            m_u32Mode = VBOX_DRAG_AND_DROP_MODE_OFF;
            break;
    }

    return VINF_SUCCESS;
}

void DragAndDropService::guestCall(VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID,
                                   void *pvClient, uint32_t u32Function,
                                   uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF1(pvClient);
    LogFlowFunc(("u32ClientID=%RU32, u32Function=%RU32, cParms=%RU32\n",
                 u32ClientID, u32Function, cParms));

    /* Check if we've the right mode set. */
    int rc = VERR_ACCESS_DENIED; /* Play safe. */
    switch (u32Function)
    {
        case GUEST_DND_GET_NEXT_HOST_MSG:
        {
            if (modeGet() != VBOX_DRAG_AND_DROP_MODE_OFF)
            {
                rc = VINF_SUCCESS;
            }
            else
            {
                LogFlowFunc(("DnD disabled, deferring request\n"));
                rc = VINF_HGCM_ASYNC_EXECUTE;
            }
            break;
        }

        /* New since protocol v2. */
        case GUEST_DND_CONNECT:
        {
            /*
             * Never block the initial connect call, as the clients do this when
             * initializing and might get stuck if drag and drop is set to "disabled" at
             * that time.
             */
            rc = VINF_SUCCESS;
            break;
        }
        case GUEST_DND_HG_ACK_OP:
            /* Fall through is intentional. */
        case GUEST_DND_HG_REQ_DATA:
            /* Fall through is intentional. */
        case GUEST_DND_HG_EVT_PROGRESS:
        {
            if (   modeGet() == VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL
                || modeGet() == VBOX_DRAG_AND_DROP_MODE_HOST_TO_GUEST)
            {
                rc = VINF_SUCCESS;
            }
            else
                LogFlowFunc(("Host -> Guest DnD mode disabled, ignoring request\n"));
            break;
        }

        case GUEST_DND_GH_ACK_PENDING:
        case GUEST_DND_GH_SND_DATA_HDR:
        case GUEST_DND_GH_SND_DATA:
        case GUEST_DND_GH_SND_DIR:
        case GUEST_DND_GH_SND_FILE_HDR:
        case GUEST_DND_GH_SND_FILE_DATA:
        case GUEST_DND_GH_EVT_ERROR:
        {
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
            if (   modeGet() == VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL
                || modeGet() == VBOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST)
            {
                rc = VINF_SUCCESS;
            }
            else
#endif
                LogFlowFunc(("Guest -> Host DnD mode disabled, ignoring request\n"));
            break;
        }

        default:
            /* Reach through to DnD manager. */
            rc = VINF_SUCCESS;
            break;
    }

#ifdef DEBUG_andy
    LogFlowFunc(("Mode (%RU32) check rc=%Rrc\n", modeGet(), rc));
#endif

#define DO_HOST_CALLBACK();                                                                   \
    if (   RT_SUCCESS(rc)                                                                     \
        && m_SvcCtx.pfnHostCallback)                                                          \
    {                                                                                         \
        rc = m_SvcCtx.pfnHostCallback(m_SvcCtx.pvHostData, u32Function, &data, sizeof(data)); \
    }

    /*
     * Lookup client.
     */
    DragAndDropClient *pClient = NULL;

    DnDClientMap::iterator itClient =  m_clientMap.find(u32ClientID);
    if (itClient != m_clientMap.end())
    {
        pClient = itClient->second;
        AssertPtr(pClient);
    }
    else
    {
        LogFunc(("Client %RU32 was not found\n", u32ClientID));
        rc = VERR_NOT_FOUND;
    }

    if (rc == VINF_SUCCESS) /* Note: rc might be VINF_HGCM_ASYNC_EXECUTE! */
    {
        LogFlowFunc(("Client %RU32: Protocol v%RU32\n", pClient->clientId(), pClient->protocol()));

        rc = VERR_INVALID_PARAMETER; /* Play safe. */

        switch (u32Function)
        {
            /*
             * Note: Older VBox versions with enabled DnD guest->host support (< 5.0)
             *       used the same message ID (300) for GUEST_DND_GET_NEXT_HOST_MSG and
             *       HOST_DND_GH_REQ_PENDING, which led this service returning
             *       VERR_INVALID_PARAMETER when the guest wanted to actually
             *       handle HOST_DND_GH_REQ_PENDING.
             */
            case GUEST_DND_GET_NEXT_HOST_MSG:
            {
                LogFlowFunc(("GUEST_DND_GET_NEXT_HOST_MSG\n"));
                if (cParms == 3)
                {
                    rc = m_pManager->nextMessageInfo(&paParms[0].u.uint32 /* uMsg */, &paParms[1].u.uint32 /* cParms */);
                    if (RT_FAILURE(rc)) /* No queued messages available? */
                    {
                        if (m_SvcCtx.pfnHostCallback) /* Try asking the host. */
                        {
                            VBOXDNDCBHGGETNEXTHOSTMSG data;
                            RT_ZERO(data);
                            data.hdr.uMagic = CB_MAGIC_DND_HG_GET_NEXT_HOST_MSG;
                            rc = m_SvcCtx.pfnHostCallback(m_SvcCtx.pvHostData, u32Function, &data, sizeof(data));
                            if (RT_SUCCESS(rc))
                            {
                                paParms[0].u.uint32 = data.uMsg;   /* uMsg */
                                paParms[1].u.uint32 = data.cParms; /* cParms */
                                /* Note: paParms[2] was set by the guest as blocking flag. */
                            }
                        }
                        else /* No host callback in place, so drag and drop is not supported by the host. */
                            rc = VERR_NOT_SUPPORTED;

                        if (RT_FAILURE(rc))
                            rc = m_pManager->nextMessage(u32Function, cParms, paParms);

                        /* Some error occurred or no (new) messages available? */
                        if (RT_FAILURE(rc))
                        {
                            uint32_t fFlags = 0;
                            int rc2 = paParms[2].getUInt32(&fFlags);
                            if (   RT_SUCCESS(rc2)
                                && fFlags) /* Blocking flag set? */
                            {
                                /* Defer client returning. */
                                rc = VINF_HGCM_ASYNC_EXECUTE;
                            }
                            else
                                rc = VERR_INVALID_PARAMETER;

                            LogFlowFunc(("Message queue is empty, returning %Rrc to guest\n", rc));
                        }
                    }
                }
                break;
            }
            case GUEST_DND_CONNECT:
            {
                LogFlowFunc(("GUEST_DND_CONNECT\n"));
                if (cParms >= 2)
                {
                    const uint8_t idxProto = cParms >= 3 ? 1 : 0;

                    VBOXDNDCBCONNECTMSGDATA data;
                    RT_ZERO(data);
                    data.hdr.uMagic = CB_MAGIC_DND_CONNECT;
                    if (cParms >= 3)
                        rc = paParms[0].getUInt32(&data.hdr.uContextID);
                    else /* Older protocols don't have a context ID. */
                        rc = VINF_SUCCESS;
                    if (RT_SUCCESS(rc))
                        rc = paParms[idxProto].getUInt32(&data.uProtocol);
                    if (RT_SUCCESS(rc))
                        rc = paParms[idxProto + 1].getUInt32(&data.uFlags);
                    if (RT_SUCCESS(rc))
                        rc = pClient->setProtocol(data.uProtocol);
                    if (RT_SUCCESS(rc))
                    {
                        LogFlowFunc(("Client %RU32 is now using protocol v%RU32\n", pClient->clientId(), pClient->protocol()));
                        DO_HOST_CALLBACK();
                    }
                }
                break;
            }
            case GUEST_DND_HG_ACK_OP:
            {
                LogFlowFunc(("GUEST_DND_HG_ACK_OP\n"));

                VBOXDNDCBHGACKOPDATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_HG_ACK_OP;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 2)
                        {
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.uAction); /* Get drop action. */
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 1)
                            rc = paParms[0].getUInt32(&data.uAction); /* Get drop action. */
                        break;
                    }
                }

                DO_HOST_CALLBACK();
                break;
            }
            case GUEST_DND_HG_REQ_DATA:
            {
                LogFlowFunc(("GUEST_DND_HG_REQ_DATA\n"));

                VBOXDNDCBHGREQDATADATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_HG_REQ_DATA;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 3)
                        {
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getPointer((void **)&data.pszFormat, &data.cbFormat);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.cbFormat);
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 1)
                            rc = paParms[0].getPointer((void**)&data.pszFormat, &data.cbFormat);
                        break;
                    }
                }

                DO_HOST_CALLBACK();
                break;
            }
            case GUEST_DND_HG_EVT_PROGRESS:
            {
                LogFlowFunc(("GUEST_DND_HG_EVT_PROGRESS\n"));

                VBOXDNDCBHGEVTPROGRESSDATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_HG_EVT_PROGRESS;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 4)
                        {
                            rc = paParms[0].getUInt32(&data.uStatus);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.uStatus);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.uPercentage);
                            if (RT_SUCCESS(rc))
                                rc = paParms[3].getUInt32(&data.rc);
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 3)
                        {
                            rc = paParms[0].getUInt32(&data.uStatus);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.uPercentage);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.rc);
                        }
                        break;
                    }
                }

                DO_HOST_CALLBACK();
                break;
            }
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
            case GUEST_DND_GH_ACK_PENDING:
            {
                LogFlowFunc(("GUEST_DND_GH_ACK_PENDING\n"));

                VBOXDNDCBGHACKPENDINGDATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_GH_ACK_PENDING;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 5)
                        {
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.uDefAction);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.uAllActions);
                            if (RT_SUCCESS(rc))
                                rc = paParms[3].getPointer((void**)&data.pszFormat, &data.cbFormat);
                            if (RT_SUCCESS(rc))
                                rc = paParms[4].getUInt32(&data.cbFormat);
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 3)
                        {
                            rc = paParms[0].getUInt32(&data.uDefAction);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.uAllActions);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getPointer((void**)&data.pszFormat, &data.cbFormat);
                        }
                        break;
                    }
                }

                DO_HOST_CALLBACK();
                break;
            }
            /* New since protocol v3. */
            case GUEST_DND_GH_SND_DATA_HDR:
            {
                LogFlowFunc(("GUEST_DND_GH_SND_DATA_HDR\n"));
                if (cParms == 12)
                {
                    VBOXDNDCBSNDDATAHDRDATA data;
                    RT_ZERO(data);
                    data.hdr.uMagic = CB_MAGIC_DND_GH_SND_DATA_HDR;
                    rc = paParms[0].getUInt32(&data.hdr.uContextID);
                    if (RT_SUCCESS(rc))
                        rc = paParms[1].getUInt32(&data.data.uFlags);
                    if (RT_SUCCESS(rc))
                        rc = paParms[2].getUInt32(&data.data.uScreenId);
                    if (RT_SUCCESS(rc))
                        rc = paParms[3].getUInt64(&data.data.cbTotal);
                    if (RT_SUCCESS(rc))
                        rc = paParms[4].getUInt32(&data.data.cbMeta);
                    if (RT_SUCCESS(rc))
                        rc = paParms[5].getPointer(&data.data.pvMetaFmt, &data.data.cbMetaFmt);
                    if (RT_SUCCESS(rc))
                        rc = paParms[6].getUInt32(&data.data.cbMetaFmt);
                    if (RT_SUCCESS(rc))
                        rc = paParms[7].getUInt64(&data.data.cObjects);
                    if (RT_SUCCESS(rc))
                        rc = paParms[8].getUInt32(&data.data.enmCompression);
                    if (RT_SUCCESS(rc))
                        rc = paParms[9].getUInt32((uint32_t *)&data.data.enmChecksumType);
                    if (RT_SUCCESS(rc))
                        rc = paParms[10].getPointer(&data.data.pvChecksum, &data.data.cbChecksum);
                    if (RT_SUCCESS(rc))
                        rc = paParms[11].getUInt32(&data.data.cbChecksum);

                    LogFlowFunc(("fFlags=0x%x, cbTotalSize=%RU64, cObj=%RU64\n",
                                 data.data.uFlags, data.data.cbTotal, data.data.cObjects));
                    DO_HOST_CALLBACK();
                }
                break;
            }
            case GUEST_DND_GH_SND_DATA:
            {
                LogFlowFunc(("GUEST_DND_GH_SND_DATA\n"));
                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 5)
                        {
                            VBOXDNDCBSNDDATADATA data;
                            RT_ZERO(data);
                            data.hdr.uMagic = CB_MAGIC_DND_GH_SND_DATA;
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getPointer((void**)&data.data.u.v3.pvData, &data.data.u.v3.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.data.u.v3.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[3].getPointer((void**)&data.data.u.v3.pvChecksum, &data.data.u.v3.cbChecksum);
                            if (RT_SUCCESS(rc))
                                rc = paParms[4].getUInt32(&data.data.u.v3.cbChecksum);
                            DO_HOST_CALLBACK();
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 2)
                        {
                            VBOXDNDCBSNDDATADATA data;
                            RT_ZERO(data);
                            data.hdr.uMagic = CB_MAGIC_DND_GH_SND_DATA;
                            rc = paParms[0].getPointer((void**)&data.data.u.v1.pvData, &data.data.u.v1.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.data.u.v1.cbTotalSize);
                            DO_HOST_CALLBACK();
                        }
                        break;
                    }
                }
                break;
            }
            case GUEST_DND_GH_SND_DIR:
            {
                LogFlowFunc(("GUEST_DND_GH_SND_DIR\n"));

                VBOXDNDCBSNDDIRDATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_GH_SND_DIR;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 4)
                        {
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getPointer((void**)&data.pszPath, &data.cbPath);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.cbPath);
                            if (RT_SUCCESS(rc))
                                rc = paParms[3].getUInt32(&data.fMode);
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 3)
                        {
                            rc = paParms[0].getPointer((void**)&data.pszPath, &data.cbPath);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.cbPath);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.fMode);
                        }
                        break;
                    }
                }

                DO_HOST_CALLBACK();
                break;
            }
            /* New since protocol v2 (>= VBox 5.0). */
            case GUEST_DND_GH_SND_FILE_HDR:
            {
                LogFlowFunc(("GUEST_DND_GH_SND_FILE_HDR\n"));
                if (cParms == 6)
                {
                    VBOXDNDCBSNDFILEHDRDATA data;
                    RT_ZERO(data);
                    data.hdr.uMagic = CB_MAGIC_DND_GH_SND_FILE_HDR;

                    rc = paParms[0].getUInt32(&data.hdr.uContextID);
                    if (RT_SUCCESS(rc))
                        rc = paParms[1].getPointer((void**)&data.pszFilePath, &data.cbFilePath);
                    if (RT_SUCCESS(rc))
                        rc = paParms[2].getUInt32(&data.cbFilePath);
                    if (RT_SUCCESS(rc))
                        rc = paParms[3].getUInt32(&data.fFlags);
                    if (RT_SUCCESS(rc))
                        rc = paParms[4].getUInt32(&data.fMode);
                    if (RT_SUCCESS(rc))
                        rc = paParms[5].getUInt64(&data.cbSize);

                    LogFlowFunc(("pszPath=%s, cbPath=%RU32, fMode=0x%x, cbSize=%RU64\n",
                                 data.pszFilePath, data.cbFilePath, data.fMode, data.cbSize));
                    DO_HOST_CALLBACK();
                }
                break;
            }
            case GUEST_DND_GH_SND_FILE_DATA:
            {
                LogFlowFunc(("GUEST_DND_GH_SND_FILE_DATA\n"));

                switch (pClient->protocol())
                {
                    /* Protocol v3 adds (optional) checksums. */
                    case 3:
                    {
                        if (cParms == 5)
                        {
                            VBOXDNDCBSNDFILEDATADATA data;
                            RT_ZERO(data);
                            data.hdr.uMagic = CB_MAGIC_DND_GH_SND_FILE_DATA;

                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getPointer((void**)&data.pvData, &data.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[3].getPointer((void**)&data.u.v3.pvChecksum, &data.u.v3.cbChecksum);
                            if (RT_SUCCESS(rc))
                                rc = paParms[4].getUInt32(&data.u.v3.cbChecksum);

                            LogFlowFunc(("pvData=0x%p, cbData=%RU32\n", data.pvData, data.cbData));
                            DO_HOST_CALLBACK();
                        }
                        break;
                    }
                    /* Protocol v2 only sends the next data chunks to reduce traffic. */
                    case 2:
                    {
                        if (cParms == 3)
                        {
                            VBOXDNDCBSNDFILEDATADATA data;
                            RT_ZERO(data);
                            data.hdr.uMagic = CB_MAGIC_DND_GH_SND_FILE_DATA;
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getPointer((void**)&data.pvData, &data.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getUInt32(&data.cbData);

                            LogFlowFunc(("cbData=%RU32, pvData=0x%p\n", data.cbData, data.pvData));
                            DO_HOST_CALLBACK();
                        }
                        break;
                    }
                    /* Protocol v1 sends the file path and attributes for every file chunk (!). */
                    default:
                    {
                        if (cParms == 5)
                        {
                            VBOXDNDCBSNDFILEDATADATA data;
                            RT_ZERO(data);
                            data.hdr.uMagic = CB_MAGIC_DND_GH_SND_FILE_DATA;
                            uint32_t cTmp;
                            rc = paParms[0].getPointer((void**)&data.u.v1.pszFilePath, &cTmp);
                            if (RT_SUCCESS(rc))
                                rc = paParms[1].getUInt32(&data.u.v1.cbFilePath);
                            if (RT_SUCCESS(rc))
                                rc = paParms[2].getPointer((void**)&data.pvData, &cTmp);
                            if (RT_SUCCESS(rc))
                                rc = paParms[3].getUInt32(&data.cbData);
                            if (RT_SUCCESS(rc))
                                rc = paParms[4].getUInt32(&data.u.v1.fMode);

                            LogFlowFunc(("pszFilePath=%s, cbData=%RU32, pvData=0x%p, fMode=0x%x\n",
                                         data.u.v1.pszFilePath, data.cbData, data.pvData, data.u.v1.fMode));
                            DO_HOST_CALLBACK();
                        }
                        break;
                    }
                }
                break;
            }
            case GUEST_DND_GH_EVT_ERROR:
            {
                LogFlowFunc(("GUEST_DND_GH_EVT_ERROR\n"));

                VBOXDNDCBEVTERRORDATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_GH_EVT_ERROR;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        if (cParms == 2)
                        {
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);
                            if (RT_SUCCESS(rc))
                            {
                                uint32_t rcOp;
                                rc = paParms[1].getUInt32(&rcOp);
                                if (RT_SUCCESS(rc))
                                    data.rc = rcOp;
                            }
                        }
                        break;
                    }

                    case 2:
                    default:
                    {
                        if (cParms == 1)
                        {
                            uint32_t rcOp;
                            rc = paParms[0].getUInt32(&rcOp);
                            if (RT_SUCCESS(rc))
                                data.rc = (int32_t)rcOp;
                        }
                        break;
                    }
                }

                DO_HOST_CALLBACK();
                break;
            }
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

            /*
             * Note: This is a fire-and-forget message, as the host should
             *       not rely on an answer from the guest side in order to
             *       properly cancel the operation.
             */
            case HOST_DND_HG_EVT_CANCEL:
            {
                LogFlowFunc(("HOST_DND_HG_EVT_CANCEL\n"));

                VBOXDNDCBEVTERRORDATA data;
                RT_ZERO(data);
                data.hdr.uMagic = CB_MAGIC_DND_GH_EVT_ERROR;

                switch (pClient->protocol())
                {
                    case 3:
                    {
                        /* Protocol v3+ at least requires the context ID. */
                        if (cParms == 1)
                            rc = paParms[0].getUInt32(&data.hdr.uContextID);

                        break;
                    }

                    default:
                        break;
                }

                /* Tell the host that the guest has cancelled the operation. */
                data.rc = VERR_CANCELLED;

                DO_HOST_CALLBACK();

                /* Note: If the host is not prepared for handling the cancelling reply
                 *       from the guest, don't report this back to the guest. */
                if (RT_FAILURE(rc))
                    rc = VINF_SUCCESS;
                break;
            }

            default:
            {
                /* All other messages are handled by the DnD manager. */
                rc = m_pManager->nextMessage(u32Function, cParms, paParms);
                if (rc == VERR_NO_DATA) /* Manager has no new messsages? Try asking the host. */
                {
                    if (m_SvcCtx.pfnHostCallback)
                    {
                        VBOXDNDCBHGGETNEXTHOSTMSGDATA data;
                        RT_ZERO(data);

                        data.hdr.uMagic = VBOX_DND_CB_MAGIC_MAKE(0 /* uFn */, 0 /* uVer */);

                        data.uMsg    = u32Function;
                        data.cParms  = cParms;
                        data.paParms = paParms;

                        rc = m_SvcCtx.pfnHostCallback(m_SvcCtx.pvHostData, u32Function,
                                                      &data, sizeof(data));
                        if (RT_SUCCESS(rc))
                        {
                            cParms  = data.cParms;
                            paParms = data.paParms;
                        }
                        else
                        {
                            /*
                             * In case the guest is too fast asking for the next message
                             * and the host did not supply it yet, just defer the client's
                             * return until a response from the host available.
                             */
                            LogFlowFunc(("No new messages from the host (yet), deferring request: %Rrc\n", rc));
                            rc = VINF_HGCM_ASYNC_EXECUTE;
                        }
                    }
                    else /* No host callback in place, so drag and drop is not supported by the host. */
                        rc = VERR_NOT_SUPPORTED;
                }
                break;
            }
        }
    }

    /*
     * If async execution is requested, we didn't notify the guest yet about
     * completion. The client is queued into the waiters list and will be
     * notified as soon as a new event is available.
     */
    if (rc == VINF_HGCM_ASYNC_EXECUTE)
    {
        try
        {
            AssertPtr(pClient);
            pClient->setDeferred(callHandle, u32Function, cParms, paParms);
            m_clientQueue.push_back(u32ClientID);
        }
        catch (std::bad_alloc)
        {
            rc = VERR_NO_MEMORY;
            /* Don't report to guest. */
        }
    }
    else if (pClient)
        pClient->complete(callHandle, rc);
    else
    {
        AssertMsgFailed(("Guest call failed with %Rrc\n", rc));
        rc = VERR_NOT_IMPLEMENTED;
    }

    LogFlowFunc(("Returning rc=%Rrc\n", rc));
}

int DragAndDropService::hostCall(uint32_t u32Function,
                                 uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    LogFlowFunc(("u32Function=%RU32, cParms=%RU32, cClients=%zu, cQueue=%zu\n",
                 u32Function, cParms, m_clientMap.size(), m_clientQueue.size()));

    int rc;

    do
    {
        bool fSendToGuest = false; /* Whether to send the message down to the guest side or not. */

        switch (u32Function)
        {
            case HOST_DND_SET_MODE:
            {
                if (cParms != 1)
                    rc = VERR_INVALID_PARAMETER;
                else if (paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT)
                    rc = VERR_INVALID_PARAMETER;
                else
                    rc = modeSet(paParms[0].u.uint32);
                break;
            }

            case HOST_DND_HG_EVT_ENTER:
            {
                /* Clear the message queue as a new DnD operation just began. */
                m_pManager->clear();

                fSendToGuest = true;
                rc = VINF_SUCCESS;
                break;
            }

            case HOST_DND_HG_EVT_CANCEL:
            {
                LogFlowFunc(("Cancelling all waiting clients ...\n"));

                /* Clear the message queue as the host cancelled the whole operation. */
                m_pManager->clear();

                /*
                 * Wake up all deferred clients and tell them to process
                 * the cancelling message next.
                 */
                DnDClientQueue::iterator itQueue = m_clientQueue.begin();
                while (itQueue != m_clientQueue.end())
                {
                    DnDClientMap::iterator itClient = m_clientMap.find(*itQueue);
                    Assert(itClient != m_clientMap.end());

                    DragAndDropClient *pClient = itClient->second;
                    AssertPtr(pClient);

                    int rc2 = pClient->addMessageInfo(HOST_DND_HG_EVT_CANCEL,
                                                      /* Protocol v3+ also contains the context ID. */
                                                      pClient->protocol() >= 3 ? 1 : 0);
                    pClient->completeDeferred(rc2);

                    m_clientQueue.erase(itQueue);
                    itQueue = m_clientQueue.begin();
                }

                Assert(m_clientQueue.size() == 0);

                /* Tell the host that everything went well. */
                rc = VINF_SUCCESS;
                break;
            }

            default:
            {
                fSendToGuest = true;
                rc = VINF_SUCCESS;
                break;
            }
        }

        if (fSendToGuest)
        {
            if (modeGet() == VBOX_DRAG_AND_DROP_MODE_OFF)
            {
                /* Tell the host that a wrong drag'n drop mode is set. */
                rc = VERR_ACCESS_DENIED;
                break;
            }

            if (m_clientMap.size() == 0) /* At least one client on the guest connected? */
            {
                /*
                 * Tell the host that the guest does not support drag'n drop.
                 * This might happen due to not installed Guest Additions or
                 * not running VBoxTray/VBoxClient.
                 */
                rc = VERR_NOT_SUPPORTED;
                break;
            }

            rc = m_pManager->addMessage(u32Function, cParms, paParms, true /* fAppend */);
            if (RT_FAILURE(rc))
            {
                AssertMsgFailed(("Adding new message of type=%RU32 failed with rc=%Rrc\n", u32Function, rc));
                break;
            }

            /* Any clients in our queue ready for processing the next command? */
            if (m_clientQueue.size() == 0)
            {
                LogFlowFunc(("All clients (%zu) busy -- delaying execution\n", m_clientMap.size()));
                break;
            }

            uint32_t uClientNext = m_clientQueue.front();
            DnDClientMap::iterator itClientNext = m_clientMap.find(uClientNext);
            Assert(itClientNext != m_clientMap.end());

            DragAndDropClient *pClient = itClientNext->second;
            AssertPtr(pClient);

            /*
             * Check if this was a request for getting the next host
             * message. If so, return the message ID and the parameter
             * count. The message itself has to be queued.
             */
            uint32_t uMsgClient = pClient->message();

            uint32_t uMsgNext   = 0;
            uint32_t cParmsNext = 0;
            int rcNext = m_pManager->nextMessageInfo(&uMsgNext, &cParmsNext);

            LogFlowFunc(("uMsgClient=%RU32, uMsgNext=%RU32, cParmsNext=%RU32, rcNext=%Rrc\n",
                         uMsgClient, uMsgNext, cParmsNext, rcNext));

            if (RT_SUCCESS(rcNext))
            {
                if (uMsgClient == GUEST_DND_GET_NEXT_HOST_MSG)
                {
                    rc = pClient->addMessageInfo(uMsgNext, cParmsNext);

                    /* Note: Report the current rc back to the guest. */
                    pClient->completeDeferred(rc);
                }
                /*
                 * Does the message the client is waiting for match the message
                 * next in the queue? Process it right away then.
                 */
                else if (uMsgClient == uMsgNext)
                {
                    rc = m_pManager->nextMessage(u32Function, cParms, paParms);

                    /* Note: Report the current rc back to the guest. */
                    pClient->completeDeferred(rc);
                }
                else /* Should not happen; cancel the operation on the guest. */
                {
                    LogFunc(("Client ID=%RU32 in wrong state with uMsg=%RU32 (next message in queue: %RU32), cancelling\n",
                             pClient->clientId(), uMsgClient, uMsgNext));

                    pClient->completeDeferred(VERR_CANCELLED);
                }

                m_clientQueue.pop_front();
            }

        } /* fSendToGuest */

    } while (0); /* To use breaks. */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

DECLCALLBACK(int) DragAndDropService::progressCallback(uint32_t uStatus, uint32_t uPercentage, int rc, void *pvUser)
{
    AssertPtrReturn(pvUser, VERR_INVALID_POINTER);

    DragAndDropService *pSelf = static_cast<DragAndDropService *>(pvUser);
    AssertPtr(pSelf);

    if (pSelf->m_SvcCtx.pfnHostCallback)
    {
        LogFlowFunc(("GUEST_DND_HG_EVT_PROGRESS: uStatus=%RU32, uPercentage=%RU32, rc=%Rrc\n",
                     uStatus, uPercentage, rc));

        VBOXDNDCBHGEVTPROGRESSDATA data;
        data.hdr.uMagic = CB_MAGIC_DND_HG_EVT_PROGRESS;
        data.uPercentage  = RT_MIN(uPercentage, 100);
        data.uStatus      = uStatus;
        data.rc           = rc; /** @todo uin32_t vs. int. */

        return pSelf->m_SvcCtx.pfnHostCallback(pSelf->m_SvcCtx.pvHostData,
                                               GUEST_DND_HG_EVT_PROGRESS,
                                               &data, sizeof(data));
    }

    return VINF_SUCCESS;
}

/**
 * @copydoc VBOXHGCMSVCLOAD
 */
extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad(VBOXHGCMSVCFNTABLE *pTable)
{
    return DragAndDropService::svcLoad(pTable);
}

