/* $Id: HM.cpp $ */
/** @file
 * HM - Intel/AMD VM Hardware Support Manager.
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

/** @page pg_hm     HM - Hardware Assisted Virtualization Manager
 *
 * The HM manages guest execution using the VT-x and AMD-V CPU hardware
 * extensions.
 *
 * {summary of what HM does}
 *
 * Hardware assisted virtualization manager was originally abbreviated HWACCM,
 * however that was cumbersome to write and parse for such a central component,
 * so it was shortened to HM when refactoring the code in the 4.3 development
 * cycle.
 *
 * {add sections with more details}
 *
 * @sa @ref grp_hm
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_HM
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/selm.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/hm_vmx.h>
#include <VBox/vmm/hm_svm.h>
#include "HMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>
#include <VBox/param.h>

#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/env.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#define EXIT_REASON(def, val, str) #def " - " #val " - " str
#define EXIT_REASON_NIL() NULL
/** Exit reason descriptions for VT-x, used to describe statistics. */
static const char * const g_apszVTxExitReasons[MAX_EXITREASON_STAT] =
{
    EXIT_REASON(VMX_EXIT_XCPT_OR_NMI            ,   0, "Exception or non-maskable interrupt (NMI)."),
    EXIT_REASON(VMX_EXIT_EXT_INT                ,   1, "External interrupt."),
    EXIT_REASON(VMX_EXIT_TRIPLE_FAULT           ,   2, "Triple fault."),
    EXIT_REASON(VMX_EXIT_INIT_SIGNAL            ,   3, "INIT signal."),
    EXIT_REASON(VMX_EXIT_SIPI                   ,   4, "Start-up IPI (SIPI)."),
    EXIT_REASON(VMX_EXIT_IO_SMI_IRQ             ,   5, "I/O system-management interrupt (SMI)."),
    EXIT_REASON(VMX_EXIT_SMI_IRQ                ,   6, "Other SMI."),
    EXIT_REASON(VMX_EXIT_INT_WINDOW             ,   7, "Interrupt window."),
    EXIT_REASON(VMX_EXIT_NMI_WINDOW             ,   8, "NMI window."),
    EXIT_REASON(VMX_EXIT_TASK_SWITCH            ,   9, "Task switch."),
    EXIT_REASON(VMX_EXIT_CPUID                  ,  10, "CPUID instruction."),
    EXIT_REASON(VMX_EXIT_GETSEC                 ,  11, "GETSEC instrunction."),
    EXIT_REASON(VMX_EXIT_HLT                    ,  12, "HLT instruction."),
    EXIT_REASON(VMX_EXIT_INVD                   ,  13, "INVD instruction."),
    EXIT_REASON(VMX_EXIT_INVLPG                 ,  14, "INVLPG instruction."),
    EXIT_REASON(VMX_EXIT_RDPMC                  ,  15, "RDPMCinstruction."),
    EXIT_REASON(VMX_EXIT_RDTSC                  ,  16, "RDTSC instruction."),
    EXIT_REASON(VMX_EXIT_RSM                    ,  17, "RSM instruction in SMM."),
    EXIT_REASON(VMX_EXIT_VMCALL                 ,  18, "VMCALL instruction."),
    EXIT_REASON(VMX_EXIT_VMCLEAR                ,  19, "VMCLEAR instruction."),
    EXIT_REASON(VMX_EXIT_VMLAUNCH               ,  20, "VMLAUNCH instruction."),
    EXIT_REASON(VMX_EXIT_VMPTRLD                ,  21, "VMPTRLD instruction."),
    EXIT_REASON(VMX_EXIT_VMPTRST                ,  22, "VMPTRST instruction."),
    EXIT_REASON(VMX_EXIT_VMREAD                 ,  23, "VMREAD instruction."),
    EXIT_REASON(VMX_EXIT_VMRESUME               ,  24, "VMRESUME instruction."),
    EXIT_REASON(VMX_EXIT_VMWRITE                ,  25, "VMWRITE instruction."),
    EXIT_REASON(VMX_EXIT_VMXOFF                 ,  26, "VMXOFF instruction."),
    EXIT_REASON(VMX_EXIT_VMXON                  ,  27, "VMXON instruction."),
    EXIT_REASON(VMX_EXIT_MOV_CRX                ,  28, "Control-register accesses."),
    EXIT_REASON(VMX_EXIT_MOV_DRX                ,  29, "Debug-register accesses."),
    EXIT_REASON(VMX_EXIT_PORT_IO                ,  30, "I/O instruction."),
    EXIT_REASON(VMX_EXIT_RDMSR                  ,  31, "RDMSR instruction."),
    EXIT_REASON(VMX_EXIT_WRMSR                  ,  32, "WRMSR instruction."),
    EXIT_REASON(VMX_EXIT_ERR_INVALID_GUEST_STATE,  33, "VM-entry failure due to invalid guest state."),
    EXIT_REASON(VMX_EXIT_ERR_MSR_LOAD           ,  34, "VM-entry failure due to MSR loading."),
    EXIT_REASON_NIL(),
    EXIT_REASON(VMX_EXIT_MWAIT                  ,  36, "MWAIT instruction."),
    EXIT_REASON(VMX_EXIT_MTF                    ,  37, "Monitor Trap Flag."),
    EXIT_REASON_NIL(),
    EXIT_REASON(VMX_EXIT_MONITOR                ,  39, "MONITOR instruction."),
    EXIT_REASON(VMX_EXIT_PAUSE                  ,  40, "PAUSE instruction."),
    EXIT_REASON(VMX_EXIT_ERR_MACHINE_CHECK      ,  41, "VM-entry failure due to machine-check."),
    EXIT_REASON_NIL(),
    EXIT_REASON(VMX_EXIT_TPR_BELOW_THRESHOLD    ,  43, "TPR below threshold (MOV to CR8)."),
    EXIT_REASON(VMX_EXIT_APIC_ACCESS            ,  44, "APIC access."),
    EXIT_REASON(VMX_EXIT_VIRTUALIZED_EOI        ,  45, "Virtualized EOI."),
    EXIT_REASON(VMX_EXIT_XDTR_ACCESS            ,  46, "GDTR/IDTR access using LGDT/SGDT/LIDT/SIDT."),
    EXIT_REASON(VMX_EXIT_TR_ACCESS              ,  47, "LDTR/TR access using LLDT/SLDT/LTR/STR."),
    EXIT_REASON(VMX_EXIT_EPT_VIOLATION          ,  48, "EPT violation."),
    EXIT_REASON(VMX_EXIT_EPT_MISCONFIG          ,  49, "EPT misconfiguration."),
    EXIT_REASON(VMX_EXIT_INVEPT                 ,  50, "INVEPT instruction."),
    EXIT_REASON(VMX_EXIT_RDTSCP                 ,  51, "RDTSCP instruction."),
    EXIT_REASON(VMX_EXIT_PREEMPT_TIMER          ,  52, "VMX-preemption timer expired."),
    EXIT_REASON(VMX_EXIT_INVVPID                ,  53, "INVVPID instruction."),
    EXIT_REASON(VMX_EXIT_WBINVD                 ,  54, "WBINVD instruction."),
    EXIT_REASON(VMX_EXIT_XSETBV                 ,  55, "XSETBV instruction."),
    EXIT_REASON(VMX_EXIT_APIC_WRITE             ,  56, "APIC write completed to virtual-APIC page."),
    EXIT_REASON(VMX_EXIT_RDRAND                 ,  57, "RDRAND instruction."),
    EXIT_REASON(VMX_EXIT_INVPCID                ,  58, "INVPCID instruction."),
    EXIT_REASON(VMX_EXIT_VMFUNC                 ,  59, "VMFUNC instruction."),
    EXIT_REASON(VMX_EXIT_ENCLS                  ,  60, "ENCLS instrunction."),
    EXIT_REASON(VMX_EXIT_RDSEED                 ,  61, "RDSEED instruction."),
    EXIT_REASON(VMX_EXIT_PML_FULL               ,  62, "Page-modification log full."),
    EXIT_REASON(VMX_EXIT_XSAVES                 ,  63, "XSAVES instruction."),
    EXIT_REASON(VMX_EXIT_XRSTORS                ,  64, "XRSTORS instruction.")
};
/** Array index of the last valid VT-x exit reason. */
#define MAX_EXITREASON_VTX                         64

/** A partial list of Exit reason descriptions for AMD-V, used to describe
 *  statistics.
 *
 *  @note AMD-V have annoyingly large gaps (e.g. \#NPF VMEXIT comes at 1024),
 *        this array doesn't contain the entire set of exit reasons, we
 *        handle them via hmSvmGetSpecialExitReasonDesc(). */
static const char * const g_apszAmdVExitReasons[MAX_EXITREASON_STAT] =
{
    EXIT_REASON(SVM_EXIT_READ_CR0     ,    0, "Read CR0."),
    EXIT_REASON(SVM_EXIT_READ_CR1     ,    1, "Read CR1."),
    EXIT_REASON(SVM_EXIT_READ_CR2     ,    2, "Read CR2."),
    EXIT_REASON(SVM_EXIT_READ_CR3     ,    3, "Read CR3."),
    EXIT_REASON(SVM_EXIT_READ_CR4     ,    4, "Read CR4."),
    EXIT_REASON(SVM_EXIT_READ_CR5     ,    5, "Read CR5."),
    EXIT_REASON(SVM_EXIT_READ_CR6     ,    6, "Read CR6."),
    EXIT_REASON(SVM_EXIT_READ_CR7     ,    7, "Read CR7."),
    EXIT_REASON(SVM_EXIT_READ_CR8     ,    8, "Read CR8."),
    EXIT_REASON(SVM_EXIT_READ_CR9     ,    9, "Read CR9."),
    EXIT_REASON(SVM_EXIT_READ_CR10    ,   10, "Read CR10."),
    EXIT_REASON(SVM_EXIT_READ_CR11    ,   11, "Read CR11."),
    EXIT_REASON(SVM_EXIT_READ_CR12    ,   12, "Read CR12."),
    EXIT_REASON(SVM_EXIT_READ_CR13    ,   13, "Read CR13."),
    EXIT_REASON(SVM_EXIT_READ_CR14    ,   14, "Read CR14."),
    EXIT_REASON(SVM_EXIT_READ_CR15    ,   15, "Read CR15."),
    EXIT_REASON(SVM_EXIT_WRITE_CR0    ,   16, "Write CR0."),
    EXIT_REASON(SVM_EXIT_WRITE_CR1    ,   17, "Write CR1."),
    EXIT_REASON(SVM_EXIT_WRITE_CR2    ,   18, "Write CR2."),
    EXIT_REASON(SVM_EXIT_WRITE_CR3    ,   19, "Write CR3."),
    EXIT_REASON(SVM_EXIT_WRITE_CR4    ,   20, "Write CR4."),
    EXIT_REASON(SVM_EXIT_WRITE_CR5    ,   21, "Write CR5."),
    EXIT_REASON(SVM_EXIT_WRITE_CR6    ,   22, "Write CR6."),
    EXIT_REASON(SVM_EXIT_WRITE_CR7    ,   23, "Write CR7."),
    EXIT_REASON(SVM_EXIT_WRITE_CR8    ,   24, "Write CR8."),
    EXIT_REASON(SVM_EXIT_WRITE_CR9    ,   25, "Write CR9."),
    EXIT_REASON(SVM_EXIT_WRITE_CR10   ,   26, "Write CR10."),
    EXIT_REASON(SVM_EXIT_WRITE_CR11   ,   27, "Write CR11."),
    EXIT_REASON(SVM_EXIT_WRITE_CR12   ,   28, "Write CR12."),
    EXIT_REASON(SVM_EXIT_WRITE_CR13   ,   29, "Write CR13."),
    EXIT_REASON(SVM_EXIT_WRITE_CR14   ,   30, "Write CR14."),
    EXIT_REASON(SVM_EXIT_WRITE_CR15   ,   31, "Write CR15."),
    EXIT_REASON(SVM_EXIT_READ_DR0     ,   32, "Read DR0."),
    EXIT_REASON(SVM_EXIT_READ_DR1     ,   33, "Read DR1."),
    EXIT_REASON(SVM_EXIT_READ_DR2     ,   34, "Read DR2."),
    EXIT_REASON(SVM_EXIT_READ_DR3     ,   35, "Read DR3."),
    EXIT_REASON(SVM_EXIT_READ_DR4     ,   36, "Read DR4."),
    EXIT_REASON(SVM_EXIT_READ_DR5     ,   37, "Read DR5."),
    EXIT_REASON(SVM_EXIT_READ_DR6     ,   38, "Read DR6."),
    EXIT_REASON(SVM_EXIT_READ_DR7     ,   39, "Read DR7."),
    EXIT_REASON(SVM_EXIT_READ_DR8     ,   40, "Read DR8."),
    EXIT_REASON(SVM_EXIT_READ_DR9     ,   41, "Read DR9."),
    EXIT_REASON(SVM_EXIT_READ_DR10    ,   42, "Read DR10."),
    EXIT_REASON(SVM_EXIT_READ_DR11    ,   43, "Read DR11"),
    EXIT_REASON(SVM_EXIT_READ_DR12    ,   44, "Read DR12."),
    EXIT_REASON(SVM_EXIT_READ_DR13    ,   45, "Read DR13."),
    EXIT_REASON(SVM_EXIT_READ_DR14    ,   46, "Read DR14."),
    EXIT_REASON(SVM_EXIT_READ_DR15    ,   47, "Read DR15."),
    EXIT_REASON(SVM_EXIT_WRITE_DR0    ,   48, "Write DR0."),
    EXIT_REASON(SVM_EXIT_WRITE_DR1    ,   49, "Write DR1."),
    EXIT_REASON(SVM_EXIT_WRITE_DR2    ,   50, "Write DR2."),
    EXIT_REASON(SVM_EXIT_WRITE_DR3    ,   51, "Write DR3."),
    EXIT_REASON(SVM_EXIT_WRITE_DR4    ,   52, "Write DR4."),
    EXIT_REASON(SVM_EXIT_WRITE_DR5    ,   53, "Write DR5."),
    EXIT_REASON(SVM_EXIT_WRITE_DR6    ,   54, "Write DR6."),
    EXIT_REASON(SVM_EXIT_WRITE_DR7    ,   55, "Write DR7."),
    EXIT_REASON(SVM_EXIT_WRITE_DR8    ,   56, "Write DR8."),
    EXIT_REASON(SVM_EXIT_WRITE_DR9    ,   57, "Write DR9."),
    EXIT_REASON(SVM_EXIT_WRITE_DR10   ,   58, "Write DR10."),
    EXIT_REASON(SVM_EXIT_WRITE_DR11   ,   59, "Write DR11."),
    EXIT_REASON(SVM_EXIT_WRITE_DR12   ,   60, "Write DR12."),
    EXIT_REASON(SVM_EXIT_WRITE_DR13   ,   61, "Write DR13."),
    EXIT_REASON(SVM_EXIT_WRITE_DR14   ,   62, "Write DR14."),
    EXIT_REASON(SVM_EXIT_WRITE_DR15   ,   63, "Write DR15."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_0  ,   64, "Exception Vector 0  (#DE)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1  ,   65, "Exception Vector 1  (#DB)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_2  ,   66, "Exception Vector 2  (#NMI)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_3  ,   67, "Exception Vector 3  (#BP)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_4  ,   68, "Exception Vector 4  (#OF)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_5  ,   69, "Exception Vector 5  (#BR)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_6  ,   70, "Exception Vector 6  (#UD)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_7  ,   71, "Exception Vector 7  (#NM)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_8  ,   72, "Exception Vector 8  (#DF)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_9  ,   73, "Exception Vector 9  (#CO_SEG_OVERRUN)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_A  ,   74, "Exception Vector 10 (#TS)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_B  ,   75, "Exception Vector 11 (#NP)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_C  ,   76, "Exception Vector 12 (#SS)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_D  ,   77, "Exception Vector 13 (#GP)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_E  ,   78, "Exception Vector 14 (#PF)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_F  ,   79, "Exception Vector 15 (0x0f)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_10 ,   80, "Exception Vector 16 (#MF)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_11 ,   81, "Exception Vector 17 (#AC)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_12 ,   82, "Exception Vector 18 (#MC)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_13 ,   83, "Exception Vector 19 (#XF)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_14 ,   84, "Exception Vector 20 (0x14)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_15 ,   85, "Exception Vector 22 (0x15)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_16 ,   86, "Exception Vector 22 (0x16)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_17 ,   87, "Exception Vector 23 (0x17)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_18 ,   88, "Exception Vector 24 (0x18)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_19 ,   89, "Exception Vector 25 (0x19)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1A ,   90, "Exception Vector 26 (0x1A)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1B ,   91, "Exception Vector 27 (0x1B)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1C ,   92, "Exception Vector 28 (0x1C)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1D ,   93, "Exception Vector 29 (0x1D)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1E ,   94, "Exception Vector 30 (0x1E)."),
    EXIT_REASON(SVM_EXIT_EXCEPTION_1F ,   95, "Exception Vector 31 (0x1F)."),
    EXIT_REASON(SVM_EXIT_INTR         ,   96, "Physical maskable interrupt (host)."),
    EXIT_REASON(SVM_EXIT_NMI          ,   97, "Physical non-maskable interrupt (host)."),
    EXIT_REASON(SVM_EXIT_SMI          ,   98, "System management interrupt (host)."),
    EXIT_REASON(SVM_EXIT_INIT         ,   99, "Physical INIT signal (host)."),
    EXIT_REASON(SVM_EXIT_VINTR        ,  100, "Virtual interrupt-window exit."),
    EXIT_REASON(SVM_EXIT_CR0_SEL_WRITE,  101, "Write to CR0 that changed any bits other than CR0.TS or CR0.MP."),
    EXIT_REASON(SVM_EXIT_IDTR_READ    ,  102, "Read IDTR"),
    EXIT_REASON(SVM_EXIT_GDTR_READ    ,  103, "Read GDTR"),
    EXIT_REASON(SVM_EXIT_LDTR_READ    ,  104, "Read LDTR."),
    EXIT_REASON(SVM_EXIT_TR_READ      ,  105, "Read TR."),
    EXIT_REASON(SVM_EXIT_IDTR_WRITE   ,  106, "Write IDTR."),
    EXIT_REASON(SVM_EXIT_GDTR_WRITE   ,  107, "Write GDTR."),
    EXIT_REASON(SVM_EXIT_LDTR_WRITE   ,  108, "Write LDTR."),
    EXIT_REASON(SVM_EXIT_TR_WRITE     ,  109, "Write TR."),
    EXIT_REASON(SVM_EXIT_RDTSC        ,  110, "RDTSC instruction."),
    EXIT_REASON(SVM_EXIT_RDPMC        ,  111, "RDPMC instruction."),
    EXIT_REASON(SVM_EXIT_PUSHF        ,  112, "PUSHF instruction."),
    EXIT_REASON(SVM_EXIT_POPF         ,  113, "POPF instruction."),
    EXIT_REASON(SVM_EXIT_CPUID        ,  114, "CPUID instruction."),
    EXIT_REASON(SVM_EXIT_RSM          ,  115, "RSM instruction."),
    EXIT_REASON(SVM_EXIT_IRET         ,  116, "IRET instruction."),
    EXIT_REASON(SVM_EXIT_SWINT        ,  117, "Software interrupt (INTn instructions)."),
    EXIT_REASON(SVM_EXIT_INVD         ,  118, "INVD instruction."),
    EXIT_REASON(SVM_EXIT_PAUSE        ,  119, "PAUSE instruction."),
    EXIT_REASON(SVM_EXIT_HLT          ,  120, "HLT instruction."),
    EXIT_REASON(SVM_EXIT_INVLPG       ,  121, "INVLPG instruction."),
    EXIT_REASON(SVM_EXIT_INVLPGA      ,  122, "INVLPGA instruction."),
    EXIT_REASON(SVM_EXIT_IOIO         ,  123, "IN/OUT accessing protected port."),
    EXIT_REASON(SVM_EXIT_MSR          ,  124, "RDMSR or WRMSR access to protected MSR."),
    EXIT_REASON(SVM_EXIT_TASK_SWITCH  ,  125, "Task switch."),
    EXIT_REASON(SVM_EXIT_FERR_FREEZE  ,  126, "Legacy FPU handling enabled; CPU frozen in an x87/mmx instr. waiting for interrupt."),
    EXIT_REASON(SVM_EXIT_SHUTDOWN     ,  127, "Shutdown."),
    EXIT_REASON(SVM_EXIT_VMRUN        ,  128, "VMRUN instruction."),
    EXIT_REASON(SVM_EXIT_VMMCALL      ,  129, "VMCALL instruction."),
    EXIT_REASON(SVM_EXIT_VMLOAD       ,  130, "VMLOAD instruction."),
    EXIT_REASON(SVM_EXIT_VMSAVE       ,  131, "VMSAVE instruction."),
    EXIT_REASON(SVM_EXIT_STGI         ,  132, "STGI instruction."),
    EXIT_REASON(SVM_EXIT_CLGI         ,  133, "CLGI instruction."),
    EXIT_REASON(SVM_EXIT_SKINIT       ,  134, "SKINIT instruction."),
    EXIT_REASON(SVM_EXIT_RDTSCP       ,  135, "RDTSCP instruction."),
    EXIT_REASON(SVM_EXIT_ICEBP        ,  136, "ICEBP instruction."),
    EXIT_REASON(SVM_EXIT_WBINVD       ,  137, "WBINVD instruction."),
    EXIT_REASON(SVM_EXIT_MONITOR      ,  138, "MONITOR instruction."),
    EXIT_REASON(SVM_EXIT_MWAIT        ,  139, "MWAIT instruction."),
    EXIT_REASON(SVM_EXIT_MWAIT_ARMED  ,  140, "MWAIT instruction when armed."),
    EXIT_REASON(SVM_EXIT_XSETBV       ,  141, "XSETBV instruction."),
};
/** Array index of the last valid AMD-V exit reason. */
#define MAX_EXITREASON_AMDV              141

/** Special exit reasons not covered in the array above. */
#define SVM_EXIT_REASON_NPF                  EXIT_REASON(SVM_EXIT_NPF                , 1024, "Nested Page Fault.")
#define SVM_EXIT_REASON_AVIC_INCOMPLETE_IPI  EXIT_REASON(SVM_EXIT_AVIC_INCOMPLETE_IPI, 1025, "AVIC - Incomplete IPI delivery.")
#define SVM_EXIT_REASON_AVIC_NOACCEL         EXIT_REASON(SVM_EXIT_AVIC_NOACCEL       , 1026, "AVIC - Unhandled register.")

/**
 * Gets the SVM exit reason if it's one of the reasons not present in the @c
 * g_apszAmdVExitReasons array.
 *
 * @returns The exit reason or NULL if unknown.
 * @param   uExit       The exit.
 */
DECLINLINE(const char *) hmSvmGetSpecialExitReasonDesc(uint16_t uExit)
{
    switch (uExit)
    {
        case SVM_EXIT_NPF:                 return SVM_EXIT_REASON_NPF;
        case SVM_EXIT_AVIC_INCOMPLETE_IPI: return SVM_EXIT_REASON_AVIC_INCOMPLETE_IPI;
        case SVM_EXIT_AVIC_NOACCEL:        return SVM_EXIT_REASON_AVIC_NOACCEL;
    }
    return EXIT_REASON_NIL();
}
#undef EXIT_REASON_NIL
#undef EXIT_REASON

/** @def HMVMX_REPORT_FEATURE
 * Reports VT-x feature to the release log.
 *
 * @param   allowed1        Mask of allowed feature bits.
 * @param   disallowed0     Mask of disallowed feature bits.
 * @param   strdesc         The description string to report.
 * @param   featflag        Mask of the feature to report.
 */
#define HMVMX_REPORT_FEATURE(allowed1, disallowed0, strdesc, featflag) \
    do { \
        if ((allowed1) & (featflag)) \
        { \
            if ((disallowed0) & (featflag)) \
                LogRel(("HM:   " strdesc " (must be set)\n")); \
            else \
                LogRel(("HM:   " strdesc "\n")); \
        } \
        else \
            LogRel(("HM:   " strdesc " (must be cleared)\n")); \
    } while (0)

/** @def HMVMX_REPORT_ALLOWED_FEATURE
 * Reports an allowed VT-x feature to the release log.
 *
 * @param   allowed1        Mask of allowed feature bits.
 * @param   strdesc         The description string to report.
 * @param   featflag        Mask of the feature to report.
 */
#define HMVMX_REPORT_ALLOWED_FEATURE(allowed1, strdesc, featflag) \
    do { \
        if ((allowed1) & (featflag)) \
            LogRel(("HM:   " strdesc "\n")); \
        else \
            LogRel(("HM:   " strdesc " not supported\n")); \
    } while (0)

/** @def HMVMX_REPORT_MSR_CAPABILITY
 * Reports MSR feature capability.
 *
 * @param   msrcaps         Mask of MSR feature bits.
 * @param   strdesc         The description string to report.
 * @param   cap             Mask of the feature to report.
 */
#define HMVMX_REPORT_MSR_CAPABILITY(msrcaps, strdesc, cap) \
    do { \
        if ((msrcaps) & (cap)) \
            LogRel(("HM:   " strdesc "\n")); \
    } while (0)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)  hmR3Save(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int)  hmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
static DECLCALLBACK(void) hmR3InfoExitHistory(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static DECLCALLBACK(void) hmR3InfoEventPending(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);
static int                hmR3InitCPU(PVM pVM);
static int                hmR3InitFinalizeR0(PVM pVM);
static int                hmR3InitFinalizeR0Intel(PVM pVM);
static int                hmR3InitFinalizeR0Amd(PVM pVM);
static int                hmR3TermCPU(PVM pVM);



/**
 * Initializes the HM.
 *
 * This reads the config and check whether VT-x or AMD-V hardware is available
 * if configured to use it.  This is one of the very first components to be
 * initialized after CFGM, so that we can fall back to raw-mode early in the
 * initialization process.
 *
 * Note that a lot of the set up work is done in ring-0 and thus postponed till
 * the ring-3 and ring-0 callback to HMR3InitCompleted.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 *
 * @remarks Be careful with what we call here, since most of the VMM components
 *          are uninitialized.
 */
VMMR3_INT_DECL(int) HMR3Init(PVM pVM)
{
    LogFlow(("HMR3Init\n"));

    /*
     * Assert alignment and sizes.
     */
    AssertCompileMemberAlignment(VM, hm.s, 32);
    AssertCompile(sizeof(pVM->hm.s) <= sizeof(pVM->hm.padding));

    /*
     * Register the saved state data unit.
     */
    int rc = SSMR3RegisterInternal(pVM, "HWACCM", 0, HM_SAVED_STATE_VERSION, sizeof(HM),
                                   NULL, NULL, NULL,
                                   NULL, hmR3Save, NULL,
                                   NULL, hmR3Load, NULL);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register info handlers.
     */
    rc = DBGFR3InfoRegisterInternalEx(pVM, "exithistory", "Dumps the HM VM-exit history.", hmR3InfoExitHistory,
                                      DBGFINFO_FLAGS_ALL_EMTS);
    AssertRCReturn(rc, rc);

    rc = DBGFR3InfoRegisterInternalEx(pVM, "hmeventpending", "Dumps the pending HM event.", hmR3InfoEventPending,
                                      DBGFINFO_FLAGS_ALL_EMTS);
    AssertRCReturn(rc, rc);

    /*
     * Read configuration.
     */
    PCFGMNODE pCfgHm = CFGMR3GetChild(CFGMR3GetRoot(pVM), "HM/");

    /*
     * Validate the HM settings.
     */
    rc = CFGMR3ValidateConfig(pCfgHm, "/HM/",
                              "HMForced"
                              "|EnableNestedPaging"
                              "|EnableUX"
                              "|EnableLargePages"
                              "|EnableVPID"
                              "|IBPBOnVMExit"
                              "|IBPBOnVMEntry"
                              "|SpecCtrlByHost"
                              "|TPRPatchingEnabled"
                              "|64bitEnabled"
                              "|VmxPleGap"
                              "|VmxPleWindow"
                              "|SvmPauseFilter"
                              "|SvmPauseFilterThreshold"
                              "|Exclusive"
                              "|MaxResumeLoops"
                              "|UseVmxPreemptTimer",
                              "" /* pszValidNodes */, "HM" /* pszWho */, 0 /* uInstance */);
    if (RT_FAILURE(rc))
        return rc;

    /** @cfgm{/HM/HMForced, bool, false}
     * Forces hardware virtualization, no falling back on raw-mode. HM must be
     * enabled, i.e. /HMEnabled must be true. */
    bool fHMForced;
#ifdef VBOX_WITH_RAW_MODE
    rc = CFGMR3QueryBoolDef(pCfgHm, "HMForced", &fHMForced, false);
    AssertRCReturn(rc, rc);
    AssertLogRelMsgReturn(!fHMForced || pVM->fHMEnabled, ("Configuration error: HM forced but not enabled!\n"),
                          VERR_INVALID_PARAMETER);
# if defined(RT_OS_DARWIN)
    if (pVM->fHMEnabled)
        fHMForced = true;
# endif
    AssertLogRelMsgReturn(pVM->cCpus == 1 || pVM->fHMEnabled, ("Configuration error: SMP requires HM to be enabled!\n"),
                          VERR_INVALID_PARAMETER);
    if (pVM->cCpus > 1)
        fHMForced = true;
#else  /* !VBOX_WITH_RAW_MODE */
    AssertRelease(pVM->fHMEnabled);
    fHMForced = true;
#endif /* !VBOX_WITH_RAW_MODE */

    /** @cfgm{/HM/EnableNestedPaging, bool, false}
     * Enables nested paging (aka extended page tables). */
    rc = CFGMR3QueryBoolDef(pCfgHm, "EnableNestedPaging", &pVM->hm.s.fAllowNestedPaging, false);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/EnableUX, bool, true}
     * Enables the VT-x unrestricted execution feature. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "EnableUX", &pVM->hm.s.vmx.fAllowUnrestricted, true);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/EnableLargePages, bool, false}
     * Enables using large pages (2 MB) for guest memory, thus saving on (nested)
     * page table walking and maybe better TLB hit rate in some cases. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "EnableLargePages", &pVM->hm.s.fLargePages, false);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/EnableVPID, bool, false}
     * Enables the VT-x VPID feature. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "EnableVPID", &pVM->hm.s.vmx.fAllowVpid, false);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/TPRPatchingEnabled, bool, false}
     * Enables TPR patching for 32-bit windows guests with IO-APIC. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "TPRPatchingEnabled", &pVM->hm.s.fTprPatchingAllowed, false);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/64bitEnabled, bool, 32-bit:false, 64-bit:true}
     * Enables AMD64 cpu features.
     * On 32-bit hosts this isn't default and require host CPU support. 64-bit hosts
     * already have the support. */
