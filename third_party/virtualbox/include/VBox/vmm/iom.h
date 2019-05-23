/** @file
 * IOM - Input / Output Monitor.
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

#ifndef ___VBox_vmm_iom_h
#define ___VBox_vmm_iom_h

#include <VBox/types.h>
#include <VBox/dis.h>
#include <VBox/vmm/dbgf.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_iom   The Input / Ouput Monitor API
 * @ingroup grp_vmm
 * @{
 */

/** @def IOM_NO_PDMINS_CHECKS
 * Until all devices have been fully adjusted to PDM style, the pPdmIns
 * parameter is not checked by IOM.
 * @todo Check this again, now.
 */
#define IOM_NO_PDMINS_CHECKS

/**
 * Macro for checking if an I/O or MMIO emulation call succeeded.
 *
 * This macro shall only be used with the IOM APIs where it's mentioned
 * in the return value description.  And there it must be used to correctly
 * determine if the call succeeded and things like the RIP needs updating.
 *
 *
 * @returns Success indicator (true/false).
 *
 * @param   rc          The status code.  This may be evaluated
 *                      more than once!
 *
 * @remarks To avoid making assumptions about the layout of the
 *          VINF_EM_FIRST...VINF_EM_LAST range we're checking explicitly for
 *          each exact exception. However, for efficiency we ASSUME that the
 *          VINF_EM_LAST is smaller than most of the relevant status codes. We
 *          also ASSUME that the VINF_EM_RESCHEDULE_REM status code is the
 *          most frequent status code we'll enounter in this range.
 *
 * @todo    Will have to add VINF_EM_DBG_HYPER_BREAKPOINT if the
 *          I/O port and MMIO breakpoints should trigger before
 *          the I/O is done.  Currently, we don't implement these
 *          kind of breakpoints.
 */
#ifdef IN_RING3
# define IOM_SUCCESS(rc)   (   (rc) == VINF_SUCCESS \
                             || (   (rc) <= VINF_EM_LAST \
                                 && (rc) != VINF_EM_RESCHEDULE_REM \
                                 && (rc) >= VINF_EM_FIRST \
                                 && (rc) != VINF_EM_RESCHEDULE_RAW \
                                 && (rc) != VINF_EM_RESCHEDULE_HM \
                                ) \
                            )
#else
# define IOM_SUCCESS(rc)   (   (rc) == VINF_SUCCESS \
                             || (   (rc) <= VINF_EM_LAST \
                                 && (rc) != VINF_EM_RESCHEDULE_REM \
                                 && (rc) >= VINF_EM_FIRST \
                                 && (rc) != VINF_EM_RESCHEDULE_RAW \
                                 && (rc) != VINF_EM_RESCHEDULE_HM \
                                ) \
                             || (rc) == VINF_IOM_R3_IOPORT_COMMIT_WRITE \
                             || (rc) == VINF_IOM_R3_MMIO_COMMIT_WRITE \
                            )
#endif

/** @name IOMMMIO_FLAGS_XXX
 * @{ */
/** Pass all reads thru unmodified. */
#define IOMMMIO_FLAGS_READ_PASSTHRU                     UINT32_C(0x00000000)
/** All read accesses are DWORD sized (32-bit). */
#define IOMMMIO_FLAGS_READ_DWORD                        UINT32_C(0x00000001)
/** All read accesses are DWORD (32-bit) or QWORD (64-bit) sized.
 * Only accesses that are both QWORD sized and aligned are performed as QWORD.
 * All other access will be done DWORD fashion (because it is way simpler). */
#define IOMMMIO_FLAGS_READ_DWORD_QWORD                  UINT32_C(0x00000002)
/** The read access mode mask. */
#define IOMMMIO_FLAGS_READ_MODE                         UINT32_C(0x00000003)

/** Pass all writes thru unmodified. */
#define IOMMMIO_FLAGS_WRITE_PASSTHRU                    UINT32_C(0x00000000)
/** All write accesses are DWORD (32-bit) sized and unspecified bytes are
 * written as zero. */
#define IOMMMIO_FLAGS_WRITE_DWORD_ZEROED                UINT32_C(0x00000010)
/** All write accesses are either DWORD (32-bit) or QWORD (64-bit) sized,
 * missing bytes will be written as zero.  Only accesses that are both QWORD
 * sized and aligned are performed as QWORD, all other accesses will be done
 * DWORD fashion (because it's way simpler). */
#define IOMMMIO_FLAGS_WRITE_DWORD_QWORD_ZEROED          UINT32_C(0x00000020)
/** All write accesses are DWORD (32-bit) sized and unspecified bytes are
 * read from the device first as DWORDs.
 * @remarks This isn't how it happens on real hardware, but it allows
 *          simplifications of devices where reads doesn't change the device
 *          state in any way. */
#define IOMMMIO_FLAGS_WRITE_DWORD_READ_MISSING          UINT32_C(0x00000030)
/** All write accesses are DWORD (32-bit) or QWORD (64-bit) sized and
 * unspecified bytes are read from the device first as DWORDs.  Only accesses
 * that are both QWORD sized and aligned are performed as QWORD, all other
 * accesses will be done DWORD fashion (because it's way simpler).
 * @remarks This isn't how it happens on real hardware, but it allows
 *          simplifications of devices where reads doesn't change the device
 *          state in any way. */
#define IOMMMIO_FLAGS_WRITE_DWORD_QWORD_READ_MISSING    UINT32_C(0x00000040)
/** All write accesses are DWORD (32-bit) sized and aligned, attempts at other
 * accesses are ignored.
 * @remarks E1000, APIC */
#define IOMMMIO_FLAGS_WRITE_ONLY_DWORD                  UINT32_C(0x00000050)
/** All write accesses are DWORD (32-bit) or QWORD (64-bit) sized and aligned,
 * attempts at other accesses are ignored.
 * @remarks Seemingly required by AHCI (although I doubt it's _really_
 *          required as EM/REM doesn't do the right thing in ring-3 anyway,
 *          esp. not in raw-mode). */
#define IOMMMIO_FLAGS_WRITE_ONLY_DWORD_QWORD            UINT32_C(0x00000060)
/** The read access mode mask. */
#define IOMMMIO_FLAGS_WRITE_MODE                        UINT32_C(0x00000070)

/** Whether to do a DBGSTOP on complicated reads.
 * What this includes depends on the read mode, but generally all misaligned
 * reads as well as word and byte reads and maybe qword reads. */
#define IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_READ       UINT32_C(0x00000100)
/** Whether to do a DBGSTOP on complicated writes.
 * This depends on the write mode, but generally all writes where we have to
 * supply bytes (zero them or read them). */
#define IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_WRITE      UINT32_C(0x00000200)

/** Mask of valid flags. */
#define IOMMMIO_FLAGS_VALID_MASK                        UINT32_C(0x00000373)
/** @} */

/**
 * Checks whether the write mode allows aligned QWORD accesses to be passed
 * thru to the device handler.
 * @param   a_fFlags        The MMIO handler flags.
 * @remarks The current implementation makes ASSUMPTIONS about the mode values!
 */
#define IOMMMIO_DOES_WRITE_MODE_ALLOW_QWORD(a_fFlags)   RT_BOOL((a_fFlags) & UINT32_C(0x00000020))


/**
 * Port I/O Handler for IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the IN operation.
 * @param   pu32        Where to store the result.  This is always a 32-bit
 *                      variable regardless of what @a cb might say.
 * @param   cb          Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMIOPORTIN(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb);
/** Pointer to a FNIOMIOPORTIN(). */
typedef FNIOMIOPORTIN *PFNIOMIOPORTIN;

/**
 * Port I/O Handler for string IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the IN operation.
 * @param   pbDst       Pointer to the destination buffer.
 * @param   pcTransfers Pointer to the number of transfer units to read, on
 *                      return remaining transfer units.
 * @param   cb          Size of the transfer unit (1, 2 or 4 bytes).
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMIOPORTINSTRING(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint8_t *pbDst,
                                              uint32_t *pcTransfers, unsigned cb);
/** Pointer to a FNIOMIOPORTINSTRING(). */
typedef FNIOMIOPORTINSTRING *PFNIOMIOPORTINSTRING;

/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMIOPORTOUT(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb);
/** Pointer to a FNIOMIOPORTOUT(). */
typedef FNIOMIOPORTOUT *PFNIOMIOPORTOUT;

/**
 * Port I/O Handler for string OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   pbSrc       Pointer to the source buffer.
 * @param   pcTransfers Pointer to the number of transfer units to write, on
 *                      return remaining transfer units.
 * @param   cb          Size of the transfer unit (1, 2 or 4 bytes).
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMIOPORTOUTSTRING(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, const uint8_t *pbSrc,
                                               uint32_t *pcTransfers, unsigned cb);
/** Pointer to a FNIOMIOPORTOUTSTRING(). */
typedef FNIOMIOPORTOUTSTRING *PFNIOMIOPORTOUTSTRING;


