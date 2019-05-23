/* $Id: SerialPortImpl.h $ */

/** @file
 *
 * VirtualBox COM class implementation
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

#ifndef ____H_SERIALPORTIMPL
#define ____H_SERIALPORTIMPL

#include "SerialPortWrap.h"

class GuestOSType;

namespace settings
{
    struct SerialPort;
}

class ATL_NO_VTABLE SerialPort :
    public SerialPortWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(SerialPort)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent, ULONG aSlot);
    HRESULT init(Machine *aParent, SerialPort *aThat);
    HRESULT initCopy(Machine *parent, SerialPort *aThat);
    void uninit();

    // public methods only for internal purposes
    HRESULT i_loadSettings(const settings::SerialPort &data);
    HRESULT i_saveSettings(settings::SerialPort &data);

    bool i_isModified();
    void i_rollback();
    void i_commit();
    void i_copyFrom(SerialPort *aThat);

    void i_applyDefaults(GuestOSType *aOsType);
    bool i_hasDefaults();

    // public methods for internal purposes only
    // (ensure there is a caller and a read lock before calling them!)

private:

    HRESULT i_checkSetPath(const Utf8Str &str);

    // Wrapped ISerialPort properties
    HRESULT getEnabled(BOOL *aEnabled);
    HRESULT setEnabled(BOOL aEnabled);
    HRESULT getHostMode(PortMode_T *aHostMode);
    HRESULT setHostMode(PortMode_T aHostMode);
    HRESULT getSlot(ULONG *aSlot);
    HRESULT getIRQ(ULONG *aIRQ);
    HRESULT setIRQ(ULONG aIRQ);
    HRESULT getIOBase(ULONG *aIOBase);
    HRESULT setIOBase(ULONG aIOBase);
    HRESULT getServer(BOOL *aServer);
    HRESULT setServer(BOOL aServer);
    HRESULT getPath(com::Utf8Str &aPath);
    HRESULT setPath(const com::Utf8Str &aPath);

    struct Data;
    Data *m;
};

#endif // ____H_SERIALPORTIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
