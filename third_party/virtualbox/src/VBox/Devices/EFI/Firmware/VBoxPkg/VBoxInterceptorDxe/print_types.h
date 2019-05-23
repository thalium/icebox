/* $Id: print_types.h $ */
/** @file
 * print_types.h - helper macrodifinition to convert types to right printf flags.
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
#ifndef PRINT_TYPES_H
#define PRINT_TYPES_H
#define PRNT_EFI_TPL PRNT_UINTN
#define PRNT_EFI_ALLOCATE_TYPE PRNT_UINTN
#define PRNT_EFI_MEMORY_TYPE PRNT_UINTN
#define PRNT_EFI_TIMER_DELAY PRNT_UINTN
#define PRNT_EFI_EVENT_NOTIFY  PRNT_P_VOID
#define PRNT_EFI_EVENT PRNT_P_VOID
#define PRNT_EFI_INTERFACE_TYPE PRNT_UINTN
#define PRNT_EFI_LOCATE_SEARCH_TYPE  PRNT_UINTN
#define PRNT_PP_EFI_CAPSULE_HEADER PRNT_P_VOID
#define PRNT_P_EFI_RESET_TYPE PRNT_P_UINTN
#define PRNT_EFI_RESET_TYPE PRNT_P_UINTN

#define PRNT_EFI_HANDLE PRNT_P_VOID
#define PRNT_P_EFI_HANDLE PRNT_P_VOID
#define PRNT_PP_EFI_HANDLE PRNT_P_VOID

#define PRNT_PP_EFI_DEVICE_PATH_PROTOCOL  PRNT_P_EFI_DEVICE_PATH_PROTOCOL
#define PRNT_P_EFI_DEVICE_PATH_PROTOCOL  PRNT_P_VOID
#define PRNT_PP_CHAR16 PRNT_P_VOID

/* Pointers */
#define PRNT_P_EFI_PHYSICAL_ADDRESS PRNT_P_UINT64
#define PRNT_P_EFI_MEMORY_DESCRIPTOR PRNT_P_VOID
#define PRNT_PP_VOID PRNT_P_VOID
#define PRNT_P_EFI_EVENT PRNT_P_VOID
#define PRNT_P_UINTN PRNT_P_VOID
#define PRNT_PP_EFI_OPEN_PROTOCOL_INFORMATION_ENTRY PRNT_P_VOID
#define PRNT_EFI_PHYSICAL_ADDRESS PRNT_UINT64
#define PRNT_P_EFI_TIME PRNT_P_VOID
#define PRNT_P_EFI_TIME_CAPABILITIES PRNT_P_VOID

#define PRNT_P_VOID "%p"
#define PRNT_P_CHAR16 "%s"
#define PRNT_P_CHAR8 PRNT_P_VOID
#define PRNT_P_UINT64 PRNT_P_VOID
#define PRNT_P_UINT32 PRNT_P_VOID
#define PRNT_P_UINTN PRNT_P_VOID
#define PRNT_P_UINT8 PRNT_P_VOID
#define PRNT_PP_EFI_GUID PRNT_P_VOID
#define PRNT_PPP_EFI_GUID PRNT_P_VOID
#define PRNT_P_EFI_GUID "%g"
/* this is machine dependednt */
#define PRNT_UINT64 "%llx"
#define PRNT_UINT32 "%x"
#define PRNT_UINTN "%x"
#define PRNT_UINT16 "%hx"
#define PRNT_UINT8 "%c"
#define PRNT_EFI_STATUS "%r"
#define PRNT_BOOLEAN PRNT_UINT8
#define PRNT_CHAR8 PRNT_UINT8
#define PRNT_P_BOOLEAN PRNT_P_UINT8
#endif
