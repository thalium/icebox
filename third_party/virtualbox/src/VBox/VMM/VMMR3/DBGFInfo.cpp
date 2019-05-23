/* $Id: DBGFInfo.cpp $ */
/** @file
 * DBGF - Debugger Facility, Info.
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
#define LOG_GROUP LOG_GROUP_DBGF_INFO
#include <VBox/vmm/dbgf.h>

#include <VBox/vmm/mm.h>
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/param.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(void) dbgfR3InfoLog_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...);
static DECLCALLBACK(void) dbgfR3InfoLog_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args);
static DECLCALLBACK(void) dbgfR3InfoLogRel_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...);
static DECLCALLBACK(void) dbgfR3InfoLogRel_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args);
static DECLCALLBACK(void) dbgfR3InfoStdErr_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...);
static DECLCALLBACK(void) dbgfR3InfoStdErr_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args);
static DECLCALLBACK(void) dbgfR3InfoHelp(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Logger output. */
static const DBGFINFOHLP g_dbgfR3InfoLogHlp =
{
    dbgfR3InfoLog_Printf,
    dbgfR3InfoLog_PrintfV
};

/** Release logger output. */
static const DBGFINFOHLP g_dbgfR3InfoLogRelHlp =
{
    dbgfR3InfoLogRel_Printf,
    dbgfR3InfoLogRel_PrintfV
};

/** Standard error output. */
static const DBGFINFOHLP g_dbgfR3InfoStdErrHlp =
{
    dbgfR3InfoStdErr_Printf,
    dbgfR3InfoStdErr_PrintfV
};


/**
 * Initialize the info handlers.
 *
 * This is called first during the DBGF init process and thus does the shared
 * critsect init.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 */
int dbgfR3InfoInit(PUVM pUVM)
{
    /*
     * Make sure we already didn't initialized in the lazy manner.
     */
    if (RTCritSectRwIsInitialized(&pUVM->dbgf.s.CritSect))
        return VINF_SUCCESS;

    /*
     * Initialize the crit sect.
     */
    int rc = RTCritSectRwInit(&pUVM->dbgf.s.CritSect);
    AssertRCReturn(rc, rc);

    /*
     * Register the 'info help' item.
     */
    rc = DBGFR3InfoRegisterInternal(pUVM->pVM, "help", "List of info items.", dbgfR3InfoHelp);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}


/**
 * Terminate the info handlers.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 */
int dbgfR3InfoTerm(PUVM pUVM)
{
    /*
     * Delete the crit sect.
     */
    int rc = RTCritSectRwDelete(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);
    return rc;
}


/** Logger output.
 * @copydoc DBGFINFOHLP::pfnPrintf */
static DECLCALLBACK(void) dbgfR3InfoLog_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    NOREF(pHlp);
    va_list args;
    va_start(args, pszFormat);
    RTLogPrintfV(pszFormat, args);
    va_end(args);
}

/** Logger output.
 * @copydoc DBGFINFOHLP::pfnPrintfV */
static DECLCALLBACK(void) dbgfR3InfoLog_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args)
{
    NOREF(pHlp);
    RTLogPrintfV(pszFormat, args);
}


/**
 * Gets the logger info helper.
 * The returned info helper will unconditionally write all output to the log.
 *
 * @returns Pointer to the logger info helper.
 */
VMMR3DECL(PCDBGFINFOHLP) DBGFR3InfoLogHlp(void)
{
    return &g_dbgfR3InfoLogHlp;
}


/** Release logger output.
 * @copydoc DBGFINFOHLP::pfnPrintf */
static DECLCALLBACK(void) dbgfR3InfoLogRel_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    NOREF(pHlp);
    va_list args;
    va_start(args, pszFormat);
    RTLogRelPrintfV(pszFormat, args);
    va_end(args);
}

/** Release logger output.
 * @copydoc DBGFINFOHLP::pfnPrintfV */
static DECLCALLBACK(void) dbgfR3InfoLogRel_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args)
{
    NOREF(pHlp);
    RTLogRelPrintfV(pszFormat, args);
}


/** Standard error output.
 * @copydoc DBGFINFOHLP::pfnPrintf */
static DECLCALLBACK(void) dbgfR3InfoStdErr_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    NOREF(pHlp);
    va_list args;
    va_start(args, pszFormat);
    RTStrmPrintfV(g_pStdErr, pszFormat, args);
    va_end(args);
}

/** Standard error output.
 * @copydoc DBGFINFOHLP::pfnPrintfV */
