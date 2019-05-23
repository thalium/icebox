/* $Id: DevHDA.cpp $ */
/** @file
 * DevHDA.cpp - VBox Intel HD Audio Controller.
 *
 * Implemented against the specifications found in "High Definition Audio
 * Specification", Revision 1.0a June 17, 2010, and  "Intel I/O Controller
 * HUB 6 (ICH6) Family, Datasheet", document number 301473-002.
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
#define LOG_GROUP LOG_GROUP_DEV_HDA
#include <VBox/log.h>

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/version.h>

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>
#include <iprt/file.h>
#include <iprt/list.h>
#ifdef IN_RING3
# include <iprt/mem.h>
# include <iprt/semaphore.h>
# include <iprt/string.h>
# include <iprt/uuid.h>
#endif

#include "VBoxDD.h"

#include "AudioMixBuffer.h"
#include "AudioMixer.h"

#include "DevHDA.h"
#include "DevHDACommon.h"

#include "HDACodec.h"
#include "HDAStream.h"
# if defined(VBOX_WITH_HDA_AUDIO_INTERLEAVING_STREAMS_SUPPORT) || defined(VBOX_WITH_AUDIO_HDA_51_SURROUND)
#  include "HDAStreamChannel.h"
# endif
#include "HDAStreamMap.h"
#include "HDAStreamPeriod.h"

#include "DrvAudio.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
//#define HDA_AS_PCI_EXPRESS
#define VBOX_WITH_INTEL_HDA

/* Installs a DMA access handler (via PGM callback) to monitor
 * HDA's DMA operations, that is, writing / reading audio stream data.
 *
 * !!! Note: Certain guests are *that* timing sensitive that when enabling  !!!
 * !!!       such a handler will mess up audio completely (e.g. Windows 7). !!! */
//#define HDA_USE_DMA_ACCESS_HANDLER
#ifdef HDA_USE_DMA_ACCESS_HANDLER
# include <VBox/vmm/pgm.h>
#endif

/* Uses the DMA access handler to read the written DMA audio (output) data.
 * Only valid if HDA_USE_DMA_ACCESS_HANDLER is set.
 *
 * Also see the note / warning for HDA_USE_DMA_ACCESS_HANDLER. */
//# define HDA_USE_DMA_ACCESS_HANDLER_WRITING

/* Useful to debug the device' timing. */
//#define HDA_DEBUG_TIMING

/* To debug silence coming from the guest in form of audio gaps.
 * Very crude implementation for now. */
//#define HDA_DEBUG_SILENCE

#if defined(VBOX_WITH_HP_HDA)
/* HP Pavilion dv4t-1300 */
# define HDA_PCI_VENDOR_ID 0x103c
# define HDA_PCI_DEVICE_ID 0x30f7
#elif defined(VBOX_WITH_INTEL_HDA)
/* Intel HDA controller */
# define HDA_PCI_VENDOR_ID 0x8086
# define HDA_PCI_DEVICE_ID 0x2668
#elif defined(VBOX_WITH_NVIDIA_HDA)
/* nVidia HDA controller */
# define HDA_PCI_VENDOR_ID 0x10de
# define HDA_PCI_DEVICE_ID 0x0ac0
#else
# error "Please specify your HDA device vendor/device IDs"
#endif

/* Make sure that interleaving streams support is enabled if the 5.1 surround code is being used. */
#if defined (VBOX_WITH_AUDIO_HDA_51_SURROUND) && !defined(VBOX_WITH_HDA_AUDIO_INTERLEAVING_STREAMS_SUPPORT)
# define VBOX_WITH_HDA_AUDIO_INTERLEAVING_STREAMS_SUPPORT
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Structure defining a (host backend) driver stream.
 * Each driver has its own instances of audio mixer streams, which then
 * can go into the same (or even different) audio mixer sinks.
 */
typedef struct HDADRIVERSTREAM
{
    union
    {
        /** Desired playback destination (for an output stream). */
        PDMAUDIOPLAYBACKDEST           Dest;
        /** Desired recording source (for an input stream). */
        PDMAUDIORECSOURCE              Source;
    } DestSource;
    uint8_t                            Padding1[4];
    /** Associated mixer handle. */
    R3PTRTYPE(PAUDMIXSTREAM)           pMixStrm;
} HDADRIVERSTREAM, *PHDADRIVERSTREAM;

#ifdef HDA_USE_DMA_ACCESS_HANDLER
/**
 * Struct for keeping an HDA DMA access handler context.
 */
typedef struct HDADMAACCESSHANDLER
{
    /** Node for storing this handler in our list in HDASTREAMSTATE. */
    RTLISTNODER3            Node;
    /** Pointer to stream to which this access handler is assigned to. */
    R3PTRTYPE(PHDASTREAM)   pStream;
    /** Access handler type handle. */
    PGMPHYSHANDLERTYPE      hAccessHandlerType;
    /** First address this handler uses. */
    RTGCPHYS                GCPhysFirst;
    /** Last address this handler uses. */
    RTGCPHYS                GCPhysLast;
    /** Actual BDLE address to handle. */
    RTGCPHYS                BDLEAddr;
    /** Actual BDLE buffer size to handle. */
    RTGCPHYS                BDLESize;
    /** Whether the access handler has been registered or not. */
    bool                    fRegistered;
    uint8_t                 Padding[3];
} HDADMAACCESSHANDLER, *PHDADMAACCESSHANDLER;
#endif

/**
 * Struct for maintaining a host backend driver.
 * This driver must be associated to one, and only one,
 * HDA codec. The HDA controller does the actual multiplexing
 * of HDA codec data to various host backend drivers then.
 *
 * This HDA device uses a timer in order to synchronize all
 * read/write accesses across all attached LUNs / backends.
 */
typedef struct HDADRIVER
{
    /** Node for storing this driver in our device driver list of HDASTATE. */
    RTLISTNODER3                       Node;
    /** Pointer to HDA controller (state). */
    R3PTRTYPE(PHDASTATE)               pHDAState;
    /** Driver flags. */
    PDMAUDIODRVFLAGS                   fFlags;
    uint8_t                            u32Padding0[2];
    /** LUN to which this driver has been assigned. */
    uint8_t                            uLUN;
    /** Whether this driver is in an attached state or not. */
    bool                               fAttached;
    /** Pointer to attached driver base interface. */
    R3PTRTYPE(PPDMIBASE)               pDrvBase;
    /** Audio connector interface to the underlying host backend. */
    R3PTRTYPE(PPDMIAUDIOCONNECTOR)     pConnector;
    /** Mixer stream for line input. */
    HDADRIVERSTREAM                    LineIn;
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    /** Mixer stream for mic input. */
    HDADRIVERSTREAM                    MicIn;
#endif
    /** Mixer stream for front output. */
    HDADRIVERSTREAM                    Front;
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    /** Mixer stream for center/LFE output. */
    HDADRIVERSTREAM                    CenterLFE;
    /** Mixer stream for rear output. */
    HDADRIVERSTREAM                    Rear;
#endif
} HDADRIVER;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#ifndef VBOX_DEVICE_STRUCT_TESTCASE
#ifdef IN_RING3
static void hdaGCTLReset(PHDASTATE pThis);
#endif

/** @name Register read/write stubs.
 * @{
 */
