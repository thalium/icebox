/* $Id: MediumFormatImpl.h $ */

/** @file
 *
 * MediumFormat COM class implementation
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MEDIUMFORMAT_IMPL_H_
#define MEDIUMFORMAT_IMPL_H_

#include "MediumFormatWrap.h"


struct VDBACKENDINFO;

/**
 * The MediumFormat class represents the backend used to store medium data
 * (IMediumFormat interface).
 *
 * @note Instances of this class are permanently caller-referenced by Medium
 * objects (through addCaller()) so that an attempt to uninitialize or delete
 * them before all Medium objects are uninitialized will produce an endless
 * wait!
 */
class ATL_NO_VTABLE MediumFormat :
    public MediumFormatWrap
{
public:

    struct Property
    {
        Utf8Str     strName;
        Utf8Str     strDescription;
        DataType_T  type;
        ULONG       flags;
        Utf8Str     strDefaultValue;
    };

    typedef std::vector<Property> PropertyArray;
    typedef std::vector<com::Utf8Str> StrArray;

    DECLARE_EMPTY_CTOR_DTOR(MediumFormat)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(const VDBACKENDINFO *aVDInfo);
    void uninit();

    // public methods for internal purposes only
    // (ensure there is a caller and a read lock before calling them!)

    /** Const, no need to lock */
    const Utf8Str &i_getId() const { return m.strId; }
    /** Const, no need to lock */
    const StrArray &i_getFileExtensions() const { return m.maFileExtensions; }
    /** Const, no need to lock */
    MediumFormatCapabilities_T i_getCapabilities() const { return m.capabilities; }
    /** Const, no need to lock */
    const PropertyArray &i_getProperties() const { return m.maProperties; }

private:

    // wrapped IMediumFormat properties
    HRESULT getId(com::Utf8Str &aId);
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getCapabilities(std::vector<MediumFormatCapabilities_T> &aCapabilities);

    // wrapped IMediumFormat methods
    HRESULT describeFileExtensions(std::vector<com::Utf8Str> &aExtensions,
                                   std::vector<DeviceType_T> &aTypes);
    HRESULT describeProperties(std::vector<com::Utf8Str> &aNames,
                               std::vector<com::Utf8Str> &aDescriptions,
                               std::vector<DataType_T> &aTypes,
                               std::vector<ULONG> &aFlags,
                               std::vector<com::Utf8Str> &aDefaults);

    // types
    typedef std::vector<DeviceType_T> DeviceTypeArray;

    // data
    struct Data
    {
        Data() : capabilities((MediumFormatCapabilities_T)0) {}

        const Utf8Str        strId;
        const Utf8Str        strName;
        const StrArray       maFileExtensions;
        const DeviceTypeArray maDeviceTypes;
        const MediumFormatCapabilities_T capabilities;
        const PropertyArray  maProperties;
    };

    Data m;
};

#endif // MEDIUMFORMAT_IMPL_H_

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
