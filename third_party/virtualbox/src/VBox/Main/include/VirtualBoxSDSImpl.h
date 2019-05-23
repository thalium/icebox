/* $Id: VirtualBoxSDSImpl.h $ */
/** @file
 * VBox Global COM Class definition
 */

/*
 * Copyright (C) 2015-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_VIRTUALBOXSDSIMPL
#define ____H_VIRTUALBOXSDSIMPL

#include "VirtualBoxSDSWrap.h"
#include "TokenWrap.h"

#ifdef RT_OS_WINDOWS
# include "win/resource.h"
#endif

 /**
 * The MediumLockToken class automates cleanup of a Medium lock.
 */
class ATL_NO_VTABLE VirtualBoxToken :
    public TokenWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(VirtualBoxToken)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(const std::wstring& wstrUserName);
    void uninit();

    /**
    * Fabrique method to create and initialize COM token object.
    *
    * @param aToken         Pointer to result Token COM object.
    * @param wstrUserName   User name of client for that we create the token
    */
    static HRESULT CreateToken(ComPtr<IToken>& aToken, const std::wstring& wstrUserName);

    // Trace COM reference counting in debug mode
#ifdef RT_STRICT
    virtual ULONG InternalAddRef();
    virtual ULONG InternalRelease();
#endif

private:

    // wrapped IToken methods
    HRESULT abandon(AutoCaller &aAutoCaller);
    HRESULT dummy();

    // data
    struct Data
    {
        std::wstring wstrUserName;
    };

    Data m;
};

class ATL_NO_VTABLE VirtualBoxSDS :
    public VirtualBoxSDSWrap
#ifdef RT_OS_WINDOWS
    , public ATL::CComCoClass<VirtualBoxSDS, &CLSID_VirtualBoxSDS>
#endif
{
public:

    DECLARE_CLASSFACTORY_SINGLETON(VirtualBoxSDS)

    //DECLARE_REGISTRY_RESOURCEID(IDR_VIRTUALBOX)

    // Kind of redundant (VirtualBoxSDSWrap declares itself not aggregatable
    // and CComCoClass<VirtualBoxSDS, &CLSID_VirtualBoxSDS> as aggregatable,
    // the former is the first inheritance), but the C multiple inheritance
    // rules and the class factory in VBoxSDS.cpp needs this to disambiguate.
    DECLARE_NOT_AGGREGATABLE(VirtualBoxSDS)

    DECLARE_EMPTY_CTOR_DTOR(VirtualBoxSDS)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializers/uninitializers for internal purposes only
    HRESULT init();
    void uninit();

private:

    // Wrapped IVirtualBoxSDS properties

    /**
    *    Returns the instance of VirtualBox for client.
    *    It impersonates client and returns his VirtualBox instance
    *    either from cahce or makes new one.
    */
    HRESULT getVirtualBox(ComPtr<IVirtualBox> &aVirtualBox, ComPtr<IToken> &aToken);

    // Wrapped IVirtualBoxSDS methods

    // Inner methods

    /**
    *  Gets the current user name of current thread
    */
    int GetCurrentUserName(std::wstring& wstrUserName);

    /**
    *  Prints current user name of this thread to the log
    *  @param prefix    string fragment that will be inserted at the beginning
    *                   of the logging line
    */
    void LogUserName(char *prefix);

    /**
    *  Thread that periodically checks items in cache and cleans obsolete items
    */
    static DWORD WINAPI CheckCacheThread(LPVOID);

    // data fields
    class VirtualBoxCache;
    static VirtualBoxCache m_cache;
    friend VirtualBoxToken;
};


#endif // !____H_VIRTUALBOXSDSIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
