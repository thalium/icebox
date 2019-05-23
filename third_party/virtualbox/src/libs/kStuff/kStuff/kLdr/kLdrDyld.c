/* $Id: kLdrDyld.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader.
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
/** @def KLDRDYLD_STRICT
 * Define KLDRDYLD_STRICT to enabled strict checks in kLdrDyld. */
#define KLDRDYLD_STRICT 1

/** @def KLDRDYLD_ASSERT
 * Assert that an expression is true when KLDRDYLD_STRICT is defined.
 */
#ifdef KLDRDYLD_STRICT
# define KLDRDYLD_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRDYLD_ASSERT(expr)  do {} while (0)
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Pointer to the executable module.
 * (This is exported, so no prefix.) */
PKLDRDYLDMOD    kLdrDyldExe = NULL;
/** Pointer to the head module (the executable).
 * (This is exported, so no prefix.) */
PKLDRDYLDMOD    kLdrDyldHead = NULL;
/** Pointer to the tail module.
 * (This is exported, so no prefix.) */
PKLDRDYLDMOD    kLdrDyldTail = NULL;
/** Pointer to the head module of the initialization list.
 * The outermost load call will pop elements from this list in LIFO order (i.e.
 * from the tail). The list is only used during non-recursive initialization
 * and may therefore share the pNext/pPrev members with the termination list
 * since we don't push a module onto the termination list untill it has been
 * successfully initialized. */
PKLDRDYLDMOD    g_pkLdrDyldInitHead;
/** Pointer to the tail module of the initalization list.*/
PKLDRDYLDMOD    g_pkLdrDyldInitTail;
/** Pointer to the head module of the termination order list.
 * This is a LIFO just like the the init list. */
PKLDRDYLDMOD    g_pkLdrDyldTermHead;
/** Pointer to the tail module of the termination order list. */
PKLDRDYLDMOD    g_pkLdrDyldTermTail;
/** Pointer to the head module of the bind order list.
 * The modules in this list makes up the global namespace used when binding symbol unix fashion. */
PKLDRDYLDMOD    g_pkLdrDyldBindHead;
/** Pointer to the tail module of the bind order list. */
PKLDRDYLDMOD    g_pkLdrDyldBindTail;

/** Flag indicating bootstrap time.
 * When set the error behaviour changes. Any kind of serious failure
 * is fatal and will terminate the process. */
int             g_fBootstrapping;
/** The global error buffer. */
char            g_szkLdrDyldError[1024];

/** The default flags. */
KU32            kLdrDyldFlags = 0;
/** The default search method. */
KLDRDYLDSEARCH  kLdrDyldSearch = KLDRDYLD_SEARCH_HOST;


/** @name The main stack.
 * @{ */
/** Indicates that the other MainStack globals have been filled in. */
unsigned        g_fkLdrDyldDoneMainStack = 0;
/** Whether the stack was allocated seperatly or was part of the executable. */
unsigned        g_fkLdrDyldMainStackAllocated = 0;
/** Pointer to the main stack object. */
void           *g_pvkLdrDyldMainStack = NULL;
/** The size of the main stack object. */
KSIZE           g_cbkLdrDyldMainStack = 0;
/** @} */


/** The load stack.
 * This contains frames with modules affected by active loads.
 *
 * Each kLdrDyldLoad and kLdrDyldLoadExe call will create a new stack frame containing
 * all the modules involved in the operation. The modules will be ordered in recursive
 * init order within the frame.
 */
static PPKLDRDYLDMOD    g_papStackMods;
/** The number of used entries in the g_papStackMods array. */
static KU32             g_cStackMods;
/** The number of entries allocated for the g_papStackMods array. */
static KU32             g_cStackModsAllocated;
/** Number of active load calls. */
static KU32             g_cActiveLoadCalls;
/** Number of active unload calls. */
static KU32             g_cActiveUnloadCalls;
/** Total number of load calls. */
static KU32             g_cTotalLoadCalls;
/** Total mumber of unload calls. */
static KU32             g_cTotalUnloadCalls;
/** Boolean flag indicating that GC is active. */
static KU32             g_fActiveGC;



/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
/** @name API worker routines.
 * @internal
 * @{ */
void       kldrDyldDoLoadExeStackSwitch(PKLDRDYLDMOD pExe, void *pvStack, KSIZE cbStack);
static int kldrDyldDoLoad(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                          unsigned fFlags, PPKLDRDYLDMOD ppMod, char *pszErr, KSIZE cchErr);
static int kldrDyldDoLoad2(PKLDRDYLDMOD pLoadedMod, const char *pszPrefix, const char *pszSuffix,
                           KLDRDYLDSEARCH enmSearch, unsigned fFlags);
static int kldrDyldDoLoadPrerequisites(PKLDRDYLDMOD pMod, const char *pszPrefix, const char *pszSuffix,
                                       KLDRDYLDSEARCH enmSearch, unsigned fFlags);
static int kldrDyldDoUnload(PKLDRDYLDMOD pMod);
static int kldrDyldDoFindByName(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                              unsigned fFlags, PPKLDRDYLDMOD ppMod);
static int kldrDyldDoFindByAddress(KUPTR Address, PPKLDRDYLDMOD ppMod, KU32 *piSegment, KUPTR *poffSegment);
static int kldrDyldDoGetName(PKLDRDYLDMOD pMod, char *pszName, KSIZE cchName);
static int kldrDyldDoGetFilename(PKLDRDYLDMOD pMod, char *pszFilename, KSIZE cchFilename);
static int kldrDyldDoQuerySymbol(PKLDRDYLDMOD pMod, KU32 uSymbolOrdinal, const char *pszSymbolName, KUPTR *pValue, KU32 *pfKind);
/** @} */

/** @name Misc load/unload workers
 * @internal
 * @{
 */
static void         kldrDyldDoModuleTerminationAndGarabageCollection(void);
/** @} */

/** @name The load stack.
 * @internal
 * @{ */
static KU32         kldrDyldStackNewFrame(PKLDRDYLDMOD pMod);
static int          kldrDyldStackAddModule(PKLDRDYLDMOD pMod);
static int          kldrDyldStackFrameCompleted(void);
static void         kldrDyldStackCleanupOne(PKLDRDYLDMOD pMod, int rc);
static void         kldrDyldStackDropFrame(KU32 iLoad1st, KU32 iLoadEnd, int rc);
/** @} */

