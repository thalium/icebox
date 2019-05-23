/* $Id: BusAssignmentManager.cpp $ */
/** @file
 * VirtualBox bus slots assignment manager
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

#define LOG_GROUP LOG_GROUP_MAIN
#include "LoggingNew.h"

#include "BusAssignmentManager.h"

#include <iprt/asm.h>
#include <iprt/string.h>

#include <VBox/vmm/cfgm.h>
#include <VBox/com/array.h>

#include <map>
#include <vector>
#include <algorithm>

struct DeviceAssignmentRule
{
    const char *pszName;
    int         iBus;
    int         iDevice;
    int         iFn;
    int         iPriority;
};

struct DeviceAliasRule
{
    const char *pszDevName;
    const char *pszDevAlias;
};

/* Those rules define PCI slots assignment */
/** @note
 * The EFI takes assumptions about PCI slot assignments which are different
 * from the following tables in certain cases, for example the IDE device
 * is assumed to be 00:01.1! */

/* Device           Bus  Device Function Priority */

/* Generic rules */
static const DeviceAssignmentRule aGenericRules[] =
{
    /* VGA controller */
    {"vga",           0,  2, 0,  0},

    /* VMM device */
    {"VMMDev",        0,  4, 0,  0},

    /* Audio controllers */
    {"ichac97",       0,  5, 0,  0},
    {"hda",           0,  5, 0,  0},

    /* Storage controllers */
    {"lsilogic",      0, 20, 0,  1},
    {"buslogic",      0, 21, 0,  1},
    {"lsilogicsas",   0, 22, 0,  1},
    {"nvme",          0, 14, 0,  1},

    /* USB controllers */
    {"usb-ohci",      0,  6,  0, 0},
    {"usb-ehci",      0, 11,  0, 0},
    {"usb-xhci",      0, 12,  0, 0},

    /* ACPI controller */
    {"acpi",          0,  7,  0, 0},

    /* Network controllers */
    /* the first network card gets the PCI ID 3, the next 3 gets 8..10,
     * next 4 get 16..19. In "VMWare compatibility" mode the IDs 3 and 17
     * swap places, i.e. the first card goes to ID 17=0x11. */
    {"nic",           0,  3,  0, 1},
    {"nic",           0,  8,  0, 1},
    {"nic",           0,  9,  0, 1},
    {"nic",           0, 10,  0, 1},
    {"nic",           0, 16,  0, 1},
    {"nic",           0, 17,  0, 1},
    {"nic",           0, 18,  0, 1},
    {"nic",           0, 19,  0, 1},

    /* ISA/LPC controller */
    {"lpc",           0, 31,  0, 0},

    { NULL,          -1, -1, -1,  0}
};

/* PIIX3 chipset rules */
static const DeviceAssignmentRule aPiix3Rules[] =
{
    {"piix3ide",      0,  1,  1, 0},
    {"ahci",          0, 13,  0, 1},
    {"pcibridge",     0, 24,  0, 0},
    {"pcibridge",     0, 25,  0, 0},
    { NULL,          -1, -1, -1, 0}
};