static DECLCALLBACK(void) dbgfR3InfoStdErr_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args)
{
    NOREF(pHlp);
    RTStrmPrintfV(g_pStdErr, pszFormat, args);
}


/**
 * Gets the release logger info helper.
 * The returned info helper will unconditionally write all output to the release log.
 *
 * @returns Pointer to the release logger info helper.
 */
VMMR3DECL(PCDBGFINFOHLP) DBGFR3InfoLogRelHlp(void)
{
    return &g_dbgfR3InfoLogRelHlp;
}


/**
 * Handle registration worker.
 *
 * This allocates the structure, initializes the common fields and inserts into the list.
 * Upon successful return the we're inside the crit sect and the caller must leave it.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pszName     The identifier of the info.
 * @param   pszDesc     The description of the info and any arguments the handler may take.
 * @param   fFlags      The flags.
 * @param   ppInfo      Where to store the created
 */
static int dbgfR3InfoRegister(PUVM pUVM, const char *pszName, const char *pszDesc, uint32_t fFlags, PDBGFINFO *ppInfo)
{
    /*
     * Validate.
     */
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    AssertReturn(*pszName, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDesc, VERR_INVALID_POINTER);
    AssertMsgReturn(!(fFlags & ~(DBGFINFO_FLAGS_RUN_ON_EMT | DBGFINFO_FLAGS_ALL_EMTS)),
                    ("fFlags=%#x\n", fFlags), VERR_INVALID_FLAGS);

    /*
     * Allocate and initialize.
     */
    int rc;
    size_t cchName = strlen(pszName) + 1;
    PDBGFINFO pInfo = (PDBGFINFO)MMR3HeapAllocU(pUVM, MM_TAG_DBGF_INFO, RT_OFFSETOF(DBGFINFO, szName[cchName]));
    if (pInfo)
    {
        pInfo->enmType = DBGFINFOTYPE_INVALID;
        pInfo->fFlags = fFlags;
        pInfo->pszDesc = pszDesc;
        pInfo->cchName = cchName - 1;
        memcpy(pInfo->szName, pszName, cchName);

        /* lazy init */
        rc = VINF_SUCCESS;
        if (!RTCritSectRwIsInitialized(&pUVM->dbgf.s.CritSect))
            rc = dbgfR3InfoInit(pUVM);
        if (RT_SUCCESS(rc))
        {
            /*
             * Insert in alphabetical order.
             */
            rc = RTCritSectRwEnterExcl(&pUVM->dbgf.s.CritSect);
            AssertRC(rc);
            PDBGFINFO pPrev = NULL;
            PDBGFINFO pCur;
            for (pCur = pUVM->dbgf.s.pInfoFirst; pCur; pPrev = pCur, pCur = pCur->pNext)
                if (strcmp(pszName, pCur->szName) < 0)
                    break;
            pInfo->pNext = pCur;
            if (pPrev)
                pPrev->pNext = pInfo;
            else
                pUVM->dbgf.s.pInfoFirst = pInfo;

            *ppInfo = pInfo;
            return VINF_SUCCESS;
        }
        MMR3HeapFree(pInfo);
    }
    else
        rc = VERR_NO_MEMORY;
    return rc;
}


/**
 * Register a info handler owned by a device.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pszName     The identifier of the info.
 * @param   pszDesc     The description of the info and any arguments the handler may take.
 * @param   pfnHandler  The handler function to be called to display the info.
 * @param   pDevIns     The device instance owning the info.
 */
VMMR3_INT_DECL(int) DBGFR3InfoRegisterDevice(PVM pVM, const char *pszName, const char *pszDesc,
                                             PFNDBGFHANDLERDEV pfnHandler, PPDMDEVINS pDevIns)
{
    LogFlow(("DBGFR3InfoRegisterDevice: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p pDevIns=%p\n",
             pszName, pszName, pszDesc, pszDesc, pfnHandler, pDevIns));

    /*
     * Validate the specific stuff.
     */
    AssertPtrReturn(pfnHandler, VERR_INVALID_POINTER);
    AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);

    /*
     * Register
     */
    PDBGFINFO pInfo;
    int rc = dbgfR3InfoRegister(pVM->pUVM, pszName, pszDesc, 0, &pInfo);
    if (RT_SUCCESS(rc))
    {
        pInfo->enmType = DBGFINFOTYPE_DEV;
        pInfo->u.Dev.pfnHandler = pfnHandler;
        pInfo->u.Dev.pDevIns = pDevIns;
        RTCritSectRwLeaveExcl(&pVM->pUVM->dbgf.s.CritSect);
    }

    return rc;
}


