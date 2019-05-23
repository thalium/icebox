/* $Id: VMMDevTesting.h $ */
/** @file
 * VMMDev - Testing Extensions.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


#ifndef ___VBox_VMMDevTesting_h
#define ___VBox_VMMDevTesting_h

#include <VBox/types.h>


/** @defgroup grp_vmmdev_testing    VMM Device Testing
 * @ingroup grp_vmmdev
 * @{
 */

/** The base address of the MMIO range used for testing.
 * This is intentionally put at the 2nd page above 1M so that it can be
 * accessed from both real (!A20) and protected mode. */
#define VMMDEV_TESTING_MMIO_BASE        UINT32_C(0x00101000)
/** The size of the MMIO range used for testing.  */
#define VMMDEV_TESTING_MMIO_SIZE        UINT32_C(0x00001000)

/** MMIO offset: The NOP register - 1248 RW. */
#define VMMDEV_TESTING_MMIO_OFF_NOP         (0x000)
/** MMIO offset: The go-to-ring-3-NOP register - 1248 RW. */
#define VMMDEV_TESTING_MMIO_OFF_NOP_R3      (0x008)
/** MMIO offset: The readback registers - 64 bytes of read/write "memory". */
#define VMMDEV_TESTING_MMIO_OFF_READBACK    (0x040)
/** MMIO offset: Readback register view that always goes to ring-3. */
#define VMMDEV_TESTING_MMIO_OFF_READBACK_R3 (0x080)
/** The size of the MMIO readback registers. */
#define VMMDEV_TESTING_READBACK_SIZE        (0x40)

/** Default address of VMMDEV_TESTING_MMIO_OFF_NOP. */
#define VMMDEV_TESTING_MMIO_NOP             (VMMDEV_TESTING_MMIO_BASE + VMMDEV_TESTING_MMIO_OFF_NOP)
/** Default address of VMMDEV_TESTING_MMIO_OFF_NOP_R3. */
#define VMMDEV_TESTING_MMIO_NOP_R3          (VMMDEV_TESTING_MMIO_BASE + VMMDEV_TESTING_MMIO_OFF_NOP_R3)
/** Default address of VMMDEV_TESTING_MMIO_OFF_READBACK. */
#define VMMDEV_TESTING_MMIO_READBACK        (VMMDEV_TESTING_MMIO_BASE + VMMDEV_TESTING_MMIO_OFF_READBACK)
/** Default address of VMMDEV_TESTING_MMIO_OFF_READBACK_R3. */
#define VMMDEV_TESTING_MMIO_READBACK_R3     (VMMDEV_TESTING_MMIO_BASE + VMMDEV_TESTING_MMIO_OFF_READBACK_R3)

/** The real mode selector to use.
 * @remarks Requires that the A20 gate is enabled. */
#define VMMDEV_TESTING_MMIO_RM_SEL       0xffff
/** Calculate the real mode offset of a MMIO register. */
#define VMMDEV_TESTING_MMIO_RM_OFF(val)  ((val) - 0xffff0)
/** Calculate the real mode offset of a MMIO register offset. */
#define VMMDEV_TESTING_MMIO_RM_OFF2(off) ((off) + 16 + 0x1000)

/** The base port of the I/O range used for testing. */
#define VMMDEV_TESTING_IOPORT_BASE      0x0510
/** The number of I/O ports reserved for testing. */
#define VMMDEV_TESTING_IOPORT_COUNT     0x0010
/** The NOP I/O port - 1,2,4 RW. */
#define VMMDEV_TESTING_IOPORT_NOP       (VMMDEV_TESTING_IOPORT_BASE + 0)
/** The low nanosecond timestamp - 4 RO.  */
#define VMMDEV_TESTING_IOPORT_TS_LOW    (VMMDEV_TESTING_IOPORT_BASE + 1)
/** The high nanosecond timestamp - 4 RO.  Read this after the low one!  */
#define VMMDEV_TESTING_IOPORT_TS_HIGH   (VMMDEV_TESTING_IOPORT_BASE + 2)
/** Command register usually used for preparing the data register - 4/2 WO. */
#define VMMDEV_TESTING_IOPORT_CMD       (VMMDEV_TESTING_IOPORT_BASE + 3)
/** Data register which use depends on the current command - 1s, 4 WO. */
#define VMMDEV_TESTING_IOPORT_DATA      (VMMDEV_TESTING_IOPORT_BASE + 4)
/** The go-to-ring-3-NOP I/O port - 1,2,4 RW. */
#define VMMDEV_TESTING_IOPORT_NOP_R3    (VMMDEV_TESTING_IOPORT_BASE + 5)

/** @name Commands.
 * @{ */
/** Initialize test, sending name (zero terminated string). (RTTestCreate) */
#define VMMDEV_TESTING_CMD_INIT         UINT32_C(0xcab1e000)
/** Test done, sending 32-bit total error count with it. (RTTestSummaryAndDestroy) */
#define VMMDEV_TESTING_CMD_TERM         UINT32_C(0xcab1e001)
/** Start a new sub-test, sending name (zero terminated string). (RTTestSub) */
#define VMMDEV_TESTING_CMD_SUB_NEW      UINT32_C(0xcab1e002)
/** Sub-test is done, sending 32-bit error count for it. (RTTestDone) */
#define VMMDEV_TESTING_CMD_SUB_DONE     UINT32_C(0xcab1e003)
/** Report a failure, sending reason (zero terminated string). (RTTestFailed) */
#define VMMDEV_TESTING_CMD_FAILED       UINT32_C(0xcab1e004)
/** Report a value, sending the 64-bit value (2x4), the 32-bit unit (4), and
 * finally the name (zero terminated string).  (RTTestValue) */
