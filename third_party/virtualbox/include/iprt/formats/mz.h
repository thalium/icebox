/* $Id: mz.h $ */
/** @file
 * IPRT, MZ Executable Header.
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

#ifndef ___iprt_formats_mz_h
#define ___iprt_formats_mz_h

#include <iprt/types.h>
#include <iprt/assertcompile.h>

typedef struct _IMAGE_DOS_HEADER
{
    uint16_t   e_magic;
    uint16_t   e_cblp;
    uint16_t   e_cp;
    uint16_t   e_crlc;
    uint16_t   e_cparhdr;
    uint16_t   e_minalloc;
    uint16_t   e_maxalloc;
    uint16_t   e_ss;
    uint16_t   e_sp;
    uint16_t   e_csum;
    uint16_t   e_ip;
    uint16_t   e_cs;
    uint16_t   e_lfarlc;
    uint16_t   e_ovno;
    uint16_t   e_res[4];
    uint16_t   e_oemid;
    uint16_t   e_oeminfo;
    uint16_t   e_res2[10];
    uint32_t   e_lfanew;
} IMAGE_DOS_HEADER;
AssertCompileSize(IMAGE_DOS_HEADER, 0x40);
typedef IMAGE_DOS_HEADER *PIMAGE_DOS_HEADER;

#ifndef IMAGE_DOS_SIGNATURE
# define IMAGE_DOS_SIGNATURE ('M' | ('Z' << 8)) /* fix endianness */
#endif


#endif

