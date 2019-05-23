/** @file
 * VirtualBox - Extension Pack Interface.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_ExtPack_ExtPack_h
#define ___VBox_ExtPack_ExtPack_h

#include <VBox/types.h>

/** @def VBOXEXTPACK_IF_CS
 * Selects 'class' on 'struct' for interface references.
 * @param I         The interface name
 */
#if defined(__cplusplus) && !defined(RT_OS_WINDOWS)
# define VBOXEXTPACK_IF_CS(I)   class I
#else
# define VBOXEXTPACK_IF_CS(I)   struct I
#endif

VBOXEXTPACK_IF_CS(IConsole);
VBOXEXTPACK_IF_CS(IMachine);
VBOXEXTPACK_IF_CS(IVirtualBox);

/**
 * Module kind for use with VBOXEXTPACKHLP::pfnFindModule.
 */
typedef enum VBOXEXTPACKMODKIND
{
    /** Zero is invalid as always. */
    VBOXEXTPACKMODKIND_INVALID = 0,
    /** Raw-mode context module. */
    VBOXEXTPACKMODKIND_RC,
    /** Ring-0 context module. */
    VBOXEXTPACKMODKIND_R0,
    /** Ring-3 context module. */
    VBOXEXTPACKMODKIND_R3,
    /** End of the valid values (exclusive). */
    VBOXEXTPACKMODKIND_END,
    /** The usual 32-bit type hack. */
    VBOXEXTPACKMODKIND_32BIT_HACK = 0x7fffffff
} VBOXEXTPACKMODKIND;

/**
 * Contexts returned by VBOXEXTPACKHLP::pfnGetContext.
 */
typedef enum VBOXEXTPACKCTX
{
    /** Zero is invalid as always. */
    VBOXEXTPACKCTX_INVALID = 0,
    /** The per-user daemon process (VBoxSVC). */
    VBOXEXTPACKCTX_PER_USER_DAEMON,
    /** A VM process. */
    VBOXEXTPACKCTX_VM_PROCESS,
    /** An API client process.
     * @remarks This will not be returned by VirtualBox yet. */
    VBOXEXTPACKCTX_CLIENT_PROCESS,
    /** End of the valid values (exclusive). */
    VBOXEXTPACKCTX_END,
    /** The usual 32-bit type hack. */
    VBOXEXTPACKCTX_32BIT_HACK = 0x7fffffff
} VBOXEXTPACKCTX;


/** Pointer to const helpers passed to the VBoxExtPackRegister() call. */
typedef const struct VBOXEXTPACKHLP *PCVBOXEXTPACKHLP;
/**
 * Extension pack helpers passed to VBoxExtPackRegister().
 *
 * This will be valid until the module is unloaded.
 */