#define VMMDEV_TESTING_CMD_VALUE        UINT32_C(0xcab1e005)
/** Report a failure, sending reason (zero terminated string). (RTTestSkipped) */
#define VMMDEV_TESTING_CMD_SKIPPED      UINT32_C(0xcab1e006)
/** Report a value found in a VMM register, sending a string on the form
 * "value-name:register-name". */
#define VMMDEV_TESTING_CMD_VALUE_REG    UINT32_C(0xcab1e007)
/** Print string, sending a string including newline. (RTTestPrintf) */
#define VMMDEV_TESTING_CMD_PRINT        UINT32_C(0xcab1e008)

/** The magic part of the command. */
#define VMMDEV_TESTING_CMD_MAGIC        UINT32_C(0xcab1e000)
/** The magic part of the command. */
#define VMMDEV_TESTING_CMD_MAGIC_MASK   UINT32_C(0xffffff00)
/** The magic high word automatically supplied to 16-bit CMD writes. */
#define VMMDEV_TESTING_CMD_MAGIC_HI_WORD UINT32_C(0xcab10000)
/** @} */

/** @name Value units
 * @{ */
#define VMMDEV_TESTING_UNIT_PCT                 UINT8_C(0x01)   /**< Percentage. */
#define VMMDEV_TESTING_UNIT_BYTES               UINT8_C(0x02)   /**< Bytes. */
#define VMMDEV_TESTING_UNIT_BYTES_PER_SEC       UINT8_C(0x03)   /**< Bytes per second. */
#define VMMDEV_TESTING_UNIT_KILOBYTES           UINT8_C(0x04)   /**< Kilobytes. */
#define VMMDEV_TESTING_UNIT_KILOBYTES_PER_SEC   UINT8_C(0x05)   /**< Kilobytes per second. */
#define VMMDEV_TESTING_UNIT_MEGABYTES           UINT8_C(0x06)   /**< Megabytes. */
#define VMMDEV_TESTING_UNIT_MEGABYTES_PER_SEC   UINT8_C(0x07)   /**< Megabytes per second. */
#define VMMDEV_TESTING_UNIT_PACKETS             UINT8_C(0x08)   /**< Packets. */
#define VMMDEV_TESTING_UNIT_PACKETS_PER_SEC     UINT8_C(0x09)   /**< Packets per second. */
#define VMMDEV_TESTING_UNIT_FRAMES              UINT8_C(0x0a)   /**< Frames. */
#define VMMDEV_TESTING_UNIT_FRAMES_PER_SEC      UINT8_C(0x0b)   /**< Frames per second. */
#define VMMDEV_TESTING_UNIT_OCCURRENCES         UINT8_C(0x0c)   /**< Occurrences. */
#define VMMDEV_TESTING_UNIT_OCCURRENCES_PER_SEC UINT8_C(0x0d)   /**< Occurrences per second. */
#define VMMDEV_TESTING_UNIT_CALLS               UINT8_C(0x0e)   /**< Calls. */
#define VMMDEV_TESTING_UNIT_CALLS_PER_SEC       UINT8_C(0x0f)   /**< Calls per second. */
#define VMMDEV_TESTING_UNIT_ROUND_TRIP          UINT8_C(0x10)   /**< Round trips. */
#define VMMDEV_TESTING_UNIT_SECS                UINT8_C(0x11)   /**< Seconds. */
#define VMMDEV_TESTING_UNIT_MS                  UINT8_C(0x12)   /**< Milliseconds. */
#define VMMDEV_TESTING_UNIT_NS                  UINT8_C(0x13)   /**< Nanoseconds. */
#define VMMDEV_TESTING_UNIT_NS_PER_CALL         UINT8_C(0x14)   /**< Nanoseconds per call. */
#define VMMDEV_TESTING_UNIT_NS_PER_FRAME        UINT8_C(0x15)   /**< Nanoseconds per frame. */
#define VMMDEV_TESTING_UNIT_NS_PER_OCCURRENCE   UINT8_C(0x16)   /**< Nanoseconds per occurrence. */
#define VMMDEV_TESTING_UNIT_NS_PER_PACKET       UINT8_C(0x17)   /**< Nanoseconds per frame. */
#define VMMDEV_TESTING_UNIT_NS_PER_ROUND_TRIP   UINT8_C(0x18)   /**< Nanoseconds per round trip. */
#define VMMDEV_TESTING_UNIT_INSTRS              UINT8_C(0x19)   /**< Instructions. */
#define VMMDEV_TESTING_UNIT_INSTRS_PER_SEC      UINT8_C(0x1a)   /**< Instructions per second. */
#define VMMDEV_TESTING_UNIT_NONE                UINT8_C(0x1b)   /**< No unit. */
/** @}  */


/** What the NOP accesses returns. */
#define VMMDEV_TESTING_NOP_RET          UINT32_C(0x64726962) /* bird */

/** @} */

#endif

