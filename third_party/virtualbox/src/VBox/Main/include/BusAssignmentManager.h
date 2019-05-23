/* $Id: BusAssignmentManager.h $ */

/** @file
 *
 * VirtualBox bus slots assignment manager
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef __BusAssignmentManager_h
#define __BusAssignmentManager_h

#include "VBox/types.h"
#include "VBox/pci.h"
#include "VirtualBoxBase.h"
#include <vector>

class BusAssignmentManager
{
private:
    struct State;
    State *pState;

    BusAssignmentManager();
    virtual ~BusAssignmentManager();

    HRESULT assignPCIDeviceImpl(const char *pszDevName, PCFGMNODE pCfg, PCIBusAddress& GuestAddress,
                                PCIBusAddress HostAddress, bool fGuestAddressRequired = false);

public:
    struct PCIDeviceInfo
    {
        com::Utf8Str strDeviceName;
        PCIBusAddress guestAddress;
        PCIBusAddress hostAddress;
    };

    static BusAssignmentManager *createInstance(ChipsetType_T chipsetType);
    virtual void AddRef();
    virtual void Release();

    virtual HRESULT assignHostPCIDevice(const char *pszDevName, PCFGMNODE pCfg, PCIBusAddress HostAddress,
                                        PCIBusAddress& GuestAddress, bool fAddressRequired = false)
    {
        return assignPCIDeviceImpl(pszDevName, pCfg, GuestAddress, HostAddress, fAddressRequired);
    }

    virtual HRESULT assignPCIDevice(const char *pszDevName, PCFGMNODE pCfg, PCIBusAddress& Address, bool fAddressRequired = false)
    {
        PCIBusAddress HostAddress;
        return assignPCIDeviceImpl(pszDevName, pCfg, Address, HostAddress, fAddressRequired);
    }

    virtual HRESULT assignPCIDevice(const char *pszDevName, PCFGMNODE pCfg)
    {
        PCIBusAddress GuestAddress;
        PCIBusAddress HostAddress;
        return assignPCIDeviceImpl(pszDevName, pCfg, GuestAddress, HostAddress, false);
    }
    virtual bool findPCIAddress(const char *pszDevName, int iInstance, PCIBusAddress& Address);
    virtual bool hasPCIDevice(const char *pszDevName, int iInstance)
    {
        PCIBusAddress Address;
        return findPCIAddress(pszDevName, iInstance, Address);
    }
    virtual void listAttachedPCIDevices(std::vector<PCIDeviceInfo> &aAttached);
};

#endif //  __BusAssignmentManager_h
