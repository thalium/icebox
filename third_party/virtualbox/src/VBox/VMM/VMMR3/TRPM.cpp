/* $Id: TRPM.cpp $ */
/** @file
 * TRPM - The Trap Monitor.
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

/** @page pg_trpm   TRPM - The Trap Monitor
 *
 * The Trap Monitor (TRPM) is responsible for all trap and interrupt handling in
 * the VMM.  It plays a major role in raw-mode execution and a lesser one in the
 * hardware assisted mode.
 *
 * Note first, the following will use trap as a collective term for faults,
 * aborts and traps.
 *
 * @see grp_trpm
 *
 *
 * @section sec_trpm_rc     Raw-Mode Context
 *
 * When executing in the raw-mode context, TRPM will be managing the IDT and
 * processing all traps and interrupts.  It will also monitor the guest IDT
 * because CSAM wishes to know about changes to it (trap/interrupt/syscall
 * handler patching) and TRPM needs to keep the \#BP gate in sync (ring-3
 * considerations).  See TRPMR3SyncIDT and CSAMR3CheckGates.
 *
 * External interrupts will be forwarded to the host context by the quickest
 * possible route where they will be reasserted.  The other events will be
 * categorized into virtualization traps, genuine guest traps and hypervisor
 * traps.  The latter group may be recoverable depending on when they happen and
 * whether there is a handler for it, otherwise it will cause a guru meditation.
 *
 * TRPM distinguishes the between the first two (virt and guest traps) and the
 * latter (hyper) by checking the CPL of the trapping code, if CPL == 0 then
 * it's a hyper trap otherwise it's a virt/guest trap.  There are three trap
 * dispatcher tables, one ad-hoc for one time traps registered via
 * TRPMGCSetTempHandler(), one for hyper traps and one for virt/guest traps.
 * The latter two live in TRPMGCHandlersA.asm, the former in the VM structure.
 *
 * The raw-mode context trap handlers found in TRPMGCHandlers.cpp (for the most
 * part), will call up the other VMM sub-systems depending on what it things
 * happens.  The two most busy traps are page faults (\#PF) and general
 * protection fault/trap (\#GP).
 *
 * Before resuming guest code after having taken a virtualization trap or
 * injected a guest trap, TRPM will check for pending forced action and
 * every now and again let TM check for timed out timers.  This allows code that
 * is being executed as part of virtualization traps to signal ring-3 exits,
 * page table resyncs and similar without necessarily using the status code.  It
 * also make sure we're more responsive to timers and requests from other
 * threads (necessarily running on some different core/cpu in most cases).
 *
 *
 * @section sec_trpm_all    All Contexts
 *
 * TRPM will also dispatch / inject interrupts and traps to the guest, both when
 * in raw-mode and when in hardware assisted mode.  See TRPMInject().
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_TRPM
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/patm.h>
#include "TRPMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/em.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/hm.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/alloc.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Trap handler function.
 * @todo need to specialize this as we go along.
 */
typedef enum TRPMHANDLER
{
    /** Generic Interrupt handler. */
    TRPM_HANDLER_INT = 0,
    /** Generic Trap handler. */
    TRPM_HANDLER_TRAP,
    /** Trap 8 (\#DF) handler. */
    TRPM_HANDLER_TRAP_08,
    /** Trap 12 (\#MC) handler. */
    TRPM_HANDLER_TRAP_12,
    /** Max. */
    TRPM_HANDLER_MAX
} TRPMHANDLER, *PTRPMHANDLER;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Preinitialized IDT.
 * The u16OffsetLow is a value of the TRPMHANDLER enum which TRPMR3Relocate()
 * will use to pick the right address. The u16SegSel is always VMM CS.
 */
static VBOXIDTE_GENERIC     g_aIdt[256] =
{
/* special trap handler - still, this is an interrupt gate not a trap gate... */
#define IDTE_TRAP(enm)  { (unsigned)enm,                    0, 0, VBOX_IDTE_TYPE1, VBOX_IDTE_TYPE2_INT_32, 0, 1, 0 }
/* generic trap handler. */
#define IDTE_TRAP_GEN() IDTE_TRAP(TRPM_HANDLER_TRAP)
/* special interrupt handler. */
#define IDTE_INT(enm)   { (unsigned)enm,                    0, 0, VBOX_IDTE_TYPE1, VBOX_IDTE_TYPE2_INT_32,  0, 1, 0 }
/* generic interrupt handler. */
#define IDTE_INT_GEN()  IDTE_INT(TRPM_HANDLER_INT)
/* special task gate IDT entry (for critical exceptions like #DF). */
#define IDTE_TASK(enm)  { (unsigned)enm,                    0, 0, VBOX_IDTE_TYPE1, VBOX_IDTE_TYPE2_TASK,    0, 1, 0 }
/* draft, fixme later when the handler is written. */
#define IDTE_RESERVED() { 0,                                0, 0,               0,                      0,  0, 0, 0 }

                                        /* N  - M M -  T  - C - D i */
                                        /* o  - n o -  y  - o - e p */
                                        /*    - e n -  p  - d - s t */
                                        /*    -   i -  e  - e - c . */
                                        /*    -   c -     -   - r   */
                                        /* ============================================================= */
    IDTE_TRAP_GEN(),                    /*  0 - #DE - F   - N - Divide error */
    IDTE_TRAP_GEN(),                    /*  1 - #DB - F/T - N - Single step, INT 1 instruction */
#ifdef VBOX_WITH_NMI
    IDTE_TRAP_GEN(),                    /*  2 -     - I   - N - Non-Maskable Interrupt (NMI) */
#else
    IDTE_INT_GEN(),                     /*  2 -     - I   - N - Non-Maskable Interrupt (NMI) */
#endif
    IDTE_TRAP_GEN(),                    /*  3 - #BP - T   - N - Breakpoint, INT 3 instruction. */
    IDTE_TRAP_GEN(),                    /*  4 - #OF - T   - N - Overflow, INTO instruction. */
    IDTE_TRAP_GEN(),                    /*  5 - #BR - F   - N - BOUND Range Exceeded, BOUND instruction. */
    IDTE_TRAP_GEN(),                    /*  6 - #UD - F   - N - Undefined(/Invalid) Opcode. */
    IDTE_TRAP_GEN(),                    /*  7 - #NM - F   - N - Device not available, FP or (F)WAIT instruction. */
    IDTE_TASK(TRPM_HANDLER_TRAP_08),    /*  8 - #DF - A   - 0 - Double fault. */
    IDTE_TRAP_GEN(),                    /*  9 -     - F   - N - Coprocessor Segment Overrun (obsolete). */
    IDTE_TRAP_GEN(),                    /*  a - #TS - F   - Y - Invalid TSS, Taskswitch or TSS access. */
    IDTE_TRAP_GEN(),                    /*  b - #NP - F   - Y - Segment not present. */
    IDTE_TRAP_GEN(),                    /*  c - #SS - F   - Y - Stack-Segment fault. */
    IDTE_TRAP_GEN(),                    /*  d - #GP - F   - Y - General protection fault. */
    IDTE_TRAP_GEN(),                    /*  e - #PF - F   - Y - Page fault. - interrupt gate!!! */
    IDTE_RESERVED(),                    /*  f -     -     -   - Intel Reserved. Do not use. */
    IDTE_TRAP_GEN(),                    /* 10 - #MF - F   - N - x86 FPU Floating-Point Error (Math fault), FP or (F)WAIT instruction. */
    IDTE_TRAP_GEN(),                    /* 11 - #AC - F   - 0 - Alignment Check. */
    IDTE_TRAP(TRPM_HANDLER_TRAP_12),    /* 12 - #MC - A   - N - Machine Check. */
    IDTE_TRAP_GEN(),                    /* 13 - #XF - F   - N - SIMD Floating-Point Exception. */
    IDTE_RESERVED(),                    /* 14 -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 15 -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 16 -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 17 -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 18 -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 19 -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 1a -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 1b -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 1c -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 1d -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 1e -     -     -   - Intel Reserved. Do not use. */
    IDTE_RESERVED(),                    /* 1f -     -     -   - Intel Reserved. Do not use. */
    IDTE_INT_GEN(),                     /* 20 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 21 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 22 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 23 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 24 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 25 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 26 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 27 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 28 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 29 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 2a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 2b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 2c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 2d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 2e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 2f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 30 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 31 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 32 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 33 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 34 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 35 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 36 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 37 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 38 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 39 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 3a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 3b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 3c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 3d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 3e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 3f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 40 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 41 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 42 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 43 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 44 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 45 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 46 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 47 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 48 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 49 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 4a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 4b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 4c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 4d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 4e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 4f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 50 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 51 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 52 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 53 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 54 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 55 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 56 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 57 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 58 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 59 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 5a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 5b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 5c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 5d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 5e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 5f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 60 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 61 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 62 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 63 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 64 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 65 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 66 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 67 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 68 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 69 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 6a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 6b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 6c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 6d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 6e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 6f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 70 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 71 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 72 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 73 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 74 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 75 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 76 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 77 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 78 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 79 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 7a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 7b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 7c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 7d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 7e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 7f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 80 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 81 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 82 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 83 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 84 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 85 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 86 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 87 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 88 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 89 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 8a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 8b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 8c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 8d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 8e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 8f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 90 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 91 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 92 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 93 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 94 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 95 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 96 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 97 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 98 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 99 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 9a -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 9b -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 9c -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 9d -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 9e -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* 9f -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a0 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a1 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a2 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a3 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a4 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a5 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a6 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a7 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a8 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* a9 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* aa -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ab -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ac -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ad -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ae -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* af -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b0 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b1 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b2 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b3 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b4 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b5 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b6 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b7 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b8 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* b9 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ba -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* bb -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* bc -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* bd -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* be -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* bf -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c0 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c1 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c2 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c3 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c4 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c5 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c6 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c7 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c8 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* c9 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ca -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* cb -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* cc -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* cd -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ce -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* cf -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d0 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d1 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d2 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d3 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d4 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d5 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d6 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d7 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d8 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* d9 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* da -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* db -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* dc -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* dd -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* de -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* df -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e0 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e1 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e2 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e3 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e4 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e5 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e6 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e7 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e8 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* e9 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ea -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* eb -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ec -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ed -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ee -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ef -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f0 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f1 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f2 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f3 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f4 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f5 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f6 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f7 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f8 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* f9 -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* fa -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* fb -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* fc -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* fd -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* fe -     - I   -   - User defined Interrupts, external of INT n. */
    IDTE_INT_GEN(),                     /* ff -     - I   -   - User defined Interrupts, external of INT n. */
#undef IDTE_TRAP
#undef IDTE_TRAP_GEN
#undef IDTE_INT
#undef IDTE_INT_GEN
#undef IDTE_TASK
#undef IDTE_UNUSED
#undef IDTE_RESERVED
};


