/* $Id: prfreader.cpp.h 77 2016-06-22 17:03:55Z bird $ */
/** @file
 * kProfiler Mark 2 - Reader Code Template.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * Validates the non-header parts of a data-set.
 *
 * @returns true if valid.
 * @returns false if invalid. (written description to pOut)
 *
 * @param   pHdr        Pointer to the data set.
 * @param   cb          The size of the data set.
 * @param   pOut        Where to write error messages.
 */
static bool KPRF_NAME(IsValid)(KPRF_TYPE(PC,HDR) pHdr, KU32 cb, FILE *pOut)
{
    KPRF_TYPE(,UPTR) uMaxPtr = ~(KPRF_TYPE(,UPTR))0 - pHdr->uBasePtr;

    /*
     * Iterate the module segments.
     */
    KU32 off = pHdr->offModSegs;
    while (off < pHdr->offModSegs + pHdr->cbModSegs)
    {
        KPRF_TYPE(PC,MODSEG) pCur = KPRF_OFF2PTR(PC,MODSEG, off, pHdr);
        KU32 cbCur = KPRF_OFFSETOF(MODSEG, szPath[pCur->cchPath + 1]);
        cbCur = KPRF_ALIGN(cbCur, KPRF_SIZEOF(UPTR));
        if (cbCur + off > pHdr->offModSegs + pHdr->cbModSegs)
        {
            fprintf(pOut, "The module segment record at 0x%x is too long!\n", off);
            return false;
        }
        if (pCur->uBasePtr > uMaxPtr)
            fprintf(pOut, "warning: The module segment record at 0x%x has a too high base address.\n", off);

        if (strlen(pCur->szPath) != pCur->cchPath)
        {
            fprintf(pOut, "The module segment record at 0x%x has an invalid path length 0x%x it the actual length is 0x%x\n",
                    off, pCur->cchPath, strlen(pCur->szPath));
            return false;
        }

        /* next */
        off += cbCur;
    }


    /*
     * Iterate the functions.
     */
    KPRF_TYPE(PC,FUNC) paFuncs = KPRF_OFF2PTR(PC,FUNC, pHdr->offFunctions, pHdr);
    for (KU32 i = 0; i < pHdr->cFunctions; i++)
    {
        KPRF_TYPE(PC,FUNC) pCur = &paFuncs[i];
        if (pCur->uEntryPtr > uMaxPtr)
            fprintf(pOut, "warning: Function 0x%x has a too high base address.\n", i);
        if (pCur->offModSeg)
        {
            if (    pCur->offModSeg < pHdr->offModSegs
                ||  pCur->offModSeg >= pHdr->offModSegs + pHdr->cbModSegs
                ||  pCur->offModSeg != KPRF_ALIGN(pCur->offModSeg, sizeof(pCur->uEntryPtr))
               )
            {
                fprintf(pOut, "Function 0x%x has an invalid offModSeg value (0x%x).\n", i, pCur->offModSeg);
                return false;
            }
            /** @todo more validation here.. */
        }
    }


    /*
     * Validate the threads.
     */
    KPRF_TYPE(PC,THREAD) paThreads = KPRF_OFF2PTR(PC,THREAD, pHdr->offThreads, pHdr);
    for (KU32 i = 0; i < pHdr->cThreads; i++)
    {
        KPRF_TYPE(PC,THREAD) pCur = &paThreads[i];
        if (pCur->uStackBasePtr > uMaxPtr)
            fprintf(pOut, "warning: Thread 0x%x has a too high base address.\n", i);
        switch (pCur->enmState)
        {
            case KPRF_TYPE(,THREADSTATE_ACTIVE):
            case KPRF_TYPE(,THREADSTATE_SUSPENDED):
            case KPRF_TYPE(,THREADSTATE_OVERFLOWED):
            case KPRF_TYPE(,THREADSTATE_TERMINATED):
                break;
            default:
                fprintf(pOut, "Thread 0x%x has an invalid state value (0x%x).\n", i, pCur->enmState);
                return false;
        }
    }


    return true;
}


/**
 * Dumps a file of a particular format.
 *
 * @returns 0 on success. (you might want to check the pOut state)
 * @returns -1 on failure.
 *
 * @param   pHdr        Pointer to the data set.
 * @param   pOut        The output file. This is opened for text writing.
 * @param   pReader     The reader object.
 */
static int KPRF_NAME(Dump)(KPRF_TYPE(PC,HDR) pHdr, FILE *pOut)
{
    /*
     * Any commandline?
     */
    if (pHdr->offCommandLine)
        fprintf(pOut,
                "Commandline: %s (%d  bytes)\n",
                (char *)KPRF_OFF2PTR(PC,MODSEG, pHdr->offCommandLine, pHdr), /* stupid, stupid, type hacking. */
                pHdr->cchCommandLine);

    /*
     * Dump the module segments.
     */
    fprintf(pOut,
            "Module Segments: off=0x%x 0x%x/0x%x (bytes)\n"
            "----------------\n",
            pHdr->offModSegs, pHdr->cbModSegs, pHdr->cbMaxModSegs);
    KU32 off = pHdr->offModSegs;
    while (off < pHdr->offModSegs + pHdr->cbModSegs)
    {
        KPRF_TYPE(PC,MODSEG) pCur = KPRF_OFF2PTR(PC,MODSEG, off, pHdr);
        KU32 cbCur = KPRF_OFFSETOF(MODSEG, szPath[pCur->cchPath + 1]);
        cbCur = KPRF_ALIGN(cbCur, KPRF_SIZEOF(UPTR));

        fprintf(pOut,
                "0x%04x: iSegment=0x%08x uBasePtr=%" KPRF_FMT_UPTR " szPath='%s' (%d bytes)\n",
                off, pCur->iSegment, pCur->uBasePtr, pCur->szPath, pCur->cchPath);

        /* next */
        off += cbCur;
    }
    fprintf(pOut, "\n");

    /*
     * Dump the functions.
     */
    fprintf(pOut,
            "Functions: off=0x%x 0x%x/0x%x\n"
            "----------\n",
            pHdr->offFunctions, pHdr->cFunctions, pHdr->cMaxFunctions);
    KPRF_TYPE(PC,FUNC) paFuncs = KPRF_OFF2PTR(PC,FUNC, pHdr->offFunctions, pHdr);
    for (KU32 i = 0; i < pHdr->cFunctions; i++)
    {
        KPRF_TYPE(PC,FUNC) pCur = &paFuncs[i];
        fprintf(pOut, "0x%04x: uEntryPtr=%" KPRF_FMT_UPTR " cOnStack=0x%" KPRF_FMT_X64 " cCalls=0x%" KPRF_FMT_X64 "\n"
                      "       OnStack={0x%" KPRF_FMT_X64 ", 0x%" KPRF_FMT_X64 ", 0x%" KPRF_FMT_X64 "}\n"
                      "  OnTopOfStack={0x%" KPRF_FMT_X64 ", 0x%" KPRF_FMT_X64 ", 0x%" KPRF_FMT_X64 "}\n",
                i, pCur->uEntryPtr, pCur->cOnStack, pCur->cCalls,
                pCur->OnStack.MinTicks, pCur->OnStack.MaxTicks,  pCur->OnStack.SumTicks,
                pCur->OnTopOfStack.MinTicks, pCur->OnTopOfStack.MaxTicks,  pCur->OnTopOfStack.SumTicks);
        if (pCur->offModSeg)
        {
            KPRF_TYPE(PC,MODSEG) pModSeg = KPRF_OFF2PTR(PC,MODSEG, pCur->offModSeg, pHdr);
            fprintf(pOut, "  offModSeg=0x%08x iSegment=0x%02x uBasePtr=%" KPRF_FMT_UPTR " szPath='%s' (%d bytes)\n",
                    pCur->offModSeg, pModSeg->iSegment, pModSeg->uBasePtr, pModSeg->szPath, pModSeg->cchPath);

#if 1
            PKDBGMOD pMod;
            int rc = kDbgModuleOpen(&pMod, pModSeg->szPath, NULL /* pLdrMod */);
            if (!rc)
            {
                KDBGSYMBOL Sym;
                rc = kDbgModuleQuerySymbol(pMod, pModSeg->iSegment, pCur->uEntryPtr - pModSeg->uBasePtr, &Sym);
                if (!rc)
                {
                    fprintf(pOut, "  %s\n", Sym.szName);
                }
                kDbgModuleClose(pMod);
            }
#endif

        }
    }
    fprintf(pOut, "\n");

    /*
     * Dump the threads.
     */
    fprintf(pOut,
            "Threads: off=0x%x 0x%x/0x%x (Stacks=0x%x/0x%x cMaxStackFrames=0x%x)\n"
            "--------\n",
            pHdr->offThreads, pHdr->cThreads, pHdr->cMaxThreads, pHdr->cStacks, pHdr->cMaxStacks, pHdr->cMaxStackFrames);
    KPRF_TYPE(PC,THREAD) paThreads = KPRF_OFF2PTR(PC,THREAD, pHdr->offThreads, pHdr);
    for (KU32 i = 0; i < pHdr->cThreads; i++)
    {
        KPRF_TYPE(PC,THREAD) pCur = &paThreads[i];
        fprintf(pOut,
                "0x%02x: ThreadId=0x%08" KPRF_FMT_X64 " enmState=%d szName='%s'\n"
                "  uStackBasePtr=%" KPRF_FMT_UPTR " cbMaxStack=%" KPRF_FMT_UPTR "\n"
                "  cCalls=0x%" KPRF_FMT_X64 " cOverflows=0x%" KPRF_FMT_X64 " cStackSwitchRejects=0x%" KPRF_FMT_X64 "\n"
                "  cUnwinds=0x%" KPRF_FMT_X64 " ProfiledTicks=0x%" KPRF_FMT_X64 " OverheadTicks=0x%" KPRF_FMT_X64 "\n",
                i, pCur->ThreadId, pCur->enmState, pCur->szName,
                pCur->uStackBasePtr, pCur->cbMaxStack,
                pCur->cCalls, pCur->cOverflows, pCur->cStackSwitchRejects,
                pCur->cUnwinds, pCur->ProfiledTicks, pCur->OverheadTicks);
    }

    return 0;
}


/** Pointer to a report module.
 * @internal */
typedef struct KPRF_TYPE(,REPORTMOD) *KPRF_TYPE(P,REPORTMOD);
/** Pointer to a report module segment.
 * @internal */
typedef struct KPRF_TYPE(,REPORTMODSEG) *KPRF_TYPE(P,REPORTMODSEG);


/**
 * A report module segment.
 *
 * @internal
 */
typedef struct KPRF_TYPE(,REPORTMODSEG)
{
    /** AVL node core. The key is the data set offset of the module segment record. */
    KDBGADDR                    offSegment;
    struct KPRF_TYPE(,REPORTMODSEG) *mpLeft;    /**< AVL left branch. */
    struct KPRF_TYPE(,REPORTMODSEG) *mpRight;   /**< AVL rigth branch. */
    /** Pointer to the next segment for the module. */
    KPRF_TYPE(P,REPORTMODSEG)   pNext;
    /** Pointer to the module segment data in the data set. */
    KPRF_TYPE(PC,MODSEG)        pModSeg;
    /** Pointer to the module this segment belongs to. */
    KPRF_TYPE(P,REPORTMOD)      pMod;
    /** The time this segment has spent on the stack.. */
    KU64                        OnStackTicks;
    /** The time this segment has spent on the top of the stack.. */
    KU64                        OnTopOfStackTicks;
    /** The number of profiled functions from this segment. */
    KU32                        cFunctions;
    KU8                         mHeight;        /**< AVL Subtree height. */
} KPRF_TYPE(,REPORTMODSEG), *KPRF_TYPE(P,REPORTMODSEG);


/**
 * A report module segment.
 *
 * @internal
 */
typedef struct KPRF_TYPE(,REPORTMOD)
{
    /** The module number. */
    KU32                        iMod;
    /** Pointer to the next module in the list. */
    KPRF_TYPE(P,REPORTMOD)      pNext;
    /** Pointer to the list of segments belonging to this module. */
    KPRF_TYPE(P,REPORTMODSEG)   pFirstSeg;
    /** The debug module handle. */
    PKDBGMOD                    pDbgMod;
    /** The time this segment has spent on the stack.. */
    KU64                        OnStackTicks;
    /** The time this segment has spent on the top of the stack.. */
    KU64                        OnTopOfStackTicks;
    /** The number of profiled functions from this segment. */
    KU32                        cFunctions;
} KPRF_TYPE(,REPORTMOD), *KPRF_TYPE(P,REPORTMOD);


/**
 * A report function.
 *
 * @internal
 */
typedef struct KPRF_TYPE(,REPORTFUNC)
{
    /** Pointer to the function data in the data set. */
    KPRF_TYPE(PC,FUNC)          pFunc;
    /** Pointer to the module segment this function belongs to. (can be NULL) */
    KPRF_TYPE(P,REPORTMODSEG)   pModSeg;
    /** Pointer to the function symbol. */
    PKDBGSYMBOL                 pSym;
    /** Pointer to the function line number. */
    PKDBGLINE                   pLine;
} KPRF_TYPE(,REPORTFUNC), *KPRF_TYPE(P,REPORTFUNC);


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack time.
 */
static int KPRF_NAME(FuncCompareOnStack)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnStack.SumTicks > p2->OnStack.SumTicks)
        return -1;
    if (p1->OnStack.SumTicks < p2->OnStack.SumTicks)
        return 1;
    if (p1->OnStack.MaxTicks > p2->OnStack.MaxTicks)
        return -1;
    if (p1->OnStack.MaxTicks < p2->OnStack.MaxTicks)
        return 1;
    if (p1->OnStack.MinTicks > p2->OnStack.MinTicks)
        return -1;
    if (p1->OnStack.MinTicks < p2->OnStack.MinTicks)
        return 1;
    if (p1 < p2)
        return -1;
    return 1;
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack average time.
 */
static int KPRF_NAME(FuncCompareOnStackAvg)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnStack.SumTicks / p1->cOnStack > p2->OnStack.SumTicks / p2->cOnStack)
        return -1;
    if (p1->OnStack.SumTicks / p1->cOnStack < p2->OnStack.SumTicks / p2->cOnStack)
        return 1;
    return KPRF_NAME(FuncCompareOnStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack min time.
 */
static int KPRF_NAME(FuncCompareOnStackMin)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnStack.MinTicks > p2->OnStack.MinTicks)
        return -1;
    if (p1->OnStack.MinTicks < p2->OnStack.MinTicks)
        return 1;
    return KPRF_NAME(FuncCompareOnStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack max time.
 */
static int KPRF_NAME(FuncCompareOnStackMax)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnStack.MaxTicks > p2->OnStack.MaxTicks)
        return -1;
    if (p1->OnStack.MaxTicks < p2->OnStack.MaxTicks)
        return 1;
    return KPRF_NAME(FuncCompareOnStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack time.
 */
static int KPRF_NAME(FuncCompareOnTopOfStack)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnTopOfStack.SumTicks > p2->OnTopOfStack.SumTicks)
        return -1;
    if (p1->OnTopOfStack.SumTicks < p2->OnTopOfStack.SumTicks)
        return 1;
    if (p1->OnTopOfStack.MaxTicks > p2->OnTopOfStack.MaxTicks)
        return -1;
    if (p1->OnTopOfStack.MaxTicks < p2->OnTopOfStack.MaxTicks)
        return 1;
    if (p1->OnTopOfStack.MinTicks > p2->OnTopOfStack.MinTicks)
        return -1;
    if (p1->OnTopOfStack.MinTicks < p2->OnTopOfStack.MinTicks)
        return 1;
    if (p1 < p2)
        return -1;
    return 1;
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack average time.
 */
static int KPRF_NAME(FuncCompareOnTopOfStackAvg)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnTopOfStack.SumTicks / p1->cOnStack > p2->OnTopOfStack.SumTicks / p2->cOnStack)
        return -1;
    if (p1->OnTopOfStack.SumTicks / p1->cOnStack < p2->OnTopOfStack.SumTicks / p2->cOnStack)
        return 1;
    return KPRF_NAME(FuncCompareOnTopOfStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack min time.
 */
static int KPRF_NAME(FuncCompareOnTopOfStackMin)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnTopOfStack.MinTicks > p2->OnTopOfStack.MinTicks)
        return -1;
    if (p1->OnTopOfStack.MinTicks < p2->OnTopOfStack.MinTicks)
        return 1;
    return KPRF_NAME(FuncCompareOnTopOfStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher on-stack min time.
 */
static int KPRF_NAME(FuncCompareOnTopOfStackMax)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->OnTopOfStack.MaxTicks > p2->OnTopOfStack.MaxTicks)
        return -1;
    if (p1->OnTopOfStack.MaxTicks < p2->OnTopOfStack.MaxTicks)
        return 1;
    return KPRF_NAME(FuncCompareOnTopOfStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher call to count.
 */
static int KPRF_NAME(FuncCompareCallsTo)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->cOnStack > p2->cOnStack)
        return -1;
    if (p1->cOnStack < p2->cOnStack)
        return 1;
    return KPRF_NAME(FuncCompareOnStack)(pv1, pv2);
}


/**
 * Compares two REPROTFUNC records to determin which has the higher call from count.
 */
static int KPRF_NAME(FuncCompareCallsFrom)(const void *pv1, const void *pv2)
{
    KPRF_TYPE(PC,FUNC) p1 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv1)->pFunc;
    KPRF_TYPE(PC,FUNC) p2 = (*(KPRF_TYPE(P,REPORTFUNC) *)pv2)->pFunc;
    if (p1->cCalls > p2->cCalls)
        return -1;
    if (p1->cCalls < p2->cCalls)
        return 1;
    return KPRF_NAME(FuncCompareOnTopOfStack)(pv1, pv2);
}


/**
 * A report thread.
 *
 * @internal
 */
typedef struct KPRF_TYPE(,REPORTTHREAD)
{
    /** Pointer to the thread data in the data set. */
    KPRF_TYPE(PC,THREAD)        pThread;
} KPRF_TYPE(,REPORTTHREAD), *KPRF_TYPE(P,REPORTTHREAD);


/**
 * Data-set analysis report.
 *
 * This is an internal structure to store temporary data between the
 * analysis stage and the priting state.
 *
 * @internal
 */
typedef struct KPRF_TYPE(,REPORT)
{
    /** Pointer to the data set. */
    KPRF_TYPE(PC,HDR)           pHdr;

    /** @name Data-set item wrappers.
     * @{ */
    /** Pointer to the array of threads. */
    KPRF_TYPE(P,REPORTTHREAD)   paThreads;
    /** Pointer to the array of functions. */
    KPRF_TYPE(P,REPORTFUNC)     paFunctions;
    /** Pointer to the head of the module list. */
    KPRF_TYPE(P,REPORTMOD)      pFirstMod;
    /** The number of modules in the list. */
    KU32                        cMods;
    /** The module segment tree. (Only kAvl cares about this.) */
    KPRF_TYPE(P,REPORTMODSEG)   pModSegTree;
    /** The number of module segments in the tree. */
    KU32                        cModSegs;
    /** @} */

    /** @name Sorting.
     * @{ */
    /** Pointer to the array of threads. */
    KPRF_TYPE(P,REPORTTHREAD)  *papSortedThreads;
    /** Pointer to the array of functions. */
    KPRF_TYPE(P,REPORTFUNC)    *papSortedFunctions;
    /** @} */

    /** @name Accumulated Thread Data.
     * @{ */
    /** Sum of the profiled ticks. */
    KU64                        ProfiledTicks;
    /** Sum of the overhead ticks. */
    KU64                        OverheadTicks;
    /** Sum of the sleep ticks. */
    KU64                        SleepTicks;
    /** Sum of calls performed. */
    KU64                        cCalls;
    /** @} */

} KPRF_TYPE(,REPORT), *KPRF_TYPE(P,REPORT), **KPRF_TYPE(PP,REPORT);


/* Instantiate the AVL tree code. */
#define KAVL_CHECK_FOR_EQUAL_INSERT
#define KAVL_MAX_STACK          32
#define KAVL_STD_KEY_COMP
#define mKey                    offSegment
#define KAVLKEY                 KDBGADDR
#define KAVLNODE                KPRF_TYPE(,REPORTMODSEG)
#define mpRoot                  pModSegTree
#define KAVLROOT                KPRF_TYPE(,REPORT)
#define KAVL_FN(name)           KPRF_NAME(ReportTree ## name)
#define KAVL_TYPE(prefix,name)  KPRF_TYPE(prefix, REPORTMODESEG ## name)
#define KAVL_INT(name)          KPRF_NAME(REPORTMODESEGINT ## name)
#define KAVL_DECL(type)         K_DECL_INLINE(type)
#include <k/kAvlTmpl/kAvlBase.h>
#include <k/kAvlTmpl/kAvlDestroy.h>
#include <k/kAvlTmpl/kAvlGet.h>
#include <k/kAvlTmpl/kAvlUndef.h>


/**
 * Allocates and initializes a report.
 *
 * @returns Pointer to the report on success.
 * @returns NULL on failure.
 */
static KPRF_TYPE(P,REPORT) KPRF_NAME(NewReport)(KPRF_TYPE(PC,HDR) pHdr)
{
    /*
     * Allocate memory for the report.
     * Everything but the mods and modsegs is allocated in the same block as the report.
     */
    KSIZE cb = KPRF_ALIGN(KPRF_SIZEOF(REPORT), 32);
    KUPTR offThreads = cb;
    cb += KPRF_ALIGN(KPRF_SIZEOF(REPORTTHREAD) * pHdr->cThreads, 32);
    KUPTR offFunctions = cb;
    cb += KPRF_ALIGN(KPRF_SIZEOF(REPORTFUNC) * pHdr->cFunctions, 32);
    KUPTR offSortedThreads = cb;
    cb += KPRF_ALIGN(sizeof(KPRF_TYPE(P,REPORTTHREAD)) * pHdr->cThreads, 32);
    KUPTR offSortedFunctions = cb;
    cb += KPRF_ALIGN(sizeof(KPRF_TYPE(P,REPORTFUNC)) * pHdr->cFunctions, 32);
    KPRF_TYPE(P,REPORT) pReport = (KPRF_TYPE(P,REPORT))malloc(cb);
    if (!pReport)
        return NULL;

    /*
     * Initialize it.
     */
    pReport->pHdr = pHdr;
    pReport->paThreads = (KPRF_TYPE(P,REPORTTHREAD))((KU8 *)pReport + offThreads);
    pReport->paFunctions = (KPRF_TYPE(P,REPORTFUNC))((KU8 *)pReport + offFunctions);
    pReport->pFirstMod = NULL;
    pReport->cMods = 0;
    KPRF_NAME(ReportTreeInit)(pReport);
    pReport->cModSegs = 0;
    pReport->papSortedThreads = (KPRF_TYPE(P,REPORTTHREAD) *)((KU8 *)pReport + offSortedThreads);
    pReport->papSortedFunctions = (KPRF_TYPE(P,REPORTFUNC) *)((KU8 *)pReport + offSortedFunctions);
    pReport->ProfiledTicks = 0;
    pReport->OverheadTicks = 0;
    pReport->SleepTicks = 0;
    pReport->cCalls = 0;

    return pReport;
}


/**
 * AVL callback for deleting a module segment node.
 *
 * @returns 0
 * @param   pCore       The tree node to delete.
 * @param   pvParam     User parameter, ignored.
 */
static int KPRF_NAME(DeleteModSeg)(KPRF_TYPE(P,REPORTMODSEG) pCore, void *pvParam)
{
    free(pCore);
    return 0;
}


/**
 * Releases all the resources held be a report.
 *
 * @param   pReport     The report to delete.
 */
static void KPRF_NAME(DeleteReport)(KPRF_TYPE(P,REPORT) pReport)
{
    /*
     * The list and AVL.
     */
    while (pReport->pFirstMod)
    {
        KPRF_TYPE(P,REPORTMOD) pFree = pReport->pFirstMod;
        pReport->pFirstMod = pFree->pNext;
        kDbgModuleClose(pFree->pDbgMod);
        free(pFree);
    }

    KPRF_NAME(ReportTreeDestroy)(pReport, KPRF_NAME(DeleteModSeg), NULL);

    /*
     * The function debug info.
     */
    KU32 i = pReport->pHdr->cFunctions;
    while (i-- > 0)
    {
        kDbgSymbolFree(pReport->paFunctions[i].pSym);
        kDbgLineFree(pReport->paFunctions[i].pLine);
    }

    /*
     * The report it self.
     */
    pReport->pHdr = NULL;
    free(pReport);
}


/**
 * Builds the module segment tree and the list of modules.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pReport     The report to work on.
 */
static int KPRF_NAME(AnalyzeModSegs)(KPRF_TYPE(P,REPORT) pReport)
{
    const KU32 offEnd = pReport->pHdr->offModSegs + pReport->pHdr->cbModSegs;
    KU32 off = pReport->pHdr->offModSegs;
    while (off < offEnd)
    {
        KPRF_TYPE(PC,MODSEG) pCur = KPRF_OFF2PTR(PC,MODSEG, off, pReport->pHdr);
        KU32 cbCur = KPRF_OFFSETOF(MODSEG, szPath[pCur->cchPath + 1]);
        cbCur = KPRF_ALIGN(cbCur, KPRF_SIZEOF(UPTR));

        /*
         * Create a new modseg record.
         */
        KPRF_TYPE(P,REPORTMODSEG) pSeg = (KPRF_TYPE(P,REPORTMODSEG))malloc(sizeof(*pSeg));
        if (!pSeg)
            return -1;

        pSeg->offSegment = off;
        pSeg->pModSeg = pCur;
        pSeg->pMod = NULL; /* below */
        pSeg->OnStackTicks = 0;
        pSeg->OnTopOfStackTicks = 0;
        pSeg->cFunctions = 0;

        if (!KPRF_NAME(ReportTreeInsert)(pReport, pSeg))
        {
            free(pSeg);
            return -1;
        }
        pReport->cModSegs++;

        /*
         * Search for the module record.
         */
        KPRF_TYPE(P,REPORTMOD) pMod = pReport->pFirstMod;
        while (     pMod
               &&   (   pMod->pFirstSeg->pModSeg->cchPath != pCur->cchPath
                     || memcmp(pMod->pFirstSeg->pModSeg->szPath, pCur->szPath, pCur->cchPath)))
            pMod = pMod->pNext;
        if (pMod)
        {
            /** @todo sort segments */
            pSeg->pMod = pMod;
            pSeg->pNext = pMod->pFirstSeg;
            pMod->pFirstSeg = pSeg;
        }
        else
        {
            KPRF_TYPE(P,REPORTMOD) pMod = (KPRF_TYPE(P,REPORTMOD))malloc(sizeof(*pMod) + pCur->cchPath);
            if (!pMod)
                return -1;
            pSeg->pMod = pMod;
            pSeg->pNext = NULL;
            pMod->iMod = pReport->cMods++;
            pMod->pNext = pReport->pFirstMod;
            pReport->pFirstMod = pMod;
            pMod->pFirstSeg = pSeg;
            pMod->pDbgMod = NULL;
            pMod->OnStackTicks = 0;
            pMod->OnTopOfStackTicks = 0;
            pMod->cFunctions = 0;

            int rc = kDbgModuleOpen(&pMod->pDbgMod, pSeg->pModSeg->szPath, NULL /* kLdrMod */);
            if (rc)
                pMod->pDbgMod = NULL;
        }

        /* next */
        off += cbCur;
    }

    return 0;
}


/**
 * Initializes the function arrays.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pReport     The report to work on.
 */
static int KPRF_NAME(AnalyseFunctions)(KPRF_TYPE(P,REPORT) pReport)
{
    KU32 iFunc = pReport->pHdr->cFunctions;
    KPRF_TYPE(PC,FUNC) pFunc = KPRF_OFF2PTR(PC,FUNC, pReport->pHdr->offFunctions + iFunc * sizeof(*pFunc), pReport->pHdr);
    KPRF_TYPE(P,REPORTFUNC) pReportFunc = &pReport->paFunctions[iFunc];
    while (iFunc-- > 0)
    {
        pFunc--;
        pReportFunc--;

        pReport->papSortedFunctions[iFunc] = pReportFunc;
        pReportFunc->pFunc = pFunc;
        pReportFunc->pModSeg = KPRF_NAME(ReportTreeGet)(pReport, pFunc->offModSeg);
        pReportFunc->pSym = NULL;
        pReportFunc->pLine = NULL;
        if (pReportFunc->pModSeg)
        {
            /* Collect module segment and module statistics. */
            KPRF_TYPE(P,REPORTMODSEG) pModSeg = pReportFunc->pModSeg;
            pModSeg->cFunctions++;
            pModSeg->OnStackTicks += pFunc->OnStack.SumTicks;
            pModSeg->OnTopOfStackTicks += pFunc->OnTopOfStack.SumTicks;

            KPRF_TYPE(P,REPORTMOD) pMod = pModSeg->pMod;
            pMod->cFunctions++;
            pMod->OnStackTicks += pFunc->OnStack.SumTicks;
            pMod->OnTopOfStackTicks += pFunc->OnTopOfStack.SumTicks;

            /* Get debug info. */
            KDBGADDR offSegment = pFunc->uEntryPtr - pModSeg->pModSeg->uBasePtr;
            int rc = kDbgModuleQuerySymbolA(pMod->pDbgMod, pModSeg->pModSeg->iSegment, offSegment, &pReportFunc->pSym);
            /** @todo check displacement! */
            if (rc)
                pReportFunc->pSym = NULL;
            rc = kDbgModuleQueryLineA(pMod->pDbgMod, pModSeg->pModSeg->iSegment, offSegment, &pReportFunc->pLine);
            if (rc)
                pReportFunc->pLine = NULL;
        }
    }
    return 0;
}


/**
 * Initializes the thread arrays.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pReport     The report to work on.
 */
static int KPRF_NAME(AnalyseThreads)(KPRF_TYPE(P,REPORT) pReport)
{
    KU32 iThread = pReport->pHdr->cThreads;
    KPRF_TYPE(PC,THREAD) pThread = KPRF_OFF2PTR(PC,THREAD, pReport->pHdr->offThreads + iThread * sizeof(*pThread), pReport->pHdr);
    KPRF_TYPE(P,REPORTTHREAD) pReportThread = &pReport->paThreads[iThread];
    while (iThread-- > 0)
    {
        pThread--;
        pReportThread--;

        pReport->papSortedThreads[iThread] = pReportThread;
        pReportThread->pThread = pThread;

        /* collect statistics */
        pReport->ProfiledTicks += pThread->ProfiledTicks;
        pReport->OverheadTicks += pThread->OverheadTicks;
        pReport->SleepTicks += pThread->SleepTicks;
        pReport->cCalls += pThread->cCalls;

    }
    return 0;
}


/**
 * Analyses the data set, producing a report.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 *
 * @param   pHdr        The data set.
 * @param   ppReport    Where to store the report.
 */
static int KPRF_NAME(Analyse)(KPRF_TYPE(PC,HDR) pHdr, KPRF_TYPE(PP,REPORT) ppReport)
{
    *ppReport = NULL;

    /* allocate it */
    KPRF_TYPE(P,REPORT) pReport = KPRF_NAME(NewReport)(pHdr);
    if (!pReport)
        return -1;

    /* read module segments */
    int rc = KPRF_NAME(AnalyzeModSegs)(pReport);
    if (!rc)
    {
        /* read functions. */
        rc = KPRF_NAME(AnalyseFunctions)(pReport);
        if (!rc)
        {
            /* read threads */
            rc = KPRF_NAME(AnalyseThreads)(pReport);
            if (!rc)
            {
                *ppReport = pReport;
                return 0;
            }
        }
    }

    KPRF_NAME(DeleteReport)(pReport);
    return rc;
}


/**
 * Writes row with 32-bit value.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowU32X32)(FILE *pOut, const char *pszName, KU32 u32, const char *pszUnit)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th>%s</th>\n"
            "    <td colspan=\"6\">%u (0x%x)%s%s</td>\n"
            "  </tr>\n",
            pszName,
            u32, u32, pszUnit ? " " : "", pszUnit ? pszUnit : "");
}


/**
 * Writes row with 32-bit value.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowU32)(FILE *pOut, const char *pszName, KU32 u32, const char *pszUnit)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th>%s</th>\n"
            "    <td colspan=\"6\">%u%s%s</td>\n"
            "  </tr>\n",
            pszName,
            u32, pszUnit ? " " : "", pszUnit ? pszUnit : "");
}


/**
 * Writes row with 64-bit value.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowU64)(FILE *pOut, const char *pszName, KU64 u64, const char *pszUnit)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th>%s</th>\n"
            "    <td colspan=\"6\">% " KPRF_FMT_U64 " (0x%" KPRF_FMT_X64 ")%s%s</td>\n"
            "  </tr>\n",
            pszName,
            u64, u64, pszUnit ? " " : "", pszUnit ? pszUnit : "");
}


/**
 * Writes row with 64-bit hex value.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowX64)(FILE *pOut, const char *pszName, KU64 u64, const char *pszUnit)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th>%s</th>\n"
            "    <td colspan=\"6\">0x%" KPRF_FMT_X64 "%s%s</td>\n"
            "  </tr>\n",
            pszName,
            u64, pszUnit ? " " : "", pszUnit ? pszUnit : "");
}


/**
 * Writes a ticks.
 */
static void KPRF_NAME(HtmlWriteParts)(FILE *pOut, KU64 cTicks, KU64 cTotalTicks)
{
    /** U+2030 PER MILLE SIGN */
    static const KU8 s_szPerMilleSignUtf8[4] = { 0xe2, 0x80, 0xb0, 0};

    if (cTicks * 100 / cTotalTicks)
    {
        KU32 u = (KU32)((cTicks * 1000) / cTotalTicks);
        fprintf(pOut, "%u.%01u%%", u / 10, u %10);
    }
    else //if (cTicks * 100000 / cTotalTicks)
    {
        KU32 u = (KU32)((cTicks * 100000) / cTotalTicks);
        fprintf(pOut, "%u.%02u%s", u / 100, u % 100, s_szPerMilleSignUtf8);
    }
    /*
    else if (cTicks * 1000000 / cTotalTicks)
        fprintf(pOut, "%u ppm", (unsigned)((cTicks * 1000000) / cTotalTicks));
    else
        fprintf(pOut, "%u ppb", (unsigned)((cTicks * 1000000000) / cTotalTicks));
    */
}


/**
 * Writes a ticks.
 */
static void KPRF_NAME(HtmlWriteTicks)(FILE *pOut, KU64 cTicks, KU64 cTotalTicks)
{
    fprintf(pOut, "%" KPRF_FMT_U64 "", cTicks);
    if (cTotalTicks)
    {
        fprintf(pOut, "</td><td class=\"PartsRow\">");
        KPRF_NAME(HtmlWriteParts)(pOut, cTicks, cTotalTicks);
    }
}


/**
 * Writes row with ticks value.
 *
 * @param   pOut            Where to write.
 * @aaran   pszName         The row name.
 * @param   cTicks          The tick count.
 * @param   cTotalTicks     If non-zero, this is used for cTicks / cTotalTicks.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowTicks)(FILE *pOut, const char *pszName, KU64 cTicks, KU64 cTotalTicks)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th class=\"TicksRow\">%s</th>\n"
            "    <td class=\"TicksRow\">",
            pszName);
    KPRF_NAME(HtmlWriteTicks)(pOut, cTicks, cTotalTicks);
    fprintf(pOut,
                     "</td><td colspan=\"%d\"/>\n"
            "  </tr>\n",
            cTotalTicks ? 4 : 5);
}


/**
 * Writes row with a time stat value.
 *
 * @param   pOut            Where to write.
 * @aaran   pszName         The row name.
 * @param   cTicks          The tick count.
 * @param   cTotalTicks     If non-zero, this is used for cTicks / cTotalTicks.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowTimeStat)(FILE *pOut, const char *pszName, KPRF_TYPE(PC,TIMESTAT) pTimeStat, KU64 cTotalTicks)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th class=\"TicksRow\">%s</th>\n"
            "    <td class=\"TicksRow\">",
            pszName);
    KPRF_NAME(HtmlWriteTicks)(pOut, pTimeStat->SumTicks, cTotalTicks);
    fprintf(pOut,    "</td>\n"
            "    <td class=\"MinMaxTicksRow\">");
    KPRF_NAME(HtmlWriteTicks)(pOut, pTimeStat->MinTicks, cTotalTicks);
    fprintf(pOut,    "</td>\n"
            "    <td class=\"MinMaxTicksRow\">");
    KPRF_NAME(HtmlWriteTicks)(pOut, pTimeStat->MaxTicks, cTotalTicks);
    fprintf(pOut,    "</td>\n"
            "  </tr>\n");
}


/**
 * Writes row with calls value.
 *
 * @param   pOut            Where to write.
 * @aaran   pszName         The row name.
 * @param   cCalls          The call count.
 * @param   cTotalCalls     This is used for cCalls / cTotalCalls.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowCalls)(FILE *pOut, const char *pszName, KU64 cCalls, KU64 cTotalCalls)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th class=\"CallsRow\">%s</th>\n"
            "    <td class=\"CallsRow\">%" KPRF_FMT_U64"</td><td class=\"PartsRow\">",
            pszName, cCalls);
    KPRF_NAME(HtmlWriteParts)(pOut, cCalls, cTotalCalls);
    fprintf(pOut, "</td><td colspan=4></td>"
            "  </tr>\n");
}


/**
 * Writes row with pointer value.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowUPTR)(FILE *pOut, const char *pszName, KPRF_TYPE(,UPTR) uPtr, const char *pszUnit)
{
    fprintf(pOut,
            "  <tr>\n"
            "    <th>%s</th>\n"
            "    <td colspan=\"6\">%" KPRF_FMT_UPTR "%s%s</td>\n"
            "  </tr>\n",
            pszName,
            uPtr, pszUnit ? " " : "", pszUnit ? pszUnit : "");
}


/**
 * Writes row with string value.
 * @internal
 */
static void KPRF_NAME(HtmlWriteRowString)(FILE *pOut, const char *pszName, const char *pszClass, const char *pszFormat, ...)
{
    fprintf(pOut,
             "  <tr>\n"
             "    <th>%s</th>\n"
             "    <td%s%s%s colspan=\"6\">",
             pszName,
             pszClass ? " class=\"" : "", pszClass ? pszClass : "", pszClass ? "\"" : "");
    va_list va;
    va_start(va, pszFormat);
    vfprintf(pOut, pszFormat, va);
    va_end(va);
    fprintf(pOut,      "</td>\n"
             "  </tr>\n");
}


/**
 * The first column
 */
typedef enum KPRF_TYPE(,FIRSTCOLUMN)
{
    KPRF_TYPE(,FIRSTCOLUMN_ON_STACK) = 0,
    KPRF_TYPE(,FIRSTCOLUMN_ON_TOP_OF_STACK),
    KPRF_TYPE(,FIRSTCOLUMN_CALLS_TO),
    KPRF_TYPE(,FIRSTCOLUMN_CALLS_FROM),
    KPRF_TYPE(,FIRSTCOLUMN_MAX)
} KPRF_TYPE(,FIRSTCOLUMN);


/**
 * Prints the table with the sorted functions.
 * The tricky bit is that the sorted column should be to the left of the function name.
 */
static void KPRF_NAME(HtmlWriteSortedFunctions)(KPRF_TYPE(P,REPORT) pReport, FILE *pOut, const char *pszName,
                                                const char *pszTitle, KPRF_TYPE(,FIRSTCOLUMN) enmFirst)
{
    fprintf(pOut,
            "<h2><a name=\"%s\">%s</a></h2>\n"
            "\n",
            pszName, pszTitle);

    fprintf(pOut,
            "<table class=\"FunctionsSorted\">\n"
            "  <tr>\n"
            "    <th/>\n");
    static const char *s_pszHeaders[KPRF_TYPE(,FIRSTCOLUMN_MAX) * 2] =
    {
        "    <th colspan=8><a href=\"#Functions-TimeOnStack\">Time On Stack</a> (ticks)</th>\n",
        "    <th colspan=2><a href=\"#Functions-TimeOnStack\">Sum</a></th>\n"
        "    <th colspan=2><a href=\"#Functions-TimeOnStack-Min\">Min</a></th>\n"
        "    <th colspan=2><a href=\"#Functions-TimeOnStack-Avg\">Average</a></th>\n"
        "    <th colspan=2><a href=\"#Functions-TimeOnStack-Max\">Max</a></th>\n",

        "    <th colspan=8><a href=\"#Functions-TimeOnTopOfStack\">Time On To Top</a> (ticks)</th>\n",
        "    <th colspan=2><a href=\"#Functions-TimeOnTopOfStack\">Sum</a></th>\n"
        "    <th colspan=2><a href=\"#Functions-TimeOnTopOfStack-Min\">Min</a></th>\n"
        "    <th colspan=2><a href=\"#Functions-TimeOnTopOfStack-Avg\">Average</a></th>\n"
        "    <th colspan=2><a href=\"#Functions-TimeOnTopOfStack-Max\">Max</a></th>\n",

        "    <th colspan=2><a href=\"#Functions-CallsTo\">Calls To</a></th>\n",
        "    <th/><th/>\n",

        "    <th colspan=2><a href=\"#Functions-CallsFrom\">Calls From</a></th>\n",
        "    <th/><th/>\n",
    };

    fprintf(pOut, "%s",  s_pszHeaders[enmFirst * 2]);
    fprintf(pOut, "    <th>Function</th>\n");
    for (unsigned i = (enmFirst + 1) % KPRF_TYPE(,FIRSTCOLUMN_MAX); i != enmFirst; i = (i + 1) % KPRF_TYPE(,FIRSTCOLUMN_MAX))
        fprintf(pOut, "%s",  s_pszHeaders[i * 2]);
    fprintf(pOut,
            "  </tr>\n"
            "  <tr>\n"
            "    <th/>\n");
    fprintf(pOut, "%s",  s_pszHeaders[enmFirst * 2 + 1]);
    fprintf(pOut, "    <th/>\n");
    for (unsigned i = (enmFirst + 1) % KPRF_TYPE(,FIRSTCOLUMN_MAX); i != enmFirst; i = (i + 1) % KPRF_TYPE(,FIRSTCOLUMN_MAX))
        fprintf(pOut, "%s",  s_pszHeaders[i * 2 + 1]);
    fprintf(pOut,
            "  </tr>\n");

    for (KU32 iFunc = 0; iFunc < pReport->pHdr->cFunctions; iFunc++)
    {
        KPRF_TYPE(P,REPORTFUNC) pReportFunc = pReport->papSortedFunctions[iFunc];
        KPRF_TYPE(PC,FUNC) pFunc = pReportFunc->pFunc;
        fprintf(pOut,
                "  <tr>\n"
                "    <td>%u</td>\n",
               iFunc);

        unsigned i = enmFirst;
        do
        {
            switch (i)
            {
                case KPRF_TYPE(,FIRSTCOLUMN_ON_STACK):
                    fprintf(pOut,
                            "    <td class=\"Ticks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnStack.SumTicks);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnStack.SumTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n"
                            "    <td class=\"MinMaxTicks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnStack.MinTicks);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnStack.MinTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n"
                            "    <td class=\"MinMaxTicks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnStack.SumTicks / pFunc->cOnStack);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnStack.MinTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n"
                            "    <td class=\"MinMaxTicks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnStack.MaxTicks);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnStack.MaxTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n");
                    break;

                case KPRF_TYPE(,FIRSTCOLUMN_ON_TOP_OF_STACK):
                    fprintf(pOut,
                            "    <td class=\"Ticks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnTopOfStack.SumTicks);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnTopOfStack.SumTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n"
                            "    <td class=\"MinMaxTicks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnTopOfStack.MinTicks);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnTopOfStack.MinTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n"
                            "    <td class=\"MinMaxTicks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnTopOfStack.SumTicks / pFunc->cOnStack);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnTopOfStack.MinTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n"
                            "    <td class=\"MinMaxTicks\">%" KPRF_FMT_U64 "</td><td class=\"Parts\">",
                            pFunc->OnTopOfStack.MaxTicks);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->OnTopOfStack.MaxTicks, pReport->ProfiledTicks);
                    fprintf(pOut,       "</td>\n");
                    break;

                case KPRF_TYPE(,FIRSTCOLUMN_CALLS_TO):
                    fprintf(pOut,
                            "    <td class=\"Calls\">%" KPRF_FMT_U64 "</td><td Class=\"Parts\">",
                            pFunc->cOnStack);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->cOnStack, pReport->cCalls);
                    fprintf(pOut,       "</td>\n");
                    break;

                case KPRF_TYPE(,FIRSTCOLUMN_CALLS_FROM):
                    fprintf(pOut,
                            "    <td class=\"Calls\">%" KPRF_FMT_U64 "</td><td Class=\"Parts\">",
                            pFunc->cCalls);
                    KPRF_NAME(HtmlWriteParts)(pOut, pFunc->cCalls, pReport->cCalls);
                    fprintf(pOut,       "</td>\n");
                    break;

                default:
                    break;
            }

            /* inject the function column */
            if (i == enmFirst)
            {
                fprintf(pOut,
                        "    <td><a href=\"#Func-%u\">",
                        (unsigned)(uintptr_t)(pReportFunc - pReport->paFunctions));
                if (pReportFunc->pSym)
                    fprintf(pOut, "%s</a></td>\n", pReportFunc->pSym->szName);
                else
                    fprintf(pOut, "%" KPRF_FMT_UPTR "</a></td>\n", pFunc->uEntryPtr);
            }

            /* next */
            i = (i + 1) % KPRF_TYPE(,FIRSTCOLUMN_MAX);
        } while (i != enmFirst);

        fprintf(pOut,
                "  </tr>\n");
    }
    fprintf(pOut,
            "</table>\n"
            "\n");

}


/**
 * Writes an HTML report.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pReport     The report to put into HTML.
 * @param   pOut        The file stream to write the HTML to.
 */
static int KPRF_NAME(WriteHtmlReport)(KPRF_TYPE(P,REPORT) pReport, FILE *pOut)
{
    KPRF_TYPE(PC,HDR) pHdr = pReport->pHdr;

    /*
     * Write the standard html.
     */
    fprintf(pOut,
            "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
            "<html>\n"
            "<head>\n"
            "  <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\n"
            "  <title>kProfiler 2 - %s</title>\n"
            "</head>\n"
            "<style>\n"
            "table\n"
            "{\n"
//            "  width: 90%%;\n"
            "  background: #999999;\n"
//            "  margin-top: .6em;\n"
//            "  margin-bottom: .3em;\n"
            "}\n"
            "th\n"
            "{\n"
            "  padding: 1px 4px;\n"
            "  background: #cccccc;\n"
//            "  text-align: left;\n"
            "  font-size: 90%%;\n"
            //"  width: 30%%;\n"
            "}\n"
            "td\n"
            "{\n"
            "  padding: 1px 4px;\n"
            "  background: #ffffff;\n"
            "  font-size: 90%%;\n"
            "}\n"
            "td.Ticks\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.TicksRow\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.MinMaxTicks\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.MinMaxTicksRow\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.Parts\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.PartsRow\n"
            "{\n"
            "  text-align: left;\n"
            "}\n"
            "td.Calls\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.CallsRow\n"
            "{\n"
            "  text-align: right;\n"
            "}\n"
            "td.BlankRow\n"
            "{\n"
            "  background: #e0e0e0;\n"
            "}\n"
            "td.Name\n"
            "{\n"
            "  font-weight: bold;\n"
            "}\n"
            "table.Summary th\n"
            "{\n"
            "  width:200px;\n"
            "}\n"
            "table.Thread\n"
            "{\n"
            "  min-width:60%%\n"
            "}\n"
            "table.Thread th\n"
            "{\n"
            "  width:200px;\n"
            "}\n"
            "table.Functions\n"
            "{\n"
            "  width:60%%;\n"
            "}\n"
            "table.Functions th\n"
            "{\n"
            "  width:200px;\n"
            "}\n"
            "table.Modules\n"
            "{\n"
            "  width:60%%;\n"
            "}\n"
            "table.Modules th\n"
            "{\n"
            "  width:200px;\n"
            "}\n"
            "table.FunctionsSorted\n"
            "{\n"
            "}\n"
            "</style>\n"
            "<body topmargin=\"0\">\n"
            ,
            pHdr->offCommandLine
                ? (const char *)KPRF_OFF2PTR(P,FUNC, pHdr->offCommandLine, pHdr)
                : ""
            );

    /*
     * Table of contents.
     */
    fprintf(pOut,
            "<h2>Table of Contents</h2>\n"
            "\n"
            "<ul>\n"
            "  <li><a href=\"#Summary\"  >1.0 Summary</a></li>\n"
            "  <li><a href=\"#Functions\">2.0 Functions</a></li>\n"
            "      <ul>\n"
            "        <li><a href=\"#Functions-TimeOnStack\"     >2.1 Time On Stack</a></li>\n"
            "          <ul>\n"
            "            <li><a href=\"#Functions-TimeOnStack-Avg\"     >2.2.1 Time On Stack - Average</a></li>\n"
            "            <li><a href=\"#Functions-TimeOnStack-Min\"     >2.2.1 Time On Stack - Min</a></li>\n"
            "            <li><a href=\"#Functions-TimeOnStack-Max\"     >2.2.2 Time On Stack - Max</a></li>\n"
            "          </ul>\n"
            "        <li><a href=\"#Functions-TimeOnTopOfStack\">2.3 Time On Top Of Stack</a></li>\n"
            "          <ul>\n"
            "            <li><a href=\"#Functions-TimeOnTopOfStack-Avg\">2.3.1 Time On Top Of Stack - Average</a></li>\n"
            "            <li><a href=\"#Functions-TimeOnTopOfStack-Min\">2.3.2 Time On Top Of Stack - Min</a></li>\n"
            "            <li><a href=\"#Functions-TimeOnTopOfStack-Max\">2.3.3 Time On Top Of Stack - Max</a></li>\n"
            "          </ul>\n"
            "        <li><a href=\"#Functions-CallsTo\"         >2.3 Calls To</a></li>\n"
            "        <li><a href=\"#Functions-CallsFrom\"       >2.4 Calls From</a></li>\n"
            "        <li><a href=\"#Function-Details\"          >2.5 Function Details</a></li>\n"
            "      </ul>\n"
            "  <li><a href=\"#Threads\"  >3.0 Threads</a></li>\n"
            "  <li><a href=\"#Modules\"  >4.0 Modules</a></li>\n"
            "</ul>\n"
            "\n"
            "\n");

    /*
     * Summary.
     */
    fprintf(pOut,
            "<h2><a name=\"Summary\">1.0 Summary</a></h2>\n"
            "\n"
            "<p>\n"
            "<table class=\"Summary\">\n");
    if (pHdr->offCommandLine)
        KPRF_NAME(HtmlWriteRowString)(pOut, "Command Line", NULL, "%s", (const char *)KPRF_OFF2PTR(P,FUNC, pHdr->offCommandLine, pHdr));
    KPRF_NAME(HtmlWriteRowU32X32)(pOut, "Threads", pHdr->cThreads, NULL);
    KPRF_NAME(HtmlWriteRowU32X32)(pOut, "Modules", pReport->cMods, NULL);
    KPRF_NAME(HtmlWriteRowU32X32)(pOut, "Functions", pHdr->cFunctions, NULL);
    KPRF_NAME(HtmlWriteRowTicks)(pOut, "Profiled", pReport->ProfiledTicks, pReport->ProfiledTicks);
    KPRF_NAME(HtmlWriteRowTicks)(pOut, "Sleep", pReport->SleepTicks, pReport->ProfiledTicks);
    KPRF_NAME(HtmlWriteRowTicks)(pOut, "Overhead", pReport->OverheadTicks, pReport->ProfiledTicks + pReport->OverheadTicks);
    KPRF_NAME(HtmlWriteRowCalls)(pOut, "Recorded Calls", pReport->cCalls, pReport->cCalls);
    fprintf(pOut, "<tr><td class=\"BlankRow\" colspan=7>&nbsp;</td></tr>\n");
    KPRF_NAME(HtmlWriteRowString)(pOut, "kProfiler Version ", NULL, "Mark 2 Alpha 1");
    KPRF_NAME(HtmlWriteRowString)(pOut, "kProfiler Build Time ", NULL, __DATE__ " " __TIME__);
    fprintf(pOut,
            "</table>\n"
            "</p>\n"
            "\n"
            "\n");

    /*
     * Functions.
     */
    fprintf(pOut,
            "<h2><a name=\"Functions\">2.0 Functions</a></h2>\n"
            "\n");

    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnStack));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnStack",     "2.1 Time On Stack", KPRF_TYPE(,FIRSTCOLUMN_ON_STACK));
    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnStackAvg));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnStack-Avg",     "2.2.1 Time On Stack - Average", KPRF_TYPE(,FIRSTCOLUMN_ON_STACK));
    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnStackMin));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnStack-Min",     "2.2.2 Time On Stack - Min", KPRF_TYPE(,FIRSTCOLUMN_ON_STACK));
    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnStackMax));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnStack-Max",     "2.2.3 Time On Stack - Max", KPRF_TYPE(,FIRSTCOLUMN_ON_STACK));

    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnTopOfStack));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnTopOfStack",    "2.2 Time On Top Of Stack", KPRF_TYPE(,FIRSTCOLUMN_ON_TOP_OF_STACK));
    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnTopOfStackAvg));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnTopOfStack-Avg","2.2.1 Time On Top Of Stack - Average", KPRF_TYPE(,FIRSTCOLUMN_ON_TOP_OF_STACK));
    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnTopOfStackMin));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnTopOfStack-Min","2.2.2 Time On Top Of Stack - Min", KPRF_TYPE(,FIRSTCOLUMN_ON_TOP_OF_STACK));
    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareOnTopOfStackMax));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-TimeOnTopOfStack-Max","2.2.3 Time On Top Of Stack - Max", KPRF_TYPE(,FIRSTCOLUMN_ON_TOP_OF_STACK));

    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareCallsTo));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-CallsTo",         "2.4 Calls To", KPRF_TYPE(,FIRSTCOLUMN_CALLS_TO));

    qsort(&pReport->papSortedFunctions[0], pHdr->cFunctions, sizeof(pReport->papSortedFunctions[0]), KPRF_NAME(FuncCompareCallsFrom));
    KPRF_NAME(HtmlWriteSortedFunctions)(pReport, pOut, "Functions-CallsFrom",       "2.5 Calls From", KPRF_TYPE(,FIRSTCOLUMN_CALLS_FROM));

    fprintf(pOut,
            "<h2><a name=\"Function-Details\">2.5 Function Details</a></h2>\n"
            "\n"
            "<p>\n"
            "<table class=\"Functions\">\n");
    for (KU32 iFunc = 0; iFunc < pHdr->cFunctions; iFunc++)
    {
        KPRF_TYPE(P,REPORTFUNC) pReportFunc = &pReport->paFunctions[iFunc];
        KPRF_TYPE(PC,FUNC) pFunc = pReportFunc->pFunc;

        fprintf(pOut,
                "<tr><td class=\"BlankRow\" colspan=7><a name=\"Func-%u\">&nbsp;</a></td></tr>\n",
                iFunc);
        KPRF_NAME(HtmlWriteRowU32)(pOut, "Function No.", iFunc, NULL);
        if (pReportFunc->pSym)
            KPRF_NAME(HtmlWriteRowString)(pOut, "Name", "Name", "%s", pReportFunc->pSym->szName);
        if (pReportFunc->pLine)
            KPRF_NAME(HtmlWriteRowString)(pOut, "Location", NULL, "<a href=\"file:///%s\">%s</a> Line #%d",
                                          pReportFunc->pLine->szFile, pReportFunc->pLine->szFile, pReportFunc->pLine->iLine);
        if (pReportFunc->pModSeg)
        {
            KPRF_NAME(HtmlWriteRowString)(pOut, "Module", NULL, "<a href=\"#Mod-%u\">%s</a>",
                                          pReportFunc->pModSeg->pMod->iMod, pReportFunc->pModSeg->pModSeg->szPath);
            KPRF_NAME(HtmlWriteRowString)(pOut, "Segment:Offset", NULL, "%x:%" KPRF_FMT_UPTR,
                                          pReportFunc->pModSeg->pModSeg->iSegment,
                                          pFunc->uEntryPtr - pReportFunc->pModSeg->pModSeg->uBasePtr);
        }
        KPRF_NAME(HtmlWriteRowUPTR)(pOut, "Address", pFunc->uEntryPtr, NULL);

        KPRF_NAME(HtmlWriteRowTimeStat)(pOut, "On Stack", &pFunc->OnStack, pReport->ProfiledTicks);
        KPRF_NAME(HtmlWriteRowTimeStat)(pOut, "On Top Of Stack", &pFunc->OnTopOfStack, pReport->ProfiledTicks);
        KPRF_NAME(HtmlWriteRowCalls)(pOut, "Calls To", pFunc->cOnStack, pReport->cCalls);
        KPRF_NAME(HtmlWriteRowCalls)(pOut, "Calls From", pFunc->cCalls, pReport->cCalls);

        fprintf(pOut,
                "\n");
    }
    fprintf(pOut,
            "</table>\n"
            "</p>\n"
            "\n");

    /*
     * Threads.
     */
    fprintf(pOut,
            "<h2><a name=\"Threads\">3.0 Threads</a></h2>\n"
            "\n"
            "<p>\n"
            "<table class=\"Threads\">\n");

    for (KU32 iThread = 0; iThread < pHdr->cThreads; iThread++)
    {
        KPRF_TYPE(PC,THREAD) pThread = pReport->paThreads[iThread].pThread;

        fprintf(pOut,
                "<tr><td class=\"BlankRow\" colspan=7><a name=\"Thread-%u\">&nbsp;</a></td></tr>\n",
                iThread);
        KPRF_NAME(HtmlWriteRowU32)(pOut, "Thread No.", iThread, NULL);
        KPRF_NAME(HtmlWriteRowX64)(pOut, "Thread Id", pThread->ThreadId, NULL);
        if (pThread->szName[0])
            KPRF_NAME(HtmlWriteRowString)(pOut, "Name", "Name", "%s", pThread->szName);
        KPRF_NAME(HtmlWriteRowUPTR)(pOut, "Stack Base Address", pThread->uStackBasePtr, NULL);
        KPRF_NAME(HtmlWriteRowUPTR)(pOut, "Max Stack Depth", pThread->cbMaxStack, "bytes");
        //KPRF_NAME(HtmlWriteRowUPTR)(pOut, "Max Stack Depth", pThread->cMaxFrames, "frames"); /** @todo max stack frames! */
        KPRF_NAME(HtmlWriteRowTicks)(pOut, "Profiled", pThread->ProfiledTicks, pReport->ProfiledTicks);
        KPRF_NAME(HtmlWriteRowTicks)(pOut, "Sleep", pThread->SleepTicks, pReport->ProfiledTicks);
        KPRF_NAME(HtmlWriteRowTicks)(pOut, "Overhead", pThread->OverheadTicks, pReport->ProfiledTicks + pReport->OverheadTicks);
        KPRF_NAME(HtmlWriteRowCalls)(pOut, "Recorded Calls",  pThread->cCalls, pReport->cCalls);
        KPRF_NAME(HtmlWriteRowU64)(pOut, "Unwinds", pThread->cUnwinds, NULL);
        KPRF_NAME(HtmlWriteRowU64)(pOut, "Profiler Stack Overflows", pThread->cOverflows, NULL);
        KPRF_NAME(HtmlWriteRowU64)(pOut, "Profiler Stack Switch Rejects", pThread->cStackSwitchRejects, NULL);

        fprintf(pOut,
                "\n");
    }
    fprintf(pOut,
            "</table>\n"
            "</p>\n"
            "\n");


    /*
     * Modules.
     */
    fprintf(pOut,
            "<h2><a name=\"Modules\">4.0 Modules</a></h2>\n"
            "\n"
            "<p>\n"
            "<table class=\"Modules\">\n");

    KPRF_TYPE(P,REPORTMOD) pMod = pReport->pFirstMod;
    KU32 iMod = 0;
    while (pMod)
    {
        fprintf(pOut,
                "<a name=\"Mod-%u\">\n"
                "<tr><td class=\"BlankRow\" colspan=7><a name=\"Module-%u\">&nbsp;</a></td></tr>\n",
                iMod, iMod);
        KPRF_NAME(HtmlWriteRowU32)(pOut, "Module No.", iMod, NULL);
        KPRF_NAME(HtmlWriteRowString)(pOut, "Name", "Name", "%s", pMod->pFirstSeg->pModSeg->szPath);

        for (KPRF_TYPE(P,REPORTMODSEG) pSeg = pMod->pFirstSeg; pSeg; pSeg = pSeg->pNext)
        {
            char szName[64];
            sprintf(szName, "Segment No.%u - Base", pSeg->pModSeg->iSegment);
            KPRF_NAME(HtmlWriteRowUPTR)(pOut, szName, pSeg->pModSeg->uBasePtr, NULL);
            sprintf(szName, "Segment No.%u - Size", pSeg->pModSeg->iSegment);
            KPRF_NAME(HtmlWriteRowUPTR)(pOut, szName,
                                        pSeg->pModSeg->cbSegmentMinusOne + 1 > pSeg->pModSeg->cbSegmentMinusOne
                                        ? pSeg->pModSeg->cbSegmentMinusOne + 1
                                        : pSeg->pModSeg->cbSegmentMinusOne,
                                        NULL);
        }

        KPRF_NAME(HtmlWriteRowTicks)(pOut, "On Stack", pMod->OnStackTicks, pReport->ProfiledTicks);
        KPRF_NAME(HtmlWriteRowTicks)(pOut, "On Top Of Stack", pMod->OnTopOfStackTicks, pReport->ProfiledTicks);
        KPRF_NAME(HtmlWriteRowU32)(pOut, "Functions", pMod->cFunctions, NULL);

        fprintf(pOut,
                "\n");

        /* next */
        iMod++;
        pMod = pMod->pNext;
    }
    fprintf(pOut,
            "</table>\n"
            "</p>\n"
            "\n");


    /*
     * The End.
     */
    fprintf(pOut,
            "</body>\n"
            "</html>\n");
    return 0;
}
