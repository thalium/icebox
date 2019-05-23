/* $Id: GuestControl.h $ */
/** @file
 * Guest Control - Common Guest and Host Code.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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

#ifndef ___VBox_GuestHost_GuestControl_h
#define ___VBox_GuestHost_GuestControl_h

#include <iprt/types.h>

/* Everything defined in this file lives in this namespace. */
namespace guestControl {

/**
 * Process status when executed in the guest.
 */
enum eProcessStatus
{
    /** Process is in an undefined state. */
    PROC_STS_UNDEFINED = 0,
    /** Process has been started. */
    PROC_STS_STARTED = 1,
    /** Process terminated normally. */
    PROC_STS_TEN = 2,
    /** Process terminated via signal. */
    PROC_STS_TES = 3,
    /** Process terminated abnormally. */
    PROC_STS_TEA = 4,
    /** Process timed out and was killed. */
    PROC_STS_TOK = 5,
    /** Process timed out and was not killed successfully. */
    PROC_STS_TOA = 6,
    /** Service/OS is stopping, process was killed. */
    PROC_STS_DWN = 7,
    /** Something went wrong (error code in flags). */
    PROC_STS_ERROR = 8
};

/** @todo r=bird: Most defines in this file needs to be scoped a little
 *        better!  For instance INPUT_FLAG_NONE is very generic. */

/**
 * Input flags, set by the host. This is needed for
 * handling flags on the guest side.
 * Note: Has to match Main's ProcessInputFlag_* flags!
 */
#define INPUT_FLAG_NONE                     0x0
#define INPUT_FLAG_EOF                      RT_BIT(0)

/**
 * Guest session creation flags.
 * Only handled internally at the moment.
 */
#define SESSIONCREATIONFLAG_NONE            0x0

/** @name DIRREMOVE_FLAG_XXX - Guest directory removement flags.
 * Essentially using what IPRT's RTDIRRMREC_F_
 * defines have to offer.
 * @{
 */
#define DIRREMOVE_FLAG_RECURSIVE            RT_BIT(0)
/** Delete the content of the directory and the directory itself. */
#define DIRREMOVE_FLAG_CONTENT_AND_DIR      RT_BIT(1)
/** Only delete the content of the directory, omit the directory it self. */
#define DIRREMOVE_FLAG_CONTENT_ONLY         RT_BIT(2)
/** Mask of valid flags. */
#define DIRREMOVE_FLAG_VALID_MASK           UINT32_C(0x00000003)
/** @}   */

/** @name EXECUTEPROCESSFLAG_XXX - Guest process creation flags.
 * @note Has to match Main's ProcessCreateFlag_* flags!
 * @{
 */
#define EXECUTEPROCESSFLAG_NONE             UINT32_C(0x0)
#define EXECUTEPROCESSFLAG_WAIT_START       RT_BIT(0)
#define EXECUTEPROCESSFLAG_IGNORE_ORPHANED  RT_BIT(1)
#define EXECUTEPROCESSFLAG_HIDDEN           RT_BIT(2)
#define EXECUTEPROCESSFLAG_PROFILE          RT_BIT(3)
#define EXECUTEPROCESSFLAG_WAIT_STDOUT      RT_BIT(4)
#define EXECUTEPROCESSFLAG_WAIT_STDERR      RT_BIT(5)
#define EXECUTEPROCESSFLAG_EXPAND_ARGUMENTS RT_BIT(6)
#define EXECUTEPROCESSFLAG_UNQUOTED_ARGS    RT_BIT(7)
/** @} */

/** @name OUTPUT_HANDLE_ID_XXX - Pipe handle IDs used internally for referencing
 *        to a certain pipe buffer.
 * @{
 */
#define OUTPUT_HANDLE_ID_STDOUT_DEPRECATED  0 /**< Needed for VBox hosts < 4.1.0. */
#define OUTPUT_HANDLE_ID_STDOUT             1
#define OUTPUT_HANDLE_ID_STDERR             2
/** @} */

/** @name PATHRENAME_FLAG_XXX - Guest path rename flags.
 * Essentially using what IPRT's RTPATHRENAME_FLAGS_XXX have to offer.
 * @{
 */
/** Do not replace anything. */
#define PATHRENAME_FLAG_NO_REPLACE          UINT32_C(0)
/** This will replace attempt any target which isn't a directory. */
#define PATHRENAME_FLAG_REPLACE             RT_BIT(0)
/** Don't allow symbolic links as part of the path. */
#define PATHRENAME_FLAG_NO_SYMLINKS         RT_BIT(1)
/** Mask of valid flags. */
#define PATHRENAME_FLAG_VALID_MASK          UINT32_C(0x00000002)
/** @} */

/** @name Defines for guest process array lengths.
 * @{
 */
#define GUESTPROCESS_MAX_CMD_LEN            _1K
#define GUESTPROCESS_MAX_ARGS_LEN           _1K
#define GUESTPROCESS_MAX_ENV_LEN            _64K
#define GUESTPROCESS_MAX_USER_LEN           128
#define GUESTPROCESS_MAX_PASSWORD_LEN       128
#define GUESTPROCESS_MAX_DOMAIN_LEN         256
/** @} */

/** @name Internal tools built into VBoxService which are used in order
 *        to accomplish tasks host<->guest.
 * @{
 */
#define VBOXSERVICE_TOOL_CAT        "vbox_cat"
#define VBOXSERVICE_TOOL_LS         "vbox_ls"
#define VBOXSERVICE_TOOL_RM         "vbox_rm"
#define VBOXSERVICE_TOOL_MKDIR      "vbox_mkdir"
#define VBOXSERVICE_TOOL_MKTEMP     "vbox_mktemp"
#define VBOXSERVICE_TOOL_STAT       "vbox_stat"
/** @} */

/** Special process exit codes for "vbox_cat". */
typedef enum VBOXSERVICETOOLBOX_CAT_EXITCODE
{
    VBOXSERVICETOOLBOX_CAT_EXITCODE_ACCESS_DENIED = RTEXITCODE_END,
    VBOXSERVICETOOLBOX_CAT_EXITCODE_FILE_NOT_FOUND,
    VBOXSERVICETOOLBOX_CAT_EXITCODE_PATH_NOT_FOUND,
    VBOXSERVICETOOLBOX_CAT_EXITCODE_SHARING_VIOLATION,
    /** The usual 32-bit type hack. */
    VBOXSERVICETOOLBOX_CAT_32BIT_HACK = 0x7fffffff
} VBOXSERVICETOOLBOX_CAT_EXITCODE;

/** Special process exit codes for "vbox_stat". */
typedef enum VBOXSERVICETOOLBOX_STAT_EXITCODE
{
    VBOXSERVICETOOLBOX_STAT_EXITCODE_ACCESS_DENIED = RTEXITCODE_END,
    VBOXSERVICETOOLBOX_STAT_EXITCODE_FILE_NOT_FOUND,
    VBOXSERVICETOOLBOX_STAT_EXITCODE_PATH_NOT_FOUND,
    /** The usual 32-bit type hack. */
    VBOXSERVICETOOLBOX_STAT_32BIT_HACK = 0x7fffffff
} VBOXSERVICETOOLBOX_STAT_EXITCODE;

/**
 * Input status, reported by the client.
 */
enum eInputStatus
{
    /** Input is in an undefined state. */
    INPUT_STS_UNDEFINED = 0,
    /** Input was written (partially, see cbProcessed). */
    INPUT_STS_WRITTEN = 1,
    /** Input failed with an error (see flags for rc). */
    INPUT_STS_ERROR = 20,
    /** Process has abandoned / terminated input handling. */
    INPUT_STS_TERMINATED = 21,
    /** Too much input data. */
    INPUT_STS_OVERFLOW = 30
};



} /* namespace guestControl */

#endif /* !___VBox_GuestHost_GuestControl_h */

