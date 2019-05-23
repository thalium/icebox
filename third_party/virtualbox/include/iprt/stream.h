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

#ifndef ___iprt_stream_h
#define ___iprt_stream_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_stream     RTStrm - File Streams
 * @ingroup grp_rt
 * @{
 */

/** Pointer to a stream. */
typedef struct RTSTREAM *PRTSTREAM;

/** Pointer to the standard input stream. */
extern RTDATADECL(PRTSTREAM)    g_pStdIn;

/** Pointer to the standard error stream. */
extern RTDATADECL(PRTSTREAM)    g_pStdErr;

/** Pointer to the standard output stream. */
extern RTDATADECL(PRTSTREAM)    g_pStdOut;


/**
 * Opens a file stream.
 *
 * @returns iprt status code.
 * @param   pszFilename     Path to the file to open.
 * @param   pszMode         The open mode. See fopen() standard.
 *                          Format: <a|r|w>[+][b|t]
 * @param   ppStream        Where to store the opened stream.
 */
RTR3DECL(int) RTStrmOpen(const char *pszFilename, const char *pszMode, PRTSTREAM *ppStream);

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
RTR3DECL(int) RTStrmOpenFV(const char *pszMode, PRTSTREAM *ppStream, const char *pszFilenameFmt,
                           va_list args) RT_IPRT_FORMAT_ATTR(3, 0);

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
RTR3DECL(int) RTStrmOpenF(const char *pszMode, PRTSTREAM *ppStream, const char *pszFilenameFmt, ...) RT_IPRT_FORMAT_ATTR(3, 4);

/**
 * Closes the specified stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream to close.
 */
RTR3DECL(int) RTStrmClose(PRTSTREAM pStream);

/**
 * Get the pending error of the stream.
 *
 * @returns iprt status code. of the stream.
 * @param   pStream         The stream.
 */
RTR3DECL(int) RTStrmError(PRTSTREAM pStream);

/**
 * Clears stream error condition.
 *
 * All stream operations save RTStrmClose and this will fail
 * while an error is asserted on the stream
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 */
RTR3DECL(int) RTStrmClearError(PRTSTREAM pStream);

/**
 * Changes the stream mode.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   fBinary         The desired binary (@c true) / text mode (@c false).
 *                          Pass -1 to leave it unchanged.
 * @param   fCurrentCodeSet Whether converting the stream from UTF-8 to the
 *                          current code set is desired (@c true) or not (@c
 *                          false).  Pass -1 to leave this property unchanged.
 */
RTR3DECL(int) RTStrmSetMode(PRTSTREAM pStream, int fBinary, int fCurrentCodeSet);

/**
 * Returns the current echo mode.
 * This works only for standard input streams.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pfEchoChars     Where to store the flag whether typed characters are echoed.
 */
RTR3DECL(int) RTStrmInputGetEchoChars(PRTSTREAM pStream, bool *pfEchoChars);

/**
 * Changes the behavior for echoing inpit characters on the command line.
 * This works only for standard input streams.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   fEchoChars      Flag whether echoing typed characters is wanted.
 */
RTR3DECL(int) RTStrmInputSetEchoChars(PRTSTREAM pStream, bool fEchoChars);

/**
 * Checks if this is a terminal (TTY) or not.
 *
 * @returns true if it is, false if it isn't or the stream isn't valid.
 * @param   pStream         The stream.
 */
RTR3DECL(bool) RTStrmIsTerminal(PRTSTREAM pStream);

/**
 * Gets the width of the terminal the stream is associated with.
 *
 * @returns IPRT status code.
 * @retval  VERR_INVALID_FUNCTION if not connected to a terminal.
 * @param   pStream         The stream.
 * @param   pcchWidth       Where to return the width.  This will never be zero
 *                          and always be set, even on error.
 */
RTR3DECL(int) RTStrmQueryTerminalWidth(PRTSTREAM pStream, uint32_t *pcchWidth);

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
RTR3DECL(int) RTStrmRewind(PRTSTREAM pStream);

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
RTR3DECL(int) RTStrmReadEx(PRTSTREAM pStream, void *pvBuf, size_t cbRead, size_t *pcbRead);

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
RTR3DECL(int) RTStrmWriteEx(PRTSTREAM pStream, const void *pvBuf, size_t cbWrite, size_t *pcbWritten);

/**
 * Reads from a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pvBuf           Where to put the read bits.
 *                          Must be cbRead bytes or more.
 * @param   cbRead          Number of bytes to read.
 */
DECLINLINE(int) RTStrmRead(PRTSTREAM pStream, void *pvBuf, size_t cbRead)
{
    return RTStrmReadEx(pStream, pvBuf, cbRead, NULL);
}

/**
 * Writes to a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pvBuf           Where to get the bits to write from.
 * @param   cbWrite         Number of bytes to write.
 */
DECLINLINE(int) RTStrmWrite(PRTSTREAM pStream, const void *pvBuf, size_t cbWrite)
{
    return RTStrmWriteEx(pStream, pvBuf, cbWrite, NULL);
}

/**
 * Reads a character from a file stream.
 *
 * @returns The char as an unsigned char cast to int.
 * @returns -1 on failure.
 * @param   pStream         The stream.
 */
RTR3DECL(int) RTStrmGetCh(PRTSTREAM pStream);

/**
 * Writes a character to a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   ch              The char to write.
 */
RTR3DECL(int) RTStrmPutCh(PRTSTREAM pStream, int ch);

/**
 * Writes a string to a file stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream.
 * @param   pszString       The string to write.
 *                          No newlines or anything are appended or prepended.
 *                          The terminating '\\0' is not written, of course.
 */
RTR3DECL(int) RTStrmPutStr(PRTSTREAM pStream, const char *pszString);

/**
 * Reads a line from a file stream.
 *
 * A line ends with a '\\n', '\\r\\n', '\\0' or the end of the file.
 *
 * @returns iprt status code.
 * @retval  VINF_BUFFER_OVERFLOW if the buffer wasn't big enough to read an
 *          entire line.
 * @retval  VERR_BUFFER_OVERFLOW if a lone '\\r' was encountered at the end of
 *          the buffer and we ended up dropping the following character.
 *
 * @param   pStream         The stream.
 * @param   pszString       Where to store the line.
 *                          The line will *NOT* contain any '\\n'.
 * @param   cbString        The size of the string buffer.
 */
RTR3DECL(int) RTStrmGetLine(PRTSTREAM pStream, char *pszString, size_t cbString);

/**
 * Flushes a stream.
 *
 * @returns iprt status code.
 * @param   pStream         The stream to flush.
 */
RTR3DECL(int) RTStrmFlush(PRTSTREAM pStream);

/**
 * Prints a formatted string to the specified stream.
 *
 * @returns Number of bytes printed.
 * @param   pStream         The stream to print to.
 * @param   pszFormat       Runtime format string.
 * @param   ...             Arguments specified by pszFormat.
 */
RTR3DECL(int) RTStrmPrintf(PRTSTREAM pStream, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(2, 3);

/**
 * Prints a formatted string to the specified stream.
 *
 * @returns Number of bytes printed.
 * @param   pStream         The stream to print to.
 * @param   pszFormat       Runtime format string.
 * @param   args            Arguments specified by pszFormat.
 */
RTR3DECL(int) RTStrmPrintfV(PRTSTREAM pStream, const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * Dumper vprintf-like function outputting to a stream.
 *
 * @param   pvUser          The stream to print to.  NULL means standard output.
 * @param   pszFormat       Runtime format string.
 * @param   va              Arguments specified by pszFormat.
 */
RTR3DECL(void) RTStrmDumpPrintfV(void *pvUser, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * Prints a formatted string to the standard output stream (g_pStdOut).
 *
 * @returns Number of bytes printed.
 * @param   pszFormat       Runtime format string.
 * @param   ...             Arguments specified by pszFormat.
 */
RTR3DECL(int) RTPrintf(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Prints a formatted string to the standard output stream (g_pStdOut).
 *
 * @returns Number of bytes printed.
 * @param   pszFormat       Runtime format string.
 * @param   args            Arguments specified by pszFormat.
 */
RTR3DECL(int) RTPrintfV(const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(1, 0);

/** @} */

RT_C_DECLS_END

#endif

