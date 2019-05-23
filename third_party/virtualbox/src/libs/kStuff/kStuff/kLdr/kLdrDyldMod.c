/* $Id: kLdrDyldMod.c 81 2016-08-18 22:10:38Z bird $ */
/** @file
 * kLdr - The Dynamic Loader, Dyld module methods.
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
#include <k/kLdr.h>
#include "kLdrInternal.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRDYLDMOD_STRICT
 * Define KLDRDYLDMOD_STRICT to enabled strict checks in kLdrDyld. */
#define KLDRDYLDMOD_STRICT 1

/** @def KLDRDYLDMOD_ASSERT
 * Assert that an expression is true when KLDRDYLD_STRICT is defined.
 */
#ifdef KLDRDYLDMOD_STRICT
# define KLDRDYLDMOD_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRDYLDMOD_ASSERT(expr)  do {} while (0)
#endif

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void kldrDyldModUnlink(PKLDRDYLDMOD pMod);



/**
 * Creates a module from the specified file provider instance.
 *
 * @returns 0 on success and *ppMod pointing to the new instance.
 *          On failure a non-zero kLdr status code is returned.
 * @param   pRdr    The file provider instance.
 * @param   fFlags  Load/search flags.
 * @param   ppMod   Where to put the pointer to the new module on success.
 */
int kldrDyldModCreate(PKRDR pRdr, KU32 fFlags, PPKLDRDYLDMOD ppMod)
{
    PKLDRDYLDMOD pMod;
    PKLDRMOD pRawMod;
    int rc;

    *ppMod = NULL;

/** @todo deal with fFlags (exec/dll) */
/** @todo Check up the cpu architecture. */

    /*
     * Try open an module interpreter.
     */
    rc = kLdrModOpenFromRdr(pRdr, 0 /*fFlags*/, KCPUARCH_UNKNOWN, &pRawMod);
    if (rc)
        return kldrDyldFailure(rc, "%s: %rc", kRdrName(pRdr), rc);

    /*
     * Match the module aginst the load flags.
     */
    switch (pRawMod->enmType)
    {
        case KLDRTYPE_EXECUTABLE_FIXED:
        case KLDRTYPE_EXECUTABLE_RELOCATABLE:
        case KLDRTYPE_EXECUTABLE_PIC:
            if (!(fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE))
            {
                kLdrModClose(pRawMod);
                return KLDR_ERR_NOT_EXE;
            }
            break;

        case KLDRTYPE_OBJECT: /* We can handle these as DLLs. */
        case KLDRTYPE_SHARED_LIBRARY_FIXED:
        case KLDRTYPE_SHARED_LIBRARY_RELOCATABLE:
        case KLDRTYPE_SHARED_LIBRARY_PIC:
        case KLDRTYPE_FORWARDER_DLL:
            if (fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE)
            {
                kLdrModClose(pRawMod);
                return KLDR_ERR_NOT_DLL;
            }
            break;

        default:
            KLDRDYLDMOD_ASSERT(!"Bad enmType!");
        case KLDRTYPE_CORE:
            return fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE ? KLDR_ERR_NOT_EXE : KLDR_ERR_NOT_DLL;
    }

    /*
     * Allocate a new dyld module.
     */
    pMod = (PKLDRDYLDMOD)kHlpAlloc(sizeof(*pMod));
    if (pMod)
    {
        pMod->enmState = KLDRSTATE_OPEN;
        pMod->pMod = pRawMod;
        pMod->hMod = pMod;
        pMod->cDepRefs = pMod->cDynRefs = pMod->cRefs = 0;
        switch (pRawMod->enmType)
        {
            case KLDRTYPE_EXECUTABLE_FIXED:
            case KLDRTYPE_EXECUTABLE_RELOCATABLE:
            case KLDRTYPE_EXECUTABLE_PIC:
                pMod->fExecutable = 1;
                break;
            default:
                pMod->fExecutable = 0;
                break;
        }
        pMod->fGlobalOrSpecific = 0;
        pMod->fBindable = 0;
        pMod->fInitList = 0;
        pMod->fAlreadySeen = 0;
        pMod->fMapped = 0;
        pMod->fAllocatedTLS = 0;
        pMod->f25Reserved = 0;
        pMod->InitTerm.pNext = NULL;
        pMod->InitTerm.pPrev = NULL;
        pMod->Bind.pNext = NULL;
        pMod->Bind.pPrev = NULL;
        pMod->cPrereqs = 0;
        pMod->papPrereqs = NULL;
        pMod->u32MagicHead = KLDRDYMOD_MAGIC;
        pMod->u32MagicTail = KLDRDYMOD_MAGIC;

        /* it. */
        pMod->Load.pNext = NULL;
        pMod->Load.pPrev = kLdrDyldTail;
        if (kLdrDyldTail)
            kLdrDyldTail->Load.pNext = pMod;
        else
            kLdrDyldHead = pMod;
        kLdrDyldTail = pMod;

        /* deal with the remaining flags. */
        if (fFlags & KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE)
            kldrDyldModMarkSpecific(pMod);
        else
            kldrDyldModMarkGlobal(pMod);

        if (fFlags & KLDRYDLD_LOAD_FLAGS_GLOBAL_SYMBOLS)
            kldrDyldModSetBindable(pMod, 0 /* not deep binable */);
        else
            kldrDyldModClearBindable(pMod);

        /*
         * We're good.
         */
        *ppMod = pMod;
        rc = 0;
    }
    else
    {
        kLdrModClose(pRawMod);
        rc = KERR_NO_MEMORY;
    }
    return rc;
}


