/* $Id: PCIDeviceAttachmentImpl.cpp $ */

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

#include "PCIDeviceAttachmentImpl.h"
#include "AutoCaller.h"
#include "Global.h"
#include "Logging.h"

#include <VBox/settings.h>

struct PCIDeviceAttachment::Data
{
    Data(const Utf8Str &aDevName,
         LONG          aHostAddress,
         LONG          aGuestAddress,
         BOOL          afPhysical) :
        DevName(aDevName),
        HostAddress(aHostAddress),
        GuestAddress(aGuestAddress),
        fPhysical(afPhysical)
    {
    }

    Utf8Str          DevName;
    LONG             HostAddress;
    LONG             GuestAddress;
    BOOL             fPhysical;
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////
DEFINE_EMPTY_CTOR_DTOR(PCIDeviceAttachment)

HRESULT PCIDeviceAttachment::FinalConstruct()
{
    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void PCIDeviceAttachment::FinalRelease()
{
    LogFlowThisFunc(("\n"));
    uninit();
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////
HRESULT PCIDeviceAttachment::init(IMachine      *aParent,
                                  const Utf8Str &aDevName,
                                  LONG          aHostAddress,
                                  LONG          aGuestAddress,
                                  BOOL          fPhysical)
{
    NOREF(aParent);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data(aDevName, aHostAddress, aGuestAddress, fPhysical);

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

HRESULT PCIDeviceAttachment::initCopy(IMachine *aParent, PCIDeviceAttachment *aThat)
{
    LogFlowThisFunc(("aParent=%p, aThat=%p\n", aParent, aThat));

    ComAssertRet(aParent && aThat, E_INVALIDARG);

    return init(aParent, aThat->m->DevName, aThat->m->HostAddress, aThat->m->GuestAddress, aThat->m->fPhysical);
}

HRESULT PCIDeviceAttachment::i_loadSettings(IMachine *aParent,
                                            const settings::HostPCIDeviceAttachment &hpda)
{
    return init(aParent, hpda.strDeviceName, hpda.uHostAddress, hpda.uGuestAddress, TRUE);
}


HRESULT PCIDeviceAttachment::i_saveSettings(settings::HostPCIDeviceAttachment &data)
{
    Assert(m);
    data.uHostAddress = m->HostAddress;
    data.uGuestAddress = m->GuestAddress;
    data.strDeviceName = m->DevName;

    return S_OK;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void PCIDeviceAttachment::uninit()
{
    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    delete m;
    m = NULL;
}

// IPCIDeviceAttachment properties
/////////////////////////////////////////////////////////////////////////////
HRESULT PCIDeviceAttachment::getName(com::Utf8Str &aName)
{
    aName = m->DevName;
    return S_OK;
}

HRESULT PCIDeviceAttachment::getIsPhysicalDevice(BOOL *aIsPhysicalDevice)
{
    *aIsPhysicalDevice = m->fPhysical;
    return S_OK;
}

HRESULT PCIDeviceAttachment::getHostAddress(LONG *aHostAddress)
{
    *aHostAddress = m->HostAddress;
    return S_OK;
}
HRESULT PCIDeviceAttachment::getGuestAddress(LONG *aGuestAddress)
{
    *aGuestAddress = m->GuestAddress;
    return S_OK;
}