static int hdaRegReadUnimpl(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegWriteUnimpl(PHDASTATE pThis, uint32_t iReg, uint32_t pu32Value);
/** @} */

/** @name Global register set read/write functions.
 * @{
 */
static int hdaRegWriteGCTL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegReadLPIB(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegReadWALCLK(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegWriteCORBWP(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteCORBRP(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteCORBCTL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteCORBSTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteRIRBWP(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteRIRBSTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSTATESTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteIRS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegReadIRS(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegWriteBase(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
/** @} */

/** @name {IOB}SDn write functions.
 * @{
 */
static int hdaRegWriteSDCBL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDCTL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDSTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDLVI(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDFIFOW(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDFIFOS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDFMT(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDBDPL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegWriteSDBDPU(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
/** @} */

/** @name Generic register read/write functions.
 * @{
 */
static int hdaRegReadU32(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegWriteU32(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegReadU24(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
#ifdef IN_RING3
static int hdaRegWriteU24(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
#endif
static int hdaRegReadU16(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegWriteU16(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
static int hdaRegReadU8(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
static int hdaRegWriteU8(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
/** @} */

/** @name HDA device functions.
 * @{
 */
#ifdef IN_RING3
# ifdef HDA_USE_DMA_ACCESS_HANDLER
static DECLCALLBACK(VBOXSTRICTRC) hdaDMAAccessHandler(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void *pvPhys, void *pvBuf, size_t cbBuf, PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser);
# endif
static void                       hdaDoTransfers(PHDASTATE pThis);
#endif /* IN_RING3 */
/** @} */

/** @name Timer functions.
 * @{
 */
#if !defined(VBOX_WITH_AUDIO_HDA_CALLBACKS) && defined(IN_RING3)
static int           hdaTimerMaybeStart(PHDASTATE pThis);
static int           hdaTimerMaybeStop(PHDASTATE pThis);
static void          hdaTimerMain(PHDASTATE pThis);
#endif
/** @} */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/** No register description (RD) flags defined. */
#define HDA_RD_FLAG_NONE           0
/** Writes to SD are allowed while RUN bit is set. */
#define HDA_RD_FLAG_SD_WRITE_RUN   RT_BIT(0)

/** Emits a single audio stream register set (e.g. OSD0) at a specified offset. */
#define HDA_REG_MAP_STRM(offset, name) \
    /* offset        size     read mask   write mask  flags                     read callback   write callback     index + abbrev                 description */ \
    /* -------       -------  ----------  ----------  ------------------------- --------------  -----------------  -----------------------------  ----------- */ \
    /* Offset 0x80 (SD0) */ \
    { offset,        0x00003, 0x00FF001F, 0x00F0001F, HDA_RD_FLAG_SD_WRITE_RUN, hdaRegReadU24 , hdaRegWriteSDCTL  , HDA_REG_IDX_STRM(name, CTL)  , #name " Stream Descriptor Control" }, \
    /* Offset 0x83 (SD0) */ \
    { offset + 0x3,  0x00001, 0x0000003C, 0x0000001C, HDA_RD_FLAG_SD_WRITE_RUN, hdaRegReadU8  , hdaRegWriteSDSTS  , HDA_REG_IDX_STRM(name, STS)  , #name " Status" }, \
    /* Offset 0x84 (SD0) */ \
    { offset + 0x4,  0x00004, 0xFFFFFFFF, 0x00000000, HDA_RD_FLAG_NONE,         hdaRegReadLPIB, hdaRegWriteU32    , HDA_REG_IDX_STRM(name, LPIB) , #name " Link Position In Buffer" }, \
    /* Offset 0x88 (SD0) */ \
    { offset + 0x8,  0x00004, 0xFFFFFFFF, 0xFFFFFFFF, HDA_RD_FLAG_NONE,         hdaRegReadU32 , hdaRegWriteSDCBL  , HDA_REG_IDX_STRM(name, CBL)  , #name " Cyclic Buffer Length" }, \
    /* Offset 0x8C (SD0) */ \
    { offset + 0xC,  0x00002, 0x0000FFFF, 0x0000FFFF, HDA_RD_FLAG_NONE,         hdaRegReadU16 , hdaRegWriteSDLVI  , HDA_REG_IDX_STRM(name, LVI)  , #name " Last Valid Index" }, \
    /* Reserved: FIFO Watermark. ** @todo Document this! */ \
    { offset + 0xE,  0x00002, 0x00000007, 0x00000007, HDA_RD_FLAG_NONE,         hdaRegReadU16 , hdaRegWriteSDFIFOW, HDA_REG_IDX_STRM(name, FIFOW), #name " FIFO Watermark" }, \
    /* Offset 0x90 (SD0) */ \
    { offset + 0x10, 0x00002, 0x000000FF, 0x000000FF, HDA_RD_FLAG_NONE,         hdaRegReadU16 , hdaRegWriteSDFIFOS, HDA_REG_IDX_STRM(name, FIFOS), #name " FIFO Size" }, \
    /* Offset 0x92 (SD0) */ \
    { offset + 0x12, 0x00002, 0x00007F7F, 0x00007F7F, HDA_RD_FLAG_NONE,         hdaRegReadU16 , hdaRegWriteSDFMT  , HDA_REG_IDX_STRM(name, FMT)  , #name " Stream Format" }, \
    /* Reserved: 0x94 - 0x98. */ \
    /* Offset 0x98 (SD0) */ \
    { offset + 0x18, 0x00004, 0xFFFFFF80, 0xFFFFFF80, HDA_RD_FLAG_NONE,         hdaRegReadU32 , hdaRegWriteSDBDPL , HDA_REG_IDX_STRM(name, BDPL) , #name " Buffer Descriptor List Pointer-Lower Base Address" }, \
    /* Offset 0x9C (SD0) */ \
    { offset + 0x1C, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, HDA_RD_FLAG_NONE,         hdaRegReadU32 , hdaRegWriteSDBDPU , HDA_REG_IDX_STRM(name, BDPU) , #name " Buffer Descriptor List Pointer-Upper Base Address" }

/** Defines a single audio stream register set (e.g. OSD0). */
#define HDA_REG_MAP_DEF_STREAM(index, name) \
    HDA_REG_MAP_STRM(HDA_REG_DESC_SD0_BASE + (index * 32 /* 0x20 */), name)

/* See 302349 p 6.2. */
const HDAREGDESC g_aHdaRegMap[HDA_NUM_REGS] =
{
    /* offset  size     read mask   write mask  flags             read callback     write callback       index + abbrev              */
    /*-------  -------  ----------  ----------  ----------------- ----------------  -------------------     ------------------------ */
    { 0x00000, 0x00002, 0x0000FFFB, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteUnimpl  , HDA_REG_IDX(GCAP)         }, /* Global Capabilities */
    { 0x00002, 0x00001, 0x000000FF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteUnimpl  , HDA_REG_IDX(VMIN)         }, /* Minor Version */
    { 0x00003, 0x00001, 0x000000FF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteUnimpl  , HDA_REG_IDX(VMAJ)         }, /* Major Version */
    { 0x00004, 0x00002, 0x0000FFFF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteU16     , HDA_REG_IDX(OUTPAY)       }, /* Output Payload Capabilities */
    { 0x00006, 0x00002, 0x0000FFFF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteUnimpl  , HDA_REG_IDX(INPAY)        }, /* Input Payload Capabilities */
    { 0x00008, 0x00004, 0x00000103, 0x00000103, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteGCTL    , HDA_REG_IDX(GCTL)         }, /* Global Control */
    { 0x0000c, 0x00002, 0x00007FFF, 0x00007FFF, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteU16     , HDA_REG_IDX(WAKEEN)       }, /* Wake Enable */
    { 0x0000e, 0x00002, 0x00000007, 0x00000007, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteSTATESTS, HDA_REG_IDX(STATESTS)     }, /* State Change Status */
    { 0x00010, 0x00002, 0xFFFFFFFF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadUnimpl, hdaRegWriteUnimpl  , HDA_REG_IDX(GSTS)         }, /* Global Status */
    { 0x00018, 0x00002, 0x0000FFFF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteU16     , HDA_REG_IDX(OUTSTRMPAY)   }, /* Output Stream Payload Capability */
    { 0x0001A, 0x00002, 0x0000FFFF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteUnimpl  , HDA_REG_IDX(INSTRMPAY)    }, /* Input Stream Payload Capability */
    { 0x00020, 0x00004, 0xC00000FF, 0xC00000FF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteU32     , HDA_REG_IDX(INTCTL)       }, /* Interrupt Control */
    { 0x00024, 0x00004, 0xC00000FF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteUnimpl  , HDA_REG_IDX(INTSTS)       }, /* Interrupt Status */
    { 0x00030, 0x00004, 0xFFFFFFFF, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadWALCLK, hdaRegWriteUnimpl  , HDA_REG_IDX_NOMEM(WALCLK) }, /* Wall Clock Counter */
    { 0x00034, 0x00004, 0x000000FF, 0x000000FF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteU32     , HDA_REG_IDX(SSYNC)        }, /* Stream Synchronization */
    { 0x00040, 0x00004, 0xFFFFFF80, 0xFFFFFF80, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteBase    , HDA_REG_IDX(CORBLBASE)    }, /* CORB Lower Base Address */
    { 0x00044, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteBase    , HDA_REG_IDX(CORBUBASE)    }, /* CORB Upper Base Address */
    { 0x00048, 0x00002, 0x000000FF, 0x000000FF, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteCORBWP  , HDA_REG_IDX(CORBWP)       }, /* CORB Write Pointer */
    { 0x0004A, 0x00002, 0x000080FF, 0x000080FF, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteCORBRP  , HDA_REG_IDX(CORBRP)       }, /* CORB Read Pointer */
    { 0x0004C, 0x00001, 0x00000003, 0x00000003, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteCORBCTL , HDA_REG_IDX(CORBCTL)      }, /* CORB Control */
    { 0x0004D, 0x00001, 0x00000001, 0x00000001, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteCORBSTS , HDA_REG_IDX(CORBSTS)      }, /* CORB Status */
    { 0x0004E, 0x00001, 0x000000F3, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteUnimpl  , HDA_REG_IDX(CORBSIZE)     }, /* CORB Size */
    { 0x00050, 0x00004, 0xFFFFFF80, 0xFFFFFF80, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteBase    , HDA_REG_IDX(RIRBLBASE)    }, /* RIRB Lower Base Address */
    { 0x00054, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteBase    , HDA_REG_IDX(RIRBUBASE)    }, /* RIRB Upper Base Address */
    { 0x00058, 0x00002, 0x000000FF, 0x00008000, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteRIRBWP  , HDA_REG_IDX(RIRBWP)       }, /* RIRB Write Pointer */
    { 0x0005A, 0x00002, 0x000000FF, 0x000000FF, HDA_RD_FLAG_NONE, hdaRegReadU16   , hdaRegWriteU16     , HDA_REG_IDX(RINTCNT)      }, /* Response Interrupt Count */
    { 0x0005C, 0x00001, 0x00000007, 0x00000007, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteU8      , HDA_REG_IDX(RIRBCTL)      }, /* RIRB Control */
    { 0x0005D, 0x00001, 0x00000005, 0x00000005, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteRIRBSTS , HDA_REG_IDX(RIRBSTS)      }, /* RIRB Status */
    { 0x0005E, 0x00001, 0x000000F3, 0x00000000, HDA_RD_FLAG_NONE, hdaRegReadU8    , hdaRegWriteUnimpl  , HDA_REG_IDX(RIRBSIZE)     }, /* RIRB Size */
    { 0x00060, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteU32     , HDA_REG_IDX(IC)           }, /* Immediate Command */
    { 0x00064, 0x00004, 0x00000000, 0xFFFFFFFF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteUnimpl  , HDA_REG_IDX(IR)           }, /* Immediate Response */
    { 0x00068, 0x00002, 0x00000002, 0x00000002, HDA_RD_FLAG_NONE, hdaRegReadIRS   , hdaRegWriteIRS     , HDA_REG_IDX(IRS)          }, /* Immediate Command Status */
    { 0x00070, 0x00004, 0xFFFFFFFF, 0xFFFFFF81, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteBase    , HDA_REG_IDX(DPLBASE)      }, /* DMA Position Lower Base */
    { 0x00074, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, HDA_RD_FLAG_NONE, hdaRegReadU32   , hdaRegWriteBase    , HDA_REG_IDX(DPUBASE)      }, /* DMA Position Upper Base */
    /* 4 Serial Data In (SDI). */
    HDA_REG_MAP_DEF_STREAM(0, SD0),
    HDA_REG_MAP_DEF_STREAM(1, SD1),
    HDA_REG_MAP_DEF_STREAM(2, SD2),
    HDA_REG_MAP_DEF_STREAM(3, SD3),
    /* 4 Serial Data Out (SDO). */
    HDA_REG_MAP_DEF_STREAM(4, SD4),
    HDA_REG_MAP_DEF_STREAM(5, SD5),
    HDA_REG_MAP_DEF_STREAM(6, SD6),
    HDA_REG_MAP_DEF_STREAM(7, SD7)
};

const HDAREGALIAS g_aHdaRegAliases[] =
{
    { 0x2084, HDA_REG_SD0LPIB },
    { 0x20a4, HDA_REG_SD1LPIB },
    { 0x20c4, HDA_REG_SD2LPIB },
    { 0x20e4, HDA_REG_SD3LPIB },
    { 0x2104, HDA_REG_SD4LPIB },
    { 0x2124, HDA_REG_SD5LPIB },
    { 0x2144, HDA_REG_SD6LPIB },
    { 0x2164, HDA_REG_SD7LPIB }
};

#ifdef IN_RING3
/** HDABDLEDESC field descriptors for the v7 saved state. */
static SSMFIELD const g_aSSMBDLEDescFields7[] =
{
    SSMFIELD_ENTRY(HDABDLEDESC, u64BufAdr),
    SSMFIELD_ENTRY(HDABDLEDESC, u32BufSize),
    SSMFIELD_ENTRY(HDABDLEDESC, fFlags),
    SSMFIELD_ENTRY_TERM()
};

/** HDABDLESTATE field descriptors for the v6+ saved state. */
static SSMFIELD const g_aSSMBDLEStateFields6[] =
{
    SSMFIELD_ENTRY(HDABDLESTATE, u32BDLIndex),
    SSMFIELD_ENTRY(HDABDLESTATE, cbBelowFIFOW),
    SSMFIELD_ENTRY_OLD(FIFO,     HDA_FIFO_MAX), /* Deprecated; now is handled in the stream's circular buffer. */
    SSMFIELD_ENTRY(HDABDLESTATE, u32BufOff),
    SSMFIELD_ENTRY_TERM()
};

/** HDABDLESTATE field descriptors for the v7 saved state. */
static SSMFIELD const g_aSSMBDLEStateFields7[] =
{
    SSMFIELD_ENTRY(HDABDLESTATE, u32BDLIndex),
    SSMFIELD_ENTRY(HDABDLESTATE, cbBelowFIFOW),
    SSMFIELD_ENTRY(HDABDLESTATE, u32BufOff),
    SSMFIELD_ENTRY_TERM()
};

/** HDASTREAMSTATE field descriptors for the v6 saved state. */
static SSMFIELD const g_aSSMStreamStateFields6[] =
{
    SSMFIELD_ENTRY_OLD(cBDLE,      sizeof(uint16_t)), /* Deprecated. */
    SSMFIELD_ENTRY(HDASTREAMSTATE, uCurBDLE),
    SSMFIELD_ENTRY_OLD(fStop,      1),                /* Deprecated; see SSMR3PutBool(). */
    SSMFIELD_ENTRY_OLD(fRunning,   1),                /* Deprecated; using the HDA_SDCTL_RUN bit is sufficient. */
    SSMFIELD_ENTRY(HDASTREAMSTATE, fInReset),
    SSMFIELD_ENTRY_TERM()
};

/** HDASTREAMSTATE field descriptors for the v7 saved state. */
static SSMFIELD const g_aSSMStreamStateFields7[] =
{
    SSMFIELD_ENTRY(HDASTREAMSTATE, uCurBDLE),
    SSMFIELD_ENTRY(HDASTREAMSTATE, fInReset),
    SSMFIELD_ENTRY(HDASTREAMSTATE, uTimerTS),
    SSMFIELD_ENTRY_TERM()
};

/** HDASTREAMPERIOD field descriptors for the v7 saved state. */
static SSMFIELD const g_aSSMStreamPeriodFields7[] =
{
    SSMFIELD_ENTRY(HDASTREAMPERIOD, u64StartWalClk),
    SSMFIELD_ENTRY(HDASTREAMPERIOD, u64ElapsedWalClk),
    SSMFIELD_ENTRY(HDASTREAMPERIOD, framesTransferred),
    SSMFIELD_ENTRY(HDASTREAMPERIOD, cIntPending),
    SSMFIELD_ENTRY_TERM()
};
#endif

/**
 * 32-bit size indexed masks, i.e. g_afMasks[2 bytes] = 0xffff.
 */
static uint32_t const g_afMasks[5] =
{
    UINT32_C(0), UINT32_C(0x000000ff), UINT32_C(0x0000ffff), UINT32_C(0x00ffffff), UINT32_C(0xffffffff)
};

/**
 * Acquires the HDA lock.
 */
#define DEVHDA_LOCK(a_pThis) \
    do { \
        int rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, VERR_IGNORED); \
        AssertRC(rcLock); \
    } while (0)

/**
 * Acquires the HDA lock or returns.
 */
# define DEVHDA_LOCK_RETURN(a_pThis, a_rcBusy) \
    do { \
        int rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, a_rcBusy); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            return rcLock; \
        } \
    } while (0)

/**
 * Acquires the HDA lock or returns.
 */
# define DEVHDA_LOCK_RETURN_VOID(a_pThis) \
    do { \
        int rcLock = PDMCritSectEnter(&(a_pThis)->CritSect, VERR_IGNORED); \
        if (rcLock != VINF_SUCCESS) \
        { \
            AssertRC(rcLock); \
            return; \
        } \
    } while (0)

/**
 * Releases the HDA lock.
 */
#define DEVHDA_UNLOCK(a_pThis) \
    do { PDMCritSectLeave(&(a_pThis)->CritSect); } while (0)

/**
 * Acquires the TM lock and HDA lock, returns on failure.
 */
#define DEVHDA_LOCK_BOTH_RETURN_VOID(a_pThis) \
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
 * Acquires the TM lock and HDA lock, returns on failure.
 */
#define DEVHDA_LOCK_BOTH_RETURN(a_pThis, a_rcBusy) \
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
 * Releases the HDA lock and TM lock.
 */
#define DEVHDA_UNLOCK_BOTH(a_pThis) \
    do { \
        PDMCritSectLeave(&(a_pThis)->CritSect); \
        TMTimerUnlock((a_pThis)->pTimer); \
    } while (0)

#ifdef IN_RING3
/**
 * Retrieves the number of bytes of a FIFOW register.
 *
 * @return Number of bytes of a given FIFOW register.
 */
DECLINLINE(uint8_t) hdaSDFIFOWToBytes(uint32_t u32RegFIFOW)
{
    uint32_t cb;
    switch (u32RegFIFOW)
    {
        case HDA_SDFIFOW_8B:  cb = 8;  break;
        case HDA_SDFIFOW_16B: cb = 16; break;
        case HDA_SDFIFOW_32B: cb = 32; break;
        default:              cb = 0;  break;
    }

    Assert(RT_IS_POWER_OF_TWO(cb));
    return cb;
}

/**
 * Reschedules pending interrupts for all audio streams which have complete
 * audio periods but did not have the chance to issue their (pending) interrupts yet.
 *
 * @param   pThis               The HDA device state.
 */
static void hdaReschedulePendingInterrupts(PHDASTATE pThis)
{
    bool fInterrupt = false;

    for (uint8_t i = 0; i < HDA_MAX_STREAMS; ++i)
    {
        PHDASTREAM pStream = hdaGetStreamFromSD(pThis, i);
        if (!pStream)
            continue;

        if (   hdaStreamPeriodIsComplete    (&pStream->State.Period)
            && hdaStreamPeriodNeedsInterrupt(&pStream->State.Period)
            && hdaWalClkSet(pThis, hdaStreamPeriodGetAbsElapsedWalClk(&pStream->State.Period), false /* fForce */))
        {
            fInterrupt = true;
            break;
        }
    }

    LogFunc(("fInterrupt=%RTbool\n", fInterrupt));

#ifndef DEBUG
    hdaProcessInterrupt(pThis);
#else
    hdaProcessInterrupt(pThis, __FUNCTION__);
#endif
}
#endif

/**
 * Looks up a register at the exact offset given by @a offReg.
 *
 * @returns Register index on success, -1 if not found.
 * @param   offReg              The register offset.
 */
static int hdaRegLookup(uint32_t offReg)
{
    /*
     * Aliases.
     */
    if (offReg >= g_aHdaRegAliases[0].offReg)
    {
        for (unsigned i = 0; i < RT_ELEMENTS(g_aHdaRegAliases); i++)
            if (offReg == g_aHdaRegAliases[i].offReg)
                return g_aHdaRegAliases[i].idxAlias;
        Assert(g_aHdaRegMap[RT_ELEMENTS(g_aHdaRegMap) - 1].offset < offReg);
        return -1;
    }

    /*
     * Binary search the
     */
    int idxEnd  = RT_ELEMENTS(g_aHdaRegMap);
    int idxLow  = 0;
    for (;;)
    {
        int idxMiddle = idxLow + (idxEnd - idxLow) / 2;
        if (offReg < g_aHdaRegMap[idxMiddle].offset)
        {
            if (idxLow == idxMiddle)
                break;
            idxEnd = idxMiddle;
        }
        else if (offReg > g_aHdaRegMap[idxMiddle].offset)
        {
            idxLow = idxMiddle + 1;
            if (idxLow >= idxEnd)
                break;
        }
        else
            return idxMiddle;
    }

#ifdef RT_STRICT
    for (unsigned i = 0; i < RT_ELEMENTS(g_aHdaRegMap); i++)
        Assert(g_aHdaRegMap[i].offset != offReg);
#endif
    return -1;
}

/**
 * Looks up a register covering the offset given by @a offReg.
 *
 * @returns Register index on success, -1 if not found.
 * @param   offReg              The register offset.
 */
static int hdaRegLookupWithin(uint32_t offReg)
{
    /*
     * Aliases.
     */
    if (offReg >= g_aHdaRegAliases[0].offReg)
    {
        for (unsigned i = 0; i < RT_ELEMENTS(g_aHdaRegAliases); i++)
        {
            uint32_t off = offReg - g_aHdaRegAliases[i].offReg;
            if (off < 4 && off < g_aHdaRegMap[g_aHdaRegAliases[i].idxAlias].size)
                return g_aHdaRegAliases[i].idxAlias;
        }
        Assert(g_aHdaRegMap[RT_ELEMENTS(g_aHdaRegMap) - 1].offset < offReg);
        return -1;
    }

    /*
     * Binary search the register map.
     */
    int idxEnd  = RT_ELEMENTS(g_aHdaRegMap);
    int idxLow  = 0;
    for (;;)
    {
        int idxMiddle = idxLow + (idxEnd - idxLow) / 2;
        if (offReg < g_aHdaRegMap[idxMiddle].offset)
        {
            if (idxLow == idxMiddle)
                break;
            idxEnd = idxMiddle;
        }
        else if (offReg >= g_aHdaRegMap[idxMiddle].offset + g_aHdaRegMap[idxMiddle].size)
        {
            idxLow = idxMiddle + 1;
            if (idxLow >= idxEnd)
                break;
        }
        else
            return idxMiddle;
    }

#ifdef RT_STRICT
    for (unsigned i = 0; i < RT_ELEMENTS(g_aHdaRegMap); i++)
        Assert(offReg - g_aHdaRegMap[i].offset >= g_aHdaRegMap[i].size);
#endif
    return -1;
}

#ifdef IN_RING3
/**
 * Synchronizes the CORB / RIRB buffers between internal <-> device state.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA state.
 * @param   fLocal              Specify true to synchronize HDA state's CORB buffer with the device state,
 *                              or false to synchronize the device state's RIRB buffer with the HDA state.
 *
 * @todo r=andy Break this up into two functions?
 */
static int hdaCmdSync(PHDASTATE pThis, bool fLocal)
{
    int rc = VINF_SUCCESS;
    if (fLocal)
    {
        Assert((HDA_REG(pThis, CORBCTL) & HDA_CORBCTL_DMA));
        Assert(pThis->u64CORBBase);
        AssertPtr(pThis->pu32CorbBuf);
        Assert(pThis->cbCorbBuf);

        rc = PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), pThis->u64CORBBase, pThis->pu32CorbBuf, pThis->cbCorbBuf);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
# ifdef DEBUG_CMD_BUFFER
        uint8_t i = 0;
        do
        {
            LogFunc(("CORB%02x: ", i));
            uint8_t j = 0;
            do
            {
                const char *pszPrefix;
                if ((i + j) == HDA_REG(pThis, CORBRP));
                    pszPrefix = "[R]";
                else if ((i + j) == HDA_REG(pThis, CORBWP));
                    pszPrefix = "[W]";
                else
                    pszPrefix = "   "; /* three spaces */
                LogFunc(("%s%08x", pszPrefix, pThis->pu32CorbBuf[i + j]));
                j++;
            } while (j < 8);
            LogFunc(("\n"));
            i += 8;
        } while(i != 0);
# endif
    }
    else
    {
        Assert((HDA_REG(pThis, RIRBCTL) & HDA_RIRBCTL_RDMAEN));
        rc = PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns), pThis->u64RIRBBase, pThis->pu64RirbBuf, pThis->cbRirbBuf);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
# ifdef DEBUG_CMD_BUFFER
        uint8_t i = 0;
        do {
            LogFunc(("RIRB%02x: ", i));
            uint8_t j = 0;
            do {
                const char *prefix;
                if ((i + j) == HDA_REG(pThis, RIRBWP))
                    prefix = "[W]";
                else
                    prefix = "   ";
                LogFunc((" %s%016lx", prefix, pThis->pu64RirbBuf[i + j]));
            } while (++j < 8);
            LogFunc(("\n"));
            i += 8;
        } while (i != 0);
# endif
    }
    return rc;
}

/**
 * Processes the next CORB buffer command in the queue.
 * This will invoke the HDA codec verb dispatcher.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA state.
 */
static int hdaCORBCmdProcess(PHDASTATE pThis)
{
    int rc = hdaCmdSync(pThis, true);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);

    uint8_t corbRp = HDA_REG(pThis, CORBRP);
    uint8_t corbWp = HDA_REG(pThis, CORBWP);
    uint8_t rirbWp = HDA_REG(pThis, RIRBWP);

    Assert((corbWp != corbRp));
    Log3Func(("CORB(RP:%x, WP:%x) RIRBWP:%x\n", HDA_REG(pThis, CORBRP), HDA_REG(pThis, CORBWP), HDA_REG(pThis, RIRBWP)));

    while (corbRp != corbWp)
    {
        uint64_t uResp;
        uint32_t uCmd = pThis->pu32CorbBuf[++corbRp];

        int rc2 = pThis->pCodec->pfnLookup(pThis->pCodec, HDA_CODEC_CMD(uCmd, 0 /* Codec index */), &uResp);
        if (RT_FAILURE(rc2))
            LogFunc(("Codec lookup failed with rc=%Rrc\n", rc2));

        (rirbWp)++;

        if (   (uResp & CODEC_RESPONSE_UNSOLICITED)
            && !(HDA_REG(pThis, GCTL) & HDA_GCTL_UNSOL))
        {
            LogFunc(("Unexpected unsolicited response\n"));
            HDA_REG(pThis, CORBRP) = corbRp;
            return rc;
        }

        pThis->pu64RirbBuf[rirbWp] = uResp;

        pThis->u8RespIntCnt++;
        if (pThis->u8RespIntCnt == RINTCNT_N(pThis))
            break;
    }

    HDA_REG(pThis, CORBRP) = corbRp;
    HDA_REG(pThis, RIRBWP) = rirbWp;

    rc = hdaCmdSync(pThis, false);

    Log3Func(("CORB(RP:%x, WP:%x) RIRBWP:%x\n",
              HDA_REG(pThis, CORBRP), HDA_REG(pThis, CORBWP), HDA_REG(pThis, RIRBWP)));

    if (HDA_REG(pThis, RIRBCTL) & HDA_RIRBCTL_RINTCTL) /* Response Interrupt Control (RINTCTL) enabled? */
    {
        if (pThis->u8RespIntCnt)
        {
            pThis->u8RespIntCnt = 0;

            HDA_REG(pThis, RIRBSTS) |= HDA_RIRBSTS_RINTFL;

#ifndef DEBUG
            rc = hdaProcessInterrupt(pThis);
#else
            rc = hdaProcessInterrupt(pThis, __FUNCTION__);
#endif
        }
    }

    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);

    return rc;
}
#endif /* IN_RING3 */

/* Register access handlers. */

static int hdaRegReadUnimpl(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg);
    *pu32Value = 0;
    return VINF_SUCCESS;
}

static int hdaRegWriteUnimpl(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_SUCCESS;
}

/* U8 */
static int hdaRegReadU8(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Assert(((pThis->au32Regs[g_aHdaRegMap[iReg].mem_idx] & g_aHdaRegMap[iReg].readable) & 0xffffff00) == 0);
    return hdaRegReadU32(pThis, iReg, pu32Value);
}

static int hdaRegWriteU8(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    Assert((u32Value & 0xffffff00) == 0);
    return hdaRegWriteU32(pThis, iReg, u32Value);
}

/* U16 */
static int hdaRegReadU16(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Assert(((pThis->au32Regs[g_aHdaRegMap[iReg].mem_idx] & g_aHdaRegMap[iReg].readable) & 0xffff0000) == 0);
    return hdaRegReadU32(pThis, iReg, pu32Value);
}

static int hdaRegWriteU16(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    Assert((u32Value & 0xffff0000) == 0);
    return hdaRegWriteU32(pThis, iReg, u32Value);
}

/* U24 */
static int hdaRegReadU24(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Assert(((pThis->au32Regs[g_aHdaRegMap[iReg].mem_idx] & g_aHdaRegMap[iReg].readable) & 0xff000000) == 0);
    return hdaRegReadU32(pThis, iReg, pu32Value);
}

#ifdef IN_RING3
static int hdaRegWriteU24(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    Assert((u32Value & 0xff000000) == 0);
    return hdaRegWriteU32(pThis, iReg, u32Value);
}
#endif

/* U32 */
static int hdaRegReadU32(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t iRegMem = g_aHdaRegMap[iReg].mem_idx;

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_READ);

    *pu32Value = pThis->au32Regs[iRegMem] & g_aHdaRegMap[iReg].readable;

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
}

static int hdaRegWriteU32(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    uint32_t iRegMem = g_aHdaRegMap[iReg].mem_idx;

    pThis->au32Regs[iRegMem]  = (u32Value & g_aHdaRegMap[iReg].writable)
                              | (pThis->au32Regs[iRegMem] & ~g_aHdaRegMap[iReg].writable);
    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
}

static int hdaRegWriteGCTL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(iReg);

    if (u32Value & HDA_GCTL_CRST)
    {
        DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

        /* Set the CRST bit to indicate that we're leaving reset mode. */
        HDA_REG(pThis, GCTL) |= HDA_GCTL_CRST;
        LogFunc(("Guest leaving HDA reset\n"));

        DEVHDA_UNLOCK(pThis);
    }
    else
    {
#ifdef IN_RING3
        DEVHDA_LOCK(pThis);

        /* Enter reset state. */
        LogFunc(("Guest entering HDA reset with DMA(RIRB:%s, CORB:%s)\n",
                 HDA_REG(pThis, CORBCTL) & HDA_CORBCTL_DMA ? "on" : "off",
                 HDA_REG(pThis, RIRBCTL) & HDA_RIRBCTL_RDMAEN ? "on" : "off"));

        /* Clear the CRST bit to indicate that we're in reset state. */
        HDA_REG(pThis, GCTL) &= ~HDA_GCTL_CRST;

        hdaGCTLReset(pThis);

        DEVHDA_UNLOCK(pThis);
#else
        return VINF_IOM_R3_MMIO_WRITE;
#endif
    }

    if (u32Value & HDA_GCTL_FCNTRL)
    {
        DEVHDA_LOCK(pThis);

        /* Flush: GSTS:1 set, see 6.2.6. */
        HDA_REG(pThis, GSTS) |= HDA_GSTS_FSTS;  /* Set the flush status. */
        /* DPLBASE and DPUBASE should be initialized with initial value (see 6.2.6). */

        DEVHDA_UNLOCK(pThis);
    }

    return VINF_SUCCESS;
}

static int hdaRegWriteSTATESTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    uint32_t v  = HDA_REG_IND(pThis, iReg);
    uint32_t nv = u32Value & HDA_STATESTS_SCSF_MASK;

    HDA_REG(pThis, STATESTS) &= ~(v & nv); /* Write of 1 clears corresponding bit. */

    DEVHDA_UNLOCK(pThis);

    return VINF_SUCCESS;
}

static int hdaRegReadLPIB(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_READ);

    const uint8_t  uSD     = HDA_SD_NUM_FROM_REG(pThis, LPIB, iReg);
    uint32_t       u32LPIB = HDA_STREAM_REG(pThis, LPIB, uSD);
#ifdef LOG_ENABLED
    const uint32_t u32CBL  = HDA_STREAM_REG(pThis, CBL,  uSD);
    LogFlowFunc(("[SD%RU8] LPIB=%RU32, CBL=%RU32\n", uSD, u32LPIB, u32CBL));
#endif

    *pu32Value = u32LPIB;

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
}

#ifdef IN_RING3
/**
 * Returns the current maximum value the wall clock counter can be set to.
 * This maximum value depends on all currently handled HDA streams and their own current timing.
 *
 * @return  Current maximum value the wall clock counter can be set to.
 * @param   pThis               HDA state.
 *
 * @remark  Does not actually set the wall clock counter.
 */
uint64_t hdaWalClkGetMax(PHDASTATE pThis)
{
    const uint64_t u64WalClkCur       = ASMAtomicReadU64(&pThis->u64WalClk);
    const uint64_t u64FrontAbsWalClk  = hdaStreamPeriodGetAbsElapsedWalClk(&hdaGetStreamFromSink(pThis, &pThis->SinkFront)->State.Period);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
# error "Implement me!"
#endif
    const uint64_t u64LineInAbsWalClk = hdaStreamPeriodGetAbsElapsedWalClk(&hdaGetStreamFromSink(pThis, &pThis->SinkLineIn)->State.Period);
#ifdef VBOX_WITH_HDA_MIC_IN
    const uint64_t u64MicInAbsWalClk  = hdaStreamPeriodGetAbsElapsedWalClk(&hdaGetStreamFromSink(pThis, &pThis->SinkMicIn)->State.Period);
#endif

    uint64_t u64WalClkNew = RT_MAX(u64WalClkCur, u64FrontAbsWalClk);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
# error "Implement me!"
#endif
             u64WalClkNew = RT_MAX(u64WalClkNew, u64LineInAbsWalClk);
#ifdef VBOX_WITH_HDA_MIC_IN
             u64WalClkNew = RT_MAX(u64WalClkNew, u64MicInAbsWalClk);
#endif

    Log3Func(("%RU64 -> Front=%RU64, LineIn=%RU64 -> %RU64\n",
              u64WalClkCur, u64FrontAbsWalClk, u64LineInAbsWalClk, u64WalClkNew));

    return u64WalClkNew;
}
#endif /* IN_RING3 */

static int hdaRegReadWALCLK(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
#ifdef IN_RING3
    RT_NOREF(iReg);

    DEVHDA_LOCK(pThis);

    *pu32Value = RT_LO_U32(ASMAtomicReadU64(&pThis->u64WalClk));

    Log3Func(("%RU32 (max @ %RU64)\n",*pu32Value, hdaWalClkGetMax(pThis)));

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
#else
    RT_NOREF(pThis, iReg, pu32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif
}

static int hdaRegWriteCORBRP(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(iReg);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    if (u32Value & HDA_CORBRP_RST)
        HDA_REG(pThis, CORBRP) = HDA_CORBRP_RST;    /* Clears the pointer. */
    else
        HDA_REG(pThis, CORBRP) &= ~HDA_CORBRP_RST;  /* Only CORBRP_RST bit is writable. */

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
}

static int hdaRegWriteCORBCTL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    int rc = hdaRegWriteU8(pThis, iReg, u32Value);
    AssertRC(rc);

    DEVHDA_LOCK(pThis);

    if (   (uint8_t)HDA_REG(pThis, CORBWP) != (uint8_t)HDA_REG(pThis, CORBRP)
        && (HDA_REG(pThis, CORBCTL) & HDA_CORBCTL_DMA))
    {
        rc = hdaCORBCmdProcess(pThis);
    }

    DEVHDA_UNLOCK(pThis);

    return rc;
#else
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif
}

static int hdaRegWriteCORBSTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(iReg);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    uint32_t v = HDA_REG(pThis, CORBSTS);
    HDA_REG(pThis, CORBSTS) &= ~(v & u32Value);

    DEVHDA_UNLOCK(pThis);

    return VINF_SUCCESS;
}

static int hdaRegWriteCORBWP(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    int rc = hdaRegWriteU16(pThis, iReg, u32Value);
    AssertRCReturn(rc, rc);

    DEVHDA_LOCK(pThis);

    if ((uint8_t)HDA_REG(pThis, CORBWP) == (uint8_t)HDA_REG(pThis, CORBRP))
    {
        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    if (!(HDA_REG(pThis, CORBCTL) & HDA_CORBCTL_DMA))
    {
        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    rc = hdaCORBCmdProcess(pThis);

    DEVHDA_UNLOCK(pThis);

    return rc;
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

static int hdaRegWriteSDCBL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK(pThis);

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, HDA_SD_NUM_FROM_REG(pThis, CBL, iReg));
    if (!pStream)
    {
        LogFunc(("[SD%RU8] Warning: Changing SDCBL on non-attached stream (0x%x)\n",
                 HDA_SD_NUM_FROM_REG(pThis, CTL, iReg), u32Value));

        DEVHDA_UNLOCK(pThis);
        return hdaRegWriteU32(pThis, iReg, u32Value);
    }

    pStream->u32CBL = u32Value;

    LogFlowFunc(("[SD%RU8] CBL=%RU32\n", pStream->u8SD, u32Value));

    DEVHDA_UNLOCK(pThis);

    int rc2 = hdaRegWriteU32(pThis, iReg, u32Value);
    AssertRC(rc2);

    return VINF_SUCCESS; /* Always return success to the MMIO handler. */
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

static int hdaRegWriteSDCTL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    /*
     * Some guests write too much (that is, 32-bit with the top 8 bit being junk)
     * instead of 24-bit required for SDCTL. So just mask this here to be safe.
     */
    u32Value = (u32Value & 0x00ffffff);

    bool fRun      = RT_BOOL(u32Value & HDA_SDCTL_RUN);
    bool fInRun    = RT_BOOL(HDA_REG_IND(pThis, iReg) & HDA_SDCTL_RUN);

    bool fReset    = RT_BOOL(u32Value & HDA_SDCTL_SRST);
    bool fInReset  = RT_BOOL(HDA_REG_IND(pThis, iReg) & HDA_SDCTL_SRST);

    /* Get the stream descriptor. */
    uint8_t uSD = HDA_SD_NUM_FROM_REG(pThis, CTL, iReg);

    LogFunc(("[SD%RU8] fRun=%RTbool, fInRun=%RTbool, fReset=%RTbool, fInReset=%RTbool, %R[sdctl]\n",
             uSD, fRun, fInRun, fReset, fInReset, u32Value));

    /*
     * Extract the stream tag the guest wants to use for this specific
     * stream descriptor (SDn). This only can happen if the stream is in a non-running
     * state, so we're doing the lookup and assignment here.
     *
     * So depending on the guest OS, SD3 can use stream tag 4, for example.
     */
    uint8_t uTag = (u32Value >> HDA_SDCTL_NUM_SHIFT) & HDA_SDCTL_NUM_MASK;
    if (uTag > HDA_MAX_TAGS)
    {
        LogFunc(("[SD%RU8] Warning: Invalid stream tag %RU8 specified!\n", uSD, uTag));

        DEVHDA_UNLOCK_BOTH(pThis);
        return hdaRegWriteU24(pThis, iReg, u32Value);
    }

    PHDATAG pTag = &pThis->aTags[uTag];
    AssertPtr(pTag);

    LogFunc(("[SD%RU8] Using stream tag=%RU8\n", uSD, uTag));

    /* Assign new values. */
    pTag->uTag    = uTag;
    pTag->pStream = hdaGetStreamFromSD(pThis, uSD);

    PHDASTREAM pStream = pTag->pStream;
    AssertPtr(pStream);

    if (fInReset)
    {
        Assert(!fReset);
        Assert(!fInRun && !fRun);

        /* Exit reset state. */
        ASMAtomicXchgBool(&pStream->State.fInReset, false);

        /* Report that we're done resetting this stream by clearing SRST. */
        HDA_STREAM_REG(pThis, CTL, uSD) &= ~HDA_SDCTL_SRST;

        LogFunc(("[SD%RU8] Reset exit\n", uSD));
    }
    else if (fReset)
    {
        /* ICH6 datasheet 18.2.33 says that RUN bit should be cleared before initiation of reset. */
        Assert(!fInRun && !fRun);

        LogFunc(("[SD%RU8] Reset enter\n", uSD));

        hdaStreamLock(pStream);

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        hdaStreamAsyncIOLock(pStream);
        hdaStreamAsyncIOEnable(pStream, false /* fEnable */);
# endif
        hdaStreamReset(pThis, pStream, pStream->u8SD);

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        hdaStreamAsyncIOUnlock(pStream);
# endif
        hdaStreamUnlock(pStream);
    }
    else
    {
        /*
         * We enter here to change DMA states only.
         */
        if (fInRun != fRun)
        {
            Assert(!fReset && !fInReset);
            LogFunc(("[SD%RU8] State changed (fRun=%RTbool)\n", uSD, fRun));

            hdaStreamLock(pStream);

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
            hdaStreamAsyncIOLock(pStream);
            hdaStreamAsyncIOEnable(pStream, fRun /* fEnable */);
# endif
            /* (Re-)initialize the stream with current values. */
            int rc2 = hdaStreamInit(pStream, pStream->u8SD);
            AssertRC(rc2);

            /* Enable/disable the stream. */
            hdaStreamEnable(pStream, fRun /* fEnable */);

            if (fRun)
            {
                /* (Re-)init the stream's period. */
                hdaStreamPeriodInit(&pStream->State.Period,
                                    pStream->u8SD, pStream->u16LVI, pStream->u32CBL, &pStream->State.Cfg);

                /* Begin a new period for this stream. */
                rc2 = hdaStreamPeriodBegin(&pStream->State.Period, hdaWalClkGetCurrent(pThis)/* Use current wall clock time */);
                AssertRC(rc2);
            }
            else
            {
                /* Make sure to (re-)schedule outstanding (delayed) interrupts. */
                hdaReschedulePendingInterrupts(pThis);

                /* Reset the period. */
                hdaStreamPeriodReset(&pStream->State.Period);
            }

# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
            hdaStreamAsyncIOUnlock(pStream);
# endif
            /* Make sure to leave the lock before (eventually) starting the timer. */
            hdaStreamUnlock(pStream);

# ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
            /* See if we need to start or stop the timer. */
            if (!fRun)
                hdaTimerMaybeStop(pThis);
            else
                hdaTimerMaybeStart(pThis);
# endif
        }
    }

    DEVHDA_UNLOCK_BOTH(pThis);

    int rc2 = hdaRegWriteU24(pThis, iReg, u32Value);
    AssertRC(rc2);

    return VINF_SUCCESS; /* Always return success to the MMIO handler. */
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

static int hdaRegWriteSDSTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK(pThis);

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, HDA_SD_NUM_FROM_REG(pThis, STS, iReg));
    if (!pStream)
    {
        AssertMsgFailed(("[SD%RU8] Warning: Writing SDSTS on non-attached stream (0x%x)\n",
                         HDA_SD_NUM_FROM_REG(pThis, STS, iReg), u32Value));

        DEVHDA_UNLOCK(pThis);
        return hdaRegWriteU16(pThis, iReg, u32Value);
    }

    uint32_t v = HDA_REG_IND(pThis, iReg);

    /* Clear (zero) FIFOE, DESE and BCIS bits when writing 1 to it (6.2.33). */
    HDA_REG_IND(pThis, iReg) &= ~(u32Value & v);

    /* Some guests tend to write SDnSTS even if the stream is not running.
     * So make sure to check if the RUN bit is set first. */
    const bool fInRun = RT_BOOL(HDA_STREAM_REG(pThis, CTL, pStream->u8SD) & HDA_SDCTL_RUN);

    Log3Func(("[SD%RU8] fRun=%RTbool %R[sdsts]\n", pStream->u8SD, fInRun, v));

    PHDASTREAMPERIOD pPeriod = &pStream->State.Period;

    if (hdaStreamPeriodLock(pPeriod))
    {
        const bool fNeedsInterrupt = hdaStreamPeriodNeedsInterrupt(pPeriod);
        if (fNeedsInterrupt)
            hdaStreamPeriodReleaseInterrupt(pPeriod);

        if (hdaStreamPeriodIsComplete(pPeriod))
        {
            hdaStreamPeriodEnd(pPeriod);

            if (fInRun)
                hdaStreamPeriodBegin(pPeriod, hdaWalClkGetCurrent(pThis) /* Use current wall clock time */);
        }

        hdaStreamPeriodUnlock(pPeriod); /* Unlock before processing interrupt. */

        if (fNeedsInterrupt)
        {
#ifndef DEBUG
            hdaProcessInterrupt(pThis);
#else
            hdaProcessInterrupt(pThis, __FUNCTION__);
#endif
        }
    }

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
#else /* IN_RING3 */
    RT_NOREF(pThis, iReg, u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* !IN_RING3 */
}

static int hdaRegWriteSDLVI(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK(pThis);

    if (HDA_REG_IND(pThis, iReg) == u32Value) /* Value already set? */
    {
        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    uint8_t uSD = HDA_SD_NUM_FROM_REG(pThis, LVI, iReg);

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, uSD);
    if (!pStream)
    {
        AssertMsgFailed(("[SD%RU8] Warning: Changing SDLVI on non-attached stream (0x%x)\n", uSD, u32Value));

        DEVHDA_UNLOCK(pThis);
        return hdaRegWriteU16(pThis, iReg, u32Value);
    }

    /** @todo Validate LVI. */
    pStream->u16LVI = u32Value;
    LogFunc(("[SD%RU8] Updating LVI to %RU16\n", uSD, pStream->u16LVI));

# ifdef HDA_USE_DMA_ACCESS_HANDLER
    if (hdaGetDirFromSD(uSD) == PDMAUDIODIR_OUT)
    {
        /* Try registering the DMA handlers.
         * As we can't be sure in which order LVI + BDL base are set, try registering in both routines. */
        if (hdaStreamRegisterDMAHandlers(pThis, pStream))
            LogFunc(("[SD%RU8] DMA logging enabled\n", pStream->u8SD));
    }
# endif

    DEVHDA_UNLOCK(pThis);

    int rc2 = hdaRegWriteU16(pThis, iReg, u32Value);
    AssertRC(rc2);

    return VINF_SUCCESS; /* Always return success to the MMIO handler. */
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

static int hdaRegWriteSDFIFOW(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK(pThis);

    uint8_t uSD = HDA_SD_NUM_FROM_REG(pThis, FIFOW, iReg);

    if (hdaGetDirFromSD(uSD) != PDMAUDIODIR_IN) /* FIFOW for input streams only. */
    {
        LogRel(("HDA: Warning: Guest tried to write read-only FIFOW to output stream #%RU8, ignoring\n", uSD));

        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, HDA_SD_NUM_FROM_REG(pThis, FIFOW, iReg));
    if (!pStream)
    {
        AssertMsgFailed(("[SD%RU8] Warning: Changing FIFOW on non-attached stream (0x%x)\n", uSD, u32Value));

        DEVHDA_UNLOCK(pThis);
        return hdaRegWriteU16(pThis, iReg, u32Value);
    }

    uint32_t u32FIFOW = 0;

    switch (u32Value)
    {
        case HDA_SDFIFOW_8B:
        case HDA_SDFIFOW_16B:
        case HDA_SDFIFOW_32B:
            u32FIFOW = u32Value;
            break;
        default:
            LogRel(("HDA: Warning: Guest tried write unsupported FIFOW (0x%x) to stream #%RU8, defaulting to 32 bytes\n",
                    u32Value, uSD));
            AssertFailed();
            u32FIFOW = HDA_SDFIFOW_32B;
            break;
    }

    if (u32FIFOW)
    {
        pStream->u16FIFOW = hdaSDFIFOWToBytes(u32FIFOW);
        LogFunc(("[SD%RU8] Updating FIFOW to %RU32 bytes\n", uSD, pStream->u16FIFOW));

        DEVHDA_UNLOCK(pThis);

        int rc2 = hdaRegWriteU16(pThis, iReg, u32FIFOW);
        AssertRC(rc2);
    }

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS; /* Always return success to the MMIO handler. */
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

/**
 * @note This method could be called for changing value on Output Streams only (ICH6 datasheet 18.2.39).
 */
static int hdaRegWriteSDFIFOS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK(pThis);

    uint8_t uSD = HDA_SD_NUM_FROM_REG(pThis, FIFOS, iReg);

    if (hdaGetDirFromSD(uSD) != PDMAUDIODIR_OUT) /* FIFOS for output streams only. */
    {
        LogRel(("HDA: Warning: Guest tried to write read-only FIFOS to input stream #%RU8, ignoring\n", uSD));

        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, uSD);
    if (!pStream)
    {
        AssertMsgFailed(("[SD%RU8] Warning: Changing FIFOS on non-attached stream (0x%x)\n", uSD, u32Value));

        DEVHDA_UNLOCK(pThis);
        return hdaRegWriteU16(pThis, iReg, u32Value);
    }

    uint32_t u32FIFOS = 0;

    switch(u32Value)
    {
        case HDA_SDOFIFO_16B:
        case HDA_SDOFIFO_32B:
        case HDA_SDOFIFO_64B:
        case HDA_SDOFIFO_128B:
        case HDA_SDOFIFO_192B:
        case HDA_SDOFIFO_256B:
            u32FIFOS = u32Value;
            break;

        default:
            LogRel(("HDA: Warning: Guest tried write unsupported FIFOS (0x%x) to stream #%RU8, defaulting to 192 bytes\n",
                    u32Value, uSD));
            AssertFailed();
            u32FIFOS = HDA_SDOFIFO_192B;
            break;
    }

    if (u32FIFOS)
    {
        pStream->u16FIFOS = u32FIFOS + 1;
        LogFunc(("[SD%RU8] Updating FIFOS to %RU32 bytes\n", uSD, pStream->u16FIFOS));

        DEVHDA_UNLOCK(pThis);

        int rc2 = hdaRegWriteU16(pThis, iReg, u32FIFOS);
        AssertRC(rc2);
    }
    else
        DEVHDA_UNLOCK(pThis);

    return VINF_SUCCESS; /* Always return success to the MMIO handler. */
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

#ifdef IN_RING3
/**
 * Adds an audio output stream to the device setup using the given configuration.
 *
 * @returns IPRT status code.
 * @param   pThis               Device state.
 * @param   pCfg                Stream configuration to use for adding a stream.
 */
static int hdaAddStreamOut(PHDASTATE pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);

    AssertReturn(pCfg->enmDir == PDMAUDIODIR_OUT, VERR_INVALID_PARAMETER);

    LogFlowFunc(("Stream=%s\n", pCfg->szName));

    int rc = VINF_SUCCESS;

    bool fUseFront = true; /* Always use front out by default. */
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    bool fUseRear;
    bool fUseCenter;
    bool fUseLFE;

    fUseRear = fUseCenter = fUseLFE = false;

    /*
     * Use commonly used setups for speaker configurations.
     */

    /** @todo Make the following configurable through mixer API and/or CFGM? */
    switch (pCfg->Props.cChannels)
    {
        case 3:  /* 2.1: Front (Stereo) + LFE. */
        {
            fUseLFE   = true;
            break;
        }

        case 4:  /* Quadrophonic: Front (Stereo) + Rear (Stereo). */
        {
            fUseRear  = true;
            break;
        }

        case 5:  /* 4.1: Front (Stereo) + Rear (Stereo) + LFE. */
        {
            fUseRear  = true;
            fUseLFE   = true;
            break;
        }

        case 6:  /* 5.1: Front (Stereo) + Rear (Stereo) + Center/LFE. */
        {
            fUseRear   = true;
            fUseCenter = true;
            fUseLFE    = true;
            break;
        }

        default: /* Unknown; fall back to 2 front channels (stereo). */
        {
            rc = VERR_NOT_SUPPORTED;
            break;
        }
    }
#else /* !VBOX_WITH_AUDIO_HDA_51_SURROUND */
    /* Only support mono or stereo channels. */
    if (   pCfg->Props.cChannels != 1 /* Mono */
        && pCfg->Props.cChannels != 2 /* Stereo */)
    {
        rc = VERR_NOT_SUPPORTED;
    }
#endif

    if (rc == VERR_NOT_SUPPORTED)
    {
        LogRel2(("HDA: Unsupported channel count (%RU8), falling back to stereo channels\n", pCfg->Props.cChannels));

        /* Fall back to 2 channels (see below in fUseFront block). */
        rc = VINF_SUCCESS;
    }

    do
    {
        if (RT_FAILURE(rc))
            break;

        if (fUseFront)
        {
            RTStrPrintf(pCfg->szName, RT_ELEMENTS(pCfg->szName), "Front");

            pCfg->DestSource.Dest = PDMAUDIOPLAYBACKDEST_FRONT;
            pCfg->enmLayout       = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            pCfg->Props.cChannels = 2;
            pCfg->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfg->Props.cBits, pCfg->Props.cChannels);

            rc = hdaCodecRemoveStream(pThis->pCodec,  PDMAUDIOMIXERCTL_FRONT);
            if (RT_SUCCESS(rc))
                rc = hdaCodecAddStream(pThis->pCodec, PDMAUDIOMIXERCTL_FRONT, pCfg);
        }

#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
        if (   RT_SUCCESS(rc)
            && (fUseCenter || fUseLFE))
        {
            RTStrPrintf(pCfg->szName, RT_ELEMENTS(pCfg->szName), "Center/LFE");

            pCfg->DestSource.Dest = PDMAUDIOPLAYBACKDEST_CENTER_LFE;
            pCfg->enmLayout       = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            pCfg->Props.cChannels = (fUseCenter && fUseLFE) ? 2 : 1;
            pCfg->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfg->Props.cBits, pCfg->Props.cChannels);

            rc = hdaCodecRemoveStream(pThis->pCodec,  PDMAUDIOMIXERCTL_CENTER_LFE);
            if (RT_SUCCESS(rc))
                rc = hdaCodecAddStream(pThis->pCodec, PDMAUDIOMIXERCTL_CENTER_LFE, pCfg);
        }

        if (   RT_SUCCESS(rc)
            && fUseRear)
        {
            RTStrPrintf(pCfg->szName, RT_ELEMENTS(pCfg->szName), "Rear");

            pCfg->DestSource.Dest = PDMAUDIOPLAYBACKDEST_REAR;
            pCfg->enmLayout       = PDMAUDIOSTREAMLAYOUT_NON_INTERLEAVED;

            pCfg->Props.cChannels = 2;
            pCfg->Props.cShift    = PDMAUDIOPCMPROPS_MAKE_SHIFT_PARMS(pCfg->Props.cBits, pCfg->Props.cChannels);

            rc = hdaCodecRemoveStream(pThis->pCodec,  PDMAUDIOMIXERCTL_REAR);
            if (RT_SUCCESS(rc))
                rc = hdaCodecAddStream(pThis->pCodec, PDMAUDIOMIXERCTL_REAR, pCfg);
        }
#endif /* VBOX_WITH_AUDIO_HDA_51_SURROUND */

    } while (0);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Adds an audio input stream to the device setup using the given configuration.
 *
 * @returns IPRT status code.
 * @param   pThis               Device state.
 * @param   pCfg                Stream configuration to use for adding a stream.
 */
static int hdaAddStreamIn(PHDASTATE pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);

    AssertReturn(pCfg->enmDir == PDMAUDIODIR_IN, VERR_INVALID_PARAMETER);

    LogFlowFunc(("Stream=%s, Source=%ld\n", pCfg->szName, pCfg->DestSource.Source));

    int rc;

    switch (pCfg->DestSource.Source)
    {
        case PDMAUDIORECSOURCE_LINE:
        {
            rc = hdaCodecRemoveStream(pThis->pCodec,  PDMAUDIOMIXERCTL_LINE_IN);
            if (RT_SUCCESS(rc))
                rc = hdaCodecAddStream(pThis->pCodec, PDMAUDIOMIXERCTL_LINE_IN, pCfg);
            break;
        }
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
        case PDMAUDIORECSOURCE_MIC:
        {
            rc = hdaCodecRemoveStream(pThis->pCodec,  PDMAUDIOMIXERCTL_MIC_IN);
            if (RT_SUCCESS(rc))
                rc = hdaCodecAddStream(pThis->pCodec, PDMAUDIOMIXERCTL_MIC_IN, pCfg);
            break;
        }
#endif
        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Adds an audio stream to the device setup using the given configuration.
 *
 * @returns IPRT status code.
 * @param   pThis               Device state.
 * @param   pCfg                Stream configuration to use for adding a stream.
 */
static int hdaAddStream(PHDASTATE pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    PHDADRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, HDADRIVER, Node)
    {
        int rc2;

        switch (pCfg->enmDir)
        {
            case PDMAUDIODIR_OUT:
                rc2 = hdaAddStreamOut(pThis, pCfg);
                break;

            case PDMAUDIODIR_IN:
                rc2 = hdaAddStreamIn(pThis, pCfg);
                break;

            default:
                rc2 = VERR_NOT_SUPPORTED;
                AssertFailed();
                break;
        }

        if (   RT_FAILURE(rc2)
            && (pDrv->fFlags & PDMAUDIODRVFLAGS_PRIMARY)) /* We only care about primary drivers here, the rest may fail. */
        {
            if (RT_SUCCESS(rc))
                rc = rc2;
            /* Keep going. */
        }
    }

    return rc;
}
#endif /* IN_RING3 */

static int hdaRegWriteSDFMT(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
#ifdef IN_RING3
    DEVHDA_LOCK(pThis);

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, HDA_SD_NUM_FROM_REG(pThis, FMT, iReg));
    if (!pStream)
    {
        LogFunc(("[SD%RU8] Warning: Changing SDFMT on non-attached stream (0x%x)\n",
                 HDA_SD_NUM_FROM_REG(pThis, FMT, iReg), u32Value));
        return hdaRegWriteU16(pThis, iReg, u32Value);
    }

    /* Write the wanted stream format into the register in any case.
     *
     * This is important for e.g. MacOS guests, as those try to initialize streams which are not reported
     * by the device emulation (wants 4 channels, only have 2 channels at the moment).
     *
     * When ignoring those (invalid) formats, this leads to MacOS thinking that the device is malfunctioning
     * and therefore disabling the device completely. */
    int rc = hdaRegWriteU16(pThis, iReg, u32Value);
    AssertRC(rc);

    rc = hdaStreamInit(pStream, pStream->u8SD);
    if (RT_SUCCESS(rc))
    {
        /* Add the stream to the device setup. */
        rc = hdaAddStream(pThis, &pStream->State.Cfg);
# ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
        if (RT_SUCCESS(rc))
            rc = hdaStreamAsyncIOCreate(pStream);
# endif
    }

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS; /* Never return failure. */
#else /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value);
    return VINF_IOM_R3_MMIO_WRITE;
#endif
}

/* Note: Will be called for both, BDPL and BDPU, registers. */
DECLINLINE(int) hdaRegWriteSDBDPX(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value, uint8_t uSD)
{
#ifdef IN_RING3
    int rc2 = hdaRegWriteU32(pThis, iReg, u32Value);
    AssertRC(rc2);

    DEVHDA_LOCK(pThis);

    PHDASTREAM pStream = hdaGetStreamFromSD(pThis, uSD);
    if (!pStream)
    {
        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    /* Update BDL base. */
    pStream->u64BDLBase = RT_MAKE_U64(HDA_STREAM_REG(pThis, BDPL, uSD),
                                      HDA_STREAM_REG(pThis, BDPU, uSD));

# ifdef HDA_USE_DMA_ACCESS_HANDLER
    if (hdaGetDirFromSD(uSD) == PDMAUDIODIR_OUT)
    {
        /* Try registering the DMA handlers.
         * As we can't be sure in which order LVI + BDL base are set, try registering in both routines. */
        if (hdaStreamRegisterDMAHandlers(pThis, pStream))
            LogFunc(("[SD%RU8] DMA logging enabled\n", pStream->u8SD));
    }
# endif

    LogFlowFunc(("[SD%RU8] BDLBase=0x%x\n", pStream->u8SD, pStream->u64BDLBase));

    DEVHDA_UNLOCK(pThis);

    return VINF_SUCCESS; /* Always return success to the MMIO handler. */
#else  /* !IN_RING3 */
    RT_NOREF_PV(pThis); RT_NOREF_PV(iReg); RT_NOREF_PV(u32Value); RT_NOREF_PV(uSD);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* IN_RING3 */
}

static int hdaRegWriteSDBDPL(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    return hdaRegWriteSDBDPX(pThis, iReg, u32Value, HDA_SD_NUM_FROM_REG(pThis, BDPL, iReg));
}

static int hdaRegWriteSDBDPU(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    return hdaRegWriteSDBDPX(pThis, iReg, u32Value, HDA_SD_NUM_FROM_REG(pThis, BDPU, iReg));
}

static int hdaRegReadIRS(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value)
{
    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_READ);

    /* regarding 3.4.3 we should mark IRS as busy in case CORB is active */
    if (   HDA_REG(pThis, CORBWP) != HDA_REG(pThis, CORBRP)
        || (HDA_REG(pThis, CORBCTL) & HDA_CORBCTL_DMA))
    {
        HDA_REG(pThis, IRS) = HDA_IRS_ICB;  /* busy */
    }

    DEVHDA_UNLOCK(pThis);

    return hdaRegReadU32(pThis, iReg, pu32Value);
}

static int hdaRegWriteIRS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(iReg);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    /*
     * If the guest set the ICB bit of IRS register, HDA should process the verb in IC register,
     * write the response to IR register, and set the IRV (valid in case of success) bit of IRS register.
     */
    if (   (u32Value & HDA_IRS_ICB)
        && !(HDA_REG(pThis, IRS) & HDA_IRS_ICB))
    {
#ifdef IN_RING3
        uint32_t uCmd = HDA_REG(pThis, IC);

        if (HDA_REG(pThis, CORBWP) != HDA_REG(pThis, CORBRP))
        {
            DEVHDA_UNLOCK(pThis);

            /*
             * 3.4.3: Defines behavior of immediate Command status register.
             */
            LogRel(("HDA: Guest attempted process immediate verb (%x) with active CORB\n", uCmd));
            return VINF_SUCCESS;
        }

        HDA_REG(pThis, IRS) = HDA_IRS_ICB;  /* busy */

        uint64_t uResp;
        int rc2 = pThis->pCodec->pfnLookup(pThis->pCodec,
                                           HDA_CODEC_CMD(uCmd, 0 /* LUN */), &uResp);
        if (RT_FAILURE(rc2))
            LogFunc(("Codec lookup failed with rc2=%Rrc\n", rc2));

        HDA_REG(pThis, IR)   = (uint32_t)uResp; /** @todo r=andy Do we need a 64-bit response? */
        HDA_REG(pThis, IRS)  = HDA_IRS_IRV;     /* result is ready  */
        /** @todo r=michaln We just set the IRS value, why are we clearing unset bits? */
        HDA_REG(pThis, IRS) &= ~HDA_IRS_ICB;    /* busy is clear */

        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
#else  /* !IN_RING3 */
        DEVHDA_UNLOCK(pThis);
        return VINF_IOM_R3_MMIO_WRITE;
#endif /* !IN_RING3 */
    }

    /*
     * Once the guest read the response, it should clear the IRV bit of the IRS register.
     */
    HDA_REG(pThis, IRS) &= ~(u32Value & HDA_IRS_IRV);

    DEVHDA_UNLOCK(pThis);
    return VINF_SUCCESS;
}

static int hdaRegWriteRIRBWP(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(iReg);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    if (u32Value & HDA_RIRBWP_RST)
        HDA_REG(pThis, RIRBWP) = 0;

    DEVHDA_UNLOCK(pThis);

    /* The remaining bits are O, see 6.2.22. */
    return VINF_SUCCESS;
}

static int hdaRegWriteBase(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    uint32_t iRegMem = g_aHdaRegMap[iReg].mem_idx;
    int rc = hdaRegWriteU32(pThis, iReg, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    switch(iReg)
    {
        case HDA_REG_CORBLBASE:
            pThis->u64CORBBase &= UINT64_C(0xFFFFFFFF00000000);
            pThis->u64CORBBase |= pThis->au32Regs[iRegMem];
            break;
        case HDA_REG_CORBUBASE:
            pThis->u64CORBBase &= UINT64_C(0x00000000FFFFFFFF);
            pThis->u64CORBBase |= ((uint64_t)pThis->au32Regs[iRegMem] << 32);
            break;
        case HDA_REG_RIRBLBASE:
            pThis->u64RIRBBase &= UINT64_C(0xFFFFFFFF00000000);
            pThis->u64RIRBBase |= pThis->au32Regs[iRegMem];
            break;
        case HDA_REG_RIRBUBASE:
            pThis->u64RIRBBase &= UINT64_C(0x00000000FFFFFFFF);
            pThis->u64RIRBBase |= ((uint64_t)pThis->au32Regs[iRegMem] << 32);
            break;
        case HDA_REG_DPLBASE:
        {
            pThis->u64DPBase = pThis->au32Regs[iRegMem] & DPBASE_ADDR_MASK;
            Assert(pThis->u64DPBase % 128 == 0); /* Must be 128-byte aligned. */

            /* Also make sure to handle the DMA position enable bit. */
            pThis->fDMAPosition = pThis->au32Regs[iRegMem] & RT_BIT_32(0);
            LogRel(("HDA: %s DMA position buffer\n", pThis->fDMAPosition ? "Enabled" : "Disabled"));
            break;
        }
        case HDA_REG_DPUBASE:
            pThis->u64DPBase = RT_MAKE_U64(RT_LO_U32(pThis->u64DPBase) & DPBASE_ADDR_MASK, pThis->au32Regs[iRegMem]);
            break;
        default:
            AssertMsgFailed(("Invalid index\n"));
            break;
    }

    LogFunc(("CORB base:%llx RIRB base: %llx DP base: %llx\n",
             pThis->u64CORBBase, pThis->u64RIRBBase, pThis->u64DPBase));

    DEVHDA_UNLOCK(pThis);
    return rc;
}

static int hdaRegWriteRIRBSTS(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF_PV(iReg);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    uint8_t v = HDA_REG(pThis, RIRBSTS);
    HDA_REG(pThis, RIRBSTS) &= ~(v & u32Value);

    DEVHDA_UNLOCK(pThis);

#ifndef DEBUG
    return hdaProcessInterrupt(pThis);
#else
    return hdaProcessInterrupt(pThis, __FUNCTION__);
#endif
}

#ifdef IN_RING3

#ifdef LOG_ENABLED
static void hdaBDLEDumpAll(PHDASTATE pThis, uint64_t u64BDLBase, uint16_t cBDLE)
{
    LogFlowFunc(("BDLEs @ 0x%x (%RU16):\n", u64BDLBase, cBDLE));
    if (!u64BDLBase)
        return;

    uint32_t cbBDLE = 0;
    for (uint16_t i = 0; i < cBDLE; i++)
    {
        HDABDLEDESC bd;
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), u64BDLBase + i * sizeof(HDABDLEDESC), &bd, sizeof(bd));

        LogFunc(("\t#%03d BDLE(adr:0x%llx, size:%RU32, ioc:%RTbool)\n",
                 i, bd.u64BufAdr, bd.u32BufSize, bd.fFlags & HDA_BDLE_FLAG_IOC));

        cbBDLE += bd.u32BufSize;
    }

    LogFlowFunc(("Total: %RU32 bytes\n", cbBDLE));

    if (!pThis->u64DPBase) /* No DMA base given? Bail out. */
        return;

    LogFlowFunc(("DMA counters:\n"));

    for (int i = 0; i < cBDLE; i++)
    {
        uint32_t uDMACnt;
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), (pThis->u64DPBase & DPBASE_ADDR_MASK) + (i * 2 * sizeof(uint32_t)),
                          &uDMACnt, sizeof(uDMACnt));

        LogFlowFunc(("\t#%03d DMA @ 0x%x\n", i , uDMACnt));
    }
}
#endif /* LOG_ENABLED */

/**
 * Retrieves a corresponding sink for a given mixer control.
 * Returns NULL if no sink is found.
 *
 * @return  PHDAMIXERSINK
 * @param   pThis               HDA state.
 * @param   enmMixerCtl         Mixer control to get the corresponding sink for.
 */
static PHDAMIXERSINK hdaMixerControlToSink(PHDASTATE pThis, PDMAUDIOMIXERCTL enmMixerCtl)
{
    PHDAMIXERSINK pSink;

    switch (enmMixerCtl)
    {
        case PDMAUDIOMIXERCTL_VOLUME_MASTER:
            /* Fall through is intentional. */
        case PDMAUDIOMIXERCTL_FRONT:
            pSink = &pThis->SinkFront;
            break;
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
        case PDMAUDIOMIXERCTL_CENTER_LFE:
            pSink = &pThis->SinkCenterLFE;
            break;
        case PDMAUDIOMIXERCTL_REAR:
            pSink = &pThis->SinkRear;
            break;
#endif
        case PDMAUDIOMIXERCTL_LINE_IN:
            pSink = &pThis->SinkLineIn;
            break;
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
        case PDMAUDIOMIXERCTL_MIC_IN:
            pSink = &pThis->SinkMicIn;
            break;
#endif
        default:
            pSink = NULL;
            AssertMsgFailed(("Unhandled mixer control\n"));
            break;
    }

    return pSink;
}

/**
 * Adds a driver stream to a specific mixer sink.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA state.
 * @param   pMixSink            Audio mixer sink to add audio streams to.
 * @param   pCfg                Audio stream configuration to use for the audio streams to add.
 * @param   pDrv                Driver stream to add.
 */
static int hdaMixerAddDrvStream(PHDASTATE pThis, PAUDMIXSINK pMixSink, PPDMAUDIOSTREAMCFG pCfg, PHDADRIVER pDrv)
{
    AssertPtrReturn(pThis,    VERR_INVALID_POINTER);
    AssertPtrReturn(pMixSink, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,     VERR_INVALID_POINTER);

    LogFunc(("Sink=%s, Stream=%s\n", pMixSink->pszName, pCfg->szName));

    PPDMAUDIOSTREAMCFG pStreamCfg = DrvAudioHlpStreamCfgDup(pCfg);
    if (!pStreamCfg)
        return VERR_NO_MEMORY;

    if (!RTStrPrintf(pStreamCfg->szName, sizeof(pStreamCfg->szName), "%s", pCfg->szName))
    {
        RTMemFree(pStreamCfg);
        return VERR_BUFFER_OVERFLOW;
    }

    LogFunc(("[LUN#%RU8] %s\n", pDrv->uLUN, pStreamCfg->szName));

    int rc = VINF_SUCCESS;

    PHDADRIVERSTREAM pDrvStream = NULL;

    if (pStreamCfg->enmDir == PDMAUDIODIR_IN)
    {
        LogFunc(("enmRecSource=%d\n", pStreamCfg->DestSource.Source));

        switch (pStreamCfg->DestSource.Source)
        {
            case PDMAUDIORECSOURCE_LINE:
                pDrvStream = &pDrv->LineIn;
                break;
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
            case PDMAUDIORECSOURCE_MIC:
                pDrvStream = &pDrv->MicIn;
                break;
#endif
            default:
                rc = VERR_NOT_SUPPORTED;
                break;
        }
    }
    else if (pStreamCfg->enmDir == PDMAUDIODIR_OUT)
    {
        LogFunc(("enmPlaybackDest=%d\n", pStreamCfg->DestSource.Dest));

        switch (pStreamCfg->DestSource.Dest)
        {
            case PDMAUDIOPLAYBACKDEST_FRONT:
                pDrvStream = &pDrv->Front;
                break;
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
            case PDMAUDIOPLAYBACKDEST_CENTER_LFE:
                pDrvStream = &pDrv->CenterLFE;
                break;
            case PDMAUDIOPLAYBACKDEST_REAR:
                pDrvStream = &pDrv->Rear;
                break;
#endif
            default:
                rc = VERR_NOT_SUPPORTED;
                break;
        }
    }
    else
        rc = VERR_NOT_SUPPORTED;

    if (RT_SUCCESS(rc))
    {
        AssertPtr(pDrvStream);
        AssertMsg(pDrvStream->pMixStrm == NULL, ("[LUN#%RU8] Driver stream already present when it must not\n", pDrv->uLUN));

        PAUDMIXSTREAM pMixStrm;
        rc = AudioMixerSinkCreateStream(pMixSink, pDrv->pConnector, pStreamCfg, 0 /* fFlags */, &pMixStrm);
        if (RT_SUCCESS(rc))
        {
            rc = AudioMixerSinkAddStream(pMixSink, pMixStrm);
            LogFlowFunc(("LUN#%RU8: Added \"%s\" to sink, rc=%Rrc\n", pDrv->uLUN, pStreamCfg->szName, rc));
        }

        if (RT_SUCCESS(rc))
            pDrvStream->pMixStrm = pMixStrm;
    }

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
 * @param   pThis               HDA state.
 * @param   pMixSink            Audio mixer sink to add stream to.
 * @param   pCfg                Audio stream configuration to use for the audio streams to add.
 */
static int hdaMixerAddDrvStreams(PHDASTATE pThis, PAUDMIXSINK pMixSink, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis,    VERR_INVALID_POINTER);
    AssertPtrReturn(pMixSink, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,     VERR_INVALID_POINTER);

    LogFunc(("Sink=%s, Stream=%s\n", pMixSink->pszName, pCfg->szName));

    if (!DrvAudioHlpStreamCfgIsValid(pCfg))
        return VERR_INVALID_PARAMETER;

    int rc = AudioMixerSinkSetFormat(pMixSink, &pCfg->Props);
    if (RT_FAILURE(rc))
        return rc;

    PHDADRIVER pDrv;
    RTListForEach(&pThis->lstDrv, pDrv, HDADRIVER, Node)
    {
        int rc2 = hdaMixerAddDrvStream(pThis, pMixSink, pCfg, pDrv);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Adds a new audio stream to a specific mixer control.
 * Depending on the mixer control the stream then gets assigned to one of the internal
 * mixer sinks, which in turn then handle the mixing of all connected streams to that sink.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 * @param   enmMixerCtl         Mixer control to assign new stream to.
 * @param   pCfg                Stream configuration for the new stream.
 */
static DECLCALLBACK(int) hdaMixerAddStream(PHDASTATE pThis, PDMAUDIOMIXERCTL enmMixerCtl, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);

    int rc;

    PHDAMIXERSINK pSink = hdaMixerControlToSink(pThis, enmMixerCtl);
    if (pSink)
    {
        rc = hdaMixerAddDrvStreams(pThis, pSink->pMixSink, pCfg);

        AssertPtr(pSink->pMixSink);
        LogFlowFunc(("Sink=%s, enmMixerCtl=%d\n", pSink->pMixSink->pszName, enmMixerCtl));
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Removes a specified mixer control from the HDA's mixer.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 * @param   enmMixerCtl         Mixer control to remove.
 *
 * @remarks Can be called as a callback by the HDA codec.
 */
static DECLCALLBACK(int) hdaMixerRemoveStream(PHDASTATE pThis, PDMAUDIOMIXERCTL enmMixerCtl)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    int rc;

    PHDAMIXERSINK pSink = hdaMixerControlToSink(pThis, enmMixerCtl);
    if (pSink)
    {
        PHDADRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, HDADRIVER, Node)
        {
            PAUDMIXSTREAM pMixStream = NULL;
            switch (enmMixerCtl)
            {
                /*
                 * Input.
                 */
                case PDMAUDIOMIXERCTL_LINE_IN:
                    pMixStream = pDrv->LineIn.pMixStrm;
                    pDrv->LineIn.pMixStrm = NULL;
                    break;
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
                case PDMAUDIOMIXERCTL_MIC_IN:
                    pMixStream = pDrv->MicIn.pMixStrm;
                    pDrv->MicIn.pMixStrm = NULL;
                    break;
#endif
                /*
                 * Output.
                 */
                case PDMAUDIOMIXERCTL_FRONT:
                    pMixStream = pDrv->Front.pMixStrm;
                    pDrv->Front.pMixStrm = NULL;
                    break;
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
                case PDMAUDIOMIXERCTL_CENTER_LFE:
                    pMixStream = pDrv->CenterLFE.pMixStrm;
                    pDrv->CenterLFE.pMixStrm = NULL;
                    break;
                case PDMAUDIOMIXERCTL_REAR:
                    pMixStream = pDrv->Rear.pMixStrm;
                    pDrv->Rear.pMixStrm = NULL;
                    break;
#endif
                default:
                    AssertMsgFailed(("Mixer control %d not implemented\n", enmMixerCtl));
                    break;
            }

            if (pMixStream)
            {
                AudioMixerSinkRemoveStream(pSink->pMixSink, pMixStream);
                AudioMixerStreamDestroy(pMixStream);

                pMixStream = NULL;
            }
        }

        AudioMixerSinkRemoveAllStreams(pSink->pMixSink);
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFunc(("enmMixerCtl=%d, rc=%Rrc\n", enmMixerCtl, rc));
    return rc;
}

/**
 * Sets a SDn stream number and channel to a particular mixer control.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA State.
 * @param   enmMixerCtl         Mixer control to set SD stream number and channel for.
 * @param   uSD                 SD stream number (number + 1) to set. Set to 0 for unassign.
 * @param   uChannel            Channel to set. Only valid if a valid SD stream number is specified.
 *
 * @remarks Can be called as a callback by the HDA codec.
 */
static DECLCALLBACK(int) hdaMixerSetStream(PHDASTATE pThis, PDMAUDIOMIXERCTL enmMixerCtl, uint8_t uSD, uint8_t uChannel)
{
    LogFlowFunc(("enmMixerCtl=%RU32, uSD=%RU8, uChannel=%RU8\n", enmMixerCtl, uSD, uChannel));

    if (uSD == 0) /* Stream number 0 is reserved. */
    {
        LogFlowFunc(("Invalid SDn (%RU8) number for mixer control %d, ignoring\n", uSD, enmMixerCtl));
        return VINF_SUCCESS;
    }
    /* uChannel is optional. */

    /* SDn0 starts as 1. */
    Assert(uSD);
    uSD--;

    int rc;

    PHDAMIXERSINK pSink = hdaMixerControlToSink(pThis, enmMixerCtl);
    if (pSink)
    {
        if (   (uSD < HDA_MAX_SDI)
            && AudioMixerSinkGetDir(pSink->pMixSink) == AUDMIXSINKDIR_OUTPUT)
        {
            uSD += HDA_MAX_SDI;
        }

        LogFlowFunc(("%s: Setting to stream ID=%RU8, channel=%RU8, enmMixerCtl=%RU32\n",
                     pSink->pMixSink->pszName, uSD, uChannel, enmMixerCtl));

        Assert(uSD < HDA_MAX_STREAMS);

        PHDASTREAM pStream = hdaGetStreamFromSD(pThis, uSD);
        if (pStream)
        {
            hdaStreamLock(pStream);

            pSink->uSD        = uSD;
            pSink->uChannel   = uChannel;
            pStream->pMixSink = pSink;

            hdaStreamUnlock(pStream);

            rc = VINF_SUCCESS;
        }
        else
        {
            LogRel(("HDA: Guest wanted to assign invalid stream ID=%RU8 (channel %RU8) to mixer control %RU32, skipping\n",
                    uSD, uChannel, enmMixerCtl));
            rc = VERR_INVALID_PARAMETER;
        }
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Sets the volume of a specified mixer control.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA State.
 * @param   enmMixerCtl         Mixer control to set volume for.
 * @param   pVol                Pointer to volume data to set.
 *
 * @remarks Can be called as a callback by the HDA codec.
 */
static DECLCALLBACK(int) hdaMixerSetVolume(PHDASTATE pThis,
                                           PDMAUDIOMIXERCTL enmMixerCtl, PPDMAUDIOVOLUME pVol)
{
    int rc;

    PHDAMIXERSINK pSink = hdaMixerControlToSink(pThis, enmMixerCtl);
    if (   pSink
        && pSink->pMixSink)
    {
        LogRel2(("HDA: Setting volume for mixer sink '%s' to %RU8/%RU8 (%s)\n",
                 pSink->pMixSink->pszName, pVol->uLeft, pVol->uRight, pVol->fMuted ? "Muted" : "Unmuted"));

        /* Set the volume.
         * We assume that the codec already converted it to the correct range. */
        rc = AudioMixerSinkSetVolume(pSink->pMixSink, pVol);
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

#ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
/**
 * Starts the internal audio device timer.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 */
static int hdaTimerStart(PHDASTATE pThis)
{
    LogFlowFuncEnter();

    DEVHDA_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    AssertPtr(pThis->pTimer);

    if (!pThis->fTimerActive)
    {
        LogRel2(("HDA: Starting transfers\n"));

        pThis->fTimerActive  = true;
        pThis->tsTimerExpire = TMTimerGet(pThis->pTimer) + pThis->cTimerTicks; /* Update current time timestamp. */

        /* Start transfers. */
        hdaTimerMain(pThis);
    }

    DEVHDA_UNLOCK_BOTH(pThis);

    return VINF_SUCCESS;
}

/**
 * Starts the internal audio device timer (if not started yet).
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 */
static int hdaTimerMaybeStart(PHDASTATE pThis)
{
    LogFlowFuncEnter();

    if (!pThis->pTimer)
        return VERR_WRONG_ORDER;

    pThis->cStreamsActive++;

    /* Only start the timer at the first active stream. */
    if (pThis->cStreamsActive == 1)
        return hdaTimerStart(pThis);

    return VINF_SUCCESS;
}

/**
 * Stops the internal audio device timer.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 */
static int hdaTimerStop(PHDASTATE pThis)
{
    LogFlowFuncEnter();

    if (!pThis->pTimer) /* Only can happen on device construction time, so no locking needed here. */
        return VINF_SUCCESS;

    DEVHDA_LOCK_BOTH_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    if (pThis->fTimerActive)
    {
        LogRel2(("HDA: Stopping transfers ...\n"));

        pThis->fTimerActive = false;

        /* Note: Do not stop the timer via TMTimerStop() here, as there still might
         *       be queued audio data which needs to be handled (e.g. played back) first
         *       before actually stopping the timer for good. */
    }

    DEVHDA_UNLOCK_BOTH(pThis);

    return VINF_SUCCESS;
}

/**
 * Decreases the active HDA streams count by one and
 * then checks if the internal audio device timer can be
 * stopped.
 *
 * @return  IPRT status code.
 * @param   pThis               HDA state.
 */
static int hdaTimerMaybeStop(PHDASTATE pThis)
{
    LogFlowFuncEnter();

    if (!pThis->pTimer)
        return VERR_WRONG_ORDER;

    if (pThis->cStreamsActive) /* Disable can be called mupltiple times. */
    {
        pThis->cStreamsActive--;

        if (pThis->cStreamsActive == 0)
            return hdaTimerStop(pThis);
    }

    return VINF_SUCCESS;
}

/**
 * Main routine for the device timer.
 *
 * @param   pThis               HDA state.
 */
static void hdaTimerMain(PHDASTATE pThis)
{
    AssertPtrReturnVoid(pThis);

    STAM_PROFILE_START(&pThis->StatTimer, a);

    DEVHDA_LOCK_BOTH_RETURN_VOID(pThis);

    /* Flag indicating whether to kick the timer again for a
     * new data processing round. */
    bool fKickTimer = false;

    hdaDoTransfers(pThis);

    /* Do we need to kick the timer again? */
    if (   AudioMixerSinkIsActive(pThis->SinkFront.pMixSink)
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
        || AudioMixerSinkIsActive(pThis->SinkCenterLFE.pMixSink)
        || AudioMixerSinkIsActive(pThis->SinkRear.pMixSink)
#endif
        || AudioMixerSinkIsActive(pThis->SinkLineIn.pMixSink)
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
        || AudioMixerSinkIsActive(pThis->SinkMicIn.pMixSink)
#endif
        )
    {
        fKickTimer = true;
    }

    if (   ASMAtomicReadBool(&pThis->fTimerActive)
        || fKickTimer)
    {
        /* Kick the timer again. */
        pThis->tsTimerExpire += pThis->cTimerTicks;

        TMTimerSet(pThis->pTimer, pThis->tsTimerExpire);
    }
    else
        LogRel2(("HDA: Stopped transfers\n"));

    DEVHDA_UNLOCK_BOTH(pThis);

    STAM_PROFILE_STOP(&pThis->StatTimer, a);
}

#ifdef HDA_USE_DMA_ACCESS_HANDLER
/**
 * HC access handler for the FIFO.
 *
 * @returns VINF_SUCCESS if the handler have carried out the operation.
 * @returns VINF_PGM_HANDLER_DO_DEFAULT if the caller should carry out the access operation.
 * @param   pVM             VM Handle.
 * @param   pVCpu           The cross context CPU structure for the calling EMT.
 * @param   GCPhys          The physical address the guest is writing to.
 * @param   pvPhys          The HC mapping of that address.
 * @param   pvBuf           What the guest is reading/writing.
 * @param   cbBuf           How much it's reading/writing.
 * @param   enmAccessType   The access type.
 * @param   enmOrigin       Who is making the access.
 * @param   pvUser          User argument.
 */
static DECLCALLBACK(VBOXSTRICTRC) hdaDMAAccessHandler(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void *pvPhys,
                                                      void *pvBuf, size_t cbBuf,
                                                      PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    RT_NOREF(pVM, pVCpu, pvPhys, pvBuf, enmOrigin);

    PHDADMAACCESSHANDLER pHandler = (PHDADMAACCESSHANDLER)pvUser;
    AssertPtr(pHandler);

    PHDASTREAM pStream = pHandler->pStream;
    AssertPtr(pStream);

    Assert(GCPhys >= pHandler->GCPhysFirst);
    Assert(GCPhys <= pHandler->GCPhysLast);
    Assert(enmAccessType == PGMACCESSTYPE_WRITE);

    /* Not within BDLE range? Bail out. */
    if (   (GCPhys         < pHandler->BDLEAddr)
        || (GCPhys + cbBuf > pHandler->BDLEAddr + pHandler->BDLESize))
    {
        return VINF_PGM_HANDLER_DO_DEFAULT;
    }

    switch(enmAccessType)
    {
        case PGMACCESSTYPE_WRITE:
        {
# ifdef DEBUG
            PHDASTREAMDBGINFO pStreamDbg = &pStream->Dbg;

            const uint64_t tsNowNs     = RTTimeNanoTS();
            const uint32_t tsElapsedMs = (tsNowNs - pStreamDbg->tsWriteSlotBegin) / 1000 / 1000;

            uint64_t cWritesHz   = ASMAtomicReadU64(&pStreamDbg->cWritesHz);
            uint64_t cbWrittenHz = ASMAtomicReadU64(&pStreamDbg->cbWrittenHz);

            if (tsElapsedMs >= (1000 / HDA_TIMER_HZ))
            {
                LogFunc(("[SD%RU8] %RU32ms elapsed, cbWritten=%RU64, cWritten=%RU64 -- %RU32 bytes on average per time slot (%zums)\n",
                         pStream->u8SD, tsElapsedMs, cbWrittenHz, cWritesHz,
                         ASMDivU64ByU32RetU32(cbWrittenHz, cWritesHz ? cWritesHz : 1), 1000 / HDA_TIMER_HZ));

                pStreamDbg->tsWriteSlotBegin = tsNowNs;

                cWritesHz   = 0;
                cbWrittenHz = 0;
            }

            cWritesHz   += 1;
            cbWrittenHz += cbBuf;

            ASMAtomicIncU64(&pStreamDbg->cWritesTotal);
            ASMAtomicAddU64(&pStreamDbg->cbWrittenTotal, cbBuf);

            ASMAtomicWriteU64(&pStreamDbg->cWritesHz,   cWritesHz);
            ASMAtomicWriteU64(&pStreamDbg->cbWrittenHz, cbWrittenHz);

            LogFunc(("[SD%RU8] Writing %3zu @ 0x%x (off %zu)\n",
                     pStream->u8SD, cbBuf, GCPhys, GCPhys - pHandler->BDLEAddr));

            LogFunc(("[SD%RU8] cWrites=%RU64, cbWritten=%RU64 -> %RU32 bytes on average\n",
                     pStream->u8SD, pStreamDbg->cWritesTotal, pStreamDbg->cbWrittenTotal,
                     ASMDivU64ByU32RetU32(pStreamDbg->cbWrittenTotal, pStreamDbg->cWritesTotal)));
# endif

# ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
            RTFILE fh;
            RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaDMAAccessWrite.pcm",
                       RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
            RTFileWrite(fh, pvBuf, cbBuf, NULL);
            RTFileClose(fh);
# endif

# ifdef HDA_USE_DMA_ACCESS_HANDLER_WRITING
            PRTCIRCBUF pCircBuf = pStream->State.pCircBuf;
            AssertPtr(pCircBuf);

            uint8_t *pbBuf = (uint8_t *)pvBuf;
            while (cbBuf)
            {
                /* Make sure we only copy as much as the stream's FIFO can hold (SDFIFOS, 18.2.39). */
                void *pvChunk;
                size_t cbChunk;
                RTCircBufAcquireWriteBlock(pCircBuf, cbBuf, &pvChunk, &cbChunk);

                if (cbChunk)
                {
                    memcpy(pvChunk, pbBuf, cbChunk);

                    pbBuf  += cbChunk;
                    Assert(cbBuf >= cbChunk);
                    cbBuf  -= cbChunk;
                }
                else
                {
                    //AssertMsg(RTCircBufFree(pCircBuf), ("No more space but still %zu bytes to write\n", cbBuf));
                    break;
                }

                LogFunc(("[SD%RU8] cbChunk=%zu\n", pStream->u8SD, cbChunk));

                RTCircBufReleaseWriteBlock(pCircBuf, cbChunk);
            }
# endif /* HDA_USE_DMA_ACCESS_HANDLER_WRITING */
            break;
        }

        default:
            AssertMsgFailed(("Access type not implemented\n"));
            break;
    }

    return VINF_PGM_HANDLER_DO_DEFAULT;
}
#endif /* HDA_USE_DMA_ACCESS_HANDLER */

/**
 * Soft reset of the device triggered via GCTL.
 *
 * @param   pThis   HDA state.
 *
 */
static void hdaGCTLReset(PHDASTATE pThis)
{
    LogFlowFuncEnter();

# ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
    /*
     * Stop the timer, if any.
     */
    hdaTimerStop(pThis);

    pThis->cStreamsActive = 0;
# endif

    memset(pThis->au32Regs, 0, sizeof(pThis->au32Regs));
    /* See 6.2.1. */
    HDA_REG(pThis, GCAP)     = HDA_MAKE_GCAP(HDA_MAX_SDO /* Ouput streams */,
                                             HDA_MAX_SDI /* Input streams */,
                                             0           /* Bidirectional output streams */,
                                             0           /* Serial data out signals */,
                                             1           /* 64-bit */);
    HDA_REG(pThis, VMIN)     = 0x00;                     /* see 6.2.2 */
    HDA_REG(pThis, VMAJ)     = 0x01;                     /* see 6.2.3 */
    /* Announce the full 60 words output payload. */
    HDA_REG(pThis, OUTPAY)   = 0x003C;                   /* see 6.2.4 */
    /* Announce the full 29 words input payload. */
    HDA_REG(pThis, INPAY)    = 0x001D;                   /* see 6.2.5 */
    HDA_REG(pThis, CORBSIZE) = 0x42;                     /* see 6.2.1 */
    HDA_REG(pThis, RIRBSIZE) = 0x42;                     /* see 6.2.1 */

    /*
     * Stop any audio currently playing and/or recording.
     */
    if (pThis->SinkFront.pMixSink)
        AudioMixerSinkReset(pThis->SinkFront.pMixSink);
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    if (pThis->SinkMicIn.pMixSink)
        AudioMixerSinkReset(pThis->SinkMicIn.pMixSink);
# endif
    if (pThis->SinkLineIn.pMixSink)
        AudioMixerSinkReset(pThis->SinkLineIn.pMixSink);
# ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    if (pThis->SinkCenterLFE.pMixSink)
        AudioMixerSinkReset(pThis->SinkCenterLFE.pMixSink);
    if (pThis->SinkRear.pMixSink)
        AudioMixerSinkReset(pThis->SinkRear.pMixSink);
# endif

    /*
     * Reset the codec.
     */
    if (   pThis->pCodec
        && pThis->pCodec->pfnReset)
    {
        pThis->pCodec->pfnReset(pThis->pCodec);
    }

    /*
     * Set some sensible defaults for which HDA sinks
     * are connected to which stream number.
     *
     * We use SD0 for input and SD4 for output by default.
     * These stream numbers can be changed by the guest dynamically lateron.
     */
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    hdaMixerSetStream(pThis, PDMAUDIOMIXERCTL_MIC_IN    , 1 /* SD0 */, 0 /* Channel */);
#endif
    hdaMixerSetStream(pThis, PDMAUDIOMIXERCTL_LINE_IN   , 1 /* SD0 */, 0 /* Channel */);

    hdaMixerSetStream(pThis, PDMAUDIOMIXERCTL_FRONT     , 5 /* SD4 */, 0 /* Channel */);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    hdaMixerSetStream(pThis, PDMAUDIOMIXERCTL_CENTER_LFE, 5 /* SD4 */, 0 /* Channel */);
    hdaMixerSetStream(pThis, PDMAUDIOMIXERCTL_REAR      , 5 /* SD4 */, 0 /* Channel */);
#endif

    pThis->cbCorbBuf = 256 * sizeof(uint32_t); /** @todo Use a define here. */

    if (pThis->pu32CorbBuf)
        RT_BZERO(pThis->pu32CorbBuf, pThis->cbCorbBuf);
    else
        pThis->pu32CorbBuf = (uint32_t *)RTMemAllocZ(pThis->cbCorbBuf);

    pThis->cbRirbBuf = 256 * sizeof(uint64_t); /** @todo Use a define here. */
    if (pThis->pu64RirbBuf)
        RT_BZERO(pThis->pu64RirbBuf, pThis->cbRirbBuf);
    else
        pThis->pu64RirbBuf = (uint64_t *)RTMemAllocZ(pThis->cbRirbBuf);

    for (uint8_t uSD = 0; uSD < HDA_MAX_STREAMS; ++uSD)
    {
        /* Remove the RUN bit from SDnCTL in case the stream was in a running state before. */
        HDA_STREAM_REG(pThis, CTL, uSD) &= ~HDA_SDCTL_RUN;
        hdaStreamReset(pThis, &pThis->aStreams[uSD], uSD);
    }

    /* Clear stream tags <-> objects mapping table. */
    RT_ZERO(pThis->aTags);

    /* Emulation of codec "wake up" (HDA spec 5.5.1 and 6.5). */
    HDA_REG(pThis, STATESTS) = 0x1;

    LogFlowFuncLeave();
    LogRel(("HDA: Reset\n"));
}


/**
 * Timer callback which handles the audio data transfers on a periodic basis.
 *
 * @param   pDevIns             Device instance.
 * @param   pTimer              Timer which was used when calling this.
 * @param   pvUser              User argument as PHDASTATE.
 */
static DECLCALLBACK(void) hdaTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer);

    PHDASTATE pThis = (PHDASTATE)pvUser;
    Assert(pThis == PDMINS_2_DATA(pDevIns, PHDASTATE));
    AssertPtr(pThis);

    hdaTimerMain(pThis);
}

#else /* VBOX_WITH_AUDIO_HDA_CALLBACKS */

static DECLCALLBACK(int) hdaCallbackInput(PDMAUDIOBACKENDCBTYPE enmType, void *pvCtx, size_t cbCtx, void *pvUser, size_t cbUser)
{
    Assert(enmType == PDMAUDIOCALLBACKTYPE_INPUT);
    AssertPtrReturn(pvCtx,  VERR_INVALID_POINTER);
    AssertReturn(cbCtx,     VERR_INVALID_PARAMETER);
    AssertPtrReturn(pvUser, VERR_INVALID_POINTER);
    AssertReturn(cbUser,    VERR_INVALID_PARAMETER);

    PHDACALLBACKCTX pCtx = (PHDACALLBACKCTX)pvCtx;
    AssertReturn(cbCtx == sizeof(HDACALLBACKCTX), VERR_INVALID_PARAMETER);

    PPDMAUDIOCBDATA_DATA_INPUT pData = (PPDMAUDIOCBDATA_DATA_INPUT)pvUser;
    AssertReturn(cbUser == sizeof(PDMAUDIOCBDATA_DATA_INPUT), VERR_INVALID_PARAMETER);

    return hdaStreamDoDMA(pCtx->pThis, PI_INDEX, UINT32_MAX, &pData->cbOutRead);
}

static DECLCALLBACK(int) hdaCallbackOutput(PDMAUDIOBACKENDCBTYPE enmType, void *pvCtx, size_t cbCtx, void *pvUser, size_t cbUser)
{
    Assert(enmType == PDMAUDIOCALLBACKTYPE_OUTPUT);
    AssertPtrReturn(pvCtx,  VERR_INVALID_POINTER);
    AssertReturn(cbCtx,     VERR_INVALID_PARAMETER);
    AssertPtrReturn(pvUser, VERR_INVALID_POINTER);
    AssertReturn(cbUser,    VERR_INVALID_PARAMETER);

    PHDACALLBACKCTX pCtx = (PHDACALLBACKCTX)pvCtx;
    AssertReturn(cbCtx == sizeof(HDACALLBACKCTX), VERR_INVALID_PARAMETER);

    PPDMAUDIOCBDATA_DATA_OUTPUT pData = (PPDMAUDIOCBDATA_DATA_OUTPUT)pvUser;
    AssertReturn(cbUser == sizeof(PDMAUDIOCBDATA_DATA_OUTPUT), VERR_INVALID_PARAMETER);

    PHDASTATE pThis = pCtx->pThis;

    int rc = hdaStreamDoDMA(pCtx->pThis, PO_INDEX, UINT32_MAX, &pData->cbOutWritten);
    if (   RT_SUCCESS(rc)
        && pData->cbOutWritten)
    {
        PHDADRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, HDADRIVER, Node)
        {
            uint32_t cFramesPlayed;
            int rc2 = pDrv->pConnector->pfnPlay(pDrv->pConnector, &cFramesPlayed);
            LogFlowFunc(("LUN#%RU8: cFramesPlayed=%RU32, rc=%Rrc\n", pDrv->uLUN, cFramesPlayed, rc2));
        }
    }
}
#endif /* VBOX_WITH_AUDIO_HDA_CALLBACKS */

/**
 * Main routine to perform the actual audio data transfers from the HDA streams
 * to the backend(s) and vice versa.
 *
 * @param   pThis               HDA state.
 */
static void hdaDoTransfers(PHDASTATE pThis)
{
    PHDASTREAM pStreamLineIn  = hdaGetStreamFromSink(pThis, &pThis->SinkLineIn);
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    PHDASTREAM pStreamMicIn   = hdaGetStreamFromSink(pThis, &pThis->SinkMicIn);
#endif
    PHDASTREAM pStreamFront   = hdaGetStreamFromSink(pThis, &pThis->SinkFront);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    /** @todo See note below. */
#endif

    hdaStreamUpdate(pStreamFront,  true /* fInTimer */);
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    hdaStreamUpdate(pStreamMicIn,  true /* fInTimer */);
#endif
    hdaStreamUpdate(pStreamLineIn, true /* fInTimer */);
}

#ifdef DEBUG_andy
# define HDA_DEBUG_DMA
#endif

#endif /* IN_RING3 */

/* MMIO callbacks */

/**
 * @callback_method_impl{FNIOMMMIOREAD, Looks up and calls the appropriate handler.}
 *
 * @note During implementation, we discovered so-called "forgotten" or "hole"
 *       registers whose description is not listed in the RPM, datasheet, or
 *       spec.
 */
PDMBOTHCBDECL(int) hdaMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    PHDASTATE   pThis  = PDMINS_2_DATA(pDevIns, PHDASTATE);
    int         rc;
    RT_NOREF_PV(pvUser);

    /*
     * Look up and log.
     */
    uint32_t        offReg = GCPhysAddr - pThis->MMIOBaseAddr;
    int             idxRegDsc = hdaRegLookup(offReg);    /* Register descriptor index. */
#ifdef LOG_ENABLED
    unsigned const  cbLog     = cb;
    uint32_t        offRegLog = offReg;
#endif

    Log3Func(("offReg=%#x cb=%#x\n", offReg, cb));
    Assert(cb == 4); Assert((offReg & 3) == 0);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_READ);

    if (!(HDA_REG(pThis, GCTL) & HDA_GCTL_CRST) && idxRegDsc != HDA_REG_GCTL)
        LogFunc(("Access to registers except GCTL is blocked while reset\n"));

    if (idxRegDsc == -1)
        LogRel(("HDA: Invalid read access @0x%x (bytes=%u)\n", offReg, cb));

    if (idxRegDsc != -1)
    {
        /* Leave lock before calling read function. */
        DEVHDA_UNLOCK(pThis);

        /* ASSUMES gapless DWORD at end of map. */
        if (g_aHdaRegMap[idxRegDsc].size == 4)
        {
            /*
             * Straight forward DWORD access.
             */
            rc = g_aHdaRegMap[idxRegDsc].pfnRead(pThis, idxRegDsc, (uint32_t *)pv);
            Log3Func(("\tRead %s => %x (%Rrc)\n", g_aHdaRegMap[idxRegDsc].abbrev, *(uint32_t *)pv, rc));
        }
        else
        {
            /*
             * Multi register read (unless there are trailing gaps).
             * ASSUMES that only DWORD reads have sideeffects.
             */
            uint32_t u32Value = 0;
            unsigned cbLeft   = 4;
            do
            {
                uint32_t const  cbReg        = g_aHdaRegMap[idxRegDsc].size;
                uint32_t        u32Tmp       = 0;

                rc = g_aHdaRegMap[idxRegDsc].pfnRead(pThis, idxRegDsc, &u32Tmp);
                Log3Func(("\tRead %s[%db] => %x (%Rrc)*\n", g_aHdaRegMap[idxRegDsc].abbrev, cbReg, u32Tmp, rc));
                if (rc != VINF_SUCCESS)
                    break;
                u32Value |= (u32Tmp & g_afMasks[cbReg]) << ((4 - cbLeft) * 8);

                cbLeft -= cbReg;
                offReg += cbReg;
                idxRegDsc++;
            } while (cbLeft > 0 && g_aHdaRegMap[idxRegDsc].offset == offReg);

            if (rc == VINF_SUCCESS)
                *(uint32_t *)pv = u32Value;
            else
                Assert(!IOM_SUCCESS(rc));
        }
    }
    else
    {
        DEVHDA_UNLOCK(pThis);

        rc = VINF_IOM_MMIO_UNUSED_FF;
        Log3Func(("\tHole at %x is accessed for read\n", offReg));
    }

    /*
     * Log the outcome.
     */
#ifdef LOG_ENABLED
    if (cbLog == 4)
        Log3Func(("\tReturning @%#05x -> %#010x %Rrc\n", offRegLog, *(uint32_t *)pv, rc));
    else if (cbLog == 2)
        Log3Func(("\tReturning @%#05x -> %#06x %Rrc\n", offRegLog, *(uint16_t *)pv, rc));
    else if (cbLog == 1)
        Log3Func(("\tReturning @%#05x -> %#04x %Rrc\n", offRegLog, *(uint8_t *)pv, rc));
#endif
    return rc;
}


DECLINLINE(int) hdaWriteReg(PHDASTATE pThis, int idxRegDsc, uint32_t u32Value, char const *pszLog)
{
    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_READ);

    if (!(HDA_REG(pThis, GCTL) & HDA_GCTL_CRST) && idxRegDsc != HDA_REG_GCTL)
    {
        Log(("hdaWriteReg: Warning: Access to %s is blocked while controller is in reset mode\n", g_aHdaRegMap[idxRegDsc].abbrev));
        LogRel2(("HDA: Warning: Access to register %s is blocked while controller is in reset mode\n",
                 g_aHdaRegMap[idxRegDsc].abbrev));

        DEVHDA_UNLOCK(pThis);
        return VINF_SUCCESS;
    }

    /*
     * Handle RD (register description) flags.
     */

    /* For SDI / SDO: Check if writes to those registers are allowed while SDCTL's RUN bit is set. */
    if (idxRegDsc >= HDA_NUM_GENERAL_REGS)
    {
        const uint32_t uSDCTL = HDA_STREAM_REG(pThis, CTL, HDA_SD_NUM_FROM_REG(pThis, CTL, idxRegDsc));

        /*
         * Some OSes (like Win 10 AU) violate the spec by writing stuff to registers which are not supposed to be be touched
         * while SDCTL's RUN bit is set. So just ignore those values.
         */

            /* Is the RUN bit currently set? */
        if (   RT_BOOL(uSDCTL & HDA_SDCTL_RUN)
            /* Are writes to the register denied if RUN bit is set? */
            && !(g_aHdaRegMap[idxRegDsc].fFlags & HDA_RD_FLAG_SD_WRITE_RUN))
        {
            Log(("hdaWriteReg: Warning: Access to %s is blocked! %R[sdctl]\n", g_aHdaRegMap[idxRegDsc].abbrev, uSDCTL));
            LogRel2(("HDA: Warning: Access to register %s is blocked while the stream's RUN bit is set\n",
                     g_aHdaRegMap[idxRegDsc].abbrev));

            DEVHDA_UNLOCK(pThis);
            return VINF_SUCCESS;
        }
    }

    /* Leave the lock before calling write function. */
    DEVHDA_UNLOCK(pThis);

#ifdef LOG_ENABLED
    uint32_t const idxRegMem   = g_aHdaRegMap[idxRegDsc].mem_idx;
    uint32_t const u32OldValue = pThis->au32Regs[idxRegMem];
#endif
    int rc = g_aHdaRegMap[idxRegDsc].pfnWrite(pThis, idxRegDsc, u32Value);
    Log3Func(("Written value %#x to %s[%d byte]; %x => %x%s\n", u32Value, g_aHdaRegMap[idxRegDsc].abbrev,
              g_aHdaRegMap[idxRegDsc].size, u32OldValue, pThis->au32Regs[idxRegMem], pszLog));
    RT_NOREF(pszLog);
    return rc;
}


/**
 * @callback_method_impl{FNIOMMMIOWRITE, Looks up and calls the appropriate handler.}
 */
PDMBOTHCBDECL(int) hdaMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    PHDASTATE pThis  = PDMINS_2_DATA(pDevIns, PHDASTATE);
    int       rc;
    RT_NOREF_PV(pvUser);

    /*
     * The behavior of accesses that aren't aligned on natural boundraries is
     * undefined. Just reject them outright.
     */
    /** @todo IOM could check this, it could also split the 8 byte accesses for us. */
    Assert(cb == 1 || cb == 2 || cb == 4 || cb == 8);
    if (GCPhysAddr & (cb - 1))
        return PDMDevHlpDBGFStop(pDevIns, RT_SRC_POS, "misaligned write access: GCPhysAddr=%RGp cb=%u\n", GCPhysAddr, cb);

    /*
     * Look up and log the access.
     */
    uint32_t    offReg = GCPhysAddr - pThis->MMIOBaseAddr;
    int         idxRegDsc = hdaRegLookup(offReg);
    uint32_t    idxRegMem = idxRegDsc != -1 ? g_aHdaRegMap[idxRegDsc].mem_idx : UINT32_MAX;
    uint64_t    u64Value;
    if (cb == 4)        u64Value = *(uint32_t const *)pv;
    else if (cb == 2)   u64Value = *(uint16_t const *)pv;
    else if (cb == 1)   u64Value = *(uint8_t const *)pv;
    else if (cb == 8)   u64Value = *(uint64_t const *)pv;
    else
    {
        u64Value = 0;   /* shut up gcc. */
        AssertReleaseMsgFailed(("%u\n", cb));
    }

#ifdef LOG_ENABLED
    uint32_t const u32LogOldValue = idxRegDsc >= 0 ? pThis->au32Regs[idxRegMem] : UINT32_MAX;
    if (idxRegDsc == -1)
        Log3Func(("@%#05x u32=%#010x cb=%d\n", offReg, *(uint32_t const *)pv, cb));
    else if (cb == 4)
        Log3Func(("@%#05x u32=%#010x %s\n", offReg, *(uint32_t *)pv, g_aHdaRegMap[idxRegDsc].abbrev));
    else if (cb == 2)
        Log3Func(("@%#05x u16=%#06x (%#010x) %s\n", offReg, *(uint16_t *)pv, *(uint32_t *)pv, g_aHdaRegMap[idxRegDsc].abbrev));
    else if (cb == 1)
        Log3Func(("@%#05x u8=%#04x (%#010x) %s\n", offReg, *(uint8_t *)pv, *(uint32_t *)pv, g_aHdaRegMap[idxRegDsc].abbrev));

    if (idxRegDsc >= 0 && g_aHdaRegMap[idxRegDsc].size != cb)
        Log3Func(("\tsize=%RU32 != cb=%u!!\n", g_aHdaRegMap[idxRegDsc].size, cb));
#endif

    /*
     * Try for a direct hit first.
     */
    if (idxRegDsc != -1 && g_aHdaRegMap[idxRegDsc].size == cb)
    {
        rc = hdaWriteReg(pThis, idxRegDsc, u64Value, "");
        Log3Func(("\t%#x -> %#x\n", u32LogOldValue, idxRegMem != UINT32_MAX ? pThis->au32Regs[idxRegMem] : UINT32_MAX));
    }
    /*
     * Partial or multiple register access, loop thru the requested memory.
     */
    else
    {
        /*
         * If it's an access beyond the start of the register, shift the input
         * value and fill in missing bits. Natural alignment rules means we
         * will only see 1 or 2 byte accesses of this kind, so no risk of
         * shifting out input values.
         */
        if (idxRegDsc == -1 && (idxRegDsc = hdaRegLookupWithin(offReg)) != -1)
        {
            uint32_t const cbBefore = offReg - g_aHdaRegMap[idxRegDsc].offset; Assert(cbBefore > 0 && cbBefore < 4);
            offReg    -= cbBefore;
            idxRegMem = g_aHdaRegMap[idxRegDsc].mem_idx;
            u64Value <<= cbBefore * 8;
            u64Value  |= pThis->au32Regs[idxRegMem] & g_afMasks[cbBefore];
            Log3Func(("\tWithin register, supplied %u leading bits: %#llx -> %#llx ...\n",
                      cbBefore * 8, ~g_afMasks[cbBefore] & u64Value, u64Value));
        }

        /* Loop thru the write area, it may cover multiple registers. */
        rc = VINF_SUCCESS;
        for (;;)
        {
            uint32_t cbReg;
            if (idxRegDsc != -1)
            {
                idxRegMem = g_aHdaRegMap[idxRegDsc].mem_idx;
                cbReg = g_aHdaRegMap[idxRegDsc].size;
                if (cb < cbReg)
                {
                    u64Value |= pThis->au32Regs[idxRegMem] & g_afMasks[cbReg] & ~g_afMasks[cb];
                    Log3Func(("\tSupplying missing bits (%#x): %#llx -> %#llx ...\n",
                              g_afMasks[cbReg] & ~g_afMasks[cb], u64Value & g_afMasks[cb], u64Value));
                }
#ifdef LOG_ENABLED
                uint32_t uLogOldVal = pThis->au32Regs[idxRegMem];
#endif
                rc = hdaWriteReg(pThis, idxRegDsc, u64Value, "*");
                Log3Func(("\t%#x -> %#x\n", uLogOldVal, pThis->au32Regs[idxRegMem]));
            }
            else
            {
                LogRel(("HDA: Invalid write access @0x%x\n", offReg));
                cbReg = 1;
            }
            if (rc != VINF_SUCCESS)
                break;
            if (cbReg >= cb)
                break;

            /* Advance. */
            offReg += cbReg;
            cb     -= cbReg;
            u64Value >>= cbReg * 8;
            if (idxRegDsc == -1)
                idxRegDsc = hdaRegLookup(offReg);
            else
            {
                idxRegDsc++;
                if (   (unsigned)idxRegDsc >= RT_ELEMENTS(g_aHdaRegMap)
                    || g_aHdaRegMap[idxRegDsc].offset != offReg)
                {
                    idxRegDsc = -1;
                }
            }
        }
    }

    return rc;
}


/* PCI callback. */

#ifdef IN_RING3
/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int)  hdaPciIoRegionMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                            RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF(iRegion, enmType);
    PHDASTATE   pThis = RT_FROM_MEMBER(pPciDev, HDASTATE, PciDev);

    /*
     * 18.2 of the ICH6 datasheet defines the valid access widths as byte, word, and double word.
     *
     * Let IOM talk DWORDs when reading, saves a lot of complications. On
     * writing though, we have to do it all ourselves because of sideeffects.
     */
    Assert(enmType == PCI_ADDRESS_SPACE_MEM);
    int rc = PDMDevHlpMMIORegister(pDevIns, GCPhysAddress, cb, NULL /*pvUser*/,
                                     IOMMMIO_FLAGS_READ_DWORD
                                   | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                                   hdaMMIOWrite, hdaMMIORead, "HDA");

    if (RT_FAILURE(rc))
        return rc;

    if (pThis->fR0Enabled)
    {
        rc = PDMDevHlpMMIORegisterR0(pDevIns, GCPhysAddress, cb, NIL_RTR0PTR /*pvUser*/,
                                     "hdaMMIOWrite", "hdaMMIORead");
        if (RT_FAILURE(rc))
            return rc;
    }

    if (pThis->fRCEnabled)
    {
        rc = PDMDevHlpMMIORegisterRC(pDevIns, GCPhysAddress, cb, NIL_RTRCPTR /*pvUser*/,
                                     "hdaMMIOWrite", "hdaMMIORead");
        if (RT_FAILURE(rc))
            return rc;
    }

    pThis->MMIOBaseAddr = GCPhysAddress;
    return VINF_SUCCESS;
}


/* Saved state callbacks. */

static int hdaSaveStream(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, PHDASTREAM pStrm)
{
    RT_NOREF(pDevIns);
#ifdef VBOX_STRICT
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);
#endif

    Log2Func(("[SD%RU8]\n", pStrm->u8SD));

    /* Save stream ID. */
    int rc = SSMR3PutU8(pSSM, pStrm->u8SD);
    AssertRCReturn(rc, rc);
    Assert(pStrm->u8SD < HDA_MAX_STREAMS);

    rc = SSMR3PutStructEx(pSSM, &pStrm->State, sizeof(HDASTREAMSTATE), 0 /*fFlags*/, g_aSSMStreamStateFields7, NULL);
    AssertRCReturn(rc, rc);

#ifdef VBOX_STRICT /* Sanity checks. */
    uint64_t u64BaseDMA = RT_MAKE_U64(HDA_STREAM_REG(pThis, BDPL, pStrm->u8SD),
                                      HDA_STREAM_REG(pThis, BDPU, pStrm->u8SD));
    uint16_t u16LVI     = HDA_STREAM_REG(pThis, LVI, pStrm->u8SD);
    uint32_t u32CBL     = HDA_STREAM_REG(pThis, CBL, pStrm->u8SD);

    Assert(u64BaseDMA == pStrm->u64BDLBase);
    Assert(u16LVI     == pStrm->u16LVI);
    Assert(u32CBL     == pStrm->u32CBL);
#endif

    rc = SSMR3PutStructEx(pSSM, &pStrm->State.BDLE.Desc, sizeof(HDABDLEDESC),
                          0 /*fFlags*/, g_aSSMBDLEDescFields7, NULL);
    AssertRCReturn(rc, rc);

    rc = SSMR3PutStructEx(pSSM, &pStrm->State.BDLE.State, sizeof(HDABDLESTATE),
                          0 /*fFlags*/, g_aSSMBDLEStateFields7, NULL);
    AssertRCReturn(rc, rc);

    rc = SSMR3PutStructEx(pSSM, &pStrm->State.Period, sizeof(HDASTREAMPERIOD),
                          0 /* fFlags */, g_aSSMStreamPeriodFields7, NULL);
    AssertRCReturn(rc, rc);

#ifdef VBOX_STRICT /* Sanity checks. */
    PHDABDLE pBDLE = &pStrm->State.BDLE;
    if (u64BaseDMA)
    {
        Assert(pStrm->State.uCurBDLE <= u16LVI + 1);

        HDABDLE curBDLE;
        rc = hdaBDLEFetch(pThis, &curBDLE, u64BaseDMA, pStrm->State.uCurBDLE);
        AssertRC(rc);

        Assert(curBDLE.Desc.u32BufSize == pBDLE->Desc.u32BufSize);
        Assert(curBDLE.Desc.u64BufAdr  == pBDLE->Desc.u64BufAdr);
        Assert(curBDLE.Desc.fFlags     == pBDLE->Desc.fFlags);
    }
    else
    {
        Assert(pBDLE->Desc.u64BufAdr  == 0);
        Assert(pBDLE->Desc.u32BufSize == 0);
    }
#endif

    uint32_t cbCircBufSize = 0;
    uint32_t cbCircBufUsed = 0;

    if (pStrm->State.pCircBuf)
    {
        cbCircBufSize = (uint32_t)RTCircBufSize(pStrm->State.pCircBuf);
        cbCircBufUsed = (uint32_t)RTCircBufUsed(pStrm->State.pCircBuf);
    }

    rc = SSMR3PutU32(pSSM, cbCircBufSize);
    AssertRCReturn(rc, rc);

    rc = SSMR3PutU32(pSSM, cbCircBufUsed);
    AssertRCReturn(rc, rc);

    if (cbCircBufUsed)
    {
        /*
         * We now need to get the circular buffer's data without actually modifying
         * the internal read / used offsets -- otherwise we would end up with broken audio
         * data after saving the state.
         *
         * So get the current read offset and serialize the buffer data manually based on that.
         */
        size_t cbCircBufOffRead = RTCircBufOffsetRead(pStrm->State.pCircBuf);

        void  *pvBuf;
        size_t cbBuf;
        RTCircBufAcquireReadBlock(pStrm->State.pCircBuf, cbCircBufUsed, &pvBuf, &cbBuf);

        if (cbBuf)
        {
            size_t cbToRead = cbCircBufUsed;
            size_t cbEnd    = 0;

            if (cbCircBufUsed > cbCircBufOffRead)
                cbEnd = cbCircBufUsed - cbCircBufOffRead;

            if (cbEnd) /* Save end of buffer first. */
            {
                rc = SSMR3PutMem(pSSM, (uint8_t *)pvBuf + cbCircBufSize - cbEnd /* End of buffer */, cbEnd);
                AssertRCReturn(rc, rc);

                Assert(cbToRead >= cbEnd);
                cbToRead -= cbEnd;
            }

            if (cbToRead) /* Save remaining stuff at start of buffer (if any). */
            {
                rc = SSMR3PutMem(pSSM, (uint8_t *)pvBuf - cbCircBufUsed /* Start of buffer */, cbToRead);
                AssertRCReturn(rc, rc);
            }
        }

        RTCircBufReleaseReadBlock(pStrm->State.pCircBuf, 0 /* Don't advance read pointer -- see comment above */);
    }

    Log2Func(("[SD%RU8] LPIB=%RU32, CBL=%RU32, LVI=%RU32\n",
              pStrm->u8SD,
              HDA_STREAM_REG(pThis, LPIB, pStrm->u8SD), HDA_STREAM_REG(pThis, CBL, pStrm->u8SD), HDA_STREAM_REG(pThis, LVI, pStrm->u8SD)));

#ifdef LOG_ENABLED
    hdaBDLEDumpAll(pThis, pStrm->u64BDLBase, pStrm->u16LVI + 1);
#endif

    return rc;
}

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) hdaSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    /* Save Codec nodes states. */
    hdaCodecSaveState(pThis->pCodec, pSSM);

    /* Save MMIO registers. */
    SSMR3PutU32(pSSM, RT_ELEMENTS(pThis->au32Regs));
    SSMR3PutMem(pSSM, pThis->au32Regs, sizeof(pThis->au32Regs));

    /* Save controller-specifc internals. */
    SSMR3PutU64(pSSM, pThis->u64WalClk);
    SSMR3PutU8(pSSM, pThis->u8IRQL);

    /* Save number of streams. */
    SSMR3PutU32(pSSM, HDA_MAX_STREAMS);

    /* Save stream states. */
    for (uint8_t i = 0; i < HDA_MAX_STREAMS; i++)
    {
        int rc = hdaSaveStream(pDevIns, pSSM, &pThis->aStreams[i]);
        AssertRCReturn(rc, rc);
    }

    return VINF_SUCCESS;
}

/**
 * Does required post processing when loading a saved state.
 *
 * @param   pThis               Pointer to HDA state.
 */
static int hdaLoadExecPost(PHDASTATE pThis)
{
    int rc = VINF_SUCCESS;

    bool fStartTimer = false; /* Whether to resume the device timer. */

    /*
     * Enable all previously active streams.
     */
    for (uint8_t i = 0; i < HDA_MAX_STREAMS; i++)
    {
        PHDASTREAM pStream = hdaGetStreamFromSD(pThis, i);
        if (pStream)
        {
            int rc2;

            bool fActive = RT_BOOL(HDA_STREAM_REG(pThis, CTL, i) & HDA_SDCTL_RUN);
            if (fActive)
            {
#ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
                /* Make sure to also create the async I/O thread before actually enabling the stream. */
                rc2 = hdaStreamAsyncIOCreate(pStream);
                AssertRC(rc2);

                /* ... and enabling it. */
                hdaStreamAsyncIOEnable(pStream, true /* fEnable */);
#endif
                /* (Re-)initialize the stream with current values. */
                rc2 = hdaStreamInit(pStream, pStream->u8SD);
                AssertRC(rc2);

                /* Resume the stream's period. */
                hdaStreamPeriodResume(&pStream->State.Period);

                /* (Re-)enable the stream. */
                rc2 = hdaStreamEnable(pStream, true /* fEnable */);
                AssertRC(rc2);

                /* Add the stream to the device setup. */
                rc2 = hdaAddStream(pThis, &pStream->State.Cfg);
                AssertRC(rc2);

#ifdef HDA_USE_DMA_ACCESS_HANDLER
                /* (Re-)install the DMA handler. */
                hdaStreamRegisterDMAHandlers(pThis, pStream);
#endif
                fStartTimer = true;
            }
        }
    }

#ifndef VBOX_WITH_AUDIO_CALLBACKS
    /* Start the timer if one of the above streams were active during taking the saved state. */
    if (fStartTimer)
        hdaTimerMaybeStart(pThis);
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * Handles loading of all saved state versions older than the current one.
 *
 * @param   pThis               Pointer to HDA state.
 * @param   pSSM                Pointer to SSM handle.
 * @param   uVersion            Saved state version to load.
 * @param   uPass               Loading stage to handle.
 */
static int hdaLoadExecLegacy(PHDASTATE pThis, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    RT_NOREF(uPass);

    int rc = VINF_SUCCESS;

    /*
     * Load MMIO registers.
     */
    uint32_t cRegs;
    switch (uVersion)
    {
        case HDA_SSM_VERSION_1:
            /* Starting with r71199, we would save 112 instead of 113
               registers due to some code cleanups.  This only affected trunk
               builds in the 4.1 development period. */
            cRegs = 113;
            if (SSMR3HandleRevision(pSSM) >= 71199)
            {
                uint32_t uVer = SSMR3HandleVersion(pSSM);
                if (   VBOX_FULL_VERSION_GET_MAJOR(uVer) == 4
                    && VBOX_FULL_VERSION_GET_MINOR(uVer) == 0
                    && VBOX_FULL_VERSION_GET_BUILD(uVer) >= 51)
                    cRegs = 112;
            }
            break;

        case HDA_SSM_VERSION_2:
        case HDA_SSM_VERSION_3:
            cRegs = 112;
            AssertCompile(RT_ELEMENTS(pThis->au32Regs) >= 112);
            break;

        /* Since version 4 we store the register count to stay flexible. */
        case HDA_SSM_VERSION_4:
        case HDA_SSM_VERSION_5:
        case HDA_SSM_VERSION_6:
            rc = SSMR3GetU32(pSSM, &cRegs); AssertRCReturn(rc, rc);
            if (cRegs != RT_ELEMENTS(pThis->au32Regs))
                LogRel(("HDA: SSM version cRegs is %RU32, expected %RU32\n", cRegs, RT_ELEMENTS(pThis->au32Regs)));
            break;

        default:
            LogRel(("HDA: Unsupported / too new saved state version (%RU32)\n", uVersion));
            return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    if (cRegs >= RT_ELEMENTS(pThis->au32Regs))
    {
        SSMR3GetMem(pSSM, pThis->au32Regs, sizeof(pThis->au32Regs));
        SSMR3Skip(pSSM, sizeof(uint32_t) * (cRegs - RT_ELEMENTS(pThis->au32Regs)));
    }
    else
        SSMR3GetMem(pSSM, pThis->au32Regs, sizeof(uint32_t) * cRegs);

    /* Make sure to update the base addresses first before initializing any streams down below. */
    pThis->u64CORBBase  = RT_MAKE_U64(HDA_REG(pThis, CORBLBASE), HDA_REG(pThis, CORBUBASE));
    pThis->u64RIRBBase  = RT_MAKE_U64(HDA_REG(pThis, RIRBLBASE), HDA_REG(pThis, RIRBUBASE));
    pThis->u64DPBase    = RT_MAKE_U64(HDA_REG(pThis, DPLBASE) & DPBASE_ADDR_MASK, HDA_REG(pThis, DPUBASE));

    /* Also make sure to update the DMA position bit if this was enabled when saving the state. */
    pThis->fDMAPosition = RT_BOOL(HDA_REG(pThis, DPLBASE) & RT_BIT_32(0));

    /*
     * Note: Saved states < v5 store LVI (u32BdleMaxCvi) for
     *       *every* BDLE state, whereas it only needs to be stored
     *       *once* for every stream. Most of the BDLE state we can
     *       get out of the registers anyway, so just ignore those values.
     *
     *       Also, only the current BDLE was saved, regardless whether
     *       there were more than one (and there are at least two entries,
     *       according to the spec).
     */
#define HDA_SSM_LOAD_BDLE_STATE_PRE_V5(v, x)                                \
    {                                                                       \
        rc = SSMR3Skip(pSSM, sizeof(uint32_t));        /* Begin marker */   \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3GetU64(pSSM, &x.Desc.u64BufAdr);     /* u64BdleCviAddr */ \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3Skip(pSSM, sizeof(uint32_t));        /* u32BdleMaxCvi */  \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3GetU32(pSSM, &x.State.u32BDLIndex);  /* u32BdleCvi */     \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3GetU32(pSSM, &x.Desc.u32BufSize);    /* u32BdleCviLen */  \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3GetU32(pSSM, &x.State.u32BufOff);    /* u32BdleCviPos */  \
        AssertRCReturn(rc, rc);                                             \
        bool fIOC;                                                          \
        rc = SSMR3GetBool(pSSM, &fIOC);                /* fBdleCviIoc */    \
        AssertRCReturn(rc, rc);                                             \
        x.Desc.fFlags = fIOC ? HDA_BDLE_FLAG_IOC : 0;                       \
        rc = SSMR3GetU32(pSSM, &x.State.cbBelowFIFOW); /* cbUnderFifoW */   \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3Skip(pSSM, sizeof(uint8_t) * 256);   /* FIFO */           \
        AssertRCReturn(rc, rc);                                             \
        rc = SSMR3Skip(pSSM, sizeof(uint32_t));        /* End marker */     \
        AssertRCReturn(rc, rc);                                             \
    }

    /*
     * Load BDLEs (Buffer Descriptor List Entries) and DMA counters.
     */
    switch (uVersion)
    {
        case HDA_SSM_VERSION_1:
        case HDA_SSM_VERSION_2:
        case HDA_SSM_VERSION_3:
        case HDA_SSM_VERSION_4:
        {
            /* Only load the internal states.
             * The rest will be initialized from the saved registers later. */

            /* Note 1: Only the *current* BDLE for a stream was saved! */
            /* Note 2: The stream's saving order is/was fixed, so don't touch! */

            /* Output */
            PHDASTREAM pStream = &pThis->aStreams[4];
            rc = hdaStreamInit(pStream, 4 /* Stream descriptor, hardcoded */);
            if (RT_FAILURE(rc))
                break;
            HDA_SSM_LOAD_BDLE_STATE_PRE_V5(uVersion, pStream->State.BDLE);
            pStream->State.uCurBDLE = pStream->State.BDLE.State.u32BDLIndex;

            /* Microphone-In */
            pStream = &pThis->aStreams[2];
            rc = hdaStreamInit(pStream, 2 /* Stream descriptor, hardcoded */);
            if (RT_FAILURE(rc))
                break;
            HDA_SSM_LOAD_BDLE_STATE_PRE_V5(uVersion, pStream->State.BDLE);
            pStream->State.uCurBDLE = pStream->State.BDLE.State.u32BDLIndex;

            /* Line-In */
            pStream = &pThis->aStreams[0];
            rc = hdaStreamInit(pStream, 0 /* Stream descriptor, hardcoded */);
            if (RT_FAILURE(rc))
                break;
            HDA_SSM_LOAD_BDLE_STATE_PRE_V5(uVersion, pStream->State.BDLE);
            pStream->State.uCurBDLE = pStream->State.BDLE.State.u32BDLIndex;
            break;
        }

#undef HDA_SSM_LOAD_BDLE_STATE_PRE_V5

        default: /* Since v5 we support flexible stream and BDLE counts. */
        {
            uint32_t cStreams;
            rc = SSMR3GetU32(pSSM, &cStreams);
            if (RT_FAILURE(rc))
                break;

            if (cStreams > HDA_MAX_STREAMS)
                cStreams = HDA_MAX_STREAMS; /* Sanity. */

            /* Load stream states. */
            for (uint32_t i = 0; i < cStreams; i++)
            {
                uint8_t uStreamID;
                rc = SSMR3GetU8(pSSM, &uStreamID);
                if (RT_FAILURE(rc))
                    break;

                PHDASTREAM pStrm = hdaGetStreamFromSD(pThis, uStreamID);
                HDASTREAM  StreamDummy;

                if (!pStrm)
                {
                    pStrm = &StreamDummy;
                    LogRel2(("HDA: Warning: Stream ID=%RU32 not supported, skipping to load ...\n", uStreamID));
                }

                rc = hdaStreamInit(pStrm, uStreamID);
                if (RT_FAILURE(rc))
                {
                    LogRel(("HDA: Stream #%RU32: Initialization of stream %RU8 failed, rc=%Rrc\n", i, uStreamID, rc));
                    break;
                }

                /*
                 * Load BDLEs (Buffer Descriptor List Entries) and DMA counters.
                 */

                if (uVersion == HDA_SSM_VERSION_5)
                {
                    /* Get the current BDLE entry and skip the rest. */
                    uint16_t cBDLE;

                    rc = SSMR3Skip(pSSM, sizeof(uint32_t));         /* Begin marker */
                    AssertRC(rc);
                    rc = SSMR3GetU16(pSSM, &cBDLE);                 /* cBDLE */
                    AssertRC(rc);
                    rc = SSMR3GetU16(pSSM, &pStrm->State.uCurBDLE); /* uCurBDLE */
                    AssertRC(rc);
                    rc = SSMR3Skip(pSSM, sizeof(uint32_t));         /* End marker */
                    AssertRC(rc);

                    uint32_t u32BDLEIndex;
                    for (uint16_t a = 0; a < cBDLE; a++)
                    {
                        rc = SSMR3Skip(pSSM, sizeof(uint32_t));     /* Begin marker */
                        AssertRC(rc);
                        rc = SSMR3GetU32(pSSM, &u32BDLEIndex);      /* u32BDLIndex */
                        AssertRC(rc);

                        /* Does the current BDLE index match the current BDLE to process? */
                        if (u32BDLEIndex == pStrm->State.uCurBDLE)
                        {
                            rc = SSMR3GetU32(pSSM, &pStrm->State.BDLE.State.cbBelowFIFOW); /* cbBelowFIFOW */
                            AssertRC(rc);
                            rc = SSMR3Skip(pSSM, sizeof(uint8_t) * 256);                   /* FIFO, deprecated */
                            AssertRC(rc);
                            rc = SSMR3GetU32(pSSM, &pStrm->State.BDLE.State.u32BufOff);    /* u32BufOff */
                            AssertRC(rc);
                            rc = SSMR3Skip(pSSM, sizeof(uint32_t));                        /* End marker */
                            AssertRC(rc);
                        }
                        else /* Skip not current BDLEs. */
                        {
                            rc = SSMR3Skip(pSSM,   sizeof(uint32_t)      /* cbBelowFIFOW */
                                                 + sizeof(uint8_t) * 256 /* au8FIFO */
                                                 + sizeof(uint32_t)      /* u32BufOff */
                                                 + sizeof(uint32_t));    /* End marker */
                            AssertRC(rc);
                        }
                    }
                }
                else
                {
                    rc = SSMR3GetStructEx(pSSM, &pStrm->State, sizeof(HDASTREAMSTATE),
                                          0 /* fFlags */, g_aSSMStreamStateFields6, NULL);
                    if (RT_FAILURE(rc))
                        break;

                    /* Get HDABDLEDESC. */
                    uint32_t uMarker;
                    rc = SSMR3GetU32(pSSM, &uMarker);      /* Begin marker. */
                    AssertRC(rc);
                    Assert(uMarker == UINT32_C(0x19200102) /* SSMR3STRUCT_BEGIN */);
                    rc = SSMR3GetU64(pSSM, &pStrm->State.BDLE.Desc.u64BufAdr);
                    AssertRC(rc);
                    rc = SSMR3GetU32(pSSM, &pStrm->State.BDLE.Desc.u32BufSize);
                    AssertRC(rc);
                    bool fFlags = false;
                    rc = SSMR3GetBool(pSSM, &fFlags);      /* Saved states < v7 only stored the IOC as boolean flag. */
                    AssertRC(rc);
                    pStrm->State.BDLE.Desc.fFlags = fFlags ? HDA_BDLE_FLAG_IOC : 0;
                    rc = SSMR3GetU32(pSSM, &uMarker);      /* End marker. */
                    AssertRC(rc);
                    Assert(uMarker == UINT32_C(0x19920406) /* SSMR3STRUCT_END */);

                    rc = SSMR3GetStructEx(pSSM, &pStrm->State.BDLE.State, sizeof(HDABDLESTATE),
                                          0 /* fFlags */, g_aSSMBDLEStateFields6, NULL);
                    if (RT_FAILURE(rc))
                        break;

                    Log2Func(("[SD%RU8] LPIB=%RU32, CBL=%RU32, LVI=%RU32\n",
                              uStreamID,
                              HDA_STREAM_REG(pThis, LPIB, uStreamID), HDA_STREAM_REG(pThis, CBL, uStreamID), HDA_STREAM_REG(pThis, LVI, uStreamID)));
#ifdef LOG_ENABLED
                    hdaBDLEDumpAll(pThis, pStrm->u64BDLBase, pStrm->u16LVI + 1);
#endif
                }

            } /* for cStreams */
            break;
        } /* default */
    }

    return rc;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) hdaLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    LogRel2(("hdaLoadExec: uVersion=%RU32, uPass=0x%x\n", uVersion, uPass));

    /*
     * Load Codec nodes states.
     */
    int rc = hdaCodecLoadState(pThis->pCodec, pSSM, uVersion);
    if (RT_FAILURE(rc))
    {
        LogRel(("HDA: Failed loading codec state (version %RU32, pass 0x%x), rc=%Rrc\n", uVersion, uPass, rc));
        return rc;
    }

    if (uVersion < HDA_SSM_VERSION) /* Handle older saved states? */
    {
        rc = hdaLoadExecLegacy(pThis, pSSM, uVersion, uPass);
        if (RT_SUCCESS(rc))
            rc = hdaLoadExecPost(pThis);

        return rc;
    }

    /*
     * Load MMIO registers.
     */
    uint32_t cRegs;
    rc = SSMR3GetU32(pSSM, &cRegs); AssertRCReturn(rc, rc);
    if (cRegs != RT_ELEMENTS(pThis->au32Regs))
        LogRel(("HDA: SSM version cRegs is %RU32, expected %RU32\n", cRegs, RT_ELEMENTS(pThis->au32Regs)));

    if (cRegs >= RT_ELEMENTS(pThis->au32Regs))
    {
        SSMR3GetMem(pSSM, pThis->au32Regs, sizeof(pThis->au32Regs));
        SSMR3Skip(pSSM, sizeof(uint32_t) * (cRegs - RT_ELEMENTS(pThis->au32Regs)));
    }
    else
        SSMR3GetMem(pSSM, pThis->au32Regs, sizeof(uint32_t) * cRegs);

    /* Make sure to update the base addresses first before initializing any streams down below. */
    pThis->u64CORBBase  = RT_MAKE_U64(HDA_REG(pThis, CORBLBASE), HDA_REG(pThis, CORBUBASE));
    pThis->u64RIRBBase  = RT_MAKE_U64(HDA_REG(pThis, RIRBLBASE), HDA_REG(pThis, RIRBUBASE));
    pThis->u64DPBase    = RT_MAKE_U64(HDA_REG(pThis, DPLBASE) & DPBASE_ADDR_MASK, HDA_REG(pThis, DPUBASE));

    /* Also make sure to update the DMA position bit if this was enabled when saving the state. */
    pThis->fDMAPosition = RT_BOOL(HDA_REG(pThis, DPLBASE) & RT_BIT_32(0));

    /*
     * Load controller-specifc internals.
     * Don't annoy other team mates (forgot this for state v7).
     */
    if (   SSMR3HandleRevision(pSSM) >= 116273
        || SSMR3HandleVersion(pSSM)  >= VBOX_FULL_VERSION_MAKE(5, 2, 0))
    {
        rc = SSMR3GetU64(pSSM, &pThis->u64WalClk);
        AssertRC(rc);

        rc = SSMR3GetU8(pSSM, &pThis->u8IRQL);
        AssertRC(rc);
    }

    /*
     * Load streams.
     */
    uint32_t cStreams;
    rc = SSMR3GetU32(pSSM, &cStreams);
    AssertRC(rc);

    if (cStreams > HDA_MAX_STREAMS)
        cStreams = HDA_MAX_STREAMS; /* Sanity. */

    Log2Func(("cStreams=%RU32\n", cStreams));

    /* Load stream states. */
    for (uint32_t i = 0; i < cStreams; i++)
    {
        uint8_t uStreamID;
        rc = SSMR3GetU8(pSSM, &uStreamID);
        AssertRC(rc);

        PHDASTREAM pStrm = hdaGetStreamFromSD(pThis, uStreamID);
        HDASTREAM  StreamDummy;

        if (!pStrm)
        {
            pStrm = &StreamDummy;
            LogRel2(("HDA: Warning: Loading of stream #%RU8 not supported, skipping to load ...\n", uStreamID));
        }

        rc = hdaStreamInit(pStrm, uStreamID);
        if (RT_FAILURE(rc))
        {
            LogRel(("HDA: Stream #%RU8: Loading initialization failed, rc=%Rrc\n", uStreamID, rc));
            /* Continue. */
        }

        /*
         * Load BDLEs (Buffer Descriptor List Entries) and DMA counters.
         */
        rc = SSMR3GetStructEx(pSSM, &pStrm->State, sizeof(HDASTREAMSTATE),
                              0 /* fFlags */, g_aSSMStreamStateFields7,
                              NULL);
        AssertRC(rc);

        rc = SSMR3GetStructEx(pSSM, &pStrm->State.BDLE.Desc, sizeof(HDABDLEDESC),
                              0 /* fFlags */, g_aSSMBDLEDescFields7, NULL);
        AssertRC(rc);

        rc = SSMR3GetStructEx(pSSM, &pStrm->State.BDLE.State, sizeof(HDABDLESTATE),
                              0 /* fFlags */, g_aSSMBDLEStateFields7, NULL);
        AssertRC(rc);

        Log2Func(("[SD%RU8] %R[bdle]\n", pStrm->u8SD, &pStrm->State.BDLE));

        /*
         * Load period state.
         * Don't annoy other team mates (forgot this for state v7).
         */
        hdaStreamPeriodInit(&pStrm->State.Period,
                            pStrm->u8SD, pStrm->u16LVI, pStrm->u32CBL, &pStrm->State.Cfg);

        if (   SSMR3HandleRevision(pSSM) >= 116273
            || SSMR3HandleVersion(pSSM)  >= VBOX_FULL_VERSION_MAKE(5, 2, 0))
        {
            rc = SSMR3GetStructEx(pSSM, &pStrm->State.Period, sizeof(HDASTREAMPERIOD),
                                  0 /* fFlags */, g_aSSMStreamPeriodFields7, NULL);
            AssertRC(rc);
        }

        /*
         * Load internal (FIFO) buffer.
         */

        uint32_t cbCircBufSize = 0;
        rc = SSMR3GetU32(pSSM, &cbCircBufSize); /* cbCircBuf */
        AssertRC(rc);

        uint32_t cbCircBufUsed = 0;
        rc = SSMR3GetU32(pSSM, &cbCircBufUsed); /* cbCircBuf */
        AssertRC(rc);

        if (cbCircBufSize) /* If 0, skip the buffer. */
        {
            /* Paranoia. */
            AssertReleaseMsg(cbCircBufSize <= _1M,
                             ("HDA: Saved state contains bogus DMA buffer size (%RU32) for stream #%RU8",
                              cbCircBufSize, uStreamID));
            AssertReleaseMsg(cbCircBufUsed <= cbCircBufSize,
                             ("HDA: Saved state contains invalid DMA buffer usage (%RU32/%RU32) for stream #%RU8",
                              cbCircBufUsed, cbCircBufSize, uStreamID));
            AssertPtr(pStrm->State.pCircBuf);

            /* Do we need to cre-create the circular buffer do fit the data size? */
            if (cbCircBufSize != (uint32_t)RTCircBufSize(pStrm->State.pCircBuf))
            {
                RTCircBufDestroy(pStrm->State.pCircBuf);
                pStrm->State.pCircBuf = NULL;

                rc = RTCircBufCreate(&pStrm->State.pCircBuf, cbCircBufSize);
                AssertRC(rc);
            }

            if (   RT_SUCCESS(rc)
                && cbCircBufUsed)
            {
                void  *pvBuf;
                size_t cbBuf;

                RTCircBufAcquireWriteBlock(pStrm->State.pCircBuf, cbCircBufUsed, &pvBuf, &cbBuf);

                if (cbBuf)
                {
                    rc = SSMR3GetMem(pSSM, pvBuf, cbBuf);
                    AssertRC(rc);
                }

                RTCircBufReleaseWriteBlock(pStrm->State.pCircBuf, cbBuf);

                Assert(cbBuf == cbCircBufUsed);
            }
        }

        Log2Func(("[SD%RU8] LPIB=%RU32, CBL=%RU32, LVI=%RU32\n",
                  uStreamID,
                  HDA_STREAM_REG(pThis, LPIB, uStreamID), HDA_STREAM_REG(pThis, CBL, uStreamID), HDA_STREAM_REG(pThis, LVI, uStreamID)));
#ifdef LOG_ENABLED
        hdaBDLEDumpAll(pThis, pStrm->u64BDLBase, pStrm->u16LVI + 1);
#endif
        /** @todo (Re-)initialize active periods? */

    } /* for cStreams */

    rc = hdaLoadExecPost(pThis);
    AssertRC(rc);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/* Debug and log type formatters. */

/**
 * @callback_method_impl{FNRTSTRFORMATTYPE}
 */
static DECLCALLBACK(size_t) hdaDbgFmtBDLE(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                          const char *pszType, void const *pvValue,
                                          int cchWidth, int cchPrecision, unsigned fFlags,
                                          void *pvUser)
{
    RT_NOREF(pszType, cchWidth,  cchPrecision, fFlags, pvUser);
    PHDABDLE pBDLE = (PHDABDLE)pvValue;
    return RTStrFormat(pfnOutput,  pvArgOutput, NULL, 0,
                       "BDLE(idx:%RU32, off:%RU32, fifow:%RU32, IOC:%RTbool, DMA[%RU32 bytes @ 0x%x])",
                       pBDLE->State.u32BDLIndex, pBDLE->State.u32BufOff, pBDLE->State.cbBelowFIFOW,
                       pBDLE->Desc.fFlags & HDA_BDLE_FLAG_IOC, pBDLE->Desc.u32BufSize, pBDLE->Desc.u64BufAdr);
}

/**
 * @callback_method_impl{FNRTSTRFORMATTYPE}
 */
static DECLCALLBACK(size_t) hdaDbgFmtSDCTL(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                           const char *pszType, void const *pvValue,
                                           int cchWidth, int cchPrecision, unsigned fFlags,
                                           void *pvUser)
{
    RT_NOREF(pszType, cchWidth,  cchPrecision, fFlags, pvUser);
    uint32_t uSDCTL = (uint32_t)(uintptr_t)pvValue;
    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0,
                       "SDCTL(raw:%#x, DIR:%s, TP:%RTbool, STRIPE:%x, DEIE:%RTbool, FEIE:%RTbool, IOCE:%RTbool, RUN:%RTbool, RESET:%RTbool)",
                       uSDCTL,
                       uSDCTL & HDA_SDCTL_DIR ? "OUT" : "IN",
                       RT_BOOL(uSDCTL & HDA_SDCTL_TP),
                       (uSDCTL & HDA_SDCTL_STRIPE_MASK) >> HDA_SDCTL_STRIPE_SHIFT,
                       RT_BOOL(uSDCTL & HDA_SDCTL_DEIE),
                       RT_BOOL(uSDCTL & HDA_SDCTL_FEIE),
                       RT_BOOL(uSDCTL & HDA_SDCTL_IOCE),
                       RT_BOOL(uSDCTL & HDA_SDCTL_RUN),
                       RT_BOOL(uSDCTL & HDA_SDCTL_SRST));
}

/**
 * @callback_method_impl{FNRTSTRFORMATTYPE}
 */
static DECLCALLBACK(size_t) hdaDbgFmtSDFIFOS(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                             const char *pszType, void const *pvValue,
                                             int cchWidth, int cchPrecision, unsigned fFlags,
                                             void *pvUser)
{
    RT_NOREF(pszType, cchWidth,  cchPrecision, fFlags, pvUser);
    uint32_t uSDFIFOS = (uint32_t)(uintptr_t)pvValue;
    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "SDFIFOS(raw:%#x, sdfifos:%RU8 B)", uSDFIFOS, uSDFIFOS ? uSDFIFOS + 1 : 0);
}

/**
 * @callback_method_impl{FNRTSTRFORMATTYPE}
 */
static DECLCALLBACK(size_t) hdaDbgFmtSDFIFOW(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                             const char *pszType, void const *pvValue,
                                             int cchWidth, int cchPrecision, unsigned fFlags,
                                             void *pvUser)
{
    RT_NOREF(pszType, cchWidth,  cchPrecision, fFlags, pvUser);
    uint32_t uSDFIFOW = (uint32_t)(uintptr_t)pvValue;
    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "SDFIFOW(raw: %#0x, sdfifow:%d B)", uSDFIFOW, hdaSDFIFOWToBytes(uSDFIFOW));
}

/**
 * @callback_method_impl{FNRTSTRFORMATTYPE}
 */
static DECLCALLBACK(size_t) hdaDbgFmtSDSTS(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                           const char *pszType, void const *pvValue,
                                           int cchWidth, int cchPrecision, unsigned fFlags,
                                           void *pvUser)
{
    RT_NOREF(pszType, cchWidth,  cchPrecision, fFlags, pvUser);
    uint32_t uSdSts = (uint32_t)(uintptr_t)pvValue;
    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0,
                       "SDSTS(raw:%#0x, fifordy:%RTbool, dese:%RTbool, fifoe:%RTbool, bcis:%RTbool)",
                       uSdSts,
                       RT_BOOL(uSdSts & HDA_SDSTS_FIFORDY),
                       RT_BOOL(uSdSts & HDA_SDSTS_DESE),
                       RT_BOOL(uSdSts & HDA_SDSTS_FIFOE),
                       RT_BOOL(uSdSts & HDA_SDSTS_BCIS));
}

static int hdaDbgLookupRegByName(const char *pszArgs)
{
    int iReg = 0;
    for (; iReg < HDA_NUM_REGS; ++iReg)
        if (!RTStrICmp(g_aHdaRegMap[iReg].abbrev, pszArgs))
            return iReg;
    return -1;
}


static void hdaDbgPrintRegister(PHDASTATE pThis, PCDBGFINFOHLP pHlp, int iHdaIndex)
{
    Assert(   pThis
           && iHdaIndex >= 0
           && iHdaIndex < HDA_NUM_REGS);
    pHlp->pfnPrintf(pHlp, "%s: 0x%x\n", g_aHdaRegMap[iHdaIndex].abbrev, pThis->au32Regs[g_aHdaRegMap[iHdaIndex].mem_idx]);
}

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) hdaDbgInfo(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);
    int iHdaRegisterIndex = hdaDbgLookupRegByName(pszArgs);
    if (iHdaRegisterIndex != -1)
        hdaDbgPrintRegister(pThis, pHlp, iHdaRegisterIndex);
    else
    {
        for(iHdaRegisterIndex = 0; (unsigned int)iHdaRegisterIndex < HDA_NUM_REGS; ++iHdaRegisterIndex)
            hdaDbgPrintRegister(pThis, pHlp, iHdaRegisterIndex);
    }
}

static void hdaDbgPrintStream(PHDASTATE pThis, PCDBGFINFOHLP pHlp, int iIdx)
{
    Assert(   pThis
           && iIdx >= 0
           && iIdx < HDA_MAX_STREAMS);

    const PHDASTREAM pStrm = &pThis->aStreams[iIdx];

    pHlp->pfnPrintf(pHlp, "Stream #%d:\n", iIdx);
    pHlp->pfnPrintf(pHlp, "\tSD%dCTL  : %R[sdctl]\n",   iIdx, HDA_STREAM_REG(pThis, CTL,   iIdx));
    pHlp->pfnPrintf(pHlp, "\tSD%dCTS  : %R[sdsts]\n",   iIdx, HDA_STREAM_REG(pThis, STS,   iIdx));
    pHlp->pfnPrintf(pHlp, "\tSD%dFIFOS: %R[sdfifos]\n", iIdx, HDA_STREAM_REG(pThis, FIFOS, iIdx));
    pHlp->pfnPrintf(pHlp, "\tSD%dFIFOW: %R[sdfifow]\n", iIdx, HDA_STREAM_REG(pThis, FIFOW, iIdx));
    pHlp->pfnPrintf(pHlp, "\tBDLE     : %R[bdle]\n",    &pStrm->State.BDLE);
}

static void hdaDbgPrintBDLE(PHDASTATE pThis, PCDBGFINFOHLP pHlp, int iIdx)
{
    Assert(   pThis
           && iIdx >= 0
           && iIdx < HDA_MAX_STREAMS);

    const PHDASTREAM pStrm = &pThis->aStreams[iIdx];
    const PHDABDLE   pBDLE = &pStrm->State.BDLE;

    pHlp->pfnPrintf(pHlp, "Stream #%d BDLE:\n",      iIdx);

    uint64_t u64BaseDMA = RT_MAKE_U64(HDA_STREAM_REG(pThis, BDPL, iIdx),
                                      HDA_STREAM_REG(pThis, BDPU, iIdx));
    uint16_t u16LVI     = HDA_STREAM_REG(pThis, LVI, iIdx);
    uint32_t u32CBL     = HDA_STREAM_REG(pThis, CBL, iIdx);

    if (!u64BaseDMA)
        return;

    pHlp->pfnPrintf(pHlp, "\tCurrent: %R[bdle]\n\n", pBDLE);

    pHlp->pfnPrintf(pHlp, "\tMemory:\n");

    uint32_t cbBDLE = 0;
    for (uint16_t i = 0; i < u16LVI + 1; i++)
    {
        HDABDLEDESC bd;
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), u64BaseDMA + i * sizeof(HDABDLEDESC), &bd, sizeof(bd));

        pHlp->pfnPrintf(pHlp, "\t\t%s #%03d BDLE(adr:0x%llx, size:%RU32, ioc:%RTbool)\n",
                        pBDLE->State.u32BDLIndex == i ? "*" : " ", i, bd.u64BufAdr, bd.u32BufSize, bd.fFlags & HDA_BDLE_FLAG_IOC);

        cbBDLE += bd.u32BufSize;
    }

    pHlp->pfnPrintf(pHlp, "Total: %RU32 bytes\n", cbBDLE);

    if (cbBDLE != u32CBL)
        pHlp->pfnPrintf(pHlp, "Warning: %RU32 bytes does not match CBL (%RU32)!\n", cbBDLE, u32CBL);

    pHlp->pfnPrintf(pHlp, "DMA counters (base @ 0x%llx):\n", u64BaseDMA);
    if (!u64BaseDMA) /* No DMA base given? Bail out. */
    {
        pHlp->pfnPrintf(pHlp, "\tNo counters found\n");
        return;
    }

    for (int i = 0; i < u16LVI + 1; i++)
    {
        uint32_t uDMACnt;
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), (pThis->u64DPBase & DPBASE_ADDR_MASK) + (i * 2 * sizeof(uint32_t)),
                          &uDMACnt, sizeof(uDMACnt));

        pHlp->pfnPrintf(pHlp, "\t#%03d DMA @ 0x%x\n", i , uDMACnt);
    }
}

static int hdaDbgLookupStrmIdx(PHDASTATE pThis, const char *pszArgs)
{
    RT_NOREF(pThis, pszArgs);
    /** @todo Add args parsing. */
    return -1;
}

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) hdaDbgInfoStream(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PHDASTATE   pThis         = PDMINS_2_DATA(pDevIns, PHDASTATE);
    int         iHdaStreamdex = hdaDbgLookupStrmIdx(pThis, pszArgs);
    if (iHdaStreamdex != -1)
        hdaDbgPrintStream(pThis, pHlp, iHdaStreamdex);
    else
        for(iHdaStreamdex = 0; iHdaStreamdex < HDA_MAX_STREAMS; ++iHdaStreamdex)
            hdaDbgPrintStream(pThis, pHlp, iHdaStreamdex);
}

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) hdaDbgInfoBDLE(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PHDASTATE   pThis         = PDMINS_2_DATA(pDevIns, PHDASTATE);
    int         iHdaStreamdex = hdaDbgLookupStrmIdx(pThis, pszArgs);
    if (iHdaStreamdex != -1)
        hdaDbgPrintBDLE(pThis, pHlp, iHdaStreamdex);
    else
        for(iHdaStreamdex = 0; iHdaStreamdex < HDA_MAX_STREAMS; ++iHdaStreamdex)
            hdaDbgPrintBDLE(pThis, pHlp, iHdaStreamdex);
}

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) hdaDbgInfoCodecNodes(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    if (pThis->pCodec->pfnDbgListNodes)
        pThis->pCodec->pfnDbgListNodes(pThis->pCodec, pHlp, pszArgs);
    else
        pHlp->pfnPrintf(pHlp, "Codec implementation doesn't provide corresponding callback\n");
}

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) hdaDbgInfoCodecSelector(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    if (pThis->pCodec->pfnDbgSelector)
        pThis->pCodec->pfnDbgSelector(pThis->pCodec, pHlp, pszArgs);
    else
        pHlp->pfnPrintf(pHlp, "Codec implementation doesn't provide corresponding callback\n");
}

/**
 * @callback_method_impl{FNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) hdaDbgInfoMixer(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    if (pThis->pMixer)
        AudioMixerDebug(pThis->pMixer, pHlp, pszArgs);
    else
        pHlp->pfnPrintf(pHlp, "Mixer not available\n");
}


/* PDMIBASE */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) hdaQueryInterface(struct PDMIBASE *pInterface, const char *pszIID)
{
    PHDASTATE pThis = RT_FROM_MEMBER(pInterface, HDASTATE, IBase);
    Assert(&pThis->IBase == pInterface);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    return NULL;
}


/* PDMDEVREG */


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) hdaReset(PPDMDEVINS pDevIns)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    LogFlowFuncEnter();

    DEVHDA_LOCK_RETURN_VOID(pThis);

     /*
     * 18.2.6,7 defines that values of this registers might be cleared on power on/reset
     * hdaReset shouldn't affects these registers.
     */
    HDA_REG(pThis, WAKEEN)  = 0x0;

    hdaGCTLReset(pThis);

    /* Indicate that HDA is not in reset. The firmware is supposed to (un)reset HDA,
     * but we can take a shortcut.
     */
    HDA_REG(pThis, GCTL)    = HDA_GCTL_CRST;

    DEVHDA_UNLOCK(pThis);
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) hdaDestruct(PPDMDEVINS pDevIns)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    PHDADRIVER pDrv;
    while (!RTListIsEmpty(&pThis->lstDrv))
    {
        pDrv = RTListGetFirst(&pThis->lstDrv, HDADRIVER, Node);

        RTListNodeRemove(&pDrv->Node);
        RTMemFree(pDrv);
    }

    if (pThis->pCodec)
    {
        hdaCodecDestruct(pThis->pCodec);

        RTMemFree(pThis->pCodec);
        pThis->pCodec = NULL;
    }

    RTMemFree(pThis->pu32CorbBuf);
    pThis->pu32CorbBuf = NULL;

    RTMemFree(pThis->pu64RirbBuf);
    pThis->pu64RirbBuf = NULL;

    for (uint8_t i = 0; i < HDA_MAX_STREAMS; i++)
        hdaStreamDestroy(&pThis->aStreams[i]);

    DEVHDA_UNLOCK(pThis);

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
 * @param   pThis       HDA state.
 * @param   uLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 * @param   ppDrv       Attached driver instance on success. Optional.
 */
static int hdaAttachInternal(PHDASTATE pThis, unsigned uLUN, uint32_t fFlags, PHDADRIVER *ppDrv)
{
    RT_NOREF(fFlags);

    /*
     * Attach driver.
     */
    char *pszDesc;
    if (RTStrAPrintf(&pszDesc, "Audio driver port (HDA) for LUN#%u", uLUN) <= 0)
        AssertLogRelFailedReturn(VERR_NO_MEMORY);

    PPDMIBASE pDrvBase;
    int rc = PDMDevHlpDriverAttach(pThis->pDevInsR3, uLUN,
                                   &pThis->IBase, &pDrvBase, pszDesc);
    if (RT_SUCCESS(rc))
    {
        PHDADRIVER pDrv = (PHDADRIVER)RTMemAllocZ(sizeof(HDADRIVER));
        if (pDrv)
        {
            pDrv->pDrvBase   = pDrvBase;
            pDrv->pConnector = PDMIBASE_QUERY_INTERFACE(pDrvBase, PDMIAUDIOCONNECTOR);
            AssertMsg(pDrv->pConnector != NULL, ("Configuration error: LUN#%u has no host audio interface, rc=%Rrc\n", uLUN, rc));
            pDrv->pHDAState  = pThis;
            pDrv->uLUN       = uLUN;

            /*
             * For now we always set the driver at LUN 0 as our primary
             * host backend. This might change in the future.
             */
            if (pDrv->uLUN == 0)
                pDrv->fFlags |= PDMAUDIODRVFLAGS_PRIMARY;

            LogFunc(("LUN#%u: pCon=%p, drvFlags=0x%x\n", uLUN, pDrv->pConnector, pDrv->fFlags));

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
 * @param   pThis       HDA state.
 * @param   pDrv        Driver to detach device from.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
static int hdaDetachInternal(PHDASTATE pThis, PHDADRIVER pDrv, uint32_t fFlags)
{
    RT_NOREF(fFlags);

    AudioMixerSinkRemoveStream(pThis->SinkFront.pMixSink,     pDrv->Front.pMixStrm);
    AudioMixerStreamDestroy(pDrv->Front.pMixStrm);
    pDrv->Front.pMixStrm = NULL;

#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    AudioMixerSinkRemoveStream(pThis->SinkCenterLFE.pMixSink, pDrv->CenterLFE.pMixStrm);
    AudioMixerStreamDestroy(pDrv->CenterLFE.pMixStrm);
    pDrv->CenterLFE.pMixStrm = NULL;

    AudioMixerSinkRemoveStream(pThis->SinkRear.pMixSink,      pDrv->Rear.pMixStrm);
    AudioMixerStreamDestroy(pDrv->Rear.pMixStrm);
    pDrv->Rear.pMixStrm = NULL;
#endif

    AudioMixerSinkRemoveStream(pThis->SinkLineIn.pMixSink,    pDrv->LineIn.pMixStrm);
    AudioMixerStreamDestroy(pDrv->LineIn.pMixStrm);
    pDrv->LineIn.pMixStrm = NULL;

#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    AudioMixerSinkRemoveStream(pThis->SinkMicIn.pMixSink,     pDrv->MicIn.pMixStrm);
    AudioMixerStreamDestroy(pDrv->MicIn.pMixStrm);
    pDrv->MicIn.pMixStrm = NULL;
#endif

    RTListNodeRemove(&pDrv->Node);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", pDrv->uLUN, fFlags));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 */
static DECLCALLBACK(int) hdaAttach(PPDMDEVINS pDevIns, unsigned uLUN, uint32_t fFlags)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    DEVHDA_LOCK_RETURN(pThis, VINF_IOM_R3_MMIO_WRITE);

    PHDADRIVER pDrv;
    int rc2 = hdaAttachInternal(pThis, uLUN, fFlags, &pDrv);
    if (RT_SUCCESS(rc2))
    {
        PHDASTREAM pStream = hdaGetStreamFromSink(pThis, &pThis->SinkFront);
        if (DrvAudioHlpStreamCfgIsValid(&pStream->State.Cfg))
            hdaMixerAddDrvStream(pThis, pThis->SinkFront.pMixSink,     &pStream->State.Cfg, pDrv);

#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
        pStream = hdaGetStreamFromSink(pThis, &pThis->SinkCenterLFE);
        if (DrvAudioHlpStreamCfgIsValid(&pStream->State.Cfg))
            hdaMixerAddDrvStream(pThis, pThis->SinkCenterLFE.pMixSink, &pStream->State.Cfg, pDrv);

        pStream = hdaGetStreamFromSink(pThis, &pThis->SinkRear);
        if (DrvAudioHlpStreamCfgIsValid(&pStream->State.Cfg))
            hdaMixerAddDrvStream(pThis, pThis->SinkRear.pMixSink,      &pStream->State.Cfg, pDrv);
#endif
        pStream = hdaGetStreamFromSink(pThis, &pThis->SinkLineIn);
        if (DrvAudioHlpStreamCfgIsValid(&pStream->State.Cfg))
            hdaMixerAddDrvStream(pThis, pThis->SinkLineIn.pMixSink,    &pStream->State.Cfg, pDrv);

#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
        pStream = hdaGetStreamFromSink(pThis, &pThis->SinkMicIn);
        if (DrvAudioHlpStreamCfgIsValid(&pStream->State.Cfg))
            hdaMixerAddDrvStream(pThis, pThis->SinkMicIn.pMixSink,     &pStream->State.Cfg, pDrv);
#endif
    }

    DEVHDA_UNLOCK(pThis);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 */
static DECLCALLBACK(void) hdaDetach(PPDMDEVINS pDevIns, unsigned uLUN, uint32_t fFlags)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    LogFunc(("uLUN=%u, fFlags=0x%x\n", uLUN, fFlags));

    DEVHDA_LOCK(pThis);

    PHDADRIVER pDrv, pDrvNext;
    RTListForEachSafe(&pThis->lstDrv, pDrv, pDrvNext, HDADRIVER, Node)
    {
        if (pDrv->uLUN == uLUN)
        {
            int rc2 = hdaDetachInternal(pThis, pDrv, fFlags);
            if (RT_SUCCESS(rc2))
            {
                RTMemFree(pDrv);
                pDrv = NULL;
            }

            break;
        }
    }

    DEVHDA_UNLOCK(pThis);
}

/**
 * Re-attaches (replaces) a driver with a new driver.
 *
 * @returns VBox status code.
 * @param   pThis       Device instance to re-attach driver to.
 * @param   pDrv        Driver instance used for attaching to.
 *                      If NULL is specified, a new driver will be created and appended
 *                      to the driver list.
 * @param   uLUN        The logical unit which is being re-detached.
 * @param   pszDriver   New driver name to attach.
 */
static int hdaReattachInternal(PHDASTATE pThis, PHDADRIVER pDrv, uint8_t uLUN, const char *pszDriver)
{
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pszDriver, VERR_INVALID_POINTER);

    int rc;

    if (pDrv)
    {
        rc = hdaDetachInternal(pThis, pDrv, 0 /* fFlags */);
        if (RT_SUCCESS(rc))
            rc = PDMDevHlpDriverDetach(pThis->pDevInsR3, PDMIBASE_2_PDMDRV(pDrv->pDrvBase), 0 /* fFlags */);

        if (RT_FAILURE(rc))
            return rc;

        pDrv = NULL;
    }

    PVM pVM = PDMDevHlpGetVM(pThis->pDevInsR3);
    PCFGMNODE pRoot = CFGMR3GetRoot(pVM);
    PCFGMNODE pDev0 = CFGMR3GetChild(pRoot, "Devices/hda/0/");

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
        rc = hdaAttachInternal(pThis, uLUN, 0 /* fFlags */, NULL /* ppDrv */);

    LogFunc(("pThis=%p, uLUN=%u, pszDriver=%s, rc=%Rrc\n", pThis, uLUN, pszDriver, rc));

#undef RC_CHECK

    return rc;
}

/**
 * Powers off the device.
 *
 * @param   pDevIns             Device instance to power off.
 */
static DECLCALLBACK(void) hdaPowerOff(PPDMDEVINS pDevIns)
{
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);

    DEVHDA_LOCK_RETURN_VOID(pThis);

    LogRel2(("HDA: Powering off ...\n"));

    /* Ditto goes for the codec, which in turn uses the mixer. */
    hdaCodecPowerOff(pThis->pCodec);

    /**
     * Note: Destroy the mixer while powering off and *not* in hdaDestruct,
     *       giving the mixer the chance to release any references held to
     *       PDM audio streams it maintains.
     */
    if (pThis->pMixer)
    {
        AudioMixerDestroy(pThis->pMixer);
        pThis->pMixer = NULL;
    }

    DEVHDA_UNLOCK(pThis);
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) hdaConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    PHDASTATE pThis = PDMINS_2_DATA(pDevIns, PHDASTATE);
    Assert(iInstance == 0);

    /*
     * Validations.
     */
    if (!CFGMR3AreValuesValid(pCfg, "R0Enabled\0"
                                    "RCEnabled\0"
                                    "TimerHz\0"))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_ ("Invalid configuration for the Intel HDA device"));

    int rc = CFGMR3QueryBoolDef(pCfg, "RCEnabled", &pThis->fRCEnabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("HDA configuration error: failed to read RCEnabled as boolean"));
    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &pThis->fR0Enabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("HDA configuration error: failed to read R0Enabled as boolean"));
#ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
    uint16_t uTimerHz;
    rc = CFGMR3QueryU16Def(pCfg, "TimerHz", &uTimerHz, HDA_TIMER_HZ /* Default value, if not set. */);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("HDA configuration error: failed to read Hertz (Hz) rate as unsigned integer"));
#endif

    /*
     * Use an own critical section for the device instead of the default
     * one provided by PDM. This allows fine-grained locking in combination
     * with TM when timer-specific stuff is being called in e.g. the MMIO handlers.
     */
    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->CritSect, RT_SRC_POS, "HDA");
    AssertRCReturn(rc, rc);

    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /*
     * Initialize data (most of it anyway).
     */
    pThis->pDevInsR3                = pDevIns;
    pThis->pDevInsR0                = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC                = PDMDEVINS_2_RCPTR(pDevIns);
    /* IBase */
    pThis->IBase.pfnQueryInterface  = hdaQueryInterface;

    /* PCI Device */
    PCIDevSetVendorId           (&pThis->PciDev, HDA_PCI_VENDOR_ID); /* nVidia */
    PCIDevSetDeviceId           (&pThis->PciDev, HDA_PCI_DEVICE_ID); /* HDA */

    PCIDevSetCommand            (&pThis->PciDev, 0x0000); /* 04 rw,ro - pcicmd. */
    PCIDevSetStatus             (&pThis->PciDev, VBOX_PCI_STATUS_CAP_LIST); /* 06 rwc?,ro? - pcists. */
    PCIDevSetRevisionId         (&pThis->PciDev, 0x01);   /* 08 ro - rid. */
    PCIDevSetClassProg          (&pThis->PciDev, 0x00);   /* 09 ro - pi. */
    PCIDevSetClassSub           (&pThis->PciDev, 0x03);   /* 0a ro - scc; 03 == HDA. */
    PCIDevSetClassBase          (&pThis->PciDev, 0x04);   /* 0b ro - bcc; 04 == multimedia. */
    PCIDevSetHeaderType         (&pThis->PciDev, 0x00);   /* 0e ro - headtyp. */
    PCIDevSetBaseAddress        (&pThis->PciDev, 0,       /* 10 rw - MMIO */
                                 false /* fIoSpace */, false /* fPrefetchable */, true /* f64Bit */, 0x00000000);
    PCIDevSetInterruptLine      (&pThis->PciDev, 0x00);   /* 3c rw. */
    PCIDevSetInterruptPin       (&pThis->PciDev, 0x01);   /* 3d ro - INTA#. */

#if defined(HDA_AS_PCI_EXPRESS)
    PCIDevSetCapabilityList     (&pThis->PciDev, 0x80);
#elif defined(VBOX_WITH_MSI_DEVICES)
    PCIDevSetCapabilityList     (&pThis->PciDev, 0x60);
#else
    PCIDevSetCapabilityList     (&pThis->PciDev, 0x50);   /* ICH6 datasheet 18.1.16 */
#endif

    /// @todo r=michaln: If there are really no PCIDevSetXx for these, the meaning
    /// of these values needs to be properly documented!
    /* HDCTL off 0x40 bit 0 selects signaling mode (1-HDA, 0 - Ac97) 18.1.19 */
    PCIDevSetByte(&pThis->PciDev, 0x40, 0x01);

    /* Power Management */
    PCIDevSetByte(&pThis->PciDev, 0x50 + 0, VBOX_PCI_CAP_ID_PM);
    PCIDevSetByte(&pThis->PciDev, 0x50 + 1, 0x0); /* next */
    PCIDevSetWord(&pThis->PciDev, 0x50 + 2, VBOX_PCI_PM_CAP_DSI | 0x02 /* version, PM1.1 */ );

#ifdef HDA_AS_PCI_EXPRESS
    /* PCI Express */
    PCIDevSetByte(&pThis->PciDev, 0x80 + 0, VBOX_PCI_CAP_ID_EXP); /* PCI_Express */
    PCIDevSetByte(&pThis->PciDev, 0x80 + 1, 0x60); /* next */
    /* Device flags */
    PCIDevSetWord(&pThis->PciDev, 0x80 + 2,
                   /* version */ 0x1     |
                   /* Root Complex Integrated Endpoint */ (VBOX_PCI_EXP_TYPE_ROOT_INT_EP << 4) |
                   /* MSI */ (100) << 9 );
    /* Device capabilities */
    PCIDevSetDWord(&pThis->PciDev, 0x80 + 4, VBOX_PCI_EXP_DEVCAP_FLRESET);
    /* Device control */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 8, 0);
    /* Device status */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 10, 0);
    /* Link caps */
    PCIDevSetDWord(&pThis->PciDev, 0x80 + 12, 0);
    /* Link control */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 16, 0);
    /* Link status */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 18, 0);
    /* Slot capabilities */
    PCIDevSetDWord(&pThis->PciDev, 0x80 + 20, 0);
    /* Slot control */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 24, 0);
    /* Slot status */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 26, 0);
    /* Root control */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 28, 0);
    /* Root capabilities */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 30, 0);
    /* Root status */
    PCIDevSetDWord(&pThis->PciDev, 0x80 + 32, 0);
    /* Device capabilities 2 */
    PCIDevSetDWord(&pThis->PciDev, 0x80 + 36, 0);
    /* Device control 2 */
    PCIDevSetQWord(&pThis->PciDev, 0x80 + 40, 0);
    /* Link control 2 */
    PCIDevSetQWord(&pThis->PciDev, 0x80 + 48, 0);
    /* Slot control 2 */
    PCIDevSetWord( &pThis->PciDev, 0x80 + 56, 0);
#endif

    /*
     * Register the PCI device.
     */
    rc = PDMDevHlpPCIRegister(pDevIns, &pThis->PciDev);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0, 0x4000, PCI_ADDRESS_SPACE_MEM, hdaPciIoRegionMap);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_MSI_DEVICES
    PDMMSIREG MsiReg;
    RT_ZERO(MsiReg);
    MsiReg.cMsiVectors    = 1;
    MsiReg.iMsiCapOffset  = 0x60;
    MsiReg.iMsiNextOffset = 0x50;
    rc = PDMDevHlpPCIRegisterMsi(pDevIns, &MsiReg);
    if (RT_FAILURE(rc))
    {
        /* That's OK, we can work without MSI */
        PCIDevSetCapabilityList(&pThis->PciDev, 0x50);
    }
#endif

    rc = PDMDevHlpSSMRegister(pDevIns, HDA_SSM_VERSION, sizeof(*pThis), hdaSaveExec, hdaLoadExec);
    if (RT_FAILURE(rc))
        return rc;

    RTListInit(&pThis->lstDrv);

#ifdef VBOX_WITH_AUDIO_HDA_ASYNC_IO
    LogRel(("HDA: Asynchronous I/O enabled\n"));
#endif

    uint8_t uLUN;
    for (uLUN = 0; uLUN < UINT8_MAX; ++uLUN)
    {
        LogFunc(("Trying to attach driver for LUN #%RU32 ...\n", uLUN));
        rc = hdaAttachInternal(pThis, uLUN, 0 /* fFlags */, NULL /* ppDrv */);
        if (RT_FAILURE(rc))
        {
            if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
                rc = VINF_SUCCESS;
            else if (rc == VERR_AUDIO_BACKEND_INIT_FAILED)
            {
                hdaReattachInternal(pThis, NULL /* pDrv */, uLUN, "NullAudio");
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
        rc = AudioMixerCreate("HDA Mixer", 0 /* uFlags */, &pThis->pMixer);
        if (RT_SUCCESS(rc))
        {
            /*
             * Add mixer output sinks.
             */
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
            rc = AudioMixerCreateSink(pThis->pMixer, "[Playback] Front",
                                      AUDMIXSINKDIR_OUTPUT, &pThis->SinkFront.pMixSink);
            AssertRC(rc);
            rc = AudioMixerCreateSink(pThis->pMixer, "[Playback] Center / Subwoofer",
                                      AUDMIXSINKDIR_OUTPUT, &pThis->SinkCenterLFE.pMixSink);
            AssertRC(rc);
            rc = AudioMixerCreateSink(pThis->pMixer, "[Playback] Rear",
                                      AUDMIXSINKDIR_OUTPUT, &pThis->SinkRear.pMixSink);
            AssertRC(rc);
#else
            rc = AudioMixerCreateSink(pThis->pMixer, "[Playback] PCM Output",
                                      AUDMIXSINKDIR_OUTPUT, &pThis->SinkFront.pMixSink);
            AssertRC(rc);
#endif
            /*
             * Add mixer input sinks.
             */
            rc = AudioMixerCreateSink(pThis->pMixer, "[Recording] Line In",
                                      AUDMIXSINKDIR_INPUT, &pThis->SinkLineIn.pMixSink);
            AssertRC(rc);
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
            rc = AudioMixerCreateSink(pThis->pMixer, "[Recording] Microphone In",
                                      AUDMIXSINKDIR_INPUT, &pThis->SinkMicIn.pMixSink);
            AssertRC(rc);
#endif
            /* There is no master volume control. Set the master to max. */
            PDMAUDIOVOLUME vol = { false, 255, 255 };
            rc = AudioMixerSetMasterVolume(pThis->pMixer, &vol);
            AssertRC(rc);
        }
    }

    if (RT_SUCCESS(rc))
    {
        /* Construct codec. */
        pThis->pCodec = (PHDACODEC)RTMemAllocZ(sizeof(HDACODEC));
        if (!pThis->pCodec)
            return PDMDEV_SET_ERROR(pDevIns, VERR_NO_MEMORY, N_("Out of memory allocating HDA codec state"));

        /* Set codec callbacks to this controller. */
        pThis->pCodec->pfnCbMixerAddStream    = hdaMixerAddStream;
        pThis->pCodec->pfnCbMixerRemoveStream = hdaMixerRemoveStream;
        pThis->pCodec->pfnCbMixerSetStream    = hdaMixerSetStream;
        pThis->pCodec->pfnCbMixerSetVolume    = hdaMixerSetVolume;

        pThis->pCodec->pHDAState = pThis; /* Assign HDA controller state to codec. */

        /* Construct the codec. */
        rc = hdaCodecConstruct(pDevIns, pThis->pCodec, 0 /* Codec index */, pCfg);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);

        /* ICH6 datasheet defines 0 values for SVID and SID (18.1.14-15), which together with values returned for
           verb F20 should provide device/codec recognition. */
        Assert(pThis->pCodec->u16VendorId);
        Assert(pThis->pCodec->u16DeviceId);
        PCIDevSetSubSystemVendorId(&pThis->PciDev, pThis->pCodec->u16VendorId); /* 2c ro - intel.) */
        PCIDevSetSubSystemId(      &pThis->PciDev, pThis->pCodec->u16DeviceId); /* 2e ro. */
    }

    if (RT_SUCCESS(rc))
    {
        /*
         * Create all hardware streams.
         */
        for (uint8_t i = 0; i < HDA_MAX_STREAMS; ++i)
        {
            rc = hdaStreamCreate(&pThis->aStreams[i], pThis);
            AssertRC(rc);
        }

#ifdef VBOX_WITH_AUDIO_HDA_ONETIME_INIT
        /*
         * Initialize the driver chain.
         */
        PHDADRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, HDADRIVER, Node)
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
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
            bool fValidMicIn  = AudioMixerStreamIsValid(pDrv->MicIn.pMixStrm);
# endif
            bool fValidOut    = AudioMixerStreamIsValid(pDrv->Front.pMixStrm);
# ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
            /** @todo Anything to do here? */
# endif

            if (    !fValidLineIn
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
                 && !fValidMicIn
# endif
                 && !fValidOut)
            {
                LogRel(("HDA: Falling back to NULL backend (no sound audible)\n"));

                hdaReset(pDevIns);
                hdaReattachInternal(pThis, pDrv, pDrv->uLUN, "NullAudio");

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
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
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
# else /* !VBOX_WITH_AUDIO_HDA_MIC_IN */
                        /* We only have line-in as input source. */
                        fWarn = !fValidLineIn;
# endif /* VBOX_WITH_AUDIO_HDA_MIC_IN */
                    }

                    if (   !fWarn
                        && backendCfg.cMaxStreamsOut)
                    {
                        fWarn = !fValidOut;
                    }
                }
                else
                {
                    LogRel(("HDA: Unable to retrieve audio backend configuration for LUN #%RU8, rc=%Rrc\n", pDrv->uLUN, rc2));
                    fWarn = true;
                }

                if (fWarn)
                {
                    char   szMissingStreams[255];
                    size_t len = 0;
                    if (!fValidLineIn)
                    {
                        LogRel(("HDA: WARNING: Unable to open PCM line input for LUN #%RU8!\n", pDrv->uLUN));
                        len = RTStrPrintf(szMissingStreams, sizeof(szMissingStreams), "PCM Input");
                    }
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
                    if (!fValidMicIn)
                    {
                        LogRel(("HDA: WARNING: Unable to open PCM microphone input for LUN #%RU8!\n", pDrv->uLUN));
                        len += RTStrPrintf(szMissingStreams + len,
                                           sizeof(szMissingStreams) - len, len ? ", PCM Microphone" : "PCM Microphone");
                    }
# endif /* VBOX_WITH_AUDIO_HDA_MIC_IN */
                    if (!fValidOut)
                    {
                        LogRel(("HDA: WARNING: Unable to open PCM output for LUN #%RU8!\n", pDrv->uLUN));
                        len += RTStrPrintf(szMissingStreams + len,
                                           sizeof(szMissingStreams) - len, len ? ", PCM Output" : "PCM Output");
                    }

                    PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "HostAudioNotResponding",
                                               N_("Some HDA audio streams (%s) could not be opened. Guest applications generating audio "
                                                  "output or depending on audio input may hang. Make sure your host audio device "
                                                  "is working properly. Check the logfile for error messages of the audio "
                                                  "subsystem"), szMissingStreams);
                }
            }
        }
#endif /* VBOX_WITH_AUDIO_HDA_ONETIME_INIT */
    }

    if (RT_SUCCESS(rc))
    {
        hdaReset(pDevIns);

        /*
         * Debug and string formatter types.
         */
        PDMDevHlpDBGFInfoRegister(pDevIns, "hda",         "HDA info. (hda [register case-insensitive])",     hdaDbgInfo);
        PDMDevHlpDBGFInfoRegister(pDevIns, "hdabdle",     "HDA stream BDLE info. (hdabdle [stream number])", hdaDbgInfoBDLE);
        PDMDevHlpDBGFInfoRegister(pDevIns, "hdastream",   "HDA stream info. (hdastream [stream number])",    hdaDbgInfoStream);
        PDMDevHlpDBGFInfoRegister(pDevIns, "hdcnodes",    "HDA codec nodes.",                                hdaDbgInfoCodecNodes);
        PDMDevHlpDBGFInfoRegister(pDevIns, "hdcselector", "HDA codec's selector states [node number].",      hdaDbgInfoCodecSelector);
        PDMDevHlpDBGFInfoRegister(pDevIns, "hdamixer",    "HDA mixer state.",                                hdaDbgInfoMixer);

        rc = RTStrFormatTypeRegister("bdle",    hdaDbgFmtBDLE,    NULL);
        AssertRC(rc);
        rc = RTStrFormatTypeRegister("sdctl",   hdaDbgFmtSDCTL,   NULL);
        AssertRC(rc);
        rc = RTStrFormatTypeRegister("sdsts",   hdaDbgFmtSDSTS,   NULL);
        AssertRC(rc);
        rc = RTStrFormatTypeRegister("sdfifos", hdaDbgFmtSDFIFOS, NULL);
        AssertRC(rc);
        rc = RTStrFormatTypeRegister("sdfifow", hdaDbgFmtSDFIFOW, NULL);
        AssertRC(rc);

        /*
         * Some debug assertions.
         */
        for (unsigned i = 0; i < RT_ELEMENTS(g_aHdaRegMap); i++)
        {
            struct HDAREGDESC const *pReg     = &g_aHdaRegMap[i];
            struct HDAREGDESC const *pNextReg = i + 1 < RT_ELEMENTS(g_aHdaRegMap) ?  &g_aHdaRegMap[i + 1] : NULL;

            /* binary search order. */
            AssertReleaseMsg(!pNextReg || pReg->offset + pReg->size <= pNextReg->offset,
                             ("[%#x] = {%#x LB %#x}  vs. [%#x] = {%#x LB %#x}\n",
                              i, pReg->offset, pReg->size, i + 1, pNextReg->offset, pNextReg->size));

            /* alignment. */
            AssertReleaseMsg(   pReg->size == 1
                             || (pReg->size == 2 && (pReg->offset & 1) == 0)
                             || (pReg->size == 3 && (pReg->offset & 3) == 0)
                             || (pReg->size == 4 && (pReg->offset & 3) == 0),
                             ("[%#x] = {%#x LB %#x}\n", i, pReg->offset, pReg->size));

            /* registers are packed into dwords - with 3 exceptions with gaps at the end of the dword. */
            AssertRelease(((pReg->offset + pReg->size) & 3) == 0 || pNextReg);
            if (pReg->offset & 3)
            {
                struct HDAREGDESC const *pPrevReg = i > 0 ?  &g_aHdaRegMap[i - 1] : NULL;
                AssertReleaseMsg(pPrevReg, ("[%#x] = {%#x LB %#x}\n", i, pReg->offset, pReg->size));
                if (pPrevReg)
                    AssertReleaseMsg(pPrevReg->offset + pPrevReg->size == pReg->offset,
                                     ("[%#x] = {%#x LB %#x}  vs. [%#x] = {%#x LB %#x}\n",
                                      i - 1, pPrevReg->offset, pPrevReg->size, i + 1, pReg->offset, pReg->size));
            }
#if 0
            if ((pReg->offset + pReg->size) & 3)
            {
                AssertReleaseMsg(pNextReg, ("[%#x] = {%#x LB %#x}\n", i, pReg->offset, pReg->size));
                if (pNextReg)
                    AssertReleaseMsg(pReg->offset + pReg->size == pNextReg->offset,
                                     ("[%#x] = {%#x LB %#x}  vs. [%#x] = {%#x LB %#x}\n",
                                      i, pReg->offset, pReg->size, i + 1,  pNextReg->offset, pNextReg->size));
            }
#endif
            /* The final entry is a full DWORD, no gaps! Allows shortcuts. */
            AssertReleaseMsg(pNextReg || ((pReg->offset + pReg->size) & 3) == 0,
                             ("[%#x] = {%#x LB %#x}\n", i, pReg->offset, pReg->size));
        }
    }

# ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
    if (RT_SUCCESS(rc))
    {
        /* Create the emulation timer.
         *
         * Note:  Use TMCLOCK_VIRTUAL_SYNC here, as the guest's HDA driver
         *        relies on exact (virtual) DMA timing and uses DMA Position Buffers
         *        instead of the LPIB registers.
         */
        rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL_SYNC, hdaTimer, pThis,
                                    TMTIMER_FLAGS_NO_CRIT_SECT, "HDA Timer", &pThis->pTimer);
        AssertRCReturn(rc, rc);

        /* Use our own critcal section for the device timer.
         * That way we can control more fine-grained when to lock what. */
        rc = TMR3TimerSetCritSect(pThis->pTimer, &pThis->CritSect);
        AssertRCReturn(rc, rc);

        pThis->cTimerTicks = TMTimerGetFreq(pThis->pTimer) / uTimerHz;
        LogFunc(("Timer ticks=%RU64 (%RU16 Hz)\n", pThis->cTimerTicks, uTimerHz));
    }
# else
    if (RT_SUCCESS(rc))
    {
        PHDADRIVER pDrv;
        RTListForEach(&pThis->lstDrv, pDrv, HDADRIVER, Node)
        {
            /* Only register primary driver.
             * The device emulation does the output multiplexing then. */
            if (pDrv->fFlags != PDMAUDIODRVFLAGS_PRIMARY)
                continue;

            PDMAUDIOCBRECORD AudioCallbacks[2];

            HDACALLBACKCTX Ctx = { pThis, pDrv };

            AudioCallbacks[0].enmType     = PDMAUDIOCALLBACKTYPE_INPUT;
            AudioCallbacks[0].pfnCallback = hdaCallbackInput;
            AudioCallbacks[0].pvCtx       = &Ctx;
            AudioCallbacks[0].cbCtx       = sizeof(HDACALLBACKCTX);

            AudioCallbacks[1].enmType     = PDMAUDIOCALLBACKTYPE_OUTPUT;
            AudioCallbacks[1].pfnCallback = hdaCallbackOutput;
            AudioCallbacks[1].pvCtx       = &Ctx;
            AudioCallbacks[1].cbCtx       = sizeof(HDACALLBACKCTX);

            rc = pDrv->pConnector->pfnRegisterCallbacks(pDrv->pConnector, AudioCallbacks, RT_ELEMENTS(AudioCallbacks));
            if (RT_FAILURE(rc))
                break;
        }
    }
# endif

# ifdef VBOX_WITH_STATISTICS
    if (RT_SUCCESS(rc))
    {
        /*
         * Register statistics.
         */
#  ifndef VBOX_WITH_AUDIO_HDA_CALLBACKS
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatTimer,            STAMTYPE_PROFILE, "/Devices/HDA/Timer",             STAMUNIT_TICKS_PER_CALL, "Profiling hdaTimer.");
#  endif
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatIn,               STAMTYPE_PROFILE, "/Devices/HDA/Input",             STAMUNIT_TICKS_PER_CALL, "Profiling input.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatOut,              STAMTYPE_PROFILE, "/Devices/HDA/Output",            STAMUNIT_TICKS_PER_CALL, "Profiling output.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatBytesRead,        STAMTYPE_COUNTER, "/Devices/HDA/BytesRead"   ,      STAMUNIT_BYTES,          "Bytes read from HDA emulation.");
        PDMDevHlpSTAMRegister(pDevIns, &pThis->StatBytesWritten,     STAMTYPE_COUNTER, "/Devices/HDA/BytesWritten",      STAMUNIT_BYTES,          "Bytes written to HDA emulation.");
    }
# endif

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaDMARead.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaDMAWrite.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaStreamRead.pcm");
    RTFileDelete(VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "hdaStreamWrite.pcm");
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceHDA =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "hda",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Intel HD Audio Controller",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_AUDIO,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(HDASTATE),
    /* pfnConstruct */
    hdaConstruct,
    /* pfnDestruct */
    hdaDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    hdaReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    hdaAttach,
    /* pfnDetach */
    hdaDetach,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    hdaPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

#endif /* IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */

