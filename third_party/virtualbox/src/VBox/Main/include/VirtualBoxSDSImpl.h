/* $Id: VirtualBoxSDSImpl.h $ */
/** @file
 * VBox Global COM Class definition
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
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

#include "VirtualBoxBase.h"


class VBoxSDSPerUserData; /* See VirtualBoxSDSImpl.cpp. */

/**
 * The IVirtualBoxSDS implementation.
 *
 * This class helps different VBoxSVC processes make sure a user only have a
 * single VirtualBox instance.
 *
 * @note This is a simple internal class living in a privileged process.  So, we
 *       do not use the API wrappers as they add complexity.  In particular,
 *       they add the auto caller logic, which is an excellent tool to create
 *       unkillable processes.  If an API method during development or product
 *       for instance triggers an NT exception like STATUS_ACCESS_VIOLATION, the
 *       caller will be unwound without releasing the caller.  When uninit is
 *       called during COM shutdown/whatever, the thread gets stuck waiting for
 *       the long gone caller and cannot be killed (Windows 10, build 16299),
 *       requiring a reboot to continue.
 *
 * @todo Would be very nice to get rid of the ATL cruft too here.
 */
class VirtualBoxSDS
    : public IVirtualBoxSDS
    , public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>
    , public ATL::CComCoClass<VirtualBoxSDS, &CLSID_VirtualBoxSDS>
{
private:
    typedef std::map<com::Utf8Str, VBoxSDSPerUserData *> UserDataMap_T;
    /** Per user data map (key is SID string).
     * This is an insert-only map! */
    UserDataMap_T       m_UserDataMap;
    /** Lock protecting m_UserDataMap.*/
    RTCRITSECTRW        m_MapCritSect;

public:
    DECLARE_CLASSFACTORY_SINGLETON(VirtualBoxSDS)
    DECLARE_NOT_AGGREGATABLE(VirtualBoxSDS)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(VirtualBoxSDS)
        COM_INTERFACE_ENTRY(IVirtualBoxSDS)
    END_COM_MAP()

    DECLARE_EMPTY_CTOR_DTOR(VirtualBoxSDS)

    HRESULT FinalConstruct();
    void    FinalRelease();

private:

    // IVirtualBoxSDS methods
    HRESULT RegisterVBoxSVC(IVBoxSVCRegistration *aVBoxSVC, LONG aPid, IUnknown **aExistingVirtualBox);
    HRESULT DeregisterVBoxSVC(IVBoxSVCRegistration *aVBoxSVC, LONG aPid);


    // Private methods

    /**
     * Gets the client user SID of the
     */
    static bool i_getClientUserSid(com::Utf8Str *a_pStrSid, com::Utf8Str *a_pStrUsername);

    /**
     * Looks up the given user.
     *
     * @returns Pointer to the LOCKED per user data.  NULL if not found.
     * @param   a_rStrUserSid   The user SID.
     */
    VBoxSDSPerUserData *i_lookupPerUserData(com::Utf8Str const &a_rStrUserSid);

    /**
     * Looks up the given user, creating it if not found
     *
     * @returns Pointer to the LOCKED per user data.  NULL on allocation error.
     * @param   a_rStrUserSid   The user SID.
     * @param   a_rStrUsername  The user name if available.
     */
    VBoxSDSPerUserData *i_lookupOrCreatePerUserData(com::Utf8Str const &a_rStrUserSid, com::Utf8Str const &a_rStrUsername);
};


#endif // !____H_VIRTUALBOXSDSIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
