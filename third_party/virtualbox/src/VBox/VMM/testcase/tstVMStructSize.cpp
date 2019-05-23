/* $Id: tstVMStructSize.cpp $ */
/** @file
 * tstVMStructSize - testcase for check structure sizes/alignment
 *                   and to verify that HC and GC uses the same
 *                   representation of the structures.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define IN_TSTVMSTRUCT 1
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/stam.h>
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include "CFGMInternal.h"
#include "CPUMInternal.h"
#include "MMInternal.h"
#include "PGMInternal.h"
#include "SELMInternal.h"
#include "TRPMInternal.h"
#include "TMInternal.h"
#include "IOMInternal.h"
#include "REMInternal.h"
#include "SSMInternal.h"
#include "HMInternal.h"
#include "VMMInternal.h"
#include "DBGFInternal.h"
#include "GIMInternal.h"
#include "APICInternal.h"
#include "STAMInternal.h"
#include "VMInternal.h"
#include "EMInternal.h"
#include "IEMInternal.h"
#include "REMInternal.h"
#include "../VMMR0/GMMR0Internal.h"
#include "../VMMR0/GVMMR0Internal.h"
#ifdef VBOX_WITH_RAW_MODE
# include "CSAMInternal.h"
# include "PATMInternal.h"
#endif
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/vmm/gvm.h>
#include <VBox/param.h>
#include <VBox/dis.h>
#include <iprt/x86.h>

#include "tstHelp.h"
#include <stdio.h>



int main()
{
    int rc = 0;
    printf("tstVMStructSize: TESTING\n");

    printf("info: struct VM: %d bytes\n", (int)sizeof(VM));

#define CHECK_PADDING_VM(align, member) \
    do \
    { \
        CHECK_PADDING(VM, member, align); \
        CHECK_MEMBER_ALIGNMENT(VM, member, align); \
        VM *p = NULL; NOREF(p); \
        if (sizeof(p->member.padding) >= (ssize_t)sizeof(p->member.s) + 128 + sizeof(p->member.s) / 20) \
            printf("warning: VM::%-8s: padding=%-5d s=%-5d -> %-4d  suggest=%-5u\n", \
                   #member, (int)sizeof(p->member.padding), (int)sizeof(p->member.s), \
                   (int)sizeof(p->member.padding) - (int)sizeof(p->member.s), \
                   (int)RT_ALIGN_Z(sizeof(p->member.s), (align))); \
    } while (0)


#define CHECK_PADDING_VMCPU(align, member) \
    do \
    { \
        CHECK_PADDING(VMCPU, member, align); \
        CHECK_MEMBER_ALIGNMENT(VMCPU, member, align); \
        VMCPU *p = NULL; NOREF(p); \
        if (sizeof(p->member.padding) >= (ssize_t)sizeof(p->member.s) + 128 + sizeof(p->member.s) / 20) \
            printf("warning: VMCPU::%-8s: padding=%-5d s=%-5d -> %-4d  suggest=%-5u\n", \
                   #member, (int)sizeof(p->member.padding), (int)sizeof(p->member.s), \
                   (int)sizeof(p->member.padding) - (int)sizeof(p->member.s), \
                   (int)RT_ALIGN_Z(sizeof(p->member.s), (align))); \
    } while (0)

#define CHECK_CPUMCTXCORE(member) \
    do { \
        unsigned off1 = RT_OFFSETOF(CPUMCTX, member) - RT_OFFSETOF(CPUMCTX, rax); \
        unsigned off2 = RT_OFFSETOF(CPUMCTXCORE, member); \
        if (off1 != off2) \
        { \
            printf("error! CPUMCTX/CORE:: %s! (%#x vs %#x (ctx))\n", #member, off1, off2); \
            rc++; \
        } \
    } while (0)

#define CHECK_PADDING_UVM(align, member) \
    do \
    { \
        CHECK_PADDING(UVM, member, align); \
        CHECK_MEMBER_ALIGNMENT(UVM, member, align); \
        UVM *p = NULL; NOREF(p); \
        if (sizeof(p->member.padding) >= (ssize_t)sizeof(p->member.s) + 128 + sizeof(p->member.s) / 20) \
            printf("warning: UVM::%-8s: padding=%-5d s=%-5d -> %-4d  suggest=%-5u\n", \
                   #member, (int)sizeof(p->member.padding), (int)sizeof(p->member.s), \
                   (int)sizeof(p->member.padding) - (int)sizeof(p->member.s), \
                   (int)RT_ALIGN_Z(sizeof(p->member.s), (align))); \
    } while (0)

#define CHECK_PADDING_UVMCPU(align, member) \
    do \
    { \
        CHECK_PADDING(UVMCPU, member, align); \
        CHECK_MEMBER_ALIGNMENT(UVMCPU, member, align); \
        UVMCPU *p = NULL; NOREF(p); \
        if (sizeof(p->member.padding) >= (ssize_t)sizeof(p->member.s) + 128 + sizeof(p->member.s) / 20) \
            printf("warning: UVMCPU::%-8s: padding=%-5d s=%-5d -> %-4d  suggest=%-5u\n", \
                   #member, (int)sizeof(p->member.padding), (int)sizeof(p->member.s), \
                   (int)sizeof(p->member.padding) - (int)sizeof(p->member.s), \
                   (int)RT_ALIGN_Z(sizeof(p->member.s), (align))); \
    } while (0)

#define CHECK_PADDING_GVM(align, member) \
    do \
    { \
        CHECK_PADDING(GVM, member, align); \
        CHECK_MEMBER_ALIGNMENT(GVM, member, align); \
        GVM *p = NULL; NOREF(p); \
        if (sizeof(p->member.padding) >= (ssize_t)sizeof(p->member.s) + 128 + sizeof(p->member.s) / 20) \
            printf("warning: GVM::%-8s: padding=%-5d s=%-5d -> %-4d  suggest=%-5u\n", \
                   #member, (int)sizeof(p->member.padding), (int)sizeof(p->member.s), \
                   (int)sizeof(p->member.padding) - (int)sizeof(p->member.s), \
                   (int)RT_ALIGN_Z(sizeof(p->member.s), (align))); \
    } while (0)

#define CHECK_PADDING_GVMCPU(align, member) \
    do \
    { \
        CHECK_PADDING(GVMCPU, member, align); \
        CHECK_MEMBER_ALIGNMENT(GVMCPU, member, align); \
        GVMCPU *p = NULL; NOREF(p); \
        if (sizeof(p->member.padding) >= (ssize_t)sizeof(p->member.s) + 128 + sizeof(p->member.s) / 20) \
            printf("warning: GVMCPU::%-8s: padding=%-5d s=%-5d -> %-4d  suggest=%-5u\n", \
                   #member, (int)sizeof(p->member.padding), (int)sizeof(p->member.s), \
                   (int)sizeof(p->member.padding) - (int)sizeof(p->member.s), \
                   (int)RT_ALIGN_Z(sizeof(p->member.s), (align))); \
    } while (0)

#define PRINT_OFFSET(strct, member) \
    do \
    { \
        printf("info: %10s::%-24s offset %#6x (%6d) sizeof %4d\n",  #strct, #member, (int)RT_OFFSETOF(strct, member), (int)RT_OFFSETOF(strct, member), (int)RT_SIZEOFMEMB(strct, member)); \
    } while (0)


    CHECK_SIZE(uint128_t, 128/8);
    CHECK_SIZE(int128_t, 128/8);
    CHECK_SIZE(uint64_t, 64/8);
    CHECK_SIZE(int64_t, 64/8);
    CHECK_SIZE(uint32_t, 32/8);
    CHECK_SIZE(int32_t, 32/8);
    CHECK_SIZE(uint16_t, 16/8);
    CHECK_SIZE(int16_t, 16/8);
    CHECK_SIZE(uint8_t, 8/8);
    CHECK_SIZE(int8_t, 8/8);

    CHECK_SIZE(X86DESC, 8);
    CHECK_SIZE(X86DESC64, 16);
    CHECK_SIZE(VBOXIDTE, 8);
    CHECK_SIZE(VBOXIDTR, 10);
    CHECK_SIZE(VBOXGDTR, 10);
    CHECK_SIZE(VBOXTSS, 136);
    CHECK_SIZE(X86FXSTATE, 512);
    CHECK_SIZE(RTUUID, 16);
    CHECK_SIZE(X86PTE, 4);
    CHECK_SIZE(X86PD, PAGE_SIZE);
    CHECK_SIZE(X86PDE, 4);
    CHECK_SIZE(X86PT, PAGE_SIZE);
    CHECK_SIZE(X86PTEPAE, 8);
    CHECK_SIZE(X86PTPAE, PAGE_SIZE);
    CHECK_SIZE(X86PDEPAE, 8);
    CHECK_SIZE(X86PDPAE, PAGE_SIZE);
    CHECK_SIZE(X86PDPE, 8);
    CHECK_SIZE(X86PDPT, PAGE_SIZE);
    CHECK_SIZE(X86PML4E, 8);
    CHECK_SIZE(X86PML4, PAGE_SIZE);

    PRINT_OFFSET(VM, cpum);
    CHECK_PADDING_VM(64, cpum);
    CHECK_PADDING_VM(64, vmm);
    PRINT_OFFSET(VM, pgm);
    PRINT_OFFSET(VM, pgm.s.CritSectX);
    CHECK_PADDING_VM(64, pgm);
    PRINT_OFFSET(VM, hm);
    CHECK_PADDING_VM(64, hm);
    CHECK_PADDING_VM(64, trpm);
    CHECK_PADDING_VM(64, selm);
    CHECK_PADDING_VM(64, mm);
    CHECK_PADDING_VM(64, pdm);
    PRINT_OFFSET(VM, pdm.s.CritSect);
    CHECK_PADDING_VM(64, iom);
#ifdef VBOX_WITH_RAW_MODE
    CHECK_PADDING_VM(64, patm);
    CHECK_PADDING_VM(64, csam);
#endif
    CHECK_PADDING_VM(64, em);
    /*CHECK_PADDING_VM(64, iem);*/
    CHECK_PADDING_VM(64, tm);
    PRINT_OFFSET(VM, tm.s.VirtualSyncLock);
    CHECK_PADDING_VM(64, dbgf);
    CHECK_PADDING_VM(64, gim);
    CHECK_PADDING_VM(64, ssm);
