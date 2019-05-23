/* $Id: DevIchAc97.cpp $ */
/** @file
 * DevIchAc97 - VBox ICH AC97 Audio Controller.
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
#define LOG_GROUP LOG_GROUP_DEV_AC97
#include <VBox/log.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>

#include <iprt/assert.h>
#ifdef IN_RING3
# ifdef DEBUG
#  include <iprt/file.h>
# endif
# include <iprt/mem.h>
# include <iprt/semaphore.h>
# include <iprt/string.h>
# include <iprt/uuid.h>
#endif

#include "VBoxDD.h"

#include "AudioMixBuffer.h"
#include "AudioMixer.h"
#include "DrvAudio.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Current saved state version. */
#define AC97_SSM_VERSION    1

/** Default timer frequency (in Hz). */
#define AC97_TIMER_HZ       200

/** Maximum FIFO size (in bytes). */
#define AC97_FIFO_MAX       256

#define AC97_SR_FIFOE RT_BIT(4)          /* rwc, FIFO error. */
#define AC97_SR_BCIS  RT_BIT(3)          /* rwc, Buffer completion interrupt status. */
#define AC97_SR_LVBCI RT_BIT(2)          /* rwc, Last valid buffer completion interrupt. */
#define AC97_SR_CELV  RT_BIT(1)          /* ro,  Current equals last valid. */
#define AC97_SR_DCH   RT_BIT(0)          /* ro,  Controller halted. */
#define AC97_SR_VALID_MASK (RT_BIT(5) - 1)
#define AC97_SR_WCLEAR_MASK (AC97_SR_FIFOE | AC97_SR_BCIS | AC97_SR_LVBCI)
#define AC97_SR_RO_MASK (AC97_SR_DCH | AC97_SR_CELV)
#define AC97_SR_INT_MASK (AC97_SR_FIFOE | AC97_SR_BCIS | AC97_SR_LVBCI)

#define AC97_CR_IOCE  RT_BIT(4)         /* rw,   Interrupt On Completion Enable. */
#define AC97_CR_FEIE  RT_BIT(3)         /* rw    FIFO Error Interrupt Enable. */
#define AC97_CR_LVBIE RT_BIT(2)         /* rw    Last Valid Buffer Interrupt Enable. */
#define AC97_CR_RR    RT_BIT(1)         /* rw    Reset Registers. */
#define AC97_CR_RPBM  RT_BIT(0)         /* rw    Run/Pause Bus Master. */
#define AC97_CR_VALID_MASK (RT_BIT(5) - 1)
#define AC97_CR_DONT_CLEAR_MASK (AC97_CR_IOCE | AC97_CR_FEIE | AC97_CR_LVBIE)

#define AC97_GC_WR    4                 /* rw    Warm reset. */
#define AC97_GC_CR    2                 /* rw    Cold reset. */
#define AC97_GC_VALID_MASK (RT_BIT(6) - 1)

#define AC97_GS_MD3   RT_BIT(17)        /* rw */
#define AC97_GS_AD3   RT_BIT(16)        /* rw */
#define AC97_GS_RCS   RT_BIT(15)        /* rwc */
#define AC97_GS_B3S12 RT_BIT(14)        /* ro */
#define AC97_GS_B2S12 RT_BIT(13)        /* ro */
#define AC97_GS_B1S12 RT_BIT(12)        /* ro */
#define AC97_GS_S1R1  RT_BIT(11)        /* rwc */
#define AC97_GS_S0R1  RT_BIT(10)        /* rwc */
#define AC97_GS_S1CR  RT_BIT(9)         /* ro */
#define AC97_GS_S0CR  RT_BIT(8)         /* ro */
#define AC97_GS_MINT  RT_BIT(7)         /* ro */
#define AC97_GS_POINT RT_BIT(6)         /* ro */
#define AC97_GS_PIINT RT_BIT(5)         /* ro */
#define AC97_GS_RSRVD (RT_BIT(4)|RT_BIT(3))
#define AC97_GS_MOINT RT_BIT(2)         /* ro */
#define AC97_GS_MIINT RT_BIT(1)         /* ro */
#define AC97_GS_GSCI  RT_BIT(0)         /* rwc */
#define AC97_GS_RO_MASK (AC97_GS_B3S12 |                   \
                         AC97_GS_B2S12 |                   \
                         AC97_GS_B1S12 |                   \
                         AC97_GS_S1CR  |                   \
                         AC97_GS_S0CR  |                   \
                         AC97_GS_MINT  |                   \
                         AC97_GS_POINT |                   \
                         AC97_GS_PIINT |                   \
                         AC97_GS_RSRVD |                   \
                         AC97_GS_MOINT |                   \
                         AC97_GS_MIINT)
#define AC97_GS_VALID_MASK (RT_BIT(18) - 1)
#define AC97_GS_WCLEAR_MASK (AC97_GS_RCS|AC97_GS_S1R1|AC97_GS_S0R1|AC97_GS_GSCI)

/** @name Buffer Descriptor (BD).
 * @{ */
#define AC97_BD_IOC             RT_BIT(31)          /**< Interrupt on Completion. */
#define AC97_BD_BUP             RT_BIT(30)          /**< Buffer Underrun Policy. */

#define AC97_BD_LEN_MASK        0xFFFF              /**< Mask for the BDL buffer length. */

#define AC97_MAX_BDLE           32                  /**< Maximum number of BDLEs. */
/** @} */

/** @name Extended Audio Status and Control Register (EACS).
 * @{ */
#define AC97_EACS_VRA 1                 /**< Variable Rate Audio (4.2.1.1). */
#define AC97_EACS_VRM 8                 /**< Variable Rate Mic Audio (4.2.1.1). */
/** @} */

/** @name Baseline Audio Register Set (BARS).
 * @{ */
#define AC97_BARS_VOL_MASK              0x1f   /**< Volume mask for the Baseline Audio Register Set (5.7.2). */
#define AC97_BARS_VOL_STEPS             31     /**< Volume steps for the Baseline Audio Register Set (5.7.2). */
#define AC97_BARS_VOL_MUTE_SHIFT        15     /**< Mute bit shift for the Baseline Audio Register Set (5.7.2). */
/** @} */

/* AC'97 uses 1.5dB steps, we use 0.375dB steps: 1 AC'97 step equals 4 PDM steps. */
#define AC97_DB_FACTOR                  4

#define AC97_REC_MASK 7
enum
{
    AC97_REC_MIC = 0,
    AC97_REC_CD,
    AC97_REC_VIDEO,
    AC97_REC_AUX,
    AC97_REC_LINE_IN,
    AC97_REC_STEREO_MIX,
    AC97_REC_MONO_MIX,
    AC97_REC_PHONE
};

enum
{
    AC97_Reset                     = 0x00,
    AC97_Master_Volume_Mute        = 0x02,
    AC97_Headphone_Volume_Mute     = 0x04, /** Also known as AUX, see table 16, section 5.7. */
    AC97_Master_Volume_Mono_Mute   = 0x06,
    AC97_Master_Tone_RL            = 0x08,
    AC97_PC_BEEP_Volume_Mute       = 0x0A,
    AC97_Phone_Volume_Mute         = 0x0C,
    AC97_Mic_Volume_Mute           = 0x0E,
    AC97_Line_In_Volume_Mute       = 0x10,
    AC97_CD_Volume_Mute            = 0x12,
    AC97_Video_Volume_Mute         = 0x14,
    AC97_Aux_Volume_Mute           = 0x16,
    AC97_PCM_Out_Volume_Mute       = 0x18,
    AC97_Record_Select             = 0x1A,
    AC97_Record_Gain_Mute          = 0x1C,
    AC97_Record_Gain_Mic_Mute      = 0x1E,
    AC97_General_Purpose           = 0x20,
    AC97_3D_Control                = 0x22,
    AC97_AC_97_RESERVED            = 0x24,
    AC97_Powerdown_Ctrl_Stat       = 0x26,
    AC97_Extended_Audio_ID         = 0x28,
    AC97_Extended_Audio_Ctrl_Stat  = 0x2A,
    AC97_PCM_Front_DAC_Rate        = 0x2C,
    AC97_PCM_Surround_DAC_Rate     = 0x2E,
    AC97_PCM_LFE_DAC_Rate          = 0x30,
    AC97_PCM_LR_ADC_Rate           = 0x32,
    AC97_MIC_ADC_Rate              = 0x34,
    AC97_6Ch_Vol_C_LFE_Mute        = 0x36,
    AC97_6Ch_Vol_L_R_Surround_Mute = 0x38,
    AC97_Vendor_Reserved           = 0x58,
    AC97_AD_Misc                   = 0x76,
    AC97_Vendor_ID1                = 0x7c,
    AC97_Vendor_ID2                = 0x7e
};

/* Codec models. */
typedef enum
{
    AC97_CODEC_STAC9700 = 0,     /* SigmaTel STAC9700 */
    AC97_CODEC_AD1980,           /* Analog Devices AD1980 */
    AC97_CODEC_AD1981B           /* Analog Devices AD1981B */
} AC97CODEC;

/* Analog Devices miscellaneous regiter bits used in AD1980. */
#define AC97_AD_MISC_LOSEL       RT_BIT(5)   /* Surround (rear) goes to line out outputs. */
#define AC97_AD_MISC_HPSEL       RT_BIT(10)  /* PCM (front) goes to headphone outputs. */

#define ICHAC97STATE_2_DEVINS(a_pAC97)   ((a_pAC97)->pDevInsR3)

enum
{
    BUP_SET  = RT_BIT(0),
    BUP_LAST = RT_BIT(1)
};

/** Emits registers for a specific (Native Audio Bus Master BAR) NABMBAR. */
#define AC97_NABMBAR_REGS(prefix, off)                                    \
    enum {                                                                \
        prefix ## _BDBAR = off,      /* Buffer Descriptor Base Address */ \
        prefix ## _CIV   = off + 4,  /* Current Index Value */            \
        prefix ## _LVI   = off + 5,  /* Last Valid Index */               \
        prefix ## _SR    = off + 6,  /* Status Register */                \
        prefix ## _PICB  = off + 8,  /* Position in Current Buffer */     \
        prefix ## _PIV   = off + 10, /* Prefetched Index Value */         \
        prefix ## _CR    = off + 11  /* Control Register */               \
    }

#ifndef VBOX_DEVICE_STRUCT_TESTCASE
typedef enum
{
    AC97SOUNDSOURCE_PI_INDEX = 0, /** PCM in */
    AC97SOUNDSOURCE_PO_INDEX,     /** PCM out */
    AC97SOUNDSOURCE_MC_INDEX,     /** Mic in */
    AC97SOUNDSOURCE_LAST_INDEX
} AC97SOUNDSOURCE;

AC97_NABMBAR_REGS(PI, AC97SOUNDSOURCE_PI_INDEX * 16);
AC97_NABMBAR_REGS(PO, AC97SOUNDSOURCE_PO_INDEX * 16);
AC97_NABMBAR_REGS(MC, AC97SOUNDSOURCE_MC_INDEX * 16);
#endif

enum
{
    /** NABMBAR: Global Control Register. */
    AC97_GLOB_CNT = 0x2c,
    /** NABMBAR Global Status. */
    AC97_GLOB_STA = 0x30,
    /** Codec Access Semaphore Register. */
    AC97_CAS      = 0x34
};

#define AC97_PORT2IDX(a_idx)   ( ((a_idx) >> 4) & 3 )


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Buffer Descriptor List Entry (BDLE).
 */
typedef struct AC97BDLE
{
    uint32_t addr;
    uint32_t ctl_len;
} AC97BDLE, *PAC97BDLE;

/**
 * Bus master register set for an audio stream.
 */
typedef struct AC97BMREGS
{
    uint32_t bdbar;             /** rw 0, Buffer Descriptor List: BAR (Base Address Register). */
    uint8_t  civ;               /** ro 0, Current index value. */
    uint8_t  lvi;               /** rw 0, Last valid index. */
    uint16_t sr;                /** rw 1, Status register. */
    uint16_t picb;              /** ro 0, Position in current buffer (in samples). */
    uint8_t  piv;               /** ro 0, Prefetched index value. */
    uint8_t  cr;                /** rw 0, Control register. */
    int      bd_valid;          /** Whether current BDLE is initialized or not. */
    AC97BDLE bd;                /** Current Buffer Descriptor List Entry (BDLE). */
} AC97BMREGS, *PAC97BMREGS;

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
/**
 * Structure keeping the AC'97 stream's state for asynchronous I/O.
 */
typedef struct AC97STREAMSTATEAIO
{
    /** Thread handle for the actual I/O thread. */
    RTTHREAD              Thread;
    /** Event for letting the thread know there is some data to process. */
    RTSEMEVENT            Event;
    /** Critical section for synchronizing access. */
    RTCRITSECT            CritSect;
    /** Started indicator. */
    volatile bool         fStarted;
    /** Shutdown indicator. */
    volatile bool         fShutdown;
    /** Whether the thread should do any data processing or not. */
    volatile bool         fEnabled;
    uint32_t              Padding1;
} AC97STREAMSTATEAIO, *PAC97STREAMSTATEAIO;
#endif

/**
 * Structure for keeping the internal state of an AC'97 stream.
 */
typedef struct AC97STREAMSTATE
{
    /** Circular buffer (FIFO) for holding DMA'ed data. */
    R3PTRTYPE(PRTCIRCBUF) pCircBuf;
    /** Criticial section for this stream. */
    RTCRITSECT            CritSect;
    /** The stream's current configuration. */
    PDMAUDIOSTREAMCFG     Cfg;
    uint32_t              Padding;
#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
    /** Asynchronous I/O state members. */
    AC97STREAMSTATEAIO    AIO;
#endif
} AC97STREAMSTATE, *PAC97STREAMSTATE;

/**
 * Structure for an AC'97 stream.
 */
typedef struct AC97STREAM
{
    /** Stream number (SDn). */
    uint8_t         u8SD;
    /** Bus master registers of this stream. */
    AC97BMREGS      Regs;
    /** Internal state of this stream. */
    AC97STREAMSTATE State;
} AC97STREAM, *PAC97STREAM;

typedef struct AC97STATE *PAC97STATE;
#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
/**
 * Structure for the async I/O thread context.
 */
typedef struct AC97STREAMTHREADCTX
{
    PAC97STATE  pThis;
    PAC97STREAM pStream;
} AC97STREAMTHREADCTX, *PAC97STREAMTHREADCTX;
#endif

/**
 * Structure defining a (host backend) driver stream.
 * Each driver has its own instances of audio mixer streams, which then
 * can go into the same (or even different) audio mixer sinks.
 */
typedef struct AC97DRIVERSTREAM
{
    union
    {
        /** Desired playback destination (for an output stream). */
        PDMAUDIOPLAYBACKDEST           Dest;
        /** Desired recording source (for an input stream). */
        PDMAUDIORECSOURCE              Source;
    } DestSource;
    uint8_t                            Padding1[4];
    /** Associated mixer stream handle. */
    R3PTRTYPE(PAUDMIXSTREAM)           pMixStrm;
} AC97DRIVERSTREAM, *PAC97DRIVERSTREAM;

/**
 * Struct for maintaining a host backend driver.
 */
typedef struct AC97DRIVER
{
    /** Node for storing this driver in our device driver list of AC97STATE. */
    RTLISTNODER3                       Node;
    /** Pointer to AC97 controller (state). */
    R3PTRTYPE(PAC97STATE)              pAC97State;
    /** Driver flags. */
    PDMAUDIODRVFLAGS                   fFlags;
    uint32_t                           PaddingFlags;
    /** LUN # to which this driver has been assigned. */
    uint8_t                            uLUN;
    /** Whether this driver is in an attached state or not. */
    bool                               fAttached;
    uint8_t                            Padding[4];
    /** Pointer to attached driver base interface. */
    R3PTRTYPE(PPDMIBASE)               pDrvBase;
    /** Audio connector interface to the underlying host backend. */
    R3PTRTYPE(PPDMIAUDIOCONNECTOR)     pConnector;
    /** Driver stream for line input. */
    AC97DRIVERSTREAM                   LineIn;
    /** Driver stream for mic input. */
    AC97DRIVERSTREAM                   MicIn;
    /** Driver stream for output. */
    AC97DRIVERSTREAM                   Out;
} AC97DRIVER, *PAC97DRIVER;

/**
 * Structure for maintaining an AC'97 device state.
 */
typedef struct AC97STATE
{
    /** The PCI device state. */
    PDMPCIDEV               PciDev;
    /** R3 Pointer to the device instance. */
    PPDMDEVINSR3            pDevInsR3;
    /** Critical section protecting the AC'97 state. */
    PDMCRITSECT             CritSect;
    /** Global Control (Bus Master Control Register). */
    uint32_t                glob_cnt;
    /** Global Status (Bus Master Control Register). */
    uint32_t                glob_sta;
    /** Codec Access Semaphore Register (Bus Master Control Register). */
    uint32_t                cas;
    uint32_t                last_samp;
    uint8_t                 mixer_data[256];
    /** AC'97 stream for line-in. */
    AC97STREAM              StreamLineIn;
    /** AC'97 stream for microphone-in. */
    AC97STREAM              StreamMicIn;
    /** AC'97 stream for output. */
    AC97STREAM              StreamOut;
    /** Number of active (running) SDn streams. */
    uint8_t                 cStreamsActive;
#ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
    /** The timer for pumping data thru the attached LUN drivers. */
    PTMTIMERR3              pTimer;
# if HC_ARCH_BITS == 32
    uint32_t                Padding0;
# endif
    /** Flag indicating whether the timer is active or not. */
    bool                    fTimerActive;
    uint8_t                 u8Padding1[7];
    /** The timer interval for pumping data thru the LUN drivers in timer ticks. */
    uint64_t                cTimerTicks;
    /** Timestamp of the last timer callback (ac97Timer).
     * Used to calculate the time actually elapsed between two timer callbacks. */
    uint64_t                uTimerTS;
#endif
#ifdef VBOX_WITH_STATISTICS
    STAMPROFILE             StatTimer;
    STAMPROFILE             StatIn;
    STAMPROFILE             StatOut;
    STAMCOUNTER             StatBytesRead;
    STAMCOUNTER             StatBytesWritten;
#endif
    /** List of associated LUN drivers (AC97DRIVER). */
    RTLISTANCHOR            lstDrv;
    /** The device's software mixer. */
    R3PTRTYPE(PAUDIOMIXER)  pMixer;
    /** Audio sink for PCM output. */
    R3PTRTYPE(PAUDMIXSINK)  pSinkOut;
    /** Audio sink for line input. */
    R3PTRTYPE(PAUDMIXSINK)  pSinkLineIn;
    /** Audio sink for microphone input. */
    R3PTRTYPE(PAUDMIXSINK)  pSinkMicIn;
    uint8_t                 silence[128];
    int                     bup_flag;
    /** The base interface for LUN\#0. */
    PDMIBASE                IBase;
    /** Base port of the I/O space region. */
    RTIOPORT                IOPortBase[2];
    /** Codec model. */
    uint32_t                uCodecModel;
} AC97STATE, *PAC97STATE;

/**
 * Acquires the AC'97 lock.
 */
#define DEVAC97_LOCK(a_pThis) \
    do { \
        int rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, VERR_IGNORED); \
        AssertRC(rcLock); \
    } while (0)

/**
 * Acquires the AC'97 lock or returns.
 */
# define DEVAC97_LOCK_RETURN(a_pThis, a_rcBusy) \
    do { \
        int rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, a_rcBusy); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            return rcLock; \
        } \
    } while (0)

/**
 * Acquires the AC'97 lock or returns.
 */
# define DEVAC97_LOCK_RETURN_VOID(a_pThis) \
    do { \
        int rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, VERR_IGNORED); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            return; \
        } \
    } while (0)

/**
 * Releases the AC'97 lock.
 */
#define DEVAC97_UNLOCK(a_pThis) \
    do { PDMCritSectLeave(&(a_pThis)->CritSect); } while (0)

/**
 * Acquires the TM lock and AC'97 lock, returns on failure.
 */
#define DEVAC97_LOCK_BOTH_RETURN_VOID(a_pThis) \
    do { \
        int rcLock = TMTimerLock((a_pThis)->pTimer, VERR_IGNORED); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            return; \
        } \
        rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, VERR_IGNORED); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            TMTimerUnlock((a_pThis)->pTimer); \
            return; \
        } \
    } while (0)

/**
 * Acquires the TM lock and AC'97 lock, returns on failure.
 */
#define DEVAC97_LOCK_BOTH_RETURN(a_pThis, a_rcBusy) \
    do { \
        int rcLock = TMTimerLock((a_pThis)->pTimer, (a_rcBusy)); \
        if (rcLock != VINF_SUCCESS) \
            return rcLock; \
        rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, (a_rcBusy)); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            TMTimerUnlock((a_pThis)->pTimer); \
            return rcLock; \
        } \
    } while (0)

/**
 * Releases the AC'97 lock and TM lock.
 */
#define DEVAC97_UNLOCK_BOTH(a_pThis) \
    do { \
        PDMCritSectLeave(&(a_pThis)->CritSect); \
        TMTimerUnlock((a_pThis)->pTimer); \
    } while (0)

#ifdef VBOX_WITH_STATISTICS
AssertCompileMemberAlignment(AC97STATE, StatTimer,        8);
AssertCompileMemberAlignment(AC97STATE, StatBytesRead,    8);
AssertCompileMemberAlignment(AC97STATE, StatBytesWritten, 8);
#endif

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

DECLINLINE(PAC97STREAM)   ichac97GetStreamFromIdx(PAC97STATE pThis, uint32_t uIdx);
static int                ichac97StreamCreate(PAC97STATE pThis, PAC97STREAM pStream, uint8_t u8Strm);
static void               ichac97StreamDestroy(PAC97STATE pThis, PAC97STREAM pStream);
static int                ichac97StreamOpen(PAC97STATE pThis, PAC97STREAM pStream);
static int                ichac97StreamReOpen(PAC97STATE pThis, PAC97STREAM pStream);
static int                ichac97StreamClose(PAC97STATE pThis, PAC97STREAM pStream);
static void               ichac97StreamReset(PAC97STATE pThis, PAC97STREAM pStream);
static void               ichac97StreamLock(PAC97STREAM pStream);
static void               ichac97StreamUnlock(PAC97STREAM pStream);
static uint32_t           ichac97StreamGetUsed(PAC97STREAM pStream);
static uint32_t           ichac97StreamGetFree(PAC97STREAM pStream);
static int                ichac97StreamTransfer(PAC97STATE pThis, PAC97STREAM pStream, uint32_t cbToProcessMax);
static void               ichac97StreamUpdate(PAC97STATE pThis, PAC97STREAM pStream, bool fInTimer);

static DECLCALLBACK(void) ichac97Reset(PPDMDEVINS pDevIns);
#ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
static int                ichac97TimerStart(PAC97STATE pThis);
static int                ichac97TimerMaybeStart(PAC97STATE pThis);
static int                ichac97TimerStop(PAC97STATE pThis);
static int                ichac97TimerMaybeStop(PAC97STATE pThis);
static void               ichac97TimerMain(PAC97STATE pThis);
static DECLCALLBACK(void) ichac97Timer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser);
#endif
static void               ichac97DoTransfers(PAC97STATE pThis);

static int                ichac97MixerAddDrvStream(PAC97STATE pThis, PAUDMIXSINK pMixSink, PPDMAUDIOSTREAMCFG pCfg, PAC97DRIVER pDrv);
static int                ichac97MixerAddDrvStreams(PAC97STATE pThis, PAUDMIXSINK pMixSink, PPDMAUDIOSTREAMCFG pCfg);
static void               ichac97MixerRemoveDrvStream(PAC97STATE pThis, PAUDMIXSINK pMixSink, PDMAUDIODIR enmDir, PDMAUDIODESTSOURCE dstSrc, PAC97DRIVER pDrv);
static void               ichac97MixerRemoveDrvStreams(PAC97STATE pThis, PAUDMIXSINK pMixSink, PDMAUDIODIR enmDir, PDMAUDIODESTSOURCE dstSrc);

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
static DECLCALLBACK(int)  ichac97StreamAsyncIOThread(RTTHREAD hThreadSelf, void *pvUser);
static int                ichac97StreamAsyncIOCreate(PAC97STATE pThis, PAC97STREAM pStream);
static int                ichac97StreamAsyncIODestroy(PAC97STATE pThis, PAC97STREAM pStream);
static int                ichac97StreamAsyncIONotify(PAC97STATE pThis, PAC97STREAM pStream);
static void               ichac97StreamAsyncIOLock(PAC97STREAM pStream);
static void               ichac97StreamAsyncIOUnlock(PAC97STREAM pStream);
static void               ichac97StreamAsyncIOEnable(PAC97STREAM pStream, bool fEnable);
#endif

static void ichac97WarmReset(PAC97STATE pThis)
{
    NOREF(pThis);
}

static void ichac97ColdReset(PAC97STATE pThis)
{
    NOREF(pThis);
}

/**
 * Retrieves the audio mixer sink of a corresponding AC'97 stream index.
 *
 * @returns Pointer to audio mixer sink if found, or NULL if not found / invalid.
 * @param   pThis               AC'97 state.
 * @param   uIndex              Stream index to get audio mixer sink for.
 */
DECLINLINE(PAUDMIXSINK) ichac97IndexToSink(PAC97STATE pThis, uint8_t uIndex)
{
    AssertPtrReturn(pThis, NULL);

    switch (uIndex)
    {
        case AC97SOUNDSOURCE_PI_INDEX: return pThis->pSinkLineIn; break;
        case AC97SOUNDSOURCE_PO_INDEX: return pThis->pSinkOut;    break;
        case AC97SOUNDSOURCE_MC_INDEX: return pThis->pSinkMicIn;  break;
        default:                                                  break;
    }

    AssertMsgFailed(("Wrong index %RU8\n", uIndex));
    return NULL;
}

/**
 * Fetches the current BDLE (Buffer Descriptor List Entry) of an AC'97 audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to fetch BDLE for.
 *
 * @remark  Uses CIV as BDLE index.
 */
static void ichac97StreamFetchBDLE(PAC97STATE pThis, PAC97STREAM pStream)
{
    PPDMDEVINS  pDevIns = ICHAC97STATE_2_DEVINS(pThis);
    PAC97BMREGS pRegs   = &pStream->Regs;

    uint32_t u32[2];

    PDMDevHlpPhysRead(pDevIns, pRegs->bdbar + pRegs->civ * 8, &u32[0], sizeof(u32));
    pRegs->bd_valid   = 1;
#if !defined(RT_ARCH_X86) && !defined(RT_ARCH_AMD64)
# error Please adapt the code (audio buffers are little endian)!
#else
    pRegs->bd.addr    = RT_H2LE_U32(u32[0] & ~3);
    pRegs->bd.ctl_len = RT_H2LE_U32(u32[1]);
#endif
    pRegs->picb       = pRegs->bd.ctl_len & AC97_BD_LEN_MASK;
    LogFlowFunc(("bd %2d addr=%#x ctl=%#06x len=%#x(%d bytes)\n",
                  pRegs->civ, pRegs->bd.addr, pRegs->bd.ctl_len >> 16,
                  pRegs->bd.ctl_len & AC97_BD_LEN_MASK,
                 (pRegs->bd.ctl_len & AC97_BD_LEN_MASK) << 1)); /** @todo r=andy Assumes 16bit samples. */
}

/**
 * Updates the status register (SR) of an AC'97 audio stream.
 *
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to update SR for.
 * @param   new_sr              New value for status register (SR).
 */
static void ichac97StreamUpdateSR(PAC97STATE pThis, PAC97STREAM pStream, uint32_t new_sr)
{
    PPDMDEVINS  pDevIns = ICHAC97STATE_2_DEVINS(pThis);
    PAC97BMREGS pRegs   = &pStream->Regs;

    bool fSignal = false;
    int  iIRQL = 0;

    uint32_t new_mask = new_sr & AC97_SR_INT_MASK;
    uint32_t old_mask = pRegs->sr  & AC97_SR_INT_MASK;

    static uint32_t const masks[] = { AC97_GS_PIINT, AC97_GS_POINT, AC97_GS_MINT };

    if (new_mask ^ old_mask)
    {
        /** @todo Is IRQ deasserted when only one of status bits is cleared? */
        if (!new_mask)
        {
            fSignal = true;
            iIRQL   = 0;
        }
        else if ((new_mask & AC97_SR_LVBCI) && (pRegs->cr & AC97_CR_LVBIE))
        {
            fSignal = true;
            iIRQL   = 1;
        }
        else if ((new_mask & AC97_SR_BCIS) && (pRegs->cr & AC97_CR_IOCE))
        {
            fSignal = true;
            iIRQL   = 1;
        }
    }

    pRegs->sr = new_sr;

    LogFlowFunc(("IOC%d, LVB%d, sr=%#x, fSignal=%RTbool, IRQL=%d\n",
                 pRegs->sr & AC97_SR_BCIS, pRegs->sr & AC97_SR_LVBCI, pRegs->sr, fSignal, iIRQL));

    if (fSignal)
    {
        if (iIRQL)
            pThis->glob_sta |=  masks[pStream->u8SD];
        else
            pThis->glob_sta &= ~masks[pStream->u8SD];

        LogFlowFunc(("Setting IRQ level=%d\n", iIRQL));
        PDMDevHlpPCISetIrq(pDevIns, 0, iIRQL);
    }
}

/**
 * Returns whether an AC'97 stream is enabled or not.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 device state.
 * @param   pStream             Stream to return status for.
 */
static bool ichac97StreamIsEnabled(PAC97STATE pThis, PAC97STREAM pStream)
{
    AssertPtrReturn(pThis,   false);
    AssertPtrReturn(pStream, false);

    PAUDMIXSINK pSink = ichac97IndexToSink(pThis, pStream->u8SD);
    bool fIsEnabled = RT_BOOL(AudioMixerSinkGetStatus(pSink) & AUDMIXSINK_STS_RUNNING);

    LogFunc(("[SD%RU8] fIsEnabled=%RTbool\n", pStream->u8SD, fIsEnabled));
    return fIsEnabled;
}

/**
 * Enables or disables an AC'97 audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to enable or disable.
 * @param   fEnable             Whether to enable or disable the stream.
 *
 */
static int ichac97StreamEnable(PAC97STATE pThis, PAC97STREAM pStream, bool fEnable)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    ichac97StreamLock(pStream);

    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
    if (fEnable)
        rc = ichac97StreamAsyncIOCreate(pThis, pStream);
    if (RT_SUCCESS(rc))
    {
        ichac97StreamAsyncIOLock(pStream);
        ichac97StreamAsyncIOEnable(pStream, fEnable);
    }
#endif

    if (fEnable)
    {
        if (pStream->State.pCircBuf)
            RTCircBufReset(pStream->State.pCircBuf);

        rc = ichac97StreamOpen(pThis, pStream);
    }
    else
        rc = ichac97StreamClose(pThis, pStream);

    if (RT_SUCCESS(rc))
    {
        /* First, enable or disable the stream and the stream's sink, if any. */
        rc = AudioMixerSinkCtl(ichac97IndexToSink(pThis, pStream->u8SD),
                               fEnable ? AUDMIXSINKCMD_ENABLE : AUDMIXSINKCMD_DISABLE);
    }

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
    ichac97StreamAsyncIOUnlock(pStream);
#endif

    /* Make sure to leave the lock before (eventually) starting the timer. */
    ichac97StreamUnlock(pStream);

#ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
    /* Second, see if we need to start or stop the timer. */
    if (!fEnable)
        ichac97TimerMaybeStop(pThis);
    else
        ichac97TimerMaybeStart(pThis);
#endif

    LogFunc(("[SD%RU8] cStreamsActive=%RU8, fEnable=%RTbool, rc=%Rrc\n", pStream->u8SD, pThis->cStreamsActive, fEnable, rc));
    return rc;
}

/**
 * Resets an AC'97 stream.
 *
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to reset.
 *
 */
static void ichac97StreamReset(PAC97STATE pThis, PAC97STREAM pStream)
{
    AssertPtrReturnVoid(pThis);
    AssertPtrReturnVoid(pStream);

    ichac97StreamLock(pStream);

    LogFunc(("[SD%RU8]\n", pStream->u8SD));

    if (pStream->State.pCircBuf)
        RTCircBufReset(pStream->State.pCircBuf);

    PAC97BMREGS pRegs = &pStream->Regs;

    pRegs->bdbar    = 0;
    pRegs->civ      = 0;
    pRegs->lvi      = 0;

    pRegs->picb     = 0;
    pRegs->piv      = 0;
    pRegs->cr       = pRegs->cr & AC97_CR_DONT_CLEAR_MASK;
    pRegs->bd_valid = 0;

    RT_ZERO(pThis->silence);

    ichac97StreamUnlock(pStream);
}

/**
 * Creates an AC'97 audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to create.
 * @param   u8Strm              Stream ID to assign AC'97 stream to.
 */
static int ichac97StreamCreate(PAC97STATE pThis, PAC97STREAM pStream, uint8_t u8Strm)
{
    RT_NOREF(pThis);
    AssertPtrReturn(pStream, VERR_INVALID_PARAMETER);
    /** @todo Validate u8Strm. */

    LogFunc(("[SD%RU8] pStream=%p\n", u8Strm, pStream));

    pStream->u8SD = u8Strm;

    int rc = RTCritSectInit(&pStream->State.CritSect);
    if (RT_SUCCESS(rc))
        rc = RTCircBufCreate(&pStream->State.pCircBuf, _4K); /** @todo Make this configurable. */

    return rc;
}

/**
 * Destroys an AC'97 audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to destroy.
 */
static void ichac97StreamDestroy(PAC97STATE pThis, PAC97STREAM pStream)
{
    LogFlowFunc(("[SD%RU8]\n", pStream->u8SD));

    int rc2 = RTCritSectDelete(&pStream->State.CritSect);
    AssertRC(rc2);

    if (pStream->State.pCircBuf)
    {
        RTCircBufDestroy(pStream->State.pCircBuf);
        pStream->State.pCircBuf = NULL;
    }

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
    rc2 = ichac97StreamAsyncIODestroy(pThis, pStream);
    AssertRC(rc2);
#else
    RT_NOREF(pThis);
#endif

    LogFlowFuncLeave();
}

/**
 * Destroys all AC'97 audio streams of the device.
 *
 * @param   pThis               AC'97 state.
 */
static void ichac97StreamsDestroy(PAC97STATE pThis)
{
    LogFlowFuncEnter();

    /*
     * Destroy all AC'97 streams.
     */

    ichac97StreamDestroy(pThis, &pThis->StreamLineIn);
    ichac97StreamDestroy(pThis, &pThis->StreamMicIn);
    ichac97StreamDestroy(pThis, &pThis->StreamOut);

    /*
     * Destroy all sinks.
     */

    PDMAUDIODESTSOURCE dstSrc;
    if (pThis->pSinkLineIn)
    {
        dstSrc.Source = PDMAUDIORECSOURCE_LINE;
        ichac97MixerRemoveDrvStreams(pThis, pThis->pSinkLineIn, PDMAUDIODIR_IN, dstSrc);

        AudioMixerSinkDestroy(pThis->pSinkLineIn);
        pThis->pSinkLineIn = NULL;
    }

    if (pThis->pSinkMicIn)
    {
        dstSrc.Source = PDMAUDIORECSOURCE_MIC;
        ichac97MixerRemoveDrvStreams(pThis, pThis->pSinkMicIn, PDMAUDIODIR_IN, dstSrc);

        AudioMixerSinkDestroy(pThis->pSinkMicIn);
        pThis->pSinkMicIn = NULL;
    }

    if (pThis->pSinkOut)
    {
        dstSrc.Dest = PDMAUDIOPLAYBACKDEST_FRONT;
        ichac97MixerRemoveDrvStreams(pThis, pThis->pSinkOut, PDMAUDIODIR_OUT, dstSrc);

        AudioMixerSinkDestroy(pThis->pSinkOut);
        pThis->pSinkOut = NULL;
    }
}

/**
 * Writes audio data from a mixer sink into an AC'97 stream's DMA buffer.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pDstStream          AC'97 stream to write to.
 * @param   pSrcMixSink         Mixer sink to get audio data to write from.
 * @param   cbToWrite           Number of bytes to write.
 * @param   pcbWritten          Number of bytes written. Optional.
 */
static int ichac97StreamWrite(PAC97STATE pThis, PAC97STREAM pDstStream, PAUDMIXSINK pSrcMixSink, uint32_t cbToWrite,
                              uint32_t *pcbWritten)
{
    RT_NOREF(pThis);
    AssertPtrReturn(pDstStream,  VERR_INVALID_POINTER);
    AssertPtrReturn(pSrcMixSink, VERR_INVALID_POINTER);
    AssertReturn(cbToWrite,      VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    PRTCIRCBUF pCircBuf = pDstStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    void *pvDst;
    size_t cbDst;

    uint32_t cbRead = 0;

    RTCircBufAcquireWriteBlock(pCircBuf, cbToWrite, &pvDst, &cbDst);

    if (cbDst)
    {
        int rc2 = AudioMixerSinkRead(pSrcMixSink, AUDMIXOP_COPY, pvDst, (uint32_t)cbDst, &cbRead);
        AssertRC(rc2);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
        RTFILE fh;
        RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ichac97StreamWrite.pcm",
                   RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
        RTFileWrite(fh, pvDst, cbRead, NULL);
        RTFileClose(fh);
#endif
    }

    RTCircBufReleaseWriteBlock(pCircBuf, cbRead);

    if (pcbWritten)
        *pcbWritten = cbRead;

    return VINF_SUCCESS;
}

/**
 * Reads audio data from an AC'97 stream's DMA buffer and writes into a specified mixer sink.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pSrcStream          AC'97 stream to read audio data from.
 * @param   pDstMixSink         Mixer sink to write audio data to.
 * @param   cbToRead            Number of bytes to read.
 * @param   pcbRead             Number of bytes read. Optional.
 */
static int ichac97StreamRead(PAC97STATE pThis, PAC97STREAM pSrcStream, PAUDMIXSINK pDstMixSink, uint32_t cbToRead,
                             uint32_t *pcbRead)
{
    RT_NOREF(pThis);
    AssertPtrReturn(pSrcStream,  VERR_INVALID_POINTER);
    AssertPtrReturn(pDstMixSink, VERR_INVALID_POINTER);
    AssertReturn(cbToRead,       VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;

    PRTCIRCBUF pCircBuf = pSrcStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    void *pvSrc;
    size_t cbSrc;

    while (cbToRead)
    {
        uint32_t cbWritten = 0;

        RTCircBufAcquireReadBlock(pCircBuf, cbToRead, &pvSrc, &cbSrc);

        if (cbSrc)
        {
#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
            RTFILE fh;
            RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97StreamRead.pcm",
                       RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
            RTFileWrite(fh, pvSrc, cbSrc, NULL);
            RTFileClose(fh);
#endif
            rc = AudioMixerSinkWrite(pDstMixSink, AUDMIXOP_COPY, pvSrc, (uint32_t)cbSrc, &cbWritten);
            if (RT_SUCCESS(rc))
            {
                Assert(cbWritten <= cbSrc);

                cbReadTotal += cbWritten;

                Assert(cbToRead >= cbWritten);
                cbToRead    -= cbWritten;
            }
        }

        RTCircBufReleaseReadBlock(pCircBuf, cbWritten);

        if (   !cbWritten
            || !RTCircBufUsed(pCircBuf))
               break;

        if (RT_FAILURE(rc))
            break;
    }

    if (pcbRead)
        *pcbRead = cbReadTotal;

    return rc;
}

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
/**
 * Asynchronous I/O thread for an AC'97 stream.
 * This will do the heavy lifting work for us as soon as it's getting notified by another thread.
 *
 * @returns IPRT status code.
 * @param   hThreadSelf         Thread handle.
 * @param   pvUser              User argument. Must be of type PAC97STREAMTHREADCTX.
 */
static DECLCALLBACK(int) ichac97StreamAsyncIOThread(RTTHREAD hThreadSelf, void *pvUser)
{
    PAC97STREAMTHREADCTX pCtx = (PAC97STREAMTHREADCTX)pvUser;
    AssertPtr(pCtx);

    PAC97STATE pThis = pCtx->pThis;
    AssertPtr(pThis);

    PAC97STREAM pStream = pCtx->pStream;
    AssertPtr(pStream);

    PAC97STREAMSTATEAIO pAIO = &pCtx->pStream->State.AIO;

    PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    PAUDMIXSINK pMixSink = ichac97IndexToSink(pThis, pStream->u8SD);
    AssertPtr(pMixSink);

    ASMAtomicXchgBool(&pAIO->fStarted, true);

    RTThreadUserSignal(hThreadSelf);

    LogFunc(("[SD%RU8] Started\n", pStream->u8SD));

    for (;;)
    {
        Log2Func(("[SD%RU8] Waiting ...\n", pStream->u8SD));

        int rc2 = RTSemEventWait(pAIO->Event, RT_INDEFINITE_WAIT);
        if (RT_FAILURE(rc2))
            break;

        if (ASMAtomicReadBool(&pAIO->fShutdown))
            break;

        rc2 = RTCritSectEnter(&pAIO->CritSect);
        if (RT_SUCCESS(rc2))
        {
            if (!pAIO->fEnabled)
            {
                RTCritSectLeave(&pAIO->CritSect);
                continue;
            }

            ichac97StreamUpdate(pThis, pStream, false /* fInTimer */);

            int rc3 = RTCritSectLeave(&pAIO->CritSect);
            AssertRC(rc3);
        }

        AssertRC(rc2);
    }

    LogFunc(("[SD%RU8] Ended\n", pStream->u8SD));

    ASMAtomicXchgBool(&pAIO->fStarted, false);

    return VINF_SUCCESS;
}

/**
 * Creates the async I/O thread for a specific AC'97 audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 audio stream to create the async I/O thread for.
 */
static int ichac97StreamAsyncIOCreate(PAC97STATE pThis, PAC97STREAM pStream)
{
    PAC97STREAMSTATEAIO pAIO = &pStream->State.AIO;

    int rc;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
    {
        pAIO->fShutdown = false;

        rc = RTSemEventCreate(&pAIO->Event);
        if (RT_SUCCESS(rc))
        {
            rc = RTCritSectInit(&pAIO->CritSect);
            if (RT_SUCCESS(rc))
            {
                AC97STREAMTHREADCTX Ctx = { pThis, pStream };

                char szThreadName[64];
                RTStrPrintf2(szThreadName, sizeof(szThreadName), "ac97AIO%RU8", pStream->u8SD);

                rc = RTThreadCreate(&pAIO->Thread, ichac97StreamAsyncIOThread, &Ctx,
                                    0, RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, szThreadName);
                if (RT_SUCCESS(rc))
                    rc = RTThreadUserWait(pAIO->Thread, 10 * 1000 /* 10s timeout */);
            }
        }
    }
    else
        rc = VINF_SUCCESS;

    LogFunc(("[SD%RU8] Returning %Rrc\n", pStream->u8SD, rc));
    return rc;
}

/**
 * Destroys the async I/O thread of a specific AC'97 audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 audio stream to destroy the async I/O thread for.
 */
static int ichac97StreamAsyncIODestroy(PAC97STATE pThis, PAC97STREAM pStream)
{
    PAC97STREAMSTATEAIO pAIO = &pStream->State.AIO;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
        return VINF_SUCCESS;

    ASMAtomicWriteBool(&pAIO->fShutdown, true);

    int rc = ichac97StreamAsyncIONotify(pThis, pStream);
    AssertRC(rc);

    int rcThread;
    rc = RTThreadWait(pAIO->Thread, 30 * 1000 /* 30s timeout */, &rcThread);
    LogFunc(("Async I/O thread ended with %Rrc (%Rrc)\n", rc, rcThread));

    if (RT_SUCCESS(rc))
    {
        rc = RTCritSectDelete(&pAIO->CritSect);
        AssertRC(rc);

        rc = RTSemEventDestroy(pAIO->Event);
        AssertRC(rc);

        pAIO->fStarted  = false;
        pAIO->fShutdown = false;
        pAIO->fEnabled  = false;
    }

    LogFunc(("[SD%RU8] Returning %Rrc\n", pStream->u8SD, rc));
    return rc;
}

/**
 * Lets the stream's async I/O thread know that there is some data to process.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to notify async I/O thread for.
 */
static int ichac97StreamAsyncIONotify(PAC97STATE pThis, PAC97STREAM pStream)
{
    RT_NOREF(pThis);

    LogFunc(("[SD%RU8]\n", pStream->u8SD));
    return RTSemEventSignal(pStream->State.AIO.Event);
}

/**
 * Locks the async I/O thread of a specific AC'97 audio stream.
 *
 * @param   pStream             AC'97 stream to lock async I/O thread for.
 */
static void ichac97StreamAsyncIOLock(PAC97STREAM pStream)
{
    PAC97STREAMSTATEAIO pAIO = &pStream->State.AIO;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
        return;

    int rc2 = RTCritSectEnter(&pAIO->CritSect);
    AssertRC(rc2);
}

/**
 * Unlocks the async I/O thread of a specific AC'97 audio stream.
 *
 * @param   pStream             AC'97 stream to unlock async I/O thread for.
 */
static void ichac97StreamAsyncIOUnlock(PAC97STREAM pStream)
{
    PAC97STREAMSTATEAIO pAIO = &pStream->State.AIO;

    if (!ASMAtomicReadBool(&pAIO->fStarted))
        return;

    int rc2 = RTCritSectLeave(&pAIO->CritSect);
    AssertRC(rc2);
}

/**
 * Enables (resumes) or disables (pauses) the async I/O thread.
 *
 * @param   pStream             AC'97 stream to enable/disable async I/O thread for.
 * @param   fEnable             Whether to enable or disable the I/O thread.
 *
 * @remarks Does not do locking.
 */
static void ichac97StreamAsyncIOEnable(PAC97STREAM pStream, bool fEnable)
{
    PAC97STREAMSTATEAIO pAIO = &pStream->State.AIO;
    ASMAtomicXchgBool(&pAIO->fEnabled, fEnable);
}
#endif /* VBOX_WITH_AUDIO_AC97_ASYNC_IO */

/**
 * Updates an AC'97 stream by doing its required data transfers.
 * The host sink(s) set the overall pace.
 *
 * This routine is called by both, the synchronous and the asynchronous, implementations.
 *
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to update.
 * @param   fInTimer            Whether to this function was called from the timer
 *                              context or an asynchronous I/O stream thread (if supported).
 */
static void ichac97StreamUpdate(PAC97STATE pThis, PAC97STREAM pStream, bool fInTimer)
{
    PAUDMIXSINK pSink = ichac97IndexToSink(pThis, pStream->u8SD);
    AssertPtr(pSink);

    if (!AudioMixerSinkIsActive(pSink)) /* No sink available? Bail out. */
        return;

    int rc2;

    if (pStream->u8SD == AC97SOUNDSOURCE_PO_INDEX) /* Output (SDO). */
    {
        /* Is the AC'97 stream ready to be written (guest output data) to? If so, by how much? */
        const uint32_t cbFree = ichac97StreamGetFree(pStream);

        if (   fInTimer
            && cbFree)
        {
            Log3Func(("[SD%RU8] cbFree=%RU32\n", pStream->u8SD, cbFree));

            /* Do the DMA transfer. */
            rc2 = ichac97StreamTransfer(pThis, pStream, cbFree);
            AssertRC(rc2);
        }

        /* How much (guest output) data is available at the moment for the AC'97 stream? */
        uint32_t cbUsed = ichac97StreamGetUsed(pStream);

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
        if (   fInTimer
            && cbUsed)
        {
            rc2 = ichac97StreamAsyncIONotify(pThis, pStream);
            AssertRC(rc2);
        }
        else
        {
#endif
            const uint32_t cbSinkWritable = AudioMixerSinkGetWritable(pSink);

            /* Do not write more than the sink can hold at the moment.
             * The host sets the overall pace. */
            if (cbUsed > cbSinkWritable)
                cbUsed = cbSinkWritable;

            if (cbUsed)
            {
                /* Read (guest output) data and write it to the stream's sink. */
                uint32_t cbRead;
                rc2 = ichac97StreamRead(pThis, pStream, pSink, cbUsed, &cbRead);
                AssertRC(rc2);

                AssertMsg(cbUsed == cbRead, ("[SD%RU8] %RU32 bytes announced to be ready to read but %RU32 bytes read\n",
                                             pStream->u8SD, cbUsed, cbRead));
            }

            /* When running synchronously, update the associated sink here.
             * Otherwise this will be done in the device timer. */
            rc2 = AudioMixerSinkUpdate(pSink);
            AssertRC(rc2);

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
        }
#endif
    }
    else /* Input (SDI). */
    {
#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
        if (fInTimer)
        {
            rc2 = ichac97StreamAsyncIONotify(pThis, pStream);
            AssertRC(rc2);
        }
        else
        {
#endif
            rc2 = AudioMixerSinkUpdate(pSink);
            AssertRC(rc2);

            /* Is the sink ready to be read (host input data) from? If so, by how much? */
            const uint32_t cbReadable = AudioMixerSinkGetReadable(pSink);

            /* How much (guest input) data is free at the moment? */
            uint32_t cbFree = ichac97StreamGetFree(pStream);

            Log3Func(("[SD%RU8] cbReadable=%RU32, cbFree=%RU32\n", pStream->u8SD, cbReadable, cbFree));

            /* Do not read more than the sink can provide at the moment.
             * The host sets the overall pace. */
            if (cbFree > cbReadable)
                cbFree = cbReadable;

            if (cbFree)
            {
                /* Write (guest input) data to the stream which was read from stream's sink before. */
                rc2 = ichac97StreamWrite(pThis, pStream, pSink, cbFree, NULL /* pcbWritten */);
                AssertRC(rc2);
            }
#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
        }
#endif

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
        if (fInTimer)
        {
#endif
            const uint32_t cbToTransfer = ichac97StreamGetUsed(pStream);
            if (cbToTransfer)
            {
                /* When running synchronously, do the DMA data transfers here.
                 * Otherwise this will be done in the stream's async I/O thread. */
                rc2 = ichac97StreamTransfer(pThis, pStream, cbToTransfer);
                AssertRC(rc2);
            }
#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
        }
#endif
    }
}

/**
 * Sets a AC'97 mixer control to a specific value.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   uMixerIdx           Mixer control to set value for.
 * @param   uVal                Value to set.
 */
static void ichac97MixerSet(PAC97STATE pThis, uint8_t uMixerIdx, uint16_t uVal)
{
    if (size_t(uMixerIdx + 2) > sizeof(pThis->mixer_data))
    {
        AssertMsgFailed(("Index %RU8 out of bounds(%zu)\n", uMixerIdx, sizeof(pThis->mixer_data)));
        return;
    }

    pThis->mixer_data[uMixerIdx + 0] = RT_LO_U8(uVal);
    pThis->mixer_data[uMixerIdx + 1] = RT_HI_U8(uVal);
}

/**
 * Gets a value from a specific AC'97 mixer control.
 *
 * @returns Retrieved mixer control value.
 * @param   pThis               AC'97 state.
 * @param   uMixerIdx           Mixer control to get value for.
 */
static uint16_t ichac97MixerGet(PAC97STATE pThis, uint32_t uMixerIdx)
{
    uint16_t uVal;

    if (size_t(uMixerIdx + 2) > sizeof(pThis->mixer_data))
    {
        AssertMsgFailed(("Index %RU8 out of bounds (%zu)\n", uMixerIdx, sizeof(pThis->mixer_data)));
        uVal = UINT16_MAX;
    }
    else
        uVal = RT_MAKE_U16(pThis->mixer_data[uMixerIdx + 0], pThis->mixer_data[uMixerIdx + 1]);

    return uVal;
}

/**
 * Retrieves a specific driver stream of a AC'97 driver.
 *
 * @returns Pointer to driver stream if found, or NULL if not found.
 * @param   pThis               AC'97 state.
 * @param   pDrv                Driver to retrieve driver stream for.
 * @param   enmDir              Stream direction to retrieve.
 * @param   dstSrc              Stream destination / source to retrieve.
 */
static PAC97DRIVERSTREAM ichac97MixerGetDrvStream(PAC97STATE pThis, PAC97DRIVER pDrv,
                                                  PDMAUDIODIR enmDir, PDMAUDIODESTSOURCE dstSrc)
{
    RT_NOREF(pThis);

    PAC97DRIVERSTREAM pDrvStream = NULL;

    if (enmDir == PDMAUDIODIR_IN)
    {
        LogFunc(("enmRecSource=%d\n", dstSrc.Source));

        switch (dstSrc.Source)
        {
            case PDMAUDIORECSOURCE_LINE:
                pDrvStream = &pDrv->LineIn;
                break;
            case PDMAUDIORECSOURCE_MIC:
                pDrvStream = &pDrv->MicIn;
                break;
            default:
                AssertFailed();
                break;
        }
    }
    else if (enmDir == PDMAUDIODIR_OUT)
    {
        LogFunc(("enmPlaybackDest=%d\n", dstSrc.Dest));

        switch (dstSrc.Dest)
        {
            case PDMAUDIOPLAYBACKDEST_FRONT:
                pDrvStream = &pDrv->Out;
                break;
            default:
                AssertFailed();
                break;
        }
    }
    else
        AssertFailed();

    return pDrvStream;
}

/**
 * Adds a driver stream to a specific mixer sink.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pMixSink            Mixer sink to add driver stream to.
 * @param   pCfg                Stream configuration to use.
 * @param   pDrv                Driver stream to add.
 */
static int ichac97MixerAddDrvStream(PAC97STATE pThis, PAUDMIXSINK pMixSink, PPDMAUDIOSTREAMCFG pCfg, PAC97DRIVER pDrv)
{
    AssertPtrReturn(pThis,    VERR_INVALID_POINTER);
    AssertPtrReturn(pMixSink, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,     VERR_INVALID_POINTER);

    PPDMAUDIOSTREAMCFG pStreamCfg = DrvAudioHlpStreamCfgDup(pCfg);
    if (!pStreamCfg)
        return VERR_NO_MEMORY;

    if (!RTStrPrintf(pStreamCfg->szName, sizeof(pStreamCfg->szName), "%s", pCfg->szName))
    {
        RTMemFree(pStreamCfg);
        return VERR_BUFFER_OVERFLOW;
    }

    LogFunc(("[LUN#%RU8] %s\n", pDrv->uLUN, pStreamCfg->szName));

    int rc;

    PAC97DRIVERSTREAM pDrvStream = ichac97MixerGetDrvStream(pThis, pDrv, pStreamCfg->enmDir, pStreamCfg->DestSource);
    if (pDrvStream)
    {
        AssertMsg(pDrvStream->pMixStrm == NULL, ("[LUN#%RU8] Driver stream already present when it must not\n", pDrv->uLUN));

        PAUDMIXSTREAM pMixStrm;
        rc = AudioMixerSinkCreateStream(pMixSink, pDrv->pConnector, pStreamCfg, 0 /* fFlags */, &pMixStrm);
        if (RT_SUCCESS(rc))
        {
            rc = AudioMixerSinkAddStream(pMixSink, pMixStrm);
            LogFlowFunc(("LUN#%RU8: Created stream \"%s\", rc=%Rrc\n", pDrv->uLUN, pCfg->szName, rc));
        }

        if (RT_SUCCESS(rc))
            pDrvStream->pMixStrm = pMixStrm;
    }
    else
        rc = VERR_INVALID_PARAMETER;

    if (pStreamCfg)
    {
        RTMemFree(pStreamCfg);
        pStreamCfg = NULL;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Adds all current driver streams to a specific mixer sink.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pMixSink            Mixer sink to add stream to.
 * @param   pCfg                Stream configuration to use.
 */
static int ichac97MixerAddDrvStreams(PAC97STATE pThis, PAUDMIXSINK pMixSink, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis,    VERR_INVALID_POINTER);
    AssertPtrReturn(pMixSink, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,     VERR_INVALID_POINTER);

    if (!DrvAudioHlpStreamCfgIsValid(pCfg))
        return VERR_INVALID_PARAMETER;

    int rc = AudioMixerSinkSetFormat(pMixSink, &pCfg->Props);
    if (RT_FAILURE(rc))
        return rc;

    PAC97DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, AC97DRIVER, Node)
    {
        int rc2 = ichac97MixerAddDrvStream(pThis, pMixSink, pCfg, pDrv);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Removes a driver stream from a specific mixer sink.
 *
 * @param   pThis               AC'97 state.
 * @param   pMixSink            Mixer sink to remove audio streams from.
 * @param   enmDir              Stream direction to remove.
 * @param   dstSrc              Stream destination / source to remove.
 * @param   pDrv                Driver stream to remove.
 */
static void ichac97MixerRemoveDrvStream(PAC97STATE pThis, PAUDMIXSINK pMixSink,
                                        PDMAUDIODIR enmDir, PDMAUDIODESTSOURCE dstSrc, PAC97DRIVER pDrv)
{
    AssertPtrReturnVoid(pThis);
    AssertPtrReturnVoid(pMixSink);

    PAC97DRIVERSTREAM pDrvStream = ichac97MixerGetDrvStream(pThis, pDrv, enmDir, dstSrc);
    if (pDrvStream)
    {
        if (pDrvStream->pMixStrm)
        {
            AudioMixerSinkRemoveStream(pMixSink, pDrvStream->pMixStrm);

            AudioMixerStreamDestroy(pDrvStream->pMixStrm);
            pDrvStream->pMixStrm = NULL;
        }
    }
}

/**
 * Removes all driver streams from a specific mixer sink.
 *
 * @param   pThis               AC'97 state.
 * @param   pMixSink            Mixer sink to remove audio streams from.
 * @param   enmDir              Stream direction to remove.
 * @param   dstSrc              Stream destination / source to remove.
 */
static void ichac97MixerRemoveDrvStreams(PAC97STATE pThis, PAUDMIXSINK pMixSink,
                                         PDMAUDIODIR enmDir, PDMAUDIODESTSOURCE dstSrc)
{
    AssertPtrReturnVoid(pThis);
    AssertPtrReturnVoid(pMixSink);

    PAC97DRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, AC97DRIVER, Node)
        ichac97MixerRemoveDrvStream(pThis, pMixSink, enmDir, dstSrc, pDrv);
}

/**
 * Opens an AC'97 stream with its current mixer settings.
 *
 * This will open an AC'97 stream with 2 (stereo) channels, 16-bit samples and
 * the last set sample rate in the AC'97 mixer for this stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 Stream to open.
 */
static int ichac97StreamOpen(PAC97STATE pThis, PAC97STREAM pStream)
{
    int rc = VINF_SUCCESS;

    LogFunc(("[SD%RU8]\n", pStream->u8SD));

    RT_ZERO(pStream->State.Cfg);

    PPDMAUDIOSTREAMCFG pCfg     = &pStream->State.Cfg;
    PAUDMIXSINK        pMixSink = NULL;

    switch (pStream->u8SD)
    {
        case AC97SOUNDSOURCE_PI_INDEX:
        {
            pCfg->Props.uHz         = ichac97MixerGet(pThis, AC97_PCM_LR_ADC_Rate);
            pCfg->enmDir            = PDMAUDIODIR_IN;
            pCfg->DestSource.Source = PDMAUDIORECSOURCE_LINE;
            pCfg->enmLayout         = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            RTStrPrintf2(pCfg->szName, sizeof(pCfg->szName), "Line-In");

            pMixSink                = pThis->pSinkLineIn;
            break;
        }

        case AC97SOUNDSOURCE_MC_INDEX:
        {
            pCfg->Props.uHz         = ichac97MixerGet(pThis, AC97_MIC_ADC_Rate);
            pCfg->enmDir            = PDMAUDIODIR_IN;
            pCfg->DestSource.Source = PDMAUDIORECSOURCE_MIC;
            pCfg->enmLayout         = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            RTStrPrintf2(pCfg->szName, sizeof(pCfg->szName), "Mic-In");

            pMixSink                = pThis->pSinkMicIn;
            break;
        }

        case AC97SOUNDSOURCE_PO_INDEX:
        {
            pCfg->Props.uHz         = ichac97MixerGet(pThis, AC97_PCM_Front_DAC_Rate);
            pCfg->enmDir            = PDMAUDIODIR_OUT;
            pCfg->DestSource.Dest   = PDMAUDIOPLAYBACKDEST_FRONT;
            pCfg->enmLayout         = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            RTStrPrintf2(pCfg->szName, sizeof(pCfg->szName), "Output");

            pMixSink                = pThis->pSinkOut;
            break;
        }

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        ichac97MixerRemoveDrvStreams(pThis, pMixSink, pCfg->enmDir, pCfg->DestSource);

        if (pCfg->Props.uHz)
        {
            Assert(pCfg->enmDir != PDMAUDIODIR_UNKNOWN);

            pCfg->Props.cChannels = 2;
            pCfg->Props.cBits     = 16;
            pCfg->Props.fSigned   = true;
            pCfg->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfg->Props.cBits, pCfg->Props.cChannels);

            rc = ichac97MixerAddDrvStreams(pThis, pMixSink, pCfg);
        }
    }

    LogFlowFunc(("[SD%RU8] rc=%Rrc\n", pStream->u8SD, rc));
    return rc;
}

/**
 * Closes an AC'97 stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to close.
 */
static int ichac97StreamClose(PAC97STATE pThis, PAC97STREAM pStream)
{
    RT_NOREF(pThis);
    RT_NOREF(pStream);

    LogFlowFunc(("[SD%RU8]\n", pStream->u8SD));

    return VINF_SUCCESS;
}

/**
 * Re-opens (that is, closes and opens again) an AC'97 stream on the backend
 * side with the current AC'97 mixer settings for this stream.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 device state.
 * @param   pStream             AC'97 stream to re-open.
 */
static int ichac97StreamReOpen(PAC97STATE pThis, PAC97STREAM pStream)
{
    LogFlowFunc(("[SD%RU8]\n", pStream->u8SD));

    int rc = ichac97StreamClose(pThis, pStream);
    if (RT_SUCCESS(rc))
        rc = ichac97StreamOpen(pThis, pStream);

    return rc;
}

/**
 * Locks an AC'97 stream for serialized access.
 *
 * @returns IPRT status code.
 * @param   pStream             AC'97 stream to lock.
 */
static void ichac97StreamLock(PAC97STREAM pStream)
{
    AssertPtrReturnVoid(pStream);
    int rc2 = RTCritSectEnter(&pStream->State.CritSect);
    AssertRC(rc2);
}

/**
 * Unlocks a formerly locked AC'97 stream.
 *
 * @returns IPRT status code.
 * @param   pStream             AC'97 stream to unlock.
 */
static void ichac97StreamUnlock(PAC97STREAM pStream)
{
    AssertPtrReturnVoid(pStream);
    int rc2 = RTCritSectLeave(&pStream->State.CritSect);
    AssertRC(rc2);
}

/**
 * Retrieves the available size of (buffered) audio data (in bytes) of a given AC'97 stream.
 *
 * @returns Available data (in bytes).
 * @param   pStream             AC'97 stream to retrieve size for.
 */
static uint32_t ichac97StreamGetUsed(PAC97STREAM pStream)
{
    AssertPtrReturn(pStream, 0);

    if (!pStream->State.pCircBuf)
        return 0;

    return (uint32_t)RTCircBufUsed(pStream->State.pCircBuf);
}

/**
 * Retrieves the free size of audio data (in bytes) of a given AC'97 stream.
 *
 * @returns Free data (in bytes).
 * @param   pStream             AC'97 stream to retrieve size for.
 */
static uint32_t ichac97StreamGetFree(PAC97STREAM pStream)
{
    AssertPtrReturn(pStream, 0);

    if (!pStream->State.pCircBuf)
        return 0;

    return (uint32_t)RTCircBufFree(pStream->State.pCircBuf);
}

/**
 * Sets the volume of a specific AC'97 mixer control.
 *
 * This currently only supports attenuation -- gain support is currently not implemented.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   index               AC'97 mixer index to set volume for.
 * @param   enmMixerCtl         Corresponding audio mixer sink.
 * @param   uVal                Volume value to set.
 */
static int ichac97MixerSetVolume(PAC97STATE pThis, int index, PDMAUDIOMIXERCTL enmMixerCtl, uint32_t uVal)
{
    /*
     * From AC'97 SoundMax Codec AD1981A/AD1981B:
     * "Because AC '97 defines 6-bit volume registers, to maintain compatibility whenever the
     *  D5 or D13 bits are set to 1, their respective lower five volume bits are automatically
     *  set to 1 by the Codec logic. On readback, all lower 5 bits will read ones whenever
     *  these bits are set to 1."
     *
     * Linux ALSA depends on this behavior.
     */
    /// @todo Does this apply to anything other than the master volume control?
    if (uVal & RT_BIT(5))  /* D5 bit set? */
        uVal |= RT_BIT(4) | RT_BIT(3) | RT_BIT(2) | RT_BIT(1) | RT_BIT(0);
    if (uVal & RT_BIT(13)) /* D13 bit set? */
        uVal |= RT_BIT(12) | RT_BIT(11) | RT_BIT(10) | RT_BIT(9) | RT_BIT(8);

    const bool    fCtlMuted    = (uVal >> AC97_BARS_VOL_MUTE_SHIFT) & 1;
          uint8_t uCtlAttLeft  = (uVal >> 8) & AC97_BARS_VOL_MASK;
          uint8_t uCtlAttRight = uVal & AC97_BARS_VOL_MASK;

    /* For the master and headphone volume, 0 corresponds to 0dB attenuation. For the other
     * volume controls, 0 means 12dB gain and 8 means unity gain.
     */
    if (index != AC97_Master_Volume_Mute && index != AC97_Headphone_Volume_Mute)
    {
#ifndef VBOX_WITH_AC97_GAIN_SUPPORT
        /* NB: Currently there is no gain support, only attenuation. */
        uCtlAttLeft  = uCtlAttLeft  < 8 ? 0 : uCtlAttLeft  - 8;
        uCtlAttRight = uCtlAttRight < 8 ? 0 : uCtlAttRight - 8;
#endif
    }
    Assert(uCtlAttLeft  <= 255 / AC97_DB_FACTOR);
    Assert(uCtlAttRight <= 255 / AC97_DB_FACTOR);

    LogFunc(("index=0x%x, uVal=%RU32, enmMixerCtl=%RU32\n", index, uVal, enmMixerCtl));
    LogFunc(("uCtlAttLeft=%RU8, uCtlAttRight=%RU8 ", uCtlAttLeft, uCtlAttRight));

    /*
     * For AC'97 volume controls, each additional step means -1.5dB attenuation with
     * zero being maximum. In contrast, we're internally using 255 (PDMAUDIO_VOLUME_MAX)
     * steps, each -0.375dB, where 0 corresponds to -96dB and 255 corresponds to 0dB.
     */
    uint8_t lVol = PDMAUDIO_VOLUME_MAX - uCtlAttLeft  * AC97_DB_FACTOR;
    uint8_t rVol = PDMAUDIO_VOLUME_MAX - uCtlAttRight * AC97_DB_FACTOR;

    Log(("-> fMuted=%RTbool, lVol=%RU8, rVol=%RU8\n", fCtlMuted, lVol, rVol));

    int rc = VINF_SUCCESS;

    if (pThis->pMixer) /* Device can be in reset state, so no mixer available. */
    {
        PDMAUDIOVOLUME Vol   = { fCtlMuted, lVol, rVol };
        PAUDMIXSINK    pSink = NULL;

        switch (enmMixerCtl)
        {
            case PDMAUDIOMIXERCTL_VOLUME_MASTER:
                rc = AudioMixerSetMasterVolume(pThis->pMixer, &Vol);
                break;

            case PDMAUDIOMIXERCTL_FRONT:
                pSink = pThis->pSinkOut;
                break;

            case PDMAUDIOMIXERCTL_MIC_IN:
                pSink = pThis->pSinkMicIn;
                break;

            case PDMAUDIOMIXERCTL_LINE_IN:
                pSink = pThis->pSinkLineIn;
                break;

            default:
                AssertFailed();
                rc = VERR_NOT_SUPPORTED;
                break;
        }

        if (pSink)
            rc = AudioMixerSinkSetVolume(pSink, &Vol);
    }

    ichac97MixerSet(pThis, index, uVal);

    if (RT_FAILURE(rc))
        LogFlowFunc(("Failed with %Rrc\n", rc));

    return rc;
}

/**
 * Converts an AC'97 recording source index to a PDM audio recording source.
 *
 * @returns PDM audio recording source.
 * @param   uIdx                AC'97 index to convert.
 */
static PDMAUDIORECSOURCE ichac97IdxToRecSource(uint8_t uIdx)
{
    switch (uIdx)
    {
        case AC97_REC_MIC:     return PDMAUDIORECSOURCE_MIC;
        case AC97_REC_CD:      return PDMAUDIORECSOURCE_CD;
        case AC97_REC_VIDEO:   return PDMAUDIORECSOURCE_VIDEO;
        case AC97_REC_AUX:     return PDMAUDIORECSOURCE_AUX;
        case AC97_REC_LINE_IN: return PDMAUDIORECSOURCE_LINE;
        case AC97_REC_PHONE:   return PDMAUDIORECSOURCE_PHONE;
        default:
            break;
    }

    LogFlowFunc(("Unknown record source %d, using MIC\n", uIdx));
    return PDMAUDIORECSOURCE_MIC;
}

/**
 * Converts a PDM audio recording source to an AC'97 recording source index.
 *
 * @returns AC'97 recording source index.
 * @param   enmRecSrc           PDM audio recording source to convert.
 */
static uint8_t ichac97RecSourceToIdx(PDMAUDIORECSOURCE enmRecSrc)
{
    switch (enmRecSrc)
    {
        case PDMAUDIORECSOURCE_MIC:     return AC97_REC_MIC;
        case PDMAUDIORECSOURCE_CD:      return AC97_REC_CD;
        case PDMAUDIORECSOURCE_VIDEO:   return AC97_REC_VIDEO;
        case PDMAUDIORECSOURCE_AUX:     return AC97_REC_AUX;
        case PDMAUDIORECSOURCE_LINE:    return AC97_REC_LINE_IN;
        case PDMAUDIORECSOURCE_PHONE:   return AC97_REC_PHONE;
        default:
            break;
    }

    LogFlowFunc(("Unknown audio recording source %d using MIC\n", enmRecSrc));
    return AC97_REC_MIC;
}

/**
 * Retrieves an AC'97 audio stream from an AC'97 stream index.
 *
 * @returns Pointer to AC'97 audio stream if found, or NULL if not found / invalid.
 * @param   pThis               AC'97 state.
 * @param   uIdx                AC'97 stream index to retrieve AC'97 audio stream for.
 */
DECLINLINE(PAC97STREAM) ichac97GetStreamFromIdx(PAC97STATE pThis, uint32_t uIdx)
{
    switch (uIdx)
    {
        case AC97SOUNDSOURCE_PI_INDEX: return &pThis->StreamLineIn;
        case AC97SOUNDSOURCE_MC_INDEX: return &pThis->StreamMicIn;
        case AC97SOUNDSOURCE_PO_INDEX: return &pThis->StreamOut;
        default:                       break;
    }

    return NULL;
}

/**
 * Performs an AC'97 mixer record select to switch to a different recording
 * source.
 *
 * @param   pThis               AC'97 state.
 * @param   val                 AC'97 recording source index to set.
 */
static void ichac97MixerRecordSelect(PAC97STATE pThis, uint32_t val)
{
    uint8_t rs = val & AC97_REC_MASK;
    uint8_t ls = (val >> 8) & AC97_REC_MASK;
    PDMAUDIORECSOURCE ars = ichac97IdxToRecSource(rs);
    PDMAUDIORECSOURCE als = ichac97IdxToRecSource(ls);
    rs = ichac97RecSourceToIdx(ars);
    ls = ichac97RecSourceToIdx(als);
    ichac97MixerSet(pThis, AC97_Record_Select, rs | (ls << 8));
}

/**
 * Resets the AC'97 mixer.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 */
static int ichac97MixerReset(PAC97STATE pThis)
{
    AssertPtrReturn(pThis, VERR_INVALID_PARAMETER);

    LogFlowFuncEnter();

    RT_ZERO(pThis->mixer_data);

    /* Note: Make sure to reset all registers first before bailing out on error. */

    ichac97MixerSet(pThis, AC97_Reset                   , 0x0000); /* 6940 */
    ichac97MixerSet(pThis, AC97_Master_Volume_Mono_Mute , 0x8000);
    ichac97MixerSet(pThis, AC97_PC_BEEP_Volume_Mute     , 0x0000);

    ichac97MixerSet(pThis, AC97_Phone_Volume_Mute       , 0x8008);
    ichac97MixerSet(pThis, AC97_Mic_Volume_Mute         , 0x8008);
    ichac97MixerSet(pThis, AC97_CD_Volume_Mute          , 0x8808);
    ichac97MixerSet(pThis, AC97_Aux_Volume_Mute         , 0x8808);
    ichac97MixerSet(pThis, AC97_Record_Gain_Mic_Mute    , 0x8000);
    ichac97MixerSet(pThis, AC97_General_Purpose         , 0x0000);
    ichac97MixerSet(pThis, AC97_3D_Control              , 0x0000);
    ichac97MixerSet(pThis, AC97_Powerdown_Ctrl_Stat     , 0x000f);

    ichac97MixerSet(pThis, AC97_Extended_Audio_ID       , 0x0809);
    ichac97MixerSet(pThis, AC97_Extended_Audio_Ctrl_Stat, 0x0009);
    ichac97MixerSet(pThis, AC97_PCM_Front_DAC_Rate      , 0xbb80);
    ichac97MixerSet(pThis, AC97_PCM_Surround_DAC_Rate   , 0xbb80);
    ichac97MixerSet(pThis, AC97_PCM_LFE_DAC_Rate        , 0xbb80);
    ichac97MixerSet(pThis, AC97_PCM_LR_ADC_Rate         , 0xbb80);
    ichac97MixerSet(pThis, AC97_MIC_ADC_Rate            , 0xbb80);

    if (pThis->uCodecModel == AC97_CODEC_AD1980)
    {
        /* Analog Devices 1980 (AD1980) */
        ichac97MixerSet(pThis, AC97_Reset                   , 0x0010); /* Headphones. */
        ichac97MixerSet(pThis, AC97_Vendor_ID1              , 0x4144);
        ichac97MixerSet(pThis, AC97_Vendor_ID2              , 0x5370);
        ichac97MixerSet(pThis, AC97_Headphone_Volume_Mute   , 0x8000);
    }
    else if (pThis->uCodecModel == AC97_CODEC_AD1981B)
    {
        /* Analog Devices 1981B (AD1981B) */
        ichac97MixerSet(pThis, AC97_Vendor_ID1              , 0x4144);
        ichac97MixerSet(pThis, AC97_Vendor_ID2              , 0x5374);
    }
    else
    {
        /* Sigmatel 9700 (STAC9700) */
        ichac97MixerSet(pThis, AC97_Vendor_ID1              , 0x8384);
        ichac97MixerSet(pThis, AC97_Vendor_ID2              , 0x7600); /* 7608 */
    }
    ichac97MixerRecordSelect(pThis, 0);

    /* The default value is 8000h, which corresponds to 0 dB attenuation with mute on. */
    ichac97MixerSetVolume(pThis, AC97_Master_Volume_Mute,  PDMAUDIOMIXERCTL_VOLUME_MASTER, 0x8000);

    /* The default value for stereo registers is 8808h, which corresponds to 0 dB gain with mute on.*/
    ichac97MixerSetVolume(pThis, AC97_PCM_Out_Volume_Mute, PDMAUDIOMIXERCTL_FRONT,         0x8808);
    ichac97MixerSetVolume(pThis, AC97_Line_In_Volume_Mute, PDMAUDIOMIXERCTL_LINE_IN,       0x8808);
    ichac97MixerSetVolume(pThis, AC97_Mic_Volume_Mute,     PDMAUDIOMIXERCTL_MIC_IN,        0x8808);

    return VINF_SUCCESS;
}

/* Unused */
#if 0
static void ichac97WriteBUP(PAC97STATE pThis, uint32_t cbElapsed)
{
    LogFlowFunc(("cbElapsed=%RU32\n", cbElapsed));

    if (!(pThis->bup_flag & BUP_SET))
    {
        if (pThis->bup_flag & BUP_LAST)
        {
            unsigned int i;
            uint32_t *p = (uint32_t*)pThis->silence;
            for (i = 0; i < sizeof(pThis->silence) / 4; i++) /** @todo r=andy Assumes 16-bit samples, stereo. */
                *p++ = pThis->last_samp;
        }
        else
            RT_ZERO(pThis->silence);

        pThis->bup_flag |= BUP_SET;
    }

    while (cbElapsed)
    {
        uint32_t cbToWrite = RT_MIN(cbElapsed, (uint32_t)sizeof(pThis->silence));
        uint32_t cbWrittenToStream;

        int rc2 = AudioMixerSinkWrite(pThis->pSinkOut, AUDMIXOP_COPY,
                                      pThis->silence, cbToWrite, &cbWrittenToStream);
        if (RT_SUCCESS(rc2))
        {
            if (cbWrittenToStream < cbToWrite) /* Lagging behind? */
                LogFlowFunc(("Warning: Only written %RU32 / %RU32 bytes, expect lags\n", cbWrittenToStream, cbToWrite));
        }

        /* Always report all data as being written;
         * backends who were not able to catch up have to deal with it themselves. */
        Assert(cbElapsed >= cbToWrite);
        cbElapsed -= cbToWrite;
    }
}
#endif /* Unused */

#ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
/**
 * Starts the internal audio device timer.
 *
 * @return  IPRT status code.
 * @param   pThis               AC'97 state.
 */
static int  ichac97TimerStart(PAC97STATE pThis)
{
    LogFlowFuncEnter();

    DEVAC97_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    AssertPtr(pThis->pTimer);

    if (!pThis->fTimerActive)
    {
        LogRel2(("AC97: Starting transfers\n"));

        pThis->fTimerActive = true;

        /* Start transfers. */
        ichac97TimerMain(pThis);
    }

    DEVAC97_UNLOCK_BOTH(pThis);

    return VINF_SUCCESS;
}

/**
 * Starts the internal audio device timer (if not started yet).
 *
 * @return  IPRT status code.
 * @param   pThis               AC'97 state.
 */
static int ichac97TimerMaybeStart(PAC97STATE pThis)
{
    LogFlowFuncEnter();

    if (!pThis->pTimer)
        return VERR_WRONG_ORDER;

    pThis->cStreamsActive++;

    /* Only start the timer at the first active stream. */
    if (pThis->cStreamsActive == 1)
        return ichac97TimerStart(pThis);

    return VINF_SUCCESS;
}

/**
 * Stops the internal audio device timer.
 *
 * @return  IPRT status code.
 * @param   pThis               AC'97 state.
 */
static int ichac97TimerStop(PAC97STATE pThis)
{
    LogFlowFuncEnter();

    if (!pThis->pTimer) /* Only can happen on device construction time, so no locking needed here. */
        return VINF_SUCCESS;

    DEVAC97_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    if (pThis->fTimerActive)
    {
        LogRel2(("AC97: Stopping transfers ...\n"));

        pThis->fTimerActive = false;

        /* Note: Do not stop the timer via TMTimerStop() here, as there still might
         *       be queued audio data which needs to be handled (e.g. played back) first
         *       before actually stopping the timer for good. */
    }

    DEVAC97_UNLOCK_BOTH(pThis);

    return VINF_SUCCESS;
}

/**
 * Decreases the active AC'97 streams count by one and
 * then checks if the internal audio device timer can be
 * stopped.
 *
 * @return  IPRT status code.
 * @param   pThis               AC'97 state.
 */
static int ichac97TimerMaybeStop(PAC97STATE pThis)
{
    LogFlowFuncEnter();

    if (!pThis->pTimer)
        return VERR_WRONG_ORDER;

    if (pThis->cStreamsActive) /* Function can be called mupltiple times. */
    {
        pThis->cStreamsActive--;

        if (pThis->cStreamsActive == 0)
            return ichac97TimerStop(pThis);
    }

    return VINF_SUCCESS;
}

/**
 * Main routine for the device timer.
 *
 * @param   pThis               AC'97 state.
 */
static void ichac97TimerMain(PAC97STATE pThis)
{
    STAM_PROFILE_START(&pThis->StatTimer, a);

    DEVAC97_LOCK_BOTH_RETURN_VOID(pThis);

    uint64_t cTicksNow = TMTimerGet(pThis->pTimer);

    /* Update current time timestamp. */
    pThis->uTimerTS = cTicksNow;

    /* Flag indicating whether to kick the timer again for the next DMA transfer or sink processing. */
    bool fKickTimer = false;

    ichac97DoTransfers(pThis);

    /* Do we need to kick the timer again? */
    if (   AudioMixerSinkIsActive(ichac97IndexToSink(pThis, pThis->StreamLineIn.u8SD))
        || AudioMixerSinkIsActive(ichac97IndexToSink(pThis, pThis->StreamMicIn.u8SD))
        || AudioMixerSinkIsActive(ichac97IndexToSink(pThis, pThis->StreamOut.u8SD)))
    {
        fKickTimer = true;
    }

    if (   ASMAtomicReadBool(&pThis->fTimerActive)
        || fKickTimer)
    {
        /* Kick the timer again. */
        uint64_t cTicks = pThis->cTimerTicks;
        /** @todo adjust cTicks down by now much cbOutMin represents. */
        TMTimerSet(pThis->pTimer, cTicksNow + cTicks);
    }
    else
        LogRel2(("AC97: Stopped transfers\n"));

    DEVAC97_UNLOCK_BOTH(pThis);

    STAM_PROFILE_STOP(&pThis->StatTimer, a);
}

/**
 * Timer callback which handles the audio data transfers on a periodic basis.
 *
 * @param   pDevIns             Device instance.
 * @param   pTimer              Timer which was used when calling this.
 * @param   pvUser              User argument as PAC97STATE.
 */
static DECLCALLBACK(void) ichac97Timer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer);

    PAC97STATE pThis = (PAC97STATE)pvUser;
    Assert(pThis == PDMINS_2_DATA(pDevIns, PAC97STATE));
    AssertPtr(pThis);

    ichac97TimerMain(pThis);
}
#endif /* !VBOX_WITH_AUDIO_AC97_CALLBACKS */

/**
 * Main routine to perform the actual audio data transfers from the AC'97 streams
 * to the backend(s) and vice versa.
 *
 * @param   pThis               AC'97 state.
 */
static void ichac97DoTransfers(PAC97STATE pThis)
{
    AssertPtrReturnVoid(pThis);

    ichac97StreamUpdate(pThis, &pThis->StreamLineIn, true /* fInTimer */);
    ichac97StreamUpdate(pThis, &pThis->StreamMicIn,  true /* fInTimer */);
    ichac97StreamUpdate(pThis, &pThis->StreamOut,    true /* fInTimer */);
}

/**
 * Transfers data of an AC'97 stream according to its usage (input / output).
 *
 * For an SDO (output) stream this means reading DMA data from the device to
 * the AC'97 stream's internal FIFO buffer.
 *
 * For an SDI (input) stream this is reading audio data from the AC'97 stream's
 * internal FIFO buffer and writing it as DMA data to the device.
 *
 * @returns IPRT status code.
 * @param   pThis               AC'97 state.
 * @param   pStream             AC'97 stream to update.
 * @param   cbToProcessMax      Maximum of data (in bytes) to process.
 */
static int ichac97StreamTransfer(PAC97STATE pThis, PAC97STREAM pStream, uint32_t cbToProcessMax)
{
    AssertPtrReturn(pThis,       VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,     VERR_INVALID_POINTER);
    AssertReturn(cbToProcessMax, VERR_INVALID_PARAMETER);

    ichac97StreamLock(pStream);

    PAC97BMREGS pRegs = &pStream->Regs;

    if (pRegs->sr & AC97_SR_DCH) /* Controller halted? */
    {
        if (pRegs->cr & AC97_CR_RPBM) /* Bus master operation starts. */
        {
            switch (pStream->u8SD)
            {
                case AC97SOUNDSOURCE_PO_INDEX:
                    /*ichac97WriteBUP(pThis, cbToProcess);*/
                    break;

                default:
                    break;
            }
        }

        ichac97StreamUnlock(pStream);
        return VINF_SUCCESS;
    }

    /* BCIS flag still set? Skip iteration. */
    if (pRegs->sr & AC97_SR_BCIS)
    {
        Log3Func(("[SD%RU8] BCIS set\n", pStream->u8SD));

        ichac97StreamUnlock(pStream);
        return VINF_SUCCESS;
    }

    uint32_t cbLeft           = RT_MIN((uint32_t)(pRegs->picb << 1), cbToProcessMax); /** @todo r=andy Assumes 16bit samples. */
    uint32_t cbProcessedTotal = 0;

    PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;
    AssertPtr(pCircBuf);

    int rc = VINF_SUCCESS;

    Log3Func(("[SD%RU8] cbToProcessMax=%RU32, cbLeft=%RU32\n", pStream->u8SD, cbToProcessMax, cbLeft));

    while (cbLeft)
    {
        if (!pRegs->bd_valid)
        {
            Log3Func(("Invalid buffer descriptor, fetching next one ...\n"));
            ichac97StreamFetchBDLE(pThis, pStream);
        }

        if (!pRegs->picb) /* Got a new buffer descriptor, that is, the position is 0? */
        {
            Log3Func(("Fresh buffer descriptor %RU8 is empty, addr=%#x, len=%#x, skipping\n",
                      pRegs->civ, pRegs->bd.addr, pRegs->bd.ctl_len));
            if (pRegs->civ == pRegs->lvi)
            {
                pRegs->sr |= AC97_SR_DCH; /** @todo r=andy Also set CELV? */
                pThis->bup_flag = 0;

                rc = VINF_EOF;
                break;
            }

            pRegs->sr &= ~AC97_SR_CELV;
            pRegs->civ = pRegs->piv;
            pRegs->piv = (pRegs->piv + 1) % AC97_MAX_BDLE;

            ichac97StreamFetchBDLE(pThis, pStream);
            continue;
        }

        uint32_t cbChunk = RT_MIN((uint32_t)(pRegs->picb << 1), cbLeft); /** @todo r=andy Assumes 16bit samples. */
        Assert(cbChunk);

        switch (pStream->u8SD)
        {
            case AC97SOUNDSOURCE_PO_INDEX: /* Output */
            {
                void *pvDst;
                size_t cbDst;

                RTCircBufAcquireWriteBlock(pCircBuf, cbChunk, &pvDst, &cbDst);

                if (cbDst)
                {
                    int rc2 = PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), pRegs->bd.addr, (uint8_t *)pvDst, cbDst);
                    AssertRC(rc2);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
                    RTFILE fh;
                    RTFileOpen(&fh,  VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97DMARead.pcm",
                               RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
                    RTFileWrite(fh, pvDst, cbDst, NULL);
                    RTFileClose(fh);
#endif
                }

                RTCircBufReleaseWriteBlock(pCircBuf, cbDst);

                cbChunk = (uint32_t)cbDst; /* Update the current chunk size to what really has been written. */
                break;
            }

            case AC97SOUNDSOURCE_PI_INDEX: /* Input */
            case AC97SOUNDSOURCE_MC_INDEX: /* Input */
            {
                void *pvSrc;
                size_t cbSrc;

                RTCircBufAcquireReadBlock(pCircBuf, cbChunk, &pvSrc, &cbSrc);

                if (cbSrc)
                {
                    int rc2 = PDMDevHlpPhysWrite(pThis->CTX_SUFF(pDevIns), pRegs->bd.addr, (uint8_t *)pvSrc, cbSrc);
                    AssertRC(rc2);

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
                    RTFILE fh;
                    RTFileOpen(&fh,  VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97DMAWrite.pcm",
                               RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
                    RTFileWrite(fh, pvSrc, cbSrc, NULL);
                    RTFileClose(fh);
#endif
                }

                RTCircBufReleaseReadBlock(pCircBuf, cbSrc);

                cbChunk = (uint32_t)cbSrc; /* Update the current chunk size to what really has been read. */
                break;
            }

            default:
                AssertMsgFailed(("Stream #%RU8 not supported\n", pStream->u8SD));
                rc = VERR_NOT_SUPPORTED;
                break;
        }

        if (RT_FAILURE(rc))
            break;

        if (cbChunk)
        {
            cbProcessedTotal     += cbChunk;
            Assert(cbProcessedTotal <= cbToProcessMax);
            Assert(cbLeft >= cbChunk);
            cbLeft      -= cbChunk;
            Assert((cbChunk & 1) == 0); /* Else the following shift won't work */

            pRegs->picb    -= (cbChunk >> 1); /** @todo r=andy Assumes 16bit samples. */
            pRegs->bd.addr += cbChunk;
        }

        LogFlowFunc(("[SD%RU8] cbChunk=%RU32, cbLeft=%RU32, cbTotal=%RU32, rc=%Rrc\n",
                     pStream->u8SD, cbChunk, cbLeft, cbProcessedTotal, rc));

        if (!pRegs->picb)
        {
            uint32_t new_sr = pRegs->sr & ~AC97_SR_CELV;

            if (pRegs->bd.ctl_len & AC97_BD_IOC)
            {
                new_sr |= AC97_SR_BCIS;
            }

            if (pRegs->civ == pRegs->lvi)
            {
                /* Did we run out of data? */
                LogFunc(("Underrun CIV (%RU8) == LVI (%RU8)\n", pRegs->civ, pRegs->lvi));

                new_sr |= AC97_SR_LVBCI | AC97_SR_DCH | AC97_SR_CELV;
                pThis->bup_flag = (pRegs->bd.ctl_len & AC97_BD_BUP) ? BUP_LAST : 0;

                rc = VINF_EOF;
            }
            else
            {
                pRegs->civ = pRegs->piv;
                pRegs->piv = (pRegs->piv + 1) % AC97_MAX_BDLE;
                ichac97StreamFetchBDLE(pThis, pStream);
            }

            ichac97StreamUpdateSR(pThis, pStream, new_sr);
        }

        if (/* All data processed? */
               rc == VINF_EOF
            /* ... or an error occurred? */
            || RT_FAILURE(rc))
        {
            break;
        }
    }

    ichac97StreamUnlock(pStream);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Port I/O Handler for IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the IN operation.
 * @param   pu32Val     Where to store the result.  This is always a 32-bit
 *                      variable regardless of what @a cbVal might say.
 * @param   cbVal       Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
static DECLCALLBACK(int) ichac97IOPortNABMRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort,
                                               uint32_t *pu32Val, unsigned cbVal)
{
    RT_NOREF(pDevIns);

    PAC97STATE pThis = (PAC97STATE)pvUser;

    DEVAC97_LOCK(pThis);

    /* Get the index of the NABMBAR port. */
    const uint32_t uPortIdx = uPort - pThis->IOPortBase[1];

    PAC97STREAM pStream = ichac97GetStreamFromIdx(pThis, AC97_PORT2IDX(uPortIdx));
    PAC97BMREGS pRegs   = NULL;

    if (pStream) /* Can be NULL, depending on the index (port). */
        pRegs = &pStream->Regs;

    int rc = VINF_SUCCESS;

    switch (cbVal)
    {
        case 1:
        {
            switch (uPortIdx)
            {
                case AC97_CAS:
                    /* Codec Access Semaphore Register */
                    Log3Func(("CAS %d\n", pThis->cas));
                    *pu32Val = pThis->cas;
                    pThis->cas = 1;
                    break;
                case PI_CIV:
                case PO_CIV:
                case MC_CIV:
                    /* Current Index Value Register */
                    *pu32Val = pRegs->civ;
                    Log3Func(("CIV[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                case PI_LVI:
                case PO_LVI:
                case MC_LVI:
                    /* Last Valid Index Register */
                    *pu32Val = pRegs->lvi;
                    Log3Func(("LVI[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                case PI_PIV:
                case PO_PIV:
                case MC_PIV:
                    /* Prefetched Index Value Register */
                    *pu32Val = pRegs->piv;
                    Log3Func(("PIV[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                case PI_CR:
                case PO_CR:
                case MC_CR:
                    /* Control Register */
                    *pu32Val = pRegs->cr;
                    Log3Func(("CR[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                case PI_SR:
                case PO_SR:
                case MC_SR:
                    /* Status Register (lower part) */
                    *pu32Val = RT_LO_U8(pRegs->sr);
                    Log3Func(("SRb[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                default:
                    *pu32Val = UINT32_MAX;
                    LogFunc(("U nabm readb %#x -> %#x\n", uPort, *pu32Val));
                    break;
            }
            break;
        }

        case 2:
        {
            switch (uPortIdx)
            {
                case PI_SR:
                case PO_SR:
                case MC_SR:
                    /* Status Register */
                    *pu32Val = pRegs->sr;
                    Log3Func(("SR[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                case PI_PICB:
                case PO_PICB:
                case MC_PICB:
                    /* Position in Current Buffer */
                    *pu32Val = pRegs->picb;
                    Log3Func(("PICB[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                default:
                    *pu32Val = UINT32_MAX;
                    LogFunc(("U nabm readw %#x -> %#x\n", uPort, *pu32Val));
                    break;
            }
            break;
        }

        case 4:
        {
            switch (uPortIdx)
            {
                case PI_BDBAR:
                case PO_BDBAR:
                case MC_BDBAR:
                    /* Buffer Descriptor Base Address Register */
                    *pu32Val = pRegs->bdbar;
                    Log3Func(("BMADDR[%d] -> %#x\n", AC97_PORT2IDX(uPortIdx), *pu32Val));
                    break;
                case PI_CIV:
                case PO_CIV:
                case MC_CIV:
                    /* 32-bit access: Current Index Value Register +
                     *                Last Valid Index Register +
                     *                Status Register */
                    *pu32Val = pRegs->civ | (pRegs->lvi << 8) | (pRegs->sr << 16); /** @todo r=andy Use RT_MAKE_U32_FROM_U8. */
                    Log3Func(("CIV LVI SR[%d] -> %#x, %#x, %#x\n",
                              AC97_PORT2IDX(uPortIdx), pRegs->civ, pRegs->lvi, pRegs->sr));
                    break;
                case PI_PICB:
                case PO_PICB:
                case MC_PICB:
                    /* 32-bit access: Position in Current Buffer Register +
                     *                Prefetched Index Value Register +
                     *                Control Register */
                    *pu32Val = pRegs->picb | (pRegs->piv << 16) | (pRegs->cr << 24); /** @todo r=andy Use RT_MAKE_U32_FROM_U8. */
                    Log3Func(("PICB PIV CR[%d] -> %#x %#x %#x %#x\n",
                              AC97_PORT2IDX(uPortIdx), *pu32Val, pRegs->picb, pRegs->piv, pRegs->cr));
                    break;
                case AC97_GLOB_CNT:
                    /* Global Control */
                    *pu32Val = pThis->glob_cnt;
                    Log3Func(("glob_cnt -> %#x\n", *pu32Val));
                    break;
                case AC97_GLOB_STA:
                    /* Global Status */
                    *pu32Val = pThis->glob_sta | AC97_GS_S0CR;
                    Log3Func(("glob_sta -> %#x\n", *pu32Val));
                    break;
                default:
                    *pu32Val = UINT32_MAX;
                    LogFunc(("U nabm readl %#x -> %#x\n", uPort, *pu32Val));
                    break;
            }
            break;
        }

        default:
        {
            AssertFailed();
            rc = VERR_IOM_IOPORT_UNUSED;
        }
    }

    DEVAC97_UNLOCK(pThis);

    return rc;
}

/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   u32Val      The value to output.
 * @param   cbVal       The value size in bytes.
 * @remarks Caller enters the device critical section.
 */
static DECLCALLBACK(int) ichac97IOPortNABMWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort,
                                                uint32_t u32Val, unsigned cbVal)
{
    RT_NOREF(pDevIns);

    PAC97STATE pThis = (PAC97STATE)pvUser;

    DEVAC97_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_IOPORT_WRITE);

    /* Get the index of the NABMBAR register. */
    const uint32_t uPortIdx = uPort - pThis->IOPortBase[1];

    PAC97STREAM pStream = ichac97GetStreamFromIdx(pThis, AC97_PORT2IDX(uPortIdx));
    PAC97BMREGS pRegs   = NULL;

    if (pStream) /* Can be NULL, depending on the index (port). */
        pRegs = &pStream->Regs;

    switch (cbVal)
    {
        case 1:
        {
            switch (uPortIdx)
            {
                /*
                 * Last Valid Index.
                 */
                case PI_LVI:
                case PO_LVI:
                case MC_LVI:
                {
                    if (   (pRegs->cr & AC97_CR_RPBM)
                        && (pRegs->sr & AC97_SR_DCH))
                    {
                        pRegs->sr &= ~(AC97_SR_DCH | AC97_SR_CELV);
                        pRegs->civ = pRegs->piv;
                        pRegs->piv = (pRegs->piv + 1) % AC97_MAX_BDLE;

                        ichac97StreamFetchBDLE(pThis, pStream);
                    }
                    pRegs->lvi = u32Val % AC97_MAX_BDLE;
                    Log3Func(("[SD%RU8] LVI <- %#x\n", pStream->u8SD, u32Val));
                    break;
                }

                /*
                 * Control Registers.
                 */
                case PI_CR:
                case PO_CR:
                case MC_CR:
                {
                    Log3Func(("[SD%RU8] CR <- %#x (cr %#x)\n", pStream->u8SD, u32Val, pRegs->cr));

                    if (u32Val & AC97_CR_RR) /* Busmaster reset. */
                    {
                        Log3Func(("[SD%RU8] Reset\n", pStream->u8SD));

                        /* Make sure that Run/Pause Bus Master bit (RPBM) is cleared (0). */
                        Assert((pRegs->cr & AC97_CR_RPBM) == 0);

                        ichac97StreamEnable(pThis, pStream, false /* fEnable */);
                        ichac97StreamReset(pThis, pStream);

                        ichac97StreamUpdateSR(pThis, pStream, AC97_SR_DCH); /** @todo Do we need to do that? */
                    }
                    else
                    {
                        pRegs->cr = u32Val & AC97_CR_VALID_MASK;

                        if (!(pRegs->cr & AC97_CR_RPBM))
                        {
                            Log3Func(("[SD%RU8] Disable\n", pStream->u8SD));

                            ichac97StreamEnable(pThis, pStream, false /* fEnable */);

                            pRegs->sr |= AC97_SR_DCH;
                        }
                        else
                        {
                            Log3Func(("[SD%RU8] Enable\n", pStream->u8SD));

                            pRegs->civ = pRegs->piv;
                            pRegs->piv = (pRegs->piv + 1) % AC97_MAX_BDLE;

                            pRegs->sr &= ~AC97_SR_DCH;

                            /* Fetch the initial BDLE descriptor. */
                            ichac97StreamFetchBDLE(pThis, pStream);

                            ichac97StreamEnable(pThis, pStream, true /* fEnable */);
                        }
                    }
                    break;
                }

                /*
                 * Status Registers.
                 */
                case PI_SR:
                case PO_SR:
                case MC_SR:
                {
                    pRegs->sr |= u32Val & ~(AC97_SR_RO_MASK | AC97_SR_WCLEAR_MASK);
                    ichac97StreamUpdateSR(pThis, pStream, pRegs->sr & ~(u32Val & AC97_SR_WCLEAR_MASK));
                    Log3Func(("[SD%RU8] SR <- %#x (sr %#x)\n", pStream->u8SD, u32Val, pRegs->sr));
                    break;
                }

                default:
                    LogRel2(("AC97: Warning: Unimplemented NABMWrite (%u byte) portIdx=%#x <- %#x\n", cbVal, uPortIdx, u32Val));
                    break;
            }
            break;
        }

        case 2:
        {
            switch (uPortIdx)
            {
                case PI_SR:
                case PO_SR:
                case MC_SR:
                    /* Status Register */
                    pRegs->sr |= u32Val & ~(AC97_SR_RO_MASK | AC97_SR_WCLEAR_MASK);
                    ichac97StreamUpdateSR(pThis, pStream, pRegs->sr & ~(u32Val & AC97_SR_WCLEAR_MASK));
                    Log3Func(("[SD%RU8] SR <- %#x (sr %#x)\n", pStream->u8SD, u32Val, pRegs->sr));
                    break;
                default:
                    LogRel2(("AC97: Warning: Unimplemented NABMWrite (%u byte) portIdx=%#x <- %#x\n", cbVal, uPortIdx, u32Val));
                    break;
            }
            break;
        }

        case 4:
        {
            switch (uPortIdx)
            {
                case PI_BDBAR:
                case PO_BDBAR:
                case MC_BDBAR:
                    /* Buffer Descriptor list Base Address Register */
                    pRegs->bdbar = u32Val & ~3;
                    Log3Func(("[SD%RU8] BDBAR <- %#x (bdbar %#x)\n", AC97_PORT2IDX(uPortIdx), u32Val, pRegs->bdbar));
                    break;
                case AC97_GLOB_CNT:
                    /* Global Control */
                    if (u32Val & AC97_GC_WR)
                        ichac97WarmReset(pThis);
                    if (u32Val & AC97_GC_CR)
                        ichac97ColdReset(pThis);
                    if (!(u32Val & (AC97_GC_WR | AC97_GC_CR)))
                        pThis->glob_cnt = u32Val & AC97_GC_VALID_MASK;
                    Log3Func(("glob_cnt <- %#x (glob_cnt %#x)\n", u32Val, pThis->glob_cnt));
                    break;
                case AC97_GLOB_STA:
                    /* Global Status */
                    pThis->glob_sta &= ~(u32Val & AC97_GS_WCLEAR_MASK);
                    pThis->glob_sta |= (u32Val & ~(AC97_GS_WCLEAR_MASK | AC97_GS_RO_MASK)) & AC97_GS_VALID_MASK;
                    Log3Func(("glob_sta <- %#x (glob_sta %#x)\n", u32Val, pThis->glob_sta));
                    break;
                default:
                    LogRel2(("AC97: Warning: Unimplemented NABMWrite (%u byte) portIdx=%#x <- %#x\n", cbVal, uPortIdx, u32Val));
                    break;
            }
            break;
        }

        default:
            LogRel2(("AC97: Warning: Unimplemented NABMWrite (%u byte) portIdx=%#x <- %#x\n", cbVal, uPortIdx, u32Val));
            break;
    }

    DEVAC97_UNLOCK_BOTH(pThis);

    return VINF_SUCCESS;
}

/**
 * Port I/O Handler for IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the IN operation.
 * @param   pu32Val     Where to store the result.  This is always a 32-bit
 *                      variable regardless of what @a cbVal might say.
 * @param   cbVal       Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
static DECLCALLBACK(int) ichac97IOPortNAMRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32Val, unsigned cbVal)
{
    RT_NOREF(pDevIns);
    PAC97STATE pThis = (PAC97STATE)pvUser;

    DEVAC97_LOCK(pThis);

    int rc = VINF_SUCCESS;

    uint32_t index = uPort - pThis->IOPortBase[0];

    switch (cbVal)
    {
        case 1:
        {
            LogRel2(("AC97: Warning: Unimplemented read (%u byte) port=%#x, idx=%RU32\n", cbVal, uPort, index));
            pThis->cas = 0;
            *pu32Val = UINT32_MAX;
            break;
        }

        case 2:
        {
            *pu32Val = UINT32_MAX;
            pThis->cas = 0;

            switch (index)
            {
                default:
                    *pu32Val = ichac97MixerGet(pThis, index);
                    break;
            }
            break;
        }

        case 4:
        {
            LogRel2(("AC97: Warning: Unimplemented read (%u byte) port=%#x, idx=%RU32\n", cbVal, uPort, index));
            pThis->cas = 0;
            *pu32Val = UINT32_MAX;
            break;
        }

        default:
        {
            AssertFailed();
            rc = VERR_IOM_IOPORT_UNUSED;
        }
    }

    DEVAC97_UNLOCK(pThis);

    return rc;
}

/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   u32Val      The value to output.
 * @param   cbVal       The value size in bytes.
 * @remarks Caller enters the device critical section.
 */
static DECLCALLBACK(int) ichac97IOPortNAMWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort,
                                               uint32_t u32Val, unsigned cbVal)
{
    RT_NOREF(pDevIns);
    PAC97STATE pThis = (PAC97STATE)pvUser;

    DEVAC97_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_IOPORT_WRITE);

    uint32_t uPortIdx = uPort - pThis->IOPortBase[0];

    switch (cbVal)
    {
        case 1:
        {
            LogRel2(("AC97: Warning: Unimplemented NAMWrite (%u byte) port=%#x, idx=0x%x <- %#x\n", cbVal, uPort, uPortIdx, u32Val));
            pThis->cas = 0;
            break;
        }

        case 2:
        {
            pThis->cas = 0;
            switch (uPortIdx)
            {
                case AC97_Reset:
                    ichac97Reset(pThis->CTX_SUFF(pDevIns));
                    break;
                case AC97_Powerdown_Ctrl_Stat:
                    u32Val &= ~0xf;
                    u32Val |= ichac97MixerGet(pThis, uPortIdx) & 0xf;
                    ichac97MixerSet(pThis, uPortIdx, u32Val);
                    break;
                case AC97_Master_Volume_Mute:
                    if (pThis->uCodecModel == AC97_CODEC_AD1980)
                    {
                        if (ichac97MixerGet(pThis, AC97_AD_Misc) & AC97_AD_MISC_LOSEL)
                            break; /* Register controls surround (rear), do nothing. */
                    }
                    ichac97MixerSetVolume(pThis, uPortIdx, PDMAUDIOMIXERCTL_VOLUME_MASTER, u32Val);
                    break;
                case AC97_Headphone_Volume_Mute:
                    if (pThis->uCodecModel == AC97_CODEC_AD1980)
                    {
                        if (ichac97MixerGet(pThis, AC97_AD_Misc) & AC97_AD_MISC_HPSEL)
                        {
                            /* Register controls PCM (front) outputs. */
                            ichac97MixerSetVolume(pThis, uPortIdx, PDMAUDIOMIXERCTL_VOLUME_MASTER, u32Val);
                        }
                    }
                    break;
                case AC97_PCM_Out_Volume_Mute:
                    ichac97MixerSetVolume(pThis, uPortIdx, PDMAUDIOMIXERCTL_FRONT, u32Val);
                    break;
                case AC97_Line_In_Volume_Mute:
                    ichac97MixerSetVolume(pThis, uPortIdx, PDMAUDIOMIXERCTL_LINE_IN, u32Val);
                    break;
                case AC97_Record_Select:
                    ichac97MixerRecordSelect(pThis, u32Val);
                    break;
                case AC97_Record_Gain_Mute:
                    /* Newer Ubuntu guests rely on that when controlling gain and muting
                     * the recording (capturing) levels. */
                    ichac97MixerSetVolume(pThis, uPortIdx, PDMAUDIOMIXERCTL_LINE_IN, u32Val);
                    break;
                case AC97_Record_Gain_Mic_Mute:
                    /* Ditto; see note above. */
                    ichac97MixerSetVolume(pThis, uPortIdx, PDMAUDIOMIXERCTL_MIC_IN,  u32Val);
                    break;
                case AC97_Vendor_ID1:
                case AC97_Vendor_ID2:
                    LogFunc(("Attempt to write vendor ID to %#x\n", u32Val));
                    break;
                case AC97_Extended_Audio_ID:
                    LogFunc(("Attempt to write extended audio ID to %#x\n", u32Val));
                    break;
                case AC97_Extended_Audio_Ctrl_Stat:
                    if (!(u32Val & AC97_EACS_VRA))
                    {
                        ichac97MixerSet(pThis, AC97_PCM_Front_DAC_Rate, 48000);
                        ichac97StreamReOpen(pThis, &pThis->StreamOut);

                        ichac97MixerSet(pThis, AC97_PCM_LR_ADC_Rate,    48000);
                        ichac97StreamReOpen(pThis, &pThis->StreamLineIn);
                    }
                    else
                        LogRel2(("AC97: Variable rate audio (VRA) is not supported\n"));

                    if (!(u32Val & AC97_EACS_VRM))
                    {
                        ichac97MixerSet(pThis, AC97_MIC_ADC_Rate,       48000);
                        ichac97StreamReOpen(pThis, &pThis->StreamMicIn);
                    }
                    else
                        LogRel2(("AC97: Variable rate microphone audio (VRM) is not supported\n"));

                    LogFunc(("Setting extended audio control to %#x\n", u32Val));
                    ichac97MixerSet(pThis, AC97_Extended_Audio_Ctrl_Stat, u32Val);
                    break;
                case AC97_PCM_Front_DAC_Rate:
                    if (ichac97MixerGet(pThis, AC97_Extended_Audio_Ctrl_Stat) & AC97_EACS_VRA)
                    {
                        ichac97MixerSet(pThis, uPortIdx, u32Val);
                        LogFunc(("Set front DAC rate to %RU32\n", u32Val));
                        ichac97StreamReOpen(pThis, &pThis->StreamOut);
                    }
                    else
                        AssertMsgFailed(("Attempt to set front DAC rate to %RU32, but VRA is not set\n", u32Val));
                    break;
                case AC97_MIC_ADC_Rate:
                    if (ichac97MixerGet(pThis, AC97_Extended_Audio_Ctrl_Stat) & AC97_EACS_VRM)
                    {
                        ichac97MixerSet(pThis, uPortIdx, u32Val);
                        LogFunc(("Set MIC ADC rate to %RU32\n", u32Val));
                        ichac97StreamReOpen(pThis, &pThis->StreamMicIn);
                    }
                    else
                        AssertMsgFailed(("Attempt to set MIC ADC rate to %RU32, but VRM is not set\n", u32Val));
                    break;
                case AC97_PCM_LR_ADC_Rate:
                    if (ichac97MixerGet(pThis, AC97_Extended_Audio_Ctrl_Stat) & AC97_EACS_VRA)
                    {
                        ichac97MixerSet(pThis, uPortIdx, u32Val);
                        LogFunc(("Set front LR ADC rate to %RU32\n", u32Val));
                        ichac97StreamReOpen(pThis, &pThis->StreamLineIn);
                    }
                    else
                        AssertMsgFailed(("Attempt to set LR ADC rate to %RU32, but VRA is not set\n", u32Val));
                    break;
                default:
                    LogRel2(("AC97: Warning: Unimplemented NAMWrite (%u byte) port=%#x, idx=0x%x <- %#x\n", cbVal, uPort, uPortIdx, u32Val));
                    ichac97MixerSet(pThis, uPortIdx, u32Val);
                    break;
            }
            break;
        }

        case 4:
        {
            LogRel2(("AC97: Warning: Unimplemented NAMWrite (%u byte) port=%#x, idx=0x%x <- %#x\n", cbVal, uPort, uPortIdx, u32Val));
            pThis->cas = 0;
            break;
        }

        default:
            AssertMsgFailed(("Unhandled NAMWrite port=%#x, cbVal=%u u32Val=%#x\n", uPort, cbVal, u32Val));
            break;
    }

    DEVAC97_UNLOCK_BOTH(pThis);

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) ichac97IOPortMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                          RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF(cb, enmType);

    Assert(enmType == PCI_ADDRESS_SPACE_IO);
    Assert(cb >= 0x20);

    if (iRegion > 1) /* We support 2 regions max. at the moment. */
        return VERR_INVALID_PARAMETER;

    PAC97STATE pThis   = RT_FROM_MEMBER(pPciDev, AC97STATE, PciDev);
    RTIOPORT   Port    = (RTIOPORT)GCPhysAddress;

    int rc;
    if (iRegion == 0)
        rc = PDMDevHlpIOPortRegister(pDevIns, Port, 256, pThis,
                                     ichac97IOPortNAMWrite, ichac97IOPortNAMRead,
                                     NULL, NULL, "ICHAC97 NAM");
    else
        rc = PDMDevHlpIOPortRegister(pDevIns, Port, 64, pThis,
                                     ichac97IOPortNABMWrite, ichac97IOPortNABMRead,
                                     NULL, NULL, "ICHAC97 NABM");
    if (RT_FAILURE(rc))
        return rc;

    pThis->IOPortBase[iRegion] = Port;
    return VINF_SUCCESS;
}

#ifdef IN_RING3
/**
 * Saves (serializes) an AC'97 stream using SSM.
 *
 * @returns IPRT status code.
 * @param   pDevIns             Device instance.
 * @param   pSSM                Saved state manager (SSM) handle to use.
 * @param   pStream             AC'97 stream to save.
 */
static int ichac97SaveStream(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, PAC97STREAM pStream)
{
    RT_NOREF(pDevIns);
    PAC97BMREGS pRegs = &pStream->Regs;

    SSMR3PutU32(pSSM, pRegs->bdbar);
    SSMR3PutU8( pSSM, pRegs->civ);
    SSMR3PutU8( pSSM, pRegs->lvi);
    SSMR3PutU16(pSSM, pRegs->sr);
    SSMR3PutU16(pSSM, pRegs->picb);
    SSMR3PutU8( pSSM, pRegs->piv);
    SSMR3PutU8( pSSM, pRegs->cr);
    SSMR3PutS32(pSSM, pRegs->bd_valid);
    SSMR3PutU32(pSSM, pRegs->bd.addr);
    SSMR3PutU32(pSSM, pRegs->bd.ctl_len);

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) ichac97SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogFlowFuncEnter();

    SSMR3PutU32(pSSM, pThis->glob_cnt);
    SSMR3PutU32(pSSM, pThis->glob_sta);
    SSMR3PutU32(pSSM, pThis->cas);

    /** @todo r=andy For the next saved state version, add unique stream identifiers and a stream count. */
    /* Note: The order the streams are saved here is critical, so don't touch. */
    int rc2 = ichac97SaveStream(pDevIns, pSSM, &pThis->StreamLineIn);
    AssertRC(rc2);
    rc2 = ichac97SaveStream(pDevIns, pSSM, &pThis->StreamOut);
    AssertRC(rc2);
    rc2 = ichac97SaveStream(pDevIns, pSSM, &pThis->StreamMicIn);
    AssertRC(rc2);

    SSMR3PutMem(pSSM, pThis->mixer_data, sizeof(pThis->mixer_data));

    uint8_t active[AC97SOUNDSOURCE_LAST_INDEX];

    active[AC97SOUNDSOURCE_PI_INDEX] = ichac97StreamIsEnabled(pThis, &pThis->StreamLineIn) ? 1 : 0;
    active[AC97SOUNDSOURCE_PO_INDEX] = ichac97StreamIsEnabled(pThis, &pThis->StreamOut)    ? 1 : 0;
    active[AC97SOUNDSOURCE_MC_INDEX] = ichac97StreamIsEnabled(pThis, &pThis->StreamMicIn)  ? 1 : 0;

    SSMR3PutMem(pSSM, active, sizeof(active));

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

/**
 * Loads an AC'97 stream from SSM.
 *
 * @returns IPRT status code.
 * @param   pDevIns             Device instance.
 * @param   pSSM                Saved state manager (SSM) handle to use.
 * @param   pStream             AC'97 stream to load.
 */
static int ichac97LoadStream(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, PAC97STREAM pStream)
{
    RT_NOREF(pDevIns);
    PAC97BMREGS pRegs = &pStream->Regs;

    SSMR3GetU32(pSSM, &pRegs->bdbar);
    SSMR3GetU8( pSSM, &pRegs->civ);
    SSMR3GetU8( pSSM, &pRegs->lvi);
    SSMR3GetU16(pSSM, &pRegs->sr);
    SSMR3GetU16(pSSM, &pRegs->picb);
    SSMR3GetU8( pSSM, &pRegs->piv);
    SSMR3GetU8( pSSM, &pRegs->cr);
    SSMR3GetS32(pSSM, &pRegs->bd_valid);
    SSMR3GetU32(pSSM, &pRegs->bd.addr);
    SSMR3GetU32(pSSM, &pRegs->bd.ctl_len);

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) ichac97LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogRel2(("ichac97LoadExec: uVersion=%RU32, uPass=0x%x\n", uVersion, uPass));

    AssertMsgReturn (uVersion == AC97_SSM_VERSION, ("%RU32\n", uVersion), VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    SSMR3GetU32(pSSM, &pThis->glob_cnt);
    SSMR3GetU32(pSSM, &pThis->glob_sta);
    SSMR3GetU32(pSSM, &pThis->cas);

    /** @todo r=andy For the next saved state version, add unique stream identifiers and a stream count. */
    /* Note: The order the streams are loaded here is critical, so don't touch. */
    int rc2 = ichac97LoadStream(pDevIns, pSSM, &pThis->StreamLineIn);
    AssertRC(rc2);
    rc2 = ichac97LoadStream(pDevIns, pSSM, &pThis->StreamOut);
    AssertRC(rc2);
    rc2 = ichac97LoadStream(pDevIns, pSSM, &pThis->StreamMicIn);
    AssertRC(rc2);

    SSMR3GetMem(pSSM, pThis->mixer_data, sizeof(pThis->mixer_data));

    /** @todo r=andy Stream IDs are hardcoded to certain streams. */
    uint8_t uaStrmsActive[AC97SOUNDSOURCE_LAST_INDEX];
    SSMR3GetMem(pSSM, uaStrmsActive, sizeof(uaStrmsActive));

    ichac97MixerRecordSelect(pThis, ichac97MixerGet(pThis, AC97_Record_Select));
# define V_(a, b) ichac97MixerSetVolume(pThis, a, b, ichac97MixerGet(pThis, a))
    V_(AC97_Master_Volume_Mute,  PDMAUDIOMIXERCTL_VOLUME_MASTER);
    V_(AC97_PCM_Out_Volume_Mute, PDMAUDIOMIXERCTL_FRONT);
    V_(AC97_Line_In_Volume_Mute, PDMAUDIOMIXERCTL_LINE_IN);
    V_(AC97_Mic_Volume_Mute,     PDMAUDIOMIXERCTL_MIC_IN);
# undef V_
    if (pThis->uCodecModel == AC97_CODEC_AD1980)
        if (ichac97MixerGet(pThis, AC97_AD_Misc) & AC97_AD_MISC_HPSEL)
            ichac97MixerSetVolume(pThis, AC97_Headphone_Volume_Mute, PDMAUDIOMIXERCTL_VOLUME_MASTER,
                             ichac97MixerGet(pThis, AC97_Headphone_Volume_Mute));

    /** @todo r=andy Stream IDs are hardcoded to certain streams. */
    rc2 = ichac97StreamEnable(pThis, &pThis->StreamLineIn,    RT_BOOL(uaStrmsActive[AC97SOUNDSOURCE_PI_INDEX]));
    if (RT_SUCCESS(rc2))
        rc2 = ichac97StreamEnable(pThis, &pThis->StreamMicIn, RT_BOOL(uaStrmsActive[AC97SOUNDSOURCE_MC_INDEX]));
    if (RT_SUCCESS(rc2))
        rc2 = ichac97StreamEnable(pThis, &pThis->StreamOut,   RT_BOOL(uaStrmsActive[AC97SOUNDSOURCE_PO_INDEX]));

    pThis->bup_flag  = 0;
    pThis->last_samp = 0;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) ichac97QueryInterface(struct PDMIBASE *pInterface, const char *pszIID)
{
    PAC97STATE pThis = RT_FROM_MEMBER(pInterface, AC97STATE, IBase);
    Assert(&pThis->IBase == pInterface);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    return NULL;
}


/**
 * Powers off the device.
 *
 * @param   pDevIns             Device instance to power off.
 */
static DECLCALLBACK(void) ichac97PowerOff(PPDMDEVINS pDevIns)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogRel2(("AC97: Powering off ...\n"));

    /* Note: Involves mixer stream / sink destruction, so also do this here
     *       instead of in ichac97Destruct(). */
    ichac97StreamsDestroy(pThis);

    /**
     * Note: Destroy the mixer while powering off and *not* in ichac97Destruct,
     *       giving the mixer the chance to release any references held to
     *       PDM audio streams it maintains.
     */
    if (pThis->pMixer)
    {
        AudioMixerDestroy(pThis->pMixer);
        pThis->pMixer = NULL;
    }
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 *
 * @remarks The original sources didn't install a reset handler, but it seems to
 *          make sense to me so we'll do it.
 */
static DECLCALLBACK(void) ichac97Reset(PPDMDEVINS pDevIns)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogRel(("AC97: Reset\n"));

    /*
     * Reset the mixer too. The Windows XP driver seems to rely on
     * this. At least it wants to read the vendor id before it resets
     * the codec manually.
     */
    ichac97MixerReset(pThis);

    /*
     * Reset all streams.
     */
    ichac97StreamEnable(pThis, &pThis->StreamLineIn, false /* fEnable */);
    ichac97StreamReset(pThis, &pThis->StreamLineIn);

    ichac97StreamEnable(pThis, &pThis->StreamMicIn, false /* fEnable */);
    ichac97StreamReset(pThis, &pThis->StreamMicIn);

    ichac97StreamEnable(pThis, &pThis->StreamOut, false /* fEnable */);
    ichac97StreamReset(pThis, &pThis->StreamOut);

    /*
     * Reset mixer sinks.
     *
     * Do the reset here instead of in ichac97StreamReset();
     * the mixer sink(s) might still have data to be processed when an audio stream gets reset.
     */
    AudioMixerSinkReset(pThis->pSinkLineIn);
    AudioMixerSinkReset(pThis->pSinkMicIn);
    AudioMixerSinkReset(pThis->pSinkOut);

# ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
    /*
     * Stop the timer, if any.
     */
    ichac97TimerStop(pThis);

    pThis->cStreamsActive = 0;
# endif
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) ichac97Destruct(PPDMDEVINS pDevIns)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogFlowFuncEnter();

    PAC97DRIVER pDrv, pDrvNext;
    RTListForEachSafe(&pThis->lstDrv, pDrv, pDrvNext, AC97DRIVER, Node)
    {
        RTListNodeRemove(&pDrv->Node);
        RTMemFree(pDrv);
    }

    /* Sanity. */
    Assert(RTListIsEmpty(&pThis->lstDrv));

    return VINF_SUCCESS;
}


/**
 * Attach command, internal version.
 *
 * This is called to let the device attach to a driver for a specified LUN
 * during runtime. This is not called during VM construction, the device
 * constructor has to attach to all the available drivers.
 *
 * @returns VBox status code.
 * @param   pThis       AC'97 state.
 * @param   uLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @param   ppDrv       Attached driver instance on success. Optional.
 */
static int ichac97AttachInternal(PAC97STATE pThis, unsigned uLUN, uint32_t fFlags, PAC97DRIVER *ppDrv)
{
    RT_NOREF(fFlags);

    /*
     * Attach driver.
     */
    char *pszDesc;
    if (RTStrAPrintf(&pszDesc, "Audio driver port (AC'97) for LUN #%u", uLUN) <= 0)
        AssertLogRelFailedReturn(VERR_NO_MEMORY);

    PPDMIBASE pDrvBase;
    int rc = PDMDevHlpDriverAttach(pThis->pDevInsR3, uLUN,
                                   &pThis->IBase, &pDrvBase, pszDesc);
    if (RT_SUCCESS(rc))
    {
        PAC97DRIVER pDrv = (PAC97DRIVER)RTMemAllocZ(sizeof(AC97DRIVER));
        if (pDrv)
        {
            pDrv->pDrvBase   = pDrvBase;
            pDrv->pConnector = PDMIBASE_QUERY_INTERFACE(pDrvBase, PDMIAUDIOCONNECTOR);
            AssertMsg(pDrv->pConnector != NULL, ("Configuration error: LUN #%u has no host audio interface, rc=%Rrc\n", uLUN, rc));
            pDrv->pAC97State = pThis;
            pDrv->uLUN       = uLUN;

            /*
             * For now we always set the driver at LUN 0 as our primary
             * host backend. This might change in the future.
             */
            if (pDrv->uLUN == 0)
                pDrv->fFlags |= PDMAUDIODRVFLAGS_PRIMARY;

            LogFunc(("LUN#%RU8: pCon=%p, drvFlags=0x%x\n", uLUN, pDrv->pConnector, pDrv->fFlags));

            /* Attach to driver list if not attached yet. */
            if (!pDrv->fAttached)
            {
                RTListAppend(&pThis->lstDrv, &pDrv->Node);
                pDrv->fAttached = true;
            }

            if (ppDrv)
                *ppDrv = pDrv;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
        LogFunc(("No attached driver for LUN #%u\n", uLUN));

    if (RT_FAILURE(rc))
    {
        /* Only free this string on failure;
         * must remain valid for the live of the driver instance. */
        RTStrFree(pszDesc);
    }

    LogFunc(("uLUN=%u, fFlags=0x%x, rc=%Rrc\n", uLUN, fFlags, rc));
    return rc;
}

/**
 * Detach command, internal version.
 *
 * This is called to let the device detach from a driver for a specified LUN
 * during runtime.
 *
 * @returns VBox status code.
 * @param   pThis       AC'97 state.
 * @param   pDrv        Driver to detach device from.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
static int ichac97DetachInternal(PAC97STATE pThis, PAC97DRIVER pDrv, uint32_t fFlags)
{
    RT_NOREF(fFlags);

    AudioMixerSinkRemoveStream(pThis->pSinkMicIn,  pDrv->MicIn.pMixStrm);
    AudioMixerStreamDestroy(pDrv->MicIn.pMixStrm);
    pDrv->MicIn.pMixStrm = NULL;

    AudioMixerSinkRemoveStream(pThis->pSinkLineIn, pDrv->LineIn.pMixStrm);
    AudioMixerStreamDestroy(pDrv->LineIn.pMixStrm);
    pDrv->LineIn.pMixStrm = NULL;

    AudioMixerSinkRemoveStream(pThis->pSinkOut,    pDrv->Out.pMixStrm);
    AudioMixerStreamDestroy(pDrv->Out.pMixStrm);
    pDrv->Out.pMixStrm = NULL;

    RTListNodeRemove(&pDrv->Node);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", pDrv->uLUN, fFlags));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 */
static DECLCALLBACK(int) ichac97Attach(PPDMDEVINS pDevIns, unsigned uLUN, uint32_t fFlags)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", uLUN, fFlags));

    DEVAC97_LOCK(pThis);

    PAC97DRIVER pDrv;
    int rc2 = ichac97AttachInternal(pThis, uLUN, fFlags, &pDrv);
    if (RT_SUCCESS(rc2))
    {
        if (DrvAudioHlpStreamCfgIsValid(&pThis->StreamLineIn.State.Cfg))
            ichac97MixerAddDrvStream(pThis, pThis->pSinkLineIn, &pThis->StreamLineIn.State.Cfg, pDrv);

        if (DrvAudioHlpStreamCfgIsValid(&pThis->StreamMicIn.State.Cfg))
            ichac97MixerAddDrvStream(pThis, pThis->pSinkMicIn,  &pThis->StreamMicIn.State.Cfg, pDrv);

        if (DrvAudioHlpStreamCfgIsValid(&pThis->StreamOut.State.Cfg))
            ichac97MixerAddDrvStream(pThis, pThis->pSinkOut,    &pThis->StreamOut.State.Cfg, pDrv);
    }

    DEVAC97_UNLOCK(pThis);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 */
static DECLCALLBACK(void) ichac97Detach(PPDMDEVINS pDevIns, unsigned uLUN, uint32_t fFlags)
{
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", uLUN, fFlags));

    DEVAC97_LOCK(pThis);

    PAC97DRIVER pDrv, pDrvNext;
    RTListForEachSafe(&pThis->lstDrv, pDrv, pDrvNext, AC97DRIVER, Node)
    {
        if (pDrv->uLUN == uLUN)
        {
            int rc2 = ichac97DetachInternal(pThis, pDrv, fFlags);
            if (RT_SUCCESS(rc2))
            {
                RTMemFree(pDrv);
                pDrv = NULL;
            }

            break;
        }
    }

    DEVAC97_UNLOCK(pThis);
}

/**
 * Re-attaches (replaces) a driver with a new driver.
 *
 * @returns VBox status code.
 * @param   pThis       Device instance.
 * @param   pDrv        Driver instance used for attaching to.
 *                      If NULL is specified, a new driver will be created and appended
 *                      to the driver list.
 * @param   uLUN        The logical unit which is being re-detached.
 * @param   pszDriver   New driver name to attach.
 */
static int ichac97ReattachInternal(PAC97STATE pThis, PAC97DRIVER pDrv, uint8_t uLUN, const char *pszDriver)
{
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pszDriver, VERR_INVALID_POINTER);

    int rc;

    if (pDrv)
    {
        rc = ichac97DetachInternal(pThis, pDrv, 0 /* fFlags */);
        if (RT_SUCCESS(rc))
            rc = PDMDevHlpDriverDetach(pThis->pDevInsR3, PDMIBASE_2_PDMDRV(pDrv->pDrvBase), 0 /* fFlags */);

        if (RT_FAILURE(rc))
            return rc;

        pDrv = NULL;
    }

    PVM pVM = PDMDevHlpGetVM(pThis->pDevInsR3);
    PCFGMNODE pRoot = CFGMR3GetRoot(pVM);
    PCFGMNODE pDev0 = CFGMR3GetChild(pRoot, "Devices/ichac97/0/");

    /* Remove LUN branch. */
    CFGMR3RemoveNode(CFGMR3GetChildF(pDev0, "LUN#%u/", uLUN));

#define RC_CHECK() if (RT_FAILURE(rc)) { AssertReleaseRC(rc); break; }

    do
    {
        PCFGMNODE pLunL0;
        rc = CFGMR3InsertNodeF(pDev0, &pLunL0, "LUN#%u/", uLUN);        RC_CHECK();
        rc = CFGMR3InsertString(pLunL0, "Driver",       "AUDIO");       RC_CHECK();
        rc = CFGMR3InsertNode(pLunL0,   "Config/",       NULL);         RC_CHECK();

        PCFGMNODE pLunL1, pLunL2;
        rc = CFGMR3InsertNode  (pLunL0, "AttachedDriver/", &pLunL1);    RC_CHECK();
        rc = CFGMR3InsertNode  (pLunL1,  "Config/",        &pLunL2);    RC_CHECK();
        rc = CFGMR3InsertString(pLunL1,  "Driver",          pszDriver); RC_CHECK();

        rc = CFGMR3InsertString(pLunL2, "AudioDriver", pszDriver);      RC_CHECK();

    } while (0);

    if (RT_SUCCESS(rc))
        rc = ichac97AttachInternal(pThis, uLUN, 0 /* fFlags */, NULL /* ppDrv */);

    LogFunc(("pThis=%p, uLUN=%u, pszDriver=%s, rc=%Rrc\n", pThis, uLUN, pszDriver, rc));

#undef RC_CHECK

    return rc;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) ichac97Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    PAC97STATE pThis = PDMINS_2_DATA(pDevIns, PAC97STATE);

    /* NB: This must be done *before* any possible failure (and running the destructor). */
    RTListInit(&pThis->lstDrv);

    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validations.
     */
    if (!CFGMR3AreValuesValid(pCfg,
                              "Codec\0"
                              "TimerHz\0"))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("Invalid configuration for the AC'97 device"));

    /*
     * Read config data.
     */
    char szCodec[20];
    int rc = CFGMR3QueryStringDef(pCfg, "Codec", &szCodec[0], sizeof(szCodec), "STAC9700");
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("AC'97 configuration error: Querying \"Codec\" as string failed"));

#ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
    uint16_t uTimerHz;
    rc = CFGMR3QueryU16Def(pCfg, "TimerHz", &uTimerHz, AC97_TIMER_HZ /* Default value, if not set. */);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("AC'97 configuration error: failed to read Hertz (Hz) rate as unsigned integer"));
#endif

    /*
     * The AD1980 codec (with corresponding PCI subsystem vendor ID) is whitelisted
     * in the Linux kernel; Linux makes no attempt to measure the data rate and assumes
     * 48 kHz rate, which is exactly what we need. Same goes for AD1981B.
     */
    if (!strcmp(szCodec, "STAC9700"))
        pThis->uCodecModel = AC97_CODEC_STAC9700;
    else if (!strcmp(szCodec, "AD1980"))
        pThis->uCodecModel = AC97_CODEC_AD1980;
    else if (!strcmp(szCodec, "AD1981B"))
        pThis->uCodecModel = AC97_CODEC_AD1981B;
    else
    {
        return PDMDevHlpVMSetError(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES, RT_SRC_POS,
                                   N_("AC'97 configuration error: The \"Codec\" value \"%s\" is unsupported"),
                                   szCodec);
    }

    /*
     * Use an own critical section for the device instead of the default
     * one provided by PDM. This allows fine-grained locking in combination
     * with TM when timer-specific stuff is being called in e.g. the MMIO handlers.
     */
    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->CritSect, RT_SRC_POS, "AC'97");
    AssertRCReturn(rc, rc);

    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /*
     * Initialize data (most of it anyway).
     */
    pThis->pDevInsR3                = pDevIns;
    /* IBase */
    pThis->IBase.pfnQueryInterface  = ichac97QueryInterface;

    /* PCI Device */
    PCIDevSetVendorId         (&pThis->PciDev, 0x8086); /* 00 ro - intel. */               Assert(pThis->PciDev.abConfig[0x00] == 0x86); Assert(pThis->PciDev.abConfig[0x01] == 0x80);
    PCIDevSetDeviceId         (&pThis->PciDev, 0x2415); /* 02 ro - 82801 / 82801aa(?). */  Assert(pThis->PciDev.abConfig[0x02] == 0x15); Assert(pThis->PciDev.abConfig[0x03] == 0x24);
    PCIDevSetCommand          (&pThis->PciDev, 0x0000); /* 04 rw,ro - pcicmd. */           Assert(pThis->PciDev.abConfig[0x04] == 0x00); Assert(pThis->PciDev.abConfig[0x05] == 0x00);
    PCIDevSetStatus           (&pThis->PciDev, VBOX_PCI_STATUS_DEVSEL_MEDIUM |  VBOX_PCI_STATUS_FAST_BACK); /* 06 rwc?,ro? - pcists. */      Assert(pThis->PciDev.abConfig[0x06] == 0x80); Assert(pThis->PciDev.abConfig[0x07] == 0x02);
    PCIDevSetRevisionId       (&pThis->PciDev, 0x01);   /* 08 ro - rid. */                 Assert(pThis->PciDev.abConfig[0x08] == 0x01);
    PCIDevSetClassProg        (&pThis->PciDev, 0x00);   /* 09 ro - pi. */                  Assert(pThis->PciDev.abConfig[0x09] == 0x00);
    PCIDevSetClassSub         (&pThis->PciDev, 0x01);   /* 0a ro - scc; 01 == Audio. */    Assert(pThis->PciDev.abConfig[0x0a] == 0x01);
    PCIDevSetClassBase        (&pThis->PciDev, 0x04);   /* 0b ro - bcc; 04 == multimedia. */ Assert(pThis->PciDev.abConfig[0x0b] == 0x04);
    PCIDevSetHeaderType       (&pThis->PciDev, 0x00);   /* 0e ro - headtyp. */             Assert(pThis->PciDev.abConfig[0x0e] == 0x00);
    PCIDevSetBaseAddress      (&pThis->PciDev, 0,       /* 10 rw - nambar - native audio mixer base. */
                               true /* fIoSpace */, false /* fPrefetchable */, false /* f64Bit */, 0x00000000); Assert(pThis->PciDev.abConfig[0x10] == 0x01); Assert(pThis->PciDev.abConfig[0x11] == 0x00); Assert(pThis->PciDev.abConfig[0x12] == 0x00); Assert(pThis->PciDev.abConfig[0x13] == 0x00);
    PCIDevSetBaseAddress      (&pThis->PciDev, 1,       /* 14 rw - nabmbar - native audio bus mastering. */
                               true /* fIoSpace */, false /* fPrefetchable */, false /* f64Bit */, 0x00000000); Assert(pThis->PciDev.abConfig[0x14] == 0x01); Assert(pThis->PciDev.abConfig[0x15] == 0x00); Assert(pThis->PciDev.abConfig[0x16] == 0x00); Assert(pThis->PciDev.abConfig[0x17] == 0x00);
    PCIDevSetInterruptLine    (&pThis->PciDev, 0x00);   /* 3c rw. */                       Assert(pThis->PciDev.abConfig[0x3c] == 0x00);
    PCIDevSetInterruptPin     (&pThis->PciDev, 0x01);   /* 3d ro - INTA#. */               Assert(pThis->PciDev.abConfig[0x3d] == 0x01);

    if (pThis->uCodecModel == AC97_CODEC_AD1980)
    {
        PCIDevSetSubSystemVendorId(&pThis->PciDev, 0x1028); /* 2c ro - Dell.) */
        PCIDevSetSubSystemId      (&pThis->PciDev, 0x0177); /* 2e ro. */
    }
    else if (pThis->uCodecModel == AC97_CODEC_AD1981B)
    {
        PCIDevSetSubSystemVendorId(&pThis->PciDev, 0x1028); /* 2c ro - Dell.) */
        PCIDevSetSubSystemId      (&pThis->PciDev, 0x01ad); /* 2e ro. */
    }
    else
    {
        PCIDevSetSubSystemVendorId(&pThis->PciDev, 0x8086); /* 2c ro - Intel.) */
        PCIDevSetSubSystemId      (&pThis->PciDev, 0x0000); /* 2e ro. */
    }

    /*
     * Register the PCI device, it's I/O regions, the timer and the
     * saved state item.
     */
    rc = PDMDevHlpPCIRegister(pDevIns, &pThis->PciDev);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0, 256, PCI_ADDRESS_SPACE_IO, ichac97IOPortMap);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 1, 64, PCI_ADDRESS_SPACE_IO, ichac97IOPortMap);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpSSMRegister(pDevIns, AC97_SSM_VERSION, sizeof(*pThis), ichac97SaveExec, ichac97LoadExec);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_AUDIO_AC97_ASYNC_IO
    LogRel(("AC97: Asynchronous I/O enabled\n"));
#endif

    /*
     * Attach driver.
     */
    uint8_t uLUN;
    for (uLUN = 0; uLUN < UINT8_MAX; ++uLUN)
    {
        LogFunc(("Trying to attach driver for LUN #%RU8 ...\n", uLUN));
        rc = ichac97AttachInternal(pThis, uLUN, 0 /* fFlags */, NULL /* ppDrv */);
        if (RT_FAILURE(rc))
        {
            if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
                rc = VINF_SUCCESS;
            else if (rc == VERR_AUDIO_BACKEND_INIT_FAILED)
            {
                ichac97ReattachInternal(pThis, NULL /* pDrv */, uLUN, "NullAudio");
                PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "HostAudioNotResponding",
                        N_("Host audio backend initialization has failed. Selecting the NULL audio backend "
                            "with the consequence that no sound is audible"));
                /* Attaching to the NULL audio backend will never fail. */
                rc = VINF_SUCCESS;
            }
            break;
        }
    }

    LogFunc(("cLUNs=%RU8, rc=%Rrc\n", uLUN, rc));

    if (RT_SUCCESS(rc))
    {
        rc = AudioMixerCreate("AC'97 Mixer", 0 /* uFlags */, &pThis->pMixer);
        if (RT_SUCCESS(rc))
        {
            rc = AudioMixerCreateSink(pThis->pMixer, "[Recording] Line In", AUDMIXSINKDIR_INPUT, &pThis->pSinkLineIn);
            AssertRC(rc);
            rc = AudioMixerCreateSink(pThis->pMixer, "[Recording] Microphone In", AUDMIXSINKDIR_INPUT, &pThis->pSinkMicIn);
            AssertRC(rc);
            rc = AudioMixerCreateSink(pThis->pMixer, "[Playback] PCM Output", AUDMIXSINKDIR_OUTPUT, &pThis->pSinkOut);
            AssertRC(rc);
        }
    }

    if (RT_SUCCESS(rc))
    {
        /*
         * Create all hardware streams.
         */
        rc = ichac97StreamCreate(pThis, &pThis->StreamLineIn, AC97SOUNDSOURCE_PI_INDEX);
        if (RT_SUCCESS(rc))
        {
            rc = ichac97StreamCreate(pThis, &pThis->StreamMicIn, AC97SOUNDSOURCE_MC_INDEX);
            if (RT_SUCCESS(rc))
                rc = ichac97StreamCreate(pThis, &pThis->StreamOut, AC97SOUNDSOURCE_PO_INDEX);
        }

#ifdef VBOX_WITH_AUDIO_AC97_ONETIME_INIT
        PAC97DRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, AC97DRIVER, Node)
        {
            /*
             * Only primary drivers are critical for the VM to run. Everything else
             * might not worth showing an own error message box in the GUI.
             */
            if (!(pDrv->fFlags & PDMAUDIODRVFLAGS_PRIMARY))
                continue;

            PPDMIAUDIOCONNECTOR pCon = pDrv->pConnector;
            AssertPtr(pCon);

            bool fValidLineIn = AudioMixerStreamIsValid(pDrv->LineIn.pMixStrm);
            bool fValidMicIn  = AudioMixerStreamIsValid(pDrv->MicIn.pMixStrm);
            bool fValidOut    = AudioMixerStreamIsValid(pDrv->Out.pMixStrm);

            if (    !fValidLineIn
                 && !fValidMicIn
                 && !fValidOut)
            {
                LogRel(("AC97: Falling back to NULL backend (no sound audible)\n"));

                ichac97Reset(pDevIns);
                ichac97ReattachInternal(pThis, pDrv, pDrv->uLUN, "NullAudio");

                PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "HostAudioNotResponding",
                    N_("No audio devices could be opened. Selecting the NULL audio backend "
                       "with the consequence that no sound is audible"));
            }
            else
            {
                bool fWarn = false;

                PDMAUDIOBACKENDCFG backendCfg;
                int rc2 = pCon->pfnGetConfig(pCon, &backendCfg);
                if (RT_SUCCESS(rc2))
                {
                    if (backendCfg.cMaxStreamsIn)
                    {
                        /* If the audio backend supports two or more input streams at once,
                         * warn if one of our two inputs (microphone-in and line-in) failed to initialize. */
                        if (backendCfg.cMaxStreamsIn >= 2)
                            fWarn = !fValidLineIn || !fValidMicIn;
                        /* If the audio backend only supports one input stream at once (e.g. pure ALSA, and
                         * *not* ALSA via PulseAudio plugin!), only warn if both of our inputs failed to initialize.
                         * One of the two simply is not in use then. */
                        else if (backendCfg.cMaxStreamsIn == 1)
                            fWarn = !fValidLineIn && !fValidMicIn;
                        /* Don't warn if our backend is not able of supporting any input streams at all. */
                    }

                    if (   !fWarn
                        && backendCfg.cMaxStreamsOut)
                    {
                        fWarn = !fValidOut;
                    }
                }
                else
                {
                    LogRel(("AC97: Unable to retrieve audio backend configuration for LUN #%RU8, rc=%Rrc\n", pDrv->uLUN, rc2));
                    fWarn = true;
                }

                if (fWarn)
                {
                    char   szMissingStreams[255] = "";
                    size_t len = 0;
                    if (!fValidLineIn)
                    {
                        LogRel(("AC97: WARNING: Unable to open PCM line input for LUN #%RU8!\n", pDrv->uLUN));
                        len = RTStrPrintf(szMissingStreams, sizeof(szMissingStreams), "PCM Input");
                    }
                    if (!fValidMicIn)
                    {
                        LogRel(("AC97: WARNING: Unable to open PCM microphone input for LUN #%RU8!\n", pDrv->uLUN));
                        len += RTStrPrintf(szMissingStreams + len,
                                           sizeof(szMissingStreams) - len, len ? ", PCM Microphone" : "PCM Microphone");
                    }
                    if (!fValidOut)
                    {
                        LogRel(("AC97: WARNING: Unable to open PCM output for LUN #%RU8!\n", pDrv->uLUN));
                        len += RTStrPrintf(szMissingStreams + len,
                                           sizeof(szMissingStreams) - len, len ? ", PCM Output" : "PCM Output");
                    }

                    PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "HostAudioNotResponding",
                                               N_("Some AC'97 audio streams (%s) could not be opened. Guest applications generating audio "
                                                  "output or depending on audio input may hang. Make sure your host audio device "
                                                  "is working properly. Check the logfile for error messages of the audio "
                                                  "subsystem"), szMissingStreams);
                }
            }
        }
#endif /* VBOX_WITH_AUDIO_AC97_ONETIME_INIT */
    }

    if (RT_SUCCESS(rc))
        ichac97Reset(pDevIns);

#ifndef VBOX_WITH_AUDIO_AC97_CALLBACKS
    if (RT_SUCCESS(rc))
    {
        /* Create the emulation timer.
         *
         * Note:  Use TMCLOCK_VIRTUAL_SYNC here, as the guest's AC'97 driver
         *        relies on exact (virtual) DMA timing and uses DMA Position Buffers
         *        instead of the LPIB registers.
         */
        rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL_SYNC, ichac97Timer, pThis,
                                    TMTIMER_FLAGS_NO_CRIT_SECT, "AC'97 Timer", &pThis->pTimer);
        AssertRCReturn(rc, rc);

        /* Use our own critcal section for the device timer.
         * That way we can control more fine-grained when to lock what. */
        rc = TMR3TimerSetCritSect(pThis->pTimer, &pThis->CritSect);
        AssertRCReturn(rc, rc);

        pThis->cTimerTicks = TMTimerGetFreq(pThis->pTimer) / uTimerHz;
        pThis->uTimerTS    = TMTimerGet(pThis->pTimer);
        LogFunc(("Timer ticks=%RU64 (%RU16 Hz)\n", pThis->cTimerTicks, uTimerHz));
    }
