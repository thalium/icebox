/* $Id: PCIDeviceAttachmentImpl.h $ */

/** @file
 *
 * PCI attachment information implmentation.
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

#ifndef ____H_PCIDEVICEATTACHMENTIMPL
#define ____H_PCIDEVICEATTACHMENTIMPL

#include "PCIDeviceAttachmentWrap.h"

namespace settings
{
    struct HostPCIDeviceAttachment;
}

class ATL_NO_VTABLE PCIDeviceAttachment :
    public PCIDeviceAttachmentWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(PCIDeviceAttachment)

    // public initializer/uninitializer for internal purposes only
    HRESULT init(IMachine *    aParent,
                 const Utf8Str &aDevName,
                 LONG          aHostAddess,
                 LONG          aGuestAddress,
                 BOOL          fPhysical);
    HRESULT initCopy(IMachine *aParent, PCIDeviceAttachment *aThat);
    void uninit();

    // settings
    HRESULT i_loadSettings(IMachine * aParent,
                           const settings::HostPCIDeviceAttachment& aHpda);
    HRESULT i_saveSettings(settings::HostPCIDeviceAttachment &data);

    HRESULT FinalConstruct();
    void FinalRelease();

private:

    // wrapped IPCIDeviceAttachment properties
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getIsPhysicalDevice(BOOL *aIsPhysicalDevice);
    HRESULT getHostAddress(LONG *aHostAddress);
    HRESULT getGuestAddress(LONG *aGuestAddress);

    struct Data;
    Data*  m;
};

#endif // ____H_PCIDEVICEATTACHMENTIMPL