/**
 * Memory mapped I/O Handler for read operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the read starts.
 * @param   pv          Where to store the result.
 * @param   cb          Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMMMIOREAD(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb);
/** Pointer to a FNIOMMMIOREAD(). */
typedef FNIOMMMIOREAD *PFNIOMMMIOREAD;

/**
 * Memory mapped I/O Handler for write operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the read starts.
 * @param   pv          Where to fetch the result.
 * @param   cb          Number of bytes to write.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMMMIOWRITE(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb);
/** Pointer to a FNIOMMMIOWRITE(). */
typedef FNIOMMMIOWRITE *PFNIOMMMIOWRITE;

/**
 * Memory mapped I/O Handler for memset operations, actually for REP STOS* instructions handling.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the write starts.
 * @param   u32Item     Byte/Word/Dword data to fill.
 * @param   cbItem      Size of data in u32Item parameter, restricted to 1/2/4 bytes.
 * @param   cItems      Number of iterations.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACK(int) FNIOMMMIOFILL(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, uint32_t u32Item, unsigned cbItem, unsigned cItems);
/** Pointer to a FNIOMMMIOFILL(). */
typedef FNIOMMMIOFILL *PFNIOMMMIOFILL;

VMMDECL(VBOXSTRICTRC)   IOMIOPortRead(PVM pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t *pu32Value, size_t cbValue);
VMMDECL(VBOXSTRICTRC)   IOMIOPortWrite(PVM pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue);
VMM_INT_DECL(VBOXSTRICTRC) IOMIOPortReadString(PVM pVM, PVMCPU pVCpu, RTIOPORT Port, void *pvDst,
                                               uint32_t *pcTransfers, unsigned cb);
VMM_INT_DECL(VBOXSTRICTRC) IOMIOPortWriteString(PVM pVM, PVMCPU pVCpu, RTIOPORT uPort, void const *pvSrc,
                                                uint32_t *pcTransfers, unsigned cb);
VMMDECL(VBOXSTRICTRC)   IOMInterpretINSEx(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t uPort, uint32_t uPrefix, DISCPUMODE enmAddrMode, uint32_t cbTransfer);
VMMDECL(VBOXSTRICTRC)   IOMInterpretOUTSEx(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, uint32_t uPort, uint32_t uPrefix, DISCPUMODE enmAddrMode, uint32_t cbTransfer);
VMMDECL(VBOXSTRICTRC)   IOMMMIORead(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, uint32_t *pu32Value, size_t cbValue);
VMMDECL(VBOXSTRICTRC)   IOMMMIOWrite(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, uint32_t u32Value, size_t cbValue);
VMMDECL(VBOXSTRICTRC)   IOMMMIOPhysHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pCtxCore, RTGCPHYS GCPhysFault);
VMMDECL(int)            IOMMMIOMapMMIO2Page(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysRemapped, uint64_t fPageFlags);
VMMDECL(int)            IOMMMIOMapMMIOHCPage(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint64_t fPageFlags);
VMMDECL(int)            IOMMMIOResetRegion(PVM pVM, RTGCPHYS GCPhys);
VMMDECL(bool)           IOMIsLockWriteOwner(PVM pVM);

#ifdef IN_RC
/** @defgroup grp_iom_rc    The IOM Raw-Mode Context API
 * @{
 */
VMMRCDECL(VBOXSTRICTRC) IOMRCIOPortHandler(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, PDISCPUSTATE pCpu);
/** @} */
#endif /* IN_RC */



#ifdef IN_RING3
/** @defgroup grp_iom_r3    The IOM Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)  IOMR3Init(PVM pVM);
VMMR3_INT_DECL(void) IOMR3Reset(PVM pVM);
VMMR3_INT_DECL(void) IOMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)  IOMR3Term(PVM pVM);
VMMR3_INT_DECL(int)  IOMR3IOPortRegisterR3(PVM pVM, PPDMDEVINS pDevIns, RTIOPORT PortStart, RTUINT cPorts, RTHCPTR pvUser,
                                           R3PTRTYPE(PFNIOMIOPORTOUT) pfnOutCallback, R3PTRTYPE(PFNIOMIOPORTIN) pfnInCallback,
                                           R3PTRTYPE(PFNIOMIOPORTOUTSTRING) pfnOutStringCallback, R3PTRTYPE(PFNIOMIOPORTINSTRING) pfnInStringCallback,
                                           const char *pszDesc);
VMMR3_INT_DECL(int)  IOMR3IOPortRegisterRC(PVM pVM, PPDMDEVINS pDevIns, RTIOPORT PortStart, RTUINT cPorts, RTRCPTR pvUser,
                                           RCPTRTYPE(PFNIOMIOPORTOUT) pfnOutCallback, RCPTRTYPE(PFNIOMIOPORTIN) pfnInCallback,
                                           RCPTRTYPE(PFNIOMIOPORTOUTSTRING) pfnOutStrCallback, RCPTRTYPE(PFNIOMIOPORTINSTRING) pfnInStrCallback,
                                           const char *pszDesc);
VMMR3_INT_DECL(int)  IOMR3IOPortRegisterR0(PVM pVM, PPDMDEVINS pDevIns, RTIOPORT PortStart, RTUINT cPorts, RTR0PTR pvUser,
                                           R0PTRTYPE(PFNIOMIOPORTOUT) pfnOutCallback, R0PTRTYPE(PFNIOMIOPORTIN) pfnInCallback,
                                           R0PTRTYPE(PFNIOMIOPORTOUTSTRING) pfnOutStrCallback, R0PTRTYPE(PFNIOMIOPORTINSTRING) pfnInStrCallback,
                                           const char *pszDesc);
VMMR3_INT_DECL(int)  IOMR3IOPortDeregister(PVM pVM, PPDMDEVINS pDevIns, RTIOPORT PortStart, RTUINT cPorts);

VMMR3_INT_DECL(int)  IOMR3MmioRegisterR3(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTHCPTR pvUser,
                                         R3PTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallback,
                                         R3PTRTYPE(PFNIOMMMIOREAD)  pfnReadCallback,
                                         R3PTRTYPE(PFNIOMMMIOFILL)  pfnFillCallback,
                                         uint32_t fFlags, const char *pszDesc);
VMMR3_INT_DECL(int)  IOMR3MmioRegisterR0(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTR0PTR pvUser,
                                         R0PTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallback,
                                         R0PTRTYPE(PFNIOMMMIOREAD)  pfnReadCallback,
                                         R0PTRTYPE(PFNIOMMMIOFILL)  pfnFillCallback);
VMMR3_INT_DECL(int)  IOMR3MmioRegisterRC(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTGCPTR pvUser,
                                         RCPTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallback,
                                         RCPTRTYPE(PFNIOMMMIOREAD)  pfnReadCallback,
                                         RCPTRTYPE(PFNIOMMMIOFILL)  pfnFillCallback);
VMMR3_INT_DECL(int)  IOMR3MmioDeregister(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange);
VMMR3_INT_DECL(int)  IOMR3MmioExPreRegister(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS cbRange,
                                            uint32_t fFlags, const char *pszDesc,
                                            RTR3PTR pvUserR3,
                                            R3PTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallbackR3,
                                            R3PTRTYPE(PFNIOMMMIOREAD)  pfnReadCallbackR3,
                                            R3PTRTYPE(PFNIOMMMIOFILL)  pfnFillCallbackR3,
                                            RTR0PTR pvUserR0,
                                            R0PTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallbackR0,
                                            R0PTRTYPE(PFNIOMMMIOREAD)  pfnReadCallbackR0,
                                            R0PTRTYPE(PFNIOMMMIOFILL)  pfnFillCallbackR0,
                                            RTRCPTR pvUserRC,
                                            RCPTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallbackRC,
                                            RCPTRTYPE(PFNIOMMMIOREAD)  pfnReadCallbackRC,
                                            RCPTRTYPE(PFNIOMMMIOFILL)  pfnFillCallbackRC);
VMMR3_INT_DECL(int)  IOMR3MmioExNotifyMapped(PVM pVM, void *pvUser, RTGCPHYS GCPhys);
VMMR3_INT_DECL(void) IOMR3MmioExNotifyUnmapped(PVM pVM, void *pvUser, RTGCPHYS GCPhys);
VMMR3_INT_DECL(void) IOMR3MmioExNotifyDeregistered(PVM pVM, void *pvUser);

VMMR3_INT_DECL(VBOXSTRICTRC) IOMR3ProcessForceFlag(PVM pVM, PVMCPU pVCpu, VBOXSTRICTRC rcStrict);

VMMR3_INT_DECL(void) IOMR3NotifyBreakpointCountChange(PVM pVM, bool fPortIo, bool fMmio);
VMMR3_INT_DECL(void) IOMR3NotifyDebugEventChange(PVM pVM, DBGFEVENT enmEvent, bool fEnabled);

/** @} */
#endif /* IN_RING3 */


/** @} */

RT_C_DECLS_END

#endif

