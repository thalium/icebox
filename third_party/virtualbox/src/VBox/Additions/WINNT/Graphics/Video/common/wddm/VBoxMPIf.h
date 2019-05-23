/* $Id: VBoxMPIf.h $ */
/** @file
 * VBox WDDM Miniport driver.
 *
 * Contains base definitions of constants & structures used to control & perform
 * rendering, such as DMA commands types, allocation types, escape codes, etc.
 * used by both miniport & display drivers.
 *
 * The latter uses these and only these defs to communicate with the former
 * by posting appropriate requests via D3D RT Krnl Svc accessing callbacks.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxMPIf_h___
#define ___VBoxMPIf_h___

#include <VBoxVideo.h>
#include "../../../../include/VBoxDisplay.h"
#include "../VBoxVideoTools.h"
#include <VBoxUhgsmi.h>
#include <VBox/VBoxGuestCoreTypes.h> /* for VBGLIOCHGCMCALL */

/* One would increase this whenever definitions in this file are changed */
#define VBOXVIDEOIF_VERSION 20

#define VBOXWDDM_NODE_ID_SYSTEM             0
#define VBOXWDDM_NODE_ID_3D                 (VBOXWDDM_NODE_ID_SYSTEM)
#define VBOXWDDM_NODE_ID_3D_KMT             (VBOXWDDM_NODE_ID_3D)
#define VBOXWDDM_NODE_ID_2D_VIDEO           (VBOXWDDM_NODE_ID_3D_KMT + 1)
#define VBOXWDDM_NUM_NODES                  (VBOXWDDM_NODE_ID_2D_VIDEO + 1)

#define VBOXWDDM_ENGINE_ID_SYSTEM           0
#if (VBOXWDDM_NODE_ID_3D == VBOXWDDM_NODE_ID_SYSTEM)
# define VBOXWDDM_ENGINE_ID_3D              (VBOXWDDM_ENGINE_ID_SYSTEM + 1)
#else
# define VBOXWDDM_ENGINE_ID_3D              0
#endif
#if (VBOXWDDM_NODE_ID_3D_KMT == VBOXWDDM_NODE_ID_3D)
# define VBOXWDDM_ENGINE_ID_3D_KMT          VBOXWDDM_ENGINE_ID_3D
#else
# define VBOXWDDM_ENGINE_ID_3D_KMT          0
#endif
#if (VBOXWDDM_NODE_ID_2D_VIDEO == VBOXWDDM_NODE_ID_3D)
# define VBOXWDDM_ENGINE_ID_2D_VIDEO        VBOXWDDM_ENGINE_ID_3D
#else
# define VBOXWDDM_ENGINE_ID_2D_VIDEO        0
#endif


/* create allocation func */
typedef enum
{
    VBOXWDDM_ALLOC_TYPE_UNEFINED = 0,
    VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE,
    VBOXWDDM_ALLOC_TYPE_STD_SHADOWSURFACE,
    VBOXWDDM_ALLOC_TYPE_STD_STAGINGSURFACE,
    /* this one is win 7-specific and hence unused for now */
    VBOXWDDM_ALLOC_TYPE_STD_GDISURFACE
    /* custom allocation types requested from user-mode d3d module will go here */
    , VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC
    , VBOXWDDM_ALLOC_TYPE_UMD_HGSMI_BUFFER
} VBOXWDDM_ALLOC_TYPE;

/* usage */
typedef enum
{
    VBOXWDDM_ALLOCUSAGE_TYPE_UNEFINED = 0,
    /* set for the allocation being primary */
    VBOXWDDM_ALLOCUSAGE_TYPE_PRIMARY,
} VBOXWDDM_ALLOCUSAGE_TYPE;

typedef struct VBOXWDDM_SURFACE_DESC
{
    UINT width;
    UINT height;
    D3DDDIFORMAT format;
    UINT bpp;
    UINT pitch;
    UINT depth;
    UINT slicePitch;
    UINT d3dWidth;
    UINT cbSize;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DDDI_RATIONAL RefreshRate;
} VBOXWDDM_SURFACE_DESC, *PVBOXWDDM_SURFACE_DESC;

typedef struct VBOXWDDM_ALLOCINFO
{
    VBOXWDDM_ALLOC_TYPE enmType;
    union
    {
        struct
        {
            D3DDDI_RESOURCEFLAGS fFlags;
            /* id used to identify the allocation on the host */
            uint32_t hostID;
            uint64_t hSharedHandle;
            VBOXWDDM_SURFACE_DESC SurfDesc;
        };

        struct
        {
            uint32_t cbBuffer;
            VBOXUHGSMI_BUFFER_TYPE_FLAGS fUhgsmiType;
        };
    };
} VBOXWDDM_ALLOCINFO, *PVBOXWDDM_ALLOCINFO;

typedef struct VBOXWDDM_RC_DESC
{
    D3DDDI_RESOURCEFLAGS fFlags;
    D3DDDIFORMAT enmFormat;
    D3DDDI_POOL enmPool;
    D3DDDIMULTISAMPLE_TYPE enmMultisampleType;
    UINT MultisampleQuality;
    UINT MipLevels;
    UINT Fvf;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DDDI_RATIONAL RefreshRate;
    D3DDDI_ROTATION enmRotation;
} VBOXWDDM_RC_DESC, *PVBOXWDDM_RC_DESC;

typedef struct VBOXWDDMDISP_RESOURCE_FLAGS
{
    union
    {
        struct
        {
            UINT Opened     : 1; /* this resource is OpenResource'd rather than CreateResource'd */
            UINT Generic    : 1; /* identifies this is a resource created with CreateResource, the VBOXWDDMDISP_RESOURCE::fRcFlags is valid */
            UINT KmResource : 1; /* this resource has underlying km resource */
            UINT Reserved   : 29; /* reserved */
        };
        UINT        Value;
    };
} VBOXWDDMDISP_RESOURCE_FLAGS, *PVBOXWDDMDISP_RESOURCE_FLAGS;

typedef struct VBOXWDDM_RCINFO
{
    VBOXWDDMDISP_RESOURCE_FLAGS fFlags;
    VBOXWDDM_RC_DESC RcDesc;
    uint32_t cAllocInfos;
//    VBOXWDDM_ALLOCINFO aAllocInfos[1];
} VBOXWDDM_RCINFO, *PVBOXWDDM_RCINFO;

typedef struct VBOXWDDM_DMA_PRIVATEDATA_FLAFS
{
    union
    {
        struct
        {
            UINT bCmdInDmaBuffer : 1;
            UINT bReserved : 31;
        };
        uint32_t Value;
    };
} VBOXWDDM_DMA_PRIVATEDATA_FLAFS, *PVBOXWDDM_DMA_PRIVATEDATA_FLAFS;

typedef struct VBOXWDDM_DMA_PRIVATEDATA_BASEHDR
{
    VBOXVDMACMD_TYPE enmCmd;
    union
    {
        VBOXWDDM_DMA_PRIVATEDATA_FLAFS fFlags;
        uint32_t u32CmdReserved;
    };
} VBOXWDDM_DMA_PRIVATEDATA_BASEHDR, *PVBOXWDDM_DMA_PRIVATEDATA_BASEHDR;

typedef struct VBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO
{
    uint32_t offData;
    uint32_t cbData;
} VBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO, *PVBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO;

typedef struct VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD
{
    VBOXWDDM_DMA_PRIVATEDATA_BASEHDR Base;
    VBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO aBufInfos[1];
} VBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD, *PVBOXWDDM_DMA_PRIVATEDATA_UM_CHROMIUM_CMD;


#define VBOXVHWA_F_ENABLED  0x00000001
#define VBOXVHWA_F_CKEY_DST 0x00000002
#define VBOXVHWA_F_CKEY_SRC 0x00000004

#define VBOXVHWA_MAX_FORMATS 8

typedef struct VBOXVHWA_INFO
{
    uint32_t fFlags;
    uint32_t cOverlaysSupported;
    uint32_t cFormats;
    D3DDDIFORMAT aFormats[VBOXVHWA_MAX_FORMATS];
} VBOXVHWA_INFO;

#define VBOXWDDM_OVERLAY_F_CKEY_DST      0x00000001
#define VBOXWDDM_OVERLAY_F_CKEY_DSTRANGE 0x00000002
#define VBOXWDDM_OVERLAY_F_CKEY_SRC      0x00000004
#define VBOXWDDM_OVERLAY_F_CKEY_SRCRANGE 0x00000008
#define VBOXWDDM_OVERLAY_F_BOB           0x00000010
#define VBOXWDDM_OVERLAY_F_INTERLEAVED   0x00000020
#define VBOXWDDM_OVERLAY_F_MIRROR_LR     0x00000040
#define VBOXWDDM_OVERLAY_F_MIRROR_UD     0x00000080
#define VBOXWDDM_OVERLAY_F_DEINTERLACED  0x00000100

typedef struct VBOXWDDM_OVERLAY_DESC
{
    uint32_t fFlags;
    UINT DstColorKeyLow;
    UINT DstColorKeyHigh;
    UINT SrcColorKeyLow;
    UINT SrcColorKeyHigh;
} VBOXWDDM_OVERLAY_DESC, *PVBOXWDDM_OVERLAY_DESC;

typedef struct VBOXWDDM_OVERLAY_INFO
{
    VBOXWDDM_OVERLAY_DESC OverlayDesc;
    VBOXWDDM_DIRTYREGION DirtyRegion; /* <- the dirty region of the overlay surface */
} VBOXWDDM_OVERLAY_INFO, *PVBOXWDDM_OVERLAY_INFO;

typedef struct VBOXWDDM_OVERLAYFLIP_INFO
{
    VBOXWDDM_DIRTYREGION DirtyRegion; /* <- the dirty region of the overlay surface */
} VBOXWDDM_OVERLAYFLIP_INFO, *PVBOXWDDM_OVERLAYFLIP_INFO;


typedef enum
{
    VBOXWDDM_CONTEXT_TYPE_UNDEFINED = 0,
    /* system-created context (for GDI rendering) */
    VBOXWDDM_CONTEXT_TYPE_SYSTEM,
    /* context created by the D3D User-mode driver when crogl IS available */
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D,
    /* context created by the D3D User-mode driver when crogl is NOT available or for ddraw overlay acceleration */
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D,
    /* contexts created by the cromium HGSMI transport for HGSMI commands submission */
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_3D,
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_UHGSMI_GL,
    /* context created by the kernel->user communication mechanism for visible rects reporting, etc.  */
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_SESSION,
    /* context created by VBoxTray to handle resize operations */
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_RESIZE,
    /* context created by VBoxTray to handle seamless operations */
    VBOXWDDM_CONTEXT_TYPE_CUSTOM_DISPIF_SEAMLESS
} VBOXWDDM_CONTEXT_TYPE;

typedef struct VBOXWDDM_CREATECONTEXT_INFO
{
    /* interface version, i.e. 9 for d3d9, 8 for d3d8, etc. */
    uint32_t u32IfVersion;
    /* true if d3d false if ddraw */
    VBOXWDDM_CONTEXT_TYPE enmType;
    uint32_t crVersionMajor;
    uint32_t crVersionMinor;
    /* we use uint64_t instead of HANDLE to ensure structure def is the same for both 32-bit and 64-bit
     * since x64 kernel driver can be called by 32-bit UMD */
    uint64_t hUmEvent;
    /* info to be passed to UMD notification to identify the context */
    uint64_t u64UmInfo;
} VBOXWDDM_CREATECONTEXT_INFO, *PVBOXWDDM_CREATECONTEXT_INFO;

typedef uint64_t VBOXDISP_UMHANDLE;
typedef uint32_t VBOXDISP_KMHANDLE;

typedef struct VBOXWDDM_RECTS_FLAFS
{
    union
    {
        struct
        {
            /* used only in conjunction with bSetVisibleRects.
             * if set - VBOXWDDM_RECTS_INFO::aRects[0] contains view rectangle */
            UINT bSetViewRect : 1;
            /* adds visible regions */
            UINT bAddVisibleRects : 1;
            /* adds hidden regions */
            UINT bAddHiddenRects : 1;
            /* hide entire window */
            UINT bHide : 1;
            /* reserved */
            UINT Reserved : 28;
        };
        uint32_t Value;
    };
} VBOXWDDM_RECTS_FLAFS, *PVBOXWDDM_RECTS_FLAFS;

typedef struct VBOXWDDM_RECTS_INFO
{
    uint32_t cRects;
    RECT aRects[1];
} VBOXWDDM_RECTS_INFO, *PVBOXWDDM_RECTS_INFO;

#define VBOXWDDM_RECTS_INFO_SIZE4CRECTS(_cRects) (RT_OFFSETOF(VBOXWDDM_RECTS_INFO, aRects[(_cRects)]))
#define VBOXWDDM_RECTS_INFO_SIZE(_pRects) (VBOXVIDEOCM_CMD_RECTS_SIZE4CRECTS((_pRects)->cRects))

typedef enum
{
    /* command to be post to user mode */
    VBOXVIDEOCM_CMD_TYPE_UM = 0,
    /* control command processed in kernel mode */
    VBOXVIDEOCM_CMD_TYPE_CTL_KM,
    VBOXVIDEOCM_CMD_DUMMY_32BIT = 0x7fffffff
} VBOXVIDEOCM_CMD_TYPE;

typedef struct VBOXVIDEOCM_CMD_HDR
{
    uint64_t u64UmData;
    uint32_t cbCmd;
    VBOXVIDEOCM_CMD_TYPE enmType;
}VBOXVIDEOCM_CMD_HDR, *PVBOXVIDEOCM_CMD_HDR;

AssertCompile((sizeof (VBOXVIDEOCM_CMD_HDR) & 7) == 0);

typedef struct VBOXVIDEOCM_CMD_RECTS
{
    VBOXWDDM_RECTS_FLAFS fFlags;
    VBOXWDDM_RECTS_INFO RectsInfo;
} VBOXVIDEOCM_CMD_RECTS, *PVBOXVIDEOCM_CMD_RECTS;

typedef struct VBOXVIDEOCM_CMD_RECTS_INTERNAL
{
    union
    {
        VBOXDISP_UMHANDLE hSwapchainUm;
        uint64_t hWnd;
        uint64_t u64Value;
    };
    VBOXVIDEOCM_CMD_RECTS Cmd;
} VBOXVIDEOCM_CMD_RECTS_INTERNAL, *PVBOXVIDEOCM_CMD_RECTS_INTERNAL;

typedef struct VBOXVIDEOCM_CMD_RECTS_HDR
{
    VBOXVIDEOCM_CMD_HDR Hdr;
    VBOXVIDEOCM_CMD_RECTS_INTERNAL Data;
} VBOXVIDEOCM_CMD_RECTS_HDR, *PVBOXVIDEOCM_CMD_RECTS_HDR;

#define VBOXVIDEOCM_CMD_RECTS_INTERNAL_SIZE4CRECTS(_cRects) (RT_OFFSETOF(VBOXVIDEOCM_CMD_RECTS_INTERNAL, Cmd.RectsInfo.aRects[(_cRects)]))
#define VBOXVIDEOCM_CMD_RECTS_INTERNAL_SIZE(_pCmd) (VBOXVIDEOCM_CMD_RECTS_INTERNAL_SIZE4CRECTS((_pCmd)->cRects))

typedef struct VBOXWDDM_GETVBOXVIDEOCMCMD_HDR
{
    uint32_t cbCmdsReturned;
    uint32_t cbRemainingCmds;
    uint32_t cbRemainingFirstCmd;
    uint32_t u32Reserved;
} VBOXWDDM_GETVBOXVIDEOCMCMD_HDR, *PVBOXWDDM_GETVBOXVIDEOCMCMD_HDR;

typedef struct VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXWDDM_GETVBOXVIDEOCMCMD_HDR Hdr;
} VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD, *PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD;

AssertCompile((sizeof (VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD) & 7) == 0);
AssertCompile(RT_OFFSETOF(VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD, EscapeHdr) == 0);

typedef struct VBOXDISPIFESCAPE_DBGPRINT
{
    VBOXDISPIFESCAPE EscapeHdr;
    /* null-terminated string to DbgPrint including \0 */
    char aStringBuf[1];
} VBOXDISPIFESCAPE_DBGPRINT, *PVBOXDISPIFESCAPE_DBGPRINT;
AssertCompile(RT_OFFSETOF(VBOXDISPIFESCAPE_DBGPRINT, EscapeHdr) == 0);

typedef enum
{
    VBOXDISPIFESCAPE_DBGDUMPBUF_TYPE_UNDEFINED = 0,
    VBOXDISPIFESCAPE_DBGDUMPBUF_TYPE_D3DCAPS9 = 1,
    VBOXDISPIFESCAPE_DBGDUMPBUF_TYPE_DUMMY32BIT = 0x7fffffff
} VBOXDISPIFESCAPE_DBGDUMPBUF_TYPE;

typedef struct VBOXDISPIFESCAPE_DBGDUMPBUF_FLAGS
{
    union
    {
        struct
        {
            UINT WoW64      : 1;
            UINT Reserved   : 31; /* reserved */
        };
        UINT  Value;
    };
} VBOXDISPIFESCAPE_DBGDUMPBUF_FLAGS, *PVBOXDISPIFESCAPE_DBGDUMPBUF_FLAGS;

typedef struct VBOXDISPIFESCAPE_DBGDUMPBUF
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXDISPIFESCAPE_DBGDUMPBUF_TYPE enmType;
    VBOXDISPIFESCAPE_DBGDUMPBUF_FLAGS Flags;
    char aBuf[1];
} VBOXDISPIFESCAPE_DBGDUMPBUF, *PVBOXDISPIFESCAPE_DBGDUMPBUF;
AssertCompile(RT_OFFSETOF(VBOXDISPIFESCAPE_DBGDUMPBUF, EscapeHdr) == 0);

typedef struct VBOXSCREENLAYOUT_ELEMENT
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    POINT pos;
} VBOXSCREENLAYOUT_ELEMENT, *PVBOXSCREENLAYOUT_ELEMENT;

typedef struct VBOXSCREENLAYOUT
{
    uint32_t cScreens;
    VBOXSCREENLAYOUT_ELEMENT aScreens[1];
} VBOXSCREENLAYOUT, *PVBOXSCREENLAYOUT;

typedef struct VBOXDISPIFESCAPE_SCREENLAYOUT
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXSCREENLAYOUT ScreenLayout;
} VBOXDISPIFESCAPE_SCREENLAYOUT, *PVBOXDISPIFESCAPE_SCREENLAYOUT;

typedef struct VBOXSWAPCHAININFO
{
    VBOXDISP_KMHANDLE hSwapchainKm; /* in, NULL if new is being created */
    VBOXDISP_UMHANDLE hSwapchainUm; /* in, UMD private data */
    int32_t winHostID;
    RECT Rect;
    UINT u32Reserved;
    UINT cAllocs;
    D3DKMT_HANDLE ahAllocs[1];
}VBOXSWAPCHAININFO, *PVBOXSWAPCHAININFO;
typedef struct VBOXDISPIFESCAPE_SWAPCHAININFO
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXSWAPCHAININFO SwapchainInfo;
} VBOXDISPIFESCAPE_SWAPCHAININFO, *PVBOXDISPIFESCAPE_SWAPCHAININFO;

typedef struct VBOXVIDEOCM_UM_ALLOC
{
    VBOXDISP_KMHANDLE hAlloc;
    uint32_t cbData;
    uint64_t pvData;
    uint64_t hSynch;
    VBOXUHGSMI_BUFFER_TYPE_FLAGS fUhgsmiType;
} VBOXVIDEOCM_UM_ALLOC, *PVBOXVIDEOCM_UM_ALLOC;

typedef struct VBOXDISPIFESCAPE_UHGSMI_ALLOCATE
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXVIDEOCM_UM_ALLOC Alloc;
} VBOXDISPIFESCAPE_UHGSMI_ALLOCATE, *PVBOXDISPIFESCAPE_UHGSMI_ALLOCATE;

typedef struct VBOXDISPIFESCAPE_UHGSMI_DEALLOCATE
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXDISP_KMHANDLE hAlloc;
} VBOXDISPIFESCAPE_UHGSMI_DEALLOCATE, *PVBOXDISPIFESCAPE_UHGSMI_DEALLOCATE;

typedef struct VBOXWDDM_UHGSMI_BUFFER_UI_INFO_ESCAPE
{
    VBOXDISP_KMHANDLE hAlloc;
    VBOXWDDM_UHGSMI_BUFFER_UI_SUBMIT_INFO Info;
} VBOXWDDM_UHGSMI_BUFFER_UI_INFO_ESCAPE, *PVBOXWDDM_UHGSMI_BUFFER_UI_INFO_ESCAPE;

typedef struct VBOXDISPIFESCAPE_UHGSMI_SUBMIT
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBOXWDDM_UHGSMI_BUFFER_UI_INFO_ESCAPE aBuffers[1];
} VBOXDISPIFESCAPE_UHGSMI_SUBMIT, *PVBOXDISPIFESCAPE_UHGSMI_SUBMIT;

typedef struct VBOXDISPIFESCAPE_SHRC_REF
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint64_t hAlloc;
} VBOXDISPIFESCAPE_SHRC_REF, *PVBOXDISPIFESCAPE_SHRC_REF;

typedef struct VBOXDISPIFESCAPE_SETALLOCHOSTID
{
    VBOXDISPIFESCAPE EscapeHdr;
    int32_t rc;
    uint32_t hostID;
    uint64_t hAlloc;

} VBOXDISPIFESCAPE_SETALLOCHOSTID, *PVBOXDISPIFESCAPE_SETALLOCHOSTID;

typedef struct VBOXDISPIFESCAPE_CRHGSMICTLCON_CALL
{
    VBOXDISPIFESCAPE EscapeHdr;
    VBGLIOCHGCMCALL CallInfo;
} VBOXDISPIFESCAPE_CRHGSMICTLCON_CALL, *PVBOXDISPIFESCAPE_CRHGSMICTLCON_CALL;

/* query info func */
typedef struct VBOXWDDM_QI
{
    uint32_t u32Version;
    uint32_t u32VBox3DCaps;
    uint32_t cInfos;
    VBOXVHWA_INFO aInfos[VBOX_VIDEO_MAX_SCREENS];
} VBOXWDDM_QI;

/** Convert a given FourCC code to a D3DDDIFORMAT enum. */
#define VBOXWDDM_D3DDDIFORMAT_FROM_FOURCC(_a, _b, _c, _d) \
    ((D3DDDIFORMAT)MAKEFOURCC(_a, _b, _c, _d))

/* submit cmd func */
DECLINLINE(D3DDDIFORMAT) vboxWddmFmtNoAlphaFormat(D3DDDIFORMAT enmFormat)
{
    switch (enmFormat)
    {
        case D3DDDIFMT_A8R8G8B8:
            return D3DDDIFMT_X8R8G8B8;
        case D3DDDIFMT_A1R5G5B5:
            return D3DDDIFMT_X1R5G5B5;
        case D3DDDIFMT_A4R4G4B4:
            return D3DDDIFMT_X4R4G4B4;
        case D3DDDIFMT_A8B8G8R8:
            return D3DDDIFMT_X8B8G8R8;
        default:
            return enmFormat;
    }
}

/* tooling */
DECLINLINE(UINT) vboxWddmCalcBitsPerPixel(D3DDDIFORMAT enmFormat)
{
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4063) /* VBOXWDDM_D3DDDIFORMAT_FROM_FOURCC('Y', 'V', '1', '2'): isn't part of the enum */
#endif
    switch (enmFormat)
    {
        case D3DDDIFMT_R8G8B8:
            return 24;
        case D3DDDIFMT_A8R8G8B8:
        case D3DDDIFMT_X8R8G8B8:
            return 32;
        case D3DDDIFMT_R5G6B5:
        case D3DDDIFMT_X1R5G5B5:
        case D3DDDIFMT_A1R5G5B5:
        case D3DDDIFMT_A4R4G4B4:
            return 16;
        case D3DDDIFMT_R3G3B2:
        case D3DDDIFMT_A8:
            return 8;
        case D3DDDIFMT_A8R3G3B2:
        case D3DDDIFMT_X4R4G4B4:
            return 16;
        case D3DDDIFMT_A2B10G10R10:
        case D3DDDIFMT_A8B8G8R8:
        case D3DDDIFMT_X8B8G8R8:
        case D3DDDIFMT_G16R16:
        case D3DDDIFMT_A2R10G10B10:
            return 32;
        case D3DDDIFMT_A16B16G16R16:
        case D3DDDIFMT_A16B16G16R16F:
            return 64;
        case D3DDDIFMT_A32B32G32R32F:
            return 128;
        case D3DDDIFMT_A8P8:
            return 16;
        case D3DDDIFMT_P8:
        case D3DDDIFMT_L8:
            return 8;
        case D3DDDIFMT_L16:
        case D3DDDIFMT_A8L8:
            return 16;
        case D3DDDIFMT_A4L4:
            return 8;
        case D3DDDIFMT_V8U8:
        case D3DDDIFMT_L6V5U5:
            return 16;
        case D3DDDIFMT_X8L8V8U8:
        case D3DDDIFMT_Q8W8V8U8:
        case D3DDDIFMT_V16U16:
        case D3DDDIFMT_W11V11U10:
        case D3DDDIFMT_A2W10V10U10:
            return 32;
        case D3DDDIFMT_D16_LOCKABLE:
        case D3DDDIFMT_D16:
        case D3DDDIFMT_D15S1:
            return 16;
        case D3DDDIFMT_D32:
        case D3DDDIFMT_D24S8:
        case D3DDDIFMT_D24X8:
        case D3DDDIFMT_D24X4S4:
        case D3DDDIFMT_D24FS8:
        case D3DDDIFMT_D32_LOCKABLE:
        case D3DDDIFMT_D32F_LOCKABLE:
            return 32;
        case D3DDDIFMT_S8_LOCKABLE:
            return 8;
        case D3DDDIFMT_DXT1:
            return 4;
        case D3DDDIFMT_DXT2:
        case D3DDDIFMT_DXT3:
        case D3DDDIFMT_DXT4:
        case D3DDDIFMT_DXT5:
        case D3DDDIFMT_VERTEXDATA:
        case D3DDDIFMT_INDEX16: /* <- yes, dx runtime treats it as such */
            return 8;
        case D3DDDIFMT_INDEX32:
            return 8;
        case D3DDDIFMT_R32F:
            return 32;
        case D3DDDIFMT_R16F:
            return 16;
        case D3DDDIFMT_YUY2: /* 4 bytes per 2 pixels. */
        case VBOXWDDM_D3DDDIFORMAT_FROM_FOURCC('Y', 'V', '1', '2'):
            return 16;
        default:
            AssertBreakpoint();
            return 0;
    }
#ifdef _MSC_VER
# pragma warning(pop)
#endif
}

DECLINLINE(uint32_t) vboxWddmFormatToFourcc(D3DDDIFORMAT enmFormat)
{
    uint32_t uFormat = (uint32_t)enmFormat;
    /* assume that in case both four bytes are non-zero, this is a fourcc */
    if ((uFormat & 0xff000000)
            && (uFormat & 0x00ff0000)
            && (uFormat & 0x0000ff00)
            && (uFormat & 0x000000ff)
            )
        return uFormat;
    return 0;
}

#define VBOXWDDM_ROUNDBOUND(_v, _b) (((_v) + ((_b) - 1)) & ~((_b) - 1))

DECLINLINE(UINT) vboxWddmCalcOffXru(UINT w, D3DDDIFORMAT enmFormat)
{
    switch (enmFormat)
    {
        /* pitch for the DXT* (aka compressed) formats is the size in bytes of blocks that fill in an image width
         * i.e. each block decompressed into 4 x 4 pixels, so we have ((Width + 3) / 4) blocks for Width.
         * then each block has 64 bits (8 bytes) for DXT1 and 64+64 bits (16 bytes) for DXT2-DXT5, so.. : */
        case D3DDDIFMT_DXT1:
        {
            UINT Pitch = (w + 3) / 4; /* <- pitch size in blocks */
            Pitch *= 8;               /* <- pitch size in bytes */
            return Pitch;
        }
        case D3DDDIFMT_DXT2:
        case D3DDDIFMT_DXT3:
        case D3DDDIFMT_DXT4:
        case D3DDDIFMT_DXT5:
        {
            UINT Pitch = (w + 3) / 4; /* <- pitch size in blocks */
            Pitch *= 8;               /* <- pitch size in bytes */
            return Pitch;
        }
        default:
        {
            /* the default is just to calculate the pitch from bpp */
            UINT bpp = vboxWddmCalcBitsPerPixel(enmFormat);
            UINT Pitch = bpp * w;
            /* pitch is now in bits, translate in bytes */
            return VBOXWDDM_ROUNDBOUND(Pitch, 8) >> 3;
        }
    }
}

DECLINLINE(UINT) vboxWddmCalcOffXrd(UINT w, D3DDDIFORMAT enmFormat)
{
    switch (enmFormat)
    {
        /* pitch for the DXT* (aka compressed) formats is the size in bytes of blocks that fill in an image width
         * i.e. each block decompressed into 4 x 4 pixels, so we have ((Width + 3) / 4) blocks for Width.
         * then each block has 64 bits (8 bytes) for DXT1 and 64+64 bits (16 bytes) for DXT2-DXT5, so.. : */
        case D3DDDIFMT_DXT1:
        {
            UINT Pitch = w / 4; /* <- pitch size in blocks */
            Pitch *= 8;         /* <- pitch size in bytes */
            return Pitch;
        }
        case D3DDDIFMT_DXT2:
        case D3DDDIFMT_DXT3:
        case D3DDDIFMT_DXT4:
        case D3DDDIFMT_DXT5:
        {
            UINT Pitch = w / 4; /* <- pitch size in blocks */
            Pitch *= 16;               /* <- pitch size in bytes */
            return Pitch;
        }
        default:
        {
            /* the default is just to calculate the pitch from bpp */
            UINT bpp = vboxWddmCalcBitsPerPixel(enmFormat);
            UINT Pitch = bpp * w;
            /* pitch is now in bits, translate in bytes */
            return Pitch >> 3;
        }
    }
}

DECLINLINE(UINT) vboxWddmCalcHightPacking(D3DDDIFORMAT enmFormat)
{
    switch (enmFormat)
    {
        /* for the DXT* (aka compressed) formats each block is decompressed into 4 x 4 pixels,
         * so packing is 4
         */
        case D3DDDIFMT_DXT1:
        case D3DDDIFMT_DXT2:
        case D3DDDIFMT_DXT3:
        case D3DDDIFMT_DXT4:
        case D3DDDIFMT_DXT5:
            return 4;
        default:
            return 1;
    }
}

DECLINLINE(UINT) vboxWddmCalcOffYru(UINT height, D3DDDIFORMAT enmFormat)
{
    UINT packing = vboxWddmCalcHightPacking(enmFormat);
    /* round it up */
    return (height + packing - 1) / packing;
}

DECLINLINE(UINT) vboxWddmCalcOffYrd(UINT height, D3DDDIFORMAT enmFormat)
{
    UINT packing = vboxWddmCalcHightPacking(enmFormat);
    /* round it up */
    return height / packing;
}

DECLINLINE(UINT) vboxWddmCalcPitch(UINT w, D3DDDIFORMAT enmFormat)
{
    return vboxWddmCalcOffXru(w, enmFormat);
}

DECLINLINE(UINT) vboxWddmCalcWidthForPitch(UINT Pitch, D3DDDIFORMAT enmFormat)
{
    switch (enmFormat)
    {
        /* pitch for the DXT* (aka compressed) formats is the size in bytes of blocks that fill in an image width
         * i.e. each block decompressed into 4 x 4 pixels, so we have ((Width + 3) / 4) blocks for Width.
         * then each block has 64 bits (8 bytes) for DXT1 and 64+64 bits (16 bytes) for DXT2-DXT5, so.. : */
        case D3DDDIFMT_DXT1:
        {
            return (Pitch / 8) * 4;
        }
        case D3DDDIFMT_DXT2:
        case D3DDDIFMT_DXT3:
        case D3DDDIFMT_DXT4:
        case D3DDDIFMT_DXT5:
        {
            return (Pitch / 16) * 4;;
        }
        default:
        {
            /* the default is just to calculate it from bpp */
            UINT bpp = vboxWddmCalcBitsPerPixel(enmFormat);
            return (Pitch << 3) / bpp;
        }
    }
}

DECLINLINE(UINT) vboxWddmCalcNumRows(UINT top, UINT bottom, D3DDDIFORMAT enmFormat)
{
    Assert(bottom > top);
    top = top ? vboxWddmCalcOffYrd(top, enmFormat) : 0; /* <- just to optimize it a bit */
    bottom = vboxWddmCalcOffYru(bottom, enmFormat);
    return bottom - top;
}

DECLINLINE(UINT) vboxWddmCalcRowSize(UINT left, UINT right, D3DDDIFORMAT enmFormat)
{
    Assert(right > left);
    left = left ? vboxWddmCalcOffXrd(left, enmFormat) : 0; /* <- just to optimize it a bit */
    right = vboxWddmCalcOffXru(right, enmFormat);
    return right - left;
}

DECLINLINE(UINT) vboxWddmCalcSize(UINT pitch, UINT height, D3DDDIFORMAT enmFormat)
{
    UINT cRows = vboxWddmCalcNumRows(0, height, enmFormat);
    return pitch * cRows;
}

DECLINLINE(UINT) vboxWddmCalcOffXYrd(UINT x, UINT y, UINT pitch, D3DDDIFORMAT enmFormat)
{
    UINT offY = 0;
    if (y)
        offY = vboxWddmCalcSize(pitch, y, enmFormat);

    return offY + vboxWddmCalcOffXrd(x, enmFormat);
}

#define VBOXWDDM_ARRAY_MAXELEMENTSU32(_t) ((uint32_t)((UINT32_MAX) / sizeof (_t)))
#define VBOXWDDM_TRAILARRAY_MAXELEMENTSU32(_t, _af) ((uint32_t)(((~(0UL)) - (uint32_t)RT_OFFSETOF(_t, _af[0])) / RT_SIZEOFMEMB(_t, _af[0])))

#endif /* #ifndef ___VBoxMPIf_h___ */
