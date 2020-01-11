/* $Id: DevHDACommon.h $ */
/** @file
 * DevHDACommon.h - Shared HDA device defines / functions.
 */

/*
 * Copyright (C) 2016-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_Audio_DevHDACommon_h
#define VBOX_INCLUDED_SRC_Audio_DevHDACommon_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "AudioMixer.h"
#include <VBox/log.h> /* LOG_ENABLED */

/** See 302349 p 6.2. */
typedef struct HDAREGDESC
{
    /** Register offset in the register space. */
    uint32_t    offset;
    /** Size in bytes. Registers of size > 4 are in fact tables. */
    uint32_t    size;
    /** Readable bits. */
    uint32_t    readable;
    /** Writable bits. */
    uint32_t    writable;
    /** Register descriptor (RD) flags of type HDA_RD_FLAG_.
     *  These are used to specify the handling (read/write)
     *  policy of the register. */
    uint32_t    fFlags;
    /** Read callback. */
    int       (*pfnRead)(PHDASTATE pThis, uint32_t iReg, uint32_t *pu32Value);
    /** Write callback. */
    int       (*pfnWrite)(PHDASTATE pThis, uint32_t iReg, uint32_t u32Value);
    /** Index into the register storage array. */
    uint32_t    mem_idx;
    /** Abbreviated name. */
    const char *abbrev;
    /** Descripton. */
    const char *desc;
} HDAREGDESC, *PHDAREGDESC;

/**
 * HDA register aliases (HDA spec 3.3.45).
 * @remarks Sorted by offReg.
 */
typedef struct HDAREGALIAS
{
    /** The alias register offset. */
    uint32_t    offReg;
    /** The register index. */
    int         idxAlias;
} HDAREGALIAS, *PHDAREGALIAS;

/**
 * At the moment we support 4 input + 4 output streams max, which is 8 in total.
 * Bidirectional streams are currently *not* supported.
 *
 * Note: When changing any of those values, be prepared for some saved state
 *       fixups / trouble!
 */
#define HDA_MAX_SDI                 4
#define HDA_MAX_SDO                 4
#define HDA_MAX_STREAMS             (HDA_MAX_SDI + HDA_MAX_SDO)
AssertCompile(HDA_MAX_SDI <= HDA_MAX_SDO);

/** Number of general registers. */
#define HDA_NUM_GENERAL_REGS        34
/** Number of total registers in the HDA's register map. */
#define HDA_NUM_REGS                (HDA_NUM_GENERAL_REGS + (HDA_MAX_STREAMS * 10 /* Each stream descriptor has 10 registers */))
/** Total number of stream tags (channels). Index 0 is reserved / invalid. */
#define HDA_MAX_TAGS                16

/**
 * ICH6 datasheet defines limits for FIFOS registers (18.2.39).
 * Formula: size - 1
 * Other values not listed are not supported.
 */
/** Maximum FIFO size (in bytes). */
#define HDA_FIFO_MAX                256

/** Default timer frequency (in Hz).
 *
 * Lowering this value can ask for trouble, as backends then can run
 * into data underruns.
 *
 * Note: For handling surround setups (e.g. 5.1 speaker setups) we need
 *       a higher Hz rate, as the device emulation otherwise will come into
 *       timing trouble, making the output (DMA reads) crackling. */
#define HDA_TIMER_HZ_DEFAULT        100

/** Default position adjustment (in audio samples).
 *
 * For snd_hda_intel (Linux guests), the first BDL entry always is being used as
 * so-called BDL adjustment, which can vary, and is being used for chipsets which
 * misbehave and/or are incorrectly implemented.
 *
 * The BDL adjustment entry *always* has the IOC (Interrupt on Completion) bit set.
 *
 * For Intel Baytrail / Braswell implementations the BDL default adjustment is 32 frames, whereas
 * for ICH / PCH it's only one (1) frame.
 *
 * See default_bdl_pos_adj() and snd_hdac_stream_setup_periods() for more information.
 *
 * By default we apply some simple heuristics in hdaStreamInit().
 */
