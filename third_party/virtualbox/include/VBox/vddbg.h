/** @file
 * VD Debug API.
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

#ifndef ___VBox_vddbg_h
#define ___VBox_vddbg_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/vd.h> /* for VDRANGE */
#include <iprt/sg.h>

RT_C_DECLS_BEGIN

#ifdef IN_RING0
# error "There are no VD Debug APIs available in Ring-0 Host Context!"
#endif

/** @defgroup grp_vddbg            VD Debug API
 * @ingroup grp_vd
 * @{
 */

/** I/O logger handle. */
typedef struct VDIOLOGGERINT *VDIOLOGGER;
/** Pointer to an I/O logger handler. */
typedef VDIOLOGGER *PVDIOLOGGER;

/** Pointer to an I/O log entry handle. */
typedef struct VDIOLOGENTINT *VDIOLOGENT;
/** Pointer to an I/O log entry handle. */
typedef VDIOLOGENT *PVDIOLOGENT;

/** I/O logger buffers all log entries in memory until VDDbgIoLogCommit() is called.
 * If not given all entries are immediately logged to the file. */
#define VDDBG_IOLOG_MEMORY_BUFFER    RT_BIT_32(0)
/** I/O logger logs the written data. */
#define VDDBG_IOLOG_LOG_DATA_WRITTEN RT_BIT_32(1)
/** I/O logger logs the read data. */
#define VDDBG_IOLOG_LOG_DATA_READ    RT_BIT_32(2)
/** I/O logger logs all data. */
#define VDDBG_IOLOG_LOG_DATA (VDDBG_IOLOG_LOG_DATA_READ | VDDBG_IOLOG_LOG_DATA_WRITTEN)
/** Mask of valid flags. */
#define VDDBG_IOLOG_VALID_MASK (VDDBG_IOLOG_MEMORY_BUFFER | VDDBG_IOLOG_LOG_DATA)

/**
 * I/O direction.
 */
typedef enum VDDBGIOLOGREQ
{
    /** Invalid direction. */
    VDDBGIOLOGREQ_INVALID = 0,
    /** Read. */
    VDDBGIOLOGREQ_READ,
    /** Write. */
    VDDBGIOLOGREQ_WRITE,
    /** Flush. */
    VDDBGIOLOGREQ_FLUSH,
    /** Discard. */
    VDDBGIOLOGREQ_DISCARD,
    /** 32bit hack. */
    VDDBGIOLOGREQ_32BIT_HACK = 0x7fffffff
} VDDBGIOLOGREQ;
/** Pointer to a I/O direction. */
typedef VDDBGIOLOGREQ *PVDDBGIOLOGREQ;

/**
 * I/O log event types.
 */
typedef enum VDIOLOGEVENT
{
    /** Invalid event. */
    VDIOLOGEVENT_INVALID = 0,
    /** I/O request start event. */
    VDIOLOGEVENT_START,
    /** I/O request complete event. */
    VDIOLOGEVENT_COMPLETE,
    /** No more events logged. */
    VDIOLOGEVENT_END,
    /** 32bit type blowup. */
    VDIOLOGEVENT_32BIT_HACK = 0x7fffffff
} VDIOLOGEVENT;
/** Pointer to an I/O log event. */
typedef VDIOLOGEVENT *PVDIOLOGEVENT;

/**
 * Creates a new I/O logger for writing to the I/O log.
 *
 * @returns VBox status code.
 * @param   phIoLogger    Where to store the I/O logger handle on success.
 * @param   pszFilename   The file to log into.
 * @param   fFlags        Flags for the I/O logger.
 */
VBOXDDU_DECL(int) VDDbgIoLogCreate(PVDIOLOGGER phIoLogger, const char *pszFilename, uint32_t fFlags);

/**
 * Opens an existing I/O log and creates a new I/O logger from it.
 *
 * @returns VBox status code.
 * @param   phIoLogger    Where to store the I/O logger handle on success.
 * @param   pszFilename   The I/O log to use.
 */
VBOXDDU_DECL(int) VDDbgIoLogOpen(PVDIOLOGGER phIoLogger, const char *pszFilename);

/**
 * Destroys the given I/O logger handle.
 *
 * @returns nothing.
 * @param   hIoLogger    The I/O logger handle to destroy.
 */
VBOXDDU_DECL(void) VDDbgIoLogDestroy(VDIOLOGGER hIoLogger);

/**
 * Commit all log entries to the log file.
 *
 * @returns VBox status code.
 * @param   hIoLogger    The I/O logger to flush.
 */
VBOXDDU_DECL(int) VDDbgIoLogCommit(VDIOLOGGER hIoLogger);

/**
 * Returns the flags of the given I/O logger.
 *
 * @returns Flags of the I/O logger.
 * @param   hIoLogger    The I/O logger to use.
 */
VBOXDDU_DECL(uint32_t) VDDbgIoLogGetFlags(VDIOLOGGER hIoLogger);

/**
 * Starts logging of an I/O request.
 *
 * @returns VBox status code.
 * @param   hIoLogger    The I/O logger to use.
 * @param   fAsync       Flag whether the request is synchronous or asynchronous.
 * @param   enmTxDir     The transfer direction to log.
 * @param   off          The start offset of the I/O request to log.
 * @param   cbIo         The number of bytes the I/O request transfered.
 * @param   pSgBuf       The data the I/O request is writing if it is a write request.
 *                       Can be NULL if the logger is instructed to not log the data
 *                       or a flush request is logged.
 * @param   phIoLogEntry Where to store the I/O log entry handle on success.
 */