typedef struct VBOXEXTPACKHLP
{
    /** Interface version.
     * This is set to VBOXEXTPACKHLP_VERSION. */
    uint32_t                    u32Version;

    /** The VirtualBox full version (see VBOX_FULL_VERSION).  */
    uint32_t                    uVBoxFullVersion;
    /** The VirtualBox subversion tree revision.  */
    uint32_t                    uVBoxInternalRevision;
    /** Explicit alignment padding, must be zero. */
    uint32_t                    u32Padding;
    /** Pointer to the version string (read-only). */
    const char                 *pszVBoxVersion;

    /**
     * Finds a module belonging to this extension pack.
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pszName         The module base name.
     * @param   pszExt          The extension. If NULL the default ring-3
     *                          library extension will be used.
     * @param   enmKind         The kind of module to locate.
     * @param   pszFound        Where to return the path to the module on
     *                          success.
     * @param   cbFound         The size of the buffer @a pszFound points to.
     * @param   pfNative        Where to return the native/agnostic indicator.
     */
    DECLR3CALLBACKMEMBER(int, pfnFindModule,(PCVBOXEXTPACKHLP pHlp, const char *pszName, const char *pszExt,
                                             VBOXEXTPACKMODKIND enmKind,
                                             char *pszFound, size_t cbFound, bool *pfNative));

    /**
     * Gets the path to a file belonging to this extension pack.
     *
     * @returns VBox status code.
     * @retval  VERR_INVALID_POINTER if any of the pointers are invalid.
     * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small.  The buffer
     *          will contain nothing.
     *
     * @param   pHlp            Pointer to this helper structure.
     * @param   pszFilename     The filename.
     * @param   pszPath         Where to return the path to the file on
     *                          success.
     * @param   cbPath          The size of the buffer @a pszPath.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetFilePath,(PCVBOXEXTPACKHLP pHlp, const char *pszFilename, char *pszPath, size_t cbPath));

    /**
     * Gets the context the extension pack is operating in.
     *
     * @returns The context.
     * @retval  VBOXEXTPACKCTX_INVALID if @a pHlp is invalid.
     *
     * @param   pHlp            Pointer to this helper structure.
     */
    DECLR3CALLBACKMEMBER(VBOXEXTPACKCTX, pfnGetContext,(PCVBOXEXTPACKHLP pHlp));

    /**
     * Loads a HGCM service provided by an extension pack.
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pConsole        Pointer to the VM's console object.
     * @param   pszServiceLibrary Name of the library file containing the
     *                          service implementation, without extension.
     * @param   pszServiceName  Name of HGCM service.
     */
    DECLR3CALLBACKMEMBER(int, pfnLoadHGCMService,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IConsole) *pConsole,
                                                  const char *pszServiceLibrary, const char *pszServiceName));

    /**
     * Loads a VD plugin provided by an extension pack.
     *
     * This makes sense only in the context of the per-user service (VBoxSVC).
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pVirtualBox     Pointer to the VirtualBox object.
     * @param   pszPluginLibrary Name of the library file containing the plugin
     *                          implementation, without extension.
     */
    DECLR3CALLBACKMEMBER(int, pfnLoadVDPlugin,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                               const char *pszPluginLibrary));

    /**
     * Unloads a VD plugin provided by an extension pack.
     *
     * This makes sense only in the context of the per-user service (VBoxSVC).
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pVirtualBox     Pointer to the VirtualBox object.
     * @param   pszPluginLibrary Name of the library file containing the plugin
     *                          implementation, without extension.
     */
    DECLR3CALLBACKMEMBER(int, pfnUnloadVDPlugin,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                                 const char *pszPluginLibrary));

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */

    /** End of structure marker (VBOXEXTPACKHLP_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXEXTPACKHLP;
/** Current version of the VBOXEXTPACKHLP structure.  */
#define VBOXEXTPACKHLP_VERSION          RT_MAKE_U32(2, 1)


/** Pointer to the extension pack callback table. */
typedef struct VBOXEXTPACKREG const *PCVBOXEXTPACKREG;
/**
 * Callback table returned by VBoxExtPackRegister.
 *
 * All the callbacks are called the context of the per-user service (VBoxSVC).
 *
 * This must be valid until the extension pack main module is unloaded.
 */
typedef struct VBOXEXTPACKREG
{
    /** Interface version.
     * This is set to VBOXEXTPACKREG_VERSION. */
    uint32_t                    u32Version;
    /** The VirtualBox version this extension pack was built against.  */
    uint32_t                    uVBoxVersion;

    /**
     * Hook for doing setups after the extension pack was installed.
     *
     * @returns VBox status code.
     * @retval  VERR_EXTPACK_UNSUPPORTED_HOST_UNINSTALL if the extension pack
     *          requires some different host version or a prerequisite is
     *          missing from the host.  Automatic uninstall will be attempted.
     *          Must set error info.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     * @param   pErrInfo    Where to return extended error information.
     */
    DECLCALLBACKMEMBER(int, pfnInstalled)(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                          PRTERRINFO pErrInfo);

    /**
     * Hook for cleaning up before the extension pack is uninstalled.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     *
     * @todo    This is currently called holding locks making pVirtualBox
     *          relatively unusable.
     */
    DECLCALLBACKMEMBER(int, pfnUninstall)(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox);

    /**
     * Hook for doing work after the VirtualBox object is ready.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     */
    DECLCALLBACKMEMBER(void, pfnVirtualBoxReady)(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox);

    /**
     * Hook for doing work before unloading.
     *
     * @param   pThis       Pointer to this structure.
     *
     * @remarks The helpers are not available at this point in time.
     * @remarks This is not called on uninstall, then pfnUninstall will be the
     *          last callback.
     */
    DECLCALLBACKMEMBER(void, pfnUnload)(PCVBOXEXTPACKREG pThis);

    /**
     * Hook for changing the default VM configuration upon creation.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     * @param   pMachine    The machine interface.
     */
    DECLCALLBACKMEMBER(int, pfnVMCreated)(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                          VBOXEXTPACK_IF_CS(IMachine) *pMachine);

    /**
     * Query the IUnknown interface to an object in the main module.
     *
     * @returns IUnknown pointer (referenced) on success, NULL on failure.
     * @param   pThis       Pointer to this structure.
     * @param   pObjectId   Pointer to the object ID (UUID).
     */
    DECLCALLBACKMEMBER(void *, pfnQueryObject)(PCVBOXEXTPACKREG pThis, PCRTUUID pObjectId);

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */

    /** Reserved for minor structure revisions. */
    uint32_t                    uReserved7;

    /** End of structure marker (VBOXEXTPACKREG_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXEXTPACKREG;
/** Current version of the VBOXEXTPACKREG structure.  */
#define VBOXEXTPACKREG_VERSION        RT_MAKE_U32(2, 0)


/**
 * The VBoxExtPackRegister callback function.
 *
 * The Main API (as in VBoxSVC) will invoke this function after loading an
 * extension pack Main module. Its job is to do version compatibility checking
 * and returning the extension pack registration structure.
 *
 * @returns VBox status code.
 * @param   pHlp            Pointer to the extension pack helper function
 *                          table.  This is valid until the module is unloaded.
 * @param   ppReg           Where to return the pointer to the registration
 *                          structure containing all the hooks.  This structure
 *                          be valid and unchanged until the module is unloaded
 *                          (i.e. use some static const data for it).
 * @param   pErrInfo        Where to return extended error information.
 */
typedef DECLCALLBACK(int) FNVBOXEXTPACKREGISTER(PCVBOXEXTPACKHLP pHlp, PCVBOXEXTPACKREG *ppReg, PRTERRINFO pErrInfo);
/** Pointer to a FNVBOXEXTPACKREGISTER. */
typedef FNVBOXEXTPACKREGISTER *PFNVBOXEXTPACKREGISTER;

/** The name of the main module entry point. */
#define VBOX_EXTPACK_MAIN_MOD_ENTRY_POINT   "VBoxExtPackRegister"


/** Pointer to the extension pack VM callback table. */
typedef struct VBOXEXTPACKVMREG const *PCVBOXEXTPACKVMREG;
/**
 * Callback table returned by VBoxExtPackVMRegister.
 *
 * All the callbacks are called the context of a VM process.
 *
 * This must be valid until the extension pack main VM module is unloaded.
 */
typedef struct VBOXEXTPACKVMREG
{
    /** Interface version.
     * This is set to VBOXEXTPACKVMREG_VERSION. */
    uint32_t                    u32Version;
    /** The VirtualBox version this extension pack was built against.  */
    uint32_t                    uVBoxVersion;

    /**
     * Hook for doing work after the Console object is ready.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The Console interface.
     */
    DECLCALLBACKMEMBER(void, pfnConsoleReady)(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole);

    /**
     * Hook for doing work before unloading.
     *
     * @param   pThis       Pointer to this structure.
     *
     * @remarks The helpers are not available at this point in time.
     */
    DECLCALLBACKMEMBER(void, pfnUnload)(PCVBOXEXTPACKVMREG pThis);

    /**
     * Hook for configuring the VMM for a VM.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The console interface.
     * @param   pVM         The cross context VM structure.
     */
    DECLCALLBACKMEMBER(int, pfnVMConfigureVMM)(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole, PVM pVM);

    /**
     * Hook for doing work right before powering on the VM.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The console interface.
     * @param   pVM         The cross context VM structure.
     */
    DECLCALLBACKMEMBER(int, pfnVMPowerOn)(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole, PVM pVM);

    /**
     * Hook for doing work after powering off the VM.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The console interface.
     * @param   pVM         The cross context VM structure. Can be NULL.
     */
    DECLCALLBACKMEMBER(void, pfnVMPowerOff)(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole, PVM pVM);

    /**
     * Query the IUnknown interface to an object in the main VM module.
     *
     * @returns IUnknown pointer (referenced) on success, NULL on failure.
     * @param   pThis       Pointer to this structure.
     * @param   pObjectId   Pointer to the object ID (UUID).
     */
    DECLCALLBACKMEMBER(void *, pfnQueryObject)(PCVBOXEXTPACKVMREG pThis, PCRTUUID pObjectId);

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */

    /** Reserved for minor structure revisions. */
    uint32_t                    uReserved7;

    /** End of structure marker (VBOXEXTPACKVMREG_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXEXTPACKVMREG;
/** Current version of the VBOXEXTPACKVMREG structure.  */
#define VBOXEXTPACKVMREG_VERSION      RT_MAKE_U32(2, 0)


/**
 * The VBoxExtPackVMRegister callback function.
 *
 * The Main API (in a VM process) will invoke this function after loading an
 * extension pack VM module. Its job is to do version compatibility checking
 * and returning the extension pack registration structure for a VM.
 *
 * @returns VBox status code.
 * @param   pHlp            Pointer to the extension pack helper function
 *                          table.  This is valid until the module is unloaded.
 * @param   ppReg           Where to return the pointer to the registration
 *                          structure containing all the hooks.  This structure
 *                          be valid and unchanged until the module is unloaded
 *                          (i.e. use some static const data for it).
 * @param   pErrInfo        Where to return extended error information.
 */
typedef DECLCALLBACK(int) FNVBOXEXTPACKVMREGISTER(PCVBOXEXTPACKHLP pHlp, PCVBOXEXTPACKVMREG *ppReg, PRTERRINFO pErrInfo);
/** Pointer to a FNVBOXEXTPACKVMREGISTER. */
typedef FNVBOXEXTPACKVMREGISTER *PFNVBOXEXTPACKVMREGISTER;

/** The name of the main VM module entry point. */
#define VBOX_EXTPACK_MAIN_VM_MOD_ENTRY_POINT   "VBoxExtPackVMRegister"


/**
 * Checks if extension pack interface version is compatible.
 *
 * @returns true if the do, false if they don't.
 * @param   u32Provider     The provider version.
 * @param   u32User         The user version.
 */
#define VBOXEXTPACK_IS_VER_COMPAT(u32Provider, u32User) \
    (    VBOXEXTPACK_IS_MAJOR_VER_EQUAL(u32Provider, u32User) \
      && (int32_t)RT_LOWORD(u32Provider) >= (int32_t)RT_LOWORD(u32User) ) /* stupid casts to shut up gcc */

/**
 * Check if two extension pack interface versions has the same major version.
 *
 * @returns true if the do, false if they don't.
 * @param   u32Ver1         The first version number.
 * @param   u32Ver2         The second version number.
 */
#define VBOXEXTPACK_IS_MAJOR_VER_EQUAL(u32Ver1, u32Ver2)  (RT_HIWORD(u32Ver1) == RT_HIWORD(u32Ver2))

#endif

