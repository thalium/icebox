/* $Id: MediumLock.cpp $ */
/** @file
 *
 * Medium lock management helper classes
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "MediumLock.h"
#include "MediumImpl.h"
#include "MediumAttachmentImpl.h"


MediumLock::MediumLock()
    : mMedium(NULL), mMediumCaller(NULL), mLockWrite(false),
      mIsLocked(false), mLockSkipped(false)
{
}

MediumLock::~MediumLock()
{
    // destroying medium locks is routinely done as part of error handling
    // and it's not expected to lose error info
    ErrorInfoKeeper eik;
    Unlock();
}

MediumLock::MediumLock(const MediumLock &aMediumLock)
    : mMedium(aMediumLock.mMedium), mMediumCaller(NULL),
      mLockWrite(aMediumLock.mLockWrite), mIsLocked(false), mLockSkipped(false)
{
}

MediumLock::MediumLock(const ComObjPtr<Medium> &aMedium, bool aLockWrite)
    : mMedium(aMedium), mMediumCaller(NULL), mLockWrite(aLockWrite),
      mIsLocked(false), mLockSkipped(false)
{
}

HRESULT MediumLock::UpdateLock(bool aLockWrite)
{
    bool fPrevLockWrite = mLockWrite;
    if (mIsLocked)
    {
        Unlock();
        mLockWrite = aLockWrite;
        HRESULT rc = Lock();
        if (FAILED(rc))
        {
            mLockWrite = fPrevLockWrite;
            Lock();
            return rc;
        }
        return S_OK;
    }

    mLockWrite = aLockWrite;
    return S_OK;
}

const ComObjPtr<Medium> &MediumLock::GetMedium() const
{
    return mMedium;
}

bool MediumLock::GetLockRequest() const
{
    return mLockWrite;
}

bool MediumLock::IsLocked() const
{
    return mIsLocked;
}

HRESULT MediumLock::Lock(bool aIgnoreLockedMedia)
{
    if (mIsLocked)
        return S_OK;

    mMediumCaller.attach(mMedium);
    if (FAILED(mMediumCaller.rc()))
    {
        mMediumCaller.attach(NULL);
        return VBOX_E_INVALID_OBJECT_STATE;
    }

    HRESULT rc = S_OK;
    MediumState_T state;
    {
        AutoReadLock alock(mMedium COMMA_LOCKVAL_SRC_POS);
        state = mMedium->i_getState();
    }
    switch (state)
    {
        case MediumState_NotCreated:
        case MediumState_Creating:
        case MediumState_Deleting:
            mLockSkipped = true;
            break;
        default:
            if (mLockWrite)
            {
                if (aIgnoreLockedMedia && (   state == MediumState_LockedRead
                                           || state == MediumState_LockedWrite))
                    return S_OK;
                else
                    rc = mMedium->LockWrite(mToken.asOutParam());
            }
            else
            {
                if (aIgnoreLockedMedia && state == MediumState_LockedWrite)
                    return S_OK;
                else
                    rc = mMedium->LockRead(mToken.asOutParam());
            }
    }
    if (SUCCEEDED(rc))
    {
        mIsLocked = true;
        return S_OK;
    }
    else
    {
        mMediumCaller.attach(NULL);
        return VBOX_E_INVALID_OBJECT_STATE;
    }
}

HRESULT MediumLock::Unlock()
{
    HRESULT rc = S_OK;
    if (mIsLocked && !mLockSkipped && mToken)
    {
        mToken->Abandon();
        mToken.setNull();
    }
    mMediumCaller.attach(NULL);
    mLockSkipped = false;
    mIsLocked = false;
    return rc;
}

MediumLockList::MediumLockList()
{
    mIsLocked = false;
}

MediumLockList::~MediumLockList()
{
    // destroying medium lock lists is routinely done as part of error handling
    // and it's not expected to lose error info
    ErrorInfoKeeper eik;
    Clear();
    // rest is done by the list object's destructor
}

bool MediumLockList::IsEmpty()
{
    return mMediumLocks.empty();
}

HRESULT MediumLockList::Append(const ComObjPtr<Medium> &aMedium, bool aLockWrite)
{
    if (mIsLocked)
        return VBOX_E_INVALID_OBJECT_STATE;
    mMediumLocks.push_back(MediumLock(aMedium, aLockWrite));
    return S_OK;
}

HRESULT MediumLockList::Prepend(const ComObjPtr<Medium> &aMedium, bool aLockWrite)
{
    if (mIsLocked)
        return VBOX_E_INVALID_OBJECT_STATE;
    mMediumLocks.push_front(MediumLock(aMedium, aLockWrite));
    return S_OK;
}

HRESULT MediumLockList::Update(const ComObjPtr<Medium> &aMedium, bool aLockWrite)
{
    for (MediumLockList::Base::iterator it = mMediumLocks.begin();
         it != mMediumLocks.end();
         ++it)
    {
        if (it->GetMedium() == aMedium)
            return it->UpdateLock(aLockWrite);
    }
    return VBOX_E_INVALID_OBJECT_STATE;
}

HRESULT MediumLockList::RemoveByIterator(Base::iterator &aIt)
{
    HRESULT rc = aIt->Unlock();
    aIt = mMediumLocks.erase(aIt);
    return rc;
}

HRESULT MediumLockList::Clear()
{
    HRESULT rc = Unlock();
    mMediumLocks.clear();
    return rc;
}

MediumLockList::Base::iterator MediumLockList::GetBegin()
{
    return mMediumLocks.begin();
}

MediumLockList::Base::iterator MediumLockList::GetEnd()
{
    return mMediumLocks.end();
}

HRESULT MediumLockList::Lock(bool fSkipOverLockedMedia /* = false */)
{
    if (mIsLocked)
        return S_OK;
    HRESULT rc = S_OK;
    for (MediumLockList::Base::iterator it = mMediumLocks.begin();
         it != mMediumLocks.end();
         ++it)
    {
        rc = it->Lock(fSkipOverLockedMedia);
        if (FAILED(rc))
        {
            for (MediumLockList::Base::iterator it2 = mMediumLocks.begin();
                 it2 != it;
                 ++it2)
            {
                HRESULT rc2 = it2->Unlock();
                AssertComRC(rc2);
            }
            break;
        }
    }
    if (SUCCEEDED(rc))
        mIsLocked = true;
    return rc;
}

HRESULT MediumLockList::Unlock()
{
    if (!mIsLocked)
        return S_OK;
    HRESULT rc = S_OK;
    for (MediumLockList::Base::iterator it = mMediumLocks.begin();
         it != mMediumLocks.end();
         ++it)
    {
        HRESULT rc2 = it->Unlock();
        if (SUCCEEDED(rc) && FAILED(rc2))
            rc = rc2;
    }
    mIsLocked = false;
    return rc;
}


MediumLockListMap::MediumLockListMap()
{
    mIsLocked = false;
}

MediumLockListMap::~MediumLockListMap()
{
    // destroying medium lock list maps is routinely done as part of
    // error handling and it's not expected to lose error info
    ErrorInfoKeeper eik;
    Clear();
    // rest is done by the map object's destructor
}

bool MediumLockListMap::IsEmpty()
{
    return mMediumLocks.empty();
}

HRESULT MediumLockListMap::Insert(const ComObjPtr<MediumAttachment> &aMediumAttachment,
                              MediumLockList *aMediumLockList)
{
    if (mIsLocked)
        return VBOX_E_INVALID_OBJECT_STATE;
    mMediumLocks[aMediumAttachment] = aMediumLockList;
    return S_OK;
}

HRESULT MediumLockListMap::ReplaceKey(const ComObjPtr<MediumAttachment> &aMediumAttachmentOld,
                                  const ComObjPtr<MediumAttachment> &aMediumAttachmentNew)
{
    MediumLockListMap::Base::iterator it = mMediumLocks.find(aMediumAttachmentOld);
    if (it == mMediumLocks.end())
        return VBOX_E_INVALID_OBJECT_STATE;
    MediumLockList *pMediumLockList = it->second;
    mMediumLocks.erase(it);
    mMediumLocks[aMediumAttachmentNew] = pMediumLockList;
    return S_OK;
}

HRESULT MediumLockListMap::Remove(const ComObjPtr<MediumAttachment> &aMediumAttachment)
{
    MediumLockListMap::Base::iterator it = mMediumLocks.find(aMediumAttachment);
    if (it == mMediumLocks.end())
        return VBOX_E_INVALID_OBJECT_STATE;
    MediumLockList *pMediumLockList = it->second;
    mMediumLocks.erase(it);
    delete pMediumLockList;
    return S_OK;
}

HRESULT MediumLockListMap::Clear()
{
    HRESULT rc = Unlock();
    for (MediumLockListMap::Base::iterator it = mMediumLocks.begin();
         it != mMediumLocks.end();
         ++it)
    {
        MediumLockList *pMediumLockList = it->second;
        delete pMediumLockList;
    }
    mMediumLocks.clear();
    return rc;
}

HRESULT MediumLockListMap::Get(const ComObjPtr<MediumAttachment> &aMediumAttachment,
                           MediumLockList * &aMediumLockList)
{
    MediumLockListMap::Base::iterator it = mMediumLocks.find(aMediumAttachment);
    if (it == mMediumLocks.end())
    {
        aMediumLockList = NULL;
        return VBOX_E_INVALID_OBJECT_STATE;
    }
    aMediumLockList = it->second;
    return S_OK;
}

HRESULT MediumLockListMap::Lock()
{
    if (mIsLocked)
        return S_OK;
    HRESULT rc = S_OK;
    for (MediumLockListMap::Base::const_iterator it = mMediumLocks.begin();
         it != mMediumLocks.end();
         ++it)
    {
        rc = it->second->Lock();
        if (FAILED(rc))
        {
            for (MediumLockListMap::Base::const_iterator it2 = mMediumLocks.begin();
                 it2 != it;
                 ++it2)
            {
                HRESULT rc2 = it2->second->Unlock();
                AssertComRC(rc2);
            }
            break;
        }
    }
    if (SUCCEEDED(rc))
        mIsLocked = true;
    return rc;
}

HRESULT MediumLockListMap::Unlock()
{
    if (!mIsLocked)
        return S_OK;
    HRESULT rc = S_OK;
    for (MediumLockListMap::Base::const_iterator it = mMediumLocks.begin();
         it != mMediumLocks.end();
         ++it)
    {
        MediumLockList *pMediumLockList = it->second;
        HRESULT rc2 = pMediumLockList->Unlock();
        if (SUCCEEDED(rc) && FAILED(rc2))
            rc = rc2;
    }
    mIsLocked = false;
    return rc;
}
