/* $Id: VDDbgIoLog.cpp $ */
/** @file
 *
 * VD Debug library - I/O logger.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#define LOGGROUP LOGGROUP_DEFAULT
#include <VBox/vddbg.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/mem.h>
#include <iprt/memcache.h>
#include <iprt/file.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/asm.h>


/*********************************************************************************************************************************
*   Structures in a I/O log file, little endian                                                                                  *
*********************************************************************************************************************************/

/**
 * I/O log header.
 */
#pragma pack(1)
typedef struct IoLogHeader
{
    /** Magic string */
    char        szMagic[8];
    /** Flags for the log file. */
    uint32_t    fFlags;
    /** Id counter. */
    uint64_t    u64Id;
} IoLogHeader;
#pragma pack()

#define VDIOLOG_MAGIC "VDIOLOG"

/** Event type - I/O request start. */
#define VDIOLOG_EVENT_START    0x01
/** Event type - I/O request complete. */
#define VDIOLOG_EVENT_COMPLETE 0x02

/**
 * I/O log entry marking the start of a new I/O transaction.
 */
#pragma pack(1)
typedef struct IoLogEntryStart
{
    /** Event type. */
    uint32_t    u32Type;
    /** Transfer type. */
    uint32_t    u32ReqType;
    /** Flag whether this is a sync or async request. */
    uint8_t     u8AsyncIo;
    /** Id of the entry. */
    uint64_t    u64Id;
    /** Type dependent data. */
    union
    {
        /** I/O. */
        struct
        {
            /** Start offset. */
            uint64_t    u64Off;
            /** Size of the request. */
            uint64_t    u64IoSize;
        } Io;
        /** Discard */
        struct
        {
            /** Number of ranges to discard. */
            uint32_t    cRanges;
        } Discard;
    };
} IoLogEntryStart;
#pragma pack()

/**
 * I/O log entry markign the completion of an I/O transaction.
 */
#pragma pack(1)
typedef struct IoLogEntryComplete
{
    /** Event type. */
    uint32_t    u32Type;
    /** Id of the matching start entry. */
    uint64_t    u64Id;
    /** Status code the request completed with */
    int32_t     i32Rc;
    /** Number of milliseconds the request needed to complete. */
    uint64_t    msDuration;
    /** Number of bytes of data following this entry. */
    uint64_t    u64IoBuffer;
} IoLogEntryComplete;
#pragma pack()

#pragma pack(1)
typedef struct IoLogEntryDiscard
{
    /** Start offset. */
    uint64_t    u64Off;
    /** Number of bytes to discard. */
    uint32_t    u32Discard;
} IoLogEntryDiscard;
#pragma pack()


/*********************************************************************************************************************************
*   Constants And Macros, Structures and Typedefs                                                                                *
*********************************************************************************************************************************/

/**
 * I/O logger instance data.
 */
typedef struct VDIOLOGGERINT
{
    /** File handle. */
    RTFILE         hFile;
    /** Current offset to append new entries to. */
    uint64_t       offWriteNext;
    /** Offset to read the next entry from. */
    uint64_t       offReadNext;
    /** Flags given during creation. */
    uint32_t       fFlags;
    /** Id for the next entry. */
    uint64_t       idNext;
    /** Memory cache for the I/O log entries. */
    RTMEMCACHE     hMemCacheIoLogEntries;
    /** Mutex section protecting the logger. */
    RTSEMFASTMUTEX hMtx;
    /** Cached event type of the next event. */
    uint32_t       u32EventTypeNext;
    /** Cached request type of the next request. */
    VDDBGIOLOGREQ  enmReqTypeNext;
} VDIOLOGGERINT;
/** Pointer to the internal I/O logger instance data. */
typedef VDIOLOGGERINT *PVDIOLOGGERINT;

/**
 * I/O log entry data.
 */
typedef struct VDIOLOGENTINT
{
    /** Id of the start entry. */
    uint64_t       idStart;
    /** Timestamnp when the request started. */
    uint64_t       tsStart;
    /** Size of the buffer to write on success. */
    size_t         cbIo;
} VDIOLOGENTINT;
/** Pointer to the internal I/O log entry data. */
typedef VDIOLOGENTINT *PVDIOLOGENTINT;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Creates a new empty I/O logger.
 *
 * @returns VBox status code.
 * @param   ppIoLogger    Where to store the new I/O logger handle.
 */
