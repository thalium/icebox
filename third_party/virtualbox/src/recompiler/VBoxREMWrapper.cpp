/* $Id: VBoxREMWrapper.cpp $ */
/** @file
 *
 * VBoxREM Win64 DLL Wrapper.
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


/** @page pg_vboxrem_amd64      VBoxREM Hacks on AMD64
 *
 * There are problems with building BoxREM both on WIN64 and 64-bit linux.
 *
 * On linux binutils refuses to link shared objects without -fPIC compiled code
 * (bitches about some fixup types). But when trying to build with -fPIC dyngen
 * doesn't like the code anymore. Sweet. The current solution is to build the
 * VBoxREM code as a relocatable module and use our ELF loader to load it.
 *
 * On WIN64 we're not aware of any GCC port which can emit code using the MSC
 * calling convention. So, we're in for some real fun here. The choice is between
 * porting GCC to AMD64 WIN64 and coming up with some kind of wrapper around
 * either the win32 build or the 64-bit linux build.
 *
 *  -# Porting GCC will be a lot of work. For one thing the calling convention differs
 *     and messing with such stuff can easily create ugly bugs. We would also have to
 *     do some binutils changes, but I think those are rather small compared to GCC.
 *     (That said, the MSC calling convention is far simpler than the linux one, it
 *     reminds me of _Optlink which we have working already.)
 *  -# Wrapping win32 code will work, but addresses outside the first 4GB are
 *     inaccessible and we will have to create 32-64 thunks for all imported functions.
 *     (To switch between 32-bit and 64-bit is load the right CS using far jmps (32->64)
 *     or far returns (both).)
 *  -# Wrapping 64-bit linux code might be the easier solution. The requirements here
 *     are:
 *       - Remove all CRT references we possibly, either by using intrinsics or using
 *         IPRT. Part of IPRT will be linked into VBoxREM2.rel, this will be yet another
 *         IPRT mode which I've dubbed 'no-crt'. The no-crt mode provide basic non-system
 *         dependent stuff.
 *       - Compile and link it into a relocatable object (include the gcc intrinsics
 *         in libgcc). Call this VBoxREM2.rel.
 *       - Write a wrapper dll, VBoxREM.dll, for which during REMR3Init() will load
 *         VBoxREM2.rel (using IPRT) and generate calling convention wrappers
 *         for all IPRT functions and VBoxVMM functions that it uses. All exports
 *         will be wrapped vice versa.
 *       - For building on windows hosts, we will use a mingw32 hosted cross compiler.
 *         and add a 'no-crt' mode to IPRT where it provides the necessary CRT headers
 *         and function implementations.
 *
 * The 3rd solution will be tried out first since it requires the least effort and
 * will let us make use of the full 64-bit register set.
 *
 *
 *
 * @section sec_vboxrem_amd64_compare   Comparing the GCC and MSC calling conventions
 *
 * GCC expects the following (cut & past from page 20 in the ABI draft 0.96):
 *
 * @verbatim
    %rax     temporary register; with variable arguments passes information about the
             number of SSE registers used; 1st return register.
             [Not preserved]
    %rbx     callee-saved register; optionally used as base pointer.
             [Preserved]
    %rcx     used to pass 4th integer argument to functions.
             [Not preserved]
    %rdx     used to pass 3rd argument to functions; 2nd return register
             [Not preserved]
    %rsp     stack pointer
             [Preserved]
    %rbp     callee-saved register; optionally used as frame pointer
             [Preserved]
    %rsi     used to pass 2nd argument to functions
             [Not preserved]
    %rdi     used to pass 1st argument to functions
             [Not preserved]
    %r8      used to pass 5th argument to functions
             [Not preserved]
    %r9      used to pass 6th argument to functions
             [Not preserved]
    %r10     temporary register, used for passing a function's static chain
             pointer [Not preserved]
    %r11     temporary register
             [Not preserved]
    %r12-r15 callee-saved registers
             [Preserved]
    %xmm0-%xmm1  used to pass and return floating point arguments
             [Not preserved]
    %xmm2-%xmm7  used to pass floating point arguments
             [Not preserved]
    %xmm8-%xmm15 temporary registers
             [Not preserved]
    %mmx0-%mmx7  temporary registers
             [Not preserved]
    %st0     temporary register; used to return long double arguments
             [Not preserved]
    %st1     temporary registers; used to return long double arguments
             [Not preserved]
    %st2-%st7 temporary registers
             [Not preserved]
    %fs      Reserved for system use (as thread specific data register)
             [Not preserved]
   @endverbatim
 *
 * Direction flag is preserved as cleared.
 * The stack must be aligned on a 16-byte boundary before the 'call/jmp' instruction.
 *
 *
 *
 * MSC expects the following:
 * @verbatim
    rax      return value, not preserved.
    rbx      preserved.
    rcx      1st argument, integer, not preserved.
    rdx      2nd argument, integer, not preserved.
    rbp      preserved.
    rsp      preserved.
    rsi      preserved.
    rdi      preserved.
    r8       3rd argument, integer, not preserved.
    r9       4th argument, integer, not preserved.
    r10      scratch register, not preserved.
    r11      scratch register, not preserved.
    r12-r15  preserved.
    xmm0     1st argument, fp, return value, not preserved.
    xmm1     2st argument, fp, not preserved.
    xmm2     3st argument, fp, not preserved.
    xmm3     4st argument, fp, not preserved.
    xmm4-xmm5    scratch, not preserved.
    xmm6-xmm15   preserved.
   @endverbatim
 *
 * Dunno what the direction flag is...
 * The stack must be aligned on a 16-byte boundary before the 'call/jmp' instruction.
 *
 *
 * Thus, When GCC code is calling MSC code we don't really have to preserve
 * anything. But but MSC code is calling GCC code, we'll have to save esi and edi.
 *
 */


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def USE_REM_STUBS
 * Define USE_REM_STUBS to stub the entire REM stuff. This is useful during
 * early porting (before we start running stuff).
 */
#if defined(DOXYGEN_RUNNING)
# define USE_REM_STUBS
#endif

/** @def USE_REM_CALLING_CONVENTION_GLUE
 * Define USE_REM_CALLING_CONVENTION_GLUE for platforms where it's necessary to
 * use calling convention wrappers.
 */
#if (defined(RT_ARCH_AMD64) && defined(RT_OS_WINDOWS)) || defined(DOXYGEN_RUNNING)
# define USE_REM_CALLING_CONVENTION_GLUE
#endif

/** @def USE_REM_IMPORT_JUMP_GLUE
 * Define USE_REM_IMPORT_JUMP_GLUE for platforms where we need to
 * emit some jump glue to deal with big addresses.
 */
#if (defined(RT_ARCH_AMD64) && !defined(USE_REM_CALLING_CONVENTION_GLUE) && !defined(RT_OS_DARWIN)) || defined(DOXYGEN_RUNNING)
# define USE_REM_IMPORT_JUMP_GLUE
#endif

/** @def VBOX_USE_BITNESS_SELECTOR
 * Define VBOX_USE_BITNESS_SELECTOR to build this module as a bitness selector
 * between VBoxREM32 and VBoxREM64.
 */
#if defined(DOXYGEN_RUNNING)
# define VBOX_USE_BITNESS_SELECTOR
#endif

/** @def VBOX_WITHOUT_REM_LDR_CYCLE
 * Define VBOX_WITHOUT_REM_LDR_CYCLE dynamically resolve any dependencies on
 * VBoxVMM and thus avoid the cyclic dependency between VBoxREM and VBoxVMM.
 */
#if defined(DOXYGEN_RUNNING)
# define VBOX_WITHOUT_REM_LDR_CYCLE
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_REM
#include <VBox/vmm/rem.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/dbg.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/dis.h>

#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/ldr.h>
#include <iprt/lockvalidator.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/stream.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Parameter descriptor.
 */
typedef struct REMPARMDESC
{
    /** Parameter flags (REMPARMDESC_FLAGS_*). */
    uint8_t     fFlags;
    /** The parameter size if REMPARMDESC_FLAGS_SIZE is set. */
    uint8_t     cb;
    /** Pointer to additional data.
     * For REMPARMDESC_FLAGS_PFN this is a PREMFNDESC. */
    void       *pvExtra;

} REMPARMDESC, *PREMPARMDESC;
/** Pointer to a constant parameter descriptor. */
typedef const REMPARMDESC *PCREMPARMDESC;

/** @name Parameter descriptor flags.
 * @{ */
/** The parameter type is a kind of integer which could fit in a register. This includes pointers. */
#define REMPARMDESC_FLAGS_INT           0
/** The parameter is a GC pointer. */
#define REMPARMDESC_FLAGS_GCPTR         1
/** The parameter is a GC physical address. */
#define REMPARMDESC_FLAGS_GCPHYS        2
/** The parameter is a HC physical address. */
#define REMPARMDESC_FLAGS_HCPHYS        3
/** The parameter type is a kind of floating point. */
#define REMPARMDESC_FLAGS_FLOAT         4
/** The parameter value is a struct. This type takes a size. */
#define REMPARMDESC_FLAGS_STRUCT        5
/** The parameter is an elipsis. */
#define REMPARMDESC_FLAGS_ELLIPSIS      6
/** The parameter is a va_list. */
#define REMPARMDESC_FLAGS_VALIST        7
/** The parameter is a function pointer. pvExtra is a PREMFNDESC. */
#define REMPARMDESC_FLAGS_PFN           8
/** The parameter type mask. */
#define REMPARMDESC_FLAGS_TYPE_MASK     15
/** The parameter size field is valid. */
#define REMPARMDESC_FLAGS_SIZE          RT_BIT(7)
/** @} */

/**
 * Function descriptor.
 */
typedef struct REMFNDESC
{
    /** The function name. */
    const char         *pszName;
    /** Exports: Pointer to the function pointer.
     * Imports: Pointer to the function. */
    void               *pv;
    /** Array of parameter descriptors. */
    PCREMPARMDESC       paParams;
    /** The number of parameter descriptors pointed to by paParams. */
    uint8_t             cParams;
    /** Function flags (REMFNDESC_FLAGS_*). */
    uint8_t             fFlags;
    /** The size of the return value. */
    uint8_t             cbReturn;
    /** Pointer to the wrapper code for imports. */
    void               *pvWrapper;
} REMFNDESC, *PREMFNDESC;
/** Pointer to a constant function descriptor. */
typedef const REMFNDESC *PCREMFNDESC;

/** @name Function descriptor flags.
 * @{ */
/** The return type is void. */
#define REMFNDESC_FLAGS_RET_VOID        0
/** The return type is a kind of integer passed in rax/eax. This includes pointers. */
#define REMFNDESC_FLAGS_RET_INT         1
/** The return type is a kind of floating point. */
#define REMFNDESC_FLAGS_RET_FLOAT       2
/** The return value is a struct. This type take a size. */
#define REMFNDESC_FLAGS_RET_STRUCT      3
/** The return type mask. */
#define REMFNDESC_FLAGS_RET_TYPE_MASK   7
/** The argument list contains one or more va_list arguments (i.e. problems). */
#define REMFNDESC_FLAGS_VALIST          RT_BIT(6)
/** The function has an ellipsis (i.e. a problem). */
#define REMFNDESC_FLAGS_ELLIPSIS        RT_BIT(7)
/** @} */

/**
 * Chunk of read-write-executable memory.
 */
typedef struct REMEXECMEM
{
    /** The number of bytes left. */
    struct REMEXECMEM  *pNext;
    /** The size of this chunk. */
    uint32_t            cb;
    /** The offset of the next code block. */
    uint32_t            off;
#if ARCH_BITS == 32
    uint32_t            padding;
#endif
} REMEXECMEM, *PREMEXECMEM;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifndef USE_REM_STUBS
/** Loader handle of the REM object/DLL. */
static RTLDRMOD g_ModREM2 = NIL_RTLDRMOD;
# ifndef VBOX_USE_BITNESS_SELECTOR
/** Pointer to the memory containing the loaded REM2 object/DLL. */
static void    *g_pvREM2 = NULL;
/** The size of the memory g_pvREM2 is pointing to. */
static size_t   g_cbREM2 = 0;
# endif
# ifdef VBOX_WITHOUT_REM_LDR_CYCLE
/** Loader handle of the VBoxVMM DLL. */
static RTLDRMOD g_ModVMM = NIL_RTLDRMOD;
# endif

/** Linux object export addresses.
 * These are references from the assembly wrapper code.
 * @{ */
