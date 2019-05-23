/* $Id: StorageControllerImpl.h $ */

/** @file
 *
 * VBox StorageController COM Class declaration.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_STORAGECONTROLLERIMPL
#define ____H_STORAGECONTROLLERIMPL
#include "StorageControllerWrap.h"

class ATL_NO_VTABLE StorageController :
    public StorageControllerWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(StorageController)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent,
                 const com::Utf8Str &aName,
                 StorageBus_T aBus,
                 ULONG aInstance,
                 bool fBootable);
    HRESULT init(Machine *aParent,
                 StorageController *aThat,
                 bool aReshare = false);
    HRESULT initCopy(Machine *aParent,
                     StorageController *aThat);
    void uninit();

    // public methods only for internal purposes
    const Utf8Str &i_getName() const;
    StorageControllerType_T i_getControllerType() const;
    StorageBus_T i_getStorageBus() const;
    ULONG i_getInstance() const;
    bool i_getBootable() const;
    HRESULT i_checkPortAndDeviceValid(LONG aControllerPort,
                                      LONG aDevice);
    void i_setBootable(BOOL fBootable);
    void i_rollback();
    void i_commit();

    // public methods for internal purposes only
    // (ensure there is a caller and a read lock before calling them!)

    void i_unshare();

    /** @note this doesn't require a read lock since mParent is constant. */
    Machine* i_getMachine();
    ComObjPtr<StorageController> i_getPeer();

private:

    // Wrapped IStorageController properties
    HRESULT getName(com::Utf8Str &aName);
    HRESULT setName(const com::Utf8Str &aName);
    HRESULT getMaxDevicesPerPortCount(ULONG *aMaxDevicesPerPortCount);
    HRESULT getMinPortCount(ULONG *aMinPortCount);
    HRESULT getMaxPortCount(ULONG *aMaxPortCount);
    HRESULT getInstance(ULONG *aInstance);
    HRESULT setInstance(ULONG aInstance);
    HRESULT getPortCount(ULONG *aPortCount);
    HRESULT setPortCount(ULONG aPortCount);
    HRESULT getBus(StorageBus_T *aBus);
    HRESULT getControllerType(StorageControllerType_T *aControllerType);
    HRESULT setControllerType(StorageControllerType_T aControllerType);
    HRESULT getUseHostIOCache(BOOL *aUseHostIOCache);
    HRESULT setUseHostIOCache(BOOL aUseHostIOCache);
    HRESULT getBootable(BOOL *aBootable);

    void i_printList();

    struct Data;
    Data *m;
};

#endif //!____H_STORAGECONTROLLERIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
