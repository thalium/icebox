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

/*MYCODE*/
#include <VBox/vmm/vm.h>
/*ENDMYCODE*/

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
#define DBGCTCP_BACK2DBGCTCP(pBack) ( (PDBGCTCP)((char *)(pBack) - RT_UOFFSETOF(DBGCTCP, Back)) )


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

/*MYCODE*/
#include <stdio.h>
#include <FDP/include/FDP.h>

#ifndef _MSC_VER
#include <FDP/FDP.cpp>
#else
#include <FDP/include/FDP_structs.h>
#endif

#include <VBox/vmm/pgm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/cpum.h>

#ifdef  __linux
#include <pthread.h>
#define SLEEP(X) (usleep(X*1000))

#elif   _WIN32
#include <Windows.h>

#define SLEEP(X) (Sleep(X))
#endif


#define MIN(a,b) (((a)<(b))?(a):(b))

#define DEBUG_LEVEL 0
#define DEBUG_FLOW 0

#if DEBUG_LEVEL > 0
// #define Log1(fmt,...) printf(fmt, ##__VA_ARGS__)
#define Log1(x) LogRel(x)
#else
#define Log1(x)
#endif

#if DEBUG_LEVEL > 2
#define Log3(x) LogRel(x)
#else
#define Log3(x)
#endif

#ifdef DEBUG_FLOW > 0
#define LogFloww() printf("%s\n", __FUNCTION__);
#else
#define LogFloww()
#endif

typedef struct _MEMORY_SSM_T{
    uint8_t        *pMemory;
    uint64_t    cbMemory;
    uint64_t    CurrentOffset;
    uint64_t    MaxOffset;
}MEMORY_SSM_T;

typedef struct FDPVBOX_USERHANDLE_T{
    PUVM            pUVM;
    MEMORY_SSM_T*    pMemorySSM;
    FDP_SHM*        pFDPServer;
    uint64_t        aVisibleGuestDebugRegisterSave[7];
    char            TempBuffer[1*1024*1024];
}FDPVBOX_USERHANDLE_T;

bool FDPVBOX_Resume(void *pUserHandle)
{
    Log2("RESUME\n");
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    VMR3Continue(myVBOXHandle->pUVM);
    return true;
}

