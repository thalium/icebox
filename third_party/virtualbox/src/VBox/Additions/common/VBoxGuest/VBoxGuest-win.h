/* $Id: VBoxGuest-win.h $ */
/** @file
 * VBoxGuest - Windows specifics.
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

#ifndef ___VBoxGuest_win_h
#define ___VBoxGuest_win_h


#include <iprt/cdefs.h>

#include <iprt/nt/ntddk.h>

#include <iprt/spinlock.h>
#include <iprt/memobj.h>

#include <VBox/VMMDev.h>
#include <VBox/VBoxGuest.h>
#include "VBoxGuestInternal.h"



/** Possible device states for our state machine. */
typedef enum VGDRVNTDEVSTATE
{
    VGDRVNTDEVSTATE_STOPPED,
    VGDRVNTDEVSTATE_WORKING,
    VGDRVNTDEVSTATE_PENDINGSTOP,
    VGDRVNTDEVSTATE_PENDINGREMOVE,
    VGDRVNTDEVSTATE_SURPRISEREMOVED,
    VGDRVNTDEVSTATE_REMOVED
} VGDRVNTDEVSTATE;

typedef struct VBOXGUESTWINBASEADDRESS
{
    /** Original device physical address. */
    PHYSICAL_ADDRESS RangeStart;
    /** Length of I/O or memory range. */
    ULONG   RangeLength;
    /** Flag: Unmapped range is I/O or memory range. */
    BOOLEAN RangeInMemory;
    /** Mapped I/O or memory range. */
    PVOID   MappedRangeStart;
    /** Flag: mapped range is I/O or memory range. */
    BOOLEAN MappedRangeInMemory;
    /** Flag: resource is mapped (i.e. MmMapIoSpace called). */
    BOOLEAN ResourceMapped;
} VBOXGUESTWINBASEADDRESS, *PVBOXGUESTWINBASEADDRESS;


/** Windows-specific device extension bits. */
typedef struct VBOXGUESTDEVEXTWIN
{
    VBOXGUESTDEVEXT Core;

    /** Our functional driver object. */
    PDEVICE_OBJECT pDeviceObject;
    /** Top of the stack. */
    PDEVICE_OBJECT pNextLowerDriver;
    /** Currently active Irp. */
    IRP *pCurrentIrp;
    /** Interrupt object pointer. */
    PKINTERRUPT pInterruptObject;

    /** Bus number where the device is located. */
    ULONG busNumber;
    /** Slot number where the device is located. */
    ULONG slotNumber;
    /** Device interrupt level. */
    ULONG interruptLevel;
    /** Device interrupt vector. */
    ULONG interruptVector;
    /** Affinity mask. */
    KAFFINITY interruptAffinity;
    /** LevelSensitive or Latched. */
    KINTERRUPT_MODE interruptMode;

    /** PCI base address information. */
    ULONG pciAddressCount;
    VBOXGUESTWINBASEADDRESS pciBaseAddress[PCI_TYPE0_ADDRESSES];

    /** Physical address and length of VMMDev memory. */
    PHYSICAL_ADDRESS vmmDevPhysMemoryAddress;
    ULONG vmmDevPhysMemoryLength;

    /** Device state. */
    VGDRVNTDEVSTATE enmDevState;
    /** The previous device state.   */
    VGDRVNTDEVSTATE enmPrevDevState;

    /** Last system power action set (see VBoxGuestPower). */
    POWER_ACTION LastSystemPowerAction;
    /** Preallocated generic request for shutdown. */
    VMMDevPowerStateRequest *pPowerStateRequest;
    /** Preallocated VMMDevEvents for IRQ handler. */
    VMMDevEvents *pIrqAckEvents;

    /** Pre-allocated kernel session data. This is needed
     * for handling kernel IOCtls. */
    struct VBOXGUESTSESSION *pKernelSession;

    /** Spinlock protecting MouseNotifyCallback. Required since the consumer is
     *  in a DPC callback and not the ISR. */
    KSPIN_LOCK MouseEventAccessLock;
} VBOXGUESTDEVEXTWIN, *PVBOXGUESTDEVEXTWIN;


/** NT (windows) version identifier. */
typedef enum VGDRVNTVER
{
    VGDRVNTVER_INVALID = 0,
    VGDRVNTVER_WINNT4,
    VGDRVNTVER_WIN2K,
    VGDRVNTVER_WINXP,
    VGDRVNTVER_WIN2K3,
    VGDRVNTVER_WINVISTA,
    VGDRVNTVER_WIN7,
    VGDRVNTVER_WIN8,
    VGDRVNTVER_WIN81,
    VGDRVNTVER_WIN10
} VGDRVNTVER;
extern VGDRVNTVER g_enmVGDrvNtVer;


#define VBOXGUEST_UPDATE_DEVSTATE(a_pDevExt, a_enmNewDevState) \
    do { \
        (a_pDevExt)->enmPrevDevState = (a_pDevExt)->enmDevState; \
        (a_pDevExt)->enmDevState     = (a_enmNewDevState); \
    } while (0)

/** CM_RESOURCE_MEMORY_* flags which were used on XP or earlier. */
#define VBOX_CM_PRE_VISTA_MASK (0x3f)


RT_C_DECLS_BEGIN

#ifdef TARGET_NT4
NTSTATUS   vgdrvNt4CreateDevice(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath);
#else
NTSTATUS   vgdrvNtPnP(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS   vgdrvNtPower(PDEVICE_OBJECT pDevObj, PIRP pIrp);
#endif

/** @name Common routines used in both (PnP and legacy) driver versions.
 * @{
 */
#ifdef TARGET_NT4
NTSTATUS   vgdrvNtInit(PDRIVER_OBJECT pDrvObj, PDEVICE_OBJECT pDevObj, PUNICODE_STRING pRegPath);
#else
NTSTATUS   vgdrvNtInit(PDEVICE_OBJECT pDevObj, PIRP pIrp);
#endif
NTSTATUS   vgdrvNtCleanup(PDEVICE_OBJECT pDevObj);
void       vgdrvNtUnmapVMMDevMemory(PVBOXGUESTDEVEXTWIN pDevExt); /**< @todo make static once buggy vgdrvNtInit is fixed. */
VBOXOSTYPE vgdrvNtVersionToOSType(VGDRVNTVER enmNtVer);
/** @}  */

RT_C_DECLS_END

#ifdef TARGET_NT4
/*
 * XP DDK #defines ExFreePool to ExFreePoolWithTag. The latter does not exist
 * on NT4, so... The same for ExAllocatePool.
 */
# undef ExAllocatePool
# undef ExFreePool
#endif

#endif /* !___VBoxGuest_win_h */

