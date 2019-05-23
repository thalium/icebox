/* $Id: avlrogcptr.cpp $ */
/** @file
 * IPRT - AVL tree, RTGCPTR, range, unique keys, offset pointers.
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

#ifndef NOFILEID
static const char szFileId[] = "Id: kAVLULInt.c,v 1.4 2003/02/13 02:02:38 bird Exp $";
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/*
 * AVL configuration.
 */
#define KAVL_FN(a)                  RTAvlroGCPtr##a
#define KAVL_MAX_STACK              27  /* Up to 2^24 nodes. */
#define KAVL_CHECK_FOR_EQUAL_INSERT 1   /* No duplicate keys! */
#define KAVLNODECORE                AVLROGCPTRNODECORE
#define PKAVLNODECORE               PAVLROGCPTRNODECORE
#define PPKAVLNODECORE              PPAVLROGCPTRNODECORE
#define KAVLKEY                     RTGCPTR
#define PKAVLKEY                    PRTGCPTR
#define KAVLENUMDATA                AVLROGCPTRENUMDATA
#define PKAVLENUMDATA               PAVLROGCPTRENUMDATA
#define PKAVLCALLBACK               PAVLROGCPTRCALLBACK
#define KAVL_OFFSET                 1
#define KAVL_RANGE                  1


/*
 * AVL Compare macros
 */
#define KAVL_G( key1, key2)         ( (RTGCUINTPTR)(key1) >  (RTGCUINTPTR)(key2) )
#define KAVL_E( key1, key2)         ( (RTGCUINTPTR)(key1) == (RTGCUINTPTR)(key2) )
#define KAVL_NE(key1, key2)         ( (RTGCUINTPTR)(key1) != (RTGCUINTPTR)(key2) )
#define KAVL_R_IS_IDENTICAL(key1B, key2B, key1E, key2E)     ( (RTGCUINTPTR)(key1B) == (RTGCUINTPTR)(key2B) && (RTGCUINTPTR)(key1E) == (RTGCUINTPTR)(key2E) )
#define KAVL_R_IS_INTERSECTING(key1B, key2B, key1E, key2E)  ( (RTGCUINTPTR)(key1B) <= (RTGCUINTPTR)(key2E) && (RTGCUINTPTR)(key1E) >= (RTGCUINTPTR)(key2B) )
#define KAVL_R_IS_IN_RANGE(key1B, key1E, key2)              KAVL_R_IS_INTERSECTING(key1B, key2, key1E, key2)



/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/avl.h>
#include <iprt/assert.h>
#include <iprt/err.h>

/*
 * Include the code.
 */
#define SSToDS(ptr) ptr
#define KMAX        RT_MAX
#define kASSERT     Assert
#include "avl_Base.cpp.h"
#include "avl_Get.cpp.h"
#include "avl_Range.cpp.h"
#include "avl_DoWithAll.cpp.h"
#include "avl_Destroy.cpp.h"
#include "avl_GetBestFit.cpp.h"
#include "avl_Enum.cpp.h"