static int          kldrDyldCopyError(int rc, char *pszErr, KSIZE cchErr);



/**
 * Initialize the dynamic loader.
 */
int kldrDyldInit(void)
{
    kLdrDyldHead = kLdrDyldTail = NULL;
    g_pkLdrDyldTermHead = g_pkLdrDyldTermTail = NULL;
    g_pkLdrDyldBindHead = g_pkLdrDyldBindTail = NULL;
    kLdrDyldFlags = 0;
    g_szkLdrDyldError[0] = '\0';

    g_fkLdrDyldDoneMainStack = 0;
    g_fkLdrDyldMainStackAllocated = 0;
    g_pvkLdrDyldMainStack = NULL;
    g_cbkLdrDyldMainStack = 0;

    return kldrDyldFindInit();
}


/**
 * Terminate the dynamic loader.
 */
void kldrDyldTerm(void)
{

}


/**
 * Bootstrap an executable.
 *
 * This is called from the executable stub to replace the stub and run the
 * executable specified in the argument package.
 *
 * Since this is boostrap time there isn't anything to return to. So, instead
 * the process will be terminated upon failure.
 *
 * We also have to keep in mind that this function is called on a small, small,
 * stack and therefore any kind of large stack objects or deep recursions must
 * be avoided. Since loading the executable will involve more or less all
 * operations in the loader, this restriction really applies everywhere.
 *
 * @param   pArgs       Pointer to the argument package residing in the executable stub.
 * @param   pvOS        OS specific argument.
 */
#ifndef __OS2__  /* kLdrDyldLoadExe is implemented in assembly on OS/2. */
void kLdrDyldLoadExe(PCKLDREXEARGS pArgs, void *pvOS)
#else
void kldrDyldLoadExe(PCKLDREXEARGS pArgs, void *pvOS)
#endif
{
    void *pvStack;
    KSIZE  cbStack;
    PKLDRDYLDMOD pExe;
    int rc;

    /*
     * Indicate that we're boostrapping and ensure that initialization was successful.
     */
    g_fBootstrapping = 1;
    rc = kldrInit();
    if (rc)
        kldrDyldFailure(rc, "Init failure, rc=%d", rc);

    /*
     * Validate the argument package.
     */
    if (pArgs->fFlags & ~(  KLDRYDLD_LOAD_FLAGS_GLOBAL_SYMBOLS
                          | KLDRYDLD_LOAD_FLAGS_DEEP_SYMBOLS
                          | KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT
                          | KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE))
        kldrDyldFailure(KERR_INVALID_PARAMETER, "Bad fFlags=%#x", pArgs->fFlags);
    if (    pArgs->enmSearch <= KLDRDYLD_SEARCH_INVALID
        ||  pArgs->enmSearch >= KLDRDYLD_SEARCH_END)
        kldrDyldFailure(KERR_INVALID_PARAMETER, "Bad enmSearch=%d", pArgs->enmSearch);

    /*
     * Set defaults.
     */
    kLdrDyldFlags |= (pArgs->fFlags & KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT);
    kLdrDyldSearch = pArgs->enmSearch;

    /** @todo make sense of this default prefix/suffix stuff. */
    if (pArgs->szDefPrefix[0] != '\0')
        kHlpMemCopy(kLdrDyldDefPrefix, pArgs->szDefPrefix, K_MIN(sizeof(pArgs->szDefPrefix), sizeof(kLdrDyldDefPrefix)));
    if (pArgs->szDefSuffix[0] != '\0')
        kHlpMemCopy(kLdrDyldDefSuffix, pArgs->szDefSuffix, K_MIN(sizeof(pArgs->szDefSuffix), sizeof(kLdrDyldDefSuffix)));

    /** @todo append that path to the one for the specified search method. */
    /** @todo create a function for doing this, an exposed api preferably. */
    /* append path */
    cbStack = sizeof(kLdrDyldLibraryPath) - kHlpStrLen(kLdrDyldLibraryPath); /* borrow cbStack for a itty bit. */
    kHlpMemCopy(kLdrDyldLibraryPath, pArgs->szLibPath, K_MIN(sizeof(pArgs->szLibPath), cbStack));
    kLdrDyldLibraryPath[sizeof(kLdrDyldLibraryPath) - 1] = '\0';

    /*
     * Make sure we own the loader semaphore (necessary for init).
     */
    rc = kLdrDyldSemRequest();
    if (rc)
        kldrDyldFailure(rc, "Sem req. failure, rc=%d", rc);

    /*
     * Open and map the executable module before we join paths with kLdrDyldLoad().
     */
    rc = kldrDyldFindNewModule(pArgs->szExecutable, NULL, NULL, pArgs->enmSearch,
                               pArgs->fFlags | KLDRDYLD_LOAD_FLAGS_EXECUTABLE, &pExe);
    if (rc)
        kldrDyldFailure(rc, "Can't find/open the executable '%s', rc=%d", pArgs->szExecutable, rc);
    rc = kldrDyldModMap(pExe);
    if (rc)
        kldrDyldFailure(rc, "Failed to map the executable '%s', rc=%d", pExe->pMod->pszFilename, rc);

    kLdrDyldExe = pExe;

    /*
     * Query the stack and go to OS specific code to
     * setup and switch stack. The OS specific code will call us
     * back at kldrDyldDoLoadExe.
     */
    rc = kldrDyldModGetMainStack(pExe, &pvStack, &cbStack);
    if (rc)
        kldrDyldFailure(rc, "Failed to map the executable '%s', rc=%d", pExe->pMod->pszFilename, rc);
    kldrDyldDoLoadExeStackSwitch(pExe, pvStack, cbStack);
    kldrDyldFailure(-1, "Failed to setup the stack for '%s'.", pExe->pMod->pszFilename);
}


/**
 * Loads a module into the current process.
 *
 * @returns 0 on success, non-zero native OS status code or kLdr status code on failure.
 * @param   pszDll          The name of the dll to open.
 * @param   pszPrefix       Prefix to use when searching.
 * @param   pszSuffix       Suffix to use when searching.
 * @param   enmSearch       Method to use when locating the module and any modules it may depend on.
 * @param   fFlags          Flags, a combintation of the KLDRYDLD_LOAD_FLAGS_* \#defines.
 * @param   phMod           Where to store the handle to the loaded module.
 * @param   pszErr          Where to store extended error information. (optional)
 * @param   cchErr          The size of the buffer pointed to by pszErr.
 */
