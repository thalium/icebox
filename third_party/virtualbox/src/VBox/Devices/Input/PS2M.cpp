/* $Id: PS2M.cpp $ */
/** @file
 * PS2M - PS/2 auxiliary device (mouse) emulation.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * References:
 *
 * The Undocumented PC (2nd Ed.), Frank van Gilluwe, Addison-Wesley, 1996.
 * IBM TrackPoint System Version 4.0 Engineering Specification, 1999.
 * ELAN Microelectronics eKM8025 USB & PS/2 Mouse Controller, 2006.
 *
 *
 * Notes:
 *
 *  - The auxiliary device commands are very similar to keyboard commands.
 *    Most keyboard commands which do not specifically deal with the keyboard
 *    (enable, disable, reset) have identical counterparts.
 *  - The code refers to 'auxiliary device' and 'mouse'; these terms are not
 *    quite interchangeable. 'Auxiliary device' is used when referring to the
 *    generic PS/2 auxiliary device interface and 'mouse' when referring to
 *    a mouse attached to the auxiliary port.
 *  - The basic modes of operation are reset, stream, and remote. Those are
 *    mutually exclusive. Stream and remote modes can additionally have wrap
 *    mode enabled.
 *  - The auxiliary device sends unsolicited data to the host only when it is
 *    both in stream mode and enabled. Otherwise it only responds to commands.
 *
 *
 * There are three report packet formats supported by the emulated device. The
 * standard three-byte PS/2 format (with middle button support), IntelliMouse
 * four-byte format with added scroll wheel, and IntelliMouse Explorer four-byte
 * format with reduced scroll wheel range but two additional buttons. Note that
 * the first three bytes of the report are always the same.
 *
 * Upon reset, the mouse is always in the standard PS/2 mode. A special 'knock'
 * sequence can be used to switch to ImPS/2 or ImEx mode. Three consecutive
 * Set Sampling Rate (0F3h) commands with arguments 200, 100, 80 switch to ImPS/2
 * mode. While in ImPS/2 or PS/2 mode, three consecutive Set Sampling Rate
 * commands with arguments 200, 200, 80 switch to ImEx mode. The Read ID (0F2h)
 * command will report the currently selected protocol.
 *
 *
 * Standard PS/2 pointing device three-byte report packet format:
 *
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * |Bit/byte|  bit 7 |  bit 6 |  bit 5 |  bit 4 |  bit 3 |  bit 2 |  bit 1 |  bit 0 |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * | Byte 1 | Y ovfl | X ovfl | Y sign | X sign |  Sync  | M btn  | R btn  | L btn  |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * | Byte 2 |              X movement delta (two's complement)                      |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * | Byte 3 |              Y movement delta (two's complement)                      |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 *
 *   - The sync bit is always set. It allows software to synchronize data packets
 *     as the X/Y position data typically does not have bit 4 set.
 *   - The overflow bits are set if motion exceeds accumulator range. We use the
 *     maximum range (effectively 9 bits) and do not set the overflow bits.
 *   - Movement in the up/right direction is defined as having positive sign.
 *
 *
 * IntelliMouse PS/2 (ImPS/2) fourth report packet byte:
 *
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * |Bit/byte|  bit 7 |  bit 6 |  bit 5 |  bit 4 |  bit 3 |  bit 2 |  bit 1 |  bit 0 |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * | Byte 4 |              Z movement delta (two's complement)                      |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 *
 *   - The valid range for Z delta values is only -8/+7, i.e. 4 bits.
 *
 * IntelliMouse Explorer (ImEx) fourth report packet byte:
 *
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * |Bit/byte|  bit 7 |  bit 6 |  bit 5 |  bit 4 |  bit 3 |  bit 2 |  bit 1 |  bit 0 |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * | Byte 4 |   0    |   0    | Btn 5  | Btn 4  |  Z mov't delta (two's complement) |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP   LOG_GROUP_DEV_KBD
#include <VBox/vmm/pdmdev.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include "VBoxDD.h"
#define IN_PS2M
#include "PS2Dev.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @name Auxiliary device commands sent by the system.
 * @{ */
#define ACMD_SET_SCALE_11   0xE6    /* Set 1:1 scaling. */
#define ACMD_SET_SCALE_21   0xE7    /* Set 2:1 scaling. */
#define ACMD_SET_RES        0xE8    /* Set resolution. */
#define ACMD_REQ_STATUS     0xE9    /* Get device status. */
#define ACMD_SET_STREAM     0xEA    /* Set stream mode. */
#define ACMD_READ_REMOTE    0xEB    /* Read remote data. */
#define ACMD_RESET_WRAP     0xEC    /* Exit wrap mode. */
#define ACMD_INVALID_1      0xED
#define ACMD_SET_WRAP       0xEE    /* Set wrap (echo) mode. */
#define ACMD_INVALID_2      0xEF
#define ACMD_SET_REMOTE     0xF0    /* Set remote mode. */
#define ACMD_INVALID_3      0xF1
#define ACMD_READ_ID        0xF2    /* Read device ID. */
#define ACMD_SET_SAMP_RATE  0xF3    /* Set sampling rate. */
#define ACMD_ENABLE         0xF4    /* Enable (streaming mode). */
#define ACMD_DISABLE        0xF5    /* Disable (streaming mode). */
#define ACMD_SET_DEFAULT    0xF6    /* Set defaults. */
#define ACMD_INVALID_4      0xF7
#define ACMD_INVALID_5      0xF8
#define ACMD_INVALID_6      0xF9
#define ACMD_INVALID_7      0xFA
#define ACMD_INVALID_8      0xFB
#define ACMD_INVALID_9      0xFC
#define ACMD_INVALID_10     0xFD
#define ACMD_RESEND         0xFE    /* Resend response. */
#define ACMD_RESET          0xFF    /* Reset device. */
/** @} */

/** @name Auxiliary device responses sent to the system.
 * @{ */
#define ARSP_ID             0x00
#define ARSP_BAT_OK         0xAA    /* Self-test passed. */
#define ARSP_ACK            0xFA    /* Command acknowledged. */
#define ARSP_ERROR          0xFC    /* Bad command. */
#define ARSP_RESEND         0xFE    /* Requesting resend. */
/** @} */