#ifdef VBOX_WITH_REM
    CHECK_PADDING_VM(64, rem);
#endif
    CHECK_PADDING_VM(8, vm);
    CHECK_PADDING_VM(8, cfgm);
    CHECK_PADDING_VM(8, apic);

    PRINT_OFFSET(VMCPU, cpum);
    CHECK_PADDING_VMCPU(64, iem);
    CHECK_PADDING_VMCPU(64, hm);
    CHECK_PADDING_VMCPU(64, em);
    CHECK_PADDING_VMCPU(64, trpm);
    CHECK_PADDING_VMCPU(64, tm);
    CHECK_PADDING_VMCPU(64, vmm);
    CHECK_PADDING_VMCPU(64, pdm);
    CHECK_PADDING_VMCPU(64, iom);
    CHECK_PADDING_VMCPU(64, dbgf);
    CHECK_PADDING_VMCPU(64, gim);
    CHECK_PADDING_VMCPU(64, apic);

    PRINT_OFFSET(VMCPU, pgm);
    CHECK_PADDING_VMCPU(4096, pgm);
    CHECK_PADDING_VMCPU(4096, cpum);
#ifdef VBOX_WITH_STATISTICS
    PRINT_OFFSET(VMCPU, pgm.s.pStatTrap0eAttributionRC);
