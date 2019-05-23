/** @file
 * PGM - Page Monitor / Monitor.
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

#ifndef ___VBox_vmm_pgm_h
#define ___VBox_vmm_pgm_h

#include <VBox/types.h>
#include <VBox/sup.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/vmm/gmm.h>               /* for PGMMREGISTERSHAREDMODULEREQ */
#include <iprt/x86.h>
#include <VBox/VMMDev.h>                /* for VMMDEVSHAREDREGIONDESC */
#include <VBox/param.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_pgm   The Page Monitor / Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * FNPGMRELOCATE callback mode.
 */
typedef enum PGMRELOCATECALL
{
    /** The callback is for checking if the suggested address is suitable. */
    PGMRELOCATECALL_SUGGEST = 1,
    /** The callback is for executing the relocation. */
    PGMRELOCATECALL_RELOCATE
} PGMRELOCATECALL;


/**
 * Callback function which will be called when PGM is trying to find
 * a new location for the mapping.
 *
 * The callback is called in two modes, 1) the check mode and 2) the relocate mode.
 * In 1) the callback should say if it objects to a suggested new location. If it
 * accepts the new location, it is called again for doing it's relocation.
 *
 *
 * @returns true if the location is ok.
 * @returns false if another location should be found.
 * @param   pVM         The cross context VM structure.
 * @param   GCPtrOld    The old virtual address.
 * @param   GCPtrNew    The new virtual address.
 * @param   enmMode     Used to indicate the callback mode.
 * @param   pvUser      User argument.
 * @remark  The return value is no a failure indicator, it's an acceptance
 *          indicator. Relocation can not fail!
 */
typedef DECLCALLBACK(bool) FNPGMRELOCATE(PVM pVM, RTGCPTR GCPtrOld, RTGCPTR GCPtrNew, PGMRELOCATECALL enmMode, void *pvUser);
/** Pointer to a relocation callback function. */
typedef FNPGMRELOCATE *PFNPGMRELOCATE;


/**
 * Memory access origin.
 */
typedef enum PGMACCESSORIGIN
{
    /** Invalid zero value. */
    PGMACCESSORIGIN_INVALID = 0,
    /** IEM is access memory. */
    PGMACCESSORIGIN_IEM,
    /** HM is access memory. */
    PGMACCESSORIGIN_HM,
    /** Some device is access memory. */
    PGMACCESSORIGIN_DEVICE,
    /** Someone debugging is access memory. */
    PGMACCESSORIGIN_DEBUGGER,
    /** SELM is access memory. */
    PGMACCESSORIGIN_SELM,
    /** FTM is access memory. */
    PGMACCESSORIGIN_FTM,
    /** REM is access memory. */
    PGMACCESSORIGIN_REM,
    /** IOM is access memory. */
    PGMACCESSORIGIN_IOM,
    /** End of valid values. */
    PGMACCESSORIGIN_END,
    /** Type size hack. */
    PGMACCESSORIGIN_32BIT_HACK = 0x7fffffff
} PGMACCESSORIGIN;


/**
 * Physical page access handler kind.
 */
typedef enum PGMPHYSHANDLERKIND
{
    /** MMIO range. Pages are not present, all access is done in interpreter or recompiler. */
    PGMPHYSHANDLERKIND_MMIO = 1,
    /** Handler all write access to a physical page range. */
    PGMPHYSHANDLERKIND_WRITE,
    /** Handler all access to a physical page range. */
    PGMPHYSHANDLERKIND_ALL

} PGMPHYSHANDLERKIND;

/**
 * Guest Access type
 */
typedef enum PGMACCESSTYPE
{
    /** Read access. */
    PGMACCESSTYPE_READ = 1,
    /** Write access. */
    PGMACCESSTYPE_WRITE
} PGMACCESSTYPE;


/** @def PGM_ALL_CB_DECL
 * Macro for declaring a handler callback for all contexts.  The handler
 * callback is static in ring-3, and exported in RC and R0.
 * @sa PGM_ALL_CB2_DECL.
 */
#if defined(IN_RC) || defined(IN_RING0)
# ifdef __cplusplus
#  define PGM_ALL_CB_DECL(type)     extern "C" DECLCALLBACK(DECLEXPORT(type))
# else
#  define PGM_ALL_CB_DECL(type)     DECLCALLBACK(DECLEXPORT(type))
# endif
#else
# define PGM_ALL_CB_DECL(type)      static DECLCALLBACK(type)
#endif

/** @def PGM_ALL_CB2_DECL
 * Macro for declaring a handler callback for all contexts.  The handler
 * callback is hidden in ring-3, and exported in RC and R0.
 * @sa PGM_ALL_CB2_DECL.
 */
#if defined(IN_RC) || defined(IN_RING0)
# ifdef __cplusplus
#  define PGM_ALL_CB2_DECL(type)    extern "C" DECLCALLBACK(DECLEXPORT(type))
# else
#  define PGM_ALL_CB2_DECL(type)    DECLCALLBACK(DECLEXPORT(type))
# endif
#else
# define PGM_ALL_CB2_DECL(type)     DECLCALLBACK(DECLHIDDEN(type))
#endif

/** @def PGM_ALL_CB2_PROTO
 * Macro for declaring a handler callback for all contexts.  The handler
 * callback is hidden in ring-3, and exported in RC and R0.
 * @param   fnType      The callback function type.
 * @sa PGM_ALL_CB2_DECL.
 */
#if defined(IN_RC) || defined(IN_RING0)
# ifdef __cplusplus
#  define PGM_ALL_CB2_PROTO(fnType)    extern "C" DECLEXPORT(fnType)
# else
#  define PGM_ALL_CB2_PROTO(fnType)    DECLEXPORT(fnType)
# endif
#else
# define PGM_ALL_CB2_PROTO(fnType)     DECLHIDDEN(fnType)
#endif


/**
 * \#PF Handler callback for physical access handler ranges in RC and R0.
 *
 * @returns Strict VBox status code (appropriate for ring-0 and raw-mode).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   uErrorCode  CPU Error code.
 * @param   pRegFrame   Trap register frame.
 *                      NULL on DMA and other non CPU access.
 * @param   pvFault     The fault address (cr2).
 * @param   GCPhysFault The GC physical address corresponding to pvFault.
 * @param   pvUser      User argument.
 * @thread  EMT(pVCpu)
 */
typedef DECLCALLBACK(VBOXSTRICTRC) FNPGMRZPHYSPFHANDLER(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                        RTGCPTR pvFault, RTGCPHYS GCPhysFault, void *pvUser);
/** Pointer to PGM access callback. */
typedef FNPGMRZPHYSPFHANDLER *PFNPGMRZPHYSPFHANDLER;


