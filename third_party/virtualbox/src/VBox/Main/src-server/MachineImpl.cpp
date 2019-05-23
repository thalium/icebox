/* $Id: MachineImpl.cpp $ */
/** @file
 * Implementation of IMachine in VBoxSVC.
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Make sure all the stdint.h macros are included - must come first! */
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

#include "Logging.h"
#include "VirtualBoxImpl.h"
#include "MachineImpl.h"
#include "ClientToken.h"
#include "ProgressImpl.h"
#include "ProgressProxyImpl.h"
#include "MediumAttachmentImpl.h"
#include "MediumImpl.h"
#include "MediumLock.h"
#include "USBControllerImpl.h"
#include "USBDeviceFiltersImpl.h"
#include "HostImpl.h"
#include "SharedFolderImpl.h"
#include "GuestOSTypeImpl.h"
#include "VirtualBoxErrorInfoImpl.h"
#include "StorageControllerImpl.h"
#include "DisplayImpl.h"
#include "DisplayUtils.h"
#include "MachineImplCloneVM.h"
#include "AutostartDb.h"
#include "SystemPropertiesImpl.h"

// generated header
#include "VBoxEvents.h"

#ifdef VBOX_WITH_USB
# include "USBProxyService.h"
#endif

#include "AutoCaller.h"
#include "HashedPw.h"
#include "Performance.h"

#include <iprt/asm.h>
#include <iprt/path.h>
#include <iprt/dir.h>
#include <iprt/env.h>
#include <iprt/lockvalidator.h>
#include <iprt/process.h>
#include <iprt/cpp/utils.h>
#include <iprt/cpp/xml.h>               /* xml::XmlFileWriter::s_psz*Suff. */
#include <iprt/sha.h>
#include <iprt/string.h>

#include <VBox/com/array.h>
#include <VBox/com/list.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/settings.h>
#include <VBox/vmm/ssm.h>

#ifdef VBOX_WITH_GUEST_PROPS
# include <VBox/HostServices/GuestPropertySvc.h>
# include <VBox/com/array.h>
#endif

#include "VBox/com/MultiResult.h"

#include <algorithm>

#ifdef VBOX_WITH_DTRACE_R3_MAIN
# include "dtrace/VBoxAPI.h"
#endif

#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
# define HOSTSUFF_EXE ".exe"
#else /* !RT_OS_WINDOWS */
# define HOSTSUFF_EXE ""
#endif /* !RT_OS_WINDOWS */

// defines / prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Machine::Data structure
/////////////////////////////////////////////////////////////////////////////

Machine::Data::Data()
{
    mRegistered                = FALSE;
    pMachineConfigFile         = NULL;
    /* Contains hints on what has changed when the user is using the VM (config
     * changes, running the VM, ...). This is used to decide if a config needs
     * to be written to disk. */
    flModifications            = 0;
    /* VM modification usually also trigger setting the current state to
     * "Modified". Although this is not always the case. An e.g. is the VM
     * initialization phase or when snapshot related data is changed. The
     * actually behavior is controlled by the following flag. */
    m_fAllowStateModification  = false;
    mAccessible                = FALSE;
    /* mUuid is initialized in Machine::init() */

    mMachineState              = MachineState_PoweredOff;
    RTTimeNow(&mLastStateChange);

    mMachineStateDeps          = 0;
    mMachineStateDepsSem       = NIL_RTSEMEVENTMULTI;
    mMachineStateChangePending = 0;

    mCurrentStateModified      = TRUE;
    mGuestPropertiesModified   = FALSE;

    mSession.mPID              = NIL_RTPROCESS;
    mSession.mLockType         = LockType_Null;
    mSession.mState            = SessionState_Unlocked;
}

Machine::Data::~Data()
{
    if (mMachineStateDepsSem != NIL_RTSEMEVENTMULTI)
    {
        RTSemEventMultiDestroy(mMachineStateDepsSem);
        mMachineStateDepsSem = NIL_RTSEMEVENTMULTI;
    }
    if (pMachineConfigFile)
    {
        delete pMachineConfigFile;
        pMachineConfigFile = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Machine::HWData structure
/////////////////////////////////////////////////////////////////////////////

Machine::HWData::HWData()
{
    /* default values for a newly created machine */
    mHWVersion = Utf8StrFmt("%d", SchemaDefs::DefaultHardwareVersion);
    mMemorySize = 128;
    mCPUCount = 1;
    mCPUHotPlugEnabled = false;
    mMemoryBalloonSize = 0;
    mPageFusionEnabled = false;
    mGraphicsControllerType = GraphicsControllerType_VBoxVGA;
    mVRAMSize = 8;
    mAccelerate3DEnabled = false;
    mAccelerate2DVideoEnabled = false;
    mMonitorCount = 1;
    mVideoCaptureWidth = 1024;
    mVideoCaptureHeight = 768;
    mVideoCaptureRate = 512;
    mVideoCaptureFPS = 25;
    mVideoCaptureMaxTime = 0;
    mVideoCaptureMaxFileSize = 0;
    mVideoCaptureEnabled = false;
    for (unsigned i = 0; i < RT_ELEMENTS(maVideoCaptureScreens); ++i)
        maVideoCaptureScreens[i] = true;

    mHWVirtExEnabled = true;
    mHWVirtExNestedPagingEnabled = true;
#if HC_ARCH_BITS == 64 && !defined(RT_OS_LINUX)
    mHWVirtExLargePagesEnabled = true;
#else
    /* Not supported on 32 bits hosts. */
    mHWVirtExLargePagesEnabled = false;
#endif
    mHWVirtExVPIDEnabled = true;
    mHWVirtExUXEnabled = true;
    mHWVirtExForceEnabled = false;
#if HC_ARCH_BITS == 64 || defined(RT_OS_WINDOWS) || defined(RT_OS_DARWIN)
    mPAEEnabled = true;
#else
    mPAEEnabled = false;
#endif
    mLongMode =  HC_ARCH_BITS == 64 ? settings::Hardware::LongMode_Enabled : settings::Hardware::LongMode_Disabled;
    mTripleFaultReset = false;
    mAPIC = true;
    mX2APIC = false;
    mIBPBOnVMExit = false;
    mIBPBOnVMEntry = false;
    mSpecCtrl = false;
    mSpecCtrlByHost = false;
    mHPETEnabled = false;
    mCpuExecutionCap = 100; /* Maximum CPU execution cap by default. */
    mCpuIdPortabilityLevel = 0;
    mCpuProfile = "host";

    /* default boot order: floppy - DVD - HDD */
    mBootOrder[0] = DeviceType_Floppy;
    mBootOrder[1] = DeviceType_DVD;
    mBootOrder[2] = DeviceType_HardDisk;
    for (size_t i = 3; i < RT_ELEMENTS(mBootOrder); ++i)
        mBootOrder[i] = DeviceType_Null;

    mClipboardMode = ClipboardMode_Disabled;
    mDnDMode = DnDMode_Disabled;

    mFirmwareType = FirmwareType_BIOS;
    mKeyboardHIDType = KeyboardHIDType_PS2Keyboard;
    mPointingHIDType = PointingHIDType_PS2Mouse;
    mChipsetType = ChipsetType_PIIX3;
    mParavirtProvider = ParavirtProvider_Default;
    mEmulatedUSBCardReaderEnabled = FALSE;

    for (size_t i = 0; i < RT_ELEMENTS(mCPUAttached); ++i)
        mCPUAttached[i] = false;

    mIOCacheEnabled = true;
    mIOCacheSize    = 5; /* 5MB */
}

Machine::HWData::~HWData()
{
}

/////////////////////////////////////////////////////////////////////////////
// Machine class
/////////////////////////////////////////////////////////////////////////////

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

Machine::Machine() :
#ifdef VBOX_WITH_RESOURCE_USAGE_API
    mCollectorGuest(NULL),
#endif
    mPeer(NULL),
    mParent(NULL),
    mSerialPorts(),
    mParallelPorts(),
    uRegistryNeedsSaving(0)
{}

Machine::~Machine()
{}

HRESULT Machine::FinalConstruct()
{
    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void Machine::FinalRelease()
{
    LogFlowThisFunc(("\n"));
    uninit();
    BaseFinalRelease();
}

/**
 *  Initializes a new machine instance; this init() variant creates a new, empty machine.
 *  This gets called from VirtualBox::CreateMachine().
 *
 *  @param aParent      Associated parent object
 *  @param strConfigFile  Local file system path to the VM settings file (can
 *                      be relative to the VirtualBox config directory).
 *  @param strName      name for the machine
 *  @param llGroups     list of groups for the machine
 *  @param aOsType      OS Type of this machine or NULL.
 *  @param aId          UUID for the new machine.
 *  @param fForceOverwrite Whether to overwrite an existing machine settings file.
 *  @param fDirectoryIncludesUUID Whether the use a special VM directory naming
 *                      scheme (includes the UUID).
 *
 *  @return  Success indicator. if not S_OK, the machine object is invalid
 */
HRESULT Machine::init(VirtualBox *aParent,
                      const Utf8Str &strConfigFile,
                      const Utf8Str &strName,
                      const StringsList &llGroups,
                      GuestOSType *aOsType,
                      const Guid &aId,
                      bool fForceOverwrite,
                      bool fDirectoryIncludesUUID)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("(Init_New) aConfigFile='%s'\n", strConfigFile.c_str()));

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    HRESULT rc = initImpl(aParent, strConfigFile);
    if (FAILED(rc)) return rc;

    rc = i_tryCreateMachineConfigFile(fForceOverwrite);
    if (FAILED(rc)) return rc;

    if (SUCCEEDED(rc))
    {
        // create an empty machine config
        mData->pMachineConfigFile = new settings::MachineConfigFile(NULL);

        rc = initDataAndChildObjects();
    }

    if (SUCCEEDED(rc))
    {
        // set to true now to cause uninit() to call uninitDataAndChildObjects() on failure
        mData->mAccessible = TRUE;

        unconst(mData->mUuid) = aId;

        mUserData->s.strName = strName;

        mUserData->s.llGroups = llGroups;

        mUserData->s.fDirectoryIncludesUUID = fDirectoryIncludesUUID;
        // the "name sync" flag determines whether the machine directory gets renamed along
        // with the machine file; say so if the settings file name is the same as the
        // settings file parent directory (machine directory)
        mUserData->s.fNameSync = i_isInOwnDir();

        // initialize the default snapshots folder
        rc = COMSETTER(SnapshotFolder)(NULL);
        AssertComRC(rc);

        if (aOsType)
        {
            /* Store OS type */
            mUserData->s.strOsType = aOsType->i_id();

            /* Apply BIOS defaults */
            mBIOSSettings->i_applyDefaults(aOsType);

            /* Let the OS type select 64-bit ness. */
            mHWData->mLongMode = aOsType->i_is64Bit()
                               ? settings::Hardware::LongMode_Enabled : settings::Hardware::LongMode_Disabled;

            /* Let the OS type enable the X2APIC */
            mHWData->mX2APIC = aOsType->i_recommendedX2APIC();
        }

        /* Apply network adapters defaults */
        for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
            mNetworkAdapters[slot]->i_applyDefaults(aOsType);

        /* Apply serial port defaults */
        for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
            mSerialPorts[slot]->i_applyDefaults(aOsType);

        /* Apply parallel port defaults */
        for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
            mParallelPorts[slot]->i_applyDefaults();

        /* At this point the changing of the current state modification
         * flag is allowed. */
        i_allowStateModification();

        /* commit all changes made during the initialization */
        i_commit();
    }

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
    {
        if (mData->mAccessible)
            autoInitSpan.setSucceeded();
        else
            autoInitSpan.setLimited();
    }

    LogFlowThisFunc(("mName='%s', mRegistered=%RTbool, mAccessible=%RTbool, rc=%08X\n",
                     !!mUserData ? mUserData->s.strName.c_str() : "NULL",
                     mData->mRegistered,
                     mData->mAccessible,
                     rc));

    LogFlowThisFuncLeave();

    return rc;
}

/**
 *  Initializes a new instance with data from machine XML (formerly Init_Registered).
 *  Gets called in two modes:
 *
 *      -- from VirtualBox::initMachines() during VirtualBox startup; in that case, the
 *         UUID is specified and we mark the machine as "registered";
 *
 *      -- from the public VirtualBox::OpenMachine() API, in which case the UUID is NULL
 *         and the machine remains unregistered until RegisterMachine() is called.
 *
 *  @param aParent      Associated parent object
 *  @param strConfigFile Local file system path to the VM settings file (can
 *                      be relative to the VirtualBox config directory).
 *  @param aId          UUID of the machine or NULL (see above).
 *
 *  @return  Success indicator. if not S_OK, the machine object is invalid
 */
HRESULT Machine::initFromSettings(VirtualBox *aParent,
                                  const Utf8Str &strConfigFile,
                                  const Guid *aId)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("(Init_Registered) aConfigFile='%s\n", strConfigFile.c_str()));

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    HRESULT rc = initImpl(aParent, strConfigFile);
    if (FAILED(rc)) return rc;

    if (aId)
    {
        // loading a registered VM:
        unconst(mData->mUuid) = *aId;
        mData->mRegistered = TRUE;
        // now load the settings from XML:
        rc = i_registeredInit();
            // this calls initDataAndChildObjects() and loadSettings()
    }
    else
    {
        // opening an unregistered VM (VirtualBox::OpenMachine()):
        rc = initDataAndChildObjects();

        if (SUCCEEDED(rc))
        {
            // set to true now to cause uninit() to call uninitDataAndChildObjects() on failure
            mData->mAccessible = TRUE;

            try
            {
                // load and parse machine XML; this will throw on XML or logic errors
                mData->pMachineConfigFile = new settings::MachineConfigFile(&mData->m_strConfigFileFull);

                // reject VM UUID duplicates, they can happen if someone
                // tries to register an already known VM config again
                if (aParent->i_findMachine(mData->pMachineConfigFile->uuid,
                                           true /* fPermitInaccessible */,
                                           false /* aDoSetError */,
                                           NULL) != VBOX_E_OBJECT_NOT_FOUND)
                {
                    throw setError(E_FAIL,
                                   tr("Trying to open a VM config '%s' which has the same UUID as an existing virtual machine"),
                                   mData->m_strConfigFile.c_str());
                }

                // use UUID from machine config
                unconst(mData->mUuid) = mData->pMachineConfigFile->uuid;

                rc = i_loadMachineDataFromSettings(*mData->pMachineConfigFile,
                                                 NULL /* puuidRegistry */);
                if (FAILED(rc)) throw rc;

                /* At this point the changing of the current state modification
                 * flag is allowed. */
                i_allowStateModification();

                i_commit();
            }
            catch (HRESULT err)
            {
                /* we assume that error info is set by the thrower */
                rc = err;
            }
            catch (...)
            {
                rc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
            }
        }
    }

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
    {
        if (mData->mAccessible)
            autoInitSpan.setSucceeded();
        else
        {
            autoInitSpan.setLimited();

            // uninit media from this machine's media registry, or else
            // reloading the settings will fail
            mParent->i_unregisterMachineMedia(i_getId());
        }
    }

    LogFlowThisFunc(("mName='%s', mRegistered=%RTbool, mAccessible=%RTbool "
                      "rc=%08X\n",
                      !!mUserData ? mUserData->s.strName.c_str() : "NULL",
                      mData->mRegistered, mData->mAccessible, rc));

    LogFlowThisFuncLeave();

    return rc;
}

/**
 *  Initializes a new instance from a machine config that is already in memory
 *  (import OVF case). Since we are importing, the UUID in the machine
 *  config is ignored and we always generate a fresh one.
 *
 *  @param aParent  Associated parent object.
 *  @param strName  Name for the new machine; this overrides what is specified in config and is used
 *                  for the settings file as well.
 *  @param config   Machine configuration loaded and parsed from XML.
 *
 *  @return  Success indicator. if not S_OK, the machine object is invalid
 */
HRESULT Machine::init(VirtualBox *aParent,
                      const Utf8Str &strName,
                      const settings::MachineConfigFile &config)
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    Utf8Str strConfigFile;
    aParent->i_getDefaultMachineFolder(strConfigFile);
    strConfigFile.append(RTPATH_DELIMITER);
    strConfigFile.append(strName);
    strConfigFile.append(RTPATH_DELIMITER);
    strConfigFile.append(strName);
    strConfigFile.append(".vbox");

    HRESULT rc = initImpl(aParent, strConfigFile);
    if (FAILED(rc)) return rc;

    rc = i_tryCreateMachineConfigFile(false /* fForceOverwrite */);
    if (FAILED(rc)) return rc;

    rc = initDataAndChildObjects();

    if (SUCCEEDED(rc))
    {
        // set to true now to cause uninit() to call uninitDataAndChildObjects() on failure
        mData->mAccessible = TRUE;

        // create empty machine config for instance data
        mData->pMachineConfigFile = new settings::MachineConfigFile(NULL);

        // generate fresh UUID, ignore machine config
        unconst(mData->mUuid).create();

        rc = i_loadMachineDataFromSettings(config,
                                           &mData->mUuid); // puuidRegistry: initialize media with this registry ID

        // override VM name as well, it may be different
        mUserData->s.strName = strName;

        if (SUCCEEDED(rc))
        {
            /* At this point the changing of the current state modification
             * flag is allowed. */
            i_allowStateModification();

            /* commit all changes made during the initialization */
            i_commit();
        }
    }

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
    {
        if (mData->mAccessible)
            autoInitSpan.setSucceeded();
        else
        {
            /* Ignore all errors from unregistering, they would destroy
-            * the more interesting error information we already have,
-            * pinpointing the issue with the VM config. */
            ErrorInfoKeeper eik;

            autoInitSpan.setLimited();

            // uninit media from this machine's media registry, or else
            // reloading the settings will fail
            mParent->i_unregisterMachineMedia(i_getId());
        }
    }

    LogFlowThisFunc(("mName='%s', mRegistered=%RTbool, mAccessible=%RTbool "
                     "rc=%08X\n",
                      !!mUserData ? mUserData->s.strName.c_str() : "NULL",
                      mData->mRegistered, mData->mAccessible, rc));

    LogFlowThisFuncLeave();

    return rc;
}

/**
 * Shared code between the various init() implementations.
 * @param   aParent         The VirtualBox object.
 * @param   strConfigFile   Settings file.
 * @return
 */
HRESULT Machine::initImpl(VirtualBox *aParent,
                          const Utf8Str &strConfigFile)
{
    LogFlowThisFuncEnter();

    AssertReturn(aParent, E_INVALIDARG);
    AssertReturn(!strConfigFile.isEmpty(), E_INVALIDARG);

    HRESULT rc = S_OK;

    /* share the parent weakly */
    unconst(mParent) = aParent;

    /* allocate the essential machine data structure (the rest will be
     * allocated later by initDataAndChildObjects() */
    mData.allocate();

    /* memorize the config file name (as provided) */
    mData->m_strConfigFile = strConfigFile;

    /* get the full file name */
    int vrc1 = mParent->i_calculateFullPath(strConfigFile, mData->m_strConfigFileFull);
    if (RT_FAILURE(vrc1))
        return setError(VBOX_E_FILE_ERROR,
                        tr("Invalid machine settings file name '%s' (%Rrc)"),
                        strConfigFile.c_str(),
                        vrc1);

    LogFlowThisFuncLeave();

    return rc;
}

/**
 * Tries to create a machine settings file in the path stored in the machine
 * instance data. Used when a new machine is created to fail gracefully if
 * the settings file could not be written (e.g. because machine dir is read-only).
 * @return
 */
HRESULT Machine::i_tryCreateMachineConfigFile(bool fForceOverwrite)
{
    HRESULT rc = S_OK;

    // when we create a new machine, we must be able to create the settings file
    RTFILE f = NIL_RTFILE;
    int vrc = RTFileOpen(&f, mData->m_strConfigFileFull.c_str(), RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
    if (    RT_SUCCESS(vrc)
         || vrc == VERR_SHARING_VIOLATION
       )
    {
        if (RT_SUCCESS(vrc))
            RTFileClose(f);
        if (!fForceOverwrite)
            rc = setError(VBOX_E_FILE_ERROR,
                          tr("Machine settings file '%s' already exists"),
                          mData->m_strConfigFileFull.c_str());
        else
        {
            /* try to delete the config file, as otherwise the creation
             * of a new settings file will fail. */
            int vrc2 = RTFileDelete(mData->m_strConfigFileFull.c_str());
            if (RT_FAILURE(vrc2))
                rc = setError(VBOX_E_FILE_ERROR,
                              tr("Could not delete the existing settings file '%s' (%Rrc)"),
                              mData->m_strConfigFileFull.c_str(), vrc2);
        }
    }
    else if (    vrc != VERR_FILE_NOT_FOUND
              && vrc != VERR_PATH_NOT_FOUND
            )
        rc = setError(VBOX_E_FILE_ERROR,
                      tr("Invalid machine settings file name '%s' (%Rrc)"),
                      mData->m_strConfigFileFull.c_str(),
                      vrc);
    return rc;
}

/**
 *  Initializes the registered machine by loading the settings file.
 *  This method is separated from #init() in order to make it possible to
 *  retry the operation after VirtualBox startup instead of refusing to
 *  startup the whole VirtualBox server in case if the settings file of some
 *  registered VM is invalid or inaccessible.
 *
 *  @note Must be always called from this object's write lock
 *        (unless called from #init() that doesn't need any locking).
 *  @note Locks the mUSBController method for writing.
 *  @note Subclasses must not call this method.
 */
HRESULT Machine::i_registeredInit()
{
    AssertReturn(!i_isSessionMachine(), E_FAIL);
    AssertReturn(!i_isSnapshotMachine(), E_FAIL);
    AssertReturn(mData->mUuid.isValid(), E_FAIL);
    AssertReturn(!mData->mAccessible, E_FAIL);

    HRESULT rc = initDataAndChildObjects();

    if (SUCCEEDED(rc))
    {
        /* Temporarily reset the registered flag in order to let setters
         * potentially called from loadSettings() succeed (isMutable() used in
         * all setters will return FALSE for a Machine instance if mRegistered
         * is TRUE). */
        mData->mRegistered = FALSE;

        try
        {
            // load and parse machine XML; this will throw on XML or logic errors
            mData->pMachineConfigFile = new settings::MachineConfigFile(&mData->m_strConfigFileFull);

            if (mData->mUuid != mData->pMachineConfigFile->uuid)
                throw setError(E_FAIL,
                               tr("Machine UUID {%RTuuid} in '%s' doesn't match its UUID {%s} in the registry file '%s'"),
                               mData->pMachineConfigFile->uuid.raw(),
                               mData->m_strConfigFileFull.c_str(),
                               mData->mUuid.toString().c_str(),
                               mParent->i_settingsFilePath().c_str());

            rc = i_loadMachineDataFromSettings(*mData->pMachineConfigFile,
                                               NULL /* const Guid *puuidRegistry */);
            if (FAILED(rc)) throw rc;
        }
        catch (HRESULT err)
        {
            /* we assume that error info is set by the thrower */
            rc = err;
        }
        catch (...)
        {
            rc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
        }

        /* Restore the registered flag (even on failure) */
        mData->mRegistered = TRUE;
    }

    if (SUCCEEDED(rc))
    {
        /* Set mAccessible to TRUE only if we successfully locked and loaded
         * the settings file */
        mData->mAccessible = TRUE;

        /* commit all changes made during loading the settings file */
        i_commit(); /// @todo r=dj why do we need a commit during init?!? this is very expensive
        /// @todo r=klaus for some reason the settings loading logic backs up
        // the settings, and therefore a commit is needed. Should probably be changed.
    }
    else
    {
        /* If the machine is registered, then, instead of returning a
         * failure, we mark it as inaccessible and set the result to
         * success to give it a try later */

        /* fetch the current error info */
        mData->mAccessError = com::ErrorInfo();
        Log1Warning(("Machine {%RTuuid} is inaccessible! [%ls]\n", mData->mUuid.raw(), mData->mAccessError.getText().raw()));

        /* rollback all changes */
        i_rollback(false /* aNotify */);

        // uninit media from this machine's media registry, or else
        // reloading the settings will fail
        mParent->i_unregisterMachineMedia(i_getId());

        /* uninitialize the common part to make sure all data is reset to
         * default (null) values */
        uninitDataAndChildObjects();

        rc = S_OK;
    }

    return rc;
}

/**
 *  Uninitializes the instance.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 *
 *  @note The caller of this method must make sure that this object
 *  a) doesn't have active callers on the current thread and b) is not locked
 *  by the current thread; otherwise uninit() will hang either a) due to
 *  AutoUninitSpan waiting for a number of calls to drop to zero or b) due to
 *  a dead-lock caused by this thread waiting for all callers on the other
 *  threads are done but preventing them from doing so by holding a lock.
 */
void Machine::uninit()
{
    LogFlowThisFuncEnter();

    Assert(!isWriteLockOnCurrentThread());

    Assert(!uRegistryNeedsSaving);
    if (uRegistryNeedsSaving)
    {
        AutoCaller autoCaller(this);
        if (SUCCEEDED(autoCaller.rc()))
        {
            AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
            i_saveSettings(NULL, Machine::SaveS_Force);
        }
    }

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    Assert(!i_isSnapshotMachine());
    Assert(!i_isSessionMachine());
    Assert(!!mData);

    LogFlowThisFunc(("initFailed()=%d\n", autoUninitSpan.initFailed()));
    LogFlowThisFunc(("mRegistered=%d\n", mData->mRegistered));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mData->mSession.mMachine.isNull())
    {
        /* Theoretically, this can only happen if the VirtualBox server has been
         * terminated while there were clients running that owned open direct
         * sessions. Since in this case we are definitely called by
         * VirtualBox::uninit(), we may be sure that SessionMachine::uninit()
         * won't happen on the client watcher thread (because it has a
         * VirtualBox caller for the duration of the
         * SessionMachine::i_checkForDeath() call, so that VirtualBox::uninit()
         * cannot happen until the VirtualBox caller is released). This is
         * important, because SessionMachine::uninit() cannot correctly operate
         * after we return from this method (it expects the Machine instance is
         * still valid). We'll call it ourselves below.
         */
        Log1WarningThisFunc(("Session machine is not NULL (%p), the direct session is still open!\n",
                             (SessionMachine*)mData->mSession.mMachine));

        if (Global::IsOnlineOrTransient(mData->mMachineState))
        {
            Log1WarningThisFunc(("Setting state to Aborted!\n"));
            /* set machine state using SessionMachine reimplementation */
            static_cast<Machine*>(mData->mSession.mMachine)->i_setMachineState(MachineState_Aborted);
        }

        /*
         *  Uninitialize SessionMachine using public uninit() to indicate
         *  an unexpected uninitialization.
         */
        mData->mSession.mMachine->uninit();
        /* SessionMachine::uninit() must set mSession.mMachine to null */
        Assert(mData->mSession.mMachine.isNull());
    }

    // uninit media from this machine's media registry, if they're still there
    Guid uuidMachine(i_getId());

    /* the lock is no more necessary (SessionMachine is uninitialized) */
    alock.release();

    /* XXX This will fail with
     *   "cannot be closed because it is still attached to 1 virtual machines"
     * because at this point we did not call uninitDataAndChildObjects() yet
     * and therefore also removeBackReference() for all these mediums was not called! */

    if (uuidMachine.isValid() && !uuidMachine.isZero())     // can be empty if we're called from a failure of Machine::init
        mParent->i_unregisterMachineMedia(uuidMachine);

    // has machine been modified?
    if (mData->flModifications)
    {
        Log1WarningThisFunc(("Discarding unsaved settings changes!\n"));
        i_rollback(false /* aNotify */);
    }

    if (mData->mAccessible)
        uninitDataAndChildObjects();

    /* free the essential data structure last */
    mData.free();

    LogFlowThisFuncLeave();
}

// Wrapped IMachine properties
/////////////////////////////////////////////////////////////////////////////
HRESULT Machine::getParent(ComPtr<IVirtualBox> &aParent)
{
    /* mParent is constant during life time, no need to lock */
    ComObjPtr<VirtualBox> pVirtualBox(mParent);
    aParent = pVirtualBox;

    return S_OK;
}


HRESULT Machine::getAccessible(BOOL *aAccessible)
{
    /* In some cases (medium registry related), it is necessary to be able to
     * go through the list of all machines. Happens when an inaccessible VM
     * has a sensible medium registry. */
    AutoReadLock mllock(mParent->i_getMachinesListLockHandle() COMMA_LOCKVAL_SRC_POS);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    if (!mData->mAccessible)
    {
        /* try to initialize the VM once more if not accessible */

        AutoReinitSpan autoReinitSpan(this);
        AssertReturn(autoReinitSpan.isOk(), E_FAIL);

#ifdef DEBUG
        LogFlowThisFunc(("Dumping media backreferences\n"));
        mParent->i_dumpAllBackRefs();
#endif

        if (mData->pMachineConfigFile)
        {
            // reset the XML file to force loadSettings() (called from i_registeredInit())
            // to parse it again; the file might have changed
            delete mData->pMachineConfigFile;
            mData->pMachineConfigFile = NULL;
        }

        rc = i_registeredInit();

        if (SUCCEEDED(rc) && mData->mAccessible)
        {
            autoReinitSpan.setSucceeded();

            /* make sure interesting parties will notice the accessibility
             * state change */
            mParent->i_onMachineStateChange(mData->mUuid, mData->mMachineState);
            mParent->i_onMachineDataChange(mData->mUuid);
        }
    }

    if (SUCCEEDED(rc))
        *aAccessible = mData->mAccessible;

    LogFlowThisFuncLeave();

    return rc;
}

HRESULT Machine::getAccessError(ComPtr<IVirtualBoxErrorInfo> &aAccessError)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData->mAccessible || !mData->mAccessError.isBasicAvailable())
    {
        /* return shortly */
        aAccessError = NULL;
        return S_OK;
    }

    HRESULT rc = S_OK;

    ComObjPtr<VirtualBoxErrorInfo> errorInfo;
    rc = errorInfo.createObject();
    if (SUCCEEDED(rc))
    {
        errorInfo->init(mData->mAccessError.getResultCode(),
                        mData->mAccessError.getInterfaceID().ref(),
                        Utf8Str(mData->mAccessError.getComponent()).c_str(),
                        Utf8Str(mData->mAccessError.getText()));
        aAccessError = errorInfo;
    }

    return rc;
}

HRESULT Machine::getName(com::Utf8Str &aName)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aName = mUserData->s.strName;

    return S_OK;
}

HRESULT Machine::setName(const com::Utf8Str &aName)
{
    // prohibit setting a UUID only as the machine name, or else it can
    // never be found by findMachine()
    Guid test(aName);

    if (test.isValid())
        return setError(E_INVALIDARG, tr("A machine cannot have a UUID as its name"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.strName = aName;

    return S_OK;
}

HRESULT Machine::getDescription(com::Utf8Str &aDescription)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aDescription = mUserData->s.strDescription;

    return S_OK;
}

HRESULT Machine::setDescription(const com::Utf8Str &aDescription)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    // this can be done in principle in any state as it doesn't affect the VM
    // significantly, but play safe by not messing around while complex
    // activities are going on
    HRESULT rc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.strDescription = aDescription;

    return S_OK;
}

HRESULT Machine::getId(com::Guid &aId)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aId = mData->mUuid;

    return S_OK;
}

HRESULT Machine::getGroups(std::vector<com::Utf8Str> &aGroups)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aGroups.resize(mUserData->s.llGroups.size());
    size_t i = 0;
    for (StringsList::const_iterator
         it = mUserData->s.llGroups.begin();
         it != mUserData->s.llGroups.end();
         ++it, ++i)
        aGroups[i] = (*it);

    return S_OK;
}

HRESULT Machine::setGroups(const std::vector<com::Utf8Str> &aGroups)
{
    StringsList llGroups;
    HRESULT rc = mParent->i_convertMachineGroups(aGroups, &llGroups);
    if (FAILED(rc))
        return rc;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.llGroups = llGroups;

    return S_OK;
}

HRESULT Machine::getOSTypeId(com::Utf8Str &aOSTypeId)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aOSTypeId = mUserData->s.strOsType;

    return S_OK;
}

HRESULT Machine::setOSTypeId(const com::Utf8Str &aOSTypeId)
{
    /* look up the object by Id to check it is valid */
    ComObjPtr<GuestOSType> pGuestOSType;
    HRESULT rc = mParent->i_findGuestOSType(aOSTypeId,
                                            pGuestOSType);
    if (FAILED(rc)) return rc;

    /* when setting, always use the "etalon" value for consistency -- lookup
     * by ID is case-insensitive and the input value may have different case */
    Utf8Str osTypeId = pGuestOSType->i_id();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.strOsType = osTypeId;

    return S_OK;
}

HRESULT Machine::getFirmwareType(FirmwareType_T *aFirmwareType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aFirmwareType = mHWData->mFirmwareType;

    return S_OK;
}

HRESULT Machine::setFirmwareType(FirmwareType_T aFirmwareType)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mFirmwareType = aFirmwareType;

    return S_OK;
}

HRESULT Machine::getKeyboardHIDType(KeyboardHIDType_T *aKeyboardHIDType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aKeyboardHIDType = mHWData->mKeyboardHIDType;

    return S_OK;
}

HRESULT Machine::setKeyboardHIDType(KeyboardHIDType_T aKeyboardHIDType)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mKeyboardHIDType = aKeyboardHIDType;

    return S_OK;
}

HRESULT Machine::getPointingHIDType(PointingHIDType_T *aPointingHIDType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aPointingHIDType = mHWData->mPointingHIDType;

    return S_OK;
}

HRESULT Machine::setPointingHIDType(PointingHIDType_T aPointingHIDType)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mPointingHIDType = aPointingHIDType;

    return S_OK;
}

HRESULT Machine::getChipsetType(ChipsetType_T *aChipsetType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aChipsetType = mHWData->mChipsetType;

    return S_OK;
}

HRESULT Machine::setChipsetType(ChipsetType_T aChipsetType)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (aChipsetType != mHWData->mChipsetType)
    {
        i_setModified(IsModified_MachineData);
        mHWData.backup();
        mHWData->mChipsetType = aChipsetType;

        // Resize network adapter array, to be finalized on commit/rollback.
        // We must not throw away entries yet, otherwise settings are lost
        // without a way to roll back.
        size_t newCount = Global::getMaxNetworkAdapters(aChipsetType);
        size_t oldCount = mNetworkAdapters.size();
        if (newCount > oldCount)
        {
            mNetworkAdapters.resize(newCount);
            for (size_t slot = oldCount; slot < mNetworkAdapters.size(); slot++)
            {
                unconst(mNetworkAdapters[slot]).createObject();
                mNetworkAdapters[slot]->init(this, (ULONG)slot);
            }
        }
    }

    return S_OK;
}

HRESULT Machine::getParavirtDebug(com::Utf8Str &aParavirtDebug)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aParavirtDebug = mHWData->mParavirtDebug;
    return S_OK;
}

HRESULT Machine::setParavirtDebug(const com::Utf8Str &aParavirtDebug)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /** @todo Parse/validate options? */
    if (aParavirtDebug != mHWData->mParavirtDebug)
    {
        i_setModified(IsModified_MachineData);
        mHWData.backup();
        mHWData->mParavirtDebug = aParavirtDebug;
    }

    return S_OK;
}

HRESULT Machine::getParavirtProvider(ParavirtProvider_T *aParavirtProvider)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aParavirtProvider = mHWData->mParavirtProvider;

    return S_OK;
}

HRESULT Machine::setParavirtProvider(ParavirtProvider_T aParavirtProvider)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (aParavirtProvider != mHWData->mParavirtProvider)
    {
        i_setModified(IsModified_MachineData);
        mHWData.backup();
        mHWData->mParavirtProvider = aParavirtProvider;
    }

    return S_OK;
}

HRESULT Machine::getEffectiveParavirtProvider(ParavirtProvider_T *aParavirtProvider)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aParavirtProvider = mHWData->mParavirtProvider;
    switch (mHWData->mParavirtProvider)
    {
        case ParavirtProvider_None:
        case ParavirtProvider_HyperV:
        case ParavirtProvider_KVM:
        case ParavirtProvider_Minimal:
            break;

        /* Resolve dynamic provider types to the effective types. */
        default:
        {
            ComObjPtr<GuestOSType> pGuestOSType;
            HRESULT hrc2 = mParent->i_findGuestOSType(mUserData->s.strOsType,
                                                      pGuestOSType);
            AssertMsgReturn(SUCCEEDED(hrc2), ("Failed to get guest OS type. hrc2=%Rhrc\n", hrc2), hrc2);

            Utf8Str guestTypeFamilyId = pGuestOSType->i_familyId();
            bool fOsXGuest = guestTypeFamilyId == "MacOS";

            switch (mHWData->mParavirtProvider)
            {
                case ParavirtProvider_Legacy:
                {
                    if (fOsXGuest)
                        *aParavirtProvider = ParavirtProvider_Minimal;
                    else
                        *aParavirtProvider = ParavirtProvider_None;
                    break;
                }

                case ParavirtProvider_Default:
                {
                    if (fOsXGuest)
                        *aParavirtProvider = ParavirtProvider_Minimal;
                    else if (   mUserData->s.strOsType == "Windows10"
                             || mUserData->s.strOsType == "Windows10_64"
                             || mUserData->s.strOsType == "Windows81"
                             || mUserData->s.strOsType == "Windows81_64"
                             || mUserData->s.strOsType == "Windows8"
                             || mUserData->s.strOsType == "Windows8_64"
                             || mUserData->s.strOsType == "Windows7"
                             || mUserData->s.strOsType == "Windows7_64"
                             || mUserData->s.strOsType == "WindowsVista"
                             || mUserData->s.strOsType == "WindowsVista_64"
                             || mUserData->s.strOsType == "Windows2012"
                             || mUserData->s.strOsType == "Windows2012_64"
                             || mUserData->s.strOsType == "Windows2008"
                             || mUserData->s.strOsType == "Windows2008_64")
                    {
                        *aParavirtProvider = ParavirtProvider_HyperV;
                    }
                    else if (   mUserData->s.strOsType == "Linux26"      // Linux22 and Linux24 omitted as they're too old
                             || mUserData->s.strOsType == "Linux26_64"   // for having any KVM paravirtualization support.
                             || mUserData->s.strOsType == "Linux"
                             || mUserData->s.strOsType == "Linux_64"
                             || mUserData->s.strOsType == "ArchLinux"
                             || mUserData->s.strOsType == "ArchLinux_64"
                             || mUserData->s.strOsType == "Debian"
                             || mUserData->s.strOsType == "Debian_64"
                             || mUserData->s.strOsType == "Fedora"
                             || mUserData->s.strOsType == "Fedora_64"
                             || mUserData->s.strOsType == "Gentoo"
                             || mUserData->s.strOsType == "Gentoo_64"
                             || mUserData->s.strOsType == "Mandriva"
                             || mUserData->s.strOsType == "Mandriva_64"
                             || mUserData->s.strOsType == "OpenSUSE"
                             || mUserData->s.strOsType == "OpenSUSE_64"
                             || mUserData->s.strOsType == "Oracle"
                             || mUserData->s.strOsType == "Oracle_64"
                             || mUserData->s.strOsType == "RedHat"
                             || mUserData->s.strOsType == "RedHat_64"
                             || mUserData->s.strOsType == "Turbolinux"
                             || mUserData->s.strOsType == "Turbolinux_64"
                             || mUserData->s.strOsType == "Ubuntu"
                             || mUserData->s.strOsType == "Ubuntu_64"
                             || mUserData->s.strOsType == "Xandros"
                             || mUserData->s.strOsType == "Xandros_64")
                    {
                        *aParavirtProvider = ParavirtProvider_KVM;
                    }
                    else
                        *aParavirtProvider = ParavirtProvider_None;
                    break;
                }

                default: AssertFailedBreak(); /* Shut up MSC. */
            }
            break;
        }
    }

    Assert(   *aParavirtProvider == ParavirtProvider_None
           || *aParavirtProvider == ParavirtProvider_Minimal
           || *aParavirtProvider == ParavirtProvider_HyperV
           || *aParavirtProvider == ParavirtProvider_KVM);
    return S_OK;
}

HRESULT Machine::getHardwareVersion(com::Utf8Str &aHardwareVersion)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aHardwareVersion = mHWData->mHWVersion;

    return S_OK;
}

HRESULT Machine::setHardwareVersion(const com::Utf8Str &aHardwareVersion)
{
    /* check known version */
    Utf8Str hwVersion = aHardwareVersion;
    if (    hwVersion.compare("1") != 0
        &&  hwVersion.compare("2") != 0)    // VBox 2.1.x and later (VMMDev heap)
        return setError(E_INVALIDARG,
                        tr("Invalid hardware version: %s\n"), aHardwareVersion.c_str());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mHWVersion = aHardwareVersion;

    return S_OK;
}

HRESULT Machine::getHardwareUUID(com::Guid &aHardwareUUID)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mHWData->mHardwareUUID.isZero())
        aHardwareUUID = mHWData->mHardwareUUID;
    else
        aHardwareUUID = mData->mUuid;

    return S_OK;
}

HRESULT Machine::setHardwareUUID(const com::Guid &aHardwareUUID)
{
    if (!aHardwareUUID.isValid())
        return E_INVALIDARG;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    if (aHardwareUUID == mData->mUuid)
        mHWData->mHardwareUUID.clear();
    else
        mHWData->mHardwareUUID = aHardwareUUID;

    return S_OK;
}

HRESULT Machine::getMemorySize(ULONG *aMemorySize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aMemorySize = mHWData->mMemorySize;

    return S_OK;
}

HRESULT Machine::setMemorySize(ULONG aMemorySize)
{
    /* check RAM limits */
    if (    aMemorySize < MM_RAM_MIN_IN_MB
         || aMemorySize > MM_RAM_MAX_IN_MB
       )
        return setError(E_INVALIDARG,
                        tr("Invalid RAM size: %lu MB (must be in range [%lu, %lu] MB)"),
                        aMemorySize, MM_RAM_MIN_IN_MB, MM_RAM_MAX_IN_MB);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mMemorySize = aMemorySize;

    return S_OK;
}

HRESULT Machine::getCPUCount(ULONG *aCPUCount)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCPUCount = mHWData->mCPUCount;

    return S_OK;
}

HRESULT Machine::setCPUCount(ULONG aCPUCount)
{
    /* check CPU limits */
    if (    aCPUCount < SchemaDefs::MinCPUCount
         || aCPUCount > SchemaDefs::MaxCPUCount
       )
        return setError(E_INVALIDARG,
                        tr("Invalid virtual CPU count: %lu (must be in range [%lu, %lu])"),
                        aCPUCount, SchemaDefs::MinCPUCount, SchemaDefs::MaxCPUCount);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* We cant go below the current number of CPUs attached if hotplug is enabled*/
    if (mHWData->mCPUHotPlugEnabled)
    {
        for (unsigned idx = aCPUCount; idx < SchemaDefs::MaxCPUCount; idx++)
        {
            if (mHWData->mCPUAttached[idx])
                return setError(E_INVALIDARG,
                                tr("There is still a CPU attached to socket %lu."
                                   "Detach the CPU before removing the socket"),
                                aCPUCount, idx+1);
        }
    }

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mCPUCount = aCPUCount;

    return S_OK;
}

HRESULT Machine::getCPUExecutionCap(ULONG *aCPUExecutionCap)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCPUExecutionCap = mHWData->mCpuExecutionCap;

    return S_OK;
}

HRESULT Machine::setCPUExecutionCap(ULONG aCPUExecutionCap)
{
    HRESULT rc = S_OK;

    /* check throttle limits */
    if (    aCPUExecutionCap < 1
         || aCPUExecutionCap > 100
       )
        return setError(E_INVALIDARG,
                        tr("Invalid CPU execution cap value: %lu (must be in range [%lu, %lu])"),
                        aCPUExecutionCap, 1, 100);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    alock.release();
    rc = i_onCPUExecutionCapChange(aCPUExecutionCap);
    alock.acquire();
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mCpuExecutionCap = aCPUExecutionCap;

    /** Save settings if online - @todo why is this required? -- @bugref{6818} */
    if (Global::IsOnline(mData->mMachineState))
        i_saveSettings(NULL);

    return S_OK;
}

HRESULT Machine::getCPUHotPlugEnabled(BOOL *aCPUHotPlugEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCPUHotPlugEnabled = mHWData->mCPUHotPlugEnabled;

    return S_OK;
}

HRESULT Machine::setCPUHotPlugEnabled(BOOL aCPUHotPlugEnabled)
{
    HRESULT rc = S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (mHWData->mCPUHotPlugEnabled != aCPUHotPlugEnabled)
    {
        if (aCPUHotPlugEnabled)
        {
            i_setModified(IsModified_MachineData);
            mHWData.backup();

            /* Add the amount of CPUs currently attached */
            for (unsigned i = 0; i < mHWData->mCPUCount; ++i)
                mHWData->mCPUAttached[i] = true;
        }
        else
        {
            /*
             * We can disable hotplug only if the amount of maximum CPUs is equal
             * to the amount of attached CPUs
             */
            unsigned cCpusAttached = 0;
            unsigned iHighestId = 0;

            for (unsigned i = 0; i < SchemaDefs::MaxCPUCount; ++i)
            {
                if (mHWData->mCPUAttached[i])
                {
                    cCpusAttached++;
                    iHighestId = i;
                }
            }

            if (   (cCpusAttached != mHWData->mCPUCount)
                || (iHighestId >= mHWData->mCPUCount))
                return setError(E_INVALIDARG,
                                tr("CPU hotplugging can't be disabled because the maximum number of CPUs is not equal to the amount of CPUs attached"));

            i_setModified(IsModified_MachineData);
            mHWData.backup();
        }
    }

    mHWData->mCPUHotPlugEnabled = aCPUHotPlugEnabled;

    return rc;
}

HRESULT Machine::getCPUIDPortabilityLevel(ULONG *aCPUIDPortabilityLevel)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCPUIDPortabilityLevel = mHWData->mCpuIdPortabilityLevel;

    return S_OK;
}

HRESULT Machine::setCPUIDPortabilityLevel(ULONG aCPUIDPortabilityLevel)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT hrc = i_checkStateDependency(MutableStateDep);
    if (SUCCEEDED(hrc))
    {
        i_setModified(IsModified_MachineData);
        mHWData.backup();
        mHWData->mCpuIdPortabilityLevel = aCPUIDPortabilityLevel;
    }
    return hrc;
}

HRESULT Machine::getCPUProfile(com::Utf8Str &aCPUProfile)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aCPUProfile = mHWData->mCpuProfile;
    return S_OK;
}

HRESULT Machine::setCPUProfile(const com::Utf8Str &aCPUProfile)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableStateDep);
    if (SUCCEEDED(hrc))
    {
        i_setModified(IsModified_MachineData);
        mHWData.backup();
        /* Empty equals 'host'. */
        if (aCPUProfile.isNotEmpty())
            mHWData->mCpuProfile = aCPUProfile;
        else
            mHWData->mCpuProfile = "host";
    }
    return hrc;
}

HRESULT Machine::getEmulatedUSBCardReaderEnabled(BOOL *aEmulatedUSBCardReaderEnabled)
{
#ifdef VBOX_WITH_USB_CARDREADER
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aEmulatedUSBCardReaderEnabled = mHWData->mEmulatedUSBCardReaderEnabled;

    return S_OK;
#else
    NOREF(aEmulatedUSBCardReaderEnabled);
    return E_NOTIMPL;
#endif
}

HRESULT Machine::setEmulatedUSBCardReaderEnabled(BOOL aEmulatedUSBCardReaderEnabled)
{
#ifdef VBOX_WITH_USB_CARDREADER
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mEmulatedUSBCardReaderEnabled = aEmulatedUSBCardReaderEnabled;

    return S_OK;
#else
    NOREF(aEmulatedUSBCardReaderEnabled);
    return E_NOTIMPL;
#endif
}

HRESULT Machine::getHPETEnabled(BOOL *aHPETEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aHPETEnabled = mHWData->mHPETEnabled;

    return S_OK;
}

HRESULT Machine::setHPETEnabled(BOOL aHPETEnabled)
{
    HRESULT rc = S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();

    mHWData->mHPETEnabled = aHPETEnabled;

    return rc;
}

HRESULT Machine::getVideoCaptureEnabled(BOOL *aVideoCaptureEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aVideoCaptureEnabled = mHWData->mVideoCaptureEnabled;
    return S_OK;
}

HRESULT Machine::setVideoCaptureEnabled(BOOL aVideoCaptureEnabled)
{
    HRESULT rc = S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureEnabled = aVideoCaptureEnabled;

    alock.release();
    rc = i_onVideoCaptureChange();
    alock.acquire();
    if (FAILED(rc))
    {
        /*
         * Normally we would do the actual change _after_ i_onVideoCaptureChange() succeeded.
         * We cannot do this because that function uses Machine::GetVideoCaptureEnabled to
         * determine if it should start or stop capturing. Therefore we need to manually
         * undo change.
         */
        mHWData->mVideoCaptureEnabled = mHWData.backedUpData()->mVideoCaptureEnabled;
        return rc;
    }

    /** Save settings if online - @todo why is this required? -- @bugref{6818} */
    if (Global::IsOnline(mData->mMachineState))
        i_saveSettings(NULL);

    return rc;
}

HRESULT Machine::getVideoCaptureScreens(std::vector<BOOL> &aVideoCaptureScreens)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aVideoCaptureScreens.resize(mHWData->mMonitorCount);
    for (unsigned i = 0; i < mHWData->mMonitorCount; ++i)
        aVideoCaptureScreens[i] = mHWData->maVideoCaptureScreens[i];
    return S_OK;
}

HRESULT Machine::setVideoCaptureScreens(const std::vector<BOOL> &aVideoCaptureScreens)
{
    AssertReturn(aVideoCaptureScreens.size() <= RT_ELEMENTS(mHWData->maVideoCaptureScreens), E_INVALIDARG);
    bool fChanged = false;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (unsigned i = 0; i < aVideoCaptureScreens.size(); ++i)
    {
        if (mHWData->maVideoCaptureScreens[i] != RT_BOOL(aVideoCaptureScreens[i]))
        {
            mHWData->maVideoCaptureScreens[i] = RT_BOOL(aVideoCaptureScreens[i]);
            fChanged = true;
        }
    }
    if (fChanged)
    {
        alock.release();
        HRESULT rc = i_onVideoCaptureChange();
        alock.acquire();
        if (FAILED(rc)) return rc;
        i_setModified(IsModified_MachineData);

        /** Save settings if online - @todo why is this required? -- @bugref{6818} */
        if (Global::IsOnline(mData->mMachineState))
            i_saveSettings(NULL);
    }

    return S_OK;
}

HRESULT Machine::getVideoCaptureFile(com::Utf8Str &aVideoCaptureFile)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (mHWData->mVideoCaptureFile.isEmpty())
        i_getDefaultVideoCaptureFile(aVideoCaptureFile);
    else
        aVideoCaptureFile = mHWData->mVideoCaptureFile;
    return S_OK;
}

HRESULT Machine::setVideoCaptureFile(const com::Utf8Str &aVideoCaptureFile)
{
    Utf8Str strFile(aVideoCaptureFile);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    if (!RTPathStartsWithRoot(strFile.c_str()))
        return setError(E_INVALIDARG, tr("Video capture file name '%s' is not absolute"), strFile.c_str());

    if (!strFile.isEmpty())
    {
        Utf8Str defaultFile;
        i_getDefaultVideoCaptureFile(defaultFile);
        if (!RTPathCompare(strFile.c_str(), defaultFile.c_str()))
            strFile.setNull();
    }

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureFile = strFile;

    return S_OK;
}

HRESULT Machine::getVideoCaptureWidth(ULONG *aVideoCaptureWidth)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aVideoCaptureWidth = mHWData->mVideoCaptureWidth;
    return S_OK;
}

HRESULT Machine::setVideoCaptureWidth(ULONG aVideoCaptureWidth)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureWidth = aVideoCaptureWidth;

    return S_OK;
}

HRESULT Machine::getVideoCaptureHeight(ULONG *aVideoCaptureHeight)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aVideoCaptureHeight = mHWData->mVideoCaptureHeight;
    return S_OK;
}

HRESULT Machine::setVideoCaptureHeight(ULONG aVideoCaptureHeight)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureHeight = aVideoCaptureHeight;

    return S_OK;
}

HRESULT Machine::getVideoCaptureRate(ULONG *aVideoCaptureRate)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aVideoCaptureRate = mHWData->mVideoCaptureRate;
    return S_OK;
}

HRESULT Machine::setVideoCaptureRate(ULONG aVideoCaptureRate)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureRate = aVideoCaptureRate;

    return S_OK;
}

HRESULT Machine::getVideoCaptureFPS(ULONG *aVideoCaptureFPS)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aVideoCaptureFPS = mHWData->mVideoCaptureFPS;
    return S_OK;
}

HRESULT Machine::setVideoCaptureFPS(ULONG aVideoCaptureFPS)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureFPS = aVideoCaptureFPS;

    return S_OK;
}

HRESULT Machine::getVideoCaptureMaxTime(ULONG *aVideoCaptureMaxTime)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aVideoCaptureMaxTime = mHWData->mVideoCaptureMaxTime;
    return S_OK;
}

HRESULT Machine::setVideoCaptureMaxTime(ULONG aVideoCaptureMaxTime)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureMaxTime = aVideoCaptureMaxTime;

    return S_OK;
}

HRESULT Machine::getVideoCaptureMaxFileSize(ULONG *aVideoCaptureMaxFileSize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aVideoCaptureMaxFileSize = mHWData->mVideoCaptureMaxFileSize;
    return S_OK;
}

HRESULT Machine::setVideoCaptureMaxFileSize(ULONG aVideoCaptureMaxFileSize)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureMaxFileSize = aVideoCaptureMaxFileSize;

    return S_OK;
}

HRESULT Machine::getVideoCaptureOptions(com::Utf8Str &aVideoCaptureOptions)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aVideoCaptureOptions = mHWData->mVideoCaptureOptions;
    return S_OK;
}

HRESULT Machine::setVideoCaptureOptions(const com::Utf8Str &aVideoCaptureOptions)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   Global::IsOnline(mData->mMachineState)
        && mHWData->mVideoCaptureEnabled)
        return setError(E_INVALIDARG, tr("Cannot change parameters while capturing is enabled"));

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVideoCaptureOptions = aVideoCaptureOptions;

    return S_OK;
}

HRESULT Machine::getGraphicsControllerType(GraphicsControllerType_T *aGraphicsControllerType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aGraphicsControllerType = mHWData->mGraphicsControllerType;

    return S_OK;
}

HRESULT Machine::setGraphicsControllerType(GraphicsControllerType_T aGraphicsControllerType)
{
    switch (aGraphicsControllerType)
    {
        case GraphicsControllerType_Null:
        case GraphicsControllerType_VBoxVGA:
#ifdef VBOX_WITH_VMSVGA
        case GraphicsControllerType_VMSVGA:
#endif
            break;
        default:
            return setError(E_INVALIDARG, tr("The graphics controller type (%d) is invalid"), aGraphicsControllerType);
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mGraphicsControllerType = aGraphicsControllerType;

    return S_OK;
}

HRESULT Machine::getVRAMSize(ULONG *aVRAMSize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aVRAMSize = mHWData->mVRAMSize;

    return S_OK;
}

HRESULT Machine::setVRAMSize(ULONG aVRAMSize)
{
    /* check VRAM limits */
    if (aVRAMSize > SchemaDefs::MaxGuestVRAM)
        return setError(E_INVALIDARG,
                        tr("Invalid VRAM size: %lu MB (must be in range [%lu, %lu] MB)"),
                        aVRAMSize, SchemaDefs::MinGuestVRAM, SchemaDefs::MaxGuestVRAM);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mVRAMSize = aVRAMSize;

    return S_OK;
}

/** @todo this method should not be public */
HRESULT Machine::getMemoryBalloonSize(ULONG *aMemoryBalloonSize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aMemoryBalloonSize = mHWData->mMemoryBalloonSize;

    return S_OK;
}

/**
 * Set the memory balloon size.
 *
 * This method is also called from IGuest::COMSETTER(MemoryBalloonSize) so
 * we have to make sure that we never call IGuest from here.
 */
HRESULT Machine::setMemoryBalloonSize(ULONG aMemoryBalloonSize)
{
    /* This must match GMMR0Init; currently we only support memory ballooning on all 64-bit hosts except Mac OS X */
#if HC_ARCH_BITS == 64 && (defined(RT_OS_WINDOWS) || defined(RT_OS_SOLARIS) || defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD))
    /* check limits */
    if (aMemoryBalloonSize >= VMMDEV_MAX_MEMORY_BALLOON(mHWData->mMemorySize))
        return setError(E_INVALIDARG,
                        tr("Invalid memory balloon size: %lu MB (must be in range [%lu, %lu] MB)"),
                        aMemoryBalloonSize, 0, VMMDEV_MAX_MEMORY_BALLOON(mHWData->mMemorySize));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mMemoryBalloonSize = aMemoryBalloonSize;

    return S_OK;
#else
    NOREF(aMemoryBalloonSize);
    return setError(E_NOTIMPL, tr("Memory ballooning is only supported on 64-bit hosts"));
#endif
}

HRESULT Machine::getPageFusionEnabled(BOOL *aPageFusionEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aPageFusionEnabled = mHWData->mPageFusionEnabled;
    return S_OK;
}

HRESULT Machine::setPageFusionEnabled(BOOL aPageFusionEnabled)
{
#ifdef VBOX_WITH_PAGE_SHARING
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo must support changes for running vms and keep this in sync with IGuest. */
    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mPageFusionEnabled = aPageFusionEnabled;
    return S_OK;
#else
    NOREF(aPageFusionEnabled);
    return setError(E_NOTIMPL, tr("Page fusion is only supported on 64-bit hosts"));
#endif
}

HRESULT Machine::getAccelerate3DEnabled(BOOL *aAccelerate3DEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAccelerate3DEnabled = mHWData->mAccelerate3DEnabled;

    return S_OK;
}

HRESULT Machine::setAccelerate3DEnabled(BOOL aAccelerate3DEnabled)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /** @todo check validity! */

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mAccelerate3DEnabled = aAccelerate3DEnabled;

    return S_OK;
}


HRESULT Machine::getAccelerate2DVideoEnabled(BOOL *aAccelerate2DVideoEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAccelerate2DVideoEnabled = mHWData->mAccelerate2DVideoEnabled;

    return S_OK;
}

HRESULT Machine::setAccelerate2DVideoEnabled(BOOL aAccelerate2DVideoEnabled)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /** @todo check validity! */
    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mAccelerate2DVideoEnabled = aAccelerate2DVideoEnabled;

    return S_OK;
}

HRESULT Machine::getMonitorCount(ULONG *aMonitorCount)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aMonitorCount = mHWData->mMonitorCount;

    return S_OK;
}

HRESULT Machine::setMonitorCount(ULONG aMonitorCount)
{
    /* make sure monitor count is a sensible number */
    if (aMonitorCount < 1 || aMonitorCount > SchemaDefs::MaxGuestMonitors)
        return setError(E_INVALIDARG,
                        tr("Invalid monitor count: %lu (must be in range [%lu, %lu])"),
                        aMonitorCount, 1, SchemaDefs::MaxGuestMonitors);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mMonitorCount = aMonitorCount;

    return S_OK;
}

HRESULT Machine::getBIOSSettings(ComPtr<IBIOSSettings> &aBIOSSettings)
{
    /* mBIOSSettings is constant during life time, no need to lock */
    aBIOSSettings = mBIOSSettings;

    return S_OK;
}

HRESULT Machine::getCPUProperty(CPUPropertyType_T aProperty, BOOL *aValue)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    switch (aProperty)
    {
        case CPUPropertyType_PAE:
            *aValue = mHWData->mPAEEnabled;
            break;

        case CPUPropertyType_LongMode:
            if (mHWData->mLongMode == settings::Hardware::LongMode_Enabled)
                *aValue = TRUE;
            else if (mHWData->mLongMode == settings::Hardware::LongMode_Disabled)
                *aValue = FALSE;
#if HC_ARCH_BITS == 64
            else
                *aValue = TRUE;
#else
            else
            {
                *aValue = FALSE;

                ComObjPtr<GuestOSType> pGuestOSType;
                HRESULT hrc2 = mParent->i_findGuestOSType(mUserData->s.strOsType,
                                                          pGuestOSType);
                if (SUCCEEDED(hrc2))
                {
                    if (pGuestOSType->i_is64Bit())
                    {
                        ComObjPtr<Host> pHost = mParent->i_host();
                        alock.release();

                        hrc2 = pHost->GetProcessorFeature(ProcessorFeature_LongMode, aValue); AssertComRC(hrc2);
                        if (FAILED(hrc2))
                            *aValue = FALSE;
                    }
                }
            }
#endif
            break;

        case CPUPropertyType_TripleFaultReset:
            *aValue = mHWData->mTripleFaultReset;
            break;

        case CPUPropertyType_APIC:
            *aValue = mHWData->mAPIC;
            break;

        case CPUPropertyType_X2APIC:
            *aValue = mHWData->mX2APIC;
            break;

        case CPUPropertyType_IBPBOnVMExit:
            *aValue = mHWData->mIBPBOnVMExit;
            break;

        case CPUPropertyType_IBPBOnVMEntry:
            *aValue = mHWData->mIBPBOnVMEntry;
            break;

        case CPUPropertyType_SpecCtrl:
            *aValue = mHWData->mSpecCtrl;
            break;

        case CPUPropertyType_SpecCtrlByHost:
            *aValue = mHWData->mSpecCtrlByHost;
            break;

        default:
            return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT Machine::setCPUProperty(CPUPropertyType_T aProperty, BOOL aValue)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    switch (aProperty)
    {
        case CPUPropertyType_PAE:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mPAEEnabled = !!aValue;
            break;

        case CPUPropertyType_LongMode:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mLongMode = !aValue ? settings::Hardware::LongMode_Disabled : settings::Hardware::LongMode_Enabled;
            break;

        case CPUPropertyType_TripleFaultReset:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mTripleFaultReset = !!aValue;
            break;

        case CPUPropertyType_APIC:
            if (mHWData->mX2APIC)
                aValue = TRUE;
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mAPIC = !!aValue;
            break;

        case CPUPropertyType_X2APIC:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mX2APIC = !!aValue;
            if (aValue)
                mHWData->mAPIC = !!aValue;
            break;

        case CPUPropertyType_IBPBOnVMExit:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mIBPBOnVMExit = !!aValue;
            break;

        case CPUPropertyType_IBPBOnVMEntry:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mIBPBOnVMEntry = !!aValue;
            break;

        case CPUPropertyType_SpecCtrl:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mSpecCtrl = !!aValue;
            break;

        case CPUPropertyType_SpecCtrlByHost:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mSpecCtrlByHost = !!aValue;
            break;

        default:
            return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT Machine::getCPUIDLeafByOrdinal(ULONG aOrdinal, ULONG *aIdx, ULONG *aSubIdx, ULONG *aValEax, ULONG *aValEbx,
                                       ULONG *aValEcx, ULONG *aValEdx)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (aOrdinal < mHWData->mCpuIdLeafList.size())
    {
        for (settings::CpuIdLeafsList::const_iterator it = mHWData->mCpuIdLeafList.begin();
             it != mHWData->mCpuIdLeafList.end();
             ++it)
        {
            if (aOrdinal == 0)
            {
                const settings::CpuIdLeaf &rLeaf= *it;
                *aIdx    = rLeaf.idx;
                *aSubIdx = rLeaf.idxSub;
                *aValEax = rLeaf.uEax;
                *aValEbx = rLeaf.uEbx;
                *aValEcx = rLeaf.uEcx;
                *aValEdx = rLeaf.uEdx;
                return S_OK;
            }
            aOrdinal--;
        }
    }
    return E_INVALIDARG;
}

HRESULT Machine::getCPUIDLeaf(ULONG aIdx, ULONG aSubIdx, ULONG *aValEax, ULONG *aValEbx, ULONG *aValEcx, ULONG *aValEdx)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    /*
     * Search the list.
     */
    for (settings::CpuIdLeafsList::const_iterator it = mHWData->mCpuIdLeafList.begin(); it != mHWData->mCpuIdLeafList.end(); ++it)
    {
        const settings::CpuIdLeaf &rLeaf= *it;
        if (   rLeaf.idx == aIdx
            && (   aSubIdx == UINT32_MAX
                || rLeaf.idxSub == aSubIdx) )
        {
            *aValEax = rLeaf.uEax;
            *aValEbx = rLeaf.uEbx;
            *aValEcx = rLeaf.uEcx;
            *aValEdx = rLeaf.uEdx;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}


HRESULT Machine::setCPUIDLeaf(ULONG aIdx, ULONG aSubIdx, ULONG aValEax, ULONG aValEbx, ULONG aValEcx, ULONG aValEdx)
{
    /*
     * Validate input before taking locks and checking state.
     */
    if (aSubIdx != 0 && aSubIdx != UINT32_MAX)
        return setError(E_INVALIDARG, tr("Currently only aSubIdx values 0 and 0xffffffff are supported: %#x"), aSubIdx);
    if (   aIdx >= UINT32_C(0x20)
        && aIdx - UINT32_C(0x80000000) >= UINT32_C(0x20)
        && aIdx - UINT32_C(0xc0000000) >= UINT32_C(0x10) )
        return setError(E_INVALIDARG, tr("CpuId override leaf %#x is out of range"), aIdx);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /*
     * Impose a maximum number of leaves.
     */
    if (mHWData->mCpuIdLeafList.size() > 256)
        return setError(E_FAIL, tr("Max of 256 CPUID override leaves reached"));

    /*
     * Updating the list is a bit more complicated.  So, let's do a remove first followed by an insert.
     */
    i_setModified(IsModified_MachineData);
    mHWData.backup();

    for (settings::CpuIdLeafsList::iterator it = mHWData->mCpuIdLeafList.begin(); it != mHWData->mCpuIdLeafList.end(); )
    {
        settings::CpuIdLeaf &rLeaf= *it;
        if (   rLeaf.idx == aIdx
            && (   aSubIdx == UINT32_MAX
                || rLeaf.idxSub == aSubIdx) )
            it = mHWData->mCpuIdLeafList.erase(it);
        else
            ++it;
    }

    settings::CpuIdLeaf NewLeaf;
    NewLeaf.idx    = aIdx;
    NewLeaf.idxSub = aSubIdx == UINT32_MAX ? 0 : aSubIdx;
    NewLeaf.uEax   = aValEax;
    NewLeaf.uEbx   = aValEbx;
    NewLeaf.uEcx   = aValEcx;
    NewLeaf.uEdx   = aValEdx;
    mHWData->mCpuIdLeafList.push_back(NewLeaf);
    return S_OK;
}

HRESULT Machine::removeCPUIDLeaf(ULONG aIdx, ULONG aSubIdx)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /*
     * Do the removal.
     */
    bool fModified = false;
    for (settings::CpuIdLeafsList::iterator it = mHWData->mCpuIdLeafList.begin(); it != mHWData->mCpuIdLeafList.end(); )
    {
        settings::CpuIdLeaf &rLeaf= *it;
        if (   rLeaf.idx == aIdx
            && (   aSubIdx == UINT32_MAX
                || rLeaf.idxSub == aSubIdx) )
        {
            if (!fModified)
            {
                fModified = true;
                i_setModified(IsModified_MachineData);
                mHWData.backup();
            }
            it = mHWData->mCpuIdLeafList.erase(it);
        }
        else
            ++it;
    }

    return S_OK;
}

HRESULT Machine::removeAllCPUIDLeaves()
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (mHWData->mCpuIdLeafList.size() > 0)
    {
        i_setModified(IsModified_MachineData);
        mHWData.backup();

        mHWData->mCpuIdLeafList.clear();
    }

    return S_OK;
}
HRESULT Machine::getHWVirtExProperty(HWVirtExPropertyType_T aProperty, BOOL *aValue)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    switch(aProperty)
    {
        case HWVirtExPropertyType_Enabled:
            *aValue = mHWData->mHWVirtExEnabled;
            break;

        case HWVirtExPropertyType_VPID:
            *aValue = mHWData->mHWVirtExVPIDEnabled;
            break;

        case HWVirtExPropertyType_NestedPaging:
            *aValue = mHWData->mHWVirtExNestedPagingEnabled;
            break;

        case HWVirtExPropertyType_UnrestrictedExecution:
            *aValue = mHWData->mHWVirtExUXEnabled;
            break;

        case HWVirtExPropertyType_LargePages:
            *aValue = mHWData->mHWVirtExLargePagesEnabled;
#if defined(DEBUG_bird) && defined(RT_OS_LINUX) /* This feature is deadly here */
            *aValue = FALSE;
#endif
            break;

        case HWVirtExPropertyType_Force:
            *aValue = mHWData->mHWVirtExForceEnabled;
            break;

        default:
            return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT Machine::setHWVirtExProperty(HWVirtExPropertyType_T aProperty, BOOL aValue)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    switch(aProperty)
    {
        case HWVirtExPropertyType_Enabled:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mHWVirtExEnabled = !!aValue;
            break;

        case HWVirtExPropertyType_VPID:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mHWVirtExVPIDEnabled = !!aValue;
            break;

        case HWVirtExPropertyType_NestedPaging:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mHWVirtExNestedPagingEnabled = !!aValue;
            break;

        case HWVirtExPropertyType_UnrestrictedExecution:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mHWVirtExUXEnabled = !!aValue;
            break;

        case HWVirtExPropertyType_LargePages:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mHWVirtExLargePagesEnabled = !!aValue;
            break;

        case HWVirtExPropertyType_Force:
            i_setModified(IsModified_MachineData);
            mHWData.backup();
            mHWData->mHWVirtExForceEnabled = !!aValue;
            break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT Machine::getSnapshotFolder(com::Utf8Str &aSnapshotFolder)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    i_calculateFullPath(mUserData->s.strSnapshotFolder, aSnapshotFolder);

    return S_OK;
}

HRESULT Machine::setSnapshotFolder(const com::Utf8Str &aSnapshotFolder)
{
    /** @todo (r=dmik):
     *  1. Allow to change the name of the snapshot folder containing snapshots
     *  2. Rename the folder on disk instead of just changing the property
     *     value (to be smart and not to leave garbage). Note that it cannot be
     *     done here because the change may be rolled back. Thus, the right
     *     place is #saveSettings().
     */

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (!mData->mCurrentSnapshot.isNull())
        return setError(E_FAIL,
                        tr("The snapshot folder of a machine with snapshots cannot be changed (please delete all snapshots first)"));

    Utf8Str strSnapshotFolder(aSnapshotFolder);       // keep original

    if (strSnapshotFolder.isEmpty())
        strSnapshotFolder = "Snapshots";
    int vrc = i_calculateFullPath(strSnapshotFolder,
                                strSnapshotFolder);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL,
                        tr("Invalid snapshot folder '%s' (%Rrc)"),
                        strSnapshotFolder.c_str(), vrc);

    i_setModified(IsModified_MachineData);
    mUserData.backup();

    i_copyPathRelativeToMachine(strSnapshotFolder, mUserData->s.strSnapshotFolder);

    return S_OK;
}

HRESULT Machine::getMediumAttachments(std::vector<ComPtr<IMediumAttachment> > &aMediumAttachments)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aMediumAttachments.resize(mMediumAttachments->size());
    size_t i = 0;
    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it, ++i)
        aMediumAttachments[i] = *it;

    return S_OK;
}

HRESULT Machine::getVRDEServer(ComPtr<IVRDEServer> &aVRDEServer)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    Assert(!!mVRDEServer);

    aVRDEServer = mVRDEServer;

    return S_OK;
}

HRESULT Machine::getAudioAdapter(ComPtr<IAudioAdapter> &aAudioAdapter)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aAudioAdapter = mAudioAdapter;

    return S_OK;
}

HRESULT Machine::getUSBControllers(std::vector<ComPtr<IUSBController> > &aUSBControllers)
{
#ifdef VBOX_WITH_VUSB
    clearError();
    MultiResult rc(S_OK);

# ifdef VBOX_WITH_USB
    rc = mParent->i_host()->i_checkUSBProxyService();
    if (FAILED(rc)) return rc;
# endif

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aUSBControllers.resize(mUSBControllers->size());
    size_t i = 0;
    for (USBControllerList::const_iterator
         it = mUSBControllers->begin();
         it != mUSBControllers->end();
         ++it, ++i)
        aUSBControllers[i] = *it;

    return S_OK;
#else
    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE */
    NOREF(aUSBControllers);
    ReturnComNotImplemented();
#endif /* VBOX_WITH_VUSB */
}

HRESULT Machine::getUSBDeviceFilters(ComPtr<IUSBDeviceFilters> &aUSBDeviceFilters)
{
#ifdef VBOX_WITH_VUSB
    clearError();
    MultiResult rc(S_OK);

# ifdef VBOX_WITH_USB
    rc = mParent->i_host()->i_checkUSBProxyService();
    if (FAILED(rc)) return rc;
# endif

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aUSBDeviceFilters = mUSBDeviceFilters;
    return rc;
#else
    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE */
    NOREF(aUSBDeviceFilters);
    ReturnComNotImplemented();
#endif /* VBOX_WITH_VUSB */
}

HRESULT Machine::getSettingsFilePath(com::Utf8Str &aSettingsFilePath)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aSettingsFilePath = mData->m_strConfigFileFull;

    return S_OK;
}

HRESULT Machine::getSettingsAuxFilePath(com::Utf8Str &aSettingsFilePath)
{
    RT_NOREF(aSettingsFilePath);
    ReturnComNotImplemented();
}

HRESULT Machine::getSettingsModified(BOOL *aSettingsModified)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (FAILED(rc)) return rc;

    if (!mData->pMachineConfigFile->fileExists())
        // this is a new machine, and no config file exists yet:
        *aSettingsModified = TRUE;
    else
        *aSettingsModified = (mData->flModifications != 0);

    return S_OK;
}

HRESULT Machine::getSessionState(SessionState_T *aSessionState)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aSessionState = mData->mSession.mState;

    return S_OK;
}

HRESULT Machine::getSessionName(com::Utf8Str &aSessionName)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aSessionName = mData->mSession.mName;

    return S_OK;
}

HRESULT Machine::getSessionPID(ULONG *aSessionPID)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aSessionPID = mData->mSession.mPID;

    return S_OK;
}

HRESULT Machine::getState(MachineState_T *aState)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aState = mData->mMachineState;
    Assert(mData->mMachineState != MachineState_Null);

    return S_OK;
}

HRESULT Machine::getLastStateChange(LONG64 *aLastStateChange)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aLastStateChange = RTTimeSpecGetMilli(&mData->mLastStateChange);

    return S_OK;
}

HRESULT Machine::getStateFilePath(com::Utf8Str &aStateFilePath)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aStateFilePath = mSSData->strStateFilePath;

    return S_OK;
}

HRESULT Machine::getLogFolder(com::Utf8Str &aLogFolder)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    i_getLogFolder(aLogFolder);

    return S_OK;
}

HRESULT Machine::getCurrentSnapshot(ComPtr<ISnapshot> &aCurrentSnapshot)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aCurrentSnapshot = mData->mCurrentSnapshot;

    return S_OK;
}

HRESULT Machine::getSnapshotCount(ULONG *aSnapshotCount)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aSnapshotCount = mData->mFirstSnapshot.isNull()
                          ? 0
                          : mData->mFirstSnapshot->i_getAllChildrenCount() + 1;

    return S_OK;
}

HRESULT Machine::getCurrentStateModified(BOOL *aCurrentStateModified)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Note: for machines with no snapshots, we always return FALSE
     * (mData->mCurrentStateModified will be TRUE in this case, for historical
     * reasons :) */

    *aCurrentStateModified = mData->mFirstSnapshot.isNull()
                            ? FALSE
                            : mData->mCurrentStateModified;

    return S_OK;
}

HRESULT Machine::getSharedFolders(std::vector<ComPtr<ISharedFolder> > &aSharedFolders)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aSharedFolders.resize(mHWData->mSharedFolders.size());
    size_t i = 0;
    for (std::list<ComObjPtr<SharedFolder> >::const_iterator
         it = mHWData->mSharedFolders.begin();
         it != mHWData->mSharedFolders.end();
         ++it, ++i)
        aSharedFolders[i] = *it;

    return S_OK;
}

HRESULT Machine::getClipboardMode(ClipboardMode_T *aClipboardMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aClipboardMode = mHWData->mClipboardMode;

    return S_OK;
}

HRESULT Machine::setClipboardMode(ClipboardMode_T aClipboardMode)
{
    HRESULT rc = S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    alock.release();
    rc = i_onClipboardModeChange(aClipboardMode);
    alock.acquire();
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mClipboardMode = aClipboardMode;

    /** Save settings if online - @todo why is this required? -- @bugref{6818} */
    if (Global::IsOnline(mData->mMachineState))
        i_saveSettings(NULL);

    return S_OK;
}

HRESULT Machine::getDnDMode(DnDMode_T *aDnDMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aDnDMode = mHWData->mDnDMode;

    return S_OK;
}

HRESULT Machine::setDnDMode(DnDMode_T aDnDMode)
{
    HRESULT rc = S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    alock.release();
    rc = i_onDnDModeChange(aDnDMode);

    alock.acquire();
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mDnDMode = aDnDMode;

    /** Save settings if online - @todo why is this required? -- @bugref{6818} */
    if (Global::IsOnline(mData->mMachineState))
        i_saveSettings(NULL);

    return S_OK;
}

HRESULT Machine::getStorageControllers(std::vector<ComPtr<IStorageController> > &aStorageControllers)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aStorageControllers.resize(mStorageControllers->size());
    size_t i = 0;
    for (StorageControllerList::const_iterator
         it = mStorageControllers->begin();
         it != mStorageControllers->end();
         ++it, ++i)
        aStorageControllers[i] = *it;

    return S_OK;
}

HRESULT Machine::getTeleporterEnabled(BOOL *aEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aEnabled = mUserData->s.fTeleporterEnabled;

    return S_OK;
}

HRESULT Machine::setTeleporterEnabled(BOOL aTeleporterEnabled)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Only allow it to be set to true when PoweredOff or Aborted.
       (Clearing it is always permitted.) */
    if (    aTeleporterEnabled
        &&  mData->mRegistered
        &&  (   !i_isSessionMachine()
             || (   mData->mMachineState != MachineState_PoweredOff
                 && mData->mMachineState != MachineState_Teleported
                 && mData->mMachineState != MachineState_Aborted
                )
            )
       )
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("The machine is not powered off (state is %s)"),
                        Global::stringifyMachineState(mData->mMachineState));

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.fTeleporterEnabled = !! aTeleporterEnabled;

    return S_OK;
}

HRESULT Machine::getTeleporterPort(ULONG *aTeleporterPort)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aTeleporterPort = (ULONG)mUserData->s.uTeleporterPort;

    return S_OK;
}

HRESULT Machine::setTeleporterPort(ULONG aTeleporterPort)
{
    if (aTeleporterPort >= _64K)
        return setError(E_INVALIDARG, tr("Invalid port number %d"), aTeleporterPort);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.uTeleporterPort = (uint32_t)aTeleporterPort;

    return S_OK;
}

HRESULT Machine::getTeleporterAddress(com::Utf8Str &aTeleporterAddress)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aTeleporterAddress = mUserData->s.strTeleporterAddress;

    return S_OK;
}

HRESULT Machine::setTeleporterAddress(const com::Utf8Str &aTeleporterAddress)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.strTeleporterAddress = aTeleporterAddress;

    return S_OK;
}

HRESULT Machine::getTeleporterPassword(com::Utf8Str &aTeleporterPassword)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aTeleporterPassword =  mUserData->s.strTeleporterPassword;

    return S_OK;
}

HRESULT Machine::setTeleporterPassword(const com::Utf8Str &aTeleporterPassword)
{
    /*
     * Hash the password first.
     */
    com::Utf8Str aT = aTeleporterPassword;

    if (!aT.isEmpty())
    {
        if (VBoxIsPasswordHashed(&aT))
            return setError(E_INVALIDARG, tr("Cannot set an already hashed password, only plain text password please"));
        VBoxHashPassword(&aT);
    }

    /*
     * Do the update.
     */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableOrSavedStateDep);
    if (SUCCEEDED(hrc))
    {
        i_setModified(IsModified_MachineData);
        mUserData.backup();
        mUserData->s.strTeleporterPassword = aT;
    }

    return hrc;
}

HRESULT Machine::getFaultToleranceState(FaultToleranceState_T *aFaultToleranceState)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aFaultToleranceState = mUserData->s.enmFaultToleranceState;
    return S_OK;
}

HRESULT Machine::setFaultToleranceState(FaultToleranceState_T aFaultToleranceState)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo deal with running state change. */
    HRESULT rc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.enmFaultToleranceState = aFaultToleranceState;
    return S_OK;
}

HRESULT Machine::getFaultToleranceAddress(com::Utf8Str &aFaultToleranceAddress)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aFaultToleranceAddress = mUserData->s.strFaultToleranceAddress;
    return S_OK;
}

HRESULT Machine::setFaultToleranceAddress(const com::Utf8Str &aFaultToleranceAddress)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo deal with running state change. */
    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.strFaultToleranceAddress = aFaultToleranceAddress;
    return S_OK;
}

HRESULT Machine::getFaultTolerancePort(ULONG *aFaultTolerancePort)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aFaultTolerancePort = mUserData->s.uFaultTolerancePort;
    return S_OK;
}

HRESULT Machine::setFaultTolerancePort(ULONG aFaultTolerancePort)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo deal with running state change. */
    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.uFaultTolerancePort = aFaultTolerancePort;
    return S_OK;
}

HRESULT Machine::getFaultTolerancePassword(com::Utf8Str &aFaultTolerancePassword)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aFaultTolerancePassword = mUserData->s.strFaultTolerancePassword;

    return S_OK;
}

HRESULT Machine::setFaultTolerancePassword(const com::Utf8Str &aFaultTolerancePassword)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo deal with running state change. */
    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.strFaultTolerancePassword = aFaultTolerancePassword;

    return S_OK;
}

HRESULT Machine::getFaultToleranceSyncInterval(ULONG *aFaultToleranceSyncInterval)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aFaultToleranceSyncInterval = mUserData->s.uFaultToleranceInterval;
    return S_OK;
}

HRESULT Machine::setFaultToleranceSyncInterval(ULONG aFaultToleranceSyncInterval)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo deal with running state change. */
    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.uFaultToleranceInterval = aFaultToleranceSyncInterval;
    return S_OK;
}

HRESULT Machine::getRTCUseUTC(BOOL *aRTCUseUTC)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aRTCUseUTC = mUserData->s.fRTCUseUTC;

    return S_OK;
}

HRESULT Machine::setRTCUseUTC(BOOL aRTCUseUTC)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Only allow it to be set to true when PoweredOff or Aborted.
       (Clearing it is always permitted.) */
    if (    aRTCUseUTC
        &&  mData->mRegistered
        &&  (   !i_isSessionMachine()
             || (   mData->mMachineState != MachineState_PoweredOff
                 && mData->mMachineState != MachineState_Teleported
                 && mData->mMachineState != MachineState_Aborted
                )
            )
       )
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("The machine is not powered off (state is %s)"),
                        Global::stringifyMachineState(mData->mMachineState));

    i_setModified(IsModified_MachineData);
    mUserData.backup();
    mUserData->s.fRTCUseUTC = !!aRTCUseUTC;

    return S_OK;
}

HRESULT Machine::getIOCacheEnabled(BOOL *aIOCacheEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aIOCacheEnabled = mHWData->mIOCacheEnabled;

    return S_OK;
}

HRESULT Machine::setIOCacheEnabled(BOOL aIOCacheEnabled)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mIOCacheEnabled = aIOCacheEnabled;

    return S_OK;
}

HRESULT Machine::getIOCacheSize(ULONG *aIOCacheSize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aIOCacheSize = mHWData->mIOCacheSize;

    return S_OK;
}

HRESULT Machine::setIOCacheSize(ULONG aIOCacheSize)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mIOCacheSize = aIOCacheSize;

    return S_OK;
}


/**
 *  @note Locks objects!
 */
HRESULT Machine::lockMachine(const ComPtr<ISession> &aSession,
                             LockType_T aLockType)
{
    /* check the session state */
    SessionState_T state;
    HRESULT rc = aSession->COMGETTER(State)(&state);
    if (FAILED(rc)) return rc;

    if (state != SessionState_Unlocked)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("The given session is busy"));

    // get the client's IInternalSessionControl interface
    ComPtr<IInternalSessionControl> pSessionControl = aSession;
    ComAssertMsgRet(!!pSessionControl, ("No IInternalSessionControl interface"),
                    E_INVALIDARG);

    // session name (only used in some code paths)
    Utf8Str strSessionName;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mData->mRegistered)
        return setError(E_UNEXPECTED,
                        tr("The machine '%s' is not registered"),
                        mUserData->s.strName.c_str());

    LogFlowThisFunc(("mSession.mState=%s\n", Global::stringifySessionState(mData->mSession.mState)));

    SessionState_T oldState = mData->mSession.mState;
    /* Hack: in case the session is closing and there is a progress object
     * which allows waiting for the session to be closed, take the opportunity
     * and do a limited wait (max. 1 second). This helps a lot when the system
     * is busy and thus session closing can take a little while. */
    if (    mData->mSession.mState == SessionState_Unlocking
        &&  mData->mSession.mProgress)
    {
        alock.release();
        mData->mSession.mProgress->WaitForCompletion(1000);
        alock.acquire();
        LogFlowThisFunc(("after waiting: mSession.mState=%s\n", Global::stringifySessionState(mData->mSession.mState)));
    }

    // try again now
    if (    (mData->mSession.mState == SessionState_Locked)         // machine is write-locked already
                                                                    // (i.e. session machine exists)
         && (aLockType == LockType_Shared)                          // caller wants a shared link to the
                                                                    // existing session that holds the write lock:
       )
    {
        // OK, share the session... we are now dealing with three processes:
        // 1) VBoxSVC (where this code runs);
        // 2) process C: the caller's client process (who wants a shared session);
        // 3) process W: the process which already holds the write lock on the machine (write-locking session)

        // copy pointers to W (the write-locking session) before leaving lock (these must not be NULL)
        ComPtr<IInternalSessionControl> pSessionW = mData->mSession.mDirectControl;
        ComAssertRet(!pSessionW.isNull(), E_FAIL);
        ComObjPtr<SessionMachine> pSessionMachine = mData->mSession.mMachine;
        AssertReturn(!pSessionMachine.isNull(), E_FAIL);

        /*
         *  Release the lock before calling the client process. It's safe here
         *  since the only thing to do after we get the lock again is to add
         *  the remote control to the list (which doesn't directly influence
         *  anything).
         */
        alock.release();

        // get the console of the session holding the write lock (this is a remote call)
        ComPtr<IConsole> pConsoleW;
        if (mData->mSession.mLockType == LockType_VM)
        {
            LogFlowThisFunc(("Calling GetRemoteConsole()...\n"));
            rc = pSessionW->COMGETTER(RemoteConsole)(pConsoleW.asOutParam());
            LogFlowThisFunc(("GetRemoteConsole() returned %08X\n", rc));
            if (FAILED(rc))
                // the failure may occur w/o any error info (from RPC), so provide one
                return setError(VBOX_E_VM_ERROR,
                                tr("Failed to get a console object from the direct session (%Rhrc)"), rc);
            ComAssertRet(!pConsoleW.isNull(), E_FAIL);
        }

        // share the session machine and W's console with the caller's session
        LogFlowThisFunc(("Calling AssignRemoteMachine()...\n"));
        rc = pSessionControl->AssignRemoteMachine(pSessionMachine, pConsoleW);
        LogFlowThisFunc(("AssignRemoteMachine() returned %08X\n", rc));

        if (FAILED(rc))
            // the failure may occur w/o any error info (from RPC), so provide one
            return setError(VBOX_E_VM_ERROR,
                            tr("Failed to assign the machine to the session (%Rhrc)"), rc);
        alock.acquire();

        // need to revalidate the state after acquiring the lock again
        if (mData->mSession.mState != SessionState_Locked)
        {
            pSessionControl->Uninitialize();
            return setError(VBOX_E_INVALID_SESSION_STATE,
                            tr("The machine '%s' was unlocked unexpectedly while attempting to share its session"),
                               mUserData->s.strName.c_str());
        }

        // add the caller's session to the list
        mData->mSession.mRemoteControls.push_back(pSessionControl);
    }
    else if (    mData->mSession.mState == SessionState_Locked
              || mData->mSession.mState == SessionState_Unlocking
            )
    {
        // sharing not permitted, or machine still unlocking:
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("The machine '%s' is already locked for a session (or being unlocked)"),
                        mUserData->s.strName.c_str());
    }
    else
    {
        // machine is not locked: then write-lock the machine (create the session machine)

        // must not be busy
        AssertReturn(!Global::IsOnlineOrTransient(mData->mMachineState), E_FAIL);

        // get the caller's session PID
        RTPROCESS pid = NIL_RTPROCESS;
        AssertCompile(sizeof(ULONG) == sizeof(RTPROCESS));
        pSessionControl->COMGETTER(PID)((ULONG*)&pid);
        Assert(pid != NIL_RTPROCESS);

        bool fLaunchingVMProcess = (mData->mSession.mState == SessionState_Spawning);

        if (fLaunchingVMProcess)
        {
            if (mData->mSession.mPID == NIL_RTPROCESS)
            {
                // two or more clients racing for a lock, the one which set the
                // session state to Spawning will win, the others will get an
                // error as we can't decide here if waiting a little would help
                // (only for shared locks this would avoid an error)
                return setError(VBOX_E_INVALID_OBJECT_STATE,
                                tr("The machine '%s' already has a lock request pending"),
                                mUserData->s.strName.c_str());
            }

            // this machine is awaiting for a spawning session to be opened:
            // then the calling process must be the one that got started by
            // LaunchVMProcess()

            LogFlowThisFunc(("mSession.mPID=%d(0x%x)\n", mData->mSession.mPID, mData->mSession.mPID));
            LogFlowThisFunc(("session.pid=%d(0x%x)\n", pid, pid));

#if defined(VBOX_WITH_HARDENING) && defined(RT_OS_WINDOWS)
            /* Hardened windows builds spawns three processes when a VM is
               launched, the 3rd one is the one that will end up here.  */
            RTPROCESS ppid;
            int rc = RTProcQueryParent(pid, &ppid);
            if (RT_SUCCESS(rc))
                rc = RTProcQueryParent(ppid, &ppid);
            if (   (RT_SUCCESS(rc) && mData->mSession.mPID == ppid)
                || rc == VERR_ACCESS_DENIED)
            {
                LogFlowThisFunc(("mSession.mPID => %d(%#x) - windows hardening stub\n", mData->mSession.mPID, pid));
                mData->mSession.mPID = pid;
            }
#endif

            if (mData->mSession.mPID != pid)
                return setError(E_ACCESSDENIED,
                                tr("An unexpected process (PID=0x%08X) has tried to lock the "
                                   "machine '%s', while only the process started by LaunchVMProcess (PID=0x%08X) is allowed"),
                                pid, mUserData->s.strName.c_str(), mData->mSession.mPID);
        }

        // create the mutable SessionMachine from the current machine
        ComObjPtr<SessionMachine> sessionMachine;
        sessionMachine.createObject();
        rc = sessionMachine->init(this);
        AssertComRC(rc);

        /* NOTE: doing return from this function after this point but
         * before the end is forbidden since it may call SessionMachine::uninit()
         * (through the ComObjPtr's destructor) which requests the VirtualBox write
         * lock while still holding the Machine lock in alock so that a deadlock
         * is possible due to the wrong lock order. */

        if (SUCCEEDED(rc))
        {
            /*
             *  Set the session state to Spawning to protect against subsequent
             *  attempts to open a session and to unregister the machine after
             *  we release the lock.
             */
            SessionState_T origState = mData->mSession.mState;
            mData->mSession.mState = SessionState_Spawning;

#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
            /* Get the client token ID to be passed to the client process */
            Utf8Str strTokenId;
            sessionMachine->i_getTokenId(strTokenId);
            Assert(!strTokenId.isEmpty());
#else /* VBOX_WITH_GENERIC_SESSION_WATCHER */
            /* Get the client token to be passed to the client process */
            ComPtr<IToken> pToken(sessionMachine->i_getToken());
            /* The token is now "owned" by pToken, fix refcount */
            if (!pToken.isNull())
                pToken->Release();
#endif /* VBOX_WITH_GENERIC_SESSION_WATCHER */

            /*
             *  Release the lock before calling the client process -- it will call
             *  Machine/SessionMachine methods. Releasing the lock here is quite safe
             *  because the state is Spawning, so that LaunchVMProcess() and
             *  LockMachine() calls will fail. This method, called before we
             *  acquire the lock again, will fail because of the wrong PID.
             *
             *  Note that mData->mSession.mRemoteControls accessed outside
             *  the lock may not be modified when state is Spawning, so it's safe.
             */
            alock.release();

            LogFlowThisFunc(("Calling AssignMachine()...\n"));
#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
            rc = pSessionControl->AssignMachine(sessionMachine, aLockType, Bstr(strTokenId).raw());
#else /* VBOX_WITH_GENERIC_SESSION_WATCHER */
            rc = pSessionControl->AssignMachine(sessionMachine, aLockType, pToken);
            /* Now the token is owned by the client process. */
            pToken.setNull();
#endif /* VBOX_WITH_GENERIC_SESSION_WATCHER */
            LogFlowThisFunc(("AssignMachine() returned %08X\n", rc));

            /* The failure may occur w/o any error info (from RPC), so provide one */
            if (FAILED(rc))
                setError(VBOX_E_VM_ERROR,
                         tr("Failed to assign the machine to the session (%Rhrc)"), rc);

            // get session name, either to remember or to compare against
            // the already known session name.
            {
                Bstr bstrSessionName;
                HRESULT rc2 = aSession->COMGETTER(Name)(bstrSessionName.asOutParam());
                if (SUCCEEDED(rc2))
                    strSessionName = bstrSessionName;
            }

            if (    SUCCEEDED(rc)
                 && fLaunchingVMProcess
               )
            {
                /* complete the remote session initialization */

                /* get the console from the direct session */
                ComPtr<IConsole> console;
                rc = pSessionControl->COMGETTER(RemoteConsole)(console.asOutParam());
                ComAssertComRC(rc);

                if (SUCCEEDED(rc) && !console)
                {
                    ComAssert(!!console);
                    rc = E_FAIL;
                }

                /* assign machine & console to the remote session */
                if (SUCCEEDED(rc))
                {
                    /*
                     *  after LaunchVMProcess(), the first and the only
                     *  entry in remoteControls is that remote session
                     */
                    LogFlowThisFunc(("Calling AssignRemoteMachine()...\n"));
                    rc = mData->mSession.mRemoteControls.front()->AssignRemoteMachine(sessionMachine, console);
                    LogFlowThisFunc(("AssignRemoteMachine() returned %08X\n", rc));

                    /* The failure may occur w/o any error info (from RPC), so provide one */
                    if (FAILED(rc))
                        setError(VBOX_E_VM_ERROR,
                                 tr("Failed to assign the machine to the remote session (%Rhrc)"), rc);
                }

                if (FAILED(rc))
                    pSessionControl->Uninitialize();
            }

            /* acquire the lock again */
            alock.acquire();

            /* Restore the session state */
            mData->mSession.mState = origState;
        }

        // finalize spawning anyway (this is why we don't return on errors above)
        if (fLaunchingVMProcess)
        {
            Assert(mData->mSession.mName == strSessionName);
            /* Note that the progress object is finalized later */
            /** @todo Consider checking mData->mSession.mProgress for cancellation
             *        around here.  */

            /* We don't reset mSession.mPID here because it is necessary for
             * SessionMachine::uninit() to reap the child process later. */

            if (FAILED(rc))
            {
                /* Close the remote session, remove the remote control from the list
                 * and reset session state to Closed (@note keep the code in sync
                 * with the relevant part in checkForSpawnFailure()). */

                Assert(mData->mSession.mRemoteControls.size() == 1);
                if (mData->mSession.mRemoteControls.size() == 1)
                {
                    ErrorInfoKeeper eik;
                    mData->mSession.mRemoteControls.front()->Uninitialize();
                }

                mData->mSession.mRemoteControls.clear();
                mData->mSession.mState = SessionState_Unlocked;
            }
        }
        else
        {
            /* memorize PID of the directly opened session */
            if (SUCCEEDED(rc))
                mData->mSession.mPID = pid;
        }

        if (SUCCEEDED(rc))
        {
            mData->mSession.mLockType = aLockType;
            /* memorize the direct session control and cache IUnknown for it */
            mData->mSession.mDirectControl = pSessionControl;
            mData->mSession.mState = SessionState_Locked;
            if (!fLaunchingVMProcess)
                mData->mSession.mName = strSessionName;
            /* associate the SessionMachine with this Machine */
            mData->mSession.mMachine = sessionMachine;

            /* request an IUnknown pointer early from the remote party for later
             * identity checks (it will be internally cached within mDirectControl
             * at least on XPCOM) */
            ComPtr<IUnknown> unk = mData->mSession.mDirectControl;
            NOREF(unk);
        }

        /* Release the lock since SessionMachine::uninit() locks VirtualBox which
         * would break the lock order */
        alock.release();

        /* uninitialize the created session machine on failure */
        if (FAILED(rc))
            sessionMachine->uninit();
    }

    if (SUCCEEDED(rc))
    {
        /*
         *  tell the client watcher thread to update the set of
         *  machines that have open sessions
         */
        mParent->i_updateClientWatcher();

        if (oldState != SessionState_Locked)
            /* fire an event */
            mParent->i_onSessionStateChange(i_getId(), SessionState_Locked);
    }

    return rc;
}

/**
 *  @note Locks objects!
 */
HRESULT Machine::launchVMProcess(const ComPtr<ISession> &aSession,
                                 const com::Utf8Str &aName,
                                 const com::Utf8Str &aEnvironment,
                                 ComPtr<IProgress> &aProgress)
{
    Utf8Str strFrontend(aName);
    /* "emergencystop" doesn't need the session, so skip the checks/interface
     * retrieval. This code doesn't quite fit in here, but introducing a
     * special API method would be even more effort, and would require explicit
     * support by every API client. It's better to hide the feature a bit. */
    if (strFrontend != "emergencystop")
        CheckComArgNotNull(aSession);

    HRESULT rc = S_OK;
    if (strFrontend.isEmpty())
    {
        Bstr bstrFrontend;
        rc = COMGETTER(DefaultFrontend)(bstrFrontend.asOutParam());
        if (FAILED(rc))
            return rc;
        strFrontend = bstrFrontend;
        if (strFrontend.isEmpty())
        {
            ComPtr<ISystemProperties> systemProperties;
            rc = mParent->COMGETTER(SystemProperties)(systemProperties.asOutParam());
            if (FAILED(rc))
                return rc;
            rc = systemProperties->COMGETTER(DefaultFrontend)(bstrFrontend.asOutParam());
            if (FAILED(rc))
                return rc;
            strFrontend = bstrFrontend;
        }
        /* paranoia - emergencystop is not a valid default */
        if (strFrontend == "emergencystop")
            strFrontend = Utf8Str::Empty;
    }
    /* default frontend: Qt GUI */
    if (strFrontend.isEmpty())
        strFrontend = "GUI/Qt";

    if (strFrontend != "emergencystop")
    {
        /* check the session state */
        SessionState_T state;
        rc = aSession->COMGETTER(State)(&state);
        if (FAILED(rc))
            return rc;

        if (state != SessionState_Unlocked)
            return setError(VBOX_E_INVALID_OBJECT_STATE,
                            tr("The given session is busy"));

        /* get the IInternalSessionControl interface */
        ComPtr<IInternalSessionControl> control(aSession);
        ComAssertMsgRet(!control.isNull(),
                        ("No IInternalSessionControl interface"),
                        E_INVALIDARG);

        /* get the teleporter enable state for the progress object init. */
        BOOL fTeleporterEnabled;
        rc = COMGETTER(TeleporterEnabled)(&fTeleporterEnabled);
        if (FAILED(rc))
            return rc;

        /* create a progress object */
        ComObjPtr<ProgressProxy> progress;
        progress.createObject();
        rc = progress->init(mParent,
                            static_cast<IMachine*>(this),
                            Bstr(tr("Starting VM")).raw(),
                            TRUE /* aCancelable */,
                            fTeleporterEnabled ? 20 : 10 /* uTotalOperationsWeight */,
                            BstrFmt(tr("Creating process for virtual machine \"%s\" (%s)"),
                            mUserData->s.strName.c_str(), strFrontend.c_str()).raw(),
                            2 /* uFirstOperationWeight */,
                            fTeleporterEnabled ? 3 : 1 /* cOtherProgressObjectOperations */);

        if (SUCCEEDED(rc))
        {
            rc = i_launchVMProcess(control, strFrontend, aEnvironment, progress);
            if (SUCCEEDED(rc))
            {
                aProgress = progress;

                /* signal the client watcher thread */
                mParent->i_updateClientWatcher();

                /* fire an event */
                mParent->i_onSessionStateChange(i_getId(), SessionState_Spawning);
            }
        }
    }
    else
    {
        /* no progress object - either instant success or failure */
        aProgress = NULL;

        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (mData->mSession.mState != SessionState_Locked)
            return setError(VBOX_E_INVALID_OBJECT_STATE,
                            tr("The machine '%s' is not locked by a session"),
                            mUserData->s.strName.c_str());

        /* must have a VM process associated - do not kill normal API clients
         * with an open session */
        if (!Global::IsOnline(mData->mMachineState))
            return setError(VBOX_E_INVALID_OBJECT_STATE,
                            tr("The machine '%s' does not have a VM process"),
                            mUserData->s.strName.c_str());

        /* forcibly terminate the VM process */
        if (mData->mSession.mPID != NIL_RTPROCESS)
            RTProcTerminate(mData->mSession.mPID);

        /* signal the client watcher thread, as most likely the client has
         * been terminated */
        mParent->i_updateClientWatcher();
    }

    return rc;
}

HRESULT Machine::setBootOrder(ULONG aPosition, DeviceType_T aDevice)
{
    if (aPosition < 1 || aPosition > SchemaDefs::MaxBootPosition)
        return setError(E_INVALIDARG,
                        tr("Invalid boot position: %lu (must be in range [1, %lu])"),
                        aPosition, SchemaDefs::MaxBootPosition);

    if (aDevice == DeviceType_USB)
        return setError(E_NOTIMPL,
                        tr("Booting from USB device is currently not supported"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mBootOrder[aPosition - 1] = aDevice;

    return S_OK;
}

HRESULT Machine::getBootOrder(ULONG aPosition, DeviceType_T *aDevice)
{
    if (aPosition < 1 || aPosition > SchemaDefs::MaxBootPosition)
        return setError(E_INVALIDARG,
                       tr("Invalid boot position: %lu (must be in range [1, %lu])"),
                       aPosition, SchemaDefs::MaxBootPosition);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aDevice = mHWData->mBootOrder[aPosition - 1];

    return S_OK;
}

HRESULT Machine::attachDevice(const com::Utf8Str &aName,
                              LONG aControllerPort,
                              LONG aDevice,
                              DeviceType_T aType,
                              const ComPtr<IMedium> &aMedium)
{
    IMedium *aM = aMedium;
    LogFlowThisFunc(("aControllerName=\"%s\" aControllerPort=%d aDevice=%d aType=%d aMedium=%p\n",
                     aName.c_str(), aControllerPort, aDevice, aType, aM));

    // request the host lock first, since might be calling Host methods for getting host drives;
    // next, protect the media tree all the while we're in here, as well as our member variables
    AutoMultiWriteLock2 alock(mParent->i_host(), this COMMA_LOCKVAL_SRC_POS);
    AutoWriteLock treeLock(&mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrRunningStateDep);
    if (FAILED(rc)) return rc;

    /// @todo NEWMEDIA implicit machine registration
    if (!mData->mRegistered)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("Cannot attach storage devices to an unregistered machine"));

    AssertReturn(mData->mMachineState != MachineState_Saved, E_FAIL);

    /* Check for an existing controller. */
    ComObjPtr<StorageController> ctl;
    rc = i_getStorageControllerByName(aName, ctl, true /* aSetError */);
    if (FAILED(rc)) return rc;

    StorageControllerType_T ctrlType;
    rc = ctl->COMGETTER(ControllerType)(&ctrlType);
    if (FAILED(rc))
        return setError(E_FAIL,
                        tr("Could not get type of controller '%s'"),
                        aName.c_str());

    bool fSilent = false;
    Utf8Str strReconfig;

    /* Check whether the flag to allow silent storage attachment reconfiguration is set. */
    strReconfig = i_getExtraData(Utf8Str("VBoxInternal2/SilentReconfigureWhilePaused"));
    if (   mData->mMachineState == MachineState_Paused
        && strReconfig == "1")
        fSilent = true;

    /* Check that the controller can do hotplugging if we detach the device while the VM is running. */
    bool fHotplug = false;
    if (!fSilent && Global::IsOnlineOrTransient(mData->mMachineState))
        fHotplug = true;

    if (fHotplug && !i_isControllerHotplugCapable(ctrlType))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Controller '%s' does not support hotplugging"),
                        aName.c_str());

    // check that the port and device are not out of range
    rc = ctl->i_checkPortAndDeviceValid(aControllerPort, aDevice);
    if (FAILED(rc)) return rc;

    /* check if the device slot is already busy */
    MediumAttachment *pAttachTemp;
    if ((pAttachTemp = i_findAttachment(*mMediumAttachments.data(),
                                        aName,
                                        aControllerPort,
                                        aDevice)))
    {
        Medium *pMedium = pAttachTemp->i_getMedium();
        if (pMedium)
        {
            AutoReadLock mediumLock(pMedium COMMA_LOCKVAL_SRC_POS);
            return setError(VBOX_E_OBJECT_IN_USE,
                            tr("Medium '%s' is already attached to port %d, device %d of controller '%s' of this virtual machine"),
                            pMedium->i_getLocationFull().c_str(),
                            aControllerPort,
                            aDevice,
                            aName.c_str());
        }
        else
            return setError(VBOX_E_OBJECT_IN_USE,
                            tr("Device is already attached to port %d, device %d of controller '%s' of this virtual machine"),
                            aControllerPort, aDevice, aName.c_str());
    }

    ComObjPtr<Medium> medium = static_cast<Medium*>(aM);
    if (aMedium && medium.isNull())
        return setError(E_INVALIDARG, "The given medium pointer is invalid");

    AutoCaller mediumCaller(medium);
    if (FAILED(mediumCaller.rc())) return mediumCaller.rc();

    AutoWriteLock mediumLock(medium COMMA_LOCKVAL_SRC_POS);

    if (    (pAttachTemp = i_findAttachment(*mMediumAttachments.data(), medium))
         && !medium.isNull()
       )
        return setError(VBOX_E_OBJECT_IN_USE,
                        tr("Medium '%s' is already attached to this virtual machine"),
                        medium->i_getLocationFull().c_str());

    if (!medium.isNull())
    {
        MediumType_T mtype = medium->i_getType();
        // MediumType_Readonly is also new, but only applies to DVDs and floppies.
        // For DVDs it's not written to the config file, so needs no global config
        // version bump. For floppies it's a new attribute "type", which is ignored
        // by older VirtualBox version, so needs no global config version bump either.
        // For hard disks this type is not accepted.
        if (mtype == MediumType_MultiAttach)
        {
            // This type is new with VirtualBox 4.0 and therefore requires settings
            // version 1.11 in the settings backend. Unfortunately it is not enough to do
            // the usual routine in MachineConfigFile::bumpSettingsVersionIfNeeded() for
            // two reasons: The medium type is a property of the media registry tree, which
            // can reside in the global config file (for pre-4.0 media); we would therefore
            // possibly need to bump the global config version. We don't want to do that though
            // because that might make downgrading to pre-4.0 impossible.
            // As a result, we can only use these two new types if the medium is NOT in the
            // global registry:
            const Guid &uuidGlobalRegistry = mParent->i_getGlobalRegistryId();
            if (    medium->i_isInRegistry(uuidGlobalRegistry)
                 || !mData->pMachineConfigFile->canHaveOwnMediaRegistry()
               )
                return setError(VBOX_E_INVALID_OBJECT_STATE,
                                tr("Cannot attach medium '%s': the media type 'MultiAttach' can only be attached "
                                   "to machines that were created with VirtualBox 4.0 or later"),
                                medium->i_getLocationFull().c_str());
        }
    }

    bool fIndirect = false;
    if (!medium.isNull())
        fIndirect = medium->i_isReadOnly();
    bool associate = true;

    do
    {
        if (    aType == DeviceType_HardDisk
             && mMediumAttachments.isBackedUp())
        {
            const MediumAttachmentList &oldAtts = *mMediumAttachments.backedUpData();

            /* check if the medium was attached to the VM before we started
             * changing attachments in which case the attachment just needs to
             * be restored */
            if ((pAttachTemp = i_findAttachment(oldAtts, medium)))
            {
                AssertReturn(!fIndirect, E_FAIL);

                /* see if it's the same bus/channel/device */
                if (pAttachTemp->i_matches(aName, aControllerPort, aDevice))
                {
                    /* the simplest case: restore the whole attachment
                     * and return, nothing else to do */
                    mMediumAttachments->push_back(pAttachTemp);

                    /* Reattach the medium to the VM. */
                    if (fHotplug || fSilent)
                    {
                        mediumLock.release();
                        treeLock.release();
                        alock.release();

                        MediumLockList *pMediumLockList(new MediumLockList());

                        rc = medium->i_createMediumLockList(true /* fFailIfInaccessible */,
                                                            medium /* pToLockWrite */,
                                                            false /* fMediumLockWriteAll */,
                                                            NULL,
                                                            *pMediumLockList);
                        alock.acquire();
                        if (FAILED(rc))
                            delete pMediumLockList;
                        else
                        {
                            mData->mSession.mLockedMedia.Unlock();
                            alock.release();
                            rc = mData->mSession.mLockedMedia.Insert(pAttachTemp, pMediumLockList);
                            mData->mSession.mLockedMedia.Lock();
                            alock.acquire();
                        }
                        alock.release();

                        if (SUCCEEDED(rc))
                        {
                            rc = i_onStorageDeviceChange(pAttachTemp, FALSE /* aRemove */, fSilent);
                            /* Remove lock list in case of error. */
                            if (FAILED(rc))
                            {
                                mData->mSession.mLockedMedia.Unlock();
                                mData->mSession.mLockedMedia.Remove(pAttachTemp);
                                mData->mSession.mLockedMedia.Lock();
                            }
                        }
                    }

                    return S_OK;
                }

                /* bus/channel/device differ; we need a new attachment object,
                 * but don't try to associate it again */
                associate = false;
                break;
            }
        }

        /* go further only if the attachment is to be indirect */
        if (!fIndirect)
            break;

        /* perform the so called smart attachment logic for indirect
         * attachments. Note that smart attachment is only applicable to base
         * hard disks. */

        if (medium->i_getParent().isNull())
        {
            /* first, investigate the backup copy of the current hard disk
             * attachments to make it possible to re-attach existing diffs to
             * another device slot w/o losing their contents */
            if (mMediumAttachments.isBackedUp())
            {
                const MediumAttachmentList &oldAtts = *mMediumAttachments.backedUpData();

                MediumAttachmentList::const_iterator foundIt = oldAtts.end();
                uint32_t foundLevel = 0;

                for (MediumAttachmentList::const_iterator
                     it = oldAtts.begin();
                     it != oldAtts.end();
                     ++it)
                {
                    uint32_t level = 0;
                    MediumAttachment *pAttach = *it;
                    ComObjPtr<Medium> pMedium = pAttach->i_getMedium();
                    Assert(!pMedium.isNull() || pAttach->i_getType() != DeviceType_HardDisk);
                    if (pMedium.isNull())
                        continue;

                    if (pMedium->i_getBase(&level) == medium)
                    {
                        /* skip the hard disk if its currently attached (we
                         * cannot attach the same hard disk twice) */
                        if (i_findAttachment(*mMediumAttachments.data(),
                                             pMedium))
                            continue;

                        /* matched device, channel and bus (i.e. attached to the
                         * same place) will win and immediately stop the search;
                         * otherwise the attachment that has the youngest
                         * descendant of medium will be used
                         */
                        if (pAttach->i_matches(aName, aControllerPort, aDevice))
                        {
                            /* the simplest case: restore the whole attachment
                             * and return, nothing else to do */
                            mMediumAttachments->push_back(*it);

                            /* Reattach the medium to the VM. */
                            if (fHotplug || fSilent)
                            {
                                mediumLock.release();
                                treeLock.release();
                                alock.release();

                                MediumLockList *pMediumLockList(new MediumLockList());

                                rc = medium->i_createMediumLockList(true /* fFailIfInaccessible */,
                                                                    medium /* pToLockWrite */,
                                                                    false /* fMediumLockWriteAll */,
                                                                    NULL,
                                                                    *pMediumLockList);
                                alock.acquire();
                                if (FAILED(rc))
                                    delete pMediumLockList;
                                else
                                {
                                    mData->mSession.mLockedMedia.Unlock();
                                    alock.release();
                                    rc = mData->mSession.mLockedMedia.Insert(pAttachTemp, pMediumLockList);
                                    mData->mSession.mLockedMedia.Lock();
                                    alock.acquire();
                                }
                                alock.release();

                                if (SUCCEEDED(rc))
                                {
                                    rc = i_onStorageDeviceChange(pAttachTemp, FALSE /* aRemove */, fSilent);
                                    /* Remove lock list in case of error. */
                                    if (FAILED(rc))
                                    {
                                        mData->mSession.mLockedMedia.Unlock();
                                        mData->mSession.mLockedMedia.Remove(pAttachTemp);
                                        mData->mSession.mLockedMedia.Lock();
                                    }
                                }
                            }

                            return S_OK;
                        }
                        else if (    foundIt == oldAtts.end()
                                  || level > foundLevel /* prefer younger */
                                )
                        {
                            foundIt = it;
                            foundLevel = level;
                        }
                    }
                }

                if (foundIt != oldAtts.end())
                {
                    /* use the previously attached hard disk */
                    medium = (*foundIt)->i_getMedium();
                    mediumCaller.attach(medium);
                    if (FAILED(mediumCaller.rc())) return mediumCaller.rc();
                    mediumLock.attach(medium);
                    /* not implicit, doesn't require association with this VM */
                    fIndirect = false;
                    associate = false;
                    /* go right to the MediumAttachment creation */
                    break;
                }
            }

            /* must give up the medium lock and medium tree lock as below we
             * go over snapshots, which needs a lock with higher lock order. */
            mediumLock.release();
            treeLock.release();

            /* then, search through snapshots for the best diff in the given
             * hard disk's chain to base the new diff on */

            ComObjPtr<Medium> base;
            ComObjPtr<Snapshot> snap = mData->mCurrentSnapshot;
            while (snap)
            {
                AutoReadLock snapLock(snap COMMA_LOCKVAL_SRC_POS);

                const MediumAttachmentList &snapAtts = *snap->i_getSnapshotMachine()->mMediumAttachments.data();

                MediumAttachment *pAttachFound = NULL;
                uint32_t foundLevel = 0;

                for (MediumAttachmentList::const_iterator
                     it = snapAtts.begin();
                     it != snapAtts.end();
                     ++it)
                {
                    MediumAttachment *pAttach = *it;
                    ComObjPtr<Medium> pMedium = pAttach->i_getMedium();
                    Assert(!pMedium.isNull() || pAttach->i_getType() != DeviceType_HardDisk);
                    if (pMedium.isNull())
                        continue;

                    uint32_t level = 0;
                    if (pMedium->i_getBase(&level) == medium)
                    {
                        /* matched device, channel and bus (i.e. attached to the
                         * same place) will win and immediately stop the search;
                         * otherwise the attachment that has the youngest
                         * descendant of medium will be used
                         */
                        if (    pAttach->i_getDevice() == aDevice
                             && pAttach->i_getPort() == aControllerPort
                             && pAttach->i_getControllerName() == aName
                           )
                        {
                            pAttachFound = pAttach;
                            break;
                        }
                        else if (    !pAttachFound
                                  || level > foundLevel /* prefer younger */
                                )
                        {
                            pAttachFound = pAttach;
                            foundLevel = level;
                        }
                    }
                }

                if (pAttachFound)
                {
                    base = pAttachFound->i_getMedium();
                    break;
                }

                snap = snap->i_getParent();
            }

            /* re-lock medium tree and the medium, as we need it below */
            treeLock.acquire();
            mediumLock.acquire();

            /* found a suitable diff, use it as a base */
            if (!base.isNull())
            {
                medium = base;
                mediumCaller.attach(medium);
                if (FAILED(mediumCaller.rc())) return mediumCaller.rc();
                mediumLock.attach(medium);
            }
        }

        Utf8Str strFullSnapshotFolder;
        i_calculateFullPath(mUserData->s.strSnapshotFolder, strFullSnapshotFolder);

        ComObjPtr<Medium> diff;
        diff.createObject();
        // store this diff in the same registry as the parent
        Guid uuidRegistryParent;
        if (!medium->i_getFirstRegistryMachineId(uuidRegistryParent))
        {
            // parent image has no registry: this can happen if we're attaching a new immutable
            // image that has not yet been attached (medium then points to the base and we're
            // creating the diff image for the immutable, and the parent is not yet registered);
            // put the parent in the machine registry then
            mediumLock.release();
            treeLock.release();
            alock.release();
            i_addMediumToRegistry(medium);
            alock.acquire();
            treeLock.acquire();
            mediumLock.acquire();
            medium->i_getFirstRegistryMachineId(uuidRegistryParent);
        }
        rc = diff->init(mParent,
                        medium->i_getPreferredDiffFormat(),
                        strFullSnapshotFolder.append(RTPATH_SLASH_STR),
                        uuidRegistryParent,
                        DeviceType_HardDisk);
        if (FAILED(rc)) return rc;

        /* Apply the normal locking logic to the entire chain. */
        MediumLockList *pMediumLockList(new MediumLockList());
        mediumLock.release();
        treeLock.release();
        rc = diff->i_createMediumLockList(true /* fFailIfInaccessible */,
                                          diff /* pToLockWrite */,
                                          false /* fMediumLockWriteAll */,
                                          medium,
                                          *pMediumLockList);
        treeLock.acquire();
        mediumLock.acquire();
        if (SUCCEEDED(rc))
        {
            mediumLock.release();
            treeLock.release();
            rc = pMediumLockList->Lock();
            treeLock.acquire();
            mediumLock.acquire();
            if (FAILED(rc))
                setError(rc,
                         tr("Could not lock medium when creating diff '%s'"),
                         diff->i_getLocationFull().c_str());
            else
            {
                /* will release the lock before the potentially lengthy
                 * operation, so protect with the special state */
                MachineState_T oldState = mData->mMachineState;
                i_setMachineState(MachineState_SettingUp);

                mediumLock.release();
                treeLock.release();
                alock.release();

                rc = medium->i_createDiffStorage(diff,
                                                 medium->i_getPreferredDiffVariant(),
                                                 pMediumLockList,
                                                 NULL /* aProgress */,
                                                 true /* aWait */);

                alock.acquire();
                treeLock.acquire();
                mediumLock.acquire();

                i_setMachineState(oldState);
            }
        }

        /* Unlock the media and free the associated memory. */
        delete pMediumLockList;

        if (FAILED(rc)) return rc;

        /* use the created diff for the actual attachment */
        medium = diff;
        mediumCaller.attach(medium);
        if (FAILED(mediumCaller.rc())) return mediumCaller.rc();
        mediumLock.attach(medium);
    }
    while (0);

    ComObjPtr<MediumAttachment> attachment;
    attachment.createObject();
    rc = attachment->init(this,
                          medium,
                          aName,
                          aControllerPort,
                          aDevice,
                          aType,
                          fIndirect,
                          false /* fPassthrough */,
                          false /* fTempEject */,
                          false /* fNonRotational */,
                          false /* fDiscard */,
                          fHotplug /* fHotPluggable */,
                          Utf8Str::Empty);
    if (FAILED(rc)) return rc;

    if (associate && !medium.isNull())
    {
        // as the last step, associate the medium to the VM
        rc = medium->i_addBackReference(mData->mUuid);
        // here we can fail because of Deleting, or being in process of creating a Diff
        if (FAILED(rc)) return rc;

        mediumLock.release();
        treeLock.release();
        alock.release();
        i_addMediumToRegistry(medium);
        alock.acquire();
        treeLock.acquire();
        mediumLock.acquire();
    }

    /* success: finally remember the attachment */
    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();
    mMediumAttachments->push_back(attachment);

    mediumLock.release();
    treeLock.release();
    alock.release();

    if (fHotplug || fSilent)
    {
        if (!medium.isNull())
        {
            MediumLockList *pMediumLockList(new MediumLockList());

            rc = medium->i_createMediumLockList(true /* fFailIfInaccessible */,
                                                medium /* pToLockWrite */,
                                                false /* fMediumLockWriteAll */,
                                                NULL,
                                                *pMediumLockList);
            alock.acquire();
            if (FAILED(rc))
                delete pMediumLockList;
            else
            {
                mData->mSession.mLockedMedia.Unlock();
                alock.release();
                rc = mData->mSession.mLockedMedia.Insert(attachment, pMediumLockList);
                mData->mSession.mLockedMedia.Lock();
                alock.acquire();
            }
            alock.release();
        }

        if (SUCCEEDED(rc))
        {
            rc = i_onStorageDeviceChange(attachment, FALSE /* aRemove */, fSilent);
            /* Remove lock list in case of error. */
            if (FAILED(rc))
            {
                mData->mSession.mLockedMedia.Unlock();
                mData->mSession.mLockedMedia.Remove(attachment);
                mData->mSession.mLockedMedia.Lock();
            }
        }
    }

    /* Save modified registries, but skip this machine as it's the caller's
     * job to save its settings like all other settings changes. */
    mParent->i_unmarkRegistryModified(i_getId());
    mParent->i_saveModifiedRegistries();

    return rc;
}

HRESULT Machine::detachDevice(const com::Utf8Str &aName, LONG aControllerPort,
                              LONG aDevice)
{
    LogFlowThisFunc(("aControllerName=\"%s\" aControllerPort=%d aDevice=%d\n",
                     aName.c_str(), aControllerPort, aDevice));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrRunningStateDep);
    if (FAILED(rc)) return rc;

    AssertReturn(mData->mMachineState != MachineState_Saved, E_FAIL);

    /* Check for an existing controller. */
    ComObjPtr<StorageController> ctl;
    rc = i_getStorageControllerByName(aName, ctl, true /* aSetError */);
    if (FAILED(rc)) return rc;

    StorageControllerType_T ctrlType;
    rc = ctl->COMGETTER(ControllerType)(&ctrlType);
    if (FAILED(rc))
        return setError(E_FAIL,
                        tr("Could not get type of controller '%s'"),
                        aName.c_str());

    bool fSilent = false;
    Utf8Str strReconfig;

    /* Check whether the flag to allow silent storage attachment reconfiguration is set. */
    strReconfig = i_getExtraData(Utf8Str("VBoxInternal2/SilentReconfigureWhilePaused"));
    if (   mData->mMachineState == MachineState_Paused
        && strReconfig == "1")
        fSilent = true;

    /* Check that the controller can do hotplugging if we detach the device while the VM is running. */
    bool fHotplug = false;
    if (!fSilent && Global::IsOnlineOrTransient(mData->mMachineState))
        fHotplug = true;

    if (fHotplug && !i_isControllerHotplugCapable(ctrlType))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Controller '%s' does not support hotplugging"),
                        aName.c_str());

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());

    if (fHotplug && !pAttach->i_getHotPluggable())
        return setError(VBOX_E_NOT_SUPPORTED,
                        tr("The device slot %d on port %d of controller '%s' does not support hotplugging"),
                        aDevice, aControllerPort, aName.c_str());

    /*
     * The VM has to detach the device before we delete any implicit diffs.
     * If this fails we can roll back without loosing data.
     */
    if (fHotplug || fSilent)
    {
        alock.release();
        rc = i_onStorageDeviceChange(pAttach, TRUE /* aRemove */, fSilent);
        alock.acquire();
    }
    if (FAILED(rc)) return rc;

    /* If we are here everything went well and we can delete the implicit now. */
    rc = i_detachDevice(pAttach, alock, NULL /* pSnapshot */);

    alock.release();

    /* Save modified registries, but skip this machine as it's the caller's
     * job to save its settings like all other settings changes. */
    mParent->i_unmarkRegistryModified(i_getId());
    mParent->i_saveModifiedRegistries();

    return rc;
}

HRESULT Machine::passthroughDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                   LONG aDevice, BOOL aPassthrough)
{
    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aPassthrough=%d\n",
                     aName.c_str(), aControllerPort, aDevice, aPassthrough));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    AssertReturn(mData->mMachineState != MachineState_Saved, E_FAIL);

    if (Global::IsOnlineOrTransient(mData->mMachineState))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Invalid machine state: %s"),
                        Global::stringifyMachineState(mData->mMachineState));

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());


    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

    if (pAttach->i_getType() != DeviceType_DVD)
        return setError(E_INVALIDARG,
                        tr("Setting passthrough rejected as the device attached to device slot %d on port %d of controller '%s' is not a DVD"),
                        aDevice, aControllerPort, aName.c_str());
    pAttach->i_updatePassthrough(!!aPassthrough);

    return S_OK;
}

HRESULT Machine::temporaryEjectDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                      LONG aDevice, BOOL aTemporaryEject)
{

    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aTemporaryEject=%d\n",
                     aName.c_str(), aControllerPort, aDevice, aTemporaryEject));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (FAILED(rc)) return rc;

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());


    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

    if (pAttach->i_getType() != DeviceType_DVD)
        return setError(E_INVALIDARG,
                        tr("Setting temporary eject flag rejected as the device attached to device slot %d on port %d of controller '%s' is not a DVD"),
                        aDevice, aControllerPort, aName.c_str());
    pAttach->i_updateTempEject(!!aTemporaryEject);

    return S_OK;
}

HRESULT Machine::nonRotationalDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                     LONG aDevice, BOOL aNonRotational)
{

    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aNonRotational=%d\n",
                     aName.c_str(), aControllerPort, aDevice, aNonRotational));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    AssertReturn(mData->mMachineState != MachineState_Saved, E_FAIL);

    if (Global::IsOnlineOrTransient(mData->mMachineState))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Invalid machine state: %s"),
                        Global::stringifyMachineState(mData->mMachineState));

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());


    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

    if (pAttach->i_getType() != DeviceType_HardDisk)
        return setError(E_INVALIDARG,
                        tr("Setting the non-rotational medium flag rejected as the device attached to device slot %d on port %d of controller '%s' is not a hard disk"),
                        aDevice, aControllerPort, aName.c_str());
    pAttach->i_updateNonRotational(!!aNonRotational);

    return S_OK;
}

HRESULT Machine::setAutoDiscardForDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                         LONG aDevice, BOOL aDiscard)
{

    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aDiscard=%d\n",
                     aName.c_str(), aControllerPort, aDevice, aDiscard));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    AssertReturn(mData->mMachineState != MachineState_Saved, E_FAIL);

    if (Global::IsOnlineOrTransient(mData->mMachineState))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Invalid machine state: %s"),
                        Global::stringifyMachineState(mData->mMachineState));

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());


    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

    if (pAttach->i_getType() != DeviceType_HardDisk)
        return setError(E_INVALIDARG,
                        tr("Setting the discard medium flag rejected as the device attached to device slot %d on port %d of controller '%s' is not a hard disk"),
                        aDevice, aControllerPort, aName.c_str());
    pAttach->i_updateDiscard(!!aDiscard);

    return S_OK;
}

HRESULT Machine::setHotPluggableForDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                          LONG aDevice, BOOL aHotPluggable)
{
    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aHotPluggable=%d\n",
                     aName.c_str(), aControllerPort, aDevice, aHotPluggable));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    AssertReturn(mData->mMachineState != MachineState_Saved, E_FAIL);

    if (Global::IsOnlineOrTransient(mData->mMachineState))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Invalid machine state: %s"),
                        Global::stringifyMachineState(mData->mMachineState));

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());

    /* Check for an existing controller. */
    ComObjPtr<StorageController> ctl;
    rc = i_getStorageControllerByName(aName, ctl, true /* aSetError */);
    if (FAILED(rc)) return rc;

    StorageControllerType_T ctrlType;
    rc = ctl->COMGETTER(ControllerType)(&ctrlType);
    if (FAILED(rc))
        return setError(E_FAIL,
                        tr("Could not get type of controller '%s'"),
                        aName.c_str());

    if (!i_isControllerHotplugCapable(ctrlType))
    return setError(VBOX_E_NOT_SUPPORTED,
                    tr("Controller '%s' does not support changing the hot-pluggable device flag"),
                    aName.c_str());

    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

    if (pAttach->i_getType() == DeviceType_Floppy)
        return setError(E_INVALIDARG,
                        tr("Setting the hot-pluggable device flag rejected as the device attached to device slot %d on port %d of controller '%s' is a floppy drive"),
                        aDevice, aControllerPort, aName.c_str());
    pAttach->i_updateHotPluggable(!!aHotPluggable);

    return S_OK;
}

HRESULT Machine::setNoBandwidthGroupForDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                              LONG aDevice)
{
    int rc = S_OK;
    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d\n",
                     aName.c_str(), aControllerPort, aDevice));

    rc = setBandwidthGroupForDevice(aName, aControllerPort, aDevice, NULL);

    return rc;
}

HRESULT Machine::setBandwidthGroupForDevice(const com::Utf8Str &aName, LONG aControllerPort,
                                            LONG aDevice, const ComPtr<IBandwidthGroup> &aBandwidthGroup)
{
    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d\n",
                     aName.c_str(), aControllerPort, aDevice));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    if (Global::IsOnlineOrTransient(mData->mMachineState))
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Invalid machine state: %s"),
                        Global::stringifyMachineState(mData->mMachineState));

    MediumAttachment *pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                 aName,
                                                 aControllerPort,
                                                 aDevice);
    if (!pAttach)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());


    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    IBandwidthGroup *iB = aBandwidthGroup;
    ComObjPtr<BandwidthGroup> group = static_cast<BandwidthGroup*>(iB);
    if (aBandwidthGroup && group.isNull())
        return setError(E_INVALIDARG, "The given bandwidth group pointer is invalid");

    AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

    const Utf8Str strBandwidthGroupOld = pAttach->i_getBandwidthGroup();
    if (strBandwidthGroupOld.isNotEmpty())
    {
        /* Get the bandwidth group object and release it - this must not fail. */
        ComObjPtr<BandwidthGroup> pBandwidthGroupOld;
        rc = i_getBandwidthGroup(strBandwidthGroupOld, pBandwidthGroupOld, false);
        Assert(SUCCEEDED(rc));

        pBandwidthGroupOld->i_release();
        pAttach->i_updateBandwidthGroup(Utf8Str::Empty);
    }

    if (!group.isNull())
    {
        group->i_reference();
        pAttach->i_updateBandwidthGroup(group->i_getName());
    }

    return S_OK;
}

HRESULT Machine::attachDeviceWithoutMedium(const com::Utf8Str &aName,
                                           LONG aControllerPort,
                                           LONG aDevice,
                                           DeviceType_T aType)
{
     HRESULT rc = S_OK;

     LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aType=%d\n",
                      aName.c_str(), aControllerPort, aDevice, aType));

     rc = attachDevice(aName, aControllerPort, aDevice, aType, NULL);

     return rc;
}


HRESULT Machine::unmountMedium(const com::Utf8Str &aName,
                               LONG aControllerPort,
                               LONG aDevice,
                               BOOL aForce)
{
     int rc = S_OK;
     LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d",
                      aName.c_str(), aControllerPort, aForce));

     rc = mountMedium(aName, aControllerPort, aDevice, NULL, aForce);

     return rc;
}

HRESULT Machine::mountMedium(const com::Utf8Str &aName,
                             LONG aControllerPort,
                             LONG aDevice,
                             const ComPtr<IMedium> &aMedium,
                             BOOL aForce)
{
    int rc = S_OK;
    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d aForce=%d\n",
                     aName.c_str(), aControllerPort, aDevice, aForce));

    // request the host lock first, since might be calling Host methods for getting host drives;
    // next, protect the media tree all the while we're in here, as well as our member variables
    AutoMultiWriteLock3 multiLock(mParent->i_host()->lockHandle(),
                                  this->lockHandle(),
                                  &mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<MediumAttachment> pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                           aName,
                                                           aControllerPort,
                                                           aDevice);
    if (pAttach.isNull())
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No drive attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());

    /* Remember previously mounted medium. The medium before taking the
     * backup is not necessarily the same thing. */
    ComObjPtr<Medium> oldmedium;
    oldmedium = pAttach->i_getMedium();

    IMedium *iM = aMedium;
    ComObjPtr<Medium> pMedium = static_cast<Medium*>(iM);
    if (aMedium && pMedium.isNull())
        return setError(E_INVALIDARG, "The given medium pointer is invalid");

    AutoCaller mediumCaller(pMedium);
    if (FAILED(mediumCaller.rc())) return mediumCaller.rc();

    AutoWriteLock mediumLock(pMedium COMMA_LOCKVAL_SRC_POS);
    if (pMedium)
    {
        DeviceType_T mediumType = pAttach->i_getType();
        switch (mediumType)
        {
            case DeviceType_DVD:
            case DeviceType_Floppy:
            break;

            default:
                return setError(VBOX_E_INVALID_OBJECT_STATE,
                                tr("The device at port %d, device %d of controller '%s' of this virtual machine is not removeable"),
                                aControllerPort,
                                aDevice,
                                aName.c_str());
        }
    }

    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();

    {
        // The backup operation makes the pAttach reference point to the
        // old settings. Re-get the correct reference.
        pAttach = i_findAttachment(*mMediumAttachments.data(),
                                   aName,
                                   aControllerPort,
                                   aDevice);
        if (!oldmedium.isNull())
            oldmedium->i_removeBackReference(mData->mUuid);
        if (!pMedium.isNull())
        {
            pMedium->i_addBackReference(mData->mUuid);

            mediumLock.release();
            multiLock.release();
            i_addMediumToRegistry(pMedium);
            multiLock.acquire();
            mediumLock.acquire();
        }

        AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);
        pAttach->i_updateMedium(pMedium);
    }

    i_setModified(IsModified_Storage);

    mediumLock.release();
    multiLock.release();
    rc = i_onMediumChange(pAttach, aForce);
    multiLock.acquire();
    mediumLock.acquire();

    /* On error roll back this change only. */
    if (FAILED(rc))
    {
        if (!pMedium.isNull())
            pMedium->i_removeBackReference(mData->mUuid);
        pAttach = i_findAttachment(*mMediumAttachments.data(),
                                   aName,
                                   aControllerPort,
                                   aDevice);
        /* If the attachment is gone in the meantime, bail out. */
        if (pAttach.isNull())
            return rc;
        AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);
        if (!oldmedium.isNull())
            oldmedium->i_addBackReference(mData->mUuid);
        pAttach->i_updateMedium(oldmedium);
    }

    mediumLock.release();
    multiLock.release();

    /* Save modified registries, but skip this machine as it's the caller's
     * job to save its settings like all other settings changes. */
    mParent->i_unmarkRegistryModified(i_getId());
    mParent->i_saveModifiedRegistries();

    return rc;
}
HRESULT Machine::getMedium(const com::Utf8Str &aName,
                           LONG aControllerPort,
                           LONG aDevice,
                           ComPtr<IMedium> &aMedium)
{
    LogFlowThisFunc(("aName=\"%s\" aControllerPort=%d aDevice=%d\n",
                     aName.c_str(), aControllerPort, aDevice));

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aMedium = NULL;

    ComObjPtr<MediumAttachment> pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                           aName,
                                                           aControllerPort,
                                                           aDevice);
    if (pAttach.isNull())
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());

    aMedium = pAttach->i_getMedium();

    return S_OK;
}

HRESULT Machine::getSerialPort(ULONG aSlot, ComPtr<ISerialPort> &aPort)
{

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    mSerialPorts[aSlot].queryInterfaceTo(aPort.asOutParam());

    return S_OK;
}

HRESULT Machine::getParallelPort(ULONG aSlot, ComPtr<IParallelPort> &aPort)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    mParallelPorts[aSlot].queryInterfaceTo(aPort.asOutParam());

    return S_OK;
}

HRESULT Machine::getNetworkAdapter(ULONG aSlot, ComPtr<INetworkAdapter> &aAdapter)
{
    /* Do not assert if slot is out of range, just return the advertised
       status.  testdriver/vbox.py triggers this in logVmInfo. */
    if (aSlot >= mNetworkAdapters.size())
        return setError(E_INVALIDARG,
                        tr("No network adapter in slot %RU32 (total %RU32 adapters)"),
                        aSlot, mNetworkAdapters.size());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    mNetworkAdapters[aSlot].queryInterfaceTo(aAdapter.asOutParam());

    return S_OK;
}

HRESULT Machine::getExtraDataKeys(std::vector<com::Utf8Str> &aKeys)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aKeys.resize(mData->pMachineConfigFile->mapExtraDataItems.size());
    size_t i = 0;
    for (settings::StringsMap::const_iterator
         it = mData->pMachineConfigFile->mapExtraDataItems.begin();
         it != mData->pMachineConfigFile->mapExtraDataItems.end();
         ++it, ++i)
        aKeys[i] = it->first;

    return S_OK;
}

  /**
   *  @note Locks this object for reading.
   */
HRESULT Machine::getExtraData(const com::Utf8Str &aKey,
                              com::Utf8Str &aValue)
{
    /* start with nothing found */
    aValue = "";

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    settings::StringsMap::const_iterator it = mData->pMachineConfigFile->mapExtraDataItems.find(aKey);
    if (it != mData->pMachineConfigFile->mapExtraDataItems.end())
        // found:
        aValue = it->second; // source is a Utf8Str

    /* return the result to caller (may be empty) */
    return S_OK;
}

  /**
   *  @note Locks mParent for writing + this object for writing.
   */
HRESULT Machine::setExtraData(const com::Utf8Str &aKey, const com::Utf8Str &aValue)
{
    Utf8Str strOldValue;            // empty

    // locking note: we only hold the read lock briefly to look up the old value,
    // then release it and call the onExtraCanChange callbacks. There is a small
    // chance of a race insofar as the callback might be called twice if two callers
    // change the same key at the same time, but that's a much better solution
    // than the deadlock we had here before. The actual changing of the extradata
    // is then performed under the write lock and race-free.

    // look up the old value first; if nothing has changed then we need not do anything
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS); // hold read lock only while looking up

        // For snapshots don't even think about allowing changes, extradata
        // is global for a machine, so there is nothing snapshot specific.
        if (i_isSnapshotMachine())
            return setError(VBOX_E_INVALID_VM_STATE,
                            tr("Cannot set extradata for a snapshot"));

        // check if the right IMachine instance is used
        if (mData->mRegistered && !i_isSessionMachine())
            return setError(VBOX_E_INVALID_VM_STATE,
                            tr("Cannot set extradata for an immutable machine"));

        settings::StringsMap::const_iterator it = mData->pMachineConfigFile->mapExtraDataItems.find(aKey);
        if (it != mData->pMachineConfigFile->mapExtraDataItems.end())
            strOldValue = it->second;
    }

    bool fChanged;
    if ((fChanged = (strOldValue != aValue)))
    {
        // ask for permission from all listeners outside the locks;
        // i_onExtraDataCanChange() only briefly requests the VirtualBox
        // lock to copy the list of callbacks to invoke
        Bstr error;
        Bstr bstrValue(aValue);

        if (!mParent->i_onExtraDataCanChange(mData->mUuid, Bstr(aKey).raw(), bstrValue.raw(), error))
        {
            const char *sep = error.isEmpty() ? "" : ": ";
            CBSTR err = error.raw();
            Log1WarningFunc(("Someone vetoed! Change refused%s%ls\n", sep, err));
            return setError(E_ACCESSDENIED,
                            tr("Could not set extra data because someone refused the requested change of '%s' to '%s'%s%ls"),
                            aKey.c_str(),
                            aValue.c_str(),
                            sep,
                            err);
        }

        // data is changing and change not vetoed: then write it out under the lock
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (aValue.isEmpty())
            mData->pMachineConfigFile->mapExtraDataItems.erase(aKey);
        else
            mData->pMachineConfigFile->mapExtraDataItems[aKey] = aValue;
                // creates a new key if needed

        bool fNeedsGlobalSaveSettings = false;
        // This saving of settings is tricky: there is no "old state" for the
        // extradata items at all (unlike all other settings), so the old/new
        // settings comparison would give a wrong result!
        i_saveSettings(&fNeedsGlobalSaveSettings, SaveS_Force);

        if (fNeedsGlobalSaveSettings)
        {
            // save the global settings; for that we should hold only the VirtualBox lock
            alock.release();
            AutoWriteLock vboxlock(mParent COMMA_LOCKVAL_SRC_POS);
            mParent->i_saveSettings();
        }
    }

    // fire notification outside the lock
    if (fChanged)
        mParent->i_onExtraDataChange(mData->mUuid, Bstr(aKey).raw(), Bstr(aValue).raw());

    return S_OK;
}

HRESULT Machine::setSettingsFilePath(const com::Utf8Str &aSettingsFilePath, ComPtr<IProgress> &aProgress)
{
    aProgress = NULL;
    NOREF(aSettingsFilePath);
    ReturnComNotImplemented();
}

HRESULT Machine::saveSettings()
{
    AutoWriteLock mlock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (FAILED(rc)) return rc;

    /* the settings file path may never be null */
    ComAssertRet(!mData->m_strConfigFileFull.isEmpty(), E_FAIL);

    /* save all VM data excluding snapshots */
    bool fNeedsGlobalSaveSettings = false;
    rc = i_saveSettings(&fNeedsGlobalSaveSettings);
    mlock.release();

    if (SUCCEEDED(rc) && fNeedsGlobalSaveSettings)
    {
        // save the global settings; for that we should hold only the VirtualBox lock
        AutoWriteLock vlock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}


HRESULT Machine::discardSettings()
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (FAILED(rc)) return rc;

    /*
     *  during this rollback, the session will be notified if data has
     *  been actually changed
     */
    i_rollback(true /* aNotify */);

    return S_OK;
}

/** @note Locks objects! */
HRESULT Machine::unregister(AutoCaller &autoCaller,
                            CleanupMode_T aCleanupMode,
                            std::vector<ComPtr<IMedium> > &aMedia)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    Guid id(i_getId());

    if (mData->mSession.mState != SessionState_Unlocked)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("Cannot unregister the machine '%s' while it is locked"),
                        mUserData->s.strName.c_str());

    // wait for state dependents to drop to zero
    i_ensureNoStateDependencies();

    if (!mData->mAccessible)
    {
        // inaccessible maschines can only be unregistered; uninitialize ourselves
        // here because currently there may be no unregistered that are inaccessible
        // (this state combination is not supported). Note releasing the caller and
        // leaving the lock before calling uninit()
        alock.release();
        autoCaller.release();

        uninit();

        mParent->i_unregisterMachine(this, id);
            // calls VirtualBox::i_saveSettings()

        return S_OK;
    }

    HRESULT rc = S_OK;

    /// @todo r=klaus this is stupid... why is the saved state always deleted?
    // discard saved state
    if (mData->mMachineState == MachineState_Saved)
    {
        // add the saved state file to the list of files the caller should delete
        Assert(!mSSData->strStateFilePath.isEmpty());
        mData->llFilesToDelete.push_back(mSSData->strStateFilePath);

        mSSData->strStateFilePath.setNull();

        // unconditionally set the machine state to powered off, we now
        // know no session has locked the machine
        mData->mMachineState = MachineState_PoweredOff;
    }

    size_t cSnapshots = 0;
    if (mData->mFirstSnapshot)
        cSnapshots = mData->mFirstSnapshot->i_getAllChildrenCount() + 1;
    if (cSnapshots && aCleanupMode == CleanupMode_UnregisterOnly)
        // fail now before we start detaching media
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("Cannot unregister the machine '%s' because it has %d snapshots"),
                           mUserData->s.strName.c_str(), cSnapshots);

    // This list collects the medium objects from all medium attachments
    // which we will detach from the machine and its snapshots, in a specific
    // order which allows for closing all media without getting "media in use"
    // errors, simply by going through the list from the front to the back:
    // 1) first media from machine attachments (these have the "leaf" attachments with snapshots
    //    and must be closed before the parent media from the snapshots, or closing the parents
    //    will fail because they still have children);
    // 2) media from the youngest snapshots followed by those from the parent snapshots until
    //    the root ("first") snapshot of the machine.
    MediaList llMedia;

    if (    !mMediumAttachments.isNull()    // can be NULL if machine is inaccessible
         && mMediumAttachments->size()
       )
    {
        // we have media attachments: detach them all and add the Medium objects to our list
        if (aCleanupMode != CleanupMode_UnregisterOnly)
            i_detachAllMedia(alock, NULL /* pSnapshot */, aCleanupMode, llMedia);
        else
            return setError(VBOX_E_INVALID_OBJECT_STATE,
                            tr("Cannot unregister the machine '%s' because it has %d media attachments"),
                            mUserData->s.strName.c_str(), mMediumAttachments->size());
    }

    if (cSnapshots)
    {
        // add the media from the medium attachments of the snapshots to llMedia
        // as well, after the "main" machine media; Snapshot::uninitRecursively()
        // calls Machine::detachAllMedia() for the snapshot machine, recursing
        // into the children first

        // Snapshot::beginDeletingSnapshot() asserts if the machine state is not this
        MachineState_T oldState = mData->mMachineState;
        mData->mMachineState = MachineState_DeletingSnapshot;

        // make a copy of the first snapshot so the refcount does not drop to 0
        // in beginDeletingSnapshot, which sets pFirstSnapshot to 0 (that hangs
        // because of the AutoCaller voodoo)
        ComObjPtr<Snapshot> pFirstSnapshot = mData->mFirstSnapshot;

        // GO!
        pFirstSnapshot->i_uninitRecursively(alock, aCleanupMode, llMedia, mData->llFilesToDelete);

        mData->mMachineState = oldState;
    }

    if (FAILED(rc))
    {
        i_rollbackMedia();
        return rc;
    }

    // commit all the media changes made above
    i_commitMedia();

    mData->mRegistered = false;

    // machine lock no longer needed
    alock.release();

    // return media to caller
    aMedia.resize(llMedia.size());
    size_t i = 0;
    for (MediaList::const_iterator
         it = llMedia.begin();
         it != llMedia.end();
         ++it, ++i)
        (*it).queryInterfaceTo(aMedia[i].asOutParam());

    mParent->i_unregisterMachine(this, id);
            // calls VirtualBox::i_saveSettings() and VirtualBox::saveModifiedRegistries()

    return S_OK;
}

/**
 * Task record for deleting a machine config.
 */
class Machine::DeleteConfigTask
    : public Machine::Task
{
public:
    DeleteConfigTask(Machine *m,
                     Progress *p,
                     const Utf8Str &t,
                     const RTCList<ComPtr<IMedium> > &llMediums,
                     const StringsList &llFilesToDelete)
        : Task(m, p, t),
          m_llMediums(llMediums),
          m_llFilesToDelete(llFilesToDelete)
    {}

private:
    void handler()
    {
        try
        {
            m_pMachine->i_deleteConfigHandler(*this);
        }
        catch (...)
        {
            LogRel(("Some exception in the function Machine::i_deleteConfigHandler()\n"));
        }
    }

    RTCList<ComPtr<IMedium> >   m_llMediums;
    StringsList                 m_llFilesToDelete;

    friend void Machine::i_deleteConfigHandler(DeleteConfigTask &task);
};

/**
 * Task thread implementation for SessionMachine::DeleteConfig(), called from
 * SessionMachine::taskHandler().
 *
 * @note Locks this object for writing.
 *
 * @param task
 * @return
 */
void Machine::i_deleteConfigHandler(DeleteConfigTask &task)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    LogFlowThisFunc(("state=%d\n", getObjectState().getState()));
    if (FAILED(autoCaller.rc()))
    {
        /* we might have been uninitialized because the session was accidentally
         * closed by the client, so don't assert */
        HRESULT rc = setError(E_FAIL,
                              tr("The session has been accidentally closed"));
        task.m_pProgress->i_notifyComplete(rc);
        LogFlowThisFuncLeave();
        return;
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    try
    {
        ULONG uLogHistoryCount = 3;
        ComPtr<ISystemProperties> systemProperties;
        rc = mParent->COMGETTER(SystemProperties)(systemProperties.asOutParam());
        if (FAILED(rc)) throw rc;

        if (!systemProperties.isNull())
        {
            rc = systemProperties->COMGETTER(LogHistoryCount)(&uLogHistoryCount);
            if (FAILED(rc)) throw rc;
        }

        MachineState_T oldState = mData->mMachineState;
        i_setMachineState(MachineState_SettingUp);
        alock.release();
        for (size_t i = 0; i < task.m_llMediums.size(); ++i)
        {
            ComObjPtr<Medium> pMedium = (Medium*)(IMedium*)(task.m_llMediums.at(i));
            {
                AutoCaller mac(pMedium);
                if (FAILED(mac.rc())) throw mac.rc();
                Utf8Str strLocation = pMedium->i_getLocationFull();
                LogFunc(("Deleting file %s\n", strLocation.c_str()));
                rc = task.m_pProgress->SetNextOperation(BstrFmt(tr("Deleting '%s'"), strLocation.c_str()).raw(), 1);
                if (FAILED(rc)) throw rc;
            }
            if (pMedium->i_isMediumFormatFile())
            {
                ComPtr<IProgress> pProgress2;
                rc = pMedium->DeleteStorage(pProgress2.asOutParam());
                if (FAILED(rc)) throw rc;
                rc = task.m_pProgress->WaitForAsyncProgressCompletion(pProgress2);
                if (FAILED(rc)) throw rc;
                /* Check the result of the asynchronous process. */
                LONG iRc;
                rc = pProgress2->COMGETTER(ResultCode)(&iRc);
                if (FAILED(rc)) throw rc;
                /* If the thread of the progress object has an error, then
                 * retrieve the error info from there, or it'll be lost. */
                if (FAILED(iRc))
                    throw setError(ProgressErrorInfo(pProgress2));
            }

            /* Close the medium, deliberately without checking the return
             * code, and without leaving any trace in the error info, as
             * a failure here is a very minor issue, which shouldn't happen
             * as above we even managed to delete the medium. */
            {
                ErrorInfoKeeper eik;
                pMedium->Close();
            }
        }
        i_setMachineState(oldState);
        alock.acquire();

        // delete the files pushed on the task list by Machine::Delete()
        // (this includes saved states of the machine and snapshots and
        // medium storage files from the IMedium list passed in, and the
        // machine XML file)
        for (StringsList::const_iterator
             it = task.m_llFilesToDelete.begin();
             it != task.m_llFilesToDelete.end();
             ++it)
        {
            const Utf8Str &strFile = *it;
            LogFunc(("Deleting file %s\n", strFile.c_str()));
            rc = task.m_pProgress->SetNextOperation(BstrFmt(tr("Deleting '%s'"), it->c_str()).raw(), 1);
            if (FAILED(rc)) throw rc;

            int vrc = RTFileDelete(strFile.c_str());
            if (RT_FAILURE(vrc))
                throw setError(VBOX_E_IPRT_ERROR,
                               tr("Could not delete file '%s' (%Rrc)"), strFile.c_str(), vrc);
        }

        rc = task.m_pProgress->SetNextOperation(Bstr(tr("Cleaning up machine directory")).raw(), 1);
        if (FAILED(rc)) throw rc;

        /* delete the settings only when the file actually exists */
        if (mData->pMachineConfigFile->fileExists())
        {
            /* Delete any backup or uncommitted XML files. Ignore failures.
               See the fSafe parameter of xml::XmlFileWriter::write for details. */
            /** @todo Find a way to avoid referring directly to iprt/xml.h here. */
            Utf8Str otherXml = Utf8StrFmt("%s%s", mData->m_strConfigFileFull.c_str(), xml::XmlFileWriter::s_pszTmpSuff);
            RTFileDelete(otherXml.c_str());
            otherXml = Utf8StrFmt("%s%s", mData->m_strConfigFileFull.c_str(), xml::XmlFileWriter::s_pszPrevSuff);
            RTFileDelete(otherXml.c_str());

            /* delete the Logs folder, nothing important should be left
             * there (we don't check for errors because the user might have
             * some private files there that we don't want to delete) */
            Utf8Str logFolder;
            getLogFolder(logFolder);
            Assert(logFolder.length());
            if (RTDirExists(logFolder.c_str()))
            {
                /* Delete all VBox.log[.N] files from the Logs folder
                 * (this must be in sync with the rotation logic in
                 * Console::powerUpThread()). Also, delete the VBox.png[.N]
                 * files that may have been created by the GUI. */
                Utf8Str log = Utf8StrFmt("%s%cVBox.log",
                                         logFolder.c_str(), RTPATH_DELIMITER);
                RTFileDelete(log.c_str());
                log = Utf8StrFmt("%s%cVBox.png",
                                 logFolder.c_str(), RTPATH_DELIMITER);
                RTFileDelete(log.c_str());
                for (int i = uLogHistoryCount; i > 0; i--)
                {
                    log = Utf8StrFmt("%s%cVBox.log.%d",
                                     logFolder.c_str(), RTPATH_DELIMITER, i);
                    RTFileDelete(log.c_str());
                    log = Utf8StrFmt("%s%cVBox.png.%d",
                                     logFolder.c_str(), RTPATH_DELIMITER, i);
                    RTFileDelete(log.c_str());
                }
#if defined(RT_OS_WINDOWS)
                log = Utf8StrFmt("%s%cVBoxStartup.log", logFolder.c_str(), RTPATH_DELIMITER);
                RTFileDelete(log.c_str());
                log = Utf8StrFmt("%s%cVBoxHardening.log", logFolder.c_str(), RTPATH_DELIMITER);
                RTFileDelete(log.c_str());
#endif

                RTDirRemove(logFolder.c_str());
            }

            /* delete the Snapshots folder, nothing important should be left
             * there (we don't check for errors because the user might have
             * some private files there that we don't want to delete) */
            Utf8Str strFullSnapshotFolder;
            i_calculateFullPath(mUserData->s.strSnapshotFolder, strFullSnapshotFolder);
            Assert(!strFullSnapshotFolder.isEmpty());
            if (RTDirExists(strFullSnapshotFolder.c_str()))
                RTDirRemove(strFullSnapshotFolder.c_str());

            // delete the directory that contains the settings file, but only
            // if it matches the VM name
            Utf8Str settingsDir;
            if (i_isInOwnDir(&settingsDir))
                RTDirRemove(settingsDir.c_str());
        }

        alock.release();

        mParent->i_saveModifiedRegistries();
    }
    catch (HRESULT aRC) { rc = aRC; }

    task.m_pProgress->i_notifyComplete(rc);

    LogFlowThisFuncLeave();
}

HRESULT Machine::deleteConfig(const std::vector<ComPtr<IMedium> > &aMedia, ComPtr<IProgress> &aProgress)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (mData->mRegistered)
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Cannot delete settings of a registered machine"));

    // collect files to delete
    StringsList llFilesToDelete(mData->llFilesToDelete);    // saved states pushed here by Unregister()
    if (mData->pMachineConfigFile->fileExists())
        llFilesToDelete.push_back(mData->m_strConfigFileFull);

    RTCList<ComPtr<IMedium> > llMediums;
    for (size_t i = 0; i < aMedia.size(); ++i)
    {
        IMedium *pIMedium(aMedia[i]);
        ComObjPtr<Medium> pMedium = static_cast<Medium*>(pIMedium);
        if (pMedium.isNull())
            return setError(E_INVALIDARG, "The given medium pointer with index %d is invalid", i);
        SafeArray<BSTR> ids;
        rc = pMedium->COMGETTER(MachineIds)(ComSafeArrayAsOutParam(ids));
        if (FAILED(rc)) return rc;
        /* At this point the medium should not have any back references
         * anymore. If it has it is attached to another VM and *must* not
         * deleted. */
        if (ids.size() < 1)
            llMediums.append(pMedium);
    }

    ComObjPtr<Progress> pProgress;
    pProgress.createObject();
    rc = pProgress->init(i_getVirtualBox(),
                         static_cast<IMachine*>(this) /* aInitiator */,
                         tr("Deleting files"),
                         true /* fCancellable */,
                         (ULONG)(1 + llMediums.size() + llFilesToDelete.size() + 1),    // cOperations
                         tr("Collecting file inventory"));
    if (FAILED(rc))
        return rc;

    /* create and start the task on a separate thread (note that it will not
     * start working until we release alock) */
    DeleteConfigTask *pTask = new DeleteConfigTask(this, pProgress, "DeleteVM", llMediums, llFilesToDelete);
    rc = pTask->createThread();
    if (FAILED(rc))
        return rc;

    pProgress.queryInterfaceTo(aProgress.asOutParam());

    LogFlowFuncLeave();

    return S_OK;
}

HRESULT Machine::findSnapshot(const com::Utf8Str &aNameOrId, ComPtr<ISnapshot> &aSnapshot)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<Snapshot> pSnapshot;
    HRESULT rc;

    if (aNameOrId.isEmpty())
        // null case (caller wants root snapshot): i_findSnapshotById() handles this
        rc = i_findSnapshotById(Guid(), pSnapshot, true /* aSetError */);
    else
    {
        Guid uuid(aNameOrId);
        if (uuid.isValid())
            rc = i_findSnapshotById(uuid, pSnapshot, true /* aSetError */);
        else
            rc = i_findSnapshotByName(aNameOrId, pSnapshot, true /* aSetError */);
    }
    pSnapshot.queryInterfaceTo(aSnapshot.asOutParam());

    return rc;
}

HRESULT Machine::createSharedFolder(const com::Utf8Str &aName, const com::Utf8Str &aHostPath, BOOL  aWritable, BOOL  aAutomount)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrRunningStateDep);
    if (FAILED(rc)) return rc;

    ComObjPtr<SharedFolder> sharedFolder;
    rc = i_findSharedFolder(aName, sharedFolder, false /* aSetError */);
    if (SUCCEEDED(rc))
        return setError(VBOX_E_OBJECT_IN_USE,
                        tr("Shared folder named '%s' already exists"),
                        aName.c_str());

    sharedFolder.createObject();
    rc = sharedFolder->init(i_getMachine(),
                            aName,
                            aHostPath,
                            !!aWritable,
                            !!aAutomount,
                            true /* fFailOnError */);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_SharedFolders);
    mHWData.backup();
    mHWData->mSharedFolders.push_back(sharedFolder);

    /* inform the direct session if any */
    alock.release();
    i_onSharedFolderChange();

    return S_OK;
}

HRESULT Machine::removeSharedFolder(const com::Utf8Str &aName)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrRunningStateDep);
    if (FAILED(rc)) return rc;

    ComObjPtr<SharedFolder> sharedFolder;
    rc = i_findSharedFolder(aName, sharedFolder, true /* aSetError */);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_SharedFolders);
    mHWData.backup();
    mHWData->mSharedFolders.remove(sharedFolder);

    /* inform the direct session if any */
    alock.release();
    i_onSharedFolderChange();

    return S_OK;
}

HRESULT Machine::canShowConsoleWindow(BOOL *aCanShow)
{
    /* start with No */
    *aCanShow = FALSE;

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (mData->mSession.mState != SessionState_Locked)
            return setError(VBOX_E_INVALID_VM_STATE,
                            tr("Machine is not locked for session (session state: %s)"),
                            Global::stringifySessionState(mData->mSession.mState));

        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore calls made after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    LONG64 dummy;
    return directControl->OnShowWindow(TRUE /* aCheck */, aCanShow, &dummy);
}

HRESULT Machine::showConsoleWindow(LONG64 *aWinId)
{
    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (mData->mSession.mState != SessionState_Locked)
            return setError(E_FAIL,
                            tr("Machine is not locked for session (session state: %s)"),
                            Global::stringifySessionState(mData->mSession.mState));

        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore calls made after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    BOOL dummy;
    return directControl->OnShowWindow(FALSE /* aCheck */, &dummy, aWinId);
}

#ifdef VBOX_WITH_GUEST_PROPS
/**
 * Look up a guest property in VBoxSVC's internal structures.
 */
HRESULT Machine::i_getGuestPropertyFromService(const com::Utf8Str &aName,
                                               com::Utf8Str &aValue,
                                               LONG64 *aTimestamp,
                                               com::Utf8Str &aFlags) const
{
    using namespace guestProp;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    HWData::GuestPropertyMap::const_iterator it = mHWData->mGuestProperties.find(aName);

    if (it != mHWData->mGuestProperties.end())
    {
        char szFlags[MAX_FLAGS_LEN + 1];
        aValue = it->second.strValue;
        *aTimestamp = it->second.mTimestamp;
        writeFlags(it->second.mFlags, szFlags);
        aFlags = Utf8Str(szFlags);
    }

    return S_OK;
}

/**
 * Query the VM that a guest property belongs to for the property.
 * @returns E_ACCESSDENIED if the VM process is not available or not
 *          currently handling queries and the lookup should then be done in
 *          VBoxSVC.
 */
HRESULT Machine::i_getGuestPropertyFromVM(const com::Utf8Str &aName,
                                          com::Utf8Str &aValue,
                                          LONG64 *aTimestamp,
                                          com::Utf8Str &aFlags) const
{
    HRESULT rc = S_OK;
    BSTR bValue = NULL;
    BSTR bFlags = NULL;

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore calls made after #OnSessionEnd() is called */
    if (!directControl)
        rc = E_ACCESSDENIED;
    else
        rc = directControl->AccessGuestProperty(Bstr(aName).raw(), Bstr::Empty.raw(), Bstr::Empty.raw(),
                                                0 /* accessMode */,
                                                &bValue, aTimestamp, &bFlags);

    aValue = bValue;
    aFlags = bFlags;

    return rc;
}
#endif // VBOX_WITH_GUEST_PROPS

HRESULT Machine::getGuestProperty(const com::Utf8Str &aName,
                                  com::Utf8Str &aValue,
                                  LONG64 *aTimestamp,
                                  com::Utf8Str &aFlags)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else // VBOX_WITH_GUEST_PROPS

    HRESULT rc = i_getGuestPropertyFromVM(aName, aValue, aTimestamp, aFlags);

    if (rc == E_ACCESSDENIED)
        /* The VM is not running or the service is not (yet) accessible */
        rc = i_getGuestPropertyFromService(aName, aValue, aTimestamp, aFlags);
    return rc;
#endif // VBOX_WITH_GUEST_PROPS
}

HRESULT Machine::getGuestPropertyValue(const com::Utf8Str &aProperty, com::Utf8Str &aValue)
{
    LONG64 dummyTimestamp;
    com::Utf8Str dummyFlags;
    HRESULT rc = getGuestProperty(aProperty, aValue, &dummyTimestamp, dummyFlags);
    return rc;

}
HRESULT Machine::getGuestPropertyTimestamp(const com::Utf8Str &aProperty, LONG64 *aValue)
{
    com::Utf8Str dummyFlags;
    com::Utf8Str dummyValue;
    HRESULT rc = getGuestProperty(aProperty, dummyValue, aValue, dummyFlags);
    return rc;
}

#ifdef VBOX_WITH_GUEST_PROPS
/**
 * Set a guest property in VBoxSVC's internal structures.
 */
HRESULT Machine::i_setGuestPropertyToService(const com::Utf8Str &aName, const com::Utf8Str &aValue,
                                             const com::Utf8Str &aFlags, bool fDelete)
{
    using namespace guestProp;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = S_OK;

    rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    try
    {
        uint32_t fFlags = NILFLAG;
        if (aFlags.length() && RT_FAILURE(validateFlags(aFlags.c_str(), &fFlags)))
            return setError(E_INVALIDARG, tr("Invalid guest property flag values: '%s'"), aFlags.c_str());

        HWData::GuestPropertyMap::iterator it = mHWData->mGuestProperties.find(aName);
        if (it == mHWData->mGuestProperties.end())
        {
            if (!fDelete)
            {
                i_setModified(IsModified_MachineData);
                mHWData.backupEx();

                RTTIMESPEC time;
                HWData::GuestProperty prop;
                prop.strValue   = Bstr(aValue).raw();
                prop.mTimestamp = RTTimeSpecGetNano(RTTimeNow(&time));
                prop.mFlags     = fFlags;
                mHWData->mGuestProperties[aName] = prop;
            }
        }
        else
        {
            if (it->second.mFlags & (RDONLYHOST))
            {
                rc = setError(E_ACCESSDENIED, tr("The property '%s' cannot be changed by the host"), aName.c_str());
            }
            else
            {
                i_setModified(IsModified_MachineData);
                mHWData.backupEx();

                /* The backupEx() operation invalidates our iterator,
                 * so get a new one. */
                it = mHWData->mGuestProperties.find(aName);
                Assert(it != mHWData->mGuestProperties.end());

                if (!fDelete)
                {
                    RTTIMESPEC time;
                    it->second.strValue   = aValue;
                    it->second.mTimestamp = RTTimeSpecGetNano(RTTimeNow(&time));
                    it->second.mFlags     = fFlags;
                }
                else
                    mHWData->mGuestProperties.erase(it);
            }
        }

        if (SUCCEEDED(rc))
        {
            alock.release();

            mParent->i_onGuestPropertyChange(mData->mUuid,
                                             Bstr(aName).raw(),
                                             Bstr(aValue).raw(),
                                             Bstr(aFlags).raw());
        }
    }
    catch (std::bad_alloc &)
    {
        rc = E_OUTOFMEMORY;
    }

    return rc;
}

/**
 * Set a property on the VM that that property belongs to.
 * @returns E_ACCESSDENIED if the VM process is not available or not
 *          currently handling queries and the setting should then be done in
 *          VBoxSVC.
 */
HRESULT Machine::i_setGuestPropertyToVM(const com::Utf8Str &aName, const com::Utf8Str &aValue,
                                        const com::Utf8Str &aFlags, bool fDelete)
{
    HRESULT rc;

    try
    {
        ComPtr<IInternalSessionControl> directControl;
        {
            AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
            if (mData->mSession.mLockType == LockType_VM)
                directControl = mData->mSession.mDirectControl;
        }

        BSTR dummy = NULL; /* will not be changed (setter) */
        LONG64 dummy64;
        if (!directControl)
            rc = E_ACCESSDENIED;
        else
            /** @todo Fix when adding DeleteGuestProperty(), see defect. */
            rc = directControl->AccessGuestProperty(Bstr(aName).raw(), Bstr(aValue).raw(), Bstr(aFlags).raw(),
                                                    fDelete? 2: 1 /* accessMode */,
                                                    &dummy, &dummy64, &dummy);
    }
    catch (std::bad_alloc &)
    {
        rc = E_OUTOFMEMORY;
    }

    return rc;
}
#endif // VBOX_WITH_GUEST_PROPS

HRESULT Machine::setGuestProperty(const com::Utf8Str &aProperty, const com::Utf8Str &aValue,
                                  const com::Utf8Str &aFlags)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else // VBOX_WITH_GUEST_PROPS
    HRESULT rc = i_setGuestPropertyToVM(aProperty, aValue, aFlags, /* fDelete = */ false);
    if (rc == E_ACCESSDENIED)
        /* The VM is not running or the service is not (yet) accessible */
        rc = i_setGuestPropertyToService(aProperty, aValue, aFlags, /* fDelete = */ false);
    return rc;
#endif // VBOX_WITH_GUEST_PROPS
}

HRESULT Machine::setGuestPropertyValue(const com::Utf8Str &aProperty, const com::Utf8Str &aValue)
{
    return setGuestProperty(aProperty, aValue, "");
}

HRESULT Machine::deleteGuestProperty(const com::Utf8Str &aName)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else // VBOX_WITH_GUEST_PROPS
    HRESULT rc = i_setGuestPropertyToVM(aName, "", "", /* fDelete = */ true);
    if (rc == E_ACCESSDENIED)
        /* The VM is not running or the service is not (yet) accessible */
        rc = i_setGuestPropertyToService(aName, "", "", /* fDelete = */ true);
    return rc;
#endif // VBOX_WITH_GUEST_PROPS
}

#ifdef VBOX_WITH_GUEST_PROPS
/**
 * Enumerate the guest properties in VBoxSVC's internal structures.
 */
HRESULT Machine::i_enumerateGuestPropertiesInService(const com::Utf8Str &aPatterns,
                                                     std::vector<com::Utf8Str> &aNames,
                                                     std::vector<com::Utf8Str> &aValues,
                                                     std::vector<LONG64> &aTimestamps,
                                                     std::vector<com::Utf8Str> &aFlags)
{
    using namespace guestProp;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    Utf8Str strPatterns(aPatterns);

    HWData::GuestPropertyMap propMap;

    /*
     * Look for matching patterns and build up a list.
     */
    for (HWData::GuestPropertyMap::const_iterator
         it = mHWData->mGuestProperties.begin();
         it != mHWData->mGuestProperties.end();
         ++it)
    {
        if (   strPatterns.isEmpty()
            || RTStrSimplePatternMultiMatch(strPatterns.c_str(),
                                            RTSTR_MAX,
                                            it->first.c_str(),
                                            RTSTR_MAX,
                                            NULL)
           )
            propMap.insert(*it);
    }

    alock.release();

    /*
     * And build up the arrays for returning the property information.
     */
    size_t cEntries = propMap.size();

    aNames.resize(cEntries);
    aValues.resize(cEntries);
    aTimestamps.resize(cEntries);
    aFlags.resize(cEntries);

    char szFlags[MAX_FLAGS_LEN + 1];
    size_t i = 0;
    for (HWData::GuestPropertyMap::const_iterator
         it = propMap.begin();
         it != propMap.end();
         ++it, ++i)
    {
        aNames[i] = it->first;
        aValues[i] = it->second.strValue;
        aTimestamps[i] = it->second.mTimestamp;
        writeFlags(it->second.mFlags, szFlags);
        aFlags[i] = Utf8Str(szFlags);
    }

    return S_OK;
}

/**
 * Enumerate the properties managed by a VM.
 * @returns E_ACCESSDENIED if the VM process is not available or not
 *          currently handling queries and the setting should then be done in
 *          VBoxSVC.
 */
HRESULT Machine::i_enumerateGuestPropertiesOnVM(const com::Utf8Str &aPatterns,
                                                std::vector<com::Utf8Str> &aNames,
                                                std::vector<com::Utf8Str> &aValues,
                                                std::vector<LONG64> &aTimestamps,
                                                std::vector<com::Utf8Str> &aFlags)
{
    HRESULT rc;
    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    com::SafeArray<BSTR> bNames;
    com::SafeArray<BSTR> bValues;
    com::SafeArray<LONG64> bTimestamps;
    com::SafeArray<BSTR> bFlags;

    if (!directControl)
        rc = E_ACCESSDENIED;
    else
        rc = directControl->EnumerateGuestProperties(Bstr(aPatterns).raw(),
                                                     ComSafeArrayAsOutParam(bNames),
                                                     ComSafeArrayAsOutParam(bValues),
                                                     ComSafeArrayAsOutParam(bTimestamps),
                                                     ComSafeArrayAsOutParam(bFlags));
    size_t i;
    aNames.resize(bNames.size());
    for (i = 0; i < bNames.size(); ++i)
        aNames[i] = Utf8Str(bNames[i]);
    aValues.resize(bValues.size());
    for (i = 0; i < bValues.size(); ++i)
        aValues[i] = Utf8Str(bValues[i]);
    aTimestamps.resize(bTimestamps.size());
    for (i = 0; i < bTimestamps.size(); ++i)
        aTimestamps[i] = bTimestamps[i];
    aFlags.resize(bFlags.size());
    for (i = 0; i < bFlags.size(); ++i)
        aFlags[i] = Utf8Str(bFlags[i]);

    return rc;
}
#endif // VBOX_WITH_GUEST_PROPS
HRESULT Machine::enumerateGuestProperties(const com::Utf8Str &aPatterns,
                                          std::vector<com::Utf8Str> &aNames,
                                          std::vector<com::Utf8Str> &aValues,
                                          std::vector<LONG64> &aTimestamps,
                                          std::vector<com::Utf8Str> &aFlags)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else // VBOX_WITH_GUEST_PROPS

    HRESULT rc = i_enumerateGuestPropertiesOnVM(aPatterns, aNames, aValues, aTimestamps, aFlags);

    if (rc == E_ACCESSDENIED)
        /* The VM is not running or the service is not (yet) accessible */
        rc = i_enumerateGuestPropertiesInService(aPatterns, aNames, aValues, aTimestamps, aFlags);
    return rc;
#endif // VBOX_WITH_GUEST_PROPS
}

HRESULT Machine::getMediumAttachmentsOfController(const com::Utf8Str &aName,
                                                  std::vector<ComPtr<IMediumAttachment> > &aMediumAttachments)
{
    MediumAttachmentList atts;

    HRESULT rc = i_getMediumAttachmentsOfController(aName, atts);
    if (FAILED(rc)) return rc;

    aMediumAttachments.resize(atts.size());
    size_t i = 0;
    for (MediumAttachmentList::const_iterator
         it = atts.begin();
         it != atts.end();
         ++it, ++i)
        (*it).queryInterfaceTo(aMediumAttachments[i].asOutParam());

    return S_OK;
}

HRESULT Machine::getMediumAttachment(const com::Utf8Str &aName,
                                     LONG aControllerPort,
                                     LONG aDevice,
                                     ComPtr<IMediumAttachment> &aAttachment)
{
    LogFlowThisFunc(("aControllerName=\"%s\" aControllerPort=%d aDevice=%d\n",
                     aName.c_str(), aControllerPort, aDevice));

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aAttachment = NULL;

    ComObjPtr<MediumAttachment> pAttach = i_findAttachment(*mMediumAttachments.data(),
                                                           aName,
                                                           aControllerPort,
                                                           aDevice);
    if (pAttach.isNull())
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("No storage device attached to device slot %d on port %d of controller '%s'"),
                        aDevice, aControllerPort, aName.c_str());

    pAttach.queryInterfaceTo(aAttachment.asOutParam());

    return S_OK;
}


HRESULT Machine::addStorageController(const com::Utf8Str &aName,
                                      StorageBus_T aConnectionType,
                                      ComPtr<IStorageController> &aController)
{
    if (   (aConnectionType <= StorageBus_Null)
        || (aConnectionType >  StorageBus_PCIe))
        return setError(E_INVALIDARG,
                        tr("Invalid connection type: %d"),
                        aConnectionType);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /* try to find one with the name first. */
    ComObjPtr<StorageController> ctrl;

    rc = i_getStorageControllerByName(aName, ctrl, false /* aSetError */);
    if (SUCCEEDED(rc))
        return setError(VBOX_E_OBJECT_IN_USE,
                        tr("Storage controller named '%s' already exists"),
                        aName.c_str());

    ctrl.createObject();

    /* get a new instance number for the storage controller */
    ULONG ulInstance = 0;
    bool fBootable = true;
    for (StorageControllerList::const_iterator
         it = mStorageControllers->begin();
         it != mStorageControllers->end();
         ++it)
    {
        if ((*it)->i_getStorageBus() == aConnectionType)
        {
            ULONG ulCurInst = (*it)->i_getInstance();

            if (ulCurInst >= ulInstance)
                ulInstance = ulCurInst + 1;

            /* Only one controller of each type can be marked as bootable. */
            if ((*it)->i_getBootable())
                fBootable = false;
        }
    }

    rc = ctrl->init(this, aName, aConnectionType, ulInstance, fBootable);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_Storage);
    mStorageControllers.backup();
    mStorageControllers->push_back(ctrl);

    ctrl.queryInterfaceTo(aController.asOutParam());

    /* inform the direct session if any */
    alock.release();
    i_onStorageControllerChange();

    return S_OK;
}

HRESULT Machine::getStorageControllerByName(const com::Utf8Str &aName,
                                            ComPtr<IStorageController> &aStorageController)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<StorageController> ctrl;

    HRESULT rc = i_getStorageControllerByName(aName, ctrl, true /* aSetError */);
    if (SUCCEEDED(rc))
        ctrl.queryInterfaceTo(aStorageController.asOutParam());

    return rc;
}

HRESULT Machine::getStorageControllerByInstance(StorageBus_T aConnectionType,
                                                ULONG aInstance,
                                                ComPtr<IStorageController> &aStorageController)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (StorageControllerList::const_iterator
         it = mStorageControllers->begin();
         it != mStorageControllers->end();
         ++it)
    {
        if (   (*it)->i_getStorageBus() == aConnectionType
            && (*it)->i_getInstance() == aInstance)
        {
            (*it).queryInterfaceTo(aStorageController.asOutParam());
            return S_OK;
        }
    }

    return setError(VBOX_E_OBJECT_NOT_FOUND,
                    tr("Could not find a storage controller with instance number '%lu'"),
                    aInstance);
}

HRESULT Machine::setStorageControllerBootable(const com::Utf8Str &aName, BOOL aBootable)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    ComObjPtr<StorageController> ctrl;

    rc = i_getStorageControllerByName(aName, ctrl, true /* aSetError */);
    if (SUCCEEDED(rc))
    {
        /* Ensure that only one controller of each type is marked as bootable. */
        if (aBootable == TRUE)
        {
            for (StorageControllerList::const_iterator
                 it = mStorageControllers->begin();
                 it != mStorageControllers->end();
                 ++it)
            {
                ComObjPtr<StorageController> aCtrl = (*it);

                if (   (aCtrl->i_getName() != aName)
                    && aCtrl->i_getBootable() == TRUE
                    && aCtrl->i_getStorageBus() == ctrl->i_getStorageBus()
                    && aCtrl->i_getControllerType() == ctrl->i_getControllerType())
                {
                    aCtrl->i_setBootable(FALSE);
                    break;
                }
            }
        }

        if (SUCCEEDED(rc))
        {
            ctrl->i_setBootable(aBootable);
            i_setModified(IsModified_Storage);
        }
    }

    if (SUCCEEDED(rc))
    {
        /* inform the direct session if any */
        alock.release();
        i_onStorageControllerChange();
    }

    return rc;
}

HRESULT Machine::removeStorageController(const com::Utf8Str &aName)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    ComObjPtr<StorageController> ctrl;
    rc = i_getStorageControllerByName(aName, ctrl, true /* aSetError */);
    if (FAILED(rc)) return rc;

    {
        /* find all attached devices to the appropriate storage controller and detach them all */
        // make a temporary list because detachDevice invalidates iterators into
        // mMediumAttachments
        MediumAttachmentList llAttachments2 = *mMediumAttachments.data();

        for (MediumAttachmentList::const_iterator
             it = llAttachments2.begin();
             it != llAttachments2.end();
             ++it)
        {
            MediumAttachment *pAttachTemp = *it;

            AutoCaller localAutoCaller(pAttachTemp);
            if (FAILED(localAutoCaller.rc())) return localAutoCaller.rc();

            AutoReadLock local_alock(pAttachTemp COMMA_LOCKVAL_SRC_POS);

            if (pAttachTemp->i_getControllerName() == aName)
            {
                rc = i_detachDevice(pAttachTemp, alock, NULL);
                if (FAILED(rc)) return rc;
            }
        }
    }

    /* We can remove it now. */
    i_setModified(IsModified_Storage);
    mStorageControllers.backup();

    ctrl->i_unshare();

    mStorageControllers->remove(ctrl);

    /* inform the direct session if any */
    alock.release();
    i_onStorageControllerChange();

    return S_OK;
}

HRESULT Machine::addUSBController(const com::Utf8Str &aName, USBControllerType_T aType,
                                  ComPtr<IUSBController> &aController)
{
    if (   (aType <= USBControllerType_Null)
        || (aType >= USBControllerType_Last))
        return setError(E_INVALIDARG,
                        tr("Invalid USB controller type: %d"),
                        aType);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    /* try to find one with the same type first. */
    ComObjPtr<USBController> ctrl;

    rc = i_getUSBControllerByName(aName, ctrl, false /* aSetError */);
    if (SUCCEEDED(rc))
        return setError(VBOX_E_OBJECT_IN_USE,
                        tr("USB controller named '%s' already exists"),
                        aName.c_str());

    /* Check that we don't exceed the maximum number of USB controllers for the given type. */
    ULONG maxInstances;
    rc = mParent->i_getSystemProperties()->GetMaxInstancesOfUSBControllerType(mHWData->mChipsetType, aType, &maxInstances);
    if (FAILED(rc))
        return rc;

    ULONG cInstances = i_getUSBControllerCountByType(aType);
    if (cInstances >= maxInstances)
        return setError(E_INVALIDARG,
                        tr("Too many USB controllers of this type"));

    ctrl.createObject();

    rc = ctrl->init(this, aName, aType);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_USB);
    mUSBControllers.backup();
    mUSBControllers->push_back(ctrl);

    ctrl.queryInterfaceTo(aController.asOutParam());

    /* inform the direct session if any */
    alock.release();
    i_onUSBControllerChange();

    return S_OK;
}

HRESULT Machine::getUSBControllerByName(const com::Utf8Str &aName, ComPtr<IUSBController> &aController)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<USBController> ctrl;

    HRESULT rc = i_getUSBControllerByName(aName, ctrl, true /* aSetError */);
    if (SUCCEEDED(rc))
        ctrl.queryInterfaceTo(aController.asOutParam());

    return rc;
}

HRESULT Machine::getUSBControllerCountByType(USBControllerType_T aType,
                                             ULONG *aControllers)
{
    if (   (aType <= USBControllerType_Null)
        || (aType >= USBControllerType_Last))
        return setError(E_INVALIDARG,
                        tr("Invalid USB controller type: %d"),
                        aType);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<USBController> ctrl;

    *aControllers = i_getUSBControllerCountByType(aType);

    return S_OK;
}

HRESULT Machine::removeUSBController(const com::Utf8Str &aName)
{

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    ComObjPtr<USBController> ctrl;
    rc = i_getUSBControllerByName(aName, ctrl, true /* aSetError */);
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_USB);
    mUSBControllers.backup();

    ctrl->i_unshare();

    mUSBControllers->remove(ctrl);

    /* inform the direct session if any */
    alock.release();
    i_onUSBControllerChange();

    return S_OK;
}

HRESULT Machine::querySavedGuestScreenInfo(ULONG aScreenId,
                                           ULONG *aOriginX,
                                           ULONG *aOriginY,
                                           ULONG *aWidth,
                                           ULONG *aHeight,
                                           BOOL  *aEnabled)
{
    uint32_t u32OriginX= 0;
    uint32_t u32OriginY= 0;
    uint32_t u32Width = 0;
    uint32_t u32Height = 0;
    uint16_t u16Flags = 0;

    int vrc = readSavedGuestScreenInfo(mSSData->strStateFilePath, aScreenId,
                                       &u32OriginX, &u32OriginY, &u32Width, &u32Height, &u16Flags);
    if (RT_FAILURE(vrc))
    {
#ifdef RT_OS_WINDOWS
        /* HACK: GUI sets *pfEnabled to 'true' and expects it to stay so if the API fails.
         * This works with XPCOM. But Windows COM sets all output parameters to zero.
         * So just assign fEnable to TRUE again.
         * The right fix would be to change GUI API wrappers to make sure that parameters
         * are changed only if API succeeds.
         */
        *aEnabled = TRUE;
#endif
        return setError(VBOX_E_IPRT_ERROR,
                        tr("Saved guest size is not available (%Rrc)"),
                        vrc);
    }

    *aOriginX = u32OriginX;
    *aOriginY = u32OriginY;
    *aWidth = u32Width;
    *aHeight = u32Height;
    *aEnabled = (u16Flags & VBVA_SCREEN_F_DISABLED) == 0;

    return S_OK;
}

HRESULT Machine::readSavedThumbnailToArray(ULONG aScreenId, BitmapFormat_T aBitmapFormat,
                                           ULONG *aWidth, ULONG *aHeight, std::vector<BYTE> &aData)
{
    if (aScreenId != 0)
        return E_NOTIMPL;

    if (   aBitmapFormat != BitmapFormat_BGR0
        && aBitmapFormat != BitmapFormat_BGRA
        && aBitmapFormat != BitmapFormat_RGBA
        && aBitmapFormat != BitmapFormat_PNG)
        return setError(E_NOTIMPL,
                        tr("Unsupported saved thumbnail format 0x%08X"), aBitmapFormat);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    uint8_t *pu8Data = NULL;
    uint32_t cbData = 0;
    uint32_t u32Width = 0;
    uint32_t u32Height = 0;

    int vrc = readSavedDisplayScreenshot(mSSData->strStateFilePath, 0 /* u32Type */, &pu8Data, &cbData, &u32Width, &u32Height);

    if (RT_FAILURE(vrc))
        return setError(VBOX_E_IPRT_ERROR,
                        tr("Saved thumbnail data is not available (%Rrc)"),
                        vrc);

    HRESULT hr = S_OK;

    *aWidth = u32Width;
    *aHeight = u32Height;

    if (cbData > 0)
    {
        /* Convert pixels to the format expected by the API caller. */
        if (aBitmapFormat == BitmapFormat_BGR0)
        {
            /* [0] B, [1] G, [2] R, [3] 0. */
            aData.resize(cbData);
            memcpy(&aData.front(), pu8Data, cbData);
        }
        else if (aBitmapFormat == BitmapFormat_BGRA)
        {
            /* [0] B, [1] G, [2] R, [3] A. */
            aData.resize(cbData);
            for (uint32_t i = 0; i < cbData; i += 4)
            {
                aData[i]     = pu8Data[i];
                aData[i + 1] = pu8Data[i + 1];
                aData[i + 2] = pu8Data[i + 2];
                aData[i + 3] = 0xff;
            }
        }
        else if (aBitmapFormat == BitmapFormat_RGBA)
        {
            /* [0] R, [1] G, [2] B, [3] A. */
            aData.resize(cbData);
            for (uint32_t i = 0; i < cbData; i += 4)
            {
                aData[i]     = pu8Data[i + 2];
                aData[i + 1] = pu8Data[i + 1];
                aData[i + 2] = pu8Data[i];
                aData[i + 3] = 0xff;
            }
        }
        else if (aBitmapFormat == BitmapFormat_PNG)
        {
            uint8_t *pu8PNG = NULL;
            uint32_t cbPNG = 0;
            uint32_t cxPNG = 0;
            uint32_t cyPNG = 0;

            vrc = DisplayMakePNG(pu8Data, u32Width, u32Height, &pu8PNG, &cbPNG, &cxPNG, &cyPNG, 0);

            if (RT_SUCCESS(vrc))
            {
                aData.resize(cbPNG);
                if (cbPNG)
                    memcpy(&aData.front(), pu8PNG, cbPNG);
            }
            else
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Could not convert saved thumbnail to PNG (%Rrc)"),
                              vrc);

            RTMemFree(pu8PNG);
        }
    }

    freeSavedDisplayScreenshot(pu8Data);

    return hr;
}

HRESULT Machine::querySavedScreenshotInfo(ULONG aScreenId,
                                          ULONG *aWidth,
                                          ULONG *aHeight,
                                          std::vector<BitmapFormat_T> &aBitmapFormats)
{
    if (aScreenId != 0)
        return E_NOTIMPL;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    uint8_t *pu8Data = NULL;
    uint32_t cbData = 0;
    uint32_t u32Width = 0;
    uint32_t u32Height = 0;

    int vrc = readSavedDisplayScreenshot(mSSData->strStateFilePath, 1 /* u32Type */, &pu8Data, &cbData, &u32Width, &u32Height);

    if (RT_FAILURE(vrc))
        return setError(VBOX_E_IPRT_ERROR,
                        tr("Saved screenshot data is not available (%Rrc)"),
                        vrc);

    *aWidth = u32Width;
    *aHeight = u32Height;
    aBitmapFormats.resize(1);
    aBitmapFormats[0] = BitmapFormat_PNG;

    freeSavedDisplayScreenshot(pu8Data);

    return S_OK;
}

HRESULT Machine::readSavedScreenshotToArray(ULONG aScreenId,
                                            BitmapFormat_T aBitmapFormat,
                                            ULONG *aWidth,
                                            ULONG *aHeight,
                                            std::vector<BYTE> &aData)
{
    if (aScreenId != 0)
        return E_NOTIMPL;

    if (aBitmapFormat != BitmapFormat_PNG)
        return E_NOTIMPL;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    uint8_t *pu8Data = NULL;
    uint32_t cbData = 0;
    uint32_t u32Width = 0;
    uint32_t u32Height = 0;

    int vrc = readSavedDisplayScreenshot(mSSData->strStateFilePath, 1 /* u32Type */, &pu8Data, &cbData, &u32Width, &u32Height);

    if (RT_FAILURE(vrc))
        return setError(VBOX_E_IPRT_ERROR,
                        tr("Saved screenshot thumbnail data is not available (%Rrc)"),
                        vrc);

    *aWidth = u32Width;
    *aHeight = u32Height;

    aData.resize(cbData);
    if (cbData)
        memcpy(&aData.front(), pu8Data, cbData);

    freeSavedDisplayScreenshot(pu8Data);

    return S_OK;
}

HRESULT Machine::hotPlugCPU(ULONG aCpu)
{
    HRESULT rc = S_OK;
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mHWData->mCPUHotPlugEnabled)
        return setError(E_INVALIDARG, tr("CPU hotplug is not enabled"));

    if (aCpu >= mHWData->mCPUCount)
        return setError(E_INVALIDARG, tr("CPU id exceeds number of possible CPUs [0:%lu]"), mHWData->mCPUCount-1);

    if (mHWData->mCPUAttached[aCpu])
        return setError(VBOX_E_OBJECT_IN_USE, tr("CPU %lu is already attached"), aCpu);

    alock.release();
    rc = i_onCPUChange(aCpu, false);
    alock.acquire();
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mCPUAttached[aCpu] = true;

    /** Save settings if online - @todo why is this required? -- @bugref{6818} */
    if (Global::IsOnline(mData->mMachineState))
        i_saveSettings(NULL);

    return S_OK;
}

HRESULT Machine::hotUnplugCPU(ULONG aCpu)
{
    HRESULT rc = S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mHWData->mCPUHotPlugEnabled)
        return setError(E_INVALIDARG, tr("CPU hotplug is not enabled"));

    if (aCpu >= SchemaDefs::MaxCPUCount)
        return setError(E_INVALIDARG,
                        tr("CPU index exceeds maximum CPU count (must be in range [0:%lu])"),
                        SchemaDefs::MaxCPUCount);

    if (!mHWData->mCPUAttached[aCpu])
        return setError(VBOX_E_OBJECT_NOT_FOUND, tr("CPU %lu is not attached"), aCpu);

    /* CPU 0 can't be detached */
    if (aCpu == 0)
        return setError(E_INVALIDARG, tr("It is not possible to detach CPU 0"));

    alock.release();
    rc = i_onCPUChange(aCpu, true);
    alock.acquire();
    if (FAILED(rc)) return rc;

    i_setModified(IsModified_MachineData);
    mHWData.backup();
    mHWData->mCPUAttached[aCpu] = false;

    /** Save settings if online - @todo why is this required? -- @bugref{6818} */
    if (Global::IsOnline(mData->mMachineState))
        i_saveSettings(NULL);

    return S_OK;
}

HRESULT Machine::getCPUStatus(ULONG aCpu, BOOL *aAttached)
{
    *aAttached = false;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* If hotplug is enabled the CPU is always enabled. */
    if (!mHWData->mCPUHotPlugEnabled)
    {
        if (aCpu < mHWData->mCPUCount)
            *aAttached = true;
    }
    else
    {
        if (aCpu < SchemaDefs::MaxCPUCount)
            *aAttached = mHWData->mCPUAttached[aCpu];
    }

    return S_OK;
}

HRESULT Machine::queryLogFilename(ULONG aIdx, com::Utf8Str &aFilename)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    Utf8Str log = i_getLogFilename(aIdx);
    if (!RTFileExists(log.c_str()))
        log.setNull();
    aFilename = log;

    return S_OK;
}

HRESULT Machine::readLog(ULONG aIdx, LONG64 aOffset, LONG64 aSize, std::vector<BYTE> &aData)
{
    if (aSize < 0)
        return setError(E_INVALIDARG, tr("The size argument (%lld) is negative"), aSize);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;
    Utf8Str log = i_getLogFilename(aIdx);

    /* do not unnecessarily hold the lock while doing something which does
     * not need the lock and potentially takes a long time. */
    alock.release();

    /* Limit the chunk size to 32K for now, as that gives better performance
     * over (XP)COM, and keeps the SOAP reply size under 1M for the webservice.
     * One byte expands to approx. 25 bytes of breathtaking XML. */
    size_t cbData = (size_t)RT_MIN(aSize, 32768);
    aData.resize(cbData);

    RTFILE LogFile;
    int vrc = RTFileOpen(&LogFile, log.c_str(),
                         RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_NONE);
    if (RT_SUCCESS(vrc))
    {
        vrc = RTFileReadAt(LogFile, aOffset, cbData? &aData.front(): NULL, cbData, &cbData);
        if (RT_SUCCESS(vrc))
            aData.resize(cbData);
        else
            rc = setError(VBOX_E_IPRT_ERROR,
                          tr("Could not read log file '%s' (%Rrc)"),
                          log.c_str(), vrc);
        RTFileClose(LogFile);
    }
    else
        rc = setError(VBOX_E_IPRT_ERROR,
                      tr("Could not open log file '%s' (%Rrc)"),
                      log.c_str(), vrc);

    if (FAILED(rc))
        aData.resize(0);

    return rc;
}


/**
 * Currently this method doesn't attach device to the running VM,
 * just makes sure it's plugged on next VM start.
 */
HRESULT Machine::attachHostPCIDevice(LONG aHostAddress, LONG aDesiredGuestAddress, BOOL /* aTryToUnbind */)
{
    // lock scope
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        HRESULT rc = i_checkStateDependency(MutableStateDep);
        if (FAILED(rc)) return rc;

        ChipsetType_T aChipset = ChipsetType_PIIX3;
        COMGETTER(ChipsetType)(&aChipset);

        if (aChipset != ChipsetType_ICH9)
        {
            return setError(E_INVALIDARG,
                            tr("Host PCI attachment only supported with ICH9 chipset"));
        }

        // check if device with this host PCI address already attached
        for (HWData::PCIDeviceAssignmentList::const_iterator
             it = mHWData->mPCIDeviceAssignments.begin();
             it != mHWData->mPCIDeviceAssignments.end();
             ++it)
        {
            LONG iHostAddress = -1;
            ComPtr<PCIDeviceAttachment> pAttach;
            pAttach = *it;
            pAttach->COMGETTER(HostAddress)(&iHostAddress);
            if (iHostAddress == aHostAddress)
                return setError(E_INVALIDARG,
                                tr("Device with host PCI address already attached to this VM"));
        }

        ComObjPtr<PCIDeviceAttachment> pda;
        char name[32];

        RTStrPrintf(name, sizeof(name), "host%02x:%02x.%x", (aHostAddress>>8) & 0xff,
                    (aHostAddress & 0xf8) >> 3, aHostAddress & 7);
        pda.createObject();
        pda->init(this, name, aHostAddress, aDesiredGuestAddress, TRUE);
        i_setModified(IsModified_MachineData);
        mHWData.backup();
        mHWData->mPCIDeviceAssignments.push_back(pda);
    }

    return S_OK;
}

/**
 * Currently this method doesn't detach device from the running VM,
 * just makes sure it's not plugged on next VM start.
 */
HRESULT Machine::detachHostPCIDevice(LONG aHostAddress)
{
    ComObjPtr<PCIDeviceAttachment> pAttach;
    bool fRemoved = false;
    HRESULT rc;

    // lock scope
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        rc = i_checkStateDependency(MutableStateDep);
        if (FAILED(rc)) return rc;

        for (HWData::PCIDeviceAssignmentList::const_iterator
             it = mHWData->mPCIDeviceAssignments.begin();
             it != mHWData->mPCIDeviceAssignments.end();
             ++it)
        {
            LONG iHostAddress = -1;
            pAttach = *it;
            pAttach->COMGETTER(HostAddress)(&iHostAddress);
            if (iHostAddress  != -1  && iHostAddress == aHostAddress)
            {
                i_setModified(IsModified_MachineData);
                mHWData.backup();
                mHWData->mPCIDeviceAssignments.remove(pAttach);
                fRemoved = true;
                break;
            }
        }
    }


    /* Fire event outside of the lock */
    if (fRemoved)
    {
        Assert(!pAttach.isNull());
        ComPtr<IEventSource> es;
        rc = mParent->COMGETTER(EventSource)(es.asOutParam());
        Assert(SUCCEEDED(rc));
        Bstr mid;
        rc = this->COMGETTER(Id)(mid.asOutParam());
        Assert(SUCCEEDED(rc));
        fireHostPCIDevicePlugEvent(es, mid.raw(), false /* unplugged */, true /* success */, pAttach, NULL);
    }

    return fRemoved ? S_OK : setError(VBOX_E_OBJECT_NOT_FOUND,
                                      tr("No host PCI device %08x attached"),
                                      aHostAddress
                                      );
}

HRESULT Machine::getPCIDeviceAssignments(std::vector<ComPtr<IPCIDeviceAttachment> > &aPCIDeviceAssignments)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aPCIDeviceAssignments.resize(mHWData->mPCIDeviceAssignments.size());
    size_t i = 0;
    for (std::list<ComObjPtr<PCIDeviceAttachment> >::const_iterator
         it = mHWData->mPCIDeviceAssignments.begin();
         it != mHWData->mPCIDeviceAssignments.end();
         ++it, ++i)
        (*it).queryInterfaceTo(aPCIDeviceAssignments[i].asOutParam());

    return S_OK;
}

HRESULT Machine::getBandwidthControl(ComPtr<IBandwidthControl> &aBandwidthControl)
{
    mBandwidthControl.queryInterfaceTo(aBandwidthControl.asOutParam());

    return S_OK;
}

HRESULT Machine::getTracingEnabled(BOOL *aTracingEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aTracingEnabled = mHWData->mDebugging.fTracingEnabled;

    return S_OK;
}

HRESULT Machine::setTracingEnabled(BOOL aTracingEnabled)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableStateDep);
    if (SUCCEEDED(hrc))
    {
        hrc = mHWData.backupEx();
        if (SUCCEEDED(hrc))
        {
            i_setModified(IsModified_MachineData);
            mHWData->mDebugging.fTracingEnabled = aTracingEnabled != FALSE;
        }
    }
    return hrc;
}

HRESULT Machine::getTracingConfig(com::Utf8Str &aTracingConfig)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aTracingConfig = mHWData->mDebugging.strTracingConfig;
    return S_OK;
}

HRESULT Machine::setTracingConfig(const com::Utf8Str &aTracingConfig)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableStateDep);
    if (SUCCEEDED(hrc))
    {
        hrc = mHWData.backupEx();
        if (SUCCEEDED(hrc))
        {
            mHWData->mDebugging.strTracingConfig = aTracingConfig;
            if (SUCCEEDED(hrc))
                i_setModified(IsModified_MachineData);
        }
    }
    return hrc;
}

HRESULT Machine::getAllowTracingToAccessVM(BOOL *aAllowTracingToAccessVM)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAllowTracingToAccessVM = mHWData->mDebugging.fAllowTracingToAccessVM;

    return S_OK;
}

HRESULT Machine::setAllowTracingToAccessVM(BOOL aAllowTracingToAccessVM)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableStateDep);
    if (SUCCEEDED(hrc))
    {
        hrc = mHWData.backupEx();
        if (SUCCEEDED(hrc))
        {
            i_setModified(IsModified_MachineData);
            mHWData->mDebugging.fAllowTracingToAccessVM = aAllowTracingToAccessVM != FALSE;
        }
    }
    return hrc;
}

HRESULT Machine::getAutostartEnabled(BOOL *aAutostartEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAutostartEnabled = mHWData->mAutostart.fAutostartEnabled;

    return S_OK;
}

HRESULT Machine::setAutostartEnabled(BOOL aAutostartEnabled)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT hrc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (   SUCCEEDED(hrc)
        && mHWData->mAutostart.fAutostartEnabled != !!aAutostartEnabled)
    {
        AutostartDb *autostartDb = mParent->i_getAutostartDb();
        int vrc;

        if (aAutostartEnabled)
            vrc = autostartDb->addAutostartVM(mUserData->s.strName.c_str());
        else
            vrc = autostartDb->removeAutostartVM(mUserData->s.strName.c_str());

        if (RT_SUCCESS(vrc))
        {
            hrc = mHWData.backupEx();
            if (SUCCEEDED(hrc))
            {
                i_setModified(IsModified_MachineData);
                mHWData->mAutostart.fAutostartEnabled = aAutostartEnabled != FALSE;
            }
        }
        else if (vrc == VERR_NOT_SUPPORTED)
            hrc = setError(VBOX_E_NOT_SUPPORTED,
                           tr("The VM autostart feature is not supported on this platform"));
        else if (vrc == VERR_PATH_NOT_FOUND)
            hrc = setError(E_FAIL,
                           tr("The path to the autostart database is not set"));
        else
            hrc = setError(E_UNEXPECTED,
                           tr("%s machine '%s' to the autostart database failed with %Rrc"),
                           aAutostartEnabled ? "Adding" : "Removing",
                           mUserData->s.strName.c_str(), vrc);
    }
    return hrc;
}

HRESULT Machine::getAutostartDelay(ULONG *aAutostartDelay)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAutostartDelay = mHWData->mAutostart.uAutostartDelay;

    return S_OK;
}

HRESULT Machine::setAutostartDelay(ULONG aAutostartDelay)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (SUCCEEDED(hrc))
    {
        hrc = mHWData.backupEx();
        if (SUCCEEDED(hrc))
        {
            i_setModified(IsModified_MachineData);
            mHWData->mAutostart.uAutostartDelay = aAutostartDelay;
        }
    }
    return hrc;
}

HRESULT Machine::getAutostopType(AutostopType_T *aAutostopType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAutostopType = mHWData->mAutostart.enmAutostopType;

    return S_OK;
}

HRESULT Machine::setAutostopType(AutostopType_T aAutostopType)
{
   AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
   HRESULT hrc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
   if (   SUCCEEDED(hrc)
       && mHWData->mAutostart.enmAutostopType != aAutostopType)
   {
       AutostartDb *autostartDb = mParent->i_getAutostartDb();
       int vrc;

       if (aAutostopType != AutostopType_Disabled)
           vrc = autostartDb->addAutostopVM(mUserData->s.strName.c_str());
       else
           vrc = autostartDb->removeAutostopVM(mUserData->s.strName.c_str());

       if (RT_SUCCESS(vrc))
       {
           hrc = mHWData.backupEx();
           if (SUCCEEDED(hrc))
           {
               i_setModified(IsModified_MachineData);
               mHWData->mAutostart.enmAutostopType = aAutostopType;
           }
       }
       else if (vrc == VERR_NOT_SUPPORTED)
           hrc = setError(VBOX_E_NOT_SUPPORTED,
                          tr("The VM autostop feature is not supported on this platform"));
       else if (vrc == VERR_PATH_NOT_FOUND)
           hrc = setError(E_FAIL,
                          tr("The path to the autostart database is not set"));
       else
           hrc = setError(E_UNEXPECTED,
                          tr("%s machine '%s' to the autostop database failed with %Rrc"),
                          aAutostopType != AutostopType_Disabled ? "Adding" : "Removing",
                          mUserData->s.strName.c_str(), vrc);
    }
    return hrc;
}

HRESULT Machine::getDefaultFrontend(com::Utf8Str &aDefaultFrontend)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aDefaultFrontend = mHWData->mDefaultFrontend;

    return S_OK;
}

HRESULT Machine::setDefaultFrontend(const com::Utf8Str &aDefaultFrontend)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableOrSavedStateDep);
    if (SUCCEEDED(hrc))
    {
        hrc = mHWData.backupEx();
        if (SUCCEEDED(hrc))
        {
            i_setModified(IsModified_MachineData);
            mHWData->mDefaultFrontend = aDefaultFrontend;
        }
    }
    return hrc;
}

HRESULT Machine::getIcon(std::vector<BYTE> &aIcon)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    size_t cbIcon = mUserData->s.ovIcon.size();
    aIcon.resize(cbIcon);
    if (cbIcon)
        memcpy(&aIcon.front(), &mUserData->s.ovIcon[0], cbIcon);
    return S_OK;
}

HRESULT Machine::setIcon(const std::vector<BYTE> &aIcon)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableOrSavedStateDep);
    if (SUCCEEDED(hrc))
    {
        i_setModified(IsModified_MachineData);
        mUserData.backup();
        size_t cbIcon = aIcon.size();
        mUserData->s.ovIcon.resize(cbIcon);
        if (cbIcon)
            memcpy(&mUserData->s.ovIcon[0], &aIcon.front(), cbIcon);
    }
    return hrc;
}

HRESULT Machine::getUSBProxyAvailable(BOOL *aUSBProxyAvailable)
{
#ifdef VBOX_WITH_USB
    *aUSBProxyAvailable = true;
#else
    *aUSBProxyAvailable = false;
#endif
    return S_OK;
}

HRESULT Machine::getVMProcessPriority(com::Utf8Str &aVMProcessPriority)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aVMProcessPriority = mUserData->s.strVMPriority;

    return S_OK;
}

HRESULT Machine::setVMProcessPriority(const com::Utf8Str &aVMProcessPriority)
{
    RT_NOREF(aVMProcessPriority);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT hrc = i_checkStateDependency(MutableOrSavedOrRunningStateDep);
    if (SUCCEEDED(hrc))
    {
        /** @todo r=klaus: currently this is marked as not implemented, as
         * the code for setting the priority of the process is not there
         * (neither when starting the VM nor at runtime). */
        ReturnComNotImplemented();
#if 0
        hrc = mUserData.backupEx();
        if (SUCCEEDED(hrc))
        {
            i_setModified(IsModified_MachineData);
            mUserData->s.strVMPriority = aVMProcessPriority;
        }
#endif
    }
    return hrc;
}

HRESULT Machine::cloneTo(const ComPtr<IMachine> &aTarget, CloneMode_T aMode, const std::vector<CloneOptions_T> &aOptions,
                         ComPtr<IProgress> &aProgress)
{
    ComObjPtr<Progress> pP;
    Progress  *ppP = pP;
    IProgress *iP  = static_cast<IProgress *>(ppP);
    IProgress **pProgress = &iP;

    IMachine  *pTarget    = aTarget;

    /* Convert the options. */
    RTCList<CloneOptions_T> optList;
    if (aOptions.size())
        for (size_t i = 0; i < aOptions.size(); ++i)
            optList.append(aOptions[i]);

    if (optList.contains(CloneOptions_Link))
    {
        if (!i_isSnapshotMachine())
            return setError(E_INVALIDARG,
                            tr("Linked clone can only be created from a snapshot"));
        if (aMode != CloneMode_MachineState)
            return setError(E_INVALIDARG,
                            tr("Linked clone can only be created for a single machine state"));
    }
    AssertReturn(!(optList.contains(CloneOptions_KeepAllMACs) && optList.contains(CloneOptions_KeepNATMACs)), E_INVALIDARG);

    MachineCloneVM *pWorker = new MachineCloneVM(this, static_cast<Machine*>(pTarget), aMode, optList);

    HRESULT rc = pWorker->start(pProgress);

    pP = static_cast<Progress *>(*pProgress);
    pP.queryInterfaceTo(aProgress.asOutParam());

    return rc;

}

HRESULT Machine::saveState(ComPtr<IProgress> &aProgress)
{
    NOREF(aProgress);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    // This check should always fail.
    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    AssertFailedReturn(E_NOTIMPL);
}

HRESULT Machine::adoptSavedState(const com::Utf8Str &aSavedStateFile)
{
    NOREF(aSavedStateFile);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    // This check should always fail.
    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    AssertFailedReturn(E_NOTIMPL);
}

HRESULT Machine::discardSavedState(BOOL aFRemoveFile)
{
    NOREF(aFRemoveFile);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    // This check should always fail.
    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    AssertFailedReturn(E_NOTIMPL);
}

// public methods for internal purposes
/////////////////////////////////////////////////////////////////////////////

/**
 * Adds the given IsModified_* flag to the dirty flags of the machine.
 * This must be called either during i_loadSettings or under the machine write lock.
 * @param   fl                       Flag
 * @param   fAllowStateModification  If state modifications are allowed.
 */
void Machine::i_setModified(uint32_t fl, bool fAllowStateModification /* = true */)
{
    mData->flModifications |= fl;
    if (fAllowStateModification && i_isStateModificationAllowed())
        mData->mCurrentStateModified = true;
}

/**
 * Adds the given IsModified_* flag to the dirty flags of the machine, taking
 * care of the write locking.
 *
 * @param   fModification            The flag to add.
 * @param   fAllowStateModification  If state modifications are allowed.
 */
void Machine::i_setModifiedLock(uint32_t fModification, bool fAllowStateModification /* = true */)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    i_setModified(fModification, fAllowStateModification);
}

/**
 *  Saves the registry entry of this machine to the given configuration node.
 *
 *  @param data     Machine registry data.
 *
 *  @note locks this object for reading.
 */
HRESULT Machine::i_saveRegistryEntry(settings::MachineRegistryEntry &data)
{
    AutoLimitedCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data.uuid = mData->mUuid;
    data.strSettingsFile = mData->m_strConfigFile;

    return S_OK;
}

/**
 * Calculates the absolute path of the given path taking the directory of the
 * machine settings file as the current directory.
 *
 * @param  strPath  Path to calculate the absolute path for.
 * @param  aResult  Where to put the result (used only on success, can be the
 *                  same Utf8Str instance as passed in @a aPath).
 * @return IPRT result.
 *
 * @note Locks this object for reading.
 */
int Machine::i_calculateFullPath(const Utf8Str &strPath, Utf8Str &aResult)
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(!mData->m_strConfigFileFull.isEmpty(), VERR_GENERAL_FAILURE);

    Utf8Str strSettingsDir = mData->m_strConfigFileFull;

    strSettingsDir.stripFilename();
    char folder[RTPATH_MAX];
    int vrc = RTPathAbsEx(strSettingsDir.c_str(), strPath.c_str(), folder, sizeof(folder));
    if (RT_SUCCESS(vrc))
        aResult = folder;

    return vrc;
}

/**
 * Copies strSource to strTarget, making it relative to the machine folder
 * if it is a subdirectory thereof, or simply copying it otherwise.
 *
 * @param strSource Path to evaluate and copy.
 * @param strTarget Buffer to receive target path.
 *
 * @note Locks this object for reading.
 */
void Machine::i_copyPathRelativeToMachine(const Utf8Str &strSource,
                                          Utf8Str &strTarget)
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), (void)0);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturnVoid(!mData->m_strConfigFileFull.isEmpty());
    // use strTarget as a temporary buffer to hold the machine settings dir
    strTarget = mData->m_strConfigFileFull;
    strTarget.stripFilename();
    if (RTPathStartsWith(strSource.c_str(), strTarget.c_str()))
    {
        // is relative: then append what's left
        strTarget = strSource.substr(strTarget.length() + 1); // skip '/'
        // for empty paths (only possible for subdirs) use "." to avoid
        // triggering default settings for not present config attributes.
        if (strTarget.isEmpty())
            strTarget = ".";
    }
    else
        // is not relative: then overwrite
        strTarget = strSource;
}

/**
 *  Returns the full path to the machine's log folder in the
 *  \a aLogFolder argument.
 */
void Machine::i_getLogFolder(Utf8Str &aLogFolder)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    char szTmp[RTPATH_MAX];
    int vrc = RTEnvGetEx(RTENV_DEFAULT, "VBOX_USER_VMLOGDIR", szTmp, sizeof(szTmp), NULL);
    if (RT_SUCCESS(vrc))
    {
        if (szTmp[0] && !mUserData.isNull())
        {
            char szTmp2[RTPATH_MAX];
            vrc = RTPathAbs(szTmp, szTmp2, sizeof(szTmp2));
            if (RT_SUCCESS(vrc))
                aLogFolder = Utf8StrFmt("%s%c%s",
                                        szTmp2,
                                        RTPATH_DELIMITER,
                                        mUserData->s.strName.c_str()); // path/to/logfolder/vmname
        }
        else
            vrc = VERR_PATH_IS_RELATIVE;
    }

    if (RT_FAILURE(vrc))
    {
        // fallback if VBOX_USER_LOGHOME is not set or invalid
        aLogFolder = mData->m_strConfigFileFull;    // path/to/machinesfolder/vmname/vmname.vbox
        aLogFolder.stripFilename();                 // path/to/machinesfolder/vmname
        aLogFolder.append(RTPATH_DELIMITER);
        aLogFolder.append("Logs");                  // path/to/machinesfolder/vmname/Logs
    }
}

/**
 *  Returns the full path to the machine's log file for an given index.
 */
Utf8Str Machine::i_getLogFilename(ULONG idx)
{
    Utf8Str logFolder;
    getLogFolder(logFolder);
    Assert(logFolder.length());

    Utf8Str log;
    if (idx == 0)
        log = Utf8StrFmt("%s%cVBox.log", logFolder.c_str(), RTPATH_DELIMITER);
#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_HARDENING)
    else if (idx == 1)
        log = Utf8StrFmt("%s%cVBoxHardening.log", logFolder.c_str(), RTPATH_DELIMITER);
    else
        log = Utf8StrFmt("%s%cVBox.log.%u", logFolder.c_str(), RTPATH_DELIMITER, idx - 1);
#else
    else
        log = Utf8StrFmt("%s%cVBox.log.%u", logFolder.c_str(), RTPATH_DELIMITER, idx);
#endif
    return log;
}

/**
 * Returns the full path to the machine's hardened log file.
 */
Utf8Str Machine::i_getHardeningLogFilename(void)
{
    Utf8Str strFilename;
    getLogFolder(strFilename);
    Assert(strFilename.length());
    strFilename.append(RTPATH_SLASH_STR "VBoxHardening.log");
    return strFilename;
}


/**
 * Composes a unique saved state filename based on the current system time. The filename is
 * granular to the second so this will work so long as no more than one snapshot is taken on
 * a machine per second.
 *
 * Before version 4.1, we used this formula for saved state files:
 *      Utf8StrFmt("%s%c{%RTuuid}.sav", strFullSnapshotFolder.c_str(), RTPATH_DELIMITER, mData->mUuid.raw())
 * which no longer works because saved state files can now be shared between the saved state of the
 * "saved" machine and an online snapshot, and the following would cause problems:
 * 1) save machine
 * 2) create online snapshot from that machine state --> reusing saved state file
 * 3) save machine again --> filename would be reused, breaking the online snapshot
 *
 * So instead we now use a timestamp.
 *
 * @param strStateFilePath
 */

void Machine::i_composeSavedStateFilename(Utf8Str &strStateFilePath)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        i_calculateFullPath(mUserData->s.strSnapshotFolder, strStateFilePath);
    }

    RTTIMESPEC ts;
    RTTimeNow(&ts);
    RTTIME time;
    RTTimeExplode(&time, &ts);

    strStateFilePath += RTPATH_DELIMITER;
    strStateFilePath += Utf8StrFmt("%04d-%02u-%02uT%02u-%02u-%02u-%09uZ.sav",
                                   time.i32Year, time.u8Month, time.u8MonthDay,
                                   time.u8Hour, time.u8Minute, time.u8Second, time.u32Nanosecond);
}

/**
 *  Returns the full path to the default video capture file.
 */
void Machine::i_getDefaultVideoCaptureFile(Utf8Str &strFile)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    strFile = mData->m_strConfigFileFull;       // path/to/machinesfolder/vmname/vmname.vbox
    strFile.stripSuffix();                      // path/to/machinesfolder/vmname/vmname
    strFile.append(".webm");                    // path/to/machinesfolder/vmname/vmname.webm
}

/**
 * Returns whether at least one USB controller is present for the VM.
 */
bool Machine::i_isUSBControllerPresent()
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), false);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return (mUSBControllers->size() > 0);
}

/**
 *  @note Locks this object for writing, calls the client process
 *        (inside the lock).
 */
HRESULT Machine::i_launchVMProcess(IInternalSessionControl *aControl,
                                   const Utf8Str &strFrontend,
                                   const Utf8Str &strEnvironment,
                                   ProgressProxy *aProgress)
{
    LogFlowThisFuncEnter();

    AssertReturn(aControl, E_FAIL);
    AssertReturn(aProgress, E_FAIL);
    AssertReturn(!strFrontend.isEmpty(), E_FAIL);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mData->mRegistered)
        return setError(E_UNEXPECTED,
                        tr("The machine '%s' is not registered"),
                        mUserData->s.strName.c_str());

    LogFlowThisFunc(("mSession.mState=%s\n", Global::stringifySessionState(mData->mSession.mState)));

    /* The process started when launching a VM with separate UI/VM processes is always
     * the UI process, i.e. needs special handling as it won't claim the session. */
    bool fSeparate = strFrontend.endsWith("separate", Utf8Str::CaseInsensitive);

    if (fSeparate)
    {
        if (mData->mSession.mState != SessionState_Unlocked && mData->mSession.mName != "headless")
            return setError(VBOX_E_INVALID_OBJECT_STATE,
                            tr("The machine '%s' is in a state which is incompatible with launching a separate UI process"),
                            mUserData->s.strName.c_str());
    }
    else
    {
        if (    mData->mSession.mState == SessionState_Locked
             || mData->mSession.mState == SessionState_Spawning
             || mData->mSession.mState == SessionState_Unlocking)
            return setError(VBOX_E_INVALID_OBJECT_STATE,
                            tr("The machine '%s' is already locked by a session (or being locked or unlocked)"),
                            mUserData->s.strName.c_str());

        /* may not be busy */
        AssertReturn(!Global::IsOnlineOrTransient(mData->mMachineState), E_FAIL);
    }

    /* get the path to the executable */
    char szPath[RTPATH_MAX];
    RTPathAppPrivateArch(szPath, sizeof(szPath) - 1);
    size_t cchBufLeft = strlen(szPath);
    szPath[cchBufLeft++] = RTPATH_DELIMITER;
    szPath[cchBufLeft] = 0;
    char *pszNamePart = szPath + cchBufLeft;
    cchBufLeft = sizeof(szPath) - cchBufLeft;

    int vrc = VINF_SUCCESS;
    RTPROCESS pid = NIL_RTPROCESS;

    RTENV env = RTENV_DEFAULT;

    if (!strEnvironment.isEmpty())
    {
        char *newEnvStr = NULL;

        do
        {
            /* clone the current environment */
            int vrc2 = RTEnvClone(&env, RTENV_DEFAULT);
            AssertRCBreakStmt(vrc2, vrc = vrc2);

            newEnvStr = RTStrDup(strEnvironment.c_str());
            AssertPtrBreakStmt(newEnvStr, vrc = vrc2);

            /* put new variables to the environment
             * (ignore empty variable names here since RTEnv API
             * intentionally doesn't do that) */
            char *var = newEnvStr;
            for (char *p = newEnvStr; *p; ++p)
            {
                if (*p == '\n' && (p == newEnvStr || *(p - 1) != '\\'))
                {
                    *p = '\0';
                    if (*var)
                    {
                        char *val = strchr(var, '=');
                        if (val)
                        {
                            *val++ = '\0';
                            vrc2 = RTEnvSetEx(env, var, val);
                        }
                        else
                            vrc2 = RTEnvUnsetEx(env, var);
                        if (RT_FAILURE(vrc2))
                            break;
                    }
                    var = p + 1;
                }
            }
            if (RT_SUCCESS(vrc2) && *var)
                vrc2 = RTEnvPutEx(env, var);

            AssertRCBreakStmt(vrc2, vrc = vrc2);
        }
        while (0);

        if (newEnvStr != NULL)
            RTStrFree(newEnvStr);
    }

    /* Hardening logging */
#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_HARDENING)
    Utf8Str strSupHardeningLogArg("--sup-hardening-log=");
    {
        Utf8Str strHardeningLogFile = i_getHardeningLogFilename();
        int vrc2 = RTFileDelete(strHardeningLogFile.c_str());
        if (vrc2 == VERR_PATH_NOT_FOUND || vrc2 == VERR_FILE_NOT_FOUND)
        {
            Utf8Str strStartupLogDir = strHardeningLogFile;
            strStartupLogDir.stripFilename();
            RTDirCreateFullPath(strStartupLogDir.c_str(), 0755); /** @todo add a variant for creating the path to a
                                                                     file without stripping the file. */
        }
        strSupHardeningLogArg.append(strHardeningLogFile);

        /* Remove legacy log filename to avoid confusion. */
        Utf8Str strOldStartupLogFile;
        getLogFolder(strOldStartupLogFile);
        strOldStartupLogFile.append(RTPATH_SLASH_STR "VBoxStartup.log");
        RTFileDelete(strOldStartupLogFile.c_str());
    }
    const char *pszSupHardeningLogArg = strSupHardeningLogArg.c_str();
#else
    const char *pszSupHardeningLogArg = NULL;
#endif

    Utf8Str strCanonicalName;

#ifdef VBOX_WITH_QTGUI
    if (   !strFrontend.compare("gui", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("GUI/Qt", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("separate", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("gui/separate", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("GUI/Qt/separate", Utf8Str::CaseInsensitive))
    {
        strCanonicalName = "GUI/Qt";
# ifdef RT_OS_DARWIN /* Avoid Launch Services confusing this with the selector by using a helper app. */
        /* Modify the base path so that we don't need to use ".." below. */
        RTPathStripTrailingSlash(szPath);
        RTPathStripFilename(szPath);
        cchBufLeft = strlen(szPath);
        pszNamePart = szPath + cchBufLeft;
        cchBufLeft = sizeof(szPath) - cchBufLeft;

#  define OSX_APP_NAME "VirtualBoxVM"
#  define OSX_APP_PATH_FMT "/Resources/%s.app/Contents/MacOS/VirtualBoxVM"

        Utf8Str strAppOverride = i_getExtraData(Utf8Str("VBoxInternal2/VirtualBoxVMAppOverride"));
        if (   strAppOverride.contains(".")
            || strAppOverride.contains("/")
            || strAppOverride.contains("\\")
            || strAppOverride.contains(":"))
            strAppOverride.setNull();
        Utf8Str strAppPath;
        if (!strAppOverride.isEmpty())
        {
            strAppPath = Utf8StrFmt(OSX_APP_PATH_FMT, strAppOverride.c_str());
            Utf8Str strFullPath(szPath);
            strFullPath.append(strAppPath);
            /* there is a race, but people using this deserve the failure */
            if (!RTFileExists(strFullPath.c_str()))
                strAppOverride.setNull();
        }
        if (strAppOverride.isEmpty())
            strAppPath = Utf8StrFmt(OSX_APP_PATH_FMT, OSX_APP_NAME);
        AssertReturn(cchBufLeft > strAppPath.length(), E_UNEXPECTED);
        strcpy(pszNamePart, strAppPath.c_str());
# else
        static const char s_szVirtualBox_exe[] = "VirtualBox" HOSTSUFF_EXE;
        Assert(cchBufLeft >= sizeof(s_szVirtualBox_exe));
        strcpy(pszNamePart, s_szVirtualBox_exe);
# endif

        Utf8Str idStr = mData->mUuid.toString();
        const char *apszArgs[] =
        {
            szPath,
            "--comment", mUserData->s.strName.c_str(),
            "--startvm", idStr.c_str(),
            "--no-startvm-errormsgbox",
            NULL, /* For "--separate". */
            NULL, /* For "--sup-startup-log". */
            NULL
        };
        unsigned iArg = 6;
        if (fSeparate)
            apszArgs[iArg++] = "--separate";
        apszArgs[iArg++] = pszSupHardeningLogArg;

        vrc = RTProcCreate(szPath, apszArgs, env, 0, &pid);
    }
#else /* !VBOX_WITH_QTGUI */
    if (0)
        ;
#endif /* VBOX_WITH_QTGUI */

    else

#ifdef VBOX_WITH_VBOXSDL
    if (   !strFrontend.compare("sdl", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("GUI/SDL", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("sdl/separate", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("GUI/SDL/separate", Utf8Str::CaseInsensitive))
    {
        strCanonicalName = "GUI/SDL";
        static const char s_szVBoxSDL_exe[] = "VBoxSDL" HOSTSUFF_EXE;
        Assert(cchBufLeft >= sizeof(s_szVBoxSDL_exe));
        strcpy(pszNamePart, s_szVBoxSDL_exe);

        Utf8Str idStr = mData->mUuid.toString();
        const char *apszArgs[] =
        {
            szPath,
            "--comment", mUserData->s.strName.c_str(),
            "--startvm", idStr.c_str(),
            NULL, /* For "--separate". */
            NULL, /* For "--sup-startup-log". */
            NULL
        };
        unsigned iArg = 5;
        if (fSeparate)
            apszArgs[iArg++] = "--separate";
        apszArgs[iArg++] = pszSupHardeningLogArg;

        vrc = RTProcCreate(szPath, apszArgs, env, 0, &pid);
    }
#else /* !VBOX_WITH_VBOXSDL */
    if (0)
        ;
#endif /* !VBOX_WITH_VBOXSDL */

    else

#ifdef VBOX_WITH_HEADLESS
    if (   !strFrontend.compare("headless", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("capture", Utf8Str::CaseInsensitive)
        || !strFrontend.compare("vrdp", Utf8Str::CaseInsensitive) /* Deprecated. Same as headless. */
       )
    {
        strCanonicalName = "headless";
        /* On pre-4.0 the "headless" type was used for passing "--vrdp off" to VBoxHeadless to let it work in OSE,
         * which did not contain VRDP server. In VBox 4.0 the remote desktop server (VRDE) is optional,
         * and a VM works even if the server has not been installed.
         * So in 4.0 the "headless" behavior remains the same for default VBox installations.
         * Only if a VRDE has been installed and the VM enables it, the "headless" will work
         * differently in 4.0 and 3.x.
         */
        static const char s_szVBoxHeadless_exe[] = "VBoxHeadless" HOSTSUFF_EXE;
        Assert(cchBufLeft >= sizeof(s_szVBoxHeadless_exe));
        strcpy(pszNamePart, s_szVBoxHeadless_exe);

        Utf8Str idStr = mData->mUuid.toString();
        const char *apszArgs[] =
        {
            szPath,
            "--comment", mUserData->s.strName.c_str(),
            "--startvm", idStr.c_str(),
            "--vrde", "config",
            NULL, /* For "--capture". */
            NULL, /* For "--sup-startup-log". */
            NULL
        };
        unsigned iArg = 7;
        if (!strFrontend.compare("capture", Utf8Str::CaseInsensitive))
            apszArgs[iArg++] = "--capture";
        apszArgs[iArg++] = pszSupHardeningLogArg;

# ifdef RT_OS_WINDOWS
        vrc = RTProcCreate(szPath, apszArgs, env, RTPROC_FLAGS_NO_WINDOW, &pid);
# else
        vrc = RTProcCreate(szPath, apszArgs, env, 0, &pid);
# endif
    }
#else /* !VBOX_WITH_HEADLESS */
    if (0)
        ;
#endif /* !VBOX_WITH_HEADLESS */
    else
    {
        RTEnvDestroy(env);
        return setError(E_INVALIDARG,
                        tr("Invalid frontend name: '%s'"),
                        strFrontend.c_str());
    }

    RTEnvDestroy(env);

    if (RT_FAILURE(vrc))
        return setError(VBOX_E_IPRT_ERROR,
                        tr("Could not launch a process for the machine '%s' (%Rrc)"),
                        mUserData->s.strName.c_str(), vrc);

    LogFlowThisFunc(("launched.pid=%d(0x%x)\n", pid, pid));

    if (!fSeparate)
    {
        /*
         *  Note that we don't release the lock here before calling the client,
         *  because it doesn't need to call us back if called with a NULL argument.
         *  Releasing the lock here is dangerous because we didn't prepare the
         *  launch data yet, but the client we've just started may happen to be
         *  too fast and call LockMachine() that will fail (because of PID, etc.),
         *  so that the Machine will never get out of the Spawning session state.
         */

        /* inform the session that it will be a remote one */
        LogFlowThisFunc(("Calling AssignMachine (NULL)...\n"));
#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
        HRESULT rc = aControl->AssignMachine(NULL, LockType_Write, Bstr::Empty.raw());
#else /* VBOX_WITH_GENERIC_SESSION_WATCHER */
        HRESULT rc = aControl->AssignMachine(NULL, LockType_Write, NULL);
#endif /* VBOX_WITH_GENERIC_SESSION_WATCHER */
        LogFlowThisFunc(("AssignMachine (NULL) returned %08X\n", rc));

        if (FAILED(rc))
        {
            /* restore the session state */
            mData->mSession.mState = SessionState_Unlocked;
            alock.release();
            mParent->i_addProcessToReap(pid);
            /* The failure may occur w/o any error info (from RPC), so provide one */
            return setError(VBOX_E_VM_ERROR,
                            tr("Failed to assign the machine to the session (%Rhrc)"), rc);
        }

        /* attach launch data to the machine */
        Assert(mData->mSession.mPID == NIL_RTPROCESS);
        mData->mSession.mRemoteControls.push_back(aControl);
        mData->mSession.mProgress = aProgress;
        mData->mSession.mPID = pid;
        mData->mSession.mState = SessionState_Spawning;
        Assert(strCanonicalName.isNotEmpty());
        mData->mSession.mName = strCanonicalName;
    }
    else
    {
        /* For separate UI process we declare the launch as completed instantly, as the
         * actual headless VM start may or may not come. No point in remembering anything
         * yet, as what matters for us is when the headless VM gets started. */
        aProgress->i_notifyComplete(S_OK);
    }

    alock.release();
    mParent->i_addProcessToReap(pid);

    LogFlowThisFuncLeave();
    return S_OK;
}

/**
 * Returns @c true if the given session machine instance has an open direct
 * session (and optionally also for direct sessions which are closing) and
 * returns the session control machine instance if so.
 *
 * Note that when the method returns @c false, the arguments remain unchanged.
 *
 * @param aMachine      Session machine object.
 * @param aControl      Direct session control object (optional).
 * @param aRequireVM    If true then only allow VM sessions.
 * @param aAllowClosing If true then additionally a session which is currently
 *                      being closed will also be allowed.
 *
 * @note locks this object for reading.
 */
bool Machine::i_isSessionOpen(ComObjPtr<SessionMachine> &aMachine,
                              ComPtr<IInternalSessionControl> *aControl /*= NULL*/,
                              bool aRequireVM /*= false*/,
                              bool aAllowClosing /*= false*/)
{
    AutoLimitedCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), false);

    /* just return false for inaccessible machines */
    if (getObjectState().getState() != ObjectState::Ready)
        return false;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (    (   mData->mSession.mState == SessionState_Locked
             && (!aRequireVM || mData->mSession.mLockType == LockType_VM))
         || (aAllowClosing && mData->mSession.mState == SessionState_Unlocking)
       )
    {
        AssertReturn(!mData->mSession.mMachine.isNull(), false);

        aMachine = mData->mSession.mMachine;

        if (aControl != NULL)
            *aControl = mData->mSession.mDirectControl;

        return true;
    }

    return false;
}

/**
 * Returns @c true if the given machine has an spawning direct session.
 *
 * @note locks this object for reading.
 */
bool Machine::i_isSessionSpawning()
{
    AutoLimitedCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), false);

    /* just return false for inaccessible machines */
    if (getObjectState().getState() != ObjectState::Ready)
        return false;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData->mSession.mState == SessionState_Spawning)
        return true;

    return false;
}

/**
 * Called from the client watcher thread to check for unexpected client process
 * death during Session_Spawning state (e.g. before it successfully opened a
 * direct session).
 *
 * On Win32 and on OS/2, this method is called only when we've got the
 * direct client's process termination notification, so it always returns @c
 * true.
 *
 * On other platforms, this method returns @c true if the client process is
 * terminated and @c false if it's still alive.
 *
 * @note Locks this object for writing.
 */
bool Machine::i_checkForSpawnFailure()
{
    AutoCaller autoCaller(this);
    if (!autoCaller.isOk())
    {
        /* nothing to do */
        LogFlowThisFunc(("Already uninitialized!\n"));
        return true;
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData->mSession.mState != SessionState_Spawning)
    {
        /* nothing to do */
        LogFlowThisFunc(("Not spawning any more!\n"));
        return true;
    }

    HRESULT rc = S_OK;

    /* PID not yet initialized, skip check. */
    if (mData->mSession.mPID == NIL_RTPROCESS)
        return false;

    RTPROCSTATUS status;
    int vrc = RTProcWait(mData->mSession.mPID, RTPROCWAIT_FLAGS_NOBLOCK, &status);

    if (vrc != VERR_PROCESS_RUNNING)
    {
        Utf8Str strExtraInfo;

#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_HARDENING)
        /* If the startup logfile exists and is of non-zero length, tell the
           user to look there for more details to encourage them to attach it
           when reporting startup issues. */
        Utf8Str strHardeningLogFile = i_getHardeningLogFilename();
        uint64_t cbStartupLogFile = 0;
        int vrc2 = RTFileQuerySize(strHardeningLogFile.c_str(), &cbStartupLogFile);
        if (RT_SUCCESS(vrc2) && cbStartupLogFile > 0)
            strExtraInfo.append(Utf8StrFmt(tr(".  More details may be available in '%s'"), strHardeningLogFile.c_str()));
#endif

        if (RT_SUCCESS(vrc) && status.enmReason == RTPROCEXITREASON_NORMAL)
            rc = setError(E_FAIL,
                          tr("The virtual machine '%s' has terminated unexpectedly during startup with exit code %d (%#x)%s"),
                          i_getName().c_str(), status.iStatus, status.iStatus, strExtraInfo.c_str());
        else if (RT_SUCCESS(vrc) && status.enmReason == RTPROCEXITREASON_SIGNAL)
            rc = setError(E_FAIL,
                          tr("The virtual machine '%s' has terminated unexpectedly during startup because of signal %d%s"),
                          i_getName().c_str(), status.iStatus, strExtraInfo.c_str());
        else if (RT_SUCCESS(vrc) && status.enmReason == RTPROCEXITREASON_ABEND)
            rc = setError(E_FAIL,
                          tr("The virtual machine '%s' has terminated abnormally (iStatus=%#x)%s"),
                          i_getName().c_str(), status.iStatus, strExtraInfo.c_str());
        else
            rc = setError(E_FAIL,
                          tr("The virtual machine '%s' has terminated unexpectedly during startup (%Rrc)%s"),
                          i_getName().c_str(), vrc, strExtraInfo.c_str());
    }

    if (FAILED(rc))
    {
        /* Close the remote session, remove the remote control from the list
         * and reset session state to Closed (@note keep the code in sync with
         * the relevant part in LockMachine()). */

        Assert(mData->mSession.mRemoteControls.size() == 1);
        if (mData->mSession.mRemoteControls.size() == 1)
        {
            ErrorInfoKeeper eik;
            mData->mSession.mRemoteControls.front()->Uninitialize();
        }

        mData->mSession.mRemoteControls.clear();
        mData->mSession.mState = SessionState_Unlocked;

        /* finalize the progress after setting the state */
        if (!mData->mSession.mProgress.isNull())
        {
            mData->mSession.mProgress->notifyComplete(rc);
            mData->mSession.mProgress.setNull();
        }

        mData->mSession.mPID = NIL_RTPROCESS;

        mParent->i_onSessionStateChange(mData->mUuid, SessionState_Unlocked);
        return true;
    }

    return false;
}

/**
 *  Checks whether the machine can be registered. If so, commits and saves
 *  all settings.
 *
 *  @note Must be called from mParent's write lock. Locks this object and
 *  children for writing.
 */
HRESULT Machine::i_prepareRegister()
{
    AssertReturn(mParent->isWriteLockOnCurrentThread(), E_FAIL);

    AutoLimitedCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* wait for state dependents to drop to zero */
    i_ensureNoStateDependencies();

    if (!mData->mAccessible)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("The machine '%s' with UUID {%s} is inaccessible and cannot be registered"),
                        mUserData->s.strName.c_str(),
                        mData->mUuid.toString().c_str());

    AssertReturn(getObjectState().getState() == ObjectState::Ready, E_FAIL);

    if (mData->mRegistered)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("The machine '%s' with UUID {%s} is already registered"),
                        mUserData->s.strName.c_str(),
                        mData->mUuid.toString().c_str());

    HRESULT rc = S_OK;

    // Ensure the settings are saved. If we are going to be registered and
    // no config file exists yet, create it by calling i_saveSettings() too.
    if (    (mData->flModifications)
         || (!mData->pMachineConfigFile->fileExists())
       )
    {
        rc = i_saveSettings(NULL);
                // no need to check whether VirtualBox.xml needs saving too since
                // we can't have a machine XML file rename pending
        if (FAILED(rc)) return rc;
    }

    /* more config checking goes here */

    if (SUCCEEDED(rc))
    {
        /* we may have had implicit modifications we want to fix on success */
        i_commit();

        mData->mRegistered = true;
    }
    else
    {
        /* we may have had implicit modifications we want to cancel on failure*/
        i_rollback(false /* aNotify */);
    }

    return rc;
}

/**
 * Increases the number of objects dependent on the machine state or on the
 * registered state. Guarantees that these two states will not change at least
 * until #i_releaseStateDependency() is called.
 *
 * Depending on the @a aDepType value, additional state checks may be made.
 * These checks will set extended error info on failure. See
 * #i_checkStateDependency() for more info.
 *
 * If this method returns a failure, the dependency is not added and the caller
 * is not allowed to rely on any particular machine state or registration state
 * value and may return the failed result code to the upper level.
 *
 * @param aDepType      Dependency type to add.
 * @param aState        Current machine state (NULL if not interested).
 * @param aRegistered   Current registered state (NULL if not interested).
 *
 * @note Locks this object for writing.
 */
HRESULT Machine::i_addStateDependency(StateDependency aDepType /* = AnyStateDep */,
                                      MachineState_T *aState /* = NULL */,
                                      BOOL *aRegistered /* = NULL */)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(aDepType);
    if (FAILED(rc)) return rc;

    {
        if (mData->mMachineStateChangePending != 0)
        {
            /* i_ensureNoStateDependencies() is waiting for state dependencies to
             * drop to zero so don't add more. It may make sense to wait a bit
             * and retry before reporting an error (since the pending state
             * transition should be really quick) but let's just assert for
             * now to see if it ever happens on practice. */

            AssertFailed();

            return setError(E_ACCESSDENIED,
                            tr("Machine state change is in progress. Please retry the operation later."));
        }

        ++mData->mMachineStateDeps;
        Assert(mData->mMachineStateDeps != 0 /* overflow */);
    }

    if (aState)
        *aState = mData->mMachineState;
    if (aRegistered)
        *aRegistered = mData->mRegistered;

    return S_OK;
}

/**
 * Decreases the number of objects dependent on the machine state.
 * Must always complete the #i_addStateDependency() call after the state
 * dependency is no more necessary.
 */
void Machine::i_releaseStateDependency()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* releaseStateDependency() w/o addStateDependency()? */
    AssertReturnVoid(mData->mMachineStateDeps != 0);
    -- mData->mMachineStateDeps;

    if (mData->mMachineStateDeps == 0)
    {
        /* inform i_ensureNoStateDependencies() that there are no more deps */
        if (mData->mMachineStateChangePending != 0)
        {
            Assert(mData->mMachineStateDepsSem != NIL_RTSEMEVENTMULTI);
            RTSemEventMultiSignal (mData->mMachineStateDepsSem);
        }
    }
}

Utf8Str Machine::i_getExtraData(const Utf8Str &strKey)
{
    /* start with nothing found */
    Utf8Str strResult("");

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    settings::StringsMap::const_iterator it = mData->pMachineConfigFile->mapExtraDataItems.find(strKey);
    if (it != mData->pMachineConfigFile->mapExtraDataItems.end())
        // found:
        strResult = it->second; // source is a Utf8Str

    return strResult;
}

// protected methods
/////////////////////////////////////////////////////////////////////////////

/**
 *  Performs machine state checks based on the @a aDepType value. If a check
 *  fails, this method will set extended error info, otherwise it will return
 *  S_OK. It is supposed, that on failure, the caller will immediately return
 *  the return value of this method to the upper level.
 *
 *  When @a aDepType is AnyStateDep, this method always returns S_OK.
 *
 *  When @a aDepType is MutableStateDep, this method returns S_OK only if the
 *  current state of this machine object allows to change settings of the
 *  machine (i.e. the machine is not registered, or registered but not running
 *  and not saved). It is useful to call this method from Machine setters
 *  before performing any change.
 *
 *  When @a aDepType is MutableOrSavedStateDep, this method behaves the same
 *  as for MutableStateDep except that if the machine is saved, S_OK is also
 *  returned. This is useful in setters which allow changing machine
 *  properties when it is in the saved state.
 *
 *  When @a aDepType is MutableOrRunningStateDep, this method returns S_OK only
 *  if the current state of this machine object allows to change runtime
 *  changeable settings of the machine (i.e. the machine is not registered, or
 *  registered but either running or not running and not saved). It is useful
 *  to call this method from Machine setters before performing any changes to
 *  runtime changeable settings.
 *
 *  When @a aDepType is MutableOrSavedOrRunningStateDep, this method behaves
 *  the same as for MutableOrRunningStateDep except that if the machine is
 *  saved, S_OK is also returned. This is useful in setters which allow
 *  changing runtime and saved state changeable machine properties.
 *
 *  @param aDepType     Dependency type to check.
 *
 *  @note Non Machine based classes should use #i_addStateDependency() and
 *  #i_releaseStateDependency() methods or the smart AutoStateDependency
 *  template.
 *
 *  @note This method must be called from under this object's read or write
 *        lock.
 */
HRESULT Machine::i_checkStateDependency(StateDependency aDepType)
{
    switch (aDepType)
    {
        case AnyStateDep:
        {
            break;
        }
        case MutableStateDep:
        {
            if (   mData->mRegistered
                && (   !i_isSessionMachine()
                    || (   mData->mMachineState != MachineState_Aborted
                        && mData->mMachineState != MachineState_Teleported
                        && mData->mMachineState != MachineState_PoweredOff
                       )
                   )
               )
                return setError(VBOX_E_INVALID_VM_STATE,
                                tr("The machine is not mutable (state is %s)"),
                                Global::stringifyMachineState(mData->mMachineState));
            break;
        }
        case MutableOrSavedStateDep:
        {
            if (   mData->mRegistered
                && (   !i_isSessionMachine()
                    || (   mData->mMachineState != MachineState_Aborted
                        && mData->mMachineState != MachineState_Teleported
                        && mData->mMachineState != MachineState_Saved
                        && mData->mMachineState != MachineState_PoweredOff
                       )
                   )
               )
                return setError(VBOX_E_INVALID_VM_STATE,
                                tr("The machine is not mutable or saved (state is %s)"),
                                Global::stringifyMachineState(mData->mMachineState));
            break;
        }
        case MutableOrRunningStateDep:
        {
            if (   mData->mRegistered
                && (   !i_isSessionMachine()
                    || (   mData->mMachineState != MachineState_Aborted
                        && mData->mMachineState != MachineState_Teleported
                        && mData->mMachineState != MachineState_PoweredOff
                        && !Global::IsOnline(mData->mMachineState)
                       )
                   )
               )
                return setError(VBOX_E_INVALID_VM_STATE,
                                tr("The machine is not mutable or running (state is %s)"),
                                Global::stringifyMachineState(mData->mMachineState));
            break;
        }
        case MutableOrSavedOrRunningStateDep:
        {
            if (   mData->mRegistered
                && (   !i_isSessionMachine()
                    || (   mData->mMachineState != MachineState_Aborted
                        && mData->mMachineState != MachineState_Teleported
                        && mData->mMachineState != MachineState_Saved
                        && mData->mMachineState != MachineState_PoweredOff
                        && !Global::IsOnline(mData->mMachineState)
                       )
                   )
               )
                return setError(VBOX_E_INVALID_VM_STATE,
                                tr("The machine is not mutable, saved or running (state is %s)"),
                                Global::stringifyMachineState(mData->mMachineState));
            break;
        }
    }

    return S_OK;
}

/**
 * Helper to initialize all associated child objects and allocate data
 * structures.
 *
 * This method must be called as a part of the object's initialization procedure
 * (usually done in the #init() method).
 *
 * @note Must be called only from #init() or from #i_registeredInit().
 */
HRESULT Machine::initDataAndChildObjects()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());
    AssertComRCReturn(   getObjectState().getState() == ObjectState::InInit
                      || getObjectState().getState() == ObjectState::Limited, E_FAIL);

    AssertReturn(!mData->mAccessible, E_FAIL);

    /* allocate data structures */
    mSSData.allocate();
    mUserData.allocate();
    mHWData.allocate();
    mMediumAttachments.allocate();
    mStorageControllers.allocate();
    mUSBControllers.allocate();

    /* initialize mOSTypeId */
    mUserData->s.strOsType = mParent->i_getUnknownOSType()->i_id();

/** @todo r=bird: init() methods never fails, right? Why don't we make them
 *        return void then! */

    /* create associated BIOS settings object */
    unconst(mBIOSSettings).createObject();
    mBIOSSettings->init(this);

    /* create an associated VRDE object (default is disabled) */
    unconst(mVRDEServer).createObject();
    mVRDEServer->init(this);

    /* create associated serial port objects */
    for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
    {
        unconst(mSerialPorts[slot]).createObject();
        mSerialPorts[slot]->init(this, slot);
    }

    /* create associated parallel port objects */
    for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
    {
        unconst(mParallelPorts[slot]).createObject();
        mParallelPorts[slot]->init(this, slot);
    }

    /* create the audio adapter object (always present, default is disabled) */
    unconst(mAudioAdapter).createObject();
    mAudioAdapter->init(this);

    /* create the USB device filters object (always present) */
    unconst(mUSBDeviceFilters).createObject();
    mUSBDeviceFilters->init(this);

    /* create associated network adapter objects */
    mNetworkAdapters.resize(Global::getMaxNetworkAdapters(mHWData->mChipsetType));
    for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
    {
        unconst(mNetworkAdapters[slot]).createObject();
        mNetworkAdapters[slot]->init(this, slot);
    }

    /* create the bandwidth control */
    unconst(mBandwidthControl).createObject();
    mBandwidthControl->init(this);

    return S_OK;
}

/**
 * Helper to uninitialize all associated child objects and to free all data
 * structures.
 *
 * This method must be called as a part of the object's uninitialization
 * procedure (usually done in the #uninit() method).
 *
 * @note Must be called only from #uninit() or from #i_registeredInit().
 */
void Machine::uninitDataAndChildObjects()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());
    AssertComRCReturnVoid(   getObjectState().getState() == ObjectState::InUninit
                          || getObjectState().getState() == ObjectState::Limited);

    /* tell all our other child objects we've been uninitialized */
    if (mBandwidthControl)
    {
        mBandwidthControl->uninit();
        unconst(mBandwidthControl).setNull();
    }

    for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
    {
        if (mNetworkAdapters[slot])
        {
            mNetworkAdapters[slot]->uninit();
            unconst(mNetworkAdapters[slot]).setNull();
        }
    }

    if (mUSBDeviceFilters)
    {
        mUSBDeviceFilters->uninit();
        unconst(mUSBDeviceFilters).setNull();
    }

    if (mAudioAdapter)
    {
        mAudioAdapter->uninit();
        unconst(mAudioAdapter).setNull();
    }

    for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
    {
        if (mParallelPorts[slot])
        {
            mParallelPorts[slot]->uninit();
            unconst(mParallelPorts[slot]).setNull();
        }
    }

    for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
    {
        if (mSerialPorts[slot])
        {
            mSerialPorts[slot]->uninit();
            unconst(mSerialPorts[slot]).setNull();
        }
    }

    if (mVRDEServer)
    {
        mVRDEServer->uninit();
        unconst(mVRDEServer).setNull();
    }

    if (mBIOSSettings)
    {
        mBIOSSettings->uninit();
        unconst(mBIOSSettings).setNull();
    }

    /* Deassociate media (only when a real Machine or a SnapshotMachine
     * instance is uninitialized; SessionMachine instances refer to real
     * Machine media). This is necessary for a clean re-initialization of
     * the VM after successfully re-checking the accessibility state. Note
     * that in case of normal Machine or SnapshotMachine uninitialization (as
     * a result of unregistering or deleting the snapshot), outdated media
     * attachments will already be uninitialized and deleted, so this
     * code will not affect them. */
    if (    !mMediumAttachments.isNull()
         && !i_isSessionMachine()
       )
    {
        for (MediumAttachmentList::const_iterator
             it = mMediumAttachments->begin();
             it != mMediumAttachments->end();
             ++it)
        {
            ComObjPtr<Medium> pMedium = (*it)->i_getMedium();
            if (pMedium.isNull())
                continue;
            HRESULT rc = pMedium->i_removeBackReference(mData->mUuid, i_getSnapshotId());
            AssertComRC(rc);
        }
    }

    if (!i_isSessionMachine() && !i_isSnapshotMachine())
    {
        // clean up the snapshots list (Snapshot::uninit() will handle the snapshot's children recursively)
        if (mData->mFirstSnapshot)
        {
            // snapshots tree is protected by machine write lock; strictly
            // this isn't necessary here since we're deleting the entire
            // machine, but otherwise we assert in Snapshot::uninit()
            AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
            mData->mFirstSnapshot->uninit();
            mData->mFirstSnapshot.setNull();
        }

        mData->mCurrentSnapshot.setNull();
    }

    /* free data structures (the essential mData structure is not freed here
     * since it may be still in use) */
    mMediumAttachments.free();
    mStorageControllers.free();
    mUSBControllers.free();
    mHWData.free();
    mUserData.free();
    mSSData.free();
}

/**
 *  Returns a pointer to the Machine object for this machine that acts like a
 *  parent for complex machine data objects such as shared folders, etc.
 *
 *  For primary Machine objects and for SnapshotMachine objects, returns this
 *  object's pointer itself. For SessionMachine objects, returns the peer
 *  (primary) machine pointer.
 */
Machine *Machine::i_getMachine()
{
    if (i_isSessionMachine())
        return (Machine*)mPeer;
    return this;
}

/**
 * Makes sure that there are no machine state dependents. If necessary, waits
 * for the number of dependents to drop to zero.
 *
 * Make sure this method is called from under this object's write lock to
 * guarantee that no new dependents may be added when this method returns
 * control to the caller.
 *
 * @note Locks this object for writing. The lock will be released while waiting
 *       (if necessary).
 *
 * @warning To be used only in methods that change the machine state!
 */
void Machine::i_ensureNoStateDependencies()
{
    AssertReturnVoid(isWriteLockOnCurrentThread());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Wait for all state dependents if necessary */
    if (mData->mMachineStateDeps != 0)
    {
        /* lazy semaphore creation */
        if (mData->mMachineStateDepsSem == NIL_RTSEMEVENTMULTI)
            RTSemEventMultiCreate(&mData->mMachineStateDepsSem);

        LogFlowThisFunc(("Waiting for state deps (%d) to drop to zero...\n",
                          mData->mMachineStateDeps));

        ++mData->mMachineStateChangePending;

        /* reset the semaphore before waiting, the last dependent will signal
         * it */
        RTSemEventMultiReset(mData->mMachineStateDepsSem);

        alock.release();

        RTSemEventMultiWait(mData->mMachineStateDepsSem, RT_INDEFINITE_WAIT);

        alock.acquire();

        -- mData->mMachineStateChangePending;
    }
}

/**
 * Changes the machine state and informs callbacks.
 *
 * This method is not intended to fail so it either returns S_OK or asserts (and
 * returns a failure).
 *
 * @note Locks this object for writing.
 */
HRESULT Machine::i_setMachineState(MachineState_T aMachineState)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aMachineState=%s\n", Global::stringifyMachineState(aMachineState) ));
    Assert(aMachineState != MachineState_Null);

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* wait for state dependents to drop to zero */
    i_ensureNoStateDependencies();

    MachineState_T const enmOldState = mData->mMachineState;
    if (enmOldState != aMachineState)
    {
        mData->mMachineState = aMachineState;
        RTTimeNow(&mData->mLastStateChange);

#ifdef VBOX_WITH_DTRACE_R3_MAIN
        VBOXAPI_MACHINE_STATE_CHANGED(this, aMachineState, enmOldState, mData->mUuid.toStringCurly().c_str());
#endif
        mParent->i_onMachineStateChange(mData->mUuid, aMachineState);
    }

    LogFlowThisFuncLeave();
    return S_OK;
}

/**
 *  Searches for a shared folder with the given logical name
 *  in the collection of shared folders.
 *
 *  @param aName            logical name of the shared folder
 *  @param aSharedFolder    where to return the found object
 *  @param aSetError        whether to set the error info if the folder is
 *                          not found
 *  @return
 *      S_OK when found or VBOX_E_OBJECT_NOT_FOUND when not found
 *
 *  @note
 *      must be called from under the object's lock!
 */
HRESULT Machine::i_findSharedFolder(const Utf8Str &aName,
                                    ComObjPtr<SharedFolder> &aSharedFolder,
                                    bool aSetError /* = false */)
{
    HRESULT rc = VBOX_E_OBJECT_NOT_FOUND;
    for (HWData::SharedFolderList::const_iterator
         it = mHWData->mSharedFolders.begin();
         it != mHWData->mSharedFolders.end();
         ++it)
    {
        SharedFolder *pSF = *it;
        AutoCaller autoCaller(pSF);
        if (pSF->i_getName() == aName)
        {
            aSharedFolder = pSF;
            rc = S_OK;
            break;
        }
    }

    if (aSetError && FAILED(rc))
        setError(rc, tr("Could not find a shared folder named '%s'"), aName.c_str());

    return rc;
}

/**
 * Initializes all machine instance data from the given settings structures
 * from XML. The exception is the machine UUID which needs special handling
 * depending on the caller's use case, so the caller needs to set that herself.
 *
 * This gets called in several contexts during machine initialization:
 *
 * -- When machine XML exists on disk already and needs to be loaded into memory,
 *    for example, from #i_registeredInit() to load all registered machines on
 *    VirtualBox startup. In this case, puuidRegistry is NULL because the media
 *    attached to the machine should be part of some media registry already.
 *
 * -- During OVF import, when a machine config has been constructed from an
 *    OVF file. In this case, puuidRegistry is set to the machine UUID to
 *    ensure that the media listed as attachments in the config (which have
 *    been imported from the OVF) receive the correct registry ID.
 *
 * -- During VM cloning.
 *
 * @param config Machine settings from XML.
 * @param puuidRegistry If != NULL, Medium::setRegistryIdIfFirst() gets called with this registry ID
 * for each attached medium in the config.
 * @return
 */
HRESULT Machine::i_loadMachineDataFromSettings(const settings::MachineConfigFile &config,
                                               const Guid *puuidRegistry)
{
    // copy name, description, OS type, teleporter, UTC etc.
    mUserData->s = config.machineUserData;

    // look up the object by Id to check it is valid
    ComObjPtr<GuestOSType> pGuestOSType;
    HRESULT rc = mParent->i_findGuestOSType(mUserData->s.strOsType,
                                            pGuestOSType);
    if (FAILED(rc)) return rc;
    mUserData->s.strOsType = pGuestOSType->i_id();

    // stateFile (optional)
    if (config.strStateFile.isEmpty())
        mSSData->strStateFilePath.setNull();
    else
    {
        Utf8Str stateFilePathFull(config.strStateFile);
        int vrc = i_calculateFullPath(stateFilePathFull, stateFilePathFull);
        if (RT_FAILURE(vrc))
            return setError(E_FAIL,
                            tr("Invalid saved state file path '%s' (%Rrc)"),
                            config.strStateFile.c_str(),
                            vrc);
        mSSData->strStateFilePath = stateFilePathFull;
    }

    // snapshot folder needs special processing so set it again
    rc = COMSETTER(SnapshotFolder)(Bstr(config.machineUserData.strSnapshotFolder).raw());
    if (FAILED(rc)) return rc;

    /* Copy the extra data items (config may or may not be the same as
     * mData->pMachineConfigFile) if necessary. When loading the XML files
     * from disk they are the same, but not for OVF import. */
    if (mData->pMachineConfigFile != &config)
        mData->pMachineConfigFile->mapExtraDataItems = config.mapExtraDataItems;

    /* currentStateModified (optional, default is true) */
    mData->mCurrentStateModified = config.fCurrentStateModified;

    mData->mLastStateChange = config.timeLastStateChange;

    /*
     *  note: all mUserData members must be assigned prior this point because
     *  we need to commit changes in order to let mUserData be shared by all
     *  snapshot machine instances.
     */
    mUserData.commitCopy();

    // machine registry, if present (must be loaded before snapshots)
    if (config.canHaveOwnMediaRegistry())
    {
        // determine machine folder
        Utf8Str strMachineFolder = i_getSettingsFileFull();
        strMachineFolder.stripFilename();
        rc = mParent->initMedia(i_getId(),       // media registry ID == machine UUID
                                config.mediaRegistry,
                                strMachineFolder);
        if (FAILED(rc)) return rc;
    }

    /* Snapshot node (optional) */
    size_t cRootSnapshots;
    if ((cRootSnapshots = config.llFirstSnapshot.size()))
    {
        // there must be only one root snapshot
        Assert(cRootSnapshots == 1);

        const settings::Snapshot &snap = config.llFirstSnapshot.front();

        rc = i_loadSnapshot(snap,
                            config.uuidCurrentSnapshot,
                            NULL);        // no parent == first snapshot
        if (FAILED(rc)) return rc;
    }

    // hardware data
    rc = i_loadHardware(puuidRegistry, NULL, config.hardwareMachine, &config.debugging, &config.autostart);
    if (FAILED(rc)) return rc;

    /*
     *  NOTE: the assignment below must be the last thing to do,
     *  otherwise it will be not possible to change the settings
     *  somewhere in the code above because all setters will be
     *  blocked by i_checkStateDependency(MutableStateDep).
     */

    /* set the machine state to Aborted or Saved when appropriate */
    if (config.fAborted)
    {
        mSSData->strStateFilePath.setNull();

        /* no need to use i_setMachineState() during init() */
        mData->mMachineState = MachineState_Aborted;
    }
    else if (!mSSData->strStateFilePath.isEmpty())
    {
        /* no need to use i_setMachineState() during init() */
        mData->mMachineState = MachineState_Saved;
    }

    // after loading settings, we are no longer different from the XML on disk
    mData->flModifications = 0;

    return S_OK;
}

/**
 *  Recursively loads all snapshots starting from the given.
 *
 *  @param data             snapshot settings.
 *  @param aCurSnapshotId   Current snapshot ID from the settings file.
 *  @param aParentSnapshot  Parent snapshot.
 */
HRESULT Machine::i_loadSnapshot(const settings::Snapshot &data,
                                const Guid &aCurSnapshotId,
                                Snapshot *aParentSnapshot)
{
    AssertReturn(!i_isSnapshotMachine(), E_FAIL);
    AssertReturn(!i_isSessionMachine(), E_FAIL);

    HRESULT rc = S_OK;

    Utf8Str strStateFile;
    if (!data.strStateFile.isEmpty())
    {
        /* optional */
        strStateFile = data.strStateFile;
        int vrc = i_calculateFullPath(strStateFile, strStateFile);
        if (RT_FAILURE(vrc))
            return setError(E_FAIL,
                            tr("Invalid saved state file path '%s' (%Rrc)"),
                            strStateFile.c_str(),
                            vrc);
    }

    /* create a snapshot machine object */
    ComObjPtr<SnapshotMachine> pSnapshotMachine;
    pSnapshotMachine.createObject();
    rc = pSnapshotMachine->initFromSettings(this,
                                            data.hardware,
                                            &data.debugging,
                                            &data.autostart,
                                            data.uuid.ref(),
                                            strStateFile);
    if (FAILED(rc)) return rc;

    /* create a snapshot object */
    ComObjPtr<Snapshot> pSnapshot;
    pSnapshot.createObject();
    /* initialize the snapshot */
    rc = pSnapshot->init(mParent, // VirtualBox object
                         data.uuid,
                         data.strName,
                         data.strDescription,
                         data.timestamp,
                         pSnapshotMachine,
                         aParentSnapshot);
    if (FAILED(rc)) return rc;

    /* memorize the first snapshot if necessary */
    if (!mData->mFirstSnapshot)
        mData->mFirstSnapshot = pSnapshot;

    /* memorize the current snapshot when appropriate */
    if (    !mData->mCurrentSnapshot
         && pSnapshot->i_getId() == aCurSnapshotId
       )
        mData->mCurrentSnapshot = pSnapshot;

    // now create the children
    for (settings::SnapshotsList::const_iterator
         it = data.llChildSnapshots.begin();
         it != data.llChildSnapshots.end();
         ++it)
    {
        const settings::Snapshot &childData = *it;
        // recurse
        rc = i_loadSnapshot(childData,
                            aCurSnapshotId,
                            pSnapshot);       // parent = the one we created above
        if (FAILED(rc)) return rc;
    }

    return rc;
}

/**
 *  Loads settings into mHWData.
 *
 * @param puuidRegistry Registry ID.
 * @param puuidSnapshot Snapshot ID
 * @param data          Reference to the hardware settings.
 * @param pDbg          Pointer to the debugging settings.
 * @param pAutostart    Pointer to the autostart settings.
 */
HRESULT Machine::i_loadHardware(const Guid *puuidRegistry,
                                const Guid *puuidSnapshot,
                                const settings::Hardware &data,
                                const settings::Debugging *pDbg,
                                const settings::Autostart *pAutostart)
{
    AssertReturn(!i_isSessionMachine(), E_FAIL);

    HRESULT rc = S_OK;

    try
    {
        ComObjPtr<GuestOSType> pGuestOSType;
        rc = mParent->i_findGuestOSType(Bstr(mUserData->s.strOsType).raw(),
                                        pGuestOSType);
        if (FAILED(rc))
            return rc;

        /* The hardware version attribute (optional). */
        mHWData->mHWVersion = data.strVersion;
        mHWData->mHardwareUUID = data.uuid;

        mHWData->mHWVirtExEnabled             = data.fHardwareVirt;
        mHWData->mHWVirtExNestedPagingEnabled = data.fNestedPaging;
        mHWData->mHWVirtExLargePagesEnabled   = data.fLargePages;
        mHWData->mHWVirtExVPIDEnabled         = data.fVPID;
        mHWData->mHWVirtExUXEnabled           = data.fUnrestrictedExecution;
        mHWData->mHWVirtExForceEnabled        = data.fHardwareVirtForce;
        mHWData->mPAEEnabled                  = data.fPAE;
        mHWData->mLongMode                    = data.enmLongMode;
        mHWData->mTripleFaultReset            = data.fTripleFaultReset;
        mHWData->mAPIC                        = data.fAPIC;
        mHWData->mX2APIC                      = data.fX2APIC;
        mHWData->mIBPBOnVMExit                = data.fIBPBOnVMExit;
        mHWData->mIBPBOnVMEntry               = data.fIBPBOnVMEntry;
        mHWData->mSpecCtrl                    = data.fSpecCtrl;
        mHWData->mSpecCtrlByHost              = data.fSpecCtrlByHost;
        mHWData->mCPUCount                    = data.cCPUs;
        mHWData->mCPUHotPlugEnabled           = data.fCpuHotPlug;
        mHWData->mCpuExecutionCap             = data.ulCpuExecutionCap;
        mHWData->mCpuIdPortabilityLevel       = data.uCpuIdPortabilityLevel;
        mHWData->mCpuProfile                  = data.strCpuProfile;

        // cpu
        if (mHWData->mCPUHotPlugEnabled)
        {
            for (settings::CpuList::const_iterator
                 it = data.llCpus.begin();
                 it != data.llCpus.end();
                 ++it)
            {
                const settings::Cpu &cpu = *it;

                mHWData->mCPUAttached[cpu.ulId] = true;
            }
        }

        // cpuid leafs
        for (settings::CpuIdLeafsList::const_iterator
             it = data.llCpuIdLeafs.begin();
             it != data.llCpuIdLeafs.end();
             ++it)
        {
            const settings::CpuIdLeaf &rLeaf= *it;
            if (   rLeaf.idx < UINT32_C(0x20)
                || rLeaf.idx - UINT32_C(0x80000000) < UINT32_C(0x20)
                || rLeaf.idx - UINT32_C(0xc0000000) < UINT32_C(0x10) )
                mHWData->mCpuIdLeafList.push_back(rLeaf);
            /* else: just ignore */
        }

        mHWData->mMemorySize = data.ulMemorySizeMB;
        mHWData->mPageFusionEnabled = data.fPageFusionEnabled;

        // boot order
        for (unsigned i = 0; i < RT_ELEMENTS(mHWData->mBootOrder); ++i)
        {
            settings::BootOrderMap::const_iterator it = data.mapBootOrder.find(i);
            if (it == data.mapBootOrder.end())
                mHWData->mBootOrder[i] = DeviceType_Null;
            else
                mHWData->mBootOrder[i] = it->second;
        }

        mHWData->mGraphicsControllerType = data.graphicsControllerType;
        mHWData->mVRAMSize      = data.ulVRAMSizeMB;
        mHWData->mMonitorCount  = data.cMonitors;
        mHWData->mAccelerate3DEnabled = data.fAccelerate3D;
        mHWData->mAccelerate2DVideoEnabled = data.fAccelerate2DVideo;
        mHWData->mVideoCaptureWidth = data.ulVideoCaptureHorzRes;
        mHWData->mVideoCaptureHeight = data.ulVideoCaptureVertRes;
        mHWData->mVideoCaptureEnabled = data.fVideoCaptureEnabled;
        for (unsigned i = 0; i < RT_ELEMENTS(mHWData->maVideoCaptureScreens); ++i)
            mHWData->maVideoCaptureScreens[i] = ASMBitTest(&data.u64VideoCaptureScreens, i);
        AssertCompile(RT_ELEMENTS(mHWData->maVideoCaptureScreens) == sizeof(data.u64VideoCaptureScreens) * 8);
        mHWData->mVideoCaptureRate = data.ulVideoCaptureRate;
        mHWData->mVideoCaptureFPS = data.ulVideoCaptureFPS;
        if (!data.strVideoCaptureFile.isEmpty())
            i_calculateFullPath(data.strVideoCaptureFile, mHWData->mVideoCaptureFile);
        else
            mHWData->mVideoCaptureFile.setNull();
        mHWData->mVideoCaptureOptions = data.strVideoCaptureOptions;
        mHWData->mFirmwareType = data.firmwareType;
        mHWData->mPointingHIDType = data.pointingHIDType;
        mHWData->mKeyboardHIDType = data.keyboardHIDType;
        mHWData->mChipsetType = data.chipsetType;
        mHWData->mParavirtProvider = data.paravirtProvider;
        mHWData->mParavirtDebug = data.strParavirtDebug;
        mHWData->mEmulatedUSBCardReaderEnabled = data.fEmulatedUSBCardReader;
        mHWData->mHPETEnabled = data.fHPETEnabled;

        /* VRDEServer */
        rc = mVRDEServer->i_loadSettings(data.vrdeSettings);
        if (FAILED(rc)) return rc;

        /* BIOS */
        rc = mBIOSSettings->i_loadSettings(data.biosSettings);
        if (FAILED(rc)) return rc;

        // Bandwidth control (must come before network adapters)
        rc = mBandwidthControl->i_loadSettings(data.ioSettings);
        if (FAILED(rc)) return rc;

        /* Shared folders */
        for (settings::USBControllerList::const_iterator
             it = data.usbSettings.llUSBControllers.begin();
             it != data.usbSettings.llUSBControllers.end();
             ++it)
        {
            const settings::USBController &settingsCtrl = *it;
            ComObjPtr<USBController> newCtrl;

            newCtrl.createObject();
            newCtrl->init(this, settingsCtrl.strName, settingsCtrl.enmType);
            mUSBControllers->push_back(newCtrl);
        }

        /* USB device filters */
        rc = mUSBDeviceFilters->i_loadSettings(data.usbSettings);
        if (FAILED(rc)) return rc;

        // network adapters (establish array size first and apply defaults, to
        // ensure reading the same settings as we saved, since the list skips
        // adapters having defaults)
        size_t newCount = Global::getMaxNetworkAdapters(mHWData->mChipsetType);
        size_t oldCount = mNetworkAdapters.size();
        if (newCount > oldCount)
        {
            mNetworkAdapters.resize(newCount);
            for (size_t slot = oldCount; slot < mNetworkAdapters.size(); ++slot)
            {
                unconst(mNetworkAdapters[slot]).createObject();
                mNetworkAdapters[slot]->init(this, (ULONG)slot);
            }
        }
        else if (newCount < oldCount)
            mNetworkAdapters.resize(newCount);
        for (unsigned i = 0; i < mNetworkAdapters.size(); i++)
            mNetworkAdapters[i]->i_applyDefaults(pGuestOSType);
        for (settings::NetworkAdaptersList::const_iterator
             it = data.llNetworkAdapters.begin();
             it != data.llNetworkAdapters.end();
             ++it)
        {
            const settings::NetworkAdapter &nic = *it;

            /* slot uniqueness is guaranteed by XML Schema */
            AssertBreak(nic.ulSlot < mNetworkAdapters.size());
            rc = mNetworkAdapters[nic.ulSlot]->i_loadSettings(mBandwidthControl, nic);
            if (FAILED(rc)) return rc;
        }

        // serial ports (establish defaults first, to ensure reading the same
        // settings as we saved, since the list skips ports having defaults)
        for (unsigned i = 0; i < RT_ELEMENTS(mSerialPorts); i++)
            mSerialPorts[i]->i_applyDefaults(pGuestOSType);
        for (settings::SerialPortsList::const_iterator
             it = data.llSerialPorts.begin();
             it != data.llSerialPorts.end();
             ++it)
        {
            const settings::SerialPort &s = *it;

            AssertBreak(s.ulSlot < RT_ELEMENTS(mSerialPorts));
            rc = mSerialPorts[s.ulSlot]->i_loadSettings(s);
            if (FAILED(rc)) return rc;
        }

        // parallel ports (establish defaults first, to ensure reading the same
        // settings as we saved, since the list skips ports having defaults)
        for (unsigned i = 0; i < RT_ELEMENTS(mParallelPorts); i++)
            mParallelPorts[i]->i_applyDefaults();
        for (settings::ParallelPortsList::const_iterator
             it = data.llParallelPorts.begin();
             it != data.llParallelPorts.end();
             ++it)
        {
            const settings::ParallelPort &p = *it;

            AssertBreak(p.ulSlot < RT_ELEMENTS(mParallelPorts));
            rc = mParallelPorts[p.ulSlot]->i_loadSettings(p);
            if (FAILED(rc)) return rc;
        }

        /* AudioAdapter */
        rc = mAudioAdapter->i_loadSettings(data.audioAdapter);
        if (FAILED(rc)) return rc;

        /* storage controllers */
        rc = i_loadStorageControllers(data.storage,
                                      puuidRegistry,
                                      puuidSnapshot);
        if (FAILED(rc)) return rc;

        /* Shared folders */
        for (settings::SharedFoldersList::const_iterator
             it = data.llSharedFolders.begin();
             it != data.llSharedFolders.end();
             ++it)
        {
            const settings::SharedFolder &sf = *it;

            ComObjPtr<SharedFolder> sharedFolder;
            /* Check for double entries. Not allowed! */
            rc = i_findSharedFolder(sf.strName, sharedFolder, false /* aSetError */);
            if (SUCCEEDED(rc))
                return setError(VBOX_E_OBJECT_IN_USE,
                                tr("Shared folder named '%s' already exists"),
                                sf.strName.c_str());

            /* Create the new shared folder. Don't break on error. This will be
             * reported when the machine starts. */
            sharedFolder.createObject();
            rc = sharedFolder->init(i_getMachine(),
                                    sf.strName,
                                    sf.strHostPath,
                                    RT_BOOL(sf.fWritable),
                                    RT_BOOL(sf.fAutoMount),
                                    false /* fFailOnError */);
            if (FAILED(rc)) return rc;
            mHWData->mSharedFolders.push_back(sharedFolder);
        }

        // Clipboard
        mHWData->mClipboardMode = data.clipboardMode;

        // drag'n'drop
        mHWData->mDnDMode = data.dndMode;

        // guest settings
        mHWData->mMemoryBalloonSize = data.ulMemoryBalloonSize;

        // IO settings
        mHWData->mIOCacheEnabled = data.ioSettings.fIOCacheEnabled;
        mHWData->mIOCacheSize = data.ioSettings.ulIOCacheSize;

        // Host PCI devices
        for (settings::HostPCIDeviceAttachmentList::const_iterator
             it = data.pciAttachments.begin();
             it != data.pciAttachments.end();
             ++it)
        {
            const settings::HostPCIDeviceAttachment &hpda = *it;
            ComObjPtr<PCIDeviceAttachment> pda;

            pda.createObject();
            pda->i_loadSettings(this, hpda);
            mHWData->mPCIDeviceAssignments.push_back(pda);
        }

        /*
         * (The following isn't really real hardware, but it lives in HWData
         * for reasons of convenience.)
         */

#ifdef VBOX_WITH_GUEST_PROPS
        /* Guest properties (optional) */

        /* Only load transient guest properties for configs which have saved
         * state, because there shouldn't be any for powered off VMs. The same
         * logic applies for snapshots, as offline snapshots shouldn't have
         * any such properties. They confuse the code in various places.
         * Note: can't rely on the machine state, as it isn't set yet. */
        bool fSkipTransientGuestProperties = mSSData->strStateFilePath.isEmpty();
        /* apologies for the hacky unconst() usage, but this needs hacking
         * actually inconsistent settings into consistency, otherwise there
         * will be some corner cases where the inconsistency survives
         * surprisingly long without getting fixed, especially for snapshots
         * as there are no config changes. */
        settings::GuestPropertiesList &llGuestProperties = unconst(data.llGuestProperties);
        for (settings::GuestPropertiesList::iterator
             it = llGuestProperties.begin();
             it != llGuestProperties.end();
             /*nothing*/)
        {
            const settings::GuestProperty &prop = *it;
            uint32_t fFlags = guestProp::NILFLAG;
            guestProp::validateFlags(prop.strFlags.c_str(), &fFlags);
            if (   fSkipTransientGuestProperties
                && (   fFlags & guestProp::TRANSIENT
                    || fFlags & guestProp::TRANSRESET))
            {
                it = llGuestProperties.erase(it);
                continue;
            }
            HWData::GuestProperty property = { prop.strValue, (LONG64) prop.timestamp, fFlags };
            mHWData->mGuestProperties[prop.strName] = property;
            ++it;
        }
#endif /* VBOX_WITH_GUEST_PROPS defined */

        rc = i_loadDebugging(pDbg);
        if (FAILED(rc))
            return rc;

        mHWData->mAutostart = *pAutostart;

        /* default frontend */
        mHWData->mDefaultFrontend = data.strDefaultFrontend;
    }
    catch (std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }

    AssertComRC(rc);
    return rc;
}

/**
 * Called from i_loadHardware() to load the debugging settings of the
 * machine.
 *
 * @param   pDbg        Pointer to the settings.
 */
HRESULT Machine::i_loadDebugging(const settings::Debugging *pDbg)
{
    mHWData->mDebugging = *pDbg;
    /* no more processing currently required, this will probably change. */
    return S_OK;
}

/**
 *  Called from i_loadMachineDataFromSettings() for the storage controller data, including media.
 *
 * @param data          storage settings.
 * @param puuidRegistry media registry ID to set media to or NULL;
 *                      see Machine::i_loadMachineDataFromSettings()
 * @param puuidSnapshot snapshot ID
 * @return
 */
HRESULT Machine::i_loadStorageControllers(const settings::Storage &data,
                                          const Guid *puuidRegistry,
                                          const Guid *puuidSnapshot)
{
    AssertReturn(!i_isSessionMachine(), E_FAIL);

    HRESULT rc = S_OK;

    for (settings::StorageControllersList::const_iterator
         it = data.llStorageControllers.begin();
         it != data.llStorageControllers.end();
         ++it)
    {
        const settings::StorageController &ctlData = *it;

        ComObjPtr<StorageController> pCtl;
        /* Try to find one with the name first. */
        rc = i_getStorageControllerByName(ctlData.strName, pCtl, false /* aSetError */);
        if (SUCCEEDED(rc))
            return setError(VBOX_E_OBJECT_IN_USE,
                            tr("Storage controller named '%s' already exists"),
                            ctlData.strName.c_str());

        pCtl.createObject();
        rc = pCtl->init(this,
                        ctlData.strName,
                        ctlData.storageBus,
                        ctlData.ulInstance,
                        ctlData.fBootable);
        if (FAILED(rc)) return rc;

        mStorageControllers->push_back(pCtl);

        rc = pCtl->COMSETTER(ControllerType)(ctlData.controllerType);
        if (FAILED(rc)) return rc;

        rc = pCtl->COMSETTER(PortCount)(ctlData.ulPortCount);
        if (FAILED(rc)) return rc;

        rc = pCtl->COMSETTER(UseHostIOCache)(ctlData.fUseHostIOCache);
        if (FAILED(rc)) return rc;

        /* Load the attached devices now. */
        rc = i_loadStorageDevices(pCtl,
                                  ctlData,
                                  puuidRegistry,
                                  puuidSnapshot);
        if (FAILED(rc)) return rc;
    }

    return S_OK;
}

/**
 * Called from i_loadStorageControllers for a controller's devices.
 *
 * @param   aStorageController
 * @param   data
 * @param   puuidRegistry   media registry ID to set media to or NULL; see
 *                          Machine::i_loadMachineDataFromSettings()
 * @param   puuidSnapshot   pointer to the snapshot ID if this is a snapshot machine
 * @return
 */
HRESULT Machine::i_loadStorageDevices(StorageController *aStorageController,
                                      const settings::StorageController &data,
                                      const Guid *puuidRegistry,
                                      const Guid *puuidSnapshot)
{
    HRESULT rc = S_OK;

    /* paranoia: detect duplicate attachments */
    for (settings::AttachedDevicesList::const_iterator
         it = data.llAttachedDevices.begin();
         it != data.llAttachedDevices.end();
         ++it)
    {
        const settings::AttachedDevice &ad = *it;

        for (settings::AttachedDevicesList::const_iterator it2 = it;
             it2 != data.llAttachedDevices.end();
             ++it2)
        {
            if (it == it2)
                continue;

            const settings::AttachedDevice &ad2 = *it2;

            if (   ad.lPort == ad2.lPort
                && ad.lDevice == ad2.lDevice)
            {
                return setError(E_FAIL,
                                tr("Duplicate attachments for storage controller '%s', port %d, device %d of the virtual machine '%s'"),
                                aStorageController->i_getName().c_str(),
                                ad.lPort,
                                ad.lDevice,
                                mUserData->s.strName.c_str());
            }
        }
    }

    for (settings::AttachedDevicesList::const_iterator
         it = data.llAttachedDevices.begin();
         it != data.llAttachedDevices.end();
         ++it)
    {
        const settings::AttachedDevice &dev = *it;
        ComObjPtr<Medium> medium;

        switch (dev.deviceType)
        {
            case DeviceType_Floppy:
            case DeviceType_DVD:
                if (dev.strHostDriveSrc.isNotEmpty())
                    rc = mParent->i_host()->i_findHostDriveByName(dev.deviceType, dev.strHostDriveSrc,
                                                                  false /* fRefresh */, medium);
                else
                    rc = mParent->i_findRemoveableMedium(dev.deviceType,
                                                         dev.uuid,
                                                         false /* fRefresh */,
                                                         false /* aSetError */,
                                                         medium);
                if (rc == VBOX_E_OBJECT_NOT_FOUND)
                    // This is not an error. The host drive or UUID might have vanished, so just go
                    // ahead without this removeable medium attachment
                    rc = S_OK;
            break;

            case DeviceType_HardDisk:
            {
                /* find a hard disk by UUID */
                rc = mParent->i_findHardDiskById(dev.uuid, true /* aDoSetError */, &medium);
                if (FAILED(rc))
                {
                    if (i_isSnapshotMachine())
                    {
                        // wrap another error message around the "cannot find hard disk" set by findHardDisk
                        // so the user knows that the bad disk is in a snapshot somewhere
                        com::ErrorInfo info;
                        return setError(E_FAIL,
                                        tr("A differencing image of snapshot {%RTuuid} could not be found. %ls"),
                                        puuidSnapshot->raw(),
                                        info.getText().raw());
                    }
                    else
                        return rc;
                }

                AutoWriteLock hdLock(medium COMMA_LOCKVAL_SRC_POS);

                if (medium->i_getType() == MediumType_Immutable)
                {
                    if (i_isSnapshotMachine())
                        return setError(E_FAIL,
                                        tr("Immutable hard disk '%s' with UUID {%RTuuid} cannot be directly attached to snapshot with UUID {%RTuuid} "
                                           "of the virtual machine '%s' ('%s')"),
                                        medium->i_getLocationFull().c_str(),
                                        dev.uuid.raw(),
                                        puuidSnapshot->raw(),
                                        mUserData->s.strName.c_str(),
                                        mData->m_strConfigFileFull.c_str());

                    return setError(E_FAIL,
                                    tr("Immutable hard disk '%s' with UUID {%RTuuid} cannot be directly attached to the virtual machine '%s' ('%s')"),
                                    medium->i_getLocationFull().c_str(),
                                    dev.uuid.raw(),
                                    mUserData->s.strName.c_str(),
                                    mData->m_strConfigFileFull.c_str());
                }

                if (medium->i_getType() == MediumType_MultiAttach)
                {
                    if (i_isSnapshotMachine())
                        return setError(E_FAIL,
                                        tr("Multi-attach hard disk '%s' with UUID {%RTuuid} cannot be directly attached to snapshot with UUID {%RTuuid} "
                                           "of the virtual machine '%s' ('%s')"),
                                        medium->i_getLocationFull().c_str(),
                                        dev.uuid.raw(),
                                        puuidSnapshot->raw(),
                                        mUserData->s.strName.c_str(),
                                        mData->m_strConfigFileFull.c_str());

                    return setError(E_FAIL,
                                    tr("Multi-attach hard disk '%s' with UUID {%RTuuid} cannot be directly attached to the virtual machine '%s' ('%s')"),
                                    medium->i_getLocationFull().c_str(),
                                    dev.uuid.raw(),
                                    mUserData->s.strName.c_str(),
                                    mData->m_strConfigFileFull.c_str());
                }

                if (    !i_isSnapshotMachine()
                     && medium->i_getChildren().size() != 0
                   )
                    return setError(E_FAIL,
                                    tr("Hard disk '%s' with UUID {%RTuuid} cannot be directly attached to the virtual machine '%s' ('%s') "
                                       "because it has %d differencing child hard disks"),
                                    medium->i_getLocationFull().c_str(),
                                    dev.uuid.raw(),
                                    mUserData->s.strName.c_str(),
                                    mData->m_strConfigFileFull.c_str(),
                                    medium->i_getChildren().size());

                if (i_findAttachment(*mMediumAttachments.data(),
                                     medium))
                    return setError(E_FAIL,
                                    tr("Hard disk '%s' with UUID {%RTuuid} is already attached to the virtual machine '%s' ('%s')"),
                                    medium->i_getLocationFull().c_str(),
                                    dev.uuid.raw(),
                                    mUserData->s.strName.c_str(),
                                    mData->m_strConfigFileFull.c_str());

                break;
            }

            default:
                return setError(E_FAIL,
                                tr("Device '%s' with unknown type is attached to the virtual machine '%s' ('%s')"),
                                medium->i_getLocationFull().c_str(),
                                mUserData->s.strName.c_str(),
                                mData->m_strConfigFileFull.c_str());
        }

        if (FAILED(rc))
            break;

        /* Bandwidth groups are loaded at this point. */
        ComObjPtr<BandwidthGroup> pBwGroup;

        if (!dev.strBwGroup.isEmpty())
        {
            rc = mBandwidthControl->i_getBandwidthGroupByName(dev.strBwGroup, pBwGroup, false /* aSetError */);
            if (FAILED(rc))
                return setError(E_FAIL,
                                tr("Device '%s' with unknown bandwidth group '%s' is attached to the virtual machine '%s' ('%s')"),
                                medium->i_getLocationFull().c_str(),
                                dev.strBwGroup.c_str(),
                                mUserData->s.strName.c_str(),
                                mData->m_strConfigFileFull.c_str());
            pBwGroup->i_reference();
        }

        const Utf8Str controllerName = aStorageController->i_getName();
        ComObjPtr<MediumAttachment> pAttachment;
        pAttachment.createObject();
        rc = pAttachment->init(this,
                               medium,
                               controllerName,
                               dev.lPort,
                               dev.lDevice,
                               dev.deviceType,
                               false,
                               dev.fPassThrough,
                               dev.fTempEject,
                               dev.fNonRotational,
                               dev.fDiscard,
                               dev.fHotPluggable,
                               pBwGroup.isNull() ? Utf8Str::Empty : pBwGroup->i_getName());
        if (FAILED(rc)) break;

        /* associate the medium with this machine and snapshot */
        if (!medium.isNull())
        {
            AutoCaller medCaller(medium);
            if (FAILED(medCaller.rc())) return medCaller.rc();
            AutoWriteLock mlock(medium COMMA_LOCKVAL_SRC_POS);

            if (i_isSnapshotMachine())
                rc = medium->i_addBackReference(mData->mUuid, *puuidSnapshot);
            else
                rc = medium->i_addBackReference(mData->mUuid);
            /* If the medium->addBackReference fails it sets an appropriate
             * error message, so no need to do any guesswork here. */

            if (puuidRegistry)
                // caller wants registry ID to be set on all attached media (OVF import case)
                medium->i_addRegistry(*puuidRegistry);
        }

        if (FAILED(rc))
            break;

        /* back up mMediumAttachments to let registeredInit() properly rollback
         * on failure (= limited accessibility) */
        i_setModified(IsModified_Storage);
        mMediumAttachments.backup();
        mMediumAttachments->push_back(pAttachment);
    }

    return rc;
}

/**
 *  Returns the snapshot with the given UUID or fails of no such snapshot exists.
 *
 *  @param aId          snapshot UUID to find (empty UUID refers the first snapshot)
 *  @param aSnapshot    where to return the found snapshot
 *  @param aSetError    true to set extended error info on failure
 */
HRESULT Machine::i_findSnapshotById(const Guid &aId,
                                    ComObjPtr<Snapshot> &aSnapshot,
                                    bool aSetError /* = false */)
{
    AutoReadLock chlock(this COMMA_LOCKVAL_SRC_POS);

    if (!mData->mFirstSnapshot)
    {
        if (aSetError)
            return setError(E_FAIL, tr("This machine does not have any snapshots"));
        return E_FAIL;
    }

    if (aId.isZero())
        aSnapshot = mData->mFirstSnapshot;
    else
        aSnapshot = mData->mFirstSnapshot->i_findChildOrSelf(aId.ref());

    if (!aSnapshot)
    {
        if (aSetError)
            return setError(E_FAIL,
                            tr("Could not find a snapshot with UUID {%s}"),
                            aId.toString().c_str());
        return E_FAIL;
    }

    return S_OK;
}

/**
 *  Returns the snapshot with the given name or fails of no such snapshot.
 *
 *  @param strName      snapshot name to find
 *  @param aSnapshot    where to return the found snapshot
 *  @param aSetError    true to set extended error info on failure
 */
HRESULT Machine::i_findSnapshotByName(const Utf8Str &strName,
                                      ComObjPtr<Snapshot> &aSnapshot,
                                      bool aSetError /* = false */)
{
    AssertReturn(!strName.isEmpty(), E_INVALIDARG);

    AutoReadLock chlock(this COMMA_LOCKVAL_SRC_POS);

    if (!mData->mFirstSnapshot)
    {
        if (aSetError)
            return setError(VBOX_E_OBJECT_NOT_FOUND,
                            tr("This machine does not have any snapshots"));
        return VBOX_E_OBJECT_NOT_FOUND;
    }

    aSnapshot = mData->mFirstSnapshot->i_findChildOrSelf(strName);

    if (!aSnapshot)
    {
        if (aSetError)
            return setError(VBOX_E_OBJECT_NOT_FOUND,
                            tr("Could not find a snapshot named '%s'"), strName.c_str());
        return VBOX_E_OBJECT_NOT_FOUND;
    }

    return S_OK;
}

/**
 * Returns a storage controller object with the given name.
 *
 *  @param aName                 storage controller name to find
 *  @param aStorageController    where to return the found storage controller
 *  @param aSetError             true to set extended error info on failure
 */
HRESULT Machine::i_getStorageControllerByName(const Utf8Str &aName,
                                              ComObjPtr<StorageController> &aStorageController,
                                              bool aSetError /* = false */)
{
    AssertReturn(!aName.isEmpty(), E_INVALIDARG);

    for (StorageControllerList::const_iterator
         it = mStorageControllers->begin();
         it != mStorageControllers->end();
         ++it)
    {
        if ((*it)->i_getName() == aName)
        {
            aStorageController = (*it);
            return S_OK;
        }
    }

    if (aSetError)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("Could not find a storage controller named '%s'"),
                        aName.c_str());
    return VBOX_E_OBJECT_NOT_FOUND;
}

/**
 * Returns a USB controller object with the given name.
 *
 *  @param aName                 USB controller name to find
 *  @param aUSBController        where to return the found USB controller
 *  @param aSetError             true to set extended error info on failure
 */
HRESULT Machine::i_getUSBControllerByName(const Utf8Str &aName,
                                          ComObjPtr<USBController> &aUSBController,
                                          bool aSetError /* = false */)
{
    AssertReturn(!aName.isEmpty(), E_INVALIDARG);

    for (USBControllerList::const_iterator
         it = mUSBControllers->begin();
         it != mUSBControllers->end();
         ++it)
    {
        if ((*it)->i_getName() == aName)
        {
            aUSBController = (*it);
            return S_OK;
        }
    }

    if (aSetError)
        return setError(VBOX_E_OBJECT_NOT_FOUND,
                        tr("Could not find a storage controller named '%s'"),
                        aName.c_str());
    return VBOX_E_OBJECT_NOT_FOUND;
}

/**
 * Returns the number of USB controller instance of the given type.
 *
 * @param enmType                USB controller type.
 */
ULONG Machine::i_getUSBControllerCountByType(USBControllerType_T enmType)
{
    ULONG cCtrls = 0;

    for (USBControllerList::const_iterator
         it = mUSBControllers->begin();
         it != mUSBControllers->end();
         ++it)
    {
        if ((*it)->i_getControllerType() == enmType)
            cCtrls++;
    }

    return cCtrls;
}

HRESULT Machine::i_getMediumAttachmentsOfController(const Utf8Str &aName,
                                                    MediumAttachmentList &atts)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it)
    {
        const ComObjPtr<MediumAttachment> &pAtt = *it;
        // should never happen, but deal with NULL pointers in the list.
        AssertContinue(!pAtt.isNull());

        // getControllerName() needs caller+read lock
        AutoCaller autoAttCaller(pAtt);
        if (FAILED(autoAttCaller.rc()))
        {
            atts.clear();
            return autoAttCaller.rc();
        }
        AutoReadLock attLock(pAtt COMMA_LOCKVAL_SRC_POS);

        if (pAtt->i_getControllerName() == aName)
            atts.push_back(pAtt);
    }

    return S_OK;
}


/**
 *  Helper for #i_saveSettings. Cares about renaming the settings directory and
 *  file if the machine name was changed and about creating a new settings file
 *  if this is a new machine.
 *
 *  @note Must be never called directly but only from #saveSettings().
 */
HRESULT Machine::i_prepareSaveSettings(bool *pfNeedsGlobalSaveSettings)
{
    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

    HRESULT rc = S_OK;

    bool fSettingsFileIsNew = !mData->pMachineConfigFile->fileExists();

    /// @todo need to handle primary group change, too

    /* attempt to rename the settings file if machine name is changed */
    if (    mUserData->s.fNameSync
         && mUserData.isBackedUp()
         && (   mUserData.backedUpData()->s.strName != mUserData->s.strName
             || mUserData.backedUpData()->s.llGroups.front() != mUserData->s.llGroups.front())
       )
    {
        bool dirRenamed = false;
        bool fileRenamed = false;

        Utf8Str configFile, newConfigFile;
        Utf8Str configFilePrev, newConfigFilePrev;
        Utf8Str configDir, newConfigDir;

        do
        {
            int vrc = VINF_SUCCESS;

            Utf8Str name = mUserData.backedUpData()->s.strName;
            Utf8Str newName = mUserData->s.strName;
            Utf8Str group = mUserData.backedUpData()->s.llGroups.front();
            if (group == "/")
                group.setNull();
            Utf8Str newGroup = mUserData->s.llGroups.front();
            if (newGroup == "/")
                newGroup.setNull();

            configFile = mData->m_strConfigFileFull;

            /* first, rename the directory if it matches the group and machine name */
            Utf8Str groupPlusName = Utf8StrFmt("%s%c%s",
                group.c_str(), RTPATH_DELIMITER, name.c_str());
            /** @todo hack, make somehow use of ComposeMachineFilename */
            if (mUserData->s.fDirectoryIncludesUUID)
                groupPlusName += Utf8StrFmt(" (%RTuuid)", mData->mUuid.raw());
            Utf8Str newGroupPlusName = Utf8StrFmt("%s%c%s",
                newGroup.c_str(), RTPATH_DELIMITER, newName.c_str());
            /** @todo hack, make somehow use of ComposeMachineFilename */
            if (mUserData->s.fDirectoryIncludesUUID)
                newGroupPlusName += Utf8StrFmt(" (%RTuuid)", mData->mUuid.raw());
            configDir = configFile;
            configDir.stripFilename();
            newConfigDir = configDir;
            if (   configDir.length() >= groupPlusName.length()
                && !RTPathCompare(configDir.substr(configDir.length() - groupPlusName.length(), groupPlusName.length()).c_str(),
                                  groupPlusName.c_str()))
            {
                newConfigDir = newConfigDir.substr(0, configDir.length() - groupPlusName.length());
                Utf8Str newConfigBaseDir(newConfigDir);
                newConfigDir.append(newGroupPlusName);
                /* consistency: use \ if appropriate on the platform */
                RTPathChangeToDosSlashes(newConfigDir.mutableRaw(), false);
                /* new dir and old dir cannot be equal here because of 'if'
                 * above and because name != newName */
                Assert(configDir != newConfigDir);
                if (!fSettingsFileIsNew)
                {
                    /* perform real rename only if the machine is not new */
                    vrc = RTPathRename(configDir.c_str(), newConfigDir.c_str(), 0);
                    if (   vrc == VERR_FILE_NOT_FOUND
                        || vrc == VERR_PATH_NOT_FOUND)
                    {
                        /* create the parent directory, then retry renaming */
                        Utf8Str parent(newConfigDir);
                        parent.stripFilename();
                        (void)RTDirCreateFullPath(parent.c_str(), 0700);
                        vrc = RTPathRename(configDir.c_str(), newConfigDir.c_str(), 0);
                    }
                    if (RT_FAILURE(vrc))
                    {
                        rc = setError(E_FAIL,
                                      tr("Could not rename the directory '%s' to '%s' to save the settings file (%Rrc)"),
                                      configDir.c_str(),
                                      newConfigDir.c_str(),
                                      vrc);
                        break;
                    }
                    /* delete subdirectories which are no longer needed */
                    Utf8Str dir(configDir);
                    dir.stripFilename();
                    while (dir != newConfigBaseDir && dir != ".")
                    {
                        vrc = RTDirRemove(dir.c_str());
                        if (RT_FAILURE(vrc))
                            break;
                        dir.stripFilename();
                    }
                    dirRenamed = true;
                }
            }

            newConfigFile = Utf8StrFmt("%s%c%s.vbox",
                newConfigDir.c_str(), RTPATH_DELIMITER, newName.c_str());

            /* then try to rename the settings file itself */
            if (newConfigFile != configFile)
            {
                /* get the path to old settings file in renamed directory */
                configFile = Utf8StrFmt("%s%c%s",
                                        newConfigDir.c_str(),
                                        RTPATH_DELIMITER,
                                        RTPathFilename(configFile.c_str()));
                if (!fSettingsFileIsNew)
                {
                    /* perform real rename only if the machine is not new */
                    vrc = RTFileRename(configFile.c_str(), newConfigFile.c_str(), 0);
                    if (RT_FAILURE(vrc))
                    {
                        rc = setError(E_FAIL,
                                      tr("Could not rename the settings file '%s' to '%s' (%Rrc)"),
                                      configFile.c_str(),
                                      newConfigFile.c_str(),
                                      vrc);
                        break;
                    }
                    fileRenamed = true;
                    configFilePrev = configFile;
                    configFilePrev += "-prev";
                    newConfigFilePrev = newConfigFile;
                    newConfigFilePrev += "-prev";
                    RTFileRename(configFilePrev.c_str(), newConfigFilePrev.c_str(), 0);
                }
            }

            // update m_strConfigFileFull amd mConfigFile
            mData->m_strConfigFileFull = newConfigFile;
            // compute the relative path too
            mParent->i_copyPathRelativeToConfig(newConfigFile, mData->m_strConfigFile);

            // store the old and new so that VirtualBox::i_saveSettings() can update
            // the media registry
            if (    mData->mRegistered
                 && (configDir != newConfigDir || configFile != newConfigFile))
            {
                mParent->i_rememberMachineNameChangeForMedia(configDir, newConfigDir);

                if (pfNeedsGlobalSaveSettings)
                    *pfNeedsGlobalSaveSettings = true;
            }

            // in the saved state file path, replace the old directory with the new directory
            if (RTPathStartsWith(mSSData->strStateFilePath.c_str(), configDir.c_str()))
            {
                Utf8Str strStateFileName = mSSData->strStateFilePath.c_str() + configDir.length();
                mSSData->strStateFilePath = newConfigDir + strStateFileName;
            }

            // and do the same thing for the saved state file paths of all the online snapshots
            if (mData->mFirstSnapshot)
                mData->mFirstSnapshot->i_updateSavedStatePaths(configDir.c_str(),
                                                               newConfigDir.c_str());
        }
        while (0);

        if (FAILED(rc))
        {
            /* silently try to rename everything back */
            if (fileRenamed)
            {
                RTFileRename(newConfigFilePrev.c_str(), configFilePrev.c_str(), 0);
                RTFileRename(newConfigFile.c_str(), configFile.c_str(), 0);
            }
            if (dirRenamed)
                RTPathRename(newConfigDir.c_str(), configDir.c_str(), 0);
        }

        if (FAILED(rc)) return rc;
    }

    if (fSettingsFileIsNew)
    {
        /* create a virgin config file */
        int vrc = VINF_SUCCESS;

        /* ensure the settings directory exists */
        Utf8Str path(mData->m_strConfigFileFull);
        path.stripFilename();
        if (!RTDirExists(path.c_str()))
        {
            vrc = RTDirCreateFullPath(path.c_str(), 0700);
            if (RT_FAILURE(vrc))
            {
                return setError(E_FAIL,
                                tr("Could not create a directory '%s' to save the settings file (%Rrc)"),
                                path.c_str(),
                                vrc);
            }
        }

        /* Note: open flags must correlate with RTFileOpen() in lockConfig() */
        path = Utf8Str(mData->m_strConfigFileFull);
        RTFILE f = NIL_RTFILE;
        vrc = RTFileOpen(&f, path.c_str(),
                         RTFILE_O_READWRITE | RTFILE_O_CREATE | RTFILE_O_DENY_WRITE);
        if (RT_FAILURE(vrc))
            return setError(E_FAIL,
                            tr("Could not create the settings file '%s' (%Rrc)"),
                            path.c_str(),
                            vrc);
        RTFileClose(f);
    }

    return rc;
}

/**
 * Saves and commits machine data, user data and hardware data.
 *
 * Note that on failure, the data remains uncommitted.
 *
 * @a aFlags may combine the following flags:
 *
 *  - SaveS_ResetCurStateModified: Resets mData->mCurrentStateModified to FALSE.
 *    Used when saving settings after an operation that makes them 100%
 *    correspond to the settings from the current snapshot.
 *  - SaveS_Force: settings will be saved without doing a deep compare of the
 *    settings structures. This is used when this is called because snapshots
 *    have changed to avoid the overhead of the deep compare.
 *
 * @note Must be called from under this object's write lock. Locks children for
 * writing.
 *
 * @param pfNeedsGlobalSaveSettings Optional pointer to a bool that must have been
 *          initialized to false and that will be set to true by this function if
 *          the caller must invoke VirtualBox::i_saveSettings() because the global
 *          settings have changed. This will happen if a machine rename has been
 *          saved and the global machine and media registries will therefore need
 *          updating.
 * @param   aFlags  Flags.
 */
HRESULT Machine::i_saveSettings(bool *pfNeedsGlobalSaveSettings,
                                int  aFlags /*= 0*/)
{
    LogFlowThisFuncEnter();

    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

    /* make sure child objects are unable to modify the settings while we are
     * saving them */
    i_ensureNoStateDependencies();

    AssertReturn(!i_isSnapshotMachine(),
                 E_FAIL);

    HRESULT rc = S_OK;
    bool fNeedsWrite = false;

    /* First, prepare to save settings. It will care about renaming the
     * settings directory and file if the machine name was changed and about
     * creating a new settings file if this is a new machine. */
    rc = i_prepareSaveSettings(pfNeedsGlobalSaveSettings);
    if (FAILED(rc)) return rc;

    // keep a pointer to the current settings structures
    settings::MachineConfigFile *pOldConfig = mData->pMachineConfigFile;
    settings::MachineConfigFile *pNewConfig = NULL;

    try
    {
        // make a fresh one to have everyone write stuff into
        pNewConfig = new settings::MachineConfigFile(NULL);
        pNewConfig->copyBaseFrom(*mData->pMachineConfigFile);

        // now go and copy all the settings data from COM to the settings structures
        // (this calls i_saveSettings() on all the COM objects in the machine)
        i_copyMachineDataToSettings(*pNewConfig);

        if (aFlags & SaveS_ResetCurStateModified)
        {
            // this gets set by takeSnapshot() (if offline snapshot) and restoreSnapshot()
            mData->mCurrentStateModified = FALSE;
            fNeedsWrite = true;     // always, no need to compare
        }
        else if (aFlags & SaveS_Force)
        {
            fNeedsWrite = true;     // always, no need to compare
        }
        else
        {
            if (!mData->mCurrentStateModified)
            {
                // do a deep compare of the settings that we just saved with the settings
                // previously stored in the config file; this invokes MachineConfigFile::operator==
                // which does a deep compare of all the settings, which is expensive but less expensive
                // than writing out XML in vain
                bool fAnySettingsChanged = !(*pNewConfig == *pOldConfig);

                // could still be modified if any settings changed
                mData->mCurrentStateModified = fAnySettingsChanged;

                fNeedsWrite = fAnySettingsChanged;
            }
            else
                fNeedsWrite = true;
        }

        pNewConfig->fCurrentStateModified = !!mData->mCurrentStateModified;

        if (fNeedsWrite)
            // now spit it all out!
            pNewConfig->write(mData->m_strConfigFileFull);

        mData->pMachineConfigFile = pNewConfig;
        delete pOldConfig;
        i_commit();

        // after saving settings, we are no longer different from the XML on disk
        mData->flModifications = 0;
    }
    catch (HRESULT err)
    {
        // we assume that error info is set by the thrower
        rc = err;

        // restore old config
        delete pNewConfig;
        mData->pMachineConfigFile = pOldConfig;
    }
    catch (...)
    {
        rc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
    }

    if (fNeedsWrite)
    {
        /* Fire the data change event, even on failure (since we've already
         * committed all data). This is done only for SessionMachines because
         * mutable Machine instances are always not registered (i.e. private
         * to the client process that creates them) and thus don't need to
         * inform callbacks. */
        if (i_isSessionMachine())
            mParent->i_onMachineDataChange(mData->mUuid);
    }

    LogFlowThisFunc(("rc=%08X\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

/**
 * Implementation for saving the machine settings into the given
 * settings::MachineConfigFile instance. This copies machine extradata
 * from the previous machine config file in the instance data, if any.
 *
 * This gets called from two locations:
 *
 *  --  Machine::i_saveSettings(), during the regular XML writing;
 *
 *  --  Appliance::buildXMLForOneVirtualSystem(), when a machine gets
 *      exported to OVF and we write the VirtualBox proprietary XML
 *      into a <vbox:Machine> tag.
 *
 * This routine fills all the fields in there, including snapshots, *except*
 * for the following:
 *
 * -- fCurrentStateModified. There is some special logic associated with that.
 *
 * The caller can then call MachineConfigFile::write() or do something else
 * with it.
 *
 * Caller must hold the machine lock!
 *
 * This throws XML errors and HRESULT, so the caller must have a catch block!
 */
void Machine::i_copyMachineDataToSettings(settings::MachineConfigFile &config)
{
    // deep copy extradata, being extra careful with self assignment (the STL
    // map assignment on Mac OS X clang based Xcode isn't checking)
    if (&config != mData->pMachineConfigFile)
        config.mapExtraDataItems = mData->pMachineConfigFile->mapExtraDataItems;

    config.uuid = mData->mUuid;

    // copy name, description, OS type, teleport, UTC etc.
    config.machineUserData = mUserData->s;

    if (    mData->mMachineState == MachineState_Saved
         || mData->mMachineState == MachineState_Restoring
            // when doing certain snapshot operations we may or may not have
            // a saved state in the current state, so keep everything as is
         || (    (   mData->mMachineState == MachineState_Snapshotting
                  || mData->mMachineState == MachineState_DeletingSnapshot
                  || mData->mMachineState == MachineState_RestoringSnapshot)
              && (!mSSData->strStateFilePath.isEmpty())
            )
        )
    {
        Assert(!mSSData->strStateFilePath.isEmpty());
        /* try to make the file name relative to the settings file dir */
        i_copyPathRelativeToMachine(mSSData->strStateFilePath, config.strStateFile);
    }
    else
    {
        Assert(mSSData->strStateFilePath.isEmpty() || mData->mMachineState == MachineState_Saving);
        config.strStateFile.setNull();
    }

    if (mData->mCurrentSnapshot)
        config.uuidCurrentSnapshot = mData->mCurrentSnapshot->i_getId();
    else
        config.uuidCurrentSnapshot.clear();

    config.timeLastStateChange = mData->mLastStateChange;
    config.fAborted = (mData->mMachineState == MachineState_Aborted);
    /// @todo Live Migration:        config.fTeleported = (mData->mMachineState == MachineState_Teleported);

    HRESULT rc = i_saveHardware(config.hardwareMachine, &config.debugging, &config.autostart);
    if (FAILED(rc)) throw rc;

    // save machine's media registry if this is VirtualBox 4.0 or later
    if (config.canHaveOwnMediaRegistry())
    {
        // determine machine folder
        Utf8Str strMachineFolder = i_getSettingsFileFull();
        strMachineFolder.stripFilename();
        mParent->i_saveMediaRegistry(config.mediaRegistry,
                                     i_getId(),             // only media with registry ID == machine UUID
                                     strMachineFolder);
            // this throws HRESULT
    }

    // save snapshots
    rc = i_saveAllSnapshots(config);
    if (FAILED(rc)) throw rc;
}

/**
 * Saves all snapshots of the machine into the given machine config file. Called
 * from Machine::buildMachineXML() and SessionMachine::deleteSnapshotHandler().
 * @param config
 * @return
 */
HRESULT Machine::i_saveAllSnapshots(settings::MachineConfigFile &config)
{
    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

    HRESULT rc = S_OK;

    try
    {
        config.llFirstSnapshot.clear();

        if (mData->mFirstSnapshot)
        {
            // the settings use a list for "the first snapshot"
            config.llFirstSnapshot.push_back(settings::Snapshot::Empty);

            // get reference to the snapshot on the list and work on that
            // element straight in the list to avoid excessive copying later
            rc = mData->mFirstSnapshot->i_saveSnapshot(config.llFirstSnapshot.back());
            if (FAILED(rc)) throw rc;
        }

//         if (mType == IsSessionMachine)
//             mParent->onMachineDataChange(mData->mUuid);          @todo is this necessary?

    }
    catch (HRESULT err)
    {
        /* we assume that error info is set by the thrower */
        rc = err;
    }
    catch (...)
    {
        rc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
    }

    return rc;
}

/**
 *  Saves the VM hardware configuration. It is assumed that the
 *  given node is empty.
 *
 *  @param data           Reference to the settings object for the hardware config.
 *  @param pDbg           Pointer to the settings object for the debugging config
 *                        which happens to live in mHWData.
 *  @param pAutostart     Pointer to the settings object for the autostart config
 *                        which happens to live in mHWData.
 */
HRESULT Machine::i_saveHardware(settings::Hardware &data, settings::Debugging *pDbg,
                                settings::Autostart *pAutostart)
{
    HRESULT rc = S_OK;

    try
    {
        /* The hardware version attribute (optional).
           Automatically upgrade from 1 to current default hardware version
           when there is no saved state. (ugly!) */
        if (    mHWData->mHWVersion == "1"
             && mSSData->strStateFilePath.isEmpty()
           )
            mHWData->mHWVersion = Utf8StrFmt("%d", SchemaDefs::DefaultHardwareVersion);

        data.strVersion = mHWData->mHWVersion;
        data.uuid = mHWData->mHardwareUUID;

        // CPU
        data.fHardwareVirt          = !!mHWData->mHWVirtExEnabled;
        data.fNestedPaging          = !!mHWData->mHWVirtExNestedPagingEnabled;
        data.fLargePages            = !!mHWData->mHWVirtExLargePagesEnabled;
        data.fVPID                  = !!mHWData->mHWVirtExVPIDEnabled;
        data.fUnrestrictedExecution = !!mHWData->mHWVirtExUXEnabled;
        data.fHardwareVirtForce     = !!mHWData->mHWVirtExForceEnabled;
        data.fPAE                   = !!mHWData->mPAEEnabled;
        data.enmLongMode            = mHWData->mLongMode;
        data.fTripleFaultReset      = !!mHWData->mTripleFaultReset;
        data.fAPIC                  = !!mHWData->mAPIC;
        data.fX2APIC                = !!mHWData->mX2APIC;
        data.fIBPBOnVMExit          = !!mHWData->mIBPBOnVMExit;
        data.fIBPBOnVMEntry         = !!mHWData->mIBPBOnVMEntry;
        data.fSpecCtrl              = !!mHWData->mSpecCtrl;
        data.fSpecCtrlByHost        = !!mHWData->mSpecCtrlByHost;
        data.cCPUs                  = mHWData->mCPUCount;
        data.fCpuHotPlug            = !!mHWData->mCPUHotPlugEnabled;
        data.ulCpuExecutionCap      = mHWData->mCpuExecutionCap;
        data.uCpuIdPortabilityLevel = mHWData->mCpuIdPortabilityLevel;
        data.strCpuProfile          = mHWData->mCpuProfile;

        data.llCpus.clear();
        if (data.fCpuHotPlug)
        {
            for (unsigned idx = 0; idx < data.cCPUs; ++idx)
            {
                if (mHWData->mCPUAttached[idx])
                {
                    settings::Cpu cpu;
                    cpu.ulId = idx;
                    data.llCpus.push_back(cpu);
                }
            }
        }

        /* Standard and Extended CPUID leafs. */
        data.llCpuIdLeafs.clear();
        data.llCpuIdLeafs = mHWData->mCpuIdLeafList;

        // memory
        data.ulMemorySizeMB = mHWData->mMemorySize;
        data.fPageFusionEnabled = !!mHWData->mPageFusionEnabled;

        // firmware
        data.firmwareType = mHWData->mFirmwareType;

        // HID
        data.pointingHIDType = mHWData->mPointingHIDType;
        data.keyboardHIDType = mHWData->mKeyboardHIDType;

        // chipset
        data.chipsetType = mHWData->mChipsetType;

        // paravirt
        data.paravirtProvider = mHWData->mParavirtProvider;
        data.strParavirtDebug = mHWData->mParavirtDebug;

        // emulated USB card reader
        data.fEmulatedUSBCardReader = !!mHWData->mEmulatedUSBCardReaderEnabled;

        // HPET
        data.fHPETEnabled = !!mHWData->mHPETEnabled;

        // boot order
        data.mapBootOrder.clear();
        for (unsigned i = 0; i < RT_ELEMENTS(mHWData->mBootOrder); ++i)
            data.mapBootOrder[i] = mHWData->mBootOrder[i];

        // display
        data.graphicsControllerType = mHWData->mGraphicsControllerType;
        data.ulVRAMSizeMB = mHWData->mVRAMSize;
        data.cMonitors = mHWData->mMonitorCount;
        data.fAccelerate3D = !!mHWData->mAccelerate3DEnabled;
        data.fAccelerate2DVideo = !!mHWData->mAccelerate2DVideoEnabled;
        data.ulVideoCaptureHorzRes = mHWData->mVideoCaptureWidth;
        data.ulVideoCaptureVertRes = mHWData->mVideoCaptureHeight;
        data.ulVideoCaptureRate = mHWData->mVideoCaptureRate;
        data.ulVideoCaptureFPS = mHWData->mVideoCaptureFPS;
        data.fVideoCaptureEnabled  = !!mHWData->mVideoCaptureEnabled;
        for (unsigned i = 0; i < sizeof(data.u64VideoCaptureScreens) * 8; ++i)
        {
            if (mHWData->maVideoCaptureScreens[i])
                ASMBitSet(&data.u64VideoCaptureScreens, i);
            else
                ASMBitClear(&data.u64VideoCaptureScreens, i);
        }
        /* store relative video capture file if possible */
        i_copyPathRelativeToMachine(mHWData->mVideoCaptureFile, data.strVideoCaptureFile);
        data.strVideoCaptureOptions = mHWData->mVideoCaptureOptions;

        /* VRDEServer settings (optional) */
        rc = mVRDEServer->i_saveSettings(data.vrdeSettings);
        if (FAILED(rc)) throw rc;

        /* BIOS (required) */
        rc = mBIOSSettings->i_saveSettings(data.biosSettings);
        if (FAILED(rc)) throw rc;

        /* USB Controller (required) */
        data.usbSettings.llUSBControllers.clear();
        for (USBControllerList::const_iterator
             it = mUSBControllers->begin();
             it != mUSBControllers->end();
             ++it)
        {
            ComObjPtr<USBController> ctrl = *it;
            settings::USBController settingsCtrl;

            settingsCtrl.strName = ctrl->i_getName();
            settingsCtrl.enmType = ctrl->i_getControllerType();

            data.usbSettings.llUSBControllers.push_back(settingsCtrl);
        }

        /* USB device filters (required) */
        rc = mUSBDeviceFilters->i_saveSettings(data.usbSettings);
        if (FAILED(rc)) throw rc;

        /* Network adapters (required) */
        size_t uMaxNICs = RT_MIN(Global::getMaxNetworkAdapters(mHWData->mChipsetType), mNetworkAdapters.size());
        data.llNetworkAdapters.clear();
        /* Write out only the nominal number of network adapters for this
         * chipset type. Since Machine::commit() hasn't been called there
         * may be extra NIC settings in the vector. */
        for (size_t slot = 0; slot < uMaxNICs; ++slot)
        {
            settings::NetworkAdapter nic;
            nic.ulSlot = (uint32_t)slot;
            /* paranoia check... must not be NULL, but must not crash either. */
            if (mNetworkAdapters[slot])
            {
                if (mNetworkAdapters[slot]->i_hasDefaults())
                    continue;

                rc = mNetworkAdapters[slot]->i_saveSettings(nic);
                if (FAILED(rc)) throw rc;

                data.llNetworkAdapters.push_back(nic);
            }
        }

        /* Serial ports */
        data.llSerialPorts.clear();
        for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
        {
            if (mSerialPorts[slot]->i_hasDefaults())
                continue;

            settings::SerialPort s;
            s.ulSlot = slot;
            rc = mSerialPorts[slot]->i_saveSettings(s);
            if (FAILED(rc)) return rc;

            data.llSerialPorts.push_back(s);
        }

        /* Parallel ports */
        data.llParallelPorts.clear();
        for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
        {
            if (mParallelPorts[slot]->i_hasDefaults())
                continue;

            settings::ParallelPort p;
            p.ulSlot = slot;
            rc = mParallelPorts[slot]->i_saveSettings(p);
            if (FAILED(rc)) return rc;

            data.llParallelPorts.push_back(p);
        }

        /* Audio adapter */
        rc = mAudioAdapter->i_saveSettings(data.audioAdapter);
        if (FAILED(rc)) return rc;

        rc = i_saveStorageControllers(data.storage);
        if (FAILED(rc)) return rc;

        /* Shared folders */
        data.llSharedFolders.clear();
        for (HWData::SharedFolderList::const_iterator
             it = mHWData->mSharedFolders.begin();
             it != mHWData->mSharedFolders.end();
             ++it)
        {
            SharedFolder *pSF = *it;
            AutoCaller sfCaller(pSF);
            AutoReadLock sfLock(pSF COMMA_LOCKVAL_SRC_POS);
            settings::SharedFolder sf;
            sf.strName = pSF->i_getName();
            sf.strHostPath = pSF->i_getHostPath();
            sf.fWritable = !!pSF->i_isWritable();
            sf.fAutoMount = !!pSF->i_isAutoMounted();

            data.llSharedFolders.push_back(sf);
        }

        // clipboard
        data.clipboardMode = mHWData->mClipboardMode;

        // drag'n'drop
        data.dndMode = mHWData->mDnDMode;

        /* Guest */
        data.ulMemoryBalloonSize = mHWData->mMemoryBalloonSize;

        // IO settings
        data.ioSettings.fIOCacheEnabled = !!mHWData->mIOCacheEnabled;
        data.ioSettings.ulIOCacheSize = mHWData->mIOCacheSize;

        /* BandwidthControl (required) */
        rc = mBandwidthControl->i_saveSettings(data.ioSettings);
        if (FAILED(rc)) throw rc;

        /* Host PCI devices */
        data.pciAttachments.clear();
        for (HWData::PCIDeviceAssignmentList::const_iterator
             it = mHWData->mPCIDeviceAssignments.begin();
             it != mHWData->mPCIDeviceAssignments.end();
             ++it)
        {
            ComObjPtr<PCIDeviceAttachment> pda = *it;
            settings::HostPCIDeviceAttachment hpda;

            rc = pda->i_saveSettings(hpda);
            if (FAILED(rc)) throw rc;

            data.pciAttachments.push_back(hpda);
        }

        // guest properties
        data.llGuestProperties.clear();
#ifdef VBOX_WITH_GUEST_PROPS
        for (HWData::GuestPropertyMap::const_iterator
             it = mHWData->mGuestProperties.begin();
             it != mHWData->mGuestProperties.end();
             ++it)
        {
            HWData::GuestProperty property = it->second;

            /* Remove transient guest properties at shutdown unless we
             * are saving state. Note that restoring snapshot intentionally
             * keeps them, they will be removed if appropriate once the final
             * machine state is set (as crashes etc. need to work). */
            if (   (   mData->mMachineState == MachineState_PoweredOff
                    || mData->mMachineState == MachineState_Aborted
                    || mData->mMachineState == MachineState_Teleported)
                && (   property.mFlags & guestProp::TRANSIENT
                    || property.mFlags & guestProp::TRANSRESET))
                continue;
            settings::GuestProperty prop;
            prop.strName = it->first;
            prop.strValue = property.strValue;
            prop.timestamp = property.mTimestamp;
            char szFlags[guestProp::MAX_FLAGS_LEN + 1];
            guestProp::writeFlags(property.mFlags, szFlags);
            prop.strFlags = szFlags;

            data.llGuestProperties.push_back(prop);
        }

        /* I presume this doesn't require a backup(). */
        mData->mGuestPropertiesModified = FALSE;
#endif /* VBOX_WITH_GUEST_PROPS defined */

        *pDbg = mHWData->mDebugging;
        *pAutostart = mHWData->mAutostart;

        data.strDefaultFrontend = mHWData->mDefaultFrontend;
    }
    catch (std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }

    AssertComRC(rc);
    return rc;
}

/**
 *  Saves the storage controller configuration.
 *
 *  @param data    storage settings.
 */
HRESULT Machine::i_saveStorageControllers(settings::Storage &data)
{
    data.llStorageControllers.clear();

    for (StorageControllerList::const_iterator
         it = mStorageControllers->begin();
         it != mStorageControllers->end();
         ++it)
    {
        HRESULT rc;
        ComObjPtr<StorageController> pCtl = *it;

        settings::StorageController ctl;
        ctl.strName = pCtl->i_getName();
        ctl.controllerType = pCtl->i_getControllerType();
        ctl.storageBus = pCtl->i_getStorageBus();
        ctl.ulInstance = pCtl->i_getInstance();
        ctl.fBootable = pCtl->i_getBootable();

        /* Save the port count. */
        ULONG portCount;
        rc = pCtl->COMGETTER(PortCount)(&portCount);
        ComAssertComRCRet(rc, rc);
        ctl.ulPortCount = portCount;

        /* Save fUseHostIOCache */
        BOOL fUseHostIOCache;
        rc = pCtl->COMGETTER(UseHostIOCache)(&fUseHostIOCache);
        ComAssertComRCRet(rc, rc);
        ctl.fUseHostIOCache = !!fUseHostIOCache;

        /* save the devices now. */
        rc = i_saveStorageDevices(pCtl, ctl);
        ComAssertComRCRet(rc, rc);

        data.llStorageControllers.push_back(ctl);
    }

    return S_OK;
}

/**
 *  Saves the hard disk configuration.
 */
HRESULT Machine::i_saveStorageDevices(ComObjPtr<StorageController> aStorageController,
                                      settings::StorageController &data)
{
    MediumAttachmentList atts;

    HRESULT rc = i_getMediumAttachmentsOfController(aStorageController->i_getName(), atts);
    if (FAILED(rc)) return rc;

    data.llAttachedDevices.clear();
    for (MediumAttachmentList::const_iterator
         it = atts.begin();
         it != atts.end();
         ++it)
    {
        settings::AttachedDevice dev;
        IMediumAttachment *iA = *it;
        MediumAttachment *pAttach = static_cast<MediumAttachment *>(iA);
        Medium *pMedium = pAttach->i_getMedium();

        dev.deviceType = pAttach->i_getType();
        dev.lPort = pAttach->i_getPort();
        dev.lDevice = pAttach->i_getDevice();
        dev.fPassThrough = pAttach->i_getPassthrough();
        dev.fHotPluggable = pAttach->i_getHotPluggable();
        if (pMedium)
        {
            if (pMedium->i_isHostDrive())
                dev.strHostDriveSrc = pMedium->i_getLocationFull();
            else
                dev.uuid = pMedium->i_getId();
            dev.fTempEject = pAttach->i_getTempEject();
            dev.fNonRotational = pAttach->i_getNonRotational();
            dev.fDiscard = pAttach->i_getDiscard();
        }

        dev.strBwGroup = pAttach->i_getBandwidthGroup();

        data.llAttachedDevices.push_back(dev);
    }

    return S_OK;
}

/**
 *  Saves machine state settings as defined by aFlags
 *  (SaveSTS_* values).
 *
 *  @param aFlags   Combination of SaveSTS_* flags.
 *
 *  @note Locks objects for writing.
 */
HRESULT Machine::i_saveStateSettings(int aFlags)
{
    if (aFlags == 0)
        return S_OK;

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    /* This object's write lock is also necessary to serialize file access
     * (prevent concurrent reads and writes) */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    Assert(mData->pMachineConfigFile);

    try
    {
        if (aFlags & SaveSTS_CurStateModified)
            mData->pMachineConfigFile->fCurrentStateModified = true;

        if (aFlags & SaveSTS_StateFilePath)
        {
            if (!mSSData->strStateFilePath.isEmpty())
                /* try to make the file name relative to the settings file dir */
                i_copyPathRelativeToMachine(mSSData->strStateFilePath, mData->pMachineConfigFile->strStateFile);
            else
                mData->pMachineConfigFile->strStateFile.setNull();
        }

        if (aFlags & SaveSTS_StateTimeStamp)
        {
            Assert(    mData->mMachineState != MachineState_Aborted
                    || mSSData->strStateFilePath.isEmpty());

            mData->pMachineConfigFile->timeLastStateChange = mData->mLastStateChange;

            mData->pMachineConfigFile->fAborted = (mData->mMachineState == MachineState_Aborted);
/// @todo live migration             mData->pMachineConfigFile->fTeleported = (mData->mMachineState == MachineState_Teleported);
        }

        mData->pMachineConfigFile->write(mData->m_strConfigFileFull);
    }
    catch (...)
    {
        rc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
    }

    return rc;
}

/**
 * Ensures that the given medium is added to a media registry. If this machine
 * was created with 4.0 or later, then the machine registry is used. Otherwise
 * the global VirtualBox media registry is used.
 *
 * Caller must NOT hold machine lock, media tree or any medium locks!
 *
 * @param pMedium
 */
void Machine::i_addMediumToRegistry(ComObjPtr<Medium> &pMedium)
{
    /* Paranoia checks: do not hold machine or media tree locks. */
    AssertReturnVoid(!isWriteLockOnCurrentThread());
    AssertReturnVoid(!mParent->i_getMediaTreeLockHandle().isWriteLockOnCurrentThread());

    ComObjPtr<Medium> pBase;
    {
        AutoReadLock treeLock(&mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);
        pBase = pMedium->i_getBase();
    }

    /* Paranoia checks: do not hold medium locks. */
    AssertReturnVoid(!pMedium->isWriteLockOnCurrentThread());
    AssertReturnVoid(!pBase->isWriteLockOnCurrentThread());

    // decide which medium registry to use now that the medium is attached:
    Guid uuid;
    if (mData->pMachineConfigFile->canHaveOwnMediaRegistry())
        // machine XML is VirtualBox 4.0 or higher:
        uuid = i_getId();     // machine UUID
    else
        uuid = mParent->i_getGlobalRegistryId(); // VirtualBox global registry UUID

    if (pMedium->i_addRegistry(uuid))
        mParent->i_markRegistryModified(uuid);

    /* For more complex hard disk structures it can happen that the base
     * medium isn't yet associated with any medium registry. Do that now. */
    if (pMedium != pBase)
    {
        /* Tree lock needed by Medium::addRegistry when recursing. */
        AutoReadLock treeLock(&mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);
        if (pBase->i_addRegistryRecursive(uuid))
        {
            treeLock.release();
            mParent->i_markRegistryModified(uuid);
        }
    }
}

/**
 * Creates differencing hard disks for all normal hard disks attached to this
 * machine and a new set of attachments to refer to created disks.
 *
 * Used when taking a snapshot or when deleting the current state. Gets called
 * from SessionMachine::BeginTakingSnapshot() and SessionMachine::restoreSnapshotHandler().
 *
 * This method assumes that mMediumAttachments contains the original hard disk
 * attachments it needs to create diffs for. On success, these attachments will
 * be replaced with the created diffs.
 *
 * Attachments with non-normal hard disks are left as is.
 *
 * If @a aOnline is @c false then the original hard disks that require implicit
 * diffs will be locked for reading. Otherwise it is assumed that they are
 * already locked for writing (when the VM was started). Note that in the latter
 * case it is responsibility of the caller to lock the newly created diffs for
 * writing if this method succeeds.
 *
 * @param aProgress         Progress object to run (must contain at least as
 *                          many operations left as the number of hard disks
 *                          attached).
 * @param aWeight           Weight of this operation.
 * @param aOnline           Whether the VM was online prior to this operation.
 *
 * @note The progress object is not marked as completed, neither on success nor
 *       on failure. This is a responsibility of the caller.
 *
 * @note Locks this object and the media tree for writing.
 */
HRESULT Machine::i_createImplicitDiffs(IProgress *aProgress,
                                       ULONG aWeight,
                                       bool aOnline)
{
    LogFlowThisFunc(("aOnline=%d\n", aOnline));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    AutoMultiWriteLock2 alock(this->lockHandle(),
                              &mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    /* must be in a protective state because we release the lock below */
    AssertReturn(   mData->mMachineState == MachineState_Snapshotting
                 || mData->mMachineState == MachineState_OnlineSnapshotting
                 || mData->mMachineState == MachineState_LiveSnapshotting
                 || mData->mMachineState == MachineState_RestoringSnapshot
                 || mData->mMachineState == MachineState_DeletingSnapshot
                 , E_FAIL);

    HRESULT rc = S_OK;

    // use appropriate locked media map (online or offline)
    MediumLockListMap lockedMediaOffline;
    MediumLockListMap *lockedMediaMap;
    if (aOnline)
        lockedMediaMap = &mData->mSession.mLockedMedia;
    else
        lockedMediaMap = &lockedMediaOffline;

    try
    {
        if (!aOnline)
        {
            /* lock all attached hard disks early to detect "in use"
             * situations before creating actual diffs */
            for (MediumAttachmentList::const_iterator
                 it = mMediumAttachments->begin();
                 it != mMediumAttachments->end();
                 ++it)
            {
                MediumAttachment *pAtt = *it;
                if (pAtt->i_getType() == DeviceType_HardDisk)
                {
                    Medium *pMedium = pAtt->i_getMedium();
                    Assert(pMedium);

                    MediumLockList *pMediumLockList(new MediumLockList());
                    alock.release();
                    rc = pMedium->i_createMediumLockList(true /* fFailIfInaccessible */,
                                                         NULL /* pToLockWrite */,
                                                         false /* fMediumLockWriteAll */,
                                                         NULL,
                                                         *pMediumLockList);
                    alock.acquire();
                    if (FAILED(rc))
                    {
                        delete pMediumLockList;
                        throw rc;
                    }
                    rc = lockedMediaMap->Insert(pAtt, pMediumLockList);
                    if (FAILED(rc))
                    {
                        throw setError(rc,
                                       tr("Collecting locking information for all attached media failed"));
                    }
                }
            }

            /* Now lock all media. If this fails, nothing is locked. */
            alock.release();
            rc = lockedMediaMap->Lock();
            alock.acquire();
            if (FAILED(rc))
            {
                throw setError(rc,
                               tr("Locking of attached media failed"));
            }
        }

        /* remember the current list (note that we don't use backup() since
         * mMediumAttachments may be already backed up) */
        MediumAttachmentList atts = *mMediumAttachments.data();

        /* start from scratch */
        mMediumAttachments->clear();

        /* go through remembered attachments and create diffs for normal hard
         * disks and attach them */
        for (MediumAttachmentList::const_iterator
             it = atts.begin();
             it != atts.end();
             ++it)
        {
            MediumAttachment *pAtt = *it;

            DeviceType_T devType = pAtt->i_getType();
            Medium *pMedium = pAtt->i_getMedium();

            if (   devType != DeviceType_HardDisk
                || pMedium == NULL
                || pMedium->i_getType() != MediumType_Normal)
            {
                /* copy the attachment as is */

                /** @todo the progress object created in SessionMachine::TakeSnaphot
                 * only expects operations for hard disks. Later other
                 * device types need to show up in the progress as well. */
                if (devType == DeviceType_HardDisk)
                {
                    if (pMedium == NULL)
                        aProgress->SetNextOperation(Bstr(tr("Skipping attachment without medium")).raw(),
                                                    aWeight);        // weight
                    else
                        aProgress->SetNextOperation(BstrFmt(tr("Skipping medium '%s'"),
                                                            pMedium->i_getBase()->i_getName().c_str()).raw(),
                                                    aWeight);        // weight
                }

                mMediumAttachments->push_back(pAtt);
                continue;
            }

            /* need a diff */
            aProgress->SetNextOperation(BstrFmt(tr("Creating differencing hard disk for '%s'"),
                                                pMedium->i_getBase()->i_getName().c_str()).raw(),
                                        aWeight);        // weight

            Utf8Str strFullSnapshotFolder;
            i_calculateFullPath(mUserData->s.strSnapshotFolder, strFullSnapshotFolder);

            ComObjPtr<Medium> diff;
            diff.createObject();
            // store the diff in the same registry as the parent
            // (this cannot fail here because we can't create implicit diffs for
            // unregistered images)
            Guid uuidRegistryParent;
            bool fInRegistry = pMedium->i_getFirstRegistryMachineId(uuidRegistryParent);
            Assert(fInRegistry); NOREF(fInRegistry);
            rc = diff->init(mParent,
                            pMedium->i_getPreferredDiffFormat(),
                            strFullSnapshotFolder.append(RTPATH_SLASH_STR),
                            uuidRegistryParent,
                            DeviceType_HardDisk);
            if (FAILED(rc)) throw rc;

            /** @todo r=bird: How is the locking and diff image cleaned up if we fail before
             *        the push_back?  Looks like we're going to release medium with the
             *        wrong kind of lock (general issue with if we fail anywhere at all)
             *        and an orphaned VDI in the snapshots folder. */

            /* update the appropriate lock list */
            MediumLockList *pMediumLockList;
            rc = lockedMediaMap->Get(pAtt, pMediumLockList);
            AssertComRCThrowRC(rc);
            if (aOnline)
            {
                alock.release();
                /* The currently attached medium will be read-only, change
                 * the lock type to read. */
                rc = pMediumLockList->Update(pMedium, false);
                alock.acquire();
                AssertComRCThrowRC(rc);
            }

            /* release the locks before the potentially lengthy operation */
            alock.release();
            rc = pMedium->i_createDiffStorage(diff,
                                              pMedium->i_getPreferredDiffVariant(),
                                              pMediumLockList,
                                              NULL /* aProgress */,
                                              true /* aWait */);
            alock.acquire();
            if (FAILED(rc)) throw rc;

            /* actual lock list update is done in Machine::i_commitMedia */

            rc = diff->i_addBackReference(mData->mUuid);
            AssertComRCThrowRC(rc);

            /* add a new attachment */
            ComObjPtr<MediumAttachment> attachment;
            attachment.createObject();
            rc = attachment->init(this,
                                  diff,
                                  pAtt->i_getControllerName(),
                                  pAtt->i_getPort(),
                                  pAtt->i_getDevice(),
                                  DeviceType_HardDisk,
                                  true /* aImplicit */,
                                  false /* aPassthrough */,
                                  false /* aTempEject */,
                                  pAtt->i_getNonRotational(),
                                  pAtt->i_getDiscard(),
                                  pAtt->i_getHotPluggable(),
                                  pAtt->i_getBandwidthGroup());
            if (FAILED(rc)) throw rc;

            rc = lockedMediaMap->ReplaceKey(pAtt, attachment);
            AssertComRCThrowRC(rc);
            mMediumAttachments->push_back(attachment);
        }
    }
    catch (HRESULT aRC) { rc = aRC; }

    /* unlock all hard disks we locked when there is no VM */
    if (!aOnline)
    {
        ErrorInfoKeeper eik;

        HRESULT rc1 = lockedMediaMap->Clear();
        AssertComRC(rc1);
    }

    return rc;
}

/**
 * Deletes implicit differencing hard disks created either by
 * #i_createImplicitDiffs() or by #attachDevice() and rolls back
 * mMediumAttachments.
 *
 * Note that to delete hard disks created by #attachDevice() this method is
 * called from #i_rollbackMedia() when the changes are rolled back.
 *
 * @note Locks this object and the media tree for writing.
 */
HRESULT Machine::i_deleteImplicitDiffs(bool aOnline)
{
    LogFlowThisFunc(("aOnline=%d\n", aOnline));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    AutoMultiWriteLock2 alock(this->lockHandle(),
                              &mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    /* We absolutely must have backed up state. */
    AssertReturn(mMediumAttachments.isBackedUp(), E_FAIL);

    /* Check if there are any implicitly created diff images. */
    bool fImplicitDiffs = false;
    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it)
    {
        const ComObjPtr<MediumAttachment> &pAtt = *it;
        if (pAtt->i_isImplicit())
        {
            fImplicitDiffs = true;
            break;
        }
    }
    /* If there is nothing to do, leave early. This saves lots of image locking
     * effort. It also avoids a MachineStateChanged event without real reason.
     * This is important e.g. when loading a VM config, because there should be
     * no events. Otherwise API clients can become thoroughly confused for
     * inaccessible VMs (the code for loading VM configs uses this method for
     * cleanup if the config makes no sense), as they take such events as an
     * indication that the VM is alive, and they would force the VM config to
     * be reread, leading to an endless loop. */
    if (!fImplicitDiffs)
        return S_OK;

    HRESULT rc = S_OK;
    MachineState_T oldState = mData->mMachineState;

    /* will release the lock before the potentially lengthy operation,
     * so protect with the special state (unless already protected) */
    if (   oldState != MachineState_Snapshotting
        && oldState != MachineState_OnlineSnapshotting
        && oldState != MachineState_LiveSnapshotting
        && oldState != MachineState_RestoringSnapshot
        && oldState != MachineState_DeletingSnapshot
        && oldState != MachineState_DeletingSnapshotOnline
        && oldState != MachineState_DeletingSnapshotPaused
       )
           i_setMachineState(MachineState_SettingUp);

    // use appropriate locked media map (online or offline)
    MediumLockListMap lockedMediaOffline;
    MediumLockListMap *lockedMediaMap;
    if (aOnline)
        lockedMediaMap = &mData->mSession.mLockedMedia;
    else
        lockedMediaMap = &lockedMediaOffline;

    try
    {
        if (!aOnline)
        {
            /* lock all attached hard disks early to detect "in use"
             * situations before deleting actual diffs */
            for (MediumAttachmentList::const_iterator
                 it = mMediumAttachments->begin();
                 it != mMediumAttachments->end();
                 ++it)
            {
                MediumAttachment *pAtt = *it;
                if (pAtt->i_getType() == DeviceType_HardDisk)
                {
                    Medium *pMedium = pAtt->i_getMedium();
                    Assert(pMedium);

                    MediumLockList *pMediumLockList(new MediumLockList());
                    alock.release();
                    rc = pMedium->i_createMediumLockList(true /* fFailIfInaccessible */,
                                                         NULL /* pToLockWrite */,
                                                         false /* fMediumLockWriteAll */,
                                                         NULL,
                                                         *pMediumLockList);
                    alock.acquire();

                    if (FAILED(rc))
                    {
                        delete pMediumLockList;
                        throw rc;
                    }

                    rc = lockedMediaMap->Insert(pAtt, pMediumLockList);
                    if (FAILED(rc))
                        throw rc;
                }
            }

            if (FAILED(rc))
                throw rc;
        } // end of offline

        /* Lock lists are now up to date and include implicitly created media */

        /* Go through remembered attachments and delete all implicitly created
         * diffs and fix up the attachment information */
        const MediumAttachmentList &oldAtts = *mMediumAttachments.backedUpData();
        MediumAttachmentList implicitAtts;
        for (MediumAttachmentList::const_iterator
             it = mMediumAttachments->begin();
             it != mMediumAttachments->end();
             ++it)
        {
            ComObjPtr<MediumAttachment> pAtt = *it;
            ComObjPtr<Medium> pMedium = pAtt->i_getMedium();
            if (pMedium.isNull())
                continue;

            // Implicit attachments go on the list for deletion and back references are removed.
            if (pAtt->i_isImplicit())
            {
                /* Deassociate and mark for deletion */
                LogFlowThisFunc(("Detaching '%s', pending deletion\n", pAtt->i_getLogName()));
                rc = pMedium->i_removeBackReference(mData->mUuid);
                if (FAILED(rc))
                   throw rc;
                implicitAtts.push_back(pAtt);
                continue;
            }

            /* Was this medium attached before? */
            if (!i_findAttachment(oldAtts, pMedium))
            {
                /* no: de-associate */
                LogFlowThisFunc(("Detaching '%s', no deletion\n", pAtt->i_getLogName()));
                rc = pMedium->i_removeBackReference(mData->mUuid);
                if (FAILED(rc))
                    throw rc;
                continue;
            }
            LogFlowThisFunc(("Not detaching '%s'\n", pAtt->i_getLogName()));
        }

        /* If there are implicit attachments to delete, throw away the lock
         * map contents (which will unlock all media) since the medium
         * attachments will be rolled back. Below we need to completely
         * recreate the lock map anyway since it is infinitely complex to
         * do this incrementally (would need reconstructing each attachment
         * change, which would be extremely hairy). */
        if (implicitAtts.size() != 0)
        {
            ErrorInfoKeeper eik;

            HRESULT rc1 = lockedMediaMap->Clear();
            AssertComRC(rc1);
        }

        /* rollback hard disk changes */
        mMediumAttachments.rollback();

        MultiResult mrc(S_OK);

        // Delete unused implicit diffs.
        if (implicitAtts.size() != 0)
        {
            alock.release();

            for (MediumAttachmentList::const_iterator
                 it = implicitAtts.begin();
                 it != implicitAtts.end();
                 ++it)
            {
                // Remove medium associated with this attachment.
                ComObjPtr<MediumAttachment> pAtt = *it;
                Assert(pAtt);
                LogFlowThisFunc(("Deleting '%s'\n", pAtt->i_getLogName()));
                ComObjPtr<Medium> pMedium = pAtt->i_getMedium();
                Assert(pMedium);

                rc = pMedium->i_deleteStorage(NULL /*aProgress*/, true /*aWait*/);
                // continue on delete failure, just collect error messages
                AssertMsg(SUCCEEDED(rc), ("rc=%Rhrc it=%s hd=%s\n", rc, pAtt->i_getLogName(),
                                          pMedium->i_getLocationFull().c_str() ));
                mrc = rc;
            }
            // Clear the list of deleted implicit attachments now, while not
            // holding the lock, as it will ultimately trigger Medium::uninit()
            // calls which assume that the media tree lock isn't held.
            implicitAtts.clear();

            alock.acquire();

            /* if there is a VM recreate media lock map as mentioned above,
             * otherwise it is a waste of time and we leave things unlocked */
            if (aOnline)
            {
                const ComObjPtr<SessionMachine> pMachine = mData->mSession.mMachine;
                /* must never be NULL, but better safe than sorry */
                if (!pMachine.isNull())
                {
                    alock.release();
                    rc = mData->mSession.mMachine->i_lockMedia();
                    alock.acquire();
                    if (FAILED(rc))
                        throw rc;
                }
            }
        }
    }
    catch (HRESULT aRC) {rc = aRC;}

    if (mData->mMachineState == MachineState_SettingUp)
        i_setMachineState(oldState);

    /* unlock all hard disks we locked when there is no VM */
    if (!aOnline)
    {
        ErrorInfoKeeper eik;

        HRESULT rc1 = lockedMediaMap->Clear();
        AssertComRC(rc1);
    }

    return rc;
}


/**
 * Looks through the given list of media attachments for one with the given parameters
 * and returns it, or NULL if not found. The list is a parameter so that backup lists
 * can be searched as well if needed.
 *
 * @param ll
 * @param aControllerName
 * @param aControllerPort
 * @param aDevice
 * @return
 */
MediumAttachment *Machine::i_findAttachment(const MediumAttachmentList &ll,
                                            const Utf8Str &aControllerName,
                                            LONG aControllerPort,
                                            LONG aDevice)
{
    for (MediumAttachmentList::const_iterator
         it = ll.begin();
         it != ll.end();
         ++it)
    {
        MediumAttachment *pAttach = *it;
        if (pAttach->i_matches(aControllerName, aControllerPort, aDevice))
            return pAttach;
    }

    return NULL;
}

/**
 * Looks through the given list of media attachments for one with the given parameters
 * and returns it, or NULL if not found. The list is a parameter so that backup lists
 * can be searched as well if needed.
 *
 * @param ll
 * @param pMedium
 * @return
 */
MediumAttachment *Machine::i_findAttachment(const MediumAttachmentList &ll,
                                            ComObjPtr<Medium> pMedium)
{
    for (MediumAttachmentList::const_iterator
         it = ll.begin();
         it != ll.end();
         ++it)
    {
        MediumAttachment *pAttach = *it;
        ComObjPtr<Medium> pMediumThis = pAttach->i_getMedium();
        if (pMediumThis == pMedium)
            return pAttach;
    }

    return NULL;
}

/**
 * Looks through the given list of media attachments for one with the given parameters
 * and returns it, or NULL if not found. The list is a parameter so that backup lists
 * can be searched as well if needed.
 *
 * @param ll
 * @param id
 * @return
 */
MediumAttachment *Machine::i_findAttachment(const MediumAttachmentList &ll,
                                            Guid &id)
{
    for (MediumAttachmentList::const_iterator
         it = ll.begin();
         it != ll.end();
         ++it)
    {
        MediumAttachment *pAttach = *it;
        ComObjPtr<Medium> pMediumThis = pAttach->i_getMedium();
        if (pMediumThis->i_getId() == id)
            return pAttach;
    }

    return NULL;
}

/**
 * Main implementation for Machine::DetachDevice. This also gets called
 * from Machine::prepareUnregister() so it has been taken out for simplicity.
 *
 * @param pAttach   Medium attachment to detach.
 * @param writeLock Machine write lock which the caller must have locked once.
 *                  This may be released temporarily in here.
 * @param pSnapshot If NULL, then the detachment is for the current machine.
 *                  Otherwise this is for a SnapshotMachine, and this must be
 *                  its snapshot.
 * @return
 */
HRESULT Machine::i_detachDevice(MediumAttachment *pAttach,
                                AutoWriteLock &writeLock,
                                Snapshot *pSnapshot)
{
    ComObjPtr<Medium> oldmedium = pAttach->i_getMedium();
    DeviceType_T mediumType = pAttach->i_getType();

    LogFlowThisFunc(("Entering, medium of attachment is %s\n", oldmedium ? oldmedium->i_getLocationFull().c_str() : "NULL"));

    if (pAttach->i_isImplicit())
    {
        /* attempt to implicitly delete the implicitly created diff */

        /// @todo move the implicit flag from MediumAttachment to Medium
        /// and forbid any hard disk operation when it is implicit. Or maybe
        /// a special media state for it to make it even more simple.

        Assert(mMediumAttachments.isBackedUp());

        /* will release the lock before the potentially lengthy operation, so
         * protect with the special state */
        MachineState_T oldState = mData->mMachineState;
        i_setMachineState(MachineState_SettingUp);

        writeLock.release();

        HRESULT rc = oldmedium->i_deleteStorage(NULL /*aProgress*/,
                                                true /*aWait*/);

        writeLock.acquire();

        i_setMachineState(oldState);

        if (FAILED(rc)) return rc;
    }

    i_setModified(IsModified_Storage);
    mMediumAttachments.backup();
    mMediumAttachments->remove(pAttach);

    if (!oldmedium.isNull())
    {
        // if this is from a snapshot, do not defer detachment to i_commitMedia()
        if (pSnapshot)
            oldmedium->i_removeBackReference(mData->mUuid, pSnapshot->i_getId());
        // else if non-hard disk media, do not defer detachment to i_commitMedia() either
        else if (mediumType != DeviceType_HardDisk)
            oldmedium->i_removeBackReference(mData->mUuid);
    }

    return S_OK;
}

/**
 * Goes thru all media of the given list and
 *
 * 1) calls i_detachDevice() on each of them for this machine and
 * 2) adds all Medium objects found in the process to the given list,
 *    depending on cleanupMode.
 *
 * If cleanupMode is CleanupMode_DetachAllReturnHardDisksOnly, this only
 * adds hard disks to the list. If it is CleanupMode_Full, this adds all
 * media to the list.
 *
 * This gets called from Machine::Unregister, both for the actual Machine and
 * the SnapshotMachine objects that might be found in the snapshots.
 *
 * Requires caller and locking. The machine lock must be passed in because it
 * will be passed on to i_detachDevice which needs it for temporary unlocking.
 *
 * @param writeLock Machine lock from top-level caller; this gets passed to
 *                  i_detachDevice.
 * @param pSnapshot Must be NULL when called for a "real" Machine or a snapshot
 *                  object if called for a SnapshotMachine.
 * @param cleanupMode If DetachAllReturnHardDisksOnly, only hard disk media get
 *                  added to llMedia; if Full, then all media get added;
 *                  otherwise no media get added.
 * @param llMedia   Caller's list to receive Medium objects which got detached so
 *                  caller can close() them, depending on cleanupMode.
 * @return
 */
HRESULT Machine::i_detachAllMedia(AutoWriteLock &writeLock,
                                  Snapshot *pSnapshot,
                                  CleanupMode_T cleanupMode,
                                  MediaList &llMedia)
{
    Assert(isWriteLockOnCurrentThread());

    HRESULT rc;

    // make a temporary list because i_detachDevice invalidates iterators into
    // mMediumAttachments
    MediumAttachmentList llAttachments2 = *mMediumAttachments.data();

    for (MediumAttachmentList::iterator
         it = llAttachments2.begin();
         it != llAttachments2.end();
         ++it)
    {
        ComObjPtr<MediumAttachment> &pAttach = *it;
        ComObjPtr<Medium> pMedium = pAttach->i_getMedium();

        if (!pMedium.isNull())
        {
            AutoCaller mac(pMedium);
            if (FAILED(mac.rc())) return mac.rc();
            AutoReadLock lock(pMedium COMMA_LOCKVAL_SRC_POS);
            DeviceType_T devType = pMedium->i_getDeviceType();
            if (    (    cleanupMode == CleanupMode_DetachAllReturnHardDisksOnly
                      && devType == DeviceType_HardDisk)
                 || (cleanupMode == CleanupMode_Full)
               )
            {
                llMedia.push_back(pMedium);
                ComObjPtr<Medium> pParent = pMedium->i_getParent();
                /* Not allowed to keep this lock as below we need the parent
                 * medium lock, and the lock order is parent to child. */
                lock.release();
                /*
                 * Search for medias which are not attached to any machine, but
                 * in the chain to an attached disk. Mediums are only consided
                 * if they are:
                 * - have only one child
                 * - no references to any machines
                 * - are of normal medium type
                 */
                while (!pParent.isNull())
                {
                    AutoCaller mac1(pParent);
                    if (FAILED(mac1.rc())) return mac1.rc();
                    AutoReadLock lock1(pParent COMMA_LOCKVAL_SRC_POS);
                    if (pParent->i_getChildren().size() == 1)
                    {
                        if (   pParent->i_getMachineBackRefCount() == 0
                            && pParent->i_getType() == MediumType_Normal
                            && find(llMedia.begin(), llMedia.end(), pParent) == llMedia.end())
                            llMedia.push_back(pParent);
                    }
                    else
                        break;
                    pParent = pParent->i_getParent();
                }
            }
        }

        // real machine: then we need to use the proper method
        rc = i_detachDevice(pAttach, writeLock, pSnapshot);

        if (FAILED(rc))
            return rc;
    }

    return S_OK;
}

/**
 * Perform deferred hard disk detachments.
 *
 * Does nothing if the hard disk attachment data (mMediumAttachments) is not
 * changed (not backed up).
 *
 * If @a aOnline is @c true then this method will also unlock the old hard
 * disks for which the new implicit diffs were created and will lock these new
 * diffs for writing.
 *
 * @param aOnline       Whether the VM was online prior to this operation.
 *
 * @note Locks this object for writing!
 */
void Machine::i_commitMedia(bool aOnline /*= false*/)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("Entering, aOnline=%d\n", aOnline));

    HRESULT rc = S_OK;

    /* no attach/detach operations -- nothing to do */
    if (!mMediumAttachments.isBackedUp())
        return;

    MediumAttachmentList &oldAtts = *mMediumAttachments.backedUpData();
    bool fMediaNeedsLocking = false;

    /* enumerate new attachments */
    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it)
    {
        MediumAttachment *pAttach = *it;

        pAttach->i_commit();

        Medium *pMedium = pAttach->i_getMedium();
        bool fImplicit = pAttach->i_isImplicit();

        LogFlowThisFunc(("Examining current medium '%s' (implicit: %d)\n",
                         (pMedium) ? pMedium->i_getName().c_str() : "NULL",
                         fImplicit));

        /** @todo convert all this Machine-based voodoo to MediumAttachment
         * based commit logic. */
        if (fImplicit)
        {
            /* convert implicit attachment to normal */
            pAttach->i_setImplicit(false);

            if (    aOnline
                 && pMedium
                 && pAttach->i_getType() == DeviceType_HardDisk
               )
            {
                /* update the appropriate lock list */
                MediumLockList *pMediumLockList;
                rc = mData->mSession.mLockedMedia.Get(pAttach, pMediumLockList);
                AssertComRC(rc);
                if (pMediumLockList)
                {
                    /* unlock if there's a need to change the locking */
                    if (!fMediaNeedsLocking)
                    {
                        rc = mData->mSession.mLockedMedia.Unlock();
                        AssertComRC(rc);
                        fMediaNeedsLocking = true;
                    }
                    rc = pMediumLockList->Update(pMedium->i_getParent(), false);
                    AssertComRC(rc);
                    rc = pMediumLockList->Append(pMedium, true);
                    AssertComRC(rc);
                }
            }

            continue;
        }

        if (pMedium)
        {
            /* was this medium attached before? */
            for (MediumAttachmentList::iterator
                 oldIt = oldAtts.begin();
                 oldIt != oldAtts.end();
                 ++oldIt)
            {
                MediumAttachment *pOldAttach = *oldIt;
                if (pOldAttach->i_getMedium() == pMedium)
                {
                    LogFlowThisFunc(("--> medium '%s' was attached before, will not remove\n", pMedium->i_getName().c_str()));

                    /* yes: remove from old to avoid de-association */
                    oldAtts.erase(oldIt);
                    break;
                }
            }
        }
    }

    /* enumerate remaining old attachments and de-associate from the
     * current machine state */
    for (MediumAttachmentList::const_iterator
         it = oldAtts.begin();
         it != oldAtts.end();
         ++it)
    {
        MediumAttachment *pAttach = *it;
        Medium *pMedium = pAttach->i_getMedium();

        /* Detach only hard disks, since DVD/floppy media is detached
         * instantly in MountMedium. */
        if (pAttach->i_getType() == DeviceType_HardDisk && pMedium)
        {
            LogFlowThisFunc(("detaching medium '%s' from machine\n", pMedium->i_getName().c_str()));

            /* now de-associate from the current machine state */
            rc = pMedium->i_removeBackReference(mData->mUuid);
            AssertComRC(rc);

            if (aOnline)
            {
                /* unlock since medium is not used anymore */
                MediumLockList *pMediumLockList;
                rc = mData->mSession.mLockedMedia.Get(pAttach, pMediumLockList);
                if (RT_UNLIKELY(rc == VBOX_E_INVALID_OBJECT_STATE))
                {
                    /* this happens for online snapshots, there the attachment
                     * is changing, but only to a diff image created under
                     * the old one, so there is no separate lock list */
                    Assert(!pMediumLockList);
                }
                else
                {
                    AssertComRC(rc);
                    if (pMediumLockList)
                    {
                        rc = mData->mSession.mLockedMedia.Remove(pAttach);
                        AssertComRC(rc);
                    }
                }
            }
        }
    }

    /* take media locks again so that the locking state is consistent */
    if (fMediaNeedsLocking)
    {
        Assert(aOnline);
        rc = mData->mSession.mLockedMedia.Lock();
        AssertComRC(rc);
    }

    /* commit the hard disk changes */
    mMediumAttachments.commit();

    if (i_isSessionMachine())
    {
        /*
         * Update the parent machine to point to the new owner.
         * This is necessary because the stored parent will point to the
         * session machine otherwise and cause crashes or errors later
         * when the session machine gets invalid.
         */
        /** @todo Change the MediumAttachment class to behave like any other
         *        class in this regard by creating peer MediumAttachment
         *        objects for session machines and share the data with the peer
         *        machine.
         */
        for (MediumAttachmentList::const_iterator
             it = mMediumAttachments->begin();
             it != mMediumAttachments->end();
             ++it)
            (*it)->i_updateParentMachine(mPeer);

        /* attach new data to the primary machine and reshare it */
        mPeer->mMediumAttachments.attach(mMediumAttachments);
    }

    return;
}

/**
 * Perform deferred deletion of implicitly created diffs.
 *
 * Does nothing if the hard disk attachment data (mMediumAttachments) is not
 * changed (not backed up).
 *
 * @note Locks this object for writing!
 */
void Machine::i_rollbackMedia()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    // AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    LogFlowThisFunc(("Entering rollbackMedia\n"));

    HRESULT rc = S_OK;

    /* no attach/detach operations -- nothing to do */
    if (!mMediumAttachments.isBackedUp())
        return;

    /* enumerate new attachments */
    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it)
    {
        MediumAttachment *pAttach = *it;
        /* Fix up the backrefs for DVD/floppy media. */
        if (pAttach->i_getType() != DeviceType_HardDisk)
        {
            Medium *pMedium = pAttach->i_getMedium();
            if (pMedium)
            {
                rc = pMedium->i_removeBackReference(mData->mUuid);
                AssertComRC(rc);
            }
        }

        (*it)->i_rollback();

        pAttach = *it;
        /* Fix up the backrefs for DVD/floppy media. */
        if (pAttach->i_getType() != DeviceType_HardDisk)
        {
            Medium *pMedium = pAttach->i_getMedium();
            if (pMedium)
            {
                rc = pMedium->i_addBackReference(mData->mUuid);
                AssertComRC(rc);
            }
        }
    }

    /** @todo convert all this Machine-based voodoo to MediumAttachment
     * based rollback logic. */
    i_deleteImplicitDiffs(Global::IsOnline(mData->mMachineState));

    return;
}

/**
 *  Returns true if the settings file is located in the directory named exactly
 *  as the machine; this means, among other things, that the machine directory
 *  should be auto-renamed.
 *
 *  @param aSettingsDir if not NULL, the full machine settings file directory
 *                      name will be assigned there.
 *
 *  @note Doesn't lock anything.
 *  @note Not thread safe (must be called from this object's lock).
 */
bool Machine::i_isInOwnDir(Utf8Str *aSettingsDir /* = NULL */) const
{
    Utf8Str strMachineDirName(mData->m_strConfigFileFull);  // path/to/machinesfolder/vmname/vmname.vbox
    strMachineDirName.stripFilename();                      // path/to/machinesfolder/vmname
    if (aSettingsDir)
        *aSettingsDir = strMachineDirName;
    strMachineDirName.stripPath();                          // vmname
    Utf8Str strConfigFileOnly(mData->m_strConfigFileFull);  // path/to/machinesfolder/vmname/vmname.vbox
    strConfigFileOnly.stripPath()                           // vmname.vbox
                     .stripSuffix();                        // vmname
    /** @todo hack, make somehow use of ComposeMachineFilename */
    if (mUserData->s.fDirectoryIncludesUUID)
        strConfigFileOnly += Utf8StrFmt(" (%RTuuid)", mData->mUuid.raw());

    AssertReturn(!strMachineDirName.isEmpty(), false);
    AssertReturn(!strConfigFileOnly.isEmpty(), false);

    return strMachineDirName == strConfigFileOnly;
}

/**
 * Discards all changes to machine settings.
 *
 * @param aNotify   Whether to notify the direct session about changes or not.
 *
 * @note Locks objects for writing!
 */
void Machine::i_rollback(bool aNotify)
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), (void)0);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mStorageControllers.isNull())
    {
        if (mStorageControllers.isBackedUp())
        {
            /* unitialize all new devices (absent in the backed up list). */
            StorageControllerList *backedList = mStorageControllers.backedUpData();
            for (StorageControllerList::const_iterator
                 it = mStorageControllers->begin();
                 it != mStorageControllers->end();
                 ++it)
            {
                if (   std::find(backedList->begin(), backedList->end(), *it)
                    == backedList->end()
                   )
                {
                    (*it)->uninit();
                }
            }

            /* restore the list */
            mStorageControllers.rollback();
        }

        /* rollback any changes to devices after restoring the list */
        if (mData->flModifications & IsModified_Storage)
        {
            for (StorageControllerList::const_iterator
                 it = mStorageControllers->begin();
                 it != mStorageControllers->end();
                 ++it)
            {
                (*it)->i_rollback();
            }
        }
    }

    if (!mUSBControllers.isNull())
    {
        if (mUSBControllers.isBackedUp())
        {
            /* unitialize all new devices (absent in the backed up list). */
            USBControllerList *backedList = mUSBControllers.backedUpData();
            for (USBControllerList::const_iterator
                 it = mUSBControllers->begin();
                 it != mUSBControllers->end();
                 ++it)
            {
                if (   std::find(backedList->begin(), backedList->end(), *it)
                    == backedList->end()
                   )
                {
                    (*it)->uninit();
                }
            }

            /* restore the list */
            mUSBControllers.rollback();
        }

        /* rollback any changes to devices after restoring the list */
        if (mData->flModifications & IsModified_USB)
        {
            for (USBControllerList::const_iterator
                 it = mUSBControllers->begin();
                 it != mUSBControllers->end();
                 ++it)
            {
                (*it)->i_rollback();
            }
        }
    }

    mUserData.rollback();

    mHWData.rollback();

    if (mData->flModifications & IsModified_Storage)
        i_rollbackMedia();

    if (mBIOSSettings)
        mBIOSSettings->i_rollback();

    if (mVRDEServer && (mData->flModifications & IsModified_VRDEServer))
        mVRDEServer->i_rollback();

    if (mAudioAdapter)
        mAudioAdapter->i_rollback();

    if (mUSBDeviceFilters && (mData->flModifications & IsModified_USB))
        mUSBDeviceFilters->i_rollback();

    if (mBandwidthControl && (mData->flModifications & IsModified_BandwidthControl))
        mBandwidthControl->i_rollback();

    if (!mHWData.isNull())
        mNetworkAdapters.resize(Global::getMaxNetworkAdapters(mHWData->mChipsetType));
    NetworkAdapterVector networkAdapters(mNetworkAdapters.size());
    ComPtr<ISerialPort> serialPorts[RT_ELEMENTS(mSerialPorts)];
    ComPtr<IParallelPort> parallelPorts[RT_ELEMENTS(mParallelPorts)];

    if (mData->flModifications & IsModified_NetworkAdapters)
        for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
            if (    mNetworkAdapters[slot]
                 && mNetworkAdapters[slot]->i_isModified())
            {
                mNetworkAdapters[slot]->i_rollback();
                networkAdapters[slot] = mNetworkAdapters[slot];
            }

    if (mData->flModifications & IsModified_SerialPorts)
        for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
            if (    mSerialPorts[slot]
                 && mSerialPorts[slot]->i_isModified())
            {
                mSerialPorts[slot]->i_rollback();
                serialPorts[slot] = mSerialPorts[slot];
            }

    if (mData->flModifications & IsModified_ParallelPorts)
        for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
            if (    mParallelPorts[slot]
                 && mParallelPorts[slot]->i_isModified())
            {
                mParallelPorts[slot]->i_rollback();
                parallelPorts[slot] = mParallelPorts[slot];
            }

    if (aNotify)
    {
        /* inform the direct session about changes */

        ComObjPtr<Machine> that = this;
        uint32_t flModifications = mData->flModifications;
        alock.release();

        if (flModifications & IsModified_SharedFolders)
            that->i_onSharedFolderChange();

        if (flModifications & IsModified_VRDEServer)
            that->i_onVRDEServerChange(/* aRestart */ TRUE);
        if (flModifications & IsModified_USB)
            that->i_onUSBControllerChange();

        for (ULONG slot = 0; slot < networkAdapters.size(); ++slot)
            if (networkAdapters[slot])
                that->i_onNetworkAdapterChange(networkAdapters[slot], FALSE);
        for (ULONG slot = 0; slot < RT_ELEMENTS(serialPorts); ++slot)
            if (serialPorts[slot])
                that->i_onSerialPortChange(serialPorts[slot]);
        for (ULONG slot = 0; slot < RT_ELEMENTS(parallelPorts); ++slot)
            if (parallelPorts[slot])
                that->i_onParallelPortChange(parallelPorts[slot]);

        if (flModifications & IsModified_Storage)
            that->i_onStorageControllerChange();

#if 0
        if (flModifications & IsModified_BandwidthControl)
            that->onBandwidthControlChange();
#endif
    }
}

/**
 * Commits all the changes to machine settings.
 *
 * Note that this operation is supposed to never fail.
 *
 * @note Locks this object and children for writing.
 */
void Machine::i_commit()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoCaller peerCaller(mPeer);
    AssertComRCReturnVoid(peerCaller.rc());

    AutoMultiWriteLock2 alock(mPeer, this COMMA_LOCKVAL_SRC_POS);

    /*
     *  use safe commit to ensure Snapshot machines (that share mUserData)
     *  will still refer to a valid memory location
     */
    mUserData.commitCopy();

    mHWData.commit();

    if (mMediumAttachments.isBackedUp())
        i_commitMedia(Global::IsOnline(mData->mMachineState));

    mBIOSSettings->i_commit();
    mVRDEServer->i_commit();
    mAudioAdapter->i_commit();
    mUSBDeviceFilters->i_commit();
    mBandwidthControl->i_commit();

    /* Since mNetworkAdapters is a list which might have been changed (resized)
     * without using the Backupable<> template we need to handle the copying
     * of the list entries manually, including the creation of peers for the
     * new objects. */
    bool commitNetworkAdapters = false;
    size_t newSize = Global::getMaxNetworkAdapters(mHWData->mChipsetType);
    if (mPeer)
    {
        /* commit everything, even the ones which will go away */
        for (size_t slot = 0; slot < mNetworkAdapters.size(); slot++)
            mNetworkAdapters[slot]->i_commit();
        /* copy over the new entries, creating a peer and uninit the original */
        mPeer->mNetworkAdapters.resize(RT_MAX(newSize, mPeer->mNetworkAdapters.size()));
        for (size_t slot = 0; slot < newSize; slot++)
        {
            /* look if this adapter has a peer device */
            ComObjPtr<NetworkAdapter> peer = mNetworkAdapters[slot]->i_getPeer();
            if (!peer)
            {
                /* no peer means the adapter is a newly created one;
                 * create a peer owning data this data share it with */
                peer.createObject();
                peer->init(mPeer, mNetworkAdapters[slot], true /* aReshare */);
            }
            mPeer->mNetworkAdapters[slot] = peer;
        }
        /* uninit any no longer needed network adapters */
        for (size_t slot = newSize; slot < mNetworkAdapters.size(); ++slot)
            mNetworkAdapters[slot]->uninit();
        for (size_t slot = newSize; slot < mPeer->mNetworkAdapters.size(); ++slot)
        {
            if (mPeer->mNetworkAdapters[slot])
                mPeer->mNetworkAdapters[slot]->uninit();
        }
        /* Keep the original network adapter count until this point, so that
         * discarding a chipset type change will not lose settings. */
        mNetworkAdapters.resize(newSize);
        mPeer->mNetworkAdapters.resize(newSize);
    }
    else
    {
        /* we have no peer (our parent is the newly created machine);
         * just commit changes to the network adapters */
        commitNetworkAdapters = true;
    }
    if (commitNetworkAdapters)
        for (size_t slot = 0; slot < mNetworkAdapters.size(); ++slot)
            mNetworkAdapters[slot]->i_commit();

    for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
        mSerialPorts[slot]->i_commit();
    for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
        mParallelPorts[slot]->i_commit();

    bool commitStorageControllers = false;

    if (mStorageControllers.isBackedUp())
    {
        mStorageControllers.commit();

        if (mPeer)
        {
            /* Commit all changes to new controllers (this will reshare data with
             * peers for those who have peers) */
            StorageControllerList *newList = new StorageControllerList();
            for (StorageControllerList::const_iterator
                 it = mStorageControllers->begin();
                 it != mStorageControllers->end();
                 ++it)
            {
                (*it)->i_commit();

                /* look if this controller has a peer device */
                ComObjPtr<StorageController> peer = (*it)->i_getPeer();
                if (!peer)
                {
                    /* no peer means the device is a newly created one;
                     * create a peer owning data this device share it with */
                    peer.createObject();
                    peer->init(mPeer, *it, true /* aReshare */);
                }
                else
                {
                    /* remove peer from the old list */
                    mPeer->mStorageControllers->remove(peer);
                }
                /* and add it to the new list */
                newList->push_back(peer);
            }

            /* uninit old peer's controllers that are left */
            for (StorageControllerList::const_iterator
                 it = mPeer->mStorageControllers->begin();
                 it != mPeer->mStorageControllers->end();
                 ++it)
            {
                (*it)->uninit();
            }

            /* attach new list of controllers to our peer */
            mPeer->mStorageControllers.attach(newList);
        }
        else
        {
            /* we have no peer (our parent is the newly created machine);
             * just commit changes to devices */
            commitStorageControllers = true;
        }
    }
    else
    {
        /* the list of controllers itself is not changed,
         * just commit changes to controllers themselves */
        commitStorageControllers = true;
    }

    if (commitStorageControllers)
    {
        for (StorageControllerList::const_iterator
             it = mStorageControllers->begin();
             it != mStorageControllers->end();
             ++it)
        {
            (*it)->i_commit();
        }
    }

    bool commitUSBControllers = false;

    if (mUSBControllers.isBackedUp())
    {
        mUSBControllers.commit();

        if (mPeer)
        {
            /* Commit all changes to new controllers (this will reshare data with
             * peers for those who have peers) */
            USBControllerList *newList = new USBControllerList();
            for (USBControllerList::const_iterator
                 it = mUSBControllers->begin();
                 it != mUSBControllers->end();
                 ++it)
            {
                (*it)->i_commit();

                /* look if this controller has a peer device */
                ComObjPtr<USBController> peer = (*it)->i_getPeer();
                if (!peer)
                {
                    /* no peer means the device is a newly created one;
                     * create a peer owning data this device share it with */
                    peer.createObject();
                    peer->init(mPeer, *it, true /* aReshare */);
                }
                else
                {
                    /* remove peer from the old list */
                    mPeer->mUSBControllers->remove(peer);
                }
                /* and add it to the new list */
                newList->push_back(peer);
            }

            /* uninit old peer's controllers that are left */
            for (USBControllerList::const_iterator
                 it = mPeer->mUSBControllers->begin();
                 it != mPeer->mUSBControllers->end();
                 ++it)
            {
                (*it)->uninit();
            }

            /* attach new list of controllers to our peer */
            mPeer->mUSBControllers.attach(newList);
        }
        else
        {
            /* we have no peer (our parent is the newly created machine);
             * just commit changes to devices */
            commitUSBControllers = true;
        }
    }
    else
    {
        /* the list of controllers itself is not changed,
         * just commit changes to controllers themselves */
        commitUSBControllers = true;
    }

    if (commitUSBControllers)
    {
        for (USBControllerList::const_iterator
             it = mUSBControllers->begin();
             it != mUSBControllers->end();
             ++it)
        {
            (*it)->i_commit();
        }
    }

    if (i_isSessionMachine())
    {
        /* attach new data to the primary machine and reshare it */
        mPeer->mUserData.attach(mUserData);
        mPeer->mHWData.attach(mHWData);
        /* mmMediumAttachments is reshared by fixupMedia */
        // mPeer->mMediumAttachments.attach(mMediumAttachments);
        Assert(mPeer->mMediumAttachments.data() == mMediumAttachments.data());
    }
}

/**
 * Copies all the hardware data from the given machine.
 *
 * Currently, only called when the VM is being restored from a snapshot. In
 * particular, this implies that the VM is not running during this method's
 * call.
 *
 * @note This method must be called from under this object's lock.
 *
 * @note This method doesn't call #i_commit(), so all data remains backed up and
 *       unsaved.
 */
void Machine::i_copyFrom(Machine *aThat)
{
    AssertReturnVoid(!i_isSnapshotMachine());
    AssertReturnVoid(aThat->i_isSnapshotMachine());

    AssertReturnVoid(!Global::IsOnline(mData->mMachineState));

    mHWData.assignCopy(aThat->mHWData);

    // create copies of all shared folders (mHWData after attaching a copy
    // contains just references to original objects)
    for (HWData::SharedFolderList::iterator
         it = mHWData->mSharedFolders.begin();
         it != mHWData->mSharedFolders.end();
         ++it)
    {
        ComObjPtr<SharedFolder> folder;
        folder.createObject();
        HRESULT rc = folder->initCopy(i_getMachine(), *it);
        AssertComRC(rc);
        *it = folder;
    }

    mBIOSSettings->i_copyFrom(aThat->mBIOSSettings);
    mVRDEServer->i_copyFrom(aThat->mVRDEServer);
    mAudioAdapter->i_copyFrom(aThat->mAudioAdapter);
    mUSBDeviceFilters->i_copyFrom(aThat->mUSBDeviceFilters);
    mBandwidthControl->i_copyFrom(aThat->mBandwidthControl);

    /* create private copies of all controllers */
    mStorageControllers.backup();
    mStorageControllers->clear();
    for (StorageControllerList::const_iterator
         it = aThat->mStorageControllers->begin();
         it != aThat->mStorageControllers->end();
         ++it)
    {
        ComObjPtr<StorageController> ctrl;
        ctrl.createObject();
        ctrl->initCopy(this, *it);
        mStorageControllers->push_back(ctrl);
    }

    /* create private copies of all USB controllers */
    mUSBControllers.backup();
    mUSBControllers->clear();
    for (USBControllerList::const_iterator
         it = aThat->mUSBControllers->begin();
         it != aThat->mUSBControllers->end();
         ++it)
    {
        ComObjPtr<USBController> ctrl;
        ctrl.createObject();
        ctrl->initCopy(this, *it);
        mUSBControllers->push_back(ctrl);
    }

    mNetworkAdapters.resize(aThat->mNetworkAdapters.size());
    for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
    {
        if (mNetworkAdapters[slot].isNotNull())
            mNetworkAdapters[slot]->i_copyFrom(aThat->mNetworkAdapters[slot]);
        else
        {
            unconst(mNetworkAdapters[slot]).createObject();
            mNetworkAdapters[slot]->initCopy(this, aThat->mNetworkAdapters[slot]);
        }
    }
    for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
        mSerialPorts[slot]->i_copyFrom(aThat->mSerialPorts[slot]);
    for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
        mParallelPorts[slot]->i_copyFrom(aThat->mParallelPorts[slot]);
}

/**
 * Returns whether the given storage controller is hotplug capable.
 *
 * @returns true if the controller supports hotplugging
 *          false otherwise.
 * @param   enmCtrlType    The controller type to check for.
 */
bool Machine::i_isControllerHotplugCapable(StorageControllerType_T enmCtrlType)
{
    ComPtr<ISystemProperties> systemProperties;
    HRESULT rc = mParent->COMGETTER(SystemProperties)(systemProperties.asOutParam());
    if (FAILED(rc))
        return false;

    BOOL aHotplugCapable = FALSE;
    systemProperties->GetStorageControllerHotplugCapable(enmCtrlType, &aHotplugCapable);

    return RT_BOOL(aHotplugCapable);
}

#ifdef VBOX_WITH_RESOURCE_USAGE_API

void Machine::i_getDiskList(MediaList &list)
{
    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it)
    {
        MediumAttachment *pAttach = *it;
        /* just in case */
        AssertContinue(pAttach);

        AutoCaller localAutoCallerA(pAttach);
        if (FAILED(localAutoCallerA.rc())) continue;

        AutoReadLock local_alockA(pAttach COMMA_LOCKVAL_SRC_POS);

        if (pAttach->i_getType() == DeviceType_HardDisk)
            list.push_back(pAttach->i_getMedium());
    }
}

void Machine::i_registerMetrics(PerformanceCollector *aCollector, Machine *aMachine, RTPROCESS pid)
{
    AssertReturnVoid(isWriteLockOnCurrentThread());
    AssertPtrReturnVoid(aCollector);

    pm::CollectorHAL *hal = aCollector->getHAL();
    /* Create sub metrics */
    pm::SubMetric *cpuLoadUser = new pm::SubMetric("CPU/Load/User",
        "Percentage of processor time spent in user mode by the VM process.");
    pm::SubMetric *cpuLoadKernel = new pm::SubMetric("CPU/Load/Kernel",
        "Percentage of processor time spent in kernel mode by the VM process.");
    pm::SubMetric *ramUsageUsed  = new pm::SubMetric("RAM/Usage/Used",
        "Size of resident portion of VM process in memory.");
    pm::SubMetric *diskUsageUsed  = new pm::SubMetric("Disk/Usage/Used",
        "Actual size of all VM disks combined.");
    pm::SubMetric *machineNetRx = new pm::SubMetric("Net/Rate/Rx",
        "Network receive rate.");
    pm::SubMetric *machineNetTx = new pm::SubMetric("Net/Rate/Tx",
        "Network transmit rate.");
    /* Create and register base metrics */
    pm::BaseMetric *cpuLoad = new pm::MachineCpuLoadRaw(hal, aMachine, pid,
                                                        cpuLoadUser, cpuLoadKernel);
    aCollector->registerBaseMetric(cpuLoad);
    pm::BaseMetric *ramUsage = new pm::MachineRamUsage(hal, aMachine, pid,
                                                       ramUsageUsed);
    aCollector->registerBaseMetric(ramUsage);
    MediaList disks;
    i_getDiskList(disks);
    pm::BaseMetric *diskUsage = new pm::MachineDiskUsage(hal, aMachine, disks,
                                                         diskUsageUsed);
    aCollector->registerBaseMetric(diskUsage);

    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser, 0));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser,
                                                new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser,
                                              new pm::AggregateMax()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel, 0));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed, 0));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(diskUsage, diskUsageUsed, 0));
    aCollector->registerMetric(new pm::Metric(diskUsage, diskUsageUsed,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(diskUsage, diskUsageUsed,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(diskUsage, diskUsageUsed,
                                              new pm::AggregateMax()));


    /* Guest metrics collector */
    mCollectorGuest = new pm::CollectorGuest(aMachine, pid);
    aCollector->registerGuest(mCollectorGuest);
    Log7Func(("{%p}: mCollectorGuest=%p\n", this, mCollectorGuest));

    /* Create sub metrics */
    pm::SubMetric *guestLoadUser = new pm::SubMetric("Guest/CPU/Load/User",
        "Percentage of processor time spent in user mode as seen by the guest.");
    pm::SubMetric *guestLoadKernel = new pm::SubMetric("Guest/CPU/Load/Kernel",
        "Percentage of processor time spent in kernel mode as seen by the guest.");
    pm::SubMetric *guestLoadIdle = new pm::SubMetric("Guest/CPU/Load/Idle",
        "Percentage of processor time spent idling as seen by the guest.");

    /* The total amount of physical ram is fixed now, but we'll support dynamic guest ram configurations in the future. */
    pm::SubMetric *guestMemTotal = new pm::SubMetric("Guest/RAM/Usage/Total",      "Total amount of physical guest RAM.");
    pm::SubMetric *guestMemFree = new pm::SubMetric("Guest/RAM/Usage/Free",        "Free amount of physical guest RAM.");
    pm::SubMetric *guestMemBalloon = new pm::SubMetric("Guest/RAM/Usage/Balloon",  "Amount of ballooned physical guest RAM.");
    pm::SubMetric *guestMemShared = new pm::SubMetric("Guest/RAM/Usage/Shared",  "Amount of shared physical guest RAM.");
    pm::SubMetric *guestMemCache = new pm::SubMetric(
                                                "Guest/RAM/Usage/Cache",        "Total amount of guest (disk) cache memory.");

    pm::SubMetric *guestPagedTotal = new pm::SubMetric(
                                         "Guest/Pagefile/Usage/Total",    "Total amount of space in the page file.");

    /* Create and register base metrics */
    pm::BaseMetric *machineNetRate = new pm::MachineNetRate(mCollectorGuest, aMachine,
                                                            machineNetRx, machineNetTx);
    aCollector->registerBaseMetric(machineNetRate);

    pm::BaseMetric *guestCpuLoad = new pm::GuestCpuLoad(mCollectorGuest, aMachine,
                                                        guestLoadUser, guestLoadKernel, guestLoadIdle);
    aCollector->registerBaseMetric(guestCpuLoad);

    pm::BaseMetric *guestCpuMem = new pm::GuestRamUsage(mCollectorGuest, aMachine,
                                                        guestMemTotal, guestMemFree,
                                                        guestMemBalloon, guestMemShared,
                                                        guestMemCache, guestPagedTotal);
    aCollector->registerBaseMetric(guestCpuMem);

    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetRx, 0));
    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetRx, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetRx, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetRx, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetTx, 0));
    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetTx, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetTx, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(machineNetRate, machineNetTx, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadUser, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadUser, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadUser, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadUser, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadKernel, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadKernel, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadKernel, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadKernel, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadIdle, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadIdle, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadIdle, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuLoad, guestLoadIdle, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemTotal, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemTotal, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemTotal, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemTotal, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemFree, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemFree, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemFree, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemFree, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemBalloon, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemBalloon, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemBalloon, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemBalloon, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemShared, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemShared, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemShared, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemShared, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemCache, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemCache, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemCache, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestMemCache, new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestPagedTotal, 0));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestPagedTotal, new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestPagedTotal, new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(guestCpuMem, guestPagedTotal, new pm::AggregateMax()));
}

void Machine::i_unregisterMetrics(PerformanceCollector *aCollector, Machine *aMachine)
{
    AssertReturnVoid(isWriteLockOnCurrentThread());

    if (aCollector)
    {
        aCollector->unregisterMetricsFor(aMachine);
        aCollector->unregisterBaseMetricsFor(aMachine);
    }
}

#endif /* VBOX_WITH_RESOURCE_USAGE_API */


////////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(SessionMachine)

HRESULT SessionMachine::FinalConstruct()
{
    LogFlowThisFunc(("\n"));

    mClientToken = NULL;

    return BaseFinalConstruct();
}

void SessionMachine::FinalRelease()
{
    LogFlowThisFunc(("\n"));

    Assert(!mClientToken);
    /* paranoia, should not hang around any more */
    if (mClientToken)
    {
        delete mClientToken;
        mClientToken = NULL;
    }

    uninit(Uninit::Unexpected);

    BaseFinalRelease();
}

/**
 *  @note Must be called only by Machine::LockMachine() from its own write lock.
 */
HRESULT SessionMachine::init(Machine *aMachine)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("mName={%s}\n", aMachine->mUserData->s.strName.c_str()));

    AssertReturn(aMachine, E_INVALIDARG);

    AssertReturn(aMachine->lockHandle()->isWriteLockOnCurrentThread(), E_FAIL);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    HRESULT rc = S_OK;

    RT_ZERO(mAuthLibCtx);

    /* create the machine client token */
    try
    {
        mClientToken = new ClientToken(aMachine, this);
        if (!mClientToken->isReady())
        {
            delete mClientToken;
            mClientToken = NULL;
            rc = E_FAIL;
        }
    }
    catch (std::bad_alloc &)
    {
        rc = E_OUTOFMEMORY;
    }
    if (FAILED(rc))
        return rc;

    /* memorize the peer Machine */
    unconst(mPeer) = aMachine;
    /* share the parent pointer */
    unconst(mParent) = aMachine->mParent;

    /* take the pointers to data to share */
    mData.share(aMachine->mData);
    mSSData.share(aMachine->mSSData);

    mUserData.share(aMachine->mUserData);
    mHWData.share(aMachine->mHWData);
    mMediumAttachments.share(aMachine->mMediumAttachments);

    mStorageControllers.allocate();
    for (StorageControllerList::const_iterator
         it = aMachine->mStorageControllers->begin();
         it != aMachine->mStorageControllers->end();
         ++it)
    {
        ComObjPtr<StorageController> ctl;
        ctl.createObject();
        ctl->init(this, *it);
        mStorageControllers->push_back(ctl);
    }

    mUSBControllers.allocate();
    for (USBControllerList::const_iterator
         it = aMachine->mUSBControllers->begin();
         it != aMachine->mUSBControllers->end();
         ++it)
    {
        ComObjPtr<USBController> ctl;
        ctl.createObject();
        ctl->init(this, *it);
        mUSBControllers->push_back(ctl);
    }

    unconst(mBIOSSettings).createObject();
    mBIOSSettings->init(this, aMachine->mBIOSSettings);
    /* create another VRDEServer object that will be mutable */
    unconst(mVRDEServer).createObject();
    mVRDEServer->init(this, aMachine->mVRDEServer);
    /* create another audio adapter object that will be mutable */
    unconst(mAudioAdapter).createObject();
    mAudioAdapter->init(this, aMachine->mAudioAdapter);
    /* create a list of serial ports that will be mutable */
    for (ULONG slot = 0; slot < RT_ELEMENTS(mSerialPorts); ++slot)
    {
        unconst(mSerialPorts[slot]).createObject();
        mSerialPorts[slot]->init(this, aMachine->mSerialPorts[slot]);
    }
    /* create a list of parallel ports that will be mutable */
    for (ULONG slot = 0; slot < RT_ELEMENTS(mParallelPorts); ++slot)
    {
        unconst(mParallelPorts[slot]).createObject();
        mParallelPorts[slot]->init(this, aMachine->mParallelPorts[slot]);
    }

    /* create another USB device filters object that will be mutable */
    unconst(mUSBDeviceFilters).createObject();
    mUSBDeviceFilters->init(this, aMachine->mUSBDeviceFilters);

    /* create a list of network adapters that will be mutable */
    mNetworkAdapters.resize(aMachine->mNetworkAdapters.size());
    for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
    {
        unconst(mNetworkAdapters[slot]).createObject();
        mNetworkAdapters[slot]->init(this, aMachine->mNetworkAdapters[slot]);
    }

    /* create another bandwidth control object that will be mutable */
    unconst(mBandwidthControl).createObject();
    mBandwidthControl->init(this, aMachine->mBandwidthControl);

    /* default is to delete saved state on Saved -> PoweredOff transition */
    mRemoveSavedState = true;

    /* Confirm a successful initialization when it's the case */
    autoInitSpan.setSucceeded();

    miNATNetworksStarted = 0;

    LogFlowThisFuncLeave();
    return rc;
}

/**
 *  Uninitializes this session object. If the reason is other than
 *  Uninit::Unexpected, then this method MUST be called from #i_checkForDeath()
 *  or the client watcher code.
 *
 *  @param aReason          uninitialization reason
 *
 *  @note Locks mParent + this object for writing.
 */
void SessionMachine::uninit(Uninit::Reason aReason)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("reason=%d\n", aReason));

    /*
     *  Strongly reference ourselves to prevent this object deletion after
     *  mData->mSession.mMachine.setNull() below (which can release the last
     *  reference and call the destructor). Important: this must be done before
     *  accessing any members (and before AutoUninitSpan that does it as well).
     *  This self reference will be released as the very last step on return.
     */
    ComObjPtr<SessionMachine> selfRef;
    if (aReason != Uninit::Unexpected)
        selfRef = this;

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
    {
        LogFlowThisFunc(("Already uninitialized\n"));
        LogFlowThisFuncLeave();
        return;
    }

    if (autoUninitSpan.initFailed())
    {
        /* We've been called by init() because it's failed. It's not really
         * necessary (nor it's safe) to perform the regular uninit sequence
         * below, the following is enough.
         */
        LogFlowThisFunc(("Initialization failed.\n"));
        /* destroy the machine client token */
        if (mClientToken)
        {
            delete mClientToken;
            mClientToken = NULL;
        }
        uninitDataAndChildObjects();
        mData.free();
        unconst(mParent) = NULL;
        unconst(mPeer) = NULL;
        LogFlowThisFuncLeave();
        return;
    }

    MachineState_T lastState;
    {
        AutoReadLock tempLock(this COMMA_LOCKVAL_SRC_POS);
        lastState = mData->mMachineState;
    }
    NOREF(lastState);

#ifdef VBOX_WITH_USB
    // release all captured USB devices, but do this before requesting the locks below
    if (aReason == Uninit::Abnormal && Global::IsOnline(lastState))
    {
        /* Console::captureUSBDevices() is called in the VM process only after
         * setting the machine state to Starting or Restoring.
         * Console::detachAllUSBDevices() will be called upon successful
         * termination. So, we need to release USB devices only if there was
         * an abnormal termination of a running VM.
         *
         * This is identical to SessionMachine::DetachAllUSBDevices except
         * for the aAbnormal argument. */
        HRESULT rc = mUSBDeviceFilters->i_notifyProxy(false /* aInsertFilters */);
        AssertComRC(rc);
        NOREF(rc);

        USBProxyService *service = mParent->i_host()->i_usbProxyService();
        if (service)
            service->detachAllDevicesFromVM(this, true /* aDone */, true /* aAbnormal */);
    }
#endif /* VBOX_WITH_USB */

    // we need to lock this object in uninit() because the lock is shared
    // with mPeer (as well as data we modify below). mParent lock is needed
    // by several calls to it.
    AutoMultiWriteLock2 multilock(mParent, this COMMA_LOCKVAL_SRC_POS);

#ifdef VBOX_WITH_RESOURCE_USAGE_API
    /*
     * It is safe to call Machine::i_unregisterMetrics() here because
     * PerformanceCollector::samplerCallback no longer accesses guest methods
     * holding the lock.
     */
    i_unregisterMetrics(mParent->i_performanceCollector(), mPeer);
    /* The guest must be unregistered after its metrics (@bugref{5949}). */
    Log7Func(("{%p}: mCollectorGuest=%p\n", this, mCollectorGuest));
    if (mCollectorGuest)
    {
        mParent->i_performanceCollector()->unregisterGuest(mCollectorGuest);
        // delete mCollectorGuest; => CollectorGuestManager::destroyUnregistered()
        mCollectorGuest = NULL;
    }
#endif

    if (aReason == Uninit::Abnormal)
    {
        Log1WarningThisFunc(("ABNORMAL client termination! (wasBusy=%d)\n", Global::IsOnlineOrTransient(lastState)));

        /* reset the state to Aborted */
        if (mData->mMachineState != MachineState_Aborted)
            i_setMachineState(MachineState_Aborted);
    }

    // any machine settings modified?
    if (mData->flModifications)
    {
        Log1WarningThisFunc(("Discarding unsaved settings changes!\n"));
        i_rollback(false /* aNotify */);
    }

    mData->mSession.mPID = NIL_RTPROCESS;

    if (aReason == Uninit::Unexpected)
    {
        /* Uninitialization didn't come from #i_checkForDeath(), so tell the
         * client watcher thread to update the set of machines that have open
         * sessions. */
        mParent->i_updateClientWatcher();
    }

    /* uninitialize all remote controls */
    if (mData->mSession.mRemoteControls.size())
    {
        LogFlowThisFunc(("Closing remote sessions (%d):\n",
                          mData->mSession.mRemoteControls.size()));

        /* Always restart a the beginning, since the iterator is invalidated
         * by using erase(). */
        for (Data::Session::RemoteControlList::iterator
             it = mData->mSession.mRemoteControls.begin();
             it != mData->mSession.mRemoteControls.end();
             it = mData->mSession.mRemoteControls.begin())
        {
            ComPtr<IInternalSessionControl> pControl = *it;
            mData->mSession.mRemoteControls.erase(it);
            multilock.release();
            LogFlowThisFunc(("  Calling remoteControl->Uninitialize()...\n"));
            HRESULT rc = pControl->Uninitialize();
            LogFlowThisFunc(("  remoteControl->Uninitialize() returned %08X\n", rc));
            if (FAILED(rc))
                Log1WarningThisFunc(("Forgot to close the remote session?\n"));
            multilock.acquire();
        }
        mData->mSession.mRemoteControls.clear();
    }

    /* Remove all references to the NAT network service. The service will stop
     * if all references (also from other VMs) are removed. */
    for (; miNATNetworksStarted > 0; miNATNetworksStarted--)
    {
        for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
        {
            BOOL enabled;
            HRESULT hrc = mNetworkAdapters[slot]->COMGETTER(Enabled)(&enabled);
            if (   FAILED(hrc)
                || !enabled)
                continue;

            NetworkAttachmentType_T type;
            hrc = mNetworkAdapters[slot]->COMGETTER(AttachmentType)(&type);
            if (   SUCCEEDED(hrc)
                && type == NetworkAttachmentType_NATNetwork)
            {
                Bstr name;
                hrc = mNetworkAdapters[slot]->COMGETTER(NATNetwork)(name.asOutParam());
                if (SUCCEEDED(hrc))
                {
                    multilock.release();
                    Utf8Str strName(name);
                    LogRel(("VM '%s' stops using NAT network '%s'\n",
                            mUserData->s.strName.c_str(), strName.c_str()));
                    mParent->i_natNetworkRefDec(strName);
                    multilock.acquire();
                }
            }
        }
    }

    /*
     *  An expected uninitialization can come only from #i_checkForDeath().
     *  Otherwise it means that something's gone really wrong (for example,
     *  the Session implementation has released the VirtualBox reference
     *  before it triggered #OnSessionEnd(), or before releasing IPC semaphore,
     *  etc). However, it's also possible, that the client releases the IPC
     *  semaphore correctly (i.e. before it releases the VirtualBox reference),
     *  but the VirtualBox release event comes first to the server process.
     *  This case is practically possible, so we should not assert on an
     *  unexpected uninit, just log a warning.
     */

    if (aReason == Uninit::Unexpected)
        Log1WarningThisFunc(("Unexpected SessionMachine uninitialization!\n"));

    if (aReason != Uninit::Normal)
    {
        mData->mSession.mDirectControl.setNull();
    }
    else
    {
        /* this must be null here (see #OnSessionEnd()) */
        Assert(mData->mSession.mDirectControl.isNull());
        Assert(mData->mSession.mState == SessionState_Unlocking);
        Assert(!mData->mSession.mProgress.isNull());
    }
    if (mData->mSession.mProgress)
    {
        if (aReason == Uninit::Normal)
            mData->mSession.mProgress->i_notifyComplete(S_OK);
        else
            mData->mSession.mProgress->i_notifyComplete(E_FAIL,
                                                        COM_IIDOF(ISession),
                                                        getComponentName(),
                                                        tr("The VM session was aborted"));
        mData->mSession.mProgress.setNull();
    }

    if (mConsoleTaskData.mProgress)
    {
        Assert(aReason == Uninit::Abnormal);
        mConsoleTaskData.mProgress->i_notifyComplete(E_FAIL,
                                                     COM_IIDOF(ISession),
                                                     getComponentName(),
                                                     tr("The VM session was aborted"));
        mConsoleTaskData.mProgress.setNull();
    }

    /* remove the association between the peer machine and this session machine */
    Assert(   (SessionMachine*)mData->mSession.mMachine == this
            || aReason == Uninit::Unexpected);

    /* reset the rest of session data */
    mData->mSession.mLockType = LockType_Null;
    mData->mSession.mMachine.setNull();
    mData->mSession.mState = SessionState_Unlocked;
    mData->mSession.mName.setNull();

    /* destroy the machine client token before leaving the exclusive lock */
    if (mClientToken)
    {
        delete mClientToken;
        mClientToken = NULL;
    }

    /* fire an event */
    mParent->i_onSessionStateChange(mData->mUuid, SessionState_Unlocked);

    uninitDataAndChildObjects();

    /* free the essential data structure last */
    mData.free();

    /* release the exclusive lock before setting the below two to NULL */
    multilock.release();

    unconst(mParent) = NULL;
    unconst(mPeer) = NULL;

    AuthLibUnload(&mAuthLibCtx);

    LogFlowThisFuncLeave();
}

// util::Lockable interface
////////////////////////////////////////////////////////////////////////////////

/**
 *  Overrides VirtualBoxBase::lockHandle() in order to share the lock handle
 *  with the primary Machine instance (mPeer).
 */
RWLockHandle *SessionMachine::lockHandle() const
{
    AssertReturn(mPeer != NULL, NULL);
    return mPeer->lockHandle();
}

// IInternalMachineControl methods
////////////////////////////////////////////////////////////////////////////////

/**
 *  Passes collected guest statistics to performance collector object
 */
HRESULT SessionMachine::reportVmStatistics(ULONG aValidStats, ULONG aCpuUser,
                                           ULONG aCpuKernel, ULONG aCpuIdle,
                                           ULONG aMemTotal, ULONG aMemFree,
                                           ULONG aMemBalloon, ULONG aMemShared,
                                           ULONG aMemCache, ULONG aPageTotal,
                                           ULONG aAllocVMM, ULONG aFreeVMM,
                                           ULONG aBalloonedVMM, ULONG aSharedVMM,
                                           ULONG aVmNetRx, ULONG aVmNetTx)
{
#ifdef VBOX_WITH_RESOURCE_USAGE_API
    if (mCollectorGuest)
        mCollectorGuest->updateStats(aValidStats, aCpuUser, aCpuKernel, aCpuIdle,
                                     aMemTotal, aMemFree, aMemBalloon, aMemShared,
                                     aMemCache, aPageTotal, aAllocVMM, aFreeVMM,
                                     aBalloonedVMM, aSharedVMM, aVmNetRx, aVmNetTx);

    return S_OK;
#else
    NOREF(aValidStats);
    NOREF(aCpuUser);
    NOREF(aCpuKernel);
    NOREF(aCpuIdle);
    NOREF(aMemTotal);
    NOREF(aMemFree);
    NOREF(aMemBalloon);
    NOREF(aMemShared);
    NOREF(aMemCache);
    NOREF(aPageTotal);
    NOREF(aAllocVMM);
    NOREF(aFreeVMM);
    NOREF(aBalloonedVMM);
    NOREF(aSharedVMM);
    NOREF(aVmNetRx);
    NOREF(aVmNetTx);
    return E_NOTIMPL;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// SessionMachine task records
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Task record for saving the machine state.
 */
class SessionMachine::SaveStateTask
    : public Machine::Task
{
public:
    SaveStateTask(SessionMachine *m,
                  Progress *p,
                  const Utf8Str &t,
                  Reason_T enmReason,
                  const Utf8Str &strStateFilePath)
        : Task(m, p, t),
          m_enmReason(enmReason),
          m_strStateFilePath(strStateFilePath)
    {}

private:
    void handler()
    {
        ((SessionMachine *)(Machine *)m_pMachine)->i_saveStateHandler(*this);
    }

    Reason_T m_enmReason;
    Utf8Str m_strStateFilePath;

    friend class SessionMachine;
};

/**
 * Task thread implementation for SessionMachine::SaveState(), called from
 * SessionMachine::taskHandler().
 *
 * @note Locks this object for writing.
 *
 * @param task
 * @return
 */
void SessionMachine::i_saveStateHandler(SaveStateTask &task)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    LogFlowThisFunc(("state=%d\n", getObjectState().getState()));
    if (FAILED(autoCaller.rc()))
    {
        /* we might have been uninitialized because the session was accidentally
         * closed by the client, so don't assert */
        HRESULT rc = setError(E_FAIL,
                              tr("The session has been accidentally closed"));
        task.m_pProgress->i_notifyComplete(rc);
        LogFlowThisFuncLeave();
        return;
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    try
    {
        ComPtr<IInternalSessionControl> directControl;
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
        if (directControl.isNull())
            throw setError(VBOX_E_INVALID_VM_STATE,
                           tr("Trying to save state without a running VM"));
        alock.release();
        BOOL fSuspendedBySave;
        rc = directControl->SaveStateWithReason(task.m_enmReason, task.m_pProgress, NULL, Bstr(task.m_strStateFilePath).raw(), task.m_machineStateBackup != MachineState_Paused, &fSuspendedBySave);
        Assert(!fSuspendedBySave);
        alock.acquire();

        AssertStmt(   (SUCCEEDED(rc) && mData->mMachineState == MachineState_Saved)
                   || (FAILED(rc) && mData->mMachineState == MachineState_Saving),
                   throw E_FAIL);

        if (SUCCEEDED(rc))
        {
            mSSData->strStateFilePath = task.m_strStateFilePath;

            /* save all VM settings */
            rc = i_saveSettings(NULL);
                    // no need to check whether VirtualBox.xml needs saving also since
                    // we can't have a name change pending at this point
        }
        else
        {
            // On failure, set the state to the state we had at the beginning.
            i_setMachineState(task.m_machineStateBackup);
            i_updateMachineStateOnClient();

            // Delete the saved state file (might have been already created).
            // No need to check whether this is shared with a snapshot here
            // because we certainly created a fresh saved state file here.
            RTFileDelete(task.m_strStateFilePath.c_str());
        }
    }
    catch (HRESULT aRC) { rc = aRC; }

    task.m_pProgress->i_notifyComplete(rc);

    LogFlowThisFuncLeave();
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::saveState(ComPtr<IProgress> &aProgress)
{
    return i_saveStateWithReason(Reason_Unspecified, aProgress);
}

HRESULT SessionMachine::i_saveStateWithReason(Reason_T aReason, ComPtr<IProgress> &aProgress)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrRunningStateDep);
    if (FAILED(rc)) return rc;

    if (   mData->mMachineState != MachineState_Running
        && mData->mMachineState != MachineState_Paused
       )
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot save the execution state as the machine is not running or paused (machine state: %s)"),
            Global::stringifyMachineState(mData->mMachineState));

    ComObjPtr<Progress> pProgress;
    pProgress.createObject();
    rc = pProgress->init(i_getVirtualBox(),
                         static_cast<IMachine *>(this) /* aInitiator */,
                         tr("Saving the execution state of the virtual machine"),
                         FALSE /* aCancelable */);
    if (FAILED(rc))
        return rc;

    Utf8Str strStateFilePath;
    i_composeSavedStateFilename(strStateFilePath);

    /* create and start the task on a separate thread (note that it will not
     * start working until we release alock) */
    SaveStateTask *pTask = new SaveStateTask(this, pProgress, "SaveState", aReason, strStateFilePath);
    rc = pTask->createThread();
    if (FAILED(rc))
        return rc;

    /* set the state to Saving (expected by Session::SaveStateWithReason()) */
    i_setMachineState(MachineState_Saving);
    i_updateMachineStateOnClient();

    pProgress.queryInterfaceTo(aProgress.asOutParam());

    return S_OK;
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::adoptSavedState(const com::Utf8Str &aSavedStateFile)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableStateDep);
    if (FAILED(rc)) return rc;

    if (   mData->mMachineState != MachineState_PoweredOff
        && mData->mMachineState != MachineState_Teleported
        && mData->mMachineState != MachineState_Aborted
       )
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot adopt the saved machine state as the machine is not in Powered Off, Teleported or Aborted state (machine state: %s)"),
            Global::stringifyMachineState(mData->mMachineState));

    com::Utf8Str stateFilePathFull;
    int vrc = i_calculateFullPath(aSavedStateFile, stateFilePathFull);
    if (RT_FAILURE(vrc))
        return setError(VBOX_E_FILE_ERROR,
                        tr("Invalid saved state file path '%s' (%Rrc)"),
                        aSavedStateFile.c_str(),
                        vrc);

    mSSData->strStateFilePath = stateFilePathFull;

    /* The below i_setMachineState() will detect the state transition and will
     * update the settings file */

    return i_setMachineState(MachineState_Saved);
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::discardSavedState(BOOL aFRemoveFile)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_checkStateDependency(MutableOrSavedStateDep);
    if (FAILED(rc)) return rc;

    if (mData->mMachineState != MachineState_Saved)
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot delete the machine state as the machine is not in the saved state (machine state: %s)"),
            Global::stringifyMachineState(mData->mMachineState));

    mRemoveSavedState = RT_BOOL(aFRemoveFile);

    /*
     * Saved -> PoweredOff transition will be detected in the SessionMachine
     * and properly handled.
     */
    rc = i_setMachineState(MachineState_PoweredOff);
    return rc;
}


/**
 *  @note Locks the same as #i_setMachineState() does.
 */
HRESULT SessionMachine::updateState(MachineState_T aState)
{
    return i_setMachineState(aState);
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::beginPowerUp(const ComPtr<IProgress> &aProgress)
{
    IProgress *pProgress(aProgress);

    LogFlowThisFunc(("aProgress=%p\n", pProgress));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData->mSession.mState != SessionState_Locked)
        return VBOX_E_INVALID_OBJECT_STATE;

    if (!mData->mSession.mProgress.isNull())
        mData->mSession.mProgress->setOtherProgressObject(pProgress);

    /* If we didn't reference the NAT network service yet, add a reference to
     * force a start */
    if (miNATNetworksStarted < 1)
    {
        for (ULONG slot = 0; slot < mNetworkAdapters.size(); ++slot)
        {
            BOOL enabled;
            HRESULT hrc = mNetworkAdapters[slot]->COMGETTER(Enabled)(&enabled);
            if (   FAILED(hrc)
                || !enabled)
                continue;

            NetworkAttachmentType_T type;
            hrc = mNetworkAdapters[slot]->COMGETTER(AttachmentType)(&type);
            if (   SUCCEEDED(hrc)
                && type == NetworkAttachmentType_NATNetwork)
            {
                Bstr name;
                hrc = mNetworkAdapters[slot]->COMGETTER(NATNetwork)(name.asOutParam());
                if (SUCCEEDED(hrc))
                {
                    Utf8Str strName(name);
                    LogRel(("VM '%s' starts using NAT network '%s'\n",
                            mUserData->s.strName.c_str(), strName.c_str()));
                    mPeer->lockHandle()->unlockWrite();
                    mParent->i_natNetworkRefInc(strName);
#ifdef RT_LOCK_STRICT
                    mPeer->lockHandle()->lockWrite(RT_SRC_POS);
#else
                    mPeer->lockHandle()->lockWrite();
#endif
                }
            }
        }
        miNATNetworksStarted++;
    }

    LogFlowThisFunc(("returns S_OK.\n"));
    return S_OK;
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::endPowerUp(LONG aResult)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData->mSession.mState != SessionState_Locked)
        return VBOX_E_INVALID_OBJECT_STATE;

    /* Finalize the LaunchVMProcess progress object. */
    if (mData->mSession.mProgress)
    {
        mData->mSession.mProgress->notifyComplete((HRESULT)aResult);
        mData->mSession.mProgress.setNull();
    }

    if (SUCCEEDED((HRESULT)aResult))
    {
#ifdef VBOX_WITH_RESOURCE_USAGE_API
        /* The VM has been powered up successfully, so it makes sense
         * now to offer the performance metrics for a running machine
         * object. Doing it earlier wouldn't be safe. */
        i_registerMetrics(mParent->i_performanceCollector(), mPeer,
                          mData->mSession.mPID);
#endif /* VBOX_WITH_RESOURCE_USAGE_API */
    }

    return S_OK;
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::beginPoweringDown(ComPtr<IProgress> &aProgress)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(mConsoleTaskData.mLastState == MachineState_Null,
                 E_FAIL);

    /* create a progress object to track operation completion */
    ComObjPtr<Progress> pProgress;
    pProgress.createObject();
    pProgress->init(i_getVirtualBox(),
                    static_cast<IMachine *>(this) /* aInitiator */,
                    tr("Stopping the virtual machine"),
                    FALSE /* aCancelable */);

    /* fill in the console task data */
    mConsoleTaskData.mLastState = mData->mMachineState;
    mConsoleTaskData.mProgress = pProgress;

    /* set the state to Stopping (this is expected by Console::PowerDown()) */
    i_setMachineState(MachineState_Stopping);

    pProgress.queryInterfaceTo(aProgress.asOutParam());

    return S_OK;
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::endPoweringDown(LONG aResult,
                                        const com::Utf8Str &aErrMsg)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(    (   (SUCCEEDED(aResult) && mData->mMachineState == MachineState_PoweredOff)
                      || (FAILED(aResult) && mData->mMachineState == MachineState_Stopping))
                  && mConsoleTaskData.mLastState != MachineState_Null,
                 E_FAIL);

    /*
     * On failure, set the state to the state we had when BeginPoweringDown()
     * was called (this is expected by Console::PowerDown() and the associated
     * task). On success the VM process already changed the state to
     * MachineState_PoweredOff, so no need to do anything.
     */
    if (FAILED(aResult))
        i_setMachineState(mConsoleTaskData.mLastState);

    /* notify the progress object about operation completion */
    Assert(mConsoleTaskData.mProgress);
    if (SUCCEEDED(aResult))
        mConsoleTaskData.mProgress->i_notifyComplete(S_OK);
    else
    {
        if (aErrMsg.length())
            mConsoleTaskData.mProgress->i_notifyComplete(aResult,
                                                         COM_IIDOF(ISession),
                                                         getComponentName(),
                                                         aErrMsg.c_str());
        else
            mConsoleTaskData.mProgress->i_notifyComplete(aResult);
    }

    /* clear out the temporary saved state data */
    mConsoleTaskData.mLastState = MachineState_Null;
    mConsoleTaskData.mProgress.setNull();

    LogFlowThisFuncLeave();
    return S_OK;
}


/**
 *  Goes through the USB filters of the given machine to see if the given
 *  device matches any filter or not.
 *
 *  @note Locks the same as USBController::hasMatchingFilter() does.
 */
HRESULT SessionMachine::runUSBDeviceFilters(const ComPtr<IUSBDevice> &aDevice,
                                            BOOL  *aMatched,
                                            ULONG *aMaskedInterfaces)
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_USB
    *aMatched = mUSBDeviceFilters->i_hasMatchingFilter(aDevice, aMaskedInterfaces);
#else
    NOREF(aDevice);
    NOREF(aMaskedInterfaces);
    *aMatched = FALSE;
#endif

    return S_OK;
}

/**
 *  @note Locks the same as Host::captureUSBDevice() does.
 */
HRESULT SessionMachine::captureUSBDevice(const com::Guid &aId, const com::Utf8Str &aCaptureFilename)
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_USB
    /* if captureDeviceForVM() fails, it must have set extended error info */
    clearError();
    MultiResult rc = mParent->i_host()->i_checkUSBProxyService();
    if (FAILED(rc)) return rc;

    USBProxyService *service = mParent->i_host()->i_usbProxyService();
    AssertReturn(service, E_FAIL);
    return service->captureDeviceForVM(this, aId.ref(), aCaptureFilename);
#else
    NOREF(aId);
    return E_NOTIMPL;
#endif
}

/**
 *  @note Locks the same as Host::detachUSBDevice() does.
 */
HRESULT SessionMachine::detachUSBDevice(const com::Guid &aId,
                                        BOOL aDone)
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_USB
    USBProxyService *service = mParent->i_host()->i_usbProxyService();
    AssertReturn(service, E_FAIL);
    return service->detachDeviceFromVM(this, aId.ref(), !!aDone);
#else
    NOREF(aId);
    NOREF(aDone);
    return E_NOTIMPL;
#endif
}

/**
 *  Inserts all machine filters to the USB proxy service and then calls
 *  Host::autoCaptureUSBDevices().
 *
 *  Called by Console from the VM process upon VM startup.
 *
 *  @note Locks what called methods lock.
 */
HRESULT SessionMachine::autoCaptureUSBDevices()
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_USB
    HRESULT rc = mUSBDeviceFilters->i_notifyProxy(true /* aInsertFilters */);
    AssertComRC(rc);
    NOREF(rc);

    USBProxyService *service = mParent->i_host()->i_usbProxyService();
    AssertReturn(service, E_FAIL);
    return service->autoCaptureDevicesForVM(this);
#else
    return S_OK;
#endif
}

/**
 *  Removes all machine filters from the USB proxy service and then calls
 *  Host::detachAllUSBDevices().
 *
 *  Called by Console from the VM process upon normal VM termination or by
 *  SessionMachine::uninit() upon abnormal VM termination (from under the
 *  Machine/SessionMachine lock).
 *
 *  @note Locks what called methods lock.
 */
HRESULT SessionMachine::detachAllUSBDevices(BOOL aDone)
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_USB
    HRESULT rc = mUSBDeviceFilters->i_notifyProxy(false /* aInsertFilters */);
    AssertComRC(rc);
    NOREF(rc);

    USBProxyService *service = mParent->i_host()->i_usbProxyService();
    AssertReturn(service, E_FAIL);
    return service->detachAllDevicesFromVM(this, !!aDone, false /* aAbnormal */);
#else
    NOREF(aDone);
    return S_OK;
#endif
}

/**
 *  @note Locks this object for writing.
 */
HRESULT SessionMachine::onSessionEnd(const ComPtr<ISession> &aSession,
                                     ComPtr<IProgress> &aProgress)
{
    LogFlowThisFuncEnter();

    LogFlowThisFunc(("callerstate=%d\n", getObjectState().getState()));
    /*
     *  We don't assert below because it might happen that a non-direct session
     *  informs us it is closed right after we've been uninitialized -- it's ok.
     */

    /* get IInternalSessionControl interface */
    ComPtr<IInternalSessionControl> control(aSession);

    ComAssertRet(!control.isNull(), E_INVALIDARG);

    /* Creating a Progress object requires the VirtualBox lock, and
     * thus locking it here is required by the lock order rules. */
    AutoMultiWriteLock2 alock(mParent, this COMMA_LOCKVAL_SRC_POS);

    if (control == mData->mSession.mDirectControl)
    {
        /* The direct session is being normally closed by the client process
         * ----------------------------------------------------------------- */

        /* go to the closing state (essential for all open*Session() calls and
         * for #i_checkForDeath()) */
        Assert(mData->mSession.mState == SessionState_Locked);
        mData->mSession.mState = SessionState_Unlocking;

        /* set direct control to NULL to release the remote instance */
        mData->mSession.mDirectControl.setNull();
        LogFlowThisFunc(("Direct control is set to NULL\n"));

        if (mData->mSession.mProgress)
        {
            /* finalize the progress, someone might wait if a frontend
             * closes the session before powering on the VM. */
            mData->mSession.mProgress->notifyComplete(E_FAIL,
                                                      COM_IIDOF(ISession),
                                                      getComponentName(),
                                                      tr("The VM session was closed before any attempt to power it on"));
            mData->mSession.mProgress.setNull();
        }

        /* Create the progress object the client will use to wait until
         * #i_checkForDeath() is called to uninitialize this session object after
         * it releases the IPC semaphore.
         * Note! Because we're "reusing" mProgress here, this must be a proxy
         *       object just like for LaunchVMProcess. */
        Assert(mData->mSession.mProgress.isNull());
        ComObjPtr<ProgressProxy> progress;
        progress.createObject();
        ComPtr<IUnknown> pPeer(mPeer);
        progress->init(mParent, pPeer,
                       Bstr(tr("Closing session")).raw(),
                       FALSE /* aCancelable */);
        progress.queryInterfaceTo(aProgress.asOutParam());
        mData->mSession.mProgress = progress;
    }
    else
    {
        /* the remote session is being normally closed */
        bool found = false;
        for (Data::Session::RemoteControlList::iterator
             it = mData->mSession.mRemoteControls.begin();
             it != mData->mSession.mRemoteControls.end();
             ++it)
        {
            if (control == *it)
            {
                found = true;
                // This MUST be erase(it), not remove(*it) as the latter
                // triggers a very nasty use after free due to the place where
                // the value "lives".
                mData->mSession.mRemoteControls.erase(it);
                break;
            }
        }
        ComAssertMsgRet(found, ("The session is not found in the session list!"),
                         E_INVALIDARG);
    }

    /* signal the client watcher thread, because the client is going away */
    mParent->i_updateClientWatcher();

    LogFlowThisFuncLeave();
    return S_OK;
}

HRESULT SessionMachine::pullGuestProperties(std::vector<com::Utf8Str> &aNames,
                                            std::vector<com::Utf8Str> &aValues,
                                            std::vector<LONG64>       &aTimestamps,
                                            std::vector<com::Utf8Str> &aFlags)
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_GUEST_PROPS
    using namespace guestProp;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    size_t cEntries = mHWData->mGuestProperties.size();
    aNames.resize(cEntries);
    aValues.resize(cEntries);
    aTimestamps.resize(cEntries);
    aFlags.resize(cEntries);

    size_t  i = 0;
    for (HWData::GuestPropertyMap::const_iterator
         it = mHWData->mGuestProperties.begin();
         it != mHWData->mGuestProperties.end();
         ++it, ++i)
    {
        char szFlags[MAX_FLAGS_LEN + 1];
        aNames[i] = it->first;
        aValues[i] = it->second.strValue;
        aTimestamps[i] = it->second.mTimestamp;

        /* If it is NULL, keep it NULL. */
        if (it->second.mFlags)
        {
            writeFlags(it->second.mFlags, szFlags);
            aFlags[i] = szFlags;
        }
        else
            aFlags[i] = "";
    }
    return S_OK;
#else
    ReturnComNotImplemented();
#endif
}

HRESULT SessionMachine::pushGuestProperty(const com::Utf8Str &aName,
                                          const com::Utf8Str &aValue,
                                          LONG64 aTimestamp,
                                          const com::Utf8Str &aFlags)
{
    LogFlowThisFunc(("\n"));

#ifdef VBOX_WITH_GUEST_PROPS
    using namespace guestProp;

    try
    {
        /*
         * Convert input up front.
         */
        uint32_t fFlags = NILFLAG;
        if (aFlags.length())
        {
            int vrc = validateFlags(aFlags.c_str(), &fFlags);
            AssertRCReturn(vrc, E_INVALIDARG);
        }

        /*
         * Now grab the object lock, validate the state and do the update.
         */

        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (!Global::IsOnline(mData->mMachineState))
        {
            AssertMsgFailedReturn(("%s\n", Global::stringifyMachineState(mData->mMachineState)),
                                  VBOX_E_INVALID_VM_STATE);
        }

        i_setModified(IsModified_MachineData);
        mHWData.backup();

        bool fDelete = !aValue.length();
        HWData::GuestPropertyMap::iterator it = mHWData->mGuestProperties.find(aName);
        if (it != mHWData->mGuestProperties.end())
        {
            if (!fDelete)
            {
                it->second.strValue   = aValue;
                it->second.mTimestamp = aTimestamp;
                it->second.mFlags     = fFlags;
            }
            else
                mHWData->mGuestProperties.erase(it);

            mData->mGuestPropertiesModified = TRUE;
        }
        else if (!fDelete)
        {
            HWData::GuestProperty prop;
            prop.strValue   = aValue;
            prop.mTimestamp = aTimestamp;
            prop.mFlags     = fFlags;

            mHWData->mGuestProperties[aName] = prop;
            mData->mGuestPropertiesModified = TRUE;
        }

        alock.release();

        mParent->i_onGuestPropertyChange(mData->mUuid,
                                         Bstr(aName).raw(),
                                         Bstr(aValue).raw(),
                                         Bstr(aFlags).raw());
    }
    catch (...)
    {
        return VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
    }
    return S_OK;
#else
    ReturnComNotImplemented();
#endif
}


HRESULT SessionMachine::lockMedia()
{
    AutoMultiWriteLock2 alock(this->lockHandle(),
                              &mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    AssertReturn(   mData->mMachineState == MachineState_Starting
                 || mData->mMachineState == MachineState_Restoring
                 || mData->mMachineState == MachineState_TeleportingIn, E_FAIL);

    clearError();
    alock.release();
    return i_lockMedia();
}

HRESULT SessionMachine::unlockMedia()
{
    HRESULT hrc = i_unlockMedia();
    return hrc;
}

HRESULT SessionMachine::ejectMedium(const ComPtr<IMediumAttachment> &aAttachment,
                                    ComPtr<IMediumAttachment> &aNewAttachment)
{
    // request the host lock first, since might be calling Host methods for getting host drives;
    // next, protect the media tree all the while we're in here, as well as our member variables
    AutoMultiWriteLock3 multiLock(mParent->i_host()->lockHandle(),
                                  this->lockHandle(),
                                  &mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    IMediumAttachment *iAttach = aAttachment;
    ComObjPtr<MediumAttachment> pAttach = static_cast<MediumAttachment *>(iAttach);

    Utf8Str ctrlName;
    LONG lPort;
    LONG lDevice;
    bool fTempEject;
    {
        AutoReadLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);

        /* Need to query the details first, as the IMediumAttachment reference
         * might be to the original settings, which we are going to change. */
        ctrlName = pAttach->i_getControllerName();
        lPort = pAttach->i_getPort();
        lDevice = pAttach->i_getDevice();
        fTempEject = pAttach->i_getTempEject();
    }

    if (!fTempEject)
    {
        /* Remember previously mounted medium. The medium before taking the
         * backup is not necessarily the same thing. */
        ComObjPtr<Medium> oldmedium;
        oldmedium = pAttach->i_getMedium();

        i_setModified(IsModified_Storage);
        mMediumAttachments.backup();

        // The backup operation makes the pAttach reference point to the
        // old settings. Re-get the correct reference.
        pAttach = i_findAttachment(*mMediumAttachments.data(),
                                   ctrlName,
                                   lPort,
                                   lDevice);

        {
            AutoCaller autoAttachCaller(this);
            if (FAILED(autoAttachCaller.rc())) return autoAttachCaller.rc();

            AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);
            if (!oldmedium.isNull())
                oldmedium->i_removeBackReference(mData->mUuid);

            pAttach->i_updateMedium(NULL);
            pAttach->i_updateEjected();
        }

        i_setModified(IsModified_Storage);
    }
    else
    {
        {
            AutoWriteLock attLock(pAttach COMMA_LOCKVAL_SRC_POS);
            pAttach->i_updateEjected();
        }
    }

    pAttach.queryInterfaceTo(aNewAttachment.asOutParam());

    return S_OK;
}

HRESULT SessionMachine::authenticateExternal(const std::vector<com::Utf8Str> &aAuthParams,
                                             com::Utf8Str &aResult)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT hr = S_OK;

    if (!mAuthLibCtx.hAuthLibrary)
    {
        /* Load the external authentication library. */
        Bstr authLibrary;
        mVRDEServer->COMGETTER(AuthLibrary)(authLibrary.asOutParam());

        Utf8Str filename = authLibrary;

        int rc = AuthLibLoad(&mAuthLibCtx, filename.c_str());
        if (RT_FAILURE(rc))
        {
            hr = setError(E_FAIL,
                          tr("Could not load the external authentication library '%s' (%Rrc)"),
                          filename.c_str(), rc);
        }
    }

    /* The auth library might need the machine lock. */
    alock.release();

    if (FAILED(hr))
       return hr;

    if (aAuthParams[0] == "VRDEAUTH" && aAuthParams.size() == 7)
    {
        enum VRDEAuthParams
        {
           parmUuid = 1,
           parmGuestJudgement,
           parmUser,
           parmPassword,
           parmDomain,
           parmClientId
        };

        AuthResult result = AuthResultAccessDenied;

        Guid uuid(aAuthParams[parmUuid]);
        AuthGuestJudgement guestJudgement = (AuthGuestJudgement)aAuthParams[parmGuestJudgement].toUInt32();
        uint32_t u32ClientId = aAuthParams[parmClientId].toUInt32();

        result = AuthLibAuthenticate(&mAuthLibCtx,
                                     uuid.raw(), guestJudgement,
                                     aAuthParams[parmUser].c_str(),
                                     aAuthParams[parmPassword].c_str(),
                                     aAuthParams[parmDomain].c_str(),
                                     u32ClientId);

        /* Hack: aAuthParams[parmPassword] is const but the code believes in writable memory. */
        size_t cbPassword = aAuthParams[parmPassword].length();
        if (cbPassword)
        {
            RTMemWipeThoroughly((void *)aAuthParams[parmPassword].c_str(), cbPassword, 10 /* cPasses */);
            memset((void *)aAuthParams[parmPassword].c_str(), 'x', cbPassword);
        }

        if (result == AuthResultAccessGranted)
            aResult = "granted";
        else
            aResult = "denied";

        LogRel(("AUTH: VRDE authentification for user '%s' result '%s'\n",
                aAuthParams[parmUser].c_str(), aResult.c_str()));
    }
    else if (aAuthParams[0] == "VRDEAUTHDISCONNECT" && aAuthParams.size() == 3)
    {
        enum VRDEAuthDisconnectParams
        {
           parmUuid = 1,
           parmClientId
        };

        Guid uuid(aAuthParams[parmUuid]);
        uint32_t u32ClientId = 0;
        AuthLibDisconnect(&mAuthLibCtx, uuid.raw(), u32ClientId);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// public methods only for internal purposes
/////////////////////////////////////////////////////////////////////////////

#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
/**
 * Called from the client watcher thread to check for expected or unexpected
 * death of the client process that has a direct session to this machine.
 *
 * On Win32 and on OS/2, this method is called only when we've got the
 * mutex (i.e. the client has either died or terminated normally) so it always
 * returns @c true (the client is terminated, the session machine is
 * uninitialized).
 *
 * On other platforms, the method returns @c true if the client process has
 * terminated normally or abnormally and the session machine was uninitialized,
 * and @c false if the client process is still alive.
 *
 * @note Locks this object for writing.
 */
bool SessionMachine::i_checkForDeath()
{
    Uninit::Reason reason;
    bool terminated = false;

    /* Enclose autoCaller with a block because calling uninit() from under it
     * will deadlock. */
    {
        AutoCaller autoCaller(this);
        if (!autoCaller.isOk())
        {
            /* return true if not ready, to cause the client watcher to exclude
             * the corresponding session from watching */
            LogFlowThisFunc(("Already uninitialized!\n"));
            return true;
        }

        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        /* Determine the reason of death: if the session state is Closing here,
         * everything is fine. Otherwise it means that the client did not call
         * OnSessionEnd() before it released the IPC semaphore. This may happen
         * either because the client process has abnormally terminated, or
         * because it simply forgot to call ISession::Close() before exiting. We
         * threat the latter also as an abnormal termination (see
         * Session::uninit() for details). */
        reason = mData->mSession.mState == SessionState_Unlocking ?
                 Uninit::Normal :
                 Uninit::Abnormal;

        if (mClientToken)
            terminated = mClientToken->release();
    } /* AutoCaller block */

    if (terminated)
        uninit(reason);

    return terminated;
}

void SessionMachine::i_getTokenId(Utf8Str &strTokenId)
{
    LogFlowThisFunc(("\n"));

    strTokenId.setNull();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    Assert(mClientToken);
    if (mClientToken)
        mClientToken->getId(strTokenId);
}
#else /* VBOX_WITH_GENERIC_SESSION_WATCHER */
IToken *SessionMachine::i_getToken()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), NULL);

    Assert(mClientToken);
    if (mClientToken)
        return mClientToken->getToken();
    else
        return NULL;
}
#endif /* VBOX_WITH_GENERIC_SESSION_WATCHER */

Machine::ClientToken *SessionMachine::i_getClientToken()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), NULL);

    return mClientToken;
}


/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onNetworkAdapterChange(INetworkAdapter *networkAdapter, BOOL changeAdapter)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnNetworkAdapterChange(networkAdapter, changeAdapter);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onNATRedirectRuleChange(ULONG ulSlot, BOOL aNatRuleRemove, IN_BSTR aRuleName,
                                                  NATProtocol_T aProto, IN_BSTR aHostIp, LONG aHostPort,
                                                  IN_BSTR aGuestIp, LONG aGuestPort)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;
    /*
     * instead acting like callback we ask IVirtualBox deliver corresponding event
     */

    mParent->i_onNatRedirectChange(i_getId(), ulSlot, RT_BOOL(aNatRuleRemove), aRuleName, aProto, aHostIp,
                                   (uint16_t)aHostPort, aGuestIp, (uint16_t)aGuestPort);
    return S_OK;
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onAudioAdapterChange(IAudioAdapter *audioAdapter)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnAudioAdapterChange(audioAdapter);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onSerialPortChange(ISerialPort *serialPort)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnSerialPortChange(serialPort);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onParallelPortChange(IParallelPort *parallelPort)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnParallelPortChange(parallelPort);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onStorageControllerChange()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnStorageControllerChange();
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onMediumChange(IMediumAttachment *aAttachment, BOOL aForce)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnMediumChange(aAttachment, aForce);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onCPUChange(ULONG aCPU, BOOL aRemove)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnCPUChange(aCPU, aRemove);
}

HRESULT SessionMachine::i_onCPUExecutionCapChange(ULONG aExecutionCap)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnCPUExecutionCapChange(aExecutionCap);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onVRDEServerChange(BOOL aRestart)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnVRDEServerChange(aRestart);
}

/**
 * @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onVideoCaptureChange()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnVideoCaptureChange();
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onUSBControllerChange()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnUSBControllerChange();
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onSharedFolderChange()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnSharedFolderChange(FALSE /* aGlobal */);
}

/**
 * @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onClipboardModeChange(ClipboardMode_T aClipboardMode)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnClipboardModeChange(aClipboardMode);
}

/**
 * @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onDnDModeChange(DnDMode_T aDnDMode)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnDnDModeChange(aDnDMode);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onBandwidthGroupChange(IBandwidthGroup *aBandwidthGroup)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnBandwidthGroupChange(aBandwidthGroup);
}

/**
 *  @note Locks this object for reading.
 */
HRESULT SessionMachine::i_onStorageDeviceChange(IMediumAttachment *aAttachment, BOOL aRemove, BOOL aSilent)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->OnStorageDeviceChange(aAttachment, aRemove, aSilent);
}

/**
 *  Returns @c true if this machine's USB controller reports it has a matching
 *  filter for the given USB device and @c false otherwise.
 *
 *  @note locks this object for reading.
 */
bool SessionMachine::i_hasMatchingUSBFilter(const ComObjPtr<HostUSBDevice> &aDevice, ULONG *aMaskedIfs)
{
    AutoCaller autoCaller(this);
    /* silently return if not ready -- this method may be called after the
     * direct machine session has been called */
    if (!autoCaller.isOk())
        return false;

#ifdef VBOX_WITH_USB
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    switch (mData->mMachineState)
    {
        case MachineState_Starting:
        case MachineState_Restoring:
        case MachineState_TeleportingIn:
        case MachineState_Paused:
        case MachineState_Running:
        /** @todo Live Migration: snapshoting & teleporting. Need to fend things of
         *        elsewhere... */
            alock.release();
            return mUSBDeviceFilters->i_hasMatchingFilter(aDevice, aMaskedIfs);
        default: break;
    }
#else
    NOREF(aDevice);
    NOREF(aMaskedIfs);
#endif
    return false;
}

/**
 *  @note The calls shall hold no locks. Will temporarily lock this object for reading.
 */
HRESULT SessionMachine::i_onUSBDeviceAttach(IUSBDevice *aDevice,
                                            IVirtualBoxErrorInfo *aError,
                                            ULONG aMaskedIfs,
                                            const com::Utf8Str &aCaptureFilename)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);

    /* This notification may happen after the machine object has been
     * uninitialized (the session was closed), so don't assert. */
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* fail on notifications sent after #OnSessionEnd() is called, it is
     * expected by the caller */
    if (!directControl)
        return E_FAIL;

    /* No locks should be held at this point. */
    AssertMsg(RTLockValidatorWriteLockGetCount(RTThreadSelf()) == 0, ("%d\n", RTLockValidatorWriteLockGetCount(RTThreadSelf())));
    AssertMsg(RTLockValidatorReadLockGetCount(RTThreadSelf()) == 0, ("%d\n", RTLockValidatorReadLockGetCount(RTThreadSelf())));

    return directControl->OnUSBDeviceAttach(aDevice, aError, aMaskedIfs, Bstr(aCaptureFilename).raw());
}

/**
 *  @note The calls shall hold no locks. Will temporarily lock this object for reading.
 */
HRESULT SessionMachine::i_onUSBDeviceDetach(IN_BSTR aId,
                                            IVirtualBoxErrorInfo *aError)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);

    /* This notification may happen after the machine object has been
     * uninitialized (the session was closed), so don't assert. */
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;
    }

    /* fail on notifications sent after #OnSessionEnd() is called, it is
     * expected by the caller */
    if (!directControl)
        return E_FAIL;

    /* No locks should be held at this point. */
    AssertMsg(RTLockValidatorWriteLockGetCount(RTThreadSelf()) == 0, ("%d\n", RTLockValidatorWriteLockGetCount(RTThreadSelf())));
    AssertMsg(RTLockValidatorReadLockGetCount(RTThreadSelf()) == 0, ("%d\n", RTLockValidatorReadLockGetCount(RTThreadSelf())));

    return directControl->OnUSBDeviceDetach(aId, aError);
}

// protected methods
/////////////////////////////////////////////////////////////////////////////

/**
 * Deletes the given file if it is no longer in use by either the current machine state
 * (if the machine is "saved") or any of the machine's snapshots.
 *
 * Note: This checks mSSData->strStateFilePath, which is shared by the Machine and SessionMachine
 * but is different for each SnapshotMachine. When calling this, the order of calling this
 * function on the one hand and changing that variable OR the snapshots tree on the other hand
 * is therefore critical. I know, it's all rather messy.
 *
 * @param strStateFile
 * @param pSnapshotToIgnore  Passed to Snapshot::sharesSavedStateFile(); this snapshot is ignored in
 * the test for whether the saved state file is in use.
 */
void SessionMachine::i_releaseSavedStateFile(const Utf8Str &strStateFile,
                                             Snapshot *pSnapshotToIgnore)
{
    // it is safe to delete this saved state file if it is not currently in use by the machine ...
    if (    (strStateFile.isNotEmpty())
         && (strStateFile != mSSData->strStateFilePath)     // session machine's saved state
       )
        // ... and it must also not be shared with other snapshots
        if (    !mData->mFirstSnapshot
             || !mData->mFirstSnapshot->i_sharesSavedStateFile(strStateFile, pSnapshotToIgnore)
                                // this checks the SnapshotMachine's state file paths
           )
            RTFileDelete(strStateFile.c_str());
}

/**
 * Locks the attached media.
 *
 * All attached hard disks are locked for writing and DVD/floppy are locked for
 * reading. Parents of attached hard disks (if any) are locked for reading.
 *
 * This method also performs accessibility check of all media it locks: if some
 * media is inaccessible, the method will return a failure and a bunch of
 * extended error info objects per each inaccessible medium.
 *
 * Note that this method is atomic: if it returns a success, all media are
 * locked as described above; on failure no media is locked at all (all
 * succeeded individual locks will be undone).
 *
 * The caller is responsible for doing the necessary state sanity checks.
 *
 * The locks made by this method must be undone by calling #unlockMedia() when
 * no more needed.
 */
HRESULT SessionMachine::i_lockMedia()
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    AutoMultiWriteLock2 alock(this->lockHandle(),
                              &mParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    /* bail out if trying to lock things with already set up locking */
    AssertReturn(mData->mSession.mLockedMedia.IsEmpty(), E_FAIL);

    MultiResult mrc(S_OK);

    /* Collect locking information for all medium objects attached to the VM. */
    for (MediumAttachmentList::const_iterator
         it = mMediumAttachments->begin();
         it != mMediumAttachments->end();
         ++it)
    {
        MediumAttachment *pAtt = *it;
        DeviceType_T devType = pAtt->i_getType();
        Medium *pMedium = pAtt->i_getMedium();

        MediumLockList *pMediumLockList(new MediumLockList());
        // There can be attachments without a medium (floppy/dvd), and thus
        // it's impossible to create a medium lock list. It still makes sense
        // to have the empty medium lock list in the map in case a medium is
        // attached later.
        if (pMedium != NULL)
        {
            MediumType_T mediumType = pMedium->i_getType();
            bool fIsReadOnlyLock =    mediumType == MediumType_Readonly
                                   || mediumType == MediumType_Shareable;
            bool fIsVitalImage = (devType == DeviceType_HardDisk);

            alock.release();
            mrc = pMedium->i_createMediumLockList(fIsVitalImage /* fFailIfInaccessible */,
                                                  !fIsReadOnlyLock ? pMedium : NULL /* pToLockWrite */,
                                                  false /* fMediumLockWriteAll */,
                                                  NULL,
                                                  *pMediumLockList);
            alock.acquire();
            if (FAILED(mrc))
            {
                delete pMediumLockList;
                mData->mSession.mLockedMedia.Clear();
                break;
            }
        }

        HRESULT rc = mData->mSession.mLockedMedia.Insert(pAtt, pMediumLockList);
        if (FAILED(rc))
        {
            mData->mSession.mLockedMedia.Clear();
            mrc = setError(rc,
                           tr("Collecting locking information for all attached media failed"));
            break;
        }
    }

    if (SUCCEEDED(mrc))
    {
        /* Now lock all media. If this fails, nothing is locked. */
        alock.release();
        HRESULT rc = mData->mSession.mLockedMedia.Lock();
        alock.acquire();
        if (FAILED(rc))
        {
            mrc = setError(rc,
                           tr("Locking of attached media failed. A possible reason is that one of the media is attached to a running VM"));
        }
    }

    return mrc;
}

/**
 * Undoes the locks made by by #lockMedia().
 */
HRESULT SessionMachine::i_unlockMedia()
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(),autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* we may be holding important error info on the current thread;
     * preserve it */
    ErrorInfoKeeper eik;

    HRESULT rc = mData->mSession.mLockedMedia.Clear();
    AssertComRC(rc);
    return rc;
}

/**
 * Helper to change the machine state (reimplementation).
 *
 * @note Locks this object for writing.
 * @note This method must not call i_saveSettings or SaveSettings, otherwise
 *       it can cause crashes in random places due to unexpectedly committing
 *       the current settings. The caller is responsible for that. The call
 *       to saveStateSettings is fine, because this method does not commit.
 */
HRESULT SessionMachine::i_setMachineState(MachineState_T aMachineState)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aMachineState=%s\n", Global::stringifyMachineState(aMachineState) ));

    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    MachineState_T oldMachineState = mData->mMachineState;

    AssertMsgReturn(oldMachineState != aMachineState,
                    ("oldMachineState=%s, aMachineState=%s\n",
                     Global::stringifyMachineState(oldMachineState), Global::stringifyMachineState(aMachineState)),
                    E_FAIL);

    HRESULT rc = S_OK;

    int stsFlags = 0;
    bool deleteSavedState = false;

    /* detect some state transitions */

    if (   (   oldMachineState == MachineState_Saved
            && aMachineState   == MachineState_Restoring)
        || (   (   oldMachineState == MachineState_PoweredOff
                || oldMachineState == MachineState_Teleported
                || oldMachineState == MachineState_Aborted
               )
            && (   aMachineState   == MachineState_TeleportingIn
                || aMachineState   == MachineState_Starting
               )
           )
       )
    {
        /* The EMT thread is about to start */

        /* Nothing to do here for now... */

        /// @todo NEWMEDIA don't let mDVDDrive and other children
        /// change anything when in the Starting/Restoring state
    }
    else if (   (   oldMachineState == MachineState_Running
                 || oldMachineState == MachineState_Paused
                 || oldMachineState == MachineState_Teleporting
                 || oldMachineState == MachineState_OnlineSnapshotting
                 || oldMachineState == MachineState_LiveSnapshotting
                 || oldMachineState == MachineState_Stuck
                 || oldMachineState == MachineState_Starting
                 || oldMachineState == MachineState_Stopping
                 || oldMachineState == MachineState_Saving
                 || oldMachineState == MachineState_Restoring
                 || oldMachineState == MachineState_TeleportingPausedVM
                 || oldMachineState == MachineState_TeleportingIn
                 )
             && (   aMachineState == MachineState_PoweredOff
                 || aMachineState == MachineState_Saved
                 || aMachineState == MachineState_Teleported
                 || aMachineState == MachineState_Aborted
                )
            )
    {
        /* The EMT thread has just stopped, unlock attached media. Note that as
         * opposed to locking that is done from Console, we do unlocking here
         * because the VM process may have aborted before having a chance to
         * properly unlock all media it locked. */

        unlockMedia();
    }

    if (oldMachineState == MachineState_Restoring)
    {
        if (aMachineState != MachineState_Saved)
        {
            /*
             *  delete the saved state file once the machine has finished
             *  restoring from it (note that Console sets the state from
             *  Restoring to Saved if the VM couldn't restore successfully,
             *  to give the user an ability to fix an error and retry --
             *  we keep the saved state file in this case)
             */
            deleteSavedState = true;
        }
    }
    else if (   oldMachineState == MachineState_Saved
             && (   aMachineState == MachineState_PoweredOff
                 || aMachineState == MachineState_Aborted
                 || aMachineState == MachineState_Teleported
                )
            )
    {
        /*
         *  delete the saved state after SessionMachine::ForgetSavedState() is called
         *  or if the VM process (owning a direct VM session) crashed while the
         *  VM was Saved
         */

        /// @todo (dmik)
        //      Not sure that deleting the saved state file just because of the
        //      client death before it attempted to restore the VM is a good
        //      thing. But when it crashes we need to go to the Aborted state
        //      which cannot have the saved state file associated... The only
        //      way to fix this is to make the Aborted condition not a VM state
        //      but a bool flag: i.e., when a crash occurs, set it to true and
        //      change the state to PoweredOff or Saved depending on the
        //      saved state presence.

        deleteSavedState = true;
        mData->mCurrentStateModified = TRUE;
        stsFlags |= SaveSTS_CurStateModified;
    }

    if (   aMachineState == MachineState_Starting
        || aMachineState == MachineState_Restoring
        || aMachineState == MachineState_TeleportingIn
       )
    {
        /* set the current state modified flag to indicate that the current
         * state is no more identical to the state in the
         * current snapshot */
        if (!mData->mCurrentSnapshot.isNull())
        {
            mData->mCurrentStateModified = TRUE;
            stsFlags |= SaveSTS_CurStateModified;
        }
    }

    if (deleteSavedState)
    {
        if (mRemoveSavedState)
        {
            Assert(!mSSData->strStateFilePath.isEmpty());

            // it is safe to delete the saved state file if ...
            if (    !mData->mFirstSnapshot      // ... we have no snapshots or
                 || !mData->mFirstSnapshot->i_sharesSavedStateFile(mSSData->strStateFilePath, NULL /* pSnapshotToIgnore */)
                                                // ... none of the snapshots share the saved state file
               )
                RTFileDelete(mSSData->strStateFilePath.c_str());
        }

        mSSData->strStateFilePath.setNull();
        stsFlags |= SaveSTS_StateFilePath;
    }

    /* redirect to the underlying peer machine */
    mPeer->i_setMachineState(aMachineState);

    if (   oldMachineState != MachineState_RestoringSnapshot
        && (   aMachineState == MachineState_PoweredOff
            || aMachineState == MachineState_Teleported
            || aMachineState == MachineState_Aborted
            || aMachineState == MachineState_Saved))
    {
        /* the machine has stopped execution
         * (or the saved state file was adopted) */
        stsFlags |= SaveSTS_StateTimeStamp;
    }

    if (   (   oldMachineState == MachineState_PoweredOff
            || oldMachineState == MachineState_Aborted
            || oldMachineState == MachineState_Teleported
           )
        && aMachineState == MachineState_Saved)
    {
        /* the saved state file was adopted */
        Assert(!mSSData->strStateFilePath.isEmpty());
        stsFlags |= SaveSTS_StateFilePath;
    }

#ifdef VBOX_WITH_GUEST_PROPS
    if (   aMachineState == MachineState_PoweredOff
        || aMachineState == MachineState_Aborted
        || aMachineState == MachineState_Teleported)
    {
        /* Make sure any transient guest properties get removed from the
         * property store on shutdown. */
        BOOL fNeedsSaving = mData->mGuestPropertiesModified;

        /* remove it from the settings representation */
        settings::GuestPropertiesList &llGuestProperties = mData->pMachineConfigFile->hardwareMachine.llGuestProperties;
        for (settings::GuestPropertiesList::iterator
             it = llGuestProperties.begin();
             it != llGuestProperties.end();
             /*nothing*/)
        {
            const settings::GuestProperty &prop = *it;
            if (   prop.strFlags.contains("TRANSRESET", Utf8Str::CaseInsensitive)
                || prop.strFlags.contains("TRANSIENT", Utf8Str::CaseInsensitive))
            {
                it = llGuestProperties.erase(it);
                fNeedsSaving = true;
            }
            else
            {
                ++it;
            }
        }

        /* Additionally remove it from the HWData representation. Required to
         * keep everything in sync, as this is what the API keeps using. */
        HWData::GuestPropertyMap &llHWGuestProperties = mHWData->mGuestProperties;
        for (HWData::GuestPropertyMap::iterator
             it = llHWGuestProperties.begin();
             it != llHWGuestProperties.end();
             /*nothing*/)
        {
            uint32_t fFlags = it->second.mFlags;
            if (   fFlags & guestProp::TRANSIENT
                || fFlags & guestProp::TRANSRESET)
            {
                /* iterator where we need to continue after the erase call
                 * (C++03 is a fact still, and it doesn't return the iterator
                 * which would allow continuing) */
                HWData::GuestPropertyMap::iterator it2 = it;
                ++it2;
                llHWGuestProperties.erase(it);
                it = it2;
                fNeedsSaving = true;
            }
            else
            {
                ++it;
            }
        }

        if (fNeedsSaving)
        {
            mData->mCurrentStateModified = TRUE;
            stsFlags |= SaveSTS_CurStateModified;
        }
    }
#endif /* VBOX_WITH_GUEST_PROPS */

    rc = i_saveStateSettings(stsFlags);

    if (   (   oldMachineState != MachineState_PoweredOff
            && oldMachineState != MachineState_Aborted
            && oldMachineState != MachineState_Teleported
           )
        && (   aMachineState == MachineState_PoweredOff
            || aMachineState == MachineState_Aborted
            || aMachineState == MachineState_Teleported
           )
       )
    {
        /* we've been shut down for any reason */
        /* no special action so far */
    }

    LogFlowThisFunc(("rc=%Rhrc [%s]\n", rc, Global::stringifyMachineState(mData->mMachineState) ));
    LogFlowThisFuncLeave();
    return rc;
}

/**
 *  Sends the current machine state value to the VM process.
 *
 *  @note Locks this object for reading, then calls a client process.
 */
HRESULT SessionMachine::i_updateMachineStateOnClient()
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), autoCaller.rc());

    ComPtr<IInternalSessionControl> directControl;
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        AssertReturn(!!mData, E_FAIL);
        if (mData->mSession.mLockType == LockType_VM)
            directControl = mData->mSession.mDirectControl;

        /* directControl may be already set to NULL here in #OnSessionEnd()
         * called too early by the direct session process while there is still
         * some operation (like deleting the snapshot) in progress. The client
         * process in this case is waiting inside Session::close() for the
         * "end session" process object to complete, while #uninit() called by
         * #i_checkForDeath() on the Watcher thread is waiting for the pending
         * operation to complete. For now, we accept this inconsistent behavior
         * and simply do nothing here. */

        if (mData->mSession.mState == SessionState_Unlocking)
            return S_OK;
    }

    /* ignore notifications sent after #OnSessionEnd() is called */
    if (!directControl)
        return S_OK;

    return directControl->UpdateMachineState(mData->mMachineState);
}


/*static*/
HRESULT Machine::i_setErrorStatic(HRESULT aResultCode, const char *pcszMsg, ...)
{
    va_list args;
    va_start(args, pcszMsg);
    HRESULT rc = setErrorInternal(aResultCode,
                                  getStaticClassIID(),
                                  getStaticComponentName(),
                                  Utf8Str(pcszMsg, args),
                                  false /* aWarning */,
                                  true /* aLogIt */);
    va_end(args);
    return rc;
}


HRESULT Machine::updateState(MachineState_T aState)
{
    NOREF(aState);
    ReturnComNotImplemented();
}

HRESULT Machine::beginPowerUp(const ComPtr<IProgress> &aProgress)
{
    NOREF(aProgress);
    ReturnComNotImplemented();
}

HRESULT Machine::endPowerUp(LONG aResult)
{
    NOREF(aResult);
    ReturnComNotImplemented();
}

HRESULT Machine::beginPoweringDown(ComPtr<IProgress> &aProgress)
{
    NOREF(aProgress);
    ReturnComNotImplemented();
}

HRESULT Machine::endPoweringDown(LONG aResult,
                                 const com::Utf8Str &aErrMsg)
{
    NOREF(aResult);
    NOREF(aErrMsg);
    ReturnComNotImplemented();
}

HRESULT Machine::runUSBDeviceFilters(const ComPtr<IUSBDevice> &aDevice,
                                     BOOL  *aMatched,
                                     ULONG *aMaskedInterfaces)
{
    NOREF(aDevice);
    NOREF(aMatched);
    NOREF(aMaskedInterfaces);
    ReturnComNotImplemented();

}

HRESULT Machine::captureUSBDevice(const com::Guid &aId, const com::Utf8Str &aCaptureFilename)
{
    NOREF(aId); NOREF(aCaptureFilename);
    ReturnComNotImplemented();
}

HRESULT Machine::detachUSBDevice(const com::Guid &aId,
                                 BOOL aDone)
{
    NOREF(aId);
    NOREF(aDone);
    ReturnComNotImplemented();
}

HRESULT Machine::autoCaptureUSBDevices()
{
    ReturnComNotImplemented();
}

HRESULT Machine::detachAllUSBDevices(BOOL aDone)
{
    NOREF(aDone);
    ReturnComNotImplemented();
}

HRESULT Machine::onSessionEnd(const ComPtr<ISession> &aSession,
                              ComPtr<IProgress> &aProgress)
{
    NOREF(aSession);
    NOREF(aProgress);
    ReturnComNotImplemented();
}

HRESULT Machine::finishOnlineMergeMedium()
{
    ReturnComNotImplemented();
}

HRESULT Machine::pullGuestProperties(std::vector<com::Utf8Str> &aNames,
                                     std::vector<com::Utf8Str> &aValues,
                                     std::vector<LONG64> &aTimestamps,
                                     std::vector<com::Utf8Str> &aFlags)
{
    NOREF(aNames);
    NOREF(aValues);
    NOREF(aTimestamps);
    NOREF(aFlags);
    ReturnComNotImplemented();
}

HRESULT Machine::pushGuestProperty(const com::Utf8Str &aName,
                                   const com::Utf8Str &aValue,
                                   LONG64 aTimestamp,
                                   const com::Utf8Str &aFlags)
{
    NOREF(aName);
    NOREF(aValue);
    NOREF(aTimestamp);
    NOREF(aFlags);
    ReturnComNotImplemented();
}

HRESULT Machine::lockMedia()
{
    ReturnComNotImplemented();
}

HRESULT Machine::unlockMedia()
{
    ReturnComNotImplemented();
}

HRESULT Machine::ejectMedium(const ComPtr<IMediumAttachment> &aAttachment,
                             ComPtr<IMediumAttachment> &aNewAttachment)
{
    NOREF(aAttachment);
    NOREF(aNewAttachment);
    ReturnComNotImplemented();
}

HRESULT Machine::reportVmStatistics(ULONG aValidStats,
                                    ULONG aCpuUser,
                                    ULONG aCpuKernel,
                                    ULONG aCpuIdle,
                                    ULONG aMemTotal,
                                    ULONG aMemFree,
                                    ULONG aMemBalloon,
                                    ULONG aMemShared,
                                    ULONG aMemCache,
                                    ULONG aPagedTotal,
                                    ULONG aMemAllocTotal,
                                    ULONG aMemFreeTotal,
                                    ULONG aMemBalloonTotal,
                                    ULONG aMemSharedTotal,
                                    ULONG aVmNetRx,
                                    ULONG aVmNetTx)
{
    NOREF(aValidStats);
    NOREF(aCpuUser);
    NOREF(aCpuKernel);
    NOREF(aCpuIdle);
    NOREF(aMemTotal);
    NOREF(aMemFree);
    NOREF(aMemBalloon);
    NOREF(aMemShared);
    NOREF(aMemCache);
    NOREF(aPagedTotal);
    NOREF(aMemAllocTotal);
    NOREF(aMemFreeTotal);
    NOREF(aMemBalloonTotal);
    NOREF(aMemSharedTotal);
    NOREF(aVmNetRx);
    NOREF(aVmNetTx);
    ReturnComNotImplemented();
}

HRESULT Machine::authenticateExternal(const std::vector<com::Utf8Str> &aAuthParams,
                                             com::Utf8Str &aResult)
{
    NOREF(aAuthParams);
    NOREF(aResult);
    ReturnComNotImplemented();
}

HRESULT Machine::applyDefaults(const com::Utf8Str &aFlags)
{
    NOREF(aFlags);
    ReturnComNotImplemented();
}

/* This isn't handled entirely by the wrapper generator yet. */
#ifdef VBOX_WITH_XPCOM
NS_DECL_CLASSINFO(SessionMachine)
NS_IMPL_THREADSAFE_ISUPPORTS2_CI(SessionMachine, IMachine, IInternalMachineControl)

NS_DECL_CLASSINFO(SnapshotMachine)
NS_IMPL_THREADSAFE_ISUPPORTS1_CI(SnapshotMachine, IMachine)
#endif
