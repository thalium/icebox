/* $Id: SELMInternal.h $ */
/** @file
 * SELM - Internal header file.
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
 */

#ifndef ___SELMInternal_h
#define ___SELMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pgm.h>
#include <VBox/log.h>
#include <iprt/x86.h>



/** @defgroup grp_selm_int   Internals
 * @ingroup grp_selm
 * @internal
 * @{
 */

/** Enable or disable tracking of Shadow GDT/LDT/TSS.
 * @{
 */
#if defined(VBOX_WITH_RAW_MODE) || defined(DOXYGEN_RUNNING)
# define SELM_TRACK_SHADOW_GDT_CHANGES
# define SELM_TRACK_SHADOW_LDT_CHANGES
# define SELM_TRACK_SHADOW_TSS_CHANGES
#endif
/** @} */

/** Enable or disable tracking of Guest GDT/LDT/TSS.
 * @{
 */
#if defined(VBOX_WITH_RAW_MODE) || defined(DOXYGEN_RUNNING)
# define SELM_TRACK_GUEST_GDT_CHANGES
# define SELM_TRACK_GUEST_LDT_CHANGES
# define SELM_TRACK_GUEST_TSS_CHANGES
#endif
/** @} */


/** The number of GDTS allocated for our GDT. (full size) */
#define SELM_GDT_ELEMENTS                   8192

/** aHyperSel index to retrieve hypervisor selectors */
/** The Flat CS selector used by the VMM inside the GC. */
#define SELM_HYPER_SEL_CS                   0
/** The Flat DS selector used by the VMM inside the GC. */
#define SELM_HYPER_SEL_DS                   1
/** The 64-bit mode CS selector used by the VMM inside the GC. */
#define SELM_HYPER_SEL_CS64                 2
/** The TSS selector used by the VMM inside the GC. */
#define SELM_HYPER_SEL_TSS                  3
/** The TSS selector for taking trap 08 (\#DF). */
#define SELM_HYPER_SEL_TSS_TRAP08           4
/** Number of GDTs we need for internal use */
#define SELM_HYPER_SEL_MAX                  (SELM_HYPER_SEL_TSS_TRAP08 + 1)


/** Default GDT selectors we use for the hypervisor. */
#define SELM_HYPER_DEFAULT_SEL_CS           ((SELM_GDT_ELEMENTS - 0x1) << 3)
#define SELM_HYPER_DEFAULT_SEL_DS           ((SELM_GDT_ELEMENTS - 0x2) << 3)
#define SELM_HYPER_DEFAULT_SEL_CS64         ((SELM_GDT_ELEMENTS - 0x3) << 3)
#define SELM_HYPER_DEFAULT_SEL_TSS          ((SELM_GDT_ELEMENTS - 0x4) << 3)
#define SELM_HYPER_DEFAULT_SEL_TSS_TRAP08   ((SELM_GDT_ELEMENTS - 0x5) << 3)
/** The lowest value default we use. */
#define SELM_HYPER_DEFAULT_BASE             SELM_HYPER_DEFAULT_SEL_TSS_TRAP08

/**
 * Converts a SELM pointer into a VM pointer.
 * @returns Pointer to the VM structure the SELM is part of.
 * @param   pSELM   Pointer to SELM instance data.
 */
#define SELM2VM(pSELM)  ( (PVM)((char *)pSELM - pSELM->offVM) )



/**
 * SELM Data (part of VM)
 */