#endif

    CHECK_MEMBER_ALIGNMENT(VM, selm.s.Tss, 16);
    PRINT_OFFSET(VM, selm.s.Tss);
    PVM pVM = NULL; NOREF(pVM);
    if ((RT_OFFSETOF(VM, selm.s.Tss) & PAGE_OFFSET_MASK) > PAGE_SIZE - sizeof(pVM->selm.s.Tss))
    {
        printf("error! SELM:Tss is crossing a page!\n");
        rc++;
    }
    PRINT_OFFSET(VM, selm.s.TssTrap08);
    if ((RT_OFFSETOF(VM, selm.s.TssTrap08) & PAGE_OFFSET_MASK) > PAGE_SIZE - sizeof(pVM->selm.s.TssTrap08))
    {
        printf("error! SELM:TssTrap08 is crossing a page!\n");
        rc++;
    }
    CHECK_MEMBER_ALIGNMENT(VM, trpm.s.aIdt, 16);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0], PAGE_SIZE);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[1], PAGE_SIZE);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0].cpum.s.Host, 64);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0].cpum.s.Guest, 64);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[1].cpum.s.Host, 64);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[1].cpum.s.Guest, 64);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0].cpum.s.Hyper, 64);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[1].cpum.s.Hyper, 64);
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0].cpum.s.pvApicBase, 8);
#endif

    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0].iem.s.DataTlb, 64);
    CHECK_MEMBER_ALIGNMENT(VM, aCpus[0].iem.s.CodeTlb, 64);

    CHECK_MEMBER_ALIGNMENT(VMCPU, vmm.s.u64CallRing3Arg, 8);
