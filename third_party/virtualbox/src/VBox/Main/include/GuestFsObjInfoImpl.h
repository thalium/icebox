/* $Id: GuestFsObjInfoImpl.h $ */
/** @file
 * VirtualBox Main - Guest file system object information implementation.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_GUESTFSOBJINFOIMPL
#define ____H_GUESTFSOBJINFOIMPL

#include "GuestFsObjInfoWrap.h"
#include "GuestCtrlImplPrivate.h"

class ATL_NO_VTABLE GuestFsObjInfo :
    public GuestFsObjInfoWrap
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_EMPTY_CTOR_DTOR(GuestFsObjInfo)

    int     init(const GuestFsObjData &objData);
    void    uninit(void);

    HRESULT FinalConstruct(void);
    void    FinalRelease(void);
    /** @}  */

private:

    /** Wrapped @name IGuestFsObjInfo properties.
     * @{ */
    HRESULT getAccessTime(LONG64 *aAccessTime);
    HRESULT getAllocatedSize(LONG64 *aAllocatedSize);
    HRESULT getBirthTime(LONG64 *aBirthTime);
    HRESULT getChangeTime(LONG64 *aChangeTime);
    HRESULT getDeviceNumber(ULONG *aDeviceNumber);
    HRESULT getFileAttributes(com::Utf8Str &aFileAttributes);
    HRESULT getGenerationId(ULONG *aGenerationId);
    HRESULT getGID(ULONG *aGID);
    HRESULT getGroupName(com::Utf8Str &aGroupName);
    HRESULT getHardLinks(ULONG *aHardLinks);
    HRESULT getModificationTime(LONG64 *aModificationTime);
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getNodeId(LONG64 *aNodeId);
    HRESULT getNodeIdDevice(ULONG *aNodeIdDevice);
    HRESULT getObjectSize(LONG64 *aObjectSize);
    HRESULT getType(FsObjType_T *aType);
    HRESULT getUID(ULONG *aUID);
    HRESULT getUserFlags(ULONG *aUserFlags);
    HRESULT getUserName(com::Utf8Str &aUserName);
    /** @}  */

    GuestFsObjData mData;
};

#endif /* !____H_GUESTFSOBJINFOIMPL */

