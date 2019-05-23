/* $Id: VBoxStub.cpp $ */
/** @file
 * VBoxStub - VirtualBox's Windows installer stub.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0501
# undef  _WIN32_WINNT
# define _WIN32_WINNT 0x0501 /* AttachConsole() / FreeConsole(). */
#endif

#include <iprt/win/windows.h>
#include <commctrl.h>
#include <fcntl.h>
#include <io.h>
#include <lmerr.h>
#include <msiquery.h>
#include <iprt/win/objbase.h>

#include <iprt/win/shlobj.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>

#include <VBox/version.h>

#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/list.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include "VBoxStub.h"
#include "../StubBld/VBoxStubBld.h"
#include "resource.h"

#ifdef VBOX_WITH_CODE_SIGNING
# include "VBoxStubCertUtil.h"
# include "VBoxStubPublicCert.h"
#endif

#ifndef TARGET_NT4
/* Use an own console window if run in verbose mode. */
# define VBOX_STUB_WITH_OWN_CONSOLE
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define MY_UNICODE_SUB(str) L ##str
#define MY_UNICODE(str)     MY_UNICODE_SUB(str)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Cleanup record.
 */
typedef struct STUBCLEANUPREC
{
    /** List entry. */
    RTLISTNODE  ListEntry;
    /** True if file, false if directory. */
    bool        fFile;
    /** The path to the file or directory to clean up. */
    char        szPath[1];
} STUBCLEANUPREC;
/** Pointer to a cleanup record. */
typedef STUBCLEANUPREC *PSTUBCLEANUPREC;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Whether it's a silent or interactive GUI driven install. */
static bool             g_fSilent = false;
/** List of temporary files. */
static RTLISTANCHOR     g_TmpFiles;
/** Verbosity flag. */
static int              g_iVerbosity = 0;



/**
 * Shows an error message box with a printf() style formatted string.
 *
 * @returns RTEXITCODE_FAILURE
 * @param   pszFmt              Printf-style format string to show in the message box body.
 *
 */
static RTEXITCODE ShowError(const char *pszFmt, ...)
{
    char       *pszMsg;
    va_list     va;

    va_start(va, pszFmt);
    if (RTStrAPrintfV(&pszMsg, pszFmt, va))
    {
        if (g_fSilent)
            RTMsgError("%s", pszMsg);
        else
        {
            PRTUTF16 pwszMsg;
            int rc = RTStrToUtf16(pszMsg, &pwszMsg);
            if (RT_SUCCESS(rc))
            {
                MessageBoxW(GetDesktopWindow(), pwszMsg, MY_UNICODE(VBOX_STUB_TITLE), MB_ICONERROR);
                RTUtf16Free(pwszMsg);
            }
            else
                MessageBoxA(GetDesktopWindow(), pszMsg, VBOX_STUB_TITLE, MB_ICONERROR);
        }
        RTStrFree(pszMsg);
    }
    else /* Should never happen! */
        AssertMsgFailed(("Failed to format error text of format string: %s!\n", pszFmt));
    va_end(va);
    return RTEXITCODE_FAILURE;
}


/**
 * Shows a message box with a printf() style formatted string.
 *
 * @param   uType               Type of the message box (see MSDN).
 * @param   pszFmt              Printf-style format string to show in the message box body.
 *
 */
static void ShowInfo(const char *pszFmt, ...)
{
    char       *pszMsg;
    va_list     va;
    va_start(va, pszFmt);
    int rc = RTStrAPrintfV(&pszMsg, pszFmt, va);
    va_end(va);
    if (rc >= 0)
    {
        if (g_fSilent)
            RTPrintf("%s\n", pszMsg);
        else
        {
            PRTUTF16 pwszMsg;
            int rc = RTStrToUtf16(pszMsg, &pwszMsg);
            if (RT_SUCCESS(rc))
            {
                MessageBoxW(GetDesktopWindow(), pwszMsg, MY_UNICODE(VBOX_STUB_TITLE), MB_ICONINFORMATION);
                RTUtf16Free(pwszMsg);
            }
            else
                MessageBoxA(GetDesktopWindow(), pszMsg, VBOX_STUB_TITLE, MB_ICONINFORMATION);
        }
    }
    else /* Should never happen! */
        AssertMsgFailed(("Failed to format error text of format string: %s!\n", pszFmt));
    RTStrFree(pszMsg);
}


/**
 * Finds the specified in the resource section of the executable.
 *
 * @returns IPRT status code.
 *
 * @param   pszDataName         Name of resource to read.
 * @param   ppvResource         Where to return the pointer to the data.
 * @param   pdwSize             Where to return the size of the data (if found).
 *                              Optional.
 */
static int FindData(const char *pszDataName, PVOID *ppvResource, DWORD *pdwSize)
{
    AssertReturn(pszDataName, VERR_INVALID_PARAMETER);
    HINSTANCE hInst = NULL;             /* indicates the executable image */

    /* Find our resource. */
    PRTUTF16 pwszDataName;
    int rc = RTStrToUtf16(pszDataName, &pwszDataName);
    AssertRCReturn(rc, rc);
    HRSRC hRsrc = FindResourceExW(hInst,
                                  (LPWSTR)RT_RCDATA,
                                  pwszDataName,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    RTUtf16Free(pwszDataName);
    AssertReturn(hRsrc, VERR_IO_GEN_FAILURE);

    /* Get resource size. */
    DWORD cb = SizeofResource(hInst, hRsrc);
    AssertReturn(cb > 0, VERR_NO_DATA);
    if (pdwSize)
        *pdwSize = cb;

    /* Get pointer to resource. */
    HGLOBAL hData = LoadResource(hInst, hRsrc);
    AssertReturn(hData, VERR_IO_GEN_FAILURE);

    /* Lock resource. */
    *ppvResource = LockResource(hData);
    AssertReturn(*ppvResource, VERR_IO_GEN_FAILURE);
    return VINF_SUCCESS;
}


/**
 * Finds the header for the given package.
 *
 * @returns Pointer to the package header on success.  On failure NULL is
 *          returned after ShowError has been invoked.
 * @param   iPackage            The package number.
 */
static PVBOXSTUBPKG FindPackageHeader(unsigned iPackage)
{
    char szHeaderName[32];
    RTStrPrintf(szHeaderName, sizeof(szHeaderName), "HDR_%02d", iPackage);

    PVBOXSTUBPKG pPackage;
    int rc = FindData(szHeaderName, (PVOID *)&pPackage, NULL);
    if (RT_FAILURE(rc))
    {
        ShowError("Internal error: Could not find package header #%u: %Rrc", iPackage, rc);
        return NULL;
    }

    /** @todo validate it. */
    return pPackage;
}



/**
 * Constructs a full temporary file path from the given parameters.
 *
 * @returns iprt status code.
 *
 * @param   pszTempPath         The pure path to use for construction.
 * @param   pszTargetFileName   The pure file name to use for construction.
 * @param   ppszTempFile        Pointer to the constructed string.  Must be freed
 *                              using RTStrFree().
 */
static int GetTempFileAlloc(const char  *pszTempPath,
                            const char  *pszTargetFileName,
                            char       **ppszTempFile)
{
    if (RTStrAPrintf(ppszTempFile, "%s\\%s", pszTempPath, pszTargetFileName) >= 0)
        return VINF_SUCCESS;
    return VERR_NO_STR_MEMORY;
}


/**
 * Extracts a built-in resource to disk.
 *
 * @returns iprt status code.
 *
 * @param   pszResourceName     The resource name to extract.
 * @param   pszTempFile         The full file path + name to extract the resource to.
 *
 */
static int ExtractFile(const char *pszResourceName,
                       const char *pszTempFile)
{
#if 0 /* Another example of how unnecessarily complicated things get with
         do-break-while-false and you end up with buggy code using uninitialized
         variables. */
    int rc;
    RTFILE fh;
    BOOL bCreatedFile = FALSE;

    do
    {
        AssertMsgBreak(pszResourceName, ("Resource pointer invalid!\n")); /* rc is not initialized here, we'll return garbage. */
        AssertMsgBreak(pszTempFile, ("Temp file pointer invalid!"));      /* Ditto. */

        /* Read the data of the built-in resource. */
        PVOID pvData = NULL;
        DWORD dwDataSize = 0;
        rc = FindData(pszResourceName, &pvData, &dwDataSize);
        AssertMsgRCBreak(rc, ("Could not read resource data!\n"));

        /* Create new (and replace an old) file. */
        rc = RTFileOpen(&fh, pszTempFile,
                          RTFILE_O_CREATE_REPLACE
                        | RTFILE_O_WRITE
                        | RTFILE_O_DENY_NOT_DELETE
                        | RTFILE_O_DENY_WRITE);
        AssertMsgRCBreak(rc, ("Could not open file for writing!\n"));
        bCreatedFile = TRUE;

        /* Write contents to new file. */
        size_t cbWritten = 0;
        rc = RTFileWrite(fh, pvData, dwDataSize, &cbWritten);
        AssertMsgRCBreak(rc, ("Could not open file for writing!\n"));
        AssertMsgBreak(dwDataSize == cbWritten, ("File was not extracted completely! Disk full?\n"));

    } while (0);

    if (RTFileIsValid(fh)) /* fh is unused uninitalized (MSC agrees) */
        RTFileClose(fh);

    if (RT_FAILURE(rc))
    {
        if (bCreatedFile)
            RTFileDelete(pszTempFile);
    }

#else /* This is exactly the same as above, except no bug and better assertion
         message.  Note only the return-success statment is indented, indicating
         that the whole do-break-while-false approach was totally unnecessary.   */

    AssertPtrReturn(pszResourceName, VERR_INVALID_POINTER);
    AssertPtrReturn(pszTempFile, VERR_INVALID_POINTER);

    /* Read the data of the built-in resource. */
    PVOID pvData = NULL;
    DWORD dwDataSize = 0;
    int rc = FindData(pszResourceName, &pvData, &dwDataSize);
    AssertMsgRCReturn(rc, ("Could not read resource data: %Rrc\n", rc), rc);

    /* Create new (and replace an old) file. */
    RTFILE hFile;
    rc = RTFileOpen(&hFile, pszTempFile,
                      RTFILE_O_CREATE_REPLACE
                    | RTFILE_O_WRITE
                    | RTFILE_O_DENY_NOT_DELETE
                    | RTFILE_O_DENY_WRITE);
    AssertMsgRCReturn(rc, ("Could not open '%s' for writing: %Rrc\n", pszTempFile, rc), rc);

    /* Write contents to new file. */
    size_t cbWritten = 0;
    rc = RTFileWrite(hFile, pvData, dwDataSize, &cbWritten);
    AssertMsgStmt(cbWritten == dwDataSize || RT_FAILURE_NP(rc), ("%#zx vs %#x\n", cbWritten, dwDataSize), rc = VERR_WRITE_ERROR);

    int rc2 = RTFileClose(hFile);
    AssertRC(rc2);

    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    RTFileDelete(pszTempFile);

#endif
    return rc;
}


/**
 * Extracts a built-in resource to disk.
 *
 * @returns iprt status code.
 *
 * @param   pPackage            Pointer to a VBOXSTUBPKG struct that contains the resource.
 * @param   pszTempFile         The full file path + name to extract the resource to.
 *
 */
static int Extract(const PVBOXSTUBPKG  pPackage,
                   const char         *pszTempFile)
{
    return ExtractFile(pPackage->szResourceName, pszTempFile);
}


/**
 * Detects whether we're running on a 32- or 64-bit platform and returns the result.
 *
 * @returns TRUE if we're running on a 64-bit OS, FALSE if not.
 *
 */
static BOOL IsWow64(void)
{
    BOOL bIsWow64 = TRUE;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            /* Error in retrieving process type - assume that we're running on 32bit. */
            return FALSE;
        }
    }
    return bIsWow64;
}


/**
 * Decides whether we need a specified package to handle or not.
 *
 * @returns @c true if we need to handle the specified package, @c false if not.
 *
 * @param   pPackage            Pointer to a VBOXSTUBPKG struct that contains the resource.
 *
 */
static bool PackageIsNeeded(PVBOXSTUBPKG pPackage)
{
    if (pPackage->byArch == VBOXSTUBPKGARCH_ALL)
        return true;
    VBOXSTUBPKGARCH enmArch = IsWow64() ? VBOXSTUBPKGARCH_AMD64 : VBOXSTUBPKGARCH_X86;
    return pPackage->byArch == enmArch;
}


/**
 * Adds a cleanup record.
 *
 * @returns Fully complained boolean success indicator.
 * @param   pszPath             The path to the file or directory to clean up.
 * @param   fFile               @c true if file, @c false if directory.
 */
static bool AddCleanupRec(const char *pszPath, bool fFile)
{
    size_t cchPath = strlen(pszPath); Assert(cchPath > 0);
    PSTUBCLEANUPREC pRec = (PSTUBCLEANUPREC)RTMemAlloc(RT_OFFSETOF(STUBCLEANUPREC, szPath[cchPath + 1]));
    if (!pRec)
    {
        ShowError("Out of memory!");
        return false;
    }
    pRec->fFile = fFile;
    memcpy(pRec->szPath, pszPath, cchPath + 1);

    RTListPrepend(&g_TmpFiles, &pRec->ListEntry);
    return true;
}


/**
 * Cleans up all the extracted files and optionally removes the package
 * directory.
 *
 * @param   pszPkgDir           The package directory, NULL if it shouldn't be
 *                              removed.
 */
static void CleanUp(const char *pszPkgDir)
{
    for (int i = 0; i < 5; i++)
    {
        int rc;
        bool fFinalTry = i == 4;

        PSTUBCLEANUPREC pCur, pNext;
        RTListForEachSafe(&g_TmpFiles, pCur, pNext, STUBCLEANUPREC, ListEntry)
        {
            if (pCur->fFile)
                rc = RTFileDelete(pCur->szPath);
            else
            {
                rc = RTDirRemoveRecursive(pCur->szPath, RTDIRRMREC_F_CONTENT_AND_DIR);
                if (rc == VERR_DIR_NOT_EMPTY && fFinalTry)
                    rc = VINF_SUCCESS;
            }
            if (rc == VERR_FILE_NOT_FOUND || rc == VERR_PATH_NOT_FOUND)
                rc = VINF_SUCCESS;
            if (RT_SUCCESS(rc))
            {
                RTListNodeRemove(&pCur->ListEntry);
                RTMemFree(pCur);
            }
            else if (fFinalTry)
            {
                if (pCur->fFile)
                    ShowError("Failed to delete temporary file '%s': %Rrc", pCur->szPath, rc);
                else
                    ShowError("Failed to delete temporary directory '%s': %Rrc", pCur->szPath, rc);
            }
        }

        if (RTListIsEmpty(&g_TmpFiles) || fFinalTry)
        {
            if (!pszPkgDir)
                return;
            rc = RTDirRemove(pszPkgDir);
            if (RT_SUCCESS(rc) || rc == VERR_FILE_NOT_FOUND || rc == VERR_PATH_NOT_FOUND || fFinalTry)
                return;
        }

        /* Delay a little and try again. */
        RTThreadSleep(i == 0 ? 100 : 3000);
    }
}


/**
 * Processes an MSI package.
 *
 * @returns Fully complained exit code.
 * @param   pszMsi              The path to the MSI to process.
 * @param   pszMsiArgs          Any additional installer (MSI) argument
 * @param   fLogging            Whether to enable installer logging.
 */
static RTEXITCODE ProcessMsiPackage(const char *pszMsi, const char *pszMsiArgs, bool fLogging)
{
    int rc;

    /*
     * Set UI level.
     */
    INSTALLUILEVEL enmDesiredUiLevel = g_fSilent ? INSTALLUILEVEL_NONE : INSTALLUILEVEL_FULL;
    INSTALLUILEVEL enmRet = MsiSetInternalUI(enmDesiredUiLevel, NULL);
    if (enmRet == INSTALLUILEVEL_NOCHANGE /* means error */)
        return ShowError("Internal error: MsiSetInternalUI failed.");

    /*
     * Enable logging?
     */
    if (fLogging)
    {
        char szLogFile[RTPATH_MAX];
        rc = RTStrCopy(szLogFile, sizeof(szLogFile), pszMsi);
        if (RT_SUCCESS(rc))
        {
            RTPathStripFilename(szLogFile);
            rc = RTPathAppend(szLogFile, sizeof(szLogFile), "VBoxInstallLog.txt");
        }
        if (RT_FAILURE(rc))
            return ShowError("Internal error: Filename path too long.");

        PRTUTF16 pwszLogFile;
        rc = RTStrToUtf16(szLogFile, &pwszLogFile);
        if (RT_FAILURE(rc))
            return ShowError("RTStrToUtf16 failed on '%s': %Rrc", szLogFile, rc);

        UINT uLogLevel = MsiEnableLogW(INSTALLLOGMODE_VERBOSE,
                                       pwszLogFile,
                                       INSTALLLOGATTRIBUTES_FLUSHEACHLINE);
        RTUtf16Free(pwszLogFile);
        if (uLogLevel != ERROR_SUCCESS)
            return ShowError("MsiEnableLogW failed");
    }

    /*
     * Initialize the common controls (extended version). This is necessary to
     * run the actual .MSI installers with the new fancy visual control
     * styles (XP+). Also, an integrated manifest is required.
     */
    INITCOMMONCONTROLSEX ccEx;
    ccEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ccEx.dwICC = ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES | ICC_PAGESCROLLER_CLASS |
                 ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES |
                 ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&ccEx); /* Ignore failure. */

    /*
     * Convert both strings to UTF-16 and start the installation.
     */
    PRTUTF16 pwszMsi;
    rc = RTStrToUtf16(pszMsi, &pwszMsi);
    if (RT_FAILURE(rc))
        return ShowError("RTStrToUtf16 failed on '%s': %Rrc", pszMsi, rc);
    PRTUTF16 pwszMsiArgs;
    rc = RTStrToUtf16(pszMsiArgs, &pwszMsiArgs);
    if (RT_FAILURE(rc))
    {
        RTUtf16Free(pwszMsi);
        return ShowError("RTStrToUtf16 failed on '%s': %Rrc", pszMsi, rc);
    }

    UINT uStatus = MsiInstallProductW(pwszMsi, pwszMsiArgs);
    RTUtf16Free(pwszMsi);
    RTUtf16Free(pwszMsiArgs);

    if (uStatus == ERROR_SUCCESS)
        return RTEXITCODE_SUCCESS;
    if (uStatus == ERROR_SUCCESS_REBOOT_REQUIRED)
        return RTEXITCODE_SUCCESS; /* we currently don't indicate this */

    /*
     * Installation failed. Figure out what to say.
     */
    switch (uStatus)
    {
        case ERROR_INSTALL_USEREXIT:
            /* Don't say anything? */
            break;

        case ERROR_INSTALL_PACKAGE_VERSION:
            ShowError("This installation package cannot be installed by the Windows Installer service.\n"
                      "You must install a Windows service pack that contains a newer version of the Windows Installer service.");
            break;

        case ERROR_INSTALL_PLATFORM_UNSUPPORTED:
            ShowError("This installation package is not supported on this platform.");
            break;

        default:
        {
            /*
             * Try get windows to format the message.
             */
            DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                                | FORMAT_MESSAGE_IGNORE_INSERTS
                                | FORMAT_MESSAGE_FROM_SYSTEM;
            HMODULE hModule = NULL;
            if (uStatus >= NERR_BASE && uStatus <= MAX_NERR)
            {
                hModule = LoadLibraryExW(L"netmsg.dll",
                                         NULL,
                                         LOAD_LIBRARY_AS_DATAFILE);
                if (hModule != NULL)
                    dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            }

            PWSTR pwszMsg;
            if (FormatMessageW(dwFormatFlags,
                               hModule, /* If NULL, load system stuff. */
                               uStatus,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (PWSTR)&pwszMsg,
                               0,
                               NULL) > 0)
            {
                ShowError("Installation failed! Error: %ls", pwszMsg);
                LocalFree(pwszMsg);
            }
            else /* If text lookup failed, show at least the error number. */
                ShowError("Installation failed! Error: %u", uStatus);

            if (hModule)
                FreeLibrary(hModule);
            break;
        }
    }

    return RTEXITCODE_FAILURE;
}


/**
 * Processes a package.
 *
 * @returns Fully complained exit code.
 * @param   iPackage            The package number.
 * @param   pszPkgDir           The package directory (aka extraction dir).
 * @param   pszMsiArgs          Any additional installer (MSI) argument
 * @param   fLogging            Whether to enable installer logging.
 */
static RTEXITCODE ProcessPackage(unsigned iPackage, const char *pszPkgDir, const char *pszMsiArgs, bool fLogging)
{
    /*
     * Get the package header and check if it's needed.
     */
    PVBOXSTUBPKG pPackage = FindPackageHeader(iPackage);
    if (pPackage == NULL)
        return RTEXITCODE_FAILURE;

    if (!PackageIsNeeded(pPackage))
        return RTEXITCODE_SUCCESS;

    /*
     * Deal with the file based on it's extension.
     */
    char szPkgFile[RTPATH_MAX];
    int rc = RTPathJoin(szPkgFile, sizeof(szPkgFile), pszPkgDir, pPackage->szFileName);
    if (RT_FAILURE(rc))
        return ShowError("Internal error: RTPathJoin failed: %Rrc", rc);
    RTPathChangeToDosSlashes(szPkgFile, true /* Force conversion. */); /* paranoia */

    RTEXITCODE rcExit;
    const char *pszSuff = RTPathSuffix(szPkgFile);
    if (RTStrICmp(pszSuff, ".msi") == 0)
        rcExit = ProcessMsiPackage(szPkgFile, pszMsiArgs, fLogging);
    else if (RTStrICmp(pszSuff, ".cab") == 0)
        rcExit = RTEXITCODE_SUCCESS; /* Ignore .cab files, they're generally referenced by other files. */
    else
        rcExit = ShowError("Internal error: Do not know how to handle file '%s'.", pPackage->szFileName);

    return rcExit;
}


#ifdef VBOX_WITH_CODE_SIGNING
/**
 * Install the public certificate into TrustedPublishers so the installer won't
 * prompt the user during silent installs.
 *
 * @returns Fully complained exit code.
 */
static RTEXITCODE InstallCertificates(void)
{
    for (uint32_t i = 0; i < RT_ELEMENTS(g_aVBoxStubTrustedCerts); i++)
    {
        if (!addCertToStore(CERT_SYSTEM_STORE_LOCAL_MACHINE,
                            "TrustedPublisher",
                            g_aVBoxStubTrustedCerts[i].pab,
                            g_aVBoxStubTrustedCerts[i].cb))
            return ShowError("Failed to construct install certificate.");
    }
    return RTEXITCODE_SUCCESS;
}
#endif /* VBOX_WITH_CODE_SIGNING */


/**
 * Copies the "<exepath>.custom" directory to the extraction path if it exists.
 *
 * This is used by the MSI packages from the resource section.
 *
 * @returns Fully complained exit code.
 * @param   pszDstDir       The destination directory.
 */
static RTEXITCODE CopyCustomDir(const char *pszDstDir)
{
    char szSrcDir[RTPATH_MAX];
    int rc = RTPathExecDir(szSrcDir, sizeof(szSrcDir));
    if (RT_SUCCESS(rc))
        rc = RTPathAppend(szSrcDir, sizeof(szSrcDir), ".custom");
    if (RT_FAILURE(rc))
        return ShowError("Failed to construct '.custom' dir path: %Rrc", rc);

    if (RTDirExists(szSrcDir))
    {
        /*
         * Use SHFileOperation w/ FO_COPY to do the job.  This API requires an
         * extra zero at the end of both source and destination paths.
         */
        size_t   cwc;
        RTUTF16  wszSrcDir[RTPATH_MAX + 1];
        PRTUTF16 pwszSrcDir = wszSrcDir;
        rc = RTStrToUtf16Ex(szSrcDir, RTSTR_MAX, &pwszSrcDir, RTPATH_MAX, &cwc);
        if (RT_FAILURE(rc))
            return ShowError("RTStrToUtf16Ex failed on '%s': %Rrc", szSrcDir, rc);
        wszSrcDir[cwc] = '\0';

        RTUTF16  wszDstDir[RTPATH_MAX + 1];
        PRTUTF16 pwszDstDir = wszSrcDir;
        rc = RTStrToUtf16Ex(pszDstDir, RTSTR_MAX, &pwszDstDir, RTPATH_MAX, &cwc);
        if (RT_FAILURE(rc))
            return ShowError("RTStrToUtf16Ex failed on '%s': %Rrc", pszDstDir, rc);
        wszDstDir[cwc] = '\0';

        SHFILEOPSTRUCTW FileOp;
        RT_ZERO(FileOp); /* paranoia */
        FileOp.hwnd     = NULL;
        FileOp.wFunc    = FO_COPY;
        FileOp.pFrom    = wszSrcDir;
        FileOp.pTo      = wszDstDir;
        FileOp.fFlags   = FOF_SILENT
                        | FOF_NOCONFIRMATION
                        | FOF_NOCONFIRMMKDIR
                        | FOF_NOERRORUI;
        FileOp.fAnyOperationsAborted = FALSE;
        FileOp.hNameMappings = NULL;
        FileOp.lpszProgressTitle = NULL;

        rc = SHFileOperationW(&FileOp);
        if (rc != 0)    /* Not a Win32 status code! */
            return ShowError("Copying the '.custom' dir failed: %#x", rc);

        /*
         * Add a cleanup record for recursively deleting the destination
         * .custom directory.  We should actually add this prior to calling
         * SHFileOperationW since it may partially succeed...
         */
        char *pszDstSubDir = RTPathJoinA(pszDstDir, ".custom");
        if (!pszDstSubDir)
            return ShowError("Out of memory!");
        bool fRc = AddCleanupRec(pszDstSubDir, false /*fFile*/);
        RTStrFree(pszDstSubDir);
        if (!fRc)
            return RTEXITCODE_FAILURE;
    }

    return RTEXITCODE_SUCCESS;
}


static RTEXITCODE ExtractFiles(unsigned cPackages, const char *pszDstDir, bool fExtractOnly, bool *pfCreatedExtractDir)
{
    int rc;

    /*
     * Make sure the directory exists.
     */
    *pfCreatedExtractDir = false;
    if (!RTDirExists(pszDstDir))
    {
        rc = RTDirCreate(pszDstDir, 0700, 0);
        if (RT_FAILURE(rc))
            return ShowError("Failed to create extraction path '%s': %Rrc", pszDstDir, rc);
        *pfCreatedExtractDir = true;
    }

    /*
     * Extract files.
     */
    for (unsigned k = 0; k < cPackages; k++)
    {
        PVBOXSTUBPKG pPackage = FindPackageHeader(k);
        if (!pPackage)
            return RTEXITCODE_FAILURE; /* Done complaining already. */

        if (fExtractOnly || PackageIsNeeded(pPackage))
        {
            char szDstFile[RTPATH_MAX];
            rc = RTPathJoin(szDstFile, sizeof(szDstFile), pszDstDir, pPackage->szFileName);
            if (RT_FAILURE(rc))
                return ShowError("Internal error: RTPathJoin failed: %Rrc", rc);

            rc = Extract(pPackage, szDstFile);
            if (RT_FAILURE(rc))
                return ShowError("Error extracting package #%u: %Rrc", k, rc);

            if (!fExtractOnly && !AddCleanupRec(szDstFile, true /*fFile*/))
            {
                RTFileDelete(szDstFile);
                return RTEXITCODE_FAILURE;
            }
        }
    }

    return RTEXITCODE_SUCCESS;
}


int WINAPI WinMain(HINSTANCE  hInstance,
                   HINSTANCE  hPrevInstance,
                   char      *lpCmdLine,
                   int        nCmdShow)
{
    RT_NOREF(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    char **argv = __argv;
    int argc    = __argc;

    /*
     * Init IPRT. This is _always_ the very first thing we do.
     */
    int vrc = RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_STANDALONE_APP);
    if (RT_FAILURE(vrc))
        return RTMsgInitFailure(vrc);

    /*
     * Check if we're already running and jump out if so.
     *
     * Note! Do not use a global namespace ("Global\\") for mutex name here,
     *       will blow up NT4 compatibility!
     */
    HANDLE hMutexAppRunning = CreateMutex(NULL, FALSE, "VBoxStubInstaller");
    if (   hMutexAppRunning != NULL
        && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        /* Close the mutex for this application instance. */
        CloseHandle(hMutexAppRunning);
        hMutexAppRunning = NULL;
        return RTEXITCODE_FAILURE;
    }

    /*
     * Parse arguments.
     */

    /* Parameter variables. */
    bool fExtractOnly              = false;
    bool fEnableLogging            = false;
#ifdef VBOX_WITH_CODE_SIGNING
    bool fEnableSilentCert         = true;
#endif
    char szExtractPath[RTPATH_MAX] = {0};
    char szMSIArgs[_4K]            = {0};

    /* Parameter definitions. */
    static const RTGETOPTDEF s_aOptions[] =
    {
        /** @todo Replace short parameters with enums since they're not
         *        used (and not documented to the public). */
        { "--extract",          'x', RTGETOPT_REQ_NOTHING },
        { "-extract",           'x', RTGETOPT_REQ_NOTHING },
        { "/extract",           'x', RTGETOPT_REQ_NOTHING },
        { "--silent",           's', RTGETOPT_REQ_NOTHING },
        { "-silent",            's', RTGETOPT_REQ_NOTHING },
        { "/silent",            's', RTGETOPT_REQ_NOTHING },
#ifdef VBOX_WITH_CODE_SIGNING
        { "--no-silent-cert",   'c', RTGETOPT_REQ_NOTHING },
        { "-no-silent-cert",    'c', RTGETOPT_REQ_NOTHING },
        { "/no-silent-cert",    'c', RTGETOPT_REQ_NOTHING },
#endif
        { "--logging",          'l', RTGETOPT_REQ_NOTHING },
        { "-logging",           'l', RTGETOPT_REQ_NOTHING },
        { "/logging",           'l', RTGETOPT_REQ_NOTHING },
        { "--path",             'p', RTGETOPT_REQ_STRING  },
        { "-path",              'p', RTGETOPT_REQ_STRING  },
        { "/path",              'p', RTGETOPT_REQ_STRING  },
        { "--msiparams",        'm', RTGETOPT_REQ_STRING  },
        { "-msiparams",         'm', RTGETOPT_REQ_STRING  },
        { "--reinstall",        'f', RTGETOPT_REQ_NOTHING },
        { "-reinstall",         'f', RTGETOPT_REQ_NOTHING },
        { "/reinstall",         'f', RTGETOPT_REQ_NOTHING },
        { "--verbose",          'v', RTGETOPT_REQ_NOTHING },
        { "-verbose",           'v', RTGETOPT_REQ_NOTHING },
        { "/verbose",           'v', RTGETOPT_REQ_NOTHING },
        { "--version",          'V', RTGETOPT_REQ_NOTHING },
        { "-version",           'V', RTGETOPT_REQ_NOTHING },
        { "/version",           'V', RTGETOPT_REQ_NOTHING },
        { "-v",                 'V', RTGETOPT_REQ_NOTHING },
        { "--help",             'h', RTGETOPT_REQ_NOTHING },
        { "-help",              'h', RTGETOPT_REQ_NOTHING },
        { "/help",              'h', RTGETOPT_REQ_NOTHING },
        { "/?",                 'h', RTGETOPT_REQ_NOTHING },
    };

    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;

    /* Parse the parameters. */
    int ch;
    bool fExitEarly = false;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0);
    while (   (ch = RTGetOpt(&GetState, &ValueUnion))
           && rcExit == RTEXITCODE_SUCCESS
           && !fExitEarly)
    {
        switch (ch)
        {
            case 'f': /* Force re-installation. */
                if (szMSIArgs[0])
                    vrc = RTStrCat(szMSIArgs, sizeof(szMSIArgs), " ");
                if (RT_SUCCESS(vrc))
                    vrc = RTStrCat(szMSIArgs, sizeof(szMSIArgs),
                                   "REINSTALLMODE=vomus REINSTALL=ALL");
                if (RT_FAILURE(vrc))
                    rcExit = ShowError("MSI parameters are too long.");
                break;

            case 'x':
                fExtractOnly = true;
                break;

            case 's':
                g_fSilent = true;
                break;

#ifdef VBOX_WITH_CODE_SIGNING
            case 'c':
                fEnableSilentCert = false;
                break;
#endif
            case 'l':
                fEnableLogging = true;
                break;

            case 'p':
                vrc = RTStrCopy(szExtractPath, sizeof(szExtractPath), ValueUnion.psz);
                if (RT_FAILURE(vrc))
                    rcExit = ShowError("Extraction path is too long.");
                break;

            case 'm':
                if (szMSIArgs[0])
                    vrc = RTStrCat(szMSIArgs, sizeof(szMSIArgs), " ");
                if (RT_SUCCESS(vrc))
                    vrc = RTStrCat(szMSIArgs, sizeof(szMSIArgs), ValueUnion.psz);
                if (RT_FAILURE(vrc))
                    rcExit = ShowError("MSI parameters are too long.");
                break;

            case 'V':
                ShowInfo("Version: %d.%d.%d.%d",
                         VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD,
                         VBOX_SVN_REV);
                fExitEarly = true;
                break;

            case 'v':
                g_iVerbosity++;
                break;

            case 'h':
                ShowInfo("-- %s v%d.%d.%d.%d --\n"
                         "\n"
                         "Command Line Parameters:\n\n"
                         "--extract                - Extract file contents to temporary directory\n"
                         "--help                   - Print this help and exit\n"
                         "--logging                - Enables installer logging\n"
                         "--msiparams <parameters> - Specifies extra parameters for the MSI installers\n"
                         "--no-silent-cert         - Do not install VirtualBox Certificate automatically when --silent option is specified\n"
                         "--path                   - Sets the path of the extraction directory\n"
                         "--reinstall              - Forces VirtualBox to get re-installed\n"
                         "--silent                 - Enables silent mode installation\n"
                         "--version                - Print version number and exit\n\n"
                         "Examples:\n"
                         "%s --msiparams INSTALLDIR=C:\\VBox\n"
                         "%s --extract -path C:\\VBox",
                         VBOX_STUB_TITLE, VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV,
                         argv[0], argv[0]);
                fExitEarly = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                /* Are (optional) MSI parameters specified and this is the last
                 * parameter? Append everything to the MSI parameter list then. */
                if (szMSIArgs[0])
                {
                    vrc = RTStrCat(szMSIArgs, sizeof(szMSIArgs), " ");
                    if (RT_SUCCESS(vrc))
                        vrc = RTStrCat(szMSIArgs, sizeof(szMSIArgs), ValueUnion.psz);
                    if (RT_FAILURE(vrc))
                        rcExit = ShowError("MSI parameters are too long.");
                    continue;
                }
                /* Fall through is intentional. */

            default:
                if (g_fSilent)
                    rcExit = RTGetOptPrintError(ch, &ValueUnion);
                if (ch == VERR_GETOPT_UNKNOWN_OPTION)
                    rcExit = ShowError("Unknown option \"%s\"\n"
                                       "Please refer to the command line help by specifying \"/?\"\n"
                                       "to get more information.", ValueUnion.psz);
                else
                    rcExit = ShowError("Parameter parsing error: %Rrc\n"
                                       "Please refer to the command line help by specifying \"/?\"\n"
                                       "to get more information.", ch);
                break;
        }
    }

    /* Check if we can bail out early. */
    if (fExitEarly)
        return rcExit;

    if (rcExit != RTEXITCODE_SUCCESS)
        vrc = VERR_PARSE_ERROR;

/** @todo
 *
 *  Split the remainder up in functions and simplify the code flow!!
 *
 *   */

#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0501
# ifdef VBOX_STUB_WITH_OWN_CONSOLE /* Use an own console window if run in debug mode. */
    if (   RT_SUCCESS(vrc)
        && g_iVerbosity)
    {
        if (!AllocConsole())
        {
            DWORD dwErr = GetLastError();
            ShowError("Unable to allocate console, error = %ld\n",
                      dwErr);

            /* Close the mutex for this application instance. */
            CloseHandle(hMutexAppRunning);
            hMutexAppRunning = NULL;
            return RTEXITCODE_FAILURE;
        }

        freopen("CONOUT$", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0);

        freopen("CONOUT$", "w", stderr);
    }
# endif /* VBOX_STUB_WITH_OWN_CONSOLE */
#endif

    if (   RT_SUCCESS(vrc)
        && g_iVerbosity)
    {
        RTPrintf("Silent installation      : %RTbool\n", g_fSilent);
        RTPrintf("Logging enabled          : %RTbool\n", fEnableLogging);
#ifdef VBOX_WITH_CODE_SIGNING
        RTPrintf("Certificate installation : %RTbool\n", fEnableSilentCert);
#endif
        RTPrintf("Additional MSI parameters: %s\n",
                 szMSIArgs[0] ? szMSIArgs : "<None>");
    }

    if (RT_SUCCESS(vrc))
    {
        /*
         * Determine the extration path if not given by the user, and gather some
         * other bits we'll be needing later.
         */
        if (szExtractPath[0] == '\0')
        {
            vrc = RTPathTemp(szExtractPath, sizeof(szExtractPath));
            if (RT_SUCCESS(vrc))
                vrc = RTPathAppend(szExtractPath, sizeof(szExtractPath), "VirtualBox");
            if (RT_FAILURE(vrc))
                ShowError("Failed to determine extraction path (%Rrc)", vrc);

        }
        else
        {
            /** @todo should check if there is a .custom subdirectory there or not. */
        }
        RTPathChangeToDosSlashes(szExtractPath,
                                 true /* Force conversion. */); /* MSI requirement. */
    }

    /* Read our manifest. */
    if (RT_SUCCESS(vrc))
    {
        PVBOXSTUBPKGHEADER pHeader;
        vrc = FindData("MANIFEST", (PVOID *)&pHeader, NULL);
        if (RT_SUCCESS(vrc))
        {
            /** @todo If we could, we should validate the header. Only the magic isn't
             *        commonly defined, nor the version number... */

            RTListInit(&g_TmpFiles);

            /*
             * Up to this point, we haven't done anything that requires any cleanup.
             * From here on, we do everything in function so we can counter clean up.
             */
            bool fCreatedExtractDir;
            rcExit = ExtractFiles(pHeader->byCntPkgs, szExtractPath,
                                  fExtractOnly, &fCreatedExtractDir);
            if (rcExit == RTEXITCODE_SUCCESS)
            {
                if (fExtractOnly)
                    ShowInfo("Files were extracted to: %s", szExtractPath);
                else
                {
                    rcExit = CopyCustomDir(szExtractPath);
#ifdef VBOX_WITH_CODE_SIGNING
                    if (rcExit == RTEXITCODE_SUCCESS && fEnableSilentCert && g_fSilent)
                        rcExit = InstallCertificates();
#endif
                    unsigned iPackage = 0;
                    while (   iPackage < pHeader->byCntPkgs
                           && rcExit == RTEXITCODE_SUCCESS)
                    {
                        rcExit = ProcessPackage(iPackage, szExtractPath,
                                                szMSIArgs, fEnableLogging);
                        iPackage++;
                    }

                    /* Don't fail if cleanup fail. At least for now. */
                    CleanUp(   !fEnableLogging
                            && fCreatedExtractDir ? szExtractPath : NULL);
                }
            }

            /* Free any left behind cleanup records (not strictly needed). */
            PSTUBCLEANUPREC pCur, pNext;
            RTListForEachSafe(&g_TmpFiles, pCur, pNext, STUBCLEANUPREC, ListEntry)
            {
                RTListNodeRemove(&pCur->ListEntry);
                RTMemFree(pCur);
            }
        }
        else
            rcExit = ShowError("Internal package error: Manifest not found (%Rrc)", vrc);
    }

#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0501
# ifdef VBOX_STUB_WITH_OWN_CONSOLE
    if (g_iVerbosity)
        FreeConsole();
# endif /* VBOX_STUB_WITH_OWN_CONSOLE */
#endif

    /*
     * Release instance mutex.
     */
    if (hMutexAppRunning != NULL)
    {
        CloseHandle(hMutexAppRunning);
        hMutexAppRunning = NULL;
    }

    return rcExit;
}

