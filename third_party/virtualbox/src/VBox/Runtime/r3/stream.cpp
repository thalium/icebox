/* $Id: stream.cpp $ */
/** @file
 * IPRT - I/O Stream.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */



#if defined(RT_OS_LINUX) /* PORTME: check for the _unlocked functions in stdio.h */
#define HAVE_FWRITE_UNLOCKED
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/stream.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#ifndef HAVE_FWRITE_UNLOCKED
# include <iprt/critsect.h>
#endif
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/string.h>

#include "internal/alignmentchecks.h"
#include "internal/magics.h"

#include <stdio.h>
#include <errno.h>
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
# include <io.h>
# include <fcntl.h>
#endif
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
#else
# include <termios.h>
# include <unistd.h>
# include <sys/ioctl.h>
#endif

#ifdef RT_OS_OS2
# define _O_TEXT   O_TEXT
# define _O_BINARY O_BINARY
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * File stream.
 */
typedef struct RTSTREAM
{
    /** Magic value used to validate the stream. (RTSTREAM_MAGIC) */
    uint32_t            u32Magic;
    /** File stream error. */
    int32_t volatile    i32Error;
    /** Pointer to the LIBC file stream. */
    FILE               *pFile;
    /** Stream is using the current process code set. */
    bool                fCurrentCodeSet;
    /** Whether the stream was opened in binary mode. */
    bool                fBinary;
    /** Whether to recheck the stream mode before writing. */
    bool                fRecheckMode;
#ifndef HAVE_FWRITE_UNLOCKED
    /** Critical section for serializing access to the stream. */
    PRTCRITSECT         pCritSect;
#endif
} RTSTREAM;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The standard input stream. */
static RTSTREAM    g_StdIn =
{
    RTSTREAM_MAGIC,
    0,
    stdin,
    /*.fCurrentCodeSet = */ true,
    /*.fBinary = */ false,
    /*.fRecheckMode = */ true
#ifndef HAVE_FWRITE_UNLOCKED
    , NULL
#endif
};

/** The standard error stream. */
static RTSTREAM    g_StdErr =
{
    RTSTREAM_MAGIC,
    0,
    stderr,
    /*.fCurrentCodeSet = */ true,
    /*.fBinary = */ false,
    /*.fRecheckMode = */ true
#ifndef HAVE_FWRITE_UNLOCKED
    , NULL
#endif
};

/** The standard output stream. */
static RTSTREAM    g_StdOut =
{
    RTSTREAM_MAGIC,
    0,
    stdout,
    /*.fCurrentCodeSet = */ true,
    /*.fBinary = */ false,
    /*.fRecheckMode = */ true
#ifndef HAVE_FWRITE_UNLOCKED
    , NULL
#endif
};

/** Pointer to the standard input stream. */
RTDATADECL(PRTSTREAM)   g_pStdIn  = &g_StdIn;

/** Pointer to the standard output stream. */
RTDATADECL(PRTSTREAM)   g_pStdErr = &g_StdErr;

/** Pointer to the standard output stream. */
RTDATADECL(PRTSTREAM)   g_pStdOut = &g_StdOut;


#ifndef HAVE_FWRITE_UNLOCKED
/**
 * Allocates and acquires the lock for the stream.
 *
 * @returns IPRT status code.
 * @param   pStream     The stream (valid).
 */
static int rtStrmAllocLock(PRTSTREAM pStream)
{
    Assert(pStream->pCritSect == NULL);

    PRTCRITSECT pCritSect = (PRTCRITSECT)RTMemAlloc(sizeof(*pCritSect));
    if (!pCritSect)
        return VERR_NO_MEMORY;

    /* The native stream lock are normally not recursive. */
    int rc = RTCritSectInitEx(pCritSect, RTCRITSECT_FLAGS_NO_NESTING,
                              NIL_RTLOCKVALCLASS, RTLOCKVAL_SUB_CLASS_NONE, "RTSemSpinMutex");
    if (RT_SUCCESS(rc))
    {
        rc = RTCritSectEnter(pCritSect);
        if (RT_SUCCESS(rc))
        {
            if (RT_LIKELY(ASMAtomicCmpXchgPtr(&pStream->pCritSect, pCritSect, NULL)))
                return VINF_SUCCESS;

            RTCritSectLeave(pCritSect);
        }
        RTCritSectDelete(pCritSect);
    }
    RTMemFree(pCritSect);

    /* Handle the lost race case... */
    pCritSect = ASMAtomicReadPtrT(&pStream->pCritSect, PRTCRITSECT);
    if (pCritSect)
        return RTCritSectEnter(pCritSect);

    return rc;
}
#endif /* !HAVE_FWRITE_UNLOCKED */