#ifdef VBOX_ENABLE_64_BITS_GUESTS
    rc = CFGMR3QueryBoolDef(pCfgHm, "64bitEnabled", &pVM->hm.s.fAllow64BitGuests, HC_ARCH_BITS == 64);
    AssertLogRelRCReturn(rc, rc);
#else
    pVM->hm.s.fAllow64BitGuests = false;
#endif

    /** @cfgm{/HM/VmxPleGap, uint32_t, 0}
     * The pause-filter exiting gap in TSC ticks. When the number of ticks between
     * two successive PAUSE instructions exceeds VmxPleGap, the CPU considers the
     * latest PAUSE instruction to be start of a new PAUSE loop.
     */
    rc = CFGMR3QueryU32Def(pCfgHm, "VmxPleGap", &pVM->hm.s.vmx.cPleGapTicks, 0);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/VmxPleWindow, uint32_t, 0}
     * The pause-filter exiting window in TSC ticks. When the number of ticks
     * between the current PAUSE instruction and first PAUSE of a loop exceeds
     * VmxPleWindow, a VM-exit is triggered.
     *
     * Setting VmxPleGap and VmxPleGap to 0 disables pause-filter exiting.
     */
    rc = CFGMR3QueryU32Def(pCfgHm, "VmxPleWindow", &pVM->hm.s.vmx.cPleWindowTicks, 0);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/SvmPauseFilterCount, uint16_t, 0}
     * A counter that is decrement each time a PAUSE instruction is executed by the
     * guest. When the counter is 0, a \#VMEXIT is triggered.
     */
    rc = CFGMR3QueryU16Def(pCfgHm, "SvmPauseFilter", &pVM->hm.s.svm.cPauseFilter, 0);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/SvmPauseFilterThreshold, uint16_t, 0}
     * The pause filter threshold in ticks. When the elapsed time between two
     * successive PAUSE instructions exceeds SvmPauseFilterThreshold, the PauseFilter
     * count is reset to its initial value. However, if PAUSE is executed PauseFilter
     * times within PauseFilterThreshold ticks, a VM-exit will be triggered.
     *
     * Setting both SvmPauseFilterCount and SvmPauseFilterCount to 0 disables
     * pause-filter exiting.
     */
    rc = CFGMR3QueryU16Def(pCfgHm, "SvmPauseFilterThreshold", &pVM->hm.s.svm.cPauseFilterThresholdTicks, 0);
    AssertRCReturn(rc, rc);

    /** @cfgm{/HM/Exclusive, bool}
     * Determines the init method for AMD-V and VT-x. If set to true, HM will do a
     * global init for each host CPU.  If false, we do local init each time we wish
     * to execute guest code.
     *
     * On Windows, default is false due to the higher risk of conflicts with other
     * hypervisors.
     *
     * On Mac OS X, this setting is ignored since the code does not handle local
     * init when it utilizes the OS provided VT-x function, SUPR0EnableVTx().
     */
#if defined(RT_OS_DARWIN)
    pVM->hm.s.fGlobalInit = true;
#else
    rc = CFGMR3QueryBoolDef(pCfgHm, "Exclusive", &pVM->hm.s.fGlobalInit,
# if defined(RT_OS_WINDOWS)
                            false
# else
                            true
# endif
                           );
    AssertLogRelRCReturn(rc, rc);
#endif

    /** @cfgm{/HM/MaxResumeLoops, uint32_t}
     * The number of times to resume guest execution before we forcibly return to
     * ring-3.  The return value of RTThreadPreemptIsPendingTrusty in ring-0
     * determines the default value. */
    rc = CFGMR3QueryU32Def(pCfgHm, "MaxResumeLoops", &pVM->hm.s.cMaxResumeLoops, 0 /* set by R0 later */);
    AssertLogRelRCReturn(rc, rc);

    /** @cfgm{/HM/UseVmxPreemptTimer, bool}
     * Whether to make use of the VMX-preemption timer feature of the CPU if it's
     * available. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "UseVmxPreemptTimer", &pVM->hm.s.vmx.fUsePreemptTimer, true);
    AssertLogRelRCReturn(rc, rc);

    /** @cfgm{/HM/IBPBOnVMExit, bool}
     * Costly paranoia setting. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "IBPBOnVMExit", &pVM->hm.s.fIbpbOnVmExit, false);
    AssertLogRelRCReturn(rc, rc);

    /** @cfgm{/HM/IBPBOnVMEntry, bool}
     * Costly paranoia setting. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "IBPBOnVMEntry", &pVM->hm.s.fIbpbOnVmEntry, false);
    AssertLogRelRCReturn(rc, rc);

    /** @cfgm{/HM/SpecCtrlByHost, bool}
     * Another expensive paranoia setting. */
    rc = CFGMR3QueryBoolDef(pCfgHm, "SpecCtrlByHost", &pVM->hm.s.fSpecCtrlByHost, false);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Check if VT-x or AMD-v support according to the users wishes.
     */
    /** @todo SUPR3QueryVTCaps won't catch VERR_VMX_IN_VMX_ROOT_MODE or
     *        VERR_SVM_IN_USE. */
    if (pVM->fHMEnabled)
    {
        uint32_t fCaps;
        rc = SUPR3QueryVTCaps(&fCaps);
        if (RT_SUCCESS(rc))
        {
            if (fCaps & SUPVTCAPS_AMD_V)
            {
                LogRel(("HM: HMR3Init: AMD-V%s\n", fCaps & SUPVTCAPS_NESTED_PAGING ? " w/ nested paging" : ""));
                pVM->hm.s.svm.fSupported = true;
            }
            else if (fCaps & SUPVTCAPS_VT_X)
            {
                rc = SUPR3QueryVTxSupported();
                if (RT_SUCCESS(rc))
                {
                    LogRel(("HM: HMR3Init: VT-x%s%s%s\n",
                            fCaps & SUPVTCAPS_NESTED_PAGING ? " w/ nested paging" : "",
                            fCaps & SUPVTCAPS_VTX_UNRESTRICTED_GUEST ? " and unrestricted guest execution" : "",
                            (fCaps & (SUPVTCAPS_NESTED_PAGING | SUPVTCAPS_VTX_UNRESTRICTED_GUEST)) ? " hw support" : ""));
                    pVM->hm.s.vmx.fSupported = true;
                }
                else
                {
#ifdef RT_OS_LINUX
                    const char *pszMinReq = " Linux 2.6.13 or newer required!";
#else
                    const char *pszMinReq = "";
#endif
                    if (fHMForced)
                        return VMSetError(pVM, rc, RT_SRC_POS, "The host kernel does not support VT-x.%s\n", pszMinReq);

                    /* Fall back to raw-mode. */
                    LogRel(("HM: HMR3Init: Falling back to raw-mode: The host kernel does not support VT-x.%s\n", pszMinReq));
                    pVM->fHMEnabled = false;
                }
            }
            else
                AssertLogRelMsgFailedReturn(("SUPR3QueryVTCaps didn't return either AMD-V or VT-x flag set (%#x)!\n", fCaps),
                                            VERR_INTERNAL_ERROR_5);

            /*
             * Do we require a little bit or raw-mode for 64-bit guest execution?
             */
            pVM->fHMNeedRawModeCtx = HC_ARCH_BITS == 32
                                  && pVM->fHMEnabled
                                  && pVM->hm.s.fAllow64BitGuests;

            /*
             * Disable nested paging and unrestricted guest execution now if they're
             * configured so that CPUM can make decisions based on our configuration.
             */
            Assert(!pVM->hm.s.fNestedPaging);
            if (pVM->hm.s.fAllowNestedPaging)
            {
                if (fCaps & SUPVTCAPS_NESTED_PAGING)
                    pVM->hm.s.fNestedPaging = true;
                else
                    pVM->hm.s.fAllowNestedPaging = false;
            }

            if (fCaps & SUPVTCAPS_VT_X)
            {
                Assert(!pVM->hm.s.vmx.fUnrestrictedGuest);
                if (pVM->hm.s.vmx.fAllowUnrestricted)
                {
                    if (   (fCaps & SUPVTCAPS_VTX_UNRESTRICTED_GUEST)
                        && pVM->hm.s.fNestedPaging)
                        pVM->hm.s.vmx.fUnrestrictedGuest = true;
                    else
                        pVM->hm.s.vmx.fAllowUnrestricted = false;
                }
            }
        }
        else
        {
            const char *pszMsg;
            switch (rc)
            {
                case VERR_UNSUPPORTED_CPU:
                    pszMsg = "Unknown CPU, VT-x or AMD-v features cannot be ascertained";
                    break;

                case VERR_VMX_NO_VMX:
                    pszMsg = "VT-x is not available";
                    break;

                case VERR_VMX_MSR_VMX_DISABLED:
                    pszMsg = "VT-x is disabled in the BIOS";
                    break;

                case VERR_VMX_MSR_ALL_VMX_DISABLED:
                    pszMsg = "VT-x is disabled in the BIOS for all CPU modes";
                    break;

                case VERR_VMX_MSR_LOCKING_FAILED:
                    pszMsg = "Failed to enable and lock VT-x features";
                    break;

                case VERR_SVM_NO_SVM:
                    pszMsg = "AMD-V is not available";
                    break;

                case VERR_SVM_DISABLED:
                    pszMsg = "AMD-V is disabled in the BIOS (or by the host OS)";
                    break;

                default:
                    pszMsg = NULL;
                    break;
            }
            if (fHMForced && pszMsg)
                return VM_SET_ERROR(pVM, rc, pszMsg);
            if (!pszMsg)
                return VMSetError(pVM, rc, RT_SRC_POS, "SUPR3QueryVTCaps failed with %Rrc", rc);

            /* Fall back to raw-mode. */
            LogRel(("HM: HMR3Init: Falling back to raw-mode: %s\n", pszMsg));
            pVM->fHMEnabled = false;
        }
    }

    /* It's now OK to use the predicate function. */
    pVM->fHMEnabledFixed = true;
    return VINF_SUCCESS;
}


/**
 * Initializes the per-VCPU HM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
static int hmR3InitCPU(PVM pVM)
{
    LogFlow(("HMR3InitCPU\n"));

    if (!HMIsEnabled(pVM))
        return VINF_SUCCESS;

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        pVCpu->hm.s.fActive = false;
    }

#ifdef VBOX_WITH_STATISTICS
    STAM_REG(pVM, &pVM->hm.s.StatTprPatchSuccess,   STAMTYPE_COUNTER, "/HM/TPR/Patch/Success",  STAMUNIT_OCCURENCES, "Number of times an instruction was successfully patched.");
    STAM_REG(pVM, &pVM->hm.s.StatTprPatchFailure,   STAMTYPE_COUNTER, "/HM/TPR/Patch/Failed",   STAMUNIT_OCCURENCES, "Number of unsuccessful patch attempts.");
    STAM_REG(pVM, &pVM->hm.s.StatTprReplaceSuccessCr8, STAMTYPE_COUNTER, "/HM/TPR/Replace/SuccessCR8",STAMUNIT_OCCURENCES, "Number of instruction replacements by MOV CR8.");
    STAM_REG(pVM, &pVM->hm.s.StatTprReplaceSuccessVmc, STAMTYPE_COUNTER, "/HM/TPR/Replace/SuccessVMC",STAMUNIT_OCCURENCES, "Number of instruction replacements by VMMCALL.");
    STAM_REG(pVM, &pVM->hm.s.StatTprReplaceFailure, STAMTYPE_COUNTER, "/HM/TPR/Replace/Failed", STAMUNIT_OCCURENCES, "Number of unsuccessful replace attempts.");
#endif

    /*
     * Statistics.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        int    rc;

#ifdef VBOX_WITH_STATISTICS
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatPoke, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of RTMpPokeCpu",
                             "/PROF/CPU%d/HM/Poke", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatSpinPoke, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of poke wait",
                             "/PROF/CPU%d/HM/PokeWait", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatSpinPokeFailed, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of poke wait when RTMpPokeCpu fails",
                             "/PROF/CPU%d/HM/PokeWaitFailed", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatEntry, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of VMXR0RunGuestCode entry",
                             "/PROF/CPU%d/HM/StatEntry", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExit1, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of VMXR0RunGuestCode exit part 1",
                             "/PROF/CPU%d/HM/SwitchFromGC_1", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExit2, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of VMXR0RunGuestCode exit part 2",
                             "/PROF/CPU%d/HM/SwitchFromGC_2", i);
        AssertRC(rc);

        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitIO, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "I/O",
                             "/PROF/CPU%d/HM/SwitchFromGC_2/IO", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitMovCRx, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "MOV CRx",
                             "/PROF/CPU%d/HM/SwitchFromGC_2/MovCRx", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitXcptNmi, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Exceptions, NMIs",
                             "/PROF/CPU%d/HM/SwitchFromGC_2/XcptNmi", i);
        AssertRC(rc);

        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatLoadGuestState, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of VMXR0LoadGuestState",
                             "/PROF/CPU%d/HM/StatLoadGuestState", i);
        AssertRC(rc);
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatInGC, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL,
                             "Profiling of VMLAUNCH/VMRESUME.",
                             "/PROF/CPU%d/HM/InGC", i);
        AssertRC(rc);

# if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatWorldSwitch3264, STAMTYPE_PROFILE, STAMVISIBILITY_USED,
                             STAMUNIT_TICKS_PER_CALL, "Profiling of the 32/64 switcher.",
                             "/PROF/CPU%d/HM/Switcher3264", i);
        AssertRC(rc);
# endif

# ifdef HM_PROFILE_EXIT_DISPATCH
        rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitDispatch, STAMTYPE_PROFILE_ADV, STAMVISIBILITY_USED,
                             STAMUNIT_TICKS_PER_CALL, "Profiling the dispatching of exit handlers.",
                             "/PROF/CPU%d/HM/ExitDispatch", i);
        AssertRC(rc);
# endif

#endif
# define HM_REG_COUNTER(a, b, desc) \
        rc = STAMR3RegisterF(pVM, a, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, desc, b, i); \
        AssertRC(rc);

#ifdef VBOX_WITH_STATISTICS
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitAll,                "/HM/CPU%d/Exit/All", "Exits (total).");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitShadowNM,           "/HM/CPU%d/Exit/Trap/Shw/#NM", "Shadow #NM (device not available, no math co-processor) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestNM,            "/HM/CPU%d/Exit/Trap/Gst/#NM", "Guest #NM (device not available, no math co-processor) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitShadowPF,           "/HM/CPU%d/Exit/Trap/Shw/#PF", "Shadow #PF (page fault) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitShadowPFEM,         "/HM/CPU%d/Exit/Trap/Shw/#PF-EM", "#PF (page fault) exception going back to ring-3 for emulating the instruction.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestPF,            "/HM/CPU%d/Exit/Trap/Gst/#PF", "Guest #PF (page fault) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestUD,            "/HM/CPU%d/Exit/Trap/Gst/#UD", "Guest #UD (undefined opcode) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestSS,            "/HM/CPU%d/Exit/Trap/Gst/#SS", "Guest #SS (stack-segment fault) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestNP,            "/HM/CPU%d/Exit/Trap/Gst/#NP", "Guest #NP (segment not present) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestGP,            "/HM/CPU%d/Exit/Trap/Gst/#GP", "Guest #GP (general protection) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestMF,            "/HM/CPU%d/Exit/Trap/Gst/#MF", "Guest #MF (x87 FPU error, math fault) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestDE,            "/HM/CPU%d/Exit/Trap/Gst/#DE", "Guest #DE (divide error) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestDB,            "/HM/CPU%d/Exit/Trap/Gst/#DB", "Guest #DB (debug) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestBP,            "/HM/CPU%d/Exit/Trap/Gst/#BP", "Guest #BP (breakpoint) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestXF,            "/HM/CPU%d/Exit/Trap/Gst/#XF", "Guest #XF (extended math fault, SIMD FPU) exception.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitGuestXcpUnk,        "/HM/CPU%d/Exit/Trap/Gst/Other", "Other guest exceptions.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitInvlpg,             "/HM/CPU%d/Exit/Instr/Invlpg", "Guest attempted to execute INVLPG.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitInvd,               "/HM/CPU%d/Exit/Instr/Invd", "Guest attempted to execute INVD.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitWbinvd,             "/HM/CPU%d/Exit/Instr/Wbinvd", "Guest attempted to execute WBINVD.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitPause,              "/HM/CPU%d/Exit/Instr/Pause", "Guest attempted to execute PAUSE.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitCpuid,              "/HM/CPU%d/Exit/Instr/Cpuid", "Guest attempted to execute CPUID.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitRdtsc,              "/HM/CPU%d/Exit/Instr/Rdtsc", "Guest attempted to execute RDTSC.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitRdtscp,             "/HM/CPU%d/Exit/Instr/Rdtscp", "Guest attempted to execute RDTSCP.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitRdpmc,              "/HM/CPU%d/Exit/Instr/Rdpmc", "Guest attempted to execute RDPMC.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitRdrand,             "/HM/CPU%d/Exit/Instr/Rdrand", "Guest attempted to execute RDRAND.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitRdmsr,              "/HM/CPU%d/Exit/Instr/Rdmsr", "Guest attempted to execute RDMSR.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitWrmsr,              "/HM/CPU%d/Exit/Instr/Wrmsr", "Guest attempted to execute WRMSR.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitMwait,              "/HM/CPU%d/Exit/Instr/Mwait", "Guest attempted to execute MWAIT.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitMonitor,            "/HM/CPU%d/Exit/Instr/Monitor", "Guest attempted to execute MONITOR.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitDRxWrite,           "/HM/CPU%d/Exit/Instr/DR/Write", "Guest attempted to write a debug register.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitDRxRead,            "/HM/CPU%d/Exit/Instr/DR/Read", "Guest attempted to read a debug register.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitClts,               "/HM/CPU%d/Exit/Instr/CLTS", "Guest attempted to execute CLTS.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitLmsw,               "/HM/CPU%d/Exit/Instr/LMSW", "Guest attempted to execute LMSW.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitCli,                "/HM/CPU%d/Exit/Instr/Cli", "Guest attempted to execute CLI.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitSti,                "/HM/CPU%d/Exit/Instr/Sti", "Guest attempted to execute STI.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitPushf,              "/HM/CPU%d/Exit/Instr/Pushf", "Guest attempted to execute PUSHF.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitPopf,               "/HM/CPU%d/Exit/Instr/Popf", "Guest attempted to execute POPF.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitIret,               "/HM/CPU%d/Exit/Instr/Iret", "Guest attempted to execute IRET.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitInt,                "/HM/CPU%d/Exit/Instr/Int", "Guest attempted to execute INT.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitHlt,                "/HM/CPU%d/Exit/Instr/Hlt", "Guest attempted to execute HLT.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitXdtrAccess,         "/HM/CPU%d/Exit/Instr/XdtrAccess", "Guest attempted to access descriptor table register (GDTR, IDTR, LDTR).");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitIOWrite,            "/HM/CPU%d/Exit/IO/Write", "I/O write.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitIORead,             "/HM/CPU%d/Exit/IO/Read", "I/O read.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitIOStringWrite,      "/HM/CPU%d/Exit/IO/WriteString", "String I/O write.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitIOStringRead,       "/HM/CPU%d/Exit/IO/ReadString", "String I/O read.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitIntWindow,          "/HM/CPU%d/Exit/IntWindow", "Interrupt-window exit. Guest is ready to receive interrupts again.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitExtInt,             "/HM/CPU%d/Exit/ExtInt", "Host interrupt received.");
#endif
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitHostNmiInGC,        "/HM/CPU%d/Exit/HostNmiInGC", "Host NMI received while in guest context.");
#ifdef VBOX_WITH_STATISTICS
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitPreemptTimer,       "/HM/CPU%d/Exit/PreemptTimer", "VMX-preemption timer expired.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitTprBelowThreshold,  "/HM/CPU%d/Exit/TprBelowThreshold", "TPR lowered below threshold by the guest.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitTaskSwitch,         "/HM/CPU%d/Exit/TaskSwitch", "Guest attempted a task switch.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitMtf,                "/HM/CPU%d/Exit/MonitorTrapFlag", "Monitor Trap Flag.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatExitApicAccess,         "/HM/CPU%d/Exit/ApicAccess", "APIC access. Guest attempted to access memory at a physical address on the APIC-access page.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchTprMaskedIrq,     "/HM/CPU%d/Switch/TprMaskedIrq", "PDMGetInterrupt() signals TPR masks pending Irq.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchGuestIrq,         "/HM/CPU%d/Switch/IrqPending", "PDMGetInterrupt() cleared behind our back!?!.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatPendingHostIrq,         "/HM/CPU%d/Switch/PendingHostIrq", "Exit to ring-3 due to pending host interrupt before executing guest code.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchHmToR3FF,         "/HM/CPU%d/Switch/HmToR3FF", "Exit to ring-3 due to pending timers, EMT rendezvous, critical section etc.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchExitToR3,         "/HM/CPU%d/Switch/ExitToR3", "Exit to ring-3 (total).");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchLongJmpToR3,      "/HM/CPU%d/Switch/LongJmpToR3", "Longjump to ring-3.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchMaxResumeLoops,   "/HM/CPU%d/Switch/MaxResumeToR3", "Maximum VMRESUME inner-loop counter reached.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchHltToR3,          "/HM/CPU%d/Switch/HltToR3", "HLT causing us to go to ring-3.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchApicAccessToR3,   "/HM/CPU%d/Switch/ApicAccessToR3", "APIC access causing us to go to ring-3.");
#endif
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchPreempt,          "/HM/CPU%d/Switch/Preempting", "EMT has been preempted while in HM context.");
#ifdef VBOX_WITH_STATISTICS
        HM_REG_COUNTER(&pVCpu->hm.s.StatSwitchPreemptSaveHostState, "/HM/CPU%d/Switch/SaveHostState", "Preemption caused us to resave host state.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatInjectInterrupt,        "/HM/CPU%d/EventInject/Interrupt", "Injected an external interrupt into the guest.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatInjectXcpt,             "/HM/CPU%d/EventInject/Trap", "Injected an exception into the guest.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatInjectPendingReflect,   "/HM/CPU%d/EventInject/PendingReflect", "Reflecting an exception (or #DF) caused due to event injection.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatInjectPendingInterpret, "/HM/CPU%d/EventInject/PendingInterpret", "Falling to interpreter for handling exception caused due to event injection.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushPage,              "/HM/CPU%d/Flush/Page", "Invalidating a guest page on all guest CPUs.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushPageManual,        "/HM/CPU%d/Flush/Page/Virt", "Invalidating a guest page using guest-virtual address.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushPhysPageManual,    "/HM/CPU%d/Flush/Page/Phys", "Invalidating a guest page using guest-physical address.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushTlb,               "/HM/CPU%d/Flush/TLB", "Forcing a full guest-TLB flush (ring-0).");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushTlbManual,         "/HM/CPU%d/Flush/TLB/Manual", "Request a full guest-TLB flush.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushTlbWorldSwitch,    "/HM/CPU%d/Flush/TLB/CpuSwitch", "Forcing a full guest-TLB flush due to host-CPU reschedule or ASID-limit hit by another guest-VCPU.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatNoFlushTlbWorldSwitch,  "/HM/CPU%d/Flush/TLB/Skipped", "No TLB flushing required.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushEntire,            "/HM/CPU%d/Flush/TLB/Entire", "Flush the entire TLB (host + guest).");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushAsid,              "/HM/CPU%d/Flush/TLB/ASID", "Flushed guest-TLB entries for the current VPID.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushNestedPaging,      "/HM/CPU%d/Flush/TLB/NestedPaging", "Flushed guest-TLB entries for the current EPT.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushTlbInvlpgVirt,     "/HM/CPU%d/Flush/TLB/InvlpgVirt", "Invalidated a guest-TLB entry for a guest-virtual address.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatFlushTlbInvlpgPhys,     "/HM/CPU%d/Flush/TLB/InvlpgPhys", "Currently not possible, flushes entire guest-TLB.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatTlbShootdown,           "/HM/CPU%d/Flush/Shootdown/Page", "Inter-VCPU request to flush queued guest page.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatTlbShootdownFlush,      "/HM/CPU%d/Flush/Shootdown/TLB", "Inter-VCPU request to flush entire guest-TLB.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatTscParavirt,            "/HM/CPU%d/TSC/Paravirt", "Paravirtualized TSC in effect.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatTscOffset,              "/HM/CPU%d/TSC/Offset", "TSC offsetting is in effect.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatTscIntercept,           "/HM/CPU%d/TSC/Intercept", "Intercept TSC accesses.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatDRxArmed,               "/HM/CPU%d/Debug/Armed", "Loaded guest-debug state while loading guest-state.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatDRxContextSwitch,       "/HM/CPU%d/Debug/ContextSwitch", "Loaded guest-debug state on MOV DRx.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatDRxIoCheck,             "/HM/CPU%d/Debug/IOCheck", "Checking for I/O breakpoint.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatLoadMinimal,            "/HM/CPU%d/Load/Minimal", "VM-entry loading minimal guest-state.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatLoadFull,               "/HM/CPU%d/Load/Full", "VM-entry loading the full guest-state.");

        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckBadRmSelBase,   "/HM/CPU%d/VMXCheck/RMSelBase", "Could not use VMX due to unsuitable real-mode selector base.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckBadRmSelLimit,  "/HM/CPU%d/VMXCheck/RMSelLimit", "Could not use VMX due to unsuitable real-mode selector limit.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckRmOk,           "/HM/CPU%d/VMXCheck/VMX_RM", "VMX execution in real (V86) mode OK.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckBadSel,         "/HM/CPU%d/VMXCheck/Selector", "Could not use VMX due to unsuitable selector.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckBadRpl,         "/HM/CPU%d/VMXCheck/RPL", "Could not use VMX due to unsuitable RPL.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckBadLdt,         "/HM/CPU%d/VMXCheck/LDT", "Could not use VMX due to unsuitable LDT.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckBadTr,          "/HM/CPU%d/VMXCheck/TR", "Could not use VMX due to unsuitable TR.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatVmxCheckPmOk,           "/HM/CPU%d/VMXCheck/VMX_PM", "VMX execution in protected mode OK.");

#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
        HM_REG_COUNTER(&pVCpu->hm.s.StatFpu64SwitchBack,        "/HM/CPU%d/Switch64/Fpu", "Saving guest FPU/XMM state.");
        HM_REG_COUNTER(&pVCpu->hm.s.StatDebug64SwitchBack,      "/HM/CPU%d/Switch64/Debug", "Saving guest debug state.");
#endif

        for (unsigned j = 0; j < RT_ELEMENTS(pVCpu->hm.s.StatExitCRxWrite); j++)
        {
            rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitCRxWrite[j], STAMTYPE_COUNTER, STAMVISIBILITY_USED,
                                 STAMUNIT_OCCURENCES, "Profiling of CRx writes",
                                 "/HM/CPU%d/Exit/Instr/CR/Write/%x", i, j);
            AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitCRxRead[j], STAMTYPE_COUNTER, STAMVISIBILITY_USED,
                                 STAMUNIT_OCCURENCES, "Profiling of CRx reads",
                                 "/HM/CPU%d/Exit/Instr/CR/Read/%x", i, j);
            AssertRC(rc);
        }

#undef HM_REG_COUNTER

        pVCpu->hm.s.paStatExitReason = NULL;

        rc = MMHyperAlloc(pVM, MAX_EXITREASON_STAT * sizeof(*pVCpu->hm.s.paStatExitReason), 0 /* uAlignment */, MM_TAG_HM,
                          (void **)&pVCpu->hm.s.paStatExitReason);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            const char *const *papszDesc = ASMIsIntelCpu() || ASMIsViaCentaurCpu() ?
                                           &g_apszVTxExitReasons[0] : &g_apszAmdVExitReasons[0];
            for (int j = 0; j < MAX_EXITREASON_STAT; j++)
            {
                if (papszDesc[j])
                {
                    rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.paStatExitReason[j], STAMTYPE_COUNTER, STAMVISIBILITY_USED,
                                         STAMUNIT_OCCURENCES, papszDesc[j], "/HM/CPU%d/Exit/Reason/%02x", i, j);
                    AssertRC(rc);
                }
            }
            rc = STAMR3RegisterF(pVM, &pVCpu->hm.s.StatExitReasonNpf, STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,
                                 "Nested page fault", "/HM/CPU%d/Exit/Reason/#NPF", i);
            AssertRC(rc);
        }
        pVCpu->hm.s.paStatExitReasonR0 = MMHyperR3ToR0(pVM, pVCpu->hm.s.paStatExitReason);
# ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
        Assert(pVCpu->hm.s.paStatExitReasonR0 != NIL_RTR0PTR || !HMIsEnabled(pVM));
# else
        Assert(pVCpu->hm.s.paStatExitReasonR0 != NIL_RTR0PTR);
# endif

        rc = MMHyperAlloc(pVM, sizeof(STAMCOUNTER) * 256, 8, MM_TAG_HM, (void **)&pVCpu->hm.s.paStatInjectedIrqs);
        AssertRCReturn(rc, rc);
        pVCpu->hm.s.paStatInjectedIrqsR0 = MMHyperR3ToR0(pVM, pVCpu->hm.s.paStatInjectedIrqs);
# ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
        Assert(pVCpu->hm.s.paStatInjectedIrqsR0 != NIL_RTR0PTR || !HMIsEnabled(pVM));
# else
        Assert(pVCpu->hm.s.paStatInjectedIrqsR0 != NIL_RTR0PTR);
# endif
        for (unsigned j = 0; j < 255; j++)
        {
            STAMR3RegisterF(pVM, &pVCpu->hm.s.paStatInjectedIrqs[j], STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,
                            "Injected event.",
                            (j < 0x20) ? "/HM/CPU%d/EventInject/InjectTrap/%02X" : "/HM/CPU%d/EventInject/InjectIRQ/%02X", i, j);
        }

#endif /* VBOX_WITH_STATISTICS */
    }

#ifdef VBOX_WITH_CRASHDUMP_MAGIC
    /*
     * Magic marker for searching in crash dumps.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        PVMCSCACHE pCache = &pVCpu->hm.s.vmx.VMCSCache;
        strcpy((char *)pCache->aMagic, "VMCSCACHE Magic");
        pCache->uMagic = UINT64_C(0xDEADBEEFDEADBEEF);
    }
#endif

    return VINF_SUCCESS;
}


/**
 * Called when a init phase has completed.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   enmWhat             The phase that completed.
 */
VMMR3_INT_DECL(int) HMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
    switch (enmWhat)
    {
        case VMINITCOMPLETED_RING3:
            return hmR3InitCPU(pVM);
        case VMINITCOMPLETED_RING0:
            return hmR3InitFinalizeR0(pVM);
        default:
            return VINF_SUCCESS;
    }
}


/**
 * Turns off normal raw mode features.
 *
 * @param   pVM         The cross context VM structure.
 */
static void hmR3DisableRawMode(PVM pVM)
{
    /* Reinit the paging mode to force the new shadow mode. */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        PGMR3ChangeMode(pVM, pVCpu, PGMMODE_REAL);
    }
}


/**
 * Initialize VT-x or AMD-V.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
static int hmR3InitFinalizeR0(PVM pVM)
{
    int rc;

    if (!HMIsEnabled(pVM))
        return VINF_SUCCESS;

    /*
     * Hack to allow users to work around broken BIOSes that incorrectly set
     * EFER.SVME, which makes us believe somebody else is already using AMD-V.
     */
    if (    !pVM->hm.s.vmx.fSupported
        &&  !pVM->hm.s.svm.fSupported
        &&  pVM->hm.s.lLastError == VERR_SVM_IN_USE /* implies functional AMD-V */
        &&  RTEnvExist("VBOX_HWVIRTEX_IGNORE_SVM_IN_USE"))
    {
        LogRel(("HM: VBOX_HWVIRTEX_IGNORE_SVM_IN_USE active!\n"));
        pVM->hm.s.svm.fSupported        = true;
        pVM->hm.s.svm.fIgnoreInUseError = true;
        pVM->hm.s.lLastError = VINF_SUCCESS;
    }

    /*
     * Report ring-0 init errors.
     */
    if (   !pVM->hm.s.vmx.fSupported
        && !pVM->hm.s.svm.fSupported)
    {
        LogRel(("HM: Failed to initialize VT-x / AMD-V: %Rrc\n", pVM->hm.s.lLastError));
        LogRel(("HM: VMX MSR_IA32_FEATURE_CONTROL=%RX64\n", pVM->hm.s.vmx.Msrs.u64FeatureCtrl));
        switch (pVM->hm.s.lLastError)
        {
            case VERR_VMX_IN_VMX_ROOT_MODE:
                return VM_SET_ERROR(pVM, VERR_VMX_IN_VMX_ROOT_MODE, "VT-x is being used by another hypervisor");
            case VERR_VMX_NO_VMX:
                return VM_SET_ERROR(pVM, VERR_VMX_NO_VMX, "VT-x is not available");
            case VERR_VMX_MSR_VMX_DISABLED:
                return VM_SET_ERROR(pVM, VERR_VMX_MSR_VMX_DISABLED, "VT-x is disabled in the BIOS");
            case VERR_VMX_MSR_ALL_VMX_DISABLED:
                return VM_SET_ERROR(pVM, VERR_VMX_MSR_ALL_VMX_DISABLED, "VT-x is disabled in the BIOS for all CPU modes");
            case VERR_VMX_MSR_LOCKING_FAILED:
                return VM_SET_ERROR(pVM, VERR_VMX_MSR_LOCKING_FAILED, "Failed to lock VT-x features while trying to enable VT-x");
            case VERR_VMX_MSR_VMX_ENABLE_FAILED:
                return VM_SET_ERROR(pVM, VERR_VMX_MSR_VMX_ENABLE_FAILED, "Failed to enable VT-x features");
            case VERR_VMX_MSR_SMX_VMX_ENABLE_FAILED:
                return VM_SET_ERROR(pVM, VERR_VMX_MSR_SMX_VMX_ENABLE_FAILED, "Failed to enable VT-x features in SMX mode");

            case VERR_SVM_IN_USE:
                return VM_SET_ERROR(pVM, VERR_SVM_IN_USE, "AMD-V is being used by another hypervisor");
            case VERR_SVM_NO_SVM:
                return VM_SET_ERROR(pVM, VERR_SVM_NO_SVM, "AMD-V is not available");
            case VERR_SVM_DISABLED:
                return VM_SET_ERROR(pVM, VERR_SVM_DISABLED, "AMD-V is disabled in the BIOS");
        }
        return VMSetError(pVM, pVM->hm.s.lLastError, RT_SRC_POS, "HM ring-0 init failed: %Rrc", pVM->hm.s.lLastError);
    }

    /*
     * Enable VT-x or AMD-V on all host CPUs.
     */
    rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /*idCpu*/, VMMR0_DO_HM_ENABLE, 0, NULL);
    if (RT_FAILURE(rc))
    {
        LogRel(("HM: Failed to enable, error %Rrc\n", rc));
        HMR3CheckError(pVM, rc);
        return rc;
    }

    /*
     * No TPR patching is required when the IO-APIC is not enabled for this VM.
     * (Main should have taken care of this already)
     */
    if (!PDMHasIoApic(pVM))
    {
        Assert(!pVM->hm.s.fTprPatchingAllowed); /* paranoia */
        pVM->hm.s.fTprPatchingAllowed = false;
    }

    /*
     * Sync options.
     */
    /** @todo Move this out of of CPUMCTX and into some ring-0 only HM structure.
     *        That will require a little bit of work, of course. */
    for (VMCPUID iCpu = 0; iCpu < pVM->cCpus; iCpu++)
    {
        PVMCPU   pVCpu   = &pVM->aCpus[iCpu];
        PCPUMCTX pCpuCtx = CPUMQueryGuestCtxPtr(pVCpu);
        pCpuCtx->fWorldSwitcher &= ~(CPUMCTX_WSF_IBPB_EXIT | CPUMCTX_WSF_IBPB_ENTRY);
        if (pVM->cpum.ro.HostFeatures.fIbpb)
        {
            if (pVM->hm.s.fIbpbOnVmExit)
                pCpuCtx->fWorldSwitcher |= CPUMCTX_WSF_IBPB_EXIT;
            if (pVM->hm.s.fIbpbOnVmEntry)
                pCpuCtx->fWorldSwitcher |= CPUMCTX_WSF_IBPB_ENTRY;
        }
        if (iCpu == 0)
            LogRel(("HM: fWorldSwitcher=%#x (fIbpbOnVmExit=%d fIbpbOnVmEntry=%d)\n",
                    pCpuCtx->fWorldSwitcher, pVM->hm.s.fIbpbOnVmExit, pVM->hm.s.fIbpbOnVmEntry));
    }

    /*
     * Do the vendor specific initialization                                                                                                               .
     *                                                                                                                                                    .
     * Note! We disable release log buffering here since we're doing relatively                                                                           .
     *       lot of logging and doesn't want to hit the disk with each LogRel                                                                             .
     *       statement.
     */
    AssertLogRelReturn(!pVM->hm.s.fInitialized, VERR_HM_IPE_5);
    bool fOldBuffered = RTLogRelSetBuffering(true /*fBuffered*/);
    if (pVM->hm.s.vmx.fSupported)
        rc = hmR3InitFinalizeR0Intel(pVM);
    else
        rc = hmR3InitFinalizeR0Amd(pVM);
    LogRel(("HM: VT-x/AMD-V init method: %s\n", (pVM->hm.s.fGlobalInit) ? "GLOBAL" : "LOCAL"));
    RTLogRelSetBuffering(fOldBuffered);
    pVM->hm.s.fInitialized = true;

    return rc;
}


/**
 * @callback_method_impl{FNPDMVMMDEVHEAPNOTIFY}
 */
static DECLCALLBACK(void) hmR3VmmDevHeapNotify(PVM pVM, void *pvAllocation, RTGCPHYS GCPhysAllocation)
{
    NOREF(pVM);
    NOREF(pvAllocation);
    NOREF(GCPhysAllocation);
}


/**
 * Finish VT-x initialization (after ring-0 init).
 *
 * @returns VBox status code.
 * @param   pVM                The cross context VM structure.
 */
