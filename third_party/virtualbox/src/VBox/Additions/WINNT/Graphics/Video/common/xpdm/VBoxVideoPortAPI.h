/* $Id: VBoxVideoPortAPI.h $ */
/** @file
 * VBox video port functions header
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOXVIDEOPORTAPI_H
#define VBOXVIDEOPORTAPI_H

/* To maintain binary backward compatibility with older windows versions
 * we query at runtime for video port functions which are not present in NT 4.0
 * Those could used in the display driver also.
 */

/*Basic datatypes*/
typedef long VBOXVP_STATUS;
#ifndef VBOX_USING_W2K3DDK
typedef struct _ENG_EVENT *VBOXPEVENT;
#else
typedef struct _VIDEO_PORT_EVENT *VBOXPEVENT;
#endif
typedef struct _VIDEO_PORT_SPIN_LOCK *VBOXPSPIN_LOCK;
typedef union _LARGE_INTEGER *VBOXPLARGE_INTEGER;

typedef enum VBOXVP_POOL_TYPE
{
    VBoxVpNonPagedPool,
    VBoxVpPagedPool,
    VBoxVpNonPagedPoolCacheAligned = 4,
    VBoxVpPagedPoolCacheAligned
} VBOXVP_POOL_TYPE;

#define VBOXNOTIFICATION_EVENT 0x00000001UL
#define VBOXNO_ERROR           0x00000000UL

/*VideoPort API functions*/
typedef VBOXVP_STATUS (*PFNWAITFORSINGLEOBJECT) (void*  HwDeviceExtension, void*  Object, VBOXPLARGE_INTEGER  Timeout);
typedef long (*PFNSETEVENT) (void* HwDeviceExtension, VBOXPEVENT  pEvent);
typedef void (*PFNCLEAREVENT) (void*  HwDeviceExtension, VBOXPEVENT  pEvent);
typedef VBOXVP_STATUS (*PFNCREATEEVENT) (void*  HwDeviceExtension, unsigned long  EventFlag, void*  Unused, VBOXPEVENT  *ppEvent);
typedef VBOXVP_STATUS (*PFNDELETEEVENT) (void*  HwDeviceExtension, VBOXPEVENT  pEvent);
typedef void* (*PFNALLOCATEPOOL) (void*  HwDeviceExtension, VBOXVP_POOL_TYPE PoolType, size_t NumberOfBytes, unsigned long Tag);
typedef void (*PFNFREEPOOL) (void*  HwDeviceExtension, void*  Ptr);
typedef unsigned char (*PFNQUEUEDPC) (void* HwDeviceExtension, void (*CallbackRoutine)(void* HwDeviceExtension, void *Context), void *Context);
typedef VBOXVP_STATUS (*PFNCREATESECONDARYDISPLAY)(void* HwDeviceExtension, void* SecondaryDeviceExtension, unsigned long ulFlag);

/* pfn*Event and pfnWaitForSingleObject functions are available */
#define VBOXVIDEOPORTPROCS_EVENT    0x00000002
/* pfn*Pool functions are available */
#define VBOXVIDEOPORTPROCS_POOL     0x00000004
/* pfnQueueDpc function is available */
#define VBOXVIDEOPORTPROCS_DPC      0x00000008
/* pfnCreateSecondaryDisplay function is available */
#define VBOXVIDEOPORTPROCS_CSD      0x00000010

typedef struct VBOXVIDEOPORTPROCS
{
    /* ored VBOXVIDEOPORTPROCS_xxx constants describing the supported functionality */
    uint32_t fSupportedTypes;

    PFNWAITFORSINGLEOBJECT pfnWaitForSingleObject;

    PFNSETEVENT pfnSetEvent;
    PFNCLEAREVENT pfnClearEvent;
    PFNCREATEEVENT pfnCreateEvent;
    PFNDELETEEVENT pfnDeleteEvent;

    PFNALLOCATEPOOL pfnAllocatePool;
    PFNFREEPOOL pfnFreePool;

    PFNQUEUEDPC pfnQueueDpc;

    PFNCREATESECONDARYDISPLAY pfnCreateSecondaryDisplay;
} VBOXVIDEOPORTPROCS;

#endif /* !VBOXVIDEOPORTAPI_H */
