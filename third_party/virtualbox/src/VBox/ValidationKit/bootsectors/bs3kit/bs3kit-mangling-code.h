/* $Id: bs3kit-mangling-code.h $ */
/** @file
 * BS3Kit - Symbol mangling, code.
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


/*
 * Do function mangling.  This can be redone at compile time (templates).
 */
#undef BS3_CMN_MANGLER
#undef BS3_MODE_MANGLER
#if ARCH_BITS != 16 || !defined(BS3_USE_ALT_16BIT_TEXT_SEG)
# define BS3_CMN_MANGLER(a_Function)            BS3_CMN_NM(a_Function)
# define BS3_MODE_MANGLER(a_Function)           TMPL_NM(a_Function)
#else
# define BS3_CMN_MANGLER(a_Function)            BS3_CMN_FAR_NM(a_Function)
# define BS3_MODE_MANGLER(a_Function)           TMPL_FAR_NM(a_Function)
#endif
#include "bs3kit-mangling-code-undef.h"
#include "bs3kit-mangling-code-define.h"

