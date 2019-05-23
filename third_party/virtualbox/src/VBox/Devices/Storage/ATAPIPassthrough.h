/* $Id: ATAPIPassthrough.h $ */
/** @file
 * VBox storage devices: ATAPI passthrough helpers (common code for DevATA and DevAHCI).
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef __ATAPIPassthrough_h
#define __ATAPIPassthrough_h

#include <VBox/cdefs.h>
#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/pdmstorageifs.h>

RT_C_DECLS_BEGIN

/**
 * Opaque media track list.
 */
typedef struct TRACKLIST *PTRACKLIST;

/**
 * Creates an empty track list handle.
 *
 * @returns VBox status code.
 * @param   ppTrackList    Where to store the track list handle on success.
 */
DECLHIDDEN(int) ATAPIPassthroughTrackListCreateEmpty(PTRACKLIST *ppTrackList);

/**
 * Destroys the allocated task list handle.
 *
 * @returns nothing.
 * @param   pTrackList    The track list handle to destroy.
 */
DECLHIDDEN(void) ATAPIPassthroughTrackListDestroy(PTRACKLIST pTrackList);

/**
 * Clears all tracks from the given task list.
 *
 * @returns nothing.
 * @param   pTrackList    The track list to clear.
 */
DECLHIDDEN(void) ATAPIPassthroughTrackListClear(PTRACKLIST pTrackList);

/**
 * Updates the track list from the given CDB and data buffer.
 *
 * @returns VBox status code.
 * @param   pTrackList    The track list to update.
 * @param   pCDB          The CDB buffer.
 * @param   pvBuf         The data buffer.
 */
DECLHIDDEN(int) ATAPIPassthroughTrackListUpdate(PTRACKLIST pTrackList, const uint8_t *pCDB, const void *pvBuf);

/**
 * Return the sector size from the track matching the LBA in the given track list.
 *
 * @returns Sector size.
 * @param   pTrackList    The track list to use.
 * @param   iAtapiLba     The start LBA to get the sector size for.
 */
DECLHIDDEN(uint32_t) ATAPIPassthroughTrackListGetSectorSizeFromLba(PTRACKLIST pTrackList, uint32_t iAtapiLba);

/**
 * Parses the given CDB and returns whether it is safe to pass it through to the host drive.
 *
 * @returns Flag whether passing the CDB through to the host drive is safe.
 * @param   pbCdb         The CDB to parse.
 * @param   cbCdb         Size of the CDB in bytes.
 * @param   cbBuf         Size of the guest buffer.
 * @param   pTrackList    The track list for the current medium if available (optional).
 * @param   pbSense       Pointer to the sense buffer.
 * @param   cbSense       Size of the sense buffer.
 * @param   penmTxDir     Where to store the transfer direction (guest to host or vice versa).
 * @param   pcbXfer       Where to store the transfer size encoded in the CDB.
 * @param   pcbSector     Where to store the sector size used for the transfer.
 * @param   pu8ScsiSts    Where to store the SCSI status code.
 */
DECLHIDDEN(bool) ATAPIPassthroughParseCdb(const uint8_t *pbCdb, size_t cbCdb, size_t cbBuf,
                                          PTRACKLIST pTrackList, uint8_t *pbSense, size_t cbSense,
                                          PDMMEDIATXDIR *penmTxDir, size_t *pcbXfer,
                                          size_t *pcbSector, uint8_t *pu8ScsiSts);

RT_C_DECLS_END

#endif /* __ATAPIPassthrough_h */
