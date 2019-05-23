/* $Id: BandwidthControlImpl.h $ */
/** @file
 *
 * VirtualBox COM class implementation
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

#ifndef ____H_BANDWIDTHCONTROLIMPL
#define ____H_BANDWIDTHCONTROLIMPL

#include "BandwidthControlWrap.h"

class BandwidthGroup;

namespace settings
{
    struct IOSettings;
}

class ATL_NO_VTABLE BandwidthControl :
    public BandwidthControlWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(BandwidthControl)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent);
    HRESULT init(Machine *aParent, BandwidthControl *aThat);
    HRESULT initCopy(Machine *aParent, BandwidthControl *aThat);
    void uninit();

    // public internal methods
    HRESULT i_loadSettings(const settings::IOSettings &data);
    HRESULT i_saveSettings(settings::IOSettings &data);
    void i_rollback();
    void i_commit();
    void i_copyFrom(BandwidthControl *aThat);
    Machine *i_getMachine() const;
    HRESULT i_getBandwidthGroupByName(const Utf8Str &aName,
                                      ComObjPtr<BandwidthGroup> &aBandwidthGroup,
                                      bool aSetError /* = false */);

private:

    // wrapped IBandwidthControl properties
    HRESULT getNumGroups(ULONG *aNumGroups);

    // wrapped IBandwidthControl methods
    HRESULT createBandwidthGroup(const com::Utf8Str &aName,
                                       BandwidthGroupType_T aType,
                                       LONG64 aMaxBytesPerSec);
    HRESULT deleteBandwidthGroup(const com::Utf8Str &aName);
    HRESULT getBandwidthGroup(const com::Utf8Str &aName,
                                    ComPtr<IBandwidthGroup> &aBandwidthGroup);
    HRESULT getAllBandwidthGroups(std::vector<ComPtr<IBandwidthGroup> > &aBandwidthGroups);

    // Data
    typedef std::list< ComObjPtr<BandwidthGroup> > BandwidthGroupList;

    struct Data
    {
        Data(Machine *pMachine)
        : pParent(pMachine)
        { }

        ~Data()
        {};

        Machine * const                 pParent;

        // peer machine's bandwidth control
        const ComObjPtr<BandwidthControl>  pPeer;

        // the following fields need special backup/rollback/commit handling,
        // so they cannot be a part of BackupableData
        Backupable<BandwidthGroupList>    llBandwidthGroups;
    };

    Data *m;
};

#endif // ____H_BANDWIDTHCONTROLIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