/**
 * Creates a module for a native module.
 *
 * @returns 0 on success and *ppMod pointing to the new instance.
 *          On failure a non-zero kLdr status code is returned.
 * @param   hNativeModule   The native handle.
 * @param   ppMod           Where to put the pointer to the new module on success.
 * @remark  This function ain't finalized yet.
 */
int kldrDyldModCreateNative(KUPTR hNativeModule)
{
#if 0
    /*
     * Check if this module is already loaded by the native OS loader.
     */
    rc = kld
    {
#if K_OS == K_OS_OS2
    HMODULE hmod = NULLHANDLE;
    APIRET rc = DosQueryModuleHandle(kRdrName(pRdr), &hmod);
    if (!rc)

#elif K_OS == K_OS_WINDOWS
    HMODULE hmod = NULL;
    if (GetModuleHandle(kRdrName(pRdr))

#else
# error "Port me"
#endif
    }
#endif
    return -1;
}


/**
 * Destroys a module pending destruction.
 *
 * @param   pMod        The module in question.
 */
void kldrDyldModDestroy(PKLDRDYLDMOD pMod)
{
    int rc;

    /*
     * Validate the state.
     */
    switch (pMod->enmState)
    {
        case KLDRSTATE_PENDING_DESTROY:
        case KLDRSTATE_GC:
            break;
        default:
            KLDRDYLDMOD_ASSERT(!"Invalid state");
            break;
    }
    KLDRDYLDMOD_ASSERT(!pMod->fInitList);
    KLDRDYLDMOD_ASSERT(!pMod->cDynRefs);
    KLDRDYLDMOD_ASSERT(!pMod->cDepRefs);

    /*
     * Ensure that the module is unmapped.
     */
    if (pMod->fAllocatedTLS)
    {
        kLdrModFreeTLS(pMod->pMod, KLDRMOD_INT_MAP);
        pMod->fAllocatedTLS = 0;
    }
    if (pMod->fMapped)
    {
        rc = kLdrModUnmap(pMod->pMod); KLDRDYLDMOD_ASSERT(!rc);
        pMod->fMapped = 0;
    }

    /*
     * Ensure it's unlinked from all chains.
     */
    if (pMod->enmState < KLDRSTATE_PENDING_DESTROY)
        kldrDyldModUnlink(pMod);

    /*
     * Free everything associated with the module.
     */
    /* the prerequisite array. */
    if (pMod->papPrereqs)
    {
        KU32 i = pMod->cPrereqs;
        while (i-- > 0)
        {
            KLDRDYLDMOD_ASSERT(pMod->papPrereqs[i] == NULL);
            pMod->papPrereqs[i] = NULL;
        }

        kHlpFree(pMod->papPrereqs);
        pMod->papPrereqs = NULL;
        pMod->cPrereqs = 0;
    }

    /* the module interpreter.  */
    if (pMod->pMod)
    {
        rc = kLdrModClose(pMod->pMod); KLDRDYLDMOD_ASSERT(!rc);
        pMod->pMod = NULL;
    }


    /*
     * Finally, change the module state and free the module if
     * there are not more references to it. If somebody is still
     * referencing it, postpone the freeing to Deref.
     */
    pMod->enmState = KLDRSTATE_DESTROYED;
    if (!pMod->cRefs)
    {
        pMod->u32MagicHead = 1;
        pMod->u32MagicTail = 2;
        kHlpFree(pMod);
    }
}


/**
 * Unlinks the module from any list it might be in.
 * It is assumed that the module is at least linked into the load list.
 *
 * @param   pMod    The moduel.
 */
static void kldrDyldModUnlink(PKLDRDYLDMOD pMod)
{
    /* load list */
    if (pMod->Load.pNext)
        pMod->Load.pNext->Load.pPrev = pMod->Load.pPrev;
    else
        kLdrDyldTail = pMod->Load.pPrev;
    if (pMod->Load.pPrev)
        pMod->Load.pPrev->Load.pNext = pMod->Load.pNext;
    else
        kLdrDyldHead = pMod->Load.pNext;

    /* bind list */
    if (pMod->fBindable)
        kldrDyldModClearBindable(pMod);

    /* init term */
    if (pMod->fInitList)
    {
        KLDRDYLDMOD_ASSERT(pMod->enmState < KLDRSTATE_INITIALIZATION_FAILED);
        pMod->fInitList = 0;
        if (pMod->InitTerm.pNext)
            pMod->InitTerm.pNext->InitTerm.pPrev = pMod->InitTerm.pPrev;
        else
            g_pkLdrDyldInitTail = pMod->InitTerm.pPrev;
        if (pMod->InitTerm.pPrev)
            pMod->InitTerm.pPrev->InitTerm.pNext = pMod->InitTerm.pNext;
        else
            g_pkLdrDyldInitHead = pMod->InitTerm.pNext;
    }
    else if (pMod->enmState > KLDRSTATE_INITIALIZATION_FAILED)
    {
        KLDRDYLDMOD_ASSERT(pMod->enmState >= KLDRSTATE_GOOD);
        if (pMod->InitTerm.pNext)
            pMod->InitTerm.pNext->InitTerm.pPrev = pMod->InitTerm.pPrev;
        else
            g_pkLdrDyldTermTail = pMod->InitTerm.pPrev;
        if (pMod->InitTerm.pPrev)
            pMod->InitTerm.pPrev->InitTerm.pNext = pMod->InitTerm.pNext;
        else
            g_pkLdrDyldTermHead = pMod->InitTerm.pNext;
    }
    pMod->InitTerm.pNext = NULL;
    pMod->InitTerm.pPrev = NULL;
}


/**
 * Marks a module as bindable, i.e. it'll be considered when
 * resolving names the unix way.
 *
 * @param   pMod    The module.
 * @param   fDeep   When set the module will be inserted at the head of the
 *                  module list used to resolve symbols. This means that the
 *                  symbols in this module will be prefered of all the other
 *                  modules.
 */
void kldrDyldModSetBindable(PKLDRDYLDMOD pMod, unsigned fDeep)
{
    KLDRDYLDMOD_ASSERT(pMod->enmState >= KLDRSTATE_OPEN && pMod->enmState < KLDRSTATE_PENDING_GC);
    if (!pMod->fBindable)
    {
        pMod->fBindable = 1;
        if (!fDeep)
        {
            pMod->Bind.pNext = NULL;
            pMod->Bind.pPrev = g_pkLdrDyldBindTail;
            if (g_pkLdrDyldBindTail)
                g_pkLdrDyldBindTail->Bind.pNext = pMod;
            else
                g_pkLdrDyldBindHead = pMod;
            g_pkLdrDyldBindTail = pMod;
        }
        else
        {
            pMod->Bind.pPrev = NULL;
            pMod->Bind.pNext = g_pkLdrDyldBindHead;
            if (g_pkLdrDyldBindHead)
                g_pkLdrDyldBindHead->Bind.pPrev = pMod;
            else
                g_pkLdrDyldBindTail = pMod;
            g_pkLdrDyldBindHead = pMod;
        }
    }
}


/**
 * Marks a module as not bindable, i.e. it will not be considered when
 * resolving names the unix way.
 *
 * @param   pMod    The module.
 */
void kldrDyldModClearBindable(PKLDRDYLDMOD pMod)
{
    KLDRDYLDMOD_ASSERT(pMod->enmState >= KLDRSTATE_OPEN && pMod->enmState < KLDRSTATE_PENDING_DESTROY);
    if (pMod->fBindable)
    {
        pMod->fBindable = 0;
        if (pMod->Bind.pPrev)
            pMod->Bind.pPrev->Bind.pNext = pMod->Bind.pNext;
        else
            g_pkLdrDyldBindHead = pMod->Bind.pNext;
        if (pMod->Bind.pNext)
            pMod->Bind.pNext->Bind.pPrev = pMod->Bind.pPrev;
        else
            g_pkLdrDyldBindTail = pMod->Bind.pPrev;
        pMod->Bind.pNext = NULL;
        pMod->Bind.pPrev = NULL;
    }
}


/**
 * Marks the module as global instead of being specific.
 *
 * A global module can be a matching result when the request
 * doesn't specify a path. A specific module will not match
 * unless the path also matches.
 *
 * @param   pMod    The module.
 */
void kldrDyldModMarkGlobal(PKLDRDYLDMOD pMod)
{
    pMod->fGlobalOrSpecific = 1;
}


/**
 * Marks the module as specific instead of global.
 *
 * See kldrDyldModMarkGlobal for an explanation of the two terms.
 *
 * @param   pMod    The module.
 */
void kldrDyldModMarkSpecific(PKLDRDYLDMOD pMod)
{
    pMod->fGlobalOrSpecific = 0;
}


/**
 * Adds a reference to the module making sure it won't be freed just yet.
 *
 * @param   pMod    The module.
 */
void kldrDyldModAddRef(PKLDRDYLDMOD pMod)
{
    pMod->cRefs++;
}


/**
 * Dereference a module.
 *
 * @param   pMod
 */
void kldrDyldModDeref(PKLDRDYLDMOD pMod)
{
    /* validate input */
    KLDRDYLDMOD_ASSERT(pMod->cRefs > 0);
    KLDRDYLDMOD_ASSERT(pMod->cRefs >= pMod->cDepRefs + pMod->cDynRefs);
    KLDRDYLDMOD_ASSERT(pMod->enmState > KLDRSTATE_INVALID && pMod->enmState <= KLDRSTATE_END);

    /* decrement. */
    if (pMod->cRefs > 0)
        pMod->cRefs--;

    /* execute delayed freeing. */
    if (    pMod->enmState == KLDRSTATE_DESTROYED
        &&  !pMod->cRefs)
    {
        pMod->u32MagicHead = 1;
        pMod->u32MagicTail = 2;
        kHlpFree(pMod);
    }
}


/**
 * Increment the count of modules depending on this module.
 *
 * @param   pMod    The module.
 * @param   pDep    The module which depends on us.
 */
void kldrDyldModAddDep(PKLDRDYLDMOD pMod, PKLDRDYLDMOD pDep)
{
    (void)pDep;

    /* validate state */
    switch (pMod->enmState)
    {
        case KLDRSTATE_MAPPED:
        case KLDRSTATE_RELOADED:
        case KLDRSTATE_LOADED_PREREQUISITES:
        case KLDRSTATE_RELOADED_LOADED_PREREQUISITES:
        case KLDRSTATE_PENDING_INITIALIZATION:
        case KLDRSTATE_INITIALIZING:
        case KLDRSTATE_GOOD:
            break;
        default:
            KLDRDYLDMOD_ASSERT(!"invalid state");
            break;

    }
    KLDRDYLDMOD_ASSERT(pMod->enmState > KLDRSTATE_INVALID && pMod->enmState <= KLDRSTATE_END);
    pMod->cRefs++;
    pMod->cDepRefs++;
}


/**
 * Drop a dependency.
 *
 * @param   pMod    The module.
 * @param   pDep    The module which depends on us.
 */
void kldrDyldModRemoveDep(PKLDRDYLDMOD pMod, PKLDRDYLDMOD pDep)
{
    KLDRDYLDMOD_ASSERT(pMod->cDepRefs > 0);
    if (pMod->cDepRefs == 0)
        return;
    KLDRDYLDMOD_ASSERT(pMod->cDepRefs <= pMod->cRefs);
    KLDRDYLDMOD_ASSERT(pMod->enmState >= KLDRSTATE_MAPPED && pMod->enmState <= KLDRSTATE_PENDING_DESTROY);

    pMod->cRefs--;
    pMod->cDepRefs--;
    if (    pMod->cDepRefs > 0
        ||  pMod->cDynRefs > 0)
        return;

    /*
     * The module should be unloaded.
     */
    kldrDyldModUnloadPrerequisites(pMod);
}


/**
 * Increment the dynamic load count.
 *
 * @returns 0
 * @param   pMod    The module.
 */
int kldrDyldModDynamicLoad(PKLDRDYLDMOD pMod)
{
    KLDRDYLDMOD_ASSERT(     pMod->enmState == KLDRSTATE_GOOD
                       ||   pMod->enmState == KLDRSTATE_PENDING_INITIALIZATION
                       ||   pMod->enmState == KLDRSTATE_INITIALIZING);
    pMod->cRefs++;
    pMod->cDynRefs++;
    return 0;
}


/**
 * Decrement the dynamic load count of the module and unload the module
 * if the total reference count reaches zero.
 *
 * This may cause a cascade of unloading to occure. See kldrDyldModUnloadPrerequisites().
 *
 * @returns status code.
 * @retval  0 on success.
 * @retval  KLDR_ERR_NOT_LOADED_DYNAMICALLY if the module wasn't loaded dynamically.
 * @param   pMod        The module to unload.
 */
int kldrDyldModDynamicUnload(PKLDRDYLDMOD pMod)
{
    if (pMod->cDynRefs == 0)
        return KLDR_ERR_NOT_LOADED_DYNAMICALLY;
    KLDRDYLDMOD_ASSERT(pMod->cDynRefs <= pMod->cRefs);
    KLDRDYLDMOD_ASSERT(pMod->enmState == KLDRSTATE_GOOD);

    pMod->cRefs--;
    pMod->cDynRefs--;
    if (    pMod->cDynRefs > 0
        ||  pMod->cDepRefs > 0)
        return 0;

    /*
     * The module should be unloaded.
     */
    kldrDyldModUnloadPrerequisites(pMod);
    return 0;
}


/**
 * Worker for kldrDyldModUnloadPrerequisites.
 *
 * @returns The number of modules that now can be unloaded.
 * @param   pMod    The module in  question.
 */
static KU32 kldrDyldModUnloadPrerequisitesOne(PKLDRDYLDMOD pMod)
{
    PKLDRDYLDMOD    pMod2;
    KU32            cToUnload = 0;
    KU32            i;

    KLDRDYLDMOD_ASSERT(pMod->papPrereqs || !pMod->cPrereqs);

    /*
     * Release the one in this module.
     */
    for (i = 0; i < pMod->cPrereqs; i++)
    {
        pMod2 = pMod->papPrereqs[i];
        if (pMod2)
        {
            pMod->papPrereqs[i] = NULL;

            /* do the derefering ourselves or we'll end up in a recursive loop here. */
            KLDRDYLDMOD_ASSERT(pMod2->cDepRefs > 0);
            KLDRDYLDMOD_ASSERT(pMod2->cRefs >= pMod2->cDepRefs);
            pMod2->cDepRefs--;
            pMod2->cRefs--;
            cToUnload += !pMod2->cDepRefs && !pMod2->cDynRefs;
        }
    }

    /*
     * Change the state
     */
    switch (pMod->enmState)
    {
        case KLDRSTATE_LOADED_PREREQUISITES:
        case KLDRSTATE_FIXED_UP:
            pMod->enmState = KLDRSTATE_PENDING_DESTROY;
            kldrDyldModUnlink(pMod);
            break;

        case KLDRSTATE_PENDING_INITIALIZATION:
            pMod->enmState = KLDRSTATE_PENDING_GC;
            break;

        case KLDRSTATE_RELOADED_FIXED_UP:
        case KLDRSTATE_RELOADED_LOADED_PREREQUISITES:
        case KLDRSTATE_GOOD:
            pMod->enmState = KLDRSTATE_PENDING_TERMINATION;
            break;

        case KLDRSTATE_INITIALIZATION_FAILED:
            break;

        default:
            KLDRDYLDMOD_ASSERT(!"invalid state");
            break;
    }

    return cToUnload;
}


/**
 * This is the heart of the unload code.
 *
 * It will recursivly (using the load list) initiate module unloading
 * of all affected modules.
 *
 * This function will cause a state transition to PENDING_DESTROY, PENDING_GC
 * or PENDING_TERMINATION depending on the module state. There is one exception
 * to this, and that's INITIALIZATION_FAILED, where the state will not be changed.
 *
 * @param   pMod        The module which prerequisites should be unloaded.
 */
void kldrDyldModUnloadPrerequisites(PKLDRDYLDMOD pMod)
{
    KU32            cToUnload;

    /* sanity */
#ifdef KLDRDYLD_STRICT
    {
    PKLDRDYLDMOD pMod2;
    for (pMod2 = kLdrDyldHead; pMod2; pMod2 = pMod2->Load.pNext)
        KLDRDYLDMOD_ASSERT(pMod2->enmState != KLDRSTATE_GOOD || pMod2->cRefs);
    }
#endif
    KLDRDYLDMOD_ASSERT(pMod->papPrereqs);

    /*
     * Unload prereqs of the module we're called on first.
     */
    cToUnload = kldrDyldModUnloadPrerequisitesOne(pMod);

    /*
     * Iterate the load list in a cyclic manner until there are no more
     * modules that can be pushed on into unloading.
     */
    while (cToUnload)
    {
        cToUnload = 0;
        for (pMod = kLdrDyldHead; pMod; pMod = pMod->Load.pNext)
        {
            if (    pMod->cDepRefs
                ||  pMod->cDynRefs
                ||  pMod->enmState >= KLDRSTATE_PENDING_TERMINATION
                ||  pMod->enmState < KLDRSTATE_LOADED_PREREQUISITES)
                continue;
            cToUnload += kldrDyldModUnloadPrerequisitesOne(pMod);
        }
    }
}


/**
 * Loads the prerequisite modules this module depends on.
 *
 * To find each of the prerequisite modules this method calls
 * kldrDyldGetPrerequisite() and it will make sure the modules
 * are added to the load stack frame.
 *
 * @returns 0 on success, non-zero native OS or kLdr status code on failure.
 *          The state is changed to LOADED_PREREQUISITES or RELOADED_LOADED_PREREQUISITES.
 * @param   pMod            The module.
 * @param   pszPrefix       Prefix to use when searching.
 * @param   pszSuffix       Suffix to use when searching.
 * @param   enmSearch       Method to use when locating the module and any modules it may depend on.
 * @param   fFlags          Flags, a combintation of the KLDRYDLD_LOAD_FLAGS_* \#defines.
 */
int kldrDyldModLoadPrerequisites(PKLDRDYLDMOD pMod, const char *pszPrefix, const char *pszSuffix,
                                 KLDRDYLDSEARCH enmSearch, unsigned fFlags)
{
    KI32    cPrereqs;
    KU32    i;
    int     rc = 0;

    /* sanity */
    switch (pMod->enmState)
    {
        case KLDRSTATE_MAPPED:
        case KLDRSTATE_RELOADED:
            break;
        default:
            KLDRDYLDMOD_ASSERT(!"invalid state");
            return -1;
    }

    /*
     * Query number of prerequiste modules and allocate the array.
     */
    cPrereqs = kLdrModNumberOfImports(pMod->pMod, NULL);
    kHlpAssert(cPrereqs >= 0);
    if (pMod->cPrereqs != cPrereqs)
    {
        KLDRDYLDMOD_ASSERT(!pMod->papPrereqs);
        pMod->papPrereqs = (PPKLDRDYLDMOD)kHlpAllocZ(sizeof(pMod->papPrereqs[0]) * cPrereqs);
        if (!pMod->papPrereqs)
            return KERR_NO_MEMORY;
        pMod->cPrereqs = cPrereqs;
    }
    else
        KLDRDYLDMOD_ASSERT(pMod->papPrereqs || !pMod->cPrereqs);

    /*
     * Iterate the prerequisites and load them.
     */
    for (i = 0; i < pMod->cPrereqs; i++)
    {
        static char s_szPrereq[260];
        PKLDRDYLDMOD pPrereqMod;

        KLDRDYLDMOD_ASSERT(pMod->papPrereqs[i] == NULL);
        rc = kLdrModGetImport(pMod->pMod, NULL, i, s_szPrereq, sizeof(s_szPrereq));
        if (rc)
            break;
        rc = kldrDyldGetPrerequisite(s_szPrereq, pszPrefix, pszSuffix, enmSearch, fFlags, pMod, &pPrereqMod);
        if (rc)
            break;
        pMod->papPrereqs[i] = pPrereqMod;
    }

    /* change the state regardless of what happend. */
    if (pMod->enmState == KLDRSTATE_MAPPED)
        pMod->enmState = KLDRSTATE_LOADED_PREREQUISITES;
    else
        pMod->enmState = KLDRSTATE_RELOADED_LOADED_PREREQUISITES;
    return rc;
}


/**
 * Maps an open module.
 *
 * On success the module will be in the MAPPED state.
 *
 * @returns 0 on success, non-zero native OS or kLdr status code on failure.
 * @param   pMod    The module which needs to be unmapped and set pending for destruction.
 */
int kldrDyldModMap(PKLDRDYLDMOD pMod)
{
    int rc;

    /* sanity */
    KLDRDYLDMOD_ASSERT(pMod->enmState == KLDRSTATE_OPEN);
    KLDRDYLDMOD_ASSERT(!pMod->fMapped);
    if (pMod->fMapped)
        return 0;

    /* do the job. */
    rc = kLdrModMap(pMod->pMod);
    if (!rc)
    {
        rc = kLdrModAllocTLS(pMod->pMod, KLDRMOD_INT_MAP);
        if (!rc)
        {
            /** @todo TLS */
            pMod->fMapped = 1;
            pMod->enmState = KLDRSTATE_MAPPED;
        }
        else
            kLdrModUnmap(pMod->pMod);
    }
    return rc;
}


/**
 * Unmaps the module, unlinks it from everywhere marks it PENDING_DESTROY.
 *
 * @returns 0 on success, non-zero native OS or kLdr status code on failure.
 * @param   pMod    The module which needs to be unmapped and set pending for destruction.
 */
int kldrDyldModUnmap(PKLDRDYLDMOD pMod)
{
    int rc;

    /* sanity */
    KLDRDYLDMOD_ASSERT(pMod->cRefs > 0);
    KLDRDYLDMOD_ASSERT(pMod->fMapped);
    switch (pMod->enmState)
    {
        case KLDRSTATE_MAPPED:
        case KLDRSTATE_GC:
        case KLDRSTATE_PENDING_DESTROY:
            break;
        default:
            KLDRDYLDMOD_ASSERT(!"invalid state");
            return -1;
    }

    /* do the job. */
    if (pMod->fAllocatedTLS)
    {
        kLdrModFreeTLS(pMod->pMod, KLDRMOD_INT_MAP);
        pMod->fAllocatedTLS = 0;
    }
    rc = kLdrModUnmap(pMod->pMod);
    if (!rc)
    {
        pMod->fMapped = 0;
        if (pMod->enmState < KLDRSTATE_PENDING_DESTROY)
        {
            pMod->enmState = KLDRSTATE_PENDING_DESTROY;
            kldrDyldModUnlink(pMod);
        }
    }

    return rc;
}


/**
 * Reloads the module.
 *
 * Reloading means that all modified pages are restored to their original
 * state. Whether this includes the code segments depends on whether the fixups
 * depend on the addend in the place they are fixing up - so it's format specific.
 *
 * @returns 0 on success, non-zero native OS or kLdr status code on failure.
 * @param   pMod    The module which needs to be unmapped and set pending for destruction.
 */
int kldrDyldModReload(PKLDRDYLDMOD pMod)
{
    int rc;

    /* sanity */
    KLDRDYLDMOD_ASSERT(pMod->cRefs > 0);
    KLDRDYLDMOD_ASSERT(pMod->fMapped);

    switch (pMod->enmState)
    {
        case KLDRSTATE_MAPPED:
        case KLDRSTATE_GC:
        case KLDRSTATE_PENDING_DESTROY:
            break;
        default:
            KLDRDYLDMOD_ASSERT(!"invalid state");
            return -1;
    }

    /* Free TLS before reloading. */
    if (pMod->fAllocatedTLS)
    {
        kLdrModFreeTLS(pMod->pMod, KLDRMOD_INT_MAP);
        pMod->fAllocatedTLS = 0;
    }

    /* Let the module interpreter do the reloading of the mapping. */
    rc = kLdrModReload(pMod->pMod);
    if (!rc)
    {
        rc = kLdrModAllocTLS(pMod->pMod, KLDRMOD_INT_MAP);
        if (!rc)
        {
            pMod->fAllocatedTLS = 1;
            pMod->enmState = KLDRSTATE_RELOADED;
        }
    }
    return rc;
}


/**
 * @copydoc FNKLDRMODGETIMPORT
 * pvUser points to the KLDRDYLDMOD.
 */
static int kldrDyldModFixupGetImportCallback(PKLDRMOD pMod, KU32 iImport, KU32 iSymbol,
                                             const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                             PKLDRADDR puValue, KU32 *pfKind, void *pvUser)
{
    static int s_cRecursiveCalls = 0;
    PKLDRDYLDMOD pDyldMod = (PKLDRDYLDMOD)pvUser;
    int rc;

    /* guard against too deep forwarder recursion. */
    if (s_cRecursiveCalls >= 5)
        return KLDR_ERR_TOO_LONG_FORWARDER_CHAIN;
    s_cRecursiveCalls++;

    if (iImport != NIL_KLDRMOD_IMPORT)
    {
        /* specific import module search. */
        PKLDRDYLDMOD pPrereqMod;

        KLDRDYLDMOD_ASSERT(iImport < pDyldMod->cPrereqs);
        pPrereqMod = pDyldMod->papPrereqs[iImport];

        KLDRDYLDMOD_ASSERT(pPrereqMod);
        KLDRDYLDMOD_ASSERT(pPrereqMod->u32MagicHead == KLDRDYMOD_MAGIC);
        KLDRDYLDMOD_ASSERT(pPrereqMod->u32MagicTail == KLDRDYMOD_MAGIC);
        KLDRDYLDMOD_ASSERT(pPrereqMod->enmState < KLDRSTATE_TERMINATING);

        rc = kLdrModQuerySymbol(pPrereqMod->pMod, NULL, KLDRMOD_BASEADDRESS_MAP,
                                iSymbol, pchSymbol, cchSymbol, pszVersion,
                                kldrDyldModFixupGetImportCallback, pPrereqMod, puValue, pfKind);
        if (rc)
        {
            if (pchSymbol)
                kldrDyldFailure(rc, "%s[%d]->%s.%.*s%s", pDyldMod->pMod->pszName, iImport,
                                pPrereqMod->pMod->pszName, cchSymbol, pchSymbol, pszVersion ? pszVersion : "");
            else
                kldrDyldFailure(rc, "%s[%d]->%s.%d%s", pDyldMod->pMod->pszName, iImport,
                                pPrereqMod->pMod->pszName, iSymbol, pszVersion ? pszVersion : "");
        }
    }
    else
    {
        /* bind list search. */
        unsigned fFound = 0;
        PKLDRDYLDMOD pBindMod = g_pkLdrDyldBindHead;
        rc = 0;
        while (pBindMod)
        {
            KU32 fKind;
            KLDRADDR uValue;
            rc = kLdrModQuerySymbol(pBindMod->pMod, NULL, KLDRMOD_BASEADDRESS_MAP,
                                    iSymbol, pchSymbol, cchSymbol, pszVersion,
                                    kldrDyldModFixupGetImportCallback, pBindMod, &uValue, &fKind);
            if (    !rc
                &&  (   !fFound
                     || !(fKind & KLDRSYMKIND_WEAK)
                    )
               )
            {
                *pfKind = fKind;
                *puValue = uValue;
                fFound = 1;
                if (!(fKind & KLDRSYMKIND_WEAK))
                    break;
            }

            /* next */
            pBindMod = pBindMod->Bind.pNext;
        }
        rc = fFound ? 0 : KLDR_ERR_SYMBOL_NOT_FOUND;
        if (!fFound)
        {
            if (pchSymbol)
                kldrDyldFailure(rc, "%s->%.*s%s", pDyldMod->pMod->pszName, cchSymbol, pchSymbol, pszVersion ? pszVersion : "");
            else
                kldrDyldFailure(rc, "%s->%d%s", pDyldMod->pMod->pszName, iSymbol, pszVersion ? pszVersion : "");
        }
    }

    s_cRecursiveCalls--;
    return rc;
}


/**
 * Applies fixups to a module which prerequisistes has been
 * successfully loaded.
 *
 * @returns 0 on success, non-zero native OS or kLdr status code on failure.
 * @param   pMod    The module which needs to be unmapped and set pending for destruction.
 */
int kldrDyldModFixup(PKLDRDYLDMOD pMod)
{
    int rc;

    /* sanity */
    KLDRDYLDMOD_ASSERT(pMod->cRefs > 0);
    KLDRDYLDMOD_ASSERT(     pMod->enmState == KLDRSTATE_LOADED_PREREQUISITES
                       ||   pMod->enmState == KLDRSTATE_RELOADED_LOADED_PREREQUISITES);

    /* do the job */
    rc = kLdrModFixupMapping(pMod->pMod, kldrDyldModFixupGetImportCallback, pMod);/** @todo fixme. */
    if (!rc)
        pMod->enmState = KLDRSTATE_FIXED_UP;
    return rc;
}


/**
 * Calls the module initialization entry point if any.
 *
 * This is considered to be a module specific thing and leave if
 * to the module interpreter. They will have to deal with different
 * module init practices between platforms should there be any.
 *
 * @returns 0 and state changed to GOOD on success.
 *          Non-zero OS or kLdr status code and status changed to INITIALIZATION_FAILED on failure.
 * @param   pMod        The module that should be initialized.
 */
int kldrDyldModCallInit(PKLDRDYLDMOD pMod)
{
    int rc;

    KLDRDYLDMOD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_INITIALIZATION);
    KLDRDYLDMOD_ASSERT(!pMod->fInitList);

    pMod->enmState = KLDRSTATE_INITIALIZING;
    rc = kLdrModCallInit(pMod->pMod, KLDRMOD_INT_MAP, (KUPTR)pMod->hMod);
    if (!rc)
    {
        pMod->enmState = KLDRSTATE_GOOD;
        /* push it onto the termination list.*/
        pMod->InitTerm.pPrev = NULL;
        pMod->InitTerm.pNext = g_pkLdrDyldTermHead;
        if (g_pkLdrDyldTermHead)
            g_pkLdrDyldTermHead->InitTerm.pPrev = pMod;
        else
            g_pkLdrDyldTermTail = pMod;
        g_pkLdrDyldTermHead = pMod;
    }
    else
        pMod->enmState = KLDRSTATE_INITIALIZATION_FAILED;

    return rc;
}


