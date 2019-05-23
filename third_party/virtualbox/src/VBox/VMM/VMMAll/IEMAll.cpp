/* $Id: IEMAll.cpp $ */
/** @file
 * IEM - Interpreted Execution Manager - All Contexts.
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


/** @page pg_iem    IEM - Interpreted Execution Manager
 *
 * The interpreted exeuction manager (IEM) is for executing short guest code
 * sequences that are causing too many exits / virtualization traps.  It will
 * also be used to interpret single instructions, thus replacing the selective
 * interpreters in EM and IOM.
 *
 * Design goals:
 *      - Relatively small footprint, although we favour speed and correctness
 *        over size.
 *      - Reasonably fast.
 *      - Correctly handle lock prefixed instructions.
 *      - Complete instruction set - eventually.
 *      - Refactorable into a recompiler, maybe.
 *      - Replace EMInterpret*.
 *
 * Using the existing disassembler has been considered, however this is thought
 * to conflict with speed as the disassembler chews things a bit too much while
 * leaving us with a somewhat complicated state to interpret afterwards.
 *
 *
 * The current code is very much work in progress. You've been warned!
 *
 *
 * @section sec_iem_fpu_instr   FPU Instructions
 *
 * On x86 and AMD64 hosts, the FPU instructions are implemented by executing the
 * same or equivalent instructions on the host FPU.  To make life easy, we also
 * let the FPU prioritize the unmasked exceptions for us.  This however, only
 * works reliably when CR0.NE is set, i.e. when using \#MF instead the IRQ 13
 * for FPU exception delivery, because with CR0.NE=0 there is a window where we
 * can trigger spurious FPU exceptions.
 *
 * The guest FPU state is not loaded into the host CPU and kept there till we
 * leave IEM because the calling conventions have declared an all year open
 * season on much of the FPU state.  For instance an innocent looking call to
 * memcpy might end up using a whole bunch of XMM or MM registers if the
 * particular implementation finds it worthwhile.
 *
 *
 * @section sec_iem_logging     Logging
 *
 * The IEM code uses the \"IEM\" log group for the main logging. The different
 * logging levels/flags are generally used for the following purposes:
 *      - Level 1 (Log) : Errors, exceptions, interrupts and such major events.
 *      - Flow (LogFlow): Basic enter/exit IEM state info.
 *      - Level 2 (Log2): ?
 *      - Level 3 (Log3): More detailed enter/exit IEM state info.
 *      - Level 4 (Log4): Decoding mnemonics w/ EIP.
 *      - Level 5 (Log5): Decoding details.
 *      - Level 6 (Log6): Enables/disables the lockstep comparison with REM.
 *      - Level 7 (Log7): iret++ execution logging.
 *      - Level 8 (Log8): Memory writes.
 *      - Level 9 (Log9): Memory reads.
 *
 */

/** @def IEM_VERIFICATION_MODE_MINIMAL
 * Use for pitting IEM against EM or something else in ring-0 or raw-mode
 * context. */
#if defined(DOXYGEN_RUNNING)
# define IEM_VERIFICATION_MODE_MINIMAL
#endif
//#define IEM_LOG_MEMORY_WRITES
#define IEM_IMPLEMENTS_TASKSWITCH

/* Disabled warning C4505: 'iemRaisePageFaultJmp' : unreferenced local function has been removed */
#ifdef _MSC_VER
# pragma warning(disable:4505)
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP   LOG_GROUP_IEM
#define VMCPU_INCL_CPUM_GST_CTX
#include <VBox/vmm/iem.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#ifdef VBOX_WITH_NESTED_HWVIRT
# include <VBox/vmm/em.h>
# include <VBox/vmm/hm_svm.h>
#endif
#include <VBox/vmm/tm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/dbgftrace.h>
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
# include <VBox/vmm/patm.h>
# if defined(VBOX_WITH_CALL_RECORD) || defined(REM_MONITOR_CODE_PAGES)
#  include <VBox/vmm/csam.h>
# endif
#endif
#include "IEMInternal.h"
#ifdef IEM_VERIFICATION_MODE_FULL
# include <VBox/vmm/rem.h>
# include <VBox/vmm/mm.h>
#endif
#include <VBox/vmm/vm.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/x86.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** @typedef PFNIEMOP
 * Pointer to an opcode decoder function.
 */

/** @def FNIEMOP_DEF
 * Define an opcode decoder function.
 *
 * We're using macors for this so that adding and removing parameters as well as
 * tweaking compiler specific attributes becomes easier.  See FNIEMOP_CALL
 *
 * @param   a_Name      The function name.
 */

/** @typedef PFNIEMOPRM
 * Pointer to an opcode decoder function with RM byte.
 */

/** @def FNIEMOPRM_DEF
 * Define an opcode decoder function with RM byte.
 *
 * We're using macors for this so that adding and removing parameters as well as
 * tweaking compiler specific attributes becomes easier.  See FNIEMOP_CALL_1
 *
 * @param   a_Name      The function name.
 */

#if defined(__GNUC__) && defined(RT_ARCH_X86)
typedef VBOXSTRICTRC (__attribute__((__fastcall__)) * PFNIEMOP)(PVMCPU pVCpu);
typedef VBOXSTRICTRC (__attribute__((__fastcall__)) * PFNIEMOPRM)(PVMCPU pVCpu, uint8_t bRm);
# define FNIEMOP_DEF(a_Name) \
    IEM_STATIC VBOXSTRICTRC __attribute__((__fastcall__, __nothrow__)) a_Name(PVMCPU pVCpu)
# define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    IEM_STATIC VBOXSTRICTRC __attribute__((__fastcall__, __nothrow__)) a_Name(PVMCPU pVCpu, a_Type0 a_Name0)
# define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) \
    IEM_STATIC VBOXSTRICTRC __attribute__((__fastcall__, __nothrow__)) a_Name(PVMCPU pVCpu, a_Type0 a_Name0, a_Type1 a_Name1)

#elif defined(_MSC_VER) && defined(RT_ARCH_X86)
typedef VBOXSTRICTRC (__fastcall * PFNIEMOP)(PVMCPU pVCpu);
typedef VBOXSTRICTRC (__fastcall * PFNIEMOPRM)(PVMCPU pVCpu, uint8_t bRm);
# define FNIEMOP_DEF(a_Name) \
    IEM_STATIC /*__declspec(naked)*/ VBOXSTRICTRC __fastcall a_Name(PVMCPU pVCpu) RT_NO_THROW_DEF
# define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    IEM_STATIC /*__declspec(naked)*/ VBOXSTRICTRC __fastcall a_Name(PVMCPU pVCpu, a_Type0 a_Name0) RT_NO_THROW_DEF
# define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) \
    IEM_STATIC /*__declspec(naked)*/ VBOXSTRICTRC __fastcall a_Name(PVMCPU pVCpu, a_Type0 a_Name0, a_Type1 a_Name1) RT_NO_THROW_DEF

#elif defined(__GNUC__)
typedef VBOXSTRICTRC (* PFNIEMOP)(PVMCPU pVCpu);
typedef VBOXSTRICTRC (* PFNIEMOPRM)(PVMCPU pVCpu, uint8_t bRm);
# define FNIEMOP_DEF(a_Name) \
    IEM_STATIC VBOXSTRICTRC __attribute__((__nothrow__)) a_Name(PVMCPU pVCpu)
# define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    IEM_STATIC VBOXSTRICTRC __attribute__((__nothrow__)) a_Name(PVMCPU pVCpu, a_Type0 a_Name0)
# define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) \
    IEM_STATIC VBOXSTRICTRC __attribute__((__nothrow__)) a_Name(PVMCPU pVCpu, a_Type0 a_Name0, a_Type1 a_Name1)

#else
typedef VBOXSTRICTRC (* PFNIEMOP)(PVMCPU pVCpu);
typedef VBOXSTRICTRC (* PFNIEMOPRM)(PVMCPU pVCpu, uint8_t bRm);
# define FNIEMOP_DEF(a_Name) \
    IEM_STATIC VBOXSTRICTRC a_Name(PVMCPU pVCpu) RT_NO_THROW_DEF
# define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    IEM_STATIC VBOXSTRICTRC a_Name(PVMCPU pVCpu, a_Type0 a_Name0) RT_NO_THROW_DEF
# define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) \
    IEM_STATIC VBOXSTRICTRC a_Name(PVMCPU pVCpu, a_Type0 a_Name0, a_Type1 a_Name1) RT_NO_THROW_DEF

#endif
#define FNIEMOPRM_DEF(a_Name) FNIEMOP_DEF_1(a_Name, uint8_t, bRm)


/**
 * Selector descriptor table entry as fetched by iemMemFetchSelDesc.
 */
typedef union IEMSELDESC
{
    /** The legacy view. */
    X86DESC     Legacy;
    /** The long mode view. */
    X86DESC64   Long;
} IEMSELDESC;
/** Pointer to a selector descriptor table entry. */
typedef IEMSELDESC *PIEMSELDESC;

/**
 * CPU exception classes.
 */
typedef enum IEMXCPTCLASS
{
    IEMXCPTCLASS_BENIGN,
    IEMXCPTCLASS_CONTRIBUTORY,
    IEMXCPTCLASS_PAGE_FAULT,
    IEMXCPTCLASS_DOUBLE_FAULT
} IEMXCPTCLASS;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def IEM_WITH_SETJMP
 * Enables alternative status code handling using setjmps.
 *
 * This adds a bit of expense via the setjmp() call since it saves all the
 * non-volatile registers.  However, it eliminates return code checks and allows
 * for more optimal return value passing (return regs instead of stack buffer).
 */
#if defined(DOXYGEN_RUNNING) || defined(RT_OS_WINDOWS) || 1
# define IEM_WITH_SETJMP
#endif

/** Temporary hack to disable the double execution.  Will be removed in favor
 * of a dedicated execution mode in EM. */
//#define IEM_VERIFICATION_MODE_NO_REM

/** Used to shut up GCC warnings about variables that 'may be used uninitialized'
 * due to GCC lacking knowledge about the value range of a switch. */
#define IEM_NOT_REACHED_DEFAULT_CASE_RET() default: AssertFailedReturn(VERR_IPE_NOT_REACHED_DEFAULT_CASE)

/** Variant of IEM_NOT_REACHED_DEFAULT_CASE_RET that returns a custom value. */
#define IEM_NOT_REACHED_DEFAULT_CASE_RET2(a_RetValue) default: AssertFailedReturn(a_RetValue)

/**
 * Returns IEM_RETURN_ASPECT_NOT_IMPLEMENTED, and in debug builds logs the
 * occation.
 */
#ifdef LOG_ENABLED
# define IEM_RETURN_ASPECT_NOT_IMPLEMENTED() \
    do { \
        /*Log*/ LogAlways(("%s: returning IEM_RETURN_ASPECT_NOT_IMPLEMENTED (line %d)\n", __FUNCTION__, __LINE__)); \
        return VERR_IEM_ASPECT_NOT_IMPLEMENTED; \
    } while (0)
#else
# define IEM_RETURN_ASPECT_NOT_IMPLEMENTED() \
    return VERR_IEM_ASPECT_NOT_IMPLEMENTED
#endif

/**
 * Returns IEM_RETURN_ASPECT_NOT_IMPLEMENTED, and in debug builds logs the
 * occation using the supplied logger statement.
 *
 * @param   a_LoggerArgs    What to log on failure.
 */
#ifdef LOG_ENABLED
# define IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(a_LoggerArgs) \
    do { \
        LogAlways((LOG_FN_FMT ": ", __PRETTY_FUNCTION__)); LogAlways(a_LoggerArgs); \
        /*LogFunc(a_LoggerArgs);*/ \
        return VERR_IEM_ASPECT_NOT_IMPLEMENTED; \
    } while (0)
#else
# define IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(a_LoggerArgs) \
    return VERR_IEM_ASPECT_NOT_IMPLEMENTED
#endif

/**
 * Call an opcode decoder function.
 *
 * We're using macors for this so that adding and removing parameters can be
 * done as we please.  See FNIEMOP_DEF.
 */
#define FNIEMOP_CALL(a_pfn) (a_pfn)(pVCpu)

/**
 * Call a common opcode decoder function taking one extra argument.
 *
 * We're using macors for this so that adding and removing parameters can be
 * done as we please.  See FNIEMOP_DEF_1.
 */
#define FNIEMOP_CALL_1(a_pfn, a0)           (a_pfn)(pVCpu, a0)

/**
 * Call a common opcode decoder function taking one extra argument.
 *
 * We're using macors for this so that adding and removing parameters can be
 * done as we please.  See FNIEMOP_DEF_1.
 */
#define FNIEMOP_CALL_2(a_pfn, a0, a1)       (a_pfn)(pVCpu, a0, a1)

/**
 * Check if we're currently executing in real or virtual 8086 mode.
 *
 * @returns @c true if it is, @c false if not.
 * @param   a_pVCpu         The IEM state of the current CPU.
 */
#define IEM_IS_REAL_OR_V86_MODE(a_pVCpu)    (CPUMIsGuestInRealOrV86ModeEx(IEM_GET_CTX(a_pVCpu)))

/**
 * Check if we're currently executing in virtual 8086 mode.
 *
 * @returns @c true if it is, @c false if not.
 * @param   a_pVCpu         The cross context virtual CPU structure of the calling thread.
 */
#define IEM_IS_V86_MODE(a_pVCpu)            (CPUMIsGuestInV86ModeEx(IEM_GET_CTX(a_pVCpu)))

/**
 * Check if we're currently executing in long mode.
 *
 * @returns @c true if it is, @c false if not.
 * @param   a_pVCpu         The cross context virtual CPU structure of the calling thread.
 */
#define IEM_IS_LONG_MODE(a_pVCpu)           (CPUMIsGuestInLongModeEx(IEM_GET_CTX(a_pVCpu)))

/**
 * Check if we're currently executing in real mode.
 *
 * @returns @c true if it is, @c false if not.
 * @param   a_pVCpu         The cross context virtual CPU structure of the calling thread.
 */
#define IEM_IS_REAL_MODE(a_pVCpu)           (CPUMIsGuestInRealModeEx(IEM_GET_CTX(a_pVCpu)))

/**
 * Returns a (const) pointer to the CPUMFEATURES for the guest CPU.
 * @returns PCCPUMFEATURES
 * @param   a_pVCpu         The cross context virtual CPU structure of the calling thread.
 */
#define IEM_GET_GUEST_CPU_FEATURES(a_pVCpu) (&((a_pVCpu)->CTX_SUFF(pVM)->cpum.ro.GuestFeatures))

/**
 * Returns a (const) pointer to the CPUMFEATURES for the host CPU.
 * @returns PCCPUMFEATURES
 * @param   a_pVCpu         The cross context virtual CPU structure of the calling thread.
 */
#define IEM_GET_HOST_CPU_FEATURES(a_pVCpu)  (&((a_pVCpu)->CTX_SUFF(pVM)->cpum.ro.HostFeatures))

/**
 * Evaluates to true if we're presenting an Intel CPU to the guest.
 */
#define IEM_IS_GUEST_CPU_INTEL(a_pVCpu)     ( (a_pVCpu)->iem.s.enmCpuVendor == CPUMCPUVENDOR_INTEL )

/**
 * Evaluates to true if we're presenting an AMD CPU to the guest.
 */
#define IEM_IS_GUEST_CPU_AMD(a_pVCpu)       ( (a_pVCpu)->iem.s.enmCpuVendor == CPUMCPUVENDOR_AMD )

/**
 * Check if the address is canonical.
 */
#define IEM_IS_CANONICAL(a_u64Addr)         X86_IS_CANONICAL(a_u64Addr)

/**
 * Gets the effective VEX.VVVV value.
 *
 * The 4th bit is ignored if not 64-bit code.
 * @returns effective V-register value.
 * @param   a_pVCpu         The cross context virtual CPU structure of the calling thread.
 */
#define IEM_GET_EFFECTIVE_VVVV(a_pVCpu) \
    ((a_pVCpu)->iem.s.enmCpuMode == IEMMODE_64BIT ? (a_pVCpu)->iem.s.uVex3rdReg : (a_pVCpu)->iem.s.uVex3rdReg & 7)

/** @def IEM_USE_UNALIGNED_DATA_ACCESS
 * Use unaligned accesses instead of elaborate byte assembly. */
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86) || defined(DOXYGEN_RUNNING)
# define IEM_USE_UNALIGNED_DATA_ACCESS
#endif

#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Check the common SVM instruction preconditions.
 */
# define IEM_SVM_INSTR_COMMON_CHECKS(a_pVCpu, a_Instr) \
    do { \
        if (!IEM_IS_SVM_ENABLED(a_pVCpu)) \
        { \
            Log((RT_STR(a_Instr) ": EFER.SVME not enabled -> #UD\n")); \
            return iemRaiseUndefinedOpcode(pVCpu); \
        } \
        if (IEM_IS_REAL_OR_V86_MODE(pVCpu)) \
        { \
            Log((RT_STR(a_Instr) ": Real or v8086 mode -> #UD\n")); \
            return iemRaiseUndefinedOpcode(pVCpu); \
        } \
        if (pVCpu->iem.s.uCpl != 0) \
        { \
            Log((RT_STR(a_Instr) ": CPL != 0 -> #GP(0)\n")); \
            return iemRaiseGeneralProtectionFault0(pVCpu); \
        } \
    } while (0)

/**
 * Check if an SVM is enabled.
 */
# define IEM_IS_SVM_ENABLED(a_pVCpu)                         (CPUMIsGuestSvmEnabled(IEM_GET_CTX(a_pVCpu)))

/**
 * Check if an SVM control/instruction intercept is set.
 */
# define IEM_IS_SVM_CTRL_INTERCEPT_SET(a_pVCpu, a_Intercept) (CPUMIsGuestSvmCtrlInterceptSet(IEM_GET_CTX(a_pVCpu), (a_Intercept)))

/**
 * Check if an SVM read CRx intercept is set.
 */
# define IEM_IS_SVM_READ_CR_INTERCEPT_SET(a_pVCpu, a_uCr)    (CPUMIsGuestSvmReadCRxInterceptSet(IEM_GET_CTX(a_pVCpu), (a_uCr)))

/**
 * Check if an SVM write CRx intercept is set.
 */
# define IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(a_pVCpu, a_uCr)   (CPUMIsGuestSvmWriteCRxInterceptSet(IEM_GET_CTX(a_pVCpu), (a_uCr)))

/**
 * Check if an SVM read DRx intercept is set.
 */
# define IEM_IS_SVM_READ_DR_INTERCEPT_SET(a_pVCpu, a_uDr)    (CPUMIsGuestSvmReadDRxInterceptSet(IEM_GET_CTX(a_pVCpu), (a_uDr)))

/**
 * Check if an SVM write DRx intercept is set.
 */
# define IEM_IS_SVM_WRITE_DR_INTERCEPT_SET(a_pVCpu, a_uDr)   (CPUMIsGuestSvmWriteDRxInterceptSet(IEM_GET_CTX(a_pVCpu), (a_uDr)))

/**
 * Check if an SVM exception intercept is set.
 */
# define IEM_IS_SVM_XCPT_INTERCEPT_SET(a_pVCpu, a_uVector)   (CPUMIsGuestSvmXcptInterceptSet(IEM_GET_CTX(a_pVCpu), (a_uVector)))

/**
 * Invokes the SVM \#VMEXIT handler for the nested-guest.
 */
# define IEM_RETURN_SVM_VMEXIT(a_pVCpu, a_uExitCode, a_uExitInfo1, a_uExitInfo2) \
    do \
    { \
        return iemSvmVmexit((a_pVCpu), IEM_GET_CTX(a_pVCpu), (a_uExitCode), (a_uExitInfo1), (a_uExitInfo2)); \
    } while (0)

/**
 * Invokes the 'MOV CRx' SVM \#VMEXIT handler after constructing the
 * corresponding decode assist information.
 */
# define IEM_RETURN_SVM_CRX_VMEXIT(a_pVCpu, a_uExitCode, a_enmAccessCrX, a_iGReg) \
    do \
    { \
        uint64_t uExitInfo1; \
        if (   IEM_GET_GUEST_CPU_FEATURES(a_pVCpu)->fSvmDecodeAssist \
            && (a_enmAccessCrX) == IEMACCESSCRX_MOV_CRX) \
            uExitInfo1 = SVM_EXIT1_MOV_CRX_MASK | ((a_iGReg) & 7); \
        else \
            uExitInfo1 = 0; \
        IEM_RETURN_SVM_VMEXIT(a_pVCpu, a_uExitCode, uExitInfo1, 0); \
    } while (0)

#else
# define IEM_SVM_INSTR_COMMON_CHECKS(a_pVCpu, a_Instr)                                    do { } while (0)
# define IEM_IS_SVM_ENABLED(a_pVCpu)                                                      (false)
# define IEM_IS_SVM_CTRL_INTERCEPT_SET(a_pVCpu, a_Intercept)                              (false)
# define IEM_IS_SVM_READ_CR_INTERCEPT_SET(a_pVCpu, a_uCr)                                 (false)
# define IEM_IS_SVM_WRITE_CR_INTERCEPT_SET(a_pVCpu, a_uCr)                                (false)
# define IEM_IS_SVM_READ_DR_INTERCEPT_SET(a_pVCpu, a_uDr)                                 (false)
# define IEM_IS_SVM_WRITE_DR_INTERCEPT_SET(a_pVCpu, a_uDr)                                (false)
# define IEM_IS_SVM_XCPT_INTERCEPT_SET(a_pVCpu, a_uVector)                                (false)
# define IEM_RETURN_SVM_VMEXIT(a_pVCpu, a_uExitCode, a_uExitInfo1, a_uExitInfo2)          do { return VERR_SVM_IPE_1; } while (0)
# define IEM_RETURN_SVM_CRX_VMEXIT(a_pVCpu, a_uExitCode, a_enmAccessCrX, a_iGReg)         do { return VERR_SVM_IPE_1; } while (0)

#endif /* VBOX_WITH_NESTED_HWVIRT */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
extern const PFNIEMOP g_apfnOneByteMap[256]; /* not static since we need to forward declare it. */


/** Function table for the ADD instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_add =
{
    iemAImpl_add_u8,  iemAImpl_add_u8_locked,
    iemAImpl_add_u16, iemAImpl_add_u16_locked,
    iemAImpl_add_u32, iemAImpl_add_u32_locked,
    iemAImpl_add_u64, iemAImpl_add_u64_locked
};

/** Function table for the ADC instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_adc =
{
    iemAImpl_adc_u8,  iemAImpl_adc_u8_locked,
    iemAImpl_adc_u16, iemAImpl_adc_u16_locked,
    iemAImpl_adc_u32, iemAImpl_adc_u32_locked,
    iemAImpl_adc_u64, iemAImpl_adc_u64_locked
};

/** Function table for the SUB instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_sub =
{
    iemAImpl_sub_u8,  iemAImpl_sub_u8_locked,
    iemAImpl_sub_u16, iemAImpl_sub_u16_locked,
    iemAImpl_sub_u32, iemAImpl_sub_u32_locked,
    iemAImpl_sub_u64, iemAImpl_sub_u64_locked
};

/** Function table for the SBB instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_sbb =
{
    iemAImpl_sbb_u8,  iemAImpl_sbb_u8_locked,
    iemAImpl_sbb_u16, iemAImpl_sbb_u16_locked,
    iemAImpl_sbb_u32, iemAImpl_sbb_u32_locked,
    iemAImpl_sbb_u64, iemAImpl_sbb_u64_locked
};

/** Function table for the OR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_or =
{
    iemAImpl_or_u8,  iemAImpl_or_u8_locked,
    iemAImpl_or_u16, iemAImpl_or_u16_locked,
    iemAImpl_or_u32, iemAImpl_or_u32_locked,
    iemAImpl_or_u64, iemAImpl_or_u64_locked
};

/** Function table for the XOR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_xor =
{
    iemAImpl_xor_u8,  iemAImpl_xor_u8_locked,
    iemAImpl_xor_u16, iemAImpl_xor_u16_locked,
    iemAImpl_xor_u32, iemAImpl_xor_u32_locked,
    iemAImpl_xor_u64, iemAImpl_xor_u64_locked
};

/** Function table for the AND instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_and =
{
    iemAImpl_and_u8,  iemAImpl_and_u8_locked,
    iemAImpl_and_u16, iemAImpl_and_u16_locked,
    iemAImpl_and_u32, iemAImpl_and_u32_locked,
    iemAImpl_and_u64, iemAImpl_and_u64_locked
};

/** Function table for the CMP instruction.
 * @remarks Making operand order ASSUMPTIONS.
 */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_cmp =
{
    iemAImpl_cmp_u8,  NULL,
    iemAImpl_cmp_u16, NULL,
    iemAImpl_cmp_u32, NULL,
    iemAImpl_cmp_u64, NULL
};

/** Function table for the TEST instruction.
 * @remarks Making operand order ASSUMPTIONS.
 */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_test =
{
    iemAImpl_test_u8,  NULL,
    iemAImpl_test_u16, NULL,
    iemAImpl_test_u32, NULL,
    iemAImpl_test_u64, NULL
};

/** Function table for the BT instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bt =
{
    NULL,  NULL,
    iemAImpl_bt_u16, NULL,
    iemAImpl_bt_u32, NULL,
    iemAImpl_bt_u64, NULL
};

/** Function table for the BTC instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_btc =
{
    NULL,  NULL,
    iemAImpl_btc_u16, iemAImpl_btc_u16_locked,
    iemAImpl_btc_u32, iemAImpl_btc_u32_locked,
    iemAImpl_btc_u64, iemAImpl_btc_u64_locked
};

/** Function table for the BTR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_btr =
{
    NULL,  NULL,
    iemAImpl_btr_u16, iemAImpl_btr_u16_locked,
    iemAImpl_btr_u32, iemAImpl_btr_u32_locked,
    iemAImpl_btr_u64, iemAImpl_btr_u64_locked
};

/** Function table for the BTS instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bts =
{
    NULL,  NULL,
    iemAImpl_bts_u16, iemAImpl_bts_u16_locked,
    iemAImpl_bts_u32, iemAImpl_bts_u32_locked,
    iemAImpl_bts_u64, iemAImpl_bts_u64_locked
};

/** Function table for the BSF instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsf =
{
    NULL,  NULL,
    iemAImpl_bsf_u16, NULL,
    iemAImpl_bsf_u32, NULL,
    iemAImpl_bsf_u64, NULL
};

/** Function table for the BSR instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_bsr =
{
    NULL,  NULL,
    iemAImpl_bsr_u16, NULL,
    iemAImpl_bsr_u32, NULL,
    iemAImpl_bsr_u64, NULL
};

/** Function table for the IMUL instruction. */
IEM_STATIC const IEMOPBINSIZES g_iemAImpl_imul_two =
{
    NULL,  NULL,
    iemAImpl_imul_two_u16, NULL,
    iemAImpl_imul_two_u32, NULL,
    iemAImpl_imul_two_u64, NULL
};

/** Group 1 /r lookup table. */
IEM_STATIC const PCIEMOPBINSIZES g_apIemImplGrp1[8] =
{
    &g_iemAImpl_add,
    &g_iemAImpl_or,
    &g_iemAImpl_adc,
    &g_iemAImpl_sbb,
    &g_iemAImpl_and,
    &g_iemAImpl_sub,
    &g_iemAImpl_xor,
    &g_iemAImpl_cmp
};

/** Function table for the INC instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_inc =
{
    iemAImpl_inc_u8,  iemAImpl_inc_u8_locked,
    iemAImpl_inc_u16, iemAImpl_inc_u16_locked,
    iemAImpl_inc_u32, iemAImpl_inc_u32_locked,
    iemAImpl_inc_u64, iemAImpl_inc_u64_locked
};

/** Function table for the DEC instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_dec =
{
    iemAImpl_dec_u8,  iemAImpl_dec_u8_locked,
    iemAImpl_dec_u16, iemAImpl_dec_u16_locked,
    iemAImpl_dec_u32, iemAImpl_dec_u32_locked,
    iemAImpl_dec_u64, iemAImpl_dec_u64_locked
};

/** Function table for the NEG instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_neg =
{
    iemAImpl_neg_u8,  iemAImpl_neg_u8_locked,
    iemAImpl_neg_u16, iemAImpl_neg_u16_locked,
    iemAImpl_neg_u32, iemAImpl_neg_u32_locked,
    iemAImpl_neg_u64, iemAImpl_neg_u64_locked
};

/** Function table for the NOT instruction. */
IEM_STATIC const IEMOPUNARYSIZES g_iemAImpl_not =
{
    iemAImpl_not_u8,  iemAImpl_not_u8_locked,
    iemAImpl_not_u16, iemAImpl_not_u16_locked,
    iemAImpl_not_u32, iemAImpl_not_u32_locked,
    iemAImpl_not_u64, iemAImpl_not_u64_locked
};


/** Function table for the ROL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rol =
{
    iemAImpl_rol_u8,
    iemAImpl_rol_u16,
    iemAImpl_rol_u32,
    iemAImpl_rol_u64
};

/** Function table for the ROR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_ror =
{
    iemAImpl_ror_u8,
    iemAImpl_ror_u16,
    iemAImpl_ror_u32,
    iemAImpl_ror_u64
};

/** Function table for the RCL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcl =
{
    iemAImpl_rcl_u8,
    iemAImpl_rcl_u16,
    iemAImpl_rcl_u32,
    iemAImpl_rcl_u64
};

/** Function table for the RCR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_rcr =
{
    iemAImpl_rcr_u8,
    iemAImpl_rcr_u16,
    iemAImpl_rcr_u32,
    iemAImpl_rcr_u64
};

/** Function table for the SHL instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shl =
{
    iemAImpl_shl_u8,
    iemAImpl_shl_u16,
    iemAImpl_shl_u32,
    iemAImpl_shl_u64
};

/** Function table for the SHR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_shr =
{
    iemAImpl_shr_u8,
    iemAImpl_shr_u16,
    iemAImpl_shr_u32,
    iemAImpl_shr_u64
};

/** Function table for the SAR instruction. */
IEM_STATIC const IEMOPSHIFTSIZES g_iemAImpl_sar =
{
    iemAImpl_sar_u8,
    iemAImpl_sar_u16,
    iemAImpl_sar_u32,
    iemAImpl_sar_u64
};


/** Function table for the MUL instruction. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_mul =
{
    iemAImpl_mul_u8,
    iemAImpl_mul_u16,
    iemAImpl_mul_u32,
    iemAImpl_mul_u64
};

/** Function table for the IMUL instruction working implicitly on rAX. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_imul =
{
    iemAImpl_imul_u8,
    iemAImpl_imul_u16,
    iemAImpl_imul_u32,
    iemAImpl_imul_u64
};

/** Function table for the DIV instruction. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_div =
{
    iemAImpl_div_u8,
    iemAImpl_div_u16,
    iemAImpl_div_u32,
    iemAImpl_div_u64
};

/** Function table for the MUL instruction. */
IEM_STATIC const IEMOPMULDIVSIZES g_iemAImpl_idiv =
{
    iemAImpl_idiv_u8,
    iemAImpl_idiv_u16,
    iemAImpl_idiv_u32,
    iemAImpl_idiv_u64
};

/** Function table for the SHLD instruction */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shld =
{
    iemAImpl_shld_u16,
    iemAImpl_shld_u32,
    iemAImpl_shld_u64,
};

/** Function table for the SHRD instruction */
IEM_STATIC const IEMOPSHIFTDBLSIZES g_iemAImpl_shrd =
{
    iemAImpl_shrd_u16,
    iemAImpl_shrd_u32,
    iemAImpl_shrd_u64,
};


/** Function table for the PUNPCKLBW instruction */
IEM_STATIC const IEMOPMEDIAF1L1 g_iemAImpl_punpcklbw  = { iemAImpl_punpcklbw_u64,  iemAImpl_punpcklbw_u128 };
/** Function table for the PUNPCKLBD instruction */
IEM_STATIC const IEMOPMEDIAF1L1 g_iemAImpl_punpcklwd  = { iemAImpl_punpcklwd_u64,  iemAImpl_punpcklwd_u128 };
/** Function table for the PUNPCKLDQ instruction */
IEM_STATIC const IEMOPMEDIAF1L1 g_iemAImpl_punpckldq  = { iemAImpl_punpckldq_u64,  iemAImpl_punpckldq_u128 };
/** Function table for the PUNPCKLQDQ instruction */
IEM_STATIC const IEMOPMEDIAF1L1 g_iemAImpl_punpcklqdq = { NULL, iemAImpl_punpcklqdq_u128 };

/** Function table for the PUNPCKHBW instruction */
IEM_STATIC const IEMOPMEDIAF1H1 g_iemAImpl_punpckhbw  = { iemAImpl_punpckhbw_u64,  iemAImpl_punpckhbw_u128 };
/** Function table for the PUNPCKHBD instruction */
IEM_STATIC const IEMOPMEDIAF1H1 g_iemAImpl_punpckhwd  = { iemAImpl_punpckhwd_u64,  iemAImpl_punpckhwd_u128 };
/** Function table for the PUNPCKHDQ instruction */
IEM_STATIC const IEMOPMEDIAF1H1 g_iemAImpl_punpckhdq  = { iemAImpl_punpckhdq_u64,  iemAImpl_punpckhdq_u128 };
/** Function table for the PUNPCKHQDQ instruction */
IEM_STATIC const IEMOPMEDIAF1H1 g_iemAImpl_punpckhqdq = { NULL, iemAImpl_punpckhqdq_u128 };

/** Function table for the PXOR instruction */
IEM_STATIC const IEMOPMEDIAF2 g_iemAImpl_pxor         = { iemAImpl_pxor_u64,       iemAImpl_pxor_u128 };
/** Function table for the PCMPEQB instruction */
IEM_STATIC const IEMOPMEDIAF2 g_iemAImpl_pcmpeqb      = { iemAImpl_pcmpeqb_u64,    iemAImpl_pcmpeqb_u128 };
/** Function table for the PCMPEQW instruction */
IEM_STATIC const IEMOPMEDIAF2 g_iemAImpl_pcmpeqw      = { iemAImpl_pcmpeqw_u64,    iemAImpl_pcmpeqw_u128 };
/** Function table for the PCMPEQD instruction */
IEM_STATIC const IEMOPMEDIAF2 g_iemAImpl_pcmpeqd      = { iemAImpl_pcmpeqd_u64,    iemAImpl_pcmpeqd_u128 };


#if defined(IEM_VERIFICATION_MODE_MINIMAL) || defined(IEM_LOG_MEMORY_WRITES)
/** What IEM just wrote. */
uint8_t g_abIemWrote[256];
/** How much IEM just wrote. */
size_t g_cbIemWrote;
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
IEM_STATIC VBOXSTRICTRC     iemRaiseTaskSwitchFaultWithErr(PVMCPU pVCpu, uint16_t uErr);
IEM_STATIC VBOXSTRICTRC     iemRaiseTaskSwitchFaultCurrentTSS(PVMCPU pVCpu);
IEM_STATIC VBOXSTRICTRC     iemRaiseTaskSwitchFault0(PVMCPU pVCpu);
IEM_STATIC VBOXSTRICTRC     iemRaiseTaskSwitchFaultBySelector(PVMCPU pVCpu, uint16_t uSel);
/*IEM_STATIC VBOXSTRICTRC     iemRaiseSelectorNotPresent(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess);*/
IEM_STATIC VBOXSTRICTRC     iemRaiseSelectorNotPresentBySelector(PVMCPU pVCpu, uint16_t uSel);
IEM_STATIC VBOXSTRICTRC     iemRaiseSelectorNotPresentWithErr(PVMCPU pVCpu, uint16_t uErr);
IEM_STATIC VBOXSTRICTRC     iemRaiseStackSelectorNotPresentBySelector(PVMCPU pVCpu, uint16_t uSel);
IEM_STATIC VBOXSTRICTRC     iemRaiseStackSelectorNotPresentWithErr(PVMCPU pVCpu, uint16_t uErr);
IEM_STATIC VBOXSTRICTRC     iemRaiseGeneralProtectionFault(PVMCPU pVCpu, uint16_t uErr);
IEM_STATIC VBOXSTRICTRC     iemRaiseGeneralProtectionFault0(PVMCPU pVCpu);
IEM_STATIC VBOXSTRICTRC     iemRaiseGeneralProtectionFaultBySelector(PVMCPU pVCpu, RTSEL uSel);
IEM_STATIC VBOXSTRICTRC     iemRaiseSelectorBounds(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess);
IEM_STATIC VBOXSTRICTRC     iemRaiseSelectorBoundsBySelector(PVMCPU pVCpu, RTSEL Sel);
IEM_STATIC VBOXSTRICTRC     iemRaiseSelectorInvalidAccess(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess);
IEM_STATIC VBOXSTRICTRC     iemRaisePageFault(PVMCPU pVCpu, RTGCPTR GCPtrWhere, uint32_t fAccess, int rc);
IEM_STATIC VBOXSTRICTRC     iemRaiseAlignmentCheckException(PVMCPU pVCpu);
#ifdef IEM_WITH_SETJMP
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaisePageFaultJmp(PVMCPU pVCpu, RTGCPTR GCPtrWhere, uint32_t fAccess, int rc);
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseGeneralProtectionFault0Jmp(PVMCPU pVCpu);
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseSelectorBoundsJmp(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess);
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseSelectorBoundsBySelectorJmp(PVMCPU pVCpu, RTSEL Sel);
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseSelectorInvalidAccessJmp(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess);
#endif

IEM_STATIC VBOXSTRICTRC     iemMemMap(PVMCPU pVCpu, void **ppvMem, size_t cbMem, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t fAccess);
IEM_STATIC VBOXSTRICTRC     iemMemCommitAndUnmap(PVMCPU pVCpu, void *pvMem, uint32_t fAccess);
IEM_STATIC VBOXSTRICTRC     iemMemFetchDataU32(PVMCPU pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem);
IEM_STATIC VBOXSTRICTRC     iemMemFetchDataU64(PVMCPU pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem);
IEM_STATIC VBOXSTRICTRC     iemMemFetchSysU8(PVMCPU pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem);
IEM_STATIC VBOXSTRICTRC     iemMemFetchSysU16(PVMCPU pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem);
IEM_STATIC VBOXSTRICTRC     iemMemFetchSysU32(PVMCPU pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem);
IEM_STATIC VBOXSTRICTRC     iemMemFetchSysU64(PVMCPU pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem);
IEM_STATIC VBOXSTRICTRC     iemMemFetchSelDescWithErr(PVMCPU pVCpu, PIEMSELDESC pDesc, uint16_t uSel, uint8_t uXcpt, uint16_t uErrorCode);
IEM_STATIC VBOXSTRICTRC     iemMemFetchSelDesc(PVMCPU pVCpu, PIEMSELDESC pDesc, uint16_t uSel, uint8_t uXcpt);
IEM_STATIC VBOXSTRICTRC     iemMemStackPushCommitSpecial(PVMCPU pVCpu, void *pvMem, uint64_t uNewRsp);
IEM_STATIC VBOXSTRICTRC     iemMemStackPushBeginSpecial(PVMCPU pVCpu, size_t cbMem, void **ppvMem, uint64_t *puNewRsp);
IEM_STATIC VBOXSTRICTRC     iemMemStackPushU32(PVMCPU pVCpu, uint32_t u32Value);
IEM_STATIC VBOXSTRICTRC     iemMemStackPushU16(PVMCPU pVCpu, uint16_t u16Value);
IEM_STATIC VBOXSTRICTRC     iemMemMarkSelDescAccessed(PVMCPU pVCpu, uint16_t uSel);
IEM_STATIC uint16_t         iemSRegFetchU16(PVMCPU pVCpu, uint8_t iSegReg);
IEM_STATIC uint64_t         iemSRegBaseFetchU64(PVMCPU pVCpu, uint8_t iSegReg);

#if defined(IEM_VERIFICATION_MODE_FULL) && !defined(IEM_VERIFICATION_MODE_MINIMAL)
IEM_STATIC PIEMVERIFYEVTREC iemVerifyAllocRecord(PVMCPU pVCpu);
#endif
IEM_STATIC VBOXSTRICTRC     iemVerifyFakeIOPortRead(PVMCPU pVCpu, RTIOPORT Port, uint32_t *pu32Value, size_t cbValue);
IEM_STATIC VBOXSTRICTRC     iemVerifyFakeIOPortWrite(PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue);

#ifdef VBOX_WITH_NESTED_HWVIRT
IEM_STATIC VBOXSTRICTRC     iemSvmVmexit(PVMCPU pVCpu, PCPUMCTX pCtx, uint64_t uExitCode, uint64_t uExitInfo1,
                                         uint64_t uExitInfo2);
IEM_STATIC VBOXSTRICTRC     iemHandleSvmEventIntercept(PVMCPU pVCpu, PCPUMCTX pCtx, uint8_t u8Vector, uint32_t fFlags,
                                                       uint32_t uErr, uint64_t uCr2);
#endif

/**
 * Sets the pass up status.
 *
 * @returns VINF_SUCCESS.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   rcPassUp            The pass up status.  Must be informational.
 *                              VINF_SUCCESS is not allowed.
 */
IEM_STATIC int iemSetPassUpStatus(PVMCPU pVCpu, VBOXSTRICTRC rcPassUp)
{
    AssertRC(VBOXSTRICTRC_VAL(rcPassUp)); Assert(rcPassUp != VINF_SUCCESS);

    int32_t const rcOldPassUp = pVCpu->iem.s.rcPassUp;
    if (rcOldPassUp == VINF_SUCCESS)
        pVCpu->iem.s.rcPassUp = VBOXSTRICTRC_VAL(rcPassUp);
    /* If both are EM scheduling codes, use EM priority rules. */
    else if (   rcOldPassUp >= VINF_EM_FIRST && rcOldPassUp <= VINF_EM_LAST
             && rcPassUp    >= VINF_EM_FIRST && rcPassUp    <= VINF_EM_LAST)
    {
        if (rcPassUp < rcOldPassUp)
        {
            Log(("IEM: rcPassUp=%Rrc! rcOldPassUp=%Rrc\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
            pVCpu->iem.s.rcPassUp = VBOXSTRICTRC_VAL(rcPassUp);
        }
        else
            Log(("IEM: rcPassUp=%Rrc  rcOldPassUp=%Rrc!\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
    }
    /* Override EM scheduling with specific status code. */
    else if (rcOldPassUp >= VINF_EM_FIRST && rcOldPassUp <= VINF_EM_LAST)
    {
        Log(("IEM: rcPassUp=%Rrc! rcOldPassUp=%Rrc\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
        pVCpu->iem.s.rcPassUp = VBOXSTRICTRC_VAL(rcPassUp);
    }
    /* Don't override specific status code, first come first served. */
    else
        Log(("IEM: rcPassUp=%Rrc  rcOldPassUp=%Rrc!\n", VBOXSTRICTRC_VAL(rcPassUp), rcOldPassUp));
    return VINF_SUCCESS;
}


/**
 * Calculates the CPU mode.
 *
 * This is mainly for updating IEMCPU::enmCpuMode.
 *
 * @returns CPU mode.
 * @param   pCtx        The register context for the CPU.
 */
DECLINLINE(IEMMODE) iemCalcCpuMode(PCPUMCTX pCtx)
{
    if (CPUMIsGuestIn64BitCodeEx(pCtx))
        return IEMMODE_64BIT;
    if (pCtx->cs.Attr.n.u1DefBig) /** @todo check if this is correct... */
        return IEMMODE_32BIT;
    return IEMMODE_16BIT;
}


/**
 * Initializes the execution state.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fBypassHandlers     Whether to bypass access handlers.
 *
 * @remarks Callers of this must call iemUninitExec() to undo potentially fatal
 *          side-effects in strict builds.
 */
DECLINLINE(void) iemInitExec(PVMCPU pVCpu, bool fBypassHandlers)
{
    PCPUMCTX const pCtx = IEM_GET_CTX(pVCpu);

    Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_IEM));

#if defined(VBOX_STRICT) && (defined(IEM_VERIFICATION_MODE_FULL) || !defined(VBOX_WITH_RAW_MODE_NOT_R0))
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->es));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ds));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->fs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->gs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ldtr));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->tr));
#endif

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    CPUMGuestLazyLoadHiddenCsAndSs(pVCpu);
#endif
    pVCpu->iem.s.uCpl               = CPUMGetGuestCPL(pVCpu);
    pVCpu->iem.s.enmCpuMode         = iemCalcCpuMode(pCtx);
#ifdef VBOX_STRICT
    pVCpu->iem.s.enmDefAddrMode     = (IEMMODE)0xfe;
    pVCpu->iem.s.enmEffAddrMode     = (IEMMODE)0xfe;
    pVCpu->iem.s.enmDefOpSize       = (IEMMODE)0xfe;
    pVCpu->iem.s.enmEffOpSize       = (IEMMODE)0xfe;
    pVCpu->iem.s.fPrefixes          = 0xfeedbeef;
    pVCpu->iem.s.uRexReg            = 127;
    pVCpu->iem.s.uRexB              = 127;
    pVCpu->iem.s.uRexIndex          = 127;
    pVCpu->iem.s.iEffSeg            = 127;
    pVCpu->iem.s.idxPrefix          = 127;
    pVCpu->iem.s.uVex3rdReg         = 127;
    pVCpu->iem.s.uVexLength         = 127;
    pVCpu->iem.s.fEvexStuff         = 127;
    pVCpu->iem.s.uFpuOpcode         = UINT16_MAX;
# ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.offInstrNextByte   = UINT16_MAX;
    pVCpu->iem.s.pbInstrBuf         = NULL;
    pVCpu->iem.s.cbInstrBuf         = UINT16_MAX;
    pVCpu->iem.s.cbInstrBufTotal    = UINT16_MAX;
    pVCpu->iem.s.offCurInstrStart   = INT16_MAX;
    pVCpu->iem.s.uInstrBufPc        = UINT64_C(0xc0ffc0ffcff0c0ff);
# else
    pVCpu->iem.s.offOpcode          = 127;
    pVCpu->iem.s.cbOpcode           = 127;
# endif
#endif

    pVCpu->iem.s.cActiveMappings    = 0;
    pVCpu->iem.s.iNextMapping       = 0;
    pVCpu->iem.s.rcPassUp           = VINF_SUCCESS;
    pVCpu->iem.s.fBypassHandlers    = fBypassHandlers;
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    pVCpu->iem.s.fInPatchCode       = pVCpu->iem.s.uCpl == 0
                               && pCtx->cs.u64Base == 0
                               && pCtx->cs.u32Limit == UINT32_MAX
                               && PATMIsPatchGCAddr(pVCpu->CTX_SUFF(pVM), pCtx->eip);
    if (!pVCpu->iem.s.fInPatchCode)
        CPUMRawLeave(pVCpu, VINF_SUCCESS);
#endif

#ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.fNoRemSavedByExec = pVCpu->iem.s.fNoRem;
    pVCpu->iem.s.fNoRem = true;
#endif
}

#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Performs a minimal reinitialization of the execution state.
 *
 * This is intended to be used by VM-exits, SMM, LOADALL and other similar
 * 'world-switch' types operations on the CPU. Currently only nested
 * hardware-virtualization uses it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
IEM_STATIC void iemReInitExec(PVMCPU pVCpu)
{
    PCPUMCTX const pCtx   = IEM_GET_CTX(pVCpu);
    IEMMODE const enmMode = iemCalcCpuMode(pCtx);
    uint8_t const uCpl    = CPUMGetGuestCPL(pVCpu);

    pVCpu->iem.s.uCpl             = uCpl;
    pVCpu->iem.s.enmCpuMode       = enmMode;
    pVCpu->iem.s.enmDefAddrMode   = enmMode;  /** @todo check if this is correct... */
    pVCpu->iem.s.enmEffAddrMode   = enmMode;
    if (enmMode != IEMMODE_64BIT)
    {
        pVCpu->iem.s.enmDefOpSize = enmMode;  /** @todo check if this is correct... */
        pVCpu->iem.s.enmEffOpSize = enmMode;
    }
    else
    {
        pVCpu->iem.s.enmDefOpSize = IEMMODE_32BIT;
        pVCpu->iem.s.enmEffOpSize = enmMode;
    }
    pVCpu->iem.s.iEffSeg          = X86_SREG_DS;
#ifndef IEM_WITH_CODE_TLB
    /** @todo Shouldn't we be doing this in IEMTlbInvalidateAll()? */
    pVCpu->iem.s.offOpcode        = 0;
    pVCpu->iem.s.cbOpcode         = 0;
#endif
}
#endif

/**
 * Counterpart to #iemInitExec that undoes evil strict-build stuff.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 */
DECLINLINE(void) iemUninitExec(PVMCPU pVCpu)
{
    /* Note! do not touch fInPatchCode here! (see iemUninitExecAndFiddleStatusAndMaybeReenter) */
#ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.fNoRem = pVCpu->iem.s.fNoRemSavedByExec;
#endif
#ifdef VBOX_STRICT
# ifdef IEM_WITH_CODE_TLB
    NOREF(pVCpu);
# else
    pVCpu->iem.s.cbOpcode = 0;
# endif
#else
    NOREF(pVCpu);
#endif
}


/**
 * Initializes the decoder state.
 *
 * iemReInitDecoder is mostly a copy of this function.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fBypassHandlers     Whether to bypass access handlers.
 */
DECLINLINE(void) iemInitDecoder(PVMCPU pVCpu, bool fBypassHandlers)
{
    PCPUMCTX const pCtx = IEM_GET_CTX(pVCpu);

    Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_IEM));

#if defined(VBOX_STRICT) && (defined(IEM_VERIFICATION_MODE_FULL) || !defined(VBOX_WITH_RAW_MODE_NOT_R0))
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->es));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ds));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->fs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->gs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ldtr));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->tr));
#endif

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    CPUMGuestLazyLoadHiddenCsAndSs(pVCpu);
#endif
    pVCpu->iem.s.uCpl               = CPUMGetGuestCPL(pVCpu);
#ifdef IEM_VERIFICATION_MODE_FULL
    if (pVCpu->iem.s.uInjectCpl != UINT8_MAX)
        pVCpu->iem.s.uCpl           = pVCpu->iem.s.uInjectCpl;
#endif
    IEMMODE enmMode = iemCalcCpuMode(pCtx);
    pVCpu->iem.s.enmCpuMode         = enmMode;
    pVCpu->iem.s.enmDefAddrMode     = enmMode;  /** @todo check if this is correct... */
    pVCpu->iem.s.enmEffAddrMode     = enmMode;
    if (enmMode != IEMMODE_64BIT)
    {
        pVCpu->iem.s.enmDefOpSize   = enmMode;  /** @todo check if this is correct... */
        pVCpu->iem.s.enmEffOpSize   = enmMode;
    }
    else
    {
        pVCpu->iem.s.enmDefOpSize   = IEMMODE_32BIT;
        pVCpu->iem.s.enmEffOpSize   = IEMMODE_32BIT;
    }
    pVCpu->iem.s.fPrefixes          = 0;
    pVCpu->iem.s.uRexReg            = 0;
    pVCpu->iem.s.uRexB              = 0;
    pVCpu->iem.s.uRexIndex          = 0;
    pVCpu->iem.s.idxPrefix          = 0;
    pVCpu->iem.s.uVex3rdReg         = 0;
    pVCpu->iem.s.uVexLength         = 0;
    pVCpu->iem.s.fEvexStuff         = 0;
    pVCpu->iem.s.iEffSeg            = X86_SREG_DS;
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf         = NULL;
    pVCpu->iem.s.offInstrNextByte   = 0;
    pVCpu->iem.s.offCurInstrStart   = 0;
# ifdef VBOX_STRICT
    pVCpu->iem.s.cbInstrBuf         = UINT16_MAX;
    pVCpu->iem.s.cbInstrBufTotal    = UINT16_MAX;
    pVCpu->iem.s.uInstrBufPc        = UINT64_C(0xc0ffc0ffcff0c0ff);
# endif
#else
    pVCpu->iem.s.offOpcode          = 0;
    pVCpu->iem.s.cbOpcode           = 0;
#endif
    pVCpu->iem.s.cActiveMappings    = 0;
    pVCpu->iem.s.iNextMapping       = 0;
    pVCpu->iem.s.rcPassUp           = VINF_SUCCESS;
    pVCpu->iem.s.fBypassHandlers    = fBypassHandlers;
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    pVCpu->iem.s.fInPatchCode       = pVCpu->iem.s.uCpl == 0
                               && pCtx->cs.u64Base == 0
                               && pCtx->cs.u32Limit == UINT32_MAX
                               && PATMIsPatchGCAddr(pVCpu->CTX_SUFF(pVM), pCtx->eip);
    if (!pVCpu->iem.s.fInPatchCode)
        CPUMRawLeave(pVCpu, VINF_SUCCESS);
#endif

#ifdef DBGFTRACE_ENABLED
    switch (enmMode)
    {
        case IEMMODE_64BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I64/%u %08llx", pVCpu->iem.s.uCpl, pCtx->rip);
            break;
        case IEMMODE_32BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I32/%u %04x:%08x", pVCpu->iem.s.uCpl, pCtx->cs.Sel, pCtx->eip);
            break;
        case IEMMODE_16BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I16/%u %04x:%04x", pVCpu->iem.s.uCpl, pCtx->cs.Sel, pCtx->eip);
            break;
    }
#endif
}


/**
 * Reinitializes the decoder state 2nd+ loop of IEMExecLots.
 *
 * This is mostly a copy of iemInitDecoder.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 */
DECLINLINE(void) iemReInitDecoder(PVMCPU pVCpu)
{
    PCPUMCTX const pCtx = IEM_GET_CTX(pVCpu);

    Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_IEM));

#if defined(VBOX_STRICT) && (defined(IEM_VERIFICATION_MODE_FULL) || !defined(VBOX_WITH_RAW_MODE_NOT_R0))
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->es));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ds));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->fs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->gs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ldtr));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->tr));
#endif

    pVCpu->iem.s.uCpl               = CPUMGetGuestCPL(pVCpu);   /** @todo this should be updated during execution! */
#ifdef IEM_VERIFICATION_MODE_FULL
    if (pVCpu->iem.s.uInjectCpl != UINT8_MAX)
        pVCpu->iem.s.uCpl           = pVCpu->iem.s.uInjectCpl;
#endif
    IEMMODE enmMode = iemCalcCpuMode(pCtx);
    pVCpu->iem.s.enmCpuMode         = enmMode;                  /** @todo this should be updated during execution! */
    pVCpu->iem.s.enmDefAddrMode     = enmMode;  /** @todo check if this is correct... */
    pVCpu->iem.s.enmEffAddrMode     = enmMode;
    if (enmMode != IEMMODE_64BIT)
    {
        pVCpu->iem.s.enmDefOpSize   = enmMode;  /** @todo check if this is correct... */
        pVCpu->iem.s.enmEffOpSize   = enmMode;
    }
    else
    {
        pVCpu->iem.s.enmDefOpSize   = IEMMODE_32BIT;
        pVCpu->iem.s.enmEffOpSize   = IEMMODE_32BIT;
    }
    pVCpu->iem.s.fPrefixes          = 0;
    pVCpu->iem.s.uRexReg            = 0;
    pVCpu->iem.s.uRexB              = 0;
    pVCpu->iem.s.uRexIndex          = 0;
    pVCpu->iem.s.idxPrefix          = 0;
    pVCpu->iem.s.uVex3rdReg         = 0;
    pVCpu->iem.s.uVexLength         = 0;
    pVCpu->iem.s.fEvexStuff         = 0;
    pVCpu->iem.s.iEffSeg            = X86_SREG_DS;
#ifdef IEM_WITH_CODE_TLB
    if (pVCpu->iem.s.pbInstrBuf)
    {
        uint64_t off = (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT ? pCtx->rip : pCtx->eip + (uint32_t)pCtx->cs.u64Base)
                     - pVCpu->iem.s.uInstrBufPc;
        if (off < pVCpu->iem.s.cbInstrBufTotal)
        {
            pVCpu->iem.s.offInstrNextByte = (uint32_t)off;
            pVCpu->iem.s.offCurInstrStart = (uint16_t)off;
            if ((uint16_t)off + 15 <= pVCpu->iem.s.cbInstrBufTotal)
                pVCpu->iem.s.cbInstrBuf = (uint16_t)off + 15;
            else
                pVCpu->iem.s.cbInstrBuf = pVCpu->iem.s.cbInstrBufTotal;
        }
        else
        {
            pVCpu->iem.s.pbInstrBuf       = NULL;
            pVCpu->iem.s.offInstrNextByte = 0;
            pVCpu->iem.s.offCurInstrStart = 0;
            pVCpu->iem.s.cbInstrBuf       = 0;
            pVCpu->iem.s.cbInstrBufTotal  = 0;
        }
    }
    else
    {
        pVCpu->iem.s.offInstrNextByte = 0;
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.cbInstrBuf       = 0;
        pVCpu->iem.s.cbInstrBufTotal  = 0;
    }
#else
    pVCpu->iem.s.cbOpcode           = 0;
    pVCpu->iem.s.offOpcode          = 0;
#endif
    Assert(pVCpu->iem.s.cActiveMappings == 0);
    pVCpu->iem.s.iNextMapping       = 0;
    Assert(pVCpu->iem.s.rcPassUp   == VINF_SUCCESS);
    Assert(pVCpu->iem.s.fBypassHandlers == false);
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!pVCpu->iem.s.fInPatchCode)
    { /* likely */ }
    else
    {
        pVCpu->iem.s.fInPatchCode   = pVCpu->iem.s.uCpl == 0
                               && pCtx->cs.u64Base == 0
                               && pCtx->cs.u32Limit == UINT32_MAX
                               && PATMIsPatchGCAddr(pVCpu->CTX_SUFF(pVM), pCtx->eip);
        if (!pVCpu->iem.s.fInPatchCode)
            CPUMRawLeave(pVCpu, VINF_SUCCESS);
    }
#endif

#ifdef DBGFTRACE_ENABLED
    switch (enmMode)
    {
        case IEMMODE_64BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I64/%u %08llx", pVCpu->iem.s.uCpl, pCtx->rip);
            break;
        case IEMMODE_32BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I32/%u %04x:%08x", pVCpu->iem.s.uCpl, pCtx->cs.Sel, pCtx->eip);
            break;
        case IEMMODE_16BIT:
            RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "I16/%u %04x:%04x", pVCpu->iem.s.uCpl, pCtx->cs.Sel, pCtx->eip);
            break;
    }
#endif
}



/**
 * Prefetch opcodes the first time when starting executing.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fBypassHandlers     Whether to bypass access handlers.
 */
IEM_STATIC VBOXSTRICTRC iemInitDecoderAndPrefetchOpcodes(PVMCPU pVCpu, bool fBypassHandlers)
{
#ifdef IEM_VERIFICATION_MODE_FULL
    uint8_t const cbOldOpcodes = pVCpu->iem.s.cbOpcode;
#endif
    iemInitDecoder(pVCpu, fBypassHandlers);

#ifdef IEM_WITH_CODE_TLB
    /** @todo Do ITLB lookup here. */

#else /* !IEM_WITH_CODE_TLB */

    /*
     * What we're doing here is very similar to iemMemMap/iemMemBounceBufferMap.
     *
     * First translate CS:rIP to a physical address.
     */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    uint32_t    cbToTryRead;
    RTGCPTR     GCPtrPC;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        cbToTryRead = PAGE_SIZE;
        GCPtrPC     = pCtx->rip;
        if (IEM_IS_CANONICAL(GCPtrPC))
            cbToTryRead = PAGE_SIZE - (GCPtrPC & PAGE_OFFSET_MASK);
        else
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    else
    {
        uint32_t GCPtrPC32 = pCtx->eip;
        AssertMsg(!(GCPtrPC32 & ~(uint32_t)UINT16_MAX) || pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT, ("%04x:%RX64\n", pCtx->cs.Sel, pCtx->rip));
        if (GCPtrPC32 <= pCtx->cs.u32Limit)
            cbToTryRead = pCtx->cs.u32Limit - GCPtrPC32 + 1;
        else
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        if (cbToTryRead) { /* likely */ }
        else /* overflowed */
        {
            Assert(GCPtrPC32 == 0); Assert(pCtx->cs.u32Limit == UINT32_MAX);
            cbToTryRead = UINT32_MAX;
        }
        GCPtrPC = (uint32_t)pCtx->cs.u64Base + GCPtrPC32;
        Assert(GCPtrPC <= UINT32_MAX);
    }

# ifdef VBOX_WITH_RAW_MODE_NOT_R0
    /* Allow interpretation of patch manager code blocks since they can for
       instance throw #PFs for perfectly good reasons. */
    if (pVCpu->iem.s.fInPatchCode)
    {
        size_t cbRead = 0;
        int rc = PATMReadPatchCode(pVCpu->CTX_SUFF(pVM), GCPtrPC, pVCpu->iem.s.abOpcode, sizeof(pVCpu->iem.s.abOpcode), &cbRead);
        AssertRCReturn(rc, rc);
        pVCpu->iem.s.cbOpcode = (uint8_t)cbRead; Assert(pVCpu->iem.s.cbOpcode == cbRead); Assert(cbRead > 0);
        return VINF_SUCCESS;
    }
# endif /* VBOX_WITH_RAW_MODE_NOT_R0 */

    RTGCPHYS    GCPhys;
    uint64_t    fFlags;
    int rc = PGMGstGetPage(pVCpu, GCPtrPC, &fFlags, &GCPhys);
    if (RT_SUCCESS(rc)) { /* probable */ }
    else
    {
        Log(("iemInitDecoderAndPrefetchOpcodes: %RGv - rc=%Rrc\n", GCPtrPC, rc));
        return iemRaisePageFault(pVCpu, GCPtrPC, IEM_ACCESS_INSTRUCTION, rc);
    }
    if ((fFlags & X86_PTE_US) || pVCpu->iem.s.uCpl != 3) { /* likely */ }
    else
    {
        Log(("iemInitDecoderAndPrefetchOpcodes: %RGv - supervisor page\n", GCPtrPC));
        return iemRaisePageFault(pVCpu, GCPtrPC, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    if (!(fFlags & X86_PTE_PAE_NX) || !(pCtx->msrEFER & MSR_K6_EFER_NXE)) { /* likely */ }
    else
    {
        Log(("iemInitDecoderAndPrefetchOpcodes: %RGv - NX\n", GCPtrPC));
        return iemRaisePageFault(pVCpu, GCPtrPC, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    GCPhys |= GCPtrPC & PAGE_OFFSET_MASK;
    /** @todo Check reserved bits and such stuff. PGM is better at doing
     *        that, so do it when implementing the guest virtual address
     *        TLB... */

# ifdef IEM_VERIFICATION_MODE_FULL
    /*
     * Optimistic optimization: Use unconsumed opcode bytes from the previous
     *                          instruction.
     */
    /** @todo optimize this differently by not using PGMPhysRead. */
    RTGCPHYS const offPrevOpcodes = GCPhys - pVCpu->iem.s.GCPhysOpcodes;
    pVCpu->iem.s.GCPhysOpcodes = GCPhys;
    if (   offPrevOpcodes < cbOldOpcodes
        && PAGE_SIZE - (GCPhys & PAGE_OFFSET_MASK) > sizeof(pVCpu->iem.s.abOpcode))
    {
        uint8_t cbNew = cbOldOpcodes - (uint8_t)offPrevOpcodes;
        Assert(cbNew <= RT_ELEMENTS(pVCpu->iem.s.abOpcode));
        memmove(&pVCpu->iem.s.abOpcode[0], &pVCpu->iem.s.abOpcode[offPrevOpcodes], cbNew);
        pVCpu->iem.s.cbOpcode = cbNew;
        return VINF_SUCCESS;
    }
# endif

    /*
     * Read the bytes at this address.
     */
    PVM pVM = pVCpu->CTX_SUFF(pVM);
# if defined(IN_RING3) && defined(VBOX_WITH_RAW_MODE_NOT_R0)
    size_t cbActual;
    if (   PATMIsEnabled(pVM)
        && RT_SUCCESS(PATMR3ReadOrgInstr(pVM, GCPtrPC, pVCpu->iem.s.abOpcode, sizeof(pVCpu->iem.s.abOpcode), &cbActual)))
    {
        Log4(("decode - Read %u unpatched bytes at %RGv\n", cbActual, GCPtrPC));
        Assert(cbActual > 0);
        pVCpu->iem.s.cbOpcode = (uint8_t)cbActual;
    }
    else
# endif
    {
        uint32_t cbLeftOnPage = PAGE_SIZE - (GCPtrPC & PAGE_OFFSET_MASK);
        if (cbToTryRead > cbLeftOnPage)
            cbToTryRead = cbLeftOnPage;
        if (cbToTryRead > sizeof(pVCpu->iem.s.abOpcode))
            cbToTryRead = sizeof(pVCpu->iem.s.abOpcode);

        if (!pVCpu->iem.s.fBypassHandlers)
        {
            VBOXSTRICTRC rcStrict = PGMPhysRead(pVM, GCPhys, pVCpu->iem.s.abOpcode, cbToTryRead, PGMACCESSORIGIN_IEM);
            if (RT_LIKELY(rcStrict == VINF_SUCCESS))
            { /* likely */ }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                Log(("iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                     GCPtrPC, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
            }
            else
            {
                Log((RT_SUCCESS(rcStrict)
                     ? "iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                     : "iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                     GCPtrPC, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
                return rcStrict;
            }
        }
        else
        {
            rc = PGMPhysSimpleReadGCPhys(pVM, pVCpu->iem.s.abOpcode, GCPhys, cbToTryRead);
            if (RT_SUCCESS(rc))
            { /* likely */ }
            else
            {
                Log(("iemInitDecoderAndPrefetchOpcodes: %RGv/%RGp LB %#x - read error - rc=%Rrc (!!)\n",
                     GCPtrPC, GCPhys, rc, cbToTryRead));
                return rc;
            }
        }
        pVCpu->iem.s.cbOpcode = cbToTryRead;
    }
#endif /* !IEM_WITH_CODE_TLB */
    return VINF_SUCCESS;
}


/**
 * Invalidates the IEM TLBs.
 *
 * This is called internally as well as by PGM when moving GC mappings.
 *
 * @returns
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      thread.
 * @param   fVmm        Set when PGM calls us with a remapping.
 */
VMM_INT_DECL(void) IEMTlbInvalidateAll(PVMCPU pVCpu, bool fVmm)
{
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.cbInstrBufTotal = 0;
    pVCpu->iem.s.CodeTlb.uTlbRevision += IEMTLB_REVISION_INCR;
    if (pVCpu->iem.s.CodeTlb.uTlbRevision != 0)
    { /* very likely */ }
    else
    {
        pVCpu->iem.s.CodeTlb.uTlbRevision = IEMTLB_REVISION_INCR;
        unsigned i = RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries);
        while (i-- > 0)
            pVCpu->iem.s.CodeTlb.aEntries[i].uTag = 0;
    }
#endif

#ifdef IEM_WITH_DATA_TLB
    pVCpu->iem.s.DataTlb.uTlbRevision += IEMTLB_REVISION_INCR;
    if (pVCpu->iem.s.DataTlb.uTlbRevision != 0)
    { /* very likely */ }
    else
    {
        pVCpu->iem.s.DataTlb.uTlbRevision = IEMTLB_REVISION_INCR;
        unsigned i = RT_ELEMENTS(pVCpu->iem.s.DataTlb.aEntries);
        while (i-- > 0)
            pVCpu->iem.s.DataTlb.aEntries[i].uTag = 0;
    }
#endif
    NOREF(pVCpu); NOREF(fVmm);
}


/**
 * Invalidates a page in the TLBs.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      thread.
 * @param   GCPtr       The address of the page to invalidate
 */
VMM_INT_DECL(void) IEMTlbInvalidatePage(PVMCPU pVCpu, RTGCPTR GCPtr)
{
#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
    GCPtr = GCPtr >> X86_PAGE_SHIFT;
    AssertCompile(RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries) == 256);
    AssertCompile(RT_ELEMENTS(pVCpu->iem.s.DataTlb.aEntries) == 256);
    uintptr_t idx = (uint8_t)GCPtr;

# ifdef IEM_WITH_CODE_TLB
    if (pVCpu->iem.s.CodeTlb.aEntries[idx].uTag == (GCPtr | pVCpu->iem.s.CodeTlb.uTlbRevision))
    {
        pVCpu->iem.s.CodeTlb.aEntries[idx].uTag = 0;
        if (GCPtr == (pVCpu->iem.s.uInstrBufPc >> X86_PAGE_SHIFT))
            pVCpu->iem.s.cbInstrBufTotal = 0;
    }
# endif

# ifdef IEM_WITH_DATA_TLB
    if (pVCpu->iem.s.DataTlb.aEntries[idx].uTag == (GCPtr | pVCpu->iem.s.DataTlb.uTlbRevision))
        pVCpu->iem.s.DataTlb.aEntries[idx].uTag = 0;
# endif
#else
    NOREF(pVCpu); NOREF(GCPtr);
#endif
}


/**
 * Invalidates the host physical aspects of the IEM TLBs.
 *
 * This is called internally as well as by PGM when moving GC mappings.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling
 *                      thread.
 */
VMM_INT_DECL(void) IEMTlbInvalidateAllPhysical(PVMCPU pVCpu)
{
#if defined(IEM_WITH_CODE_TLB) || defined(IEM_WITH_DATA_TLB)
    /* Note! This probably won't end up looking exactly like this, but it give an idea... */

# ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.cbInstrBufTotal = 0;
# endif
    uint64_t uTlbPhysRev = pVCpu->iem.s.CodeTlb.uTlbPhysRev + IEMTLB_PHYS_REV_INCR;
    if (uTlbPhysRev != 0)
    {
        pVCpu->iem.s.CodeTlb.uTlbPhysRev = uTlbPhysRev;
        pVCpu->iem.s.DataTlb.uTlbPhysRev = uTlbPhysRev;
    }
    else
    {
        pVCpu->iem.s.CodeTlb.uTlbPhysRev = IEMTLB_PHYS_REV_INCR;
        pVCpu->iem.s.DataTlb.uTlbPhysRev = IEMTLB_PHYS_REV_INCR;

        unsigned i;
# ifdef IEM_WITH_CODE_TLB
        i = RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries);
        while (i-- > 0)
        {
            pVCpu->iem.s.CodeTlb.aEntries[i].pbMappingR3       = NULL;
            pVCpu->iem.s.CodeTlb.aEntries[i].fFlagsAndPhysRev &= ~(IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_NO_READ | IEMTLBE_F_PHYS_REV);
        }
# endif
# ifdef IEM_WITH_DATA_TLB
        i = RT_ELEMENTS(pVCpu->iem.s.DataTlb.aEntries);
        while (i-- > 0)
        {
            pVCpu->iem.s.DataTlb.aEntries[i].pbMappingR3       = NULL;
            pVCpu->iem.s.DataTlb.aEntries[i].fFlagsAndPhysRev &= ~(IEMTLBE_F_PG_NO_WRITE | IEMTLBE_F_PG_NO_READ | IEMTLBE_F_PHYS_REV);
        }
# endif
    }
#else
    NOREF(pVCpu);
#endif
}


/**
 * Invalidates the host physical aspects of the IEM TLBs.
 *
 * This is called internally as well as by PGM when moving GC mappings.
 *
 * @param   pVM         The cross context VM structure.
 *
 * @remarks Caller holds the PGM lock.
 */
VMM_INT_DECL(void) IEMTlbInvalidateAllPhysicalAllCpus(PVM pVM)
{
    RT_NOREF_PV(pVM);
}

#ifdef IEM_WITH_CODE_TLB

/**
 * Tries to fetches @a cbDst opcode bytes, raise the appropriate exception on
 * failure and jumps.
 *
 * We end up here for a number of reasons:
 *      - pbInstrBuf isn't yet initialized.
 *      - Advancing beyond the buffer boundrary (e.g. cross page).
 *      - Advancing beyond the CS segment limit.
 *      - Fetching from non-mappable page (e.g. MMIO).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pvDst               Where to return the bytes.
 * @param   cbDst               Number of bytes to read.
 *
 * @todo    Make cbDst = 0 a way of initializing pbInstrBuf?
 */
IEM_STATIC void iemOpcodeFetchBytesJmp(PVMCPU pVCpu, size_t cbDst, void *pvDst)
{
#ifdef IN_RING3
//__debugbreak();
    for (;;)
    {
        Assert(cbDst <= 8);
        uint32_t offBuf = pVCpu->iem.s.offInstrNextByte;

        /*
         * We might have a partial buffer match, deal with that first to make the
         * rest simpler.  This is the first part of the cross page/buffer case.
         */
        if (pVCpu->iem.s.pbInstrBuf != NULL)
        {
            if (offBuf < pVCpu->iem.s.cbInstrBuf)
            {
                Assert(offBuf + cbDst > pVCpu->iem.s.cbInstrBuf);
                uint32_t const cbCopy = pVCpu->iem.s.cbInstrBuf - pVCpu->iem.s.offInstrNextByte;
                memcpy(pvDst, &pVCpu->iem.s.pbInstrBuf[offBuf], cbCopy);

                cbDst  -= cbCopy;
                pvDst   = (uint8_t *)pvDst + cbCopy;
                offBuf += cbCopy;
                pVCpu->iem.s.offInstrNextByte += offBuf;
            }
        }

        /*
         * Check segment limit, figuring how much we're allowed to access at this point.
         *
         * We will fault immediately if RIP is past the segment limit / in non-canonical
         * territory.  If we do continue, there are one or more bytes to read before we
         * end up in trouble and we need to do that first before faulting.
         */
        PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
        RTGCPTR  GCPtrFirst;
        uint32_t cbMaxRead;
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        {
            GCPtrFirst = pCtx->rip + (offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart);
            if (RT_LIKELY(IEM_IS_CANONICAL(GCPtrFirst)))
            { /* likely */ }
            else
                iemRaiseGeneralProtectionFault0Jmp(pVCpu);
            cbMaxRead = X86_PAGE_SIZE - ((uint32_t)GCPtrFirst & X86_PAGE_OFFSET_MASK);
        }
        else
        {
            GCPtrFirst = pCtx->eip + (offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart);
            Assert(!(GCPtrFirst & ~(uint32_t)UINT16_MAX) || pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT);
            if (RT_LIKELY((uint32_t)GCPtrFirst <= pCtx->cs.u32Limit))
            { /* likely */ }
            else
                iemRaiseSelectorBoundsJmp(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
            cbMaxRead = pCtx->cs.u32Limit - (uint32_t)GCPtrFirst + 1;
            if (cbMaxRead != 0)
            { /* likely */ }
            else
            {
                /* Overflowed because address is 0 and limit is max. */
                Assert(GCPtrFirst == 0); Assert(pCtx->cs.u32Limit == UINT32_MAX);
                cbMaxRead = X86_PAGE_SIZE;
            }
            GCPtrFirst = (uint32_t)GCPtrFirst + (uint32_t)pCtx->cs.u64Base;
            uint32_t cbMaxRead2 = X86_PAGE_SIZE - ((uint32_t)GCPtrFirst & X86_PAGE_OFFSET_MASK);
            if (cbMaxRead2 < cbMaxRead)
                cbMaxRead = cbMaxRead2;
            /** @todo testcase: unreal modes, both huge 16-bit and 32-bit. */
        }

        /*
         * Get the TLB entry for this piece of code.
         */
        uint64_t     uTag  = (GCPtrFirst >> X86_PAGE_SHIFT) | pVCpu->iem.s.CodeTlb.uTlbRevision;
        AssertCompile(RT_ELEMENTS(pVCpu->iem.s.CodeTlb.aEntries) == 256);
        PIEMTLBENTRY pTlbe = &pVCpu->iem.s.CodeTlb.aEntries[(uint8_t)uTag];
        if (pTlbe->uTag == uTag)
        {
            /* likely when executing lots of code, otherwise unlikely */
# ifdef VBOX_WITH_STATISTICS
            pVCpu->iem.s.CodeTlb.cTlbHits++;
# endif
        }
        else
        {
            pVCpu->iem.s.CodeTlb.cTlbMisses++;
# ifdef VBOX_WITH_RAW_MODE_NOT_R0
            if (PATMIsPatchGCAddr(pVCpu->CTX_SUFF(pVM), pCtx->eip))
            {
                pTlbe->uTag             = uTag;
                pTlbe->fFlagsAndPhysRev = IEMTLBE_F_PATCH_CODE  | IEMTLBE_F_PT_NO_WRITE | IEMTLBE_F_PT_NO_USER
                                        | IEMTLBE_F_PT_NO_WRITE | IEMTLBE_F_PT_NO_DIRTY | IEMTLBE_F_NO_MAPPINGR3;
                pTlbe->GCPhys           = NIL_RTGCPHYS;
                pTlbe->pbMappingR3      = NULL;
            }
            else
# endif
            {
                RTGCPHYS    GCPhys;
                uint64_t    fFlags;
                int rc = PGMGstGetPage(pVCpu, GCPtrFirst, &fFlags, &GCPhys);
                if (RT_FAILURE(rc))
                {
                    Log(("iemOpcodeFetchMoreBytes: %RGv - rc=%Rrc\n", GCPtrFirst, rc));
                    iemRaisePageFaultJmp(pVCpu, GCPtrFirst, IEM_ACCESS_INSTRUCTION, rc);
                }

                AssertCompile(IEMTLBE_F_PT_NO_EXEC == 1);
                pTlbe->uTag             = uTag;
                pTlbe->fFlagsAndPhysRev = (~fFlags & (X86_PTE_US | X86_PTE_RW | X86_PTE_D)) | (fFlags >> X86_PTE_PAE_BIT_NX);
                pTlbe->GCPhys           = GCPhys;
                pTlbe->pbMappingR3      = NULL;
            }
        }

        /*
         * Check TLB page table level access flags.
         */
        if (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PT_NO_USER | IEMTLBE_F_PT_NO_EXEC))
        {
            if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_USER) && pVCpu->iem.s.uCpl == 3)
            {
                Log(("iemOpcodeFetchBytesJmp: %RGv - supervisor page\n", GCPtrFirst));
                iemRaisePageFaultJmp(pVCpu, GCPtrFirst, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
            }
            if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PT_NO_EXEC) && (pCtx->msrEFER & MSR_K6_EFER_NXE))
            {
                Log(("iemOpcodeFetchMoreBytes: %RGv - NX\n", GCPtrFirst));
                iemRaisePageFaultJmp(pVCpu, GCPtrFirst, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
            }
        }

# ifdef VBOX_WITH_RAW_MODE_NOT_R0
        /*
         * Allow interpretation of patch manager code blocks since they can for
         * instance throw #PFs for perfectly good reasons.
         */
        if (!(pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PATCH_CODE))
        { /* no unlikely */ }
        else
        {
            /** @todo Could be optimized this a little in ring-3 if we liked. */
            size_t cbRead = 0;
            int rc = PATMReadPatchCode(pVCpu->CTX_SUFF(pVM), GCPtrFirst, pvDst, cbDst, &cbRead);
            AssertRCStmt(rc, longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), rc));
            AssertStmt(cbRead == cbDst, longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VERR_IEM_IPE_1));
            return;
        }
# endif /* VBOX_WITH_RAW_MODE_NOT_R0 */

        /*
         * Look up the physical page info if necessary.
         */
        if ((pTlbe->fFlagsAndPhysRev & IEMTLBE_F_PHYS_REV) == pVCpu->iem.s.CodeTlb.uTlbPhysRev)
        { /* not necessary */ }
        else
        {
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_WRITE     == IEMTLBE_F_PG_NO_WRITE);
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_READ      == IEMTLBE_F_PG_NO_READ);
            AssertCompile(PGMIEMGCPHYS2PTR_F_NO_MAPPINGR3 == IEMTLBE_F_NO_MAPPINGR3);
            pTlbe->fFlagsAndPhysRev &= ~(  IEMTLBE_F_PHYS_REV
                                         | IEMTLBE_F_NO_MAPPINGR3 | IEMTLBE_F_PG_NO_READ | IEMTLBE_F_PG_NO_WRITE);
            int rc = PGMPhysIemGCPhys2PtrNoLock(pVCpu->CTX_SUFF(pVM), pVCpu, pTlbe->GCPhys, &pVCpu->iem.s.CodeTlb.uTlbPhysRev,
                                                &pTlbe->pbMappingR3, &pTlbe->fFlagsAndPhysRev);
            AssertRCStmt(rc, longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), rc));
        }

# if defined(IN_RING3) || (defined(IN_RING0) && !defined(VBOX_WITH_2X_4GB_ADDR_SPACE))
        /*
         * Try do a direct read using the pbMappingR3 pointer.
         */
        if (    (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PHYS_REV | IEMTLBE_F_NO_MAPPINGR3 | IEMTLBE_F_PG_NO_READ))
             == pVCpu->iem.s.CodeTlb.uTlbPhysRev)
        {
            uint32_t const offPg = (GCPtrFirst & X86_PAGE_OFFSET_MASK);
            pVCpu->iem.s.cbInstrBufTotal  = offPg + cbMaxRead;
            if (offBuf == (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart)
            {
                pVCpu->iem.s.cbInstrBuf       = offPg + RT_MIN(15, cbMaxRead);
                pVCpu->iem.s.offCurInstrStart = (int16_t)offPg;
            }
            else
            {
                uint32_t const cbInstr = offBuf - (uint32_t)(int32_t)pVCpu->iem.s.offCurInstrStart;
                Assert(cbInstr < cbMaxRead);
                pVCpu->iem.s.cbInstrBuf       = offPg + RT_MIN(cbMaxRead + cbInstr, 15) - cbInstr;
                pVCpu->iem.s.offCurInstrStart = (int16_t)(offPg - cbInstr);
            }
            if (cbDst <= cbMaxRead)
            {
                pVCpu->iem.s.offInstrNextByte = offPg + (uint32_t)cbDst;
                pVCpu->iem.s.uInstrBufPc      = GCPtrFirst & ~(RTGCPTR)X86_PAGE_OFFSET_MASK;
                pVCpu->iem.s.pbInstrBuf       = pTlbe->pbMappingR3;
                memcpy(pvDst, &pTlbe->pbMappingR3[offPg], cbDst);
                return;
            }
            pVCpu->iem.s.pbInstrBuf = NULL;

            memcpy(pvDst, &pTlbe->pbMappingR3[offPg], cbMaxRead);
            pVCpu->iem.s.offInstrNextByte = offPg + cbMaxRead;
        }
        else
# endif
#if 0
        /*
         * If there is no special read handling, so we can read a bit more and
         * put it in the prefetch buffer.
         */
        if (   cbDst < cbMaxRead
            && (pTlbe->fFlagsAndPhysRev & (IEMTLBE_F_PHYS_REV | IEMTLBE_F_PG_NO_READ)) == pVCpu->iem.s.CodeTlb.uTlbPhysRev)
        {
            VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), pTlbe->GCPhys,
                                                &pVCpu->iem.s.abOpcode[0], cbToTryRead, PGMACCESSORIGIN_IEM);
            if (RT_LIKELY(rcStrict == VINF_SUCCESS))
            { /* likely */ }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                Log(("iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                     GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                AssertStmt(rcStrict == VINF_SUCCESS, longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VBOXSTRICRC_VAL(rcStrict)));
            }
            else
            {
                Log((RT_SUCCESS(rcStrict)
                     ? "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                     : "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                     GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
                longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
            }
        }
        /*
         * Special read handling, so only read exactly what's needed.
         * This is a highly unlikely scenario.
         */
        else
#endif
        {
            pVCpu->iem.s.CodeTlb.cTlbSlowReadPath++;
            uint32_t const cbToRead = RT_MIN((uint32_t)cbDst, cbMaxRead);
            VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), pTlbe->GCPhys + (GCPtrFirst & X86_PAGE_OFFSET_MASK),
                                                pvDst, cbToRead, PGMACCESSORIGIN_IEM);
            if (RT_LIKELY(rcStrict == VINF_SUCCESS))
            { /* likely */ }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                Log(("iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                     GCPtrFirst, pTlbe->GCPhys + (GCPtrFirst & X86_PAGE_OFFSET_MASK), VBOXSTRICTRC_VAL(rcStrict), cbToRead));
                rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                AssertStmt(rcStrict == VINF_SUCCESS, longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VBOXSTRICTRC_VAL(rcStrict)));
            }
            else
            {
                Log((RT_SUCCESS(rcStrict)
                     ? "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                     : "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                     GCPtrFirst, pTlbe->GCPhys + (GCPtrFirst & X86_PAGE_OFFSET_MASK), VBOXSTRICTRC_VAL(rcStrict), cbToRead));
                longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
            }
            pVCpu->iem.s.offInstrNextByte = offBuf + cbToRead;
            if (cbToRead == cbDst)
                return;
        }

        /*
         * More to read, loop.
         */
        cbDst -= cbMaxRead;
        pvDst  = (uint8_t *)pvDst + cbMaxRead;
    }
#else
    RT_NOREF(pvDst, cbDst);
    longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VERR_INTERNAL_ERROR);
#endif
}

#else

/**
 * Try fetch at least @a cbMin bytes more opcodes, raise the appropriate
 * exception if it fails.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   cbMin               The minimum number of bytes relative offOpcode
 *                              that must be read.
 */
IEM_STATIC VBOXSTRICTRC iemOpcodeFetchMoreBytes(PVMCPU pVCpu, size_t cbMin)
{
    /*
     * What we're doing here is very similar to iemMemMap/iemMemBounceBufferMap.
     *
     * First translate CS:rIP to a physical address.
     */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    uint8_t     cbLeft = pVCpu->iem.s.cbOpcode - pVCpu->iem.s.offOpcode; Assert(cbLeft < cbMin);
    uint32_t    cbToTryRead;
    RTGCPTR     GCPtrNext;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        cbToTryRead = PAGE_SIZE;
        GCPtrNext   = pCtx->rip + pVCpu->iem.s.cbOpcode;
        if (!IEM_IS_CANONICAL(GCPtrNext))
            return iemRaiseGeneralProtectionFault0(pVCpu);
    }
    else
    {
        uint32_t GCPtrNext32 = pCtx->eip;
        Assert(!(GCPtrNext32 & ~(uint32_t)UINT16_MAX) || pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT);
        GCPtrNext32 += pVCpu->iem.s.cbOpcode;
        if (GCPtrNext32 > pCtx->cs.u32Limit)
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        cbToTryRead = pCtx->cs.u32Limit - GCPtrNext32 + 1;
        if (!cbToTryRead) /* overflowed */
        {
            Assert(GCPtrNext32 == 0); Assert(pCtx->cs.u32Limit == UINT32_MAX);
            cbToTryRead = UINT32_MAX;
            /** @todo check out wrapping around the code segment.  */
        }
        if (cbToTryRead < cbMin - cbLeft)
            return iemRaiseSelectorBounds(pVCpu, X86_SREG_CS, IEM_ACCESS_INSTRUCTION);
        GCPtrNext = (uint32_t)pCtx->cs.u64Base + GCPtrNext32;
    }

    /* Only read up to the end of the page, and make sure we don't read more
       than the opcode buffer can hold. */
    uint32_t cbLeftOnPage = PAGE_SIZE - (GCPtrNext & PAGE_OFFSET_MASK);
    if (cbToTryRead > cbLeftOnPage)
        cbToTryRead = cbLeftOnPage;
    if (cbToTryRead > sizeof(pVCpu->iem.s.abOpcode) - pVCpu->iem.s.cbOpcode)
        cbToTryRead = sizeof(pVCpu->iem.s.abOpcode) - pVCpu->iem.s.cbOpcode;
/** @todo r=bird: Convert assertion into undefined opcode exception? */
    Assert(cbToTryRead >= cbMin - cbLeft); /* ASSUMPTION based on iemInitDecoderAndPrefetchOpcodes. */

# ifdef VBOX_WITH_RAW_MODE_NOT_R0
    /* Allow interpretation of patch manager code blocks since they can for
       instance throw #PFs for perfectly good reasons. */
    if (pVCpu->iem.s.fInPatchCode)
    {
        size_t cbRead = 0;
        int rc = PATMReadPatchCode(pVCpu->CTX_SUFF(pVM), GCPtrNext, pVCpu->iem.s.abOpcode, cbToTryRead, &cbRead);
        AssertRCReturn(rc, rc);
        pVCpu->iem.s.cbOpcode = (uint8_t)cbRead; Assert(pVCpu->iem.s.cbOpcode == cbRead); Assert(cbRead > 0);
        return VINF_SUCCESS;
    }
# endif /* VBOX_WITH_RAW_MODE_NOT_R0 */

    RTGCPHYS    GCPhys;
    uint64_t    fFlags;
    int rc = PGMGstGetPage(pVCpu, GCPtrNext, &fFlags, &GCPhys);
    if (RT_FAILURE(rc))
    {
        Log(("iemOpcodeFetchMoreBytes: %RGv - rc=%Rrc\n", GCPtrNext, rc));
        return iemRaisePageFault(pVCpu, GCPtrNext, IEM_ACCESS_INSTRUCTION, rc);
    }
    if (!(fFlags & X86_PTE_US) && pVCpu->iem.s.uCpl == 3)
    {
        Log(("iemOpcodeFetchMoreBytes: %RGv - supervisor page\n", GCPtrNext));
        return iemRaisePageFault(pVCpu, GCPtrNext, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    if ((fFlags & X86_PTE_PAE_NX) && (pCtx->msrEFER & MSR_K6_EFER_NXE))
    {
        Log(("iemOpcodeFetchMoreBytes: %RGv - NX\n", GCPtrNext));
        return iemRaisePageFault(pVCpu, GCPtrNext, IEM_ACCESS_INSTRUCTION, VERR_ACCESS_DENIED);
    }
    GCPhys |= GCPtrNext & PAGE_OFFSET_MASK;
    Log5(("GCPtrNext=%RGv GCPhys=%RGp cbOpcodes=%#x\n",  GCPtrNext,  GCPhys,  pVCpu->iem.s.cbOpcode));
    /** @todo Check reserved bits and such stuff. PGM is better at doing
     *        that, so do it when implementing the guest virtual address
     *        TLB... */

    /*
     * Read the bytes at this address.
     *
     * We read all unpatched bytes in iemInitDecoderAndPrefetchOpcodes already,
     * and since PATM should only patch the start of an instruction there
     * should be no need to check again here.
     */
    if (!pVCpu->iem.s.fBypassHandlers)
    {
        VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), GCPhys, &pVCpu->iem.s.abOpcode[pVCpu->iem.s.cbOpcode],
                                            cbToTryRead, PGMACCESSORIGIN_IEM);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        { /* likely */ }
        else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
        {
            Log(("iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status -  rcStrict=%Rrc\n",
                 GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
            rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
        }
        else
        {
            Log((RT_SUCCESS(rcStrict)
                 ? "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read status - rcStrict=%Rrc\n"
                 : "iemOpcodeFetchMoreBytes: %RGv/%RGp LB %#x - read error - rcStrict=%Rrc (!!)\n",
                 GCPtrNext, GCPhys, VBOXSTRICTRC_VAL(rcStrict), cbToTryRead));
            return rcStrict;
        }
    }
    else
    {
        rc = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.abOpcode[pVCpu->iem.s.cbOpcode], GCPhys, cbToTryRead);
        if (RT_SUCCESS(rc))
        { /* likely */ }
        else
        {
            Log(("iemOpcodeFetchMoreBytes: %RGv - read error - rc=%Rrc (!!)\n", GCPtrNext, rc));
            return rc;
        }
    }
    pVCpu->iem.s.cbOpcode += cbToTryRead;
    Log5(("%.*Rhxs\n", pVCpu->iem.s.cbOpcode, pVCpu->iem.s.abOpcode));

    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_CODE_TLB */
#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU8 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pb                  Where to return the opcode byte.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU8Slow(PVMCPU pVCpu, uint8_t *pb)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 1);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pb = pVCpu->iem.s.abOpcode[offOpcode];
        pVCpu->iem.s.offOpcode = offOpcode + 1;
    }
    else
        *pb = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   pu8                 Where to return the opcode byte.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU8(PVMCPU pVCpu, uint8_t *pu8)
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        *pu8 = pVCpu->iem.s.abOpcode[offOpcode];
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU8Slow(pVCpu, pu8);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU8Jmp doesn't like, longjmp on error.
 *
 * @returns The opcode byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_NO_INLINE(IEM_STATIC, uint8_t) iemOpcodeGetNextU8SlowJmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uint8_t u8;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u8), &u8);
    return u8;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 1);
    if (rcStrict == VINF_SUCCESS)
        return pVCpu->iem.s.abOpcode[pVCpu->iem.s.offOpcode++];
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
# endif
}


/**
 * Fetches the next opcode byte, longjmp on error.
 *
 * @returns The opcode byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(uint8_t) iemOpcodeGetNextU8Jmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf < pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 1;
        return pbBuf[offBuf];
    }
# else
    uintptr_t offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode < pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 1;
        return pVCpu->iem.s.abOpcode[offOpcode];
    }
# endif
    return iemOpcodeGetNextU8SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode byte, returns automatically on failure.
 *
 * @param   a_pu8               Where to return the opcode byte.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U8(a_pu8) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU8(pVCpu, (a_pu8)); \
        if (rcStrict2 == VINF_SUCCESS) \
        { /* likely */ } \
        else \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U8(a_pu8) (*(a_pu8) = iemOpcodeGetNextU8Jmp(pVCpu))
#endif /* IEM_WITH_SETJMP */


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed byte from the opcode stream.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pi8                 Where to return the signed byte.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8(PVMCPU pVCpu, int8_t *pi8)
{
    return iemOpcodeGetNextU8(pVCpu, (uint8_t *)pi8);
}
#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next signed byte from the opcode stream, returning automatically
 * on failure.
 *
 * @param   a_pi8               Where to return the signed byte.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8(a_pi8) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8(pVCpu, (a_pi8)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else /* IEM_WITH_SETJMP */
# define IEM_OPCODE_GET_NEXT_S8(a_pi8) (*(a_pi8) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))

#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextS8SxU16 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the opcode dword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextS8SxU16Slow(PVMCPU pVCpu, uint16_t *pu16)
{
    uint8_t      u8;
    VBOXSTRICTRC rcStrict = iemOpcodeGetNextU8Slow(pVCpu, &u8);
    if (rcStrict == VINF_SUCCESS)
        *pu16 = (int8_t)u8;
    return rcStrict;
}


/**
 * Fetches the next signed byte from the opcode stream, extending it to
 * unsigned 16-bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the unsigned word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8SxU16(PVMCPU pVCpu, uint16_t *pu16)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode >= pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS8SxU16Slow(pVCpu, pu16);

    *pu16 = (int8_t)pVCpu->iem.s.abOpcode[offOpcode];
    pVCpu->iem.s.offOpcode = offOpcode + 1;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next signed byte from the opcode stream and sign-extending it to
 * a word, returning automatically on failure.
 *
 * @param   a_pu16              Where to return the word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8_SX_U16(a_pu16) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8SxU16(pVCpu, (a_pu16)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S8_SX_U16(a_pu16) (*(a_pu16) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextS8SxU32 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode dword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextS8SxU32Slow(PVMCPU pVCpu, uint32_t *pu32)
{
    uint8_t      u8;
    VBOXSTRICTRC rcStrict = iemOpcodeGetNextU8Slow(pVCpu, &u8);
    if (rcStrict == VINF_SUCCESS)
        *pu32 = (int8_t)u8;
    return rcStrict;
}


/**
 * Fetches the next signed byte from the opcode stream, extending it to
 * unsigned 32-bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the unsigned dword.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8SxU32(PVMCPU pVCpu, uint32_t *pu32)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode >= pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS8SxU32Slow(pVCpu, pu32);

    *pu32 = (int8_t)pVCpu->iem.s.abOpcode[offOpcode];
    pVCpu->iem.s.offOpcode = offOpcode + 1;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next signed byte from the opcode stream and sign-extending it to
 * a word, returning automatically on failure.
 *
 * @param   a_pu32              Where to return the word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
#define IEM_OPCODE_GET_NEXT_S8_SX_U32(a_pu32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8SxU32(pVCpu, (a_pu32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S8_SX_U32(a_pu32) (*(a_pu32) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextS8SxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextS8SxU64Slow(PVMCPU pVCpu, uint64_t *pu64)
{
    uint8_t      u8;
    VBOXSTRICTRC rcStrict = iemOpcodeGetNextU8Slow(pVCpu, &u8);
    if (rcStrict == VINF_SUCCESS)
        *pu64 = (int8_t)u8;
    return rcStrict;
}


/**
 * Fetches the next signed byte from the opcode stream, extending it to
 * unsigned 64-bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the unsigned qword.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS8SxU64(PVMCPU pVCpu, uint64_t *pu64)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode >= pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS8SxU64Slow(pVCpu, pu64);

    *pu64 = (int8_t)pVCpu->iem.s.abOpcode[offOpcode];
    pVCpu->iem.s.offOpcode = offOpcode + 1;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next signed byte from the opcode stream and sign-extending it to
 * a word, returning automatically on failure.
 *
 * @param   a_pu64              Where to return the word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S8_SX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS8SxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S8_SX_U64(a_pu64) (*(a_pu64) = (int8_t)iemOpcodeGetNextU8Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the opcode word.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU16Slow(PVMCPU pVCpu, uint16_t *pu16)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu16 = *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu16 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
# endif
        pVCpu->iem.s.offOpcode = offOpcode + 2;
    }
    else
        *pu16 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16                Where to return the opcode word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU16(PVMCPU pVCpu, uint16_t *pu16)
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 2 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 2;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu16 = *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu16 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
# endif
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU16Slow(pVCpu, pu16);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16Jmp doesn't like, longjmp on error
 *
 * @returns The opcode word.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_NO_INLINE(IEM_STATIC, uint16_t) iemOpcodeGetNextU16SlowJmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uint16_t u16;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u16), &u16);
    return u16;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        pVCpu->iem.s.offOpcode += 2;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
#  endif
    }
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
# endif
}


/**
 * Fetches the next opcode word, longjmp on error.
 *
 * @returns The opcode word.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(uint16_t) iemOpcodeGetNextU16Jmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf + 2 <= pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 2;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint16_t const *)&pbBuf[offBuf];
#  else
        return RT_MAKE_U16(pbBuf[offBuf], pbBuf[offBuf + 1]);
#  endif
    }
# else
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 2 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 2;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint16_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
#  endif
    }
# endif
    return iemOpcodeGetNextU16SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */


/**
 * Fetches the next opcode word, returns automatically on failure.
 *
 * @param   a_pu16              Where to return the opcode word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U16(a_pu16) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU16(pVCpu, (a_pu16)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U16(a_pu16) (*(a_pu16) = iemOpcodeGetNextU16Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16ZxU32 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode double word.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU16ZxU32Slow(PVMCPU pVCpu, uint32_t *pu32)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu32 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
        pVCpu->iem.s.offOpcode = offOpcode + 2;
    }
    else
        *pu32 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode word, zero extending it to a double word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode double word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU16ZxU32(PVMCPU pVCpu, uint32_t *pu32)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 2 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextU16ZxU32Slow(pVCpu, pu32);

    *pu32 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
    pVCpu->iem.s.offOpcode = offOpcode + 2;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next opcode word and zero extends it to a double word, returns
 * automatically on failure.
 *
 * @param   a_pu32              Where to return the opcode double word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U16_ZX_U32(a_pu32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU16ZxU32(pVCpu, (a_pu32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U16_ZX_U32(a_pu32) (*(a_pu32) = iemOpcodeGetNextU16Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU16ZxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU16ZxU64Slow(PVMCPU pVCpu, uint64_t *pu64)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 2);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu64 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
        pVCpu->iem.s.offOpcode = offOpcode + 2;
    }
    else
        *pu64 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode word, zero extending it to a quad word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU16ZxU64(PVMCPU pVCpu, uint64_t *pu64)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 2 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextU16ZxU64Slow(pVCpu, pu64);

    *pu64 = RT_MAKE_U16(pVCpu->iem.s.abOpcode[offOpcode], pVCpu->iem.s.abOpcode[offOpcode + 1]);
    pVCpu->iem.s.offOpcode = offOpcode + 2;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */

/**
 * Fetches the next opcode word and zero extends it to a quad word, returns
 * automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U16_ZX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU16ZxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U16_ZX_U64(a_pu64)  (*(a_pu64) = iemOpcodeGetNextU16Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed word from the opcode stream.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pi16                Where to return the signed word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS16(PVMCPU pVCpu, int16_t *pi16)
{
    return iemOpcodeGetNextU16(pVCpu, (uint16_t *)pi16);
}
#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next signed word from the opcode stream, returning automatically
 * on failure.
 *
 * @param   a_pi16              Where to return the signed word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S16(a_pi16) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS16(pVCpu, (a_pi16)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S16(a_pi16) (*(a_pi16) = (int16_t)iemOpcodeGetNextU16Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU32 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode dword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU32Slow(PVMCPU pVCpu, uint32_t *pu32)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu32 = *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu32 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3]);
# endif
        pVCpu->iem.s.offOpcode = offOpcode + 4;
    }
    else
        *pu32 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32                Where to return the opcode double word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU32(PVMCPU pVCpu, uint32_t *pu32)
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 4 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 4;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu32 = *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu32 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3]);
# endif
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU32Slow(pVCpu, pu32);
}

#else  /* !IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU32Jmp doesn't like, longjmp on error.
 *
 * @returns The opcode dword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_NO_INLINE(IEM_STATIC, uint32_t) iemOpcodeGetNextU32SlowJmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uint32_t u32;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u32), &u32);
    return u32;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        pVCpu->iem.s.offOpcode = offOpcode + 4;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3]);
#  endif
    }
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
# endif
}


/**
 * Fetches the next opcode dword, longjmp on error.
 *
 * @returns The opcode dword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(uint32_t) iemOpcodeGetNextU32Jmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf + 4 <= pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 4;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint32_t const *)&pbBuf[offBuf];
#  else
        return RT_MAKE_U32_FROM_U8(pbBuf[offBuf],
                                   pbBuf[offBuf + 1],
                                   pbBuf[offBuf + 2],
                                   pbBuf[offBuf + 3]);
#  endif
    }
# else
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 4 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 4;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint32_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3]);
#  endif
    }
# endif
    return iemOpcodeGetNextU32SlowJmp(pVCpu);
}

#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next opcode dword, returns automatically on failure.
 *
 * @param   a_pu32              Where to return the opcode dword.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U32(a_pu32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU32(pVCpu, (a_pu32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U32(a_pu32) (*(a_pu32) = iemOpcodeGetNextU32Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU32ZxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode dword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU32ZxU64Slow(PVMCPU pVCpu, uint64_t *pu64)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu64 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3]);
        pVCpu->iem.s.offOpcode = offOpcode + 4;
    }
    else
        *pu64 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode dword, zero extending it to a quad word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU32ZxU64(PVMCPU pVCpu, uint64_t *pu64)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 4 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextU32ZxU64Slow(pVCpu, pu64);

    *pu64 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                pVCpu->iem.s.abOpcode[offOpcode + 1],
                                pVCpu->iem.s.abOpcode[offOpcode + 2],
                                pVCpu->iem.s.abOpcode[offOpcode + 3]);
    pVCpu->iem.s.offOpcode = offOpcode + 4;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next opcode dword and zero extends it to a quad word, returns
 * automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U32_ZX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU32ZxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U32_ZX_U64(a_pu64) (*(a_pu64) = iemOpcodeGetNextU32Jmp(pVCpu))
#endif


#ifndef IEM_WITH_SETJMP
/**
 * Fetches the next signed double word from the opcode stream.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pi32                Where to return the signed double word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS32(PVMCPU pVCpu, int32_t *pi32)
{
    return iemOpcodeGetNextU32(pVCpu, (uint32_t *)pi32);
}
#endif

/**
 * Fetches the next signed double word from the opcode stream, returning
 * automatically on failure.
 *
 * @param   a_pi32              Where to return the signed double word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S32(a_pi32) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS32(pVCpu, (a_pi32)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S32(a_pi32)    (*(a_pi32) = (int32_t)iemOpcodeGetNextU32Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextS32SxU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextS32SxU64Slow(PVMCPU pVCpu, uint64_t *pu64)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 4);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        *pu64 = (int32_t)RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                             pVCpu->iem.s.abOpcode[offOpcode + 1],
                                             pVCpu->iem.s.abOpcode[offOpcode + 2],
                                             pVCpu->iem.s.abOpcode[offOpcode + 3]);
        pVCpu->iem.s.offOpcode = offOpcode + 4;
    }
    else
        *pu64 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode dword, sign extending it into a quad word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode quad word.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextS32SxU64(PVMCPU pVCpu, uint64_t *pu64)
{
    uint8_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_UNLIKELY(offOpcode + 4 > pVCpu->iem.s.cbOpcode))
        return iemOpcodeGetNextS32SxU64Slow(pVCpu, pu64);

    int32_t i32 = RT_MAKE_U32_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                      pVCpu->iem.s.abOpcode[offOpcode + 1],
                                      pVCpu->iem.s.abOpcode[offOpcode + 2],
                                      pVCpu->iem.s.abOpcode[offOpcode + 3]);
    *pu64 = i32;
    pVCpu->iem.s.offOpcode = offOpcode + 4;
    return VINF_SUCCESS;
}

#endif /* !IEM_WITH_SETJMP */


/**
 * Fetches the next opcode double word and sign extends it to a quad word,
 * returns automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_S32_SX_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextS32SxU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_S32_SX_U64(a_pu64) (*(a_pu64) = (int32_t)iemOpcodeGetNextU32Jmp(pVCpu))
#endif

#ifndef IEM_WITH_SETJMP

/**
 * Deals with the problematic cases that iemOpcodeGetNextU64 doesn't like.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemOpcodeGetNextU64Slow(PVMCPU pVCpu, uint64_t *pu64)
{
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 8);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu64 = *(uint64_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu64 = RT_MAKE_U64_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3],
                                    pVCpu->iem.s.abOpcode[offOpcode + 4],
                                    pVCpu->iem.s.abOpcode[offOpcode + 5],
                                    pVCpu->iem.s.abOpcode[offOpcode + 6],
                                    pVCpu->iem.s.abOpcode[offOpcode + 7]);
# endif
        pVCpu->iem.s.offOpcode = offOpcode + 8;
    }
    else
        *pu64 = 0;
    return rcStrict;
}


/**
 * Fetches the next opcode qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64                Where to return the opcode qword.
 */
DECLINLINE(VBOXSTRICTRC) iemOpcodeGetNextU64(PVMCPU pVCpu, uint64_t *pu64)
{
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 8 <= pVCpu->iem.s.cbOpcode))
    {
# ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        *pu64 = *(uint64_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
# else
        *pu64 = RT_MAKE_U64_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                    pVCpu->iem.s.abOpcode[offOpcode + 1],
                                    pVCpu->iem.s.abOpcode[offOpcode + 2],
                                    pVCpu->iem.s.abOpcode[offOpcode + 3],
                                    pVCpu->iem.s.abOpcode[offOpcode + 4],
                                    pVCpu->iem.s.abOpcode[offOpcode + 5],
                                    pVCpu->iem.s.abOpcode[offOpcode + 6],
                                    pVCpu->iem.s.abOpcode[offOpcode + 7]);
# endif
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 8;
        return VINF_SUCCESS;
    }
    return iemOpcodeGetNextU64Slow(pVCpu, pu64);
}

#else  /* IEM_WITH_SETJMP */

/**
 * Deals with the problematic cases that iemOpcodeGetNextU64Jmp doesn't like, longjmp on error.
 *
 * @returns The opcode qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_NO_INLINE(IEM_STATIC, uint64_t) iemOpcodeGetNextU64SlowJmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uint64_t u64;
    iemOpcodeFetchBytesJmp(pVCpu, sizeof(u64), &u64);
    return u64;
# else
    VBOXSTRICTRC rcStrict = iemOpcodeFetchMoreBytes(pVCpu, 8);
    if (rcStrict == VINF_SUCCESS)
    {
        uint8_t offOpcode = pVCpu->iem.s.offOpcode;
        pVCpu->iem.s.offOpcode = offOpcode + 8;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint64_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U64_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3],
                                   pVCpu->iem.s.abOpcode[offOpcode + 4],
                                   pVCpu->iem.s.abOpcode[offOpcode + 5],
                                   pVCpu->iem.s.abOpcode[offOpcode + 6],
                                   pVCpu->iem.s.abOpcode[offOpcode + 7]);
#  endif
    }
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
# endif
}


/**
 * Fetches the next opcode qword, longjmp on error.
 *
 * @returns The opcode qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(uint64_t) iemOpcodeGetNextU64Jmp(PVMCPU pVCpu)
{
# ifdef IEM_WITH_CODE_TLB
    uintptr_t       offBuf = pVCpu->iem.s.offInstrNextByte;
    uint8_t const  *pbBuf  = pVCpu->iem.s.pbInstrBuf;
    if (RT_LIKELY(   pbBuf != NULL
                  && offBuf + 8 <= pVCpu->iem.s.cbInstrBuf))
    {
        pVCpu->iem.s.offInstrNextByte = (uint32_t)offBuf + 8;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint64_t const *)&pbBuf[offBuf];
#  else
        return RT_MAKE_U64_FROM_U8(pbBuf[offBuf],
                                   pbBuf[offBuf + 1],
                                   pbBuf[offBuf + 2],
                                   pbBuf[offBuf + 3],
                                   pbBuf[offBuf + 4],
                                   pbBuf[offBuf + 5],
                                   pbBuf[offBuf + 6],
                                   pbBuf[offBuf + 7]);
#  endif
    }
# else
    uintptr_t const offOpcode = pVCpu->iem.s.offOpcode;
    if (RT_LIKELY((uint8_t)offOpcode + 8 <= pVCpu->iem.s.cbOpcode))
    {
        pVCpu->iem.s.offOpcode = (uint8_t)offOpcode + 8;
#  ifdef IEM_USE_UNALIGNED_DATA_ACCESS
        return *(uint64_t const *)&pVCpu->iem.s.abOpcode[offOpcode];
#  else
        return RT_MAKE_U64_FROM_U8(pVCpu->iem.s.abOpcode[offOpcode],
                                   pVCpu->iem.s.abOpcode[offOpcode + 1],
                                   pVCpu->iem.s.abOpcode[offOpcode + 2],
                                   pVCpu->iem.s.abOpcode[offOpcode + 3],
                                   pVCpu->iem.s.abOpcode[offOpcode + 4],
                                   pVCpu->iem.s.abOpcode[offOpcode + 5],
                                   pVCpu->iem.s.abOpcode[offOpcode + 6],
                                   pVCpu->iem.s.abOpcode[offOpcode + 7]);
#  endif
    }
# endif
    return iemOpcodeGetNextU64SlowJmp(pVCpu);
}

#endif /* IEM_WITH_SETJMP */

/**
 * Fetches the next opcode quad word, returns automatically on failure.
 *
 * @param   a_pu64              Where to return the opcode quad word.
 * @remark Implicitly references pVCpu.
 */
#ifndef IEM_WITH_SETJMP
# define IEM_OPCODE_GET_NEXT_U64(a_pu64) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = iemOpcodeGetNextU64(pVCpu, (a_pu64)); \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)
#else
# define IEM_OPCODE_GET_NEXT_U64(a_pu64)    ( *(a_pu64) = iemOpcodeGetNextU64Jmp(pVCpu) )
#endif


/** @name  Misc Worker Functions.
 * @{
 */

/**
 * Gets the exception class for the specified exception vector.
 *
 * @returns The class of the specified exception.
 * @param   uVector       The exception vector.
 */
IEM_STATIC IEMXCPTCLASS iemGetXcptClass(uint8_t uVector)
{
    Assert(uVector <= X86_XCPT_LAST);
    switch (uVector)
    {
        case X86_XCPT_DE:
        case X86_XCPT_TS:
        case X86_XCPT_NP:
        case X86_XCPT_SS:
        case X86_XCPT_GP:
        case X86_XCPT_SX:   /* AMD only */
            return IEMXCPTCLASS_CONTRIBUTORY;

        case X86_XCPT_PF:
        case X86_XCPT_VE:   /* Intel only */
            return IEMXCPTCLASS_PAGE_FAULT;

        case X86_XCPT_DF:
            return IEMXCPTCLASS_DOUBLE_FAULT;
    }
    return IEMXCPTCLASS_BENIGN;
}


/**
 * Evaluates how to handle an exception caused during delivery of another event
 * (exception / interrupt).
 *
 * @returns How to handle the recursive exception.
 * @param   pVCpu               The cross context virtual CPU structure of the
 *                              calling thread.
 * @param   fPrevFlags          The flags of the previous event.
 * @param   uPrevVector         The vector of the previous event.
 * @param   fCurFlags           The flags of the current exception.
 * @param   uCurVector          The vector of the current exception.
 * @param   pfXcptRaiseInfo     Where to store additional information about the
 *                              exception condition. Optional.
 */
VMM_INT_DECL(IEMXCPTRAISE) IEMEvaluateRecursiveXcpt(PVMCPU pVCpu, uint32_t fPrevFlags, uint8_t uPrevVector, uint32_t fCurFlags,
                                                    uint8_t uCurVector, PIEMXCPTRAISEINFO pfXcptRaiseInfo)
{
    /*
     * Only CPU exceptions can be raised while delivering other events, software interrupt
     * (INTn/INT3/INTO/ICEBP) generated exceptions cannot occur as the current (second) exception.
     */
    AssertReturn(fCurFlags & IEM_XCPT_FLAGS_T_CPU_XCPT, IEMXCPTRAISE_INVALID);
    Assert(pVCpu); RT_NOREF(pVCpu);
    Log2(("IEMEvaluateRecursiveXcpt: uPrevVector=%#x uCurVector=%#x\n", uPrevVector, uCurVector));

    IEMXCPTRAISE     enmRaise   = IEMXCPTRAISE_CURRENT_XCPT;
    IEMXCPTRAISEINFO fRaiseInfo = IEMXCPTRAISEINFO_NONE;
    if (fPrevFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
    {
        IEMXCPTCLASS enmPrevXcptClass = iemGetXcptClass(uPrevVector);
        if (enmPrevXcptClass != IEMXCPTCLASS_BENIGN)
        {
            IEMXCPTCLASS enmCurXcptClass  = iemGetXcptClass(uCurVector);
            if (   enmPrevXcptClass == IEMXCPTCLASS_PAGE_FAULT
                && (   enmCurXcptClass == IEMXCPTCLASS_PAGE_FAULT
                    || enmCurXcptClass == IEMXCPTCLASS_CONTRIBUTORY))
            {
                enmRaise = IEMXCPTRAISE_DOUBLE_FAULT;
                fRaiseInfo = enmCurXcptClass == IEMXCPTCLASS_PAGE_FAULT ? IEMXCPTRAISEINFO_PF_PF
                                                                        : IEMXCPTRAISEINFO_PF_CONTRIBUTORY_XCPT;
                Log2(("IEMEvaluateRecursiveXcpt: Vectoring page fault. uPrevVector=%#x uCurVector=%#x uCr2=%#RX64\n", uPrevVector,
                      uCurVector, IEM_GET_CTX(pVCpu)->cr2));
            }
            else if (   enmPrevXcptClass == IEMXCPTCLASS_CONTRIBUTORY
                     && enmCurXcptClass  == IEMXCPTCLASS_CONTRIBUTORY)
            {
                enmRaise = IEMXCPTRAISE_DOUBLE_FAULT;
                Log2(("IEMEvaluateRecursiveXcpt: uPrevVector=%u uCurVector=%u -> #DF\n", uPrevVector, uCurVector));
            }
            else if (   enmPrevXcptClass == IEMXCPTCLASS_DOUBLE_FAULT
                     && (   enmCurXcptClass == IEMXCPTCLASS_CONTRIBUTORY
                         || enmCurXcptClass == IEMXCPTCLASS_PAGE_FAULT))
            {
                enmRaise = IEMXCPTRAISE_TRIPLE_FAULT;
                Log2(("IEMEvaluateRecursiveXcpt: #DF handler raised a %#x exception -> triple fault\n", uCurVector));
            }
        }
        else
        {
            if (uPrevVector == X86_XCPT_NMI)
            {
                fRaiseInfo = IEMXCPTRAISEINFO_NMI_XCPT;
                if (uCurVector == X86_XCPT_PF)
                {
                    fRaiseInfo |= IEMXCPTRAISEINFO_NMI_PF;
                    Log2(("IEMEvaluateRecursiveXcpt: NMI delivery caused a page fault\n"));
                }
            }
            else if (   uPrevVector == X86_XCPT_AC
                     && uCurVector  == X86_XCPT_AC)
            {
                enmRaise   = IEMXCPTRAISE_CPU_HANG;
                fRaiseInfo = IEMXCPTRAISEINFO_AC_AC;
                Log2(("IEMEvaluateRecursiveXcpt: Recursive #AC - Bad guest\n"));
            }
        }
    }
    else if (fPrevFlags & IEM_XCPT_FLAGS_T_EXT_INT)
    {
        fRaiseInfo = IEMXCPTRAISEINFO_EXT_INT_XCPT;
        if (uCurVector == X86_XCPT_PF)
            fRaiseInfo |= IEMXCPTRAISEINFO_EXT_INT_PF;
    }
    else
    {
        Assert(fPrevFlags & IEM_XCPT_FLAGS_T_SOFT_INT);
        fRaiseInfo = IEMXCPTRAISEINFO_SOFT_INT_XCPT;
    }

    if (pfXcptRaiseInfo)
        *pfXcptRaiseInfo = fRaiseInfo;
    return enmRaise;
}


/**
 * Enters the CPU shutdown state initiated by a triple fault or other
 * unrecoverable conditions.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling thread.
 */
IEM_STATIC VBOXSTRICTRC iemInitiateCpuShutdown(PVMCPU pVCpu)
{
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_SHUTDOWN))
    {
        Log2(("shutdown: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_SHUTDOWN, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    RT_NOREF(pVCpu);
    return VINF_EM_TRIPLE_FAULT;
}


/**
 * Validates a new SS segment.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling thread.
 * @param   pCtx            The CPU context.
 * @param   NewSS           The new SS selctor.
 * @param   uCpl            The CPL to load the stack for.
 * @param   pDesc           Where to return the descriptor.
 */
IEM_STATIC VBOXSTRICTRC iemMiscValidateNewSS(PVMCPU pVCpu, PCCPUMCTX pCtx, RTSEL NewSS, uint8_t uCpl, PIEMSELDESC pDesc)
{
    NOREF(pCtx);

    /* Null selectors are not allowed (we're not called for dispatching
       interrupts with SS=0 in long mode). */
    if (!(NewSS & X86_SEL_MASK_OFF_RPL))
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - null selector -> #TS(0)\n", NewSS));
        return iemRaiseTaskSwitchFault0(pVCpu);
    }

    /** @todo testcase: check that the TSS.ssX RPL is checked.  Also check when. */
    if ((NewSS & X86_SEL_RPL) != uCpl)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - RPL and CPL (%d) differs -> #TS\n", NewSS, uCpl));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }

    /*
     * Read the descriptor.
     */
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, pDesc, NewSS, X86_XCPT_TS);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Perform the descriptor validation documented for LSS, POP SS and MOV SS.
     */
    if (!pDesc->Legacy.Gen.u1DescType)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - system selector (%#x) -> #TS\n", NewSS, pDesc->Legacy.Gen.u4Type));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }

    if (    (pDesc->Legacy.Gen.u4Type & X86_SEL_TYPE_CODE)
        || !(pDesc->Legacy.Gen.u4Type & X86_SEL_TYPE_WRITE) )
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - code or read only (%#x) -> #TS\n", NewSS, pDesc->Legacy.Gen.u4Type));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }
    if (pDesc->Legacy.Gen.u2Dpl != uCpl)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - DPL (%d) and CPL (%d) differs -> #TS\n", NewSS, pDesc->Legacy.Gen.u2Dpl, uCpl));
        return iemRaiseTaskSwitchFaultBySelector(pVCpu, NewSS);
    }

    /* Is it there? */
    /** @todo testcase: Is this checked before the canonical / limit check below? */
    if (!pDesc->Legacy.Gen.u1Present)
    {
        Log(("iemMiscValidateNewSSandRsp: %#x - segment not present -> #NP\n", NewSS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, NewSS);
    }

    return VINF_SUCCESS;
}


/**
 * Gets the correct EFLAGS regardless of whether PATM stores parts of them or
 * not.
 *
 * @param   a_pVCpu The cross context virtual CPU structure of the calling thread.
 * @param   a_pCtx  The CPU context.
 */
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
# define IEMMISC_GET_EFL(a_pVCpu, a_pCtx) \
    ( IEM_VERIFICATION_ENABLED(a_pVCpu) \
      ? (a_pCtx)->eflags.u \
      : CPUMRawGetEFlags(a_pVCpu) )
#else
# define IEMMISC_GET_EFL(a_pVCpu, a_pCtx) \
    ( (a_pCtx)->eflags.u  )
#endif

/**
 * Updates the EFLAGS in the correct manner wrt. PATM.
 *
 * @param   a_pVCpu The cross context virtual CPU structure of the calling thread.
 * @param   a_pCtx  The CPU context.
 * @param   a_fEfl  The new EFLAGS.
 */
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
# define IEMMISC_SET_EFL(a_pVCpu, a_pCtx, a_fEfl) \
    do { \
        if (IEM_VERIFICATION_ENABLED(a_pVCpu)) \
            (a_pCtx)->eflags.u = (a_fEfl); \
        else \
            CPUMRawSetEFlags((a_pVCpu), a_fEfl); \
    } while (0)
#else
# define IEMMISC_SET_EFL(a_pVCpu, a_pCtx, a_fEfl) \
    do { \
        (a_pCtx)->eflags.u = (a_fEfl); \
    } while (0)
#endif


/** @}  */

/** @name  Raising Exceptions.
 *
 * @{
 */


/**
 * Loads the specified stack far pointer from the TSS.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pCtx            The CPU context.
 * @param   uCpl            The CPL to load the stack for.
 * @param   pSelSS          Where to return the new stack segment.
 * @param   puEsp           Where to return the new stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemRaiseLoadStackFromTss32Or16(PVMCPU pVCpu, PCCPUMCTX pCtx, uint8_t uCpl,
                                                       PRTSEL pSelSS, uint32_t *puEsp)
{
    VBOXSTRICTRC rcStrict;
    Assert(uCpl < 4);

    switch (pCtx->tr.Attr.n.u4Type)
    {
        /*
         * 16-bit TSS (X86TSS16).
         */
        case X86_SEL_TYPE_SYS_286_TSS_AVAIL: AssertFailed(); RT_FALL_THRU();
        case X86_SEL_TYPE_SYS_286_TSS_BUSY:
        {
            uint32_t off = uCpl * 4 + 2;
            if (off + 4 <= pCtx->tr.u32Limit)
            {
                /** @todo check actual access pattern here. */
                uint32_t u32Tmp = 0; /* gcc maybe... */
                rcStrict = iemMemFetchSysU32(pVCpu, &u32Tmp, UINT8_MAX, pCtx->tr.u64Base + off);
                if (rcStrict == VINF_SUCCESS)
                {
                    *puEsp  = RT_LOWORD(u32Tmp);
                    *pSelSS = RT_HIWORD(u32Tmp);
                    return VINF_SUCCESS;
                }
            }
            else
            {
                Log(("LoadStackFromTss32Or16: out of bounds! uCpl=%d, u32Limit=%#x TSS16\n", uCpl, pCtx->tr.u32Limit));
                rcStrict = iemRaiseTaskSwitchFaultCurrentTSS(pVCpu);
            }
            break;
        }

        /*
         * 32-bit TSS (X86TSS32).
         */
        case X86_SEL_TYPE_SYS_386_TSS_AVAIL: AssertFailed(); RT_FALL_THRU();
        case X86_SEL_TYPE_SYS_386_TSS_BUSY:
        {
            uint32_t off = uCpl * 8 + 4;
            if (off + 7 <= pCtx->tr.u32Limit)
            {
/** @todo check actual access pattern here. */
                uint64_t u64Tmp;
                rcStrict = iemMemFetchSysU64(pVCpu, &u64Tmp, UINT8_MAX, pCtx->tr.u64Base + off);
                if (rcStrict == VINF_SUCCESS)
                {
                    *puEsp  = u64Tmp & UINT32_MAX;
                    *pSelSS = (RTSEL)(u64Tmp >> 32);
                    return VINF_SUCCESS;
                }
            }
            else
            {
                Log(("LoadStackFromTss32Or16: out of bounds! uCpl=%d, u32Limit=%#x TSS16\n", uCpl, pCtx->tr.u32Limit));
                rcStrict = iemRaiseTaskSwitchFaultCurrentTSS(pVCpu);
            }
            break;
        }

        default:
            AssertFailed();
            rcStrict = VERR_IEM_IPE_4;
            break;
    }

    *puEsp  = 0; /* make gcc happy */
    *pSelSS = 0; /* make gcc happy */
    return rcStrict;
}


/**
 * Loads the specified stack pointer from the 64-bit TSS.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pCtx            The CPU context.
 * @param   uCpl            The CPL to load the stack for.
 * @param   uIst            The interrupt stack table index, 0 if to use uCpl.
 * @param   puRsp           Where to return the new stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemRaiseLoadStackFromTss64(PVMCPU pVCpu, PCCPUMCTX pCtx, uint8_t uCpl, uint8_t uIst, uint64_t *puRsp)
{
    Assert(uCpl < 4);
    Assert(uIst < 8);
    *puRsp  = 0; /* make gcc happy */

    AssertReturn(pCtx->tr.Attr.n.u4Type == AMD64_SEL_TYPE_SYS_TSS_BUSY, VERR_IEM_IPE_5);

    uint32_t off;
    if (uIst)
        off = (uIst - 1) * sizeof(uint64_t) + RT_UOFFSETOF(X86TSS64, ist1);
    else
        off = uCpl * sizeof(uint64_t) + RT_UOFFSETOF(X86TSS64, rsp0);
    if (off + sizeof(uint64_t) > pCtx->tr.u32Limit)
    {
        Log(("iemRaiseLoadStackFromTss64: out of bounds! uCpl=%d uIst=%d, u32Limit=%#x\n", uCpl, uIst, pCtx->tr.u32Limit));
        return iemRaiseTaskSwitchFaultCurrentTSS(pVCpu);
    }

    return iemMemFetchSysU64(pVCpu, puRsp, UINT8_MAX, pCtx->tr.u64Base + off);
}


/**
 * Adjust the CPU state according to the exception being raised.
 *
 * @param   pCtx                The CPU context.
 * @param   u8Vector            The exception that has been raised.
 */
DECLINLINE(void) iemRaiseXcptAdjustState(PCPUMCTX pCtx, uint8_t u8Vector)
{
    switch (u8Vector)
    {
        case X86_XCPT_DB:
            pCtx->dr[7] &= ~X86_DR7_GD;
            break;
        /** @todo Read the AMD and Intel exception reference... */
    }
}


/**
 * Implements exceptions and interrupts for real mode.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pCtx            The CPU context.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
IEM_STATIC VBOXSTRICTRC
iemRaiseXcptOrIntInRealMode(PVMCPU      pVCpu,
                            PCPUMCTX    pCtx,
                            uint8_t     cbInstr,
                            uint8_t     u8Vector,
                            uint32_t    fFlags,
                            uint16_t    uErr,
                            uint64_t    uCr2)
{
    NOREF(uErr); NOREF(uCr2);

    /*
     * Read the IDT entry.
     */
    if (pCtx->idtr.cbIdt < UINT32_C(4) * u8Vector + 3)
    {
        Log(("RaiseXcptOrIntInRealMode: %#x is out of bounds (%#x)\n", u8Vector, pCtx->idtr.cbIdt));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    RTFAR16 Idte;
    VBOXSTRICTRC rcStrict = iemMemFetchDataU32(pVCpu, (uint32_t *)&Idte, UINT8_MAX, pCtx->idtr.pIdt + UINT32_C(4) * u8Vector);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
        return rcStrict;

    /*
     * Push the stack frame.
     */
    uint16_t *pu16Frame;
    uint64_t  uNewRsp;
    rcStrict = iemMemStackPushBeginSpecial(pVCpu, 6, (void **)&pu16Frame, &uNewRsp);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    uint32_t fEfl = IEMMISC_GET_EFL(pVCpu, pCtx);
#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
    AssertCompile(IEMTARGETCPU_8086 <= IEMTARGETCPU_186 && IEMTARGETCPU_V20 <= IEMTARGETCPU_186 && IEMTARGETCPU_286 > IEMTARGETCPU_186);
    if (pVCpu->iem.s.uTargetCpu <= IEMTARGETCPU_186)
        fEfl |= UINT16_C(0xf000);
#endif
    pu16Frame[2] = (uint16_t)fEfl;
    pu16Frame[1] = (uint16_t)pCtx->cs.Sel;
    pu16Frame[0] = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pCtx->ip + cbInstr : pCtx->ip;
    rcStrict = iemMemStackPushCommitSpecial(pVCpu, pu16Frame, uNewRsp);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
        return rcStrict;

    /*
     * Load the vector address into cs:ip and make exception specific state
     * adjustments.
     */
    pCtx->cs.Sel           = Idte.sel;
    pCtx->cs.ValidSel      = Idte.sel;
    pCtx->cs.fFlags        = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.u64Base       = (uint32_t)Idte.sel << 4;
    /** @todo do we load attribs and limit as well? Should we check against limit like far jump? */
    pCtx->rip              = Idte.off;
    fEfl &= ~(X86_EFL_IF | X86_EFL_TF | X86_EFL_AC);
    IEMMISC_SET_EFL(pVCpu, pCtx, fEfl);

    /** @todo do we actually do this in real mode? */
    if (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        iemRaiseXcptAdjustState(pCtx, u8Vector);

    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Loads a NULL data selector into when coming from V8086 mode.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pSReg           Pointer to the segment register.
 */
IEM_STATIC void iemHlpLoadNullDataSelectorOnV86Xcpt(PVMCPU pVCpu, PCPUMSELREG pSReg)
{
    pSReg->Sel      = 0;
    pSReg->ValidSel = 0;
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
    {
        /* VT-x (Intel 3960x) doesn't change the base and limit, clears and sets the following attributes */
        pSReg->Attr.u &= X86DESCATTR_DT | X86DESCATTR_TYPE | X86DESCATTR_DPL | X86DESCATTR_G | X86DESCATTR_D;
        pSReg->Attr.u |= X86DESCATTR_UNUSABLE;
    }
    else
    {
        pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
        /** @todo check this on AMD-V */
        pSReg->u64Base  = 0;
        pSReg->u32Limit = 0;
    }
}


/**
 * Loads a segment selector during a task switch in V8086 mode.
 *
 * @param   pSReg           Pointer to the segment register.
 * @param   uSel            The selector value to load.
 */
IEM_STATIC void iemHlpLoadSelectorInV86Mode(PCPUMSELREG pSReg, uint16_t uSel)
{
    /* See Intel spec. 26.3.1.2 "Checks on Guest Segment Registers". */
    pSReg->Sel      = uSel;
    pSReg->ValidSel = uSel;
    pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
    pSReg->u64Base  = uSel << 4;
    pSReg->u32Limit = 0xffff;
    pSReg->Attr.u   = 0xf3;
}


/**
 * Loads a NULL data selector into a selector register, both the hidden and
 * visible parts, in protected mode.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pSReg               Pointer to the segment register.
 * @param   uRpl                The RPL.
 */
IEM_STATIC void iemHlpLoadNullDataSelectorProt(PVMCPU pVCpu, PCPUMSELREG pSReg, RTSEL uRpl)
{
    /** @todo Testcase: write a testcase checking what happends when loading a NULL
     *        data selector in protected mode. */
    pSReg->Sel      = uRpl;
    pSReg->ValidSel = uRpl;
    pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
    {
        /* VT-x (Intel 3960x) observed doing something like this. */
        pSReg->Attr.u   = X86DESCATTR_UNUSABLE | X86DESCATTR_G | X86DESCATTR_D | (pVCpu->iem.s.uCpl << X86DESCATTR_DPL_SHIFT);
        pSReg->u32Limit = UINT32_MAX;
        pSReg->u64Base  = 0;
    }
    else
    {
        pSReg->Attr.u   = X86DESCATTR_UNUSABLE;
        pSReg->u32Limit = 0;
        pSReg->u64Base  = 0;
    }
}


/**
 * Loads a segment selector during a task switch in protected mode.
 *
 * In this task switch scenario, we would throw \#TS exceptions rather than
 * \#GPs.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pSReg           Pointer to the segment register.
 * @param   uSel            The new selector value.
 *
 * @remarks This does _not_ handle CS or SS.
 * @remarks This expects pVCpu->iem.s.uCpl to be up to date.
 */
IEM_STATIC VBOXSTRICTRC iemHlpTaskSwitchLoadDataSelectorInProtMode(PVMCPU pVCpu, PCPUMSELREG pSReg, uint16_t uSel)
{
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

    /* Null data selector. */
    if (!(uSel & X86_SEL_MASK_OFF_RPL))
    {
        iemHlpLoadNullDataSelectorProt(pVCpu, pSReg, uSel);
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);
        return VINF_SUCCESS;
    }

    /* Fetch the descriptor. */
    IEMSELDESC Desc;
    VBOXSTRICTRC rcStrict = iemMemFetchSelDesc(pVCpu, &Desc, uSel, X86_XCPT_TS);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: failed to fetch selector. uSel=%u rc=%Rrc\n", uSel,
             VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a data segment or readable code segment. */
    if (   !Desc.Legacy.Gen.u1DescType
        || (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: invalid segment type. uSel=%u Desc.u4Type=%#x\n", uSel,
             Desc.Legacy.Gen.u4Type));
        return iemRaiseTaskSwitchFaultWithErr(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /* Check privileges for data segments and non-conforming code segments. */
    if (   (Desc.Legacy.Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
        != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
    {
        /* The RPL and the new CPL must be less than or equal to the DPL. */
        if (   (unsigned)(uSel & X86_SEL_RPL) > Desc.Legacy.Gen.u2Dpl
            || (pVCpu->iem.s.uCpl > Desc.Legacy.Gen.u2Dpl))
        {
            Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: Invalid priv. uSel=%u uSel.RPL=%u DPL=%u CPL=%u\n",
                 uSel, (uSel & X86_SEL_RPL), Desc.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
        }
    }

    /* Is it there? */
    if (!Desc.Legacy.Gen.u1Present)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: Segment not present. uSel=%u\n", uSel));
        return iemRaiseSelectorNotPresentWithErr(pVCpu, uSel & X86_SEL_MASK_OFF_RPL);
    }

    /* The base and limit. */
    uint32_t cbLimit = X86DESC_LIMIT_G(&Desc.Legacy);
    uint64_t u64Base = X86DESC_BASE(&Desc.Legacy);

    /*
     * Ok, everything checked out fine. Now set the accessed bit before
     * committing the result into the registers.
     */
    if (!(Desc.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, uSel);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        Desc.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /* Commit */
    pSReg->Sel      = uSel;
    pSReg->Attr.u   = X86DESC_GET_HID_ATTR(&Desc.Legacy);
    pSReg->u32Limit = cbLimit;
    pSReg->u64Base  = u64Base;  /** @todo testcase/investigate: seen claims that the upper half of the base remains unchanged... */
    pSReg->ValidSel = uSel;
    pSReg->fFlags   = CPUMSELREG_FLAGS_VALID;
    if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
        pSReg->Attr.u &= ~X86DESCATTR_UNUSABLE;

    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);
    return VINF_SUCCESS;
}


/**
 * Performs a task switch.
 *
 * If the task switch is the result of a JMP, CALL or IRET instruction, the
 * caller is responsible for performing the necessary checks (like DPL, TSS
 * present etc.) which are specific to JMP/CALL/IRET. See Intel Instruction
 * reference for JMP, CALL, IRET.
 *
 * If the task switch is the due to a software interrupt or hardware exception,
 * the caller is responsible for validating the TSS selector and descriptor. See
 * Intel Instruction reference for INT n.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pCtx            The CPU context.
 * @param   enmTaskSwitch   What caused this task switch.
 * @param   uNextEip        The EIP effective after the task switch.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 * @param   SelTSS          The TSS selector of the new task.
 * @param   pNewDescTSS     Pointer to the new TSS descriptor.
 */
IEM_STATIC VBOXSTRICTRC
iemTaskSwitch(PVMCPU          pVCpu,
              PCPUMCTX        pCtx,
              IEMTASKSWITCH   enmTaskSwitch,
              uint32_t        uNextEip,
              uint32_t        fFlags,
              uint16_t        uErr,
              uint64_t        uCr2,
              RTSEL           SelTSS,
              PIEMSELDESC     pNewDescTSS)
{
    Assert(!IEM_IS_REAL_MODE(pVCpu));
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

    uint32_t const uNewTSSType = pNewDescTSS->Legacy.Gate.u4Type;
    Assert(   uNewTSSType == X86_SEL_TYPE_SYS_286_TSS_AVAIL
           || uNewTSSType == X86_SEL_TYPE_SYS_286_TSS_BUSY
           || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_AVAIL
           || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_BUSY);

    bool const fIsNewTSS386 = (   uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_AVAIL
                               || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_BUSY);

    Log(("iemTaskSwitch: enmTaskSwitch=%u NewTSS=%#x fIsNewTSS386=%RTbool EIP=%#RX32 uNextEip=%#RX32\n", enmTaskSwitch, SelTSS,
         fIsNewTSS386, pCtx->eip, uNextEip));

    /* Update CR2 in case it's a page-fault. */
    /** @todo This should probably be done much earlier in IEM/PGM. See
     *        @bugref{5653#c49}. */
    if (fFlags & IEM_XCPT_FLAGS_CR2)
        pCtx->cr2 = uCr2;

    /*
     * Check the new TSS limit. See Intel spec. 6.15 "Exception and Interrupt Reference"
     * subsection "Interrupt 10 - Invalid TSS Exception (#TS)".
     */
    uint32_t const uNewTSSLimit    = pNewDescTSS->Legacy.Gen.u16LimitLow | (pNewDescTSS->Legacy.Gen.u4LimitHigh << 16);
    uint32_t const uNewTSSLimitMin = fIsNewTSS386 ? X86_SEL_TYPE_SYS_386_TSS_LIMIT_MIN : X86_SEL_TYPE_SYS_286_TSS_LIMIT_MIN;
    if (uNewTSSLimit < uNewTSSLimitMin)
    {
        Log(("iemTaskSwitch: Invalid new TSS limit. enmTaskSwitch=%u uNewTSSLimit=%#x uNewTSSLimitMin=%#x -> #TS\n",
             enmTaskSwitch, uNewTSSLimit, uNewTSSLimitMin));
        return iemRaiseTaskSwitchFaultWithErr(pVCpu, SelTSS & X86_SEL_MASK_OFF_RPL);
    }

    /*
     * Check the current TSS limit. The last written byte to the current TSS during the
     * task switch will be 2 bytes at offset 0x5C (32-bit) and 1 byte at offset 0x28 (16-bit).
     * See Intel spec. 7.2.1 "Task-State Segment (TSS)" for static and dynamic fields.
     *
     * The AMD docs doesn't mention anything about limit checks with LTR which suggests you can
     * end up with smaller than "legal" TSS limits.
     */
    uint32_t const uCurTSSLimit    = pCtx->tr.u32Limit;
    uint32_t const uCurTSSLimitMin = fIsNewTSS386 ? 0x5F : 0x29;
    if (uCurTSSLimit < uCurTSSLimitMin)
    {
        Log(("iemTaskSwitch: Invalid current TSS limit. enmTaskSwitch=%u uCurTSSLimit=%#x uCurTSSLimitMin=%#x -> #TS\n",
             enmTaskSwitch, uCurTSSLimit, uCurTSSLimitMin));
        return iemRaiseTaskSwitchFaultWithErr(pVCpu, SelTSS & X86_SEL_MASK_OFF_RPL);
    }

    /*
     * Verify that the new TSS can be accessed and map it. Map only the required contents
     * and not the entire TSS.
     */
    void     *pvNewTSS;
    uint32_t  cbNewTSS    = uNewTSSLimitMin + 1;
    RTGCPTR   GCPtrNewTSS = X86DESC_BASE(&pNewDescTSS->Legacy);
    AssertCompile(sizeof(X86TSS32) == X86_SEL_TYPE_SYS_386_TSS_LIMIT_MIN + 1);
    /** @todo Handle if the TSS crosses a page boundary. Intel specifies that it may
     *        not perform correct translation if this happens. See Intel spec. 7.2.1
     *        "Task-State Segment" */
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, &pvNewTSS, cbNewTSS, UINT8_MAX, GCPtrNewTSS, IEM_ACCESS_SYS_RW);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemTaskSwitch: Failed to read new TSS. enmTaskSwitch=%u cbNewTSS=%u uNewTSSLimit=%u rc=%Rrc\n", enmTaskSwitch,
             cbNewTSS, uNewTSSLimit, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * Clear the busy bit in current task's TSS descriptor if it's a task switch due to JMP/IRET.
     */
    uint32_t u32EFlags = pCtx->eflags.u32;
    if (   enmTaskSwitch == IEMTASKSWITCH_JUMP
        || enmTaskSwitch == IEMTASKSWITCH_IRET)
    {
        PX86DESC pDescCurTSS;
        rcStrict = iemMemMap(pVCpu, (void **)&pDescCurTSS, sizeof(*pDescCurTSS), UINT8_MAX,
                             pCtx->gdtr.pGdt + (pCtx->tr.Sel & X86_SEL_MASK), IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read new TSS descriptor in GDT. enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pCtx->gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        pDescCurTSS->Gate.u4Type &= ~X86_SEL_TYPE_SYS_TSS_BUSY_MASK;
        rcStrict = iemMemCommitAndUnmap(pVCpu, pDescCurTSS, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit new TSS descriptor in GDT. enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pCtx->gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* Clear EFLAGS.NT (Nested Task) in the eflags memory image, if it's a task switch due to an IRET. */
        if (enmTaskSwitch == IEMTASKSWITCH_IRET)
        {
            Assert(   uNewTSSType == X86_SEL_TYPE_SYS_286_TSS_BUSY
                   || uNewTSSType == X86_SEL_TYPE_SYS_386_TSS_BUSY);
            u32EFlags &= ~X86_EFL_NT;
        }
    }

    /*
     * Save the CPU state into the current TSS.
     */
    RTGCPTR GCPtrCurTSS = pCtx->tr.u64Base;
    if (GCPtrNewTSS == GCPtrCurTSS)
    {
        Log(("iemTaskSwitch: Switching to the same TSS! enmTaskSwitch=%u GCPtr[Cur|New]TSS=%#RGv\n", enmTaskSwitch, GCPtrCurTSS));
        Log(("uCurCr3=%#x uCurEip=%#x uCurEflags=%#x uCurEax=%#x uCurEsp=%#x uCurEbp=%#x uCurCS=%#04x uCurSS=%#04x uCurLdt=%#x\n",
             pCtx->cr3, pCtx->eip, pCtx->eflags.u32, pCtx->eax, pCtx->esp, pCtx->ebp, pCtx->cs.Sel, pCtx->ss.Sel, pCtx->ldtr.Sel));
    }
    if (fIsNewTSS386)
    {
        /*
         * Verify that the current TSS (32-bit) can be accessed, only the minimum required size.
         * See Intel spec. 7.2.1 "Task-State Segment (TSS)" for static and dynamic fields.
         */
        void    *pvCurTSS32;
        uint32_t offCurTSS = RT_UOFFSETOF(X86TSS32, eip);
        uint32_t cbCurTSS  = RT_UOFFSETOF(X86TSS32, selLdt) - RT_UOFFSETOF(X86TSS32, eip);
        AssertCompile(RTASSERT_OFFSET_OF(X86TSS32, selLdt) - RTASSERT_OFFSET_OF(X86TSS32, eip) == 64);
        rcStrict = iemMemMap(pVCpu, &pvCurTSS32, cbCurTSS, UINT8_MAX, GCPtrCurTSS + offCurTSS, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read current 32-bit TSS. enmTaskSwitch=%u GCPtrCurTSS=%#RGv cb=%u rc=%Rrc\n",
                 enmTaskSwitch, GCPtrCurTSS, cbCurTSS, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* !! WARNING !! Access -only- the members (dynamic fields) that are mapped, i.e interval [offCurTSS..cbCurTSS). */
        PX86TSS32 pCurTSS32 = (PX86TSS32)((uintptr_t)pvCurTSS32 - offCurTSS);
        pCurTSS32->eip    = uNextEip;
        pCurTSS32->eflags = u32EFlags;
        pCurTSS32->eax    = pCtx->eax;
        pCurTSS32->ecx    = pCtx->ecx;
        pCurTSS32->edx    = pCtx->edx;
        pCurTSS32->ebx    = pCtx->ebx;
        pCurTSS32->esp    = pCtx->esp;
        pCurTSS32->ebp    = pCtx->ebp;
        pCurTSS32->esi    = pCtx->esi;
        pCurTSS32->edi    = pCtx->edi;
        pCurTSS32->es     = pCtx->es.Sel;
        pCurTSS32->cs     = pCtx->cs.Sel;
        pCurTSS32->ss     = pCtx->ss.Sel;
        pCurTSS32->ds     = pCtx->ds.Sel;
        pCurTSS32->fs     = pCtx->fs.Sel;
        pCurTSS32->gs     = pCtx->gs.Sel;

        rcStrict = iemMemCommitAndUnmap(pVCpu, pvCurTSS32, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit current 32-bit TSS. enmTaskSwitch=%u rc=%Rrc\n", enmTaskSwitch,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }
    else
    {
        /*
         * Verify that the current TSS (16-bit) can be accessed. Again, only the minimum required size.
         */
        void    *pvCurTSS16;
        uint32_t offCurTSS = RT_UOFFSETOF(X86TSS16, ip);
        uint32_t cbCurTSS  = RT_UOFFSETOF(X86TSS16, selLdt) - RT_UOFFSETOF(X86TSS16, ip);
        AssertCompile(RTASSERT_OFFSET_OF(X86TSS16, selLdt) - RTASSERT_OFFSET_OF(X86TSS16, ip) == 28);
        rcStrict = iemMemMap(pVCpu, &pvCurTSS16, cbCurTSS, UINT8_MAX, GCPtrCurTSS + offCurTSS, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read current 16-bit TSS. enmTaskSwitch=%u GCPtrCurTSS=%#RGv cb=%u rc=%Rrc\n",
                 enmTaskSwitch, GCPtrCurTSS, cbCurTSS, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* !! WARNING !! Access -only- the members (dynamic fields) that are mapped, i.e interval [offCurTSS..cbCurTSS). */
        PX86TSS16 pCurTSS16 = (PX86TSS16)((uintptr_t)pvCurTSS16 - offCurTSS);
        pCurTSS16->ip    = uNextEip;
        pCurTSS16->flags = u32EFlags;
        pCurTSS16->ax    = pCtx->ax;
        pCurTSS16->cx    = pCtx->cx;
        pCurTSS16->dx    = pCtx->dx;
        pCurTSS16->bx    = pCtx->bx;
        pCurTSS16->sp    = pCtx->sp;
        pCurTSS16->bp    = pCtx->bp;
        pCurTSS16->si    = pCtx->si;
        pCurTSS16->di    = pCtx->di;
        pCurTSS16->es    = pCtx->es.Sel;
        pCurTSS16->cs    = pCtx->cs.Sel;
        pCurTSS16->ss    = pCtx->ss.Sel;
        pCurTSS16->ds    = pCtx->ds.Sel;

        rcStrict = iemMemCommitAndUnmap(pVCpu, pvCurTSS16, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit current 16-bit TSS. enmTaskSwitch=%u rc=%Rrc\n", enmTaskSwitch,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /*
     * Update the previous task link field for the new TSS, if the task switch is due to a CALL/INT_XCPT.
     */
    if (   enmTaskSwitch == IEMTASKSWITCH_CALL
        || enmTaskSwitch == IEMTASKSWITCH_INT_XCPT)
    {
        /* 16 or 32-bit TSS doesn't matter, we only access the first, common 16-bit field (selPrev) here. */
        PX86TSS32 pNewTSS = (PX86TSS32)pvNewTSS;
        pNewTSS->selPrev  = pCtx->tr.Sel;
    }

    /*
     * Read the state from the new TSS into temporaries. Setting it immediately as the new CPU state is tricky,
     * it's done further below with error handling (e.g. CR3 changes will go through PGM).
     */
    uint32_t uNewCr3, uNewEip, uNewEflags, uNewEax, uNewEcx, uNewEdx, uNewEbx, uNewEsp, uNewEbp, uNewEsi, uNewEdi;
    uint16_t uNewES,  uNewCS, uNewSS, uNewDS, uNewFS, uNewGS, uNewLdt;
    bool     fNewDebugTrap;
    if (fIsNewTSS386)
    {
        PX86TSS32 pNewTSS32 = (PX86TSS32)pvNewTSS;
        uNewCr3       = (pCtx->cr0 & X86_CR0_PG) ? pNewTSS32->cr3 : 0;
        uNewEip       = pNewTSS32->eip;
        uNewEflags    = pNewTSS32->eflags;
        uNewEax       = pNewTSS32->eax;
        uNewEcx       = pNewTSS32->ecx;
        uNewEdx       = pNewTSS32->edx;
        uNewEbx       = pNewTSS32->ebx;
        uNewEsp       = pNewTSS32->esp;
        uNewEbp       = pNewTSS32->ebp;
        uNewEsi       = pNewTSS32->esi;
        uNewEdi       = pNewTSS32->edi;
        uNewES        = pNewTSS32->es;
        uNewCS        = pNewTSS32->cs;
        uNewSS        = pNewTSS32->ss;
        uNewDS        = pNewTSS32->ds;
        uNewFS        = pNewTSS32->fs;
        uNewGS        = pNewTSS32->gs;
        uNewLdt       = pNewTSS32->selLdt;
        fNewDebugTrap = RT_BOOL(pNewTSS32->fDebugTrap);
    }
    else
    {
        PX86TSS16 pNewTSS16 = (PX86TSS16)pvNewTSS;
        uNewCr3       = 0;
        uNewEip       = pNewTSS16->ip;
        uNewEflags    = pNewTSS16->flags;
        uNewEax       = UINT32_C(0xffff0000) | pNewTSS16->ax;
        uNewEcx       = UINT32_C(0xffff0000) | pNewTSS16->cx;
        uNewEdx       = UINT32_C(0xffff0000) | pNewTSS16->dx;
        uNewEbx       = UINT32_C(0xffff0000) | pNewTSS16->bx;
        uNewEsp       = UINT32_C(0xffff0000) | pNewTSS16->sp;
        uNewEbp       = UINT32_C(0xffff0000) | pNewTSS16->bp;
        uNewEsi       = UINT32_C(0xffff0000) | pNewTSS16->si;
        uNewEdi       = UINT32_C(0xffff0000) | pNewTSS16->di;
        uNewES        = pNewTSS16->es;
        uNewCS        = pNewTSS16->cs;
        uNewSS        = pNewTSS16->ss;
        uNewDS        = pNewTSS16->ds;
        uNewFS        = 0;
        uNewGS        = 0;
        uNewLdt       = pNewTSS16->selLdt;
        fNewDebugTrap = false;
    }

    if (GCPtrNewTSS == GCPtrCurTSS)
        Log(("uNewCr3=%#x uNewEip=%#x uNewEflags=%#x uNewEax=%#x uNewEsp=%#x uNewEbp=%#x uNewCS=%#04x uNewSS=%#04x uNewLdt=%#x\n",
             uNewCr3, uNewEip, uNewEflags, uNewEax, uNewEsp, uNewEbp, uNewCS, uNewSS, uNewLdt));

    /*
     * We're done accessing the new TSS.
     */
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvNewTSS, IEM_ACCESS_SYS_RW);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemTaskSwitch: Failed to commit new TSS. enmTaskSwitch=%u rc=%Rrc\n", enmTaskSwitch, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /*
     * Set the busy bit in the new TSS descriptor, if the task switch is a JMP/CALL/INT_XCPT.
     */
    if (enmTaskSwitch != IEMTASKSWITCH_IRET)
    {
        rcStrict = iemMemMap(pVCpu, (void **)&pNewDescTSS, sizeof(*pNewDescTSS), UINT8_MAX,
                             pCtx->gdtr.pGdt + (SelTSS & X86_SEL_MASK), IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to read new TSS descriptor in GDT (2). enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pCtx->gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* Check that the descriptor indicates the new TSS is available (not busy). */
        AssertMsg(   pNewDescTSS->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_286_TSS_AVAIL
                  || pNewDescTSS->Legacy.Gate.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL,
                     ("Invalid TSS descriptor type=%#x", pNewDescTSS->Legacy.Gate.u4Type));

        pNewDescTSS->Legacy.Gate.u4Type |= X86_SEL_TYPE_SYS_TSS_BUSY_MASK;
        rcStrict = iemMemCommitAndUnmap(pVCpu, pNewDescTSS, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Failed to commit new TSS descriptor in GDT (2). enmTaskSwitch=%u pGdt=%#RX64 rc=%Rrc\n",
                 enmTaskSwitch, pCtx->gdtr.pGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /*
     * From this point on, we're technically in the new task. We will defer exceptions
     * until the completion of the task switch but before executing any instructions in the new task.
     */
    pCtx->tr.Sel      = SelTSS;
    pCtx->tr.ValidSel = SelTSS;
    pCtx->tr.fFlags   = CPUMSELREG_FLAGS_VALID;
    pCtx->tr.Attr.u   = X86DESC_GET_HID_ATTR(&pNewDescTSS->Legacy);
    pCtx->tr.u32Limit = X86DESC_LIMIT_G(&pNewDescTSS->Legacy);
    pCtx->tr.u64Base  = X86DESC_BASE(&pNewDescTSS->Legacy);
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_TR);

    /* Set the busy bit in TR. */
    pCtx->tr.Attr.n.u4Type |= X86_SEL_TYPE_SYS_TSS_BUSY_MASK;
    /* Set EFLAGS.NT (Nested Task) in the eflags loaded from the new TSS, if it's a task switch due to a CALL/INT_XCPT. */
    if (   enmTaskSwitch == IEMTASKSWITCH_CALL
        || enmTaskSwitch == IEMTASKSWITCH_INT_XCPT)
    {
        uNewEflags |= X86_EFL_NT;
    }

    pCtx->dr[7] &= ~X86_DR7_LE_ALL;     /** @todo Should we clear DR7.LE bit too? */
    pCtx->cr0   |= X86_CR0_TS;
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_CR0);

    pCtx->eip    = uNewEip;
    pCtx->eax    = uNewEax;
    pCtx->ecx    = uNewEcx;
    pCtx->edx    = uNewEdx;
    pCtx->ebx    = uNewEbx;
    pCtx->esp    = uNewEsp;
    pCtx->ebp    = uNewEbp;
    pCtx->esi    = uNewEsi;
    pCtx->edi    = uNewEdi;

    uNewEflags &= X86_EFL_LIVE_MASK;
    uNewEflags |= X86_EFL_RA1_MASK;
    IEMMISC_SET_EFL(pVCpu, pCtx, uNewEflags);

    /*
     * Switch the selectors here and do the segment checks later. If we throw exceptions, the selectors
     * will be valid in the exception handler. We cannot update the hidden parts until we've switched CR3
     * due to the hidden part data originating from the guest LDT/GDT which is accessed through paging.
     */
    pCtx->es.Sel       = uNewES;
    pCtx->es.Attr.u   &= ~X86DESCATTR_P;

    pCtx->cs.Sel       = uNewCS;
    pCtx->cs.Attr.u   &= ~X86DESCATTR_P;

    pCtx->ss.Sel       = uNewSS;
    pCtx->ss.Attr.u   &= ~X86DESCATTR_P;

    pCtx->ds.Sel       = uNewDS;
    pCtx->ds.Attr.u   &= ~X86DESCATTR_P;

    pCtx->fs.Sel       = uNewFS;
    pCtx->fs.Attr.u   &= ~X86DESCATTR_P;

    pCtx->gs.Sel       = uNewGS;
    pCtx->gs.Attr.u   &= ~X86DESCATTR_P;
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_HIDDEN_SEL_REGS);

    pCtx->ldtr.Sel     = uNewLdt;
    pCtx->ldtr.fFlags  = CPUMSELREG_FLAGS_STALE;
    pCtx->ldtr.Attr.u &= ~X86DESCATTR_P;
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_LDTR);

    if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
    {
        pCtx->es.Attr.u   |= X86DESCATTR_UNUSABLE;
        pCtx->cs.Attr.u   |= X86DESCATTR_UNUSABLE;
        pCtx->ss.Attr.u   |= X86DESCATTR_UNUSABLE;
        pCtx->ds.Attr.u   |= X86DESCATTR_UNUSABLE;
        pCtx->fs.Attr.u   |= X86DESCATTR_UNUSABLE;
        pCtx->gs.Attr.u   |= X86DESCATTR_UNUSABLE;
        pCtx->ldtr.Attr.u |= X86DESCATTR_UNUSABLE;
    }

    /*
     * Switch CR3 for the new task.
     */
    if (   fIsNewTSS386
        && (pCtx->cr0 & X86_CR0_PG))
    {
        /** @todo Should we update and flush TLBs only if CR3 value actually changes? */
        if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
        {
            int rc = CPUMSetGuestCR3(pVCpu, uNewCr3);
            AssertRCSuccessReturn(rc, rc);
        }
        else
            pCtx->cr3 = uNewCr3;

        /* Inform PGM. */
        if (!IEM_FULL_VERIFICATION_ENABLED(pVCpu))
        {
            int rc = PGMFlushTLB(pVCpu, pCtx->cr3, !(pCtx->cr4 & X86_CR4_PGE));
            AssertRCReturn(rc, rc);
            /* ignore informational status codes */
        }
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_CR3);
    }

    /*
     * Switch LDTR for the new task.
     */
    if (!(uNewLdt & X86_SEL_MASK_OFF_RPL))
        iemHlpLoadNullDataSelectorProt(pVCpu, &pCtx->ldtr, uNewLdt);
    else
    {
        Assert(!pCtx->ldtr.Attr.n.u1Present);       /* Ensures that LDT.TI check passes in iemMemFetchSelDesc() below. */

        IEMSELDESC DescNewLdt;
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescNewLdt, uNewLdt, X86_XCPT_TS);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: fetching LDT failed. enmTaskSwitch=%u uNewLdt=%u cbGdt=%u rc=%Rrc\n", enmTaskSwitch,
                 uNewLdt, pCtx->gdtr.cbGdt, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
        if (   !DescNewLdt.Legacy.Gen.u1Present
            ||  DescNewLdt.Legacy.Gen.u1DescType
            ||  DescNewLdt.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_LDT)
        {
            Log(("iemTaskSwitch: Invalid LDT. enmTaskSwitch=%u uNewLdt=%u DescNewLdt.Legacy.u=%#RX64 -> #TS\n", enmTaskSwitch,
                 uNewLdt, DescNewLdt.Legacy.u));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewLdt & X86_SEL_MASK_OFF_RPL);
        }

        pCtx->ldtr.ValidSel = uNewLdt;
        pCtx->ldtr.fFlags   = CPUMSELREG_FLAGS_VALID;
        pCtx->ldtr.u64Base  = X86DESC_BASE(&DescNewLdt.Legacy);
        pCtx->ldtr.u32Limit = X86DESC_LIMIT_G(&DescNewLdt.Legacy);
        pCtx->ldtr.Attr.u   = X86DESC_GET_HID_ATTR(&DescNewLdt.Legacy);
        if (IEM_IS_GUEST_CPU_INTEL(pVCpu) && !IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
            pCtx->ldtr.Attr.u &= ~X86DESCATTR_UNUSABLE;
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ldtr));
    }

    IEMSELDESC DescSS;
    if (IEM_IS_V86_MODE(pVCpu))
    {
        pVCpu->iem.s.uCpl = 3;
        iemHlpLoadSelectorInV86Mode(&pCtx->es, uNewES);
        iemHlpLoadSelectorInV86Mode(&pCtx->cs, uNewCS);
        iemHlpLoadSelectorInV86Mode(&pCtx->ss, uNewSS);
        iemHlpLoadSelectorInV86Mode(&pCtx->ds, uNewDS);
        iemHlpLoadSelectorInV86Mode(&pCtx->fs, uNewFS);
        iemHlpLoadSelectorInV86Mode(&pCtx->gs, uNewGS);

        /* quick fix: fake DescSS. */ /** @todo fix the code further down? */
        DescSS.Legacy.u = 0;
        DescSS.Legacy.Gen.u16LimitLow = (uint16_t)pCtx->ss.u32Limit;
        DescSS.Legacy.Gen.u4LimitHigh = pCtx->ss.u32Limit >> 16;
        DescSS.Legacy.Gen.u16BaseLow  = (uint16_t)pCtx->ss.u64Base;
        DescSS.Legacy.Gen.u8BaseHigh1 = (uint8_t)(pCtx->ss.u64Base >> 16);
        DescSS.Legacy.Gen.u8BaseHigh2 = (uint8_t)(pCtx->ss.u64Base >> 24);
        DescSS.Legacy.Gen.u4Type      = X86_SEL_TYPE_RW_ACC;
        DescSS.Legacy.Gen.u2Dpl       = 3;
    }
    else
    {
        uint8_t uNewCpl = (uNewCS & X86_SEL_RPL);

        /*
         * Load the stack segment for the new task.
         */
        if (!(uNewSS & X86_SEL_MASK_OFF_RPL))
        {
            Log(("iemTaskSwitch: Null stack segment. enmTaskSwitch=%u uNewSS=%#x -> #TS\n", enmTaskSwitch, uNewSS));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        /* Fetch the descriptor. */
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescSS, uNewSS, X86_XCPT_TS);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: failed to fetch SS. uNewSS=%#x rc=%Rrc\n", uNewSS,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* SS must be a data segment and writable. */
        if (    !DescSS.Legacy.Gen.u1DescType
            ||  (DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE)
            || !(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_WRITE))
        {
            Log(("iemTaskSwitch: SS invalid descriptor type. uNewSS=%#x u1DescType=%u u4Type=%#x\n",
                 uNewSS, DescSS.Legacy.Gen.u1DescType, DescSS.Legacy.Gen.u4Type));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        /* The SS.RPL, SS.DPL, CS.RPL (CPL) must be equal. */
        if (   (uNewSS & X86_SEL_RPL) != uNewCpl
            || DescSS.Legacy.Gen.u2Dpl != uNewCpl)
        {
            Log(("iemTaskSwitch: Invalid priv. for SS. uNewSS=%#x SS.DPL=%u uNewCpl=%u -> #TS\n", uNewSS, DescSS.Legacy.Gen.u2Dpl,
                 uNewCpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        /* Is it there? */
        if (!DescSS.Legacy.Gen.u1Present)
        {
            Log(("iemTaskSwitch: SS not present. uNewSS=%#x -> #NP\n", uNewSS));
            return iemRaiseSelectorNotPresentWithErr(pVCpu, uNewSS & X86_SEL_MASK_OFF_RPL);
        }

        uint32_t cbLimit = X86DESC_LIMIT_G(&DescSS.Legacy);
        uint64_t u64Base = X86DESC_BASE(&DescSS.Legacy);

        /* Set the accessed bit before committing the result into SS. */
        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewSS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* Commit SS. */
        pCtx->ss.Sel      = uNewSS;
        pCtx->ss.ValidSel = uNewSS;
        pCtx->ss.Attr.u   = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
        pCtx->ss.u32Limit = cbLimit;
        pCtx->ss.u64Base  = u64Base;
        pCtx->ss.fFlags   = CPUMSELREG_FLAGS_VALID;
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->ss));

        /* CPL has changed, update IEM before loading rest of segments. */
        pVCpu->iem.s.uCpl = uNewCpl;

        /*
         * Load the data segments for the new task.
         */
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pCtx->es, uNewES);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pCtx->ds, uNewDS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pCtx->fs, uNewFS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        rcStrict = iemHlpTaskSwitchLoadDataSelectorInProtMode(pVCpu, &pCtx->gs, uNewGS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /*
         * Load the code segment for the new task.
         */
        if (!(uNewCS & X86_SEL_MASK_OFF_RPL))
        {
            Log(("iemTaskSwitch #TS: Null code segment. enmTaskSwitch=%u uNewCS=%#x\n", enmTaskSwitch, uNewCS));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* Fetch the descriptor. */
        IEMSELDESC DescCS;
        rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, uNewCS, X86_XCPT_TS);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: failed to fetch CS. uNewCS=%u rc=%Rrc\n", uNewCS, VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* CS must be a code segment. */
        if (   !DescCS.Legacy.Gen.u1DescType
            || !(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
        {
            Log(("iemTaskSwitch: CS invalid descriptor type. uNewCS=%#x u1DescType=%u u4Type=%#x -> #TS\n", uNewCS,
                 DescCS.Legacy.Gen.u1DescType, DescCS.Legacy.Gen.u4Type));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* For conforming CS, DPL must be less than or equal to the RPL. */
        if (   (DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
            && DescCS.Legacy.Gen.u2Dpl > (uNewCS & X86_SEL_RPL))
        {
            Log(("iemTaskSwitch: confirming CS DPL > RPL. uNewCS=%#x u4Type=%#x DPL=%u -> #TS\n", uNewCS, DescCS.Legacy.Gen.u4Type,
                 DescCS.Legacy.Gen.u2Dpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* For non-conforming CS, DPL must match RPL. */
        if (   !(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF)
            && DescCS.Legacy.Gen.u2Dpl != (uNewCS & X86_SEL_RPL))
        {
            Log(("iemTaskSwitch: non-confirming CS DPL RPL mismatch. uNewCS=%#x u4Type=%#x DPL=%u -> #TS\n", uNewCS,
                 DescCS.Legacy.Gen.u4Type, DescCS.Legacy.Gen.u2Dpl));
            return iemRaiseTaskSwitchFaultWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        /* Is it there? */
        if (!DescCS.Legacy.Gen.u1Present)
        {
            Log(("iemTaskSwitch: CS not present. uNewCS=%#x -> #NP\n", uNewCS));
            return iemRaiseSelectorNotPresentWithErr(pVCpu, uNewCS & X86_SEL_MASK_OFF_RPL);
        }

        cbLimit = X86DESC_LIMIT_G(&DescCS.Legacy);
        u64Base = X86DESC_BASE(&DescCS.Legacy);

        /* Set the accessed bit before committing the result into CS. */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, uNewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /* Commit CS. */
        pCtx->cs.Sel      = uNewCS;
        pCtx->cs.ValidSel = uNewCS;
        pCtx->cs.Attr.u   = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
        pCtx->cs.u32Limit = cbLimit;
        pCtx->cs.u64Base  = u64Base;
        pCtx->cs.fFlags   = CPUMSELREG_FLAGS_VALID;
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &pCtx->cs));
    }

    /** @todo Debug trap. */
    if (fIsNewTSS386 && fNewDebugTrap)
        Log(("iemTaskSwitch: Debug Trap set in new TSS. Not implemented!\n"));

    /*
     * Construct the error code masks based on what caused this task switch.
     * See Intel Instruction reference for INT.
     */
    uint16_t uExt;
    if (   enmTaskSwitch == IEMTASKSWITCH_INT_XCPT
        && !(fFlags & IEM_XCPT_FLAGS_T_SOFT_INT))
    {
        uExt = 1;
    }
    else
        uExt = 0;

    /*
     * Push any error code on to the new stack.
     */
    if (fFlags & IEM_XCPT_FLAGS_ERR)
    {
        Assert(enmTaskSwitch == IEMTASKSWITCH_INT_XCPT);
        uint32_t cbLimitSS = X86DESC_LIMIT_G(&DescSS.Legacy);
        uint8_t const cbStackFrame = fIsNewTSS386 ? 4 : 2;

        /* Check that there is sufficient space on the stack. */
        /** @todo Factor out segment limit checking for normal/expand down segments
         *        into a separate function. */
        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_DOWN))
        {
            if (   pCtx->esp - 1 > cbLimitSS
                || pCtx->esp < cbStackFrame)
            {
                /** @todo Intel says \#SS(EXT) for INT/XCPT, I couldn't figure out AMD yet. */
                Log(("iemTaskSwitch: SS=%#x ESP=%#x cbStackFrame=%#x is out of bounds -> #SS\n",
                     pCtx->ss.Sel, pCtx->esp, cbStackFrame));
                return iemRaiseStackSelectorNotPresentWithErr(pVCpu, uExt);
            }
        }
        else
        {
            if (   pCtx->esp - 1 > (DescSS.Legacy.Gen.u1DefBig ? UINT32_MAX : UINT32_C(0xffff))
                || pCtx->esp - cbStackFrame < cbLimitSS + UINT32_C(1))
            {
                Log(("iemTaskSwitch: SS=%#x ESP=%#x cbStackFrame=%#x (expand down) is out of bounds -> #SS\n",
                     pCtx->ss.Sel, pCtx->esp, cbStackFrame));
                return iemRaiseStackSelectorNotPresentWithErr(pVCpu, uExt);
            }
        }


        if (fIsNewTSS386)
            rcStrict = iemMemStackPushU32(pVCpu, uErr);
        else
            rcStrict = iemMemStackPushU16(pVCpu, uErr);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("iemTaskSwitch: Can't push error code to new task's stack. %s-bit TSS. rc=%Rrc\n",
                 fIsNewTSS386 ? "32" : "16", VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }
    }

    /* Check the new EIP against the new CS limit. */
    if (pCtx->eip > pCtx->cs.u32Limit)
    {
        Log(("iemHlpTaskSwitchLoadDataSelectorInProtMode: New EIP exceeds CS limit. uNewEIP=%#RX32 CS limit=%u -> #GP(0)\n",
             pCtx->eip, pCtx->cs.u32Limit));
        /** @todo Intel says \#GP(EXT) for INT/XCPT, I couldn't figure out AMD yet. */
        return iemRaiseGeneralProtectionFault(pVCpu, uExt);
    }

    Log(("iemTaskSwitch: Success! New CS:EIP=%#04x:%#x SS=%#04x\n", pCtx->cs.Sel, pCtx->eip, pCtx->ss.Sel));
    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Implements exceptions and interrupts for protected mode.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pCtx            The CPU context.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
IEM_STATIC VBOXSTRICTRC
iemRaiseXcptOrIntInProtMode(PVMCPU      pVCpu,
                            PCPUMCTX    pCtx,
                            uint8_t     cbInstr,
                            uint8_t     u8Vector,
                            uint32_t    fFlags,
                            uint16_t    uErr,
                            uint64_t    uCr2)
{
    /*
     * Read the IDT entry.
     */
    if (pCtx->idtr.cbIdt < UINT32_C(8) * u8Vector + 7)
    {
        Log(("RaiseXcptOrIntInProtMode: %#x is out of bounds (%#x)\n", u8Vector, pCtx->idtr.cbIdt));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    X86DESC Idte;
    VBOXSTRICTRC rcStrict = iemMemFetchSysU64(pVCpu, &Idte.u, UINT8_MAX,
                                              pCtx->idtr.pIdt + UINT32_C(8) * u8Vector);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
        return rcStrict;
    Log(("iemRaiseXcptOrIntInProtMode: vec=%#x P=%u DPL=%u DT=%u:%u A=%u %04x:%04x%04x\n",
         u8Vector, Idte.Gate.u1Present, Idte.Gate.u2Dpl, Idte.Gate.u1DescType, Idte.Gate.u4Type,
         Idte.Gate.u5ParmCount, Idte.Gate.u16Sel, Idte.Gate.u16OffsetHigh, Idte.Gate.u16OffsetLow));

    /*
     * Check the descriptor type, DPL and such.
     * ASSUMES this is done in the same order as described for call-gate calls.
     */
    if (Idte.Gate.u1DescType)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - not system selector (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    bool     fTaskGate   = false;
    uint8_t  f32BitGate  = true;
    uint32_t fEflToClear = X86_EFL_TF | X86_EFL_NT | X86_EFL_RF | X86_EFL_VM;
    switch (Idte.Gate.u4Type)
    {
        case X86_SEL_TYPE_SYS_UNDEFINED:
        case X86_SEL_TYPE_SYS_286_TSS_AVAIL:
        case X86_SEL_TYPE_SYS_LDT:
        case X86_SEL_TYPE_SYS_286_TSS_BUSY:
        case X86_SEL_TYPE_SYS_286_CALL_GATE:
        case X86_SEL_TYPE_SYS_UNDEFINED2:
        case X86_SEL_TYPE_SYS_386_TSS_AVAIL:
        case X86_SEL_TYPE_SYS_UNDEFINED3:
        case X86_SEL_TYPE_SYS_386_TSS_BUSY:
        case X86_SEL_TYPE_SYS_386_CALL_GATE:
        case X86_SEL_TYPE_SYS_UNDEFINED4:
        {
            /** @todo check what actually happens when the type is wrong...
             *        esp. call gates. */
            Log(("RaiseXcptOrIntInProtMode %#x - invalid type (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
        }

        case X86_SEL_TYPE_SYS_286_INT_GATE:
            f32BitGate = false;
            RT_FALL_THRU();
        case X86_SEL_TYPE_SYS_386_INT_GATE:
            fEflToClear |= X86_EFL_IF;
            break;

        case X86_SEL_TYPE_SYS_TASK_GATE:
            fTaskGate = true;
#ifndef IEM_IMPLEMENTS_TASKSWITCH
            IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(("Task gates\n"));
#endif
            break;

        case X86_SEL_TYPE_SYS_286_TRAP_GATE:
            f32BitGate = false;
        case X86_SEL_TYPE_SYS_386_TRAP_GATE:
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    /* Check DPL against CPL if applicable. */
    if (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT)
    {
        if (pVCpu->iem.s.uCpl > Idte.Gate.u2Dpl)
        {
            Log(("RaiseXcptOrIntInProtMode %#x - CPL (%d) > DPL (%d) -> #GP\n", u8Vector, pVCpu->iem.s.uCpl, Idte.Gate.u2Dpl));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
        }
    }

    /* Is it there? */
    if (!Idte.Gate.u1Present)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - not present -> #NP\n", u8Vector));
        return iemRaiseSelectorNotPresentWithErr(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }

    /* Is it a task-gate? */
    if (fTaskGate)
    {
        /*
         * Construct the error code masks based on what caused this task switch.
         * See Intel Instruction reference for INT.
         */
        uint16_t const uExt     = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? 0 : 1;
        uint16_t const uSelMask = X86_SEL_MASK_OFF_RPL;
        RTSEL          SelTSS   = Idte.Gate.u16Sel;

        /*
         * Fetch the TSS descriptor in the GDT.
         */
        IEMSELDESC DescTSS;
        rcStrict = iemMemFetchSelDescWithErr(pVCpu, &DescTSS, SelTSS, X86_XCPT_GP, (SelTSS & uSelMask) | uExt);
        if (rcStrict != VINF_SUCCESS)
        {
            Log(("RaiseXcptOrIntInProtMode %#x - failed to fetch TSS selector %#x, rc=%Rrc\n", u8Vector, SelTSS,
                 VBOXSTRICTRC_VAL(rcStrict)));
            return rcStrict;
        }

        /* The TSS descriptor must be a system segment and be available (not busy). */
        if (   DescTSS.Legacy.Gen.u1DescType
            || (   DescTSS.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_286_TSS_AVAIL
                && DescTSS.Legacy.Gen.u4Type != X86_SEL_TYPE_SYS_386_TSS_AVAIL))
        {
            Log(("RaiseXcptOrIntInProtMode %#x - TSS selector %#x of task gate not a system descriptor or not available %#RX64\n",
                 u8Vector, SelTSS, DescTSS.Legacy.au64));
            return iemRaiseGeneralProtectionFault(pVCpu, (SelTSS & uSelMask) | uExt);
        }

        /* The TSS must be present. */
        if (!DescTSS.Legacy.Gen.u1Present)
        {
            Log(("RaiseXcptOrIntInProtMode %#x - TSS selector %#x not present %#RX64\n", u8Vector, SelTSS, DescTSS.Legacy.au64));
            return iemRaiseSelectorNotPresentWithErr(pVCpu, (SelTSS & uSelMask) | uExt);
        }

        /* Do the actual task switch. */
        return iemTaskSwitch(pVCpu, pCtx, IEMTASKSWITCH_INT_XCPT, (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pVCpu->cpum.GstCtx.eip + cbInstr : pVCpu->cpum.GstCtx.eip, fFlags, uErr, uCr2, SelTSS, &DescTSS);
    }

    /* A null CS is bad. */
    RTSEL NewCS = Idte.Gate.u16Sel;
    if (!(NewCS & X86_SEL_MASK_OFF_RPL))
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x -> #GP\n", u8Vector, NewCS));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor for the new CS. */
    IEMSELDESC DescCS;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, NewCS, X86_XCPT_GP); /** @todo correct exception? */
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - rc=%Rrc\n", u8Vector, NewCS, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a code segment. */
    if (!DescCS.Legacy.Gen.u1DescType)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - system selector (%#x) -> #GP\n", u8Vector, NewCS, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CODE))
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - data selector (%#x) -> #GP\n", u8Vector, NewCS, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }

    /* Don't allow lowering the privilege level. */
    /** @todo Does the lowering of privileges apply to software interrupts
     *        only?  This has bearings on the more-privileged or
     *        same-privilege stack behavior further down.  A testcase would
     *        be nice. */
    if (DescCS.Legacy.Gen.u2Dpl > pVCpu->iem.s.uCpl)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - DPL (%d) > CPL (%d) -> #GP\n",
             u8Vector, NewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }

    /* Make sure the selector is present. */
    if (!DescCS.Legacy.Gen.u1Present)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - segment not present -> #NP\n", u8Vector, NewCS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, NewCS);
    }

    /* Check the new EIP against the new CS limit. */
    uint32_t const uNewEip =    Idte.Gate.u4Type == X86_SEL_TYPE_SYS_286_INT_GATE
                             || Idte.Gate.u4Type == X86_SEL_TYPE_SYS_286_TRAP_GATE
                           ? Idte.Gate.u16OffsetLow
                           : Idte.Gate.u16OffsetLow | ((uint32_t)Idte.Gate.u16OffsetHigh << 16);
    uint32_t cbLimitCS = X86DESC_LIMIT_G(&DescCS.Legacy);
    if (uNewEip > cbLimitCS)
    {
        Log(("RaiseXcptOrIntInProtMode %#x - EIP=%#x > cbLimitCS=%#x (CS=%#x) -> #GP(0)\n",
             u8Vector, uNewEip, cbLimitCS, NewCS));
        return iemRaiseGeneralProtectionFault(pVCpu, 0);
    }
    Log7(("iemRaiseXcptOrIntInProtMode: new EIP=%#x CS=%#x\n", uNewEip, NewCS));

    /* Calc the flag image to push. */
    uint32_t        fEfl    = IEMMISC_GET_EFL(pVCpu, pCtx);
    if (fFlags & (IEM_XCPT_FLAGS_DRx_INSTR_BP | IEM_XCPT_FLAGS_T_SOFT_INT))
        fEfl &= ~X86_EFL_RF;
    else if (!IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
        fEfl |= X86_EFL_RF; /* Vagueness is all I've found on this so far... */ /** @todo Automatically pushing EFLAGS.RF. */

    /* From V8086 mode only go to CPL 0. */
    uint8_t const   uNewCpl = DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF
                            ? pVCpu->iem.s.uCpl : DescCS.Legacy.Gen.u2Dpl;
    if ((fEfl & X86_EFL_VM) && uNewCpl != 0) /** @todo When exactly is this raised? */
    {
        Log(("RaiseXcptOrIntInProtMode %#x - CS=%#x - New CPL (%d) != 0 w/ VM=1 -> #GP\n", u8Vector, NewCS, uNewCpl));
        return iemRaiseGeneralProtectionFault(pVCpu, 0);
    }

    /*
     * If the privilege level changes, we need to get a new stack from the TSS.
     * This in turns means validating the new SS and ESP...
     */
    if (uNewCpl != pVCpu->iem.s.uCpl)
    {
        RTSEL    NewSS;
        uint32_t uNewEsp;
        rcStrict = iemRaiseLoadStackFromTss32Or16(pVCpu, pCtx, uNewCpl, &NewSS, &uNewEsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        IEMSELDESC DescSS;
        rcStrict = iemMiscValidateNewSS(pVCpu, pCtx, NewSS, uNewCpl, &DescSS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        /* If the new SS is 16-bit, we are only going to use SP, not ESP. */
        if (!DescSS.Legacy.Gen.u1DefBig)
        {
            Log(("iemRaiseXcptOrIntInProtMode: Forcing ESP=%#x to 16 bits\n", uNewEsp));
            uNewEsp = (uint16_t)uNewEsp;
        }

        Log7(("iemRaiseXcptOrIntInProtMode: New SS=%#x ESP=%#x (from TSS); current SS=%#x ESP=%#x\n", NewSS, uNewEsp, pCtx->ss.Sel, pCtx->esp));

        /* Check that there is sufficient space for the stack frame. */
        uint32_t cbLimitSS = X86DESC_LIMIT_G(&DescSS.Legacy);
        uint8_t const cbStackFrame = !(fEfl & X86_EFL_VM)
                                   ? (fFlags & IEM_XCPT_FLAGS_ERR ? 12 : 10) << f32BitGate
                                   : (fFlags & IEM_XCPT_FLAGS_ERR ? 20 : 18) << f32BitGate;

        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_DOWN))
        {
            if (   uNewEsp - 1 > cbLimitSS
                || uNewEsp < cbStackFrame)
            {
                Log(("RaiseXcptOrIntInProtMode: %#x - SS=%#x ESP=%#x cbStackFrame=%#x is out of bounds -> #GP\n",
                     u8Vector, NewSS, uNewEsp, cbStackFrame));
                return iemRaiseSelectorBoundsBySelector(pVCpu, NewSS);
            }
        }
        else
        {
            if (   uNewEsp - 1 > (DescSS.Legacy.Gen.u1DefBig ? UINT32_MAX : UINT16_MAX)
                || uNewEsp - cbStackFrame < cbLimitSS + UINT32_C(1))
            {
                Log(("RaiseXcptOrIntInProtMode: %#x - SS=%#x ESP=%#x cbStackFrame=%#x (expand down) is out of bounds -> #GP\n",
                     u8Vector, NewSS, uNewEsp, cbStackFrame));
                return iemRaiseSelectorBoundsBySelector(pVCpu, NewSS);
            }
        }

        /*
         * Start making changes.
         */

        /* Set the new CPL so that stack accesses use it. */
        uint8_t const uOldCpl = pVCpu->iem.s.uCpl;
        pVCpu->iem.s.uCpl = uNewCpl;

        /* Create the stack frame. */
        RTPTRUNION uStackFrame;
        rcStrict = iemMemMap(pVCpu, &uStackFrame.pv, cbStackFrame, UINT8_MAX,
                             uNewEsp - cbStackFrame + X86DESC_BASE(&DescSS.Legacy), IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS); /* _SYS is a hack ... */
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        void * const pvStackFrame = uStackFrame.pv;
        if (f32BitGate)
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu32++ = uErr;
            uStackFrame.pu32[0] = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pCtx->eip + cbInstr : pCtx->eip;
            uStackFrame.pu32[1] = (pCtx->cs.Sel & ~X86_SEL_RPL) | uOldCpl;
            uStackFrame.pu32[2] = fEfl;
            uStackFrame.pu32[3] = pCtx->esp;
            uStackFrame.pu32[4] = pCtx->ss.Sel;
            Log7(("iemRaiseXcptOrIntInProtMode: 32-bit push SS=%#x ESP=%#x\n", pCtx->ss.Sel, pCtx->esp));
            if (fEfl & X86_EFL_VM)
            {
                uStackFrame.pu32[1] = pCtx->cs.Sel;
                uStackFrame.pu32[5] = pCtx->es.Sel;
                uStackFrame.pu32[6] = pCtx->ds.Sel;
                uStackFrame.pu32[7] = pCtx->fs.Sel;
                uStackFrame.pu32[8] = pCtx->gs.Sel;
            }
        }
        else
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu16++ = uErr;
            uStackFrame.pu16[0] = (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT) ? pCtx->ip + cbInstr : pCtx->ip;
            uStackFrame.pu16[1] = (pCtx->cs.Sel & ~X86_SEL_RPL) | uOldCpl;
            uStackFrame.pu16[2] = fEfl;
            uStackFrame.pu16[3] = pCtx->sp;
            uStackFrame.pu16[4] = pCtx->ss.Sel;
            Log7(("iemRaiseXcptOrIntInProtMode: 16-bit push SS=%#x SP=%#x\n", pCtx->ss.Sel, pCtx->sp));
            if (fEfl & X86_EFL_VM)
            {
                uStackFrame.pu16[1] = pCtx->cs.Sel;
                uStackFrame.pu16[5] = pCtx->es.Sel;
                uStackFrame.pu16[6] = pCtx->ds.Sel;
                uStackFrame.pu16[7] = pCtx->fs.Sel;
                uStackFrame.pu16[8] = pCtx->gs.Sel;
            }
        }
        rcStrict = iemMemCommitAndUnmap(pVCpu, pvStackFrame, IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /* Mark the selectors 'accessed' (hope this is the correct time). */
        /** @todo testcase: excatly _when_ are the accessed bits set - before or
         *        after pushing the stack frame? (Write protect the gdt + stack to
         *        find out.) */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        if (!(DescSS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewSS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescSS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /*
         * Start comitting the register changes (joins with the DPL=CPL branch).
         */
        pCtx->ss.Sel            = NewSS;
        pCtx->ss.ValidSel       = NewSS;
        pCtx->ss.fFlags         = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.u32Limit       = cbLimitSS;
        pCtx->ss.u64Base        = X86DESC_BASE(&DescSS.Legacy);
        pCtx->ss.Attr.u         = X86DESC_GET_HID_ATTR(&DescSS.Legacy);
        /** @todo When coming from 32-bit code and operating with a 16-bit TSS and
         *        16-bit handler, the high word of ESP remains unchanged (i.e. only
         *        SP is loaded).
         *  Need to check the other combinations too:
         *      - 16-bit TSS, 32-bit handler
         *      - 32-bit TSS, 16-bit handler */
        if (!pCtx->ss.Attr.n.u1DefBig)
            pCtx->sp            = (uint16_t)(uNewEsp - cbStackFrame);
        else
            pCtx->rsp           = uNewEsp - cbStackFrame;

        if (fEfl & X86_EFL_VM)
        {
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pCtx->gs);
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pCtx->fs);
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pCtx->es);
            iemHlpLoadNullDataSelectorOnV86Xcpt(pVCpu, &pCtx->ds);
        }
    }
    /*
     * Same privilege, no stack change and smaller stack frame.
     */
    else
    {
        uint64_t        uNewRsp;
        RTPTRUNION      uStackFrame;
        uint8_t const   cbStackFrame = (fFlags & IEM_XCPT_FLAGS_ERR ? 8 : 6) << f32BitGate;
        rcStrict = iemMemStackPushBeginSpecial(pVCpu, cbStackFrame, &uStackFrame.pv, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        void * const pvStackFrame = uStackFrame.pv;

        if (f32BitGate)
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu32++ = uErr;
            uStackFrame.pu32[0] = fFlags & IEM_XCPT_FLAGS_T_SOFT_INT ? pCtx->eip + cbInstr : pCtx->eip;
            uStackFrame.pu32[1] = (pCtx->cs.Sel & ~X86_SEL_RPL) | pVCpu->iem.s.uCpl;
            uStackFrame.pu32[2] = fEfl;
        }
        else
        {
            if (fFlags & IEM_XCPT_FLAGS_ERR)
                *uStackFrame.pu16++ = uErr;
            uStackFrame.pu16[0] = fFlags & IEM_XCPT_FLAGS_T_SOFT_INT ? pCtx->eip + cbInstr : pCtx->eip;
            uStackFrame.pu16[1] = (pCtx->cs.Sel & ~X86_SEL_RPL) | pVCpu->iem.s.uCpl;
            uStackFrame.pu16[2] = fEfl;
        }
        rcStrict = iemMemCommitAndUnmap(pVCpu, pvStackFrame, IEM_ACCESS_STACK_W); /* don't use the commit here */
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;

        /* Mark the CS selector as 'accessed'. */
        if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
        {
            rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewCS);
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
            DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
        }

        /*
         * Start committing the register changes (joins with the other branch).
         */
        pCtx->rsp = uNewRsp;
    }

    /* ... register committing continues. */
    pCtx->cs.Sel            = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pCtx->cs.ValidSel       = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pCtx->cs.fFlags         = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.u32Limit       = cbLimitCS;
    pCtx->cs.u64Base        = X86DESC_BASE(&DescCS.Legacy);
    pCtx->cs.Attr.u         = X86DESC_GET_HID_ATTR(&DescCS.Legacy);

    pCtx->rip               = uNewEip;  /* (The entire register is modified, see pe16_32 bs3kit tests.) */
    fEfl &= ~fEflToClear;
    IEMMISC_SET_EFL(pVCpu, pCtx, fEfl);

    if (fFlags & IEM_XCPT_FLAGS_CR2)
        pCtx->cr2 = uCr2;

    if (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        iemRaiseXcptAdjustState(pCtx, u8Vector);

    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Implements exceptions and interrupts for long mode.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pCtx            The CPU context.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
IEM_STATIC VBOXSTRICTRC
iemRaiseXcptOrIntInLongMode(PVMCPU      pVCpu,
                            PCPUMCTX    pCtx,
                            uint8_t     cbInstr,
                            uint8_t     u8Vector,
                            uint32_t    fFlags,
                            uint16_t    uErr,
                            uint64_t    uCr2)
{
    /*
     * Read the IDT entry.
     */
    uint16_t offIdt = (uint16_t)u8Vector << 4;
    if (pCtx->idtr.cbIdt < offIdt + 7)
    {
        Log(("iemRaiseXcptOrIntInLongMode: %#x is out of bounds (%#x)\n", u8Vector, pCtx->idtr.cbIdt));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    X86DESC64 Idte;
    VBOXSTRICTRC rcStrict = iemMemFetchSysU64(pVCpu, &Idte.au64[0], UINT8_MAX, pCtx->idtr.pIdt + offIdt);
    if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        rcStrict = iemMemFetchSysU64(pVCpu, &Idte.au64[1], UINT8_MAX, pCtx->idtr.pIdt + offIdt + 8);
    if (RT_UNLIKELY(rcStrict != VINF_SUCCESS))
        return rcStrict;
    Log(("iemRaiseXcptOrIntInLongMode: vec=%#x P=%u DPL=%u DT=%u:%u IST=%u %04x:%08x%04x%04x\n",
         u8Vector, Idte.Gate.u1Present, Idte.Gate.u2Dpl, Idte.Gate.u1DescType, Idte.Gate.u4Type,
         Idte.Gate.u3IST, Idte.Gate.u16Sel, Idte.Gate.u32OffsetTop, Idte.Gate.u16OffsetHigh, Idte.Gate.u16OffsetLow));

    /*
     * Check the descriptor type, DPL and such.
     * ASSUMES this is done in the same order as described for call-gate calls.
     */
    if (Idte.Gate.u1DescType)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - not system selector (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }
    uint32_t fEflToClear = X86_EFL_TF | X86_EFL_NT | X86_EFL_RF | X86_EFL_VM;
    switch (Idte.Gate.u4Type)
    {
        case AMD64_SEL_TYPE_SYS_INT_GATE:
            fEflToClear |= X86_EFL_IF;
            break;
        case AMD64_SEL_TYPE_SYS_TRAP_GATE:
            break;

        default:
            Log(("iemRaiseXcptOrIntInLongMode %#x - invalid type (%#x) -> #GP\n", u8Vector, Idte.Gate.u4Type));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }

    /* Check DPL against CPL if applicable. */
    if (fFlags & IEM_XCPT_FLAGS_T_SOFT_INT)
    {
        if (pVCpu->iem.s.uCpl > Idte.Gate.u2Dpl)
        {
            Log(("iemRaiseXcptOrIntInLongMode %#x - CPL (%d) > DPL (%d) -> #GP\n", u8Vector, pVCpu->iem.s.uCpl, Idte.Gate.u2Dpl));
            return iemRaiseGeneralProtectionFault(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
        }
    }

    /* Is it there? */
    if (!Idte.Gate.u1Present)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - not present -> #NP\n", u8Vector));
        return iemRaiseSelectorNotPresentWithErr(pVCpu, X86_TRAP_ERR_IDT | ((uint16_t)u8Vector << X86_TRAP_ERR_SEL_SHIFT));
    }

    /* A null CS is bad. */
    RTSEL NewCS = Idte.Gate.u16Sel;
    if (!(NewCS & X86_SEL_MASK_OFF_RPL))
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x -> #GP\n", u8Vector, NewCS));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /* Fetch the descriptor for the new CS. */
    IEMSELDESC DescCS;
    rcStrict = iemMemFetchSelDesc(pVCpu, &DescCS, NewCS, X86_XCPT_GP);
    if (rcStrict != VINF_SUCCESS)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - rc=%Rrc\n", u8Vector, NewCS, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }

    /* Must be a 64-bit code segment. */
    if (!DescCS.Long.Gen.u1DescType)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - system selector (%#x) -> #GP\n", u8Vector, NewCS, DescCS.Legacy.Gen.u4Type));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }
    if (   !DescCS.Long.Gen.u1Long
        || DescCS.Long.Gen.u1DefBig
        || !(DescCS.Long.Gen.u4Type & X86_SEL_TYPE_CODE) )
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - not 64-bit code selector (%#x, L=%u, D=%u) -> #GP\n",
             u8Vector, NewCS, DescCS.Legacy.Gen.u4Type, DescCS.Long.Gen.u1Long, DescCS.Long.Gen.u1DefBig));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }

    /* Don't allow lowering the privilege level.  For non-conforming CS
       selectors, the CS.DPL sets the privilege level the trap/interrupt
       handler runs at.  For conforming CS selectors, the CPL remains
       unchanged, but the CS.DPL must be <= CPL. */
    /** @todo Testcase: Interrupt handler with CS.DPL=1, interrupt dispatched
     *        when CPU in Ring-0. Result \#GP?  */
    if (DescCS.Legacy.Gen.u2Dpl > pVCpu->iem.s.uCpl)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - DPL (%d) > CPL (%d) -> #GP\n",
             u8Vector, NewCS, DescCS.Legacy.Gen.u2Dpl, pVCpu->iem.s.uCpl));
        return iemRaiseGeneralProtectionFault(pVCpu, NewCS & X86_SEL_MASK_OFF_RPL);
    }


    /* Make sure the selector is present. */
    if (!DescCS.Legacy.Gen.u1Present)
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - CS=%#x - segment not present -> #NP\n", u8Vector, NewCS));
        return iemRaiseSelectorNotPresentBySelector(pVCpu, NewCS);
    }

    /* Check that the new RIP is canonical. */
    uint64_t const uNewRip = Idte.Gate.u16OffsetLow
                           | ((uint32_t)Idte.Gate.u16OffsetHigh << 16)
                           | ((uint64_t)Idte.Gate.u32OffsetTop  << 32);
    if (!IEM_IS_CANONICAL(uNewRip))
    {
        Log(("iemRaiseXcptOrIntInLongMode %#x - RIP=%#RX64 - Not canonical -> #GP(0)\n", u8Vector, uNewRip));
        return iemRaiseGeneralProtectionFault0(pVCpu);
    }

    /*
     * If the privilege level changes or if the IST isn't zero, we need to get
     * a new stack from the TSS.
     */
    uint64_t        uNewRsp;
    uint8_t const   uNewCpl = DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_CONF
                            ? pVCpu->iem.s.uCpl : DescCS.Legacy.Gen.u2Dpl;
    if (   uNewCpl != pVCpu->iem.s.uCpl
        || Idte.Gate.u3IST != 0)
    {
        rcStrict = iemRaiseLoadStackFromTss64(pVCpu, pCtx, uNewCpl, Idte.Gate.u3IST, &uNewRsp);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
    else
        uNewRsp = pCtx->rsp;
    uNewRsp &= ~(uint64_t)0xf;

    /*
     * Calc the flag image to push.
     */
    uint32_t        fEfl    = IEMMISC_GET_EFL(pVCpu, pCtx);
    if (fFlags & (IEM_XCPT_FLAGS_DRx_INSTR_BP | IEM_XCPT_FLAGS_T_SOFT_INT))
        fEfl &= ~X86_EFL_RF;
    else if (!IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
        fEfl |= X86_EFL_RF; /* Vagueness is all I've found on this so far... */ /** @todo Automatically pushing EFLAGS.RF. */

    /*
     * Start making changes.
     */
    /* Set the new CPL so that stack accesses use it. */
    uint8_t const uOldCpl = pVCpu->iem.s.uCpl;
    pVCpu->iem.s.uCpl = uNewCpl;

    /* Create the stack frame. */
    uint32_t   cbStackFrame = sizeof(uint64_t) * (5 + !!(fFlags & IEM_XCPT_FLAGS_ERR));
    RTPTRUNION uStackFrame;
    rcStrict = iemMemMap(pVCpu, &uStackFrame.pv, cbStackFrame, UINT8_MAX,
                         uNewRsp - cbStackFrame, IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS); /* _SYS is a hack ... */
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    void * const pvStackFrame = uStackFrame.pv;

    if (fFlags & IEM_XCPT_FLAGS_ERR)
        *uStackFrame.pu64++ = uErr;
    uStackFrame.pu64[0] = fFlags & IEM_XCPT_FLAGS_T_SOFT_INT ? pCtx->rip + cbInstr : pCtx->rip;
    uStackFrame.pu64[1] = (pCtx->cs.Sel & ~X86_SEL_RPL) | uOldCpl; /* CPL paranoia */
    uStackFrame.pu64[2] = fEfl;
    uStackFrame.pu64[3] = pCtx->rsp;
    uStackFrame.pu64[4] = pCtx->ss.Sel;
    rcStrict = iemMemCommitAndUnmap(pVCpu, pvStackFrame, IEM_ACCESS_STACK_W | IEM_ACCESS_WHAT_SYS);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /* Mark the CS selectors 'accessed' (hope this is the correct time). */
    /** @todo testcase: excatly _when_ are the accessed bits set - before or
     *        after pushing the stack frame? (Write protect the gdt + stack to
     *        find out.) */
    if (!(DescCS.Legacy.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
    {
        rcStrict = iemMemMarkSelDescAccessed(pVCpu, NewCS);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        DescCS.Legacy.Gen.u4Type |= X86_SEL_TYPE_ACCESSED;
    }

    /*
     * Start comitting the register changes.
     */
    /** @todo research/testcase: Figure out what VT-x and AMD-V loads into the
     *        hidden registers when interrupting 32-bit or 16-bit code! */
    if (uNewCpl != uOldCpl)
    {
        pCtx->ss.Sel        = 0 | uNewCpl;
        pCtx->ss.ValidSel   = 0 | uNewCpl;
        pCtx->ss.fFlags     = CPUMSELREG_FLAGS_VALID;
        pCtx->ss.u32Limit   = UINT32_MAX;
        pCtx->ss.u64Base    = 0;
        pCtx->ss.Attr.u     = (uNewCpl << X86DESCATTR_DPL_SHIFT) | X86DESCATTR_UNUSABLE;
    }
    pCtx->rsp           = uNewRsp - cbStackFrame;
    pCtx->cs.Sel        = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pCtx->cs.ValidSel   = (NewCS & ~X86_SEL_RPL) | uNewCpl;
    pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.u32Limit   = X86DESC_LIMIT_G(&DescCS.Legacy);
    pCtx->cs.u64Base    = X86DESC_BASE(&DescCS.Legacy);
    pCtx->cs.Attr.u     = X86DESC_GET_HID_ATTR(&DescCS.Legacy);
    pCtx->rip           = uNewRip;

    fEfl &= ~fEflToClear;
    IEMMISC_SET_EFL(pVCpu, pCtx, fEfl);

    if (fFlags & IEM_XCPT_FLAGS_CR2)
        pCtx->cr2 = uCr2;

    if (fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT)
        iemRaiseXcptAdjustState(pCtx, u8Vector);

    return fFlags & IEM_XCPT_FLAGS_T_CPU_XCPT ? VINF_IEM_RAISED_XCPT : VINF_SUCCESS;
}


/**
 * Implements exceptions and interrupts.
 *
 * All exceptions and interrupts goes thru this function!
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr         The number of bytes to offset rIP by in the return
 *                          address.
 * @param   u8Vector        The interrupt / exception vector number.
 * @param   fFlags          The flags.
 * @param   uErr            The error value if IEM_XCPT_FLAGS_ERR is set.
 * @param   uCr2            The CR2 value if IEM_XCPT_FLAGS_CR2 is set.
 */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC)
iemRaiseXcptOrInt(PVMCPU      pVCpu,
                  uint8_t     cbInstr,
                  uint8_t     u8Vector,
                  uint32_t    fFlags,
                  uint16_t    uErr,
                  uint64_t    uCr2)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
#ifdef IN_RING0
    int rc = HMR0EnsureCompleteBasicContext(pVCpu, pCtx);
    AssertRCReturn(rc, rc);
#endif

#ifndef IEM_WITH_CODE_TLB /** @todo we're doing it afterwards too, that should suffice... */
    /*
     * Flush prefetch buffer
     */
    pVCpu->iem.s.cbOpcode = pVCpu->iem.s.offOpcode;
#endif

    /*
     * Perform the V8086 IOPL check and upgrade the fault without nesting.
     */
    if (   pCtx->eflags.Bits.u1VM
        && pCtx->eflags.Bits.u2IOPL != 3
        && (fFlags & (IEM_XCPT_FLAGS_T_SOFT_INT | IEM_XCPT_FLAGS_BP_INSTR)) == IEM_XCPT_FLAGS_T_SOFT_INT
        && (pCtx->cr0 & X86_CR0_PE) )
    {
        Log(("iemRaiseXcptOrInt: V8086 IOPL check failed for int %#x -> #GP(0)\n", u8Vector));
        fFlags   = IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR;
        u8Vector = X86_XCPT_GP;
        uErr     = 0;
    }
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "Xcpt/%u: %02x %u %x %x %llx %04x:%04llx %04x:%04llx",
                      pVCpu->iem.s.cXcptRecursions, u8Vector, cbInstr, fFlags, uErr, uCr2,
                      pCtx->cs.Sel, pCtx->rip, pCtx->ss.Sel, pCtx->rsp);
#endif

#ifdef VBOX_WITH_NESTED_HWVIRT
    if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
    {
        /*
         * If the event is being injected as part of VMRUN, it isn't subject to event
         * intercepts in the nested-guest. However, secondary exceptions that occur
         * during injection of any event -are- subject to exception intercepts.
         * See AMD spec. 15.20 "Event Injection".
         */
        if (!pCtx->hwvirt.svm.fInterceptEvents)
            pCtx->hwvirt.svm.fInterceptEvents = 1;
        else
        {
            /*
             * Check and handle if the event being raised is intercepted.
             */
            VBOXSTRICTRC rcStrict0 = iemHandleSvmEventIntercept(pVCpu, pCtx, u8Vector, fFlags, uErr, uCr2);
            if (rcStrict0 != VINF_HM_INTERCEPT_NOT_ACTIVE)
                return rcStrict0;
        }
    }
#endif /* VBOX_WITH_NESTED_HWVIRT */

    /*
     * Do recursion accounting.
     */
    uint8_t const  uPrevXcpt = pVCpu->iem.s.uCurXcpt;
    uint32_t const fPrevXcpt = pVCpu->iem.s.fCurXcpt;
    if (pVCpu->iem.s.cXcptRecursions == 0)
        Log(("iemRaiseXcptOrInt: %#x at %04x:%RGv cbInstr=%#x fFlags=%#x uErr=%#x uCr2=%llx\n",
             u8Vector, pCtx->cs.Sel, pCtx->rip, cbInstr, fFlags, uErr, uCr2));
    else
    {
        Log(("iemRaiseXcptOrInt: %#x at %04x:%RGv cbInstr=%#x fFlags=%#x uErr=%#x uCr2=%llx; prev=%#x depth=%d flags=%#x\n",
             u8Vector, pCtx->cs.Sel, pCtx->rip, cbInstr, fFlags, uErr, uCr2, pVCpu->iem.s.uCurXcpt,
             pVCpu->iem.s.cXcptRecursions + 1, fPrevXcpt));

        if (pVCpu->iem.s.cXcptRecursions >= 3)
        {
#ifdef DEBUG_bird
            AssertFailed();
#endif
            IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(("Too many fault nestings.\n"));
        }

        /*
         * Evaluate the sequence of recurring events.
         */
        IEMXCPTRAISE enmRaise = IEMEvaluateRecursiveXcpt(pVCpu, fPrevXcpt, uPrevXcpt, fFlags, u8Vector,
                                                         NULL /* pXcptRaiseInfo */);
        if (enmRaise == IEMXCPTRAISE_CURRENT_XCPT)
        { /* likely */ }
        else if (enmRaise == IEMXCPTRAISE_DOUBLE_FAULT)
        {
            Log2(("iemRaiseXcptOrInt: Raising double fault. uPrevXcpt=%#x\n", uPrevXcpt));
            fFlags   = IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR;
            u8Vector = X86_XCPT_DF;
            uErr     = 0;
            /* SVM nested-guest #DF intercepts need to be checked now. See AMD spec. 15.12 "Exception Intercepts". */
            if (IEM_IS_SVM_XCPT_INTERCEPT_SET(pVCpu, X86_XCPT_DF))
                IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_EXCEPTION_0 + X86_XCPT_DF, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
        }
        else if (enmRaise == IEMXCPTRAISE_TRIPLE_FAULT)
        {
            Log2(("iemRaiseXcptOrInt: Raising triple fault. uPrevXcpt=%#x\n", uPrevXcpt));
            return iemInitiateCpuShutdown(pVCpu);
        }
        else if (enmRaise == IEMXCPTRAISE_CPU_HANG)
        {
            /* If a nested-guest enters an endless CPU loop condition, we'll emulate it; otherwise guru. */
            Log2(("iemRaiseXcptOrInt: CPU hang condition detected\n"));
            if (!CPUMIsGuestInNestedHwVirtMode(pCtx))
                return VERR_EM_GUEST_CPU_HANG;
        }
        else
        {
            AssertMsgFailed(("Unexpected condition! enmRaise=%#x uPrevXcpt=%#x fPrevXcpt=%#x, u8Vector=%#x fFlags=%#x\n",
                             enmRaise, uPrevXcpt, fPrevXcpt, u8Vector, fFlags));
            return VERR_IEM_IPE_9;
        }

        /*
         * The 'EXT' bit is set when an exception occurs during deliver of an external
         * event (such as an interrupt or earlier exception)[1]. Privileged software
         * exception (INT1) also sets the EXT bit[2]. Exceptions generated by software
         * interrupts and INTO, INT3 instructions, the 'EXT' bit will not be set.
         *
         * [1] - Intel spec. 6.13 "Error Code"
         * [2] - Intel spec. 26.5.1.1 "Details of Vectored-Event Injection".
         * [3] - Intel Instruction reference for INT n.
         */
        if (   (fPrevXcpt & (IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_T_EXT_INT | IEM_XCPT_FLAGS_ICEBP_INSTR))
            && (fFlags & IEM_XCPT_FLAGS_ERR)
            && u8Vector != X86_XCPT_PF
            && u8Vector != X86_XCPT_DF)
        {
            uErr |= X86_TRAP_ERR_EXTERNAL;
        }
    }

    pVCpu->iem.s.cXcptRecursions++;
    pVCpu->iem.s.uCurXcpt    = u8Vector;
    pVCpu->iem.s.fCurXcpt    = fFlags;
    pVCpu->iem.s.uCurXcptErr = uErr;
    pVCpu->iem.s.uCurXcptCr2 = uCr2;

    /*
     * Extensive logging.
     */
#if defined(LOG_ENABLED) && defined(IN_RING3)
    if (LogIs3Enabled())
    {
        PVM     pVM = pVCpu->CTX_SUFF(pVM);
        char    szRegs[4096];
        DBGFR3RegPrintf(pVM->pUVM, pVCpu->idCpu, &szRegs[0], sizeof(szRegs),
                        "rax=%016VR{rax} rbx=%016VR{rbx} rcx=%016VR{rcx} rdx=%016VR{rdx}\n"
                        "rsi=%016VR{rsi} rdi=%016VR{rdi} r8 =%016VR{r8} r9 =%016VR{r9}\n"
                        "r10=%016VR{r10} r11=%016VR{r11} r12=%016VR{r12} r13=%016VR{r13}\n"
                        "r14=%016VR{r14} r15=%016VR{r15} %VRF{rflags}\n"
                        "rip=%016VR{rip} rsp=%016VR{rsp} rbp=%016VR{rbp}\n"
                        "cs={%04VR{cs} base=%016VR{cs_base} limit=%08VR{cs_lim} flags=%04VR{cs_attr}} cr0=%016VR{cr0}\n"
                        "ds={%04VR{ds} base=%016VR{ds_base} limit=%08VR{ds_lim} flags=%04VR{ds_attr}} cr2=%016VR{cr2}\n"
                        "es={%04VR{es} base=%016VR{es_base} limit=%08VR{es_lim} flags=%04VR{es_attr}} cr3=%016VR{cr3}\n"
                        "fs={%04VR{fs} base=%016VR{fs_base} limit=%08VR{fs_lim} flags=%04VR{fs_attr}} cr4=%016VR{cr4}\n"
                        "gs={%04VR{gs} base=%016VR{gs_base} limit=%08VR{gs_lim} flags=%04VR{gs_attr}} cr8=%016VR{cr8}\n"
                        "ss={%04VR{ss} base=%016VR{ss_base} limit=%08VR{ss_lim} flags=%04VR{ss_attr}}\n"
                        "dr0=%016VR{dr0} dr1=%016VR{dr1} dr2=%016VR{dr2} dr3=%016VR{dr3}\n"
                        "dr6=%016VR{dr6} dr7=%016VR{dr7}\n"
                        "gdtr=%016VR{gdtr_base}:%04VR{gdtr_lim}  idtr=%016VR{idtr_base}:%04VR{idtr_lim}  rflags=%08VR{rflags}\n"
                        "ldtr={%04VR{ldtr} base=%016VR{ldtr_base} limit=%08VR{ldtr_lim} flags=%08VR{ldtr_attr}}\n"
                        "tr  ={%04VR{tr} base=%016VR{tr_base} limit=%08VR{tr_lim} flags=%08VR{tr_attr}}\n"
                        "    sysenter={cs=%04VR{sysenter_cs} eip=%08VR{sysenter_eip} esp=%08VR{sysenter_esp}}\n"
                        "        efer=%016VR{efer}\n"
                        "         pat=%016VR{pat}\n"
                        "     sf_mask=%016VR{sf_mask}\n"
                        "krnl_gs_base=%016VR{krnl_gs_base}\n"
                        "       lstar=%016VR{lstar}\n"
                        "        star=%016VR{star} cstar=%016VR{cstar}\n"
                        "fcw=%04VR{fcw} fsw=%04VR{fsw} ftw=%04VR{ftw} mxcsr=%04VR{mxcsr} mxcsr_mask=%04VR{mxcsr_mask}\n"
                        );

        char szInstr[256];
        DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                           DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                           szInstr, sizeof(szInstr), NULL);
        Log3(("%s%s\n", szRegs, szInstr));
    }
#endif /* LOG_ENABLED */

    /*
     * Call the mode specific worker function.
     */
    VBOXSTRICTRC    rcStrict;
    if (!(pCtx->cr0 & X86_CR0_PE))
        rcStrict = iemRaiseXcptOrIntInRealMode(pVCpu, pCtx, cbInstr, u8Vector, fFlags, uErr, uCr2);
    else if (pCtx->msrEFER & MSR_K6_EFER_LMA)
        rcStrict = iemRaiseXcptOrIntInLongMode(pVCpu, pCtx, cbInstr, u8Vector, fFlags, uErr, uCr2);
    else
        rcStrict = iemRaiseXcptOrIntInProtMode(pVCpu, pCtx, cbInstr, u8Vector, fFlags, uErr, uCr2);

    /* Flush the prefetch buffer. */
#ifdef IEM_WITH_CODE_TLB
    pVCpu->iem.s.pbInstrBuf = NULL;
#else
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    /*
     * Unwind.
     */
    pVCpu->iem.s.cXcptRecursions--;
    pVCpu->iem.s.uCurXcpt = uPrevXcpt;
    pVCpu->iem.s.fCurXcpt = fPrevXcpt;
    Log(("iemRaiseXcptOrInt: returns %Rrc (vec=%#x); cs:rip=%04x:%RGv ss:rsp=%04x:%RGv cpl=%u\n",
         VBOXSTRICTRC_VAL(rcStrict), u8Vector, pCtx->cs.Sel, pCtx->rip, pCtx->ss.Sel, pCtx->esp, pVCpu->iem.s.uCpl));
    return rcStrict;
}

#ifdef IEM_WITH_SETJMP
/**
 * See iemRaiseXcptOrInt.  Will not return.
 */
IEM_STATIC DECL_NO_RETURN(void)
iemRaiseXcptOrIntJmp(PVMCPU      pVCpu,
                     uint8_t     cbInstr,
                     uint8_t     u8Vector,
                     uint32_t    fFlags,
                     uint16_t    uErr,
                     uint64_t    uCr2)
{
    VBOXSTRICTRC rcStrict = iemRaiseXcptOrInt(pVCpu, cbInstr, u8Vector, fFlags, uErr, uCr2);
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
}
#endif


/** \#DE - 00.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseDivideError(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_DE, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#DB - 01.
 * @note This automatically clear DR7.GD.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseDebugException(PVMCPU pVCpu)
{
    /** @todo set/clear RF. */
    IEM_GET_CTX(pVCpu)->dr[7] &= ~X86_DR7_GD;
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_DB, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#BR - 05.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseBoundRangeExceeded(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_BR, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#UD - 06.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseUndefinedOpcode(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_UD, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#NM - 07.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseDeviceNotAvailable(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_NM, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#TS(err) - 0a.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseTaskSwitchFaultWithErr(PVMCPU pVCpu, uint16_t uErr)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#TS(tr) - 0a.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseTaskSwitchFaultCurrentTSS(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             IEM_GET_CTX(pVCpu)->tr.Sel, 0);
}


/** \#TS(0) - 0a.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseTaskSwitchFault0(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             0, 0);
}


/** \#TS(err) - 0a.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseTaskSwitchFaultBySelector(PVMCPU pVCpu, uint16_t uSel)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_TS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             uSel & X86_SEL_MASK_OFF_RPL, 0);
}


/** \#NP(err) - 0b.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseSelectorNotPresentWithErr(PVMCPU pVCpu, uint16_t uErr)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_NP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#NP(sel) - 0b.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseSelectorNotPresentBySelector(PVMCPU pVCpu, uint16_t uSel)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_NP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             uSel & ~X86_SEL_RPL, 0);
}


/** \#SS(seg) - 0c.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseStackSelectorNotPresentBySelector(PVMCPU pVCpu, uint16_t uSel)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_SS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             uSel & ~X86_SEL_RPL, 0);
}


/** \#SS(err) - 0c.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseStackSelectorNotPresentWithErr(PVMCPU pVCpu, uint16_t uErr)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_SS, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#GP(n) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseGeneralProtectionFault(PVMCPU pVCpu, uint16_t uErr)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErr, 0);
}


/** \#GP(0) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseGeneralProtectionFault0(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(0) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseGeneralProtectionFault0Jmp(PVMCPU pVCpu)
{
    iemRaiseXcptOrIntJmp(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif


/** \#GP(sel) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseGeneralProtectionFaultBySelector(PVMCPU pVCpu, RTSEL Sel)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                             Sel & ~X86_SEL_RPL, 0);
}


/** \#GP(0) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseNotCanonical(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}


/** \#GP(sel) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseSelectorBounds(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess)
{
    NOREF(iSegReg); NOREF(fAccess);
    return iemRaiseXcptOrInt(pVCpu, 0, iSegReg == X86_SREG_SS ? X86_XCPT_SS : X86_XCPT_GP,
                             IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(sel) - 0d, longjmp.  */
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseSelectorBoundsJmp(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess)
{
    NOREF(iSegReg); NOREF(fAccess);
    iemRaiseXcptOrIntJmp(pVCpu, 0, iSegReg == X86_SREG_SS ? X86_XCPT_SS : X86_XCPT_GP,
                         IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif

/** \#GP(sel) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseSelectorBoundsBySelector(PVMCPU pVCpu, RTSEL Sel)
{
    NOREF(Sel);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(sel) - 0d, longjmp.  */
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseSelectorBoundsBySelectorJmp(PVMCPU pVCpu, RTSEL Sel)
{
    NOREF(Sel);
    iemRaiseXcptOrIntJmp(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif


/** \#GP(sel) - 0d.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseSelectorInvalidAccess(PVMCPU pVCpu, uint32_t iSegReg, uint32_t fAccess)
{
    NOREF(iSegReg); NOREF(fAccess);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}

#ifdef IEM_WITH_SETJMP
/** \#GP(sel) - 0d, longjmp.  */
DECL_NO_INLINE(IEM_STATIC, DECL_NO_RETURN(void)) iemRaiseSelectorInvalidAccessJmp(PVMCPU pVCpu, uint32_t iSegReg,
                                                                                  uint32_t fAccess)
{
    NOREF(iSegReg); NOREF(fAccess);
    iemRaiseXcptOrIntJmp(pVCpu, 0, X86_XCPT_GP, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, 0, 0);
}
#endif


/** \#PF(n) - 0e.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaisePageFault(PVMCPU pVCpu, RTGCPTR GCPtrWhere, uint32_t fAccess, int rc)
{
    uint16_t uErr;
    switch (rc)
    {
        case VERR_PAGE_NOT_PRESENT:
        case VERR_PAGE_TABLE_NOT_PRESENT:
        case VERR_PAGE_DIRECTORY_PTR_NOT_PRESENT:
        case VERR_PAGE_MAP_LEVEL4_NOT_PRESENT:
            uErr = 0;
            break;

        default:
            AssertMsgFailed(("%Rrc\n", rc));
            RT_FALL_THRU();
        case VERR_ACCESS_DENIED:
            uErr = X86_TRAP_PF_P;
            break;

        /** @todo reserved  */
    }

    if (pVCpu->iem.s.uCpl == 3)
        uErr |= X86_TRAP_PF_US;

    if (   (fAccess & IEM_ACCESS_WHAT_MASK) == IEM_ACCESS_WHAT_CODE
        && (   (IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_PAE)
            && (IEM_GET_CTX(pVCpu)->msrEFER & MSR_K6_EFER_NXE) ) )
        uErr |= X86_TRAP_PF_ID;

#if 0 /* This is so much non-sense, really.  Why was it done like that? */
    /* Note! RW access callers reporting a WRITE protection fault, will clear
             the READ flag before calling.  So, read-modify-write accesses (RW)
             can safely be reported as READ faults. */
    if ((fAccess & (IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_TYPE_READ)) == IEM_ACCESS_TYPE_WRITE)
        uErr |= X86_TRAP_PF_RW;
#else
    if (fAccess & IEM_ACCESS_TYPE_WRITE)
    {
        if (!IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu) || !(fAccess & IEM_ACCESS_TYPE_READ))
            uErr |= X86_TRAP_PF_RW;
    }
#endif

    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_PF, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR | IEM_XCPT_FLAGS_CR2,
                             uErr, GCPtrWhere);
}

#ifdef IEM_WITH_SETJMP
/** \#PF(n) - 0e, longjmp.  */
IEM_STATIC DECL_NO_RETURN(void) iemRaisePageFaultJmp(PVMCPU pVCpu, RTGCPTR GCPtrWhere, uint32_t fAccess, int rc)
{
    longjmp(*CTX_SUFF(pVCpu->iem.s.pJmpBuf), VBOXSTRICTRC_VAL(iemRaisePageFault(pVCpu, GCPtrWhere, fAccess, rc)));
}
#endif


/** \#MF(0) - 10.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseMathFault(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_MF, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** \#AC(0) - 11.  */
DECL_NO_INLINE(IEM_STATIC, VBOXSTRICTRC) iemRaiseAlignmentCheckException(PVMCPU pVCpu)
{
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_AC, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/**
 * Macro for calling iemCImplRaiseDivideError().
 *
 * This enables us to add/remove arguments and force different levels of
 * inlining as we wish.
 *
 * @return  Strict VBox status code.
 */
#define IEMOP_RAISE_DIVIDE_ERROR()          IEM_MC_DEFER_TO_CIMPL_0(iemCImplRaiseDivideError)
IEM_CIMPL_DEF_0(iemCImplRaiseDivideError)
{
    NOREF(cbInstr);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_DE, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/**
 * Macro for calling iemCImplRaiseInvalidLockPrefix().
 *
 * This enables us to add/remove arguments and force different levels of
 * inlining as we wish.
 *
 * @return  Strict VBox status code.
 */
#define IEMOP_RAISE_INVALID_LOCK_PREFIX()   IEM_MC_DEFER_TO_CIMPL_0(iemCImplRaiseInvalidLockPrefix)
IEM_CIMPL_DEF_0(iemCImplRaiseInvalidLockPrefix)
{
    NOREF(cbInstr);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_UD, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/**
 * Macro for calling iemCImplRaiseInvalidOpcode().
 *
 * This enables us to add/remove arguments and force different levels of
 * inlining as we wish.
 *
 * @return  Strict VBox status code.
 */
#define IEMOP_RAISE_INVALID_OPCODE()        IEM_MC_DEFER_TO_CIMPL_0(iemCImplRaiseInvalidOpcode)
IEM_CIMPL_DEF_0(iemCImplRaiseInvalidOpcode)
{
    NOREF(cbInstr);
    return iemRaiseXcptOrInt(pVCpu, 0, X86_XCPT_UD, IEM_XCPT_FLAGS_T_CPU_XCPT, 0, 0);
}


/** @}  */


/*
 *
 * Helpers routines.
 * Helpers routines.
 * Helpers routines.
 *
 */

/**
 * Recalculates the effective operand size.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemRecalEffOpSize(PVMCPU pVCpu)
{
    switch (pVCpu->iem.s.enmCpuMode)
    {
        case IEMMODE_16BIT:
            pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP ? IEMMODE_32BIT : IEMMODE_16BIT;
            break;
        case IEMMODE_32BIT:
            pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SIZE_OP ? IEMMODE_16BIT : IEMMODE_32BIT;
            break;
        case IEMMODE_64BIT:
            switch (pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP))
            {
                case 0:
                    pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.enmDefOpSize;
                    break;
                case IEM_OP_PRF_SIZE_OP:
                    pVCpu->iem.s.enmEffOpSize = IEMMODE_16BIT;
                    break;
                case IEM_OP_PRF_SIZE_REX_W:
                case IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP:
                    pVCpu->iem.s.enmEffOpSize = IEMMODE_64BIT;
                    break;
            }
            break;
        default:
            AssertFailed();
    }
}


/**
 * Sets the default operand size to 64-bit and recalculates the effective
 * operand size.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemRecalEffOpSize64Default(PVMCPU pVCpu)
{
    Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);
    pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT;
    if ((pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_SIZE_REX_W | IEM_OP_PRF_SIZE_OP)) != IEM_OP_PRF_SIZE_OP)
        pVCpu->iem.s.enmEffOpSize = IEMMODE_64BIT;
    else
        pVCpu->iem.s.enmEffOpSize = IEMMODE_16BIT;
}


/*
 *
 * Common opcode decoders.
 * Common opcode decoders.
 * Common opcode decoders.
 *
 */
//#include <iprt/mem.h>

/**
 * Used to add extra details about a stub case.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemOpStubMsg2(PVMCPU pVCpu)
{
#if defined(LOG_ENABLED) && defined(IN_RING3)
    PVM  pVM = pVCpu->CTX_SUFF(pVM);
    char szRegs[4096];
    DBGFR3RegPrintf(pVM->pUVM, pVCpu->idCpu, &szRegs[0], sizeof(szRegs),
                    "rax=%016VR{rax} rbx=%016VR{rbx} rcx=%016VR{rcx} rdx=%016VR{rdx}\n"
                    "rsi=%016VR{rsi} rdi=%016VR{rdi} r8 =%016VR{r8} r9 =%016VR{r9}\n"
                    "r10=%016VR{r10} r11=%016VR{r11} r12=%016VR{r12} r13=%016VR{r13}\n"
                    "r14=%016VR{r14} r15=%016VR{r15} %VRF{rflags}\n"
                    "rip=%016VR{rip} rsp=%016VR{rsp} rbp=%016VR{rbp}\n"
                    "cs={%04VR{cs} base=%016VR{cs_base} limit=%08VR{cs_lim} flags=%04VR{cs_attr}} cr0=%016VR{cr0}\n"
                    "ds={%04VR{ds} base=%016VR{ds_base} limit=%08VR{ds_lim} flags=%04VR{ds_attr}} cr2=%016VR{cr2}\n"
                    "es={%04VR{es} base=%016VR{es_base} limit=%08VR{es_lim} flags=%04VR{es_attr}} cr3=%016VR{cr3}\n"
                    "fs={%04VR{fs} base=%016VR{fs_base} limit=%08VR{fs_lim} flags=%04VR{fs_attr}} cr4=%016VR{cr4}\n"
                    "gs={%04VR{gs} base=%016VR{gs_base} limit=%08VR{gs_lim} flags=%04VR{gs_attr}} cr8=%016VR{cr8}\n"
                    "ss={%04VR{ss} base=%016VR{ss_base} limit=%08VR{ss_lim} flags=%04VR{ss_attr}}\n"
                    "dr0=%016VR{dr0} dr1=%016VR{dr1} dr2=%016VR{dr2} dr3=%016VR{dr3}\n"
                    "dr6=%016VR{dr6} dr7=%016VR{dr7}\n"
                    "gdtr=%016VR{gdtr_base}:%04VR{gdtr_lim}  idtr=%016VR{idtr_base}:%04VR{idtr_lim}  rflags=%08VR{rflags}\n"
                    "ldtr={%04VR{ldtr} base=%016VR{ldtr_base} limit=%08VR{ldtr_lim} flags=%08VR{ldtr_attr}}\n"
                    "tr  ={%04VR{tr} base=%016VR{tr_base} limit=%08VR{tr_lim} flags=%08VR{tr_attr}}\n"
                    "    sysenter={cs=%04VR{sysenter_cs} eip=%08VR{sysenter_eip} esp=%08VR{sysenter_esp}}\n"
                    "        efer=%016VR{efer}\n"
                    "         pat=%016VR{pat}\n"
                    "     sf_mask=%016VR{sf_mask}\n"
                    "krnl_gs_base=%016VR{krnl_gs_base}\n"
                    "       lstar=%016VR{lstar}\n"
                    "        star=%016VR{star} cstar=%016VR{cstar}\n"
                    "fcw=%04VR{fcw} fsw=%04VR{fsw} ftw=%04VR{ftw} mxcsr=%04VR{mxcsr} mxcsr_mask=%04VR{mxcsr_mask}\n"
                    );

    char szInstr[256];
    DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                       DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                       szInstr, sizeof(szInstr), NULL);

    RTAssertMsg2Weak("%s%s\n", szRegs, szInstr);
#else
    RTAssertMsg2Weak("cs:rip=%04x:%RX64\n", IEM_GET_CTX(pVCpu)->cs, IEM_GET_CTX(pVCpu)->rip);
#endif
}

/**
 * Complains about a stub.
 *
 * Providing two versions of this macro, one for daily use and one for use when
 * working on IEM.
 */
#if 0
# define IEMOP_BITCH_ABOUT_STUB() \
    do { \
        RTAssertMsg1(NULL, __LINE__, __FILE__, __FUNCTION__); \
        iemOpStubMsg2(pVCpu); \
        RTAssertPanic(); \
    } while (0)
#else
# define IEMOP_BITCH_ABOUT_STUB() Log(("Stub: %s (line %d)\n", __FUNCTION__, __LINE__));
#endif

/** Stubs an opcode. */
#define FNIEMOP_STUB(a_Name) \
    FNIEMOP_DEF(a_Name) \
    { \
        RT_NOREF_PV(pVCpu); \
        IEMOP_BITCH_ABOUT_STUB(); \
        return VERR_IEM_INSTR_NOT_IMPLEMENTED; \
    } \
    typedef int ignore_semicolon

/** Stubs an opcode. */
#define FNIEMOP_STUB_1(a_Name, a_Type0, a_Name0) \
    FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    { \
        RT_NOREF_PV(pVCpu); \
        RT_NOREF_PV(a_Name0); \
        IEMOP_BITCH_ABOUT_STUB(); \
        return VERR_IEM_INSTR_NOT_IMPLEMENTED; \
    } \
    typedef int ignore_semicolon

/** Stubs an opcode which currently should raise \#UD. */
#define FNIEMOP_UD_STUB(a_Name) \
    FNIEMOP_DEF(a_Name) \
    { \
        Log(("Unsupported instruction %Rfn\n", __FUNCTION__)); \
        return IEMOP_RAISE_INVALID_OPCODE(); \
    } \
    typedef int ignore_semicolon

/** Stubs an opcode which currently should raise \#UD. */
#define FNIEMOP_UD_STUB_1(a_Name, a_Type0, a_Name0) \
    FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    { \
        RT_NOREF_PV(pVCpu); \
        RT_NOREF_PV(a_Name0); \
        Log(("Unsupported instruction %Rfn\n", __FUNCTION__)); \
        return IEMOP_RAISE_INVALID_OPCODE(); \
    } \
    typedef int ignore_semicolon



/** @name   Register Access.
 * @{
 */

/**
 * Gets a reference (pointer) to the specified hidden segment register.
 *
 * @returns Hidden register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
IEM_STATIC PCPUMSELREG iemSRegGetHid(PVMCPU pVCpu, uint8_t iSegReg)
{
    Assert(iSegReg < X86_SREG_COUNT);
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    PCPUMSELREG pSReg = &pCtx->aSRegs[iSegReg];

#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (RT_LIKELY(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg)))
    { /* likely */ }
    else
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, pSReg);
#else
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
#endif
    return pSReg;
}


/**
 * Ensures that the given hidden segment register is up to date.
 *
 * @returns Hidden register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pSReg               The segment register.
 */
IEM_STATIC PCPUMSELREG iemSRegUpdateHid(PVMCPU pVCpu, PCPUMSELREG pSReg)
{
#ifdef VBOX_WITH_RAW_MODE_NOT_R0
    if (!CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg))
        CPUMGuestLazyLoadHiddenSelectorReg(pVCpu, pSReg);
#else
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, pSReg));
    NOREF(pVCpu);
#endif
    return pSReg;
}


/**
 * Gets a reference (pointer) to the specified segment register (the selector
 * value).
 *
 * @returns Pointer to the selector variable.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(uint16_t *) iemSRegRef(PVMCPU pVCpu, uint8_t iSegReg)
{
    Assert(iSegReg < X86_SREG_COUNT);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    return &pCtx->aSRegs[iSegReg].Sel;
}


/**
 * Fetches the selector value of a segment register.
 *
 * @returns The selector value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(uint16_t) iemSRegFetchU16(PVMCPU pVCpu, uint8_t iSegReg)
{
    Assert(iSegReg < X86_SREG_COUNT);
    return IEM_GET_CTX(pVCpu)->aSRegs[iSegReg].Sel;
}


/**
 * Fetches the base address value of a segment register.
 *
 * @returns The selector value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment register.
 */
DECLINLINE(uint64_t) iemSRegBaseFetchU64(PVMCPU pVCpu, uint8_t iSegReg)
{
    Assert(iSegReg < X86_SREG_COUNT);
    return IEM_GET_CTX(pVCpu)->aSRegs[iSegReg].u64Base;
}


/**
 * Gets a reference (pointer) to the specified general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The general purpose register.
 */
DECLINLINE(void *) iemGRegRef(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 16);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    return &pCtx->aGRegs[iReg];
}


/**
 * Gets a reference (pointer) to the specified 8-bit general purpose register.
 *
 * Because of AH, CH, DH and BH we cannot use iemGRegRef directly here.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint8_t *) iemGRegRefU8(PVMCPU pVCpu, uint8_t iReg)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    if (iReg < 4 || (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REX))
    {
        Assert(iReg < 16);
        return &pCtx->aGRegs[iReg].u8;
    }
    /* high 8-bit register. */
    Assert(iReg < 8);
    return &pCtx->aGRegs[iReg & 3].bHi;
}


/**
 * Gets a reference (pointer) to the specified 16-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint16_t *) iemGRegRefU16(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 16);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    return &pCtx->aGRegs[iReg].u16;
}


/**
 * Gets a reference (pointer) to the specified 32-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint32_t *) iemGRegRefU32(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 16);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    return &pCtx->aGRegs[iReg].u32;
}


/**
 * Gets a reference (pointer) to the specified 64-bit general purpose register.
 *
 * @returns Register reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint64_t *) iemGRegRefU64(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 64);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    return &pCtx->aGRegs[iReg].u64;
}


/**
 * Gets a reference (pointer) to the specified segment register's base address.
 *
 * @returns Segment register base address reference.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The segment selector.
 */
DECLINLINE(uint64_t *) iemSRegBaseRefU64(PVMCPU pVCpu, uint8_t iSegReg)
{
    Assert(iSegReg < X86_SREG_COUNT);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    return &pCtx->aSRegs[iSegReg].u64Base;
}


/**
 * Fetches the value of a 8-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint8_t) iemGRegFetchU8(PVMCPU pVCpu, uint8_t iReg)
{
    return *iemGRegRefU8(pVCpu, iReg);
}


/**
 * Fetches the value of a 16-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint16_t) iemGRegFetchU16(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 16);
    return IEM_GET_CTX(pVCpu)->aGRegs[iReg].u16;
}


/**
 * Fetches the value of a 32-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint32_t) iemGRegFetchU32(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 16);
    return IEM_GET_CTX(pVCpu)->aGRegs[iReg].u32;
}


/**
 * Fetches the value of a 64-bit general purpose register.
 *
 * @returns The register value.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iReg                The register.
 */
DECLINLINE(uint64_t) iemGRegFetchU64(PVMCPU pVCpu, uint8_t iReg)
{
    Assert(iReg < 16);
    return IEM_GET_CTX(pVCpu)->aGRegs[iReg].u64;
}


/**
 * Adds a 8-bit signed jump offset to RIP/EIP/IP.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   offNextInstr        The offset of the next instruction.
 */
IEM_STATIC VBOXSTRICTRC iemRegRipRelativeJumpS8(PVMCPU pVCpu, int8_t offNextInstr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            uint16_t uNewIp = pCtx->ip + offNextInstr + IEM_GET_INSTR_LEN(pVCpu);
            if (   uNewIp > pCtx->cs.u32Limit
                && pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT) /* no need to check for non-canonical. */
                return iemRaiseGeneralProtectionFault0(pVCpu);
            pCtx->rip = uNewIp;
            break;
        }

        case IEMMODE_32BIT:
        {
            Assert(pCtx->rip <= UINT32_MAX);
            Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

            uint32_t uNewEip = pCtx->eip + offNextInstr + IEM_GET_INSTR_LEN(pVCpu);
            if (uNewEip > pCtx->cs.u32Limit)
                return iemRaiseGeneralProtectionFault0(pVCpu);
            pCtx->rip = uNewEip;
            break;
        }

        case IEMMODE_64BIT:
        {
            Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);

            uint64_t uNewRip = pCtx->rip + offNextInstr + IEM_GET_INSTR_LEN(pVCpu);
            if (!IEM_IS_CANONICAL(uNewRip))
                return iemRaiseGeneralProtectionFault0(pVCpu);
            pCtx->rip = uNewRip;
            break;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    return VINF_SUCCESS;
}


/**
 * Adds a 16-bit signed jump offset to RIP/EIP/IP.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   offNextInstr        The offset of the next instruction.
 */
IEM_STATIC VBOXSTRICTRC iemRegRipRelativeJumpS16(PVMCPU pVCpu, int16_t offNextInstr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    Assert(pVCpu->iem.s.enmEffOpSize == IEMMODE_16BIT);

    uint16_t uNewIp = pCtx->ip + offNextInstr + IEM_GET_INSTR_LEN(pVCpu);
    if (   uNewIp > pCtx->cs.u32Limit
        && pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT) /* no need to check for non-canonical. */
        return iemRaiseGeneralProtectionFault0(pVCpu);
    /** @todo Test 16-bit jump in 64-bit mode. possible?  */
    pCtx->rip = uNewIp;
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    return VINF_SUCCESS;
}


/**
 * Adds a 32-bit signed jump offset to RIP/EIP/IP.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   offNextInstr        The offset of the next instruction.
 */
IEM_STATIC VBOXSTRICTRC iemRegRipRelativeJumpS32(PVMCPU pVCpu, int32_t offNextInstr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    Assert(pVCpu->iem.s.enmEffOpSize != IEMMODE_16BIT);

    if (pVCpu->iem.s.enmEffOpSize == IEMMODE_32BIT)
    {
        Assert(pCtx->rip <= UINT32_MAX); Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

        uint32_t uNewEip = pCtx->eip + offNextInstr + IEM_GET_INSTR_LEN(pVCpu);
        if (uNewEip > pCtx->cs.u32Limit)
            return iemRaiseGeneralProtectionFault0(pVCpu);
        pCtx->rip = uNewEip;
    }
    else
    {
        Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);

        uint64_t uNewRip = pCtx->rip + offNextInstr + IEM_GET_INSTR_LEN(pVCpu);
        if (!IEM_IS_CANONICAL(uNewRip))
            return iemRaiseGeneralProtectionFault0(pVCpu);
        pCtx->rip = uNewRip;
    }
    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    return VINF_SUCCESS;
}


/**
 * Performs a near jump to the specified address.
 *
 * May raise a \#GP(0) if the new RIP is non-canonical or outside the code
 * segment limit.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uNewRip             The new RIP value.
 */
IEM_STATIC VBOXSTRICTRC iemRegRipJump(PVMCPU pVCpu, uint64_t uNewRip)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    switch (pVCpu->iem.s.enmEffOpSize)
    {
        case IEMMODE_16BIT:
        {
            Assert(uNewRip <= UINT16_MAX);
            if (   uNewRip > pCtx->cs.u32Limit
                && pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT) /* no need to check for non-canonical. */
                return iemRaiseGeneralProtectionFault0(pVCpu);
            /** @todo Test 16-bit jump in 64-bit mode.  */
            pCtx->rip = uNewRip;
            break;
        }

        case IEMMODE_32BIT:
        {
            Assert(uNewRip <= UINT32_MAX);
            Assert(pCtx->rip <= UINT32_MAX);
            Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

            if (uNewRip > pCtx->cs.u32Limit)
                return iemRaiseGeneralProtectionFault0(pVCpu);
            pCtx->rip = uNewRip;
            break;
        }

        case IEMMODE_64BIT:
        {
            Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);

            if (!IEM_IS_CANONICAL(uNewRip))
                return iemRaiseGeneralProtectionFault0(pVCpu);
            pCtx->rip = uNewRip;
            break;
        }

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    pCtx->eflags.Bits.u1RF = 0;

#ifndef IEM_WITH_CODE_TLB
    /* Flush the prefetch buffer. */
    pVCpu->iem.s.cbOpcode = IEM_GET_INSTR_LEN(pVCpu);
#endif

    return VINF_SUCCESS;
}


/**
 * Get the address of the top of the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The CPU context which SP/ESP/RSP should be
 *                              read.
 */
DECLINLINE(RTGCPTR) iemRegGetEffRsp(PCVMCPU pVCpu, PCCPUMCTX pCtx)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        return pCtx->rsp;
    if (pCtx->ss.Attr.n.u1DefBig)
        return pCtx->esp;
    return pCtx->sp;
}


/**
 * Updates the RIP/EIP/IP to point to the next instruction.
 *
 * This function leaves the EFLAGS.RF flag alone.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             The number of bytes to add.
 */
IEM_STATIC void iemRegAddToRipKeepRF(PVMCPU pVCpu, uint8_t cbInstr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    switch (pVCpu->iem.s.enmCpuMode)
    {
        case IEMMODE_16BIT:
            Assert(pCtx->rip <= UINT16_MAX);
            pCtx->eip += cbInstr;
            pCtx->eip &= UINT32_C(0xffff);
            break;

        case IEMMODE_32BIT:
            pCtx->eip += cbInstr;
            Assert(pCtx->rip <= UINT32_MAX);
            break;

        case IEMMODE_64BIT:
            pCtx->rip += cbInstr;
            break;
        default: AssertFailed();
    }
}


#if 0
/**
 * Updates the RIP/EIP/IP to point to the next instruction.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemRegUpdateRipKeepRF(PVMCPU pVCpu)
{
    return iemRegAddToRipKeepRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu));
}
#endif



/**
 * Updates the RIP/EIP/IP to point to the next instruction and clears EFLAGS.RF.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbInstr             The number of bytes to add.
 */
IEM_STATIC void iemRegAddToRipAndClearRF(PVMCPU pVCpu, uint8_t cbInstr)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    pCtx->eflags.Bits.u1RF = 0;

    AssertCompile(IEMMODE_16BIT == 0 && IEMMODE_32BIT == 1 && IEMMODE_64BIT == 2);
#if ARCH_BITS >= 64
    static uint64_t const s_aRipMasks[] = { UINT64_C(0xffffffff), UINT64_C(0xffffffff), UINT64_MAX };
    Assert(pCtx->rip <= s_aRipMasks[(unsigned)pVCpu->iem.s.enmCpuMode]);
    pCtx->rip = (pCtx->rip + cbInstr) & s_aRipMasks[(unsigned)pVCpu->iem.s.enmCpuMode];
#else
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pCtx->rip += cbInstr;
    else
        pCtx->eip += cbInstr;
#endif
}


/**
 * Updates the RIP/EIP/IP to point to the next instruction and clears EFLAGS.RF.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemRegUpdateRipAndClearRF(PVMCPU pVCpu)
{
    return iemRegAddToRipAndClearRF(pVCpu, IEM_GET_INSTR_LEN(pVCpu));
}


/**
 * Adds to the stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The CPU context which SP/ESP/RSP should be
 *                              updated.
 * @param   cbToAdd             The number of bytes to add (8-bit!).
 */
DECLINLINE(void) iemRegAddToRsp(PCVMCPU pVCpu, PCPUMCTX pCtx, uint8_t cbToAdd)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pCtx->rsp += cbToAdd;
    else if (pCtx->ss.Attr.n.u1DefBig)
        pCtx->esp += cbToAdd;
    else
        pCtx->sp  += cbToAdd;
}


/**
 * Subtracts from the stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The CPU context which SP/ESP/RSP should be
 *                              updated.
 * @param   cbToSub             The number of bytes to subtract (8-bit!).
 */
DECLINLINE(void) iemRegSubFromRsp(PCVMCPU pVCpu, PCPUMCTX pCtx, uint8_t cbToSub)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pCtx->rsp -= cbToSub;
    else if (pCtx->ss.Attr.n.u1DefBig)
        pCtx->esp -= cbToSub;
    else
        pCtx->sp  -= cbToSub;
}


/**
 * Adds to the temporary stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pTmpRsp             The temporary SP/ESP/RSP to update.
 * @param   cbToAdd             The number of bytes to add (16-bit).
 * @param   pCtx                Where to get the current stack mode.
 */
DECLINLINE(void) iemRegAddToRspEx(PCVMCPU pVCpu, PCCPUMCTX pCtx, PRTUINT64U pTmpRsp, uint16_t cbToAdd)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pTmpRsp->u           += cbToAdd;
    else if (pCtx->ss.Attr.n.u1DefBig)
        pTmpRsp->DWords.dw0  += cbToAdd;
    else
        pTmpRsp->Words.w0    += cbToAdd;
}


/**
 * Subtracts from the temporary stack pointer.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pTmpRsp             The temporary SP/ESP/RSP to update.
 * @param   cbToSub             The number of bytes to subtract.
 * @param   pCtx                Where to get the current stack mode.
 * @remarks The @a cbToSub argument *MUST* be 16-bit, iemCImpl_enter is
 *          expecting that.
 */
DECLINLINE(void) iemRegSubFromRspEx(PCVMCPU pVCpu, PCCPUMCTX pCtx, PRTUINT64U pTmpRsp, uint16_t cbToSub)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        pTmpRsp->u          -= cbToSub;
    else if (pCtx->ss.Attr.n.u1DefBig)
        pTmpRsp->DWords.dw0 -= cbToSub;
    else
        pTmpRsp->Words.w0   -= cbToSub;
}


/**
 * Calculates the effective stack address for a push of the specified size as
 * well as the new RSP value (upper bits may be masked).
 *
 * @returns Effective stack addressf for the push.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                Where to get the current stack mode.
 * @param   cbItem              The size of the stack item to pop.
 * @param   puNewRsp            Where to return the new RSP value.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPush(PCVMCPU pVCpu, PCCPUMCTX pCtx, uint8_t cbItem, uint64_t *puNewRsp)
{
    RTUINT64U   uTmpRsp;
    RTGCPTR     GCPtrTop;
    uTmpRsp.u = pCtx->rsp;

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        GCPtrTop = uTmpRsp.u            -= cbItem;
    else if (pCtx->ss.Attr.n.u1DefBig)
        GCPtrTop = uTmpRsp.DWords.dw0   -= cbItem;
    else
        GCPtrTop = uTmpRsp.Words.w0     -= cbItem;
    *puNewRsp = uTmpRsp.u;
    return GCPtrTop;
}


/**
 * Gets the current stack pointer and calculates the value after a pop of the
 * specified size.
 *
 * @returns Current stack pointer.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                Where to get the current stack mode.
 * @param   cbItem              The size of the stack item to pop.
 * @param   puNewRsp            Where to return the new RSP value.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPop(PCVMCPU pVCpu, PCCPUMCTX pCtx, uint8_t cbItem, uint64_t *puNewRsp)
{
    RTUINT64U   uTmpRsp;
    RTGCPTR     GCPtrTop;
    uTmpRsp.u = pCtx->rsp;

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        GCPtrTop = uTmpRsp.u;
        uTmpRsp.u += cbItem;
    }
    else if (pCtx->ss.Attr.n.u1DefBig)
    {
        GCPtrTop = uTmpRsp.DWords.dw0;
        uTmpRsp.DWords.dw0 += cbItem;
    }
    else
    {
        GCPtrTop = uTmpRsp.Words.w0;
        uTmpRsp.Words.w0 += cbItem;
    }
    *puNewRsp = uTmpRsp.u;
    return GCPtrTop;
}


/**
 * Calculates the effective stack address for a push of the specified size as
 * well as the new temporary RSP value (upper bits may be masked).
 *
 * @returns Effective stack addressf for the push.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                Where to get the current stack mode.
 * @param   pTmpRsp             The temporary stack pointer.  This is updated.
 * @param   cbItem              The size of the stack item to pop.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPushEx(PCVMCPU pVCpu, PCCPUMCTX pCtx, PRTUINT64U pTmpRsp, uint8_t cbItem)
{
    RTGCPTR GCPtrTop;

    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        GCPtrTop = pTmpRsp->u          -= cbItem;
    else if (pCtx->ss.Attr.n.u1DefBig)
        GCPtrTop = pTmpRsp->DWords.dw0 -= cbItem;
    else
        GCPtrTop = pTmpRsp->Words.w0   -= cbItem;
    return GCPtrTop;
}


/**
 * Gets the effective stack address for a pop of the specified size and
 * calculates and updates the temporary RSP.
 *
 * @returns Current stack pointer.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                Where to get the current stack mode.
 * @param   pTmpRsp             The temporary stack pointer.  This is updated.
 * @param   cbItem              The size of the stack item to pop.
 */
DECLINLINE(RTGCPTR) iemRegGetRspForPopEx(PCVMCPU pVCpu, PCCPUMCTX pCtx, PRTUINT64U pTmpRsp, uint8_t cbItem)
{
    RTGCPTR GCPtrTop;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        GCPtrTop = pTmpRsp->u;
        pTmpRsp->u          += cbItem;
    }
    else if (pCtx->ss.Attr.n.u1DefBig)
    {
        GCPtrTop = pTmpRsp->DWords.dw0;
        pTmpRsp->DWords.dw0 += cbItem;
    }
    else
    {
        GCPtrTop = pTmpRsp->Words.w0;
        pTmpRsp->Words.w0   += cbItem;
    }
    return GCPtrTop;
}

/** @}  */


/** @name   FPU access and helpers.
 *
 * @{
 */


/**
 * Hook for preparing to use the host FPU.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuPrepareUsage(PVMCPU pVCpu)
{
#ifdef IN_RING3
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStatePrepareHostCpuForUse(pVCpu);
#endif
}


/**
 * Hook for preparing to use the host FPU for SSE.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuPrepareUsageSse(PVMCPU pVCpu)
{
    iemFpuPrepareUsage(pVCpu);
}


/**
 * Hook for preparing to use the host FPU for AVX.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuPrepareUsageAvx(PVMCPU pVCpu)
{
    iemFpuPrepareUsage(pVCpu);
}


/**
 * Hook for actualizing the guest FPU state before the interpreter reads it.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeStateForRead(PVMCPU pVCpu)
{
#ifdef IN_RING3
    NOREF(pVCpu);
#else
    CPUMRZFpuStateActualizeForRead(pVCpu);
#endif
}


/**
 * Hook for actualizing the guest FPU state before the interpreter changes it.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeStateForChange(PVMCPU pVCpu)
{
#ifdef IN_RING3
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStateActualizeForChange(pVCpu);
#endif
}


/**
 * Hook for actualizing the guest XMM0..15 and MXCSR register state for read
 * only.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeSseStateForRead(PVMCPU pVCpu)
{
#if defined(IN_RING3) || defined(VBOX_WITH_KERNEL_USING_XMM)
    NOREF(pVCpu);
#else
    CPUMRZFpuStateActualizeSseForRead(pVCpu);
#endif
}


/**
 * Hook for actualizing the guest XMM0..15 and MXCSR register state for
 * read+write.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeSseStateForChange(PVMCPU pVCpu)
{
#if defined(IN_RING3) || defined(VBOX_WITH_KERNEL_USING_XMM)
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStateActualizeForChange(pVCpu);
#endif
}


/**
 * Hook for actualizing the guest YMM0..15 and MXCSR register state for read
 * only.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeAvxStateForRead(PVMCPU pVCpu)
{
#ifdef IN_RING3
    NOREF(pVCpu);
#else
    CPUMRZFpuStateActualizeAvxForRead(pVCpu);
#endif
}


/**
 * Hook for actualizing the guest YMM0..15 and MXCSR register state for
 * read+write.
 *
 * This is necessary in ring-0 and raw-mode context (nop in ring-3).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECLINLINE(void) iemFpuActualizeAvxStateForChange(PVMCPU pVCpu)
{
#ifdef IN_RING3
    CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_FPU_REM);
#else
    CPUMRZFpuStateActualizeForChange(pVCpu);
#endif
}


/**
 * Stores a QNaN value into a FPU register.
 *
 * @param   pReg                Pointer to the register.
 */
DECLINLINE(void) iemFpuStoreQNan(PRTFLOAT80U pReg)
{
    pReg->au32[0] = UINT32_C(0x00000000);
    pReg->au32[1] = UINT32_C(0xc0000000);
    pReg->au16[4] = UINT16_C(0xffff);
}


/**
 * Updates the FOP, FPU.CS and FPUIP registers.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The CPU context.
 * @param   pFpuCtx             The FPU context.
 */
DECLINLINE(void) iemFpuUpdateOpcodeAndIpWorker(PVMCPU pVCpu, PCPUMCTX pCtx, PX86FXSTATE pFpuCtx)
{
    Assert(pVCpu->iem.s.uFpuOpcode != UINT16_MAX);
    pFpuCtx->FOP = pVCpu->iem.s.uFpuOpcode;
    /** @todo x87.CS and FPUIP needs to be kept seperately. */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        /** @todo Testcase: making assumptions about how FPUIP and FPUDP are handled
         *        happens in real mode here based on the fnsave and fnstenv images. */
        pFpuCtx->CS    = 0;
        pFpuCtx->FPUIP = pCtx->eip | ((uint32_t)pCtx->cs.Sel << 4);
    }
    else
    {
        pFpuCtx->CS    = pCtx->cs.Sel;
        pFpuCtx->FPUIP = pCtx->rip;
    }
}


/**
 * Updates the x87.DS and FPUDP registers.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pCtx                The CPU context.
 * @param   pFpuCtx             The FPU context.
 * @param   iEffSeg             The effective segment register.
 * @param   GCPtrEff            The effective address relative to @a iEffSeg.
 */
DECLINLINE(void) iemFpuUpdateDP(PVMCPU pVCpu, PCPUMCTX pCtx, PX86FXSTATE pFpuCtx, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    RTSEL sel;
    switch (iEffSeg)
    {
        case X86_SREG_DS: sel = pCtx->ds.Sel; break;
        case X86_SREG_SS: sel = pCtx->ss.Sel; break;
        case X86_SREG_CS: sel = pCtx->cs.Sel; break;
        case X86_SREG_ES: sel = pCtx->es.Sel; break;
        case X86_SREG_FS: sel = pCtx->fs.Sel; break;
        case X86_SREG_GS: sel = pCtx->gs.Sel; break;
        default:
            AssertMsgFailed(("%d\n", iEffSeg));
            sel = pCtx->ds.Sel;
    }
    /** @todo pFpuCtx->DS and FPUDP needs to be kept seperately. */
    if (IEM_IS_REAL_OR_V86_MODE(pVCpu))
    {
        pFpuCtx->DS    = 0;
        pFpuCtx->FPUDP = (uint32_t)GCPtrEff + ((uint32_t)sel << 4);
    }
    else
    {
        pFpuCtx->DS    = sel;
        pFpuCtx->FPUDP = GCPtrEff;
    }
}


/**
 * Rotates the stack registers in the push direction.
 *
 * @param   pFpuCtx             The FPU context.
 * @remarks This is a complete waste of time, but fxsave stores the registers in
 *          stack order.
 */
DECLINLINE(void) iemFpuRotateStackPush(PX86FXSTATE pFpuCtx)
{
    RTFLOAT80U r80Tmp = pFpuCtx->aRegs[7].r80;
    pFpuCtx->aRegs[7].r80 = pFpuCtx->aRegs[6].r80;
    pFpuCtx->aRegs[6].r80 = pFpuCtx->aRegs[5].r80;
    pFpuCtx->aRegs[5].r80 = pFpuCtx->aRegs[4].r80;
    pFpuCtx->aRegs[4].r80 = pFpuCtx->aRegs[3].r80;
    pFpuCtx->aRegs[3].r80 = pFpuCtx->aRegs[2].r80;
    pFpuCtx->aRegs[2].r80 = pFpuCtx->aRegs[1].r80;
    pFpuCtx->aRegs[1].r80 = pFpuCtx->aRegs[0].r80;
    pFpuCtx->aRegs[0].r80 = r80Tmp;
}


/**
 * Rotates the stack registers in the pop direction.
 *
 * @param   pFpuCtx             The FPU context.
 * @remarks This is a complete waste of time, but fxsave stores the registers in
 *          stack order.
 */
DECLINLINE(void) iemFpuRotateStackPop(PX86FXSTATE pFpuCtx)
{
    RTFLOAT80U r80Tmp = pFpuCtx->aRegs[0].r80;
    pFpuCtx->aRegs[0].r80 = pFpuCtx->aRegs[1].r80;
    pFpuCtx->aRegs[1].r80 = pFpuCtx->aRegs[2].r80;
    pFpuCtx->aRegs[2].r80 = pFpuCtx->aRegs[3].r80;
    pFpuCtx->aRegs[3].r80 = pFpuCtx->aRegs[4].r80;
    pFpuCtx->aRegs[4].r80 = pFpuCtx->aRegs[5].r80;
    pFpuCtx->aRegs[5].r80 = pFpuCtx->aRegs[6].r80;
    pFpuCtx->aRegs[6].r80 = pFpuCtx->aRegs[7].r80;
    pFpuCtx->aRegs[7].r80 = r80Tmp;
}


/**
 * Updates FSW and pushes a FPU result onto the FPU stack if no pending
 * exception prevents it.
 *
 * @param   pResult             The FPU operation result to push.
 * @param   pFpuCtx             The FPU context.
 */
IEM_STATIC void iemFpuMaybePushResult(PIEMFPURESULT pResult, PX86FXSTATE pFpuCtx)
{
    /* Update FSW and bail if there are pending exceptions afterwards. */
    uint16_t fFsw = pFpuCtx->FSW & ~X86_FSW_C_MASK;
    fFsw |= pResult->FSW & ~X86_FSW_TOP_MASK;
    if (   (fFsw             & (X86_FSW_IE | X86_FSW_ZE | X86_FSW_DE))
        & ~(pFpuCtx->FCW & (X86_FCW_IM | X86_FCW_ZM | X86_FCW_DM)))
    {
        pFpuCtx->FSW = fFsw;
        return;
    }

    uint16_t iNewTop = (X86_FSW_TOP_GET(fFsw) + 7) & X86_FSW_TOP_SMASK;
    if (!(pFpuCtx->FTW & RT_BIT(iNewTop)))
    {
        /* All is fine, push the actual value. */
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        pFpuCtx->aRegs[7].r80 = pResult->r80Result;
    }
    else if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked stack overflow, push QNaN. */
        fFsw |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1;
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
    }
    else
    {
        /* Raise stack overflow, don't push anything. */
        pFpuCtx->FSW |= pResult->FSW & ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1 | X86_FSW_B | X86_FSW_ES;
        return;
    }

    fFsw &= ~X86_FSW_TOP_MASK;
    fFsw |= iNewTop << X86_FSW_TOP_SHIFT;
    pFpuCtx->FSW = fFsw;

    iemFpuRotateStackPush(pFpuCtx);
}


/**
 * Stores a result in a FPU register and updates the FSW and FTW.
 *
 * @param   pFpuCtx             The FPU context.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 */
IEM_STATIC void iemFpuStoreResultOnly(PX86FXSTATE pFpuCtx, PIEMFPURESULT pResult, uint8_t iStReg)
{
    Assert(iStReg < 8);
    uint16_t iReg = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    pFpuCtx->FSW &= ~X86_FSW_C_MASK;
    pFpuCtx->FSW |= pResult->FSW & ~X86_FSW_TOP_MASK;
    pFpuCtx->FTW |= RT_BIT(iReg);
    pFpuCtx->aRegs[iStReg].r80 = pResult->r80Result;
}


/**
 * Only updates the FPU status word (FSW) with the result of the current
 * instruction.
 *
 * @param   pFpuCtx             The FPU context.
 * @param   u16FSW              The FSW output of the current instruction.
 */
IEM_STATIC void iemFpuUpdateFSWOnly(PX86FXSTATE pFpuCtx, uint16_t u16FSW)
{
    pFpuCtx->FSW &= ~X86_FSW_C_MASK;
    pFpuCtx->FSW |= u16FSW & ~X86_FSW_TOP_MASK;
}


/**
 * Pops one item off the FPU stack if no pending exception prevents it.
 *
 * @param   pFpuCtx             The FPU context.
 */
IEM_STATIC void iemFpuMaybePopOne(PX86FXSTATE pFpuCtx)
{
    /* Check pending exceptions. */
    uint16_t uFSW = pFpuCtx->FSW;
    if (   (pFpuCtx->FSW & (X86_FSW_IE | X86_FSW_ZE | X86_FSW_DE))
        & ~(pFpuCtx->FCW & (X86_FCW_IM | X86_FCW_ZM | X86_FCW_DM)))
        return;

    /* TOP--. */
    uint16_t iOldTop = uFSW & X86_FSW_TOP_MASK;
    uFSW &= ~X86_FSW_TOP_MASK;
    uFSW |= (iOldTop + (UINT16_C(9) << X86_FSW_TOP_SHIFT)) & X86_FSW_TOP_MASK;
    pFpuCtx->FSW = uFSW;

    /* Mark the previous ST0 as empty. */
    iOldTop >>= X86_FSW_TOP_SHIFT;
    pFpuCtx->FTW &= ~RT_BIT(iOldTop);

    /* Rotate the registers. */
    iemFpuRotateStackPop(pFpuCtx);
}


/**
 * Pushes a FPU result onto the FPU stack if no pending exception prevents it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to push.
 */
IEM_STATIC void iemFpuPushResult(PVMCPU pVCpu, PIEMFPURESULT pResult)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuMaybePushResult(pResult, pFpuCtx);
}


/**
 * Pushes a FPU result onto the FPU stack if no pending exception prevents it,
 * and sets FPUDP and FPUDS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to push.
 * @param   iEffSeg             The effective segment register.
 * @param   GCPtrEff            The effective address relative to @a iEffSeg.
 */
IEM_STATIC void iemFpuPushResultWithMemOp(PVMCPU pVCpu, PIEMFPURESULT pResult, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuMaybePushResult(pResult, pFpuCtx);
}


/**
 * Replace ST0 with the first value and push the second onto the FPU stack,
 * unless a pending exception prevents it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The FPU operation result to store and push.
 */
IEM_STATIC void iemFpuPushResultTwo(PVMCPU pVCpu, PIEMFPURESULTTWO pResult)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);

    /* Update FSW and bail if there are pending exceptions afterwards. */
    uint16_t fFsw = pFpuCtx->FSW & ~X86_FSW_C_MASK;
    fFsw |= pResult->FSW & ~X86_FSW_TOP_MASK;
    if (   (fFsw             & (X86_FSW_IE | X86_FSW_ZE | X86_FSW_DE))
        & ~(pFpuCtx->FCW & (X86_FCW_IM | X86_FCW_ZM | X86_FCW_DM)))
    {
        pFpuCtx->FSW = fFsw;
        return;
    }

    uint16_t iNewTop = (X86_FSW_TOP_GET(fFsw) + 7) & X86_FSW_TOP_SMASK;
    if (!(pFpuCtx->FTW & RT_BIT(iNewTop)))
    {
        /* All is fine, push the actual value. */
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        pFpuCtx->aRegs[0].r80 = pResult->r80Result1;
        pFpuCtx->aRegs[7].r80 = pResult->r80Result2;
    }
    else if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked stack overflow, push QNaN. */
        fFsw |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1;
        iemFpuStoreQNan(&pFpuCtx->aRegs[0].r80);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
    }
    else
    {
        /* Raise stack overflow, don't push anything. */
        pFpuCtx->FSW |= pResult->FSW & ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_C1 | X86_FSW_B | X86_FSW_ES;
        return;
    }

    fFsw &= ~X86_FSW_TOP_MASK;
    fFsw |= iNewTop << X86_FSW_TOP_SHIFT;
    pFpuCtx->FSW = fFsw;

    iemFpuRotateStackPush(pFpuCtx);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, and
 * FOP.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 */
IEM_STATIC void iemFpuStoreResult(PVMCPU pVCpu, PIEMFPURESULT pResult, uint8_t iStReg)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStoreResultOnly(pFpuCtx, pResult, iStReg);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, and
 * FOP, and then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 */
IEM_STATIC void iemFpuStoreResultThenPop(PVMCPU pVCpu, PIEMFPURESULT pResult, uint8_t iStReg)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStoreResultOnly(pFpuCtx, pResult, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, FOP,
 * FPUDP, and FPUDS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
IEM_STATIC void iemFpuStoreResultWithMemOp(PVMCPU pVCpu, PIEMFPURESULT pResult, uint8_t iStReg,
                                           uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStoreResultOnly(pFpuCtx, pResult, iStReg);
}


/**
 * Stores a result in a FPU register, updates the FSW, FTW, FPUIP, FPUCS, FOP,
 * FPUDP, and FPUDS, and then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pResult             The result to store.
 * @param   iStReg              Which FPU register to store it in.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
IEM_STATIC void iemFpuStoreResultWithMemOpThenPop(PVMCPU pVCpu, PIEMFPURESULT pResult,
                                                  uint8_t iStReg, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStoreResultOnly(pFpuCtx, pResult, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Updates the FOP, FPUIP, and FPUCS.  For FNOP.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemFpuUpdateOpcodeAndIp(PVMCPU pVCpu)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
}


/**
 * Marks the specified stack register as free (for FFREE).
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iStReg              The register to free.
 */
IEM_STATIC void iemFpuStackFree(PVMCPU pVCpu, uint8_t iStReg)
{
    Assert(iStReg < 8);
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint8_t     iReg    = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    pFpuCtx->FTW &= ~RT_BIT(iReg);
}


/**
 * Increments FSW.TOP, i.e. pops an item off the stack without freeing it.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemFpuStackIncTop(PVMCPU pVCpu)
{
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint16_t    uFsw    = pFpuCtx->FSW;
    uint16_t    uTop    = uFsw & X86_FSW_TOP_MASK;
    uTop  = (uTop + (1 << X86_FSW_TOP_SHIFT)) & X86_FSW_TOP_MASK;
    uFsw &= ~X86_FSW_TOP_MASK;
    uFsw |= uTop;
    pFpuCtx->FSW = uFsw;
}


/**
 * Decrements FSW.TOP, i.e. push an item off the stack without storing anything.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemFpuStackDecTop(PVMCPU pVCpu)
{
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint16_t    uFsw    = pFpuCtx->FSW;
    uint16_t    uTop    = uFsw & X86_FSW_TOP_MASK;
    uTop  = (uTop + (7 << X86_FSW_TOP_SHIFT)) & X86_FSW_TOP_MASK;
    uFsw &= ~X86_FSW_TOP_MASK;
    uFsw |= uTop;
    pFpuCtx->FSW = uFsw;
}


/**
 * Updates the FSW, FOP, FPUIP, and FPUCS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 */
IEM_STATIC void iemFpuUpdateFSW(PVMCPU pVCpu, uint16_t u16FSW)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuUpdateFSWOnly(pFpuCtx, u16FSW);
}


/**
 * Updates the FSW, FOP, FPUIP, and FPUCS, then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 */
IEM_STATIC void iemFpuUpdateFSWThenPop(PVMCPU pVCpu, uint16_t u16FSW)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuUpdateFSWOnly(pFpuCtx, u16FSW);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Updates the FSW, FOP, FPUIP, FPUCS, FPUDP, and FPUDS.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
IEM_STATIC void iemFpuUpdateFSWWithMemOp(PVMCPU pVCpu, uint16_t u16FSW, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuUpdateFSWOnly(pFpuCtx, u16FSW);
}


/**
 * Updates the FSW, FOP, FPUIP, and FPUCS, then pops the stack twice.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 */
IEM_STATIC void iemFpuUpdateFSWThenPopPop(PVMCPU pVCpu, uint16_t u16FSW)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuUpdateFSWOnly(pFpuCtx, u16FSW);
    iemFpuMaybePopOne(pFpuCtx);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Updates the FSW, FOP, FPUIP, FPUCS, FPUDP, and FPUDS, then pops the stack.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16FSW              The FSW from the current instruction.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
IEM_STATIC void iemFpuUpdateFSWWithMemOpThenPop(PVMCPU pVCpu, uint16_t u16FSW, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuUpdateFSWOnly(pFpuCtx, u16FSW);
    iemFpuMaybePopOne(pFpuCtx);
}


/**
 * Worker routine for raising an FPU stack underflow exception.
 *
 * @param   pFpuCtx             The FPU context.
 * @param   iStReg              The stack register being accessed.
 */
IEM_STATIC void iemFpuStackUnderflowOnly(PX86FXSTATE pFpuCtx, uint8_t iStReg)
{
    Assert(iStReg < 8 || iStReg == UINT8_MAX);
    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked underflow. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        uint16_t iReg = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
        if (iStReg != UINT8_MAX)
        {
            pFpuCtx->FTW |= RT_BIT(iReg);
            iemFpuStoreQNan(&pFpuCtx->aRegs[iStReg].r80);
        }
    }
    else
    {
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
    }
}


/**
 * Raises a FPU stack underflow exception.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iStReg              The destination register that should be loaded
 *                              with QNaN if \#IS is not masked. Specify
 *                              UINT8_MAX if none (like for fcom).
 */
DECL_NO_INLINE(IEM_STATIC, void) iemFpuStackUnderflow(PVMCPU pVCpu, uint8_t iStReg)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackUnderflowOnly(pFpuCtx, iStReg);
}


DECL_NO_INLINE(IEM_STATIC, void)
iemFpuStackUnderflowWithMemOp(PVMCPU pVCpu, uint8_t iStReg, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackUnderflowOnly(pFpuCtx, iStReg);
}


DECL_NO_INLINE(IEM_STATIC, void) iemFpuStackUnderflowThenPop(PVMCPU pVCpu, uint8_t iStReg)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackUnderflowOnly(pFpuCtx, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


DECL_NO_INLINE(IEM_STATIC, void)
iemFpuStackUnderflowWithMemOpThenPop(PVMCPU pVCpu, uint8_t iStReg, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx      = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackUnderflowOnly(pFpuCtx, iStReg);
    iemFpuMaybePopOne(pFpuCtx);
}


DECL_NO_INLINE(IEM_STATIC, void) iemFpuStackUnderflowThenPopPop(PVMCPU pVCpu)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackUnderflowOnly(pFpuCtx, UINT8_MAX);
    iemFpuMaybePopOne(pFpuCtx);
    iemFpuMaybePopOne(pFpuCtx);
}


DECL_NO_INLINE(IEM_STATIC, void)
iemFpuStackPushUnderflow(PVMCPU pVCpu)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);

    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked overflow - Push QNaN. */
        uint16_t iNewTop = (X86_FSW_TOP_GET(pFpuCtx->FSW) + 7) & X86_FSW_TOP_SMASK;
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_C_MASK);
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        pFpuCtx->FSW |= iNewTop << X86_FSW_TOP_SHIFT;
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
        iemFpuRotateStackPush(pFpuCtx);
    }
    else
    {
        /* Exception pending - don't change TOP or the register stack. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
    }
}


DECL_NO_INLINE(IEM_STATIC, void)
iemFpuStackPushUnderflowTwo(PVMCPU pVCpu)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);

    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked overflow - Push QNaN. */
        uint16_t iNewTop = (X86_FSW_TOP_GET(pFpuCtx->FSW) + 7) & X86_FSW_TOP_SMASK;
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_C_MASK);
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF;
        pFpuCtx->FSW |= iNewTop << X86_FSW_TOP_SHIFT;
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        iemFpuStoreQNan(&pFpuCtx->aRegs[0].r80);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
        iemFpuRotateStackPush(pFpuCtx);
    }
    else
    {
        /* Exception pending - don't change TOP or the register stack. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
    }
}


/**
 * Worker routine for raising an FPU stack overflow exception on a push.
 *
 * @param   pFpuCtx             The FPU context.
 */
IEM_STATIC void iemFpuStackPushOverflowOnly(PX86FXSTATE pFpuCtx)
{
    if (pFpuCtx->FCW & X86_FCW_IM)
    {
        /* Masked overflow. */
        uint16_t iNewTop = (X86_FSW_TOP_GET(pFpuCtx->FSW) + 7) & X86_FSW_TOP_SMASK;
        pFpuCtx->FSW &= ~(X86_FSW_TOP_MASK | X86_FSW_C_MASK);
        pFpuCtx->FSW |= X86_FSW_C1 | X86_FSW_IE | X86_FSW_SF;
        pFpuCtx->FSW |= iNewTop << X86_FSW_TOP_SHIFT;
        pFpuCtx->FTW |= RT_BIT(iNewTop);
        iemFpuStoreQNan(&pFpuCtx->aRegs[7].r80);
        iemFpuRotateStackPush(pFpuCtx);
    }
    else
    {
        /* Exception pending - don't change TOP or the register stack. */
        pFpuCtx->FSW &= ~X86_FSW_C_MASK;
        pFpuCtx->FSW |= X86_FSW_C1 | X86_FSW_IE | X86_FSW_SF | X86_FSW_ES | X86_FSW_B;
    }
}


/**
 * Raises a FPU stack overflow exception on a push.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
DECL_NO_INLINE(IEM_STATIC, void) iemFpuStackPushOverflow(PVMCPU pVCpu)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackPushOverflowOnly(pFpuCtx);
}


/**
 * Raises a FPU stack overflow exception on a push with a memory operand.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iEffSeg             The effective memory operand selector register.
 * @param   GCPtrEff            The effective memory operand offset.
 */
DECL_NO_INLINE(IEM_STATIC, void)
iemFpuStackPushOverflowWithMemOp(PVMCPU pVCpu, uint8_t iEffSeg, RTGCPTR GCPtrEff)
{
    PCPUMCTX    pCtx    = IEM_GET_CTX(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    iemFpuUpdateDP(pVCpu, pCtx, pFpuCtx, iEffSeg, GCPtrEff);
    iemFpuUpdateOpcodeAndIpWorker(pVCpu, pCtx, pFpuCtx);
    iemFpuStackPushOverflowOnly(pFpuCtx);
}


IEM_STATIC int iemFpuStRegNotEmpty(PVMCPU pVCpu, uint8_t iStReg)
{
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint16_t    iReg    = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    if (pFpuCtx->FTW & RT_BIT(iReg))
        return VINF_SUCCESS;
    return VERR_NOT_FOUND;
}


IEM_STATIC int iemFpuStRegNotEmptyRef(PVMCPU pVCpu, uint8_t iStReg, PCRTFLOAT80U *ppRef)
{
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint16_t    iReg    = (X86_FSW_TOP_GET(pFpuCtx->FSW) + iStReg) & X86_FSW_TOP_SMASK;
    if (pFpuCtx->FTW & RT_BIT(iReg))
    {
        *ppRef = &pFpuCtx->aRegs[iStReg].r80;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}


IEM_STATIC int iemFpu2StRegsNotEmptyRef(PVMCPU pVCpu, uint8_t iStReg0, PCRTFLOAT80U *ppRef0,
                                        uint8_t iStReg1, PCRTFLOAT80U *ppRef1)
{
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint16_t    iTop    = X86_FSW_TOP_GET(pFpuCtx->FSW);
    uint16_t    iReg0   = (iTop + iStReg0) & X86_FSW_TOP_SMASK;
    uint16_t    iReg1   = (iTop + iStReg1) & X86_FSW_TOP_SMASK;
    if ((pFpuCtx->FTW & (RT_BIT(iReg0) | RT_BIT(iReg1))) == (RT_BIT(iReg0) | RT_BIT(iReg1)))
    {
        *ppRef0 = &pFpuCtx->aRegs[iStReg0].r80;
        *ppRef1 = &pFpuCtx->aRegs[iStReg1].r80;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}


IEM_STATIC int iemFpu2StRegsNotEmptyRefFirst(PVMCPU pVCpu, uint8_t iStReg0, PCRTFLOAT80U *ppRef0, uint8_t iStReg1)
{
    PX86FXSTATE pFpuCtx = &IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87;
    uint16_t    iTop    = X86_FSW_TOP_GET(pFpuCtx->FSW);
    uint16_t    iReg0   = (iTop + iStReg0) & X86_FSW_TOP_SMASK;
    uint16_t    iReg1   = (iTop + iStReg1) & X86_FSW_TOP_SMASK;
    if ((pFpuCtx->FTW & (RT_BIT(iReg0) | RT_BIT(iReg1))) == (RT_BIT(iReg0) | RT_BIT(iReg1)))
    {
        *ppRef0 = &pFpuCtx->aRegs[iStReg0].r80;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}


/**
 * Updates the FPU exception status after FCW is changed.
 *
 * @param   pFpuCtx             The FPU context.
 */
IEM_STATIC void iemFpuRecalcExceptionStatus(PX86FXSTATE pFpuCtx)
{
    uint16_t u16Fsw = pFpuCtx->FSW;
    if ((u16Fsw & X86_FSW_XCPT_MASK) & ~(pFpuCtx->FCW & X86_FCW_XCPT_MASK))
        u16Fsw |= X86_FSW_ES | X86_FSW_B;
    else
        u16Fsw &= ~(X86_FSW_ES | X86_FSW_B);
    pFpuCtx->FSW = u16Fsw;
}


/**
 * Calculates the full FTW (FPU tag word) for use in FNSTENV and FNSAVE.
 *
 * @returns The full FTW.
 * @param   pFpuCtx             The FPU context.
 */
IEM_STATIC uint16_t iemFpuCalcFullFtw(PCX86FXSTATE pFpuCtx)
{
    uint8_t const   u8Ftw  = (uint8_t)pFpuCtx->FTW;
    uint16_t        u16Ftw = 0;
    unsigned const  iTop   = X86_FSW_TOP_GET(pFpuCtx->FSW);
    for (unsigned iSt = 0; iSt < 8; iSt++)
    {
        unsigned const iReg = (iSt + iTop) & 7;
        if (!(u8Ftw & RT_BIT(iReg)))
            u16Ftw |= 3 << (iReg * 2); /* empty */
        else
        {
            uint16_t uTag;
            PCRTFLOAT80U const pr80Reg = &pFpuCtx->aRegs[iSt].r80;
            if (pr80Reg->s.uExponent == 0x7fff)
                uTag = 2; /* Exponent is all 1's => Special. */
            else if (pr80Reg->s.uExponent == 0x0000)
            {
                if (pr80Reg->s.u64Mantissa == 0x0000)
                    uTag = 1; /* All bits are zero => Zero. */
                else
                    uTag = 2; /* Must be special. */
            }
            else if (pr80Reg->s.u64Mantissa & RT_BIT_64(63)) /* The J bit. */
                uTag = 0; /* Valid. */
            else
                uTag = 2; /* Must be special. */

            u16Ftw |= uTag << (iReg * 2); /* empty */
        }
    }

    return u16Ftw;
}


/**
 * Converts a full FTW to a compressed one (for use in FLDENV and FRSTOR).
 *
 * @returns The compressed FTW.
 * @param   u16FullFtw      The full FTW to convert.
 */
IEM_STATIC uint16_t iemFpuCompressFtw(uint16_t u16FullFtw)
{
    uint8_t u8Ftw = 0;
    for (unsigned i = 0; i < 8; i++)
    {
        if ((u16FullFtw & 3) != 3 /*empty*/)
            u8Ftw |= RT_BIT(i);
        u16FullFtw >>= 2;
    }

    return u8Ftw;
}

/** @}  */


/** @name   Memory access.
 *
 * @{
 */


/**
 * Updates the IEMCPU::cbWritten counter if applicable.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   fAccess             The access being accounted for.
 * @param   cbMem               The access size.
 */
DECL_FORCE_INLINE(void) iemMemUpdateWrittenCounter(PVMCPU pVCpu, uint32_t fAccess, size_t cbMem)
{
    if (   (fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_WRITE)) == (IEM_ACCESS_WHAT_STACK | IEM_ACCESS_TYPE_WRITE)
        || (fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_WRITE)) == (IEM_ACCESS_WHAT_DATA | IEM_ACCESS_TYPE_WRITE) )
        pVCpu->iem.s.cbWritten += (uint32_t)cbMem;
}


/**
 * Checks if the given segment can be written to, raise the appropriate
 * exception if not.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pHid                Pointer to the hidden register.
 * @param   iSegReg             The register number.
 * @param   pu64BaseAddr        Where to return the base address to use for the
 *                              segment. (In 64-bit code it may differ from the
 *                              base in the hidden segment.)
 */
IEM_STATIC VBOXSTRICTRC
iemMemSegCheckWriteAccessEx(PVMCPU pVCpu, PCCPUMSELREGHID pHid, uint8_t iSegReg, uint64_t *pu64BaseAddr)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        *pu64BaseAddr = iSegReg < X86_SREG_FS ? 0 : pHid->u64Base;
    else
    {
        if (!pHid->Attr.n.u1Present)
        {
            uint16_t    uSel = iemSRegFetchU16(pVCpu, iSegReg);
            AssertRelease(uSel == 0);
            Log(("iemMemSegCheckWriteAccessEx: %#x (index %u) - bad selector -> #GP\n", uSel, iSegReg));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        if (   (   (pHid->Attr.n.u4Type & X86_SEL_TYPE_CODE)
                || !(pHid->Attr.n.u4Type & X86_SEL_TYPE_WRITE) )
            &&  pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT )
            return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, IEM_ACCESS_DATA_W);
        *pu64BaseAddr = pHid->u64Base;
    }
    return VINF_SUCCESS;
}


/**
 * Checks if the given segment can be read from, raise the appropriate
 * exception if not.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pHid                Pointer to the hidden register.
 * @param   iSegReg             The register number.
 * @param   pu64BaseAddr        Where to return the base address to use for the
 *                              segment. (In 64-bit code it may differ from the
 *                              base in the hidden segment.)
 */
IEM_STATIC VBOXSTRICTRC
iemMemSegCheckReadAccessEx(PVMCPU pVCpu, PCCPUMSELREGHID pHid, uint8_t iSegReg, uint64_t *pu64BaseAddr)
{
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
        *pu64BaseAddr = iSegReg < X86_SREG_FS ? 0 : pHid->u64Base;
    else
    {
        if (!pHid->Attr.n.u1Present)
        {
            uint16_t    uSel = iemSRegFetchU16(pVCpu, iSegReg);
            AssertRelease(uSel == 0);
            Log(("iemMemSegCheckReadAccessEx: %#x (index %u) - bad selector -> #GP\n", uSel, iSegReg));
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        if ((pHid->Attr.n.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
            return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, IEM_ACCESS_DATA_R);
        *pu64BaseAddr = pHid->u64Base;
    }
    return VINF_SUCCESS;
}


/**
 * Applies the segment limit, base and attributes.
 *
 * This may raise a \#GP or \#SS.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   fAccess             The kind of access which is being performed.
 * @param   iSegReg             The index of the segment register to apply.
 *                              This is UINT8_MAX if none (for IDT, GDT, LDT,
 *                              TSS, ++).
 * @param   cbMem               The access size.
 * @param   pGCPtrMem           Pointer to the guest memory address to apply
 *                              segmentation to.  Input and output parameter.
 */
IEM_STATIC VBOXSTRICTRC
iemMemApplySegment(PVMCPU pVCpu, uint32_t fAccess, uint8_t iSegReg, size_t cbMem, PRTGCPTR pGCPtrMem)
{
    if (iSegReg == UINT8_MAX)
        return VINF_SUCCESS;

    PCPUMSELREGHID pSel = iemSRegGetHid(pVCpu, iSegReg);
    switch (pVCpu->iem.s.enmCpuMode)
    {
        case IEMMODE_16BIT:
        case IEMMODE_32BIT:
        {
            RTGCPTR32 GCPtrFirst32 = (RTGCPTR32)*pGCPtrMem;
            RTGCPTR32 GCPtrLast32  = GCPtrFirst32 + (uint32_t)cbMem - 1;

            if (   pSel->Attr.n.u1Present
                && !pSel->Attr.n.u1Unusable)
            {
                Assert(pSel->Attr.n.u1DescType);
                if (!(pSel->Attr.n.u4Type & X86_SEL_TYPE_CODE))
                {
                    if (   (fAccess & IEM_ACCESS_TYPE_WRITE)
                        && !(pSel->Attr.n.u4Type & X86_SEL_TYPE_WRITE) )
                        return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, fAccess);

                    if (!IEM_IS_REAL_OR_V86_MODE(pVCpu))
                    {
                        /** @todo CPL check. */
                    }

                    /*
                     * There are two kinds of data selectors, normal and expand down.
                     */
                    if (!(pSel->Attr.n.u4Type & X86_SEL_TYPE_DOWN))
                    {
                        if (   GCPtrFirst32 > pSel->u32Limit
                            || GCPtrLast32  > pSel->u32Limit) /* yes, in real mode too (since 80286). */
                            return iemRaiseSelectorBounds(pVCpu, iSegReg, fAccess);
                    }
                    else
                    {
                       /*
                        * The upper boundary is defined by the B bit, not the G bit!
                        */
                       if (   GCPtrFirst32 < pSel->u32Limit + UINT32_C(1)
                           || GCPtrLast32  > (pSel->Attr.n.u1DefBig ? UINT32_MAX : UINT32_C(0xffff)))
                          return iemRaiseSelectorBounds(pVCpu, iSegReg, fAccess);
                    }
                    *pGCPtrMem = GCPtrFirst32 += (uint32_t)pSel->u64Base;
                }
                else
                {

                    /*
                     * Code selector and usually be used to read thru, writing is
                     * only permitted in real and V8086 mode.
                     */
                    if (   (   (fAccess & IEM_ACCESS_TYPE_WRITE)
                            || (   (fAccess & IEM_ACCESS_TYPE_READ)
                               && !(pSel->Attr.n.u4Type & X86_SEL_TYPE_READ)) )
                        && !IEM_IS_REAL_OR_V86_MODE(pVCpu) )
                        return iemRaiseSelectorInvalidAccess(pVCpu, iSegReg, fAccess);

                    if (   GCPtrFirst32 > pSel->u32Limit
                        || GCPtrLast32  > pSel->u32Limit) /* yes, in real mode too (since 80286). */
                        return iemRaiseSelectorBounds(pVCpu, iSegReg, fAccess);

                    if (!IEM_IS_REAL_OR_V86_MODE(pVCpu))
                    {
                        /** @todo CPL check. */
                    }

                    *pGCPtrMem  = GCPtrFirst32 += (uint32_t)pSel->u64Base;
                }
            }
            else
                return iemRaiseGeneralProtectionFault0(pVCpu);
            return VINF_SUCCESS;
        }

        case IEMMODE_64BIT:
        {
            RTGCPTR GCPtrMem = *pGCPtrMem;
            if (iSegReg == X86_SREG_GS || iSegReg == X86_SREG_FS)
                *pGCPtrMem = GCPtrMem + pSel->u64Base;

            Assert(cbMem >= 1);
            if (RT_LIKELY(X86_IS_CANONICAL(GCPtrMem) && X86_IS_CANONICAL(GCPtrMem + cbMem - 1)))
                return VINF_SUCCESS;
            return iemRaiseGeneralProtectionFault0(pVCpu);
        }

        default:
            AssertFailedReturn(VERR_IEM_IPE_7);
    }
}


/**
 * Translates a virtual address to a physical physical address and checks if we
 * can access the page as specified.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   GCPtrMem            The virtual address.
 * @param   fAccess             The intended access.
 * @param   pGCPhysMem          Where to return the physical address.
 */
IEM_STATIC VBOXSTRICTRC
iemMemPageTranslateAndCheckAccess(PVMCPU pVCpu, RTGCPTR GCPtrMem, uint32_t fAccess, PRTGCPHYS pGCPhysMem)
{
    /** @todo Need a different PGM interface here.  We're currently using
     *        generic / REM interfaces. this won't cut it for R0 & RC. */
    /** @todo If/when PGM handles paged real-mode, we can remove the hack in
     *        iemSvmHandleWorldSwitch to work around raising a page-fault here. */
    RTGCPHYS    GCPhys;
    uint64_t    fFlags;
    int rc = PGMGstGetPage(pVCpu, GCPtrMem, &fFlags, &GCPhys);
    if (RT_FAILURE(rc))
    {
        /** @todo Check unassigned memory in unpaged mode. */
        /** @todo Reserved bits in page tables. Requires new PGM interface. */
        *pGCPhysMem = NIL_RTGCPHYS;
        return iemRaisePageFault(pVCpu, GCPtrMem, fAccess, rc);
    }

    /* If the page is writable and does not have the no-exec bit set, all
       access is allowed.  Otherwise we'll have to check more carefully... */
    if ((fFlags & (X86_PTE_RW | X86_PTE_US | X86_PTE_PAE_NX)) != (X86_PTE_RW | X86_PTE_US))
    {
        /* Write to read only memory? */
        if (   (fAccess & IEM_ACCESS_TYPE_WRITE)
            && !(fFlags & X86_PTE_RW)
            && (       (pVCpu->iem.s.uCpl == 3
                    && !(fAccess & IEM_ACCESS_WHAT_SYS))
                || (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_WP)))
        {
            Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - read-only page -> #PF\n", GCPtrMem));
            *pGCPhysMem = NIL_RTGCPHYS;
            return iemRaisePageFault(pVCpu, GCPtrMem, fAccess & ~IEM_ACCESS_TYPE_READ, VERR_ACCESS_DENIED);
        }

        /* Kernel memory accessed by userland? */
        if (   !(fFlags & X86_PTE_US)
            && pVCpu->iem.s.uCpl == 3
            && !(fAccess & IEM_ACCESS_WHAT_SYS))
        {
            Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - user access to kernel page -> #PF\n", GCPtrMem));
            *pGCPhysMem = NIL_RTGCPHYS;
            return iemRaisePageFault(pVCpu, GCPtrMem, fAccess, VERR_ACCESS_DENIED);
        }

        /* Executing non-executable memory? */
        if (   (fAccess & IEM_ACCESS_TYPE_EXEC)
            && (fFlags & X86_PTE_PAE_NX)
            && (IEM_GET_CTX(pVCpu)->msrEFER & MSR_K6_EFER_NXE) )
        {
            Log(("iemMemPageTranslateAndCheckAccess: GCPtrMem=%RGv - NX -> #PF\n", GCPtrMem));
            *pGCPhysMem = NIL_RTGCPHYS;
            return iemRaisePageFault(pVCpu, GCPtrMem, fAccess & ~(IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_WRITE),
                                     VERR_ACCESS_DENIED);
        }
    }

    /*
     * Set the dirty / access flags.
     * ASSUMES this is set when the address is translated rather than on committ...
     */
    /** @todo testcase: check when A and D bits are actually set by the CPU.  */
    uint32_t fAccessedDirty = fAccess & IEM_ACCESS_TYPE_WRITE ? X86_PTE_D | X86_PTE_A : X86_PTE_A;
    if ((fFlags & fAccessedDirty) != fAccessedDirty)
    {
        int rc2 = PGMGstModifyPage(pVCpu, GCPtrMem, 1, fAccessedDirty, ~(uint64_t)fAccessedDirty);
        AssertRC(rc2);
    }

    GCPhys |= GCPtrMem & PAGE_OFFSET_MASK;
    *pGCPhysMem = GCPhys;
    return VINF_SUCCESS;
}



/**
 * Maps a physical page.
 *
 * @returns VBox status code (see PGMR3PhysTlbGCPhys2Ptr).
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   GCPhysMem           The physical address.
 * @param   fAccess             The intended access.
 * @param   ppvMem              Where to return the mapping address.
 * @param   pLock               The PGM lock.
 */
IEM_STATIC int iemMemPageMap(PVMCPU pVCpu, RTGCPHYS GCPhysMem, uint32_t fAccess, void **ppvMem, PPGMPAGEMAPLOCK pLock)
{
#ifdef IEM_VERIFICATION_MODE_FULL
    /* Force the alternative path so we can ignore writes. */
    if ((fAccess & IEM_ACCESS_TYPE_WRITE) && !pVCpu->iem.s.fNoRem)
    {
        if (IEM_FULL_VERIFICATION_ENABLED(pVCpu))
        {
            int rc2 = PGMPhysIemQueryAccess(pVCpu->CTX_SUFF(pVM), GCPhysMem,
                                            RT_BOOL(fAccess & IEM_ACCESS_TYPE_WRITE), pVCpu->iem.s.fBypassHandlers);
            if (RT_FAILURE(rc2))
                pVCpu->iem.s.fProblematicMemory = true;
        }
        return VERR_PGM_PHYS_TLB_CATCH_ALL;
    }
#endif
#ifdef IEM_LOG_MEMORY_WRITES
    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        return VERR_PGM_PHYS_TLB_CATCH_ALL;
#endif
#ifdef IEM_VERIFICATION_MODE_MINIMAL
    return VERR_PGM_PHYS_TLB_CATCH_ALL;
#endif

    /** @todo This API may require some improving later.  A private deal with PGM
     *        regarding locking and unlocking needs to be struct.  A couple of TLBs
     *        living in PGM, but with publicly accessible inlined access methods
     *        could perhaps be an even better solution. */
    int rc = PGMPhysIemGCPhys2Ptr(pVCpu->CTX_SUFF(pVM), pVCpu,
                                  GCPhysMem,
                                  RT_BOOL(fAccess & IEM_ACCESS_TYPE_WRITE),
                                  pVCpu->iem.s.fBypassHandlers,
                                  ppvMem,
                                  pLock);
    /*Log(("PGMPhysIemGCPhys2Ptr %Rrc pLock=%.*Rhxs\n", rc, sizeof(*pLock), pLock));*/
    AssertMsg(rc == VINF_SUCCESS || RT_FAILURE_NP(rc), ("%Rrc\n", rc));

#ifdef IEM_VERIFICATION_MODE_FULL
    if (RT_FAILURE(rc) && IEM_FULL_VERIFICATION_ENABLED(pVCpu))
        pVCpu->iem.s.fProblematicMemory = true;
#endif
    return rc;
}


/**
 * Unmap a page previously mapped by iemMemPageMap.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   GCPhysMem           The physical address.
 * @param   fAccess             The intended access.
 * @param   pvMem               What iemMemPageMap returned.
 * @param   pLock               The PGM lock.
 */
DECLINLINE(void) iemMemPageUnmap(PVMCPU pVCpu, RTGCPHYS GCPhysMem, uint32_t fAccess, const void *pvMem, PPGMPAGEMAPLOCK pLock)
{
    NOREF(pVCpu);
    NOREF(GCPhysMem);
    NOREF(fAccess);
    NOREF(pvMem);
    PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), pLock);
}


/**
 * Looks up a memory mapping entry.
 *
 * @returns The mapping index (positive) or VERR_NOT_FOUND (negative).
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pvMem           The memory address.
 * @param   fAccess         The access to.
 */
DECLINLINE(int) iemMapLookup(PVMCPU pVCpu, void *pvMem, uint32_t fAccess)
{
    Assert(pVCpu->iem.s.cActiveMappings <= RT_ELEMENTS(pVCpu->iem.s.aMemMappings));
    fAccess &= IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK;
    if (   pVCpu->iem.s.aMemMappings[0].pv == pvMem
        && (pVCpu->iem.s.aMemMappings[0].fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK)) == fAccess)
        return 0;
    if (   pVCpu->iem.s.aMemMappings[1].pv == pvMem
        && (pVCpu->iem.s.aMemMappings[1].fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK)) == fAccess)
        return 1;
    if (   pVCpu->iem.s.aMemMappings[2].pv == pvMem
        && (pVCpu->iem.s.aMemMappings[2].fAccess & (IEM_ACCESS_WHAT_MASK | IEM_ACCESS_TYPE_MASK)) == fAccess)
        return 2;
    return VERR_NOT_FOUND;
}


/**
 * Finds a free memmap entry when using iNextMapping doesn't work.
 *
 * @returns Memory mapping index, 1024 on failure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC unsigned iemMemMapFindFree(PVMCPU pVCpu)
{
    /*
     * The easy case.
     */
    if (pVCpu->iem.s.cActiveMappings == 0)
    {
        pVCpu->iem.s.iNextMapping = 1;
        return 0;
    }

    /* There should be enough mappings for all instructions. */
    AssertReturn(pVCpu->iem.s.cActiveMappings < RT_ELEMENTS(pVCpu->iem.s.aMemMappings), 1024);

    for (unsigned i = 0; i < RT_ELEMENTS(pVCpu->iem.s.aMemMappings); i++)
        if (pVCpu->iem.s.aMemMappings[i].fAccess == IEM_ACCESS_INVALID)
            return i;

    AssertFailedReturn(1024);
}


/**
 * Commits a bounce buffer that needs writing back and unmaps it.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   iMemMap         The index of the buffer to commit.
 * @param   fPostponeFail   Whether we can postpone writer failures to ring-3.
 *                          Always false in ring-3, obviously.
 */
IEM_STATIC VBOXSTRICTRC iemMemBounceBufferCommitAndUnmap(PVMCPU pVCpu, unsigned iMemMap, bool fPostponeFail)
{
    Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED);
    Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE);
#ifdef IN_RING3
    Assert(!fPostponeFail);
    RT_NOREF_PV(fPostponeFail);
#endif

    /*
     * Do the writing.
     */
#ifndef IEM_VERIFICATION_MODE_MINIMAL
    PVM          pVM = pVCpu->CTX_SUFF(pVM);
    if (   !pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned
        && !IEM_VERIFICATION_ENABLED(pVCpu))
    {
        uint16_t const  cbFirst  = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst;
        uint16_t const  cbSecond = pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
        uint8_t const  *pbBuf    = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];
        if (!pVCpu->iem.s.fBypassHandlers)
        {
            /*
             * Carefully and efficiently dealing with access handler return
             * codes make this a little bloated.
             */
            VBOXSTRICTRC rcStrict = PGMPhysWrite(pVM,
                                                 pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst,
                                                 pbBuf,
                                                 cbFirst,
                                                 PGMACCESSORIGIN_IEM);
            if (rcStrict == VINF_SUCCESS)
            {
                if (cbSecond)
                {
                    rcStrict = PGMPhysWrite(pVM,
                                            pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
                                            pbBuf + cbFirst,
                                            cbSecond,
                                            PGMACCESSORIGIN_IEM);
                    if (rcStrict == VINF_SUCCESS)
                    { /* nothing */ }
                    else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                    }
# ifndef IN_RING3
                    else if (fPostponeFail)
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (postponed)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_2ND;
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_IEM);
                        return iemSetPassUpStatus(pVCpu, rcStrict);
                    }
# endif
                    else
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (!!)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        return rcStrict;
                    }
                }
            }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                if (!cbSecond)
                {
                    Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc\n",
                         pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict) ));
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                }
                else
                {
                    VBOXSTRICTRC rcStrict2 = PGMPhysWrite(pVM,
                                                          pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
                                                          pbBuf + cbFirst,
                                                          cbSecond,
                                                          PGMACCESSORIGIN_IEM);
                    if (rcStrict2 == VINF_SUCCESS)
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc GCPhysSecond=%RGp/%#x\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond));
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                    }
                    else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict2))
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc GCPhysSecond=%RGp/%#x %Rrc\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict2) ));
                        PGM_PHYS_RW_DO_UPDATE_STRICT_RC(rcStrict, rcStrict2);
                        rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                    }
# ifndef IN_RING3
                    else if (fPostponeFail)
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (postponed)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                        pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_2ND;
                        VMCPU_FF_SET(pVCpu, VMCPU_FF_IEM);
                        return iemSetPassUpStatus(pVCpu, rcStrict);
                    }
# endif
                    else
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc GCPhysSecond=%RGp/%#x %Rrc (!!)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict2) ));
                        return rcStrict2;
                    }
                }
            }
# ifndef IN_RING3
            else if (fPostponeFail)
            {
                Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (postponed)\n",
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                if (!cbSecond)
                    pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_1ST;
                else
                    pVCpu->iem.s.aMemMappings[iMemMap].fAccess |= IEM_ACCESS_PENDING_R3_WRITE_1ST | IEM_ACCESS_PENDING_R3_WRITE_2ND;
                VMCPU_FF_SET(pVCpu, VMCPU_FF_IEM);
                return iemSetPassUpStatus(pVCpu, rcStrict);
            }
# endif
            else
            {
                Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysWrite GCPhysFirst=%RGp/%#x %Rrc [GCPhysSecond=%RGp/%#x] (!!)\n",
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, VBOXSTRICTRC_VAL(rcStrict),
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond));
                return rcStrict;
            }
        }
        else
        {
            /*
             * No access handlers, much simpler.
             */
            int rc = PGMPhysSimpleWriteGCPhys(pVM, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, pbBuf, cbFirst);
            if (RT_SUCCESS(rc))
            {
                if (cbSecond)
                {
                    rc = PGMPhysSimpleWriteGCPhys(pVM, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, pbBuf + cbFirst, cbSecond);
                    if (RT_SUCCESS(rc))
                    { /* likely */ }
                    else
                    {
                        Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysSimpleWriteGCPhys GCPhysFirst=%RGp/%#x GCPhysSecond=%RGp/%#x %Rrc (!!)\n",
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                             pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond, rc));
                        return rc;
                    }
                }
            }
            else
            {
                Log(("iemMemBounceBufferCommitAndUnmap: PGMPhysSimpleWriteGCPhys GCPhysFirst=%RGp/%#x %Rrc [GCPhysSecond=%RGp/%#x] (!!)\n",
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst, rc,
                     pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond));
                return rc;
            }
        }
    }
#endif

#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)
    /*
     * Record the write(s).
     */
    if (!pVCpu->iem.s.fNoRem)
    {
        PIEMVERIFYEVTREC pEvtRec = iemVerifyAllocRecord(pVCpu);
        if (pEvtRec)
        {
            pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_WRITE;
            pEvtRec->u.RamWrite.GCPhys  = pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst;
            pEvtRec->u.RamWrite.cb      = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst;
            memcpy(pEvtRec->u.RamWrite.ab, &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0], pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst);
            AssertCompile(sizeof(pEvtRec->u.RamWrite.ab) == sizeof(pVCpu->iem.s.aBounceBuffers[0].ab));
            pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
            *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
        }
        if (pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond)
        {
            pEvtRec = iemVerifyAllocRecord(pVCpu);
            if (pEvtRec)
            {
                pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_WRITE;
                pEvtRec->u.RamWrite.GCPhys  = pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond;
                pEvtRec->u.RamWrite.cb      = pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
                memcpy(pEvtRec->u.RamWrite.ab,
                       &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst],
                       pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond);
                pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
                *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
            }
        }
    }
#endif
#if defined(IEM_VERIFICATION_MODE_MINIMAL) || defined(IEM_LOG_MEMORY_WRITES)
    Log(("IEM Wrote %RGp: %.*Rhxs\n", pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst,
         RT_MAX(RT_MIN(pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst, 64), 1), &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0]));
    if (pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond)
        Log(("IEM Wrote %RGp: %.*Rhxs [2nd page]\n", pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
             RT_MIN(pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond, 64),
             &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst]));

    size_t cbWrote = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst + pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
    g_cbIemWrote = cbWrote;
    memcpy(g_abIemWrote, &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0], RT_MIN(cbWrote, sizeof(g_abIemWrote)));
#endif

    /*
     * Free the mapping entry.
     */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
    return VINF_SUCCESS;
}


/**
 * iemMemMap worker that deals with a request crossing pages.
 */
IEM_STATIC VBOXSTRICTRC
iemMemBounceBufferMapCrossPage(PVMCPU pVCpu, int iMemMap, void **ppvMem, size_t cbMem, RTGCPTR GCPtrFirst, uint32_t fAccess)
{
    /*
     * Do the address translations.
     */
    RTGCPHYS GCPhysFirst;
    VBOXSTRICTRC rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrFirst, fAccess, &GCPhysFirst);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    RTGCPHYS GCPhysSecond;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, (GCPtrFirst + (cbMem - 1)) & ~(RTGCPTR)PAGE_OFFSET_MASK,
                                                 fAccess, &GCPhysSecond);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    GCPhysSecond &= ~(RTGCPHYS)PAGE_OFFSET_MASK;

    PVM pVM = pVCpu->CTX_SUFF(pVM);
#ifdef IEM_VERIFICATION_MODE_FULL
    /*
     * Detect problematic memory when verifying so we can select
     * the right execution engine. (TLB: Redo this.)
     */
    if (IEM_FULL_VERIFICATION_ENABLED(pVCpu))
    {
        int rc2 = PGMPhysIemQueryAccess(pVM, GCPhysFirst,  RT_BOOL(fAccess & IEM_ACCESS_TYPE_WRITE), pVCpu->iem.s.fBypassHandlers);
        if (RT_SUCCESS(rc2))
            rc2 = PGMPhysIemQueryAccess(pVM, GCPhysSecond, RT_BOOL(fAccess & IEM_ACCESS_TYPE_WRITE), pVCpu->iem.s.fBypassHandlers);
        if (RT_FAILURE(rc2))
            pVCpu->iem.s.fProblematicMemory = true;
    }
#endif


    /*
     * Read in the current memory content if it's a read, execute or partial
     * write access.
     */
    uint8_t        *pbBuf        = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];
    uint32_t const  cbFirstPage  = PAGE_SIZE - (GCPhysFirst & PAGE_OFFSET_MASK);
    uint32_t const  cbSecondPage = (uint32_t)(cbMem - cbFirstPage);

    if (fAccess & (IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_EXEC | IEM_ACCESS_PARTIAL_WRITE))
    {
        if (!pVCpu->iem.s.fBypassHandlers)
        {
            /*
             * Must carefully deal with access handler status codes here,
             * makes the code a bit bloated.
             */
            rcStrict = PGMPhysRead(pVM, GCPhysFirst, pbBuf, cbFirstPage, PGMACCESSORIGIN_IEM);
            if (rcStrict == VINF_SUCCESS)
            {
                rcStrict = PGMPhysRead(pVM, GCPhysSecond, pbBuf + cbFirstPage, cbSecondPage, PGMACCESSORIGIN_IEM);
                if (rcStrict == VINF_SUCCESS)
                { /*likely */ }
                else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysSecond=%RGp rcStrict2=%Rrc (!!)\n",
                         GCPhysSecond, VBOXSTRICTRC_VAL(rcStrict) ));
                    return rcStrict;
                }
            }
            else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
            {
                VBOXSTRICTRC rcStrict2 = PGMPhysRead(pVM, GCPhysSecond, pbBuf + cbFirstPage, cbSecondPage, PGMACCESSORIGIN_IEM);
                if (PGM_PHYS_RW_IS_SUCCESS(rcStrict2))
                {
                    PGM_PHYS_RW_DO_UPDATE_STRICT_RC(rcStrict, rcStrict2);
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                }
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysSecond=%RGp rcStrict2=%Rrc (rcStrict=%Rrc) (!!)\n",
                         GCPhysSecond, VBOXSTRICTRC_VAL(rcStrict2), VBOXSTRICTRC_VAL(rcStrict2) ));
                    return rcStrict2;
                }
            }
            else
            {
                Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysFirst=%RGp rcStrict=%Rrc (!!)\n",
                     GCPhysFirst, VBOXSTRICTRC_VAL(rcStrict) ));
                return rcStrict;
            }
        }
        else
        {
            /*
             * No informational status codes here, much more straight forward.
             */
            int rc = PGMPhysSimpleReadGCPhys(pVM, pbBuf, GCPhysFirst, cbFirstPage);
            if (RT_SUCCESS(rc))
            {
                Assert(rc == VINF_SUCCESS);
                rc = PGMPhysSimpleReadGCPhys(pVM, pbBuf + cbFirstPage, GCPhysSecond, cbSecondPage);
                if (RT_SUCCESS(rc))
                    Assert(rc == VINF_SUCCESS);
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysSimpleReadGCPhys GCPhysSecond=%RGp rc=%Rrc (!!)\n", GCPhysSecond, rc));
                    return rc;
                }
            }
            else
            {
                Log(("iemMemBounceBufferMapPhys: PGMPhysSimpleReadGCPhys GCPhysFirst=%RGp rc=%Rrc (!!)\n", GCPhysFirst, rc));
                return rc;
            }
        }

#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)
        if (   !pVCpu->iem.s.fNoRem
            && (fAccess & (IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_EXEC)) )
        {
            /*
             * Record the reads.
             */
            PIEMVERIFYEVTREC pEvtRec = iemVerifyAllocRecord(pVCpu);
            if (pEvtRec)
            {
                pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_READ;
                pEvtRec->u.RamRead.GCPhys  = GCPhysFirst;
                pEvtRec->u.RamRead.cb      = cbFirstPage;
                pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
                *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
            }
            pEvtRec = iemVerifyAllocRecord(pVCpu);
            if (pEvtRec)
            {
                pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_READ;
                pEvtRec->u.RamRead.GCPhys  = GCPhysSecond;
                pEvtRec->u.RamRead.cb      = cbSecondPage;
                pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
                *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
            }
        }
#endif
    }
#ifdef VBOX_STRICT
    else
        memset(pbBuf, 0xcc, cbMem);
    if (cbMem < sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab))
        memset(pbBuf + cbMem, 0xaa, sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab) - cbMem);
#endif

    /*
     * Commit the bounce buffer entry.
     */
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst    = GCPhysFirst;
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond   = GCPhysSecond;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst        = (uint16_t)cbFirstPage;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond       = (uint16_t)cbSecondPage;
    pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned    = false;
    pVCpu->iem.s.aMemMappings[iMemMap].pv               = pbBuf;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess          = fAccess | IEM_ACCESS_BOUNCE_BUFFERED;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    *ppvMem = pbBuf;
    return VINF_SUCCESS;
}


/**
 * iemMemMap woker that deals with iemMemPageMap failures.
 */
IEM_STATIC VBOXSTRICTRC iemMemBounceBufferMapPhys(PVMCPU pVCpu, unsigned iMemMap, void **ppvMem, size_t cbMem,
                                                  RTGCPHYS GCPhysFirst, uint32_t fAccess, VBOXSTRICTRC rcMap)
{
    /*
     * Filter out conditions we can handle and the ones which shouldn't happen.
     */
    if (   rcMap != VERR_PGM_PHYS_TLB_CATCH_WRITE
        && rcMap != VERR_PGM_PHYS_TLB_CATCH_ALL
        && rcMap != VERR_PGM_PHYS_TLB_UNASSIGNED)
    {
        AssertReturn(RT_FAILURE_NP(rcMap), VERR_IEM_IPE_8);
        return rcMap;
    }
    pVCpu->iem.s.cPotentialExits++;

    /*
     * Read in the current memory content if it's a read, execute or partial
     * write access.
     */
    uint8_t *pbBuf = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];
    if (fAccess & (IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_EXEC | IEM_ACCESS_PARTIAL_WRITE))
    {
        if (rcMap == VERR_PGM_PHYS_TLB_UNASSIGNED)
            memset(pbBuf, 0xff, cbMem);
        else
        {
            int rc;
            if (!pVCpu->iem.s.fBypassHandlers)
            {
                VBOXSTRICTRC rcStrict = PGMPhysRead(pVCpu->CTX_SUFF(pVM), GCPhysFirst, pbBuf, cbMem, PGMACCESSORIGIN_IEM);
                if (rcStrict == VINF_SUCCESS)
                { /* nothing */ }
                else if (PGM_PHYS_RW_IS_SUCCESS(rcStrict))
                    rcStrict = iemSetPassUpStatus(pVCpu, rcStrict);
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysRead GCPhysFirst=%RGp rcStrict=%Rrc (!!)\n",
                         GCPhysFirst, VBOXSTRICTRC_VAL(rcStrict) ));
                    return rcStrict;
                }
            }
            else
            {
                rc = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), pbBuf, GCPhysFirst, cbMem);
                if (RT_SUCCESS(rc))
                { /* likely */ }
                else
                {
                    Log(("iemMemBounceBufferMapPhys: PGMPhysSimpleReadGCPhys GCPhysFirst=%RGp rcStrict=%Rrc (!!)\n",
                         GCPhysFirst, rc));
                    return rc;
                }
            }
        }

#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)
        if (   !pVCpu->iem.s.fNoRem
            && (fAccess & (IEM_ACCESS_TYPE_READ | IEM_ACCESS_TYPE_EXEC)) )
        {
            /*
             * Record the read.
             */
            PIEMVERIFYEVTREC pEvtRec = iemVerifyAllocRecord(pVCpu);
            if (pEvtRec)
            {
                pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_READ;
                pEvtRec->u.RamRead.GCPhys  = GCPhysFirst;
                pEvtRec->u.RamRead.cb      = (uint32_t)cbMem;
                pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
                *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
            }
        }
#endif
    }
#ifdef VBOX_STRICT
    else
        memset(pbBuf, 0xcc, cbMem);
#endif
#ifdef VBOX_STRICT
    if (cbMem < sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab))
        memset(pbBuf + cbMem, 0xaa, sizeof(pVCpu->iem.s.aBounceBuffers[iMemMap].ab) - cbMem);
#endif

    /*
     * Commit the bounce buffer entry.
     */
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst    = GCPhysFirst;
    pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond   = NIL_RTGCPHYS;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst        = (uint16_t)cbMem;
    pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond       = 0;
    pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned    = rcMap == VERR_PGM_PHYS_TLB_UNASSIGNED;
    pVCpu->iem.s.aMemMappings[iMemMap].pv               = pbBuf;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess          = fAccess | IEM_ACCESS_BOUNCE_BUFFERED;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    *ppvMem = pbBuf;
    return VINF_SUCCESS;
}



/**
 * Maps the specified guest memory for the given kind of access.
 *
 * This may be using bounce buffering of the memory if it's crossing a page
 * boundary or if there is an access handler installed for any of it.  Because
 * of lock prefix guarantees, we're in for some extra clutter when this
 * happens.
 *
 * This may raise a \#GP, \#SS, \#PF or \#AC.
 *
 * @returns VBox strict status code.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   ppvMem              Where to return the pointer to the mapped
 *                              memory.
 * @param   cbMem               The number of bytes to map.  This is usually 1,
 *                              2, 4, 6, 8, 12, 16, 32 or 512.  When used by
 *                              string operations it can be up to a page.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 *                              Use UINT8_MAX to indicate that no segmentation
 *                              is required (for IDT, GDT and LDT accesses).
 * @param   GCPtrMem            The address of the guest memory.
 * @param   fAccess             How the memory is being accessed.  The
 *                              IEM_ACCESS_TYPE_XXX bit is used to figure out
 *                              how to map the memory, while the
 *                              IEM_ACCESS_WHAT_XXX bit is used when raising
 *                              exceptions.
 */
IEM_STATIC VBOXSTRICTRC
iemMemMap(PVMCPU pVCpu, void **ppvMem, size_t cbMem, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t fAccess)
{
    /*
     * Check the input and figure out which mapping entry to use.
     */
    Assert(cbMem <= 64 || cbMem == 512 || cbMem == 256 || cbMem == 108 || cbMem == 104 || cbMem == 94); /* 512 is the max! */
    Assert(~(fAccess & ~(IEM_ACCESS_TYPE_MASK | IEM_ACCESS_WHAT_MASK)));
    Assert(pVCpu->iem.s.cActiveMappings < RT_ELEMENTS(pVCpu->iem.s.aMemMappings));

    unsigned iMemMap = pVCpu->iem.s.iNextMapping;
    if (   iMemMap >= RT_ELEMENTS(pVCpu->iem.s.aMemMappings)
        || pVCpu->iem.s.aMemMappings[iMemMap].fAccess != IEM_ACCESS_INVALID)
    {
        iMemMap = iemMemMapFindFree(pVCpu);
        AssertLogRelMsgReturn(iMemMap < RT_ELEMENTS(pVCpu->iem.s.aMemMappings),
                              ("active=%d fAccess[0] = {%#x, %#x, %#x}\n", pVCpu->iem.s.cActiveMappings,
                               pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess,
                               pVCpu->iem.s.aMemMappings[2].fAccess),
                              VERR_IEM_IPE_9);
    }

    /*
     * Map the memory, checking that we can actually access it.  If something
     * slightly complicated happens, fall back on bounce buffering.
     */
    VBOXSTRICTRC rcStrict = iemMemApplySegment(pVCpu, fAccess, iSegReg, cbMem, &GCPtrMem);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    if ((GCPtrMem & PAGE_OFFSET_MASK) + cbMem > PAGE_SIZE) /* Crossing a page boundary? */
        return iemMemBounceBufferMapCrossPage(pVCpu, iMemMap, ppvMem, cbMem, GCPtrMem, fAccess);

    RTGCPHYS GCPhysFirst;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrMem, fAccess, &GCPhysFirst);
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        Log8(("IEM WR %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));
    if (fAccess & IEM_ACCESS_TYPE_READ)
        Log9(("IEM RD %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));

    void *pvMem;
    rcStrict = iemMemPageMap(pVCpu, GCPhysFirst, fAccess, &pvMem, &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
    if (rcStrict != VINF_SUCCESS)
        return iemMemBounceBufferMapPhys(pVCpu, iMemMap, ppvMem, cbMem, GCPhysFirst, fAccess, rcStrict);

    /*
     * Fill in the mapping table entry.
     */
    pVCpu->iem.s.aMemMappings[iMemMap].pv      = pvMem;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = fAccess;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    *ppvMem = pvMem;
    return VINF_SUCCESS;
}


/**
 * Commits the guest memory if bounce buffered and unmaps it.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The mapping.
 * @param   fAccess             The kind of access.
 */
IEM_STATIC VBOXSTRICTRC iemMemCommitAndUnmap(PVMCPU pVCpu, void *pvMem, uint32_t fAccess)
{
    int iMemMap = iemMapLookup(pVCpu, pvMem, fAccess);
    AssertReturn(iMemMap >= 0, iMemMap);

    /* If it's bounce buffered, we may need to write back the buffer. */
    if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED)
    {
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE)
            return iemMemBounceBufferCommitAndUnmap(pVCpu, iMemMap, false /*fPostponeFail*/);
    }
    /* Otherwise unlock it. */
    else
        PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);

    /* Free the entry. */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
    return VINF_SUCCESS;
}

#ifdef IEM_WITH_SETJMP

/**
 * Maps the specified guest memory for the given kind of access, longjmp on
 * error.
 *
 * This may be using bounce buffering of the memory if it's crossing a page
 * boundary or if there is an access handler installed for any of it.  Because
 * of lock prefix guarantees, we're in for some extra clutter when this
 * happens.
 *
 * This may raise a \#GP, \#SS, \#PF or \#AC.
 *
 * @returns Pointer to the mapped memory.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbMem               The number of bytes to map.  This is usually 1,
 *                              2, 4, 6, 8, 12, 16, 32 or 512.  When used by
 *                              string operations it can be up to a page.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 *                              Use UINT8_MAX to indicate that no segmentation
 *                              is required (for IDT, GDT and LDT accesses).
 * @param   GCPtrMem            The address of the guest memory.
 * @param   fAccess             How the memory is being accessed.  The
 *                              IEM_ACCESS_TYPE_XXX bit is used to figure out
 *                              how to map the memory, while the
 *                              IEM_ACCESS_WHAT_XXX bit is used when raising
 *                              exceptions.
 */
IEM_STATIC void *iemMemMapJmp(PVMCPU pVCpu, size_t cbMem, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t fAccess)
{
    /*
     * Check the input and figure out which mapping entry to use.
     */
    Assert(cbMem <= 64 || cbMem == 512 || cbMem == 108 || cbMem == 104 || cbMem == 94); /* 512 is the max! */
    Assert(~(fAccess & ~(IEM_ACCESS_TYPE_MASK | IEM_ACCESS_WHAT_MASK)));
    Assert(pVCpu->iem.s.cActiveMappings < RT_ELEMENTS(pVCpu->iem.s.aMemMappings));

    unsigned iMemMap = pVCpu->iem.s.iNextMapping;
    if (   iMemMap >= RT_ELEMENTS(pVCpu->iem.s.aMemMappings)
        || pVCpu->iem.s.aMemMappings[iMemMap].fAccess != IEM_ACCESS_INVALID)
    {
        iMemMap = iemMemMapFindFree(pVCpu);
        AssertLogRelMsgStmt(iMemMap < RT_ELEMENTS(pVCpu->iem.s.aMemMappings),
                            ("active=%d fAccess[0] = {%#x, %#x, %#x}\n", pVCpu->iem.s.cActiveMappings,
                             pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess,
                             pVCpu->iem.s.aMemMappings[2].fAccess),
                            longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VERR_IEM_IPE_9));
    }

    /*
     * Map the memory, checking that we can actually access it.  If something
     * slightly complicated happens, fall back on bounce buffering.
     */
    VBOXSTRICTRC rcStrict = iemMemApplySegment(pVCpu, fAccess, iSegReg, cbMem, &GCPtrMem);
    if (rcStrict == VINF_SUCCESS) { /*likely*/ }
    else longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));

    /* Crossing a page boundary? */
    if ((GCPtrMem & PAGE_OFFSET_MASK) + cbMem <= PAGE_SIZE)
    { /* No (likely). */ }
    else
    {
        void *pvMem;
        rcStrict = iemMemBounceBufferMapCrossPage(pVCpu, iMemMap, &pvMem, cbMem, GCPtrMem, fAccess);
        if (rcStrict == VINF_SUCCESS)
            return pvMem;
        longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
    }

    RTGCPHYS GCPhysFirst;
    rcStrict = iemMemPageTranslateAndCheckAccess(pVCpu, GCPtrMem, fAccess, &GCPhysFirst);
    if (rcStrict == VINF_SUCCESS) { /*likely*/ }
    else longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));

    if (fAccess & IEM_ACCESS_TYPE_WRITE)
        Log8(("IEM WR %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));
    if (fAccess & IEM_ACCESS_TYPE_READ)
        Log9(("IEM RD %RGv (%RGp) LB %#zx\n", GCPtrMem, GCPhysFirst, cbMem));

    void *pvMem;
    rcStrict = iemMemPageMap(pVCpu, GCPhysFirst, fAccess, &pvMem, &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
    if (rcStrict == VINF_SUCCESS)
    { /* likely */ }
    else
    {
        rcStrict = iemMemBounceBufferMapPhys(pVCpu, iMemMap, &pvMem, cbMem, GCPhysFirst, fAccess, rcStrict);
        if (rcStrict == VINF_SUCCESS)
            return pvMem;
        longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
    }

    /*
     * Fill in the mapping table entry.
     */
    pVCpu->iem.s.aMemMappings[iMemMap].pv      = pvMem;
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = fAccess;
    pVCpu->iem.s.iNextMapping = iMemMap + 1;
    pVCpu->iem.s.cActiveMappings++;

    iemMemUpdateWrittenCounter(pVCpu, fAccess, cbMem);
    return pvMem;
}


/**
 * Commits the guest memory if bounce buffered and unmaps it, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The mapping.
 * @param   fAccess             The kind of access.
 */
IEM_STATIC void iemMemCommitAndUnmapJmp(PVMCPU pVCpu, void *pvMem, uint32_t fAccess)
{
    int iMemMap = iemMapLookup(pVCpu, pvMem, fAccess);
    AssertStmt(iMemMap >= 0, longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), iMemMap));

    /* If it's bounce buffered, we may need to write back the buffer. */
    if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED)
    {
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE)
        {
            VBOXSTRICTRC rcStrict = iemMemBounceBufferCommitAndUnmap(pVCpu, iMemMap, false /*fPostponeFail*/);
            if (rcStrict == VINF_SUCCESS)
                return;
            longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
        }
    }
    /* Otherwise unlock it. */
    else
        PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);

    /* Free the entry. */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
}

#endif

#ifndef IN_RING3
/**
 * Commits the guest memory if bounce buffered and unmaps it, if any bounce
 * buffer part shows trouble it will be postponed to ring-3 (sets FF and stuff).
 *
 * Allows the instruction to be completed and retired, while the IEM user will
 * return to ring-3 immediately afterwards and do the postponed writes there.
 *
 * @returns VBox status code (no strict statuses).  Caller must check
 *          VMCPU_FF_IEM before repeating string instructions and similar stuff.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The mapping.
 * @param   fAccess             The kind of access.
 */
IEM_STATIC VBOXSTRICTRC iemMemCommitAndUnmapPostponeTroubleToR3(PVMCPU pVCpu, void *pvMem, uint32_t fAccess)
{
    int iMemMap = iemMapLookup(pVCpu, pvMem, fAccess);
    AssertReturn(iMemMap >= 0, iMemMap);

    /* If it's bounce buffered, we may need to write back the buffer. */
    if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED)
    {
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE)
            return iemMemBounceBufferCommitAndUnmap(pVCpu, iMemMap, true /*fPostponeFail*/);
    }
    /* Otherwise unlock it. */
    else
        PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);

    /* Free the entry. */
    pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    Assert(pVCpu->iem.s.cActiveMappings != 0);
    pVCpu->iem.s.cActiveMappings--;
    return VINF_SUCCESS;
}
#endif


/**
 * Rollbacks mappings, releasing page locks and such.
 *
 * The caller shall only call this after checking cActiveMappings.
 *
 * @returns Strict VBox status code to pass up.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemMemRollback(PVMCPU pVCpu)
{
    Assert(pVCpu->iem.s.cActiveMappings > 0);

    uint32_t iMemMap = RT_ELEMENTS(pVCpu->iem.s.aMemMappings);
    while (iMemMap-- > 0)
    {
        uint32_t fAccess = pVCpu->iem.s.aMemMappings[iMemMap].fAccess;
        if (fAccess != IEM_ACCESS_INVALID)
        {
            AssertMsg(!(fAccess & ~IEM_ACCESS_VALID_MASK) && fAccess != 0, ("%#x\n", fAccess));
            pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
            if (!(fAccess & IEM_ACCESS_BOUNCE_BUFFERED))
                PGMPhysReleasePageMappingLock(pVCpu->CTX_SUFF(pVM), &pVCpu->iem.s.aMemMappingLocks[iMemMap].Lock);
            Assert(pVCpu->iem.s.cActiveMappings > 0);
            pVCpu->iem.s.cActiveMappings--;
        }
    }
}


/**
 * Fetches a data byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu8Dst              Where to return the byte.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU8(PVMCPU pVCpu, uint8_t *pu8Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint8_t const *pu8Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu8Src, sizeof(*pu8Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pu8Dst = *pu8Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu8Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data byte, longjmp on error.
 *
 * @returns The byte.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, uint8_t) iemMemFetchDataU8Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint8_t const *pu8Src = (uint8_t const *)iemMemMapJmp(pVCpu, sizeof(*pu8Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    uint8_t const  bRet   = *pu8Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu8Src, IEM_ACCESS_DATA_R);
    return bRet;
}
#endif /* IEM_WITH_SETJMP */


/**
 * Fetches a data word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Dst             Where to return the word.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU16(PVMCPU pVCpu, uint16_t *pu16Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data word, longjmp on error.
 *
 * @returns The word
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, uint16_t) iemMemFetchDataU16Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint16_t const *pu16Src = (uint16_t const *)iemMemMapJmp(pVCpu, sizeof(*pu16Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    uint16_t const u16Ret = *pu16Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu16Src, IEM_ACCESS_DATA_R);
    return u16Ret;
}
#endif


/**
 * Fetches a data dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Dst             Where to return the dword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU32(PVMCPU pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP

IEM_STATIC RTGCPTR iemMemApplySegmentToReadJmp(PVMCPU pVCpu, uint8_t iSegReg, size_t cbMem, RTGCPTR GCPtrMem)
{
    Assert(cbMem >= 1);
    Assert(iSegReg < X86_SREG_COUNT);

    /*
     * 64-bit mode is simpler.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        if (iSegReg >= X86_SREG_FS)
        {
            PCPUMSELREGHID pSel = iemSRegGetHid(pVCpu, iSegReg);
            GCPtrMem += pSel->u64Base;
        }

        if (RT_LIKELY(X86_IS_CANONICAL(GCPtrMem) && X86_IS_CANONICAL(GCPtrMem + cbMem - 1)))
            return GCPtrMem;
    }
    /*
     * 16-bit and 32-bit segmentation.
     */
    else
    {
        PCPUMSELREGHID pSel = iemSRegGetHid(pVCpu, iSegReg);
        if (      (pSel->Attr.u & (X86DESCATTR_P | X86DESCATTR_UNUSABLE | X86_SEL_TYPE_CODE | X86_SEL_TYPE_DOWN))
               == X86DESCATTR_P /* data, expand up */
            ||    (pSel->Attr.u & (X86DESCATTR_P | X86DESCATTR_UNUSABLE | X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ))
               == (X86DESCATTR_P | X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ) /* code, read-only */ )
        {
            /* expand up */
            uint32_t GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem;
            if (RT_LIKELY(   GCPtrLast32 > pSel->u32Limit
                          && GCPtrLast32 > (uint32_t)GCPtrMem))
                return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
        }
        else if (   (pSel->Attr.u & (X86DESCATTR_P | X86DESCATTR_UNUSABLE | X86_SEL_TYPE_CODE | X86_SEL_TYPE_DOWN))
                 == (X86DESCATTR_P | X86_SEL_TYPE_DOWN) /* data, expand down */ )
        {
            /* expand down */
            uint32_t GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem;
            if (RT_LIKELY(   (uint32_t)GCPtrMem >  pSel->u32Limit
                          && GCPtrLast32        <= (pSel->Attr.n.u1DefBig ? UINT32_MAX : UINT32_C(0xffff))
                          && GCPtrLast32 > (uint32_t)GCPtrMem))
                return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
        }
        else
            iemRaiseSelectorInvalidAccessJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_R);
        iemRaiseSelectorBoundsJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_R);
    }
    iemRaiseGeneralProtectionFault0Jmp(pVCpu);
}


IEM_STATIC RTGCPTR iemMemApplySegmentToWriteJmp(PVMCPU pVCpu, uint8_t iSegReg, size_t cbMem, RTGCPTR GCPtrMem)
{
    Assert(cbMem >= 1);
    Assert(iSegReg < X86_SREG_COUNT);

    /*
     * 64-bit mode is simpler.
     */
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        if (iSegReg >= X86_SREG_FS)
        {
            PCPUMSELREGHID pSel = iemSRegGetHid(pVCpu, iSegReg);
            GCPtrMem += pSel->u64Base;
        }

        if (RT_LIKELY(X86_IS_CANONICAL(GCPtrMem) && X86_IS_CANONICAL(GCPtrMem + cbMem - 1)))
            return GCPtrMem;
    }
    /*
     * 16-bit and 32-bit segmentation.
     */
    else
    {
        PCPUMSELREGHID pSel           = iemSRegGetHid(pVCpu, iSegReg);
        uint32_t const fRelevantAttrs = pSel->Attr.u & (  X86DESCATTR_P     | X86DESCATTR_UNUSABLE
                                                        | X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE | X86_SEL_TYPE_DOWN);
        if (fRelevantAttrs == (X86DESCATTR_P | X86_SEL_TYPE_WRITE)) /* data, expand up */
        {
            /* expand up */
            uint32_t GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem;
            if (RT_LIKELY(   GCPtrLast32 > pSel->u32Limit
                          && GCPtrLast32 > (uint32_t)GCPtrMem))
                return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
        }
        else if (fRelevantAttrs == (X86DESCATTR_P | X86_SEL_TYPE_WRITE | X86_SEL_TYPE_DOWN)) /* data, expand up */
        {
            /* expand down */
            uint32_t GCPtrLast32 = (uint32_t)GCPtrMem + (uint32_t)cbMem;
            if (RT_LIKELY(   (uint32_t)GCPtrMem >  pSel->u32Limit
                          && GCPtrLast32        <= (pSel->Attr.n.u1DefBig ? UINT32_MAX : UINT32_C(0xffff))
                          && GCPtrLast32 > (uint32_t)GCPtrMem))
                return (uint32_t)GCPtrMem + (uint32_t)pSel->u64Base;
        }
        else
            iemRaiseSelectorInvalidAccessJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_W);
        iemRaiseSelectorBoundsJmp(pVCpu, iSegReg, IEM_ACCESS_DATA_W);
    }
    iemRaiseGeneralProtectionFault0Jmp(pVCpu);
}


/**
 * Fetches a data dword, longjmp on error, fallback/safe version.
 *
 * @returns The dword
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC uint32_t iemMemFetchDataU32SafeJmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    uint32_t const *pu32Src = (uint32_t const *)iemMemMapJmp(pVCpu, sizeof(*pu32Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    uint32_t const  u32Ret  = *pu32Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    return u32Ret;
}


/**
 * Fetches a data dword, longjmp on error.
 *
 * @returns The dword
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, uint32_t) iemMemFetchDataU32Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
# ifdef IEM_WITH_DATA_TLB
    RTGCPTR GCPtrEff = iemMemApplySegmentToReadJmp(pVCpu, iSegReg, sizeof(uint32_t), GCPtrMem);
    if (RT_LIKELY((GCPtrEff & X86_PAGE_OFFSET_MASK) <= X86_PAGE_SIZE - sizeof(uint32_t)))
    {
        /// @todo more later.
    }

    return iemMemFetchDataU32SafeJmp(pVCpu, iSegReg, GCPtrMem);
# else
    /* The lazy approach. */
    uint32_t const *pu32Src = (uint32_t const *)iemMemMapJmp(pVCpu, sizeof(*pu32Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    uint32_t const  u32Ret  = *pu32Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu32Src, IEM_ACCESS_DATA_R);
    return u32Ret;
# endif
}
#endif


#ifdef SOME_UNUSED_FUNCTION
/**
 * Fetches a data dword and sign extends it to a qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the sign extended value.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataS32SxU64(PVMCPU pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    int32_t const *pi32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pi32Src, sizeof(*pi32Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pi32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pi32Src, IEM_ACCESS_DATA_R);
    }
#ifdef __GNUC__ /* warning: GCC may be a royal pain */
    else
        *pu64Dst = 0;
#endif
    return rc;
}
#endif


/**
 * Fetches a data qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU64(PVMCPU pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data qword, longjmp on error.
 *
 * @returns The qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, uint64_t) iemMemFetchDataU64Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src = (uint64_t const *)iemMemMapJmp(pVCpu, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    uint64_t const u64Ret = *pu64Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    return u64Ret;
}
#endif


/**
 * Fetches a data qword, aligned at a 16 byte boundrary (for SSE).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU64AlignedU128(PVMCPU pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    /** @todo testcase: Ordering of \#SS(0) vs \#GP() vs \#PF on SSE stuff. */
    if (RT_UNLIKELY(GCPtrMem & 15))
        return iemRaiseGeneralProtectionFault0(pVCpu);

    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data qword, longjmp on error.
 *
 * @returns The qword.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, uint64_t) iemMemFetchDataU64AlignedU128Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    /** @todo testcase: Ordering of \#SS(0) vs \#GP() vs \#PF on SSE stuff. */
    if (RT_LIKELY(!(GCPtrMem & 15)))
    {
        uint64_t const *pu64Src = (uint64_t const *)iemMemMapJmp(pVCpu, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
        uint64_t const u64Ret = *pu64Src;
        iemMemCommitAndUnmapJmp(pVCpu, (void *)pu64Src, IEM_ACCESS_DATA_R);
        return u64Ret;
    }

    VBOXSTRICTRC rc = iemRaiseGeneralProtectionFault0(pVCpu);
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rc));
}
#endif


/**
 * Fetches a data tword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pr80Dst             Where to return the tword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataR80(PVMCPU pVCpu, PRTFLOAT80U pr80Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    PCRTFLOAT80U pr80Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pr80Src, sizeof(*pr80Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        *pr80Dst = *pr80Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pr80Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data tword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pr80Dst             Where to return the tword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, void) iemMemFetchDataR80Jmp(PVMCPU pVCpu, PRTFLOAT80U pr80Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    PCRTFLOAT80U pr80Src = (PCRTFLOAT80U)iemMemMapJmp(pVCpu, sizeof(*pr80Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    *pr80Dst = *pr80Src;
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pr80Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data dqword (double qword), generally SSE related.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU128(PVMCPU pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    PCRTUINT128U pu128Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Src, sizeof(*pu128Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = pu128Src->au64[0];
        pu128Dst->au64[1] = pu128Src->au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data dqword (double qword), generally SSE related.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC void iemMemFetchDataU128Jmp(PVMCPU pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    PCRTUINT128U pu128Src = (PCRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    pu128Dst->au64[0] = pu128Src->au64[0];
    pu128Dst->au64[1] = pu128Src->au64[1];
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data dqword (double qword) at an aligned address, generally SSE
 * related.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU128AlignedSse(PVMCPU pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    /** @todo testcase: Ordering of \#SS(0) vs \#GP() vs \#PF on SSE stuff. */
    if (   (GCPtrMem & 15)
        && !(IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.MXCSR & X86_MXCSR_MM)) /** @todo should probably check this *after* applying seg.u64Base... Check real HW. */
        return iemRaiseGeneralProtectionFault0(pVCpu);

    PCRTUINT128U pu128Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Src, sizeof(*pu128Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = pu128Src->au64[0];
        pu128Dst->au64[1] = pu128Src->au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data dqword (double qword) at an aligned address, generally SSE
 * related, longjmp on error.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu128Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, void) iemMemFetchDataU128AlignedSseJmp(PVMCPU pVCpu, PRTUINT128U pu128Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    /** @todo testcase: Ordering of \#SS(0) vs \#GP() vs \#PF on SSE stuff. */
    if (   (GCPtrMem & 15) == 0
        || (IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.MXCSR & X86_MXCSR_MM)) /** @todo should probably check this *after* applying seg.u64Base... Check real HW. */
    {
        PCRTUINT128U pu128Src = (PCRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
        pu128Dst->au64[0] = pu128Src->au64[0];
        pu128Dst->au64[1] = pu128Src->au64[1];
        iemMemCommitAndUnmapJmp(pVCpu, (void *)pu128Src, IEM_ACCESS_DATA_R);
        return;
    }

    VBOXSTRICTRC rcStrict = iemRaiseGeneralProtectionFault0(pVCpu);
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
}
#endif


/**
 * Fetches a data oword (octo word), generally AVX related.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU256(PVMCPU pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    PCRTUINT256U pu256Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Src, sizeof(*pu256Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Src->au64[0];
        pu256Dst->au64[1] = pu256Src->au64[1];
        pu256Dst->au64[2] = pu256Src->au64[2];
        pu256Dst->au64[3] = pu256Src->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data oword (octo word), generally AVX related.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC void iemMemFetchDataU256Jmp(PVMCPU pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    PCRTUINT256U pu256Src = (PCRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    pu256Dst->au64[0] = pu256Src->au64[0];
    pu256Dst->au64[1] = pu256Src->au64[1];
    pu256Dst->au64[2] = pu256Src->au64[2];
    pu256Dst->au64[3] = pu256Src->au64[3];
    iemMemCommitAndUnmapJmp(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
}
#endif


/**
 * Fetches a data oword (octo word) at an aligned address, generally AVX
 * related.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataU256AlignedSse(PVMCPU pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    /** @todo testcase: Ordering of \#SS(0) vs \#GP() vs \#PF on AVX stuff. */
    if (GCPtrMem & 31)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    PCRTUINT256U pu256Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Src, sizeof(*pu256Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Src->au64[0];
        pu256Dst->au64[1] = pu256Src->au64[1];
        pu256Dst->au64[2] = pu256Src->au64[2];
        pu256Dst->au64[3] = pu256Src->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Fetches a data oword (octo word) at an aligned address, generally AVX
 * related, longjmp on error.
 *
 * Raises \#GP(0) if not aligned.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu256Dst            Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
DECL_NO_INLINE(IEM_STATIC, void) iemMemFetchDataU256AlignedSseJmp(PVMCPU pVCpu, PRTUINT256U pu256Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    /** @todo testcase: Ordering of \#SS(0) vs \#GP() vs \#PF on AVX stuff. */
    if ((GCPtrMem & 31) == 0)
    {
        PCRTUINT256U pu256Src = (PCRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Src), iSegReg, GCPtrMem, IEM_ACCESS_DATA_R);
        pu256Dst->au64[0] = pu256Src->au64[0];
        pu256Dst->au64[1] = pu256Src->au64[1];
        pu256Dst->au64[2] = pu256Src->au64[2];
        pu256Dst->au64[3] = pu256Src->au64[3];
        iemMemCommitAndUnmapJmp(pVCpu, (void *)pu256Src, IEM_ACCESS_DATA_R);
        return;
    }

    VBOXSTRICTRC rcStrict = iemRaiseGeneralProtectionFault0(pVCpu);
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
}
#endif



/**
 * Fetches a descriptor register (lgdt, lidt).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pcbLimit            Where to return the limit.
 * @param   pGCPtrBase          Where to return the base.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   enmOpSize           The effective operand size.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchDataXdtr(PVMCPU pVCpu, uint16_t *pcbLimit, PRTGCPTR pGCPtrBase, uint8_t iSegReg,
                                            RTGCPTR GCPtrMem, IEMMODE enmOpSize)
{
    /*
     * Just like SIDT and SGDT, the LIDT and LGDT instructions are a
     * little special:
     *      - The two reads are done separately.
     *      - Operand size override works in 16-bit and 32-bit code, but 64-bit.
     *      - We suspect the 386 to actually commit the limit before the base in
     *        some cases (search for 386 in  bs3CpuBasic2_lidt_lgdt_One).  We
     *        don't try emulate this eccentric behavior, because it's not well
     *        enough understood and rather hard to trigger.
     *      - The 486 seems to do a dword limit read when the operand size is 32-bit.
     */
    VBOXSTRICTRC rcStrict;
    if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT)
    {
        rcStrict = iemMemFetchDataU16(pVCpu, pcbLimit, iSegReg, GCPtrMem);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemFetchDataU64(pVCpu, pGCPtrBase, iSegReg, GCPtrMem + 2);
    }
    else
    {
        uint32_t uTmp = 0; /* (Visual C++ maybe used uninitialized) */
        if (enmOpSize == IEMMODE_32BIT)
        {
            if (IEM_GET_TARGET_CPU(pVCpu) != IEMTARGETCPU_486)
            {
                rcStrict = iemMemFetchDataU16(pVCpu, pcbLimit, iSegReg, GCPtrMem);
                if (rcStrict == VINF_SUCCESS)
                    rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem + 2);
            }
            else
            {
                rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem);
                if (rcStrict == VINF_SUCCESS)
                {
                    *pcbLimit = (uint16_t)uTmp;
                    rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem + 2);
                }
            }
            if (rcStrict == VINF_SUCCESS)
                *pGCPtrBase = uTmp;
        }
        else
        {
            rcStrict = iemMemFetchDataU16(pVCpu, pcbLimit, iSegReg, GCPtrMem);
            if (rcStrict == VINF_SUCCESS)
            {
                rcStrict = iemMemFetchDataU32(pVCpu, &uTmp, iSegReg, GCPtrMem + 2);
                if (rcStrict == VINF_SUCCESS)
                    *pGCPtrBase = uTmp & UINT32_C(0x00ffffff);
            }
        }
    }
    return rcStrict;
}



/**
 * Stores a data byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u8Value             The value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU8(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint8_t u8Value)
{
    /* The lazy approach for now... */
    uint8_t *pu8Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu8Dst, sizeof(*pu8Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        *pu8Dst = u8Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu8Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data byte, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u8Value             The value to store.
 */
IEM_STATIC void iemMemStoreDataU8Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint8_t u8Value)
{
    /* The lazy approach for now... */
    uint8_t *pu8Dst = (uint8_t *)iemMemMapJmp(pVCpu, sizeof(*pu8Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    *pu8Dst = u8Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu8Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u16Value            The value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU16(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint16_t u16Value)
{
    /* The lazy approach for now... */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(*pu16Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = u16Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data word, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u16Value            The value to store.
 */
IEM_STATIC void iemMemStoreDataU16Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint16_t u16Value)
{
    /* The lazy approach for now... */
    uint16_t *pu16Dst = (uint16_t *)iemMemMapJmp(pVCpu, sizeof(*pu16Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    *pu16Dst = u16Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu16Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u32Value            The value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU32(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t u32Value)
{
    /* The lazy approach for now... */
    uint32_t *pu32Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u32Value            The value to store.
 */
IEM_STATIC void iemMemStoreDataU32Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint32_t u32Value)
{
    /* The lazy approach for now... */
    uint32_t *pu32Dst = (uint32_t *)iemMemMapJmp(pVCpu, sizeof(*pu32Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    *pu32Dst = u32Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu32Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u64Value            The value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU64(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint64_t u64Value)
{
    /* The lazy approach for now... */
    uint64_t *pu64Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Dst, sizeof(*pu64Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = u64Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu64Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data qword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u64Value            The value to store.
 */
IEM_STATIC void iemMemStoreDataU64Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, uint64_t u64Value)
{
    /* The lazy approach for now... */
    uint64_t *pu64Dst = (uint64_t *)iemMemMapJmp(pVCpu, sizeof(*pu64Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    *pu64Dst = u64Value;
    iemMemCommitAndUnmapJmp(pVCpu, pu64Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value            The value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU128(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value)
{
    /* The lazy approach for now... */
    PRTUINT128U pu128Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Dst, sizeof(*pu128Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = u128Value.au64[0];
        pu128Dst->au64[1] = u128Value.au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value            The value to store.
 */
IEM_STATIC void iemMemStoreDataU128Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value)
{
    /* The lazy approach for now... */
    PRTUINT128U pu128Dst = (PRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    pu128Dst->au64[0] = u128Value.au64[0];
    pu128Dst->au64[1] = u128Value.au64[1];
    iemMemCommitAndUnmapJmp(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword, SSE aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value           The value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU128AlignedSse(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value)
{
    /* The lazy approach for now... */
    if (   (GCPtrMem & 15)
        && !(IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.MXCSR & X86_MXCSR_MM)) /** @todo should probably check this *after* applying seg.u64Base... Check real HW. */
        return iemRaiseGeneralProtectionFault0(pVCpu);

    PRTUINT128U pu128Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu128Dst, sizeof(*pu128Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        pu128Dst->au64[0] = u128Value.au64[0];
        pu128Dst->au64[1] = u128Value.au64[1];
        rc = iemMemCommitAndUnmap(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, SSE aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   u128Value           The value to store.
 */
DECL_NO_INLINE(IEM_STATIC, void)
iemMemStoreDataU128AlignedSseJmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, RTUINT128U u128Value)
{
    /* The lazy approach for now... */
    if (   (GCPtrMem & 15) == 0
        || (IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.MXCSR & X86_MXCSR_MM)) /** @todo should probably check this *after* applying seg.u64Base... Check real HW. */
    {
        PRTUINT128U pu128Dst = (PRTUINT128U)iemMemMapJmp(pVCpu, sizeof(*pu128Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
        pu128Dst->au64[0] = u128Value.au64[0];
        pu128Dst->au64[1] = u128Value.au64[1];
        iemMemCommitAndUnmapJmp(pVCpu, pu128Dst, IEM_ACCESS_DATA_W);
        return;
    }

    VBOXSTRICTRC rcStrict = iemRaiseGeneralProtectionFault0(pVCpu);
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
}
#endif


/**
 * Stores a data dqword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU256(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value)
{
    /* The lazy approach for now... */
    PRTUINT256U pu256Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Dst, sizeof(*pu256Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Value->au64[0];
        pu256Dst->au64[1] = pu256Value->au64[1];
        pu256Dst->au64[2] = pu256Value->au64[2];
        pu256Dst->au64[3] = pu256Value->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, longjmp on error.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
IEM_STATIC void iemMemStoreDataU256Jmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value)
{
    /* The lazy approach for now... */
    PRTUINT256U pu256Dst = (PRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    pu256Dst->au64[0] = pu256Value->au64[0];
    pu256Dst->au64[1] = pu256Value->au64[1];
    pu256Dst->au64[2] = pu256Value->au64[2];
    pu256Dst->au64[3] = pu256Value->au64[3];
    iemMemCommitAndUnmapJmp(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
}
#endif


/**
 * Stores a data dqword, AVX aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
IEM_STATIC VBOXSTRICTRC iemMemStoreDataU256AlignedAvx(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value)
{
    /* The lazy approach for now... */
    if (GCPtrMem & 31)
        return iemRaiseGeneralProtectionFault0(pVCpu);

    PRTUINT256U pu256Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu256Dst, sizeof(*pu256Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
    if (rc == VINF_SUCCESS)
    {
        pu256Dst->au64[0] = pu256Value->au64[0];
        pu256Dst->au64[1] = pu256Value->au64[1];
        pu256Dst->au64[2] = pu256Value->au64[2];
        pu256Dst->au64[3] = pu256Value->au64[3];
        rc = iemMemCommitAndUnmap(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
    }
    return rc;
}


#ifdef IEM_WITH_SETJMP
/**
 * Stores a data dqword, AVX aligned.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 * @param   pu256Value          Pointer to the value to store.
 */
DECL_NO_INLINE(IEM_STATIC, void)
iemMemStoreDataU256AlignedAvxJmp(PVMCPU pVCpu, uint8_t iSegReg, RTGCPTR GCPtrMem, PCRTUINT256U pu256Value)
{
    /* The lazy approach for now... */
    if ((GCPtrMem & 31) == 0)
    {
        PRTUINT256U pu256Dst = (PRTUINT256U)iemMemMapJmp(pVCpu, sizeof(*pu256Dst), iSegReg, GCPtrMem, IEM_ACCESS_DATA_W);
        pu256Dst->au64[0] = pu256Value->au64[0];
        pu256Dst->au64[1] = pu256Value->au64[1];
        pu256Dst->au64[2] = pu256Value->au64[2];
        pu256Dst->au64[3] = pu256Value->au64[3];
        iemMemCommitAndUnmapJmp(pVCpu, pu256Dst, IEM_ACCESS_DATA_W);
        return;
    }

    VBOXSTRICTRC rcStrict = iemRaiseGeneralProtectionFault0(pVCpu);
    longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VBOXSTRICTRC_VAL(rcStrict));
}
#endif


/**
 * Stores a descriptor register (sgdt, sidt).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbLimit             The limit.
 * @param   GCPtrBase           The base address.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC
iemMemStoreDataXdtr(PVMCPU pVCpu, uint16_t cbLimit, RTGCPTR GCPtrBase, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    VBOXSTRICTRC rcStrict;
    if (IEM_IS_SVM_CTRL_INTERCEPT_SET(pVCpu, SVM_CTRL_INTERCEPT_IDTR_READS))
    {
        Log(("sidt/sgdt: Guest intercept -> #VMEXIT\n"));
        IEM_RETURN_SVM_VMEXIT(pVCpu, SVM_EXIT_IDTR_READ, 0 /* uExitInfo1 */, 0 /* uExitInfo2 */);
    }

    /*
     * The SIDT and SGDT instructions actually stores the data using two
     * independent writes.  The instructions does not respond to opsize prefixes.
     */
    rcStrict = iemMemStoreDataU16(pVCpu, iSegReg, GCPtrMem, cbLimit);
    if (rcStrict == VINF_SUCCESS)
    {
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_16BIT)
            rcStrict = iemMemStoreDataU32(pVCpu, iSegReg, GCPtrMem + 2,
                                          IEM_GET_TARGET_CPU(pVCpu) <= IEMTARGETCPU_286
                                          ? (uint32_t)GCPtrBase | UINT32_C(0xff000000) : (uint32_t)GCPtrBase);
        else if (pVCpu->iem.s.enmCpuMode == IEMMODE_32BIT)
            rcStrict = iemMemStoreDataU32(pVCpu, iSegReg, GCPtrMem + 2, (uint32_t)GCPtrBase);
        else
            rcStrict = iemMemStoreDataU64(pVCpu, iSegReg, GCPtrMem + 2, GCPtrBase);
    }
    return rcStrict;
}


/**
 * Pushes a word onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16Value            The value to push.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU16(PVMCPU pVCpu, uint16_t u16Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, pCtx, 2, &uNewRsp);

    /* Write the word the lazy way. */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(*pu16Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = u16Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pCtx->rsp = uNewRsp;

    return rc;
}


/**
 * Pushes a dword onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u32Value            The value to push.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU32(PVMCPU pVCpu, uint32_t u32Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, pCtx, 4, &uNewRsp);

    /* Write the dword the lazy way. */
    uint32_t *pu32Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pCtx->rsp = uNewRsp;

    return rc;
}


/**
 * Pushes a dword segment register value onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u32Value            The value to push.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU32SReg(PVMCPU pVCpu, uint32_t u32Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, pCtx, 4, &uNewRsp);

    VBOXSTRICTRC rc;
    if (IEM_FULL_VERIFICATION_REM_ENABLED(pVCpu))
    {
        /* The recompiler writes a full dword. */
        uint32_t *pu32Dst;
        rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
        if (rc == VINF_SUCCESS)
        {
            *pu32Dst = u32Value;
            rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_STACK_W);
        }
    }
    else
    {
        /* The intel docs talks about zero extending the selector register
           value.  My actual intel CPU here might be zero extending the value
           but it still only writes the lower word... */
        /** @todo Test this on new HW and on AMD and in 64-bit mode.  Also test what
         * happens when crossing an electric page boundrary, is the high word checked
         * for write accessibility or not? Probably it is.  What about segment limits?
         * It appears this behavior is also shared with trap error codes.
         *
         * Docs indicate the behavior changed maybe in Pentium or Pentium Pro. Check
         * ancient hardware when it actually did change. */
        uint16_t *pu16Dst;
        rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(uint32_t), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_RW);
        if (rc == VINF_SUCCESS)
        {
            *pu16Dst = (uint16_t)u32Value;
            rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_STACK_RW);
        }
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pCtx->rsp = uNewRsp;

    return rc;
}


/**
 * Pushes a qword onto the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u64Value            The value to push.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU64(PVMCPU pVCpu, uint64_t u64Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, pCtx, 8, &uNewRsp);

    /* Write the word the lazy way. */
    uint64_t *pu64Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Dst, sizeof(*pu64Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = u64Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu64Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        pCtx->rsp = uNewRsp;

    return rc;
}


/**
 * Pops a word from the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Value           Where to store the popped value.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopU16(PVMCPU pVCpu, uint16_t *pu16Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, pCtx, 2, &uNewRsp);

    /* Write the word the lazy way. */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
    if (rc == VINF_SUCCESS)
    {
        *pu16Value = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            pCtx->rsp = uNewRsp;
    }

    return rc;
}


/**
 * Pops a dword from the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Value           Where to store the popped value.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopU32(PVMCPU pVCpu, uint32_t *pu32Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, pCtx, 4, &uNewRsp);

    /* Write the word the lazy way. */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
    if (rc == VINF_SUCCESS)
    {
        *pu32Value = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            pCtx->rsp = uNewRsp;
    }

    return rc;
}


/**
 * Pops a qword from the stack.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Value           Where to store the popped value.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopU64(PVMCPU pVCpu, uint64_t *pu64Value)
{
    /* Increment the stack pointer. */
    uint64_t    uNewRsp;
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, pCtx, 8, &uNewRsp);

    /* Write the word the lazy way. */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
    if (rc == VINF_SUCCESS)
    {
        *pu64Value = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            pCtx->rsp = uNewRsp;
    }

    return rc;
}


/**
 * Pushes a word onto the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u16Value            The value to push.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU16Ex(PVMCPU pVCpu, uint16_t u16Value, PRTUINT64U pTmpRsp)
{
    /* Increment the stack pointer. */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPushEx(pVCpu, pCtx, &NewRsp, 2);

    /* Write the word the lazy way. */
    uint16_t *pu16Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Dst, sizeof(*pu16Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = u16Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu16Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        *pTmpRsp = NewRsp;

    return rc;
}


/**
 * Pushes a dword onto the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u32Value            The value to push.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU32Ex(PVMCPU pVCpu, uint32_t u32Value, PRTUINT64U pTmpRsp)
{
    /* Increment the stack pointer. */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPushEx(pVCpu, pCtx, &NewRsp, 4);

    /* Write the word the lazy way. */
    uint32_t *pu32Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Dst, sizeof(*pu32Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = u32Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu32Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        *pTmpRsp = NewRsp;

    return rc;
}


/**
 * Pushes a dword onto the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   u64Value            The value to push.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushU64Ex(PVMCPU pVCpu, uint64_t u64Value, PRTUINT64U pTmpRsp)
{
    /* Increment the stack pointer. */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPushEx(pVCpu, pCtx, &NewRsp, 8);

    /* Write the word the lazy way. */
    uint64_t *pu64Dst;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Dst, sizeof(*pu64Dst), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = u64Value;
        rc = iemMemCommitAndUnmap(pVCpu, pu64Dst, IEM_ACCESS_STACK_W);
    }

    /* Commit the new RSP value unless we an access handler made trouble. */
    if (rc == VINF_SUCCESS)
        *pTmpRsp = NewRsp;

    return rc;
}


/**
 * Pops a word from the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Value           Where to store the popped value.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopU16Ex(PVMCPU pVCpu, uint16_t *pu16Value, PRTUINT64U pTmpRsp)
{
    /* Increment the stack pointer. */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, pCtx, &NewRsp, 2);

    /* Write the word the lazy way. */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
    if (rc == VINF_SUCCESS)
    {
        *pu16Value = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            *pTmpRsp = NewRsp;
    }

    return rc;
}


/**
 * Pops a dword from the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Value           Where to store the popped value.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopU32Ex(PVMCPU pVCpu, uint32_t *pu32Value, PRTUINT64U pTmpRsp)
{
    /* Increment the stack pointer. */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, pCtx, &NewRsp, 4);

    /* Write the word the lazy way. */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
    if (rc == VINF_SUCCESS)
    {
        *pu32Value = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rc == VINF_SUCCESS)
            *pTmpRsp = NewRsp;
    }

    return rc;
}


/**
 * Pops a qword from the stack, using a temporary stack pointer.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Value           Where to store the popped value.
 * @param   pTmpRsp             Pointer to the temporary stack pointer.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopU64Ex(PVMCPU pVCpu, uint64_t *pu64Value, PRTUINT64U pTmpRsp)
{
    /* Increment the stack pointer. */
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp = *pTmpRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, pCtx, &NewRsp, 8);

    /* Write the word the lazy way. */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rcStrict = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
    if (rcStrict == VINF_SUCCESS)
    {
        *pu64Value = *pu64Src;
        rcStrict = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_STACK_R);

        /* Commit the new RSP value. */
        if (rcStrict == VINF_SUCCESS)
            *pTmpRsp = NewRsp;
    }

    return rcStrict;
}


/**
 * Begin a special stack push (used by interrupt, exceptions and such).
 *
 * This will raise \#SS or \#PF if appropriate.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbMem               The number of bytes to push onto the stack.
 * @param   ppvMem              Where to return the pointer to the stack memory.
 *                              As with the other memory functions this could be
 *                              direct access or bounce buffered access, so
 *                              don't commit register until the commit call
 *                              succeeds.
 * @param   puNewRsp            Where to return the new RSP value.  This must be
 *                              passed unchanged to
 *                              iemMemStackPushCommitSpecial().
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushBeginSpecial(PVMCPU pVCpu, size_t cbMem, void **ppvMem, uint64_t *puNewRsp)
{
    Assert(cbMem < UINT8_MAX);
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPush(pVCpu, pCtx, (uint8_t)cbMem, puNewRsp);
    return iemMemMap(pVCpu, ppvMem, cbMem, X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_W);
}


/**
 * Commits a special stack push (started by iemMemStackPushBeginSpecial).
 *
 * This will update the rSP.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The pointer returned by
 *                              iemMemStackPushBeginSpecial().
 * @param   uNewRsp             The new RSP value returned by
 *                              iemMemStackPushBeginSpecial().
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPushCommitSpecial(PVMCPU pVCpu, void *pvMem, uint64_t uNewRsp)
{
    VBOXSTRICTRC rcStrict = iemMemCommitAndUnmap(pVCpu, pvMem, IEM_ACCESS_STACK_W);
    if (rcStrict == VINF_SUCCESS)
        IEM_GET_CTX(pVCpu)->rsp = uNewRsp;
    return rcStrict;
}


/**
 * Begin a special stack pop (used by iret, retf and such).
 *
 * This will raise \#SS or \#PF if appropriate.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbMem               The number of bytes to pop from the stack.
 * @param   ppvMem              Where to return the pointer to the stack memory.
 * @param   puNewRsp            Where to return the new RSP value.  This must be
 *                              assigned to CPUMCTX::rsp manually some time
 *                              after iemMemStackPopDoneSpecial() has been
 *                              called.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopBeginSpecial(PVMCPU pVCpu, size_t cbMem, void const **ppvMem, uint64_t *puNewRsp)
{
    Assert(cbMem < UINT8_MAX);
    PCPUMCTX    pCtx     = IEM_GET_CTX(pVCpu);
    RTGCPTR     GCPtrTop = iemRegGetRspForPop(pVCpu, pCtx, (uint8_t)cbMem, puNewRsp);
    return iemMemMap(pVCpu, (void **)ppvMem, cbMem, X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
}


/**
 * Continue a special stack pop (used by iret and retf).
 *
 * This will raise \#SS or \#PF if appropriate.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   cbMem               The number of bytes to pop from the stack.
 * @param   ppvMem              Where to return the pointer to the stack memory.
 * @param   puNewRsp            Where to return the new RSP value.  This must be
 *                              assigned to CPUMCTX::rsp manually some time
 *                              after iemMemStackPopDoneSpecial() has been
 *                              called.
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopContinueSpecial(PVMCPU pVCpu, size_t cbMem, void const **ppvMem, uint64_t *puNewRsp)
{
    Assert(cbMem < UINT8_MAX);
    PCPUMCTX    pCtx = IEM_GET_CTX(pVCpu);
    RTUINT64U   NewRsp;
    NewRsp.u = *puNewRsp;
    RTGCPTR     GCPtrTop = iemRegGetRspForPopEx(pVCpu, pCtx, &NewRsp, 8);
    *puNewRsp = NewRsp.u;
    return iemMemMap(pVCpu, (void **)ppvMem, cbMem, X86_SREG_SS, GCPtrTop, IEM_ACCESS_STACK_R);
}


/**
 * Done with a special stack pop (started by iemMemStackPopBeginSpecial or
 * iemMemStackPopContinueSpecial).
 *
 * The caller will manually commit the rSP.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pvMem               The pointer returned by
 *                              iemMemStackPopBeginSpecial() or
 *                              iemMemStackPopContinueSpecial().
 */
IEM_STATIC VBOXSTRICTRC iemMemStackPopDoneSpecial(PVMCPU pVCpu, void const *pvMem)
{
    return iemMemCommitAndUnmap(pVCpu, (void *)pvMem, IEM_ACCESS_STACK_R);
}


/**
 * Fetches a system table byte.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pbDst               Where to return the byte.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchSysU8(PVMCPU pVCpu, uint8_t *pbDst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint8_t const *pbSrc;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pbSrc, sizeof(*pbSrc), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R);
    if (rc == VINF_SUCCESS)
    {
        *pbDst = *pbSrc;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pbSrc, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a system table word.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu16Dst             Where to return the word.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchSysU16(PVMCPU pVCpu, uint16_t *pu16Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint16_t const *pu16Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu16Src, sizeof(*pu16Src), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R);
    if (rc == VINF_SUCCESS)
    {
        *pu16Dst = *pu16Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu16Src, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a system table dword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu32Dst             Where to return the dword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchSysU32(PVMCPU pVCpu, uint32_t *pu32Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint32_t const *pu32Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu32Src, sizeof(*pu32Src), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R);
    if (rc == VINF_SUCCESS)
    {
        *pu32Dst = *pu32Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu32Src, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a system table qword.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pu64Dst             Where to return the qword.
 * @param   iSegReg             The index of the segment register to use for
 *                              this access.  The base and limits are checked.
 * @param   GCPtrMem            The address of the guest memory.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchSysU64(PVMCPU pVCpu, uint64_t *pu64Dst, uint8_t iSegReg, RTGCPTR GCPtrMem)
{
    /* The lazy approach for now... */
    uint64_t const *pu64Src;
    VBOXSTRICTRC rc = iemMemMap(pVCpu, (void **)&pu64Src, sizeof(*pu64Src), iSegReg, GCPtrMem, IEM_ACCESS_SYS_R);
    if (rc == VINF_SUCCESS)
    {
        *pu64Dst = *pu64Src;
        rc = iemMemCommitAndUnmap(pVCpu, (void *)pu64Src, IEM_ACCESS_SYS_R);
    }
    return rc;
}


/**
 * Fetches a descriptor table entry with caller specified error code.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pDesc               Where to return the descriptor table entry.
 * @param   uSel                The selector which table entry to fetch.
 * @param   uXcpt               The exception to raise on table lookup error.
 * @param   uErrorCode          The error code associated with the exception.
 */
IEM_STATIC VBOXSTRICTRC
iemMemFetchSelDescWithErr(PVMCPU pVCpu, PIEMSELDESC pDesc, uint16_t uSel, uint8_t uXcpt, uint16_t uErrorCode)
{
    AssertPtr(pDesc);
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /** @todo did the 286 require all 8 bytes to be accessible? */
    /*
     * Get the selector table base and check bounds.
     */
    RTGCPTR GCPtrBase;
    if (uSel & X86_SEL_LDT)
    {
        if (   !pCtx->ldtr.Attr.n.u1Present
            || (uSel | X86_SEL_RPL_LDT) > pCtx->ldtr.u32Limit )
        {
            Log(("iemMemFetchSelDesc: LDT selector %#x is out of bounds (%3x) or ldtr is NP (%#x)\n",
                 uSel, pCtx->ldtr.u32Limit, pCtx->ldtr.Sel));
            return iemRaiseXcptOrInt(pVCpu, 0, uXcpt, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                                     uErrorCode, 0);
        }

        Assert(pCtx->ldtr.Attr.n.u1Present);
        GCPtrBase = pCtx->ldtr.u64Base;
    }
    else
    {
        if ((uSel | X86_SEL_RPL_LDT) > pCtx->gdtr.cbGdt)
        {
            Log(("iemMemFetchSelDesc: GDT selector %#x is out of bounds (%3x)\n", uSel, pCtx->gdtr.cbGdt));
            return iemRaiseXcptOrInt(pVCpu, 0, uXcpt, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR,
                                     uErrorCode, 0);
        }
        GCPtrBase = pCtx->gdtr.pGdt;
    }

    /*
     * Read the legacy descriptor and maybe the long mode extensions if
     * required.
     */
    VBOXSTRICTRC rcStrict;
    if (IEM_GET_TARGET_CPU(pVCpu) > IEMTARGETCPU_286)
        rcStrict = iemMemFetchSysU64(pVCpu, &pDesc->Legacy.u, UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK));
    else
    {
        rcStrict     = iemMemFetchSysU16(pVCpu, &pDesc->Legacy.au16[0], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 0);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemFetchSysU16(pVCpu, &pDesc->Legacy.au16[1], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 2);
        if (rcStrict == VINF_SUCCESS)
            rcStrict = iemMemFetchSysU16(pVCpu, &pDesc->Legacy.au16[2], UINT8_MAX, GCPtrBase + (uSel & X86_SEL_MASK) + 4);
        if (rcStrict == VINF_SUCCESS)
            pDesc->Legacy.au16[3] = 0;
        else
            return rcStrict;
    }

    if (rcStrict == VINF_SUCCESS)
    {
        if (   !IEM_IS_LONG_MODE(pVCpu)
            || pDesc->Legacy.Gen.u1DescType)
            pDesc->Long.au64[1] = 0;
        else if ((uint32_t)(uSel | X86_SEL_RPL_LDT) + 8 <= (uSel & X86_SEL_LDT ? pCtx->ldtr.u32Limit : pCtx->gdtr.cbGdt))
            rcStrict = iemMemFetchSysU64(pVCpu, &pDesc->Long.au64[1], UINT8_MAX, GCPtrBase + (uSel | X86_SEL_RPL_LDT) + 1);
        else
        {
            Log(("iemMemFetchSelDesc: system selector %#x is out of bounds\n", uSel));
            /** @todo is this the right exception? */
            return iemRaiseXcptOrInt(pVCpu, 0, uXcpt, IEM_XCPT_FLAGS_T_CPU_XCPT | IEM_XCPT_FLAGS_ERR, uErrorCode, 0);
        }
    }
    return rcStrict;
}


/**
 * Fetches a descriptor table entry.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   pDesc               Where to return the descriptor table entry.
 * @param   uSel                The selector which table entry to fetch.
 * @param   uXcpt               The exception to raise on table lookup error.
 */
IEM_STATIC VBOXSTRICTRC iemMemFetchSelDesc(PVMCPU pVCpu, PIEMSELDESC pDesc, uint16_t uSel, uint8_t uXcpt)
{
    return iemMemFetchSelDescWithErr(pVCpu, pDesc, uSel, uXcpt, uSel & X86_SEL_MASK_OFF_RPL);
}


/**
 * Fakes a long mode stack selector for SS = 0.
 *
 * @param   pDescSs             Where to return the fake stack descriptor.
 * @param   uDpl                The DPL we want.
 */
IEM_STATIC void iemMemFakeStackSelDesc(PIEMSELDESC pDescSs, uint32_t uDpl)
{
    pDescSs->Long.au64[0] = 0;
    pDescSs->Long.au64[1] = 0;
    pDescSs->Long.Gen.u4Type     = X86_SEL_TYPE_RW_ACC;
    pDescSs->Long.Gen.u1DescType = 1; /* 1 = code / data, 0 = system. */
    pDescSs->Long.Gen.u2Dpl      = uDpl;
    pDescSs->Long.Gen.u1Present  = 1;
    pDescSs->Long.Gen.u1Long     = 1;
}


/**
 * Marks the selector descriptor as accessed (only non-system descriptors).
 *
 * This function ASSUMES that iemMemFetchSelDesc has be called previously and
 * will therefore skip the limit checks.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   uSel                The selector.
 */
IEM_STATIC VBOXSTRICTRC iemMemMarkSelDescAccessed(PVMCPU pVCpu, uint16_t uSel)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    /*
     * Get the selector table base and calculate the entry address.
     */
    RTGCPTR GCPtr = uSel & X86_SEL_LDT
                  ? pCtx->ldtr.u64Base
                  : pCtx->gdtr.pGdt;
    GCPtr += uSel & X86_SEL_MASK;

    /*
     * ASMAtomicBitSet will assert if the address is misaligned, so do some
     * ugly stuff to avoid this.  This will make sure it's an atomic access
     * as well more or less remove any question about 8-bit or 32-bit accesss.
     */
    VBOXSTRICTRC        rcStrict;
    uint32_t volatile  *pu32;
    if ((GCPtr & 3) == 0)
    {
        /* The normal case, map the 32-bit bits around the accessed bit (40). */
        GCPtr += 2 + 2;
        rcStrict = iemMemMap(pVCpu, (void **)&pu32, 4, UINT8_MAX, GCPtr, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        ASMAtomicBitSet(pu32, 8); /* X86_SEL_TYPE_ACCESSED is 1, but it is preceeded by u8BaseHigh1. */
    }
    else
    {
        /* The misaligned GDT/LDT case, map the whole thing. */
        rcStrict = iemMemMap(pVCpu, (void **)&pu32, 8, UINT8_MAX, GCPtr, IEM_ACCESS_SYS_RW);
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
        switch ((uintptr_t)pu32 & 3)
        {
            case 0: ASMAtomicBitSet(pu32,                         40 + 0 -  0); break;
            case 1: ASMAtomicBitSet((uint8_t volatile *)pu32 + 3, 40 + 0 - 24); break;
            case 2: ASMAtomicBitSet((uint8_t volatile *)pu32 + 2, 40 + 0 - 16); break;
            case 3: ASMAtomicBitSet((uint8_t volatile *)pu32 + 1, 40 + 0 -  8); break;
        }
    }

    return iemMemCommitAndUnmap(pVCpu, (void *)pu32, IEM_ACCESS_SYS_RW);
}

/** @} */


/*
 * Include the C/C++ implementation of instruction.
 */
#include "IEMAllCImpl.cpp.h"



/** @name   "Microcode" macros.
 *
 * The idea is that we should be able to use the same code to interpret
 * instructions as well as recompiler instructions.  Thus this obfuscation.
 *
 * @{
 */
#define IEM_MC_BEGIN(a_cArgs, a_cLocals)                {
#define IEM_MC_END()                                    }
#define IEM_MC_PAUSE()                                  do {} while (0)
#define IEM_MC_CONTINUE()                               do {} while (0)

/** Internal macro. */
#define IEM_MC_RETURN_ON_FAILURE(a_Expr) \
    do \
    { \
        VBOXSTRICTRC rcStrict2 = a_Expr; \
        if (rcStrict2 != VINF_SUCCESS) \
            return rcStrict2; \
    } while (0)


#define IEM_MC_ADVANCE_RIP()                            iemRegUpdateRipAndClearRF(pVCpu)
#define IEM_MC_REL_JMP_S8(a_i8)                         IEM_MC_RETURN_ON_FAILURE(iemRegRipRelativeJumpS8(pVCpu, a_i8))
#define IEM_MC_REL_JMP_S16(a_i16)                       IEM_MC_RETURN_ON_FAILURE(iemRegRipRelativeJumpS16(pVCpu, a_i16))
#define IEM_MC_REL_JMP_S32(a_i32)                       IEM_MC_RETURN_ON_FAILURE(iemRegRipRelativeJumpS32(pVCpu, a_i32))
#define IEM_MC_SET_RIP_U16(a_u16NewIP)                  IEM_MC_RETURN_ON_FAILURE(iemRegRipJump((pVCpu), (a_u16NewIP)))
#define IEM_MC_SET_RIP_U32(a_u32NewIP)                  IEM_MC_RETURN_ON_FAILURE(iemRegRipJump((pVCpu), (a_u32NewIP)))
#define IEM_MC_SET_RIP_U64(a_u64NewIP)                  IEM_MC_RETURN_ON_FAILURE(iemRegRipJump((pVCpu), (a_u64NewIP)))
#define IEM_MC_RAISE_DIVIDE_ERROR()                     return iemRaiseDivideError(pVCpu)
#define IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE()       \
    do { \
        if ((pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0 & (X86_CR0_EM | X86_CR0_TS)) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_WAIT_DEVICE_NOT_AVAILABLE()  \
    do { \
        if (((pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0 & (X86_CR0_MP | X86_CR0_TS)) == (X86_CR0_MP | X86_CR0_TS)) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_FPU_XCPT() \
    do { \
        if ((pVCpu)->iem.s.CTX_SUFF(pCtx)->CTX_SUFF(pXState)->x87.FSW & X86_FSW_ES) \
            return iemRaiseMathFault(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_AVX2_RELATED_XCPT() \
    do { \
        if (   (IEM_GET_CTX(pVCpu)->aXcr[0] & (XSAVE_C_YMM | XSAVE_C_SSE)) != (XSAVE_C_YMM | XSAVE_C_SSE) \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_OSXSAVE) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx2) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_AVX_RELATED_XCPT() \
    do { \
        if (   (IEM_GET_CTX(pVCpu)->aXcr[0] & (XSAVE_C_YMM | XSAVE_C_SSE)) != (XSAVE_C_YMM | XSAVE_C_SSE) \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_OSXSAVE) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAvx) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE41_RELATED_XCPT() \
    do { \
        if (   (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_EM) \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse41) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE3_RELATED_XCPT() \
    do { \
        if (   (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_EM) \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse3) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE2_RELATED_XCPT() \
    do { \
        if (   (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_EM) \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse2) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_SSE_RELATED_XCPT() \
    do { \
        if (   (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_EM) \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_OSFXSR) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT() \
    do { \
        if (   ((pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0 & X86_CR0_EM) \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fMmx) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT_CHECK_SSE_OR_MMXEXT() \
    do { \
        if (   ((pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0 & X86_CR0_EM) \
            || (   !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fSse \
                && !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fAmdMmxExts) ) \
            return iemRaiseUndefinedOpcode(pVCpu); \
        if (IEM_GET_CTX(pVCpu)->cr0 & X86_CR0_TS) \
            return iemRaiseDeviceNotAvailable(pVCpu); \
    } while (0)
#define IEM_MC_RAISE_GP0_IF_CPL_NOT_ZERO() \
    do { \
        if (pVCpu->iem.s.uCpl != 0) \
            return iemRaiseGeneralProtectionFault0(pVCpu); \
    } while (0)
#define IEM_MC_RAISE_GP0_IF_EFF_ADDR_UNALIGNED(a_EffAddr, a_cbAlign) \
    do { \
        if (!((a_EffAddr) & ((a_cbAlign) - 1))) { /* likely */ } \
        else return iemRaiseGeneralProtectionFault0(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_FSGSBASE_XCPT() \
    do { \
        if (   pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT \
            || !IEM_GET_GUEST_CPU_FEATURES(pVCpu)->fFsGsBase \
            || !(IEM_GET_CTX(pVCpu)->cr4 & X86_CR4_FSGSBASE)) \
            return iemRaiseUndefinedOpcode(pVCpu); \
    } while (0)
#define IEM_MC_MAYBE_RAISE_NON_CANONICAL_ADDR_GP0(a_u64Addr) \
    do { \
        if (!IEM_IS_CANONICAL(a_u64Addr)) \
            return iemRaiseGeneralProtectionFault0(pVCpu); \
    } while (0)


#define IEM_MC_LOCAL(a_Type, a_Name)                    a_Type a_Name
#define IEM_MC_LOCAL_CONST(a_Type, a_Name, a_Value)     a_Type const a_Name = (a_Value)
#define IEM_MC_REF_LOCAL(a_pRefArg, a_Local)            (a_pRefArg) = &(a_Local)
#define IEM_MC_ARG(a_Type, a_Name, a_iArg)              a_Type a_Name
#define IEM_MC_ARG_CONST(a_Type, a_Name, a_Value, a_iArg)       a_Type const a_Name = (a_Value)
#define IEM_MC_ARG_LOCAL_REF(a_Type, a_Name, a_Local, a_iArg)   a_Type const a_Name = &(a_Local)
#define IEM_MC_ARG_LOCAL_EFLAGS(a_pName, a_Name, a_iArg) \
    uint32_t a_Name; \
    uint32_t *a_pName = &a_Name
#define IEM_MC_COMMIT_EFLAGS(a_EFlags) \
   do { (pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u = (a_EFlags); Assert((pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u & X86_EFL_1); } while (0)

#define IEM_MC_ASSIGN(a_VarOrArg, a_CVariableOrConst)   (a_VarOrArg) = (a_CVariableOrConst)
#define IEM_MC_ASSIGN_TO_SMALLER                        IEM_MC_ASSIGN

#define IEM_MC_FETCH_GREG_U8(a_u8Dst, a_iGReg)          (a_u8Dst)  = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_ZX_U16(a_u16Dst, a_iGReg)  (a_u16Dst) = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_ZX_U32(a_u32Dst, a_iGReg)  (a_u32Dst) = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_ZX_U64(a_u64Dst, a_iGReg)  (a_u64Dst) = iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_SX_U16(a_u16Dst, a_iGReg)  (a_u16Dst) = (int8_t)iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_SX_U32(a_u32Dst, a_iGReg)  (a_u32Dst) = (int8_t)iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U8_SX_U64(a_u64Dst, a_iGReg)  (a_u64Dst) = (int8_t)iemGRegFetchU8(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16(a_u16Dst, a_iGReg)        (a_u16Dst) = iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_ZX_U32(a_u32Dst, a_iGReg) (a_u32Dst) = iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_ZX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_SX_U32(a_u32Dst, a_iGReg) (a_u32Dst) = (int16_t)iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U16_SX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = (int16_t)iemGRegFetchU16(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U32(a_u32Dst, a_iGReg)        (a_u32Dst) = iemGRegFetchU32(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U32_ZX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = iemGRegFetchU32(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U32_SX_U64(a_u64Dst, a_iGReg) (a_u64Dst) = (int32_t)iemGRegFetchU32(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U64(a_u64Dst, a_iGReg)        (a_u64Dst) = iemGRegFetchU64(pVCpu, (a_iGReg))
#define IEM_MC_FETCH_GREG_U64_ZX_U64                    IEM_MC_FETCH_GREG_U64
#define IEM_MC_FETCH_SREG_U16(a_u16Dst, a_iSReg)        (a_u16Dst) = iemSRegFetchU16(pVCpu, (a_iSReg))
#define IEM_MC_FETCH_SREG_ZX_U32(a_u32Dst, a_iSReg)     (a_u32Dst) = iemSRegFetchU16(pVCpu, (a_iSReg))
#define IEM_MC_FETCH_SREG_ZX_U64(a_u64Dst, a_iSReg)     (a_u64Dst) = iemSRegFetchU16(pVCpu, (a_iSReg))
#define IEM_MC_FETCH_SREG_BASE_U64(a_u64Dst, a_iSReg)   (a_u64Dst) = iemSRegBaseFetchU64(pVCpu, (a_iSReg));
#define IEM_MC_FETCH_SREG_BASE_U32(a_u32Dst, a_iSReg)   (a_u32Dst) = iemSRegBaseFetchU64(pVCpu, (a_iSReg));
#define IEM_MC_FETCH_CR0_U16(a_u16Dst)                  (a_u16Dst) = (uint16_t)(pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0
#define IEM_MC_FETCH_CR0_U32(a_u32Dst)                  (a_u32Dst) = (uint32_t)(pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0
#define IEM_MC_FETCH_CR0_U64(a_u64Dst)                  (a_u64Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->cr0
#define IEM_MC_FETCH_LDTR_U16(a_u16Dst)                 (a_u16Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->ldtr.Sel
#define IEM_MC_FETCH_LDTR_U32(a_u32Dst)                 (a_u32Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->ldtr.Sel
#define IEM_MC_FETCH_LDTR_U64(a_u64Dst)                 (a_u64Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->ldtr.Sel
#define IEM_MC_FETCH_TR_U16(a_u16Dst)                   (a_u16Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->tr.Sel
#define IEM_MC_FETCH_TR_U32(a_u32Dst)                   (a_u32Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->tr.Sel
#define IEM_MC_FETCH_TR_U64(a_u64Dst)                   (a_u64Dst) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->tr.Sel
/** @note Not for IOPL or IF testing or modification. */
#define IEM_MC_FETCH_EFLAGS(a_EFlags)                   (a_EFlags) = (pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u
#define IEM_MC_FETCH_EFLAGS_U8(a_EFlags)                (a_EFlags) = (uint8_t)(pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u
#define IEM_MC_FETCH_FSW(a_u16Fsw)                      (a_u16Fsw) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FSW
#define IEM_MC_FETCH_FCW(a_u16Fcw)                      (a_u16Fcw) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FCW

#define IEM_MC_STORE_GREG_U8(a_iGReg, a_u8Value)        *iemGRegRefU8( pVCpu, (a_iGReg)) = (a_u8Value)
#define IEM_MC_STORE_GREG_U16(a_iGReg, a_u16Value)      *iemGRegRefU16(pVCpu, (a_iGReg)) = (a_u16Value)
#define IEM_MC_STORE_GREG_U32(a_iGReg, a_u32Value)      *iemGRegRefU64(pVCpu, (a_iGReg)) = (uint32_t)(a_u32Value) /* clear high bits. */
#define IEM_MC_STORE_GREG_U64(a_iGReg, a_u64Value)      *iemGRegRefU64(pVCpu, (a_iGReg)) = (a_u64Value)
#define IEM_MC_STORE_GREG_U8_CONST                      IEM_MC_STORE_GREG_U8
#define IEM_MC_STORE_GREG_U16_CONST                     IEM_MC_STORE_GREG_U16
#define IEM_MC_STORE_GREG_U32_CONST                     IEM_MC_STORE_GREG_U32
#define IEM_MC_STORE_GREG_U64_CONST                     IEM_MC_STORE_GREG_U64
#define IEM_MC_CLEAR_HIGH_GREG_U64(a_iGReg)             *iemGRegRefU64(pVCpu, (a_iGReg)) &= UINT32_MAX
#define IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(a_pu32Dst)    do { (a_pu32Dst)[1] = 0; } while (0)
#define IEM_MC_STORE_SREG_BASE_U64(a_iSeg, a_u64Value)  *iemSRegBaseRefU64(pVCpu, (a_iSeg)) = (a_u64Value)
#define IEM_MC_STORE_SREG_BASE_U32(a_iSeg, a_u32Value)  *iemSRegBaseRefU64(pVCpu, (a_iSeg)) = (uint32_t)(a_u32Value) /* clear high bits. */
#define IEM_MC_STORE_FPUREG_R80_SRC_REF(a_iSt, a_pr80Src) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[a_iSt].r80 = *(a_pr80Src); } while (0)


#define IEM_MC_REF_GREG_U8(a_pu8Dst, a_iGReg)           (a_pu8Dst)  = iemGRegRefU8( pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_U16(a_pu16Dst, a_iGReg)         (a_pu16Dst) = iemGRegRefU16(pVCpu, (a_iGReg))
/** @todo User of IEM_MC_REF_GREG_U32 needs to clear the high bits on commit.
 *        Use IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF! */
#define IEM_MC_REF_GREG_U32(a_pu32Dst, a_iGReg)         (a_pu32Dst) = iemGRegRefU32(pVCpu, (a_iGReg))
#define IEM_MC_REF_GREG_U64(a_pu64Dst, a_iGReg)         (a_pu64Dst) = iemGRegRefU64(pVCpu, (a_iGReg))
/** @note Not for IOPL or IF testing or modification. */
#define IEM_MC_REF_EFLAGS(a_pEFlags)                    (a_pEFlags) = &(pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u

#define IEM_MC_ADD_GREG_U8(a_iGReg, a_u8Value)          *iemGRegRefU8( pVCpu, (a_iGReg)) += (a_u8Value)
#define IEM_MC_ADD_GREG_U16(a_iGReg, a_u16Value)        *iemGRegRefU16(pVCpu, (a_iGReg)) += (a_u16Value)
#define IEM_MC_ADD_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg += (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_ADD_GREG_U64(a_iGReg, a_u64Value)        *iemGRegRefU64(pVCpu, (a_iGReg)) += (a_u64Value)

#define IEM_MC_SUB_GREG_U8(a_iGReg,  a_u8Value)         *iemGRegRefU8( pVCpu, (a_iGReg)) -= (a_u8Value)
#define IEM_MC_SUB_GREG_U16(a_iGReg, a_u16Value)        *iemGRegRefU16(pVCpu, (a_iGReg)) -= (a_u16Value)
#define IEM_MC_SUB_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg -= (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_SUB_GREG_U64(a_iGReg, a_u64Value)        *iemGRegRefU64(pVCpu, (a_iGReg)) -= (a_u64Value)
#define IEM_MC_SUB_LOCAL_U16(a_u16Value, a_u16Const)   do { (a_u16Value) -= a_u16Const; } while (0)

#define IEM_MC_ADD_GREG_U8_TO_LOCAL(a_u8Value, a_iGReg)    do { (a_u8Value)  += iemGRegFetchU8( pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_GREG_U16_TO_LOCAL(a_u16Value, a_iGReg)  do { (a_u16Value) += iemGRegFetchU16(pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_GREG_U32_TO_LOCAL(a_u32Value, a_iGReg)  do { (a_u32Value) += iemGRegFetchU32(pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_GREG_U64_TO_LOCAL(a_u64Value, a_iGReg)  do { (a_u64Value) += iemGRegFetchU64(pVCpu, (a_iGReg)); } while (0)
#define IEM_MC_ADD_LOCAL_S16_TO_EFF_ADDR(a_EffAddr, a_i16) do { (a_EffAddr) += (a_i16); } while (0)
#define IEM_MC_ADD_LOCAL_S32_TO_EFF_ADDR(a_EffAddr, a_i32) do { (a_EffAddr) += (a_i32); } while (0)
#define IEM_MC_ADD_LOCAL_S64_TO_EFF_ADDR(a_EffAddr, a_i64) do { (a_EffAddr) += (a_i64); } while (0)

#define IEM_MC_AND_LOCAL_U8(a_u8Local, a_u8Mask)        do { (a_u8Local)  &= (a_u8Mask);  } while (0)
#define IEM_MC_AND_LOCAL_U16(a_u16Local, a_u16Mask)     do { (a_u16Local) &= (a_u16Mask); } while (0)
#define IEM_MC_AND_LOCAL_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); } while (0)
#define IEM_MC_AND_LOCAL_U64(a_u64Local, a_u64Mask)     do { (a_u64Local) &= (a_u64Mask); } while (0)

#define IEM_MC_AND_ARG_U16(a_u16Arg, a_u16Mask)         do { (a_u16Arg) &= (a_u16Mask); } while (0)
#define IEM_MC_AND_ARG_U32(a_u32Arg, a_u32Mask)         do { (a_u32Arg) &= (a_u32Mask); } while (0)
#define IEM_MC_AND_ARG_U64(a_u64Arg, a_u64Mask)         do { (a_u64Arg) &= (a_u64Mask); } while (0)

#define IEM_MC_OR_LOCAL_U8(a_u8Local, a_u8Mask)         do { (a_u8Local)  |= (a_u8Mask);  } while (0)
#define IEM_MC_OR_LOCAL_U16(a_u16Local, a_u16Mask)      do { (a_u16Local) |= (a_u16Mask); } while (0)
#define IEM_MC_OR_LOCAL_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); } while (0)

#define IEM_MC_SAR_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) >>= (a_cShift);  } while (0)
#define IEM_MC_SAR_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) >>= (a_cShift);  } while (0)
#define IEM_MC_SAR_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) >>= (a_cShift);  } while (0)

#define IEM_MC_SHL_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) <<= (a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) <<= (a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) <<= (a_cShift);  } while (0)

#define IEM_MC_AND_2LOCS_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); } while (0)

#define IEM_MC_OR_2LOCS_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); } while (0)

#define IEM_MC_AND_GREG_U8(a_iGReg, a_u8Value)          *iemGRegRefU8( pVCpu, (a_iGReg)) &= (a_u8Value)
#define IEM_MC_AND_GREG_U16(a_iGReg, a_u16Value)        *iemGRegRefU16(pVCpu, (a_iGReg)) &= (a_u16Value)
#define IEM_MC_AND_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg &= (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_AND_GREG_U64(a_iGReg, a_u64Value)        *iemGRegRefU64(pVCpu, (a_iGReg)) &= (a_u64Value)

#define IEM_MC_OR_GREG_U8(a_iGReg, a_u8Value)           *iemGRegRefU8( pVCpu, (a_iGReg)) |= (a_u8Value)
#define IEM_MC_OR_GREG_U16(a_iGReg, a_u16Value)         *iemGRegRefU16(pVCpu, (a_iGReg)) |= (a_u16Value)
#define IEM_MC_OR_GREG_U32(a_iGReg, a_u32Value) \
    do { \
        uint32_t *pu32Reg = iemGRegRefU32(pVCpu, (a_iGReg)); \
        *pu32Reg |= (a_u32Value); \
        pu32Reg[1] = 0; /* implicitly clear the high bit. */ \
    } while (0)
#define IEM_MC_OR_GREG_U64(a_iGReg, a_u64Value)         *iemGRegRefU64(pVCpu, (a_iGReg)) |= (a_u64Value)


/** @note Not for IOPL or IF modification. */
#define IEM_MC_SET_EFL_BIT(a_fBit)                      do { (pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u |= (a_fBit); } while (0)
/** @note Not for IOPL or IF modification. */
#define IEM_MC_CLEAR_EFL_BIT(a_fBit)                    do { (pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u &= ~(a_fBit); } while (0)
/** @note Not for IOPL or IF modification. */
#define IEM_MC_FLIP_EFL_BIT(a_fBit)                     do { (pVCpu)->iem.s.CTX_SUFF(pCtx)->eflags.u ^= (a_fBit); } while (0)

#define IEM_MC_CLEAR_FSW_EX()   do { (pVCpu)->iem.s.CTX_SUFF(pCtx)->CTX_SUFF(pXState)->x87.FSW &= X86_FSW_C_MASK | X86_FSW_TOP_MASK; } while (0)

/** Switches the FPU state to MMX mode (FSW.TOS=0, FTW=0) if necessary. */
#define IEM_MC_FPU_TO_MMX_MODE() do { \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FSW &= ~X86_FSW_TOP_MASK; \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FTW  = 0xff; \
    } while (0)

/** Switches the FPU state from MMX mode (FTW=0xffff). */
#define IEM_MC_FPU_FROM_MMX_MODE() do { \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FTW  = 0; \
    } while (0)

#define IEM_MC_FETCH_MREG_U64(a_u64Value, a_iMReg) \
    do { (a_u64Value) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].mmx; } while (0)
#define IEM_MC_FETCH_MREG_U32(a_u32Value, a_iMReg) \
    do { (a_u32Value) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].au32[0]; } while (0)
#define IEM_MC_STORE_MREG_U64(a_iMReg, a_u64Value) do { \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].mmx = (a_u64Value); \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].au32[2] = 0xffff; \
    } while (0)
#define IEM_MC_STORE_MREG_U32_ZX_U64(a_iMReg, a_u32Value) do { \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].mmx = (uint32_t)(a_u32Value); \
        IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].au32[2] = 0xffff; \
    } while (0)
#define IEM_MC_REF_MREG_U64(a_pu64Dst, a_iMReg) /** @todo need to set high word to 0xffff on commit (see IEM_MC_STORE_MREG_U64) */ \
        (a_pu64Dst) = (&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].mmx)
#define IEM_MC_REF_MREG_U64_CONST(a_pu64Dst, a_iMReg) \
        (a_pu64Dst) = ((uint64_t const *)&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].mmx)
#define IEM_MC_REF_MREG_U32_CONST(a_pu32Dst, a_iMReg) \
        (a_pu32Dst) = ((uint32_t const *)&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aRegs[(a_iMReg)].mmx)

#define IEM_MC_FETCH_XREG_U128(a_u128Value, a_iXReg) \
    do { (a_u128Value).au64[0] = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0]; \
         (a_u128Value).au64[1] = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[1]; \
    } while (0)
#define IEM_MC_FETCH_XREG_U64(a_u64Value, a_iXReg) \
    do { (a_u64Value) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0]; } while (0)
#define IEM_MC_FETCH_XREG_U32(a_u32Value, a_iXReg) \
    do { (a_u32Value) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au32[0]; } while (0)
#define IEM_MC_FETCH_XREG_HI_U64(a_u64Value, a_iXReg) \
    do { (a_u64Value) = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[1]; } while (0)
#define IEM_MC_STORE_XREG_U128(a_iXReg, a_u128Value) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0] = (a_u128Value).au64[0]; \
         IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[1] = (a_u128Value).au64[1]; \
    } while (0)
#define IEM_MC_STORE_XREG_U64(a_iXReg, a_u64Value) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0] = (a_u64Value); } while (0)
#define IEM_MC_STORE_XREG_U64_ZX_U128(a_iXReg, a_u64Value) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0] = (a_u64Value); \
         IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[1] = 0; \
    } while (0)
#define IEM_MC_STORE_XREG_U32(a_iXReg, a_u32Value) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au32[0] = (a_u32Value); } while (0)
#define IEM_MC_STORE_XREG_U32_ZX_U128(a_iXReg, a_u32Value) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0] = (uint32_t)(a_u32Value); \
         IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[1] = 0; \
    } while (0)
#define IEM_MC_STORE_XREG_HI_U64(a_iXReg, a_u64Value) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[1] = (a_u64Value); } while (0)
#define IEM_MC_REF_XREG_U128(a_pu128Dst, a_iXReg)       \
    (a_pu128Dst) = (&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].uXmm)
#define IEM_MC_REF_XREG_U128_CONST(a_pu128Dst, a_iXReg) \
    (a_pu128Dst) = ((PCRTUINT128U)&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].uXmm)
#define IEM_MC_REF_XREG_U64_CONST(a_pu64Dst, a_iXReg) \
    (a_pu64Dst) = ((uint64_t const *)&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXReg)].au64[0])
#define IEM_MC_COPY_XREG_U128(a_iXRegDst, a_iXRegSrc) \
    do { IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXRegDst)].au64[0] \
            = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXRegSrc)].au64[0]; \
         IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXRegDst)].au64[1] \
            = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aXMM[(a_iXRegSrc)].au64[1]; \
    } while (0)

#define IEM_MC_FETCH_YREG_U32(a_u32Dst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u32Dst) = pXStateTmp->x87.aXMM[iYRegSrcTmp].au32[0]; \
    } while (0)
#define IEM_MC_FETCH_YREG_U64(a_u64Dst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u64Dst) = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[0]; \
    } while (0)
#define IEM_MC_FETCH_YREG_U128(a_u128Dst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u128Dst).au64[0] = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[0]; \
         (a_u128Dst).au64[1] = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[1]; \
    } while (0)
#define IEM_MC_FETCH_YREG_U256(a_u256Dst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         (a_u256Dst).au64[0] = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[0]; \
         (a_u256Dst).au64[1] = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[1]; \
         (a_u256Dst).au64[2] = pXStateTmp->u.YmmHi.aYmmHi[iYRegSrcTmp].au64[0]; \
         (a_u256Dst).au64[3] = pXStateTmp->u.YmmHi.aYmmHi[iYRegSrcTmp].au64[1]; \
    } while (0)

#define IEM_MC_INT_CLEAR_ZMM_256_UP(a_pXState, a_iXRegDst) do { /* For AVX512 and AVX1024 support. */ } while (0)
#define IEM_MC_STORE_YREG_U32_ZX_VLMAX(a_iYRegDst, a_u32Src) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au32[0]       = (a_u32Src); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au32[1]       = 0; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_STORE_YREG_U64_ZX_VLMAX(a_iYRegDst, a_u64Src) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = (a_u64Src); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_STORE_YREG_U128_ZX_VLMAX(a_iYRegDst, a_u128Src) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = (a_u128Src).au64[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = (a_u128Src).au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_STORE_YREG_U256_ZX_VLMAX(a_iYRegDst, a_u256Src) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = (a_u256Src).au64[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = (a_u256Src).au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = (a_u256Src).au64[2]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = (a_u256Src).au64[3]; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)

#define IEM_MC_REF_YREG_U128(a_pu128Dst, a_iYReg)       \
    (a_pu128Dst) = (&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aYMM[(a_iYReg)].uXmm)
#define IEM_MC_REF_YREG_U128_CONST(a_pu128Dst, a_iYReg) \
    (a_pu128Dst) = ((PCRTUINT128U)&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aYMM[(a_iYReg)].uXmm)
#define IEM_MC_REF_YREG_U64_CONST(a_pu64Dst, a_iYReg) \
    (a_pu64Dst) = ((uint64_t const *)&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.aYMM[(a_iYReg)].au64[0])
#define IEM_MC_CLEAR_YREG_128_UP(a_iYReg) \
    do { PX86XSAVEAREA   pXStateTmp = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegTmp   = (a_iYReg); \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegTmp); \
    } while (0)

#define IEM_MC_COPY_YREG_U256_ZX_VLMAX(a_iYRegDst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = pXStateTmp->u.YmmHi.aYmmHi[iYRegSrcTmp].au64[0]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = pXStateTmp->u.YmmHi.aYmmHi[iYRegSrcTmp].au64[1]; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_COPY_YREG_U128_ZX_VLMAX(a_iYRegDst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_COPY_YREG_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcTmp    = (a_iYRegSrc); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = pXStateTmp->x87.aXMM[iYRegSrcTmp].au64[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)

#define IEM_MC_MERGE_YREG_U32_U96_ZX_VLMAX(a_iYRegDst, a_iYRegSrc32, a_iYRegSrcHx) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc32Tmp  = (a_iYRegSrc32); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au32[0]       = pXStateTmp->x87.aXMM[iYRegSrc32Tmp].au32[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au32[1]       = pXStateTmp->x87.aXMM[iYRegSrcHxTmp].au32[1]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = pXStateTmp->x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc64Tmp  = (a_iYRegSrc64); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = pXStateTmp->x87.aXMM[iYRegSrc64Tmp].au64[0]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = pXStateTmp->x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64HI_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx) /* for vmovhlps */ \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrc64Tmp  = (a_iYRegSrc64); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = pXStateTmp->x87.aXMM[iYRegSrc64Tmp].au64[1]; \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = pXStateTmp->x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)
#define IEM_MC_MERGE_YREG_U64LOCAL_U64_ZX_VLMAX(a_iYRegDst, a_u64Local, a_iYRegSrcHx) \
    do { PX86XSAVEAREA   pXStateTmp     = IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState); \
         uintptr_t const iYRegDstTmp    = (a_iYRegDst); \
         uintptr_t const iYRegSrcHxTmp  = (a_iYRegSrcHx); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[0]       = (a_u64Local); \
         pXStateTmp->x87.aXMM[iYRegDstTmp].au64[1]       = pXStateTmp->x87.aXMM[iYRegSrcHxTmp].au64[1]; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[0] = 0; \
         pXStateTmp->u.YmmHi.aYmmHi[iYRegDstTmp].au64[1] = 0; \
         IEM_MC_INT_CLEAR_ZMM_256_UP(pXStateTmp, iYRegDstTmp); \
    } while (0)

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U8(a_u8Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &(a_u8Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM16_U8(a_u8Dst, a_iSeg, a_GCPtrMem16) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &(a_u8Dst), (a_iSeg), (a_GCPtrMem16)))
# define IEM_MC_FETCH_MEM32_U8(a_u8Dst, a_iSeg, a_GCPtrMem32) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &(a_u8Dst), (a_iSeg), (a_GCPtrMem32)))
#else
# define IEM_MC_FETCH_MEM_U8(a_u8Dst, a_iSeg, a_GCPtrMem) \
    ((a_u8Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM16_U8(a_u8Dst, a_iSeg, a_GCPtrMem16) \
    ((a_u8Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem16)))
# define IEM_MC_FETCH_MEM32_U8(a_u8Dst, a_iSeg, a_GCPtrMem32) \
    ((a_u8Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem32)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &(a_u16Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_DISP(a_u16Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &(a_u16Dst), (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I16(a_i16Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, (uint16_t *)&(a_i16Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    ((a_u16Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_DISP(a_u16Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    ((a_u16Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I16(a_i16Dst, a_iSeg, a_GCPtrMem) \
    ((a_i16Dst) = (int16_t)iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_u32Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_DISP(a_u32Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_u32Dst), (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I32(a_i32Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, (uint32_t *)&(a_i32Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_DISP(a_u32Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    ((a_u32Dst) = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_I32(a_i32Dst, a_iSeg, a_GCPtrMem) \
    ((a_i32Dst) = (int32_t)iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif

#ifdef SOME_UNUSED_FUNCTION
# define IEM_MC_FETCH_MEM_S32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataS32SxU64(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U64_DISP(a_u64Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_U64_ALIGN_U128(a_u64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64AlignedU128(pVCpu, &(a_u64Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_I64(a_i64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, (uint64_t *)&(a_i64Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U64_DISP(a_u64Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    ((a_u64Dst) = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem) + (a_offDisp)))
# define IEM_MC_FETCH_MEM_U64_ALIGN_U128(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU64AlignedU128Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_I64(a_i64Dst, a_iSeg, a_GCPtrMem) \
    ((a_i64Dst) = (int64_t)iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_R32(a_r32Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &(a_r32Dst).u32, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R64(a_r64Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU64(pVCpu, &(a_r64Dst).au64[0], (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R80(a_r80Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataR80(pVCpu, &(a_r80Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_R32(a_r32Dst, a_iSeg, a_GCPtrMem) \
    ((a_r32Dst).u32 = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R64(a_r64Dst, a_iSeg, a_GCPtrMem) \
    ((a_r64Dst).au64[0] = iemMemFetchDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_R80(a_r80Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataR80Jmp(pVCpu, &(a_r80Dst), (a_iSeg), (a_GCPtrMem))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U128(a_u128Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U128_ALIGN_SSE(a_u128Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU128AlignedSse(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U128(a_u128Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128Jmp(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_U128_ALIGN_SSE(a_u128Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU128AlignedSseJmp(pVCpu, &(a_u128Dst), (a_iSeg), (a_GCPtrMem))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U256(a_u256Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U256_ALIGN_AVX(a_u256Dst, a_iSeg, a_GCPtrMem) \
    IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU256AlignedSse(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem)))
#else
# define IEM_MC_FETCH_MEM_U256(a_u256Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256Jmp(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem))
# define IEM_MC_FETCH_MEM_U256_ALIGN_AVX(a_u256Dst, a_iSeg, a_GCPtrMem) \
    iemMemFetchDataU256AlignedSseJmp(pVCpu, &(a_u256Dst), (a_iSeg), (a_GCPtrMem))
#endif



#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U8_ZX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u16Dst) = u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U32_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint32_t u32Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &u32Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = u32Tmp; \
    } while (0)
#else  /* IEM_WITH_SETJMP */
# define IEM_MC_FETCH_MEM_U8_ZX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    ((a_u16Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP
# define IEM_MC_FETCH_MEM_U8_SX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u16Dst) = (int8_t)u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = (int8_t)u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U8_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint8_t u8Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU8(pVCpu, &u8Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = (int8_t)u8Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u32Dst) = (int16_t)u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U16_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint16_t u16Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU16(pVCpu, &u16Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = (int16_t)u16Tmp; \
    } while (0)
# define IEM_MC_FETCH_MEM_U32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    do { \
        uint32_t u32Tmp; \
        IEM_MC_RETURN_ON_FAILURE(iemMemFetchDataU32(pVCpu, &u32Tmp, (a_iSeg), (a_GCPtrMem))); \
        (a_u64Dst) = (int32_t)u32Tmp; \
    } while (0)
#else  /* IEM_WITH_SETJMP */
# define IEM_MC_FETCH_MEM_U8_SX_U16(a_u16Dst, a_iSeg, a_GCPtrMem) \
    ((a_u16Dst) = (int8_t)iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = (int8_t)iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U8_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = (int8_t)iemMemFetchDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem) \
    ((a_u32Dst) = (int16_t)iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U16_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = (int16_t)iemMemFetchDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
# define IEM_MC_FETCH_MEM_U32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem) \
    ((a_u64Dst) = (int32_t)iemMemFetchDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem)))
#endif /* IEM_WITH_SETJMP */

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U8(a_iSeg, a_GCPtrMem, a_u8Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU8(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8Value)))
# define IEM_MC_STORE_MEM_U16(a_iSeg, a_GCPtrMem, a_u16Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU16(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16Value)))
# define IEM_MC_STORE_MEM_U32(a_iSeg, a_GCPtrMem, a_u32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU32(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32Value)))
# define IEM_MC_STORE_MEM_U64(a_iSeg, a_GCPtrMem, a_u64Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU64(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64Value)))
#else
# define IEM_MC_STORE_MEM_U8(a_iSeg, a_GCPtrMem, a_u8Value) \
    iemMemStoreDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8Value))
# define IEM_MC_STORE_MEM_U16(a_iSeg, a_GCPtrMem, a_u16Value) \
    iemMemStoreDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16Value))
# define IEM_MC_STORE_MEM_U32(a_iSeg, a_GCPtrMem, a_u32Value) \
    iemMemStoreDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32Value))
# define IEM_MC_STORE_MEM_U64(a_iSeg, a_GCPtrMem, a_u64Value) \
    iemMemStoreDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64Value))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U8_CONST(a_iSeg, a_GCPtrMem, a_u8C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU8(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8C)))
# define IEM_MC_STORE_MEM_U16_CONST(a_iSeg, a_GCPtrMem, a_u16C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU16(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16C)))
# define IEM_MC_STORE_MEM_U32_CONST(a_iSeg, a_GCPtrMem, a_u32C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU32(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32C)))
# define IEM_MC_STORE_MEM_U64_CONST(a_iSeg, a_GCPtrMem, a_u64C) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU64(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64C)))
#else
# define IEM_MC_STORE_MEM_U8_CONST(a_iSeg, a_GCPtrMem, a_u8C) \
    iemMemStoreDataU8Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u8C))
# define IEM_MC_STORE_MEM_U16_CONST(a_iSeg, a_GCPtrMem, a_u16C) \
    iemMemStoreDataU16Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u16C))
# define IEM_MC_STORE_MEM_U32_CONST(a_iSeg, a_GCPtrMem, a_u32C) \
    iemMemStoreDataU32Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u32C))
# define IEM_MC_STORE_MEM_U64_CONST(a_iSeg, a_GCPtrMem, a_u64C) \
    iemMemStoreDataU64Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u64C))
#endif

#define IEM_MC_STORE_MEM_I8_CONST_BY_REF( a_pi8Dst,  a_i8C)     *(a_pi8Dst)  = (a_i8C)
#define IEM_MC_STORE_MEM_I16_CONST_BY_REF(a_pi16Dst, a_i16C)    *(a_pi16Dst) = (a_i16C)
#define IEM_MC_STORE_MEM_I32_CONST_BY_REF(a_pi32Dst, a_i32C)    *(a_pi32Dst) = (a_i32C)
#define IEM_MC_STORE_MEM_I64_CONST_BY_REF(a_pi64Dst, a_i64C)    *(a_pi64Dst) = (a_i64C)
#define IEM_MC_STORE_MEM_NEG_QNAN_R32_BY_REF(a_pr32Dst)         (a_pr32Dst)->u32     = UINT32_C(0xffc00000)
#define IEM_MC_STORE_MEM_NEG_QNAN_R64_BY_REF(a_pr64Dst)         (a_pr64Dst)->au64[0] = UINT64_C(0xfff8000000000000)
#define IEM_MC_STORE_MEM_NEG_QNAN_R80_BY_REF(a_pr80Dst) \
    do { \
        (a_pr80Dst)->au64[0] = UINT64_C(0xc000000000000000); \
        (a_pr80Dst)->au16[4] = UINT16_C(0xffff); \
    } while (0)

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U128(a_iSeg, a_GCPtrMem, a_u128Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU128(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value)))
# define IEM_MC_STORE_MEM_U128_ALIGN_SSE(a_iSeg, a_GCPtrMem, a_u128Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU128AlignedSse(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value)))
#else
# define IEM_MC_STORE_MEM_U128(a_iSeg, a_GCPtrMem, a_u128Value) \
    iemMemStoreDataU128Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value))
# define IEM_MC_STORE_MEM_U128_ALIGN_SSE(a_iSeg, a_GCPtrMem, a_u128Value) \
    iemMemStoreDataU128AlignedSseJmp(pVCpu, (a_iSeg), (a_GCPtrMem), (a_u128Value))
#endif

#ifndef IEM_WITH_SETJMP
# define IEM_MC_STORE_MEM_U256(a_iSeg, a_GCPtrMem, a_u256Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU256(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value)))
# define IEM_MC_STORE_MEM_U256_ALIGN_AVX(a_iSeg, a_GCPtrMem, a_u256Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStoreDataU256AlignedAvx(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value)))
#else
# define IEM_MC_STORE_MEM_U256(a_iSeg, a_GCPtrMem, a_u256Value) \
    iemMemStoreDataU256Jmp(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value))
# define IEM_MC_STORE_MEM_U256_ALIGN_AVX(a_iSeg, a_GCPtrMem, a_u256Value) \
    iemMemStoreDataU256AlignedAvxJmp(pVCpu, (a_iSeg), (a_GCPtrMem), &(a_u256Value))
#endif


#define IEM_MC_PUSH_U16(a_u16Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU16(pVCpu, (a_u16Value)))
#define IEM_MC_PUSH_U32(a_u32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU32(pVCpu, (a_u32Value)))
#define IEM_MC_PUSH_U32_SREG(a_u32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU32SReg(pVCpu, (a_u32Value)))
#define IEM_MC_PUSH_U64(a_u64Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPushU64(pVCpu, (a_u64Value)))

#define IEM_MC_POP_U16(a_pu16Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPopU16(pVCpu, (a_pu16Value)))
#define IEM_MC_POP_U32(a_pu32Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPopU32(pVCpu, (a_pu32Value)))
#define IEM_MC_POP_U64(a_pu64Value) \
    IEM_MC_RETURN_ON_FAILURE(iemMemStackPopU64(pVCpu, (a_pu64Value)))

/** Maps guest memory for direct or bounce buffered access.
 * The purpose is to pass it to an operand implementation, thus the a_iArg.
 * @remarks     May return.
 */
#define IEM_MC_MEM_MAP(a_pMem, a_fAccess, a_iSeg, a_GCPtrMem, a_iArg) \
    IEM_MC_RETURN_ON_FAILURE(iemMemMap(pVCpu, (void **)&(a_pMem), sizeof(*(a_pMem)), (a_iSeg), (a_GCPtrMem), (a_fAccess)))

/** Maps guest memory for direct or bounce buffered access.
 * The purpose is to pass it to an operand implementation, thus the a_iArg.
 * @remarks     May return.
 */
#define IEM_MC_MEM_MAP_EX(a_pvMem, a_fAccess, a_cbMem, a_iSeg, a_GCPtrMem, a_iArg) \
    IEM_MC_RETURN_ON_FAILURE(iemMemMap(pVCpu, (void **)&(a_pvMem), (a_cbMem), (a_iSeg), (a_GCPtrMem), (a_fAccess)))

/** Commits the memory and unmaps the guest memory.
 * @remarks     May return.
 */
#define IEM_MC_MEM_COMMIT_AND_UNMAP(a_pvMem, a_fAccess) \
    IEM_MC_RETURN_ON_FAILURE(iemMemCommitAndUnmap(pVCpu, (a_pvMem), (a_fAccess)))

/** Commits the memory and unmaps the guest memory unless the FPU status word
 * indicates (@a a_u16FSW) and FPU control word indicates a pending exception
 * that would cause FLD not to store.
 *
 * The current understanding is that \#O, \#U, \#IA and \#IS will prevent a
 * store, while \#P will not.
 *
 * @remarks     May in theory return - for now.
 */
#define IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(a_pvMem, a_fAccess, a_u16FSW) \
    do { \
        if (   !(a_u16FSW & X86_FSW_ES) \
            || !(  (a_u16FSW & (X86_FSW_UE | X86_FSW_OE | X86_FSW_IE)) \
                 & ~(IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FCW & X86_FCW_MASK_ALL) ) ) \
            IEM_MC_RETURN_ON_FAILURE(iemMemCommitAndUnmap(pVCpu, (a_pvMem), (a_fAccess))); \
    } while (0)

/** Calculate efficient address from R/M. */
#ifndef IEM_WITH_SETJMP
# define IEM_MC_CALC_RM_EFF_ADDR(a_GCPtrEff, bRm, cbImm) \
    IEM_MC_RETURN_ON_FAILURE(iemOpHlpCalcRmEffAddr(pVCpu, (bRm), (cbImm), &(a_GCPtrEff)))
#else
# define IEM_MC_CALC_RM_EFF_ADDR(a_GCPtrEff, bRm, cbImm) \
    ((a_GCPtrEff) = iemOpHlpCalcRmEffAddrJmp(pVCpu, (bRm), (cbImm)))
#endif

#define IEM_MC_CALL_VOID_AIMPL_0(a_pfn)                   (a_pfn)()
#define IEM_MC_CALL_VOID_AIMPL_1(a_pfn, a0)               (a_pfn)((a0))
#define IEM_MC_CALL_VOID_AIMPL_2(a_pfn, a0, a1)           (a_pfn)((a0), (a1))
#define IEM_MC_CALL_VOID_AIMPL_3(a_pfn, a0, a1, a2)       (a_pfn)((a0), (a1), (a2))
#define IEM_MC_CALL_VOID_AIMPL_4(a_pfn, a0, a1, a2, a3)   (a_pfn)((a0), (a1), (a2), (a3))
#define IEM_MC_CALL_AIMPL_3(a_rc, a_pfn, a0, a1, a2)      (a_rc) = (a_pfn)((a0), (a1), (a2))
#define IEM_MC_CALL_AIMPL_4(a_rc, a_pfn, a0, a1, a2, a3)  (a_rc) = (a_pfn)((a0), (a1), (a2), (a3))

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, only taking the standard parameters.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @sa      IEM_DECL_IMPL_C_TYPE_0 and IEM_CIMPL_DEF_0.
 */
#define IEM_MC_CALL_CIMPL_0(a_pfnCImpl)                 return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu))

/**
 * Defers the rest of instruction emulation to a C implementation routine and
 * returns, taking one argument in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The argument.
 */
#define IEM_MC_CALL_CIMPL_1(a_pfnCImpl, a0)             return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking two arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_CIMPL_2(a_pfnCImpl, a0, a1)         return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking three arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_CIMPL_3(a_pfnCImpl, a0, a1, a2)     return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking four arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 * @param   a3              The fourth extra argument.
 */
#define IEM_MC_CALL_CIMPL_4(a_pfnCImpl, a0, a1, a2, a3)     return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2, a3)

/**
 * Defers the rest of the instruction emulation to a C implementation routine
 * and returns, taking two arguments in addition to the standard ones.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 * @param   a3              The fourth extra argument.
 * @param   a4              The fifth extra argument.
 */
#define IEM_MC_CALL_CIMPL_5(a_pfnCImpl, a0, a1, a2, a3, a4) return (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2, a3, a4)

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, only taking the standard parameters.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @sa      IEM_DECL_IMPL_C_TYPE_0 and IEM_CIMPL_DEF_0.
 */
#define IEM_MC_DEFER_TO_CIMPL_0(a_pfnCImpl)             (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu))

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, taking one argument in addition to the standard ones.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The argument.
 */
#define IEM_MC_DEFER_TO_CIMPL_1(a_pfnCImpl, a0)         (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0)

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, taking two arguments in addition to the standard ones.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_DEFER_TO_CIMPL_2(a_pfnCImpl, a0, a1)     (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1)

/**
 * Defers the entire instruction emulation to a C implementation routine and
 * returns, taking three arguments in addition to the standard ones.
 *
 * This shall be used without any IEM_MC_BEGIN or IEM_END macro surrounding it.
 *
 * @param   a_pfnCImpl      The pointer to the C routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_DEFER_TO_CIMPL_3(a_pfnCImpl, a0, a1, a2) (a_pfnCImpl)(pVCpu, IEM_GET_INSTR_LEN(pVCpu), a0, a1, a2)

/**
 * Calls a FPU assembly implementation taking one visible argument.
 *
 * @param   a_pfnAImpl      Pointer to the assembly FPU routine.
 * @param   a0              The first extra argument.
 */
#define IEM_MC_CALL_FPU_AIMPL_1(a_pfnAImpl, a0) \
    do { \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0)); \
    } while (0)

/**
 * Calls a FPU assembly implementation taking two visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly FPU routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_FPU_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0), (a1)); \
    } while (0)

/**
 * Calls a FPU assembly implementation taking three visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly FPU routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_FPU_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0), (a1), (a2)); \
    } while (0)

#define IEM_MC_SET_FPU_RESULT(a_FpuData, a_FSW, a_pr80Value) \
    do { \
        (a_FpuData).FSW       = (a_FSW); \
        (a_FpuData).r80Result = *(a_pr80Value); \
    } while (0)

/** Pushes FPU result onto the stack. */
#define IEM_MC_PUSH_FPU_RESULT(a_FpuData) \
    iemFpuPushResult(pVCpu, &a_FpuData)
/** Pushes FPU result onto the stack and sets the FPUDP. */
#define IEM_MC_PUSH_FPU_RESULT_MEM_OP(a_FpuData, a_iEffSeg, a_GCPtrEff) \
    iemFpuPushResultWithMemOp(pVCpu, &a_FpuData, a_iEffSeg, a_GCPtrEff)

/** Replaces ST0 with value one and pushes value 2 onto the FPU stack. */
#define IEM_MC_PUSH_FPU_RESULT_TWO(a_FpuDataTwo) \
    iemFpuPushResultTwo(pVCpu, &a_FpuDataTwo)

/** Stores FPU result in a stack register. */
#define IEM_MC_STORE_FPU_RESULT(a_FpuData, a_iStReg) \
    iemFpuStoreResult(pVCpu, &a_FpuData, a_iStReg)
/** Stores FPU result in a stack register and pops the stack. */
#define IEM_MC_STORE_FPU_RESULT_THEN_POP(a_FpuData, a_iStReg) \
    iemFpuStoreResultThenPop(pVCpu, &a_FpuData, a_iStReg)
/** Stores FPU result in a stack register and sets the FPUDP. */
#define IEM_MC_STORE_FPU_RESULT_MEM_OP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff) \
    iemFpuStoreResultWithMemOp(pVCpu, &a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)
/** Stores FPU result in a stack register, sets the FPUDP, and pops the
 *  stack. */
#define IEM_MC_STORE_FPU_RESULT_WITH_MEM_OP_THEN_POP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff) \
    iemFpuStoreResultWithMemOpThenPop(pVCpu, &a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)

/** Only update the FOP, FPUIP, and FPUCS. (For FNOP.) */
#define IEM_MC_UPDATE_FPU_OPCODE_IP() \
    iemFpuUpdateOpcodeAndIp(pVCpu)
/** Free a stack register (for FFREE and FFREEP). */
#define IEM_MC_FPU_STACK_FREE(a_iStReg) \
    iemFpuStackFree(pVCpu, a_iStReg)
/** Increment the FPU stack pointer. */
#define IEM_MC_FPU_STACK_INC_TOP() \
    iemFpuStackIncTop(pVCpu)
/** Decrement the FPU stack pointer. */
#define IEM_MC_FPU_STACK_DEC_TOP() \
    iemFpuStackDecTop(pVCpu)

/** Updates the FSW, FOP, FPUIP, and FPUCS. */
#define IEM_MC_UPDATE_FSW(a_u16FSW) \
    iemFpuUpdateFSW(pVCpu, a_u16FSW)
/** Updates the FSW with a constant value as well as FOP, FPUIP, and FPUCS. */
#define IEM_MC_UPDATE_FSW_CONST(a_u16FSW) \
    iemFpuUpdateFSW(pVCpu, a_u16FSW)
/** Updates the FSW, FOP, FPUIP, FPUCS, FPUDP, and FPUDS. */
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP(a_u16FSW, a_iEffSeg, a_GCPtrEff) \
    iemFpuUpdateFSWWithMemOp(pVCpu, a_u16FSW, a_iEffSeg, a_GCPtrEff)
/** Updates the FSW, FOP, FPUIP, and FPUCS, and then pops the stack. */
#define IEM_MC_UPDATE_FSW_THEN_POP(a_u16FSW) \
    iemFpuUpdateFSWThenPop(pVCpu, a_u16FSW)
/** Updates the FSW, FOP, FPUIP, FPUCS, FPUDP and FPUDS, and then pops the
 *  stack. */
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(a_u16FSW, a_iEffSeg, a_GCPtrEff) \
    iemFpuUpdateFSWWithMemOpThenPop(pVCpu, a_u16FSW, a_iEffSeg, a_GCPtrEff)
/** Updates the FSW, FOP, FPUIP, and FPUCS, and then pops the stack twice. */
#define IEM_MC_UPDATE_FSW_THEN_POP_POP(a_u16FSW) \
    iemFpuUpdateFSWThenPopPop(pVCpu, a_u16FSW)

/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_UNDERFLOW(a_iStDst) \
    iemFpuStackUnderflow(pVCpu, a_iStDst)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS and FOP. Pops
 *  stack. */
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(a_iStDst) \
    iemFpuStackUnderflowThenPop(pVCpu, a_iStDst)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS, FOP, FPUDP and
 *  FPUDS. */
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(a_iStDst, a_iEffSeg, a_GCPtrEff) \
    iemFpuStackUnderflowWithMemOp(pVCpu, a_iStDst, a_iEffSeg, a_GCPtrEff)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS, FOP, FPUDP and
 *  FPUDS. Pops stack. */
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(a_iStDst, a_iEffSeg, a_GCPtrEff) \
    iemFpuStackUnderflowWithMemOpThenPop(pVCpu, a_iStDst, a_iEffSeg, a_GCPtrEff)
/** Raises a FPU stack underflow exception.  Sets FPUIP, FPUCS and FOP. Pops
 *  stack twice. */
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP_POP() \
    iemFpuStackUnderflowThenPopPop(pVCpu)
/** Raises a FPU stack underflow exception for an instruction pushing a result
 *  value onto the stack. Sets FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW() \
    iemFpuStackPushUnderflow(pVCpu)
/** Raises a FPU stack underflow exception for an instruction pushing a result
 *  value onto the stack and replacing ST0. Sets FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW_TWO() \
    iemFpuStackPushUnderflowTwo(pVCpu)

/** Raises a FPU stack overflow exception as part of a push attempt.  Sets
 *  FPUIP, FPUCS and FOP. */
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW() \
    iemFpuStackPushOverflow(pVCpu)
/** Raises a FPU stack overflow exception as part of a push attempt.  Sets
 *  FPUIP, FPUCS, FOP, FPUDP and FPUDS. */
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(a_iEffSeg, a_GCPtrEff) \
    iemFpuStackPushOverflowWithMemOp(pVCpu, a_iEffSeg, a_GCPtrEff)
/** Prepares for using the FPU state.
 * Ensures that we can use the host FPU in the current context (RC+R0.
 * Ensures the guest FPU state in the CPUMCTX is up to date. */
#define IEM_MC_PREPARE_FPU_USAGE()              iemFpuPrepareUsage(pVCpu)
/** Actualizes the guest FPU state so it can be accessed read-only fashion. */
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ()   iemFpuActualizeStateForRead(pVCpu)
/** Actualizes the guest FPU state so it can be accessed and modified. */
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE() iemFpuActualizeStateForChange(pVCpu)

/** Prepares for using the SSE state.
 * Ensures that we can use the host SSE/FPU in the current context (RC+R0.
 * Ensures the guest SSE state in the CPUMCTX is up to date. */
#define IEM_MC_PREPARE_SSE_USAGE()              iemFpuPrepareUsageSse(pVCpu)
/** Actualizes the guest XMM0..15 and MXCSR register state for read-only access. */
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_READ()   iemFpuActualizeSseStateForRead(pVCpu)
/** Actualizes the guest XMM0..15 and MXCSR register state for read-write access. */
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_CHANGE() iemFpuActualizeSseStateForChange(pVCpu)

/** Prepares for using the AVX state.
 * Ensures that we can use the host AVX/FPU in the current context (RC+R0.
 * Ensures the guest AVX state in the CPUMCTX is up to date.
 * @note This will include the AVX512 state too when support for it is added
 *       due to the zero extending feature of VEX instruction. */
#define IEM_MC_PREPARE_AVX_USAGE()              iemFpuPrepareUsageAvx(pVCpu)
/** Actualizes the guest XMM0..15 and MXCSR register state for read-only access. */
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_READ()   iemFpuActualizeAvxStateForRead(pVCpu)
/** Actualizes the guest YMM0..15 and MXCSR register state for read-write access. */
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_CHANGE() iemFpuActualizeAvxStateForChange(pVCpu)

/**
 * Calls a MMX assembly implementation taking two visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly MMX routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_MMX_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { \
        IEM_MC_PREPARE_FPU_USAGE(); \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0), (a1)); \
    } while (0)

/**
 * Calls a MMX assembly implementation taking three visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly MMX routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_MMX_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { \
        IEM_MC_PREPARE_FPU_USAGE(); \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0), (a1), (a2)); \
    } while (0)


/**
 * Calls a SSE assembly implementation taking two visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly SSE routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 */
#define IEM_MC_CALL_SSE_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { \
        IEM_MC_PREPARE_SSE_USAGE(); \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0), (a1)); \
    } while (0)

/**
 * Calls a SSE assembly implementation taking three visible arguments.
 *
 * @param   a_pfnAImpl      Pointer to the assembly SSE routine.
 * @param   a0              The first extra argument.
 * @param   a1              The second extra argument.
 * @param   a2              The third extra argument.
 */
#define IEM_MC_CALL_SSE_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { \
        IEM_MC_PREPARE_SSE_USAGE(); \
        a_pfnAImpl(&IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87, (a0), (a1), (a2)); \
    } while (0)


/** Declares implicit arguments for IEM_MC_CALL_AVX_AIMPL_2,
 *  IEM_MC_CALL_AVX_AIMPL_3, IEM_MC_CALL_AVX_AIMPL_4, ... */
#define IEM_MC_IMPLICIT_AVX_AIMPL_ARGS() \
    IEM_MC_ARG_CONST(PX86XSAVEAREA, pXState, (pVCpu)->iem.s.CTX_SUFF(pCtx)->CTX_SUFF(pXState), 0)

/**
 * Calls a AVX assembly implementation taking two visible arguments.
 *
 * There is one implicit zero'th argument, a pointer to the extended state.
 *
 * @param   a_pfnAImpl      Pointer to the assembly AVX routine.
 * @param   a1              The first extra argument.
 * @param   a2              The second extra argument.
 */
#define IEM_MC_CALL_AVX_AIMPL_2(a_pfnAImpl, a1, a2) \
    do { \
        IEM_MC_PREPARE_AVX_USAGE(); \
        a_pfnAImpl(pXState, (a1), (a2)); \
    } while (0)

/**
 * Calls a AVX assembly implementation taking three visible arguments.
 *
 * There is one implicit zero'th argument, a pointer to the extended state.
 *
 * @param   a_pfnAImpl      Pointer to the assembly AVX routine.
 * @param   a1              The first extra argument.
 * @param   a2              The second extra argument.
 * @param   a3              The third extra argument.
 */
#define IEM_MC_CALL_AVX_AIMPL_3(a_pfnAImpl, a1, a2, a3) \
    do { \
        IEM_MC_PREPARE_AVX_USAGE(); \
        a_pfnAImpl(pXState, (a1), (a2), (a3)); \
    } while (0)

/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_SET(a_fBit)                   if (IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_NOT_SET(a_fBit)               if (!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit))) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_ANY_BITS_SET(a_fBits)             if (IEM_GET_CTX(pVCpu)->eflags.u & (a_fBits)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_NO_BITS_SET(a_fBits)              if (!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBits))) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BITS_NE(a_fBit1, a_fBit2)         \
    if (   !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit1)) \
        != !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit2)) ) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BITS_EQ(a_fBit1, a_fBit2)         \
    if (   !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit1)) \
        == !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit2)) ) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(a_fBit, a_fBit1, a_fBit2) \
    if (   (IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit)) \
        ||    !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit1)) \
           != !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit2)) ) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_EFL_BIT_NOT_SET_AND_BITS_EQ(a_fBit, a_fBit1, a_fBit2) \
    if (   !(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit)) \
        &&    !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit1)) \
           == !!(IEM_GET_CTX(pVCpu)->eflags.u & (a_fBit2)) ) {
#define IEM_MC_IF_CX_IS_NZ()                            if (IEM_GET_CTX(pVCpu)->cx != 0) {
#define IEM_MC_IF_ECX_IS_NZ()                           if (IEM_GET_CTX(pVCpu)->ecx != 0) {
#define IEM_MC_IF_RCX_IS_NZ()                           if (IEM_GET_CTX(pVCpu)->rcx != 0) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_SET(a_fBit) \
        if (   IEM_GET_CTX(pVCpu)->cx != 0 \
            && (IEM_GET_CTX(pVCpu)->eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_SET(a_fBit) \
        if (   IEM_GET_CTX(pVCpu)->ecx != 0 \
            && (IEM_GET_CTX(pVCpu)->eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_SET(a_fBit) \
        if (   IEM_GET_CTX(pVCpu)->rcx != 0 \
            && (IEM_GET_CTX(pVCpu)->eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit) \
        if (   IEM_GET_CTX(pVCpu)->cx != 0 \
            && !(IEM_GET_CTX(pVCpu)->eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit) \
        if (   IEM_GET_CTX(pVCpu)->ecx != 0 \
            && !(IEM_GET_CTX(pVCpu)->eflags.u & a_fBit)) {
/** @note Not for IOPL or IF testing. */
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit) \
        if (   IEM_GET_CTX(pVCpu)->rcx != 0 \
            && !(IEM_GET_CTX(pVCpu)->eflags.u & a_fBit)) {
#define IEM_MC_IF_LOCAL_IS_Z(a_Local)                   if ((a_Local) == 0) {
#define IEM_MC_IF_GREG_BIT_SET(a_iGReg, a_iBitNo)       if (iemGRegFetchU64(pVCpu, (a_iGReg)) & RT_BIT_64(a_iBitNo)) {

#define IEM_MC_IF_FPUREG_NOT_EMPTY(a_iSt) \
    if (iemFpuStRegNotEmpty(pVCpu, (a_iSt)) == VINF_SUCCESS) {
#define IEM_MC_IF_FPUREG_IS_EMPTY(a_iSt) \
    if (iemFpuStRegNotEmpty(pVCpu, (a_iSt)) != VINF_SUCCESS) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(a_pr80Dst, a_iSt) \
    if (iemFpuStRegNotEmptyRef(pVCpu, (a_iSt), &(a_pr80Dst)) == VINF_SUCCESS) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(a_pr80Dst0, a_iSt0, a_pr80Dst1, a_iSt1) \
    if (iemFpu2StRegsNotEmptyRef(pVCpu, (a_iSt0), &(a_pr80Dst0), (a_iSt1), &(a_pr80Dst1)) == VINF_SUCCESS) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(a_pr80Dst0, a_iSt0, a_iSt1) \
    if (iemFpu2StRegsNotEmptyRefFirst(pVCpu, (a_iSt0), &(a_pr80Dst0), (a_iSt1)) == VINF_SUCCESS) {
#define IEM_MC_IF_FCW_IM() \
    if (IEM_GET_CTX(pVCpu)->CTX_SUFF(pXState)->x87.FCW & X86_FCW_IM) {

#define IEM_MC_ELSE()                                   } else {
#define IEM_MC_ENDIF()                                  } do {} while (0)

/** @}  */


/** @name   Opcode Debug Helpers.
 * @{
 */
#ifdef VBOX_WITH_STATISTICS
# define IEMOP_INC_STATS(a_Stats) do { pVCpu->iem.s.CTX_SUFF(pStats)->a_Stats += 1; } while (0)
#else
# define IEMOP_INC_STATS(a_Stats) do { } while (0)
#endif

#ifdef DEBUG
# define IEMOP_MNEMONIC(a_Stats, a_szMnemonic) \
    do { \
        IEMOP_INC_STATS(a_Stats); \
        Log4(("decode - %04x:%RGv %s%s [#%u]\n", IEM_GET_CTX(pVCpu)->cs.Sel, IEM_GET_CTX(pVCpu)->rip, \
              pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK ? "lock " : "", a_szMnemonic, pVCpu->iem.s.cInstructions)); \
    } while (0)

# define IEMOP_MNEMONIC0EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints) \
    do { \
        IEMOP_MNEMONIC(a_Stats, a_szMnemonic); \
        (void)RT_CONCAT(IEMOPFORM_, a_Form); \
        (void)RT_CONCAT(OP_,a_Upper); \
        (void)(a_fDisHints); \
        (void)(a_fIemHints); \
    } while (0)

# define IEMOP_MNEMONIC1EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints) \
    do { \
        IEMOP_MNEMONIC(a_Stats, a_szMnemonic); \
        (void)RT_CONCAT(IEMOPFORM_, a_Form); \
        (void)RT_CONCAT(OP_,a_Upper); \
        (void)RT_CONCAT(OP_PARM_,a_Op1); \
        (void)(a_fDisHints); \
        (void)(a_fIemHints); \
    } while (0)

# define IEMOP_MNEMONIC2EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints) \
    do { \
        IEMOP_MNEMONIC(a_Stats, a_szMnemonic); \
        (void)RT_CONCAT(IEMOPFORM_, a_Form); \
        (void)RT_CONCAT(OP_,a_Upper); \
        (void)RT_CONCAT(OP_PARM_,a_Op1); \
        (void)RT_CONCAT(OP_PARM_,a_Op2); \
        (void)(a_fDisHints); \
        (void)(a_fIemHints); \
    } while (0)

# define IEMOP_MNEMONIC3EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints) \
    do { \
        IEMOP_MNEMONIC(a_Stats, a_szMnemonic); \
        (void)RT_CONCAT(IEMOPFORM_, a_Form); \
        (void)RT_CONCAT(OP_,a_Upper); \
        (void)RT_CONCAT(OP_PARM_,a_Op1); \
        (void)RT_CONCAT(OP_PARM_,a_Op2); \
        (void)RT_CONCAT(OP_PARM_,a_Op3); \
        (void)(a_fDisHints); \
        (void)(a_fIemHints); \
    } while (0)

# define IEMOP_MNEMONIC4EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints) \
    do { \
        IEMOP_MNEMONIC(a_Stats, a_szMnemonic); \
        (void)RT_CONCAT(IEMOPFORM_, a_Form); \
        (void)RT_CONCAT(OP_,a_Upper); \
        (void)RT_CONCAT(OP_PARM_,a_Op1); \
        (void)RT_CONCAT(OP_PARM_,a_Op2); \
        (void)RT_CONCAT(OP_PARM_,a_Op3); \
        (void)RT_CONCAT(OP_PARM_,a_Op4); \
        (void)(a_fDisHints); \
        (void)(a_fIemHints); \
    } while (0)

#else
# define IEMOP_MNEMONIC(a_Stats, a_szMnemonic) IEMOP_INC_STATS(a_Stats)

# define IEMOP_MNEMONIC0EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints) \
         IEMOP_MNEMONIC(a_Stats, a_szMnemonic)
# define IEMOP_MNEMONIC1EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints) \
         IEMOP_MNEMONIC(a_Stats, a_szMnemonic)
# define IEMOP_MNEMONIC2EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints) \
         IEMOP_MNEMONIC(a_Stats, a_szMnemonic)
# define IEMOP_MNEMONIC3EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints) \
         IEMOP_MNEMONIC(a_Stats, a_szMnemonic)
# define IEMOP_MNEMONIC4EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints) \
         IEMOP_MNEMONIC(a_Stats, a_szMnemonic)

#endif

#define IEMOP_MNEMONIC0(a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints) \
    IEMOP_MNEMONIC0EX(a_Lower, \
                      #a_Lower, \
                      a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints)
#define IEMOP_MNEMONIC1(a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints) \
    IEMOP_MNEMONIC1EX(RT_CONCAT3(a_Lower,_,a_Op1), \
                      #a_Lower " " #a_Op1, \
                      a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints)
#define IEMOP_MNEMONIC2(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints) \
    IEMOP_MNEMONIC2EX(RT_CONCAT5(a_Lower,_,a_Op1,_,a_Op2), \
                      #a_Lower " " #a_Op1 "," #a_Op2, \
                      a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints)
#define IEMOP_MNEMONIC3(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints) \
    IEMOP_MNEMONIC3EX(RT_CONCAT7(a_Lower,_,a_Op1,_,a_Op2,_,a_Op3), \
                      #a_Lower " " #a_Op1 "," #a_Op2 "," #a_Op3, \
                      a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints)
#define IEMOP_MNEMONIC4(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints) \
    IEMOP_MNEMONIC4EX(RT_CONCAT9(a_Lower,_,a_Op1,_,a_Op2,_,a_Op3,_,a_Op4), \
                      #a_Lower " " #a_Op1 "," #a_Op2 "," #a_Op3 "," #a_Op4, \
                      a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints)

/** @} */


/** @name   Opcode Helpers.
 * @{
 */

#ifdef IN_RING3
# define IEMOP_HLP_MIN_CPU(a_uMinCpu, a_fOnlyIf) \
    do { \
        if (IEM_GET_TARGET_CPU(pVCpu) >= (a_uMinCpu) || !(a_fOnlyIf)) { } \
        else \
        { \
            (void)DBGFSTOP(pVCpu->CTX_SUFF(pVM)); \
            return IEMOP_RAISE_INVALID_OPCODE(); \
        } \
    } while (0)
#else
# define IEMOP_HLP_MIN_CPU(a_uMinCpu, a_fOnlyIf) \
    do { \
        if (IEM_GET_TARGET_CPU(pVCpu) >= (a_uMinCpu) || !(a_fOnlyIf)) { } \
        else return IEMOP_RAISE_INVALID_OPCODE(); \
    } while (0)
#endif

/** The instruction requires a 186 or later. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_186
# define IEMOP_HLP_MIN_186() do { } while (0)
#else
# define IEMOP_HLP_MIN_186() IEMOP_HLP_MIN_CPU(IEMTARGETCPU_186, true)
#endif

/** The instruction requires a 286 or later. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_286
# define IEMOP_HLP_MIN_286() do { } while (0)
#else
# define IEMOP_HLP_MIN_286() IEMOP_HLP_MIN_CPU(IEMTARGETCPU_286, true)
#endif

/** The instruction requires a 386 or later. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_386
# define IEMOP_HLP_MIN_386() do { } while (0)
#else
# define IEMOP_HLP_MIN_386() IEMOP_HLP_MIN_CPU(IEMTARGETCPU_386, true)
#endif

/** The instruction requires a 386 or later if the given expression is true. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_386
# define IEMOP_HLP_MIN_386_EX(a_fOnlyIf) do { } while (0)
#else
# define IEMOP_HLP_MIN_386_EX(a_fOnlyIf) IEMOP_HLP_MIN_CPU(IEMTARGETCPU_386, a_fOnlyIf)
#endif

/** The instruction requires a 486 or later. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_486
# define IEMOP_HLP_MIN_486() do { } while (0)
#else
# define IEMOP_HLP_MIN_486() IEMOP_HLP_MIN_CPU(IEMTARGETCPU_486, true)
#endif

/** The instruction requires a Pentium (586) or later. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_PENTIUM
# define IEMOP_HLP_MIN_586() do { } while (0)
#else
# define IEMOP_HLP_MIN_586() IEMOP_HLP_MIN_CPU(IEMTARGETCPU_PENTIUM, true)
#endif

/** The instruction requires a PentiumPro (686) or later. */
#if IEM_CFG_TARGET_CPU >= IEMTARGETCPU_PPRO
# define IEMOP_HLP_MIN_686() do { } while (0)
#else
# define IEMOP_HLP_MIN_686() IEMOP_HLP_MIN_CPU(IEMTARGETCPU_PPRO, true)
#endif


/** The instruction raises an \#UD in real and V8086 mode. */
#define IEMOP_HLP_NO_REAL_OR_V86_MODE() \
    do \
    { \
        if (!IEM_IS_REAL_OR_V86_MODE(pVCpu)) { /* likely */ } \
        else return IEMOP_RAISE_INVALID_OPCODE(); \
    } while (0)

/** The instruction is not available in 64-bit mode, throw \#UD if we're in
 * 64-bit mode. */
#define IEMOP_HLP_NO_64BIT() \
    do \
    { \
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT) \
            return IEMOP_RAISE_INVALID_OPCODE(); \
    } while (0)

/** The instruction is only available in 64-bit mode, throw \#UD if we're not in
 * 64-bit mode. */
#define IEMOP_HLP_ONLY_64BIT() \
    do \
    { \
        if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT) \
            return IEMOP_RAISE_INVALID_OPCODE(); \
    } while (0)

/** The instruction defaults to 64-bit operand size if 64-bit mode. */
#define IEMOP_HLP_DEFAULT_64BIT_OP_SIZE() \
    do \
    { \
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT) \
            iemRecalEffOpSize64Default(pVCpu); \
    } while (0)

/** The instruction has 64-bit operand size if 64-bit mode. */
#define IEMOP_HLP_64BIT_OP_SIZE() \
    do \
    { \
        if (pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT) \
            pVCpu->iem.s.enmEffOpSize = pVCpu->iem.s.enmDefOpSize = IEMMODE_64BIT; \
    } while (0)

/** Only a REX prefix immediately preceeding the first opcode byte takes
 * effect. This macro helps ensuring this as well as logging bad guest code.  */
#define IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE(a_szPrf) \
    do \
    { \
        if (RT_UNLIKELY(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REX)) \
        { \
            Log5((a_szPrf ": Overriding REX prefix at %RX16! fPrefixes=%#x\n", \
                  IEM_GET_CTX(pVCpu)->rip, pVCpu->iem.s.fPrefixes)); \
            pVCpu->iem.s.fPrefixes &= ~IEM_OP_PRF_REX_MASK; \
            pVCpu->iem.s.uRexB     = 0; \
            pVCpu->iem.s.uRexIndex = 0; \
            pVCpu->iem.s.uRexReg   = 0; \
            iemRecalEffOpSize(pVCpu); \
        } \
    } while (0)

/**
 * Done decoding.
 */
#define IEMOP_HLP_DONE_DECODING() \
    do \
    { \
        /*nothing for now, maybe later... */ \
    } while (0)

/**
 * Done decoding, raise \#UD exception if lock prefix present.
 */
#define IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX() \
    do \
    { \
        if (RT_LIKELY(!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))) \
        { /* likely */ } \
        else \
            return IEMOP_RAISE_INVALID_LOCK_PREFIX(); \
    } while (0)


/**
 * Done decoding VEX instruction, raise \#UD exception if any lock, rex, repz,
 * repnz or size prefixes are present, or if in real or v8086 mode.
 */
#define IEMOP_HLP_DONE_VEX_DECODING() \
    do \
    { \
        if (RT_LIKELY(   !(  pVCpu->iem.s.fPrefixes \
                           & (IEM_OP_PRF_LOCK | IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ | IEM_OP_PRF_SIZE_OP | IEM_OP_PRF_REX)) \
                      && !IEM_IS_REAL_OR_V86_MODE(pVCpu) )) \
        { /* likely */ } \
        else \
            return IEMOP_RAISE_INVALID_LOCK_PREFIX(); \
    } while (0)

/**
 * Done decoding VEX instruction, raise \#UD exception if any lock, rex, repz,
 * repnz or size prefixes are present, or if in real or v8086 mode.
 */
#define IEMOP_HLP_DONE_VEX_DECODING_L0() \
    do \
    { \
        if (RT_LIKELY(   !(  pVCpu->iem.s.fPrefixes \
                           & (IEM_OP_PRF_LOCK | IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ | IEM_OP_PRF_SIZE_OP | IEM_OP_PRF_REX)) \
                      && !IEM_IS_REAL_OR_V86_MODE(pVCpu) \
                      && pVCpu->iem.s.uVexLength == 0)) \
        { /* likely */ } \
        else \
            return IEMOP_RAISE_INVALID_LOCK_PREFIX(); \
    } while (0)


/**
 * Done decoding VEX instruction, raise \#UD exception if any lock, rex, repz,
 * repnz or size prefixes are present, or if the VEX.VVVV field doesn't indicate
 * register 0, or if in real or v8086 mode.
 */
#define IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV() \
    do \
    { \
        if (RT_LIKELY(   !(  pVCpu->iem.s.fPrefixes \
                           & (IEM_OP_PRF_LOCK | IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ | IEM_OP_PRF_SIZE_OP | IEM_OP_PRF_REX)) \
                      && !pVCpu->iem.s.uVex3rdReg \
                      && !IEM_IS_REAL_OR_V86_MODE(pVCpu) )) \
        { /* likely */ } \
        else \
            return IEMOP_RAISE_INVALID_LOCK_PREFIX(); \
    } while (0)

/**
 * Done decoding VEX, no V, L=0.
 * Raises \#UD exception if rex, rep, opsize or lock prefixes are present, if
 * we're in real or v8086 mode, if VEX.V!=0xf, or if VEX.L!=0.
 */
#define IEMOP_HLP_DONE_VEX_DECODING_L0_AND_NO_VVVV() \
    do \
    { \
        if (RT_LIKELY(   !(  pVCpu->iem.s.fPrefixes \
                           & (IEM_OP_PRF_LOCK | IEM_OP_PRF_SIZE_OP | IEM_OP_PRF_REPZ | IEM_OP_PRF_REPNZ | IEM_OP_PRF_REX)) \
                      && pVCpu->iem.s.uVexLength == 0 \
                      && pVCpu->iem.s.uVex3rdReg == 0 \
                      && !IEM_IS_REAL_OR_V86_MODE(pVCpu))) \
        { /* likely */ } \
        else \
            return IEMOP_RAISE_INVALID_OPCODE(); \
    } while (0)

#define IEMOP_HLP_DECODED_NL_1(a_uDisOpNo, a_fIemOpFlags, a_uDisParam0, a_fDisOpType) \
    do \
    { \
        if (RT_LIKELY(!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))) \
        { /* likely */ } \
        else \
        { \
            NOREF(a_uDisOpNo); NOREF(a_fIemOpFlags); NOREF(a_uDisParam0); NOREF(a_fDisOpType); \
            return IEMOP_RAISE_INVALID_LOCK_PREFIX(); \
        } \
    } while (0)
#define IEMOP_HLP_DECODED_NL_2(a_uDisOpNo, a_fIemOpFlags, a_uDisParam0, a_uDisParam1, a_fDisOpType) \
    do \
    { \
        if (RT_LIKELY(!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_LOCK))) \
        { /* likely */ } \
        else \
        { \
            NOREF(a_uDisOpNo); NOREF(a_fIemOpFlags); NOREF(a_uDisParam0); NOREF(a_uDisParam1); NOREF(a_fDisOpType); \
            return IEMOP_RAISE_INVALID_LOCK_PREFIX(); \
        } \
    } while (0)

/**
 * Done decoding, raise \#UD exception if any lock, repz or repnz prefixes
 * are present.
 */
#define IEMOP_HLP_DONE_DECODING_NO_LOCK_REPZ_OR_REPNZ_PREFIXES() \
    do \
    { \
        if (RT_LIKELY(!(pVCpu->iem.s.fPrefixes & (IEM_OP_PRF_LOCK | IEM_OP_PRF_REPNZ | IEM_OP_PRF_REPZ)))) \
        { /* likely */ } \
        else \
            return IEMOP_RAISE_INVALID_OPCODE(); \
    } while (0)


#ifdef VBOX_WITH_NESTED_HWVIRT
/** Check and handles SVM nested-guest control & instruction intercept. */
# define IEMOP_HLP_SVM_CTRL_INTERCEPT(a_pVCpu, a_Intercept, a_uExitCode, a_uExitInfo1, a_uExitInfo2) \
    do \
    { \
        if (IEM_IS_SVM_CTRL_INTERCEPT_SET(a_pVCpu, a_Intercept)) \
            IEM_RETURN_SVM_VMEXIT(a_pVCpu, a_uExitCode, a_uExitInfo1, a_uExitInfo2); \
    } while (0)

/** Check and handle SVM nested-guest CR0 read intercept. */
# define IEMOP_HLP_SVM_READ_CR_INTERCEPT(a_pVCpu, a_uCr, a_uExitInfo1, a_uExitInfo2) \
    do \
    { \
        if (IEM_IS_SVM_READ_CR_INTERCEPT_SET(a_pVCpu, a_uCr)) \
            IEM_RETURN_SVM_VMEXIT(a_pVCpu, SVM_EXIT_READ_CR0 + (a_uCr), a_uExitInfo1, a_uExitInfo2); \
    } while (0)

#else  /* !VBOX_WITH_NESTED_HWVIRT */
# define IEMOP_HLP_SVM_CTRL_INTERCEPT(a_pVCpu, a_Intercept, a_uExitCode, a_uExitInfo1, a_uExitInfo2)    do { } while (0)
# define IEMOP_HLP_SVM_READ_CR_INTERCEPT(a_pVCpu, a_uCr, a_uExitInfo1, a_uExitInfo2)                    do { } while (0)
#endif /* !VBOX_WITH_NESTED_HWVIRT */


/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   cbImm               The size of any immediate following the
 *                              effective address opcode bytes. Important for
 *                              RIP relative addressing.
 * @param   pGCPtrEff           Where to return the effective address.
 */
IEM_STATIC VBOXSTRICTRC iemOpHlpCalcRmEffAddr(PVMCPU pVCpu, uint8_t bRm, uint8_t cbImm, PRTGCPTR pGCPtrEff)
{
    Log5(("iemOpHlpCalcRmEffAddr: bRm=%#x\n", bRm));
    PCCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
# define SET_SS_DEF() \
    do \
    { \
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SEG_MASK)) \
            pVCpu->iem.s.iEffSeg = X86_SREG_SS; \
    } while (0)

    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
/** @todo Check the effective address size crap! */
        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)
        {
            uint16_t u16EffAddr;

            /* Handle the disp16 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
                IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);
            else
            {
                /* Get the displacment. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:  u16EffAddr = 0;                             break;
                    case 1:  IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16EffAddr); break;
                    case 2:  IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);       break;
                    default: AssertFailedReturn(VERR_IEM_IPE_1); /* (caller checked for these) */
                }

                /* Add the base and index registers to the disp. */
                switch (bRm & X86_MODRM_RM_MASK)
                {
                    case 0: u16EffAddr += pCtx->bx + pCtx->si; break;
                    case 1: u16EffAddr += pCtx->bx + pCtx->di; break;
                    case 2: u16EffAddr += pCtx->bp + pCtx->si; SET_SS_DEF(); break;
                    case 3: u16EffAddr += pCtx->bp + pCtx->di; SET_SS_DEF(); break;
                    case 4: u16EffAddr += pCtx->si;            break;
                    case 5: u16EffAddr += pCtx->di;            break;
                    case 6: u16EffAddr += pCtx->bp;            SET_SS_DEF(); break;
                    case 7: u16EffAddr += pCtx->bx;            break;
                }
            }

            *pGCPtrEff = u16EffAddr;
        }
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            uint32_t u32EffAddr;

            /* Handle the disp32 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
                IEM_OPCODE_GET_NEXT_U32(&u32EffAddr);
            else
            {
                /* Get the register (or SIB) value. */
                switch ((bRm & X86_MODRM_RM_MASK))
                {
                    case 0: u32EffAddr = pCtx->eax; break;
                    case 1: u32EffAddr = pCtx->ecx; break;
                    case 2: u32EffAddr = pCtx->edx; break;
                    case 3: u32EffAddr = pCtx->ebx; break;
                    case 4: /* SIB */
                    {
                        uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                        /* Get the index and scale it. */
                        switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
                        {
                            case 0: u32EffAddr = pCtx->eax; break;
                            case 1: u32EffAddr = pCtx->ecx; break;
                            case 2: u32EffAddr = pCtx->edx; break;
                            case 3: u32EffAddr = pCtx->ebx; break;
                            case 4: u32EffAddr = 0; /*none */ break;
                            case 5: u32EffAddr = pCtx->ebp; break;
                            case 6: u32EffAddr = pCtx->esi; break;
                            case 7: u32EffAddr = pCtx->edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                        /* add base */
                        switch (bSib & X86_SIB_BASE_MASK)
                        {
                            case 0: u32EffAddr += pCtx->eax; break;
                            case 1: u32EffAddr += pCtx->ecx; break;
                            case 2: u32EffAddr += pCtx->edx; break;
                            case 3: u32EffAddr += pCtx->ebx; break;
                            case 4: u32EffAddr += pCtx->esp; SET_SS_DEF(); break;
                            case 5:
                                if ((bRm & X86_MODRM_MOD_MASK) != 0)
                                {
                                    u32EffAddr += pCtx->ebp;
                                    SET_SS_DEF();
                                }
                                else
                                {
                                    uint32_t u32Disp;
                                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                    u32EffAddr += u32Disp;
                                }
                                break;
                            case 6: u32EffAddr += pCtx->esi; break;
                            case 7: u32EffAddr += pCtx->edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        break;
                    }
                    case 5: u32EffAddr = pCtx->ebp; SET_SS_DEF(); break;
                    case 6: u32EffAddr = pCtx->esi; break;
                    case 7: u32EffAddr = pCtx->edi; break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }

                /* Get and add the displacement. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:
                        break;
                    case 1:
                    {
                        int8_t i8Disp; IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                        u32EffAddr += i8Disp;
                        break;
                    }
                    case 2:
                    {
                        uint32_t u32Disp; IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                        u32EffAddr += u32Disp;
                        break;
                    }
                    default:
                        AssertFailedReturn(VERR_IEM_IPE_2); /* (caller checked for these) */
                }

            }
            if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)
                *pGCPtrEff = u32EffAddr;
            else
            {
                Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT);
                *pGCPtrEff = u32EffAddr & UINT16_MAX;
            }
        }
    }
    else
    {
        uint64_t u64EffAddr;

        /* Handle the rip+disp32 form with no registers first. */
        if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
        {
            IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64EffAddr);
            u64EffAddr += pCtx->rip + IEM_GET_INSTR_LEN(pVCpu) + cbImm;
        }
        else
        {
            /* Get the register (or SIB) value. */
            switch ((bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB)
            {
                case  0: u64EffAddr = pCtx->rax; break;
                case  1: u64EffAddr = pCtx->rcx; break;
                case  2: u64EffAddr = pCtx->rdx; break;
                case  3: u64EffAddr = pCtx->rbx; break;
                case  5: u64EffAddr = pCtx->rbp; SET_SS_DEF(); break;
                case  6: u64EffAddr = pCtx->rsi; break;
                case  7: u64EffAddr = pCtx->rdi; break;
                case  8: u64EffAddr = pCtx->r8;  break;
                case  9: u64EffAddr = pCtx->r9;  break;
                case 10: u64EffAddr = pCtx->r10; break;
                case 11: u64EffAddr = pCtx->r11; break;
                case 13: u64EffAddr = pCtx->r13; break;
                case 14: u64EffAddr = pCtx->r14; break;
                case 15: u64EffAddr = pCtx->r15; break;
                /* SIB */
                case 4:
                case 12:
                {
                    uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                    /* Get the index and scale it. */
                    switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | pVCpu->iem.s.uRexIndex)
                    {
                        case  0: u64EffAddr = pCtx->rax; break;
                        case  1: u64EffAddr = pCtx->rcx; break;
                        case  2: u64EffAddr = pCtx->rdx; break;
                        case  3: u64EffAddr = pCtx->rbx; break;
                        case  4: u64EffAddr = 0; /*none */ break;
                        case  5: u64EffAddr = pCtx->rbp; break;
                        case  6: u64EffAddr = pCtx->rsi; break;
                        case  7: u64EffAddr = pCtx->rdi; break;
                        case  8: u64EffAddr = pCtx->r8;  break;
                        case  9: u64EffAddr = pCtx->r9;  break;
                        case 10: u64EffAddr = pCtx->r10; break;
                        case 11: u64EffAddr = pCtx->r11; break;
                        case 12: u64EffAddr = pCtx->r12; break;
                        case 13: u64EffAddr = pCtx->r13; break;
                        case 14: u64EffAddr = pCtx->r14; break;
                        case 15: u64EffAddr = pCtx->r15; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                    /* add base */
                    switch ((bSib & X86_SIB_BASE_MASK) | pVCpu->iem.s.uRexB)
                    {
                        case  0: u64EffAddr += pCtx->rax; break;
                        case  1: u64EffAddr += pCtx->rcx; break;
                        case  2: u64EffAddr += pCtx->rdx; break;
                        case  3: u64EffAddr += pCtx->rbx; break;
                        case  4: u64EffAddr += pCtx->rsp; SET_SS_DEF(); break;
                        case  6: u64EffAddr += pCtx->rsi; break;
                        case  7: u64EffAddr += pCtx->rdi; break;
                        case  8: u64EffAddr += pCtx->r8;  break;
                        case  9: u64EffAddr += pCtx->r9;  break;
                        case 10: u64EffAddr += pCtx->r10; break;
                        case 11: u64EffAddr += pCtx->r11; break;
                        case 12: u64EffAddr += pCtx->r12; break;
                        case 14: u64EffAddr += pCtx->r14; break;
                        case 15: u64EffAddr += pCtx->r15; break;
                        /* complicated encodings */
                        case 5:
                        case 13:
                            if ((bRm & X86_MODRM_MOD_MASK) != 0)
                            {
                                if (!pVCpu->iem.s.uRexB)
                                {
                                    u64EffAddr += pCtx->rbp;
                                    SET_SS_DEF();
                                }
                                else
                                    u64EffAddr += pCtx->r13;
                            }
                            else
                            {
                                uint32_t u32Disp;
                                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                u64EffAddr += (int32_t)u32Disp;
                            }
                            break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }

            /* Get and add the displacement. */
            switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
            {
                case 0:
                    break;
                case 1:
                {
                    int8_t i8Disp;
                    IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                    u64EffAddr += i8Disp;
                    break;
                }
                case 2:
                {
                    uint32_t u32Disp;
                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                    u64EffAddr += (int32_t)u32Disp;
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* (caller checked for these) */
            }

        }

        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)
            *pGCPtrEff = u64EffAddr;
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            *pGCPtrEff = u64EffAddr & UINT32_MAX;
        }
    }

    Log5(("iemOpHlpCalcRmEffAddr: EffAddr=%#010RGv\n", *pGCPtrEff));
    return VINF_SUCCESS;
}


/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   cbImm               The size of any immediate following the
 *                              effective address opcode bytes. Important for
 *                              RIP relative addressing.
 * @param   pGCPtrEff           Where to return the effective address.
 * @param   offRsp              RSP displacement.
 */
IEM_STATIC VBOXSTRICTRC iemOpHlpCalcRmEffAddrEx(PVMCPU pVCpu, uint8_t bRm, uint8_t cbImm, PRTGCPTR pGCPtrEff, int8_t offRsp)
{
    Log5(("iemOpHlpCalcRmEffAddr: bRm=%#x\n", bRm));
    PCCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
# define SET_SS_DEF() \
    do \
    { \
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SEG_MASK)) \
            pVCpu->iem.s.iEffSeg = X86_SREG_SS; \
    } while (0)

    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
/** @todo Check the effective address size crap! */
        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)
        {
            uint16_t u16EffAddr;

            /* Handle the disp16 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
                IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);
            else
            {
                /* Get the displacment. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:  u16EffAddr = 0;                             break;
                    case 1:  IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16EffAddr); break;
                    case 2:  IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);       break;
                    default: AssertFailedReturn(VERR_IEM_IPE_1); /* (caller checked for these) */
                }

                /* Add the base and index registers to the disp. */
                switch (bRm & X86_MODRM_RM_MASK)
                {
                    case 0: u16EffAddr += pCtx->bx + pCtx->si; break;
                    case 1: u16EffAddr += pCtx->bx + pCtx->di; break;
                    case 2: u16EffAddr += pCtx->bp + pCtx->si; SET_SS_DEF(); break;
                    case 3: u16EffAddr += pCtx->bp + pCtx->di; SET_SS_DEF(); break;
                    case 4: u16EffAddr += pCtx->si;            break;
                    case 5: u16EffAddr += pCtx->di;            break;
                    case 6: u16EffAddr += pCtx->bp;            SET_SS_DEF(); break;
                    case 7: u16EffAddr += pCtx->bx;            break;
                }
            }

            *pGCPtrEff = u16EffAddr;
        }
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            uint32_t u32EffAddr;

            /* Handle the disp32 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
                IEM_OPCODE_GET_NEXT_U32(&u32EffAddr);
            else
            {
                /* Get the register (or SIB) value. */
                switch ((bRm & X86_MODRM_RM_MASK))
                {
                    case 0: u32EffAddr = pCtx->eax; break;
                    case 1: u32EffAddr = pCtx->ecx; break;
                    case 2: u32EffAddr = pCtx->edx; break;
                    case 3: u32EffAddr = pCtx->ebx; break;
                    case 4: /* SIB */
                    {
                        uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                        /* Get the index and scale it. */
                        switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
                        {
                            case 0: u32EffAddr = pCtx->eax; break;
                            case 1: u32EffAddr = pCtx->ecx; break;
                            case 2: u32EffAddr = pCtx->edx; break;
                            case 3: u32EffAddr = pCtx->ebx; break;
                            case 4: u32EffAddr = 0; /*none */ break;
                            case 5: u32EffAddr = pCtx->ebp; break;
                            case 6: u32EffAddr = pCtx->esi; break;
                            case 7: u32EffAddr = pCtx->edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                        /* add base */
                        switch (bSib & X86_SIB_BASE_MASK)
                        {
                            case 0: u32EffAddr += pCtx->eax; break;
                            case 1: u32EffAddr += pCtx->ecx; break;
                            case 2: u32EffAddr += pCtx->edx; break;
                            case 3: u32EffAddr += pCtx->ebx; break;
                            case 4:
                                u32EffAddr += pCtx->esp + offRsp;
                                SET_SS_DEF();
                                break;
                            case 5:
                                if ((bRm & X86_MODRM_MOD_MASK) != 0)
                                {
                                    u32EffAddr += pCtx->ebp;
                                    SET_SS_DEF();
                                }
                                else
                                {
                                    uint32_t u32Disp;
                                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                    u32EffAddr += u32Disp;
                                }
                                break;
                            case 6: u32EffAddr += pCtx->esi; break;
                            case 7: u32EffAddr += pCtx->edi; break;
                            IEM_NOT_REACHED_DEFAULT_CASE_RET();
                        }
                        break;
                    }
                    case 5: u32EffAddr = pCtx->ebp; SET_SS_DEF(); break;
                    case 6: u32EffAddr = pCtx->esi; break;
                    case 7: u32EffAddr = pCtx->edi; break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET();
                }

                /* Get and add the displacement. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:
                        break;
                    case 1:
                    {
                        int8_t i8Disp; IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                        u32EffAddr += i8Disp;
                        break;
                    }
                    case 2:
                    {
                        uint32_t u32Disp; IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                        u32EffAddr += u32Disp;
                        break;
                    }
                    default:
                        AssertFailedReturn(VERR_IEM_IPE_2); /* (caller checked for these) */
                }

            }
            if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)
                *pGCPtrEff = u32EffAddr;
            else
            {
                Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT);
                *pGCPtrEff = u32EffAddr & UINT16_MAX;
            }
        }
    }
    else
    {
        uint64_t u64EffAddr;

        /* Handle the rip+disp32 form with no registers first. */
        if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
        {
            IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64EffAddr);
            u64EffAddr += pCtx->rip + IEM_GET_INSTR_LEN(pVCpu) + cbImm;
        }
        else
        {
            /* Get the register (or SIB) value. */
            switch ((bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB)
            {
                case  0: u64EffAddr = pCtx->rax; break;
                case  1: u64EffAddr = pCtx->rcx; break;
                case  2: u64EffAddr = pCtx->rdx; break;
                case  3: u64EffAddr = pCtx->rbx; break;
                case  5: u64EffAddr = pCtx->rbp; SET_SS_DEF(); break;
                case  6: u64EffAddr = pCtx->rsi; break;
                case  7: u64EffAddr = pCtx->rdi; break;
                case  8: u64EffAddr = pCtx->r8;  break;
                case  9: u64EffAddr = pCtx->r9;  break;
                case 10: u64EffAddr = pCtx->r10; break;
                case 11: u64EffAddr = pCtx->r11; break;
                case 13: u64EffAddr = pCtx->r13; break;
                case 14: u64EffAddr = pCtx->r14; break;
                case 15: u64EffAddr = pCtx->r15; break;
                /* SIB */
                case 4:
                case 12:
                {
                    uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                    /* Get the index and scale it. */
                    switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | pVCpu->iem.s.uRexIndex)
                    {
                        case  0: u64EffAddr = pCtx->rax; break;
                        case  1: u64EffAddr = pCtx->rcx; break;
                        case  2: u64EffAddr = pCtx->rdx; break;
                        case  3: u64EffAddr = pCtx->rbx; break;
                        case  4: u64EffAddr = 0; /*none */ break;
                        case  5: u64EffAddr = pCtx->rbp; break;
                        case  6: u64EffAddr = pCtx->rsi; break;
                        case  7: u64EffAddr = pCtx->rdi; break;
                        case  8: u64EffAddr = pCtx->r8;  break;
                        case  9: u64EffAddr = pCtx->r9;  break;
                        case 10: u64EffAddr = pCtx->r10; break;
                        case 11: u64EffAddr = pCtx->r11; break;
                        case 12: u64EffAddr = pCtx->r12; break;
                        case 13: u64EffAddr = pCtx->r13; break;
                        case 14: u64EffAddr = pCtx->r14; break;
                        case 15: u64EffAddr = pCtx->r15; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                    /* add base */
                    switch ((bSib & X86_SIB_BASE_MASK) | pVCpu->iem.s.uRexB)
                    {
                        case  0: u64EffAddr += pCtx->rax; break;
                        case  1: u64EffAddr += pCtx->rcx; break;
                        case  2: u64EffAddr += pCtx->rdx; break;
                        case  3: u64EffAddr += pCtx->rbx; break;
                        case  4: u64EffAddr += pCtx->rsp + offRsp; SET_SS_DEF(); break;
                        case  6: u64EffAddr += pCtx->rsi; break;
                        case  7: u64EffAddr += pCtx->rdi; break;
                        case  8: u64EffAddr += pCtx->r8;  break;
                        case  9: u64EffAddr += pCtx->r9;  break;
                        case 10: u64EffAddr += pCtx->r10; break;
                        case 11: u64EffAddr += pCtx->r11; break;
                        case 12: u64EffAddr += pCtx->r12; break;
                        case 14: u64EffAddr += pCtx->r14; break;
                        case 15: u64EffAddr += pCtx->r15; break;
                        /* complicated encodings */
                        case 5:
                        case 13:
                            if ((bRm & X86_MODRM_MOD_MASK) != 0)
                            {
                                if (!pVCpu->iem.s.uRexB)
                                {
                                    u64EffAddr += pCtx->rbp;
                                    SET_SS_DEF();
                                }
                                else
                                    u64EffAddr += pCtx->r13;
                            }
                            else
                            {
                                uint32_t u32Disp;
                                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                u64EffAddr += (int32_t)u32Disp;
                            }
                            break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET();
                    }
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET();
            }

            /* Get and add the displacement. */
            switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
            {
                case 0:
                    break;
                case 1:
                {
                    int8_t i8Disp;
                    IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                    u64EffAddr += i8Disp;
                    break;
                }
                case 2:
                {
                    uint32_t u32Disp;
                    IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                    u64EffAddr += (int32_t)u32Disp;
                    break;
                }
                IEM_NOT_REACHED_DEFAULT_CASE_RET(); /* (caller checked for these) */
            }

        }

        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)
            *pGCPtrEff = u64EffAddr;
        else
        {
            Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
            *pGCPtrEff = u64EffAddr & UINT32_MAX;
        }
    }

    Log5(("iemOpHlpCalcRmEffAddr: EffAddr=%#010RGv\n", *pGCPtrEff));
    return VINF_SUCCESS;
}


#ifdef IEM_WITH_SETJMP
/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR.
 *
 * May longjmp on internal error.
 *
 * @return  The effective address.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   cbImm               The size of any immediate following the
 *                              effective address opcode bytes. Important for
 *                              RIP relative addressing.
 */
IEM_STATIC RTGCPTR iemOpHlpCalcRmEffAddrJmp(PVMCPU pVCpu, uint8_t bRm, uint8_t cbImm)
{
    Log5(("iemOpHlpCalcRmEffAddrJmp: bRm=%#x\n", bRm));
    PCCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
# define SET_SS_DEF() \
    do \
    { \
        if (!(pVCpu->iem.s.fPrefixes & IEM_OP_PRF_SEG_MASK)) \
            pVCpu->iem.s.iEffSeg = X86_SREG_SS; \
    } while (0)

    if (pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT)
    {
/** @todo Check the effective address size crap! */
        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)
        {
            uint16_t u16EffAddr;

            /* Handle the disp16 form with no registers first. */
            if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
                IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);
            else
            {
                /* Get the displacment. */
                switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
                {
                    case 0:  u16EffAddr = 0;                             break;
                    case 1:  IEM_OPCODE_GET_NEXT_S8_SX_U16(&u16EffAddr); break;
                    case 2:  IEM_OPCODE_GET_NEXT_U16(&u16EffAddr);       break;
                    default: AssertFailedStmt(longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VERR_IEM_IPE_1)); /* (caller checked for these) */
                }

                /* Add the base and index registers to the disp. */
                switch (bRm & X86_MODRM_RM_MASK)
                {
                    case 0: u16EffAddr += pCtx->bx + pCtx->si; break;
                    case 1: u16EffAddr += pCtx->bx + pCtx->di; break;
                    case 2: u16EffAddr += pCtx->bp + pCtx->si; SET_SS_DEF(); break;
                    case 3: u16EffAddr += pCtx->bp + pCtx->di; SET_SS_DEF(); break;
                    case 4: u16EffAddr += pCtx->si;            break;
                    case 5: u16EffAddr += pCtx->di;            break;
                    case 6: u16EffAddr += pCtx->bp;            SET_SS_DEF(); break;
                    case 7: u16EffAddr += pCtx->bx;            break;
                }
            }

            Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#06RX16\n", u16EffAddr));
            return u16EffAddr;
        }

        Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
        uint32_t u32EffAddr;

        /* Handle the disp32 form with no registers first. */
        if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
            IEM_OPCODE_GET_NEXT_U32(&u32EffAddr);
        else
        {
            /* Get the register (or SIB) value. */
            switch ((bRm & X86_MODRM_RM_MASK))
            {
                case 0: u32EffAddr = pCtx->eax; break;
                case 1: u32EffAddr = pCtx->ecx; break;
                case 2: u32EffAddr = pCtx->edx; break;
                case 3: u32EffAddr = pCtx->ebx; break;
                case 4: /* SIB */
                {
                    uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                    /* Get the index and scale it. */
                    switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
                    {
                        case 0: u32EffAddr = pCtx->eax; break;
                        case 1: u32EffAddr = pCtx->ecx; break;
                        case 2: u32EffAddr = pCtx->edx; break;
                        case 3: u32EffAddr = pCtx->ebx; break;
                        case 4: u32EffAddr = 0; /*none */ break;
                        case 5: u32EffAddr = pCtx->ebp; break;
                        case 6: u32EffAddr = pCtx->esi; break;
                        case 7: u32EffAddr = pCtx->edi; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                    }
                    u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                    /* add base */
                    switch (bSib & X86_SIB_BASE_MASK)
                    {
                        case 0: u32EffAddr += pCtx->eax; break;
                        case 1: u32EffAddr += pCtx->ecx; break;
                        case 2: u32EffAddr += pCtx->edx; break;
                        case 3: u32EffAddr += pCtx->ebx; break;
                        case 4: u32EffAddr += pCtx->esp; SET_SS_DEF(); break;
                        case 5:
                            if ((bRm & X86_MODRM_MOD_MASK) != 0)
                            {
                                u32EffAddr += pCtx->ebp;
                                SET_SS_DEF();
                            }
                            else
                            {
                                uint32_t u32Disp;
                                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                                u32EffAddr += u32Disp;
                            }
                            break;
                        case 6: u32EffAddr += pCtx->esi; break;
                        case 7: u32EffAddr += pCtx->edi; break;
                        IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                    }
                    break;
                }
                case 5: u32EffAddr = pCtx->ebp; SET_SS_DEF(); break;
                case 6: u32EffAddr = pCtx->esi; break;
                case 7: u32EffAddr = pCtx->edi; break;
                IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
            }

            /* Get and add the displacement. */
            switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
            {
                case 0:
                    break;
                case 1:
                {
                    int8_t i8Disp; IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                    u32EffAddr += i8Disp;
                    break;
                }
                case 2:
                {
                    uint32_t u32Disp; IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                    u32EffAddr += u32Disp;
                    break;
                }
                default:
                    AssertFailedStmt(longjmp(*pVCpu->iem.s.CTX_SUFF(pJmpBuf), VERR_IEM_IPE_2)); /* (caller checked for these) */
            }
        }

        if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)
        {
            Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#010RX32\n", u32EffAddr));
            return u32EffAddr;
        }
        Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT);
        Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#06RX32\n", u32EffAddr & UINT16_MAX));
        return u32EffAddr & UINT16_MAX;
    }

    uint64_t u64EffAddr;

    /* Handle the rip+disp32 form with no registers first. */
    if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
    {
        IEM_OPCODE_GET_NEXT_S32_SX_U64(&u64EffAddr);
        u64EffAddr += pCtx->rip + IEM_GET_INSTR_LEN(pVCpu) + cbImm;
    }
    else
    {
        /* Get the register (or SIB) value. */
        switch ((bRm & X86_MODRM_RM_MASK) | pVCpu->iem.s.uRexB)
        {
            case  0: u64EffAddr = pCtx->rax; break;
            case  1: u64EffAddr = pCtx->rcx; break;
            case  2: u64EffAddr = pCtx->rdx; break;
            case  3: u64EffAddr = pCtx->rbx; break;
            case  5: u64EffAddr = pCtx->rbp; SET_SS_DEF(); break;
            case  6: u64EffAddr = pCtx->rsi; break;
            case  7: u64EffAddr = pCtx->rdi; break;
            case  8: u64EffAddr = pCtx->r8;  break;
            case  9: u64EffAddr = pCtx->r9;  break;
            case 10: u64EffAddr = pCtx->r10; break;
            case 11: u64EffAddr = pCtx->r11; break;
            case 13: u64EffAddr = pCtx->r13; break;
            case 14: u64EffAddr = pCtx->r14; break;
            case 15: u64EffAddr = pCtx->r15; break;
            /* SIB */
            case 4:
            case 12:
            {
                uint8_t bSib; IEM_OPCODE_GET_NEXT_U8(&bSib);

                /* Get the index and scale it. */
                switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | pVCpu->iem.s.uRexIndex)
                {
                    case  0: u64EffAddr = pCtx->rax; break;
                    case  1: u64EffAddr = pCtx->rcx; break;
                    case  2: u64EffAddr = pCtx->rdx; break;
                    case  3: u64EffAddr = pCtx->rbx; break;
                    case  4: u64EffAddr = 0; /*none */ break;
                    case  5: u64EffAddr = pCtx->rbp; break;
                    case  6: u64EffAddr = pCtx->rsi; break;
                    case  7: u64EffAddr = pCtx->rdi; break;
                    case  8: u64EffAddr = pCtx->r8;  break;
                    case  9: u64EffAddr = pCtx->r9;  break;
                    case 10: u64EffAddr = pCtx->r10; break;
                    case 11: u64EffAddr = pCtx->r11; break;
                    case 12: u64EffAddr = pCtx->r12; break;
                    case 13: u64EffAddr = pCtx->r13; break;
                    case 14: u64EffAddr = pCtx->r14; break;
                    case 15: u64EffAddr = pCtx->r15; break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                }
                u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                /* add base */
                switch ((bSib & X86_SIB_BASE_MASK) | pVCpu->iem.s.uRexB)
                {
                    case  0: u64EffAddr += pCtx->rax; break;
                    case  1: u64EffAddr += pCtx->rcx; break;
                    case  2: u64EffAddr += pCtx->rdx; break;
                    case  3: u64EffAddr += pCtx->rbx; break;
                    case  4: u64EffAddr += pCtx->rsp; SET_SS_DEF(); break;
                    case  6: u64EffAddr += pCtx->rsi; break;
                    case  7: u64EffAddr += pCtx->rdi; break;
                    case  8: u64EffAddr += pCtx->r8;  break;
                    case  9: u64EffAddr += pCtx->r9;  break;
                    case 10: u64EffAddr += pCtx->r10; break;
                    case 11: u64EffAddr += pCtx->r11; break;
                    case 12: u64EffAddr += pCtx->r12; break;
                    case 14: u64EffAddr += pCtx->r14; break;
                    case 15: u64EffAddr += pCtx->r15; break;
                    /* complicated encodings */
                    case 5:
                    case 13:
                        if ((bRm & X86_MODRM_MOD_MASK) != 0)
                        {
                            if (!pVCpu->iem.s.uRexB)
                            {
                                u64EffAddr += pCtx->rbp;
                                SET_SS_DEF();
                            }
                            else
                                u64EffAddr += pCtx->r13;
                        }
                        else
                        {
                            uint32_t u32Disp;
                            IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                            u64EffAddr += (int32_t)u32Disp;
                        }
                        break;
                    IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
                }
                break;
            }
            IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX);
        }

        /* Get and add the displacement. */
        switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
        {
            case 0:
                break;
            case 1:
            {
                int8_t i8Disp;
                IEM_OPCODE_GET_NEXT_S8(&i8Disp);
                u64EffAddr += i8Disp;
                break;
            }
            case 2:
            {
                uint32_t u32Disp;
                IEM_OPCODE_GET_NEXT_U32(&u32Disp);
                u64EffAddr += (int32_t)u32Disp;
                break;
            }
            IEM_NOT_REACHED_DEFAULT_CASE_RET2(RTGCPTR_MAX); /* (caller checked for these) */
        }

    }

    if (pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)
    {
        Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#010RGv\n", u64EffAddr));
        return u64EffAddr;
    }
    Assert(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT);
    Log5(("iemOpHlpCalcRmEffAddrJmp: EffAddr=%#010RGv\n", u64EffAddr & UINT32_MAX));
    return u64EffAddr & UINT32_MAX;
}
#endif /* IEM_WITH_SETJMP */


/** @}  */



/*
 * Include the instructions
 */
#include "IEMAllInstructions.cpp.h"




#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)

/**
 * Sets up execution verification mode.
 */
IEM_STATIC void iemExecVerificationModeSetup(PVMCPU pVCpu)
{
    PVMCPU   pVCpu   = pVCpu;
    PCPUMCTX pOrgCtx = IEM_GET_CTX(pVCpu);

    /*
     * Always note down the address of the current instruction.
     */
    pVCpu->iem.s.uOldCs  = pOrgCtx->cs.Sel;
    pVCpu->iem.s.uOldRip = pOrgCtx->rip;

    /*
     * Enable verification and/or logging.
     */
    bool fNewNoRem = !LogIs6Enabled(); /* logging triggers the no-rem/rem verification stuff */;
    if (    fNewNoRem
        && (   0
#if 0 /* auto enable on first paged protected mode interrupt */
            || (   pOrgCtx->eflags.Bits.u1IF
                && (pOrgCtx->cr0 & (X86_CR0_PE | X86_CR0_PG)) == (X86_CR0_PE | X86_CR0_PG)
                && TRPMHasTrap(pVCpu)
                && EMGetInhibitInterruptsPC(pVCpu) != pOrgCtx->rip) )
#endif
#if 0
            || (   pOrgCtx->cs  == 0x10
                && (   pOrgCtx->rip == 0x90119e3e
                    || pOrgCtx->rip == 0x901d9810)
#endif
#if 0 /* Auto enable DSL - FPU stuff. */
            || (   pOrgCtx->cs  == 0x10
                && (//   pOrgCtx->rip == 0xc02ec07f
                    //|| pOrgCtx->rip == 0xc02ec082
                    //|| pOrgCtx->rip == 0xc02ec0c9
                       0
                    || pOrgCtx->rip == 0x0c010e7c4   /* fxsave */ ) )
#endif
#if 0 /* Auto enable DSL - fstp st0 stuff. */
            || (pOrgCtx->cs.Sel  == 0x23  pOrgCtx->rip == 0x804aff7)
#endif
#if 0
            || pOrgCtx->rip == 0x9022bb3a
#endif
#if 0
            || (pOrgCtx->cs.Sel == 0x58 && pOrgCtx->rip == 0x3be) /* NT4SP1 sidt/sgdt in early loader code */
#endif
#if 0
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8013ec28) /* NT4SP1 first str (early boot) */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x80119e3f) /* NT4SP1 second str (early boot) */
#endif
#if 0 /* NT4SP1 - later on the blue screen, things goes wrong... */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8010a5df)
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8013a7c4)
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8013a7d2)
#endif
#if 0 /* NT4SP1 - xadd early boot. */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8019cf0f)
#endif
#if 0 /* NT4SP1 - wrmsr (intel MSR). */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8011a6d4)
#endif
#if 0 /* NT4SP1 - cmpxchg (AMD). */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x801684c1)
#endif
#if 0 /* NT4SP1 - fnstsw + 2 (AMD). */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x801c6b88+2)
#endif
#if 0 /* NT4SP1 - iret to v8086 -- too generic a place? (N/A with GAs installed) */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8013bd5d)

#endif
#if 0 /* NT4SP1 - iret to v8086 (executing edlin) */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8013b609)

#endif
#if 0 /* NT4SP1 - frstor [ecx] */
            || (pOrgCtx->cs.Sel == 8 && pOrgCtx->rip == 0x8013d11f)
#endif
#if 0 /* xxxxxx - All long mode code. */
            || (pOrgCtx->msrEFER & MSR_K6_EFER_LMA)
#endif
#if 0 /* rep movsq linux 3.7 64-bit boot. */
            || (pOrgCtx->rip == 0x0000000000100241)
#endif
#if 0 /* linux 3.7 64-bit boot - '000000000215e240'. */
            || (pOrgCtx->rip == 0x000000000215e240)
#endif
#if 0 /* DOS's size-overridden iret to v8086. */
            || (pOrgCtx->rip == 0x427 && pOrgCtx->cs.Sel == 0xb8)
#endif
           )
       )
    {
        RTLogGroupSettings(NULL, "iem.eo.l6.l2");
        RTLogFlags(NULL, "enabled");
        fNewNoRem = false;
    }
    if (fNewNoRem != pVCpu->iem.s.fNoRem)
    {
        pVCpu->iem.s.fNoRem = fNewNoRem;
        if (!fNewNoRem)
        {
            LogAlways(("Enabling verification mode!\n"));
            CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_ALL);
        }
        else
            LogAlways(("Disabling verification mode!\n"));
    }

    /*
     * Switch state.
     */
    if (IEM_VERIFICATION_ENABLED(pVCpu))
    {
        static CPUMCTX  s_DebugCtx; /* Ugly! */

        s_DebugCtx = *pOrgCtx;
        IEM_GET_CTX(pVCpu) = &s_DebugCtx;
    }

    /*
     * See if there is an interrupt pending in TRPM and inject it if we can.
     */
    pVCpu->iem.s.uInjectCpl = UINT8_MAX;
    /** @todo Maybe someday we can centralize this under CPUMCanInjectInterrupt()? */
#if defined(VBOX_WITH_NESTED_HWVIRT)
    bool fIntrEnabled = pOrgCtx->hwvirt.svm.fGif;
    if (fIntrEnabled)
    {
        if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
            fIntrEnabled = CPUMCanSvmNstGstTakePhysIntr(pCtx);
        else
            fIntrEnabled = pOrgCtx->eflags.Bits.u1IF;
    }
#else
    bool fIntrEnabled = pOrgCtx->eflags.Bits.u1IF;
#endif
    if (   fIntrEnabled
        && TRPMHasTrap(pVCpu)
        && EMGetInhibitInterruptsPC(pVCpu) != pOrgCtx->rip)
    {
        uint8_t     u8TrapNo;
        TRPMEVENT   enmType;
        RTGCUINT    uErrCode;
        RTGCPTR     uCr2;
        int rc2 = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrCode, &uCr2, NULL /* pu8InstLen */); AssertRC(rc2);
        IEMInjectTrap(pVCpu, u8TrapNo, enmType, (uint16_t)uErrCode, uCr2, 0 /* cbInstr */);
        if (!IEM_VERIFICATION_ENABLED(pVCpu))
            TRPMResetTrap(pVCpu);
        pVCpu->iem.s.uInjectCpl = pVCpu->iem.s.uCpl;
    }

    /*
     * Reset the counters.
     */
    pVCpu->iem.s.cIOReads    = 0;
    pVCpu->iem.s.cIOWrites   = 0;
    pVCpu->iem.s.fIgnoreRaxRdx = false;
    pVCpu->iem.s.fOverlappingMovs = false;
    pVCpu->iem.s.fProblematicMemory = false;
    pVCpu->iem.s.fUndefinedEFlags = 0;

    if (IEM_VERIFICATION_ENABLED(pVCpu))
    {
        /*
         * Free all verification records.
         */
        PIEMVERIFYEVTREC pEvtRec = pVCpu->iem.s.pIemEvtRecHead;
        pVCpu->iem.s.pIemEvtRecHead = NULL;
        pVCpu->iem.s.ppIemEvtRecNext = &pVCpu->iem.s.pIemEvtRecHead;
        do
        {
            while (pEvtRec)
            {
                PIEMVERIFYEVTREC pNext = pEvtRec->pNext;
                pEvtRec->pNext = pVCpu->iem.s.pFreeEvtRec;
                pVCpu->iem.s.pFreeEvtRec = pEvtRec;
                pEvtRec = pNext;
            }
            pEvtRec = pVCpu->iem.s.pOtherEvtRecHead;
            pVCpu->iem.s.pOtherEvtRecHead = NULL;
            pVCpu->iem.s.ppOtherEvtRecNext = &pVCpu->iem.s.pOtherEvtRecHead;
        } while (pEvtRec);
    }
}


/**
 * Allocate an event record.
 * @returns Pointer to a record.
 */
IEM_STATIC PIEMVERIFYEVTREC iemVerifyAllocRecord(PVMCPU pVCpu)
{
    if (!IEM_VERIFICATION_ENABLED(pVCpu))
        return NULL;

    PIEMVERIFYEVTREC pEvtRec = pVCpu->iem.s.pFreeEvtRec;
    if (pEvtRec)
        pVCpu->iem.s.pFreeEvtRec = pEvtRec->pNext;
    else
    {
        if (!pVCpu->iem.s.ppIemEvtRecNext)
            return NULL; /* Too early (fake PCIBIOS), ignore notification. */

        pEvtRec = (PIEMVERIFYEVTREC)MMR3HeapAlloc(pVCpu->CTX_SUFF(pVM), MM_TAG_EM /* lazy bird*/, sizeof(*pEvtRec));
        if (!pEvtRec)
            return NULL;
    }
    pEvtRec->enmEvent = IEMVERIFYEVENT_INVALID;
    pEvtRec->pNext    = NULL;
    return pEvtRec;
}


/**
 * IOMMMIORead notification.
 */
VMM_INT_DECL(void)   IEMNotifyMMIORead(PVM pVM, RTGCPHYS GCPhys, size_t cbValue)
{
    PVMCPU              pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        return;
    PIEMVERIFYEVTREC    pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (!pEvtRec)
        return;
    pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_READ;
    pEvtRec->u.RamRead.GCPhys  = GCPhys;
    pEvtRec->u.RamRead.cb      = (uint32_t)cbValue;
    pEvtRec->pNext = *pVCpu->iem.s.ppOtherEvtRecNext;
    *pVCpu->iem.s.ppOtherEvtRecNext = pEvtRec;
}


/**
 * IOMMMIOWrite notification.
 */
VMM_INT_DECL(void)   IEMNotifyMMIOWrite(PVM pVM, RTGCPHYS GCPhys, uint32_t u32Value, size_t cbValue)
{
    PVMCPU              pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        return;
    PIEMVERIFYEVTREC    pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (!pEvtRec)
        return;
    pEvtRec->enmEvent = IEMVERIFYEVENT_RAM_WRITE;
    pEvtRec->u.RamWrite.GCPhys   = GCPhys;
    pEvtRec->u.RamWrite.cb       = (uint32_t)cbValue;
    pEvtRec->u.RamWrite.ab[0]    = RT_BYTE1(u32Value);
    pEvtRec->u.RamWrite.ab[1]    = RT_BYTE2(u32Value);
    pEvtRec->u.RamWrite.ab[2]    = RT_BYTE3(u32Value);
    pEvtRec->u.RamWrite.ab[3]    = RT_BYTE4(u32Value);
    pEvtRec->pNext = *pVCpu->iem.s.ppOtherEvtRecNext;
    *pVCpu->iem.s.ppOtherEvtRecNext = pEvtRec;
}


/**
 * IOMIOPortRead notification.
 */
VMM_INT_DECL(void)   IEMNotifyIOPortRead(PVM pVM, RTIOPORT Port, size_t cbValue)
{
    PVMCPU              pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        return;
    PIEMVERIFYEVTREC    pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (!pEvtRec)
        return;
    pEvtRec->enmEvent = IEMVERIFYEVENT_IOPORT_READ;
    pEvtRec->u.IOPortRead.Port    = Port;
    pEvtRec->u.IOPortRead.cbValue = (uint8_t)cbValue;
    pEvtRec->pNext = *pVCpu->iem.s.ppOtherEvtRecNext;
    *pVCpu->iem.s.ppOtherEvtRecNext = pEvtRec;
}

/**
 * IOMIOPortWrite notification.
 */
VMM_INT_DECL(void)   IEMNotifyIOPortWrite(PVM pVM, RTIOPORT Port, uint32_t u32Value, size_t cbValue)
{
    PVMCPU              pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        return;
    PIEMVERIFYEVTREC    pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (!pEvtRec)
        return;
    pEvtRec->enmEvent = IEMVERIFYEVENT_IOPORT_WRITE;
    pEvtRec->u.IOPortWrite.Port     = Port;
    pEvtRec->u.IOPortWrite.cbValue  = (uint8_t)cbValue;
    pEvtRec->u.IOPortWrite.u32Value = u32Value;
    pEvtRec->pNext = *pVCpu->iem.s.ppOtherEvtRecNext;
    *pVCpu->iem.s.ppOtherEvtRecNext = pEvtRec;
}


VMM_INT_DECL(void)   IEMNotifyIOPortReadString(PVM pVM, RTIOPORT Port, void *pvDst, RTGCUINTREG cTransfers, size_t cbValue)
{
    PVMCPU              pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        return;
    PIEMVERIFYEVTREC    pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (!pEvtRec)
        return;
    pEvtRec->enmEvent = IEMVERIFYEVENT_IOPORT_STR_READ;
    pEvtRec->u.IOPortStrRead.Port       = Port;
    pEvtRec->u.IOPortStrRead.cbValue    = (uint8_t)cbValue;
    pEvtRec->u.IOPortStrRead.cTransfers = cTransfers;
    pEvtRec->pNext = *pVCpu->iem.s.ppOtherEvtRecNext;
    *pVCpu->iem.s.ppOtherEvtRecNext = pEvtRec;
}


VMM_INT_DECL(void)   IEMNotifyIOPortWriteString(PVM pVM, RTIOPORT Port, void const *pvSrc, RTGCUINTREG cTransfers, size_t cbValue)
{
    PVMCPU              pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        return;
    PIEMVERIFYEVTREC    pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (!pEvtRec)
        return;
    pEvtRec->enmEvent = IEMVERIFYEVENT_IOPORT_STR_WRITE;
    pEvtRec->u.IOPortStrWrite.Port       = Port;
    pEvtRec->u.IOPortStrWrite.cbValue    = (uint8_t)cbValue;
    pEvtRec->u.IOPortStrWrite.cTransfers = cTransfers;
    pEvtRec->pNext = *pVCpu->iem.s.ppOtherEvtRecNext;
    *pVCpu->iem.s.ppOtherEvtRecNext = pEvtRec;
}


/**
 * Fakes and records an I/O port read.
 *
 * @returns VINF_SUCCESS.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   Port                The I/O port.
 * @param   pu32Value           Where to store the fake value.
 * @param   cbValue             The size of the access.
 */
IEM_STATIC VBOXSTRICTRC iemVerifyFakeIOPortRead(PVMCPU pVCpu, RTIOPORT Port, uint32_t *pu32Value, size_t cbValue)
{
    PIEMVERIFYEVTREC pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (pEvtRec)
    {
        pEvtRec->enmEvent = IEMVERIFYEVENT_IOPORT_READ;
        pEvtRec->u.IOPortRead.Port    = Port;
        pEvtRec->u.IOPortRead.cbValue = (uint8_t)cbValue;
        pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
        *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
    }
    pVCpu->iem.s.cIOReads++;
    *pu32Value = 0xcccccccc;
    return VINF_SUCCESS;
}


/**
 * Fakes and records an I/O port write.
 *
 * @returns VINF_SUCCESS.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   Port                The I/O port.
 * @param   u32Value            The value being written.
 * @param   cbValue             The size of the access.
 */
IEM_STATIC VBOXSTRICTRC iemVerifyFakeIOPortWrite(PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue)
{
    PIEMVERIFYEVTREC pEvtRec = iemVerifyAllocRecord(pVCpu);
    if (pEvtRec)
    {
        pEvtRec->enmEvent = IEMVERIFYEVENT_IOPORT_WRITE;
        pEvtRec->u.IOPortWrite.Port     = Port;
        pEvtRec->u.IOPortWrite.cbValue  = (uint8_t)cbValue;
        pEvtRec->u.IOPortWrite.u32Value = u32Value;
        pEvtRec->pNext = *pVCpu->iem.s.ppIemEvtRecNext;
        *pVCpu->iem.s.ppIemEvtRecNext = pEvtRec;
    }
    pVCpu->iem.s.cIOWrites++;
    return VINF_SUCCESS;
}


/**
 * Used to add extra details about a stub case.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 */
IEM_STATIC void iemVerifyAssertMsg2(PVMCPU pVCpu)
{
    PCPUMCTX pCtx  = IEM_GET_CTX(pVCpu);
    PVM      pVM   = pVCpu->CTX_SUFF(pVM);
    PVMCPU   pVCpu = pVCpu;
    char szRegs[4096];
    DBGFR3RegPrintf(pVM->pUVM, pVCpu->idCpu, &szRegs[0], sizeof(szRegs),
                    "rax=%016VR{rax} rbx=%016VR{rbx} rcx=%016VR{rcx} rdx=%016VR{rdx}\n"
                    "rsi=%016VR{rsi} rdi=%016VR{rdi} r8 =%016VR{r8} r9 =%016VR{r9}\n"
                    "r10=%016VR{r10} r11=%016VR{r11} r12=%016VR{r12} r13=%016VR{r13}\n"
                    "r14=%016VR{r14} r15=%016VR{r15} %VRF{rflags}\n"
                    "rip=%016VR{rip} rsp=%016VR{rsp} rbp=%016VR{rbp}\n"
                    "cs={%04VR{cs} base=%016VR{cs_base} limit=%08VR{cs_lim} flags=%04VR{cs_attr}} cr0=%016VR{cr0}\n"
                    "ds={%04VR{ds} base=%016VR{ds_base} limit=%08VR{ds_lim} flags=%04VR{ds_attr}} cr2=%016VR{cr2}\n"
                    "es={%04VR{es} base=%016VR{es_base} limit=%08VR{es_lim} flags=%04VR{es_attr}} cr3=%016VR{cr3}\n"
                    "fs={%04VR{fs} base=%016VR{fs_base} limit=%08VR{fs_lim} flags=%04VR{fs_attr}} cr4=%016VR{cr4}\n"
                    "gs={%04VR{gs} base=%016VR{gs_base} limit=%08VR{gs_lim} flags=%04VR{gs_attr}} cr8=%016VR{cr8}\n"
                    "ss={%04VR{ss} base=%016VR{ss_base} limit=%08VR{ss_lim} flags=%04VR{ss_attr}}\n"
                    "dr0=%016VR{dr0} dr1=%016VR{dr1} dr2=%016VR{dr2} dr3=%016VR{dr3}\n"
                    "dr6=%016VR{dr6} dr7=%016VR{dr7}\n"
                    "gdtr=%016VR{gdtr_base}:%04VR{gdtr_lim}  idtr=%016VR{idtr_base}:%04VR{idtr_lim}  rflags=%08VR{rflags}\n"
                    "ldtr={%04VR{ldtr} base=%016VR{ldtr_base} limit=%08VR{ldtr_lim} flags=%08VR{ldtr_attr}}\n"
                    "tr  ={%04VR{tr} base=%016VR{tr_base} limit=%08VR{tr_lim} flags=%08VR{tr_attr}}\n"
                    "    sysenter={cs=%04VR{sysenter_cs} eip=%08VR{sysenter_eip} esp=%08VR{sysenter_esp}}\n"
                    "        efer=%016VR{efer}\n"
                    "         pat=%016VR{pat}\n"
                    "     sf_mask=%016VR{sf_mask}\n"
                    "krnl_gs_base=%016VR{krnl_gs_base}\n"
                    "       lstar=%016VR{lstar}\n"
                    "        star=%016VR{star} cstar=%016VR{cstar}\n"
                    "fcw=%04VR{fcw} fsw=%04VR{fsw} ftw=%04VR{ftw} mxcsr=%04VR{mxcsr} mxcsr_mask=%04VR{mxcsr_mask}\n"
                    );

    char szInstr1[256];
    DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pVCpu->iem.s.uOldCs, pVCpu->iem.s.uOldRip,
                       DBGF_DISAS_FLAGS_DEFAULT_MODE,
                       szInstr1, sizeof(szInstr1), NULL);
    char szInstr2[256];
    DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                       DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                       szInstr2, sizeof(szInstr2), NULL);

    RTAssertMsg2Weak("%s%s\n%s\n", szRegs, szInstr1, szInstr2);
}


/**
 * Used by iemVerifyAssertRecord and iemVerifyAssertRecords to add a record
 * dump to the assertion info.
 *
 * @param   pEvtRec         The record to dump.
 */
IEM_STATIC void iemVerifyAssertAddRecordDump(PIEMVERIFYEVTREC pEvtRec)
{
    switch (pEvtRec->enmEvent)
    {
        case IEMVERIFYEVENT_IOPORT_READ:
            RTAssertMsg2Add("I/O PORT READ from %#6x, %d bytes\n",
                            pEvtRec->u.IOPortWrite.Port,
                            pEvtRec->u.IOPortWrite.cbValue);
            break;
        case IEMVERIFYEVENT_IOPORT_WRITE:
            RTAssertMsg2Add("I/O PORT WRITE  to %#6x, %d bytes, value %#x\n",
                            pEvtRec->u.IOPortWrite.Port,
                            pEvtRec->u.IOPortWrite.cbValue,
                            pEvtRec->u.IOPortWrite.u32Value);
            break;
        case IEMVERIFYEVENT_IOPORT_STR_READ:
            RTAssertMsg2Add("I/O PORT STRING READ from %#6x, %d bytes, %#x times\n",
                            pEvtRec->u.IOPortStrWrite.Port,
                            pEvtRec->u.IOPortStrWrite.cbValue,
                            pEvtRec->u.IOPortStrWrite.cTransfers);
            break;
        case IEMVERIFYEVENT_IOPORT_STR_WRITE:
            RTAssertMsg2Add("I/O PORT STRING WRITE  to %#6x, %d bytes, %#x times\n",
                            pEvtRec->u.IOPortStrWrite.Port,
                            pEvtRec->u.IOPortStrWrite.cbValue,
                            pEvtRec->u.IOPortStrWrite.cTransfers);
            break;
        case IEMVERIFYEVENT_RAM_READ:
            RTAssertMsg2Add("RAM READ  at %RGp, %#4zx bytes\n",
                            pEvtRec->u.RamRead.GCPhys,
                            pEvtRec->u.RamRead.cb);
            break;
        case IEMVERIFYEVENT_RAM_WRITE:
            RTAssertMsg2Add("RAM WRITE at %RGp, %#4zx bytes: %.*Rhxs\n",
                            pEvtRec->u.RamWrite.GCPhys,
                            pEvtRec->u.RamWrite.cb,
                            (int)pEvtRec->u.RamWrite.cb,
                            pEvtRec->u.RamWrite.ab);
            break;
        default:
            AssertMsgFailed(("Invalid event type %d\n", pEvtRec->enmEvent));
            break;
    }
}


/**
 * Raises an assertion on the specified record, showing the given message with
 * a record dump attached.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pEvtRec1        The first record.
 * @param   pEvtRec2        The second record.
 * @param   pszMsg          The message explaining why we're asserting.
 */
IEM_STATIC void iemVerifyAssertRecords(PVMCPU pVCpu, PIEMVERIFYEVTREC pEvtRec1, PIEMVERIFYEVTREC pEvtRec2, const char *pszMsg)
{
    RTAssertMsg1(pszMsg, __LINE__, __FILE__, __PRETTY_FUNCTION__);
    iemVerifyAssertAddRecordDump(pEvtRec1);
    iemVerifyAssertAddRecordDump(pEvtRec2);
    iemVerifyAssertMsg2(pVCpu);
    RTAssertPanic();
}


/**
 * Raises an assertion on the specified record, showing the given message with
 * a record dump attached.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pEvtRec1        The first record.
 * @param   pszMsg          The message explaining why we're asserting.
 */
IEM_STATIC void iemVerifyAssertRecord(PVMCPU pVCpu, PIEMVERIFYEVTREC pEvtRec, const char *pszMsg)
{
    RTAssertMsg1(pszMsg, __LINE__, __FILE__, __PRETTY_FUNCTION__);
    iemVerifyAssertAddRecordDump(pEvtRec);
    iemVerifyAssertMsg2(pVCpu);
    RTAssertPanic();
}


/**
 * Verifies a write record.
 *
 * @param   pVCpu           The cross context virtual CPU structure of the calling thread.
 * @param   pEvtRec         The write record.
 * @param   fRem            Set if REM was doing the other executing. If clear
 *                          it was HM.
 */
IEM_STATIC void iemVerifyWriteRecord(PVMCPU pVCpu, PIEMVERIFYEVTREC pEvtRec, bool fRem)
{
    uint8_t abBuf[sizeof(pEvtRec->u.RamWrite.ab)]; RT_ZERO(abBuf);
    Assert(sizeof(abBuf) >= pEvtRec->u.RamWrite.cb);
    int rc = PGMPhysSimpleReadGCPhys(pVCpu->CTX_SUFF(pVM), abBuf, pEvtRec->u.RamWrite.GCPhys, pEvtRec->u.RamWrite.cb);
    if (   RT_FAILURE(rc)
        || memcmp(abBuf, pEvtRec->u.RamWrite.ab, pEvtRec->u.RamWrite.cb) )
    {
        /* fend off ins */
        if (   !pVCpu->iem.s.cIOReads
            || pEvtRec->u.RamWrite.ab[0] != 0xcc
            || (   pEvtRec->u.RamWrite.cb != 1
                && pEvtRec->u.RamWrite.cb != 2
                && pEvtRec->u.RamWrite.cb != 4) )
        {
            /* fend off ROMs and MMIO */
            if (   pEvtRec->u.RamWrite.GCPhys - UINT32_C(0x000a0000) > UINT32_C(0x60000)
                && pEvtRec->u.RamWrite.GCPhys - UINT32_C(0xfffc0000) > UINT32_C(0x40000) )
            {
                /* fend off fxsave */
                if (pEvtRec->u.RamWrite.cb != 512)
                {
                    const char *pszWho = fRem ? "rem" : HMR3IsVmxEnabled(pVCpu->CTX_SUFF(pVM)->pUVM) ? "vmx" : "svm";
                    RTAssertMsg1(NULL, __LINE__, __FILE__, __PRETTY_FUNCTION__);
                    RTAssertMsg2Weak("Memory at %RGv differs\n", pEvtRec->u.RamWrite.GCPhys);
                    RTAssertMsg2Add("%s: %.*Rhxs\n"
                                    "iem: %.*Rhxs\n",
                                    pszWho, pEvtRec->u.RamWrite.cb, abBuf,
                                    pEvtRec->u.RamWrite.cb, pEvtRec->u.RamWrite.ab);
                    iemVerifyAssertAddRecordDump(pEvtRec);
                    iemVerifyAssertMsg2(pVCpu);
                    RTAssertPanic();
                }
            }
        }
    }

}

/**
 * Performs the post-execution verfication checks.
 */
IEM_STATIC VBOXSTRICTRC iemExecVerificationModeCheck(PVMCPU pVCpu, VBOXSTRICTRC rcStrictIem)
{
    if (!IEM_VERIFICATION_ENABLED(pVCpu))
        return rcStrictIem;

    /*
     * Switch back the state.
     */
    PCPUMCTX    pOrgCtx   = CPUMQueryGuestCtxPtr(pVCpu);
    PCPUMCTX    pDebugCtx = IEM_GET_CTX(pVCpu);
    Assert(pOrgCtx != pDebugCtx);
    IEM_GET_CTX(pVCpu) = pOrgCtx;

    /*
     * Execute the instruction in REM.
     */
    bool   fRem  = false;
    PVM    pVM   = pVCpu->CTX_SUFF(pVM);
    PVMCPU pVCpu = pVCpu;
    VBOXSTRICTRC rc = VERR_EM_CANNOT_EXEC_GUEST;
#ifdef IEM_VERIFICATION_MODE_FULL_HM
    if (   HMIsEnabled(pVM)
        && pVCpu->iem.s.cIOReads == 0
        && pVCpu->iem.s.cIOWrites == 0
        && !pVCpu->iem.s.fProblematicMemory)
    {
        uint64_t uStartRip = pOrgCtx->rip;
        unsigned iLoops = 0;
        do
        {
            rc = EMR3HmSingleInstruction(pVM, pVCpu, EM_ONE_INS_FLAGS_RIP_CHANGE);
            iLoops++;
        } while (   rc == VINF_SUCCESS
                 || (   rc == VINF_EM_DBG_STEPPED
                     && VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                     && EMGetInhibitInterruptsPC(pVCpu) == pOrgCtx->rip)
                 || (   pOrgCtx->rip != pDebugCtx->rip
                     && pVCpu->iem.s.uInjectCpl != UINT8_MAX
                     && iLoops < 8) );
        if (rc == VINF_EM_RESCHEDULE && pOrgCtx->rip != uStartRip)
            rc = VINF_SUCCESS;
    }
#endif
    if (   rc == VERR_EM_CANNOT_EXEC_GUEST
        || rc == VINF_IOM_R3_IOPORT_READ
        || rc == VINF_IOM_R3_IOPORT_WRITE
        || rc == VINF_IOM_R3_MMIO_READ
        || rc == VINF_IOM_R3_MMIO_READ_WRITE
        || rc == VINF_IOM_R3_MMIO_WRITE
        || rc == VINF_CPUM_R3_MSR_READ
        || rc == VINF_CPUM_R3_MSR_WRITE
        || rc == VINF_EM_RESCHEDULE
        )
    {
        EMRemLock(pVM);
        rc = REMR3EmulateInstruction(pVM, pVCpu);
        AssertRC(rc);
        EMRemUnlock(pVM);
        fRem = true;
    }

#  if 1 /* Skip unimplemented instructions for now. */
    if (rcStrictIem == VERR_IEM_INSTR_NOT_IMPLEMENTED)
    {
        IEM_GET_CTX(pVCpu) = pOrgCtx;
        if (rc == VINF_EM_DBG_STEPPED)
            return VINF_SUCCESS;
        return rc;
    }
#  endif

    /*
     * Compare the register states.
     */
    unsigned cDiffs = 0;
    if (memcmp(pOrgCtx, pDebugCtx, sizeof(*pDebugCtx)))
    {
        //Log(("REM and IEM ends up with different registers!\n"));
        const char *pszWho = fRem ? "rem" : HMR3IsVmxEnabled(pVM->pUVM) ? "vmx" : "svm";

#  define CHECK_FIELD(a_Field) \
    do \
    { \
        if (pOrgCtx->a_Field != pDebugCtx->a_Field) \
        { \
            switch (sizeof(pOrgCtx->a_Field)) \
            { \
                case 1: RTAssertMsg2Weak("  %8s differs - iem=%02x - %s=%02x\n", #a_Field, pDebugCtx->a_Field, pszWho, pOrgCtx->a_Field); break; \
                case 2: RTAssertMsg2Weak("  %8s differs - iem=%04x - %s=%04x\n", #a_Field, pDebugCtx->a_Field, pszWho, pOrgCtx->a_Field); break; \
                case 4: RTAssertMsg2Weak("  %8s differs - iem=%08x - %s=%08x\n", #a_Field, pDebugCtx->a_Field, pszWho, pOrgCtx->a_Field); break; \
                case 8: RTAssertMsg2Weak("  %8s differs - iem=%016llx - %s=%016llx\n", #a_Field, pDebugCtx->a_Field, pszWho, pOrgCtx->a_Field); break; \
                default: RTAssertMsg2Weak("  %8s differs\n", #a_Field); break; \
            } \
            cDiffs++; \
        } \
    } while (0)
#  define CHECK_XSTATE_FIELD(a_Field) \
    do \
    { \
        if (pOrgXState->a_Field != pDebugXState->a_Field) \
        { \
            switch (sizeof(pOrgXState->a_Field)) \
            { \
                case 1: RTAssertMsg2Weak("  %8s differs - iem=%02x - %s=%02x\n", #a_Field, pDebugXState->a_Field, pszWho, pOrgXState->a_Field); break; \
                case 2: RTAssertMsg2Weak("  %8s differs - iem=%04x - %s=%04x\n", #a_Field, pDebugXState->a_Field, pszWho, pOrgXState->a_Field); break; \
                case 4: RTAssertMsg2Weak("  %8s differs - iem=%08x - %s=%08x\n", #a_Field, pDebugXState->a_Field, pszWho, pOrgXState->a_Field); break; \
                case 8: RTAssertMsg2Weak("  %8s differs - iem=%016llx - %s=%016llx\n", #a_Field, pDebugXState->a_Field, pszWho, pOrgXState->a_Field); break; \
                default: RTAssertMsg2Weak("  %8s differs\n", #a_Field); break; \
            } \
            cDiffs++; \
        } \
    } while (0)

#  define CHECK_BIT_FIELD(a_Field) \
    do \
    { \
        if (pOrgCtx->a_Field != pDebugCtx->a_Field) \
        { \
            RTAssertMsg2Weak("  %8s differs - iem=%02x - %s=%02x\n", #a_Field, pDebugCtx->a_Field, pszWho, pOrgCtx->a_Field); \
            cDiffs++; \
        } \
    } while (0)

#  define CHECK_SEL(a_Sel) \
    do \
    { \
        CHECK_FIELD(a_Sel.Sel); \
        CHECK_FIELD(a_Sel.Attr.u); \
        CHECK_FIELD(a_Sel.u64Base); \
        CHECK_FIELD(a_Sel.u32Limit); \
        CHECK_FIELD(a_Sel.fFlags); \
    } while (0)

        PX86XSAVEAREA pOrgXState   = pOrgCtx->CTX_SUFF(pXState);
        PX86XSAVEAREA pDebugXState = pDebugCtx->CTX_SUFF(pXState);

#if 1 /* The recompiler doesn't update these the intel way. */
        if (fRem)
        {
            pOrgXState->x87.FOP        = pDebugXState->x87.FOP;
            pOrgXState->x87.FPUIP      = pDebugXState->x87.FPUIP;
            pOrgXState->x87.CS         = pDebugXState->x87.CS;
            pOrgXState->x87.Rsrvd1     = pDebugXState->x87.Rsrvd1;
            pOrgXState->x87.FPUDP      = pDebugXState->x87.FPUDP;
            pOrgXState->x87.DS         = pDebugXState->x87.DS;
            pOrgXState->x87.Rsrvd2     = pDebugXState->x87.Rsrvd2;
            //pOrgXState->x87.MXCSR_MASK = pDebugXState->x87.MXCSR_MASK;
            if ((pOrgXState->x87.FSW & X86_FSW_TOP_MASK) == (pDebugXState->x87.FSW & X86_FSW_TOP_MASK))
                pOrgXState->x87.FSW = pDebugXState->x87.FSW;
        }
#endif
        if (memcmp(&pOrgXState->x87, &pDebugXState->x87, sizeof(pDebugXState->x87)))
        {
            RTAssertMsg2Weak("  the FPU state differs\n");
            cDiffs++;
            CHECK_XSTATE_FIELD(x87.FCW);
            CHECK_XSTATE_FIELD(x87.FSW);
            CHECK_XSTATE_FIELD(x87.FTW);
            CHECK_XSTATE_FIELD(x87.FOP);
            CHECK_XSTATE_FIELD(x87.FPUIP);
            CHECK_XSTATE_FIELD(x87.CS);
            CHECK_XSTATE_FIELD(x87.Rsrvd1);
            CHECK_XSTATE_FIELD(x87.FPUDP);
            CHECK_XSTATE_FIELD(x87.DS);
            CHECK_XSTATE_FIELD(x87.Rsrvd2);
            CHECK_XSTATE_FIELD(x87.MXCSR);
            CHECK_XSTATE_FIELD(x87.MXCSR_MASK);
            CHECK_XSTATE_FIELD(x87.aRegs[0].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[0].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[1].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[1].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[2].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[2].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[3].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[3].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[4].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[4].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[5].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[5].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[6].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[6].au64[1]);
            CHECK_XSTATE_FIELD(x87.aRegs[7].au64[0]); CHECK_XSTATE_FIELD(x87.aRegs[7].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 0].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 0].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 1].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 1].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 2].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 2].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 3].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 3].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 4].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 4].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 5].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 5].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 6].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 6].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 7].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 7].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 8].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 8].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[ 9].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[ 9].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[10].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[10].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[11].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[11].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[12].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[12].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[13].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[13].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[14].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[14].au64[1]);
            CHECK_XSTATE_FIELD(x87.aXMM[15].au64[0]);  CHECK_XSTATE_FIELD(x87.aXMM[15].au64[1]);
            for (unsigned i = 0; i < RT_ELEMENTS(pOrgXState->x87.au32RsrvdRest); i++)
                CHECK_XSTATE_FIELD(x87.au32RsrvdRest[i]);
        }
        CHECK_FIELD(rip);
        uint32_t fFlagsMask = UINT32_MAX & ~pVCpu->iem.s.fUndefinedEFlags;
        if ((pOrgCtx->rflags.u & fFlagsMask) != (pDebugCtx->rflags.u & fFlagsMask))
        {
            RTAssertMsg2Weak("  rflags differs - iem=%08llx %s=%08llx\n", pDebugCtx->rflags.u, pszWho, pOrgCtx->rflags.u);
            CHECK_BIT_FIELD(rflags.Bits.u1CF);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved0);
            CHECK_BIT_FIELD(rflags.Bits.u1PF);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved1);
            CHECK_BIT_FIELD(rflags.Bits.u1AF);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved2);
            CHECK_BIT_FIELD(rflags.Bits.u1ZF);
            CHECK_BIT_FIELD(rflags.Bits.u1SF);
            CHECK_BIT_FIELD(rflags.Bits.u1TF);
            CHECK_BIT_FIELD(rflags.Bits.u1IF);
            CHECK_BIT_FIELD(rflags.Bits.u1DF);
            CHECK_BIT_FIELD(rflags.Bits.u1OF);
            CHECK_BIT_FIELD(rflags.Bits.u2IOPL);
            CHECK_BIT_FIELD(rflags.Bits.u1NT);
            CHECK_BIT_FIELD(rflags.Bits.u1Reserved3);
            if (0 && !fRem) /** @todo debug the occational clear RF flags when running against VT-x. */
                CHECK_BIT_FIELD(rflags.Bits.u1RF);
            CHECK_BIT_FIELD(rflags.Bits.u1VM);
            CHECK_BIT_FIELD(rflags.Bits.u1AC);
            CHECK_BIT_FIELD(rflags.Bits.u1VIF);
            CHECK_BIT_FIELD(rflags.Bits.u1VIP);
            CHECK_BIT_FIELD(rflags.Bits.u1ID);
        }

        if (pVCpu->iem.s.cIOReads != 1 && !pVCpu->iem.s.fIgnoreRaxRdx)
            CHECK_FIELD(rax);
        CHECK_FIELD(rcx);
        if (!pVCpu->iem.s.fIgnoreRaxRdx)
            CHECK_FIELD(rdx);
        CHECK_FIELD(rbx);
        CHECK_FIELD(rsp);
        CHECK_FIELD(rbp);
        CHECK_FIELD(rsi);
        CHECK_FIELD(rdi);
        CHECK_FIELD(r8);
        CHECK_FIELD(r9);
        CHECK_FIELD(r10);
        CHECK_FIELD(r11);
        CHECK_FIELD(r12);
        CHECK_FIELD(r13);
        CHECK_SEL(cs);
        CHECK_SEL(ss);
        CHECK_SEL(ds);
        CHECK_SEL(es);
        CHECK_SEL(fs);
        CHECK_SEL(gs);
        CHECK_FIELD(cr0);

        /* Klugde #1: REM fetches code and across the page boundrary and faults on the next page, while we execute
           the faulting instruction first: 001b:77f61ff3 66 8b 42 02   mov ax, word [edx+002h] (NT4SP1) */
        /* Kludge #2: CR2 differs slightly on cross page boundrary faults, we report the last address of the access
           while REM reports the address of the first byte on the page.  Pending investigation as to which is correct. */
        if (pOrgCtx->cr2 != pDebugCtx->cr2)
        {
            if (pVCpu->iem.s.uOldCs == 0x1b && pVCpu->iem.s.uOldRip == 0x77f61ff3 && fRem)
            { /* ignore */ }
            else if (   (pOrgCtx->cr2 & ~(uint64_t)3) == (pDebugCtx->cr2 & ~(uint64_t)3)
                     && (pOrgCtx->cr2 & PAGE_OFFSET_MASK) == 0
                     && fRem)
            { /* ignore */ }
            else
                CHECK_FIELD(cr2);
        }
        CHECK_FIELD(cr3);
        CHECK_FIELD(cr4);
        CHECK_FIELD(dr[0]);
        CHECK_FIELD(dr[1]);
        CHECK_FIELD(dr[2]);
        CHECK_FIELD(dr[3]);
        CHECK_FIELD(dr[6]);
        if (!fRem || (pOrgCtx->dr[7] & ~X86_DR7_RA1_MASK) != (pDebugCtx->dr[7] & ~X86_DR7_RA1_MASK)) /* REM 'mov drX,greg' bug.*/
            CHECK_FIELD(dr[7]);
        CHECK_FIELD(gdtr.cbGdt);
        CHECK_FIELD(gdtr.pGdt);
        CHECK_FIELD(idtr.cbIdt);
        CHECK_FIELD(idtr.pIdt);
        CHECK_SEL(ldtr);
        CHECK_SEL(tr);
        CHECK_FIELD(SysEnter.cs);
        CHECK_FIELD(SysEnter.eip);
        CHECK_FIELD(SysEnter.esp);
        CHECK_FIELD(msrEFER);
        CHECK_FIELD(msrSTAR);
        CHECK_FIELD(msrPAT);
        CHECK_FIELD(msrLSTAR);
        CHECK_FIELD(msrCSTAR);
        CHECK_FIELD(msrSFMASK);
        CHECK_FIELD(msrKERNELGSBASE);

        if (cDiffs != 0)
        {
            DBGFR3InfoEx(pVM->pUVM, pVCpu->idCpu, "cpumguest", "verbose", NULL);
            RTAssertMsg1(NULL, __LINE__, __FILE__, __FUNCTION__);
            RTAssertPanic();
            static bool volatile s_fEnterDebugger = true;
            if (s_fEnterDebugger)
                DBGFSTOP(pVM);

#  if 1 /* Ignore unimplemented instructions for now. */
            if (rcStrictIem == VERR_IEM_INSTR_NOT_IMPLEMENTED)
                rcStrictIem = VINF_SUCCESS;
#  endif
        }
#  undef CHECK_FIELD
#  undef CHECK_BIT_FIELD
    }

    /*
     * If the register state compared fine, check the verification event
     * records.
     */
    if (cDiffs == 0 && !pVCpu->iem.s.fOverlappingMovs)
    {
        /*
         * Compare verficiation event records.
         *  - I/O port accesses should be a 1:1 match.
         */
        PIEMVERIFYEVTREC pIemRec   = pVCpu->iem.s.pIemEvtRecHead;
        PIEMVERIFYEVTREC pOtherRec = pVCpu->iem.s.pOtherEvtRecHead;
        while (pIemRec && pOtherRec)
        {
            /* Since we might miss RAM writes and reads, ignore reads and check
               that any written memory is the same extra ones.  */
            while (   IEMVERIFYEVENT_IS_RAM(pIemRec->enmEvent)
                   && !IEMVERIFYEVENT_IS_RAM(pOtherRec->enmEvent)
                   && pIemRec->pNext)
            {
                if (pIemRec->enmEvent == IEMVERIFYEVENT_RAM_WRITE)
                    iemVerifyWriteRecord(pVCpu, pIemRec, fRem);
                pIemRec = pIemRec->pNext;
            }

            /* Do the compare. */
            if (pIemRec->enmEvent != pOtherRec->enmEvent)
            {
                iemVerifyAssertRecords(pVCpu, pIemRec, pOtherRec, "Type mismatches");
                break;
            }
            bool fEquals;
            switch (pIemRec->enmEvent)
            {
                case IEMVERIFYEVENT_IOPORT_READ:
                    fEquals = pIemRec->u.IOPortRead.Port            == pOtherRec->u.IOPortRead.Port
                           && pIemRec->u.IOPortRead.cbValue         == pOtherRec->u.IOPortRead.cbValue;
                    break;
                case IEMVERIFYEVENT_IOPORT_WRITE:
                    fEquals = pIemRec->u.IOPortWrite.Port           == pOtherRec->u.IOPortWrite.Port
                           && pIemRec->u.IOPortWrite.cbValue        == pOtherRec->u.IOPortWrite.cbValue
                           && pIemRec->u.IOPortWrite.u32Value       == pOtherRec->u.IOPortWrite.u32Value;
                    break;
                case IEMVERIFYEVENT_IOPORT_STR_READ:
                    fEquals = pIemRec->u.IOPortStrRead.Port         == pOtherRec->u.IOPortStrRead.Port
                           && pIemRec->u.IOPortStrRead.cbValue      == pOtherRec->u.IOPortStrRead.cbValue
                           && pIemRec->u.IOPortStrRead.cTransfers   == pOtherRec->u.IOPortStrRead.cTransfers;
                    break;
                case IEMVERIFYEVENT_IOPORT_STR_WRITE:
                    fEquals = pIemRec->u.IOPortStrWrite.Port        == pOtherRec->u.IOPortStrWrite.Port
                           && pIemRec->u.IOPortStrWrite.cbValue     == pOtherRec->u.IOPortStrWrite.cbValue
                           && pIemRec->u.IOPortStrWrite.cTransfers  == pOtherRec->u.IOPortStrWrite.cTransfers;
                    break;
                case IEMVERIFYEVENT_RAM_READ:
                    fEquals = pIemRec->u.RamRead.GCPhys             == pOtherRec->u.RamRead.GCPhys
                           && pIemRec->u.RamRead.cb                 == pOtherRec->u.RamRead.cb;
                    break;
                case IEMVERIFYEVENT_RAM_WRITE:
                    fEquals = pIemRec->u.RamWrite.GCPhys            == pOtherRec->u.RamWrite.GCPhys
                           && pIemRec->u.RamWrite.cb                == pOtherRec->u.RamWrite.cb
                           && !memcmp(pIemRec->u.RamWrite.ab, pOtherRec->u.RamWrite.ab, pIemRec->u.RamWrite.cb);
                    break;
                default:
                    fEquals = false;
                    break;
            }
            if (!fEquals)
            {
                iemVerifyAssertRecords(pVCpu, pIemRec, pOtherRec, "Mismatch");
                break;
            }

            /* advance */
            pIemRec   = pIemRec->pNext;
            pOtherRec = pOtherRec->pNext;
        }

        /* Ignore extra writes and reads. */
        while (pIemRec && IEMVERIFYEVENT_IS_RAM(pIemRec->enmEvent))
        {
            if (pIemRec->enmEvent == IEMVERIFYEVENT_RAM_WRITE)
                iemVerifyWriteRecord(pVCpu, pIemRec, fRem);
            pIemRec = pIemRec->pNext;
        }
        if (pIemRec != NULL)
            iemVerifyAssertRecord(pVCpu, pIemRec, "Extra IEM record!");
        else if (pOtherRec != NULL)
            iemVerifyAssertRecord(pVCpu, pOtherRec, "Extra Other record!");
    }
    IEM_GET_CTX(pVCpu) = pOrgCtx;

    return rcStrictIem;
}

#else  /* !IEM_VERIFICATION_MODE_FULL || !IN_RING3 */

/* stubs */
IEM_STATIC VBOXSTRICTRC     iemVerifyFakeIOPortRead(PVMCPU pVCpu, RTIOPORT Port, uint32_t *pu32Value, size_t cbValue)
{
    NOREF(pVCpu); NOREF(Port); NOREF(pu32Value); NOREF(cbValue);
    return VERR_INTERNAL_ERROR;
}

IEM_STATIC VBOXSTRICTRC     iemVerifyFakeIOPortWrite(PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue)
{
    NOREF(pVCpu); NOREF(Port); NOREF(u32Value); NOREF(cbValue);
    return VERR_INTERNAL_ERROR;
}

#endif /* !IEM_VERIFICATION_MODE_FULL || !IN_RING3 */


#ifdef LOG_ENABLED
/**
 * Logs the current instruction.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pCtx        The current CPU context.
 * @param   fSameCtx    Set if we have the same context information as the VMM,
 *                      clear if we may have already executed an instruction in
 *                      our debug context. When clear, we assume IEMCPU holds
 *                      valid CPU mode info.
 */
IEM_STATIC void iemLogCurInstr(PVMCPU pVCpu, PCPUMCTX pCtx, bool fSameCtx)
{
# ifdef IN_RING3
    if (LogIs2Enabled())
    {
        char     szInstr[256];
        uint32_t cbInstr = 0;
        if (fSameCtx)
            DBGFR3DisasInstrEx(pVCpu->pVMR3->pUVM, pVCpu->idCpu, 0, 0,
                               DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                               szInstr, sizeof(szInstr), &cbInstr);
        else
        {
            uint32_t fFlags = 0;
            switch (pVCpu->iem.s.enmCpuMode)
            {
                case IEMMODE_64BIT: fFlags |= DBGF_DISAS_FLAGS_64BIT_MODE; break;
                case IEMMODE_32BIT: fFlags |= DBGF_DISAS_FLAGS_32BIT_MODE; break;
                case IEMMODE_16BIT:
                    if (!(pCtx->cr0 & X86_CR0_PE) || pCtx->eflags.Bits.u1VM)
                        fFlags |= DBGF_DISAS_FLAGS_16BIT_REAL_MODE;
                    else
                        fFlags |= DBGF_DISAS_FLAGS_16BIT_MODE;
                    break;
            }
            DBGFR3DisasInstrEx(pVCpu->pVMR3->pUVM, pVCpu->idCpu, pCtx->cs.Sel, pCtx->rip, fFlags,
                               szInstr, sizeof(szInstr), &cbInstr);
        }

        PCX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
        Log2(("****\n"
              " eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
              " eip=%08x esp=%08x ebp=%08x iopl=%d tr=%04x\n"
              " cs=%04x ss=%04x ds=%04x es=%04x fs=%04x gs=%04x efl=%08x\n"
              " fsw=%04x fcw=%04x ftw=%02x mxcsr=%04x/%04x\n"
              " %s\n"
              ,
              pCtx->eax, pCtx->ebx, pCtx->ecx, pCtx->edx, pCtx->esi, pCtx->edi,
              pCtx->eip, pCtx->esp, pCtx->ebp, pCtx->eflags.Bits.u2IOPL, pCtx->tr.Sel,
              pCtx->cs.Sel, pCtx->ss.Sel, pCtx->ds.Sel, pCtx->es.Sel,
              pCtx->fs.Sel, pCtx->gs.Sel, pCtx->eflags.u,
              pFpuCtx->FSW, pFpuCtx->FCW, pFpuCtx->FTW, pFpuCtx->MXCSR, pFpuCtx->MXCSR_MASK,
              szInstr));

        if (LogIs3Enabled())
            DBGFR3InfoEx(pVCpu->pVMR3->pUVM, pVCpu->idCpu, "cpumguest", "verbose", NULL);
    }
    else
# endif
        LogFlow(("IEMExecOne: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x\n",
                 pCtx->cs.Sel, pCtx->rip, pCtx->ss.Sel, pCtx->rsp, pCtx->eflags.u));
    RT_NOREF_PV(pVCpu); RT_NOREF_PV(pCtx); RT_NOREF_PV(fSameCtx);
}
#endif


/**
 * Makes status code addjustments (pass up from I/O and access handler)
 * as well as maintaining statistics.
 *
 * @returns Strict VBox status code to pass up.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   rcStrict    The status from executing an instruction.
 */
DECL_FORCE_INLINE(VBOXSTRICTRC) iemExecStatusCodeFiddling(PVMCPU pVCpu, VBOXSTRICTRC rcStrict)
{
    if (rcStrict != VINF_SUCCESS)
    {
        if (RT_SUCCESS(rcStrict))
        {
            AssertMsg(   (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST)
                      || rcStrict == VINF_IOM_R3_IOPORT_READ
                      || rcStrict == VINF_IOM_R3_IOPORT_WRITE
                      || rcStrict == VINF_IOM_R3_IOPORT_COMMIT_WRITE
                      || rcStrict == VINF_IOM_R3_MMIO_READ
                      || rcStrict == VINF_IOM_R3_MMIO_READ_WRITE
                      || rcStrict == VINF_IOM_R3_MMIO_WRITE
                      || rcStrict == VINF_IOM_R3_MMIO_COMMIT_WRITE
                      || rcStrict == VINF_CPUM_R3_MSR_READ
                      || rcStrict == VINF_CPUM_R3_MSR_WRITE
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR
                      || rcStrict == VINF_EM_RAW_TO_R3
                      || rcStrict == VINF_EM_RAW_EMULATE_IO_BLOCK
                      || rcStrict == VINF_EM_TRIPLE_FAULT
                      /* raw-mode / virt handlers only: */
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_TSS_FAULT
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_LDT_FAULT
                      || rcStrict == VINF_EM_RAW_EMULATE_INSTR_IDT_FAULT
                      || rcStrict == VINF_SELM_SYNC_GDT
                      || rcStrict == VINF_CSAM_PENDING_ACTION
                      || rcStrict == VINF_PATM_CHECK_PATCH_PAGE
                      /* nested hw.virt codes: */
                      || rcStrict == VINF_SVM_VMEXIT
                      , ("rcStrict=%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
/** @todo adjust for VINF_EM_RAW_EMULATE_INSTR   */
            int32_t const rcPassUp = pVCpu->iem.s.rcPassUp;
#ifdef VBOX_WITH_NESTED_HWVIRT
            if (   rcStrict == VINF_SVM_VMEXIT
                && rcPassUp == VINF_SUCCESS)
                rcStrict = VINF_SUCCESS;
            else
#endif
            if (rcPassUp == VINF_SUCCESS)
                pVCpu->iem.s.cRetInfStatuses++;
            else if (   rcPassUp < VINF_EM_FIRST
                     || rcPassUp > VINF_EM_LAST
                     || rcPassUp < VBOXSTRICTRC_VAL(rcStrict))
            {
                Log(("IEM: rcPassUp=%Rrc! rcStrict=%Rrc\n", rcPassUp, VBOXSTRICTRC_VAL(rcStrict)));
                pVCpu->iem.s.cRetPassUpStatus++;
                rcStrict = rcPassUp;
            }
            else
            {
                Log(("IEM: rcPassUp=%Rrc  rcStrict=%Rrc!\n", rcPassUp, VBOXSTRICTRC_VAL(rcStrict)));
                pVCpu->iem.s.cRetInfStatuses++;
            }
        }
        else if (rcStrict == VERR_IEM_ASPECT_NOT_IMPLEMENTED)
            pVCpu->iem.s.cRetAspectNotImplemented++;
        else if (rcStrict == VERR_IEM_INSTR_NOT_IMPLEMENTED)
            pVCpu->iem.s.cRetInstrNotImplemented++;
#ifdef IEM_VERIFICATION_MODE_FULL
        else if (rcStrict == VERR_IEM_RESTART_INSTRUCTION)
            rcStrict = VINF_SUCCESS;
#endif
        else
            pVCpu->iem.s.cRetErrStatuses++;
    }
    else if (pVCpu->iem.s.rcPassUp != VINF_SUCCESS)
    {
        pVCpu->iem.s.cRetPassUpStatus++;
        rcStrict = pVCpu->iem.s.rcPassUp;
    }

    return rcStrict;
}


/**
 * The actual code execution bits of IEMExecOne, IEMExecOneEx, and
 * IEMExecOneWithPrefetchedByPC.
 *
 * Similar code is found in IEMExecLots.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   fExecuteInhibit     If set, execute the instruction following CLI,
 *                      POP SS and MOV SS,GR.
 */
DECLINLINE(VBOXSTRICTRC) iemExecOneInner(PVMCPU pVCpu, bool fExecuteInhibit)
{
#ifdef IEM_WITH_SETJMP
    VBOXSTRICTRC rcStrict;
    jmp_buf      JmpBuf;
    jmp_buf     *pSavedJmpBuf  = pVCpu->iem.s.CTX_SUFF(pJmpBuf);
    pVCpu->iem.s.CTX_SUFF(pJmpBuf) = &JmpBuf;
    if ((rcStrict = setjmp(JmpBuf)) == 0)
    {
        uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
        rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
    }
    else
        pVCpu->iem.s.cLongJumps++;
    pVCpu->iem.s.CTX_SUFF(pJmpBuf) = pSavedJmpBuf;
#else
    uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
    VBOXSTRICTRC rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
#endif
    if (rcStrict == VINF_SUCCESS)
        pVCpu->iem.s.cInstructions++;
    if (pVCpu->iem.s.cActiveMappings > 0)
    {
        Assert(rcStrict != VINF_SUCCESS);
        iemMemRollback(pVCpu);
    }
//#ifdef DEBUG
//    AssertMsg(IEM_GET_INSTR_LEN(pVCpu) == cbInstr || rcStrict != VINF_SUCCESS, ("%u %u\n", IEM_GET_INSTR_LEN(pVCpu), cbInstr));
//#endif

    /* Execute the next instruction as well if a cli, pop ss or
       mov ss, Gr has just completed successfully. */
    if (   fExecuteInhibit
        && rcStrict == VINF_SUCCESS
        && VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
        && EMGetInhibitInterruptsPC(pVCpu) == IEM_GET_CTX(pVCpu)->rip )
    {
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, pVCpu->iem.s.fBypassHandlers);
        if (rcStrict == VINF_SUCCESS)
        {
#ifdef LOG_ENABLED
            iemLogCurInstr(pVCpu, IEM_GET_CTX(pVCpu), false);
#endif
#ifdef IEM_WITH_SETJMP
            pVCpu->iem.s.CTX_SUFF(pJmpBuf) = &JmpBuf;
            if ((rcStrict = setjmp(JmpBuf)) == 0)
            {
                uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
                rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
            }
            else
                pVCpu->iem.s.cLongJumps++;
            pVCpu->iem.s.CTX_SUFF(pJmpBuf) = pSavedJmpBuf;
#else
            IEM_OPCODE_GET_NEXT_U8(&b);
            rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
#endif
            if (rcStrict == VINF_SUCCESS)
                pVCpu->iem.s.cInstructions++;
            if (pVCpu->iem.s.cActiveMappings > 0)
            {
                Assert(rcStrict != VINF_SUCCESS);
                iemMemRollback(pVCpu);
            }
        }
        EMSetInhibitInterruptsPC(pVCpu, UINT64_C(0x7777555533331111));
    }

    /*
     * Return value fiddling, statistics and sanity assertions.
     */
    rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);

    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->cs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->ss));
#if defined(IEM_VERIFICATION_MODE_FULL)
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->es));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->ds));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->fs));
    Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->gs));
#endif
    return rcStrict;
}


#ifdef IN_RC
/**
 * Re-enters raw-mode or ensure we return to ring-3.
 *
 * @returns rcStrict, maybe modified.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   pCtx        The current CPU context.
 * @param   rcStrict    The status code returne by the interpreter.
 */
DECLINLINE(VBOXSTRICTRC) iemRCRawMaybeReenter(PVMCPU pVCpu, PCPUMCTX pCtx, VBOXSTRICTRC rcStrict)
{
    if (   !pVCpu->iem.s.fInPatchCode
        && (   rcStrict == VINF_SUCCESS
            || rcStrict == VERR_IEM_INSTR_NOT_IMPLEMENTED  /* pgmPoolAccessPfHandlerFlush */
            || rcStrict == VERR_IEM_ASPECT_NOT_IMPLEMENTED /* ditto */ ) )
    {
        if (pCtx->eflags.Bits.u1IF || rcStrict != VINF_SUCCESS)
            CPUMRawEnter(pVCpu);
        else
        {
            Log(("iemRCRawMaybeReenter: VINF_EM_RESCHEDULE\n"));
            rcStrict = VINF_EM_RESCHEDULE;
        }
    }
    return rcStrict;
}
#endif


/**
 * Execute one instruction.
 *
 * @return  Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMDECL(VBOXSTRICTRC) IEMExecOne(PVMCPU pVCpu)
{
#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)
    if (++pVCpu->iem.s.cVerifyDepth == 1)
        iemExecVerificationModeSetup(pVCpu);
#endif
#ifdef LOG_ENABLED
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    iemLogCurInstr(pVCpu, pCtx, true);
#endif

    /*
     * Do the decoding and emulation.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, true);

#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)
    /*
     * Assert some sanity.
     */
    if (pVCpu->iem.s.cVerifyDepth == 1)
        rcStrict = iemExecVerificationModeCheck(pVCpu, rcStrict);
    pVCpu->iem.s.cVerifyDepth--;
#endif
#ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, IEM_GET_CTX(pVCpu), rcStrict);
#endif
    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecOne: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pCtx->cs.Sel, pCtx->rip, pCtx->ss.Sel, pCtx->rsp, pCtx->eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC)       IEMExecOneEx(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, uint32_t *pcbWritten)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    AssertReturn(CPUMCTX2CORE(pCtx) == pCtxCore, VERR_IEM_IPE_3);

    uint32_t const cbOldWritten = pVCpu->iem.s.cbWritten;
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false);
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = iemExecOneInner(pVCpu, true);
        if (pcbWritten)
            *pcbWritten = pVCpu->iem.s.cbWritten - cbOldWritten;
    }

#ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, pCtx, rcStrict);
#endif
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC)       IEMExecOneWithPrefetchedByPC(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, uint64_t OpcodeBytesPC,
                                                         const void *pvOpcodeBytes, size_t cbOpcodeBytes)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    AssertReturn(CPUMCTX2CORE(pCtx) == pCtxCore, VERR_IEM_IPE_3);

    VBOXSTRICTRC rcStrict;
    if (   cbOpcodeBytes
        && pCtx->rip == OpcodeBytesPC)
    {
        iemInitDecoder(pVCpu, false);
#ifdef IEM_WITH_CODE_TLB
        pVCpu->iem.s.uInstrBufPc      = OpcodeBytesPC;
        pVCpu->iem.s.pbInstrBuf       = (uint8_t const *)pvOpcodeBytes;
        pVCpu->iem.s.cbInstrBufTotal  = (uint16_t)RT_MIN(X86_PAGE_SIZE, cbOpcodeBytes);
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.offInstrNextByte = 0;
#else
        pVCpu->iem.s.cbOpcode = (uint8_t)RT_MIN(cbOpcodeBytes, sizeof(pVCpu->iem.s.abOpcode));
        memcpy(pVCpu->iem.s.abOpcode, pvOpcodeBytes, pVCpu->iem.s.cbOpcode);
#endif
        rcStrict = VINF_SUCCESS;
    }
    else
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false);
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = iemExecOneInner(pVCpu, true);
    }

#ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, pCtx, rcStrict);
#endif
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC)       IEMExecOneBypassEx(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, uint32_t *pcbWritten)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    AssertReturn(CPUMCTX2CORE(pCtx) == pCtxCore, VERR_IEM_IPE_3);

    uint32_t const cbOldWritten = pVCpu->iem.s.cbWritten;
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, true);
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = iemExecOneInner(pVCpu, false);
        if (pcbWritten)
            *pcbWritten = pVCpu->iem.s.cbWritten - cbOldWritten;
    }

#ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, pCtx, rcStrict);
#endif
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC)       IEMExecOneBypassWithPrefetchedByPC(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, uint64_t OpcodeBytesPC,
                                                               const void *pvOpcodeBytes, size_t cbOpcodeBytes)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
    AssertReturn(CPUMCTX2CORE(pCtx) == pCtxCore, VERR_IEM_IPE_3);

    VBOXSTRICTRC rcStrict;
    if (   cbOpcodeBytes
        && pCtx->rip == OpcodeBytesPC)
    {
        iemInitDecoder(pVCpu, true);
#ifdef IEM_WITH_CODE_TLB
        pVCpu->iem.s.uInstrBufPc      = OpcodeBytesPC;
        pVCpu->iem.s.pbInstrBuf       = (uint8_t const *)pvOpcodeBytes;
        pVCpu->iem.s.cbInstrBufTotal  = (uint16_t)RT_MIN(X86_PAGE_SIZE, cbOpcodeBytes);
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.offInstrNextByte = 0;
#else
        pVCpu->iem.s.cbOpcode = (uint8_t)RT_MIN(cbOpcodeBytes, sizeof(pVCpu->iem.s.abOpcode));
        memcpy(pVCpu->iem.s.abOpcode, pvOpcodeBytes, pVCpu->iem.s.cbOpcode);
#endif
        rcStrict = VINF_SUCCESS;
    }
    else
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, true);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, false);

#ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, pCtx, rcStrict);
#endif
    return rcStrict;
}


/**
 * For debugging DISGetParamSize, may come in handy.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling EMT.
 * @param   pCtxCore        The context core structure.
 * @param   OpcodeBytesPC   The PC of the opcode bytes.
 * @param   pvOpcodeBytes   Prefeched opcode bytes.
 * @param   cbOpcodeBytes   Number of prefetched bytes.
 * @param   pcbWritten      Where to return the number of bytes written.
 *                          Optional.
 */
VMMDECL(VBOXSTRICTRC)       IEMExecOneBypassWithPrefetchedByPCWritten(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore, uint64_t OpcodeBytesPC,
                                                                      const void *pvOpcodeBytes, size_t cbOpcodeBytes,
                                                                      uint32_t *pcbWritten)
{
    PCPUMCTX pCtx    = IEM_GET_CTX(pVCpu);
    AssertReturn(CPUMCTX2CORE(pCtx) == pCtxCore, VERR_IEM_IPE_3);

    uint32_t const cbOldWritten = pVCpu->iem.s.cbWritten;
    VBOXSTRICTRC rcStrict;
    if (   cbOpcodeBytes
        && pCtx->rip == OpcodeBytesPC)
    {
        iemInitDecoder(pVCpu, true);
#ifdef IEM_WITH_CODE_TLB
        pVCpu->iem.s.uInstrBufPc      = OpcodeBytesPC;
        pVCpu->iem.s.pbInstrBuf       = (uint8_t const *)pvOpcodeBytes;
        pVCpu->iem.s.cbInstrBufTotal  = (uint16_t)RT_MIN(X86_PAGE_SIZE, cbOpcodeBytes);
        pVCpu->iem.s.offCurInstrStart = 0;
        pVCpu->iem.s.offInstrNextByte = 0;
#else
        pVCpu->iem.s.cbOpcode = (uint8_t)RT_MIN(cbOpcodeBytes, sizeof(pVCpu->iem.s.abOpcode));
        memcpy(pVCpu->iem.s.abOpcode, pvOpcodeBytes, pVCpu->iem.s.cbOpcode);
#endif
        rcStrict = VINF_SUCCESS;
    }
    else
        rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, true);
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = iemExecOneInner(pVCpu, false);
        if (pcbWritten)
            *pcbWritten = pVCpu->iem.s.cbWritten - cbOldWritten;
    }

#ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, pCtx, rcStrict);
#endif
    return rcStrict;
}


VMMDECL(VBOXSTRICTRC) IEMExecLots(PVMCPU pVCpu, uint32_t *pcInstructions)
{
    uint32_t const cInstructionsAtStart = pVCpu->iem.s.cInstructions;

#if defined(IEM_VERIFICATION_MODE_FULL) && defined(IN_RING3)
    /*
     * See if there is an interrupt pending in TRPM, inject it if we can.
     */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
# ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.uInjectCpl = UINT8_MAX;
# endif

    /** @todo Maybe someday we can centralize this under CPUMCanInjectInterrupt()? */
#if defined(VBOX_WITH_NESTED_HWVIRT)
    bool fIntrEnabled = pCtx->hwvirt.svm.fGif;
    if (fIntrEnabled)
    {
        if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
            fIntrEnabled = CPUMCanSvmNstGstTakePhysIntr(pCtx);
        else
            fIntrEnabled = pCtx->eflags.Bits.u1IF;
    }
#else
    bool fIntrEnabled = pCtx->eflags.Bits.u1IF;
#endif
    if (   fIntrEnabled
        && TRPMHasTrap(pVCpu)
        && EMGetInhibitInterruptsPC(pVCpu) != pCtx->rip)
    {
        uint8_t     u8TrapNo;
        TRPMEVENT   enmType;
        RTGCUINT    uErrCode;
        RTGCPTR     uCr2;
        int rc2 = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrCode, &uCr2, NULL /* pu8InstLen */); AssertRC(rc2);
        IEMInjectTrap(pVCpu, u8TrapNo, enmType, (uint16_t)uErrCode, uCr2, 0 /* cbInstr */);
        if (!IEM_VERIFICATION_ENABLED(pVCpu))
            TRPMResetTrap(pVCpu);
    }

    /*
     * Log the state.
     */
# ifdef LOG_ENABLED
    iemLogCurInstr(pVCpu, pCtx, true);
# endif

    /*
     * Do the decoding and emulation.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false);
    if (rcStrict == VINF_SUCCESS)
        rcStrict = iemExecOneInner(pVCpu, true);

    /*
     * Assert some sanity.
     */
    rcStrict = iemExecVerificationModeCheck(pVCpu, rcStrict);

    /*
     * Log and return.
     */
    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecLots: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pCtx->cs.Sel, pCtx->rip, pCtx->ss.Sel, pCtx->rsp, pCtx->eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    if (pcInstructions)
        *pcInstructions = pVCpu->iem.s.cInstructions - cInstructionsAtStart;
    return rcStrict;

#else  /* Not verification mode */

    /*
     * See if there is an interrupt pending in TRPM, inject it if we can.
     */
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);
# ifdef IEM_VERIFICATION_MODE_FULL
    pVCpu->iem.s.uInjectCpl = UINT8_MAX;
# endif

    /** @todo Can we centralize this under CPUMCanInjectInterrupt()? */
#if defined(VBOX_WITH_NESTED_HWVIRT)
    bool fIntrEnabled = pCtx->hwvirt.svm.fGif;
    if (fIntrEnabled)
    {
        if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
            fIntrEnabled = CPUMCanSvmNstGstTakePhysIntr(pCtx);
        else
            fIntrEnabled = pCtx->eflags.Bits.u1IF;
    }
#else
    bool fIntrEnabled = pCtx->eflags.Bits.u1IF;
#endif
    if (   fIntrEnabled
        && TRPMHasTrap(pVCpu)
        && EMGetInhibitInterruptsPC(pVCpu) != pCtx->rip)
    {
        uint8_t     u8TrapNo;
        TRPMEVENT   enmType;
        RTGCUINT    uErrCode;
        RTGCPTR     uCr2;
        int rc2 = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrCode, &uCr2, NULL /* pu8InstLen */); AssertRC(rc2);
        IEMInjectTrap(pVCpu, u8TrapNo, enmType, (uint16_t)uErrCode, uCr2, 0 /* cbInstr */);
        if (!IEM_VERIFICATION_ENABLED(pVCpu))
            TRPMResetTrap(pVCpu);
    }

    /*
     * Initial decoder init w/ prefetch, then setup setjmp.
     */
    VBOXSTRICTRC rcStrict = iemInitDecoderAndPrefetchOpcodes(pVCpu, false);
    if (rcStrict == VINF_SUCCESS)
    {
# ifdef IEM_WITH_SETJMP
        jmp_buf         JmpBuf;
        jmp_buf        *pSavedJmpBuf = pVCpu->iem.s.CTX_SUFF(pJmpBuf);
        pVCpu->iem.s.CTX_SUFF(pJmpBuf)   = &JmpBuf;
        pVCpu->iem.s.cActiveMappings     = 0;
        if ((rcStrict = setjmp(JmpBuf)) == 0)
# endif
        {
            /*
             * The run loop.  We limit ourselves to 4096 instructions right now.
             */
            PVM         pVM    = pVCpu->CTX_SUFF(pVM);
            uint32_t    cInstr = 4096;
            for (;;)
            {
                /*
                 * Log the state.
                 */
# ifdef LOG_ENABLED
                iemLogCurInstr(pVCpu, pCtx, true);
# endif

                /*
                 * Do the decoding and emulation.
                 */
                uint8_t b; IEM_OPCODE_GET_NEXT_U8(&b);
                rcStrict = FNIEMOP_CALL(g_apfnOneByteMap[b]);
                if (RT_LIKELY(rcStrict == VINF_SUCCESS))
                {
                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                    pVCpu->iem.s.cInstructions++;
                    if (RT_LIKELY(pVCpu->iem.s.rcPassUp == VINF_SUCCESS))
                    {
                        uint32_t fCpu = pVCpu->fLocalForcedActions
                                      & ( VMCPU_FF_ALL_MASK & ~(  VMCPU_FF_PGM_SYNC_CR3
                                                                | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL
                                                                | VMCPU_FF_TLB_FLUSH
# ifdef VBOX_WITH_RAW_MODE
                                                                | VMCPU_FF_TRPM_SYNC_IDT
                                                                | VMCPU_FF_SELM_SYNC_TSS
                                                                | VMCPU_FF_SELM_SYNC_GDT
                                                                | VMCPU_FF_SELM_SYNC_LDT
# endif
                                                                | VMCPU_FF_INHIBIT_INTERRUPTS
                                                                | VMCPU_FF_BLOCK_NMIS
                                                                | VMCPU_FF_UNHALT ));

                        if (RT_LIKELY(   (   !fCpu
                                          || (   !(fCpu & ~(VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
                                              && !pCtx->rflags.Bits.u1IF) )
                                      && !VM_FF_IS_PENDING(pVM, VM_FF_ALL_MASK) ))
                        {
                            if (cInstr-- > 0)
                            {
                                Assert(pVCpu->iem.s.cActiveMappings == 0);
                                iemReInitDecoder(pVCpu);
                                continue;
                            }
                        }
                    }
                    Assert(pVCpu->iem.s.cActiveMappings == 0);
                }
                else if (pVCpu->iem.s.cActiveMappings > 0)
                        iemMemRollback(pVCpu);
                rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
                break;
            }
        }
# ifdef IEM_WITH_SETJMP
        else
        {
            if (pVCpu->iem.s.cActiveMappings > 0)
                iemMemRollback(pVCpu);
            pVCpu->iem.s.cLongJumps++;
#  ifdef VBOX_WITH_NESTED_HWVIRT
            /*
             * When a nested-guest causes an exception intercept when fetching memory
             * (e.g. IEM_MC_FETCH_MEM_U16) as part of instruction execution, we need this
             * to fix-up VINF_SVM_VMEXIT on the longjmp way out, otherwise we will guru.
             */
            rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
#  endif
        }
        pVCpu->iem.s.CTX_SUFF(pJmpBuf) = pSavedJmpBuf;
# endif

        /*
         * Assert hidden register sanity (also done in iemInitDecoder and iemReInitDecoder).
         */
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->cs));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->ss));
# if defined(IEM_VERIFICATION_MODE_FULL)
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->es));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->ds));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->fs));
        Assert(CPUMSELREG_ARE_HIDDEN_PARTS_VALID(pVCpu, &IEM_GET_CTX(pVCpu)->gs));
# endif
    }
#  ifdef VBOX_WITH_NESTED_HWVIRT
    else
    {
        /*
         * When a nested-guest causes an exception intercept (e.g. #PF) when fetching
         * code as part of instruction execution, we need this to fix-up VINF_SVM_VMEXIT.
         */
        rcStrict = iemExecStatusCodeFiddling(pVCpu, rcStrict);
    }
#  endif

    /*
     * Maybe re-enter raw-mode and log.
     */
# ifdef IN_RC
    rcStrict = iemRCRawMaybeReenter(pVCpu, IEM_GET_CTX(pVCpu), rcStrict);
# endif
    if (rcStrict != VINF_SUCCESS)
        LogFlow(("IEMExecLots: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pCtx->cs.Sel, pCtx->rip, pCtx->ss.Sel, pCtx->rsp, pCtx->eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    if (pcInstructions)
        *pcInstructions = pVCpu->iem.s.cInstructions - cInstructionsAtStart;
    return rcStrict;
#endif /* Not verification mode */
}



/**
 * Injects a trap, fault, abort, software interrupt or external interrupt.
 *
 * The parameter list matches TRPMQueryTrapAll pretty closely.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   u8TrapNo            The trap number.
 * @param   enmType             What type is it (trap/fault/abort), software
 *                              interrupt or hardware interrupt.
 * @param   uErrCode            The error code if applicable.
 * @param   uCr2                The CR2 value if applicable.
 * @param   cbInstr             The instruction length (only relevant for
 *                              software interrupts).
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMInjectTrap(PVMCPU pVCpu, uint8_t u8TrapNo, TRPMEVENT enmType, uint16_t uErrCode, RTGCPTR uCr2,
                                         uint8_t cbInstr)
{
    iemInitDecoder(pVCpu, false);
#ifdef DBGFTRACE_ENABLED
    RTTraceBufAddMsgF(pVCpu->CTX_SUFF(pVM)->CTX_SUFF(hTraceBuf), "IEMInjectTrap: %x %d %x %llx",
                      u8TrapNo, enmType, uErrCode, uCr2);
#endif

    uint32_t fFlags;
    switch (enmType)
    {
        case TRPM_HARDWARE_INT:
            Log(("IEMInjectTrap: %#4x ext\n", u8TrapNo));
            fFlags = IEM_XCPT_FLAGS_T_EXT_INT;
            uErrCode = uCr2 = 0;
            break;

        case TRPM_SOFTWARE_INT:
            Log(("IEMInjectTrap: %#4x soft\n", u8TrapNo));
            fFlags = IEM_XCPT_FLAGS_T_SOFT_INT;
            uErrCode = uCr2 = 0;
            break;

        case TRPM_TRAP:
            Log(("IEMInjectTrap: %#4x trap err=%#x cr2=%#RGv\n", u8TrapNo, uErrCode, uCr2));
            fFlags = IEM_XCPT_FLAGS_T_CPU_XCPT;
            if (u8TrapNo == X86_XCPT_PF)
                fFlags |= IEM_XCPT_FLAGS_CR2;
            switch (u8TrapNo)
            {
                case X86_XCPT_DF:
                case X86_XCPT_TS:
                case X86_XCPT_NP:
                case X86_XCPT_SS:
                case X86_XCPT_PF:
                case X86_XCPT_AC:
                    fFlags |= IEM_XCPT_FLAGS_ERR;
                    break;

                case X86_XCPT_NMI:
                    VMCPU_FF_SET(pVCpu, VMCPU_FF_BLOCK_NMIS);
                    break;
            }
            break;

        IEM_NOT_REACHED_DEFAULT_CASE_RET();
    }

    return iemRaiseXcptOrInt(pVCpu, cbInstr, u8TrapNo, fFlags, uErrCode, uCr2);
}


/**
 * Injects the active TRPM event.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 */
VMMDECL(VBOXSTRICTRC) IEMInjectTrpmEvent(PVMCPU pVCpu)
{
#ifndef IEM_IMPLEMENTS_TASKSWITCH
    IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(("Event injection\n"));
#else
    uint8_t     u8TrapNo;
    TRPMEVENT   enmType;
    RTGCUINT    uErrCode;
    RTGCUINTPTR uCr2;
    uint8_t     cbInstr;
    int rc = TRPMQueryTrapAll(pVCpu, &u8TrapNo, &enmType, &uErrCode, &uCr2, &cbInstr);
    if (RT_FAILURE(rc))
        return rc;

    VBOXSTRICTRC rcStrict = IEMInjectTrap(pVCpu, u8TrapNo, enmType, uErrCode, uCr2, cbInstr);

    /** @todo Are there any other codes that imply the event was successfully
     *        delivered to the guest? See @bugref{6607}.  */
    if (   rcStrict == VINF_SUCCESS
        || rcStrict == VINF_IEM_RAISED_XCPT)
    {
        TRPMResetTrap(pVCpu);
    }
    return rcStrict;
#endif
}


VMM_INT_DECL(int) IEMBreakpointSet(PVM pVM, RTGCPTR GCPtrBp)
{
    RT_NOREF_PV(pVM); RT_NOREF_PV(GCPtrBp);
    return VERR_NOT_IMPLEMENTED;
}


VMM_INT_DECL(int) IEMBreakpointClear(PVM pVM, RTGCPTR GCPtrBp)
{
    RT_NOREF_PV(pVM); RT_NOREF_PV(GCPtrBp);
    return VERR_NOT_IMPLEMENTED;
}


#if 0 /* The IRET-to-v8086 mode in PATM is very optimistic, so I don't dare do this yet. */
/**
 * Executes a IRET instruction with default operand size.
 *
 * This is for PATM.
 *
 * @returns VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   pCtxCore            The register frame.
 */
VMM_INT_DECL(int) IEMExecInstr_iret(PVMCPU pVCpu, PCPUMCTXCORE pCtxCore)
{
    PCPUMCTX pCtx = IEM_GET_CTX(pVCpu);

    iemCtxCoreToCtx(pCtx, pCtxCore);
    iemInitDecoder(pVCpu);
    VBOXSTRICTRC rcStrict = iemCImpl_iret(pVCpu, 1, pVCpu->iem.s.enmDefOpSize);
    if (rcStrict == VINF_SUCCESS)
        iemCtxToCtxCore(pCtxCore, pCtx);
    else
        LogFlow(("IEMExecInstr_iret: cs:rip=%04x:%08RX64 ss:rsp=%04x:%08RX64 EFL=%06x - rcStrict=%Rrc\n",
                 pCtx->cs, pCtx->rip, pCtx->ss, pCtx->rsp, pCtx->eflags.u, VBOXSTRICTRC_VAL(rcStrict)));
    return rcStrict;
}
#endif


/**
 * Macro used by the IEMExec* method to check the given instruction length.
 *
 * Will return on failure!
 *
 * @param   a_cbInstr   The given instruction length.
 * @param   a_cbMin     The minimum length.
 */
#define IEMEXEC_ASSERT_INSTR_LEN_RETURN(a_cbInstr, a_cbMin) \
    AssertMsgReturn((unsigned)(a_cbInstr) - (unsigned)(a_cbMin) <= (unsigned)15 - (unsigned)(a_cbMin), \
                    ("cbInstr=%u cbMin=%u\n", (a_cbInstr), (a_cbMin)), VERR_IEM_INVALID_INSTR_LENGTH)


/**
 * Calls iemUninitExec, iemExecStatusCodeFiddling and iemRCRawMaybeReenter.
 *
 * Only calling iemRCRawMaybeReenter in raw-mode, obviously.
 *
 * @returns Fiddled strict vbox status code, ready to return to non-IEM caller.
 * @param   pVCpu       The cross context virtual CPU structure of the calling thread.
 * @param   rcStrict    The status code to fiddle.
 */
DECLINLINE(VBOXSTRICTRC) iemUninitExecAndFiddleStatusAndMaybeReenter(PVMCPU pVCpu, VBOXSTRICTRC rcStrict)
{
    iemUninitExec(pVCpu);
#ifdef IN_RC
    return iemRCRawMaybeReenter(pVCpu, IEM_GET_CTX(pVCpu),
                                iemExecStatusCodeFiddling(pVCpu, rcStrict));
#else
    return iemExecStatusCodeFiddling(pVCpu, rcStrict);
#endif
}


/**
 * Interface for HM and EM for executing string I/O OUT (write) instructions.
 *
 * This API ASSUMES that the caller has already verified that the guest code is
 * allowed to access the I/O port.  (The I/O port is in the DX register in the
 * guest state.)
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbValue             The size of the I/O port access (1, 2, or 4).
 * @param   enmAddrMode         The addressing mode.
 * @param   fRepPrefix          Indicates whether a repeat prefix is used
 *                              (doesn't matter which for this instruction).
 * @param   cbInstr             The instruction length in bytes.
 * @param   iEffSeg             The effective segment address.
 * @param   fIoChecked          Whether the access to the I/O port has been
 *                              checked or not.  It's typically checked in the
 *                              HM scenario.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecStringIoWrite(PVMCPU pVCpu, uint8_t cbValue, IEMMODE enmAddrMode,
                                                bool fRepPrefix, uint8_t cbInstr, uint8_t iEffSeg, bool fIoChecked)
{
    AssertMsgReturn(iEffSeg < X86_SREG_COUNT, ("%#x\n", iEffSeg), VERR_IEM_INVALID_EFF_SEG);
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);

    /*
     * State init.
     */
    iemInitExec(pVCpu, false /*fBypassHandlers*/);

    /*
     * Switch orgy for getting to the right handler.
     */
    VBOXSTRICTRC rcStrict;
    if (fRepPrefix)
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_outs_op8_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_outs_op16_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_outs_op32_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_outs_op8_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_outs_op16_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_outs_op32_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_outs_op8_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_outs_op16_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_outs_op32_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }
    else
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_outs_op8_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_outs_op16_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_outs_op32_addr16(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_outs_op8_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_outs_op16_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_outs_op32_addr32(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_outs_op8_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_outs_op16_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_outs_op32_addr64(pVCpu, cbInstr, iEffSeg, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }

    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM for executing string I/O IN (read) instructions.
 *
 * This API ASSUMES that the caller has already verified that the guest code is
 * allowed to access the I/O port.  (The I/O port is in the DX register in the
 * guest state.)
 *
 * @returns Strict VBox status code.
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbValue             The size of the I/O port access (1, 2, or 4).
 * @param   enmAddrMode         The addressing mode.
 * @param   fRepPrefix          Indicates whether a repeat prefix is used
 *                              (doesn't matter which for this instruction).
 * @param   cbInstr             The instruction length in bytes.
 * @param   fIoChecked          Whether the access to the I/O port has been
 *                              checked or not.  It's typically checked in the
 *                              HM scenario.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecStringIoRead(PVMCPU pVCpu, uint8_t cbValue, IEMMODE enmAddrMode,
                                               bool fRepPrefix, uint8_t cbInstr, bool fIoChecked)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);

    /*
     * State init.
     */
    iemInitExec(pVCpu, false /*fBypassHandlers*/);

    /*
     * Switch orgy for getting to the right handler.
     */
    VBOXSTRICTRC rcStrict;
    if (fRepPrefix)
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_ins_op8_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_ins_op16_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_ins_op32_addr16(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_ins_op8_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_ins_op16_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_ins_op32_addr32(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_rep_ins_op8_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_rep_ins_op16_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_rep_ins_op32_addr64(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }
    else
    {
        switch (enmAddrMode)
        {
            case IEMMODE_16BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_ins_op8_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_ins_op16_addr16(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_ins_op32_addr16(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_32BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_ins_op8_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_ins_op16_addr32(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_ins_op32_addr32(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            case IEMMODE_64BIT:
                switch (cbValue)
                {
                    case 1: rcStrict = iemCImpl_ins_op8_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 2: rcStrict = iemCImpl_ins_op16_addr64(pVCpu, cbInstr, fIoChecked); break;
                    case 4: rcStrict = iemCImpl_ins_op32_addr64(pVCpu, cbInstr, fIoChecked); break;
                    default:
                        AssertMsgFailedReturn(("cbValue=%#x\n", cbValue), VERR_IEM_INVALID_OPERAND_SIZE);
                }
                break;

            default:
                AssertMsgFailedReturn(("enmAddrMode=%d\n", enmAddrMode), VERR_IEM_INVALID_ADDRESS_MODE);
        }
    }

    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for rawmode to write execute an OUT instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   u16Port     The port to read.
 * @param   cbReg       The register size.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedOut(PVMCPU pVCpu, uint8_t cbInstr, uint16_t u16Port, uint8_t cbReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);
    Assert(cbReg <= 4 && cbReg != 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_out, u16Port, cbReg);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for rawmode to write execute an IN instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   u16Port     The port to read.
 * @param   cbReg       The register size.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedIn(PVMCPU pVCpu, uint8_t cbInstr, uint16_t u16Port, uint8_t cbReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 1);
    Assert(cbReg <= 4 && cbReg != 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_in, u16Port, cbReg);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to write to a CRx register.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iCrReg      The control register number (destination).
 * @param   iGReg       The general purpose register number (source).
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMovCRxWrite(PVMCPU pVCpu, uint8_t cbInstr, uint8_t iCrReg, uint8_t iGReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    Assert(iCrReg < 16);
    Assert(iGReg < 16);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_mov_Cd_Rd, iCrReg, iGReg);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to read from a CRx register.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   iGReg       The general purpose register number (destination).
 * @param   iCrReg      The control register number (source).
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedMovCRxRead(PVMCPU pVCpu, uint8_t cbInstr, uint8_t iGReg, uint8_t iCrReg)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);
    Assert(iCrReg < 16);
    Assert(iGReg < 16);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_mov_Rd_Cd, iGReg, iCrReg);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to clear the CR0[TS] bit.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedClts(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 2);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_clts);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the LMSW instruction (loads CR0).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   uValue      The value to load into CR0.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedLmsw(PVMCPU pVCpu, uint8_t cbInstr, uint16_t uValue)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_1(iemCImpl_lmsw, uValue);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the XSETBV instruction (loads XCRx).
 *
 * Takes input values in ecx and edx:eax of the CPU context of the calling EMT.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @remarks In ring-0 not all of the state needs to be synced in.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedXsetbv(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_xsetbv);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the INVLPG instruction.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cbInstr     The instruction length in bytes.
 * @param   GCPtrPage   The effective address of the page to invalidate.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedInvlpg(PVMCPU pVCpu, uint8_t cbInstr, RTGCPTR GCPtrPage)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_1(iemCImpl_invlpg, GCPtrPage);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the INVPCID instruction.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   cbInstr             The instruction length in bytes.
 * @param   uType               The invalidation type.
 * @param   GCPtrInvpcidDesc    The effective address of the INVPCID descriptor.
 *
 * @remarks In ring-0 not all of the state needs to be synced in.
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedInvpcid(PVMCPU pVCpu, uint8_t cbInstr, uint8_t uType, RTGCPTR GCPtrInvpcidDesc)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 4);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_2(iemCImpl_invpcid, uType, GCPtrInvpcidDesc);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Checks if IEM is in the process of delivering an event (interrupt or
 * exception).
 *
 * @returns true if we're in the process of raising an interrupt or exception,
 *          false otherwise.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   puVector        Where to store the vector associated with the
 *                          currently delivered event, optional.
 * @param   pfFlags         Where to store th event delivery flags (see
 *                          IEM_XCPT_FLAGS_XXX), optional.
 * @param   puErr           Where to store the error code associated with the
 *                          event, optional.
 * @param   puCr2           Where to store the CR2 associated with the event,
 *                          optional.
 * @remarks The caller should check the flags to determine if the error code and
 *          CR2 are valid for the event.
 */
VMM_INT_DECL(bool) IEMGetCurrentXcpt(PVMCPU pVCpu, uint8_t *puVector, uint32_t *pfFlags, uint32_t *puErr, uint64_t *puCr2)
{
    bool const fRaisingXcpt = pVCpu->iem.s.cXcptRecursions > 0;
    if (fRaisingXcpt)
    {
        if (puVector)
            *puVector = pVCpu->iem.s.uCurXcpt;
        if (pfFlags)
            *pfFlags = pVCpu->iem.s.fCurXcpt;
        if (puErr)
            *puErr = pVCpu->iem.s.uCurXcptErr;
        if (puCr2)
            *puCr2 = pVCpu->iem.s.uCurXcptCr2;
    }
    return fRaisingXcpt;
}

#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Interface for HM and EM to emulate the CLGI instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedClgi(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_clgi);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the STGI instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedStgi(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_stgi);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the VMLOAD instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedVmload(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_vmload);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the VMSAVE instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedVmsave(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_vmsave);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the INVLPGA instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedInvlpga(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_invlpga);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate the VMRUN instruction.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   cbInstr     The instruction length in bytes.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecDecodedVmrun(PVMCPU pVCpu, uint8_t cbInstr)
{
    IEMEXEC_ASSERT_INSTR_LEN_RETURN(cbInstr, 3);

    iemInitExec(pVCpu, false /*fBypassHandlers*/);
    VBOXSTRICTRC rcStrict = IEM_CIMPL_CALL_0(iemCImpl_vmrun);
    return iemUninitExecAndFiddleStatusAndMaybeReenter(pVCpu, rcStrict);
}


/**
 * Interface for HM and EM to emulate \#VMEXIT.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   uExitCode   The exit code.
 * @param   uExitInfo1  The exit info. 1 field.
 * @param   uExitInfo2  The exit info. 2 field.
 * @thread  EMT(pVCpu)
 */
VMM_INT_DECL(VBOXSTRICTRC) IEMExecSvmVmexit(PVMCPU pVCpu, uint64_t uExitCode, uint64_t uExitInfo1, uint64_t uExitInfo2)
{
    VBOXSTRICTRC rcStrict = iemSvmVmexit(pVCpu, IEM_GET_CTX(pVCpu), uExitCode, uExitInfo1, uExitInfo2);
    return iemExecStatusCodeFiddling(pVCpu, rcStrict);
}
#endif /* VBOX_WITH_NESTED_HWVIRT */

#ifdef IN_RING3

/**
 * Handles the unlikely and probably fatal merge cases.
 *
 * @returns Merged status code.
 * @param   rcStrict        Current EM status code.
 * @param   rcStrictCommit  The IOM I/O or MMIO write commit status to merge
 *                          with @a rcStrict.
 * @param   iMemMap         The memory mapping index. For error reporting only.
 * @param   pVCpu           The cross context virtual CPU structure of the calling
 *                          thread, for error reporting only.
 */
DECL_NO_INLINE(static, VBOXSTRICTRC) iemR3MergeStatusSlow(VBOXSTRICTRC rcStrict, VBOXSTRICTRC rcStrictCommit,
                                                          unsigned iMemMap, PVMCPU pVCpu)
{
    if (RT_FAILURE_NP(rcStrict))
        return rcStrict;

    if (RT_FAILURE_NP(rcStrictCommit))
        return rcStrictCommit;

    if (rcStrict == rcStrictCommit)
        return rcStrictCommit;

    AssertLogRelMsgFailed(("rcStrictCommit=%Rrc rcStrict=%Rrc iMemMap=%u fAccess=%#x FirstPg=%RGp LB %u SecondPg=%RGp LB %u\n",
                           VBOXSTRICTRC_VAL(rcStrictCommit), VBOXSTRICTRC_VAL(rcStrict), iMemMap,
                           pVCpu->iem.s.aMemMappings[iMemMap].fAccess,
                           pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst,
                           pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond));
    return VERR_IOM_FF_STATUS_IPE;
}


/**
 * Helper for IOMR3ProcessForceFlag.
 *
 * @returns Merged status code.
 * @param   rcStrict        Current EM status code.
 * @param   rcStrictCommit  The IOM I/O or MMIO write commit status to merge
 *                          with @a rcStrict.
 * @param   iMemMap         The memory mapping index. For error reporting only.
 * @param   pVCpu           The cross context virtual CPU structure of the calling
 *                          thread, for error reporting only.
 */
DECLINLINE(VBOXSTRICTRC) iemR3MergeStatus(VBOXSTRICTRC rcStrict, VBOXSTRICTRC rcStrictCommit, unsigned iMemMap, PVMCPU pVCpu)
{
    /* Simple. */
    if (RT_LIKELY(rcStrict == VINF_SUCCESS || rcStrict == VINF_EM_RAW_TO_R3))
        return rcStrictCommit;

    if (RT_LIKELY(rcStrictCommit == VINF_SUCCESS))
        return rcStrict;

    /* EM scheduling status codes. */
    if (RT_LIKELY(   rcStrict >= VINF_EM_FIRST
                  && rcStrict <= VINF_EM_LAST))
    {
        if (RT_LIKELY(   rcStrictCommit >= VINF_EM_FIRST
                      && rcStrictCommit <= VINF_EM_LAST))
            return rcStrict < rcStrictCommit ? rcStrict : rcStrictCommit;
    }

    /* Unlikely */
    return iemR3MergeStatusSlow(rcStrict, rcStrictCommit, iMemMap, pVCpu);
}


/**
 * Called by force-flag handling code when VMCPU_FF_IEM is set.
 *
 * @returns Merge between @a rcStrict and what the commit operation returned.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   rcStrict    The status code returned by ring-0 or raw-mode.
 */
VMMR3_INT_DECL(VBOXSTRICTRC) IEMR3ProcessForceFlag(PVM pVM, PVMCPU pVCpu, VBOXSTRICTRC rcStrict)
{
    /*
     * Reset the pending commit.
     */
    AssertMsg(  (pVCpu->iem.s.aMemMappings[0].fAccess | pVCpu->iem.s.aMemMappings[1].fAccess | pVCpu->iem.s.aMemMappings[2].fAccess)
              & (IEM_ACCESS_PENDING_R3_WRITE_1ST | IEM_ACCESS_PENDING_R3_WRITE_2ND),
              ("%#x %#x %#x\n",
               pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemMappings[2].fAccess));
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_IEM);

    /*
     * Commit the pending bounce buffers (usually just one).
     */
    unsigned cBufs = 0;
    unsigned iMemMap = RT_ELEMENTS(pVCpu->iem.s.aMemMappings);
    while (iMemMap-- > 0)
        if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & (IEM_ACCESS_PENDING_R3_WRITE_1ST | IEM_ACCESS_PENDING_R3_WRITE_2ND))
        {
            Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_TYPE_WRITE);
            Assert(pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_BOUNCE_BUFFERED);
            Assert(!pVCpu->iem.s.aMemBbMappings[iMemMap].fUnassigned);

            uint16_t const  cbFirst  = pVCpu->iem.s.aMemBbMappings[iMemMap].cbFirst;
            uint16_t const  cbSecond = pVCpu->iem.s.aMemBbMappings[iMemMap].cbSecond;
            uint8_t const  *pbBuf    = &pVCpu->iem.s.aBounceBuffers[iMemMap].ab[0];

            if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_PENDING_R3_WRITE_1ST)
            {
                VBOXSTRICTRC rcStrictCommit1 = PGMPhysWrite(pVM,
                                                            pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst,
                                                            pbBuf,
                                                            cbFirst,
                                                            PGMACCESSORIGIN_IEM);
                rcStrict = iemR3MergeStatus(rcStrict, rcStrictCommit1, iMemMap, pVCpu);
                Log(("IEMR3ProcessForceFlag: iMemMap=%u GCPhysFirst=%RGp LB %#x %Rrc => %Rrc\n",
                     iMemMap, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysFirst, cbFirst,
                     VBOXSTRICTRC_VAL(rcStrictCommit1), VBOXSTRICTRC_VAL(rcStrict)));
            }

            if (pVCpu->iem.s.aMemMappings[iMemMap].fAccess & IEM_ACCESS_PENDING_R3_WRITE_2ND)
            {
                VBOXSTRICTRC rcStrictCommit2 = PGMPhysWrite(pVM,
                                                            pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond,
                                                            pbBuf + cbFirst,
                                                            cbSecond,
                                                            PGMACCESSORIGIN_IEM);
                rcStrict = iemR3MergeStatus(rcStrict, rcStrictCommit2, iMemMap, pVCpu);
                Log(("IEMR3ProcessForceFlag: iMemMap=%u GCPhysSecond=%RGp LB %#x %Rrc => %Rrc\n",
                     iMemMap, pVCpu->iem.s.aMemBbMappings[iMemMap].GCPhysSecond, cbSecond,
                     VBOXSTRICTRC_VAL(rcStrictCommit2), VBOXSTRICTRC_VAL(rcStrict)));
            }
            cBufs++;
            pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
        }

    AssertMsg(cBufs > 0 && cBufs == pVCpu->iem.s.cActiveMappings,
              ("cBufs=%u cActiveMappings=%u - %#x %#x %#x\n", cBufs, pVCpu->iem.s.cActiveMappings,
               pVCpu->iem.s.aMemMappings[0].fAccess, pVCpu->iem.s.aMemMappings[1].fAccess, pVCpu->iem.s.aMemMappings[2].fAccess));
    pVCpu->iem.s.cActiveMappings = 0;
    return rcStrict;
}

#endif /* IN_RING3 */