static int hmR3InitFinalizeR0Intel(PVM pVM)
{
    int rc;

    Log(("pVM->hm.s.vmx.fSupported = %d\n", pVM->hm.s.vmx.fSupported));
    AssertLogRelReturn(pVM->hm.s.vmx.Msrs.u64FeatureCtrl != 0, VERR_HM_IPE_4);

    uint64_t val;
    uint64_t zap;

    LogRel(("HM: Using VT-x implementation 2.0\n"));
    LogRel(("HM: Host CR4                        = %#RX64\n", pVM->hm.s.vmx.u64HostCr4));
    LogRel(("HM: Host EFER                       = %#RX64\n", pVM->hm.s.vmx.u64HostEfer));
    LogRel(("HM: MSR_IA32_SMM_MONITOR_CTL        = %#RX64\n", pVM->hm.s.vmx.u64HostSmmMonitorCtl));
    LogRel(("HM: MSR_IA32_FEATURE_CONTROL        = %#RX64\n", pVM->hm.s.vmx.Msrs.u64FeatureCtrl));
    if (!(pVM->hm.s.vmx.Msrs.u64FeatureCtrl & MSR_IA32_FEATURE_CONTROL_LOCK))
        LogRel(("HM:   IA32_FEATURE_CONTROL lock bit not set, possibly bad hardware!\n"));
    LogRel(("HM: MSR_IA32_VMX_BASIC_INFO         = %#RX64\n", pVM->hm.s.vmx.Msrs.u64BasicInfo));
    LogRel(("HM:   VMCS id                         = %#x\n",      MSR_IA32_VMX_BASIC_INFO_VMCS_ID(pVM->hm.s.vmx.Msrs.u64BasicInfo)));
    LogRel(("HM:   VMCS size                       = %u bytes\n", MSR_IA32_VMX_BASIC_INFO_VMCS_SIZE(pVM->hm.s.vmx.Msrs.u64BasicInfo)));
    LogRel(("HM:   VMCS physical address limit     = %s\n",       MSR_IA32_VMX_BASIC_INFO_VMCS_PHYS_WIDTH(pVM->hm.s.vmx.Msrs.u64BasicInfo) ? "< 4 GB" : "None"));
    LogRel(("HM:   VMCS memory type                = %#x\n",      MSR_IA32_VMX_BASIC_INFO_VMCS_MEM_TYPE(pVM->hm.s.vmx.Msrs.u64BasicInfo)));
    LogRel(("HM:   Dual-monitor treatment support  = %RTbool\n",  MSR_IA32_VMX_BASIC_INFO_VMCS_DUAL_MON(pVM->hm.s.vmx.Msrs.u64BasicInfo)));
    LogRel(("HM:   OUTS & INS instruction-info     = %RTbool\n",  MSR_IA32_VMX_BASIC_INFO_VMCS_INS_OUTS(pVM->hm.s.vmx.Msrs.u64BasicInfo)));
    LogRel(("HM:   Supports true capability MSRs   = %RTbool\n",  MSR_IA32_VMX_BASIC_INFO_TRUE_CONTROLS(pVM->hm.s.vmx.Msrs.u64BasicInfo)));
    LogRel(("HM: Max resume loops                = %u\n", pVM->hm.s.cMaxResumeLoops));

    LogRel(("HM: MSR_IA32_VMX_PINBASED_CTLS      = %#RX64\n", pVM->hm.s.vmx.Msrs.VmxPinCtls.u));
    val = pVM->hm.s.vmx.Msrs.VmxPinCtls.n.allowed1;
    zap = pVM->hm.s.vmx.Msrs.VmxPinCtls.n.disallowed0;
    HMVMX_REPORT_FEATURE(val, zap, "EXT_INT_EXIT",  VMX_VMCS_CTRL_PIN_EXEC_EXT_INT_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "NMI_EXIT",      VMX_VMCS_CTRL_PIN_EXEC_NMI_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "VIRTUAL_NMI",   VMX_VMCS_CTRL_PIN_EXEC_VIRTUAL_NMI);
    HMVMX_REPORT_FEATURE(val, zap, "PREEMPT_TIMER", VMX_VMCS_CTRL_PIN_EXEC_PREEMPT_TIMER);
    HMVMX_REPORT_FEATURE(val, zap, "POSTED_INTR",   VMX_VMCS_CTRL_PIN_EXEC_POSTED_INTR);

    LogRel(("HM: MSR_IA32_VMX_PROCBASED_CTLS     = %#RX64\n", pVM->hm.s.vmx.Msrs.VmxProcCtls.u));
    val = pVM->hm.s.vmx.Msrs.VmxProcCtls.n.allowed1;
    zap = pVM->hm.s.vmx.Msrs.VmxProcCtls.n.disallowed0;
    HMVMX_REPORT_FEATURE(val, zap, "INT_WINDOW_EXIT",         VMX_VMCS_CTRL_PROC_EXEC_INT_WINDOW_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "USE_TSC_OFFSETTING",      VMX_VMCS_CTRL_PROC_EXEC_USE_TSC_OFFSETTING);
    HMVMX_REPORT_FEATURE(val, zap, "HLT_EXIT",                VMX_VMCS_CTRL_PROC_EXEC_HLT_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "INVLPG_EXIT",             VMX_VMCS_CTRL_PROC_EXEC_INVLPG_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "MWAIT_EXIT",              VMX_VMCS_CTRL_PROC_EXEC_MWAIT_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "RDPMC_EXIT",              VMX_VMCS_CTRL_PROC_EXEC_RDPMC_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "RDTSC_EXIT",              VMX_VMCS_CTRL_PROC_EXEC_RDTSC_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "CR3_LOAD_EXIT",           VMX_VMCS_CTRL_PROC_EXEC_CR3_LOAD_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "CR3_STORE_EXIT",          VMX_VMCS_CTRL_PROC_EXEC_CR3_STORE_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "CR8_LOAD_EXIT",           VMX_VMCS_CTRL_PROC_EXEC_CR8_LOAD_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "CR8_STORE_EXIT",          VMX_VMCS_CTRL_PROC_EXEC_CR8_STORE_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "USE_TPR_SHADOW",          VMX_VMCS_CTRL_PROC_EXEC_USE_TPR_SHADOW);
    HMVMX_REPORT_FEATURE(val, zap, "NMI_WINDOW_EXIT",         VMX_VMCS_CTRL_PROC_EXEC_NMI_WINDOW_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "MOV_DR_EXIT",             VMX_VMCS_CTRL_PROC_EXEC_MOV_DR_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "UNCOND_IO_EXIT",          VMX_VMCS_CTRL_PROC_EXEC_UNCOND_IO_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "USE_IO_BITMAPS",          VMX_VMCS_CTRL_PROC_EXEC_USE_IO_BITMAPS);
    HMVMX_REPORT_FEATURE(val, zap, "MONITOR_TRAP_FLAG",       VMX_VMCS_CTRL_PROC_EXEC_MONITOR_TRAP_FLAG);
    HMVMX_REPORT_FEATURE(val, zap, "USE_MSR_BITMAPS",         VMX_VMCS_CTRL_PROC_EXEC_USE_MSR_BITMAPS);
    HMVMX_REPORT_FEATURE(val, zap, "MONITOR_EXIT",            VMX_VMCS_CTRL_PROC_EXEC_MONITOR_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "PAUSE_EXIT",              VMX_VMCS_CTRL_PROC_EXEC_PAUSE_EXIT);
    HMVMX_REPORT_FEATURE(val, zap, "USE_SECONDARY_EXEC_CTRL", VMX_VMCS_CTRL_PROC_EXEC_USE_SECONDARY_EXEC_CTRL);
    if (pVM->hm.s.vmx.Msrs.VmxProcCtls.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC_USE_SECONDARY_EXEC_CTRL)
    {
        LogRel(("HM: MSR_IA32_VMX_PROCBASED_CTLS2    = %#RX64\n", pVM->hm.s.vmx.Msrs.VmxProcCtls2.u));
        val = pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.allowed1;
        zap = pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.disallowed0;
        HMVMX_REPORT_FEATURE(val, zap, "VIRT_APIC",             VMX_VMCS_CTRL_PROC_EXEC2_VIRT_APIC);
        HMVMX_REPORT_FEATURE(val, zap, "EPT",                   VMX_VMCS_CTRL_PROC_EXEC2_EPT);
        HMVMX_REPORT_FEATURE(val, zap, "DESCRIPTOR_TABLE_EXIT", VMX_VMCS_CTRL_PROC_EXEC2_DESCRIPTOR_TABLE_EXIT);
        HMVMX_REPORT_FEATURE(val, zap, "RDTSCP",                VMX_VMCS_CTRL_PROC_EXEC2_RDTSCP);
        HMVMX_REPORT_FEATURE(val, zap, "VIRT_X2APIC",           VMX_VMCS_CTRL_PROC_EXEC2_VIRT_X2APIC);
        HMVMX_REPORT_FEATURE(val, zap, "VPID",                  VMX_VMCS_CTRL_PROC_EXEC2_VPID);
        HMVMX_REPORT_FEATURE(val, zap, "WBINVD_EXIT",           VMX_VMCS_CTRL_PROC_EXEC2_WBINVD_EXIT);
        HMVMX_REPORT_FEATURE(val, zap, "UNRESTRICTED_GUEST",    VMX_VMCS_CTRL_PROC_EXEC2_UNRESTRICTED_GUEST);
        HMVMX_REPORT_FEATURE(val, zap, "APIC_REG_VIRT",         VMX_VMCS_CTRL_PROC_EXEC2_APIC_REG_VIRT);
        HMVMX_REPORT_FEATURE(val, zap, "VIRT_INTR_DELIVERY",    VMX_VMCS_CTRL_PROC_EXEC2_VIRT_INTR_DELIVERY);
        HMVMX_REPORT_FEATURE(val, zap, "PAUSE_LOOP_EXIT",       VMX_VMCS_CTRL_PROC_EXEC2_PAUSE_LOOP_EXIT);
        HMVMX_REPORT_FEATURE(val, zap, "RDRAND_EXIT",           VMX_VMCS_CTRL_PROC_EXEC2_RDRAND_EXIT);
        HMVMX_REPORT_FEATURE(val, zap, "INVPCID",               VMX_VMCS_CTRL_PROC_EXEC2_INVPCID);
        HMVMX_REPORT_FEATURE(val, zap, "VMFUNC",                VMX_VMCS_CTRL_PROC_EXEC2_VMFUNC);
        HMVMX_REPORT_FEATURE(val, zap, "VMCS_SHADOWING",        VMX_VMCS_CTRL_PROC_EXEC2_VMCS_SHADOWING);
        HMVMX_REPORT_FEATURE(val, zap, "ENCLS_EXIT",            VMX_VMCS_CTRL_PROC_EXEC2_ENCLS_EXIT);
        HMVMX_REPORT_FEATURE(val, zap, "RDSEED_EXIT",           VMX_VMCS_CTRL_PROC_EXEC2_RDSEED_EXIT);
        HMVMX_REPORT_FEATURE(val, zap, "PML",                   VMX_VMCS_CTRL_PROC_EXEC2_PML);
        HMVMX_REPORT_FEATURE(val, zap, "EPT_VE",                VMX_VMCS_CTRL_PROC_EXEC2_EPT_VE);
        HMVMX_REPORT_FEATURE(val, zap, "CONCEAL_FROM_PT",       VMX_VMCS_CTRL_PROC_EXEC2_CONCEAL_FROM_PT);
        HMVMX_REPORT_FEATURE(val, zap, "XSAVES_XRSTORS",        VMX_VMCS_CTRL_PROC_EXEC2_XSAVES_XRSTORS);
        HMVMX_REPORT_FEATURE(val, zap, "TSC_SCALING",           VMX_VMCS_CTRL_PROC_EXEC2_TSC_SCALING);
    }

    LogRel(("HM: MSR_IA32_VMX_ENTRY_CTLS         = %#RX64\n", pVM->hm.s.vmx.Msrs.VmxEntry.u));
    val = pVM->hm.s.vmx.Msrs.VmxEntry.n.allowed1;
    zap = pVM->hm.s.vmx.Msrs.VmxEntry.n.disallowed0;
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_DEBUG",          VMX_VMCS_CTRL_ENTRY_LOAD_DEBUG);
    HMVMX_REPORT_FEATURE(val, zap, "IA32E_MODE_GUEST",    VMX_VMCS_CTRL_ENTRY_IA32E_MODE_GUEST);
    HMVMX_REPORT_FEATURE(val, zap, "ENTRY_SMM",           VMX_VMCS_CTRL_ENTRY_ENTRY_SMM);
    HMVMX_REPORT_FEATURE(val, zap, "DEACTIVATE_DUALMON",  VMX_VMCS_CTRL_ENTRY_DEACTIVATE_DUALMON);
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_GUEST_PERF_MSR", VMX_VMCS_CTRL_ENTRY_LOAD_GUEST_PERF_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_GUEST_PAT_MSR",  VMX_VMCS_CTRL_ENTRY_LOAD_GUEST_PAT_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_GUEST_EFER_MSR", VMX_VMCS_CTRL_ENTRY_LOAD_GUEST_EFER_MSR);

    LogRel(("HM: MSR_IA32_VMX_EXIT_CTLS          = %#RX64\n", pVM->hm.s.vmx.Msrs.VmxExit.u));
    val = pVM->hm.s.vmx.Msrs.VmxExit.n.allowed1;
    zap = pVM->hm.s.vmx.Msrs.VmxExit.n.disallowed0;
    HMVMX_REPORT_FEATURE(val, zap, "SAVE_DEBUG",             VMX_VMCS_CTRL_EXIT_SAVE_DEBUG);
    HMVMX_REPORT_FEATURE(val, zap, "HOST_ADDR_SPACE_SIZE",   VMX_VMCS_CTRL_EXIT_HOST_ADDR_SPACE_SIZE);
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_PERF_MSR",          VMX_VMCS_CTRL_EXIT_LOAD_PERF_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "ACK_EXT_INT",            VMX_VMCS_CTRL_EXIT_ACK_EXT_INT);
    HMVMX_REPORT_FEATURE(val, zap, "SAVE_GUEST_PAT_MSR",     VMX_VMCS_CTRL_EXIT_SAVE_GUEST_PAT_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_HOST_PAT_MSR",      VMX_VMCS_CTRL_EXIT_LOAD_HOST_PAT_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "SAVE_GUEST_EFER_MSR",    VMX_VMCS_CTRL_EXIT_SAVE_GUEST_EFER_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "LOAD_HOST_EFER_MSR",     VMX_VMCS_CTRL_EXIT_LOAD_HOST_EFER_MSR);
    HMVMX_REPORT_FEATURE(val, zap, "SAVE_VMX_PREEMPT_TIMER", VMX_VMCS_CTRL_EXIT_SAVE_VMX_PREEMPT_TIMER);

    if (pVM->hm.s.vmx.Msrs.u64EptVpidCaps)
    {
        val = pVM->hm.s.vmx.Msrs.u64EptVpidCaps;
        LogRel(("HM: MSR_IA32_VMX_EPT_VPID_CAP       = %#RX64\n", val));
        HMVMX_REPORT_MSR_CAPABILITY(val, "RWX_X_ONLY",                            MSR_IA32_VMX_EPT_VPID_CAP_RWX_X_ONLY);
        HMVMX_REPORT_MSR_CAPABILITY(val, "PAGE_WALK_LENGTH_4",                    MSR_IA32_VMX_EPT_VPID_CAP_PAGE_WALK_LENGTH_4);
        HMVMX_REPORT_MSR_CAPABILITY(val, "EMT_UC",                                MSR_IA32_VMX_EPT_VPID_CAP_EMT_UC);
        HMVMX_REPORT_MSR_CAPABILITY(val, "EMT_WB",                                MSR_IA32_VMX_EPT_VPID_CAP_EMT_WB);
        HMVMX_REPORT_MSR_CAPABILITY(val, "PDE_2M",                                MSR_IA32_VMX_EPT_VPID_CAP_PDE_2M);
        HMVMX_REPORT_MSR_CAPABILITY(val, "PDPTE_1G",                              MSR_IA32_VMX_EPT_VPID_CAP_PDPTE_1G);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVEPT",                                MSR_IA32_VMX_EPT_VPID_CAP_INVEPT);
        HMVMX_REPORT_MSR_CAPABILITY(val, "EPT_ACCESS_DIRTY",                      MSR_IA32_VMX_EPT_VPID_CAP_EPT_ACCESS_DIRTY);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVEPT_SINGLE_CONTEXT",                 MSR_IA32_VMX_EPT_VPID_CAP_INVEPT_SINGLE_CONTEXT);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVEPT_ALL_CONTEXTS",                   MSR_IA32_VMX_EPT_VPID_CAP_INVEPT_ALL_CONTEXTS);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVVPID",                               MSR_IA32_VMX_EPT_VPID_CAP_INVVPID);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVVPID_INDIV_ADDR",                    MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_INDIV_ADDR);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVVPID_SINGLE_CONTEXT",                MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVVPID_ALL_CONTEXTS",                  MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_ALL_CONTEXTS);
        HMVMX_REPORT_MSR_CAPABILITY(val, "INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS", MSR_IA32_VMX_EPT_VPID_CAP_INVVPID_SINGLE_CONTEXT_RETAIN_GLOBALS);
    }

    val = pVM->hm.s.vmx.Msrs.u64Misc;
    LogRel(("HM: MSR_IA32_VMX_MISC               = %#RX64\n", val));
    if (MSR_IA32_VMX_MISC_PREEMPT_TSC_BIT(val) == pVM->hm.s.vmx.cPreemptTimerShift)
        LogRel(("HM:   PREEMPT_TSC_BIT                 = %#x\n", MSR_IA32_VMX_MISC_PREEMPT_TSC_BIT(val)));
    else
    {
        LogRel(("HM:   PREEMPT_TSC_BIT                 = %#x - erratum detected, using %#x instead\n",
                MSR_IA32_VMX_MISC_PREEMPT_TSC_BIT(val), pVM->hm.s.vmx.cPreemptTimerShift));
    }

    LogRel(("HM:   STORE_EFERLMA_VMEXIT            = %RTbool\n", RT_BOOL(MSR_IA32_VMX_MISC_STORE_EFERLMA_VMEXIT(val))));
    LogRel(("HM:   ACTIVITY_STATES                 = %#x\n",     MSR_IA32_VMX_MISC_ACTIVITY_STATES(val)));
    LogRel(("HM:   CR3_TARGET                      = %#x\n",     MSR_IA32_VMX_MISC_CR3_TARGET(val)));
    LogRel(("HM:   MAX_MSR                         = %u\n",      MSR_IA32_VMX_MISC_MAX_MSR(val)));
    LogRel(("HM:   RDMSR_SMBASE_MSR_SMM            = %RTbool\n", RT_BOOL(MSR_IA32_VMX_MISC_RDMSR_SMBASE_MSR_SMM(val))));
    LogRel(("HM:   SMM_MONITOR_CTL_B2              = %RTbool\n", RT_BOOL(MSR_IA32_VMX_MISC_SMM_MONITOR_CTL_B2(val))));
    LogRel(("HM:   VMWRITE_VMEXIT_INFO             = %RTbool\n", RT_BOOL(MSR_IA32_VMX_MISC_VMWRITE_VMEXIT_INFO(val))));
    LogRel(("HM:   MSEG_ID                         = %#x\n",     MSR_IA32_VMX_MISC_MSEG_ID(val)));

    /* Paranoia */
    AssertRelease(MSR_IA32_VMX_MISC_MAX_MSR(pVM->hm.s.vmx.Msrs.u64Misc) >= 512);

    LogRel(("HM: MSR_IA32_VMX_CR0_FIXED0         = %#RX64\n", pVM->hm.s.vmx.Msrs.u64Cr0Fixed0));
    LogRel(("HM: MSR_IA32_VMX_CR0_FIXED1         = %#RX64\n", pVM->hm.s.vmx.Msrs.u64Cr0Fixed1));
    LogRel(("HM: MSR_IA32_VMX_CR4_FIXED0         = %#RX64\n", pVM->hm.s.vmx.Msrs.u64Cr4Fixed0));
    LogRel(("HM: MSR_IA32_VMX_CR4_FIXED1         = %#RX64\n", pVM->hm.s.vmx.Msrs.u64Cr4Fixed1));

    val = pVM->hm.s.vmx.Msrs.u64VmcsEnum;
    LogRel(("HM: MSR_IA32_VMX_VMCS_ENUM          = %#RX64\n", val));
    LogRel(("HM:   HIGHEST_INDEX                   = %#x\n", MSR_IA32_VMX_VMCS_ENUM_HIGHEST_INDEX(val)));

    val = pVM->hm.s.vmx.Msrs.u64Vmfunc;
    if (val)
    {
        LogRel(("HM: MSR_IA32_VMX_VMFUNC             = %#RX64\n", val));
        HMVMX_REPORT_ALLOWED_FEATURE(val, "EPTP_SWITCHING", VMX_VMCS_CTRL_VMFUNC_EPTP_SWITCHING);
    }

    LogRel(("HM: APIC-access page physaddr       = %#RHp\n", pVM->hm.s.vmx.HCPhysApicAccess));

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        LogRel(("HM: VCPU%3d: MSR bitmap physaddr    = %#RHp\n", i, pVM->aCpus[i].hm.s.vmx.HCPhysMsrBitmap));
        LogRel(("HM: VCPU%3d: VMCS physaddr          = %#RHp\n", i, pVM->aCpus[i].hm.s.vmx.HCPhysVmcs));
    }

    /*
     * EPT and unhampered guest execution are determined in HMR3Init, verify the sanity of that.
     */
    AssertLogRelReturn(   !pVM->hm.s.fNestedPaging
                       || (pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC2_EPT),
                       VERR_HM_IPE_1);
    AssertLogRelReturn(   !pVM->hm.s.vmx.fUnrestrictedGuest
                       || (   (pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC2_UNRESTRICTED_GUEST)
                           && pVM->hm.s.fNestedPaging),
                       VERR_HM_IPE_1);

    /*
     * Enable VPID if configured and supported.
     */
    if (pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC2_VPID)
        pVM->hm.s.vmx.fVpid = pVM->hm.s.vmx.fAllowVpid;

#if 0
    /*
     * Enable APIC register virtualization and virtual-interrupt delivery if supported.
     */
    if (   (pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC2_APIC_REG_VIRT)
        && (pVM->hm.s.vmx.Msrs.VmxProcCtls2.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC2_VIRT_INTR_DELIVERY))
        pVM->hm.s.fVirtApicRegs = true;

    /*
     * Enable posted-interrupt processing if supported.
     */
    /** @todo Add and query IPRT API for host OS support for posted-interrupt IPI
     *        here. */
    if (   (pVM->hm.s.vmx.Msrs.VmxPinCtls.n.allowed1 & VMX_VMCS_CTRL_PIN_EXEC_POSTED_INTR)
        && (pVM->hm.s.vmx.Msrs.VmxExit.n.allowed1 & VMX_VMCS_CTRL_EXIT_ACK_EXT_INT))
        pVM->hm.s.fPostedIntrs = true;
