/* $Id: ConsoleImpl.h $ */
/** @file
 * VBox Console COM Class definition
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

#ifndef ____H_CONSOLEIMPL
#define ____H_CONSOLEIMPL

#include "VirtualBoxBase.h"
#include "VBox/com/array.h"
#include "EventImpl.h"
#include "SecretKeyStore.h"
#include "ConsoleWrap.h"

class Guest;
class Keyboard;
class Mouse;
class Display;
class MachineDebugger;
class TeleporterStateSrc;
class OUSBDevice;
class RemoteUSBDevice;
class SharedFolder;
class VRDEServerInfo;
class EmulatedUSB;
class AudioVRDE;
#ifdef VBOX_WITH_AUDIO_VIDEOREC
class AudioVideoRec;
#endif
class Nvram;
#ifdef VBOX_WITH_USB_CARDREADER
class UsbCardReader;
#endif
class ConsoleVRDPServer;
class VMMDev;
class Progress;
class BusAssignmentManager;
COM_STRUCT_OR_CLASS(IEventListener);
#ifdef VBOX_WITH_EXTPACK
class ExtPackManager;
#endif
class VMMDevMouseInterface;
class DisplayMouseInterface;
class VMPowerUpTask;
class VMPowerDownTask;

#include <iprt/uuid.h>
#include <iprt/memsafer.h>
#include <VBox/RemoteDesktop/VRDE.h>
#include <VBox/vmm/pdmdrv.h>
#ifdef VBOX_WITH_GUEST_PROPS
# include <VBox/HostServices/GuestPropertySvc.h>  /* For the property notification callback */
#endif

struct VUSBIRHCONFIG;
typedef struct VUSBIRHCONFIG *PVUSBIRHCONFIG;

#include <list>
#include <vector>

// defines
///////////////////////////////////////////////////////////////////////////////

/**
 *  Checks the availability of the underlying VM device driver corresponding
 *  to the COM interface (IKeyboard, IMouse, IDisplay, etc.). When the driver is
 *  not available (NULL), sets error info and returns returns E_ACCESSDENIED.
 *  The translatable error message is defined in null context.
 *
 *  Intended to used only within Console children (i.e. Keyboard, Mouse,
 *  Display, etc.).
 *
 *  @param drv  driver pointer to check (compare it with NULL)
 */
#define CHECK_CONSOLE_DRV(drv) \
    do { \
        if (!(drv)) \
            return setError(E_ACCESSDENIED, tr("The console is not powered up")); \
    } while (0)

// Console
///////////////////////////////////////////////////////////////////////////////

class ConsoleMouseInterface
{
public:
    virtual VMMDevMouseInterface  *i_getVMMDevMouseInterface(){return NULL;}
    virtual DisplayMouseInterface *i_getDisplayMouseInterface(){return NULL;}
    virtual void i_onMouseCapabilityChange(BOOL supportsAbsolute,
                                           BOOL supportsRelative,
                                           BOOL supportsMT,
                                           BOOL needsHostCursor){NOREF(supportsAbsolute); NOREF(supportsRelative); NOREF(supportsMT); NOREF(needsHostCursor);}
};

/** IConsole implementation class */
class ATL_NO_VTABLE Console :
    public ConsoleWrap,
    public ConsoleMouseInterface
{

public:

    DECLARE_EMPTY_CTOR_DTOR(Console)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializers/uninitializers for internal purposes only
    HRESULT init(IMachine *aMachine, IInternalMachineControl *aControl, LockType_T aLockType);
    void uninit();


    // public methods for internal purposes only

    /*
     *  Note: the following methods do not increase refcount. intended to be
     *  called only by the VM execution thread.
     */

    Guest *i_getGuest() const { return mGuest; }
    Keyboard *i_getKeyboard() const { return mKeyboard; }
    Mouse *i_getMouse() const { return mMouse; }
    Display *i_getDisplay() const { return mDisplay; }
    MachineDebugger *i_getMachineDebugger() const { return mDebugger; }
#ifdef VBOX_WITH_VRDE_AUDIO
    AudioVRDE *i_getAudioVRDE() const { return mAudioVRDE; }
#endif
#ifdef VBOX_WITH_AUDIO_VIDEOREC
    AudioVideoRec *i_getAudioVideoRec() const { return mAudioVideoRec; }
    HRESULT i_audioVideoRecSendAudio(const void *pvData, size_t cbData, uint64_t uDurationMs);
#endif

    const ComPtr<IMachine> &i_machine() const { return mMachine; }
    const Bstr &i_getId() const { return mstrUuid; }

    bool i_useHostClipboard() { return mfUseHostClipboard; }

    /** Method is called only from ConsoleVRDPServer */
    IVRDEServer *i_getVRDEServer() const { return mVRDEServer; }

    ConsoleVRDPServer *i_consoleVRDPServer() const { return mConsoleVRDPServer; }

    HRESULT i_updateMachineState(MachineState_T aMachineState);
    HRESULT i_getNominalState(MachineState_T &aNominalState);
    Utf8Str i_getAudioAdapterDeviceName(IAudioAdapter *aAudioAdapter);

    // events from IInternalSessionControl
    HRESULT i_onNetworkAdapterChange(INetworkAdapter *aNetworkAdapter, BOOL changeAdapter);
    HRESULT i_onAudioAdapterChange(IAudioAdapter *aAudioAdapter);
    HRESULT i_onSerialPortChange(ISerialPort *aSerialPort);
    HRESULT i_onParallelPortChange(IParallelPort *aParallelPort);
    HRESULT i_onStorageControllerChange();
    HRESULT i_onMediumChange(IMediumAttachment *aMediumAttachment, BOOL aForce);
    HRESULT i_onCPUChange(ULONG aCPU, BOOL aRemove);
    HRESULT i_onCPUExecutionCapChange(ULONG aExecutionCap);
    HRESULT i_onClipboardModeChange(ClipboardMode_T aClipboardMode);
    HRESULT i_onDnDModeChange(DnDMode_T aDnDMode);
    HRESULT i_onVRDEServerChange(BOOL aRestart);
    HRESULT i_onVideoCaptureChange();
    HRESULT i_onUSBControllerChange();
    HRESULT i_onSharedFolderChange(BOOL aGlobal);
    HRESULT i_onUSBDeviceAttach(IUSBDevice *aDevice, IVirtualBoxErrorInfo *aError, ULONG aMaskedIfs,
                                const Utf8Str &aCaptureFilename);
    HRESULT i_onUSBDeviceDetach(IN_BSTR aId, IVirtualBoxErrorInfo *aError);
    HRESULT i_onBandwidthGroupChange(IBandwidthGroup *aBandwidthGroup);
    HRESULT i_onStorageDeviceChange(IMediumAttachment *aMediumAttachment, BOOL aRemove, BOOL aSilent);
    HRESULT i_onExtraDataChange(IN_BSTR aMachineId, IN_BSTR aKey, IN_BSTR aVal);

    HRESULT i_getGuestProperty(const Utf8Str &aName, Utf8Str *aValue, LONG64 *aTimestamp, Utf8Str *aFlags);
    HRESULT i_setGuestProperty(const Utf8Str &aName, const Utf8Str &aValue, const Utf8Str &aFlags);
    HRESULT i_deleteGuestProperty(const Utf8Str &aName);
    HRESULT i_enumerateGuestProperties(const Utf8Str &aPatterns,
                                       std::vector<Utf8Str> &aNames,
                                       std::vector<Utf8Str> &aValues,
                                       std::vector<LONG64>  &aTimestamps,
                                       std::vector<Utf8Str> &aFlags);
    HRESULT i_onlineMergeMedium(IMediumAttachment *aMediumAttachment,
                                ULONG aSourceIdx, ULONG aTargetIdx,
                                IProgress *aProgress);
    HRESULT i_reconfigureMediumAttachments(const std::vector<ComPtr<IMediumAttachment> > &aAttachments);
    int i_hgcmLoadService(const char *pszServiceLibrary, const char *pszServiceName);
    VMMDev *i_getVMMDev() { return m_pVMMDev; }

#ifdef VBOX_WITH_EXTPACK
    ExtPackManager *i_getExtPackManager();
#endif
    EventSource *i_getEventSource() { return mEventSource; }
#ifdef VBOX_WITH_USB_CARDREADER
    UsbCardReader *i_getUsbCardReader() { return mUsbCardReader; }
#endif

    int i_VRDPClientLogon(uint32_t u32ClientId, const char *pszUser, const char *pszPassword, const char *pszDomain);
    void i_VRDPClientStatusChange(uint32_t u32ClientId, const char *pszStatus);
    void i_VRDPClientConnect(uint32_t u32ClientId);
    void i_VRDPClientDisconnect(uint32_t u32ClientId, uint32_t fu32Intercepted);
    void i_VRDPInterceptAudio(uint32_t u32ClientId);
    void i_VRDPInterceptUSB(uint32_t u32ClientId, void **ppvIntercept);
    void i_VRDPInterceptClipboard(uint32_t u32ClientId);

    void i_processRemoteUSBDevices(uint32_t u32ClientId, VRDEUSBDEVICEDESC *pDevList, uint32_t cbDevList, bool fDescExt);
    void i_reportVmStatistics(ULONG aValidStats, ULONG aCpuUser,
                              ULONG aCpuKernel, ULONG aCpuIdle,
                              ULONG aMemTotal, ULONG aMemFree,
                              ULONG aMemBalloon, ULONG aMemShared,
                              ULONG aMemCache, ULONG aPageTotal,
                              ULONG aAllocVMM, ULONG aFreeVMM,
                              ULONG aBalloonedVMM, ULONG aSharedVMM,
                              ULONG aVmNetRx, ULONG aVmNetTx)
    {
        mControl->ReportVmStatistics(aValidStats, aCpuUser, aCpuKernel, aCpuIdle,
                                     aMemTotal, aMemFree, aMemBalloon, aMemShared,
                                     aMemCache, aPageTotal, aAllocVMM, aFreeVMM,
                                     aBalloonedVMM, aSharedVMM, aVmNetRx, aVmNetTx);
    }
    void i_enableVMMStatistics(BOOL aEnable);

    HRESULT i_pause(Reason_T aReason);
    HRESULT i_resume(Reason_T aReason, AutoWriteLock &alock);
    HRESULT i_saveState(Reason_T aReason, const ComPtr<IProgress> &aProgress,
                        const ComPtr<ISnapshot> &aSnapshot,
                        const Utf8Str &aStateFilePath, bool fPauseVM, bool &fLeftPaused);
    HRESULT i_cancelSaveState();

    // callback callers (partly; for some events console callbacks are notified
    // directly from IInternalSessionControl event handlers declared above)
    void i_onMousePointerShapeChange(bool fVisible, bool fAlpha,
                                     uint32_t xHot, uint32_t yHot,
                                     uint32_t width, uint32_t height,
                                     const uint8_t *pu8Shape,
                                     uint32_t cbShape);
    void i_onMouseCapabilityChange(BOOL supportsAbsolute, BOOL supportsRelative,
                                   BOOL supportsMT, BOOL needsHostCursor);
    void i_onStateChange(MachineState_T aMachineState);
    void i_onAdditionsStateChange();
    void i_onAdditionsOutdated();
    void i_onKeyboardLedsChange(bool fNumLock, bool fCapsLock, bool fScrollLock);
    void i_onUSBDeviceStateChange(IUSBDevice *aDevice, bool aAttached,
                                  IVirtualBoxErrorInfo *aError);
    void i_onRuntimeError(BOOL aFatal, IN_BSTR aErrorID, IN_BSTR aMessage);
    HRESULT i_onShowWindow(BOOL aCheck, BOOL *aCanShow, LONG64 *aWinId);
    void i_onVRDEServerInfoChange();
    HRESULT i_sendACPIMonitorHotPlugEvent();

    static const PDMDRVREG DrvStatusReg;

    static HRESULT i_setErrorStatic(HRESULT aResultCode, const char *pcsz, ...);
    HRESULT i_setInvalidMachineStateError();

    static const char *i_storageControllerTypeToStr(StorageControllerType_T enmCtrlType);
    static HRESULT i_storageBusPortDeviceToLun(StorageBus_T enmBus, LONG port, LONG device, unsigned &uLun);
    // Called from event listener
    HRESULT i_onNATRedirectRuleChange(ULONG ulInstance, BOOL aNatRuleRemove,
                                      NATProtocol_T aProto, IN_BSTR aHostIp, LONG aHostPort, IN_BSTR aGuestIp, LONG aGuestPort);
    HRESULT i_onNATDnsChanged();

    // Mouse interface
    VMMDevMouseInterface *i_getVMMDevMouseInterface();
    DisplayMouseInterface *i_getDisplayMouseInterface();

    EmulatedUSB *i_getEmulatedUSB(void) { return mEmulatedUSB; }

    /**
     * Sets the disk encryption keys.
     *
     * @returns COM status code.
     * @þaram   strCfg    The config for the disks.
     *
     * @note: One line in the config string contains all required data for one disk.
     *        The format for one disk is some sort of comma separated value using
     *        key=value pairs.
     *        There are two keys defined at the moment:
     *            - uuid: The uuid of the base image the key is for (with or without)
     *                    the curly braces.
     *            - dek: The data encryption key in base64 encoding
     */
    HRESULT i_setDiskEncryptionKeys(const Utf8Str &strCfg);

private:

    // wraped IConsole properties
    HRESULT getMachine(ComPtr<IMachine> &aMachine);
    HRESULT getState(MachineState_T *aState);
    HRESULT getGuest(ComPtr<IGuest> &aGuest);
    HRESULT getKeyboard(ComPtr<IKeyboard> &aKeyboard);
    HRESULT getMouse(ComPtr<IMouse> &aMouse);
    HRESULT getDisplay(ComPtr<IDisplay> &aDisplay);
    HRESULT getDebugger(ComPtr<IMachineDebugger> &aDebugger);
    HRESULT getUSBDevices(std::vector<ComPtr<IUSBDevice> > &aUSBDevices);
    HRESULT getRemoteUSBDevices(std::vector<ComPtr<IHostUSBDevice> > &aRemoteUSBDevices);
    HRESULT getSharedFolders(std::vector<ComPtr<ISharedFolder> > &aSharedFolders);
    HRESULT getVRDEServerInfo(ComPtr<IVRDEServerInfo> &aVRDEServerInfo);
    HRESULT getEventSource(ComPtr<IEventSource> &aEventSource);
    HRESULT getAttachedPCIDevices(std::vector<ComPtr<IPCIDeviceAttachment> > &aAttachedPCIDevices);
    HRESULT getUseHostClipboard(BOOL *aUseHostClipboard);
    HRESULT setUseHostClipboard(BOOL aUseHostClipboard);
    HRESULT getEmulatedUSB(ComPtr<IEmulatedUSB> &aEmulatedUSB);

    // wraped IConsole methods
    HRESULT powerUp(ComPtr<IProgress> &aProgress);
    HRESULT powerUpPaused(ComPtr<IProgress> &aProgress);
    HRESULT powerDown(ComPtr<IProgress> &aProgress);
    HRESULT reset();
    HRESULT pause();
    HRESULT resume();
    HRESULT powerButton();
    HRESULT sleepButton();
    HRESULT getPowerButtonHandled(BOOL *aHandled);
    HRESULT getGuestEnteredACPIMode(BOOL *aEntered);
    HRESULT getDeviceActivity(const std::vector<DeviceType_T> &aType,
                              std::vector<DeviceActivity_T> &aActivity);
    HRESULT attachUSBDevice(const com::Guid &aId, const com::Utf8Str &aCaptureFilename);
    HRESULT detachUSBDevice(const com::Guid &aId,
                            ComPtr<IUSBDevice> &aDevice);
    HRESULT findUSBDeviceByAddress(const com::Utf8Str &aName,
                                   ComPtr<IUSBDevice> &aDevice);
    HRESULT findUSBDeviceById(const com::Guid &aId,
                              ComPtr<IUSBDevice> &aDevice);
    HRESULT createSharedFolder(const com::Utf8Str &aName,
                               const com::Utf8Str &aHostPath,
                               BOOL aWritable,
                               BOOL aAutomount);
    HRESULT removeSharedFolder(const com::Utf8Str &aName);
    HRESULT teleport(const com::Utf8Str &aHostname,
                     ULONG aTcpport,
                     const com::Utf8Str &aPassword,
                     ULONG aMaxDowntime,
                     ComPtr<IProgress> &aProgress);
    HRESULT addDiskEncryptionPassword(const com::Utf8Str &aId, const com::Utf8Str &aPassword,
                                      BOOL aClearOnSuspend);
    HRESULT addDiskEncryptionPasswords(const std::vector<com::Utf8Str> &aIds, const std::vector<com::Utf8Str> &aPasswords,
                                      BOOL aClearOnSuspend);
    HRESULT removeDiskEncryptionPassword(const com::Utf8Str &aId);
    HRESULT clearAllDiskEncryptionPasswords();

    void notifyNatDnsChange(PUVM pUVM, const char *pszDevice, ULONG ulInstanceMax);
    Utf8Str VRDPServerErrorToMsg(int vrc);

    /**
     *  Base template for AutoVMCaller and SafeVMPtr. Template arguments
     *  have the same meaning as arguments of Console::addVMCaller().
     */
    template <bool taQuiet = false, bool taAllowNullVM = false>
    class AutoVMCallerBase
    {
    public:
        AutoVMCallerBase(Console *aThat) : mThat(aThat), mRC(E_FAIL)
        {
            Assert(aThat);
            mRC = aThat->i_addVMCaller(taQuiet, taAllowNullVM);
        }
        ~AutoVMCallerBase()
        {
            doRelease();
        }
        /** Decreases the number of callers before the instance is destroyed. */
        void releaseCaller()
        {
            Assert(SUCCEEDED(mRC));
            doRelease();
        }
        /** Restores the number of callers after by #release(). #rc() must be
         *  rechecked to ensure the operation succeeded. */
        void addYY()
        {
            AssertReturnVoid(!SUCCEEDED(mRC));
            mRC = mThat->i_addVMCaller(taQuiet, taAllowNullVM);
        }
        /** Returns the result of Console::addVMCaller() */
        HRESULT rc() const { return mRC; }
        /** Shortcut to SUCCEEDED(rc()) */
        bool isOk() const { return SUCCEEDED(mRC); }
    protected:
        Console *mThat;
        void doRelease()
        {
            if (SUCCEEDED(mRC))
            {
                mThat->i_releaseVMCaller();
                mRC = E_FAIL;
            }
        }
    private:
        HRESULT mRC; /* Whether the caller was added. */
        DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AutoVMCallerBase);
    };

#if 0
    /**
     *  Helper class that protects sections of code using the mpUVM pointer by
     *  automatically calling addVMCaller() on construction and
     *  releaseVMCaller() on destruction. Intended for Console methods dealing
     *  with mpUVM. The usage pattern is:
     *  <code>
     *      AutoVMCaller autoVMCaller(this);
     *      if (FAILED(autoVMCaller.rc())) return autoVMCaller.rc();
     *      ...
     *      VMR3ReqCall (mpUVM, ...
     *  </code>
     *
     *  @note Temporarily locks the argument for writing.
     *
     *  @sa SafeVMPtr, SafeVMPtrQuiet
     *  @note Obsolete, use SafeVMPtr
     */
    typedef AutoVMCallerBase<false, false> AutoVMCaller;
#endif

    /**
     *  Same as AutoVMCaller but doesn't set extended error info on failure.
     *
     *  @note Temporarily locks the argument for writing.
     *  @note Obsolete, use SafeVMPtrQuiet
     */
    typedef AutoVMCallerBase<true, false> AutoVMCallerQuiet;

    /**
     *  Same as AutoVMCaller but allows a null VM pointer (to trigger an error
     *  instead of assertion).
     *
     *  @note Temporarily locks the argument for writing.
     *  @note Obsolete, use SafeVMPtr
     */
    typedef AutoVMCallerBase<false, true> AutoVMCallerWeak;

    /**
     *  Same as AutoVMCaller but doesn't set extended error info on failure
     *  and allows a null VM pointer (to trigger an error instead of
     *  assertion).
     *
     *  @note Temporarily locks the argument for writing.
     *  @note Obsolete, use SafeVMPtrQuiet
     */
    typedef AutoVMCallerBase<true, true> AutoVMCallerQuietWeak;

    /**
     *  Base template for SafeVMPtr and SafeVMPtrQuiet.
     */
    template<bool taQuiet = false>
    class SafeVMPtrBase : public AutoVMCallerBase<taQuiet, true>
    {
        typedef AutoVMCallerBase<taQuiet, true> Base;
    public:
        SafeVMPtrBase(Console *aThat) : Base(aThat), mRC(E_FAIL), mpUVM(NULL)
        {
            if (Base::isOk())
                mRC = aThat->i_safeVMPtrRetainer(&mpUVM, taQuiet);
        }
        ~SafeVMPtrBase()
        {
            doRelease();
        }
        /** Direct PUVM access. */
        PUVM rawUVM() const { return mpUVM; }
        /** Release the handles. */
        void release()
        {
            Assert(SUCCEEDED(mRC));
            doRelease();
        }

        /** The combined result of Console::addVMCaller() and Console::safeVMPtrRetainer */
        HRESULT rc() const { return Base::isOk()? mRC: Base::rc(); }
        /** Shortcut to SUCCEEDED(rc()) */
        bool isOk() const { return SUCCEEDED(mRC) && Base::isOk(); }

    private:
        void doRelease()
        {
            if (SUCCEEDED(mRC))
            {
                Base::mThat->i_safeVMPtrReleaser(&mpUVM);
                mRC = E_FAIL;
            }
            Base::doRelease();
        }
        HRESULT mRC; /* Whether the VM ptr was retained. */
        PUVM    mpUVM;
        DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(SafeVMPtrBase);
    };

public:

    /*
     *  Helper class that safely manages the Console::mpUVM pointer
     *  by calling addVMCaller() on construction and releaseVMCaller() on
     *  destruction. Intended for Console children. The usage pattern is:
     *  <code>
     *      Console::SafeVMPtr ptrVM(mParent);
     *      if (!ptrVM.isOk())
     *          return ptrVM.rc();
     *      ...
     *      VMR3ReqCall(ptrVM.rawUVM(), ...
     *      ...
     *      printf("%p\n", ptrVM.rawUVM());
     *  </code>
     *
     *  @note Temporarily locks the argument for writing.
     *
     *  @sa SafeVMPtrQuiet, AutoVMCaller
     */
    typedef SafeVMPtrBase<false> SafeVMPtr;

    /**
     *  A deviation of SafeVMPtr that doesn't set the error info on failure.
     *  Intended for pieces of code that don't need to return the VM access
     *  failure to the caller. The usage pattern is:
     *  <code>
     *      Console::SafeVMPtrQuiet pVM(mParent);
     *      if (pVM.rc())
     *          VMR3ReqCall(pVM, ...
     *      return S_OK;
     *  </code>
     *
     *  @note Temporarily locks the argument for writing.
     *
     *  @sa SafeVMPtr, AutoVMCaller
     */
    typedef SafeVMPtrBase<true> SafeVMPtrQuiet;

    class SharedFolderData
    {
    public:
        SharedFolderData()
        { }

        SharedFolderData(const Utf8Str &aHostPath,
                         bool aWritable,
                         bool aAutoMount)
           : m_strHostPath(aHostPath),
             m_fWritable(aWritable),
             m_fAutoMount(aAutoMount)
        { }

        // copy constructor
        SharedFolderData(const SharedFolderData& aThat)
           : m_strHostPath(aThat.m_strHostPath),
             m_fWritable(aThat.m_fWritable),
             m_fAutoMount(aThat.m_fAutoMount)
        { }

        Utf8Str m_strHostPath;
        bool m_fWritable;
        bool m_fAutoMount;
    };

    /**
     * Class for managing emulated USB MSDs.
     */
    class USBStorageDevice
    {
    public:
        USBStorageDevice()
        { }
        /** The UUID associated with the USB device. */
        RTUUID   mUuid;
        /** Port of the storage device. */
        LONG     iPort;
    };

    typedef std::map<Utf8Str, ComObjPtr<SharedFolder> > SharedFolderMap;
    typedef std::map<Utf8Str, SharedFolderData> SharedFolderDataMap;
    typedef std::map<Utf8Str, ComPtr<IMediumAttachment> > MediumAttachmentMap;
    typedef std::list <USBStorageDevice> USBStorageDeviceList;

    static void i_powerUpThreadTask(VMPowerUpTask *pTask);
    static void i_powerDownThreadTask(VMPowerDownTask *pTask);

private:

    typedef std::list <ComObjPtr<OUSBDevice> > USBDeviceList;
    typedef std::list <ComObjPtr<RemoteUSBDevice> > RemoteUSBDeviceList;

    HRESULT i_addVMCaller(bool aQuiet = false, bool aAllowNullVM = false);
    void    i_releaseVMCaller();
    HRESULT i_safeVMPtrRetainer(PUVM *a_ppUVM, bool aQuiet);
    void    i_safeVMPtrReleaser(PUVM *a_ppUVM);

    HRESULT i_consoleInitReleaseLog(const ComPtr<IMachine> aMachine);

    HRESULT i_powerUp(IProgress **aProgress, bool aPaused);
    HRESULT i_powerDown(IProgress *aProgress = NULL);

/* Note: FreeBSD needs this whether netflt is used or not. */
#if ((defined(RT_OS_LINUX) && !defined(VBOX_WITH_NETFLT)) || defined(RT_OS_FREEBSD))
    HRESULT i_attachToTapInterface(INetworkAdapter *networkAdapter);
    HRESULT i_detachFromTapInterface(INetworkAdapter *networkAdapter);
#endif
    HRESULT i_powerDownHostInterfaces();

    HRESULT i_setMachineState(MachineState_T aMachineState, bool aUpdateServer = true);
    HRESULT i_setMachineStateLocally(MachineState_T aMachineState)
    {
        return i_setMachineState(aMachineState, false /* aUpdateServer */);
    }

    HRESULT i_findSharedFolder(const Utf8Str &strName,
                               ComObjPtr<SharedFolder> &aSharedFolder,
                               bool aSetError = false);

    HRESULT i_fetchSharedFolders(BOOL aGlobal);
    bool    i_findOtherSharedFolder(const Utf8Str &straName,
                                    SharedFolderDataMap::const_iterator &aIt);

    HRESULT i_createSharedFolder(const Utf8Str &strName, const SharedFolderData &aData);
    HRESULT i_removeSharedFolder(const Utf8Str &strName);

    HRESULT i_suspendBeforeConfigChange(PUVM pUVM, AutoWriteLock *pAlock, bool *pfResume);
    void    i_resumeAfterConfigChange(PUVM pUVM);

    static DECLCALLBACK(int) i_configConstructor(PUVM pUVM, PVM pVM, void *pvConsole);
    int i_configConstructorInner(PUVM pUVM, PVM pVM, AutoWriteLock *pAlock);
    int i_configCfgmOverlay(PCFGMNODE pRoot, IVirtualBox *pVirtualBox, IMachine *pMachine);
    int i_configDumpAPISettingsTweaks(IVirtualBox *pVirtualBox, IMachine *pMachine);

    int i_configGraphicsController(PCFGMNODE pDevices,
                                   const GraphicsControllerType_T graphicsController,
                                   BusAssignmentManager *pBusMgr,
                                   const ComPtr<IMachine> &ptrMachine,
                                   const ComPtr<IBIOSSettings> &ptrBiosSettings,
                                   bool fHMEnabled);
    int i_checkMediumLocation(IMedium *pMedium, bool *pfUseHostIOCache);
    int i_unmountMediumFromGuest(PUVM pUVM, StorageBus_T enmBus, DeviceType_T enmDevType,
                                 const char *pcszDevice, unsigned uInstance, unsigned uLUN,
                                 bool fForceUnmount);
    int i_removeMediumDriverFromVm(PCFGMNODE pCtlInst,
                                   const char *pcszDevice,
                                   unsigned uInstance,
                                   unsigned uLUN,
                                   StorageBus_T enmBus,
                                   bool fAttachDetach,
                                   bool fHotplug,
                                   bool fForceUnmount,
                                   PUVM pUVM,
                                   DeviceType_T enmDevType,
                                   PCFGMNODE *ppLunL0);
    int i_configMediumAttachment(const char *pcszDevice,
                                 unsigned uInstance,
                                 StorageBus_T enmBus,
                                 bool fUseHostIOCache,
                                 bool fBuiltinIoCache,
                                 bool fInsertDiskIntegrityDrv,
                                 bool fSetupMerge,
                                 unsigned uMergeSource,
                                 unsigned uMergeTarget,
                                 IMediumAttachment *pMediumAtt,
                                 MachineState_T aMachineState,
                                 HRESULT *phrc,
                                 bool fAttachDetach,
                                 bool fForceUnmount,
                                 bool fHotplug,
                                 PUVM pUVM,
                                 DeviceType_T *paLedDevType,
                                 PCFGMNODE *ppLunL0);
    int i_configMedium(PCFGMNODE pLunL0,
                       bool fPassthrough,
                       DeviceType_T enmType,
                       bool fUseHostIOCache,
                       bool fBuiltinIoCache,
                       bool fInsertDiskIntegrityDrv,
                       bool fSetupMerge,
                       unsigned uMergeSource,
                       unsigned uMergeTarget,
                       const char *pcszBwGroup,
                       bool fDiscard,
                       bool fNonRotational,
                       IMedium *pMedium,
                       MachineState_T aMachineState,
                       HRESULT *phrc);
    int i_configMediumProperties(PCFGMNODE pCur, IMedium *pMedium, bool *pfHostIP, bool *pfEncrypted);
    static DECLCALLBACK(int) i_reconfigureMediumAttachment(Console *pThis,
                                                           PUVM pUVM,
                                                           const char *pcszDevice,
                                                           unsigned uInstance,
                                                           StorageBus_T enmBus,
                                                           bool fUseHostIOCache,
                                                           bool fBuiltinIoCache,
                                                           bool fInsertDiskIntegrityDrv,
                                                           bool fSetupMerge,
                                                           unsigned uMergeSource,
                                                           unsigned uMergeTarget,
                                                           IMediumAttachment *aMediumAtt,
                                                           MachineState_T aMachineState,
                                                           HRESULT *phrc);
    static DECLCALLBACK(int) i_changeRemovableMedium(Console *pThis,
                                                     PUVM pUVM,
                                                     const char *pcszDevice,
                                                     unsigned uInstance,
                                                     StorageBus_T enmBus,
                                                     bool fUseHostIOCache,
                                                     IMediumAttachment *aMediumAtt,
                                                     bool fForce);

    HRESULT i_attachRawPCIDevices(PUVM pUVM, BusAssignmentManager *BusMgr, PCFGMNODE pDevices);
    void i_attachStatusDriver(PCFGMNODE pCtlInst, PPDMLED *papLeds,
                              uint64_t uFirst, uint64_t uLast,
                              Console::MediumAttachmentMap *pmapMediumAttachments,
                              const char *pcszDevice, unsigned uInstance);

    int i_configNetwork(const char *pszDevice, unsigned uInstance, unsigned uLun,
                        INetworkAdapter *aNetworkAdapter, PCFGMNODE pCfg,
                        PCFGMNODE pLunL0, PCFGMNODE pInst,
                        bool fAttachDetach, bool fIgnoreConnectFailure);

    static DECLCALLBACK(int) i_configGuestProperties(void *pvConsole, PUVM pUVM);
    static DECLCALLBACK(int) i_configGuestControl(void *pvConsole);
    static DECLCALLBACK(void) i_vmstateChangeCallback(PUVM pUVM, VMSTATE enmState, VMSTATE enmOldState, void *pvUser);
    static DECLCALLBACK(int) i_unplugCpu(Console *pThis, PUVM pUVM, VMCPUID idCpu);
    static DECLCALLBACK(int) i_plugCpu(Console *pThis, PUVM pUVM, VMCPUID idCpu);
    HRESULT i_doMediumChange(IMediumAttachment *aMediumAttachment, bool fForce, PUVM pUVM);
    HRESULT i_doCPURemove(ULONG aCpu, PUVM pUVM);
    HRESULT i_doCPUAdd(ULONG aCpu, PUVM pUVM);

    HRESULT i_doNetworkAdapterChange(PUVM pUVM, const char *pszDevice, unsigned uInstance,
                                     unsigned uLun, INetworkAdapter *aNetworkAdapter);
    static DECLCALLBACK(int) i_changeNetworkAttachment(Console *pThis, PUVM pUVM, const char *pszDevice,
                                                       unsigned uInstance, unsigned uLun,
                                                       INetworkAdapter *aNetworkAdapter);

    void i_changeClipboardMode(ClipboardMode_T aClipboardMode);
    int i_changeDnDMode(DnDMode_T aDnDMode);

