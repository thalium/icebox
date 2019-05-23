/** @file
 * GIM - Guest Interface Manager.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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

#ifndef ___VBox_vmm_gim_h
#define ___VBox_vmm_gim_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/param.h>

#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pdmifs.h>

/** The value used to specify that VirtualBox must use the newest
 *  implementation version of the GIM provider. */
#define GIM_VERSION_LATEST                  UINT32_C(0)

RT_C_DECLS_BEGIN

/** @defgroup grp_gim   The Guest Interface Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * GIM Provider Identifiers.
 * @remarks Part of saved state!
 */
typedef enum GIMPROVIDERID
{
    /** None. */
    GIMPROVIDERID_NONE = 0,
    /** Minimal. */
    GIMPROVIDERID_MINIMAL,
    /** Microsoft Hyper-V. */
    GIMPROVIDERID_HYPERV,
    /** Linux KVM Interface. */
    GIMPROVIDERID_KVM
} GIMPROVIDERID;
AssertCompileSize(GIMPROVIDERID, sizeof(uint32_t));


/**
 * A GIM MMIO2 region record.
 */
typedef struct GIMMMIO2REGION
{
    /** The region index. */
    uint8_t             iRegion;
    /** Whether an RC mapping is required. */
    bool                fRCMapping;
    /** Whether this region has been registered. */
    bool                fRegistered;
    /** Whether this region is currently mapped. */
    bool                fMapped;
    /** Alignment padding. */
    uint8_t             au8Alignment0[3];
    /** Size of the region (must be page aligned). */
    uint32_t            cbRegion;
    /** Alignment padding. */
    uint32_t            u32Alignment0;
    /** The host ring-0 address of the first page in the region. */
    R0PTRTYPE(void *)   pvPageR0;
    /** The host ring-3 address of the first page in the region. */
    R3PTRTYPE(void *)   pvPageR3;
    /** The ring-context address of the first page in the region. */
    RCPTRTYPE(void *)   pvPageRC;
    /** The guest-physical address of the first page in the region. */
    RTGCPHYS            GCPhysPage;
    /** The description of the region. */
    char                szDescription[32];
} GIMMMIO2REGION;
/** Pointer to a GIM MMIO2 region. */
typedef GIMMMIO2REGION *PGIMMMIO2REGION;
/** Pointer to a const GIM MMIO2 region. */
typedef GIMMMIO2REGION const *PCGIMMMIO2REGION;
AssertCompileMemberAlignment(GIMMMIO2REGION, cbRegion, 8);
AssertCompileMemberAlignment(GIMMMIO2REGION, pvPageR0, 8);

/**
 * Debug data buffer available callback over the GIM debug connection.
 *
 * @param   pVM             The cross context VM structure.
 */
typedef DECLCALLBACK(void) FNGIMDEBUGBUFAVAIL(PVM pVM);
/** Pointer to GIM debug buffer available callback. */
typedef FNGIMDEBUGBUFAVAIL *PFNGIMDEBUGBUFAVAIL;

/**
 * GIM debug setup.
 *
 * These are parameters/options filled in by the GIM provider and passed along
 * to the GIM device.
 */
typedef struct GIMDEBUGSETUP
{
    /** The callback to invoke when the receive buffer has data. */
    PFNGIMDEBUGBUFAVAIL     pfnDbgRecvBufAvail;
    /** The size of the receive buffer as specified by the GIM provider. */
    uint32_t                cbDbgRecvBuf;
} GIMDEBUGSETUP;
/** Pointer to a GIM debug setup struct. */
typedef struct GIMDEBUGSETUP *PGIMDEBUGSETUP;
/** Pointer to a const GIM debug setup struct. */
typedef struct GIMDEBUGSETUP const *PCGGIMDEBUGSETUP;

/**
 * GIM debug structure (common to the GIM device and GIM).
 *
 * This is used to exchanging data between the GIM provider and the GIM device.
 */
typedef struct GIMDEBUG
{
    /** The receive buffer. */
    void                   *pvDbgRecvBuf;
    /** The debug I/O stream driver. */
    PPDMISTREAM             pDbgDrvStream;
    /** Number of bytes pending to be read from the receive buffer. */
    size_t                  cbDbgRecvBufRead;
    /** The flag synchronizing reads of the receive buffer from EMT. */
    volatile bool           fDbgRecvBufRead;
    /** The receive thread wakeup semaphore. */
    RTSEMEVENTMULTI         hDbgRecvThreadSem;
} GIMDEBUG;
/** Pointer to a GIM debug struct. */
typedef struct GIMDEBUG *PGIMDEBUG;
/** Pointer to a const GIM debug struct. */
typedef struct GIMDEBUG const *PCGIMDEBUG;


#ifdef IN_RC
/** @defgroup grp_gim_rc  The GIM Raw-mode Context API
 * @{
 */
/** @} */
#endif /* IN_RC */

#ifdef IN_RING0
/** @defgroup grp_gim_r0  The GIM Host Context Ring-0 API
 * @{
 */
VMMR0_INT_DECL(int)         GIMR0InitVM(PVM pVM);
VMMR0_INT_DECL(int)         GIMR0TermVM(PVM pVM);
VMMR0_INT_DECL(int)         GIMR0UpdateParavirtTsc(PVM pVM, uint64_t u64Offset);
/** @} */
#endif /* IN_RING0 */


#ifdef IN_RING3
/** @defgroup grp_gim_r3  The GIM Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)         GIMR3Init(PVM pVM);
VMMR3_INT_DECL(int)         GIMR3InitCompleted(PVM pVM);
VMMR3_INT_DECL(void)        GIMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)         GIMR3Term(PVM pVM);
VMMR3_INT_DECL(void)        GIMR3Reset(PVM pVM);
VMMR3DECL(void)             GIMR3GimDeviceRegister(PVM pVM, PPDMDEVINS pDevInsR3, PGIMDEBUG pDbg);
VMMR3DECL(int)              GIMR3GetDebugSetup(PVM pVM, PGIMDEBUGSETUP pDbgSetup);
VMMR3DECL(PGIMMMIO2REGION)  GIMR3GetMmio2Regions(PVM pVM, uint32_t *pcRegions);
/** @} */
#endif /* IN_RING3 */

VMMDECL(bool)               GIMIsEnabled(PVM pVM);
VMMDECL(GIMPROVIDERID)      GIMGetProvider(PVM pVM);
VMM_INT_DECL(bool)          GIMIsParavirtTscEnabled(PVM pVM);
VMM_INT_DECL(bool)          GIMAreHypercallsEnabled(PVMCPU pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  GIMHypercall(PVMCPU pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(VBOXSTRICTRC)  GIMExecHypercallInstr(PVMCPU pVCpu, PCPUMCTX pCtx, uint8_t *pcbInstr);
VMM_INT_DECL(VBOXSTRICTRC)  GIMXcptUD(PVMCPU pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis, uint8_t *pcbInstr);
VMM_INT_DECL(bool)          GIMShouldTrapXcptUD(PVMCPU pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  GIMReadMsr(PVMCPU pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t *puValue);
VMM_INT_DECL(VBOXSTRICTRC)  GIMWriteMsr(PVMCPU pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t uValue, uint64_t uRawValue);
/** @} */

RT_C_DECLS_END

#endif