#if defined(RT_OS_WINDOWS) && defined(RT_ARCH_AMD64)
    CHECK_MEMBER_ALIGNMENT(VMCPU, vmm.s.CallRing3JmpBufR0, 16);
    CHECK_MEMBER_ALIGNMENT(VMCPU, vmm.s.CallRing3JmpBufR0.xmm6, 16);
#endif
    CHECK_MEMBER_ALIGNMENT(VM, vmm.s.u64LastYield, 8);
    CHECK_MEMBER_ALIGNMENT(VM, vmm.s.StatRunRC, 8);
    CHECK_MEMBER_ALIGNMENT(VM, StatTotalQemuToGC, 8);
#ifdef VBOX_WITH_REM
    CHECK_MEMBER_ALIGNMENT(VM, rem.s.uPendingExcptCR2, 8);
    CHECK_MEMBER_ALIGNMENT(VM, rem.s.StatsInQEMU, 8);
    CHECK_MEMBER_ALIGNMENT(VM, rem.s.Env, 64);
#endif

    /* the VMCPUs are page aligned TLB hit reasons. */
    CHECK_MEMBER_ALIGNMENT(VM, aCpus, 4096);
    CHECK_SIZE_ALIGNMENT(VMCPU, 4096);

    /* cpumctx */
    CHECK_MEMBER_ALIGNMENT(CPUMCTX, rax, 32);
    CHECK_MEMBER_ALIGNMENT(CPUMCTX, idtr.pIdt, 8);
    CHECK_MEMBER_ALIGNMENT(CPUMCTX, gdtr.pGdt, 8);
    CHECK_MEMBER_ALIGNMENT(CPUMCTX, SysEnter, 8);
    CHECK_MEMBER_ALIGNMENT(CPUMCTX, hwvirt, 8);
    CHECK_CPUMCTXCORE(rax);
    CHECK_CPUMCTXCORE(rcx);
    CHECK_CPUMCTXCORE(rdx);
    CHECK_CPUMCTXCORE(rbx);
    CHECK_CPUMCTXCORE(rsp);
    CHECK_CPUMCTXCORE(rbp);
    CHECK_CPUMCTXCORE(rsi);
    CHECK_CPUMCTXCORE(rdi);
    CHECK_CPUMCTXCORE(r8);
    CHECK_CPUMCTXCORE(r9);
    CHECK_CPUMCTXCORE(r10);
    CHECK_CPUMCTXCORE(r11);
    CHECK_CPUMCTXCORE(r12);
    CHECK_CPUMCTXCORE(r13);
    CHECK_CPUMCTXCORE(r14);
    CHECK_CPUMCTXCORE(r15);
    CHECK_CPUMCTXCORE(es);
    CHECK_CPUMCTXCORE(ss);
    CHECK_CPUMCTXCORE(cs);
    CHECK_CPUMCTXCORE(ds);
    CHECK_CPUMCTXCORE(fs);
    CHECK_CPUMCTXCORE(gs);
    CHECK_CPUMCTXCORE(rip);
    CHECK_CPUMCTXCORE(rflags);