/** Define a simple PS/2 input device queue. */
#define DEF_PS2Q_TYPE(name, size)   \
     typedef struct {               \
        uint32_t    rpos;           \
        uint32_t    wpos;           \
        uint32_t    cUsed;          \
        uint32_t    cSize;          \
        uint8_t     abQueue[size];  \
     } name

/* Internal mouse queue sizes. The input queue is relatively large,
 * but the command queue only needs to handle a few bytes.
 */
#define AUX_EVT_QUEUE_SIZE        256
#define AUX_CMD_QUEUE_SIZE          8


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

DEF_PS2Q_TYPE(AuxEvtQ, AUX_EVT_QUEUE_SIZE);
DEF_PS2Q_TYPE(AuxCmdQ, AUX_CMD_QUEUE_SIZE);
#ifndef VBOX_DEVICE_STRUCT_TESTCASE /// @todo hack
DEF_PS2Q_TYPE(GeneriQ, 1);
#endif

/* Auxiliary device special modes of operation. */
typedef enum {
    AUX_MODE_STD,           /* Standard operation. */
    AUX_MODE_RESET,         /* Currently in reset. */
    AUX_MODE_WRAP           /* Wrap mode (echoing input). */
} PS2M_MODE;

/* Auxiliary device operational state. */
typedef enum {
    AUX_STATE_RATE_ERR  = RT_BIT(0),    /* Invalid rate received. */
    AUX_STATE_RES_ERR   = RT_BIT(1),    /* Invalid resolution received. */
    AUX_STATE_SCALING   = RT_BIT(4),    /* 2:1 scaling in effect. */
    AUX_STATE_ENABLED   = RT_BIT(5),    /* Reporting enabled in stream mode. */
    AUX_STATE_REMOTE    = RT_BIT(6)     /* Remote mode (reports on request). */
} PS2M_STATE;

/* Externally visible state bits. */
#define AUX_STATE_EXTERNAL  (AUX_STATE_SCALING | AUX_STATE_ENABLED | AUX_STATE_REMOTE)

/* Protocols supported by the PS/2 mouse. */
typedef enum {
    PS2M_PROTO_PS2STD = 0,  /* Standard PS/2 mouse protocol. */
    PS2M_PROTO_IMPS2  = 3,  /* IntelliMouse PS/2 protocol. */
    PS2M_PROTO_IMEX   = 4   /* IntelliMouse Explorer protocol. */
} PS2M_PROTO;

/* Protocol selection 'knock' states. */
typedef enum {
    PS2M_KNOCK_INITIAL,
    PS2M_KNOCK_1ST,
    PS2M_KNOCK_IMPS2_2ND,
    PS2M_KNOCK_IMEX_2ND
} PS2M_KNOCK_STATE;

/**
 * The PS/2 auxiliary device instance data.
 */
typedef struct PS2M
{
    /** Pointer to parent device (keyboard controller). */
    R3PTRTYPE(void *)   pParent;
    /** Operational state. */
    uint8_t             u8State;
    /** Configured sampling rate. */
    uint8_t             u8SampleRate;
    /** Configured resolution. */
    uint8_t             u8Resolution;
    /** Currently processed command (if any). */
    uint8_t             u8CurrCmd;
    /** Set if the throttle delay is active. */
    bool                fThrottleActive;
    /** Set if the throttle delay is active. */
    bool                fDelayReset;
    /** Operational mode. */
    PS2M_MODE           enmMode;
    /** Currently used protocol. */
    PS2M_PROTO          enmProtocol;
    /** Currently used protocol. */
    PS2M_KNOCK_STATE    enmKnockState;
    /** Buffer holding mouse events to be sent to the host. */
    AuxEvtQ             evtQ;
    /** Command response queue (priority). */
    AuxCmdQ             cmdQ;
    /** Accumulated horizontal movement. */
    int32_t             iAccumX;
    /** Accumulated vertical movement. */
    int32_t             iAccumY;
    /** Accumulated Z axis movement. */
    int32_t             iAccumZ;
    /** Accumulated button presses. */
    uint32_t            fAccumB;
    /** Instantaneous button data. */
    uint32_t            fCurrB;
    /** Button state last sent to the guest. */
    uint32_t            fReportedB;
    /** Throttling delay in milliseconds. */
    uint32_t            uThrottleDelay;

    /** The device critical section protecting everything - R3 Ptr */
    R3PTRTYPE(PPDMCRITSECT) pCritSectR3;
    /** Command delay timer - R3 Ptr. */
    PTMTIMERR3          pDelayTimerR3;
    /** Interrupt throttling timer - R3 Ptr. */
    PTMTIMERR3          pThrottleTimerR3;
    RTR3PTR             Alignment1;

    /** Command delay timer - RC Ptr. */
    PTMTIMERRC          pDelayTimerRC;
    /** Interrupt throttling timer - RC Ptr. */
    PTMTIMERRC          pThrottleTimerRC;

    /** Command delay timer - R0 Ptr. */
    PTMTIMERR0          pDelayTimerR0;
    /** Interrupt throttling timer - R0 Ptr. */
    PTMTIMERR0          pThrottleTimerR0;

    /**
     * Mouse port - LUN#1.
     *
     * @implements  PDMIBASE
     * @implements  PDMIMOUSEPORT
     */
    struct
    {
        /** The base interface for the mouse port. */
        PDMIBASE                            IBase;
        /** The keyboard port base interface. */
        PDMIMOUSEPORT                       IPort;

        /** The base interface of the attached mouse driver. */
        R3PTRTYPE(PPDMIBASE)                pDrvBase;
        /** The keyboard interface of the attached mouse driver. */
        R3PTRTYPE(PPDMIMOUSECONNECTOR)      pDrv;
    } Mouse;
} PS2M, *PPS2M;

AssertCompile(PS2M_STRUCT_FILLER >= sizeof(PS2M));

#ifndef VBOX_DEVICE_STRUCT_TESTCASE


/*********************************************************************************************************************************
*   Test code function declarations                                                                                              *
*********************************************************************************************************************************/
#if defined(RT_STRICT) && defined(IN_RING3)
static void ps2mTestAccumulation(void);
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/**
 * Clear a queue.
 *
 * @param   pQ                  Pointer to the queue.
 */