/**
 * Register a info handler owned by a driver.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pszName     The identifier of the info.
 * @param   pszDesc     The description of the info and any arguments the handler may take.
 * @param   pfnHandler  The handler function to be called to display the info.
 * @param   pDrvIns     The driver instance owning the info.
 */
VMMR3_INT_DECL(int) DBGFR3InfoRegisterDriver(PVM pVM, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler, PPDMDRVINS pDrvIns)
{
    LogFlow(("DBGFR3InfoRegisterDriver: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p pDrvIns=%p\n",
             pszName, pszName, pszDesc, pszDesc, pfnHandler, pDrvIns));

    /*
     * Validate the specific stuff.
     */
    AssertPtrReturn(pfnHandler, VERR_INVALID_POINTER);
    AssertPtrReturn(pDrvIns, VERR_INVALID_POINTER);

    /*
     * Register
     */
    PDBGFINFO pInfo;
    int rc = dbgfR3InfoRegister(pVM->pUVM, pszName, pszDesc, 0, &pInfo);
    if (RT_SUCCESS(rc))
    {
        pInfo->enmType = DBGFINFOTYPE_DRV;
        pInfo->u.Drv.pfnHandler = pfnHandler;
        pInfo->u.Drv.pDrvIns = pDrvIns;
        RTCritSectRwLeaveExcl(&pVM->pUVM->dbgf.s.CritSect);
    }

    return rc;
}


/**
 * Register a info handler owned by an internal component.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pszName     The identifier of the info.
 * @param   pszDesc     The description of the info and any arguments the handler may take.
 * @param   pfnHandler  The handler function to be called to display the info.
 */
VMMR3_INT_DECL(int) DBGFR3InfoRegisterInternal(PVM pVM, const char *pszName, const char *pszDesc, PFNDBGFHANDLERINT pfnHandler)
{
    return DBGFR3InfoRegisterInternalEx(pVM, pszName, pszDesc, pfnHandler, 0);
}


/**
 * Register a info handler owned by an internal component.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pszName     The identifier of the info.
 * @param   pszDesc     The description of the info and any arguments the handler may take.
 * @param   pfnHandler  The handler function to be called to display the info.
 * @param   fFlags      Flags, see the DBGFINFO_FLAGS_*.
 */
VMMR3_INT_DECL(int) DBGFR3InfoRegisterInternalEx(PVM pVM, const char *pszName, const char *pszDesc,
                                                 PFNDBGFHANDLERINT pfnHandler, uint32_t fFlags)
{
    LogFlow(("DBGFR3InfoRegisterInternal: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p fFlags=%x\n",
             pszName, pszName, pszDesc, pszDesc, pfnHandler, fFlags));

    /*
     * Validate the specific stuff.
     */
    AssertPtrReturn(pfnHandler, VERR_INVALID_POINTER);

    /*
     * Register
     */
    PDBGFINFO pInfo;
    int rc = dbgfR3InfoRegister(pVM->pUVM, pszName, pszDesc, fFlags, &pInfo);
    if (RT_SUCCESS(rc))
    {
        pInfo->enmType = DBGFINFOTYPE_INT;
        pInfo->u.Int.pfnHandler = pfnHandler;
        RTCritSectRwLeaveExcl(&pVM->pUVM->dbgf.s.CritSect);
    }

    return rc;
}


/**
 * Register a info handler owned by an external component.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pszName     The identifier of the info.
 * @param   pszDesc     The description of the info and any arguments the handler may take.
 * @param   pfnHandler  The handler function to be called to display the info.
 * @param   pvUser      User argument to be passed to the handler.
 */
VMMR3DECL(int) DBGFR3InfoRegisterExternal(PUVM pUVM, const char *pszName, const char *pszDesc,
                                          PFNDBGFHANDLEREXT pfnHandler, void *pvUser)
{
    LogFlow(("DBGFR3InfoRegisterExternal: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p pvUser=%p\n",
             pszName, pszName, pszDesc, pszDesc, pfnHandler, pvUser));

    /*
     * Validate the specific stuff.
     */
    AssertPtrReturn(pfnHandler, VERR_INVALID_POINTER);
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);

    /*
     * Register
     */
    PDBGFINFO pInfo;
    int rc = dbgfR3InfoRegister(pUVM, pszName, pszDesc, 0, &pInfo);
    if (RT_SUCCESS(rc))
    {
        pInfo->enmType = DBGFINFOTYPE_EXT;
        pInfo->u.Ext.pfnHandler = pfnHandler;
        pInfo->u.Ext.pvUser = pvUser;
        RTCritSectRwLeaveExcl(&pUVM->dbgf.s.CritSect);
    }

    return rc;
}


