/* $Id: TokenImpl.h $ */

/** @file
 *
 * Token COM class implementations: MachineToken and MediumLockToken
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef TOKEN_IMPL_H_
#define TOKEN_IMPL_H_

#include "TokenWrap.h"
#include "MachineImpl.h"


/**
 * The MachineToken class automates cleanup of a SessionMachine object.
 */
class ATL_NO_VTABLE MachineToken :
    public TokenWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(MachineToken)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(const ComObjPtr<SessionMachine> &pSessionMachine);
    void uninit(bool fAbandon);

private:

    // wrapped IToken methods
    HRESULT abandon(AutoCaller &aAutoCaller);
    HRESULT dummy();

    // data
    struct Data
    {
        Data()
        {
        }

        ComObjPtr<SessionMachine> pSessionMachine;
    };

    Data m;
};


class Medium;

/**
 * The MediumLockToken class automates cleanup of a Medium lock.
 */
class ATL_NO_VTABLE MediumLockToken :
    public TokenWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(MediumLockToken)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(const ComObjPtr<Medium> &pMedium, bool fWrite);
    void uninit();

private:

    // wrapped IToken methods
    HRESULT abandon(AutoCaller &aAutoCaller);
    HRESULT dummy();

    // data
    struct Data
    {
        Data() :
            fWrite(false)
        {
        }

        ComObjPtr<Medium> pMedium;
        bool fWrite;
    };

    Data m;
};


#endif // TOKEN_IMPL_H_

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
