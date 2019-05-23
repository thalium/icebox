/* $Id: BandwidthGroupImpl.h $ */
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

#ifndef ____H_BANDWIDTHGROUPIMPL
#define ____H_BANDWIDTHGROUPIMPL

#include <VBox/settings.h>
#include "BandwidthControlImpl.h"
#include "BandwidthGroupWrap.h"


class ATL_NO_VTABLE BandwidthGroup :
    public BandwidthGroupWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(BandwidthGroup)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(BandwidthControl *aParent,
                 const com::Utf8Str &aName,
                 BandwidthGroupType_T aType,
                 LONG64 aMaxBytesPerSec);
    HRESULT init(BandwidthControl *aParent, BandwidthGroup *aThat, bool aReshare = false);
    HRESULT initCopy(BandwidthControl *aParent, BandwidthGroup *aThat);
    void uninit();

    // public methods only for internal purposes
    void i_rollback();
    void i_commit();
    void i_unshare();
    void i_reference();
    void i_release();

    ComObjPtr<BandwidthGroup> i_getPeer() { return m->pPeer; }
    const Utf8Str &i_getName() const { return m->bd->mData.strName; }
    BandwidthGroupType_T i_getType() const { return m->bd->mData.enmType; }
    LONG64 i_getMaxBytesPerSec() const { return m->bd->mData.cMaxBytesPerSec; }
    ULONG i_getReferences() const { return m->bd->cReferences; }

private:

    // wrapped IBandwidthGroup properties
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getType(BandwidthGroupType_T *aType);
    HRESULT getReference(ULONG *aReferences);
    HRESULT getMaxBytesPerSec(LONG64 *aMaxBytesPerSec);
    HRESULT setMaxBytesPerSec(LONG64 MaxBytesPerSec);

    ////////////////////////////////////////////////////////////////////////////////
    ////
    //// private member data definition
    ////
    //////////////////////////////////////////////////////////////////////////////////
    //
    struct BackupableBandwidthGroupData
    {
       BackupableBandwidthGroupData()
           : cReferences(0)
       { }

       settings::BandwidthGroup mData;
       ULONG                    cReferences;
    };

    struct Data
    {
        Data(BandwidthControl * const aBandwidthControl)
            : pParent(aBandwidthControl),
              pPeer(NULL)
        { }

       BandwidthControl * const    pParent;
       ComObjPtr<BandwidthGroup>   pPeer;

       // use the XML settings structure in the members for simplicity
       Backupable<BackupableBandwidthGroupData> bd;
    };

    Data *m;
};

#endif // ____H_BANDWIDTHGROUPIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
