/* $Id: initterm.cpp $ */

/** @file
 * MS COM / XPCOM Abstraction Layer - Initialization and Termination.
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

#if !defined(VBOX_WITH_XPCOM)

# include <iprt/nt/nt-and-windows.h>
# include <iprt/win/objbase.h>

#else /* !defined(VBOX_WITH_XPCOM) */

# include <stdlib.h>

# include <nsIComponentRegistrar.h>
# include <nsIServiceManager.h>
# include <nsCOMPtr.h>
# include <nsEventQueueUtils.h>
# include <nsEmbedString.h>

# include <nsILocalFile.h>
# include <nsIDirectoryService.h>
# include <nsDirectoryServiceDefs.h>

#endif /* !defined(VBOX_WITH_XPCOM) */

#include "VBox/com/com.h"
#include "VBox/com/assert.h"
#include "VBox/com/NativeEventQueue.h"
#include "VBox/com/AutoLock.h"

#include "../include/Logging.h"

#include <iprt/asm.h>
#include <iprt/env.h>
#include <iprt/ldr.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include <VBox/err.h>

namespace com
{

#if defined(VBOX_WITH_XPCOM)

class DirectoryServiceProvider : public nsIDirectoryServiceProvider
{
public:

    NS_DECL_ISUPPORTS

    DirectoryServiceProvider()
        : mCompRegLocation(NULL), mXPTIDatLocation(NULL)
        , mComponentDirLocation(NULL), mCurrProcDirLocation(NULL)
        {}

    virtual ~DirectoryServiceProvider();

    HRESULT init(const char *aCompRegLocation,
                 const char *aXPTIDatLocation,
                 const char *aComponentDirLocation,
                 const char *aCurrProcDirLocation);

    NS_DECL_NSIDIRECTORYSERVICEPROVIDER

private:
    /** @remarks This is not a UTF-8 string. */
    char *mCompRegLocation;
    /** @remarks This is not a UTF-8 string. */
    char *mXPTIDatLocation;
    /** @remarks This is not a UTF-8 string. */
    char *mComponentDirLocation;
    /** @remarks This is not a UTF-8 string. */
    char *mCurrProcDirLocation;
};

NS_IMPL_ISUPPORTS1(DirectoryServiceProvider, nsIDirectoryServiceProvider)

DirectoryServiceProvider::~DirectoryServiceProvider()
{
    if (mCompRegLocation)
    {
        RTStrFree(mCompRegLocation);
        mCompRegLocation = NULL;
    }
    if (mXPTIDatLocation)
    {
        RTStrFree(mXPTIDatLocation);
        mXPTIDatLocation = NULL;
    }
    if (mComponentDirLocation)
    {
        RTStrFree(mComponentDirLocation);
        mComponentDirLocation = NULL;
    }
    if (mCurrProcDirLocation)
    {
        RTStrFree(mCurrProcDirLocation);
        mCurrProcDirLocation = NULL;
    }
}

/**
 *  @param aCompRegLocation Path to compreg.dat, in Utf8.
 *  @param aXPTIDatLocation Path to xpti.data, in Utf8.
 */
HRESULT
DirectoryServiceProvider::init(const char *aCompRegLocation,
                               const char *aXPTIDatLocation,
                               const char *aComponentDirLocation,
                               const char *aCurrProcDirLocation)
{
    AssertReturn(aCompRegLocation, NS_ERROR_INVALID_ARG);
    AssertReturn(aXPTIDatLocation, NS_ERROR_INVALID_ARG);

/** @todo r=bird: Gotta check how this encoding stuff plays out on darwin!
 *  We get down to [VBoxNsxp]NS_NewNativeLocalFile and that file isn't
 *  nsLocalFileUnix.cpp on 32-bit darwin.  On 64-bit darwin it's a question
 *  of what we're doing in IPRT and such...  We should probably add a
 *  RTPathConvertToNative for use here. */
    int vrc = RTStrUtf8ToCurrentCP(&mCompRegLocation, aCompRegLocation);
    if (RT_SUCCESS(vrc))
        vrc = RTStrUtf8ToCurrentCP(&mXPTIDatLocation, aXPTIDatLocation);
    if (RT_SUCCESS(vrc) && aComponentDirLocation)
        vrc = RTStrUtf8ToCurrentCP(&mComponentDirLocation, aComponentDirLocation);
    if (RT_SUCCESS(vrc) && aCurrProcDirLocation)
        vrc = RTStrUtf8ToCurrentCP(&mCurrProcDirLocation, aCurrProcDirLocation);

    return RT_SUCCESS(vrc) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
DirectoryServiceProvider::GetFile(const char *aProp,
                                  PRBool *aPersistent,
                                  nsIFile **aRetval)
{
    nsCOMPtr <nsILocalFile> localFile;
    nsresult rv = NS_ERROR_FAILURE;

    *aRetval = nsnull;
    *aPersistent = PR_TRUE;

    const char *fileLocation = NULL;

    if (strcmp(aProp, NS_XPCOM_COMPONENT_REGISTRY_FILE) == 0)
        fileLocation = mCompRegLocation;
    else if (strcmp(aProp, NS_XPCOM_XPTI_REGISTRY_FILE) == 0)
        fileLocation = mXPTIDatLocation;
    else if (mComponentDirLocation && strcmp(aProp, NS_XPCOM_COMPONENT_DIR) == 0)
        fileLocation = mComponentDirLocation;
    else if (mCurrProcDirLocation && strcmp(aProp, NS_XPCOM_CURRENT_PROCESS_DIR) == 0)
        fileLocation = mCurrProcDirLocation;
    else
        return NS_ERROR_FAILURE;

    rv = NS_NewNativeLocalFile(nsEmbedCString(fileLocation),
                               PR_TRUE, getter_AddRefs(localFile));
    if (NS_FAILED(rv))
        return rv;

    return localFile->QueryInterface(NS_GET_IID(nsIFile), (void **)aRetval);
}

/**
 *  Global XPCOM initialization flag (we maintain it ourselves since XPCOM
 *  doesn't provide such functionality)
 */
static bool volatile gIsXPCOMInitialized = false;

/**
 *  Number of Initialize() calls on the main thread.
 */
static unsigned int gXPCOMInitCount = 0;

#else /* !defined(VBOX_WITH_XPCOM) */

/**
 *  The COM main thread handle. (The first caller of com::Initialize().)
 */
static RTTHREAD volatile gCOMMainThread = NIL_RTTHREAD;

/**
 *  Number of Initialize() calls on the main thread.
 */
static uint32_t gCOMMainInitCount = 0;

#endif /* !defined(VBOX_WITH_XPCOM) */


/**
 * Initializes the COM runtime.
 *
 * This method must be called on each thread of the client application that
 * wants to access COM facilities. The initialization must be performed before
 * calling any other COM method or attempting to instantiate COM objects.
 *
 * On platforms using XPCOM, this method uses the following scheme to search for
 * XPCOM runtime:
 *
 * 1. If the VBOX_APP_HOME environment variable is set, the path it specifies
 *    is used to search XPCOM libraries and components. If this method fails to
 *    initialize XPCOM runtime using this path, it will immediately return a
 *    failure and will NOT check for other paths as described below.
 *
 * 2. If VBOX_APP_HOME is not set, this methods tries the following paths in the
 *    given order:
 *
 *    a) Compiled-in application data directory (as returned by
 *       RTPathAppPrivateArch())
 *    b) "/usr/lib/virtualbox" (Linux only)
 *    c) "/opt/VirtualBox" (Linux only)
 *
 *    The first path for which the initialization succeeds will be used.
 *
 * On MS COM platforms, the COM runtime is provided by the system and does not
 * need to be searched for.
 *
 * Once the COM subsystem is no longer necessary on a given thread, Shutdown()
 * must be called to free resources allocated for it. Note that a thread may
 * call Initialize() several times but for each of tese calls there must be a
 * corresponding Shutdown() call.
 *
 * @return S_OK on success and a COM result code in case of failure.
 */
HRESULT Initialize(bool fGui /*= false*/, bool fAutoRegUpdate /*= true*/)
{
    HRESULT rc = E_FAIL;
    NOREF(fAutoRegUpdate);

#if !defined(VBOX_WITH_XPCOM)

# ifdef VBOX_WITH_AUTO_COM_REG_UPDATE
    /*
     * First time we're called in a process, we refresh the VBox COM
     * registrations.   Use a global mutex to prevent updating when there are
     * API users already active, as that could lead to a bit of a mess.
     */
    if (   fAutoRegUpdate
        && gCOMMainThread == NIL_RTTHREAD)
    {
        SetLastError(ERROR_SUCCESS);
        HANDLE hLeakIt = CreateMutexW(NULL/*pSecAttr*/, FALSE, L"Global\\VirtualBoxComLazyRegistrationMutant");
        DWORD  dwErr   = GetLastError();
        AssertMsg(dwErr == ERROR_SUCCESS || dwErr == ERROR_ALREADY_EXISTS || dwErr == ERROR_ACCESS_DENIED, ("%u\n", dwErr));
        if (dwErr == ERROR_SUCCESS)
        {
            char szPath[RTPATH_MAX];
            int vrc = RTPathAppPrivateArch(szPath, sizeof(szPath));
            if (RT_SUCCESS(vrc))
#  ifndef VBOX_IN_32_ON_64_MAIN_API
                vrc = RTPathAppend(szPath, sizeof(szPath),
                                      RT_MAKE_U64(((PKUSER_SHARED_DATA)MM_SHARED_USER_DATA_VA)->NtMinorVersion,
                                                  ((PKUSER_SHARED_DATA)MM_SHARED_USER_DATA_VA)->NtMajorVersion)
                                   >= RT_MAKE_U64(1/*Lo*/,6/*Hi*/)
                                   ? "VBoxProxyStub.dll" : "VBoxProxyStubLegacy.dll");
#  else
                vrc = RTPathAppend(szPath, sizeof(szPath), "x86\\VBoxProxyStub-x86.dll");
#  endif
            if (RT_SUCCESS(vrc))
            {
                RTLDRMOD hMod;
                vrc = RTLdrLoad(szPath, &hMod);
                if (RT_SUCCESS(vrc))
                {
                    union
                    {
                        void *pv;
                        DECLCALLBACKMEMBER(uint32_t, pfnRegUpdate)(void);
                    } u;
                    vrc = RTLdrGetSymbol(hMod, "VbpsUpdateRegistrations", &u.pv);
                    if (RT_SUCCESS(vrc))
                        u.pfnRegUpdate();
                    /* Just keep it loaded. */
                }
            }
            Assert(hLeakIt != NULL); NOREF(hLeakIt);
        }
    }
# endif

    /*
     * We initialize COM in GUI thread in STA, to be compliant with QT and
     * OLE requirments (for example to allow D&D), while other threads
     * initialized in regular MTA. To allow fast proxyless access from
     * GUI thread to COM objects, we explicitly provide our COM objects
     * with free threaded marshaller.
     * !!!!! Please think twice before touching this code !!!!!
     */
    DWORD flags = fGui ?
                  COINIT_APARTMENTTHREADED
                | COINIT_SPEED_OVER_MEMORY
                :
                  COINIT_MULTITHREADED
                | COINIT_DISABLE_OLE1DDE
                | COINIT_SPEED_OVER_MEMORY;

    rc = CoInitializeEx(NULL, flags);

    /* the overall result must be either S_OK or S_FALSE (S_FALSE means
     * "already initialized using the same apartment model") */
    AssertMsg(rc == S_OK || rc == S_FALSE, ("rc=%08X\n", rc));

    /* To be flow compatible with the XPCOM case, we return here if this isn't
     * the main thread or if it isn't its first initialization call.
     * Note! CoInitializeEx and CoUninitialize does it's own reference
     *       counting, so this exercise is entirely for the EventQueue init. */
    bool fRc;
    RTTHREAD hSelf = RTThreadSelf();
    if (hSelf != NIL_RTTHREAD)
        ASMAtomicCmpXchgHandle(&gCOMMainThread, hSelf, NIL_RTTHREAD, fRc);
    else
        fRc = false;

    if (fGui)
        Assert(RTThreadIsMain(hSelf));

    if (!fRc)
    {
        if (   gCOMMainThread == hSelf
            && SUCCEEDED(rc))
            gCOMMainInitCount++;

        AssertComRC(rc);
        return rc;
    }
    Assert(RTThreadIsMain(hSelf));

    /* this is the first main thread initialization */
    Assert(gCOMMainInitCount == 0);
    if (SUCCEEDED(rc))
        gCOMMainInitCount = 1;

#else /* !defined(VBOX_WITH_XPCOM) */

    /* Unused here */
    NOREF(fGui);

    if (ASMAtomicXchgBool(&gIsXPCOMInitialized, true) == true)
    {
        /* XPCOM is already initialized on the main thread, no special
         * initialization is necessary on additional threads. Just increase
         * the init counter if it's a main thread again (to correctly support
         * nested calls to Initialize()/Shutdown() for compatibility with
         * Win32). */

        nsCOMPtr<nsIEventQueue> eventQ;
        rc = NS_GetMainEventQ(getter_AddRefs(eventQ));

        if (NS_SUCCEEDED(rc))
        {
            PRBool isOnMainThread = PR_FALSE;
            rc = eventQ->IsOnCurrentThread(&isOnMainThread);
            if (NS_SUCCEEDED(rc) && isOnMainThread)
                ++gXPCOMInitCount;
        }

        AssertComRC(rc);
        return rc;
    }
    Assert(RTThreadIsMain(RTThreadSelf()));

    /* this is the first initialization */
    gXPCOMInitCount = 1;

    /* prepare paths for registry files */
    char szCompReg[RTPATH_MAX];
    char szXptiDat[RTPATH_MAX];

    int vrc = GetVBoxUserHomeDirectory(szCompReg, sizeof(szCompReg));
    if (vrc == VERR_ACCESS_DENIED)
        return NS_ERROR_FILE_ACCESS_DENIED;
    AssertRCReturn(vrc, NS_ERROR_FAILURE);
    vrc = RTStrCopy(szXptiDat, sizeof(szXptiDat), szCompReg);
    AssertRCReturn(vrc, NS_ERROR_FAILURE);
# ifdef VBOX_IN_32_ON_64_MAIN_API
    vrc = RTPathAppend(szCompReg, sizeof(szCompReg), "compreg-x86.dat");
    AssertRCReturn(vrc, NS_ERROR_FAILURE);
    vrc = RTPathAppend(szXptiDat, sizeof(szXptiDat), "xpti-x86.dat");
    AssertRCReturn(vrc, NS_ERROR_FAILURE);
# else
    vrc = RTPathAppend(szCompReg, sizeof(szCompReg), "compreg.dat");
    AssertRCReturn(vrc, NS_ERROR_FAILURE);
    vrc = RTPathAppend(szXptiDat, sizeof(szXptiDat), "xpti.dat");
    AssertRCReturn(vrc, NS_ERROR_FAILURE);
# endif

    LogFlowFunc(("component registry  : \"%s\"\n", szCompReg));
    LogFlowFunc(("XPTI data file      : \"%s\"\n", szXptiDat));

    static const char *kAppPathsToProbe[] =
    {
        NULL, /* 0: will use VBOX_APP_HOME */
        NULL, /* 1: will try RTPathAppPrivateArch(), correctly installed release builds will never go further */
        NULL, /* 2: will try parent directory of RTPathAppPrivateArch(), only for testcases in non-hardened builds */
        /* There used to be hard coded paths, but they only caused trouble
         * because they often led to mixing of builds or even versions.
         * If you feel tempted to add anything here, think again. They would
         * only be used if option 1 would not work, which is a sign of a big
         * problem, as it returns a fixed location defined at compile time.
         * It is better to fail than blindly trying to cover the problem. */
    };

    /* Find out the directory where VirtualBox binaries are located */
    for (size_t i = 0; i < RT_ELEMENTS(kAppPathsToProbe); ++ i)
    {
        char szAppHomeDir[RTPATH_MAX];

        if (i == 0)
        {
            /* Use VBOX_APP_HOME if present */
            vrc = RTEnvGetEx(RTENV_DEFAULT, "VBOX_APP_HOME", szAppHomeDir, sizeof(szAppHomeDir), NULL);
            if (vrc == VERR_ENV_VAR_NOT_FOUND)
                continue;
            AssertRC(vrc);
        }
        else if (i == 1)
        {
            /* Use RTPathAppPrivateArch() first */
            vrc = RTPathAppPrivateArch(szAppHomeDir, sizeof(szAppHomeDir));
            AssertRC(vrc);
        }
        else if (i == 2)
        {
# ifdef VBOX_WITH_HARDENING
            continue;
# else /* !VBOX_WITH_HARDENING */
            /* Use parent of RTPathAppPrivateArch() if ends with "testcase" */
            vrc = RTPathAppPrivateArch(szAppHomeDir, sizeof(szAppHomeDir));
            AssertRC(vrc);
            vrc = RTPathStripTrailingSlash(szAppHomeDir);
            AssertRC(vrc);
            char *filename = RTPathFilename(szAppHomeDir);
            if (!filename || strcmp(filename, "testcase"))
                continue;
            RTPathStripFilename(szAppHomeDir);
# endif /* !VBOX_WITH_HARDENING */
        }
        else
        {
            /* Iterate over all other paths */
            RTStrCopy(szAppHomeDir, sizeof(szAppHomeDir), kAppPathsToProbe[i]);
            vrc = VINF_SUCCESS;
        }
        if (RT_FAILURE(vrc))
        {
            rc = NS_ERROR_FAILURE;
            continue;
        }
        char szCompDir[RTPATH_MAX];
        vrc = RTStrCopy(szCompDir, sizeof(szCompDir), szAppHomeDir);
        if (RT_FAILURE(vrc))
        {
            rc = NS_ERROR_FAILURE;
            continue;
        }
        vrc = RTPathAppend(szCompDir, sizeof(szCompDir), "components");
        if (RT_FAILURE(vrc))
        {
            rc = NS_ERROR_FAILURE;
            continue;
        }
        LogFlowFunc(("component directory : \"%s\"\n", szCompDir));

        nsCOMPtr<DirectoryServiceProvider> dsProv;
        dsProv = new DirectoryServiceProvider();
        if (dsProv)
            rc = dsProv->init(szCompReg, szXptiDat, szCompDir, szAppHomeDir);
        else
            rc = NS_ERROR_OUT_OF_MEMORY;
        if (NS_FAILED(rc))
            break;

        /* Setup the application path for NS_InitXPCOM2. Note that we properly
         * answer the NS_XPCOM_CURRENT_PROCESS_DIR query in our directory
         * service provider but it seems to be activated after the directory
         * service is used for the first time (see the source NS_InitXPCOM2). So
         * use the same value here to be on the safe side. */
        nsCOMPtr <nsIFile> appDir;
        {
            char *appDirCP = NULL;
            vrc = RTStrUtf8ToCurrentCP(&appDirCP, szAppHomeDir);
            if (RT_SUCCESS(vrc))
            {
                nsCOMPtr<nsILocalFile> file;
                rc = NS_NewNativeLocalFile(nsEmbedCString(appDirCP),
                                           PR_FALSE, getter_AddRefs(file));
                if (NS_SUCCEEDED(rc))
                    appDir = do_QueryInterface(file, &rc);

                RTStrFree(appDirCP);
            }
            else
                rc = NS_ERROR_FAILURE;
        }
        if (NS_FAILED(rc))
            break;

        /* Set VBOX_XPCOM_HOME to the same app path to make XPCOM sources that
         * still use it instead of the directory service happy */
        vrc = RTEnvSetEx(RTENV_DEFAULT, "VBOX_XPCOM_HOME", szAppHomeDir);
        AssertRC(vrc);

        /* Finally, initialize XPCOM */
        {
            nsCOMPtr<nsIServiceManager> serviceManager;
            rc = NS_InitXPCOM2(getter_AddRefs(serviceManager), appDir, dsProv);
            if (NS_SUCCEEDED(rc))
            {
                nsCOMPtr<nsIComponentRegistrar> registrar =
                    do_QueryInterface(serviceManager, &rc);
                if (NS_SUCCEEDED(rc))
                {
                    rc = registrar->AutoRegister(nsnull);
                    if (NS_SUCCEEDED(rc))
                    {
                        /* We succeeded, stop probing paths */
                        LogFlowFunc(("Succeeded.\n"));
                        break;
                    }
                }
            }
        }

        /* clean up before the new try */
        HRESULT rc2 = NS_ShutdownXPCOM(nsnull);
        if (SUCCEEDED(rc))
            rc = rc2;

        if (i == 0)
        {
            /* We failed with VBOX_APP_HOME, don't probe other paths */
            break;
        }
    }

#endif /* !defined(VBOX_WITH_XPCOM) */

    AssertComRCReturnRC(rc);

    // for both COM and XPCOM, we only get here if this is the main thread;
    // only then initialize the autolock system (AutoLock.cpp)
    Assert(RTThreadIsMain(RTThreadSelf()));
    util::InitAutoLockSystem();

    /*
     * Init the main event queue (ASSUMES it cannot fail).
     */
    if (SUCCEEDED(rc))
        NativeEventQueue::init();

    return rc;
}

HRESULT Shutdown()
{
    HRESULT rc = S_OK;

#if !defined(VBOX_WITH_XPCOM)

    /* EventQueue::uninit reference counting fun. */
    RTTHREAD hSelf = RTThreadSelf();
    if (    hSelf == gCOMMainThread
        &&  hSelf != NIL_RTTHREAD)
    {
        if (-- gCOMMainInitCount == 0)
        {
            NativeEventQueue::uninit();
            ASMAtomicWriteHandle(&gCOMMainThread, NIL_RTTHREAD);
        }
    }

    CoUninitialize();

#else /* !defined(VBOX_WITH_XPCOM) */

    nsCOMPtr<nsIEventQueue> eventQ;
    rc = NS_GetMainEventQ(getter_AddRefs(eventQ));

    if (NS_SUCCEEDED(rc) || rc == NS_ERROR_NOT_AVAILABLE)
    {
        /* NS_ERROR_NOT_AVAILABLE seems to mean that
         * nsIEventQueue::StopAcceptingEvents() has been called (see
         * nsEventQueueService.cpp). We hope that this error code always means
         * just that in this case and assume that we're on the main thread
         * (it's a kind of unexpected behavior if a non-main thread ever calls
         * StopAcceptingEvents() on the main event queue). */

        PRBool isOnMainThread = PR_FALSE;
        if (NS_SUCCEEDED(rc))
        {
            rc = eventQ->IsOnCurrentThread(&isOnMainThread);
            eventQ = nsnull; /* early release before shutdown */
        }
        else
        {
            isOnMainThread = RTThreadIsMain(RTThreadSelf());
            rc = NS_OK;
        }

        if (NS_SUCCEEDED(rc) && isOnMainThread)
        {
            /* only the main thread needs to uninitialize XPCOM and only if
             * init counter drops to zero */
            if (--gXPCOMInitCount == 0)
            {
                NativeEventQueue::uninit();
                rc = NS_ShutdownXPCOM(nsnull);

                /* This is a thread initialized XPCOM and set gIsXPCOMInitialized to
                 * true. Reset it back to false. */
                bool wasInited = ASMAtomicXchgBool(&gIsXPCOMInitialized, false);
                Assert(wasInited == true);
                NOREF(wasInited);
            }
        }
    }

#endif /* !defined(VBOX_WITH_XPCOM) */

    AssertComRC(rc);

    return rc;
}

} /* namespace com */