static DECLCALLBACKPTR(int, pfnREMR3Init)(PVM);
static DECLCALLBACKPTR(int, pfnREMR3InitFinalize)(PVM);
static DECLCALLBACKPTR(int, pfnREMR3Term)(PVM);
static DECLCALLBACKPTR(void, pfnREMR3Reset)(PVM);
static DECLCALLBACKPTR(int, pfnREMR3Step)(PVM, PVMCPU);
static DECLCALLBACKPTR(int, pfnREMR3BreakpointSet)(PVM, RTGCUINTPTR);
static DECLCALLBACKPTR(int, pfnREMR3BreakpointClear)(PVM, RTGCUINTPTR);
static DECLCALLBACKPTR(int, pfnREMR3EmulateInstruction)(PVM, PVMCPU);
static DECLCALLBACKPTR(int, pfnREMR3Run)(PVM, PVMCPU);
static DECLCALLBACKPTR(int, pfnREMR3State)(PVM, PVMCPU);
static DECLCALLBACKPTR(int, pfnREMR3StateBack)(PVM, PVMCPU);
static DECLCALLBACKPTR(void, pfnREMR3StateUpdate)(PVM, PVMCPU);
static DECLCALLBACKPTR(void, pfnREMR3A20Set)(PVM, PVMCPU, bool);
static DECLCALLBACKPTR(void, pfnREMR3ReplayHandlerNotifications)(PVM pVM);
static DECLCALLBACKPTR(void, pfnREMR3NotifyPhysRamRegister)(PVM, RTGCPHYS, RTGCPHYS, unsigned);
static DECLCALLBACKPTR(void, pfnREMR3NotifyPhysRamDeregister)(PVM, RTGCPHYS, RTUINT);
static DECLCALLBACKPTR(void, pfnREMR3NotifyPhysRomRegister)(PVM, RTGCPHYS, RTUINT, void *, bool);
static DECLCALLBACKPTR(void, pfnREMR3NotifyHandlerPhysicalModify)(PVM, PGMPHYSHANDLERKIND, RTGCPHYS, RTGCPHYS, RTGCPHYS, bool, bool);
static DECLCALLBACKPTR(void, pfnREMR3NotifyHandlerPhysicalRegister)(PVM, PGMPHYSHANDLERKIND, RTGCPHYS, RTGCPHYS, bool);
static DECLCALLBACKPTR(void, pfnREMR3NotifyHandlerPhysicalDeregister)(PVM, PGMPHYSHANDLERKIND, RTGCPHYS, RTGCPHYS, bool, bool);
static DECLCALLBACKPTR(void, pfnREMR3NotifyInterruptSet)(PVM, PVMCPU);
static DECLCALLBACKPTR(void, pfnREMR3NotifyInterruptClear)(PVM, PVMCPU);
static DECLCALLBACKPTR(void, pfnREMR3NotifyTimerPending)(PVM, PVMCPU);
static DECLCALLBACKPTR(void, pfnREMR3NotifyDmaPending)(PVM);
static DECLCALLBACKPTR(void, pfnREMR3NotifyQueuePending)(PVM);
static DECLCALLBACKPTR(void, pfnREMR3NotifyFF)(PVM);
static DECLCALLBACKPTR(int, pfnREMR3NotifyCodePageChanged)(PVM, PVMCPU, RTGCPTR);
static DECLCALLBACKPTR(int, pfnREMR3DisasEnableStepping)(PVM, bool);
static DECLCALLBACKPTR(bool, pfnREMR3IsPageAccessHandled)(PVM, RTGCPHYS);
/** @} */

/** Export and import parameter descriptors.
 * @{
 */
/* Common args. */
static const REMPARMDESC g_aArgsSIZE_T[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsPTR[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL }
};
static const REMPARMDESC g_aArgsSIZE_TTag[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsPTRTag[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsPTR_SIZE_T[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsSIZE_TTagLoc[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned int),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsPTRLoc[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned int),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsVM[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL }
};
static const REMPARMDESC g_aArgsVMCPU[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL }
};

static const REMPARMDESC g_aArgsVMandVMCPU[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL }
};

/* REM args */
static const REMPARMDESC g_aArgsBreakpoint[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCUINTPTR),        NULL }
};
static const REMPARMDESC g_aArgsA20Set[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsNotifyPhysRamRegister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL }
};
static const REMPARMDESC g_aArgsNotifyPhysRamChunkRegister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTUINT),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTHCUINTPTR),        NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL }
};
static const REMPARMDESC g_aArgsNotifyPhysRamDeregister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTUINT),             NULL }
};
static const REMPARMDESC g_aArgsNotifyPhysRomRegister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTUINT),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsNotifyHandlerPhysicalModify[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMPHYSHANDLERKIND), NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsNotifyHandlerPhysicalRegister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMPHYSHANDLERKIND), NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsNotifyHandlerPhysicalDeregister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMPHYSHANDLERKIND), NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsNotifyCodePageChanged[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCUINTPTR),        NULL }
};
static const REMPARMDESC g_aArgsNotifyPendingInterrupt[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t),            NULL }
};
static const REMPARMDESC g_aArgsDisasEnableStepping[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsIsPageAccessHandled[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL }
};

# ifndef VBOX_USE_BITNESS_SELECTOR

/* VMM args */
static const REMPARMDESC g_aArgsAPICUpdatePendingInterrupts[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL }
};
static const REMPARMDESC g_aArgsAPICGetTpr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t *),          NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t *),          NULL }
};
static const REMPARMDESC g_aArgsAPICSetTpr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t),            NULL }
};
static const REMPARMDESC g_aArgsCPUMGetGuestCpl[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
};

/* CPUMQueryGuestMsr args */
static const REMPARMDESC g_aArgsCPUMQueryGuestMsr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t *),         NULL },
};

/* CPUMSetGuestMsr args */
static const REMPARMDESC g_aArgsCPUMSetGuestMsr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t),           NULL },
};

static const REMPARMDESC g_aArgsCPUMGetGuestCpuId[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL }
};

static const REMPARMDESC g_aArgsCPUMR3RemEnter[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL }
};

static const REMPARMDESC g_aArgsCPUMR3RemLeave[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};

static const REMPARMDESC g_aArgsCPUMSetChangedFlags[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};

static const REMPARMDESC g_aArgsCPUMQueryGuestCtxPtr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL }
};
static const REMPARMDESC g_aArgsCSAMR3MonitorPage[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTRCPTR),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(CSAMTAG),            NULL }
};
static const REMPARMDESC g_aArgsCSAMR3UnmonitorPage[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTRCPTR),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(CSAMTAG),            NULL }
};

static const REMPARMDESC g_aArgsCSAMR3RecordCallAddress[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTRCPTR),            NULL }
};

#  if defined(VBOX_WITH_DEBUGGER) && !(defined(RT_OS_WINDOWS) && defined(RT_ARCH_AMD64)) /* the callbacks are problematic */
static const REMPARMDESC g_aArgsDBGCRegisterCommands[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PCDBGCCMD),          NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL }
};
#  endif
static const REMPARMDESC g_aArgsDBGFR3DisasInstrEx[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PUVM),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(VMCPUID),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTSEL),              NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTGCPTR),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(char *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL }
};
static const REMPARMDESC g_aArgsDBGFR3DisasInstrCurrentLogInternal[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(char *),             NULL }
};
static const REMPARMDESC g_aArgsDBGFR3Info[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PUVM),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PCDBGFINFOHLP),      NULL }
};
static const REMPARMDESC g_aArgsDBGFR3AsSymbolByAddr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PUVM),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTDBGAS),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PCDBGFADDRESS),      NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(PRTGCINTPTR),        NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTDBGSYMBOL),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTDBGMOD),          NULL }
};
static const REMPARMDESC g_aArgsDBGFR3AddrFromFlat[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PUVM),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PDBGFADDRESS),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTGCUINTPTR),        NULL }
};
static const REMPARMDESC g_aArgsDISInstrToStr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t const *),    NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(DISCPUMODE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PDISCPUSTATE),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(char *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsEMR3FatalError[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(int),                NULL }
};
static const REMPARMDESC g_aArgsEMSetInhibitInterruptsPC[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTGCPTR),            NULL }
};
static const REMPARMDESC g_aArgsHMR3CanExecuteGuest[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsIOMIOPortRead[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTIOPORT),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsIOMIOPortWrite[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTIOPORT),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsIOMMMIORead[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsIOMMMIOWrite[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsMMR3HeapAlloc[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(MMTAG),              NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsMMR3HeapAllocZ[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(MMTAG),              NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsPATMIsPatchGCAddr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTRCUINTPTR),        NULL }
};
static const REMPARMDESC g_aArgsPATMR3QueryOpcode[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTRCPTR),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t *),          NULL }
};
static const REMPARMDESC g_aArgsPDMGetInterrupt[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t *),          NULL }
};
static const REMPARMDESC g_aArgsPDMIsaSetIrq[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsPDMR3CritSectInit[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PPDMCRITSECT),       NULL },
    /* RT_SRC_POS_DECL */
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned int),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(char *),             NULL },
    { REMPARMDESC_FLAGS_ELLIPSIS,   0 }
};
static const REMPARMDESC g_aArgsPDMCritSectEnter[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PPDMCRITSECT),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(int),                NULL }
};
static const REMPARMDESC g_aArgsPDMCritSectEnterDebug[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PPDMCRITSECT),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(int),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTHCUINTPTR),        NULL },
    /* RT_SRC_POS_DECL */
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsPGMGetGuestMode[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
};
static const REMPARMDESC g_aArgsPGMGstGetPage[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCPTR),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t *),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTGCPHYS),          NULL }
};
static const REMPARMDESC g_aArgsPGMInvalidatePage[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCPTR),            NULL }
};
static const REMPARMDESC g_aArgsPGMR3PhysTlbGCPhys2Ptr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL }
};
static const REMPARMDESC g_aArgsPGM3PhysGrowRange[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PCRTGCPHYS),         NULL }
};
static const REMPARMDESC g_aArgsPGMPhysIsGCPhysValid[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL }
};
static const REMPARMDESC g_aArgsPGMPhysRead[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsPGMPhysSimpleReadGCPtr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCPTR),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsPGMPhysWrite[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const void *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsPGMChangeMode[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t),           NULL }
};
static const REMPARMDESC g_aArgsPGMFlushTLB[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(bool),               NULL }
};
static const REMPARMDESC g_aArgsPGMR3PhysReadUxx[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsPGMR3PhysWriteU8[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsPGMR3PhysWriteU16[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint16_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsPGMR3PhysWriteU32[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsPGMR3PhysWriteU64[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_GCPHYS,     sizeof(RTGCPHYS),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint64_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PGMACCESSORIGIN),    NULL }
};
static const REMPARMDESC g_aArgsRTMemReallocTag[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void*),              NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsRTMemEfRealloc[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void*),              NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned int),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsSSMR3GetGCPtr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTGCPTR),           NULL }
};
static const REMPARMDESC g_aArgsSSMR3GetMem[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsSSMR3GetU32[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t *),         NULL }
};
static const REMPARMDESC g_aArgsSSMR3GetUInt[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTUINT),            NULL }
};
static const REMPARMDESC g_aArgsSSMR3PutGCPtr[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCPTR),            NULL }
};
static const REMPARMDESC g_aArgsSSMR3PutMem[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const void *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsSSMR3PutU32[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
};
static const REMPARMDESC g_aArgsSSMR3PutUInt[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(RTUINT),             NULL },
};

static const REMPARMDESC g_aArgsSSMIntLiveExecCallback[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
};
static REMFNDESC g_SSMIntLiveExecCallback =
{
    "SSMIntLiveExecCallback", NULL, &g_aArgsSSMIntLiveExecCallback[0], RT_ELEMENTS(g_aArgsSSMIntLiveExecCallback), REMFNDESC_FLAGS_RET_INT, sizeof(int),  NULL
};

static const REMPARMDESC g_aArgsSSMIntLiveVoteCallback[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
};
static REMFNDESC g_SSMIntLiveVoteCallback =
{
    "SSMIntLiveVoteCallback", NULL, &g_aArgsSSMIntLiveVoteCallback[0], RT_ELEMENTS(g_aArgsSSMIntLiveVoteCallback), REMFNDESC_FLAGS_RET_INT, sizeof(bool),  NULL
};

static const REMPARMDESC g_aArgsSSMIntCallback[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
};
static REMFNDESC g_SSMIntCallback =
{
    "SSMIntCallback", NULL, &g_aArgsSSMIntCallback[0], RT_ELEMENTS(g_aArgsSSMIntCallback), REMFNDESC_FLAGS_RET_INT, sizeof(int),  NULL
};

static const REMPARMDESC g_aArgsSSMIntLoadExecCallback[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(PSSMHANDLE),         NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
};
static REMFNDESC g_SSMIntLoadExecCallback =
{
    "SSMIntLoadExecCallback", NULL, &g_aArgsSSMIntLoadExecCallback[0], RT_ELEMENTS(g_aArgsSSMIntLoadExecCallback), REMFNDESC_FLAGS_RET_INT, sizeof(int),  NULL
};
/* Note: don't forget about the handwritten assembly wrapper when changing this! */
static const REMPARMDESC g_aArgsSSMR3RegisterInternal[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTLIVEPREP),  &g_SSMIntCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTLIVEEXEC),  &g_SSMIntLiveExecCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTLIVEVOTE),  &g_SSMIntLiveVoteCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTSAVEPREP),  &g_SSMIntCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTSAVEEXEC),  &g_SSMIntCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTSAVEDONE),  &g_SSMIntCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTLOADPREP),  &g_SSMIntCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTLOADEXEC),  &g_SSMIntLoadExecCallback },
    { REMPARMDESC_FLAGS_PFN,        sizeof(PFNSSMINTLOADDONE),  &g_SSMIntCallback },
};

static const REMPARMDESC g_aArgsSTAMR3Register[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(STAMTYPE),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(STAMVISIBILITY),     NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(STAMUNIT),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsSTAMR3Deregister[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
};
static const REMPARMDESC g_aArgsTRPMAssertTrap[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(TRPMEVENT),          NULL }
};
static const REMPARMDESC g_aArgsTRPMQueryTrap[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(uint8_t *),          NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(TRPMEVENT *),        NULL }
};
static const REMPARMDESC g_aArgsTRPMSetErrorCode[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCUINT),           NULL }
};
static const REMPARMDESC g_aArgsTRPMSetFaultAddress[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMCPU),             NULL },
    { REMPARMDESC_FLAGS_GCPTR,      sizeof(RTGCUINT),           NULL }
};
static const REMPARMDESC g_aArgsVMR3ReqCallWait[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVM),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(VMCPUID),            NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_ELLIPSIS,   0,                          NULL }
};
static const REMPARMDESC g_aArgsVMR3ReqFree[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PVMREQ),             NULL }
};

