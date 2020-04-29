/* $Id: main.cpp $ */
/** @file
 * VBox Qt GUI - The main() function.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QMessageBox>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIMachine.h"
# include "UISelectorWindow.h"
# include "UIModalWindowManager.h"
# ifdef VBOX_WS_MAC
#  include "VBoxUtils.h"
#  include "UICocoaApplication.h"
# endif /* VBOX_WS_MAC */

/* Other VBox includes: */
# include <iprt/buildconfig.h>
# include <iprt/stream.h>
# include <VBox/version.h>
# ifdef VBOX_WITH_HARDENING
#  include <VBox/sup.h>
# else /* !VBOX_WITH_HARDENING */
#  include <iprt/initterm.h>
#  ifdef VBOX_WS_MAC
#   include <iprt/asm.h>
#  endif /* VBOX_WS_MAC */
# endif /* !VBOX_WITH_HARDENING */
# ifdef VBOX_WS_X11
#  include <iprt/env.h>
# endif /* VBOX_WS_X11 */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
#ifdef VBOX_WS_X11
# ifndef Q_OS_SOLARIS
#  include <QFontDatabase>
# endif /* !Q_OS_SOLARIS */
#endif /* VBOX_WS_X11 */

/* Other VBox includes: */
#ifdef VBOX_WITH_HARDENING
# include <iprt/ctype.h>
#else /* !VBOX_WITH_HARDENING */
# include <VBox/err.h>
#endif /* !VBOX_WITH_HARDENING */
#ifdef VBOX_WS_MAC
# include <iprt/path.h>
#endif

/* Other includes: */
#ifdef VBOX_WS_MAC
# include <dlfcn.h>
# include <sys/mman.h>
//# include <mach-o/dyld.h>
extern "C" const char *_dyld_get_image_name(uint32_t);
#endif /* VBOX_WS_MAC */
#ifdef VBOX_WS_X11
# include <dlfcn.h>
# include <unistd.h>
# include <X11/Xlib.h>
# if defined(RT_OS_LINUX) && defined(DEBUG)
#  include <signal.h>
#  include <execinfo.h>
#  ifndef __USE_GNU
#   define __USE_GNU
#  endif /* !__USE_GNU */
#  include <ucontext.h>
#  ifdef RT_ARCH_AMD64
#   define REG_PC REG_RIP
#  else /* !RT_ARCH_AMD64 */
#   define REG_PC REG_EIP
#  endif /* !RT_ARCH_AMD64 */
# endif /* RT_OS_LINUX && DEBUG */
#endif /* VBOX_WS_X11 */


/* XXX Temporarily. Don't rely on the user to hack the Makefile himself! */
QString g_QStrHintLinuxNoMemory = QApplication::tr(
    "This error means that the kernel driver was either not able to "
    "allocate enough memory or that some mapping operation failed."
    );

QString g_QStrHintLinuxNoDriver = QApplication::tr(
    "The VirtualBox Linux kernel driver is either not loaded or not set "
    "up correctly. Please try setting it up again by executing<br/><br/>"
    "  <font color=blue>'/sbin/vboxconfig'</font><br/><br/>"
    "as root.<br/><br/>"
    "If your system has EFI Secure Boot enabled you may also need to sign "
    "the kernel modules (vboxdrv, vboxnetflt, vboxnetadp, vboxpci) before "
    "you can load them. Please see your Linux system's documentation for "
    "more information."
    );

QString g_QStrHintOtherWrongDriverVersion = QApplication::tr(
    "The VirtualBox kernel modules do not match this version of "
    "VirtualBox. The installation of VirtualBox was apparently not "
    "successful. Please try completely uninstalling and reinstalling "
    "VirtualBox."
    );

QString g_QStrHintLinuxWrongDriverVersion = QApplication::tr(
    "The VirtualBox kernel modules do not match this version of "
    "VirtualBox. The installation of VirtualBox was apparently not "
    "successful. Executing<br/><br/>"
    "  <font color=blue>'/sbin/vboxconfig'</font><br/><br/>"
    "may correct this. Make sure that you are not mixing builds "
    "of VirtualBox from different sources."
    );

QString g_QStrHintOtherNoDriver = QApplication::tr(
    "Make sure the kernel module has been loaded successfully."
    );

/* I hope this isn't (C), (TM) or (R) Microsoft support ;-) */
QString g_QStrHintReinstall = QApplication::tr(
    "Please try reinstalling VirtualBox."
    );


#ifdef VBOX_WS_MAC

# include <unistd.h>
# include <stdio.h>
# include <dlfcn.h>
# include <iprt/formats/mach-o.h>

/**
 * Override this one to try hide the fact that we're setuid to root
 * orginially.
 */
int issetugid_for_AppKit(void)
{
    Dl_info Info = {0};
    char szMsg[512];
    size_t cchMsg;
    const void * uCaller = __builtin_return_address(0);
    if (dladdr(uCaller, &Info))
        cchMsg = snprintf(szMsg, sizeof(szMsg), "DEBUG: issetugid_for_AppKit was called by %p %s::%s+%p (via %p)\n",
                          uCaller, Info.dli_fname, Info.dli_sname, (void *)((uintptr_t)uCaller - (uintptr_t)Info.dli_saddr), __builtin_return_address(1));
    else
        cchMsg = snprintf(szMsg, sizeof(szMsg), "DEBUG: issetugid_for_AppKit was called by %p (via %p)\n", uCaller, __builtin_return_address(1));
    write(2, szMsg, cchMsg);
    return 0;
}

static bool patchExtSym(mach_header_64_t *pHdr, const char *pszSymbol, uintptr_t uNewValue)
{
    /*
     * First do some basic header checks and the scan the load
     * commands for the symbol table info.
     */
    AssertLogRelMsgReturn(pHdr->magic == (ARCH_BITS == 64 ? MH_MAGIC_64 : MH_MAGIC),
                          ("%p: magic=%#x\n", pHdr, pHdr->magic), false);
    uint32_t const cCmds = pHdr->ncmds;
    uint32_t const cbCmds = pHdr->sizeofcmds;
    AssertLogRelMsgReturn(cCmds < 16384 && cbCmds < _2M, ("%p: ncmds=%u sizeofcmds=%u\n", pHdr, cCmds, cbCmds), false);

    /*
     * First command pass: Locate the symbol table and dynamic symbol table info
     *                     commands, also calc the slide (load addr - link addr).
     */
    dysymtab_command_t const   *pDySymTab = NULL;
    symtab_command_t const     *pSymTab   = NULL;
    segment_command_64_t const *pFirstSeg = NULL;
    uintptr_t                   offSlide  = 0;
    uint32_t                    offCmd    = 0;
    for (uint32_t iCmd = 0; iCmd < cCmds; iCmd++)
    {
        AssertLogRelMsgReturn(offCmd + sizeof(load_command_t) <= cbCmds,
                              ("%p: iCmd=%u offCmd=%#x cbCmds=%#x\n", pHdr, iCmd, offCmd, cbCmds), false);
        load_command_t const * const pCmd = (load_command_t const *)((uintptr_t)(pHdr + 1) + offCmd);
        uint32_t const cbCurCmd = pCmd->cmdsize;
        AssertLogRelMsgReturn(offCmd + cbCurCmd <= cbCmds && cbCurCmd <= cbCmds,
                              ("%p: iCmd=%u offCmd=%#x cbCurCmd=%#x cbCmds=%#x\n", pHdr, iCmd, offCmd, cbCurCmd, cbCmds), false);
        offCmd += cbCurCmd;

        if (pCmd->cmd == LC_SYMTAB)
        {
            AssertLogRelMsgReturn(!pSymTab, ("%p: pSymTab=%p pCmd=%p\n", pHdr, pSymTab, pCmd), false);
            pSymTab = (symtab_command_t const *)pCmd;
            AssertLogRelMsgReturn(cbCurCmd == sizeof(*pSymTab), ("%p: pSymTab=%p cbCurCmd=%#x\n", pHdr, pCmd, cbCurCmd), false);

        }
        else if (pCmd->cmd == LC_DYSYMTAB)
        {
            AssertLogRelMsgReturn(!pDySymTab, ("%p: pDySymTab=%p pCmd=%p\n", pHdr, pDySymTab, pCmd), false);
            pDySymTab = (dysymtab_command_t const *)pCmd;
            AssertLogRelMsgReturn(cbCurCmd == sizeof(*pDySymTab), ("%p: pDySymTab=%p cbCurCmd=%#x\n", pHdr, pCmd, cbCurCmd),
                                  false);
        }
        else if (pCmd->cmd == LC_SEGMENT_64 && !pFirstSeg) /* ASSUMES the first seg is the one with the header and stuff. */
        {
            /* Note! the fileoff and vmaddr seems to be modified. */
            pFirstSeg = (segment_command_64_t const *)pCmd;
            AssertLogRelMsgReturn(cbCurCmd >= sizeof(*pFirstSeg), ("%p: iCmd=%u cbCurCmd=%#x\n", pHdr, iCmd, cbCurCmd), false);
            AssertLogRelMsgReturn(/*pFirstSeg->fileoff == 0 && */ pFirstSeg->vmsize >= sizeof(*pHdr) + cbCmds,
                                  ("%p: iCmd=%u fileoff=%llx vmsize=%#llx cbCmds=%#x name=%.16s\n",
                                   pHdr, iCmd, pFirstSeg->fileoff, pFirstSeg->vmsize, cbCmds, pFirstSeg->segname), false);
            offSlide = (uintptr_t)pHdr - pFirstSeg->vmaddr;
        }
    }
    AssertLogRelMsgReturn(pSymTab, ("%p: no LC_SYMTAB\n", pHdr), false);
    AssertLogRelMsgReturn(pDySymTab, ("%p: no LC_DYSYMTAB\n", pHdr), false);
    AssertLogRelMsgReturn(pFirstSeg, ("%p: no LC_SEGMENT_64\n", pHdr), false);

    /*
     * Second command pass: Locate the memory locations of the symbol table, string
     *                      table and the indirect symbol table by checking LC_SEGMENT_xx.
     */
    macho_nlist_64_t const *paSymbols  = NULL;
    uint32_t const          offSymbols = pSymTab->symoff;
    uint32_t const          cSymbols   = pSymTab->nsyms;
    AssertLogRelMsgReturn(cSymbols > 0 && offSymbols >= sizeof(pHdr) + cbCmds,
                          ("%p: cSymbols=%#x offSymbols=%#x\n", pHdr, cSymbols, offSymbols), false);

    const char    *pchStrTab = NULL;
    uint32_t const offStrTab = pSymTab->stroff;
    uint32_t const cbStrTab  = pSymTab->strsize;
    AssertLogRelMsgReturn(cbStrTab > 0 && offStrTab >= sizeof(pHdr) + cbCmds,
                          ("%p: cbStrTab=%#x offStrTab=%#x\n", pHdr, cbStrTab, offStrTab), false);

    uint32_t const *paidxIndirSymbols = NULL;
    uint32_t const  offIndirSymbols = pDySymTab->indirectsymboff;
    uint32_t const  cIndirSymbols   = pDySymTab->nindirectsymb;
    AssertLogRelMsgReturn(cIndirSymbols > 0 && offIndirSymbols >= sizeof(pHdr) + cbCmds,
                          ("%p: cIndirSymbols=%#x offIndirSymbols=%#x\n", pHdr, cIndirSymbols, offIndirSymbols), false);

    offCmd = 0;
    for (uint32_t iCmd = 0; iCmd < cCmds; iCmd++)
    {
        load_command_t const * const pCmd = (load_command_t const *)((uintptr_t)(pHdr + 1) + offCmd);
        uint32_t const cbCurCmd = pCmd->cmdsize;
        AssertLogRelMsgReturn(offCmd + cbCurCmd <= cbCmds && cbCurCmd <= cbCmds,
                              ("%p: iCmd=%u offCmd=%#x cbCurCmd=%#x cbCmds=%#x\n", pHdr, iCmd, offCmd, cbCurCmd, cbCmds), false);
        offCmd += cbCurCmd;

        if (pCmd->cmd == LC_SEGMENT_64)
        {
            segment_command_64_t const *pSeg = (segment_command_64_t const *)pCmd;
            AssertLogRelMsgReturn(cbCurCmd >= sizeof(*pSeg), ("%p: iCmd=%u cbCurCmd=%#x\n", pHdr, iCmd, cbCurCmd), false);
            uintptr_t const uPtrSeg = pSeg->vmaddr + offSlide;
            uint64_t const  cbSeg   = pSeg->vmsize;
            uint64_t const  offFile = pSeg->fileoff;

            uint64_t offSeg = offSymbols - offFile;
            if (offSeg < cbSeg)
            {
                AssertLogRelMsgReturn(!paSymbols, ("%p: paSymbols=%p uPtrSeg=%p off=%#llx\n", pHdr, paSymbols, uPtrSeg, offSeg),
                                      false);
                AssertLogRelMsgReturn(offSeg + cSymbols * sizeof(paSymbols[0]) <= cbSeg,
                                      ("%p: offSeg=%#llx cSymbols=%#x cbSeg=%llx\n", pHdr, offSeg, cSymbols, cbSeg), false);
                paSymbols = (macho_nlist_64_t const *)(uPtrSeg + offSeg);
            }

            offSeg = offStrTab - offFile;
            if (offSeg < cbSeg)
            {
                AssertLogRelMsgReturn(!pchStrTab, ("%p: paSymbols=%p uPtrSeg=%p\n", pHdr, pchStrTab, uPtrSeg), false);
                AssertLogRelMsgReturn(offSeg + cbStrTab <= cbSeg,
                                      ("%p: offSeg=%#llx cbStrTab=%#x cbSeg=%llx\n", pHdr, offSeg, cbStrTab, cbSeg), false);
                pchStrTab = (const char *)(uPtrSeg + offSeg);
            }

            offSeg = offIndirSymbols - offFile;
            if (offSeg < cbSeg)
            {
                AssertLogRelMsgReturn(!paidxIndirSymbols,
                                      ("%p: paidxIndirSymbols=%p uPtrSeg=%p\n", pHdr, paidxIndirSymbols, uPtrSeg), false);
                AssertLogRelMsgReturn(offSeg + cIndirSymbols * sizeof(paidxIndirSymbols[0]) <= cbSeg,
                                      ("%p: offSeg=%#llx cIndirSymbols=%#x cbSeg=%llx\n", pHdr, offSeg, cIndirSymbols, cbSeg),
                                      false);
                paidxIndirSymbols = (uint32_t const *)(uPtrSeg + offSeg);
            }
        }
    }

    AssertLogRelMsgReturn(paSymbols, ("%p: offSymbols=%#x\n", pHdr, offSymbols), false);
    AssertLogRelMsgReturn(pchStrTab, ("%p: offStrTab=%#x\n", pHdr, offStrTab), false);
    AssertLogRelMsgReturn(paidxIndirSymbols, ("%p: offIndirSymbols=%#x\n", pHdr, offIndirSymbols), false);

    /*
     * Third command pass: Process sections of types S_NON_LAZY_SYMBOL_POINTERS
     *                     and S_LAZY_SYMBOL_POINTERS
     */
    bool fFound = false;
    offCmd = 0;
    for (uint32_t iCmd = 0; iCmd < cCmds; iCmd++)
    {
        load_command_t const * const pCmd = (load_command_t const *)((uintptr_t)(pHdr + 1) + offCmd);
        uint32_t const cbCurCmd = pCmd->cmdsize;
        AssertLogRelMsgReturn(offCmd + cbCurCmd <= cbCmds && cbCurCmd <= cbCmds,
                              ("%p: iCmd=%u offCmd=%#x cbCurCmd=%#x cbCmds=%#x\n", pHdr, iCmd, offCmd, cbCurCmd, cbCmds), false);
        offCmd += cbCurCmd;
        if (pCmd->cmd == LC_SEGMENT_64)
        {
            segment_command_64_t const *pSeg = (segment_command_64_t const *)pCmd;
            AssertLogRelMsgReturn(cbCurCmd >= sizeof(*pSeg), ("%p: iCmd=%u cbCurCmd=%#x\n", pHdr, iCmd, cbCurCmd), false);
            uint64_t const  uSegAddr = pSeg->vmaddr;
            uint64_t const  cbSeg    = pSeg->vmsize;

            uint32_t const             cSections  = pSeg->nsects;
            section_64_t const * const paSections = (section_64_t const *)(pSeg + 1);
            AssertLogRelMsgReturn(cSections < _256K && sizeof(*pSeg) + cSections * sizeof(paSections[0]) <= cbCurCmd,
                                  ("%p: iCmd=%u cSections=%#x cbCurCmd=%#x\n", pHdr, iCmd, cSections, cbCurCmd), false);
            for (uint32_t iSection = 0; iSection < cSections; iSection++)
            {
                if (   paSections[iSection].flags == S_NON_LAZY_SYMBOL_POINTERS
                    || paSections[iSection].flags == S_LAZY_SYMBOL_POINTERS)
                {
                    uint32_t const idxIndirBase = paSections[iSection].reserved1;
                    uint32_t const cEntries     = paSections[iSection].size / sizeof(uintptr_t);
                    AssertLogRelMsgReturn(idxIndirBase <= cIndirSymbols && idxIndirBase + cEntries <= cIndirSymbols,
                                          ("%p: idxIndirBase=%#x cEntries=%#x cIndirSymbols=%#x\n",
                                           pHdr, idxIndirBase, cEntries, cIndirSymbols), false);
                    uint64_t const uSecAddr = paSections[iSection].addr;
                    uint64_t const offInSeg = uSecAddr - uSegAddr;
                    AssertLogRelMsgReturn(offInSeg < cbSeg && offInSeg + cEntries * sizeof(uintptr_t) <= cbSeg,
                                          ("%p: offInSeg=%#llx cEntries=%#x cbSeg=%#llx\n", pHdr, offInSeg, cEntries, cbSeg),
                                          false);
                    uintptr_t *pauPtrs = (uintptr_t *)(uSecAddr + offSlide);
                    for (uint32_t iEntry = 0; iEntry < cEntries; iEntry++)
                    {
                        uint32_t const idxSym = paidxIndirSymbols[idxIndirBase + iEntry];
                        if (idxSym < cSymbols)
                        {
                            macho_nlist_64_t const * const pSym    = &paSymbols[idxSym];
                            const char * const             pszName = pSym->n_un.n_strx < cbStrTab
                                                                   ? &pchStrTab[pSym->n_un.n_strx] : "!invalid symtab offset!";
                            if (strcmp(pszName, pszSymbol) == 0)
                            {
                                pauPtrs[iEntry] = uNewValue;
                                fFound = true;
                                break;
                            }
                        }
                        else
                            AssertMsg(idxSym == INDIRECT_SYMBOL_LOCAL || idxSym == INDIRECT_SYMBOL_ABS, ("%#x\n", idxSym));
                    }
                }
            }
        }
    }
    AssertLogRel(fFound);
    return fFound;
}

/**
 * Mac OS X: Really ugly hack to bypass a set-uid check in AppKit.
 *
 * This will modify the issetugid() function to always return zero.  This must
 * be done _before_ AppKit is initialized, otherwise it will refuse to play ball
 * with us as it distrusts set-uid processes since Snow Leopard.  We, however,
 * have carefully dropped all root privileges at this point and there should be
 * no reason for any security concern here.
 */
static void HideSetUidRootFromAppKit()
{
    void *pvAddr;
    /* Find issetguid() and make it always return 0 by modifying the code: */
# if 0
    pvAddr = dlsym(RTLD_DEFAULT, "issetugid");
    int rc = mprotect((void *)((uintptr_t)pvAddr & ~(uintptr_t)0xfff), 0x2000, PROT_WRITE | PROT_READ | PROT_EXEC);
    if (!rc)
        ASMAtomicWriteU32((volatile uint32_t *)pvAddr, 0xccc3c031); /* xor eax, eax; ret; int3 */
    else
# endif
    {
        /* Failing that, find AppKit and patch its import table: */
        void *pvAppKit = dlopen("/System/Library/Frameworks/AppKit.framework/AppKit", RTLD_NOLOAD);
        pvAddr = dlsym(pvAppKit, "NSApplicationMain");
        Dl_info Info = {0};
        if (   dladdr(pvAddr, &Info)
            && Info.dli_fbase != NULL)
        {
            if (!patchExtSym((mach_header_64_t *)Info.dli_fbase, "_issetugid", (uintptr_t)&issetugid_for_AppKit))
                write(2, RT_STR_TUPLE("WARNING: Failed to patch issetugid in AppKit! (patchExtSym)\n"));
# ifdef DEBUG
            else
                write(2, RT_STR_TUPLE("INFO: Successfully patched _issetugid import for AppKit!\n"));
# endif
        }
        else
            write(2, RT_STR_TUPLE("WARNING: Failed to patch issetugid in AppKit! (dladdr)\n"));
    }

}

#endif /* VBOX_WS_MAC */

#ifdef VBOX_WS_X11
/** X11: For versions of Xlib which are aware of multi-threaded environments this function
  *      calls for XInitThreads() which initializes Xlib support for concurrent threads.
  * @returns @c non-zero unless it is unsafe to make multi-threaded calls to Xlib.
  * @remarks This is a workaround for a bug on old Xlib versions, fixed in commit
  *          941f02e and released in Xlib version 1.1. We check for the symbol
  *          "xcb_connect" which was introduced in that version. */
static Status MakeSureMultiThreadingIsSafe()
{
    /* Success by default: */
    Status rc = 1;
    /* Get a global handle to process symbols: */
    void *pvProcess = dlopen(0, RTLD_GLOBAL | RTLD_LAZY);
    /* Initialize multi-thread environment only if we can obtain
     * an address of xcb_connect symbol in this process: */
    if (pvProcess && dlsym(pvProcess, "xcb_connect"))
        rc = XInitThreads();
    /* Close the handle: */
    if (pvProcess)
        dlclose(pvProcess);
    /* Return result: */
    return rc;
}

# if defined(RT_OS_LINUX) && defined(DEBUG)
/** X11, Linux, Debug: The signal handler that prints out a backtrace of the call stack.
  * @remarks The code is taken from http://www.linuxjournal.com/article/6391. */
static void BackTraceSignalHandler(int sig, siginfo_t *pInfo, void *pSecret)
{
    void *trace[16];
    char **messages = (char **)0;
    int i, iTraceSize = 0;
    ucontext_t *uc = (ucontext_t *)pSecret;

    /* Do something useful with siginfo_t: */
    if (sig == SIGSEGV)
        Log(("GUI: Got signal %d, faulty address is %p, from %p\n",
             sig, pInfo->si_addr, uc->uc_mcontext.gregs[REG_PC]));
    /* Or do nothing by default: */
    else
        Log(("GUI: Got signal %d\n", sig));

    /* Acquire backtrace of 16 lvls depth: */
    iTraceSize = backtrace(trace, 16);

    /* Overwrite sigaction with caller's address: */
    trace[1] = (void *)uc->uc_mcontext.gregs[REG_PC];

    /* Translate the addresses into an array of messages: */
    messages = backtrace_symbols(trace, iTraceSize);

    /* Skip first stack frame (points here): */
    Log(("GUI: [bt] Execution path:\n"));
    for (i = 1; i < iTraceSize; ++i)
        Log(("GUI: [bt] %s\n", messages[i]));

    exit(0);
}

/** X11, Linux, Debug: Installs signal handler printing backtrace of the call stack. */
static void InstallSignalHandler()
{
    struct sigaction sa;
    sa.sa_sigaction = BackTraceSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGBUS,  &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
}
# endif /* RT_OS_LINUX && DEBUG */
#endif /* VBOX_WS_X11 */

/** Qt5 message handler, function that prints out
  * debug, warning, critical, fatal and system error messages.
  * @param  type        Holds the type of the message.
  * @param  context     Holds the message context.
  * @param  strMessage  Holds the message body. */
static void QtMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &strMessage)
{
    NOREF(context);
# ifndef VBOX_WS_X11
    NOREF(strMessage);
# endif
    switch (type)
    {
        case QtDebugMsg:
            Log(("Qt DEBUG: %s\n", strMessage.toUtf8().constData()));
            break;
        case QtWarningMsg:
            Log(("Qt WARNING: %s\n", strMessage.toUtf8().constData()));
# ifdef VBOX_WS_X11
            /* Needed for instance for the message ``cannot connect to X server'': */
            RTStrmPrintf(g_pStdErr, "Qt WARNING: %s\n", strMessage.toUtf8().constData());
# endif
            break;
        case QtCriticalMsg:
            Log(("Qt CRITICAL: %s\n", strMessage.toUtf8().constData()));
# ifdef VBOX_WS_X11
            /* Needed for instance for the message ``cannot connect to X server'': */
            RTStrmPrintf(g_pStdErr, "Qt CRITICAL: %s\n", strMessage.toUtf8().constData());
# endif
            break;
        case QtFatalMsg:
            Log(("Qt FATAL: %s\n", strMessage.toUtf8().constData()));
# ifdef VBOX_WS_X11
            /* Needed for instance for the message ``cannot connect to X server'': */
            RTStrmPrintf(g_pStdErr, "Qt FATAL: %s\n", strMessage.toUtf8().constData());
# endif
# if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        case QtInfoMsg:
            /** @todo ignore? */
            break;
# endif
    }
}

/** Shows all available command line parameters. */
static void ShowHelp()
{
    RTPrintf(VBOX_PRODUCT " Manager %s\n"
            "(C) 2005-" VBOX_C_YEAR " " VBOX_VENDOR "\n"
            "All rights reserved.\n"
            "\n"
            "Usage:\n"
            "  --startvm <vmname|UUID>    start a VM by specifying its UUID or name\n"
            "  --separate                 start a separate VM process\n"
            "  --normal                   keep normal (windowed) mode during startup\n"
            "  --fullscreen               switch to fullscreen mode during startup\n"
            "  --seamless                 switch to seamless mode during startup\n"
            "  --scale                    switch to scale mode during startup\n"
            "  --no-startvm-errormsgbox   do not show a message box for VM start errors\n"
            "  --restore-current          restore the current snapshot before starting\n"
            "  --no-aggressive-caching    delays caching media info in VM processes\n"
            "  --fda <image|none>         Mount the specified floppy image\n"
            "  --dvd <image|none>         Mount the specified DVD image\n"
# ifdef VBOX_GUI_WITH_PIDFILE
            "  --pidfile <file>           create a pidfile file when a VM is up and running\n"
# endif /* VBOX_GUI_WITH_PIDFILE */
# ifdef VBOX_WITH_DEBUGGER_GUI
            "  --dbg                      enable the GUI debug menu\n"
            "  --debug                    like --dbg and show debug windows at VM startup\n"
            "  --debug-command-line       like --dbg and show command line window at VM startup\n"
            "  --debug-statistics         like --dbg and show statistics window at VM startup\n"
            "  --no-debug                 disable the GUI debug menu and debug windows\n"
            "  --start-paused             start the VM in the paused state\n"
            "  --start-running            start the VM running (for overriding --debug*)\n"
            "\n"
# endif /* VBOX_WITH_DEBUGGER_GUI */
            "Expert options:\n"
            "  --disable-patm             disable code patching (ignored by AMD-V/VT-x)\n"
            "  --disable-csam             disable code scanning (ignored by AMD-V/VT-x)\n"
            "  --recompile-supervisor     recompiled execution of supervisor code (*)\n"
            "  --recompile-user           recompiled execution of user code (*)\n"
            "  --recompile-all            recompiled execution of all code, with disabled\n"
            "                             code patching and scanning\n"
            "  --execute-all-in-iem       For debugging the interpreted execution mode.\n"
            "  --warp-pct <pct>           time warp factor, 100%% (= 1.0) = normal speed\n"
            "  (*) For AMD-V/VT-x setups the effect is --recompile-all.\n"
            "\n"
# ifdef VBOX_WITH_DEBUGGER_GUI
            "The following environment (and extra data) variables are evaluated:\n"
            "  VBOX_GUI_DBG_ENABLED (GUI/Dbg/Enabled)\n"
            "                             enable the GUI debug menu if set\n"
            "  VBOX_GUI_DBG_AUTO_SHOW (GUI/Dbg/AutoShow)\n"
            "                             show debug windows at VM startup\n"
            "  VBOX_GUI_NO_DEBUGGER       disable the GUI debug menu and debug windows\n"
# endif /* VBOX_WITH_DEBUGGER_GUI */
            "\n",
            RTBldCfgVersion());
    /** @todo Show this as a dialog on windows. */
}

extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char ** /*envp*/)
{
#ifdef RT_OS_WINDOWS
    ATL::CComModule _Module; /* Required internally by ATL (constructor records instance in global variable). */
#endif

    /* Failed result initially: */
    int iResultCode = 1;

    /* Start logging: */
    LogFlowFuncEnter();

    /* Simulate try-catch block: */
    do
    {
#ifdef VBOX_WS_MAC
        /* Hide setuid root from AppKit: */
        HideSetUidRootFromAppKit();
#endif /* VBOX_WS_MAC */

#ifdef VBOX_WS_X11
        /* Make sure multi-threaded environment is safe: */
        if (!MakeSureMultiThreadingIsSafe())
            break;
#endif /* VBOX_WS_X11 */

        /* Console help preprocessing: */
        bool fHelpShown = false;
        for (int i = 0; i < argc; ++i)
        {
            if (   !strcmp(argv[i], "-h")
                || !strcmp(argv[i], "-?")
                || !strcmp(argv[i], "-help")
                || !strcmp(argv[i], "--help"))
            {
                fHelpShown = true;
                ShowHelp();
                break;
            }
        }
        if (fHelpShown)
        {
            iResultCode = 0;
            break;
        }

#ifdef VBOX_WITH_HARDENING
        /* Make sure the image verification code works: */
        SUPR3HardenedVerifyInit();
#endif /* VBOX_WITH_HARDENING */

#ifdef VBOX_WS_MAC
        /* Apply font fixes (before QApplication get created and instantiated font-hints): */
        switch (VBoxGlobal::determineOsRelease())
        {
            case MacOSXRelease_Mavericks: QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande"); break;
            case MacOSXRelease_Yosemite:  QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Helvetica Neue"); break;
            case MacOSXRelease_ElCapitan: QFont::insertSubstitution(".SF NS Text", "Helvetica Neue"); break;
            default: break;
        }

        /* Instantiate own NSApplication before QApplication do it for us: */
        UICocoaApplication::instance();

# ifdef VBOX_RUNTIME_UI
        /* If we're a helper app inside Resources in the main application bundle,
           we need to amend the library path so the platform plugin can be found.
           Note! This builds on the initIprtForDarwinHelperApp() hack. */
        {
            char szExecDir[RTPATH_MAX];
            int vrc = RTPathExecDir(szExecDir, sizeof(szExecDir));
            AssertRC(vrc);
            RTPathStripTrailingSlash(szExecDir); /* .../Contents/MacOS */
            RTPathStripFilename(szExecDir);      /* .../Contents */
            RTPathAppend(szExecDir, sizeof(szExecDir), "plugins");      /* .../Contents/plugins */
            QCoreApplication::addLibraryPath(QString::fromUtf8(szExecDir));
        }
# endif
#endif /* VBOX_WS_MAC */

#ifdef VBOX_WS_X11
# if defined(RT_OS_LINUX) && defined(DEBUG)
        /* Install signal handler to backtrace the call stack: */
        InstallSignalHandler();
# endif /* RT_OS_LINUX && DEBUG */
#endif /* VBOX_WS_X11 */

        /* Install Qt console message handler: */
        qInstallMessageHandler(QtMessageOutput);

        /* Create application: */
        QApplication a(argc, argv);

#ifdef VBOX_WS_WIN
        /* Drag in the sound drivers and DLLs early to get rid of the delay taking
         * place when the main menu bar (or any action from that menu bar) is
         * activated for the first time. This delay is especially annoying if it
         * happens when the VM is executing in real mode (which gives 100% CPU
         * load and slows down the load process that happens on the main GUI
         * thread to several seconds). */
        PlaySound(NULL, NULL, 0);
#endif /* VBOX_WS_WIN */

#ifdef VBOX_WS_MAC
        /* Enable HiDPI icons: */
        a.setAttribute(Qt::AA_UseHighDpiPixmaps);

        /* Disable menu icons on MacOS X host: */
        ::darwinDisableIconsInMenus();
#endif /* VBOX_WS_MAC */

#ifdef VBOX_WS_X11
        /* Make all widget native.
         * We did it to avoid various Qt crashes while testing widget attributes or acquiring winIds.
         * Yes, we aware of note that alien widgets faster to draw but the only widget we need to be fast
         * is viewport of VM which was always native since we are using his id for 3D service needs. */
        a.setAttribute(Qt::AA_NativeWindows);

# ifdef Q_OS_SOLARIS
        a.setStyle("fusion");
# endif /* Q_OS_SOLARIS */

# ifndef Q_OS_SOLARIS
        /* Apply font fixes (after QApplication get created and instantiated font-family): */
        QFontDatabase fontDataBase;
        QString currentFamily(QApplication::font().family());
        bool isCurrentScaleable = fontDataBase.isScalable(currentFamily);
        QString subFamily(QFont::substitute(currentFamily));
        bool isSubScaleable = fontDataBase.isScalable(subFamily);
        if (isCurrentScaleable && !isSubScaleable)
            QFont::removeSubstitutions(currentFamily);
# endif /* !Q_OS_SOLARIS */

        /* Qt version check (major.minor are sensitive, fix number is ignored): */
        if (VBoxGlobal::qtRTVersion() < (VBoxGlobal::qtCTVersion() & 0xFFFF00))
        {
            QString strMsg = QApplication::tr("Executable <b>%1</b> requires Qt %2.x, found Qt %3.")
                                              .arg(qAppName())
                                              .arg(VBoxGlobal::qtCTVersionString().section('.', 0, 1))
                                              .arg(VBoxGlobal::qtRTVersionString());
            QMessageBox::critical(0, QApplication::tr("Incompatible Qt Library Error"),
                                  strMsg, QMessageBox::Abort, 0);
            qFatal("%s", strMsg.toUtf8().constData());
            break;
        }
#endif /* VBOX_WS_X11 */

        /* Create modal-window manager: */
        UIModalWindowManager::create();

        /* Create global UI instance: */
        VBoxGlobal::create();

        /* Simulate try-catch block: */
        do
        {
            /* Exit if VBoxGlobal is not valid: */
            if (!vboxGlobal().isValid())
                break;

            /* Exit if VBoxGlobal pre-processed arguments: */
            if (vboxGlobal().processArgs())
                break;

            /* For Runtime UI: */
            if (vboxGlobal().isVMConsoleProcess())
            {
                /* Prevent application from exiting when all window(s) closed: */
                qApp->setQuitOnLastWindowClosed(false);
            }

            /* Request to Show UI _after_ QApplication started: */
            QMetaObject::invokeMethod(&vboxGlobal(), "showUI", Qt::QueuedConnection);

            /* Start application: */
            iResultCode = a.exec();
        }
        while (0);

        /* Destroy global UI instance: */
        VBoxGlobal::destroy();

        /* Destroy modal-window manager: */
        UIModalWindowManager::destroy();
    }
    while (0);

    /* Finish logging: */
    LogFlowFunc(("rc=%d\n", iResultCode));
    LogFlowFuncLeave();

    /* Return result: */
    return iResultCode;
}

#ifndef VBOX_WITH_HARDENING

# ifdef RT_OS_DARWIN
/** Init runtime with the executable path pointing into the
 * VirtualBox.app/Contents/MacOS/ rather than
 * VirtualBox.app/Contents/Resource/VirtualBoxVM.app/Contents/MacOS/ for the
 * VirtualBoxVM helper app.
 *
 * This is a HACK to make codesign and friends happy on OS X.   The idea is to
 * improve and eliminate this over time.
 */
DECL_NO_INLINE(static, int) initIprtForDarwinTakingHelperAppIntoAccount(int cArgs, char ***ppapszArgs, uint32_t fFlags)
{
    const char *pszImageName = _dyld_get_image_name(0);
    AssertReturn(pszImageName, VERR_INTERNAL_ERROR);

    char szTmpPath[PATH_MAX + 1];
    const char *psz = realpath(pszImageName, szTmpPath);
    int rc;
    if (psz)
    {
        char *pszFilename = RTPathFilename(szTmpPath);
        if (pszFilename)
        {
            /* Is it the helper VM process? */
            if (   RTStrICmp(pszFilename, "VirtualBoxVM") == 0
                && RTStrIStr(szTmpPath, "VirtualBoxVM.app") != NULL)
            {
                char const chSavedFilename0 = *pszFilename;
                *pszFilename = '\0';
                RTPathStripTrailingSlash(szTmpPath); /* VirtualBox.app/Contents/Resources/VirtualBoxVM.app/Contents/MacOS */
                RTPathStripFilename(szTmpPath);      /* VirtualBox.app/Contents/Resources/VirtualBoxVM.app/Contents/ */
                RTPathStripFilename(szTmpPath);      /* VirtualBox.app/Contents/Resources/VirtualBoxVM.app */
                RTPathStripFilename(szTmpPath);      /* VirtualBox.app/Contents/Resources */
                RTPathStripFilename(szTmpPath);      /* VirtualBox.app/Contents */
                char *pszDst = strchr(szTmpPath, '\0');
                pszDst = (char *)memcpy(pszDst, RT_STR_TUPLE("/MacOS/")) + sizeof("/MacOS/") - 1; /** @todo where is mempcpy? */
                *pszFilename = chSavedFilename0;
                memmove(pszDst, pszFilename, strlen(pszFilename) + 1);

                return RTR3InitEx(RTR3INIT_VER_CUR, fFlags, cArgs, ppapszArgs, szTmpPath);
            }

            /* It's the main application: */
            return RTR3InitExe(cArgs, ppapszArgs, fFlags);
        }
        rc = VERR_INVALID_NAME;
    }
    else
        rc = RTErrConvertFromErrno(errno);
    AssertMsgRCReturn(rc, ("rc=%Rrc pszLink=\"%s\"\nhex: %.*Rhxs\n", rc, pszImageName, strlen(pszImageName), pszImageName), rc);
    return rc;
}
# endif


int main(int argc, char **argv, char **envp)
{
# ifdef VBOX_WS_X11
    /* Make sure multi-threaded environment is safe: */
    if (!MakeSureMultiThreadingIsSafe())
        return 1;
# endif /* VBOX_WS_X11 */

    /* Initialize VBox Runtime.
     * Initialize the SUPLib as well only if we are really about to start a VM.
     * Don't do this if we are only starting the selector window or a separate VM process. */
    bool fStartVM = false;
    bool fSeparateProcess = false;
    for (int i = 1; i < argc && !(fStartVM && fSeparateProcess); ++i)
    {
        /* NOTE: the check here must match the corresponding check for the
         * options to start a VM in hardenedmain.cpp and VBoxGlobal.cpp exactly,
         * otherwise there will be weird error messages. */
        if (   !::strcmp(argv[i], "--startvm")
            || !::strcmp(argv[i], "-startvm"))
        {
            fStartVM = true;
        }
        else if (   !::strcmp(argv[i], "--separate")
                 || !::strcmp(argv[i], "-separate"))
        {
            fSeparateProcess = true;
        }
    }

    uint32_t fFlags = fStartVM && !fSeparateProcess ? RTR3INIT_FLAGS_SUPLIB : 0;
# ifdef RT_OS_DARWIN
    int rc = initIprtForDarwinTakingHelperAppIntoAccount(argc, &argv, fFlags);
# else
    int rc = RTR3InitExe(argc, &argv, fFlags);
# endif

    /* Initialization failed: */
    if (RT_FAILURE(rc))
    {
        /* We have to create QApplication anyway
         * just to show the only one error-message: */
        QApplication a(argc, &argv[0]);
        Q_UNUSED(a);

# ifdef Q_OS_SOLARIS
        a.setStyle("fusion");
# endif /* Q_OS_SOLARIS */

        /* Prepare the error-message: */
        QString strTitle = QApplication::tr("VirtualBox - Runtime Error");
        QString strText = "<html>";
        switch (rc)
        {
            case VERR_VM_DRIVER_NOT_INSTALLED:
            case VERR_VM_DRIVER_LOAD_ERROR:
                strText += QApplication::tr("<b>Cannot access the kernel driver!</b><br/><br/>");
# ifdef RT_OS_LINUX
                strText += g_QStrHintLinuxNoDriver;
# else /* RT_OS_LINUX */
                strText += g_QStrHintOtherNoDriver;
# endif /* !RT_OS_LINUX */
                break;
# ifdef RT_OS_LINUX
            case VERR_NO_MEMORY:
                strText += g_QStrHintLinuxNoMemory;
                break;
# endif /* RT_OS_LINUX */
            case VERR_VM_DRIVER_NOT_ACCESSIBLE:
                strText += QApplication::tr("Kernel driver not accessible");
                break;
            case VERR_VM_DRIVER_VERSION_MISMATCH:
# ifdef RT_OS_LINUX
                strText += g_QStrHintLinuxWrongDriverVersion;
# else /* RT_OS_LINUX */
                strText += g_QStrHintOtherWrongDriverVersion;
# endif /* !RT_OS_LINUX */
                break;
            default:
                strText += QApplication::tr("Unknown error %2 during initialization of the Runtime").arg(rc);
                break;
        }
        strText += "</html>";

        /* Show the error-message: */
        QMessageBox::critical(0 /* parent */, strTitle, strText,
                              QMessageBox::Abort /* 1st button */, 0 /* 2nd button */);

        /* Default error-result: */
        return 1;
    }

    /* Actual main function: */
    return TrustedMain(argc, argv, envp);
}

#else  /* VBOX_WITH_HARDENING */


/**
 * Special entrypoint used by the hardening code when something goes south.
 *
 * Display an error dialog to the user.
 *
 * @param   pszWhere    Indicates where the error occured.
 * @param   enmWhat     Indicates what init operation was going on at the time.
 * @param   rc          The VBox status code corresponding to the error.
 * @param   pszMsgFmt   The message format string.
 * @param   va          Format arguments.
 */
extern "C" DECLEXPORT(void) TrustedError(const char *pszWhere, SUPINITOP enmWhat, int rc, const char *pszMsgFmt, va_list va)
{
# ifdef VBOX_WS_MAC
    /* Hide setuid root from AppKit: */
    HideSetUidRootFromAppKit();
# endif /* VBOX_WS_MAC */

    char szMsgBuf[_16K];

    /*
     * We have to create QApplication anyway just to show the only one error-message.
     * This is a bit hackish as we don't have the argument vector handy.
     */
    int argc = 0;
    char *argv[2] = { NULL, NULL };
    QApplication a(argc, &argv[0]);

    /*
     * The details starts off a properly formatted rc and where/what, we use
     * the szMsgBuf for this, thus this have to come before the actual message
     * formatting.
     */
    RTStrPrintf(szMsgBuf, sizeof(szMsgBuf),
                "<!--EOM-->"
                "where: %s\n"
                "what:  %d\n"
                "%Rra\n",
                pszWhere, enmWhat, rc);
    QString strDetails = szMsgBuf;

    /*
     * Format the error message. Take whatever comes after a double new line as
     * something better off in the details section.
     */
    RTStrPrintfV(szMsgBuf, sizeof(szMsgBuf), pszMsgFmt, va);

    char *pszDetails = strstr(szMsgBuf, "\n\n");
    if (pszDetails)
    {
        while (RT_C_IS_SPACE(*pszDetails))
            *pszDetails++ = '\0';
        if (*pszDetails)
        {
            strDetails += "\n";
            strDetails += pszDetails;
        }
        RTStrStripR(szMsgBuf);
    }

    QString strText = QApplication::tr("<html><b>%1 (rc=%2)</b><br/><br/>").arg(szMsgBuf).arg(rc);
    strText.replace(QString("\n"), QString("<br>"));

    /*
     * Append possibly helpful hints to the error message.
     */
    switch (enmWhat)
    {
        case kSupInitOp_Driver:
# ifdef RT_OS_LINUX
            strText += g_QStrHintLinuxNoDriver;
# else /* RT_OS_LINUX */
            strText += g_QStrHintOtherNoDriver;
# endif /* !RT_OS_LINUX */
            break;
        case kSupInitOp_IPRT:
        case kSupInitOp_Misc:
            if (rc == VERR_VM_DRIVER_VERSION_MISMATCH)
# ifndef RT_OS_LINUX
                strText += g_QStrHintOtherWrongDriverVersion;
# else
                strText += g_QStrHintLinuxWrongDriverVersion;
            else if (rc == VERR_NO_MEMORY)
                strText += g_QStrHintLinuxNoMemory;
# endif
            else
                strText += g_QStrHintReinstall;
            break;
        case kSupInitOp_Integrity:
        case kSupInitOp_RootCheck:
            strText += g_QStrHintReinstall;
            break;
        default:
            /* no hints here */
            break;
    }

# ifdef VBOX_WS_X11
    /* We have to to make sure that we display the error-message
     * after the parent displayed its own message. */
    sleep(2);
# endif /* VBOX_WS_X11 */

    /* Update strText with strDetails: */
    if (!strDetails.isEmpty())
        strText += QString("<br><br>%1").arg(strDetails);

    /* Close the <html> scope: */
    strText += "</html>";

    /* Create and show the error message-box: */
    QMessageBox::critical(0, QApplication::tr("VirtualBox - Error In %1").arg(pszWhere), strText);

    qFatal("%s", strText.toUtf8().constData());
}

#endif /* VBOX_WITH_HARDENING */

