/* $Id: bs3-cmn-pic.h $ */
/** @file
 * BS3Kit - Internal PIC Defines, Variables and Functions.
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

#ifndef ___bs3_cmn_pic_h
#define ___bs3_cmn_pic_h

#include "bs3kit.h"


/** The master PIC port (base).   */
#define BS3_PIC_PORT_MASTER         UINT8_C(0x20)
/** The slave PIC port (base).   */
#define BS3_PIC_PORT_SLAVE          UINT8_C(0xa0)

/** The init command. */
#define BS3_PIC_CMD_INIT            UINT8_C(0x10)
/** 4th init step option for the init command.   */
#define BS3_PIC_CMD_INIT_F_4STEP    UINT8_C(0x01)

/** Auto end of interrupt flag for the 4th init step. */
#define BS3_PIC_I4_F_AUTO_EOI       UINT8_C(0x01)


RT_C_DECLS_BEGIN

extern bool g_fBs3PicConfigured;

RT_C_DECLS_END


#include "bs3kit-mangling-code.h"

#endif