/** TRPM saved state version. */
#define TRPM_SAVED_STATE_VERSION        9
#define TRPM_SAVED_STATE_VERSION_UNI    8   /* SMP support bumped the version */


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int) trpmR3Save(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int) trpmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
static DECLCALLBACK(void) trpmR3InfoEvent(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);


/**
 * Initializes the Trap Manager
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) TRPMR3Init(PVM pVM)
{
    LogFlow(("TRPMR3Init\n"));
    int rc;

    /*
     * Assert sizes and alignments.
     */
    AssertRelease(!(RT_OFFSETOF(VM, trpm.s) & 31));
    AssertRelease(!(RT_OFFSETOF(VM, trpm.s.aIdt) & 15));
    AssertRelease(sizeof(pVM->trpm.s) <= sizeof(pVM->trpm.padding));
    AssertRelease(RT_ELEMENTS(pVM->trpm.s.aGuestTrapHandler) == sizeof(pVM->trpm.s.au32IdtPatched)*8);

    /*
     * Initialize members.
     */
    pVM->trpm.s.offVM              = RT_OFFSETOF(VM, trpm);
    pVM->trpm.s.offTRPMCPU         = RT_OFFSETOF(VM, aCpus[0].trpm) - RT_OFFSETOF(VM, trpm);

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        pVCpu->trpm.s.offVM         = RT_OFFSETOF(VM, aCpus[i].trpm);
        pVCpu->trpm.s.offVMCpu      = RT_OFFSETOF(VMCPU, trpm);
        pVCpu->trpm.s.uActiveVector = ~0U;
    }

    pVM->trpm.s.GuestIdtr.pIdt     = RTRCPTR_MAX;
    pVM->trpm.s.pvMonShwIdtRC      = RTRCPTR_MAX;
    pVM->trpm.s.fSafeToDropGuestIDTMonitoring = false;

    /*
     * Read the configuration (if any).
     */
    PCFGMNODE pTRPMNode = CFGMR3GetChild(CFGMR3GetRoot(pVM), "TRPM");
    if (pTRPMNode)
    {
        bool f;
        rc = CFGMR3QueryBool(pTRPMNode, "SafeToDropGuestIDTMonitoring", &f);
        if (RT_SUCCESS(rc))
            pVM->trpm.s.fSafeToDropGuestIDTMonitoring = f;
    }

    /* write config summary to log */
    if (pVM->trpm.s.fSafeToDropGuestIDTMonitoring)
        LogRel(("TRPM: Dropping Guest IDT Monitoring\n"));

    /*
     * Initialize the IDT.
     * The handler addresses will be set in the TRPMR3Relocate() function.
     */
    Assert(sizeof(pVM->trpm.s.aIdt) == sizeof(g_aIdt));
    memcpy(&pVM->trpm.s.aIdt[0], &g_aIdt[0], sizeof(pVM->trpm.s.aIdt));

    /*
     * Register virtual access handlers.
     */
    pVM->trpm.s.hShadowIdtWriteHandlerType = NIL_PGMVIRTHANDLERTYPE;
    pVM->trpm.s.hGuestIdtWriteHandlerType  = NIL_PGMVIRTHANDLERTYPE;
#ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
# ifdef TRPM_TRACK_SHADOW_IDT_CHANGES
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_HYPERVISOR, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, NULL /*pfnHandlerR3*/,
                                             NULL /*pszHandlerRC*/, "trpmRCShadowIDTWritePfHandler",
                                             "Shadow IDT write access handler", &pVM->trpm.s.hShadowIdtWriteHandlerType);
        AssertRCReturn(rc, rc);
# endif
        rc = PGMR3HandlerVirtualTypeRegister(pVM, PGMVIRTHANDLERKIND_WRITE, false /*fRelocUserRC*/,
                                             NULL /*pfnInvalidateR3*/, trpmGuestIDTWriteHandler,
                                             "trpmGuestIDTWriteHandler", "trpmRCGuestIDTWritePfHandler",
                                             "Guest IDT write access handler", &pVM->trpm.s.hGuestIdtWriteHandlerType);
        AssertRCReturn(rc, rc);
    }
#endif /* VBOX_WITH_RAW_MODE */

    /*
     * Register the saved state data unit.
     */
    rc = SSMR3RegisterInternal(pVM, "trpm", 1, TRPM_SAVED_STATE_VERSION, sizeof(TRPM),
                               NULL, NULL, NULL,
                               NULL, trpmR3Save, NULL,
                               NULL, trpmR3Load, NULL);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register info handlers.
     */
    rc = DBGFR3InfoRegisterInternalEx(pVM, "trpmevent", "Dumps TRPM pending event.", trpmR3InfoEvent,
                                      DBGFINFO_FLAGS_ALL_EMTS);
    AssertRCReturn(rc, rc);

    /*
     * Statistics.
     */
#ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
        STAM_REG(pVM, &pVM->trpm.s.StatRCWriteGuestIDTFault,    STAMTYPE_COUNTER, "/TRPM/RC/IDTWritesFault",    STAMUNIT_OCCURENCES,     "Guest IDT writes the we returned to R3 to handle.");
        STAM_REG(pVM, &pVM->trpm.s.StatRCWriteGuestIDTHandled,  STAMTYPE_COUNTER, "/TRPM/RC/IDTWritesHandled",  STAMUNIT_OCCURENCES,     "Guest IDT writes that we handled successfully.");
        STAM_REG(pVM, &pVM->trpm.s.StatSyncIDT,                 STAMTYPE_PROFILE, "/PROF/TRPM/SyncIDT",         STAMUNIT_TICKS_PER_CALL, "Profiling of TRPMR3SyncIDT().");

        /* traps */
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x00],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/00",          STAMUNIT_TICKS_PER_CALL, "#DE - Divide error.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x01],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/01",          STAMUNIT_TICKS_PER_CALL, "#DB - Debug (single step and more).");
        //STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x02],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/02",          STAMUNIT_TICKS_PER_CALL, "NMI");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x03],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/03",          STAMUNIT_TICKS_PER_CALL, "#BP - Breakpoint.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x04],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/04",          STAMUNIT_TICKS_PER_CALL, "#OF - Overflow.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x05],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/05",          STAMUNIT_TICKS_PER_CALL, "#BR - Bound range exceeded.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x06],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/06",          STAMUNIT_TICKS_PER_CALL, "#UD - Undefined opcode.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x07],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/07",          STAMUNIT_TICKS_PER_CALL, "#NM - Device not available (FPU).");
        //STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x08],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/08",          STAMUNIT_TICKS_PER_CALL, "#DF - Double fault.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x09],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/09",          STAMUNIT_TICKS_PER_CALL, "#?? - Coprocessor segment overrun (obsolete).");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x0a],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/0a",          STAMUNIT_TICKS_PER_CALL, "#TS - Task switch fault.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x0b],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/0b",          STAMUNIT_TICKS_PER_CALL, "#NP - Segment not present.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x0c],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/0c",          STAMUNIT_TICKS_PER_CALL, "#SS - Stack segment fault.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x0d],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/0d",          STAMUNIT_TICKS_PER_CALL, "#GP - General protection fault.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x0e],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/0e",          STAMUNIT_TICKS_PER_CALL, "#PF - Page fault.");
        //STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x0f],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/0f",          STAMUNIT_TICKS_PER_CALL, "Reserved.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x10],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/10",          STAMUNIT_TICKS_PER_CALL, "#MF - Math fault..");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x11],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/11",          STAMUNIT_TICKS_PER_CALL, "#AC - Alignment check.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x12],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/12",          STAMUNIT_TICKS_PER_CALL, "#MC - Machine check.");
        STAM_REG(pVM, &pVM->trpm.s.aStatGCTraps[0x13],      STAMTYPE_PROFILE_ADV, "/TRPM/GC/Traps/13",          STAMUNIT_TICKS_PER_CALL, "#XF - SIMD Floating-Point Exception.");
    }
#endif

# ifdef VBOX_WITH_STATISTICS
    rc = MMHyperAlloc(pVM, sizeof(STAMCOUNTER) * 256, sizeof(STAMCOUNTER), MM_TAG_TRPM, (void **)&pVM->trpm.s.paStatForwardedIRQR3);
    AssertRCReturn(rc, rc);
    pVM->trpm.s.paStatForwardedIRQRC = MMHyperR3ToRC(pVM, pVM->trpm.s.paStatForwardedIRQR3);
    for (unsigned i = 0; i < 256; i++)
        STAMR3RegisterF(pVM, &pVM->trpm.s.paStatForwardedIRQR3[i], STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES, "Forwarded interrupts.",
                        i < 0x20 ? "/TRPM/ForwardRaw/TRAP/%02X" : "/TRPM/ForwardRaw/IRQ/%02X", i);

#  ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
        rc = MMHyperAlloc(pVM, sizeof(STAMCOUNTER) * 256, sizeof(STAMCOUNTER), MM_TAG_TRPM, (void **)&pVM->trpm.s.paStatHostIrqR3);
        AssertRCReturn(rc, rc);
        pVM->trpm.s.paStatHostIrqRC = MMHyperR3ToRC(pVM, pVM->trpm.s.paStatHostIrqR3);
        for (unsigned i = 0; i < 256; i++)
            STAMR3RegisterF(pVM, &pVM->trpm.s.paStatHostIrqR3[i], STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,
                            "Host interrupts.", "/TRPM/HostIRQs/%02x", i);
    }
#  endif
# endif

#ifdef VBOX_WITH_RAW_MODE
    if (!HMIsEnabled(pVM))
    {
        STAM_REG(pVM, &pVM->trpm.s.StatForwardProfR3,       STAMTYPE_PROFILE_ADV, "/TRPM/ForwardRaw/ProfR3",   STAMUNIT_TICKS_PER_CALL, "Profiling TRPMForwardTrap.");
        STAM_REG(pVM, &pVM->trpm.s.StatForwardProfRZ,       STAMTYPE_PROFILE_ADV, "/TRPM/ForwardRaw/ProfRZ",   STAMUNIT_TICKS_PER_CALL, "Profiling TRPMForwardTrap.");
        STAM_REG(pVM, &pVM->trpm.s.StatForwardFailNoHandler,    STAMTYPE_COUNTER, "/TRPM/ForwardRaw/FailNoHandler", STAMUNIT_OCCURENCES,"Failure to forward interrupt in raw mode.");
        STAM_REG(pVM, &pVM->trpm.s.StatForwardFailPatchAddr,    STAMTYPE_COUNTER, "/TRPM/ForwardRaw/FailPatchAddr", STAMUNIT_OCCURENCES,"Failure to forward interrupt in raw mode.");
        STAM_REG(pVM, &pVM->trpm.s.StatForwardFailR3,           STAMTYPE_COUNTER, "/TRPM/ForwardRaw/FailR3",       STAMUNIT_OCCURENCES, "Failure to forward interrupt in raw mode.");
        STAM_REG(pVM, &pVM->trpm.s.StatForwardFailRZ,           STAMTYPE_COUNTER, "/TRPM/ForwardRaw/FailRZ",       STAMUNIT_OCCURENCES, "Failure to forward interrupt in raw mode.");

        STAM_REG(pVM, &pVM->trpm.s.StatTrap0dDisasm,            STAMTYPE_PROFILE, "/TRPM/RC/Traps/0d/Disasm",   STAMUNIT_TICKS_PER_CALL, "Profiling disassembly part of trpmGCTrap0dHandler.");
        STAM_REG(pVM, &pVM->trpm.s.StatTrap0dRdTsc,             STAMTYPE_COUNTER, "/TRPM/RC/Traps/0d/RdTsc",        STAMUNIT_OCCURENCES, "Number of RDTSC #GPs.");
    }
#endif

#ifdef VBOX_WITH_RAW_MODE
    /*
     * Default action when entering raw mode for the first time
     */
    if (!HMIsEnabled(pVM))
    {
        PVMCPU pVCpu = &pVM->aCpus[0];  /* raw mode implies on VCPU */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
    }
#endif
    return 0;
}


/**
 * Applies relocations to data and code managed by this component.
 *
 * This function will be called at init and whenever the VMM need
 * to relocate itself inside the GC.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    Relocation delta relative to old location.
 */