int     kLdrDyldLoad(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                     unsigned fFlags, PHKLDRMOD phMod, char *pszErr, KSIZE cchErr)
{
    int rc;

    /* validate arguments and initialize return values. */
    if (pszErr && cchErr)
        *pszErr = '\0';
    *phMod = NIL_HKLDRMOD;
    K_VALIDATE_STRING(pszDll);
    K_VALIDATE_OPTIONAL_STRING(pszPrefix);
    K_VALIDATE_OPTIONAL_STRING(pszSuffix);
    K_VALIDATE_ENUM(enmSearch, KLDRDYLD_SEARCH);
    K_VALIDATE_OPTIONAL_BUFFER(pszErr, cchErr);

    /* get the semaphore and do the job. */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        PKLDRDYLDMOD pMod = NULL;
        g_cTotalLoadCalls++;
        g_cActiveLoadCalls++;
        rc = kldrDyldDoLoad(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, &pMod, pszErr, cchErr);
        g_cActiveLoadCalls--;
        kldrDyldDoModuleTerminationAndGarabageCollection();
        kLdrDyldSemRelease();
        *phMod = pMod ? pMod->hMod : NIL_HKLDRMOD;
    }
    return rc;
}


/**
 * Unloads a module loaded by kLdrDyldLoad.
 *
 * @returns 0 on success, non-zero native OS status code or kLdr status code on failure.
 * @param   hMod            Module handle.
 */
int     kLdrDyldUnload(HKLDRMOD hMod)
{
    int rc;

    /* validate */
    KLDRDYLD_VALIDATE_HKLDRMOD(hMod);

    /* get sem & do work */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        g_cTotalUnloadCalls++;
        g_cActiveUnloadCalls++;
        rc = kldrDyldDoUnload(hMod);
        g_cActiveUnloadCalls--;
        kldrDyldDoModuleTerminationAndGarabageCollection();
        kLdrDyldSemRelease();
    }
    return rc;
}


/**
 * Finds a module by name or filename.
 *
 * This call does not increase any reference counters and must not be
 * paired with kLdrDyldUnload() like kLdrDyldLoad().
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MODULE_NOT_FOUND or some I/O error on failure.
 * @param   pszDll          The name of the dll to look for.
 * @param   pszPrefix       Prefix than can be used when searching.
 * @param   pszSuffix       Suffix than can be used when searching.
 * @param   enmSearch       Method to use when locating the module.
 * @param   fFlags          Flags, a combintation of the KLDRYDLD_LOAD_FLAGS_* \#defines.
 * @param   phMod           Where to store the handle of the module on success.
 */
int     kLdrDyldFindByName(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                           unsigned fFlags, PHKLDRMOD phMod)
{
    int rc;

    /* validate & initialize */
    *phMod = NIL_HKLDRMOD;
    K_VALIDATE_STRING(pszDll);

    /* get sem & do work */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        PKLDRDYLDMOD pMod = NULL;
        rc = kldrDyldDoFindByName(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, &pMod);
        kLdrDyldSemRelease();
        *phMod = pMod ? pMod->hMod : NIL_HKLDRMOD;
    }
    return rc;
}


/**
 * Finds a module by address.
 *
 * This call does not increase any reference counters and must not be
 * paired with kLdrDyldUnload() like kLdrDyldLoad().
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MODULE_NOT_FOUND on failure.
 * @param   Address         The address believed to be within some module.
 * @param   phMod           Where to store the module handle on success.
 * @param   piSegment       Where to store the segment number. (optional)
 * @param   poffSegment     Where to store the offset into the segment. (optional)
 */
int     kLdrDyldFindByAddress(KUPTR Address, PHKLDRMOD phMod, KU32 *piSegment, KUPTR *poffSegment)
{
    int rc;

    /* validate & initialize */
    *phMod = NIL_HKLDRMOD;
    if (piSegment)
        *piSegment = ~(KU32)0;
    if (poffSegment)
        *poffSegment = ~(KUPTR)0;

    /* get sem & do work */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        PKLDRDYLDMOD pMod = NULL;
        rc = kldrDyldDoFindByAddress(Address, &pMod, piSegment, poffSegment);
        kLdrDyldSemRelease();
        *phMod = pMod ? pMod->hMod : NIL_HKLDRMOD;
    }
    return rc;
}


/**
 * Gets the module name.
 *
 * @returns 0 on success and pszName filled with the name.
 * @returns KERR_INVALID_HANDLE or KERR_BUFFER_OVERFLOW on failure.
 * @param   hMod        The module handle.
 * @param   pszName     Where to put the name.
 * @param   cchName     The size of the name buffer.
 * @see kLdrDyldGetFilename
 */
int     kLdrDyldGetName(HKLDRMOD hMod, char *pszName, KSIZE cchName)
{
    int rc;

    /* validate */
    if (pszName && cchName)
        *pszName = '\0';
    KLDRDYLD_VALIDATE_HKLDRMOD(hMod);
    K_VALIDATE_BUFFER(pszName, cchName);

    /* get sem & do work */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        rc = kldrDyldDoGetName(hMod, pszName, cchName);
        kLdrDyldSemRelease();
    }
    return rc;
}


/**
 * Gets the module filename.
 *
 * @returns 0 on success and pszFilename filled with the name.
 * @returns KERR_INVALID_HANDLE or KERR_BUFFER_OVERFLOW on failure.
 * @param   hMod            The module handle.
 * @param   pszFilename     Where to put the filename.
 * @param   cchFilename     The size of the filename buffer.
 * @see kLdrDyldGetName
 */
int     kLdrDyldGetFilename(HKLDRMOD hMod, char *pszFilename, KSIZE cchFilename)
{
    int rc;

    /* validate & initialize */
    if  (pszFilename && cchFilename);
        *pszFilename = '\0';
    KLDRDYLD_VALIDATE_HKLDRMOD(hMod);
    K_VALIDATE_BUFFER(pszFilename, cchFilename);

    /* get sem & do work */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        rc = kldrDyldDoGetFilename(hMod, pszFilename, cchFilename);
        kLdrDyldSemRelease();
    }
    return rc;
}