/**
 * Access handler callback for physical access handler ranges.
 *
 * The handler can not raise any faults, it's mainly for monitoring write access
 * to certain pages (like MMIO).
 *
 * @returns Strict VBox status code in ring-0 and raw-mode context, in ring-3
 *          the only supported informational status code is
 *          VINF_PGM_HANDLER_DO_DEFAULT.
 * @retval  VINF_SUCCESS if the handler have carried out the operation.
 * @retval  VINF_PGM_HANDLER_DO_DEFAULT if the caller should carry out the
 *          access operation.
 * @retval  VINF_EM_XXX in ring-0 and raw-mode context.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   GCPhys          The physical address the guest is writing to.
 * @param   pvPhys          The HC mapping of that address.
 * @param   pvBuf           What the guest is reading/writing.
 * @param   cbBuf           How much it's reading/writing.
 * @param   enmAccessType   The access type.
 * @param   enmOrigin       The origin of this call.
 * @param   pvUser          User argument.
 * @thread  EMT(pVCpu)
 */
typedef DECLCALLBACK(VBOXSTRICTRC) FNPGMPHYSHANDLER(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void *pvPhys, void *pvBuf, size_t cbBuf,
                                                    PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser);
/** Pointer to PGM access callback. */
typedef FNPGMPHYSHANDLER *PFNPGMPHYSHANDLER;


/**
 * Virtual access handler type.
 */
typedef enum PGMVIRTHANDLERKIND
{
    /** Write access handled. */
    PGMVIRTHANDLERKIND_WRITE = 1,
    /** All access handled. */
    PGMVIRTHANDLERKIND_ALL,
    /** Hypervisor write access handled.
     * This is used to catch the guest trying to write to LDT, TSS and any other
     * system structure which the brain dead intel guys let unprivilegde code find. */
    PGMVIRTHANDLERKIND_HYPERVISOR
} PGMVIRTHANDLERKIND;

/**
 * \#PF handler callback for virtual access handler ranges, RC.
 *
 * Important to realize that a physical page in a range can have aliases, and
 * for ALL and WRITE handlers these will also trigger.
 *
 * @returns Strict VBox status code (appropriate for raw-mode).
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   uErrorCode      CPU Error code (X86_TRAP_PF_XXX).
 * @param   pRegFrame       Trap register frame.
 * @param   pvFault         The fault address (cr2).
 * @param   pvRange         The base address of the handled virtual range.
 * @param   offRange        The offset of the access into this range.
 *                          (If it's a EIP range this is the EIP, if not it's pvFault.)
 * @param   pvUser          User argument.
 * @thread  EMT(pVCpu)
 */
typedef DECLCALLBACK(VBOXSTRICTRC) FNPGMRCVIRTPFHANDLER(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame,
                                                        RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange, void *pvUser);
/** Pointer to PGM access callback. */
typedef FNPGMRCVIRTPFHANDLER *PFNPGMRCVIRTPFHANDLER;

/**
 * Access handler callback for virtual access handler ranges.
 *
 * Important to realize that a physical page in a range can have aliases, and
 * for ALL and WRITE handlers these will also trigger.
 *
 * @returns VINF_SUCCESS if the handler have carried out the operation.
 * @returns VINF_PGM_HANDLER_DO_DEFAULT if the caller should carry out the access operation.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   GCPtr           The virtual address the guest is writing to.  This
 *                          is the registered address corresponding to the
 *                          access, so no aliasing trouble here.
 * @param   pvPtr           The HC mapping of that address.
 * @param   pvBuf           What the guest is reading/writing.
 * @param   cbBuf           How much it's reading/writing.
 * @param   enmAccessType   The access type.
 * @param   enmOrigin       Who is calling.
 * @param   pvUser          User argument.
 * @thread  EMT(pVCpu)
 */
typedef DECLCALLBACK(VBOXSTRICTRC) FNPGMVIRTHANDLER(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvPtr, void *pvBuf, size_t cbBuf,
                                                    PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser);
/** Pointer to PGM access callback. */
typedef FNPGMVIRTHANDLER *PFNPGMVIRTHANDLER;

/**
 * \#PF Handler callback for invalidation of virtual access handler ranges.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the calling EMT.
 * @param   GCPtr           The virtual address the guest has changed.
 * @param   pvUser          User argument.
 * @thread  EMT(pVCpu)
 *
 * @todo    FNPGMR3VIRTINVALIDATE will not actually be called! It was introduced
 *          in r13179 (1.1) and stopped working with r13806 (PGMPool merge,
 *          v1.2), exactly a month later.
 */
typedef DECLCALLBACK(int) FNPGMR3VIRTINVALIDATE(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, void *pvUser);
/** Pointer to PGM invalidation callback. */
typedef FNPGMR3VIRTINVALIDATE *PFNPGMR3VIRTINVALIDATE;


/**
 * PGMR3PhysEnumDirtyFTPages callback for syncing dirty physical pages
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          GC physical address
 * @param   pRange          HC virtual address of the page(s)
 * @param   cbRange         Size of the dirty range in bytes.
 * @param   pvUser          User argument.
 */
typedef DECLCALLBACK(int) FNPGMENUMDIRTYFTPAGES(PVM pVM, RTGCPHYS GCPhys, uint8_t *pRange, unsigned cbRange, void *pvUser);
/** Pointer to PGMR3PhysEnumDirtyFTPages callback. */
typedef FNPGMENUMDIRTYFTPAGES *PFNPGMENUMDIRTYFTPAGES;


/**
 * Paging mode.
 *
 * @note    Part of saved state.  Change with extreme care.
 */
typedef enum PGMMODE
{
    /** The usual invalid value. */
    PGMMODE_INVALID = 0,
    /** Real mode. */
    PGMMODE_REAL,
    /** Protected mode, no paging. */
    PGMMODE_PROTECTED,
    /** 32-bit paging. */
    PGMMODE_32_BIT,
    /** PAE paging. */
    PGMMODE_PAE,
    /** PAE paging with NX enabled. */
    PGMMODE_PAE_NX,
    /** 64-bit AMD paging (long mode). */
    PGMMODE_AMD64,
    /** 64-bit AMD paging (long mode) with NX enabled. */
    PGMMODE_AMD64_NX,
    /** Nested paging mode (shadow only; guest physical to host physical). */
    PGMMODE_NESTED,
    /** Extended paging (Intel) mode. */
    PGMMODE_EPT,
    /** The max number of modes */
    PGMMODE_MAX,
    /** 32bit hackishness. */
    PGMMODE_32BIT_HACK = 0x7fffffff
} PGMMODE;

/** Macro for checking if the guest is using paging.
 * @param enmMode   PGMMODE_*.
 * @remark  ASSUMES certain order of the PGMMODE_* values.
 */
#define PGMMODE_WITH_PAGING(enmMode) ((enmMode) >= PGMMODE_32_BIT)

/** Macro for checking if it's one of the long mode modes.
 * @param enmMode   PGMMODE_*.
 */
#define PGMMODE_IS_LONG_MODE(enmMode) ((enmMode) == PGMMODE_AMD64_NX || (enmMode) == PGMMODE_AMD64)

/**
 * Is the ROM mapped (true) or is the shadow RAM mapped (false).
 *
 * @returns boolean.
 * @param   enmProt     The PGMROMPROT value, must be valid.
 */
