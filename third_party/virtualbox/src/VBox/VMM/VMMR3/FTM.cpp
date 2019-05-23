/* $Id: FTM.cpp $ */
/** @file
 * FTM - Fault Tolerance Manager
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_FTM
#include <VBox/vmm/ftm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/vmm.h>
#include "FTMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/tcp.h>
#include <iprt/socket.h>
#include <iprt/semaphore.h>
#include <iprt/asm.h>


/*******************************************************************************
 * Structures and Typedefs                                                     *
 *******************************************************************************/

/**
 * TCP stream header.
 *
 * This is an extra layer for fixing the problem with figuring out when the SSM
 * stream ends.
 */
typedef struct FTMTCPHDR
{
    /** Magic value. */
    uint32_t    u32Magic;
    /** The size of the data block following this header.
     * 0 indicates the end of the stream, while UINT32_MAX indicates
     * cancelation. */
    uint32_t    cb;
} FTMTCPHDR;
/** Magic value for FTMTCPHDR::u32Magic. (Egberto Gismonti Amin) */
#define FTMTCPHDR_MAGIC       UINT32_C(0x19471205)
/** The max block size. */
#define FTMTCPHDR_MAX_SIZE    UINT32_C(0x00fffff8)

/**
 * TCP stream header.
 *
 * This is an extra layer for fixing the problem with figuring out when the SSM
 * stream ends.
 */
typedef struct FTMTCPHDRMEM
{
    /** Magic value. */
    uint32_t    u32Magic;
    /** Size (Uncompressed) of the pages following the header. */
    uint32_t    cbPageRange;
    /** GC Physical address of the page(s) to sync. */
    RTGCPHYS    GCPhys;
    /** The size of the data block following this header.
     * 0 indicates the end of the stream, while UINT32_MAX indicates
     * cancelation. */
    uint32_t    cb;
} FTMTCPHDRMEM;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const char g_szWelcome[] = "VirtualBox-Fault-Tolerance-Sync-1.0\n";

static DECLCALLBACK(int) ftmR3PageTreeDestroyCallback(PAVLGCPHYSNODECORE pBaseNode, void *pvUser);

/**
 * Initializes the FTM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) FTMR3Init(PVM pVM)
{
    /*
     * Assert alignment and sizes.
     */
    AssertCompile(sizeof(pVM->ftm.s) <= sizeof(pVM->ftm.padding));
    AssertCompileMemberAlignment(FTM, CritSect, sizeof(uintptr_t));

    /** @todo saved state for master nodes! */
    pVM->ftm.s.pszAddress               = NULL;
    pVM->ftm.s.pszPassword              = NULL;
    pVM->fFaultTolerantMaster           = false;
    pVM->ftm.s.fIsStandbyNode           = false;
    pVM->ftm.s.standby.hServer          = NIL_RTTCPSERVER;
    pVM->ftm.s.hShutdownEvent           = NIL_RTSEMEVENT;
    pVM->ftm.s.hSocket                  = NIL_RTSOCKET;

    /*
     * Initialize the PGM critical section.
     */
    int rc = PDMR3CritSectInit(pVM, &pVM->ftm.s.CritSect, RT_SRC_POS, "FTM");
    AssertRCReturn(rc, rc);

    /*
     * Register statistics.
     */
    STAM_REL_REG(pVM, &pVM->ftm.s.StatReceivedMem,               STAMTYPE_COUNTER, "/FT/Received/Mem",                  STAMUNIT_BYTES, "The amount of memory pages that was received.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatReceivedState,             STAMTYPE_COUNTER, "/FT/Received/State",                STAMUNIT_BYTES, "The amount of state information that was received.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatSentMem,                   STAMTYPE_COUNTER, "/FT/Sent/Mem",                      STAMUNIT_BYTES, "The amount of memory pages that was sent.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatSentState,                 STAMTYPE_COUNTER, "/FT/Sent/State",                    STAMUNIT_BYTES, "The amount of state information that was sent.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatDeltaVM,                   STAMTYPE_COUNTER, "/FT/Sync/DeltaVM",                  STAMUNIT_OCCURENCES, "Number of delta vm syncs.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatFullSync,                  STAMTYPE_COUNTER, "/FT/Sync/Full",                     STAMUNIT_OCCURENCES, "Number of full vm syncs.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatDeltaMem,                  STAMTYPE_COUNTER, "/FT/Sync/DeltaMem",                 STAMUNIT_OCCURENCES, "Number of delta mem syncs.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatCheckpointStorage,         STAMTYPE_COUNTER, "/FT/Checkpoint/Storage",            STAMUNIT_OCCURENCES, "Number of storage checkpoints.");
    STAM_REL_REG(pVM, &pVM->ftm.s.StatCheckpointNetwork,         STAMTYPE_COUNTER, "/FT/Checkpoint/Network",            STAMUNIT_OCCURENCES, "Number of network checkpoints.");
#ifdef VBOX_WITH_STATISTICS
    STAM_REG(pVM,     &pVM->ftm.s.StatCheckpoint,                STAMTYPE_PROFILE, "/FT/Checkpoint",                    STAMUNIT_TICKS_PER_CALL, "Profiling of FTMR3SetCheckpoint.");
    STAM_REG(pVM,     &pVM->ftm.s.StatCheckpointPause,           STAMTYPE_PROFILE, "/FT/Checkpoint/Pause",              STAMUNIT_TICKS_PER_CALL, "Profiling of FTMR3SetCheckpoint.");
    STAM_REG(pVM,     &pVM->ftm.s.StatCheckpointResume,          STAMTYPE_PROFILE, "/FT/Checkpoint/Resume",             STAMUNIT_TICKS_PER_CALL, "Profiling of FTMR3SetCheckpoint.");
    STAM_REG(pVM,     &pVM->ftm.s.StatSentMemRAM,                STAMTYPE_COUNTER, "/FT/Sent/Mem/RAM",                  STAMUNIT_BYTES, "The amount of memory pages that was sent.");
    STAM_REG(pVM,     &pVM->ftm.s.StatSentMemMMIO2,              STAMTYPE_COUNTER, "/FT/Sent/Mem/MMIO2",                STAMUNIT_BYTES, "The amount of memory pages that was sent.");
    STAM_REG(pVM,     &pVM->ftm.s.StatSentMemShwROM,             STAMTYPE_COUNTER, "/FT/Sent/Mem/ShwROM",               STAMUNIT_BYTES, "The amount of memory pages that was sent.");
    STAM_REG(pVM,     &pVM->ftm.s.StatSentStateWrite,            STAMTYPE_COUNTER, "/FT/Sent/State/Writes",             STAMUNIT_BYTES, "The nr of write calls.");
#endif
    return VINF_SUCCESS;
}

