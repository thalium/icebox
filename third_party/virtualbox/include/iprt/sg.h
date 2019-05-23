/** @file
 * IPRT - S/G buffer handling.
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

#ifndef ___iprt_sg_h
#define ___iprt_sg_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/**
 * A S/G entry.
 */
typedef struct RTSGSEG
{
    /** Pointer to the segment buffer. */
    void   *pvSeg;
    /** Size of the segment buffer. */
    size_t  cbSeg;
} RTSGSEG;
/** Pointer to a S/G entry. */
typedef RTSGSEG *PRTSGSEG;
/** Pointer to a const S/G entry. */
typedef const RTSGSEG *PCRTSGSEG;
/** Pointer to a S/G entry pointer. */
typedef PRTSGSEG *PPRTSGSEG;

/**
 * A S/G buffer.
 *
 * The members should be treated as private.
 */
typedef struct RTSGBUF
{
    /** Pointer to the scatter/gather array. */
    PCRTSGSEG paSegs;
    /** Number of segments. */
    unsigned  cSegs;
    /** Current segment we are in. */
    unsigned  idxSeg;
    /** Pointer to the current segment start. */
    void     *pvSegCur;
    /** Number of bytes left in the current buffer. */
    size_t    cbSegLeft;
} RTSGBUF;
/** Pointer to a S/G entry. */
typedef RTSGBUF *PRTSGBUF;
/** Pointer to a const S/G entry. */
typedef const RTSGBUF *PCRTSGBUF;
/** Pointer to a S/G entry pointer. */
typedef PRTSGBUF *PPRTSGBUF;

/**
 * Initialize a S/G buffer structure.
 *
 * @returns nothing.
 * @param   pSgBuf    Pointer to the S/G buffer to initialize.
 * @param   paSegs    Pointer to the start of the segment array.
 * @param   cSegs     Number of segments in the array.
 *
 * @note paSegs and cSegs can be NULL and 0 respectively to indicate
 *       an empty S/G buffer. All operations on the S/G buffer will
 *       not do anything in this case.
 */
RTDECL(void) RTSgBufInit(PRTSGBUF pSgBuf, PCRTSGSEG paSegs, size_t cSegs);

/**
 * Resets the internal buffer position of the S/G buffer to the beginning.
 *
 * @returns nothing.
 * @param   pSgBuf    The S/G buffer to reset.
 */
RTDECL(void) RTSgBufReset(PRTSGBUF pSgBuf);

/**
 * Clones a given S/G buffer.
 *
 * @returns nothing.
 * @param   pSgBufNew    The new S/G buffer to clone to.
 * @param   pSgBufOld    The source S/G buffer to clone from.
 *
 * @note This is only a shallow copy. Both S/G buffers will point to the
 *       same segment array.
 */
RTDECL(void) RTSgBufClone(PRTSGBUF pSgBufNew, PCRTSGBUF pSgBufOld);

/**
 * Returns the next segment in the S/G buffer or NULL if no segment is left.
 *
 * @returns Pointer to the next segment in the S/G buffer.
 * @param   pSgBuf       The S/G buffer.
 * @param   pcbSeg       Where to store the size of the returned segment.
 *                       Holds the number of bytes requested initially or 0 to
 *                       indicate that the size doesn't matter.
 *                       This may contain fewer bytes on success if the current segment
 *                       is smaller than the amount of bytes requested.
 *
 * @note This operation advances the internal buffer pointer of both S/G buffers.
 */
RTDECL(void *) RTSgBufGetNextSegment(PRTSGBUF pSgBuf, size_t *pcbSeg);

/**
 * Copy data between two S/G buffers.
 *
 * @returns The number of bytes copied.
 * @param   pSgBufDst    The destination S/G buffer.
 * @param   pSgBufSrc    The source S/G buffer.
 * @param   cbCopy       Number of bytes to copy.
 *
 * @note This operation advances the internal buffer pointer of both S/G buffers.
 */
RTDECL(size_t) RTSgBufCopy(PRTSGBUF pSgBufDst, PRTSGBUF pSgBufSrc, size_t cbCopy);

/**
 * Compares the content of two S/G buffers.
 *
 * @returns Whatever memcmp returns.
 * @param   pSgBuf1      First S/G buffer.
 * @param   pSgBuf2      Second S/G buffer.
 * @param   cbCmp        How many bytes to compare.
 *
 * @note This operation doesn't change the internal position of the S/G buffers.
 */
RTDECL(int) RTSgBufCmp(PCRTSGBUF pSgBuf1, PCRTSGBUF pSgBuf2, size_t cbCmp);

/**
 * Compares the content of two S/G buffers - advanced version.
 *
 * @returns Whatever memcmp returns.
 * @param   pSgBuf1      First S/G buffer.
 * @param   pSgBuf2      Second S/G buffer.
 * @param   cbCmp        How many bytes to compare.
 * @param   poffDiff     Where to store the offset of the first different byte
 *                       in the buffer starting from the position of the S/G
 *                       buffer before this call.
 * @param   fAdvance     Flag whether the internal buffer position should be advanced.
 *
 */
RTDECL(int) RTSgBufCmpEx(PRTSGBUF pSgBuf1, PRTSGBUF pSgBuf2, size_t cbCmp, size_t *poffDiff, bool fAdvance);

/**
 * Fills an S/G buf with a constant byte.
 *
 * @returns The number of actually filled bytes.
 *          Can be less than than cbSet if the end of the S/G buffer was reached.
 * @param   pSgBuf       The S/G buffer.
 * @param   ubFill       The byte to fill the buffer with.
 * @param   cbSet        How many bytes to set.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufSet(PRTSGBUF pSgBuf, uint8_t ubFill, size_t cbSet);

/**
 * Copies data from an S/G buffer into a given non scattered buffer.
 *
 * @returns Number of bytes copied.
 * @param   pSgBuf       The S/G buffer to copy from.
 * @param   pvBuf        Buffer to copy the data into.
 * @param   cbCopy       How many bytes to copy.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufCopyToBuf(PRTSGBUF pSgBuf, void *pvBuf, size_t cbCopy);

/**
 * Copies data from a non scattered buffer into an S/G buffer.
 *
 * @returns Number of bytes copied.
 * @param   pSgBuf       The S/G buffer to copy to.
 * @param   pvBuf        Buffer to copy the data from.
 * @param   cbCopy       How many bytes to copy.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer.
 */
RTDECL(size_t) RTSgBufCopyFromBuf(PRTSGBUF pSgBuf, const void *pvBuf, size_t cbCopy);

/**
 * Advances the internal buffer pointer.
 *
 * @returns Number of bytes the pointer was moved forward.
 * @param   pSgBuf      The S/G buffer.
 * @param   cbAdvance   Number of bytes to move forward.
 */
RTDECL(size_t) RTSgBufAdvance(PRTSGBUF pSgBuf, size_t cbAdvance);

/**
 * Constructs a new segment array starting from the current position
 * and describing the given number of bytes.
 *
 * @returns Number of bytes the array describes.
 * @param   pSgBuf      The S/G buffer.
 * @param   paSeg       The uninitialized segment array.
 *                      If NULL pcSeg will contain the number of segments needed
 *                      to describe the requested amount of data.
 * @param   pcSeg       The number of segments the given array has.
 *                      This will hold the actual number of entries needed upon return.
 * @param   cbData      Number of bytes the new array should describe.
 *
 * @note This operation advances the internal buffer pointer of the S/G buffer if paSeg is not NULL.
 */
RTDECL(size_t) RTSgBufSegArrayCreate(PRTSGBUF pSgBuf, PRTSGSEG paSeg, unsigned *pcSeg, size_t cbData);

/**
 * Returns whether the given S/G buffer is zeroed out from the current position
 * upto the number of bytes to check.
 *
 * @returns true if the buffer has only zeros
 *          false otherwise.
 * @param   pSgBuf      The S/G buffer.
 * @param   cbCheck     Number of bytes to check.
 */
RTDECL(bool) RTSgBufIsZero(PRTSGBUF pSgBuf, size_t cbCheck);

/**
 * Maps the given S/G buffer to a segment array of another type (for example to
 * iovec on POSIX or WSABUF on Windows).
 *
 * @param   paMapped    Where to store the pointer to the start of the native
 *                      array or NULL.  The memory needs to be freed with
 *                      RTMemTmpFree().
 * @param   pSgBuf      The S/G buffer to map.
 * @param   Struct      Struct used as the destination.
 * @param   pvBufField  Name of the field holding the pointer to a buffer.
 * @param   TypeBufPtr  Type of the buffer pointer.
 * @param   cbBufField  Name of the field holding the size of the buffer.
 * @param   TypeBufSize Type of the field for the buffer size.
 * @param   cSegsMapped Where to store the number of segments the native array
 *                      has.
 *
 * @note    This operation maps the whole S/G buffer starting at the current
 *          internal position.  The internal buffer position is unchanged by
 *          this operation.
 *
 * @remark  Usage is a bit ugly but saves a few lines of duplicated code
 *          somewhere else and makes it possible to keep the S/G buffer members
 *          private without going through RTSgBufSegArrayCreate() first.
 */
#define RTSgBufMapToNative(paMapped, pSgBuf, Struct, pvBufField, TypeBufPtr, cbBufField, TypeBufSize, cSegsMapped) \
    do \
    { \
        AssertCompileMemberSize(Struct, pvBufField, RT_SIZEOFMEMB(RTSGSEG, pvSeg)); \
        /*AssertCompile(RT_SIZEOFMEMB(Struct, cbBufField) >=  RT_SIZEOFMEMB(RTSGSEG, cbSeg));*/ \
        (cSegsMapped) = (pSgBuf)->cSegs - (pSgBuf)->idxSeg; \
        \
        /* We need room for at least one segment. */ \
        if ((pSgBuf)->cSegs == (pSgBuf)->idxSeg) \
            (cSegsMapped)++; \
        \
        (paMapped) = (Struct *)RTMemTmpAllocZ((cSegsMapped) * sizeof(Struct)); \
        if ((paMapped)) \
        { \
            /* The first buffer is special because we could be in the middle of a segment. */ \
            (paMapped)[0].pvBufField = (TypeBufPtr)(pSgBuf)->pvSegCur; \
            (paMapped)[0].cbBufField = (TypeBufSize)(pSgBuf)->cbSegLeft; \
            \
            for (unsigned i = 1; i < (cSegsMapped); i++) \
            { \
                (paMapped)[i].pvBufField = (TypeBufPtr)(pSgBuf)->paSegs[(pSgBuf)->idxSeg + i].pvSeg; \
                (paMapped)[i].cbBufField = (TypeBufSize)(pSgBuf)->paSegs[(pSgBuf)->idxSeg + i].cbSeg; \
            } \
        } \
    } while (0)

RT_C_DECLS_END

/** @} */

#endif