/**
 * Calls the module termination entry point if any.
 *
 * This'll change the module status to PENDING_GC.
 *
 * @param   pMod        The module that should be initialized.
 */
void kldrDyldModCallTerm(PKLDRDYLDMOD pMod)
{
    KLDRDYLDMOD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_TERMINATION);

    pMod->enmState = KLDRSTATE_TERMINATING;
    kLdrModCallTerm(pMod->pMod, KLDRMOD_INT_MAP, (KUPTR)pMod->hMod);
    pMod->enmState = KLDRSTATE_PENDING_GC;
    /* unlinking on destruction. */
}


/**
 * Calls the thread attach entry point if any.
 *
 * @returns 0 on success, non-zero on failure.
 * @param   pMod        The module.
 */
int kldrDyldModAttachThread(PKLDRDYLDMOD pMod)
{
    KLDRDYLDMOD_ASSERT(pMod->enmState == KLDRSTATE_GOOD);

    return kLdrModCallThread(pMod->pMod, KLDRMOD_INT_MAP, (KUPTR)pMod->hMod, 1 /* attach */);
}


/**
 * Calls the thread detach entry point if any.
 *
 * @returns 0 on success, non-zero on failure.
 * @param   pMod        The module.
 */
void kldrDyldModDetachThread(PKLDRDYLDMOD pMod)
{
    KLDRDYLDMOD_ASSERT(pMod->enmState == KLDRSTATE_GOOD);

    kLdrModCallThread(pMod->pMod, KLDRMOD_INT_MAP, (KUPTR)pMod->hMod, 0 /* detach */);
}


