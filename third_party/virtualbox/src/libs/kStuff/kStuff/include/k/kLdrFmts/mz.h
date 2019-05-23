/* $Id: mz.h 31 2009-07-01 21:08:06Z bird $ */
/** @file
 * MZ structures, types and defines.
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

#ifndef ___k_kLdrFmts_mz_h___
#define ___k_kLdrFmts_mz_h___

#include <k/kDefs.h>
#include <k/kTypes.h>

#pragma pack(1) /* not required */

typedef struct _IMAGE_DOS_HEADER
{
    KU16       e_magic;
    KU16       e_cblp;
    KU16       e_cp;
    KU16       e_crlc;
    KU16       e_cparhdr;
    KU16       e_minalloc;
    KU16       e_maxalloc;
    KU16       e_ss;
    KU16       e_sp;
    KU16       e_csum;
    KU16       e_ip;
    KU16       e_cs;
    KU16       e_lfarlc;
    KU16       e_ovno;
    KU16       e_res[4];
    KU16       e_oemid;
    KU16       e_oeminfo;
    KU16       e_res2[10];
    KU32       e_lfanew;
} IMAGE_DOS_HEADER;
typedef IMAGE_DOS_HEADER *PIMAGE_DOS_HEADER;

#ifndef IMAGE_DOS_SIGNATURE
# define IMAGE_DOS_SIGNATURE K_LE2H_U16('M' | ('Z' << 8))
#endif

#pragma pack()

#endif

