/* $Id: SystemPropertiesImpl.h $ */

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

#ifndef ____H_SYSTEMPROPERTIESIMPL
#define ____H_SYSTEMPROPERTIESIMPL

#include "MediumFormatImpl.h"
#include "SystemPropertiesWrap.h"


namespace settings
{
    struct SystemProperties;
}

class ATL_NO_VTABLE SystemProperties :
    public SystemPropertiesWrap
{
public:
    typedef std::list<ComObjPtr<MediumFormat> > MediumFormatList;

    DECLARE_EMPTY_CTOR_DTOR(SystemProperties)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(VirtualBox *aParent);
    void uninit();

    // public methods for internal purposes only
    // (ensure there is a caller and a read lock before calling them!)
    HRESULT i_loadSettings(const settings::SystemProperties &data);
    HRESULT i_saveSettings(settings::SystemProperties &data);

    ComObjPtr<MediumFormat> i_mediumFormat(const Utf8Str &aFormat);
    ComObjPtr<MediumFormat> i_mediumFormatFromExtension(const Utf8Str &aExt);

    int i_loadVDPlugin(const char *pszPluginLibrary);
    int i_unloadVDPlugin(const char *pszPluginLibrary);

    HRESULT i_getDefaultAdditionsISO(com::Utf8Str &aDefaultAdditionsISO);

private:

    // wrapped ISystemProperties properties
    HRESULT getMinGuestRAM(ULONG *aMinGuestRAM);
    HRESULT getMaxGuestRAM(ULONG *aMaxGuestRAM);
    HRESULT getMinGuestVRAM(ULONG *aMinGuestVRAM);
    HRESULT getMaxGuestVRAM(ULONG *aMaxGuestVRAM);
    HRESULT getMinGuestCPUCount(ULONG *aMinGuestCPUCount);
    HRESULT getMaxGuestCPUCount(ULONG *aMaxGuestCPUCount);
    HRESULT getMaxGuestMonitors(ULONG *aMaxGuestMonitors);
    HRESULT getInfoVDSize(LONG64 *aInfoVDSize);
    HRESULT getSerialPortCount(ULONG *aSerialPortCount);
    HRESULT getParallelPortCount(ULONG *aParallelPortCount);
    HRESULT getMaxBootPosition(ULONG *aMaxBootPosition);
    HRESULT getRawModeSupported(BOOL *aRawModeSupported);
    HRESULT getExclusiveHwVirt(BOOL *aExclusiveHwVirt);
    HRESULT setExclusiveHwVirt(BOOL aExclusiveHwVirt);
    HRESULT getDefaultMachineFolder(com::Utf8Str &aDefaultMachineFolder);
    HRESULT setDefaultMachineFolder(const com::Utf8Str &aDefaultMachineFolder);
    HRESULT getLoggingLevel(com::Utf8Str &aLoggingLevel);
    HRESULT setLoggingLevel(const com::Utf8Str &aLoggingLevel);
    HRESULT getMediumFormats(std::vector<ComPtr<IMediumFormat> > &aMediumFormats);
    HRESULT getDefaultHardDiskFormat(com::Utf8Str &aDefaultHardDiskFormat);
    HRESULT setDefaultHardDiskFormat(const com::Utf8Str &aDefaultHardDiskFormat);
    HRESULT getFreeDiskSpaceWarning(LONG64 *aFreeDiskSpaceWarning);
    HRESULT setFreeDiskSpaceWarning(LONG64 aFreeDiskSpaceWarning);
    HRESULT getFreeDiskSpacePercentWarning(ULONG *aFreeDiskSpacePercentWarning);
    HRESULT setFreeDiskSpacePercentWarning(ULONG aFreeDiskSpacePercentWarning);
    HRESULT getFreeDiskSpaceError(LONG64 *aFreeDiskSpaceError);
    HRESULT setFreeDiskSpaceError(LONG64 aFreeDiskSpaceError);
    HRESULT getFreeDiskSpacePercentError(ULONG *aFreeDiskSpacePercentError);
    HRESULT setFreeDiskSpacePercentError(ULONG aFreeDiskSpacePercentError);
    HRESULT getVRDEAuthLibrary(com::Utf8Str &aVRDEAuthLibrary);
    HRESULT setVRDEAuthLibrary(const com::Utf8Str &aVRDEAuthLibrary);
    HRESULT getWebServiceAuthLibrary(com::Utf8Str &aWebServiceAuthLibrary);
    HRESULT setWebServiceAuthLibrary(const com::Utf8Str &aWebServiceAuthLibrary);
    HRESULT getDefaultVRDEExtPack(com::Utf8Str &aDefaultVRDEExtPack);
    HRESULT setDefaultVRDEExtPack(const com::Utf8Str &aDefaultVRDEExtPack);
    HRESULT getLogHistoryCount(ULONG *aLogHistoryCount);
    HRESULT setLogHistoryCount(ULONG aLogHistoryCount);
    HRESULT getDefaultAudioDriver(AudioDriverType_T *aDefaultAudioDriver);
    HRESULT getAutostartDatabasePath(com::Utf8Str &aAutostartDatabasePath);
    HRESULT setAutostartDatabasePath(const com::Utf8Str &aAutostartDatabasePath);
    HRESULT getDefaultAdditionsISO(com::Utf8Str &aDefaultAdditionsISO);
    HRESULT setDefaultAdditionsISO(const com::Utf8Str &aDefaultAdditionsISO);
    HRESULT getDefaultFrontend(com::Utf8Str &aDefaultFrontend);
    HRESULT setDefaultFrontend(const com::Utf8Str &aDefaultFrontend);
    HRESULT getScreenShotFormats(std::vector<BitmapFormat_T> &aScreenShotFormats);

    // wrapped ISystemProperties methods
    HRESULT getMaxNetworkAdapters(ChipsetType_T aChipset,
                                  ULONG *aMaxNetworkAdapters);
    HRESULT getMaxNetworkAdaptersOfType(ChipsetType_T aChipset,
                                        NetworkAttachmentType_T aType,
                                        ULONG *aMaxNetworkAdapters);
    HRESULT getMaxDevicesPerPortForStorageBus(StorageBus_T aBus,
                                              ULONG *aMaxDevicesPerPort);
    HRESULT getMinPortCountForStorageBus(StorageBus_T aBus,
                                         ULONG *aMinPortCount);
    HRESULT getMaxPortCountForStorageBus(StorageBus_T aBus,
                                         ULONG *aMaxPortCount);
    HRESULT getMaxInstancesOfStorageBus(ChipsetType_T aChipset,
                                        StorageBus_T aBus,
                                        ULONG *aMaxInstances);
    HRESULT getDeviceTypesForStorageBus(StorageBus_T aBus,
                                        std::vector<DeviceType_T> &aDeviceTypes);
    HRESULT getDefaultIoCacheSettingForStorageController(StorageControllerType_T aControllerType,
                                                         BOOL *aEnabled);
    HRESULT getStorageControllerHotplugCapable(StorageControllerType_T aControllerType,
                                               BOOL *aHotplugCapable);
    HRESULT getMaxInstancesOfUSBControllerType(ChipsetType_T aChipset,
                                               USBControllerType_T aType,
                                               ULONG *aMaxInstances);

    HRESULT i_getUserHomeDirectory(Utf8Str &strPath);
    HRESULT i_setDefaultMachineFolder(const Utf8Str &strPath);
    HRESULT i_setLoggingLevel(const com::Utf8Str &aLoggingLevel);
    HRESULT i_setDefaultHardDiskFormat(const com::Utf8Str &aFormat);
    HRESULT i_setVRDEAuthLibrary(const com::Utf8Str &aPath);

    HRESULT i_setWebServiceAuthLibrary(const com::Utf8Str &aPath);
    HRESULT i_setDefaultVRDEExtPack(const com::Utf8Str &aExtPack);
    HRESULT i_setAutostartDatabasePath(const com::Utf8Str &aPath);
    HRESULT i_setDefaultAdditionsISO(const com::Utf8Str &aPath);
    HRESULT i_setDefaultFrontend(const com::Utf8Str &aDefaultFrontend);

    VirtualBox * const  mParent;

    settings::SystemProperties *m;

    MediumFormatList m_llMediumFormats;

    friend class VirtualBox;
};

#endif // ____H_SYSTEMPROPERTIESIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
