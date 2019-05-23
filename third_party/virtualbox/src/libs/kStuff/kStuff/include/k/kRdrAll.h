/* $Id: kRdrAll.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kRdr - The File Provider, All Details and Dependencies Included.
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

#ifndef ___k_kRdrAll_h___
#define ___k_kRdrAll_h___

#include <k/kDefs.h>
#include <k/kLdr.h>
#include <k/kRdr.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup grp_kRdrAll   All
 * @addtogroup grp_kRdr
 * @{
 */

/**
 * File provider instance operations.
 */
typedef struct KRDROPS
{
    /** The name of this file provider. */
    const char *pszName;
    /** Pointer to the next file provider. */
    const struct KRDROPS *pNext;

    /** Try create a new file provider instance.
     *
     * @returns 0 on success, OS specific error code on failure.
     * @param   ppRdr       Where to store the file provider instance.
     * @param   pszFilename The filename to open.
     */
    int     (* pfnCreate)(  PPKRDR ppRdr, const char *pszFilename);
    /** Destroy the file provider instance.
     *
     * @returns 0 on success, OS specific error code on failure.
     *          On failure, the file provider instance will be in an indeterminate state - don't touch it!
     * @param   pRdr        The file provider instance.
     */
    int     (* pfnDestroy)( PKRDR pRdr);
    /** @copydoc kRdrRead */
    int     (* pfnRead)(    PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off);
    /** @copydoc kRdrAllMap */
    int     (* pfnAllMap)(  PKRDR pRdr, const void **ppvBits);
    /** @copydoc kRdrAllUnmap */
    int     (* pfnAllUnmap)(PKRDR pRdr, const void *pvBits);
    /** @copydoc kRdrSize */
    KFOFF   (* pfnSize)(    PKRDR pRdr);
    /** @copydoc kRdrTell */
    KFOFF   (* pfnTell)(    PKRDR pRdr);
    /** @copydoc kRdrName */
    const char * (* pfnName)(PKRDR pRdr);
    /** @copydoc kRdrNativeFH */
    KIPTR  (* pfnNativeFH)(PKRDR pRdr);
    /** @copydoc kRdrPageSize */
    KSIZE   (* pfnPageSize)(PKRDR pRdr);
    /** @copydoc kRdrMap */
    int     (* pfnMap)(     PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
    /** @copydoc kRdrRefresh */
    int     (* pfnRefresh)( PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
    /** @copydoc kRdrProtect */
    int     (* pfnProtect)( PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
    /** @copydoc kRdrUnmap */
    int     (* pfnUnmap)(   PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
    /** @copydoc kRdrDone */
    void    (* pfnDone)(    PKRDR pRdr);
    /** The usual non-zero dummy that makes sure we've initialized all members. */
    KU32    u32Dummy;
} KRDROPS;
/** Pointer to file provider operations. */
typedef KRDROPS *PKRDROPS;
/** Pointer to const file provider operations. */
typedef const KRDROPS *PCKRDROPS;


/**
 * File provider instance core.
 */
typedef struct KRDR
{
    /** Magic number (KRDR_MAGIC). */
    KU32        u32Magic;
    /** Pointer to the file provider operations. */
    PCKRDROPS   pOps;
} KRDR;

void    kRdrAddProvider(PKRDROPS pAdd);

/** @} */

#ifdef __cplusplus
}
#endif

#endif

