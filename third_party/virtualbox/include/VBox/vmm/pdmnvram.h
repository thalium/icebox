/** @file
 * PDM - Pluggable Device Manager, EFI NVRAM storage back-end.
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

#ifndef ___VBox_vmm_pdmnvram_h_
#define ___VBox_vmm_pdmnvram_h_

#include <VBox/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_ifs_nvram       NVRAM Interface
 * @ingroup grp_pdm_interfaces
 * @{
 */

/** Pointer to NVRAM interface provided by the driver. */
typedef struct PDMINVRAMCONNECTOR *PPDMINVRAMCONNECTOR;

/**
 * Non-volatile RAM storage interface provided by the driver (up).
 *
 * @note    The variable indexes used here 0-based, sequential and without gaps.
 */
typedef struct PDMINVRAMCONNECTOR
{
    /**
     * Query a variable by variable index.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_FOUND if the variable was not found. This indicates that
     *          there are not variables with a higher index.
     *
     * @param   pInterface      Pointer to this interface structure.
     * @param   idxVariable     The variable index. By starting @a idxVariable at 0
     *                          and increasing it with each call, this can be used
     *                          to enumerate all available variables.
     * @param   pVendorUuid     The vendor UUID of the variable.
     * @param   pszName         The variable name buffer.
     * @param   pcchName        On input this hold the name buffer size (including
     *                          the space for the terminator char).  On successful
     *                          return it holds the strlen() value for @a pszName.
     * @param   pfAttributes    Where to return the value attributes.
     * @param   pbValue         The value buffer.
     * @param   pcbValue        On input the size of the value buffer, on output the
     *                          actual number of bytes returned.
     */
    DECLR3CALLBACKMEMBER(int, pfnVarQueryByIndex,(PPDMINVRAMCONNECTOR pInterface, uint32_t idxVariable,
                                                  PRTUUID pVendorUuid, char *pszName, uint32_t *pcchName,
                                                  uint32_t *pfAttributes, uint8_t *pbValue, uint32_t *pcbValue));

    /**
     * Begins variable store sequence.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to this interface structure.
     * @param   cVariables      The number of variables.
     */
    DECLR3CALLBACKMEMBER(int, pfnVarStoreSeqBegin,(PPDMINVRAMCONNECTOR pInterface, uint32_t cVariables));

    /**
     * Puts the next variable in the store sequence.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to this interface structure.
     * @param   idxVariable     The variable index. This will start at 0 and advance
     *                          up to @a cVariables - 1.
     * @param   pVendorUuid     The vendor UUID of the variable.
     * @param   pszName         The variable name buffer.
     * @param   cchName         On input this hold the name buffer size (including
     *                          the space for the terminator char).  On successful
     *                          return it holds the strlen() value for @a pszName.
     * @param   fAttributes     The value attributes.
     * @param   pbValue         The value buffer.
     * @param   cbValue         On input the size of the value buffer, on output the
     *                          actual number of bytes returned.
     */
    DECLR3CALLBACKMEMBER(int, pfnVarStoreSeqPut,(PPDMINVRAMCONNECTOR pInterface, int idxVariable,
                                                 PCRTUUID pVendorUuid, const char *pszName, size_t cchName,
                                                 uint32_t fAttributes, uint8_t const *pbValue, size_t cbValue));

    /**
     * Ends a variable store sequence.
     *
     * @returns VBox status code, @a rc on success.
     * @param   pInterface      Pointer to this interface structure.
     * @param   rc              The VBox status code for the whole store operation.
     */
    DECLR3CALLBACKMEMBER(int, pfnVarStoreSeqEnd,(PPDMINVRAMCONNECTOR pInterface, int rc));

} PDMINVRAMCONNECTOR;
/** PDMINVRAMCONNECTOR interface ID. */
#define PDMINVRAMCONNECTOR_IID                 "057bc5c9-8022-43a8-9a41-0b106f97a89f"

/** @} */

RT_C_DECLS_END

#endif