#define PGMROMPROT_IS_ROM(enmProt) \
    (    (enmProt) == PGMROMPROT_READ_ROM_WRITE_IGNORE \
      || (enmProt) == PGMROMPROT_READ_ROM_WRITE_RAM )


VMMDECL(bool)           PGMIsLockOwner(PVM pVM);

VMMDECL(int)            PGMRegisterStringFormatTypes(void);
VMMDECL(void)           PGMDeregisterStringFormatTypes(void);
VMMDECL(RTHCPHYS)       PGMGetHyperCR3(PVMCPU pVCpu);
VMMDECL(RTHCPHYS)       PGMGetNestedCR3(PVMCPU pVCpu, PGMMODE enmShadowMode);
VMMDECL(RTHCPHYS)       PGMGetInterHCCR3(PVM pVM);
VMMDECL(RTHCPHYS)       PGMGetInterRCCR3(PVM pVM, PVMCPU pVCpu);
VMMDECL(RTHCPHYS)       PGMGetInter32BitCR3(PVM pVM);
VMMDECL(RTHCPHYS)       PGMGetInterPaeCR3(PVM pVM);
VMMDECL(RTHCPHYS)       PGMGetInterAmd64CR3(PVM pVM);
VMMDECL(int)            PGMTrap0eHandler(PVMCPU pVCpu, RTGCUINT uErr, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault);
VMMDECL(int)            PGMPrefetchPage(PVMCPU pVCpu, RTGCPTR GCPtrPage);
VMMDECL(int)            PGMVerifyAccess(PVMCPU pVCpu, RTGCPTR Addr, uint32_t cbSize, uint32_t fAccess);
VMMDECL(int)            PGMIsValidAccess(PVMCPU pVCpu, RTGCPTR Addr, uint32_t cbSize, uint32_t fAccess);
VMMDECL(VBOXSTRICTRC)   PGMInterpretInstruction(PVM pVM, PVMCPU pVCpu, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault);
VMMDECL(int)            PGMMap(PVM pVM, RTGCPTR GCPtr, RTHCPHYS HCPhys, uint32_t cbPages, unsigned fFlags);
VMMDECL(int)            PGMMapGetPage(PVM pVM, RTGCPTR GCPtr, uint64_t *pfFlags, PRTHCPHYS pHCPhys);
VMMDECL(int)            PGMMapSetPage(PVM pVM, RTGCPTR GCPtr, uint64_t cb, uint64_t fFlags);
VMMDECL(int)            PGMMapModifyPage(PVM pVM, RTGCPTR GCPtr, size_t cb, uint64_t fFlags, uint64_t fMask);
#ifndef IN_RING0
VMMDECL(bool)           PGMMapHasConflicts(PVM pVM);
#endif
#ifdef VBOX_STRICT
VMMDECL(void)           PGMMapCheck(PVM pVM);
#endif
VMMDECL(int)            PGMShwGetPage(PVMCPU pVCpu, RTGCPTR GCPtr, uint64_t *pfFlags, PRTHCPHYS pHCPhys);
VMMDECL(int)            PGMShwMakePageReadonly(PVMCPU pVCpu, RTGCPTR GCPtr, uint32_t fFlags);
VMMDECL(int)            PGMShwMakePageWritable(PVMCPU pVCpu, RTGCPTR GCPtr, uint32_t fFlags);
VMMDECL(int)            PGMShwMakePageNotPresent(PVMCPU pVCpu, RTGCPTR GCPtr, uint32_t fFlags);
/** @name Flags for PGMShwMakePageReadonly, PGMShwMakePageWritable and
 *        PGMShwMakePageNotPresent
 * @{ */
/** The call is from an access handler for dealing with the a faulting write
 * operation.  The virtual address is within the same page. */
#define PGM_MK_PG_IS_WRITE_FAULT     RT_BIT(0)
/** The page is an MMIO2. */
#define PGM_MK_PG_IS_MMIO2           RT_BIT(1)
/** @}*/
VMMDECL(int)        PGMGstGetPage(PVMCPU pVCpu, RTGCPTR GCPtr, uint64_t *pfFlags, PRTGCPHYS pGCPhys);
VMMDECL(bool)       PGMGstIsPagePresent(PVMCPU pVCpu, RTGCPTR GCPtr);
VMMDECL(int)        PGMGstSetPage(PVMCPU pVCpu, RTGCPTR GCPtr, size_t cb, uint64_t fFlags);
VMMDECL(int)        PGMGstModifyPage(PVMCPU pVCpu, RTGCPTR GCPtr, size_t cb, uint64_t fFlags, uint64_t fMask);
VMM_INT_DECL(int)   PGMGstGetPaePdpes(PVMCPU pVCpu, PX86PDPE paPdpes);
VMM_INT_DECL(void)  PGMGstUpdatePaePdpes(PVMCPU pVCpu, PCX86PDPE paPdpes);

VMMDECL(int)        PGMInvalidatePage(PVMCPU pVCpu, RTGCPTR GCPtrPage);
VMMDECL(int)        PGMFlushTLB(PVMCPU pVCpu, uint64_t cr3, bool fGlobal);
VMMDECL(int)        PGMSyncCR3(PVMCPU pVCpu, uint64_t cr0, uint64_t cr3, uint64_t cr4, bool fGlobal);
VMMDECL(int)        PGMUpdateCR3(PVMCPU pVCpu, uint64_t cr3);
VMMDECL(int)        PGMChangeMode(PVMCPU pVCpu, uint64_t cr0, uint64_t cr4, uint64_t efer);
VMMDECL(void)       PGMCr0WpEnabled(PVMCPU pVCpu);
VMMDECL(PGMMODE)    PGMGetGuestMode(PVMCPU pVCpu);
VMMDECL(PGMMODE)    PGMGetShadowMode(PVMCPU pVCpu);
VMMDECL(PGMMODE)    PGMGetHostMode(PVM pVM);
VMMDECL(const char *) PGMGetModeName(PGMMODE enmMode);
VMM_INT_DECL(void)  PGMNotifyNxeChanged(PVMCPU pVCpu, bool fNxe);
VMMDECL(bool)       PGMHasDirtyPages(PVM pVM);

/** PGM physical access handler type registration handle (heap offset, valid
 * cross contexts without needing fixing up).  Callbacks and handler type is
 * associated with this and it is shared by all handler registrations. */
typedef uint32_t PGMPHYSHANDLERTYPE;
/** Pointer to a PGM physical handler type registration handle. */
typedef PGMPHYSHANDLERTYPE *PPGMPHYSHANDLERTYPE;
/** NIL value for PGM physical access handler type handle. */
#define NIL_PGMPHYSHANDLERTYPE  UINT32_MAX
VMMDECL(uint32_t)   PGMHandlerPhysicalTypeRelease(PVM pVM, PGMPHYSHANDLERTYPE hType);
VMMDECL(uint32_t)   PGMHandlerPhysicalTypeRetain(PVM pVM, PGMPHYSHANDLERTYPE hType);

