/* $Id: kRdrBuffered.cpp 79 2016-07-27 14:25:09Z bird $ */
/** @file
 * kRdrBuffered - Buffered File Provider.
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
#include "kRdrInternal.h"
#include <k/kHlpAlloc.h>
#include <k/kHlpString.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * The buffered file provier instance.
 * This is just a wrapper around another file provider.
 */
typedef struct KRDRBUF
{
    /** The file reader vtable. */
    KRDR                Core;
    /** The actual file provider that we're wrapping. */
    PKRDR               pRdr;
    /** The current file offset. */
    KFOFF               offFile;
    /** The file size. */
    KFOFF               cbFile;
    /** The offset of the buffer. */
    KFOFF               offBuf;
    /** The offset of the end of the buffer. */
    KFOFF               offBufEnd;
    /** The number of valid buffer bytes. */
    KSIZE               cbBufValid;
    /** The size of the buffer. */
    KSIZE               cbBuf;
    /** The buffer. */
    KU8                *pbBuf;
    /** Whether the pRdr instance should be closed together with us or not. */
    KBOOL               fCloseIt;
    /** Set if the buffer has been messed up by kRdrBufLineQ. */
    KBOOL               fTainedByLineQ;
} KRDRBUF, *PKRDRBUF;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void     krdrBufDone(PKRDR pRdr);
static int      krdrBufUnmap(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrBufProtect(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
static int      krdrBufRefresh(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrBufMap(PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
static KSIZE    krdrBufPageSize(PKRDR pRdr);
static const char *krdrBufName(PKRDR pRdr);
static KIPTR    krdrBufNativeFH(PKRDR pRdr);
static KFOFF    krdrBufTell(PKRDR pRdr);
static KFOFF    krdrBufSize(PKRDR pRdr);
static int      krdrBufAllUnmap(PKRDR pRdr, const void *pvBits);
static int      krdrBufAllMap(PKRDR pRdr, const void **ppvBits);
static int      krdrBufRead(PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off);
static int      krdrBufDestroy(PKRDR pRdr);
static int      krdrBufCreate(PPKRDR ppRdr, const char *pszFilename);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Native file provider operations.
 *
 * @remark  This is not in the file provider list as its intended for wrapping
 *          other kRdr instances.
 */
static const KRDROPS g_krdrBufOps =
{
    "Buffered kRdr",
    NULL,
    krdrBufCreate,
    krdrBufDestroy,
    krdrBufRead,
    krdrBufAllMap,
    krdrBufAllUnmap,
    krdrBufSize,
    krdrBufTell,
    krdrBufName,
    krdrBufNativeFH,
    krdrBufPageSize,
    krdrBufMap,
    krdrBufRefresh,
    krdrBufProtect,
    krdrBufUnmap,
    krdrBufDone,
    42
};


/** @copydoc KRDROPS::pfnDone */
static void     krdrBufDone(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnDone(pThis->pRdr);
}


/** @copydoc KRDROPS::pfnUnmap */
static int      krdrBufUnmap(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnUnmap(pThis->pRdr, pvBase, cSegments, paSegments);
}


/** @copydoc KRDROPS::pfnProtect */
static int      krdrBufProtect(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnProtect(pThis->pRdr, pvBase, cSegments, paSegments, fUnprotectOrProtect);
}


/** @copydoc KRDROPS::pfnRefresh */
static int krdrBufRefresh(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnRefresh(pThis->pRdr, pvBase, cSegments, paSegments);
}


/** @copydoc KRDROPS::pfnMap */
static int krdrBufMap(PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnMap(pThis->pRdr, ppvBase, cSegments, paSegments, fFixed);
}


/** @copydoc KRDROPS::pfnPageSize */
static KSIZE krdrBufPageSize(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnPageSize(pThis->pRdr);
}


/** @copydoc KRDROPS::pfnName */
static const char *krdrBufName(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnName(pThis->pRdr);
}


/** @copydoc KRDROPS::pfnNativeFH */
static KIPTR krdrBufNativeFH(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnNativeFH(pThis->pRdr);
}


/** @copydoc KRDROPS::pfnTell */
static KFOFF krdrBufTell(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->offFile;
}


/** @copydoc KRDROPS::pfnSize */
static KFOFF krdrBufSize(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->cbFile;
}


/** @copydoc KRDROPS::pfnAllUnmap */
static int krdrBufAllUnmap(PKRDR pRdr, const void *pvBits)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnAllUnmap(pThis->pRdr, pvBits);
}


/** @copydoc KRDROPS::pfnAllMap */
static int krdrBufAllMap(PKRDR pRdr, const void **ppvBits)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    return pThis->pRdr->pOps->pfnAllMap(pThis->pRdr, ppvBits);
}


/**
 * Fills the buffer with file bits starting at the specified offset.
 *
 * @returns 0 on success, pfnRead error code on failure.
 * @param   pThis   The instance.
 * @param   off     Where to start reading.
 */
static int krdrBufFillBuffer(PKRDRBUF pThis, KFOFF off)
{
    kRdrAssert(off < pThis->cbFile);

    /* Reposition the buffer if it's past the end of the file so that
       we maximize its usability. We leave one unused byte at the end
       of the buffer so kRdrBufLineQ can terminate its string properly.
       Of course, this might end up re-reading a lot of stuff for no
       future gain, but whatever... */
    kRdrAssert(pThis->cbBuf <= pThis->cbFile + 1);
    KFOFF cbLeft = pThis->cbFile - off;
    KSIZE cbRead = pThis->cbBuf;
    if ((KSSIZE)cbRead - 1 >= cbLeft)
    {
        cbRead--;
        off = pThis->cbFile - cbRead;
    }
    int rc = pThis->pRdr->pOps->pfnRead(pThis->pRdr, pThis->pbBuf, cbRead, off);
    if (!rc)
    {
        pThis->offBuf = off;
        pThis->offBufEnd = off + cbRead;
        pThis->cbBufValid = cbRead;
    }
    else
    {
        pThis->offBuf = pThis->offBufEnd = 0;
        pThis->cbBufValid = 0;
    }
    pThis->fTainedByLineQ = K_FALSE;
    return rc;
}


/** @copydoc KRDROPS::pfnRead */
static int krdrBufRead(PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;

    /*
     * We need to validate and update the file offset before
     * we start making partial reads from the buffer and stuff.
     */
    KFOFF offEnd = off + cb;
    if (    off >= pThis->cbFile
        ||  offEnd > pThis->cbFile
        ||  offEnd < off)
        return KERR_OUT_OF_RANGE; /* includes EOF. */
    pThis->offFile = offEnd;
    if (!cb)
        return 0;

    /*
     * Scratch the buffer if kRdrBufLineQ has tained it.
     */
    if (pThis->fTainedByLineQ)
    {
        pThis->offBuf = pThis->offBufEnd = 0;
        pThis->cbBufValid = 0;
    }

    /*
     * Is any part of the request in the buffer?
     *
     * We will currently ignore buffer hits in the middle of the
     * request because it's annoying to implement and it's
     * questionable whether it'll benefit much performance wise.
     */
    if (pThis->cbBufValid > 0)
    {
        if (off >= pThis->offBuf)
        {
            if (off < pThis->offBufEnd)
            {
                /* head (or all) of the request is in the buffer. */
                KSIZE cbMaxChunk = (KSIZE)(pThis->offBufEnd - off);
                KSIZE cbChunk = K_MIN(cb, cbMaxChunk);
                kHlpMemCopy(pvBuf, &pThis->pbBuf[off - pThis->offBuf], cbChunk);
                if (cbChunk == cb)
                    return 0;

                cb -= cbChunk;
                pvBuf = (KU8 *)pvBuf + cbChunk;
                off += cbChunk;
            }
        }
        else if (   offEnd > pThis->offBuf
                 && offEnd <= pThis->offBufEnd)
        {
            /* the end of the request is in the buffer. */
            KSIZE cbChunk = (KSIZE)(pThis->offBufEnd - (offEnd));
            kHlpMemCopy((KU8 *)pvBuf + (pThis->offBuf - off), pThis->pbBuf, cbChunk);
            kRdrAssert(cbChunk < cb);
            cb -= cbChunk;
            offEnd -= cbChunk;
        }
    }

    /*
     * If the buffer is larger than the read request, read a full buffer
     * starting at the requested offset. Otherwise perform an unbuffered
     * read.
     */
    if (pThis->cbBuf > cb)
    {
        int rc = krdrBufFillBuffer(pThis, off);
        if (rc)
            return rc;
        if (pThis->offBuf == off)
            kHlpMemCopy(pvBuf, pThis->pbBuf, cb);
        else
        {
            kRdrAssert(off > pThis->offBuf);
            kRdrAssert(off + cb <= pThis->offBufEnd);
            kHlpMemCopy(pvBuf, pThis->pbBuf + (off - pThis->offBuf), cb);
        }
    }
    else
    {
        int rc = pThis->pRdr->pOps->pfnRead(pThis->pRdr, pvBuf, cb, off);
        if (rc)
            return rc;
    }
    return 0;
}


/** @copydoc KRDROPS::pfnDestroy */
static int krdrBufDestroy(PKRDR pRdr)
{
    PKRDRBUF pThis = (PKRDRBUF)pRdr;

    /* Close the kRdr instance that we're wrapping. */
    if (pThis->fCloseIt)
    {
        int rc = pThis->pRdr->pOps->pfnDestroy(pThis->pRdr);
        if (rc)
            return rc;
        pThis->fCloseIt = K_FALSE;
        pThis->pRdr = NULL;
    }

    kHlpFree(pThis->pbBuf);
    pThis->pbBuf = NULL;
    kHlpFree(pRdr);
    return 0;
}


/** @copydoc KRDROPS::pfnCreate */
static int krdrBufCreate(PPKRDR ppRdr, const char *pszFilename)
{
    K_NOREF(ppRdr);
    K_NOREF(pszFilename);
    return KERR_NOT_IMPLEMENTED;
}


/**
 * Worker for kRdrBufOpen and kRdrBufWrap.
 *
 * It's essentially kRdrBufWrap without error checking.
 *
 * @returns 0 on success, one of the kErrors status code on failure.
 * @param   ppRdr           Where to store the new file provider instance.
 * @param   pRdrWrapped     The file provider instance to buffer.
 * @param   fCloseIt        Whether it the pRdrWrapped instance should be closed
 *                          when the new instance is closed.
 */
static int krdrBufWrapIt(PPKRDR ppRdr, PKRDR pRdrWrapped, KBOOL fCloseIt)
{
    PKRDRBUF pThis = (PKRDRBUF)kHlpAlloc(sizeof(*pThis));
    if (pThis)
    {
        pThis->Core.u32Magic = KRDR_MAGIC;
        pThis->Core.pOps = &g_krdrBufOps;
        pThis->pRdr = pRdrWrapped;
        pThis->offFile = pRdrWrapped->pOps->pfnTell(pRdrWrapped);
        pThis->cbFile = pRdrWrapped->pOps->pfnSize(pRdrWrapped);
        pThis->offBuf = pThis->offBufEnd = 0;
        pThis->cbBufValid = 0;
        pThis->fCloseIt = fCloseIt;
        pThis->fTainedByLineQ = K_FALSE;
        if (pThis->cbFile < 128*1024)
            pThis->cbBuf = (KSIZE)pThis->cbFile + 1; /* need space for the kRdrBufLineQ terminator. */
        else
            pThis->cbBuf = 64*1024;
        pThis->pbBuf = (KU8 *)kHlpAlloc(pThis->cbBuf);
        if (pThis->pbBuf)
        {
            *ppRdr = &pThis->Core;
            return 0;
        }

        pThis->Core.u32Magic = 0;
        kHlpFree(pThis);
    }
    return KERR_NO_MEMORY;
}


/**
 * Opens a file provider with a buffered wrapper.
 *
 * @returns 0 on success, KERR_* on failure.
 * @param   ppRdr       Where to store the buffered file reader instance on success.
 * @param   pszFilename The name of the file that should be opened.
 */
KRDR_DECL(int) kRdrBufOpen(PPKRDR ppRdr, const char *pszFilename)
{
    kRdrAssertPtrReturn(ppRdr, KERR_INVALID_POINTER);
    *ppRdr = NULL;

    PKRDR pRdrWrapped;
    int rc = kRdrOpen(&pRdrWrapped, pszFilename);
    if (!rc)
    {
        rc = krdrBufWrapIt(ppRdr, pRdrWrapped, K_TRUE);
        if (rc)
            kRdrClose(pRdrWrapped);
    }
    return rc;
}


/**
 * Creates a buffered file provider instance for an existing one.
 *
 * @returns 0 on success, KERR_* on failure.
 * @param   ppRdr           Where to store the new file provider pointer.
 * @param   pRdr            The file provider instance to wrap.
 * @param   fCLoseIt        Whether it the wrapped reader should be automatically
 *                          closed when the wrapper closes.
 */
KRDR_DECL(int) kRdrBufWrap(PPKRDR ppRdr, PKRDR pRdr, KBOOL fCloseIt)
{
    KRDR_VALIDATE(pRdr);
    return krdrBufWrapIt(ppRdr, pRdr, fCloseIt);
}


/**
 * Checks whether the file provider instance is of the buffered type or not.
 *
 * @returns K_TRUE if it is, otherwise K_FALSE.
 * @param   pRdr            The file provider instance to check.
 */
KRDR_DECL(KBOOL) kRdrBufIsBuffered(PKRDR pRdr)
{
    KRDR_VALIDATE_EX(pRdr, K_FALSE);
    return pRdr->pOps == &g_krdrBufOps;
}


/**
 * Reads a line from a buffered file provider.
 *
 * The trailing '\n' or '\r\n' is stripped.
 *
 * @returns 0 on success. KERR_* on failure.
 * @retval  KRDR_ERR_LINE_TOO_LONG if the line is too long to fit in the passed in buffer.
 * @retval  KRDR_ERR_NOT_BUFFERED_RDR if pRdr isn't a buffered reader.
 * @param   pRdr        The buffered file reader.
 * @param   pszLine     Where to store the line.
 * @param   cbLine      The size of the the line buffer.
 */
KRDR_DECL(int) kRdrBufLine(PKRDR pRdr, char *pszLine, KSIZE cbLine)
{
    return kRdrBufLineEx(pRdr, pszLine, &cbLine);
}


/**
 * Reads a line from a buffered file provider.
 *
 * The trailing '\n' or '\r\n' is stripped.
 *
 * @returns 0 on success. KERR_* on failure.
 * @retval  KRDR_ERR_LINE_TOO_LONG if the line is too long to fit in the passed in buffer.
 * @retval  KRDR_ERR_NOT_BUFFERED_RDR if pRdr isn't a buffered reader.
 * @param   pRdr        The buffered file reader.
 * @param   pszLine     Where to store the line.
 * @param   pcbLine     The size of the the line buffer on input, the length of the
 *                      returned line on output.
 */
KRDR_DECL(int) kRdrBufLineEx(PKRDR pRdr, char *pszLine, KSIZE *pcbLine)
{
    /*
     * Validate input.
     */
    kRdrAssertPtrReturn(pcbLine, KERR_INVALID_POINTER);
    KSIZE cbLeft = *pcbLine;
    *pcbLine = 0;
    kRdrAssertReturn(cbLeft > 0, KERR_INVALID_PARAMETER);
    KRDR_VALIDATE(pRdr);
    kRdrAssertReturn(pRdr->pOps != &g_krdrBufOps, KRDR_ERR_NOT_BUFFERED_RDR);
    kRdrAssertPtrReturn(pszLine, KERR_INVALID_POINTER);

    /* check for EOF */
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    if (pThis->offFile >= pThis->cbFile)
    {
        kRdrAssert(pThis->offFile == pThis->cbFile);
        *pszLine = '\0';
        *pcbLine = 0;
        return KERR_EOF;
    }

    /*
     * Scratch the buffer if kRdrBufLineQ has tained it.
     */
    if (pThis->fTainedByLineQ)
    {
        pThis->offBuf = pThis->offBufEnd = 0;
        pThis->cbBufValid = 0;
    }

    /*
     * Buffered read loop.
     *
     * The overflow logic is a bit fishy wrt to overflowing at an "\r\n"
     * that arrives at a buffer boundrary. The current policy is to try
     * our best to not to fail with overflow in the EOL sequence or EOF.
     * If it's the end of the buffer, it will not be refilled just to
     * check for this because that's too much work.
     */
    cbLeft--; /* reserve space for the terminator. */
    char *pszOut = pszLine;
    for (;;)
    {
        /*
         * Do we need to (re-)fill the buffer or does it contain something
         * that we can work on already?
         */
        if (    !pThis->cbBufValid
            ||  pThis->offFile >= pThis->offBufEnd
            ||  pThis->offFile < pThis->offBuf)
        {
            int rc = krdrBufFillBuffer(pThis, pThis->offFile);
            if (rc)
            {
                *pszOut = '\0';
                return rc;
            }
        }

        /*
         * Parse the buffer looking for the EOL indicator.
         */
        kRdrAssert(pThis->offFile >= pThis->offBuf && pThis->offFile < pThis->offBufEnd);
        kRdrAssert(sizeof(char) == sizeof(*pThis->pbBuf));
        const char * const pszStart = (const char *)&pThis->pbBuf[pThis->offFile - pThis->offBuf];
        const char * const pszEnd   = (const char *)&pThis->pbBuf[pThis->cbBufValid];
        const char *psz = pszStart;
        while (psz < pszEnd)
        {
            const char ch = *psz;
            if (ch == '\n')
            {
                /* found the EOL, update file position and line length. */
                pThis->offFile += psz - pszStart + 1;
                *pcbLine += psz - pszStart;

                /* terminate the string, checking for "\r\n" first. */
                if (    *pcbLine
                    &&  pszOut[-1] == '\r')
                {
                    *pcbLine -= 1;
                    pszOut--;
                }
                *pszOut = '\0';
                return 0;
            }
            if (!cbLeft)
            {
                /* the line is *probably* too long. */
                pThis->offFile += psz - pszStart;
                *pcbLine += psz - pszStart;
                *pszOut = '\0';

                /* The only possible case where the line actually isn't too long
                   is if we're at a "\r\n" sequence. We will re-fill the buffer
                   if necessary to check for the '\n' as it's not that much work. */
                if (    ch == '\r'
                    &&  pThis->offFile + 2 <= pThis->cbFile)
                {
                    if (psz + 1 >= pszEnd)
                    {
                        int rc = krdrBufFillBuffer(pThis, pThis->offFile);
                        if (rc)
                        {
                            *pszOut = '\0';
                            return rc;
                        }
                    }
                    psz = (const char *)&pThis->pbBuf[pThis->offFile - pThis->offBuf];
                    kRdrAssert(*psz == '\r');
                    if (psz[1] == '\n')
                    {
                        *pcbLine -= 1;
                        pszOut[-1] = '\0';
                        pThis->offFile += 2;
                        return 0;
                    }
                }
                return KRDR_ERR_LINE_TOO_LONG;
            }

            /* copy and advance */
            *pszOut++ = ch;
            cbLeft--;
            psz++;
        }

        /* advance past the buffer and check for EOF. */
        *pcbLine += pszEnd - pszStart;
        pThis->offFile = pThis->offBufEnd;
        if (pThis->offFile >= pThis->cbFile)
        {
            kRdrAssert(pThis->offFile == pThis->cbFile);
            *pszOut = '\0';
            return 0;
        }
    }
}


/**
 * Worker for kRdrBufLineQ that searches the current buffer for EOL or EOF.
 *
 * When a EOF marker is found
 *
 *
 * @returns NULL if EOL/EOF isn't found the buffer.
 * @param   pThis   The buffered reader instance.
 */
static const char * krdrBufLineQWorker(PKRDRBUF pThis)
{
    kRdrAssert(pThis->offFile >= pThis->offBuf && pThis->offFile < pThis->offBufEnd);

    /*
     * Search the buffer.
     */
    kRdrAssert(sizeof(char) == sizeof(*pThis->pbBuf));
    const char * const pszStart = (const char *)&pThis->pbBuf[pThis->offFile - pThis->offBuf];
    const char * const pszEnd   = (const char *)&pThis->pbBuf[pThis->cbBufValid];
    char *psz = (char *)pszStart;
    while (psz < pszEnd)
    {
        char ch = *psz;
        if (ch == '\n')
        {
            pThis->offFile += psz - pszStart;
            pThis->fTainedByLineQ = K_TRUE;
            *psz = '\0';
            if (    psz > pszStart
                &&  psz[-1] == '\r')
                *--psz = '\0';
            return pszStart;
        }
        psz++;
    }

    /*
     * Check for EOF. There must be room for a terminator char here.
     */
    if (    pThis->offBufEnd >= pThis->cbFile
        &&  (pThis->offBufEnd - pThis->offBuf) < (KSSIZE)pThis->cbBuf)
    {
        pThis->offFile = pThis->cbFile;
        pThis->pbBuf[pThis->cbBufValid] = '\0';
        return pszStart;
    }

    return NULL;
}


/**
 * Get the pointer to the next next line in the buffer.
 * The returned line is zero terminated.
 *
 * @returns A pointer to the line on success. This becomes invalid
 *          upon the next call to this kRdr instance.
 * @returns NULL on EOF, read error of if the line was too long.
 * @param   pRdr        The buffered file reader.
 */
KRDR_DECL(const char *) kRdrBufLineQ(PKRDR pRdr)
{
    /*
     * Validate input.
     */
    KRDR_VALIDATE_EX(pRdr, NULL);
    kRdrAssertReturn(pRdr->pOps != &g_krdrBufOps, NULL);

    /* check for EOF */
    PKRDRBUF pThis = (PKRDRBUF)pRdr;
    if (pThis->offFile >= pThis->cbFile)
    {
        kRdrAssert(pThis->offFile == pThis->cbFile);
        return NULL;
    }

    /*
     * Search the current buffer if possible
     */
    if (    pThis->cbBufValid
        &&  pThis->offFile >= pThis->offBuf
        &&  pThis->offFile < pThis->offBufEnd)
    {
        const char *psz = krdrBufLineQWorker(pThis);
        if (psz)
            return psz;
    }

    /*
     * Fill the buffer in an optimal way and look for the EOL/EOF (again).
     */
    int rc = krdrBufFillBuffer(pThis, pThis->offFile);
    if (rc)
        return NULL;
    return krdrBufLineQWorker(pThis);
}