/**
 * Deregister one(/all) info handler(s) owned by a device.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDevIns     Device instance.
 * @param   pszName     The identifier of the info. If NULL all owned by the device.
 */
VMMR3_INT_DECL(int) DBGFR3InfoDeregisterDevice(PVM pVM, PPDMDEVINS pDevIns, const char *pszName)
{
    LogFlow(("DBGFR3InfoDeregisterDevice: pDevIns=%p pszName=%p:{%s}\n", pDevIns, pszName, pszName));

    /*
     * Validate input.
     */
    AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pszName, VERR_INVALID_POINTER);
    size_t cchName = pszName ? strlen(pszName) : 0;
    PUVM pUVM = pVM->pUVM;

    /*
     * Enumerate the info handlers and free the requested entries.
     */
    int rc = RTCritSectRwEnterExcl(&pUVM->dbgf.s.CritSect); AssertRC(rc);
    rc = VERR_FILE_NOT_FOUND;
    PDBGFINFO pPrev = NULL;
    PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst;
    if (pszName)
    {
        /*
         * Free a specific one.
         */
        for (; pInfo; pPrev = pInfo, pInfo = pInfo->pNext)
            if (    pInfo->enmType == DBGFINFOTYPE_DEV
                &&  pInfo->u.Dev.pDevIns == pDevIns
                &&  pInfo->cchName == cchName
                &&  !strcmp(pInfo->szName, pszName))
            {
                if (pPrev)
                    pPrev->pNext = pInfo->pNext;
                else
                    pUVM->dbgf.s.pInfoFirst = pInfo->pNext;
                MMR3HeapFree(pInfo);
                rc = VINF_SUCCESS;
                break;
            }
    }
    else
    {
        /*
         * Free all owned by the device.
         */
        while (pInfo != NULL)
            if (    pInfo->enmType == DBGFINFOTYPE_DEV
                &&  pInfo->u.Dev.pDevIns == pDevIns)
            {
                PDBGFINFO volatile pFree = pInfo;
                pInfo = pInfo->pNext;
                if (pPrev)
                    pPrev->pNext = pInfo;
                else
                    pUVM->dbgf.s.pInfoFirst = pInfo;
                MMR3HeapFree(pFree);
            }
            else
            {
                pPrev = pInfo;
                pInfo = pInfo->pNext;
            }
        rc = VINF_SUCCESS;
    }
    int rc2 = RTCritSectRwLeaveExcl(&pUVM->dbgf.s.CritSect);
    AssertRC(rc2);
    AssertRC(rc);
    LogFlow(("DBGFR3InfoDeregisterDevice: returns %Rrc\n", rc));
    return rc;
}

/**
 * Deregister one(/all) info handler(s) owned by a driver.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pDrvIns     Driver instance.
 * @param   pszName     The identifier of the info. If NULL all owned by the driver.
 */
VMMR3_INT_DECL(int) DBGFR3InfoDeregisterDriver(PVM pVM, PPDMDRVINS pDrvIns, const char *pszName)
{
    LogFlow(("DBGFR3InfoDeregisterDriver: pDrvIns=%p pszName=%p:{%s}\n", pDrvIns, pszName, pszName));

    /*
     * Validate input.
     */
    AssertPtrReturn(pDrvIns, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pszName, VERR_INVALID_POINTER);
    size_t cchName = pszName ? strlen(pszName) : 0;
    PUVM pUVM = pVM->pUVM;

    /*
     * Enumerate the info handlers and free the requested entries.
     */
    int rc = RTCritSectRwEnterExcl(&pUVM->dbgf.s.CritSect); AssertRC(rc);
    rc = VERR_FILE_NOT_FOUND;
    PDBGFINFO pPrev = NULL;
    PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst;
    if (pszName)
    {
        /*
         * Free a specific one.
         */
        for (; pInfo; pPrev = pInfo, pInfo = pInfo->pNext)
            if (    pInfo->enmType == DBGFINFOTYPE_DRV
                &&  pInfo->u.Drv.pDrvIns == pDrvIns
                &&  pInfo->cchName == cchName
                &&  !strcmp(pInfo->szName, pszName))
            {
                if (pPrev)
                    pPrev->pNext = pInfo->pNext;
                else
                    pUVM->dbgf.s.pInfoFirst = pInfo->pNext;
                MMR3HeapFree(pInfo);
                rc = VINF_SUCCESS;
                break;
            }
    }
    else
    {
        /*
         * Free all owned by the driver.
         */
        while (pInfo != NULL)
            if (    pInfo->enmType == DBGFINFOTYPE_DRV
                &&  pInfo->u.Drv.pDrvIns == pDrvIns)
            {
                PDBGFINFO volatile pFree = pInfo;
                pInfo = pInfo->pNext;
                if (pPrev)
                    pPrev->pNext = pInfo;
                else
                    pUVM->dbgf.s.pInfoFirst = pInfo;
                MMR3HeapFree(pFree);
            }
            else
            {
                pPrev = pInfo;
                pInfo = pInfo->pNext;
            }
        rc = VINF_SUCCESS;
    }
    int rc2 = RTCritSectRwLeaveExcl(&pUVM->dbgf.s.CritSect);
    AssertRC(rc2);
    AssertRC(rc);
    LogFlow(("DBGFR3InfoDeregisterDriver: returns %Rrc\n", rc));
    return rc;
}


