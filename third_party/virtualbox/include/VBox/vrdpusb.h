/** @file
 * VBox Remote Desktop Protocol - Remote USB backend interface. (VRDP)
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_vrdpusb_h
#define ___VBox_vrdpusb_h

#include <VBox/cdefs.h>
#include <VBox/types.h>

#ifdef IN_RING0
# error "There are no VRDP APIs available in Ring-0 Host Context!"
#endif
#ifdef IN_RC
# error "There are no VRDP APIs available Guest Context!"
#endif


/** @defgroup grp_vrdpusb   Remote USB over VURP
 * @ingroup grp_vusb
 * @{
 */

#define REMOTE_USB_BACKEND_PREFIX_S   "REMOTEUSB"
#define REMOTE_USB_BACKEND_PREFIX_LEN 9

/* Forward declaration. */
struct _REMOTEUSBDEVICE;
typedef struct _REMOTEUSBDEVICE *PREMOTEUSBDEVICE;

/* Forward declaration. */
struct _REMOTEUSBQURB;
typedef struct _REMOTEUSBQURB *PREMOTEUSBQURB;

/* Forward declaration. Actually a class. */
struct _REMOTEUSBBACKEND;
typedef struct _REMOTEUSBBACKEND *PREMOTEUSBBACKEND;

/**
 * Pointer to this structure is passed to pfnCreateProxyDevice as the device
 * specific pointer, when creating remote devices.
 */
typedef struct REMOTEUSBCALLBACK
{
    PREMOTEUSBBACKEND pInstance;

    DECLCALLBACKMEMBER(int, pfnOpen)             (PREMOTEUSBBACKEND pInstance, const char *pszAddress, size_t cbAddress, PREMOTEUSBDEVICE *ppDevice);
    DECLCALLBACKMEMBER(void, pfnClose)           (PREMOTEUSBDEVICE pDevice);
    DECLCALLBACKMEMBER(int, pfnReset)            (PREMOTEUSBDEVICE pDevice);
    DECLCALLBACKMEMBER(int, pfnSetConfig)        (PREMOTEUSBDEVICE pDevice, uint8_t u8Cfg);
    DECLCALLBACKMEMBER(int, pfnClaimInterface)   (PREMOTEUSBDEVICE pDevice, uint8_t u8Ifnum);
    DECLCALLBACKMEMBER(int, pfnReleaseInterface) (PREMOTEUSBDEVICE pDevice, uint8_t u8Ifnum);
    DECLCALLBACKMEMBER(int, pfnInterfaceSetting) (PREMOTEUSBDEVICE pDevice, uint8_t u8Ifnum, uint8_t u8Setting);
    DECLCALLBACKMEMBER(int, pfnQueueURB)         (PREMOTEUSBDEVICE pDevice, uint8_t u8Type, uint8_t u8Ep, uint8_t u8Direction, uint32_t u32Len, void *pvData, void *pvURB, PREMOTEUSBQURB *ppRemoteURB);
    DECLCALLBACKMEMBER(int, pfnReapURB)          (PREMOTEUSBDEVICE pDevice, uint32_t u32Millies, void **ppvURB, uint32_t *pu32Len, uint32_t *pu32Err);
    DECLCALLBACKMEMBER(int, pfnClearHaltedEP)    (PREMOTEUSBDEVICE pDevice, uint8_t u8Ep);
    DECLCALLBACKMEMBER(void, pfnCancelURB)       (PREMOTEUSBDEVICE pDevice, PREMOTEUSBQURB pRemoteURB);
    DECLCALLBACKMEMBER(int, pfnWakeup)           (PREMOTEUSBDEVICE pDevice);
} REMOTEUSBCALLBACK;

/** @} */

#endif

