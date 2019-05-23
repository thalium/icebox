/* $Id: AdditionsFacilityImpl.cpp $ */
/** @file
 *
 * VirtualBox COM class implementation
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_MAIN_ADDITIONSFACILITY
#include "LoggingNew.h"

#include "AdditionsFacilityImpl.h"
#include "Global.h"

#include "AutoCaller.h"


/* static */
const AdditionsFacility::FacilityInfo AdditionsFacility::s_aFacilityInfo[8] =
{
    /* NOTE: We assume that unknown is always the first entry! */
    { "Unknown", AdditionsFacilityType_None, AdditionsFacilityClass_None },
    { "VirtualBox Base Driver", AdditionsFacilityType_VBoxGuestDriver, AdditionsFacilityClass_Driver },
    { "Auto Logon", AdditionsFacilityType_AutoLogon, AdditionsFacilityClass_Feature },
    { "VirtualBox System Service", AdditionsFacilityType_VBoxService, AdditionsFacilityClass_Service },
    { "VirtualBox Desktop Integration", AdditionsFacilityType_VBoxTrayClient, AdditionsFacilityClass_Program },
    { "Seamless Mode", AdditionsFacilityType_Seamless, AdditionsFacilityClass_Feature },
    { "Graphics Mode", AdditionsFacilityType_Graphics, AdditionsFacilityClass_Feature },
    { "Guest Monitor Attach", AdditionsFacilityType_MonitorAttach, AdditionsFacilityClass_Feature },
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(AdditionsFacility)

HRESULT AdditionsFacility::FinalConstruct()
{
    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void AdditionsFacility::FinalRelease()
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

HRESULT AdditionsFacility::init(Guest *a_pParent, AdditionsFacilityType_T a_enmFacility, AdditionsFacilityStatus_T a_enmStatus,
                                uint32_t a_fFlags, PCRTTIMESPEC a_pTimeSpecTS)
{
    RT_NOREF(a_pParent);
    LogFlowThisFunc(("a_pParent=%p\n", a_pParent));

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    FacilityState state;
    state.mStatus    = a_enmStatus;
    state.mTimestamp = *a_pTimeSpecTS;
    NOREF(a_fFlags);

    Assert(mData.mStates.size() == 0);
    mData.mStates.push_back(state);
    mData.mType      = a_enmFacility;
    /** @todo mClass is not initialized here. */
    NOREF(a_fFlags);

    /* Confirm a successful initialization when it's the case. */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void AdditionsFacility::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    mData.mStates.clear();
}

HRESULT AdditionsFacility::getClassType(AdditionsFacilityClass_T *aClassType)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aClassType = i_getClass();

    return S_OK;
}

HRESULT AdditionsFacility::getName(com::Utf8Str &aName)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aName = i_getName();

    return S_OK;
}

HRESULT AdditionsFacility::getLastUpdated(LONG64 *aLastUpdated)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aLastUpdated = i_getLastUpdated();

    return S_OK;
}

HRESULT AdditionsFacility::getStatus(AdditionsFacilityStatus_T *aStatus)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aStatus = i_getStatus();

    return S_OK;
}

HRESULT AdditionsFacility::getType(AdditionsFacilityType_T *aType)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aType = i_getType();

    return S_OK;
}

const AdditionsFacility::FacilityInfo &AdditionsFacility::i_typeToInfo(AdditionsFacilityType_T aType)
{
    for (size_t i = 0; i < RT_ELEMENTS(s_aFacilityInfo); ++i)
    {
        if (s_aFacilityInfo[i].mType == aType)
            return s_aFacilityInfo[i];
    }
    return s_aFacilityInfo[0]; /* Return unknown type. */
}

AdditionsFacilityClass_T AdditionsFacility::i_getClass() const
{
    return AdditionsFacility::i_typeToInfo(mData.mType).mClass;
}

com::Utf8Str AdditionsFacility::i_getName() const
{
    return AdditionsFacility::i_typeToInfo(mData.mType).mName;
}

LONG64 AdditionsFacility::i_getLastUpdated() const
{
    if (mData.mStates.size())
        return RTTimeSpecGetMilli(&mData.mStates.front().mTimestamp);

    AssertMsgFailed(("Unknown timestamp of facility!\n"));
    return 0; /* Should never happen! */
}

AdditionsFacilityStatus_T AdditionsFacility::i_getStatus() const
{
    if (mData.mStates.size())
        return mData.mStates.back().mStatus;

    AssertMsgFailed(("Unknown status of facility!\n"));
    return AdditionsFacilityStatus_Unknown; /* Should never happen! */
}

AdditionsFacilityType_T AdditionsFacility::i_getType() const
{
    return mData.mType;
}

/**
 * Method used by IGuest::facilityUpdate to make updates.
 */
void AdditionsFacility::i_update(AdditionsFacilityStatus_T a_enmStatus, uint32_t a_fFlags, PCRTTIMESPEC a_pTimeSpecTS)
{
    FacilityState state;
    state.mStatus    = a_enmStatus;
    state.mTimestamp = *a_pTimeSpecTS;
    NOREF(a_fFlags);

    mData.mStates.push_back(state);
    if (mData.mStates.size() > 10) /* Only keep the last 10 states. */
        mData.mStates.erase(mData.mStates.begin());
}