/**
 * Terminates the FTM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM itself is at this point powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) FTMR3Term(PVM pVM)
{
    if (pVM->ftm.s.hShutdownEvent != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(pVM->ftm.s.hShutdownEvent);
        pVM->ftm.s.hShutdownEvent = NIL_RTSEMEVENT;
    }
    if (pVM->ftm.s.hSocket != NIL_RTSOCKET)
    {
        RTTcpClientClose(pVM->ftm.s.hSocket);
        pVM->ftm.s.hSocket = NIL_RTSOCKET;
    }
    if (pVM->ftm.s.standby.hServer)
    {
        RTTcpServerDestroy(pVM->ftm.s.standby.hServer);
        pVM->ftm.s.standby.hServer = NULL;
    }
    if (pVM->ftm.s.pszAddress)
        RTMemFree(pVM->ftm.s.pszAddress);
    if (pVM->ftm.s.pszPassword)
        RTMemFree(pVM->ftm.s.pszPassword);

    /* Remove all pending memory updates. */
    if (pVM->ftm.s.standby.pPhysPageTree)
    {
        RTAvlGCPhysDestroy(&pVM->ftm.s.standby.pPhysPageTree, ftmR3PageTreeDestroyCallback, NULL);
        pVM->ftm.s.standby.pPhysPageTree = NULL;
    }

    pVM->ftm.s.pszAddress  = NULL;
    pVM->ftm.s.pszPassword = NULL;

    PDMR3CritSectDelete(&pVM->ftm.s.CritSect);
    return VINF_SUCCESS;
}


static int ftmR3TcpWriteACK(PVM pVM)
{
    int rc = RTTcpWrite(pVM->ftm.s.hSocket, RT_STR_TUPLE("ACK\n"));
    if (RT_FAILURE(rc))
    {
        LogRel(("FTSync: RTTcpWrite(,ACK,) -> %Rrc\n", rc));
    }
    return rc;
}


static int ftmR3TcpWriteNACK(PVM pVM, int32_t rc2, const char *pszMsgText = NULL)
{
    char    szMsg[256];
    size_t  cch;
    if (pszMsgText && *pszMsgText)
    {
        cch = RTStrPrintf(szMsg, sizeof(szMsg), "NACK=%d;%s\n", rc2, pszMsgText);
        for (size_t off = 6; off + 1 < cch; off++)
            if (szMsg[off] == '\n')
                szMsg[off] = '\r';
    }
    else
        cch = RTStrPrintf(szMsg, sizeof(szMsg), "NACK=%d\n", rc2);
    int rc = RTTcpWrite(pVM->ftm.s.hSocket, szMsg, cch);
    if (RT_FAILURE(rc))
        LogRel(("FTSync: RTTcpWrite(,%s,%zu) -> %Rrc\n", szMsg, cch, rc));
    return rc;
}

/**
 * Reads a string from the socket.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pszBuf      The output buffer.
 * @param   cchBuf      The size of the output buffer.
 *
 */
static int ftmR3TcpReadLine(PVM pVM, char *pszBuf, size_t cchBuf)
{
    char       *pszStart = pszBuf;
    RTSOCKET    Sock     = pVM->ftm.s.hSocket;

    AssertReturn(cchBuf > 1, VERR_INTERNAL_ERROR);
    *pszBuf = '\0';

    /* dead simple approach. */
    for (;;)
    {
        char ch;
        int rc = RTTcpRead(Sock, &ch, sizeof(ch), NULL);
        if (RT_FAILURE(rc))
        {
            LogRel(("FTSync: RTTcpRead -> %Rrc while reading string ('%s')\n", rc, pszStart));
            return rc;
        }
        if (    ch == '\n'
            ||  ch == '\0')
            return VINF_SUCCESS;
        if (cchBuf <= 1)
        {
            LogRel(("FTSync: String buffer overflow: '%s'\n", pszStart));
            return VERR_BUFFER_OVERFLOW;
        }
        *pszBuf++ = ch;
        *pszBuf = '\0';
        cchBuf--;
    }
}

/**
 * Reads an ACK or NACK.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pszWhich            Which ACK is this this?
 * @param   pszNAckMsg          Optional NACK message.
 */
static int ftmR3TcpReadACK(PVM pVM, const char *pszWhich, const char *pszNAckMsg = NULL)
{
    char szMsg[256];
    int rc = ftmR3TcpReadLine(pVM, szMsg, sizeof(szMsg));
    if (RT_FAILURE(rc))
        return rc;

    if (!strcmp(szMsg, "ACK"))
        return VINF_SUCCESS;

    if (!strncmp(szMsg, RT_STR_TUPLE("NACK=")))
    {
        char *pszMsgText = strchr(szMsg, ';');
        if (pszMsgText)
            *pszMsgText++ = '\0';

        int32_t vrc2;
        rc = RTStrToInt32Full(&szMsg[sizeof("NACK=") - 1], 10, &vrc2);
        if (rc == VINF_SUCCESS)
        {
            /*
             * Well formed NACK, transform it into an error.
             */
            if (pszNAckMsg)
            {
                LogRel(("FTSync: %s: NACK=%Rrc (%d)\n", pszWhich, vrc2, vrc2));
                return VERR_INTERNAL_ERROR;
            }

            if (pszMsgText)
            {
                pszMsgText = RTStrStrip(pszMsgText);
                for (size_t off = 0; pszMsgText[off]; off++)
                    if (pszMsgText[off] == '\r')
                        pszMsgText[off] = '\n';

                LogRel(("FTSync: %s: NACK=%Rrc (%d) - '%s'\n", pszWhich, vrc2, vrc2, pszMsgText));
            }
            return VERR_INTERNAL_ERROR_2;
        }

        if (pszMsgText)
            pszMsgText[-1] = ';';
    }
    return VERR_INTERNAL_ERROR_3;
}

/**
 * Submitts a command to the destination and waits for the ACK.
 *
 * @returns VBox status code.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pszCommand          The command.
 * @param   fWaitForAck         Whether to wait for the ACK.
 */
