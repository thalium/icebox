/* $Id: HDACodec.cpp $ */
/** @file
 * HDACodec - VBox HD Audio Codec.
 *
 * Implemented based on the Intel HD Audio specification and the
 * Sigmatel/IDT STAC9220 datasheet.
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
#define LOG_GROUP LOG_GROUP_DEV_HDA_CODEC
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/asm.h>
#include <iprt/cpp/utils.h>

#include "VBoxDD.h"
#include "DrvAudio.h"
#include "HDACodec.h"
#include "DevHDACommon.h"
#include "AudioMixer.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/* PRM 5.3.1 */
/** Codec address mask. */
#define CODEC_CAD_MASK                                     0xF0000000
/** Codec address shift. */
#define CODEC_CAD_SHIFT                                    28
#define CODEC_DIRECT_MASK                                  RT_BIT(27)
/** Node ID mask. */
#define CODEC_NID_MASK                                     0x07F00000
/** Node ID shift. */
#define CODEC_NID_SHIFT                                    20
#define CODEC_VERBDATA_MASK                                0x000FFFFF
#define CODEC_VERB_4BIT_CMD                                0x000FFFF0
#define CODEC_VERB_4BIT_DATA                               0x0000000F
#define CODEC_VERB_8BIT_CMD                                0x000FFF00
#define CODEC_VERB_8BIT_DATA                               0x000000FF
#define CODEC_VERB_16BIT_CMD                               0x000F0000
#define CODEC_VERB_16BIT_DATA                              0x0000FFFF

#define CODEC_CAD(cmd)                                     (((cmd) & CODEC_CAD_MASK) >> CODEC_CAD_SHIFT)
#define CODEC_DIRECT(cmd)                                  ((cmd) & CODEC_DIRECT_MASK)
#define CODEC_NID(cmd)                                     ((((cmd) & CODEC_NID_MASK)) >> CODEC_NID_SHIFT)
#define CODEC_VERBDATA(cmd)                                ((cmd) & CODEC_VERBDATA_MASK)
#define CODEC_VERB_CMD(cmd, mask, x)                       (((cmd) & (mask)) >> (x))
#define CODEC_VERB_CMD4(cmd)                               (CODEC_VERB_CMD((cmd), CODEC_VERB_4BIT_CMD, 4))
#define CODEC_VERB_CMD8(cmd)                               (CODEC_VERB_CMD((cmd), CODEC_VERB_8BIT_CMD, 8))
#define CODEC_VERB_CMD16(cmd)                              (CODEC_VERB_CMD((cmd), CODEC_VERB_16BIT_CMD, 16))
#define CODEC_VERB_PAYLOAD4(cmd)                           ((cmd) & CODEC_VERB_4BIT_DATA)
#define CODEC_VERB_PAYLOAD8(cmd)                           ((cmd) & CODEC_VERB_8BIT_DATA)
#define CODEC_VERB_PAYLOAD16(cmd)                          ((cmd) & CODEC_VERB_16BIT_DATA)

#define CODEC_VERB_GET_AMP_DIRECTION                       RT_BIT(15)
#define CODEC_VERB_GET_AMP_SIDE                            RT_BIT(13)
#define CODEC_VERB_GET_AMP_INDEX                           0x7

/* HDA spec 7.3.3.7 NoteA */
#define CODEC_GET_AMP_DIRECTION(cmd)                       (((cmd) & CODEC_VERB_GET_AMP_DIRECTION) >> 15)
#define CODEC_GET_AMP_SIDE(cmd)                            (((cmd) & CODEC_VERB_GET_AMP_SIDE) >> 13)
#define CODEC_GET_AMP_INDEX(cmd)                           (CODEC_GET_AMP_DIRECTION(cmd) ? 0 : ((cmd) & CODEC_VERB_GET_AMP_INDEX))

/* HDA spec 7.3.3.7 NoteC */
#define CODEC_VERB_SET_AMP_OUT_DIRECTION                   RT_BIT(15)
#define CODEC_VERB_SET_AMP_IN_DIRECTION                    RT_BIT(14)
#define CODEC_VERB_SET_AMP_LEFT_SIDE                       RT_BIT(13)
#define CODEC_VERB_SET_AMP_RIGHT_SIDE                      RT_BIT(12)
#define CODEC_VERB_SET_AMP_INDEX                           (0x7 << 8)
#define CODEC_VERB_SET_AMP_MUTE                            RT_BIT(7)
/** Note: 7-bit value [6:0]. */
#define CODEC_VERB_SET_AMP_GAIN                            0x7F

#define CODEC_SET_AMP_IS_OUT_DIRECTION(cmd)                (((cmd) & CODEC_VERB_SET_AMP_OUT_DIRECTION) != 0)
#define CODEC_SET_AMP_IS_IN_DIRECTION(cmd)                 (((cmd) & CODEC_VERB_SET_AMP_IN_DIRECTION) != 0)
#define CODEC_SET_AMP_IS_LEFT_SIDE(cmd)                    (((cmd) & CODEC_VERB_SET_AMP_LEFT_SIDE) != 0)
#define CODEC_SET_AMP_IS_RIGHT_SIDE(cmd)                   (((cmd) & CODEC_VERB_SET_AMP_RIGHT_SIDE) != 0)
#define CODEC_SET_AMP_INDEX(cmd)                           (((cmd) & CODEC_VERB_SET_AMP_INDEX) >> 7)
#define CODEC_SET_AMP_MUTE(cmd)                            ((cmd) & CODEC_VERB_SET_AMP_MUTE)
#define CODEC_SET_AMP_GAIN(cmd)                            ((cmd) & CODEC_VERB_SET_AMP_GAIN)

/* HDA spec 7.3.3.1 defines layout of configuration registers/verbs (0xF00) */
/* VendorID (7.3.4.1) */
#define CODEC_MAKE_F00_00(vendorID, deviceID)              (((vendorID) << 16) | (deviceID))
#define CODEC_F00_00_VENDORID(f00_00)                      (((f00_00) >> 16) & 0xFFFF)
#define CODEC_F00_00_DEVICEID(f00_00)                      ((f00_00) & 0xFFFF)

/** RevisionID (7.3.4.2). */
#define CODEC_MAKE_F00_02(majRev, minRev, venFix, venProg, stepFix, stepProg) \
    (  (((majRev)   & 0xF) << 20) \
     | (((minRev)   & 0xF) << 16) \
     | (((venFix)   & 0xF) << 12) \
     | (((venProg)  & 0xF) << 8)  \
     | (((stepFix)  & 0xF) << 4)  \
     |  ((stepProg) & 0xF))

/** Subordinate node count (7.3.4.3). */
#define CODEC_MAKE_F00_04(startNodeNumber, totalNodeNumber) ((((startNodeNumber) & 0xFF) << 16)|((totalNodeNumber) & 0xFF))
#define CODEC_F00_04_TO_START_NODE_NUMBER(f00_04)          (((f00_04) >> 16) & 0xFF)
#define CODEC_F00_04_TO_NODE_COUNT(f00_04)                 ((f00_04) & 0xFF)
/*
 * Function Group Type  (7.3.4.4)
 * 0 & [0x3-0x7f] are reserved types
 * [0x80 - 0xff] are vendor defined function groups
 */
#define CODEC_MAKE_F00_05(UnSol, NodeType)                 (((UnSol) << 8)|(NodeType))
#define CODEC_F00_05_UNSOL                                 RT_BIT(8)
#define CODEC_F00_05_AFG                                   (0x1)
#define CODEC_F00_05_MFG                                   (0x2)
#define CODEC_F00_05_IS_UNSOL(f00_05)                      RT_BOOL((f00_05) & RT_BIT(8))
#define CODEC_F00_05_GROUP(f00_05)                         ((f00_05) & 0xff)
/* Audio Function Group capabilities (7.3.4.5). */
#define CODEC_MAKE_F00_08(BeepGen, InputDelay, OutputDelay) ((((BeepGen) & 0x1) << 16)| (((InputDelay) & 0xF) << 8) | ((OutputDelay) & 0xF))
#define CODEC_F00_08_BEEP_GEN(f00_08)                      ((f00_08) & RT_BIT(16)

/* Converter Stream, Channel (7.3.3.11). */
#define CODEC_F00_06_GET_STREAM_ID(cmd)                    (((cmd) >> 4) & 0x0F)
#define CODEC_F00_06_GET_CHANNEL_ID(cmd)                   (((cmd) & 0x0F))

/* Widget Capabilities (7.3.4.6). */
#define CODEC_MAKE_F00_09(type, delay, chan_ext) \
    ( (((type)     & 0xF) << 20)            \
    | (((delay)    & 0xF) << 16)           \
    | (((chan_ext) & 0xF) << 13))
/* note: types 0x8-0xe are reserved */
#define CODEC_F00_09_TYPE_AUDIO_OUTPUT                     (0x0)
#define CODEC_F00_09_TYPE_AUDIO_INPUT                      (0x1)
#define CODEC_F00_09_TYPE_AUDIO_MIXER                      (0x2)
#define CODEC_F00_09_TYPE_AUDIO_SELECTOR                   (0x3)
#define CODEC_F00_09_TYPE_PIN_COMPLEX                      (0x4)
#define CODEC_F00_09_TYPE_POWER_WIDGET                     (0x5)
#define CODEC_F00_09_TYPE_VOLUME_KNOB                      (0x6)
#define CODEC_F00_09_TYPE_BEEP_GEN                         (0x7)
#define CODEC_F00_09_TYPE_VENDOR_DEFINED                   (0xF)

#define CODEC_F00_09_CAP_CP                                RT_BIT(12)
#define CODEC_F00_09_CAP_L_R_SWAP                          RT_BIT(11)
#define CODEC_F00_09_CAP_POWER_CTRL                        RT_BIT(10)
#define CODEC_F00_09_CAP_DIGITAL                           RT_BIT(9)
#define CODEC_F00_09_CAP_CONNECTION_LIST                   RT_BIT(8)
#define CODEC_F00_09_CAP_UNSOL                             RT_BIT(7)
#define CODEC_F00_09_CAP_PROC_WIDGET                       RT_BIT(6)
#define CODEC_F00_09_CAP_STRIPE                            RT_BIT(5)
#define CODEC_F00_09_CAP_FMT_OVERRIDE                      RT_BIT(4)
#define CODEC_F00_09_CAP_AMP_FMT_OVERRIDE                  RT_BIT(3)
#define CODEC_F00_09_CAP_OUT_AMP_PRESENT                   RT_BIT(2)
#define CODEC_F00_09_CAP_IN_AMP_PRESENT                    RT_BIT(1)
#define CODEC_F00_09_CAP_STEREO                            RT_BIT(0)

#define CODEC_F00_09_TYPE(f00_09)                          (((f00_09) >> 20) & 0xF)

#define CODEC_F00_09_IS_CAP_CP(f00_09)                     RT_BOOL((f00_09) & RT_BIT(12))
#define CODEC_F00_09_IS_CAP_L_R_SWAP(f00_09)               RT_BOOL((f00_09) & RT_BIT(11))
#define CODEC_F00_09_IS_CAP_POWER_CTRL(f00_09)             RT_BOOL((f00_09) & RT_BIT(10))
#define CODEC_F00_09_IS_CAP_DIGITAL(f00_09)                RT_BOOL((f00_09) & RT_BIT(9))
#define CODEC_F00_09_IS_CAP_CONNECTION_LIST(f00_09)        RT_BOOL((f00_09) & RT_BIT(8))
#define CODEC_F00_09_IS_CAP_UNSOL(f00_09)                  RT_BOOL((f00_09) & RT_BIT(7))
#define CODEC_F00_09_IS_CAP_PROC_WIDGET(f00_09)            RT_BOOL((f00_09) & RT_BIT(6))
#define CODEC_F00_09_IS_CAP_STRIPE(f00_09)                 RT_BOOL((f00_09) & RT_BIT(5))
#define CODEC_F00_09_IS_CAP_FMT_OVERRIDE(f00_09)           RT_BOOL((f00_09) & RT_BIT(4))
#define CODEC_F00_09_IS_CAP_AMP_OVERRIDE(f00_09)           RT_BOOL((f00_09) & RT_BIT(3))
#define CODEC_F00_09_IS_CAP_OUT_AMP_PRESENT(f00_09)        RT_BOOL((f00_09) & RT_BIT(2))
#define CODEC_F00_09_IS_CAP_IN_AMP_PRESENT(f00_09)         RT_BOOL((f00_09) & RT_BIT(1))
#define CODEC_F00_09_IS_CAP_LSB(f00_09)                    RT_BOOL((f00_09) & RT_BIT(0))

/* Supported PCM size, rates (7.3.4.7) */
#define CODEC_F00_0A_32_BIT                                RT_BIT(19)
#define CODEC_F00_0A_24_BIT                                RT_BIT(18)
#define CODEC_F00_0A_16_BIT                                RT_BIT(17)
#define CODEC_F00_0A_8_BIT                                 RT_BIT(16)

#define CODEC_F00_0A_48KHZ_MULT_8X                         RT_BIT(11)
#define CODEC_F00_0A_48KHZ_MULT_4X                         RT_BIT(10)
#define CODEC_F00_0A_44_1KHZ_MULT_4X                       RT_BIT(9)
#define CODEC_F00_0A_48KHZ_MULT_2X                         RT_BIT(8)
#define CODEC_F00_0A_44_1KHZ_MULT_2X                       RT_BIT(7)
#define CODEC_F00_0A_48KHZ                                 RT_BIT(6)
#define CODEC_F00_0A_44_1KHZ                               RT_BIT(5)
/* 2/3 * 48kHz */
#define CODEC_F00_0A_48KHZ_2_3X                            RT_BIT(4)
/* 1/2 * 44.1kHz */
#define CODEC_F00_0A_44_1KHZ_1_2X                          RT_BIT(3)
/* 1/3 * 48kHz */
#define CODEC_F00_0A_48KHZ_1_3X                            RT_BIT(2)
/* 1/4 * 44.1kHz */
#define CODEC_F00_0A_44_1KHZ_1_4X                          RT_BIT(1)
/* 1/6 * 48kHz */
#define CODEC_F00_0A_48KHZ_1_6X                            RT_BIT(0)

/* Supported streams formats (7.3.4.8) */
#define CODEC_F00_0B_AC3                                   RT_BIT(2)
#define CODEC_F00_0B_FLOAT32                               RT_BIT(1)
#define CODEC_F00_0B_PCM                                   RT_BIT(0)

/* Pin Capabilities (7.3.4.9)*/
#define CODEC_MAKE_F00_0C(vref_ctrl) (((vref_ctrl) & 0xFF) << 8)
#define CODEC_F00_0C_CAP_HBR                               RT_BIT(27)
#define CODEC_F00_0C_CAP_DP                                RT_BIT(24)
#define CODEC_F00_0C_CAP_EAPD                              RT_BIT(16)
#define CODEC_F00_0C_CAP_HDMI                              RT_BIT(7)
#define CODEC_F00_0C_CAP_BALANCED_IO                       RT_BIT(6)
#define CODEC_F00_0C_CAP_INPUT                             RT_BIT(5)
#define CODEC_F00_0C_CAP_OUTPUT                            RT_BIT(4)
#define CODEC_F00_0C_CAP_HEADPHONE_AMP                     RT_BIT(3)
#define CODEC_F00_0C_CAP_PRESENCE_DETECT                   RT_BIT(2)
#define CODEC_F00_0C_CAP_TRIGGER_REQUIRED                  RT_BIT(1)
#define CODEC_F00_0C_CAP_IMPENDANCE_SENSE                  RT_BIT(0)

#define CODEC_F00_0C_IS_CAP_HBR(f00_0c)                    ((f00_0c) & RT_BIT(27))
#define CODEC_F00_0C_IS_CAP_DP(f00_0c)                     ((f00_0c) & RT_BIT(24))
#define CODEC_F00_0C_IS_CAP_EAPD(f00_0c)                   ((f00_0c) & RT_BIT(16))
#define CODEC_F00_0C_IS_CAP_HDMI(f00_0c)                   ((f00_0c) & RT_BIT(7))
#define CODEC_F00_0C_IS_CAP_BALANCED_IO(f00_0c)            ((f00_0c) & RT_BIT(6))
#define CODEC_F00_0C_IS_CAP_INPUT(f00_0c)                  ((f00_0c) & RT_BIT(5))
#define CODEC_F00_0C_IS_CAP_OUTPUT(f00_0c)                 ((f00_0c) & RT_BIT(4))
#define CODEC_F00_0C_IS_CAP_HP(f00_0c)                     ((f00_0c) & RT_BIT(3))
#define CODEC_F00_0C_IS_CAP_PRESENCE_DETECT(f00_0c)        ((f00_0c) & RT_BIT(2))
#define CODEC_F00_0C_IS_CAP_TRIGGER_REQUIRED(f00_0c)       ((f00_0c) & RT_BIT(1))
#define CODEC_F00_0C_IS_CAP_IMPENDANCE_SENSE(f00_0c)       ((f00_0c) & RT_BIT(0))

/* Input Amplifier capabilities (7.3.4.10). */
#define CODEC_MAKE_F00_0D(mute_cap, step_size, num_steps, offset) \
        (  (((mute_cap)  & UINT32_C(0x1))  << 31) \
         | (((step_size) & UINT32_C(0xFF)) << 16) \
         | (((num_steps) & UINT32_C(0xFF)) << 8) \
         |  ((offset)    & UINT32_C(0xFF)))

#define CODEC_F00_0D_CAP_MUTE                              RT_BIT(7)

#define CODEC_F00_0D_IS_CAP_MUTE(f00_0d)                   ( ( f00_0d) & RT_BIT(31))
#define CODEC_F00_0D_STEP_SIZE(f00_0d)                     ((( f00_0d) & (0x7F << 16)) >> 16)
#define CODEC_F00_0D_NUM_STEPS(f00_0d)                     ((((f00_0d) & (0x7F << 8)) >> 8) + 1)
#define CODEC_F00_0D_OFFSET(f00_0d)                        (  (f00_0d) & 0x7F)

/** Indicates that the amplifier can be muted. */
#define CODEC_AMP_CAP_MUTE                                 0x1
/** The amplifier's maximum number of steps. We want
 *  a ~90dB dynamic range, so 64 steps with 1.25dB each
 *  should do the trick.
 *
 *  As we want to map our range to [0..128] values we can avoid
 *  multiplication and simply doing a shift later.
 *
 *  Produces -96dB to +0dB.
 *  "0" indicates a step of 0.25dB, "127" indicates a step of 32dB.
 */