bool FDPVBOX_Pause(void *pUserHandle)
{
    Log1(("[DBGC] PAUSE !\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    VMR3Break(myVBOXHandle->pUVM);
    return true;
}

bool FDPVBOX_singleStep(void *pUserHandle, uint32_t CpuId)
{
    Log2("SINGLE_STEP\n");
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);
    int rc = VMR3SingleStep(myVBOXHandle->pUVM, pVCpu);
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}

bool FDPVBOX_getMemorySize(void *pUserHandle, uint64_t* MemorySize)
{
    Log1(("[DBGC] GET_PHYSICALMEMORYSIZE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    *MemorySize = MMR3PhysGetRamSizeU(myVBOXHandle->pUVM);
    return true;
}

bool FDPVBOX_readPhysicalMemory(void *pUserHandle, uint8_t *pDstBuffer, uint64_t PhysicalAddress, uint32_t ReadSize)
{
    Log1(("[DBGC] READ_PHYSICAL %p %d ... ", PhysicalAddress, ReadSize));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    int rc = VMR3PhysSimpleReadGCPhysU(myVBOXHandle->pUVM, pDstBuffer, PhysicalAddress, ReadSize);
    Log1(("[DBGC]  %s\n", RT_SUCCESS(rc) ? "OK" : "KO"));
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}


bool FDPVBOX_writePhysicalMemory(void *pUserHandle, uint8_t *pSrcBuffer, uint64_t PhysicalAddress, uint32_t WriteSize)
{
    Log1(("[DBGC] WRITE_PHYSICAL %p %d...", PhysicalAddress, WriteSize));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;

    //Check Read access
    if(FDPVBOX_readPhysicalMemory(pUserHandle, (uint8_t*)myVBOXHandle->TempBuffer, PhysicalAddress, WriteSize) == false){
        return false;
    }
    //Effective Write
    int rc =  VMR3PhysSimpleWriteGCPhysU(myVBOXHandle->pUVM, pSrcBuffer, PhysicalAddress, WriteSize);
    Log1(("[DBGC]  %s\n", RT_SUCCESS(rc) ? "OK" : "KO"));
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}


bool FDPVBOX_writeVirtualMemory(void *pUserHandle, uint32_t CpuId, uint8_t *pSrcBuffer, uint64_t VirtualAddress, uint32_t WriteSize)
{
    Log1(("[DBGC] writeVirtualMemory %p %d ...", VirtualAddress, WriteSize));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);
    int rc = PGMPhysSimpleWriteGCPtr(pVCpu, VirtualAddress, pSrcBuffer, WriteSize);
    Log1(("[DBGC]  %s\n", RT_SUCCESS(rc) ? "OK" : "KO"));
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}

bool FDPVBOX_writeMsr(void *pUserHandle, uint32_t CpuId, uint64_t MSRId, uint64_t MSRValue)
{
    Log1(("[DBGC] WRITE_MSR %p %p\n", MSRId, MSRValue));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);
    int rc = CPUMSetGuestMsr(pVCpu, MSRId, MSRValue);
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}

bool FDPVBOX_getState(void *pUserHandle, uint8_t *currentState)
{
    Log3(("[DBGC] GET_STATE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    *currentState = VMR3GetFDPState(myVBOXHandle->pUVM);
    return true;
}

bool FDPVBOX_getCpuState(void *pUserHandle, uint32_t CpuId, uint8_t *pCurrentState)
{
    Log1(("[DBGC] GET_CPU_STATE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);
    *pCurrentState = pVCpu->mystate.s.u8StateBitmap;
    return true;
}


bool FDPVBOX_getCpuCount(void *pUserHandle, uint32_t *pCpuCount)
{
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    *pCpuCount = VMR3GetCPUCount(myVBOXHandle->pUVM);
    return true;
}

bool FDPVBOX_readMsr(void *pUserHandle, uint32_t CpuId, uint64_t MsrId, uint64_t *pMsrValue)
{
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    CPUMQueryGuestMsr(pVCpu, MsrId, pMsrValue);
    Log1(("[DBGC] READ_MSR %p => %p\n", MsrId, *pMsrValue));
    return true;
}

bool FDPVBOX_readRegister(void *pUserHandle, uint32_t CpuId, FDP_Register RegisterId, uint64_t *pRegisterValue)
{
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    PCCPUMCTXCORE pCtxCore = CPUMGetGuestCtxCore(pVCpu);
    PCPUMCTXCORE pRegFrame = (PCPUMCTXCORE)CPUMGetGuestCtxCore(pVCpu);

    switch(RegisterId){
        case FDP_CR0_REGISTER: *pRegisterValue = CPUMGetGuestCR0(pVCpu); break;
        case FDP_CR2_REGISTER: *pRegisterValue = CPUMGetGuestCR2(pVCpu); break;
        case FDP_CR3_REGISTER: *pRegisterValue = CPUMGetGuestCR3(pVCpu); break;
        case FDP_CR4_REGISTER: *pRegisterValue = CPUMGetGuestCR4(pVCpu); break;
        case FDP_CR8_REGISTER: *pRegisterValue = CPUMGetGuestCR8(pVCpu); break;
        case FDP_RAX_REGISTER: *pRegisterValue = pCtxCore->rax; break;
        case FDP_RBX_REGISTER: *pRegisterValue = pCtxCore->rbx; break;
        case FDP_RCX_REGISTER: *pRegisterValue = pCtxCore->rcx; break;
        case FDP_RDX_REGISTER: *pRegisterValue = pCtxCore->rdx; break;
        case FDP_R8_REGISTER:  *pRegisterValue = pCtxCore->r8; break;
        case FDP_R9_REGISTER:  *pRegisterValue = pCtxCore->r9; break;
        case FDP_R10_REGISTER: *pRegisterValue = pCtxCore->r10; break;
        case FDP_R11_REGISTER: *pRegisterValue = pCtxCore->r11; break;
        case FDP_R12_REGISTER: *pRegisterValue = pCtxCore->r12; break;
        case FDP_R13_REGISTER: *pRegisterValue = pCtxCore->r13; break;
        case FDP_R14_REGISTER: *pRegisterValue = pCtxCore->r14; break;
        case FDP_R15_REGISTER: *pRegisterValue = pCtxCore->r15; break;
        case FDP_RSP_REGISTER: *pRegisterValue = pCtxCore->rsp; break;
        case FDP_RBP_REGISTER: *pRegisterValue = pCtxCore->rbp; break;
        case FDP_RSI_REGISTER: *pRegisterValue = pCtxCore->rsi; break;
        case FDP_RDI_REGISTER: *pRegisterValue = pCtxCore->rdi; break;
        case FDP_RIP_REGISTER: *pRegisterValue = pCtxCore->rip; break;

        //Visible for Guest Debug Register
        case FDP_VDR0_REGISTER: *pRegisterValue = pVCpu->mystate.s.aGuestDr[0]; break;
        case FDP_VDR1_REGISTER: *pRegisterValue = pVCpu->mystate.s.aGuestDr[1]; break;
        case FDP_VDR2_REGISTER: *pRegisterValue = pVCpu->mystate.s.aGuestDr[2]; break;
        case FDP_VDR3_REGISTER: *pRegisterValue = pVCpu->mystate.s.aGuestDr[3]; break;
        case FDP_VDR6_REGISTER: *pRegisterValue = pVCpu->mystate.s.aGuestDr[6]; break;
        case FDP_VDR7_REGISTER: *pRegisterValue = pVCpu->mystate.s.aGuestDr[7]; break;

        //Invisible for Guest Debug Register
        case FDP_DR0_REGISTER: *pRegisterValue = CPUMGetGuestDR0(pVCpu); break;
        case FDP_DR1_REGISTER: *pRegisterValue = CPUMGetGuestDR1(pVCpu); break;
        case FDP_DR2_REGISTER: *pRegisterValue = CPUMGetGuestDR2(pVCpu); break;
        case FDP_DR3_REGISTER: *pRegisterValue = CPUMGetGuestDR3(pVCpu); break;
        case FDP_DR6_REGISTER: *pRegisterValue = CPUMGetGuestDR6(pVCpu); break;
        case FDP_DR7_REGISTER: *pRegisterValue = CPUMGetGuestDR7(pVCpu); break;

        case FDP_CS_REGISTER: *pRegisterValue = CPUMGetGuestCS(pVCpu); break;
        case FDP_DS_REGISTER: *pRegisterValue = CPUMGetGuestDS(pVCpu); break;
        case FDP_ES_REGISTER: *pRegisterValue = CPUMGetGuestES(pVCpu); break;
        case FDP_FS_REGISTER: *pRegisterValue = CPUMGetGuestFS(pVCpu); break;
        case FDP_GS_REGISTER: *pRegisterValue = CPUMGetGuestGS(pVCpu); break;
        case FDP_SS_REGISTER: *pRegisterValue = CPUMGetGuestSS(pVCpu); break;
        case FDP_RFLAGS_REGISTER: *pRegisterValue = CPUMGetGuestEFlags(pVCpu); break;
        case FDP_GDTRB_REGISTER:
            {
                VBOXGDTR gdtr = {0, 0};
                CPUMGetGuestGDTR(pVCpu,&gdtr);
                *pRegisterValue = gdtr.pGdt;
                break;
            }
        case FDP_GDTRL_REGISTER:
            {
                VBOXGDTR gdtr = {0, 0};
                CPUMGetGuestGDTR(pVCpu,&gdtr);
                *pRegisterValue = gdtr.cbGdt;
                break;
            }
        case FDP_IDTRB_REGISTER:
            {
                uint16_t cbIDT;
                RTGCPTR    GCPtrIDT = (RTGCPTR)CPUMGetGuestIDTR(pVCpu, &cbIDT);
                *pRegisterValue = GCPtrIDT;
                break;
            }
        case FDP_IDTRL_REGISTER:
            {
                uint16_t cbIDT;
                RTGCPTR    GCPtrIDT = (RTGCPTR)CPUMGetGuestIDTR(pVCpu, &cbIDT);
                *pRegisterValue = cbIDT;
                break;
            }
        case FDP_LDTR_REGISTER:
            {
                uint64_t Ldtrb;
                uint32_t Ldtrl;
                *pRegisterValue = CPUMGetGuestLdtrEx(pVCpu, &Ldtrb, &Ldtrl);
                break;
            }
        case FDP_LDTRB_REGISTER:
            {
                uint64_t Ldtrb;
                uint32_t Ldtrl;
                CPUMGetGuestLdtrEx(pVCpu, &Ldtrb, &Ldtrl);
                *pRegisterValue = Ldtrb;
                break;
            }
        case FDP_LDTRL_REGISTER:
            {
                uint64_t Ldtrb;
                uint32_t Ldtrl;
                CPUMGetGuestLdtrEx(pVCpu, &Ldtrb, &Ldtrl);
                *pRegisterValue = Ldtrl;
                break;
            }
        case FDP_TR_REGISTER:
            {
                *pRegisterValue = CPUMGetGuestTR(pVCpu, NULL);
                break;
            }
        default:
            {
                *pRegisterValue = 0xBADBADBADBADBADB;
                return false;
            }
    }
    return true;
}

bool FDPVBOX_writeRegister(void *pUserHandle, uint32_t CpuId, FDP_Register RegisterId, uint64_t RegisterValue)
{
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    PCCPUMCTXCORE pCtxCore = CPUMGetGuestCtxCore(pVCpu);
    PCPUMCTXCORE pRegFrame = (PCPUMCTXCORE)CPUMGetGuestCtxCore(pVCpu);

    FDP_CPU_CTX* pFdpCpuCtx = (FDP_CPU_CTX *)pVCpu->mystate.s.pCpuShm;

    switch(RegisterId){
        case FDP_RAX_REGISTER: pRegFrame->rax = RegisterValue; pFdpCpuCtx->rax = RegisterValue; break;
        case FDP_RBX_REGISTER: pRegFrame->rbx = RegisterValue; pFdpCpuCtx->rbx = RegisterValue; break;
        case FDP_RCX_REGISTER: pRegFrame->rcx = RegisterValue; pFdpCpuCtx->rcx = RegisterValue; break;
        case FDP_RDX_REGISTER: pRegFrame->rdx = RegisterValue; pFdpCpuCtx->rdx = RegisterValue; break;
        case FDP_R8_REGISTER:  pRegFrame->r8 = RegisterValue; pFdpCpuCtx->r8 = RegisterValue; break;
        case FDP_R9_REGISTER:  pRegFrame->r9 = RegisterValue; pFdpCpuCtx->r9 = RegisterValue; break;
        case FDP_R10_REGISTER: pRegFrame->r10 = RegisterValue; pFdpCpuCtx->r10 = RegisterValue; break;
        case FDP_R11_REGISTER: pRegFrame->r11 = RegisterValue; pFdpCpuCtx->r11 = RegisterValue; break;
        case FDP_R12_REGISTER: pRegFrame->r12 = RegisterValue; pFdpCpuCtx->r12 = RegisterValue; break;
        case FDP_R13_REGISTER: pRegFrame->r13 = RegisterValue; pFdpCpuCtx->r13 = RegisterValue; break;
        case FDP_R14_REGISTER: pRegFrame->r14 = RegisterValue; pFdpCpuCtx->r14 = RegisterValue; break;
        case FDP_R15_REGISTER: pRegFrame->r15 = RegisterValue; pFdpCpuCtx->r15 = RegisterValue; break;
        case FDP_RSP_REGISTER: pRegFrame->rsp = RegisterValue; pFdpCpuCtx->rsp = RegisterValue; break;
        case FDP_RBP_REGISTER: pRegFrame->rbp = RegisterValue; pFdpCpuCtx->rbp = RegisterValue; break;
        case FDP_RSI_REGISTER: pRegFrame->rsi = RegisterValue; pFdpCpuCtx->rsi = RegisterValue; break;
        case FDP_RDI_REGISTER: pRegFrame->rdi = RegisterValue; pFdpCpuCtx->rdi = RegisterValue; break;
        case FDP_RIP_REGISTER: pRegFrame->rip = RegisterValue; pFdpCpuCtx->rip = RegisterValue; break;

        //Invisible for Guest Debug Register
        case FDP_DR0_REGISTER: CPUMSetGuestDR0(pVCpu, RegisterValue); break;
        case FDP_DR1_REGISTER: CPUMSetGuestDR1(pVCpu, RegisterValue); break;
        case FDP_DR2_REGISTER: CPUMSetGuestDR2(pVCpu, RegisterValue); break;
        case FDP_DR3_REGISTER: CPUMSetGuestDR3(pVCpu, RegisterValue); break;
        case FDP_DR6_REGISTER: CPUMSetGuestDR6(pVCpu, RegisterValue); break;
        case FDP_DR7_REGISTER: CPUMSetGuestDR7(pVCpu, RegisterValue); break;

        //Visible for Guest Debug Register
        case FDP_VDR0_REGISTER: pVCpu->mystate.s.aGuestDr[0] = RegisterValue; break;
        case FDP_VDR1_REGISTER: pVCpu->mystate.s.aGuestDr[1] = RegisterValue; break;
        case FDP_VDR2_REGISTER: pVCpu->mystate.s.aGuestDr[2] = RegisterValue; break;
        case FDP_VDR3_REGISTER: pVCpu->mystate.s.aGuestDr[3] = RegisterValue; break;
        case FDP_VDR6_REGISTER: pVCpu->mystate.s.aGuestDr[6] = RegisterValue; break;
        case FDP_VDR7_REGISTER: pVCpu->mystate.s.aGuestDr[7] = RegisterValue; break;

        case FDP_CS_REGISTER: CPUMSetGuestCS(pVCpu, RegisterValue); break;
        case FDP_DS_REGISTER: CPUMSetGuestDS(pVCpu, RegisterValue); break;
        case FDP_ES_REGISTER: CPUMSetGuestES(pVCpu, RegisterValue); break;
        case FDP_FS_REGISTER: CPUMSetGuestFS(pVCpu, RegisterValue); break;
        case FDP_GS_REGISTER: CPUMSetGuestGS(pVCpu, RegisterValue); break;
        case FDP_SS_REGISTER: CPUMSetGuestSS(pVCpu, RegisterValue); break;
        case FDP_CR0_REGISTER: CPUMSetGuestCR0(pVCpu, RegisterValue); pFdpCpuCtx->cr0 = RegisterValue; break;
        case FDP_CR2_REGISTER: CPUMSetGuestCR2(pVCpu, RegisterValue); pFdpCpuCtx->cr3 = RegisterValue; break;
        case FDP_CR3_REGISTER:
        {
            CPUMSetGuestCR3(pVCpu, RegisterValue);
            PGMFlushTLB(pVCpu, RegisterValue, 0);
            pFdpCpuCtx->cr3 = RegisterValue;
            break;
        }
        case FDP_CR4_REGISTER: CPUMSetGuestCR4(pVCpu, RegisterValue); pFdpCpuCtx->cr4 = RegisterValue; break;
        //case FDP_CR8_REGISTER: CPUMSetGuestCR8(pVCpu, RegisterValue); break;
        case FDP_RFLAGS_REGISTER: CPUMSetGuestEFlags(pVCpu, RegisterValue); break;
        default: break;
    }
    return true;
}

bool FDPVBOX_virtualToPhysical(void *pUserHandle, uint32_t CpuId, uint64_t VirtualAddress, uint64_t *PhysicalAddress)
{
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);
    int rc = PGMPhysGCPtr2GCPhys(pVCpu, VirtualAddress, PhysicalAddress);
    if(RT_FAILURE(rc)){
        return false;
    }
    return true;
}

bool FDPVBOX_unsetBreakpoint(void *pUserHandle, int BreakpointId)
{
    Log1(("[DBGC] UNSET_BP [%d] ! \n", BreakpointId));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    int rc = VMR3RemoveBreakpoint(myVBOXHandle->pUVM, BreakpointId);
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}

bool FDPVBOX_getFxState64(void *pUserHandle, uint32_t CpuId, uint8_t *pDstBuffer, uint32_t *pDstSize)
{
    Log1(("[DBGC] GET_FXSTATE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    memcpy(pDstBuffer, pFpuCtx, sizeof(X86FXSTATE));
    *pDstSize = sizeof(X86FXSTATE);
    return true;
}

bool FDPVBOX_setFxState64(void *pUserHandle, uint32_t CpuId, uint8_t *pSrcBuffer, uint32_t uSrcSize)
{
    Log1(("[DBGC] SET_FXSTATE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    PX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
    memcpy(pFpuCtx, pSrcBuffer, sizeof(X86FXSTATE));
    return true;
}

bool FDPVBOX_readVirtualMemory(void *pUserHandle, uint32_t CpuId, uint64_t VirtualAddress, uint32_t ReadSize, uint8_t *pDstBuffer)
{
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return false;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    int rc = 0;
    rc = PGMPhysSimpleReadGCPtr(pVCpu, pDstBuffer, VirtualAddress, ReadSize);
    if(RT_SUCCESS(rc)){
        return true;
    }
    return false;
}

int FDPVBOX_setBreakpoint(
    void *pUserHandle,
    uint32_t CpuId,
    FDP_BreakpointType BreakpointType,
    int BreakpointId,
    FDP_Access BreakpointAccessType,
    FDP_AddressType BreakpointAddressType,
    uint64_t BreakpointAddress,
    uint64_t BreakpointLength,
    uint64_t BreakpointCr3)
{
    Log1(("[DBGC] SET_BREAKPOINT %p\n", BreakpointAddress));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    if(CpuId >= VMR3GetCPUCount(myVBOXHandle->pUVM)){
        return -1;
    }
    PVMCPU pVCpu = VMMR3GetCpuByIdU(myVBOXHandle->pUVM, CpuId);

    BreakpointId = -1;
    switch(BreakpointType){
        case FDP_SOFTHBP:
        {
            BreakpointId = VMR3AddSoftBreakpoint(myVBOXHandle->pUVM, pVCpu, BreakpointAddressType, BreakpointAddress, BreakpointCr3);
            Log1(("[DBGC] FDP_SOFTHBP[%d] %c %p %p\n", BreakpointId, BreakpointAddressType == 0x1 ? 'v' : 'p', BreakpointAddress, BreakpointCr3));
            break;
        }
        case FDP_PAGEHBP:
        {
            BreakpointId = VMR3AddPageBreakpoint(myVBOXHandle->pUVM, pVCpu, -1, BreakpointAccessType, BreakpointAddressType, BreakpointAddress, BreakpointLength);
            Log1(("[DBGC] FDP_PAGEHBP[%d] %02x %p\n", BreakpointId, BreakpointAccessType, BreakpointAddress));
            break;
        }
        case FDP_MSRHBP:
        {
            BreakpointId = VMR3AddMsrBreakpoint(myVBOXHandle->pUVM, BreakpointAccessType, BreakpointAddress);
            Log1(("[DBGC] FDP_MSRHBP[%d] %02x %p\n", BreakpointId, BreakpointAccessType, BreakpointAddress));
            break;
        }
        case FDP_CRHBP:
        {
            BreakpointId = VMR3AddCrBreakpoint(myVBOXHandle->pUVM, BreakpointAccessType, BreakpointAddress);
            Log1(("[DBGC] FDP_CRHBP[%d] %02x %p\n", BreakpointId, BreakpointAccessType, BreakpointAddress));
            break;
        }
        default:
        {
            Log1(("[DBGC] Unknown BreakpointType!\n"));
            break;
        }
    }

    return BreakpointId;
}


bool FDPVBOX_InjectInterrupt(void *pUserHandle, uint32_t CpuId, uint32_t InterruptionCode, uint32_t ErrorCode, uint64_t Cr2){
    Log1(("[DBGC] InjectInterrupt\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    PUVM pUVM = myVBOXHandle->pUVM;

    PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, 0);

    VMR3InjectInterrupt(NULL, pVCpu, InterruptionCode, ErrorCode, Cr2);
    return true;
}

bool FDPVBOX_Reboot(void *pUserHandle)
{
    Log1(("[DBGC] REBOOT\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    PUVM pUVM = myVBOXHandle->pUVM;

    PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, 0);

    FDPVBOX_Pause(pUserHandle);
    for(int BreakpointId = 0; BreakpointId < FDP_MAX_BREAKPOINT; BreakpointId++){
        FDPVBOX_unsetBreakpoint(pUserHandle, BreakpointId);
    }
    CPUMSetGuestDR7(pVCpu, 0);

    //Ask the EMT the Triple fault
    pVCpu->mystate.s.bRebootRequired = true;

    FDPVBOX_Resume(pUserHandle);

    //TODO: Wait for the startup
    SLEEP(100);

    //Signal that the VM as changed, and what a change...
    FDP_SetStateChanged(myVBOXHandle->pFDPServer);
    return true;
}



#include <VBox/vmm/ssm.h>
#include <iprt/file.h>



static DECLCALLBACK(int) nullProgressCallback(PUVM pUVM, unsigned uPercent, void *pvUser)
{
    NOREF(pUVM);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) pfnMemoryWrite(void *pvUser, uint64_t offStream, const void *pvBuf, size_t cbToWrite)
{
    MEMORY_SSM_T* pMemorySSM = (MEMORY_SSM_T*)pvUser;
    memcpy(pMemorySSM->pMemory+offStream, pvBuf, cbToWrite);
    pMemorySSM->CurrentOffset = offStream;
    if(offStream+cbToWrite > pMemorySSM->MaxOffset){
        pMemorySSM->MaxOffset = offStream+cbToWrite;
    }
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) pfnMemoryRead(void *pvUser, uint64_t offStream, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    MEMORY_SSM_T* pMemorySSM = (MEMORY_SSM_T*)pvUser;
    memcpy(pvBuf, pMemorySSM->pMemory+offStream, cbToRead);
    *pcbRead = cbToRead;
    pMemorySSM->CurrentOffset = offStream + cbToRead;
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) pfnMemorySeek(void *pvUser, int64_t offSeek, unsigned uMethod, uint64_t *poffActual)
{
    MEMORY_SSM_T* pMemorySSM = (MEMORY_SSM_T*)pvUser;
    if(uMethod == RTFILE_SEEK_BEGIN){
        pMemorySSM->CurrentOffset = offSeek;
    }
    if(uMethod == RTFILE_SEEK_END){
        pMemorySSM->CurrentOffset = pMemorySSM->cbMemory-offSeek;
    }
    if(uMethod == RTFILE_SEEK_CURRENT){
        pMemorySSM->CurrentOffset += offSeek;
    }
    *poffActual = pMemorySSM->CurrentOffset;
    if(*poffActual > pMemorySSM->MaxOffset){
        pMemorySSM->MaxOffset = *poffActual;
    }
    return VINF_SUCCESS;
}

static DECLCALLBACK(uint64_t) pfnMemoryTell(void *pvUser)
{
    MEMORY_SSM_T* pMemorySSM = (MEMORY_SSM_T*)pvUser;
    return pMemorySSM->CurrentOffset;
}

static DECLCALLBACK(int) pfnMemorySize(void *pvUser, uint64_t *pcb)
{
    MEMORY_SSM_T* pMemorySSM = (MEMORY_SSM_T*)pvUser;
    *pcb = pMemorySSM->MaxOffset;
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) pfnMemoryIsOk(void *pvUser){
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) pfnMemoryClose(void *pvUser, bool fCancelled)
{
    MEMORY_SSM_T* pMemorySSM = (MEMORY_SSM_T*)pvUser;
    pMemorySSM->CurrentOffset = 0;
    pMemorySSM->MaxOffset = 0;
    return VINF_SUCCESS;
}

static SSMSTRMOPS const g_ftmR3MemoryOps =
{
    SSMSTRMOPS_VERSION,
    pfnMemoryWrite,
    pfnMemoryRead,
    pfnMemorySeek,
    pfnMemoryTell,
    pfnMemorySize,
    pfnMemoryIsOk,
    pfnMemoryClose,
    SSMSTRMOPS_VERSION
};



bool FDPVBOX_Save(void *pUserHandle)
{
    Log1(("[DBGC] SAVE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    PUVM pUVM = myVBOXHandle->pUVM;

    PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, 0);

    //Avoid Interrupt during save, we don't want Interrupt in our save state
    pVCpu->mystate.s.bDisableInterrupt = true;

    //Ask all CPU to suspend
    printf("Save.FDPVBOX_Pause\n");
    FDPVBOX_Pause(pUserHandle);

    printf("Save.UsetBreakpoint\n");
    for(int BreakpointId = 0; BreakpointId < FDP_MAX_BREAKPOINT; BreakpointId++){
        FDPVBOX_unsetBreakpoint(pUserHandle, BreakpointId);
    }
    //Disable Hardware breakpoint
    FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR0_REGISTER, 0);
    FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR1_REGISTER, 0);
    FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR2_REGISTER, 0);
    FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR3_REGISTER, 0);
    FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR6_REGISTER, 0);
    FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR7_REGISTER, 0);

    FDPVBOX_readRegister(pUserHandle, 0, FDP_VDR0_REGISTER, &myVBOXHandle->aVisibleGuestDebugRegisterSave[0]);
    FDPVBOX_readRegister(pUserHandle, 0, FDP_VDR1_REGISTER, &myVBOXHandle->aVisibleGuestDebugRegisterSave[1]);
    FDPVBOX_readRegister(pUserHandle, 0, FDP_VDR2_REGISTER, &myVBOXHandle->aVisibleGuestDebugRegisterSave[2]);
    FDPVBOX_readRegister(pUserHandle, 0, FDP_VDR3_REGISTER, &myVBOXHandle->aVisibleGuestDebugRegisterSave[3]);
    FDPVBOX_readRegister(pUserHandle, 0, FDP_VDR6_REGISTER, &myVBOXHandle->aVisibleGuestDebugRegisterSave[6]);
    FDPVBOX_readRegister(pUserHandle, 0, FDP_VDR7_REGISTER, &myVBOXHandle->aVisibleGuestDebugRegisterSave[7]);

    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, i);
        pVCpu->mystate.s.bSuspendRequired = true;
    }

    //Resume all CPU for suspend
    printf("Save.FDPVBOX_Resume\n");
    FDPVBOX_Resume(pUserHandle);

    //Suspend all CPU
    printf("Save.VMR3Suspend\n");
    VMR3Suspend(pUVM, VMSUSPENDREASON_USER);
    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        PVMCPU pVCpu2 = VMMR3GetCpuByIdU(pUVM, i);
        pVCpu2->mystate.s.bSuspendRequired = false;
    }

    //Alloc SaveState memory
    if(myVBOXHandle->pMemorySSM->pMemory == NULL){
        myVBOXHandle->pMemorySSM->cbMemory = MMR3PhysGetRamSizeU(pUVM);
        myVBOXHandle->pMemorySSM->pMemory = (uint8_t*)malloc(myVBOXHandle->pMemorySSM->cbMemory);
    }

    //Set offset
    myVBOXHandle->pMemorySSM->CurrentOffset = 0;
    myVBOXHandle->pMemorySSM->MaxOffset = 0;

    //Save state
    printf("Save.VMR3SaveFT\n");
    bool bSuspended = false;
    VMR3SaveFT(pUVM, &g_ftmR3MemoryOps, (void*)myVBOXHandle->pMemorySSM, &bSuspended, true);

    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, i);
        pVCpu->mystate.s.bRestoreRequired = true;
    }

    printf("Save.VMR3Resume\n");
    VMR3Resume(pUVM, VMRESUMEREASON_STATE_RESTORED);

    printf("Save.FDPVBOX_Pause\n");
    FDPVBOX_Pause(pUserHandle);

    for(uint32_t i=0; i<VMR3GetCPUCount(pUVM); i++){
        PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, i);
        pVCpu->mystate.s.bRestoreRequired = false;
    }

    pVCpu->mystate.s.bDisableInterrupt = false;

    return true;
}