/**
 * Queries the value and type of a symbol.
 *
 * @returns 0 on success and pValue and pfKind set.
 * @returns KERR_INVALID_HANDLE or KLDR_ERR_SYMBOL_NOT_FOUND on failure.
 * @param   hMod                The module handle.
 * @param   uSymbolOrdinal      The symbol ordinal. This is ignored if pszSymbolName is non-zero.
 * @param   pszSymbolName       The symbol name.
 * @param   pszSymbolVersion    The symbol version. Optional.
 * @param   pValue              Where to put the symbol value. Optional if pfKind is non-zero.
 * @param   pfKind              Where to put the symbol kind flags. Optional if pValue is non-zero.
 */
int     kLdrDyldQuerySymbol(HKLDRMOD hMod, KU32 uSymbolOrdinal, const char *pszSymbolName,
                            const char *pszSymbolVersion, KUPTR *pValue, KU32 *pfKind)
{
    int rc;

    /* validate & initialize */
    if (pfKind)
        *pfKind = 0;
    if (pValue)
        *pValue = 0;
    if (!pfKind && !pValue)
        return KERR_INVALID_PARAMETER;
    KLDRDYLD_VALIDATE_HKLDRMOD(hMod);
    K_VALIDATE_OPTIONAL_STRING(pszSymbolName);

    /* get sem & do work */
    rc = kLdrDyldSemRequest();
    if (!rc)
    {
        rc = kldrDyldDoQuerySymbol(hMod, uSymbolOrdinal, pszSymbolName, pValue, pfKind);
        kLdrDyldSemRelease();
    }
    return rc;
}


/**
 * Worker kLdrDoLoadExe().
 * Used after we've switch to the final process stack.
 *
 * @param   pExe    The executable module.
 * @internal
 */
void kldrDyldDoLoadExe(PKLDRDYLDMOD pExe)
{
    int rc;

    /*
     * Load the executable module with its prerequisites and initialize them.
     */
    g_cActiveLoadCalls++;
    rc = kldrDyldDoLoad2(pExe, NULL, NULL, kLdrDyldSearch, kLdrDyldFlags | KLDRDYLD_LOAD_FLAGS_EXECUTABLE);
    if (rc)
        kldrDyldFailure(rc, "load 2 failed for '%s', rc=%d", pExe->pMod->pszFilename);
    g_cActiveLoadCalls--;
    kldrDyldDoModuleTerminationAndGarabageCollection();

    /*
     * Invoke the executable entry point.
     */
    kldrDyldModStartExe(pExe);
    kldrDyldFailure(-1, "failed to invoke main!");
}


/**
 * Worker for kLdrDyldLoad() and helper for kLdrDyldLoadExe().
 * @internal
 */
static int kldrDyldDoLoad(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                          unsigned fFlags, PPKLDRDYLDMOD ppMod, char *pszErr, KSIZE cchErr)
{
    int rc;

    /*
     * Try find the module among the ones that's already loaded.
     */
    rc = kldrDyldFindExistingModule(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, ppMod);
    if (!rc)
    {
        switch ((*ppMod)->enmState)
        {
            /*
             * Prerequisites are ok, so nothing to do really.
             */
            case KLDRSTATE_GOOD:
            case KLDRSTATE_INITIALIZING:
                return kldrDyldModDynamicLoad(*ppMod);

            /*
             * The module can't be loaded because it failed to initialize.
             */
            case KLDRSTATE_INITIALIZATION_FAILED:
                return KLDR_ERR_MODULE_INIT_FAILED_ALREADY;

            /*
             * Prerequisites needs loading / reattaching and the module
             * (may depending on fFlags) needs to be initialized.
             */
            case KLDRSTATE_PENDING_INITIALIZATION:
                break;

            /*
             * Prerequisites needs to be loaded again
             */
            case KLDRSTATE_PENDING_TERMINATION:
                break;

            /*
             * The module has been terminated so it need to be reloaded, have it's
             * prereqs loaded, fixed up and initialized before we can use it again.
             */
            case KLDRSTATE_PENDING_GC:
                rc = kldrDyldModReload(*ppMod);
                if (rc)
                    return kldrDyldCopyError(rc, pszErr, cchErr);
                break;

            /*
             * Forget it, we don't know how to deal with re-initialization here.
             */
            case KLDRSTATE_TERMINATING:
                KLDRDYLD_ASSERT(!"KLDR_ERR_MODULE_TERMINATING");
                return KLDR_ERR_MODULE_TERMINATING;

            /*
             * Invalid state.
             */
            default:
                KLDRDYLD_ASSERT(!"invalid state");
                break;
        }
    }
    else
    {
        /*
         * We'll have to load it from file.
         */
        rc = kldrDyldFindNewModule(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, ppMod);
        if (rc)
            return kldrDyldCopyError(rc, pszErr, cchErr);
        rc = kldrDyldModMap(*ppMod);
    }

    /*
     * Join cause with kLdrDyldLoadExe.
     */
    if (!rc)
        rc = kldrDyldDoLoad2(*ppMod, pszPrefix, pszSuffix, enmSearch, fFlags);
    else
        kldrDyldStackCleanupOne(*ppMod, rc);

    /*
     * Copy any error or warning to the error buffer.
     */
    return kldrDyldCopyError(rc, pszErr, cchErr);
}


/**
 * 2nd half of kLdrDyldLoad() and kLdrDyldLoadExe().
 *
 * @internal
 */
