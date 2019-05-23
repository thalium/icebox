/* $Id: USBControllerImpl.h $ */

/** @file
 *
 * VBox USBController COM Class declaration.
 */

/*
 * Copyright (C) 2005-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_USBCONTROLLERIMPL
#define ____H_USBCONTROLLERIMPL

#include "USBControllerWrap.h"

class HostUSBDevice;
class USBDeviceFilter;

namespace settings
{
    struct USBController;
}

class ATL_NO_VTABLE USBController :
    public USBControllerWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(USBController)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent, const com::Utf8Str &aName, USBControllerType_T enmType);
    HRESULT init(Machine *aParent, USBController *aThat, bool fReshare = false);
    HRESULT initCopy(Machine *aParent, USBController *aThat);
    void uninit();

    // public methods only for internal purposes
    void i_rollback();
    void i_commit();
    void i_copyFrom(USBController *aThat);
    void i_unshare();

    ComObjPtr<USBController> i_getPeer();
    const Utf8Str &i_getName() const;
    const USBControllerType_T &i_getControllerType() const;

private:

    // wrapped IUSBController properties
    HRESULT getName(com::Utf8Str &aName);
    HRESULT setName(const com::Utf8Str &aName);
    HRESULT getType(USBControllerType_T *aType);
    HRESULT setType(USBControllerType_T aType);
    HRESULT getUSBStandard(USHORT *aUSBStandard);

    void printList();

    struct Data;
    Data *m;
};

#endif //!____H_USBCONTROLLERIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
