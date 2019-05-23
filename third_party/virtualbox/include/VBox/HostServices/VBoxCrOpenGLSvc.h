/** @file
 * OpenGL:
 * Common header for host service and guest clients.
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

#ifndef ___VBox_HostService_VBoxCrOpenGLSvc_h
#define ___VBox_HostService_VBoxCrOpenGLSvc_h

#include <VBox/VMMDevCoreTypes.h>
#include <VBox/VBoxGuestCoreTypes.h>
#include <VBox/hgcmsvc.h>
#include <VBox/Graphics/VBoxVideo.h>
#include <VBox/Graphics/VBoxVideoHost3D.h>

/* crOpenGL host functions */
#define SHCRGL_HOST_FN_SET_CONSOLE (1)
#define SHCRGL_HOST_FN_SET_VISIBLE_REGION (5)
#define SHCRGL_HOST_FN_SET_VM (7)
#define SHCRGL_HOST_FN_SCREEN_CHANGED (8)
#ifdef VBOX_WITH_CRHGSMI
#define SHCRGL_HOST_FN_CRHGSMI_CMD (10)
#define SHCRGL_HOST_FN_CRHGSMI_CTL (11)
#endif
#define SHCRGL_HOST_FN_VIEWPORT_CHANGED (15)
#define SHCRGL_HOST_FN_SET_OUTPUT_REDIRECT (20)
#define SHCRGL_HOST_FN_DEV_RESIZE          (22)
#define SHCRGL_HOST_FN_VIEWPORT_CHANGED2 (23)
#define SHCRGL_HOST_FN_TAKE_SCREENSHOT (24)
#define SHCRGL_HOST_FN_WINDOWS_SHOW (25)
#define SHCRGL_HOST_FN_CTL          (26)
#define SHCRGL_HOST_FN_SET_SCALE_FACTOR (27)
#define SHCRGL_HOST_FN_SET_UNSCALED_HIDPI (28)

/* crOpenGL guest functions */
#define SHCRGL_GUEST_FN_WRITE       (2)
#define SHCRGL_GUEST_FN_READ        (3)
#define SHCRGL_GUEST_FN_WRITE_READ  (4)
#define SHCRGL_GUEST_FN_SET_VERSION (6)
#define SHCRGL_GUEST_FN_INJECT      (9)
#define SHCRGL_GUEST_FN_SET_PID     (12)
#define SHCRGL_GUEST_FN_WRITE_BUFFER        (13)
#define SHCRGL_GUEST_FN_WRITE_READ_BUFFERED (14)
#define SHCRGL_GUEST_FN_GET_CAPS_LEGACY     (15)
#define SHCRGL_GUEST_FN_GET_CAPS_NEW     (16)

/* Parameters count */
#define SHCRGL_CPARMS_SET_CONSOLE (1)
#define SHCRGL_CPARMS_SET_VM (1)
#define SHCRGL_CPARMS_SET_VISIBLE_REGION (1)
#define SHCRGL_CPARMS_WRITE      (1)
#define SHCRGL_CPARMS_READ       (2)
#define SHCRGL_CPARMS_WRITE_READ (3)
#define SHCRGL_CPARMS_SET_VERSION (2)
#define SHCRGL_CPARMS_SCREEN_CHANGED (1)
#define SHCRGL_CPARMS_DEV_RESIZE (1)
#define SHCRGL_CPARMS_INJECT (2)
#define SHCRGL_CPARMS_SET_PID (1)
#define SHCRGL_CPARMS_WRITE_BUFFER        (4)
#define SHCRGL_CPARMS_WRITE_READ_BUFFERED (3)
#define SHCRGL_CPARMS_SET_OUTPUT_REDIRECT (1)
#define SHCRGL_CPARMS_CRCMD_NOTIFY_CMDS (0)
#define SHCRGL_CPARMS_VIEWPORT_CHANGED (5)
#define SHCRGL_CPARMS_VIEWPORT_CHANGED2 (1)
#define SHCRGL_CPARMS_GET_CAPS_LEGACY (1)
#define SHCRGL_CPARMS_GET_CAPS_NEW (1)

/* @todo Move to H3DOR.h begin */

/* Names of supported output redirect formats. */
#define H3DOR_FMT_RGBA_TOPDOWN "H3DOR_FMT_RGBA_TOPDOWN"
#define H3DOR_FMT_RGBA "H3DOR_FMT_RGBA"

/* Comma separated list of output formats supported by the output redirect target. */
#define H3DOR_PROP_FORMATS 0

#pragma pack(1)
typedef struct {
    /* The caller's context of the redirection. */
    const void *pvContext;
    /* Inform caller that a new window will be redirected. */
    DECLR3CALLBACKMEMBER(void, H3DORBegin,           (const void *pvContext, void **ppvInstance,
                                                      const char *pszFormat));
    /* The window dimension has been changed. */
    DECLR3CALLBACKMEMBER(void, H3DORGeometry,        (void *pvInstance,
                                                      int32_t x, int32_t y, uint32_t w, uint32_t h));
    /* Update the window visible region. */
    DECLR3CALLBACKMEMBER(void, H3DORVisibleRegion,   (void *pvInstance,
                                                      uint32_t cRects, const RTRECT *paRects));
    /* A rendered 3D frame is ready. Format of pvData is "pszFormat" parameter of H3DORBegin. */
    DECLR3CALLBACKMEMBER(void, H3DORFrame,           (void *pvInstance,
                                                      void *pvData, uint32_t cbData));
    /* The window is closed. */
    DECLR3CALLBACKMEMBER(void, H3DOREnd,             (void *pvInstance));
    /* Obtain caller's parameters: the list of supported formats, etc. */
    DECLR3CALLBACKMEMBER(int,  H3DORContextProperty, (const void *pvContext, uint32_t index,
                                                      void *pvBuffer, uint32_t cbBuffer, uint32_t *pcbOut));
} H3DOUTPUTREDIRECT;
#pragma pack()

/* @todo Move to H3DOR.h end */

#ifdef VBOX_WITH_CRHGSMI
#pragma pack(1)
typedef struct
{
    int32_t result;           /**< OUT Host HGSMI return code.*/
    uint32_t u32ClientID;     /**< IN  The id of the caller. */
    uint32_t u32Function;     /**< IN  Function number. */
    uint32_t u32Reserved;
} CRVBOXHGSMIHDR;
AssertCompileSize(CRVBOXHGSMIHDR, 16);

/** GUEST_FN_WRITE Parameters structure. */
typedef struct
{
    CRVBOXHGSMIHDR hdr;
    /** buffer index, in
     *  Data buffer
     */
    uint32_t iBuffer;
} CRVBOXHGSMIWRITE;

/** GUEST_FN_READ Parameters structure. */
typedef struct
{
    CRVBOXHGSMIHDR hdr;

    /** buffer index, in/out
     *  Data buffer
     */
    uint32_t iBuffer;
    uint32_t cbBuffer;
} CRVBOXHGSMIREAD;

/** GUEST_FN_WRITE_READ Parameters structure. */
typedef struct
{
    CRVBOXHGSMIHDR hdr;

    /** buffer index, in
     *  Data buffer
     */
    uint32_t iBuffer;

    /** buffer index, out
     *  Writeback buffer
     */
    uint32_t iWriteback;
    uint32_t cbWriteback;
} CRVBOXHGSMIWRITEREAD;

/** GUEST_FN_SET_VERSION Parameters structure. */
typedef struct
{
    CRVBOXHGSMIHDR hdr;

    /** 32bit, in
     *  Major version
     */
    uint32_t vMajor;

    /** 32bit, in
     *  Minor version
     */
    uint32_t vMinor;
} CRVBOXHGSMISETVERSION;

/** GUEST_FN_INJECT Parameters structure. */
typedef struct
{
    CRVBOXHGSMIHDR hdr;

    /** 32bit, in
     *  ClientID to inject commands buffer for
     */
    uint32_t u32ClientID;
    /** buffer index, in
     *  Data buffer
     */
    uint32_t iBuffer;
} CRVBOXHGSMIINJECT;

/** GUEST_FN_SET_PID Parameters structure. */
typedef struct
{
    CRVBOXHGSMIHDR hdr;

    /** 64bit, in
     *  PID
     */
    uint64_t   u64PID;
} CRVBOXHGSMISETPID;

#pragma pack()
#endif
/**
 * SHCRGL_GUEST_FN_WRITE
 */

/** GUEST_FN_WRITE Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** pointer, in
     *  Data buffer
     */
    HGCMFunctionParameter   pBuffer;
} CRVBOXHGCMWRITE;

/** GUEST_FN_READ Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** pointer, in/out
     *  Data buffer
     */
    HGCMFunctionParameter   pBuffer;

    /** 32bit, out
     * Count of bytes written to buffer
     */
    HGCMFunctionParameter   cbBuffer;

} CRVBOXHGCMREAD;

/** GUEST_FN_WRITE_READ Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** pointer, in
     *  Data buffer
     */
    HGCMFunctionParameter   pBuffer;

    /** pointer, out
     *  Writeback buffer
     */
    HGCMFunctionParameter   pWriteback;

    /** 32bit, out
     * Count of bytes written to writeback buffer
     */
    HGCMFunctionParameter   cbWriteback;

} CRVBOXHGCMWRITEREAD;

/** GUEST_FN_SET_VERSION Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 32bit, in
     *  Major version
     */
    HGCMFunctionParameter   vMajor;

    /** 32bit, in
     *  Minor version
     */
    HGCMFunctionParameter   vMinor;

} CRVBOXHGCMSETVERSION;

/** GUEST_FN_GET_CAPS Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 32bit, out
     *  Caps
     */
    HGCMFunctionParameter   Caps;
} CRVBOXHGCMGETCAPS;

/** GUEST_FN_INJECT Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 32bit, in
     *  ClientID to inject commands buffer for
     */
    HGCMFunctionParameter   u32ClientID;
    /** pointer, in
     *  Data buffer
     */
    HGCMFunctionParameter   pBuffer;
} CRVBOXHGCMINJECT;

/** GUEST_FN_SET_PID Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 64bit, in
     *  PID
     */
    HGCMFunctionParameter   u64PID;
} CRVBOXHGCMSETPID;

/** GUEST_FN_WRITE_BUFFER Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 32bit, in/out
     *  Buffer id, 0 means host have to allocate one
     */
    HGCMFunctionParameter   iBufferID;

    /** 32bit, in
     *  Buffer size
     */
    HGCMFunctionParameter   cbBufferSize;

    /** 32bit, in
     *  Write offset in buffer
     */
    HGCMFunctionParameter   ui32Offset;

    /** pointer, in
     *  Data buffer
     */
    HGCMFunctionParameter   pBuffer;

} CRVBOXHGCMWRITEBUFFER;

/** GUEST_FN_WRITE_READ_BUFFERED Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 32bit, in
     *  Buffer id.
     */
    HGCMFunctionParameter   iBufferID;

    /** pointer, out
     *  Writeback buffer
     */
    HGCMFunctionParameter   pWriteback;

    /** 32bit, out
     * Count of bytes written to writeback buffer
     */
    HGCMFunctionParameter   cbWriteback;

} CRVBOXHGCMWRITEREADBUFFERED;


typedef struct
{
    VBVAINFOSCREEN Screen;
    void *pvVRAM;
} CRVBOXHGCMDEVRESIZE;

typedef struct
{
    uint32_t u32Screen;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} CRVBOXHGCMVIEWPORT;

typedef DECLCALLBACKPTR(void, PFNCRSCREENSHOTREPORT)(void *pvCtx, uint32_t uScreen,
                uint32_t x, uint32_t y, uint32_t uBitsPerPixel,
                uint32_t uBytesPerLine, uint32_t uGuestWidth, uint32_t uGuestHeight,
                uint8_t *pu8BufferAddress, uint64_t u64TimeStamp);

typedef DECLCALLBACKPTR(bool, PFNCRSCREENSHOTBEGIN)(void *pvCtx, uint32_t uScreen, uint64_t u64TimeStamp);
typedef DECLCALLBACKPTR(void, PFNCRSCREENSHOTEND)(void *pvCtx, uint32_t uScreen, uint64_t u64TimeStamp);

#define CRSCREEN_ALL (0xffffffff)

typedef struct
{
    /* screen id or CRSCREEN_ALL to specify all enabled */
    uint32_t u32Screen;
    uint32_t u32Width;
    uint32_t u32Height;
    uint32_t u32Pitch;
    void *pvBuffer;
    void *pvContext;
    PFNCRSCREENSHOTBEGIN pfnScreenshotBegin;
    PFNCRSCREENSHOTREPORT pfnScreenshotPerform;
    PFNCRSCREENSHOTEND pfnScreenshotEnd;
} CRVBOXHGCMTAKESCREENSHOT;

typedef struct
{
    uint32_t u32Screen;
    uint32_t u32ScaleFactorWMultiplied;
    uint32_t u32ScaleFactorHMultiplied;
} CRVBOXHGCMSETSCALEFACTOR;

typedef struct
{
    bool     fUnscaledHiDPI;
} CRVBOXHGCMSETUNSCALEDHIDPIOUTPUT;

#endif