static int vddbgIoLoggerCreate(PVDIOLOGGERINT *ppIoLogger)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = NULL;

    pIoLogger = (PVDIOLOGGERINT)RTMemAllocZ(sizeof(VDIOLOGGERINT));
    if (pIoLogger)
    {
        rc = RTSemFastMutexCreate(&pIoLogger->hMtx);
        if (RT_SUCCESS(rc))
        {
            rc = RTMemCacheCreate(&pIoLogger->hMemCacheIoLogEntries, sizeof(VDIOLOGENTINT),
                                  0, UINT32_MAX, NULL, NULL, NULL, 0);
            if (RT_SUCCESS(rc))
            {
                *ppIoLogger = pIoLogger;
                return rc;
            }
        }
        RTMemFree(pIoLogger);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

/**
 * Update the header of the I/O logger to the current state.
 *
 * @returns VBox status code.
 * @param   pIoLogger    The I/O logger to update.
 */
static int vddbgIoLoggerHeaderUpdate(PVDIOLOGGERINT pIoLogger)
{
    int rc = VINF_SUCCESS;
    IoLogHeader Hdr;

    memcpy(Hdr.szMagic, VDIOLOG_MAGIC, sizeof(Hdr.szMagic));
    Hdr.fFlags = RT_H2LE_U32(pIoLogger->fFlags);
    Hdr.u64Id  = RT_H2LE_U64(pIoLogger->idNext);
    rc = RTFileWriteAt(pIoLogger->hFile, 0, &Hdr, sizeof(Hdr), NULL);

    return rc;
}

/**
 * Writes data from the given S/G buffer into the I/O log.
 *
 * @returns VBox status code.
 * @param   pIoLogger    The I/O logger to use.
 * @param   off          The start offset in the log to write to.
 * @param   pSgBuf       The S/G buffer to write.
 * @param   cbSgBuf      How much data to write.
 */
static int vddbgIoLogWriteSgBuf(PVDIOLOGGERINT pIoLogger, uint64_t off, PCRTSGBUF pSgBuf, size_t cbSgBuf)
{
    int rc = VINF_SUCCESS;
    RTSGBUF SgBuf;

    RTSgBufClone(&SgBuf, pSgBuf);

    while (cbSgBuf)
    {
        void *pvSeg;
        size_t cbSeg = cbSgBuf;

        pvSeg = RTSgBufGetNextSegment(&SgBuf, &cbSeg);
        AssertPtrBreakStmt(pvSeg, rc = VERR_INTERNAL_ERROR);

        rc = RTFileWriteAt(pIoLogger->hFile, off, pvSeg, cbSeg, NULL);
        if (RT_FAILURE(rc))
            break;

        cbSgBuf -= cbSeg;
        off += cbSeg;
    }

    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogCreate(PVDIOLOGGER phIoLogger, const char *pszFilename, uint32_t fFlags)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = NULL;

    AssertPtrReturn(phIoLogger, VERR_INVALID_POINTER);
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);
    AssertReturn(!(fFlags & ~VDDBG_IOLOG_VALID_MASK), VERR_INVALID_PARAMETER);

    rc = vddbgIoLoggerCreate(&pIoLogger);
    if (RT_SUCCESS(rc))
    {
        pIoLogger->fFlags = fFlags;
        pIoLogger->hFile = NIL_RTFILE;

        /* Create new log. */
        rc = RTFileOpen(&pIoLogger->hFile, pszFilename, RTFILE_O_DENY_NONE | RTFILE_O_CREATE | RTFILE_O_WRITE | RTFILE_O_READ);
        if (RT_SUCCESS(rc))
        {
            rc = vddbgIoLoggerHeaderUpdate(pIoLogger);
            if (RT_SUCCESS(rc))
            {
                pIoLogger->offWriteNext = sizeof(IoLogHeader);
                pIoLogger->offReadNext = sizeof(IoLogHeader);
            }
        }

        if (RT_SUCCESS(rc))
            *phIoLogger = pIoLogger;
        else
        {
            if (pIoLogger->hFile != NIL_RTFILE)
                RTFileClose(pIoLogger->hFile);
            RTMemFree(pIoLogger);
        }
    }

    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogOpen(PVDIOLOGGER phIoLogger, const char *pszFilename)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = NULL;

    AssertPtrReturn(phIoLogger, VERR_INVALID_POINTER);
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);

    rc = vddbgIoLoggerCreate(&pIoLogger);
    if (RT_SUCCESS(rc))
    {
        /* open existing log. */
        rc = RTFileOpen(&pIoLogger->hFile, pszFilename, RTFILE_O_DENY_NONE | RTFILE_O_OPEN | RTFILE_O_WRITE | RTFILE_O_READ);
        if (RT_SUCCESS(rc))
        {
            IoLogHeader Hdr;
            uint64_t cbLog;

            rc = RTFileGetSize(pIoLogger->hFile, &cbLog);

            /* Read the header. */
            if (RT_SUCCESS(rc))
                rc = RTFileRead(pIoLogger->hFile, &Hdr, sizeof(Hdr), NULL);

            if (   RT_SUCCESS(rc)
                && !memcmp(Hdr.szMagic, VDIOLOG_MAGIC, sizeof(Hdr.szMagic)))
            {
                pIoLogger->fFlags = RT_LE2H_U32(Hdr.fFlags);
                pIoLogger->offWriteNext = cbLog;
                pIoLogger->offReadNext  = sizeof(Hdr);
                pIoLogger->idNext       = RT_LE2H_U64(Hdr.u64Id);
                *phIoLogger = pIoLogger;
            }
            else if (RT_SUCCESS(rc))
                rc = VERR_INVALID_PARAMETER;
        }
    }

    return rc;
}