#define HDA_POS_ADJUST_DEFAULT      0

/** HDA's (fixed) audio frame size in bytes.
 *  We only support 16-bit stereo frames at the moment. */
#define HDA_FRAME_SIZE_DEFAULT      4

/** Offset of the SD0 register map. */
#define HDA_REG_DESC_SD0_BASE       0x80

/** Turn a short global register name into an memory index and a stringized name. */
#define HDA_REG_IDX(abbrev)         HDA_MEM_IND_NAME(abbrev), #abbrev

/** Turns a short stream register name into an memory index and a stringized name. */
#define HDA_REG_IDX_STRM(reg, suff) HDA_MEM_IND_NAME(reg ## suff), #reg #suff

/** Same as above for a register *not* stored in memory. */
#define HDA_REG_IDX_NOMEM(abbrev)   0, #abbrev

extern const HDAREGDESC g_aHdaRegMap[HDA_NUM_REGS];

/**
 * NB: Register values stored in memory (au32Regs[]) are indexed through
 * the HDA_RMX_xxx macros (also HDA_MEM_IND_NAME()). On the other hand, the
 * register descriptors in g_aHdaRegMap[] are indexed through the
 * HDA_REG_xxx macros (also HDA_REG_IND_NAME()).
 *
 * The au32Regs[] layout is kept unchanged for saved state
 * compatibility.
 */

/* Registers */
#define HDA_REG_IND_NAME(x)         HDA_REG_##x
#define HDA_MEM_IND_NAME(x)         HDA_RMX_##x
#define HDA_REG_IND(pThis, x)       ((pThis)->au32Regs[g_aHdaRegMap[x].mem_idx])
#define HDA_REG(pThis, x)           (HDA_REG_IND((pThis), HDA_REG_IND_NAME(x)))


#define HDA_REG_GCAP                0           /* Range 0x00 - 0x01 */
#define HDA_RMX_GCAP                0
/**
 * GCAP HDASpec 3.3.2 This macro encodes the following information about HDA in a compact manner:
 *
 * oss (15:12) - Number of output streams supported.
 * iss (11:8)  - Number of input streams supported.
 * bss (7:3)   - Number of bidirectional streams supported.
 * bds (2:1)   - Number of serial data out (SDO) signals supported.
 * b64sup (0)  - 64 bit addressing supported.
 */
#define HDA_MAKE_GCAP(oss, iss, bss, bds, b64sup) \
    (  (((oss)   & 0xF)  << 12) \
     | (((iss)   & 0xF)  << 8)  \
     | (((bss)   & 0x1F) << 3)  \
     | (((bds)   & 0x3)  << 2)  \
     | ((b64sup) & 1))

#define HDA_REG_VMIN                1           /* 0x02 */
#define HDA_RMX_VMIN                1

#define HDA_REG_VMAJ                2           /* 0x03 */
#define HDA_RMX_VMAJ                2

#define HDA_REG_OUTPAY              3           /* 0x04-0x05 */
#define HDA_RMX_OUTPAY              3

#define HDA_REG_INPAY               4           /* 0x06-0x07 */
#define HDA_RMX_INPAY               4

#define HDA_REG_GCTL                5           /* 0x08-0x0B */
#define HDA_RMX_GCTL                5
#define HDA_GCTL_UNSOL              RT_BIT(8)   /* Accept Unsolicited Response Enable */
#define HDA_GCTL_FCNTRL             RT_BIT(1)   /* Flush Control */
#define HDA_GCTL_CRST               RT_BIT(0)   /* Controller Reset */

#define HDA_REG_WAKEEN              6           /* 0x0C */
#define HDA_RMX_WAKEEN              6

#define HDA_REG_STATESTS            7           /* 0x0E */
#define HDA_RMX_STATESTS            7
#define HDA_STATESTS_SCSF_MASK      0x7         /* State Change Status Flags (6.2.8). */

#define HDA_REG_GSTS                8           /* 0x10-0x11*/
#define HDA_RMX_GSTS                8
#define HDA_GSTS_FSTS               RT_BIT(1)   /* Flush Status */

#define HDA_REG_OUTSTRMPAY          9           /* 0x18 */
#define HDA_RMX_OUTSTRMPAY          112

#define HDA_REG_INSTRMPAY           10          /* 0x1a */
#define HDA_RMX_INSTRMPAY           113

#define HDA_REG_INTCTL              11          /* 0x20 */
#define HDA_RMX_INTCTL              9
#define HDA_INTCTL_GIE              RT_BIT(31)  /* Global Interrupt Enable */
#define HDA_INTCTL_CIE              RT_BIT(30)  /* Controller Interrupt Enable */
/** Bits 0-29 correspond to streams 0-29. */
#define HDA_STRMINT_MASK            0xFF        /* Streams 0-7 implemented. Applies to INTCTL and INTSTS. */

#define HDA_REG_INTSTS              12          /* 0x24 */
#define HDA_RMX_INTSTS              10
#define HDA_INTSTS_GIS              RT_BIT(31)  /* Global Interrupt Status */
#define HDA_INTSTS_CIS              RT_BIT(30)  /* Controller Interrupt Status */

#define HDA_REG_WALCLK              13          /* 0x30 */
/**NB: HDA_RMX_WALCLK is not defined because the register is not stored in memory. */

/**
 * Note: The HDA specification defines a SSYNC register at offset 0x38. The
 * ICH6/ICH9 datahseet defines SSYNC at offset 0x34. The Linux HDA driver matches
 * the datasheet.
 */
#define HDA_REG_SSYNC               14          /* 0x34 */
#define HDA_RMX_SSYNC               12

#define HDA_REG_CORBLBASE           15          /* 0x40 */
#define HDA_RMX_CORBLBASE           13

#define HDA_REG_CORBUBASE           16          /* 0x44 */
#define HDA_RMX_CORBUBASE           14

#define HDA_REG_CORBWP              17          /* 0x48 */
#define HDA_RMX_CORBWP              15

#define HDA_REG_CORBRP              18          /* 0x4A */
#define HDA_RMX_CORBRP              16
#define HDA_CORBRP_RST              RT_BIT(15)  /* CORB Read Pointer Reset */

#define HDA_REG_CORBCTL             19          /* 0x4C */
#define HDA_RMX_CORBCTL             17
#define HDA_CORBCTL_DMA             RT_BIT(1)   /* Enable CORB DMA Engine */
#define HDA_CORBCTL_CMEIE           RT_BIT(0)   /* CORB Memory Error Interrupt Enable */

#define HDA_REG_CORBSTS             20          /* 0x4D */
#define HDA_RMX_CORBSTS             18

#define HDA_REG_CORBSIZE            21          /* 0x4E */
#define HDA_RMX_CORBSIZE            19
#define HDA_CORBSIZE_SZ_CAP         0xF0
#define HDA_CORBSIZE_SZ             0x3

/** Number of CORB buffer entries. */
#define HDA_CORB_SIZE               256
/** CORB element size (in bytes). */
#define HDA_CORB_ELEMENT_SIZE       4
/** Number of RIRB buffer entries. */
#define HDA_RIRB_SIZE               256
/** RIRB element size (in bytes). */
#define HDA_RIRB_ELEMENT_SIZE       8

#define HDA_REG_RIRBLBASE           22          /* 0x50 */
#define HDA_RMX_RIRBLBASE           20

#define HDA_REG_RIRBUBASE           23          /* 0x54 */
#define HDA_RMX_RIRBUBASE           21

#define HDA_REG_RIRBWP              24          /* 0x58 */
#define HDA_RMX_RIRBWP              22
#define HDA_RIRBWP_RST              RT_BIT(15)  /* RIRB Write Pointer Reset */

#define HDA_REG_RINTCNT             25          /* 0x5A */
#define HDA_RMX_RINTCNT             23

/** Maximum number of Response Interrupts. */
#define HDA_MAX_RINTCNT             256

#define HDA_REG_RIRBCTL             26          /* 0x5C */
#define HDA_RMX_RIRBCTL             24
#define HDA_RIRBCTL_ROIC            RT_BIT(2)   /* Response Overrun Interrupt Control */
#define HDA_RIRBCTL_RDMAEN          RT_BIT(1)   /* RIRB DMA Enable */
#define HDA_RIRBCTL_RINTCTL         RT_BIT(0)   /* Response Interrupt Control */

#define HDA_REG_RIRBSTS             27          /* 0x5D */
#define HDA_RMX_RIRBSTS             25
#define HDA_RIRBSTS_RIRBOIS         RT_BIT(2)   /* Response Overrun Interrupt Status */
#define HDA_RIRBSTS_RINTFL          RT_BIT(0)   /* Response Interrupt Flag */

#define HDA_REG_RIRBSIZE            28          /* 0x5E */
#define HDA_RMX_RIRBSIZE            26

#define HDA_REG_IC                  29          /* 0x60 */
#define HDA_RMX_IC                  27

#define HDA_REG_IR                  30          /* 0x64 */
#define HDA_RMX_IR                  28

#define HDA_REG_IRS                 31          /* 0x68 */
#define HDA_RMX_IRS                 29
#define HDA_IRS_IRV                 RT_BIT(1)   /* Immediate Result Valid */
#define HDA_IRS_ICB                 RT_BIT(0)   /* Immediate Command Busy */

#define HDA_REG_DPLBASE             32          /* 0x70 */
#define HDA_RMX_DPLBASE             30

#define HDA_REG_DPUBASE             33          /* 0x74 */
#define HDA_RMX_DPUBASE             31

#define DPBASE_ADDR_MASK            (~(uint64_t)0x7f)

#define HDA_STREAM_REG_DEF(name, num)           (HDA_REG_SD##num##name)
#define HDA_STREAM_RMX_DEF(name, num)           (HDA_RMX_SD##num##name)
/** Note: sdnum here _MUST_ be stream reg number [0,7]. */
#define HDA_STREAM_REG(pThis, name, sdnum)      (HDA_REG_IND((pThis), HDA_REG_SD0##name + (sdnum) * 10))

#define HDA_SD_NUM_FROM_REG(pThis, func, reg)   ((reg - HDA_STREAM_REG_DEF(func, 0)) / 10)

/** @todo Condense marcos! */

#define HDA_REG_SD0CTL              HDA_NUM_GENERAL_REGS /* 0x80; other streams offset by 0x20 */
#define HDA_RMX_SD0CTL              32
#define HDA_RMX_SD1CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 10)
#define HDA_RMX_SD2CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 20)
#define HDA_RMX_SD3CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 30)
#define HDA_RMX_SD4CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 40)
#define HDA_RMX_SD5CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 50)
#define HDA_RMX_SD6CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 60)
#define HDA_RMX_SD7CTL              (HDA_STREAM_RMX_DEF(CTL, 0) + 70)

#define HDA_SDCTL_NUM_MASK          0xF
#define HDA_SDCTL_NUM_SHIFT         20
#define HDA_SDCTL_DIR               RT_BIT(19)  /* Direction (Bidirectional streams only!) */
#define HDA_SDCTL_TP                RT_BIT(18)  /* Traffic Priority (PCI Express) */
#define HDA_SDCTL_STRIPE_MASK       0x3
#define HDA_SDCTL_STRIPE_SHIFT      16
#define HDA_SDCTL_DEIE              RT_BIT(4)   /* Descriptor Error Interrupt Enable */
#define HDA_SDCTL_FEIE              RT_BIT(3)   /* FIFO Error Interrupt Enable */
#define HDA_SDCTL_IOCE              RT_BIT(2)   /* Interrupt On Completion Enable */
#define HDA_SDCTL_RUN               RT_BIT(1)   /* Stream Run */
#define HDA_SDCTL_SRST              RT_BIT(0)   /* Stream Reset */

#define HDA_REG_SD0STS              35          /* 0x83; other streams offset by 0x20 */
#define HDA_RMX_SD0STS              33
#define HDA_RMX_SD1STS              (HDA_STREAM_RMX_DEF(STS, 0) + 10)
#define HDA_RMX_SD2STS              (HDA_STREAM_RMX_DEF(STS, 0) + 20)
#define HDA_RMX_SD3STS              (HDA_STREAM_RMX_DEF(STS, 0) + 30)
#define HDA_RMX_SD4STS              (HDA_STREAM_RMX_DEF(STS, 0) + 40)
#define HDA_RMX_SD5STS              (HDA_STREAM_RMX_DEF(STS, 0) + 50)
#define HDA_RMX_SD6STS              (HDA_STREAM_RMX_DEF(STS, 0) + 60)
#define HDA_RMX_SD7STS              (HDA_STREAM_RMX_DEF(STS, 0) + 70)

#define HDA_SDSTS_FIFORDY           RT_BIT(5)   /* FIFO Ready */
#define HDA_SDSTS_DESE              RT_BIT(4)   /* Descriptor Error */
#define HDA_SDSTS_FIFOE             RT_BIT(3)   /* FIFO Error */
#define HDA_SDSTS_BCIS              RT_BIT(2)   /* Buffer Completion Interrupt Status */

#define HDA_REG_SD0LPIB             36          /* 0x84; other streams offset by 0x20 */
#define HDA_REG_SD1LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 10) /* 0xA4 */
#define HDA_REG_SD2LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 20) /* 0xC4 */
#define HDA_REG_SD3LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 30) /* 0xE4 */
#define HDA_REG_SD4LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 40) /* 0x104 */
#define HDA_REG_SD5LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 50) /* 0x124 */
#define HDA_REG_SD6LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 60) /* 0x144 */
#define HDA_REG_SD7LPIB             (HDA_STREAM_REG_DEF(LPIB, 0) + 70) /* 0x164 */
#define HDA_RMX_SD0LPIB             34
#define HDA_RMX_SD1LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 10)
#define HDA_RMX_SD2LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 20)
#define HDA_RMX_SD3LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 30)
#define HDA_RMX_SD4LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 40)
#define HDA_RMX_SD5LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 50)
#define HDA_RMX_SD6LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 60)
#define HDA_RMX_SD7LPIB             (HDA_STREAM_RMX_DEF(LPIB, 0) + 70)

#define HDA_REG_SD0CBL              37          /* 0x88; other streams offset by 0x20 */
#define HDA_RMX_SD0CBL              35
#define HDA_RMX_SD1CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 10)
#define HDA_RMX_SD2CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 20)
#define HDA_RMX_SD3CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 30)
#define HDA_RMX_SD4CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 40)
#define HDA_RMX_SD5CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 50)
#define HDA_RMX_SD6CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 60)
#define HDA_RMX_SD7CBL              (HDA_STREAM_RMX_DEF(CBL, 0) + 70)

#define HDA_REG_SD0LVI              38          /* 0x8C; other streams offset by 0x20 */
#define HDA_RMX_SD0LVI              36
#define HDA_RMX_SD1LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 10)
#define HDA_RMX_SD2LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 20)
#define HDA_RMX_SD3LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 30)
#define HDA_RMX_SD4LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 40)
#define HDA_RMX_SD5LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 50)
#define HDA_RMX_SD6LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 60)
#define HDA_RMX_SD7LVI              (HDA_STREAM_RMX_DEF(LVI, 0) + 70)

#define HDA_REG_SD0FIFOW            39          /* 0x8E; other streams offset by 0x20 */
#define HDA_RMX_SD0FIFOW            37
#define HDA_RMX_SD1FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 10)
#define HDA_RMX_SD2FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 20)
#define HDA_RMX_SD3FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 30)
#define HDA_RMX_SD4FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 40)
#define HDA_RMX_SD5FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 50)
#define HDA_RMX_SD6FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 60)
#define HDA_RMX_SD7FIFOW            (HDA_STREAM_RMX_DEF(FIFOW, 0) + 70)

/*
 * ICH6 datasheet defined limits for FIFOW values (18.2.38).
 */