bool FDPVBOX_Restore(void *pUserHandle)
{
    Log1(("[DBGC] RESTORE\n"));
    FDPVBOX_USERHANDLE_T* myVBOXHandle = (FDPVBOX_USERHANDLE_T*)pUserHandle;
    PUVM pUVM = myVBOXHandle->pUVM;
    int rc;
    if(myVBOXHandle->pMemorySSM->pMemory != NULL){
        PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, 0);

        //Avoid Interrupt during save, we don't want Interrupt in our save state
        pVCpu->mystate.s.bDisableInterrupt = true;

        printf("Restore.Pause\n");
        FDPVBOX_Pause(pUserHandle);

        printf("Restore.UsetBreakpoint\n");
        for(int BreakpointId = 0; BreakpointId < FDP_MAX_BREAKPOINT; BreakpointId++){
            FDPVBOX_unsetBreakpoint(pUserHandle, BreakpointId);
        }
        //Disable Hardware breakpoint
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR0_REGISTER, 0);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR1_REGISTER, 0);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR2_REGISTER, 0);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR3_REGISTER, 0);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR6_REGISTER, 0);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_DR7_REGISTER, 0);

        printf("Restore.FDPVBOX_Resume\n");
        //Force console client to reconnect
        FDPVBOX_Resume(pUserHandle);

        printf("Restore.VMR3Reset\n");
        VMR3Reset(pUVM);

        SLEEP(500);

        printf("Restore.VMR3Suspend\n");
        rc = VMR3Suspend(pUVM, VMSUSPENDREASON_USER);


        //rc = VMR3Suspend(pUVM, VMSUSPENDREASON_USER);
        //printf("%d\n", rc);

        printf("Restore.VMR3LoadFromStream\n");
        VMR3LoadFromStream(pUVM, &g_ftmR3MemoryOps, (void*)myVBOXHandle->pMemorySSM, nullProgressCallback, NULL);

        printf("Restore.VMR3Resume\n");
        pVCpu->mystate.s.bRestoreRequired = true;
        VMR3Resume(pUVM, VMRESUMEREASON_STATE_RESTORED);

        printf("Restore.FDPVBOX_Pause\n");
        FDPVBOX_Pause(pUserHandle);
        pVCpu->mystate.s.bRestoreRequired = false;

        //Restore visible for Guest Debug Register
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_VDR0_REGISTER, myVBOXHandle->aVisibleGuestDebugRegisterSave[0]);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_VDR1_REGISTER, myVBOXHandle->aVisibleGuestDebugRegisterSave[1]);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_VDR2_REGISTER, myVBOXHandle->aVisibleGuestDebugRegisterSave[2]);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_VDR3_REGISTER, myVBOXHandle->aVisibleGuestDebugRegisterSave[3]);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_VDR6_REGISTER, myVBOXHandle->aVisibleGuestDebugRegisterSave[6]);
        FDPVBOX_writeRegister(pUserHandle, 0, FDP_VDR7_REGISTER, myVBOXHandle->aVisibleGuestDebugRegisterSave[7]);

        pVCpu->mystate.s.bDisableInterrupt = false;

        //printf("%d\n", VMR3ClearInterrupt(pUVM, NULL));

        printf("Restore.Done!\n");
        return true;
    }

    return false;
}

void *CreateCPUSHM(PUVM pUVM)
{
    void* pBuf;

    char aCpuShmName[512];
    strcpy(aCpuShmName,"CPU_");
    strcat(aCpuShmName, VMR3GetName(pUVM));


#ifdef  __linux
    uint32_t fdSHM;

    /* create the shared memory segment */
    fdSHM = shm_open(aCpuShmName, O_CREAT | O_RDWR, 0666);
    if (fdSHM == NULL){
        return NULL;
    }

    /* configure the size of the shared memory segment */
    ftruncate(fdSHM,sizeof(FDP_CPU_CTX));

    /* now map the shared memory segment in the address space of the process */
    pBuf = mmap(0,sizeof(FDP_CPU_CTX), PROT_READ | PROT_WRITE, MAP_SHARED, fdSHM, 0);
    if (pBuf == NULL){
        shm_unlink(aCpuShmName);
        return NULL;
    }
#elif   _WIN32
    HANDLE hMapFile;
    hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(FDP_CPU_CTX),
        aCpuShmName);

    if (hMapFile == NULL){
        return NULL;
    }

    pBuf = MapViewOfFile(hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(FDP_CPU_CTX));

    if (pBuf == NULL){
        CloseHandle(hMapFile);
        return NULL;
    }
#endif

    //Clear SHM
    memset((void*)pBuf, 0, sizeof(FDP_CPU_CTX));

    printf("0x%p\n", sizeof(FDP_CPU_CTX));

    return pBuf;
}

#ifdef  __linux
void *FDPServerThread(void* lpParam)
#elif   _WIN32
DWORD WINAPI FDPServerThread(LPVOID lpParam)
#endif
{
    PUVM pUVM = (PUVM)lpParam;
    MEMORY_SSM_T MemorySSM;
    MemorySSM.pMemory = NULL;
    MemorySSM.CurrentOffset = 0;

    FDP_SHM* pFDPServer = FDP_CreateSHM((char*)VMR3GetName(pUVM));
    if(pFDPServer == NULL){
        printf("FDP SHM creation failed !\n");
        return 0;
    }

    PVMCPU pVCpu = VMMR3GetCpuByIdU(pUVM, 0);
    //PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);
    void* pCpuShm = CreateCPUSHM(pUVM);
    pVCpu->mystate.s.pCpuShm = pCpuShm;
    if(pCpuShm == NULL){
        printf("Failed to CreateCpuShm\n");
    }

    printf("FDP_CreateSHM OK\n");
    FDPVBOX_USERHANDLE_T *pUserHandle = (FDPVBOX_USERHANDLE_T*)malloc(sizeof(FDPVBOX_USERHANDLE_T));
    pUserHandle->pUVM = pUVM;
    pUserHandle->pMemorySSM = &MemorySSM;
    pUserHandle->pFDPServer = pFDPServer;

    //Configure FDP Server Interface
    FDP_SERVER_INTERFACE_T FDPServerInterface;
    FDPServerInterface.pUserHandle = pUserHandle;

    FDPServerInterface.pfnGetState = &FDPVBOX_getState;
    FDPServerInterface.pfnReadRegister = &FDPVBOX_readRegister;
    FDPServerInterface.pfnWriteRegister =  &FDPVBOX_writeRegister;
    FDPServerInterface.pfnWritePhysicalMemory = &FDPVBOX_writePhysicalMemory;
    FDPServerInterface.pfnWriteVirtualMemory = &FDPVBOX_writeVirtualMemory;
    FDPServerInterface.pfnGetMemorySize = &FDPVBOX_getMemorySize;
    FDPServerInterface.pfnResume =  &FDPVBOX_Resume;
    FDPServerInterface.pfnSingleStep =  &FDPVBOX_singleStep;
    FDPServerInterface.pfnPause =  &FDPVBOX_Pause;
    FDPServerInterface.pfnReadMsr =  &FDPVBOX_readMsr;
    FDPServerInterface.pfnWriteMsr =  &FDPVBOX_writeMsr;
    FDPServerInterface.pfnGetCpuCount =  &FDPVBOX_getCpuCount;
    FDPServerInterface.pfnGetCpuState = &FDPVBOX_getCpuState;
    FDPServerInterface.pfnVirtualToPhysical = &FDPVBOX_virtualToPhysical;
    FDPServerInterface.pfnUnsetBreakpoint = &FDPVBOX_unsetBreakpoint;
    FDPServerInterface.pfnGetFxState64 = &FDPVBOX_getFxState64;
    FDPServerInterface.pfnSetFxState64 = &FDPVBOX_setFxState64;
    FDPServerInterface.pfnReadVirtualMemory = &FDPVBOX_readVirtualMemory;
    FDPServerInterface.pfnReadPhysicalMemory = &FDPVBOX_readPhysicalMemory;
    FDPServerInterface.pfnSetBreakpoint = &FDPVBOX_setBreakpoint;
    FDPServerInterface.pfnReadPhysicalMemory = &FDPVBOX_readPhysicalMemory;
    FDPServerInterface.pfnSave = &FDPVBOX_Save;
    FDPServerInterface.pfnRestore = &FDPVBOX_Restore;
    FDPServerInterface.pfnReboot = &FDPVBOX_Reboot;
    FDPServerInterface.pfnInjectInterrupt = &FDPVBOX_InjectInterrupt;

    if (FDP_SetFDPServer(pFDPServer, &FDPServerInterface) == false){
        printf("Failed to FDP_SerFDPServer\n");
        return 0;
    }

    printf("FDP_SetFDPServer OK\n");

    VMR3SetFDPShm(pUVM, pFDPServer);

    printf("VMR3SetFDPShm OK\n");

    if (FDP_ServerLoop(pFDPServer) == false){
        printf("Failed to FDP_ServerLoop\n");
        return 0;
    }

    if(pUserHandle != NULL){
        free(pUserHandle);
    }
    return 0;
}
/*ENDMYCODE*/


/**
 * Spawns a new thread with a TCP based debugging console service.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   ppvData     Where to store a pointer to the instance data.
 */
DBGDECL(int)    DBGCTcpCreate(PUVM pUVM, void **ppvData)
{
    /*MYCODE*/
#ifdef  __linux
    pthread_t pFDPServerThread;
    pthread_create(&pFDPServerThread, NULL, FDPServerThread, pUVM);
#elif   _WIN32
    HANDLE hFDPServerThread = CreateThread(NULL, 0, FDPServerThread, pUVM, 0, NULL);
#endif
    /*ENDMYCODE*/
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

