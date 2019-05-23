/* $Id: SharedFolderImpl.h $ */
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

#ifndef ____H_SHAREDFOLDERIMPL
#define ____H_SHAREDFOLDERIMPL

#include "SharedFolderWrap.h"
#include <VBox/shflsvc.h>

class Console;

class ATL_NO_VTABLE SharedFolder :
    public SharedFolderWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR (SharedFolder)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aMachine, const com::Utf8Str &aName, const com::Utf8Str &aHostPath, bool aWritable, bool aAutoMount, bool fFailOnError);
    HRESULT initCopy(Machine *aMachine, SharedFolder *aThat);
    HRESULT init(Console *aConsole, const com::Utf8Str &aName, const com::Utf8Str &aHostPath, bool aWritable, bool aAutoMount, bool fFailOnError);
//     HRESULT init(VirtualBox *aVirtualBox, const Utf8Str &aName, const Utf8Str &aHostPath, bool aWritable, bool aAutoMount, bool fFailOnError);
    void uninit();

    // public methods for internal purposes only
    // (ensure there is a caller and a read lock before calling them!)

    /**
     * Public internal method. Returns the shared folder's name. Needs caller! Locking not necessary.
     * @return
     */
    const Utf8Str& i_getName() const;

    /**
     * Public internal method. Returns the shared folder's host path. Needs caller! Locking not necessary.
     * @return
     */
    const Utf8Str& i_getHostPath() const;

    /**
     * Public internal method. Returns true if the shared folder is writable. Needs caller and locking!
     * @return
     */
    bool i_isWritable() const;

    /**
     * Public internal method. Returns true if the shared folder is auto-mounted. Needs caller and locking!
     * @return
     */
    bool i_isAutoMounted() const;

protected:

    HRESULT i_protectedInit(VirtualBoxBase *aParent,
                            const Utf8Str &aName,
                            const Utf8Str &aHostPath,
                            bool aWritable,
                            bool aAutoMount,
                            bool fFailOnError);
private:

    // wrapped ISharedFolder properies.
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getHostPath(com::Utf8Str &aHostPath);
    HRESULT getAccessible(BOOL *aAccessible);
    HRESULT getWritable(BOOL *aWritable);
    HRESULT getAutoMount(BOOL *aAutoMount);
    HRESULT getLastAccessError(com::Utf8Str &aLastAccessError);

    VirtualBoxBase * const mParent;

    /* weak parents (only one of them is not null) */
#if !defined(VBOX_COM_INPROC)
    Machine        * const mMachine;
    VirtualBox     * const mVirtualBox;
#else
    Console        * const mConsole;
#endif

    struct Data;            // opaque data struct, defined in SharedFolderImpl.cpp
    Data *m;
};

#endif // ____H_SHAREDFOLDERIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
