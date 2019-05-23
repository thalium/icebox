/* $Id: DevParallel.cpp $ */
/** @file
 * DevParallel - Parallel (Port) Device Emulation.
 *
 * Contributed by: Alexander Eichner
 * Based on DevSerial.cpp
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
#define LOG_GROUP LOG_GROUP_DEV_PARALLEL
#include <VBox/vmm/pdmdev.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define PARALLEL_SAVED_STATE_VERSION 1

/* defines for accessing the register bits */
#define LPT_STATUS_BUSY                0x80
#define LPT_STATUS_ACK                 0x40
#define LPT_STATUS_PAPER_OUT           0x20
#define LPT_STATUS_SELECT_IN           0x10
#define LPT_STATUS_ERROR               0x08
#define LPT_STATUS_IRQ                 0x04
#define LPT_STATUS_BIT1                0x02 /* reserved (only for completeness) */
#define LPT_STATUS_EPP_TIMEOUT         0x01

#define LPT_CONTROL_BIT7               0x80 /* reserved (only for completeness) */
#define LPT_CONTROL_BIT6               0x40 /* reserved (only for completeness) */
#define LPT_CONTROL_ENABLE_BIDIRECT    0x20
#define LPT_CONTROL_ENABLE_IRQ_VIA_ACK 0x10
#define LPT_CONTROL_SELECT_PRINTER     0x08
#define LPT_CONTROL_RESET              0x04
#define LPT_CONTROL_AUTO_LINEFEED      0x02
#define LPT_CONTROL_STROBE             0x01

/** mode defines for the extended control register */
#define LPT_ECP_ECR_CHIPMODE_MASK      0xe0
#define LPT_ECP_ECR_CHIPMODE_GET_BITS(reg) ((reg) >> 5)
#define LPT_ECP_ECR_CHIPMODE_SET_BITS(val) ((val) << 5)
#define LPT_ECP_ECR_CHIPMODE_CONFIGURATION 0x07
#define LPT_ECP_ECR_CHIPMODE_FIFO_TEST 0x06
#define LPT_ECP_ECR_CHIPMODE_RESERVED  0x05
#define LPT_ECP_ECR_CHIPMODE_EPP       0x04
#define LPT_ECP_ECR_CHIPMODE_ECP_FIFO  0x03
#define LPT_ECP_ECR_CHIPMODE_PP_FIFO   0x02
#define LPT_ECP_ECR_CHIPMODE_BYTE      0x01
#define LPT_ECP_ECR_CHIPMODE_COMPAT    0x00

/** FIFO status bits in extended control register */
#define LPT_ECP_ECR_FIFO_MASK          0x03
#define LPT_ECP_ECR_FIFO_SOME_DATA     0x00
#define LPT_ECP_ECR_FIFO_FULL          0x02
#define LPT_ECP_ECR_FIFO_EMPTY         0x01

#define LPT_ECP_CONFIGA_FIFO_WITDH_MASK 0x70
#define LPT_ECP_CONFIGA_FIFO_WIDTH_GET_BITS(reg) ((reg) >> 4)
#define LPT_ECP_CONFIGA_FIFO_WIDTH_SET_BITS(val) ((val) << 4)
#define LPT_ECP_CONFIGA_FIFO_WIDTH_16   0x00
#define LPT_ECP_CONFIGA_FIFO_WIDTH_32   0x20
#define LPT_ECP_CONFIGA_FIFO_WIDTH_8    0x10

#define LPT_ECP_FIFO_DEPTH 2


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Parallel device state.
 *
 * @implements  PDMIBASE
 * @implements  PDMIHOSTPARALLELPORT
 */