VMMDECL(int)        PGMHandlerPhysicalRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysLast, PGMPHYSHANDLERTYPE hType,
                                               RTR3PTR pvUserR3, RTR0PTR pvUserR0, RTRCPTR pvUserRC,
                                               R3PTRTYPE(const char *) pszDesc);
VMMDECL(int)        PGMHandlerPhysicalModify(PVM pVM, RTGCPHYS GCPhysCurrent, RTGCPHYS GCPhys, RTGCPHYS GCPhysLast);
VMMDECL(int)        PGMHandlerPhysicalDeregister(PVM pVM, RTGCPHYS GCPhys);
VMMDECL(int)        PGMHandlerPhysicalChangeUserArgs(PVM pVM, RTGCPHYS GCPhys, RTR3PTR pvUserR3, RTR0PTR pvUserR0, RTRCPTR pvUserRC);
VMMDECL(int)        PGMHandlerPhysicalSplit(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysSplit);
VMMDECL(int)        PGMHandlerPhysicalJoin(PVM pVM, RTGCPHYS GCPhys1, RTGCPHYS GCPhys2);
VMMDECL(int)        PGMHandlerPhysicalPageTempOff(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysPage);
VMMDECL(int)        PGMHandlerPhysicalPageAlias(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysPage, RTGCPHYS GCPhysPageRemap);
VMMDECL(int)        PGMHandlerPhysicalPageAliasHC(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysPage, RTHCPHYS HCPhysPageRemap);
VMMDECL(int)        PGMHandlerPhysicalReset(PVM pVM, RTGCPHYS GCPhys);
VMMDECL(bool)       PGMHandlerPhysicalIsRegistered(PVM pVM, RTGCPHYS GCPhys);

/** PGM virtual access handler type registration handle (heap offset, valid
 * cross contexts without needing fixing up).  Callbacks and handler type is
 * associated with this and it is shared by all handler registrations. */
typedef uint32_t PGMVIRTHANDLERTYPE;
/** Pointer to a PGM virtual handler type registration handle. */
typedef PGMVIRTHANDLERTYPE *PPGMVIRTHANDLERTYPE;
/** NIL value for PGM virtual access handler type handle. */
#define NIL_PGMVIRTHANDLERTYPE  UINT32_MAX
#ifdef VBOX_WITH_RAW_MODE
VMM_INT_DECL(uint32_t) PGMHandlerVirtualTypeRelease(PVM pVM, PGMVIRTHANDLERTYPE hType);
VMM_INT_DECL(uint32_t) PGMHandlerVirtualTypeRetain(PVM pVM, PGMVIRTHANDLERTYPE hType);
VMM_INT_DECL(bool)     PGMHandlerVirtualIsRegistered(PVM pVM, RTGCPTR GCPtr);
#endif


/**
 * Page type.
 *
 * @remarks This enum has to fit in a 3-bit field (see PGMPAGE::u3Type).
 * @remarks This is used in the saved state, so changes to it requires bumping
 *          the saved state version.
 * @todo    So, convert to \#defines!
 */
typedef enum PGMPAGETYPE
{
    /** The usual invalid zero entry. */
    PGMPAGETYPE_INVALID = 0,
    /** RAM page. (RWX) */
    PGMPAGETYPE_RAM,
    /** MMIO2 page. (RWX) */
    PGMPAGETYPE_MMIO2,
    /** MMIO2 page aliased over an MMIO page. (RWX)
     * See PGMHandlerPhysicalPageAlias(). */
    PGMPAGETYPE_MMIO2_ALIAS_MMIO,
    /** Special page aliased over an MMIO page. (RWX)
     * See PGMHandlerPhysicalPageAliasHC(), but this is generally only used for
     * VT-x's APIC access page at the moment.  Treated as MMIO by everyone except
     * the shadow paging code. */
    PGMPAGETYPE_SPECIAL_ALIAS_MMIO,
    /** Shadowed ROM. (RWX) */
    PGMPAGETYPE_ROM_SHADOW,
    /** ROM page. (R-X) */
    PGMPAGETYPE_ROM,
    /** MMIO page. (---) */
    PGMPAGETYPE_MMIO,
    /** End of valid entries. */
    PGMPAGETYPE_END
} PGMPAGETYPE;
AssertCompile(PGMPAGETYPE_END == 8);

VMM_INT_DECL(PGMPAGETYPE) PGMPhysGetPageType(PVM pVM, RTGCPHYS GCPhys);

VMM_INT_DECL(int)   PGMPhysGCPhys2HCPhys(PVM pVM, RTGCPHYS GCPhys, PRTHCPHYS pHCPhys);
VMM_INT_DECL(int)   PGMPhysGCPtr2HCPhys(PVMCPU pVCpu, RTGCPTR GCPtr, PRTHCPHYS pHCPhys);
VMM_INT_DECL(int)   PGMPhysGCPhys2CCPtr(PVM pVM, RTGCPHYS GCPhys, void **ppv, PPGMPAGEMAPLOCK pLock);
VMM_INT_DECL(int)   PGMPhysGCPhys2CCPtrReadOnly(PVM pVM, RTGCPHYS GCPhys, void const **ppv, PPGMPAGEMAPLOCK pLock);
VMM_INT_DECL(int)   PGMPhysGCPtr2CCPtr(PVMCPU pVCpu, RTGCPTR GCPtr, void **ppv, PPGMPAGEMAPLOCK pLock);
VMM_INT_DECL(int)   PGMPhysGCPtr2CCPtrReadOnly(PVMCPU pVCpu, RTGCPTR GCPtr, void const **ppv, PPGMPAGEMAPLOCK pLock);

VMMDECL(bool)       PGMPhysIsA20Enabled(PVMCPU pVCpu);
VMMDECL(bool)       PGMPhysIsGCPhysValid(PVM pVM, RTGCPHYS GCPhys);
VMMDECL(bool)       PGMPhysIsGCPhysNormal(PVM pVM, RTGCPHYS GCPhys);
VMMDECL(int)        PGMPhysGCPtr2GCPhys(PVMCPU pVCpu, RTGCPTR GCPtr, PRTGCPHYS pGCPhys);
VMMDECL(void)       PGMPhysReleasePageMappingLock(PVM pVM, PPGMPAGEMAPLOCK pLock);

/** @def PGM_PHYS_RW_IS_SUCCESS
 * Check whether a PGMPhysRead, PGMPhysWrite, PGMPhysReadGCPtr or
 * PGMPhysWriteGCPtr call completed the given task.
 *
 * @returns true if completed, false if not.
 * @param   a_rcStrict      The status code.
 * @sa      IOM_SUCCESS
 */
#ifdef IN_RING3
# define PGM_PHYS_RW_IS_SUCCESS(a_rcStrict) \
    (   (a_rcStrict) == VINF_SUCCESS \
     || (a_rcStrict) == VINF_EM_DBG_STOP \
     || (a_rcStrict) == VINF_EM_DBG_EVENT \
     || (a_rcStrict) == VINF_EM_DBG_BREAKPOINT \
    )
