/* $Id: DBGFInternal.h $ */
/** @file
 * DBGF - Internal header file.
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

#ifndef ___DBGFInternal_h
#define ___DBGFInternal_h

#include <VBox/cdefs.h>
#ifdef IN_RING3
# include <VBox/dis.h>
#endif
#include <VBox/types.h>
#include <iprt/semaphore.h>
#include <iprt/critsect.h>
#include <iprt/string.h>
#include <iprt/avl.h>
#include <iprt/dbg.h>
#include <VBox/vmm/dbgf.h>



/** @defgroup grp_dbgf_int   Internals
 * @ingroup grp_dbgf
 * @internal
 * @{
 */


/** VMM Debugger Command. */
typedef enum DBGFCMD
{
    /** No command.
     * This is assigned to the field by the emulation thread after
     * a command has been completed. */
    DBGFCMD_NO_COMMAND = 0,
    /** Halt the VM. */
    DBGFCMD_HALT,
    /** Resume execution. */
    DBGFCMD_GO,
    /** Single step execution - stepping into calls. */
    DBGFCMD_SINGLE_STEP,
    /** Detaches the debugger.
     * Disabling all breakpoints, watch points and the like. */
    DBGFCMD_DETACH_DEBUGGER,
    /** Detached the debugger.
     * The isn't a command as such, it's just that it's necessary for the
     * detaching protocol to be racefree. */
    DBGFCMD_DETACHED_DEBUGGER
} DBGFCMD;

/**
 * VMM Debugger Command.
 */
typedef union DBGFCMDDATA
{
    uint32_t    uDummy;
} DBGFCMDDATA;
/** Pointer to DBGF Command Data. */
typedef DBGFCMDDATA *PDBGFCMDDATA;

/**
 * Info type.
 */
typedef enum DBGFINFOTYPE
{
    /** Invalid. */
    DBGFINFOTYPE_INVALID = 0,
    /** Device owner. */
    DBGFINFOTYPE_DEV,
    /** Driver owner. */
    DBGFINFOTYPE_DRV,
    /** Internal owner. */
    DBGFINFOTYPE_INT,
    /** External owner. */
    DBGFINFOTYPE_EXT
} DBGFINFOTYPE;


/** Pointer to info structure. */
typedef struct DBGFINFO *PDBGFINFO;

#ifdef IN_RING3
/**
 * Info structure.
 */
typedef struct DBGFINFO
{
    /** The flags. */
    uint32_t        fFlags;
    /** Owner type. */
    DBGFINFOTYPE    enmType;
    /** Per type data. */
    union
    {
        /** DBGFINFOTYPE_DEV */
        struct
        {
            /** Device info handler function. */
            PFNDBGFHANDLERDEV   pfnHandler;
            /** The device instance. */
            PPDMDEVINS          pDevIns;
        } Dev;

        /** DBGFINFOTYPE_DRV */
        struct
        {
            /** Driver info handler function. */
            PFNDBGFHANDLERDRV   pfnHandler;
            /** The driver instance. */
            PPDMDRVINS          pDrvIns;
        } Drv;

        /** DBGFINFOTYPE_INT */
        struct
        {
            /** Internal info handler function. */
            PFNDBGFHANDLERINT   pfnHandler;
        } Int;

        /** DBGFINFOTYPE_EXT */
        struct
        {
            /** External info handler function. */
            PFNDBGFHANDLEREXT   pfnHandler;
            /** The user argument. */
            void               *pvUser;
        } Ext;
    } u;

    /** Pointer to the description. */
    const char     *pszDesc;
    /** Pointer to the next info structure. */
    PDBGFINFO       pNext;
    /** The identifier name length. */
    size_t          cchName;
    /** The identifier name. (Extends 'beyond' the struct as usual.) */
    char            szName[1];
} DBGFINFO;
#endif /* IN_RING3 */


/**
 * Guest OS digger instance.
 */
typedef struct DBGFOS
{
    /** Pointer to the registration record. */
    PCDBGFOSREG                 pReg;
    /** Pointer to the next OS we've registered. */
    struct DBGFOS              *pNext;
    /** List of EMT interface wrappers. */
    struct DBGFOSEMTWRAPPER    *pWrapperHead;
    /** The instance data (variable size). */
    uint8_t                     abData[16];
} DBGFOS;
/** Pointer to guest OS digger instance. */
typedef DBGFOS *PDBGFOS;
/** Pointer to const guest OS digger instance. */
typedef DBGFOS const *PCDBGFOS;


/**
 * Breakpoint search optimization.
 */
typedef struct DBGFBPSEARCHOPT
{
    /** Where to start searching for hits.
     * (First enabled is #DBGF::aBreakpoints[iStartSearch]). */
    uint32_t volatile       iStartSearch;
    /** The number of aBreakpoints entries to search.
     * (Last enabled is #DBGF::aBreakpoints[iStartSearch + cToSearch - 1])  */
    uint32_t volatile       cToSearch;
} DBGFBPSEARCHOPT;
/** Pointer to a breakpoint search optimziation structure. */
typedef DBGFBPSEARCHOPT *PDBGFBPSEARCHOPT;



/**
 * DBGF Data (part of VM)
 */
typedef struct DBGF
{
    /** Bitmap of enabled hardware interrupt breakpoints. */
    uint32_t                    bmHardIntBreakpoints[256 / 32];
    /** Bitmap of enabled software interrupt breakpoints. */
    uint32_t                    bmSoftIntBreakpoints[256 / 32];
    /** Bitmap of selected events.
     * This includes non-selectable events too for simplicity, we maintain the
     * state for some of these, as it may come in handy. */
    uint64_t                    bmSelectedEvents[(DBGFEVENT_END + 63) / 64];

    /** Enabled hardware interrupt breakpoints. */
    uint32_t                    cHardIntBreakpoints;
    /** Enabled software interrupt breakpoints. */
    uint32_t                    cSoftIntBreakpoints;

    /** The number of selected events. */
    uint32_t                    cSelectedEvents;

    /** The number of enabled hardware breakpoints. */
    uint8_t                     cEnabledHwBreakpoints;
    /** The number of enabled hardware I/O breakpoints. */
    uint8_t                     cEnabledHwIoBreakpoints;
    /** The number of enabled INT3 breakpoints. */
    uint8_t                     cEnabledInt3Breakpoints;
    uint8_t                     abPadding; /**< Unused padding space up for grabs. */
    uint32_t                    uPadding;

    /** Debugger Attached flag.
     * Set if a debugger is attached, elsewise it's clear.
     */
    bool volatile               fAttached;

    /** Stopped in the Hypervisor.
     * Set if we're stopped on a trace, breakpoint or assertion inside
     * the hypervisor and have to restrict the available operations.
     */
    bool volatile               fStoppedInHyper;

    /**
     * Ping-Pong construct where the Ping side is the VMM and the Pong side
     * the Debugger.
     */
    RTPINGPONG                  PingPong;
    RTHCUINTPTR                 uPtrPadding; /**< Alignment padding. */

    /** The Event to the debugger.
     * The VMM will ping the debugger when the event is ready. The event is
     * either a response to a command or to a break/watch point issued
     * previously.
     */
    DBGFEVENT                   DbgEvent;

    /** The Command to the VMM.
     * Operated in an atomic fashion since the VMM will poll on this.
     * This means that a the command data must be written before this member
     * is set. The VMM will reset this member to the no-command state
     * when it have processed it.
     */
    DBGFCMD volatile            enmVMMCmd;
    /** The Command data.
     * Not all commands take data. */
    DBGFCMDDATA                 VMMCmdData;

    /** Stepping filtering. */
    struct
    {
        /** The CPU doing the stepping.
         * Set to NIL_VMCPUID when filtering is inactive */
        VMCPUID                 idCpu;
        /** The specified flags. */
        uint32_t                fFlags;
        /** The effective PC address to stop at, if given. */
        RTGCPTR                 AddrPc;
        /** The lowest effective stack address to stop at.
         * Together with cbStackPop, this forms a range of effective stack pointer
         * addresses that we stop for.   */
        RTGCPTR                 AddrStackPop;
        /** The size of the stack stop area starting at AddrStackPop. */
        RTGCPTR                 cbStackPop;
        /** Maximum number of steps. */
        uint32_t                cMaxSteps;

        /** Number of steps made thus far. */
        uint32_t                cSteps;
        /** Current call counting balance for step-over handling. */
        uint32_t                uCallDepth;

        uint32_t                u32Padding; /**< Alignment padding. */

    } SteppingFilter;

    uint32_t                    u32Padding[2]; /**< Alignment padding. */

    /** Array of hardware breakpoints. (0..3)
     * This is shared among all the CPUs because life is much simpler that way. */
    DBGFBP                      aHwBreakpoints[4];
    /** Array of int 3 and REM breakpoints. (4..)
     * @remark This is currently a fixed size array for reasons of simplicity. */
    DBGFBP                      aBreakpoints[32];

    /** MMIO breakpoint search optimizations. */
    DBGFBPSEARCHOPT             Mmio;
    /** I/O port breakpoint search optimizations. */
    DBGFBPSEARCHOPT             PortIo;
    /** INT3 breakpoint search optimizations. */
    DBGFBPSEARCHOPT             Int3;
} DBGF;
AssertCompileMemberAlignment(DBGF, DbgEvent, 8);
AssertCompileMemberAlignment(DBGF, aHwBreakpoints, 8);
AssertCompileMemberAlignment(DBGF, bmHardIntBreakpoints, 8);
/** Pointer to DBGF Data. */
typedef DBGF *PDBGF;


/**
 * Event state (for DBGFCPU::aEvents).
 */
typedef enum DBGFEVENTSTATE
{
    /** Invalid event stack entry. */
    DBGFEVENTSTATE_INVALID = 0,
    /** The current event stack entry. */
    DBGFEVENTSTATE_CURRENT,
    /** Event that should be ignored but hasn't yet actually been ignored. */
    DBGFEVENTSTATE_IGNORE,
    /** Event that has been ignored but may be restored to IGNORE should another
     * debug event fire before the instruction is completed. */
    DBGFEVENTSTATE_RESTORABLE,
    /** End of valid events.   */
    DBGFEVENTSTATE_END,
    /** Make sure we've got a 32-bit type. */
    DBGFEVENTSTATE_32BIT_HACK = 0x7fffffff
} DBGFEVENTSTATE;


/** Converts a DBGFCPU pointer into a VM pointer. */
#define DBGFCPU_2_VM(pDbgfCpu) ((PVM)((uint8_t *)(pDbgfCpu) + (pDbgfCpu)->offVM))

/**
 * The per CPU data for DBGF.
 */
typedef struct DBGFCPU
{
    /** The offset into the VM structure.
     * @see DBGFCPU_2_VM(). */
    uint32_t                offVM;

    /** Current active breakpoint (id).
     * This is ~0U if not active. It is set when a execution engine
     * encounters a breakpoint and returns VINF_EM_DBG_BREAKPOINT. This is
     * currently not used for REM breakpoints because of the lazy coupling
     * between VBox and REM.
     *
     * @todo drop this in favor of aEvents!  */
    uint32_t                iActiveBp;
    /** Set if we're singlestepping in raw mode.
     * This is checked and cleared in the \#DB handler. */
    bool                    fSingleSteppingRaw;

    /** Alignment padding. */
    bool                    afPadding[3];

    /** The number of events on the stack (aEvents).
     * The pending event is the last one (aEvents[cEvents - 1]), but only when
     * enmState is DBGFEVENTSTATE_CURRENT. */
    uint32_t                cEvents;
    /** Events - current, ignoring and ignored.
     *
     * We maintain a stack of events in order to try avoid ending up in an infinit
     * loop when resuming after an event fired.  There are cases where we may end
     * generating additional events before the instruction can be executed
     * successfully.  Like for instance an XCHG on MMIO with separate read and write
     * breakpoints, or a MOVSB instruction working on breakpointed MMIO as both
     * source and destination.
     *
     * So, when resuming after dropping into the debugger for an event, we convert
     * the DBGFEVENTSTATE_CURRENT event into a DBGFEVENTSTATE_IGNORE event, leaving
     * cEvents unchanged.  If the event is reported again, we will ignore it and
     * tell the reporter to continue executing.  The event change to the
     * DBGFEVENTSTATE_RESTORABLE state.
     *
     * Currently, the event reporter has to figure out that it is a nested event and
     * tell DBGF to restore DBGFEVENTSTATE_RESTORABLE events (and keep
     * DBGFEVENTSTATE_IGNORE, should they happen out of order for some weird
     * reason).
     */
    struct
    {
        /** The event details. */
        DBGFEVENT           Event;
        /** The RIP at which this happend (for validating ignoring). */
        uint64_t            rip;
        /** The event state. */
        DBGFEVENTSTATE      enmState;
        /** Alignment padding. */
        uint32_t            u32Alignment;
    } aEvents[3];
} DBGFCPU;
AssertCompileMemberAlignment(DBGFCPU, aEvents, 8);
AssertCompileMemberSizeAlignment(DBGFCPU, aEvents[0], 8);
/** Pointer to DBGFCPU data. */
typedef DBGFCPU *PDBGFCPU;

struct DBGFOSEMTWRAPPER;

/**
 * The DBGF data kept in the UVM.
 */
typedef struct DBGFUSERPERVM
{
    /** The address space database lock. */
    RTSEMRW                     hAsDbLock;
    /** The address space handle database.      (Protected by hAsDbLock.) */
    R3PTRTYPE(AVLPVTREE)        AsHandleTree;
    /** The address space process id database.  (Protected by hAsDbLock.) */
    R3PTRTYPE(AVLU32TREE)       AsPidTree;
    /** The address space name database.        (Protected by hAsDbLock.) */
    R3PTRTYPE(RTSTRSPACE)       AsNameSpace;
    /** Special address space aliases.          (Protected by hAsDbLock.) */
    RTDBGAS volatile            ahAsAliases[DBGF_AS_COUNT];
    /** For lazily populating the aliased address spaces. */
    bool volatile               afAsAliasPopuplated[DBGF_AS_COUNT];
    /** Alignment padding. */
    bool                        afAlignment1[2];
    /** Debug configuration. */
    R3PTRTYPE(RTDBGCFG)         hDbgCfg;

    /** The register database lock. */
    RTSEMRW                     hRegDbLock;
    /** String space for looking up registers.  (Protected by hRegDbLock.) */
    R3PTRTYPE(RTSTRSPACE)       RegSpace;
    /** String space holding the register sets. (Protected by hRegDbLock.)  */
    R3PTRTYPE(RTSTRSPACE)       RegSetSpace;
    /** The number of registers (aliases, sub-fields and the special CPU
     * register aliases (eg AH) are not counted). */
    uint32_t                    cRegs;
    /** For early initialization by . */
    bool volatile               fRegDbInitialized;
    /** Alignment padding. */
    bool                        afAlignment2[3];

    /** Critical section protecting the Guest OS Digger data, the info handlers
     * and the plugins.  These share to give the best possible plugin unload
     * race protection. */
    RTCRITSECTRW                CritSect;
    /** Head of the LIFO of loaded DBGF plugins. */
    R3PTRTYPE(struct DBGFPLUGIN *) pPlugInHead;
    /** The current Guest OS digger. */
    R3PTRTYPE(PDBGFOS)          pCurOS;
    /** The head of the Guest OS digger instances. */
    R3PTRTYPE(PDBGFOS)          pOSHead;
    /** List of registered info handlers. */
    R3PTRTYPE(PDBGFINFO)        pInfoFirst;

    /** The type database lock. */
    RTSEMRW                     hTypeDbLock;
    /** String space for looking up types.  (Protected by hTypeDbLock.) */
    R3PTRTYPE(RTSTRSPACE)       TypeSpace;
    /** For early initialization by . */
    bool volatile               fTypeDbInitialized;
    /** Alignment padding. */
    bool                        afAlignment3[3];

} DBGFUSERPERVM;
typedef DBGFUSERPERVM *PDBGFUSERPERVM;
typedef DBGFUSERPERVM const *PCDBGFUSERPERVM;

/**
 * The per-CPU DBGF data kept in the UVM.
 */
typedef struct DBGFUSERPERVMCPU
{
    /** The guest register set for this CPU.  Can be NULL. */
    R3PTRTYPE(struct DBGFREGSET *) pGuestRegSet;
    /** The hypervisor register set for this CPU.  Can be NULL. */
    R3PTRTYPE(struct DBGFREGSET *) pHyperRegSet;
} DBGFUSERPERVMCPU;


int  dbgfR3AsInit(PUVM pUVM);
void dbgfR3AsTerm(PUVM pUVM);
void dbgfR3AsRelocate(PUVM pUVM, RTGCUINTPTR offDelta);
int  dbgfR3BpInit(PVM pVM);
int  dbgfR3InfoInit(PUVM pUVM);
int  dbgfR3InfoTerm(PUVM pUVM);
int  dbgfR3OSInit(PUVM pUVM);
void dbgfR3OSTerm(PUVM pUVM);
int  dbgfR3RegInit(PUVM pUVM);
void dbgfR3RegTerm(PUVM pUVM);
int  dbgfR3TraceInit(PVM pVM);
void dbgfR3TraceRelocate(PVM pVM);
void dbgfR3TraceTerm(PVM pVM);
DECLHIDDEN(int)  dbgfR3TypeInit(PUVM pUVM);
DECLHIDDEN(void) dbgfR3TypeTerm(PUVM pUVM);
int  dbgfR3PlugInInit(PUVM pUVM);
void dbgfR3PlugInTerm(PUVM pUVM);



#ifdef IN_RING3
/**
 * DBGF disassembler state (substate of DISSTATE).
 */
typedef struct DBGFDISSTATE
{
    /** Pointer to the current instruction. */
    PCDISOPCODE     pCurInstr;
    /** Size of the instruction in bytes. */
    uint32_t        cbInstr;
    /** Parameters.  */
    DISOPPARAM      Param1;
    DISOPPARAM      Param2;
    DISOPPARAM      Param3;
    DISOPPARAM      Param4;
} DBGFDISSTATE;
/** Pointer to a DBGF disassembler state. */
typedef DBGFDISSTATE *PDBGFDISSTATE;

DECLHIDDEN(int) dbgfR3DisasInstrStateEx(PUVM pUVM, VMCPUID idCpu, PDBGFADDRESS pAddr, uint32_t fFlags,
                                        char *pszOutput, uint32_t cbOutput, PDBGFDISSTATE pDisState);

#endif

/** @} */

#endif
