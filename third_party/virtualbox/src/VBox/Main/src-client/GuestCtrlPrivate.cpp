/* $Id: GuestCtrlPrivate.cpp $ */
/** @file
 * Internal helpers/structures for guest control functionality.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GUEST_CONTROL
#include "LoggingNew.h"

#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestCtrlImplPrivate.h"
#include "GuestSessionImpl.h"
#include "VMMDev.h"

#include <iprt/asm.h>
#include <iprt/cpp/utils.h> /* For unconst(). */
#include <iprt/ctype.h>
#ifdef DEBUG
# include <iprt/file.h>
#endif /* DEBUG */



int GuestFsObjData::FromLs(const GuestProcessStreamBlock &strmBlk)
{
    LogFlowFunc(("\n"));

    int rc = VINF_SUCCESS;

    try
    {
#ifdef DEBUG
        strmBlk.DumpToLog();
#endif
        /* Object name. */
        mName = strmBlk.GetString("name");
        if (mName.isEmpty()) throw VERR_NOT_FOUND;
        /* Type. */
        Utf8Str strType(strmBlk.GetString("ftype"));
        if (strType.equalsIgnoreCase("-"))
            mType = FsObjType_File;
        else if (strType.equalsIgnoreCase("d"))
            mType = FsObjType_Directory;
        /** @todo Add more types! */
        else
            mType = FsObjType_Unknown;
        /* Object size. */
        rc = strmBlk.GetInt64Ex("st_size", &mObjectSize);
        if (RT_FAILURE(rc)) throw rc;
        /** @todo Add complete ls info! */
    }
    catch (int rc2)
    {
        rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestFsObjData::FromMkTemp(const GuestProcessStreamBlock &strmBlk)
{
    LogFlowFunc(("\n"));

    int rc;

    try
    {
#ifdef DEBUG
        strmBlk.DumpToLog();
#endif
        /* Object name. */
        mName = strmBlk.GetString("name");
        if (mName.isEmpty()) throw VERR_NOT_FOUND;
        /* Assign the stream block's rc. */
        rc = strmBlk.GetRc();
    }
    catch (int rc2)
    {
        rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestFsObjData::FromStat(const GuestProcessStreamBlock &strmBlk)
{
    LogFlowFunc(("\n"));

    int rc = VINF_SUCCESS;

    try
    {
#ifdef DEBUG
        strmBlk.DumpToLog();
#endif
        /* Node ID, optional because we don't include this
         * in older VBoxService (< 4.2) versions. */
        mNodeID = strmBlk.GetInt64("node_id");
        /* Object name. */
        mName = strmBlk.GetString("name");
        if (mName.isEmpty()) throw VERR_NOT_FOUND;
        /* Type. */
        Utf8Str strType(strmBlk.GetString("ftype"));
        if (strType.equalsIgnoreCase("-"))
            mType = FsObjType_File;
        else if (strType.equalsIgnoreCase("d"))
            mType = FsObjType_Directory;
        /** @todo Add more types! */
        else
            mType = FsObjType_Unknown;
        /* Object size. */
        rc = strmBlk.GetInt64Ex("st_size", &mObjectSize);
        if (RT_FAILURE(rc)) throw rc;
        /** @todo Add complete stat info! */
    }
    catch (int rc2)
    {
        rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////

/** @todo *NOT* thread safe yet! */
/** @todo Add exception handling for STL stuff! */

GuestProcessStreamBlock::GuestProcessStreamBlock(void)
{

}

/*
GuestProcessStreamBlock::GuestProcessStreamBlock(const GuestProcessStreamBlock &otherBlock)
{
    for (GuestCtrlStreamPairsIter it = otherBlock.mPairs.begin();
         it != otherBlock.end(); ++it)
    {
        mPairs[it->first] = new
        if (it->second.pszValue)
        {
            RTMemFree(it->second.pszValue);
            it->second.pszValue = NULL;
        }
    }
}*/

GuestProcessStreamBlock::~GuestProcessStreamBlock()
{
    Clear();
}

/**
 * Destroys the currently stored stream pairs.
 *
 * @return  IPRT status code.
 */
void GuestProcessStreamBlock::Clear(void)
{
    mPairs.clear();
}

#ifdef DEBUG
void GuestProcessStreamBlock::DumpToLog(void) const
{
    LogFlowFunc(("Dumping contents of stream block=0x%p (%ld items):\n",
                 this, mPairs.size()));

    for (GuestCtrlStreamPairMapIterConst it = mPairs.begin();
         it != mPairs.end(); ++it)
    {
        LogFlowFunc(("\t%s=%s\n", it->first.c_str(), it->second.mValue.c_str()));
    }
}
#endif

/**
 * Returns a 64-bit signed integer of a specified key.
 *
 * @return  IPRT status code. VERR_NOT_FOUND if key was not found.
 * @param  pszKey               Name of key to get the value for.
 * @param  piVal                Pointer to value to return.
 */
int GuestProcessStreamBlock::GetInt64Ex(const char *pszKey, int64_t *piVal) const
{
    AssertPtrReturn(pszKey, VERR_INVALID_POINTER);
    AssertPtrReturn(piVal, VERR_INVALID_POINTER);
    const char *pszValue = GetString(pszKey);
    if (pszValue)
    {
        *piVal = RTStrToInt64(pszValue);
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}

/**
 * Returns a 64-bit integer of a specified key.
 *
 * @return  int64_t             Value to return, 0 if not found / on failure.
 * @param   pszKey              Name of key to get the value for.
 */
int64_t GuestProcessStreamBlock::GetInt64(const char *pszKey) const
{
    int64_t iVal;
    if (RT_SUCCESS(GetInt64Ex(pszKey, &iVal)))
        return iVal;
    return 0;
}

/**
 * Returns the current number of stream pairs.
 *
 * @return  uint32_t            Current number of stream pairs.
 */
size_t GuestProcessStreamBlock::GetCount(void) const
{
    return mPairs.size();
}

/**
 * Gets the return code (name = "rc") of this stream block.
 *
 * @return  IPRT status code.
 */
int GuestProcessStreamBlock::GetRc(void) const
{
    const char *pszValue = GetString("rc");
    if (pszValue)
    {
        return RTStrToInt16(pszValue);
    }
    return VERR_NOT_FOUND;
}

/**
 * Returns a string value of a specified key.
 *
 * @return  uint32_t            Pointer to string to return, NULL if not found / on failure.
 * @param   pszKey              Name of key to get the value for.
 */
const char* GuestProcessStreamBlock::GetString(const char *pszKey) const
{
    AssertPtrReturn(pszKey, NULL);

    try
    {
        GuestCtrlStreamPairMapIterConst itPairs = mPairs.find(Utf8Str(pszKey));
        if (itPairs != mPairs.end())
            return itPairs->second.mValue.c_str();
    }
    catch (const std::exception &ex)
    {
        NOREF(ex);
    }
    return NULL;
}

/**
 * Returns a 32-bit unsigned integer of a specified key.
 *
 * @return  IPRT status code. VERR_NOT_FOUND if key was not found.
 * @param  pszKey               Name of key to get the value for.
 * @param  puVal                Pointer to value to return.
 */
int GuestProcessStreamBlock::GetUInt32Ex(const char *pszKey, uint32_t *puVal) const
{
    AssertPtrReturn(pszKey, VERR_INVALID_POINTER);
    AssertPtrReturn(puVal, VERR_INVALID_POINTER);
    const char *pszValue = GetString(pszKey);
    if (pszValue)
    {
        *puVal = RTStrToUInt32(pszValue);
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}

/**
 * Returns a 32-bit unsigned integer of a specified key.
 *
 * @return  uint32_t            Value to return, 0 if not found / on failure.
 * @param   pszKey              Name of key to get the value for.
 */
uint32_t GuestProcessStreamBlock::GetUInt32(const char *pszKey) const
{
    uint32_t uVal;
    if (RT_SUCCESS(GetUInt32Ex(pszKey, &uVal)))
        return uVal;
    return 0;
}

/**
 * Sets a value to a key or deletes a key by setting a NULL value.
 *
 * @return  IPRT status code.
 * @param   pszKey              Key name to process.
 * @param   pszValue            Value to set. Set NULL for deleting the key.
 */
int GuestProcessStreamBlock::SetValue(const char *pszKey, const char *pszValue)
{
    AssertPtrReturn(pszKey, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    try
    {
        Utf8Str Utf8Key(pszKey);

        /* Take a shortcut and prevent crashes on some funny versions
         * of STL if map is empty initially. */
        if (!mPairs.empty())
        {
            GuestCtrlStreamPairMapIter it = mPairs.find(Utf8Key);
            if (it != mPairs.end())
                 mPairs.erase(it);
        }

        if (pszValue)
        {
            GuestProcessStreamValue val(pszValue);
            mPairs[Utf8Key] = val;
        }
    }
    catch (const std::exception &ex)
    {
        NOREF(ex);
    }
    return rc;
}

///////////////////////////////////////////////////////////////////////////////

GuestProcessStream::GuestProcessStream(void)
    : m_cbAllocated(0),
      m_cbUsed(0),
      m_offBuffer(0),
      m_pbBuffer(NULL)
{

}

GuestProcessStream::~GuestProcessStream(void)
{
    Destroy();
}

/**
 * Adds data to the internal parser buffer. Useful if there
 * are multiple rounds of adding data needed.
 *
 * @return  IPRT status code.
 * @param   pbData              Pointer to data to add.
 * @param   cbData              Size (in bytes) of data to add.
 */
int GuestProcessStream::AddData(const BYTE *pbData, size_t cbData)
{
    AssertPtrReturn(pbData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;

    /* Rewind the buffer if it's empty. */
    size_t     cbInBuf   = m_cbUsed - m_offBuffer;
    bool const fAddToSet = cbInBuf == 0;
    if (fAddToSet)
        m_cbUsed = m_offBuffer = 0;

    /* Try and see if we can simply append the data. */
    if (cbData + m_cbUsed <= m_cbAllocated)
    {
        memcpy(&m_pbBuffer[m_cbUsed], pbData, cbData);
        m_cbUsed += cbData;
    }
    else
    {
        /* Move any buffered data to the front. */
        cbInBuf = m_cbUsed - m_offBuffer;
        if (cbInBuf == 0)
            m_cbUsed = m_offBuffer = 0;
        else if (m_offBuffer) /* Do we have something to move? */
        {
            memmove(m_pbBuffer, &m_pbBuffer[m_offBuffer], cbInBuf);
            m_cbUsed = cbInBuf;
            m_offBuffer = 0;
        }

        /* Do we need to grow the buffer? */
        if (cbData + m_cbUsed > m_cbAllocated)
        {
/** @todo Put an upper limit on the allocation?   */
            size_t cbAlloc = m_cbUsed + cbData;
            cbAlloc = RT_ALIGN_Z(cbAlloc, _64K);
            void *pvNew = RTMemRealloc(m_pbBuffer, cbAlloc);
            if (pvNew)
            {
                m_pbBuffer = (uint8_t *)pvNew;
                m_cbAllocated = cbAlloc;
            }
            else
                rc = VERR_NO_MEMORY;
        }

        /* Finally, copy the data. */
        if (RT_SUCCESS(rc))
        {
            if (cbData + m_cbUsed <= m_cbAllocated)
            {
                memcpy(&m_pbBuffer[m_cbUsed], pbData, cbData);
                m_cbUsed += cbData;
            }
            else
                rc = VERR_BUFFER_OVERFLOW;
        }
    }

    return rc;
}

/**
 * Destroys the internal data buffer.
 */
void GuestProcessStream::Destroy(void)
{
    if (m_pbBuffer)
    {
        RTMemFree(m_pbBuffer);
        m_pbBuffer = NULL;
    }

    m_cbAllocated = 0;
    m_cbUsed = 0;
    m_offBuffer = 0;
}

#ifdef DEBUG
void GuestProcessStream::Dump(const char *pszFile)
{
    LogFlowFunc(("Dumping contents of stream=0x%p (cbAlloc=%u, cbSize=%u, cbOff=%u) to %s\n",
                 m_pbBuffer, m_cbAllocated, m_cbUsed, m_offBuffer, pszFile));

    RTFILE hFile;
    int rc = RTFileOpen(&hFile, pszFile, RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_WRITE);
    if (RT_SUCCESS(rc))
    {
        rc = RTFileWrite(hFile, m_pbBuffer, m_cbUsed, NULL /* pcbWritten */);
        RTFileClose(hFile);
    }
}
#endif

/**
 * Tries to parse the next upcoming pair block within the internal
 * buffer.
 *
 * Returns VERR_NO_DATA is no data is in internal buffer or buffer has been
 * completely parsed already.
 *
 * Returns VERR_MORE_DATA if current block was parsed (with zero or more pairs
 * stored in stream block) but still contains incomplete (unterminated)
 * data.
 *
 * Returns VINF_SUCCESS if current block was parsed until the next upcoming
 * block (with zero or more pairs stored in stream block).
 *
 * @return IPRT status code.
 * @param streamBlock               Reference to guest stream block to fill.
 *
 */
int GuestProcessStream::ParseBlock(GuestProcessStreamBlock &streamBlock)
{
    if (   !m_pbBuffer
        || !m_cbUsed)
    {
        return VERR_NO_DATA;
    }

    AssertReturn(m_offBuffer <= m_cbUsed, VERR_INVALID_PARAMETER);
    if (m_offBuffer == m_cbUsed)
        return VERR_NO_DATA;

    int rc = VINF_SUCCESS;

    char    *pszOff    = (char*)&m_pbBuffer[m_offBuffer];
    char    *pszStart  = pszOff;
    uint32_t uDistance;
    while (*pszStart)
    {
        size_t pairLen = strlen(pszStart);
        uDistance = (pszStart - pszOff);
        if (m_offBuffer + uDistance + pairLen + 1 >= m_cbUsed)
        {
            rc = VERR_MORE_DATA;
            break;
        }
        else
        {
            char *pszSep = strchr(pszStart, '=');
            char *pszVal = NULL;
            if (pszSep)
                pszVal = pszSep + 1;
            if (!pszSep || !pszVal)
            {
                rc = VERR_MORE_DATA;
                break;
            }

            /* Terminate the separator so that we can
             * use pszStart as our key from now on. */
            *pszSep = '\0';

            rc = streamBlock.SetValue(pszStart, pszVal);
            if (RT_FAILURE(rc))
                return rc;
        }

        /* Next pair. */
        pszStart += pairLen + 1;
    }

    /* If we did not do any movement but we have stuff left
     * in our buffer just skip the current termination so that
     * we can try next time. */
    uDistance = (pszStart - pszOff);
    if (   !uDistance
        && *pszStart == '\0'
        && m_offBuffer < m_cbUsed)
    {
        uDistance++;
    }
    m_offBuffer += uDistance;

    return rc;
}

GuestBase::GuestBase(void)
    : mConsole(NULL),
      mNextContextID(0)
{
}

GuestBase::~GuestBase(void)
{
}

int GuestBase::baseInit(void)
{
    int rc = RTCritSectInit(&mWaitEventCritSect);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

void GuestBase::baseUninit(void)
{
    LogFlowThisFuncEnter();

    int rc = RTCritSectDelete(&mWaitEventCritSect);
    NOREF(rc);

    LogFlowFuncLeaveRC(rc);
    /* No return value. */
}

int GuestBase::cancelWaitEvents(void)
{
    LogFlowThisFuncEnter();

    int rc = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc))
    {
        GuestEventGroup::iterator itEventGroups = mWaitEventGroups.begin();
        while (itEventGroups != mWaitEventGroups.end())
        {
            GuestWaitEvents::iterator itEvents = itEventGroups->second.begin();
            while (itEvents != itEventGroups->second.end())
            {
                GuestWaitEvent *pEvent = itEvents->second;
                AssertPtr(pEvent);

                /*
                 * Just cancel the event, but don't remove it from the
                 * wait events map. Don't delete it though, this (hopefully)
                 * is done by the caller using unregisterWaitEvent().
                 */
                int rc2 = pEvent->Cancel();
                AssertRC(rc2);

                ++itEvents;
            }

            ++itEventGroups;
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestBase::dispatchGeneric(PVBOXGUESTCTRLHOSTCBCTX pCtxCb, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb)
{
    LogFlowFunc(("pCtxCb=%p, pSvcCb=%p\n", pCtxCb, pSvcCb));

    AssertPtrReturn(pCtxCb, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCb, VERR_INVALID_POINTER);

    int vrc = VINF_SUCCESS;

    try
    {
        LogFlowFunc(("uFunc=%RU32, cParms=%RU32\n",
                     pCtxCb->uFunction, pSvcCb->mParms));

        switch (pCtxCb->uFunction)
        {
            case GUEST_MSG_PROGRESS_UPDATE:
                break;

            case GUEST_MSG_REPLY:
            {
                if (pSvcCb->mParms >= 3)
                {
                    int idx = 1; /* Current parameter index. */
                    CALLBACKDATA_MSG_REPLY dataCb;
                    /* pSvcCb->mpaParms[0] always contains the context ID. */
                    vrc = pSvcCb->mpaParms[idx++].getUInt32(&dataCb.uType);
                    AssertRCReturn(vrc, vrc);
                    vrc = pSvcCb->mpaParms[idx++].getUInt32(&dataCb.rc);
                    AssertRCReturn(vrc, vrc);
                    vrc = pSvcCb->mpaParms[idx++].getPointer(&dataCb.pvPayload, &dataCb.cbPayload);
                    AssertRCReturn(vrc, vrc);

                    GuestWaitEventPayload evPayload(dataCb.uType, dataCb.pvPayload, dataCb.cbPayload);
                    int rc2 = signalWaitEventInternal(pCtxCb, dataCb.rc, &evPayload);
                    AssertRC(rc2);
                }
                else
                    vrc = VERR_INVALID_PARAMETER;
                break;
            }

            default:
                vrc = VERR_NOT_SUPPORTED;
                break;
        }
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }
    catch (int rc)
    {
        vrc = rc;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestBase::generateContextID(uint32_t uSessionID, uint32_t uObjectID, uint32_t *puContextID)
{
    AssertPtrReturn(puContextID, VERR_INVALID_POINTER);

    if (   uSessionID >= VBOX_GUESTCTRL_MAX_SESSIONS
        || uObjectID  >= VBOX_GUESTCTRL_MAX_OBJECTS)
        return VERR_INVALID_PARAMETER;

    uint32_t uCount = ASMAtomicIncU32(&mNextContextID);
    if (uCount == VBOX_GUESTCTRL_MAX_CONTEXTS)
        uCount = 0;

    uint32_t uNewContextID =
        VBOX_GUESTCTRL_CONTEXTID_MAKE(uSessionID, uObjectID, uCount);

    *puContextID = uNewContextID;

#if 0
    LogFlowThisFunc(("mNextContextID=%RU32, uSessionID=%RU32, uObjectID=%RU32, uCount=%RU32, uNewContextID=%RU32\n",
                     mNextContextID, uSessionID, uObjectID, uCount, uNewContextID));
#endif
    return VINF_SUCCESS;
}

int GuestBase::registerWaitEvent(uint32_t uSessionID, uint32_t uObjectID,
                                 GuestWaitEvent **ppEvent)
{
    GuestEventTypes eventTypesEmpty;
    return registerWaitEvent(uSessionID, uObjectID, eventTypesEmpty, ppEvent);
}

/**
 * Registers (creates) a new wait event based on a given session and object ID.
 *
 * From those IDs an unique context ID (CID) will be built, which only can be
 * around once at a time.
 *
 * @returns IPRT status code. VERR_ALREADY_EXISTS if an event with the given session
 *          and object ID already has been registered.
 *
 * @param   uSessionID              Session ID to register wait event for.
 * @param   uObjectID               Object ID to register wait event for.
 * @param   lstEvents               List of events to register the wait event for.
 * @param   ppEvent                 Pointer to registered (created) wait event on success.
 *                                  Must be destroyed with unregisterWaitEvent().
 */
int GuestBase::registerWaitEvent(uint32_t uSessionID, uint32_t uObjectID,
                                 const GuestEventTypes &lstEvents,
                                 GuestWaitEvent **ppEvent)
{
    AssertPtrReturn(ppEvent, VERR_INVALID_POINTER);

    uint32_t uContextID;
    int rc = generateContextID(uSessionID, uObjectID, &uContextID);
    if (RT_FAILURE(rc))
        return rc;

    rc = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc))
    {
        try
        {
            GuestWaitEvent *pEvent = new GuestWaitEvent(uContextID, lstEvents);
            AssertPtr(pEvent);

            LogFlowThisFunc(("New event=%p, CID=%RU32\n", pEvent, uContextID));

            /* Insert event into matching event group. This is for faster per-group
             * lookup of all events later. */
            for (GuestEventTypes::const_iterator itEvents = lstEvents.begin();
                 itEvents != lstEvents.end(); ++itEvents)
            {
                /* Check if the event group already has an event with the same
                 * context ID in it (collision). */
                GuestWaitEvents eventGroup = mWaitEventGroups[(*itEvents)];
                if (eventGroup.find(uContextID) == eventGroup.end())
                {
                    /* No, insert. */
                    mWaitEventGroups[(*itEvents)].insert(std::pair<uint32_t, GuestWaitEvent *>(uContextID, pEvent));
                }
                else
                {
                    rc = VERR_ALREADY_EXISTS;
                    break;
                }
            }

            if (RT_SUCCESS(rc))
            {
                /* Register event in regular event list. */
                if (mWaitEvents.find(uContextID) == mWaitEvents.end())
                {
                    mWaitEvents[uContextID] = pEvent;
                }
                else
                    rc  = VERR_ALREADY_EXISTS;
            }

            if (RT_SUCCESS(rc))
                *ppEvent = pEvent;
        }
        catch(std::bad_alloc &)
        {
            rc = VERR_NO_MEMORY;
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}

int GuestBase::signalWaitEvent(VBoxEventType_T aType, IEvent *aEvent)
{
    int rc = RTCritSectEnter(&mWaitEventCritSect);
#ifdef DEBUG
    uint32_t cEvents = 0;
#endif
    if (RT_SUCCESS(rc))
    {
        GuestEventGroup::iterator itGroup = mWaitEventGroups.find(aType);
        if (itGroup != mWaitEventGroups.end())
        {
            GuestWaitEvents::iterator itEvents = itGroup->second.begin();
            while (itEvents != itGroup->second.end())
            {
#ifdef DEBUG
                LogFlowThisFunc(("Signalling event=%p, type=%ld (CID %RU32: Session=%RU32, Object=%RU32, Count=%RU32) ...\n",
                                 itEvents->second, aType, itEvents->first,
                                 VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(itEvents->first),
                                 VBOX_GUESTCTRL_CONTEXTID_GET_OBJECT(itEvents->first),
                                 VBOX_GUESTCTRL_CONTEXTID_GET_COUNT(itEvents->first)));
#endif
                ComPtr<IEvent> pThisEvent = aEvent;
                Assert(!pThisEvent.isNull());
                int rc2 = itEvents->second->SignalExternal(aEvent);
                if (RT_SUCCESS(rc))
                    rc = rc2;

                if (RT_SUCCESS(rc2))
                {
                    /* Remove the event from all other event groups (except the
                     * original one!) because it was signalled. */
                    AssertPtr(itEvents->second);
                    const GuestEventTypes evTypes = itEvents->second->Types();
                    for (GuestEventTypes::const_iterator itType = evTypes.begin();
                         itType != evTypes.end(); ++itType)
                    {
                        if ((*itType) != aType) /* Only remove all other groups. */
                        {
                            /* Get current event group. */
                            GuestEventGroup::iterator evGroup = mWaitEventGroups.find((*itType));
                            Assert(evGroup != mWaitEventGroups.end());

                            /* Lookup event in event group. */
                            GuestWaitEvents::iterator evEvent = evGroup->second.find(itEvents->first /* Context ID */);
                            Assert(evEvent != evGroup->second.end());

                            LogFlowThisFunc(("Removing event=%p (type %ld)\n", evEvent->second, (*itType)));
                            evGroup->second.erase(evEvent);

                            LogFlowThisFunc(("%zu events for type=%ld left\n",
                                             evGroup->second.size(), aType));
                        }
                    }

                    /* Remove the event from the passed-in event group. */
                    GuestWaitEvents::iterator itEventsNext = itEvents;
                    ++itEventsNext;
                    itGroup->second.erase(itEvents);
                    itEvents = itEventsNext;
                }
                else
                    ++itEvents;
#ifdef DEBUG
                cEvents++;
#endif
            }
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    LogFlowThisFunc(("Signalled %RU32 events, rc=%Rrc\n", cEvents, rc));
#endif
    return rc;
}

int GuestBase::signalWaitEventInternal(PVBOXGUESTCTRLHOSTCBCTX pCbCtx,
                                       int guestRc, const GuestWaitEventPayload *pPayload)
{
    if (RT_SUCCESS(guestRc))
        return signalWaitEventInternalEx(pCbCtx, VINF_SUCCESS,
                                         0 /* Guest rc */, pPayload);

    return signalWaitEventInternalEx(pCbCtx, VERR_GSTCTL_GUEST_ERROR,
                                     guestRc, pPayload);
}

int GuestBase::signalWaitEventInternalEx(PVBOXGUESTCTRLHOSTCBCTX pCbCtx,
                                         int rc, int guestRc,
                                         const GuestWaitEventPayload *pPayload)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    /* pPayload is optional. */

    int rc2 = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc2))
    {
        GuestWaitEvents::iterator itEvent = mWaitEvents.find(pCbCtx->uContextID);
        if (itEvent != mWaitEvents.end())
        {
            LogFlowThisFunc(("Signalling event=%p (CID %RU32, rc=%Rrc, guestRc=%Rrc, pPayload=%p) ...\n",
                             itEvent->second, itEvent->first, rc, guestRc, pPayload));
            GuestWaitEvent *pEvent = itEvent->second;
            AssertPtr(pEvent);
            rc2 = pEvent->SignalInternal(rc, guestRc, pPayload);
        }
        else
            rc2 = VERR_NOT_FOUND;

        int rc3 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc2))
            rc2 = rc3;
    }

    return rc2;
}

/**
 * Unregisters (deletes) a wait event.
 *
 * After successful unregistration the event will not be valid anymore.
 *
 * @returns IPRT status code.
 * @param   pEvent                  Event to unregister (delete).
 */
int GuestBase::unregisterWaitEvent(GuestWaitEvent *pEvent)
{
    if (!pEvent) /* Nothing to unregister. */
        return VINF_SUCCESS;

    int rc = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc))
    {
        LogFlowThisFunc(("pEvent=%p\n", pEvent));

        try
        {
            /* Remove the event from all event type groups. */
            const GuestEventTypes lstTypes = pEvent->Types();
            for (GuestEventTypes::const_iterator itType = lstTypes.begin();
                 itType != lstTypes.end(); ++itType)
            {
                /** @todo Slow O(n) lookup. Optimize this. */
                GuestWaitEvents::iterator itCurEvent = mWaitEventGroups[(*itType)].begin();
                while (itCurEvent != mWaitEventGroups[(*itType)].end())
                {
                    if (itCurEvent->second == pEvent)
                    {
                        mWaitEventGroups[(*itType)].erase(itCurEvent);
                        break;
                    }
                    else
                        ++itCurEvent;
                }
            }

            /* Remove the event from the general event list as well. */
            GuestWaitEvents::iterator itEvent = mWaitEvents.find(pEvent->ContextID());

            Assert(itEvent != mWaitEvents.end());
            Assert(itEvent->second == pEvent);

            mWaitEvents.erase(itEvent);

            delete pEvent;
            pEvent = NULL;
        }
        catch (const std::exception &ex)
        {
            NOREF(ex);
            AssertFailedStmt(rc = VERR_NOT_FOUND);
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}

/**
 * Waits for a formerly registered guest event.
 *
 * @return  IPRT status code.
 * @param   pEvent                  Pointer to event to wait for.
 * @param   uTimeoutMS              Timeout (in ms) for waiting.
 * @param   pType                   Event type of following IEvent.
 *                                  Optional.
 * @param   ppEvent                 Pointer to IEvent which got triggered
 *                                  for this event. Optional.
 */
int GuestBase::waitForEvent(GuestWaitEvent *pEvent, uint32_t uTimeoutMS,
                            VBoxEventType_T *pType, IEvent **ppEvent)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    /* pType is optional. */
    /* ppEvent is optional. */

    int vrc = pEvent->Wait(uTimeoutMS);
    if (RT_SUCCESS(vrc))
    {
        const ComPtr<IEvent> pThisEvent = pEvent->Event();
        if (!pThisEvent.isNull()) /* Having a VBoxEventType_ event is optional. */
        {
            if (pType)
            {
                HRESULT hr = pThisEvent->COMGETTER(Type)(pType);
                if (FAILED(hr))
                    vrc = VERR_COM_UNEXPECTED;
            }
            if (   RT_SUCCESS(vrc)
                && ppEvent)
                pThisEvent.queryInterfaceTo(ppEvent);

            unconst(pThisEvent).setNull();
        }
    }

    return vrc;
}

GuestObject::GuestObject(void)
    : mSession(NULL),
      mObjectID(0)
{
}

GuestObject::~GuestObject(void)
{
}

int GuestObject::bindToSession(Console *pConsole, GuestSession *pSession, uint32_t uObjectID)
{
    AssertPtrReturn(pConsole, VERR_INVALID_POINTER);
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    mConsole  = pConsole;
    mSession  = pSession;
    mObjectID = uObjectID;

    return VINF_SUCCESS;
}

int GuestObject::registerWaitEvent(const GuestEventTypes &lstEvents,
                                   GuestWaitEvent **ppEvent)
{
    AssertPtr(mSession);
    return GuestBase::registerWaitEvent(mSession->i_getId(), mObjectID, lstEvents, ppEvent);
}

int GuestObject::sendCommand(uint32_t uFunction,
                             uint32_t cParms, PVBOXHGCMSVCPARM paParms)
{
#ifndef VBOX_GUESTCTRL_TEST_CASE
    ComObjPtr<Console> pConsole = mConsole;
    Assert(!pConsole.isNull());

    int vrc = VERR_HGCM_SERVICE_NOT_FOUND;

    /* Forward the information to the VMM device. */
    VMMDev *pVMMDev = pConsole->i_getVMMDev();
    if (pVMMDev)
    {
        LogFlowThisFunc(("uFunction=%RU32, cParms=%RU32\n", uFunction, cParms));
        vrc = pVMMDev->hgcmHostCall(HGCMSERVICE_NAME, uFunction, cParms, paParms);
        if (RT_FAILURE(vrc))
        {
            /** @todo What to do here? */
        }
    }
#else
    LogFlowThisFuncEnter();

    /* Not needed within testcases. */
    RT_NOREF(uFunction, cParms, paParms);
    int vrc = VINF_SUCCESS;
#endif
    return vrc;
}

GuestWaitEventBase::GuestWaitEventBase(void)
    : mfAborted(false),
      mCID(0),
      mEventSem(NIL_RTSEMEVENT),
      mRc(VINF_SUCCESS),
      mGuestRc(VINF_SUCCESS)
{
}

GuestWaitEventBase::~GuestWaitEventBase(void)
{
    if (mEventSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(mEventSem);
        mEventSem = NIL_RTSEMEVENT;
    }
}

int GuestWaitEventBase::Init(uint32_t uCID)
{
    mCID = uCID;

    return RTSemEventCreate(&mEventSem);
}

int GuestWaitEventBase::SignalInternal(int rc, int guestRc,
                                       const GuestWaitEventPayload *pPayload)
{
    if (ASMAtomicReadBool(&mfAborted))
        return VERR_CANCELLED;

#ifdef VBOX_STRICT
    if (rc == VERR_GSTCTL_GUEST_ERROR)
        AssertMsg(RT_FAILURE(guestRc), ("Guest error indicated but no actual guest error set (%Rrc)\n", guestRc));
    else
        AssertMsg(RT_SUCCESS(guestRc), ("No guest error indicated but actual guest error set (%Rrc)\n", guestRc));
#endif

    int rc2;
    if (pPayload)
        rc2 = mPayload.CopyFromDeep(*pPayload);
    else
        rc2 = VINF_SUCCESS;
    if (RT_SUCCESS(rc2))
    {
        mRc = rc;
        mGuestRc = guestRc;

        rc2 = RTSemEventSignal(mEventSem);
    }

    return rc2;
}

int GuestWaitEventBase::Wait(RTMSINTERVAL uTimeoutMS)
{
    int rc = VINF_SUCCESS;

    if (ASMAtomicReadBool(&mfAborted))
        rc = VERR_CANCELLED;

    if (RT_SUCCESS(rc))
    {
        AssertReturn(mEventSem != NIL_RTSEMEVENT, VERR_CANCELLED);

        RTMSINTERVAL msInterval = uTimeoutMS;
        if (!uTimeoutMS)
            msInterval = RT_INDEFINITE_WAIT;
        rc = RTSemEventWait(mEventSem, msInterval);
        if (ASMAtomicReadBool(&mfAborted))
            rc = VERR_CANCELLED;
        if (RT_SUCCESS(rc))
        {
            /* If waiting succeeded, return the overall
             * result code. */
            rc = mRc;
        }
    }

    return rc;
}

GuestWaitEvent::GuestWaitEvent(uint32_t uCID,
                               const GuestEventTypes &lstEvents)
{
    int rc2 = Init(uCID);
    AssertRC(rc2); /** @todo Throw exception here. */

    mEventTypes = lstEvents;
}

GuestWaitEvent::GuestWaitEvent(uint32_t uCID)
{
    int rc2 = Init(uCID);
    AssertRC(rc2); /** @todo Throw exception here. */
}

GuestWaitEvent::~GuestWaitEvent(void)
{

}

/**
 * Cancels the event.
 */
int GuestWaitEvent::Cancel(void)
{
    AssertReturn(!mfAborted, VERR_CANCELLED);
    ASMAtomicWriteBool(&mfAborted, true);

#ifdef DEBUG_andy
    LogFlowThisFunc(("Cancelling %p ...\n"));
#endif
    return RTSemEventSignal(mEventSem);
}

int GuestWaitEvent::Init(uint32_t uCID)
{
    return GuestWaitEventBase::Init(uCID);
}

/**
 * Signals the event.
 *
 * @return  IPRT status code.
 * @param   pEvent              Public IEvent to associate.
 *                              Optional.
 */
int GuestWaitEvent::SignalExternal(IEvent *pEvent)
{
    AssertReturn(mEventSem != NIL_RTSEMEVENT, VERR_CANCELLED);

    if (pEvent)
        mEvent = pEvent;

    return RTSemEventSignal(mEventSem);
}