#else /* !VBOX_WITH_AUDIO_AC97_CALLBACKS */
    if (RT_SUCCESS(rc))
    {
        PAC97DRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, AC97DRIVER, Node)
        {
            /* Only register primary driver.
             * The device emulation does the output multiplexing then. */
            if (!(pDrv->fFlags & PDMAUDIODRVFLAGS_PRIMARY))
                continue;

            PDMAUDIOCBRECORD AudioCallbacks[2];

            AC97CALLBACKCTX Ctx = { pThis, pDrv };

            AudioCallbacks[0].enmType     = PDMAUDIOCALLBACKTYPE_INPUT;
            AudioCallbacks[0].pfnCallback = ac97CallbackInput;
            AudioCallbacks[0].pvCtx       = &Ctx;
            AudioCallbacks[0].cbCtx       = sizeof(AC97CALLBACKCTX);

            AudioCallbacks[1].enmType     = PDMAUDIOCALLBACKTYPE_OUTPUT;
            AudioCallbacks[1].pfnCallback = ac97CallbackOutput;
            AudioCallbacks[1].pvCtx       = &Ctx;
            AudioCallbacks[1].cbCtx       = sizeof(AC97CALLBACKCTX);

            rc = pDrv->pConnector->pfnRegisterCallbacks(pDrv->pConnector, AudioCallbacks, RT_ELEMENTS(AudioCallbacks));
            if (RT_FAILURE(rc))
                break;
        }
    }
#endif /* VBOX_WITH_AUDIO_AC97_CALLBACKS */

#ifdef VBOX_WITH_STATISTICS
    if (RT_SUCCESS(rc))
    {
        /*
         * Register statistics.
         */
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatTimer,        STAMTYPE_PROFILE, "/Devices/AC97/Timer",        STAMUNIT_TICKS_PER_CALL, "Profiling ichac97Timer.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatIn,           STAMTYPE_PROFILE, "/Devices/AC97/Input",        STAMUNIT_TICKS_PER_CALL, "Profiling input.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatOut,          STAMTYPE_PROFILE, "/Devices/AC97/Output",       STAMUNIT_TICKS_PER_CALL, "Profiling output.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatBytesRead,    STAMTYPE_COUNTER, "/Devices/AC97/BytesRead"   , STAMUNIT_BYTES,          "Bytes read from AC97 emulation.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatBytesWritten, STAMTYPE_COUNTER, "/Devices/AC97/BytesWritten", STAMUNIT_BYTES,          "Bytes written to AC97 emulation.");
    }
#endif

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97DMARead.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97DMAWrite.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97StreamRead.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "ac97StreamWrite.pcm");
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceICHAC97 =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "ichac97",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "ICH AC'97 Audio Controller",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_AUDIO,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(AC97STATE),
    /* pfnConstruct */
    ichac97Construct,
    /* pfnDestruct */
    ichac97Destruct,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    ichac97Reset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    ichac97Attach,
    /* pfnDetach */
    ichac97Detach,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    ichac97PowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

#endif /* !IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */

