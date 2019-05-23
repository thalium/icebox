/* $Id: MachineDebuggerImpl.h $ */
/** @file
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

#ifndef ____H_MACHINEDEBUGGER
#define ____H_MACHINEDEBUGGER

#include "MachineDebuggerWrap.h"
#include <iprt/log.h>
#include <VBox/vmm/em.h>

class Console;

class ATL_NO_VTABLE MachineDebugger :
    public MachineDebuggerWrap
{

public:

    DECLARE_EMPTY_CTOR_DTOR (MachineDebugger)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init (Console *aParent);
    void uninit();

    // "public-private methods"
    void i_flushQueuedSettings();

private:

    // wrapped IMachineDeugger properties
    HRESULT getSingleStep(BOOL *aSingleStep);
    HRESULT setSingleStep(BOOL aSingleStep);
    HRESULT getRecompileUser(BOOL *aRecompileUser);
    HRESULT setRecompileUser(BOOL aRecompileUser);
    HRESULT getRecompileSupervisor(BOOL *aRecompileSupervisor);
    HRESULT setRecompileSupervisor(BOOL aRecompileSupervisor);
    HRESULT getExecuteAllInIEM(BOOL *aExecuteAllInIEM);
    HRESULT setExecuteAllInIEM(BOOL aExecuteAllInIEM);
    HRESULT getPATMEnabled(BOOL *aPATMEnabled);
    HRESULT setPATMEnabled(BOOL aPATMEnabled);
    HRESULT getCSAMEnabled(BOOL *aCSAMEnabled);
    HRESULT setCSAMEnabled(BOOL aCSAMEnabled);
    HRESULT getLogEnabled(BOOL *aLogEnabled);
    HRESULT setLogEnabled(BOOL aLogEnabled);
    HRESULT getLogDbgFlags(com::Utf8Str &aLogDbgFlags);
    HRESULT getLogDbgGroups(com::Utf8Str &aLogDbgGroups);
    HRESULT getLogDbgDestinations(com::Utf8Str &aLogDbgDestinations);
    HRESULT getLogRelFlags(com::Utf8Str &aLogRelFlags);
    HRESULT getLogRelGroups(com::Utf8Str &aLogRelGroups);
    HRESULT getLogRelDestinations(com::Utf8Str &aLogRelDestinations);
    HRESULT getHWVirtExEnabled(BOOL *aHWVirtExEnabled);
    HRESULT getHWVirtExNestedPagingEnabled(BOOL *aHWVirtExNestedPagingEnabled);
    HRESULT getHWVirtExVPIDEnabled(BOOL *aHWVirtExVPIDEnabled);
    HRESULT getHWVirtExUXEnabled(BOOL *aHWVirtExUXEnabled);
    HRESULT getOSName(com::Utf8Str &aOSName);
    HRESULT getOSVersion(com::Utf8Str &aOSVersion);
    HRESULT getPAEEnabled(BOOL *aPAEEnabled);
    HRESULT getVirtualTimeRate(ULONG *aVirtualTimeRate);
    HRESULT setVirtualTimeRate(ULONG aVirtualTimeRate);
    HRESULT getVM(LONG64 *aVM);
    HRESULT getUptime(LONG64 *aUptime);

    // wrapped IMachineDeugger properties
    HRESULT dumpGuestCore(const com::Utf8Str &aFilename,
                          const com::Utf8Str &aCompression);
    HRESULT dumpHostProcessCore(const com::Utf8Str &aFilename,
                                const com::Utf8Str &aCompression);
    HRESULT info(const com::Utf8Str &aName,
                 const com::Utf8Str &aArgs,
                 com::Utf8Str &aInfo);
    HRESULT injectNMI();
    HRESULT modifyLogGroups(const com::Utf8Str &aSettings);
    HRESULT modifyLogFlags(const com::Utf8Str &aSettings);
    HRESULT modifyLogDestinations(const com::Utf8Str &aSettings);
    HRESULT readPhysicalMemory(LONG64 aAddress,
                               ULONG aSize,
                               std::vector<BYTE> &aBytes);
    HRESULT writePhysicalMemory(LONG64 aAddress,
                                ULONG aSize,
                                const std::vector<BYTE> &aBytes);
    HRESULT readVirtualMemory(ULONG aCpuId,
                              LONG64 aAddress,
                              ULONG aSize,
                              std::vector<BYTE> &aBytes);
    HRESULT writeVirtualMemory(ULONG aCpuId,
                               LONG64 aAddress,
                               ULONG aSize,
                               const std::vector<BYTE> &aBytes);
    HRESULT loadPlugIn(const com::Utf8Str &aName,
                       com::Utf8Str &aPlugInName);
    HRESULT unloadPlugIn(const com::Utf8Str &aName);
    HRESULT detectOS(com::Utf8Str &aOs);
    HRESULT queryOSKernelLog(ULONG aMaxMessages,
                             com::Utf8Str &aDmesg);
    HRESULT getRegister(ULONG aCpuId,
                        const com::Utf8Str &aName,
                        com::Utf8Str &aValue);
    HRESULT getRegisters(ULONG aCpuId,
                         std::vector<com::Utf8Str> &aNames,
                         std::vector<com::Utf8Str> &aValues);
    HRESULT setRegister(ULONG aCpuId,
                        const com::Utf8Str &aName,
                        const com::Utf8Str &aValue);
    HRESULT setRegisters(ULONG aCpuId,
                         const std::vector<com::Utf8Str> &aNames,
                         const std::vector<com::Utf8Str> &aValues);
    HRESULT dumpGuestStack(ULONG aCpuId,
                           com::Utf8Str &aStack);
    HRESULT resetStats(const com::Utf8Str &aPattern);
    HRESULT dumpStats(const com::Utf8Str &aPattern);
    HRESULT getStats(const com::Utf8Str &aPattern,
                     BOOL aWithDescriptions,
                     com::Utf8Str &aStats);

    // private methods
    bool i_queueSettings() const;
    HRESULT i_getEmExecPolicyProperty(EMEXECPOLICY enmPolicy, BOOL *pfEnforced);
    HRESULT i_setEmExecPolicyProperty(EMEXECPOLICY enmPolicy, BOOL fEnforce);

    /** RTLogGetFlags, RTLogGetGroupSettings and RTLogGetDestinations function. */
    typedef DECLCALLBACK(int) FNLOGGETSTR(PRTLOGGER, char *, size_t);
    /** Function pointer.  */
    typedef FNLOGGETSTR *PFNLOGGETSTR;
    HRESULT i_logStringProps(PRTLOGGER pLogger, PFNLOGGETSTR pfnLogGetStr, const char *pszLogGetStr, Utf8Str *pstrSettings);

    Console * const mParent;
    /** @name Flags whether settings have been queued because they could not be sent
     *        to the VM (not up yet, etc.)
     * @{ */
    uint8_t maiQueuedEmExecPolicyParams[EMEXECPOLICY_END];
    int mSingleStepQueued;
    int mRecompileUserQueued;
    int mRecompileSupervisorQueued;
    int mPatmEnabledQueued;
    int mCsamEnabledQueued;
    int mLogEnabledQueued;
    uint32_t mVirtualTimeRateQueued;
    bool mFlushMode;
    /** @}  */
};

#endif /* !____H_MACHINEDEBUGGER */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