/**
 * Gets the main stack, allocate it if necessary.
 *
 * @returns 0 on success, non-zero native OS or kLdr status code on failure.
 * @param   pMod        The module.
 * @param   ppvStack    Where to store the address of the stack (lowest address).
 * @param   pcbStack    Where to store the size of the stack.
 */
int kldrDyldModGetMainStack(PKLDRDYLDMOD pMod, void **ppvStack, KSIZE *pcbStack)
{
    int rc = 0;
    KLDRSTACKINFO StackInfo;
    KLDRDYLDMOD_ASSERT(pMod->fExecutable);

    /*
     * Since we might have to allocate the stack ourselves, and there will only
     * ever be one main stack, we'll be keeping the main stack info in globals.
     */
    if (!g_fkLdrDyldDoneMainStack)
    {
        rc = kLdrModGetStackInfo(pMod->pMod, NULL, KLDRMOD_BASEADDRESS_MAP, &StackInfo);
        if (!rc)
        {
            /* check if there is a stack size override/default. */
            KSIZE cbDefOverride;
            if (kHlpGetEnvUZ("KLDR_MAIN_STACK_SIZE", &cbDefOverride))
                cbDefOverride = 0;


            /* needs allocating? */
            if (    StackInfo.LinkAddress == NIL_KLDRADDR
                ||  StackInfo.cbStack < cbDefOverride)
            {
                KSIZE cbStack = (KSIZE)K_MAX(StackInfo.cbStack, cbDefOverride);

                g_pvkLdrDyldMainStack = kldrDyldOSAllocStack(cbStack);
                if (g_pvkLdrDyldMainStack)
                {
                    g_cbkLdrDyldMainStack = cbStack;
                    g_fkLdrDyldMainStackAllocated = 1;
                }
                else
                    rc = KLDR_ERR_MAIN_STACK_ALLOC_FAILED;
            }
            else
            {
                KLDRDYLDMOD_ASSERT(StackInfo.Address != NIL_KLDRADDR);
                KLDRDYLDMOD_ASSERT(StackInfo.cbStack > 0);

                g_fkLdrDyldMainStackAllocated = 0;
                g_pvkLdrDyldMainStack = (void *)(KUPTR)StackInfo.Address;
                KLDRDYLDMOD_ASSERT((KUPTR)g_pvkLdrDyldMainStack == StackInfo.Address);

                g_cbkLdrDyldMainStack = (KSIZE)StackInfo.cbStack;
                KLDRDYLDMOD_ASSERT(StackInfo.cbStack == g_cbkLdrDyldMainStack);
            }
        }
        if (!rc)
            g_fkLdrDyldDoneMainStack = 1;
    }

    if (!rc)
    {
        if (ppvStack)
            *ppvStack = g_pvkLdrDyldMainStack;
        if (pcbStack)
            *pcbStack = g_cbkLdrDyldMainStack;
    }

    return rc;
}