/* ICH9 chipset rules */
static const DeviceAssignmentRule aIch9Rules[] =
{
    /* Host Controller */
    {"i82801",        0, 30, 0,  0},

    /* Those are functions of LPC at 00:1e:00 */
    /**
     *  Please note, that for devices being functions, like we do here, device 0
     *  must be multifunction, i.e. have header type 0x80. Our LPC device is.
     *  Alternative approach is to assign separate slot to each device.
     */
    {"piix3ide",      0, 31, 1,  2},
    {"ahci",          0, 31, 2,  2},
    {"smbus",         0, 31, 3,  2},
    {"usb-ohci",      0, 31, 4,  2},
    {"usb-ehci",      0, 31, 5,  2},
    {"thermal",       0, 31, 6,  2},

    /* to make sure rule never used before rules assigning devices on it */
    {"ich9pcibridge", 0, 24, 0,  10},
    {"ich9pcibridge", 0, 25, 0,  10},
    {"ich9pcibridge", 2, 24, 0,   9}, /* Bridges must be instantiated depth */
    {"ich9pcibridge", 2, 25, 0,   9}, /* first (assumption in PDM and other */
    {"ich9pcibridge", 4, 24, 0,   8}, /* places), so make sure that nested */
    {"ich9pcibridge", 4, 25, 0,   8}, /* bridges are added to the last bridge */
    {"ich9pcibridge", 6, 24, 0,   7}, /* only, avoiding the need to re-sort */
    {"ich9pcibridge", 6, 25, 0,   7}, /* everything before starting the VM. */
    {"ich9pcibridge", 8, 24, 0,   6},
    {"ich9pcibridge", 8, 25, 0,   6},
    {"ich9pcibridge", 10, 24, 0,  5},
    {"ich9pcibridge", 10, 25, 0,  5},

    /* Storage controllers */
    {"ahci",          1,  0, 0,   0},
    {"ahci",          1,  1, 0,   0},
    {"ahci",          1,  2, 0,   0},
    {"ahci",          1,  3, 0,   0},
    {"ahci",          1,  4, 0,   0},
    {"ahci",          1,  5, 0,   0},
    {"ahci",          1,  6, 0,   0},
    {"lsilogic",      1,  7, 0,   0},
    {"lsilogic",      1,  8, 0,   0},
    {"lsilogic",      1,  9, 0,   0},
    {"lsilogic",      1, 10, 0,   0},
    {"lsilogic",      1, 11, 0,   0},
    {"lsilogic",      1, 12, 0,   0},
    {"lsilogic",      1, 13, 0,   0},
    {"buslogic",      1, 14, 0,   0},
    {"buslogic",      1, 15, 0,   0},
    {"buslogic",      1, 16, 0,   0},
    {"buslogic",      1, 17, 0,   0},
    {"buslogic",      1, 18, 0,   0},
    {"buslogic",      1, 19, 0,   0},
    {"buslogic",      1, 20, 0,   0},
    {"lsilogicsas",   1, 21, 0,   0},
    {"lsilogicsas",   1, 26, 0,   0},
    {"lsilogicsas",   1, 27, 0,   0},
    {"lsilogicsas",   1, 28, 0,   0},
    {"lsilogicsas",   1, 29, 0,   0},
    {"lsilogicsas",   1, 30, 0,   0},
    {"lsilogicsas",   1, 31, 0,   0},

    /* NICs */
    {"nic",           2,  0, 0,   0},
    {"nic",           2,  1, 0,   0},
    {"nic",           2,  2, 0,   0},
    {"nic",           2,  3, 0,   0},
    {"nic",           2,  4, 0,   0},
    {"nic",           2,  5, 0,   0},
    {"nic",           2,  6, 0,   0},
    {"nic",           2,  7, 0,   0},
    {"nic",           2,  8, 0,   0},
    {"nic",           2,  9, 0,   0},
    {"nic",           2, 10, 0,   0},
    {"nic",           2, 11, 0,   0},
    {"nic",           2, 12, 0,   0},
    {"nic",           2, 13, 0,   0},
    {"nic",           2, 14, 0,   0},
    {"nic",           2, 15, 0,   0},
    {"nic",           2, 16, 0,   0},
    {"nic",           2, 17, 0,   0},
    {"nic",           2, 18, 0,   0},
    {"nic",           2, 19, 0,   0},
    {"nic",           2, 20, 0,   0},
    {"nic",           2, 21, 0,   0},
    {"nic",           2, 26, 0,   0},
    {"nic",           2, 27, 0,   0},
    {"nic",           2, 28, 0,   0},
    {"nic",           2, 29, 0,   0},
    {"nic",           2, 30, 0,   0},
    {"nic",           2, 31, 0,   0},

    /* Storage controller #2 (NVMe) */
    {"nvme",          3,  0, 0,   0},
    {"nvme",          3,  1, 0,   0},
    {"nvme",          3,  2, 0,   0},
    {"nvme",          3,  3, 0,   0},
    {"nvme",          3,  4, 0,   0},
    {"nvme",          3,  5, 0,   0},
    {"nvme",          3,  6, 0,   0},

    { NULL,          -1, -1, -1,  0}
};

