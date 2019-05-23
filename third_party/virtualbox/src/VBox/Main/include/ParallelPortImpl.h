/* $Id: ParallelPortImpl.h $ */

/** @file
 * VirtualBox COM class implementation.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_PARALLELPORTIMPL
#define ____H_PARALLELPORTIMPL

#include "ParallelPortWrap.h"

namespace settings
{
    struct ParallelPort;
}

class ATL_NO_VTABLE ParallelPort :
    public ParallelPortWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(ParallelPort)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent, ULONG aSlot);
    HRESULT init(Machine *aParent, ParallelPort *aThat);
    HRESULT initCopy(Machine *parent, ParallelPort *aThat);
    void uninit();

    HRESULT i_loadSettings(const settings::ParallelPort &data);
    HRESULT i_saveSettings(settings::ParallelPort &data);

    // public methods only for internal purposes
    bool i_isModified();
    void i_rollback();
    void i_commit();
    void i_copyFrom(ParallelPort *aThat);
    void i_applyDefaults();
    bool i_hasDefaults();

private:

    // Wrapped IParallelPort properties
    HRESULT getEnabled(BOOL *aEnabled);
    HRESULT setEnabled(BOOL aEnabled);
    HRESULT getSlot(ULONG *aSlot);
    HRESULT getIRQ(ULONG *aIRQ);
    HRESULT setIRQ(ULONG aIRQ);
    HRESULT getIOBase(ULONG *aIOBase);
    HRESULT setIOBase(ULONG aIOBase);
    HRESULT getPath(com::Utf8Str &aPath);
    HRESULT setPath(const com::Utf8Str &aPath);

    struct Data;
    Data *m;
};

#endif // ____H_PARALLELPORTIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
