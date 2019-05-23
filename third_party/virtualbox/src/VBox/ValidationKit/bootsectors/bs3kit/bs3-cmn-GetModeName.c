/* $Id: bs3-cmn-GetModeName.c $ */
/** @file
 * BS3Kit - Bs3GetModeName
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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

#include "bs3kit-template-header.h"



#undef Bs3GetModeName
BS3_CMN_DEF(const char BS3_FAR *, Bs3GetModeName,(uint8_t bMode))
{
    switch (bMode)
    {
        case BS3_MODE_RM:           return g_szBs3ModeName_rm;
        case BS3_MODE_PE16:         return g_szBs3ModeName_pe16;
        case BS3_MODE_PE16_32:      return g_szBs3ModeName_pe16_32;
        case BS3_MODE_PE16_V86:     return g_szBs3ModeName_pe16_v86;
        case BS3_MODE_PE32:         return g_szBs3ModeName_pe32;
        case BS3_MODE_PE32_16:      return g_szBs3ModeName_pe32_16;
        case BS3_MODE_PEV86:        return g_szBs3ModeName_pev86;
        case BS3_MODE_PP16:         return g_szBs3ModeName_pp16;
        case BS3_MODE_PP16_32:      return g_szBs3ModeName_pp16_32;
        case BS3_MODE_PP16_V86:     return g_szBs3ModeName_pp16_v86;
        case BS3_MODE_PP32:         return g_szBs3ModeName_pp32;
        case BS3_MODE_PP32_16:      return g_szBs3ModeName_pp32_16;
        case BS3_MODE_PPV86:        return g_szBs3ModeName_ppv86;
        case BS3_MODE_PAE16:        return g_szBs3ModeName_pae16;
        case BS3_MODE_PAE16_32:     return g_szBs3ModeName_pae16_32;
        case BS3_MODE_PAE16_V86:    return g_szBs3ModeName_pae16_v86;
        case BS3_MODE_PAE32:        return g_szBs3ModeName_pae32;
        case BS3_MODE_PAE32_16:     return g_szBs3ModeName_pae32_16;
        case BS3_MODE_PAEV86:       return g_szBs3ModeName_paev86;
        case BS3_MODE_LM16:         return g_szBs3ModeName_lm16;
        case BS3_MODE_LM32:         return g_szBs3ModeName_lm32;
        case BS3_MODE_LM64:         return g_szBs3ModeName_lm64;
        case BS3_MODE_INVALID:      return "invalid";
        default:                    return "unknow";
    }
}