/**
 * This starts the executable module.
 *
 * @returns non-zero OS or kLdr status code on failure.
 *          (won't return on success.)
 * @param   pMod    The executable module.
 */
int kldrDyldModStartExe(PKLDRDYLDMOD pMod)
{
    int         rc;
    KLDRADDR    MainEPAddress;
    void       *pvStack;
    KSIZE      cbStack;
    KLDRDYLDMOD_ASSERT(pMod->fExecutable);

    rc = kLdrModQueryMainEntrypoint(pMod->pMod, NULL, KLDRMOD_BASEADDRESS_MAP, &MainEPAddress);
    if (rc)
        return rc;
    rc = kldrDyldModGetMainStack(pMod, &pvStack, &cbStack);
    if (rc)
        return rc;
    return kldrDyldOSStartExe((KUPTR)MainEPAddress, pvStack, cbStack);
}


/**
 * Gets the module name.
 *
 * @returns 0 on success, KERR_BUFFER_OVERFLOW on failure.
 * @param   pMod            The module.
 * @param   pszName         Where to store the name.
 * @param   cchName         The size of the name buffer.
 */
int kldrDyldModGetName(PKLDRDYLDMOD pMod, char *pszName, KSIZE cchName)
{
    KSIZE cch = K_MIN(cchName, pMod->pMod->cchName + 1);
    if (cch)
    {
        kHlpMemCopy(pszName, pMod->pMod->pszName, cch - 1);
        pszName[cch - 1] = '\0';
    }
    return cchName <= pMod->pMod->cchName ? KERR_BUFFER_OVERFLOW : 0;
}


/**
 * Gets the module filename.
 *
 * @returns 0 on success, KERR_BUFFER_OVERFLOW on failure.
 * @param   pMod            The module.
 * @param   pszFilename     Where to store the filename.
 * @param   cchFilename     The size of the filename buffer.
 */
int kldrDyldModGetFilename(PKLDRDYLDMOD pMod, char *pszFilename, KSIZE cchFilename)
{
    KSIZE cch = K_MIN(cchFilename, pMod->pMod->cchFilename + 1);
    if (cch)
    {
        kHlpMemCopy(pszFilename, pMod->pMod->pszFilename, cch - 1);
        pszFilename[cch - 1] = '\0';
    }
    return cchFilename <= pMod->pMod->cchFilename ? KERR_BUFFER_OVERFLOW : 0;
}


/**
 * Gets the address/value of a symbol in the specified module.
 *
 * @returns 0 on success, KLDR_ERR_SYMBOL_NOT_FOUND on failure.
 * @param   pMod            The module.
 * @param   uSymbolOrdinal  The symbol ordinal 0. This is ignored if the name is non-zero.
 * @param   pszSymbolName   The symbol name. Can be NULL.
 * @param   puValue         Where to store the value. optional.
 * @param   pfKind          Where to store the symbol kind. optional.
 */
int kldrDyldModQuerySymbol(PKLDRDYLDMOD pMod, KU32 uSymbolOrdinal, const char *pszSymbolName,
                           KUPTR *puValue, KU32 *pfKind)
{
    int         rc;
    KLDRADDR    uValue = 0;
    KU32        fKind = 0;

    rc = kLdrModQuerySymbol(pMod->pMod, NULL, KLDRMOD_BASEADDRESS_MAP,
                            uSymbolOrdinal, pszSymbolName, kHlpStrLen(pszSymbolName), NULL,
                            kldrDyldModFixupGetImportCallback, pMod,
                            &uValue, &fKind);
    if (!rc)
    {
        if (puValue)
        {
            *puValue = (KUPTR)uValue;
            KLDRDYLDMOD_ASSERT(*puValue == uValue);
        }
        if (pfKind)
            *pfKind = fKind;
    }

    return rc;
}