#if HC_ARCH_BITS == 32
    /* CPUMHOSTCTX - lss pair */
    if (RT_OFFSETOF(CPUMHOSTCTX, esp) + 4 != RT_OFFSETOF(CPUMHOSTCTX, ss))
    {
        printf("error! CPUMHOSTCTX lss has been split up!\n");
        rc++;
    }
#endif
    CHECK_SIZE_ALIGNMENT(CPUMCTX, 64);
    CHECK_SIZE_ALIGNMENT(CPUMHOSTCTX, 64);
    CHECK_SIZE_ALIGNMENT(CPUMCTXMSRS, 64);

    /* pdm */
    PRINT_OFFSET(PDMDEVINS, Internal);
    PRINT_OFFSET(PDMDEVINS, achInstanceData);
    CHECK_MEMBER_ALIGNMENT(PDMDEVINS, achInstanceData, 64);
    CHECK_PADDING(PDMDEVINS, Internal, 1);

    PRINT_OFFSET(PDMUSBINS, Internal);
    PRINT_OFFSET(PDMUSBINS, achInstanceData);
    CHECK_MEMBER_ALIGNMENT(PDMUSBINS, achInstanceData, 32);
    CHECK_PADDING(PDMUSBINS, Internal, 1);

    PRINT_OFFSET(PDMDRVINS, Internal);
    PRINT_OFFSET(PDMDRVINS, achInstanceData);
    CHECK_MEMBER_ALIGNMENT(PDMDRVINS, achInstanceData, 32);
    CHECK_PADDING(PDMDRVINS, Internal, 1);

    CHECK_PADDING2(PDMCRITSECT);
    CHECK_PADDING2(PDMCRITSECTRW);

    /* pgm */
#if defined(VBOX_WITH_2X_4GB_ADDR_SPACE)  || defined(VBOX_WITH_RAW_MODE)
    CHECK_MEMBER_ALIGNMENT(PGMCPU, AutoSet, 8);
#endif
    CHECK_MEMBER_ALIGNMENT(PGMCPU, GCPhysCR3, sizeof(RTGCPHYS));
    CHECK_MEMBER_ALIGNMENT(PGMCPU, aGCPhysGstPaePDs, sizeof(RTGCPHYS));
    CHECK_MEMBER_ALIGNMENT(PGMCPU, DisState, 8);
    CHECK_MEMBER_ALIGNMENT(PGMCPU, cPoolAccessHandler, 8);
    CHECK_MEMBER_ALIGNMENT(PGMPOOLPAGE, idx, sizeof(uint16_t));
    CHECK_MEMBER_ALIGNMENT(PGMPOOLPAGE, pvPageR3, sizeof(RTHCPTR));
    CHECK_MEMBER_ALIGNMENT(PGMPOOLPAGE, GCPhys, sizeof(RTGCPHYS));
    CHECK_SIZE(PGMPAGE, 16);
    CHECK_MEMBER_ALIGNMENT(PGMRAMRANGE, aPages, 16);
    CHECK_MEMBER_ALIGNMENT(PGMREGMMIORANGE, RamRange, 16);

    /* rem */
    CHECK_MEMBER_ALIGNMENT(REM, aGCPtrInvalidatedPages, 8);
    CHECK_PADDING3(REMHANDLERNOTIFICATION, u.PhysicalRegister, u.padding);
    CHECK_PADDING3(REMHANDLERNOTIFICATION, u.PhysicalDeregister, u.padding);
    CHECK_PADDING3(REMHANDLERNOTIFICATION, u.PhysicalModify, u.padding);
    CHECK_SIZE_ALIGNMENT(REMHANDLERNOTIFICATION, 8);
    CHECK_MEMBER_ALIGNMENT(REMHANDLERNOTIFICATION, u.PhysicalDeregister.GCPhys, 8);

    /* TM */
    CHECK_MEMBER_ALIGNMENT(TM, TimerCritSect, sizeof(uintptr_t));
    CHECK_MEMBER_ALIGNMENT(TM, VirtualSyncLock, sizeof(uintptr_t));

    /* misc */
    CHECK_PADDING3(EMCPU, u.FatalLongJump, u.achPaddingFatalLongJump);
    CHECK_SIZE_ALIGNMENT(VMMR0JMPBUF, 8);