/* IPRT args */
static const REMPARMDESC g_aArgsRTAssertMsg1[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsRTAssertMsg2[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_ELLIPSIS,   0,                          NULL }
};
static const REMPARMDESC g_aArgsRTAssertMsg2V[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_VALIST,     0,                          NULL }
};
static const REMPARMDESC g_aArgsRTLogGetDefaultInstanceEx[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(uint32_t),           NULL }
};
static const REMPARMDESC g_aArgsRTLogFlags[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTLOGGER),          NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL }
};
static const REMPARMDESC g_aArgsRTLogFlush[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTLOGGER),          NULL }
};
static const REMPARMDESC g_aArgsRTLogLoggerEx[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTLOGGER),          NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_ELLIPSIS,   0,                          NULL }
};
static const REMPARMDESC g_aArgsRTLogLoggerExV[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(PRTLOGGER),          NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_VALIST,     0,                          NULL }
};
static const REMPARMDESC g_aArgsRTLogPrintf[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_ELLIPSIS,   0,                          NULL }
};
static const REMPARMDESC g_aArgsRTMemProtect[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(unsigned),           NULL }
};
static const REMPARMDESC g_aArgsRTStrPrintf[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(char *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_ELLIPSIS,   0,                          NULL }
};
static const REMPARMDESC g_aArgsRTStrPrintfV[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(char *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const char *),       NULL },
    { REMPARMDESC_FLAGS_VALIST,     0,                          NULL }
};
static const REMPARMDESC g_aArgsThread[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(RTTHREAD),           NULL }
};


/* CRT args */
static const REMPARMDESC g_aArgsmemcpy[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(const  void *),      NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};
static const REMPARMDESC g_aArgsmemset[] =
{
    { REMPARMDESC_FLAGS_INT,        sizeof(void *),             NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(int),                NULL },
    { REMPARMDESC_FLAGS_INT,        sizeof(size_t),             NULL }
};

# endif /* !VBOX_USE_BITNESS_SELECTOR */

/** @} */

/**
 * Descriptors for the exported functions.
 */
static const REMFNDESC g_aExports[] =
{  /* pszName,                                  (void *)pv,                                         pParams,                                    cParams,                                               fFlags,                     cb,             pvWrapper. */
    { "REMR3Init",                              (void *)&pfnREMR3Init,                              &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3InitFinalize",                      (void *)&pfnREMR3InitFinalize,                      &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3Term",                              (void *)&pfnREMR3Term,                              &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3Reset",                             (void *)&pfnREMR3Reset,                             &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3Step",                              (void *)&pfnREMR3Step,                              &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3BreakpointSet",                     (void *)&pfnREMR3BreakpointSet,                     &g_aArgsBreakpoint[0],                      RT_ELEMENTS(g_aArgsBreakpoint),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3BreakpointClear",                   (void *)&pfnREMR3BreakpointClear,                   &g_aArgsBreakpoint[0],                      RT_ELEMENTS(g_aArgsBreakpoint),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3EmulateInstruction",                (void *)&pfnREMR3EmulateInstruction,                &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3Run",                               (void *)&pfnREMR3Run,                               &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3State",                             (void *)&pfnREMR3State,                             &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3StateBack",                         (void *)&pfnREMR3StateBack,                         &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3StateUpdate",                       (void *)&pfnREMR3StateUpdate,                       &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3A20Set",                            (void *)&pfnREMR3A20Set,                            &g_aArgsA20Set[0],                          RT_ELEMENTS(g_aArgsA20Set),                            REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3ReplayHandlerNotifications",        (void *)&pfnREMR3ReplayHandlerNotifications,        &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyPhysRamRegister",             (void *)&pfnREMR3NotifyPhysRamRegister,             &g_aArgsNotifyPhysRamRegister[0],           RT_ELEMENTS(g_aArgsNotifyPhysRamRegister),             REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyPhysRamDeregister",           (void *)&pfnREMR3NotifyPhysRamDeregister,           &g_aArgsNotifyPhysRamDeregister[0],         RT_ELEMENTS(g_aArgsNotifyPhysRamDeregister),           REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyPhysRomRegister",             (void *)&pfnREMR3NotifyPhysRomRegister,             &g_aArgsNotifyPhysRomRegister[0],           RT_ELEMENTS(g_aArgsNotifyPhysRomRegister),             REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyHandlerPhysicalModify",       (void *)&pfnREMR3NotifyHandlerPhysicalModify,       &g_aArgsNotifyHandlerPhysicalModify[0],     RT_ELEMENTS(g_aArgsNotifyHandlerPhysicalModify),       REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyHandlerPhysicalRegister",     (void *)&pfnREMR3NotifyHandlerPhysicalRegister,     &g_aArgsNotifyHandlerPhysicalRegister[0],   RT_ELEMENTS(g_aArgsNotifyHandlerPhysicalRegister),     REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyHandlerPhysicalDeregister",   (void *)&pfnREMR3NotifyHandlerPhysicalDeregister,   &g_aArgsNotifyHandlerPhysicalDeregister[0], RT_ELEMENTS(g_aArgsNotifyHandlerPhysicalDeregister),   REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyInterruptSet",                (void *)&pfnREMR3NotifyInterruptSet,                &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyInterruptClear",              (void *)&pfnREMR3NotifyInterruptClear,              &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyTimerPending",                (void *)&pfnREMR3NotifyTimerPending,                &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyDmaPending",                  (void *)&pfnREMR3NotifyDmaPending,                  &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyQueuePending",                (void *)&pfnREMR3NotifyQueuePending,                &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyFF",                          (void *)&pfnREMR3NotifyFF,                          &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,              NULL },
    { "REMR3NotifyCodePageChanged",             (void *)&pfnREMR3NotifyCodePageChanged,             &g_aArgsNotifyCodePageChanged[0],           RT_ELEMENTS(g_aArgsNotifyCodePageChanged),             REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3DisasEnableStepping",               (void *)&pfnREMR3DisasEnableStepping,               &g_aArgsDisasEnableStepping[0],             RT_ELEMENTS(g_aArgsDisasEnableStepping),               REMFNDESC_FLAGS_RET_INT,    sizeof(int),    NULL },
    { "REMR3IsPageAccessHandled",               (void *)&pfnREMR3IsPageAccessHandled,               &g_aArgsIsPageAccessHandled[0],             RT_ELEMENTS(g_aArgsIsPageAccessHandled),               REMFNDESC_FLAGS_RET_INT,    sizeof(bool),   NULL }
};

# ifndef VBOX_USE_BITNESS_SELECTOR

#  ifdef VBOX_WITHOUT_REM_LDR_CYCLE
#   define VMM_FN(name)  NULL
#  else
#   define VMM_FN(name)  (void *)(uintptr_t)& name
#  endif

/**
 * Descriptors for the functions imported from VBoxVMM.
 */
static REMFNDESC g_aVMMImports[] =
{
    { "APICUpdatePendingInterrupts",            VMM_FN(APICUpdatePendingInterrupts),    &g_aArgsAPICUpdatePendingInterrupts[0],     RT_ELEMENTS(g_aArgsAPICUpdatePendingInterrupts),       REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "APICGetTpr",                             VMM_FN(APICGetTpr),                     &g_aArgsAPICGetTpr[0],                      RT_ELEMENTS(g_aArgsAPICGetTpr),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "APICSetTpr",                             VMM_FN(APICSetTpr),                     &g_aArgsAPICSetTpr[0],                      RT_ELEMENTS(g_aArgsAPICSetTpr),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "CPUMR3RemEnter",                         VMM_FN(CPUMR3RemEnter),                 &g_aArgsCPUMR3RemEnter[0],                  RT_ELEMENTS(g_aArgsCPUMR3RemEnter),                    REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMR3RemLeave",                         VMM_FN(CPUMR3RemLeave),                 &g_aArgsCPUMR3RemLeave[0],                  RT_ELEMENTS(g_aArgsCPUMR3RemLeave),                    REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "CPUMSetChangedFlags",                    VMM_FN(CPUMSetChangedFlags),            &g_aArgsCPUMSetChangedFlags[0],             RT_ELEMENTS(g_aArgsCPUMSetChangedFlags),               REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "CPUMGetGuestCPL",                        VMM_FN(CPUMGetGuestCPL),                &g_aArgsCPUMGetGuestCpl[0],                 RT_ELEMENTS(g_aArgsCPUMGetGuestCpl),                   REMFNDESC_FLAGS_RET_INT,    sizeof(unsigned),   NULL },
    { "CPUMQueryGuestMsr",                      VMM_FN(CPUMQueryGuestMsr),              &g_aArgsCPUMQueryGuestMsr[0],               RT_ELEMENTS(g_aArgsCPUMQueryGuestMsr),                 REMFNDESC_FLAGS_RET_INT,    sizeof(uint64_t),   NULL },
    { "CPUMSetGuestMsr",                        VMM_FN(CPUMSetGuestMsr),                &g_aArgsCPUMSetGuestMsr[0],                 RT_ELEMENTS(g_aArgsCPUMSetGuestMsr),                   REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "CPUMGetGuestCpuId",                      VMM_FN(CPUMGetGuestCpuId),              &g_aArgsCPUMGetGuestCpuId[0],               RT_ELEMENTS(g_aArgsCPUMGetGuestCpuId),                 REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "CPUMGetGuestEAX",                        VMM_FN(CPUMGetGuestEAX),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestEBP",                        VMM_FN(CPUMGetGuestEBP),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestEBX",                        VMM_FN(CPUMGetGuestEBX),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestECX",                        VMM_FN(CPUMGetGuestECX),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestEDI",                        VMM_FN(CPUMGetGuestEDI),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestEDX",                        VMM_FN(CPUMGetGuestEDX),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestEIP",                        VMM_FN(CPUMGetGuestEIP),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestESI",                        VMM_FN(CPUMGetGuestESI),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestESP",                        VMM_FN(CPUMGetGuestESP),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "CPUMGetGuestCS",                         VMM_FN(CPUMGetGuestCS),                 &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(RTSEL),      NULL },
    { "CPUMGetGuestSS",                         VMM_FN(CPUMGetGuestSS),                 &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(RTSEL),      NULL },
    { "CPUMGetGuestCpuVendor",                  VMM_FN(CPUMGetGuestCpuVendor),          &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT, sizeof(CPUMCPUVENDOR), NULL },
    { "CPUMQueryGuestCtxPtr",                   VMM_FN(CPUMQueryGuestCtxPtr),           &g_aArgsCPUMQueryGuestCtxPtr[0],            RT_ELEMENTS(g_aArgsCPUMQueryGuestCtxPtr),              REMFNDESC_FLAGS_RET_INT,    sizeof(PCPUMCTX),   NULL },
    { "CSAMR3MonitorPage",                      VMM_FN(CSAMR3MonitorPage),              &g_aArgsCSAMR3MonitorPage[0],               RT_ELEMENTS(g_aArgsCSAMR3MonitorPage),                 REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "CSAMR3UnmonitorPage",                    VMM_FN(CSAMR3UnmonitorPage),            &g_aArgsCSAMR3UnmonitorPage[0],             RT_ELEMENTS(g_aArgsCSAMR3UnmonitorPage),               REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "CSAMR3RecordCallAddress",                VMM_FN(CSAMR3RecordCallAddress),        &g_aArgsCSAMR3RecordCallAddress[0],         RT_ELEMENTS(g_aArgsCSAMR3RecordCallAddress),           REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
#  if defined(VBOX_WITH_DEBUGGER) && !(defined(RT_OS_WINDOWS) && defined(RT_ARCH_AMD64)) /* the callbacks are problematic */
    { "DBGCRegisterCommands",                   VMM_FN(DBGCRegisterCommands),           &g_aArgsDBGCRegisterCommands[0],            RT_ELEMENTS(g_aArgsDBGCRegisterCommands),              REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
#  endif
    { "DBGFR3DisasInstrEx",                     VMM_FN(DBGFR3DisasInstrEx),             &g_aArgsDBGFR3DisasInstrEx[0],              RT_ELEMENTS(g_aArgsDBGFR3DisasInstrEx),                REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "DBGFR3DisasInstrCurrentLogInternal",     VMM_FN(DBGFR3DisasInstrCurrentLogInternal), &g_aArgsDBGFR3DisasInstrCurrentLogInternal[0],  RT_ELEMENTS(g_aArgsDBGFR3DisasInstrCurrentLogInternal),REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "DBGFR3Info",                             VMM_FN(DBGFR3Info),                     &g_aArgsDBGFR3Info[0],                      RT_ELEMENTS(g_aArgsDBGFR3Info),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "DBGFR3InfoLogRelHlp",                    VMM_FN(DBGFR3InfoLogRelHlp),            NULL,                                       0,                                                     REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "DBGFR3AsSymbolByAddr",                   VMM_FN(DBGFR3AsSymbolByAddr),           &g_aArgsDBGFR3AsSymbolByAddr[0],            RT_ELEMENTS(g_aArgsDBGFR3AsSymbolByAddr),              REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "DBGFR3AddrFromFlat",                     VMM_FN(DBGFR3AddrFromFlat),             &g_aArgsDBGFR3AddrFromFlat[0],              RT_ELEMENTS(g_aArgsDBGFR3AddrFromFlat),                REMFNDESC_FLAGS_RET_INT,    sizeof(PDBGFADDRESS),       NULL },
    { "DISInstrToStr",                          VMM_FN(DISInstrToStr),                  &g_aArgsDISInstrToStr[0],                   RT_ELEMENTS(g_aArgsDISInstrToStr),                     REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "EMR3FatalError",                         VMM_FN(EMR3FatalError),                 &g_aArgsEMR3FatalError[0],                  RT_ELEMENTS(g_aArgsEMR3FatalError),                    REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "EMRemLock",                              VMM_FN(EMRemLock),                      &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "EMRemUnlock",                            VMM_FN(EMRemUnlock),                    &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "EMRemIsLockOwner",                       VMM_FN(EMRemIsLockOwner),               &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   sizeof(bool),       NULL },
    { "EMGetInhibitInterruptsPC",               VMM_FN(EMGetInhibitInterruptsPC),       &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(RTGCPTR),    NULL },
    { "EMSetInhibitInterruptsPC",               VMM_FN(EMSetInhibitInterruptsPC),       &g_aArgsEMSetInhibitInterruptsPC[0],        RT_ELEMENTS(g_aArgsEMSetInhibitInterruptsPC),          REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "HMIsEnabledNotMacro",                    VMM_FN(HMIsEnabledNotMacro),            &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "HMR3CanExecuteGuest",                    VMM_FN(HMR3CanExecuteGuest),            &g_aArgsHMR3CanExecuteGuest[0],             RT_ELEMENTS(g_aArgsHMR3CanExecuteGuest),               REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "IOMIOPortRead",                          VMM_FN(IOMIOPortRead),                  &g_aArgsIOMIOPortRead[0],                   RT_ELEMENTS(g_aArgsIOMIOPortRead),                     REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "IOMIOPortWrite",                         VMM_FN(IOMIOPortWrite),                 &g_aArgsIOMIOPortWrite[0],                  RT_ELEMENTS(g_aArgsIOMIOPortWrite),                    REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "IOMMMIORead",                            VMM_FN(IOMMMIORead),                    &g_aArgsIOMMMIORead[0],                     RT_ELEMENTS(g_aArgsIOMMMIORead),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "IOMMMIOWrite",                           VMM_FN(IOMMMIOWrite),                   &g_aArgsIOMMMIOWrite[0],                    RT_ELEMENTS(g_aArgsIOMMMIOWrite),                      REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "MMR3HeapAlloc",                          VMM_FN(MMR3HeapAlloc),                  &g_aArgsMMR3HeapAlloc[0],                   RT_ELEMENTS(g_aArgsMMR3HeapAlloc),                     REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "MMR3HeapAllocZ",                         VMM_FN(MMR3HeapAllocZ),                 &g_aArgsMMR3HeapAllocZ[0],                  RT_ELEMENTS(g_aArgsMMR3HeapAllocZ),                    REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "MMR3PhysGetRamSize",                     VMM_FN(MMR3PhysGetRamSize),             &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(uint64_t),   NULL },
    { "PATMIsPatchGCAddr",                      VMM_FN(PATMIsPatchGCAddr),              &g_aArgsPATMIsPatchGCAddr[0],               RT_ELEMENTS(g_aArgsPATMIsPatchGCAddr),                 REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "PATMR3QueryOpcode",                      VMM_FN(PATMR3QueryOpcode),              &g_aArgsPATMR3QueryOpcode[0],               RT_ELEMENTS(g_aArgsPATMR3QueryOpcode),                 REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PDMR3DmaRun",                            VMM_FN(PDMR3DmaRun),                    &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PDMR3CritSectInit",                      VMM_FN(PDMR3CritSectInit),              &g_aArgsPDMR3CritSectInit[0],               RT_ELEMENTS(g_aArgsPDMR3CritSectInit),                 REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PDMCritSectEnter",                       VMM_FN(PDMCritSectEnter),               &g_aArgsPDMCritSectEnter[0],                RT_ELEMENTS(g_aArgsPDMCritSectEnter),                  REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PDMCritSectLeave",                       VMM_FN(PDMCritSectLeave),               &g_aArgsPTR[0],                             RT_ELEMENTS(g_aArgsPTR),                               REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
#  ifdef VBOX_STRICT
    { "PDMCritSectEnterDebug",                  VMM_FN(PDMCritSectEnterDebug),          &g_aArgsPDMCritSectEnterDebug[0],           RT_ELEMENTS(g_aArgsPDMCritSectEnterDebug),             REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
#  endif
    { "PDMGetInterrupt",                        VMM_FN(PDMGetInterrupt),                &g_aArgsPDMGetInterrupt[0],                 RT_ELEMENTS(g_aArgsPDMGetInterrupt),                   REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PDMIsaSetIrq",                           VMM_FN(PDMIsaSetIrq),                   &g_aArgsPDMIsaSetIrq[0],                    RT_ELEMENTS(g_aArgsPDMIsaSetIrq),                      REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMGetGuestMode",                        VMM_FN(PGMGetGuestMode),                &g_aArgsPGMGetGuestMode[0],                 RT_ELEMENTS(g_aArgsPGMGetGuestMode),                   REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMGstGetPage",                          VMM_FN(PGMGstGetPage),                  &g_aArgsPGMGstGetPage[0],                   RT_ELEMENTS(g_aArgsPGMGstGetPage),                     REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMInvalidatePage",                      VMM_FN(PGMInvalidatePage),              &g_aArgsPGMInvalidatePage[0],               RT_ELEMENTS(g_aArgsPGMInvalidatePage),                 REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMPhysIsGCPhysValid",                   VMM_FN(PGMPhysIsGCPhysValid),           &g_aArgsPGMPhysIsGCPhysValid[0],            RT_ELEMENTS(g_aArgsPGMPhysIsGCPhysValid),              REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "PGMPhysIsA20Enabled",                    VMM_FN(PGMPhysIsA20Enabled),            &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "PGMPhysRead",                            VMM_FN(PGMPhysRead),                    &g_aArgsPGMPhysRead[0],                     RT_ELEMENTS(g_aArgsPGMPhysRead),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMPhysSimpleReadGCPtr",                 VMM_FN(PGMPhysSimpleReadGCPtr),         &g_aArgsPGMPhysSimpleReadGCPtr[0],          RT_ELEMENTS(g_aArgsPGMPhysSimpleReadGCPtr),            REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMPhysWrite",                           VMM_FN(PGMPhysWrite),                   &g_aArgsPGMPhysWrite[0],                    RT_ELEMENTS(g_aArgsPGMPhysWrite),                      REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PGMChangeMode",                          VMM_FN(PGMChangeMode),                  &g_aArgsPGMChangeMode[0],                   RT_ELEMENTS(g_aArgsPGMChangeMode),                     REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMFlushTLB",                            VMM_FN(PGMFlushTLB),                    &g_aArgsPGMFlushTLB[0],                     RT_ELEMENTS(g_aArgsPGMFlushTLB),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMCr0WpEnabled",                        VMM_FN(PGMCr0WpEnabled),                &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PGMR3PhysReadU8",                        VMM_FN(PGMR3PhysReadU8),                &g_aArgsPGMR3PhysReadUxx[0],                RT_ELEMENTS(g_aArgsPGMR3PhysReadUxx),                  REMFNDESC_FLAGS_RET_INT,    sizeof(uint8_t),    NULL },
    { "PGMR3PhysReadU16",                       VMM_FN(PGMR3PhysReadU16),               &g_aArgsPGMR3PhysReadUxx[0],                RT_ELEMENTS(g_aArgsPGMR3PhysReadUxx),                  REMFNDESC_FLAGS_RET_INT,    sizeof(uint16_t),   NULL },
    { "PGMR3PhysReadU32",                       VMM_FN(PGMR3PhysReadU32),               &g_aArgsPGMR3PhysReadUxx[0],                RT_ELEMENTS(g_aArgsPGMR3PhysReadUxx),                  REMFNDESC_FLAGS_RET_INT,    sizeof(uint32_t),   NULL },
    { "PGMR3PhysReadU64",                       VMM_FN(PGMR3PhysReadU64),               &g_aArgsPGMR3PhysReadUxx[0],                RT_ELEMENTS(g_aArgsPGMR3PhysReadUxx),                  REMFNDESC_FLAGS_RET_INT,    sizeof(uint64_t),   NULL },
    { "PGMR3PhysWriteU8",                       VMM_FN(PGMR3PhysWriteU8),               &g_aArgsPGMR3PhysWriteU8[0],                RT_ELEMENTS(g_aArgsPGMR3PhysWriteU8),                  REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PGMR3PhysWriteU16",                      VMM_FN(PGMR3PhysWriteU16),              &g_aArgsPGMR3PhysWriteU16[0],               RT_ELEMENTS(g_aArgsPGMR3PhysWriteU16),                 REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PGMR3PhysWriteU32",                      VMM_FN(PGMR3PhysWriteU32),              &g_aArgsPGMR3PhysWriteU32[0],               RT_ELEMENTS(g_aArgsPGMR3PhysWriteU32),                 REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PGMR3PhysWriteU64",                      VMM_FN(PGMR3PhysWriteU64),              &g_aArgsPGMR3PhysWriteU64[0],               RT_ELEMENTS(g_aArgsPGMR3PhysWriteU32),                 REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "PGMR3PhysTlbGCPhys2Ptr",                 VMM_FN(PGMR3PhysTlbGCPhys2Ptr),         &g_aArgsPGMR3PhysTlbGCPhys2Ptr[0],          RT_ELEMENTS(g_aArgsPGMR3PhysTlbGCPhys2Ptr),            REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "PGMIsLockOwner",                         VMM_FN(PGMIsLockOwner),                 &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "SSMR3GetGCPtr",                          VMM_FN(SSMR3GetGCPtr),                  &g_aArgsSSMR3GetGCPtr[0],                   RT_ELEMENTS(g_aArgsSSMR3GetGCPtr),                     REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3GetMem",                            VMM_FN(SSMR3GetMem),                    &g_aArgsSSMR3GetMem[0],                     RT_ELEMENTS(g_aArgsSSMR3GetMem),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3GetU32",                            VMM_FN(SSMR3GetU32),                    &g_aArgsSSMR3GetU32[0],                     RT_ELEMENTS(g_aArgsSSMR3GetU32),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3GetUInt",                           VMM_FN(SSMR3GetUInt),                   &g_aArgsSSMR3GetUInt[0],                    RT_ELEMENTS(g_aArgsSSMR3GetUInt),                      REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3PutGCPtr",                          VMM_FN(SSMR3PutGCPtr),                  &g_aArgsSSMR3PutGCPtr[0],                   RT_ELEMENTS(g_aArgsSSMR3PutGCPtr),                     REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3PutMem",                            VMM_FN(SSMR3PutMem),                    &g_aArgsSSMR3PutMem[0],                     RT_ELEMENTS(g_aArgsSSMR3PutMem),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3PutU32",                            VMM_FN(SSMR3PutU32),                    &g_aArgsSSMR3PutU32[0],                     RT_ELEMENTS(g_aArgsSSMR3PutU32),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3PutUInt",                           VMM_FN(SSMR3PutUInt),                   &g_aArgsSSMR3PutUInt[0],                    RT_ELEMENTS(g_aArgsSSMR3PutUInt),                      REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "SSMR3RegisterInternal",                  VMM_FN(SSMR3RegisterInternal),          &g_aArgsSSMR3RegisterInternal[0],           RT_ELEMENTS(g_aArgsSSMR3RegisterInternal),             REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "STAMR3Register",                         VMM_FN(STAMR3Register),                 &g_aArgsSTAMR3Register[0],                  RT_ELEMENTS(g_aArgsSTAMR3Register),                    REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "STAMR3Deregister",                       VMM_FN(STAMR3Deregister),               &g_aArgsSTAMR3Deregister[0],                RT_ELEMENTS(g_aArgsSTAMR3Deregister),                  REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "TMCpuTickGet",                           VMM_FN(TMCpuTickGet),                   &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(uint64_t),   NULL },
    { "TMR3NotifySuspend",                      VMM_FN(TMR3NotifySuspend),              &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "TMR3NotifyResume",                       VMM_FN(TMR3NotifyResume),               &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "TMNotifyEndOfExecution",                 VMM_FN(TMNotifyEndOfExecution),         &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "TMNotifyStartOfExecution",               VMM_FN(TMNotifyStartOfExecution),       &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "TMTimerPollBool",                        VMM_FN(TMTimerPollBool),                &g_aArgsVMandVMCPU[0],                      RT_ELEMENTS(g_aArgsVMandVMCPU),                        REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "TMR3TimerQueuesDo",                      VMM_FN(TMR3TimerQueuesDo),              &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "TRPMAssertTrap",                         VMM_FN(TRPMAssertTrap),                 &g_aArgsTRPMAssertTrap[0],                  RT_ELEMENTS(g_aArgsTRPMAssertTrap),                    REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "TRPMGetErrorCode",                       VMM_FN(TRPMGetErrorCode),               &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(RTGCUINT),   NULL },
    { "TRPMGetFaultAddress",                    VMM_FN(TRPMGetFaultAddress),            &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(RTGCUINTPTR),NULL },
    { "TRPMQueryTrap",                          VMM_FN(TRPMQueryTrap),                  &g_aArgsTRPMQueryTrap[0],                   RT_ELEMENTS(g_aArgsTRPMQueryTrap),                     REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "TRPMResetTrap",                          VMM_FN(TRPMResetTrap),                  &g_aArgsVMCPU[0],                           RT_ELEMENTS(g_aArgsVMCPU),                             REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "TRPMSetErrorCode",                       VMM_FN(TRPMSetErrorCode),               &g_aArgsTRPMSetErrorCode[0],                RT_ELEMENTS(g_aArgsTRPMSetErrorCode),                  REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "TRPMSetFaultAddress",                    VMM_FN(TRPMSetFaultAddress),            &g_aArgsTRPMSetFaultAddress[0],             RT_ELEMENTS(g_aArgsTRPMSetFaultAddress),               REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "VMMGetCpu",                              VMM_FN(VMMGetCpu),                      &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(PVMCPU),     NULL },
    { "VMR3ReqPriorityCallWait",                VMM_FN(VMR3ReqPriorityCallWait),        &g_aArgsVMR3ReqCallWait[0],                 RT_ELEMENTS(g_aArgsVMR3ReqCallWait),                   REMFNDESC_FLAGS_RET_INT | REMFNDESC_FLAGS_ELLIPSIS, sizeof(int), NULL },
    { "VMR3ReqFree",                            VMM_FN(VMR3ReqFree),                    &g_aArgsVMR3ReqFree[0],                     RT_ELEMENTS(g_aArgsVMR3ReqFree),                       REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
//    { "",                        VMM_FN(),                &g_aArgsVM[0],                              RT_ELEMENTS(g_aArgsVM),                                REMFNDESC_FLAGS_RET_INT,    sizeof(int),   NULL },
};


/**
 * Descriptors for the functions imported from VBoxRT.
 */
static REMFNDESC g_aRTImports[] =
{
    { "RTAssertMsg1",                           (void *)(uintptr_t)&RTAssertMsg1,                   &g_aArgsRTAssertMsg1[0],                    RT_ELEMENTS(g_aArgsRTAssertMsg1),                      REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTAssertMsg1Weak",                       (void *)(uintptr_t)&RTAssertMsg1Weak,               &g_aArgsRTAssertMsg1[0],                    RT_ELEMENTS(g_aArgsRTAssertMsg1),                      REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTAssertMsg2",                           (void *)(uintptr_t)&RTAssertMsg2,                   &g_aArgsRTAssertMsg2[0],                    RT_ELEMENTS(g_aArgsRTAssertMsg2),                      REMFNDESC_FLAGS_RET_VOID | REMFNDESC_FLAGS_ELLIPSIS, 0, NULL },
    { "RTAssertMsg2V",                          (void *)(uintptr_t)&RTAssertMsg2V,                  &g_aArgsRTAssertMsg2V[0],                   RT_ELEMENTS(g_aArgsRTAssertMsg2V),                     REMFNDESC_FLAGS_RET_VOID | REMFNDESC_FLAGS_VALIST, 0, NULL },
    { "RTAssertMsg2Weak",                       (void *)(uintptr_t)&RTAssertMsg2Weak,               &g_aArgsRTAssertMsg2[0],                    RT_ELEMENTS(g_aArgsRTAssertMsg2),                      REMFNDESC_FLAGS_RET_VOID | REMFNDESC_FLAGS_ELLIPSIS, 0, NULL },
    { "RTAssertShouldPanic",                    (void *)(uintptr_t)&RTAssertShouldPanic,            NULL,                                       0,                                                     REMFNDESC_FLAGS_RET_INT,    sizeof(bool),       NULL },
    { "RTLogDefaultInstance",                   (void *)(uintptr_t)&RTLogDefaultInstance,           NULL,                                       0,                                                     REMFNDESC_FLAGS_RET_INT,    sizeof(PRTLOGGER),  NULL },
    { "RTLogRelGetDefaultInstance",             (void *)(uintptr_t)&RTLogRelGetDefaultInstance,     NULL,                                       0,                                                     REMFNDESC_FLAGS_RET_INT,    sizeof(PRTLOGGER),  NULL },
    { "RTLogDefaultInstanceEx",                 (void *)(uintptr_t)&RTLogDefaultInstance,           &g_aArgsRTLogGetDefaultInstanceEx[0],       RT_ELEMENTS(g_aArgsRTLogGetDefaultInstanceEx),         REMFNDESC_FLAGS_RET_INT,    sizeof(PRTLOGGER),  NULL },
    { "RTLogRelGetDefaultInstanceEx",           (void *)(uintptr_t)&RTLogRelGetDefaultInstance,     &g_aArgsRTLogGetDefaultInstanceEx[0],       RT_ELEMENTS(g_aArgsRTLogGetDefaultInstanceEx),         REMFNDESC_FLAGS_RET_INT,    sizeof(PRTLOGGER),  NULL },
    { "RTLogFlags",                             (void *)(uintptr_t)&RTLogFlags,                     &g_aArgsRTLogFlags[0],                      RT_ELEMENTS(g_aArgsRTLogFlags),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "RTLogFlush",                             (void *)(uintptr_t)&RTLogFlush,                     &g_aArgsRTLogFlush[0],                      RT_ELEMENTS(g_aArgsRTLogFlush),                        REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "RTLogLoggerEx",                          (void *)(uintptr_t)&RTLogLoggerEx,                  &g_aArgsRTLogLoggerEx[0],                   RT_ELEMENTS(g_aArgsRTLogLoggerEx),                     REMFNDESC_FLAGS_RET_VOID | REMFNDESC_FLAGS_ELLIPSIS, 0, NULL },
    { "RTLogLoggerExV",                         (void *)(uintptr_t)&RTLogLoggerExV,                 &g_aArgsRTLogLoggerExV[0],                  RT_ELEMENTS(g_aArgsRTLogLoggerExV),                    REMFNDESC_FLAGS_RET_VOID | REMFNDESC_FLAGS_VALIST, 0, NULL },
    { "RTLogPrintf",                            (void *)(uintptr_t)&RTLogPrintf,                    &g_aArgsRTLogPrintf[0],                     RT_ELEMENTS(g_aArgsRTLogPrintf),                       REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTLogRelPrintf",                         (void *)(uintptr_t)&RTLogRelPrintf,                 &g_aArgsRTLogPrintf[0],                     RT_ELEMENTS(g_aArgsRTLogPrintf),                       REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTMemAllocTag",                          (void *)(uintptr_t)&RTMemAllocTag,                  &g_aArgsSIZE_TTag[0],                       RT_ELEMENTS(g_aArgsSIZE_TTag),                         REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemAllocZTag",                         (void *)(uintptr_t)&RTMemAllocZTag,                 &g_aArgsSIZE_TTag[0],                       RT_ELEMENTS(g_aArgsSIZE_TTag),                         REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemReallocTag",                        (void *)(uintptr_t)&RTMemReallocTag,                &g_aArgsRTMemReallocTag[0],                 RT_ELEMENTS(g_aArgsRTMemReallocTag),                   REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemExecAllocTag",                      (void *)(uintptr_t)&RTMemExecAllocTag,              &g_aArgsSIZE_TTag[0],                       RT_ELEMENTS(g_aArgsSIZE_TTag),                         REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemExecFree",                          (void *)(uintptr_t)&RTMemExecFree,                  &g_aArgsPTR_SIZE_T[0],                      RT_ELEMENTS(g_aArgsPTR_SIZE_T),                        REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTMemFree",                              (void *)(uintptr_t)&RTMemFree,                      &g_aArgsPTR[0],                             RT_ELEMENTS(g_aArgsPTR),                               REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTMemPageAllocTag",                      (void *)(uintptr_t)&RTMemPageAllocTag,              &g_aArgsSIZE_TTag[0],                       RT_ELEMENTS(g_aArgsSIZE_TTag),                         REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemPageFree",                          (void *)(uintptr_t)&RTMemPageFree,                  &g_aArgsPTR_SIZE_T[0],                      RT_ELEMENTS(g_aArgsPTR_SIZE_T),                        REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTMemProtect",                           (void *)(uintptr_t)&RTMemProtect,                   &g_aArgsRTMemProtect[0],                    RT_ELEMENTS(g_aArgsRTMemProtect),                      REMFNDESC_FLAGS_RET_INT,    sizeof(int),        NULL },
    { "RTMemEfAlloc",                           (void *)(uintptr_t)&RTMemEfAlloc,                   &g_aArgsSIZE_TTagLoc[0],                    RT_ELEMENTS(g_aArgsSIZE_TTagLoc),                      REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemEfAllocZ",                          (void *)(uintptr_t)&RTMemEfAllocZ,                  &g_aArgsSIZE_TTagLoc[0],                    RT_ELEMENTS(g_aArgsSIZE_TTagLoc),                      REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemEfRealloc",                         (void *)(uintptr_t)&RTMemEfRealloc,                 &g_aArgsRTMemEfRealloc[0],                  RT_ELEMENTS(g_aArgsRTMemEfRealloc),                    REMFNDESC_FLAGS_RET_INT,    sizeof(void *),     NULL },
    { "RTMemEfFree",                            (void *)(uintptr_t)&RTMemEfFree,                    &g_aArgsPTRLoc[0],                          RT_ELEMENTS(g_aArgsPTRLoc),                            REMFNDESC_FLAGS_RET_VOID,   0,                  NULL },
    { "RTStrPrintf",                            (void *)(uintptr_t)&RTStrPrintf,                    &g_aArgsRTStrPrintf[0],                     RT_ELEMENTS(g_aArgsRTStrPrintf),                       REMFNDESC_FLAGS_RET_INT | REMFNDESC_FLAGS_ELLIPSIS, sizeof(size_t), NULL },
    { "RTStrPrintfV",                           (void *)(uintptr_t)&RTStrPrintfV,                   &g_aArgsRTStrPrintfV[0],                    RT_ELEMENTS(g_aArgsRTStrPrintfV),                      REMFNDESC_FLAGS_RET_INT | REMFNDESC_FLAGS_VALIST, sizeof(size_t), NULL },
    { "RTThreadSelf",                           (void *)(uintptr_t)&RTThreadSelf,                   NULL,                                       0,                                                     REMFNDESC_FLAGS_RET_INT,    sizeof(RTTHREAD),    NULL },
    { "RTThreadNativeSelf",                     (void *)(uintptr_t)&RTThreadNativeSelf,             NULL,                                       0,                                                     REMFNDESC_FLAGS_RET_INT, sizeof(RTNATIVETHREAD), NULL },
    { "RTLockValidatorWriteLockGetCount",       (void *)(uintptr_t)&RTLockValidatorWriteLockGetCount, &g_aArgsThread[0],                        0,                                                     REMFNDESC_FLAGS_RET_INT,    sizeof(int32_t),     NULL },
};


/**
 * Descriptors for the functions imported from VBoxRT.
 */
static REMFNDESC g_aCRTImports[] =
{
    { "memcpy",                                (void *)(uintptr_t)&memcpy,                          &g_aArgsmemcpy[0],                          RT_ELEMENTS(g_aArgsmemcpy),                            REMFNDESC_FLAGS_RET_INT,    sizeof(void *), NULL },
    { "memset",                                (void *)(uintptr_t)&memset,                          &g_aArgsmemset[0],                          RT_ELEMENTS(g_aArgsmemset),                            REMFNDESC_FLAGS_RET_INT,    sizeof(void *), NULL }
/*
floor               floor
memcpy              memcpy
sqrt                sqrt
sqrtf               sqrtf
*/
};


#  if defined(USE_REM_CALLING_CONVENTION_GLUE) || defined(USE_REM_IMPORT_JUMP_GLUE)
/** LIFO of read-write-executable memory chunks used for wrappers. */
static PREMEXECMEM g_pExecMemHead;
#  endif
# endif /* !VBOX_USE_BITNESS_SELECTOR */



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
# ifndef VBOX_USE_BITNESS_SELECTOR
static int remGenerateExportGlue(PRTUINTPTR pValue, PCREMFNDESC pDesc);

#  ifdef USE_REM_CALLING_CONVENTION_GLUE
DECLASM(int) WrapGCC2MSC0Int(void);  DECLASM(int) WrapGCC2MSC0Int_EndProc(void);
DECLASM(int) WrapGCC2MSC1Int(void);  DECLASM(int) WrapGCC2MSC1Int_EndProc(void);
DECLASM(int) WrapGCC2MSC2Int(void);  DECLASM(int) WrapGCC2MSC2Int_EndProc(void);
DECLASM(int) WrapGCC2MSC3Int(void);  DECLASM(int) WrapGCC2MSC3Int_EndProc(void);
DECLASM(int) WrapGCC2MSC4Int(void);  DECLASM(int) WrapGCC2MSC4Int_EndProc(void);
DECLASM(int) WrapGCC2MSC5Int(void);  DECLASM(int) WrapGCC2MSC5Int_EndProc(void);
DECLASM(int) WrapGCC2MSC6Int(void);  DECLASM(int) WrapGCC2MSC6Int_EndProc(void);
DECLASM(int) WrapGCC2MSC7Int(void);  DECLASM(int) WrapGCC2MSC7Int_EndProc(void);
DECLASM(int) WrapGCC2MSC8Int(void);  DECLASM(int) WrapGCC2MSC8Int_EndProc(void);
DECLASM(int) WrapGCC2MSC9Int(void);  DECLASM(int) WrapGCC2MSC9Int_EndProc(void);
DECLASM(int) WrapGCC2MSC10Int(void); DECLASM(int) WrapGCC2MSC10Int_EndProc(void);
DECLASM(int) WrapGCC2MSC11Int(void); DECLASM(int) WrapGCC2MSC11Int_EndProc(void);
DECLASM(int) WrapGCC2MSC12Int(void); DECLASM(int) WrapGCC2MSC12Int_EndProc(void);
DECLASM(int) WrapGCC2MSCVariadictInt(void); DECLASM(int) WrapGCC2MSCVariadictInt_EndProc(void);
DECLASM(int) WrapGCC2MSC_SSMR3RegisterInternal(void); DECLASM(int) WrapGCC2MSC_SSMR3RegisterInternal_EndProc(void);

DECLASM(int) WrapMSC2GCC0Int(void);  DECLASM(int) WrapMSC2GCC0Int_EndProc(void);
DECLASM(int) WrapMSC2GCC1Int(void);  DECLASM(int) WrapMSC2GCC1Int_EndProc(void);
DECLASM(int) WrapMSC2GCC2Int(void);  DECLASM(int) WrapMSC2GCC2Int_EndProc(void);
DECLASM(int) WrapMSC2GCC3Int(void);  DECLASM(int) WrapMSC2GCC3Int_EndProc(void);
DECLASM(int) WrapMSC2GCC4Int(void);  DECLASM(int) WrapMSC2GCC4Int_EndProc(void);
DECLASM(int) WrapMSC2GCC5Int(void);  DECLASM(int) WrapMSC2GCC5Int_EndProc(void);
DECLASM(int) WrapMSC2GCC6Int(void);  DECLASM(int) WrapMSC2GCC6Int_EndProc(void);
DECLASM(int) WrapMSC2GCC7Int(void);  DECLASM(int) WrapMSC2GCC7Int_EndProc(void);
DECLASM(int) WrapMSC2GCC8Int(void);  DECLASM(int) WrapMSC2GCC8Int_EndProc(void);
DECLASM(int) WrapMSC2GCC9Int(void);  DECLASM(int) WrapMSC2GCC9Int_EndProc(void);
#  endif


#  if defined(USE_REM_CALLING_CONVENTION_GLUE) || defined(USE_REM_IMPORT_JUMP_GLUE)
/**
 * Allocates a block of memory for glue code.
 *
 * The returned memory is padded with INT3s.
 *
 * @returns Pointer to the allocated memory.
 * @param   The amount of memory to allocate.
 */
static void *remAllocGlue(size_t cb)
{
    PREMEXECMEM pCur = g_pExecMemHead;
    uint32_t cbAligned = (uint32_t)RT_ALIGN_32(cb, 32);
    while (pCur)
    {
        if (pCur->cb - pCur->off >= cbAligned)
        {
            void *pv = (uint8_t *)pCur + pCur->off;
            pCur->off += cbAligned;
            return memset(pv, 0xcc, cbAligned);
        }
        pCur = pCur->pNext;
    }

    /* add a new chunk */
    AssertReturn(_64K - RT_ALIGN_Z(sizeof(*pCur), 32) > cbAligned, NULL);
    pCur = (PREMEXECMEM)RTMemExecAlloc(_64K);
    AssertReturn(pCur, NULL);
    pCur->cb = _64K;
    pCur->off = RT_ALIGN_32(sizeof(*pCur), 32) + cbAligned;
    pCur->pNext = g_pExecMemHead;
    g_pExecMemHead = pCur;
    return memset((uint8_t *)pCur + RT_ALIGN_Z(sizeof(*pCur), 32), 0xcc, cbAligned);
}
#  endif /* USE_REM_CALLING_CONVENTION_GLUE || USE_REM_IMPORT_JUMP_GLUE */


#  ifdef USE_REM_CALLING_CONVENTION_GLUE
/**
 * Checks if a function is all straight forward integers.
 *
 * @returns True if it's simple, false if it's bothersome.
 * @param   pDesc       The function descriptor.
 */
static bool remIsFunctionAllInts(PCREMFNDESC pDesc)
{
    if (    (   (pDesc->fFlags & REMFNDESC_FLAGS_RET_TYPE_MASK) != REMFNDESC_FLAGS_RET_INT
             || pDesc->cbReturn > sizeof(uint64_t))
        &&  (pDesc->fFlags & REMFNDESC_FLAGS_RET_TYPE_MASK) != REMFNDESC_FLAGS_RET_VOID)
        return false;
    unsigned i = pDesc->cParams;
    while (i-- > 0)
        switch (pDesc->paParams[i].fFlags & REMPARMDESC_FLAGS_TYPE_MASK)
        {
            case REMPARMDESC_FLAGS_INT:
            case REMPARMDESC_FLAGS_GCPTR:
            case REMPARMDESC_FLAGS_GCPHYS:
            case REMPARMDESC_FLAGS_HCPHYS:
                break;

            default:
                AssertReleaseMsgFailed(("Invalid param flags %#x for #%d of %s!\n", pDesc->paParams[i].fFlags, i, pDesc->pszName));
            case REMPARMDESC_FLAGS_VALIST:
            case REMPARMDESC_FLAGS_ELLIPSIS:
            case REMPARMDESC_FLAGS_FLOAT:
            case REMPARMDESC_FLAGS_STRUCT:
            case REMPARMDESC_FLAGS_PFN:
                return false;
        }
    return true;
}


/**
 * Checks if the function has an ellipsis (...) argument.
 *
 * @returns true if it has an ellipsis, otherwise false.
 * @param   pDesc       The function descriptor.
 */
static bool remHasFunctionEllipsis(PCREMFNDESC pDesc)
{
    unsigned i = pDesc->cParams;
    while (i-- > 0)
        if ((pDesc->paParams[i].fFlags & REMPARMDESC_FLAGS_TYPE_MASK) == REMPARMDESC_FLAGS_ELLIPSIS)
            return true;
    return false;
}


/**
 * Checks if the function uses floating point (FP) arguments or return value.
 *
 * @returns true if it uses floating point, otherwise false.
 * @param   pDesc       The function descriptor.
 */
static bool remIsFunctionUsingFP(PCREMFNDESC pDesc)
{
    if ((pDesc->fFlags & REMFNDESC_FLAGS_RET_TYPE_MASK) == REMFNDESC_FLAGS_RET_FLOAT)
        return true;
    unsigned i = pDesc->cParams;
    while (i-- > 0)
        if ((pDesc->paParams[i].fFlags & REMPARMDESC_FLAGS_TYPE_MASK) == REMPARMDESC_FLAGS_FLOAT)
            return true;
    return false;
}


/** @name The export and import fixups.
 * @{ */
#   define REM_FIXUP_32_REAL_STUFF    UINT32_C(0xdeadbeef)
#   define REM_FIXUP_64_REAL_STUFF    UINT64_C(0xdeadf00df00ddead)
#   define REM_FIXUP_64_DESC          UINT64_C(0xdead00010001dead)
#   define REM_FIXUP_64_LOG_ENTRY     UINT64_C(0xdead00020002dead)
#   define REM_FIXUP_64_LOG_EXIT      UINT64_C(0xdead00030003dead)
#   define REM_FIXUP_64_WRAP_GCC_CB   UINT64_C(0xdead00040004dead)
/** @} */


/**
 * Entry logger function.
 *
 * @param   pDesc       The description.
 */
DECLASM(void) remLogEntry(PCREMFNDESC pDesc)
{
    RTPrintf("calling %s\n", pDesc->pszName);
}


/**
 * Exit logger function.
 *
 * @param   pDesc       The description.
 * @param   pvRet       The return code.
 */
DECLASM(void) remLogExit(PCREMFNDESC pDesc, void *pvRet)
{
    RTPrintf("returning %p from %s\n", pvRet, pDesc->pszName);
}


/**
 * Creates a wrapper for the specified callback function at run time.
 *
 * @param   pDesc       The function descriptor.
 * @param   pValue      Upon entry *pValue contains the address of the function to be wrapped.
 *                      Upon return *pValue contains the address of the wrapper glue function.
 * @param   iParam      The parameter index in the function descriptor (0 based).
 *                      If UINT32_MAX pDesc is the descriptor for *pValue.
 */
DECLASM(void) remWrapGCCCallback(PCREMFNDESC pDesc, PRTUINTPTR pValue, uint32_t iParam)
{
    AssertPtr(pDesc);
    AssertPtr(pValue);

    /*
     * Simple?
     */
    if (!*pValue)
        return;

    /*
     * Locate the right function descriptor.
     */
    if (iParam != UINT32_MAX)
    {
        AssertRelease(iParam < pDesc->cParams);
        pDesc = (PCREMFNDESC)pDesc->paParams[iParam].pvExtra;
        AssertPtr(pDesc);
    }

    /*
     * When we get serious, here is where to insert the hash table lookup.
     */

    /*
     * Create a new glue patch.
     */
#   ifdef RT_OS_WINDOWS
    int rc = remGenerateExportGlue(pValue, pDesc);
#   else
#    error "port me"
#   endif
    AssertReleaseRC(rc);

    /*
     * Add it to the hash (later)
     */
}


/**
 * Fixes export glue.
 *
 * @param   pvGlue      The glue code.
 * @param   cb          The size of the glue code.
 * @param   pvExport    The address of the export we're wrapping.
 * @param   pDesc       The export descriptor.
 */
static void remGenerateExportGlueFixup(void *pvGlue, size_t cb, uintptr_t uExport, PCREMFNDESC pDesc)
{
    union
    {
        uint8_t  *pu8;
        int32_t  *pi32;
        uint32_t *pu32;
        uint64_t *pu64;
        void     *pv;
    } u;
    u.pv = pvGlue;

    while (cb >= 4)
    {
        /** @todo add defines for the fixup constants... */
        if (*u.pu32 == REM_FIXUP_32_REAL_STUFF)
        {
            /* 32-bit rel jmp/call to real export. */
            *u.pi32 = uExport - (uintptr_t)(u.pi32 + 1);
            Assert((uintptr_t)(u.pi32 + 1) + *u.pi32 == uExport);
            u.pi32++;
            cb -= 4;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_REAL_STUFF)
        {
            /* 64-bit address to the real export. */
            *u.pu64++ = uExport;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_DESC)
        {
            /* 64-bit address to the descriptor. */
            *u.pu64++ = (uintptr_t)pDesc;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_WRAP_GCC_CB)
        {
            /* 64-bit address to the entry logger function. */
            *u.pu64++ = (uintptr_t)remWrapGCCCallback;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_LOG_ENTRY)
        {
            /* 64-bit address to the entry logger function. */
            *u.pu64++ = (uintptr_t)remLogEntry;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_LOG_EXIT)
        {
            /* 64-bit address to the entry logger function. */
            *u.pu64++ = (uintptr_t)remLogExit;
            cb -= 8;
            continue;
        }

        /* move on. */
        u.pu8++;
        cb--;
    }
}


/**
 * Fixes import glue.
 *
 * @param   pvGlue  The glue code.
 * @param   cb      The size of the glue code.
 * @param   pDesc   The import descriptor.
 */
static void remGenerateImportGlueFixup(void *pvGlue, size_t cb, PCREMFNDESC pDesc)
{
    union
    {
        uint8_t  *pu8;
        int32_t  *pi32;
        uint32_t *pu32;
        uint64_t *pu64;
        void     *pv;
    } u;
    u.pv = pvGlue;

    while (cb >= 4)
    {
        if (*u.pu32 == REM_FIXUP_32_REAL_STUFF)
        {
            /* 32-bit rel jmp/call to real function. */
            *u.pi32 = (uintptr_t)pDesc->pv - (uintptr_t)(u.pi32 + 1);
            Assert((uintptr_t)(u.pi32 + 1) + *u.pi32 == (uintptr_t)pDesc->pv);
            u.pi32++;
            cb -= 4;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_REAL_STUFF)
        {
            /* 64-bit address to the real function. */
            *u.pu64++ = (uintptr_t)pDesc->pv;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_DESC)
        {
            /* 64-bit address to the descriptor. */
            *u.pu64++ = (uintptr_t)pDesc;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_WRAP_GCC_CB)
        {
            /* 64-bit address to the entry logger function. */
            *u.pu64++ = (uintptr_t)remWrapGCCCallback;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_LOG_ENTRY)
        {
            /* 64-bit address to the entry logger function. */
            *u.pu64++ = (uintptr_t)remLogEntry;
            cb -= 8;
            continue;
        }
        if (cb >= 8 && *u.pu64 == REM_FIXUP_64_LOG_EXIT)
        {
            /* 64-bit address to the entry logger function. */
            *u.pu64++ = (uintptr_t)remLogExit;
            cb -= 8;
            continue;
        }

        /* move on. */
        u.pu8++;
        cb--;
    }
}

#  endif /* USE_REM_CALLING_CONVENTION_GLUE */


/**
 * Generate wrapper glue code for an export.
 *
 * This is only used on win64 when loading a 64-bit linux module. So, on other
 * platforms it will not do anything.
 *
 * @returns VBox status code.
 * @param   pValue      IN: Where to get the address of the function to wrap.
 *                      OUT: Where to store the glue address.
 * @param   pDesc       The export descriptor.
 */
static int remGenerateExportGlue(PRTUINTPTR pValue, PCREMFNDESC pDesc)
{
#  ifdef USE_REM_CALLING_CONVENTION_GLUE
    uintptr_t *ppfn = (uintptr_t *)pDesc->pv;

    uintptr_t pfn = 0; /* a little hack for the callback glue */
    if (!ppfn)
        ppfn = &pfn;

    if (!*ppfn)
    {
        if (remIsFunctionAllInts(pDesc))
        {
            static const struct { void *pvStart, *pvEnd; } s_aTemplates[] =
            {
                { (void *)&WrapMSC2GCC0Int,  (void *)&WrapMSC2GCC0Int_EndProc },
                { (void *)&WrapMSC2GCC1Int,  (void *)&WrapMSC2GCC1Int_EndProc },
                { (void *)&WrapMSC2GCC2Int,  (void *)&WrapMSC2GCC2Int_EndProc },
                { (void *)&WrapMSC2GCC3Int,  (void *)&WrapMSC2GCC3Int_EndProc },
                { (void *)&WrapMSC2GCC4Int,  (void *)&WrapMSC2GCC4Int_EndProc },
                { (void *)&WrapMSC2GCC5Int,  (void *)&WrapMSC2GCC5Int_EndProc },
                { (void *)&WrapMSC2GCC6Int,  (void *)&WrapMSC2GCC6Int_EndProc },
                { (void *)&WrapMSC2GCC7Int,  (void *)&WrapMSC2GCC7Int_EndProc },
                { (void *)&WrapMSC2GCC8Int,  (void *)&WrapMSC2GCC8Int_EndProc },
                { (void *)&WrapMSC2GCC9Int,  (void *)&WrapMSC2GCC9Int_EndProc },
            };
            const unsigned i = pDesc->cParams;
            AssertReleaseMsg(i < RT_ELEMENTS(s_aTemplates), ("%d (%s)\n", i, pDesc->pszName));

            /* duplicate the patch. */
            const size_t cb = (uintptr_t)s_aTemplates[i].pvEnd - (uintptr_t)s_aTemplates[i].pvStart;
            uint8_t *pb = (uint8_t *)remAllocGlue(cb);
            AssertReturn(pb, VERR_NO_MEMORY);
            memcpy(pb, s_aTemplates[i].pvStart, cb);

            /* fix it up. */
            remGenerateExportGlueFixup(pb, cb, *pValue, pDesc);
            *ppfn = (uintptr_t)pb;
        }
        else
        {
            /* custom hacks - it's simpler to make assembly templates than writing a more generic code generator... */
            static const struct { const char *pszName; PFNRT pvStart, pvEnd; } s_aTemplates[] =
            {
                { "somefunction",  (PFNRT)&WrapMSC2GCC9Int,  (PFNRT)&WrapMSC2GCC9Int_EndProc },
            };
            unsigned i;
            for (i = 0; i < RT_ELEMENTS(s_aTemplates); i++)
                if (!strcmp(pDesc->pszName, s_aTemplates[i].pszName))
                    break;
            AssertReleaseMsgReturn(i < RT_ELEMENTS(s_aTemplates), ("Not implemented! %s\n", pDesc->pszName), VERR_NOT_IMPLEMENTED);

            /* duplicate the patch. */
            const size_t cb = (uintptr_t)s_aTemplates[i].pvEnd - (uintptr_t)s_aTemplates[i].pvStart;
            uint8_t *pb = (uint8_t *)remAllocGlue(cb);
            AssertReturn(pb, VERR_NO_MEMORY);
            memcpy(pb, s_aTemplates[i].pvStart, cb);

            /* fix it up. */
            remGenerateExportGlueFixup(pb, cb, *pValue, pDesc);
            *ppfn = (uintptr_t)pb;
        }
    }
    *pValue = *ppfn;
    return VINF_SUCCESS;
#  else  /* !USE_REM_CALLING_CONVENTION_GLUE */
    return VINF_SUCCESS;
#  endif /* !USE_REM_CALLING_CONVENTION_GLUE */
}


/**
 * Generate wrapper glue code for an import.
 *
 * This is only used on win64 when loading a 64-bit linux module. So, on other
 * platforms it will simply return the address of the imported function
 * without generating any glue code.
 *
 * @returns VBox status code.
 * @param   pValue      Where to store the glue address.
 * @param   pDesc       The export descriptor.
 */
static int remGenerateImportGlue(PRTUINTPTR pValue, PREMFNDESC pDesc)
{
#  if defined(USE_REM_CALLING_CONVENTION_GLUE) || defined(USE_REM_IMPORT_JUMP_GLUE)
    if (!pDesc->pvWrapper)
    {
#   ifdef USE_REM_CALLING_CONVENTION_GLUE
        if (remIsFunctionAllInts(pDesc))
        {
            static const struct { void *pvStart, *pvEnd; } s_aTemplates[] =
            {
                { (void *)&WrapGCC2MSC0Int,  (void *)&WrapGCC2MSC0Int_EndProc },
                { (void *)&WrapGCC2MSC1Int,  (void *)&WrapGCC2MSC1Int_EndProc },
                { (void *)&WrapGCC2MSC2Int,  (void *)&WrapGCC2MSC2Int_EndProc },
                { (void *)&WrapGCC2MSC3Int,  (void *)&WrapGCC2MSC3Int_EndProc },
                { (void *)&WrapGCC2MSC4Int,  (void *)&WrapGCC2MSC4Int_EndProc },
                { (void *)&WrapGCC2MSC5Int,  (void *)&WrapGCC2MSC5Int_EndProc },
                { (void *)&WrapGCC2MSC6Int,  (void *)&WrapGCC2MSC6Int_EndProc },
                { (void *)&WrapGCC2MSC7Int,  (void *)&WrapGCC2MSC7Int_EndProc },
                { (void *)&WrapGCC2MSC8Int,  (void *)&WrapGCC2MSC8Int_EndProc },
                { (void *)&WrapGCC2MSC9Int,  (void *)&WrapGCC2MSC9Int_EndProc },
                { (void *)&WrapGCC2MSC10Int, (void *)&WrapGCC2MSC10Int_EndProc },
                { (void *)&WrapGCC2MSC11Int, (void *)&WrapGCC2MSC11Int_EndProc },
                { (void *)&WrapGCC2MSC12Int, (void *)&WrapGCC2MSC12Int_EndProc }
            };
            const unsigned i = pDesc->cParams;
            AssertReleaseMsg(i < RT_ELEMENTS(s_aTemplates), ("%d (%s)\n", i, pDesc->pszName));

            /* duplicate the patch. */
            const size_t cb = (uintptr_t)s_aTemplates[i].pvEnd - (uintptr_t)s_aTemplates[i].pvStart;
            pDesc->pvWrapper = remAllocGlue(cb);
            AssertReturn(pDesc->pvWrapper, VERR_NO_MEMORY);
            memcpy(pDesc->pvWrapper, s_aTemplates[i].pvStart, cb);

            /* fix it up. */
            remGenerateImportGlueFixup((uint8_t *)pDesc->pvWrapper, cb, pDesc);
        }
        else if (   remHasFunctionEllipsis(pDesc)
                 && !remIsFunctionUsingFP(pDesc))
        {
            /* duplicate the patch. */
            const size_t cb = (uintptr_t)&WrapGCC2MSCVariadictInt_EndProc - (uintptr_t)&WrapGCC2MSCVariadictInt;
            pDesc->pvWrapper = remAllocGlue(cb);
            AssertReturn(pDesc->pvWrapper, VERR_NO_MEMORY);
            memcpy(pDesc->pvWrapper, (void *)&WrapGCC2MSCVariadictInt, cb);

            /* fix it up. */
            remGenerateImportGlueFixup((uint8_t *)pDesc->pvWrapper, cb, pDesc);
        }
        else
        {
            /* custom hacks - it's simpler to make assembly templates than writing a more generic code generator... */
            static const struct { const char *pszName; PFNRT pvStart, pvEnd; } s_aTemplates[] =
            {
                { "SSMR3RegisterInternal",  (PFNRT)&WrapGCC2MSC_SSMR3RegisterInternal,  (PFNRT)&WrapGCC2MSC_SSMR3RegisterInternal_EndProc },
            };
            unsigned i;
            for (i = 0; i < RT_ELEMENTS(s_aTemplates); i++)
                if (!strcmp(pDesc->pszName, s_aTemplates[i].pszName))
                    break;
            AssertReleaseMsgReturn(i < RT_ELEMENTS(s_aTemplates), ("Not implemented! %s\n", pDesc->pszName), VERR_NOT_IMPLEMENTED);

            /* duplicate the patch. */
            const size_t cb = (uintptr_t)s_aTemplates[i].pvEnd - (uintptr_t)s_aTemplates[i].pvStart;
            pDesc->pvWrapper = remAllocGlue(cb);
            AssertReturn(pDesc->pvWrapper, VERR_NO_MEMORY);
            memcpy(pDesc->pvWrapper, s_aTemplates[i].pvStart, cb);

            /* fix it up. */
            remGenerateImportGlueFixup((uint8_t *)pDesc->pvWrapper, cb, pDesc);
        }
#   else  /* !USE_REM_CALLING_CONVENTION_GLUE */

        /*
         * Generate a jump patch.
         */
        uint8_t *pb;
#    ifdef RT_ARCH_AMD64
        pDesc->pvWrapper = pb = (uint8_t *)remAllocGlue(32);
        AssertReturn(pDesc->pvWrapper, VERR_NO_MEMORY);
        /**pb++ = 0xcc;*/
        *pb++ = 0xff;
        *pb++ = 0x24;
        *pb++ = 0x25;
        *(uint32_t *)pb = (uintptr_t)pb + 5;
        pb += 5;
        *(uint64_t *)pb = (uint64_t)pDesc->pv;
#    else
        pDesc->pvWrapper = pb = (uint8_t *)remAllocGlue(8);
        AssertReturn(pDesc->pvWrapper, VERR_NO_MEMORY);
        *pb++ = 0xea;
        *(uint32_t *)pb = (uint32_t)pDesc->pv;
#    endif
#   endif /* !USE_REM_CALLING_CONVENTION_GLUE */
    }
    *pValue = (uintptr_t)pDesc->pvWrapper;
#  else  /* !USE_REM_CALLING_CONVENTION_GLUE */
    *pValue = (uintptr_t)pDesc->pv;
#  endif /* !USE_REM_CALLING_CONVENTION_GLUE */
    return VINF_SUCCESS;
}


/**
 * Resolve an external symbol during RTLdrGetBits().
 *
 * @returns iprt status code.
 * @param   hLdrMod         The loader module handle.
 * @param   pszModule       Module name.
 * @param   pszSymbol       Symbol name, NULL if uSymbol should be used.
 * @param   uSymbol         Symbol ordinal, ~0 if pszSymbol should be used.
 * @param   pValue          Where to store the symbol value (address).
 * @param   pvUser          User argument.
 */
static DECLCALLBACK(int) remGetImport(RTLDRMOD hLdrMod, const char *pszModule, const char *pszSymbol, unsigned uSymbol, RTUINTPTR *pValue, void *pvUser)
{
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(g_aVMMImports); i++)
        if (!strcmp(g_aVMMImports[i].pszName, pszSymbol))
            return remGenerateImportGlue(pValue, &g_aVMMImports[i]);
    for (i = 0; i < RT_ELEMENTS(g_aRTImports); i++)
        if (!strcmp(g_aRTImports[i].pszName, pszSymbol))
            return remGenerateImportGlue(pValue, &g_aRTImports[i]);
    for (i = 0; i < RT_ELEMENTS(g_aCRTImports); i++)
        if (!strcmp(g_aCRTImports[i].pszName, pszSymbol))
            return remGenerateImportGlue(pValue, &g_aCRTImports[i]);
    LogRel(("Missing REM Import: %s\n", pszSymbol));
#  if 1
    *pValue = 0;
    AssertMsgFailed(("%s.%s\n", pszModule, pszSymbol));
    return VERR_SYMBOL_NOT_FOUND;
#  else
    return remGenerateImportGlue(pValue, &g_aCRTImports[0]);
#  endif
}

/**
 * Loads the linux object, resolves all imports and exports.
 *
 * @returns VBox status code.
 */
static int remLoadLinuxObj(void)
{
    size_t  offFilename;
    char    szPath[RTPATH_MAX];
    int rc = RTPathAppPrivateArch(szPath, sizeof(szPath) - 32);
    AssertRCReturn(rc, rc);
    offFilename = strlen(szPath);

#  ifdef VBOX_WITHOUT_REM_LDR_CYCLE
    /*
     * Resolve all the VBoxVMM references.
     */
    if (g_ModVMM != NIL_RTLDRMOD)
    {
        rc = SUPR3HardenedLdrLoadAppPriv("VBoxVMM", &g_ModVMM, RTLDRLOAD_FLAGS_LOCAL, NULL);
        AssertRCReturn(rc, rc);
        for (size_t i = 0; i < RT_ELEMENTS(g_aVMMImports); i++)
        {
            rc = RTLdrGetSymbol(g_ModVMM, g_aVMMImports[i].pszName, &g_aVMMImports[i].pv);
            AssertLogRelMsgRCReturn(rc, ("RTLdrGetSymbol(VBoxVMM,%s,) -> %Rrc\n", g_aVMMImports[i].pszName, rc), rc);
        }
    }
#  endif

    /*
     * Load the VBoxREM2.rel object/DLL.
     */
    strcpy(&szPath[offFilename], "/VBoxREM2.rel");
    rc = RTLdrOpen(szPath, 0, RTLDRARCH_HOST, &g_ModREM2);
    if (RT_SUCCESS(rc))
    {
        g_cbREM2 = RTLdrSize(g_ModREM2);
        g_pvREM2 = RTMemExecAlloc(g_cbREM2);
        if (g_pvREM2)
        {
            RTPathChangeToUnixSlashes(szPath, true);
#  ifdef DEBUG /* How to load the VBoxREM2.rel symbols into the GNU debugger. */
            RTPrintf("VBoxREMWrapper: (gdb) add-symbol-file %s 0x%p\n", szPath, g_pvREM2);
#  endif
            LogRel(("REM: Loading %s at 0x%p (%d bytes)\n"
                    "REM: (gdb) add-symbol-file %s 0x%p\n",
                    szPath, g_pvREM2, RTLdrSize(g_ModREM2), szPath, g_pvREM2));
            rc = RTLdrGetBits(g_ModREM2, g_pvREM2, (RTUINTPTR)g_pvREM2, remGetImport, NULL);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Resolve exports.
                 */
                unsigned i;
                for (i = 0; i < RT_ELEMENTS(g_aExports); i++)
                {
                    RTUINTPTR Value;
                    rc = RTLdrGetSymbolEx(g_ModREM2, g_pvREM2, (RTUINTPTR)g_pvREM2, UINT32_MAX, g_aExports[i].pszName, &Value);
                    AssertMsgRC(rc, ("%s rc=%Rrc\n", g_aExports[i].pszName, rc));
                    if (RT_FAILURE(rc))
                        break;
                    rc = remGenerateExportGlue(&Value, &g_aExports[i]);
                    if (RT_FAILURE(rc))
                        break;
                    *(void **)g_aExports[i].pv = (void *)(uintptr_t)Value;
                }
                return rc;
            }

            RTMemExecFree(g_pvREM2, g_cbREM2);
            g_pvREM2 = NULL;
        }
        g_cbREM2 = 0;
        RTLdrClose(g_ModREM2);
        g_ModREM2 = NIL_RTLDRMOD;
    }
    LogRel(("REM: failed loading '%s', rc=%Rrc\n", szPath, rc));
    return rc;
}


/**
 * Unloads the linux object, freeing up all resources (dlls and
 * import glue) we allocated during remLoadLinuxObj().
 */
static void remUnloadLinuxObj(void)
{
    unsigned i;

    /* close modules. */
    RTLdrClose(g_ModREM2);
    g_ModREM2 = NIL_RTLDRMOD;
    RTMemExecFree(g_pvREM2, g_cbREM2);
    g_pvREM2 = NULL;
    g_cbREM2 = 0;

    /* clear the pointers. */
    for (i = 0; i < RT_ELEMENTS(g_aExports); i++)
        *(void **)g_aExports[i].pv = NULL;
#  if defined(USE_REM_CALLING_CONVENTION_GLUE) || defined(USE_REM_IMPORT_JUMP_GLUE)
    for (i = 0; i < RT_ELEMENTS(g_aVMMImports); i++)
        g_aVMMImports[i].pvWrapper = NULL;
    for (i = 0; i < RT_ELEMENTS(g_aRTImports); i++)
        g_aRTImports[i].pvWrapper = NULL;
    for (i = 0; i < RT_ELEMENTS(g_aCRTImports); i++)
        g_aCRTImports[i].pvWrapper = NULL;

    /* free wrapper memory. */
    while (g_pExecMemHead)
    {
        PREMEXECMEM pCur = g_pExecMemHead;
        g_pExecMemHead = pCur->pNext;
        memset(pCur, 0xcc, pCur->cb);
        RTMemExecFree(pCur, pCur->cb);
    }
#  endif
}

# else  /* VBOX_USE_BITNESS_SELECTOR */

/**
 * Checks if 64-bit support is enabled.
 *
 * @returns true / false.
 * @param   pVM         Pointer to the shared VM structure.
 */
static bool remIs64bitEnabled(PVM pVM)
{
    bool f;
    int  rc;

#  ifdef VBOX_WITHOUT_REM_LDR_CYCLE
    if (g_ModVMM == NIL_RTLDRMOD)
    {
        rc = SUPR3HardenedLdrLoadAppPriv("VBoxVMM", &g_ModVMM, RTLDRLOAD_FLAGS_LOCAL, NULL);
        AssertRCReturn(rc, false);
    }

    DECLCALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetRoot)(PVM);
    rc = RTLdrGetSymbol(g_ModVMM, "CFGMR3GetRoot", (void **)&pfnCFGMR3GetRoot);
    AssertRCReturn(rc, false);

    DECLCALLBACKMEMBER(PCFGMNODE, pfnCFGMR3GetChild)(PCFGMNODE, const char *);
    rc = RTLdrGetSymbol(g_ModVMM, "CFGMR3GetChild", (void **)&pfnCFGMR3GetChild);
    AssertRCReturn(rc, false);

    DECLCALLBACKMEMBER(int,       pfnCFGMR3QueryBoolDef)(PCFGMNODE, const char *, bool *, bool);
    rc = RTLdrGetSymbol(g_ModVMM, "CFGMR3QueryBoolDef", (void **)&pfnCFGMR3QueryBoolDef);
    AssertRCReturn(rc, false);

    rc = pfnCFGMR3QueryBoolDef(pfnCFGMR3GetChild(pfnCFGMR3GetRoot(pVM), "REM"), "64bitEnabled", &f, false);
#  else
    rc = CFGMR3QueryBoolDef(CFGMR3GetChild(CFGMR3GetRoot(pVM), "REM"), "64bitEnabled", &f, false);
#  endif
    AssertRCReturn(rc, false);
    return f;
}


/**
 * Loads real REM object, resolves all exports (imports are done by native loader).
 *
 * @returns VBox status code.
 */
static int remLoadProperObj(PVM pVM)
{
    /*
     * Load the VBoxREM32/64 object/DLL.
     */
    const char *pszModule = remIs64bitEnabled(pVM) ? "VBoxREM64" : "VBoxREM32";
    int rc = SUPR3HardenedLdrLoadAppPriv(pszModule, &g_ModREM2, RTLDRLOAD_FLAGS_LOCAL, NULL);
    if (RT_SUCCESS(rc))
    {
        LogRel(("REM: %s\n", pszModule));

        /*
         * Resolve exports.
         */
        unsigned i;
        for (i = 0; i < RT_ELEMENTS(g_aExports); i++)
        {
            void *pvValue;
            rc = RTLdrGetSymbol(g_ModREM2, g_aExports[i].pszName, &pvValue);
            AssertLogRelMsgRCBreak(rc, ("%s rc=%Rrc\n", g_aExports[i].pszName, rc));
            *(void **)g_aExports[i].pv = pvValue;
        }
    }

    return rc;
}


/**
 * Unloads the real REM object.
 */
static void remUnloadProperObj(void)
{
    /* close module. */
    RTLdrClose(g_ModREM2);
    g_ModREM2 = NIL_RTLDRMOD;
}

# endif /* VBOX_USE_BITNESS_SELECTOR */
#endif /* USE_REM_STUBS */

REMR3DECL(int) REMR3Init(PVM pVM)
{
#ifdef USE_REM_STUBS
    return VINF_SUCCESS;

#elif defined(VBOX_USE_BITNESS_SELECTOR)
    if (!pfnREMR3Init)
    {
        int rc = remLoadProperObj(pVM);
        if (RT_FAILURE(rc))
            return rc;
    }
    return pfnREMR3Init(pVM);

#else
    if (!pfnREMR3Init)
    {
        int rc = remLoadLinuxObj();
        if (RT_FAILURE(rc))
            return rc;
    }
    return pfnREMR3Init(pVM);
#endif
}

REMR3DECL(int) REMR3InitFinalize(PVM pVM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3InitFinalize));
    return pfnREMR3InitFinalize(pVM);
#endif
}

REMR3DECL(int) REMR3Term(PVM pVM)
{
#ifdef USE_REM_STUBS
    return VINF_SUCCESS;

#elif defined(VBOX_USE_BITNESS_SELECTOR)
    int rc;
    Assert(VALID_PTR(pfnREMR3Term));
    rc = pfnREMR3Term(pVM);
    remUnloadProperObj();
    return rc;

#else
    int rc;
    Assert(VALID_PTR(pfnREMR3Term));
    rc = pfnREMR3Term(pVM);
    remUnloadLinuxObj();
    return rc;
#endif
}

REMR3DECL(void) REMR3Reset(PVM pVM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3Reset));
    pfnREMR3Reset(pVM);
#endif
}

REMR3DECL(int) REMR3Step(PVM pVM, PVMCPU pVCpu)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3Step));
    return pfnREMR3Step(pVM, pVCpu);
#endif
}

REMR3DECL(int) REMR3BreakpointSet(PVM pVM, RTGCUINTPTR Address)
{
#ifdef USE_REM_STUBS
    return VERR_REM_NO_MORE_BP_SLOTS;
#else
    Assert(VALID_PTR(pfnREMR3BreakpointSet));
    return pfnREMR3BreakpointSet(pVM, Address);
#endif
}

REMR3DECL(int) REMR3BreakpointClear(PVM pVM, RTGCUINTPTR Address)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3BreakpointClear));
    return pfnREMR3BreakpointClear(pVM, Address);
#endif
}

REMR3DECL(int) REMR3EmulateInstruction(PVM pVM, PVMCPU pVCpu)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3EmulateInstruction));
    return pfnREMR3EmulateInstruction(pVM, pVCpu);
#endif
}

REMR3DECL(int) REMR3Run(PVM pVM, PVMCPU pVCpu)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3Run));
    return pfnREMR3Run(pVM, pVCpu);
#endif
}

REMR3DECL(int) REMR3State(PVM pVM, PVMCPU pVCpu)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3State));
    return pfnREMR3State(pVM, pVCpu);
#endif
}

REMR3DECL(int) REMR3StateBack(PVM pVM, PVMCPU pVCpu)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3StateBack));
    return pfnREMR3StateBack(pVM, pVCpu);
#endif
}

REMR3DECL(void) REMR3StateUpdate(PVM pVM, PVMCPU pVCpu)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3StateUpdate));
    pfnREMR3StateUpdate(pVM, pVCpu);
#endif
}

REMR3DECL(void) REMR3A20Set(PVM pVM, PVMCPU pVCpu, bool fEnable)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3A20Set));
    pfnREMR3A20Set(pVM, pVCpu, fEnable);
#endif
}

REMR3DECL(void) REMR3ReplayHandlerNotifications(PVM pVM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3ReplayHandlerNotifications));
    pfnREMR3ReplayHandlerNotifications(pVM);
#endif
}

REMR3DECL(int) REMR3NotifyCodePageChanged(PVM pVM, PVMCPU pVCpu, RTGCPTR pvCodePage)
{
#ifdef USE_REM_STUBS
    return VINF_SUCCESS;
#else
    Assert(VALID_PTR(pfnREMR3NotifyCodePageChanged));
    return pfnREMR3NotifyCodePageChanged(pVM, pVCpu, pvCodePage);
#endif
}

REMR3DECL(void) REMR3NotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb, unsigned fFlags)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyPhysRamRegister));
    pfnREMR3NotifyPhysRamRegister(pVM, GCPhys, cb, fFlags);
#endif
}

REMR3DECL(void) REMR3NotifyPhysRomRegister(PVM pVM, RTGCPHYS GCPhys, RTUINT cb, void *pvCopy, bool fShadow)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyPhysRomRegister));
    pfnREMR3NotifyPhysRomRegister(pVM, GCPhys, cb, pvCopy, fShadow);
#endif
}

REMR3DECL(void) REMR3NotifyPhysRamDeregister(PVM pVM, RTGCPHYS GCPhys, RTUINT cb)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyPhysRamDeregister));
    pfnREMR3NotifyPhysRamDeregister(pVM, GCPhys, cb);
#endif
}

REMR3DECL(void) REMR3NotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyHandlerPhysicalRegister));
    pfnREMR3NotifyHandlerPhysicalRegister(pVM, enmKind, GCPhys, cb, fHasHCHandler);
#endif
}

REMR3DECL(void) REMR3NotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyHandlerPhysicalDeregister));
    pfnREMR3NotifyHandlerPhysicalDeregister(pVM, enmKind, GCPhys, cb, fHasHCHandler, fRestoreAsRAM);
#endif
}

REMR3DECL(void) REMR3NotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyHandlerPhysicalModify));
    pfnREMR3NotifyHandlerPhysicalModify(pVM, enmKind, GCPhysOld, GCPhysNew, cb, fHasHCHandler, fRestoreAsRAM);
#endif
}

REMR3DECL(bool) REMR3IsPageAccessHandled(PVM pVM, RTGCPHYS GCPhys)
{
#ifdef USE_REM_STUBS
    return false;
#else
    Assert(VALID_PTR(pfnREMR3IsPageAccessHandled));
    return pfnREMR3IsPageAccessHandled(pVM, GCPhys);
#endif
}

REMR3DECL(int) REMR3DisasEnableStepping(PVM pVM, bool fEnable)
{
#ifdef USE_REM_STUBS
    return VERR_NOT_IMPLEMENTED;
#else
    Assert(VALID_PTR(pfnREMR3DisasEnableStepping));
    return pfnREMR3DisasEnableStepping(pVM, fEnable);
#endif
}

REMR3DECL(void) REMR3NotifyInterruptSet(PVM pVM, PVMCPU pVCpu)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyInterruptSet));
    pfnREMR3NotifyInterruptSet(pVM, pVCpu);
#endif
}

REMR3DECL(void) REMR3NotifyInterruptClear(PVM pVM, PVMCPU pVCpu)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyInterruptClear));
    pfnREMR3NotifyInterruptClear(pVM, pVCpu);
#endif
}

REMR3DECL(void) REMR3NotifyTimerPending(PVM pVM, PVMCPU pVCpuDst)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyTimerPending));
    pfnREMR3NotifyTimerPending(pVM, pVCpuDst);
#endif
}

REMR3DECL(void) REMR3NotifyDmaPending(PVM pVM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyDmaPending));
    pfnREMR3NotifyDmaPending(pVM);
#endif
}

REMR3DECL(void) REMR3NotifyQueuePending(PVM pVM)
{
#ifndef USE_REM_STUBS
    Assert(VALID_PTR(pfnREMR3NotifyQueuePending));
    pfnREMR3NotifyQueuePending(pVM);
#endif
}

REMR3DECL(void) REMR3NotifyFF(PVM pVM)
{
#ifndef USE_REM_STUBS
    /* the timer can call this early on, so don't be picky. */
    if (pfnREMR3NotifyFF)
        pfnREMR3NotifyFF(pVM);
#endif
}