/* Aliasing rules */
static const DeviceAliasRule aDeviceAliases[] =
{
    {"e1000",       "nic"},
    {"pcnet",       "nic"},
    {"virtio-net",  "nic"},
    {"ahci",        "storage"},
    {"lsilogic",    "storage"},
    {"buslogic",    "storage"},
    {"lsilogicsas", "storage"},
    {"nvme",        "storage"}
};

struct BusAssignmentManager::State
{
    struct PCIDeviceRecord
    {
        char          szDevName[32];
        PCIBusAddress HostAddress;

        PCIDeviceRecord(const char *pszName, PCIBusAddress aHostAddress)
        {
            RTStrCopy(this->szDevName, sizeof(szDevName), pszName);
            this->HostAddress = aHostAddress;
        }

        PCIDeviceRecord(const char *pszName)
        {
            RTStrCopy(this->szDevName, sizeof(szDevName), pszName);
        }

        bool operator<(const PCIDeviceRecord &a) const
        {
            return RTStrNCmp(szDevName, a.szDevName, sizeof(szDevName)) < 0;
        }

        bool operator==(const PCIDeviceRecord &a) const
        {
            return RTStrNCmp(szDevName, a.szDevName, sizeof(szDevName)) == 0;
        }
    };

    typedef std::map<PCIBusAddress,PCIDeviceRecord>   PCIMap;
    typedef std::vector<PCIBusAddress>                PCIAddrList;
    typedef std::vector<const DeviceAssignmentRule *> PCIRulesList;
    typedef std::map<PCIDeviceRecord,PCIAddrList>     ReversePCIMap;

    volatile int32_t cRefCnt;
    ChipsetType_T    mChipsetType;
    const char *     mpszBridgeName;
    PCIMap           mPCIMap;
    ReversePCIMap    mReversePCIMap;

    State()
        : cRefCnt(1), mChipsetType(ChipsetType_Null), mpszBridgeName("unknownbridge")
    {}
    ~State()
    {}

    HRESULT init(ChipsetType_T chipsetType);

    HRESULT record(const char *pszName, PCIBusAddress& GuestAddress, PCIBusAddress HostAddress);
    HRESULT autoAssign(const char *pszName, PCIBusAddress& Address);
    bool    checkAvailable(PCIBusAddress& Address);
    bool    findPCIAddress(const char *pszDevName, int iInstance, PCIBusAddress& Address);

    const char *findAlias(const char *pszName);
    void addMatchingRules(const char *pszName, PCIRulesList& aList);
    void listAttachedPCIDevices(std::vector<PCIDeviceInfo> &aAttached);
};

HRESULT BusAssignmentManager::State::init(ChipsetType_T chipsetType)
{
    mChipsetType = chipsetType;
    switch (chipsetType)
    {
        case ChipsetType_PIIX3:
            mpszBridgeName = "pcibridge";
            break;
        case ChipsetType_ICH9:
            mpszBridgeName = "ich9pcibridge";
            break;
        default:
            mpszBridgeName = "unknownbridge";
            AssertFailed();
            break;
    }
    return S_OK;
}

HRESULT BusAssignmentManager::State::record(const char *pszName, PCIBusAddress& Address, PCIBusAddress HostAddress)
{
    PCIDeviceRecord devRec(pszName, HostAddress);

    /* Remember address -> device mapping */
    mPCIMap.insert(PCIMap::value_type(Address, devRec));

    ReversePCIMap::iterator it = mReversePCIMap.find(devRec);
    if (it == mReversePCIMap.end())
    {
        mReversePCIMap.insert(ReversePCIMap::value_type(devRec, PCIAddrList()));
        it = mReversePCIMap.find(devRec);
    }

    /* Remember device name -> addresses mapping */
    it->second.push_back(Address);

    return S_OK;
}

bool    BusAssignmentManager::State::findPCIAddress(const char *pszDevName, int iInstance, PCIBusAddress& Address)
{
    PCIDeviceRecord devRec(pszDevName);

    ReversePCIMap::iterator it = mReversePCIMap.find(devRec);
    if (it == mReversePCIMap.end())
        return false;

    if (iInstance >= (int)it->second.size())
        return false;

    Address = it->second[iInstance];
    return true;
}

void BusAssignmentManager::State::addMatchingRules(const char *pszName, PCIRulesList& aList)
{
    size_t iRuleset, iRule;
    const DeviceAssignmentRule *aArrays[2] = {aGenericRules, NULL};

    switch (mChipsetType)
    {
        case ChipsetType_PIIX3:
            aArrays[1] = aPiix3Rules;
            break;
        case ChipsetType_ICH9:
            aArrays[1] = aIch9Rules;
            break;
        default:
            AssertFailed();
            break;
    }

    for (iRuleset = 0; iRuleset < RT_ELEMENTS(aArrays); iRuleset++)
    {
        if (aArrays[iRuleset] == NULL)
            continue;

        for (iRule = 0; aArrays[iRuleset][iRule].pszName != NULL; iRule++)
        {
            if (RTStrCmp(pszName, aArrays[iRuleset][iRule].pszName) == 0)
                aList.push_back(&aArrays[iRuleset][iRule]);
        }
    }
}

const char *BusAssignmentManager::State::findAlias(const char *pszDev)
{
    for (size_t iAlias = 0; iAlias < RT_ELEMENTS(aDeviceAliases); iAlias++)
    {
        if (strcmp(pszDev, aDeviceAliases[iAlias].pszDevName) == 0)
            return aDeviceAliases[iAlias].pszDevAlias;
    }
    return NULL;
}

static bool  RuleComparator(const DeviceAssignmentRule *r1, const DeviceAssignmentRule *r2)
{
    return (r1->iPriority > r2->iPriority);
}

HRESULT BusAssignmentManager::State::autoAssign(const char *pszName, PCIBusAddress& Address)
{
    PCIRulesList matchingRules;

    addMatchingRules(pszName,  matchingRules);
    const char *pszAlias = findAlias(pszName);
    if (pszAlias)
        addMatchingRules(pszAlias, matchingRules);

    AssertMsg(matchingRules.size() > 0, ("No rule for %s(%s)\n", pszName, pszAlias));

    stable_sort(matchingRules.begin(), matchingRules.end(), RuleComparator);

    for (size_t iRule = 0; iRule < matchingRules.size(); iRule++)
    {
        const DeviceAssignmentRule *rule = matchingRules[iRule];

        Address.miBus = rule->iBus;
        Address.miDevice = rule->iDevice;
        Address.miFn = rule->iFn;

        if (checkAvailable(Address))
            return S_OK;
    }
    AssertLogRelMsgFailed(("BusAssignmentManager: All possible candidate positions for %s exhausted\n", pszName));

    return E_INVALIDARG;
}

bool BusAssignmentManager::State::checkAvailable(PCIBusAddress& Address)
{
    PCIMap::const_iterator it = mPCIMap.find(Address);

    return (it == mPCIMap.end());
}

void BusAssignmentManager::State::listAttachedPCIDevices(std::vector<PCIDeviceInfo> &aAttached)
{
    aAttached.resize(mPCIMap.size());

    size_t i = 0;
    PCIDeviceInfo dev;
    for (PCIMap::const_iterator it = mPCIMap.begin(); it !=  mPCIMap.end(); ++it, ++i)
    {
        dev.strDeviceName = it->second.szDevName;
        dev.guestAddress = it->first;
        dev.hostAddress = it->second.HostAddress;
        aAttached[i] = dev;
    }
}

BusAssignmentManager::BusAssignmentManager()
    : pState(NULL)
{
    pState = new State();
    Assert(pState);
}

BusAssignmentManager::~BusAssignmentManager()
{
    if (pState)
    {
        delete pState;
        pState = NULL;
    }
}

BusAssignmentManager *BusAssignmentManager::createInstance(ChipsetType_T chipsetType)
{
    BusAssignmentManager *pInstance = new BusAssignmentManager();
    pInstance->pState->init(chipsetType);
    Assert(pInstance);
    return pInstance;
}

void BusAssignmentManager::AddRef()
{
    ASMAtomicIncS32(&pState->cRefCnt);
}
void BusAssignmentManager::Release()
{
    if (ASMAtomicDecS32(&pState->cRefCnt) == 0)
        delete this;
}

