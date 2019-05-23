/* $Id: vboxsharedrc.h $ */
/** @file
 * VBox extension to Wine D3D - shared resource
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___vboxsharedrc_h___
#define ___vboxsharedrc_h___

#define VBOXSHRC_F_SHARED              0x00000001 /* shared rc */
#define VBOXSHRC_F_SHARED_OPENED       0x00000002 /* if set shared rc is opened, otherwise it is created */

#define VBOXSHRC_GET_SHAREFLAFS(_o) ((_o)->resource.sharerc_flags)
#define VBOXSHRC_GET_SHAREHANDLE(_o) ((HANDLE)(uintptr_t)(_o)->resource.sharerc_handle)
#define VBOXSHRC_SET_SHAREHANDLE(_o, _h) ((_o)->resource.sharerc_handle = (DWORD)(uintptr_t)(_h))
#define VBOXSHRC_COPY_SHAREDATA(_oDst, _oSrc) do { \
        VBOXSHRC_GET_SHAREFLAFS(_oDst) = VBOXSHRC_GET_SHAREFLAFS(_oSrc);   \
        VBOXSHRC_SET_SHAREHANDLE(_oDst, VBOXSHRC_GET_SHAREHANDLE(_oSrc)); \
    } while (0)
#define VBOXSHRC_SET_SHARED(_o) (VBOXSHRC_GET_SHAREFLAFS(_o) |= VBOXSHRC_F_SHARED)
#define VBOXSHRC_SET_SHARED_OPENED(_o) (VBOXSHRC_GET_SHAREFLAFS(_o) |= VBOXSHRC_F_SHARED_OPENED)

#define VBOXSHRC_IS_SHARED(_o) (!!(VBOXSHRC_GET_SHAREFLAFS(_o) & VBOXSHRC_F_SHARED))
#define VBOXSHRC_IS_SHARED_OPENED(_o) (!!(VBOXSHRC_GET_SHAREFLAFS(_o) & VBOXSHRC_F_SHARED_OPENED))
#define VBOXSHRC_IS_SHARED_UNLOCKED(_o) (VBOXSHRC_IS_SHARED(_o) && !VBOXSHRC_IS_LOCKED(_o))

#define VBOXSHRC_LOCK(_o) do{ \
        Assert(VBOXSHRC_IS_SHARED(_o)); \
        ++(_o)->resource.sharerc_locks; \
    } while (0)
#define VBOXSHRC_UNLOCK(_o) do{ \
        Assert(VBOXSHRC_IS_SHARED(_o)); \
        --(_o)->resource.sharerc_locks; \
        Assert((_o)->resource.sharerc_locks < UINT32_MAX/2); \
    } while (0)
#define VBOXSHRC_IS_LOCKED(_o) ( \
        !!((_o)->resource.sharerc_locks) \
        )

#endif /* #ifndef ___vboxsharedrc_h___ */