/**
 * Internal deregistration helper.
 *
 * @returns VBox status code.
 * @param   pUVM        Pointer to the VM.
 * @param   pszName     The identifier of the info.
 * @param   enmType     The info owner type.
 */
static int dbgfR3InfoDeregister(PUVM pUVM, const char *pszName, DBGFINFOTYPE enmType)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);

    /*
     * Find the info handler.
     */
    size_t cchName = strlen(pszName);
    int rc = RTCritSectRwEnterExcl(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);
    rc = VERR_FILE_NOT_FOUND;
    PDBGFINFO pPrev = NULL;
    PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst;
    for (; pInfo; pPrev = pInfo, pInfo = pInfo->pNext)
        if (    pInfo->cchName == cchName
            &&  !strcmp(pInfo->szName, pszName)
            &&  pInfo->enmType == enmType)
        {
            if (pPrev)
                pPrev->pNext = pInfo->pNext;
            else
                pUVM->dbgf.s.pInfoFirst = pInfo->pNext;
            MMR3HeapFree(pInfo);
            rc = VINF_SUCCESS;
            break;
        }
    int rc2 = RTCritSectRwLeaveExcl(&pUVM->dbgf.s.CritSect);
    AssertRC(rc2);
    AssertRC(rc);
    LogFlow(("dbgfR3InfoDeregister: returns %Rrc\n", rc));
    return rc;
}


/**
 * Deregister a info handler owned by an internal component.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pszName     The identifier of the info. If NULL all owned by the device.
 */
VMMR3_INT_DECL(int) DBGFR3InfoDeregisterInternal(PVM pVM, const char *pszName)
{
    LogFlow(("DBGFR3InfoDeregisterInternal: pszName=%p:{%s}\n", pszName, pszName));
    return dbgfR3InfoDeregister(pVM->pUVM, pszName, DBGFINFOTYPE_INT);
}


/**
 * Deregister a info handler owned by an external component.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pszName     The identifier of the info. If NULL all owned by the device.
 */
VMMR3DECL(int) DBGFR3InfoDeregisterExternal(PUVM pUVM, const char *pszName)
{
    LogFlow(("DBGFR3InfoDeregisterExternal: pszName=%p:{%s}\n", pszName, pszName));
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    return dbgfR3InfoDeregister(pUVM, pszName, DBGFINFOTYPE_EXT);
}


/**
 * Worker for DBGFR3InfoEx.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       Which CPU to run EMT bound handlers on.  VMCPUID_ANY or
 *                      a valid CPU ID.
 * @param   pszName     What to dump.
 * @param   pszArgs     Arguments, optional.
 * @param   pHlp        Output helper, optional.
 */
static DECLCALLBACK(int) dbgfR3Info(PUVM pUVM, VMCPUID idCpu, const char *pszName, const char *pszArgs, PCDBGFINFOHLP pHlp)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    AssertPtrNullReturn(pszArgs, VERR_INVALID_POINTER);
    if (pHlp)
    {
        AssertPtrReturn(pHlp, VERR_INVALID_PARAMETER);
        AssertPtrReturn(pHlp->pfnPrintf, VERR_INVALID_PARAMETER);
        AssertPtrReturn(pHlp->pfnPrintfV, VERR_INVALID_PARAMETER);
    }
    else
        pHlp = &g_dbgfR3InfoLogHlp;
    Assert(idCpu == NIL_VMCPUID || idCpu < pUVM->cCpus); /* if not nil, we're on that EMT already. */

    /*
     * Find the info handler.
     */
    size_t cchName = strlen(pszName);
    int rc = RTCritSectRwEnterShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);
    PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst;
    for (; pInfo; pInfo = pInfo->pNext)
        if (    pInfo->cchName == cchName
            &&  !memcmp(pInfo->szName, pszName, cchName))
            break;
    if (pInfo)
    {
        /*
         * Found it.
         */
        VMCPUID idDstCpu = NIL_VMCPUID;
        if ((pInfo->fFlags & (DBGFINFO_FLAGS_RUN_ON_EMT | DBGFINFO_FLAGS_ALL_EMTS)) && idCpu == NIL_VMCPUID)
            idDstCpu = pInfo->fFlags & DBGFINFO_FLAGS_ALL_EMTS ? VMCPUID_ALL : VMCPUID_ANY;

        rc = VINF_SUCCESS;
        switch (pInfo->enmType)
        {
            case DBGFINFOTYPE_DEV:
                if (idDstCpu != NIL_VMCPUID)
                    rc = VMR3ReqPriorityCallWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Dev.pfnHandler, 3,
                                                  pInfo->u.Dev.pDevIns, pHlp, pszArgs);
                else
                    pInfo->u.Dev.pfnHandler(pInfo->u.Dev.pDevIns, pHlp, pszArgs);
                break;

            case DBGFINFOTYPE_DRV:
                if (idDstCpu != NIL_VMCPUID)
                    rc = VMR3ReqPriorityCallWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Drv.pfnHandler, 3,
                                                  pInfo->u.Drv.pDrvIns, pHlp, pszArgs);
                else
                    pInfo->u.Drv.pfnHandler(pInfo->u.Drv.pDrvIns, pHlp, pszArgs);
                break;

            case DBGFINFOTYPE_INT:
                if (RT_VALID_PTR(pUVM->pVM))
                {
                    if (idDstCpu != NIL_VMCPUID)
                        rc = VMR3ReqPriorityCallWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Int.pfnHandler, 3,
                                                      pUVM->pVM, pHlp, pszArgs);
                    else
                        pInfo->u.Int.pfnHandler(pUVM->pVM, pHlp, pszArgs);
                }
                else
                    rc = VERR_INVALID_VM_HANDLE;
                break;

            case DBGFINFOTYPE_EXT:
                if (idDstCpu != NIL_VMCPUID)
                    rc = VMR3ReqPriorityCallWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Ext.pfnHandler, 3,
                                                  pInfo->u.Ext.pvUser, pHlp, pszArgs);
                else
                    pInfo->u.Ext.pfnHandler(pInfo->u.Ext.pvUser, pHlp, pszArgs);
                break;

            default:
                AssertMsgFailedReturn(("Invalid info type enmType=%d\n", pInfo->enmType), VERR_IPE_NOT_REACHED_DEFAULT_CASE);
        }

        int rc2 = RTCritSectRwLeaveShared(&pUVM->dbgf.s.CritSect);
        AssertRC(rc2);
    }
    else
    {
        rc = RTCritSectRwLeaveShared(&pUVM->dbgf.s.CritSect);
        AssertRC(rc);
        rc = VERR_FILE_NOT_FOUND;
    }
    return rc;
}


/**
 * Display a piece of info writing to the supplied handler.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pszName     The identifier of the info to display.
 * @param   pszArgs     Arguments to the info handler.
 * @param   pHlp        The output helper functions. If NULL the logger will be used.
 */
VMMR3DECL(int) DBGFR3Info(PUVM pUVM, const char *pszName, const char *pszArgs, PCDBGFINFOHLP pHlp)
{
    return DBGFR3InfoEx(pUVM, NIL_VMCPUID, pszName, pszArgs, pHlp);
}


/**
 * Display a piece of info writing to the supplied handler.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       The CPU to exectue the request on.  Pass NIL_VMCPUID
 *                      to not involve any EMT unless necessary.
 * @param   pszName     The identifier of the info to display.
 * @param   pszArgs     Arguments to the info handler.
 * @param   pHlp        The output helper functions. If NULL the logger will be used.
 */
VMMR3DECL(int) DBGFR3InfoEx(PUVM pUVM, VMCPUID idCpu, const char *pszName, const char *pszArgs, PCDBGFINFOHLP pHlp)
{
    /*
     * Some input validation.
     */
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(   idCpu != VMCPUID_ANY_QUEUE
                 && idCpu != VMCPUID_ALL
                 && idCpu != VMCPUID_ALL_REVERSE, VERR_INVALID_PARAMETER);

    /*
     * Run on any specific EMT?
     */
    if (idCpu == NIL_VMCPUID)
        return dbgfR3Info(pUVM, NIL_VMCPUID, pszName, pszArgs, pHlp);
    return VMR3ReqPriorityCallWaitU(pUVM, idCpu,
                                    (PFNRT)dbgfR3Info, 5, pUVM, idCpu, pszName, pszArgs, pHlp);
}


/**
 * Wrapper for DBGFR3Info that outputs to the release log.
 *
 * @returns See DBGFR3Info.
 * @param   pUVM        The user mode VM handle.
 * @param   pszName     See DBGFR3Info.
 * @param   pszArgs     See DBGFR3Info.
 */
VMMR3DECL(int) DBGFR3InfoLogRel(PUVM pUVM, const char *pszName, const char *pszArgs)
{
    return DBGFR3InfoEx(pUVM, NIL_VMCPUID, pszName, pszArgs, &g_dbgfR3InfoLogRelHlp);
}


/**
 * Wrapper for DBGFR3Info that outputs to standard error.
 *
 * @returns See DBGFR3Info.
 * @param   pUVM        The user mode VM handle.
 * @param   pszName     See DBGFR3Info.
 * @param   pszArgs     See DBGFR3Info.
 */
VMMR3DECL(int) DBGFR3InfoStdErr(PUVM pUVM, const char *pszName, const char *pszArgs)
{
    return DBGFR3InfoEx(pUVM, NIL_VMCPUID, pszName, pszArgs, &g_dbgfR3InfoStdErrHlp);
}


/**
 * Display several info items.
 *
 * This is intended used by the fatal error dump only.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pszIncludePat   Simple string pattern of info items to include.
 * @param   pszExcludePat   Simple string pattern of info items to exclude.
 * @param   pszSepFmt       Item separator format string.  The item name will be
 *                          given as parameter.
 * @param   pHlp            The output helper functions.  If NULL the logger
 *                          will be used.
 *
 * @thread  EMT
 */
VMMR3_INT_DECL(int) DBGFR3InfoMulti(PVM pVM, const char *pszIncludePat, const char *pszExcludePat, const char *pszSepFmt,
                                    PCDBGFINFOHLP pHlp)
{
    /*
     * Validate input.
     */
    PUVM pUVM = pVM->pUVM;
    VM_ASSERT_EMT_RETURN(pVM, VERR_VM_THREAD_NOT_EMT);
    AssertPtrReturn(pszIncludePat, VERR_INVALID_POINTER);
    AssertPtrReturn(pszExcludePat, VERR_INVALID_POINTER);
    if (pHlp)
    {
        AssertPtrReturn(pHlp->pfnPrintf, VERR_INVALID_POINTER);
        AssertPtrReturn(pHlp->pfnPrintfV, VERR_INVALID_POINTER);
    }
    else
        pHlp = &g_dbgfR3InfoLogHlp;

    size_t const cchIncludePat = strlen(pszIncludePat);
    size_t const cchExcludePat = strlen(pszExcludePat);
    const char *pszArgs = "";

    /*
     * Enumerate the info handlers and call the ones matching.
     * Note! We won't leave the critical section here...
     */
    int rc = RTCritSectRwEnterShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);
    rc = VWRN_NOT_FOUND;
    for (PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst; pInfo; pInfo = pInfo->pNext)
    {
        if (   RTStrSimplePatternMultiMatch(pszIncludePat, cchIncludePat, pInfo->szName, pInfo->cchName, NULL)
            && !RTStrSimplePatternMultiMatch(pszExcludePat, cchExcludePat, pInfo->szName, pInfo->cchName, NULL))
        {
            pHlp->pfnPrintf(pHlp, pszSepFmt, pInfo->szName);

            VMCPUID idDstCpu = NIL_VMCPUID;
            if (pInfo->fFlags & (DBGFINFO_FLAGS_RUN_ON_EMT | DBGFINFO_FLAGS_ALL_EMTS))
                idDstCpu = pInfo->fFlags & DBGFINFO_FLAGS_ALL_EMTS ? VMCPUID_ALL : VMCPUID_ANY;

            rc = VINF_SUCCESS;
            switch (pInfo->enmType)
            {
                case DBGFINFOTYPE_DEV:
                    if (idDstCpu != NIL_VMCPUID)
                        rc = VMR3ReqPriorityCallVoidWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Dev.pfnHandler, 3,
                                                          pInfo->u.Dev.pDevIns, pHlp, pszArgs);
                    else
                        pInfo->u.Dev.pfnHandler(pInfo->u.Dev.pDevIns, pHlp, pszArgs);
                    break;

                case DBGFINFOTYPE_DRV:
                    if (idDstCpu != NIL_VMCPUID)
                        rc = VMR3ReqPriorityCallVoidWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Drv.pfnHandler, 3,
                                                          pInfo->u.Drv.pDrvIns, pHlp, pszArgs);
                    else
                        pInfo->u.Drv.pfnHandler(pInfo->u.Drv.pDrvIns, pHlp, pszArgs);
                    break;

                case DBGFINFOTYPE_INT:
                    if (idDstCpu != NIL_VMCPUID)
                        rc = VMR3ReqPriorityCallVoidWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Int.pfnHandler, 3,
                                                          pVM, pHlp, pszArgs);
                    else
                        pInfo->u.Int.pfnHandler(pVM, pHlp, pszArgs);
                    break;

                case DBGFINFOTYPE_EXT:
                    if (idDstCpu != NIL_VMCPUID)
                        rc = VMR3ReqPriorityCallVoidWaitU(pUVM, idDstCpu, (PFNRT)pInfo->u.Ext.pfnHandler, 3,
                                                          pInfo->u.Ext.pvUser, pHlp, pszArgs);
                    else
                        pInfo->u.Ext.pfnHandler(pInfo->u.Ext.pvUser, pHlp, pszArgs);
                    break;

                default:
                    AssertMsgFailedReturn(("Invalid info type enmType=%d\n", pInfo->enmType), VERR_IPE_NOT_REACHED_DEFAULT_CASE);
            }
        }
    }
    int rc2 = RTCritSectRwLeaveShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc2);

    return rc;
}


/**
 * Enumerate all the register info handlers.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pfnCallback     Pointer to callback function.
 * @param   pvUser          User argument to pass to the callback.
 */
VMMR3DECL(int) DBGFR3InfoEnum(PUVM pUVM, PFNDBGFINFOENUM pfnCallback, void *pvUser)
{
    LogFlow(("DBGFR3InfoLog: pfnCallback=%p pvUser=%p\n", pfnCallback, pvUser));

    /*
     * Validate input.
     */
    if (!pfnCallback)
    {
        AssertMsgFailed(("!pfnCallback\n"));
        return VERR_INVALID_PARAMETER;
    }
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);

    /*
     * Enter and enumerate.
     */
    int rc = RTCritSectRwEnterShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);

    rc = VINF_SUCCESS;
    for (PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst; RT_SUCCESS(rc) && pInfo; pInfo = pInfo->pNext)
        rc = pfnCallback(pUVM, pInfo->szName, pInfo->pszDesc, pvUser);

    /*
     * Leave and exit.
     */
    int rc2 = RTCritSectRwLeaveShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc2);

    LogFlow(("DBGFR3InfoLog: returns %Rrc\n", rc));
    return rc;
}


/**
 * Info handler, internal version.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        Callback functions for doing output.
 * @param   pszArgs     Argument string. Optional and specific to the handler.
 */
static DECLCALLBACK(void) dbgfR3InfoHelp(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    LogFlow(("dbgfR3InfoHelp: pszArgs=%s\n", pszArgs));

    /*
     * Enter and enumerate.
     */
    PUVM pUVM = pVM->pUVM;
    int rc = RTCritSectRwEnterShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);

    if (pszArgs && *pszArgs)
    {
        for (PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst; pInfo; pInfo = pInfo->pNext)
        {
            const char *psz = strstr(pszArgs, pInfo->szName);
            if (    psz
                &&  (   psz == pszArgs
                     || RT_C_IS_SPACE(psz[-1]))
                &&  (   !psz[pInfo->cchName]
                     || RT_C_IS_SPACE(psz[pInfo->cchName])))
                pHlp->pfnPrintf(pHlp, "%-16s  %s\n",
                                pInfo->szName, pInfo->pszDesc);
        }
    }
    else
    {
        for (PDBGFINFO pInfo = pUVM->dbgf.s.pInfoFirst; pInfo; pInfo = pInfo->pNext)
            pHlp->pfnPrintf(pHlp, "%-16s  %s\n",
                            pInfo->szName, pInfo->pszDesc);
    }

    /*
     * Leave and exit.
     */
    rc = RTCritSectRwLeaveShared(&pUVM->dbgf.s.CritSect);
    AssertRC(rc);
}