VBOXDDU_DECL(int) VDDbgIoLogStart(VDIOLOGGER hIoLogger, bool fAsync, VDDBGIOLOGREQ enmTxDir, uint64_t off, size_t cbIo, PCRTSGBUF pSgBuf,
                                  PVDIOLOGENT phIoLogEntry);

/**
 * Starts logging of a discard request.
 *
 * @returns VBox status code.
 * @param   hIoLogger    The I/O logger to use.
 * @param   fAsync       Flag whether the request is synchronous or asynchronous.
 * @param   paRanges     The array of ranges to discard.
 * @param   cRanges      Number of rnages in the array.
 * @param   phIoLogEntry Where to store the I/O log entry handle on success.
 */
VBOXDDU_DECL(int) VDDbgIoLogStartDiscard(VDIOLOGGER hIoLogger, bool fAsync, PCRTRANGE paRanges, unsigned cRanges,
                                         PVDIOLOGENT phIoLogEntry);

/**
 * Marks the given I/O log entry as completed.
 *
 * @returns VBox status code.
 * @param   hIoLogger    The I/O logger to use.
 * @param   hIoLogEntry  The I/O log entry to complete.
 * @param   rcReq        The status code the request completed with.
 * @param   pSgBuf       The data read if the request was a read and it succeeded.
 */
VBOXDDU_DECL(int) VDDbgIoLogComplete(VDIOLOGGER hIoLogger, VDIOLOGENT hIoLogEntry, int rcReq, PCRTSGBUF pSgBuf);

/**
 * Gets the next event type from the I/O log.
 *
 * @returns VBox status code.
 * @param   hIoLogger    The I/O logger to use.
 * @param   penmEvent    Where to store the next event on success.
 */
VBOXDDU_DECL(int) VDDbgIoLogEventTypeGetNext(VDIOLOGGER hIoLogger, VDIOLOGEVENT *penmEvent);

/**
 * Gets the next request type from the I/O log.
 *
 * @returns VBox status code.
 * @param   hIoLogger    The I/O logger to use.
 * @param   penmReq      Where to store the next request on success.
 */
VBOXDDU_DECL(int) VDDbgIoLogReqTypeGetNext(VDIOLOGGER hIoLogger, PVDDBGIOLOGREQ penmReq);

/**
 * Returns the start event from the I/O log.
 *
 * @returns VBox status code.
 * @retval  VERR_EOF if the end of the log is reached.
 * @retval  VERR_BUFFER_OVERFLOW if the provided data buffer can't hold the data.
 *                               pcbIo will hold the required buffer size on return.
 * @param   hIoLogger    The I/O logger to use.
 * @param   pidEvent     The ID of the event to identify the corresponding complete event.
 * @param   pfAsync      Where to store the flag whether the request is
 * @param   poff         Where to store the offset of the next I/O log entry on success.
 * @param   pcbIo        Where to store the transfer size of the next I/O log entry on success.
 * @param   cbBuf        Size of the provided data buffer.
 * @param   pvBuf        Where to store the data of the next I/O log entry on success.
 */
VBOXDDU_DECL(int) VDDbgIoLogEventGetStart(VDIOLOGGER hIoLogger, uint64_t *pidEvent, bool *pfAsync,
                                          uint64_t *poff, size_t *pcbIo, size_t cbBuf, void *pvBuf);

/**
 * Returns the discard start event from the I/O log.
 *
 * @returns VBox status code.
 * @retval  VERR_EOF if the end of the log is reached.
 * @retval  VERR_BUFFER_OVERFLOW if the provided data buffer can't hold the data.
 *                               pcbIo will hold the required buffer size on return.
 * @param   hIoLogger    The I/O logger to use.
 * @param   pidEvent     The ID of the event to identify the corresponding complete event.
 * @param   pfAsync      Where to store the flag whether the request is
 * @param   ppaRanges    Where to store the pointer to the range array on success.
 * @param   pcRanges     Where to store the number of entries in the array on success.
 */
VBOXDDU_DECL(int) VDDbgIoLogEventGetStartDiscard(VDIOLOGGER hIoLogger, uint64_t *pidEvent, bool *pfAsync,
                                                 PRTRANGE *ppaRanges, unsigned *pcRanges);

/**
 * Returns the complete from the I/O log.
 *
 * @returns VBox status code.
 * @retval  VERR_EOF if the end of the log is reached
 * @retval  VERR_BUFFER_OVERFLOW if the provided data buffer can't hold the data.
 *                               pcbIo will hold the required buffer size on return.
 * @param   hIoLogger    The I/O logger to use.
 * @param   pidEvent     The ID of the event to identify the corresponding start event.
 * @param   pRc          Where to store the status code of the request on success.
 * @param   pmsDuration  Where to store the duration of the request.
 * @param   pcbIo        Where to store the transfer size of the next I/O log entry on success.
 * @param   cbBuf        Size of the provided data buffer.
 * @param   pvBuf        Where to store the data of the data transfered during a read request.
 */
VBOXDDU_DECL(int) VDDbgIoLogEventGetComplete(VDIOLOGGER hIoLogger, uint64_t *pidEvent, int *pRc,
                                             uint64_t *pmsDuration, size_t *pcbIo, size_t cbBuf, void *pvBuf);

/** @} */

RT_C_DECLS_END

#endif