static void ps2kClearQueue(GeneriQ *pQ)
{
    LogFlowFunc(("Clearing queue %p\n", pQ));
    pQ->wpos  = pQ->rpos;
    pQ->cUsed = 0;
}


/**
 * Add a byte to a queue.
 *
 * @param   pQ                  Pointer to the queue.
 * @param   val                 The byte to store.
 */
static void ps2kInsertQueue(GeneriQ *pQ, uint8_t val)
{
    /* Check if queue is full. */
    if (pQ->cUsed >= pQ->cSize)
    {
        LogRelFlowFunc(("queue %p full (%d entries)\n", pQ, pQ->cUsed));
        return;
    }
    /* Insert data and update circular buffer write position. */
    pQ->abQueue[pQ->wpos] = val;
    if (++pQ->wpos == pQ->cSize)
        pQ->wpos = 0;   /* Roll over. */
    ++pQ->cUsed;
    LogRelFlowFunc(("inserted 0x%02X into queue %p\n", val, pQ));
}

#ifdef IN_RING3

/**
 * Save a queue state.
 *
 * @param   pSSM                SSM handle to write the state to.
 * @param   pQ                  Pointer to the queue.
 */
static void ps2kSaveQueue(PSSMHANDLE pSSM, GeneriQ *pQ)
{
    uint32_t    cItems = pQ->cUsed;
    int         i;

    /* Only save the number of items. Note that the read/write
     * positions aren't saved as they will be rebuilt on load.
     */
    SSMR3PutU32(pSSM, cItems);

    LogFlow(("Storing %d items from queue %p\n", cItems, pQ));

    /* Save queue data - only the bytes actually used (typically zero). */
    for (i = pQ->rpos; cItems-- > 0; i = (i + 1) % pQ->cSize)
        SSMR3PutU8(pSSM, pQ->abQueue[i]);
}

/**
 * Load a queue state.
 *
 * @param   pSSM                SSM handle to read the state from.
 * @param   pQ                  Pointer to the queue.
 *
 * @return  int                 VBox status/error code.
 */
static int ps2kLoadQueue(PSSMHANDLE pSSM, GeneriQ *pQ)
{
    int         rc;

    /* On load, always put the read pointer at zero. */
    SSMR3GetU32(pSSM, &pQ->cUsed);

    LogFlow(("Loading %d items to queue %p\n", pQ->cUsed, pQ));

    if (pQ->cUsed > pQ->cSize)
    {
        AssertMsgFailed(("Saved size=%u, actual=%u\n", pQ->cUsed, pQ->cSize));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }

    /* Recalculate queue positions and load data in one go. */
    pQ->rpos = 0;
    pQ->wpos = pQ->cUsed;
    rc = SSMR3GetMem(pSSM, pQ->abQueue, pQ->cUsed);

    return rc;
}

/* Report a change in status down (or is it up?) the driver chain. */
static void ps2mSetDriverState(PPS2M pThis, bool fEnabled)
{
    PPDMIMOUSECONNECTOR pDrv = pThis->Mouse.pDrv;
    if (pDrv)
        pDrv->pfnReportModes(pDrv, fEnabled, false, false);
}

/* Reset the pointing device. */
static void ps2mReset(PPS2M pThis)
{
    ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_BAT_OK);
    ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, 0);
    pThis->enmMode   = AUX_MODE_STD;
    pThis->u8CurrCmd = 0;

    /// @todo move to its proper home!
    ps2mSetDriverState(pThis, true);
}

#endif /* IN_RING3 */

/**
 * Retrieve a byte from a queue.
 *
 * @param   pQ                  Pointer to the queue.
 * @param   pVal                Pointer to storage for the byte.
 *
 * @return  int                 VINF_TRY_AGAIN if queue is empty,
 *                              VINF_SUCCESS if a byte was read.
 */
static int ps2kRemoveQueue(GeneriQ *pQ, uint8_t *pVal)
{
    int     rc = VINF_TRY_AGAIN;

    Assert(pVal);
    if (pQ->cUsed)
    {
        *pVal = pQ->abQueue[pQ->rpos];
        if (++pQ->rpos == pQ->cSize)
            pQ->rpos = 0;   /* Roll over. */
        --pQ->cUsed;
        rc = VINF_SUCCESS;
        LogFlowFunc(("removed 0x%02X from queue %p\n", *pVal, pQ));
    } else
        LogFlowFunc(("queue %p empty\n", pQ));
    return rc;
}

static void ps2mSetRate(PPS2M pThis, uint8_t rate)
{
    Assert(rate);
    pThis->uThrottleDelay = rate ? 1000 / rate : 0;
    pThis->u8SampleRate = rate;
    LogFlowFunc(("Sampling rate %u, throttle delay %u ms\n", pThis->u8SampleRate, pThis->uThrottleDelay));
}

static void ps2mSetDefaults(PPS2M pThis)
{
    LogFlowFunc(("Set mouse defaults\n"));
    /* Standard protocol, reporting disabled, resolution 2, 1:1 scaling. */
    pThis->enmProtocol  = PS2M_PROTO_PS2STD;
    pThis->u8State      = 0;
    pThis->u8Resolution = 2;

    /* Sample rate 100 reports per second. */
    ps2mSetRate(pThis, 100);

    /* Event queue, eccumulators, and button status bits are cleared. */
    ps2kClearQueue((GeneriQ *)&pThis->evtQ);
    pThis->iAccumX = pThis->iAccumY = pThis->iAccumZ = pThis->fAccumB;
}

/* Handle the sampling rate 'knock' sequence which selects protocol. */
static void ps2mRateProtocolKnock(PPS2M pThis, uint8_t rate)
{
    switch (pThis->enmKnockState)
    {
    case PS2M_KNOCK_INITIAL:
        if (rate == 200)
            pThis->enmKnockState = PS2M_KNOCK_1ST;
        break;
    case PS2M_KNOCK_1ST:
        if (rate == 100)
            pThis->enmKnockState = PS2M_KNOCK_IMPS2_2ND;
        else if (rate == 200)
            pThis->enmKnockState = PS2M_KNOCK_IMEX_2ND;
        else
            pThis->enmKnockState = PS2M_KNOCK_INITIAL;
        break;
    case PS2M_KNOCK_IMPS2_2ND:
        if (rate == 80)
        {
            pThis->enmProtocol = PS2M_PROTO_IMPS2;
            LogRelFlow(("PS2M: Switching mouse to ImPS/2 protocol.\n"));
        }
        pThis->enmKnockState = PS2M_KNOCK_INITIAL;
        break;
    case PS2M_KNOCK_IMEX_2ND:
        if (rate == 80)
        {
            pThis->enmProtocol = PS2M_PROTO_IMEX;
            LogRelFlow(("PS2M: Switching mouse to ImEx protocol.\n"));
        }
        RT_FALL_THRU();
    default:
        pThis->enmKnockState = PS2M_KNOCK_INITIAL;
    }
}

/* Three-button event mask. */
#define PS2M_STD_BTN_MASK   (RT_BIT(0) | RT_BIT(1) | RT_BIT(2))

/* Report accumulated movement and button presses, then clear the accumulators. */
static void ps2mReportAccumulatedEvents(PPS2M pThis, GeneriQ *pQueue, bool fAccumBtns)
{
    uint32_t    fBtnState = fAccumBtns ? pThis->fAccumB : pThis->fCurrB;
    uint8_t     val;
    int         dX, dY, dZ;

    /* Clamp the accumulated delta values to the allowed range. */
    dX = RT_MIN(RT_MAX(pThis->iAccumX, -255), 255);
    dY = RT_MIN(RT_MAX(pThis->iAccumY, -255), 255);
    dZ = RT_MIN(RT_MAX(pThis->iAccumZ, -8), 7);

    /* Start with the sync bit and buttons 1-3. */
    val = RT_BIT(3) | (fBtnState & PS2M_STD_BTN_MASK);
    /* Set the X/Y sign bits. */
    if (dX < 0)
        val |= RT_BIT(4);
    if (dY < 0)
        val |= RT_BIT(5);

    /* Send the standard 3-byte packet (always the same). */
    ps2kInsertQueue(pQueue, val);
    ps2kInsertQueue(pQueue, dX);
    ps2kInsertQueue(pQueue, dY);

    /* Add fourth byte if extended protocol is in use. */
    if (pThis->enmProtocol > PS2M_PROTO_PS2STD)
    {
        if (pThis->enmProtocol == PS2M_PROTO_IMPS2)
            ps2kInsertQueue(pQueue, dZ);
        else
        {
            Assert(pThis->enmProtocol == PS2M_PROTO_IMEX);
            /* Z value uses 4 bits; buttons 4/5 in bits 4 and 5. */
            val  = dZ & 0x0f;
            val |= (fBtnState << 1) & (RT_BIT(4) | RT_BIT(5));
            ps2kInsertQueue(pQueue, val);
        }
    }

    /* Clear the movement accumulators, but not necessarily button state. */
    pThis->iAccumX = pThis->iAccumY = pThis->iAccumZ = 0;
    /* Clear accumulated button state only when it's being used. */
    if (fAccumBtns)
    {
        pThis->fReportedB = pThis->fAccumB;
        pThis->fAccumB    = 0;
    }
}


/* Determine whether a reporting rate is one of the valid ones. */
bool ps2mIsRateSupported(uint8_t rate)
{
    static uint8_t  aValidRates[] = { 10, 20, 40, 60, 80, 100, 200 };
    size_t          i;
    bool            fValid = false;

    for (i = 0; i < RT_ELEMENTS(aValidRates); ++i)
        if (aValidRates[i] == rate)
        {
            fValid = true;
            break;
        }

   return fValid;
}

/**
 * Receive and process a byte sent by the keyboard controller.
 *
 * @param   pThis               The PS/2 auxiliary device instance data.
 * @param   cmd                 The command (or data) byte.
 */
int PS2MByteToAux(PPS2M pThis, uint8_t cmd)
{
    uint8_t u8Val;
    bool    fHandled = true;

    LogFlowFunc(("cmd=0x%02X, active cmd=0x%02X\n", cmd, pThis->u8CurrCmd));

    if (pThis->enmMode == AUX_MODE_RESET)
        /* In reset mode, do not respond at all. */
        return VINF_SUCCESS;

    /* If there's anything left in the command response queue, trash it. */
    ps2kClearQueue((GeneriQ *)&pThis->cmdQ);

    if (pThis->enmMode == AUX_MODE_WRAP)
    {
        /* In wrap mode, bounce most data right back.*/
        if (cmd == ACMD_RESET || cmd == ACMD_RESET_WRAP)
            ;   /* Handle as regular commands. */
        else
        {
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, cmd);
            return VINF_SUCCESS;
        }
    }

#ifndef IN_RING3
    /* Reset, Enable, and Set Default commands must be run in R3. */
    if (cmd == ACMD_RESET || cmd == ACMD_ENABLE || cmd == ACMD_SET_DEFAULT)
        return VINF_IOM_R3_IOPORT_WRITE;
#endif

    switch (cmd)
    {
        case ACMD_SET_SCALE_11:
            pThis->u8State &= ~AUX_STATE_SCALING;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_SET_SCALE_21:
            pThis->u8State |= AUX_STATE_SCALING;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_REQ_STATUS:
            /* Report current status, sample rate, and resolution. */
            u8Val  = (pThis->u8State & AUX_STATE_EXTERNAL) | (pThis->fCurrB & PS2M_STD_BTN_MASK);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, u8Val);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, pThis->u8Resolution);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, pThis->u8SampleRate);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_SET_STREAM:
            pThis->u8State &= ~AUX_STATE_REMOTE;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_READ_REMOTE:
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            ps2mReportAccumulatedEvents(pThis, (GeneriQ *)&pThis->cmdQ, false);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_RESET_WRAP:
            pThis->enmMode = AUX_MODE_STD;
            /* NB: Stream mode reporting remains disabled! */
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_SET_WRAP:
            pThis->enmMode = AUX_MODE_WRAP;
            pThis->u8State &= ~AUX_STATE_ENABLED;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_SET_REMOTE:
            pThis->u8State |= AUX_STATE_REMOTE;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_READ_ID:
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, pThis->enmProtocol);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_ENABLE:
            pThis->u8State |= AUX_STATE_ENABLED;
#ifdef IN_RING3
            ps2mSetDriverState(pThis, true);
#else
            AssertLogRelMsgFailed(("Invalid ACMD_ENABLE outside R3!\n"));
#endif
            ps2kClearQueue((GeneriQ *)&pThis->evtQ);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_DISABLE:
            pThis->u8State &= ~AUX_STATE_ENABLED;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_SET_DEFAULT:
            ps2mSetDefaults(pThis);
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_RESEND:
            pThis->u8CurrCmd = 0;
            break;
        case ACMD_RESET:
            ps2mSetDefaults(pThis);
            /// @todo reset more?
            pThis->u8CurrCmd = cmd;
            pThis->enmMode   = AUX_MODE_RESET;
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            if (pThis->fDelayReset)
                /* Slightly delay reset completion; it might take hundreds of ms. */
                TMTimerSetMillies(pThis->CTX_SUFF(pDelayTimer), 1);
            else
#ifdef IN_RING3
                ps2mReset(pThis);
#else
                AssertLogRelMsgFailed(("Invalid ACMD_RESET outside R3!\n"));
#endif
            break;
        /* The following commands need a parameter. */
        case ACMD_SET_RES:
        case ACMD_SET_SAMP_RATE:
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
            pThis->u8CurrCmd = cmd;
            break;
        default:
            /* Sending a command instead of a parameter starts the new command. */
            switch (pThis->u8CurrCmd)
            {
                case ACMD_SET_RES:
                    if (cmd < 4)    /* Valid resolutions are 0-3. */
                    {
                        pThis->u8Resolution = cmd;
                        pThis->u8State &= ~AUX_STATE_RES_ERR;
                        ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
                        pThis->u8CurrCmd = 0;
                    }
                    else
                    {
                        /* Bad resolution. Reply with Resend or Error. */
                        if (pThis->u8State & AUX_STATE_RES_ERR)
                        {
                            pThis->u8State &= ~AUX_STATE_RES_ERR;
                            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ERROR);
                            pThis->u8CurrCmd = 0;
                        }
                        else
                        {
                            pThis->u8State |= AUX_STATE_RES_ERR;
                            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_RESEND);
                            /* NB: Current command remains unchanged. */
                        }
                    }
                    break;
                case ACMD_SET_SAMP_RATE:
                    if (ps2mIsRateSupported(cmd))
                    {
                        pThis->u8State &= ~AUX_STATE_RATE_ERR;
                        ps2mSetRate(pThis, cmd);
                        ps2mRateProtocolKnock(pThis, cmd);
                        ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ACK);
                        pThis->u8CurrCmd = 0;
                    }
                    else
                    {
                        /* Bad rate. Reply with Resend or Error. */
                        if (pThis->u8State & AUX_STATE_RATE_ERR)
                        {
                            pThis->u8State &= ~AUX_STATE_RATE_ERR;
                            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_ERROR);
                            pThis->u8CurrCmd = 0;
                        }
                        else
                        {
                            pThis->u8State |= AUX_STATE_RATE_ERR;
                            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_RESEND);
                            /* NB: Current command remains unchanged. */
                        }
                    }
                    break;
                default:
                    fHandled = false;
            }
            /* Fall through only to handle unrecognized commands. */
            if (fHandled)
                break;
            RT_FALL_THRU();

        case ACMD_INVALID_1:
        case ACMD_INVALID_2:
        case ACMD_INVALID_3:
        case ACMD_INVALID_4:
        case ACMD_INVALID_5:
        case ACMD_INVALID_6:
        case ACMD_INVALID_7:
        case ACMD_INVALID_8:
        case ACMD_INVALID_9:
        case ACMD_INVALID_10:
            Log(("Unsupported command 0x%02X!\n", cmd));
            ps2kInsertQueue((GeneriQ *)&pThis->cmdQ, ARSP_RESEND);
            pThis->u8CurrCmd = 0;
            break;
    }
    LogFlowFunc(("Active cmd now 0x%02X; updating interrupts\n", pThis->u8CurrCmd));
    return VINF_SUCCESS;
}

/**
 * Send a byte (packet data or command response) to the keyboard controller.
 *
 * @returns VINF_SUCCESS or VINF_TRY_AGAIN.
 * @param   pThis               The PS/2 auxiliary device instance data.
 * @param   pb                  Where to return the byte we've read.
 * @remarks Caller must have entered the device critical section.
 */
int PS2MByteFromAux(PPS2M pThis, uint8_t *pb)
{
    int         rc;

    AssertPtr(pb);

    /* Anything in the command queue has priority over data
     * in the event queue. Additionally, packet data are
     * blocked if a command is currently in progress, even if
     * the command queue is empty.
     */
    /// @todo Probably should flush/not fill queue if stream mode reporting disabled?!
    rc = ps2kRemoveQueue((GeneriQ *)&pThis->cmdQ, pb);
    if (rc != VINF_SUCCESS && !pThis->u8CurrCmd && (pThis->u8State & AUX_STATE_ENABLED))
        rc = ps2kRemoveQueue((GeneriQ *)&pThis->evtQ, pb);

    LogFlowFunc(("mouse sends 0x%02x (%svalid data)\n", *pb, rc == VINF_SUCCESS ? "" : "not "));

    return rc;
}

#ifdef IN_RING3

/** Is there any state change to send as events to the guest? */
static uint32_t ps2mHaveEvents(PPS2M pThis)
{
    return   pThis->iAccumX | pThis->iAccumY | pThis->iAccumZ
           | (pThis->fCurrB != pThis->fReportedB) | (pThis->fAccumB != 0);
}

/* Event rate throttling timer to emulate the auxiliary device sampling rate.
 */
static DECLCALLBACK(void) ps2mThrottleTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF2(pDevIns, pTimer);
    PPS2M       pThis = (PS2M *)pvUser;
    uint32_t    uHaveEvents;

    /* Grab the lock to avoid races with PutEvent(). */
    int rc = PDMCritSectEnter(pThis->pCritSectR3, VERR_SEM_BUSY);
    AssertReleaseRC(rc);

    /* If more movement is accumulated, report it and restart the timer. */
    uHaveEvents = ps2mHaveEvents(pThis);
    LogFlowFunc(("Have%s events\n", uHaveEvents ? "" : " no"));

    if (uHaveEvents)
    {
        /* Report accumulated data, poke the KBC, and start the timer. */
        ps2mReportAccumulatedEvents(pThis, (GeneriQ *)&pThis->evtQ, true);
        KBCUpdateInterrupts(pThis->pParent);
        TMTimerSetMillies(pThis->CTX_SUFF(pThrottleTimer), pThis->uThrottleDelay);
    }
    else
        pThis->fThrottleActive = false;

    PDMCritSectLeave(pThis->pCritSectR3);
}

/* The auxiliary device reset is specified to take up to about 500 milliseconds. We need
 * to delay sending the result to the host for at least a tiny little while.
 */
static DECLCALLBACK(void) ps2mDelayTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF2(pDevIns, pTimer);
    PPS2M pThis = (PS2M *)pvUser;

    LogFlowFunc(("Delay timer: cmd %02X\n", pThis->u8CurrCmd));

    Assert(pThis->u8CurrCmd == ACMD_RESET);
    ps2mReset(pThis);

    /// @todo Might want a PS2MCompleteCommand() to push last response, clear command, and kick the KBC...
    /* Give the KBC a kick. */
    KBCUpdateInterrupts(pThis->pParent);
}


/**
 * Debug device info handler. Prints basic auxiliary device state.
 *
 * @param   pDevIns     Device instance which registered the info.
 * @param   pHlp        Callback functions for doing output.
 * @param   pszArgs     Argument string. Optional and specific to the handler.
 */
static DECLCALLBACK(void) ps2mInfoState(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    static const char   *pcszModes[] = { "normal", "reset", "wrap" };
    static const char   *pcszProtocols[] = { "PS/2", NULL, NULL, "ImPS/2", "ImEx" };
    PPS2M   pThis = KBDGetPS2MFromDevIns(pDevIns);
    NOREF(pszArgs);

    Assert(pThis->enmMode <= RT_ELEMENTS(pcszModes));
    Assert(pThis->enmProtocol <= RT_ELEMENTS(pcszProtocols));
    pHlp->pfnPrintf(pHlp, "PS/2 mouse state: %s, %s mode, reporting %s\n",
                    pcszModes[pThis->enmMode],
                    pThis->u8State & AUX_STATE_REMOTE  ? "remote"  : "stream",
                    pThis->u8State & AUX_STATE_ENABLED ? "enabled" : "disabled");
    pHlp->pfnPrintf(pHlp, "Protocol: %s, scaling %u:1\n",
                    pcszProtocols[pThis->enmProtocol], pThis->u8State & AUX_STATE_SCALING ? 2 : 1);
    pHlp->pfnPrintf(pHlp, "Active command %02X\n", pThis->u8CurrCmd);
    pHlp->pfnPrintf(pHlp, "Sampling rate %u reports/sec, resolution %u counts/mm\n",
                    pThis->u8SampleRate, 1 << pThis->u8Resolution);
    pHlp->pfnPrintf(pHlp, "Command queue: %d items (%d max)\n",
                    pThis->cmdQ.cUsed, pThis->cmdQ.cSize);
    pHlp->pfnPrintf(pHlp, "Event queue  : %d items (%d max)\n",
                    pThis->evtQ.cUsed, pThis->evtQ.cSize);
}

/* -=-=-=-=-=- Mouse: IBase  -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) ps2mQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPS2M pThis = RT_FROM_MEMBER(pInterface, PS2M, Mouse.IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->Mouse.IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMOUSEPORT, &pThis->Mouse.IPort);
    return NULL;
}


/* -=-=-=-=-=- Mouse: IMousePort  -=-=-=-=-=- */

/**
 * Mouse event handler.
 *
 * @returns VBox status code.
 * @param   pThis           The PS/2 auxiliary device instance data.
 * @param   dx              X direction movement delta.
 * @param   dy              Y direction movement delta.
 * @param   dz              Z (vertical scroll) movement delta.
 * @param   dw              W (horizontal scroll) movement delta.
 * @param   fButtons        Depressed button mask.
 */
static int ps2mPutEventWorker(PPS2M pThis, int32_t dx, int32_t dy,
                              int32_t dz, int32_t dw, uint32_t fButtons)
{
    RT_NOREF1(dw);
    int             rc = VINF_SUCCESS;

    /* Update internal accumulators and button state. */
    pThis->iAccumX += dx;
    pThis->iAccumY += dy;
    pThis->iAccumZ += dz;
    pThis->fAccumB |= fButtons;     /// @todo accumulate based on current protocol?
    pThis->fCurrB   = fButtons;

    /* Report the event and start the throttle timer unless it's already running. */
    if (!pThis->fThrottleActive)
    {
        ps2mReportAccumulatedEvents(pThis, (GeneriQ *)&pThis->evtQ, true);
        KBCUpdateInterrupts(pThis->pParent);
        pThis->fThrottleActive = true;
        TMTimerSetMillies(pThis->CTX_SUFF(pThrottleTimer), pThis->uThrottleDelay);
    }

    return rc;
}

/* -=-=-=-=-=- Mouse: IMousePort  -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIMOUSEPORT,pfnPutEvent}
 */
static DECLCALLBACK(int) ps2mPutEvent(PPDMIMOUSEPORT pInterface, int32_t dx, int32_t dy,
                                      int32_t dz, int32_t dw, uint32_t fButtons)
{
    PPS2M       pThis = RT_FROM_MEMBER(pInterface, PS2M, Mouse.IPort);
    int rc = PDMCritSectEnter(pThis->pCritSectR3, VERR_SEM_BUSY);
    AssertReleaseRC(rc);

    LogRelFlowFunc(("dX=%d dY=%d dZ=%d dW=%d buttons=%02X\n", dx, dy, dz, dw, fButtons));
    /* NB: The PS/2 Y axis direction is inverted relative to ours. */
    ps2mPutEventWorker(pThis, dx, -dy, dz, dw, fButtons);

    PDMCritSectLeave(pThis->pCritSectR3);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIMOUSEPORT,pfnPutEventAbs}
 */
static DECLCALLBACK(int) ps2mPutEventAbs(PPDMIMOUSEPORT pInterface, uint32_t x, uint32_t y,
                                         int32_t dz, int32_t dw, uint32_t fButtons)
{
    AssertFailedReturn(VERR_NOT_SUPPORTED);
    NOREF(pInterface); NOREF(x); NOREF(y); NOREF(dz); NOREF(dw); NOREF(fButtons);
}

/**
 * @interface_method_impl{PDMIMOUSEPORT,pfnPutEventMultiTouch}
 */
static DECLCALLBACK(int) ps2mPutEventMT(PPDMIMOUSEPORT pInterface, uint8_t cContacts,
                                        const uint64_t *pau64Contacts, uint32_t u32ScanTime)
{
    AssertFailedReturn(VERR_NOT_SUPPORTED);
    NOREF(pInterface); NOREF(cContacts); NOREF(pau64Contacts); NOREF(u32ScanTime);
}



/**
 * Attach command.
 *
 * This is called to let the device attach to a driver for a
 * specified LUN.
 *
 * This is like plugging in the mouse after turning on the
 * system.
 *
 * @returns VBox status code.
 * @param   pThis       The PS/2 auxiliary device instance data.
 * @param   pDevIns     The device instance.
 * @param   iLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
int PS2MAttach(PPS2M pThis, PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    int         rc;

    /* The LUN must be 1, i.e. mouse. */
    Assert(iLUN == 1);
    AssertMsgReturn(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
                    ("PS/2 mouse does not support hotplugging\n"),
                    VERR_INVALID_PARAMETER);

    LogFlowFunc(("iLUN=%d\n", iLUN));

    rc = PDMDevHlpDriverAttach(pDevIns, iLUN, &pThis->Mouse.IBase, &pThis->Mouse.pDrvBase, "Mouse Port");
    if (RT_SUCCESS(rc))
    {
        pThis->Mouse.pDrv = PDMIBASE_QUERY_INTERFACE(pThis->Mouse.pDrvBase, PDMIMOUSECONNECTOR);
        if (!pThis->Mouse.pDrv)
        {
            AssertLogRelMsgFailed(("LUN #1 doesn't have a mouse interface! rc=%Rrc\n", rc));
            rc = VERR_PDM_MISSING_INTERFACE;
        }
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
    {
        Log(("%s/%d: warning: no driver attached to LUN #1!\n", pDevIns->pReg->szName, pDevIns->iInstance));
        rc = VINF_SUCCESS;
    }
    else
        AssertLogRelMsgFailed(("Failed to attach LUN #1! rc=%Rrc\n", rc));

    return rc;
}

void PS2MSaveState(PPS2M pThis, PSSMHANDLE pSSM)
{
    LogFlowFunc(("Saving PS2M state\n"));

    /* Save the core auxiliary device state. */
    SSMR3PutU8(pSSM, pThis->u8State);
    SSMR3PutU8(pSSM, pThis->u8SampleRate);
    SSMR3PutU8(pSSM, pThis->u8Resolution);
    SSMR3PutU8(pSSM, pThis->u8CurrCmd);
    SSMR3PutU8(pSSM, pThis->enmMode);
    SSMR3PutU8(pSSM, pThis->enmProtocol);
    SSMR3PutU8(pSSM, pThis->enmKnockState);

    /* Save the command and event queues. */
    ps2kSaveQueue(pSSM, (GeneriQ *)&pThis->cmdQ);
    ps2kSaveQueue(pSSM, (GeneriQ *)&pThis->evtQ);

    /* Save the command delay timer. Note that the rate throttling
     * timer is *not* saved.
     */
    TMR3TimerSave(pThis->CTX_SUFF(pDelayTimer), pSSM);
}

int PS2MLoadState(PPS2M pThis, PSSMHANDLE pSSM, uint32_t uVersion)
{
    uint8_t     u8;
    int         rc;

    NOREF(uVersion);
    LogFlowFunc(("Loading PS2M state version %u\n", uVersion));

    /* Load the basic auxiliary device state. */
    SSMR3GetU8(pSSM, &pThis->u8State);
    SSMR3GetU8(pSSM, &pThis->u8SampleRate);
    SSMR3GetU8(pSSM, &pThis->u8Resolution);
    SSMR3GetU8(pSSM, &pThis->u8CurrCmd);
    SSMR3GetU8(pSSM, &u8);
    pThis->enmMode       = (PS2M_MODE)u8;
    SSMR3GetU8(pSSM, &u8);
    pThis->enmProtocol   = (PS2M_PROTO)u8;
    SSMR3GetU8(pSSM, &u8);
    pThis->enmKnockState = (PS2M_KNOCK_STATE)u8;

    /* Load the command and event queues. */
    rc = ps2kLoadQueue(pSSM, (GeneriQ *)&pThis->cmdQ);
    AssertRCReturn(rc, rc);
    rc = ps2kLoadQueue(pSSM, (GeneriQ *)&pThis->evtQ);
    AssertRCReturn(rc, rc);

    /* Load the command delay timer, just in case. */
    rc = TMR3TimerLoad(pThis->CTX_SUFF(pDelayTimer), pSSM);
    AssertRCReturn(rc, rc);

    /* Recalculate the throttling delay. */
    ps2mSetRate(pThis, pThis->u8SampleRate);

    ps2mSetDriverState(pThis, !!(pThis->u8State & AUX_STATE_ENABLED));

    return rc;
}

void PS2MFixupState(PPS2M pThis, uint8_t u8State, uint8_t u8Rate, uint8_t u8Proto)
{
    LogFlowFunc(("Fixing up old PS2M state version\n"));

    /* Load the basic auxiliary device state. */
    pThis->u8State      = u8State;
    pThis->u8SampleRate = u8Rate ? u8Rate : 40; /* In case it wasn't saved right. */
    pThis->enmProtocol  = (PS2M_PROTO)u8Proto;

    /* Recalculate the throttling delay. */
    ps2mSetRate(pThis, pThis->u8SampleRate);

    ps2mSetDriverState(pThis, !!(pThis->u8State & AUX_STATE_ENABLED));
}

void PS2MReset(PPS2M pThis)
{
    LogFlowFunc(("Resetting PS2M\n"));

    pThis->u8CurrCmd         = 0;

    /* Clear the queues. */
    ps2kClearQueue((GeneriQ *)&pThis->cmdQ);
    ps2mSetDefaults(pThis);     /* Also clears event queue. */
}

void PS2MRelocate(PPS2M pThis, RTGCINTPTR offDelta, PPDMDEVINS pDevIns)
{
    RT_NOREF2(pDevIns, offDelta);
    LogFlowFunc(("Relocating PS2M\n"));
    pThis->pDelayTimerRC    = TMTimerRCPtr(pThis->pDelayTimerR3);
    pThis->pThrottleTimerRC = TMTimerRCPtr(pThis->pThrottleTimerR3);
}

int PS2MConstruct(PPS2M pThis, PPDMDEVINS pDevIns, void *pParent, int iInstance)
{
    RT_NOREF1(iInstance);

    LogFlowFunc(("iInstance=%d\n", iInstance));

#ifdef RT_STRICT
    ps2mTestAccumulation();
#endif

    pThis->pParent = pParent;

    /* Initialize the queues. */
    pThis->evtQ.cSize = AUX_EVT_QUEUE_SIZE;
    pThis->cmdQ.cSize = AUX_CMD_QUEUE_SIZE;

    pThis->Mouse.IBase.pfnQueryInterface     = ps2mQueryInterface;
    pThis->Mouse.IPort.pfnPutEvent           = ps2mPutEvent;
    pThis->Mouse.IPort.pfnPutEventAbs        = ps2mPutEventAbs;
    pThis->Mouse.IPort.pfnPutEventMultiTouch = ps2mPutEventMT;

    /*
     * Initialize the critical section pointer(s).
     */
    pThis->pCritSectR3 = pDevIns->pCritSectRoR3;

    /*
     * Create the input rate throttling timer. Does not use virtual time!
     */
    PTMTIMER pTimer;
    int rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_REAL, ps2mThrottleTimer, pThis,
                                    TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "PS2M Throttle Timer", &pTimer);
    if (RT_FAILURE(rc))
        return rc;

    pThis->pThrottleTimerR3 = pTimer;
    pThis->pThrottleTimerR0 = TMTimerR0Ptr(pTimer);
    pThis->pThrottleTimerRC = TMTimerRCPtr(pTimer);

    /*
     * Create the command delay timer.
     */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, ps2mDelayTimer, pThis,
                                TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "PS2M Delay Timer", &pTimer);
    if (RT_FAILURE(rc))
        return rc;

    pThis->pDelayTimerR3 = pTimer;
    pThis->pDelayTimerR0 = TMTimerR0Ptr(pTimer);
    pThis->pDelayTimerRC = TMTimerRCPtr(pTimer);

    /*
     * Register debugger info callbacks.
     */
    PDMDevHlpDBGFInfoRegister(pDevIns, "ps2m", "Display PS/2 mouse state.", ps2mInfoState);

    /// @todo Where should we do this?
    ps2mSetDriverState(pThis, true);
    pThis->u8State = 0;
    pThis->enmMode = AUX_MODE_STD;

    return rc;
}

#endif

#if defined(RT_STRICT) && defined(IN_RING3)
/* -=-=-=-=-=- Test code  -=-=-=-=-=- */

/** Test the event accumulation mechanism which we use to delay events going
 * to the guest to one per 10ms (the default PS/2 mouse event rate).  This
 * test depends on ps2mPutEventWorker() not touching the timer if
 * This.fThrottleActive is true. */
/** @todo if we add any more tests it might be worth using a table of test
 * operations and checks. */
static void ps2mTestAccumulation(void)
{
    PS2M This;
    unsigned i;
    int rc;
    uint8_t b;

    RT_ZERO(This);
    This.evtQ.cSize = AUX_EVT_QUEUE_SIZE;
    This.u8State = AUX_STATE_ENABLED;
    This.fThrottleActive = true;
    /* Certain Windows touch pad drivers report a double tap as a press, then
     * a release-press-release all within a single 10ms interval.  Simulate
     * this to check that it is handled right. */
    ps2mPutEventWorker(&This, 0, 0, 0, 0, 1);
    if (ps2mHaveEvents(&This))
        ps2mReportAccumulatedEvents(&This, (GeneriQ *)&This.evtQ, true);
    ps2mPutEventWorker(&This, 0, 0, 0, 0, 0);
    if (ps2mHaveEvents(&This))
        ps2mReportAccumulatedEvents(&This, (GeneriQ *)&This.evtQ, true);
    ps2mPutEventWorker(&This, 0, 0, 0, 0, 1);
    ps2mPutEventWorker(&This, 0, 0, 0, 0, 0);
    if (ps2mHaveEvents(&This))
        ps2mReportAccumulatedEvents(&This, (GeneriQ *)&This.evtQ, true);
    if (ps2mHaveEvents(&This))
        ps2mReportAccumulatedEvents(&This, (GeneriQ *)&This.evtQ, true);
    for (i = 0; i < 12; ++i)
    {
        const uint8_t abExpected[] = { 9, 0, 0, 8, 0, 0, 9, 0, 0, 8, 0, 0};

        rc = PS2MByteFromAux(&This, &b);
        AssertRCSuccess(rc);
        Assert(b == abExpected[i]);
    }
    rc = PS2MByteFromAux(&This, &b);
    Assert(rc != VINF_SUCCESS);
    /* Button hold down during mouse drags was broken at some point during
     * testing fixes for the previous issue.  Test that that works. */
    ps2mPutEventWorker(&This, 0, 0, 0, 0, 1);
    if (ps2mHaveEvents(&This))
        ps2mReportAccumulatedEvents(&This, (GeneriQ *)&This.evtQ, true);
    if (ps2mHaveEvents(&This))
        ps2mReportAccumulatedEvents(&This, (GeneriQ *)&This.evtQ, true);
    for (i = 0; i < 3; ++i)
    {
        const uint8_t abExpected[] = { 9, 0, 0 };

        rc = PS2MByteFromAux(&This, &b);
        AssertRCSuccess(rc);
        Assert(b == abExpected[i]);
    }
    rc = PS2MByteFromAux(&This, &b);
    Assert(rc != VINF_SUCCESS);
}
#endif /* RT_STRICT && IN_RING3 */

#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */
