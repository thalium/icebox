/* $Id: SharedFolderImpl.cpp $ */
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

#include "SharedFolderImpl.h"
#if !defined(VBOX_COM_INPROC)
# include "VirtualBoxImpl.h"
# include "MachineImpl.h"
#endif
#include "ConsoleImpl.h"

#include "AutoCaller.h"
#include "Logging.h"

#include <iprt/param.h>
#include <iprt/cpp/utils.h>
#include <iprt/path.h>

/////////////////////////////////////////////////////////////////////////////
// SharedFolder::Data structure
/////////////////////////////////////////////////////////////////////////////

struct SharedFolder::Data
{
    Data()
    : fWritable(false),
      fAutoMount(false)
    { }

    const Utf8Str   strName;
    const Utf8Str   strHostPath;
    bool            fWritable;
    bool            fAutoMount;
    Utf8Str         strLastAccessError;
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

SharedFolder::SharedFolder()
    : mParent(NULL),
#if !defined(VBOX_COM_INPROC)
      mMachine(NULL),
      mVirtualBox(NULL)
#else
      mConsole(NULL)
#endif
{
    m = new Data;
}

SharedFolder::~SharedFolder()
{
    delete m;
    m = NULL;
}

HRESULT SharedFolder::FinalConstruct()
{
    return BaseFinalConstruct();
}

void SharedFolder::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

#if !defined(VBOX_COM_INPROC)
/**
 *  Initializes the shared folder object.
 *
 *  This variant initializes a machine instance that lives in the server address space.
 *
 *  @param aMachine     parent Machine object
 *  @param aName        logical name of the shared folder
 *  @param aHostPath    full path to the shared folder on the host
 *  @param aWritable    writable if true, readonly otherwise
 *  @param aAutoMount   if auto mounted by guest true, false otherwise
 *  @param fFailOnError Whether to fail with an error if the shared folder path is bad.
 *
 *  @return          COM result indicator
 */
HRESULT SharedFolder::init(Machine *aMachine,
                           const Utf8Str &aName,
                           const Utf8Str &aHostPath,
                           bool aWritable,
                           bool aAutoMount,
                           bool fFailOnError)
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mMachine) = aMachine;

    HRESULT rc = i_protectedInit(aMachine, aName, aHostPath, aWritable, aAutoMount, fFailOnError);

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}

/**
 *  Initializes the shared folder object given another object
 *  (a kind of copy constructor).  This object makes a private copy of data
 *  of the original object passed as an argument.
 *
 *  @param aMachine     parent Machine object
 *  @param aThat        shared folder object to copy
 *
 *  @return          COM result indicator
 */
HRESULT SharedFolder::initCopy(Machine *aMachine, SharedFolder *aThat)
{
    ComAssertRet(aThat, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mMachine) = aMachine;

    HRESULT rc = i_protectedInit(aMachine,
                                 aThat->m->strName,
                                 aThat->m->strHostPath,
                                 aThat->m->fWritable,
                                 aThat->m->fAutoMount,
                                 false /* fFailOnError */ );

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}

# if 0

/**
 *  Initializes the shared folder object.
 *
 *  This variant initializes a global instance that lives in the server address space. It is not presently used.
 *
 *  @param aVirtualBox  VirtualBox parent object
 *  @param aName        logical name of the shared folder
 *  @param aHostPath    full path to the shared folder on the host
 *  @param aWritable    writable if true, readonly otherwise
 *  @param fFailOnError Whether to fail with an error if the shared folder path is bad.
 *
 *  @return          COM result indicator
 */
HRESULT SharedFolder::init(VirtualBox *aVirtualBox,
                           const Utf8Str &aName,
                           const Utf8Str &aHostPath,
                           bool aWritable,
                           bool aAutoMount,
                           bool fFailOnError)
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mVirtualBox) = aVirtualBox;

    HRESULT rc = protectedInit(aVirtualBox, aName, aHostPath, aWritable, aAutoMount);

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}

# endif

#else

/**
 *  Initializes the shared folder object.
 *
 *  This variant initializes an instance that lives in the console address space.
 *
 *  @param aConsole     Console parent object
 *  @param aName        logical name of the shared folder
 *  @param aHostPath    full path to the shared folder on the host
 *  @param aWritable    writable if true, readonly otherwise
 *  @param fFailOnError Whether to fail with an error if the shared folder path is bad.
 *
 *  @return          COM result indicator
 */
HRESULT SharedFolder::init(Console *aConsole,
                           const Utf8Str &aName,
                           const Utf8Str &aHostPath,
                           bool aWritable,
                           bool aAutoMount,
                           bool fFailOnError)
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mConsole) = aConsole;

    HRESULT rc = i_protectedInit(aConsole, aName, aHostPath, aWritable, aAutoMount, fFailOnError);

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}
#endif

/**
 *  Shared initialization code. Called from the other constructors.
 *
 *  @note
 *      Must be called from under the object's lock!
 */
