/* $Id: VBoxMimicry.h $ */
/** @file
 * VBoxMimicry.h - Debug and logging routines implemented by VBoxDebugLib.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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


#ifndef __VBOXMIMICRY_H__
#define __VBOXMIMICRY_H__
#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#define MIMICRY_INTERFACE_COUNT 1
#define DO_9_FAKE_DECL(name)        \
static EFI_STATUS name ## _fake_impl0();  \
static EFI_STATUS name ## _fake_impl1();  \
static EFI_STATUS name ## _fake_impl2();  \
static EFI_STATUS name ## _fake_impl3();  \
static EFI_STATUS name ## _fake_impl4();  \
static EFI_STATUS name ## _fake_impl5();  \
static EFI_STATUS name ## _fake_impl6();  \
static EFI_STATUS name ## _fake_impl7();  \
static EFI_STATUS name ## _fake_impl8();  \
static EFI_STATUS name ## _fake_impl9();

#define FAKE_IMPL(name, guid)                                       \
static EFI_STATUS name ()                                           \
{                                                                   \
    DEBUG((DEBUG_INFO, #name ": of %g called\n", &guid));           \
    return EFI_SUCCESS;                                             \
}

typedef struct
{
    EFI_HANDLE hImage;
} VBOXMIMICRY, *PVBOXMIMICRY;

PVBOXMIMICRY gThis;

EFI_STATUS install_mimic_interfaces();
EFI_STATUS uninstall_mimic_interfaces();
#endif