static int ftmR3TcpSubmitCommand(PVM pVM, const char *pszCommand, bool fWaitForAck = true)
{
    int rc = RTTcpSgWriteL(pVM->ftm.s.hSocket, 2, pszCommand, strlen(pszCommand), RT_STR_TUPLE("\n"));
    if (RT_FAILURE(rc))
        return rc;
    if (!fWaitForAck)
        return VINF_SUCCESS;
    return ftmR3TcpReadACK(pVM, pszCommand);
}

/**
 * @interface_method_impl{SSMSTRMOPS,pfnWrite}
 */
static DECLCALLBACK(int) ftmR3TcpOpWrite(void *pvUser, uint64_t offStream, const void *pvBuf, size_t cbToWrite)
{
    PVM pVM = (PVM)pvUser;
    NOREF(offStream);

    AssertReturn(cbToWrite > 0, VINF_SUCCESS);
    AssertReturn(cbToWrite < UINT32_MAX, VERR_OUT_OF_RANGE);
    AssertReturn(pVM->fFaultTolerantMaster, VERR_INVALID_HANDLE);

    STAM_COUNTER_INC(&pVM->ftm.s.StatSentStateWrite);
    for (;;)
    {
        FTMTCPHDR Hdr;
        Hdr.u32Magic = FTMTCPHDR_MAGIC;
        Hdr.cb       = RT_MIN((uint32_t)cbToWrite, FTMTCPHDR_MAX_SIZE);
        int rc = RTTcpSgWriteL(pVM->ftm.s.hSocket, 2, &Hdr, sizeof(Hdr), pvBuf, (size_t)Hdr.cb);
        if (RT_FAILURE(rc))
        {
            LogRel(("FTSync/TCP: Write error: %Rrc (cb=%#x)\n", rc, Hdr.cb));
            return rc;
        }
        pVM->ftm.s.StatSentState.c      += Hdr.cb + sizeof(Hdr);
        pVM->ftm.s.syncstate.uOffStream += Hdr.cb;
        if (Hdr.cb == cbToWrite)
            return VINF_SUCCESS;

        /* advance */
        cbToWrite -= Hdr.cb;
        pvBuf = (uint8_t const *)pvBuf + Hdr.cb;
    }
}


/**
 * Selects and poll for close condition.
 *
 * We can use a relatively high poll timeout here since it's only used to get
 * us out of error paths.  In the normal cause of events, we'll get a
 * end-of-stream header.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 */
static int ftmR3TcpReadSelect(PVM pVM)
{
    int rc;
    do
    {
        rc = RTTcpSelectOne(pVM->ftm.s.hSocket, 1000);
        if (RT_FAILURE(rc) && rc != VERR_TIMEOUT)
        {
            pVM->ftm.s.syncstate.fIOError = true;
            LogRel(("FTSync/TCP: Header select error: %Rrc\n", rc));
            break;
        }
        if (pVM->ftm.s.syncstate.fStopReading)
        {
            rc = VERR_EOF;
            break;
        }
    } while (rc == VERR_TIMEOUT);
    return rc;
}


/**
 * @interface_method_impl{SSMSTRMOPS,pfnRead}
 */
static DECLCALLBACK(int) ftmR3TcpOpRead(void *pvUser, uint64_t offStream, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    PVM pVM = (PVM)pvUser;
    AssertReturn(!pVM->fFaultTolerantMaster, VERR_INVALID_HANDLE);
    NOREF(offStream);

    for (;;)
    {
        int rc;

        /*
         * Check for various conditions and may have been signalled.
         */
        if (pVM->ftm.s.syncstate.fEndOfStream)
            return VERR_EOF;
        if (pVM->ftm.s.syncstate.fStopReading)
            return VERR_EOF;
        if (pVM->ftm.s.syncstate.fIOError)
            return VERR_IO_GEN_FAILURE;

        /*
         * If there is no more data in the current block, read the next
         * block header.
         */
        if (!pVM->ftm.s.syncstate.cbReadBlock)
        {
            rc = ftmR3TcpReadSelect(pVM);
            if (RT_FAILURE(rc))
                return rc;
            FTMTCPHDR Hdr;
            rc = RTTcpRead(pVM->ftm.s.hSocket, &Hdr, sizeof(Hdr), NULL);
            if (RT_FAILURE(rc))
            {
                pVM->ftm.s.syncstate.fIOError = true;
                LogRel(("FTSync/TCP: Header read error: %Rrc\n", rc));
                return rc;
            }
            pVM->ftm.s.StatReceivedState.c += sizeof(Hdr);

            if (RT_UNLIKELY(   Hdr.u32Magic != FTMTCPHDR_MAGIC
                            || Hdr.cb > FTMTCPHDR_MAX_SIZE
                            || Hdr.cb == 0))
            {
                if (    Hdr.u32Magic == FTMTCPHDR_MAGIC
                    &&  (   Hdr.cb == 0
                         || Hdr.cb == UINT32_MAX)
                   )
                {
                    pVM->ftm.s.syncstate.fEndOfStream = true;
                    pVM->ftm.s.syncstate.cbReadBlock  = 0;
                    return Hdr.cb ? VERR_SSM_CANCELLED : VERR_EOF;
                }
                pVM->ftm.s.syncstate.fIOError = true;
                LogRel(("FTSync/TCP: Invalid block: u32Magic=%#x cb=%#x\n", Hdr.u32Magic, Hdr.cb));
                return VERR_IO_GEN_FAILURE;
            }

            pVM->ftm.s.syncstate.cbReadBlock = Hdr.cb;
            if (pVM->ftm.s.syncstate.fStopReading)
                return VERR_EOF;
        }

        /*
         * Read more data.
         */
        rc = ftmR3TcpReadSelect(pVM);
        if (RT_FAILURE(rc))
            return rc;

        uint32_t cb = (uint32_t)RT_MIN(pVM->ftm.s.syncstate.cbReadBlock, cbToRead);
        rc = RTTcpRead(pVM->ftm.s.hSocket, pvBuf, cb, pcbRead);
        if (RT_FAILURE(rc))
        {
            pVM->ftm.s.syncstate.fIOError = true;
            LogRel(("FTSync/TCP: Data read error: %Rrc (cb=%#x)\n", rc, cb));
            return rc;
        }
        if (pcbRead)
        {
            cb = (uint32_t)*pcbRead;
            pVM->ftm.s.StatReceivedState.c   += cb;
            pVM->ftm.s.syncstate.uOffStream  += cb;
            pVM->ftm.s.syncstate.cbReadBlock -= cb;
            return VINF_SUCCESS;
        }
        pVM->ftm.s.StatReceivedState.c   += cb;
        pVM->ftm.s.syncstate.uOffStream  += cb;
        pVM->ftm.s.syncstate.cbReadBlock -= cb;
        if (cbToRead == cb)
            return VINF_SUCCESS;

        /* Advance to the next block. */
        cbToRead -= cb;
        pvBuf = (uint8_t *)pvBuf + cb;
    }
}


/**
 * @interface_method_impl{SSMSTRMOPS,pfnSeek}
 */
static DECLCALLBACK(int) ftmR3TcpOpSeek(void *pvUser, int64_t offSeek, unsigned uMethod, uint64_t *poffActual)
{
    NOREF(pvUser); NOREF(offSeek); NOREF(uMethod); NOREF(poffActual);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{SSMSTRMOPS,pfnTell}
 */
static DECLCALLBACK(uint64_t) ftmR3TcpOpTell(void *pvUser)
{
    PVM pVM = (PVM)pvUser;
    return pVM->ftm.s.syncstate.uOffStream;
}


/**
 * @interface_method_impl{SSMSTRMOPS,pfnSize}
 */
static DECLCALLBACK(int) ftmR3TcpOpSize(void *pvUser, uint64_t *pcb)
{
    NOREF(pvUser); NOREF(pcb);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{SSMSTRMOPS,pfnIsOk}
 */
static DECLCALLBACK(int) ftmR3TcpOpIsOk(void *pvUser)
{
    PVM pVM = (PVM)pvUser;

    if (pVM->fFaultTolerantMaster)
    {
        /* Poll for incoming NACKs and errors from the other side */
        int rc = RTTcpSelectOne(pVM->ftm.s.hSocket, 0);
        if (rc != VERR_TIMEOUT)
        {
            if (RT_SUCCESS(rc))
            {
                LogRel(("FTSync/TCP: Incoming data detect by IsOk, assuming it is a cancellation NACK.\n"));
                rc = VERR_SSM_CANCELLED;
            }
            else
                LogRel(("FTSync/TCP: RTTcpSelectOne -> %Rrc (IsOk).\n", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{SSMSTRMOPS,pfnClose}
 */
static DECLCALLBACK(int) ftmR3TcpOpClose(void *pvUser, bool fCancelled)
{
    PVM pVM = (PVM)pvUser;

    if (pVM->fFaultTolerantMaster)
    {
        FTMTCPHDR EofHdr;
        EofHdr.u32Magic = FTMTCPHDR_MAGIC;
        EofHdr.cb       = fCancelled ? UINT32_MAX : 0;
        int rc = RTTcpWrite(pVM->ftm.s.hSocket, &EofHdr, sizeof(EofHdr));
        if (RT_FAILURE(rc))
        {
            LogRel(("FTSync/TCP: EOF Header write error: %Rrc\n", rc));
            return rc;
        }
    }
    else
    {
        ASMAtomicWriteBool(&pVM->ftm.s.syncstate.fStopReading, true);
    }

    return VINF_SUCCESS;
}


/**
 * Method table for a TCP based stream.
 */
static SSMSTRMOPS const g_ftmR3TcpOps =
{
    SSMSTRMOPS_VERSION,
    ftmR3TcpOpWrite,
    ftmR3TcpOpRead,
    ftmR3TcpOpSeek,
    ftmR3TcpOpTell,
    ftmR3TcpOpSize,
    ftmR3TcpOpIsOk,
    ftmR3TcpOpClose,
    SSMSTRMOPS_VERSION
};


/**
 * VMR3ReqCallWait callback
 *
 * @param   pVM         The cross context VM structure.
 *
 */
static DECLCALLBACK(void) ftmR3WriteProtectMemory(PVM pVM)
{
    int rc = PGMR3PhysWriteProtectRAM(pVM);
    AssertRC(rc);
}


/**
 * Sync the VM state
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
static int ftmR3PerformFullSync(PVM pVM)
{
    bool fSuspended = false;

    int rc = VMR3Suspend(pVM->pUVM, VMSUSPENDREASON_FTM_SYNC);
    AssertRCReturn(rc, rc);

    STAM_REL_COUNTER_INC(&pVM->ftm.s.StatFullSync);

    RTSocketRetain(pVM->ftm.s.hSocket); /* For concurrent access by I/O thread and EMT. */

    /* Reset the sync state. */
    pVM->ftm.s.syncstate.uOffStream   = 0;
    pVM->ftm.s.syncstate.cbReadBlock  = 0;
    pVM->ftm.s.syncstate.fStopReading = false;
    pVM->ftm.s.syncstate.fIOError     = false;
    pVM->ftm.s.syncstate.fEndOfStream = false;

    rc = ftmR3TcpSubmitCommand(pVM, "full-sync");
    AssertRC(rc);

    pVM->ftm.s.fDeltaLoadSaveActive = false;
    rc = VMR3SaveFT(pVM->pUVM, &g_ftmR3TcpOps, pVM, &fSuspended, false /* fSkipStateChanges */);
    AssertRC(rc);

    rc = ftmR3TcpReadACK(pVM, "full-sync-complete");
    AssertRC(rc);

    RTSocketRelease(pVM->ftm.s.hSocket);

    /* Write protect all memory. */
    rc = VMR3ReqCallWait(pVM, VMCPUID_ANY, (PFNRT)ftmR3WriteProtectMemory, 1, pVM);
    AssertRCReturn(rc, rc);

    rc = VMR3Resume(pVM->pUVM, VMRESUMEREASON_FTM_SYNC);
    AssertRC(rc);

    return rc;
}


/**
 * PGMR3PhysEnumDirtyFTPages callback for syncing dirty physical pages
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          GC physical address
 * @param   pRange          HC virtual address of the page(s)
 * @param   cbRange         Size of the dirty range in bytes.
 * @param   pvUser          User argument
 */
static DECLCALLBACK(int) ftmR3SyncDirtyPage(PVM pVM, RTGCPHYS GCPhys, uint8_t *pRange, unsigned cbRange, void *pvUser)
{
    NOREF(pvUser);
    FTMTCPHDRMEM Hdr;
    Hdr.u32Magic    = FTMTCPHDR_MAGIC;
    Hdr.GCPhys      = GCPhys;
    Hdr.cbPageRange = cbRange;
    Hdr.cb          = cbRange;
    /** @todo compress page(s). */
    int rc = RTTcpSgWriteL(pVM->ftm.s.hSocket, 2, &Hdr, sizeof(Hdr), pRange, (size_t)Hdr.cb);
    if (RT_FAILURE(rc))
    {
        LogRel(("FTSync/TCP: Write error (ftmR3SyncDirtyPage): %Rrc (cb=%#x)\n", rc, Hdr.cb));
        return rc;
    }
    pVM->ftm.s.StatSentMem.c += Hdr.cb + sizeof(Hdr);

#ifdef VBOX_WITH_STATISTICS
    switch (PGMPhysGetPageType(pVM, GCPhys))
    {
    case PGMPAGETYPE_RAM:
        pVM->ftm.s.StatSentMemRAM.c    += Hdr.cb + sizeof(Hdr);
        break;

    case PGMPAGETYPE_MMIO2:
        pVM->ftm.s.StatSentMemMMIO2.c  += Hdr.cb + sizeof(Hdr);
        break;

    case PGMPAGETYPE_ROM_SHADOW:
        pVM->ftm.s.StatSentMemShwROM.c += Hdr.cb + sizeof(Hdr);
        break;

    case PGMPAGETYPE_MMIO2_ALIAS_MMIO:
    case PGMPAGETYPE_SPECIAL_ALIAS_MMIO:
        AssertFailed();
        break;

    default:
        AssertFailed();
        break;
    }
#endif

    return (pVM->ftm.s.fCheckpointingActive) ? VERR_INTERRUPTED : VINF_SUCCESS;
}

/**
 * Thread function which starts syncing process for this master VM
 *
 * @param   hThread     The thread handle.
 * @param   pvUser      Pointer to the VM.
 * @return  VINF_SUCCESS (ignored).
 *
 */
static DECLCALLBACK(int) ftmR3MasterThread(RTTHREAD hThread, void *pvUser)
{
    int rc  = VINF_SUCCESS;
    PVM pVM = (PVM)pvUser;
    NOREF(hThread);

    for (;;)
    {
        /*
         * Try connect to the standby machine.
         */
        Log(("ftmR3MasterThread: client connect to %s %d\n", pVM->ftm.s.pszAddress, pVM->ftm.s.uPort));
        rc = RTTcpClientConnect(pVM->ftm.s.pszAddress, pVM->ftm.s.uPort, &pVM->ftm.s.hSocket);
        if (RT_SUCCESS(rc))
        {
            Log(("ftmR3MasterThread: CONNECTED\n"));

            /* Disable Nagle. */
            rc = RTTcpSetSendCoalescing(pVM->ftm.s.hSocket, false /*fEnable*/);
            AssertRC(rc);

            /* Read and check the welcome message. */
            char szLine[RT_MAX(128, sizeof(g_szWelcome))];
            RT_ZERO(szLine);
            rc = RTTcpRead(pVM->ftm.s.hSocket, szLine, sizeof(g_szWelcome) - 1, NULL);
            if (    RT_SUCCESS(rc)
                &&  !strcmp(szLine, g_szWelcome))
            {
                /* password */
                if (pVM->ftm.s.pszPassword)
                    rc = RTTcpWrite(pVM->ftm.s.hSocket, pVM->ftm.s.pszPassword, strlen(pVM->ftm.s.pszPassword));

                if (RT_SUCCESS(rc))
                {
                    /* ACK */
                    rc = ftmR3TcpReadACK(pVM, "password", "Invalid password");
                    if (RT_SUCCESS(rc))
                    {
                        /** @todo verify VM config. */
                        break;
                    }
                }
            }
            /* Failed, so don't bother anymore. */
            return VINF_SUCCESS;
        }
        rc = RTSemEventWait(pVM->ftm.s.hShutdownEvent, 1000 /* 1 second */);
        if (rc != VERR_TIMEOUT)
            return VINF_SUCCESS;    /* told to quit */
    }

    /* Successfully initialized the connection to the standby node.
     * Start the sync process.
     */

    /* First sync all memory and write protect everything so
     * we can send changed pages later on.
     */

    rc = ftmR3PerformFullSync(pVM);

    for (;;)
    {
        rc = RTSemEventWait(pVM->ftm.s.hShutdownEvent, pVM->ftm.s.uInterval);
        if (rc != VERR_TIMEOUT)
            break;    /* told to quit */

        if (!pVM->ftm.s.fCheckpointingActive)
        {
            rc = PDMCritSectEnter(&pVM->ftm.s.CritSect, VERR_SEM_BUSY);
            AssertMsg(rc == VINF_SUCCESS, ("%Rrc\n", rc));

            rc = ftmR3TcpSubmitCommand(pVM, "mem-sync");
            AssertRC(rc);

            /* sync the changed memory with the standby node. */
            /* Write protect all memory. */
            if (!pVM->ftm.s.fCheckpointingActive)
            {
                rc = VMR3ReqCallWait(pVM, VMCPUID_ANY, (PFNRT)ftmR3WriteProtectMemory, 1, pVM);
                AssertRC(rc);
            }

            /* Enumerate all dirty pages and send them to the standby VM. */
            if (!pVM->ftm.s.fCheckpointingActive)
            {
                rc = PGMR3PhysEnumDirtyFTPages(pVM, ftmR3SyncDirtyPage, NULL /* pvUser */);
                Assert(rc == VINF_SUCCESS || rc == VERR_INTERRUPTED);
            }

            /* Send last memory header to signal the end. */
            FTMTCPHDRMEM Hdr;
            Hdr.u32Magic    = FTMTCPHDR_MAGIC;
            Hdr.GCPhys      = 0;
            Hdr.cbPageRange = 0;
            Hdr.cb          = 0;
            rc = RTTcpSgWriteL(pVM->ftm.s.hSocket, 1, &Hdr, sizeof(Hdr));
            if (RT_FAILURE(rc))
                LogRel(("FTSync/TCP: Write error (ftmR3MasterThread): %Rrc (cb=%#x)\n", rc, Hdr.cb));

            rc = ftmR3TcpReadACK(pVM, "mem-sync-complete");
            AssertRC(rc);

            PDMCritSectLeave(&pVM->ftm.s.CritSect);
        }
    }
    return rc;
}

/**
 * Syncs memory from the master VM
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 */
static int ftmR3SyncMem(PVM pVM)
{
    while (true)
    {
        FTMTCPHDRMEM Hdr;
        RTGCPHYS GCPhys;

        /* Read memory header. */
        int rc = RTTcpRead(pVM->ftm.s.hSocket, &Hdr, sizeof(Hdr), NULL);
        if (RT_FAILURE(rc))
        {
            Log(("RTTcpRead failed with %Rrc\n", rc));
            break;
        }
        pVM->ftm.s.StatReceivedMem.c += sizeof(Hdr);

        if (Hdr.cb == 0)
            break;  /* end of sync. */

        Assert(Hdr.cb == Hdr.cbPageRange);  /** @todo uncompress */
        GCPhys = Hdr.GCPhys;

        /* Must be a multiple of PAGE_SIZE. */
        Assert((Hdr.cbPageRange & 0xfff) == 0);

        while (Hdr.cbPageRange)
        {
            PFTMPHYSPAGETREENODE pNode = (PFTMPHYSPAGETREENODE)RTAvlGCPhysGet(&pVM->ftm.s.standby.pPhysPageTree, GCPhys);
            if (!pNode)
            {
                /* Allocate memory for the node and page. */
                pNode = (PFTMPHYSPAGETREENODE)RTMemAllocZ(sizeof(*pNode) + PAGE_SIZE);
                AssertBreak(pNode);

                /* Insert the node into the tree. */
                pNode->Core.Key = GCPhys;
                pNode->pPage = (void *)(pNode + 1);
                bool fRet = RTAvlGCPhysInsert(&pVM->ftm.s.standby.pPhysPageTree, &pNode->Core);
                Assert(fRet); NOREF(fRet);
            }

            /* Fetch the page. */
            rc = RTTcpRead(pVM->ftm.s.hSocket, pNode->pPage, PAGE_SIZE, NULL);
            if (RT_FAILURE(rc))
            {
                Log(("RTTcpRead page data (%d bytes) failed with %Rrc\n", Hdr.cb, rc));
                break;
            }
            pVM->ftm.s.StatReceivedMem.c += PAGE_SIZE;
            Hdr.cbPageRange              -= PAGE_SIZE;
            GCPhys                       += PAGE_SIZE;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Callback handler for RTAvlGCPhysDestroy
 *
 * @returns 0 to continue, otherwise stop
 * @param   pBaseNode       Node to destroy
 * @param   pvUser          Pointer to the VM.
 */
static DECLCALLBACK(int) ftmR3PageTreeDestroyCallback(PAVLGCPHYSNODECORE pBaseNode, void *pvUser)
{
    PVM pVM = (PVM)pvUser;
    PFTMPHYSPAGETREENODE pNode = (PFTMPHYSPAGETREENODE)pBaseNode;

    if (pVM)    /* NULL when the VM is destroyed. */
    {
        /* Update the guest memory of the standby VM. */
        int rc = PGMR3PhysWriteExternal(pVM, pNode->Core.Key, pNode->pPage, PAGE_SIZE, PGMACCESSORIGIN_FTM);
        AssertRC(rc);
    }
    RTMemFree(pNode);
    return 0;
}

/**
 * Thread function which monitors the health of the master VM
 *
 * @param   hThread     The thread handle.
 * @param   pvUser      Pointer to the VM.
 * @return  VINF_SUCCESS (ignored).
 *
 */
static DECLCALLBACK(int) ftmR3StandbyThread(RTTHREAD hThread, void *pvUser)
{
    PVM pVM = (PVM)pvUser;
    NOREF(hThread);

    for (;;)
    {
        uint64_t u64TimeNow;

        int rc = RTSemEventWait(pVM->ftm.s.hShutdownEvent, pVM->ftm.s.uInterval);
        if (rc != VERR_TIMEOUT)
            break;    /* told to quit */

        if (pVM->ftm.s.standby.u64LastHeartbeat)
        {
            u64TimeNow = RTTimeMilliTS();

            if (u64TimeNow > pVM->ftm.s.standby.u64LastHeartbeat + pVM->ftm.s.uInterval * 4)
            {
                /* Timeout; prepare to fallover. */
                LogRel(("FTSync: TIMEOUT (%RX64 vs %RX64 ms): activate standby VM!\n", u64TimeNow, pVM->ftm.s.standby.u64LastHeartbeat + pVM->ftm.s.uInterval * 2));

                pVM->ftm.s.fActivateStandby = true;
                /** @todo prevent split-brain. */
                break;
            }
        }
    }

    return VINF_SUCCESS;
}


/**
 * Listen for incoming traffic destined for the standby VM.
 *
 * @copydoc FNRTTCPSERVE
 *
 * @returns VINF_SUCCESS or VERR_TCP_SERVER_STOP.
 */
static DECLCALLBACK(int) ftmR3StandbyServeConnection(RTSOCKET hSocket, void *pvUser)
{
    PVM pVM = (PVM)pvUser;

    pVM->ftm.s.hSocket = hSocket;

    /*
     * Disable Nagle.
     */
    int rc = RTTcpSetSendCoalescing(hSocket, false /*fEnable*/);
    AssertRC(rc);

    /* Send the welcome message to the master node. */
    rc = RTTcpWrite(hSocket, g_szWelcome, sizeof(g_szWelcome) - 1);
    if (RT_FAILURE(rc))
    {
        LogRel(("Teleporter: Failed to write welcome message: %Rrc\n", rc));
        return VINF_SUCCESS;
    }

    /*
     * Password.
     */
    const char *pszPassword = pVM->ftm.s.pszPassword;
    if (pszPassword)
    {
        unsigned    off = 0;
        while (pszPassword[off])
        {
            char ch;
            rc = RTTcpRead(hSocket, &ch, sizeof(ch), NULL);
            if (    RT_FAILURE(rc)
                ||  pszPassword[off] != ch)
            {
                if (RT_FAILURE(rc))
                    LogRel(("FTSync: Password read failure (off=%u): %Rrc\n", off, rc));
                else
                    LogRel(("FTSync: Invalid password (off=%u)\n", off));
                ftmR3TcpWriteNACK(pVM, VERR_AUTHENTICATION_FAILURE);
                return VINF_SUCCESS;
            }
            off++;
        }
    }
    rc = ftmR3TcpWriteACK(pVM);
    if (RT_FAILURE(rc))
        return VINF_SUCCESS;

    /** @todo verify VM config. */

    /*
     * Stop the server.
     *
     * Note! After this point we must return VERR_TCP_SERVER_STOP, while prior
     *       to it we must not return that value!
     */
    RTTcpServerShutdown(pVM->ftm.s.standby.hServer);

    /*
     * Command processing loop.
     */
    //bool fDone = false;
    for (;;)
    {
        bool fFullSync = false;
        char szCmd[128];

        rc = ftmR3TcpReadLine(pVM, szCmd, sizeof(szCmd));
        if (RT_FAILURE(rc))
            break;

        pVM->ftm.s.standby.u64LastHeartbeat = RTTimeMilliTS();
        if (!strcmp(szCmd, "mem-sync"))
        {
            rc = ftmR3TcpWriteACK(pVM);
            AssertRC(rc);
            if (RT_FAILURE(rc))
                continue;

            rc = ftmR3SyncMem(pVM);
            AssertRC(rc);

            rc = ftmR3TcpWriteACK(pVM);
            AssertRC(rc);
        }
        else
        if (    !strcmp(szCmd, "checkpoint")
            ||  !strcmp(szCmd, "full-sync")
            ||  (fFullSync = true))  /* intended assignment */
        {
            rc = ftmR3TcpWriteACK(pVM);
            AssertRC(rc);
            if (RT_FAILURE(rc))
                continue;

            /* Flush all pending memory updates. */
            if (pVM->ftm.s.standby.pPhysPageTree)
            {
                RTAvlGCPhysDestroy(&pVM->ftm.s.standby.pPhysPageTree, ftmR3PageTreeDestroyCallback, pVM);
                pVM->ftm.s.standby.pPhysPageTree = NULL;
            }

            RTSocketRetain(pVM->ftm.s.hSocket); /* For concurrent access by I/O thread and EMT. */

            /* Reset the sync state. */
            pVM->ftm.s.syncstate.uOffStream   = 0;
            pVM->ftm.s.syncstate.cbReadBlock  = 0;
            pVM->ftm.s.syncstate.fStopReading = false;
            pVM->ftm.s.syncstate.fIOError     = false;
            pVM->ftm.s.syncstate.fEndOfStream = false;

            pVM->ftm.s.fDeltaLoadSaveActive = (fFullSync == false);
            rc = VMR3LoadFromStreamFT(pVM->pUVM, &g_ftmR3TcpOps, pVM);
            pVM->ftm.s.fDeltaLoadSaveActive = false;
            RTSocketRelease(pVM->ftm.s.hSocket);
            AssertRC(rc);
            if (RT_FAILURE(rc))
            {
                LogRel(("FTSync: VMR3LoadFromStream -> %Rrc\n", rc));
                ftmR3TcpWriteNACK(pVM, rc);
                continue;
            }

            /* The EOS might not have been read, make sure it is. */
            pVM->ftm.s.syncstate.fStopReading = false;
            size_t cbRead;
            rc = ftmR3TcpOpRead(pVM, pVM->ftm.s.syncstate.uOffStream, szCmd, 1, &cbRead);
            if (rc != VERR_EOF)
            {
                LogRel(("FTSync: Draining teleporterTcpOpRead -> %Rrc\n", rc));
                ftmR3TcpWriteNACK(pVM, rc);
                continue;
            }

            rc = ftmR3TcpWriteACK(pVM);
            AssertRC(rc);
        }
    }
    LogFlowFunc(("returns mRc=%Rrc\n", rc));
    return VERR_TCP_SERVER_STOP;
}

/**
 * Powers on the fault tolerant virtual machine.
 *
 * @returns VBox status code.
 *
 * @param   pUVM        The user mode VM handle.
 * @param   fMaster     FT master or standby
 * @param   uInterval   FT sync interval
 * @param   pszAddress  Standby VM address
 * @param   uPort       Standby VM port
 * @param   pszPassword FT password (NULL for none)
 *
 * @thread      Any thread.
 * @vmstate     Created
 * @vmstateto   PoweringOn+Running (master), PoweringOn+Running_FT (standby)
 */
VMMR3DECL(int) FTMR3PowerOn(PUVM pUVM, bool fMaster, unsigned uInterval,
                            const char *pszAddress, unsigned uPort, const char *pszPassword)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);

    VMSTATE enmVMState = VMR3GetState(pVM);
    AssertMsgReturn(enmVMState == VMSTATE_CREATED,
                    ("%s\n", VMR3GetStateName(enmVMState)),
                    VERR_INTERNAL_ERROR_4);
    AssertReturn(pszAddress, VERR_INVALID_PARAMETER);

    if (pVM->ftm.s.uInterval)
        pVM->ftm.s.uInterval    = uInterval;
    else
        pVM->ftm.s.uInterval    = 50;   /* standard sync interval of 50ms */

    pVM->ftm.s.uPort            = uPort;
    pVM->ftm.s.pszAddress       = RTStrDup(pszAddress);
    if (pszPassword)
        pVM->ftm.s.pszPassword  = RTStrDup(pszPassword);

    int rc = RTSemEventCreate(&pVM->ftm.s.hShutdownEvent);
    if (RT_FAILURE(rc))
        return rc;

    if (fMaster)
    {
        rc = RTThreadCreate(NULL, ftmR3MasterThread, pVM,
                            0, RTTHREADTYPE_IO /* higher than normal priority */, 0, "ftmMaster");
        if (RT_FAILURE(rc))
            return rc;

        pVM->fFaultTolerantMaster = true;
        if (PGMIsUsingLargePages(pVM))
        {
            /* Must disable large page usage as 2 MB pages are too big to write monitor. */
            LogRel(("FTSync: disabling large page usage.\n"));
            PGMSetLargePageUsage(pVM, false);
        }
        /** @todo might need to disable page fusion as well */

        return VMR3PowerOn(pVM->pUVM);
    }


    /* standby */
    rc = RTThreadCreate(NULL, ftmR3StandbyThread, pVM,
                        0, RTTHREADTYPE_DEFAULT, 0, "ftmStandby");
    if (RT_FAILURE(rc))
        return rc;

    rc = RTTcpServerCreateEx(pszAddress, uPort, &pVM->ftm.s.standby.hServer);
    if (RT_FAILURE(rc))
        return rc;
    pVM->ftm.s.fIsStandbyNode = true;

    rc = RTTcpServerListen(pVM->ftm.s.standby.hServer, ftmR3StandbyServeConnection, pVM);
    /** @todo deal with the exit code to check if we should activate this standby VM. */
    if (pVM->ftm.s.fActivateStandby)
    {
        /** @todo fallover. */
    }

    if (pVM->ftm.s.standby.hServer)
    {
        RTTcpServerDestroy(pVM->ftm.s.standby.hServer);
        pVM->ftm.s.standby.hServer = NULL;
    }
    if (rc == VERR_TCP_SERVER_SHUTDOWN)
        rc = VINF_SUCCESS;  /* ignore this error; the standby process was cancelled. */
    return rc;
}

/**
 * Powers off the fault tolerant virtual machine (standby).
 *
 * @returns VBox status code.
 *
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(int) FTMR3CancelStandby(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(!pVM->fFaultTolerantMaster, VERR_NOT_SUPPORTED);
    Assert(pVM->ftm.s.standby.hServer);

    return RTTcpServerShutdown(pVM->ftm.s.standby.hServer);
}

/**
 * Rendezvous callback used by FTMR3SetCheckpoint
 * Sync state + changed memory with the standby node.
 *
 * This is only called on one of the EMTs while the other ones are waiting for
 * it to complete this function.
 *
 * @returns VINF_SUCCESS (VBox strict status code).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT. Unused.
 * @param   pvUser      Not used.
 */
static DECLCALLBACK(VBOXSTRICTRC) ftmR3SetCheckpointRendezvous(PVM pVM, PVMCPU pVCpu, void *pvUser)
{
    int     rc         = VINF_SUCCESS;
    bool    fSuspended = false;
    NOREF(pVCpu);
    NOREF(pvUser);

    /* We don't call VMR3Suspend here to avoid the overhead of state changes and notifications. This
     * is only a short suspend.
     */
    STAM_PROFILE_START(&pVM->ftm.s.StatCheckpointPause, a);
    PDMR3Suspend(pVM);

    /* Hack alert: as EM is responsible for dealing with the suspend state. We must do this here ourselves, but only for this EMT.*/
    EMR3NotifySuspend(pVM);
    STAM_PROFILE_STOP(&pVM->ftm.s.StatCheckpointPause, a);

    STAM_REL_COUNTER_INC(&pVM->ftm.s.StatDeltaVM);

    RTSocketRetain(pVM->ftm.s.hSocket); /* For concurrent access by I/O thread and EMT. */

    /* Reset the sync state. */
    pVM->ftm.s.syncstate.uOffStream   = 0;
    pVM->ftm.s.syncstate.cbReadBlock  = 0;
    pVM->ftm.s.syncstate.fStopReading = false;
    pVM->ftm.s.syncstate.fIOError     = false;
    pVM->ftm.s.syncstate.fEndOfStream = false;

    rc = ftmR3TcpSubmitCommand(pVM, "checkpoint");
    AssertRC(rc);

    pVM->ftm.s.fDeltaLoadSaveActive = true;
    rc = VMR3SaveFT(pVM->pUVM, &g_ftmR3TcpOps, pVM, &fSuspended, true /* fSkipStateChanges */);
    pVM->ftm.s.fDeltaLoadSaveActive = false;
    AssertRC(rc);

    rc = ftmR3TcpReadACK(pVM, "checkpoint-complete");
    AssertRC(rc);

    RTSocketRelease(pVM->ftm.s.hSocket);

    /* Write protect all memory. */
    rc = PGMR3PhysWriteProtectRAM(pVM);
    AssertRC(rc);

    /* We don't call VMR3Resume here to avoid the overhead of state changes and notifications. This
     * is only a short suspend.
     */
    STAM_PROFILE_START(&pVM->ftm.s.StatCheckpointResume, b);
    PGMR3ResetNoMorePhysWritesFlag(pVM);
    PDMR3Resume(pVM);

    /* Hack alert as EM is responsible for dealing with the suspend state. We must do this here ourselves, but only for this EMT.*/
    EMR3NotifyResume(pVM);
    STAM_PROFILE_STOP(&pVM->ftm.s.StatCheckpointResume, b);

    return rc;
}

/**
 * Performs a full sync to the standby node
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   enmCheckpoint   Checkpoint type
 */
VMMR3_INT_DECL(int) FTMR3SetCheckpoint(PVM pVM, FTMCHECKPOINTTYPE enmCheckpoint)
{
    int rc;

    if (!pVM->fFaultTolerantMaster)
        return VINF_SUCCESS;

    switch (enmCheckpoint)
    {
        case FTMCHECKPOINTTYPE_NETWORK:
            STAM_REL_COUNTER_INC(&pVM->ftm.s.StatCheckpointNetwork);
            break;

        case FTMCHECKPOINTTYPE_STORAGE:
            STAM_REL_COUNTER_INC(&pVM->ftm.s.StatCheckpointStorage);
            break;

        default:
            AssertMsgFailedReturn(("%d\n", enmCheckpoint), VERR_INVALID_PARAMETER);
    }

    pVM->ftm.s.fCheckpointingActive = true;
    if (VM_IS_EMT(pVM))
    {
        PVMCPU pVCpu = VMMGetCpu(pVM);

        /* We must take special care here as the memory sync is competing with us and requires a responsive EMT. */
        while ((rc = PDMCritSectTryEnter(&pVM->ftm.s.CritSect)) == VERR_SEM_BUSY)
        {
            if (VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS))
            {
                rc = VMMR3EmtRendezvousFF(pVM, pVCpu);
                AssertRC(rc);
            }

            if (VM_FF_IS_PENDING(pVM, VM_FF_REQUEST))
            {
                rc = VMR3ReqProcessU(pVM->pUVM, VMCPUID_ANY, true /*fPriorityOnly*/);
                AssertRC(rc);
            }
        }
    }
    else
        rc = PDMCritSectEnter(&pVM->ftm.s.CritSect, VERR_SEM_BUSY);

    AssertMsg(rc == VINF_SUCCESS, ("%Rrc\n", rc));

    STAM_PROFILE_START(&pVM->ftm.s.StatCheckpoint, a);

    rc = VMMR3EmtRendezvous(pVM, VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE, ftmR3SetCheckpointRendezvous, NULL);

    STAM_PROFILE_STOP(&pVM->ftm.s.StatCheckpoint, a);

    PDMCritSectLeave(&pVM->ftm.s.CritSect);
    pVM->ftm.s.fCheckpointingActive = false;

    return rc;
}
