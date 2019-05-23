/* $Id: DBGCTcp.cpp $ */
/** @file
 * DBGC - Debugger Console, TCP backend.
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
#include <VBox/dbg.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/err.h>

#include <iprt/thread.h>
#include <iprt/tcp.h>
#include <VBox/log.h>
#include <iprt/assert.h>

#include <iprt/string.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Debug console TCP backend instance data.
 */
typedef struct DBGCTCP
{
    /** The I/O backend for the console. */
    DBGCBACK    Back;
    /** The socket of the connection. */
    RTSOCKET    Sock;
    /** Connection status. */
    bool        fAlive;
} DBGCTCP;
/** Pointer to the instance data of the console TCP backend. */
typedef DBGCTCP *PDBGCTCP;

/** Converts a pointer to DBGCTCP::Back to a pointer to DBGCTCP. */
#define DBGCTCP_BACK2DBGCTCP(pBack) ( (PDBGCTCP)((char *)pBack - RT_OFFSETOF(DBGCTCP, Back)) )


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(int)  dbgcTcpConnection(RTSOCKET Sock, void *pvUser);



/**
 * Checks if there is input.
 *
 * @returns true if there is input ready.
 * @returns false if there not input ready.
 * @param   pBack       Pointer to the backend structure supplied by
 *                      the backend. The backend can use this to find
 *                      it's instance data.
 * @param   cMillies    Number of milliseconds to wait on input data.
 */
static DECLCALLBACK(bool) dbgcTcpBackInput(PDBGCBACK pBack, uint32_t cMillies)
{
    PDBGCTCP pDbgcTcp = DBGCTCP_BACK2DBGCTCP(pBack);
    if (!pDbgcTcp->fAlive)
        return false;
    int rc = RTTcpSelectOne(pDbgcTcp->Sock, cMillies);
    if (RT_FAILURE(rc) && rc != VERR_TIMEOUT)
        pDbgcTcp->fAlive = false;
    return rc != VERR_TIMEOUT;
}


/**
 * Read input.
 *
 * @returns VBox status code.
 * @param   pBack       Pointer to the backend structure supplied by
 *                      the backend. The backend can use this to find
 *                      it's instance data.
 * @param   pvBuf       Where to put the bytes we read.
 * @param   cbBuf       Maximum nymber of bytes to read.
 * @param   pcbRead     Where to store the number of bytes actually read.
 *                      If NULL the entire buffer must be filled for a
 *                      successful return.
 */
static DECLCALLBACK(int) dbgcTcpBackRead(PDBGCBACK pBack, void *pvBuf, size_t cbBuf, size_t *pcbRead)
{
    PDBGCTCP pDbgcTcp = DBGCTCP_BACK2DBGCTCP(pBack);
    if (!pDbgcTcp->fAlive)
        return VERR_INVALID_HANDLE;
    int rc = RTTcpRead(pDbgcTcp->Sock, pvBuf, cbBuf, pcbRead);
    if (RT_SUCCESS(rc) && pcbRead != NULL && *pcbRead == 0)
        rc = VERR_NET_SHUTDOWN;
    if (RT_FAILURE(rc))
        pDbgcTcp->fAlive = false;
    return rc;
}

/**
 * Write (output).
 *
 * @returns VBox status code.
 * @param   pBack       Pointer to the backend structure supplied by
 *                      the backend. The backend can use this to find
 *                      it's instance data.
 * @param   pvBuf       What to write.
 * @param   cbBuf       Number of bytes to write.
 * @param   pcbWritten  Where to store the number of bytes actually written.
 *                      If NULL the entire buffer must be successfully written.
 */
static DECLCALLBACK(int) dbgcTcpBackWrite(PDBGCBACK pBack, const void *pvBuf, size_t cbBuf, size_t *pcbWritten)
{
    PDBGCTCP pDbgcTcp = DBGCTCP_BACK2DBGCTCP(pBack);
    if (!pDbgcTcp->fAlive)
        return VERR_INVALID_HANDLE;

    /*
     * convert '\n' to '\r\n' while writing.
     */
    int     rc = 0;
    size_t  cbLeft = cbBuf;
    while (cbLeft)
    {
        size_t  cb = cbLeft;
        /* write newlines */
        if (*(const char *)pvBuf == '\n')
        {
            rc = RTTcpWrite(pDbgcTcp->Sock, "\r\n", 2);
            cb = 1;
        }
        /* write till next newline */
        else
        {
            const char *pszNL = (const char *)memchr(pvBuf, '\n', cbLeft);
            if (pszNL)
                cb = (uintptr_t)pszNL - (uintptr_t)pvBuf;
            rc = RTTcpWrite(pDbgcTcp->Sock, pvBuf, cb);
        }
        if (RT_FAILURE(rc))
        {
            pDbgcTcp->fAlive = false;
            break;
        }

        /* advance */
        cbLeft -= cb;
        pvBuf = (const char *)pvBuf + cb;
    }

    /*
     * Set returned value and return.
     */
    if (pcbWritten)
        *pcbWritten = cbBuf - cbLeft;
    return rc;
}

/** @copydoc FNDBGCBACKSETREADY */
static DECLCALLBACK(void) dbgcTcpBackSetReady(PDBGCBACK pBack, bool fReady)
{
    /* stub */
    NOREF(pBack);
    NOREF(fReady);
}


/**
 * Serve a TCP Server connection.
 *
 * @returns VBox status code.
 * @returns VERR_TCP_SERVER_STOP to terminate the server loop forcing
 *          the RTTcpCreateServer() call to return.
 * @param   Sock        The socket which the client is connected to.
 *                      The call will close this socket.
 * @param   pvUser      The VM handle.
 */
static DECLCALLBACK(int) dbgcTcpConnection(RTSOCKET Sock, void *pvUser)
{
    LogFlow(("dbgcTcpConnection: connection! Sock=%d pvUser=%p\n", Sock, pvUser));

    /*
     * Start the console.
     */
    DBGCTCP    DbgcTcp;
    DbgcTcp.Back.pfnInput    = dbgcTcpBackInput;
    DbgcTcp.Back.pfnRead     = dbgcTcpBackRead;
    DbgcTcp.Back.pfnWrite    = dbgcTcpBackWrite;
    DbgcTcp.Back.pfnSetReady = dbgcTcpBackSetReady;
    DbgcTcp.fAlive = true;
    DbgcTcp.Sock   = Sock;
    int rc = DBGCCreate((PUVM)pvUser, &DbgcTcp.Back, 0);
    LogFlow(("dbgcTcpConnection: disconnect rc=%Rrc\n", rc));
    return rc;
}


/**
 * Spawns a new thread with a TCP based debugging console service.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   ppvData     Where to store a pointer to the instance data.
 */
DBGDECL(int)    DBGCTcpCreate(PUVM pUVM, void **ppvData)
{
    /*
     * Check what the configuration says.
     */
    PCFGMNODE pKey = CFGMR3GetChild(CFGMR3GetRootU(pUVM), "DBGC");
    bool fEnabled;
    int rc = CFGMR3QueryBoolDef(pKey, "Enabled", &fEnabled,
#if defined(VBOX_WITH_DEBUGGER) && defined(VBOX_WITH_DEBUGGER_TCP_BY_DEFAULT) && !defined(__L4ENV__) && !defined(DEBUG_dmik)
        true
#else
        false
#endif
        );
    if (RT_FAILURE(rc))
        return VM_SET_ERROR_U(pUVM, rc, "Configuration error: Failed querying \"DBGC/Enabled\"");

    if (!fEnabled)
    {
        LogFlow(("DBGCTcpCreate: returns VINF_SUCCESS (Disabled)\n"));
        return VINF_SUCCESS;
    }

    /*
     * Get the port configuration.
     */
    uint32_t u32Port;
    rc = CFGMR3QueryU32Def(pKey, "Port", &u32Port, 5000);
    if (RT_FAILURE(rc))
        return VM_SET_ERROR_U(pUVM, rc, "Configuration error: Failed querying \"DBGC/Port\"");

    /*
     * Get the address configuration.
     */
    char szAddress[512];
    rc = CFGMR3QueryStringDef(pKey, "Address", szAddress, sizeof(szAddress), "");
    if (RT_FAILURE(rc))
        return VM_SET_ERROR_U(pUVM, rc, "Configuration error: Failed querying \"DBGC/Address\"");

    /*
     * Create the server (separate thread).
     */
    PRTTCPSERVER pServer;
    rc = RTTcpServerCreate(szAddress, u32Port, RTTHREADTYPE_DEBUGGER, "DBGC", dbgcTcpConnection, pUVM, &pServer);
    if (RT_SUCCESS(rc))
    {
        LogFlow(("DBGCTcpCreate: Created server on port %d %s\n", u32Port, szAddress));
        *ppvData = pServer;
        return rc;
    }

    LogFlow(("DBGCTcpCreate: returns %Rrc\n", rc));
    return VM_SET_ERROR_U(pUVM, rc, "Cannot start TCP-based debugging console service");
}


/**
 * Terminates any running TCP base debugger console service.
 *
 * @returns VBox status code.
 * @param   pUVM            The user mode VM handle.
 * @param   pvData          The data returned by DBGCTcpCreate.
 */
DBGDECL(int) DBGCTcpTerminate(PUVM pUVM, void *pvData)
{
    RT_NOREF1(pUVM);

    /*
     * Destroy the server instance if any.
     */
    if (pvData)
    {
        int rc = RTTcpServerDestroy((PRTTCPSERVER)pvData);
        AssertRC(rc);
    }

    return VINF_SUCCESS;
}

