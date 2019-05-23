/* $Id: kRbU32.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRb - Red-Black Tree Implementation, KU32 keys.
 */

/*
 * Copyright (c) 2006-2009 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___k_kRbU32_h___
#define ___k_kRbU32_h___

typedef struct KRBU32
{
    KU32                mKey;
    KBOOL               mfRed;
    struct KRBU32      *mpLeft;
    struct KRBU32      *mpRight;
} KRBU32;
typedef KRBU32  *PRBU32;
typedef KRBU32 **PPRBU32;

/*#define KRB_EQUAL_ALLOWED*/
#define KRB_CHECK_FOR_EQUAL_INSERT
/*#define KRB_RANGE */
/*#define KRB_OFFSET */
#define KRB_MAX_STACK           48
#define KRB_STD_KEY_COMP
#define KRBKEY                  KU32
#define KRBNODE                 KRBU32
#define KRB_FN(name)            kRbU32 ## name
#define KRB_TYPE(prefix,name)   prefix ## KRBU32 ## name
#define KRB_INT(name)           KRBU32INT ## name
#define KRB_DECL(rettype)       K_DECL_INLINE(rettype)

#include <k/kRbTmpl/kRbBase.h>
#include <k/kRbTmpl/kRbDoWithAll.h>
#include <k/kRbTmpl/kRbEnum.h>
#include <k/kRbTmpl/kRbGet.h>
#include <k/kRbTmpl/kRbGetBestFit.h>
#include <k/kRbTmpl/kRbGetWithParent.h>
#include <k/kRbTmpl/kRbRemove2.h>
#include <k/kRbTmpl/kRbRemoveBestFit.h>
#include <k/kRbTmpl/kRbUndef.h>

#endif