#endif

    /*
     * Disallow RDTSCP in the guest if there is no secondary process-based VM execution controls as otherwise
     * RDTSCP would cause a #UD. There might be no CPUs out there where this happens, as RDTSCP was introduced
     * in Nehalems and secondary VM exec. controls should be supported in all of them, but nonetheless it's Intel...
     */
    if (   !(pVM->hm.s.vmx.Msrs.VmxProcCtls.n.allowed1 & VMX_VMCS_CTRL_PROC_EXEC_USE_SECONDARY_EXEC_CTRL)
        && CPUMR3GetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_RDTSCP))
    {
        CPUMR3ClearGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_RDTSCP);
        LogRel(("HM: Disabled RDTSCP\n"));
    }

    if (!pVM->hm.s.vmx.fUnrestrictedGuest)
    {
        /* Allocate three pages for the TSS we need for real mode emulation. (2 pages for the IO bitmap) */
        rc = PDMR3VmmDevHeapAlloc(pVM, HM_VTX_TOTAL_DEVHEAP_MEM, hmR3VmmDevHeapNotify, (RTR3PTR *)&pVM->hm.s.vmx.pRealModeTSS);
        if (RT_SUCCESS(rc))
        {
            /* The IO bitmap starts right after the virtual interrupt redirection bitmap.
               Refer Intel spec. 20.3.3 "Software Interrupt Handling in Virtual-8086 mode"
               esp. Figure 20-5.*/
            ASMMemZero32(pVM->hm.s.vmx.pRealModeTSS, sizeof(*pVM->hm.s.vmx.pRealModeTSS));
            pVM->hm.s.vmx.pRealModeTSS->offIoBitmap = sizeof(*pVM->hm.s.vmx.pRealModeTSS);

            /* Bit set to 0 means software interrupts are redirected to the
               8086 program interrupt handler rather than switching to
               protected-mode handler. */
            memset(pVM->hm.s.vmx.pRealModeTSS->IntRedirBitmap, 0, sizeof(pVM->hm.s.vmx.pRealModeTSS->IntRedirBitmap));

            /* Allow all port IO, so that port IO instructions do not cause
               exceptions and would instead cause a VM-exit (based on VT-x's
               IO bitmap which we currently configure to always cause an exit). */
            memset(pVM->hm.s.vmx.pRealModeTSS + 1, 0, PAGE_SIZE * 2);
            *((unsigned char *)pVM->hm.s.vmx.pRealModeTSS + HM_VTX_TSS_SIZE - 2) = 0xff;

            /*
             * Construct a 1024 element page directory with 4 MB pages for
             * the identity mapped page table used in real and protected mode
             * without paging with EPT.
             */
            pVM->hm.s.vmx.pNonPagingModeEPTPageTable = (PX86PD)((char *)pVM->hm.s.vmx.pRealModeTSS + PAGE_SIZE * 3);
            for (uint32_t i = 0; i < X86_PG_ENTRIES; i++)
            {
                pVM->hm.s.vmx.pNonPagingModeEPTPageTable->a[i].u  = _4M * i;
                pVM->hm.s.vmx.pNonPagingModeEPTPageTable->a[i].u |= X86_PDE4M_P | X86_PDE4M_RW | X86_PDE4M_US
                                                                  | X86_PDE4M_A | X86_PDE4M_D | X86_PDE4M_PS
                                                                  | X86_PDE4M_G;
            }

            /* We convert it here every time as PCI regions could be reconfigured. */
            if (PDMVmmDevHeapIsEnabled(pVM))
            {
                RTGCPHYS GCPhys;
                rc = PDMVmmDevHeapR3ToGCPhys(pVM, pVM->hm.s.vmx.pRealModeTSS, &GCPhys);
                AssertRCReturn(rc, rc);
                LogRel(("HM: Real Mode TSS guest physaddr    = %#RGp\n", GCPhys));

                rc = PDMVmmDevHeapR3ToGCPhys(pVM, pVM->hm.s.vmx.pNonPagingModeEPTPageTable, &GCPhys);
                AssertRCReturn(rc, rc);
                LogRel(("HM: Non-Paging Mode EPT CR3         = %#RGp\n", GCPhys));
            }
        }
        else
        {
            LogRel(("HM: No real mode VT-x support (PDMR3VMMDevHeapAlloc returned %Rrc)\n", rc));
            pVM->hm.s.vmx.pRealModeTSS = NULL;
            pVM->hm.s.vmx.pNonPagingModeEPTPageTable = NULL;
            return VMSetError(pVM, rc, RT_SRC_POS,
                              "HM failure: No real mode VT-x support (PDMR3VMMDevHeapAlloc returned %Rrc)", rc);
        }
    }

    LogRel((pVM->hm.s.fAllow64BitGuests
            ? "HM: Guest support: 32-bit and 64-bit\n"
            : "HM: Guest support: 32-bit only\n"));

    /*
     * Call ring-0 to set up the VM.
     */
    rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /* idCpu */, VMMR0_DO_HM_SETUP_VM, 0 /* u64Arg */, NULL /* pReqHdr */);
    if (rc != VINF_SUCCESS)
    {
        AssertMsgFailed(("%Rrc\n", rc));
        LogRel(("HM: VMX setup failed with rc=%Rrc!\n", rc));
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PVMCPU pVCpu = &pVM->aCpus[i];
            LogRel(("HM: CPU[%u] Last instruction error  %#x\n", i, pVCpu->hm.s.vmx.LastError.u32InstrError));
            LogRel(("HM: CPU[%u] HM error                %#x (%u)\n", i, pVCpu->hm.s.u32HMError, pVCpu->hm.s.u32HMError));
        }
        HMR3CheckError(pVM, rc);
        return VMSetError(pVM, rc, RT_SRC_POS, "VT-x setup failed: %Rrc", rc);
    }

    LogRel(("HM: Supports VMCS EFER fields       = %RTbool\n", pVM->hm.s.vmx.fSupportsVmcsEfer));
    LogRel(("HM: Enabled VMX\n"));
    pVM->hm.s.vmx.fEnabled = true;

    hmR3DisableRawMode(pVM); /** @todo make this go away! */

    /*
     * Change the CPU features.
     */
    CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_SEP);
    if (pVM->hm.s.fAllow64BitGuests)
    {
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_PAE);
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_LONG_MODE);
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_SYSCALL);            /* 64 bits only on Intel CPUs */
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_LAHF);
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_NX);
    }
    /* Turn on NXE if PAE has been enabled *and* the host has turned on NXE
       (we reuse the host EFER in the switcher). */
    /** @todo this needs to be fixed properly!! */
    else if (CPUMR3GetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_PAE))
    {
        if (pVM->hm.s.vmx.u64HostEfer & MSR_K6_EFER_NXE)
            CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_NX);
        else
            LogRel(("HM: NX not enabled on the host, unavailable to PAE guest\n"));
    }

    /*
     * Log configuration details.
     */
    if (pVM->hm.s.fNestedPaging)
    {
        LogRel(("HM: Enabled nested paging\n"));
        if (pVM->hm.s.vmx.enmFlushEpt == VMXFLUSHEPT_SINGLE_CONTEXT)
            LogRel(("HM:   EPT flush type                = VMXFLUSHEPT_SINGLE_CONTEXT\n"));
        else if (pVM->hm.s.vmx.enmFlushEpt == VMXFLUSHEPT_ALL_CONTEXTS)
            LogRel(("HM:   EPT flush type                = VMXFLUSHEPT_ALL_CONTEXTS\n"));
        else if (pVM->hm.s.vmx.enmFlushEpt == VMXFLUSHEPT_NOT_SUPPORTED)
            LogRel(("HM:   EPT flush type                = VMXFLUSHEPT_NOT_SUPPORTED\n"));
        else
            LogRel(("HM:   EPT flush type                = %d\n", pVM->hm.s.vmx.enmFlushEpt));

        if (pVM->hm.s.vmx.fUnrestrictedGuest)
            LogRel(("HM: Enabled unrestricted guest execution\n"));

#if HC_ARCH_BITS == 64
        if (pVM->hm.s.fLargePages)
        {
            /* Use large (2 MB) pages for our EPT PDEs where possible. */
            PGMSetLargePageUsage(pVM, true);
            LogRel(("HM: Enabled large page support\n"));
        }
#endif
    }
    else
        Assert(!pVM->hm.s.vmx.fUnrestrictedGuest);

    if (pVM->hm.s.fVirtApicRegs)
        LogRel(("HM:   Enabled APIC-register virtualization support\n"));

    if (pVM->hm.s.fPostedIntrs)
        LogRel(("HM:   Enabled posted-interrupt processing support\n"));

    if (pVM->hm.s.vmx.fVpid)
    {
        LogRel(("HM: Enabled VPID\n"));
        if (pVM->hm.s.vmx.enmFlushVpid == VMXFLUSHVPID_INDIV_ADDR)
            LogRel(("HM:   VPID flush type               = VMXFLUSHVPID_INDIV_ADDR\n"));
        else if (pVM->hm.s.vmx.enmFlushVpid == VMXFLUSHVPID_SINGLE_CONTEXT)
            LogRel(("HM:   VPID flush type               = VMXFLUSHVPID_SINGLE_CONTEXT\n"));
        else if (pVM->hm.s.vmx.enmFlushVpid == VMXFLUSHVPID_ALL_CONTEXTS)
            LogRel(("HM:   VPID flush type               = VMXFLUSHVPID_ALL_CONTEXTS\n"));
        else if (pVM->hm.s.vmx.enmFlushVpid == VMXFLUSHVPID_SINGLE_CONTEXT_RETAIN_GLOBALS)
            LogRel(("HM:   VPID flush type               = VMXFLUSHVPID_SINGLE_CONTEXT_RETAIN_GLOBALS\n"));
        else
            LogRel(("HM:   VPID flush type               = %d\n", pVM->hm.s.vmx.enmFlushVpid));
    }
    else if (pVM->hm.s.vmx.enmFlushVpid == VMXFLUSHVPID_NOT_SUPPORTED)
        LogRel(("HM: Ignoring VPID capabilities of CPU\n"));

    if (pVM->hm.s.vmx.fUsePreemptTimer)
        LogRel(("HM: Enabled VMX-preemption timer (cPreemptTimerShift=%u)\n", pVM->hm.s.vmx.cPreemptTimerShift));
    else
        LogRel(("HM: Disabled VMX-preemption timer\n"));

    return VINF_SUCCESS;
}


/**
 * Finish AMD-V initialization (after ring-0 init).
 *
 * @returns VBox status code.
 * @param   pVM                The cross context VM structure.
 */
static int hmR3InitFinalizeR0Amd(PVM pVM)
{
    Log(("pVM->hm.s.svm.fSupported = %d\n", pVM->hm.s.svm.fSupported));

    LogRel(("HM: Using AMD-V implementation 2.0\n"));

    uint32_t u32Family;
    uint32_t u32Model;
    uint32_t u32Stepping;
    if (HMAmdIsSubjectToErratum170(&u32Family, &u32Model, &u32Stepping))
        LogRel(("HM: AMD Cpu with erratum 170 family %#x model %#x stepping %#x\n", u32Family, u32Model, u32Stepping));
    LogRel(("HM: Max resume loops                  = %u\n",     pVM->hm.s.cMaxResumeLoops));
    LogRel(("HM: CPUID 0x80000001.u32AMDFeatureECX = %#RX32\n", pVM->hm.s.cpuid.u32AMDFeatureECX));
    LogRel(("HM: CPUID 0x80000001.u32AMDFeatureEDX = %#RX32\n", pVM->hm.s.cpuid.u32AMDFeatureEDX));
    LogRel(("HM: AMD HWCR MSR                      = %#RX64\n", pVM->hm.s.svm.u64MsrHwcr));
    LogRel(("HM: AMD-V revision                    = %#x\n",    pVM->hm.s.svm.u32Rev));
    LogRel(("HM: AMD-V max ASID                    = %RU32\n",  pVM->hm.s.uMaxAsid));
    LogRel(("HM: AMD-V features                    = %#x\n",    pVM->hm.s.svm.u32Features));

    /*
     * Enumerate AMD-V features.
     */
    static const struct { uint32_t fFlag; const char *pszName; } s_aSvmFeatures[] =
    {
#define HMSVM_REPORT_FEATURE(a_StrDesc, a_Define) { a_Define, a_StrDesc }
        HMSVM_REPORT_FEATURE("NESTED_PAGING",          X86_CPUID_SVM_FEATURE_EDX_NESTED_PAGING),
        HMSVM_REPORT_FEATURE("LBR_VIRT",               X86_CPUID_SVM_FEATURE_EDX_LBR_VIRT),
        HMSVM_REPORT_FEATURE("SVM_LOCK",               X86_CPUID_SVM_FEATURE_EDX_SVM_LOCK),
        HMSVM_REPORT_FEATURE("NRIP_SAVE",              X86_CPUID_SVM_FEATURE_EDX_NRIP_SAVE),
        HMSVM_REPORT_FEATURE("TSC_RATE_MSR",           X86_CPUID_SVM_FEATURE_EDX_TSC_RATE_MSR),
        HMSVM_REPORT_FEATURE("VMCB_CLEAN",             X86_CPUID_SVM_FEATURE_EDX_VMCB_CLEAN),
        HMSVM_REPORT_FEATURE("FLUSH_BY_ASID",          X86_CPUID_SVM_FEATURE_EDX_FLUSH_BY_ASID),
        HMSVM_REPORT_FEATURE("DECODE_ASSIST",          X86_CPUID_SVM_FEATURE_EDX_DECODE_ASSIST),
        HMSVM_REPORT_FEATURE("PAUSE_FILTER",           X86_CPUID_SVM_FEATURE_EDX_PAUSE_FILTER),
        HMSVM_REPORT_FEATURE("PAUSE_FILTER_THRESHOLD", X86_CPUID_SVM_FEATURE_EDX_PAUSE_FILTER_THRESHOLD),
        HMSVM_REPORT_FEATURE("AVIC",                   X86_CPUID_SVM_FEATURE_EDX_AVIC),
        HMSVM_REPORT_FEATURE("VIRT_VMSAVE_VMLOAD",     X86_CPUID_SVM_FEATURE_EDX_VIRT_VMSAVE_VMLOAD),
        HMSVM_REPORT_FEATURE("VGIF",                   X86_CPUID_SVM_FEATURE_EDX_VGIF),
#undef HMSVM_REPORT_FEATURE
    };

    uint32_t fSvmFeatures = pVM->hm.s.svm.u32Features;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aSvmFeatures); i++)
        if (fSvmFeatures & s_aSvmFeatures[i].fFlag)
        {
            LogRel(("HM:   %s\n", s_aSvmFeatures[i].pszName));
            fSvmFeatures &= ~s_aSvmFeatures[i].fFlag;
        }
    if (fSvmFeatures)
        for (unsigned iBit = 0; iBit < 32; iBit++)
            if (RT_BIT_32(iBit) & fSvmFeatures)
                LogRel(("HM:   Reserved bit %u\n", iBit));

    /*
     * SVM R0 code assumes if the decode-assist feature exists, NRIP feature exists too.
     */
    AssertLogRelReturn(  !(pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_DECODE_ASSIST)
                       || (pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_NRIP_SAVE),
                       VERR_HM_UNSUPPORTED_CPU_FEATURE_COMBO);

    /*
     * Nested paging is determined in HMR3Init, verify the sanity of that.
     */
    AssertLogRelReturn(   !pVM->hm.s.fNestedPaging
                       || (pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_NESTED_PAGING),
                       VERR_HM_IPE_1);

#if 0
    /** @todo Add and query IPRT API for host OS support for posted-interrupt IPI
     *        here. */
    if (RTR0IsPostIpiSupport())
        pVM->hm.s.fPostedIntrs = true;
#endif

    /*
     * Call ring-0 to set up the VM.
     */
    int rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /*idCpu*/, VMMR0_DO_HM_SETUP_VM, 0, NULL);
    if (rc != VINF_SUCCESS)
    {
        AssertMsgFailed(("%Rrc\n", rc));
        LogRel(("HM: AMD-V setup failed with rc=%Rrc!\n", rc));
        return VMSetError(pVM, rc, RT_SRC_POS, "AMD-V setup failed: %Rrc", rc);
    }

    LogRel(("HM: Enabled SVM\n"));
    pVM->hm.s.svm.fEnabled = true;

    if (pVM->hm.s.fNestedPaging)
    {
        LogRel(("HM:   Enabled nested paging\n"));

        /*
         * Enable large pages (2 MB) if applicable.
         */
#if HC_ARCH_BITS == 64
        if (pVM->hm.s.fLargePages)
        {
            PGMSetLargePageUsage(pVM, true);
            LogRel(("HM:   Enabled large page support\n"));
        }
#endif
    }

    if (pVM->hm.s.fVirtApicRegs)
        LogRel(("HM:   Enabled APIC-register virtualization support\n"));

    if (pVM->hm.s.fPostedIntrs)
        LogRel(("HM:   Enabled posted-interrupt processing support\n"));

    hmR3DisableRawMode(pVM);

    /*
     * Change the CPU features.
     */
    CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_SEP);
    CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_SYSCALL);
    if (pVM->hm.s.fAllow64BitGuests)
    {
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_PAE);
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_LONG_MODE);
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_NX);
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_LAHF);
    }
    /* Turn on NXE if PAE has been enabled. */
    else if (CPUMR3GetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_PAE))
        CPUMR3SetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_NX);

    LogRel(("HM: %s TPR patching\n", (pVM->hm.s.fTprPatchingAllowed) ? "Enabled" : "Disabled"));

    LogRel((pVM->hm.s.fAllow64BitGuests
            ? "HM: Guest support: 32-bit and 64-bit\n"
            : "HM: Guest support: 32-bit only\n"));

    return VINF_SUCCESS;
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(void) HMR3Relocate(PVM pVM)
{
    Log(("HMR3Relocate to %RGv\n", MMHyperGetArea(pVM, 0)));

    /* Fetch the current paging mode during the relocate callback during state loading. */
    if (VMR3GetState(pVM) == VMSTATE_LOADING)
    {
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PVMCPU pVCpu = &pVM->aCpus[i];
            pVCpu->hm.s.enmShadowMode = PGMGetShadowMode(pVCpu);
        }
    }
#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
    if (HMIsEnabled(pVM))
    {
        switch (PGMGetHostMode(pVM))
        {
            case PGMMODE_32_BIT:
                pVM->hm.s.pfnHost32ToGuest64R0 = VMMR3GetHostToGuestSwitcher(pVM, VMMSWITCHER_32_TO_AMD64);
                break;

            case PGMMODE_PAE:
            case PGMMODE_PAE_NX:
                pVM->hm.s.pfnHost32ToGuest64R0 = VMMR3GetHostToGuestSwitcher(pVM, VMMSWITCHER_PAE_TO_AMD64);
                break;

            default:
                AssertFailed();
                break;
        }
    }
#endif
    return;
}


/**
 * Notification callback which is called whenever there is a chance that a CR3
 * value might have changed.
 *
 * This is called by PGM.
 *
 * @param   pVM            The cross context VM structure.
 * @param   pVCpu          The cross context virtual CPU structure.
 * @param   enmShadowMode  New shadow paging mode.
 * @param   enmGuestMode   New guest paging mode.
 */
VMMR3_INT_DECL(void) HMR3PagingModeChanged(PVM pVM, PVMCPU pVCpu, PGMMODE enmShadowMode, PGMMODE enmGuestMode)
{
    RT_NOREF_PV(pVM);

    /* Ignore page mode changes during state loading. */
    if (VMR3GetState(pVCpu->pVMR3) == VMSTATE_LOADING)
        return;

    pVCpu->hm.s.enmShadowMode = enmShadowMode;

    /*
     * If the guest left protected mode VMX execution, we'll have to be
     * extra careful if/when the guest switches back to protected mode.
     */
    if (enmGuestMode == PGMMODE_REAL)
        pVCpu->hm.s.vmx.fWasInRealMode = true;

    Log4(("HMR3PagingModeChanged: Guest paging mode '%s', shadow paging mode '%s'\n", PGMGetModeName(enmGuestMode),
          PGMGetModeName(enmShadowMode)));
}


/**
 * Terminates the HM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM itself is, at this point, powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) HMR3Term(PVM pVM)
{
    if (pVM->hm.s.vmx.pRealModeTSS)
    {
        PDMR3VmmDevHeapFree(pVM, pVM->hm.s.vmx.pRealModeTSS);
        pVM->hm.s.vmx.pRealModeTSS       = 0;
    }
    hmR3TermCPU(pVM);
    return 0;
}


/**
 * Terminates the per-VCPU HM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
static int hmR3TermCPU(PVM pVM)
{
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i]; NOREF(pVCpu);

#ifdef VBOX_WITH_STATISTICS
        if (pVCpu->hm.s.paStatExitReason)
        {
            MMHyperFree(pVM, pVCpu->hm.s.paStatExitReason);
            pVCpu->hm.s.paStatExitReason   = NULL;
            pVCpu->hm.s.paStatExitReasonR0 = NIL_RTR0PTR;
        }
        if (pVCpu->hm.s.paStatInjectedIrqs)
        {
            MMHyperFree(pVM, pVCpu->hm.s.paStatInjectedIrqs);
            pVCpu->hm.s.paStatInjectedIrqs   = NULL;
            pVCpu->hm.s.paStatInjectedIrqsR0 = NIL_RTR0PTR;
        }
#endif

#ifdef VBOX_WITH_CRASHDUMP_MAGIC
        memset(pVCpu->hm.s.vmx.VMCSCache.aMagic, 0, sizeof(pVCpu->hm.s.vmx.VMCSCache.aMagic));
        pVCpu->hm.s.vmx.VMCSCache.uMagic = 0;
        pVCpu->hm.s.vmx.VMCSCache.uPos = 0xffffffff;
#endif
    }
    return 0;
}


/**
 * Resets a virtual CPU.
 *
 * Used by HMR3Reset and CPU hot plugging.
 *
 * @param   pVCpu   The cross context virtual CPU structure to reset.
 */
VMMR3_INT_DECL(void) HMR3ResetCpu(PVMCPU pVCpu)
{
    /* Sync. entire state on VM reset R0-reentry. It's safe to reset
       the HM flags here, all other EMTs are in ring-3. See VMR3Reset(). */
    HMCPU_CF_RESET_TO(pVCpu, HM_CHANGED_HOST_CONTEXT | HM_CHANGED_ALL_GUEST);

    pVCpu->hm.s.vmx.u32CR0Mask        = 0;
    pVCpu->hm.s.vmx.u32CR4Mask        = 0;
    pVCpu->hm.s.fActive               = false;
    pVCpu->hm.s.Event.fPending        = false;
    pVCpu->hm.s.vmx.fWasInRealMode    = true;
    pVCpu->hm.s.vmx.u64MsrApicBase    = 0;
    pVCpu->hm.s.vmx.fSwitchedTo64on32 = false;

    /* Reset the contents of the read cache. */
    PVMCSCACHE pCache = &pVCpu->hm.s.vmx.VMCSCache;
    for (unsigned j = 0; j < pCache->Read.cValidEntries; j++)
        pCache->Read.aFieldVal[j] = 0;

#ifdef VBOX_WITH_CRASHDUMP_MAGIC
    /* Magic marker for searching in crash dumps. */
    strcpy((char *)pCache->aMagic, "VMCSCACHE Magic");
    pCache->uMagic = UINT64_C(0xDEADBEEFDEADBEEF);
#endif
}


/**
 * The VM is being reset.
 *
 * For the HM component this means that any GDT/LDT/TSS monitors
 * needs to be removed.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(void) HMR3Reset(PVM pVM)
{
    LogFlow(("HMR3Reset:\n"));

    if (HMIsEnabled(pVM))
        hmR3DisableRawMode(pVM);

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        HMR3ResetCpu(pVCpu);
    }

    /* Clear all patch information. */
    pVM->hm.s.pGuestPatchMem     = 0;
    pVM->hm.s.pFreeGuestPatchMem = 0;
    pVM->hm.s.cbGuestPatchMem    = 0;
    pVM->hm.s.cPatches           = 0;
    pVM->hm.s.PatchTree          = 0;
    pVM->hm.s.fTPRPatchingActive = false;
    ASMMemZero32(pVM->hm.s.aPatches, sizeof(pVM->hm.s.aPatches));
}


/**
 * Callback to patch a TPR instruction (vmmcall or mov cr8).
 *
 * @returns VBox strict status code.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 * @param   pvUser  Unused.
 */
static DECLCALLBACK(VBOXSTRICTRC) hmR3RemovePatches(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    VMCPUID idCpu = (VMCPUID)(uintptr_t)pvUser;

    /* Only execute the handler on the VCPU the original patch request was issued. */
    if (pVCpu->idCpu != idCpu)
        return VINF_SUCCESS;

    Log(("hmR3RemovePatches\n"));
    for (unsigned i = 0; i < pVM->hm.s.cPatches; i++)
    {
        uint8_t         abInstr[15];
        PHMTPRPATCH pPatch = &pVM->hm.s.aPatches[i];
        RTGCPTR         pInstrGC = (RTGCPTR)pPatch->Core.Key;
        int             rc;

#ifdef LOG_ENABLED
        char            szOutput[256];

        rc = DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, CPUMGetGuestCS(pVCpu), pInstrGC, DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                szOutput, sizeof(szOutput), NULL);
        if (RT_SUCCESS(rc))
            Log(("Patched instr: %s\n", szOutput));
#endif

        /* Check if the instruction is still the same. */
        rc = PGMPhysSimpleReadGCPtr(pVCpu, abInstr, pInstrGC, pPatch->cbNewOp);
        if (rc != VINF_SUCCESS)
        {
            Log(("Patched code removed? (rc=%Rrc0\n", rc));
            continue;   /* swapped out or otherwise removed; skip it. */
        }

        if (memcmp(abInstr, pPatch->aNewOpcode, pPatch->cbNewOp))
        {
            Log(("Patched instruction was changed! (rc=%Rrc0\n", rc));
            continue;   /* skip it. */
        }

        rc = PGMPhysSimpleWriteGCPtr(pVCpu, pInstrGC, pPatch->aOpcode, pPatch->cbOp);
        AssertRC(rc);

#ifdef LOG_ENABLED
        rc = DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, CPUMGetGuestCS(pVCpu), pInstrGC, DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                szOutput, sizeof(szOutput), NULL);
        if (RT_SUCCESS(rc))
            Log(("Original instr: %s\n", szOutput));
#endif
    }
    pVM->hm.s.cPatches           = 0;
    pVM->hm.s.PatchTree          = 0;
    pVM->hm.s.pFreeGuestPatchMem = pVM->hm.s.pGuestPatchMem;
    pVM->hm.s.fTPRPatchingActive = false;
    return VINF_SUCCESS;
}


/**
 * Worker for enabling patching in a VT-x/AMD-V guest.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       VCPU to execute hmR3RemovePatches on.
 * @param   pPatchMem   Patch memory range.
 * @param   cbPatchMem  Size of the memory range.
 */
static int hmR3EnablePatching(PVM pVM, VMCPUID idCpu, RTRCPTR pPatchMem, unsigned cbPatchMem)
{
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE, hmR3RemovePatches, (void *)(uintptr_t)idCpu);
    AssertRC(rc);

    pVM->hm.s.pGuestPatchMem      = pPatchMem;
    pVM->hm.s.pFreeGuestPatchMem  = pPatchMem;
    pVM->hm.s.cbGuestPatchMem     = cbPatchMem;
    return VINF_SUCCESS;
}


/**
 * Enable patching in a VT-x/AMD-V guest
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatchMem   Patch memory range.
 * @param   cbPatchMem  Size of the memory range.
 */