static int kldrDyldDoLoad2(PKLDRDYLDMOD pLoadedMod, const char *pszPrefix, const char *pszSuffix,
                           KLDRDYLDSEARCH enmSearch, unsigned fFlags)
{
    /*
     * Load prerequisites.
     */
    KU32 i;
    KU32 iLoad1st = kldrDyldStackNewFrame(pLoadedMod);
    int rc = kldrDyldDoLoadPrerequisites(pLoadedMod, pszPrefix, pszSuffix, enmSearch, fFlags);
    KU32 iLoadEnd = kldrDyldStackFrameCompleted();
    if (rc)
    {
        kldrDyldModAddRef(pLoadedMod);
        kldrDyldStackCleanupOne(pLoadedMod, rc); /* in case it didn't get pushed onto the stack. */
        kldrDyldModDeref(pLoadedMod);
    }

    /*
     * Apply fixups.
     */
    for (i = iLoad1st; !rc && i < iLoadEnd; i++)
    {
        PKLDRDYLDMOD pMod = g_papStackMods[i];
        if (    pMod->enmState == KLDRSTATE_LOADED_PREREQUISITES
            ||  pMod->enmState == KLDRSTATE_RELOADED_LOADED_PREREQUISITES)
            rc = kldrDyldModFixup(pMod);
    }

    /*
     * Advance fixed up module onto initialization.
     */
    for (i = iLoad1st; !rc && i < iLoadEnd; i++)
    {
        PKLDRDYLDMOD pMod = g_papStackMods[i];
        if (    pMod->enmState == KLDRSTATE_FIXED_UP
            ||  pMod->enmState == KLDRSTATE_RELOADED_FIXED_UP)
            pMod->enmState = KLDRSTATE_PENDING_INITIALIZATION;
        KLDRDYLD_ASSERT(    pMod->enmState == KLDRSTATE_PENDING_INITIALIZATION
                        ||  pMod->enmState == KLDRSTATE_GOOD);
    }

    /*
     * Call the initializers if we're loading in recursive mode or
     * if we're the outermost load call.
     */
    if (fFlags & KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT)
    {
        for (i = iLoad1st; !rc && i < iLoadEnd; i++)
        {
            PKLDRDYLDMOD pMod = g_papStackMods[i];
            if (pMod->enmState == KLDRSTATE_PENDING_INITIALIZATION)
                rc = kldrDyldModCallInit(pMod);
            else if (pMod->enmState == KLDRSTATE_INITIALIZATION_FAILED)
                rc = KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED_ALREADY;
            else
                KLDRDYLD_ASSERT(g_papStackMods[i]->enmState == KLDRSTATE_GOOD);
        }
#ifdef KLDRDYLD_STRICT
        for (i = iLoad1st; !rc && i < iLoadEnd; i++)
            KLDRDYLD_ASSERT(g_papStackMods[i]->enmState == KLDRSTATE_GOOD);
#endif
    }
    else if (g_cActiveLoadCalls <= 1)
    {
        while (!rc && g_pkLdrDyldInitHead)
        {
            PKLDRDYLDMOD pMod = g_pkLdrDyldInitHead;
            g_pkLdrDyldInitHead = pMod->InitTerm.pNext;
            if (pMod->InitTerm.pNext)
                pMod->InitTerm.pNext->InitTerm.pPrev = NULL;
            else
                g_pkLdrDyldInitTail = NULL;
            pMod->fInitList = 0;
            rc = kldrDyldModCallInit(pMod);
        }
    }

    /*
     * Complete the load by incrementing the dynamic load count of the
     * requested module (return handle is already set).
     */
    if (!rc)
    {
        if (fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE)
        {
            pLoadedMod->cDepRefs++; /* just make it stick. */
            pLoadedMod->cRefs++;
        }
        else
            rc = kldrDyldModDynamicLoad(pLoadedMod);
    }

    kldrDyldStackDropFrame(iLoad1st, iLoadEnd, rc);
    return rc;
}


/**
 * kldrDyldDoLoad() helper which will load prerequisites and
 * build the initialization array / list.
 *
 * @returns 0 on success, non-zero error code on failure.
 * @param   pMod            The module to start at.
 * @param   pszPrefix       Prefix to use when searching.
 * @param   pszSuffix       Suffix to use when searching.
 * @param   enmSearch       Method to use when locating the module and any modules it may depend on.
 * @param   fFlags          Flags, a combintation of the KLDRYDLD_LOAD_FLAGS_* \#defines.
 */
static int kldrDyldDoLoadPrerequisites(PKLDRDYLDMOD pMod, const char *pszPrefix, const char *pszSuffix,
                                       KLDRDYLDSEARCH enmSearch, unsigned fFlags)
{
    static struct
    {
        /** The module. */
        PKLDRDYLDMOD    pMod;
        /** The number of prerequisite modules left to process.
         * This starts at ~0U to inidicate that we need to load/check prerequisistes. */
        unsigned        cLeft;
    }               s_aEntries[64];
    unsigned        cEntries;
    int             rc = 0;

    /* Prerequisites are always global and they just aren't executables. */
    fFlags &= ~(KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE | KLDRDYLD_LOAD_FLAGS_EXECUTABLE);

    /* push the first entry. */
    s_aEntries[0].pMod = pMod;
    s_aEntries[0].cLeft = ~0U;
    cEntries = 1;

    /*
     * The recursion loop.
     */
    while (!rc && cEntries > 0)
    {
        const unsigned i = cEntries - 1;
        pMod = s_aEntries[i].pMod;
        if (s_aEntries[i].cLeft == ~0U)
        {
            /*
             * Load prerequisite modules.
             */
            switch (pMod->enmState)
            {
                /*
                 * Load immediate prerequisite modules and push the ones needing
                 * attention onto the stack.
                 */
                case KLDRSTATE_MAPPED:
                case KLDRSTATE_RELOADED:
                case KLDRSTATE_PENDING_TERMINATION:
                    rc = kldrDyldModLoadPrerequisites(pMod, pszPrefix, pszSuffix, enmSearch, fFlags);
                    KLDRDYLD_ASSERT(    pMod->enmState == KLDRSTATE_GOOD
                                    ||  pMod->enmState == KLDRSTATE_RELOADED_LOADED_PREREQUISITES
                                    ||  pMod->enmState == KLDRSTATE_LOADED_PREREQUISITES
                                    ||  rc);
                    if (!rc)
                        s_aEntries[i].cLeft = pMod->cPrereqs;
                    break;

                /*
                 * Check its prerequisite modules the first time around.
                 */
                case KLDRSTATE_PENDING_INITIALIZATION:
                    if (pMod->fAlreadySeen)
                        break;
                    pMod->fAlreadySeen = 1;
                    s_aEntries[i].cLeft = pMod->cPrereqs;
                    break;

                /*
                 * These are ok.
                 */
                case KLDRSTATE_LOADED_PREREQUISITES:
                case KLDRSTATE_RELOADED_LOADED_PREREQUISITES:
                case KLDRSTATE_INITIALIZING:
                case KLDRSTATE_GOOD:
                    s_aEntries[i].cLeft = 0;
                    break;

                /*
                 * All other stats are invalid.
                 */
                default:
                    KLDRDYLD_ASSERT(!"invalid state");
                    break;
            }
        }
        else if (s_aEntries[i].cLeft > 0)
        {
            /*
             * Recurse down into the next prereq.
             */
            KLDRDYLD_ASSERT(s_aEntries[i].cLeft <= pMod->cPrereqs);
            if (cEntries < sizeof(s_aEntries) / sizeof(s_aEntries[0]))
            {
                s_aEntries[cEntries].cLeft = ~(KU32)0;
                s_aEntries[cEntries].pMod = pMod->papPrereqs[pMod->cPrereqs - s_aEntries[i].cLeft];
                s_aEntries[i].cLeft--;
                cEntries++;
            }
            else
                rc = KLDR_ERR_PREREQUISITE_RECURSED_TOO_DEEPLY;
        }
        else
        {
            /*
             * We're done with this module, record it for init/cleanup.
             */
            cEntries--;
            if (pMod->enmState != KLDRSTATE_GOOD)
            {
                kldrDyldStackAddModule(pMod);
                if  (   !(fFlags & KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT)
                     && !pMod->fInitList)
                {
                    pMod->fInitList = 1;
                    pMod->InitTerm.pNext = NULL;
                    pMod->InitTerm.pPrev = g_pkLdrDyldInitTail;
                    if (g_pkLdrDyldInitTail)
                        g_pkLdrDyldInitTail->InitTerm.pNext = pMod;
                    else
                        g_pkLdrDyldInitHead = pMod;
                    g_pkLdrDyldInitTail = pMod;
                }
            }
        }
    }

    return rc;
}


