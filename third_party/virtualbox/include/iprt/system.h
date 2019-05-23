/** @file
 * IPRT - System Information.
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

#ifndef ___iprt_system_h
#define ___iprt_system_h

#include <iprt/cdefs.h>
#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_system RTSystem - System Information
 * @ingroup grp_rt
 * @{
 */

/**
 * Info level for RTSystemGetOSInfo().
 */
typedef enum RTSYSOSINFO
{
    RTSYSOSINFO_INVALID = 0,    /**< The usual invalid entry. */
    RTSYSOSINFO_PRODUCT,        /**< OS product name. (uname -o) */
    RTSYSOSINFO_RELEASE,        /**< OS release. (uname -r) */
    RTSYSOSINFO_VERSION,        /**< OS version, optional. (uname -v) */
    RTSYSOSINFO_SERVICE_PACK,   /**< Service/fix pack level, optional. */
    RTSYSOSINFO_END             /**< End of the valid info levels. */
} RTSYSOSINFO;


/**
 * Queries information about the OS.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_INVALID_PARAMETER if enmInfo is invalid.
 * @retval  VERR_INVALID_POINTER if pszInfoStr is invalid.
 * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small. The buffer will
 *          contain the chopped off result in this case, provided cchInfo isn't 0.
 * @retval  VERR_NOT_SUPPORTED if the info level isn't implemented. The buffer will
 *          contain an empty string.
 *
 * @param   enmInfo         The OS info level.
 * @param   pszInfo         Where to store the result.
 * @param   cchInfo         The size of the output buffer.
 */
RTDECL(int) RTSystemQueryOSInfo(RTSYSOSINFO enmInfo, char *pszInfo, size_t cchInfo);

/**
 * Queries the total amount of RAM in the system.
 *
 * This figure does not given any information about how much memory is
 * currently available. Use RTSystemQueryAvailableRam instead.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS and *pcb on sucess.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 *
 * @param   pcb             Where to store the result (in bytes).
 */
RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb);

/**
 * Queries the total amount of RAM accessible to the system.
 *
 * This figure should not include memory that is installed but not used,
 * nor memory that will be slow to bring online. The definition of 'slow'
 * here is slower than swapping out a MB of pages to disk.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS and *pcb on success.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 *
 * @param   pcb             Where to store the result (in bytes).
 */
RTDECL(int) RTSystemQueryAvailableRam(uint64_t *pcb);

/**
 * Queries the amount of RAM that is currently locked down or in some other
 * way made impossible to virtualize within reasonably short time.
 *
 * The purposes of this API is, when combined with RTSystemQueryTotalRam, to
 * be able to determine an absolute max limit for how much fixed memory it is
 * (theoretically) possible to allocate (or lock down).
 *
 * The kind memory covered by this function includes:
 *      - locked (wired) memory - like for instance RTR0MemObjLockUser
 *        and RTR0MemObjLockKernel makes,
 *      - kernel pools and heaps - like for instance the ring-0 variant
 *        of RTMemAlloc taps into,
 *      - fixed (not pageable) kernel allocations - like for instance
 *        all the RTR0MemObjAlloc* functions makes,
 *      - any similar memory that isn't easily swapped out, discarded,
 *        or flushed to disk.
 *
 * This works against the value returned by RTSystemQueryTotalRam, and
 * the value reported by this function can never be larger than what a
 * call to RTSystemQueryTotalRam returns.
 *
 * The short time term here is relative to swapping to disk like in
 * RTSystemQueryTotalRam. This could mean that (part of) the dirty buffers
 * in the dynamic I/O cache could be included in the total. If the dynamic
 * I/O cache isn't likely to either flush buffers when the load increases
 * and put them back into normal circulation, they should be included in
 * the memory accounted for here.
 *
 * @retval  VINF_SUCCESS and *pcb on success.
 * @retval  VERR_NOT_SUPPORTED if the information isn't available on the
 *          system in general. The caller must handle this scenario.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 *
 * @param   pcb             Where to store the result (in bytes).
 *
 * @remarks This function could've been inverted and called
 *          RTSystemQueryAvailableRam, but that might give impression that
 *          it would be possible to allocate the amount of memory it
 *          indicates for a single purpose, something which would be very
 *          improbable on most systems.
 *
 * @remarks We might have to add another output parameter to this function
 *          that indicates if some of the memory kinds listed above cannot
 *          be accounted for on the system and therefore is not include in
 *          the returned amount.
 */
RTDECL(int) RTSystemQueryUnavailableRam(uint64_t *pcb);


/**
 * The DMI strings.
 */
typedef enum RTSYSDMISTR
{
    /** Invalid zero entry. */
    RTSYSDMISTR_INVALID = 0,
    /** The product name. */
    RTSYSDMISTR_PRODUCT_NAME,
    /** The product version. */
    RTSYSDMISTR_PRODUCT_VERSION,
    /** The product UUID. */
    RTSYSDMISTR_PRODUCT_UUID,
    /** The product serial. */
    RTSYSDMISTR_PRODUCT_SERIAL,
    /** The system manufacturer. */
    RTSYSDMISTR_MANUFACTURER,
    /** The end of the valid strings. */
    RTSYSDMISTR_END,
    /** The usual 32-bit hack.  */
    RTSYSDMISTR_32_BIT_HACK = 0x7fffffff
} RTSYSDMISTR;

/**
 * Queries a DMI string.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small.  The buffer will
 *          contain the chopped off result in this case, provided cbBuf isn't 0.
 * @retval  VERR_ACCESS_DENIED if the information isn't accessible to the
 *          caller.
 * @retval  VERR_NOT_SUPPORTED if the information isn't available on the system
 *          in general.  The caller must expect this status code and deal with
 *          it.
 *
 * @param   enmString           Which string to query.
 * @param   pszBuf              Where to store the string.  This is always
 *                              terminated, even on error.
 * @param   cbBuf               The buffer size.
 */
RTDECL(int) RTSystemQueryDmiString(RTSYSDMISTR enmString, char *pszBuf, size_t cbBuf);

/** @name Flags for RTSystemReboot and RTSystemShutdown.
 * @{ */
/** Reboot the system after shutdown. */
#define RTSYSTEM_SHUTDOWN_REBOOT            UINT32_C(0)
/** Reboot the system after shutdown.
 * The call may return VINF_SYS_MAY_POWER_OFF if the OS /
 * hardware combination may power off instead of halting. */
#define RTSYSTEM_SHUTDOWN_HALT              UINT32_C(1)
/** Power off the system after shutdown.
 * This may be equvivalent to a RTSYSTEM_SHUTDOWN_HALT on systems where we
 * cannot figure out whether the hardware/OS implements the actual powering
 * off.  If we can figure out that it's not supported, an
 * VERR_SYS_CANNOT_POWER_OFF error is raised. */
#define RTSYSTEM_SHUTDOWN_POWER_OFF         UINT32_C(2)
/** Power off the system after shutdown, or halt it if that's not possible. */
#define RTSYSTEM_SHUTDOWN_POWER_OFF_HALT    UINT32_C(3)
/** The shutdown action mask. */
#define RTSYSTEM_SHUTDOWN_ACTION_MASK       UINT32_C(3)
/** Unplanned shutdown/reboot. */
#define RTSYSTEM_SHUTDOWN_UNPLANNED         UINT32_C(0)
/** Planned shutdown/reboot. */
#define RTSYSTEM_SHUTDOWN_PLANNED           RT_BIT_32(2)
/** Force the system to shutdown/reboot regardless of objecting application
 *  or other stuff.  This flag might not be realized on all systems. */
#define RTSYSTEM_SHUTDOWN_FORCE             RT_BIT_32(3)
/** Parameter validation mask. */
#define RTSYSTEM_SHUTDOWN_VALID_MASK        UINT32_C(0x0000000f)
/** @} */

/**
 * Shuts down the system.
 *
 * @returns IPRT status code on failure, on success it may or may not return
 *          depending on the OS.
 * @retval  VINF_SUCCESS
 * @retval  VINF_SYS_MAY_POWER_OFF
 * @retval  VERR_SYS_SHUTDOWN_FAILED
 * @retval  VERR_SYS_CANNOT_POWER_OFF
 *
 * @param   cMsDelay            The delay before the actual reboot.  If this is
 *                              not supported by the OS, an immediate reboot
 *                              will be performed.
 * @param   fFlags              Shutdown flags, see RTSYSTEM_SHUTDOWN_XXX.
 * @param   pszLogMsg           Message for the log and users about why we're
 *                              shutting down.
 */
RTDECL(int) RTSystemShutdown(RTMSINTERVAL cMsDelay, uint32_t fFlags, const char *pszLogMsg);

/**
 * Checks if we're executing inside a virtual machine (VM).
 *
 * The current implemention is very simplistic and won't try to detect the
 * presence of a virtual machine monitor (VMM) unless it openly tells us it is
 * there.
 *
 * @returns true if inside a VM, false if on real hardware.
 *
 * @todo    If more information is needed, like which VMM it is and which
 *          version and such, add one or two new APIs.
 */
RTDECL(bool) RTSystemIsInsideVM(void);

/** @} */

RT_C_DECLS_END

#endif