/**
 * Locks the stream.  May have to allocate the lock as well.
 *
 * @param   pStream     The stream (valid).
 */
DECLINLINE(void) rtStrmLock(PRTSTREAM pStream)
{
#ifdef HAVE_FWRITE_UNLOCKED
    flockfile(pStream->pFile);
#else
    if (RT_LIKELY(pStream->pCritSect))
        RTCritSectEnter(pStream->pCritSect);
    else
        rtStrmAllocLock(pStream);
#endif
}


/**
 * Unlocks the stream.
 *
 * @param   pStream     The stream (valid).
 */
DECLINLINE(void) rtStrmUnlock(PRTSTREAM pStream)
{
#ifdef HAVE_FWRITE_UNLOCKED
    funlockfile(pStream->pFile);
#else
    if (RT_LIKELY(pStream->pCritSect))
        RTCritSectLeave(pStream->pCritSect);
#endif
}


/**
 * Opens a file stream.
 *
 * @returns iprt status code.
 * @param   pszFilename     Path to the file to open.
 * @param   pszMode         The open mode. See fopen() standard.
 *                          Format: <a|r|w>[+][b|t]
 * @param   ppStream        Where to store the opened stream.
 */
RTR3DECL(int) RTStrmOpen(const char *pszFilename, const char *pszMode, PRTSTREAM *ppStream)
{
    /*
     * Validate input.
     */
    if (!pszMode || !*pszMode)
    {
        AssertMsgFailed(("No pszMode!\n"));
        return VERR_INVALID_PARAMETER;
    }
    if (!pszFilename)
    {
        AssertMsgFailed(("No pszFilename!\n"));
        return VERR_INVALID_PARAMETER;
    }
    bool fOk = true;
    bool fBinary = false;
    switch (*pszMode)
    {
        case 'a':
        case 'w':
        case 'r':
            switch (pszMode[1])
            {
                case '\0':
                    break;

                case '+':
                    switch (pszMode[2])
                    {
                        case '\0':
                            break;

                        //case 't':
                        //    break;

                        case 'b':
                            fBinary = true;
                            break;

                        default:
                            fOk = false;
                            break;
                    }
                    break;

                //case 't':
                //    break;

                case 'b':
                    fBinary = true;
                    break;

                default:
                    fOk = false;
                    break;
            }
            break;
        default:
            fOk = false;
            break;
    }
    if (!fOk)
    {
        AssertMsgFailed(("Invalid pszMode='%s', '<a|r|w>[+][b|t]'\n", pszMode));
        return VINF_SUCCESS;
    }

    /*
     * Allocate the stream handle and try open it.
     */
    PRTSTREAM pStream = (PRTSTREAM)RTMemAlloc(sizeof(*pStream));
    if (pStream)
    {
        pStream->u32Magic = RTSTREAM_MAGIC;
        pStream->i32Error = VINF_SUCCESS;
        pStream->fCurrentCodeSet = false;
        pStream->fBinary  = fBinary;
        pStream->fRecheckMode = false;
#ifndef HAVE_FWRITE_UNLOCKED
        pStream->pCritSect = NULL;
#endif /* HAVE_FWRITE_UNLOCKED */
        pStream->pFile = fopen(pszFilename, pszMode);
        if (pStream->pFile)
        {
            *ppStream = pStream;
            return VINF_SUCCESS;
        }
        RTMemFree(pStream);
        return RTErrConvertFromErrno(errno);
    }
    return VERR_NO_MEMORY;
}


/**
 * Opens a file stream.
 *
 * @returns iprt status code.
 * @param   pszMode         The open mode. See fopen() standard.
 *                          Format: <a|r|w>[+][b|t]
 * @param   ppStream        Where to store the opened stream.
 * @param   pszFilenameFmt  Filename path format string.
 * @param   args            Arguments to the format string.
 */
RTR3DECL(int) RTStrmOpenFV(const char *pszMode, PRTSTREAM *ppStream, const char *pszFilenameFmt, va_list args)
{
    int     rc;
    char    szFilename[RTPATH_MAX];
    size_t  cch = RTStrPrintfV(szFilename, sizeof(szFilename), pszFilenameFmt, args);
    if (cch < sizeof(szFilename))
        rc = RTStrmOpen(szFilename, pszMode, ppStream);
    else
    {
        AssertMsgFailed(("The filename is too long cch=%d\n", cch));
        rc = VERR_FILENAME_TOO_LONG;
    }
    return rc;
}


/**
 * Opens a file stream.
 *
 * @returns iprt status code.
 * @param   pszMode         The open mode. See fopen() standard.
 *                          Format: <a|r|w>[+][b|t]
 * @param   ppStream        Where to store the opened stream.
 * @param   pszFilenameFmt  Filename path format string.
 * @param   ...             Arguments to the format string.
 */
RTR3DECL(int) RTStrmOpenF(const char *pszMode, PRTSTREAM *ppStream, const char *pszFilenameFmt, ...)
{
    va_list args;
    va_start(args, pszFilenameFmt);
    int rc = RTStrmOpenFV(pszMode, ppStream, pszFilenameFmt, args);
    va_end(args);
    return rc;
}


/**
 * Closes the specified stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream to close.
 */
RTR3DECL(int) RTStrmClose(PRTSTREAM pStream)
{
    if (!pStream)
        return VINF_SUCCESS;
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);

    if (!fclose(pStream->pFile))
    {
        pStream->u32Magic = 0xdeaddead;
        pStream->pFile = NULL;
#ifndef HAVE_FWRITE_UNLOCKED
        if (pStream->pCritSect)
        {
            RTCritSectEnter(pStream->pCritSect);
            RTCritSectLeave(pStream->pCritSect);
            RTCritSectDelete(pStream->pCritSect);
            RTMemFree(pStream->pCritSect);
            pStream->pCritSect = NULL;
        }
#endif
        RTMemFree(pStream);
        return VINF_SUCCESS;
    }

    return RTErrConvertFromErrno(errno);
}


/**
 * Get the pending error of the stream.
 *
 * @returns iprt status code. of the stream.
 * @param   pStream         The stream.
 */
RTR3DECL(int) RTStrmError(PRTSTREAM pStream)
{
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);
    return pStream->i32Error;
}


/**
 * Clears stream error condition.
 *
 * All stream operations save RTStrmClose and this will fail
 * while an error is asserted on the stream
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 */
RTR3DECL(int) RTStrmClearError(PRTSTREAM pStream)
{
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);

    clearerr(pStream->pFile);
    ASMAtomicWriteS32(&pStream->i32Error, VINF_SUCCESS);
    return VINF_SUCCESS;
}


RTR3DECL(int) RTStrmSetMode(PRTSTREAM pStream, int fBinary, int fCurrentCodeSet)
{
    AssertPtrReturn(pStream, VERR_INVALID_HANDLE);
    AssertReturn(pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_HANDLE);
    AssertReturn((unsigned)(fBinary + 1) <= 2, VERR_INVALID_PARAMETER);
    AssertReturn((unsigned)(fCurrentCodeSet + 1) <= 2, VERR_INVALID_PARAMETER);

    rtStrmLock(pStream);

    if (fBinary != -1)
    {
        pStream->fBinary      = RT_BOOL(fBinary);
        pStream->fRecheckMode = true;
    }

    if (fCurrentCodeSet != -1)
        pStream->fCurrentCodeSet = RT_BOOL(fCurrentCodeSet);

    rtStrmUnlock(pStream);

    return VINF_SUCCESS;
}


RTR3DECL(int) RTStrmInputGetEchoChars(PRTSTREAM pStream, bool *pfEchoChars)
{
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pStream, VERR_INVALID_HANDLE);
    AssertReturn(pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_HANDLE);
    AssertPtrReturn(pfEchoChars, VERR_INVALID_POINTER);

    int fh = fileno(pStream->pFile);
    if (isatty(fh))
    {
#ifdef RT_OS_WINDOWS
        DWORD dwMode;
        HANDLE hCon = (HANDLE)_get_osfhandle(fh);
        if (GetConsoleMode(hCon, &dwMode))
            *pfEchoChars = RT_BOOL(dwMode & ENABLE_ECHO_INPUT);
        else
            rc = RTErrConvertFromWin32(GetLastError());
#else
        struct termios Termios;

        int rcPosix = tcgetattr(fh, &Termios);
        if (!rcPosix)
            *pfEchoChars = RT_BOOL(Termios.c_lflag & ECHO);
        else
            rc = RTErrConvertFromErrno(errno);
#endif
    }
    else
        rc = VERR_INVALID_HANDLE;

    return rc;
}


RTR3DECL(int) RTStrmInputSetEchoChars(PRTSTREAM pStream, bool fEchoChars)
{
    int rc = VINF_SUCCESS;

    AssertPtrReturn(pStream, VERR_INVALID_HANDLE);
    AssertReturn(pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_HANDLE);

    int fh = fileno(pStream->pFile);
    if (isatty(fh))
    {
#ifdef RT_OS_WINDOWS
        DWORD dwMode;
        HANDLE hCon = (HANDLE)_get_osfhandle(fh);
        if (GetConsoleMode(hCon, &dwMode))
        {
            if (fEchoChars)
                dwMode |= ENABLE_ECHO_INPUT;
            else
                dwMode &= ~ENABLE_ECHO_INPUT;
            if (!SetConsoleMode(hCon, dwMode))
                rc = RTErrConvertFromWin32(GetLastError());
        }
        else
            rc = RTErrConvertFromWin32(GetLastError());
#else
        struct termios Termios;

        int rcPosix = tcgetattr(fh, &Termios);
        if (!rcPosix)
        {
            if (fEchoChars)
                Termios.c_lflag |= ECHO;
            else
                Termios.c_lflag &= ~ECHO;

            rcPosix = tcsetattr(fh, TCSAFLUSH, &Termios);
            if (rcPosix != 0)
                rc = RTErrConvertFromErrno(errno);
        }
        else
            rc = RTErrConvertFromErrno(errno);
#endif
    }
    else
        rc = VERR_INVALID_HANDLE;

    return rc;
}


RTR3DECL(bool) RTStrmIsTerminal(PRTSTREAM pStream)
{
    AssertPtrReturn(pStream, false);
    AssertReturn(pStream->u32Magic == RTSTREAM_MAGIC, false);

    if (pStream->pFile)
    {
        int fh = fileno(pStream->pFile);
        if (isatty(fh))
        {
#ifdef RT_OS_WINDOWS
            DWORD  dwMode;
            HANDLE hCon = (HANDLE)_get_osfhandle(fh);
            if (GetConsoleMode(hCon, &dwMode))
                return true;
#else
            return true;
#endif
        }
    }
    return false;
}


RTR3DECL(int) RTStrmQueryTerminalWidth(PRTSTREAM pStream, uint32_t *pcchWidth)
{
    AssertPtrReturn(pcchWidth, VERR_INVALID_HANDLE);
    *pcchWidth = 80;

    AssertPtrReturn(pStream, VERR_INVALID_HANDLE);
    AssertReturn(pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_HANDLE);

    if (pStream->pFile)
    {
        int fh = fileno(pStream->pFile);
        if (isatty(fh))
        {
#ifdef RT_OS_WINDOWS
            CONSOLE_SCREEN_BUFFER_INFO Info;
            HANDLE hCon = (HANDLE)_get_osfhandle(fh);
            RT_ZERO(Info);
            if (GetConsoleScreenBufferInfo(hCon, &Info))
            {
                *pcchWidth = Info.dwSize.X ? Info.dwSize.X : 80;
                return VINF_SUCCESS;
            }
            return RTErrConvertFromWin32(GetLastError());

#elif defined(TIOCGWINSZ) || !defined(RT_OS_OS2) /* only OS/2 should currently miss this */
            struct winsize Info;
            RT_ZERO(Info);
            int rc = ioctl(fh, TIOCGWINSZ, &Info);
            if (rc >= 0)
            {
                *pcchWidth = Info.ws_col ? Info.ws_col : 80;
                return VINF_SUCCESS;
            }
            return RTErrConvertFromErrno(errno);
#endif
        }
    }
    return VERR_INVALID_FUNCTION;
}



/**
 * Rewinds the stream.
 *
 * Stream errors will be reset on success.
 *
 * @returns IPRT status code.
 *
 * @param   pStream         The stream.
 *
 * @remarks Not all streams are rewindable and that behavior is currently
 *          undefined for those.
 */
RTR3DECL(int) RTStrmRewind(PRTSTREAM pStream)
{
    AssertPtrReturn(pStream, VERR_INVALID_HANDLE);
    AssertReturn(pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_HANDLE);

    int rc;
    clearerr(pStream->pFile);
    errno = 0;
    if (!fseek(pStream->pFile, 0, SEEK_SET))
    {
        ASMAtomicWriteS32(&pStream->i32Error, VINF_SUCCESS);
        rc = VINF_SUCCESS;
    }
    else
    {
        rc = RTErrConvertFromErrno(errno);
        ASMAtomicWriteS32(&pStream->i32Error, rc);
    }

    return rc;
}


/**
 * Recheck the stream mode.
 *
 * @param   pStream             The stream (locked).
 */
static void rtStreamRecheckMode(PRTSTREAM pStream)
{
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    int fh = fileno(pStream->pFile);
    if (fh >= 0)
    {
        int fExpected = pStream->fBinary ? _O_BINARY : _O_TEXT;
        int fActual   = _setmode(fh, fExpected);
        if (fActual != -1 && fExpected != (fActual & (_O_BINARY | _O_TEXT)))
        {
            fActual = _setmode(fh, fActual & (_O_BINARY | _O_TEXT));
            pStream->fBinary = !(fActual & _O_TEXT);
        }
    }
#else
    NOREF(pStream);
#endif
    pStream->fRecheckMode = false;
}


/**
 * Reads from a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pvBuf           Where to put the read bits.
 *                          Must be cbRead bytes or more.
 * @param   cbRead          Number of bytes to read.
 * @param   pcbRead         Where to store the number of bytes actually read.
 *                          If NULL cbRead bytes are read or an error is returned.
 */
RTR3DECL(int) RTStrmReadEx(PRTSTREAM pStream, void *pvBuf, size_t cbRead, size_t *pcbRead)
{
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);

    int rc = pStream->i32Error;
    if (RT_SUCCESS(rc))
    {
        if (pStream->fRecheckMode)
            rtStreamRecheckMode(pStream);

        if (pcbRead)
        {
            /*
             * Can do with a partial read.
             */
            *pcbRead = fread(pvBuf, 1, cbRead, pStream->pFile);
            if (    *pcbRead == cbRead
                || !ferror(pStream->pFile))
                return VINF_SUCCESS;
            if (feof(pStream->pFile))
            {
                if (*pcbRead)
                    return VINF_EOF;
                rc = VERR_EOF;
            }
            else if (ferror(pStream->pFile))
                rc = VERR_READ_ERROR;
            else
            {
                AssertMsgFailed(("This shouldn't happen\n"));
                rc = VERR_INTERNAL_ERROR;
            }
        }
        else
        {
            /*
             * Must read it all!
             */
            if (fread(pvBuf, cbRead, 1, pStream->pFile) == 1)
                return VINF_SUCCESS;

            /* possible error/eof. */
            if (feof(pStream->pFile))
                rc = VERR_EOF;
            else if (ferror(pStream->pFile))
                rc = VERR_READ_ERROR;
            else
            {
                AssertMsgFailed(("This shouldn't happen\n"));
                rc = VERR_INTERNAL_ERROR;
            }
        }
        ASMAtomicWriteS32(&pStream->i32Error, rc);
    }
    return rc;
}


/**
 * Check if the input text is valid UTF-8.
 *
 * @returns true/false.
 * @param   pvBuf               Pointer to the buffer.
 * @param   cbBuf               Size of the buffer.
 */
static bool rtStrmIsUtf8Text(const void *pvBuf, size_t cbBuf)
{
    NOREF(pvBuf);
    NOREF(cbBuf);
    /** @todo not sure this is a good idea... Better redefine RTStrmWrite. */
    return false;
}


#ifdef RT_OS_WINDOWS
/**
 * Check if the stream is for a Window console.
 *
 * @returns true / false.
 * @param   pStream             The stream.
 * @param   phCon               Where to return the console handle.
 */
static bool rtStrmIsConsoleUnlocked(PRTSTREAM pStream, HANDLE *phCon)
{
    int fh = fileno(pStream->pFile);
    if (isatty(fh))
    {
        DWORD dwMode;
        HANDLE hCon = (HANDLE)_get_osfhandle(fh);
        if (GetConsoleMode(hCon, &dwMode))
        {
            *phCon = hCon;
            return true;
        }
    }
    return false;
}
#endif /* RT_OS_WINDOWS */


/**
 * Internal write API, stream lock already held.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.
 * @param   pvBuf               What to write.
 * @param   cbWrite             How much to write.
 * @param   pcbWritten          Where to optionally return the number of bytes
 *                              written.
 * @param   fSureIsText         Set if we're sure this is UTF-8 text already.
 */
static int rtStrmWriteLocked(PRTSTREAM pStream, const void *pvBuf, size_t cbWrite, size_t *pcbWritten,
                              bool fSureIsText)
{
    int rc = pStream->i32Error;
    if (RT_FAILURE(rc))
        return rc;
    if (pStream->fRecheckMode)
        rtStreamRecheckMode(pStream);

#ifdef RT_OS_WINDOWS
    /*
     * Use the unicode console API when possible in order to avoid stuff
     * getting lost in unnecessary code page translations.
     */
    HANDLE hCon;
    if (rtStrmIsConsoleUnlocked(pStream, &hCon))
    {
# ifdef HAVE_FWRITE_UNLOCKED
        if (!fflush_unlocked(pStream->pFile))
# else
        if (!fflush(pStream->pFile))
# endif
        {
            /** @todo Consider buffering later. For now, we'd rather correct output than
             *        fast output. */
            DWORD    cwcWritten = 0;
            PRTUTF16 pwszSrc = NULL;
            size_t   cwcSrc = 0;
            rc = RTStrToUtf16Ex((const char *)pvBuf, cbWrite, &pwszSrc, 0, &cwcSrc);
            if (RT_SUCCESS(rc))
            {
                if (!WriteConsoleW(hCon, pwszSrc, (DWORD)cwcSrc, &cwcWritten, NULL))
                {
                    /* try write char-by-char to avoid heap problem. */
                    cwcWritten = 0;
                    while (cwcWritten != cwcSrc)
                    {
                        DWORD cwcThis;
                        if (!WriteConsoleW(hCon, &pwszSrc[cwcWritten], 1, &cwcThis, NULL))
                        {
                            if (!pcbWritten || cwcWritten == 0)
                                rc = RTErrConvertFromErrno(GetLastError());
                            break;
                        }
                        if (cwcThis != 1) /* Unable to write current char (amount)? */
                            break;
                        cwcWritten++;
                    }
                }
                if (RT_SUCCESS(rc))
                {
                    if (cwcWritten == cwcSrc)
                    {
                        if (pcbWritten)
                            *pcbWritten = cbWrite;
                    }
                    else if (pcbWritten)
                    {
                        PCRTUTF16   pwszCur = pwszSrc;
                        const char *pszCur  = (const char *)pvBuf;
                        while ((uintptr_t)(pwszCur - pwszSrc) < cwcWritten)
                        {
                            RTUNICP CpIgnored;
                            RTUtf16GetCpEx(&pwszCur, &CpIgnored);
                            RTStrGetCpEx(&pszCur, &CpIgnored);
                        }
                        *pcbWritten = pszCur - (const char *)pvBuf;
                    }
                    else
                        rc = VERR_WRITE_ERROR;
                }
                RTUtf16Free(pwszSrc);
            }
        }
        else
            rc = RTErrConvertFromErrno(errno);
        if (RT_FAILURE(rc))
            ASMAtomicWriteS32(&pStream->i32Error, rc);
        return rc;
    }
#endif /* RT_OS_WINDOWS */

    /*
     * If we're sure it's text output, convert it from UTF-8 to the current
     * code page before printing it.
     *
     * Note! Partial writes are not supported in this scenario because we
     *       cannot easily report back a written length matching the input.
     */
    /** @todo Skip this if the current code set is UTF-8. */
    if (   pStream->fCurrentCodeSet
        && !pStream->fBinary
        && (   fSureIsText
            || rtStrmIsUtf8Text(pvBuf, cbWrite))
       )
    {
        char       *pszSrcFree = NULL;
        const char *pszSrc     = (const char *)pvBuf;
        if (pszSrc[cbWrite])
        {
            pszSrc = pszSrcFree = RTStrDupN(pszSrc, cbWrite);
            if (pszSrc == NULL)
                rc = VERR_NO_STR_MEMORY;
        }
        if (RT_SUCCESS(rc))
        {
            char *pszSrcCurCP;
            rc = RTStrUtf8ToCurrentCP(&pszSrcCurCP, pszSrc);
            if (RT_SUCCESS(rc))
            {
                size_t  cchSrcCurCP = strlen(pszSrcCurCP);
                IPRT_ALIGNMENT_CHECKS_DISABLE(); /* glibc / mempcpy again */
#ifdef HAVE_FWRITE_UNLOCKED
                ssize_t cbWritten = fwrite_unlocked(pszSrcCurCP, cchSrcCurCP, 1, pStream->pFile);
#else
                ssize_t cbWritten = fwrite(pszSrcCurCP, cchSrcCurCP, 1, pStream->pFile);
#endif
                IPRT_ALIGNMENT_CHECKS_ENABLE();
                if (cbWritten == 1)
                {
                    if (pcbWritten)
                        *pcbWritten = cbWrite;
                }
#ifdef HAVE_FWRITE_UNLOCKED
                else if (!ferror_unlocked(pStream->pFile))
#else
                else if (!ferror(pStream->pFile))
#endif
                {
                    if (pcbWritten)
                        *pcbWritten = 0;
                }
                else
                    rc = VERR_WRITE_ERROR;
                RTStrFree(pszSrcCurCP);
            }
            RTStrFree(pszSrcFree);
        }

        if (RT_FAILURE(rc))
            ASMAtomicWriteS32(&pStream->i32Error, rc);
        return rc;
    }

    /*
     * Otherwise, just write it as-is.
     */
    if (pcbWritten)
    {
        IPRT_ALIGNMENT_CHECKS_DISABLE(); /* glibc / mempcpy again */
#ifdef HAVE_FWRITE_UNLOCKED
        *pcbWritten = fwrite_unlocked(pvBuf, 1, cbWrite, pStream->pFile);
#else
        *pcbWritten = fwrite(pvBuf, 1, cbWrite, pStream->pFile);
#endif
        IPRT_ALIGNMENT_CHECKS_ENABLE();
        if (    *pcbWritten == cbWrite
#ifdef HAVE_FWRITE_UNLOCKED
            ||  !ferror_unlocked(pStream->pFile))
#else
            ||  !ferror(pStream->pFile))
#endif
            return VINF_SUCCESS;
        rc = VERR_WRITE_ERROR;
    }
    else
    {
        /* Must write it all! */
        IPRT_ALIGNMENT_CHECKS_DISABLE(); /* glibc / mempcpy again */
#ifdef HAVE_FWRITE_UNLOCKED
        size_t cbWritten = fwrite_unlocked(pvBuf, cbWrite, 1, pStream->pFile);
#else
        size_t cbWritten = fwrite(pvBuf, cbWrite, 1, pStream->pFile);
#endif
        IPRT_ALIGNMENT_CHECKS_ENABLE();
        if (cbWritten == 1)
            return VINF_SUCCESS;
#ifdef HAVE_FWRITE_UNLOCKED
        if (!ferror_unlocked(pStream->pFile))
#else
        if (!ferror(pStream->pFile))
#endif
            return VINF_SUCCESS; /* WEIRD! But anyway... */

        rc = VERR_WRITE_ERROR;
    }
    ASMAtomicWriteS32(&pStream->i32Error, rc);

    return rc;
}


/**
 * Internal write API.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.
 * @param   pvBuf               What to write.
 * @param   cbWrite             How much to write.
 * @param   pcbWritten          Where to optionally return the number of bytes
 *                              written.
 * @param   fSureIsText         Set if we're sure this is UTF-8 text already.
 */
static int rtStrmWrite(PRTSTREAM pStream, const void *pvBuf, size_t cbWrite, size_t *pcbWritten, bool fSureIsText)
{
    rtStrmLock(pStream);
    int rc = rtStrmWriteLocked(pStream, pvBuf, cbWrite, pcbWritten, fSureIsText);
    rtStrmUnlock(pStream);
    return rc;
}


/**
 * Writes to a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pvBuf           Where to get the bits to write from.
 * @param   cbWrite         Number of bytes to write.
 * @param   pcbWritten      Where to store the number of bytes actually written.
 *                          If NULL cbWrite bytes are written or an error is returned.
 */
RTR3DECL(int) RTStrmWriteEx(PRTSTREAM pStream, const void *pvBuf, size_t cbWrite, size_t *pcbWritten)
{
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);
    return rtStrmWrite(pStream, pvBuf, cbWrite, pcbWritten, false);
}


/**
 * Reads a character from a file stream.
 *
 * @returns The char as an unsigned char cast to int.
 * @returns -1 on failure.
 * @param   pStream         The stream.
 */
RTR3DECL(int) RTStrmGetCh(PRTSTREAM pStream)
{
    unsigned char ch;
    int rc = RTStrmReadEx(pStream, &ch, 1, NULL);
    if (RT_SUCCESS(rc))
        return ch;
    return -1;
}


/**
 * Writes a character to a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   ch              The char to write.
 */
RTR3DECL(int) RTStrmPutCh(PRTSTREAM pStream, int ch)
{
    return rtStrmWrite(pStream, &ch, 1, NULL, true /*fSureIsText*/);
}


/**
 * Writes a string to a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pszString       The string to write.
 *                          No newlines or anything is appended or prepended.
 *                          The terminating '\\0' is not written, of course.
 */
RTR3DECL(int) RTStrmPutStr(PRTSTREAM pStream, const char *pszString)
{
    size_t cch = strlen(pszString);
    return rtStrmWrite(pStream, pszString, cch, NULL, true /*fSureIsText*/);
}


RTR3DECL(int) RTStrmGetLine(PRTSTREAM pStream, char *pszString, size_t cbString)
{
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);
    int rc;
    if (pszString && cbString > 1)
    {
        rc = pStream->i32Error;
        if (RT_SUCCESS(rc))
        {
            cbString--;            /* save space for the terminator. */
            rtStrmLock(pStream);
            for (;;)
            {
#ifdef HAVE_FWRITE_UNLOCKED /** @todo darwin + freebsd(?) has fgetc_unlocked but not fwrite_unlocked, optimize... */
                int ch = fgetc_unlocked(pStream->pFile);
#else
                int ch = fgetc(pStream->pFile);
#endif

                /* Deal with \r\n sequences here. We'll return lone CR, but
                   treat CRLF as LF. */
                if (ch == '\r')
                {
#ifdef HAVE_FWRITE_UNLOCKED /** @todo darwin + freebsd(?) has fgetc_unlocked but not fwrite_unlocked, optimize... */
                    ch = fgetc_unlocked(pStream->pFile);
#else
                    ch = fgetc(pStream->pFile);
#endif
                    if (ch == '\n')
                        break;

                    *pszString++ = '\r';
                    if (--cbString <= 0)
                    {
                        /* yeah, this is an error, we dropped a character. */
                        rc = VERR_BUFFER_OVERFLOW;
                        break;
                    }
                }

                /* Deal with end of file. */
                if (ch == EOF)
                {
#ifdef HAVE_FWRITE_UNLOCKED
                    if (feof_unlocked(pStream->pFile))
#else
                    if (feof(pStream->pFile))
#endif
                    {
                        rc = VERR_EOF;
                        break;
                    }
#ifdef HAVE_FWRITE_UNLOCKED
                    if (ferror_unlocked(pStream->pFile))
#else
                    if (ferror(pStream->pFile))
#endif
                        rc = VERR_READ_ERROR;
                    else
                    {
                        AssertMsgFailed(("This shouldn't happen\n"));
                        rc = VERR_INTERNAL_ERROR;
                    }
                    break;
                }

                /* Deal with null terminator and (lone) new line. */
                if (ch == '\0' || ch == '\n')
                    break;

                /* No special character, append it to the return string. */
                *pszString++ = ch;
                if (--cbString <= 0)
                {
                    rc = VINF_BUFFER_OVERFLOW;
                    break;
                }
            }
            rtStrmUnlock(pStream);

            *pszString = '\0';
            if (RT_FAILURE(rc))
                ASMAtomicWriteS32(&pStream->i32Error, rc);
        }
    }
    else
    {
        AssertMsgFailed(("no buffer or too small buffer!\n"));
        rc = VERR_INVALID_PARAMETER;
    }
    return rc;
}


/**
 * Flushes a stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream to flush.
 */
RTR3DECL(int) RTStrmFlush(PRTSTREAM pStream)
{
    if (!fflush(pStream->pFile))
        return VINF_SUCCESS;
    return RTErrConvertFromErrno(errno);
}


/**
 * Output callback.
 *
 * @returns number of bytes written.
 * @param   pvArg       User argument.
 * @param   pachChars   Pointer to an array of utf-8 characters.
 * @param   cchChars    Number of bytes in the character array pointed to by pachChars.
 */
static DECLCALLBACK(size_t) rtstrmOutput(void *pvArg, const char *pachChars, size_t cchChars)
{
    if (cchChars)
        rtStrmWriteLocked((PRTSTREAM)pvArg, pachChars, cchChars, NULL, true /*fSureIsText*/);
    /* else: ignore termination call. */
    return cchChars;
}


/**
 * Prints a formatted string to the specified stream.
 *
 * @returns Number of bytes printed.
 * @param   pStream         The stream to print to.
 * @param   pszFormat       IPRT format string.
 * @param   args            Arguments specified by pszFormat.
 */
RTR3DECL(int) RTStrmPrintfV(PRTSTREAM pStream, const char *pszFormat, va_list args)
{
    AssertReturn(RT_VALID_PTR(pStream) && pStream->u32Magic == RTSTREAM_MAGIC, VERR_INVALID_PARAMETER);
    int rc = pStream->i32Error;
    if (RT_SUCCESS(rc))
    {
        rtStrmLock(pStream);
//        pStream->fShouldFlush = true;
        rc = (int)RTStrFormatV(rtstrmOutput, pStream, NULL, NULL, pszFormat, args);
        rtStrmUnlock(pStream);
        Assert(rc >= 0);
    }
    else
        rc = -1;
    return rc;
}


/**
 * Prints a formatted string to the specified stream.
 *
 * @returns Number of bytes printed.
 * @param   pStream         The stream to print to.
 * @param   pszFormat       IPRT format string.
 * @param   ...             Arguments specified by pszFormat.
 */
RTR3DECL(int) RTStrmPrintf(PRTSTREAM pStream, const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    int rc = RTStrmPrintfV(pStream, pszFormat, args);
    va_end(args);
    return rc;
}


/**
 * Dumper vprintf-like function outputting to a stream.
 *
 * @param   pvUser          The stream to print to.  NULL means standard output.
 * @param   pszFormat       Runtime format string.
 * @param   va              Arguments specified by pszFormat.
 */
RTDECL(void) RTStrmDumpPrintfV(void *pvUser, const char *pszFormat, va_list va)
{
    RTStrmPrintfV(pvUser ? (PRTSTREAM)pvUser : g_pStdOut, pszFormat, va);
}


/**
 * Prints a formatted string to the standard output stream (g_pStdOut).
 *
 * @returns Number of bytes printed.
 * @param   pszFormat       IPRT format string.
 * @param   args            Arguments specified by pszFormat.
 */
RTR3DECL(int) RTPrintfV(const char *pszFormat, va_list args)
{
    return RTStrmPrintfV(g_pStdOut, pszFormat, args);
}


/**
 * Prints a formatted string to the standard output stream (g_pStdOut).
 *
 * @returns Number of bytes printed.
 * @param   pszFormat       IPRT format string.
 * @param   ...             Arguments specified by pszFormat.
 */
RTR3DECL(int) RTPrintf(const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    int rc = RTStrmPrintfV(g_pStdOut, pszFormat, args);
    va_end(args);
    return rc;
}