/**
 * Gets prerequisite module.
 *
 * This will try load the requested module if necessary, returning it in the MAPPED state.
 *
 * @returns 0 on success.
 * @returns KLDR_ERR_MODULE_NOT_FOUND or I/O error on failure.
 * @param   pszDll          The name of the dll to look for.
 * @param   pszPrefix    Prefix than can be used when searching.
 * @param   pszSuffix    Suffix than can be used when searching.
 * @param   enmSearch       Method to use when locating the module.
 * @param   fFlags          Flags, a combintation of the KLDRYDLD_LOAD_FLAGS_* \#defines.
 * @param   pDep            The depentant module.
 * @param   ppMod           Where to put the module we get.
 */
int kldrDyldGetPrerequisite(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                            unsigned fFlags, PKLDRDYLDMOD pDep, PPKLDRDYLDMOD ppMod)
{
    int             rc;
    PKLDRDYLDMOD    pMod;

    *ppMod = NULL;

    /*
     * Try find the module among the ones that's already loaded.
     *
     * This is very similar to the kldrDyldDoLoad code, except it has to deal with
     * a couple of additional states and occurs only during prerequisite loading
     * and the action taken is a little bit different.
     */
    rc = kldrDyldFindExistingModule(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, &pMod);
    if (!rc)
    {
        switch (pMod->enmState)
        {
            /*
             * These are good.
             */
            case KLDRSTATE_MAPPED:
            case KLDRSTATE_RELOADED:
            case KLDRSTATE_LOADED_PREREQUISITES:
            case KLDRSTATE_RELOADED_LOADED_PREREQUISITES:
            case KLDRSTATE_PENDING_INITIALIZATION:
            case KLDRSTATE_INITIALIZING:
            case KLDRSTATE_GOOD:
            case KLDRSTATE_PENDING_TERMINATION:
                break;

            /*
             * The module has been terminated so it need to be reloaded, have it's
             * prereqs loaded, fixed up and initialized before we can use it again.
             */
            case KLDRSTATE_PENDING_GC:
                rc = kldrDyldModReload(pMod);
                break;

            /*
             * The module can't be loaded because it failed to initialize already.
             */
            case KLDRSTATE_INITIALIZATION_FAILED:
                rc = KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED;
                break;

            /*
             * Forget it, no idea how to deal with re-initialization.
             */
            case KLDRSTATE_TERMINATING:
                return KLDR_ERR_PREREQUISITE_MODULE_TERMINATING;

            /*
             * Invalid state.
             */
            default:
                KLDRDYLD_ASSERT(!"invalid state");
                break;
        }
    }
    else
    {
        /*
         * We'll have to load it from file.
         */
        rc = kldrDyldFindNewModule(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, &pMod);
        if (!rc)
            rc = kldrDyldModMap(pMod);
    }

    /*
     * On success add dependency.
     */
    if (!rc)
    {
        kldrDyldModAddDep(pMod, pDep);
        *ppMod = pMod;
    }
    return rc;
}


/**
 * Starts a new load stack frame.
 *
 * @returns Where the new stack frame starts.
 * @param   pLoadMod        The module being loaded (only used for asserting).
 */
static KU32 kldrDyldStackNewFrame(PKLDRDYLDMOD pLoadMod)
{
    /*
     * Clear the fAlreadySeen flags.
     */
    PKLDRDYLDMOD pMod = kLdrDyldHead;
    while (pMod)
    {
        pMod->fAlreadySeen = 0;

#ifdef KLDRDYLD_ASSERT
        switch (pMod->enmState)
        {
            case KLDRSTATE_MAPPED:
            case KLDRSTATE_RELOADED:
                /* only the just loaded module can be in this state. */
                KLDRDYLD_ASSERT(pMod == pLoadMod);
                break;

            case KLDRSTATE_PENDING_INITIALIZATION:
            case KLDRSTATE_INITIALIZING:
            case KLDRSTATE_PENDING_TERMINATION:
            case KLDRSTATE_PENDING_GC:
            case KLDRSTATE_TERMINATING:
            case KLDRSTATE_INITIALIZATION_FAILED:
            case KLDRSTATE_PENDING_DESTROY:
                /* requires recursion. */
                KLDRDYLD_ASSERT(g_cActiveLoadCalls + g_cActiveLoadCalls + g_fActiveGC > 1);
                break;

            case KLDRSTATE_GOOD:
                /* requires nothing. */
                break;

            default:
                KLDRDYLD_ASSERT(!"Invalid state");
                break;
        }
#endif

        /* next */
        pMod = pMod->Load.pNext;
    }
    return g_cStackMods;
}


/**
 * Records the module.
 *
 * @return 0 on success, KERR_NO_MEMORY if we can't expand the table.
 * @param   pMod        The module to record.
 */
