/* $Id: kRdr.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kRdr - The File Provider.
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

#ifndef ___kRdr_h___
#define ___kRdr_h___

#include <k/kDefs.h>
#include <k/kTypes.h>

/** @defgroup grp_kRdr      kRdr - The File Provider
 * @{ */

/** @def KRDR_DECL
 * Declares a kRdr function according to build context.
 * @param type          The return type.
 */
#if defined(KRDR_BUILDING_DYNAMIC)
# define KRDR_DECL(type)    K_DECL_EXPORT(type)
#elif defined(KRDR_BUILT_DYNAMIC)
# define KRDR_DECL(type)    K_DECL_IMPORT(type)
#else
# define KRDR_DECL(type)    type
#endif

#ifdef __cplusplus
extern "C" {
#endif

KRDR_DECL(int)      kRdrOpen(   PPKRDR ppRdr, const char *pszFilename);
KRDR_DECL(int)      kRdrClose(    PKRDR pRdr);
KRDR_DECL(int)      kRdrRead(     PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off);
KRDR_DECL(int)      kRdrAllMap(   PKRDR pRdr, const void **ppvBits);
KRDR_DECL(int)      kRdrAllUnmap( PKRDR pRdr, const void *pvBits);
KRDR_DECL(KFOFF)    kRdrSize(     PKRDR pRdr);
KRDR_DECL(KFOFF)    kRdrTell(     PKRDR pRdr);
KRDR_DECL(const char *) kRdrName( PKRDR pRdr);
KRDR_DECL(KIPTR)    kRdrNativeFH( PKRDR pRdr);
KRDR_DECL(KSIZE)    kRdrPageSize( PKRDR pRdr);
KRDR_DECL(int)      kRdrMap(      PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
KRDR_DECL(int)      kRdrRefresh(  PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
KRDR_DECL(int)      kRdrProtect(  PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
KRDR_DECL(int)      kRdrUnmap(    PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
KRDR_DECL(void)     kRdrDone(     PKRDR pRdr);

KRDR_DECL(int)      kRdrBufOpen(PPKRDR ppRdr, const char *pszFilename);
KRDR_DECL(int)      kRdrBufWrap(PPKRDR ppRdr, PKRDR pRdr, KBOOL fCloseIt);
KRDR_DECL(KBOOL)    kRdrBufIsBuffered(PKRDR pRdr);
KRDR_DECL(int)      kRdrBufLine(PKRDR pRdr, char *pszLine, KSIZE cbLine);
KRDR_DECL(int)      kRdrBufLineEx(PKRDR pRdr, char *pszLine, KSIZE *pcbLine);
KRDR_DECL(const char *) kRdrBufLineQ(PKRDR pRdr);

#ifdef __cplusplus
}
#endif

/** @} */

#endif

