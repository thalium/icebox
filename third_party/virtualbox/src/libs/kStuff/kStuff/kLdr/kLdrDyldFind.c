/* $Id: kLdrDyldFind.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader, File Searching Methods.
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

#if K_OS == K_OS_LINUX
# include <k/kHlpSys.h>

#elif K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>
# ifndef LIBPATHSTRICT
#  define LIBPATHSTRICT 3
# endif
  extern APIRET APIENTRY DosQueryHeaderInfo(HMODULE hmod, ULONG ulIndex, PVOID pvBuffer, ULONG cbBuffer, ULONG ulSubFunction);
# define QHINF_EXEINFO       1 /* NE exeinfo. */
# define QHINF_READRSRCTBL   2 /* Reads from the resource table. */
# define QHINF_READFILE      3 /* Reads from the executable file. */
# define QHINF_LIBPATHLENGTH 4 /* Gets the libpath length. */
# define QHINF_LIBPATH       5 /* Gets the entire libpath. */
# define QHINF_FIXENTRY      6 /* NE only */
# define QHINF_STE           7 /* NE only */
# define QHINF_MAPSEL        8 /* NE only */

#elif K_OS == K_OS_WINDOWS
# undef IMAGE_DOS_SIGNATURE
# undef IMAGE_NT_SIGNATURE
# include <Windows.h>

#endif


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def KLDRDYLDFIND_STRICT
 * Define KLDRDYLDFIND_STRICT to enabled strict checks in kLdrDyldFind. */
#define KLDRDYLDFIND_STRICT 1

/** @def KLDRDYLDFIND_ASSERT
 * Assert that an expression is true when KLDRDYLDFIND_STRICT is defined.
 */
#ifdef KLDRDYLDFIND_STRICT
# define KLDRDYLDFIND_ASSERT(expr)  kHlpAssert(expr)
#else
# define KLDRDYLDFIND_ASSERT(expr)  do {} while (0)
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Search arguments.
 * This avoids a bunch of unnecessary string lengths and calculations.
 */
typedef struct KLDRDYLDFINDARGS
{
    const char *pszName;
    KSIZE       cchName;

    const char *pszPrefix;
    KSIZE       cchPrefix;

    const char *pszSuffix;
    KSIZE       cchSuffix;

    KSIZE       cchMaxLength;

    KLDRDYLDSEARCH  enmSearch;
    KU32            fFlags;
    PPKRDR          ppRdr;
} KLDRDYLDFINDARGS, *PKLDRDYLDFINDARGS;

typedef const KLDRDYLDFINDARGS *PCKLDRDYLDFINDARGS;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** @name The kLdr search method parameters.
 * @{ */
/** The kLdr EXE search path.
 * During EXE searching the it's initialized with the values of the KLDR_PATH and
 * the PATH env.vars. Both ';' and ':' can be used as separators.
 */
char            kLdrDyldExePath[8192];
/** The kLdr DLL search path.
 * During initialization the KLDR_LIBRARY_PATH env.var. and the path in the
 * executable stub is appended. Both ';' and ':' can be used as separators.
 */
char            kLdrDyldLibraryPath[8192];
/** The kLdr application directory.
 * This is initialized when the executable is 'loaded' or by a kLdr user.
 */
char            kLdrDyldAppDir[260];
/** The default kLdr DLL prefix.
 * This is initialized with the KLDR_DEF_PREFIX env.var. + the prefix in the executable stub.
 */
char            kLdrDyldDefPrefix[16];
/** The default kLdr DLL suffix.
 * This is initialized with the KLDR_DEF_SUFFIX env.var. + the prefix in the executable stub.
 */
char            kLdrDyldDefSuffix[16];
/** @} */


/** @name The OS/2 search method parameters.
 * @{
 */
/** The OS/2 LIBPATH.
 * This is queried from the os2krnl on OS/2, while on other systems initialized using
 * the KLDR_OS2_LIBPATH env.var.
 */
char            kLdrDyldOS2Libpath[2048];
/** The OS/2 LIBPATHSTRICT ("T" or '\0').
 * This is queried from the os2krnl on OS/2, while on other systems initialized using
 * the KLDR_OS2_LIBPATHSTRICT env.var.
 */
char            kLdrDyldOS2LibpathStrict[8];
/** The OS/2 BEGINLIBPATH.
 * This is queried from the os2krnl on OS/2, while on other systems initialized using
 * the KLDR_OS2_BEGINLIBPATH env.var.
 */
char            kLdrDyldOS2BeginLibpath[2048];
/** The OS/2 ENDLIBPATH.
 * This is queried from the os2krnl on OS/2, while on other systems initialized using
 * the KLDR_OS2_ENDLIBPATH env.var.
 */
char            kLdrDyldOS2EndLibpath[2048];
/** @} */


/** @name The Windows search method parameters.
 * @{ */
/** The Windows application directory.
 * This is initialized when the executable is 'loaded' or by a kLdr user.
 */
char            kLdrDyldWindowsAppDir[260];
/** The Windows system directory.
 * This is queried from the Win32/64 subsystem on Windows, while on other systems
 * initialized using the KLDR_WINDOWS_SYSTEM_DIR env.var.
 */
char            kLdrDyldWindowsSystemDir[260];
/** The Windows directory.
 * This is queried from the Win32/64 subsystem on Windows, while on other systems
 * initialized using the KLDR_WINDOWS_DIR env.var.
 */
char            kLdrDyldWindowsDir[260];
/** The Windows path.
 * This is queried from the PATH env.var. on Windows, while on other systems
 * initialized using the KLDR_WINDOWS_PATH env.var. and falling back on
 * the PATH env.var. if it wasn't found.
 */
char            kLdrDyldWindowsPath[8192];
/** @} */


/** @name The Common Unix search method parameters.
 * @{
 */
/** The Common Unix library path.
 * Initialized from the env.var. KLDR_UNIX_LIBRARY_PATH or LD_LIBRARY_PATH or the
 * former wasn't found.
 */
char            kLdrDyldUnixLibraryPath[8192];
/** The Common Unix system library path. */
char            kLdrDyldUnixSystemLibraryPath[1024] = "/lib;/usr/lib";
/** @} */

/** @todo Deal with DT_RUNPATH and DT_RPATH. */
/** @todo ld.so.cache? */


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int kldrDyldFindDoDllSearch(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                                KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKRDR ppRdr);
static int kldrDyldFindDoExeSearch(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                                   KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKRDR ppRdr);
static int kldrDyldFindTryOpen(const char *pszFilename, PPKRDR ppRdr);
static int kldrDyldFindTryOpenPath(const char *pchPath, KSIZE cchPath, PCKLDRDYLDFINDARGS pArgs);
static int kldrDyldFindEnumeratePath(const char *pszSearchPath, PCKLDRDYLDFINDARGS pArgs);
static int kldrDyldFindGetDefaults(KLDRDYLDSEARCH *penmSearch, const char **pszPrefix,
                                   const char **pszSuffix, const char *pszName, KU32 fFlags);


/**
 * Initializes the find paths.
 *
 * @returns 0 on success, non-zero on failure.
 */
int kldrDyldFindInit(void)
{
    KSIZE   cch;
    int     rc;
    char    szTmp[sizeof(kLdrDyldDefSuffix)];

    /*
     * The kLdr search parameters.
     */
    rc = kHlpGetEnv("KLDR_LIBRARY_PATH", kLdrDyldLibraryPath, sizeof(kLdrDyldLibraryPath));
    rc = kHlpGetEnv("KLDR_DEF_PREFIX", szTmp, sizeof(szTmp));
    if (!rc)
        kHlpMemCopy(kLdrDyldDefPrefix, szTmp, sizeof(szTmp));
    rc = kHlpGetEnv("KLDR_DEF_SUFFIX", szTmp, sizeof(szTmp));
    if (!rc)
        kHlpMemCopy(kLdrDyldDefSuffix, szTmp, sizeof(szTmp));

    /*
     * The OS/2 search parameters.
     */
#if K_OS == K_OS_OS2
    rc = DosQueryHeaderInfo(NULLHANDLE, 0, kLdrDyldOS2Libpath, sizeof(kLdrDyldOS2Libpath), QHINF_LIBPATH);
    if (rc)
        return rc;
    rc = DosQueryExtLIBPATH((PSZ)kLdrDyldOS2LibpathStrict, LIBPATHSTRICT);
    if (rc)
        kLdrDyldOS2LibpathStrict[0] = '\0';
    rc = DosQueryExtLIBPATH((PSZ)kLdrDyldOS2BeginLibpath, BEGIN_LIBPATH);
    if (rc)
        kLdrDyldOS2BeginLibpath[0] = '\0';
    rc = DosQueryExtLIBPATH((PSZ)kLdrDyldOS2EndLibpath, END_LIBPATH);
    if (rc)
        kLdrDyldOS2EndLibpath[0] = '\0';

#else
    kHlpGetEnv("KLDR_OS2_LIBPATH", kLdrDyldOS2Libpath, sizeof(kLdrDyldOS2Libpath));
    kHlpGetEnv("KLDR_OS2_LIBPATHSTRICT", kLdrDyldOS2LibpathStrict, sizeof(kLdrDyldOS2LibpathStrict));
    if (    kLdrDyldOS2LibpathStrict[0] == 'T'
        ||  kLdrDyldOS2LibpathStrict[0] == 't')
        kLdrDyldOS2LibpathStrict[0] = 'T';
    else
        kLdrDyldOS2LibpathStrict[0] = '\0';
    kLdrDyldOS2LibpathStrict[1] = '\0';
    kHlpGetEnv("KLDR_OS2_BEGINLIBPATH", kLdrDyldOS2BeginLibpath, sizeof(kLdrDyldOS2BeginLibpath));
    kHlpGetEnv("KLDR_OS2_ENDLIBPATH", kLdrDyldOS2EndLibpath, sizeof(kLdrDyldOS2EndLibpath));
#endif

    /*
     * The windows search parameters.
     */
#if K_OS == K_OS_WINDOWS
    cch = GetSystemDirectory(kLdrDyldWindowsSystemDir, sizeof(kLdrDyldWindowsSystemDir));
    if (cch >= sizeof(kLdrDyldWindowsSystemDir))
        return (rc = GetLastError()) ? rc : -1;
    cch = GetWindowsDirectory(kLdrDyldWindowsDir, sizeof(kLdrDyldWindowsDir));
    if (cch >= sizeof(kLdrDyldWindowsDir))
        return (rc = GetLastError()) ? rc : -1;
    kHlpGetEnv("PATH", kLdrDyldWindowsPath, sizeof(kLdrDyldWindowsPath));
#else
    kHlpGetEnv("KLDR_WINDOWS_SYSTEM_DIR", kLdrDyldWindowsSystemDir, sizeof(kLdrDyldWindowsSystemDir));
    kHlpGetEnv("KLDR_WINDOWS_DIR", kLdrDyldWindowsDir, sizeof(kLdrDyldWindowsDir));
    rc = kHlpGetEnv("KLDR_WINDOWS_PATH", kLdrDyldWindowsPath, sizeof(kLdrDyldWindowsPath));
    if (rc)
        kHlpGetEnv("PATH", kLdrDyldWindowsPath, sizeof(kLdrDyldWindowsPath));
#endif

    /*
     * The Unix search parameters.
     */
    rc = kHlpGetEnv("KLDR_UNIX_LIBRARY_PATH", kLdrDyldUnixLibraryPath, sizeof(kLdrDyldUnixLibraryPath));
    if (rc)
        kHlpGetEnv("LD_LIBRARY_PATH", kLdrDyldUnixLibraryPath, sizeof(kLdrDyldUnixLibraryPath));

    (void)cch;
    return 0;
}


/**
 * Lazily initialize the two application directory paths.
 */
static void kldrDyldFindLazyInitAppDir(void)
{
    if (!kLdrDyldAppDir[0])
    {
#if K_OS == K_OS_DARWIN
        /** @todo implement this! */
        kLdrDyldWindowsAppDir[0] = kLdrDyldAppDir[0] = '.';
        kLdrDyldWindowsAppDir[1] = kLdrDyldAppDir[1] = '\0';

#elif K_OS == K_OS_LINUX
        KSSIZE cch = kHlpSys_readlink("/proc/self/exe", kLdrDyldAppDir, sizeof(kLdrDyldAppDir) - 1);
        if (cch > 0)
        {
            kLdrDyldAppDir[cch] = '\0';
            *kHlpGetFilename(kLdrDyldAppDir) = '\0';
            kHlpMemCopy(kLdrDyldWindowsAppDir, kLdrDyldAppDir, sizeof(kLdrDyldAppDir));
        }
        else
        {
            kLdrDyldWindowsAppDir[0] = kLdrDyldAppDir[0] = '.';
            kLdrDyldWindowsAppDir[1] = kLdrDyldAppDir[1] = '\0';
        }

#elif K_OS == K_OS_OS2
        PPIB pPib;
        PTIB pTib;
        APIRET rc;

        DosGetInfoBlocks(&pTib, &pPib);
        rc = DosQueryModuleName(pPib->pib_hmte, sizeof(kLdrDyldAppDir), kLdrDyldAppDir);
        if (!rc)
        {
            *kHlpGetFilename(kLdrDyldAppDir) = '\0';
             kHlpMemCopy(kLdrDyldWindowsAppDir, kLdrDyldAppDir, sizeof(kLdrDyldAppDir));
        }
        else
        {
            kLdrDyldWindowsAppDir[0] = kLdrDyldAppDir[0] = '.';
            kLdrDyldWindowsAppDir[1] = kLdrDyldAppDir[1] = '\0';
        }

#elif K_OS == K_OS_WINDOWS
        DWORD dwSize = GetModuleFileName(NULL /* the executable */, kLdrDyldAppDir, sizeof(kLdrDyldAppDir));
        if (dwSize > 0)
        {
            *kHlpGetFilename(kLdrDyldAppDir) = '\0';
            kHlpMemCopy(kLdrDyldWindowsAppDir, kLdrDyldAppDir, sizeof(kLdrDyldAppDir));
        }
        else
        {
            kLdrDyldWindowsAppDir[0] = kLdrDyldAppDir[0] = '.';
            kLdrDyldWindowsAppDir[1] = kLdrDyldAppDir[1] = '\0';
        }

#else
# error "Port me"
#endif
    }
}


/**
 * Locates and opens a module using the specified search method.
 *
 * @returns 0 and *ppMod on success, non-zero OS specific error on failure.
 *
 * @param   pszName         Partial or complete name, it's specific to the search method to determin which.
 * @param   pszPrefix       Prefix than can be used when searching.
 * @param   pszSuffix       Suffix than can be used when searching.
 * @param   enmSearch       The file search method to apply.
 * @param   fFlags          Search flags.
 * @param   ppMod           Where to store the file provider instance on success.
 */
int kldrDyldFindNewModule(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                          KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKLDRDYLDMOD ppMod)
{
    int rc;
    PKRDR pRdr = NULL;

    *ppMod = NULL;

    /*
     * If this isn't just a filename, we the caller has specified a file
     * that should be opened directly and not a module name to be searched for.
     */
    if (!kHlpIsFilenameOnly(pszName))
        rc = kldrDyldFindTryOpen(pszName, &pRdr);
    else if (!(fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE))
        rc = kldrDyldFindDoDllSearch(pszName, pszPrefix, pszSuffix, enmSearch, fFlags, &pRdr);
    else
        rc = kldrDyldFindDoExeSearch(pszName, pszPrefix, pszSuffix, enmSearch, fFlags, &pRdr);
    if (!rc)
    {
#ifdef KLDRDYLDFIND_STRICT
        /* Sanity check of kldrDyldFindExistingModule. */
        if (fFlags & KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE)
        {
            const char     *pszFilename = kRdrName(pRdr);
            const KSIZE     cchFilename = kHlpStrLen(pszFilename);
            PKLDRDYLDMOD    pCur;
            for (pCur = kLdrDyldHead; pCur; pCur = pCur->Load.pNext)
                KLDRDYLDFIND_ASSERT(    pCur->pMod->cchFilename != cchFilename
                                    ||  kHlpMemComp(pCur->pMod->pszFilename, pszFilename, cchFilename));
        }
#endif

        /*
         * Check for matching non-global modules that should be promoted.
         */
        if (!(fFlags & KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE))
        {
            const char     *pszFilename = kRdrName(pRdr);
            const KSIZE     cchFilename = kHlpStrLen(pszFilename);
            PKLDRDYLDMOD    pCur;
            for (pCur = kLdrDyldHead; pCur; pCur = pCur->Load.pNext)
            {
                if (    !pCur->fGlobalOrSpecific
                    &&  pCur->pMod->cchFilename == cchFilename
                    &&  !kHlpMemComp(pCur->pMod->pszFilename, pszFilename, cchFilename))
                {
                    kRdrClose(pRdr);
                    kldrDyldModMarkGlobal(pCur);
                    *ppMod = pCur;
                    return 0;
                }
                KLDRDYLDFIND_ASSERT(    pCur->pMod->cchFilename != cchFilename
                                    ||  kHlpMemComp(pCur->pMod->pszFilename, pszFilename, cchFilename));
            }
        }

        /*
         * Create a new module.
         */
        rc = kldrDyldModCreate(pRdr, fFlags, ppMod);
        if (rc)
            kRdrClose(pRdr);
    }
    return rc;
}


/**
 * Searches for a DLL file using the specified method.
 *
 * @returns 0 on success and *ppMod pointing to the new module.
 * @returns KLDR_ERR_MODULE_NOT_FOUND if the specified file couldn't be opened.
 * @returns non-zero kLdr or OS specific status code on other failures.
 * @param   pszName     The name.
 * @param   pszPrefix   The prefix, optional.
 * @param   pszSuffix   The suffix, optional.
 * @param   enmSearch   The search method.
 * @param   fFlags      The load/search flags.
 * @param   ppRdr       Where to store the pointer to the file provider instance on success.
 */
static int kldrDyldFindDoDllSearch(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                                   KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKRDR ppRdr)
{
    int rc;
    KLDRDYLDFINDARGS Args;

    /*
     * Initialize the argument structure and resolve defaults.
     */
    Args.enmSearch = enmSearch;
    Args.pszPrefix = pszPrefix;
    Args.pszSuffix = pszSuffix;
    rc = kldrDyldFindGetDefaults(&Args.enmSearch, &Args.pszPrefix, &Args.pszSuffix, pszName, fFlags);
    if (rc)
        return rc;
    Args.pszName = pszName;
    Args.cchName = kHlpStrLen(pszName);
    Args.cchPrefix = Args.pszPrefix ? kHlpStrLen(Args.pszPrefix) : 0;
    Args.cchSuffix = Args.pszSuffix ? kHlpStrLen(Args.pszSuffix) : 0;
    Args.cchMaxLength = Args.cchName + Args.cchSuffix + Args.cchPrefix;
    Args.fFlags = fFlags;
    Args.ppRdr = ppRdr;

    /*
     * Apply the specified search method.
     */
/** @todo get rid of the strlen() on the various paths here! */
    switch (Args.enmSearch)
    {
        case KLDRDYLD_SEARCH_KLDR:
        {
            kldrDyldFindLazyInitAppDir();
            if (kLdrDyldAppDir[0] != '\0')
            {
                rc = kldrDyldFindTryOpenPath(kLdrDyldAppDir, kHlpStrLen(kLdrDyldAppDir), &Args);
                if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                    break;
            }
            rc = kldrDyldFindTryOpenPath(".", 1, &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                break;
            rc = kldrDyldFindEnumeratePath(kLdrDyldLibraryPath, &Args);
            break;
        }

        case KLDRDYLD_SEARCH_OS2:
        {
            rc = kldrDyldFindEnumeratePath(kLdrDyldOS2BeginLibpath, &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                break;
            rc = kldrDyldFindEnumeratePath(kLdrDyldOS2Libpath, &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                break;
            rc = kldrDyldFindEnumeratePath(kLdrDyldOS2EndLibpath, &Args);
            break;
        }

        case KLDRDYLD_SEARCH_WINDOWS:
        case KLDRDYLD_SEARCH_WINDOWS_ALTERED:
        {
            kldrDyldFindLazyInitAppDir();
            rc = kldrDyldFindTryOpenPath(kLdrDyldWindowsAppDir, kHlpStrLen(kLdrDyldWindowsAppDir), &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                break;
            if (Args.enmSearch == KLDRDYLD_SEARCH_WINDOWS_ALTERED)
            {
                rc = kldrDyldFindTryOpenPath(".", 1, &Args);
                if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                    break;
            }
            rc = kldrDyldFindTryOpenPath(kLdrDyldWindowsSystemDir, kHlpStrLen(kLdrDyldWindowsSystemDir), &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                break;
            rc = kldrDyldFindTryOpenPath(kLdrDyldWindowsDir, kHlpStrLen(kLdrDyldWindowsDir), &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                break;
            if (Args.enmSearch == KLDRDYLD_SEARCH_WINDOWS)
            {
                rc = kldrDyldFindTryOpenPath(".", 1, &Args);
                if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                    break;
            }
            rc = kldrDyldFindEnumeratePath(kLdrDyldWindowsPath, &Args);
            break;
        }

        case KLDRDYLD_SEARCH_UNIX_COMMON:
        {
            rc = kldrDyldFindEnumeratePath(kLdrDyldUnixLibraryPath, &Args);
            if (rc == KLDR_ERR_MODULE_NOT_FOUND)
                break;
            rc = kldrDyldFindEnumeratePath(kLdrDyldUnixSystemLibraryPath, &Args);
            break;
        }

        default: kHlpAssert(!"internal error"); return -1;
    }
    return rc;
}


/**
 * Searches for an EXE file using the specified method.
 *
 * @returns 0 on success and *ppMod pointing to the new module.
 * @returns KLDR_ERR_MODULE_NOT_FOUND if the specified file couldn't be opened.
 * @returns non-zero kLdr or OS specific status code on other failures.
 * @param   pszName     The name.
 * @param   pszPrefix   The prefix, optional.
 * @param   pszSuffix   The suffix, optional.
 * @param   enmSearch   The search method.
 * @param   fFlags      The load/search flags.
 * @param   ppRdr       Where to store the pointer to the file provider instance on success.
 */
static int kldrDyldFindDoExeSearch(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                                   KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKRDR ppRdr)
{
    int rc;
    KLDRDYLDFINDARGS Args;

    /*
     * Initialize the argument structure and resolve defaults.
     */
    Args.enmSearch = enmSearch;
    Args.pszPrefix = pszPrefix;
    Args.pszSuffix = pszSuffix;
    rc = kldrDyldFindGetDefaults(&Args.enmSearch, &Args.pszPrefix, &Args.pszSuffix, pszName, fFlags);
    if (rc)
        return rc;
    Args.pszName = pszName;
    Args.cchName = kHlpStrLen(pszName);
    Args.cchPrefix = Args.pszPrefix ? kHlpStrLen(Args.pszPrefix) : 0;
    Args.cchSuffix = Args.pszSuffix ? kHlpStrLen(Args.pszSuffix) : 0;
    Args.cchMaxLength = Args.cchName + Args.cchSuffix + Args.cchPrefix;
    Args.fFlags = fFlags;
    Args.ppRdr = ppRdr;

    /*
     * If we're bootstrapping a process, we'll start by looking in the
     * application directory and the check out the path.
     */
    if (g_fBootstrapping)
    {
        kldrDyldFindLazyInitAppDir();
        if (kLdrDyldAppDir[0] != '\0')
        {
            rc = kldrDyldFindTryOpenPath(kLdrDyldAppDir, kHlpStrLen(kLdrDyldAppDir), &Args);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                return rc;
        }
    }

    /*
     * Search the EXE search path. Initialize it the first time around.
     */
    if (!kLdrDyldExePath[0])
    {
        KSIZE cch;
        kHlpGetEnv("KLDR_EXE_PATH", kLdrDyldExePath, sizeof(kLdrDyldExePath) - 10);
        cch = kHlpStrLen(kLdrDyldExePath);
        kLdrDyldExePath[cch++] = ';';
        kHlpGetEnv("PATH", &kLdrDyldExePath[cch], sizeof(kLdrDyldExePath) - cch);
    }
    return kldrDyldFindEnumeratePath(kLdrDyldExePath, &Args);
}


/**
 * Try open the specfied file.
 *
 * @returns 0 on success and *ppMod pointing to the new module.
 * @returns KLDR_ERR_MODULE_NOT_FOUND if the specified file couldn't be opened.
 * @returns non-zero kLdr or OS specific status code on other failures.
 * @param   pszFilename     The filename.
 * @param   ppRdr           Where to store the pointer to the new module.
 */
static int kldrDyldFindTryOpen(const char *pszFilename, PPKRDR ppRdr)
{
    int rc;

    /*
     * Try open the file.
     */
    rc = kRdrOpen(ppRdr, pszFilename);
    if (!rc)
        return 0;
    /** @todo deal with return codes properly. */
    if (rc >= KERR_BASE && rc <= KERR_END)
        return rc;

    return KLDR_ERR_MODULE_NOT_FOUND;
}


/**
 * Composes a filename from the specified directory path,
 * prefix (optional), name and suffix (optional, will try with and without).
 *
 * @param   pchPath     The directory path - this doesn't have to be null terminated.
 * @param   cchPath     The length of the path.
 * @param   pArgs       The search argument structure.
 *
 * @returns See kldrDyldFindTryOpen
 */
static int kldrDyldFindTryOpenPath(const char *pchPath, KSIZE cchPath, PCKLDRDYLDFINDARGS pArgs)
{
    static char s_szFilename[1024];
    char *psz;
    int rc;

    /*
     * Ignore any attempts at opening empty paths.
     * This can happen when a *Dir globals is empty.
     */
    if (!cchPath)
        return KLDR_ERR_MODULE_NOT_FOUND; /* ignore */

    /*
     * Limit check first.
     */
    if (cchPath + 1 + pArgs->cchMaxLength >= sizeof(s_szFilename))
    {
        KLDRDYLDFIND_ASSERT(!"too long");
        return KLDR_ERR_MODULE_NOT_FOUND; /* ignore */
    }

    /*
     * The directory path.
     */
    kHlpMemCopy(s_szFilename, pchPath, cchPath);
    psz = &s_szFilename[cchPath];
    if (psz[-1] != '/'
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
        && psz[-1] != '\\'
        && psz[-1] != ':'
#endif
        )
        *psz++ = '/';

    /*
     * The name.
     */
    if (pArgs->cchPrefix)
    {
        kHlpMemCopy(psz, pArgs->pszPrefix, pArgs->cchPrefix);
        psz += pArgs->cchPrefix;
    }
    kHlpMemCopy(psz, pArgs->pszName, pArgs->cchName);
    psz += pArgs->cchName;
    if (pArgs->cchSuffix)
    {
        kHlpMemCopy(psz, pArgs->pszSuffix, pArgs->cchSuffix);
        psz += pArgs->cchSuffix;
    }
    *psz = '\0';


    /*
     * Try open it.
     */
    rc = kldrDyldFindTryOpen(s_szFilename, pArgs->ppRdr);
    /* If we're opening an executable, try again without the suffix.*/
    if (    rc
        &&  pArgs->cchSuffix
        &&  (pArgs->fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE))
    {
        psz -= pArgs->cchSuffix;
        *psz = '\0';
        rc = kldrDyldFindTryOpen(s_szFilename, pArgs->ppRdr);
    }
    return rc;
}


/**
 * Enumerates the specfied path.
 *
 * @returns Any return code from the kldrDyldFindTryOpenPath() which isn't KLDR_ERR_MODULE_NOT_FOUND.
 * @returns KLDR_ERR_MODULE_NOT_FOUND if the end of the search path was reached.
 * @param   pszSearchPath   The search path to enumeare.
 * @param   pArgs       The search argument structure.
 */
static int kldrDyldFindEnumeratePath(const char *pszSearchPath, PCKLDRDYLDFINDARGS pArgs)
{
    const char *psz = pszSearchPath;
    for (;;)
    {
        const char *pszEnd;
        KSIZE       cchPath;

        /*
         * Trim.
         */
        while (*psz == ';' || *psz == ':')
            psz++;
        if (*psz == '\0')
            return KLDR_ERR_MODULE_NOT_FOUND;

        /*
         * Find the end.
         */
        pszEnd = psz + 1;
        while (     *pszEnd != '\0'
               &&   *pszEnd != ';'
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
               && (     *pszEnd != ':'
                   ||   (   pszEnd - psz == 1
                         && (   (*psz >= 'A' && *psz <= 'Z')
                             || (*psz >= 'a' && *psz <= 'z')
                            )
                        )
                  )
#else
               && *pszEnd != ':'
#endif
              )
            pszEnd++;

        /*
         * If not empty path, try open the module using it.
         */
        cchPath = pszEnd - psz;
        if (cchPath > 0)
        {
            int rc;
            rc = kldrDyldFindTryOpenPath(psz, cchPath, pArgs);
            if (rc != KLDR_ERR_MODULE_NOT_FOUND)
                return rc;
        }

        /* next */
        psz = pszEnd;
    }
}


/**
 * Resolve default search method, prefix and suffix.
 *
 * @returns 0 on success, KERR_INVALID_PARAMETER on failure.
 * @param   penmSearch  The search method. In/Out.
 * @param   ppszPrefix  The prefix. In/Out.
 * @param   ppszSuffix  The suffix. In/Out.
 * @param   pszName     The name. In.
 * @param   fFlags      The load/search flags.
 */
static int kldrDyldFindGetDefaults(KLDRDYLDSEARCH *penmSearch, const char **ppszPrefix, const char **ppszSuffix,
                                   const char *pszName, KU32 fFlags)
{
    unsigned fCaseSensitive;

    /*
     * Fixup search method alias.
     */
    if (*penmSearch == KLDRDYLD_SEARCH_HOST)
#if K_OS == K_OS_DARWIN
        /** @todo *penmSearch = KLDRDYLD_SEARCH_DARWIN; */
        *penmSearch = KLDRDYLD_SEARCH_UNIX_COMMON;
#elif K_OS == K_OS_FREEBSD \
   || K_OS == K_OS_LINUX \
   || K_OS == K_OS_NETBSD \
   || K_OS == K_OS_OPENBSD \
   || K_OS == K_OS_SOLARIS
        *penmSearch = KLDRDYLD_SEARCH_UNIX_COMMON;
#elif K_OS == K_OS_OS2
        *penmSearch = KLDRDYLD_SEARCH_OS2;
#elif K_OS == K_OS_WINDOWS
        *penmSearch = KLDRDYLD_SEARCH_WINDOWS;
#else
# error "Port me"
#endif

    /*
     * Apply search method specific prefix/suffix.
     */
    switch (*penmSearch)
    {
        case KLDRDYLD_SEARCH_KLDR:
            if (!*ppszPrefix && kLdrDyldDefPrefix[0])
                *ppszPrefix = kLdrDyldDefPrefix;
            if (!*ppszSuffix && kLdrDyldDefSuffix[0])
                *ppszSuffix = kLdrDyldDefSuffix;
            fCaseSensitive = 1;
            break;

        case KLDRDYLD_SEARCH_OS2:
            if (!*ppszSuffix)
                *ppszSuffix = !(fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE) ? ".dll" : ".exe";
            fCaseSensitive = 0;
            break;

        case KLDRDYLD_SEARCH_WINDOWS:
        case KLDRDYLD_SEARCH_WINDOWS_ALTERED:
            if (!*ppszSuffix)
                *ppszSuffix = !(fFlags & KLDRDYLD_LOAD_FLAGS_EXECUTABLE) ? ".dll" : ".exe";
            fCaseSensitive = 0;
            break;

        case KLDRDYLD_SEARCH_UNIX_COMMON:
            fCaseSensitive = 1;
            break;

        default:
            KLDRDYLDFIND_ASSERT(!"invalid search method");
            return KERR_INVALID_PARAMETER;
    }

    /*
     * Drop the suffix if it's already included in the name.
     */
    if (*ppszSuffix)
    {
        const KSIZE cchName = kHlpStrLen(pszName);
        const KSIZE cchSuffix = kHlpStrLen(*ppszSuffix);
        if (    cchName > cchSuffix
            &&  (   fCaseSensitive
                 ?  !kHlpMemComp(pszName + cchName - cchSuffix, *ppszSuffix, cchSuffix)
                 :  !kHlpMemICompAscii(pszName + cchName - cchSuffix, *ppszSuffix, cchSuffix))
           )
            *ppszSuffix = NULL;
    }

    return 0;
}


/**
 * Locates an already open module using the specified search method.
 *
 * @returns 0 and *ppMod on success, non-zero OS specific error on failure.
 *
 * @param   pszName         Partial or complete name, it's specific to the search method to determin which.
 * @param   pszPrefix       Prefix than can be used when searching.
 * @param   pszSuffix       Suffix than can be used when searching.
 * @param   enmSearch       The file search method to apply.
 * @param   fFlags          Search flags.
 * @param   ppMod           Where to store the file provider instance on success.
 */
int kldrDyldFindExistingModule(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                               KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKLDRDYLDMOD ppMod)
{

    int rc;
    unsigned fOS2LibpathStrict;
    *ppMod = NULL;

    /*
     * Don't bother if no modules are loaded yet.
     */
    if (!kLdrDyldHead)
        return KLDR_ERR_MODULE_NOT_FOUND;

    /*
     * Defaults.
     */
    rc = kldrDyldFindGetDefaults(&enmSearch, &pszPrefix, &pszSuffix, pszName, fFlags);
    if (rc)
        return rc;

    /*
     * If this isn't just a filename, the caller has specified a file
     * that should be opened directly and not a module name to be searched for.
     *
     * In order to do the right thing we'll have to open the file and get the
     * correct filename for it.
     *
     * The OS/2 libpath strict method require us to find the correct DLL first.
     */
    fOS2LibpathStrict = 0;
    if (    !kHlpIsFilenameOnly(pszName)
        ||  (fOS2LibpathStrict = (   enmSearch == KLDRDYLD_SEARCH_OS2
                                  && kLdrDyldOS2LibpathStrict[0] == 'T')
            )
       )
    {
        PKRDR pRdr;
        if (fOS2LibpathStrict)
            rc = kldrDyldFindDoDllSearch(pszName, pszPrefix, pszSuffix, enmSearch, fFlags, &pRdr);
        else
            rc = kldrDyldFindTryOpen(pszName, &pRdr);
        if (!rc)
        {
            /* do a filename based search. */
            const char     *pszFilename = kRdrName(pRdr);
            const KSIZE     cchFilename = kHlpStrLen(pszFilename);
            PKLDRDYLDMOD    pCur;
            rc = KLDR_ERR_MODULE_NOT_FOUND;
            for (pCur = kLdrDyldHead; pCur; pCur = pCur->Load.pNext)
            {
                if (    pCur->pMod->cchFilename == cchFilename
                    &&  !kHlpMemComp(pCur->pMod->pszFilename, pszFilename, cchFilename))
                {
                    *ppMod = pCur;
                    rc = 0;
                    break;
                }
            }
            kRdrClose(pRdr);
        }
    }
    else
    {
        const KSIZE     cchName = kHlpStrLen(pszName);
        const KSIZE     cchPrefix = pszPrefix ? kHlpStrLen(pszPrefix) : 0;
        const KSIZE     cchSuffix = pszSuffix ? kHlpStrLen(pszSuffix) : 0;
        const char     *pszNameSuffix = kHlpGetSuff(pszName);
        PKLDRDYLDMOD    pCur = kLdrDyldHead;

        /*
         * Some of the methods are case insensitive (ASCII), others are case sensitive.
         * To avoid having todo indirect calls to the compare functions here, we split
         * ways even if it means a lot of duplicate code.
         */
        if (   enmSearch == KLDRDYLD_SEARCH_OS2
            || enmSearch == KLDRDYLD_SEARCH_WINDOWS
            || enmSearch == KLDRDYLD_SEARCH_WINDOWS_ALTERED)
        {
            const unsigned fNameHasSuffix = pszNameSuffix
                                         && kHlpStrLen(pszNameSuffix) == cchSuffix
                                         && !kHlpMemICompAscii(pszNameSuffix, pszName + cchName - cchSuffix, cchSuffix);
            for (; pCur; pCur = pCur->Load.pNext)
            {
                /* match global / specific */
                if (    !pCur->fGlobalOrSpecific
                    &&  !(fFlags & KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE))
                    continue;

                /* match name */
                if (    pCur->pMod->cchName == cchName
                    &&  !kHlpMemICompAscii(pCur->pMod->pszName, pszName, cchName))
                    break;
                if (cchPrefix)
                {
                    if (    pCur->pMod->cchName == cchName + cchPrefix
                        &&  !kHlpMemICompAscii(pCur->pMod->pszName, pszPrefix, cchPrefix)
                        &&  !kHlpMemICompAscii(pCur->pMod->pszName + cchPrefix, pszName, cchName))
                    break;
                }
                if (cchSuffix)
                {
                    if (    pCur->pMod->cchName == cchName + cchSuffix
                        &&  !kHlpMemICompAscii(pCur->pMod->pszName + cchName, pszSuffix, cchSuffix)
                        &&  !kHlpMemICompAscii(pCur->pMod->pszName, pszName, cchName))
                    break;
                    if (    fNameHasSuffix
                        &&  pCur->pMod->cchName == cchName - cchSuffix
                        &&  !kHlpMemICompAscii(pCur->pMod->pszName, pszName, cchName - cchSuffix))
                    break;
                    if (cchPrefix)
                    {
                        if (    pCur->pMod->cchName == cchName + cchPrefix + cchSuffix
                            &&  !kHlpMemICompAscii(pCur->pMod->pszName, pszPrefix, cchPrefix)
                            &&  !kHlpMemICompAscii(pCur->pMod->pszName + cchPrefix, pszName, cchName)
                            &&  !kHlpMemICompAscii(pCur->pMod->pszName + cchPrefix + cchName, pszSuffix, cchSuffix))
                        break;
                        if (    fNameHasSuffix
                            &&  pCur->pMod->cchName == cchName + cchPrefix - cchSuffix
                            &&  !kHlpMemICompAscii(pCur->pMod->pszName, pszPrefix, cchPrefix)
                            &&  !kHlpMemICompAscii(pCur->pMod->pszName + cchPrefix, pszName, cchName - cchSuffix))
                        break;
                    }
                }
            }
        }
        else
        {
            const unsigned fNameHasSuffix = pszNameSuffix
                                         && kHlpStrLen(pszNameSuffix) == cchSuffix
                                         && kHlpMemComp(pszNameSuffix, pszName + cchName - cchSuffix, cchSuffix);
            for (; pCur; pCur = pCur->Load.pNext)
            {
                /* match global / specific */
                if (    !pCur->fGlobalOrSpecific
                    &&  !(fFlags & KLDRYDLD_LOAD_FLAGS_SPECIFIC_MODULE))
                    continue;

                /* match name */
                if (    pCur->pMod->cchName == cchName
                    &&  !kHlpMemComp(pCur->pMod->pszName, pszName, cchName))
                    break;
                if (cchPrefix)
                {
                    if (    pCur->pMod->cchName == cchName + cchPrefix
                        &&  !kHlpMemComp(pCur->pMod->pszName, pszPrefix, cchPrefix)
                        &&  !kHlpMemComp(pCur->pMod->pszName + cchPrefix, pszName, cchName))
                    break;
                }
                if (cchSuffix)
                {
                    if (    pCur->pMod->cchName == cchName + cchSuffix
                        &&  !kHlpMemComp(pCur->pMod->pszName + cchName, pszSuffix, cchSuffix)
                        &&  !kHlpMemComp(pCur->pMod->pszName, pszName, cchName))
                    break;
                    if (    fNameHasSuffix
                        &&  pCur->pMod->cchName == cchName - cchSuffix
                        &&  !kHlpMemComp(pCur->pMod->pszName, pszName, cchName - cchSuffix))
                    break;
                    if (cchPrefix)
                    {
                        if (    pCur->pMod->cchName == cchName + cchPrefix + cchSuffix
                            &&  !kHlpMemComp(pCur->pMod->pszName, pszPrefix, cchPrefix)
                            &&  !kHlpMemComp(pCur->pMod->pszName + cchPrefix, pszName, cchName)
                            &&  !kHlpMemComp(pCur->pMod->pszName + cchPrefix + cchName, pszSuffix, cchSuffix))
                        break;
                        if (    pCur->pMod->cchName == cchName + cchPrefix - cchSuffix
                            &&  !kHlpMemComp(pCur->pMod->pszName, pszPrefix, cchPrefix)
                            &&  !kHlpMemComp(pCur->pMod->pszName + cchPrefix, pszName, cchName - cchSuffix))
                        break;
                    }
                }
            }
        }

        /* search result. */
        if (pCur)
        {
            *ppMod = pCur;
            rc = 0;
        }
        else
            rc = KLDR_ERR_MODULE_NOT_FOUND;
    }

    return rc;
}