#elif defined(IN_RING0)
# define PGM_PHYS_RW_IS_SUCCESS(a_rcStrict) \
    (   (a_rcStrict) == VINF_SUCCESS \
     || (a_rcStrict) == VINF_IOM_R3_MMIO_COMMIT_WRITE \
     || (a_rcStrict) == VINF_EM_OFF \
     || (a_rcStrict) == VINF_EM_SUSPEND \
     || (a_rcStrict) == VINF_EM_RESET \
     || (a_rcStrict) == VINF_EM_HALT \
     || (a_rcStrict) == VINF_EM_DBG_STOP \
     || (a_rcStrict) == VINF_EM_DBG_EVENT \
     || (a_rcStrict) == VINF_EM_DBG_BREAKPOINT \
    )
#elif defined(IN_RC)
# define PGM_PHYS_RW_IS_SUCCESS(a_rcStrict) \
    (   (a_rcStrict) == VINF_SUCCESS \
     || (a_rcStrict) == VINF_IOM_R3_MMIO_COMMIT_WRITE \
     || (a_rcStrict) == VINF_EM_OFF \
     || (a_rcStrict) == VINF_EM_SUSPEND \
     || (a_rcStrict) == VINF_EM_RESET \
     || (a_rcStrict) == VINF_EM_HALT \
     || (a_rcStrict) == VINF_SELM_SYNC_GDT \
     || (a_rcStrict) == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT \
     || (a_rcStrict) == VINF_EM_DBG_STOP \
     || (a_rcStrict) == VINF_EM_DBG_EVENT \
     || (a_rcStrict) == VINF_EM_DBG_BREAKPOINT \
    )
#endif
/** @def PGM_PHYS_RW_DO_UPDATE_STRICT_RC
 * Updates the return code with a new result.
 *
 * Both status codes must be successes according to PGM_PHYS_RW_IS_SUCCESS.
 *
 * @param   a_rcStrict      The current return code, to be updated.
 * @param   a_rcStrict2     The new return code to merge in.
 */
#ifdef IN_RING3
# define PGM_PHYS_RW_DO_UPDATE_STRICT_RC(a_rcStrict, a_rcStrict2) \
    do { \
        Assert(rcStrict == VINF_SUCCESS); \
        Assert(rcStrict2 == VINF_SUCCESS); \
    } while (0)
#elif defined(IN_RING0)
# define PGM_PHYS_RW_DO_UPDATE_STRICT_RC(a_rcStrict, a_rcStrict2) \
    do { \
        Assert(PGM_PHYS_RW_IS_SUCCESS(rcStrict)); \
        Assert(PGM_PHYS_RW_IS_SUCCESS(rcStrict2)); \
        AssertCompile(VINF_IOM_R3_MMIO_COMMIT_WRITE > VINF_EM_LAST); \
        if ((a_rcStrict2) == VINF_SUCCESS || (a_rcStrict) == (a_rcStrict2)) \
        { /* likely */ } \
        else if (   (a_rcStrict) == VINF_SUCCESS \
                 || (a_rcStrict) > (a_rcStrict2)) \
            (a_rcStrict) = (a_rcStrict2); \
    } while (0)
#elif defined(IN_RC)
# define PGM_PHYS_RW_DO_UPDATE_STRICT_RC(a_rcStrict, a_rcStrict2) \
    do { \
        Assert(PGM_PHYS_RW_IS_SUCCESS(rcStrict)); \
        Assert(PGM_PHYS_RW_IS_SUCCESS(rcStrict2)); \
        AssertCompile(VINF_SELM_SYNC_GDT > VINF_EM_LAST); \
        AssertCompile(VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT > VINF_EM_LAST); \
        AssertCompile(VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT < VINF_SELM_SYNC_GDT); \
        AssertCompile(VINF_IOM_R3_MMIO_COMMIT_WRITE > VINF_EM_LAST); \
        AssertCompile(VINF_IOM_R3_MMIO_COMMIT_WRITE > VINF_SELM_SYNC_GDT); \
        AssertCompile(VINF_IOM_R3_MMIO_COMMIT_WRITE > VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT); \
        if ((a_rcStrict2) == VINF_SUCCESS || (a_rcStrict) == (a_rcStrict2)) \
        { /* likely */ } \
        else if ((a_rcStrict) == VINF_SUCCESS) \
            (a_rcStrict) = (a_rcStrict2); \
        else if (   (   (a_rcStrict) > (a_rcStrict2) \
                     && (   (a_rcStrict2) <= VINF_EM_RESET  \
                         || (a_rcStrict) != VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT) ) \
                 || (   (a_rcStrict2) == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT \
                     && (a_rcStrict) > VINF_EM_RESET) ) \
            (a_rcStrict) = (a_rcStrict2); \
    } while (0)
#endif

VMMDECL(VBOXSTRICTRC) PGMPhysRead(PVM pVM, RTGCPHYS GCPhys, void *pvBuf, size_t cbRead, PGMACCESSORIGIN enmOrigin);
VMMDECL(VBOXSTRICTRC) PGMPhysWrite(PVM pVM, RTGCPHYS GCPhys, const void *pvBuf, size_t cbWrite, PGMACCESSORIGIN enmOrigin);
VMMDECL(VBOXSTRICTRC) PGMPhysReadGCPtr(PVMCPU pVCpu, void *pvDst, RTGCPTR GCPtrSrc, size_t cb, PGMACCESSORIGIN enmOrigin);
VMMDECL(VBOXSTRICTRC) PGMPhysWriteGCPtr(PVMCPU pVCpu, RTGCPTR GCPtrDst, const void *pvSrc, size_t cb, PGMACCESSORIGIN enmOrigin);

VMMDECL(int)        PGMPhysSimpleReadGCPhys(PVM pVM, void *pvDst, RTGCPHYS GCPhysSrc, size_t cb);
VMMDECL(int)        PGMPhysSimpleWriteGCPhys(PVM pVM, RTGCPHYS GCPhysDst, const void *pvSrc, size_t cb);
VMMDECL(int)        PGMPhysSimpleReadGCPtr(PVMCPU pVCpu, void *pvDst, RTGCPTR GCPtrSrc, size_t cb);
VMMDECL(int)        PGMPhysSimpleWriteGCPtr(PVMCPU pVCpu, RTGCPTR GCPtrDst, const void *pvSrc, size_t cb);
VMMDECL(int)        PGMPhysSimpleDirtyWriteGCPtr(PVMCPU pVCpu, RTGCPTR GCPtrDst, const void *pvSrc, size_t cb);
VMMDECL(int)        PGMPhysInterpretedRead(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, void *pvDst, RTGCPTR GCPtrSrc, size_t cb);
VMMDECL(int)        PGMPhysInterpretedReadNoHandlers(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, void *pvDst, RTGCUINTPTR GCPtrSrc, size_t cb, bool fRaiseTrap);
VMMDECL(int)        PGMPhysInterpretedWriteNoHandlers(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, RTGCPTR GCPtrDst, void const *pvSrc, size_t cb, bool fRaiseTrap);