static int kldrDyldStackAddModule(PKLDRDYLDMOD pMod)
{
    /*
     * Grow the stack if necessary.
     */
    if (g_cStackMods + 1 > g_cStackModsAllocated)
    {
        KU32 cNew = g_cStackModsAllocated ? g_cStackModsAllocated * 2 : 128;
        void *pvOld = g_papStackMods;
        void *pvNew = kHlpAlloc(cNew * sizeof(g_papStackMods[0]));
        if (!pvNew)
            return KERR_NO_MEMORY;
        kHlpMemCopy(pvNew, pvOld, g_cStackMods * sizeof(g_papStackMods[0]));
        g_papStackMods = (PPKLDRDYLDMOD)pvNew;
        kHlpFree(pvOld);
    }

    /*
     * Add a reference and push the module onto the stack.
     */
    kldrDyldModAddRef(pMod);
    g_papStackMods[g_cStackMods++] = pMod;
    return 0;
}


/**
 * The frame has been completed.
 *
 * @returns Where the frame ends.
 */
static int kldrDyldStackFrameCompleted(void)
{
    return g_cStackMods;
}


/**
 * Worker routine for kldrDyldStackDropFrame() and kldrDyldDoLoad().
 *
 * @param   pMod            The module to perform cleanups on.
 * @param   rc              Used for state verification.
 */
static void kldrDyldStackCleanupOne(PKLDRDYLDMOD pMod, int rc)
{
    switch (pMod->enmState)
    {
        /*
         * Just push it along to the PENDING_DESTROY state.
         */
        case KLDRSTATE_MAPPED:
            KLDRDYLD_ASSERT(rc);
            kldrDyldModUnmap(pMod);
            KLDRDYLD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_DESTROY);
            break;

        /*
         * Move back to PENDING_GC.
         */
        case KLDRSTATE_RELOADED:
            KLDRDYLD_ASSERT(rc);
            pMod->enmState = KLDRSTATE_PENDING_GC;
            break;

        /*
         * Unload prerequisites and unmap the modules.
         */
        case KLDRSTATE_LOADED_PREREQUISITES:
        case KLDRSTATE_FIXED_UP:
            KLDRDYLD_ASSERT(rc);
            kldrDyldModUnloadPrerequisites(pMod);
            KLDRDYLD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_DESTROY);
            kldrDyldModUnmap(pMod);
            KLDRDYLD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_DESTROY);
            break;

        /*
         * Unload prerequisites and push it back to PENDING_GC.
         */
        case KLDRSTATE_RELOADED_LOADED_PREREQUISITES:
        case KLDRSTATE_RELOADED_FIXED_UP:
            kldrDyldModUnloadPrerequisites(pMod);
            KLDRDYLD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_GC);
            break;

        /*
         * Nothing to do, just asserting sanity.
         */
        case KLDRSTATE_INITIALIZING:
            /* Implies there is another load going on. */
            KLDRDYLD_ASSERT(g_cActiveLoadCalls > 1);
            break;
        case KLDRSTATE_TERMINATING:
            /* GC in progress. */
            KLDRDYLD_ASSERT(g_fActiveGC);
            break;
        case KLDRSTATE_PENDING_TERMINATION:
        case KLDRSTATE_PENDING_INITIALIZATION:
        case KLDRSTATE_PENDING_GC:
        case KLDRSTATE_PENDING_DESTROY:
            KLDRDYLD_ASSERT(rc);
            break;
        case KLDRSTATE_GOOD:
            break;

        /*
         * Bad states.
         */
        default:
            KLDRDYLD_ASSERT(!"drop frame bad state (a)");
            break;
    }
}


/**
 * Done with the stack frame, dereference all the modules in it.
 *
 * @param   iLoad1st        The start of the stack frame.
 * @param   iLoadEnd        The end of the stack frame.
 * @param   rc              Used for state verification.
 */
static void kldrDyldStackDropFrame(KU32 iLoad1st, KU32 iLoadEnd, int rc)
{
    KU32 i;
    KLDRDYLD_ASSERT(iLoad1st <= g_cStackMods);
    KLDRDYLD_ASSERT(iLoadEnd == g_cStackMods);

    /*
     * First pass: Do all the cleanups we can, but don't destroy anything just yet.
     */
    i = iLoadEnd;
    while (i-- > iLoad1st)
    {
        PKLDRDYLDMOD pMod = g_papStackMods[i];
        kldrDyldStackCleanupOne(pMod, rc);
    }

    /*
     * Second pass: Release the references so modules pending destruction
     *              can be completely removed.
     */
    for (i = iLoad1st; i < iLoadEnd ; i++)
    {
        PKLDRDYLDMOD pMod = g_papStackMods[i];

        /*
         * Revalidate the module state.
         */
        switch (pMod->enmState)
        {
            case KLDRSTATE_INITIALIZING:
            case KLDRSTATE_TERMINATING:
            case KLDRSTATE_PENDING_TERMINATION:
            case KLDRSTATE_PENDING_INITIALIZATION:
            case KLDRSTATE_PENDING_GC:
            case KLDRSTATE_PENDING_DESTROY:
            case KLDRSTATE_GOOD:
                break;
            default:
                KLDRDYLD_ASSERT(!"drop frame bad state (b)");
                break;
        }

        /*
         * Release it.
         */
        kldrDyldModDeref(pMod);
    }

    /*
     * Drop the stack frame.
     */
    g_cStackMods = iLoad1st;
}


/**
 * Do garbage collection.
 *
 * This isn't doing anything unless it's called from the last
 * load or unload call.
 */
static void kldrDyldDoModuleTerminationAndGarabageCollection(void)
{
    PKLDRDYLDMOD pMod;

    /*
     * We don't do anything until we're got rid of all recursive calls.
     * This will ensure that we get the most optimal termination order and
     * that we don't unload anything too early.
     */
    if (g_cActiveLoadCalls || g_cActiveUnloadCalls || g_fActiveGC)
        return;
    g_fActiveGC = 1;

    do
    {
        /*
         * 1. Release prerequisites for any left over modules.
         */
        for (pMod = kLdrDyldHead; pMod; pMod = pMod->Load.pNext)
        {
            kldrDyldModAddRef(pMod);

            switch (pMod->enmState)
            {
                case KLDRSTATE_GOOD:
                case KLDRSTATE_PENDING_GC:
                case KLDRSTATE_PENDING_TERMINATION:
                    break;

                case KLDRSTATE_INITIALIZATION_FAILED: /* just in case */
                case KLDRSTATE_PENDING_INITIALIZATION:
                    kldrDyldModUnloadPrerequisites(pMod);
                    break;

                default:
                    KLDRDYLD_ASSERT(!"invalid GC state (a)");
                    break;
            }

            kldrDyldModDeref(pMod);
        }

        /*
         * 2. Do init calls until we encounter somebody calling load/unload.
         */
        for (pMod = g_pkLdrDyldTermHead; pMod; pMod = pMod->InitTerm.pNext)
        {
            int fRestart = 0;
            kldrDyldModAddRef(pMod);

            switch (pMod->enmState)
            {
                case KLDRSTATE_GOOD:
                case KLDRSTATE_PENDING_GC:
                    break;

                case KLDRSTATE_PENDING_TERMINATION:
                {
                    const KU32 cTotalLoadCalls = g_cTotalLoadCalls;
                    const KU32 cTotalUnloadCalls = g_cTotalUnloadCalls;
                    kldrDyldModCallTerm(pMod);
                    fRestart = cTotalLoadCalls != g_cTotalLoadCalls
                            || cTotalUnloadCalls != g_cTotalUnloadCalls;
                    break;
                }

                default:
                    KLDRDYLD_ASSERT(!"invalid GC state (b)");
                    break;
            }

            kldrDyldModDeref(pMod);
            if (fRestart)
                break;
        }
    } while (pMod);

    /*
     * Unmap and destroy modules pending for GC.
     */
    pMod = kLdrDyldHead;
    while (pMod)
    {
        PKLDRDYLDMOD pNext = pMod->Load.pNext;
        kldrDyldModAddRef(pMod);

        switch (pMod->enmState)
        {
            case KLDRSTATE_INITIALIZATION_FAILED:
            case KLDRSTATE_PENDING_GC:
                KLDRDYLD_ASSERT(!pMod->cDepRefs);
                KLDRDYLD_ASSERT(!pMod->cDynRefs);
                pMod->enmState = KLDRSTATE_GC;
                kldrDyldModUnmap(pMod);
                KLDRDYLD_ASSERT(pMod->enmState == KLDRSTATE_PENDING_DESTROY);
                kldrDyldModDestroy(pMod);
                KLDRDYLD_ASSERT(pMod->enmState == KLDRSTATE_DESTROYED);
                break;

            case KLDRSTATE_GOOD:
                break;
            default:
                KLDRDYLD_ASSERT(!"invalid GC state (c)");
                break;
        }

        kldrDyldModDeref(pMod);

        /* next */
        pMod = pNext;
    }

    g_fActiveGC = 0;
}


/**
 * Worker for kLdrDyldUnload().
 * @internal
 */
static int kldrDyldDoUnload(PKLDRDYLDMOD pMod)
{
    return kldrDyldModDynamicUnload(pMod);
}


/**
 * Worker for kLdrDyldFindByName().
 * @internal
 */
static int kldrDyldDoFindByName(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                                unsigned fFlags, PPKLDRDYLDMOD ppMod)
{
    return kldrDyldFindExistingModule(pszDll, pszPrefix, pszSuffix, enmSearch, fFlags, ppMod);
}


/**
 * Worker for kLdrDyldFindByAddress().
 * @internal
 */
static int kldrDyldDoFindByAddress(KUPTR Address, PPKLDRDYLDMOD ppMod, KU32 *piSegment, KUPTR *poffSegment)
{
    /* Scan the segments of each module in the load list. */
    PKLDRDYLDMOD pMod = kLdrDyldHead;
    while (pMod)
    {
        KU32 iSeg;
        for (iSeg = 0; iSeg < pMod->pMod->cSegments; iSeg++)
        {
            KLDRADDR off = (KLDRADDR)Address - pMod->pMod->aSegments[iSeg].MapAddress;
            if (off < pMod->pMod->aSegments[iSeg].cb)
            {
                *ppMod = pMod->hMod;
                if (piSegment)
                    *piSegment = iSeg;
                if (poffSegment)
                    *poffSegment = (KUPTR)off;
                return 0;
            }
        }

        /* next */
        pMod = pMod->Load.pNext;
    }

    return KLDR_ERR_MODULE_NOT_FOUND;
}


/**
 * Worker for kLdrDyldGetName().
 * @internal
 */
static int kldrDyldDoGetName(PKLDRDYLDMOD pMod, char *pszName, KSIZE cchName)
{
    return kldrDyldModGetName(pMod, pszName, cchName);
}


/**
 * Worker for kLdrDyldGetFilename().
 * @internal
 */
static int kldrDyldDoGetFilename(PKLDRDYLDMOD pMod, char *pszFilename, KSIZE cchFilename)
{
    return kldrDyldModGetFilename(pMod, pszFilename, cchFilename);
}


/**
 * Worker for kLdrDyldQuerySymbol().
 * @internal
 */
static int kldrDyldDoQuerySymbol(PKLDRDYLDMOD pMod, KU32 uSymbolOrdinal, const char *pszSymbolName, KUPTR *pValue, KU32 *pfKind)
{
    return kldrDyldModQuerySymbol(pMod, uSymbolOrdinal, pszSymbolName, pValue, pfKind);
}


/**
 * Panic / failure
 *
 * @returns rc if we're in a position where we can return.
 * @param   rc              Return code.
 * @param   pszFormat       Message string. Limited fprintf like formatted.
 * @param   ...             Message string arguments.
 */
int kldrDyldFailure(int rc, const char *pszFilename, ...)
{
    /** @todo print it. */
    if (g_fBootstrapping);
        kHlpExit(1);
    return rc;
}


/**
 * Copies the error string to the user buffer.
 *
 * @returns rc.
 * @param   rc      The status code.
 * @param   pszErr  Where to copy the error string to.
 * @param   cchErr  The size of the destination buffer.
 */
static int kldrDyldCopyError(int rc, char *pszErr, KSIZE cchErr)
{
    KSIZE  cchToCopy;

    /* if no error string, format the rc into a string. */
    if (!g_szkLdrDyldError[0] && rc)
        kHlpInt2Ascii(g_szkLdrDyldError, sizeof(g_szkLdrDyldError), rc, 10);

    /* copy it if we got something. */
    if (cchErr && pszErr && g_szkLdrDyldError[0])
    {
        cchToCopy = kHlpStrLen(g_szkLdrDyldError);
        if (cchToCopy >= cchErr)
            cchToCopy = cchErr - 1;
        kHlpMemCopy(pszErr, g_szkLdrDyldError, cchToCopy);
        pszErr[cchToCopy] = '\0';
    }

    return rc;
}