VMMR3_INT_DECL(int)  HMR3EnablePatching(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem)
{
    VM_ASSERT_EMT(pVM);
    Log(("HMR3EnablePatching %RGv size %x\n", pPatchMem, cbPatchMem));
    if (pVM->cCpus > 1)
    {
        /* We own the IOM lock here and could cause a deadlock by waiting for a VCPU that is blocking on the IOM lock. */
        int rc = VMR3ReqCallNoWait(pVM, VMCPUID_ANY_QUEUE,
                                   (PFNRT)hmR3EnablePatching, 4, pVM, VMMGetCpuId(pVM), (RTRCPTR)pPatchMem, cbPatchMem);
        AssertRC(rc);
        return rc;
    }
    return hmR3EnablePatching(pVM, VMMGetCpuId(pVM), (RTRCPTR)pPatchMem, cbPatchMem);
}


/**
 * Disable patching in a VT-x/AMD-V guest.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatchMem   Patch memory range.
 * @param   cbPatchMem  Size of the memory range.
 */
VMMR3_INT_DECL(int)  HMR3DisablePatching(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem)
{
    Log(("HMR3DisablePatching %RGv size %x\n", pPatchMem, cbPatchMem));
    RT_NOREF2(pPatchMem, cbPatchMem);

    Assert(pVM->hm.s.pGuestPatchMem == pPatchMem);
    Assert(pVM->hm.s.cbGuestPatchMem == cbPatchMem);

    /** @todo Potential deadlock when other VCPUs are waiting on the IOM lock (we own it)!! */
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE, hmR3RemovePatches,
                                (void *)(uintptr_t)VMMGetCpuId(pVM));
    AssertRC(rc);

    pVM->hm.s.pGuestPatchMem      = 0;
    pVM->hm.s.pFreeGuestPatchMem  = 0;
    pVM->hm.s.cbGuestPatchMem     = 0;
    pVM->hm.s.fTPRPatchingActive  = false;
    return VINF_SUCCESS;
}


/**
 * Callback to patch a TPR instruction (vmmcall or mov cr8).
 *
 * @returns VBox strict status code.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 * @param   pvUser  User specified CPU context.
 *
 */
static DECLCALLBACK(VBOXSTRICTRC) hmR3ReplaceTprInstr(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    /*
     * Only execute the handler on the VCPU the original patch request was
     * issued. (The other CPU(s) might not yet have switched to protected
     * mode, nor have the correct memory context.)
     */
    VMCPUID idCpu = (VMCPUID)(uintptr_t)pvUser;
    if (pVCpu->idCpu != idCpu)
        return VINF_SUCCESS;

    /*
     * We're racing other VCPUs here, so don't try patch the instruction twice
     * and make sure there is still room for our patch record.
     */
    PCPUMCTX    pCtx   = CPUMQueryGuestCtxPtr(pVCpu);
    PHMTPRPATCH pPatch = (PHMTPRPATCH)RTAvloU32Get(&pVM->hm.s.PatchTree, (AVLOU32KEY)pCtx->eip);
    if (pPatch)
    {
        Log(("hmR3ReplaceTprInstr: already patched %RGv\n", pCtx->rip));
        return VINF_SUCCESS;
    }
    uint32_t const idx = pVM->hm.s.cPatches;
    if (idx >= RT_ELEMENTS(pVM->hm.s.aPatches))
    {
        Log(("hmR3ReplaceTprInstr: no available patch slots (%RGv)\n", pCtx->rip));
        return VINF_SUCCESS;
    }
    pPatch = &pVM->hm.s.aPatches[idx];

    Log(("hmR3ReplaceTprInstr: rip=%RGv idxPatch=%u\n", pCtx->rip, idx));

    /*
     * Disassembler the instruction and get cracking.
     */
    DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "hmR3ReplaceTprInstr");
    PDISCPUSTATE    pDis = &pVCpu->hm.s.DisState;
    uint32_t        cbOp;
    int rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, &cbOp);
    AssertRC(rc);
    if (    rc == VINF_SUCCESS
        &&  pDis->pCurInstr->uOpcode == OP_MOV
        &&  cbOp >= 3)
    {
        static uint8_t const s_abVMMCall[3] = { 0x0f, 0x01, 0xd9 };

        rc = PGMPhysSimpleReadGCPtr(pVCpu, pPatch->aOpcode, pCtx->rip, cbOp);
        AssertRC(rc);

        pPatch->cbOp = cbOp;

        if (pDis->Param1.fUse == DISUSE_DISPLACEMENT32)
        {
            /* write. */
            if (pDis->Param2.fUse == DISUSE_REG_GEN32)
            {
                pPatch->enmType     = HMTPRINSTR_WRITE_REG;
                pPatch->uSrcOperand = pDis->Param2.Base.idxGenReg;
                Log(("hmR3ReplaceTprInstr: HMTPRINSTR_WRITE_REG %u\n", pDis->Param2.Base.idxGenReg));
            }
            else
            {
                Assert(pDis->Param2.fUse == DISUSE_IMMEDIATE32);
                pPatch->enmType     = HMTPRINSTR_WRITE_IMM;
                pPatch->uSrcOperand = pDis->Param2.uValue;
                Log(("hmR3ReplaceTprInstr: HMTPRINSTR_WRITE_IMM %#llx\n", pDis->Param2.uValue));
            }
            rc = PGMPhysSimpleWriteGCPtr(pVCpu, pCtx->rip, s_abVMMCall, sizeof(s_abVMMCall));
            AssertRC(rc);

            memcpy(pPatch->aNewOpcode, s_abVMMCall, sizeof(s_abVMMCall));
            pPatch->cbNewOp = sizeof(s_abVMMCall);
            STAM_COUNTER_INC(&pVM->hm.s.StatTprReplaceSuccessVmc);
        }
        else
        {
            /*
             * TPR Read.
             *
             * Found:
             *   mov eax, dword [fffe0080]        (5 bytes)
             * Check if next instruction is:
             *   shr eax, 4
             */
            Assert(pDis->Param1.fUse == DISUSE_REG_GEN32);

            uint8_t  const idxMmioReg = pDis->Param1.Base.idxGenReg;
            uint8_t  const cbOpMmio   = cbOp;
            uint64_t const uSavedRip  = pCtx->rip;

            pCtx->rip += cbOp;
            rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, &cbOp);
            DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Following read");
            pCtx->rip = uSavedRip;

            if (    rc == VINF_SUCCESS
                &&  pDis->pCurInstr->uOpcode == OP_SHR
                &&  pDis->Param1.fUse == DISUSE_REG_GEN32
                &&  pDis->Param1.Base.idxGenReg == idxMmioReg
                &&  pDis->Param2.fUse == DISUSE_IMMEDIATE8
                &&  pDis->Param2.uValue == 4
                &&  cbOpMmio + cbOp < sizeof(pVM->hm.s.aPatches[idx].aOpcode))
            {
                uint8_t abInstr[15];

                /* Replacing the two instructions above with an AMD-V specific lock-prefixed 32-bit MOV CR8 instruction so as to
                   access CR8 in 32-bit mode and not cause a #VMEXIT. */
                rc = PGMPhysSimpleReadGCPtr(pVCpu, &pPatch->aOpcode, pCtx->rip, cbOpMmio + cbOp);
                AssertRC(rc);

                pPatch->cbOp = cbOpMmio + cbOp;

                /* 0xF0, 0x0F, 0x20, 0xC0 = mov eax, cr8 */
                abInstr[0] = 0xF0;
                abInstr[1] = 0x0F;
                abInstr[2] = 0x20;
                abInstr[3] = 0xC0 | pDis->Param1.Base.idxGenReg;
                for (unsigned i = 4; i < pPatch->cbOp; i++)
                    abInstr[i] = 0x90;  /* nop */

                rc = PGMPhysSimpleWriteGCPtr(pVCpu, pCtx->rip, abInstr, pPatch->cbOp);
                AssertRC(rc);

                memcpy(pPatch->aNewOpcode, abInstr, pPatch->cbOp);
                pPatch->cbNewOp = pPatch->cbOp;
                STAM_COUNTER_INC(&pVM->hm.s.StatTprReplaceSuccessCr8);

                Log(("Acceptable read/shr candidate!\n"));
                pPatch->enmType = HMTPRINSTR_READ_SHR4;
            }
            else
            {
                pPatch->enmType     = HMTPRINSTR_READ;
                pPatch->uDstOperand = idxMmioReg;

                rc = PGMPhysSimpleWriteGCPtr(pVCpu, pCtx->rip, s_abVMMCall, sizeof(s_abVMMCall));
                AssertRC(rc);

                memcpy(pPatch->aNewOpcode, s_abVMMCall, sizeof(s_abVMMCall));
                pPatch->cbNewOp = sizeof(s_abVMMCall);
                STAM_COUNTER_INC(&pVM->hm.s.StatTprReplaceSuccessVmc);
                Log(("hmR3ReplaceTprInstr: HMTPRINSTR_READ %u\n", pPatch->uDstOperand));
            }
        }

        pPatch->Core.Key = pCtx->eip;
        rc = RTAvloU32Insert(&pVM->hm.s.PatchTree, &pPatch->Core);
        AssertRC(rc);

        pVM->hm.s.cPatches++;
        return VINF_SUCCESS;
    }

    /*
     * Save invalid patch, so we will not try again.
     */
    Log(("hmR3ReplaceTprInstr: Failed to patch instr!\n"));
    pPatch->Core.Key = pCtx->eip;
    pPatch->enmType  = HMTPRINSTR_INVALID;
    rc = RTAvloU32Insert(&pVM->hm.s.PatchTree, &pPatch->Core);
    AssertRC(rc);
    pVM->hm.s.cPatches++;
    STAM_COUNTER_INC(&pVM->hm.s.StatTprReplaceFailure);
    return VINF_SUCCESS;
}


/**
 * Callback to patch a TPR instruction (jump to generated code).
 *
 * @returns VBox strict status code.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 * @param   pvUser  User specified CPU context.
 *
 */
static DECLCALLBACK(VBOXSTRICTRC) hmR3PatchTprInstr(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    /*
     * Only execute the handler on the VCPU the original patch request was
     * issued. (The other CPU(s) might not yet have switched to protected
     * mode, nor have the correct memory context.)
     */
    VMCPUID         idCpu  = (VMCPUID)(uintptr_t)pvUser;
    if (pVCpu->idCpu != idCpu)
        return VINF_SUCCESS;

    /*
     * We're racing other VCPUs here, so don't try patch the instruction twice
     * and make sure there is still room for our patch record.
     */
    PCPUMCTX    pCtx   = CPUMQueryGuestCtxPtr(pVCpu);
    PHMTPRPATCH pPatch = (PHMTPRPATCH)RTAvloU32Get(&pVM->hm.s.PatchTree, (AVLOU32KEY)pCtx->eip);
    if (pPatch)
    {
        Log(("hmR3PatchTprInstr: already patched %RGv\n", pCtx->rip));
        return VINF_SUCCESS;
    }
    uint32_t const  idx = pVM->hm.s.cPatches;
    if (idx >= RT_ELEMENTS(pVM->hm.s.aPatches))
    {
        Log(("hmR3PatchTprInstr: no available patch slots (%RGv)\n", pCtx->rip));
        return VINF_SUCCESS;
    }
    pPatch = &pVM->hm.s.aPatches[idx];

    Log(("hmR3PatchTprInstr: rip=%RGv idxPatch=%u\n", pCtx->rip, idx));
    DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "hmR3PatchTprInstr");

    /*
     * Disassemble the instruction and get cracking.
     */
    PDISCPUSTATE    pDis   = &pVCpu->hm.s.DisState;
    uint32_t        cbOp;
    int rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, &cbOp);
    AssertRC(rc);
    if (    rc == VINF_SUCCESS
        &&  pDis->pCurInstr->uOpcode == OP_MOV
        &&  cbOp >= 5)
    {
        uint8_t         aPatch[64];
        uint32_t        off = 0;

        rc = PGMPhysSimpleReadGCPtr(pVCpu, pPatch->aOpcode, pCtx->rip, cbOp);
        AssertRC(rc);

        pPatch->cbOp    = cbOp;
        pPatch->enmType = HMTPRINSTR_JUMP_REPLACEMENT;

        if (pDis->Param1.fUse == DISUSE_DISPLACEMENT32)
        {
            /*
             * TPR write:
             *
             * push ECX                      [51]
             * push EDX                      [52]
             * push EAX                      [50]
             * xor EDX,EDX                   [31 D2]
             * mov EAX,EAX                   [89 C0]
             *  or
             * mov EAX,0000000CCh            [B8 CC 00 00 00]
             * mov ECX,0C0000082h            [B9 82 00 00 C0]
             * wrmsr                         [0F 30]
             * pop EAX                       [58]
             * pop EDX                       [5A]
             * pop ECX                       [59]
             * jmp return_address            [E9 return_address]
             *
             */
            bool fUsesEax = (pDis->Param2.fUse == DISUSE_REG_GEN32 && pDis->Param2.Base.idxGenReg == DISGREG_EAX);

            aPatch[off++] = 0x51;    /* push ecx */
            aPatch[off++] = 0x52;    /* push edx */
            if (!fUsesEax)
                aPatch[off++] = 0x50;    /* push eax */
            aPatch[off++] = 0x31;    /* xor edx, edx */
            aPatch[off++] = 0xD2;
            if (pDis->Param2.fUse == DISUSE_REG_GEN32)
            {
                if (!fUsesEax)
                {
                    aPatch[off++] = 0x89;    /* mov eax, src_reg */
                    aPatch[off++] = MAKE_MODRM(3, pDis->Param2.Base.idxGenReg, DISGREG_EAX);
                }
            }
            else
            {
                Assert(pDis->Param2.fUse == DISUSE_IMMEDIATE32);
                aPatch[off++] = 0xB8;    /* mov eax, immediate */
                *(uint32_t *)&aPatch[off] = pDis->Param2.uValue;
                off += sizeof(uint32_t);
            }
            aPatch[off++] = 0xB9;    /* mov ecx, 0xc0000082 */
            *(uint32_t *)&aPatch[off] = MSR_K8_LSTAR;
            off += sizeof(uint32_t);

            aPatch[off++] = 0x0F;    /* wrmsr */
            aPatch[off++] = 0x30;
            if (!fUsesEax)
                aPatch[off++] = 0x58;    /* pop eax */
            aPatch[off++] = 0x5A;    /* pop edx */
            aPatch[off++] = 0x59;    /* pop ecx */
        }
        else
        {
            /*
             * TPR read:
             *
             * push ECX                      [51]
             * push EDX                      [52]
             * push EAX                      [50]
             * mov ECX,0C0000082h            [B9 82 00 00 C0]
             * rdmsr                         [0F 32]
             * mov EAX,EAX                   [89 C0]
             * pop EAX                       [58]
             * pop EDX                       [5A]
             * pop ECX                       [59]
             * jmp return_address            [E9 return_address]
             *
             */
            Assert(pDis->Param1.fUse == DISUSE_REG_GEN32);

            if (pDis->Param1.Base.idxGenReg != DISGREG_ECX)
                aPatch[off++] = 0x51;    /* push ecx */
            if (pDis->Param1.Base.idxGenReg != DISGREG_EDX )
                aPatch[off++] = 0x52;    /* push edx */
            if (pDis->Param1.Base.idxGenReg != DISGREG_EAX)
                aPatch[off++] = 0x50;    /* push eax */

            aPatch[off++] = 0x31;    /* xor edx, edx */
            aPatch[off++] = 0xD2;

            aPatch[off++] = 0xB9;    /* mov ecx, 0xc0000082 */
            *(uint32_t *)&aPatch[off] = MSR_K8_LSTAR;
            off += sizeof(uint32_t);

            aPatch[off++] = 0x0F;    /* rdmsr */
            aPatch[off++] = 0x32;

            if (pDis->Param1.Base.idxGenReg != DISGREG_EAX)
            {
                aPatch[off++] = 0x89;    /* mov dst_reg, eax */
                aPatch[off++] = MAKE_MODRM(3, DISGREG_EAX, pDis->Param1.Base.idxGenReg);
            }

            if (pDis->Param1.Base.idxGenReg != DISGREG_EAX)
                aPatch[off++] = 0x58;    /* pop eax */
            if (pDis->Param1.Base.idxGenReg != DISGREG_EDX )
                aPatch[off++] = 0x5A;    /* pop edx */
            if (pDis->Param1.Base.idxGenReg != DISGREG_ECX)
                aPatch[off++] = 0x59;    /* pop ecx */
        }
        aPatch[off++] = 0xE9;    /* jmp return_address */
        *(RTRCUINTPTR *)&aPatch[off] = ((RTRCUINTPTR)pCtx->eip + cbOp) - ((RTRCUINTPTR)pVM->hm.s.pFreeGuestPatchMem + off + 4);
        off += sizeof(RTRCUINTPTR);

        if (pVM->hm.s.pFreeGuestPatchMem + off <= pVM->hm.s.pGuestPatchMem + pVM->hm.s.cbGuestPatchMem)
        {
            /* Write new code to the patch buffer. */
            rc = PGMPhysSimpleWriteGCPtr(pVCpu, pVM->hm.s.pFreeGuestPatchMem, aPatch, off);
            AssertRC(rc);

#ifdef LOG_ENABLED
            uint32_t cbCurInstr;
            for (RTGCPTR GCPtrInstr = pVM->hm.s.pFreeGuestPatchMem;
                 GCPtrInstr < pVM->hm.s.pFreeGuestPatchMem + off;
                 GCPtrInstr += RT_MAX(cbCurInstr, 1))
            {
                char     szOutput[256];
                rc = DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, pCtx->cs.Sel, GCPtrInstr, DBGF_DISAS_FLAGS_DEFAULT_MODE,
                                        szOutput, sizeof(szOutput), &cbCurInstr);
                if (RT_SUCCESS(rc))
                    Log(("Patch instr %s\n", szOutput));
                else
                    Log(("%RGv: rc=%Rrc\n", GCPtrInstr, rc));
            }
#endif

            pPatch->aNewOpcode[0] = 0xE9;
            *(RTRCUINTPTR *)&pPatch->aNewOpcode[1] = ((RTRCUINTPTR)pVM->hm.s.pFreeGuestPatchMem) - ((RTRCUINTPTR)pCtx->eip + 5);

            /* Overwrite the TPR instruction with a jump. */
            rc = PGMPhysSimpleWriteGCPtr(pVCpu, pCtx->eip, pPatch->aNewOpcode, 5);
            AssertRC(rc);

            DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "Jump");

            pVM->hm.s.pFreeGuestPatchMem += off;
            pPatch->cbNewOp = 5;

            pPatch->Core.Key = pCtx->eip;
            rc = RTAvloU32Insert(&pVM->hm.s.PatchTree, &pPatch->Core);
            AssertRC(rc);

            pVM->hm.s.cPatches++;
            pVM->hm.s.fTPRPatchingActive = true;
            STAM_COUNTER_INC(&pVM->hm.s.StatTprPatchSuccess);
            return VINF_SUCCESS;
        }

        Log(("Ran out of space in our patch buffer!\n"));
    }
    else
        Log(("hmR3PatchTprInstr: Failed to patch instr!\n"));


    /*
     * Save invalid patch, so we will not try again.
     */
    pPatch = &pVM->hm.s.aPatches[idx];
    pPatch->Core.Key = pCtx->eip;
    pPatch->enmType  = HMTPRINSTR_INVALID;
    rc = RTAvloU32Insert(&pVM->hm.s.PatchTree, &pPatch->Core);
    AssertRC(rc);
    pVM->hm.s.cPatches++;
    STAM_COUNTER_INC(&pVM->hm.s.StatTprPatchFailure);
    return VINF_SUCCESS;
}


/**
 * Attempt to patch TPR mmio instructions.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest CPU context.
 */
VMMR3_INT_DECL(int) HMR3PatchTprInstr(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    NOREF(pCtx);
    int rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE,
                                pVM->hm.s.pGuestPatchMem ? hmR3PatchTprInstr : hmR3ReplaceTprInstr,
                                (void *)(uintptr_t)pVCpu->idCpu);
    AssertRC(rc);
    return rc;
}


/**
 * Checks if a code selector (CS) is suitable for execution
 * within VMX when unrestricted execution isn't available.
 *
 * @returns true if selector is suitable for VMX, otherwise
 *        false.
 * @param   pSel        Pointer to the selector to check (CS).
 * @param   uStackDpl   The CPL, aka the DPL of the stack segment.
 */
static bool hmR3IsCodeSelectorOkForVmx(PCPUMSELREG pSel, unsigned uStackDpl)
{
    /*
     * Segment must be an accessed code segment, it must be present and it must
     * be usable.
     * Note! These are all standard requirements and if CS holds anything else
     *       we've got buggy code somewhere!
     */
    AssertCompile(X86DESCATTR_TYPE == 0xf);
    AssertMsgReturn(   (pSel->Attr.u & (X86_SEL_TYPE_ACCESSED | X86_SEL_TYPE_CODE | X86DESCATTR_DT | X86DESCATTR_P | X86DESCATTR_UNUSABLE))
                    ==                 (X86_SEL_TYPE_ACCESSED | X86_SEL_TYPE_CODE | X86DESCATTR_DT | X86DESCATTR_P),
                    ("%#x\n", pSel->Attr.u),
                    false);

    /* For conforming segments, CS.DPL must be <= SS.DPL, while CS.DPL
       must equal SS.DPL for non-confroming segments.
       Note! This is also a hard requirement like above. */
    AssertMsgReturn(  pSel->Attr.n.u4Type & X86_SEL_TYPE_CONF
                    ? pSel->Attr.n.u2Dpl <= uStackDpl
                    : pSel->Attr.n.u2Dpl == uStackDpl,
                    ("u4Type=%#x u2Dpl=%u uStackDpl=%u\n", pSel->Attr.n.u4Type, pSel->Attr.n.u2Dpl, uStackDpl),
                    false);

    /*
     * The following two requirements are VT-x specific:
     *  - G bit must be set if any high limit bits are set.
     *  - G bit must be clear if any low limit bits are clear.
     */
    if (   ((pSel->u32Limit & 0xfff00000) == 0x00000000 ||  pSel->Attr.n.u1Granularity)
        && ((pSel->u32Limit & 0x00000fff) == 0x00000fff || !pSel->Attr.n.u1Granularity) )
        return true;
    return false;
}


/**
 * Checks if a data selector (DS/ES/FS/GS) is suitable for
 * execution within VMX when unrestricted execution isn't
 * available.
 *
 * @returns true if selector is suitable for VMX, otherwise
 *        false.
 * @param   pSel        Pointer to the selector to check
 *                      (DS/ES/FS/GS).
 */
static bool hmR3IsDataSelectorOkForVmx(PCPUMSELREG pSel)
{
    /*
     * Unusable segments are OK.  These days they should be marked as such, as
     * but as an alternative we for old saved states and AMD<->VT-x migration
     * we also treat segments with all the attributes cleared as unusable.
     */
    if (pSel->Attr.n.u1Unusable || !pSel->Attr.u)
        return true;

    /** @todo tighten these checks. Will require CPUM load adjusting. */

    /* Segment must be accessed. */
    if (pSel->Attr.u & X86_SEL_TYPE_ACCESSED)
    {
        /* Code segments must also be readable. */
        if (  !(pSel->Attr.u & X86_SEL_TYPE_CODE)
            || (pSel->Attr.u & X86_SEL_TYPE_READ))
        {
            /* The S bit must be set. */
            if (pSel->Attr.n.u1DescType)
            {
                /* Except for conforming segments, DPL >= RPL. */
                if (   pSel->Attr.n.u2Dpl  >= (pSel->Sel & X86_SEL_RPL)
                    || pSel->Attr.n.u4Type >= X86_SEL_TYPE_ER_ACC)
                {
                    /* Segment must be present. */
                    if (pSel->Attr.n.u1Present)
                    {
                        /*
                         * The following two requirements are VT-x specific:
                         *  - G bit must be set if any high limit bits are set.
                         *  - G bit must be clear if any low limit bits are clear.
                         */
                        if (   ((pSel->u32Limit & 0xfff00000) == 0x00000000 ||  pSel->Attr.n.u1Granularity)
                            && ((pSel->u32Limit & 0x00000fff) == 0x00000fff || !pSel->Attr.n.u1Granularity) )
                            return true;
                    }
                }
            }
        }
    }

    return false;
}