typedef struct SELM
{
    /** Offset to the VM structure.
     * See SELM2VM(). */
    RTINT                   offVM;

    /** Flat CS, DS, 64 bit mode CS, TSS & trap 8 TSS. */
    RTSEL                   aHyperSel[SELM_HYPER_SEL_MAX];

    /** @name GDT
     * @{ */
    /** Shadow GDT virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hShadowGdtWriteHandlerType;
    /** Guest GDT virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hGuestGdtWriteHandlerType;
    /** Pointer to the GCs - R3 Ptr.
     * This size is governed by SELM_GDT_ELEMENTS. */
    R3PTRTYPE(PX86DESC)     paGdtR3;
    /** Pointer to the GCs - RC Ptr.
     * This is not initialized until the first relocation because it's used to
     * check if the shadow GDT virtual handler requires deregistration. */
    RCPTRTYPE(PX86DESC)     paGdtRC;
    /** Current (last) Guest's GDTR.
     * The pGdt member is set to RTRCPTR_MAX if we're not monitoring the guest GDT. */
    VBOXGDTR                GuestGdtr;
    /** The current (last) effective Guest GDT size. */
    uint32_t                cbEffGuestGdtLimit;
    /** Indicates that the Guest GDT access handler have been registered. */
    bool                    fGDTRangeRegistered;
    /** @} */
    bool                    padding0[3];

    /** @name LDT
     * @{ */
    /** Shadow LDT virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hShadowLdtWriteHandlerType;
    /** Guest LDT virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hGuestLdtWriteHandlerType;
    /** R3 pointer to the LDT shadow area in HMA. */
    R3PTRTYPE(void *)       pvLdtR3;
    /** RC pointer to the LDT shadow area in HMA. */
    RCPTRTYPE(void *)       pvLdtRC;
#if GC_ARCH_BITS == 64
    RTRCPTR                 padding1;
#endif
    /** The address of the guest LDT.
     * RTRCPTR_MAX if not monitored. */
    RTGCPTR                 GCPtrGuestLdt;
    /** Current LDT limit, both Guest and Shadow. */
    uint32_t                cbLdtLimit;
    /** Current LDT offset relative to pvLdtR3/pvLdtRC. */
    uint32_t                offLdtHyper;
#if HC_ARCH_BITS == 32 && GC_ARCH_BITS == 64
    uint32_t                padding2[2];
#endif
    /** @} */

    /** @name TSS
     * @{ */
    /** TSS. (This is 16 byte aligned!)
      * @todo I/O bitmap & interrupt redirection table? */
    VBOXTSS                 Tss;
    /** TSS for trap 08 (\#DF). */
    VBOXTSS                 TssTrap08;
    /** Shadow TSS virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hShadowTssWriteHandlerType;
    /** Guerst TSS virtual write access handler type. */
    PGMVIRTHANDLERTYPE      hGuestTssWriteHandlerType;
    /** Monitored shadow TSS address. */
    RCPTRTYPE(void *)       pvMonShwTssRC;
#if GC_ARCH_BITS == 64
    RTRCPTR                 padding3;
#endif
    /** GC Pointer to the current Guest's TSS.
     * RTRCPTR_MAX if not monitored. */
    RTGCPTR                 GCPtrGuestTss;
    /** The size of the guest TSS. */
    uint32_t                cbGuestTss;
    /** Set if it's a 32-bit TSS. */
    bool                    fGuestTss32Bit;
    /** Indicates whether the TSS stack selector & base address need to be refreshed.  */
    bool                    fSyncTSSRing0Stack;
    /** The size of the Guest's TSS part we're monitoring. */
    uint32_t                cbMonitoredGuestTss;
    /** The guest TSS selector at last sync (part of monitoring).
     * Contains RTSEL_MAX if not set. */
    RTSEL                   GCSelTss;
    /** The last known offset of the I/O bitmap.
     * This is only used if we monitor the bitmap. */
    uint16_t                offGuestIoBitmap;
    /** @} */
    uint16_t                padding4[3];

    /** SELMR3UpdateFromCPUM() profiling. */
    STAMPROFILE             StatUpdateFromCPUM;
    /** SELMR3SyncTSS() profiling. */
    STAMPROFILE             StatTSSSync;

    /** GC: The number of handled writes to the Guest's GDT. */
    STAMCOUNTER             StatRCWriteGuestGDTHandled;
    /** GC: The number of unhandled write to the Guest's GDT. */
    STAMCOUNTER             StatRCWriteGuestGDTUnhandled;
    /** GC: The number of times writes to Guest's LDT was detected. */
    STAMCOUNTER             StatRCWriteGuestLDT;
    /** GC: The number of handled writes to the Guest's TSS. */
    STAMCOUNTER             StatRCWriteGuestTSSHandled;
    /** GC: The number of handled writes to the Guest's TSS where we detected a change. */
    STAMCOUNTER             StatRCWriteGuestTSSHandledChanged;
    /** GC: The number of handled redir writes to the Guest's TSS where we detected a change. */
    STAMCOUNTER             StatRCWriteGuestTSSRedir;
    /** GC: The number of unhandled writes to the Guest's TSS. */
    STAMCOUNTER             StatRCWriteGuestTSSUnhandled;
    /** The number of times we had to relocate our hypervisor selectors. */
    STAMCOUNTER             StatHyperSelsChanged;
    /** The number of times we had find free hypervisor selectors. */
    STAMCOUNTER             StatScanForHyperSels;
    /** Counts the times we detected state selectors in SELMR3UpdateFromCPUM. */
    STAMCOUNTER             aStatDetectedStaleSReg[X86_SREG_COUNT];
    /** Counts the times we were called with already state selectors in
     * SELMR3UpdateFromCPUM. */
    STAMCOUNTER             aStatAlreadyStaleSReg[X86_SREG_COUNT];
    /** Counts the times we found a stale selector becomming valid again. */
    STAMCOUNTER             StatStaleToUnstaleSReg;
#ifdef VBOX_WITH_STATISTICS
    /** Times we updated hidden selector registers in CPUMR3UpdateFromCPUM. */
    STAMCOUNTER             aStatUpdatedSReg[X86_SREG_COUNT];
    STAMCOUNTER             StatLoadHidSelGst;
    STAMCOUNTER             StatLoadHidSelShw;
#endif
    STAMCOUNTER             StatLoadHidSelReadErrors;
    STAMCOUNTER             StatLoadHidSelGstNoGood;
} SELM, *PSELM;

RT_C_DECLS_BEGIN

PGM_ALL_CB2_PROTO(FNPGMVIRTHANDLER) selmGuestGDTWriteHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    selmRCGuestGDTWritePfHandler;
PGM_ALL_CB2_PROTO(FNPGMVIRTHANDLER) selmGuestLDTWriteHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    selmRCGuestLDTWritePfHandler;
PGM_ALL_CB2_PROTO(FNPGMVIRTHANDLER) selmGuestTSSWriteHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    selmRCGuestTSSWritePfHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    selmRCShadowGDTWritePfHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    selmRCShadowLDTWritePfHandler;
DECLEXPORT(FNPGMRCVIRTPFHANDLER)    selmRCShadowTSSWritePfHandler;

void            selmRCSyncGdtSegRegs(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, unsigned iGDTEntry);
void            selmRCGuestGdtPreWriteCheck(PVM pVM, PVMCPU pVCpu, uint32_t offGuestGdt, uint32_t cbWrite, PCPUMCTX pCtx);
VBOXSTRICTRC    selmRCGuestGdtPostWriteCheck(PVM pVM, PVMCPU pVCpu, uint32_t offGuestTss, uint32_t cbWrite, PCPUMCTX pCtx);
VBOXSTRICTRC    selmRCGuestTssPostWriteCheck(PVM pVM, PVMCPU pVCpu, uint32_t offGuestTss, uint32_t cbWrite);
void            selmSetRing1Stack(PVM pVM, uint32_t ss, RTGCPTR32 esp);
#ifdef VBOX_WITH_RAW_RING1
void            selmSetRing2Stack(PVM pVM, uint32_t ss, RTGCPTR32 esp);
#endif

RT_C_DECLS_END

/** @} */

#endif