VMMR3DECL(void) TRPMR3Relocate(PVM pVM, RTGCINTPTR offDelta)
{
#ifdef VBOX_WITH_RAW_MODE
    if (HMIsEnabled(pVM))
        return;

    /* Only applies to raw mode which supports only 1 VCPU. */
    PVMCPU pVCpu = &pVM->aCpus[0];
    LogFlow(("TRPMR3Relocate\n"));

    /*
     * Get the trap handler addresses.
     *
     * If VMMRC.rc is screwed, so are we. We'll assert here since it elsewise
     * would make init order impossible if we should assert the presence of these
     * exports in TRPMR3Init().
     */
    RTRCPTR aRCPtrs[TRPM_HANDLER_MAX];
    RT_ZERO(aRCPtrs);
    int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "TRPMGCHandlerInterupt", &aRCPtrs[TRPM_HANDLER_INT]);
    AssertReleaseMsgRC(rc, ("Couldn't find TRPMGCHandlerInterupt in VMMRC.rc!\n"));

    rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "TRPMGCHandlerGeneric",  &aRCPtrs[TRPM_HANDLER_TRAP]);
    AssertReleaseMsgRC(rc, ("Couldn't find TRPMGCHandlerGeneric in VMMRC.rc!\n"));

    rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "TRPMGCHandlerTrap08",   &aRCPtrs[TRPM_HANDLER_TRAP_08]);
    AssertReleaseMsgRC(rc, ("Couldn't find TRPMGCHandlerTrap08 in VMMRC.rc!\n"));

    rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "TRPMGCHandlerTrap12",   &aRCPtrs[TRPM_HANDLER_TRAP_12]);
    AssertReleaseMsgRC(rc, ("Couldn't find TRPMGCHandlerTrap12 in VMMRC.rc!\n"));

    RTSEL SelCS = CPUMGetHyperCS(pVCpu);

    /*
     * Iterate the idt and set the addresses.
     */
    PVBOXIDTE pIdte = &pVM->trpm.s.aIdt[0];
    PVBOXIDTE_GENERIC pIdteTemplate = &g_aIdt[0];
    for (unsigned i = 0; i < RT_ELEMENTS(pVM->trpm.s.aIdt); i++, pIdte++, pIdteTemplate++)
    {
        if (    pIdte->Gen.u1Present
            &&  !ASMBitTest(&pVM->trpm.s.au32IdtPatched[0], i)
           )
        {
            Assert(pIdteTemplate->u16OffsetLow < TRPM_HANDLER_MAX);
            RTGCPTR Offset = aRCPtrs[pIdteTemplate->u16OffsetLow];
            switch (pIdteTemplate->u16OffsetLow)
            {
                /*
                 * Generic handlers have different entrypoints for each possible
                 * vector number. These entrypoints makes a sort of an array with
                 * 8 byte entries where the vector number is the index.
                 * See TRPMGCHandlersA.asm for details.
                 */
                case TRPM_HANDLER_INT:
                case TRPM_HANDLER_TRAP:
                    Offset += i * 8;
                    break;
                case TRPM_HANDLER_TRAP_12:
                    break;
                case TRPM_HANDLER_TRAP_08:
                    /* Handle #DF Task Gate in special way. */
                    pIdte->Gen.u16SegSel     = SELMGetTrap8Selector(pVM);
                    pIdte->Gen.u16OffsetLow  = 0;
                    pIdte->Gen.u16OffsetHigh = 0;
                    SELMSetTrap8EIP(pVM, Offset);
                    continue;
            }
            /* (non-task gates only ) */
            pIdte->Gen.u16OffsetLow  = Offset & 0xffff;
            pIdte->Gen.u16OffsetHigh = Offset >> 16;
            pIdte->Gen.u16SegSel     = SelCS;
        }
    }

    /*
     * Update IDTR (limit is including!).
     */
    CPUMSetHyperIDTR(pVCpu, VM_RC_ADDR(pVM, &pVM->trpm.s.aIdt[0]), sizeof(pVM->trpm.s.aIdt)-1);

# ifdef TRPM_TRACK_SHADOW_IDT_CHANGES
    if (pVM->trpm.s.pvMonShwIdtRC != RTRCPTR_MAX)
    {
        rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->trpm.s.pvMonShwIdtRC, true /*fHypervisor*/);
        AssertRC(rc);
    }
    pVM->trpm.s.pvMonShwIdtRC = VM_RC_ADDR(pVM, &pVM->trpm.s.aIdt[0]);
    rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->trpm.s.hShadowIdtWriteHandlerType,
                                     pVM->trpm.s.pvMonShwIdtRC, pVM->trpm.s.pvMonShwIdtRC + sizeof(pVM->trpm.s.aIdt) - 1,
                                     NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
    AssertRC(rc);
# endif

    /* Relocate IDT handlers for forwarding guest traps/interrupts. */
    for (uint32_t iTrap = 0; iTrap < RT_ELEMENTS(pVM->trpm.s.aGuestTrapHandler); iTrap++)
    {
        if (pVM->trpm.s.aGuestTrapHandler[iTrap] != TRPM_INVALID_HANDLER)
        {
            Log(("TRPMR3Relocate: iGate=%2X Handler %RRv -> %RRv\n", iTrap, pVM->trpm.s.aGuestTrapHandler[iTrap], pVM->trpm.s.aGuestTrapHandler[iTrap] + offDelta));
            pVM->trpm.s.aGuestTrapHandler[iTrap] += offDelta;
        }

        if (ASMBitTest(&pVM->trpm.s.au32IdtPatched[0], iTrap))
        {
            PVBOXIDTE   pIdteCur = &pVM->trpm.s.aIdt[iTrap];
            RTGCPTR     pHandler = VBOXIDTE_OFFSET(*pIdteCur);

            Log(("TRPMR3Relocate: *iGate=%2X Handler %RGv -> %RGv\n", iTrap, pHandler, pHandler + offDelta));
            pHandler += offDelta;

            pIdteCur->Gen.u16OffsetHigh = pHandler >> 16;
            pIdteCur->Gen.u16OffsetLow  = pHandler & 0xFFFF;
        }
    }

# ifdef VBOX_WITH_STATISTICS
    pVM->trpm.s.paStatForwardedIRQRC += offDelta;
    pVM->trpm.s.paStatHostIrqRC += offDelta;
# endif

#else  /* !VBOX_WITH_RAW_MODE */
    RT_NOREF(pVM, offDelta);
#endif /* !VBOX_WITH_RAW_MODE */
}


/**
 * Terminates the Trap Manager
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(int) TRPMR3Term(PVM pVM)
{
    NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * Resets a virtual CPU.
 *
 * Used by TRPMR3Reset and CPU hot plugging.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 */
VMMR3DECL(void) TRPMR3ResetCpu(PVMCPU pVCpu)
{
    pVCpu->trpm.s.uActiveVector = ~0U;
}


/**
 * The VM is being reset.
 *
 * For the TRPM component this means that any IDT write monitors
 * needs to be removed, any pending trap cleared, and the IDT reset.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3DECL(void) TRPMR3Reset(PVM pVM)
{
    /*
     * Deregister any virtual handlers.
     */
#ifdef TRPM_TRACK_GUEST_IDT_CHANGES
    if (pVM->trpm.s.GuestIdtr.pIdt != RTRCPTR_MAX)
    {
        if (!pVM->trpm.s.fSafeToDropGuestIDTMonitoring)
        {
            int rc = PGMHandlerVirtualDeregister(pVM, VMMGetCpu(pVM), pVM->trpm.s.GuestIdtr.pIdt, false /*fHypervisor*/);
            AssertRC(rc);
        }
        pVM->trpm.s.GuestIdtr.pIdt = RTRCPTR_MAX;
    }
    pVM->trpm.s.GuestIdtr.cbIdt = 0;
#endif

    /*
     * Reinitialize other members calling the relocator to get things right.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
        TRPMR3ResetCpu(&pVM->aCpus[i]);
    memcpy(&pVM->trpm.s.aIdt[0], &g_aIdt[0], sizeof(pVM->trpm.s.aIdt));
    memset(pVM->trpm.s.aGuestTrapHandler, 0, sizeof(pVM->trpm.s.aGuestTrapHandler));
    TRPMR3Relocate(pVM, 0);

#ifdef VBOX_WITH_RAW_MODE
    /*
     * Default action when entering raw mode for the first time
     */
    if (!HMIsEnabled(pVM))
    {
        PVMCPU pVCpu = &pVM->aCpus[0];  /* raw mode implies on VCPU */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
    }
#endif
}


# ifdef VBOX_WITH_RAW_MODE
/**
 * Resolve a builtin RC symbol.
 *
 * Called by PDM when loading or relocating RC modules.
 *
 * @returns VBox status
 * @param   pVM             The cross context VM structure.
 * @param   pszSymbol       Symbol to resolv
 * @param   pRCPtrValue     Where to store the symbol value.
 *
 * @remark  This has to work before VMMR3Relocate() is called.
 */
