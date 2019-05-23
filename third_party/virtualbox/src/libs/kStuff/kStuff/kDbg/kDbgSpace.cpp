/* $Id: kDbgSpace.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, Address Space Manager.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "kDbgInternal.h"
#include <k/kHlpAlloc.h>
#include <k/kHlpString.h>
#include <k/kAvl.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Pointer to a name space module. */
typedef struct KDBGSPACEMOD *PKDBGSPACEMOD;

/**
 * Tracks a module segment in the address space.
 *
 * These segments are organized in two trees, by address in the
 * KDBGSPACE::pSegRoot tree and by selector value in the
 * KDBGSPACE::pSegSelRoot tree.
 *
 * While the debug module reader could easily provide us with
 * segment names and it could perhaps be interesting to lookup
 * a segment by its name in some situations, this has been
 * considered too much bother for now. :-)
 */
typedef struct KDBGSPACESEG
{
    /** The module segment index. */
    KI32                    iSegment;
    /** The address space module structure this segment belongs to. */
    PKDBGSPACEMOD           pSpaceMod;
} KDBGSPACESEG;
typedef KDBGSPACESEG *PKDBGSPACESEG;


/**
 * Track a module in the name space.
 *
 * Each module in the address space can be addressed efficiently
 * by module name. The module name has to be unique.
 */
typedef struct KDBGSPACEMOD
{
    /** The module name hash. */
    KU32                    u32Hash;
    /** The length of the module name. */
    KU32                    cchName;
    /** The module name. */
    char                   *pszName;
    /** The next module in the same bucket. */
    PKDBGSPACEMOD           pNext;
    /** Pointer to the debug module reader. */
    PKDBGMOD                pMod;
    /** The number of segments. */
    KU32                    cSegs;
    /** The segment array. (variable length) */
    KDBGSPACESEG            aSegs[1];
} KDBGSPACEMOD;


typedef struct KDBGCACHEDSYM *PKDBGCACHEDSYM;
/**
 * A cached symbol.
 */
typedef struct KDBGCACHEDSYM
{
    /** The symbol name hash. */
    KU32                    u32Hash;
    /** The next symbol in the same bucket. */
    PKDBGCACHEDSYM          pNext;
    /** The next symbol belonging to the same module as this. */
    PKDBGCACHEDSYM          pNextMod;
    /** The cached symbol information. */
    KDBGSYMBOL              Sym;
} KDBGCACHEDSYM;


/**
 * A symbol cache.
 */
typedef struct KDBGSYMCACHE
{
    /** The symbol cache magic. (KDBGSYMCACHE_MAGIC) */
    KU32                    u32Magic;
    /** The maximum number of symbols.*/
    KU32                    cMax;
    /** The current number of symbols.*/
    KU32                    cCur;
    /** The number of hash buckets. */
    KU32                    cBuckets;
    /** The address lookup tree. */
    PKDBGADDRAVL            pAddrTree;
    /** Array of hash buckets.
     * The size is selected according to the cache size. */
    PKDBGCACHEDSYM         *paBuckets[1];
} KDBGSYMCACHE;
typedef KDBGSYMCACHE *PKDBGSYMCACHE;


/**
 * A user symbol record.
 *
 * The user symbols are organized in the KDBGSPACE::pUserRoot tree
 * and form an overlay that overrides the debug info retrieved from
 * the KDBGSPACE::pSegRoot tree.
 *
 * In the current implementation the user symbols are unique and
 * one would have to first delete a symbol in order to add another
 * at the same address. This may be changed later, perhaps.
 */
typedef struct KDBGSPACEUSERSYM
{

} KDBGSPACEUSERSYM;
typedef KDBGSPACEUSERSYM *PKDBGSPACEUSERSYM;



/**
 * Address space.
 */
typedef struct KDBGSPACE
{
    /** The addresspace magic. (KDBGSPACE_MAGIC) */
    KU32            u32Magic;
    /** User defined address space identifier or data pointer. */
    KUPTR           uUser;
    /** The name of the address space. (Optional) */
    const char     *pszName;


} KDBGSPACE;
/** Pointer to an address space. */
typedef struct KDBGSPACE *PKDBGSPACE;
/** Pointer to an address space pointer. */
typedef PKDBGSPACE *PPKDBGSPACE;


KDBG_DECL(int) kDbgSpaceCreate(PPDBGSPACE ppSpace, KDBGADDR LowAddr, DBGADDR HighAddr,
                               KUPTR uUser, const char *pszName)
{
    /*
     * Validate input.
     */
    kDbgAssertPtrReturn(ppSpace);
    *ppSpace = NULL;
    kDbgAssertPtrNullReturn(pszName);
    kDbgAssertReturn(LowAddr < HighAddr);

    /*
     * Create and initialize the address space.
     */
    PKDBGSPACE pSpace = (PKDBGSPACE)kHlpAlloc(sizeof(*pSpace));
    if (!pSpace)
        return KERR_NO_MEMORY;
    pSpace->u32Magic = KDBGSPACE_MAGIC;
    pSpace->uUser = uUser;
    pSpace->pszName = pszName;

}