/**
 * Checks if the stack selector (SS) is suitable for execution
 * within VMX when unrestricted execution isn't available.
 *
 * @returns true if selector is suitable for VMX, otherwise
 *        false.
 * @param   pSel        Pointer to the selector to check (SS).
 */
static bool hmR3IsStackSelectorOkForVmx(PCPUMSELREG pSel)
{
    /*
     * Unusable segments are OK.  These days they should be marked as such, as
     * but as an alternative we for old saved states and AMD<->VT-x migration
     * we also treat segments with all the attributes cleared as unusable.
     */
    /** @todo r=bird: actually all zeroes isn't gonna cut it... SS.DPL == CPL. */
    if (pSel->Attr.n.u1Unusable || !pSel->Attr.u)
        return true;

    /*
     * Segment must be an accessed writable segment, it must be present.
     * Note! These are all standard requirements and if SS holds anything else
     *       we've got buggy code somewhere!
     */
    AssertCompile(X86DESCATTR_TYPE == 0xf);
    AssertMsgReturn(   (pSel->Attr.u & (X86_SEL_TYPE_ACCESSED | X86_SEL_TYPE_WRITE | X86DESCATTR_DT | X86DESCATTR_P | X86_SEL_TYPE_CODE))
                    ==                 (X86_SEL_TYPE_ACCESSED | X86_SEL_TYPE_WRITE | X86DESCATTR_DT | X86DESCATTR_P),
                    ("%#x\n", pSel->Attr.u),
                    false);

    /* DPL must equal RPL.
       Note! This is also a hard requirement like above. */
    AssertMsgReturn(pSel->Attr.n.u2Dpl == (pSel->Sel & X86_SEL_RPL),
                    ("u2Dpl=%u Sel=%#x\n", pSel->Attr.n.u2Dpl, pSel->Sel),
                    false);

    /*
     * The following two requirements are VT-x specific:
     *  - G bit must be set if any high limit bits are set.
     *  - G bit must be clear if any low limit bits are clear.
     */
    if (   ((pSel->u32Limit & 0xfff00000) == 0x00000000 ||  pSel->Attr.n.u1Granularity)
        && ((pSel->u32Limit & 0x00000fff) == 0x00000fff || !pSel->Attr.n.u1Granularity) )
        return true;
    return false;
}


/**
 * Force execution of the current IO code in the recompiler.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        Partial VM execution context.
 */
VMMR3_INT_DECL(int) HMR3EmulateIoBlock(PVM pVM, PCPUMCTX pCtx)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);

    Assert(HMIsEnabled(pVM));
    Log(("HMR3EmulateIoBlock\n"));

    /* This is primarily intended to speed up Grub, so we don't care about paged protected mode. */
    if (HMCanEmulateIoBlockEx(pCtx))
    {
        Log(("HMR3EmulateIoBlock -> enabled\n"));
        pVCpu->hm.s.EmulateIoBlock.fEnabled         = true;
        pVCpu->hm.s.EmulateIoBlock.GCPtrFunctionEip = pCtx->rip;
        pVCpu->hm.s.EmulateIoBlock.cr0              = pCtx->cr0;
        return VINF_EM_RESCHEDULE_REM;
    }
    return VINF_SUCCESS;
}


/**
 * Checks if we can currently use hardware accelerated raw mode.
 *
 * @returns true if we can currently use hardware acceleration, otherwise false.
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        Partial VM execution context.
 */
VMMR3DECL(bool) HMR3CanExecuteGuest(PVM pVM, PCPUMCTX pCtx)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);

    Assert(HMIsEnabled(pVM));

#ifdef VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM
    if (CPUMIsGuestInNestedHwVirtMode(pCtx))
    {
        Log(("HMR3CanExecuteGuest: In nested-guest mode - returning false"));
        return false;
    }
#endif

    /* If we're still executing the IO code, then return false. */
    if (    RT_UNLIKELY(pVCpu->hm.s.EmulateIoBlock.fEnabled)
        &&  pCtx->rip <  pVCpu->hm.s.EmulateIoBlock.GCPtrFunctionEip + 0x200
        &&  pCtx->rip >  pVCpu->hm.s.EmulateIoBlock.GCPtrFunctionEip - 0x200
        &&  pCtx->cr0 == pVCpu->hm.s.EmulateIoBlock.cr0)
        return false;

    pVCpu->hm.s.EmulateIoBlock.fEnabled = false;

    /* AMD-V supports real & protected mode with or without paging. */
    if (pVM->hm.s.svm.fEnabled)
    {
        pVCpu->hm.s.fActive = true;
        return true;
    }

    pVCpu->hm.s.fActive = false;

    /* Note! The context supplied by REM is partial. If we add more checks here, be sure to verify that REM provides this info! */
    Assert(   (pVM->hm.s.vmx.fUnrestrictedGuest && !pVM->hm.s.vmx.pRealModeTSS)
           || (!pVM->hm.s.vmx.fUnrestrictedGuest && pVM->hm.s.vmx.pRealModeTSS));

    bool fSupportsRealMode = pVM->hm.s.vmx.fUnrestrictedGuest || PDMVmmDevHeapIsEnabled(pVM);
    if (!pVM->hm.s.vmx.fUnrestrictedGuest)
    {
        /*
         * The VMM device heap is a requirement for emulating real mode or protected mode without paging with the unrestricted
         * guest execution feature is missing (VT-x only).
         */
        if (fSupportsRealMode)
        {
            if (CPUMIsGuestInRealModeEx(pCtx))
            {
                /* In V86 mode (VT-x or not), the CPU enforces real-mode compatible selector
                 * bases and limits, i.e. limit must be 64K and base must be selector * 16.
                 * If this is not true, we cannot execute real mode as V86 and have to fall
                 * back to emulation.
                 */
                if (   pCtx->cs.Sel != (pCtx->cs.u64Base >> 4)
                    || pCtx->ds.Sel != (pCtx->ds.u64Base >> 4)
                    || pCtx->es.Sel != (pCtx->es.u64Base >> 4)
                    || pCtx->ss.Sel != (pCtx->ss.u64Base >> 4)
                    || pCtx->fs.Sel != (pCtx->fs.u64Base >> 4)
                    || pCtx->gs.Sel != (pCtx->gs.u64Base >> 4))
                {
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckBadRmSelBase);
                    return false;
                }
                if (   (pCtx->cs.u32Limit != 0xffff)
                    || (pCtx->ds.u32Limit != 0xffff)
                    || (pCtx->es.u32Limit != 0xffff)
                    || (pCtx->ss.u32Limit != 0xffff)
                    || (pCtx->fs.u32Limit != 0xffff)
                    || (pCtx->gs.u32Limit != 0xffff))
                {
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckBadRmSelLimit);
                    return false;
                }
                STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckRmOk);
            }
            else
            {
                /* Verify the requirements for executing code in protected
                   mode. VT-x can't handle the CPU state right after a switch
                   from real to protected mode. (all sorts of RPL & DPL assumptions). */
                if (pVCpu->hm.s.vmx.fWasInRealMode)
                {
                    /** @todo If guest is in V86 mode, these checks should be different! */
                    if ((pCtx->cs.Sel & X86_SEL_RPL) != (pCtx->ss.Sel & X86_SEL_RPL))
                    {
                        STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckBadRpl);
                        return false;
                    }
                    if (   !hmR3IsCodeSelectorOkForVmx(&pCtx->cs, pCtx->ss.Attr.n.u2Dpl)
                        || !hmR3IsDataSelectorOkForVmx(&pCtx->ds)
                        || !hmR3IsDataSelectorOkForVmx(&pCtx->es)
                        || !hmR3IsDataSelectorOkForVmx(&pCtx->fs)
                        || !hmR3IsDataSelectorOkForVmx(&pCtx->gs)
                        || !hmR3IsStackSelectorOkForVmx(&pCtx->ss))
                    {
                        STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckBadSel);
                        return false;
                    }
                }
                /* VT-x also chokes on invalid TR or LDTR selectors (minix). */
                if (pCtx->gdtr.cbGdt)
                {
                    if ((pCtx->tr.Sel | X86_SEL_RPL_LDT) > pCtx->gdtr.cbGdt)
                    {
                        STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckBadTr);
                        return false;
                    }
                    else if ((pCtx->ldtr.Sel | X86_SEL_RPL_LDT) > pCtx->gdtr.cbGdt)
                    {
                        STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckBadLdt);
                        return false;
                    }
                }
                STAM_COUNTER_INC(&pVCpu->hm.s.StatVmxCheckPmOk);
            }
        }
        else
        {
            if (    !CPUMIsGuestInLongModeEx(pCtx)
                &&  !pVM->hm.s.vmx.fUnrestrictedGuest)
            {
                if (    !pVM->hm.s.fNestedPaging        /* Requires a fake PD for real *and* protected mode without paging - stored in the VMM device heap */
                    ||  CPUMIsGuestInRealModeEx(pCtx))  /* Requires a fake TSS for real mode - stored in the VMM device heap */
                    return false;

                /* Too early for VT-x; Solaris guests will fail with a guru meditation otherwise; same for XP. */
                if (pCtx->idtr.pIdt == 0 || pCtx->idtr.cbIdt == 0 || pCtx->tr.Sel == 0)
                    return false;

                /* The guest is about to complete the switch to protected mode. Wait a bit longer. */
                /* Windows XP; switch to protected mode; all selectors are marked not present in the
                 * hidden registers (possible recompiler bug; see load_seg_vm) */
                if (pCtx->cs.Attr.n.u1Present == 0)
                    return false;
                if (pCtx->ss.Attr.n.u1Present == 0)
                    return false;

                /* Windows XP: possible same as above, but new recompiler requires new heuristics?
                   VT-x doesn't seem to like something about the guest state and this stuff avoids it. */
                /** @todo This check is actually wrong, it doesn't take the direction of the
                 *        stack segment into account. But, it does the job for now. */
                if (pCtx->rsp >= pCtx->ss.u32Limit)
                    return false;
            }
        }
    }

    if (pVM->hm.s.vmx.fEnabled)
    {
        uint32_t mask;

        /* if bit N is set in cr0_fixed0, then it must be set in the guest's cr0. */
        mask = (uint32_t)pVM->hm.s.vmx.Msrs.u64Cr0Fixed0;
        /* Note: We ignore the NE bit here on purpose; see vmmr0\hmr0.cpp for details. */
        mask &= ~X86_CR0_NE;

        if (fSupportsRealMode)
        {
            /* Note: We ignore the PE & PG bits here on purpose; we emulate real and protected mode without paging. */
            mask &= ~(X86_CR0_PG|X86_CR0_PE);
        }
        else
        {
            /* We support protected mode without paging using identity mapping. */
            mask &= ~X86_CR0_PG;
        }
        if ((pCtx->cr0 & mask) != mask)
            return false;

        /* if bit N is cleared in cr0_fixed1, then it must be zero in the guest's cr0. */
        mask = (uint32_t)~pVM->hm.s.vmx.Msrs.u64Cr0Fixed1;
        if ((pCtx->cr0 & mask) != 0)
            return false;

        /* if bit N is set in cr4_fixed0, then it must be set in the guest's cr4. */
        mask  = (uint32_t)pVM->hm.s.vmx.Msrs.u64Cr4Fixed0;
        mask &= ~X86_CR4_VMXE;
        if ((pCtx->cr4 & mask) != mask)
            return false;

        /* if bit N is cleared in cr4_fixed1, then it must be zero in the guest's cr4. */
        mask = (uint32_t)~pVM->hm.s.vmx.Msrs.u64Cr4Fixed1;
        if ((pCtx->cr4 & mask) != 0)
            return false;

        pVCpu->hm.s.fActive = true;
        return true;
    }

    return false;
}


/**
 * Checks if we need to reschedule due to VMM device heap changes.
 *
 * @returns true if a reschedule is required, otherwise false.
 * @param   pVM         The cross context VM structure.
 * @param   pCtx        VM execution context.
 */
VMMR3_INT_DECL(bool) HMR3IsRescheduleRequired(PVM pVM, PCPUMCTX pCtx)
{
    /*
     * The VMM device heap is a requirement for emulating real-mode or protected-mode without paging
     * when the unrestricted guest execution feature is missing (VT-x only).
     */
    if (   pVM->hm.s.vmx.fEnabled
        && !pVM->hm.s.vmx.fUnrestrictedGuest
        && CPUMIsGuestInRealModeEx(pCtx)
        && !PDMVmmDevHeapIsEnabled(pVM))
    {
        return true;
    }

    return false;
}


/**
 * Noticiation callback from DBGF when interrupt breakpoints or generic debug
 * event settings changes.
 *
 * DBGF will call HMR3NotifyDebugEventChangedPerCpu on each CPU afterwards, this
 * function is just updating the VM globals.
 *
 * @param   pVM         The VM cross context VM structure.
 * @thread  EMT(0)
 */
VMMR3_INT_DECL(void) HMR3NotifyDebugEventChanged(PVM pVM)
{
    /* Interrupts. */
    bool fUseDebugLoop = pVM->dbgf.ro.cSoftIntBreakpoints > 0
                      || pVM->dbgf.ro.cHardIntBreakpoints > 0;

    /* CPU Exceptions. */
    for (DBGFEVENTTYPE enmEvent = DBGFEVENT_XCPT_FIRST;
         !fUseDebugLoop && enmEvent <= DBGFEVENT_XCPT_LAST;
         enmEvent = (DBGFEVENTTYPE)(enmEvent + 1))
        fUseDebugLoop = DBGF_IS_EVENT_ENABLED(pVM, enmEvent);

    /* Common VM exits. */
    for (DBGFEVENTTYPE enmEvent = DBGFEVENT_EXIT_FIRST;
         !fUseDebugLoop && enmEvent <= DBGFEVENT_EXIT_LAST_COMMON;
         enmEvent = (DBGFEVENTTYPE)(enmEvent + 1))
        fUseDebugLoop = DBGF_IS_EVENT_ENABLED(pVM, enmEvent);

    /* Vendor specific VM exits. */
    if (HMR3IsVmxEnabled(pVM->pUVM))
        for (DBGFEVENTTYPE enmEvent = DBGFEVENT_EXIT_VMX_FIRST;
             !fUseDebugLoop && enmEvent <= DBGFEVENT_EXIT_VMX_LAST;
             enmEvent = (DBGFEVENTTYPE)(enmEvent + 1))
            fUseDebugLoop = DBGF_IS_EVENT_ENABLED(pVM, enmEvent);
    else
        for (DBGFEVENTTYPE enmEvent = DBGFEVENT_EXIT_SVM_FIRST;
             !fUseDebugLoop && enmEvent <= DBGFEVENT_EXIT_SVM_LAST;
             enmEvent = (DBGFEVENTTYPE)(enmEvent + 1))
            fUseDebugLoop = DBGF_IS_EVENT_ENABLED(pVM, enmEvent);

    /* Done. */
    pVM->hm.s.fUseDebugLoop = fUseDebugLoop;
}


/**
 * Follow up notification callback to HMR3NotifyDebugEventChanged for each CPU.
 *
 * HM uses this to combine the decision made by HMR3NotifyDebugEventChanged with
 * per CPU settings.
 *
 * @param   pVM         The VM cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMR3_INT_DECL(void) HMR3NotifyDebugEventChangedPerCpu(PVM pVM, PVMCPU pVCpu)
{
    pVCpu->hm.s.fUseDebugLoop = pVCpu->hm.s.fSingleInstruction | pVM->hm.s.fUseDebugLoop;
}


/**
 * Notification from EM about a rescheduling into hardware assisted execution
 * mode.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
VMMR3_INT_DECL(void) HMR3NotifyScheduled(PVMCPU pVCpu)
{
    HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);
}


/**
 * Notification from EM about returning from instruction emulation (REM / EM).
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3_INT_DECL(void) HMR3NotifyEmulated(PVMCPU pVCpu)
{
    HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);
}


/**
 * Checks if we are currently using hardware acceleration.
 *
 * @returns true if hardware acceleration is being used, otherwise false.
 * @param   pVCpu        The cross context virtual CPU structure.
 */
VMMR3_INT_DECL(bool) HMR3IsActive(PVMCPU pVCpu)
{
    return pVCpu->hm.s.fActive;
}


/**
 * External interface for querying whether hardware acceleration is enabled.
 *
 * @returns true if VT-x or AMD-V is being used, otherwise false.
 * @param   pUVM        The user mode VM handle.
 * @sa      HMIsEnabled, HMIsEnabledNotMacro.
 */
VMMR3DECL(bool) HMR3IsEnabled(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->fHMEnabled; /* Don't use the macro as the GUI may query us very very early. */
}


/**
 * External interface for querying whether VT-x is being used.
 *
 * @returns true if VT-x is being used, otherwise false.
 * @param   pUVM        The user mode VM handle.
 * @sa      HMR3IsSvmEnabled, HMIsEnabled
 */
VMMR3DECL(bool) HMR3IsVmxEnabled(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.vmx.fEnabled
        && pVM->hm.s.vmx.fSupported
        && pVM->fHMEnabled;
}


/**
 * External interface for querying whether AMD-V is being used.
 *
 * @returns true if VT-x is being used, otherwise false.
 * @param   pUVM        The user mode VM handle.
 * @sa      HMR3IsVmxEnabled, HMIsEnabled
 */
VMMR3DECL(bool) HMR3IsSvmEnabled(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.svm.fEnabled
        && pVM->hm.s.svm.fSupported
        && pVM->fHMEnabled;
}


/**
 * Checks if we are currently using nested paging.
 *
 * @returns true if nested paging is being used, otherwise false.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) HMR3IsNestedPagingActive(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.fNestedPaging;
}


/**
 * Checks if virtualized APIC registers is enabled.
 *
 * When enabled this feature allows the hardware to access most of the
 * APIC registers in the virtual-APIC page without causing VM-exits. See
 * Intel spec. 29.1.1 "Virtualized APIC Registers".
 *
 * @returns true if virtualized APIC registers is enabled, otherwise
 *          false.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) HMR3IsVirtApicRegsEnabled(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.fVirtApicRegs;
}


/**
 * Checks if APIC posted-interrupt processing is enabled.
 *
 * This returns whether we can deliver interrupts to the guest without
 * leaving guest-context by updating APIC state from host-context.
 *
 * @returns true if APIC posted-interrupt processing is enabled,
 *          otherwise false.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) HMR3IsPostedIntrsEnabled(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.fPostedIntrs;
}


/**
 * Checks if we are currently using VPID in VT-x mode.
 *
 * @returns true if VPID is being used, otherwise false.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) HMR3IsVpidActive(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.vmx.fVpid;
}


/**
 * Checks if we are currently using VT-x unrestricted execution,
 * aka UX.
 *
 * @returns true if UX is being used, otherwise false.
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(bool) HMR3IsUXActive(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, false);
    return pVM->hm.s.vmx.fUnrestrictedGuest;
}


/**
 * Checks if internal events are pending. In that case we are not allowed to dispatch interrupts.
 *
 * @returns true if an internal event is pending, otherwise false.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3_INT_DECL(bool) HMR3IsEventPending(PVMCPU pVCpu)
{
    return HMIsEnabled(pVCpu->pVMR3) && pVCpu->hm.s.Event.fPending;
}


/**
 * Checks if the VMX-preemption timer is being used.
 *
 * @returns true if the VMX-preemption timer is being used, otherwise false.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(bool) HMR3IsVmxPreemptionTimerUsed(PVM pVM)
{
    return HMIsEnabled(pVM)
        && pVM->hm.s.vmx.fEnabled
        && pVM->hm.s.vmx.fUsePreemptTimer;
}


/**
 * Restart an I/O instruction that was refused in ring-0
 *
 * @returns Strict VBox status code. Informational status codes other than the one documented
 *          here are to be treated as internal failure. Use IOM_SUCCESS() to check for success.
 * @retval  VINF_SUCCESS                Success.
 * @retval  VINF_EM_FIRST-VINF_EM_LAST  Success with some exceptions (see IOM_SUCCESS()), the
 *                                      status code must be passed on to EM.
 * @retval  VERR_NOT_FOUND if no pending I/O instruction.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest CPU context.
 */
VMMR3_INT_DECL(VBOXSTRICTRC) HMR3RestartPendingIOInstr(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Check if we've got relevant data pending.
     */
    HMPENDINGIO enmType = pVCpu->hm.s.PendingIO.enmType;
    if (enmType == HMPENDINGIO_INVALID)
        return VERR_NOT_FOUND;
    pVCpu->hm.s.PendingIO.enmType = HMPENDINGIO_INVALID;
    if (pVCpu->hm.s.PendingIO.GCPtrRip != pCtx->rip)
        return VERR_NOT_FOUND;

    /*
     * Execute pending I/O.
     */
    VBOXSTRICTRC rcStrict;
    switch (enmType)
    {
        case HMPENDINGIO_PORT_READ:
        {
            uint32_t uAndVal = pVCpu->hm.s.PendingIO.s.Port.uAndVal;
            uint32_t u32Val  = 0;

            rcStrict = IOMIOPortRead(pVM, pVCpu, pVCpu->hm.s.PendingIO.s.Port.uPort, &u32Val,
                                     pVCpu->hm.s.PendingIO.s.Port.cbSize);
            if (IOM_SUCCESS(rcStrict))
            {
                /* Write back to the EAX register. */
                pCtx->eax = (pCtx->eax & ~uAndVal) | (u32Val & uAndVal);
                pCtx->rip = pVCpu->hm.s.PendingIO.GCPtrRipNext;
            }
            break;
        }

        default:
            AssertLogRelFailedReturn(VERR_HM_UNKNOWN_IO_INSTRUCTION);
    }

    if (IOM_SUCCESS(rcStrict))
    {
        /*
         * Check for I/O breakpoints.
         */
        uint32_t const uDr7 = pCtx->dr[7];
        if (   (   (uDr7 & X86_DR7_ENABLED_MASK)
                && X86_DR7_ANY_RW_IO(uDr7)
                && (pCtx->cr4 & X86_CR4_DE))
            || DBGFBpIsHwIoArmed(pVM))
        {
            VBOXSTRICTRC rcStrict2 = DBGFBpCheckIo(pVM, pVCpu, pCtx, pVCpu->hm.s.PendingIO.s.Port.uPort,
                                                   pVCpu->hm.s.PendingIO.s.Port.cbSize);
            if (rcStrict2 == VINF_EM_RAW_GUEST_TRAP)
                rcStrict2 = TRPMAssertTrap(pVCpu, X86_XCPT_DB, TRPM_TRAP);
            /* rcStrict is VINF_SUCCESS or in [VINF_EM_FIRST..VINF_EM_LAST]. */
            else if (rcStrict2 != VINF_SUCCESS && (rcStrict == VINF_SUCCESS || rcStrict2 < rcStrict))
                rcStrict = rcStrict2;
        }
    }
    return rcStrict;
}


/**
 * Check fatal VT-x/AMD-V error and produce some meaningful
 * log release message.
 *
 * @param   pVM         The cross context VM structure.
 * @param   iStatusCode VBox status code.
 */