VMMR3_INT_DECL(int) TRPMR3GetImportRC(PVM pVM, const char *pszSymbol, PRTRCPTR pRCPtrValue)
{
    if (!strcmp(pszSymbol, "g_TRPM"))
        *pRCPtrValue = VM_RC_ADDR(pVM, &pVM->trpm);
    else if (!strcmp(pszSymbol, "g_TRPMCPU"))
        *pRCPtrValue = VM_RC_ADDR(pVM, &pVM->aCpus[0].trpm);
    else if (!strcmp(pszSymbol, "g_trpmGuestCtx"))
    {
        PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(VMMGetCpuById(pVM, 0));
        *pRCPtrValue = VM_RC_ADDR(pVM, pCtx);
    }
    else if (!strcmp(pszSymbol, "g_trpmHyperCtx"))
    {
        PCPUMCTX pCtx = CPUMGetHyperCtxPtr(VMMGetCpuById(pVM, 0));
        *pRCPtrValue = VM_RC_ADDR(pVM, pCtx);
    }
    else if (!strcmp(pszSymbol, "g_trpmGuestCtxCore"))
    {
        PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(VMMGetCpuById(pVM, 0));
        *pRCPtrValue = VM_RC_ADDR(pVM, CPUMCTX2CORE(pCtx));
    }
    else if (!strcmp(pszSymbol, "g_trpmHyperCtxCore"))
    {
        PCPUMCTX pCtx = CPUMGetHyperCtxPtr(VMMGetCpuById(pVM, 0));
        *pRCPtrValue = VM_RC_ADDR(pVM, CPUMCTX2CORE(pCtx));
    }
    else
        return VERR_SYMBOL_NOT_FOUND;
    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) trpmR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    PTRPM pTrpm = &pVM->trpm.s;
    LogFlow(("trpmR3Save:\n"));

    /*
     * Active and saved traps.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PTRPMCPU pTrpmCpu = &pVM->aCpus[i].trpm.s;
        SSMR3PutUInt(pSSM,      pTrpmCpu->uActiveVector);
        SSMR3PutUInt(pSSM,      pTrpmCpu->enmActiveType);
        SSMR3PutGCUInt(pSSM,    pTrpmCpu->uActiveErrorCode);
        SSMR3PutGCUIntPtr(pSSM, pTrpmCpu->uActiveCR2);
        SSMR3PutGCUInt(pSSM,    pTrpmCpu->uSavedVector);
        SSMR3PutUInt(pSSM,      pTrpmCpu->enmSavedType);
        SSMR3PutGCUInt(pSSM,    pTrpmCpu->uSavedErrorCode);
        SSMR3PutGCUIntPtr(pSSM, pTrpmCpu->uSavedCR2);
        SSMR3PutGCUInt(pSSM,    pTrpmCpu->uPrevVector);
    }
    SSMR3PutBool(pSSM,      HMIsEnabled(pVM));
    PVMCPU pVCpu0 = &pVM->aCpus[0]; NOREF(pVCpu0); /* raw mode implies 1 VCPU */
    SSMR3PutUInt(pSSM,      VM_WHEN_RAW_MODE(VMCPU_FF_IS_SET(pVCpu0, VMCPU_FF_TRPM_SYNC_IDT), 0));
    SSMR3PutMem(pSSM,       &pTrpm->au32IdtPatched[0], sizeof(pTrpm->au32IdtPatched));
    SSMR3PutU32(pSSM, UINT32_MAX);          /* separator. */

    /*
     * Save any trampoline gates.
     */
    for (uint32_t iTrap = 0; iTrap < RT_ELEMENTS(pTrpm->aGuestTrapHandler); iTrap++)
    {
        if (pTrpm->aGuestTrapHandler[iTrap])
        {
            SSMR3PutU32(pSSM, iTrap);
            SSMR3PutGCPtr(pSSM, pTrpm->aGuestTrapHandler[iTrap]);
            SSMR3PutMem(pSSM, &pTrpm->aIdt[iTrap], sizeof(pTrpm->aIdt[iTrap]));
        }
    }

    return SSMR3PutU32(pSSM, UINT32_MAX);   /* terminator */
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
static DECLCALLBACK(int) trpmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    LogFlow(("trpmR3Load:\n"));
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Validate version.
     */
    if (    uVersion != TRPM_SAVED_STATE_VERSION
        &&  uVersion != TRPM_SAVED_STATE_VERSION_UNI)
    {
        AssertMsgFailed(("trpmR3Load: Invalid version uVersion=%d!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    /*
     * Call the reset function to kick out any handled gates and other potential trouble.
     */
    TRPMR3Reset(pVM);

    /*
     * Active and saved traps.
     */
    PTRPM pTrpm = &pVM->trpm.s;

    if (uVersion == TRPM_SAVED_STATE_VERSION)
    {
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PTRPMCPU pTrpmCpu = &pVM->aCpus[i].trpm.s;
            SSMR3GetUInt(pSSM,      &pTrpmCpu->uActiveVector);
            SSMR3GetUInt(pSSM,      (uint32_t *)&pTrpmCpu->enmActiveType);
            SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uActiveErrorCode);
            SSMR3GetGCUIntPtr(pSSM, &pTrpmCpu->uActiveCR2);
            SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uSavedVector);
            SSMR3GetUInt(pSSM,      (uint32_t *)&pTrpmCpu->enmSavedType);
            SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uSavedErrorCode);
            SSMR3GetGCUIntPtr(pSSM, &pTrpmCpu->uSavedCR2);
            SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uPrevVector);
        }

        bool fIgnored;
        SSMR3GetBool(pSSM, &fIgnored);
    }
    else
    {
        PTRPMCPU pTrpmCpu = &pVM->aCpus[0].trpm.s;
        SSMR3GetUInt(pSSM,      &pTrpmCpu->uActiveVector);
        SSMR3GetUInt(pSSM,      (uint32_t *)&pTrpmCpu->enmActiveType);
        SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uActiveErrorCode);
        SSMR3GetGCUIntPtr(pSSM, &pTrpmCpu->uActiveCR2);
        SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uSavedVector);
        SSMR3GetUInt(pSSM,      (uint32_t *)&pTrpmCpu->enmSavedType);
        SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uSavedErrorCode);
        SSMR3GetGCUIntPtr(pSSM, &pTrpmCpu->uSavedCR2);
        SSMR3GetGCUInt(pSSM,    &pTrpmCpu->uPrevVector);

        RTGCUINT fIgnored;
        SSMR3GetGCUInt(pSSM,    &fIgnored);
    }

    RTUINT fSyncIDT;
    int rc = SSMR3GetUInt(pSSM, &fSyncIDT);
    if (RT_FAILURE(rc))
        return rc;
    if (fSyncIDT & ~1)
    {
        AssertMsgFailed(("fSyncIDT=%#x\n", fSyncIDT));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }
#ifdef VBOX_WITH_RAW_MODE
    if (fSyncIDT)
    {
        PVMCPU pVCpu = &pVM->aCpus[0];  /* raw mode implies 1 VCPU */
        VMCPU_FF_SET(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
    }
    /* else: cleared by reset call above. */
#endif

    SSMR3GetMem(pSSM, &pTrpm->au32IdtPatched[0], sizeof(pTrpm->au32IdtPatched));

    /* check the separator */
    uint32_t u32Sep;
    rc = SSMR3GetU32(pSSM, &u32Sep);
    if (RT_FAILURE(rc))
        return rc;
    if (u32Sep != (uint32_t)~0)
    {
        AssertMsgFailed(("u32Sep=%#x (first)\n", u32Sep));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }

    /*
     * Restore any trampoline gates.
     */
    for (;;)
    {
        /* gate number / terminator */
        uint32_t iTrap;
        rc = SSMR3GetU32(pSSM, &iTrap);
        if (RT_FAILURE(rc))
            return rc;
        if (iTrap == (uint32_t)~0)
            break;
        if (    iTrap >= RT_ELEMENTS(pTrpm->aIdt)
            ||  pTrpm->aGuestTrapHandler[iTrap])
        {
            AssertMsgFailed(("iTrap=%#x\n", iTrap));
            return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
        }

        /* restore the IDT entry. */
        RTGCPTR GCPtrHandler;
        SSMR3GetGCPtr(pSSM, &GCPtrHandler);
        VBOXIDTE Idte;
        rc = SSMR3GetMem(pSSM, &Idte, sizeof(Idte));
        if (RT_FAILURE(rc))
            return rc;
        Assert(GCPtrHandler);
        pTrpm->aIdt[iTrap] = Idte;
    }

    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_RAW_MODE

/**
 * Check if gate handlers were updated
 * (callback for the VMCPU_FF_TRPM_SYNC_IDT forced action).
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3DECL(int) TRPMR3SyncIDT(PVM pVM, PVMCPU pVCpu)
{
    STAM_PROFILE_START(&pVM->trpm.s.StatSyncIDT, a);
    const bool  fRawRing0 = EMIsRawRing0Enabled(pVM);
    int         rc;

    AssertReturn(!HMIsEnabled(pVM), VERR_TRPM_HM_IPE);

    if (fRawRing0 && CSAMIsEnabled(pVM))
    {
        /* Clear all handlers */
        Log(("TRPMR3SyncIDT: Clear all trap handlers.\n"));
        /** @todo inefficient, but simple */
        for (unsigned iGate = 0; iGate < 256; iGate++)
            trpmClearGuestTrapHandler(pVM, iGate);

        /* Scan them all (only the first time) */
        CSAMR3CheckGates(pVM, 0, 256);
    }

    /*
     * Get the IDTR.
     */
    VBOXIDTR IDTR;
    IDTR.pIdt = CPUMGetGuestIDTR(pVCpu, &IDTR.cbIdt);
    if (!IDTR.cbIdt)
    {
        Log(("No IDT entries...\n"));
        return DBGFSTOP(pVM);
    }

# ifdef TRPM_TRACK_GUEST_IDT_CHANGES
    /*
     * Check if Guest's IDTR has changed.
     */
    if (    IDTR.pIdt  != pVM->trpm.s.GuestIdtr.pIdt
        ||  IDTR.cbIdt != pVM->trpm.s.GuestIdtr.cbIdt)
    {
        Log(("TRPMR3UpdateFromCPUM: Guest's IDT is changed to pIdt=%08X cbIdt=%08X\n", IDTR.pIdt, IDTR.cbIdt));
        if (!pVM->trpm.s.fSafeToDropGuestIDTMonitoring)
        {
            /*
             * [Re]Register write virtual handler for guest's IDT.
             */
            if (pVM->trpm.s.GuestIdtr.pIdt != RTRCPTR_MAX)
            {
                rc = PGMHandlerVirtualDeregister(pVM, pVCpu, pVM->trpm.s.GuestIdtr.pIdt, false /*fHypervisor*/);
                AssertRCReturn(rc, rc);
            }
            /* limit is including */
            rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->trpm.s.hGuestIdtWriteHandlerType,
                                             IDTR.pIdt, IDTR.pIdt + IDTR.cbIdt /* already inclusive */,
                                             NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);

            if (rc == VERR_PGM_HANDLER_VIRTUAL_CONFLICT)
            {
                /* Could be a conflict with CSAM */
                CSAMR3RemovePage(pVM, IDTR.pIdt);
                if (PAGE_ADDRESS(IDTR.pIdt) != PAGE_ADDRESS(IDTR.pIdt + IDTR.cbIdt))
                    CSAMR3RemovePage(pVM, IDTR.pIdt + IDTR.cbIdt);

                rc = PGMR3HandlerVirtualRegister(pVM, pVCpu, pVM->trpm.s.hGuestIdtWriteHandlerType,
                                                 IDTR.pIdt, IDTR.pIdt + IDTR.cbIdt /* already inclusive */,
                                                 NULL /*pvUserR3*/, NIL_RTR0PTR /*pvUserRC*/, NULL /*pszDesc*/);
            }

            AssertRCReturn(rc, rc);
        }

        /* Update saved Guest IDTR. */
        pVM->trpm.s.GuestIdtr = IDTR;
    }
# endif

    /*
     * Sync the interrupt gate.
     * Should probably check/sync the others too, but for now we'll handle that in #GP.
     */
    X86DESC  Idte3;
    rc = PGMPhysSimpleReadGCPtr(pVCpu, &Idte3, IDTR.pIdt + sizeof(Idte3) * 3,  sizeof(Idte3));
    if (RT_FAILURE(rc))
    {
        AssertMsgRC(rc, ("Failed to read IDT[3]! rc=%Rrc\n", rc));
        return DBGFSTOP(pVM);
    }
    AssertRCReturn(rc, rc);
    if (fRawRing0)
        pVM->trpm.s.aIdt[3].Gen.u2DPL = RT_MAX(Idte3.Gen.u2Dpl, 1);
    else
        pVM->trpm.s.aIdt[3].Gen.u2DPL = Idte3.Gen.u2Dpl;

    /*
     * Clear the FF and we're done.
     */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TRPM_SYNC_IDT);
    STAM_PROFILE_STOP(&pVM->trpm.s.StatSyncIDT, a);
    return VINF_SUCCESS;
}


/**
 * Clear passthrough interrupt gate handler (reset to default handler)
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   iTrap       Trap/interrupt gate number.
 */
int trpmR3ClearPassThroughHandler(PVM pVM, unsigned iTrap)
{
    /* Only applies to raw mode which supports only 1 VCPU. */
    PVMCPU pVCpu = &pVM->aCpus[0];
    Assert(!HMIsEnabled(pVM));

    /** @todo cleanup trpmR3ClearPassThroughHandler()! */
    RTRCPTR aGCPtrs[TRPM_HANDLER_MAX];
    int rc;

    memset(aGCPtrs, 0, sizeof(aGCPtrs));

    rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "TRPMGCHandlerInterupt", &aGCPtrs[TRPM_HANDLER_INT]);
    AssertReleaseMsgRC(rc, ("Couldn't find TRPMGCHandlerInterupt in VMMRC.rc!\n"));

    if (    iTrap < TRPM_HANDLER_INT_BASE
        ||  iTrap >= RT_ELEMENTS(pVM->trpm.s.aIdt))
    {
        AssertMsg(iTrap < TRPM_HANDLER_INT_BASE, ("Illegal gate number %#x!\n", iTrap));
        return VERR_INVALID_PARAMETER;
    }
    memcpy(&pVM->trpm.s.aIdt[iTrap], &g_aIdt[iTrap], sizeof(pVM->trpm.s.aIdt[0]));

    /* Unmark it for relocation purposes. */
    ASMBitClear(&pVM->trpm.s.au32IdtPatched[0], iTrap);

    RTSEL               SelCS         = CPUMGetHyperCS(pVCpu);
    PVBOXIDTE           pIdte         = &pVM->trpm.s.aIdt[iTrap];
    PVBOXIDTE_GENERIC   pIdteTemplate = &g_aIdt[iTrap];
    if (pIdte->Gen.u1Present)
    {
        Assert(pIdteTemplate->u16OffsetLow == TRPM_HANDLER_INT);
        Assert(sizeof(RTRCPTR) == sizeof(aGCPtrs[0]));
        RTRCPTR Offset = (RTRCPTR)aGCPtrs[pIdteTemplate->u16OffsetLow];

        /*
         * Generic handlers have different entrypoints for each possible
         * vector number. These entrypoints make a sort of an array with
         * 8 byte entries where the vector number is the index.
         * See TRPMGCHandlersA.asm for details.
         */
        Offset += iTrap * 8;

        if (pIdte->Gen.u5Type2 != VBOX_IDTE_TYPE2_TASK)
        {
            pIdte->Gen.u16OffsetLow  = Offset & 0xffff;
            pIdte->Gen.u16OffsetHigh = Offset >> 16;
            pIdte->Gen.u16SegSel     = SelCS;
        }
    }

    return VINF_SUCCESS;
}


/**
 * Check if address is a gate handler (interrupt or trap).
 *
 * @returns gate nr or UINT32_MAX is not found
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       GC address to check.
 */
VMMR3DECL(uint32_t) TRPMR3QueryGateByHandler(PVM pVM, RTRCPTR GCPtr)
{
    AssertReturn(!HMIsEnabled(pVM), ~0U);

    for (uint32_t iTrap = 0; iTrap < RT_ELEMENTS(pVM->trpm.s.aGuestTrapHandler); iTrap++)
    {
        if (pVM->trpm.s.aGuestTrapHandler[iTrap] == GCPtr)
            return iTrap;

        /* redundant */
        if (ASMBitTest(&pVM->trpm.s.au32IdtPatched[0], iTrap))
        {
            PVBOXIDTE   pIdte = &pVM->trpm.s.aIdt[iTrap];
            RTGCPTR     pHandler = VBOXIDTE_OFFSET(*pIdte);

            if (pHandler == GCPtr)
                return iTrap;
        }
    }
    return UINT32_MAX;
}


/**
 * Get guest trap/interrupt gate handler
 *
 * @returns Guest trap handler address or TRPM_INVALID_HANDLER if none installed
 * @param   pVM         The cross context VM structure.
 * @param   iTrap       Interrupt/trap number.
 */
VMMR3DECL(RTRCPTR) TRPMR3GetGuestTrapHandler(PVM pVM, unsigned iTrap)
{
    AssertReturn(iTrap < RT_ELEMENTS(pVM->trpm.s.aIdt), TRPM_INVALID_HANDLER);
    AssertReturn(!HMIsEnabled(pVM), TRPM_INVALID_HANDLER);

    return pVM->trpm.s.aGuestTrapHandler[iTrap];
}


/**
 * Set guest trap/interrupt gate handler
 * Used for setting up trap gates used for kernel calls.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   iTrap       Interrupt/trap number.
 * @param   pHandler    GC handler pointer
 */
VMMR3DECL(int) TRPMR3SetGuestTrapHandler(PVM pVM, unsigned iTrap, RTRCPTR pHandler)
{
    /* Only valid in raw mode which implies 1 VCPU */
    Assert(PATMIsEnabled(pVM) && pVM->cCpus == 1);
    AssertReturn(!HMIsEnabled(pVM), VERR_TRPM_HM_IPE);
    PVMCPU pVCpu = &pVM->aCpus[0];

    /*
     * Validate.
     */
    if (iTrap >= RT_ELEMENTS(pVM->trpm.s.aIdt))
    {
        AssertMsg(iTrap < TRPM_HANDLER_INT_BASE, ("Illegal gate number %d!\n", iTrap));
        return VERR_INVALID_PARAMETER;
    }

    AssertReturn(pHandler == TRPM_INVALID_HANDLER || PATMIsPatchGCAddr(pVM, pHandler), VERR_INVALID_PARAMETER);

    uint16_t    cbIDT;
    RTGCPTR     GCPtrIDT = CPUMGetGuestIDTR(pVCpu, &cbIDT);
    if (iTrap * sizeof(VBOXIDTE) >= cbIDT)
        return VERR_INVALID_PARAMETER;  /* Silently ignore out of range requests. */

    if (pHandler == TRPM_INVALID_HANDLER)
    {
        /* clear trap handler */
        Log(("TRPMR3SetGuestTrapHandler: clear handler %x\n", iTrap));
        return trpmClearGuestTrapHandler(pVM, iTrap);
    }

    /*
     * Read the guest IDT entry.
     */
    VBOXIDTE GuestIdte;
    int rc = PGMPhysSimpleReadGCPtr(pVCpu, &GuestIdte, GCPtrIDT + iTrap * sizeof(GuestIdte),  sizeof(GuestIdte));
    if (RT_FAILURE(rc))
    {
        AssertMsgRC(rc, ("Failed to read IDTE! rc=%Rrc\n", rc));
        return rc;
    }

    if (    EMIsRawRing0Enabled(pVM)
        && !EMIsRawRing1Enabled(pVM))   /* can't deal with the ambiguity of ring 1 & 2 in the patch code. */
    {
        /*
         * Only replace handlers for which we are 100% certain there won't be
         * any host interrupts.
         *
         * 0x2E is safe on Windows because it's the system service interrupt gate. Not
         * quite certain if this is safe or not on 64-bit Vista, it probably is.
         *
         * 0x80 is safe on Linux because it's the syscall vector and is part of the
         * 32-bit usermode ABI. 64-bit Linux (usually) supports 32-bit processes
         * and will therefor never assign hardware interrupts to 0x80.
         *
         * Exactly why 0x80 is safe on 32-bit Windows is a bit hazy, but it seems
         * to work ok... However on 64-bit Vista (SMP?) is doesn't work reliably.
         * Booting Linux/BSD guest will cause system lockups on most of the computers.
         * -> Update: It seems gate 0x80 is not safe on 32-bits Windows either. See
         *            @bugref{3604}.
         *
         * PORTME - Check if your host keeps any of these gates free from hw ints.
         *
         * Note! SELMR3SyncTSS also has code related to this interrupt handler replacing.
         */
        /** @todo handle those dependencies better! */
        /** @todo Solve this in a proper manner. see @bugref{1186} */
#if defined(RT_OS_WINDOWS) && defined(RT_ARCH_X86)
        if (iTrap == 0x2E)
#elif defined(RT_OS_LINUX)
        if (iTrap == 0x80)
#else
        if (0)
#endif
        {
            if (     GuestIdte.Gen.u1Present
                &&  (   GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_TRAP_32
                     || GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_INT_32)
                &&   GuestIdte.Gen.u2DPL == 3)
            {
                PVBOXIDTE   pIdte = &pVM->trpm.s.aIdt[iTrap];

                GuestIdte.Gen.u5Type2 = VBOX_IDTE_TYPE2_TRAP_32;
                GuestIdte.Gen.u16OffsetHigh = pHandler >> 16;
                GuestIdte.Gen.u16OffsetLow  = pHandler & 0xFFFF;
                GuestIdte.Gen.u16SegSel |= 1;  //ring 1
                *pIdte = GuestIdte;

                /* Mark it for relocation purposes. */
                ASMBitSet(&pVM->trpm.s.au32IdtPatched[0], iTrap);

                /* Also store it in our guest trap array. */
                pVM->trpm.s.aGuestTrapHandler[iTrap] = pHandler;

                Log(("Setting trap handler %x to %08X (direct)\n", iTrap, pHandler));
                return VINF_SUCCESS;
            }
            /* ok, let's try to install a trampoline handler then. */
        }
    }

    if (     GuestIdte.Gen.u1Present
        &&  (   GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_TRAP_32
             || GuestIdte.Gen.u5Type2 == VBOX_IDTE_TYPE2_INT_32)
        &&  (GuestIdte.Gen.u2DPL == 3 || GuestIdte.Gen.u2DPL == 0))
    {
        /*
         * Save handler which can be used for a trampoline call inside the GC
         */
        Log(("Setting trap handler %x to %08X\n", iTrap, pHandler));
        pVM->trpm.s.aGuestTrapHandler[iTrap] = pHandler;
        return VINF_SUCCESS;
    }
    return VERR_INVALID_PARAMETER;
}


/**
 * Check if address is a gate handler (interrupt/trap/task/anything).
 *
 * @returns True is gate handler, false if not.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPtr       GC address to check.
 */
VMMR3DECL(bool) TRPMR3IsGateHandler(PVM pVM, RTRCPTR GCPtr)
{
    /* Only valid in raw mode which implies 1 VCPU */
    Assert(PATMIsEnabled(pVM) && pVM->cCpus == 1);
    PVMCPU pVCpu = &pVM->aCpus[0];

    /*
     * Read IDTR and calc last entry.
     */
    uint16_t    cbIDT;
    RTGCPTR     GCPtrIDTE = CPUMGetGuestIDTR(pVCpu, &cbIDT);
    unsigned    cEntries = (cbIDT + 1) / sizeof(VBOXIDTE);
    if (!cEntries)
        return false;
    RTGCPTR   GCPtrIDTELast = GCPtrIDTE + (cEntries - 1) * sizeof(VBOXIDTE);

    /*
     * Outer loop: iterate pages.
     */
    while (GCPtrIDTE <= GCPtrIDTELast)
    {
        /*
         * Convert this page to a HC address.
         * (This function checks for not-present pages.)
         */
        PCVBOXIDTE      pIDTE;
        PGMPAGEMAPLOCK  Lock;
        int rc = PGMPhysGCPtr2CCPtrReadOnly(pVCpu, GCPtrIDTE, (const void **)&pIDTE, &Lock);
        if (RT_SUCCESS(rc))
        {
            /*
             * Inner Loop: Iterate the data on this page looking for an entry equal to GCPtr.
             * N.B. Member of the Flat Earth Society...
             */
            while (GCPtrIDTE <= GCPtrIDTELast)
            {
                if (pIDTE->Gen.u1Present)
                {
                    RTRCPTR GCPtrHandler = VBOXIDTE_OFFSET(*pIDTE);
                    if (GCPtr == GCPtrHandler)
                    {
                        PGMPhysReleasePageMappingLock(pVM, &Lock);
                        return true;
                    }
                }

                /* next entry */
                if ((GCPtrIDTE & PAGE_OFFSET_MASK) + sizeof(VBOXIDTE) >= PAGE_SIZE)
                {
                    AssertMsg(!(GCPtrIDTE & (sizeof(VBOXIDTE) - 1)),
                              ("IDT is crossing pages and it's not aligned! GCPtrIDTE=%#x cbIDT=%#x\n", GCPtrIDTE, cbIDT));
                    GCPtrIDTE += sizeof(VBOXIDTE);
                    break;
                }
                GCPtrIDTE += sizeof(VBOXIDTE);
                pIDTE++;
            }
            PGMPhysReleasePageMappingLock(pVM, &Lock);
        }
        else
        {
            /* Skip to the next page (if any). Take care not to wrap around the address space. */
            if ((GCPtrIDTELast >> PAGE_SHIFT) == (GCPtrIDTE >> PAGE_SHIFT))
                return false;
            GCPtrIDTE = RT_ALIGN_T(GCPtrIDTE, PAGE_SIZE, RTGCPTR) + PAGE_SIZE + (GCPtrIDTE & (sizeof(VBOXIDTE) - 1));
        }
    }
    return false;
}

#endif /* VBOX_WITH_RAW_MODE */

/**
 * Inject event (such as external irq or trap)
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   enmEvent    Trpm event type
 */
VMMR3DECL(int) TRPMR3InjectEvent(PVM pVM, PVMCPU pVCpu, TRPMEVENT enmEvent)
{
#ifdef VBOX_WITH_RAW_MODE
    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    Assert(!PATMIsPatchGCAddr(pVM, pCtx->eip));
#endif
    Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS));

    /* Currently only useful for external hardware interrupts. */
    Assert(enmEvent == TRPM_HARDWARE_INT);

#if defined(TRPM_FORWARD_TRAPS_IN_GC) && !defined(IEM_VERIFICATION_MODE)

# ifdef LOG_ENABLED
    DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", "TRPMInject");
    DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, "TRPMInject");
# endif

    uint8_t u8Interrupt = 0;
    int rc = PDMGetInterrupt(pVCpu, &u8Interrupt);
    Log(("TRPMR3InjectEvent: CPU%d u8Interrupt=%d (%#x) rc=%Rrc\n", pVCpu->idCpu, u8Interrupt, u8Interrupt, rc));
    if (RT_SUCCESS(rc))
    {
        if (HMIsEnabled(pVM) || EMIsSupervisorCodeRecompiled(pVM))
        {
            rc = TRPMAssertTrap(pVCpu, u8Interrupt, enmEvent);
            AssertRC(rc);
            STAM_COUNTER_INC(&pVM->trpm.s.paStatForwardedIRQR3[u8Interrupt]);
            return HMR3IsActive(pVCpu) ? VINF_EM_RESCHEDULE_HM : VINF_EM_RESCHEDULE_REM;
        }
        /* If the guest gate is not patched, then we will check (again) if we can patch it. */
        if (pVM->trpm.s.aGuestTrapHandler[u8Interrupt] == TRPM_INVALID_HANDLER)
        {
            CSAMR3CheckGates(pVM, u8Interrupt, 1);
            Log(("TRPMR3InjectEvent: recheck gate %x -> valid=%d\n", u8Interrupt, TRPMR3GetGuestTrapHandler(pVM, u8Interrupt) != TRPM_INVALID_HANDLER));
        }

        if (pVM->trpm.s.aGuestTrapHandler[u8Interrupt] != TRPM_INVALID_HANDLER)
        {
            /* Must check pending forced actions as our IDT or GDT might be out of sync */
            rc = EMR3CheckRawForcedActions(pVM, pVCpu);
            if (rc == VINF_SUCCESS)
            {
                /* There's a handler -> let's execute it in raw mode */
                rc = TRPMForwardTrap(pVCpu, CPUMCTX2CORE(pCtx), u8Interrupt, 0, TRPM_TRAP_NO_ERRORCODE, enmEvent, -1);
                if (rc == VINF_SUCCESS /* Don't use RT_SUCCESS */)
                {
                    Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_SELM_SYNC_GDT | VMCPU_FF_SELM_SYNC_LDT | VMCPU_FF_TRPM_SYNC_IDT | VMCPU_FF_SELM_SYNC_TSS));

                    STAM_COUNTER_INC(&pVM->trpm.s.paStatForwardedIRQR3[u8Interrupt]);
                    return VINF_EM_RESCHEDULE_RAW;
                }
            }
        }
        else
            STAM_COUNTER_INC(&pVM->trpm.s.StatForwardFailNoHandler);

        rc = TRPMAssertTrap(pVCpu, u8Interrupt, enmEvent);
        AssertRCReturn(rc, rc);
    }
    else
    {
        /* Can happen if the interrupt is masked by TPR or APIC is disabled. */
        AssertMsg(rc == VERR_APIC_INTR_MASKED_BY_TPR || rc == VERR_NO_DATA, ("PDMGetInterrupt failed. rc=%Rrc\n", rc));
        return HMR3IsActive(pVCpu) ? VINF_EM_RESCHEDULE_HM : VINF_EM_RESCHEDULE_REM; /* (Heed the halted state if this is changed!) */
    }

    /** @todo check if it's safe to translate the patch address to the original guest address.
     *        this implies a safe state in translated instructions and should take sti successors into account (instruction fusing)
     */
    /* Note: if it's a PATM address, then we'll go back to raw mode regardless of the return codes below. */

    /* Fall back to the recompiler */
    return VINF_EM_RESCHEDULE_REM; /* (Heed the halted state if this is changed!) */

#else  /* !TRPM_FORWARD_TRAPS_IN_GC || IEM_VERIFICATION_MODE */
    RT_NOREF(pVM, enmEvent);
    uint8_t u8Interrupt = 0;
    int rc = PDMGetInterrupt(pVCpu, &u8Interrupt);
    Log(("TRPMR3InjectEvent: u8Interrupt=%d (%#x) rc=%Rrc\n", u8Interrupt, u8Interrupt, rc));
    if (RT_SUCCESS(rc))
    {
        rc = TRPMAssertTrap(pVCpu, u8Interrupt, TRPM_HARDWARE_INT);
        AssertRC(rc);
        STAM_COUNTER_INC(&pVM->trpm.s.paStatForwardedIRQR3[u8Interrupt]);
    }
    else
    {
        /* Can happen if the interrupt is masked by TPR or APIC is disabled. */
        AssertMsg(rc == VERR_APIC_INTR_MASKED_BY_TPR || rc == VERR_NO_DATA, ("PDMGetInterrupt failed. rc=%Rrc\n", rc));
    }
    return HMR3IsActive(pVCpu) ? VINF_EM_RESCHEDULE_HM : VINF_EM_RESCHEDULE_REM; /* (Heed the halted state if this is changed!) */
#endif /* !TRPM_FORWARD_TRAPS_IN_GC || IEM_VERIFICATION_MODE */

}


/**
 * Displays the pending TRPM event.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The info helper functions.
 * @param   pszArgs     Arguments, ignored.
 */
static DECLCALLBACK(void) trpmR3InfoEvent(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    NOREF(pszArgs);
    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
        pVCpu = &pVM->aCpus[0];

    uint8_t     uVector;
    uint8_t     cbInstr;
    TRPMEVENT   enmTrapEvent;
    RTGCUINT    uErrorCode;
    RTGCUINTPTR uCR2;
    int rc = TRPMQueryTrapAll(pVCpu, &uVector, &enmTrapEvent, &uErrorCode, &uCR2, &cbInstr);
    if (RT_SUCCESS(rc))
    {
        pHlp->pfnPrintf(pHlp, "CPU[%u]: TRPM event\n", pVCpu->idCpu);
        static const char * const s_apszTrpmEventType[] =
        {
            "Trap",
            "Hardware Int",
            "Software Int"
        };
        if (RT_LIKELY((size_t)enmTrapEvent < RT_ELEMENTS(s_apszTrpmEventType)))
        {
            pHlp->pfnPrintf(pHlp, " Type       = %s\n", s_apszTrpmEventType[enmTrapEvent]);
            pHlp->pfnPrintf(pHlp, " uVector    = %#x\n", uVector);
            pHlp->pfnPrintf(pHlp, " uErrorCode = %#RGu\n", uErrorCode);
            pHlp->pfnPrintf(pHlp, " uCR2       = %#RGp\n", uCR2);
            pHlp->pfnPrintf(pHlp, " cbInstr    = %u bytes\n", cbInstr);
        }
        else
            pHlp->pfnPrintf(pHlp, " Type       = %#x (Invalid!)\n", enmTrapEvent);
    }
    else if (rc == VERR_TRPM_NO_ACTIVE_TRAP)
        pHlp->pfnPrintf(pHlp, "CPU[%u]: TRPM event (None)\n", pVCpu->idCpu);
    else
        pHlp->pfnPrintf(pHlp, "CPU[%u]: TRPM event - Query failed! rc=%Rrc\n", pVCpu->idCpu, rc);
}