#ifdef VBOX_WITH_RAW_MODE
    CHECK_SIZE_ALIGNMENT(PATCHINFO, 8);
#endif
#if 0
    PRINT_OFFSET(VM, fForcedActions);
    PRINT_OFFSET(VM, StatQemuToGC);
    PRINT_OFFSET(VM, StatGCToQemu);
#endif

    CHECK_MEMBER_ALIGNMENT(IOM, CritSect, sizeof(uintptr_t));
#ifdef VBOX_WITH_REM
    CHECK_MEMBER_ALIGNMENT(EM, CritSectREM, sizeof(uintptr_t));
#endif
    CHECK_MEMBER_ALIGNMENT(PGM, CritSectX, sizeof(uintptr_t));
    CHECK_MEMBER_ALIGNMENT(PDM, CritSect, sizeof(uintptr_t));
    CHECK_MEMBER_ALIGNMENT(MMHYPERHEAP, Lock, sizeof(uintptr_t));

    /* hm - 32-bit gcc won't align uint64_t naturally, so check. */
    CHECK_MEMBER_ALIGNMENT(HM, uMaxAsid, 8);
    CHECK_MEMBER_ALIGNMENT(HM, vmx, 8);
    CHECK_MEMBER_ALIGNMENT(HM, vmx.Msrs, 8);
    CHECK_MEMBER_ALIGNMENT(HM, svm, 8);
    CHECK_MEMBER_ALIGNMENT(HM, PatchTree, 8);
    CHECK_MEMBER_ALIGNMENT(HM, aPatches, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, vmx, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, vmx.pfnStartVM, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, vmx.HCPhysVmcs, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, vmx.LastError, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, svm, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, svm.pfnVMRun, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, Event, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, Event.u64IntInfo, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, DisState, 8);
    CHECK_MEMBER_ALIGNMENT(HMCPU, StatEntry, 8);

    /* Make sure the set is large enough and has the correct size. */
    CHECK_SIZE(VMCPUSET, 32);
    if (sizeof(VMCPUSET) * 8 < VMM_MAX_CPU_COUNT)
    {
        printf("error! VMCPUSET is too small for VMM_MAX_CPU_COUNT=%u!\n", VMM_MAX_CPU_COUNT);
        rc++;
    }

    printf("info: struct UVM: %d bytes\n", (int)sizeof(UVM));

    CHECK_PADDING_UVM(32, vm);
    CHECK_PADDING_UVM(32, mm);
    CHECK_PADDING_UVM(32, pdm);
    CHECK_PADDING_UVM(32, stam);

    printf("info: struct UVMCPU: %d bytes\n", (int)sizeof(UVMCPU));
    CHECK_PADDING_UVMCPU(32, vm);

#ifdef VBOX_WITH_RAW_MODE
    /*
     * Compare HC and RC.
     */
    printf("tstVMStructSize: Comparing HC and RC...\n");
