/* $Id: ExtPackManagerImpl.h $ */
/** @file
 * VirtualBox Main - interface for Extension Packs, VBoxSVC & VBoxC.
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

#ifndef ____H_EXTPACKMANAGERIMPL
#define ____H_EXTPACKMANAGERIMPL

#include "VirtualBoxBase.h"
#include <VBox/ExtPack/ExtPack.h>
#include "ExtPackWrap.h"
#include "ExtPackFileWrap.h"
#include "ExtPackManagerWrap.h"
#include <iprt/fs.h>


/** The name of the oracle extension back. */
#define ORACLE_PUEL_EXTPACK_NAME "Oracle VM VirtualBox Extension Pack"


#ifndef VBOX_COM_INPROC
/**
 * An extension pack file.
 */
class ATL_NO_VTABLE ExtPackFile :
    public ExtPackFileWrap
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_EMPTY_CTOR_DTOR(ExtPackFile)

    HRESULT     FinalConstruct();
    void        FinalRelease();
    HRESULT     initWithFile(const char *a_pszFile, const char *a_pszDigest, class ExtPackManager *a_pExtPackMgr, VirtualBox *a_pVirtualBox);
    void        uninit();
    /** @}  */

private:
    /** @name Misc init helpers
     * @{ */
    HRESULT     initFailed(const char *a_pszWhyFmt, ...);
    /** @} */

private:

    // wrapped IExtPackFile properties
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getDescription(com::Utf8Str &aDescription);
    HRESULT getVersion(com::Utf8Str &aVersion);
    HRESULT getRevision(ULONG *aRevision);
    HRESULT getEdition(com::Utf8Str &aEdition);
    HRESULT getVRDEModule(com::Utf8Str &aVRDEModule);
    HRESULT getPlugIns(std::vector<ComPtr<IExtPackPlugIn> > &aPlugIns);
    HRESULT getUsable(BOOL *aUsable);
    HRESULT getWhyUnusable(com::Utf8Str &aWhyUnusable);
    HRESULT getShowLicense(BOOL *aShowLicense);
    HRESULT getLicense(com::Utf8Str &aLicense);
    HRESULT getFilePath(com::Utf8Str &aFilePath);

    // wrapped IExtPackFile methods
    HRESULT queryLicense(const com::Utf8Str &aPreferredLocale,
                         const com::Utf8Str &aPreferredLanguage,
                         const com::Utf8Str &aFormat,
                         com::Utf8Str &aLicenseText);
    HRESULT install(BOOL aReplace,
                    const com::Utf8Str &aDisplayInfo,
                    ComPtr<IProgress> &aProgess);

    struct Data;
    /** Pointer to the private instance. */
    Data *m;

    friend class ExtPackManager;
    friend class ExtPackInstallTask;
};
#endif /* !VBOX_COM_INPROC */


/**
 * An installed extension pack.
 */
class ATL_NO_VTABLE ExtPack :
    public ExtPackWrap
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_EMPTY_CTOR_DTOR(ExtPack)

    HRESULT     FinalConstruct();
    void        FinalRelease();
    HRESULT     initWithDir(VBOXEXTPACKCTX a_enmContext, const char *a_pszName, const char *a_pszDir);
    void        uninit();
    RTMEMEF_NEW_AND_DELETE_OPERATORS();
    /** @}  */

    /** @name Internal interfaces used by ExtPackManager.
     * @{ */
#ifndef VBOX_COM_INPROC
    bool        i_callInstalledHook(IVirtualBox *a_pVirtualBox, AutoWriteLock *a_pLock, PRTERRINFO pErrInfo);
    HRESULT     i_callUninstallHookAndClose(IVirtualBox *a_pVirtualBox, bool a_fForcedRemoval);
    bool        i_callVirtualBoxReadyHook(IVirtualBox *a_pVirtualBox, AutoWriteLock *a_pLock);
#endif
#ifdef VBOX_COM_INPROC
    bool        i_callConsoleReadyHook(IConsole *a_pConsole, AutoWriteLock *a_pLock);
#endif
#ifndef VBOX_COM_INPROC
    bool        i_callVmCreatedHook(IVirtualBox *a_pVirtualBox, IMachine *a_pMachine, AutoWriteLock *a_pLock);
#endif
#ifdef VBOX_COM_INPROC
    bool        i_callVmConfigureVmmHook(IConsole *a_pConsole, PVM a_pVM, AutoWriteLock *a_pLock, int *a_pvrc);
    bool        i_callVmPowerOnHook(IConsole *a_pConsole, PVM a_pVM, AutoWriteLock *a_pLock, int *a_pvrc);
    bool        i_callVmPowerOffHook(IConsole *a_pConsole, PVM a_pVM, AutoWriteLock *a_pLock);
#endif
    HRESULT     i_checkVrde(void);
    HRESULT     i_getVrdpLibraryName(Utf8Str *a_pstrVrdeLibrary);
    HRESULT     i_getLibraryName(const char *a_pszModuleName, Utf8Str *a_pstrLibrary);
    bool        i_wantsToBeDefaultVrde(void) const;
    HRESULT     i_refresh(bool *pfCanDelete);
    /** @}  */

protected:
    /** @name Internal helper methods.
     * @{ */
    void        i_probeAndLoad(void);
    bool        i_findModule(const char *a_pszName, const char *a_pszExt, VBOXEXTPACKMODKIND a_enmKind,
                             Utf8Str *a_ppStrFound, bool *a_pfNative, PRTFSOBJINFO a_pObjInfo) const;
    static bool i_objinfoIsEqual(PCRTFSOBJINFO pObjInfo1, PCRTFSOBJINFO pObjInfo2);
    /** @}  */

    /** @name Extension Pack Helpers
     * @{ */
    static DECLCALLBACK(int)    i_hlpFindModule(PCVBOXEXTPACKHLP pHlp, const char *pszName, const char *pszExt,
                                                VBOXEXTPACKMODKIND enmKind, char *pszFound, size_t cbFound, bool *pfNative);
    static DECLCALLBACK(int)    i_hlpGetFilePath(PCVBOXEXTPACKHLP pHlp, const char *pszFilename, char *pszPath, size_t cbPath);
    static DECLCALLBACK(VBOXEXTPACKCTX) i_hlpGetContext(PCVBOXEXTPACKHLP pHlp);
    static DECLCALLBACK(int)    i_hlpLoadHGCMService(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IConsole) *pConsole, const char *pszServiceLibrary, const char *pszServiceName);
    static DECLCALLBACK(int)    i_hlpLoadVDPlugin(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox, const char *pszPluginLibrary);
    static DECLCALLBACK(int)    i_hlpUnloadVDPlugin(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox, const char *pszPluginLibrary);
    static DECLCALLBACK(int)    i_hlpReservedN(PCVBOXEXTPACKHLP pHlp);
    /** @}  */

private:

    // wrapped IExtPack properties
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getDescription(com::Utf8Str &aDescription);
    HRESULT getVersion(com::Utf8Str &aVersion);
    HRESULT getRevision(ULONG *aRevision);
    HRESULT getEdition(com::Utf8Str &aEdition);
    HRESULT getVRDEModule(com::Utf8Str &aVRDEModule);
    HRESULT getPlugIns(std::vector<ComPtr<IExtPackPlugIn> > &aPlugIns);
    HRESULT getUsable(BOOL *aUsable);
    HRESULT getWhyUnusable(com::Utf8Str &aWhyUnusable);
    HRESULT getShowLicense(BOOL *aShowLicense);
    HRESULT getLicense(com::Utf8Str &aLicense);

    // wrapped IExtPack methods
    HRESULT queryLicense(const com::Utf8Str &aPreferredLocale,
                         const com::Utf8Str &aPreferredLanguage,
                         const com::Utf8Str &aFormat,
                         com::Utf8Str &aLicenseText);
    HRESULT queryObject(const com::Utf8Str &aObjUuid,
                        ComPtr<IUnknown> &aReturnInterface);


    struct Data;
    /** Pointer to the private instance. */
    Data *m;

    friend class ExtPackManager;
};


/**
 * Extension pack manager.
 */
class ATL_NO_VTABLE ExtPackManager :
    public ExtPackManagerWrap
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_EMPTY_CTOR_DTOR(ExtPackManager)

    HRESULT     FinalConstruct();
    void        FinalRelease();
    HRESULT     initExtPackManager(VirtualBox *a_pVirtualBox, VBOXEXTPACKCTX a_enmContext);
    void        uninit();
    /** @}  */

    /** @name Internal interfaces used by other Main classes.
     * @{ */
#ifndef VBOX_COM_INPROC
    HRESULT     i_doInstall(ExtPackFile *a_pExtPackFile, bool a_fReplace, Utf8Str const *a_pstrDisplayInfo);
    HRESULT     i_doUninstall(const Utf8Str *a_pstrName, bool a_fForcedRemoval, const Utf8Str *a_pstrDisplayInfo);
    void        i_callAllVirtualBoxReadyHooks(void);
#endif
#ifdef VBOX_COM_INPROC
    void        i_callAllConsoleReadyHooks(IConsole *a_pConsole);
#endif
#ifndef VBOX_COM_INPROC
    void        i_callAllVmCreatedHooks(IMachine *a_pMachine);
#endif
#ifdef VBOX_COM_INPROC
    int         i_callAllVmConfigureVmmHooks(IConsole *a_pConsole, PVM a_pVM);
    int         i_callAllVmPowerOnHooks(IConsole *a_pConsole, PVM a_pVM);
    void        i_callAllVmPowerOffHooks(IConsole *a_pConsole, PVM a_pVM);
#endif
    HRESULT     i_checkVrdeExtPack(Utf8Str const *a_pstrExtPack);
    int         i_getVrdeLibraryPathForExtPack(Utf8Str const *a_pstrExtPack, Utf8Str *a_pstrVrdeLibrary);
    HRESULT     i_getLibraryPathForExtPack(const char *a_pszModuleName, const char *a_pszExtPack, Utf8Str *a_pstrLibrary);
    HRESULT     i_getDefaultVrdeExtPack(Utf8Str *a_pstrExtPack);
    bool        i_isExtPackUsable(const char *a_pszExtPack);
    void        i_dumpAllToReleaseLog(void);
    /** @}  */

private:
    // wrapped IExtPackManager properties
    HRESULT getInstalledExtPacks(std::vector<ComPtr<IExtPack> > &aInstalledExtPacks);

   // wrapped IExtPackManager methods
    HRESULT find(const com::Utf8Str &aName,
                 ComPtr<IExtPack> &aReturnData);
    HRESULT openExtPackFile(const com::Utf8Str &aPath,
                                  ComPtr<IExtPackFile> &aFile);
    HRESULT uninstall(const com::Utf8Str &aName,
                      BOOL aForcedRemoval,
                      const com::Utf8Str &aDisplayInfo,
                      ComPtr<IProgress> &aProgess);
    HRESULT cleanup();
    HRESULT queryAllPlugInsForFrontend(const com::Utf8Str &aFrontendName,
                                       std::vector<com::Utf8Str> &aPlugInModules);
    HRESULT isExtPackUsable(const com::Utf8Str &aName,
                            BOOL *aUsable);

    bool        i_areThereAnyRunningVMs(void) const;
    HRESULT     i_runSetUidToRootHelper(Utf8Str const *a_pstrDisplayInfo, const char *a_pszCommand, ...);
    ExtPack    *i_findExtPack(const char *a_pszName);
    void        i_removeExtPack(const char *a_pszName);
    HRESULT     i_refreshExtPack(const char *a_pszName, bool a_fUnsuableIsError, ExtPack **a_ppExtPack);

private:
    struct Data;
    /** Pointer to the private instance. */
    Data *m;

    friend class ExtPackUninstallTask;
};

#endif
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
