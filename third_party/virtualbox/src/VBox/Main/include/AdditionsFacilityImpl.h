/* $Id: AdditionsFacilityImpl.h $ */
/** @file
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

#ifndef ____H_ADDITIONSFACILITYIMPL
#define ____H_ADDITIONSFACILITYIMPL

#include <vector>
#include <iprt/time.h>

#include "AdditionsFacilityWrap.h"

class Guest;

class ATL_NO_VTABLE AdditionsFacility :
    public AdditionsFacilityWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(AdditionsFacility)

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Guest *a_pParent, AdditionsFacilityType_T a_enmFacility, AdditionsFacilityStatus_T a_enmStatus,
                 uint32_t a_fFlags, PCRTTIMESPEC a_pTimeSpecTS);
    void uninit();

    HRESULT FinalConstruct();
    void FinalRelease();


public:
    /** Facility <-> string mappings. */
    struct FacilityInfo
    {
        /** The facilitie's name. */
        const char              *mName; /* utf-8 */
        /** The facilitie's type. */
        AdditionsFacilityType_T  mType;
        /** The facilitie's class. */
        AdditionsFacilityClass_T mClass;
    };
    static const FacilityInfo s_aFacilityInfo[8];

    // public internal methods
    static const AdditionsFacility::FacilityInfo &i_typeToInfo(AdditionsFacilityType_T aType);
    AdditionsFacilityClass_T i_getClass() const;
    LONG64 i_getLastUpdated() const;
    com::Utf8Str i_getName() const;
    AdditionsFacilityStatus_T i_getStatus() const;
    AdditionsFacilityType_T i_getType() const;
    void i_update(AdditionsFacilityStatus_T a_enmStatus, uint32_t a_fFlags, PCRTTIMESPEC a_pTimeSpecTS);

private:

    // Wrapped IAdditionsFacility properties
    HRESULT getClassType(AdditionsFacilityClass_T *aClassType);
    HRESULT getLastUpdated(LONG64 *aLastUpdated);
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getStatus(AdditionsFacilityStatus_T *aStatus);
    HRESULT getType(AdditionsFacilityType_T *aType);

    /** A structure for keeping a facility status
     *  set at a certain time. Good for book-keeping. */
    struct FacilityState
    {
        RTTIMESPEC                mTimestamp;
        /** The facilitie's current status. */
        AdditionsFacilityStatus_T mStatus;
    };

    struct Data
    {
        /** Record of current and previous facility
         *  states, limited to the 10 last states set.
         *  Note: This intentionally only is kept in
         *        Main so far! */
        std::vector<FacilityState> mStates;
        /** The facilitie's ID/type. */
        AdditionsFacilityType_T    mType;
    } mData;
};

#endif // ____H_ADDITIONSFACILITYIMPL