#define HDA_SDFIFOW_8B              0x2
#define HDA_SDFIFOW_16B             0x3
#define HDA_SDFIFOW_32B             0x4

#define HDA_REG_SD0FIFOS            40          /* 0x90; other streams offset by 0x20 */
#define HDA_RMX_SD0FIFOS            38
#define HDA_RMX_SD1FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 10)
#define HDA_RMX_SD2FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 20)
#define HDA_RMX_SD3FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 30)
#define HDA_RMX_SD4FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 40)
#define HDA_RMX_SD5FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 50)
#define HDA_RMX_SD6FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 60)
#define HDA_RMX_SD7FIFOS            (HDA_STREAM_RMX_DEF(FIFOS, 0) + 70)

#define HDA_SDIFIFO_120B            0x77        /* 8-, 16-, 20-, 24-, 32-bit Input Streams */
#define HDA_SDIFIFO_160B            0x9F        /* 20-, 24-bit Input Streams Streams */

#define HDA_SDOFIFO_16B             0x0F        /* 8-, 16-, 20-, 24-, 32-bit Output Streams */
#define HDA_SDOFIFO_32B             0x1F        /* 8-, 16-, 20-, 24-, 32-bit Output Streams */
#define HDA_SDOFIFO_64B             0x3F        /* 8-, 16-, 20-, 24-, 32-bit Output Streams */
#define HDA_SDOFIFO_128B            0x7F        /* 8-, 16-, 20-, 24-, 32-bit Output Streams */
#define HDA_SDOFIFO_192B            0xBF        /* 8-, 16-, 20-, 24-, 32-bit Output Streams */
#define HDA_SDOFIFO_256B            0xFF        /* 20-, 24-bit Output Streams */

#define HDA_REG_SD0FMT              41          /* 0x92; other streams offset by 0x20 */
#define HDA_RMX_SD0FMT              39
#define HDA_RMX_SD1FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 10)
#define HDA_RMX_SD2FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 20)
#define HDA_RMX_SD3FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 30)
#define HDA_RMX_SD4FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 40)
#define HDA_RMX_SD5FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 50)
#define HDA_RMX_SD6FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 60)
#define HDA_RMX_SD7FMT              (HDA_STREAM_RMX_DEF(FMT, 0) + 70)

#define HDA_REG_SD0BDPL             42          /* 0x98; other streams offset by 0x20 */
#define HDA_RMX_SD0BDPL             40
#define HDA_RMX_SD1BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 10)
#define HDA_RMX_SD2BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 20)
#define HDA_RMX_SD3BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 30)
#define HDA_RMX_SD4BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 40)
#define HDA_RMX_SD5BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 50)
#define HDA_RMX_SD6BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 60)
#define HDA_RMX_SD7BDPL             (HDA_STREAM_RMX_DEF(BDPL, 0) + 70)

#define HDA_REG_SD0BDPU             43          /* 0x9C; other streams offset by 0x20 */
#define HDA_RMX_SD0BDPU             41
#define HDA_RMX_SD1BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 10)
#define HDA_RMX_SD2BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 20)
#define HDA_RMX_SD3BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 30)
#define HDA_RMX_SD4BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 40)
#define HDA_RMX_SD5BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 50)
#define HDA_RMX_SD6BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 60)
#define HDA_RMX_SD7BDPU             (HDA_STREAM_RMX_DEF(BDPU, 0) + 70)

#define HDA_CODEC_CAD_SHIFT         28
/** Encodes the (required) LUN into a codec command. */
#define HDA_CODEC_CMD(cmd, lun)     ((cmd) | (lun << HDA_CODEC_CAD_SHIFT))

#define HDA_SDFMT_NON_PCM_SHIFT     15
#define HDA_SDFMT_NON_PCM_MASK      0x1
#define HDA_SDFMT_BASE_RATE_SHIFT   14
#define HDA_SDFMT_BASE_RATE_MASK    0x1
#define HDA_SDFMT_MULT_SHIFT        11
#define HDA_SDFMT_MULT_MASK         0x7
#define HDA_SDFMT_DIV_SHIFT         8
#define HDA_SDFMT_DIV_MASK          0x7
#define HDA_SDFMT_BITS_SHIFT        4
#define HDA_SDFMT_BITS_MASK         0x7
#define HDA_SDFMT_CHANNELS_MASK     0xF

#define HDA_SDFMT_TYPE              RT_BIT(15)
#define HDA_SDFMT_TYPE_PCM          (0)
#define HDA_SDFMT_TYPE_NON_PCM      (1)

#define HDA_SDFMT_BASE              RT_BIT(14)
#define HDA_SDFMT_BASE_48KHZ        (0)
#define HDA_SDFMT_BASE_44KHZ        (1)

#define HDA_SDFMT_MULT_1X           (0)
#define HDA_SDFMT_MULT_2X           (1)
#define HDA_SDFMT_MULT_3X           (2)
#define HDA_SDFMT_MULT_4X           (3)

#define HDA_SDFMT_DIV_1X            (0)
#define HDA_SDFMT_DIV_2X            (1)
#define HDA_SDFMT_DIV_3X            (2)
#define HDA_SDFMT_DIV_4X            (3)
#define HDA_SDFMT_DIV_5X            (4)
#define HDA_SDFMT_DIV_6X            (5)
#define HDA_SDFMT_DIV_7X            (6)
#define HDA_SDFMT_DIV_8X            (7)

#define HDA_SDFMT_8_BIT             (0)
#define HDA_SDFMT_16_BIT            (1)
#define HDA_SDFMT_20_BIT            (2)
#define HDA_SDFMT_24_BIT            (3)
#define HDA_SDFMT_32_BIT            (4)

#define HDA_SDFMT_CHAN_MONO         (0)
#define HDA_SDFMT_CHAN_STEREO       (1)

/** Emits a SDnFMT register format.
 * Also being used in the codec's converter format. */
#define HDA_SDFMT_MAKE(_afNonPCM, _aBaseRate, _aMult, _aDiv, _aBits, _aChan)    \
    (  (((_afNonPCM)  & HDA_SDFMT_NON_PCM_MASK)   << HDA_SDFMT_NON_PCM_SHIFT)   \
     | (((_aBaseRate) & HDA_SDFMT_BASE_RATE_MASK) << HDA_SDFMT_BASE_RATE_SHIFT) \
     | (((_aMult)     & HDA_SDFMT_MULT_MASK)      << HDA_SDFMT_MULT_SHIFT)      \
     | (((_aDiv)      & HDA_SDFMT_DIV_MASK)       << HDA_SDFMT_DIV_SHIFT)       \
     | (((_aBits)     & HDA_SDFMT_BITS_MASK)      << HDA_SDFMT_BITS_SHIFT)      \
     | ( (_aChan)     & HDA_SDFMT_CHANNELS_MASK))

/** Interrupt on completion (IOC) flag. */
#define HDA_BDLE_FLAG_IOC           RT_BIT(0)



/** The HDA controller. */
typedef struct HDASTATE *PHDASTATE;
/** The HDA stream. */
typedef struct HDASTREAM *PHDASTREAM;

typedef struct HDAMIXERSINK *PHDAMIXERSINK;


/**
 * Internal state of a Buffer Descriptor List Entry (BDLE),
 * needed to keep track of the data needed for the actual device
 * emulation.
 */
typedef struct HDABDLESTATE
{
    /** Own index within the BDL (Buffer Descriptor List). */
    uint32_t     u32BDLIndex;
    /** Number of bytes below the stream's FIFO watermark (SDFIFOW).
     *  Used to check if we need fill up the FIFO again. */
    uint32_t     cbBelowFIFOW;
    /** Current offset in DMA buffer (in bytes).*/
    uint32_t     u32BufOff;
    uint32_t     Padding;
} HDABDLESTATE, *PHDABDLESTATE;

/**
 * BDL description structure.
 * Do not touch this, as this must match to the HDA specs.
 */
typedef struct HDABDLEDESC
{
    /** Starting address of the actual buffer. Must be 128-bit aligned. */
    uint64_t     u64BufAddr;
    /** Size of the actual buffer (in bytes). */
    uint32_t     u32BufSize;
    /** Bit 0: Interrupt on completion; the controller will generate
     *  an interrupt when the last byte of the buffer has been
     *  fetched by the DMA engine.
     *
     *  Rest is reserved for further use and must be 0. */
    uint32_t     fFlags;
} HDABDLEDESC, *PHDABDLEDESC;
AssertCompileSize(HDABDLEDESC, 16); /* Always 16 byte. Also must be aligned on 128-byte boundary. */

/**
 * Buffer Descriptor List Entry (BDLE) (3.6.3).
 */
typedef struct HDABDLE
{
    /** The actual BDL description. */
    HDABDLEDESC  Desc;
    /** Internal state of this BDLE.
     *  Not part of the actual BDLE registers. */
    HDABDLESTATE State;
} HDABDLE;
AssertCompileSizeAlignment(HDABDLE, 8);
/** Pointer to a buffer descriptor list entry (BDLE). */
typedef HDABDLE *PHDABDLE;

/** @name Object lookup functions.
 * @{
 */
#ifdef IN_RING3
PHDAMIXERSINK hdaR3GetDefaultSink(PHDASTATE pThis, uint8_t uSD);
#endif
PDMAUDIODIR   hdaGetDirFromSD(uint8_t uSD);
PHDASTREAM    hdaGetStreamFromSD(PHDASTATE pThis, uint8_t uSD);
#ifdef IN_RING3
PHDASTREAM    hdaR3GetStreamFromSink(PHDASTATE pThis, PHDAMIXERSINK pSink);
#endif
/** @} */

/** @name Interrupt functions.
 * @{
 */
#ifdef LOG_ENABLED
int           hdaProcessInterrupt(PHDASTATE pThis, const char *pszSource);
#else
int           hdaProcessInterrupt(PHDASTATE pThis);
#endif
/** @} */

/** @name Wall clock (WALCLK) functions.
 * @{
 */
uint64_t      hdaWalClkGetCurrent(PHDASTATE pThis);
#ifdef IN_RING3
bool          hdaR3WalClkSet(PHDASTATE pThis, uint64_t u64WalClk, bool fForce);
#endif
/** @} */

/** @name DMA utility functions.
 * @{
 */
#ifdef IN_RING3
int           hdaR3DMARead(PHDASTATE pThis, PHDASTREAM pStream, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead);
int           hdaR3DMAWrite(PHDASTATE pThis, PHDASTREAM pStream, const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten);
#endif
/** @} */

/** @name Register functions.
 * @{
 */
uint32_t      hdaGetINTSTS(PHDASTATE pThis);
#ifdef IN_RING3
int           hdaR3SDFMTToPCMProps(uint16_t u16SDFMT, PPDMAUDIOPCMPROPS pProps);
#endif /* IN_RING3 */
/** @} */

/** @name BDLE (Buffer Descriptor List Entry) functions.
 * @{
 */
#ifdef IN_RING3
# ifdef LOG_ENABLED
void          hdaR3BDLEDumpAll(PHDASTATE pThis, uint64_t u64BDLBase, uint16_t cBDLE);
# endif
int           hdaR3BDLEFetch(PHDASTATE pThis, PHDABDLE pBDLE, uint64_t u64BaseDMA, uint16_t u16Entry);
bool          hdaR3BDLEIsComplete(PHDABDLE pBDLE);
bool          hdaR3BDLENeedsInterrupt(PHDABDLE pBDLE);
#endif /* IN_RING3 */
/** @} */

/** @name Device timer functions.
 * @{
 */
#ifdef IN_RING3
bool          hdaR3TimerSet(PHDASTATE pThis, PHDASTREAM pStream, uint64_t u64Expire, bool fForce);
#endif /* IN_RING3 */
/** @} */

#endif /* !VBOX_INCLUDED_SRC_Audio_DevHDACommon_h */