VBOXDDU_DECL(void) VDDbgIoLogDestroy(VDIOLOGGER hIoLogger)
{
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturnVoid(pIoLogger);

    vddbgIoLoggerHeaderUpdate(pIoLogger);
    RTFileFlush(pIoLogger->hFile);
    RTFileClose(pIoLogger->hFile);
    RTMemCacheDestroy(pIoLogger->hMemCacheIoLogEntries);
    RTSemFastMutexDestroy(pIoLogger->hMtx);
    RTMemFree(pIoLogger);
}

VBOXDDU_DECL(int) VDDbgIoLogCommit(VDIOLOGGER hIoLogger)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);

    rc = vddbgIoLoggerHeaderUpdate(pIoLogger);
    if (RT_SUCCESS(rc))
        rc = RTFileFlush(pIoLogger->hFile);

    return rc;
}

VBOXDDU_DECL(uint32_t) VDDbgIoLogGetFlags(VDIOLOGGER hIoLogger)
{
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, 0);

    return pIoLogger->fFlags;
}

VBOXDDU_DECL(int) VDDbgIoLogStart(VDIOLOGGER hIoLogger, bool fAsync, VDDBGIOLOGREQ enmTxDir, uint64_t off, size_t cbIo, PCRTSGBUF pSgBuf,
                                  PVDIOLOGENT phIoLogEntry)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;
    PVDIOLOGENTINT pIoLogEntry = NULL;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(phIoLogEntry, VERR_INVALID_POINTER);
    AssertReturn(enmTxDir > VDDBGIOLOGREQ_INVALID && enmTxDir <= VDDBGIOLOGREQ_FLUSH, VERR_INVALID_PARAMETER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    pIoLogEntry = (PVDIOLOGENTINT)RTMemCacheAlloc(pIoLogger->hMemCacheIoLogEntries);
    if (pIoLogEntry)
    {
        IoLogEntryStart Entry;

        pIoLogEntry->idStart = pIoLogger->idNext++;

        Entry.u32Type       = VDIOLOG_EVENT_START;
        Entry.u8AsyncIo    = fAsync ? 1 : 0;
        Entry.u32ReqType   = enmTxDir;
        Entry.u64Id        = RT_H2LE_U64(pIoLogEntry->idStart);
        Entry.Io.u64Off    = RT_H2LE_U64(off);
        Entry.Io.u64IoSize = RT_H2LE_U64(cbIo);

        /* Write new entry. */
        rc = RTFileWriteAt(pIoLogger->hFile, pIoLogger->offWriteNext, &Entry, sizeof(Entry), NULL);
        if (RT_SUCCESS(rc))
        {
            pIoLogger->offWriteNext += sizeof(Entry);

            if (   enmTxDir == VDDBGIOLOGREQ_WRITE
                && (pIoLogger->fFlags & VDDBG_IOLOG_LOG_DATA_WRITTEN))
            {
                /* Write data. */
                rc = vddbgIoLogWriteSgBuf(pIoLogger, pIoLogger->offWriteNext, pSgBuf, cbIo);
                if (RT_FAILURE(rc))
                {
                    pIoLogger->offWriteNext -= sizeof(Entry);
                    rc = RTFileSetSize(pIoLogger->hFile, pIoLogger->offWriteNext);
                }
                else
                    pIoLogger->offWriteNext += cbIo;
            }
        }

        if (RT_SUCCESS(rc))
        {
            pIoLogEntry->tsStart = RTTimeProgramMilliTS();

            if (   enmTxDir == VDDBGIOLOGREQ_READ
                && (pIoLogger->fFlags & VDDBG_IOLOG_LOG_DATA_READ))
                pIoLogEntry->cbIo = cbIo;
            else
                pIoLogEntry->cbIo = 0;

            *phIoLogEntry = pIoLogEntry;
        }
        else
        {
            pIoLogger->idNext--;
            RTMemCacheFree(pIoLogger->hMemCacheIoLogEntries, pIoLogEntry);
        }
    }
    else
        rc = VERR_NO_MEMORY;

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogStartDiscard(VDIOLOGGER hIoLogger, bool fAsync, PCRTRANGE paRanges, unsigned cRanges,
                                         PVDIOLOGENT phIoLogEntry)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;
    PVDIOLOGENTINT pIoLogEntry = NULL;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(phIoLogEntry, VERR_INVALID_POINTER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    pIoLogEntry = (PVDIOLOGENTINT)RTMemCacheAlloc(pIoLogger->hMemCacheIoLogEntries);
    if (pIoLogEntry)
    {
        IoLogEntryStart Entry;

        pIoLogEntry->idStart = pIoLogger->idNext++;

        Entry.u32Type         = VDIOLOG_EVENT_START;
        Entry.u8AsyncIo       = fAsync ? 1 : 0;
        Entry.u32ReqType      = VDDBGIOLOGREQ_DISCARD;
        Entry.u64Id           = RT_H2LE_U64(pIoLogEntry->idStart);
        Entry.Discard.cRanges = RT_H2LE_U32(cRanges);

        /* Write new entry. */
        rc = RTFileWriteAt(pIoLogger->hFile, pIoLogger->offWriteNext, &Entry, sizeof(Entry), NULL);
        if (RT_SUCCESS(rc))
        {
            pIoLogger->offWriteNext += sizeof(Entry);

            IoLogEntryDiscard DiscardRange;

            for (unsigned i = 0; i < cRanges; i++)
            {
                DiscardRange.u64Off = RT_H2LE_U64(paRanges[i].offStart);
                DiscardRange.u32Discard = RT_H2LE_U32((uint32_t)paRanges[i].cbRange);
                rc = RTFileWriteAt(pIoLogger->hFile, pIoLogger->offWriteNext + i*sizeof(DiscardRange),
                                   &DiscardRange, sizeof(DiscardRange), NULL);
                if (RT_FAILURE(rc))
                    break;
            }

            if (RT_FAILURE(rc))
            {
                pIoLogger->offWriteNext -= sizeof(Entry);
                rc = RTFileSetSize(pIoLogger->hFile, pIoLogger->offWriteNext);
            }
            else
                pIoLogger->offWriteNext += cRanges * sizeof(DiscardRange);
        }

        if (RT_SUCCESS(rc))
        {
            pIoLogEntry->tsStart = RTTimeProgramMilliTS();
            pIoLogEntry->cbIo = 0;

            *phIoLogEntry = pIoLogEntry;
        }
        else
        {
            pIoLogger->idNext--;
            RTMemCacheFree(pIoLogger->hMemCacheIoLogEntries, pIoLogEntry);
        }
    }
    else
        rc = VERR_NO_MEMORY;

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogComplete(VDIOLOGGER hIoLogger, VDIOLOGENT hIoLogEntry, int rcReq, PCRTSGBUF pSgBuf)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;
    PVDIOLOGENTINT pIoLogEntry = hIoLogEntry;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(pIoLogEntry, VERR_INVALID_HANDLE);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    IoLogEntryComplete Entry;

    Entry.u32Type     = VDIOLOG_EVENT_COMPLETE;
    Entry.u64Id       = RT_H2LE_U64(pIoLogEntry->idStart);
    Entry.msDuration  = RTTimeProgramMilliTS() - RT_H2LE_U64(pIoLogEntry->tsStart);
    Entry.i32Rc       = (int32_t)RT_H2LE_U32((uint32_t)rcReq);
    Entry.u64IoBuffer = RT_H2LE_U64(pIoLogEntry->cbIo);

    /* Write new entry. */
    rc = RTFileWriteAt(pIoLogger->hFile, pIoLogger->offWriteNext, &Entry, sizeof(Entry), NULL);
    if (RT_SUCCESS(rc))
    {
        pIoLogger->offWriteNext += sizeof(Entry);

        if (pIoLogEntry->cbIo)
        {
            rc = vddbgIoLogWriteSgBuf(pIoLogger, pIoLogger->offWriteNext, pSgBuf, pIoLogEntry->cbIo);
            if (RT_SUCCESS(rc))
                pIoLogger->offWriteNext += pIoLogEntry->cbIo;
            else
            {
                pIoLogger->offWriteNext -= sizeof(Entry);
                rc = RTFileSetSize(pIoLogger->hFile, pIoLogger->offWriteNext);
            }
        }
    }

    RTMemCacheFree(pIoLogger->hMemCacheIoLogEntries, pIoLogEntry);
    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogEventTypeGetNext(VDIOLOGGER hIoLogger, VDIOLOGEVENT *penmEvent)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(penmEvent, VERR_INVALID_POINTER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    if (pIoLogger->offReadNext == pIoLogger->offWriteNext)
    {
        *penmEvent = VDIOLOGEVENT_END;
        RTSemFastMutexRelease(pIoLogger->hMtx);
        return VINF_SUCCESS;
    }

    if (!pIoLogger->u32EventTypeNext)
    {
        uint32_t abBuf[2];
        rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext, &abBuf, sizeof(abBuf), NULL);
        if (RT_SUCCESS(rc))
        {
            pIoLogger->u32EventTypeNext = abBuf[0];
            pIoLogger->enmReqTypeNext   = (VDDBGIOLOGREQ)abBuf[1];
        }
    }

    if (RT_SUCCESS(rc))
    {
        Assert(pIoLogger->u32EventTypeNext != VDIOLOGEVENT_INVALID);

        switch (pIoLogger->u32EventTypeNext)
        {
            case VDIOLOG_EVENT_START:
                *penmEvent = VDIOLOGEVENT_START;
                break;
            case VDIOLOG_EVENT_COMPLETE:
                *penmEvent = VDIOLOGEVENT_COMPLETE;
                break;
            default:
                AssertMsgFailed(("Invalid event type %d\n", pIoLogger->u32EventTypeNext));
        }
    }

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogReqTypeGetNext(VDIOLOGGER hIoLogger, PVDDBGIOLOGREQ penmReq)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(penmReq, VERR_INVALID_POINTER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    if (pIoLogger->offReadNext == pIoLogger->offWriteNext)
    {
        *penmReq = VDDBGIOLOGREQ_INVALID;
        RTSemFastMutexRelease(pIoLogger->hMtx);
        return VERR_INVALID_STATE;
    }

    if (RT_SUCCESS(rc))
    {
        Assert(pIoLogger->enmReqTypeNext != VDDBGIOLOGREQ_INVALID);
        *penmReq = pIoLogger->enmReqTypeNext;
    }

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogEventGetStart(VDIOLOGGER hIoLogger, uint64_t *pidEvent, bool *pfAsync,
                                          uint64_t *poff, size_t *pcbIo, size_t cbBuf, void *pvBuf)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(pidEvent, VERR_INVALID_POINTER);
    AssertPtrReturn(pfAsync, VERR_INVALID_POINTER);
    AssertPtrReturn(poff, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbIo, VERR_INVALID_POINTER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    if (pIoLogger->u32EventTypeNext == VDIOLOG_EVENT_START)
    {
        IoLogEntryStart Entry;
        rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext, &Entry, sizeof(Entry), NULL);
        if (RT_SUCCESS(rc))
        {
            *pfAsync   = RT_BOOL(Entry.u8AsyncIo);
            *pidEvent  = RT_LE2H_U64(Entry.u64Id);
            *poff      = RT_LE2H_U64(Entry.Io.u64Off);
            *pcbIo     = RT_LE2H_U64(Entry.Io.u64IoSize);

            if (   pIoLogger->enmReqTypeNext == VDDBGIOLOGREQ_WRITE
                && (pIoLogger->fFlags & VDDBG_IOLOG_LOG_DATA_WRITTEN))
            {
                /* Read data. */
                if (cbBuf < *pcbIo)
                    rc = VERR_BUFFER_OVERFLOW;
                else
                    rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext + sizeof(Entry), pvBuf, *pcbIo, NULL);

                if (rc != VERR_BUFFER_OVERFLOW)
                    pIoLogger->offReadNext += *pcbIo + sizeof(Entry);
            }
            else
                pIoLogger->offReadNext += sizeof(Entry);
        }
    }
    else
        rc = VERR_INVALID_STATE;

    if (RT_SUCCESS(rc))
        pIoLogger->u32EventTypeNext = 0;

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

VBOXDDU_DECL(int) VDDbgIoLogEventGetStartDiscard(VDIOLOGGER hIoLogger, uint64_t *pidEvent, bool *pfAsync,
                                                 PRTRANGE *ppaRanges, unsigned *pcRanges)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(pidEvent, VERR_INVALID_POINTER);
    AssertPtrReturn(pfAsync, VERR_INVALID_POINTER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    if (   pIoLogger->u32EventTypeNext == VDIOLOG_EVENT_START
        && pIoLogger->enmReqTypeNext == VDDBGIOLOGREQ_DISCARD)
    {
        IoLogEntryStart Entry;
        rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext, &Entry, sizeof(Entry), NULL);
        if (RT_SUCCESS(rc))
        {
            PRTRANGE paRanges = NULL;
            IoLogEntryDiscard DiscardRange;

            pIoLogger->offReadNext += sizeof(Entry);
            *pfAsync   = RT_BOOL(Entry.u8AsyncIo);
            *pidEvent  = RT_LE2H_U64(Entry.u64Id);
            *pcRanges  = RT_LE2H_U32(Entry.Discard.cRanges);

            paRanges = (PRTRANGE)RTMemAllocZ(*pcRanges * sizeof(RTRANGE));
            if (paRanges)
            {
                for (unsigned i = 0; i < *pcRanges; i++)
                {
                    rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext + i*sizeof(DiscardRange),
                                      &DiscardRange, sizeof(DiscardRange), NULL);
                    if (RT_FAILURE(rc))
                        break;

                    paRanges[i].offStart = RT_LE2H_U64(DiscardRange.u64Off);
                    paRanges[i].cbRange  = RT_LE2H_U32(DiscardRange.u32Discard);
                }

                if (RT_SUCCESS(rc))
                {
                    pIoLogger->offReadNext += *pcRanges * sizeof(DiscardRange);
                    *ppaRanges = paRanges;
                }
                else
                {
                    pIoLogger->offReadNext -= sizeof(Entry);
                    RTMemFree(paRanges);
                }
            }
            else
                rc = VERR_NO_MEMORY;
        }
    }
    else
        rc = VERR_INVALID_STATE;

    if (RT_SUCCESS(rc))
        pIoLogger->u32EventTypeNext = 0;

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;

}

VBOXDDU_DECL(int) VDDbgIoLogEventGetComplete(VDIOLOGGER hIoLogger, uint64_t *pidEvent, int *pRc,
                                             uint64_t *pmsDuration, size_t *pcbIo, size_t cbBuf, void *pvBuf)
{
    int rc = VINF_SUCCESS;
    PVDIOLOGGERINT pIoLogger = hIoLogger;

    AssertPtrReturn(pIoLogger, VERR_INVALID_HANDLE);
    AssertPtrReturn(pidEvent, VERR_INVALID_POINTER);
    AssertPtrReturn(pmsDuration, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbIo, VERR_INVALID_POINTER);

    rc = RTSemFastMutexRequest(pIoLogger->hMtx);
    AssertRCReturn(rc, rc);

    if (pIoLogger->u32EventTypeNext == VDIOLOG_EVENT_COMPLETE)
    {
        IoLogEntryComplete Entry;
        rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext, &Entry, sizeof(Entry), NULL);
        if (RT_SUCCESS(rc))
        {
            *pidEvent    = RT_LE2H_U64(Entry.u64Id);
            *pRc         = (int)RT_LE2H_U32((int32_t)Entry.i32Rc);
            *pmsDuration = RT_LE2H_U64(Entry.msDuration);
            *pcbIo       = RT_LE2H_U64(Entry.u64IoBuffer);

            if (*pcbIo)
            {
                /* Read data. */
                if (cbBuf < *pcbIo)
                    rc = VERR_BUFFER_OVERFLOW;
                else
                    rc = RTFileReadAt(pIoLogger->hFile, pIoLogger->offReadNext + sizeof(Entry), pvBuf, *pcbIo, NULL);

                if (rc != VERR_BUFFER_OVERFLOW)
                    pIoLogger->offReadNext += *pcbIo + sizeof(Entry);
            }
            else
                pIoLogger->offReadNext += sizeof(Entry);
        }
    }
    else
        rc = VERR_INVALID_STATE;

    if (RT_SUCCESS(rc))
        pIoLogger->u32EventTypeNext = 0;

    RTSemFastMutexRelease(pIoLogger->hMtx);
    return rc;
}

