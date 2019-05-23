/* $Id: DevVGASavedState.h $ */
/** @file
 * DevVGA - Saved state versions.
 *
 * @remarks HGSMI needs this but doesn't want to deal with DevVGA.h, thus this
 *          dedicated header.
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


#ifndef ___Graphics_DevVGASavedState_h
#define ___Graphics_DevVGASavedState_h

/** Creates an eyecatching marker in the VGA saved state ("<uSub>Marker\n"). */
#define VGA_SAVED_STATE_MAKE_MARKER(uSub) (UINT64_C(0x0a72656b72614d30) + (uint64_t)(uSub))

/** Puts a marker. Status code is not checked. */
#define VGA_SAVED_STATE_PUT_MARKER(pSSM, uSub) \
    do { SSMR3PutU64(pSSM, VGA_SAVED_STATE_MAKE_MARKER(uSub)); } while (0)

/** Retrieves a VGA saved state marker and checks that it matches, if it
 *  doesn't assert/LogRel and return. */
#define VGA_SAVED_STATE_GET_MARKER_RETURN_ON_MISMATCH(pSSM, uVersion, uSub) \
    do { \
        if (uVersion >= VGA_SAVEDSTATE_VERSION_MARKERS) \
        { \
            uint64_t uMarker; \
            int rcMarker = SSMR3GetU64(pSSM, &uMarker); \
            AssertLogRelRCReturn(rcMarker, rcMarker); \
            AssertLogRelMsgReturn(uMarker == VGA_SAVED_STATE_MAKE_MARKER(uSub), \
                                  ("Bad VGA marker: expected %llx, got %llx\n", VGA_SAVED_STATE_MAKE_MARKER(uSub), uMarker), \
                                  VERR_SSM_DATA_UNIT_FORMAT_CHANGED); \
        } \
    } while (0)

#define VGA_SAVEDSTATE_VERSION              19
#define VGA_SAVEDSTATE_VERSION_VMSVGA_TEX_STAGES 19
#define VGA_SAVEDSTATE_VERSION_VMSVGA_GMR_COUNT  18
#define VGA_SAVEDSTATE_VERSION_VMSVGA_VGA_FB_FIX 17
#define VGA_SAVEDSTATE_VERSION_MARKERS      16
#define VGA_SAVEDSTATE_VERSION_MODE_HINTS   15
#define VGA_SAVEDSTATE_VERSION_FIXED_PENDVHWA 14
#define VGA_SAVEDSTATE_VERSION_3D           13
#define VGA_SAVEDSTATE_VERSION_HGSMIMA      12 /* HGSMI memory allocator. */
#define VGA_SAVEDSTATE_VERSION_VMSVGA_2D    10 /* <- internal build with 2d state only */
#define VGA_SAVEDSTATE_VERSION_WITH_PENDVHWA 10
#define VGA_SAVEDSTATE_VERSION_INV_GCMDFIFO 8 /* <- states upto and including this version may contain invalid completed Guest Commands fifo entries */
#define VGA_SAVEDSTATE_VERSION_INV_VHEIGHT  8 /* <- states upto and including this version may contain invalid vbe_regs[VBE_DISPI_INDEX_VIRT_HEIGHT] value */
#define VGA_SAVEDSTATE_VERSION_WDDM         7
#define VGA_SAVEDSTATE_VERSION_PRE_WDDM     6
#define VGA_SAVEDSTATE_VERSION_HOST_HEAP    5
#define VGA_SAVEDSTATE_VERSION_WITH_CONFIG  4
#define VGA_SAVEDSTATE_VERSION_HGSMI        3
#define VGA_SAVEDSTATE_VERSION_PRE_HGSMI    2
#define VGA_SAVEDSTATE_VERSION_ANCIENT      1

#endif