#ifdef VBOX_WITH_USB
    HRESULT i_attachUSBDevice(IUSBDevice *aHostDevice, ULONG aMaskedIfs, const Utf8Str &aCaptureFilename);
    HRESULT i_detachUSBDevice(const ComObjPtr<OUSBDevice> &aHostDevice);

    static DECLCALLBACK(int) i_usbAttachCallback(Console *that, PUVM pUVM, IUSBDevice *aHostDevice, PCRTUUID aUuid,
                                                 const char *aBackend, const char *aAddress, void *pvRemoteBackend,
                                                 USHORT aPortVersion, ULONG aMaskedIfs, const char *pszCaptureFilename);
    static DECLCALLBACK(int) i_usbDetachCallback(Console *that, PUVM pUVM, PCRTUUID aUuid);
#endif

    static DECLCALLBACK(int) i_attachStorageDevice(Console *pThis,
                                                   PUVM pUVM,
                                                   const char *pcszDevice,
                                                   unsigned uInstance,
                                                   StorageBus_T enmBus,
                                                   bool fUseHostIOCache,
                                                   IMediumAttachment *aMediumAtt,
                                                   bool fSilent);
    static DECLCALLBACK(int) i_detachStorageDevice(Console *pThis,
                                                   PUVM pUVM,
                                                   const char *pcszDevice,
                                                   unsigned uInstance,
                                                   StorageBus_T enmBus,
                                                   IMediumAttachment *aMediumAtt,
                                                   bool fSilent);
    HRESULT i_doStorageDeviceAttach(IMediumAttachment *aMediumAttachment, PUVM pUVM, bool fSilent);
    HRESULT i_doStorageDeviceDetach(IMediumAttachment *aMediumAttachment, PUVM pUVM, bool fSilent);

    static DECLCALLBACK(int)    i_stateProgressCallback(PUVM pUVM, unsigned uPercent, void *pvUser);

    static DECLCALLBACK(void)   i_genericVMSetErrorCallback(PUVM pUVM, void *pvUser, int rc, RT_SRC_POS_DECL,
                                                            const char *pszErrorFmt, va_list va);

    void                        i_atVMRuntimeErrorCallbackF(uint32_t fFatal, const char *pszErrorId, const char *pszFormat, ...);
    static DECLCALLBACK(void)   i_atVMRuntimeErrorCallback(PUVM pUVM, void *pvUser, uint32_t fFatal,
                                                           const char *pszErrorId, const char *pszFormat, va_list va);

    HRESULT                     i_captureUSBDevices(PUVM pUVM);
    void                        i_detachAllUSBDevices(bool aDone);


    static DECLCALLBACK(int)    i_vmm2User_SaveState(PCVMM2USERMETHODS pThis, PUVM pUVM);
    static DECLCALLBACK(void)   i_vmm2User_NotifyEmtInit(PCVMM2USERMETHODS pThis, PUVM pUVM, PUVMCPU pUVCpu);
    static DECLCALLBACK(void)   i_vmm2User_NotifyEmtTerm(PCVMM2USERMETHODS pThis, PUVM pUVM, PUVMCPU pUVCpu);
    static DECLCALLBACK(void)   i_vmm2User_NotifyPdmtInit(PCVMM2USERMETHODS pThis, PUVM pUVM);
    static DECLCALLBACK(void)   i_vmm2User_NotifyPdmtTerm(PCVMM2USERMETHODS pThis, PUVM pUVM);
    static DECLCALLBACK(void)   i_vmm2User_NotifyResetTurnedIntoPowerOff(PCVMM2USERMETHODS pThis, PUVM pUVM);
    static DECLCALLBACK(void *) i_vmm2User_QueryGenericObject(PCVMM2USERMETHODS pThis, PUVM pUVM, PCRTUUID pUuid);

    static DECLCALLBACK(void *) i_drvStatus_QueryInterface(PPDMIBASE pInterface, const char *pszIID);
    static DECLCALLBACK(void)   i_drvStatus_UnitChanged(PPDMILEDCONNECTORS pInterface, unsigned iLUN);
    static DECLCALLBACK(int)    i_drvStatus_MediumEjected(PPDMIMEDIANOTIFY pInterface, unsigned iLUN);
    static DECLCALLBACK(void)   i_drvStatus_Destruct(PPDMDRVINS pDrvIns);
    static DECLCALLBACK(int)    i_drvStatus_Construct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);

    static DECLCALLBACK(int)    i_pdmIfSecKey_KeyRetain(PPDMISECKEY pInterface, const char *pszId, const uint8_t **ppbKey,
                                                        size_t *pcbKey);
    static DECLCALLBACK(int)    i_pdmIfSecKey_KeyRelease(PPDMISECKEY pInterface, const char *pszId);
    static DECLCALLBACK(int)    i_pdmIfSecKey_PasswordRetain(PPDMISECKEY pInterface, const char *pszId, const char **ppszPassword);
    static DECLCALLBACK(int)    i_pdmIfSecKey_PasswordRelease(PPDMISECKEY pInterface, const char *pszId);

    static DECLCALLBACK(int)    i_pdmIfSecKeyHlp_KeyMissingNotify(PPDMISECKEYHLP pInterface);

    int mcAudioRefs;
    volatile uint32_t mcVRDPClients;
    uint32_t mu32SingleRDPClientId; /* The id of a connected client in the single connection mode. */
    volatile  bool mcGuestCredentialsProvided;

    static const char *sSSMConsoleUnit;
    static uint32_t sSSMConsoleVer;

    HRESULT i_loadDataFromSavedState();
    int i_loadStateFileExecInternal(PSSMHANDLE pSSM, uint32_t u32Version);

    static DECLCALLBACK(void)   i_saveStateFileExec(PSSMHANDLE pSSM, void *pvUser);
    static DECLCALLBACK(int)    i_loadStateFileExec(PSSMHANDLE pSSM, void *pvUser, uint32_t uVersion, uint32_t uPass);