# include "tstVMStructRC.h"
#endif /* VBOX_WITH_RAW_MODE */

    CHECK_PADDING_GVM(4, gvmm);
    CHECK_PADDING_GVM(4, gmm);
    CHECK_PADDING_GVMCPU(4, gvmm);

    /*
     * Check that the optimized access macros for PGMPAGE works correctly.
     */
    PGMPAGE Page;
    PGM_PAGE_CLEAR(&Page);

    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_NONE);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == false);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == false);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == false);

    PGM_PAGE_SET_HNDL_PHYS_STATE(&Page, PGM_PAGE_HNDL_PHYS_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == true);

    PGM_PAGE_SET_HNDL_PHYS_STATE(&Page, PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == false);

    PGM_PAGE_CLEAR(&Page);
    PGM_PAGE_SET_HNDL_VIRT_STATE(&Page, PGM_PAGE_HNDL_VIRT_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_VIRT_STATE(&Page) == PGM_PAGE_HNDL_VIRT_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == true);

    PGM_PAGE_SET_HNDL_VIRT_STATE(&Page, PGM_PAGE_HNDL_VIRT_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_VIRT_STATE(&Page) == PGM_PAGE_HNDL_VIRT_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == false);


    PGM_PAGE_CLEAR(&Page);
    PGM_PAGE_SET_HNDL_PHYS_STATE(&Page, PGM_PAGE_HNDL_PHYS_STATE_ALL);
    PGM_PAGE_SET_HNDL_VIRT_STATE(&Page, PGM_PAGE_HNDL_VIRT_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_VIRT_STATE(&Page) == PGM_PAGE_HNDL_VIRT_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == true);

    PGM_PAGE_SET_HNDL_PHYS_STATE(&Page, PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    PGM_PAGE_SET_HNDL_VIRT_STATE(&Page, PGM_PAGE_HNDL_VIRT_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_VIRT_STATE(&Page) == PGM_PAGE_HNDL_VIRT_STATE_ALL);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == true);

    PGM_PAGE_SET_HNDL_PHYS_STATE(&Page, PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    PGM_PAGE_SET_HNDL_VIRT_STATE(&Page, PGM_PAGE_HNDL_VIRT_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_VIRT_STATE(&Page) == PGM_PAGE_HNDL_VIRT_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == false);

    PGM_PAGE_SET_HNDL_VIRT_STATE(&Page, PGM_PAGE_HNDL_VIRT_STATE_NONE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_PHYS_STATE(&Page) == PGM_PAGE_HNDL_PHYS_STATE_WRITE);
    CHECK_EXPR(PGM_PAGE_GET_HNDL_VIRT_STATE(&Page) == PGM_PAGE_HNDL_VIRT_STATE_NONE);
    CHECK_EXPR(PGM_PAGE_HAS_ANY_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_HANDLERS(&Page) == true);
    CHECK_EXPR(PGM_PAGE_HAS_ACTIVE_ALL_HANDLERS(&Page) == false);

#undef AssertFatal
#define AssertFatal(expr) do { } while (0)
#undef Assert
#define Assert(expr)      do { } while (0)

    PGM_PAGE_CLEAR(&Page);
    CHECK_EXPR(PGM_PAGE_GET_HCPHYS_NA(&Page) == 0);
    PGM_PAGE_SET_HCPHYS(NULL, &Page, UINT64_C(0x0000fffeff1ff000));
    CHECK_EXPR(PGM_PAGE_GET_HCPHYS_NA(&Page) == UINT64_C(0x0000fffeff1ff000));
    PGM_PAGE_SET_HCPHYS(NULL, &Page, UINT64_C(0x0000000000001000));
    CHECK_EXPR(PGM_PAGE_GET_HCPHYS_NA(&Page) == UINT64_C(0x0000000000001000));

    PGM_PAGE_INIT(&Page, UINT64_C(0x0000feedfacef000), UINT32_C(0x12345678), PGMPAGETYPE_RAM, PGM_PAGE_STATE_ALLOCATED);
    CHECK_EXPR(PGM_PAGE_GET_HCPHYS_NA(&Page) == UINT64_C(0x0000feedfacef000));
    CHECK_EXPR(PGM_PAGE_GET_PAGEID(&Page) == UINT32_C(0x12345678));
    CHECK_EXPR(PGM_PAGE_GET_TYPE_NA(&Page)   == PGMPAGETYPE_RAM);
    CHECK_EXPR(PGM_PAGE_GET_STATE_NA(&Page)  == PGM_PAGE_STATE_ALLOCATED);


    /*
     * Report result.
     */
    if (rc)
        printf("tstVMStructSize: FAILURE - %d errors\n", rc);
    else
        printf("tstVMStructSize: SUCCESS\n");
    return rc;
}