VMMR3_INT_DECL(void) HMR3CheckError(PVM pVM, int iStatusCode)
{
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        switch (iStatusCode)
        {
            /** @todo r=ramshankar: Are all EMTs out of ring-0 at this point!? If not, we
             *  might be getting inaccurate values for non-guru'ing EMTs. */
            case VERR_VMX_INVALID_VMCS_FIELD:
                break;

            case VERR_VMX_INVALID_VMCS_PTR:
                LogRel(("HM: VERR_VMX_INVALID_VMCS_PTR:\n"));
                LogRel(("HM: CPU[%u] Current pointer      %#RGp vs %#RGp\n", i, pVCpu->hm.s.vmx.LastError.u64VMCSPhys,
                                                                                pVCpu->hm.s.vmx.HCPhysVmcs));
                LogRel(("HM: CPU[%u] Current VMCS version %#x\n", i, pVCpu->hm.s.vmx.LastError.u32VMCSRevision));
                LogRel(("HM: CPU[%u] Entered Host Cpu     %u\n",  i, pVCpu->hm.s.vmx.LastError.idEnteredCpu));
                LogRel(("HM: CPU[%u] Current Host Cpu     %u\n",  i, pVCpu->hm.s.vmx.LastError.idCurrentCpu));
                break;

            case VERR_VMX_UNABLE_TO_START_VM:
                LogRel(("HM: VERR_VMX_UNABLE_TO_START_VM:\n"));
                LogRel(("HM: CPU[%u] Instruction error    %#x\n", i, pVCpu->hm.s.vmx.LastError.u32InstrError));
                LogRel(("HM: CPU[%u] Exit reason          %#x\n", i, pVCpu->hm.s.vmx.LastError.u32ExitReason));

                if (   pVM->aCpus[i].hm.s.vmx.LastError.u32InstrError == VMX_ERROR_VMLAUCH_NON_CLEAR_VMCS
                    || pVM->aCpus[i].hm.s.vmx.LastError.u32InstrError == VMX_ERROR_VMRESUME_NON_LAUNCHED_VMCS)
                {
                    LogRel(("HM: CPU[%u] Entered Host Cpu     %u\n",  i, pVCpu->hm.s.vmx.LastError.idEnteredCpu));
                    LogRel(("HM: CPU[%u] Current Host Cpu     %u\n",  i, pVCpu->hm.s.vmx.LastError.idCurrentCpu));
                }
                else if (pVM->aCpus[i].hm.s.vmx.LastError.u32InstrError == VMX_ERROR_VMENTRY_INVALID_CONTROL_FIELDS)
                {
                    LogRel(("HM: CPU[%u] PinCtls          %#RX32\n", i, pVCpu->hm.s.vmx.u32PinCtls));
                    LogRel(("HM: CPU[%u] ProcCtls         %#RX32\n", i, pVCpu->hm.s.vmx.u32ProcCtls));
                    LogRel(("HM: CPU[%u] ProcCtls2        %#RX32\n", i, pVCpu->hm.s.vmx.u32ProcCtls2));
                    LogRel(("HM: CPU[%u] EntryCtls        %#RX32\n", i, pVCpu->hm.s.vmx.u32EntryCtls));
                    LogRel(("HM: CPU[%u] ExitCtls         %#RX32\n", i, pVCpu->hm.s.vmx.u32ExitCtls));
                    LogRel(("HM: CPU[%u] HCPhysMsrBitmap  %#RHp\n",  i, pVCpu->hm.s.vmx.HCPhysMsrBitmap));
                    LogRel(("HM: CPU[%u] HCPhysGuestMsr   %#RHp\n",  i, pVCpu->hm.s.vmx.HCPhysGuestMsr));
                    LogRel(("HM: CPU[%u] HCPhysHostMsr    %#RHp\n",  i, pVCpu->hm.s.vmx.HCPhysHostMsr));
                    LogRel(("HM: CPU[%u] cMsrs            %u\n",     i, pVCpu->hm.s.vmx.cMsrs));
                }
                /** @todo Log VM-entry event injection control fields
                 *        VMX_VMCS_CTRL_ENTRY_IRQ_INFO, VMX_VMCS_CTRL_ENTRY_EXCEPTION_ERRCODE
                 *        and VMX_VMCS_CTRL_ENTRY_INSTR_LENGTH from the VMCS. */
                break;

            /* The guru will dump the HM error and exit history. Nothing extra to report for these errors. */
            case VERR_VMX_INVALID_VMXON_PTR:
            case VERR_HM_UNSUPPORTED_CPU_FEATURE_COMBO:
            case VERR_VMX_INVALID_GUEST_STATE:
            case VERR_VMX_UNEXPECTED_EXIT:
            case VERR_SVM_UNKNOWN_EXIT:
            case VERR_SVM_UNEXPECTED_EXIT:
            case VERR_SVM_UNEXPECTED_PATCH_TYPE:
            case VERR_SVM_UNEXPECTED_XCPT_EXIT:
            case VERR_VMX_UNEXPECTED_INTERRUPTION_EXIT_TYPE:
                break;
        }
    }

    if (iStatusCode == VERR_VMX_UNABLE_TO_START_VM)
    {
        LogRel(("HM: VERR_VMX_UNABLE_TO_START_VM: VM-entry allowed    %#RX32\n", pVM->hm.s.vmx.Msrs.VmxEntry.n.allowed1));
        LogRel(("HM: VERR_VMX_UNABLE_TO_START_VM: VM-entry disallowed %#RX32\n", pVM->hm.s.vmx.Msrs.VmxEntry.n.disallowed0));
    }
    else if (iStatusCode == VERR_VMX_INVALID_VMXON_PTR)
        LogRel(("HM: HCPhysVmxEnableError         = %#RHp\n", pVM->hm.s.vmx.HCPhysVmxEnableError));
}


/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) hmR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    int rc;

    Log(("hmR3Save:\n"));

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        /*
         * Save the basic bits - fortunately all the other things can be resynced on load.
         */
        rc = SSMR3PutU32(pSSM, pVM->aCpus[i].hm.s.Event.fPending);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU32(pSSM, pVM->aCpus[i].hm.s.Event.u32ErrCode);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU64(pSSM, pVM->aCpus[i].hm.s.Event.u64IntInfo);
        AssertRCReturn(rc, rc);
        /** @todo Shouldn't we be saving GCPtrFaultAddress too? */

        /** @todo We only need to save pVM->aCpus[i].hm.s.vmx.fWasInRealMode and
         *        perhaps not even that (the initial value of @c true is safe. */
        uint32_t u32Dummy = PGMMODE_REAL;
        rc = SSMR3PutU32(pSSM, u32Dummy);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU32(pSSM, u32Dummy);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU32(pSSM, u32Dummy);
        AssertRCReturn(rc, rc);
    }

#ifdef VBOX_HM_WITH_GUEST_PATCHING
    rc = SSMR3PutGCPtr(pSSM, pVM->hm.s.pGuestPatchMem);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutGCPtr(pSSM, pVM->hm.s.pFreeGuestPatchMem);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pVM->hm.s.cbGuestPatchMem);
    AssertRCReturn(rc, rc);

    /* Store all the guest patch records too. */
    rc = SSMR3PutU32(pSSM, pVM->hm.s.cPatches);
    AssertRCReturn(rc, rc);

    for (unsigned i = 0; i < pVM->hm.s.cPatches; i++)
    {
        PHMTPRPATCH pPatch = &pVM->hm.s.aPatches[i];

        rc = SSMR3PutU32(pSSM, pPatch->Core.Key);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutMem(pSSM, pPatch->aOpcode, sizeof(pPatch->aOpcode));
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, pPatch->cbOp);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutMem(pSSM, pPatch->aNewOpcode, sizeof(pPatch->aNewOpcode));
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, pPatch->cbNewOp);
        AssertRCReturn(rc, rc);

        AssertCompileSize(HMTPRINSTR, 4);
        rc = SSMR3PutU32(pSSM, (uint32_t)pPatch->enmType);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, pPatch->uSrcOperand);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, pPatch->uDstOperand);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, pPatch->pJumpTarget);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, pPatch->cFaults);
        AssertRCReturn(rc, rc);
    }
#endif
    return VINF_SUCCESS;
}


/**
 * Execute state load operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 * @param   uVersion        Data layout version.
 * @param   uPass           The data pass.
 */
static DECLCALLBACK(int) hmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    int rc;

    Log(("hmR3Load:\n"));
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Validate version.
     */
    if (   uVersion != HM_SAVED_STATE_VERSION
        && uVersion != HM_SAVED_STATE_VERSION_NO_PATCHING
        && uVersion != HM_SAVED_STATE_VERSION_2_0_X)
    {
        AssertMsgFailed(("hmR3Load: Invalid version uVersion=%d!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        rc = SSMR3GetU32(pSSM, &pVM->aCpus[i].hm.s.Event.fPending);
        AssertRCReturn(rc, rc);
        rc = SSMR3GetU32(pSSM, &pVM->aCpus[i].hm.s.Event.u32ErrCode);
        AssertRCReturn(rc, rc);
        rc = SSMR3GetU64(pSSM, &pVM->aCpus[i].hm.s.Event.u64IntInfo);
        AssertRCReturn(rc, rc);

        if (uVersion >= HM_SAVED_STATE_VERSION_NO_PATCHING)
        {
            uint32_t val;
            /** @todo See note in hmR3Save(). */
            rc = SSMR3GetU32(pSSM, &val);
            AssertRCReturn(rc, rc);
            rc = SSMR3GetU32(pSSM, &val);
            AssertRCReturn(rc, rc);
            rc = SSMR3GetU32(pSSM, &val);
            AssertRCReturn(rc, rc);
        }
    }
#ifdef VBOX_HM_WITH_GUEST_PATCHING
    if (uVersion > HM_SAVED_STATE_VERSION_NO_PATCHING)
    {
        rc = SSMR3GetGCPtr(pSSM, &pVM->hm.s.pGuestPatchMem);
        AssertRCReturn(rc, rc);
        rc = SSMR3GetGCPtr(pSSM, &pVM->hm.s.pFreeGuestPatchMem);
        AssertRCReturn(rc, rc);
        rc = SSMR3GetU32(pSSM, &pVM->hm.s.cbGuestPatchMem);
        AssertRCReturn(rc, rc);

        /* Fetch all TPR patch records. */
        rc = SSMR3GetU32(pSSM, &pVM->hm.s.cPatches);
        AssertRCReturn(rc, rc);

        for (unsigned i = 0; i < pVM->hm.s.cPatches; i++)
        {
            PHMTPRPATCH pPatch = &pVM->hm.s.aPatches[i];

            rc = SSMR3GetU32(pSSM, &pPatch->Core.Key);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetMem(pSSM, pPatch->aOpcode, sizeof(pPatch->aOpcode));
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &pPatch->cbOp);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetMem(pSSM, pPatch->aNewOpcode, sizeof(pPatch->aNewOpcode));
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &pPatch->cbNewOp);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, (uint32_t *)&pPatch->enmType);
            AssertRCReturn(rc, rc);

            if (pPatch->enmType == HMTPRINSTR_JUMP_REPLACEMENT)
                pVM->hm.s.fTPRPatchingActive = true;

            Assert(pPatch->enmType == HMTPRINSTR_JUMP_REPLACEMENT || pVM->hm.s.fTPRPatchingActive == false);

            rc = SSMR3GetU32(pSSM, &pPatch->uSrcOperand);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &pPatch->uDstOperand);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &pPatch->cFaults);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &pPatch->pJumpTarget);
            AssertRCReturn(rc, rc);

            Log(("hmR3Load: patch %d\n", i));
            Log(("Key       = %x\n", pPatch->Core.Key));
            Log(("cbOp      = %d\n", pPatch->cbOp));
            Log(("cbNewOp   = %d\n", pPatch->cbNewOp));
            Log(("type      = %d\n", pPatch->enmType));
            Log(("srcop     = %d\n", pPatch->uSrcOperand));
            Log(("dstop     = %d\n", pPatch->uDstOperand));
            Log(("cFaults   = %d\n", pPatch->cFaults));
            Log(("target    = %x\n", pPatch->pJumpTarget));
            rc = RTAvloU32Insert(&pVM->hm.s.PatchTree, &pPatch->Core);
            AssertRC(rc);
        }
    }
#endif

    return VINF_SUCCESS;
}


/**
 * Displays the guest VM-exit history.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) hmR3InfoExitHistory(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    NOREF(pszArgs);
    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    if (HMIsEnabled(pVM))
    {
        bool const fIsVtx = pVM->hm.s.vmx.fSupported;
        const char * const *papszDesc;
        unsigned cMaxExitDesc;
        if (fIsVtx)
        {
            cMaxExitDesc = MAX_EXITREASON_VTX;
            papszDesc    = &g_apszVTxExitReasons[0];
            pHlp->pfnPrintf(pHlp, "CPU[%u]: VT-x VM-exit history:\n", pVCpu->idCpu);
        }
        else
        {
            cMaxExitDesc = MAX_EXITREASON_AMDV;
            papszDesc    = &g_apszAmdVExitReasons[0];
            pHlp->pfnPrintf(pHlp, "CPU[%u]: AMD-V #VMEXIT history:\n", pVCpu->idCpu);
        }

        pHlp->pfnPrintf(pHlp, "  idxExitHistoryFree = %u\n", pVCpu->hm.s.idxExitHistoryFree);
        unsigned const idxLast = pVCpu->hm.s.idxExitHistoryFree > 0 ?
                                                                    pVCpu->hm.s.idxExitHistoryFree - 1 :
                                                                    RT_ELEMENTS(pVCpu->hm.s.auExitHistory) - 1;
        for (unsigned i = 0; i < RT_ELEMENTS(pVCpu->hm.s.auExitHistory); i++)
        {
            uint16_t const uExit = pVCpu->hm.s.auExitHistory[i];
            const char *pszExit  = NULL;
            if (uExit <= cMaxExitDesc)
                pszExit = papszDesc[uExit];
            else if (!fIsVtx)
                pszExit = hmSvmGetSpecialExitReasonDesc(uExit);
            else
                pszExit = NULL;

            pHlp->pfnPrintf(pHlp, "  auExitHistory[%2u] = 0x%04x  %s %s\n", i, uExit, pszExit,
                            idxLast == i ? "<-- Latest exit" : "");
        }
        pHlp->pfnPrintf(pHlp, "HM error = %#x (%u)\n", pVCpu->hm.s.u32HMError, pVCpu->hm.s.u32HMError);
    }
    else
        pHlp->pfnPrintf(pHlp, "HM is not enabled for this VM!\n");
}


/**
 * Displays the HM pending event.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) hmR3InfoEventPending(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    NOREF(pszArgs);
    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    if (HMIsEnabled(pVM))
    {
        pHlp->pfnPrintf(pHlp, "CPU[%u]: HM event (fPending=%RTbool)\n", pVCpu->idCpu, pVCpu->hm.s.Event.fPending);
        if (pVCpu->hm.s.Event.fPending)
        {
            pHlp->pfnPrintf(pHlp, "  u64IntInfo        = %#RX64\n", pVCpu->hm.s.Event.u64IntInfo);
            pHlp->pfnPrintf(pHlp, "  u32ErrCode        = %#RX64\n", pVCpu->hm.s.Event.u32ErrCode);
            pHlp->pfnPrintf(pHlp, "  cbInstr           = %u bytes\n", pVCpu->hm.s.Event.cbInstr);
            pHlp->pfnPrintf(pHlp, "  GCPtrFaultAddress = %#RGp\n", pVCpu->hm.s.Event.GCPtrFaultAddress);
        }
    }
    else
        pHlp->pfnPrintf(pHlp, "HM is not enabled for this VM!\n");
}


/**
 * Displays SVM VMCB controls.
 *
 * @param   pHlp        The info helper functions.
 * @param   pVmcbCtrl   Pointer to a SVM VMCB controls area.
 * @param   pszPrefix   Caller specified string prefix.
 */
VMMR3_INT_DECL(void) HMR3InfoSvmVmcbCtrl(PCDBGFINFOHLP pHlp, PCSVMVMCBCTRL pVmcbCtrl, const char *pszPrefix)
{
    AssertReturnVoid(pHlp);
    AssertReturnVoid(pVmcbCtrl);

    pHlp->pfnPrintf(pHlp, "%su16InterceptRdCRx          = %#RX16\n",    pszPrefix, pVmcbCtrl->u16InterceptRdCRx);
    pHlp->pfnPrintf(pHlp, "%su16InterceptWrCRx          = %#RX16\n",    pszPrefix, pVmcbCtrl->u16InterceptWrCRx);
    pHlp->pfnPrintf(pHlp, "%su16InterceptRdDRx          = %#RX16\n",    pszPrefix, pVmcbCtrl->u16InterceptRdDRx);
    pHlp->pfnPrintf(pHlp, "%su16InterceptWrDRx          = %#RX16\n",    pszPrefix, pVmcbCtrl->u16InterceptWrDRx);
    pHlp->pfnPrintf(pHlp, "%su32InterceptXcpt           = %#RX32\n",    pszPrefix, pVmcbCtrl->u32InterceptXcpt);
    pHlp->pfnPrintf(pHlp, "%su64InterceptCtrl           = %#RX64\n",    pszPrefix, pVmcbCtrl->u64InterceptCtrl);
    pHlp->pfnPrintf(pHlp, "%su16PauseFilterThreshold    = %#RX16\n",    pszPrefix, pVmcbCtrl->u16PauseFilterThreshold);
    pHlp->pfnPrintf(pHlp, "%su16PauseFilterCount        = %#RX16\n",    pszPrefix, pVmcbCtrl->u16PauseFilterCount);
    pHlp->pfnPrintf(pHlp, "%su64IOPMPhysAddr            = %#RX64\n",    pszPrefix, pVmcbCtrl->u64IOPMPhysAddr);
    pHlp->pfnPrintf(pHlp, "%su64MSRPMPhysAddr           = %#RX64\n",    pszPrefix, pVmcbCtrl->u64MSRPMPhysAddr);
    pHlp->pfnPrintf(pHlp, "%su64TSCOffset               = %#RX64\n",    pszPrefix, pVmcbCtrl->u64TSCOffset);
    pHlp->pfnPrintf(pHlp, "%sTLBCtrl\n",                                pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u32ASID                    = %#RX32\n",  pszPrefix, pVmcbCtrl->TLBCtrl.n.u32ASID);
    pHlp->pfnPrintf(pHlp, "%s  u8TLBFlush                 = %u\n",      pszPrefix, pVmcbCtrl->TLBCtrl.n.u8TLBFlush);
    pHlp->pfnPrintf(pHlp, "%sIntCtrl\n",                                pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u8VTPR                     = %#RX8 (%u)\n",   pszPrefix, pVmcbCtrl->IntCtrl.n.u8VTPR, pVmcbCtrl->IntCtrl.n.u8VTPR);
    pHlp->pfnPrintf(pHlp, "%s  u1VIrqPending              = %RTbool\n", pszPrefix, pVmcbCtrl->IntCtrl.n.u1VIrqPending);
    pHlp->pfnPrintf(pHlp, "%s  u4VIntrPrio                = %#RX8\n",   pszPrefix, pVmcbCtrl->IntCtrl.n.u4VIntrPrio);
    pHlp->pfnPrintf(pHlp, "%s  u1IgnoreTPR                = %RTbool\n", pszPrefix, pVmcbCtrl->IntCtrl.n.u1IgnoreTPR);
    pHlp->pfnPrintf(pHlp, "%s  u1VIntrMasking             = %RTbool\n", pszPrefix, pVmcbCtrl->IntCtrl.n.u1VIntrMasking);
    pHlp->pfnPrintf(pHlp, "%s  u1AvicEnable               = %RTbool\n", pszPrefix, pVmcbCtrl->IntCtrl.n.u1AvicEnable);
    pHlp->pfnPrintf(pHlp, "%s  u8VIntrVector              = %#RX8\n",   pszPrefix, pVmcbCtrl->IntCtrl.n.u8VIntrVector);
    pHlp->pfnPrintf(pHlp, "%su64IntShadow               = %#RX64\n",    pszPrefix, pVmcbCtrl->u64IntShadow);
    pHlp->pfnPrintf(pHlp, "%su64ExitCode                = %#RX64\n",    pszPrefix, pVmcbCtrl->u64ExitCode);
    pHlp->pfnPrintf(pHlp, "%su64ExitInfo1               = %#RX64\n",    pszPrefix, pVmcbCtrl->u64ExitInfo1);
    pHlp->pfnPrintf(pHlp, "%su64ExitInfo2               = %#RX64\n",    pszPrefix, pVmcbCtrl->u64ExitInfo2);
    pHlp->pfnPrintf(pHlp, "%sExitIntInfo\n",                            pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u8Vector                   = %#RX8 (%u)\n", pszPrefix, pVmcbCtrl->ExitIntInfo.n.u8Vector, pVmcbCtrl->ExitIntInfo.n.u8Vector);
    pHlp->pfnPrintf(pHlp, "%s  u3Type                     = %u\n",      pszPrefix, pVmcbCtrl->ExitIntInfo.n.u3Type);
    pHlp->pfnPrintf(pHlp, "%s  u1ErrorCodeValid           = %RTbool\n", pszPrefix, pVmcbCtrl->ExitIntInfo.n.u1ErrorCodeValid);
    pHlp->pfnPrintf(pHlp, "%s  u1Valid                    = %RTbool\n", pszPrefix, pVmcbCtrl->ExitIntInfo.n.u1Valid);
    pHlp->pfnPrintf(pHlp, "%s  u32ErrorCode               = %#RX32\n",  pszPrefix, pVmcbCtrl->ExitIntInfo.n.u32ErrorCode);
    pHlp->pfnPrintf(pHlp, "%sNestedPaging\n",                           pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u1NestedPaging             = %RTbool\n", pszPrefix, pVmcbCtrl->NestedPaging.n.u1NestedPaging);
    pHlp->pfnPrintf(pHlp, "%sAvicBar\n",                                pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u40Addr                    = %#RX64\n",  pszPrefix, pVmcbCtrl->AvicBar.n.u40Addr);
    pHlp->pfnPrintf(pHlp, "%sEventInject\n",                            pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  EventInject\n",                          pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u8Vector                   = %#RX32 (%u)\n", pszPrefix, pVmcbCtrl->EventInject.n.u8Vector, pVmcbCtrl->EventInject.n.u8Vector);
    pHlp->pfnPrintf(pHlp, "%s  u3Type                     = %u\n",      pszPrefix, pVmcbCtrl->EventInject.n.u3Type);
    pHlp->pfnPrintf(pHlp, "%s  u1ErrorCodeValid           = %RTbool\n", pszPrefix, pVmcbCtrl->EventInject.n.u1ErrorCodeValid);
    pHlp->pfnPrintf(pHlp, "%s  u1Valid                    = %RTbool\n", pszPrefix, pVmcbCtrl->EventInject.n.u1Valid);
    pHlp->pfnPrintf(pHlp, "%s  u32ErrorCode               = %#RX32\n",  pszPrefix, pVmcbCtrl->EventInject.n.u32ErrorCode);
    pHlp->pfnPrintf(pHlp, "%su64NestedPagingCR3         = %#RX64\n",    pszPrefix, pVmcbCtrl->u64NestedPagingCR3);
    pHlp->pfnPrintf(pHlp, "%su64LBRVirt                 = %#RX64\n",    pszPrefix, pVmcbCtrl->u64LBRVirt);
    pHlp->pfnPrintf(pHlp, "%su64VmcbCleanBits           = %#RX64\n",    pszPrefix, pVmcbCtrl->u64VmcbCleanBits);
    pHlp->pfnPrintf(pHlp, "%su64NextRIP                 = %#RX64\n",    pszPrefix, pVmcbCtrl->u64NextRIP);
    pHlp->pfnPrintf(pHlp, "%scbInstrFetched             = %u\n",        pszPrefix, pVmcbCtrl->cbInstrFetched);
    pHlp->pfnPrintf(pHlp, "%sabInstr                    = %.*Rhxs\n",   pszPrefix, sizeof(pVmcbCtrl->abInstr), pVmcbCtrl->abInstr);
    pHlp->pfnPrintf(pHlp, "%sAvicBackingPagePtr\n",                     pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u40Addr                    = %#RX64\n",  pszPrefix, pVmcbCtrl->AvicBackingPagePtr.n.u40Addr);
    pHlp->pfnPrintf(pHlp, "%sAvicLogicalTablePtr\n",                    pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u40Addr                    = %#RX64\n",  pszPrefix, pVmcbCtrl->AvicLogicalTablePtr.n.u40Addr);
    pHlp->pfnPrintf(pHlp, "%sAvicPhysicalTablePtr\n",                   pszPrefix);
    pHlp->pfnPrintf(pHlp, "%s  u8LastGuestCoreId          = %u\n",      pszPrefix, pVmcbCtrl->AvicPhysicalTablePtr.n.u8LastGuestCoreId);
    pHlp->pfnPrintf(pHlp, "%s  u40Addr                    = %#RX64\n",  pszPrefix, pVmcbCtrl->AvicPhysicalTablePtr.n.u40Addr);
}