typedef struct PARALLELPORT
{
    /** Pointer to the device instance - R3 Ptr */
    PPDMDEVINSR3                          pDevInsR3;
    /** Pointer to the device instance - R0 Ptr */
    PPDMDEVINSR0                          pDevInsR0;
    /** Pointer to the device instance - RC Ptr */
    PPDMDEVINSRC                          pDevInsRC;
    /** Alignment. */
    RTRCPTR                               Alignment0;
    /** LUN\#0: The base interface. */
    PDMIBASE                              IBase;
    /** LUN\#0: The host device port interface. */
    PDMIHOSTPARALLELPORT                  IHostParallelPort;
    /** Pointer to the attached base driver. */
    R3PTRTYPE(PPDMIBASE)                  pDrvBase;
    /** Pointer to the attached host device. */
    R3PTRTYPE(PPDMIHOSTPARALLELCONNECTOR) pDrvHostParallelConnector;
    /** Flag whether the device has its RC component enabled. */
    bool                                  fGCEnabled;
    /** Flag whether the device has its R0 component enabled. */
    bool                                  fR0Enabled;
    /** Flag whether an EPP timeout occurred (error handling). */
    bool                                  fEppTimeout;
    /** Base I/O port of the parallel port. */
    RTIOPORT                              IOBase;
    /** IRQ number assigned ot the parallel port. */
    int                                   iIrq;
    /** Data register. */
    uint8_t                               regData;
    /** Status register. */
    uint8_t                               regStatus;
    /** Control register. */
    uint8_t                               regControl;
    /** EPP address register. */
    uint8_t                               regEppAddr;
    /** EPP data register. */
    uint8_t                               regEppData;
    /** More alignment. */
    uint32_t                              u32Alignment;

#if 0 /* Data for ECP implementation, currently unused. */
    uint8_t                               reg_ecp_ecr;
    uint8_t                               reg_ecp_base_plus_400h; /* has different meanings */
    uint8_t                               reg_ecp_config_b;

    /** The ECP FIFO implementation*/
    uint8_t                             ecp_fifo[LPT_ECP_FIFO_DEPTH];
    uint8_t                             abAlignemnt[2];
    int                                 act_fifo_pos_write;
    int                                 act_fifo_pos_read;
#endif
} PARALLELPORT, *PPARALLELPORT;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

#define PDMIHOSTPARALLELPORT_2_PARALLELPORT(pInstance) ( (PARALLELPORT *)((uintptr_t)(pInterface) - RT_UOFFSETOF(PARALLELPORT, IHostParallelPort)) )
#define PDMIHOSTDEVICEPORT_2_PARALLELPORT(pInstance)   ( (PARALLELPORT *)((uintptr_t)(pInterface) - RT_UOFFSETOF(PARALLELPORT, IHostDevicePort)) )
#define PDMIBASE_2_PARALLELPORT(pInstance)             ( (PARALLELPORT *)((uintptr_t)(pInterface) - RT_UOFFSETOF(PARALLELPORT, IBase)) )


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
PDMBOTHCBDECL(int) parallelIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb);
PDMBOTHCBDECL(int) parallelIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb);
#if 0
PDMBOTHCBDECL(int) parallelIOPortReadECP(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb);
PDMBOTHCBDECL(int) parallelIOPortWriteECP(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
#endif
RT_C_DECLS_END


#ifdef IN_RING3
static void parallelR3IrqSet(PARALLELPORT *pThis)
{
    if (pThis->regControl & LPT_CONTROL_ENABLE_IRQ_VIA_ACK)
    {
        LogFlowFunc(("%d 1\n", pThis->iIrq));
        PDMDevHlpISASetIrqNoWait(pThis->CTX_SUFF(pDevIns), pThis->iIrq, 1);
    }
}

static void parallelR3IrqClear(PARALLELPORT *pThis)
{
    LogFlowFunc(("%d 0\n", pThis->iIrq));
    PDMDevHlpISASetIrqNoWait(pThis->CTX_SUFF(pDevIns), pThis->iIrq, 0);
}
#endif

#if 0
static int parallel_ioport_write_ecp(void *opaque, uint32_t addr, uint32_t val)
{
    PARALLELPORT *s = (PARALLELPORT *)opaque;
    unsigned char ch;

    addr &= 7;
    LogFlow(("parallel: write ecp addr=0x%02x val=0x%02x\n", addr, val));
    ch = val;
    switch (addr) {
    default:
    case 0:
        if (LPT_ECP_ECR_CHIPMODE_GET_BITS(s->reg_ecp_ecr) == LPT_ECP_ECR_CHIPMODE_FIFO_TEST) {
            s->ecp_fifo[s->act_fifo_pos_write] = ch;
            s->act_fifo_pos_write++;
            if (s->act_fifo_pos_write < LPT_ECP_FIFO_DEPTH) {
                /* FIFO has some data (clear both FIFO bits) */
                s->reg_ecp_ecr &= ~(LPT_ECP_ECR_FIFO_EMPTY | LPT_ECP_ECR_FIFO_FULL);
            } else {
                /* FIFO is full */
                /* Clear FIFO empty bit */
                s->reg_ecp_ecr &= ~LPT_ECP_ECR_FIFO_EMPTY;
                /* Set FIFO full bit */
                s->reg_ecp_ecr |= LPT_ECP_ECR_FIFO_FULL;
                s->act_fifo_pos_write = 0;
            }
        } else {
            s->reg_ecp_base_plus_400h = ch;
        }
        break;
    case 1:
        s->reg_ecp_config_b = ch;
        break;
    case 2:
        /* If we change the mode clear FIFO */
        if ((ch & LPT_ECP_ECR_CHIPMODE_MASK) != (s->reg_ecp_ecr & LPT_ECP_ECR_CHIPMODE_MASK)) {
            /* reset the fifo */
            s->act_fifo_pos_write = 0;
            s->act_fifo_pos_read = 0;
            /* Set FIFO empty bit */
            s->reg_ecp_ecr |= LPT_ECP_ECR_FIFO_EMPTY;
            /* Clear FIFO full bit */
            s->reg_ecp_ecr &= ~LPT_ECP_ECR_FIFO_FULL;
        }
        /* Set new mode */
        s->reg_ecp_ecr |= LPT_ECP_ECR_CHIPMODE_SET_BITS(LPT_ECP_ECR_CHIPMODE_GET_BITS(ch));
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;
    }
    return VINF_SUCCESS;
}

static uint32_t parallel_ioport_read_ecp(void *opaque, uint32_t addr, int *pRC)
{
    PARALLELPORT *s = (PARALLELPORT *)opaque;
    uint32_t ret = ~0U;

    *pRC = VINF_SUCCESS;

    addr &= 7;
    switch (addr) {
    default:
    case 0:
        if (LPT_ECP_ECR_CHIPMODE_GET_BITS(s->reg_ecp_ecr) == LPT_ECP_ECR_CHIPMODE_FIFO_TEST) {
            ret = s->ecp_fifo[s->act_fifo_pos_read];
            s->act_fifo_pos_read++;
            if (s->act_fifo_pos_read == LPT_ECP_FIFO_DEPTH)
                s->act_fifo_pos_read = 0; /* end of FIFO, start at beginning */
            if (s->act_fifo_pos_read == s->act_fifo_pos_write) {
                /* FIFO is empty */
                /* Set FIFO empty bit */
                s->reg_ecp_ecr |= LPT_ECP_ECR_FIFO_EMPTY;
                /* Clear FIFO full bit */
                s->reg_ecp_ecr &= ~LPT_ECP_ECR_FIFO_FULL;
            } else {
                /* FIFO has some data (clear all FIFO bits) */
                s->reg_ecp_ecr &= ~(LPT_ECP_ECR_FIFO_EMPTY | LPT_ECP_ECR_FIFO_FULL);
            }
        } else {
            ret = s->reg_ecp_base_plus_400h;
        }
        break;
    case 1:
        ret = s->reg_ecp_config_b;
        break;
    case 2:
        ret = s->reg_ecp_ecr;
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;
    }
    LogFlow(("parallel: read ecp addr=0x%02x val=0x%02x\n", addr, ret));
    return ret;
}
#endif

#ifdef IN_RING3
/**
 * @interface_method_impl{PDMIHOSTPARALLELPORT,pfnNotifyInterrupt}
 */
static DECLCALLBACK(int) parallelR3NotifyInterrupt(PPDMIHOSTPARALLELPORT pInterface)
{
    PARALLELPORT *pThis = PDMIHOSTPARALLELPORT_2_PARALLELPORT(pInterface);

    PDMCritSectEnter(pThis->pDevInsR3->pCritSectRoR3, VINF_SUCCESS);
    parallelR3IrqSet(pThis);
    PDMCritSectLeave(pThis->pDevInsR3->pCritSectRoR3);

    return VINF_SUCCESS;
}
#endif /* IN_RING3 */


/**
 * @callback_method_impl{FNIOMIOPORTOUT}
 */
PDMBOTHCBDECL(int) parallelIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PPARALLELPORT);
    int           rc = VINF_SUCCESS;
    RT_NOREF_PV(pvUser);

    if (cb == 1)
    {
        uint8_t u8 = u32;

        Log2(("%s: port %#06x val %#04x\n", __FUNCTION__, uPort, u32));

        uPort &= 7;
        switch (uPort)
        {
            case 0:
#ifndef IN_RING3
                NOREF(u8);
                rc = VINF_IOM_R3_IOPORT_WRITE;
#else
                pThis->regData = u8;
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    LogFlowFunc(("Set data lines 0x%X\n", u8));
                    rc = pThis->pDrvHostParallelConnector->pfnWrite(pThis->pDrvHostParallelConnector, &u8, 1, PDM_PARALLEL_PORT_MODE_SPP);
                    AssertRC(rc);
                }
#endif
                break;
            case 1:
                break;
            case 2:
                /* Set the reserved bits to one */
                u8 |= (LPT_CONTROL_BIT6 | LPT_CONTROL_BIT7);
                if (u8 != pThis->regControl)
                {
#ifndef IN_RING3
                    return VINF_IOM_R3_IOPORT_WRITE;
#else
                    if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                    {
                        /* Set data direction. */
                        if (u8 & LPT_CONTROL_ENABLE_BIDIRECT)
                            rc = pThis->pDrvHostParallelConnector->pfnSetPortDirection(pThis->pDrvHostParallelConnector, false /* fForward */);
                        else
                            rc = pThis->pDrvHostParallelConnector->pfnSetPortDirection(pThis->pDrvHostParallelConnector, true /* fForward */);
                        AssertRC(rc);

                        u8 &= ~LPT_CONTROL_ENABLE_BIDIRECT; /* Clear bit. */

                        rc = pThis->pDrvHostParallelConnector->pfnWriteControl(pThis->pDrvHostParallelConnector, u8);
                        AssertRC(rc);
                    }
                    else
                        u8 &= ~LPT_CONTROL_ENABLE_BIDIRECT; /* Clear bit. */

                    pThis->regControl = u8;
#endif
                }
                break;
            case 3:
#ifndef IN_RING3
                NOREF(u8);
                rc = VINF_IOM_R3_IOPORT_WRITE;
#else
                pThis->regEppAddr = u8;
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    LogFlowFunc(("Write EPP address 0x%X\n", u8));
                    rc = pThis->pDrvHostParallelConnector->pfnWrite(pThis->pDrvHostParallelConnector, &u8, 1, PDM_PARALLEL_PORT_MODE_EPP_ADDR);
                    AssertRC(rc);
                }
#endif
                break;
            case 4:
#ifndef IN_RING3
                NOREF(u8);
                rc = VINF_IOM_R3_IOPORT_WRITE;
#else
                pThis->regEppData = u8;
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    LogFlowFunc(("Write EPP data 0x%X\n", u8));
                    rc = pThis->pDrvHostParallelConnector->pfnWrite(pThis->pDrvHostParallelConnector, &u8, 1, PDM_PARALLEL_PORT_MODE_EPP_DATA);
                    AssertRC(rc);
                }
#endif
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
            default:
                break;
        }
    }
    else
        AssertMsgFailed(("Port=%#x cb=%d u32=%#x\n", uPort, cb, u32));

    return rc;
}


/**
 * @callback_method_impl{FNIOMIOPORTIN}
 */
PDMBOTHCBDECL(int) parallelIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);
    int           rc = VINF_SUCCESS;
    RT_NOREF_PV(pvUser);

    if (cb == 1)
    {
        uPort &= 7;
        switch (uPort)
        {
            case 0:
                if (!(pThis->regControl & LPT_CONTROL_ENABLE_BIDIRECT))
                    *pu32 = pThis->regData;
                else
                {
#ifndef IN_RING3
                    rc = VINF_IOM_R3_IOPORT_READ;
#else
                    if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                    {
                        rc = pThis->pDrvHostParallelConnector->pfnRead(pThis->pDrvHostParallelConnector, &pThis->regData,
                                                                       1, PDM_PARALLEL_PORT_MODE_SPP);
                        Log(("Read data lines 0x%X\n", pThis->regData));
                        AssertRC(rc);
                    }
                    *pu32 = pThis->regData;
#endif
                }
                break;
            case 1:
#ifndef IN_RING3
                rc = VINF_IOM_R3_IOPORT_READ;
#else
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    rc = pThis->pDrvHostParallelConnector->pfnReadStatus(pThis->pDrvHostParallelConnector, &pThis->regStatus);
                    AssertRC(rc);
                }
                *pu32 = pThis->regStatus;
                parallelR3IrqClear(pThis);
#endif
                break;
            case 2:
#ifndef IN_RING3
                rc = VINF_IOM_R3_IOPORT_READ;
#else
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    rc = pThis->pDrvHostParallelConnector->pfnReadControl(pThis->pDrvHostParallelConnector, &pThis->regControl);
                    AssertRC(rc);
                    pThis->regControl |= LPT_CONTROL_BIT6 | LPT_CONTROL_BIT7;
                }

                *pu32 = pThis->regControl;
#endif
                break;
            case 3:
#ifndef IN_RING3
                rc = VINF_IOM_R3_IOPORT_READ;
#else
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    rc = pThis->pDrvHostParallelConnector->pfnRead(pThis->pDrvHostParallelConnector, &pThis->regEppAddr,
                                                                   1, PDM_PARALLEL_PORT_MODE_EPP_ADDR);
                    Log(("Read EPP address 0x%X\n", pThis->regEppAddr));
                    AssertRC(rc);
                }
                *pu32 = pThis->regEppAddr;
#endif
                break;
            case 4:
#ifndef IN_RING3
                rc = VINF_IOM_R3_IOPORT_READ;
#else
                if (RT_LIKELY(pThis->pDrvHostParallelConnector))
                {
                    rc = pThis->pDrvHostParallelConnector->pfnRead(pThis->pDrvHostParallelConnector, &pThis->regEppData,
                                                                   1, PDM_PARALLEL_PORT_MODE_EPP_DATA);
                    Log(("Read EPP data 0x%X\n", pThis->regEppData));
                    AssertRC(rc);
                }
                *pu32 = pThis->regEppData;
#endif
                break;
            case 5:
                break;
            case 6:
                break;
            case 7:
                break;
        }
    }
    else
        rc = VERR_IOM_IOPORT_UNUSED;

    return rc;
}

#if 0
/**
 * @callback_method_impl{FNIOMIOPORTOUT, ECP registers.}
 */
PDMBOTHCBDECL(int) parallelIOPortWriteECP(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);
    int            rc = VINF_SUCCESS;

    if (cb == 1)
    {
        Log2(("%s: ecp port %#06x val %#04x\n", __FUNCTION__, Port, u32));
        rc = parallel_ioport_write_ecp (pThis, Port, u32);
    }
    else
        AssertMsgFailed(("Port=%#x cb=%d u32=%#x\n", Port, cb, u32));

    return rc;
}

/**
 * @callback_method_impl{FNIOMIOPORTOUT, ECP registers.}
 */
PDMBOTHCBDECL(int) parallelIOPortReadECP(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);
    int           rc = VINF_SUCCESS;

    if (cb == 1)
    {
        *pu32 = parallel_ioport_read_ecp (pThis, Port, &rc);
        Log2(("%s: ecp port %#06x val %#04x\n", __FUNCTION__, Port, *pu32));
    }
    else
        rc = VERR_IOM_IOPORT_UNUSED;

    return rc;
}
#endif

#ifdef IN_RING3

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) parallelR3LiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF(uPass);
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);

    SSMR3PutS32(pSSM, pThis->iIrq);
    SSMR3PutU32(pSSM, pThis->IOBase);
    SSMR3PutU32(pSSM, UINT32_MAX); /* sanity/terminator */
    return VINF_SSM_DONT_CALL_AGAIN;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) parallelR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);

    SSMR3PutU8(pSSM, pThis->regData);
    SSMR3PutU8(pSSM, pThis->regStatus);
    SSMR3PutU8(pSSM, pThis->regControl);

    parallelR3LiveExec(pDevIns, pSSM, 0);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) parallelR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);

    AssertMsgReturn(uVersion == PARALLEL_SAVED_STATE_VERSION, ("%d\n", uVersion), VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);
    if (uPass == SSM_PASS_FINAL)
    {
        SSMR3GetU8(pSSM, &pThis->regData);
        SSMR3GetU8(pSSM, &pThis->regStatus);
        SSMR3GetU8(pSSM, &pThis->regControl);
    }

    /* the config */
    int32_t  iIrq;
    SSMR3GetS32(pSSM, &iIrq);
    uint32_t uIoBase;
    SSMR3GetU32(pSSM, &uIoBase);
    uint32_t u32;
    int rc = SSMR3GetU32(pSSM, &u32);
    if (RT_FAILURE(rc))
        return rc;
    AssertMsgReturn(u32 == UINT32_MAX, ("%#x\n", u32), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

    if (pThis->iIrq != iIrq)
        return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("IRQ changed: config=%#x state=%#x"), pThis->iIrq, iIrq);

    if (pThis->IOBase != uIoBase)
        return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("IOBase changed: config=%#x state=%#x"), pThis->IOBase, uIoBase);

    /* not necessary... but it doesn't harm. */
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) parallelR3QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PARALLELPORT *pThis = PDMIBASE_2_PARALLELPORT(pInterface);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTPARALLELPORT, &pThis->IHostParallelPort);
    return NULL;
}


/**
 * @copydoc FNPDMDEVRELOCATE
 */
static DECLCALLBACK(void) parallelR3Relocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT *);
    pThis->pDevInsRC += offDelta;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) parallelR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    int            rc;
    PARALLELPORT *pThis = PDMINS_2_DATA(pDevIns, PARALLELPORT*);

    Assert(iInstance < 4);

    /*
     * Init the data.
     */
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);

    /* IBase */
    pThis->IBase.pfnQueryInterface = parallelR3QueryInterface;

    /* IHostParallelPort */
    pThis->IHostParallelPort.pfnNotifyInterrupt = parallelR3NotifyInterrupt;

    /* Init parallel state */
    pThis->regData = 0;
#if 0 /* ECP implementation not complete. */
    pThis->reg_ecp_ecr = LPT_ECP_ECR_CHIPMODE_COMPAT | LPT_ECP_ECR_FIFO_EMPTY;
    pThis->act_fifo_pos_read = 0;
    pThis->act_fifo_pos_write = 0;
#endif

    /*
     * Validate and read the configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "IRQ\0" "IOBase\0" "GCEnabled\0" "R0Enabled\0"))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("Configuration error: Unknown config key"));

    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &pThis->fGCEnabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"GCEnabled\" value"));

    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &pThis->fR0Enabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"R0Enabled\" value"));
    rc = CFGMR3QueryS32Def(pCfg, "IRQ", &pThis->iIrq, 7);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"IRQ\" value"));
    rc = CFGMR3QueryU16Def(pCfg, "IOBase", &pThis->IOBase, 0x378);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"IOBase\" value"));

    /*
     * Register the I/O ports and saved state.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, pThis->IOBase, 8, 0,
                                 parallelIOPortWrite, parallelIOPortRead,
                                 NULL, NULL, "Parallel");
    if (RT_FAILURE(rc))
        return rc;

#if 0
    /* register ecp registers */
    rc = PDMDevHlpIOPortRegister(pDevIns, io_base+0x400, 8, 0,
                                 parallelIOPortWriteECP, parallelIOPortReadECP,
                                 NULL, NULL, "PARALLEL ECP");
    if (RT_FAILURE(rc))
        return rc;
#endif

    if (pThis->fGCEnabled)
    {
        rc = PDMDevHlpIOPortRegisterRC(pDevIns, pThis->IOBase, 8, 0, "parallelIOPortWrite",
                                       "parallelIOPortRead", NULL, NULL, "Parallel");
        if (RT_FAILURE(rc))
            return rc;

#if 0
        rc = PDMDevHlpIOPortRegisterGC(pDevIns, io_base+0x400, 8, 0, "parallelIOPortWriteECP",
                                       "parallelIOPortReadECP", NULL, NULL, "Parallel Ecp");
        if (RT_FAILURE(rc))
            return rc;
#endif
    }

    if (pThis->fR0Enabled)
    {
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, pThis->IOBase, 8, 0, "parallelIOPortWrite",
                                       "parallelIOPortRead", NULL, NULL, "Parallel");
        if (RT_FAILURE(rc))
            return rc;

#if 0
        rc = PDMDevHlpIOPortRegisterR0(pDevIns, io_base+0x400, 8, 0, "parallelIOPortWriteECP",
                                       "parallelIOPortReadECP", NULL, NULL, "Parallel Ecp");
        if (RT_FAILURE(rc))
            return rc;
#endif
    }

    rc = PDMDevHlpSSMRegister3(pDevIns, PARALLEL_SAVED_STATE_VERSION, sizeof(*pThis),
                               parallelR3LiveExec, parallelR3SaveExec, parallelR3LoadExec);
    if (RT_FAILURE(rc))
        return rc;


    /*
     * Attach the parallel port driver and get the interfaces.
     * For now no run-time changes are supported.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->IBase, &pThis->pDrvBase, "Parallel Host");
    if (RT_SUCCESS(rc))
    {
        pThis->pDrvHostParallelConnector = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIHOSTPARALLELCONNECTOR);

        /* Set compatibility mode */
        //pThis->pDrvHostParallelConnector->pfnSetMode(pThis->pDrvHostParallelConnector, PDM_PARALLEL_PORT_MODE_COMPAT);
        /* Get status of control register */
        pThis->pDrvHostParallelConnector->pfnReadControl(pThis->pDrvHostParallelConnector, &pThis->regControl);

        AssertMsgReturn(pThis->pDrvHostParallelConnector,
                        ("Configuration error: instance %d has no host parallel interface!\n", iInstance),
                        VERR_PDM_MISSING_INTERFACE);
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
    {
        pThis->pDrvBase = NULL;
        pThis->pDrvHostParallelConnector = NULL;
        LogRel(("Parallel%d: no unit\n", iInstance));
    }
    else
    {
        AssertMsgFailed(("Parallel%d: Failed to attach to host driver. rc=%Rrc\n", iInstance, rc));
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Parallel device %d cannot attach to host driver"), iInstance);
    }

    return VINF_SUCCESS;
}

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceParallelPort =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "parallel",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Parallel Communication Port",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_PARALLEL,
    /* cMaxInstances */
    2,
    /* cbInstance */
    sizeof(PARALLELPORT),
    /* pfnConstruct */
    parallelR3Construct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    parallelR3Relocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};
#endif /* IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */

