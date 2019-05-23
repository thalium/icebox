/* $Id: ConsoleImpl.cpp $ */
/** @file
 * VBox Console COM Class implementation
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

#define LOG_GROUP LOG_GROUP_MAIN_CONSOLE
#include "LoggingNew.h"

/** @todo Move the TAP mess back into the driver! */
#if defined(RT_OS_WINDOWS)
#elif defined(RT_OS_LINUX)
# include <errno.h>
# include <sys/ioctl.h>
# include <sys/poll.h>
# include <sys/fcntl.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <net/if.h>
# include <linux/if_tun.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#elif defined(RT_OS_FREEBSD)
# include <errno.h>
# include <sys/ioctl.h>
# include <sys/poll.h>
# include <sys/fcntl.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#elif defined(RT_OS_SOLARIS)
# include <iprt/coredumper.h>
#endif

#include "ConsoleImpl.h"

#include "Global.h"
#include "VirtualBoxErrorInfoImpl.h"
#include "GuestImpl.h"
#include "KeyboardImpl.h"
#include "MouseImpl.h"
#include "DisplayImpl.h"
#include "MachineDebuggerImpl.h"
#include "USBDeviceImpl.h"
#include "RemoteUSBDeviceImpl.h"
#include "SharedFolderImpl.h"
#ifdef VBOX_WITH_VRDE_AUDIO
# include "DrvAudioVRDE.h"
#endif
#ifdef VBOX_WITH_AUDIO_VIDEOREC
# include "DrvAudioVideoRec.h"
#endif
#include "Nvram.h"
#ifdef VBOX_WITH_USB_CARDREADER
# include "UsbCardReader.h"
#endif
#include "ProgressImpl.h"
#include "ConsoleVRDPServer.h"
#include "VMMDev.h"
#ifdef VBOX_WITH_EXTPACK
# include "ExtPackManagerImpl.h"
#endif
#include "BusAssignmentManager.h"
#include "PCIDeviceAttachmentImpl.h"
#include "EmulatedUSBImpl.h"

#include "VBoxEvents.h"
#include "AutoCaller.h"
#include "ThreadTask.h"

#ifdef VBOX_WITH_VIDEOREC
# include "VideoRec.h"
#endif

#include <VBox/com/array.h>
#include "VBox/com/ErrorInfo.h"
#include <VBox/com/listeners.h>

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/cpp/utils.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/ldr.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/base64.h>
#include <iprt/memsafer.h>

#include <VBox/vmm/vmapi.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/vmm/pdmasynccompletion.h>
#include <VBox/vmm/pdmnetifs.h>
#include <VBox/vmm/pdmstorageifs.h>
#ifdef VBOX_WITH_USB
# include <VBox/vmm/pdmusb.h>
#endif
#ifdef VBOX_WITH_NETSHAPER
# include <VBox/vmm/pdmnetshaper.h>
#endif /* VBOX_WITH_NETSHAPER */
#include <VBox/vmm/mm.h>
#include <VBox/vmm/ftm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/vusb.h>

#include <VBox/VMMDev.h>

#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/HostServices/DragAndDropSvc.h>
#ifdef VBOX_WITH_GUEST_PROPS
# include <VBox/HostServices/GuestPropertySvc.h>
# include <VBox/com/array.h>
#endif

#ifdef VBOX_OPENSSL_FIPS
# include <openssl/crypto.h>
#endif

#include <set>
#include <algorithm>
#include <memory> // for auto_ptr
#include <vector>
#include <exception>// std::exception

// VMTask and friends
////////////////////////////////////////////////////////////////////////////////

/**
 * Task structure for asynchronous VM operations.
 *
 * Once created, the task structure adds itself as a Console caller. This means:
 *
 * 1. The user must check for #rc() before using the created structure
 *    (e.g. passing it as a thread function argument). If #rc() returns a
 *    failure, the Console object may not be used by the task.
 * 2. On successful initialization, the structure keeps the Console caller
 *    until destruction (to ensure Console remains in the Ready state and won't
 *    be accidentally uninitialized). Forgetting to delete the created task
 *    will lead to Console::uninit() stuck waiting for releasing all added
 *    callers.
 *
 * If \a aUsesVMPtr parameter is true, the task structure will also add itself
 * as a Console::mpUVM caller with the same meaning as above. See
 * Console::addVMCaller() for more info.
 */
class VMTask: public ThreadTask
{
public:
    VMTask(Console *aConsole,
           Progress *aProgress,
           const ComPtr<IProgress> &aServerProgress,
           bool aUsesVMPtr)
        : ThreadTask("GenericVMTask"),
          mConsole(aConsole),
          mConsoleCaller(aConsole),
          mProgress(aProgress),
          mServerProgress(aServerProgress),
          mRC(E_FAIL),
          mpSafeVMPtr(NULL)
    {
        AssertReturnVoid(aConsole);
        mRC = mConsoleCaller.rc();
        if (FAILED(mRC))
            return;
        if (aUsesVMPtr)
        {
            mpSafeVMPtr = new Console::SafeVMPtr(aConsole);
            if (!mpSafeVMPtr->isOk())
                mRC = mpSafeVMPtr->rc();
        }
    }

    virtual ~VMTask()
    {
        releaseVMCaller();
    }

    HRESULT rc() const { return mRC; }
    bool isOk() const { return SUCCEEDED(rc()); }

    /** Releases the VM caller before destruction. Not normally necessary. */
    void releaseVMCaller()
    {
        if (mpSafeVMPtr)
        {
            delete mpSafeVMPtr;
            mpSafeVMPtr = NULL;
        }
    }

    const ComObjPtr<Console>    mConsole;
    AutoCaller                  mConsoleCaller;
    const ComObjPtr<Progress>   mProgress;
    Utf8Str                     mErrorMsg;
    const ComPtr<IProgress>     mServerProgress;

private:
    HRESULT                     mRC;
    Console::SafeVMPtr         *mpSafeVMPtr;
};


class VMPowerUpTask : public VMTask
{
public:
    VMPowerUpTask(Console *aConsole,
                  Progress *aProgress)
        : VMTask(aConsole, aProgress, NULL /* aServerProgress */,
                 false /* aUsesVMPtr */),
          mConfigConstructor(NULL),
          mStartPaused(false),
          mTeleporterEnabled(FALSE),
          mEnmFaultToleranceState(FaultToleranceState_Inactive)
    {
        m_strTaskName = "VMPwrUp";
    }

    PFNCFGMCONSTRUCTOR mConfigConstructor;
    Utf8Str mSavedStateFile;
    Console::SharedFolderDataMap mSharedFolders;
    bool mStartPaused;
    BOOL mTeleporterEnabled;
    FaultToleranceState_T mEnmFaultToleranceState;

    /* array of progress objects for hard disk reset operations */
    typedef std::list<ComPtr<IProgress> > ProgressList;
    ProgressList hardDiskProgresses;

    void handler()
    {
        Console::i_powerUpThreadTask(this);
    }

};

class VMPowerDownTask : public VMTask
{
public:
    VMPowerDownTask(Console *aConsole,
                    const ComPtr<IProgress> &aServerProgress)
        : VMTask(aConsole, NULL /* aProgress */, aServerProgress,
                 true /* aUsesVMPtr */)
    {
        m_strTaskName = "VMPwrDwn";
    }

    void handler()
    {
        Console::i_powerDownThreadTask(this);
    }
};

// Handler for global events
////////////////////////////////////////////////////////////////////////////////
inline static const char *networkAdapterTypeToName(NetworkAdapterType_T adapterType);

class VmEventListener {
public:
    VmEventListener()
    {}


    HRESULT init(Console *aConsole)
    {
        mConsole = aConsole;
        return S_OK;
    }

    void uninit()
    {
    }

    virtual ~VmEventListener()
    {
    }

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent)
    {
        switch(aType)
        {
            case VBoxEventType_OnNATRedirect:
            {
                Bstr id;
                ComPtr<IMachine> pMachine = mConsole->i_machine();
                ComPtr<INATRedirectEvent> pNREv = aEvent;
                HRESULT rc = E_FAIL;
                Assert(pNREv);

                rc = pNREv->COMGETTER(MachineId)(id.asOutParam());
                AssertComRC(rc);
                if (id != mConsole->i_getId())
                    break;
                /* now we can operate with redirects */
                NATProtocol_T proto;
                pNREv->COMGETTER(Proto)(&proto);
                BOOL fRemove;
                pNREv->COMGETTER(Remove)(&fRemove);
                Bstr hostIp, guestIp;
                LONG hostPort, guestPort;
                pNREv->COMGETTER(HostIP)(hostIp.asOutParam());
                pNREv->COMGETTER(HostPort)(&hostPort);
                pNREv->COMGETTER(GuestIP)(guestIp.asOutParam());
                pNREv->COMGETTER(GuestPort)(&guestPort);
                ULONG ulSlot;
                rc = pNREv->COMGETTER(Slot)(&ulSlot);
                AssertComRC(rc);
                if (FAILED(rc))
                    break;
                mConsole->i_onNATRedirectRuleChange(ulSlot, fRemove, proto, hostIp.raw(), hostPort, guestIp.raw(), guestPort);
            }
            break;

            case VBoxEventType_OnHostNameResolutionConfigurationChange:
            {
                mConsole->i_onNATDnsChanged();
                break;
            }

            case VBoxEventType_OnHostPCIDevicePlug:
            {
                // handle if needed
                break;
            }

            case VBoxEventType_OnExtraDataChanged:
            {
                ComPtr<IExtraDataChangedEvent> pEDCEv = aEvent;
                Bstr strMachineId;
                Bstr strKey;
                Bstr strVal;
                HRESULT hrc = S_OK;

                hrc = pEDCEv->COMGETTER(MachineId)(strMachineId.asOutParam());
                if (FAILED(hrc)) break;

                hrc = pEDCEv->COMGETTER(Key)(strKey.asOutParam());
                if (FAILED(hrc)) break;

                hrc = pEDCEv->COMGETTER(Value)(strVal.asOutParam());
                if (FAILED(hrc)) break;

                mConsole->i_onExtraDataChange(strMachineId.raw(), strKey.raw(), strVal.raw());
                break;
            }

            default:
              AssertFailed();
        }

        return S_OK;
    }
private:
    ComObjPtr<Console>    mConsole;
};

typedef ListenerImpl<VmEventListener, Console*> VmEventListenerImpl;


VBOX_LISTENER_DECLARE(VmEventListenerImpl)


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

Console::Console()
    : mSavedStateDataLoaded(false)
    , mConsoleVRDPServer(NULL)
    , mfVRDEChangeInProcess(false)
    , mfVRDEChangePending(false)
    , mpUVM(NULL)
    , mVMCallers(0)
    , mVMZeroCallersSem(NIL_RTSEMEVENT)
    , mVMDestroying(false)
    , mVMPoweredOff(false)
    , mVMIsAlreadyPoweringOff(false)
    , mfSnapshotFolderSizeWarningShown(false)
    , mfSnapshotFolderExt4WarningShown(false)
    , mfSnapshotFolderDiskTypeShown(false)
    , mfVMHasUsbController(false)
    , mfPowerOffCausedByReset(false)
    , mpVmm2UserMethods(NULL)
    , m_pVMMDev(NULL)
    , mAudioVRDE(NULL)
#ifdef VBOX_WITH_AUDIO_VIDEOREC
    , mAudioVideoRec(NULL)
#endif
    , mNvram(NULL)
#ifdef VBOX_WITH_USB_CARDREADER
    , mUsbCardReader(NULL)
#endif
    , mBusMgr(NULL)
    , m_pKeyStore(NULL)
    , mpIfSecKey(NULL)
    , mpIfSecKeyHlp(NULL)
    , mVMStateChangeCallbackDisabled(false)
    , mfUseHostClipboard(true)
    , mMachineState(MachineState_PoweredOff)
{
}

Console::~Console()
{}

HRESULT Console::FinalConstruct()
{
    LogFlowThisFunc(("\n"));

    RT_ZERO(mapStorageLeds);
    RT_ZERO(mapNetworkLeds);
    RT_ZERO(mapUSBLed);
    RT_ZERO(mapSharedFolderLed);
    RT_ZERO(mapCrOglLed);

    for (unsigned i = 0; i < RT_ELEMENTS(maStorageDevType); ++i)
        maStorageDevType[i] = DeviceType_Null;

    MYVMM2USERMETHODS *pVmm2UserMethods = (MYVMM2USERMETHODS *)RTMemAllocZ(sizeof(*mpVmm2UserMethods) + sizeof(Console *));
    if (!pVmm2UserMethods)
        return E_OUTOFMEMORY;
    pVmm2UserMethods->u32Magic          = VMM2USERMETHODS_MAGIC;
    pVmm2UserMethods->u32Version        = VMM2USERMETHODS_VERSION;
    pVmm2UserMethods->pfnSaveState      = Console::i_vmm2User_SaveState;
    pVmm2UserMethods->pfnNotifyEmtInit  = Console::i_vmm2User_NotifyEmtInit;
    pVmm2UserMethods->pfnNotifyEmtTerm  = Console::i_vmm2User_NotifyEmtTerm;
    pVmm2UserMethods->pfnNotifyPdmtInit = Console::i_vmm2User_NotifyPdmtInit;
    pVmm2UserMethods->pfnNotifyPdmtTerm = Console::i_vmm2User_NotifyPdmtTerm;
    pVmm2UserMethods->pfnNotifyResetTurnedIntoPowerOff = Console::i_vmm2User_NotifyResetTurnedIntoPowerOff;
    pVmm2UserMethods->pfnQueryGenericObject = Console::i_vmm2User_QueryGenericObject;
    pVmm2UserMethods->u32EndMagic       = VMM2USERMETHODS_MAGIC;
    pVmm2UserMethods->pConsole          = this;
    mpVmm2UserMethods = pVmm2UserMethods;

    MYPDMISECKEY *pIfSecKey = (MYPDMISECKEY *)RTMemAllocZ(sizeof(*mpIfSecKey) + sizeof(Console *));
    if (!pIfSecKey)
        return E_OUTOFMEMORY;
    pIfSecKey->pfnKeyRetain             = Console::i_pdmIfSecKey_KeyRetain;
    pIfSecKey->pfnKeyRelease            = Console::i_pdmIfSecKey_KeyRelease;
    pIfSecKey->pfnPasswordRetain        = Console::i_pdmIfSecKey_PasswordRetain;
    pIfSecKey->pfnPasswordRelease       = Console::i_pdmIfSecKey_PasswordRelease;
    pIfSecKey->pConsole                 = this;
    mpIfSecKey = pIfSecKey;

    MYPDMISECKEYHLP *pIfSecKeyHlp = (MYPDMISECKEYHLP *)RTMemAllocZ(sizeof(*mpIfSecKeyHlp) + sizeof(Console *));
    if (!pIfSecKeyHlp)
        return E_OUTOFMEMORY;
    pIfSecKeyHlp->pfnKeyMissingNotify   = Console::i_pdmIfSecKeyHlp_KeyMissingNotify;
    pIfSecKeyHlp->pConsole              = this;
    mpIfSecKeyHlp = pIfSecKeyHlp;

    return BaseFinalConstruct();
}

void Console::FinalRelease()
{
    LogFlowThisFunc(("\n"));

    uninit();

    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

HRESULT Console::init(IMachine *aMachine, IInternalMachineControl *aControl, LockType_T aLockType)
{
    AssertReturn(aMachine && aControl, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aMachine=%p, aControl=%p\n", aMachine, aControl));

    HRESULT rc = E_FAIL;

    unconst(mMachine) = aMachine;
    unconst(mControl) = aControl;

    /* Cache essential properties and objects, and create child objects */

    rc = mMachine->COMGETTER(State)(&mMachineState);
    AssertComRCReturnRC(rc);

    rc = mMachine->COMGETTER(Id)(mstrUuid.asOutParam());
    AssertComRCReturnRC(rc);

#ifdef VBOX_WITH_EXTPACK
    unconst(mptrExtPackManager).createObject();
    rc = mptrExtPackManager->initExtPackManager(NULL, VBOXEXTPACKCTX_VM_PROCESS);
        AssertComRCReturnRC(rc);
#endif

    // Event source may be needed by other children
    unconst(mEventSource).createObject();
    rc = mEventSource->init();
    AssertComRCReturnRC(rc);

    mcAudioRefs = 0;
    mcVRDPClients = 0;
    mu32SingleRDPClientId = 0;
    mcGuestCredentialsProvided = false;

    /* Now the VM specific parts */
    if (aLockType == LockType_VM)
    {
        rc = mMachine->COMGETTER(VRDEServer)(unconst(mVRDEServer).asOutParam());
        AssertComRCReturnRC(rc);

        unconst(mGuest).createObject();
        rc = mGuest->init(this);
        AssertComRCReturnRC(rc);

        ULONG cCpus = 1;
        rc = mMachine->COMGETTER(CPUCount)(&cCpus);
        mGuest->i_setCpuCount(cCpus);

        unconst(mKeyboard).createObject();
        rc = mKeyboard->init(this);
        AssertComRCReturnRC(rc);

        unconst(mMouse).createObject();
        rc = mMouse->init(this);
        AssertComRCReturnRC(rc);

        unconst(mDisplay).createObject();
        rc = mDisplay->init(this);
        AssertComRCReturnRC(rc);

        unconst(mVRDEServerInfo).createObject();
        rc = mVRDEServerInfo->init(this);
        AssertComRCReturnRC(rc);

        unconst(mEmulatedUSB).createObject();
        rc = mEmulatedUSB->init(this);
        AssertComRCReturnRC(rc);

        /* Grab global and machine shared folder lists */

        rc = i_fetchSharedFolders(true /* aGlobal */);
        AssertComRCReturnRC(rc);
        rc = i_fetchSharedFolders(false /* aGlobal */);
        AssertComRCReturnRC(rc);

        /* Create other child objects */

        unconst(mConsoleVRDPServer) = new ConsoleVRDPServer(this);
        AssertReturn(mConsoleVRDPServer, E_FAIL);

        /* Figure out size of meAttachmentType vector */
        ComPtr<IVirtualBox> pVirtualBox;
        rc = aMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
        AssertComRC(rc);
        ComPtr<ISystemProperties> pSystemProperties;
        if (pVirtualBox)
            pVirtualBox->COMGETTER(SystemProperties)(pSystemProperties.asOutParam());
        ChipsetType_T chipsetType = ChipsetType_PIIX3;
        aMachine->COMGETTER(ChipsetType)(&chipsetType);
        ULONG maxNetworkAdapters = 0;
        if (pSystemProperties)
            pSystemProperties->GetMaxNetworkAdapters(chipsetType, &maxNetworkAdapters);
        meAttachmentType.resize(maxNetworkAdapters);
        for (ULONG slot = 0; slot < maxNetworkAdapters; ++slot)
            meAttachmentType[slot] = NetworkAttachmentType_Null;

#ifdef VBOX_WITH_VRDE_AUDIO
        unconst(mAudioVRDE) = new AudioVRDE(this);
        AssertReturn(mAudioVRDE, E_FAIL);
#endif
#ifdef VBOX_WITH_AUDIO_VIDEOREC
        unconst(mAudioVideoRec) = new AudioVideoRec(this);
        AssertReturn(mAudioVideoRec, E_FAIL);
#endif
        FirmwareType_T enmFirmwareType;
        mMachine->COMGETTER(FirmwareType)(&enmFirmwareType);
        if (   enmFirmwareType == FirmwareType_EFI
            || enmFirmwareType == FirmwareType_EFI32
            || enmFirmwareType == FirmwareType_EFI64
            || enmFirmwareType == FirmwareType_EFIDUAL)
        {
            unconst(mNvram) = new Nvram(this);
            AssertReturn(mNvram, E_FAIL);
        }

#ifdef VBOX_WITH_USB_CARDREADER
        unconst(mUsbCardReader) = new UsbCardReader(this);
        AssertReturn(mUsbCardReader, E_FAIL);
#endif

        m_cDisksPwProvided = 0;
        m_cDisksEncrypted = 0;

        unconst(m_pKeyStore) = new SecretKeyStore(true /* fKeyBufNonPageable */);
        AssertReturn(m_pKeyStore, E_FAIL);

        /* VirtualBox events registration. */
        {
            ComPtr<IEventSource> pES;
            rc = pVirtualBox->COMGETTER(EventSource)(pES.asOutParam());
            AssertComRC(rc);
            ComObjPtr<VmEventListenerImpl> aVmListener;
            aVmListener.createObject();
            aVmListener->init(new VmEventListener(), this);
            mVmListener = aVmListener;
            com::SafeArray<VBoxEventType_T> eventTypes;
            eventTypes.push_back(VBoxEventType_OnNATRedirect);
            eventTypes.push_back(VBoxEventType_OnHostNameResolutionConfigurationChange);
            eventTypes.push_back(VBoxEventType_OnHostPCIDevicePlug);
            eventTypes.push_back(VBoxEventType_OnExtraDataChanged);
            rc = pES->RegisterListener(aVmListener, ComSafeArrayAsInParam(eventTypes), true);
            AssertComRC(rc);
        }
    }

    /* Confirm a successful initialization when it's the case */
    autoInitSpan.setSucceeded();

#ifdef VBOX_WITH_EXTPACK
    /* Let the extension packs have a go at things (hold no locks). */
    if (SUCCEEDED(rc))
        mptrExtPackManager->i_callAllConsoleReadyHooks(this);
#endif

    LogFlowThisFuncLeave();

    return S_OK;
}

/**
 * Uninitializes the Console object.
 */
void Console::uninit()
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
    {
        LogFlowThisFunc(("Already uninitialized.\n"));
        LogFlowThisFuncLeave();
        return;
    }

    LogFlowThisFunc(("initFailed()=%d\n", autoUninitSpan.initFailed()));
    if (mVmListener)
    {
        ComPtr<IEventSource> pES;
        ComPtr<IVirtualBox> pVirtualBox;
        HRESULT rc = mMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
        AssertComRC(rc);
        if (SUCCEEDED(rc) && !pVirtualBox.isNull())
        {
            rc = pVirtualBox->COMGETTER(EventSource)(pES.asOutParam());
            AssertComRC(rc);
            if (!pES.isNull())
            {
                rc = pES->UnregisterListener(mVmListener);
                AssertComRC(rc);
            }
        }
        mVmListener.setNull();
    }

    /* power down the VM if necessary */
    if (mpUVM)
    {
        i_powerDown();
        Assert(mpUVM == NULL);
    }

    if (mVMZeroCallersSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(mVMZeroCallersSem);
        mVMZeroCallersSem = NIL_RTSEMEVENT;
    }

    if (mpVmm2UserMethods)
    {
        RTMemFree((void *)mpVmm2UserMethods);
        mpVmm2UserMethods = NULL;
    }

    if (mpIfSecKey)
    {
        RTMemFree((void *)mpIfSecKey);
        mpIfSecKey = NULL;
    }

    if (mpIfSecKeyHlp)
    {
        RTMemFree((void *)mpIfSecKeyHlp);
        mpIfSecKeyHlp = NULL;
    }

    if (mNvram)
    {
        delete mNvram;
        unconst(mNvram) = NULL;
    }

#ifdef VBOX_WITH_USB_CARDREADER
    if (mUsbCardReader)
    {
        delete mUsbCardReader;
        unconst(mUsbCardReader) = NULL;
    }
#endif

#ifdef VBOX_WITH_VRDE_AUDIO
    if (mAudioVRDE)
    {
        delete mAudioVRDE;
        unconst(mAudioVRDE) = NULL;
    }
#endif

#ifdef VBOX_WITH_AUDIO_VIDEOREC
    if (mAudioVideoRec)
    {
        delete mAudioVideoRec;
        unconst(mAudioVideoRec) = NULL;
    }
#endif

    // if the VM had a VMMDev with an HGCM thread, then remove that here
    if (m_pVMMDev)
    {
        delete m_pVMMDev;
        unconst(m_pVMMDev) = NULL;
    }

    if (mBusMgr)
    {
        mBusMgr->Release();
        mBusMgr = NULL;
    }

    if (m_pKeyStore)
    {
        delete m_pKeyStore;
        unconst(m_pKeyStore) = NULL;
    }

    m_mapGlobalSharedFolders.clear();
    m_mapMachineSharedFolders.clear();
    m_mapSharedFolders.clear();             // console instances

    mRemoteUSBDevices.clear();
    mUSBDevices.clear();

    if (mVRDEServerInfo)
    {
        mVRDEServerInfo->uninit();
        unconst(mVRDEServerInfo).setNull();
    }

    if (mEmulatedUSB)
    {
        mEmulatedUSB->uninit();
        unconst(mEmulatedUSB).setNull();
    }

    if (mDebugger)
    {
        mDebugger->uninit();
        unconst(mDebugger).setNull();
    }

    if (mDisplay)
    {
        mDisplay->uninit();
        unconst(mDisplay).setNull();
    }

    if (mMouse)
    {
        mMouse->uninit();
        unconst(mMouse).setNull();
    }

    if (mKeyboard)
    {
        mKeyboard->uninit();
        unconst(mKeyboard).setNull();
    }

    if (mGuest)
    {
        mGuest->uninit();
        unconst(mGuest).setNull();
    }

    if (mConsoleVRDPServer)
    {
        delete mConsoleVRDPServer;
        unconst(mConsoleVRDPServer) = NULL;
    }

    unconst(mVRDEServer).setNull();

    unconst(mControl).setNull();
    unconst(mMachine).setNull();

    // we don't perform uninit() as it's possible that some pending event refers to this source
    unconst(mEventSource).setNull();

#ifdef VBOX_WITH_EXTPACK
    unconst(mptrExtPackManager).setNull();
#endif

    LogFlowThisFuncLeave();
}

#ifdef VBOX_WITH_GUEST_PROPS

/**
 * Handles guest properties on a VM reset.
 *
 * We must delete properties that are flagged TRANSRESET.
 *
 * @todo r=bird: Would be more efficient if we added a request to the HGCM
 *       service to do this instead of detouring thru VBoxSVC.
 *       (IMachine::SetGuestProperty ends up in VBoxSVC, which in turns calls
 *       back into the VM process and the HGCM service.)
 */
void Console::i_guestPropertiesHandleVMReset(void)
{
    std::vector<Utf8Str> names;
    std::vector<Utf8Str> values;
    std::vector<LONG64>  timestamps;
    std::vector<Utf8Str> flags;
    HRESULT hrc = i_enumerateGuestProperties("*", names, values, timestamps, flags);
    if (SUCCEEDED(hrc))
    {
        for (size_t i = 0; i < flags.size(); i++)
        {
            /* Delete all properties which have the flag "TRANSRESET". */
            if (flags[i].contains("TRANSRESET", Utf8Str::CaseInsensitive))
            {
                hrc = mMachine->DeleteGuestProperty(Bstr(names[i]).raw());
                if (FAILED(hrc))
                    LogRel(("RESET: Could not delete transient property \"%s\", rc=%Rhrc\n",
                            names[i].c_str(), hrc));
            }
        }
    }
    else
        LogRel(("RESET: Unable to enumerate guest properties, rc=%Rhrc\n", hrc));
}

bool Console::i_guestPropertiesVRDPEnabled(void)
{
    Bstr value;
    HRESULT hrc = mMachine->GetExtraData(Bstr("VBoxInternal2/EnableGuestPropertiesVRDP").raw(),
                                         value.asOutParam());
    if (   hrc   == S_OK
        && value == "1")
        return true;
    return false;
}

void Console::i_guestPropertiesVRDPUpdateLogon(uint32_t u32ClientId, const char *pszUser, const char *pszDomain)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    char szPropNm[256];
    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Name", u32ClientId);
    Bstr clientName;
    mVRDEServerInfo->COMGETTER(ClientName)(clientName.asOutParam());

    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               clientName.raw(),
                               bstrReadOnlyGuest.raw());

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/User", u32ClientId);
    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               Bstr(pszUser).raw(),
                               bstrReadOnlyGuest.raw());

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Domain", u32ClientId);
    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               Bstr(pszDomain).raw(),
                               bstrReadOnlyGuest.raw());

    char szClientId[64];
    RTStrPrintf(szClientId, sizeof(szClientId), "%u", u32ClientId);
    mMachine->SetGuestProperty(Bstr("/VirtualBox/HostInfo/VRDP/LastConnectedClient").raw(),
                               Bstr(szClientId).raw(),
                               bstrReadOnlyGuest.raw());

    return;
}

void Console::i_guestPropertiesVRDPUpdateActiveClient(uint32_t u32ClientId)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("%d\n", u32ClientId));

    Bstr bstrFlags(L"RDONLYGUEST,TRANSIENT");

    char szClientId[64];
    RTStrPrintf(szClientId, sizeof(szClientId), "%u", u32ClientId);

    mMachine->SetGuestProperty(Bstr("/VirtualBox/HostInfo/VRDP/ActiveClient").raw(),
                               Bstr(szClientId).raw(),
                               bstrFlags.raw());

    return;
}

void Console::i_guestPropertiesVRDPUpdateNameChange(uint32_t u32ClientId, const char *pszName)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    char szPropNm[256];
    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Name", u32ClientId);
    Bstr clientName(pszName);

    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               clientName.raw(),
                               bstrReadOnlyGuest.raw());

}

void Console::i_guestPropertiesVRDPUpdateIPAddrChange(uint32_t u32ClientId, const char *pszIPAddr)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    char szPropNm[256];
    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/IPAddr", u32ClientId);
    Bstr clientIPAddr(pszIPAddr);

    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               clientIPAddr.raw(),
                               bstrReadOnlyGuest.raw());

}

void Console::i_guestPropertiesVRDPUpdateLocationChange(uint32_t u32ClientId, const char *pszLocation)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    char szPropNm[256];
    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Location", u32ClientId);
    Bstr clientLocation(pszLocation);

    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               clientLocation.raw(),
                               bstrReadOnlyGuest.raw());

}

void Console::i_guestPropertiesVRDPUpdateOtherInfoChange(uint32_t u32ClientId, const char *pszOtherInfo)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    char szPropNm[256];
    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/OtherInfo", u32ClientId);
    Bstr clientOtherInfo(pszOtherInfo);

    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               clientOtherInfo.raw(),
                               bstrReadOnlyGuest.raw());

}

void Console::i_guestPropertiesVRDPUpdateClientAttach(uint32_t u32ClientId, bool fAttached)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    char szPropNm[256];
    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Attach", u32ClientId);

    Bstr bstrValue = fAttached? "1": "0";

    mMachine->SetGuestProperty(Bstr(szPropNm).raw(),
                               bstrValue.raw(),
                               bstrReadOnlyGuest.raw());
}

void Console::i_guestPropertiesVRDPUpdateDisconnect(uint32_t u32ClientId)
{
    if (!i_guestPropertiesVRDPEnabled())
        return;

    LogFlowFunc(("\n"));

    Bstr bstrReadOnlyGuest(L"RDONLYGUEST");

    char szPropNm[256];
    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Name", u32ClientId);
    mMachine->SetGuestProperty(Bstr(szPropNm).raw(), NULL,
                               bstrReadOnlyGuest.raw());

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/User", u32ClientId);
    mMachine->SetGuestProperty(Bstr(szPropNm).raw(), NULL,
                               bstrReadOnlyGuest.raw());

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Domain", u32ClientId);
    mMachine->SetGuestProperty(Bstr(szPropNm).raw(), NULL,
                               bstrReadOnlyGuest.raw());

    RTStrPrintf(szPropNm, sizeof(szPropNm), "/VirtualBox/HostInfo/VRDP/Client/%u/Attach", u32ClientId);
    mMachine->SetGuestProperty(Bstr(szPropNm).raw(), NULL,
                               bstrReadOnlyGuest.raw());

    char szClientId[64];
    RTStrPrintf(szClientId, sizeof(szClientId), "%d", u32ClientId);
    mMachine->SetGuestProperty(Bstr("/VirtualBox/HostInfo/VRDP/LastDisconnectedClient").raw(),
                               Bstr(szClientId).raw(),
                               bstrReadOnlyGuest.raw());

    return;
}

#endif /* VBOX_WITH_GUEST_PROPS */

bool Console::i_isResetTurnedIntoPowerOff(void)
{
    Bstr value;
    HRESULT hrc = mMachine->GetExtraData(Bstr("VBoxInternal2/TurnResetIntoPowerOff").raw(),
                                         value.asOutParam());
    if (   hrc   == S_OK
        && value == "1")
        return true;
    return false;
}

#ifdef VBOX_WITH_EXTPACK
/**
 * Used by VRDEServer and others to talke to the extension pack manager.
 *
 * @returns The extension pack manager.
 */
ExtPackManager *Console::i_getExtPackManager()
{
    return mptrExtPackManager;
}
#endif


int Console::i_VRDPClientLogon(uint32_t u32ClientId, const char *pszUser, const char *pszPassword, const char *pszDomain)
{
    LogFlowFuncEnter();
    LogFlowFunc(("%d, %s, %s, %s\n", u32ClientId, pszUser, pszPassword, pszDomain));

    AutoCaller autoCaller(this);
    if (!autoCaller.isOk())
    {
        /* Console has been already uninitialized, deny request */
        LogRel(("AUTH: Access denied (Console uninitialized).\n"));
        LogFlowFuncLeave();
        return VERR_ACCESS_DENIED;
    }

    Guid uuid = Guid(i_getId());

    AuthType_T authType = AuthType_Null;
    HRESULT hrc = mVRDEServer->COMGETTER(AuthType)(&authType);
    AssertComRCReturn(hrc, VERR_ACCESS_DENIED);

    ULONG authTimeout = 0;
    hrc = mVRDEServer->COMGETTER(AuthTimeout)(&authTimeout);
    AssertComRCReturn(hrc, VERR_ACCESS_DENIED);

    AuthResult result = AuthResultAccessDenied;
    AuthGuestJudgement guestJudgement = AuthGuestNotAsked;

    LogFlowFunc(("Auth type %d\n", authType));

    LogRel(("AUTH: User: [%s]. Domain: [%s]. Authentication type: [%s]\n",
                pszUser, pszDomain,
                authType == AuthType_Null?
                    "Null":
                    (authType == AuthType_External?
                        "External":
                        (authType == AuthType_Guest?
                            "Guest":
                            "INVALID"
                        )
                    )
            ));

    switch (authType)
    {
        case AuthType_Null:
        {
            result = AuthResultAccessGranted;
            break;
        }

        case AuthType_External:
        {
            /* Call the external library. */
            result = mConsoleVRDPServer->Authenticate(uuid, guestJudgement, pszUser, pszPassword, pszDomain, u32ClientId);

            if (result != AuthResultDelegateToGuest)
            {
                break;
            }

            LogRel(("AUTH: Delegated to guest.\n"));

            LogFlowFunc(("External auth asked for guest judgement\n"));
        }
        RT_FALL_THRU();

        case AuthType_Guest:
        {
            guestJudgement = AuthGuestNotReacted;

            /** @todo r=dj locking required here for m_pVMMDev? */
            PPDMIVMMDEVPORT pDevPort;
            if (    (m_pVMMDev)
                 && ((pDevPort = m_pVMMDev->getVMMDevPort()))
               )
            {
                /* Issue the request to guest. Assume that the call does not require EMT. It should not. */

                /* Ask the guest to judge these credentials. */
                uint32_t u32GuestFlags = VMMDEV_SETCREDENTIALS_JUDGE;

                int rc = pDevPort->pfnSetCredentials(pDevPort, pszUser, pszPassword, pszDomain, u32GuestFlags);

                if (RT_SUCCESS(rc))
                {
                    /* Wait for guest. */
                    rc = m_pVMMDev->WaitCredentialsJudgement(authTimeout, &u32GuestFlags);

                    if (RT_SUCCESS(rc))
                    {
                        switch (u32GuestFlags & (VMMDEV_CREDENTIALS_JUDGE_OK | VMMDEV_CREDENTIALS_JUDGE_DENY |
                                                 VMMDEV_CREDENTIALS_JUDGE_NOJUDGEMENT))
                        {
                            case VMMDEV_CREDENTIALS_JUDGE_DENY:        guestJudgement = AuthGuestAccessDenied;  break;
                            case VMMDEV_CREDENTIALS_JUDGE_NOJUDGEMENT: guestJudgement = AuthGuestNoJudgement;   break;
                            case VMMDEV_CREDENTIALS_JUDGE_OK:          guestJudgement = AuthGuestAccessGranted; break;
                            default:
                                LogFlowFunc(("Invalid guest flags %08X!!!\n", u32GuestFlags)); break;
                        }
                    }
                    else
                    {
                        LogFlowFunc(("Wait for credentials judgement rc = %Rrc!!!\n", rc));
                    }

                    LogFlowFunc(("Guest judgement %d\n", guestJudgement));
                }
                else
                {
                    LogFlowFunc(("Could not set credentials rc = %Rrc!!!\n", rc));
                }
            }

            if (authType == AuthType_External)
            {
                LogRel(("AUTH: Guest judgement %d.\n", guestJudgement));
                LogFlowFunc(("External auth called again with guest judgement = %d\n", guestJudgement));
                result = mConsoleVRDPServer->Authenticate(uuid, guestJudgement, pszUser, pszPassword, pszDomain, u32ClientId);
            }
            else
            {
                switch (guestJudgement)
                {
                    case AuthGuestAccessGranted:
                        result = AuthResultAccessGranted;
                        break;
                    default:
                        result = AuthResultAccessDenied;
                        break;
                }
            }
        } break;

        default:
            AssertFailed();
    }

    LogFlowFunc(("Result = %d\n", result));
    LogFlowFuncLeave();

    if (result != AuthResultAccessGranted)
    {
        /* Reject. */
        LogRel(("AUTH: Access denied.\n"));
        return VERR_ACCESS_DENIED;
    }

    LogRel(("AUTH: Access granted.\n"));

    /* Multiconnection check must be made after authentication, so bad clients would not interfere with a good one. */
    BOOL allowMultiConnection = FALSE;
    hrc = mVRDEServer->COMGETTER(AllowMultiConnection)(&allowMultiConnection);
    AssertComRCReturn(hrc, VERR_ACCESS_DENIED);

    BOOL reuseSingleConnection = FALSE;
    hrc = mVRDEServer->COMGETTER(ReuseSingleConnection)(&reuseSingleConnection);
    AssertComRCReturn(hrc, VERR_ACCESS_DENIED);

    LogFlowFunc(("allowMultiConnection %d, reuseSingleConnection = %d, mcVRDPClients = %d, mu32SingleRDPClientId = %d\n",
                 allowMultiConnection, reuseSingleConnection, mcVRDPClients, mu32SingleRDPClientId));

    if (allowMultiConnection == FALSE)
    {
        /* Note: the 'mcVRDPClients' variable is incremented in ClientConnect callback, which is called when the client
         * is successfully connected, that is after the ClientLogon callback. Therefore the mcVRDPClients
         * value is 0 for first client.
         */
        if (mcVRDPClients != 0)
        {
            Assert(mcVRDPClients == 1);
            /* There is a client already.
             * If required drop the existing client connection and let the connecting one in.
             */
            if (reuseSingleConnection)
            {
                LogRel(("AUTH: Multiple connections are not enabled. Disconnecting existing client.\n"));
                mConsoleVRDPServer->DisconnectClient(mu32SingleRDPClientId, false);
            }
            else
            {
                /* Reject. */
                LogRel(("AUTH: Multiple connections are not enabled. Access denied.\n"));
                return VERR_ACCESS_DENIED;
            }
        }

        /* Save the connected client id. From now on it will be necessary to disconnect this one. */
        mu32SingleRDPClientId = u32ClientId;
    }

#ifdef VBOX_WITH_GUEST_PROPS
    i_guestPropertiesVRDPUpdateLogon(u32ClientId, pszUser, pszDomain);
#endif /* VBOX_WITH_GUEST_PROPS */

    /* Check if the successfully verified credentials are to be sent to the guest. */
    BOOL fProvideGuestCredentials = FALSE;

    Bstr value;
    hrc = mMachine->GetExtraData(Bstr("VRDP/ProvideGuestCredentials").raw(),
                                 value.asOutParam());
    if (SUCCEEDED(hrc) && value == "1")
    {
        /* Provide credentials only if there are no logged in users. */
        Utf8Str noLoggedInUsersValue;
        LONG64 ul64Timestamp = 0;
        Utf8Str flags;

        hrc = i_getGuestProperty("/VirtualBox/GuestInfo/OS/NoLoggedInUsers",
                                 &noLoggedInUsersValue, &ul64Timestamp, &flags);

        if (SUCCEEDED(hrc) && noLoggedInUsersValue != "false")
        {
            /* And only if there are no connected clients. */
            if (ASMAtomicCmpXchgBool(&mcGuestCredentialsProvided, true, false))
            {
                fProvideGuestCredentials = TRUE;
            }
        }
    }

    /** @todo r=dj locking required here for m_pVMMDev? */
    if (   fProvideGuestCredentials
        && m_pVMMDev)
    {
        uint32_t u32GuestFlags = VMMDEV_SETCREDENTIALS_GUESTLOGON;

        PPDMIVMMDEVPORT pDevPort = m_pVMMDev->getVMMDevPort();
        if (pDevPort)
        {
            int rc = pDevPort->pfnSetCredentials(m_pVMMDev->getVMMDevPort(),
                                                 pszUser, pszPassword, pszDomain, u32GuestFlags);
            AssertRC(rc);
        }
    }

    return VINF_SUCCESS;
}

void Console::i_VRDPClientStatusChange(uint32_t u32ClientId, const char *pszStatus)
{
    LogFlowFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    LogFlowFunc(("%s\n", pszStatus));

#ifdef VBOX_WITH_GUEST_PROPS
    /* Parse the status string. */
    if (RTStrICmp(pszStatus, "ATTACH") == 0)
    {
        i_guestPropertiesVRDPUpdateClientAttach(u32ClientId, true);
    }
    else if (RTStrICmp(pszStatus, "DETACH") == 0)
    {
        i_guestPropertiesVRDPUpdateClientAttach(u32ClientId, false);
    }
    else if (RTStrNICmp(pszStatus, "NAME=", strlen("NAME=")) == 0)
    {
        i_guestPropertiesVRDPUpdateNameChange(u32ClientId, pszStatus + strlen("NAME="));
    }
    else if (RTStrNICmp(pszStatus, "CIPA=", strlen("CIPA=")) == 0)
    {
        i_guestPropertiesVRDPUpdateIPAddrChange(u32ClientId, pszStatus + strlen("CIPA="));
    }
    else if (RTStrNICmp(pszStatus, "CLOCATION=", strlen("CLOCATION=")) == 0)
    {
        i_guestPropertiesVRDPUpdateLocationChange(u32ClientId, pszStatus + strlen("CLOCATION="));
    }
    else if (RTStrNICmp(pszStatus, "COINFO=", strlen("COINFO=")) == 0)
    {
        i_guestPropertiesVRDPUpdateOtherInfoChange(u32ClientId, pszStatus + strlen("COINFO="));
    }
#endif

    LogFlowFuncLeave();
}

void Console::i_VRDPClientConnect(uint32_t u32ClientId)
{
    LogFlowFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    uint32_t u32Clients = ASMAtomicIncU32(&mcVRDPClients);
    VMMDev *pDev;
    PPDMIVMMDEVPORT pPort;
    if (    (u32Clients == 1)
         && ((pDev = i_getVMMDev()))
         && ((pPort = pDev->getVMMDevPort()))
       )
    {
        pPort->pfnVRDPChange(pPort,
                             true,
                             VRDP_EXPERIENCE_LEVEL_FULL); /** @todo configurable */
    }

    NOREF(u32ClientId);
    mDisplay->i_VideoAccelVRDP(true);

#ifdef VBOX_WITH_GUEST_PROPS
    i_guestPropertiesVRDPUpdateActiveClient(u32ClientId);
#endif /* VBOX_WITH_GUEST_PROPS */

    LogFlowFuncLeave();
    return;
}

void Console::i_VRDPClientDisconnect(uint32_t u32ClientId,
                                     uint32_t fu32Intercepted)
{
    LogFlowFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AssertReturnVoid(mConsoleVRDPServer);

    uint32_t u32Clients = ASMAtomicDecU32(&mcVRDPClients);
    VMMDev *pDev;
    PPDMIVMMDEVPORT pPort;

    if (    (u32Clients == 0)
         && ((pDev = i_getVMMDev()))
         && ((pPort = pDev->getVMMDevPort()))
       )
    {
        pPort->pfnVRDPChange(pPort,
                             false,
                             0);
    }

    mDisplay->i_VideoAccelVRDP(false);

    if (fu32Intercepted & VRDE_CLIENT_INTERCEPT_USB)
    {
        mConsoleVRDPServer->USBBackendDelete(u32ClientId);
    }

    if (fu32Intercepted & VRDE_CLIENT_INTERCEPT_CLIPBOARD)
    {
        mConsoleVRDPServer->ClipboardDelete(u32ClientId);
    }

#ifdef VBOX_WITH_VRDE_AUDIO
    if (fu32Intercepted & VRDE_CLIENT_INTERCEPT_AUDIO)
    {
        if (mAudioVRDE)
            mAudioVRDE->onVRDEControl(false /* fEnable */, 0 /* uFlags */);
    }
#endif

    AuthType_T authType = AuthType_Null;
    HRESULT hrc = mVRDEServer->COMGETTER(AuthType)(&authType);
    AssertComRC(hrc);

    if (authType == AuthType_External)
        mConsoleVRDPServer->AuthDisconnect(i_getId(), u32ClientId);

#ifdef VBOX_WITH_GUEST_PROPS
    i_guestPropertiesVRDPUpdateDisconnect(u32ClientId);
    if (u32Clients == 0)
        i_guestPropertiesVRDPUpdateActiveClient(0);
#endif /* VBOX_WITH_GUEST_PROPS */

    if (u32Clients == 0)
        mcGuestCredentialsProvided = false;

    LogFlowFuncLeave();
    return;
}

void Console::i_VRDPInterceptAudio(uint32_t u32ClientId)
{
    RT_NOREF(u32ClientId);
    LogFlowFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    LogFlowFunc(("u32ClientId=%RU32\n", u32ClientId));

#ifdef VBOX_WITH_VRDE_AUDIO
    if (mAudioVRDE)
        mAudioVRDE->onVRDEControl(true /* fEnable */, 0 /* uFlags */);
#endif

    LogFlowFuncLeave();
    return;
}

void Console::i_VRDPInterceptUSB(uint32_t u32ClientId, void **ppvIntercept)
{
    LogFlowFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AssertReturnVoid(mConsoleVRDPServer);

    mConsoleVRDPServer->USBBackendCreate(u32ClientId, ppvIntercept);

    LogFlowFuncLeave();
    return;
}

void Console::i_VRDPInterceptClipboard(uint32_t u32ClientId)
{
    LogFlowFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AssertReturnVoid(mConsoleVRDPServer);

    mConsoleVRDPServer->ClipboardCreate(u32ClientId);

    LogFlowFuncLeave();
    return;
}


//static
const char *Console::sSSMConsoleUnit = "ConsoleData";
//static
uint32_t Console::sSSMConsoleVer = 0x00010001;

inline static const char *networkAdapterTypeToName(NetworkAdapterType_T adapterType)
{
    switch (adapterType)
    {
        case NetworkAdapterType_Am79C970A:
        case NetworkAdapterType_Am79C973:
            return "pcnet";
#ifdef VBOX_WITH_E1000
        case NetworkAdapterType_I82540EM:
        case NetworkAdapterType_I82543GC:
        case NetworkAdapterType_I82545EM:
            return "e1000";
#endif
#ifdef VBOX_WITH_VIRTIO
        case NetworkAdapterType_Virtio:
            return "virtio-net";
#endif
        default:
            AssertFailed();
            return "unknown";
    }
    /* not reached */
}

/**
 * Loads various console data stored in the saved state file.
 * This method does validation of the state file and returns an error info
 * when appropriate.
 *
 * The method does nothing if the machine is not in the Saved file or if
 * console data from it has already been loaded.
 *
 * @note The caller must lock this object for writing.
 */
HRESULT Console::i_loadDataFromSavedState()
{
    if (mMachineState != MachineState_Saved || mSavedStateDataLoaded)
        return S_OK;

    Bstr savedStateFile;
    HRESULT rc = mMachine->COMGETTER(StateFilePath)(savedStateFile.asOutParam());
    if (FAILED(rc))
        return rc;

    PSSMHANDLE ssm;
    int vrc = SSMR3Open(Utf8Str(savedStateFile).c_str(), 0, &ssm);
    if (RT_SUCCESS(vrc))
    {
        uint32_t version = 0;
        vrc = SSMR3Seek(ssm, sSSMConsoleUnit, 0 /* iInstance */, &version);
        if (SSM_VERSION_MAJOR(version) == SSM_VERSION_MAJOR(sSSMConsoleVer))
        {
            if (RT_SUCCESS(vrc))
                vrc = i_loadStateFileExecInternal(ssm, version);
            else if (vrc == VERR_SSM_UNIT_NOT_FOUND)
                vrc = VINF_SUCCESS;
        }
        else
            vrc = VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;

        SSMR3Close(ssm);
    }

    if (RT_FAILURE(vrc))
        rc = setError(VBOX_E_FILE_ERROR,
                      tr("The saved state file '%ls' is invalid (%Rrc). Delete the saved state and try again"),
                      savedStateFile.raw(), vrc);

    mSavedStateDataLoaded = true;

    return rc;
}

/**
 * Callback handler to save various console data to the state file,
 * called when the user saves the VM state.
 *
 * @param pSSM      SSM handle.
 * @param pvUser    pointer to Console
 *
 * @note Locks the Console object for reading.
 */
//static
DECLCALLBACK(void) Console::i_saveStateFileExec(PSSMHANDLE pSSM, void *pvUser)
{
    LogFlowFunc(("\n"));

    Console *that = static_cast<Console *>(pvUser);
    AssertReturnVoid(that);

    AutoCaller autoCaller(that);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoReadLock alock(that COMMA_LOCKVAL_SRC_POS);

    int vrc = SSMR3PutU32(pSSM, (uint32_t)that->m_mapSharedFolders.size());
    AssertRC(vrc);

    for (SharedFolderMap::const_iterator it = that->m_mapSharedFolders.begin();
         it != that->m_mapSharedFolders.end();
         ++it)
    {
        SharedFolder *pSF = (*it).second;
        AutoCaller sfCaller(pSF);
        AutoReadLock sfLock(pSF COMMA_LOCKVAL_SRC_POS);

        Utf8Str name = pSF->i_getName();
        vrc = SSMR3PutU32(pSSM, (uint32_t)name.length() + 1 /* term. 0 */);
        AssertRC(vrc);
        vrc = SSMR3PutStrZ(pSSM, name.c_str());
        AssertRC(vrc);

        Utf8Str hostPath = pSF->i_getHostPath();
        vrc = SSMR3PutU32(pSSM, (uint32_t)hostPath.length() + 1 /* term. 0 */);
        AssertRC(vrc);
        vrc = SSMR3PutStrZ(pSSM, hostPath.c_str());
        AssertRC(vrc);

        vrc = SSMR3PutBool(pSSM, !!pSF->i_isWritable());
        AssertRC(vrc);

        vrc = SSMR3PutBool(pSSM, !!pSF->i_isAutoMounted());
        AssertRC(vrc);
    }

    return;
}

/**
 * Callback handler to load various console data from the state file.
 * Called when the VM is being restored from the saved state.
 *
 * @param pSSM         SSM handle.
 * @param pvUser       pointer to Console
 * @param uVersion     Console unit version.
 *                     Should match sSSMConsoleVer.
 * @param uPass        The data pass.
 *
 * @note Should locks the Console object for writing, if necessary.
 */
//static
DECLCALLBACK(int)
Console::i_loadStateFileExec(PSSMHANDLE pSSM, void *pvUser, uint32_t uVersion, uint32_t uPass)
{
    LogFlowFunc(("\n"));

    if (SSM_VERSION_MAJOR_CHANGED(uVersion, sSSMConsoleVer))
        return VERR_VERSION_MISMATCH;
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    Console *that = static_cast<Console *>(pvUser);
    AssertReturn(that, VERR_INVALID_PARAMETER);

    /* Currently, nothing to do when we've been called from VMR3Load*. */
    return SSMR3SkipToEndOfUnit(pSSM);
}

/**
 * Method to load various console data from the state file.
 * Called from #i_loadDataFromSavedState.
 *
 * @param pSSM         SSM handle.
 * @param u32Version   Console unit version.
 *                     Should match sSSMConsoleVer.
 *
 * @note Locks the Console object for writing.
 */
int Console::i_loadStateFileExecInternal(PSSMHANDLE pSSM, uint32_t u32Version)
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), VERR_ACCESS_DENIED);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(m_mapSharedFolders.size() == 0, VERR_INTERNAL_ERROR);

    uint32_t size = 0;
    int vrc = SSMR3GetU32(pSSM, &size);
    AssertRCReturn(vrc, vrc);

    for (uint32_t i = 0; i < size; ++i)
    {
        Utf8Str strName;
        Utf8Str strHostPath;
        bool writable = true;
        bool autoMount = false;

        uint32_t szBuf = 0;
        char *buf = NULL;

        vrc = SSMR3GetU32(pSSM, &szBuf);
        AssertRCReturn(vrc, vrc);
        buf = new char[szBuf];
        vrc = SSMR3GetStrZ(pSSM, buf, szBuf);
        AssertRC(vrc);
        strName = buf;
        delete[] buf;

        vrc = SSMR3GetU32(pSSM, &szBuf);
        AssertRCReturn(vrc, vrc);
        buf = new char[szBuf];
        vrc = SSMR3GetStrZ(pSSM, buf, szBuf);
        AssertRC(vrc);
        strHostPath = buf;
        delete[] buf;

        if (u32Version > 0x00010000)
            SSMR3GetBool(pSSM, &writable);

        if (u32Version > 0x00010000) // ???
            SSMR3GetBool(pSSM, &autoMount);

        ComObjPtr<SharedFolder> pSharedFolder;
        pSharedFolder.createObject();
        HRESULT rc = pSharedFolder->init(this,
                                         strName,
                                         strHostPath,
                                         writable,
                                         autoMount,
                                         false /* fFailOnError */);
        AssertComRCReturn(rc, VERR_INTERNAL_ERROR);

        m_mapSharedFolders.insert(std::make_pair(strName, pSharedFolder));
    }

    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_GUEST_PROPS

// static
DECLCALLBACK(int) Console::i_doGuestPropNotification(void *pvExtension,
                                                     uint32_t u32Function,
                                                     void *pvParms,
                                                     uint32_t cbParms)
{
    using namespace guestProp;

    Assert(u32Function == 0); NOREF(u32Function);

    /*
     * No locking, as this is purely a notification which does not make any
     * changes to the object state.
     */
    PHOSTCALLBACKDATA   pCBData = reinterpret_cast<PHOSTCALLBACKDATA>(pvParms);
    AssertReturn(sizeof(HOSTCALLBACKDATA) == cbParms, VERR_INVALID_PARAMETER);
    AssertReturn(HOSTCALLBACKMAGIC == pCBData->u32Magic, VERR_INVALID_PARAMETER);
    LogFlow(("Console::doGuestPropNotification: pCBData={.pcszName=%s, .pcszValue=%s, .pcszFlags=%s}\n",
             pCBData->pcszName, pCBData->pcszValue, pCBData->pcszFlags));

    int  rc;
    Bstr name(pCBData->pcszName);
    Bstr value(pCBData->pcszValue);
    Bstr flags(pCBData->pcszFlags);
    ComObjPtr<Console> pConsole = reinterpret_cast<Console *>(pvExtension);
    HRESULT hrc = pConsole->mControl->PushGuestProperty(name.raw(),
                                                        value.raw(),
                                                        pCBData->u64Timestamp,
                                                        flags.raw());
    if (SUCCEEDED(hrc))
    {
        fireGuestPropertyChangedEvent(pConsole->mEventSource, pConsole->i_getId().raw(), name.raw(), value.raw(), flags.raw());
        rc = VINF_SUCCESS;
    }
    else
    {
        LogFlow(("Console::doGuestPropNotification: hrc=%Rhrc pCBData={.pcszName=%s, .pcszValue=%s, .pcszFlags=%s}\n",
                 hrc, pCBData->pcszName, pCBData->pcszValue, pCBData->pcszFlags));
        rc = Global::vboxStatusCodeFromCOM(hrc);
    }
    return rc;
}

HRESULT Console::i_doEnumerateGuestProperties(const Utf8Str &aPatterns,
                                              std::vector<Utf8Str> &aNames,
                                              std::vector<Utf8Str> &aValues,
                                              std::vector<LONG64>  &aTimestamps,
                                              std::vector<Utf8Str> &aFlags)
{
    AssertReturn(m_pVMMDev, E_FAIL);

    using namespace guestProp;

    VBOXHGCMSVCPARM parm[3];

    parm[0].type = VBOX_HGCM_SVC_PARM_PTR;
    parm[0].u.pointer.addr = (void*)aPatterns.c_str();
    parm[0].u.pointer.size = (uint32_t)aPatterns.length() + 1;

    /*
     * Now things get slightly complicated. Due to a race with the guest adding
     * properties, there is no good way to know how much to enlarge a buffer for
     * the service to enumerate into. We choose a decent starting size and loop a
     * few times, each time retrying with the size suggested by the service plus
     * one Kb.
     */
    size_t cchBuf = 4096;
    Utf8Str Utf8Buf;
    int vrc = VERR_BUFFER_OVERFLOW;
    for (unsigned i = 0; i < 10 && (VERR_BUFFER_OVERFLOW == vrc); ++i)
    {
        try
        {
            Utf8Buf.reserve(cchBuf + 1024);
        }
        catch(...)
        {
            return E_OUTOFMEMORY;
        }

        parm[1].type = VBOX_HGCM_SVC_PARM_PTR;
        parm[1].u.pointer.addr = Utf8Buf.mutableRaw();
        parm[1].u.pointer.size = (uint32_t)cchBuf + 1024;

        parm[2].type = VBOX_HGCM_SVC_PARM_32BIT;
        parm[2].u.uint32 = 0;

        vrc = m_pVMMDev->hgcmHostCall("VBoxGuestPropSvc", ENUM_PROPS_HOST, 3,
                                      &parm[0]);
        Utf8Buf.jolt();
        if (parm[2].type != VBOX_HGCM_SVC_PARM_32BIT)
            return setError(E_FAIL, tr("Internal application error"));
        cchBuf = parm[2].u.uint32;
    }
    if (VERR_BUFFER_OVERFLOW == vrc)
        return setError(E_UNEXPECTED,
                        tr("Temporary failure due to guest activity, please retry"));

    /*
     * Finally we have to unpack the data returned by the service into the safe
     * arrays supplied by the caller. We start by counting the number of entries.
     */
    const char *pszBuf
        = reinterpret_cast<const char *>(parm[1].u.pointer.addr);
    unsigned cEntries = 0;
    /* The list is terminated by a zero-length string at the end of a set
     * of four strings. */
    for (size_t i = 0; strlen(pszBuf + i) != 0; )
    {
       /* We are counting sets of four strings. */
       for (unsigned j = 0; j < 4; ++j)
           i += strlen(pszBuf + i) + 1;
       ++cEntries;
    }

    aNames.resize(cEntries);
    aValues.resize(cEntries);
    aTimestamps.resize(cEntries);
    aFlags.resize(cEntries);

    size_t iBuf = 0;
    /* Rely on the service to have formated the data correctly. */
    for (unsigned i = 0; i < cEntries; ++i)
    {
        size_t cchName = strlen(pszBuf + iBuf);
        aNames[i] = &pszBuf[iBuf];
        iBuf += cchName + 1;

        size_t cchValue = strlen(pszBuf + iBuf);
        aValues[i] = &pszBuf[iBuf];
        iBuf += cchValue + 1;

        size_t cchTimestamp = strlen(pszBuf + iBuf);
        aTimestamps[i] = RTStrToUInt64(&pszBuf[iBuf]);
        iBuf += cchTimestamp + 1;

        size_t cchFlags = strlen(pszBuf + iBuf);
        aFlags[i] = &pszBuf[iBuf];
        iBuf += cchFlags + 1;
    }

    return S_OK;
}

#endif /* VBOX_WITH_GUEST_PROPS */


// IConsole properties
/////////////////////////////////////////////////////////////////////////////
HRESULT Console::getMachine(ComPtr<IMachine> &aMachine)
{
    /* mMachine is constant during life time, no need to lock */
    mMachine.queryInterfaceTo(aMachine.asOutParam());

    /* callers expect to get a valid reference, better fail than crash them */
    if (mMachine.isNull())
        return E_FAIL;

    return S_OK;
}

HRESULT Console::getState(MachineState_T *aState)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* we return our local state (since it's always the same as on the server) */
    *aState = mMachineState;

    return S_OK;
}

HRESULT Console::getGuest(ComPtr<IGuest> &aGuest)
{
    /* mGuest is constant during life time, no need to lock */
    mGuest.queryInterfaceTo(aGuest.asOutParam());

    return S_OK;
}

HRESULT Console::getKeyboard(ComPtr<IKeyboard> &aKeyboard)
{
    /* mKeyboard is constant during life time, no need to lock */
    mKeyboard.queryInterfaceTo(aKeyboard.asOutParam());

    return S_OK;
}

HRESULT Console::getMouse(ComPtr<IMouse> &aMouse)
{
    /* mMouse is constant during life time, no need to lock */
    mMouse.queryInterfaceTo(aMouse.asOutParam());

    return S_OK;
}

HRESULT Console::getDisplay(ComPtr<IDisplay> &aDisplay)
{
    /* mDisplay is constant during life time, no need to lock */
    mDisplay.queryInterfaceTo(aDisplay.asOutParam());

    return S_OK;
}

HRESULT Console::getDebugger(ComPtr<IMachineDebugger> &aDebugger)
{
    /* we need a write lock because of the lazy mDebugger initialization*/
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* check if we have to create the debugger object */
    if (!mDebugger)
    {
        unconst(mDebugger).createObject();
        mDebugger->init(this);
    }

    mDebugger.queryInterfaceTo(aDebugger.asOutParam());

    return S_OK;
}

HRESULT Console::getUSBDevices(std::vector<ComPtr<IUSBDevice> > &aUSBDevices)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    size_t i = 0;
    aUSBDevices.resize(mUSBDevices.size());
    for (USBDeviceList::const_iterator it = mUSBDevices.begin(); it != mUSBDevices.end(); ++i, ++it)
        (*it).queryInterfaceTo(aUSBDevices[i].asOutParam());

    return S_OK;
}


HRESULT Console::getRemoteUSBDevices(std::vector<ComPtr<IHostUSBDevice> > &aRemoteUSBDevices)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    size_t i = 0;
    aRemoteUSBDevices.resize(mRemoteUSBDevices.size());
    for (RemoteUSBDeviceList::const_iterator it = mRemoteUSBDevices.begin(); it != mRemoteUSBDevices.end(); ++i, ++it)
        (*it).queryInterfaceTo(aRemoteUSBDevices[i].asOutParam());

    return S_OK;
}

HRESULT Console::getVRDEServerInfo(ComPtr<IVRDEServerInfo> &aVRDEServerInfo)
{
    /* mVRDEServerInfo is constant during life time, no need to lock */
    mVRDEServerInfo.queryInterfaceTo(aVRDEServerInfo.asOutParam());

    return S_OK;
}

HRESULT Console::getEmulatedUSB(ComPtr<IEmulatedUSB> &aEmulatedUSB)
{
    /* mEmulatedUSB is constant during life time, no need to lock */
    mEmulatedUSB.queryInterfaceTo(aEmulatedUSB.asOutParam());

    return S_OK;
}

HRESULT Console::getSharedFolders(std::vector<ComPtr<ISharedFolder> > &aSharedFolders)
{
    /* loadDataFromSavedState() needs a write lock */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Read console data stored in the saved state file (if not yet done) */
    HRESULT rc = i_loadDataFromSavedState();
    if (FAILED(rc)) return rc;

    size_t i = 0;
    aSharedFolders.resize(m_mapSharedFolders.size());
    for (SharedFolderMap::const_iterator it = m_mapSharedFolders.begin(); it != m_mapSharedFolders.end(); ++i, ++it)
        (it)->second.queryInterfaceTo(aSharedFolders[i].asOutParam());

    return S_OK;
}

HRESULT Console::getEventSource(ComPtr<IEventSource> &aEventSource)
{
    // no need to lock - lifetime constant
    mEventSource.queryInterfaceTo(aEventSource.asOutParam());

    return S_OK;
}

HRESULT Console::getAttachedPCIDevices(std::vector<ComPtr<IPCIDeviceAttachment> > &aAttachedPCIDevices)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mBusMgr)
    {
        std::vector<BusAssignmentManager::PCIDeviceInfo> devInfos;
        mBusMgr->listAttachedPCIDevices(devInfos);
        ComObjPtr<PCIDeviceAttachment> dev;
        aAttachedPCIDevices.resize(devInfos.size());
        for (size_t i = 0; i < devInfos.size(); i++)
        {
            const BusAssignmentManager::PCIDeviceInfo &devInfo = devInfos[i];
            dev.createObject();
            dev->init(NULL, devInfo.strDeviceName,
                      devInfo.hostAddress.valid() ? devInfo.hostAddress.asLong() : -1,
                      devInfo.guestAddress.asLong(),
                      devInfo.hostAddress.valid());
            dev.queryInterfaceTo(aAttachedPCIDevices[i].asOutParam());
        }
    }
    else
        aAttachedPCIDevices.resize(0);

    return S_OK;
}

HRESULT Console::getUseHostClipboard(BOOL *aUseHostClipboard)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aUseHostClipboard = mfUseHostClipboard;

    return S_OK;
}

HRESULT Console::setUseHostClipboard(BOOL aUseHostClipboard)
{
    mfUseHostClipboard = !!aUseHostClipboard;

    return S_OK;
}

// IConsole methods
/////////////////////////////////////////////////////////////////////////////

HRESULT Console::powerUp(ComPtr<IProgress> &aProgress)
{
    return i_powerUp(aProgress.asOutParam(), false /* aPaused */);
}

HRESULT Console::powerUpPaused(ComPtr<IProgress> &aProgress)
{
    return i_powerUp(aProgress.asOutParam(), true /* aPaused */);
}

HRESULT Console::powerDown(ComPtr<IProgress> &aProgress)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));
    switch (mMachineState)
    {
        case MachineState_Running:
        case MachineState_Paused:
        case MachineState_Stuck:
            break;

        /* Try cancel the save state. */
        case MachineState_Saving:
            if (!mptrCancelableProgress.isNull())
            {
                HRESULT hrc = mptrCancelableProgress->Cancel();
                if (SUCCEEDED(hrc))
                    break;
            }
            return setError(VBOX_E_INVALID_VM_STATE, tr("Cannot power down at this point during a save state"));

        /* Try cancel the teleportation. */
        case MachineState_Teleporting:
        case MachineState_TeleportingPausedVM:
            if (!mptrCancelableProgress.isNull())
            {
                HRESULT hrc = mptrCancelableProgress->Cancel();
                if (SUCCEEDED(hrc))
                    break;
            }
            return setError(VBOX_E_INVALID_VM_STATE, tr("Cannot power down at this point in a teleportation"));

        /* Try cancel the online snapshot. */
        case MachineState_OnlineSnapshotting:
            if (!mptrCancelableProgress.isNull())
            {
                HRESULT hrc = mptrCancelableProgress->Cancel();
                if (SUCCEEDED(hrc))
                    break;
            }
            return setError(VBOX_E_INVALID_VM_STATE, tr("Cannot power down at this point in an online snapshot"));

        /* Try cancel the live snapshot. */
        case MachineState_LiveSnapshotting:
            if (!mptrCancelableProgress.isNull())
            {
                HRESULT hrc = mptrCancelableProgress->Cancel();
                if (SUCCEEDED(hrc))
                    break;
            }
            return setError(VBOX_E_INVALID_VM_STATE, tr("Cannot power down at this point in a live snapshot"));

        /* Try cancel the FT sync. */
        case MachineState_FaultTolerantSyncing:
            if (!mptrCancelableProgress.isNull())
            {
                HRESULT hrc = mptrCancelableProgress->Cancel();
                if (SUCCEEDED(hrc))
                    break;
            }
            return setError(VBOX_E_INVALID_VM_STATE, tr("Cannot power down at this point in a fault tolerant sync"));

        /* extra nice error message for a common case */
        case MachineState_Saved:
            return setError(VBOX_E_INVALID_VM_STATE, tr("Cannot power down a saved virtual machine"));
        case MachineState_Stopping:
            return setError(VBOX_E_INVALID_VM_STATE, tr("The virtual machine is being powered down"));
        default:
            return setError(VBOX_E_INVALID_VM_STATE,
                            tr("Invalid machine state: %s (must be Running, Paused or Stuck)"),
                            Global::stringifyMachineState(mMachineState));
    }
    LogFlowThisFunc(("Initiating SHUTDOWN request...\n"));

    /* memorize the current machine state */
    MachineState_T lastMachineState = mMachineState;

    HRESULT rc = S_OK;
    bool fBeganPowerDown = false;
    VMPowerDownTask* task = NULL;

    do
    {
        ComPtr<IProgress> pProgress;

#ifdef VBOX_WITH_GUEST_PROPS
        alock.release();

        if (i_isResetTurnedIntoPowerOff())
        {
            mMachine->DeleteGuestProperty(Bstr("/VirtualBox/HostInfo/VMPowerOffReason").raw());
            mMachine->SetGuestProperty(Bstr("/VirtualBox/HostInfo/VMPowerOffReason").raw(),
                                       Bstr("PowerOff").raw(), Bstr("RDONLYGUEST").raw());
            mMachine->SaveSettings();
        }

        alock.acquire();
#endif

        /*
         * request a progress object from the server
         * (this will set the machine state to Stopping on the server to block
         * others from accessing this machine)
         */
        rc = mControl->BeginPoweringDown(pProgress.asOutParam());
        if (FAILED(rc))
            break;

        fBeganPowerDown = true;

        /* sync the state with the server */
        i_setMachineStateLocally(MachineState_Stopping);
        try
        {
            task = new VMPowerDownTask(this, pProgress);
            if (!task->isOk())
            {
                throw E_FAIL;
            }
        }
        catch(...)
        {
            delete task;
            rc = setError(E_FAIL, "Could not create VMPowerDownTask object \n");
            break;
        }

        rc = task->createThread();

        /* pass the progress to the caller */
        pProgress.queryInterfaceTo(aProgress.asOutParam());
    }
    while (0);

    if (FAILED(rc))
    {
        /* preserve existing error info */
        ErrorInfoKeeper eik;

        if (fBeganPowerDown)
        {
            /*
             * cancel the requested power down procedure.
             * This will reset the machine state to the state it had right
             * before calling mControl->BeginPoweringDown().
             */
            mControl->EndPoweringDown(eik.getResultCode(), eik.getText().raw());        }

        i_setMachineStateLocally(lastMachineState);
    }

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();

    return rc;
}

HRESULT Console::reset()
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));
    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting
        /** @todo r=bird: This should be allowed on paused VMs as well. Later.  */
       )
        return i_setInvalidMachineStateError();

    /* protect mpUVM */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    /* release the lock before a VMR3* call (EMT might wait for it, @bugref{7648})! */
    alock.release();

    int vrc = VMR3Reset(ptrVM.rawUVM());

    HRESULT rc = RT_SUCCESS(vrc) ? S_OK :
        setError(VBOX_E_VM_ERROR,
                 tr("Could not reset the machine (%Rrc)"),
                 vrc);

    LogFlowThisFunc(("mMachineState=%d, rc=%Rhrc\n", mMachineState, rc));
    LogFlowThisFuncLeave();
    return rc;
}

/*static*/ DECLCALLBACK(int) Console::i_unplugCpu(Console *pThis, PUVM pUVM, VMCPUID idCpu)
{
    LogFlowFunc(("pThis=%p pVM=%p idCpu=%u\n", pThis, pUVM, idCpu));

    AssertReturn(pThis, VERR_INVALID_PARAMETER);

    int vrc = PDMR3DeviceDetach(pUVM, "acpi", 0, idCpu, 0);
    Log(("UnplugCpu: rc=%Rrc\n", vrc));

    return vrc;
}

HRESULT Console::i_doCPURemove(ULONG aCpu, PUVM pUVM)
{
    HRESULT rc = S_OK;

    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));
    AssertReturn(m_pVMMDev, E_FAIL);
    PPDMIVMMDEVPORT pVmmDevPort = m_pVMMDev->getVMMDevPort();
    AssertReturn(pVmmDevPort, E_FAIL);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting
       )
        return i_setInvalidMachineStateError();

    /* Check if the CPU is present */
    BOOL fCpuAttached;
    rc = mMachine->GetCPUStatus(aCpu, &fCpuAttached);
    if (FAILED(rc))
        return rc;
    if (!fCpuAttached)
        return setError(E_FAIL, tr("CPU %d is not attached"), aCpu);

    /* Leave the lock before any EMT/VMMDev call. */
    alock.release();
    bool fLocked = true;

    /* Check if the CPU is unlocked */
    PPDMIBASE pBase;
    int vrc = PDMR3QueryDeviceLun(pUVM, "acpi", 0, aCpu, &pBase);
    if (RT_SUCCESS(vrc))
    {
        Assert(pBase);
        PPDMIACPIPORT pApicPort = PDMIBASE_QUERY_INTERFACE(pBase, PDMIACPIPORT);

        /* Notify the guest if possible. */
        uint32_t idCpuCore, idCpuPackage;
        vrc = VMR3GetCpuCoreAndPackageIdFromCpuId(pUVM, aCpu, &idCpuCore, &idCpuPackage); AssertRC(vrc);
        if (RT_SUCCESS(vrc))
            vrc = pVmmDevPort->pfnCpuHotUnplug(pVmmDevPort, idCpuCore, idCpuPackage);
        if (RT_SUCCESS(vrc))
        {
            unsigned cTries = 100;
            do
            {
                /* It will take some time until the event is processed in the guest. Wait...  */
                vrc = pApicPort ? pApicPort->pfnGetCpuStatus(pApicPort, aCpu, &fLocked) : VERR_INVALID_POINTER;
                if (RT_SUCCESS(vrc) && !fLocked)
                    break;

                /* Sleep a bit */
                RTThreadSleep(100);
            } while (cTries-- > 0);
        }
        else if (vrc == VERR_VMMDEV_CPU_HOTPLUG_NOT_MONITORED_BY_GUEST)
        {
            /* Query one time. It is possible that the user ejected the CPU. */
            vrc = pApicPort ? pApicPort->pfnGetCpuStatus(pApicPort, aCpu, &fLocked) : VERR_INVALID_POINTER;
        }
    }

    /* If the CPU was unlocked we can detach it now. */
    if (RT_SUCCESS(vrc) && !fLocked)
    {
        /*
         * Call worker in EMT, that's faster and safer than doing everything
         * using VMR3ReqCall.
         */
        PVMREQ pReq;
        vrc = VMR3ReqCallU(pUVM, 0, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)i_unplugCpu, 3,
                           this, pUVM, (VMCPUID)aCpu);

        if (vrc == VERR_TIMEOUT)
            vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);
        AssertRC(vrc);
        if (RT_SUCCESS(vrc))
            vrc = pReq->iStatus;
        VMR3ReqFree(pReq);

        if (RT_SUCCESS(vrc))
        {
            /* Detach it from the VM  */
            vrc = VMR3HotUnplugCpu(pUVM, aCpu);
            AssertRC(vrc);
        }
        else
           rc = setError(VBOX_E_VM_ERROR,
                         tr("Hot-Remove failed (rc=%Rrc)"), vrc);
    }
    else
        rc = setError(VBOX_E_VM_ERROR,
                      tr("Hot-Remove was aborted because the CPU may still be used by the guest"), VERR_RESOURCE_BUSY);

    LogFlowThisFunc(("mMachineState=%d, rc=%Rhrc\n", mMachineState, rc));
    LogFlowThisFuncLeave();
    return rc;
}

/*static*/ DECLCALLBACK(int) Console::i_plugCpu(Console *pThis, PUVM pUVM, VMCPUID idCpu)
{
    LogFlowFunc(("pThis=%p uCpu=%u\n", pThis, idCpu));

    AssertReturn(pThis, VERR_INVALID_PARAMETER);

    int rc = VMR3HotPlugCpu(pUVM, idCpu);
    AssertRC(rc);

    PCFGMNODE pInst = CFGMR3GetChild(CFGMR3GetRootU(pUVM), "Devices/acpi/0/");
    AssertRelease(pInst);
    /* nuke anything which might have been left behind. */
    CFGMR3RemoveNode(CFGMR3GetChildF(pInst, "LUN#%u", idCpu));

#define RC_CHECK() do { if (RT_FAILURE(rc)) { AssertReleaseRC(rc); break; } } while (0)

    PCFGMNODE pLunL0;
    PCFGMNODE pCfg;
    rc = CFGMR3InsertNodeF(pInst, &pLunL0, "LUN#%u", idCpu);     RC_CHECK();
    rc = CFGMR3InsertString(pLunL0, "Driver",       "ACPICpu"); RC_CHECK();
    rc = CFGMR3InsertNode(pLunL0,   "Config",       &pCfg);     RC_CHECK();

    /*
     * Attach the driver.
     */
    PPDMIBASE pBase;
    rc = PDMR3DeviceAttach(pUVM, "acpi", 0, idCpu, 0, &pBase); RC_CHECK();

    Log(("PlugCpu: rc=%Rrc\n", rc));

    CFGMR3Dump(pInst);

#undef RC_CHECK

    return VINF_SUCCESS;
}

HRESULT Console::i_doCPUAdd(ULONG aCpu, PUVM pUVM)
{
    HRESULT rc = S_OK;

    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));
    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting
        /** @todo r=bird: This should be allowed on paused VMs as well. Later.  */
       )
        return i_setInvalidMachineStateError();

    AssertReturn(m_pVMMDev, E_FAIL);
    PPDMIVMMDEVPORT pDevPort = m_pVMMDev->getVMMDevPort();
    AssertReturn(pDevPort, E_FAIL);

    /* Check if the CPU is present */
    BOOL fCpuAttached;
    rc = mMachine->GetCPUStatus(aCpu, &fCpuAttached);
    if (FAILED(rc)) return rc;

    if (fCpuAttached)
        return setError(E_FAIL,
                        tr("CPU %d is already attached"), aCpu);

    /*
     * Call worker in EMT, that's faster and safer than doing everything
     * using VMR3ReqCall. Note that we separate VMR3ReqCall from VMR3ReqWait
     * here to make requests from under the lock in order to serialize them.
     */
    PVMREQ pReq;
    int vrc = VMR3ReqCallU(pUVM, 0, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)i_plugCpu, 3,
                           this, pUVM, aCpu);

    /* release the lock before a VMR3* call (EMT might wait for it, @bugref{7648})! */
    alock.release();

    if (vrc == VERR_TIMEOUT)
        vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);
    AssertRC(vrc);
    if (RT_SUCCESS(vrc))
        vrc = pReq->iStatus;
    VMR3ReqFree(pReq);

    if (RT_SUCCESS(vrc))
    {
        /* Notify the guest if possible. */
        uint32_t idCpuCore, idCpuPackage;
        vrc = VMR3GetCpuCoreAndPackageIdFromCpuId(pUVM, aCpu, &idCpuCore, &idCpuPackage); AssertRC(vrc);
        if (RT_SUCCESS(vrc))
            vrc = pDevPort->pfnCpuHotPlug(pDevPort, idCpuCore, idCpuPackage);
        /** @todo warning if the guest doesn't support it */
    }
    else
        rc = setError(VBOX_E_VM_ERROR,
                      tr("Could not add CPU to the machine (%Rrc)"),
                      vrc);

    LogFlowThisFunc(("mMachineState=%d, rc=%Rhrc\n", mMachineState, rc));
    LogFlowThisFuncLeave();
    return rc;
}

HRESULT Console::pause()
{
    LogFlowThisFuncEnter();

    HRESULT rc = i_pause(Reason_Unspecified);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

HRESULT Console::resume()
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mMachineState != MachineState_Paused)
        return setError(VBOX_E_INVALID_VM_STATE,
                        tr("Cannot resume the machine as it is not paused (machine state: %s)"),
                        Global::stringifyMachineState(mMachineState));

    HRESULT rc = i_resume(Reason_Unspecified, alock);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

HRESULT Console::powerButton()
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting
       )
        return i_setInvalidMachineStateError();

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    // no need to release lock, as there are no cross-thread callbacks

    /* get the acpi device interface and press the button. */
    PPDMIBASE pBase;
    int vrc = PDMR3QueryDeviceLun(ptrVM.rawUVM(), "acpi", 0, 0, &pBase);
    if (RT_SUCCESS(vrc))
    {
        Assert(pBase);
        PPDMIACPIPORT pPort = PDMIBASE_QUERY_INTERFACE(pBase, PDMIACPIPORT);
        if (pPort)
            vrc = pPort->pfnPowerButtonPress(pPort);
        else
            vrc = VERR_PDM_MISSING_INTERFACE;
    }

    HRESULT rc = RT_SUCCESS(vrc) ? S_OK :
        setError(VBOX_E_PDM_ERROR,
                 tr("Controlled power off failed (%Rrc)"),
                 vrc);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

HRESULT Console::getPowerButtonHandled(BOOL *aHandled)
{
    LogFlowThisFuncEnter();

    *aHandled = FALSE;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting
       )
        return i_setInvalidMachineStateError();

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    // no need to release lock, as there are no cross-thread callbacks

    /* get the acpi device interface and check if the button press was handled. */
    PPDMIBASE pBase;
    int vrc = PDMR3QueryDeviceLun(ptrVM.rawUVM(), "acpi", 0, 0, &pBase);
    if (RT_SUCCESS(vrc))
    {
        Assert(pBase);
        PPDMIACPIPORT pPort = PDMIBASE_QUERY_INTERFACE(pBase, PDMIACPIPORT);
        if (pPort)
        {
            bool fHandled = false;
            vrc = pPort->pfnGetPowerButtonHandled(pPort, &fHandled);
            if (RT_SUCCESS(vrc))
                *aHandled = fHandled;
        }
        else
            vrc = VERR_PDM_MISSING_INTERFACE;
    }

    HRESULT rc = RT_SUCCESS(vrc) ? S_OK :
        setError(VBOX_E_PDM_ERROR,
            tr("Checking if the ACPI Power Button event was handled by the guest OS failed (%Rrc)"),
            vrc);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

HRESULT Console::getGuestEnteredACPIMode(BOOL *aEntered)
{
    LogFlowThisFuncEnter();

    *aEntered = FALSE;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting
       )
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Invalid machine state %s when checking if the guest entered the ACPI mode)"),
            Global::stringifyMachineState(mMachineState));

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    // no need to release lock, as there are no cross-thread callbacks

    /* get the acpi device interface and query the information. */
    PPDMIBASE pBase;
    int vrc = PDMR3QueryDeviceLun(ptrVM.rawUVM(), "acpi", 0, 0, &pBase);
    if (RT_SUCCESS(vrc))
    {
        Assert(pBase);
        PPDMIACPIPORT pPort = PDMIBASE_QUERY_INTERFACE(pBase, PDMIACPIPORT);
        if (pPort)
        {
            bool fEntered = false;
            vrc = pPort->pfnGetGuestEnteredACPIMode(pPort, &fEntered);
            if (RT_SUCCESS(vrc))
                *aEntered = fEntered;
        }
        else
            vrc = VERR_PDM_MISSING_INTERFACE;
    }

    LogFlowThisFuncLeave();
    return S_OK;
}

HRESULT Console::sleepButton()
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting)
        return i_setInvalidMachineStateError();

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    // no need to release lock, as there are no cross-thread callbacks

    /* get the acpi device interface and press the sleep button. */
    PPDMIBASE pBase;
    int vrc = PDMR3QueryDeviceLun(ptrVM.rawUVM(), "acpi", 0, 0, &pBase);
    if (RT_SUCCESS(vrc))
    {
        Assert(pBase);
        PPDMIACPIPORT pPort = PDMIBASE_QUERY_INTERFACE(pBase, PDMIACPIPORT);
        if (pPort)
            vrc = pPort->pfnSleepButtonPress(pPort);
        else
            vrc = VERR_PDM_MISSING_INTERFACE;
    }

    HRESULT rc = RT_SUCCESS(vrc) ? S_OK :
        setError(VBOX_E_PDM_ERROR,
            tr("Sending sleep button event failed (%Rrc)"),
            vrc);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

/** read the value of a LED. */
inline uint32_t readAndClearLed(PPDMLED pLed)
{
    if (!pLed)
        return 0;
    uint32_t u32 = pLed->Actual.u32 | pLed->Asserted.u32;
    pLed->Asserted.u32 = 0;
    return u32;
}

HRESULT Console::getDeviceActivity(const std::vector<DeviceType_T> &aType,
                                   std::vector<DeviceActivity_T> &aActivity)
{
    /*
     * Note: we don't lock the console object here because
     * readAndClearLed() should be thread safe.
     */

    aActivity.resize(aType.size());

    size_t iType;
    for (iType = 0; iType < aType.size(); ++iType)
    {
        /* Get LED array to read */
        PDMLEDCORE SumLed = {0};
        switch (aType[iType])
        {
            case DeviceType_Floppy:
            case DeviceType_DVD:
            case DeviceType_HardDisk:
            {
                for (unsigned i = 0; i < RT_ELEMENTS(mapStorageLeds); ++i)
                    if (maStorageDevType[i] == aType[iType])
                        SumLed.u32 |= readAndClearLed(mapStorageLeds[i]);
                break;
            }

            case DeviceType_Network:
            {
                for (unsigned i = 0; i < RT_ELEMENTS(mapNetworkLeds); ++i)
                    SumLed.u32 |= readAndClearLed(mapNetworkLeds[i]);
                break;
            }

            case DeviceType_USB:
            {
                for (unsigned i = 0; i < RT_ELEMENTS(mapUSBLed); ++i)
                    SumLed.u32 |= readAndClearLed(mapUSBLed[i]);
                break;
            }

            case DeviceType_SharedFolder:
            {
                SumLed.u32 |= readAndClearLed(mapSharedFolderLed);
                break;
            }

            case DeviceType_Graphics3D:
            {
                SumLed.u32 |= readAndClearLed(mapCrOglLed);
                break;
            }

            default:
                return setError(E_INVALIDARG,
                    tr("Invalid device type: %d"),
                    aType[iType]);
        }

        /* Compose the result */
        switch (SumLed.u32 & (PDMLED_READING | PDMLED_WRITING))
        {
            case 0:
                aActivity[iType] = DeviceActivity_Idle;
                break;
            case PDMLED_READING:
                aActivity[iType] = DeviceActivity_Reading;
                break;
            case PDMLED_WRITING:
            case PDMLED_READING | PDMLED_WRITING:
                aActivity[iType] = DeviceActivity_Writing;
                break;
        }
    }

    return S_OK;
}

HRESULT Console::attachUSBDevice(const com::Guid &aId, const com::Utf8Str &aCaptureFilename)
{
#ifdef VBOX_WITH_USB
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Paused)
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot attach a USB device to the machine which is not running or paused (machine state: %s)"),
            Global::stringifyMachineState(mMachineState));

    /* Get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    /* Don't proceed unless we have a USB controller. */
    if (!mfVMHasUsbController)
        return setError(VBOX_E_PDM_ERROR,
            tr("The virtual machine does not have a USB controller"));

    /* release the lock because the USB Proxy service may call us back
     * (via onUSBDeviceAttach()) */
    alock.release();

    /* Request the device capture */
    return mControl->CaptureUSBDevice(Bstr(aId.toString()).raw(), Bstr(aCaptureFilename).raw());

#else   /* !VBOX_WITH_USB */
    return setError(VBOX_E_PDM_ERROR,
        tr("The virtual machine does not have a USB controller"));
#endif  /* !VBOX_WITH_USB */
}

HRESULT Console::detachUSBDevice(const com::Guid &aId, ComPtr<IUSBDevice> &aDevice)
{
    RT_NOREF(aDevice);
#ifdef VBOX_WITH_USB

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Find it. */
    ComObjPtr<OUSBDevice> pUSBDevice;
    USBDeviceList::iterator it = mUSBDevices.begin();
    while (it != mUSBDevices.end())
    {
        if ((*it)->i_id() == aId)
        {
            pUSBDevice = *it;
            break;
        }
        ++it;
    }

    if (!pUSBDevice)
        return setError(E_INVALIDARG,
            tr("USB device with UUID {%RTuuid} is not attached to this machine"),
            aId.raw());

    /* Remove the device from the collection, it is re-added below for failures */
    mUSBDevices.erase(it);

    /*
     * Inform the USB device and USB proxy about what's cooking.
     */
    alock.release();
    HRESULT rc = mControl->DetachUSBDevice(Bstr(aId.toString()).raw(), false /* aDone */);
    if (FAILED(rc))
    {
        /* Re-add the device to the collection */
        alock.acquire();
        mUSBDevices.push_back(pUSBDevice);
        return rc;
    }

    /* Request the PDM to detach the USB device. */
    rc = i_detachUSBDevice(pUSBDevice);
    if (SUCCEEDED(rc))
    {
        /* Request the device release. Even if it fails, the device will
         * remain as held by proxy, which is OK for us (the VM process). */
        rc = mControl->DetachUSBDevice(Bstr(aId.toString()).raw(), true /* aDone */);
    }
    else
    {
        /* Re-add the device to the collection */
        alock.acquire();
        mUSBDevices.push_back(pUSBDevice);
    }

    return rc;


#else   /* !VBOX_WITH_USB */
    return setError(VBOX_E_PDM_ERROR,
        tr("The virtual machine does not have a USB controller"));
#endif  /* !VBOX_WITH_USB */
}


HRESULT Console::findUSBDeviceByAddress(const com::Utf8Str &aName, ComPtr<IUSBDevice> &aDevice)
{
#ifdef VBOX_WITH_USB

    aDevice = NULL;

    SafeIfaceArray<IUSBDevice> devsvec;
    HRESULT rc = COMGETTER(USBDevices)(ComSafeArrayAsOutParam(devsvec));
    if (FAILED(rc)) return rc;

    for (size_t i = 0; i < devsvec.size(); ++i)
    {
        Bstr address;
        rc = devsvec[i]->COMGETTER(Address)(address.asOutParam());
        if (FAILED(rc)) return rc;
        if (address == Bstr(aName))
        {
            ComObjPtr<OUSBDevice> pUSBDevice;
            pUSBDevice.createObject();
            pUSBDevice->init(devsvec[i]);
            return pUSBDevice.queryInterfaceTo(aDevice.asOutParam());
        }
    }

    return setErrorNoLog(VBOX_E_OBJECT_NOT_FOUND,
        tr("Could not find a USB device with address '%s'"),
        aName.c_str());

#else   /* !VBOX_WITH_USB */
    return E_NOTIMPL;
#endif  /* !VBOX_WITH_USB */
}

HRESULT Console::findUSBDeviceById(const com::Guid &aId, ComPtr<IUSBDevice> &aDevice)
{
#ifdef VBOX_WITH_USB

    aDevice = NULL;

    SafeIfaceArray<IUSBDevice> devsvec;
    HRESULT rc = COMGETTER(USBDevices)(ComSafeArrayAsOutParam(devsvec));
    if (FAILED(rc)) return rc;

    for (size_t i = 0; i < devsvec.size(); ++i)
    {
        Bstr id;
        rc = devsvec[i]->COMGETTER(Id)(id.asOutParam());
        if (FAILED(rc)) return rc;
        if (Utf8Str(id) == aId.toString())
        {
            ComObjPtr<OUSBDevice> pUSBDevice;
            pUSBDevice.createObject();
            pUSBDevice->init(devsvec[i]);
            ComObjPtr<IUSBDevice> iUSBDevice = static_cast <ComObjPtr<IUSBDevice> > (pUSBDevice);
            return iUSBDevice.queryInterfaceTo(aDevice.asOutParam());
        }
    }

    return setErrorNoLog(VBOX_E_OBJECT_NOT_FOUND,
        tr("Could not find a USB device with uuid {%RTuuid}"),
        Guid(aId).raw());

#else   /* !VBOX_WITH_USB */
    return E_NOTIMPL;
#endif  /* !VBOX_WITH_USB */
}

HRESULT Console::createSharedFolder(const com::Utf8Str &aName, const com::Utf8Str &aHostPath, BOOL aWritable, BOOL aAutomount)
{
    LogFlowThisFunc(("Entering for '%s' -> '%s'\n", aName.c_str(), aHostPath.c_str()));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /// @todo see @todo in AttachUSBDevice() about the Paused state
    if (mMachineState == MachineState_Saved)
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot create a transient shared folder on the machine in the saved state"));
    if (   mMachineState != MachineState_PoweredOff
        && mMachineState != MachineState_Teleported
        && mMachineState != MachineState_Aborted
        && mMachineState != MachineState_Running
        && mMachineState != MachineState_Paused
       )
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot create a transient shared folder on the machine while it is changing the state (machine state: %s)"),
            Global::stringifyMachineState(mMachineState));

    ComObjPtr<SharedFolder> pSharedFolder;
    HRESULT rc = i_findSharedFolder(aName, pSharedFolder, false /* aSetError */);
    if (SUCCEEDED(rc))
        return setError(VBOX_E_FILE_ERROR,
                        tr("Shared folder named '%s' already exists"),
                        aName.c_str());

    pSharedFolder.createObject();
    rc = pSharedFolder->init(this,
                             aName,
                             aHostPath,
                             !!aWritable,
                             !!aAutomount,
                             true /* fFailOnError */);
    if (FAILED(rc)) return rc;

    /* If the VM is online and supports shared folders, share this folder
     * under the specified name. (Ignore any failure to obtain the VM handle.) */
    SafeVMPtrQuiet ptrVM(this);
    if (    ptrVM.isOk()
         && m_pVMMDev
         && m_pVMMDev->isShFlActive()
       )
    {
        /* first, remove the machine or the global folder if there is any */
        SharedFolderDataMap::const_iterator it;
        if (i_findOtherSharedFolder(aName, it))
        {
            rc = i_removeSharedFolder(aName);
            if (FAILED(rc))
                return rc;
        }

        /* second, create the given folder */
        rc = i_createSharedFolder(aName, SharedFolderData(aHostPath, !!aWritable, !!aAutomount));
        if (FAILED(rc))
            return rc;
    }

    m_mapSharedFolders.insert(std::make_pair(aName, pSharedFolder));

    /* Notify console callbacks after the folder is added to the list. */
    alock.release();
    fireSharedFolderChangedEvent(mEventSource, Scope_Session);

    LogFlowThisFunc(("Leaving for '%s' -> '%s'\n", aName.c_str(), aHostPath.c_str()));

    return rc;
}

HRESULT Console::removeSharedFolder(const com::Utf8Str &aName)
{
    LogFlowThisFunc(("Entering for '%s'\n", aName.c_str()));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /// @todo see @todo in AttachUSBDevice() about the Paused state
    if (mMachineState == MachineState_Saved)
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot remove a transient shared folder from the machine in the saved state"));
    if (   mMachineState != MachineState_PoweredOff
        && mMachineState != MachineState_Teleported
        && mMachineState != MachineState_Aborted
        && mMachineState != MachineState_Running
        && mMachineState != MachineState_Paused
       )
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot remove a transient shared folder from the machine while it is changing the state (machine state: %s)"),
            Global::stringifyMachineState(mMachineState));

    ComObjPtr<SharedFolder> pSharedFolder;
    HRESULT rc = i_findSharedFolder(aName, pSharedFolder, true /* aSetError */);
    if (FAILED(rc)) return rc;

    /* protect the VM handle (if not NULL) */
    SafeVMPtrQuiet ptrVM(this);
    if (    ptrVM.isOk()
         && m_pVMMDev
         && m_pVMMDev->isShFlActive()
       )
    {
        /* if the VM is online and supports shared folders, UNshare this
         * folder. */

        /* first, remove the given folder */
        rc = i_removeSharedFolder(aName);
        if (FAILED(rc)) return rc;

        /* first, remove the machine or the global folder if there is any */
        SharedFolderDataMap::const_iterator it;
        if (i_findOtherSharedFolder(aName, it))
        {
            rc = i_createSharedFolder(aName, it->second);
            /* don't check rc here because we need to remove the console
             * folder from the collection even on failure */
        }
    }

    m_mapSharedFolders.erase(aName);

    /* Notify console callbacks after the folder is removed from the list. */
    alock.release();
    fireSharedFolderChangedEvent(mEventSource, Scope_Session);

    LogFlowThisFunc(("Leaving for '%s'\n", aName.c_str()));

    return rc;
}

HRESULT Console::addDiskEncryptionPassword(const com::Utf8Str &aId, const com::Utf8Str &aPassword,
                                           BOOL aClearOnSuspend)
{
    if (   aId.isEmpty()
        || aPassword.isEmpty())
        return setError(E_FAIL, tr("The ID and password must be both valid"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT hrc = S_OK;
    size_t cbKey = aPassword.length() + 1; /* Include terminator */
    const uint8_t *pbKey = (const uint8_t *)aPassword.c_str();

    int rc = m_pKeyStore->addSecretKey(aId, pbKey, cbKey);
    if (RT_SUCCESS(rc))
    {
        unsigned cDisksConfigured = 0;

        hrc = i_configureEncryptionForDisk(aId, &cDisksConfigured);
        if (SUCCEEDED(hrc))
        {
            SecretKey *pKey = NULL;
            rc = m_pKeyStore->retainSecretKey(aId, &pKey);
            AssertRCReturn(rc, E_FAIL);

            pKey->setUsers(cDisksConfigured);
            pKey->setRemoveOnSuspend(!!aClearOnSuspend);
            m_pKeyStore->releaseSecretKey(aId);
            m_cDisksPwProvided += cDisksConfigured;

            if (   m_cDisksPwProvided == m_cDisksEncrypted
                && mMachineState == MachineState_Paused)
            {
                /* get the VM handle. */
                SafeVMPtr ptrVM(this);
                if (!ptrVM.isOk())
                    return ptrVM.rc();

                alock.release();
                int vrc = VMR3Resume(ptrVM.rawUVM(), VMRESUMEREASON_RECONFIG);

                hrc = RT_SUCCESS(vrc) ? S_OK :
                    setError(VBOX_E_VM_ERROR,
                             tr("Could not resume the machine execution (%Rrc)"),
                             vrc);
            }
        }
    }
    else if (rc == VERR_ALREADY_EXISTS)
        hrc = setError(VBOX_E_OBJECT_IN_USE, tr("A password with the given ID already exists"));
    else if (rc == VERR_NO_MEMORY)
        hrc = setError(E_FAIL, tr("Failed to allocate enough secure memory for the key"));
    else
        hrc = setError(E_FAIL, tr("Unknown error happened while adding a password (%Rrc)"), rc);

    return hrc;
}

HRESULT Console::addDiskEncryptionPasswords(const std::vector<com::Utf8Str> &aIds, const std::vector<com::Utf8Str> &aPasswords,
                                            BOOL aClearOnSuspend)
{
    HRESULT hrc = S_OK;

    if (   !aIds.size()
        || !aPasswords.size())
        return setError(E_FAIL, tr("IDs and passwords must not be empty"));

    if (aIds.size() != aPasswords.size())
        return setError(E_FAIL, tr("The number of entries in the id and password arguments must match"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Check that the IDs do not exist already before changing anything. */
    for (unsigned i = 0; i < aIds.size(); i++)
    {
        SecretKey *pKey = NULL;
        int rc = m_pKeyStore->retainSecretKey(aIds[i], &pKey);
        if (rc != VERR_NOT_FOUND)
        {
            AssertPtr(pKey);
            if (pKey)
                pKey->release();
            return setError(VBOX_E_OBJECT_IN_USE, tr("A password with the given ID already exists"));
        }
    }

    for (unsigned i = 0; i < aIds.size(); i++)
    {
        hrc = addDiskEncryptionPassword(aIds[i], aPasswords[i], aClearOnSuspend);
        if (FAILED(hrc))
        {
            /*
             * Try to remove already successfully added passwords from the map to not
             * change the state of the Console object.
             */
            ErrorInfoKeeper eik; /* Keep current error info or it gets deestroyed in the IPC methods below. */
            for (unsigned ii = 0; ii < i; ii++)
            {
                i_clearDiskEncryptionKeysOnAllAttachmentsWithKeyId(aIds[ii]);
                removeDiskEncryptionPassword(aIds[ii]);
            }

            break;
        }
    }

    return hrc;
}

HRESULT Console::removeDiskEncryptionPassword(const com::Utf8Str &aId)
{
    if (aId.isEmpty())
        return setError(E_FAIL, tr("The ID must be valid"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    SecretKey *pKey = NULL;
    int rc = m_pKeyStore->retainSecretKey(aId, &pKey);
    if (RT_SUCCESS(rc))
    {
        m_cDisksPwProvided -= pKey->getUsers();
        m_pKeyStore->releaseSecretKey(aId);
        rc = m_pKeyStore->deleteSecretKey(aId);
        AssertRCReturn(rc, E_FAIL);
    }
    else if (rc == VERR_NOT_FOUND)
        return setError(VBOX_E_OBJECT_NOT_FOUND, tr("A password with the ID \"%s\" does not exist"),
                                                 aId.c_str());
    else
        return setError(E_FAIL, tr("Failed to remove password with ID \"%s\" (%Rrc)"),
                                aId.c_str(), rc);

    return S_OK;
}

HRESULT Console::clearAllDiskEncryptionPasswords()
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int rc = m_pKeyStore->deleteAllSecretKeys(false /* fSuspend */, false /* fForce */);
    if (rc == VERR_RESOURCE_IN_USE)
        return setError(VBOX_E_OBJECT_IN_USE, tr("A password is still in use by the VM"));
    else if (RT_FAILURE(rc))
        return setError(E_FAIL, tr("Deleting all passwords failed (%Rrc)"));

    m_cDisksPwProvided = 0;
    return S_OK;
}

// Non-interface public methods
/////////////////////////////////////////////////////////////////////////////

/*static*/
HRESULT Console::i_setErrorStatic(HRESULT aResultCode, const char *pcsz, ...)
{
    va_list args;
    va_start(args, pcsz);
    HRESULT rc = setErrorInternal(aResultCode,
                                  getStaticClassIID(),
                                  getStaticComponentName(),
                                  Utf8Str(pcsz, args),
                                  false /* aWarning */,
                                  true /* aLogIt */);
    va_end(args);
    return rc;
}

HRESULT Console::i_setInvalidMachineStateError()
{
    return setError(VBOX_E_INVALID_VM_STATE,
                    tr("Invalid machine state: %s"),
                    Global::stringifyMachineState(mMachineState));
}


/* static */
const char *Console::i_storageControllerTypeToStr(StorageControllerType_T enmCtrlType)
{
    switch (enmCtrlType)
    {
        case StorageControllerType_LsiLogic:
            return "lsilogicscsi";
        case StorageControllerType_BusLogic:
            return "buslogic";
        case StorageControllerType_LsiLogicSas:
            return "lsilogicsas";
        case StorageControllerType_IntelAhci:
            return "ahci";
        case StorageControllerType_PIIX3:
        case StorageControllerType_PIIX4:
        case StorageControllerType_ICH6:
            return "piix3ide";
        case StorageControllerType_I82078:
            return "i82078";
        case StorageControllerType_USB:
            return "Msd";
        case StorageControllerType_NVMe:
            return "nvme";
        default:
            return NULL;
    }
}

HRESULT Console::i_storageBusPortDeviceToLun(StorageBus_T enmBus, LONG port, LONG device, unsigned &uLun)
{
    switch (enmBus)
    {
        case StorageBus_IDE:
        case StorageBus_Floppy:
        {
            AssertMsgReturn(port < 2 && port >= 0, ("%d\n", port), E_INVALIDARG);
            AssertMsgReturn(device < 2 && device >= 0, ("%d\n", device), E_INVALIDARG);
            uLun = 2 * port + device;
            return S_OK;
        }
        case StorageBus_SATA:
        case StorageBus_SCSI:
        case StorageBus_SAS:
        case StorageBus_PCIe:
        {
            uLun = port;
            return S_OK;
        }
        case StorageBus_USB:
        {
             /*
              * It is always the first lun, the port denotes the device instance
              * for the Msd device.
              */
            uLun = 0;
            return S_OK;
        }
        default:
            uLun = 0;
            AssertMsgFailedReturn(("%d\n", enmBus), E_INVALIDARG);
    }
}

// private methods
/////////////////////////////////////////////////////////////////////////////

/**
 * Suspend the VM before we do any medium or network attachment change.
 *
 * @param pUVM              Safe VM handle.
 * @param pAlock            The automatic lock instance. This is for when we have
 *                          to leave it in order to avoid deadlocks.
 * @param pfResume          where to store the information if we need to resume
 *                          afterwards.
 */
HRESULT Console::i_suspendBeforeConfigChange(PUVM pUVM, AutoWriteLock *pAlock, bool *pfResume)
{
    *pfResume = false;
    VMSTATE enmVMState = VMR3GetStateU(pUVM);
    switch (enmVMState)
    {
        case VMSTATE_RUNNING:
        case VMSTATE_RESETTING:
        case VMSTATE_SOFT_RESETTING:
        {
            LogFlowFunc(("Suspending the VM...\n"));
            /* disable the callback to prevent Console-level state change */
            mVMStateChangeCallbackDisabled = true;
            if (pAlock)
                pAlock->release();
            int rc = VMR3Suspend(pUVM, VMSUSPENDREASON_RECONFIG);
            if (pAlock)
                pAlock->acquire();
            mVMStateChangeCallbackDisabled = false;
            if (RT_FAILURE(rc))
                return setErrorInternal(VBOX_E_INVALID_VM_STATE,
                                        COM_IIDOF(IConsole),
                                        getStaticComponentName(),
                                        Utf8StrFmt("Could suspend VM for medium change (%Rrc)", rc),
                                        false /*aWarning*/,
                                        true /*aLogIt*/);
            *pfResume = true;
            break;
        }
        case VMSTATE_SUSPENDED:
            break;
        default:
            return setErrorInternal(VBOX_E_INVALID_VM_STATE,
                                    COM_IIDOF(IConsole),
                                    getStaticComponentName(),
                                    Utf8StrFmt("Invalid state '%s' for changing medium",
                                               VMR3GetStateName(enmVMState)),
                                    false /*aWarning*/,
                                    true /*aLogIt*/);
    }

    return S_OK;
}

/**
 * Resume the VM after we did any medium or network attachment change.
 * This is the counterpart to Console::suspendBeforeConfigChange().
 *
 * @param pUVM              Safe VM handle.
 */
void Console::i_resumeAfterConfigChange(PUVM pUVM)
{
    LogFlowFunc(("Resuming the VM...\n"));
    /* disable the callback to prevent Console-level state change */
    mVMStateChangeCallbackDisabled = true;
    int rc = VMR3Resume(pUVM, VMRESUMEREASON_RECONFIG);
    mVMStateChangeCallbackDisabled = false;
    AssertRC(rc);
    if (RT_FAILURE(rc))
    {
        VMSTATE enmVMState = VMR3GetStateU(pUVM);
        if (enmVMState == VMSTATE_SUSPENDED)
        {
            /* too bad, we failed. try to sync the console state with the VMM state */
            i_vmstateChangeCallback(pUVM, VMSTATE_SUSPENDED, enmVMState, this);
        }
    }
}

/**
 * Process a medium change.
 *
 * @param aMediumAttachment The medium attachment with the new medium state.
 * @param fForce            Force medium chance, if it is locked or not.
 * @param pUVM              Safe VM handle.
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_doMediumChange(IMediumAttachment *aMediumAttachment, bool fForce, PUVM pUVM)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* We will need to release the write lock before calling EMT */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;
    const char *pszDevice = NULL;

    SafeIfaceArray<IStorageController> ctrls;
    rc = mMachine->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(ctrls));
    AssertComRC(rc);
    IMedium *pMedium;
    rc = aMediumAttachment->COMGETTER(Medium)(&pMedium);
    AssertComRC(rc);
    Bstr mediumLocation;
    if (pMedium)
    {
        rc = pMedium->COMGETTER(Location)(mediumLocation.asOutParam());
        AssertComRC(rc);
    }

    Bstr attCtrlName;
    rc = aMediumAttachment->COMGETTER(Controller)(attCtrlName.asOutParam());
    AssertComRC(rc);
    ComPtr<IStorageController> pStorageController;
    for (size_t i = 0; i < ctrls.size(); ++i)
    {
        Bstr ctrlName;
        rc = ctrls[i]->COMGETTER(Name)(ctrlName.asOutParam());
        AssertComRC(rc);
        if (attCtrlName == ctrlName)
        {
            pStorageController = ctrls[i];
            break;
        }
    }
    if (pStorageController.isNull())
        return setError(E_FAIL,
                        tr("Could not find storage controller '%ls'"), attCtrlName.raw());

    StorageControllerType_T enmCtrlType;
    rc = pStorageController->COMGETTER(ControllerType)(&enmCtrlType);
    AssertComRC(rc);
    pszDevice = i_storageControllerTypeToStr(enmCtrlType);

    StorageBus_T enmBus;
    rc = pStorageController->COMGETTER(Bus)(&enmBus);
    AssertComRC(rc);
    ULONG uInstance;
    rc = pStorageController->COMGETTER(Instance)(&uInstance);
    AssertComRC(rc);
    BOOL fUseHostIOCache;
    rc = pStorageController->COMGETTER(UseHostIOCache)(&fUseHostIOCache);
    AssertComRC(rc);

    /*
     * Suspend the VM first. The VM must not be running since it might have
     * pending I/O to the drive which is being changed.
     */
    bool fResume = false;
    rc = i_suspendBeforeConfigChange(pUVM, &alock, &fResume);
    if (FAILED(rc))
        return rc;

    /*
     * Call worker in EMT, that's faster and safer than doing everything
     * using VMR3ReqCall. Note that we separate VMR3ReqCall from VMR3ReqWait
     * here to make requests from under the lock in order to serialize them.
     */
    PVMREQ pReq;
    int vrc = VMR3ReqCallU(pUVM, VMCPUID_ANY, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)i_changeRemovableMedium, 8,
                           this, pUVM, pszDevice, uInstance, enmBus, fUseHostIOCache, aMediumAttachment, fForce);

    /* release the lock before waiting for a result (EMT might wait for it, @bugref{7648})! */
    alock.release();

    if (vrc == VERR_TIMEOUT)
        vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);
    AssertRC(vrc);
    if (RT_SUCCESS(vrc))
        vrc = pReq->iStatus;
    VMR3ReqFree(pReq);

    if (fResume)
        i_resumeAfterConfigChange(pUVM);

    if (RT_SUCCESS(vrc))
    {
        LogFlowThisFunc(("Returns S_OK\n"));
        return S_OK;
    }

    if (pMedium)
        return setError(E_FAIL,
                        tr("Could not mount the media/drive '%ls' (%Rrc)"),
                        mediumLocation.raw(), vrc);

    return setError(E_FAIL,
                    tr("Could not unmount the currently mounted media/drive (%Rrc)"),
                    vrc);
}

/**
 * Performs the medium change in EMT.
 *
 * @returns VBox status code.
 *
 * @param   pThis           Pointer to the Console object.
 * @param   pUVM            The VM handle.
 * @param   pcszDevice      The PDM device name.
 * @param   uInstance       The PDM device instance.
 * @param   enmBus          The storage bus type of the controller.
 * @param   fUseHostIOCache Whether to use the host I/O cache (disable async I/O).
 * @param   aMediumAtt      The medium attachment.
 * @param   fForce          Force unmounting.
 *
 * @thread  EMT
 * @note The VM must not be running since it might have pending I/O to the drive which is being changed.
 */
DECLCALLBACK(int) Console::i_changeRemovableMedium(Console *pThis,
                                                   PUVM pUVM,
                                                   const char *pcszDevice,
                                                   unsigned uInstance,
                                                   StorageBus_T enmBus,
                                                   bool fUseHostIOCache,
                                                   IMediumAttachment *aMediumAtt,
                                                   bool fForce)
{
    LogFlowFunc(("pThis=%p uInstance=%u pszDevice=%p:{%s} enmBus=%u, aMediumAtt=%p, fForce=%d\n",
                 pThis, uInstance, pcszDevice, pcszDevice, enmBus, aMediumAtt, fForce));

    AssertReturn(pThis, VERR_INVALID_PARAMETER);

    AutoCaller autoCaller(pThis);
    AssertComRCReturn(autoCaller.rc(), VERR_ACCESS_DENIED);

    /*
     * Check the VM for correct state.
     */
    VMSTATE enmVMState = VMR3GetStateU(pUVM);
    AssertReturn(enmVMState == VMSTATE_SUSPENDED, VERR_INVALID_STATE);

    int rc = pThis->i_configMediumAttachment(pcszDevice,
                                             uInstance,
                                             enmBus,
                                             fUseHostIOCache,
                                             false /* fSetupMerge */,
                                             false /* fBuiltinIOCache */,
                                             false /* fInsertDiskIntegrityDrv. */,
                                             0 /* uMergeSource */,
                                             0 /* uMergeTarget */,
                                             aMediumAtt,
                                             pThis->mMachineState,
                                             NULL /* phrc */,
                                             true /* fAttachDetach */,
                                             fForce /* fForceUnmount */,
                                             false  /* fHotplug */,
                                             pUVM,
                                             NULL /* paLedDevType */,
                                             NULL /* ppLunL0 */);
    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}


/**
 * Attach a new storage device to the VM.
 *
 * @param aMediumAttachment The medium attachment which is added.
 * @param pUVM              Safe VM handle.
 * @param fSilent           Flag whether to notify the guest about the attached device.
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_doStorageDeviceAttach(IMediumAttachment *aMediumAttachment, PUVM pUVM, bool fSilent)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* We will need to release the write lock before calling EMT */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;
    const char *pszDevice = NULL;

    SafeIfaceArray<IStorageController> ctrls;
    rc = mMachine->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(ctrls));
    AssertComRC(rc);
    IMedium *pMedium;
    rc = aMediumAttachment->COMGETTER(Medium)(&pMedium);
    AssertComRC(rc);
    Bstr mediumLocation;
    if (pMedium)
    {
        rc = pMedium->COMGETTER(Location)(mediumLocation.asOutParam());
        AssertComRC(rc);
    }

    Bstr attCtrlName;
    rc = aMediumAttachment->COMGETTER(Controller)(attCtrlName.asOutParam());
    AssertComRC(rc);
    ComPtr<IStorageController> pStorageController;
    for (size_t i = 0; i < ctrls.size(); ++i)
    {
        Bstr ctrlName;
        rc = ctrls[i]->COMGETTER(Name)(ctrlName.asOutParam());
        AssertComRC(rc);
        if (attCtrlName == ctrlName)
        {
            pStorageController = ctrls[i];
            break;
        }
    }
    if (pStorageController.isNull())
        return setError(E_FAIL,
                        tr("Could not find storage controller '%ls'"), attCtrlName.raw());

    StorageControllerType_T enmCtrlType;
    rc = pStorageController->COMGETTER(ControllerType)(&enmCtrlType);
    AssertComRC(rc);
    pszDevice = i_storageControllerTypeToStr(enmCtrlType);

    StorageBus_T enmBus;
    rc = pStorageController->COMGETTER(Bus)(&enmBus);
    AssertComRC(rc);
    ULONG uInstance;
    rc = pStorageController->COMGETTER(Instance)(&uInstance);
    AssertComRC(rc);
    BOOL fUseHostIOCache;
    rc = pStorageController->COMGETTER(UseHostIOCache)(&fUseHostIOCache);
    AssertComRC(rc);

    /*
     * Suspend the VM first. The VM must not be running since it might have
     * pending I/O to the drive which is being changed.
     */
    bool fResume = false;
    rc = i_suspendBeforeConfigChange(pUVM, &alock, &fResume);
    if (FAILED(rc))
        return rc;

    /*
     * Call worker in EMT, that's faster and safer than doing everything
     * using VMR3ReqCall. Note that we separate VMR3ReqCall from VMR3ReqWait
     * here to make requests from under the lock in order to serialize them.
     */
    PVMREQ pReq;
    int vrc = VMR3ReqCallU(pUVM, VMCPUID_ANY, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)i_attachStorageDevice, 8,
                           this, pUVM, pszDevice, uInstance, enmBus, fUseHostIOCache, aMediumAttachment, fSilent);

    /* release the lock before waiting for a result (EMT might wait for it, @bugref{7648})! */
    alock.release();

    if (vrc == VERR_TIMEOUT)
        vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);
    AssertRC(vrc);
    if (RT_SUCCESS(vrc))
        vrc = pReq->iStatus;
    VMR3ReqFree(pReq);

    if (fResume)
        i_resumeAfterConfigChange(pUVM);

    if (RT_SUCCESS(vrc))
    {
        LogFlowThisFunc(("Returns S_OK\n"));
        return S_OK;
    }

    if (!pMedium)
        return setError(E_FAIL,
                        tr("Could not mount the media/drive '%ls' (%Rrc)"),
                        mediumLocation.raw(), vrc);

    return setError(E_FAIL,
                    tr("Could not unmount the currently mounted media/drive (%Rrc)"),
                    vrc);
}


/**
 * Performs the storage attach operation in EMT.
 *
 * @returns VBox status code.
 *
 * @param   pThis           Pointer to the Console object.
 * @param   pUVM            The VM handle.
 * @param   pcszDevice      The PDM device name.
 * @param   uInstance       The PDM device instance.
 * @param   enmBus          The storage bus type of the controller.
 * @param   fUseHostIOCache Whether to use the host I/O cache (disable async I/O).
 * @param   aMediumAtt      The medium attachment.
 * @param   fSilent         Flag whether to inform the guest about the attached device.
 *
 * @thread  EMT
 * @note The VM must not be running since it might have pending I/O to the drive which is being changed.
 */
DECLCALLBACK(int) Console::i_attachStorageDevice(Console *pThis,
                                                 PUVM pUVM,
                                                 const char *pcszDevice,
                                                 unsigned uInstance,
                                                 StorageBus_T enmBus,
                                                 bool fUseHostIOCache,
                                                 IMediumAttachment *aMediumAtt,
                                                 bool fSilent)
{
    LogFlowFunc(("pThis=%p uInstance=%u pszDevice=%p:{%s} enmBus=%u, aMediumAtt=%p\n",
                 pThis, uInstance, pcszDevice, pcszDevice, enmBus, aMediumAtt));

    AssertReturn(pThis, VERR_INVALID_PARAMETER);

    AutoCaller autoCaller(pThis);
    AssertComRCReturn(autoCaller.rc(), VERR_ACCESS_DENIED);

    /*
     * Check the VM for correct state.
     */
    VMSTATE enmVMState = VMR3GetStateU(pUVM);
    AssertReturn(enmVMState == VMSTATE_SUSPENDED, VERR_INVALID_STATE);

    int rc = pThis->i_configMediumAttachment(pcszDevice,
                                             uInstance,
                                             enmBus,
                                             fUseHostIOCache,
                                             false /* fSetupMerge */,
                                             false /* fBuiltinIOCache */,
                                             false /* fInsertDiskIntegrityDrv. */,
                                             0 /* uMergeSource */,
                                             0 /* uMergeTarget */,
                                             aMediumAtt,
                                             pThis->mMachineState,
                                             NULL /* phrc */,
                                             true /* fAttachDetach */,
                                             false /* fForceUnmount */,
                                             !fSilent /* fHotplug */,
                                             pUVM,
                                             NULL /* paLedDevType */,
                                             NULL);
    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * Attach a new storage device to the VM.
 *
 * @param aMediumAttachment The medium attachment which is added.
 * @param pUVM              Safe VM handle.
 * @param fSilent           Flag whether to notify the guest about the detached device.
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_doStorageDeviceDetach(IMediumAttachment *aMediumAttachment, PUVM pUVM, bool fSilent)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* We will need to release the write lock before calling EMT */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;
    const char *pszDevice = NULL;

    SafeIfaceArray<IStorageController> ctrls;
    rc = mMachine->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(ctrls));
    AssertComRC(rc);
    IMedium *pMedium;
    rc = aMediumAttachment->COMGETTER(Medium)(&pMedium);
    AssertComRC(rc);
    Bstr mediumLocation;
    if (pMedium)
    {
        rc = pMedium->COMGETTER(Location)(mediumLocation.asOutParam());
        AssertComRC(rc);
    }

    Bstr attCtrlName;
    rc = aMediumAttachment->COMGETTER(Controller)(attCtrlName.asOutParam());
    AssertComRC(rc);
    ComPtr<IStorageController> pStorageController;
    for (size_t i = 0; i < ctrls.size(); ++i)
    {
        Bstr ctrlName;
        rc = ctrls[i]->COMGETTER(Name)(ctrlName.asOutParam());
        AssertComRC(rc);
        if (attCtrlName == ctrlName)
        {
            pStorageController = ctrls[i];
            break;
        }
    }
    if (pStorageController.isNull())
        return setError(E_FAIL,
                        tr("Could not find storage controller '%ls'"), attCtrlName.raw());

    StorageControllerType_T enmCtrlType;
    rc = pStorageController->COMGETTER(ControllerType)(&enmCtrlType);
    AssertComRC(rc);
    pszDevice = i_storageControllerTypeToStr(enmCtrlType);

    StorageBus_T enmBus;
    rc = pStorageController->COMGETTER(Bus)(&enmBus);
    AssertComRC(rc);
    ULONG uInstance;
    rc = pStorageController->COMGETTER(Instance)(&uInstance);
    AssertComRC(rc);

    /*
     * Suspend the VM first. The VM must not be running since it might have
     * pending I/O to the drive which is being changed.
     */
    bool fResume = false;
    rc = i_suspendBeforeConfigChange(pUVM, &alock, &fResume);
    if (FAILED(rc))
        return rc;

    /*
     * Call worker in EMT, that's faster and safer than doing everything
     * using VMR3ReqCall. Note that we separate VMR3ReqCall from VMR3ReqWait
     * here to make requests from under the lock in order to serialize them.
     */
    PVMREQ pReq;
    int vrc = VMR3ReqCallU(pUVM, VMCPUID_ANY, &pReq, 0 /* no wait! */, VMREQFLAGS_VBOX_STATUS,
                           (PFNRT)i_detachStorageDevice, 7,
                           this, pUVM, pszDevice, uInstance, enmBus, aMediumAttachment, fSilent);

    /* release the lock before waiting for a result (EMT might wait for it, @bugref{7648})! */
    alock.release();

    if (vrc == VERR_TIMEOUT)
        vrc = VMR3ReqWait(pReq, RT_INDEFINITE_WAIT);
    AssertRC(vrc);
    if (RT_SUCCESS(vrc))
        vrc = pReq->iStatus;
    VMR3ReqFree(pReq);

    if (fResume)
        i_resumeAfterConfigChange(pUVM);

    if (RT_SUCCESS(vrc))
    {
        LogFlowThisFunc(("Returns S_OK\n"));
        return S_OK;
    }

    if (!pMedium)
        return setError(E_FAIL,
                        tr("Could not mount the media/drive '%ls' (%Rrc)"),
                        mediumLocation.raw(), vrc);

    return setError(E_FAIL,
                    tr("Could not unmount the currently mounted media/drive (%Rrc)"),
                    vrc);
}

/**
 * Performs the storage detach operation in EMT.
 *
 * @returns VBox status code.
 *
 * @param   pThis           Pointer to the Console object.
 * @param   pUVM            The VM handle.
 * @param   pcszDevice      The PDM device name.
 * @param   uInstance       The PDM device instance.
 * @param   enmBus          The storage bus type of the controller.
 * @param   pMediumAtt      Pointer to the medium attachment.
 * @param   fSilent         Flag whether to notify the guest about the detached device.
 *
 * @thread  EMT
 * @note The VM must not be running since it might have pending I/O to the drive which is being changed.
 */
DECLCALLBACK(int) Console::i_detachStorageDevice(Console *pThis,
                                                 PUVM pUVM,
                                                 const char *pcszDevice,
                                                 unsigned uInstance,
                                                 StorageBus_T enmBus,
                                                 IMediumAttachment *pMediumAtt,
                                                 bool fSilent)
{
    LogFlowFunc(("pThis=%p uInstance=%u pszDevice=%p:{%s} enmBus=%u, pMediumAtt=%p\n",
                 pThis, uInstance, pcszDevice, pcszDevice, enmBus, pMediumAtt));

    AssertReturn(pThis, VERR_INVALID_PARAMETER);

    AutoCaller autoCaller(pThis);
    AssertComRCReturn(autoCaller.rc(), VERR_ACCESS_DENIED);

    /*
     * Check the VM for correct state.
     */
    VMSTATE enmVMState = VMR3GetStateU(pUVM);
    AssertReturn(enmVMState == VMSTATE_SUSPENDED, VERR_INVALID_STATE);

    /* Determine the base path for the device instance. */
    PCFGMNODE pCtlInst;
    pCtlInst = CFGMR3GetChildF(CFGMR3GetRootU(pUVM), "Devices/%s/%u/", pcszDevice, uInstance);
    AssertReturn(pCtlInst || enmBus == StorageBus_USB, VERR_INTERNAL_ERROR);

#define H()         AssertMsgReturn(!FAILED(hrc), ("hrc=%Rhrc\n", hrc), VERR_GENERAL_FAILURE)

    HRESULT hrc;
    int rc = VINF_SUCCESS;
    int rcRet = VINF_SUCCESS;
    unsigned uLUN;
    LONG lDev;
    LONG lPort;
    DeviceType_T lType;
    PCFGMNODE pLunL0 = NULL;

    hrc = pMediumAtt->COMGETTER(Device)(&lDev);                             H();
    hrc = pMediumAtt->COMGETTER(Port)(&lPort);                              H();
    hrc = pMediumAtt->COMGETTER(Type)(&lType);                              H();
    hrc = Console::i_storageBusPortDeviceToLun(enmBus, lPort, lDev, uLUN);  H();

#undef H

    if (enmBus != StorageBus_USB)
    {
        /* First check if the LUN really exists. */
        pLunL0 = CFGMR3GetChildF(pCtlInst, "LUN#%u", uLUN);
        if (pLunL0)
        {
            uint32_t fFlags = 0;

            if (fSilent)
                fFlags |= PDM_TACH_FLAGS_NOT_HOT_PLUG;

            rc = PDMR3DeviceDetach(pUVM, pcszDevice, uInstance, uLUN, fFlags);
            if (rc == VERR_PDM_NO_DRIVER_ATTACHED_TO_LUN)
                rc = VINF_SUCCESS;
            AssertRCReturn(rc, rc);
            CFGMR3RemoveNode(pLunL0);

            Utf8Str devicePath = Utf8StrFmt("%s/%u/LUN#%u", pcszDevice, uInstance, uLUN);
            pThis->mapMediumAttachments.erase(devicePath);

        }
        else
            AssertFailedReturn(VERR_INTERNAL_ERROR);

        CFGMR3Dump(pCtlInst);
    }
#ifdef VBOX_WITH_USB
    else
    {
        /* Find the correct USB device in the list. */
        USBStorageDeviceList::iterator it;
        for (it = pThis->mUSBStorageDevices.begin(); it != pThis->mUSBStorageDevices.end(); ++it)
        {
            if (it->iPort == lPort)
                break;
        }

        AssertReturn(it != pThis->mUSBStorageDevices.end(), VERR_INTERNAL_ERROR);
        rc = PDMR3UsbDetachDevice(pUVM, &it->mUuid);
        AssertRCReturn(rc, rc);
        pThis->mUSBStorageDevices.erase(it);
    }
#endif

    LogFlowFunc(("Returning %Rrc\n", rcRet));
    return rcRet;
}

/**
 * Called by IInternalSessionControl::OnNetworkAdapterChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onNetworkAdapterChange(INetworkAdapter *aNetworkAdapter, BOOL changeAdapter)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger network changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        /* Get the properties we need from the adapter */
        BOOL fCableConnected, fTraceEnabled;
        rc = aNetworkAdapter->COMGETTER(CableConnected)(&fCableConnected);
        AssertComRC(rc);
        if (SUCCEEDED(rc))
        {
            rc = aNetworkAdapter->COMGETTER(TraceEnabled)(&fTraceEnabled);
            AssertComRC(rc);
            if (SUCCEEDED(rc))
            {
                ULONG ulInstance;
                rc = aNetworkAdapter->COMGETTER(Slot)(&ulInstance);
                AssertComRC(rc);
                if (SUCCEEDED(rc))
                {
                    /*
                     * Find the adapter instance, get the config interface and update
                     * the link state.
                     */
                    NetworkAdapterType_T adapterType;
                    rc = aNetworkAdapter->COMGETTER(AdapterType)(&adapterType);
                    AssertComRC(rc);
                    const char *pszAdapterName = networkAdapterTypeToName(adapterType);

                    // prevent cross-thread deadlocks, don't need the lock any more
                    alock.release();

                    PPDMIBASE pBase;
                    int vrc = PDMR3QueryDeviceLun(ptrVM.rawUVM(), pszAdapterName, ulInstance, 0, &pBase);
                    if (RT_SUCCESS(vrc))
                    {
                        Assert(pBase);
                        PPDMINETWORKCONFIG pINetCfg;
                        pINetCfg = PDMIBASE_QUERY_INTERFACE(pBase, PDMINETWORKCONFIG);
                        if (pINetCfg)
                        {
                            Log(("Console::onNetworkAdapterChange: setting link state to %d\n",
                                  fCableConnected));
                            vrc = pINetCfg->pfnSetLinkState(pINetCfg,
                                                            fCableConnected ? PDMNETWORKLINKSTATE_UP
                                                                            : PDMNETWORKLINKSTATE_DOWN);
                            ComAssertRC(vrc);
                        }
                        if (RT_SUCCESS(vrc) && changeAdapter)
                        {
                            VMSTATE enmVMState = VMR3GetStateU(ptrVM.rawUVM());
                            if (    enmVMState == VMSTATE_RUNNING    /** @todo LiveMigration: Forbid or deal
                                                                         correctly with the _LS variants */
                                ||  enmVMState == VMSTATE_SUSPENDED)
                            {
                                if (fTraceEnabled && fCableConnected && pINetCfg)
                                {
                                    vrc = pINetCfg->pfnSetLinkState(pINetCfg, PDMNETWORKLINKSTATE_DOWN);
                                    ComAssertRC(vrc);
                                }

                                rc = i_doNetworkAdapterChange(ptrVM.rawUVM(), pszAdapterName, ulInstance, 0, aNetworkAdapter);

                                if (fTraceEnabled && fCableConnected && pINetCfg)
                                {
                                    vrc = pINetCfg->pfnSetLinkState(pINetCfg, PDMNETWORKLINKSTATE_UP);
                                    ComAssertRC(vrc);
                                }
                            }
                        }
                    }
                    else if (vrc == VERR_PDM_DEVICE_INSTANCE_NOT_FOUND)
                        return setError(E_FAIL,
                                tr("The network adapter #%u is not enabled"), ulInstance);
                    else
                        ComAssertRC(vrc);

                    if (RT_FAILURE(vrc))
                        rc = E_FAIL;

                    alock.acquire();
                }
            }
        }
        ptrVM.release();
    }

    // definitely don't need the lock any more
    alock.release();

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
        fireNetworkAdapterChangedEvent(mEventSource, aNetworkAdapter);

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Called by IInternalSessionControl::OnNATEngineChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onNATRedirectRuleChange(ULONG ulInstance, BOOL aNatRuleRemove,
                                           NATProtocol_T aProto, IN_BSTR aHostIP,
                                           LONG aHostPort, IN_BSTR aGuestIP,
                                           LONG aGuestPort)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger NAT engine changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        do
        {
            ComPtr<INetworkAdapter> pNetworkAdapter;
            rc = i_machine()->GetNetworkAdapter(ulInstance, pNetworkAdapter.asOutParam());
            if (   FAILED(rc)
                || pNetworkAdapter.isNull())
                break;

            /*
             * Find the adapter instance, get the config interface and update
             * the link state.
             */
            NetworkAdapterType_T adapterType;
            rc = pNetworkAdapter->COMGETTER(AdapterType)(&adapterType);
            if (FAILED(rc))
            {
                AssertComRC(rc);
                rc = E_FAIL;
                break;
            }

            const char *pszAdapterName = networkAdapterTypeToName(adapterType);
            PPDMIBASE pBase;
            int vrc = PDMR3QueryLun(ptrVM.rawUVM(), pszAdapterName, ulInstance, 0, &pBase);
            if (RT_FAILURE(vrc))
            {
                /* This may happen if the NAT network adapter is currently not attached.
                 * This is a valid condition. */
                if (vrc == VERR_PDM_NO_DRIVER_ATTACHED_TO_LUN)
                    break;
                ComAssertRC(vrc);
                rc = E_FAIL;
                break;
            }

            NetworkAttachmentType_T attachmentType;
            rc = pNetworkAdapter->COMGETTER(AttachmentType)(&attachmentType);
            if (   FAILED(rc)
                || attachmentType != NetworkAttachmentType_NAT)
            {
                rc = E_FAIL;
                break;
            }

            /* look down for PDMINETWORKNATCONFIG interface */
            PPDMINETWORKNATCONFIG pNetNatCfg = NULL;
            while (pBase)
            {
                pNetNatCfg = (PPDMINETWORKNATCONFIG)pBase->pfnQueryInterface(pBase, PDMINETWORKNATCONFIG_IID);
                if (pNetNatCfg)
                    break;
                /** @todo r=bird: This stinks! */
                PPDMDRVINS pDrvIns = PDMIBASE_2_PDMDRV(pBase);
                pBase = pDrvIns->pDownBase;
            }
            if (!pNetNatCfg)
                break;

            bool fUdp = aProto == NATProtocol_UDP;
            vrc = pNetNatCfg->pfnRedirectRuleCommand(pNetNatCfg, !!aNatRuleRemove, fUdp,
                                                     Utf8Str(aHostIP).c_str(), (uint16_t)aHostPort, Utf8Str(aGuestIP).c_str(),
                                                     (uint16_t)aGuestPort);
            if (RT_FAILURE(vrc))
                rc = E_FAIL;
        } while (0); /* break loop */
        ptrVM.release();
    }

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}


/*
 * IHostNameResolutionConfigurationChangeEvent
 *
 * Currently this event doesn't carry actual resolver configuration,
 * so we have to go back to VBoxSVC and ask...  This is not ideal.
 */
HRESULT Console::i_onNATDnsChanged()
{
    HRESULT hrc;

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

#if 0 /* XXX: We don't yet pass this down to pfnNotifyDnsChanged */
    ComPtr<IVirtualBox> pVirtualBox;
    hrc = mMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
    if (FAILED(hrc))
        return S_OK;

    ComPtr<IHost> pHost;
    hrc = pVirtualBox->COMGETTER(Host)(pHost.asOutParam());
    if (FAILED(hrc))
        return S_OK;

    SafeArray<BSTR> aNameServers;
    hrc = pHost->COMGETTER(NameServers)(ComSafeArrayAsOutParam(aNameServers));
    if (FAILED(hrc))
        return S_OK;

    const size_t cNameServers = aNameServers.size();
    Log(("DNS change - %zu nameservers\n", cNameServers));

    for (size_t i = 0; i < cNameServers; ++i)
    {
        com::Utf8Str strNameServer(aNameServers[i]);
        Log(("- nameserver[%zu] = \"%s\"\n", i, strNameServer.c_str()));
    }

    com::Bstr domain;
    pHost->COMGETTER(DomainName)(domain.asOutParam());
    Log(("domain name = \"%s\"\n", com::Utf8Str(domain).c_str()));
#endif /* 0 */

    ChipsetType_T enmChipsetType;
    hrc = mMachine->COMGETTER(ChipsetType)(&enmChipsetType);
    if (!FAILED(hrc))
    {
        SafeVMPtrQuiet ptrVM(this);
        if (ptrVM.isOk())
        {
            ULONG ulInstanceMax = (ULONG)Global::getMaxNetworkAdapters(enmChipsetType);

            notifyNatDnsChange(ptrVM.rawUVM(), "pcnet", ulInstanceMax);
            notifyNatDnsChange(ptrVM.rawUVM(), "e1000", ulInstanceMax);
            notifyNatDnsChange(ptrVM.rawUVM(), "virtio-net", ulInstanceMax);
        }
    }

    return S_OK;
}


/*
 * This routine walks over all network device instances, checking if
 * device instance has DrvNAT attachment and triggering DrvNAT DNS
 * change callback.
 */
void Console::notifyNatDnsChange(PUVM pUVM, const char *pszDevice, ULONG ulInstanceMax)
{
    Log(("notifyNatDnsChange: looking for DrvNAT attachment on %s device instances\n", pszDevice));
    for (ULONG ulInstance = 0; ulInstance < ulInstanceMax; ulInstance++)
    {
        PPDMIBASE pBase;
        int rc = PDMR3QueryDriverOnLun(pUVM, pszDevice, ulInstance, 0 /* iLun */, "NAT", &pBase);
        if (RT_FAILURE(rc))
            continue;

        Log(("Instance %s#%d has DrvNAT attachment; do actual notify\n", pszDevice, ulInstance));
        if (pBase)
        {
            PPDMINETWORKNATCONFIG pNetNatCfg = NULL;
            pNetNatCfg = (PPDMINETWORKNATCONFIG)pBase->pfnQueryInterface(pBase, PDMINETWORKNATCONFIG_IID);
            if (pNetNatCfg && pNetNatCfg->pfnNotifyDnsChanged)
                pNetNatCfg->pfnNotifyDnsChanged(pNetNatCfg);
        }
    }
}


VMMDevMouseInterface *Console::i_getVMMDevMouseInterface()
{
    return m_pVMMDev;
}

DisplayMouseInterface *Console::i_getDisplayMouseInterface()
{
    return mDisplay;
}

/**
 * Parses one key value pair.
 *
 * @returns VBox status code.
 * @param   psz     Configuration string.
 * @param   ppszEnd Where to store the pointer to the string following the key value pair.
 * @param   ppszKey Where to store the key on success.
 * @param   ppszVal Where to store the value on success.
 */
int Console::i_consoleParseKeyValue(const char *psz, const char **ppszEnd,
                                    char **ppszKey, char **ppszVal)
{
    int rc = VINF_SUCCESS;
    const char *pszKeyStart = psz;
    const char *pszValStart = NULL;
    size_t cchKey = 0;
    size_t cchVal = 0;

    while (   *psz != '='
           && *psz)
        psz++;

    /* End of string at this point is invalid. */
    if (*psz == '\0')
        return VERR_INVALID_PARAMETER;

    cchKey = psz - pszKeyStart;
    psz++; /* Skip = character */
    pszValStart = psz;

    while (   *psz != ','
           && *psz != '\n'
           && *psz != '\r'
           && *psz)
        psz++;

    cchVal = psz - pszValStart;

    if (cchKey && cchVal)
    {
        *ppszKey = RTStrDupN(pszKeyStart, cchKey);
        if (*ppszKey)
        {
            *ppszVal = RTStrDupN(pszValStart, cchVal);
            if (!*ppszVal)
            {
                RTStrFree(*ppszKey);
                rc = VERR_NO_MEMORY;
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VERR_INVALID_PARAMETER;

    if (RT_SUCCESS(rc))
        *ppszEnd = psz;

    return rc;
}

/**
 * Initializes the secret key interface on all configured attachments.
 *
 * @returns COM status code.
 */
HRESULT Console::i_initSecretKeyIfOnAllAttachments(void)
{
    HRESULT hrc = S_OK;
    SafeIfaceArray<IMediumAttachment> sfaAttachments;

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* Get the VM - must be done before the read-locking. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    hrc = mMachine->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(sfaAttachments));
    AssertComRCReturnRC(hrc);

    /* Find the correct attachment. */
    for (unsigned i = 0; i < sfaAttachments.size(); i++)
    {
        const ComPtr<IMediumAttachment> &pAtt = sfaAttachments[i];
        /*
         * Query storage controller, port and device
         * to identify the correct driver.
         */
        ComPtr<IStorageController> pStorageCtrl;
        Bstr storageCtrlName;
        LONG lPort, lDev;
        ULONG ulStorageCtrlInst;

        hrc = pAtt->COMGETTER(Controller)(storageCtrlName.asOutParam());
        AssertComRC(hrc);

        hrc = pAtt->COMGETTER(Port)(&lPort);
        AssertComRC(hrc);

        hrc = pAtt->COMGETTER(Device)(&lDev);
        AssertComRC(hrc);

        hrc = mMachine->GetStorageControllerByName(storageCtrlName.raw(), pStorageCtrl.asOutParam());
        AssertComRC(hrc);

        hrc = pStorageCtrl->COMGETTER(Instance)(&ulStorageCtrlInst);
        AssertComRC(hrc);

        StorageControllerType_T enmCtrlType;
        hrc = pStorageCtrl->COMGETTER(ControllerType)(&enmCtrlType);
        AssertComRC(hrc);
        const char *pcszDevice = i_storageControllerTypeToStr(enmCtrlType);

        StorageBus_T enmBus;
        hrc = pStorageCtrl->COMGETTER(Bus)(&enmBus);
        AssertComRC(hrc);

        unsigned uLUN;
        hrc = Console::i_storageBusPortDeviceToLun(enmBus, lPort, lDev, uLUN);
        AssertComRC(hrc);

        PPDMIBASE pIBase = NULL;
        PPDMIMEDIA pIMedium = NULL;
        int rc = PDMR3QueryDriverOnLun(ptrVM.rawUVM(), pcszDevice, ulStorageCtrlInst, uLUN, "VD", &pIBase);
        if (RT_SUCCESS(rc))
        {
            if (pIBase)
            {
                pIMedium = (PPDMIMEDIA)pIBase->pfnQueryInterface(pIBase, PDMIMEDIA_IID);
                if (pIMedium)
                {
                    rc = pIMedium->pfnSetSecKeyIf(pIMedium, NULL, mpIfSecKeyHlp);
                    Assert(RT_SUCCESS(rc) || rc == VERR_NOT_SUPPORTED);
                }
            }
        }
    }

    return hrc;
}

/**
 * Removes the key interfaces from all disk attachments with the given key ID.
 * Useful when changing the key store or dropping it.
 *
 * @returns COM status code.
 * @param   strId    The ID to look for.
 */
HRESULT Console::i_clearDiskEncryptionKeysOnAllAttachmentsWithKeyId(const Utf8Str &strId)
{
    HRESULT hrc = S_OK;
    SafeIfaceArray<IMediumAttachment> sfaAttachments;

    /* Get the VM - must be done before the read-locking. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    hrc = mMachine->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(sfaAttachments));
    AssertComRCReturnRC(hrc);

    /* Find the correct attachment. */
    for (unsigned i = 0; i < sfaAttachments.size(); i++)
    {
        const ComPtr<IMediumAttachment> &pAtt = sfaAttachments[i];
        ComPtr<IMedium> pMedium;
        ComPtr<IMedium> pBase;
        Bstr bstrKeyId;

        hrc = pAtt->COMGETTER(Medium)(pMedium.asOutParam());
        if (FAILED(hrc))
            break;

        /* Skip non hard disk attachments. */
        if (pMedium.isNull())
            continue;

        /* Get the UUID of the base medium and compare. */
        hrc = pMedium->COMGETTER(Base)(pBase.asOutParam());
        if (FAILED(hrc))
            break;

        hrc = pBase->GetProperty(Bstr("CRYPT/KeyId").raw(), bstrKeyId.asOutParam());
        if (hrc == VBOX_E_OBJECT_NOT_FOUND)
        {
            hrc = S_OK;
            continue;
        }
        else if (FAILED(hrc))
            break;

        if (strId.equals(Utf8Str(bstrKeyId)))
        {

            /*
             * Query storage controller, port and device
             * to identify the correct driver.
             */
            ComPtr<IStorageController> pStorageCtrl;
            Bstr storageCtrlName;
            LONG lPort, lDev;
            ULONG ulStorageCtrlInst;

            hrc = pAtt->COMGETTER(Controller)(storageCtrlName.asOutParam());
            AssertComRC(hrc);

            hrc = pAtt->COMGETTER(Port)(&lPort);
            AssertComRC(hrc);

            hrc = pAtt->COMGETTER(Device)(&lDev);
            AssertComRC(hrc);

            hrc = mMachine->GetStorageControllerByName(storageCtrlName.raw(), pStorageCtrl.asOutParam());
            AssertComRC(hrc);

            hrc = pStorageCtrl->COMGETTER(Instance)(&ulStorageCtrlInst);
            AssertComRC(hrc);

            StorageControllerType_T enmCtrlType;
            hrc = pStorageCtrl->COMGETTER(ControllerType)(&enmCtrlType);
            AssertComRC(hrc);
            const char *pcszDevice = i_storageControllerTypeToStr(enmCtrlType);

            StorageBus_T enmBus;
            hrc = pStorageCtrl->COMGETTER(Bus)(&enmBus);
            AssertComRC(hrc);

            unsigned uLUN;
            hrc = Console::i_storageBusPortDeviceToLun(enmBus, lPort, lDev, uLUN);
            AssertComRC(hrc);

            PPDMIBASE pIBase = NULL;
            PPDMIMEDIA pIMedium = NULL;
            int rc = PDMR3QueryDriverOnLun(ptrVM.rawUVM(), pcszDevice, ulStorageCtrlInst, uLUN, "VD", &pIBase);
            if (RT_SUCCESS(rc))
            {
                if (pIBase)
                {
                    pIMedium = (PPDMIMEDIA)pIBase->pfnQueryInterface(pIBase, PDMIMEDIA_IID);
                    if (pIMedium)
                    {
                        rc = pIMedium->pfnSetSecKeyIf(pIMedium, NULL, mpIfSecKeyHlp);
                        Assert(RT_SUCCESS(rc) || rc == VERR_NOT_SUPPORTED);
                    }
                }
            }
        }
    }

    return hrc;
}

/**
 * Configures the encryption support for the disk which have encryption conigured
 * with the configured key.
 *
 * @returns COM status code.
 * @param   strId                The ID of the password.
 * @param   pcDisksConfigured    Where to store the number of disks configured for the given ID.
 */
HRESULT Console::i_configureEncryptionForDisk(const com::Utf8Str &strId, unsigned *pcDisksConfigured)
{
    unsigned cDisksConfigured = 0;
    HRESULT hrc = S_OK;
    SafeIfaceArray<IMediumAttachment> sfaAttachments;

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* Get the VM - must be done before the read-locking. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    hrc = mMachine->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(sfaAttachments));
    if (FAILED(hrc))
        return hrc;

    /* Find the correct attachment. */
    for (unsigned i = 0; i < sfaAttachments.size(); i++)
    {
        const ComPtr<IMediumAttachment> &pAtt = sfaAttachments[i];
        ComPtr<IMedium> pMedium;
        ComPtr<IMedium> pBase;
        Bstr bstrKeyId;

        hrc = pAtt->COMGETTER(Medium)(pMedium.asOutParam());
        if (FAILED(hrc))
            break;

        /* Skip non hard disk attachments. */
        if (pMedium.isNull())
            continue;

        /* Get the UUID of the base medium and compare. */
        hrc = pMedium->COMGETTER(Base)(pBase.asOutParam());
        if (FAILED(hrc))
            break;

        hrc = pBase->GetProperty(Bstr("CRYPT/KeyId").raw(), bstrKeyId.asOutParam());
        if (hrc == VBOX_E_OBJECT_NOT_FOUND)
        {
            hrc = S_OK;
            continue;
        }
        else if (FAILED(hrc))
            break;

        if (strId.equals(Utf8Str(bstrKeyId)))
        {
            /*
             * Found the matching medium, query storage controller, port and device
             * to identify the correct driver.
             */
            ComPtr<IStorageController> pStorageCtrl;
            Bstr storageCtrlName;
            LONG lPort, lDev;
            ULONG ulStorageCtrlInst;

            hrc = pAtt->COMGETTER(Controller)(storageCtrlName.asOutParam());
            if (FAILED(hrc))
                break;

            hrc = pAtt->COMGETTER(Port)(&lPort);
            if (FAILED(hrc))
                break;

            hrc = pAtt->COMGETTER(Device)(&lDev);
            if (FAILED(hrc))
                break;

            hrc = mMachine->GetStorageControllerByName(storageCtrlName.raw(), pStorageCtrl.asOutParam());
            if (FAILED(hrc))
                break;

            hrc = pStorageCtrl->COMGETTER(Instance)(&ulStorageCtrlInst);
            if (FAILED(hrc))
                break;

            StorageControllerType_T enmCtrlType;
            hrc = pStorageCtrl->COMGETTER(ControllerType)(&enmCtrlType);
            AssertComRC(hrc);
            const char *pcszDevice = i_storageControllerTypeToStr(enmCtrlType);

            StorageBus_T enmBus;
            hrc = pStorageCtrl->COMGETTER(Bus)(&enmBus);
            AssertComRC(hrc);

            unsigned uLUN;
            hrc = Console::i_storageBusPortDeviceToLun(enmBus, lPort, lDev, uLUN);
            AssertComRCReturnRC(hrc);

            PPDMIBASE pIBase = NULL;
            PPDMIMEDIA pIMedium = NULL;
            int rc = PDMR3QueryDriverOnLun(ptrVM.rawUVM(), pcszDevice, ulStorageCtrlInst, uLUN, "VD", &pIBase);
            if (RT_SUCCESS(rc))
            {
                if (pIBase)
                {
                    pIMedium = (PPDMIMEDIA)pIBase->pfnQueryInterface(pIBase, PDMIMEDIA_IID);
                    if (!pIMedium)
                        return setError(E_FAIL, tr("could not query medium interface of controller"));
                    else
                    {
                        rc = pIMedium->pfnSetSecKeyIf(pIMedium, mpIfSecKey, mpIfSecKeyHlp);
                        if (rc == VERR_VD_PASSWORD_INCORRECT)
                        {
                            hrc = setError(VBOX_E_PASSWORD_INCORRECT, tr("The provided password for ID \"%s\" is not correct for at least one disk using this ID"),
                                           strId.c_str());
                            break;
                        }
                        else if (RT_FAILURE(rc))
                        {
                            hrc = setError(E_FAIL, tr("Failed to set the encryption key (%Rrc)"), rc);
                            break;
                        }

                        if (RT_SUCCESS(rc))
                           cDisksConfigured++;
                    }
                }
                else
                    return setError(E_FAIL, tr("could not query base interface of controller"));
            }
        }
    }

    if (   SUCCEEDED(hrc)
        && pcDisksConfigured)
        *pcDisksConfigured = cDisksConfigured;
    else if (FAILED(hrc))
    {
        /* Clear disk encryption setup on successfully configured attachments. */
        ErrorInfoKeeper eik; /* Keep current error info or it gets deestroyed in the IPC methods below. */
        i_clearDiskEncryptionKeysOnAllAttachmentsWithKeyId(strId);
    }

    return hrc;
}

/**
 * Parses the encryption configuration for one disk.
 *
 * @returns COM status code.
 * @param   psz     Pointer to the configuration for the encryption of one disk.
 * @param   ppszEnd Pointer to the string following encrpytion configuration.
 */
HRESULT Console::i_consoleParseDiskEncryption(const char *psz, const char **ppszEnd)
{
    char *pszUuid = NULL;
    char *pszKeyEnc = NULL;
    int rc = VINF_SUCCESS;
    HRESULT hrc = S_OK;

    while (   *psz
           && RT_SUCCESS(rc))
    {
        char *pszKey = NULL;
        char *pszVal = NULL;
        const char *pszEnd = NULL;

        rc = i_consoleParseKeyValue(psz, &pszEnd, &pszKey, &pszVal);
        if (RT_SUCCESS(rc))
        {
            if (!RTStrCmp(pszKey, "uuid"))
                pszUuid = pszVal;
            else if (!RTStrCmp(pszKey, "dek"))
                pszKeyEnc = pszVal;
            else
                rc = VERR_INVALID_PARAMETER;

            RTStrFree(pszKey);

            if (*pszEnd == ',')
                psz = pszEnd + 1;
            else
            {
                /*
                 * End of the configuration for the current disk, skip linefeed and
                 * carriage returns.
                 */
                while (   *pszEnd == '\n'
                       || *pszEnd == '\r')
                    pszEnd++;

                psz = pszEnd;
                break; /* Stop parsing */
            }

        }
    }

    if (   RT_SUCCESS(rc)
        && pszUuid
        && pszKeyEnc)
    {
        ssize_t cbKey = 0;

        /* Decode the key. */
        cbKey = RTBase64DecodedSize(pszKeyEnc, NULL);
        if (cbKey != -1)
        {
            uint8_t *pbKey;
            rc = RTMemSaferAllocZEx((void **)&pbKey, cbKey, RTMEMSAFER_F_REQUIRE_NOT_PAGABLE);
            if (RT_SUCCESS(rc))
            {
                rc = RTBase64Decode(pszKeyEnc, pbKey, cbKey, NULL, NULL);
                if (RT_SUCCESS(rc))
                {
                    rc = m_pKeyStore->addSecretKey(Utf8Str(pszUuid), pbKey, cbKey);
                    if (RT_SUCCESS(rc))
                    {
                        hrc = i_configureEncryptionForDisk(Utf8Str(pszUuid), NULL);
                        if (FAILED(hrc))
                        {
                            /* Delete the key from the map. */
                            rc = m_pKeyStore->deleteSecretKey(Utf8Str(pszUuid));
                            AssertRC(rc);
                        }
                    }
                }
                else
                    hrc = setError(E_FAIL,
                                   tr("Failed to decode the key (%Rrc)"),
                                   rc);

                RTMemSaferFree(pbKey, cbKey);
            }
            else
                hrc = setError(E_FAIL,
                               tr("Failed to allocate secure memory for the key (%Rrc)"), rc);
        }
        else
            hrc = setError(E_FAIL,
                           tr("The base64 encoding of the passed key is incorrect"));
    }
    else if (RT_SUCCESS(rc))
        hrc = setError(E_FAIL,
                       tr("The encryption configuration is incomplete"));

    if (pszUuid)
        RTStrFree(pszUuid);
    if (pszKeyEnc)
    {
        RTMemWipeThoroughly(pszKeyEnc, strlen(pszKeyEnc), 10 /* cMinPasses */);
        RTStrFree(pszKeyEnc);
    }

    if (ppszEnd)
        *ppszEnd = psz;

    return hrc;
}

HRESULT Console::i_setDiskEncryptionKeys(const Utf8Str &strCfg)
{
    HRESULT hrc = S_OK;
    const char *pszCfg = strCfg.c_str();

    while (   *pszCfg
           && SUCCEEDED(hrc))
    {
        const char *pszNext = NULL;
        hrc = i_consoleParseDiskEncryption(pszCfg, &pszNext);
        pszCfg = pszNext;
    }

    return hrc;
}

void Console::i_removeSecretKeysOnSuspend()
{
    /* Remove keys which are supposed to be removed on a suspend. */
    int rc = m_pKeyStore->deleteAllSecretKeys(true /* fSuspend */, true /* fForce */);
    AssertRC(rc); NOREF(rc);
}

/**
 * Process a network adaptor change.
 *
 * @returns COM status code.
 *
 * @param   pUVM                The VM handle (caller hold this safely).
 * @param   pszDevice           The PDM device name.
 * @param   uInstance           The PDM device instance.
 * @param   uLun                The PDM LUN number of the drive.
 * @param   aNetworkAdapter     The network adapter whose attachment needs to be changed
 */
HRESULT Console::i_doNetworkAdapterChange(PUVM pUVM,
                                          const char *pszDevice,
                                          unsigned uInstance,
                                          unsigned uLun,
                                          INetworkAdapter *aNetworkAdapter)
{
    LogFlowThisFunc(("pszDevice=%p:{%s} uInstance=%u uLun=%u aNetworkAdapter=%p\n",
                      pszDevice, pszDevice, uInstance, uLun, aNetworkAdapter));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /*
     * Suspend the VM first.
     */
    bool fResume = false;
    HRESULT hr = i_suspendBeforeConfigChange(pUVM, NULL, &fResume);
    if (FAILED(hr))
        return hr;

    /*
     * Call worker in EMT, that's faster and safer than doing everything
     * using VM3ReqCall. Note that we separate VMR3ReqCall from VMR3ReqWait
     * here to make requests from under the lock in order to serialize them.
     */
    int rc = VMR3ReqCallWaitU(pUVM, 0 /*idDstCpu*/,
                              (PFNRT)i_changeNetworkAttachment, 6,
                              this, pUVM, pszDevice, uInstance, uLun, aNetworkAdapter);

    if (fResume)
        i_resumeAfterConfigChange(pUVM);

    if (RT_SUCCESS(rc))
        return S_OK;

    return setError(E_FAIL,
                    tr("Could not change the network adaptor attachement type (%Rrc)"), rc);
}


/**
 * Performs the Network Adaptor change in EMT.
 *
 * @returns VBox status code.
 *
 * @param   pThis               Pointer to the Console object.
 * @param   pUVM                The VM handle.
 * @param   pszDevice           The PDM device name.
 * @param   uInstance           The PDM device instance.
 * @param   uLun                The PDM LUN number of the drive.
 * @param   aNetworkAdapter     The network adapter whose attachment needs to be changed
 *
 * @thread  EMT
 * @note Locks the Console object for writing.
 * @note The VM must not be running.
 */
DECLCALLBACK(int) Console::i_changeNetworkAttachment(Console *pThis,
                                                     PUVM pUVM,
                                                     const char *pszDevice,
                                                     unsigned uInstance,
                                                     unsigned uLun,
                                                     INetworkAdapter *aNetworkAdapter)
{
    LogFlowFunc(("pThis=%p pszDevice=%p:{%s} uInstance=%u uLun=%u aNetworkAdapter=%p\n",
                 pThis, pszDevice, pszDevice, uInstance, uLun, aNetworkAdapter));

    AssertReturn(pThis, VERR_INVALID_PARAMETER);

    AutoCaller autoCaller(pThis);
    AssertComRCReturn(autoCaller.rc(), VERR_ACCESS_DENIED);

    ComPtr<IVirtualBox> pVirtualBox;
    pThis->mMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
    ComPtr<ISystemProperties> pSystemProperties;
    if (pVirtualBox)
        pVirtualBox->COMGETTER(SystemProperties)(pSystemProperties.asOutParam());
    ChipsetType_T chipsetType = ChipsetType_PIIX3;
    pThis->mMachine->COMGETTER(ChipsetType)(&chipsetType);
    ULONG maxNetworkAdapters = 0;
    if (pSystemProperties)
        pSystemProperties->GetMaxNetworkAdapters(chipsetType, &maxNetworkAdapters);
    AssertMsg(   (   !strcmp(pszDevice, "pcnet")
                  || !strcmp(pszDevice, "e1000")
                  || !strcmp(pszDevice, "virtio-net"))
              && uLun == 0
              && uInstance < maxNetworkAdapters,
              ("pszDevice=%s uLun=%d uInstance=%d\n", pszDevice, uLun, uInstance));
    Log(("pszDevice=%s uLun=%d uInstance=%d\n", pszDevice, uLun, uInstance));

    /*
     * Check the VM for correct state.
     */
    VMSTATE enmVMState = VMR3GetStateU(pUVM);
    AssertReturn(enmVMState == VMSTATE_SUSPENDED, VERR_INVALID_STATE);

    PCFGMNODE pCfg = NULL;          /* /Devices/Dev/.../Config/ */
    PCFGMNODE pLunL0 = NULL;        /* /Devices/Dev/0/LUN#0/ */
    PCFGMNODE pInst = CFGMR3GetChildF(CFGMR3GetRootU(pUVM), "Devices/%s/%d/", pszDevice, uInstance);
    AssertRelease(pInst);

    int rc = pThis->i_configNetwork(pszDevice, uInstance, uLun, aNetworkAdapter, pCfg, pLunL0, pInst,
                                  true /*fAttachDetach*/, false /*fIgnoreConnectFailure*/);

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * Returns the device name of a given audio adapter.
 *
 * @returns Device name, or an empty string if no device is configured.
 * @param   aAudioAdapter       Audio adapter to return device name for.
 */
Utf8Str Console::i_getAudioAdapterDeviceName(IAudioAdapter *aAudioAdapter)
{
    Utf8Str strDevice;

    AudioControllerType_T audioController;
    HRESULT hrc = aAudioAdapter->COMGETTER(AudioController)(&audioController);
    AssertComRC(hrc);
    if (SUCCEEDED(hrc))
    {
        switch (audioController)
        {
            case AudioControllerType_HDA:  strDevice = "hda";     break;
            case AudioControllerType_AC97: strDevice = "ichac97"; break;
            case AudioControllerType_SB16: strDevice = "sb16";    break;
            default:                                              break; /* None. */
        }
    }

    return strDevice;
}

/**
 * Called by IInternalSessionControl::OnAudioAdapterChange().
 */
HRESULT Console::i_onAudioAdapterChange(IAudioAdapter *aAudioAdapter)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT hrc = S_OK;

    /* don't trigger audio changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        BOOL fEnabledIn, fEnabledOut;
        hrc = aAudioAdapter->COMGETTER(EnabledIn)(&fEnabledIn);
        AssertComRC(hrc);
        if (SUCCEEDED(hrc))
        {
            hrc = aAudioAdapter->COMGETTER(EnabledOut)(&fEnabledOut);
            AssertComRC(hrc);
            if (SUCCEEDED(hrc))
            {
                int rc = VINF_SUCCESS;

                for (ULONG ulLUN = 0; ulLUN < 16 /** @todo Use a define */; ulLUN++)
                {
                    PPDMIBASE pBase;
                    int rc2 = PDMR3QueryDriverOnLun(ptrVM.rawUVM(),
                                                    i_getAudioAdapterDeviceName(aAudioAdapter).c_str(), 0 /* iInstance */,
                                                    ulLUN, "AUDIO", &pBase);
                    if (RT_FAILURE(rc2))
                        continue;

                    if (pBase)
                    {
                        PPDMIAUDIOCONNECTOR pAudioCon =
                             (PPDMIAUDIOCONNECTOR)pBase->pfnQueryInterface(pBase, PDMIAUDIOCONNECTOR_IID);

                        if (   pAudioCon
                            && pAudioCon->pfnEnable)
                        {
                            int rcIn = pAudioCon->pfnEnable(pAudioCon, PDMAUDIODIR_IN, RT_BOOL(fEnabledIn));
                            if (RT_FAILURE(rcIn))
                                LogRel(("Audio: Failed to %s input of LUN#%RU32, rc=%Rrc\n",
                                        fEnabledIn ? "enable" : "disable", ulLUN, rcIn));

                            if (RT_SUCCESS(rc))
                                rc = rcIn;

                            int rcOut = pAudioCon->pfnEnable(pAudioCon, PDMAUDIODIR_OUT, RT_BOOL(fEnabledOut));
                            if (RT_FAILURE(rcOut))
                                LogRel(("Audio: Failed to %s output of LUN#%RU32, rc=%Rrc\n",
                                        fEnabledIn ? "enable" : "disable", ulLUN, rcOut));

                            if (RT_SUCCESS(rc))
                                rc = rcOut;
                        }
                    }
                }

                if (RT_SUCCESS(rc))
                    LogRel(("Audio: Status has changed (input is %s, output is %s)\n",
                            fEnabledIn  ? "enabled" : "disabled", fEnabledOut ? "enabled" : "disabled"));
            }
        }

        ptrVM.release();
    }

    alock.release();

    /* notify console callbacks on success */
    if (SUCCEEDED(hrc))
        fireAudioAdapterChangedEvent(mEventSource, aAudioAdapter);

    LogFlowThisFunc(("Leaving rc=%#x\n", S_OK));
    return S_OK;
}

/**
 * Called by IInternalSessionControl::OnSerialPortChange().
 */
HRESULT Console::i_onSerialPortChange(ISerialPort *aSerialPort)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    fireSerialPortChangedEvent(mEventSource, aSerialPort);

    LogFlowThisFunc(("Leaving rc=%#x\n", S_OK));
    return S_OK;
}

/**
 * Called by IInternalSessionControl::OnParallelPortChange().
 */
HRESULT Console::i_onParallelPortChange(IParallelPort *aParallelPort)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    fireParallelPortChangedEvent(mEventSource, aParallelPort);

    LogFlowThisFunc(("Leaving rc=%#x\n", S_OK));
    return S_OK;
}

/**
 * Called by IInternalSessionControl::OnStorageControllerChange().
 */
HRESULT Console::i_onStorageControllerChange()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    fireStorageControllerChangedEvent(mEventSource);

    LogFlowThisFunc(("Leaving rc=%#x\n", S_OK));
    return S_OK;
}

/**
 * Called by IInternalSessionControl::OnMediumChange().
 */
HRESULT Console::i_onMediumChange(IMediumAttachment *aMediumAttachment, BOOL aForce)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    HRESULT rc = S_OK;

    /* don't trigger medium changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        rc = i_doMediumChange(aMediumAttachment, !!aForce, ptrVM.rawUVM());
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
        fireMediumChangedEvent(mEventSource, aMediumAttachment);

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Called by IInternalSessionControl::OnCPUChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onCPUChange(ULONG aCPU, BOOL aRemove)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    HRESULT rc = S_OK;

    /* don't trigger CPU changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (aRemove)
            rc = i_doCPURemove(aCPU, ptrVM.rawUVM());
        else
            rc = i_doCPUAdd(aCPU, ptrVM.rawUVM());
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
        fireCPUChangedEvent(mEventSource, aCPU, aRemove);

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Called by IInternalSessionControl::OnCpuExecutionCapChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onCPUExecutionCapChange(ULONG aExecutionCap)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger the CPU priority change if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (   mMachineState == MachineState_Running
            || mMachineState == MachineState_Teleporting
            || mMachineState == MachineState_LiveSnapshotting
            )
        {
            /* No need to call in the EMT thread. */
            rc = VMR3SetCpuExecutionCap(ptrVM.rawUVM(), aExecutionCap);
        }
        else
            rc = i_setInvalidMachineStateError();
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireCPUExecutionCapChangedEvent(mEventSource, aExecutionCap);
    }

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Called by IInternalSessionControl::OnClipboardModeChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onClipboardModeChange(ClipboardMode_T aClipboardMode)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger the clipboard mode change if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (   mMachineState == MachineState_Running
            || mMachineState == MachineState_Teleporting
            || mMachineState == MachineState_LiveSnapshotting)
            i_changeClipboardMode(aClipboardMode);
        else
            rc = i_setInvalidMachineStateError();
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireClipboardModeChangedEvent(mEventSource, aClipboardMode);
    }

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Called by IInternalSessionControl::OnDnDModeChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onDnDModeChange(DnDMode_T aDnDMode)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger the drag and drop mode change if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (   mMachineState == MachineState_Running
            || mMachineState == MachineState_Teleporting
            || mMachineState == MachineState_LiveSnapshotting)
            i_changeDnDMode(aDnDMode);
        else
            rc = i_setInvalidMachineStateError();
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireDnDModeChangedEvent(mEventSource, aDnDMode);
    }

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Check the return code of mConsoleVRDPServer->Launch. LogRel() the error reason and
 * return an error message appropriate for setError().
 */
Utf8Str Console::VRDPServerErrorToMsg(int vrc)
{
    Utf8Str errMsg;
    if (vrc == VERR_NET_ADDRESS_IN_USE)
    {
        /* Not fatal if we start the VM, fatal if the VM is already running. */
        Bstr bstr;
        mVRDEServer->GetVRDEProperty(Bstr("TCP/Ports").raw(), bstr.asOutParam());
        errMsg = Utf8StrFmt(tr("VirtualBox Remote Desktop Extension server can't bind to the port(s): %s"),
                                Utf8Str(bstr).c_str());
        LogRel(("VRDE: Warning: failed to launch VRDE server (%Rrc): %s\n", vrc, errMsg.c_str()));
    }
    else if (vrc == VINF_NOT_SUPPORTED)
    {
        /* This means that the VRDE is not installed.
         * Not fatal if we start the VM, fatal if the VM is already running. */
        LogRel(("VRDE: VirtualBox Remote Desktop Extension is not available.\n"));
        errMsg = Utf8Str("VirtualBox Remote Desktop Extension is not available");
    }
    else if (RT_FAILURE(vrc))
    {
        /* Fail if the server is installed but can't start. Always fatal. */
        switch (vrc)
        {
            case VERR_FILE_NOT_FOUND:
                errMsg = Utf8StrFmt(tr("Could not find the VirtualBox Remote Desktop Extension library"));
                break;
            default:
                errMsg = Utf8StrFmt(tr("Failed to launch the Remote Desktop Extension server (%Rrc)"), vrc);
                break;
        }
        LogRel(("VRDE: Failed: (%Rrc): %s\n", vrc, errMsg.c_str()));
    }

    return errMsg;
}

/**
 * Called by IInternalSessionControl::OnVRDEServerChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onVRDEServerChange(BOOL aRestart)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger VRDE server changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        /* Serialize. */
        if (mfVRDEChangeInProcess)
            mfVRDEChangePending = true;
        else
        {
            do {
                mfVRDEChangeInProcess = true;
                mfVRDEChangePending = false;

                if (    mVRDEServer
                    &&  (   mMachineState == MachineState_Running
                         || mMachineState == MachineState_Teleporting
                         || mMachineState == MachineState_LiveSnapshotting
                         || mMachineState == MachineState_Paused
                         )
                   )
                {
                    BOOL vrdpEnabled = FALSE;

                    rc = mVRDEServer->COMGETTER(Enabled)(&vrdpEnabled);
                    ComAssertComRCRetRC(rc);

                    if (aRestart)
                    {
                        /* VRDP server may call this Console object back from other threads (VRDP INPUT or OUTPUT). */
                        alock.release();

                        if (vrdpEnabled)
                        {
                            // If there was no VRDP server started the 'stop' will do nothing.
                            // However if a server was started and this notification was called,
                            // we have to restart the server.
                            mConsoleVRDPServer->Stop();

                            int vrc = mConsoleVRDPServer->Launch();
                            if (vrc != VINF_SUCCESS)
                            {
                                Utf8Str errMsg = VRDPServerErrorToMsg(vrc);
                                rc = setError(E_FAIL, errMsg.c_str());
                            }
                            else
                                mConsoleVRDPServer->EnableConnections();
                        }
                        else
                            mConsoleVRDPServer->Stop();

                        alock.acquire();
                    }
                }
                else
                    rc = i_setInvalidMachineStateError();

                mfVRDEChangeInProcess = false;
            } while (mfVRDEChangePending && SUCCEEDED(rc));
        }

        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireVRDEServerChangedEvent(mEventSource);
    }

    return rc;
}

void Console::i_onVRDEServerInfoChange()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    fireVRDEServerInfoChangedEvent(mEventSource);
}

HRESULT Console::i_sendACPIMonitorHotPlugEvent()
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mMachineState != MachineState_Running
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_LiveSnapshotting)
        return i_setInvalidMachineStateError();

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    // no need to release lock, as there are no cross-thread callbacks

    /* get the acpi device interface and press the sleep button. */
    PPDMIBASE pBase;
    int vrc = PDMR3QueryDeviceLun(ptrVM.rawUVM(), "acpi", 0, 0, &pBase);
    if (RT_SUCCESS(vrc))
    {
        Assert(pBase);
        PPDMIACPIPORT pPort = PDMIBASE_QUERY_INTERFACE(pBase, PDMIACPIPORT);
        if (pPort)
            vrc = pPort->pfnMonitorHotPlugEvent(pPort);
        else
            vrc = VERR_PDM_MISSING_INTERFACE;
    }

    HRESULT rc = RT_SUCCESS(vrc) ? S_OK :
        setError(VBOX_E_PDM_ERROR,
            tr("Sending monitor hot-plug event failed (%Rrc)"),
            vrc);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

HRESULT Console::i_onVideoCaptureChange()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

#ifdef VBOX_WITH_VIDEOREC
    /* Don't trigger video capture changes if the VM isn't running. */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (mDisplay)
        {
            Display *pDisplay = mDisplay;
            AssertPtr(pDisplay);

            BOOL fEnabled;
            rc = mMachine->COMGETTER(VideoCaptureEnabled)(&fEnabled);
            AssertComRCReturnRC(rc);

            if (fEnabled)
            {
	            const PVIDEORECCFG pCfg = pDisplay->i_videoRecGetConfig();
# ifdef VBOX_WITH_AUDIO_VIDEOREC
	            const unsigned     uLUN = pCfg->Audio.uLUN; /* Get the currently configured LUN. */
# else
	            const unsigned     uLUN = 0;
# endif
	            /* Release lock because the call scheduled on EMT may also try to take it. */
	            alock.release();

	            int vrc = VMR3ReqCallWaitU(ptrVM.rawUVM(), VMCPUID_ANY /*idDstCpu*/,
	                                       (PFNRT)Display::i_videoRecConfigure, 4,
	                                       pDisplay, pCfg, true /* fAttachDetach */, &uLUN);
	            if (RT_SUCCESS(vrc))
	            {
	                /* Make sure to acquire the lock again after we're done running in EMT. */
	                alock.acquire();

                    vrc = mDisplay->i_videoRecStart();
                    if (RT_FAILURE(vrc))
                        rc = setError(E_FAIL, tr("Unable to start video capturing (%Rrc)"), vrc);
	            }
	            else
	                rc = setError(E_FAIL, tr("Unable to set screens for capturing (%Rrc)"), vrc);
	    	}
	    	else
	    	    mDisplay->i_videoRecStop();
        }

        ptrVM.release();
    }
#endif /* VBOX_WITH_VIDEOREC */

    /* Notify console callbacks on success. */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireVideoCaptureChangedEvent(mEventSource);
    }

    return rc;
}

/**
 * Called by IInternalSessionControl::OnUSBControllerChange().
 */
HRESULT Console::i_onUSBControllerChange()
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    fireUSBControllerChangedEvent(mEventSource);

    return S_OK;
}

/**
 * Called by IInternalSessionControl::OnSharedFolderChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onSharedFolderChange(BOOL aGlobal)
{
    LogFlowThisFunc(("aGlobal=%RTbool\n", aGlobal));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = i_fetchSharedFolders(aGlobal);

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireSharedFolderChangedEvent(mEventSource, aGlobal ? (Scope_T)Scope_Global : (Scope_T)Scope_Machine);
    }

    return rc;
}

/**
 * Called by IInternalSessionControl::OnUSBDeviceAttach() or locally by
 * processRemoteUSBDevices() after IInternalMachineControl::RunUSBDeviceFilters()
 * returns TRUE for a given remote USB device.
 *
 * @return S_OK if the device was attached to the VM.
 * @return failure if not attached.
 *
 * @param aDevice       The device in question.
 * @param aError        Error information.
 * @param aMaskedIfs    The interfaces to hide from the guest.
 * @param aCaptureFilename File name where to store the USB traffic.
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onUSBDeviceAttach(IUSBDevice *aDevice, IVirtualBoxErrorInfo *aError, ULONG aMaskedIfs,
                                     const Utf8Str &aCaptureFilename)
{
#ifdef VBOX_WITH_USB
    LogFlowThisFunc(("aDevice=%p aError=%p\n", aDevice, aError));

    AutoCaller autoCaller(this);
    ComAssertComRCRetRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Get the VM pointer (we don't need error info, since it's a callback). */
    SafeVMPtrQuiet ptrVM(this);
    if (!ptrVM.isOk())
    {
        /* The VM may be no more operational when this message arrives
         * (e.g. it may be Saving or Stopping or just PoweredOff) --
         * autoVMCaller.rc() will return a failure in this case. */
        LogFlowThisFunc(("Attach request ignored (mMachineState=%d).\n",
                          mMachineState));
        return ptrVM.rc();
    }

    if (aError != NULL)
    {
        /* notify callbacks about the error */
        alock.release();
        i_onUSBDeviceStateChange(aDevice, true /* aAttached */, aError);
        return S_OK;
    }

    /* Don't proceed unless there's at least one USB hub. */
    if (!PDMR3UsbHasHub(ptrVM.rawUVM()))
    {
        LogFlowThisFunc(("Attach request ignored (no USB controller).\n"));
        return E_FAIL;
    }

    alock.release();
    HRESULT rc = i_attachUSBDevice(aDevice, aMaskedIfs, aCaptureFilename);
    if (FAILED(rc))
    {
        /* take the current error info */
        com::ErrorInfoKeeper eik;
        /* the error must be a VirtualBoxErrorInfo instance */
        ComPtr<IVirtualBoxErrorInfo> pError = eik.takeError();
        Assert(!pError.isNull());
        if (!pError.isNull())
        {
            /* notify callbacks about the error */
            i_onUSBDeviceStateChange(aDevice, true /* aAttached */, pError);
        }
    }

    return rc;

#else   /* !VBOX_WITH_USB */
    return E_FAIL;
#endif  /* !VBOX_WITH_USB */
}

/**
 * Called by IInternalSessionControl::OnUSBDeviceDetach() and locally by
 * processRemoteUSBDevices().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onUSBDeviceDetach(IN_BSTR aId,
                                   IVirtualBoxErrorInfo *aError)
{
#ifdef VBOX_WITH_USB
    Guid Uuid(aId);
    LogFlowThisFunc(("aId={%RTuuid} aError=%p\n", Uuid.raw(), aError));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Find the device. */
    ComObjPtr<OUSBDevice> pUSBDevice;
    USBDeviceList::iterator it = mUSBDevices.begin();
    while (it != mUSBDevices.end())
    {
        LogFlowThisFunc(("it={%RTuuid}\n", (*it)->i_id().raw()));
        if ((*it)->i_id() == Uuid)
        {
            pUSBDevice = *it;
            break;
        }
        ++it;
    }


    if (pUSBDevice.isNull())
    {
        LogFlowThisFunc(("USB device not found.\n"));

        /* The VM may be no more operational when this message arrives
         * (e.g. it may be Saving or Stopping or just PoweredOff). Use
         * AutoVMCaller to detect it -- AutoVMCaller::rc() will return a
         * failure in this case. */

        AutoVMCallerQuiet autoVMCaller(this);
        if (FAILED(autoVMCaller.rc()))
        {
            LogFlowThisFunc(("Detach request ignored (mMachineState=%d).\n",
                              mMachineState));
            return autoVMCaller.rc();
        }

        /* the device must be in the list otherwise */
        AssertFailedReturn(E_FAIL);
    }

    if (aError != NULL)
    {
        /* notify callback about an error */
        alock.release();
        i_onUSBDeviceStateChange(pUSBDevice, false /* aAttached */, aError);
        return S_OK;
    }

    /* Remove the device from the collection, it is re-added below for failures */
    mUSBDevices.erase(it);

    alock.release();
    HRESULT rc = i_detachUSBDevice(pUSBDevice);
    if (FAILED(rc))
    {
        /* Re-add the device to the collection */
        alock.acquire();
        mUSBDevices.push_back(pUSBDevice);
        alock.release();
        /* take the current error info */
        com::ErrorInfoKeeper eik;
        /* the error must be a VirtualBoxErrorInfo instance */
        ComPtr<IVirtualBoxErrorInfo> pError = eik.takeError();
        Assert(!pError.isNull());
        if (!pError.isNull())
        {
            /* notify callbacks about the error */
            i_onUSBDeviceStateChange(pUSBDevice, false /* aAttached */, pError);
        }
    }

    return rc;

#else   /* !VBOX_WITH_USB */
    return E_FAIL;
#endif  /* !VBOX_WITH_USB */
}

/**
 * Called by IInternalSessionControl::OnBandwidthGroupChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onBandwidthGroupChange(IBandwidthGroup *aBandwidthGroup)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    /* don't trigger bandwidth group changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (   mMachineState == MachineState_Running
            || mMachineState == MachineState_Teleporting
            || mMachineState == MachineState_LiveSnapshotting
            )
        {
            /* No need to call in the EMT thread. */
            Bstr strName;
            rc = aBandwidthGroup->COMGETTER(Name)(strName.asOutParam());
            if (SUCCEEDED(rc))
            {
                LONG64 cMax;
                rc = aBandwidthGroup->COMGETTER(MaxBytesPerSec)(&cMax);
                if (SUCCEEDED(rc))
                {
                    BandwidthGroupType_T enmType;
                    rc = aBandwidthGroup->COMGETTER(Type)(&enmType);
                    if (SUCCEEDED(rc))
                    {
                        int vrc = VINF_SUCCESS;
                        if (enmType == BandwidthGroupType_Disk)
                            vrc = PDMR3AsyncCompletionBwMgrSetMaxForFile(ptrVM.rawUVM(), Utf8Str(strName).c_str(), (uint32_t)cMax);
#ifdef VBOX_WITH_NETSHAPER
                        else if (enmType == BandwidthGroupType_Network)
                            vrc = PDMR3NsBwGroupSetLimit(ptrVM.rawUVM(), Utf8Str(strName).c_str(), cMax);
                        else
                            rc = E_NOTIMPL;
#endif
                        AssertRC(vrc);
                    }
                }
            }
        }
        else
            rc = i_setInvalidMachineStateError();
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
    {
        alock.release();
        fireBandwidthGroupChangedEvent(mEventSource, aBandwidthGroup);
    }

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

/**
 * Called by IInternalSessionControl::OnStorageDeviceChange().
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_onStorageDeviceChange(IMediumAttachment *aMediumAttachment, BOOL aRemove, BOOL aSilent)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    HRESULT rc = S_OK;

    /* don't trigger medium changes if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        if (aRemove)
            rc = i_doStorageDeviceDetach(aMediumAttachment, ptrVM.rawUVM(), RT_BOOL(aSilent));
        else
            rc = i_doStorageDeviceAttach(aMediumAttachment, ptrVM.rawUVM(), RT_BOOL(aSilent));
        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(rc))
        fireStorageDeviceChangedEvent(mEventSource, aMediumAttachment, aRemove, aSilent);

    LogFlowThisFunc(("Leaving rc=%#x\n", rc));
    return rc;
}

HRESULT Console::i_onExtraDataChange(IN_BSTR aMachineId, IN_BSTR aKey, IN_BSTR aVal)
{
    LogFlowThisFunc(("\n"));

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    if (!aMachineId)
        return S_OK;

    HRESULT hrc = S_OK;
    Bstr idMachine(aMachineId);
    if (   FAILED(hrc)
        || idMachine != i_getId())
        return hrc;

    /* don't do anything if the VM isn't running */
    SafeVMPtrQuiet ptrVM(this);
    if (ptrVM.isOk())
    {
        Bstr strKey(aKey);
        Bstr strVal(aVal);

        if (strKey == "VBoxInternal2/TurnResetIntoPowerOff")
        {
            int vrc = VMR3SetPowerOffInsteadOfReset(ptrVM.rawUVM(), strVal == "1");
            AssertRC(vrc);
        }

        ptrVM.release();
    }

    /* notify console callbacks on success */
    if (SUCCEEDED(hrc))
        fireExtraDataChangedEvent(mEventSource, aMachineId, aKey, aVal);

    LogFlowThisFunc(("Leaving hrc=%#x\n", hrc));
    return hrc;
}

/**
 * @note Temporarily locks this object for writing.
 */
HRESULT Console::i_getGuestProperty(const Utf8Str &aName, Utf8Str *aValue, LONG64 *aTimestamp, Utf8Str *aFlags)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else  /* VBOX_WITH_GUEST_PROPS */
    if (!RT_VALID_PTR(aValue))
         return E_POINTER;
    if (aTimestamp != NULL && !RT_VALID_PTR(aTimestamp))
        return E_POINTER;
    if (aFlags != NULL && !RT_VALID_PTR(aFlags))
        return E_POINTER;

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* protect mpUVM (if not NULL) */
    SafeVMPtrQuiet ptrVM(this);
    if (FAILED(ptrVM.rc()))
        return ptrVM.rc();

    /* Note: validity of mVMMDev which is bound to uninit() is guaranteed by
     * ptrVM, so there is no need to hold a lock of this */

    HRESULT rc = E_UNEXPECTED;
    using namespace guestProp;

    try
    {
        VBOXHGCMSVCPARM parm[4];
        char szBuffer[MAX_VALUE_LEN + MAX_FLAGS_LEN];

        parm[0].type = VBOX_HGCM_SVC_PARM_PTR;
        parm[0].u.pointer.addr = (void*)aName.c_str();
        parm[0].u.pointer.size = (uint32_t)aName.length() + 1; /* The + 1 is the null terminator */

        parm[1].type = VBOX_HGCM_SVC_PARM_PTR;
        parm[1].u.pointer.addr = szBuffer;
        parm[1].u.pointer.size = sizeof(szBuffer);

        parm[2].type = VBOX_HGCM_SVC_PARM_64BIT;
        parm[2].u.uint64 = 0;

        parm[3].type = VBOX_HGCM_SVC_PARM_32BIT;
        parm[3].u.uint32 = 0;

        int vrc = m_pVMMDev->hgcmHostCall("VBoxGuestPropSvc", GET_PROP_HOST,
                                          4, &parm[0]);
        /* The returned string should never be able to be greater than our buffer */
        AssertLogRel(vrc != VERR_BUFFER_OVERFLOW);
        AssertLogRel(RT_FAILURE(vrc) || parm[2].type == VBOX_HGCM_SVC_PARM_64BIT);
        if (RT_SUCCESS(vrc))
        {
            *aValue = szBuffer;

            if (aTimestamp)
                *aTimestamp = parm[2].u.uint64;

            if (aFlags)
                *aFlags = &szBuffer[strlen(szBuffer) + 1];

            rc = S_OK;
        }
        else if (vrc == VERR_NOT_FOUND)
        {
            *aValue = "";
            rc = S_OK;
        }
        else
            rc = setError(VBOX_E_IPRT_ERROR,
                          tr("The VBoxGuestPropSvc service call failed with the error %Rrc"),
                          vrc);
    }
    catch(std::bad_alloc & /*e*/)
    {
        rc = E_OUTOFMEMORY;
    }

    return rc;
#endif /* VBOX_WITH_GUEST_PROPS */
}

/**
 * @note Temporarily locks this object for writing.
 */
HRESULT Console::i_setGuestProperty(const Utf8Str &aName, const Utf8Str &aValue, const Utf8Str &aFlags)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else /* VBOX_WITH_GUEST_PROPS */

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* protect mpUVM (if not NULL) */
    SafeVMPtrQuiet ptrVM(this);
    if (FAILED(ptrVM.rc()))
        return ptrVM.rc();

    /* Note: validity of mVMMDev which is bound to uninit() is guaranteed by
     * ptrVM, so there is no need to hold a lock of this */

    using namespace guestProp;

    VBOXHGCMSVCPARM parm[3];

    parm[0].type = VBOX_HGCM_SVC_PARM_PTR;
    parm[0].u.pointer.addr = (void*)aName.c_str();
    parm[0].u.pointer.size = (uint32_t)aName.length() + 1; /* The + 1 is the null terminator */

    parm[1].type = VBOX_HGCM_SVC_PARM_PTR;
    parm[1].u.pointer.addr = (void *)aValue.c_str();
    parm[1].u.pointer.size = (uint32_t)aValue.length() + 1; /* The + 1 is the null terminator */

    int vrc;
    if (aFlags.isEmpty())
    {
        vrc = m_pVMMDev->hgcmHostCall("VBoxGuestPropSvc", SET_PROP_VALUE_HOST,
                                    2, &parm[0]);
    }
    else
    {
        parm[2].type = VBOX_HGCM_SVC_PARM_PTR;
        parm[2].u.pointer.addr = (void*)aFlags.c_str();
        parm[2].u.pointer.size = (uint32_t)aFlags.length() + 1; /* The + 1 is the null terminator */

        vrc = m_pVMMDev->hgcmHostCall("VBoxGuestPropSvc", SET_PROP_HOST,
                                      3, &parm[0]);
    }

    HRESULT hrc = S_OK;
    if (RT_FAILURE(vrc))
        hrc = setError(VBOX_E_IPRT_ERROR, tr("The VBoxGuestPropSvc service call failed with the error %Rrc"), vrc);
    return hrc;
#endif /* VBOX_WITH_GUEST_PROPS */
}

HRESULT Console::i_deleteGuestProperty(const Utf8Str &aName)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else /* VBOX_WITH_GUEST_PROPS */

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* protect mpUVM (if not NULL) */
    SafeVMPtrQuiet ptrVM(this);
    if (FAILED(ptrVM.rc()))
        return ptrVM.rc();

    /* Note: validity of mVMMDev which is bound to uninit() is guaranteed by
     * ptrVM, so there is no need to hold a lock of this */

    using namespace guestProp;

    VBOXHGCMSVCPARM parm[1];

    parm[0].type = VBOX_HGCM_SVC_PARM_PTR;
    parm[0].u.pointer.addr = (void*)aName.c_str();
    parm[0].u.pointer.size = (uint32_t)aName.length() + 1; /* The + 1 is the null terminator */

    int vrc = m_pVMMDev->hgcmHostCall("VBoxGuestPropSvc", DEL_PROP_HOST,
                                      1, &parm[0]);

    HRESULT hrc = S_OK;
    if (RT_FAILURE(vrc))
        hrc = setError(VBOX_E_IPRT_ERROR, tr("The VBoxGuestPropSvc service call failed with the error %Rrc"), vrc);
    return hrc;
#endif /* VBOX_WITH_GUEST_PROPS */
}

/**
 * @note Temporarily locks this object for writing.
 */
HRESULT Console::i_enumerateGuestProperties(const Utf8Str &aPatterns,
                                            std::vector<Utf8Str> &aNames,
                                            std::vector<Utf8Str> &aValues,
                                            std::vector<LONG64>  &aTimestamps,
                                            std::vector<Utf8Str> &aFlags)
{
#ifndef VBOX_WITH_GUEST_PROPS
    ReturnComNotImplemented();
#else /* VBOX_WITH_GUEST_PROPS */

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* protect mpUVM (if not NULL) */
    AutoVMCallerWeak autoVMCaller(this);
    if (FAILED(autoVMCaller.rc()))
        return autoVMCaller.rc();

    /* Note: validity of mVMMDev which is bound to uninit() is guaranteed by
     * autoVMCaller, so there is no need to hold a lock of this */

    return i_doEnumerateGuestProperties(aPatterns, aNames, aValues, aTimestamps, aFlags);
#endif /* VBOX_WITH_GUEST_PROPS */
}


/*
 * Internal: helper function for connecting progress reporting
 */
static DECLCALLBACK(int) onlineMergeMediumProgress(void *pvUser, unsigned uPercentage)
{
    HRESULT rc = S_OK;
    IProgress *pProgress = static_cast<IProgress *>(pvUser);
    if (pProgress)
        rc = pProgress->SetCurrentOperationProgress(uPercentage);
    return SUCCEEDED(rc) ? VINF_SUCCESS : VERR_GENERAL_FAILURE;
}

/**
 * @note Temporarily locks this object for writing. bird: And/or reading?
 */
HRESULT Console::i_onlineMergeMedium(IMediumAttachment *aMediumAttachment,
                                     ULONG aSourceIdx, ULONG aTargetIdx,
                                     IProgress *aProgress)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    HRESULT rc = S_OK;
    int vrc = VINF_SUCCESS;

    /* Get the VM - must be done before the read-locking. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    /* We will need to release the lock before doing the actual merge */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* paranoia - we don't want merges to happen while teleporting etc. */
    switch (mMachineState)
    {
        case MachineState_DeletingSnapshotOnline:
        case MachineState_DeletingSnapshotPaused:
            break;

        default:
            return i_setInvalidMachineStateError();
    }

    /** @todo AssertComRC -> AssertComRCReturn! Could potentially end up
     *        using uninitialized variables here. */
    BOOL fBuiltinIOCache;
    rc = mMachine->COMGETTER(IOCacheEnabled)(&fBuiltinIOCache);
    AssertComRC(rc);
    SafeIfaceArray<IStorageController> ctrls;
    rc = mMachine->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(ctrls));
    AssertComRC(rc);
    LONG lDev;
    rc = aMediumAttachment->COMGETTER(Device)(&lDev);
    AssertComRC(rc);
    LONG lPort;
    rc = aMediumAttachment->COMGETTER(Port)(&lPort);
    AssertComRC(rc);
    IMedium *pMedium;
    rc = aMediumAttachment->COMGETTER(Medium)(&pMedium);
    AssertComRC(rc);
    Bstr mediumLocation;
    if (pMedium)
    {
        rc = pMedium->COMGETTER(Location)(mediumLocation.asOutParam());
        AssertComRC(rc);
    }

    Bstr attCtrlName;
    rc = aMediumAttachment->COMGETTER(Controller)(attCtrlName.asOutParam());
    AssertComRC(rc);
    ComPtr<IStorageController> pStorageController;
    for (size_t i = 0; i < ctrls.size(); ++i)
    {
        Bstr ctrlName;
        rc = ctrls[i]->COMGETTER(Name)(ctrlName.asOutParam());
        AssertComRC(rc);
        if (attCtrlName == ctrlName)
        {
            pStorageController = ctrls[i];
            break;
        }
    }
    if (pStorageController.isNull())
        return setError(E_FAIL,
                        tr("Could not find storage controller '%ls'"),
                        attCtrlName.raw());

    StorageControllerType_T enmCtrlType;
    rc = pStorageController->COMGETTER(ControllerType)(&enmCtrlType);
    AssertComRC(rc);
    const char *pcszDevice = i_storageControllerTypeToStr(enmCtrlType);

    StorageBus_T enmBus;
    rc = pStorageController->COMGETTER(Bus)(&enmBus);
    AssertComRC(rc);
    ULONG uInstance;
    rc = pStorageController->COMGETTER(Instance)(&uInstance);
    AssertComRC(rc);
    BOOL fUseHostIOCache;
    rc = pStorageController->COMGETTER(UseHostIOCache)(&fUseHostIOCache);
    AssertComRC(rc);

    unsigned uLUN;
    rc = Console::i_storageBusPortDeviceToLun(enmBus, lPort, lDev, uLUN);
    AssertComRCReturnRC(rc);

    Assert(mMachineState == MachineState_DeletingSnapshotOnline);

    /* Pause the VM, as it might have pending IO on this drive */
    bool fResume = false;
    rc = i_suspendBeforeConfigChange(ptrVM.rawUVM(), &alock, &fResume);
    if (FAILED(rc))
        return rc;

    bool fInsertDiskIntegrityDrv = false;
    Bstr strDiskIntegrityFlag;
    rc = mMachine->GetExtraData(Bstr("VBoxInternal2/EnableDiskIntegrityDriver").raw(),
                                strDiskIntegrityFlag.asOutParam());
    if (   rc   == S_OK
        && strDiskIntegrityFlag == "1")
        fInsertDiskIntegrityDrv = true;

    alock.release();
    vrc = VMR3ReqCallWaitU(ptrVM.rawUVM(), VMCPUID_ANY,
                           (PFNRT)i_reconfigureMediumAttachment, 14,
                           this, ptrVM.rawUVM(), pcszDevice, uInstance, enmBus, fUseHostIOCache,
                           fBuiltinIOCache, fInsertDiskIntegrityDrv, true /* fSetupMerge */,
                           aSourceIdx, aTargetIdx, aMediumAttachment, mMachineState, &rc);
    /* error handling is after resuming the VM */

    if (fResume)
        i_resumeAfterConfigChange(ptrVM.rawUVM());

    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("%Rrc"), vrc);
    if (FAILED(rc))
        return rc;

    PPDMIBASE pIBase = NULL;
    PPDMIMEDIA pIMedium = NULL;
    vrc = PDMR3QueryDriverOnLun(ptrVM.rawUVM(), pcszDevice, uInstance, uLUN, "VD", &pIBase);
    if (RT_SUCCESS(vrc))
    {
        if (pIBase)
        {
            pIMedium = (PPDMIMEDIA)pIBase->pfnQueryInterface(pIBase, PDMIMEDIA_IID);
            if (!pIMedium)
                return setError(E_FAIL, tr("could not query medium interface of controller"));
        }
        else
            return setError(E_FAIL, tr("could not query base interface of controller"));
    }

    /* Finally trigger the merge. */
    vrc = pIMedium->pfnMerge(pIMedium, onlineMergeMediumProgress, aProgress);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("Failed to perform an online medium merge (%Rrc)"), vrc);

    alock.acquire();
    /* Pause the VM, as it might have pending IO on this drive */
    rc = i_suspendBeforeConfigChange(ptrVM.rawUVM(), &alock, &fResume);
    if (FAILED(rc))
        return rc;
    alock.release();

    /* Update medium chain and state now, so that the VM can continue. */
    rc = mControl->FinishOnlineMergeMedium();

    vrc = VMR3ReqCallWaitU(ptrVM.rawUVM(), VMCPUID_ANY,
                           (PFNRT)i_reconfigureMediumAttachment, 14,
                           this, ptrVM.rawUVM(), pcszDevice, uInstance, enmBus, fUseHostIOCache,
                           fBuiltinIOCache, fInsertDiskIntegrityDrv, false /* fSetupMerge */,
                           0 /* uMergeSource */, 0 /* uMergeTarget */, aMediumAttachment,
                           mMachineState, &rc);
    /* error handling is after resuming the VM */

    if (fResume)
        i_resumeAfterConfigChange(ptrVM.rawUVM());

    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("%Rrc"), vrc);
    if (FAILED(rc))
        return rc;

    return rc;
}

HRESULT Console::i_reconfigureMediumAttachments(const std::vector<ComPtr<IMediumAttachment> > &aAttachments)
{
    HRESULT rc = S_OK;

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (size_t i = 0; i < aAttachments.size(); ++i)
    {
        ComPtr<IStorageController> pStorageController;
        Bstr controllerName;
        ULONG lInstance;
        StorageControllerType_T enmController;
        StorageBus_T enmBus;
        BOOL fUseHostIOCache;

        /*
         * We could pass the objects, but then EMT would have to do lots of
         * IPC (to VBoxSVC) which takes a significant amount of time.
         * Better query needed values here and pass them.
         */
        rc = aAttachments[i]->COMGETTER(Controller)(controllerName.asOutParam());
        if (FAILED(rc))
            throw rc;

        rc = mMachine->GetStorageControllerByName(controllerName.raw(),
                                                  pStorageController.asOutParam());
        if (FAILED(rc))
            throw rc;

        rc = pStorageController->COMGETTER(ControllerType)(&enmController);
        if (FAILED(rc))
            throw rc;
        rc = pStorageController->COMGETTER(Instance)(&lInstance);
        if (FAILED(rc))
            throw rc;
        rc = pStorageController->COMGETTER(Bus)(&enmBus);
        if (FAILED(rc))
            throw rc;
        rc = pStorageController->COMGETTER(UseHostIOCache)(&fUseHostIOCache);
        if (FAILED(rc))
            throw rc;

        const char *pcszDevice = i_storageControllerTypeToStr(enmController);

        BOOL fBuiltinIOCache;
        rc = mMachine->COMGETTER(IOCacheEnabled)(&fBuiltinIOCache);
        if (FAILED(rc))
            throw rc;

        bool fInsertDiskIntegrityDrv = false;
        Bstr strDiskIntegrityFlag;
        rc = mMachine->GetExtraData(Bstr("VBoxInternal2/EnableDiskIntegrityDriver").raw(),
                                    strDiskIntegrityFlag.asOutParam());
        if (   rc   == S_OK
            && strDiskIntegrityFlag == "1")
            fInsertDiskIntegrityDrv = true;

        alock.release();

        IMediumAttachment *pAttachment = aAttachments[i];
        int vrc = VMR3ReqCallWaitU(ptrVM.rawUVM(), VMCPUID_ANY,
                                   (PFNRT)i_reconfigureMediumAttachment, 14,
                                   this, ptrVM.rawUVM(), pcszDevice, lInstance, enmBus, fUseHostIOCache,
                                   fBuiltinIOCache, fInsertDiskIntegrityDrv,
                                   false /* fSetupMerge */, 0 /* uMergeSource */, 0 /* uMergeTarget */,
                                   pAttachment, mMachineState, &rc);
        if (RT_FAILURE(vrc))
            throw setError(E_FAIL, tr("%Rrc"), vrc);
        if (FAILED(rc))
            throw rc;

        alock.acquire();
    }

    return rc;
}


/**
 * Load an HGCM service.
 *
 * Main purpose of this method is to allow extension packs to load HGCM
 * service modules, which they can't, because the HGCM functionality lives
 * in module VBoxC (and ConsoleImpl.cpp is part of it and thus can call it).
 * Extension modules must not link directly against VBoxC, (XP)COM is
 * handling this.
 */
int Console::i_hgcmLoadService(const char *pszServiceLibrary, const char *pszServiceName)
{
    /* Everyone seems to delegate all HGCM calls to VMMDev, so stick to this
     * convention. Adds one level of indirection for no obvious reason. */
    AssertPtrReturn(m_pVMMDev, VERR_INVALID_STATE);
    return m_pVMMDev->hgcmLoadService(pszServiceLibrary, pszServiceName);
}

/**
 * Merely passes the call to Guest::enableVMMStatistics().
 */
void Console::i_enableVMMStatistics(BOOL aEnable)
{
    if (mGuest)
        mGuest->i_enableVMMStatistics(aEnable);
}

/**
 * Worker for Console::Pause and internal entry point for pausing a VM for
 * a specific reason.
 */
HRESULT Console::i_pause(Reason_T aReason)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    switch (mMachineState)
    {
        case MachineState_Running:
        case MachineState_Teleporting:
        case MachineState_LiveSnapshotting:
            break;

        case MachineState_Paused:
        case MachineState_TeleportingPausedVM:
        case MachineState_OnlineSnapshotting:
            /* Remove any keys which are supposed to be removed on a suspend. */
            if (   aReason == Reason_HostSuspend
                || aReason == Reason_HostBatteryLow)
            {
                i_removeSecretKeysOnSuspend();
                return S_OK;
            }
            return setError(VBOX_E_INVALID_VM_STATE, tr("Already paused"));

        default:
            return i_setInvalidMachineStateError();
    }

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    /* release the lock before a VMR3* call (EMT might wait for it, @bugref{7648})! */
    alock.release();

    LogFlowThisFunc(("Sending PAUSE request...\n"));
    if (aReason != Reason_Unspecified)
        LogRel(("Pausing VM execution, reason '%s'\n", Global::stringifyReason(aReason)));

    /** @todo r=klaus make use of aReason */
    VMSUSPENDREASON enmReason = VMSUSPENDREASON_USER;
    if (aReason == Reason_HostSuspend)
        enmReason = VMSUSPENDREASON_HOST_SUSPEND;
    else if (aReason == Reason_HostBatteryLow)
        enmReason = VMSUSPENDREASON_HOST_BATTERY_LOW;
    int vrc = VMR3Suspend(ptrVM.rawUVM(), enmReason);

    HRESULT hrc = S_OK;
    if (RT_FAILURE(vrc))
        hrc = setError(VBOX_E_VM_ERROR, tr("Could not suspend the machine execution (%Rrc)"), vrc);
    else if (   aReason == Reason_HostSuspend
             || aReason == Reason_HostBatteryLow)
    {
        alock.acquire();
        i_removeSecretKeysOnSuspend();
    }

    LogFlowThisFunc(("hrc=%Rhrc\n", hrc));
    LogFlowThisFuncLeave();
    return hrc;
}

/**
 * Worker for Console::Resume and internal entry point for resuming a VM for
 * a specific reason.
 */
HRESULT Console::i_resume(Reason_T aReason, AutoWriteLock &alock)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    /* release the lock before a VMR3* call (EMT might wait for it, @bugref{7648})! */
    alock.release();

    LogFlowThisFunc(("Sending RESUME request...\n"));
    if (aReason != Reason_Unspecified)
        LogRel(("Resuming VM execution, reason '%s'\n", Global::stringifyReason(aReason)));

    int vrc;
    if (VMR3GetStateU(ptrVM.rawUVM()) == VMSTATE_CREATED)
    {
#ifdef VBOX_WITH_EXTPACK
        vrc = mptrExtPackManager->i_callAllVmPowerOnHooks(this, VMR3GetVM(ptrVM.rawUVM()));
#else
        vrc = VINF_SUCCESS;
#endif
        if (RT_SUCCESS(vrc))
            vrc = VMR3PowerOn(ptrVM.rawUVM()); /* (PowerUpPaused) */
    }
    else
    {
        VMRESUMEREASON enmReason;
        if (aReason == Reason_HostResume)
        {
            /*
             * Host resume may be called multiple times successively. We don't want to VMR3Resume->vmR3Resume->vmR3TrySetState()
             * to assert on us, hence check for the VM state here and bail if it's not in the 'suspended' state.
             * See @bugref{3495}.
             *
             * Also, don't resume the VM through a host-resume unless it was suspended due to a host-suspend.
             */
            if (VMR3GetStateU(ptrVM.rawUVM()) != VMSTATE_SUSPENDED)
            {
                LogRel(("Ignoring VM resume request, VM is currently not suspended\n"));
                return S_OK;
            }
            if (VMR3GetSuspendReason(ptrVM.rawUVM()) != VMSUSPENDREASON_HOST_SUSPEND)
            {
                LogRel(("Ignoring VM resume request, VM was not suspended due to host-suspend\n"));
                return S_OK;
            }

            enmReason = VMRESUMEREASON_HOST_RESUME;
        }
        else
        {
            /*
             * Any other reason to resume the VM throws an error when the VM was suspended due to a host suspend.
             * See @bugref{7836}.
             */
            if (   VMR3GetStateU(ptrVM.rawUVM()) == VMSTATE_SUSPENDED
                && VMR3GetSuspendReason(ptrVM.rawUVM()) == VMSUSPENDREASON_HOST_SUSPEND)
                return setError(VBOX_E_INVALID_VM_STATE, tr("VM is paused due to host power management"));

            enmReason = aReason == Reason_Snapshot ? VMRESUMEREASON_STATE_SAVED : VMRESUMEREASON_USER;
        }

        // for snapshots: no state change callback, VBoxSVC does everything
        if (aReason == Reason_Snapshot)
            mVMStateChangeCallbackDisabled = true;
        vrc = VMR3Resume(ptrVM.rawUVM(), enmReason);
        if (aReason == Reason_Snapshot)
            mVMStateChangeCallbackDisabled = false;
    }

    HRESULT rc = RT_SUCCESS(vrc) ? S_OK :
        setError(VBOX_E_VM_ERROR,
                 tr("Could not resume the machine execution (%Rrc)"),
                 vrc);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    return rc;
}

/**
 * Internal entry point for saving state of a VM for a specific reason. This
 * method is completely synchronous.
 *
 * The machine state is already set appropriately. It is only changed when
 * saving state actually paused the VM (happens with live snapshots and
 * teleportation), and in this case reflects the now paused variant.
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_saveState(Reason_T aReason, const ComPtr<IProgress> &aProgress,
                             const ComPtr<ISnapshot> &aSnapshot,
                             const Utf8Str &aStateFilePath, bool aPauseVM, bool &aLeftPaused)
{
    LogFlowThisFuncEnter();
    aLeftPaused = false;

    AssertReturn(!aProgress.isNull(), E_INVALIDARG);
    AssertReturn(!aStateFilePath.isEmpty(), E_INVALIDARG);
    Assert(aSnapshot.isNull() || aReason == Reason_Snapshot);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));
    if (   mMachineState != MachineState_Saving
        && mMachineState != MachineState_LiveSnapshotting
        && mMachineState != MachineState_OnlineSnapshotting
        && mMachineState != MachineState_Teleporting
        && mMachineState != MachineState_TeleportingPausedVM)
    {
        return setError(VBOX_E_INVALID_VM_STATE,
            tr("Cannot save the execution state as the machine is not running or paused (machine state: %s)"),
            Global::stringifyMachineState(mMachineState));
    }
    bool fContinueAfterwards = mMachineState != MachineState_Saving;

    Bstr strDisableSaveState;
    mMachine->GetExtraData(Bstr("VBoxInternal2/DisableSaveState").raw(), strDisableSaveState.asOutParam());
    if (strDisableSaveState == "1")
        return setError(VBOX_E_VM_ERROR,
                        tr("Saving the execution state is disabled for this VM"));

    if (aReason != Reason_Unspecified)
        LogRel(("Saving state of VM, reason '%s'\n", Global::stringifyReason(aReason)));

    /* ensure the directory for the saved state file exists */
    {
        Utf8Str dir = aStateFilePath;
        dir.stripFilename();
        if (!RTDirExists(dir.c_str()))
        {
            int vrc = RTDirCreateFullPath(dir.c_str(), 0700);
            if (RT_FAILURE(vrc))
                return setError(VBOX_E_FILE_ERROR,
                                tr("Could not create a directory '%s' to save the state to (%Rrc)"),
                                dir.c_str(), vrc);
        }
    }

    /* Get the VM handle early, we need it in several places. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    bool fPaused = false;
    if (aPauseVM)
    {
        /* release the lock before a VMR3* call (EMT might wait for it, @bugref{7648})! */
        alock.release();
        VMSUSPENDREASON enmReason = VMSUSPENDREASON_USER;
        if (aReason == Reason_HostSuspend)
            enmReason = VMSUSPENDREASON_HOST_SUSPEND;
        else if (aReason == Reason_HostBatteryLow)
            enmReason = VMSUSPENDREASON_HOST_BATTERY_LOW;
        int vrc = VMR3Suspend(ptrVM.rawUVM(), enmReason);
        alock.acquire();

        if (RT_FAILURE(vrc))
            return setError(VBOX_E_VM_ERROR, tr("Could not suspend the machine execution (%Rrc)"), vrc);
        fPaused = true;
    }

    LogFlowFunc(("Saving the state to '%s'...\n", aStateFilePath.c_str()));

    mpVmm2UserMethods->pISnapshot = aSnapshot;
    mptrCancelableProgress = aProgress;
    alock.release();
    int vrc = VMR3Save(ptrVM.rawUVM(),
                       aStateFilePath.c_str(),
                       fContinueAfterwards,
                       Console::i_stateProgressCallback,
                       static_cast<IProgress *>(aProgress),
                       &aLeftPaused);
    alock.acquire();
    mpVmm2UserMethods->pISnapshot = NULL;
    mptrCancelableProgress.setNull();
    if (RT_FAILURE(vrc))
    {
        if (fPaused)
        {
            alock.release();
            VMR3Resume(ptrVM.rawUVM(), VMRESUMEREASON_STATE_RESTORED);
            alock.acquire();
        }
        return setError(E_FAIL, tr("Failed to save the machine state to '%s' (%Rrc)"),
                        aStateFilePath.c_str(), vrc);
    }
    Assert(fContinueAfterwards || !aLeftPaused);

    if (!fContinueAfterwards)
    {
        /*
         * The machine has been successfully saved, so power it down
         * (vmstateChangeCallback() will set state to Saved on success).
         * Note: we release the VM caller, otherwise it will deadlock.
         */
        ptrVM.release();
        alock.release();
        autoCaller.release();
        HRESULT rc = i_powerDown();
        AssertComRC(rc);
        autoCaller.add();
        alock.acquire();
    }
    else
    {
        if (fPaused)
            aLeftPaused = true;
    }

    LogFlowFuncLeave();
    return S_OK;
}

/**
 * Internal entry point for cancelling a VM save state.
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_cancelSaveState()
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    SSMR3Cancel(ptrVM.rawUVM());

    LogFlowFuncLeave();
    return S_OK;
}

#ifdef VBOX_WITH_AUDIO_VIDEOREC
/**
 * Sends audio (frame) data to the display's video capturing routines.
 *
 * @returns HRESULT
 * @param   pvData              Audio data to send.
 * @param   cbData              Size (in bytes) of audio data to send.
 * @param   uDurationMs         Duration (in ms) of audio data.
 */
HRESULT Console::i_audioVideoRecSendAudio(const void *pvData, size_t cbData, uint64_t uDurationMs)
{
    if (mDisplay)
    {
        int rc2 = mDisplay->i_videoRecSendAudio(pvData, cbData, uDurationMs);
        AssertRC(rc2);
    }

    return S_OK;
}
#endif /* VBOX_WITH_AUDIO_VIDEOREC */

/**
 * Gets called by Session::UpdateMachineState()
 * (IInternalSessionControl::updateMachineState()).
 *
 * Must be called only in certain cases (see the implementation).
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_updateMachineState(MachineState_T aMachineState)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(   mMachineState == MachineState_Saving
                 || mMachineState == MachineState_OnlineSnapshotting
                 || mMachineState == MachineState_LiveSnapshotting
                 || mMachineState == MachineState_DeletingSnapshotOnline
                 || mMachineState == MachineState_DeletingSnapshotPaused
                 || aMachineState == MachineState_Saving
                 || aMachineState == MachineState_OnlineSnapshotting
                 || aMachineState == MachineState_LiveSnapshotting
                 || aMachineState == MachineState_DeletingSnapshotOnline
                 || aMachineState == MachineState_DeletingSnapshotPaused
                 , E_FAIL);

    return i_setMachineStateLocally(aMachineState);
}

/**
 * Gets called by Session::COMGETTER(NominalState)()
 * (IInternalSessionControl::getNominalState()).
 *
 * @note Locks this object for reading.
 */
HRESULT Console::i_getNominalState(MachineState_T &aNominalState)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    /* Get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    MachineState_T enmMachineState = MachineState_Null;
    VMSTATE enmVMState = VMR3GetStateU(ptrVM.rawUVM());
    switch (enmVMState)
    {
        case VMSTATE_CREATING:
        case VMSTATE_CREATED:
        case VMSTATE_POWERING_ON:
            enmMachineState = MachineState_Starting;
            break;
        case VMSTATE_LOADING:
            enmMachineState = MachineState_Restoring;
            break;
        case VMSTATE_RESUMING:
        case VMSTATE_SUSPENDING:
        case VMSTATE_SUSPENDING_LS:
        case VMSTATE_SUSPENDING_EXT_LS:
        case VMSTATE_SUSPENDED:
        case VMSTATE_SUSPENDED_LS:
        case VMSTATE_SUSPENDED_EXT_LS:
            enmMachineState = MachineState_Paused;
            break;
        case VMSTATE_RUNNING:
        case VMSTATE_RUNNING_LS:
        case VMSTATE_RUNNING_FT:
        case VMSTATE_RESETTING:
        case VMSTATE_RESETTING_LS:
        case VMSTATE_SOFT_RESETTING:
        case VMSTATE_SOFT_RESETTING_LS:
        case VMSTATE_DEBUGGING:
        case VMSTATE_DEBUGGING_LS:
            enmMachineState = MachineState_Running;
            break;
        case VMSTATE_SAVING:
            enmMachineState = MachineState_Saving;
            break;
        case VMSTATE_POWERING_OFF:
        case VMSTATE_POWERING_OFF_LS:
        case VMSTATE_DESTROYING:
            enmMachineState = MachineState_Stopping;
            break;
        case VMSTATE_OFF:
        case VMSTATE_OFF_LS:
        case VMSTATE_FATAL_ERROR:
        case VMSTATE_FATAL_ERROR_LS:
        case VMSTATE_LOAD_FAILURE:
        case VMSTATE_TERMINATED:
            enmMachineState = MachineState_PoweredOff;
            break;
        case VMSTATE_GURU_MEDITATION:
        case VMSTATE_GURU_MEDITATION_LS:
            enmMachineState = MachineState_Stuck;
            break;
        default:
            AssertMsgFailed(("%s\n", VMR3GetStateName(enmVMState)));
            enmMachineState = MachineState_PoweredOff;
    }
    aNominalState = enmMachineState;

    LogFlowFuncLeave();
    return S_OK;
}

void Console::i_onMousePointerShapeChange(bool fVisible, bool fAlpha,
                                          uint32_t xHot, uint32_t yHot,
                                          uint32_t width, uint32_t height,
                                          const uint8_t *pu8Shape,
                                          uint32_t cbShape)
{
#if 0
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("fVisible=%d, fAlpha=%d, xHot = %d, yHot = %d, width=%d, height=%d, shape=%p\n",
                      fVisible, fAlpha, xHot, yHot, width, height, pShape));
#endif

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    if (!mMouse.isNull())
       mMouse->updateMousePointerShape(fVisible, fAlpha, xHot, yHot, width, height,
                                       pu8Shape, cbShape);

    com::SafeArray<BYTE> shape(cbShape);
    if (pu8Shape)
        memcpy(shape.raw(), pu8Shape, cbShape);
    fireMousePointerShapeChangedEvent(mEventSource, fVisible, fAlpha, xHot, yHot, width, height, ComSafeArrayAsInParam(shape));

#if 0
    LogFlowThisFuncLeave();
#endif
}

void Console::i_onMouseCapabilityChange(BOOL supportsAbsolute, BOOL supportsRelative,
                                        BOOL supportsMT, BOOL needsHostCursor)
{
    LogFlowThisFunc(("supportsAbsolute=%d supportsRelative=%d needsHostCursor=%d\n",
                      supportsAbsolute, supportsRelative, needsHostCursor));

    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    fireMouseCapabilityChangedEvent(mEventSource, supportsAbsolute, supportsRelative, supportsMT, needsHostCursor);
}

void Console::i_onStateChange(MachineState_T machineState)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());
    fireStateChangedEvent(mEventSource, machineState);
}

void Console::i_onAdditionsStateChange()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    fireAdditionsStateChangedEvent(mEventSource);
}

/**
 * @remarks This notification only is for reporting an incompatible
 *          Guest Additions interface, *not* the Guest Additions version!
 *
 *          The user will be notified inside the guest if new Guest
 *          Additions are available (via VBoxTray/VBoxClient).
 */
void Console::i_onAdditionsOutdated()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    /** @todo implement this */
}

void Console::i_onKeyboardLedsChange(bool fNumLock, bool fCapsLock, bool fScrollLock)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    fireKeyboardLedsChangedEvent(mEventSource, fNumLock, fCapsLock, fScrollLock);
}

void Console::i_onUSBDeviceStateChange(IUSBDevice *aDevice, bool aAttached,
                                       IVirtualBoxErrorInfo *aError)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    fireUSBDeviceStateChangedEvent(mEventSource, aDevice, aAttached, aError);
}

void Console::i_onRuntimeError(BOOL aFatal, IN_BSTR aErrorID, IN_BSTR aMessage)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    fireRuntimeErrorEvent(mEventSource, aFatal, aErrorID, aMessage);
}

HRESULT Console::i_onShowWindow(BOOL aCheck, BOOL *aCanShow, LONG64 *aWinId)
{
    AssertReturn(aCanShow, E_POINTER);
    AssertReturn(aWinId, E_POINTER);

    *aCanShow = FALSE;
    *aWinId = 0;

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    VBoxEventDesc evDesc;
    if (aCheck)
    {
        evDesc.init(mEventSource, VBoxEventType_OnCanShowWindow);
        BOOL fDelivered = evDesc.fire(5000); /* Wait up to 5 secs for delivery */
        //Assert(fDelivered);
        if (fDelivered)
        {
            ComPtr<IEvent> pEvent;
            evDesc.getEvent(pEvent.asOutParam());
            // bit clumsy
            ComPtr<ICanShowWindowEvent> pCanShowEvent = pEvent;
            if (pCanShowEvent)
            {
                BOOL fVetoed = FALSE;
                BOOL fApproved = FALSE;
                pCanShowEvent->IsVetoed(&fVetoed);
                pCanShowEvent->IsApproved(&fApproved);
                *aCanShow = fApproved || !fVetoed;
            }
            else
            {
                AssertFailed();
                *aCanShow = TRUE;
            }
        }
        else
            *aCanShow = TRUE;
    }
    else
    {
        evDesc.init(mEventSource, VBoxEventType_OnShowWindow, INT64_C(0));
        BOOL fDelivered = evDesc.fire(5000); /* Wait up to 5 secs for delivery */
        //Assert(fDelivered);
        if (fDelivered)
        {
            ComPtr<IEvent> pEvent;
            evDesc.getEvent(pEvent.asOutParam());
            ComPtr<IShowWindowEvent> pShowEvent = pEvent;
            if (pShowEvent)
            {
                LONG64 iEvWinId = 0;
                pShowEvent->COMGETTER(WinId)(&iEvWinId);
                if (iEvWinId != 0 && *aWinId == 0)
                    *aWinId = iEvWinId;
            }
            else
                AssertFailed();
        }
    }

    return S_OK;
}

// private methods
////////////////////////////////////////////////////////////////////////////////

/**
 * Increases the usage counter of the mpUVM pointer.
 *
 * Guarantees that VMR3Destroy() will not be called on it at least until
 * releaseVMCaller() is called.
 *
 * If this method returns a failure, the caller is not allowed to use mpUVM and
 * may return the failed result code to the upper level. This method sets the
 * extended error info on failure if \a aQuiet is false.
 *
 * Setting \a aQuiet to true is useful for methods that don't want to return
 * the failed result code to the caller when this method fails (e.g. need to
 * silently check for the mpUVM availability).
 *
 * When mpUVM is NULL but \a aAllowNullVM is true, a corresponding error will be
 * returned instead of asserting. Having it false is intended as a sanity check
 * for methods that have checked mMachineState and expect mpUVM *NOT* to be
 * NULL.
 *
 * @param aQuiet       true to suppress setting error info
 * @param aAllowNullVM true to accept mpUVM being NULL and return a failure
 *                     (otherwise this method will assert if mpUVM is NULL)
 *
 * @note Locks this object for writing.
 */
HRESULT Console::i_addVMCaller(bool aQuiet /* = false */,
                               bool aAllowNullVM /* = false */)
{
    RT_NOREF(aAllowNullVM);
    AutoCaller autoCaller(this);
    /** @todo Fix race during console/VM reference destruction, refer @bugref{6318}
     *        comment 25. */
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mVMDestroying)
    {
        /* powerDown() is waiting for all callers to finish */
        return aQuiet ? E_ACCESSDENIED : setError(E_ACCESSDENIED,
            tr("The virtual machine is being powered down"));
    }

    if (mpUVM == NULL)
    {
        Assert(aAllowNullVM == true);

        /* The machine is not powered up */
        return aQuiet ? E_ACCESSDENIED : setError(E_ACCESSDENIED,
            tr("The virtual machine is not powered up"));
    }

    ++mVMCallers;

    return S_OK;
}

/**
 * Decreases the usage counter of the mpUVM pointer.
 *
 * Must always complete the addVMCaller() call after the mpUVM pointer is no
 * more necessary.
 *
 * @note Locks this object for writing.
 */
void Console::i_releaseVMCaller()
{
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturnVoid(mpUVM != NULL);

    Assert(mVMCallers > 0);
    --mVMCallers;

    if (mVMCallers == 0 && mVMDestroying)
    {
        /* inform powerDown() there are no more callers */
        RTSemEventSignal(mVMZeroCallersSem);
    }
}


HRESULT Console::i_safeVMPtrRetainer(PUVM *a_ppUVM, bool a_Quiet)
{
    *a_ppUVM = NULL;

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /*
     * Repeat the checks done by addVMCaller.
     */
    if (mVMDestroying) /* powerDown() is waiting for all callers to finish */
        return a_Quiet
             ? E_ACCESSDENIED
             : setError(E_ACCESSDENIED, tr("The virtual machine is being powered down"));
    PUVM pUVM = mpUVM;
    if (!pUVM)
        return a_Quiet
             ? E_ACCESSDENIED
             : setError(E_ACCESSDENIED, tr("The virtual machine is powered off"));

    /*
     * Retain a reference to the user mode VM handle and get the global handle.
     */
    uint32_t cRefs = VMR3RetainUVM(pUVM);
    if (cRefs == UINT32_MAX)
        return a_Quiet
            ? E_ACCESSDENIED
            : setError(E_ACCESSDENIED, tr("The virtual machine is powered off"));

    /* done */
    *a_ppUVM = pUVM;
    return S_OK;
}

void Console::i_safeVMPtrReleaser(PUVM *a_ppUVM)
{
    if (*a_ppUVM)
        VMR3ReleaseUVM(*a_ppUVM);
    *a_ppUVM = NULL;
}


/**
 * Initialize the release logging facility. In case something
 * goes wrong, there will be no release logging. Maybe in the future
 * we can add some logic to use different file names in this case.
 * Note that the logic must be in sync with Machine::DeleteSettings().
 */
HRESULT Console::i_consoleInitReleaseLog(const ComPtr<IMachine> aMachine)
{
    HRESULT hrc = S_OK;

    Bstr logFolder;
    hrc = aMachine->COMGETTER(LogFolder)(logFolder.asOutParam());
    if (FAILED(hrc))
        return hrc;

    Utf8Str logDir = logFolder;

    /* make sure the Logs folder exists */
    Assert(logDir.length());
    if (!RTDirExists(logDir.c_str()))
        RTDirCreateFullPath(logDir.c_str(), 0700);

    Utf8Str logFile = Utf8StrFmt("%s%cVBox.log",
                                 logDir.c_str(), RTPATH_DELIMITER);
    Utf8Str pngFile = Utf8StrFmt("%s%cVBox.png",
                                 logDir.c_str(), RTPATH_DELIMITER);

    /*
     * Age the old log files
     * Rename .(n-1) to .(n), .(n-2) to .(n-1), ..., and the last log file to .1
     * Overwrite target files in case they exist.
     */
    ComPtr<IVirtualBox> pVirtualBox;
    aMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
    ComPtr<ISystemProperties> pSystemProperties;
    pVirtualBox->COMGETTER(SystemProperties)(pSystemProperties.asOutParam());
    ULONG cHistoryFiles = 3;
    pSystemProperties->COMGETTER(LogHistoryCount)(&cHistoryFiles);
    if (cHistoryFiles)
    {
        for (int i = cHistoryFiles-1; i >= 0; i--)
        {
            Utf8Str *files[] = { &logFile, &pngFile };
            Utf8Str oldName, newName;

            for (unsigned int j = 0; j < RT_ELEMENTS(files); ++j)
            {
                if (i > 0)
                    oldName = Utf8StrFmt("%s.%d", files[j]->c_str(), i);
                else
                    oldName = *files[j];
                newName = Utf8StrFmt("%s.%d", files[j]->c_str(), i + 1);
                /* If the old file doesn't exist, delete the new file (if it
                 * exists) to provide correct rotation even if the sequence is
                 * broken */
                if (   RTFileRename(oldName.c_str(), newName.c_str(), RTFILEMOVE_FLAGS_REPLACE)
                    == VERR_FILE_NOT_FOUND)
                    RTFileDelete(newName.c_str());
            }
        }
    }

    RTERRINFOSTATIC ErrInfo;
    int vrc = com::VBoxLogRelCreate("VM", logFile.c_str(),
                                    RTLOGFLAGS_PREFIX_TIME_PROG | RTLOGFLAGS_RESTRICT_GROUPS,
                                    "all all.restrict -default.restrict",
                                    "VBOX_RELEASE_LOG", RTLOGDEST_FILE,
                                    32768 /* cMaxEntriesPerGroup */,
                                    0 /* cHistory */, 0 /* uHistoryFileTime */,
                                    0 /* uHistoryFileSize */, RTErrInfoInitStatic(&ErrInfo));
    if (RT_FAILURE(vrc))
        hrc = setError(E_FAIL, tr("Failed to open release log (%s, %Rrc)"), ErrInfo.Core.pszMsg, vrc);

    /* If we've made any directory changes, flush the directory to increase
       the likelihood that the log file will be usable after a system panic.

       Tip: Try 'export VBOX_RELEASE_LOG_FLAGS=flush' if the last bits of the log
            is missing. Just don't have too high hopes for this to help. */
    if (SUCCEEDED(hrc) || cHistoryFiles)
        RTDirFlush(logDir.c_str());

    return hrc;
}

/**
 * Common worker for PowerUp and PowerUpPaused.
 *
 * @returns COM status code.
 *
 * @param   aProgress       Where to return the progress object.
 * @param   aPaused         true if PowerUpPaused called.
 */
HRESULT Console::i_powerUp(IProgress **aProgress, bool aPaused)
{
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aProgress);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));
    HRESULT rc = S_OK;
    ComObjPtr<Progress> pPowerupProgress;
    bool fBeganPoweringUp = false;

    LONG cOperations = 1;
    LONG ulTotalOperationsWeight = 1;
    VMPowerUpTask* task = NULL;

    try
    {
        if (Global::IsOnlineOrTransient(mMachineState))
            throw setError(VBOX_E_INVALID_VM_STATE,
                tr("The virtual machine is already running or busy (machine state: %s)"),
                Global::stringifyMachineState(mMachineState));

        /* Set up release logging as early as possible after the check if
         * there is already a running VM which we shouldn't disturb. */
        rc = i_consoleInitReleaseLog(mMachine);
        if (FAILED(rc))
            throw rc;

#ifdef VBOX_OPENSSL_FIPS
        LogRel(("crypto: FIPS mode %s\n", FIPS_mode() ? "enabled" : "FAILED"));
#endif

        /* test and clear the TeleporterEnabled property  */
        BOOL fTeleporterEnabled;
        rc = mMachine->COMGETTER(TeleporterEnabled)(&fTeleporterEnabled);
        if (FAILED(rc))
            throw rc;

#if 0 /** @todo we should save it afterwards, but that isn't necessarily a good idea. Find a better place for this (VBoxSVC).  */
        if (fTeleporterEnabled)
        {
            rc = mMachine->COMSETTER(TeleporterEnabled)(FALSE);
            if (FAILED(rc))
                throw rc;
        }
#endif

        /* test the FaultToleranceState property  */
        FaultToleranceState_T enmFaultToleranceState;
        rc = mMachine->COMGETTER(FaultToleranceState)(&enmFaultToleranceState);
        if (FAILED(rc))
            throw rc;
        BOOL fFaultToleranceSyncEnabled = (enmFaultToleranceState == FaultToleranceState_Standby);

        /* Create a progress object to track progress of this operation. Must
         * be done as early as possible (together with BeginPowerUp()) as this
         * is vital for communicating as much as possible early powerup
         * failure information to the API caller */
        pPowerupProgress.createObject();
        Bstr progressDesc;
        if (mMachineState == MachineState_Saved)
            progressDesc = tr("Restoring virtual machine");
        else if (fTeleporterEnabled)
            progressDesc = tr("Teleporting virtual machine");
        else if (fFaultToleranceSyncEnabled)
            progressDesc = tr("Fault Tolerance syncing of remote virtual machine");
        else
            progressDesc = tr("Starting virtual machine");

        Bstr savedStateFile;

        /*
         * Saved VMs will have to prove that their saved states seem kosher.
         */
        if (mMachineState == MachineState_Saved)
        {
            rc = mMachine->COMGETTER(StateFilePath)(savedStateFile.asOutParam());
            if (FAILED(rc))
                throw rc;
            ComAssertRet(!savedStateFile.isEmpty(), E_FAIL);
            int vrc = SSMR3ValidateFile(Utf8Str(savedStateFile).c_str(), false /* fChecksumIt */);
            if (RT_FAILURE(vrc))
                throw setError(VBOX_E_FILE_ERROR,
                               tr("VM cannot start because the saved state file '%ls' is invalid (%Rrc). Delete the saved state prior to starting the VM"),
                               savedStateFile.raw(), vrc);
        }

        /* Read console data, including console shared folders, stored in the
         * saved state file (if not yet done).
         */
        rc = i_loadDataFromSavedState();
        if (FAILED(rc))
            throw rc;

        /* Check all types of shared folders and compose a single list */
        SharedFolderDataMap sharedFolders;
        {
            /* first, insert global folders */
            for (SharedFolderDataMap::const_iterator it = m_mapGlobalSharedFolders.begin();
                 it != m_mapGlobalSharedFolders.end();
                 ++it)
            {
                const SharedFolderData &d = it->second;
                sharedFolders[it->first] = d;
            }

            /* second, insert machine folders */
            for (SharedFolderDataMap::const_iterator it = m_mapMachineSharedFolders.begin();
                 it != m_mapMachineSharedFolders.end();
                 ++it)
            {
                const SharedFolderData &d = it->second;
                sharedFolders[it->first] = d;
            }

            /* third, insert console folders */
            for (SharedFolderMap::const_iterator it = m_mapSharedFolders.begin();
                 it != m_mapSharedFolders.end();
                 ++it)
            {
                SharedFolder *pSF = it->second;
                AutoCaller sfCaller(pSF);
                AutoReadLock sfLock(pSF COMMA_LOCKVAL_SRC_POS);
                sharedFolders[it->first] = SharedFolderData(pSF->i_getHostPath(),
                                                            pSF->i_isWritable(),
                                                            pSF->i_isAutoMounted());
            }
        }


        /* Setup task object and thread to carry out the operation
         * asynchronously */
        try
        {
            task = new VMPowerUpTask(this, pPowerupProgress);
            if (!task->isOk())
            {
                throw E_FAIL;
            }
        }
        catch(...)
        {
            delete task;
            rc = setError(E_FAIL, "Could not create VMPowerUpTask object \n");
            throw rc;
        }

        task->mConfigConstructor = i_configConstructor;
        task->mSharedFolders = sharedFolders;
        task->mStartPaused = aPaused;
        if (mMachineState == MachineState_Saved)
            task->mSavedStateFile = savedStateFile;
        task->mTeleporterEnabled = fTeleporterEnabled;
        task->mEnmFaultToleranceState = enmFaultToleranceState;

        /* Reset differencing hard disks for which autoReset is true,
         * but only if the machine has no snapshots OR the current snapshot
         * is an OFFLINE snapshot; otherwise we would reset the current
         * differencing image of an ONLINE snapshot which contains the disk
         * state of the machine while it was previously running, but without
         * the corresponding machine state, which is equivalent to powering
         * off a running machine and not good idea
         */
        ComPtr<ISnapshot> pCurrentSnapshot;
        rc = mMachine->COMGETTER(CurrentSnapshot)(pCurrentSnapshot.asOutParam());
        if (FAILED(rc))
            throw rc;

        BOOL fCurrentSnapshotIsOnline = false;
        if (pCurrentSnapshot)
        {
            rc = pCurrentSnapshot->COMGETTER(Online)(&fCurrentSnapshotIsOnline);
            if (FAILED(rc))
                throw rc;
        }

        if (savedStateFile.isEmpty() && !fCurrentSnapshotIsOnline)
        {
            LogFlowThisFunc(("Looking for immutable images to reset\n"));

            com::SafeIfaceArray<IMediumAttachment> atts;
            rc = mMachine->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(atts));
            if (FAILED(rc))
                throw rc;

            for (size_t i = 0;
                 i < atts.size();
                 ++i)
            {
                DeviceType_T devType;
                rc = atts[i]->COMGETTER(Type)(&devType);
                /** @todo later applies to floppies as well */
                if (devType == DeviceType_HardDisk)
                {
                    ComPtr<IMedium> pMedium;
                    rc = atts[i]->COMGETTER(Medium)(pMedium.asOutParam());
                    if (FAILED(rc))
                        throw rc;

                    /* needs autoreset? */
                    BOOL autoReset = FALSE;
                    rc = pMedium->COMGETTER(AutoReset)(&autoReset);
                    if (FAILED(rc))
                        throw rc;

                    if (autoReset)
                    {
                        ComPtr<IProgress> pResetProgress;
                        rc = pMedium->Reset(pResetProgress.asOutParam());
                        if (FAILED(rc))
                            throw rc;

                        /* save for later use on the powerup thread */
                        task->hardDiskProgresses.push_back(pResetProgress);
                    }
                }
            }
        }
        else
            LogFlowThisFunc(("Machine has a current snapshot which is online, skipping immutable images reset\n"));

        /* setup task object and thread to carry out the operation
         * asynchronously */

#ifdef VBOX_WITH_EXTPACK
        mptrExtPackManager->i_dumpAllToReleaseLog();
#endif

#ifdef RT_OS_SOLARIS
        /* setup host core dumper for the VM */
        Bstr value;
        HRESULT hrc = mMachine->GetExtraData(Bstr("VBoxInternal2/CoreDumpEnabled").raw(), value.asOutParam());
        if (SUCCEEDED(hrc) && value == "1")
        {
            Bstr coreDumpDir, coreDumpReplaceSys, coreDumpLive;
            mMachine->GetExtraData(Bstr("VBoxInternal2/CoreDumpDir").raw(), coreDumpDir.asOutParam());
            mMachine->GetExtraData(Bstr("VBoxInternal2/CoreDumpReplaceSystemDump").raw(), coreDumpReplaceSys.asOutParam());
            mMachine->GetExtraData(Bstr("VBoxInternal2/CoreDumpLive").raw(), coreDumpLive.asOutParam());

            uint32_t fCoreFlags = 0;
            if (   coreDumpReplaceSys.isEmpty() == false
                && Utf8Str(coreDumpReplaceSys).toUInt32() == 1)
                fCoreFlags |= RTCOREDUMPER_FLAGS_REPLACE_SYSTEM_DUMP;

            if (   coreDumpLive.isEmpty() == false
                && Utf8Str(coreDumpLive).toUInt32() == 1)
                fCoreFlags |= RTCOREDUMPER_FLAGS_LIVE_CORE;

            Utf8Str strDumpDir(coreDumpDir);
            const char *pszDumpDir = strDumpDir.c_str();
            if (   pszDumpDir
                && *pszDumpDir == '\0')
                pszDumpDir = NULL;

            int vrc;
            if (   pszDumpDir
                && !RTDirExists(pszDumpDir))
            {
                /*
                 * Try create the directory.
                 */
                vrc = RTDirCreateFullPath(pszDumpDir, 0700);
                if (RT_FAILURE(vrc))
                    throw setError(E_FAIL, "Failed to setup CoreDumper. Couldn't create dump directory '%s' (%Rrc)\n",
                                   pszDumpDir, vrc);
            }

            vrc = RTCoreDumperSetup(pszDumpDir, fCoreFlags);
            if (RT_FAILURE(vrc))
                throw setError(E_FAIL, "Failed to setup CoreDumper (%Rrc)", vrc);
            else
                LogRel(("CoreDumper setup successful. pszDumpDir=%s fFlags=%#x\n", pszDumpDir ? pszDumpDir : ".", fCoreFlags));
        }
#endif


        // If there is immutable drive the process that.
        VMPowerUpTask::ProgressList progresses(task->hardDiskProgresses);
        if (aProgress && progresses.size() > 0)
        {
            for (VMPowerUpTask::ProgressList::const_iterator it = progresses.begin(); it !=  progresses.end(); ++it)
            {
                ++cOperations;
                ulTotalOperationsWeight += 1;
            }
            rc = pPowerupProgress->init(static_cast<IConsole *>(this),
                                        progressDesc.raw(),
                                        TRUE, // Cancelable
                                        cOperations,
                                        ulTotalOperationsWeight,
                                        Bstr(tr("Starting Hard Disk operations")).raw(),
                                        1);
            AssertComRCReturnRC(rc);
        }
        else if (    mMachineState == MachineState_Saved
            ||  (!fTeleporterEnabled && !fFaultToleranceSyncEnabled))
        {
            rc = pPowerupProgress->init(static_cast<IConsole *>(this),
                                        progressDesc.raw(),
                                        FALSE /* aCancelable */);
        }
        else if (fTeleporterEnabled)
        {
            rc = pPowerupProgress->init(static_cast<IConsole *>(this),
                                        progressDesc.raw(),
                                        TRUE /* aCancelable */,
                                        3    /* cOperations */,
                                        10   /* ulTotalOperationsWeight */,
                                        Bstr(tr("Teleporting virtual machine")).raw(),
                                        1    /* ulFirstOperationWeight */);
        }
        else if (fFaultToleranceSyncEnabled)
        {
            rc = pPowerupProgress->init(static_cast<IConsole *>(this),
                                        progressDesc.raw(),
                                        TRUE /* aCancelable */,
                                        3    /* cOperations */,
                                        10   /* ulTotalOperationsWeight */,
                                        Bstr(tr("Fault Tolerance syncing of remote virtual machine")).raw(),
                                        1    /* ulFirstOperationWeight */);
        }

        if (FAILED(rc))
            throw rc;

        /* Tell VBoxSVC and Machine about the progress object so they can
           combine/proxy it to any openRemoteSession caller. */
        LogFlowThisFunc(("Calling BeginPowerUp...\n"));
        rc = mControl->BeginPowerUp(pPowerupProgress);
        if (FAILED(rc))
        {
            LogFlowThisFunc(("BeginPowerUp failed\n"));
            throw rc;
        }
        fBeganPoweringUp = true;

        LogFlowThisFunc(("Checking if canceled...\n"));
        BOOL fCanceled;
        rc = pPowerupProgress->COMGETTER(Canceled)(&fCanceled);
        if (FAILED(rc))
            throw rc;

        if (fCanceled)
        {
            LogFlowThisFunc(("Canceled in BeginPowerUp\n"));
            throw setError(E_FAIL, tr("Powerup was canceled"));
        }
        LogFlowThisFunc(("Not canceled yet.\n"));

        /** @todo this code prevents starting a VM with unavailable bridged
         * networking interface. The only benefit is a slightly better error
         * message, which should be moved to the driver code. This is the
         * only reason why I left the code in for now. The driver allows
         * unavailable bridged networking interfaces in certain circumstances,
         * and this is sabotaged by this check. The VM will initially have no
         * network connectivity, but the user can fix this at runtime. */
#if 0
        /* the network cards will undergo a quick consistency check */
        for (ULONG slot = 0;
             slot < maxNetworkAdapters;
             ++slot)
        {
            ComPtr<INetworkAdapter> pNetworkAdapter;
            mMachine->GetNetworkAdapter(slot, pNetworkAdapter.asOutParam());
            BOOL enabled = FALSE;
            pNetworkAdapter->COMGETTER(Enabled)(&enabled);
            if (!enabled)
                continue;

            NetworkAttachmentType_T netattach;
            pNetworkAdapter->COMGETTER(AttachmentType)(&netattach);
            switch (netattach)
            {
                case NetworkAttachmentType_Bridged:
                {
                    /* a valid host interface must have been set */
                    Bstr hostif;
                    pNetworkAdapter->COMGETTER(HostInterface)(hostif.asOutParam());
                    if (hostif.isEmpty())
                    {
                        throw setError(VBOX_E_HOST_ERROR,
                            tr("VM cannot start because host interface networking requires a host interface name to be set"));
                    }
                    ComPtr<IVirtualBox> pVirtualBox;
                    mMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
                    ComPtr<IHost> pHost;
                    pVirtualBox->COMGETTER(Host)(pHost.asOutParam());
                    ComPtr<IHostNetworkInterface> pHostInterface;
                    if (!SUCCEEDED(pHost->FindHostNetworkInterfaceByName(hostif.raw(),
                                                                         pHostInterface.asOutParam())))
                    {
                        throw setError(VBOX_E_HOST_ERROR,
                            tr("VM cannot start because the host interface '%ls' does not exist"),
                            hostif.raw());
                    }
                    break;
                }
                default:
                    break;
            }
        }
#endif // 0


        /* setup task object and thread to carry out the operation
         * asynchronously */
        if (aProgress){
                rc = pPowerupProgress.queryInterfaceTo(aProgress);
                AssertComRCReturnRC(rc);
        }

        rc = task->createThread();

        if (FAILED(rc))
            throw rc;

        /* finally, set the state: no right to fail in this method afterwards
         * since we've already started the thread and it is now responsible for
         * any error reporting and appropriate state change! */
        if (mMachineState == MachineState_Saved)
            i_setMachineState(MachineState_Restoring);
        else if (fTeleporterEnabled)
            i_setMachineState(MachineState_TeleportingIn);
        else if (enmFaultToleranceState == FaultToleranceState_Standby)
            i_setMachineState(MachineState_FaultTolerantSyncing);
        else
            i_setMachineState(MachineState_Starting);
    }
    catch (HRESULT aRC) { rc = aRC; }

    if (FAILED(rc) && fBeganPoweringUp)
    {

        /* The progress object will fetch the current error info */
        if (!pPowerupProgress.isNull())
            pPowerupProgress->i_notifyComplete(rc);

        /* Save the error info across the IPC below. Can't be done before the
         * progress notification above, as saving the error info deletes it
         * from the current context, and thus the progress object wouldn't be
         * updated correctly. */
        ErrorInfoKeeper eik;

        /* signal end of operation */
        mControl->EndPowerUp(rc);
    }

    LogFlowThisFunc(("mMachineState=%d, rc=%Rhrc\n", mMachineState, rc));
    LogFlowThisFuncLeave();
    return rc;
}

/**
 * Internal power off worker routine.
 *
 * This method may be called only at certain places with the following meaning
 * as shown below:
 *
 * - if the machine state is either Running or Paused, a normal
 *   Console-initiated powerdown takes place (e.g. PowerDown());
 * - if the machine state is Saving, saveStateThread() has successfully done its
 *   job;
 * - if the machine state is Starting or Restoring, powerUpThread() has failed
 *   to start/load the VM;
 * - if the machine state is Stopping, the VM has powered itself off (i.e. not
 *   as a result of the powerDown() call).
 *
 * Calling it in situations other than the above will cause unexpected behavior.
 *
 * Note that this method should be the only one that destroys mpUVM and sets it
 * to NULL.
 *
 * @param aProgress Progress object to run (may be NULL).
 *
 * @note Locks this object for writing.
 *
 * @note Never call this method from a thread that called addVMCaller() or
 *       instantiated an AutoVMCaller object; first call releaseVMCaller() or
 *       release(). Otherwise it will deadlock.
 */
HRESULT Console::i_powerDown(IProgress *aProgress /*= NULL*/)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Total # of steps for the progress object. Must correspond to the
     * number of "advance percent count" comments in this method! */
    enum { StepCount = 7 };
    /* current step */
    ULONG step = 0;

    HRESULT rc = S_OK;
    int vrc = VINF_SUCCESS;

    /* sanity */
    Assert(mVMDestroying == false);

    PUVM     pUVM  = mpUVM;                 Assert(pUVM != NULL);
    uint32_t cRefs = VMR3RetainUVM(pUVM);   Assert(cRefs != UINT32_MAX);  NOREF(cRefs);

    AssertMsg(   mMachineState == MachineState_Running
              || mMachineState == MachineState_Paused
              || mMachineState == MachineState_Stuck
              || mMachineState == MachineState_Starting
              || mMachineState == MachineState_Stopping
              || mMachineState == MachineState_Saving
              || mMachineState == MachineState_Restoring
              || mMachineState == MachineState_TeleportingPausedVM
              || mMachineState == MachineState_FaultTolerantSyncing
              || mMachineState == MachineState_TeleportingIn
              , ("Invalid machine state: %s\n", Global::stringifyMachineState(mMachineState)));

    LogRel(("Console::powerDown(): A request to power off the VM has been issued (mMachineState=%s, InUninit=%d)\n",
            Global::stringifyMachineState(mMachineState), getObjectState().getState() == ObjectState::InUninit));

    /* Check if we need to power off the VM. In case of mVMPoweredOff=true, the
     * VM has already powered itself off in vmstateChangeCallback() and is just
     * notifying Console about that. In case of Starting or Restoring,
     * powerUpThread() is calling us on failure, so the VM is already off at
     * that point. */
    if (   !mVMPoweredOff
        && (   mMachineState == MachineState_Starting
            || mMachineState == MachineState_Restoring
            || mMachineState == MachineState_FaultTolerantSyncing
            || mMachineState == MachineState_TeleportingIn)
       )
        mVMPoweredOff = true;

    /*
     * Go to Stopping state if not already there.
     *
     * Note that we don't go from Saving/Restoring to Stopping because
     * vmstateChangeCallback() needs it to set the state to Saved on
     * VMSTATE_TERMINATED. In terms of protecting from inappropriate operations
     * while leaving the lock below, Saving or Restoring should be fine too.
     * Ditto for TeleportingPausedVM -> Teleported.
     */
    if (   mMachineState != MachineState_Saving
        && mMachineState != MachineState_Restoring
        && mMachineState != MachineState_Stopping
        && mMachineState != MachineState_TeleportingIn
        && mMachineState != MachineState_TeleportingPausedVM
        && mMachineState != MachineState_FaultTolerantSyncing
       )
        i_setMachineState(MachineState_Stopping);

    /* ----------------------------------------------------------------------
     * DONE with necessary state changes, perform the power down actions (it's
     * safe to release the object lock now if needed)
     * ---------------------------------------------------------------------- */

    if (mDisplay)
    {
        alock.release();

        mDisplay->i_notifyPowerDown();

        alock.acquire();
    }

    /* Stop the VRDP server to prevent new clients connection while VM is being
     * powered off. */
    if (mConsoleVRDPServer)
    {
        LogFlowThisFunc(("Stopping VRDP server...\n"));

        /* Leave the lock since EMT could call us back as addVMCaller() */
        alock.release();

        mConsoleVRDPServer->Stop();

        alock.acquire();
    }

    /* advance percent count */
    if (aProgress)
        aProgress->SetCurrentOperationProgress(99 * (++step) / StepCount );


    /* ----------------------------------------------------------------------
     * Now, wait for all mpUVM callers to finish their work if there are still
     * some on other threads. NO methods that need mpUVM (or initiate other calls
     * that need it) may be called after this point
     * ---------------------------------------------------------------------- */

    /* go to the destroying state to prevent from adding new callers */
    mVMDestroying = true;

    if (mVMCallers > 0)
    {
        /* lazy creation */
        if (mVMZeroCallersSem == NIL_RTSEMEVENT)
            RTSemEventCreate(&mVMZeroCallersSem);

        LogFlowThisFunc(("Waiting for mpUVM callers (%d) to drop to zero...\n", mVMCallers));

        alock.release();

        RTSemEventWait(mVMZeroCallersSem, RT_INDEFINITE_WAIT);

        alock.acquire();
    }

    /* advance percent count */
    if (aProgress)
        aProgress->SetCurrentOperationProgress(99 * (++step) / StepCount );

    vrc = VINF_SUCCESS;

    /*
     * Power off the VM if not already done that.
     * Leave the lock since EMT will call vmstateChangeCallback.
     *
     * Note that VMR3PowerOff() may fail here (invalid VMSTATE) if the
     * VM-(guest-)initiated power off happened in parallel a ms before this
     * call. So far, we let this error pop up on the user's side.
     */
    if (!mVMPoweredOff)
    {
        LogFlowThisFunc(("Powering off the VM...\n"));
        alock.release();
        vrc = VMR3PowerOff(pUVM);
#ifdef VBOX_WITH_EXTPACK
        mptrExtPackManager->i_callAllVmPowerOffHooks(this, VMR3GetVM(pUVM));
#endif
        alock.acquire();
    }

    /* advance percent count */
    if (aProgress)
        aProgress->SetCurrentOperationProgress(99 * (++step) / StepCount );

#ifdef VBOX_WITH_HGCM
    /* Shutdown HGCM services before destroying the VM. */
    if (m_pVMMDev)
    {
        LogFlowThisFunc(("Shutdown HGCM...\n"));

        /* Leave the lock since EMT might wait for it and will call us back as addVMCaller() */
        alock.release();

        m_pVMMDev->hgcmShutdown();

        alock.acquire();
    }

    /* advance percent count */
    if (aProgress)
        aProgress->SetCurrentOperationProgress(99 * (++step) / StepCount);

#endif /* VBOX_WITH_HGCM */

    LogFlowThisFunc(("Ready for VM destruction.\n"));

    /* If we are called from Console::uninit(), then try to destroy the VM even
     * on failure (this will most likely fail too, but what to do?..) */
    if (RT_SUCCESS(vrc) || getObjectState().getState() == ObjectState::InUninit)
    {
        /* If the machine has a USB controller, release all USB devices
         * (symmetric to the code in captureUSBDevices()) */
        if (mfVMHasUsbController)
        {
            alock.release();
            i_detachAllUSBDevices(false /* aDone */);
            alock.acquire();
        }

        /* Now we've got to destroy the VM as well. (mpUVM is not valid beyond
         * this point). We release the lock before calling VMR3Destroy() because
         * it will result into calling destructors of drivers associated with
         * Console children which may in turn try to lock Console (e.g. by
         * instantiating SafeVMPtr to access mpUVM). It's safe here because
         * mVMDestroying is set which should prevent any activity. */

        /* Set mpUVM to NULL early just in case if some old code is not using
         * addVMCaller()/releaseVMCaller(). (We have our own ref on pUVM.) */
        VMR3ReleaseUVM(mpUVM);
        mpUVM = NULL;

        LogFlowThisFunc(("Destroying the VM...\n"));

        alock.release();

        vrc = VMR3Destroy(pUVM);

        /* take the lock again */
        alock.acquire();

        /* advance percent count */
        if (aProgress)
            aProgress->SetCurrentOperationProgress(99 * (++step) / StepCount);

        if (RT_SUCCESS(vrc))
        {
            LogFlowThisFunc(("Machine has been destroyed (mMachineState=%d)\n",
                             mMachineState));
            /* Note: the Console-level machine state change happens on the
             * VMSTATE_TERMINATE state change in vmstateChangeCallback(). If
             * powerDown() is called from EMT (i.e. from vmstateChangeCallback()
             * on receiving VM-initiated VMSTATE_OFF), VMSTATE_TERMINATE hasn't
             * occurred yet. This is okay, because mMachineState is already
             * Stopping in this case, so any other attempt to call PowerDown()
             * will be rejected. */
        }
        else
        {
            /* bad bad bad, but what to do? (Give Console our UVM ref.) */
            mpUVM = pUVM;
            pUVM = NULL;
            rc = setError(VBOX_E_VM_ERROR,
                tr("Could not destroy the machine. (Error: %Rrc)"),
                vrc);
        }

        /* Complete the detaching of the USB devices. */
        if (mfVMHasUsbController)
        {
            alock.release();
            i_detachAllUSBDevices(true /* aDone */);
            alock.acquire();
        }

        /* advance percent count */
        if (aProgress)
            aProgress->SetCurrentOperationProgress(99 * (++step) / StepCount);
    }
    else
    {
        rc = setError(VBOX_E_VM_ERROR,
            tr("Could not power off the machine. (Error: %Rrc)"),
            vrc);
    }

    /*
     * Finished with the destruction.
     *
     * Note that if something impossible happened and we've failed to destroy
     * the VM, mVMDestroying will remain true and mMachineState will be
     * something like Stopping, so most Console methods will return an error
     * to the caller.
     */
    if (pUVM != NULL)
        VMR3ReleaseUVM(pUVM);
    else
        mVMDestroying = false;

    LogFlowThisFuncLeave();
    return rc;
}

/**
 * @note Locks this object for writing.
 */
HRESULT Console::i_setMachineState(MachineState_T aMachineState,
                                   bool aUpdateServer /* = true */)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;

    if (mMachineState != aMachineState)
    {
        LogThisFunc(("machineState=%s -> %s aUpdateServer=%RTbool\n",
                     Global::stringifyMachineState(mMachineState), Global::stringifyMachineState(aMachineState), aUpdateServer));
        LogRel(("Console: Machine state changed to '%s'\n", Global::stringifyMachineState(aMachineState)));
        mMachineState = aMachineState;

        /// @todo (dmik)
        //      possibly, we need to redo onStateChange() using the dedicated
        //      Event thread, like it is done in VirtualBox. This will make it
        //      much safer (no deadlocks possible if someone tries to use the
        //      console from the callback), however, listeners will lose the
        //      ability to synchronously react to state changes (is it really
        //      necessary??)
        LogFlowThisFunc(("Doing onStateChange()...\n"));
        i_onStateChange(aMachineState);
        LogFlowThisFunc(("Done onStateChange()\n"));

        if (aUpdateServer)
        {
            /* Server notification MUST be done from under the lock; otherwise
             * the machine state here and on the server might go out of sync
             * which can lead to various unexpected results (like the machine
             * state being >= MachineState_Running on the server, while the
             * session state is already SessionState_Unlocked at the same time
             * there).
             *
             * Cross-lock conditions should be carefully watched out: calling
             * UpdateState we will require Machine and SessionMachine locks
             * (remember that here we're holding the Console lock here, and also
             * all locks that have been acquire by the thread before calling
             * this method).
             */
            LogFlowThisFunc(("Doing mControl->UpdateState()...\n"));
            rc = mControl->UpdateState(aMachineState);
            LogFlowThisFunc(("mControl->UpdateState()=%Rhrc\n", rc));
        }
    }

    return rc;
}

/**
 * Searches for a shared folder with the given logical name
 * in the collection of shared folders.
 *
 * @param strName          logical name of the shared folder
 * @param aSharedFolder    where to return the found object
 * @param aSetError        whether to set the error info if the folder is
 *                         not found
 * @return
 *     S_OK when found or E_INVALIDARG when not found
 *
 * @note The caller must lock this object for writing.
 */
HRESULT Console::i_findSharedFolder(const Utf8Str &strName,
                                    ComObjPtr<SharedFolder> &aSharedFolder,
                                    bool aSetError /* = false */)
{
    /* sanity check */
    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

    SharedFolderMap::const_iterator it = m_mapSharedFolders.find(strName);
    if (it != m_mapSharedFolders.end())
    {
        aSharedFolder = it->second;
        return S_OK;
    }

    if (aSetError)
        setError(VBOX_E_FILE_ERROR,
            tr("Could not find a shared folder named '%s'."),
            strName.c_str());

    return VBOX_E_FILE_ERROR;
}

/**
 * Fetches the list of global or machine shared folders from the server.
 *
 * @param aGlobal true to fetch global folders.
 *
 * @note The caller must lock this object for writing.
 */
HRESULT Console::i_fetchSharedFolders(BOOL aGlobal)
{
    /* sanity check */
    AssertReturn(   getObjectState().getState() == ObjectState::InInit
                 || isWriteLockOnCurrentThread(), E_FAIL);

    LogFlowThisFunc(("Entering\n"));

    /* Check if we're online and keep it that way. */
    SafeVMPtrQuiet ptrVM(this);
    AutoVMCallerQuietWeak autoVMCaller(this);
    bool const online = ptrVM.isOk()
                     && m_pVMMDev
                     && m_pVMMDev->isShFlActive();

    HRESULT rc = S_OK;

    try
    {
        if (aGlobal)
        {
            /// @todo grab & process global folders when they are done
        }
        else
        {
            SharedFolderDataMap oldFolders;
            if (online)
                oldFolders = m_mapMachineSharedFolders;

            m_mapMachineSharedFolders.clear();

            SafeIfaceArray<ISharedFolder> folders;
            rc = mMachine->COMGETTER(SharedFolders)(ComSafeArrayAsOutParam(folders));
            if (FAILED(rc)) throw rc;

            for (size_t i = 0; i < folders.size(); ++i)
            {
                ComPtr<ISharedFolder> pSharedFolder = folders[i];

                Bstr bstrName;
                Bstr bstrHostPath;
                BOOL writable;
                BOOL autoMount;

                rc = pSharedFolder->COMGETTER(Name)(bstrName.asOutParam());
                if (FAILED(rc)) throw rc;
                Utf8Str strName(bstrName);

                rc = pSharedFolder->COMGETTER(HostPath)(bstrHostPath.asOutParam());
                if (FAILED(rc)) throw rc;
                Utf8Str strHostPath(bstrHostPath);

                rc = pSharedFolder->COMGETTER(Writable)(&writable);
                if (FAILED(rc)) throw rc;

                rc = pSharedFolder->COMGETTER(AutoMount)(&autoMount);
                if (FAILED(rc)) throw rc;

                m_mapMachineSharedFolders.insert(std::make_pair(strName,
                                                                SharedFolderData(strHostPath, !!writable, !!autoMount)));

                /* send changes to HGCM if the VM is running */
                if (online)
                {
                    SharedFolderDataMap::iterator it = oldFolders.find(strName);
                    if (    it == oldFolders.end()
                         || it->second.m_strHostPath != strHostPath)
                    {
                        /* a new machine folder is added or
                         * the existing machine folder is changed */
                        if (m_mapSharedFolders.find(strName) != m_mapSharedFolders.end())
                            ; /* the console folder exists, nothing to do */
                        else
                        {
                            /* remove the old machine folder (when changed)
                             * or the global folder if any (when new) */
                            if (    it != oldFolders.end()
                                 || m_mapGlobalSharedFolders.find(strName) != m_mapGlobalSharedFolders.end()
                               )
                            {
                                rc = i_removeSharedFolder(strName);
                                if (FAILED(rc)) throw rc;
                            }

                            /* create the new machine folder */
                            rc = i_createSharedFolder(strName,
                                                      SharedFolderData(strHostPath, !!writable, !!autoMount));
                            if (FAILED(rc)) throw rc;
                        }
                    }
                    /* forget the processed (or identical) folder */
                    if (it != oldFolders.end())
                        oldFolders.erase(it);
                }
            }

            /* process outdated (removed) folders */
            if (online)
            {
                for (SharedFolderDataMap::const_iterator it = oldFolders.begin();
                     it != oldFolders.end(); ++it)
                {
                    if (m_mapSharedFolders.find(it->first) != m_mapSharedFolders.end())
                        ; /* the console folder exists, nothing to do */
                    else
                    {
                        /* remove the outdated machine folder */
                        rc = i_removeSharedFolder(it->first);
                        if (FAILED(rc)) throw rc;

                        /* create the global folder if there is any */
                        SharedFolderDataMap::const_iterator git =
                            m_mapGlobalSharedFolders.find(it->first);
                        if (git != m_mapGlobalSharedFolders.end())
                        {
                            rc = i_createSharedFolder(git->first, git->second);
                            if (FAILED(rc)) throw rc;
                        }
                    }
                }
            }
        }
    }
    catch (HRESULT rc2)
    {
        rc = rc2;
        if (online)
            i_atVMRuntimeErrorCallbackF(0, "BrokenSharedFolder", N_("Broken shared folder!"));
    }

    LogFlowThisFunc(("Leaving\n"));

    return rc;
}

/**
 * Searches for a shared folder with the given name in the list of machine
 * shared folders and then in the list of the global shared folders.
 *
 * @param strName  Name of the folder to search for.
 * @param aIt      Where to store the pointer to the found folder.
 * @return         @c true if the folder was found and @c false otherwise.
 *
 * @note The caller must lock this object for reading.
 */
bool Console::i_findOtherSharedFolder(const Utf8Str &strName,
                                    SharedFolderDataMap::const_iterator &aIt)
{
    /* sanity check */
    AssertReturn(isWriteLockOnCurrentThread(), false);

    /* first, search machine folders */
    aIt = m_mapMachineSharedFolders.find(strName);
    if (aIt != m_mapMachineSharedFolders.end())
        return true;

    /* second, search machine folders */
    aIt = m_mapGlobalSharedFolders.find(strName);
    if (aIt != m_mapGlobalSharedFolders.end())
        return true;

    return false;
}

/**
 * Calls the HGCM service to add a shared folder definition.
 *
 * @param strName      Shared folder name.
 * @param aData        Shared folder data.
 *
 * @note Must be called from under AutoVMCaller and when mpUVM != NULL!
 * @note Doesn't lock anything.
 */
HRESULT Console::i_createSharedFolder(const Utf8Str &strName, const SharedFolderData &aData)
{
    ComAssertRet(strName.isNotEmpty(), E_FAIL);
    ComAssertRet(aData.m_strHostPath.isNotEmpty(), E_FAIL);

    /* sanity checks */
    AssertReturn(mpUVM, E_FAIL);
    AssertReturn(m_pVMMDev && m_pVMMDev->isShFlActive(), E_FAIL);

    VBOXHGCMSVCPARM parms[SHFL_CPARMS_ADD_MAPPING];
    SHFLSTRING *pFolderName, *pMapName;
    size_t cbString;

    Bstr value;
    HRESULT hrc = mMachine->GetExtraData(BstrFmt("VBoxInternal2/SharedFoldersEnableSymlinksCreate/%s",
                                                 strName.c_str()).raw(),
                                         value.asOutParam());
    bool fSymlinksCreate = hrc == S_OK && value == "1";

    Log(("Adding shared folder '%s' -> '%s'\n", strName.c_str(), aData.m_strHostPath.c_str()));

    // check whether the path is valid and exists
    char hostPathFull[RTPATH_MAX];
    int vrc = RTPathAbsEx(NULL,
                          aData.m_strHostPath.c_str(),
                          hostPathFull,
                          sizeof(hostPathFull));

    bool fMissing = false;
    if (RT_FAILURE(vrc))
        return setError(E_INVALIDARG,
                        tr("Invalid shared folder path: '%s' (%Rrc)"),
                        aData.m_strHostPath.c_str(), vrc);
    if (!RTPathExists(hostPathFull))
        fMissing = true;

    /* Check whether the path is full (absolute) */
    if (RTPathCompare(aData.m_strHostPath.c_str(), hostPathFull) != 0)
        return setError(E_INVALIDARG,
                        tr("Shared folder path '%s' is not absolute"),
                        aData.m_strHostPath.c_str());

    // now that we know the path is good, give it to HGCM

    Bstr bstrName(strName);
    Bstr bstrHostPath(aData.m_strHostPath);

    cbString = (bstrHostPath.length() + 1) * sizeof(RTUTF16);
    if (cbString >= UINT16_MAX)
        return setError(E_INVALIDARG, tr("The name is too long"));
    pFolderName = (SHFLSTRING*)RTMemAllocZ(SHFLSTRING_HEADER_SIZE + cbString);
    Assert(pFolderName);
    memcpy(pFolderName->String.ucs2, bstrHostPath.raw(), cbString);

    pFolderName->u16Size   = (uint16_t)cbString;
    pFolderName->u16Length = (uint16_t)(cbString - sizeof(RTUTF16));

    parms[0].type = VBOX_HGCM_SVC_PARM_PTR;
    parms[0].u.pointer.addr = pFolderName;
    parms[0].u.pointer.size = ShflStringSizeOfBuffer(pFolderName);

    cbString = (bstrName.length() + 1) * sizeof(RTUTF16);
    if (cbString >= UINT16_MAX)
    {
        RTMemFree(pFolderName);
        return setError(E_INVALIDARG, tr("The host path is too long"));
    }
    pMapName = (SHFLSTRING*)RTMemAllocZ(SHFLSTRING_HEADER_SIZE + cbString);
    Assert(pMapName);
    memcpy(pMapName->String.ucs2, bstrName.raw(), cbString);

    pMapName->u16Size   = (uint16_t)cbString;
    pMapName->u16Length = (uint16_t)(cbString - sizeof(RTUTF16));

    parms[1].type = VBOX_HGCM_SVC_PARM_PTR;
    parms[1].u.pointer.addr = pMapName;
    parms[1].u.pointer.size = ShflStringSizeOfBuffer(pMapName);

    parms[2].type = VBOX_HGCM_SVC_PARM_32BIT;
    parms[2].u.uint32 = (aData.m_fWritable ? SHFL_ADD_MAPPING_F_WRITABLE : 0)
                      | (aData.m_fAutoMount ? SHFL_ADD_MAPPING_F_AUTOMOUNT : 0)
                      | (fSymlinksCreate ? SHFL_ADD_MAPPING_F_CREATE_SYMLINKS : 0)
                      | (fMissing ? SHFL_ADD_MAPPING_F_MISSING : 0)
                      ;

    vrc = m_pVMMDev->hgcmHostCall("VBoxSharedFolders",
                                  SHFL_FN_ADD_MAPPING,
                                  SHFL_CPARMS_ADD_MAPPING, &parms[0]);
    RTMemFree(pFolderName);
    RTMemFree(pMapName);

    if (RT_FAILURE(vrc))
        return setError(E_FAIL,
            tr("Could not create a shared folder '%s' mapped to '%s' (%Rrc)"),
            strName.c_str(), aData.m_strHostPath.c_str(), vrc);

    if (fMissing)
        return setError(E_INVALIDARG,
                        tr("Shared folder path '%s' does not exist on the host"),
                        aData.m_strHostPath.c_str());

    return S_OK;
}

/**
 * Calls the HGCM service to remove the shared folder definition.
 *
 * @param strName       Shared folder name.
 *
 * @note Must be called from under AutoVMCaller and when mpUVM != NULL!
 * @note Doesn't lock anything.
 */
HRESULT Console::i_removeSharedFolder(const Utf8Str &strName)
{
    ComAssertRet(strName.isNotEmpty(), E_FAIL);

    /* sanity checks */
    AssertReturn(mpUVM, E_FAIL);
    AssertReturn(m_pVMMDev && m_pVMMDev->isShFlActive(), E_FAIL);

    VBOXHGCMSVCPARM parms;
    SHFLSTRING *pMapName;
    size_t cbString;

    Log(("Removing shared folder '%s'\n", strName.c_str()));

    Bstr bstrName(strName);
    cbString = (bstrName.length() + 1) * sizeof(RTUTF16);
    if (cbString >= UINT16_MAX)
        return setError(E_INVALIDARG, tr("The name is too long"));
    pMapName = (SHFLSTRING *) RTMemAllocZ(SHFLSTRING_HEADER_SIZE + cbString);
    Assert(pMapName);
    memcpy(pMapName->String.ucs2, bstrName.raw(), cbString);

    pMapName->u16Size   = (uint16_t)cbString;
    pMapName->u16Length = (uint16_t)(cbString - sizeof(RTUTF16));

    parms.type = VBOX_HGCM_SVC_PARM_PTR;
    parms.u.pointer.addr = pMapName;
    parms.u.pointer.size = ShflStringSizeOfBuffer(pMapName);

    int vrc = m_pVMMDev->hgcmHostCall("VBoxSharedFolders",
                                      SHFL_FN_REMOVE_MAPPING,
                                      1, &parms);
    RTMemFree(pMapName);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL,
                        tr("Could not remove the shared folder '%s' (%Rrc)"),
                        strName.c_str(), vrc);

    return S_OK;
}

/** @callback_method_impl{FNVMATSTATE}
 *
 * @note    Locks the Console object for writing.
 * @remarks The @a pUVM parameter can be NULL in one case where powerUpThread()
 *          calls after the VM was destroyed.
 */
DECLCALLBACK(void) Console::i_vmstateChangeCallback(PUVM pUVM, VMSTATE enmState, VMSTATE enmOldState, void *pvUser)
{
    LogFlowFunc(("Changing state from %s to %s (pUVM=%p)\n",
                 VMR3GetStateName(enmOldState), VMR3GetStateName(enmState),     pUVM));

    Console *that = static_cast<Console *>(pvUser);
    AssertReturnVoid(that);

    AutoCaller autoCaller(that);

    /* Note that we must let this method proceed even if Console::uninit() has
     * been already called. In such case this VMSTATE change is a result of:
     * 1) powerDown() called from uninit() itself, or
     * 2) VM-(guest-)initiated power off. */
    AssertReturnVoid(   autoCaller.isOk()
                     || that->getObjectState().getState() == ObjectState::InUninit);

    switch (enmState)
    {
        /*
         * The VM has terminated
         */
        case VMSTATE_OFF:
        {
#ifdef VBOX_WITH_GUEST_PROPS
            if (that->i_isResetTurnedIntoPowerOff())
            {
                Bstr strPowerOffReason;

                if (that->mfPowerOffCausedByReset)
                    strPowerOffReason = Bstr("Reset");
                else
                    strPowerOffReason = Bstr("PowerOff");

                that->mMachine->DeleteGuestProperty(Bstr("/VirtualBox/HostInfo/VMPowerOffReason").raw());
                that->mMachine->SetGuestProperty(Bstr("/VirtualBox/HostInfo/VMPowerOffReason").raw(),
                                                 strPowerOffReason.raw(), Bstr("RDONLYGUEST").raw());
                that->mMachine->SaveSettings();
            }
#endif

            AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);

            if (that->mVMStateChangeCallbackDisabled)
                return;

            /* Do we still think that it is running? It may happen if this is a
             * VM-(guest-)initiated shutdown/poweroff.
             */
            if (   that->mMachineState != MachineState_Stopping
                && that->mMachineState != MachineState_Saving
                && that->mMachineState != MachineState_Restoring
                && that->mMachineState != MachineState_TeleportingIn
                && that->mMachineState != MachineState_FaultTolerantSyncing
                && that->mMachineState != MachineState_TeleportingPausedVM
                && !that->mVMIsAlreadyPoweringOff
               )
            {
                LogFlowFunc(("VM has powered itself off but Console still thinks it is running. Notifying.\n"));

                /*
                 * Prevent powerDown() from calling VMR3PowerOff() again if this was called from
                 * the power off state change.
                 * When called from the Reset state make sure to call VMR3PowerOff() first.
                 */
                Assert(that->mVMPoweredOff == false);
                that->mVMPoweredOff = true;

                /*
                 * request a progress object from the server
                 * (this will set the machine state to Stopping on the server
                 * to block others from accessing this machine)
                 */
                ComPtr<IProgress> pProgress;
                HRESULT rc = that->mControl->BeginPoweringDown(pProgress.asOutParam());
                AssertComRC(rc);

                /* sync the state with the server */
                that->i_setMachineStateLocally(MachineState_Stopping);

                /* Setup task object and thread to carry out the operation
                 * asynchronously (if we call powerDown() right here but there
                 * is one or more mpUVM callers (added with addVMCaller()) we'll
                 * deadlock).
                 */
                VMPowerDownTask* task = NULL;
                try
                {
                    task = new VMPowerDownTask(that, pProgress);
                     /* If creating a task failed, this can currently mean one of
                      * two: either Console::uninit() has been called just a ms
                      * before (so a powerDown() call is already on the way), or
                      * powerDown() itself is being already executed. Just do
                      * nothing.
                      */
                    if (!task->isOk())
                    {
                        LogFlowFunc(("Console is already being uninitialized. \n"));
                        throw E_FAIL;
                    }
                }
                catch(...)
                {
                    delete task;
                    LogFlowFunc(("Problem with creating VMPowerDownTask object. \n"));
                }

                rc = task->createThread();

                if (FAILED(rc))
                {
                    LogFlowFunc(("Problem with creating thread for VMPowerDownTask. \n"));
                }

            }
            break;
        }

        /* The VM has been completely destroyed.
         *
         * Note: This state change can happen at two points:
         *       1) At the end of VMR3Destroy() if it was not called from EMT.
         *       2) At the end of vmR3EmulationThread if VMR3Destroy() was
         *          called by EMT.
         */
        case VMSTATE_TERMINATED:
        {
            AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);

            if (that->mVMStateChangeCallbackDisabled)
                break;

            /* Terminate host interface networking. If pUVM is NULL, we've been
             * manually called from powerUpThread() either before calling
             * VMR3Create() or after VMR3Create() failed, so no need to touch
             * networking.
             */
            if (pUVM)
                that->i_powerDownHostInterfaces();

            /* From now on the machine is officially powered down or remains in
             * the Saved state.
             */
            switch (that->mMachineState)
            {
                default:
                    AssertFailed();
                    RT_FALL_THRU();
                case MachineState_Stopping:
                    /* successfully powered down */
                    that->i_setMachineState(MachineState_PoweredOff);
                    break;
                case MachineState_Saving:
                    /* successfully saved */
                    that->i_setMachineState(MachineState_Saved);
                    break;
                case MachineState_Starting:
                    /* failed to start, but be patient: set back to PoweredOff
                     * (for similarity with the below) */
                    that->i_setMachineState(MachineState_PoweredOff);
                    break;
                case MachineState_Restoring:
                    /* failed to load the saved state file, but be patient: set
                     * back to Saved (to preserve the saved state file) */
                    that->i_setMachineState(MachineState_Saved);
                    break;
                case MachineState_TeleportingIn:
                    /* Teleportation failed or was canceled.  Back to powered off. */
                    that->i_setMachineState(MachineState_PoweredOff);
                    break;
                case MachineState_TeleportingPausedVM:
                    /* Successfully teleported the VM. */
                    that->i_setMachineState(MachineState_Teleported);
                    break;
                case MachineState_FaultTolerantSyncing:
                    /* Fault tolerant sync failed or was canceled.  Back to powered off. */
                    that->i_setMachineState(MachineState_PoweredOff);
                    break;
            }
            break;
        }

        case VMSTATE_RESETTING:
        /** @todo shouldn't VMSTATE_RESETTING_LS be here?   */
        {
#ifdef VBOX_WITH_GUEST_PROPS
            /* Do not take any read/write locks here! */
            that->i_guestPropertiesHandleVMReset();
#endif
            break;
        }

        case VMSTATE_SOFT_RESETTING:
        case VMSTATE_SOFT_RESETTING_LS:
            /* Shouldn't do anything here! */
            break;

        case VMSTATE_SUSPENDED:
        {
            AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);

            if (that->mVMStateChangeCallbackDisabled)
                break;

            switch (that->mMachineState)
            {
                case MachineState_Teleporting:
                    that->i_setMachineState(MachineState_TeleportingPausedVM);
                    break;

                case MachineState_LiveSnapshotting:
                    that->i_setMachineState(MachineState_OnlineSnapshotting);
                    break;

                case MachineState_TeleportingPausedVM:
                case MachineState_Saving:
                case MachineState_Restoring:
                case MachineState_Stopping:
                case MachineState_TeleportingIn:
                case MachineState_FaultTolerantSyncing:
                case MachineState_OnlineSnapshotting:
                    /* The worker thread handles the transition. */
                    break;

                case MachineState_Running:
                    that->i_setMachineState(MachineState_Paused);
                    break;

                case MachineState_Paused:
                    /* Nothing to do. */
                    break;

                default:
                    AssertMsgFailed(("%s\n", Global::stringifyMachineState(that->mMachineState)));
            }
            break;
        }

        case VMSTATE_SUSPENDED_LS:
        case VMSTATE_SUSPENDED_EXT_LS:
        {
            AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);
            if (that->mVMStateChangeCallbackDisabled)
                break;
            switch (that->mMachineState)
            {
                case MachineState_Teleporting:
                    that->i_setMachineState(MachineState_TeleportingPausedVM);
                    break;

                case MachineState_LiveSnapshotting:
                    that->i_setMachineState(MachineState_OnlineSnapshotting);
                    break;

                case MachineState_TeleportingPausedVM:
                case MachineState_Saving:
                    /* ignore */
                    break;

                default:
                    AssertMsgFailed(("%s/%s -> %s\n", Global::stringifyMachineState(that->mMachineState),
                                    VMR3GetStateName(enmOldState), VMR3GetStateName(enmState) ));
                    that->i_setMachineState(MachineState_Paused);
                    break;
            }
            break;
        }

        case VMSTATE_RUNNING:
        {
            if (   enmOldState == VMSTATE_POWERING_ON
                || enmOldState == VMSTATE_RESUMING
                || enmOldState == VMSTATE_RUNNING_FT)
            {
                AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);

                if (that->mVMStateChangeCallbackDisabled)
                    break;

                Assert(   (   (   that->mMachineState == MachineState_Starting
                               || that->mMachineState == MachineState_Paused)
                           && enmOldState == VMSTATE_POWERING_ON)
                       || (   (   that->mMachineState == MachineState_Restoring
                               || that->mMachineState == MachineState_TeleportingIn
                               || that->mMachineState == MachineState_Paused
                               || that->mMachineState == MachineState_Saving
                              )
                           && enmOldState == VMSTATE_RESUMING)
                       || (   that->mMachineState == MachineState_FaultTolerantSyncing
                           && enmOldState == VMSTATE_RUNNING_FT));

                that->i_setMachineState(MachineState_Running);
            }

            break;
        }

        case VMSTATE_RUNNING_LS:
            AssertMsg(   that->mMachineState == MachineState_LiveSnapshotting
                      || that->mMachineState == MachineState_Teleporting,
                      ("%s/%s -> %s\n", Global::stringifyMachineState(that->mMachineState),
                      VMR3GetStateName(enmOldState), VMR3GetStateName(enmState) ));
            break;

        case VMSTATE_RUNNING_FT:
            AssertMsg(that->mMachineState == MachineState_FaultTolerantSyncing,
                      ("%s/%s -> %s\n", Global::stringifyMachineState(that->mMachineState),
                      VMR3GetStateName(enmOldState), VMR3GetStateName(enmState) ));
            break;

        case VMSTATE_FATAL_ERROR:
        {
            AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);

            if (that->mVMStateChangeCallbackDisabled)
                break;

            /* Fatal errors are only for running VMs. */
            Assert(Global::IsOnline(that->mMachineState));

            /* Note! 'Pause' is used here in want of something better.  There
             *       are currently only two places where fatal errors might be
             *       raised, so it is not worth adding a new externally
             *       visible state for this yet.  */
            that->i_setMachineState(MachineState_Paused);
            break;
        }

        case VMSTATE_GURU_MEDITATION:
        {
            AutoWriteLock alock(that COMMA_LOCKVAL_SRC_POS);

            if (that->mVMStateChangeCallbackDisabled)
                break;

            /* Guru are only for running VMs */
            Assert(Global::IsOnline(that->mMachineState));

            that->i_setMachineState(MachineState_Stuck);
            break;
        }

        case VMSTATE_CREATED:
        {
            /*
             * We have to set the secret key helper interface for the VD drivers to
             * get notified about missing keys.
             */
            that->i_initSecretKeyIfOnAllAttachments();
            break;
        }

        default: /* shut up gcc */
            break;
    }
}

/**
 * Changes the clipboard mode.
 *
 * @param aClipboardMode  new clipboard mode.
 */
void Console::i_changeClipboardMode(ClipboardMode_T aClipboardMode)
{
    VMMDev *pVMMDev = m_pVMMDev;
    Assert(pVMMDev);

    VBOXHGCMSVCPARM parm;
    parm.type = VBOX_HGCM_SVC_PARM_32BIT;

    switch (aClipboardMode)
    {
        default:
        case ClipboardMode_Disabled:
            LogRel(("Shared clipboard mode: Off\n"));
            parm.u.uint32 = VBOX_SHARED_CLIPBOARD_MODE_OFF;
            break;
        case ClipboardMode_GuestToHost:
            LogRel(("Shared clipboard mode: Guest to Host\n"));
            parm.u.uint32 = VBOX_SHARED_CLIPBOARD_MODE_GUEST_TO_HOST;
            break;
        case ClipboardMode_HostToGuest:
            LogRel(("Shared clipboard mode: Host to Guest\n"));
            parm.u.uint32 = VBOX_SHARED_CLIPBOARD_MODE_HOST_TO_GUEST;
            break;
        case ClipboardMode_Bidirectional:
            LogRel(("Shared clipboard mode: Bidirectional\n"));
            parm.u.uint32 = VBOX_SHARED_CLIPBOARD_MODE_BIDIRECTIONAL;
            break;
    }

    pVMMDev->hgcmHostCall("VBoxSharedClipboard", VBOX_SHARED_CLIPBOARD_HOST_FN_SET_MODE, 1, &parm);
}

/**
 * Changes the drag and drop mode.
 *
 * @param aDnDMode  new drag and drop mode.
 */
int Console::i_changeDnDMode(DnDMode_T aDnDMode)
{
    VMMDev *pVMMDev = m_pVMMDev;
    AssertPtrReturn(pVMMDev, VERR_INVALID_POINTER);

    VBOXHGCMSVCPARM parm;
    RT_ZERO(parm);
    parm.type = VBOX_HGCM_SVC_PARM_32BIT;

    switch (aDnDMode)
    {
        default:
        case DnDMode_Disabled:
            LogRel(("Drag and drop mode: Off\n"));
            parm.u.uint32 = VBOX_DRAG_AND_DROP_MODE_OFF;
            break;
        case DnDMode_GuestToHost:
            LogRel(("Drag and drop mode: Guest to Host\n"));
            parm.u.uint32 = VBOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST;
            break;
        case DnDMode_HostToGuest:
            LogRel(("Drag and drop mode: Host to Guest\n"));
            parm.u.uint32 = VBOX_DRAG_AND_DROP_MODE_HOST_TO_GUEST;
            break;
        case DnDMode_Bidirectional:
            LogRel(("Drag and drop mode: Bidirectional\n"));
            parm.u.uint32 = VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL;
            break;
    }

    int rc = pVMMDev->hgcmHostCall("VBoxDragAndDropSvc",
                                   DragAndDropSvc::HOST_DND_SET_MODE, 1 /* cParms */, &parm);
    if (RT_FAILURE(rc))
        LogRel(("Error changing drag and drop mode: %Rrc\n", rc));

    return rc;
}

#ifdef VBOX_WITH_USB
/**
 * Sends a request to VMM to attach the given host device.
 * After this method succeeds, the attached device will appear in the
 * mUSBDevices collection.
 *
 * @param aHostDevice  device to attach
 *
 * @note Synchronously calls EMT.
 */
HRESULT Console::i_attachUSBDevice(IUSBDevice *aHostDevice, ULONG aMaskedIfs,
                                   const Utf8Str &aCaptureFilename)
{
    AssertReturn(aHostDevice, E_FAIL);
    AssertReturn(!isWriteLockOnCurrentThread(), E_FAIL);

    HRESULT hrc;

    /*
     * Get the address and the Uuid, and call the pfnCreateProxyDevice roothub
     * method in EMT (using usbAttachCallback()).
     */
    Bstr BstrAddress;
    hrc = aHostDevice->COMGETTER(Address)(BstrAddress.asOutParam());
    ComAssertComRCRetRC(hrc);

    Utf8Str Address(BstrAddress);

    Bstr id;
    hrc = aHostDevice->COMGETTER(Id)(id.asOutParam());
    ComAssertComRCRetRC(hrc);
    Guid uuid(id);

    BOOL fRemote = FALSE;
    hrc = aHostDevice->COMGETTER(Remote)(&fRemote);
    ComAssertComRCRetRC(hrc);

    Bstr BstrBackend;
    hrc = aHostDevice->COMGETTER(Backend)(BstrBackend.asOutParam());
    ComAssertComRCRetRC(hrc);

    Utf8Str Backend(BstrBackend);

    /* Get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    LogFlowThisFunc(("Proxying USB device '%s' {%RTuuid}...\n",
                      Address.c_str(), uuid.raw()));

    void *pvRemoteBackend = NULL;
    if (fRemote)
    {
        RemoteUSBDevice *pRemoteUSBDevice = static_cast<RemoteUSBDevice *>(aHostDevice);
        pvRemoteBackend = i_consoleVRDPServer()->USBBackendRequestPointer(pRemoteUSBDevice->clientId(), &uuid);
        if (!pvRemoteBackend)
            return E_INVALIDARG; /* The clientId is invalid then. */
    }

    USHORT portVersion = 0;
    hrc = aHostDevice->COMGETTER(PortVersion)(&portVersion);
    AssertComRCReturnRC(hrc);
    Assert(portVersion == 1 || portVersion == 2 || portVersion == 3);

    int vrc = VMR3ReqCallWaitU(ptrVM.rawUVM(), 0 /* idDstCpu (saved state, see #6232) */,
                               (PFNRT)i_usbAttachCallback, 10,
                               this, ptrVM.rawUVM(), aHostDevice, uuid.raw(), Backend.c_str(),
                               Address.c_str(), pvRemoteBackend, portVersion, aMaskedIfs,
                               aCaptureFilename.isEmpty() ? NULL : aCaptureFilename.c_str());
    if (RT_SUCCESS(vrc))
    {
        /* Create a OUSBDevice and add it to the device list */
        ComObjPtr<OUSBDevice> pUSBDevice;
        pUSBDevice.createObject();
        hrc = pUSBDevice->init(aHostDevice);
        AssertComRC(hrc);

        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        mUSBDevices.push_back(pUSBDevice);
        LogFlowFunc(("Attached device {%RTuuid}\n", pUSBDevice->i_id().raw()));

        /* notify callbacks */
        alock.release();
        i_onUSBDeviceStateChange(pUSBDevice, true /* aAttached */, NULL);
    }
    else
    {
        Log1WarningThisFunc(("Failed to create proxy device for '%s' {%RTuuid} (%Rrc)\n", Address.c_str(), uuid.raw(), vrc));

        switch (vrc)
        {
            case VERR_VUSB_NO_PORTS:
                hrc = setError(E_FAIL, tr("Failed to attach the USB device. (No available ports on the USB controller)."));
                break;
            case VERR_VUSB_USBFS_PERMISSION:
                hrc = setError(E_FAIL, tr("Not permitted to open the USB device, check usbfs options"));
                break;
            default:
                hrc = setError(E_FAIL, tr("Failed to create a proxy device for the USB device. (Error: %Rrc)"), vrc);
                break;
        }
    }

    return hrc;
}

/**
 * USB device attach callback used by AttachUSBDevice().
 * Note that AttachUSBDevice() doesn't return until this callback is executed,
 * so we don't use AutoCaller and don't care about reference counters of
 * interface pointers passed in.
 *
 * @thread EMT
 * @note Locks the console object for writing.
 */
//static
DECLCALLBACK(int)
Console::i_usbAttachCallback(Console *that, PUVM pUVM, IUSBDevice *aHostDevice, PCRTUUID aUuid, const char *pszBackend,
                             const char *aAddress, void *pvRemoteBackend, USHORT aPortVersion, ULONG aMaskedIfs,
                             const char *pszCaptureFilename)
{
    RT_NOREF(aHostDevice);
    LogFlowFuncEnter();
    LogFlowFunc(("that={%p} aUuid={%RTuuid}\n", that, aUuid));

    AssertReturn(that && aUuid, VERR_INVALID_PARAMETER);
    AssertReturn(!that->isWriteLockOnCurrentThread(), VERR_GENERAL_FAILURE);

    int vrc = PDMR3UsbCreateProxyDevice(pUVM, aUuid, pszBackend, aAddress, pvRemoteBackend,
                                        aPortVersion == 3 ? VUSB_STDVER_30 :
                                        aPortVersion == 2 ? VUSB_STDVER_20 : VUSB_STDVER_11,
                                        aMaskedIfs, pszCaptureFilename);
    LogFlowFunc(("vrc=%Rrc\n", vrc));
    LogFlowFuncLeave();
    return vrc;
}

/**
 * Sends a request to VMM to detach the given host device.  After this method
 * succeeds, the detached device will disappear from the mUSBDevices
 * collection.
 *
 * @param aHostDevice  device to attach
 *
 * @note Synchronously calls EMT.
 */
HRESULT Console::i_detachUSBDevice(const ComObjPtr<OUSBDevice> &aHostDevice)
{
    AssertReturn(!isWriteLockOnCurrentThread(), E_FAIL);

    /* Get the VM handle. */
    SafeVMPtr ptrVM(this);
    if (!ptrVM.isOk())
        return ptrVM.rc();

    /* if the device is attached, then there must at least one USB hub. */
    AssertReturn(PDMR3UsbHasHub(ptrVM.rawUVM()), E_FAIL);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    LogFlowThisFunc(("Detaching USB proxy device {%RTuuid}...\n",
                     aHostDevice->i_id().raw()));

    /*
     * If this was a remote device, release the backend pointer.
     * The pointer was requested in usbAttachCallback.
     */
    BOOL fRemote = FALSE;

    HRESULT hrc2 = aHostDevice->COMGETTER(Remote)(&fRemote);
    if (FAILED(hrc2))
        i_setErrorStatic(hrc2, "GetRemote() failed");

    PCRTUUID pUuid = aHostDevice->i_id().raw();
    if (fRemote)
    {
        Guid guid(*pUuid);
        i_consoleVRDPServer()->USBBackendReleasePointer(&guid);
    }

    alock.release();
    int vrc = VMR3ReqCallWaitU(ptrVM.rawUVM(), 0 /* idDstCpu (saved state, see #6232) */,
                               (PFNRT)i_usbDetachCallback, 5,
                               this, ptrVM.rawUVM(), pUuid);
    if (RT_SUCCESS(vrc))
    {
        LogFlowFunc(("Detached device {%RTuuid}\n", pUuid));

        /* notify callbacks */
        i_onUSBDeviceStateChange(aHostDevice, false /* aAttached */, NULL);
    }

    ComAssertRCRet(vrc, E_FAIL);

    return S_OK;
}

/**
 * USB device detach callback used by DetachUSBDevice().
 *
 * Note that DetachUSBDevice() doesn't return until this callback is executed,
 * so we don't use AutoCaller and don't care about reference counters of
 * interface pointers passed in.
 *
 * @thread EMT
 */
//static
DECLCALLBACK(int)
Console::i_usbDetachCallback(Console *that, PUVM pUVM, PCRTUUID aUuid)
{
    LogFlowFuncEnter();
    LogFlowFunc(("that={%p} aUuid={%RTuuid}\n", that, aUuid));

    AssertReturn(that && aUuid, VERR_INVALID_PARAMETER);
    AssertReturn(!that->isWriteLockOnCurrentThread(), VERR_GENERAL_FAILURE);

    int vrc = PDMR3UsbDetachDevice(pUVM, aUuid);

    LogFlowFunc(("vrc=%Rrc\n", vrc));
    LogFlowFuncLeave();
    return vrc;
}
#endif /* VBOX_WITH_USB */

/* Note: FreeBSD needs this whether netflt is used or not. */
#if ((defined(RT_OS_LINUX) && !defined(VBOX_WITH_NETFLT)) || defined(RT_OS_FREEBSD))
/**
 * Helper function to handle host interface device creation and attachment.
 *
 * @param   networkAdapter the network adapter which attachment should be reset
 * @return  COM status code
 *
 * @note The caller must lock this object for writing.
 *
 * @todo Move this back into the driver!
 */
HRESULT Console::i_attachToTapInterface(INetworkAdapter *networkAdapter)
{
    LogFlowThisFunc(("\n"));
    /* sanity check */
    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

# ifdef VBOX_STRICT
    /* paranoia */
    NetworkAttachmentType_T attachment;
    networkAdapter->COMGETTER(AttachmentType)(&attachment);
    Assert(attachment == NetworkAttachmentType_Bridged);
# endif /* VBOX_STRICT */

    HRESULT rc = S_OK;

    ULONG slot = 0;
    rc = networkAdapter->COMGETTER(Slot)(&slot);
    AssertComRC(rc);

# ifdef RT_OS_LINUX
    /*
     * Allocate a host interface device
     */
    int rcVBox = RTFileOpen(&maTapFD[slot], "/dev/net/tun",
                            RTFILE_O_READWRITE | RTFILE_O_OPEN | RTFILE_O_DENY_NONE | RTFILE_O_INHERIT);
    if (RT_SUCCESS(rcVBox))
    {
        /*
         * Set/obtain the tap interface.
         */
        struct ifreq IfReq;
        RT_ZERO(IfReq);
        /* The name of the TAP interface we are using */
        Bstr tapDeviceName;
        rc = networkAdapter->COMGETTER(BridgedInterface)(tapDeviceName.asOutParam());
        if (FAILED(rc))
            tapDeviceName.setNull(); /* Is this necessary? */
        if (tapDeviceName.isEmpty())
        {
            LogRel(("No TAP device name was supplied.\n"));
            rc = setError(E_FAIL, tr("No TAP device name was supplied for the host networking interface"));
        }

        if (SUCCEEDED(rc))
        {
            /* If we are using a static TAP device then try to open it. */
            Utf8Str str(tapDeviceName);
            RTStrCopy(IfReq.ifr_name, sizeof(IfReq.ifr_name), str.c_str()); /** @todo bitch about names which are too long... */
            IfReq.ifr_flags = IFF_TAP | IFF_NO_PI;
            rcVBox = ioctl(RTFileToNative(maTapFD[slot]), TUNSETIFF, &IfReq);
            if (rcVBox != 0)
            {
                LogRel(("Failed to open the host network interface %ls\n", tapDeviceName.raw()));
                rc = setError(E_FAIL,
                    tr("Failed to open the host network interface %ls"),
                    tapDeviceName.raw());
            }
        }
        if (SUCCEEDED(rc))
        {
            /*
             * Make it pollable.
             */
            if (fcntl(RTFileToNative(maTapFD[slot]), F_SETFL, O_NONBLOCK) != -1)
            {
                Log(("i_attachToTapInterface: %RTfile %ls\n", maTapFD[slot], tapDeviceName.raw()));
                /*
                 * Here is the right place to communicate the TAP file descriptor and
                 * the host interface name to the server if/when it becomes really
                 * necessary.
                 */
                maTAPDeviceName[slot] = tapDeviceName;
                rcVBox = VINF_SUCCESS;
            }
            else
            {
                int iErr = errno;

                LogRel(("Configuration error: Failed to configure /dev/net/tun non blocking. Error: %s\n", strerror(iErr)));
                rcVBox = VERR_HOSTIF_BLOCKING;
                rc = setError(E_FAIL,
                    tr("could not set up the host networking device for non blocking access: %s"),
                    strerror(errno));
            }
        }
    }
    else
    {
        LogRel(("Configuration error: Failed to open /dev/net/tun rc=%Rrc\n", rcVBox));
        switch (rcVBox)
        {
            case VERR_ACCESS_DENIED:
                /* will be handled by our caller */
                rc = rcVBox;
                break;
            default:
                rc = setError(E_FAIL,
                    tr("Could not set up the host networking device: %Rrc"),
                    rcVBox);
                break;
        }
    }

# elif defined(RT_OS_FREEBSD)
    /*
     * Set/obtain the tap interface.
     */
    /* The name of the TAP interface we are using */
    Bstr tapDeviceName;
    rc = networkAdapter->COMGETTER(BridgedInterface)(tapDeviceName.asOutParam());
    if (FAILED(rc))
        tapDeviceName.setNull(); /* Is this necessary? */
    if (tapDeviceName.isEmpty())
    {
        LogRel(("No TAP device name was supplied.\n"));
        rc = setError(E_FAIL, tr("No TAP device name was supplied for the host networking interface"));
    }
    char szTapdev[1024] = "/dev/";
    /* If we are using a static TAP device then try to open it. */
    Utf8Str str(tapDeviceName);
    if (str.length() + strlen(szTapdev) <= sizeof(szTapdev))
        strcat(szTapdev, str.c_str());
    else
        memcpy(szTapdev + strlen(szTapdev), str.c_str(),
               sizeof(szTapdev) - strlen(szTapdev) - 1); /** @todo bitch about names which are too long... */
    int rcVBox = RTFileOpen(&maTapFD[slot], szTapdev,
                            RTFILE_O_READWRITE | RTFILE_O_OPEN | RTFILE_O_DENY_NONE | RTFILE_O_INHERIT | RTFILE_O_NON_BLOCK);

    if (RT_SUCCESS(rcVBox))
        maTAPDeviceName[slot] = tapDeviceName;
    else
    {
        switch (rcVBox)
        {
            case VERR_ACCESS_DENIED:
                /* will be handled by our caller */
                rc = rcVBox;
                break;
            default:
                rc = setError(E_FAIL,
                    tr("Failed to open the host network interface %ls"),
                    tapDeviceName.raw());
                break;
        }
    }
# else
#  error "huh?"
# endif
    /* in case of failure, cleanup. */
    if (RT_FAILURE(rcVBox) && SUCCEEDED(rc))
    {
        LogRel(("General failure attaching to host interface\n"));
        rc = setError(E_FAIL,
            tr("General failure attaching to host interface"));
    }
    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    return rc;
}


/**
 * Helper function to handle detachment from a host interface
 *
 * @param   networkAdapter the network adapter which attachment should be reset
 * @return  COM status code
 *
 * @note The caller must lock this object for writing.
 *
 * @todo Move this back into the driver!
 */
HRESULT Console::i_detachFromTapInterface(INetworkAdapter *networkAdapter)
{
    /* sanity check */
    LogFlowThisFunc(("\n"));
    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

    HRESULT rc = S_OK;
# ifdef VBOX_STRICT
    /* paranoia */
    NetworkAttachmentType_T attachment;
    networkAdapter->COMGETTER(AttachmentType)(&attachment);
    Assert(attachment == NetworkAttachmentType_Bridged);
# endif /* VBOX_STRICT */

    ULONG slot = 0;
    rc = networkAdapter->COMGETTER(Slot)(&slot);
    AssertComRC(rc);

    /* is there an open TAP device? */
    if (maTapFD[slot] != NIL_RTFILE)
    {
        /*
         * Close the file handle.
         */
        Bstr tapDeviceName, tapTerminateApplication;
        bool isStatic = true;
        rc = networkAdapter->COMGETTER(BridgedInterface)(tapDeviceName.asOutParam());
        if (FAILED(rc) || tapDeviceName.isEmpty())
        {
            /* If the name is empty, this is a dynamic TAP device, so close it now,
               so that the termination script can remove the interface. Otherwise we still
               need the FD to pass to the termination script. */
            isStatic = false;
            int rcVBox = RTFileClose(maTapFD[slot]);
            AssertRC(rcVBox);
            maTapFD[slot] = NIL_RTFILE;
        }
        if (isStatic)
        {
            /* If we are using a static TAP device, we close it now, after having called the
               termination script. */
            int rcVBox = RTFileClose(maTapFD[slot]);
            AssertRC(rcVBox);
        }
        /* the TAP device name and handle are no longer valid */
        maTapFD[slot] = NIL_RTFILE;
        maTAPDeviceName[slot] = "";
    }
    LogFlowThisFunc(("returning %d\n", rc));
    return rc;
}
#endif /* (RT_OS_LINUX || RT_OS_FREEBSD) && !VBOX_WITH_NETFLT */

/**
 * Called at power down to terminate host interface networking.
 *
 * @note The caller must lock this object for writing.
 */
HRESULT Console::i_powerDownHostInterfaces()
{
    LogFlowThisFunc(("\n"));

    /* sanity check */
    AssertReturn(isWriteLockOnCurrentThread(), E_FAIL);

    /*
     * host interface termination handling
     */
    HRESULT rc = S_OK;
    ComPtr<IVirtualBox> pVirtualBox;
    mMachine->COMGETTER(Parent)(pVirtualBox.asOutParam());
    ComPtr<ISystemProperties> pSystemProperties;
    if (pVirtualBox)
        pVirtualBox->COMGETTER(SystemProperties)(pSystemProperties.asOutParam());
    ChipsetType_T chipsetType = ChipsetType_PIIX3;
    mMachine->COMGETTER(ChipsetType)(&chipsetType);
    ULONG maxNetworkAdapters = 0;
    if (pSystemProperties)
        pSystemProperties->GetMaxNetworkAdapters(chipsetType, &maxNetworkAdapters);

    for (ULONG slot = 0; slot < maxNetworkAdapters; slot++)
    {
        ComPtr<INetworkAdapter> pNetworkAdapter;
        rc = mMachine->GetNetworkAdapter(slot, pNetworkAdapter.asOutParam());
        if (FAILED(rc)) break;

        BOOL enabled = FALSE;
        pNetworkAdapter->COMGETTER(Enabled)(&enabled);
        if (!enabled)
            continue;

        NetworkAttachmentType_T attachment;
        pNetworkAdapter->COMGETTER(AttachmentType)(&attachment);
        if (attachment == NetworkAttachmentType_Bridged)
        {
#if ((defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)) && !defined(VBOX_WITH_NETFLT))
            HRESULT rc2 = i_detachFromTapInterface(pNetworkAdapter);
            if (FAILED(rc2) && SUCCEEDED(rc))
                rc = rc2;
#endif /* (RT_OS_LINUX || RT_OS_FREEBSD) && !VBOX_WITH_NETFLT */
        }
    }

    return rc;
}


/**
 * Process callback handler for VMR3LoadFromFile, VMR3LoadFromStream, VMR3Save
 * and VMR3Teleport.
 *
 * @param   pUVM        The user mode VM handle.
 * @param   uPercent    Completion percentage (0-100).
 * @param   pvUser      Pointer to an IProgress instance.
 * @return  VINF_SUCCESS.
 */
/*static*/
DECLCALLBACK(int) Console::i_stateProgressCallback(PUVM pUVM, unsigned uPercent, void *pvUser)
{
    IProgress *pProgress = static_cast<IProgress *>(pvUser);

    /* update the progress object */
    if (pProgress)
        pProgress->SetCurrentOperationProgress(uPercent);

    NOREF(pUVM);
    return VINF_SUCCESS;
}

/**
 * @copydoc FNVMATERROR
 *
 * @remarks Might be some tiny serialization concerns with access to the string
 *          object here...
 */
/*static*/ DECLCALLBACK(void)
Console::i_genericVMSetErrorCallback(PUVM pUVM, void *pvUser, int rc, RT_SRC_POS_DECL,
                                     const char *pszFormat, va_list args)
{
    RT_SRC_POS_NOREF();
    Utf8Str *pErrorText = (Utf8Str *)pvUser;
    AssertPtr(pErrorText);

    /* We ignore RT_SRC_POS_DECL arguments to avoid confusion of end-users. */
    va_list va2;
    va_copy(va2, args);

    /* Append to any the existing error message. */
    if (pErrorText->length())
        *pErrorText = Utf8StrFmt("%s.\n%N (%Rrc)", pErrorText->c_str(),
                                 pszFormat, &va2, rc, rc);
    else
        *pErrorText = Utf8StrFmt("%N (%Rrc)", pszFormat, &va2, rc, rc);

    va_end(va2);

    NOREF(pUVM);
}

/**
 * VM runtime error callback function (FNVMATRUNTIMEERROR).
 *
 * See VMSetRuntimeError for the detailed description of parameters.
 *
 * @param   pUVM            The user mode VM handle.  Ignored, so passing NULL
 *                          is fine.
 * @param   pvUser          The user argument, pointer to the Console instance.
 * @param   fFlags          The action flags. See VMSETRTERR_FLAGS_*.
 * @param   pszErrorId      Error ID string.
 * @param   pszFormat       Error message format string.
 * @param   va              Error message arguments.
 * @thread EMT.
 */
/* static */ DECLCALLBACK(void)
Console::i_atVMRuntimeErrorCallback(PUVM pUVM, void *pvUser, uint32_t fFlags,
                                    const char *pszErrorId, const char *pszFormat, va_list va)
{
    bool const fFatal = !!(fFlags & VMSETRTERR_FLAGS_FATAL);
    LogFlowFuncEnter();

    Console *that = static_cast<Console *>(pvUser);
    AssertReturnVoid(that);

    Utf8Str message(pszFormat, va);

    LogRel(("Console: VM runtime error: fatal=%RTbool, errorID=%s message=\"%s\"\n",
            fFatal, pszErrorId, message.c_str()));

    that->i_onRuntimeError(BOOL(fFatal), Bstr(pszErrorId).raw(), Bstr(message).raw());

    LogFlowFuncLeave(); NOREF(pUVM);
}

/**
 * Captures USB devices that match filters of the VM.
 * Called at VM startup.
 *
 * @param   pUVM    The VM handle.
 */
HRESULT Console::i_captureUSBDevices(PUVM pUVM)
{
    RT_NOREF(pUVM);
    LogFlowThisFunc(("\n"));

    /* sanity check */
    AssertReturn(!isWriteLockOnCurrentThread(), E_FAIL);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* If the machine has a USB controller, ask the USB proxy service to
     * capture devices */
    if (mfVMHasUsbController)
    {
        /* release the lock before calling Host in VBoxSVC since Host may call
         * us back from under its lock (e.g. onUSBDeviceAttach()) which would
         * produce an inter-process dead-lock otherwise. */
        alock.release();

        HRESULT hrc = mControl->AutoCaptureUSBDevices();
        ComAssertComRCRetRC(hrc);
    }

    return S_OK;
}


/**
 * Detach all USB device which are attached to the VM for the
 * purpose of clean up and such like.
 */
void Console::i_detachAllUSBDevices(bool aDone)
{
    LogFlowThisFunc(("aDone=%RTbool\n", aDone));

    /* sanity check */
    AssertReturnVoid(!isWriteLockOnCurrentThread());
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mUSBDevices.clear();

    /* release the lock before calling Host in VBoxSVC since Host may call
     * us back from under its lock (e.g. onUSBDeviceAttach()) which would
     * produce an inter-process dead-lock otherwise. */
    alock.release();

    mControl->DetachAllUSBDevices(aDone);
}

/**
 * @note Locks this object for writing.
 */
void Console::i_processRemoteUSBDevices(uint32_t u32ClientId, VRDEUSBDEVICEDESC *pDevList, uint32_t cbDevList, bool fDescExt)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("u32ClientId = %d, pDevList=%p, cbDevList = %d, fDescExt = %d\n",
                     u32ClientId, pDevList, cbDevList, fDescExt));

    AutoCaller autoCaller(this);
    if (!autoCaller.isOk())
    {
        /* Console has been already uninitialized, deny request */
        AssertMsgFailed(("Console is already uninitialized\n"));
        LogFlowThisFunc(("Console is already uninitialized\n"));
        LogFlowThisFuncLeave();
        return;
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /*
     * Mark all existing remote USB devices as dirty.
     */
    for (RemoteUSBDeviceList::iterator it = mRemoteUSBDevices.begin();
         it != mRemoteUSBDevices.end();
         ++it)
    {
        (*it)->dirty(true);
    }

    /*
     * Process the pDevList and add devices those are not already in the mRemoteUSBDevices list.
     */
    /** @todo (sunlover) REMOTE_USB Strict validation of the pDevList. */
    VRDEUSBDEVICEDESC *e = pDevList;

    /* The cbDevList condition must be checked first, because the function can
     * receive pDevList = NULL and cbDevList = 0 on client disconnect.
     */
    while (cbDevList >= 2 && e->oNext)
    {
        /* Sanitize incoming strings in case they aren't valid UTF-8. */
        if (e->oManufacturer)
            RTStrPurgeEncoding((char *)e + e->oManufacturer);
        if (e->oProduct)
            RTStrPurgeEncoding((char *)e + e->oProduct);
        if (e->oSerialNumber)
            RTStrPurgeEncoding((char *)e + e->oSerialNumber);

        LogFlowThisFunc(("vendor %04X, product %04X, name = %s\n",
                          e->idVendor, e->idProduct,
                          e->oProduct? (char *)e + e->oProduct: ""));

        bool fNewDevice = true;

        for (RemoteUSBDeviceList::iterator it = mRemoteUSBDevices.begin();
             it != mRemoteUSBDevices.end();
             ++it)
        {
            if ((*it)->devId() == e->id
                && (*it)->clientId() == u32ClientId)
            {
               /* The device is already in the list. */
               (*it)->dirty(false);
               fNewDevice = false;
               break;
            }
        }

        if (fNewDevice)
        {
            LogRel(("Remote USB: ++++ Vendor %04X. Product %04X. Name = [%s].\n",
                    e->idVendor, e->idProduct, e->oProduct? (char *)e + e->oProduct: ""));

            /* Create the device object and add the new device to list. */
            ComObjPtr<RemoteUSBDevice> pUSBDevice;
            pUSBDevice.createObject();
            pUSBDevice->init(u32ClientId, e, fDescExt);

            mRemoteUSBDevices.push_back(pUSBDevice);

            /* Check if the device is ok for current USB filters. */
            BOOL fMatched = FALSE;
            ULONG fMaskedIfs = 0;

            HRESULT hrc = mControl->RunUSBDeviceFilters(pUSBDevice, &fMatched, &fMaskedIfs);

            AssertComRC(hrc);

            LogFlowThisFunc(("USB filters return %d %#x\n", fMatched, fMaskedIfs));

            if (fMatched)
            {
                alock.release();
                hrc = i_onUSBDeviceAttach(pUSBDevice, NULL, fMaskedIfs, Utf8Str());
                alock.acquire();

                /// @todo (r=dmik) warning reporting subsystem

                if (hrc == S_OK)
                {
                    LogFlowThisFunc(("Device attached\n"));
                    pUSBDevice->captured(true);
                }
            }
        }

        if (cbDevList < e->oNext)
        {
            Log1WarningThisFunc(("cbDevList %d > oNext %d\n", cbDevList, e->oNext));
            break;
        }

        cbDevList -= e->oNext;

        e = (VRDEUSBDEVICEDESC *)((uint8_t *)e + e->oNext);
    }

    /*
     * Remove dirty devices, that is those which are not reported by the server anymore.
     */
    for (;;)
    {
        ComObjPtr<RemoteUSBDevice> pUSBDevice;

        RemoteUSBDeviceList::iterator it = mRemoteUSBDevices.begin();
        while (it != mRemoteUSBDevices.end())
        {
            if ((*it)->dirty())
            {
                pUSBDevice = *it;
                break;
            }

            ++it;
        }

        if (!pUSBDevice)
        {
            break;
        }

        USHORT vendorId = 0;
        pUSBDevice->COMGETTER(VendorId)(&vendorId);

        USHORT productId = 0;
        pUSBDevice->COMGETTER(ProductId)(&productId);

        Bstr product;
        pUSBDevice->COMGETTER(Product)(product.asOutParam());

        LogRel(("Remote USB: ---- Vendor %04X. Product %04X. Name = [%ls].\n",
                vendorId, productId, product.raw()));

        /* Detach the device from VM. */
        if (pUSBDevice->captured())
        {
            Bstr uuid;
            pUSBDevice->COMGETTER(Id)(uuid.asOutParam());
            alock.release();
            i_onUSBDeviceDetach(uuid.raw(), NULL);
            alock.acquire();
        }

        /* And remove it from the list. */
        mRemoteUSBDevices.erase(it);
    }

    LogFlowThisFuncLeave();
}

/**
 * Progress cancelation callback for fault tolerance VM poweron
 */
static void faultToleranceProgressCancelCallback(void *pvUser)
{
    PUVM pUVM = (PUVM)pvUser;

    if (pUVM)
        FTMR3CancelStandby(pUVM);
}

/**
 * Worker called by VMPowerUpTask::handler to start the VM (also from saved
 * state) and track progress.
 *
 * @param   pTask       The power up task.
 *
 * @note Locks the Console object for writing.
 */
/*static*/
void Console::i_powerUpThreadTask(VMPowerUpTask *pTask)
{
    LogFlowFuncEnter();

    AssertReturnVoid(pTask);
    AssertReturnVoid(!pTask->mConsole.isNull());
    AssertReturnVoid(!pTask->mProgress.isNull());

    VirtualBoxBase::initializeComForThread();

    HRESULT rc = S_OK;
    int vrc = VINF_SUCCESS;

    /* Set up a build identifier so that it can be seen from core dumps what
     * exact build was used to produce the core. */
    static char saBuildID[48];
    RTStrPrintf(saBuildID, sizeof(saBuildID), "%s%s%s%s VirtualBox %s r%u %s%s%s%s",
                "BU", "IL", "DI", "D", RTBldCfgVersion(), RTBldCfgRevision(), "BU", "IL", "DI", "D");

    ComObjPtr<Console> pConsole = pTask->mConsole;

    /* Note: no need to use AutoCaller because VMPowerUpTask does that */

    /* The lock is also used as a signal from the task initiator (which
     * releases it only after RTThreadCreate()) that we can start the job */
    AutoWriteLock alock(pConsole COMMA_LOCKVAL_SRC_POS);

    /* sanity */
    Assert(pConsole->mpUVM == NULL);

    try
    {
        // Create the VMM device object, which starts the HGCM thread; do this only
        // once for the console, for the pathological case that the same console
        // object is used to power up a VM twice.
        if (!pConsole->m_pVMMDev)
        {
            pConsole->m_pVMMDev = new VMMDev(pConsole);
            AssertReturnVoid(pConsole->m_pVMMDev);
        }

        /* wait for auto reset ops to complete so that we can successfully lock
         * the attached hard disks by calling LockMedia() below */
        for (VMPowerUpTask::ProgressList::const_iterator
             it = pTask->hardDiskProgresses.begin();
             it != pTask->hardDiskProgresses.end(); ++it)
        {
            HRESULT rc2 = (*it)->WaitForCompletion(-1);
            AssertComRC(rc2);

            rc = pTask->mProgress->SetNextOperation(BstrFmt(tr("Disk Image Reset Operation - Immutable Image")).raw(), 1);
            AssertComRCReturnVoid(rc);
        }

        /*
         * Lock attached media. This method will also check their accessibility.
         * If we're a teleporter, we'll have to postpone this action so we can
         * migrate between local processes.
         *
         * Note! The media will be unlocked automatically by
         *       SessionMachine::i_setMachineState() when the VM is powered down.
         */
        if (    !pTask->mTeleporterEnabled
            &&  pTask->mEnmFaultToleranceState != FaultToleranceState_Standby)
        {
            rc = pConsole->mControl->LockMedia();
            if (FAILED(rc)) throw rc;
        }

        /* Create the VRDP server. In case of headless operation, this will
         * also create the framebuffer, required at VM creation.
         */
        ConsoleVRDPServer *server = pConsole->i_consoleVRDPServer();
        Assert(server);

        /* Does VRDP server call Console from the other thread?
         * Not sure (and can change), so release the lock just in case.
         */
        alock.release();
        vrc = server->Launch();
        alock.acquire();

        if (vrc != VINF_SUCCESS)
        {
            Utf8Str errMsg = pConsole->VRDPServerErrorToMsg(vrc);
            if (   RT_FAILURE(vrc)
                && vrc != VERR_NET_ADDRESS_IN_USE) /* not fatal */
                throw i_setErrorStatic(E_FAIL, errMsg.c_str());
        }

        ComPtr<IMachine> pMachine = pConsole->i_machine();
        ULONG cCpus = 1;
        pMachine->COMGETTER(CPUCount)(&cCpus);

        /*
         * Create the VM
         *
         * Note! Release the lock since EMT will call Console. It's safe because
         *       mMachineState is either Starting or Restoring state here.
         */
        alock.release();

        PVM pVM;
        vrc = VMR3Create(cCpus,
                         pConsole->mpVmm2UserMethods,
                         Console::i_genericVMSetErrorCallback,
                         &pTask->mErrorMsg,
                         pTask->mConfigConstructor,
                         static_cast<Console *>(pConsole),
                         &pVM, NULL);

        alock.acquire();

        /* Enable client connections to the server. */
        pConsole->i_consoleVRDPServer()->EnableConnections();

        if (RT_SUCCESS(vrc))
        {
            do
            {
                /*
                 * Register our load/save state file handlers
                 */
                vrc = SSMR3RegisterExternal(pConsole->mpUVM, sSSMConsoleUnit, 0 /*iInstance*/, sSSMConsoleVer, 0 /* cbGuess */,
                                            NULL, NULL, NULL,
                                            NULL, i_saveStateFileExec, NULL,
                                            NULL, i_loadStateFileExec, NULL,
                                            static_cast<Console *>(pConsole));
                AssertRCBreak(vrc);

                vrc = static_cast<Console *>(pConsole)->i_getDisplay()->i_registerSSM(pConsole->mpUVM);
                AssertRC(vrc);
                if (RT_FAILURE(vrc))
                    break;

                /*
                 * Synchronize debugger settings
                 */
                MachineDebugger *machineDebugger = pConsole->i_getMachineDebugger();
                if (machineDebugger)
                    machineDebugger->i_flushQueuedSettings();

                /*
                 * Shared Folders
                 */
                if (pConsole->m_pVMMDev->isShFlActive())
                {
                    /* Does the code below call Console from the other thread?
                     * Not sure, so release the lock just in case. */
                    alock.release();

                    for (SharedFolderDataMap::const_iterator it = pTask->mSharedFolders.begin();
                         it != pTask->mSharedFolders.end();
                         ++it)
                    {
                        const SharedFolderData &d = it->second;
                        rc = pConsole->i_createSharedFolder(it->first, d);
                        if (FAILED(rc))
                        {
                            ErrorInfoKeeper eik;
                            pConsole->i_atVMRuntimeErrorCallbackF(0, "BrokenSharedFolder",
                                   N_("The shared folder '%s' could not be set up: %ls.\n"
                                      "The shared folder setup will not be complete. It is recommended to power down the virtual "
                                      "machine and fix the shared folder settings while the machine is not running"),
                                    it->first.c_str(), eik.getText().raw());
                        }
                    }
                    if (FAILED(rc))
                        rc = S_OK;          // do not fail with broken shared folders

                    /* acquire the lock again */
                    alock.acquire();
                }

                /* release the lock before a lengthy operation */
                alock.release();

                /*
                 * Capture USB devices.
                 */
                rc = pConsole->i_captureUSBDevices(pConsole->mpUVM);
                if (FAILED(rc))
                {
                    alock.acquire();
                    break;
                }

                /* Load saved state? */
                if (pTask->mSavedStateFile.length())
                {
                    LogFlowFunc(("Restoring saved state from '%s'...\n", pTask->mSavedStateFile.c_str()));

                    vrc = VMR3LoadFromFile(pConsole->mpUVM,
                                           pTask->mSavedStateFile.c_str(),
                                           Console::i_stateProgressCallback,
                                           static_cast<IProgress *>(pTask->mProgress));

                    if (RT_SUCCESS(vrc))
                    {
                        if (pTask->mStartPaused)
                            /* done */
                            pConsole->i_setMachineState(MachineState_Paused);
                        else
                        {
                            /* Start/Resume the VM execution */
#ifdef VBOX_WITH_EXTPACK
                            vrc = pConsole->mptrExtPackManager->i_callAllVmPowerOnHooks(pConsole, pVM);
#endif
                            if (RT_SUCCESS(vrc))
                                vrc = VMR3Resume(pConsole->mpUVM, VMRESUMEREASON_STATE_RESTORED);
                            AssertLogRelRC(vrc);
                        }
                    }

                    /* Power off in case we failed loading or resuming the VM */
                    if (RT_FAILURE(vrc))
                    {
                        int vrc2 = VMR3PowerOff(pConsole->mpUVM); AssertLogRelRC(vrc2);
#ifdef VBOX_WITH_EXTPACK
                        pConsole->mptrExtPackManager->i_callAllVmPowerOffHooks(pConsole, pVM);
#endif
                    }
                }
                else if (pTask->mTeleporterEnabled)
                {
                    /* -> ConsoleImplTeleporter.cpp */
                    bool fPowerOffOnFailure;
                    rc = pConsole->i_teleporterTrg(pConsole->mpUVM, pMachine, &pTask->mErrorMsg, pTask->mStartPaused,
                                                   pTask->mProgress, &fPowerOffOnFailure);
                    if (FAILED(rc) && fPowerOffOnFailure)
                    {
                        ErrorInfoKeeper eik;
                        int vrc2 = VMR3PowerOff(pConsole->mpUVM); AssertLogRelRC(vrc2);
#ifdef VBOX_WITH_EXTPACK
                        pConsole->mptrExtPackManager->i_callAllVmPowerOffHooks(pConsole, pVM);
#endif
                    }
                }
                else if (pTask->mEnmFaultToleranceState != FaultToleranceState_Inactive)
                {
                    /*
                     * Get the config.
                     */
                    ULONG uPort;
                    rc = pMachine->COMGETTER(FaultTolerancePort)(&uPort);
                    if (SUCCEEDED(rc))
                    {
                        ULONG uInterval;
                        rc = pMachine->COMGETTER(FaultToleranceSyncInterval)(&uInterval);
                        if (SUCCEEDED(rc))
                        {
                            Bstr bstrAddress;
                            rc = pMachine->COMGETTER(FaultToleranceAddress)(bstrAddress.asOutParam());
                            if (SUCCEEDED(rc))
                            {
                                Bstr bstrPassword;
                                rc = pMachine->COMGETTER(FaultTolerancePassword)(bstrPassword.asOutParam());
                                if (SUCCEEDED(rc))
                                {
                                    if (pTask->mProgress->i_setCancelCallback(faultToleranceProgressCancelCallback,
                                                                              pConsole->mpUVM))
                                    {
                                        if (SUCCEEDED(rc))
                                        {
                                            Utf8Str strAddress(bstrAddress);
                                            const char *pszAddress = strAddress.isEmpty() ? NULL : strAddress.c_str();
                                            Utf8Str strPassword(bstrPassword);
                                            const char *pszPassword = strPassword.isEmpty() ? NULL : strPassword.c_str();

                                            /* Power on the FT enabled VM. */
#ifdef VBOX_WITH_EXTPACK
                                            vrc = pConsole->mptrExtPackManager->i_callAllVmPowerOnHooks(pConsole, pVM);
#endif
                                            if (RT_SUCCESS(vrc))
                                                vrc = FTMR3PowerOn(pConsole->mpUVM,
                                                                   pTask->mEnmFaultToleranceState == FaultToleranceState_Master /* fMaster */,
                                                                   uInterval,
                                                                   pszAddress,
                                                                   uPort,
                                                                   pszPassword);
                                            AssertLogRelRC(vrc);
                                        }
                                        pTask->mProgress->i_setCancelCallback(NULL, NULL);
                                    }
                                    else
                                        rc = E_FAIL;

                                }
                            }
                        }
                    }
                }
                else if (pTask->mStartPaused)
                    /* done */
                    pConsole->i_setMachineState(MachineState_Paused);
                else
                {
                    /* Power on the VM (i.e. start executing) */
#ifdef VBOX_WITH_EXTPACK
                    vrc = pConsole->mptrExtPackManager->i_callAllVmPowerOnHooks(pConsole, pVM);
#endif
                    if (RT_SUCCESS(vrc))
                        vrc = VMR3PowerOn(pConsole->mpUVM);
                    AssertLogRelRC(vrc);
                }

                /* acquire the lock again */
                alock.acquire();
            }
            while (0);

            /* On failure, destroy the VM */
            if (FAILED(rc) || RT_FAILURE(vrc))
            {
                /* preserve existing error info */
                ErrorInfoKeeper eik;

                /* powerDown() will call VMR3Destroy() and do all necessary
                 * cleanup (VRDP, USB devices) */
                alock.release();
                HRESULT rc2 = pConsole->i_powerDown();
                alock.acquire();
                AssertComRC(rc2);
            }
            else
            {
                /*
                 * Deregister the VMSetError callback. This is necessary as the
                 * pfnVMAtError() function passed to VMR3Create() is supposed to
                 * be sticky but our error callback isn't.
                 */
                alock.release();
                VMR3AtErrorDeregister(pConsole->mpUVM, Console::i_genericVMSetErrorCallback, &pTask->mErrorMsg);
                /** @todo register another VMSetError callback? */
                alock.acquire();
            }
        }
        else
        {
            /*
             * If VMR3Create() failed it has released the VM memory.
             */
            VMR3ReleaseUVM(pConsole->mpUVM);
            pConsole->mpUVM = NULL;
        }

        if (SUCCEEDED(rc) && RT_FAILURE(vrc))
        {
            /* If VMR3Create() or one of the other calls in this function fail,
             * an appropriate error message has been set in pTask->mErrorMsg.
             * However since that happens via a callback, the rc status code in
             * this function is not updated.
             */
            if (!pTask->mErrorMsg.length())
            {
                /* If the error message is not set but we've got a failure,
                 * convert the VBox status code into a meaningful error message.
                 * This becomes unused once all the sources of errors set the
                 * appropriate error message themselves.
                 */
                AssertMsgFailed(("Missing error message during powerup for status code %Rrc\n", vrc));
                pTask->mErrorMsg = Utf8StrFmt(tr("Failed to start VM execution (%Rrc)"), vrc);
            }

            /* Set the error message as the COM error.
             * Progress::notifyComplete() will pick it up later. */
            throw i_setErrorStatic(E_FAIL, pTask->mErrorMsg.c_str());
        }
    }
    catch (HRESULT aRC) { rc = aRC; }

    if (   pConsole->mMachineState == MachineState_Starting
        || pConsole->mMachineState == MachineState_Restoring
        || pConsole->mMachineState == MachineState_TeleportingIn
       )
    {
        /* We are still in the Starting/Restoring state. This means one of:
         *
         * 1) we failed before VMR3Create() was called;
         * 2) VMR3Create() failed.
         *
         * In both cases, there is no need to call powerDown(), but we still
         * need to go back to the PoweredOff/Saved state. Reuse
         * vmstateChangeCallback() for that purpose.
         */

        /* preserve existing error info */
        ErrorInfoKeeper eik;

        Assert(pConsole->mpUVM == NULL);
        i_vmstateChangeCallback(NULL, VMSTATE_TERMINATED, VMSTATE_CREATING, pConsole);
    }

    /*
     * Evaluate the final result. Note that the appropriate mMachineState value
     * is already set by vmstateChangeCallback() in all cases.
     */

    /* release the lock, don't need it any more */
    alock.release();

    if (SUCCEEDED(rc))
    {
        /* Notify the progress object of the success */
        pTask->mProgress->i_notifyComplete(S_OK);
    }
    else
    {
        /* The progress object will fetch the current error info */
        pTask->mProgress->i_notifyComplete(rc);
        LogRel(("Power up failed (vrc=%Rrc, rc=%Rhrc (%#08X))\n", vrc, rc, rc));
    }

    /* Notify VBoxSVC and any waiting openRemoteSession progress object. */
    pConsole->mControl->EndPowerUp(rc);

#if defined(RT_OS_WINDOWS)
    /* uninitialize COM */
    CoUninitialize();
#endif

    LogFlowFuncLeave();
}


/**
 * Reconfigures a medium attachment (part of taking or deleting an online snapshot).
 *
 * @param   pThis                   Reference to the console object.
 * @param   pUVM                    The VM handle.
 * @param   pcszDevice              The name of the controller type.
 * @param   uInstance               The instance of the controller.
 * @param   enmBus                  The storage bus type of the controller.
 * @param   fUseHostIOCache         Use the host I/O cache (disable async I/O).
 * @param   fBuiltinIOCache         Use the builtin I/O cache.
 * @param   fInsertDiskIntegrityDrv Flag whether to insert the disk integrity driver into the chain
 *                                  for additionalk debugging aids.
 * @param   fSetupMerge             Whether to set up a medium merge
 * @param   uMergeSource            Merge source image index
 * @param   uMergeTarget            Merge target image index
 * @param   aMediumAtt              The medium attachment.
 * @param   aMachineState           The current machine state.
 * @param   phrc                    Where to store com error - only valid if we return VERR_GENERAL_FAILURE.
 * @return  VBox status code.
 */
/* static */
DECLCALLBACK(int) Console::i_reconfigureMediumAttachment(Console *pThis,
                                                         PUVM pUVM,
                                                         const char *pcszDevice,
                                                         unsigned uInstance,
                                                         StorageBus_T enmBus,
                                                         bool fUseHostIOCache,
                                                         bool fBuiltinIOCache,
                                                         bool fInsertDiskIntegrityDrv,
                                                         bool fSetupMerge,
                                                         unsigned uMergeSource,
                                                         unsigned uMergeTarget,
                                                         IMediumAttachment *aMediumAtt,
                                                         MachineState_T aMachineState,
                                                         HRESULT *phrc)
{
    LogFlowFunc(("pUVM=%p aMediumAtt=%p phrc=%p\n", pUVM, aMediumAtt, phrc));

    HRESULT         hrc;
    Bstr            bstr;
    *phrc = S_OK;
#define H() do { if (FAILED(hrc)) { AssertMsgFailed(("hrc=%Rhrc (%#x)\n", hrc, hrc)); *phrc = hrc; return VERR_GENERAL_FAILURE; } } while (0)

    /* Ignore attachments other than hard disks, since at the moment they are
     * not subject to snapshotting in general. */
    DeviceType_T lType;
    hrc = aMediumAtt->COMGETTER(Type)(&lType);                                  H();
    if (lType != DeviceType_HardDisk)
        return VINF_SUCCESS;

    /* Update the device instance configuration. */
    int rc = pThis->i_configMediumAttachment(pcszDevice,
                                             uInstance,
                                             enmBus,
                                             fUseHostIOCache,
                                             fBuiltinIOCache,
                                             fInsertDiskIntegrityDrv,
                                             fSetupMerge,
                                             uMergeSource,
                                             uMergeTarget,
                                             aMediumAtt,
                                             aMachineState,
                                             phrc,
                                             true /* fAttachDetach */,
                                             false /* fForceUnmount */,
                                             false /* fHotplug */,
                                             pUVM,
                                             NULL /* paLedDevType */,
                                             NULL /* ppLunL0)*/);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("rc=%Rrc\n", rc));
        return rc;
    }

#undef H

    LogFlowFunc(("Returns success\n"));
    return VINF_SUCCESS;
}

/**
 * Thread for powering down the Console.
 *
 * @param   pTask       The power down task.
 *
 * @note Locks the Console object for writing.
 */
/*static*/
void Console::i_powerDownThreadTask(VMPowerDownTask *pTask)
{
    int rc = VINF_SUCCESS; /* only used in assertion */
    LogFlowFuncEnter();
    try
    {
        if (pTask->isOk() == false)
            rc = VERR_GENERAL_FAILURE;

        const ComObjPtr<Console> &that = pTask->mConsole;

        /* Note: no need to use AutoCaller to protect Console because VMTask does
         * that */

        /* wait until the method tat started us returns */
        AutoWriteLock thatLock(that COMMA_LOCKVAL_SRC_POS);

        /* release VM caller to avoid the powerDown() deadlock */
        pTask->releaseVMCaller();

        thatLock.release();

        that->i_powerDown(pTask->mServerProgress);

        /* complete the operation */
        that->mControl->EndPoweringDown(S_OK, Bstr().raw());

    }
    catch (const std::exception &e)
    {
        AssertMsgFailed(("Exception %s was caught, rc=%Rrc\n", e.what(), rc));
        NOREF(e); NOREF(rc);
    }

    LogFlowFuncLeave();
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnSaveState}
 */
/*static*/ DECLCALLBACK(int)
Console::i_vmm2User_SaveState(PCVMM2USERMETHODS pThis, PUVM pUVM)
{
    Console *pConsole = ((MYVMM2USERMETHODS *)pThis)->pConsole;
    NOREF(pUVM);

    /*
     * For now, just call SaveState.  We should probably try notify the GUI so
     * it can pop up a progress object and stuff. The progress object created
     * by the call isn't returned to anyone and thus gets updated without
     * anyone noticing it.
     */
    ComPtr<IProgress> pProgress;
    HRESULT hrc = pConsole->mMachine->SaveState(pProgress.asOutParam());
    return SUCCEEDED(hrc) ? VINF_SUCCESS : Global::vboxStatusCodeFromCOM(hrc);
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnNotifyEmtInit}
 */
/*static*/ DECLCALLBACK(void)
Console::i_vmm2User_NotifyEmtInit(PCVMM2USERMETHODS pThis, PUVM pUVM, PUVMCPU pUVCpu)
{
    NOREF(pThis); NOREF(pUVM); NOREF(pUVCpu);
    VirtualBoxBase::initializeComForThread();
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnNotifyEmtTerm}
 */
/*static*/ DECLCALLBACK(void)
Console::i_vmm2User_NotifyEmtTerm(PCVMM2USERMETHODS pThis, PUVM pUVM, PUVMCPU pUVCpu)
{
    NOREF(pThis); NOREF(pUVM); NOREF(pUVCpu);
    VirtualBoxBase::uninitializeComForThread();
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnNotifyPdmtInit}
 */
/*static*/ DECLCALLBACK(void)
Console::i_vmm2User_NotifyPdmtInit(PCVMM2USERMETHODS pThis, PUVM pUVM)
{
    NOREF(pThis); NOREF(pUVM);
    VirtualBoxBase::initializeComForThread();
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnNotifyPdmtTerm}
 */
/*static*/ DECLCALLBACK(void)
Console::i_vmm2User_NotifyPdmtTerm(PCVMM2USERMETHODS pThis, PUVM pUVM)
{
    NOREF(pThis); NOREF(pUVM);
    VirtualBoxBase::uninitializeComForThread();
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnNotifyResetTurnedIntoPowerOff}
 */
/*static*/ DECLCALLBACK(void)
Console::i_vmm2User_NotifyResetTurnedIntoPowerOff(PCVMM2USERMETHODS pThis, PUVM pUVM)
{
    Console *pConsole = ((MYVMM2USERMETHODS *)pThis)->pConsole;
    NOREF(pUVM);

    pConsole->mfPowerOffCausedByReset = true;
}

/**
 * @interface_method_impl{VMM2USERMETHODS,pfnQueryGenericObject}
 */
/*static*/ DECLCALLBACK(void *)
Console::i_vmm2User_QueryGenericObject(PCVMM2USERMETHODS pThis, PUVM pUVM, PCRTUUID pUuid)
{
    Console *pConsole = ((MYVMM2USERMETHODS *)pThis)->pConsole;
    NOREF(pUVM);

    /* To simplify comparison we copy the UUID into a com::Guid object. */
    com::Guid const UuidCopy(*pUuid);

    if (UuidCopy == COM_IIDOF(IConsole))
    {
        IConsole *pIConsole = static_cast<IConsole *>(pConsole);
        return pIConsole;
    }

    if (UuidCopy == COM_IIDOF(IMachine))
    {
        IMachine *pIMachine = pConsole->mMachine;
        return pIMachine;
    }

    if (UuidCopy == COM_IIDOF(ISnapshot))
        return ((MYVMM2USERMETHODS *)pThis)->pISnapshot;

    return NULL;
}


/**
 * @interface_method_impl{PDMISECKEY,pfnKeyRetain}
 */
/*static*/ DECLCALLBACK(int)
Console::i_pdmIfSecKey_KeyRetain(PPDMISECKEY pInterface, const char *pszId, const uint8_t **ppbKey,
                                 size_t *pcbKey)
{
    Console *pConsole = ((MYPDMISECKEY *)pInterface)->pConsole;

    AutoReadLock thatLock(pConsole COMMA_LOCKVAL_SRC_POS);
    SecretKey *pKey = NULL;

    int rc = pConsole->m_pKeyStore->retainSecretKey(Utf8Str(pszId), &pKey);
    if (RT_SUCCESS(rc))
    {
        *ppbKey = (const uint8_t *)pKey->getKeyBuffer();
        *pcbKey = pKey->getKeySize();
    }

    return rc;
}

/**
 * @interface_method_impl{PDMISECKEY,pfnKeyRelease}
 */
/*static*/ DECLCALLBACK(int)
Console::i_pdmIfSecKey_KeyRelease(PPDMISECKEY pInterface, const char *pszId)
{
    Console *pConsole = ((MYPDMISECKEY *)pInterface)->pConsole;

    AutoReadLock thatLock(pConsole COMMA_LOCKVAL_SRC_POS);
    return pConsole->m_pKeyStore->releaseSecretKey(Utf8Str(pszId));
}

/**
 * @interface_method_impl{PDMISECKEY,pfnPasswordRetain}
 */
/*static*/ DECLCALLBACK(int)
Console::i_pdmIfSecKey_PasswordRetain(PPDMISECKEY pInterface, const char *pszId, const char **ppszPassword)
{
    Console *pConsole = ((MYPDMISECKEY *)pInterface)->pConsole;

    AutoReadLock thatLock(pConsole COMMA_LOCKVAL_SRC_POS);
    SecretKey *pKey = NULL;

    int rc = pConsole->m_pKeyStore->retainSecretKey(Utf8Str(pszId), &pKey);
    if (RT_SUCCESS(rc))
        *ppszPassword = (const char *)pKey->getKeyBuffer();

    return rc;
}

/**
 * @interface_method_impl{PDMISECKEY,pfnPasswordRelease}
 */
/*static*/ DECLCALLBACK(int)
Console::i_pdmIfSecKey_PasswordRelease(PPDMISECKEY pInterface, const char *pszId)
{
    Console *pConsole = ((MYPDMISECKEY *)pInterface)->pConsole;

    AutoReadLock thatLock(pConsole COMMA_LOCKVAL_SRC_POS);
    return pConsole->m_pKeyStore->releaseSecretKey(Utf8Str(pszId));
}

/**
 * @interface_method_impl{PDMISECKEYHLP,pfnKeyMissingNotify}
 */
/*static*/ DECLCALLBACK(int)
Console::i_pdmIfSecKeyHlp_KeyMissingNotify(PPDMISECKEYHLP pInterface)
{
    Console *pConsole = ((MYPDMISECKEYHLP *)pInterface)->pConsole;

    /* Set guest property only, the VM is paused in the media driver calling us. */
    pConsole->mMachine->DeleteGuestProperty(Bstr("/VirtualBox/HostInfo/DekMissing").raw());
    pConsole->mMachine->SetGuestProperty(Bstr("/VirtualBox/HostInfo/DekMissing").raw(),
                                         Bstr("1").raw(), Bstr("RDONLYGUEST").raw());
    pConsole->mMachine->SaveSettings();

    return VINF_SUCCESS;
}



/**
 * The Main status driver instance data.
 */
typedef struct DRVMAINSTATUS
{
    /** The LED connectors. */
    PDMILEDCONNECTORS   ILedConnectors;
    /** Pointer to the LED ports interface above us. */
    PPDMILEDPORTS       pLedPorts;
    /** Pointer to the array of LED pointers. */
    PPDMLED            *papLeds;
    /** The unit number corresponding to the first entry in the LED array. */
    RTUINT              iFirstLUN;
    /** The unit number corresponding to the last entry in the LED array.
     * (The size of the LED array is iLastLUN - iFirstLUN + 1.) */
    RTUINT              iLastLUN;
    /** Pointer to the driver instance. */
    PPDMDRVINS          pDrvIns;
    /** The Media Notify interface. */
    PDMIMEDIANOTIFY     IMediaNotify;
    /** Map for translating PDM storage controller/LUN information to
     * IMediumAttachment references. */
    Console::MediumAttachmentMap *pmapMediumAttachments;
    /** Device name+instance for mapping */
    char                *pszDeviceInstance;
    /** Pointer to the Console object, for driver triggered activities. */
    Console             *pConsole;
} DRVMAINSTATUS, *PDRVMAINSTATUS;


/**
 * Notification about a unit which have been changed.
 *
 * The driver must discard any pointers to data owned by
 * the unit and requery it.
 *
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   iLUN            The unit number.
 */
DECLCALLBACK(void) Console::i_drvStatus_UnitChanged(PPDMILEDCONNECTORS pInterface, unsigned iLUN)
{
    PDRVMAINSTATUS pThis = RT_FROM_MEMBER(pInterface, DRVMAINSTATUS, ILedConnectors);
    if (iLUN >= pThis->iFirstLUN && iLUN <= pThis->iLastLUN)
    {
        PPDMLED pLed;
        int rc = pThis->pLedPorts->pfnQueryStatusLed(pThis->pLedPorts, iLUN, &pLed);
        if (RT_FAILURE(rc))
            pLed = NULL;
        ASMAtomicWritePtr(&pThis->papLeds[iLUN - pThis->iFirstLUN], pLed);
        Log(("drvStatus_UnitChanged: iLUN=%d pLed=%p\n", iLUN, pLed));
    }
}


/**
 * Notification about a medium eject.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   uLUN            The unit number.
 */
DECLCALLBACK(int) Console::i_drvStatus_MediumEjected(PPDMIMEDIANOTIFY pInterface, unsigned uLUN)
{
    PDRVMAINSTATUS pThis = RT_FROM_MEMBER(pInterface, DRVMAINSTATUS, IMediaNotify);
    LogFunc(("uLUN=%d\n", uLUN));
    if (pThis->pmapMediumAttachments)
    {
        AutoWriteLock alock(pThis->pConsole COMMA_LOCKVAL_SRC_POS);

        ComPtr<IMediumAttachment> pMediumAtt;
        Utf8Str devicePath = Utf8StrFmt("%s/LUN#%u", pThis->pszDeviceInstance, uLUN);
        Console::MediumAttachmentMap::const_iterator end = pThis->pmapMediumAttachments->end();
        Console::MediumAttachmentMap::const_iterator it = pThis->pmapMediumAttachments->find(devicePath);
        if (it != end)
            pMediumAtt = it->second;
        Assert(!pMediumAtt.isNull());
        if (!pMediumAtt.isNull())
        {
            IMedium *pMedium = NULL;
            HRESULT rc = pMediumAtt->COMGETTER(Medium)(&pMedium);
            AssertComRC(rc);
            if (SUCCEEDED(rc) && pMedium)
            {
                BOOL fHostDrive = FALSE;
                rc = pMedium->COMGETTER(HostDrive)(&fHostDrive);
                AssertComRC(rc);
                if (!fHostDrive)
                {
                    alock.release();

                    ComPtr<IMediumAttachment> pNewMediumAtt;
                    rc = pThis->pConsole->mControl->EjectMedium(pMediumAtt, pNewMediumAtt.asOutParam());
                    if (SUCCEEDED(rc))
                    {
                        pThis->pConsole->mMachine->SaveSettings();
                        fireMediumChangedEvent(pThis->pConsole->mEventSource, pNewMediumAtt);
                    }

                    alock.acquire();
                    if (pNewMediumAtt != pMediumAtt)
                    {
                        pThis->pmapMediumAttachments->erase(devicePath);
                        pThis->pmapMediumAttachments->insert(std::make_pair(devicePath, pNewMediumAtt));
                    }
                }
            }
        }
    }
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
DECLCALLBACK(void *)  Console::i_drvStatus_QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVMAINSTATUS pThis = PDMINS_2_DATA(pDrvIns, PDRVMAINSTATUS);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDCONNECTORS, &pThis->ILedConnectors);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIANOTIFY, &pThis->IMediaNotify);
    return NULL;
}


/**
 * Destruct a status driver instance.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance data.
 */
DECLCALLBACK(void) Console::i_drvStatus_Destruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVMAINSTATUS pThis = PDMINS_2_DATA(pDrvIns, PDRVMAINSTATUS);
    LogFlowFunc(("iInstance=%d\n", pDrvIns->iInstance));

    if (pThis->papLeds)
    {
        unsigned iLed = pThis->iLastLUN - pThis->iFirstLUN + 1;
        while (iLed-- > 0)
            ASMAtomicWriteNullPtr(&pThis->papLeds[iLed]);
    }
}


/**
 * Construct a status driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
DECLCALLBACK(int) Console::i_drvStatus_Construct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVMAINSTATUS pThis = PDMINS_2_DATA(pDrvIns, PDRVMAINSTATUS);
    LogFlowFunc(("iInstance=%d\n", pDrvIns->iInstance));

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "papLeds\0pmapMediumAttachments\0DeviceInstance\0pConsole\0First\0Last\0"))
        return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;
    AssertMsgReturn(PDMDrvHlpNoAttach(pDrvIns) == VERR_PDM_NO_ATTACHED_DRIVER,
                    ("Configuration error: Not possible to attach anything to this driver!\n"),
                    VERR_PDM_DRVINS_NO_ATTACH);

    /*
     * Data.
     */
    pDrvIns->IBase.pfnQueryInterface        = Console::i_drvStatus_QueryInterface;
    pThis->ILedConnectors.pfnUnitChanged    = Console::i_drvStatus_UnitChanged;
    pThis->IMediaNotify.pfnEjected          = Console::i_drvStatus_MediumEjected;
    pThis->pDrvIns                          = pDrvIns;
    pThis->pszDeviceInstance                = NULL;

    /*
     * Read config.
     */
    int rc = CFGMR3QueryPtr(pCfg, "papLeds", (void **)&pThis->papLeds);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: Failed to query the \"papLeds\" value! rc=%Rrc\n", rc));
        return rc;
    }

    rc = CFGMR3QueryPtrDef(pCfg, "pmapMediumAttachments", (void **)&pThis->pmapMediumAttachments, NULL);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: Failed to query the \"pmapMediumAttachments\" value! rc=%Rrc\n", rc));
        return rc;
    }
    if (pThis->pmapMediumAttachments)
    {
        rc = CFGMR3QueryStringAlloc(pCfg, "DeviceInstance", &pThis->pszDeviceInstance);
        if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("Configuration error: Failed to query the \"DeviceInstance\" value! rc=%Rrc\n", rc));
            return rc;
        }
        rc = CFGMR3QueryPtr(pCfg, "pConsole", (void **)&pThis->pConsole);
        if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("Configuration error: Failed to query the \"pConsole\" value! rc=%Rrc\n", rc));
            return rc;
        }
    }

    rc = CFGMR3QueryU32(pCfg, "First", &pThis->iFirstLUN);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        pThis->iFirstLUN = 0;
    else if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: Failed to query the \"First\" value! rc=%Rrc\n", rc));
        return rc;
    }

    rc = CFGMR3QueryU32(pCfg, "Last", &pThis->iLastLUN);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        pThis->iLastLUN = 0;
    else if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: Failed to query the \"Last\" value! rc=%Rrc\n", rc));
        return rc;
    }
    if (pThis->iFirstLUN > pThis->iLastLUN)
    {
        AssertMsgFailed(("Configuration error: Invalid unit range %u-%u\n", pThis->iFirstLUN, pThis->iLastLUN));
        return VERR_GENERAL_FAILURE;
    }

    /*
     * Get the ILedPorts interface of the above driver/device and
     * query the LEDs we want.
     */
    pThis->pLedPorts = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMILEDPORTS);
    AssertMsgReturn(pThis->pLedPorts, ("Configuration error: No led ports interface above!\n"),
                    VERR_PDM_MISSING_INTERFACE_ABOVE);

    for (unsigned i = pThis->iFirstLUN; i <= pThis->iLastLUN; ++i)
        Console::i_drvStatus_UnitChanged(&pThis->ILedConnectors, i);

    return VINF_SUCCESS;
}


/**
 * Console status driver (LED) registration record.
 */
const PDMDRVREG Console::DrvStatusReg =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "MainStatus",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Main status driver (Main as in the API).",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_STATUS,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVMAINSTATUS),
    /* pfnConstruct */
    Console::i_drvStatus_Construct,
    /* pfnDestruct */
    Console::i_drvStatus_Destruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};



/* vi: set tabstop=4 shiftwidth=4 expandtab: */