VMM_INT_DECL(int)   PGMPhysIemGCPhys2Ptr(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, bool fWritable, bool fByPassHandlers, void **ppv, PPGMPAGEMAPLOCK pLock);
VMM_INT_DECL(int)   PGMPhysIemQueryAccess(PVM pVM, RTGCPHYS GCPhys, bool fWritable, bool fByPassHandlers);
VMM_INT_DECL(int)   PGMPhysIemGCPhys2PtrNoLock(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, uint64_t const volatile *puTlbPhysRev,
#if defined(IN_RC) || defined(VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0)
                                               R3PTRTYPE(uint8_t *) *ppb,
#else
                                               R3R0PTRTYPE(uint8_t *) *ppb,
#endif
                                               uint64_t *pfTlb);
/** @name Flags returned by PGMPhysIemGCPhys2PtrNoLock
 * @{ */
#define PGMIEMGCPHYS2PTR_F_NO_WRITE     RT_BIT_32(3)    /**< Not writable (IEMTLBE_F_PG_NO_WRITE). */
#define PGMIEMGCPHYS2PTR_F_NO_READ      RT_BIT_32(4)    /**< Not readable (IEMTLBE_F_PG_NO_READ). */
#define PGMIEMGCPHYS2PTR_F_NO_MAPPINGR3 RT_BIT_32(7)    /**< No ring-3 mapping (IEMTLBE_F_NO_MAPPINGR3). */
/** @} */

#ifdef VBOX_STRICT
VMMDECL(unsigned)   PGMAssertHandlerAndFlagsInSync(PVM pVM);
VMMDECL(unsigned)   PGMAssertNoMappingConflicts(PVM pVM);
VMMDECL(unsigned)   PGMAssertCR3(PVM pVM, PVMCPU pVCpu, uint64_t cr3, uint64_t cr4);
#endif /* VBOX_STRICT */

#if defined(IN_RC) || defined(VBOX_WITH_2X_4GB_ADDR_SPACE)
VMMDECL(void)       PGMRZDynMapStartAutoSet(PVMCPU pVCpu);
VMMDECL(void)       PGMRZDynMapReleaseAutoSet(PVMCPU pVCpu);
VMMDECL(void)       PGMRZDynMapFlushAutoSet(PVMCPU pVCpu);
VMMDECL(uint32_t)   PGMRZDynMapPushAutoSubset(PVMCPU pVCpu);
VMMDECL(void)       PGMRZDynMapPopAutoSubset(PVMCPU pVCpu, uint32_t iPrevSubset);
#endif

VMMDECL(int)        PGMSetLargePageUsage(PVM pVM, bool fUseLargePages);

/**
 * Query large page usage state
 *
 * @returns 0 - disabled, 1 - enabled
 * @param   pVM         The cross context VM structure.
 */
#define PGMIsUsingLargePages(pVM)   ((pVM)->fUseLargePages)


#ifdef IN_RC
/** @defgroup grp_pgm_gc  The PGM Guest Context API
 * @{
 */
VMMRCDECL(int)      PGMRCDynMapInit(PVM pVM);
/** @} */
#endif /* IN_RC */


#ifdef IN_RING0
/** @defgroup grp_pgm_r0  The PGM Host Context Ring-0 API
 * @{
 */
VMMR0_INT_DECL(int) PGMR0PhysAllocateHandyPages(PGVM pGVM, PVM pVM, VMCPUID idCpu);
VMMR0_INT_DECL(int) PGMR0PhysFlushHandyPages(PGVM pGVM, PVM pVM, VMCPUID idCpu);
VMMR0_INT_DECL(int) PGMR0PhysAllocateLargeHandyPage(PGVM pGVM, PVM pVM, VMCPUID idCpu);
VMMR0_INT_DECL(int) PGMR0PhysSetupIoMmu(PGVM pGVM, PVM pVM);
VMMR0DECL(int)      PGMR0SharedModuleCheck(PVM pVM, PGVM pGVM, VMCPUID idCpu, PGMMSHAREDMODULE pModule, PCRTGCPTR64 paRegionsGCPtrs);
VMMR0DECL(int)      PGMR0Trap0eHandlerNestedPaging(PVM pVM, PVMCPU pVCpu, PGMMODE enmShwPagingMode, RTGCUINT uErr, PCPUMCTXCORE pRegFrame, RTGCPHYS pvFault);
VMMR0DECL(VBOXSTRICTRC) PGMR0Trap0eHandlerNPMisconfig(PVM pVM, PVMCPU pVCpu, PGMMODE enmShwPagingMode, PCPUMCTXCORE pRegFrame, RTGCPHYS GCPhysFault, uint32_t uErr);
# ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
VMMR0DECL(int)      PGMR0DynMapInit(void);
VMMR0DECL(void)     PGMR0DynMapTerm(void);
VMMR0DECL(int)      PGMR0DynMapInitVM(PVM pVM);
VMMR0DECL(void)     PGMR0DynMapTermVM(PVM pVM);
VMMR0DECL(int)      PGMR0DynMapAssertIntegrity(void);
VMMR0DECL(bool)     PGMR0DynMapStartOrMigrateAutoSet(PVMCPU pVCpu);
VMMR0DECL(void)     PGMR0DynMapMigrateAutoSet(PVMCPU pVCpu);
# endif
/** @} */
#endif /* IN_RING0 */



#ifdef IN_RING3
/** @defgroup grp_pgm_r3  The PGM Host Context Ring-3 API
 * @{
 */
VMMR3DECL(int)      PGMR3Init(PVM pVM);
VMMR3DECL(int)      PGMR3InitDynMap(PVM pVM);
VMMR3DECL(int)      PGMR3InitFinalize(PVM pVM);
VMMR3_INT_DECL(int) PGMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat);
VMMR3DECL(void)     PGMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3DECL(void)     PGMR3ResetCpu(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(void)    PGMR3Reset(PVM pVM);
VMMR3_INT_DECL(void)    PGMR3ResetNoMorePhysWritesFlag(PVM pVM);
VMMR3_INT_DECL(void)    PGMR3MemSetup(PVM pVM, bool fReset);
VMMR3DECL(int)      PGMR3Term(PVM pVM);
VMMR3DECL(int)      PGMR3LockCall(PVM pVM);
VMMR3DECL(int)      PGMR3ChangeMode(PVM pVM, PVMCPU pVCpu, PGMMODE enmGuestMode);

VMMR3DECL(int)      PGMR3PhysRegisterRam(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, const char *pszDesc);
VMMR3DECL(int)      PGMR3PhysChangeMemBalloon(PVM pVM, bool fInflate, unsigned cPages, RTGCPHYS *paPhysPage);
VMMR3DECL(int)      PGMR3PhysWriteProtectRAM(PVM pVM);
VMMR3DECL(int)      PGMR3PhysEnumDirtyFTPages(PVM pVM, PFNPGMENUMDIRTYFTPAGES pfnEnum, void *pvUser);
VMMR3DECL(uint32_t) PGMR3PhysGetRamRangeCount(PVM pVM);
VMMR3DECL(int)      PGMR3PhysGetRange(PVM pVM, uint32_t iRange, PRTGCPHYS pGCPhysStart, PRTGCPHYS pGCPhysLast,
                                      const char **ppszDesc, bool *pfIsMmio);
VMMR3DECL(int)      PGMR3QueryMemoryStats(PUVM pUVM, uint64_t *pcbTotalMem, uint64_t *pcbPrivateMem, uint64_t *pcbSharedMem, uint64_t *pcbZeroMem);
VMMR3DECL(int)      PGMR3QueryGlobalMemoryStats(PUVM pUVM, uint64_t *pcbAllocMem, uint64_t *pcbFreeMem, uint64_t *pcbBallonedMem, uint64_t *pcbSharedMem);

VMMR3DECL(int)      PGMR3PhysMMIORegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, PGMPHYSHANDLERTYPE hType,
                                          RTR3PTR pvUserR3, RTR0PTR pvUserR0, RTRCPTR pvUserRC, const char *pszDesc);
VMMR3DECL(int)      PGMR3PhysMMIODeregister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb);
VMMR3DECL(int)      PGMR3PhysMMIO2Register(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS cb, uint32_t fFlags, void **ppv, const char *pszDesc);
VMMR3DECL(int)      PGMR3PhysMMIOExPreRegister(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS cbRegion, PGMPHYSHANDLERTYPE hType,
                                               RTR3PTR pvUserR3, RTR0PTR pvUserR0, RTRCPTR pvUserRC, const char *pszDesc);
VMMR3DECL(int)      PGMR3PhysMMIOExDeregister(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion);
VMMR3DECL(int)      PGMR3PhysMMIOExMap(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS GCPhys);
VMMR3DECL(int)      PGMR3PhysMMIOExUnmap(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS GCPhys);
VMMR3_INT_DECL(int) PGMR3PhysMMIOExReduce(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS cbRegion);
VMMR3DECL(bool)     PGMR3PhysMMIOExIsBase(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhys);
VMMR3_INT_DECL(int) PGMR3PhysMMIO2GetHCPhys(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS off, PRTHCPHYS pHCPhys);
VMMR3_INT_DECL(int) PGMR3PhysMMIO2MapKernel(PVM pVM, PPDMDEVINS pDevIns, uint32_t iSubDev, uint32_t iRegion, RTGCPHYS off, RTGCPHYS cb, const char *pszDesc, PRTR0PTR pR0Ptr);

/** @name PGMR3PhysRegisterRom flags.
 * @{ */
/** Inidicates that ROM shadowing should be enabled. */
#define PGMPHYS_ROM_FLAGS_SHADOWED          RT_BIT_32(0)
/** Indicates that what pvBinary points to won't go away
 * and can be used for strictness checks. */
#define PGMPHYS_ROM_FLAGS_PERMANENT_BINARY  RT_BIT_32(1)
/** @} */

VMMR3DECL(int)      PGMR3PhysRomRegister(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhys, RTGCPHYS cb,
                                         const void *pvBinary, uint32_t cbBinary, uint32_t fFlags, const char *pszDesc);
VMMR3DECL(int)      PGMR3PhysRomProtect(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, PGMROMPROT enmProt);
VMMR3DECL(int)      PGMR3PhysRegister(PVM pVM, void *pvRam, RTGCPHYS GCPhys, size_t cb, unsigned fFlags, const SUPPAGE *paPages, const char *pszDesc);
VMMDECL(void)       PGMR3PhysSetA20(PVMCPU pVCpu, bool fEnable);
/** @name PGMR3MapPT flags.
 * @{ */
/** The mapping may be unmapped later. The default is permanent mappings. */
#define PGMR3MAPPT_FLAGS_UNMAPPABLE     RT_BIT(0)
/** @} */
VMMR3DECL(int)      PGMR3MapPT(PVM pVM, RTGCPTR GCPtr, uint32_t cb, uint32_t fFlags, PFNPGMRELOCATE pfnRelocate, void *pvUser, const char *pszDesc);
VMMR3DECL(int)      PGMR3UnmapPT(PVM pVM, RTGCPTR GCPtr);
VMMR3DECL(int)      PGMR3FinalizeMappings(PVM pVM);
VMMR3DECL(int)      PGMR3MappingsSize(PVM pVM, uint32_t *pcb);
VMMR3DECL(int)      PGMR3MappingsFix(PVM pVM, RTGCPTR GCPtrBase, uint32_t cb);
VMMR3DECL(int)      PGMR3MappingsUnfix(PVM pVM);
VMMR3DECL(bool)     PGMR3MappingsNeedReFixing(PVM pVM);
#if defined(VBOX_WITH_RAW_MODE) || HC_ARCH_BITS == 32 /* (latter for 64-bit guests on 32-bit hosts) */
VMMR3DECL(int)      PGMR3MapIntermediate(PVM pVM, RTUINTPTR Addr, RTHCPHYS HCPhys, unsigned cbPages);
#endif
VMMR3DECL(int)      PGMR3MapRead(PVM pVM, void *pvDst, RTGCPTR GCPtrSrc, size_t cb);

VMMR3_INT_DECL(int) PGMR3HandlerPhysicalTypeRegisterEx(PVM pVM, PGMPHYSHANDLERKIND enmKind,
                                                       PFNPGMPHYSHANDLER pfnHandlerR3,
                                                       R0PTRTYPE(PFNPGMPHYSHANDLER)     pfnHandlerR0,
                                                       R0PTRTYPE(PFNPGMRZPHYSPFHANDLER) pfnPfHandlerR0,
                                                       RCPTRTYPE(PFNPGMPHYSHANDLER)     pfnHandlerRC,
                                                       RCPTRTYPE(PFNPGMRZPHYSPFHANDLER) pfnPfHandlerRC,
                                                       const char *pszDesc, PPGMPHYSHANDLERTYPE phType);
VMMR3DECL(int)      PGMR3HandlerPhysicalTypeRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind,
                                                     R3PTRTYPE(PFNPGMPHYSHANDLER) pfnHandlerR3,
                                                     const char *pszModR0, const char *pszHandlerR0, const char *pszPfHandlerR0,
                                                     const char *pszModRC, const char *pszHandlerRC, const char *pszPfHandlerRC,
                                                     const char *pszDesc,
                                                     PPGMPHYSHANDLERTYPE phType);
#ifdef VBOX_WITH_RAW_MODE
VMMR3_INT_DECL(int) PGMR3HandlerVirtualTypeRegisterEx(PVM pVM, PGMVIRTHANDLERKIND enmKind, bool fRelocUserRC,
                                                      PFNPGMR3VIRTINVALIDATE pfnInvalidateR3,
                                                      PFNPGMVIRTHANDLER pfnHandlerR3,
                                                      RCPTRTYPE(FNPGMVIRTHANDLER) pfnHandlerRC,
                                                      RCPTRTYPE(FNPGMRCVIRTPFHANDLER) pfnPfHandlerRC,
                                                      const char *pszDesc, PPGMVIRTHANDLERTYPE phType);