HRESULT SharedFolder::i_protectedInit(VirtualBoxBase *aParent,
                                      const Utf8Str &aName,
                                      const Utf8Str &aHostPath,
                                      bool aWritable,
                                      bool aAutoMount,
                                      bool fFailOnError)
{
    LogFlowThisFunc(("aName={%s}, aHostPath={%s}, aWritable={%d}, aAutoMount={%d}\n",
                      aName.c_str(), aHostPath.c_str(), aWritable, aAutoMount));

    ComAssertRet(aParent && aName.isNotEmpty() && aHostPath.isNotEmpty(), E_INVALIDARG);

    Utf8Str hostPath = aHostPath;
    size_t hostPathLen = hostPath.length();

    /* Remove the trailing slash unless it's a root directory
     * (otherwise the comparison with the RTPathAbs() result will fail at least
     * on Linux). Note that this isn't really necessary for the shared folder
     * itself, since adding a mapping eventually results into a
     * RTDirOpenFiltered() call (see HostServices/SharedFolders) that seems to
     * accept both the slashified paths and not. */
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
    if (hostPathLen > 2 &&
        RTPATH_IS_SEP (hostPath.c_str()[hostPathLen - 1]) &&
        RTPATH_IS_VOLSEP (hostPath.c_str()[hostPathLen - 2]))
        ;
#else
    if (hostPathLen == 1 && RTPATH_IS_SEP(hostPath[0]))
        ;
#endif
    else
        hostPath.stripTrailingSlash();

    if (fFailOnError)
    {
        /* Check whether the path is full (absolute) */
        char hostPathFull[RTPATH_MAX];
        int vrc = RTPathAbsEx(NULL,
                              hostPath.c_str(),
                              hostPathFull,
                              sizeof (hostPathFull));
        if (RT_FAILURE(vrc))
            return setError(E_INVALIDARG,
                            tr("Invalid shared folder path: '%s' (%Rrc)"),
                            hostPath.c_str(), vrc);

        if (RTPathCompare(hostPath.c_str(), hostPathFull) != 0)
            return setError(E_INVALIDARG,
                            tr("Shared folder path '%s' is not absolute"),
                            hostPath.c_str());
    }

    unconst(mParent) = aParent;

    unconst(m->strName) = aName;
    unconst(m->strHostPath) = hostPath;
    m->fWritable = aWritable;
    m->fAutoMount = aAutoMount;

    return S_OK;
}

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void SharedFolder::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    unconst(mParent) = NULL;

#if !defined(VBOX_COM_INPROC)
    unconst(mMachine) = NULL;
    unconst(mVirtualBox) = NULL;
#else
    unconst(mConsole) = NULL;
#endif
}

// wrapped ISharedFolder properties
/////////////////////////////////////////////////////////////////////////////
HRESULT SharedFolder::getName(com::Utf8Str &aName)
{
    /* mName is constant during life time, no need to lock */
    aName = m->strName;

    return S_OK;
}

HRESULT SharedFolder::getHostPath(com::Utf8Str &aHostPath)
{
    /* mHostPath is constant during life time, no need to lock */
    aHostPath = m->strHostPath;

    return S_OK;
}

HRESULT SharedFolder::getAccessible(BOOL *aAccessible)
{
    /* mName and mHostPath are constant during life time, no need to lock */

    /* check whether the host path exists */
    Utf8Str hostPath = m->strHostPath;
    char hostPathFull[RTPATH_MAX];
    int vrc = RTPathExists(hostPath.c_str()) ? RTPathReal(hostPath.c_str(),
                                                          hostPathFull,
                                                          sizeof(hostPathFull))
                                      : VERR_PATH_NOT_FOUND;
    if (RT_SUCCESS(vrc))
    {
        *aAccessible = TRUE;
        return S_OK;
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->strLastAccessError = Utf8StrFmt(tr("'%s' is not accessible (%Rrc)"),
                                       m->strHostPath.c_str(),
                                       vrc);

    Log1WarningThisFunc(("m.lastAccessError=\"%s\"\n", m->strLastAccessError.c_str()));

    *aAccessible = FALSE;

    return S_OK;
}

HRESULT SharedFolder::getWritable(BOOL *aWritable)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aWritable = !!m->fWritable;

    return S_OK;
}

HRESULT SharedFolder::getAutoMount(BOOL *aAutoMount)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAutoMount = !!m->fAutoMount;

    return S_OK;
}

HRESULT SharedFolder::getLastAccessError(com::Utf8Str &aLastAccessError)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aLastAccessError = m->strLastAccessError;

    return S_OK;
}


const Utf8Str& SharedFolder::i_getName() const
{
    return m->strName;
}

const Utf8Str& SharedFolder::i_getHostPath() const
{
    return m->strHostPath;
}

bool SharedFolder::i_isWritable() const
{
    return m->fWritable;
}

bool SharedFolder::i_isAutoMounted() const
{
    return m->fAutoMount;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
