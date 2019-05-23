/* $Id: service.cpp $ */
/** @file
 * Guest Control Service: Controlling the guest.
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

/** @page pg_svc_guest_control   Guest Control HGCM Service
 *
 * This service acts as a proxy for handling and buffering host command requests
 * and clients on the guest. It tries to be as transparent as possible to let
 * the guest (client) and host side do their protocol handling as desired.
 *
 * The following terms are used:
 * - Host:   A host process (e.g. VBoxManage or another tool utilizing the Main API)
 *           which wants to control something on the guest.
 * - Client: A client (e.g. VBoxService) running inside the guest OS waiting for
 *           new host commands to perform. There can be multiple clients connected
 *           to this service. A client is represented by its unique HGCM client ID.
 * - Context ID: An (almost) unique ID automatically generated on the host (Main API)
 *               to not only distinguish clients but individual requests. Because
 *               the host does not know anything about connected clients it needs
 *               an indicator which it can refer to later. This context ID gets
 *               internally bound by the service to a client which actually processes
 *               the command in order to have a relationship between client<->context ID(s).
 *
 * The host can trigger commands which get buffered by the service (with full HGCM
 * parameter info). As soon as a client connects (or is ready to do some new work)
 * it gets a buffered host command to process it. This command then will be immediately
 * removed from the command list. If there are ready clients but no new commands to be
 * processed, these clients will be set into a deferred state (that is being blocked
 * to return until a new command is available).
 *
 * If a client needs to inform the host that something happened, it can send a
 * message to a low level HGCM callback registered in Main. This callback contains
 * the actual data as well as the context ID to let the host do the next necessary
 * steps for this context. This context ID makes it possible to wait for an event
 * inside the host's Main API function (like starting a process on the guest and
 * wait for getting its PID returned by the client) as well as cancelling blocking
 * host calls in order the client terminated/crashed (HGCM detects disconnected
 * clients and reports it to this service's callback).
 *
 * Starting at VBox 4.2 the context ID itself consists of a session ID, an object
 * ID (for example a process or file ID) and a count. This is necessary to not break
 * compatibility between older hosts and to manage guest session on the host.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef LOG_GROUP
 #undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_CONTROL
#include <VBox/HostServices/GuestControlSvc.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/cpp/autores.h>
#include <iprt/cpp/utils.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/list.h>
#include <iprt/req.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/time.h>

#include <map>
#include <memory>  /* for auto_ptr */
#include <string>
#include <list>


namespace guestControl {

/** Flag for indicating that the client only is interested in
 *  messages of specific context IDs. */
#define CLIENTSTATE_FLAG_CONTEXTFILTER      RT_BIT(0)

/**
 * Structure for maintaining a pending (that is, a deferred and not yet completed)
 * client command.
 */
typedef struct ClientConnection
{
    /** The call handle */
    VBOXHGCMCALLHANDLE mHandle;
    /** Number of parameters */
    uint32_t mNumParms;
    /** The call parameters */
    VBOXHGCMSVCPARM *mParms;
    /** The standard constructor. */
    ClientConnection(void)
        : mHandle(0), mNumParms(0), mParms(NULL) {}
} ClientConnection;

/**
 * Structure for holding a buffered host command which has
 * not been processed yet.
 */
typedef struct HostCommand
{
    RTLISTNODE Node;

    uint32_t AddRef(void)
    {
#ifdef DEBUG_andy
        LogFlowFunc(("Adding reference pHostCmd=%p, CID=%RU32, new refCount=%RU32\n",
                     this, mContextID, mRefCount + 1));
#endif
        return ++mRefCount;
    }

    uint32_t Release(void)
    {
#ifdef DEBUG_andy
        LogFlowFunc(("Releasing reference pHostCmd=%p, CID=%RU32, new refCount=%RU32\n",
                     this, mContextID, mRefCount - 1));
#endif
        /* Release reference for current command. */
        Assert(mRefCount);
        if (--mRefCount == 0)
            Free();

        return mRefCount;
    }

    /**
     * Allocates the command with an HGCM request. Needs to be free'd using Free().
     *
     * @return  IPRT status code.
     * @param   uMsg                    Message type.
     * @param   cParms                  Number of parameters of HGCM request.
     * @param   paParms                 Array of parameters of HGCM request.
     */
    int Allocate(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
    {
        LogFlowFunc(("Allocating pHostCmd=%p, uMsg=%RU32, cParms=%RU32, paParms=%p\n",
                     this, uMsg, cParms, paParms));

        if (!cParms) /* At least one parameter (context ID) must be present. */
            return VERR_INVALID_PARAMETER;

        AssertPtrReturn(paParms, VERR_INVALID_POINTER);

        /* Paranoia. */
        if (cParms > 256)
            cParms = 256;

        int rc = VINF_SUCCESS;

        /*
         * Don't verify anything here (yet), because this function only buffers
         * the HGCM data into an internal structure and reaches it back to the guest (client)
         * in an unmodified state.
         */
        mMsgType = uMsg;
        mParmCount = cParms;
        if (mParmCount)
        {
            mpParms = (VBOXHGCMSVCPARM*)RTMemAllocZ(sizeof(VBOXHGCMSVCPARM) * mParmCount);
            if (NULL == mpParms)
                rc = VERR_NO_MEMORY;
        }

        if (RT_SUCCESS(rc))
        {
            for (uint32_t i = 0; i < mParmCount; i++)
            {
                mpParms[i].type = paParms[i].type;
                switch (paParms[i].type)
                {
                    case VBOX_HGCM_SVC_PARM_32BIT:
                        mpParms[i].u.uint32 = paParms[i].u.uint32;
                        break;

                    case VBOX_HGCM_SVC_PARM_64BIT:
                        mpParms[i].u.uint64 = paParms[i].u.uint64;
                        break;

                    case VBOX_HGCM_SVC_PARM_PTR:
                        mpParms[i].u.pointer.size = paParms[i].u.pointer.size;
                        if (mpParms[i].u.pointer.size > 0)
                        {
                            mpParms[i].u.pointer.addr = RTMemAlloc(mpParms[i].u.pointer.size);
                            if (NULL == mpParms[i].u.pointer.addr)
                            {
                                rc = VERR_NO_MEMORY;
                                break;
                            }
                            else
                                memcpy(mpParms[i].u.pointer.addr,
                                       paParms[i].u.pointer.addr,
                                       mpParms[i].u.pointer.size);
                        }
                        else
                        {
                            /* Size is 0 -- make sure we don't have any pointer. */
                            mpParms[i].u.pointer.addr = NULL;
                        }
                        break;

                    default:
                        break;
                }
                if (RT_FAILURE(rc))
                    break;
            }
        }

        if (RT_SUCCESS(rc))
        {
            /*
             * Assume that the context ID *always* is the first parameter,
             * assign the context ID to the command.
             */
            rc = mpParms[0].getUInt32(&mContextID);

            /* Set timestamp so that clients can distinguish between already
             * processed commands and new ones. */
            mTimestamp = RTTimeNanoTS();
        }

        LogFlowFunc(("Returned with rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * Frees the buffered HGCM request.
     *
     * @return  IPRT status code.
     */
    void Free(void)
    {
        AssertMsg(mRefCount == 0, ("pHostCmd=%p, CID=%RU32 still being used by a client (%RU32 refs), cannot free yet\n",
                                   this, mContextID, mRefCount));

        LogFlowFunc(("Freeing host command pHostCmd=%p, CID=%RU32, mMsgType=%RU32, mParmCount=%RU32, mpParms=%p\n",
                     this, mContextID, mMsgType, mParmCount, mpParms));

        for (uint32_t i = 0; i < mParmCount; i++)
        {
            switch (mpParms[i].type)
            {
                case VBOX_HGCM_SVC_PARM_PTR:
                    if (mpParms[i].u.pointer.size > 0)
                        RTMemFree(mpParms[i].u.pointer.addr);
                    break;

                default:
                    break;
            }
        }

        if (mpParms)
        {
            RTMemFree(mpParms);
            mpParms = NULL;
        }

        mParmCount = 0;

        /* Removes the command from its list */
        RTListNodeRemove(&Node);
    }

    /**
     * Copies data from the buffered HGCM request to the current HGCM request.
     *
     * @return  IPRT status code.
     * @param   paDstParms              Array of parameters of HGCM request to fill the data into.
     * @param   cDstParms               Number of parameters the HGCM request can handle.
     */
    int CopyTo(VBOXHGCMSVCPARM paDstParms[], uint32_t cDstParms) const
    {
        LogFlowFunc(("pHostCmd=%p, mMsgType=%RU32, mParmCount=%RU32, mContextID=%RU32 (Session %RU32)\n",
                     this, mMsgType, mParmCount, mContextID, VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(mContextID)));

        int rc = VINF_SUCCESS;
        if (cDstParms != mParmCount)
        {
            LogFlowFunc(("Parameter count does not match (got %RU32, expected %RU32)\n",
                         cDstParms, mParmCount));
            rc = VERR_INVALID_PARAMETER;
        }

        if (RT_SUCCESS(rc))
        {
            for (uint32_t i = 0; i < mParmCount; i++)
            {
                if (paDstParms[i].type != mpParms[i].type)
                {
                    LogFlowFunc(("Parameter %RU32 type mismatch (got %RU32, expected %RU32)\n",
                                 i, paDstParms[i].type, mpParms[i].type));
                    rc = VERR_INVALID_PARAMETER;
                }
                else
                {
                    switch (mpParms[i].type)
                    {
                        case VBOX_HGCM_SVC_PARM_32BIT:
#ifdef DEBUG_andy
                            LogFlowFunc(("\tmpParms[%RU32] = %RU32 (uint32_t)\n",
                                         i, mpParms[i].u.uint32));
#endif
                            paDstParms[i].u.uint32 = mpParms[i].u.uint32;
                            break;

                        case VBOX_HGCM_SVC_PARM_64BIT:
#ifdef DEBUG_andy
                            LogFlowFunc(("\tmpParms[%RU32] = %RU64 (uint64_t)\n",
                                         i, mpParms[i].u.uint64));
#endif
                            paDstParms[i].u.uint64 = mpParms[i].u.uint64;
                            break;

                        case VBOX_HGCM_SVC_PARM_PTR:
                        {
#ifdef DEBUG_andy
                            LogFlowFunc(("\tmpParms[%RU32] = %p (ptr), size = %RU32\n",
                                         i, mpParms[i].u.pointer.addr, mpParms[i].u.pointer.size));
#endif
                            if (!mpParms[i].u.pointer.size)
                                continue; /* Only copy buffer if there actually is something to copy. */

                            if (!paDstParms[i].u.pointer.addr)
                                rc = VERR_INVALID_PARAMETER;

                            if (   RT_SUCCESS(rc)
                                && paDstParms[i].u.pointer.size < mpParms[i].u.pointer.size)
                                rc = VERR_BUFFER_OVERFLOW;

                            if (RT_SUCCESS(rc))
                            {
                                memcpy(paDstParms[i].u.pointer.addr,
                                       mpParms[i].u.pointer.addr,
                                       mpParms[i].u.pointer.size);
                            }

                            break;
                        }

                        default:
                            LogFlowFunc(("Parameter %RU32 of type %RU32 is not supported yet\n",
                                         i, mpParms[i].type));
                            rc = VERR_NOT_SUPPORTED;
                            break;
                    }
                }

                if (RT_FAILURE(rc))
                {
                    LogFlowFunc(("Parameter %RU32 invalid (%Rrc), refusing\n",
                                 i, rc));
                    break;
                }
            }
        }

        LogFlowFunc(("Returned with rc=%Rrc\n", rc));
        return rc;
    }

    int Assign(const ClientConnection *pConnection)
    {
        AssertPtrReturn(pConnection, VERR_INVALID_POINTER);

        int rc;

        LogFlowFunc(("pHostCmd=%p, mMsgType=%RU32, mParmCount=%RU32, mpParms=%p\n",
                     this, mMsgType, mParmCount, mpParms));

        /* Does the current host command need more parameter space which
         * the client does not provide yet? */
        if (mParmCount > pConnection->mNumParms)
        {
            LogFlowFunc(("pHostCmd=%p requires %RU32 parms, only got %RU32 from client\n",
                         this, mParmCount, pConnection->mNumParms));

            /*
            * So this call apparently failed because the guest wanted to peek
            * how much parameters it has to supply in order to successfully retrieve
            * this command. Let's tell him so!
            */
            rc = VERR_TOO_MUCH_DATA;
        }
        else
        {
            rc = CopyTo(pConnection->mParms, pConnection->mNumParms);

            /*
             * Has there been enough parameter space but the wrong parameter types
             * were submitted -- maybe the client was just asking for the next upcoming
             * host message?
             *
             * Note: To keep this compatible to older clients we return VERR_TOO_MUCH_DATA
             *       in every case.
             */
            if (RT_FAILURE(rc))
                rc = VERR_TOO_MUCH_DATA;
        }

        return rc;
    }

    int Peek(const ClientConnection *pConnection)
    {
        AssertPtrReturn(pConnection, VERR_INVALID_POINTER);

        LogFlowFunc(("pHostCmd=%p, mMsgType=%RU32, mParmCount=%RU32, mpParms=%p\n",
                     this, mMsgType, mParmCount, mpParms));

        if (pConnection->mNumParms >= 2)
        {
            pConnection->mParms[0].setUInt32(mMsgType);   /* Message ID */
            pConnection->mParms[1].setUInt32(mParmCount); /* Required parameters for message */
        }
        else
            LogFlowFunc(("Warning: Client has not (yet) submitted enough parameters (%RU32, must be at least 2) to at least peak for the next message\n",
                         pConnection->mNumParms));

        /*
         * Always return VERR_TOO_MUCH_DATA data here to
         * keep it compatible with older clients and to
         * have correct accounting (mHostRc + mHostCmdTries).
         */
        return VERR_TOO_MUCH_DATA;
    }

    /** Reference count for keeping track how many connected
     *  clients still need to process this command until it can
     *  be removed. */
    uint32_t mRefCount;
    /** The context ID this command belongs to. Will be extracted
     *  *always* from HGCM parameter [0]. */
    uint32_t mContextID;
    /** Dynamic structure for holding the HGCM parms */
    uint32_t mMsgType;
    /** Number of HGCM parameters. */
    uint32_t mParmCount;
    /** Array of HGCM parameters. */
    PVBOXHGCMSVCPARM mpParms;
    /** Incoming timestamp (us). */
    uint64_t mTimestamp;
} HostCommand;
typedef std::list< HostCommand *> HostCmdList;
typedef std::list< HostCommand *>::iterator HostCmdListIter;
typedef std::list< HostCommand *>::const_iterator HostCmdListIterConst;

/**
 * Per-client structure used for book keeping/state tracking a
 * certain host command.
 */
typedef struct ClientContext
{
    /* Pointer to list node of this command. */
    HostCommand *mpHostCmd;
    /** The standard constructor. */
    ClientContext(void) : mpHostCmd(NULL) {}
    /** Internal constrcutor. */
    ClientContext(HostCommand *pHostCmd) : mpHostCmd(pHostCmd) {}
} ClientContext;
typedef std::map< uint32_t, ClientContext > ClientContextMap;
typedef std::map< uint32_t, ClientContext >::iterator ClientContextMapIter;
typedef std::map< uint32_t, ClientContext >::const_iterator ClientContextMapIterConst;

/**
 * Structure for holding a connected guest client
 * state.
 */
typedef struct ClientState
{
    ClientState(void)
        : mSvcHelpers(NULL),
          mID(0),
          mFlags(0),
          mFilterMask(0), mFilterValue(0),
          mHostCmdRc(VINF_SUCCESS), mHostCmdTries(0),
          mHostCmdTS(0),
          mIsPending(false),
          mPeekCount(0) { }

    ClientState(PVBOXHGCMSVCHELPERS pSvcHelpers, uint32_t uClientID)
        : mSvcHelpers(pSvcHelpers),
          mID(uClientID),
          mFlags(0),
          mFilterMask(0), mFilterValue(0),
          mHostCmdRc(VINF_SUCCESS), mHostCmdTries(0),
          mHostCmdTS(0),
          mIsPending(false),
          mPeekCount(0){ }

    void DequeueAll(void)
    {
        HostCmdListIter curItem = mHostCmdList.begin();
        while (curItem != mHostCmdList.end())
            curItem = Dequeue(curItem);
    }

    void DequeueCurrent(void)
    {
        HostCmdListIter curCmd = mHostCmdList.begin();
        if (curCmd != mHostCmdList.end())
            Dequeue(curCmd);
    }

    HostCmdListIter Dequeue(HostCmdListIter &curItem)
    {
        HostCommand *pHostCmd = (*curItem);
        AssertPtr(pHostCmd);

        if (pHostCmd->Release() == 0)
        {
            LogFlowFunc(("[Client %RU32] Destroying pHostCmd=%p\n",
                         mID, (*curItem)));

            delete pHostCmd;
            pHostCmd = NULL;
        }

        HostCmdListIter nextItem = mHostCmdList.erase(curItem);

        /* Reset everything else. */
        mHostCmdRc    = VINF_SUCCESS;
        mHostCmdTries = 0;
        mPeekCount    = 0;

        return nextItem;
    }

    int EnqueueCommand(HostCommand *pHostCmd)
    {
        AssertPtrReturn(pHostCmd, VERR_INVALID_POINTER);

        int rc = VINF_SUCCESS;

        try
        {
            mHostCmdList.push_back(pHostCmd);
            pHostCmd->AddRef();
        }
        catch (std::bad_alloc)
        {
            rc = VERR_NO_MEMORY;
        }

        return rc;
    }

    bool WantsHostCommand(const HostCommand *pHostCmd) const
    {
        AssertPtrReturn(pHostCmd, false);

#ifdef DEBUG_andy
        LogFlowFunc(("mHostCmdTS=%RU64, pHostCmdTS=%RU64\n",
                     mHostCmdTS, pHostCmd->mTimestamp));
#endif

        /* Only process newer commands. */
        if (pHostCmd->mTimestamp <= mHostCmdTS)
            return false;

        /*
         * If a sesseion filter is set, only obey those commands we're interested in
         * by applying our context ID filter mask and compare the result with the
         * original context ID.
         */
        bool fWant;
        if (mFlags & CLIENTSTATE_FLAG_CONTEXTFILTER)
        {
            fWant = (pHostCmd->mContextID & mFilterMask) == mFilterValue;
        }
        else /* Client is interested in all commands. */
            fWant = true;

        LogFlowFunc(("[Client %RU32] mFlags=0x%x, mContextID=%RU32 (session %RU32), mFilterMask=0x%x, mFilterValue=%RU32, fWant=%RTbool\n",
                     mID, mFlags, pHostCmd->mContextID,
                     VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(pHostCmd->mContextID),
                     mFilterMask, mFilterValue, fWant));

        return fWant;
    }

    /**
     * Set to inidicate that a client call (GUEST_MSG_WAIT) is pending.
     */
    int SetPending(const ClientConnection *pConnection)
    {
        AssertPtrReturn(pConnection, VERR_INVALID_POINTER);

        if (mIsPending)
        {
            LogFlowFunc(("[Client %RU32] Already is in pending mode\n", mID));

            /*
             * Signal that we don't and can't return yet.
             */
            return VINF_HGCM_ASYNC_EXECUTE;
        }

        if (mHostCmdList.empty())
        {
            AssertMsg(mIsPending == false,
                      ("Client ID=%RU32 already is pending but tried to receive a new host command\n", mID));

            mPendingCon.mHandle   = pConnection->mHandle;
            mPendingCon.mNumParms = pConnection->mNumParms;
            mPendingCon.mParms    = pConnection->mParms;

            mIsPending = true;

            LogFlowFunc(("[Client %RU32] Is now in pending mode\n", mID));

            /*
             * Signal that we don't and can't return yet.
             */
            return VINF_HGCM_ASYNC_EXECUTE;
        }

        /*
         * Signal that there already is a connection pending.
         * Shouldn't happen in daily usage.
         */
        AssertMsgFailed(("Client already has a connection pending\n"));
        return VERR_SIGNAL_PENDING;
    }

    int Run(const ClientConnection *pConnection,
                  HostCommand      *pHostCmd)
    {
        AssertPtrReturn(pConnection, VERR_INVALID_POINTER);
        AssertPtrReturn(pHostCmd, VERR_INVALID_POINTER);

        int rc = VINF_SUCCESS;

        LogFlowFunc(("[Client %RU32] pConnection=%p, mHostCmdRc=%Rrc, mHostCmdTries=%RU32, mPeekCount=%RU32\n",
                      mID, pConnection, mHostCmdRc, mHostCmdTries, mPeekCount));

        mHostCmdRc = SendReply(pConnection, pHostCmd);
        LogFlowFunc(("[Client %RU32] Processing pHostCmd=%p ended with rc=%Rrc\n",
                     mID, pHostCmd, mHostCmdRc));

        bool fRemove = false;
        if (RT_FAILURE(mHostCmdRc))
        {
            mHostCmdTries++;

            /*
             * If the client understood the message but supplied too little buffer space
             * don't send this message again and drop it after 6 unsuccessful attempts.
             *
             * Note: Due to legacy reasons this the retry counter has to be even because on
             *       every peek there will be the actual command retrieval from the client side.
             *       To not get the actual command if the client actually only wants to peek for
             *       the next command, there needs to be two rounds per try, e.g. 3 rounds = 6 tries.
             *
             ** @todo Fix the mess stated above. GUEST_MSG_WAIT should be become GUEST_MSG_PEEK, *only*
             *        (and every time) returning the next upcoming host command (if any, blocking). Then
             *        it's up to the client what to do next, either peeking again or getting the actual
             *        host command via an own GUEST_ type message.
             */
            if (mHostCmdRc == VERR_TOO_MUCH_DATA)
            {
                if (mHostCmdTries == 6)
                    fRemove = true;
            }
            /* Client did not understand the message or something else weird happened. Try again one
             * more time and drop it if it didn't get handled then. */
            else if (mHostCmdTries > 1)
                fRemove = true;
        }
        else
            fRemove = true; /* Everything went fine, remove it. */

        LogFlowFunc(("[Client %RU32] Tried pHostCmd=%p for %RU32 times, (last result=%Rrc, fRemove=%RTbool)\n",
                     mID, pHostCmd, mHostCmdTries, mHostCmdRc, fRemove));

        if (RT_SUCCESS(rc)) /** @todo r=bird: confusing statement+state, rc hasn't been touched since the top and is always VINF_SUCCESS. */
            rc = mHostCmdRc;

        if (fRemove)
        {
            /** @todo Fix this (slow) lookup. Too late today. */
            HostCmdListIter curItem = mHostCmdList.begin();
            while (curItem != mHostCmdList.end())
            {
                if ((*curItem) == pHostCmd)
                {
                    Dequeue(curItem);
                    break;
                }

                ++curItem;
            }
        }

        LogFlowFunc(("[Client %RU32] Returned with rc=%Rrc\n", mID, rc));
        return rc;
    }

    int RunCurrent(const ClientConnection *pConnection)
    {
        AssertPtrReturn(pConnection, VERR_INVALID_POINTER);

        int rc;
        if (mHostCmdList.empty())
        {
            rc = SetPending(pConnection);
        }
        else
        {
            AssertMsgReturn(!mIsPending,
                            ("Client ID=%RU32 still is in pending mode; can't use another connection\n", mID), VERR_INVALID_PARAMETER);

            HostCmdListIter curCmd = mHostCmdList.begin();
            Assert(curCmd != mHostCmdList.end());
            HostCommand *pHostCmd = (*curCmd);
            AssertPtrReturn(pHostCmd, VERR_INVALID_POINTER);

            rc = Run(pConnection, pHostCmd);
        }

        return rc;
    }

    int Wakeup(void)
    {
        int rc = VINF_NO_CHANGE;

        if (mIsPending)
        {
            LogFlowFunc(("[Client %RU32] Waking up ...\n", mID));

            rc = VINF_SUCCESS;

            HostCmdListIter curCmd = mHostCmdList.begin();
            if (curCmd != mHostCmdList.end())
            {
                HostCommand *pHostCmd = (*curCmd);
                AssertPtrReturn(pHostCmd, VERR_INVALID_POINTER);

                LogFlowFunc(("[Client %RU32] Current host command is pHostCmd=%p, CID=%RU32, cmdType=%RU32, cmdParms=%RU32, refCount=%RU32\n",
                             mID, pHostCmd, pHostCmd->mContextID, pHostCmd->mMsgType, pHostCmd->mParmCount, pHostCmd->mRefCount));

                rc = Run(&mPendingCon, pHostCmd);
            }
            else
                AssertMsgFailed(("Waking up client ID=%RU32 with no host command in queue is a bad idea\n", mID));

            return rc;
        }

        return VINF_NO_CHANGE;
    }

    int CancelWaiting(int rcPending)
    {
        LogFlowFunc(("[Client %RU32] Cancelling waiting with %Rrc, isPending=%RTbool, pendingNumParms=%RU32, flags=%x\n",
                     mID, rcPending, mIsPending, mPendingCon.mNumParms, mFlags));

        int rc;
        if (   mIsPending
            && mPendingCon.mNumParms >= 2)
        {
            mPendingCon.mParms[0].setUInt32(HOST_CANCEL_PENDING_WAITS); /* Message ID. */
            mPendingCon.mParms[1].setUInt32(0);                         /* Required parameters for message. */

            AssertPtr(mSvcHelpers);
            mSvcHelpers->pfnCallComplete(mPendingCon.mHandle, rcPending);

            mIsPending = false;

            rc = VINF_SUCCESS;
        }
        else if (mPendingCon.mNumParms < 2)
            rc = VERR_BUFFER_OVERFLOW;
        else /** @todo Enqueue command instead of dropping? */
            rc = VERR_WRONG_ORDER;

        return rc;
    }

    int SendReply(ClientConnection const *pConnection,
                  HostCommand            *pHostCmd)
    {
        AssertPtrReturn(pConnection, VERR_INVALID_POINTER);
        AssertPtrReturn(pHostCmd, VERR_INVALID_POINTER);

        int rc;
        /* If the client is in pending mode, always send back
         * the peek result first. */
        if (mIsPending)
        {
            rc = pHostCmd->Peek(pConnection);
            mPeekCount++;
        }
        else
        {
            /* If this is the very first peek, make sure to *always* give back the peeking answer
             * instead of the actual command, even if this command would fit into the current
             * connection buffer. */
            if (!mPeekCount)
            {
                rc = pHostCmd->Peek(pConnection);
                mPeekCount++;
            }
            else
            {
                /* Try assigning the host command to the client and store the
                 * result code for later use. */
                rc = pHostCmd->Assign(pConnection);
                if (RT_FAILURE(rc)) /* If something failed, let the client peek (again). */
                {
                    rc = pHostCmd->Peek(pConnection);
                    mPeekCount++;
                }
                else
                    mPeekCount = 0;
            }
        }

        /* Reset pending status. */
        mIsPending = false;

        /* In any case the client did something, so complete
         * the pending call with the result we just got. */
        AssertPtr(mSvcHelpers);
        mSvcHelpers->pfnCallComplete(pConnection->mHandle, rc);

        LogFlowFunc(("[Client %RU32] mPeekCount=%RU32, pConnection=%p, pHostCmd=%p, replyRc=%Rrc\n",
                     mID, mPeekCount, pConnection, pHostCmd, rc));
        return rc;
    }

    PVBOXHGCMSVCHELPERS mSvcHelpers;
    /** The client's ID. */
    uint32_t mID;
    /** Client flags. @sa CLIENTSTATE_FLAG_ flags. */
    uint32_t mFlags;
    /** The context ID filter mask, if any. */
    uint32_t mFilterMask;
    /** The context ID filter value, if any. */
    uint32_t mFilterValue;
    /** Host command list to process. */
    HostCmdList mHostCmdList;
    /** Last (most recent) rc after handling the host command. */
    int mHostCmdRc;
    /** How many GUEST_MSG_WAIT calls the client has issued to retrieve one command.
     *
     * This is used as a heuristic to remove a message that the client appears not
     * to be able to successfully retrieve.  */
    uint32_t mHostCmdTries;
    /** Timestamp (us) of last host command processed. */
    uint64_t mHostCmdTS;
    /** Flag indicating whether a client call (GUEST_MSG_WAIT) currently is pending.
     *
     * This means the client waits for a new host command to reply and won't return
     * from the waiting call until a new host command is available.
     */
    bool mIsPending;
    /** Number of times we've peeked at a pending message.
     *
     * This is necessary for being compatible with older Guest Additions.  In case
     * there are commands which only have two (2) parameters and therefore would fit
     * into the GUEST_MSG_WAIT reply immediately, we now can make sure that the
     * client first gets back the GUEST_MSG_WAIT results first.
     */
    uint32_t mPeekCount;
    /** The client's pending connection. */
    ClientConnection mPendingCon;
} ClientState;
typedef std::map< uint32_t, ClientState > ClientStateMap;
typedef std::map< uint32_t, ClientState >::iterator ClientStateMapIter;
typedef std::map< uint32_t, ClientState >::const_iterator ClientStateMapIterConst;

/**
 * Class containing the shared information service functionality.
 */
class Service : public RTCNonCopyable
{

private:

    /** Type definition for use in callback functions. */
    typedef Service SELF;
    /** HGCM helper functions. */
    PVBOXHGCMSVCHELPERS mpHelpers;
    /**
     * Callback function supplied by the host for notification of updates
     * to properties.
     */
    PFNHGCMSVCEXT mpfnHostCallback;
    /** User data pointer to be supplied to the host callback function. */
    void *mpvHostData;
    /** List containing all buffered host commands. */
    RTLISTANCHOR mHostCmdList;
    /** Map containing all connected clients. The primary key contains
     *  the HGCM client ID to identify the client. */
    ClientStateMap mClientStateMap;
public:
    explicit Service(PVBOXHGCMSVCHELPERS pHelpers)
        : mpHelpers(pHelpers)
        , mpfnHostCallback(NULL)
        , mpvHostData(NULL)
    {
        RTListInit(&mHostCmdList);
    }

    /**
     * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnUnload}
     * Simply deletes the service object
     */
    static DECLCALLBACK(int) svcUnload(void *pvService)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->uninit();
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            delete pSelf;
        return rc;
    }

    /**
     * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnConnect}
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcConnect(void *pvService,
                                        uint32_t u32ClientID,
                                        void *pvClient)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        AssertPtrReturn(pSelf, VERR_INVALID_POINTER);
        return pSelf->clientConnect(u32ClientID, pvClient);
    }

    /**
     * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnConnect}
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcDisconnect(void *pvService,
                                           uint32_t u32ClientID,
                                           void *pvClient)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        AssertPtrReturn(pSelf, VERR_INVALID_POINTER);
        return pSelf->clientDisconnect(u32ClientID, pvClient);
    }

    /**
     * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnCall}
     * Wraps to the call member function
     */
    static DECLCALLBACK(void) svcCall(void * pvService,
                                      VBOXHGCMCALLHANDLE callHandle,
                                      uint32_t u32ClientID,
                                      void *pvClient,
                                      uint32_t u32Function,
                                      uint32_t cParms,
                                      VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturnVoid(VALID_PTR(pvService));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        AssertPtrReturnVoid(pSelf);
        pSelf->call(callHandle, u32ClientID, pvClient, u32Function, cParms, paParms);
    }

    /**
     * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnHostCall}
     * Wraps to the hostCall member function
     */
    static DECLCALLBACK(int) svcHostCall(void *pvService,
                                         uint32_t u32Function,
                                         uint32_t cParms,
                                         VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        AssertPtrReturn(pSelf, VERR_INVALID_POINTER);
        return pSelf->hostCall(u32Function, cParms, paParms);
    }

    /**
     * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnRegisterExtension}
     * Installs a host callback for notifications of property changes.
     */
    static DECLCALLBACK(int) svcRegisterExtension(void *pvService,
                                                  PFNHGCMSVCEXT pfnExtension,
                                                  void *pvExtension)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        AssertPtrReturn(pSelf, VERR_INVALID_POINTER);
        pSelf->mpfnHostCallback = pfnExtension;
        pSelf->mpvHostData = pvExtension;
        return VINF_SUCCESS;
    }

private:

    int prepareExecute(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int clientConnect(uint32_t u32ClientID, void *pvClient);
    int clientDisconnect(uint32_t u32ClientID, void *pvClient);
    int clientGetCommand(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int clientSetMsgFilterSet(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int clientSetMsgFilterUnset(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int clientSkipMsg(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int cancelHostCmd(uint32_t u32ContextID);
    int cancelPendingWaits(uint32_t u32ClientID, int rcPending);
    int hostCallback(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int hostProcessCommand(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    void call(VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID, void *pvClient, uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int hostCall(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int sessionClose(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int uninit(void);

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(Service);
};

/**
 * Handles a client which just connected.
 *
 * @return  IPRT status code.
 * @param   u32ClientID
 * @param   pvClient
 */
int Service::clientConnect(uint32_t u32ClientID, void *pvClient)
{
    RT_NOREF1(pvClient);
    LogFlowFunc(("[Client %RU32] Connected\n", u32ClientID));
#ifdef VBOX_STRICT
    ClientStateMapIterConst it = mClientStateMap.find(u32ClientID);
    if (it != mClientStateMap.end())
    {
        AssertMsgFailed(("Client with ID=%RU32 already connected when it should not\n",
                         u32ClientID));
        return VERR_ALREADY_EXISTS;
    }
#endif
    ClientState clientState(mpHelpers, u32ClientID);
    mClientStateMap[u32ClientID] = clientState;
    /** @todo Exception handling! */
    return VINF_SUCCESS;
}

/**
 * Handles a client which disconnected. This functiond does some
 * internal cleanup as well as sends notifications to the host so
 * that the host can do the same (if required).
 *
 * @return  IPRT status code.
 * @param   u32ClientID             The client's ID of which disconnected.
 * @param   pvClient                User data, not used at the moment.
 */
int Service::clientDisconnect(uint32_t u32ClientID, void *pvClient)
{
    RT_NOREF1(pvClient);
    LogFlowFunc(("[Client %RU32] Disconnected (%zu clients total)\n",
                 u32ClientID, mClientStateMap.size()));

    AssertMsg(mClientStateMap.size(),
              ("No clients in list anymore when there should (client ID=%RU32)\n", u32ClientID));

    int rc = VINF_SUCCESS;

    ClientStateMapIter itClientState = mClientStateMap.find(u32ClientID);
    AssertMsg(itClientState != mClientStateMap.end(),
              ("Client ID=%RU32 not found in client list when it should be there\n", u32ClientID));

    if (itClientState != mClientStateMap.end())
    {
        itClientState->second.DequeueAll();

        mClientStateMap.erase(itClientState);
    }

    bool fAllClientsDisconnected = mClientStateMap.empty();
    if (fAllClientsDisconnected)
    {
        LogFlowFunc(("All clients disconnected, cancelling all host commands ...\n"));

        /*
         * If all clients disconnected we also need to make sure that all buffered
         * host commands need to be notified, because Main is waiting a notification
         * via a (multi stage) progress object.
         */
        HostCommand *pCurCmd = RTListGetFirst(&mHostCmdList, HostCommand, Node);
        while (pCurCmd)
        {
            HostCommand *pNext = RTListNodeGetNext(&pCurCmd->Node, HostCommand, Node);
            bool fLast = RTListNodeIsLast(&mHostCmdList, &pCurCmd->Node);

            int rc2 = cancelHostCmd(pCurCmd->mContextID);
            if (RT_FAILURE(rc2))
            {
                LogFlowFunc(("Cancelling host command with CID=%u (refCount=%RU32) failed with rc=%Rrc\n",
                             pCurCmd->mContextID, pCurCmd->mRefCount, rc2));
                /* Keep going. */
            }

            while (pCurCmd->Release())
                ;
            delete pCurCmd;
            pCurCmd = NULL;

            if (fLast)
                break;

            pCurCmd = pNext;
        }

        Assert(RTListIsEmpty(&mHostCmdList));
    }

    return rc;
}

/**
 * Either fills in parameters from a pending host command into our guest context or
 * defer the guest call until we have something from the host.
 *
 * @return  IPRT status code.
 * @param   u32ClientID                 The client's ID.
 * @param   callHandle                  The client's call handle.
 * @param   cParms                      Number of parameters.
 * @param   paParms                     Array of parameters.
 */
int Service::clientGetCommand(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle,
                              uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    /*
     * Lookup client in our list so that we can assign the context ID of
     * a command to that client.
     */
    ClientStateMapIter itClientState = mClientStateMap.find(u32ClientID);
    AssertMsg(itClientState != mClientStateMap.end(), ("Client with ID=%RU32 not found when it should be present\n",
                                                       u32ClientID));
    if (itClientState == mClientStateMap.end())
    {
        /* Should never happen. Complete the call on the guest side though. */
        AssertPtr(mpHelpers);
        mpHelpers->pfnCallComplete(callHandle, VERR_NOT_FOUND);

        return VERR_NOT_FOUND;
    }

    ClientState &clientState = itClientState->second;

    /* Use the current (inbound) connection. */
    ClientConnection thisCon;
    thisCon.mHandle   = callHandle;
    thisCon.mNumParms = cParms;
    thisCon.mParms    = paParms;

    return clientState.RunCurrent(&thisCon);
}

int Service::clientSetMsgFilterSet(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle,
                                   uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF1(callHandle);

    /*
     * Lookup client in our list so that we can assign the context ID of
     * a command to that client.
     */
    ClientStateMapIter itClientState = mClientStateMap.find(u32ClientID);
    AssertMsg(itClientState != mClientStateMap.end(), ("Client with ID=%RU32 not found when it should be present\n",
                                                       u32ClientID));
    if (itClientState == mClientStateMap.end())
        return VERR_NOT_FOUND; /* Should never happen. */

    if (cParms != 4)
        return VERR_INVALID_PARAMETER;

    uint32_t uValue;
    int rc = paParms[0].getUInt32(&uValue);
    if (RT_SUCCESS(rc))
    {
        uint32_t uMaskAdd;
        rc = paParms[1].getUInt32(&uMaskAdd);
        if (RT_SUCCESS(rc))
        {
            uint32_t uMaskRemove;
            rc = paParms[2].getUInt32(&uMaskRemove);
            /** @todo paParm[3] (flags) not used yet. */
            if (RT_SUCCESS(rc))
            {
                ClientState &clientState = itClientState->second;

                clientState.mFlags |= CLIENTSTATE_FLAG_CONTEXTFILTER;
                if (uMaskAdd)
                    clientState.mFilterMask |= uMaskAdd;
                if (uMaskRemove)
                    clientState.mFilterMask &= ~uMaskRemove;

                clientState.mFilterValue = uValue;

                LogFlowFunc(("[Client %RU32] Setting message filterMask=0x%x, filterVal=%RU32 set (flags=0x%x, maskAdd=0x%x, maskRemove=0x%x)\n",
                             u32ClientID, clientState.mFilterMask, clientState.mFilterValue,
                             clientState.mFlags, uMaskAdd, uMaskRemove));
            }
        }
    }

    return rc;
}

int Service::clientSetMsgFilterUnset(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle,
                                     uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF2(callHandle, paParms);

    /*
     * Lookup client in our list so that we can assign the context ID of
     * a command to that client.
     */
    ClientStateMapIter itClientState = mClientStateMap.find(u32ClientID);
    AssertMsg(itClientState != mClientStateMap.end(), ("Client with ID=%RU32 not found when it should be present\n",
                                                       u32ClientID));
    if (itClientState == mClientStateMap.end())
        return VERR_NOT_FOUND; /* Should never happen. */

    if (cParms != 1)
        return VERR_INVALID_PARAMETER;

    ClientState &clientState = itClientState->second;

    clientState.mFlags &= ~CLIENTSTATE_FLAG_CONTEXTFILTER;
    clientState.mFilterMask = 0;
    clientState.mFilterValue = 0;

    LogFlowFunc(("[Client %RU32} Unset message filter\n", u32ClientID));
    return VINF_SUCCESS;
}

int Service::clientSkipMsg(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle,
                           uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF2(callHandle, paParms);

    /*
     * Lookup client in our list so that we can assign the context ID of
     * a command to that client.
     */
    ClientStateMapIter itClientState = mClientStateMap.find(u32ClientID);
    AssertMsg(itClientState != mClientStateMap.end(), ("Client ID=%RU32 not found when it should be present\n",
                                                       u32ClientID));
    if (itClientState == mClientStateMap.end())
        return VERR_NOT_FOUND; /* Should never happen. */

    if (cParms != 1)
        return VERR_INVALID_PARAMETER;

    LogFlowFunc(("[Client %RU32] Skipping current message ...\n", u32ClientID));

    itClientState->second.DequeueCurrent();

    return VINF_SUCCESS;
}

/**
 * Cancels a buffered host command to unblock waiting on Main side
 * via callbacks.
 *
 * @return  IPRT status code.
 * @param   u32ContextID                Context ID of host command to cancel.
 */
int Service::cancelHostCmd(uint32_t u32ContextID)
{
    Assert(mpfnHostCallback);

    LogFlowFunc(("Cancelling CID=%u ...\n", u32ContextID));

    uint32_t cParms = 0;
    VBOXHGCMSVCPARM arParms[2];
    arParms[cParms++].setUInt32(u32ContextID);

    return hostCallback(GUEST_DISCONNECTED, cParms, arParms);
}

/**
 * Client asks itself (in another thread) to cancel all pending waits which are blocking the client
 * from shutting down / doing something else.
 *
 * @return  IPRT status code.
 * @param   u32ClientID                 The client's ID.
 * @param   rcPending                   Result code for completing pending operation.
 */
int Service::cancelPendingWaits(uint32_t u32ClientID, int rcPending)
{
    ClientStateMapIter itClientState = mClientStateMap.find(u32ClientID);
    if (itClientState != mClientStateMap.end())
        return itClientState->second.CancelWaiting(rcPending);

    return VINF_SUCCESS;
}

/**
 * Notifies the host (using low-level HGCM callbacks) about an event
 * which was sent from the client.
 *
 * @return  IPRT status code.
 * @param   eFunction               Function (event) that occured.
 * @param   cParms                  Number of parameters.
 * @param   paParms                 Array of parameters.
 */
int Service::hostCallback(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    LogFlowFunc(("eFunction=%ld, cParms=%ld, paParms=%p\n",
                 eFunction, cParms, paParms));

    int rc;
    if (mpfnHostCallback)
    {
        VBOXGUESTCTRLHOSTCALLBACK data(cParms, paParms);
        rc = mpfnHostCallback(mpvHostData, eFunction,
                              (void *)(&data), sizeof(data));
    }
    else
        rc = VERR_NOT_SUPPORTED;

    LogFlowFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

/**
 * Processes a command received from the host side and re-routes it to
 * a connect client on the guest.
 *
 * @return  IPRT status code.
 * @param   eFunction               Function code to process.
 * @param   cParms                  Number of parameters.
 * @param   paParms                 Array of parameters.
 */
int Service::hostProcessCommand(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    /*
     * If no client is connected at all we don't buffer any host commands
     * and immediately return an error to the host. This avoids the host
     * waiting for a response from the guest side in case VBoxService on
     * the guest is not running/system is messed up somehow.
     */
    if (mClientStateMap.empty())
        return VERR_NOT_FOUND;

    int rc;

    HostCommand *pHostCmd = NULL;
    try
    {
        pHostCmd = new HostCommand();
        rc = pHostCmd->Allocate(eFunction, cParms, paParms);
        if (RT_SUCCESS(rc))
            /* rc = */ RTListAppend(&mHostCmdList, &pHostCmd->Node);
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        LogFlowFunc(("Handling host command CID=%RU32, eFunction=%RU32, cParms=%RU32, paParms=%p, numClients=%zu\n",
                     pHostCmd->mContextID, eFunction, cParms, paParms, mClientStateMap.size()));

        /*
         * Wake up all pending clients which are interested in this
         * host command.
         */
#ifdef DEBUG
        uint32_t uClientsWokenUp = 0;
#endif
        ClientStateMapIter itClientState = mClientStateMap.begin();
        AssertMsg(itClientState != mClientStateMap.end(), ("Client state map is empty when it should not\n"));
        while (itClientState != mClientStateMap.end())
        {
            ClientState &clientState = itClientState->second;

            /* If a client indicates that it it wants the new host command,
             * add a reference to not delete it.*/
            if (clientState.WantsHostCommand(pHostCmd))
            {
                clientState.EnqueueCommand(pHostCmd);

                int rc2 = clientState.Wakeup();
                if (RT_FAILURE(rc2))
                    LogFlowFunc(("Waking up client ID=%RU32 failed with rc=%Rrc\n",
                                 itClientState->first, rc2));
#ifdef DEBUG
                uClientsWokenUp++;
#endif
            }

            ++itClientState;
        }

#ifdef DEBUG
        LogFlowFunc(("%RU32 clients have been woken up\n", uClientsWokenUp));
#endif
    }

    return rc;
}

/**
 * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnCall}
 *
 * @note    All functions which do not involve an unreasonable delay will be
 *          handled synchronously.  If needed, we will add a request handler
 *          thread in future for those which do.
 *
 * @thread  HGCM
 */
void Service::call(VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID,
                   void * /* pvClient */, uint32_t eFunction, uint32_t cParms,
                   VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    LogFlowFunc(("[Client %RU32] eFunction=%RU32, cParms=%RU32, paParms=0x%p\n",
                 u32ClientID, eFunction, cParms, paParms));
    try
    {
        /*
         * The guest asks the host for the next message to process.
         */
        if (eFunction == GUEST_MSG_WAIT)
        {
            LogFlowFunc(("[Client %RU32] GUEST_MSG_GET\n", u32ClientID));
            rc = clientGetCommand(u32ClientID, callHandle, cParms, paParms);
        }
        else
        {
            switch (eFunction)
            {
                /*
                 * A client wants to shut down and asks us (this service) to cancel
                 * all blocking/pending waits (VINF_HGCM_ASYNC_EXECUTE) so that the
                 * client can gracefully shut down.
                 */
                case GUEST_CANCEL_PENDING_WAITS:
                    LogFlowFunc(("[Client %RU32] GUEST_CANCEL_PENDING_WAITS\n", u32ClientID));
                    rc = cancelPendingWaits(u32ClientID, VINF_SUCCESS /* Pending result */);
                    break;

                /*
                 * The guest only wants certain messages set by the filter mask(s).
                 * Since VBox 4.3+.
                 */
                case GUEST_MSG_FILTER_SET:
                    LogFlowFunc(("[Client %RU32] GUEST_MSG_FILTER_SET\n", u32ClientID));
                    rc = clientSetMsgFilterSet(u32ClientID, callHandle, cParms, paParms);
                    break;

                /*
                 * Unsetting the message filter flag.
                 */
                case GUEST_MSG_FILTER_UNSET:
                    LogFlowFunc(("[Client %RU32] GUEST_MSG_FILTER_UNSET\n", u32ClientID));
                    rc = clientSetMsgFilterUnset(u32ClientID, callHandle, cParms, paParms);
                    break;

                /*
                 * The guest only wants skip the currently assigned messages. Neded
                 * for dropping its assigned reference of the current assigned host
                 * command in queue.
                 * Since VBox 4.3+.
                 */
                case GUEST_MSG_SKIP:
                    LogFlowFunc(("[Client %RU32] GUEST_MSG_SKIP\n", u32ClientID));
                    rc = clientSkipMsg(u32ClientID, callHandle, cParms, paParms);
                    break;

                /*
                 * The guest wants to close specific guest session. This is handy for
                 * shutting down dedicated guest session processes from another process.
                 */
                case GUEST_SESSION_CLOSE:
                    LogFlowFunc(("[Client %RU32] GUEST_SESSION_CLOSE\n", u32ClientID));
                    rc = sessionClose(u32ClientID, callHandle, cParms, paParms);
                    break;

                /*
                 * For all other regular commands we call our hostCallback
                 * function. If the current command does not support notifications,
                 * notifyHost will return VERR_NOT_SUPPORTED.
                 */
                default:
                    rc = hostCallback(eFunction, cParms, paParms);
                    break;
            }

            if (rc != VINF_HGCM_ASYNC_EXECUTE)
            {
                /* Tell the client that the call is complete (unblocks waiting). */
                AssertPtr(mpHelpers);
                mpHelpers->pfnCallComplete(callHandle, rc);
            }
        }
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }
}

/**
 * Service call handler for the host.
 * @interface_method_impl{VBOXHGCMSVCFNTABLE,pfnHostCall}
 * @thread  hgcm
 */
int Service::hostCall(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VERR_NOT_SUPPORTED;
    LogFlowFunc(("fn=%RU32, cParms=%RU32, paParms=0x%p\n",
                 eFunction, cParms, paParms));
    try
    {
        switch (eFunction)
        {
            /**
             * Host
             */
            case HOST_CANCEL_PENDING_WAITS:
            {
                LogFlowFunc(("HOST_CANCEL_PENDING_WAITS\n"));
                ClientStateMapIter itClientState = mClientStateMap.begin();
                while (itClientState != mClientStateMap.end())
                {
                    int rc2 = itClientState->second.CancelWaiting(VINF_SUCCESS /* Pending rc. */);
                    if (RT_FAILURE(rc2))
                        LogFlowFunc(("Cancelling waiting for client ID=%RU32 failed with rc=%Rrc",
                                     itClientState->first, rc2));
                    ++itClientState;
                }
                rc = VINF_SUCCESS;
                break;
            }

            default:
                rc = hostProcessCommand(eFunction, cParms, paParms);
                break;
        }
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

/**
 * Client asks another client (guest) session to close.
 *
 * @return  IPRT status code.
 * @param   u32ClientID                 The client's ID.
 * @param   callHandle                  The client's call handle.
 * @param   cParms                      Number of parameters.
 * @param   paParms                     Array of parameters.
 */
int Service::sessionClose(uint32_t u32ClientID, VBOXHGCMCALLHANDLE callHandle, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    RT_NOREF2(u32ClientID, callHandle);
    if (cParms < 2)
        return VERR_INVALID_PARAMETER;

    uint32_t uContextID, uFlags;
    int rc = paParms[0].getUInt32(&uContextID);
    if (RT_SUCCESS(rc))
        rc = paParms[1].getUInt32(&uFlags);

    uint32_t uSessionID = VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(uContextID);

    if (RT_SUCCESS(rc))
        rc = hostProcessCommand(HOST_SESSION_CLOSE, cParms, paParms);

    LogFlowFunc(("Closing guest session ID=%RU32 (from client ID=%RU32) returned with rc=%Rrc\n",
                 uSessionID, u32ClientID, rc)); NOREF(uSessionID);
    return rc;
}

int Service::uninit(void)
{
    return VINF_SUCCESS;
}

} /* namespace guestControl */

using guestControl::Service;

/**
 * @copydoc VBOXHGCMSVCLOAD
 */
extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad(VBOXHGCMSVCFNTABLE *pTable)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("pTable=%p\n", pTable));

    if (!VALID_PTR(pTable))
    {
        rc = VERR_INVALID_PARAMETER;
    }
    else
    {
        LogFlowFunc(("pTable->cbSize=%d, pTable->u32Version=0x%08X\n", pTable->cbSize, pTable->u32Version));

        if (   pTable->cbSize != sizeof (VBOXHGCMSVCFNTABLE)
            || pTable->u32Version != VBOX_HGCM_SVC_VERSION)
        {
            rc = VERR_VERSION_MISMATCH;
        }
        else
        {
            Service *pService = NULL;
            /* No exceptions may propagate outside. */
            try
            {
                pService = new Service(pTable->pHelpers);
            }
            catch (int rcThrown)
            {
                rc = rcThrown;
            }
            catch(std::bad_alloc &)
            {
                rc = VERR_NO_MEMORY;
            }

            if (RT_SUCCESS(rc))
            {
                /*
                 * We don't need an additional client data area on the host,
                 * because we're a class which can have members for that :-).
                 */
                pTable->cbClient = 0;

                /* Register functions. */
                pTable->pfnUnload             = Service::svcUnload;
                pTable->pfnConnect            = Service::svcConnect;
                pTable->pfnDisconnect         = Service::svcDisconnect;
                pTable->pfnCall               = Service::svcCall;
                pTable->pfnHostCall           = Service::svcHostCall;
                pTable->pfnSaveState          = NULL;  /* The service is stateless, so the normal */
                pTable->pfnLoadState          = NULL;  /* construction done before restoring suffices */
                pTable->pfnRegisterExtension  = Service::svcRegisterExtension;

                /* Service specific initialization. */
                pTable->pvService = pService;
            }
            else
            {
                if (pService)
                {
                    delete pService;
                    pService = NULL;
                }
            }
        }
    }

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}