VMMR3_INT_DECL(int) PGMR3HandlerVirtualTypeRegister(PVM pVM, PGMVIRTHANDLERKIND enmKind, bool fRelocUserRC,
                                                    PFNPGMR3VIRTINVALIDATE pfnInvalidateR3,
                                                    PFNPGMVIRTHANDLER pfnHandlerR3,
                                                    const char *pszHandlerRC, const char *pszPfHandlerRC, const char *pszDesc,
                                                    PPGMVIRTHANDLERTYPE phType);
VMMR3_INT_DECL(int) PGMR3HandlerVirtualRegister(PVM pVM, PVMCPU pVCpu, PGMVIRTHANDLERTYPE hType, RTGCPTR GCPtr,
                                                RTGCPTR GCPtrLast, void *pvUserR3, RTRCPTR pvUserRC, const char *pszDesc);
VMMR3_INT_DECL(int) PGMHandlerVirtualChangeType(PVM pVM, RTGCPTR GCPtr, PGMVIRTHANDLERTYPE hNewType);
VMMR3_INT_DECL(int) PGMHandlerVirtualDeregister(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, bool fHypervisor);
#endif
VMMR3DECL(int)      PGMR3PoolGrow(PVM pVM);

VMMR3DECL(int)      PGMR3PhysTlbGCPhys2Ptr(PVM pVM, RTGCPHYS GCPhys, bool fWritable, void **ppv);
VMMR3DECL(uint8_t)  PGMR3PhysReadU8(PVM pVM, RTGCPHYS GCPhys, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(uint16_t) PGMR3PhysReadU16(PVM pVM, RTGCPHYS GCPhys, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(uint32_t) PGMR3PhysReadU32(PVM pVM, RTGCPHYS GCPhys, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(uint64_t) PGMR3PhysReadU64(PVM pVM, RTGCPHYS GCPhys, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(void)     PGMR3PhysWriteU8(PVM pVM, RTGCPHYS GCPhys, uint8_t Value, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(void)     PGMR3PhysWriteU16(PVM pVM, RTGCPHYS GCPhys, uint16_t Value, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(void)     PGMR3PhysWriteU32(PVM pVM, RTGCPHYS GCPhys, uint32_t Value, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(void)     PGMR3PhysWriteU64(PVM pVM, RTGCPHYS GCPhys, uint64_t Value, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(int)      PGMR3PhysReadExternal(PVM pVM, RTGCPHYS GCPhys, void *pvBuf, size_t cbRead, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(int)      PGMR3PhysWriteExternal(PVM pVM, RTGCPHYS GCPhys, const void *pvBuf, size_t cbWrite, PGMACCESSORIGIN enmOrigin);
VMMR3DECL(int)      PGMR3PhysGCPhys2CCPtrExternal(PVM pVM, RTGCPHYS GCPhys, void **ppv, PPGMPAGEMAPLOCK pLock);
VMMR3DECL(int)      PGMR3PhysGCPhys2CCPtrReadOnlyExternal(PVM pVM, RTGCPHYS GCPhys, void const **ppv, PPGMPAGEMAPLOCK pLock);
VMMR3DECL(int)      PGMR3PhysChunkMap(PVM pVM, uint32_t idChunk);
VMMR3DECL(void)     PGMR3PhysChunkInvalidateTLB(PVM pVM);
VMMR3DECL(int)      PGMR3PhysAllocateHandyPages(PVM pVM);
VMMR3DECL(int)      PGMR3PhysAllocateLargeHandyPage(PVM pVM, RTGCPHYS GCPhys);

VMMR3DECL(int)      PGMR3CheckIntegrity(PVM pVM);

VMMR3DECL(int)      PGMR3DbgR3Ptr2GCPhys(PUVM pUVM, RTR3PTR R3Ptr, PRTGCPHYS pGCPhys);
VMMR3DECL(int)      PGMR3DbgR3Ptr2HCPhys(PUVM pUVM, RTR3PTR R3Ptr, PRTHCPHYS pHCPhys);
VMMR3DECL(int)      PGMR3DbgHCPhys2GCPhys(PUVM pUVM, RTHCPHYS HCPhys, PRTGCPHYS pGCPhys);
VMMR3_INT_DECL(int) PGMR3DbgReadGCPhys(PVM pVM, void *pvDst, RTGCPHYS GCPhysSrc, size_t cb, uint32_t fFlags, size_t *pcbRead);
VMMR3_INT_DECL(int) PGMR3DbgWriteGCPhys(PVM pVM, RTGCPHYS GCPhysDst, const void *pvSrc, size_t cb, uint32_t fFlags, size_t *pcbWritten);
VMMR3_INT_DECL(int) PGMR3DbgReadGCPtr(PVM pVM, void *pvDst, RTGCPTR GCPtrSrc, size_t cb, uint32_t fFlags, size_t *pcbRead);
VMMR3_INT_DECL(int) PGMR3DbgWriteGCPtr(PVM pVM, RTGCPTR GCPtrDst, void const *pvSrc, size_t cb, uint32_t fFlags, size_t *pcbWritten);
VMMR3_INT_DECL(int) PGMR3DbgScanPhysical(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cbRange, RTGCPHYS GCPhysAlign, const uint8_t *pabNeedle, size_t cbNeedle, PRTGCPHYS pGCPhysHit);
VMMR3_INT_DECL(int) PGMR3DbgScanVirtual(PVM pVM, PVMCPU pVCpu, RTGCPTR GCPtr, RTGCPTR cbRange, RTGCPTR GCPtrAlign, const uint8_t *pabNeedle, size_t cbNeedle, PRTGCUINTPTR pGCPhysHit);
VMMR3_INT_DECL(int) PGMR3DumpHierarchyShw(PVM pVM, uint64_t cr3, uint32_t fFlags, uint64_t u64FirstAddr, uint64_t u64LastAddr, uint32_t cMaxDepth, PCDBGFINFOHLP pHlp);
VMMR3_INT_DECL(int) PGMR3DumpHierarchyGst(PVM pVM, uint64_t cr3, uint32_t fFlags, RTGCPTR FirstAddr, RTGCPTR LastAddr, uint32_t cMaxDepth, PCDBGFINFOHLP pHlp);


/** @name Page sharing
 * @{ */
VMMR3DECL(int)     PGMR3SharedModuleRegister(PVM pVM, VBOXOSFAMILY enmGuestOS, char *pszModuleName, char *pszVersion,
                                             RTGCPTR GCBaseAddr, uint32_t cbModule,
                                             uint32_t cRegions, VMMDEVSHAREDREGIONDESC const *paRegions);
VMMR3DECL(int)     PGMR3SharedModuleUnregister(PVM pVM, char *pszModuleName, char *pszVersion,
                                               RTGCPTR GCBaseAddr, uint32_t cbModule);
VMMR3DECL(int)     PGMR3SharedModuleCheckAll(PVM pVM);
VMMR3DECL(int)     PGMR3SharedModuleGetPageState(PVM pVM, RTGCPTR GCPtrPage, bool *pfShared, uint64_t *pfPageFlags);
/** @} */

/** @} */
#endif /* IN_RING3 */

RT_C_DECLS_END

/** @} */
#endif