DECLINLINE(HRESULT) InsertConfigInteger(PCFGMNODE pCfg,  const char *pszName, uint64_t u64)
{
    int vrc = CFGMR3InsertInteger(pCfg, pszName, u64);
    if (RT_FAILURE(vrc))
        return E_INVALIDARG;

    return S_OK;
}

DECLINLINE(HRESULT) InsertConfigNode(PCFGMNODE pNode, const char *pcszName, PCFGMNODE *ppChild)
{
    int vrc = CFGMR3InsertNode(pNode, pcszName, ppChild);
    if (RT_FAILURE(vrc))
        return E_INVALIDARG;

    return S_OK;
}


HRESULT BusAssignmentManager::assignPCIDeviceImpl(const char *pszDevName,
                                                  PCFGMNODE pCfg,
                                                  PCIBusAddress& GuestAddress,
                                                  PCIBusAddress HostAddress,
                                                  bool fGuestAddressRequired)
{
    HRESULT rc = S_OK;

    if (!GuestAddress.valid())
        rc = pState->autoAssign(pszDevName, GuestAddress);
    else
    {
        bool fAvailable = pState->checkAvailable(GuestAddress);

        if (!fAvailable)
        {
            if (fGuestAddressRequired)
                rc = E_ACCESSDENIED;
            else
                rc = pState->autoAssign(pszDevName, GuestAddress);
        }
    }

    if (FAILED(rc))
        return rc;

    Assert(GuestAddress.valid() && pState->checkAvailable(GuestAddress));

    rc = pState->record(pszDevName, GuestAddress, HostAddress);
    if (FAILED(rc))
        return rc;

    rc = InsertConfigInteger(pCfg, "PCIBusNo",      GuestAddress.miBus);
    if (FAILED(rc))
        return rc;
    rc = InsertConfigInteger(pCfg, "PCIDeviceNo",   GuestAddress.miDevice);
    if (FAILED(rc))
        return rc;
    rc = InsertConfigInteger(pCfg, "PCIFunctionNo", GuestAddress.miFn);
    if (FAILED(rc))
        return rc;

    /* Check if the bus is still unknown, i.e. the bridge to it is missing */
    if (   GuestAddress.miBus > 0
        && !hasPCIDevice(pState->mpszBridgeName, GuestAddress.miBus - 1))
    {
        PCFGMNODE pDevices = CFGMR3GetParent(CFGMR3GetParent(pCfg));
        AssertLogRelMsgReturn(pDevices, ("BusAssignmentManager: cannot find base device configuration\n"), E_UNEXPECTED);
        PCFGMNODE pBridges = CFGMR3GetChild(pDevices, "ich9pcibridge");
        AssertLogRelMsgReturn(pBridges, ("BusAssignmentManager: cannot find bridge configuration base\n"), E_UNEXPECTED);

        /* Device should be on a not yet existing bus, add it automatically */
        for (int iBridge = 0; iBridge <= GuestAddress.miBus - 1; iBridge++)
        {
            if (!hasPCIDevice(pState->mpszBridgeName, iBridge))
            {
                PCIBusAddress BridgeGuestAddress;
                rc = pState->autoAssign(pState->mpszBridgeName, BridgeGuestAddress);
                if (FAILED(rc))
                    return rc;
                if (BridgeGuestAddress.miBus > iBridge)
                    AssertLogRelMsgFailedReturn(("BusAssignmentManager: cannot create bridge for bus %i because the possible parent bus positions are exhausted\n", iBridge + 1), E_UNEXPECTED);

                PCFGMNODE pInst;
                InsertConfigNode(pBridges, Utf8StrFmt("%d", iBridge).c_str(), &pInst);
                InsertConfigInteger(pInst, "Trusted", 1);
                rc = assignPCIDevice(pState->mpszBridgeName, pInst);
                if (FAILED(rc))
                    return rc;
            }
        }
    }

    return S_OK;
}


bool BusAssignmentManager::findPCIAddress(const char *pszDevName, int iInstance, PCIBusAddress& Address)
{
    return pState->findPCIAddress(pszDevName, iInstance, Address);
}
void BusAssignmentManager::listAttachedPCIDevices(std::vector<PCIDeviceInfo> &aAttached)
{
    pState->listAttachedPCIDevices(aAttached);
}