#ifdef VBOX_WITH_GUEST_PROPS
    static DECLCALLBACK(int)    i_doGuestPropNotification(void *pvExtension, uint32_t, void *pvParms, uint32_t cbParms);
    HRESULT                     i_doEnumerateGuestProperties(const Utf8Str &aPatterns,
                                                             std::vector<Utf8Str> &aNames,
                                                             std::vector<Utf8Str> &aValues,
                                                             std::vector<LONG64>  &aTimestamps,
                                                             std::vector<Utf8Str> &aFlags);

    void i_guestPropertiesHandleVMReset(void);
    bool i_guestPropertiesVRDPEnabled(void);
    void i_guestPropertiesVRDPUpdateLogon(uint32_t u32ClientId, const char *pszUser, const char *pszDomain);
    void i_guestPropertiesVRDPUpdateActiveClient(uint32_t u32ClientId);
    void i_guestPropertiesVRDPUpdateClientAttach(uint32_t u32ClientId, bool fAttached);
    void i_guestPropertiesVRDPUpdateNameChange(uint32_t u32ClientId, const char *pszName);
    void i_guestPropertiesVRDPUpdateIPAddrChange(uint32_t u32ClientId, const char *pszIPAddr);
    void i_guestPropertiesVRDPUpdateLocationChange(uint32_t u32ClientId, const char *pszLocation);
    void i_guestPropertiesVRDPUpdateOtherInfoChange(uint32_t u32ClientId, const char *pszOtherInfo);
    void i_guestPropertiesVRDPUpdateDisconnect(uint32_t u32ClientId);
#endif

    bool i_isResetTurnedIntoPowerOff(void);

    /** @name Disk encryption support
     * @{ */
    HRESULT i_consoleParseDiskEncryption(const char *psz, const char **ppszEnd);
    HRESULT i_configureEncryptionForDisk(const Utf8Str &strId, unsigned *pcDisksConfigured);
    HRESULT i_clearDiskEncryptionKeysOnAllAttachmentsWithKeyId(const Utf8Str &strId);
    HRESULT i_initSecretKeyIfOnAllAttachments(void);
    int i_consoleParseKeyValue(const char *psz, const char **ppszEnd,
                               char **ppszKey, char **ppszVal);
    void i_removeSecretKeysOnSuspend();
    /** @} */

    /** @name Teleporter support
     * @{ */
    static DECLCALLBACK(int)    i_teleporterSrcThreadWrapper(RTTHREAD hThreadSelf, void *pvUser);
    HRESULT                     i_teleporterSrc(TeleporterStateSrc *pState);
    HRESULT                     i_teleporterSrcReadACK(TeleporterStateSrc *pState, const char *pszWhich, const char *pszNAckMsg = NULL);
    HRESULT                     i_teleporterSrcSubmitCommand(TeleporterStateSrc *pState, const char *pszCommand, bool fWaitForAck = true);
    HRESULT                     i_teleporterTrg(PUVM pUVM, IMachine *pMachine, Utf8Str *pErrorMsg, bool fStartPaused,
                                              Progress *pProgress, bool *pfPowerOffOnFailure);
    static DECLCALLBACK(int)    i_teleporterTrgServeConnection(RTSOCKET Sock, void *pvUser);
    /** @} */

    bool mSavedStateDataLoaded : 1;

    const ComPtr<IMachine> mMachine;
    const ComPtr<IInternalMachineControl> mControl;

    const ComPtr<IVRDEServer> mVRDEServer;

    ConsoleVRDPServer * const mConsoleVRDPServer;
    bool mfVRDEChangeInProcess;
    bool mfVRDEChangePending;
    const ComObjPtr<Guest> mGuest;
    const ComObjPtr<Keyboard> mKeyboard;
    const ComObjPtr<Mouse> mMouse;
    const ComObjPtr<Display> mDisplay;
    const ComObjPtr<MachineDebugger> mDebugger;
    const ComObjPtr<VRDEServerInfo> mVRDEServerInfo;
    /** This can safely be used without holding any locks.
     * An AutoCaller suffices to prevent it being destroy while in use and
     * internally there is a lock providing the necessary serialization. */
    const ComObjPtr<EventSource> mEventSource;
#ifdef VBOX_WITH_EXTPACK
    const ComObjPtr<ExtPackManager> mptrExtPackManager;
#endif
    const ComObjPtr<EmulatedUSB> mEmulatedUSB;

    USBDeviceList mUSBDevices;
    RemoteUSBDeviceList mRemoteUSBDevices;

    SharedFolderDataMap m_mapGlobalSharedFolders;
    SharedFolderDataMap m_mapMachineSharedFolders;
    SharedFolderMap m_mapSharedFolders;             // the console instances

    /** The user mode VM handle. */
    PUVM mpUVM;
    /** Holds the number of "readonly" mpUVM callers (users). */
    uint32_t mVMCallers;
    /** Semaphore posted when the number of mpUVM callers drops to zero. */
    RTSEMEVENT mVMZeroCallersSem;
    /** true when Console has entered the mpUVM destruction phase. */
    bool mVMDestroying : 1;
    /** true when power down is initiated by vmstateChangeCallback (EMT). */
    bool mVMPoweredOff : 1;
    /** true when vmstateChangeCallback shouldn't initiate a power down.  */
    bool mVMIsAlreadyPoweringOff : 1;
    /** true if we already showed the snapshot folder size warning. */
    bool mfSnapshotFolderSizeWarningShown : 1;
    /** true if we already showed the snapshot folder ext4/xfs bug warning. */
    bool mfSnapshotFolderExt4WarningShown : 1;
    /** true if we already listed the disk type of the snapshot folder. */
    bool mfSnapshotFolderDiskTypeShown : 1;
    /** true if a USB controller is available (i.e. USB devices can be attached). */
    bool mfVMHasUsbController : 1;
    /** true if the VM power off was caused by reset. */
    bool mfPowerOffCausedByReset : 1;

    /** Pointer to the VMM -> User (that's us) callbacks. */
    struct MYVMM2USERMETHODS : public VMM2USERMETHODS
    {
        Console *pConsole;
        /** The in-progress snapshot. */
        ISnapshot *pISnapshot;
    } *mpVmm2UserMethods;

    /** The current network attachment type in the VM.
     * This doesn't have to match the network attachment type maintained in the
     * NetworkAdapter. This is needed to change the network attachment
     * dynamically.
     */
    typedef std::vector<NetworkAttachmentType_T> NetworkAttachmentTypeVector;
    NetworkAttachmentTypeVector meAttachmentType;

    VMMDev *                    m_pVMMDev;
    AudioVRDE * const           mAudioVRDE;
#ifdef VBOX_WITH_AUDIO_VIDEOREC
    /** The video recording audio backend. */
    AudioVideoRec * const       mAudioVideoRec;
#endif
    Nvram   * const             mNvram;
#ifdef VBOX_WITH_USB_CARDREADER
    UsbCardReader * const       mUsbCardReader;
#endif
    BusAssignmentManager*       mBusMgr;

    enum
    {
        iLedFloppy  = 0,
        cLedFloppy  = 2,
        iLedIde     = iLedFloppy + cLedFloppy,
        cLedIde     = 4,
        iLedSata    = iLedIde + cLedIde,
        cLedSata    = 30,
        iLedScsi    = iLedSata + cLedSata,
        cLedScsi    = 16,
        iLedSas     = iLedScsi + cLedScsi,
        cLedSas     = 8,
        iLedUsb     = iLedSas + cLedSas,
        cLedUsb     = 8,
        iLedNvme    = iLedUsb + cLedUsb,
        cLedNvme    = 30,
        cLedStorage = cLedFloppy + cLedIde + cLedSata + cLedScsi + cLedSas + cLedUsb + cLedNvme
    };
    DeviceType_T maStorageDevType[cLedStorage];
    PPDMLED      mapStorageLeds[cLedStorage];
    PPDMLED      mapNetworkLeds[36];    /**< @todo adapt this to the maximum network card count */
    PPDMLED      mapSharedFolderLed;
    PPDMLED      mapUSBLed[2];
    PPDMLED      mapCrOglLed;

    MediumAttachmentMap mapMediumAttachments;

    /** List of attached USB storage devices. */
    USBStorageDeviceList mUSBStorageDevices;

    /** Store for secret keys. */
    SecretKeyStore * const m_pKeyStore;
    /** Number of disks configured for encryption. */
    unsigned               m_cDisksEncrypted;
    /** Number of disks which have the key in the map. */
    unsigned               m_cDisksPwProvided;

    /** Pointer to the key consumer -> provider (that's us) callbacks. */
    struct MYPDMISECKEY : public PDMISECKEY
    {
        Console *pConsole;
    } *mpIfSecKey;

    /** Pointer to the key helpers -> provider (that's us) callbacks. */
    struct MYPDMISECKEYHLP : public PDMISECKEYHLP
    {
        Console *pConsole;
    } *mpIfSecKeyHlp;

/* Note: FreeBSD needs this whether netflt is used or not. */
#if ((defined(RT_OS_LINUX) && !defined(VBOX_WITH_NETFLT)) || defined(RT_OS_FREEBSD))
    Utf8Str      maTAPDeviceName[8];
    RTFILE       maTapFD[8];
#endif

    bool mVMStateChangeCallbackDisabled;

    bool mfUseHostClipboard;

    /** Local machine state value. */
    MachineState_T mMachineState;

    /** Machine uuid string. */
    Bstr mstrUuid;

    /** Pointer to the progress object of a live cancelable task.
     *
     * This is currently only used by Console::Teleport(), but is intended to later
     * be used by the live snapshot code path as well.  Actions like
     * Console::PowerDown, which automatically cancels out the running snapshot /
     * teleportation operation, will cancel the teleportation / live snapshot
     * operation before starting. */
    ComPtr<IProgress> mptrCancelableProgress;

    ComPtr<IEventListener> mVmListener;

    friend class VMTask;
    friend class ConsoleVRDPServer;
};

#endif // !____H_CONSOLEIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
