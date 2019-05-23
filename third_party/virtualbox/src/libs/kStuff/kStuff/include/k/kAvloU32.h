/* $Id: kAvloU32.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kAvl - AVL Tree Implementation, KU32 keys, Offset Based.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

#ifndef ___k_kAvloU32_h___
#define ___k_kAvloU32_h___

typedef KI32 KAVLOU32PTR;

typedef struct KAVLOU32
{
    KU32                u32;
    KU8                 cFloorsToGo;
    KAVLOU32PTR         offLeft;
    KAVLOU32PTR         offRight;
} KAVLOU32, *PKAVLOU32, **PPKAVLOU32;

#define mKey                    u32
#define mHeight                 cFloorsToGo
#define mpLeft                  offLeft
#define mpRight                 offRight

/*#define KAVL_EQUAL_ALLOWED*/
#define KAVL_CHECK_FOR_EQUAL_INSERT
#define KAVL_MAX_STACK          32
/*#define KAVL_RANGE */
#define KAVL_OFFSET
#define KAVL_STD_KEY_COMP
#define KAVLKEY                 KU32
#define KAVLTREEPTR             KAVLOU32PTR
#define KAVLNODE                KAVLOU32
#define KAVL_FN(name)           kAvloU32 ## name
#define KAVL_TYPE(prefix,name)  prefix ## KAVLOU32 ## name
#define KAVL_INT(name)          KAVLOU32INT ## name
#define KAVL_DECL(rettype)      K_DECL_INLINE(rettype)

#include <k/kAvlTmpl/kAvlBase.h>
#include <k/kAvlTmpl/kAvlDoWithAll.h>
#include <k/kAvlTmpl/kAvlEnum.h>
#include <k/kAvlTmpl/kAvlGet.h>
#include <k/kAvlTmpl/kAvlGetBestFit.h>
#include <k/kAvlTmpl/kAvlGetWithParent.h>
#include <k/kAvlTmpl/kAvlRemove2.h>
#include <k/kAvlTmpl/kAvlRemoveBestFit.h>
#include <k/kAvlTmpl/kAvlUndef.h>

#endif