#define CODEC_AMP_NUM_STEPS                                0x7F
/** The initial gain offset (and when doing a node reset). */
#define CODEC_AMP_OFF_INITIAL                              0x7F
/** The amplifier's gain step size. */
#define CODEC_AMP_STEP_SIZE                                0x2

/* Output Amplifier capabilities (7.3.4.10) */
#define CODEC_MAKE_F00_12                                  CODEC_MAKE_F00_0D

#define CODEC_F00_12_IS_CAP_MUTE(f00_12)                   CODEC_F00_0D_IS_CAP_MUTE(f00_12)
#define CODEC_F00_12_STEP_SIZE(f00_12)                     CODEC_F00_0D_STEP_SIZE(f00_12)
#define CODEC_F00_12_NUM_STEPS(f00_12)                     CODEC_F00_0D_NUM_STEPS(f00_12)
#define CODEC_F00_12_OFFSET(f00_12)                        CODEC_F00_0D_OFFSET(f00_12)

/* Connection list lenght (7.3.4.11). */
#define CODEC_MAKE_F00_0E(long_form, length)    \
    (  (((long_form) & 0x1) << 7)               \
     | ((length) & 0x7F))
/* Indicates short-form NIDs. */
#define CODEC_F00_0E_LIST_NID_SHORT                        0
/* Indicates long-form NIDs. */
#define CODEC_F00_0E_LIST_NID_LONG                         1
#define CODEC_F00_0E_IS_LONG(f00_0e)                       RT_BOOL((f00_0e) & RT_BIT(7))
#define CODEC_F00_0E_COUNT(f00_0e)                         ((f00_0e) & 0x7F)
/* Supported Power States (7.3.4.12) */
#define CODEC_F00_0F_EPSS                                  RT_BIT(31)
#define CODEC_F00_0F_CLKSTOP                               RT_BIT(30)
#define CODEC_F00_0F_S3D3                                  RT_BIT(29)
#define CODEC_F00_0F_D3COLD                                RT_BIT(4)
#define CODEC_F00_0F_D3                                    RT_BIT(3)
#define CODEC_F00_0F_D2                                    RT_BIT(2)
#define CODEC_F00_0F_D1                                    RT_BIT(1)
#define CODEC_F00_0F_D0                                    RT_BIT(0)

/* Processing capabilities 7.3.4.13 */
#define CODEC_MAKE_F00_10(num, benign)                     ((((num) & 0xFF) << 8) | ((benign) & 0x1))
#define CODEC_F00_10_NUM(f00_10)                           (((f00_10) & (0xFF << 8)) >> 8)
#define CODEC_F00_10_BENING(f00_10)                        ((f00_10) & 0x1)

/* GPIO count (7.3.4.14). */
#define CODEC_MAKE_F00_11(wake, unsol, numgpi, numgpo, numgpio) \
    (  (((wake)   & UINT32_C(0x1))  << 31) \
     | (((unsol)  & UINT32_C(0x1))  << 30) \
     | (((numgpi) & UINT32_C(0xFF)) << 16) \
     | (((numgpo) & UINT32_C(0xFF)) << 8) \
     | ((numgpio) & UINT32_C(0xFF)))

/* Processing States (7.3.3.4). */
#define CODEC_F03_OFF                                      (0)
#define CODEC_F03_ON                                       RT_BIT(0)
#define CODEC_F03_BENING                                   RT_BIT(1)
/* Power States (7.3.3.10). */
#define CODEC_MAKE_F05(reset, stopok, error, act, set) \
    (  (((reset)  & 0x1) << 10) \
     | (((stopok) & 0x1) << 9) \
     | (((error)  & 0x1) << 8) \
     | (((act)    & 0xF) << 4) \
     | ((set)     & 0xF))
#define CODEC_F05_D3COLD                                   (4)
#define CODEC_F05_D3                                       (3)
#define CODEC_F05_D2                                       (2)
#define CODEC_F05_D1                                       (1)
#define CODEC_F05_D0                                       (0)

#define CODEC_F05_IS_RESET(value)                          (((value) & RT_BIT(10)) != 0)
#define CODEC_F05_IS_STOPOK(value)                         (((value) & RT_BIT(9)) != 0)
#define CODEC_F05_IS_ERROR(value)                          (((value) & RT_BIT(8)) != 0)
#define CODEC_F05_ACT(value)                               (((value) & 0xF0) >> 4)
#define CODEC_F05_SET(value)                               (((value) & 0xF))

#define CODEC_F05_GE(p0, p1)                               ((p0) <= (p1))
#define CODEC_F05_LE(p0, p1)                               ((p0) >= (p1))

/* Converter Stream, Channel (7.3.3.11). */
#define CODEC_MAKE_F06(stream, channel) \
    (  (((stream)  & 0xF) << 4)         \
     |  ((channel) & 0xF))
#define CODEC_F06_STREAM(value)                            ((value) & 0xF0)
#define CODEC_F06_CHANNEL(value)                           ((value) & 0xF)

/* Pin Widged Control (7.3.3.13). */
#define CODEC_F07_VREF_HIZ                                 (0)
#define CODEC_F07_VREF_50                                  (0x1)
#define CODEC_F07_VREF_GROUND                              (0x2)
#define CODEC_F07_VREF_80                                  (0x4)
#define CODEC_F07_VREF_100                                 (0x5)
#define CODEC_F07_IN_ENABLE                                RT_BIT(5)
#define CODEC_F07_OUT_ENABLE                               RT_BIT(6)
#define CODEC_F07_OUT_H_ENABLE                             RT_BIT(7)

/* Volume Knob Control (7.3.3.29). */
#define CODEC_F0F_IS_DIRECT                                RT_BIT(7)
#define CODEC_F0F_VOLUME                                   (0x7F)

/* Unsolicited enabled (7.3.3.14). */
#define CODEC_MAKE_F08(enable, tag) ((((enable) & 1) << 7) | ((tag) & 0x3F))

/* Converter formats (7.3.3.8) and (3.7.1). */
/* This is the same format as SDnFMT. */
#define CODEC_MAKE_A                                       HDA_SDFMT_MAKE

#define CODEC_A_TYPE                                       HDA_SDFMT_TYPE
#define CODEC_A_TYPE_PCM                                   HDA_SDFMT_TYPE_PCM
#define CODEC_A_TYPE_NON_PCM                               HDA_SDFMT_TYPE_NON_PCM

#define CODEC_A_BASE                                       HDA_SDFMT_BASE
#define CODEC_A_BASE_48KHZ                                 HDA_SDFMT_BASE_48KHZ
#define CODEC_A_BASE_44KHZ                                 HDA_SDFMT_BASE_44KHZ

/* Pin Sense (7.3.3.15). */
#define CODEC_MAKE_F09_ANALOG(fPresent, impedance)  \
(  (((fPresent) & 0x1) << 31)                       \
 | (((impedance) & UINT32_C(0x7FFFFFFF))))
#define CODEC_F09_ANALOG_NA    UINT32_C(0x7FFFFFFF)
#define CODEC_MAKE_F09_DIGITAL(fPresent, fELDValid) \
(   (((fPresent)  & UINT32_C(0x1)) << 31)                      \
  | (((fELDValid) & UINT32_C(0x1)) << 30))

#define CODEC_MAKE_F0C(lrswap, eapd, btl) ((((lrswap) & 1) << 2) | (((eapd) & 1) << 1) | ((btl) & 1))
#define CODEC_FOC_IS_LRSWAP(f0c)                           RT_BOOL((f0c) & RT_BIT(2))
#define CODEC_FOC_IS_EAPD(f0c)                             RT_BOOL((f0c) & RT_BIT(1))
#define CODEC_FOC_IS_BTL(f0c)                              RT_BOOL((f0c) & RT_BIT(0))
/* HDA spec 7.3.3.31 defines layout of configuration registers/verbs (0xF1C) */
/* Configuration's port connection */
#define CODEC_F1C_PORT_MASK                                (0x3)
#define CODEC_F1C_PORT_SHIFT                               (30)

#define CODEC_F1C_PORT_COMPLEX                             (0x0)
#define CODEC_F1C_PORT_NO_PHYS                             (0x1)
#define CODEC_F1C_PORT_FIXED                               (0x2)
#define CODEC_F1C_BOTH                                     (0x3)

/* Configuration default: connection */
#define CODEC_F1C_PORT_MASK                                (0x3)
#define CODEC_F1C_PORT_SHIFT                               (30)

/* Connected to a jack (1/8", ATAPI, ...). */
#define CODEC_F1C_PORT_COMPLEX                             (0x0)
/* No physical connection. */
#define CODEC_F1C_PORT_NO_PHYS                             (0x1)
/* Fixed function device (integrated speaker, integrated mic, ...). */
#define CODEC_F1C_PORT_FIXED                               (0x2)
/* Both, a jack and an internal device are attached. */
#define CODEC_F1C_BOTH                                     (0x3)

/* Configuration default: Location */
#define CODEC_F1C_LOCATION_MASK                            (0x3F)
#define CODEC_F1C_LOCATION_SHIFT                           (24)

/* [4:5] bits of location region means chassis attachment */
#define CODEC_F1C_LOCATION_PRIMARY_CHASSIS                 (0)
#define CODEC_F1C_LOCATION_INTERNAL                        RT_BIT(4)
#define CODEC_F1C_LOCATION_SECONDRARY_CHASSIS              RT_BIT(5)
#define CODEC_F1C_LOCATION_OTHER                           RT_BIT(5)

/* [0:3] bits of location region means geometry location attachment */
#define CODEC_F1C_LOCATION_NA                              (0)
#define CODEC_F1C_LOCATION_REAR                            (0x1)
#define CODEC_F1C_LOCATION_FRONT                           (0x2)
#define CODEC_F1C_LOCATION_LEFT                            (0x3)
#define CODEC_F1C_LOCATION_RIGTH                           (0x4)
#define CODEC_F1C_LOCATION_TOP                             (0x5)
#define CODEC_F1C_LOCATION_BOTTOM                          (0x6)
#define CODEC_F1C_LOCATION_SPECIAL_0                       (0x7)
#define CODEC_F1C_LOCATION_SPECIAL_1                       (0x8)
#define CODEC_F1C_LOCATION_SPECIAL_2                       (0x9)

/* Configuration default: Device type */
#define CODEC_F1C_DEVICE_MASK                              (0xF)
#define CODEC_F1C_DEVICE_SHIFT                             (20)
#define CODEC_F1C_DEVICE_LINE_OUT                          (0)
#define CODEC_F1C_DEVICE_SPEAKER                           (0x1)
#define CODEC_F1C_DEVICE_HP                                (0x2)
#define CODEC_F1C_DEVICE_CD                                (0x3)
#define CODEC_F1C_DEVICE_SPDIF_OUT                         (0x4)
#define CODEC_F1C_DEVICE_DIGITAL_OTHER_OUT                 (0x5)
#define CODEC_F1C_DEVICE_MODEM_LINE_SIDE                   (0x6)
#define CODEC_F1C_DEVICE_MODEM_HANDSET_SIDE                (0x7)
#define CODEC_F1C_DEVICE_LINE_IN                           (0x8)
#define CODEC_F1C_DEVICE_AUX                               (0x9)
#define CODEC_F1C_DEVICE_MIC                               (0xA)
#define CODEC_F1C_DEVICE_PHONE                             (0xB)
#define CODEC_F1C_DEVICE_SPDIF_IN                          (0xC)
#define CODEC_F1C_DEVICE_RESERVED                          (0xE)
#define CODEC_F1C_DEVICE_OTHER                             (0xF)

/* Configuration default: Connection type */
#define CODEC_F1C_CONNECTION_TYPE_MASK                     (0xF)
#define CODEC_F1C_CONNECTION_TYPE_SHIFT                    (16)

#define CODEC_F1C_CONNECTION_TYPE_UNKNOWN                  (0)
#define CODEC_F1C_CONNECTION_TYPE_1_8INCHES                (0x1)
#define CODEC_F1C_CONNECTION_TYPE_1_4INCHES                (0x2)
#define CODEC_F1C_CONNECTION_TYPE_ATAPI                    (0x3)
#define CODEC_F1C_CONNECTION_TYPE_RCA                      (0x4)
#define CODEC_F1C_CONNECTION_TYPE_OPTICAL                  (0x5)
#define CODEC_F1C_CONNECTION_TYPE_OTHER_DIGITAL            (0x6)
#define CODEC_F1C_CONNECTION_TYPE_ANALOG                   (0x7)
#define CODEC_F1C_CONNECTION_TYPE_DIN                      (0x8)
#define CODEC_F1C_CONNECTION_TYPE_XLR                      (0x9)
#define CODEC_F1C_CONNECTION_TYPE_RJ_11                    (0xA)
#define CODEC_F1C_CONNECTION_TYPE_COMBO                    (0xB)
#define CODEC_F1C_CONNECTION_TYPE_OTHER                    (0xF)

/* Configuration's color */
#define CODEC_F1C_COLOR_MASK                               (0xF)
#define CODEC_F1C_COLOR_SHIFT                              (12)
#define CODEC_F1C_COLOR_UNKNOWN                            (0)
#define CODEC_F1C_COLOR_BLACK                              (0x1)
#define CODEC_F1C_COLOR_GREY                               (0x2)
#define CODEC_F1C_COLOR_BLUE                               (0x3)
#define CODEC_F1C_COLOR_GREEN                              (0x4)
#define CODEC_F1C_COLOR_RED                                (0x5)
#define CODEC_F1C_COLOR_ORANGE                             (0x6)
#define CODEC_F1C_COLOR_YELLOW                             (0x7)
#define CODEC_F1C_COLOR_PURPLE                             (0x8)
#define CODEC_F1C_COLOR_PINK                               (0x9)
#define CODEC_F1C_COLOR_RESERVED_0                         (0xA)
#define CODEC_F1C_COLOR_RESERVED_1                         (0xB)
#define CODEC_F1C_COLOR_RESERVED_2                         (0xC)
#define CODEC_F1C_COLOR_RESERVED_3                         (0xD)
#define CODEC_F1C_COLOR_WHITE                              (0xE)
#define CODEC_F1C_COLOR_OTHER                              (0xF)

/* Configuration's misc */
#define CODEC_F1C_MISC_MASK                                (0xF)
#define CODEC_F1C_MISC_SHIFT                               (8)
#define CODEC_F1C_MISC_NONE                                0
#define CODEC_F1C_MISC_JACK_NO_PRESENCE_DETECT             RT_BIT(0)
#define CODEC_F1C_MISC_RESERVED_0                          RT_BIT(1)
#define CODEC_F1C_MISC_RESERVED_1                          RT_BIT(2)
#define CODEC_F1C_MISC_RESERVED_2                          RT_BIT(3)

/* Configuration default: Association */
#define CODEC_F1C_ASSOCIATION_MASK                         (0xF)
#define CODEC_F1C_ASSOCIATION_SHIFT                        (4)

/** Reserved; don't use. */
#define CODEC_F1C_ASSOCIATION_INVALID                      0x0
#define CODEC_F1C_ASSOCIATION_GROUP_0                      0x1
#define CODEC_F1C_ASSOCIATION_GROUP_1                      0x2
#define CODEC_F1C_ASSOCIATION_GROUP_2                      0x3
#define CODEC_F1C_ASSOCIATION_GROUP_3                      0x4
#define CODEC_F1C_ASSOCIATION_GROUP_4                      0x5
#define CODEC_F1C_ASSOCIATION_GROUP_5                      0x6
#define CODEC_F1C_ASSOCIATION_GROUP_6                      0x7
#define CODEC_F1C_ASSOCIATION_GROUP_7                      0x8
/* Note: Windows OSes will treat group 15 (0xF) as single PIN devices.
 *       The sequence number associated with that group then will be ignored. */
#define CODEC_F1C_ASSOCIATION_GROUP_15                     0xF

/* Configuration default: Association Sequence. */
#define CODEC_F1C_SEQ_MASK                                 (0xF)
#define CODEC_F1C_SEQ_SHIFT                                (0)

/* Implementation identification (7.3.3.30). */
#define CODEC_MAKE_F20(bmid, bsku, aid)     \
    (  (((bmid) & 0xFFFF) << 16)            \
     | (((bsku) & 0xFF) << 8)               \
     | (((aid) & 0xFF))                     \
    )

/* Macro definition helping in filling the configuration registers. */
#define CODEC_MAKE_F1C(port_connectivity, location, device, connection_type, color, misc, association, sequence)    \
    (  (((port_connectivity) & 0xF) << CODEC_F1C_PORT_SHIFT)            \
     | (((location)          & 0xF) << CODEC_F1C_LOCATION_SHIFT)        \
     | (((device)            & 0xF) << CODEC_F1C_DEVICE_SHIFT)          \
     | (((connection_type)   & 0xF) << CODEC_F1C_CONNECTION_TYPE_SHIFT) \
     | (((color)             & 0xF) << CODEC_F1C_COLOR_SHIFT)           \
     | (((misc)              & 0xF) << CODEC_F1C_MISC_SHIFT)            \
     | (((association)       & 0xF) << CODEC_F1C_ASSOCIATION_SHIFT)     \
     | (((sequence)          & 0xF)))


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** The F00 parameter length (in dwords). */
#define CODECNODE_F00_PARAM_LENGTH  20
/** The F02 parameter length (in dwords). */
#define CODECNODE_F02_PARAM_LENGTH  16

/**
 * Common (or core) codec node structure.
 */
typedef struct CODECCOMMONNODE
{
    /** The node's ID. */
    uint8_t         uID;
    /** The node's name. */
    char const     *pszName;
    /** The SDn ID this node is assigned to.
     *  0 means not assigned, 1 is SDn0. */
    uint8_t         uSD;
    /** The SDn's channel to use.
     *  Only valid if a valid SDn ID is set. */
    uint8_t         uChannel;
    /* PRM 5.3.6 */
    uint32_t au32F00_param[CODECNODE_F00_PARAM_LENGTH];
    uint32_t au32F02_param[CODECNODE_F02_PARAM_LENGTH];
} CODECCOMMONNODE;
typedef CODECCOMMONNODE *PCODECCOMMONNODE;
AssertCompile(CODECNODE_F00_PARAM_LENGTH == 20);  /* saved state */
AssertCompile(CODECNODE_F02_PARAM_LENGTH == 16); /* saved state */

/**
 * Compile time assertion on the expected node size.
 */
#define AssertNodeSize(a_Node, a_cParams) \
    AssertCompile((a_cParams) <= (60 + 6)); /* the max size - saved state */ \
    AssertCompile(   sizeof(a_Node) - sizeof(CODECCOMMONNODE)  \
                  == (((a_cParams) * sizeof(uint32_t) + sizeof(void *) - 1) & ~(sizeof(void *) - 1)) )

typedef struct ROOTCODECNODE
{
    CODECCOMMONNODE node;
} ROOTCODECNODE, *PROOTCODECNODE;
AssertNodeSize(ROOTCODECNODE, 0);

#define AMPLIFIER_SIZE 60
typedef uint32_t AMPLIFIER[AMPLIFIER_SIZE];
#define AMPLIFIER_IN    0
#define AMPLIFIER_OUT   1
#define AMPLIFIER_LEFT  1
#define AMPLIFIER_RIGHT 0
#define AMPLIFIER_REGISTER(amp, inout, side, index) ((amp)[30*(inout) + 15*(side) + (index)])
typedef struct DACNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F0d_param;
    uint32_t    u32F04_param;
    uint32_t    u32F05_param;
    uint32_t    u32F06_param;
    uint32_t    u32F0c_param;

    uint32_t    u32A_param;
    AMPLIFIER   B_params;

} DACNODE, *PDACNODE;
AssertNodeSize(DACNODE, 6 + 60);

typedef struct ADCNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F01_param;
    uint32_t    u32F03_param;
    uint32_t    u32F05_param;
    uint32_t    u32F06_param;
    uint32_t    u32F09_param;

    uint32_t    u32A_param;
    AMPLIFIER   B_params;
} ADCNODE, *PADCNODE;
AssertNodeSize(DACNODE, 6 + 60);

typedef struct SPDIFOUTNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F05_param;
    uint32_t    u32F06_param;
    uint32_t    u32F09_param;
    uint32_t    u32F0d_param;

    uint32_t    u32A_param;
    AMPLIFIER   B_params;
} SPDIFOUTNODE, *PSPDIFOUTNODE;
AssertNodeSize(SPDIFOUTNODE, 5 + 60);

typedef struct SPDIFINNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F05_param;
    uint32_t    u32F06_param;
    uint32_t    u32F09_param;
    uint32_t    u32F0d_param;

    uint32_t    u32A_param;
    AMPLIFIER   B_params;
} SPDIFINNODE, *PSPDIFINNODE;
AssertNodeSize(SPDIFINNODE, 5 + 60);

typedef struct AFGCODECNODE
{
    CODECCOMMONNODE node;
    uint32_t  u32F05_param;
    uint32_t  u32F08_param;
    uint32_t  u32F17_param;
    uint32_t  u32F20_param;
} AFGCODECNODE, *PAFGCODECNODE;
AssertNodeSize(AFGCODECNODE, 4);

typedef struct PORTNODE
{
    CODECCOMMONNODE node;
    uint32_t u32F01_param;
    uint32_t u32F07_param;
    uint32_t u32F08_param;
    uint32_t u32F09_param;
    uint32_t u32F1c_param;
    AMPLIFIER   B_params;
} PORTNODE, *PPORTNODE;
AssertNodeSize(PORTNODE, 5 + 60);

typedef struct DIGOUTNODE
{
    CODECCOMMONNODE node;
    uint32_t u32F01_param;
    uint32_t u32F05_param;
    uint32_t u32F07_param;
    uint32_t u32F08_param;
    uint32_t u32F09_param;
    uint32_t u32F1c_param;
} DIGOUTNODE, *PDIGOUTNODE;
AssertNodeSize(DIGOUTNODE, 6);

typedef struct DIGINNODE
{
    CODECCOMMONNODE node;
    uint32_t u32F05_param;
    uint32_t u32F07_param;
    uint32_t u32F08_param;
    uint32_t u32F09_param;
    uint32_t u32F0c_param;
    uint32_t u32F1c_param;
    uint32_t u32F1e_param;
} DIGINNODE, *PDIGINNODE;
AssertNodeSize(DIGINNODE, 7);

typedef struct ADCMUXNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F01_param;

    uint32_t    u32A_param;
    AMPLIFIER   B_params;
} ADCMUXNODE, *PADCMUXNODE;
AssertNodeSize(ADCMUXNODE, 2 + 60);

typedef struct PCBEEPNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F07_param;
    uint32_t    u32F0a_param;

    uint32_t    u32A_param;
    AMPLIFIER   B_params;
    uint32_t    u32F1c_param;
} PCBEEPNODE, *PPCBEEPNODE;
AssertNodeSize(PCBEEPNODE, 3 + 60 + 1);

typedef struct CDNODE
{
    CODECCOMMONNODE node;
    uint32_t u32F07_param;
    uint32_t u32F1c_param;
} CDNODE, *PCDNODE;
AssertNodeSize(CDNODE, 2);

typedef struct VOLUMEKNOBNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F08_param;
    uint32_t    u32F0f_param;
} VOLUMEKNOBNODE, *PVOLUMEKNOBNODE;
AssertNodeSize(VOLUMEKNOBNODE, 2);

typedef struct ADCVOLNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F0c_param;
    uint32_t    u32F01_param;
    uint32_t    u32A_params;
    AMPLIFIER   B_params;
} ADCVOLNODE, *PADCVOLNODE;
AssertNodeSize(ADCVOLNODE, 3 + 60);

typedef struct RESNODE
{
    CODECCOMMONNODE node;
    uint32_t    u32F05_param;
    uint32_t    u32F06_param;
    uint32_t    u32F07_param;
    uint32_t    u32F1c_param;

    uint32_t    u32A_param;
} RESNODE, *PRESNODE;
AssertNodeSize(RESNODE, 5);

/**
 * Used for the saved state.
 */
typedef struct CODECSAVEDSTATENODE
{
    CODECCOMMONNODE Core;
    uint32_t        au32Params[60 + 6];
} CODECSAVEDSTATENODE;
AssertNodeSize(CODECSAVEDSTATENODE, 60 + 6);

typedef union CODECNODE
{
    CODECCOMMONNODE node;
    ROOTCODECNODE   root;
    AFGCODECNODE    afg;
    DACNODE         dac;
    ADCNODE         adc;
    SPDIFOUTNODE    spdifout;
    SPDIFINNODE     spdifin;
    PORTNODE        port;
    DIGOUTNODE      digout;
    DIGINNODE       digin;
    ADCMUXNODE      adcmux;
    PCBEEPNODE      pcbeep;
    CDNODE          cdnode;
    VOLUMEKNOBNODE  volumeKnob;
    ADCVOLNODE      adcvol;
    RESNODE         reserved;
    CODECSAVEDSTATENODE SavedState;
} CODECNODE, *PCODECNODE;
AssertNodeSize(CODECNODE, 60 + 6);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/* STAC9220 - Nodes IDs / names. */
#define STAC9220_NID_ROOT                                  0x0  /* Root node */
#define STAC9220_NID_AFG                                   0x1  /* Audio Configuration Group */
#define STAC9220_NID_DAC0                                  0x2  /* Out */
#define STAC9220_NID_DAC1                                  0x3  /* Out */
#define STAC9220_NID_DAC2                                  0x4  /* Out */
#define STAC9220_NID_DAC3                                  0x5  /* Out */
#define STAC9220_NID_ADC0                                  0x6  /* In */
#define STAC9220_NID_ADC1                                  0x7  /* In */
#define STAC9220_NID_SPDIF_OUT                             0x8  /* Out */
#define STAC9220_NID_SPDIF_IN                              0x9  /* In */
/** Also known as PIN_A. */
#define STAC9220_NID_PIN_HEADPHONE0                        0xA  /* In, Out */
#define STAC9220_NID_PIN_B                                 0xB  /* In, Out */
#define STAC9220_NID_PIN_C                                 0xC  /* In, Out */
/** Also known as PIN D. */
#define STAC9220_NID_PIN_HEADPHONE1                        0xD  /* In, Out */
#define STAC9220_NID_PIN_E                                 0xE  /* In */
#define STAC9220_NID_PIN_F                                 0xF  /* In, Out */
/** Also known as DIGOUT0. */
#define STAC9220_NID_PIN_SPDIF_OUT                         0x10 /* Out */
/** Also known as DIGIN. */
#define STAC9220_NID_PIN_SPDIF_IN                          0x11 /* In */
#define STAC9220_NID_ADC0_MUX                              0x12 /* In */
#define STAC9220_NID_ADC1_MUX                              0x13 /* In */
#define STAC9220_NID_PCBEEP                                0x14 /* Out */
#define STAC9220_NID_PIN_CD                                0x15 /* In */
#define STAC9220_NID_VOL_KNOB                              0x16
#define STAC9220_NID_AMP_ADC0                              0x17 /* In */
#define STAC9220_NID_AMP_ADC1                              0x18 /* In */
/* Only for STAC9221. */
#define STAC9221_NID_ADAT_OUT                              0x19 /* Out */
#define STAC9221_NID_I2S_OUT                               0x1A /* Out */
#define STAC9221_NID_PIN_I2S_OUT                           0x1B /* Out */

/** Number of total nodes emulated. */
#define STAC9221_NUM_NODES                                 0x1C

/* STAC9220 - Referenced through STAC9220WIDGET in the constructor below. */
static uint8_t const g_abStac9220Ports[]      = { STAC9220_NID_PIN_HEADPHONE0, STAC9220_NID_PIN_B, STAC9220_NID_PIN_C, STAC9220_NID_PIN_HEADPHONE1, STAC9220_NID_PIN_E, STAC9220_NID_PIN_F, 0 };
static uint8_t const g_abStac9220Dacs[]       = { STAC9220_NID_DAC0, STAC9220_NID_DAC1, STAC9220_NID_DAC2, STAC9220_NID_DAC3, 0 };
static uint8_t const g_abStac9220Adcs[]       = { STAC9220_NID_ADC0, STAC9220_NID_ADC1, 0 };
static uint8_t const g_abStac9220SpdifOuts[]  = { STAC9220_NID_SPDIF_OUT, 0 };
static uint8_t const g_abStac9220SpdifIns[]   = { STAC9220_NID_SPDIF_IN, 0 };
static uint8_t const g_abStac9220DigOutPins[] = { STAC9220_NID_PIN_SPDIF_OUT, 0 };
static uint8_t const g_abStac9220DigInPins[]  = { STAC9220_NID_PIN_SPDIF_IN, 0 };
static uint8_t const g_abStac9220AdcVols[]    = { STAC9220_NID_AMP_ADC0, STAC9220_NID_AMP_ADC1, 0 };
static uint8_t const g_abStac9220AdcMuxs[]    = { STAC9220_NID_ADC0_MUX, STAC9220_NID_ADC1_MUX, 0 };
static uint8_t const g_abStac9220Pcbeeps[]    = { STAC9220_NID_PCBEEP, 0 };
static uint8_t const g_abStac9220Cds[]        = { STAC9220_NID_PIN_CD, 0 };
static uint8_t const g_abStac9220VolKnobs[]   = { STAC9220_NID_VOL_KNOB, 0 };
/* STAC 9221. */
/** @todo Is STAC9220_NID_SPDIF_IN really correct for reserved nodes? */
static uint8_t const g_abStac9220Reserveds[]  = { STAC9220_NID_SPDIF_IN, STAC9221_NID_ADAT_OUT, STAC9221_NID_I2S_OUT, STAC9221_NID_PIN_I2S_OUT, 0 };

/** SSM description of a CODECNODE. */
static SSMFIELD const g_aCodecNodeFields[] =
{
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, Core.uID),
    SSMFIELD_ENTRY_PAD_HC_AUTO(3, 3),
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, Core.au32F00_param),
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, Core.au32F02_param),
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, au32Params),
    SSMFIELD_ENTRY_TERM()
};

/** Backward compatibility with v1 of the CODECNODE. */
static SSMFIELD const g_aCodecNodeFieldsV1[] =
{
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, Core.uID),
    SSMFIELD_ENTRY_PAD_HC_AUTO(3, 7),
    SSMFIELD_ENTRY_OLD_HCPTR(Core.name),
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, Core.au32F00_param),
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, Core.au32F02_param),
    SSMFIELD_ENTRY(     CODECSAVEDSTATENODE, au32Params),
    SSMFIELD_ENTRY_TERM()
};



#if 0 /* unused */
static DECLCALLBACK(void) stac9220DbgNodes(PHDACODEC pThis, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pszArgs);
    for (uint8_t i = 1; i < pThis->cTotalNodes; i++)
    {
        PCODECNODE pNode = &pThis->paNodes[i];
        AMPLIFIER *pAmp = &pNode->dac.B_params;

        uint8_t lVol = AMPLIFIER_REGISTER(*pAmp, AMPLIFIER_OUT, AMPLIFIER_LEFT, 0) & 0x7f;
        uint8_t rVol = AMPLIFIER_REGISTER(*pAmp, AMPLIFIER_OUT, AMPLIFIER_RIGHT, 0) & 0x7f;

        pHlp->pfnPrintf(pHlp, "0x%x: lVol=%RU8, rVol=%RU8\n", i, lVol, rVol);
    }
}
#endif

/**
 * Resets the codec with all its connected nodes.
 *
 * @param   pThis               HDA codec to reset.
 */
static DECLCALLBACK(void) stac9220Reset(PHDACODEC pThis)
{
    AssertPtrReturnVoid(pThis->paNodes);
    AssertPtrReturnVoid(pThis->pfnNodeReset);

    LogRel(("HDA: Codec reset\n"));

    pThis->fInReset = true;

    for (uint8_t i = 0; i < pThis->cTotalNodes; i++)
        pThis->pfnNodeReset(pThis, i, &pThis->paNodes[i]);

    pThis->fInReset = false;
}

/**
 * Resets a single node of the codec.
 *
 * @returns IPRT status code.
 * @param   pThis               HDA codec of node to reset.
 * @param   uNID                Node ID to set node to.
 * @param   pNode               Node to reset.
 */
static DECLCALLBACK(int) stac9220ResetNode(PHDACODEC pThis, uint8_t uNID, PCODECNODE pNode)
{
    LogFlowFunc(("NID=0x%x (%RU8)\n", uNID, uNID));

    if (   !pThis->fInReset
        && (   uNID != STAC9220_NID_ROOT
            && uNID != STAC9220_NID_AFG)
       )
    {
        RT_ZERO(pNode->node);
    }

    /* Set common parameters across all nodes. */
    pNode->node.uID = uNID;
    pNode->node.uSD = 0;

    switch (uNID)
    {
        /* Root node. */
        case STAC9220_NID_ROOT:
        {
            /* Set the revision ID. */
            pNode->root.node.au32F00_param[0x02] = CODEC_MAKE_F00_02(0x1, 0x0, 0x3, 0x4, 0x0, 0x1);
            break;
        }

        /*
         * AFG (Audio Function Group).
         */
        case STAC9220_NID_AFG:
        {
            pNode->afg.node.au32F00_param[0x08] = CODEC_MAKE_F00_08(1, 0xd, 0xd);
            /* We set the AFG's PCM capabitilies fixed to 44.1kHz, 16-bit signed. */
            pNode->afg.node.au32F00_param[0x0A] = CODEC_F00_0A_44_1KHZ | CODEC_F00_0A_16_BIT;
            pNode->afg.node.au32F00_param[0x0B] = CODEC_F00_0B_PCM;
            pNode->afg.node.au32F00_param[0x0C] = CODEC_MAKE_F00_0C(0x17)
                                                | CODEC_F00_0C_CAP_BALANCED_IO
                                                | CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_OUTPUT
                                                | CODEC_F00_0C_CAP_PRESENCE_DETECT
                                                | CODEC_F00_0C_CAP_TRIGGER_REQUIRED
                                                | CODEC_F00_0C_CAP_IMPENDANCE_SENSE;

            /* Default input amplifier capabilities. */
            pNode->node.au32F00_param[0x0D] = CODEC_MAKE_F00_0D(CODEC_AMP_CAP_MUTE,
                                                                CODEC_AMP_STEP_SIZE,
                                                                CODEC_AMP_NUM_STEPS,
                                                                CODEC_AMP_OFF_INITIAL);
            /* Default output amplifier capabilities. */
            pNode->node.au32F00_param[0x12] = CODEC_MAKE_F00_12(CODEC_AMP_CAP_MUTE,
                                                                CODEC_AMP_STEP_SIZE,
                                                                CODEC_AMP_NUM_STEPS,
                                                                CODEC_AMP_OFF_INITIAL);

            pNode->afg.node.au32F00_param[0x11] = CODEC_MAKE_F00_11(1, 1, 0, 0, 4);
            pNode->afg.node.au32F00_param[0x0F] = CODEC_F00_0F_D3
                                                | CODEC_F00_0F_D2
                                                | CODEC_F00_0F_D1
                                                | CODEC_F00_0F_D0;

            pNode->afg.u32F05_param = CODEC_MAKE_F05(0, 0, 0, CODEC_F05_D2, CODEC_F05_D2); /* PS-Act: D2, PS->Set D2. */
            pNode->afg.u32F08_param = 0;
            pNode->afg.u32F17_param = 0;
            break;
        }

        /*
         * DACs.
         */
        case STAC9220_NID_DAC0: /* DAC0: Headphones 0 + 1 */
        case STAC9220_NID_DAC1: /* DAC1: PIN C */
        case STAC9220_NID_DAC2: /* DAC2: PIN B */
        case STAC9220_NID_DAC3: /* DAC3: PIN F */
        {
            pNode->dac.u32A_param = CODEC_MAKE_A(HDA_SDFMT_TYPE_PCM, HDA_SDFMT_BASE_44KHZ,
                                                 HDA_SDFMT_MULT_1X, HDA_SDFMT_DIV_1X, HDA_SDFMT_16_BIT,
                                                 HDA_SDFMT_CHAN_STEREO);

            /* 7.3.4.6: Audio widget capabilities. */
            pNode->dac.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_OUTPUT, 13, 0)
                                               | CODEC_F00_09_CAP_L_R_SWAP
                                               | CODEC_F00_09_CAP_POWER_CTRL
                                               | CODEC_F00_09_CAP_OUT_AMP_PRESENT
                                               | CODEC_F00_09_CAP_STEREO;

            /* Connection list; must be 0 if the only connection for the widget is
             * to the High Definition Audio Link. */
            pNode->dac.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 0 /* Entries */);

            pNode->dac.u32F05_param = CODEC_MAKE_F05(0, 0, 0, CODEC_F05_D3, CODEC_F05_D3);

            RT_ZERO(pNode->dac.B_params);
            AMPLIFIER_REGISTER(pNode->dac.B_params, AMPLIFIER_OUT, AMPLIFIER_LEFT,  0) = 0x7F | RT_BIT(7);
            AMPLIFIER_REGISTER(pNode->dac.B_params, AMPLIFIER_OUT, AMPLIFIER_RIGHT, 0) = 0x7F | RT_BIT(7);
            break;
        }

        /*
         * ADCs.
         */
        case STAC9220_NID_ADC0: /* Analog input. */
        {
            pNode->node.au32F02_param[0] = STAC9220_NID_AMP_ADC0;
            goto adc_init;
        }

        case STAC9220_NID_ADC1: /* Analog input (CD). */
        {
            pNode->node.au32F02_param[0] = STAC9220_NID_AMP_ADC1;

            /* Fall through is intentional. */
        adc_init:

            pNode->adc.u32A_param   = CODEC_MAKE_A(HDA_SDFMT_TYPE_PCM, HDA_SDFMT_BASE_44KHZ,
                                                   HDA_SDFMT_MULT_1X, HDA_SDFMT_DIV_1X, HDA_SDFMT_16_BIT,
                                                   HDA_SDFMT_CHAN_STEREO);

            pNode->adc.u32F03_param = RT_BIT(0);
            pNode->adc.u32F05_param = CODEC_MAKE_F05(0, 0, 0, CODEC_F05_D3, CODEC_F05_D3); /* PS-Act: D3 Set: D3 */

            pNode->adc.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_INPUT, 0xD, 0)
                                               | CODEC_F00_09_CAP_POWER_CTRL
                                               | CODEC_F00_09_CAP_CONNECTION_LIST
                                               | CODEC_F00_09_CAP_PROC_WIDGET
                                               | CODEC_F00_09_CAP_STEREO;
            /* Connection list entries. */
            pNode->adc.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 1 /* Entries */);
            break;
        }

        /*
         * SP/DIF In/Out.
         */
        case STAC9220_NID_SPDIF_OUT:
        {
            pNode->spdifout.u32A_param   = CODEC_MAKE_A(HDA_SDFMT_TYPE_PCM, HDA_SDFMT_BASE_44KHZ,
                                                        HDA_SDFMT_MULT_1X, HDA_SDFMT_DIV_1X, HDA_SDFMT_16_BIT,
                                                        HDA_SDFMT_CHAN_STEREO);
            pNode->spdifout.u32F06_param = 0;
            pNode->spdifout.u32F0d_param = 0;

            pNode->spdifout.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_OUTPUT, 4, 0)
                                                    | CODEC_F00_09_CAP_DIGITAL
                                                    | CODEC_F00_09_CAP_FMT_OVERRIDE
                                                    | CODEC_F00_09_CAP_STEREO;

            /* Use a fixed format from AFG. */
            pNode->spdifout.node.au32F00_param[0xA] = pThis->paNodes[STAC9220_NID_AFG].node.au32F00_param[0xA];
            pNode->spdifout.node.au32F00_param[0xB] = CODEC_F00_0B_PCM;
            break;
        }

        case STAC9220_NID_SPDIF_IN:
        {
            pNode->spdifin.u32A_param = CODEC_MAKE_A(HDA_SDFMT_TYPE_PCM, HDA_SDFMT_BASE_44KHZ,
                                                     HDA_SDFMT_MULT_1X, HDA_SDFMT_DIV_1X, HDA_SDFMT_16_BIT,
                                                     HDA_SDFMT_CHAN_STEREO);

            pNode->spdifin.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_INPUT, 4, 0)
                                                   | CODEC_F00_09_CAP_DIGITAL
                                                   | CODEC_F00_09_CAP_CONNECTION_LIST
                                                   | CODEC_F00_09_CAP_FMT_OVERRIDE
                                                   | CODEC_F00_09_CAP_STEREO;

            /* Use a fixed format from AFG. */
            pNode->spdifin.node.au32F00_param[0xA] = pThis->paNodes[STAC9220_NID_AFG].node.au32F00_param[0xA];
            pNode->spdifin.node.au32F00_param[0xB] = CODEC_F00_0B_PCM;

            /* Connection list entries. */
            pNode->spdifin.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 1 /* Entries */);
            pNode->spdifin.node.au32F02_param[0]   = 0x11;
            break;
        }

        /*
         * PINs / Ports.
         */
        case STAC9220_NID_PIN_HEADPHONE0: /* Port A: Headphone in/out (front). */
        {
            pNode->port.u32F09_param = CODEC_MAKE_F09_ANALOG(0 /*fPresent*/, CODEC_F09_ANALOG_NA);

            pNode->port.node.au32F00_param[0xC] = CODEC_MAKE_F00_0C(0x17)
                                                | CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_OUTPUT
                                                | CODEC_F00_0C_CAP_HEADPHONE_AMP
                                                | CODEC_F00_0C_CAP_PRESENCE_DETECT
                                                | CODEC_F00_0C_CAP_TRIGGER_REQUIRED;

            /* Connection list entry 0: Goes to DAC0. */
            pNode->port.node.au32F02_param[0]   = STAC9220_NID_DAC0;

            if (!pThis->fInReset)
                pNode->port.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                          CODEC_F1C_LOCATION_FRONT,
                                                          CODEC_F1C_DEVICE_HP,
                                                          CODEC_F1C_CONNECTION_TYPE_1_8INCHES,
                                                          CODEC_F1C_COLOR_GREEN,
                                                          CODEC_F1C_MISC_NONE,
                                                          CODEC_F1C_ASSOCIATION_GROUP_1, 0x0 /* Seq */);
            goto port_init;
        }

        case STAC9220_NID_PIN_B: /* Port B: Rear CLFE (Center / Subwoofer). */
        {
            pNode->port.u32F09_param = CODEC_MAKE_F09_ANALOG(1 /*fPresent*/, CODEC_F09_ANALOG_NA);

            pNode->port.node.au32F00_param[0xC] = CODEC_MAKE_F00_0C(0x17)
                                                | CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_OUTPUT
                                                | CODEC_F00_0C_CAP_PRESENCE_DETECT
                                                | CODEC_F00_0C_CAP_TRIGGER_REQUIRED;

            /* Connection list entry 0: Goes to DAC2. */
            pNode->port.node.au32F02_param[0]   = STAC9220_NID_DAC2;

            if (!pThis->fInReset)
                pNode->port.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                          CODEC_F1C_LOCATION_REAR,
                                                          CODEC_F1C_DEVICE_SPEAKER,
                                                          CODEC_F1C_CONNECTION_TYPE_1_8INCHES,
                                                          CODEC_F1C_COLOR_BLACK,
                                                          CODEC_F1C_MISC_NONE,
                                                          CODEC_F1C_ASSOCIATION_GROUP_0, 0x1 /* Seq */);
            goto port_init;
        }

        case STAC9220_NID_PIN_C: /* Rear Speaker. */
        {
            pNode->port.u32F09_param = CODEC_MAKE_F09_ANALOG(1 /*fPresent*/, CODEC_F09_ANALOG_NA);

            pNode->port.node.au32F00_param[0xC] = CODEC_MAKE_F00_0C(0x17)
                                                | CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_OUTPUT
                                                | CODEC_F00_0C_CAP_PRESENCE_DETECT
                                                | CODEC_F00_0C_CAP_TRIGGER_REQUIRED;

            /* Connection list entry 0: Goes to DAC1. */
            pNode->port.node.au32F02_param[0x0] = STAC9220_NID_DAC1;

            if (!pThis->fInReset)
                pNode->port.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                          CODEC_F1C_LOCATION_REAR,
                                                          CODEC_F1C_DEVICE_SPEAKER,
                                                          CODEC_F1C_CONNECTION_TYPE_1_8INCHES,
                                                          CODEC_F1C_COLOR_GREEN,
                                                          CODEC_F1C_MISC_NONE,
                                                          CODEC_F1C_ASSOCIATION_GROUP_0, 0x0 /* Seq */);
            goto port_init;
        }

        case STAC9220_NID_PIN_HEADPHONE1: /* Also known as PIN_D. */
        {
            pNode->port.u32F09_param = CODEC_MAKE_F09_ANALOG(1 /*fPresent*/, CODEC_F09_ANALOG_NA);

            pNode->port.node.au32F00_param[0xC] = CODEC_MAKE_F00_0C(0x17)
                                                | CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_OUTPUT
                                                | CODEC_F00_0C_CAP_HEADPHONE_AMP
                                                | CODEC_F00_0C_CAP_PRESENCE_DETECT
                                                | CODEC_F00_0C_CAP_TRIGGER_REQUIRED;

            /* Connection list entry 0: Goes to DAC1. */
            pNode->port.node.au32F02_param[0x0] = STAC9220_NID_DAC0;

            if (!pThis->fInReset)
                pNode->port.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                          CODEC_F1C_LOCATION_FRONT,
                                                          CODEC_F1C_DEVICE_MIC,
                                                          CODEC_F1C_CONNECTION_TYPE_1_8INCHES,
                                                          CODEC_F1C_COLOR_PINK,
                                                          CODEC_F1C_MISC_NONE,
                                                          CODEC_F1C_ASSOCIATION_GROUP_15, 0x0 /* Ignored */);
            /* Fall through is intentional. */

        port_init:

            pNode->port.u32F07_param = CODEC_F07_IN_ENABLE
                                     | CODEC_F07_OUT_ENABLE;
            pNode->port.u32F08_param = 0;

            pNode->port.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 0, 0)
                                                | CODEC_F00_09_CAP_CONNECTION_LIST
                                                | CODEC_F00_09_CAP_UNSOL
                                                | CODEC_F00_09_CAP_STEREO;
            /* Connection list entries. */
            pNode->port.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 1 /* Entries */);
            break;
        }

        case STAC9220_NID_PIN_E:
        {
            pNode->port.u32F07_param = CODEC_F07_IN_ENABLE;
            pNode->port.u32F08_param = 0;
            /* If Line in is reported as enabled, OS X sees no speakers! Windows does
             * not care either way, although Linux does.
             */
            pNode->port.u32F09_param = CODEC_MAKE_F09_ANALOG(0 /* fPresent */, 0);

            pNode->port.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 0, 0)
                                                | CODEC_F00_09_CAP_UNSOL
                                                | CODEC_F00_09_CAP_STEREO;

            pNode->port.node.au32F00_param[0xC] = CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_PRESENCE_DETECT;

            if (!pThis->fInReset)
                pNode->port.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                          CODEC_F1C_LOCATION_REAR,
                                                          CODEC_F1C_DEVICE_LINE_IN,
                                                          CODEC_F1C_CONNECTION_TYPE_1_8INCHES,
                                                          CODEC_F1C_COLOR_BLUE,
                                                          CODEC_F1C_MISC_NONE,
                                                          CODEC_F1C_ASSOCIATION_GROUP_4, 0x1 /* Seq */);
            break;
        }

        case STAC9220_NID_PIN_F:
        {
            pNode->port.u32F07_param = CODEC_F07_IN_ENABLE | CODEC_F07_OUT_ENABLE;
            pNode->port.u32F08_param = 0;
            pNode->port.u32F09_param = CODEC_MAKE_F09_ANALOG(1 /* fPresent */, CODEC_F09_ANALOG_NA);

            pNode->port.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 0, 0)
                                                | CODEC_F00_09_CAP_CONNECTION_LIST
                                                | CODEC_F00_09_CAP_UNSOL
                                                | CODEC_F00_09_CAP_OUT_AMP_PRESENT
                                                | CODEC_F00_09_CAP_STEREO;

            pNode->port.node.au32F00_param[0xC] = CODEC_F00_0C_CAP_INPUT
                                                | CODEC_F00_0C_CAP_OUTPUT;

            /* Connection list entry 0: Goes to DAC3. */
            pNode->port.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 1 /* Entries */);
            pNode->port.node.au32F02_param[0x0] = STAC9220_NID_DAC3;

            if (!pThis->fInReset)
                pNode->port.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                          CODEC_F1C_LOCATION_INTERNAL,
                                                          CODEC_F1C_DEVICE_SPEAKER,
                                                          CODEC_F1C_CONNECTION_TYPE_1_8INCHES,
                                                          CODEC_F1C_COLOR_ORANGE,
                                                          CODEC_F1C_MISC_NONE,
                                                          CODEC_F1C_ASSOCIATION_GROUP_0, 0x2 /* Seq */);
            break;
        }

        case STAC9220_NID_PIN_SPDIF_OUT: /* Rear SPDIF Out. */
        {
            pNode->digout.u32F07_param = CODEC_F07_OUT_ENABLE;
            pNode->digout.u32F09_param = 0;

            pNode->digout.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 0, 0)
                                                  | CODEC_F00_09_CAP_DIGITAL
                                                  | CODEC_F00_09_CAP_CONNECTION_LIST
                                                  | CODEC_F00_09_CAP_STEREO;
            pNode->digout.node.au32F00_param[0xC] = CODEC_F00_0C_CAP_OUTPUT;

            /* Connection list entries. */
            pNode->digout.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 3 /* Entries */);
            pNode->digout.node.au32F02_param[0x0] = RT_MAKE_U32_FROM_U8(STAC9220_NID_SPDIF_OUT,
                                                                        STAC9220_NID_AMP_ADC0, STAC9221_NID_ADAT_OUT, 0);
            if (!pThis->fInReset)
                pNode->digout.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                            CODEC_F1C_LOCATION_REAR,
                                                            CODEC_F1C_DEVICE_SPDIF_OUT,
                                                            CODEC_F1C_CONNECTION_TYPE_DIN,
                                                            CODEC_F1C_COLOR_BLACK,
                                                            CODEC_F1C_MISC_NONE,
                                                            CODEC_F1C_ASSOCIATION_GROUP_2, 0x0 /* Seq */);
            break;
        }

        case STAC9220_NID_PIN_SPDIF_IN:
        {
            pNode->digin.u32F05_param = CODEC_MAKE_F05(0, 0, 0, CODEC_F05_D3, CODEC_F05_D3); /* PS-Act: D3 -> D3 */
            pNode->digin.u32F07_param = CODEC_F07_IN_ENABLE;
            pNode->digin.u32F08_param = 0;
            pNode->digin.u32F09_param = CODEC_MAKE_F09_DIGITAL(0, 0);
            pNode->digin.u32F0c_param = 0;

            pNode->digin.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 3, 0)
                                                 | CODEC_F00_09_CAP_POWER_CTRL
                                                 | CODEC_F00_09_CAP_DIGITAL
                                                 | CODEC_F00_09_CAP_UNSOL
                                                 | CODEC_F00_09_CAP_STEREO;

            pNode->digin.node.au32F00_param[0xC] = CODEC_F00_0C_CAP_EAPD
                                                 | CODEC_F00_0C_CAP_INPUT
                                                 | CODEC_F00_0C_CAP_PRESENCE_DETECT;
            if (!pThis->fInReset)
                pNode->digin.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_COMPLEX,
                                                           CODEC_F1C_LOCATION_REAR,
                                                           CODEC_F1C_DEVICE_SPDIF_IN,
                                                           CODEC_F1C_CONNECTION_TYPE_OTHER_DIGITAL,
                                                           CODEC_F1C_COLOR_BLACK,
                                                           CODEC_F1C_MISC_NONE,
                                                           CODEC_F1C_ASSOCIATION_GROUP_5, 0x0 /* Seq */);
            break;
        }

        case STAC9220_NID_ADC0_MUX:
        {
            pNode->adcmux.u32F01_param = 0; /* Connection select control index (STAC9220_NID_PIN_E). */
            goto adcmux_init;
        }

        case STAC9220_NID_ADC1_MUX:
        {
            pNode->adcmux.u32F01_param = 1; /* Connection select control index (STAC9220_NID_PIN_CD). */
            /* Fall through is intentional. */

        adcmux_init:

            pNode->adcmux.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_SELECTOR, 0, 0)
                                                  | CODEC_F00_09_CAP_CONNECTION_LIST
                                                  | CODEC_F00_09_CAP_AMP_FMT_OVERRIDE
                                                  | CODEC_F00_09_CAP_OUT_AMP_PRESENT
                                                  | CODEC_F00_09_CAP_STEREO;

            pNode->adcmux.node.au32F00_param[0xD] = CODEC_MAKE_F00_0D(0, 27, 4, 0);

            /* Connection list entries. */
            pNode->adcmux.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 7 /* Entries */);
            pNode->adcmux.node.au32F02_param[0x0] = RT_MAKE_U32_FROM_U8(STAC9220_NID_PIN_E,
                                                                        STAC9220_NID_PIN_CD,
                                                                        STAC9220_NID_PIN_F,
                                                                        STAC9220_NID_PIN_B);
            pNode->adcmux.node.au32F02_param[0x4] = RT_MAKE_U32_FROM_U8(STAC9220_NID_PIN_C,
                                                                        STAC9220_NID_PIN_HEADPHONE1,
                                                                        STAC9220_NID_PIN_HEADPHONE0,
                                                                        0x0 /* Unused */);

            /* STAC 9220 v10 6.21-22.{4,5} both(left and right) out amplifiers initialized with 0. */
            RT_ZERO(pNode->adcmux.B_params);
            break;
        }

        case STAC9220_NID_PCBEEP:
        {
            pNode->pcbeep.u32F0a_param = 0;

            pNode->pcbeep.node.au32F00_param[0x9]  = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_BEEP_GEN, 0, 0)
                                                   | CODEC_F00_09_CAP_AMP_FMT_OVERRIDE
                                                   | CODEC_F00_09_CAP_OUT_AMP_PRESENT;
            pNode->pcbeep.node.au32F00_param[0xD]  = CODEC_MAKE_F00_0D(0, 17, 3, 3);

            RT_ZERO(pNode->pcbeep.B_params);
            break;
        }

        case STAC9220_NID_PIN_CD:
        {
            pNode->cdnode.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 0, 0)
                                                  | CODEC_F00_09_CAP_STEREO;
            pNode->cdnode.node.au32F00_param[0xC] = CODEC_F00_0C_CAP_INPUT;

            if (!pThis->fInReset)
                pNode->cdnode.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_FIXED,
                                                            CODEC_F1C_LOCATION_INTERNAL,
                                                            CODEC_F1C_DEVICE_CD,
                                                            CODEC_F1C_CONNECTION_TYPE_ATAPI,
                                                            CODEC_F1C_COLOR_UNKNOWN,
                                                            CODEC_F1C_MISC_NONE,
                                                            CODEC_F1C_ASSOCIATION_GROUP_4, 0x2 /* Seq */);
            break;
        }

        case STAC9220_NID_VOL_KNOB:
        {
            pNode->volumeKnob.u32F08_param = 0;
            pNode->volumeKnob.u32F0f_param = 0x7f;

            pNode->volumeKnob.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_VOLUME_KNOB, 0, 0);
            pNode->volumeKnob.node.au32F00_param[0xD] = RT_BIT(7) | 0x7F;

            /* Connection list entries. */
            pNode->volumeKnob.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 4 /* Entries */);
            pNode->volumeKnob.node.au32F02_param[0x0] = RT_MAKE_U32_FROM_U8(STAC9220_NID_DAC0,
                                                                            STAC9220_NID_DAC1,
                                                                            STAC9220_NID_DAC2,
                                                                            STAC9220_NID_DAC3);
            break;
        }

        case STAC9220_NID_AMP_ADC0: /* ADC0Vol */
        {
            pNode->adcvol.node.au32F02_param[0] = STAC9220_NID_ADC0_MUX;
            goto adcvol_init;
        }

        case STAC9220_NID_AMP_ADC1: /* ADC1Vol */
        {
            pNode->adcvol.node.au32F02_param[0] = STAC9220_NID_ADC1_MUX;
            /* Fall through is intentional. */

        adcvol_init:

            pNode->adcvol.node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_SELECTOR, 0, 0)
                                                  | CODEC_F00_09_CAP_L_R_SWAP
                                                  | CODEC_F00_09_CAP_CONNECTION_LIST
                                                  | CODEC_F00_09_CAP_IN_AMP_PRESENT
                                                  | CODEC_F00_09_CAP_STEREO;


            pNode->adcvol.node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 1 /* Entries */);

            RT_ZERO(pNode->adcvol.B_params);
            AMPLIFIER_REGISTER(pNode->adcvol.B_params, AMPLIFIER_IN, AMPLIFIER_LEFT,  0) = RT_BIT(7);
            AMPLIFIER_REGISTER(pNode->adcvol.B_params, AMPLIFIER_IN, AMPLIFIER_RIGHT, 0) = RT_BIT(7);
            break;
        }

        /*
         * STAC9221 nodes.
         */

        case STAC9221_NID_ADAT_OUT:
        {
            pNode->node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_VENDOR_DEFINED, 3, 0)
                                           | CODEC_F00_09_CAP_DIGITAL
                                           | CODEC_F00_09_CAP_STEREO;
            break;
        }

        case STAC9221_NID_I2S_OUT:
        {
            pNode->node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_AUDIO_OUTPUT, 3, 0)
                                           | CODEC_F00_09_CAP_DIGITAL
                                           | CODEC_F00_09_CAP_STEREO;
            break;
        }

        case STAC9221_NID_PIN_I2S_OUT:
        {
            pNode->node.au32F00_param[0x9] = CODEC_MAKE_F00_09(CODEC_F00_09_TYPE_PIN_COMPLEX, 0, 0)
                                           | CODEC_F00_09_CAP_DIGITAL
                                           | CODEC_F00_09_CAP_CONNECTION_LIST
                                           | CODEC_F00_09_CAP_STEREO;

            pNode->node.au32F00_param[0xC] = CODEC_F00_0C_CAP_OUTPUT;

            /* Connection list entries. */
            pNode->node.au32F00_param[0xE] = CODEC_MAKE_F00_0E(CODEC_F00_0E_LIST_NID_SHORT, 1 /* Entries */);
            pNode->node.au32F02_param[0]   = STAC9221_NID_I2S_OUT;

            if (!pThis->fInReset)
                pNode->reserved.u32F1c_param = CODEC_MAKE_F1C(CODEC_F1C_PORT_NO_PHYS,
                                                              CODEC_F1C_LOCATION_NA,
                                                              CODEC_F1C_DEVICE_LINE_OUT,
                                                              CODEC_F1C_CONNECTION_TYPE_UNKNOWN,
                                                              CODEC_F1C_COLOR_UNKNOWN,
                                                              CODEC_F1C_MISC_NONE,
                                                              CODEC_F1C_ASSOCIATION_GROUP_15, 0x0 /* Ignored */);
            break;
        }

        default:
            AssertMsgFailed(("Node %RU8 not implemented\n", uNID));
            break;
    }

    return VINF_SUCCESS;
}

static int stac9220Construct(PHDACODEC pThis)
{
    unconst(pThis->cTotalNodes) = STAC9221_NUM_NODES;

    pThis->pfnReset     = stac9220Reset;
    pThis->pfnNodeReset = stac9220ResetNode;

    pThis->u16VendorId  = 0x8384; /* SigmaTel */
    /*
     * Note: The Linux kernel uses "patch_stac922x" for the fixups,
     *       which in turn uses "ref922x_pin_configs" for the configuration
     *       defaults tweaking in sound/pci/hda/patch_sigmatel.c.
     */
    pThis->u16DeviceId  = 0x7680; /* STAC9221 A1 */
    pThis->u8BSKU       = 0x76;
    pThis->u8AssemblyId = 0x80;

    pThis->paNodes = (PCODECNODE)RTMemAllocZ(sizeof(CODECNODE) * pThis->cTotalNodes);
    if (!pThis->paNodes)
        return VERR_NO_MEMORY;

    pThis->fInReset = false;

#define STAC9220WIDGET(type) pThis->au8##type##s = g_abStac9220##type##s
    STAC9220WIDGET(Port);
    STAC9220WIDGET(Dac);
    STAC9220WIDGET(Adc);
    STAC9220WIDGET(AdcVol);
    STAC9220WIDGET(AdcMux);
    STAC9220WIDGET(Pcbeep);
    STAC9220WIDGET(SpdifIn);
    STAC9220WIDGET(SpdifOut);
    STAC9220WIDGET(DigInPin);
    STAC9220WIDGET(DigOutPin);
    STAC9220WIDGET(Cd);
    STAC9220WIDGET(VolKnob);
    STAC9220WIDGET(Reserved);
#undef STAC9220WIDGET

    unconst(pThis->u8AdcVolsLineIn) = STAC9220_NID_AMP_ADC0;
    unconst(pThis->u8DacLineOut)    = STAC9220_NID_DAC1;

    /*
     * Initialize all codec nodes.
     * This is specific to the codec, so do this here.
     *
     * Note: Do *not* call stac9220Reset() here, as this would not
     *       initialize the node default configuration values then!
     */
    AssertPtr(pThis->paNodes);
    AssertPtr(pThis->pfnNodeReset);

    for (uint8_t i = 0; i < pThis->cTotalNodes; i++)
    {
        int rc2 = stac9220ResetNode(pThis, i, &pThis->paNodes[i]);
        AssertRC(rc2);
    }

    return VINF_SUCCESS;
}


/*
 * Some generic predicate functions.
 */

#define DECLISNODEOFTYPE(type)                                              \
    DECLINLINE(bool) hdaCodecIs##type##Node(PHDACODEC pThis, uint8_t cNode) \
    {                                                                       \
        Assert(pThis->au8##type##s);                                        \
        for (int i = 0; pThis->au8##type##s[i] != 0; ++i)                   \
            if (pThis->au8##type##s[i] == cNode)                            \
                return true;                                                \
        return false;                                                       \
    }
/* hdaCodecIsPortNode */
DECLISNODEOFTYPE(Port)
/* hdaCodecIsDacNode */
DECLISNODEOFTYPE(Dac)
/* hdaCodecIsAdcVolNode */
DECLISNODEOFTYPE(AdcVol)
/* hdaCodecIsAdcNode */
DECLISNODEOFTYPE(Adc)
/* hdaCodecIsAdcMuxNode */
DECLISNODEOFTYPE(AdcMux)
/* hdaCodecIsPcbeepNode */
DECLISNODEOFTYPE(Pcbeep)
/* hdaCodecIsSpdifOutNode */
DECLISNODEOFTYPE(SpdifOut)
/* hdaCodecIsSpdifInNode */
DECLISNODEOFTYPE(SpdifIn)
/* hdaCodecIsDigInPinNode */
DECLISNODEOFTYPE(DigInPin)
/* hdaCodecIsDigOutPinNode */
DECLISNODEOFTYPE(DigOutPin)
/* hdaCodecIsCdNode */
DECLISNODEOFTYPE(Cd)
/* hdaCodecIsVolKnobNode */
DECLISNODEOFTYPE(VolKnob)
/* hdaCodecIsReservedNode */
DECLISNODEOFTYPE(Reserved)


/*
 * Misc helpers.
 */
static int hdaCodecToAudVolume(PHDACODEC pThis, PCODECNODE pNode, AMPLIFIER *pAmp, PDMAUDIOMIXERCTL enmMixerCtl)
{
    RT_NOREF(pNode);

    uint8_t iDir;
    switch (enmMixerCtl)
    {
        case PDMAUDIOMIXERCTL_VOLUME_MASTER:
        case PDMAUDIOMIXERCTL_FRONT:
            iDir = AMPLIFIER_OUT;
            break;
        case PDMAUDIOMIXERCTL_LINE_IN:
        case PDMAUDIOMIXERCTL_MIC_IN:
            iDir = AMPLIFIER_IN;
            break;
        default:
            AssertMsgFailedReturn(("Invalid mixer control %RU32\n", enmMixerCtl), VERR_INVALID_PARAMETER);
            break;
    }

    int iMute;
    iMute  = AMPLIFIER_REGISTER(*pAmp, iDir, AMPLIFIER_LEFT,  0) & RT_BIT(7);
    iMute |= AMPLIFIER_REGISTER(*pAmp, iDir, AMPLIFIER_RIGHT, 0) & RT_BIT(7);
    iMute >>=7;
    iMute &= 0x1;

    uint8_t lVol = AMPLIFIER_REGISTER(*pAmp, iDir, AMPLIFIER_LEFT,  0) & 0x7f;
    uint8_t rVol = AMPLIFIER_REGISTER(*pAmp, iDir, AMPLIFIER_RIGHT, 0) & 0x7f;

    /*
     * The STAC9220 volume controls have 0 to -96dB attenuation range in 128 steps.
     * We have 0 to -96dB range in 256 steps. HDA volume setting of 127 must map
     * to 255 internally (0dB), while HDA volume setting of 0 (-96dB) should map
     * to 1 (rather than zero) internally.
     */
    lVol = (lVol + 1) * (2 * 255) / 256;
    rVol = (rVol + 1) * (2 * 255) / 256;

    PDMAUDIOVOLUME Vol = { RT_BOOL(iMute), lVol, rVol };

    LogFunc(("[NID0x%02x] %RU8/%RU8 (%s)\n",
             pNode->node.uID, lVol, rVol, RT_BOOL(iMute) ? "Muted" : "Unmuted"));

    LogRel2(("HDA: Setting volume for mixer control '%s' to %RU8/%RU8 (%s)\n",
             DrvAudioHlpAudMixerCtlToStr(enmMixerCtl), lVol, rVol, RT_BOOL(iMute) ? "Muted" : "Unmuted"));

    return pThis->pfnCbMixerSetVolume(pThis->pHDAState, enmMixerCtl, &Vol);
}

DECLINLINE(void) hdaCodecSetRegister(uint32_t *pu32Reg, uint32_t u32Cmd, uint8_t u8Offset, uint32_t mask)
{
    Assert((pu32Reg && u8Offset < 32));
    *pu32Reg &= ~(mask << u8Offset);
    *pu32Reg |= (u32Cmd & mask) << u8Offset;
}

DECLINLINE(void) hdaCodecSetRegisterU8(uint32_t *pu32Reg, uint32_t u32Cmd, uint8_t u8Offset)
{
    hdaCodecSetRegister(pu32Reg, u32Cmd, u8Offset, CODEC_VERB_8BIT_DATA);
}

DECLINLINE(void) hdaCodecSetRegisterU16(uint32_t *pu32Reg, uint32_t u32Cmd, uint8_t u8Offset)
{
    hdaCodecSetRegister(pu32Reg, u32Cmd, u8Offset, CODEC_VERB_16BIT_DATA);
}


/*
 * Verb processor functions.
 */
#if 0 /* unused */

static DECLCALLBACK(int) vrbProcUnimplemented(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    RT_NOREF(pThis, cmd);
    LogFlowFunc(("cmd(raw:%x: cad:%x, d:%c, nid:%x, verb:%x)\n", cmd,
                 CODEC_CAD(cmd), CODEC_DIRECT(cmd) ? 'N' : 'Y', CODEC_NID(cmd), CODEC_VERBDATA(cmd)));
    *pResp = 0;
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vrbProcBreak(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    int rc;
    rc = vrbProcUnimplemented(pThis, cmd, pResp);
    *pResp |= CODEC_RESPONSE_UNSOLICITED;
    return rc;
}

#endif /* unused */

/* B-- */
static DECLCALLBACK(int) vrbProcGetAmplifier(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    /* HDA spec 7.3.3.7 Note A */
    /** @todo If index out of range response should be 0. */
    uint8_t u8Index = CODEC_GET_AMP_DIRECTION(cmd) == AMPLIFIER_OUT ? 0 : CODEC_GET_AMP_INDEX(cmd);

    PCODECNODE pNode = &pThis->paNodes[CODEC_NID(cmd)];
    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        *pResp = AMPLIFIER_REGISTER(pNode->dac.B_params,
                            CODEC_GET_AMP_DIRECTION(cmd),
                            CODEC_GET_AMP_SIDE(cmd),
                            u8Index);
    else if (hdaCodecIsAdcVolNode(pThis, CODEC_NID(cmd)))
        *pResp = AMPLIFIER_REGISTER(pNode->adcvol.B_params,
                            CODEC_GET_AMP_DIRECTION(cmd),
                            CODEC_GET_AMP_SIDE(cmd),
                            u8Index);
    else if (hdaCodecIsAdcMuxNode(pThis, CODEC_NID(cmd)))
        *pResp = AMPLIFIER_REGISTER(pNode->adcmux.B_params,
                            CODEC_GET_AMP_DIRECTION(cmd),
                            CODEC_GET_AMP_SIDE(cmd),
                            u8Index);
    else if (hdaCodecIsPcbeepNode(pThis, CODEC_NID(cmd)))
        *pResp = AMPLIFIER_REGISTER(pNode->pcbeep.B_params,
                            CODEC_GET_AMP_DIRECTION(cmd),
                            CODEC_GET_AMP_SIDE(cmd),
                            u8Index);
    else if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        *pResp = AMPLIFIER_REGISTER(pNode->port.B_params,
                            CODEC_GET_AMP_DIRECTION(cmd),
                            CODEC_GET_AMP_SIDE(cmd),
                            u8Index);
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        *pResp = AMPLIFIER_REGISTER(pNode->adc.B_params,
                            CODEC_GET_AMP_DIRECTION(cmd),
                            CODEC_GET_AMP_SIDE(cmd),
                            u8Index);
    else
        LogRel2(("HDA: Warning: Unhandled get amplifier command: 0x%x (NID=0x%x [%RU8])\n", cmd, CODEC_NID(cmd), CODEC_NID(cmd)));

    return VINF_SUCCESS;
}

/* 3-- */
static DECLCALLBACK(int) vrbProcSetAmplifier(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    PCODECNODE pNode      = &pThis->paNodes[CODEC_NID(cmd)];
    AMPLIFIER *pAmplifier = NULL;
    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        pAmplifier = &pNode->dac.B_params;
    else if (hdaCodecIsAdcVolNode(pThis, CODEC_NID(cmd)))
        pAmplifier = &pNode->adcvol.B_params;
    else if (hdaCodecIsAdcMuxNode(pThis, CODEC_NID(cmd)))
        pAmplifier = &pNode->adcmux.B_params;
    else if (hdaCodecIsPcbeepNode(pThis, CODEC_NID(cmd)))
        pAmplifier = &pNode->pcbeep.B_params;
    else if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        pAmplifier = &pNode->port.B_params;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        pAmplifier = &pNode->adc.B_params;
    else
        LogRel2(("HDA: Warning: Unhandled set amplifier command: 0x%x (Payload=%RU16, NID=0x%x [%RU8])\n",
                 cmd, CODEC_VERB_PAYLOAD16(cmd), CODEC_NID(cmd), CODEC_NID(cmd)));

    if (!pAmplifier)
        return VINF_SUCCESS;

    bool fIsOut     = CODEC_SET_AMP_IS_OUT_DIRECTION(cmd);
    bool fIsIn      = CODEC_SET_AMP_IS_IN_DIRECTION(cmd);
    bool fIsLeft    = CODEC_SET_AMP_IS_LEFT_SIDE(cmd);
    bool fIsRight   = CODEC_SET_AMP_IS_RIGHT_SIDE(cmd);
    uint8_t u8Index = CODEC_SET_AMP_INDEX(cmd);

    if (   (!fIsLeft && !fIsRight)
        || (!fIsOut && !fIsIn))
        return VINF_SUCCESS;

    LogFunc(("[NID0x%02x] fIsOut=%RTbool, fIsIn=%RTbool, fIsLeft=%RTbool, fIsRight=%RTbool, Idx=%RU8\n",
             CODEC_NID(cmd), fIsOut, fIsIn, fIsLeft, fIsRight, u8Index));

    if (fIsIn)
    {
        if (fIsLeft)
            hdaCodecSetRegisterU8(&AMPLIFIER_REGISTER(*pAmplifier, AMPLIFIER_IN, AMPLIFIER_LEFT, u8Index), cmd, 0);
        if (fIsRight)
            hdaCodecSetRegisterU8(&AMPLIFIER_REGISTER(*pAmplifier, AMPLIFIER_IN, AMPLIFIER_RIGHT, u8Index), cmd, 0);

    //    if (CODEC_NID(cmd) == pThis->u8AdcVolsLineIn)
    //    {
            hdaCodecToAudVolume(pThis, pNode, pAmplifier, PDMAUDIOMIXERCTL_LINE_IN);
    //    }
    }
    if (fIsOut)
    {
        if (fIsLeft)
            hdaCodecSetRegisterU8(&AMPLIFIER_REGISTER(*pAmplifier, AMPLIFIER_OUT, AMPLIFIER_LEFT, u8Index), cmd, 0);
        if (fIsRight)
            hdaCodecSetRegisterU8(&AMPLIFIER_REGISTER(*pAmplifier, AMPLIFIER_OUT, AMPLIFIER_RIGHT, u8Index), cmd, 0);

        if (CODEC_NID(cmd) == pThis->u8DacLineOut)
            hdaCodecToAudVolume(pThis, pNode, pAmplifier, PDMAUDIOMIXERCTL_FRONT);
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vrbProcGetParameter(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    Assert((cmd & CODEC_VERB_8BIT_DATA) < CODECNODE_F00_PARAM_LENGTH);
    if ((cmd & CODEC_VERB_8BIT_DATA) >= CODECNODE_F00_PARAM_LENGTH)
    {
        *pResp = 0;

        LogFlowFunc(("invalid F00 parameter %d\n", (cmd & CODEC_VERB_8BIT_DATA)));
        return VINF_SUCCESS;
    }

    *pResp = pThis->paNodes[CODEC_NID(cmd)].node.au32F00_param[cmd & CODEC_VERB_8BIT_DATA];
    return VINF_SUCCESS;
}

/* F01 */
static DECLCALLBACK(int) vrbProcGetConSelectCtrl(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsAdcMuxNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adcmux.u32F01_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digout.u32F01_param;
    else if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].port.u32F01_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adc.u32F01_param;
    else if (hdaCodecIsAdcVolNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adcvol.u32F01_param;
    else
        LogRel2(("HDA: Warning: Unhandled get connection select control command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 701 */
static DECLCALLBACK(int) vrbProcSetConSelectCtrl(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsAdcMuxNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].adcmux.u32F01_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digout.u32F01_param;
    else if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].port.u32F01_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].adc.u32F01_param;
    else if (hdaCodecIsAdcVolNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].adcvol.u32F01_param;
    else
        LogRel2(("HDA: Warning: Unhandled set connection select control command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/* F07 */
static DECLCALLBACK(int) vrbProcGetPinCtrl(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].port.u32F07_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digout.u32F07_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F07_param;
    else if (hdaCodecIsCdNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].cdnode.u32F07_param;
    else if (hdaCodecIsPcbeepNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].pcbeep.u32F07_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].reserved.u32F07_param;
    else
        LogRel2(("HDA: Warning: Unhandled get pin control command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 707 */
static DECLCALLBACK(int) vrbProcSetPinCtrl(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].port.u32F07_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F07_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digout.u32F07_param;
    else if (hdaCodecIsCdNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].cdnode.u32F07_param;
    else if (hdaCodecIsPcbeepNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].pcbeep.u32F07_param;
    else if (   hdaCodecIsReservedNode(pThis, CODEC_NID(cmd))
             && CODEC_NID(cmd) == 0x1b)
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].reserved.u32F07_param;
    else
        LogRel2(("HDA: Warning: Unhandled set pin control command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/* F08 */
static DECLCALLBACK(int) vrbProcGetUnsolicitedEnabled(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].port.u32F08_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F08_param;
    else if ((cmd) == STAC9220_NID_AFG)
        *pResp = pThis->paNodes[CODEC_NID(cmd)].afg.u32F08_param;
    else if (hdaCodecIsVolKnobNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].volumeKnob.u32F08_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digout.u32F08_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F08_param;
    else
        LogRel2(("HDA: Warning: Unhandled get unsolicited enabled command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 708 */
static DECLCALLBACK(int) vrbProcSetUnsolicitedEnabled(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].port.u32F08_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F08_param;
    else if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].afg.u32F08_param;
    else if (hdaCodecIsVolKnobNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].volumeKnob.u32F08_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F08_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digout.u32F08_param;
    else
        LogRel2(("HDA: Warning: Unhandled set unsolicited enabled command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/* F09 */
static DECLCALLBACK(int) vrbProcGetPinSense(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].port.u32F09_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F09_param;
    else
    {
        AssertFailed();
        LogRel2(("HDA: Warning: Unhandled get pin sense command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));
    }

    return VINF_SUCCESS;
}

/* 709 */
static DECLCALLBACK(int) vrbProcSetPinSense(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].port.u32F09_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F09_param;
    else
        LogRel2(("HDA: Warning: Unhandled set pin sense command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vrbProcGetConnectionListEntry(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    Assert((cmd & CODEC_VERB_8BIT_DATA) < CODECNODE_F02_PARAM_LENGTH);
    if ((cmd & CODEC_VERB_8BIT_DATA) >= CODECNODE_F02_PARAM_LENGTH)
    {
        LogFlowFunc(("access to invalid F02 index %d\n", (cmd & CODEC_VERB_8BIT_DATA)));
        return VINF_SUCCESS;
    }
    *pResp = pThis->paNodes[CODEC_NID(cmd)].node.au32F02_param[cmd & CODEC_VERB_8BIT_DATA];
    return VINF_SUCCESS;
}

/* F03 */
static DECLCALLBACK(int) vrbProcGetProcessingState(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adc.u32F03_param;

    return VINF_SUCCESS;
}

/* 703 */
static DECLCALLBACK(int) vrbProcSetProcessingState(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU8(&pThis->paNodes[CODEC_NID(cmd)].adc.u32F03_param, cmd, 0);
    return VINF_SUCCESS;
}

/* F0D */
static DECLCALLBACK(int) vrbProcGetDigitalConverter(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F0d_param;
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F0d_param;

    return VINF_SUCCESS;
}

static int codecSetDigitalConverter(PHDACODEC pThis, uint32_t cmd, uint8_t u8Offset, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU8(&pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F0d_param, cmd, u8Offset);
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU8(&pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F0d_param, cmd, u8Offset);
    return VINF_SUCCESS;
}

/* 70D */
static DECLCALLBACK(int) vrbProcSetDigitalConverter1(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    return codecSetDigitalConverter(pThis, cmd, 0, pResp);
}

/* 70E */
static DECLCALLBACK(int) vrbProcSetDigitalConverter2(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    return codecSetDigitalConverter(pThis, cmd, 8, pResp);
}

/* F20 */
static DECLCALLBACK(int) vrbProcGetSubId(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    Assert(CODEC_CAD(cmd) == pThis->id);
    Assert(CODEC_NID(cmd) < pThis->cTotalNodes);
    if (CODEC_NID(cmd) >= pThis->cTotalNodes)
    {
        LogFlowFunc(("invalid node address %d\n", CODEC_NID(cmd)));
        return VINF_SUCCESS;
    }
    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        *pResp = pThis->paNodes[CODEC_NID(cmd)].afg.u32F20_param;
    else
        *pResp = 0;
    return VINF_SUCCESS;
}

static int codecSetSubIdX(PHDACODEC pThis, uint32_t cmd, uint8_t u8Offset)
{
    Assert(CODEC_CAD(cmd) == pThis->id);
    Assert(CODEC_NID(cmd) < pThis->cTotalNodes);
    if (CODEC_NID(cmd) >= pThis->cTotalNodes)
    {
        LogFlowFunc(("invalid node address %d\n", CODEC_NID(cmd)));
        return VINF_SUCCESS;
    }
    uint32_t *pu32Reg;
    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].afg.u32F20_param;
    else
        AssertFailedReturn(VINF_SUCCESS);
    hdaCodecSetRegisterU8(pu32Reg, cmd, u8Offset);
    return VINF_SUCCESS;
}

/* 720 */
static DECLCALLBACK(int) vrbProcSetSubId0(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetSubIdX(pThis, cmd, 0);
}

/* 721 */
static DECLCALLBACK(int) vrbProcSetSubId1(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetSubIdX(pThis, cmd, 8);
}

/* 722 */
static DECLCALLBACK(int) vrbProcSetSubId2(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetSubIdX(pThis, cmd, 16);
}

/* 723 */
static DECLCALLBACK(int) vrbProcSetSubId3(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetSubIdX(pThis, cmd, 24);
}

static DECLCALLBACK(int) vrbProcReset(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    Assert(CODEC_CAD(cmd) == pThis->id);
    Assert(CODEC_NID(cmd) == STAC9220_NID_AFG);

    if (   CODEC_NID(cmd) == STAC9220_NID_AFG
        && pThis->pfnReset)
    {
        pThis->pfnReset(pThis);
    }

    *pResp = 0;
    return VINF_SUCCESS;
}

/* F05 */
static DECLCALLBACK(int) vrbProcGetPowerState(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        *pResp = pThis->paNodes[CODEC_NID(cmd)].afg.u32F05_param;
    else if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].dac.u32F05_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adc.u32F05_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F05_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digout.u32F05_param;
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F05_param;
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F05_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].reserved.u32F05_param;
    else
        LogRel2(("HDA: Warning: Unhandled get power state command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    LogFunc(("[NID0x%02x]: fReset=%RTbool, fStopOk=%RTbool, Act=D%RU8, Set=D%RU8\n",
             CODEC_NID(cmd), CODEC_F05_IS_RESET(*pResp), CODEC_F05_IS_STOPOK(*pResp), CODEC_F05_ACT(*pResp), CODEC_F05_SET(*pResp)));
    return VINF_SUCCESS;
}

/* 705 */
#if 1
static DECLCALLBACK(int) vrbProcSetPowerState(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].afg.u32F05_param;
    else if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].dac.u32F05_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F05_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digout.u32F05_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].adc.u32F05_param;
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F05_param;
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F05_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].reserved.u32F05_param;
    else
    {
        LogRel2(("HDA: Warning: Unhandled set power state command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));
    }

    if (!pu32Reg)
        return VINF_SUCCESS;

    uint8_t uPwrCmd = CODEC_F05_SET      (cmd);
    bool    fReset  = CODEC_F05_IS_RESET (*pu32Reg);
    bool    fStopOk = CODEC_F05_IS_STOPOK(*pu32Reg);
#ifdef LOG_ENABLED
    bool    fError  = CODEC_F05_IS_ERROR (*pu32Reg);
    uint8_t uPwrAct = CODEC_F05_ACT      (*pu32Reg);
    uint8_t uPwrSet = CODEC_F05_SET      (*pu32Reg);
    LogFunc(("[NID0x%02x] Cmd=D%RU8, fReset=%RTbool, fStopOk=%RTbool, fError=%RTbool, uPwrAct=D%RU8, uPwrSet=D%RU8\n",
             CODEC_NID(cmd), uPwrCmd, fReset, fStopOk, fError, uPwrAct, uPwrSet));
    LogFunc(("AFG: Act=D%RU8, Set=D%RU8\n",
            CODEC_F05_ACT(pThis->paNodes[STAC9220_NID_AFG].afg.u32F05_param),
            CODEC_F05_SET(pThis->paNodes[STAC9220_NID_AFG].afg.u32F05_param)));
#endif

    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        *pu32Reg = CODEC_MAKE_F05(fReset, fStopOk, 0, uPwrCmd /* PS-Act */, uPwrCmd /* PS-Set */);

    const uint8_t uAFGPwrAct = CODEC_F05_ACT(pThis->paNodes[STAC9220_NID_AFG].afg.u32F05_param);
    if (uAFGPwrAct == CODEC_F05_D0) /* Only propagate power state if AFG is on (D0). */
    {
        /* Propagate to all other nodes under this AFG. */
        LogFunc(("Propagating Act=D%RU8 (AFG), Set=D%RU8 to all AFG child nodes ...\n", uAFGPwrAct, uPwrCmd));

#define PROPAGATE_PWR_STATE(_aList, _aMember) \
        { \
            const uint8_t *pu8NodeIndex = &_aList[0]; \
            while (*(++pu8NodeIndex)) \
            { \
                pThis->paNodes[*pu8NodeIndex]._aMember.u32F05_param = \
                    CODEC_MAKE_F05(fReset, fStopOk, 0, uAFGPwrAct, uPwrCmd); \
                LogFunc(("\t[NID0x%02x]: Act=D%RU8, Set=D%RU8\n", *pu8NodeIndex, \
                         CODEC_F05_ACT(pThis->paNodes[*pu8NodeIndex]._aMember.u32F05_param), \
                         CODEC_F05_SET(pThis->paNodes[*pu8NodeIndex]._aMember.u32F05_param))); \
            } \
        }

        PROPAGATE_PWR_STATE(pThis->au8Dacs,       dac);
        PROPAGATE_PWR_STATE(pThis->au8Adcs,       adc);
        PROPAGATE_PWR_STATE(pThis->au8DigInPins,  digin);
        PROPAGATE_PWR_STATE(pThis->au8DigOutPins, digout);
        PROPAGATE_PWR_STATE(pThis->au8SpdifIns,   spdifin);
        PROPAGATE_PWR_STATE(pThis->au8SpdifOuts,  spdifout);
        PROPAGATE_PWR_STATE(pThis->au8Reserveds,  reserved);

#undef PROPAGATE_PWR_STATE
    }
    /*
     * If this node is a reqular node (not the AFG one), adopt PS-Set of the AFG node
     * as PS-Set of this node. PS-Act always is one level under PS-Set here.
     */
    else
    {
        *pu32Reg = CODEC_MAKE_F05(fReset, fStopOk, 0, uAFGPwrAct, uPwrCmd);
    }

    LogFunc(("[NID0x%02x] fReset=%RTbool, fStopOk=%RTbool, Act=D%RU8, Set=D%RU8\n",
             CODEC_NID(cmd),
             CODEC_F05_IS_RESET(*pu32Reg), CODEC_F05_IS_STOPOK(*pu32Reg), CODEC_F05_ACT(*pu32Reg), CODEC_F05_SET(*pu32Reg)));

    return VINF_SUCCESS;
}
#else
DECLINLINE(void) codecPropogatePowerState(uint32_t *pu32F05_param)
{
    Assert(pu32F05_param);
    if (!pu32F05_param)
        return;
    bool fReset = CODEC_F05_IS_RESET(*pu32F05_param);
    bool fStopOk = CODEC_F05_IS_STOPOK(*pu32F05_param);
    uint8_t u8SetPowerState = CODEC_F05_SET(*pu32F05_param);
    *pu32F05_param = CODEC_MAKE_F05(fReset, fStopOk, 0, u8SetPowerState, u8SetPowerState);
}

static DECLCALLBACK(int) vrbProcSetPowerState(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    Assert(CODEC_CAD(cmd) == pThis->id);
    Assert(CODEC_NID(cmd) < pThis->cTotalNodes);
    if (CODEC_NID(cmd) >= pThis->cTotalNodes)
    {
        LogFlowFunc(("invalid node address %d\n", CODEC_NID(cmd)));
        return VINF_SUCCESS;
    }
    *pResp = 0;
    uint32_t *pu32Reg;
    if (CODEC_NID(cmd) == 1 /* AFG */)
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].afg.u32F05_param;
    else if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].dac.u32F05_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F05_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].adc.u32F05_param;
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F05_param;
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F05_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].reserved.u32F05_param;
    else
        AssertFailedReturn(VINF_SUCCESS);

    bool fReset = CODEC_F05_IS_RESET(*pu32Reg);
    bool fStopOk = CODEC_F05_IS_STOPOK(*pu32Reg);

    if (CODEC_NID(cmd) != 1 /* AFG */)
    {
        /*
         * We shouldn't propogate actual power state, which actual for AFG
         */
        *pu32Reg = CODEC_MAKE_F05(fReset, fStopOk, 0,
                                  CODEC_F05_ACT(pThis->paNodes[1].afg.u32F05_param),
                                  CODEC_F05_SET(cmd));
    }

    /* Propagate next power state only if AFG is on or verb modifies AFG power state */
    if (   CODEC_NID(cmd) == 1 /* AFG */
        || !CODEC_F05_ACT(pThis->paNodes[1].afg.u32F05_param))
    {
        *pu32Reg = CODEC_MAKE_F05(fReset, fStopOk, 0, CODEC_F05_SET(cmd), CODEC_F05_SET(cmd));
        if (   CODEC_NID(cmd) == 1 /* AFG */
            && (CODEC_F05_SET(cmd)) == CODEC_F05_D0)
        {
            /* now we're powered on AFG and may propogate power states on nodes */
            const uint8_t *pu8NodeIndex = &pThis->au8Dacs[0];
            while (*(++pu8NodeIndex))
                codecPropogatePowerState(&pThis->paNodes[*pu8NodeIndex].dac.u32F05_param);

            pu8NodeIndex = &pThis->au8Adcs[0];
            while (*(++pu8NodeIndex))
                codecPropogatePowerState(&pThis->paNodes[*pu8NodeIndex].adc.u32F05_param);

            pu8NodeIndex = &pThis->au8DigInPins[0];
            while (*(++pu8NodeIndex))
                codecPropogatePowerState(&pThis->paNodes[*pu8NodeIndex].digin.u32F05_param);
        }
    }
    return VINF_SUCCESS;
}
#endif

/* F06 */
static DECLCALLBACK(int) vrbProcGetStreamId(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].dac.u32F06_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adc.u32F06_param;
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F06_param;
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F06_param;
    else if (CODEC_NID(cmd) == STAC9221_NID_I2S_OUT)
        *pResp = pThis->paNodes[CODEC_NID(cmd)].reserved.u32F06_param;
    else
        LogRel2(("HDA: Warning: Unhandled get stream ID command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    LogFlowFunc(("[NID0x%02x] Stream ID=%RU8, channel=%RU8\n",
                 CODEC_NID(cmd), CODEC_F00_06_GET_STREAM_ID(cmd), CODEC_F00_06_GET_CHANNEL_ID(cmd)));

    return VINF_SUCCESS;
}

/* 706 */
static DECLCALLBACK(int) vrbProcSetStreamId(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint8_t uSD      = CODEC_F00_06_GET_STREAM_ID(cmd);
    uint8_t uChannel = CODEC_F00_06_GET_CHANNEL_ID(cmd);

    LogFlowFunc(("[NID0x%02x] Setting to stream ID=%RU8, channel=%RU8\n",
                 CODEC_NID(cmd), uSD, uChannel));

    PDMAUDIODIR enmDir;
    uint32_t *pu32Addr = NULL;
    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
    {
        pu32Addr = &pThis->paNodes[CODEC_NID(cmd)].dac.u32F06_param;
        enmDir = PDMAUDIODIR_OUT;
    }
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
    {
        pu32Addr = &pThis->paNodes[CODEC_NID(cmd)].adc.u32F06_param;
        enmDir = PDMAUDIODIR_IN;
    }
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
    {
        pu32Addr = &pThis->paNodes[CODEC_NID(cmd)].spdifout.u32F06_param;
        enmDir = PDMAUDIODIR_OUT;
    }
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
    {
        pu32Addr = &pThis->paNodes[CODEC_NID(cmd)].spdifin.u32F06_param;
        enmDir = PDMAUDIODIR_IN;
    }
    else
    {
        enmDir = PDMAUDIODIR_UNKNOWN;
        LogRel2(("HDA: Warning: Unhandled set stream ID command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));
    }

    /* Do we (re-)assign our input/output SDn (SDI/SDO) IDs? */
    if (enmDir != PDMAUDIODIR_UNKNOWN)
    {
        pThis->paNodes[CODEC_NID(cmd)].node.uSD      = uSD;
        pThis->paNodes[CODEC_NID(cmd)].node.uChannel = uChannel;

        if (enmDir == PDMAUDIODIR_OUT)
        {
            /** @todo Check if non-interleaved streams need a different channel / SDn? */

            /* Propagate to the controller. */
            pThis->pfnCbMixerControl(pThis->pHDAState, PDMAUDIOMIXERCTL_FRONT,      uSD, uChannel);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
            pThis->pfnCbMixerControl(pThis->pHDAState, PDMAUDIOMIXERCTL_CENTER_LFE, uSD, uChannel);
            pThis->pfnCbMixerControl(pThis->pHDAState, PDMAUDIOMIXERCTL_REAR,       uSD, uChannel);
#endif
        }
        else if (enmDir == PDMAUDIODIR_IN)
        {
            pThis->pfnCbMixerControl(pThis->pHDAState, PDMAUDIOMIXERCTL_LINE_IN,    uSD, uChannel);
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
            pThis->pfnCbMixerControl(pThis->pHDAState, PDMAUDIOMIXERCTL_MIC_IN,     uSD, uChannel);
#endif
        }
    }

    if (pu32Addr)
        hdaCodecSetRegisterU8(pu32Addr, cmd, 0);

    return VINF_SUCCESS;
}

/* A0 */
static DECLCALLBACK(int) vrbProcGetConverterFormat(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].dac.u32A_param;
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adc.u32A_param;
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifout.u32A_param;
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].spdifin.u32A_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].reserved.u32A_param;
    else
        LogRel2(("HDA: Warning: Unhandled get converter format command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* Also see section 3.7.1. */
static DECLCALLBACK(int) vrbProcSetConverterFormat(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU16(&pThis->paNodes[CODEC_NID(cmd)].dac.u32A_param, cmd, 0);
    else if (hdaCodecIsAdcNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU16(&pThis->paNodes[CODEC_NID(cmd)].adc.u32A_param, cmd, 0);
    else if (hdaCodecIsSpdifOutNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU16(&pThis->paNodes[CODEC_NID(cmd)].spdifout.u32A_param, cmd, 0);
    else if (hdaCodecIsSpdifInNode(pThis, CODEC_NID(cmd)))
        hdaCodecSetRegisterU16(&pThis->paNodes[CODEC_NID(cmd)].spdifin.u32A_param, cmd, 0);
    else
        LogRel2(("HDA: Warning: Unhandled set converter format command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* F0C */
static DECLCALLBACK(int) vrbProcGetEAPD_BTLEnabled(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsAdcVolNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].adcvol.u32F0c_param;
    else if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].dac.u32F0c_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F0c_param;
    else
        LogRel2(("HDA: Warning: Unhandled get EAPD/BTL enabled command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 70C */
static DECLCALLBACK(int) vrbProcSetEAPD_BTLEnabled(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsAdcVolNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].adcvol.u32F0c_param;
    else if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].dac.u32F0c_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F0c_param;
    else
        LogRel2(("HDA: Warning: Unhandled set EAPD/BTL enabled command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/* F0F */
static DECLCALLBACK(int) vrbProcGetVolumeKnobCtrl(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsVolKnobNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].volumeKnob.u32F0f_param;
    else
        LogRel2(("HDA: Warning: Unhandled get volume knob control command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 70F */
static DECLCALLBACK(int) vrbProcSetVolumeKnobCtrl(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsVolKnobNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].volumeKnob.u32F0f_param;
    else
        LogRel2(("HDA: Warning: Unhandled set volume knob control command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/* F15 */
static DECLCALLBACK(int) vrbProcGetGPIOData(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    RT_NOREF(pThis, cmd);
    *pResp = 0;
    return VINF_SUCCESS;
}

/* 715 */
static DECLCALLBACK(int) vrbProcSetGPIOData(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    RT_NOREF(pThis, cmd);
    *pResp = 0;
    return VINF_SUCCESS;
}

/* F16 */
static DECLCALLBACK(int) vrbProcGetGPIOEnableMask(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    RT_NOREF(pThis, cmd);
    *pResp = 0;
    return VINF_SUCCESS;
}

/* 716 */
static DECLCALLBACK(int) vrbProcSetGPIOEnableMask(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    RT_NOREF(pThis, cmd);
    *pResp = 0;
    return VINF_SUCCESS;
}

/* F17 */
static DECLCALLBACK(int) vrbProcGetGPIODirection(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    /* Note: this is true for ALC885. */
    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        *pResp = pThis->paNodes[1].afg.u32F17_param;
    else
        LogRel2(("HDA: Warning: Unhandled get GPIO direction command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 717 */
static DECLCALLBACK(int) vrbProcSetGPIODirection(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (CODEC_NID(cmd) == STAC9220_NID_AFG)
        pu32Reg = &pThis->paNodes[1].afg.u32F17_param;
    else
        LogRel2(("HDA: Warning: Unhandled set GPIO direction command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/* F1C */
static DECLCALLBACK(int) vrbProcGetConfig(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].port.u32F1c_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digout.u32F1c_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].digin.u32F1c_param;
    else if (hdaCodecIsPcbeepNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].pcbeep.u32F1c_param;
    else if (hdaCodecIsCdNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].cdnode.u32F1c_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].reserved.u32F1c_param;
    else
        LogRel2(("HDA: Warning: Unhandled get config command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

static int codecSetConfigX(PHDACODEC pThis, uint32_t cmd, uint8_t u8Offset)
{
    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsPortNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].port.u32F1c_param;
    else if (hdaCodecIsDigInPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digin.u32F1c_param;
    else if (hdaCodecIsDigOutPinNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].digout.u32F1c_param;
    else if (hdaCodecIsCdNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].cdnode.u32F1c_param;
    else if (hdaCodecIsPcbeepNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].pcbeep.u32F1c_param;
    else if (hdaCodecIsReservedNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].reserved.u32F1c_param;
    else
        LogRel2(("HDA: Warning: Unhandled set config command (%RU8) for NID0x%02x: 0x%x\n", u8Offset, CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, u8Offset);

    return VINF_SUCCESS;
}

/* 71C */
static DECLCALLBACK(int) vrbProcSetConfig0(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetConfigX(pThis, cmd, 0);
}

/* 71D */
static DECLCALLBACK(int) vrbProcSetConfig1(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetConfigX(pThis, cmd, 8);
}

/* 71E */
static DECLCALLBACK(int) vrbProcSetConfig2(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetConfigX(pThis, cmd, 16);
}

/* 71E */
static DECLCALLBACK(int) vrbProcSetConfig3(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;
    return codecSetConfigX(pThis, cmd, 24);
}

/* F04 */
static DECLCALLBACK(int) vrbProcGetSDISelect(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        *pResp = pThis->paNodes[CODEC_NID(cmd)].dac.u32F04_param;
    else
        LogRel2(("HDA: Warning: Unhandled get SDI select command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    return VINF_SUCCESS;
}

/* 704 */
static DECLCALLBACK(int) vrbProcSetSDISelect(PHDACODEC pThis, uint32_t cmd, uint64_t *pResp)
{
    *pResp = 0;

    uint32_t *pu32Reg = NULL;
    if (hdaCodecIsDacNode(pThis, CODEC_NID(cmd)))
        pu32Reg = &pThis->paNodes[CODEC_NID(cmd)].dac.u32F04_param;
    else
        LogRel2(("HDA: Warning: Unhandled set SDI select command for NID0x%02x: 0x%x\n", CODEC_NID(cmd), cmd));

    if (pu32Reg)
        hdaCodecSetRegisterU8(pu32Reg, cmd, 0);

    return VINF_SUCCESS;
}

/**
 * HDA codec verb map.
 * @todo Any reason not to use binary search here?
 */
static const CODECVERB g_aCodecVerbs[] =
{
    /* Verb        Verb mask            Callback                        Name
     * ---------- --------------------- ----------------------------------------------------------
     */
    { 0x000F0000, CODEC_VERB_8BIT_CMD , vrbProcGetParameter           , "GetParameter          " },
    { 0x000F0100, CODEC_VERB_8BIT_CMD , vrbProcGetConSelectCtrl       , "GetConSelectCtrl      " },
    { 0x00070100, CODEC_VERB_8BIT_CMD , vrbProcSetConSelectCtrl       , "SetConSelectCtrl      " },
    { 0x000F0600, CODEC_VERB_8BIT_CMD , vrbProcGetStreamId            , "GetStreamId           " },
    { 0x00070600, CODEC_VERB_8BIT_CMD , vrbProcSetStreamId            , "SetStreamId           " },
    { 0x000F0700, CODEC_VERB_8BIT_CMD , vrbProcGetPinCtrl             , "GetPinCtrl            " },
    { 0x00070700, CODEC_VERB_8BIT_CMD , vrbProcSetPinCtrl             , "SetPinCtrl            " },
    { 0x000F0800, CODEC_VERB_8BIT_CMD , vrbProcGetUnsolicitedEnabled  , "GetUnsolicitedEnabled " },
    { 0x00070800, CODEC_VERB_8BIT_CMD , vrbProcSetUnsolicitedEnabled  , "SetUnsolicitedEnabled " },
    { 0x000F0900, CODEC_VERB_8BIT_CMD , vrbProcGetPinSense            , "GetPinSense           " },
    { 0x00070900, CODEC_VERB_8BIT_CMD , vrbProcSetPinSense            , "SetPinSense           " },
    { 0x000F0200, CODEC_VERB_8BIT_CMD , vrbProcGetConnectionListEntry , "GetConnectionListEntry" },
    { 0x000F0300, CODEC_VERB_8BIT_CMD , vrbProcGetProcessingState     , "GetProcessingState    " },
    { 0x00070300, CODEC_VERB_8BIT_CMD , vrbProcSetProcessingState     , "SetProcessingState    " },
    { 0x000F0D00, CODEC_VERB_8BIT_CMD , vrbProcGetDigitalConverter    , "GetDigitalConverter   " },
    { 0x00070D00, CODEC_VERB_8BIT_CMD , vrbProcSetDigitalConverter1   , "SetDigitalConverter1  " },
    { 0x00070E00, CODEC_VERB_8BIT_CMD , vrbProcSetDigitalConverter2   , "SetDigitalConverter2  " },
    { 0x000F2000, CODEC_VERB_8BIT_CMD , vrbProcGetSubId               , "GetSubId              " },
    { 0x00072000, CODEC_VERB_8BIT_CMD , vrbProcSetSubId0              , "SetSubId0             " },
    { 0x00072100, CODEC_VERB_8BIT_CMD , vrbProcSetSubId1              , "SetSubId1             " },
    { 0x00072200, CODEC_VERB_8BIT_CMD , vrbProcSetSubId2              , "SetSubId2             " },
    { 0x00072300, CODEC_VERB_8BIT_CMD , vrbProcSetSubId3              , "SetSubId3             " },
    { 0x0007FF00, CODEC_VERB_8BIT_CMD , vrbProcReset                  , "Reset                 " },
    { 0x000F0500, CODEC_VERB_8BIT_CMD , vrbProcGetPowerState          , "GetPowerState         " },
    { 0x00070500, CODEC_VERB_8BIT_CMD , vrbProcSetPowerState          , "SetPowerState         " },
    { 0x000F0C00, CODEC_VERB_8BIT_CMD , vrbProcGetEAPD_BTLEnabled     , "GetEAPD_BTLEnabled    " },
    { 0x00070C00, CODEC_VERB_8BIT_CMD , vrbProcSetEAPD_BTLEnabled     , "SetEAPD_BTLEnabled    " },
    { 0x000F0F00, CODEC_VERB_8BIT_CMD , vrbProcGetVolumeKnobCtrl      , "GetVolumeKnobCtrl     " },
    { 0x00070F00, CODEC_VERB_8BIT_CMD , vrbProcSetVolumeKnobCtrl      , "SetVolumeKnobCtrl     " },
    { 0x000F1500, CODEC_VERB_8BIT_CMD , vrbProcGetGPIOData            , "GetGPIOData           " },
    { 0x00071500, CODEC_VERB_8BIT_CMD , vrbProcSetGPIOData            , "SetGPIOData           " },
    { 0x000F1600, CODEC_VERB_8BIT_CMD , vrbProcGetGPIOEnableMask      , "GetGPIOEnableMask     " },
    { 0x00071600, CODEC_VERB_8BIT_CMD , vrbProcSetGPIOEnableMask      , "SetGPIOEnableMask     " },
    { 0x000F1700, CODEC_VERB_8BIT_CMD , vrbProcGetGPIODirection       , "GetGPIODirection      " },
    { 0x00071700, CODEC_VERB_8BIT_CMD , vrbProcSetGPIODirection       , "SetGPIODirection      " },
    { 0x000F1C00, CODEC_VERB_8BIT_CMD , vrbProcGetConfig              , "GetConfig             " },
    { 0x00071C00, CODEC_VERB_8BIT_CMD , vrbProcSetConfig0             , "SetConfig0            " },
    { 0x00071D00, CODEC_VERB_8BIT_CMD , vrbProcSetConfig1             , "SetConfig1            " },
    { 0x00071E00, CODEC_VERB_8BIT_CMD , vrbProcSetConfig2             , "SetConfig2            " },
    { 0x00071F00, CODEC_VERB_8BIT_CMD , vrbProcSetConfig3             , "SetConfig3            " },
    { 0x000A0000, CODEC_VERB_16BIT_CMD, vrbProcGetConverterFormat     , "GetConverterFormat    " },
    { 0x00020000, CODEC_VERB_16BIT_CMD, vrbProcSetConverterFormat     , "SetConverterFormat    " },
    { 0x000B0000, CODEC_VERB_16BIT_CMD, vrbProcGetAmplifier           , "GetAmplifier          " },
    { 0x00030000, CODEC_VERB_16BIT_CMD, vrbProcSetAmplifier           , "SetAmplifier          " },
    { 0x000F0400, CODEC_VERB_8BIT_CMD , vrbProcGetSDISelect           , "GetSDISelect          " },
    { 0x00070400, CODEC_VERB_8BIT_CMD , vrbProcSetSDISelect           , "SetSDISelect          " }
    /** @todo Implement 0x7e7: IDT Set GPIO (STAC922x only). */
};

#ifdef DEBUG
typedef struct CODECDBGINFO
{
    /** DBGF info helpers. */
    PCDBGFINFOHLP pHlp;
    /** Current recursion level. */
    uint8_t uLevel;
    /** Pointer to codec state. */
    PHDACODEC pThis;

} CODECDBGINFO, *PCODECDBGINFO;

#define CODECDBG_INDENT   pInfo->uLevel++;
#define CODECDBG_UNINDENT if (pInfo->uLevel) pInfo->uLevel--;

#define CODECDBG_PRINT(...)  pInfo->pHlp->pfnPrintf(pInfo->pHlp, __VA_ARGS__)
#define CODECDBG_PRINTI(...) codecDbgPrintf(pInfo, __VA_ARGS__)

static void codecDbgPrintfIndentV(PCODECDBGINFO pInfo, uint16_t uIndent, const char *pszFormat, va_list va)
{
    char *pszValueFormat;
    if (RTStrAPrintfV(&pszValueFormat, pszFormat, va))
    {
        pInfo->pHlp->pfnPrintf(pInfo->pHlp, "%*s%s", uIndent, "", pszValueFormat);
        RTStrFree(pszValueFormat);
    }
}

static void codecDbgPrintf(PCODECDBGINFO pInfo, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    codecDbgPrintfIndentV(pInfo, pInfo->uLevel * 4, pszFormat, va);
    va_end(va);
}

/* Power state */
static void codecDbgPrintNodeRegF05(PCODECDBGINFO pInfo, uint32_t u32Reg)
{
    codecDbgPrintf(pInfo, "Power (F05): fReset=%RTbool, fStopOk=%RTbool, Set=%RU8, Act=%RU8\n",
                   CODEC_F05_IS_RESET(u32Reg), CODEC_F05_IS_STOPOK(u32Reg), CODEC_F05_SET(u32Reg), CODEC_F05_ACT(u32Reg));
}

static void codecDbgPrintNodeRegA(PCODECDBGINFO pInfo, uint32_t u32Reg)
{
    codecDbgPrintf(pInfo, "RegA: %x\n", u32Reg);
}

static void codecDbgPrintNodeRegF00(PCODECDBGINFO pInfo, uint32_t *paReg00)
{
    codecDbgPrintf(pInfo, "Parameters (F00):\n");

    CODECDBG_INDENT
        codecDbgPrintf(pInfo, "Connections: %RU8\n", CODEC_F00_0E_COUNT(paReg00[0xE]));
        codecDbgPrintf(pInfo, "Amplifier Caps:\n");
        uint32_t uReg = paReg00[0xD];
        CODECDBG_INDENT
            codecDbgPrintf(pInfo, "Input Steps=%02RU8, StepSize=%02RU8, StepOff=%02RU8, fCanMute=%RTbool\n",
                           CODEC_F00_0D_NUM_STEPS(uReg),
                           CODEC_F00_0D_STEP_SIZE(uReg),
                           CODEC_F00_0D_OFFSET(uReg),
                           RT_BOOL(CODEC_F00_0D_IS_CAP_MUTE(uReg)));

            uReg = paReg00[0x12];
            codecDbgPrintf(pInfo, "Output Steps=%02RU8, StepSize=%02RU8, StepOff=%02RU8, fCanMute=%RTbool\n",
                           CODEC_F00_12_NUM_STEPS(uReg),
                           CODEC_F00_12_STEP_SIZE(uReg),
                           CODEC_F00_12_OFFSET(uReg),
                           RT_BOOL(CODEC_F00_12_IS_CAP_MUTE(uReg)));
        CODECDBG_UNINDENT
    CODECDBG_UNINDENT
}

static void codecDbgPrintNodeAmp(PCODECDBGINFO pInfo, uint32_t *paReg, uint8_t uIdx, uint8_t uDir)
{
#define CODECDBG_AMP(reg, chan) \
    codecDbgPrintf(pInfo, "Amp %RU8 %s %s: In=%RTbool, Out=%RTbool, Left=%RTbool, Right=%RTbool, Idx=%RU8, fMute=%RTbool, uGain=%RU8\n", \
                   uIdx, chan, uDir == AMPLIFIER_IN ? "In" : "Out", \
                   RT_BOOL(CODEC_SET_AMP_IS_IN_DIRECTION(reg)), RT_BOOL(CODEC_SET_AMP_IS_OUT_DIRECTION(reg)), \
                   RT_BOOL(CODEC_SET_AMP_IS_LEFT_SIDE(reg)), RT_BOOL(CODEC_SET_AMP_IS_RIGHT_SIDE(reg)), \
                   CODEC_SET_AMP_INDEX(reg), RT_BOOL(CODEC_SET_AMP_MUTE(reg)), CODEC_SET_AMP_GAIN(reg));

    uint32_t regAmp = AMPLIFIER_REGISTER(paReg, uDir, AMPLIFIER_LEFT, uIdx);
    CODECDBG_AMP(regAmp, "Left");
    regAmp = AMPLIFIER_REGISTER(paReg, uDir, AMPLIFIER_RIGHT, uIdx);
    CODECDBG_AMP(regAmp, "Right");

#undef CODECDBG_AMP
}

#if 0 /* unused */
static void codecDbgPrintNodeConnections(PCODECDBGINFO pInfo, PCODECNODE pNode)
{
    if (pNode->node.au32F00_param[0xE] == 0) /* Directly connected to HDA link. */
    {
         codecDbgPrintf(pInfo, "[HDA LINK]\n");
         return;
    }
}
#endif

static void codecDbgPrintNode(PCODECDBGINFO pInfo, PCODECNODE pNode, bool fRecursive)
{
    codecDbgPrintf(pInfo, "Node 0x%02x (%02RU8): ", pNode->node.uID, pNode->node.uID);

    if (pNode->node.uID == STAC9220_NID_ROOT)
    {
        CODECDBG_PRINT("ROOT\n");
    }
    else if (pNode->node.uID == STAC9220_NID_AFG)
    {
        CODECDBG_PRINT("AFG\n");
        CODECDBG_INDENT
            codecDbgPrintNodeRegF00(pInfo, pNode->node.au32F00_param);
            codecDbgPrintNodeRegF05(pInfo, pNode->afg.u32F05_param);
        CODECDBG_UNINDENT
    }
    else if (hdaCodecIsPortNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("PORT\n");
    }
    else if (hdaCodecIsDacNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("DAC\n");
        CODECDBG_INDENT
            codecDbgPrintNodeRegF00(pInfo, pNode->node.au32F00_param);
            codecDbgPrintNodeRegF05(pInfo, pNode->dac.u32F05_param);
            codecDbgPrintNodeRegA  (pInfo, pNode->dac.u32A_param);
            codecDbgPrintNodeAmp   (pInfo, pNode->dac.B_params, 0, AMPLIFIER_OUT);
        CODECDBG_UNINDENT
    }
    else if (hdaCodecIsAdcVolNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("ADC VOLUME\n");
        CODECDBG_INDENT
            codecDbgPrintNodeRegF00(pInfo, pNode->node.au32F00_param);
            codecDbgPrintNodeRegA  (pInfo, pNode->adcvol.u32A_params);
            codecDbgPrintNodeAmp   (pInfo, pNode->adcvol.B_params, 0, AMPLIFIER_IN);
        CODECDBG_UNINDENT
    }
    else if (hdaCodecIsAdcNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("ADC\n");
        CODECDBG_INDENT
            codecDbgPrintNodeRegF00(pInfo, pNode->node.au32F00_param);
            codecDbgPrintNodeRegF05(pInfo, pNode->adc.u32F05_param);
            codecDbgPrintNodeRegA  (pInfo, pNode->adc.u32A_param);
            codecDbgPrintNodeAmp   (pInfo, pNode->adc.B_params, 0, AMPLIFIER_IN);
        CODECDBG_UNINDENT
    }
    else if (hdaCodecIsAdcMuxNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("ADC MUX\n");
        CODECDBG_INDENT
            codecDbgPrintNodeRegF00(pInfo, pNode->node.au32F00_param);
            codecDbgPrintNodeRegA  (pInfo, pNode->adcmux.u32A_param);
            codecDbgPrintNodeAmp   (pInfo, pNode->adcmux.B_params, 0, AMPLIFIER_IN);
        CODECDBG_UNINDENT
    }
    else if (hdaCodecIsPcbeepNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("PC BEEP\n");
    }
    else if (hdaCodecIsSpdifOutNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("SPDIF OUT\n");
    }
    else if (hdaCodecIsSpdifInNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("SPDIF IN\n");
    }
    else if (hdaCodecIsDigInPinNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("DIGITAL IN PIN\n");
    }
    else if (hdaCodecIsDigOutPinNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("DIGITAL OUT PIN\n");
    }
    else if (hdaCodecIsCdNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("CD\n");
    }
    else if (hdaCodecIsVolKnobNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("VOLUME KNOB\n");
    }
    else if (hdaCodecIsReservedNode(pInfo->pThis, pNode->node.uID))
    {
        CODECDBG_PRINT("RESERVED\n");
    }
    else
        CODECDBG_PRINT("UNKNOWN TYPE 0x%x\n", pNode->node.uID);

    if (fRecursive)
    {
#define CODECDBG_PRINT_CONLIST_ENTRY(_aNode, _aEntry)                              \
        if (cCnt >= _aEntry)                                                       \
        {                                                                          \
            const uint8_t uID = RT_BYTE##_aEntry(_aNode->node.au32F02_param[0x0]); \
            if (pNode->node.uID == uID)                                             \
                codecDbgPrintNode(pInfo, _aNode, false /* fRecursive */);          \
        }

        /* Slow recursion, but this is debug stuff anyway. */
        for (uint8_t i = 0; i < pInfo->pThis->cTotalNodes; i++)
        {
            const PCODECNODE pSubNode = &pInfo->pThis->paNodes[i];
            if (pSubNode->node.uID == pNode->node.uID)
                continue;

            const uint8_t cCnt = CODEC_F00_0E_COUNT(pSubNode->node.au32F00_param[0xE]);
            if (cCnt == 0) /* No connections present? Skip. */
                continue;

            CODECDBG_INDENT
                CODECDBG_PRINT_CONLIST_ENTRY(pSubNode, 1)
                CODECDBG_PRINT_CONLIST_ENTRY(pSubNode, 2)
                CODECDBG_PRINT_CONLIST_ENTRY(pSubNode, 3)
                CODECDBG_PRINT_CONLIST_ENTRY(pSubNode, 4)
            CODECDBG_UNINDENT
        }

#undef CODECDBG_PRINT_CONLIST_ENTRY
   }
}

static DECLCALLBACK(void) codecDbgListNodes(PHDACODEC pThis, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pszArgs);
    pHlp->pfnPrintf(pHlp, "HDA LINK / INPUTS\n");

    CODECDBGINFO dbgInfo;
    dbgInfo.pHlp   = pHlp;
    dbgInfo.pThis  = pThis;
    dbgInfo.uLevel = 0;

    PCODECDBGINFO pInfo = &dbgInfo;

    CODECDBG_INDENT
        for (uint8_t i = 0; i < pThis->cTotalNodes; i++)
        {
            PCODECNODE pNode = &pThis->paNodes[i];

            /* Start with all nodes which have connection entries set. */
            if (CODEC_F00_0E_COUNT(pNode->node.au32F00_param[0xE]))
                codecDbgPrintNode(&dbgInfo, pNode, true /* fRecursive */);
        }
    CODECDBG_UNINDENT
}

static DECLCALLBACK(void) codecDbgSelector(PHDACODEC pThis, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pThis, pHlp, pszArgs);
}
#endif

static DECLCALLBACK(int) codecLookup(PHDACODEC pThis, uint32_t cmd, uint64_t *puResp)
{
    AssertPtrReturn(pThis,  VERR_INVALID_POINTER);
    AssertPtrReturn(puResp, VERR_INVALID_POINTER);

    if (CODEC_CAD(cmd) != pThis->id)
    {
        *puResp = 0;
        AssertMsgFailed(("Unknown codec address 0x%x\n", CODEC_CAD(cmd)));
        return VERR_INVALID_PARAMETER;
    }

    if (   CODEC_VERBDATA(cmd) == 0
        || CODEC_NID(cmd) >= pThis->cTotalNodes)
    {
        *puResp = 0;
        AssertMsgFailed(("[NID0x%02x] Unknown / invalid node or data (0x%x)\n", CODEC_NID(cmd), CODEC_VERBDATA(cmd)));
        return VERR_INVALID_PARAMETER;
    }

    /** @todo r=andy Implement a binary search here. */
    for (size_t i = 0; i < pThis->cVerbs; i++)
    {
        if ((CODEC_VERBDATA(cmd) & pThis->paVerbs[i].mask) == pThis->paVerbs[i].verb)
        {
            int rc2 = pThis->paVerbs[i].pfn(pThis, cmd, puResp);
            AssertRC(rc2);
            Log3Func(("[NID0x%02x] (0x%x) %s: 0x%x -> 0x%x\n",
                      CODEC_NID(cmd), pThis->paVerbs[i].verb, pThis->paVerbs[i].pszName, CODEC_VERB_PAYLOAD8(cmd), *puResp));
            return rc2;
        }
    }

    *puResp = 0;
    LogFunc(("[NID0x%02x] Callback for %x not found\n", CODEC_NID(cmd), CODEC_VERBDATA(cmd)));
    return VERR_NOT_FOUND;
}

/*
 * APIs exposed to DevHDA.
 */

int hdaCodecAddStream(PHDACODEC pThis, PDMAUDIOMIXERCTL enmMixerCtl, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,  VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    switch (enmMixerCtl)
    {
        case PDMAUDIOMIXERCTL_VOLUME_MASTER:
        case PDMAUDIOMIXERCTL_FRONT:
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
        case PDMAUDIOMIXERCTL_CENTER_LFE:
        case PDMAUDIOMIXERCTL_REAR:
#endif
        {
            break;
        }
        case PDMAUDIOMIXERCTL_LINE_IN:
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
        case PDMAUDIOMIXERCTL_MIC_IN:
#endif
        {
            break;
        }
        default:
            AssertMsgFailed(("Mixer control %d not implemented\n", enmMixerCtl));
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (RT_SUCCESS(rc))
        rc = pThis->pfnCbMixerAddStream(pThis->pHDAState, enmMixerCtl, pCfg);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int hdaCodecRemoveStream(PHDACODEC pThis, PDMAUDIOMIXERCTL enmMixerCtl)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    int rc = pThis->pfnCbMixerRemoveStream(pThis->pHDAState, enmMixerCtl);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int hdaCodecSaveState(PHDACODEC pThis, PSSMHANDLE pSSM)
{
    AssertLogRelMsgReturn(pThis->cTotalNodes == STAC9221_NUM_NODES, ("cTotalNodes=%#x, should be 0x1c", pThis->cTotalNodes),
                          VERR_INTERNAL_ERROR);
    SSMR3PutU32(pSSM, pThis->cTotalNodes);
    for (unsigned idxNode = 0; idxNode < pThis->cTotalNodes; ++idxNode)
        SSMR3PutStructEx(pSSM, &pThis->paNodes[idxNode].SavedState, sizeof(pThis->paNodes[idxNode].SavedState),
                         0 /*fFlags*/, g_aCodecNodeFields, NULL /*pvUser*/);
    return VINF_SUCCESS;
}

int hdaCodecLoadState(PHDACODEC pThis, PSSMHANDLE pSSM, uint32_t uVersion)
{
    int rc = VINF_SUCCESS;

    PCSSMFIELD pFields = NULL;
    uint32_t   fFlags  = 0;
    switch (uVersion)
    {
        case HDA_SSM_VERSION_1:
            AssertReturn(pThis->cTotalNodes == 0x1c, VERR_INTERNAL_ERROR);
            pFields = g_aCodecNodeFieldsV1;
            fFlags  = SSMSTRUCT_FLAGS_MEM_BAND_AID_RELAXED;
            break;

        case HDA_SSM_VERSION_2:
        case HDA_SSM_VERSION_3:
            AssertReturn(pThis->cTotalNodes == 0x1c, VERR_INTERNAL_ERROR);
            pFields = g_aCodecNodeFields;
            fFlags  = SSMSTRUCT_FLAGS_MEM_BAND_AID_RELAXED;
            break;

        /* Since version 4 a flexible node count is supported. */
        case HDA_SSM_VERSION_4:
        case HDA_SSM_VERSION_5:
        case HDA_SSM_VERSION:
        {
            uint32_t cNodes;
            int rc2 = SSMR3GetU32(pSSM, &cNodes);
            AssertRCReturn(rc2, rc2);
            if (cNodes != 0x1c)
                return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
            AssertReturn(pThis->cTotalNodes == 0x1c, VERR_INTERNAL_ERROR);

            pFields = g_aCodecNodeFields;
            fFlags  = 0;
            break;
        }

        default:
            rc = VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        for (unsigned idxNode = 0; idxNode < pThis->cTotalNodes; ++idxNode)
        {
            uint8_t idOld = pThis->paNodes[idxNode].SavedState.Core.uID;
            int rc2 = SSMR3GetStructEx(pSSM, &pThis->paNodes[idxNode].SavedState,
                                       sizeof(pThis->paNodes[idxNode].SavedState),
                                       fFlags, pFields, NULL);
            if (RT_SUCCESS(rc))
                rc = rc2;

            if (RT_FAILURE(rc))
                break;

            AssertLogRelMsgReturn(idOld == pThis->paNodes[idxNode].SavedState.Core.uID,
                                  ("loaded %#x, expected %#x\n", pThis->paNodes[idxNode].SavedState.Core.uID, idOld),
                                  VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
        }

        if (RT_SUCCESS(rc))
        {
            /*
             * Update stuff after changing the state.
             */
            PCODECNODE pNode;
            if (hdaCodecIsDacNode(pThis, pThis->u8DacLineOut))
            {
                pNode = &pThis->paNodes[pThis->u8DacLineOut];
                hdaCodecToAudVolume(pThis, pNode, &pNode->dac.B_params, PDMAUDIOMIXERCTL_FRONT);
            }
            else if (hdaCodecIsSpdifOutNode(pThis, pThis->u8DacLineOut))
            {
                pNode = &pThis->paNodes[pThis->u8DacLineOut];
                hdaCodecToAudVolume(pThis, pNode, &pNode->spdifout.B_params, PDMAUDIOMIXERCTL_FRONT);
            }

            pNode = &pThis->paNodes[pThis->u8AdcVolsLineIn];
            hdaCodecToAudVolume(pThis, pNode, &pNode->adcvol.B_params, PDMAUDIOMIXERCTL_LINE_IN);
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Powers off the codec.
 *
 * @param   pThis           Codec to power off.
 */
void hdaCodecPowerOff(PHDACODEC pThis)
{
    if (!pThis)
        return;

    LogFlowFuncEnter();

    LogRel2(("HDA: Powering off codec ...\n"));

    int rc2 = hdaCodecRemoveStream(pThis, PDMAUDIOMIXERCTL_FRONT);
    AssertRC(rc2);
#ifdef VBOX_WITH_AUDIO_HDA_51_SURROUND
    rc2 = hdaCodecRemoveStream(pThis, PDMAUDIOMIXERCTL_CENTER_LFE);
    AssertRC(rc2);
    rc2 = hdaCodecRemoveStream(pThis, PDMAUDIOMIXERCTL_REAR);
    AssertRC(rc2);
#endif

#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
    rc2 = hdaCodecRemoveStream(pThis, PDMAUDIOMIXERCTL_MIC_IN);
    AssertRC(rc2);
#endif
    rc2 = hdaCodecRemoveStream(pThis, PDMAUDIOMIXERCTL_LINE_IN);
    AssertRC(rc2);
}

void hdaCodecDestruct(PHDACODEC pThis)
{
    if (!pThis)
        return;

    LogFlowFuncEnter();

    if (pThis->paNodes)
    {
        RTMemFree(pThis->paNodes);
        pThis->paNodes = NULL;
    }
}

int hdaCodecConstruct(PPDMDEVINS pDevIns, PHDACODEC pThis,
                      uint16_t uLUN, PCFGMNODE pCfg)
{
    AssertPtrReturn(pDevIns, VERR_INVALID_POINTER);
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,    VERR_INVALID_POINTER);

    pThis->id      = uLUN;
    pThis->paVerbs = &g_aCodecVerbs[0];
    pThis->cVerbs  = RT_ELEMENTS(g_aCodecVerbs);

#ifdef DEBUG
    pThis->pfnDbgSelector  = codecDbgSelector;
    pThis->pfnDbgListNodes = codecDbgListNodes;
#endif
    pThis->pfnLookup       = codecLookup;

    int rc = stac9220Construct(pThis);
    AssertRCReturn(rc, rc);

    /* Common root node initializers. */
    pThis->paNodes[STAC9220_NID_ROOT].root.node.au32F00_param[0] = CODEC_MAKE_F00_00(pThis->u16VendorId, pThis->u16DeviceId);
    pThis->paNodes[STAC9220_NID_ROOT].root.node.au32F00_param[4] = CODEC_MAKE_F00_04(0x1, 0x1);

    /* Common AFG node initializers. */
    pThis->paNodes[STAC9220_NID_AFG].afg.node.au32F00_param[0x4] = CODEC_MAKE_F00_04(0x2, pThis->cTotalNodes - 2);
    pThis->paNodes[STAC9220_NID_AFG].afg.node.au32F00_param[0x5] = CODEC_MAKE_F00_05(1, CODEC_F00_05_AFG);
    pThis->paNodes[STAC9220_NID_AFG].afg.node.au32F00_param[0xA] = CODEC_F00_0A_44_1KHZ | CODEC_F00_0A_16_BIT;
    pThis->paNodes[STAC9220_NID_AFG].afg.u32F20_param = CODEC_MAKE_F20(pThis->u16VendorId, pThis->u8BSKU, pThis->u8AssemblyId);

    /*
     * Set initial volume.
     */
    PCODECNODE pNode = &pThis->paNodes[pThis->u8DacLineOut];
    hdaCodecToAudVolume(pThis, pNode, &pNode->dac.B_params, PDMAUDIOMIXERCTL_FRONT);

    pNode = &pThis->paNodes[pThis->u8AdcVolsLineIn];
    hdaCodecToAudVolume(pThis, pNode, &pNode->adcvol.B_params, PDMAUDIOMIXERCTL_LINE_IN);
#ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
# error "Implement mic-in support!"
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

