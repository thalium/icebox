/* $Id: DevVGA-SVGA.cpp $ */
/** @file
 * VMware SVGA device.
 *
 * Logging levels guidelines for this and related files:
 *  - Log() for normal bits.
 *  - LogFlow() for more info.
 *  - Log2 for hex dump of cursor data.
 *  - Log3 for hex dump of shader code.
 *  - Log4 for hex dumps of 3D data.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/** @page pg_dev_vmsvga     VMSVGA - VMware SVGA II Device Emulation
 *
 * This device emulation was contributed by trivirt AG.  It offers an
 * alternative to our Bochs based VGA graphics and 3d emulations.  This is
 * valuable for Xorg based guests, as there is driver support shipping with Xorg
 * since it forked from XFree86.
 *
 *
 * @section sec_dev_vmsvga_sdk  The VMware SDK
 *
 * This is officially deprecated now, however it's still quite useful,
 * especially for getting the old features working:
 * http://vmware-svga.sourceforge.net/
 *
 * They currently point developers at the following resources.
 *  - http://cgit.freedesktop.org/xorg/driver/xf86-video-vmware/
 *  - http://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/svga/
 *  - http://cgit.freedesktop.org/mesa/vmwgfx/
 *
 * @subsection subsec_dev_vmsvga_sdk_results  Test results
 *
 * Test results:
 *  - 2dmark.img:
 *       + todo
 *  - backdoor-tclo.img:
 *       + todo
 *  - blit-cube.img:
 *       + todo
 *  - bunnies.img:
 *       + todo
 *  - cube.img:
 *       + todo
 *  - cubemark.img:
 *       + todo
 *  - dynamic-vertex-stress.img:
 *       + todo
 *  - dynamic-vertex.img:
 *       + todo
 *  - fence-stress.img:
 *       + todo
 *  - gmr-test.img:
 *       + todo
 *  - half-float-test.img:
 *       + todo
 *  - noscreen-cursor.img:
 *       - The CURSOR I/O and FIFO registers are not implemented, so the mouse
 *         cursor doesn't show. (Hacking the GUI a little, would make the cursor
 *         visible though.)
 *       - Cursor animation via the palette doesn't work.
 *       - During debugging, it turns out that the framebuffer content seems to
 *         be halfways ignore or something (memset(fb, 0xcc, lots)).
 *       - Trouble with way to small FIFO and the 256x256 cursor fails. Need to
 *         grow it 0x10 fold (128KB -> 2MB like in WS10).
 *  - null.img:
 *       + todo
 *  - pong.img:
 *       + todo
 *  - presentReadback.img:
 *       + todo
 *  - resolution-set.img:
 *       + todo
 *  - rt-gamma-test.img:
 *       + todo
 *  - screen-annotation.img:
 *       + todo
 *  - screen-cursor.img:
 *       + todo
 *  - screen-dma-coalesce.img:
 *       + todo
 *  - screen-gmr-discontig.img:
 *       + todo
 *  - screen-gmr-remap.img:
 *       + todo
 *  - screen-multimon.img:
 *       + todo
 *  - screen-present-clip.img:
 *       + todo
 *  - screen-render-test.img:
 *       + todo
 *  - screen-simple.img:
 *       + todo
 *  - screen-text.img:
 *       + todo
 *  - simple-shaders.img:
 *       + todo
 *  - simple_blit.img:
 *       + todo
 *  - tiny-2d-updates.img:
 *       + todo
 *  - video-formats.img:
 *       + todo
 *  - video-sync.img:
 *       + todo
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#define VMSVGA_USE_EMT_HALT_CODE
#include <VBox/vmm/pdmdev.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/vmm/pgm.h>
#ifdef VMSVGA_USE_EMT_HALT_CODE
# include <VBox/vmm/vmapi.h>
# include <VBox/vmm/vmcpuset.h>
#endif
#include <VBox/sup.h>

#include <iprt/assert.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>
#ifdef IN_RING3
# include <iprt/ctype.h>
# include <iprt/mem.h>
#endif

#include <VBox/AssertGuest.h>
#include <VBox/VMMDev.h>
#include <VBoxVideo.h>
#include <VBox/bioslogo.h>

/* should go BEFORE any other DevVGA include to make all DevVGA.h config defines be visible */
#include "DevVGA.h"

#include "DevVGA-SVGA.h"
#include "vmsvga/svga_reg.h"
#include "vmsvga/svga_escape.h"
#include "vmsvga/svga_overlay.h"
#include "vmsvga/svga3d_reg.h"
#include "vmsvga/svga3d_caps.h"
#ifdef VBOX_WITH_VMSVGA3D
# include "DevVGA-SVGA3d.h"
# ifdef RT_OS_DARWIN
#  include "DevVGA-SVGA3d-cocoa.h"
# endif
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * Macro for checking if a fixed FIFO register is valid according to the
 * current FIFO configuration.
 *
 * @returns true / false.
 * @param   a_iIndex        The fifo register index (like SVGA_FIFO_CAPABILITIES).
 * @param   a_offFifoMin    A valid SVGA_FIFO_MIN value.
 */
#define VMSVGA_IS_VALID_FIFO_REG(a_iIndex, a_offFifoMin) ( ((a_iIndex) + 1) * sizeof(uint32_t) <= (a_offFifoMin) )


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * 64-bit GMR descriptor.
 */
typedef struct
{
   RTGCPHYS GCPhys;
   uint64_t numPages;
} VMSVGAGMRDESCRIPTOR, *PVMSVGAGMRDESCRIPTOR;

/**
 * GMR slot
 */
typedef struct
{
    uint32_t                    cMaxPages;
    uint32_t                    cbTotal;
    uint32_t                    numDescriptors;
    PVMSVGAGMRDESCRIPTOR        paDesc;
} GMR, *PGMR;

#ifdef IN_RING3
/**
 * Internal SVGA ring-3 only state.
 */
typedef struct VMSVGAR3STATE
{
    GMR                    *paGMR; // [VMSVGAState::cGMR]
    struct
    {
        SVGAGuestPtr RT_UNTRUSTED_GUEST         ptr;
        uint32_t RT_UNTRUSTED_GUEST             bytesPerLine;
        SVGAGMRImageFormat RT_UNTRUSTED_GUEST   format;
    } GMRFB;
    struct
    {
        bool                fActive;
        uint32_t            xHotspot;
        uint32_t            yHotspot;
        uint32_t            width;
        uint32_t            height;
        uint32_t            cbData;
        void               *pData;
    } Cursor;
    SVGAColorBGRX           colorAnnotation;

# ifdef VMSVGA_USE_EMT_HALT_CODE
    /** Number of EMTs in BusyDelayedEmts (quicker than scanning the set). */
    uint32_t volatile       cBusyDelayedEmts;
    /** Set of EMTs that are   */
    VMCPUSET                BusyDelayedEmts;
# else
    /** Number of EMTs waiting on hBusyDelayedEmts. */
    uint32_t volatile       cBusyDelayedEmts;
    /** Semaphore that EMTs wait on when reading SVGA_REG_BUSY and the FIFO is
     *  busy (ugly).  */
    RTSEMEVENTMULTI         hBusyDelayedEmts;
# endif
    /** Tracks how much time we waste reading SVGA_REG_BUSY with a busy FIFO. */
    STAMPROFILE             StatBusyDelayEmts;

    STAMPROFILE             StatR3Cmd3dPresentProf;
    STAMPROFILE             StatR3Cmd3dDrawPrimitivesProf;
    STAMPROFILE             StatR3Cmd3dSurfaceDmaProf;
    STAMCOUNTER             StatR3CmdDefineGmr2;
    STAMCOUNTER             StatR3CmdDefineGmr2Free;
    STAMCOUNTER             StatR3CmdDefineGmr2Modify;
    STAMCOUNTER             StatR3CmdRemapGmr2;
    STAMCOUNTER             StatR3CmdRemapGmr2Modify;
    STAMCOUNTER             StatR3CmdInvalidCmd;
    STAMCOUNTER             StatR3CmdFence;
    STAMCOUNTER             StatR3CmdUpdate;
    STAMCOUNTER             StatR3CmdUpdateVerbose;
    STAMCOUNTER             StatR3CmdDefineCursor;
    STAMCOUNTER             StatR3CmdDefineAlphaCursor;
    STAMCOUNTER             StatR3CmdEscape;
    STAMCOUNTER             StatR3CmdDefineScreen;
    STAMCOUNTER             StatR3CmdDestroyScreen;
    STAMCOUNTER             StatR3CmdDefineGmrFb;
    STAMCOUNTER             StatR3CmdBlitGmrFbToScreen;
    STAMCOUNTER             StatR3CmdBlitScreentoGmrFb;
    STAMCOUNTER             StatR3CmdAnnotationFill;
    STAMCOUNTER             StatR3CmdAnnotationCopy;
    STAMCOUNTER             StatR3Cmd3dSurfaceDefine;
    STAMCOUNTER             StatR3Cmd3dSurfaceDefineV2;
    STAMCOUNTER             StatR3Cmd3dSurfaceDestroy;
    STAMCOUNTER             StatR3Cmd3dSurfaceCopy;
    STAMCOUNTER             StatR3Cmd3dSurfaceStretchBlt;
    STAMCOUNTER             StatR3Cmd3dSurfaceDma;
    STAMCOUNTER             StatR3Cmd3dSurfaceScreen;
    STAMCOUNTER             StatR3Cmd3dContextDefine;
    STAMCOUNTER             StatR3Cmd3dContextDestroy;
    STAMCOUNTER             StatR3Cmd3dSetTransform;
    STAMCOUNTER             StatR3Cmd3dSetZRange;
    STAMCOUNTER             StatR3Cmd3dSetRenderState;
    STAMCOUNTER             StatR3Cmd3dSetRenderTarget;
    STAMCOUNTER             StatR3Cmd3dSetTextureState;
    STAMCOUNTER             StatR3Cmd3dSetMaterial;
    STAMCOUNTER             StatR3Cmd3dSetLightData;
    STAMCOUNTER             StatR3Cmd3dSetLightEnable;
    STAMCOUNTER             StatR3Cmd3dSetViewPort;
    STAMCOUNTER             StatR3Cmd3dSetClipPlane;
    STAMCOUNTER             StatR3Cmd3dClear;
    STAMCOUNTER             StatR3Cmd3dPresent;
    STAMCOUNTER             StatR3Cmd3dPresentReadBack;
    STAMCOUNTER             StatR3Cmd3dShaderDefine;
    STAMCOUNTER             StatR3Cmd3dShaderDestroy;
    STAMCOUNTER             StatR3Cmd3dSetShader;
    STAMCOUNTER             StatR3Cmd3dSetShaderConst;
    STAMCOUNTER             StatR3Cmd3dDrawPrimitives;
    STAMCOUNTER             StatR3Cmd3dSetScissorRect;
    STAMCOUNTER             StatR3Cmd3dBeginQuery;
    STAMCOUNTER             StatR3Cmd3dEndQuery;
    STAMCOUNTER             StatR3Cmd3dWaitForQuery;
    STAMCOUNTER             StatR3Cmd3dGenerateMipmaps;
    STAMCOUNTER             StatR3Cmd3dActivateSurface;
    STAMCOUNTER             StatR3Cmd3dDeactivateSurface;

    STAMCOUNTER             StatR3RegConfigDoneWr;
    STAMCOUNTER             StatR3RegGmrDescriptorWr;
    STAMCOUNTER             StatR3RegGmrDescriptorWrErrors;
    STAMCOUNTER             StatR3RegGmrDescriptorWrFree;

    STAMCOUNTER             StatFifoCommands;
    STAMCOUNTER             StatFifoErrors;
    STAMCOUNTER             StatFifoUnkCmds;
    STAMCOUNTER             StatFifoTodoTimeout;
    STAMCOUNTER             StatFifoTodoWoken;
    STAMPROFILE             StatFifoStalls;

} VMSVGAR3STATE, *PVMSVGAR3STATE;
#endif /* IN_RING3 */


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#ifdef IN_RING3
# ifdef DEBUG_FIFO_ACCESS
static FNPGMPHYSHANDLER vmsvgaR3FIFOAccessHandler;
# endif
# ifdef DEBUG_GMR_ACCESS
static FNPGMPHYSHANDLER vmsvgaR3GMRAccessHandler;
# endif
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef IN_RING3

/**
 * SSM descriptor table for the VMSVGAGMRDESCRIPTOR structure.
 */
static SSMFIELD const g_aVMSVGAGMRDESCRIPTORFields[] =
{
    SSMFIELD_ENTRY_GCPHYS(      VMSVGAGMRDESCRIPTOR,     GCPhys),
    SSMFIELD_ENTRY(             VMSVGAGMRDESCRIPTOR,     numPages),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the GMR structure.
 */
static SSMFIELD const g_aGMRFields[] =
{
    SSMFIELD_ENTRY(             GMR, cMaxPages),
    SSMFIELD_ENTRY(             GMR, cbTotal),
    SSMFIELD_ENTRY(             GMR, numDescriptors),
    SSMFIELD_ENTRY_IGN_HCPTR(   GMR, paDesc),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the VMSVGAR3STATE structure.
 */
static SSMFIELD const g_aVMSVGAR3STATEFields[] =
{
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, paGMR),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, GMRFB),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, Cursor.fActive),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, Cursor.xHotspot),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, Cursor.yHotspot),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, Cursor.width),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, Cursor.height),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, Cursor.cbData),
    SSMFIELD_ENTRY_IGN_HCPTR(   VMSVGAR3STATE, Cursor.pData),
    SSMFIELD_ENTRY(             VMSVGAR3STATE, colorAnnotation),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, cBusyDelayedEmts),
#ifdef VMSVGA_USE_EMT_HALT_CODE
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, BusyDelayedEmts),
#else
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, hBusyDelayedEmts),
#endif
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatBusyDelayEmts),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dPresentProf),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dDrawPrimitivesProf),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceDmaProf),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineGmr2),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineGmr2Free),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineGmr2Modify),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdRemapGmr2),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdRemapGmr2Modify),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdInvalidCmd),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdFence),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdUpdate),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdUpdateVerbose),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineCursor),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineAlphaCursor),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdEscape),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineScreen),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDestroyScreen),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdDefineGmrFb),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdBlitGmrFbToScreen),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdBlitScreentoGmrFb),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdAnnotationFill),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3CmdAnnotationCopy),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceDefine),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceDefineV2),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceDestroy),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceCopy),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceStretchBlt),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceDma),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSurfaceScreen),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dContextDefine),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dContextDestroy),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetTransform),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetZRange),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetRenderState),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetRenderTarget),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetTextureState),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetMaterial),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetLightData),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetLightEnable),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetViewPort),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetClipPlane),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dClear),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dPresent),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dPresentReadBack),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dShaderDefine),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dShaderDestroy),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetShader),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetShaderConst),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dDrawPrimitives),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dSetScissorRect),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dBeginQuery),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dEndQuery),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dWaitForQuery),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dGenerateMipmaps),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dActivateSurface),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3Cmd3dDeactivateSurface),

    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3RegConfigDoneWr),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3RegGmrDescriptorWr),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3RegGmrDescriptorWrErrors),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatR3RegGmrDescriptorWrFree),

    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatFifoCommands),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatFifoErrors),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatFifoUnkCmds),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatFifoTodoTimeout),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatFifoTodoWoken),
    SSMFIELD_ENTRY_IGNORE(      VMSVGAR3STATE, StatFifoStalls),
    SSMFIELD_ENTRY_TERM()
};

/**
 * SSM descriptor table for the VGAState.svga structure.
 */
static SSMFIELD const g_aVGAStateSVGAFields[] =
{
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, u64HostWindowId),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, pFIFOR3),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, pFIFOR0),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, pSvgaR3State),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, p3dState),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, pbVgaFrameBufferR3),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, pvFIFOExtCmdParam),
    SSMFIELD_ENTRY_IGN_GCPHYS(      VMSVGAState, GCPhysFIFO),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, cbFIFO),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, cbFIFOConfig),
    SSMFIELD_ENTRY(                 VMSVGAState, u32SVGAId),
    SSMFIELD_ENTRY(                 VMSVGAState, fEnabled),
    SSMFIELD_ENTRY(                 VMSVGAState, fConfigured),
    SSMFIELD_ENTRY(                 VMSVGAState, fBusy),
    SSMFIELD_ENTRY(                 VMSVGAState, fTraces),
    SSMFIELD_ENTRY(                 VMSVGAState, u32GuestId),
    SSMFIELD_ENTRY(                 VMSVGAState, cScratchRegion),
    SSMFIELD_ENTRY(                 VMSVGAState, au32ScratchRegion),
    SSMFIELD_ENTRY(                 VMSVGAState, u32IrqStatus),
    SSMFIELD_ENTRY(                 VMSVGAState, u32IrqMask),
    SSMFIELD_ENTRY(                 VMSVGAState, u32PitchLock),
    SSMFIELD_ENTRY(                 VMSVGAState, u32CurrentGMRId),
    SSMFIELD_ENTRY(                 VMSVGAState, u32RegCaps),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, BasePort),
    SSMFIELD_ENTRY(                 VMSVGAState, u32IndexReg),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, pSupDrvSession),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, FIFORequestSem),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, FIFOExtCmdSem),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGAState, pFIFOIOThread),
    SSMFIELD_ENTRY(                 VMSVGAState, uWidth),
    SSMFIELD_ENTRY(                 VMSVGAState, uHeight),
    SSMFIELD_ENTRY(                 VMSVGAState, uBpp),
    SSMFIELD_ENTRY(                 VMSVGAState, cbScanline),
    SSMFIELD_ENTRY(                 VMSVGAState, u32MaxWidth),
    SSMFIELD_ENTRY(                 VMSVGAState, u32MaxHeight),
    SSMFIELD_ENTRY(                 VMSVGAState, u32ActionFlags),
    SSMFIELD_ENTRY(                 VMSVGAState, f3DEnabled),
    SSMFIELD_ENTRY(                 VMSVGAState, fVRAMTracking),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, u8FIFOExtCommand),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, fFifoExtCommandWakeup),
    SSMFIELD_ENTRY_IGNORE(          VMSVGAState, cGMR),
    SSMFIELD_ENTRY_TERM()
};

static void vmsvgaSetTraces(PVGASTATE pThis, bool fTraces);

#endif /* IN_RING3 */

#ifdef LOG_ENABLED

/**
 * Index register string name lookup
 *
 * @returns Index register string or "UNKNOWN"
 * @param   pThis       VMSVGA State
 * @param   idxReg      The index register.
 */
static const char *vmsvgaIndexToString(PVGASTATE pThis, uint32_t idxReg)
{
    switch (idxReg)
    {
        case SVGA_REG_ID:                       return "SVGA_REG_ID";
        case SVGA_REG_ENABLE:                   return "SVGA_REG_ENABLE";
        case SVGA_REG_WIDTH:                    return "SVGA_REG_WIDTH";
        case SVGA_REG_HEIGHT:                   return "SVGA_REG_HEIGHT";
        case SVGA_REG_MAX_WIDTH:                return "SVGA_REG_MAX_WIDTH";
        case SVGA_REG_MAX_HEIGHT:               return "SVGA_REG_MAX_HEIGHT";
        case SVGA_REG_DEPTH:                    return "SVGA_REG_DEPTH";
        case SVGA_REG_BITS_PER_PIXEL:           return "SVGA_REG_BITS_PER_PIXEL"; /* Current bpp in the guest */
        case SVGA_REG_HOST_BITS_PER_PIXEL:      return "SVGA_REG_HOST_BITS_PER_PIXEL"; /* (Deprecated) */
        case SVGA_REG_PSEUDOCOLOR:              return "SVGA_REG_PSEUDOCOLOR";
        case SVGA_REG_RED_MASK:                 return "SVGA_REG_RED_MASK";
        case SVGA_REG_GREEN_MASK:               return "SVGA_REG_GREEN_MASK";
        case SVGA_REG_BLUE_MASK:                return "SVGA_REG_BLUE_MASK";
        case SVGA_REG_BYTES_PER_LINE:           return "SVGA_REG_BYTES_PER_LINE";
        case SVGA_REG_VRAM_SIZE:                return "SVGA_REG_VRAM_SIZE"; /* VRAM size */
        case SVGA_REG_FB_START:                 return "SVGA_REG_FB_START"; /* Frame buffer physical address. */
        case SVGA_REG_FB_OFFSET:                return "SVGA_REG_FB_OFFSET"; /* Offset of the frame buffer in VRAM */
        case SVGA_REG_FB_SIZE:                  return "SVGA_REG_FB_SIZE"; /* Frame buffer size */
        case SVGA_REG_CAPABILITIES:             return "SVGA_REG_CAPABILITIES";
        case SVGA_REG_MEM_START:                return "SVGA_REG_MEM_START"; /* FIFO start */
        case SVGA_REG_MEM_SIZE:                 return "SVGA_REG_MEM_SIZE"; /* FIFO size */
        case SVGA_REG_CONFIG_DONE:              return "SVGA_REG_CONFIG_DONE"; /* Set when memory area configured */
        case SVGA_REG_SYNC:                     return "SVGA_REG_SYNC"; /* See "FIFO Synchronization Registers" */
        case SVGA_REG_BUSY:                     return "SVGA_REG_BUSY"; /* See "FIFO Synchronization Registers" */
        case SVGA_REG_GUEST_ID:                 return "SVGA_REG_GUEST_ID"; /* Set guest OS identifier */
        case SVGA_REG_SCRATCH_SIZE:             return "SVGA_REG_SCRATCH_SIZE"; /* Number of scratch registers */
        case SVGA_REG_MEM_REGS:                 return "SVGA_REG_MEM_REGS"; /* Number of FIFO registers */
        case SVGA_REG_PITCHLOCK:                return "SVGA_REG_PITCHLOCK"; /* Fixed pitch for all modes */
        case SVGA_REG_IRQMASK:                  return "SVGA_REG_IRQMASK"; /* Interrupt mask */
        case SVGA_REG_GMR_ID:                   return "SVGA_REG_GMR_ID";
        case SVGA_REG_GMR_DESCRIPTOR:           return "SVGA_REG_GMR_DESCRIPTOR";
        case SVGA_REG_GMR_MAX_IDS:              return "SVGA_REG_GMR_MAX_IDS";
        case SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH:return "SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH";
        case SVGA_REG_TRACES:                   return "SVGA_REG_TRACES"; /* Enable trace-based updates even when FIFO is on */
        case SVGA_REG_GMRS_MAX_PAGES:           return "SVGA_REG_GMRS_MAX_PAGES"; /* Maximum number of 4KB pages for all GMRs */
        case SVGA_REG_MEMORY_SIZE:              return "SVGA_REG_MEMORY_SIZE"; /* Total dedicated device memory excluding FIFO */
        case SVGA_REG_TOP:                      return "SVGA_REG_TOP"; /* Must be 1 more than the last register */
        case SVGA_PALETTE_BASE:                 return "SVGA_PALETTE_BASE"; /* Base of SVGA color map */
        case SVGA_REG_CURSOR_ID:                return "SVGA_REG_CURSOR_ID";
        case SVGA_REG_CURSOR_X:                 return "SVGA_REG_CURSOR_X";
        case SVGA_REG_CURSOR_Y:                 return "SVGA_REG_CURSOR_Y";
        case SVGA_REG_CURSOR_ON:                return "SVGA_REG_CURSOR_ON";
        case SVGA_REG_NUM_GUEST_DISPLAYS:       return "SVGA_REG_NUM_GUEST_DISPLAYS"; /* Number of guest displays in X/Y direction */
        case SVGA_REG_DISPLAY_ID:               return "SVGA_REG_DISPLAY_ID";  /* Display ID for the following display attributes */
        case SVGA_REG_DISPLAY_IS_PRIMARY:       return "SVGA_REG_DISPLAY_IS_PRIMARY"; /* Whether this is a primary display */
        case SVGA_REG_DISPLAY_POSITION_X:       return "SVGA_REG_DISPLAY_POSITION_X"; /* The display position x */
        case SVGA_REG_DISPLAY_POSITION_Y:       return "SVGA_REG_DISPLAY_POSITION_Y"; /* The display position y */
        case SVGA_REG_DISPLAY_WIDTH:            return "SVGA_REG_DISPLAY_WIDTH";    /* The display's width */
        case SVGA_REG_DISPLAY_HEIGHT:           return "SVGA_REG_DISPLAY_HEIGHT";   /* The display's height */
        case SVGA_REG_NUM_DISPLAYS:             return "SVGA_REG_NUM_DISPLAYS";     /* (Deprecated) */

        default:
            if (idxReg - (uint32_t)SVGA_SCRATCH_BASE < pThis->svga.cScratchRegion)
                return "SVGA_SCRATCH_BASE reg";
            if (idxReg - (uint32_t)SVGA_PALETTE_BASE < (uint32_t)SVGA_NUM_PALETTE_REGS)
                return "SVGA_PALETTE_BASE reg";
            return "UNKNOWN";
    }
}

#ifdef IN_RING3
/**
 * FIFO command name lookup
 *
 * @returns FIFO command string or "UNKNOWN"
 * @param   u32Cmd      FIFO command
 */
static const char *vmsvgaFIFOCmdToString(uint32_t u32Cmd)
{
    switch (u32Cmd)
    {
        case SVGA_CMD_INVALID_CMD:              return "SVGA_CMD_INVALID_CMD";
        case SVGA_CMD_UPDATE:                   return "SVGA_CMD_UPDATE";
        case SVGA_CMD_RECT_COPY:                return "SVGA_CMD_RECT_COPY";
        case SVGA_CMD_DEFINE_CURSOR:            return "SVGA_CMD_DEFINE_CURSOR";
        case SVGA_CMD_DEFINE_ALPHA_CURSOR:      return "SVGA_CMD_DEFINE_ALPHA_CURSOR";
        case SVGA_CMD_UPDATE_VERBOSE:           return "SVGA_CMD_UPDATE_VERBOSE";
        case SVGA_CMD_FRONT_ROP_FILL:           return "SVGA_CMD_FRONT_ROP_FILL";
        case SVGA_CMD_FENCE:                    return "SVGA_CMD_FENCE";
        case SVGA_CMD_ESCAPE:                   return "SVGA_CMD_ESCAPE";
        case SVGA_CMD_DEFINE_SCREEN:            return "SVGA_CMD_DEFINE_SCREEN";
        case SVGA_CMD_DESTROY_SCREEN:           return "SVGA_CMD_DESTROY_SCREEN";
        case SVGA_CMD_DEFINE_GMRFB:             return "SVGA_CMD_DEFINE_GMRFB";
        case SVGA_CMD_BLIT_GMRFB_TO_SCREEN:     return "SVGA_CMD_BLIT_GMRFB_TO_SCREEN";
        case SVGA_CMD_BLIT_SCREEN_TO_GMRFB:     return "SVGA_CMD_BLIT_SCREEN_TO_GMRFB";
        case SVGA_CMD_ANNOTATION_FILL:          return "SVGA_CMD_ANNOTATION_FILL";
        case SVGA_CMD_ANNOTATION_COPY:          return "SVGA_CMD_ANNOTATION_COPY";
        case SVGA_CMD_DEFINE_GMR2:              return "SVGA_CMD_DEFINE_GMR2";
        case SVGA_CMD_REMAP_GMR2:               return "SVGA_CMD_REMAP_GMR2";
        case SVGA_3D_CMD_SURFACE_DEFINE:        return "SVGA_3D_CMD_SURFACE_DEFINE";
        case SVGA_3D_CMD_SURFACE_DESTROY:       return "SVGA_3D_CMD_SURFACE_DESTROY";
        case SVGA_3D_CMD_SURFACE_COPY:          return "SVGA_3D_CMD_SURFACE_COPY";
        case SVGA_3D_CMD_SURFACE_STRETCHBLT:    return "SVGA_3D_CMD_SURFACE_STRETCHBLT";
        case SVGA_3D_CMD_SURFACE_DMA:           return "SVGA_3D_CMD_SURFACE_DMA";
        case SVGA_3D_CMD_CONTEXT_DEFINE:        return "SVGA_3D_CMD_CONTEXT_DEFINE";
        case SVGA_3D_CMD_CONTEXT_DESTROY:       return "SVGA_3D_CMD_CONTEXT_DESTROY";
        case SVGA_3D_CMD_SETTRANSFORM:          return "SVGA_3D_CMD_SETTRANSFORM";
        case SVGA_3D_CMD_SETZRANGE:             return "SVGA_3D_CMD_SETZRANGE";
        case SVGA_3D_CMD_SETRENDERSTATE:        return "SVGA_3D_CMD_SETRENDERSTATE";
        case SVGA_3D_CMD_SETRENDERTARGET:       return "SVGA_3D_CMD_SETRENDERTARGET";
        case SVGA_3D_CMD_SETTEXTURESTATE:       return "SVGA_3D_CMD_SETTEXTURESTATE";
        case SVGA_3D_CMD_SETMATERIAL:           return "SVGA_3D_CMD_SETMATERIAL";
        case SVGA_3D_CMD_SETLIGHTDATA:          return "SVGA_3D_CMD_SETLIGHTDATA";
        case SVGA_3D_CMD_SETLIGHTENABLED:       return "SVGA_3D_CMD_SETLIGHTENABLED";
        case SVGA_3D_CMD_SETVIEWPORT:           return "SVGA_3D_CMD_SETVIEWPORT";
        case SVGA_3D_CMD_SETCLIPPLANE:          return "SVGA_3D_CMD_SETCLIPPLANE";
        case SVGA_3D_CMD_CLEAR:                 return "SVGA_3D_CMD_CLEAR";
        case SVGA_3D_CMD_PRESENT:               return "SVGA_3D_CMD_PRESENT";
        case SVGA_3D_CMD_SHADER_DEFINE:         return "SVGA_3D_CMD_SHADER_DEFINE";
        case SVGA_3D_CMD_SHADER_DESTROY:        return "SVGA_3D_CMD_SHADER_DESTROY";
        case SVGA_3D_CMD_SET_SHADER:            return "SVGA_3D_CMD_SET_SHADER";
        case SVGA_3D_CMD_SET_SHADER_CONST:      return "SVGA_3D_CMD_SET_SHADER_CONST";
        case SVGA_3D_CMD_DRAW_PRIMITIVES:       return "SVGA_3D_CMD_DRAW_PRIMITIVES";
        case SVGA_3D_CMD_SETSCISSORRECT:        return "SVGA_3D_CMD_SETSCISSORRECT";
        case SVGA_3D_CMD_BEGIN_QUERY:           return "SVGA_3D_CMD_BEGIN_QUERY";
        case SVGA_3D_CMD_END_QUERY:             return "SVGA_3D_CMD_END_QUERY";
        case SVGA_3D_CMD_WAIT_FOR_QUERY:        return "SVGA_3D_CMD_WAIT_FOR_QUERY";
        case SVGA_3D_CMD_PRESENT_READBACK:      return "SVGA_3D_CMD_PRESENT_READBACK";
        case SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN:return "SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN";
        case SVGA_3D_CMD_SURFACE_DEFINE_V2:     return "SVGA_3D_CMD_SURFACE_DEFINE_V2";
        case SVGA_3D_CMD_GENERATE_MIPMAPS:      return "SVGA_3D_CMD_GENERATE_MIPMAPS";
        case SVGA_3D_CMD_ACTIVATE_SURFACE:      return "SVGA_3D_CMD_ACTIVATE_SURFACE";
        case SVGA_3D_CMD_DEACTIVATE_SURFACE:    return "SVGA_3D_CMD_DEACTIVATE_SURFACE";
        default:                                return "UNKNOWN";
    }
}
# endif /* IN_RING3 */

#endif /* LOG_ENABLED */

#ifdef IN_RING3
/**
 * @interface_method_impl{PDMIDISPLAYPORT,pfnSetViewport}
 */
DECLCALLBACK(void) vmsvgaPortSetViewport(PPDMIDISPLAYPORT pInterface, uint32_t idScreen, uint32_t x, uint32_t y, uint32_t cx, uint32_t cy)
{
    PVGASTATE pThis = RT_FROM_MEMBER(pInterface, VGASTATE, IPort);

    Log(("vmsvgaPortSetViewPort: screen %d (%d,%d)(%d,%d)\n", idScreen, x, y, cx, cy));
    VMSVGAVIEWPORT const OldViewport = pThis->svga.viewport;

    if (x < pThis->svga.uWidth)
    {
        pThis->svga.viewport.x      = x;
        pThis->svga.viewport.cx     = RT_MIN(cx, pThis->svga.uWidth - x);
        pThis->svga.viewport.xRight = x + pThis->svga.viewport.cx;
    }
    else
    {
        pThis->svga.viewport.x      = pThis->svga.uWidth;
        pThis->svga.viewport.cx     = 0;
        pThis->svga.viewport.xRight = pThis->svga.uWidth;
    }
    if (y < pThis->svga.uHeight)
    {
        pThis->svga.viewport.y       = y;
        pThis->svga.viewport.cy      = RT_MIN(cy, pThis->svga.uHeight - y);
        pThis->svga.viewport.yLowWC  = pThis->svga.uHeight - y - pThis->svga.viewport.cy;
        pThis->svga.viewport.yHighWC = pThis->svga.uHeight - y;
    }
    else
    {
        pThis->svga.viewport.y       = pThis->svga.uHeight;
        pThis->svga.viewport.cy      = 0;
        pThis->svga.viewport.yLowWC  = 0;
        pThis->svga.viewport.yHighWC = 0;
    }

# ifdef VBOX_WITH_VMSVGA3D
    /*
     * Now inform the 3D backend.
     */
    if (pThis->svga.f3DEnabled)
        vmsvga3dUpdateHostScreenViewport(pThis, idScreen, &OldViewport);
# else
    RT_NOREF(idScreen, OldViewport);
# endif
}
#endif /* IN_RING3 */

/**
 * Read port register
 *
 * @returns VBox status code.
 * @param   pThis       VMSVGA State
 * @param   pu32        Where to store the read value
 */
PDMBOTHCBDECL(int) vmsvgaReadPort(PVGASTATE pThis, uint32_t *pu32)
{
    int rc = VINF_SUCCESS;
    *pu32 = 0;

    /* Rough index register validation. */
    uint32_t idxReg = pThis->svga.u32IndexReg;
#if !defined(IN_RING3) && defined(VBOX_STRICT)
    ASSERT_GUEST_MSG_RETURN(idxReg < SVGA_SCRATCH_BASE + pThis->svga.cScratchRegion, ("idxReg=%#x\n", idxReg),
                            VINF_IOM_R3_IOPORT_READ);
#else
    ASSERT_GUEST_MSG_STMT_RETURN(idxReg < SVGA_SCRATCH_BASE + pThis->svga.cScratchRegion, ("idxReg=%#x\n", idxReg),
                                 STAM_REL_COUNTER_INC(&pThis->svga.StatRegUnknownRd),
                                 VINF_SUCCESS);
#endif
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* We must adjust the register number if we're in SVGA_ID_0 mode because the PALETTE range moved. */
    if (   idxReg >= SVGA_REG_CAPABILITIES
        && pThis->svga.u32SVGAId == SVGA_ID_0)
    {
        idxReg += SVGA_PALETTE_BASE - SVGA_REG_CAPABILITIES;
        Log(("vmsvgaWritePort: SVGA_ID_0 reg adj %#x -> %#x\n", pThis->svga.u32IndexReg, idxReg));
    }

    switch (idxReg)
    {
    case SVGA_REG_ID:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegIdRd);
        *pu32 = pThis->svga.u32SVGAId;
        break;

    case SVGA_REG_ENABLE:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegEnableRd);
        *pu32 = pThis->svga.fEnabled;
        break;

    case SVGA_REG_WIDTH:
    {
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegWidthRd);
        if (    pThis->svga.fEnabled
            &&  pThis->svga.uWidth != VMSVGA_VAL_UNINITIALIZED)
        {
            *pu32 = pThis->svga.uWidth;
        }
        else
        {
#ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_READ;
#else
            *pu32 = pThis->pDrv->cx;
#endif
        }
        break;
    }

    case SVGA_REG_HEIGHT:
    {
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegHeightRd);
        if (    pThis->svga.fEnabled
            &&  pThis->svga.uHeight != VMSVGA_VAL_UNINITIALIZED)
        {
            *pu32 = pThis->svga.uHeight;
        }
        else
        {
#ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_READ;
#else
            *pu32 = pThis->pDrv->cy;
#endif
        }
        break;
    }

    case SVGA_REG_MAX_WIDTH:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegMaxWidthRd);
        *pu32 = pThis->svga.u32MaxWidth;
        break;

    case SVGA_REG_MAX_HEIGHT:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegMaxHeightRd);
        *pu32 = pThis->svga.u32MaxHeight;
        break;

    case SVGA_REG_DEPTH:
        /* This returns the color depth of the current mode. */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDepthRd);
        switch (pThis->svga.uBpp)
        {
        case 15:
        case 16:
        case 24:
            *pu32 = pThis->svga.uBpp;
            break;

        default:
        case 32:
            *pu32 = 24; /* The upper 8 bits are either alpha bits or not used. */
            break;
        }
        break;

    case SVGA_REG_HOST_BITS_PER_PIXEL: /* (Deprecated) */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegHostBitsPerPixelRd);
        if (    pThis->svga.fEnabled
            &&  pThis->svga.uBpp != VMSVGA_VAL_UNINITIALIZED)
        {
            *pu32 = pThis->svga.uBpp;
        }
        else
        {
#ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_READ;
#else
            *pu32 = pThis->pDrv->cBits;
#endif
        }
        break;

    case SVGA_REG_BITS_PER_PIXEL:      /* Current bpp in the guest */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegBitsPerPixelRd);
        if (    pThis->svga.fEnabled
            &&  pThis->svga.uBpp != VMSVGA_VAL_UNINITIALIZED)
        {
            *pu32 = (pThis->svga.uBpp + 7) & ~7;
        }
        else
        {
#ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_READ;
#else
            *pu32 = (pThis->pDrv->cBits + 7) & ~7;
#endif
        }
        break;

    case SVGA_REG_PSEUDOCOLOR:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegPsuedoColorRd);
        *pu32 = pThis->svga.uBpp == 8; /* See section 6 "Pseudocolor" in svga_interface.txt. */
        break;

    case SVGA_REG_RED_MASK:
    case SVGA_REG_GREEN_MASK:
    case SVGA_REG_BLUE_MASK:
    {
        uint32_t uBpp;

        if (    pThis->svga.fEnabled
            &&  pThis->svga.uBpp != VMSVGA_VAL_UNINITIALIZED)
        {
            uBpp = pThis->svga.uBpp;
        }
        else
        {
#ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_READ;
            break;
#else
            uBpp = pThis->pDrv->cBits;
#endif
        }
        uint32_t u32RedMask, u32GreenMask, u32BlueMask;
        switch (uBpp)
        {
        case 8:
            u32RedMask   = 0x07;
            u32GreenMask = 0x38;
            u32BlueMask  = 0xc0;
            break;

        case 15:
            u32RedMask   = 0x0000001f;
            u32GreenMask = 0x000003e0;
            u32BlueMask  = 0x00007c00;
            break;

        case 16:
            u32RedMask   = 0x0000001f;
            u32GreenMask = 0x000007e0;
            u32BlueMask  = 0x0000f800;
            break;

        case 24:
        case 32:
        default:
            u32RedMask   = 0x00ff0000;
            u32GreenMask = 0x0000ff00;
            u32BlueMask  = 0x000000ff;
            break;
        }
        switch (idxReg)
        {
        case SVGA_REG_RED_MASK:
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegRedMaskRd);
            *pu32 = u32RedMask;
            break;

        case SVGA_REG_GREEN_MASK:
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegGreenMaskRd);
            *pu32 = u32GreenMask;
            break;

        case SVGA_REG_BLUE_MASK:
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegBlueMaskRd);
            *pu32 = u32BlueMask;
            break;
        }
        break;
    }

    case SVGA_REG_BYTES_PER_LINE:
    {
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegBytesPerLineRd);
        if (    pThis->svga.fEnabled
            &&  pThis->svga.cbScanline)
        {
            *pu32 = pThis->svga.cbScanline;
        }
        else
        {
#ifndef IN_RING3
            rc = VINF_IOM_R3_IOPORT_READ;
#else
            *pu32 = pThis->pDrv->cbScanline;
#endif
        }
        break;
    }

    case SVGA_REG_VRAM_SIZE:            /* VRAM size */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegVramSizeRd);
        *pu32 = pThis->vram_size;
        break;

    case SVGA_REG_FB_START:             /* Frame buffer physical address. */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegFbStartRd);
        Assert(pThis->GCPhysVRAM <= 0xffffffff);
        *pu32 = pThis->GCPhysVRAM;
        break;

    case SVGA_REG_FB_OFFSET:            /* Offset of the frame buffer in VRAM */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegFbOffsetRd);
        /* Always zero in our case. */
        *pu32 = 0;
        break;

    case SVGA_REG_FB_SIZE:              /* Frame buffer size */
    {
#ifndef IN_RING3
        rc = VINF_IOM_R3_IOPORT_READ;
#else
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegFbSizeRd);

        /* VMWare testcases want at least 4 MB in case the hardware is disabled. */
        if (    pThis->svga.fEnabled
            &&  pThis->svga.uHeight != VMSVGA_VAL_UNINITIALIZED)
        {
            /* Hardware enabled; return real framebuffer size .*/
            *pu32 = (uint32_t)pThis->svga.uHeight * pThis->svga.cbScanline;
        }
        else
            *pu32 = RT_MAX(0x100000, (uint32_t)pThis->pDrv->cy * pThis->pDrv->cbScanline);

        *pu32 = RT_MIN(pThis->vram_size, *pu32);
        Log(("h=%d w=%d bpp=%d\n", pThis->pDrv->cy, pThis->pDrv->cx, pThis->pDrv->cBits));
#endif
        break;
    }

    case SVGA_REG_CAPABILITIES:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegCapabilitesRd);
        *pu32 = pThis->svga.u32RegCaps;
        break;

    case SVGA_REG_MEM_START:           /* FIFO start */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegMemStartRd);
        Assert(pThis->svga.GCPhysFIFO <= 0xffffffff);
        *pu32 = pThis->svga.GCPhysFIFO;
        break;

    case SVGA_REG_MEM_SIZE:            /* FIFO size */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegMemSizeRd);
        *pu32 = pThis->svga.cbFIFO;
        break;

    case SVGA_REG_CONFIG_DONE:         /* Set when memory area configured */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegConfigDoneRd);
        *pu32 = pThis->svga.fConfigured;
        break;

    case SVGA_REG_SYNC:                /* See "FIFO Synchronization Registers" */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegSyncRd);
        *pu32 = 0;
        break;

    case SVGA_REG_BUSY:                /* See "FIFO Synchronization Registers" */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegBusyRd);
        if (pThis->svga.fBusy)
        {
#ifndef IN_RING3
            /* Go to ring-3 and halt the CPU. */
            rc = VINF_IOM_R3_IOPORT_READ;
            break;
#else
# if defined(VMSVGA_USE_EMT_HALT_CODE)
            /* The guest is basically doing a HLT via the device here, but with
               a special wake up condition on FIFO completion. */
            PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;
            STAM_REL_PROFILE_START(&pSVGAState->StatBusyDelayEmts, EmtDelay);
            PVM         pVM   = PDMDevHlpGetVM(pThis->pDevInsR3);
            VMCPUID     idCpu = PDMDevHlpGetCurrentCpuId(pThis->pDevInsR3);
            VMCPUSET_ATOMIC_ADD(&pSVGAState->BusyDelayedEmts, idCpu);
            ASMAtomicIncU32(&pSVGAState->cBusyDelayedEmts);
            if (pThis->svga.fBusy)
                rc = VMR3WaitForDeviceReady(pVM, idCpu);
            ASMAtomicDecU32(&pSVGAState->cBusyDelayedEmts);
            VMCPUSET_ATOMIC_DEL(&pSVGAState->BusyDelayedEmts, idCpu);
# else

            /* Delay the EMT a bit so the FIFO and others can get some work done.
               This used to be a crude 50 ms sleep. The current code tries to be
               more efficient, but the consept is still very crude. */
            PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;
            STAM_REL_PROFILE_START(&pSVGAState->StatBusyDelayEmts, EmtDelay);
            RTThreadYield();
            if (pThis->svga.fBusy)
            {
                uint32_t cRefs = ASMAtomicIncU32(&pSVGAState->cBusyDelayedEmts);

                if (pThis->svga.fBusy && cRefs == 1)
                    RTSemEventMultiReset(pSVGAState->hBusyDelayedEmts);
                if (pThis->svga.fBusy)
                {
                    /** @todo If this code is going to stay, we need to call into the halt/wait
                     *        code in VMEmt.cpp here, otherwise all kind of EMT interaction will
                     *        suffer when the guest is polling on a busy FIFO. */
                    uint64_t cNsMaxWait = TMVirtualSyncGetNsToDeadline(PDMDevHlpGetVM(pThis->pDevInsR3));
                    if (cNsMaxWait >= RT_NS_100US)
                        RTSemEventMultiWaitEx(pSVGAState->hBusyDelayedEmts,
                                              RTSEMWAIT_FLAGS_NANOSECS | RTSEMWAIT_FLAGS_RELATIVE | RTSEMWAIT_FLAGS_NORESUME,
                                              RT_MIN(cNsMaxWait, RT_NS_10MS));
                }

                ASMAtomicDecU32(&pSVGAState->cBusyDelayedEmts);
            }
            STAM_REL_PROFILE_STOP(&pSVGAState->StatBusyDelayEmts, EmtDelay);
# endif
            *pu32 = pThis->svga.fBusy != 0;
#endif
        }
        else
            *pu32 = false;
        break;

    case SVGA_REG_GUEST_ID:            /* Set guest OS identifier */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGuestIdRd);
        *pu32 = pThis->svga.u32GuestId;
        break;

    case SVGA_REG_SCRATCH_SIZE:        /* Number of scratch registers */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegScratchSizeRd);
        *pu32 = pThis->svga.cScratchRegion;
        break;

    case SVGA_REG_MEM_REGS:            /* Number of FIFO registers */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegMemRegsRd);
        *pu32 = SVGA_FIFO_NUM_REGS;
        break;

    case SVGA_REG_PITCHLOCK:           /* Fixed pitch for all modes */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegPitchLockRd);
        *pu32 = pThis->svga.u32PitchLock;
        break;

    case SVGA_REG_IRQMASK:             /* Interrupt mask */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegIrqMaskRd);
        *pu32 = pThis->svga.u32IrqMask;
        break;

    /* See "Guest memory regions" below. */
    case SVGA_REG_GMR_ID:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGmrIdRd);
        *pu32 = pThis->svga.u32CurrentGMRId;
        break;

    case SVGA_REG_GMR_DESCRIPTOR:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegWriteOnlyRd);
        /* Write only */
        *pu32 = 0;
        break;

    case SVGA_REG_GMR_MAX_IDS:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGmrMaxIdsRd);
        *pu32 = pThis->svga.cGMR;
        break;

    case SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGmrMaxDescriptorLengthRd);
        *pu32 = VMSVGA_MAX_GMR_PAGES;
        break;

    case SVGA_REG_TRACES:            /* Enable trace-based updates even when FIFO is on */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegTracesRd);
        *pu32 = pThis->svga.fTraces;
        break;

    case SVGA_REG_GMRS_MAX_PAGES:    /* Maximum number of 4KB pages for all GMRs */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGmrsMaxPagesRd);
        *pu32 = VMSVGA_MAX_GMR_PAGES;
        break;

    case SVGA_REG_MEMORY_SIZE:       /* Total dedicated device memory excluding FIFO */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegMemorySizeRd);
        *pu32 = VMSVGA_SURFACE_SIZE;
        break;

    case SVGA_REG_TOP:               /* Must be 1 more than the last register */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegTopRd);
        break;

    /* Mouse cursor support. */
    case SVGA_REG_CURSOR_ID:
    case SVGA_REG_CURSOR_X:
    case SVGA_REG_CURSOR_Y:
    case SVGA_REG_CURSOR_ON:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegCursorXxxxRd);
        break;

    /* Legacy multi-monitor support */
    case SVGA_REG_NUM_GUEST_DISPLAYS:/* Number of guest displays in X/Y direction */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegNumGuestDisplaysRd);
        *pu32 = 1;
        break;

    case SVGA_REG_DISPLAY_ID:        /* Display ID for the following display attributes */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayIdRd);
        *pu32 = 0;
        break;

    case SVGA_REG_DISPLAY_IS_PRIMARY:/* Whether this is a primary display */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayIsPrimaryRd);
        *pu32 = 0;
        break;

    case SVGA_REG_DISPLAY_POSITION_X:/* The display position x */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayPositionXRd);
        *pu32 = 0;
        break;

    case SVGA_REG_DISPLAY_POSITION_Y:/* The display position y */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayPositionYRd);
        *pu32 = 0;
        break;

    case SVGA_REG_DISPLAY_WIDTH:     /* The display's width */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayWidthRd);
        *pu32 = pThis->svga.uWidth;
        break;

    case SVGA_REG_DISPLAY_HEIGHT:    /* The display's height */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayHeightRd);
        *pu32 = pThis->svga.uHeight;
        break;

    case SVGA_REG_NUM_DISPLAYS:        /* (Deprecated) */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegNumDisplaysRd);
        *pu32 = 1;  /* Must return something sensible here otherwise the Linux driver will take a legacy code path without 3d support. */
        break;

    default:
    {
        uint32_t offReg;
        if ((offReg = idxReg - SVGA_SCRATCH_BASE) < pThis->svga.cScratchRegion)
        {
            RT_UNTRUSTED_VALIDATED_FENCE();
            *pu32 = pThis->svga.au32ScratchRegion[offReg];
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegScratchRd);
        }
        else if ((offReg = idxReg - SVGA_PALETTE_BASE) < (uint32_t)SVGA_NUM_PALETTE_REGS)
        {
            /* Note! Using last_palette rather than palette here to preserve the VGA one. */
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegPaletteRd);
            RT_UNTRUSTED_VALIDATED_FENCE();
            uint32_t u32 = pThis->last_palette[offReg / 3];
            switch (offReg % 3)
            {
                case 0: *pu32 = (u32 >> 16) & 0xff; break; /* red */
                case 1: *pu32 = (u32 >>  8) & 0xff; break; /* green */
                case 2: *pu32 =  u32        & 0xff; break; /* blue */
            }
        }
        else
        {
#if !defined(IN_RING3) && defined(VBOX_STRICT)
            rc = VINF_IOM_R3_IOPORT_READ;
#else
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegUnknownRd);
# ifndef DEBUG_sunlover
            AssertMsgFailed(("reg=%#x\n", idxReg));
# endif
#endif
        }
        break;
    }
    }
    Log(("vmsvgaReadPort index=%s (%d) val=%#x rc=%x\n", vmsvgaIndexToString(pThis, idxReg), idxReg, *pu32, rc));
    return rc;
}

#ifdef IN_RING3
/**
 * Apply the current resolution settings to change the video mode.
 *
 * @returns VBox status code.
 * @param   pThis       VMSVGA State
 */
int vmsvgaChangeMode(PVGASTATE pThis)
{
    int rc;

    if (    pThis->svga.uWidth  == VMSVGA_VAL_UNINITIALIZED
        ||  pThis->svga.uHeight == VMSVGA_VAL_UNINITIALIZED
        ||  pThis->svga.uBpp    == VMSVGA_VAL_UNINITIALIZED)
    {
        /* Mode change in progress; wait for all values to be set. */
        Log(("vmsvgaChangeMode: BOGUS sEnable LFB mode and resize to (%d,%d) bpp=%d\n", pThis->svga.uWidth, pThis->svga.uHeight, pThis->svga.uBpp));
        return VINF_SUCCESS;
    }

    if (    pThis->svga.uWidth == 0
        ||  pThis->svga.uHeight == 0
        ||  pThis->svga.uBpp == 0)
    {
        /* Invalid mode change - BB does this early in the boot up. */
        Log(("vmsvgaChangeMode: BOGUS sEnable LFB mode and resize to (%d,%d) bpp=%d\n", pThis->svga.uWidth, pThis->svga.uHeight, pThis->svga.uBpp));
        return VINF_SUCCESS;
    }

    if (    pThis->last_bpp         == (unsigned)pThis->svga.uBpp
        &&  pThis->last_scr_width   == (unsigned)pThis->svga.uWidth
        &&  pThis->last_scr_height  == (unsigned)pThis->svga.uHeight
        &&  pThis->last_width       == (unsigned)pThis->svga.uWidth
        &&  pThis->last_height      == (unsigned)pThis->svga.uHeight
        )
    {
        /* Nothing to do. */
        Log(("vmsvgaChangeMode: nothing changed; ignore\n"));
        return VINF_SUCCESS;
    }

    Log(("vmsvgaChangeMode: sEnable LFB mode and resize to (%d,%d) bpp=%d\n", pThis->svga.uWidth, pThis->svga.uHeight, pThis->svga.uBpp));
    pThis->svga.cbScanline = ((pThis->svga.uWidth * pThis->svga.uBpp + 7) & ~7) / 8;

    pThis->pDrv->pfnLFBModeChange(pThis->pDrv, true);
    rc = pThis->pDrv->pfnResize(pThis->pDrv, pThis->svga.uBpp, pThis->CTX_SUFF(vram_ptr), pThis->svga.cbScanline, pThis->svga.uWidth, pThis->svga.uHeight);
    AssertRC(rc);
    AssertReturn(rc == VINF_SUCCESS || rc == VINF_VGA_RESIZE_IN_PROGRESS, rc);

    /* last stuff */
    pThis->last_bpp         = pThis->svga.uBpp;
    pThis->last_scr_width   = pThis->svga.uWidth;
    pThis->last_scr_height  = pThis->svga.uHeight;
    pThis->last_width       = pThis->svga.uWidth;
    pThis->last_height      = pThis->svga.uHeight;

    ASMAtomicOrU32(&pThis->svga.u32ActionFlags, VMSVGA_ACTION_CHANGEMODE);

    /* vmsvgaPortSetViewPort not called after state load; set sensible defaults. */
    if (    pThis->svga.viewport.cx == 0
        &&  pThis->svga.viewport.cy == 0)
    {
        pThis->svga.viewport.cx      = pThis->svga.uWidth;
        pThis->svga.viewport.xRight  = pThis->svga.uWidth;
        pThis->svga.viewport.cy      = pThis->svga.uHeight;
        pThis->svga.viewport.yHighWC = pThis->svga.uHeight;
        pThis->svga.viewport.yLowWC  = 0;
    }
    return VINF_SUCCESS;
}
#endif /* IN_RING3 */

#if defined(IN_RING0) || defined(IN_RING3)
/**
 * Safely updates the SVGA_FIFO_BUSY register (in shared memory).
 *
 * @param   pThis               The VMSVGA state.
 * @param   fState              The busy state.
 */
DECLINLINE(void) vmsvgaSafeFifoBusyRegUpdate(PVGASTATE pThis, bool fState)
{
    ASMAtomicWriteU32(&pThis->svga.CTX_SUFF(pFIFO)[SVGA_FIFO_BUSY], fState);

    if (RT_UNLIKELY(fState != (pThis->svga.fBusy != 0)))
    {
        /* Race / unfortunately scheduling. Highly unlikly. */
        uint32_t cLoops = 64;
        do
        {
            ASMNopPause();
            fState = (pThis->svga.fBusy != 0);
            ASMAtomicWriteU32(&pThis->svga.CTX_SUFF(pFIFO)[SVGA_FIFO_BUSY], fState != 0);
        } while (cLoops-- > 0 && fState != (pThis->svga.fBusy != 0));
    }
}
#endif

/**
 * Write port register
 *
 * @returns VBox status code.
 * @param   pThis       VMSVGA State
 * @param   u32         Value to write
 */
PDMBOTHCBDECL(int) vmsvgaWritePort(PVGASTATE pThis, uint32_t u32)
{
#ifdef IN_RING3
    PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;
#endif
    int            rc = VINF_SUCCESS;

    /* Rough index register validation. */
    uint32_t idxReg = pThis->svga.u32IndexReg;
#if !defined(IN_RING3) && defined(VBOX_STRICT)
    ASSERT_GUEST_MSG_RETURN(idxReg < SVGA_SCRATCH_BASE + pThis->svga.cScratchRegion, ("idxReg=%#x\n", idxReg),
                            VINF_IOM_R3_IOPORT_WRITE);
#else
    ASSERT_GUEST_MSG_STMT_RETURN(idxReg < SVGA_SCRATCH_BASE + pThis->svga.cScratchRegion, ("idxReg=%#x\n", idxReg),
                                 STAM_REL_COUNTER_INC(&pThis->svga.StatRegUnknownWr),
                                 VINF_SUCCESS);
#endif
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* We must adjust the register number if we're in SVGA_ID_0 mode because the PALETTE range moved. */
    if (   idxReg >= SVGA_REG_CAPABILITIES
        && pThis->svga.u32SVGAId == SVGA_ID_0)
    {
        idxReg += SVGA_PALETTE_BASE - SVGA_REG_CAPABILITIES;
        Log(("vmsvgaWritePort: SVGA_ID_0 reg adj %#x -> %#x\n", pThis->svga.u32IndexReg, idxReg));
    }
    Log(("vmsvgaWritePort index=%s (%d) val=%#x\n", vmsvgaIndexToString(pThis, idxReg), idxReg, u32));
    switch (idxReg)
    {
    case SVGA_REG_ID:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegIdWr);
        if (   u32 == SVGA_ID_0
            || u32 == SVGA_ID_1
            || u32 == SVGA_ID_2)
            pThis->svga.u32SVGAId = u32;
        else
            PDMDevHlpDBGFStop(pThis->CTX_SUFF(pDevIns), RT_SRC_POS, "Trying to set SVGA_REG_ID to %#x (%d)\n", u32, u32);
        break;

    case SVGA_REG_ENABLE:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegEnableWr);
        if (    pThis->svga.fEnabled    == u32
            &&  pThis->last_bpp         == (unsigned)pThis->svga.uBpp
            &&  pThis->last_scr_width   == (unsigned)pThis->svga.uWidth
            &&  pThis->last_scr_height  == (unsigned)pThis->svga.uHeight
            &&  pThis->last_width       == (unsigned)pThis->svga.uWidth
            &&  pThis->last_height      == (unsigned)pThis->svga.uHeight
            )
            /* Nothing to do. */
            break;

#ifdef IN_RING3
        if (    u32 == 1
            &&  pThis->svga.fEnabled == false)
        {
            /* Make a backup copy of the first 512kb in order to save font data etc. */
            /** @todo should probably swap here, rather than copy + zero */
            memcpy(pThis->svga.pbVgaFrameBufferR3, pThis->vram_ptrR3, VMSVGA_VGA_FB_BACKUP_SIZE);
            memset(pThis->vram_ptrR3, 0, VMSVGA_VGA_FB_BACKUP_SIZE);
        }

        pThis->svga.fEnabled = u32;
        if (pThis->svga.fEnabled)
        {
            if (    pThis->svga.uWidth  == VMSVGA_VAL_UNINITIALIZED
                &&  pThis->svga.uHeight == VMSVGA_VAL_UNINITIALIZED
                &&  pThis->svga.uBpp    == VMSVGA_VAL_UNINITIALIZED)
            {
                /* Keep the current mode. */
                pThis->svga.uWidth  = pThis->pDrv->cx;
                pThis->svga.uHeight = pThis->pDrv->cy;
                pThis->svga.uBpp    = (pThis->pDrv->cBits + 7) & ~7;
            }

            if (    pThis->svga.uWidth  != VMSVGA_VAL_UNINITIALIZED
                &&  pThis->svga.uHeight != VMSVGA_VAL_UNINITIALIZED
                &&  pThis->svga.uBpp    != VMSVGA_VAL_UNINITIALIZED)
            {
                rc = vmsvgaChangeMode(pThis);
                AssertRCReturn(rc, rc);
            }
# ifdef LOG_ENABLED
            Log(("configured=%d busy=%d\n", pThis->svga.fConfigured, pThis->svga.pFIFOR3[SVGA_FIFO_BUSY]));
            uint32_t *pFIFO = pThis->svga.pFIFOR3;
            Log(("next %x stop %x\n", pFIFO[SVGA_FIFO_NEXT_CMD], pFIFO[SVGA_FIFO_STOP]));
# endif

            /* Disable or enable dirty page tracking according to the current fTraces value. */
            vmsvgaSetTraces(pThis, !!pThis->svga.fTraces);
        }
        else
        {
            /* Restore the text mode backup. */
            memcpy(pThis->vram_ptrR3, pThis->svga.pbVgaFrameBufferR3, VMSVGA_VGA_FB_BACKUP_SIZE);

/*            pThis->svga.uHeight    = -1;
            pThis->svga.uWidth     = -1;
            pThis->svga.uBpp       = -1;
            pThis->svga.cbScanline = 0; */
            pThis->pDrv->pfnLFBModeChange(pThis->pDrv, false);

            /* Enable dirty page tracking again when going into legacy mode. */
            vmsvgaSetTraces(pThis, true);
        }
#else  /* !IN_RING3 */
        rc = VINF_IOM_R3_IOPORT_WRITE;
#endif /* !IN_RING3 */
        break;

    case SVGA_REG_WIDTH:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegWidthWr);
        if (pThis->svga.uWidth != u32)
        {
            if (pThis->svga.fEnabled)
            {
#ifdef IN_RING3
                pThis->svga.uWidth = u32;
                rc = vmsvgaChangeMode(pThis);
                AssertRCReturn(rc, rc);
#else
                rc = VINF_IOM_R3_IOPORT_WRITE;
#endif
            }
            else
                pThis->svga.uWidth = u32;
        }
        /* else: nop */
        break;

    case SVGA_REG_HEIGHT:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegHeightWr);
        if (pThis->svga.uHeight != u32)
        {
            if (pThis->svga.fEnabled)
            {
#ifdef IN_RING3
                pThis->svga.uHeight = u32;
                rc = vmsvgaChangeMode(pThis);
                AssertRCReturn(rc, rc);
#else
                rc = VINF_IOM_R3_IOPORT_WRITE;
#endif
            }
            else
                pThis->svga.uHeight = u32;
        }
        /* else: nop */
        break;

    case SVGA_REG_DEPTH:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDepthWr);
        /** @todo read-only?? */
        break;

    case SVGA_REG_BITS_PER_PIXEL:      /* Current bpp in the guest */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegBitsPerPixelWr);
        if (pThis->svga.uBpp != u32)
        {
            if (pThis->svga.fEnabled)
            {
#ifdef IN_RING3
                pThis->svga.uBpp = u32;
                rc = vmsvgaChangeMode(pThis);
                AssertRCReturn(rc, rc);
#else
                rc = VINF_IOM_R3_IOPORT_WRITE;
#endif
            }
            else
                pThis->svga.uBpp = u32;
        }
        /* else: nop */
        break;

    case SVGA_REG_PSEUDOCOLOR:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegPseudoColorWr);
        break;

    case SVGA_REG_CONFIG_DONE:         /* Set when memory area configured */
#ifdef IN_RING3
        STAM_REL_COUNTER_INC(&pSVGAState->StatR3RegConfigDoneWr);
        pThis->svga.fConfigured = u32;
        /* Disabling the FIFO enables tracing (dirty page detection) by default. */
        if (!pThis->svga.fConfigured)
        {
            pThis->svga.fTraces = true;
        }
        vmsvgaSetTraces(pThis, !!pThis->svga.fTraces);
#else
        rc = VINF_IOM_R3_IOPORT_WRITE;
#endif
        break;

    case SVGA_REG_SYNC:                /* See "FIFO Synchronization Registers" */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegSyncWr);
        if (    pThis->svga.fEnabled
            &&  pThis->svga.fConfigured)
        {
#if defined(IN_RING3) || defined(IN_RING0)
            Log(("SVGA_REG_SYNC: SVGA_FIFO_BUSY=%d\n", pThis->svga.CTX_SUFF(pFIFO)[SVGA_FIFO_BUSY]));
            ASMAtomicWriteU32(&pThis->svga.fBusy, VMSVGA_BUSY_F_EMT_FORCE | VMSVGA_BUSY_F_FIFO);
            if (VMSVGA_IS_VALID_FIFO_REG(SVGA_FIFO_BUSY, pThis->svga.CTX_SUFF(pFIFO)[SVGA_FIFO_MIN]))
                vmsvgaSafeFifoBusyRegUpdate(pThis, true);

            /* Kick the FIFO thread to start processing commands again. */
            SUPSemEventSignal(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem);
#else
            rc = VINF_IOM_R3_IOPORT_WRITE;
#endif
        }
        /* else nothing to do. */
        else
            Log(("Sync ignored enabled=%d configured=%d\n", pThis->svga.fEnabled, pThis->svga.fConfigured));

        break;

    case SVGA_REG_BUSY:                /* See "FIFO Synchronization Registers" (read-only) */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegBusyWr);
        break;

    case SVGA_REG_GUEST_ID:            /* Set guest OS identifier */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGuestIdWr);
        pThis->svga.u32GuestId = u32;
        break;

    case SVGA_REG_PITCHLOCK:           /* Fixed pitch for all modes */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegPitchLockWr);
        pThis->svga.u32PitchLock = u32;
        break;

    case SVGA_REG_IRQMASK:             /* Interrupt mask */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegIrqMaskWr);
        pThis->svga.u32IrqMask = u32;

        /* Irq pending after the above change? */
        if (pThis->svga.u32IrqStatus & u32)
        {
            Log(("SVGA_REG_IRQMASK: Trigger interrupt with status %x\n", pThis->svga.u32IrqStatus));
            PDMDevHlpPCISetIrqNoWait(pThis->CTX_SUFF(pDevIns), 0, 1);
        }
        else
            PDMDevHlpPCISetIrqNoWait(pThis->CTX_SUFF(pDevIns), 0, 0);
        break;

    /* Mouse cursor support */
    case SVGA_REG_CURSOR_ID:
    case SVGA_REG_CURSOR_X:
    case SVGA_REG_CURSOR_Y:
    case SVGA_REG_CURSOR_ON:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegCursorXxxxWr);
        break;

    /* Legacy multi-monitor support */
    case SVGA_REG_NUM_GUEST_DISPLAYS:/* Number of guest displays in X/Y direction */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegNumGuestDisplaysWr);
        break;
    case SVGA_REG_DISPLAY_ID:        /* Display ID for the following display attributes */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayIdWr);
        break;
    case SVGA_REG_DISPLAY_IS_PRIMARY:/* Whether this is a primary display */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayIsPrimaryWr);
        break;
    case SVGA_REG_DISPLAY_POSITION_X:/* The display position x */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayPositionXWr);
        break;
    case SVGA_REG_DISPLAY_POSITION_Y:/* The display position y */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayPositionYWr);
        break;
    case SVGA_REG_DISPLAY_WIDTH:     /* The display's width */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayWidthWr);
        break;
    case SVGA_REG_DISPLAY_HEIGHT:    /* The display's height */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegDisplayHeightWr);
        break;
#ifdef VBOX_WITH_VMSVGA3D
    /* See "Guest memory regions" below. */
    case SVGA_REG_GMR_ID:
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegGmrIdWr);
        pThis->svga.u32CurrentGMRId = u32;
        break;

    case SVGA_REG_GMR_DESCRIPTOR:
# ifndef IN_RING3
        rc = VINF_IOM_R3_IOPORT_WRITE;
        break;
# else /* IN_RING3 */
    {
        STAM_REL_COUNTER_INC(&pSVGAState->StatR3RegGmrDescriptorWr);

        /* Validate current GMR id. */
        uint32_t idGMR = pThis->svga.u32CurrentGMRId;
        AssertBreak(idGMR < pThis->svga.cGMR);
        RT_UNTRUSTED_VALIDATED_FENCE();

        /* Free the old GMR if present. */
        vmsvgaGMRFree(pThis, idGMR);

        /* Just undefine the GMR? */
        RTGCPHYS GCPhys = (RTGCPHYS)u32 << PAGE_SHIFT;
        if (GCPhys == 0)
        {
            STAM_REL_COUNTER_INC(&pSVGAState->StatR3RegGmrDescriptorWrFree);
            break;
        }


        /* Never cross a page boundary automatically. */
        const uint32_t          cMaxPages   = RT_MIN(VMSVGA_MAX_GMR_PAGES, UINT32_MAX / X86_PAGE_SIZE);
        uint32_t                cPagesTotal = 0;
        uint32_t                iDesc       = 0;
        PVMSVGAGMRDESCRIPTOR    paDescs     = NULL;
        uint32_t                cLoops      = 0;
        RTGCPHYS                GCPhysBase  = GCPhys;
        while (PHYS_PAGE_ADDRESS(GCPhys) == PHYS_PAGE_ADDRESS(GCPhysBase))
        {
            /* Read descriptor. */
            SVGAGuestMemDescriptor desc;
            rc = PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), GCPhys, &desc, sizeof(desc));
            AssertRCBreak(rc);

            if (desc.numPages != 0)
            {
                AssertBreakStmt(desc.numPages <= cMaxPages, rc = VERR_OUT_OF_RANGE);
                cPagesTotal += desc.numPages;
                AssertBreakStmt(cPagesTotal   <= cMaxPages, rc = VERR_OUT_OF_RANGE);

                if ((iDesc & 15) == 0)
                {
                    void *pvNew = RTMemRealloc(paDescs, (iDesc + 16) * sizeof(VMSVGAGMRDESCRIPTOR));
                    AssertBreakStmt(pvNew, rc = VERR_NO_MEMORY);
                    paDescs = (PVMSVGAGMRDESCRIPTOR)pvNew;
                }

                paDescs[iDesc].GCPhys     = (RTGCPHYS)desc.ppn << PAGE_SHIFT;
                paDescs[iDesc++].numPages = desc.numPages;

                /* Continue with the next descriptor. */
                GCPhys += sizeof(desc);
            }
            else if (desc.ppn == 0)
                break;  /* terminator */
            else /* Pointer to the next physical page of descriptors. */
                GCPhys = GCPhysBase = (RTGCPHYS)desc.ppn << PAGE_SHIFT;

            cLoops++;
            AssertBreakStmt(cLoops < VMSVGA_MAX_GMR_DESC_LOOP_COUNT, rc = VERR_OUT_OF_RANGE);
        }

        AssertStmt(iDesc > 0 || RT_FAILURE_NP(rc), rc = VERR_OUT_OF_RANGE);
        if (RT_SUCCESS(rc))
        {
            /* Commit the GMR. */
            pSVGAState->paGMR[idGMR].paDesc         = paDescs;
            pSVGAState->paGMR[idGMR].numDescriptors = iDesc;
            pSVGAState->paGMR[idGMR].cMaxPages      = cPagesTotal;
            pSVGAState->paGMR[idGMR].cbTotal        = cPagesTotal * PAGE_SIZE;
            Assert((pSVGAState->paGMR[idGMR].cbTotal >> PAGE_SHIFT) == cPagesTotal);
            Log(("Defined new gmr %x numDescriptors=%d cbTotal=%x (%#x pages)\n",
                 idGMR, iDesc, pSVGAState->paGMR[idGMR].cbTotal, cPagesTotal));
        }
        else
        {
            RTMemFree(paDescs);
            STAM_REL_COUNTER_INC(&pSVGAState->StatR3RegGmrDescriptorWrErrors);
        }
        break;
    }
# endif /* IN_RING3 */
#endif // VBOX_WITH_VMSVGA3D

    case SVGA_REG_TRACES:            /* Enable trace-based updates even when FIFO is on */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegTracesWr);
        if (pThis->svga.fTraces == u32)
            break; /* nothing to do */

#ifdef IN_RING3
        vmsvgaSetTraces(pThis, !!u32);
#else
        rc = VINF_IOM_R3_IOPORT_WRITE;
#endif
        break;

    case SVGA_REG_TOP:               /* Must be 1 more than the last register */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegTopWr);
        break;

    case SVGA_REG_NUM_DISPLAYS:        /* (Deprecated) */
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegNumDisplaysWr);
        Log(("Write to deprecated register %x - val %x ignored\n", idxReg, u32));
        break;

    case SVGA_REG_FB_START:
    case SVGA_REG_MEM_START:
    case SVGA_REG_HOST_BITS_PER_PIXEL:
    case SVGA_REG_MAX_WIDTH:
    case SVGA_REG_MAX_HEIGHT:
    case SVGA_REG_VRAM_SIZE:
    case SVGA_REG_FB_SIZE:
    case SVGA_REG_CAPABILITIES:
    case SVGA_REG_MEM_SIZE:
    case SVGA_REG_SCRATCH_SIZE:        /* Number of scratch registers */
    case SVGA_REG_MEM_REGS:            /* Number of FIFO registers */
    case SVGA_REG_BYTES_PER_LINE:
    case SVGA_REG_FB_OFFSET:
    case SVGA_REG_RED_MASK:
    case SVGA_REG_GREEN_MASK:
    case SVGA_REG_BLUE_MASK:
    case SVGA_REG_GMRS_MAX_PAGES:    /* Maximum number of 4KB pages for all GMRs */
    case SVGA_REG_MEMORY_SIZE:       /* Total dedicated device memory excluding FIFO */
    case SVGA_REG_GMR_MAX_IDS:
    case SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH:
        /* Read only - ignore. */
        Log(("Write to R/O register %x - val %x ignored\n", idxReg, u32));
        STAM_REL_COUNTER_INC(&pThis->svga.StatRegReadOnlyWr);
        break;

    default:
    {
        uint32_t offReg;
        if ((offReg = idxReg - SVGA_SCRATCH_BASE) < pThis->svga.cScratchRegion)
        {
            RT_UNTRUSTED_VALIDATED_FENCE();
            pThis->svga.au32ScratchRegion[offReg] = u32;
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegScratchWr);
        }
        else if ((offReg = idxReg - SVGA_PALETTE_BASE) < (uint32_t)SVGA_NUM_PALETTE_REGS)
        {
            /* Note! Using last_palette rather than palette here to preserve the VGA one.
                     Btw, see rgb_to_pixel32.  */
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegPaletteWr);
            u32 &= 0xff;
            RT_UNTRUSTED_VALIDATED_FENCE();
            uint32_t uRgb = pThis->last_palette[offReg / 3];
            switch (offReg % 3)
            {
                case 0: uRgb = (uRgb & UINT32_C(0x0000ffff)) | (u32 << 16); break; /* red */
                case 1: uRgb = (uRgb & UINT32_C(0x00ff00ff)) | (u32 <<  8); break; /* green */
                case 2: uRgb = (uRgb & UINT32_C(0x00ffff00)) |  u32       ; break; /* blue */
            }
            pThis->last_palette[offReg / 3] = uRgb;
        }
        else
        {
#if !defined(IN_RING3) && defined(VBOX_STRICT)
            rc = VINF_IOM_R3_IOPORT_WRITE;
#else
            STAM_REL_COUNTER_INC(&pThis->svga.StatRegUnknownWr);
            AssertMsgFailed(("reg=%#x u32=%#x\n", idxReg, u32));
#endif
        }
        break;
    }
    }
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
 * @param   pu32        Where to store the result.  This is always a 32-bit
 *                      variable regardless of what @a cb might say.
 * @param   cb          Number of bytes read.
 */
PDMBOTHCBDECL(int) vmsvgaIORead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb)
{
    PVGASTATE   pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    RT_NOREF_PV(pvUser);

    /* Ignore non-dword accesses. */
    if (cb != 4)
    {
        Log(("Ignoring non-dword read at %x cb=%d\n", uPort, cb));
        *pu32 = UINT32_MAX;
        return VINF_SUCCESS;
    }

    switch (uPort - pThis->svga.BasePort)
    {
    case SVGA_INDEX_PORT:
        *pu32 = pThis->svga.u32IndexReg;
        break;

    case SVGA_VALUE_PORT:
        return vmsvgaReadPort(pThis, pu32);

    case SVGA_BIOS_PORT:
        Log(("Ignoring BIOS port read\n"));
        *pu32 = 0;
        break;

    case SVGA_IRQSTATUS_PORT:
        LogFlow(("vmsvgaIORead: SVGA_IRQSTATUS_PORT %x\n", pThis->svga.u32IrqStatus));
        *pu32 = pThis->svga.u32IrqStatus;
        break;

    default:
        ASSERT_GUEST_MSG_FAILED(("vmsvgaIORead: Unknown register %u (%#x) was read from.\n", uPort - pThis->svga.BasePort, uPort));
        *pu32 = UINT32_MAX;
        break;
    }

    return VINF_SUCCESS;
}

/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 */
PDMBOTHCBDECL(int) vmsvgaIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb)
{
    PVGASTATE   pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    RT_NOREF_PV(pvUser);

    /* Ignore non-dword accesses. */
    if (cb != 4)
    {
        Log(("Ignoring non-dword write at %x val=%x cb=%d\n", uPort, u32, cb));
        return VINF_SUCCESS;
    }

    switch (uPort - pThis->svga.BasePort)
    {
    case SVGA_INDEX_PORT:
        pThis->svga.u32IndexReg = u32;
        break;

    case SVGA_VALUE_PORT:
        return vmsvgaWritePort(pThis, u32);

    case SVGA_BIOS_PORT:
        Log(("Ignoring BIOS port write (val=%x)\n", u32));
        break;

    case SVGA_IRQSTATUS_PORT:
        Log(("vmsvgaIOWrite SVGA_IRQSTATUS_PORT %x: status %x -> %x\n", u32, pThis->svga.u32IrqStatus, pThis->svga.u32IrqStatus & ~u32));
        ASMAtomicAndU32(&pThis->svga.u32IrqStatus, ~u32);
        /* Clear the irq in case all events have been cleared. */
        if (!(pThis->svga.u32IrqStatus & pThis->svga.u32IrqMask))
        {
            Log(("vmsvgaIOWrite SVGA_IRQSTATUS_PORT: clearing IRQ\n"));
            PDMDevHlpPCISetIrqNoWait(pDevIns, 0, 0);
        }
        break;

    default:
        ASSERT_GUEST_MSG_FAILED(("vmsvgaIOWrite: Unknown register %u (%#x) was written to, value %#x LB %u.\n",
                                 uPort - pThis->svga.BasePort, uPort, u32, cb));
        break;
    }
    return VINF_SUCCESS;
}

#ifdef DEBUG_FIFO_ACCESS

# ifdef IN_RING3
/**
 * Handle LFB access.
 * @returns VBox status code.
 * @param   pVM             VM handle.
 * @param   pThis           VGA device instance data.
 * @param   GCPhys          The access physical address.
 * @param   fWriteAccess    Read or write access
 */
static int vmsvgaFIFOAccess(PVM pVM, PVGASTATE pThis, RTGCPHYS GCPhys, bool fWriteAccess)
{
    RT_NOREF(pVM);
    RTGCPHYS GCPhysOffset = GCPhys - pThis->svga.GCPhysFIFO;
    uint32_t *pFIFO = pThis->svga.pFIFOR3;

    switch (GCPhysOffset >> 2)
    {
    case SVGA_FIFO_MIN:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_MIN = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_MAX:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_MAX = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_NEXT_CMD:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_NEXT_CMD = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_STOP:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_STOP = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CAPABILITIES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CAPABILITIES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_FLAGS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_FLAGS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_FENCE:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_FENCE = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_HWVERSION:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_HWVERSION = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_PITCHLOCK:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_PITCHLOCK = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CURSOR_ON:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CURSOR_ON = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CURSOR_X:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CURSOR_X = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CURSOR_Y:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CURSOR_Y = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CURSOR_COUNT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CURSOR_COUNT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CURSOR_LAST_UPDATED:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CURSOR_LAST_UPDATED = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_RESERVED:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_RESERVED = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_CURSOR_SCREEN_ID:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_CURSOR_SCREEN_ID = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_DEAD:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_DEAD = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_HWVERSION_REVISED:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_HWVERSION_REVISED = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_3D:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_3D = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_LIGHTS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_LIGHTS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_TEXTURES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_TEXTURES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_CLIP_PLANES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_CLIP_PLANES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_VERTEX_SHADER_VERSION:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_VERTEX_SHADER_VERSION = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_VERTEX_SHADER:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_VERTEX_SHADER = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_FRAGMENT_SHADER:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_FRAGMENT_SHADER = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_RENDER_TARGETS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_RENDER_TARGETS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_S23E8_TEXTURES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_S23E8_TEXTURES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_S10E5_TEXTURES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_S10E5_TEXTURES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_D16_BUFFER_FORMAT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_D16_BUFFER_FORMAT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_QUERY_TYPES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_QUERY_TYPES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_POINT_SIZE:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_POINT_SIZE = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_SHADER_TEXTURES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_SHADER_TEXTURES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_VOLUME_EXTENT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_VOLUME_EXTENT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_VERTEX_INDEX:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_VERTEX_INDEX = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_TEXTURE_OPS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_TEXTURE_OPS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_R5G6B5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_ALPHA8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_ALPHA8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Z_D16:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Z_D16 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_DXT1:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_DXT1 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_DXT2:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_DXT2 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_DXT3:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_DXT3 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_DXT4:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_DXT4 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_DXT5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_DXT5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_CxV8U8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_CxV8U8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_R_S10E5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_R_S10E5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_R_S23E8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_R_S23E8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_V16U16:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_V16U16 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_G16R16:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_G16R16 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_UYVY:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_UYVY = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_YUY2:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_YUY2 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_ALPHATOCOVERAGE:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_ALPHATOCOVERAGE = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SUPERSAMPLE:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SUPERSAMPLE = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_AUTOGENMIPMAPS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_AUTOGENMIPMAPS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_NV12:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_NV12 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_AYUV:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_AYUV = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_CONTEXT_IDS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_CONTEXT_IDS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_MAX_SURFACE_IDS:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_MAX_SURFACE_IDS = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Z_DF16:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Z_DF16 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Z_DF24:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Z_DF24 = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS + SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_3D_CAPS_LAST:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_3D_CAPS_LAST = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_GUEST_3D_HWVERSION:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_GUEST_3D_HWVERSION = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_FENCE_GOAL:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_FENCE_GOAL = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    case SVGA_FIFO_BUSY:
        Log(("vmsvgaFIFOAccess [0x%x]: %s SVGA_FIFO_BUSY = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", pFIFO[GCPhysOffset >> 2]));
        break;
    default:
        Log(("vmsvgaFIFOAccess [0x%x]: %s access at offset %x = %x\n", GCPhysOffset >> 2, (fWriteAccess) ? "WRITE" : "READ", GCPhysOffset, pFIFO[GCPhysOffset >> 2]));
        break;
    }

    return VINF_EM_RAW_EMULATE_INSTR;
}

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
static DECLCALLBACK(VBOXSTRICTRC)
vmsvgaR3FIFOAccessHandler(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void *pvPhys, void *pvBuf, size_t cbBuf,
                          PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    PVGASTATE   pThis = (PVGASTATE)pvUser;
    int         rc;
    Assert(pThis);
    Assert(GCPhys >= pThis->svga.GCPhysFIFO);
    NOREF(pVCpu); NOREF(pvPhys); NOREF(pvBuf); NOREF(cbBuf); NOREF(enmOrigin);

    rc = vmsvgaFIFOAccess(pVM, pThis, GCPhys, enmAccessType == PGMACCESSTYPE_WRITE);
    if (RT_SUCCESS(rc))
        return VINF_PGM_HANDLER_DO_DEFAULT;
    AssertMsg(rc <= VINF_SUCCESS, ("rc=%Rrc\n", rc));
    return rc;
}

# endif /* IN_RING3 */
#endif /* DEBUG_FIFO_ACCESS */

#ifdef DEBUG_GMR_ACCESS
# ifdef IN_RING3

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
static DECLCALLBACK(VBOXSTRICTRC)
vmsvgaR3GMRAccessHandler(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, void *pvPhys, void *pvBuf, size_t cbBuf,
                         PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    PVGASTATE   pThis = (PVGASTATE)pvUser;
    Assert(pThis);
    PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;
    NOREF(pVCpu); NOREF(pvPhys); NOREF(pvBuf); NOREF(cbBuf); NOREF(enmAccessType); NOREF(enmOrigin);

    Log(("vmsvgaR3GMRAccessHandler: GMR access to page %RGp\n", GCPhys));

    for (uint32_t i = 0; i < pThis->svga.cGMR; ++i)
    {
        PGMR pGMR = &pSVGAState->paGMR[i];

        if (pGMR->numDescriptors)
        {
            for (uint32_t j = 0; j < pGMR->numDescriptors; j++)
            {
                if (    GCPhys >= pGMR->paDesc[j].GCPhys
                    &&  GCPhys < pGMR->paDesc[j].GCPhys + pGMR->paDesc[j].numPages * PAGE_SIZE)
                {
                    /*
                     * Turn off the write handler for this particular page and make it R/W.
                     * Then return telling the caller to restart the guest instruction.
                     */
                    int rc = PGMHandlerPhysicalPageTempOff(pVM, pGMR->paDesc[j].GCPhys, GCPhys);
                    AssertRC(rc);
                    goto end;
                }
            }
        }
    }
end:
    return VINF_PGM_HANDLER_DO_DEFAULT;
}

/* Callback handler for VMR3ReqCallWaitU */
static DECLCALLBACK(int) vmsvgaRegisterGMR(PPDMDEVINS pDevIns, uint32_t gmrId)
{
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;
    PGMR pGMR = &pSVGAState->paGMR[gmrId];
    int rc;

    for (uint32_t i = 0; i < pGMR->numDescriptors; i++)
    {
        rc = PGMHandlerPhysicalRegister(PDMDevHlpGetVM(pThis->pDevInsR3),
                                        pGMR->paDesc[i].GCPhys, pGMR->paDesc[i].GCPhys + pGMR->paDesc[i].numPages * PAGE_SIZE - 1,
                                        pThis->svga.hGmrAccessHandlerType, pThis, NIL_RTR0PTR, NIL_RTRCPTR, "VMSVGA GMR");
        AssertRC(rc);
    }
    return VINF_SUCCESS;
}

/* Callback handler for VMR3ReqCallWaitU */
static DECLCALLBACK(int) vmsvgaDeregisterGMR(PPDMDEVINS pDevIns, uint32_t gmrId)
{
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;
    PGMR pGMR = &pSVGAState->paGMR[gmrId];

    for (uint32_t i = 0; i < pGMR->numDescriptors; i++)
    {
        int rc = PGMHandlerPhysicalDeregister(PDMDevHlpGetVM(pThis->pDevInsR3), pGMR->paDesc[i].GCPhys);
        AssertRC(rc);
    }
    return VINF_SUCCESS;
}

/* Callback handler for VMR3ReqCallWaitU */
static DECLCALLBACK(int) vmsvgaResetGMRHandlers(PVGASTATE pThis)
{
    PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;

    for (uint32_t i = 0; i < pThis->svga.cGMR; ++i)
    {
        PGMR pGMR = &pSVGAState->paGMR[i];

        if (pGMR->numDescriptors)
        {
            for (uint32_t j = 0; j < pGMR->numDescriptors; j++)
            {
                int rc = PGMHandlerPhysicalReset(PDMDevHlpGetVM(pThis->pDevInsR3), pGMR->paDesc[j].GCPhys);
                AssertRC(rc);
            }
        }
    }
    return VINF_SUCCESS;
}

# endif /* IN_RING3 */
#endif /* DEBUG_GMR_ACCESS */

/* -=-=-=-=-=- Ring 3 -=-=-=-=-=- */

#ifdef IN_RING3


/**
 * Common worker for changing the pointer shape.
 *
 * @param   pThis               The VGA instance data.
 * @param   pSVGAState          The VMSVGA ring-3 instance data.
 * @param   fAlpha              Whether there is alpha or not.
 * @param   xHot                Hotspot x coordinate.
 * @param   yHot                Hotspot y coordinate.
 * @param   cx                  Width.
 * @param   cy                  Height.
 * @param   pbData              Heap copy of the cursor data.  Consumed.
 * @param   cbData              The size of the data.
 */
static void vmsvgaR3InstallNewCursor(PVGASTATE pThis, PVMSVGAR3STATE pSVGAState, bool fAlpha,
                                     uint32_t xHot, uint32_t yHot, uint32_t cx, uint32_t cy, uint8_t *pbData, uint32_t cbData)
{
    Log(("vmsvgaR3InstallNewCursor: cx=%d cy=%d xHot=%d yHot=%d fAlpha=%d cbData=%#x\n", cx, cy, xHot, yHot, fAlpha, cbData));
#ifdef LOG_ENABLED
    if (LogIs2Enabled())
    {
        uint32_t cbAndLine = RT_ALIGN(cx, 8) / 8;
        if (!fAlpha)
        {
            Log2(("VMSVGA Cursor AND mask (%d,%d):\n", cx, cy));
            for (uint32_t y = 0; y < cy; y++)
            {
                Log2(("%3u:", y));
                uint8_t const *pbLine = &pbData[y * cbAndLine];
                for (uint32_t x = 0; x < cx; x += 8)
                {
                    uint8_t   b = pbLine[x / 8];
                    char      szByte[12];
                    szByte[0] = b & 0x80 ? '*' : ' '; /* most significant bit first */
                    szByte[1] = b & 0x40 ? '*' : ' ';
                    szByte[2] = b & 0x20 ? '*' : ' ';
                    szByte[3] = b & 0x10 ? '*' : ' ';
                    szByte[4] = b & 0x08 ? '*' : ' ';
                    szByte[5] = b & 0x04 ? '*' : ' ';
                    szByte[6] = b & 0x02 ? '*' : ' ';
                    szByte[7] = b & 0x01 ? '*' : ' ';
                    szByte[8] = '\0';
                    Log2(("%s", szByte));
                }
                Log2(("\n"));
            }
        }

        Log2(("VMSVGA Cursor XOR mask (%d,%d):\n", cx, cy));
        uint32_t const *pu32Xor = (uint32_t const *)&pbData[RT_ALIGN_32(cbAndLine * cy, 4)];
        for (uint32_t y = 0; y < cy; y++)
        {
            Log2(("%3u:", y));
            uint32_t const *pu32Line = &pu32Xor[y * cx];
            for (uint32_t x = 0; x < cx; x++)
                Log2((" %08x", pu32Line[x]));
            Log2(("\n"));
        }
    }
#endif

    int rc = pThis->pDrv->pfnVBVAMousePointerShape(pThis->pDrv, true /*fVisible*/, fAlpha, xHot, yHot, cx, cy, pbData);
    AssertRC(rc);

    if (pSVGAState->Cursor.fActive)
        RTMemFree(pSVGAState->Cursor.pData);

    pSVGAState->Cursor.fActive  = true;
    pSVGAState->Cursor.xHotspot = xHot;
    pSVGAState->Cursor.yHotspot = yHot;
    pSVGAState->Cursor.width    = cx;
    pSVGAState->Cursor.height   = cy;
    pSVGAState->Cursor.cbData   = cbData;
    pSVGAState->Cursor.pData    = pbData;
}


/**
 * Handles the SVGA_CMD_DEFINE_CURSOR command.
 *
 * @param   pThis               The VGA instance data.
 * @param   pSVGAState          The VMSVGA ring-3 instance data.
 * @param   pCursor             The cursor.
 * @param   pbSrcAndMask        The AND mask.
 * @param   cbSrcAndLine        The scanline length of the AND mask.
 * @param   pbSrcXorMask        The XOR mask.
 * @param   cbSrcXorLine        The scanline length of the XOR mask.
 */
static void vmsvgaR3CmdDefineCursor(PVGASTATE pThis, PVMSVGAR3STATE pSVGAState, SVGAFifoCmdDefineCursor const *pCursor,
                                    uint8_t const *pbSrcAndMask, uint32_t cbSrcAndLine,
                                    uint8_t const *pbSrcXorMask, uint32_t cbSrcXorLine)
{
    uint32_t const cx = pCursor->width;
    uint32_t const cy = pCursor->height;

    /*
     * Convert the input to 1-bit AND mask and a 32-bit BRGA XOR mask.
     * The AND data uses 8-bit aligned scanlines.
     * The XOR data must be starting on a 32-bit boundrary.
     */
    uint32_t cbDstAndLine = RT_ALIGN_32(cx, 8) / 8;
    uint32_t cbDstAndMask = cbDstAndLine          * cy;
    uint32_t cbDstXorMask = cx * sizeof(uint32_t) * cy;
    uint32_t cbCopy = RT_ALIGN_32(cbDstAndMask, 4) + cbDstXorMask;

    uint8_t *pbCopy = (uint8_t *)RTMemAlloc(cbCopy);
    AssertReturnVoid(pbCopy);

    /* Convert the AND mask. */
    uint8_t       *pbDst     = pbCopy;
    uint8_t const *pbSrc     = pbSrcAndMask;
    switch (pCursor->andMaskDepth)
    {
        case 1:
            if (cbSrcAndLine == cbDstAndLine)
                memcpy(pbDst, pbSrc, cbSrcAndLine * cy);
            else
            {
                Assert(cbSrcAndLine > cbDstAndLine); /* lines are dword alined in source, but only byte in destination. */
                for (uint32_t y = 0; y < cy; y++)
                {
                    memcpy(pbDst, pbSrc, cbDstAndLine);
                    pbDst += cbDstAndLine;
                    pbSrc += cbSrcAndLine;
                }
            }
            break;
        /* Should take the XOR mask into account for the multi-bit AND mask. */
        case 8:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 1;
                    do
                    {
                        uintptr_t const idxPal = pbSrc[x] * 3;
                        if (((   pThis->last_palette[idxPal]
                              | (pThis->last_palette[idxPal] >>  8)
                              | (pThis->last_palette[idxPal] >> 16)) & 0xff) > 0xfc)
                            bDst |= fBit;
                        fBit <<= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 15:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 1;
                    do
                    {
                        if ((pbSrc[x * 2] | (pbSrc[x * 2 + 1] & 0x7f)) >= 0xfc)
                            bDst |= fBit;
                        fBit <<= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 16:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 1;
                    do
                    {
                        if ((pbSrc[x * 2] | pbSrc[x * 2 + 1]) >= 0xfc)
                            bDst |= fBit;
                        fBit <<= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 24:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 1;
                    do
                    {
                        if ((pbSrc[x * 3] | pbSrc[x * 3 + 1] | pbSrc[x * 3 + 2]) >= 0xfc)
                            bDst |= fBit;
                        fBit <<= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 32:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 1;
                    do
                    {
                        if ((pbSrc[x * 4] | pbSrc[x * 4 + 1] | pbSrc[x * 4 + 2] | pbSrc[x * 4 + 3]) >= 0xfc)
                            bDst |= fBit;
                        fBit <<= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        default:
            RTMemFree(pbCopy);
            AssertFailedReturnVoid();
    }

    /* Convert the XOR mask. */
    uint32_t *pu32Dst = (uint32_t *)(pbCopy + cbDstAndMask);
    pbSrc  = pbSrcXorMask;
    switch (pCursor->xorMaskDepth)
    {
        case 1:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    /* most significant bit is the left most one. */
                    uint8_t bSrc = pbSrc[x / 8];
                    do
                    {
                        *pu32Dst++ = bSrc & 0x80 ? UINT32_C(0x00ffffff) : 0;
                        bSrc <<= 1;
                        x++;
                    } while ((x & 7) && x < cx);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 8:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                {
                    uint32_t u = pThis->last_palette[pbSrc[x]];
                    *pu32Dst++ = u;//RT_MAKE_U32_FROM_U8(RT_BYTE1(u), RT_BYTE2(u), RT_BYTE3(u), 0);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 15: /* Src: RGB-5-5-5 */
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                {
                    uint32_t const uValue = RT_MAKE_U16(pbSrc[x * 2], pbSrc[x * 2 + 1]);
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(( uValue        & 0x1f) << 3,
                                                     ((uValue >>  5) & 0x1f) << 3,
                                                     ((uValue >> 10) & 0x1f) << 3, 0);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 16: /* Src: RGB-5-6-5 */
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                {
                    uint32_t const uValue = RT_MAKE_U16(pbSrc[x * 2], pbSrc[x * 2 + 1]);
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(( uValue        & 0x1f) << 3,
                                                     ((uValue >>  5) & 0x3f) << 2,
                                                     ((uValue >> 11) & 0x1f) << 3, 0);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 24:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(pbSrc[x*3], pbSrc[x*3 + 1], pbSrc[x*3 + 2], 0);
                pbSrc += cbSrcXorLine;
            }
            break;
        case 32:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(pbSrc[x*4], pbSrc[x*4 + 1], pbSrc[x*4 + 2], 0);
                pbSrc += cbSrcXorLine;
            }
            break;
        default:
            RTMemFree(pbCopy);
            AssertFailedReturnVoid();
    }

    /*
     * Pass it to the frontend/whatever.
     */
    vmsvgaR3InstallNewCursor(pThis, pSVGAState, false /*fAlpha*/, pCursor->hotspotX, pCursor->hotspotY, cx, cy, pbCopy, cbCopy);
}


/**
 * Worker for vmsvgaR3FifoThread that handles an external command.
 *
 * @param   pThis           VGA device instance data.
 */
static void vmsvgaR3FifoHandleExtCmd(PVGASTATE pThis)
{
    uint8_t uExtCmd = pThis->svga.u8FIFOExtCommand;
    switch (pThis->svga.u8FIFOExtCommand)
    {
        case VMSVGA_FIFO_EXTCMD_RESET:
            Log(("vmsvgaFIFOLoop: reset the fifo thread.\n"));
            Assert(pThis->svga.pvFIFOExtCmdParam == NULL);
# ifdef VBOX_WITH_VMSVGA3D
            if (pThis->svga.f3DEnabled)
            {
                /* The 3d subsystem must be reset from the fifo thread. */
                vmsvga3dReset(pThis);
            }
# endif
            break;

        case VMSVGA_FIFO_EXTCMD_TERMINATE:
            Log(("vmsvgaFIFOLoop: terminate the fifo thread.\n"));
            Assert(pThis->svga.pvFIFOExtCmdParam == NULL);
# ifdef VBOX_WITH_VMSVGA3D
            if (pThis->svga.f3DEnabled)
            {
                /* The 3d subsystem must be shut down from the fifo thread. */
                vmsvga3dTerminate(pThis);
            }
# endif
            break;

        case VMSVGA_FIFO_EXTCMD_SAVESTATE:
        {
            Log(("vmsvgaFIFOLoop: VMSVGA_FIFO_EXTCMD_SAVESTATE.\n"));
# ifdef VBOX_WITH_VMSVGA3D
            PSSMHANDLE pSSM = (PSSMHANDLE)pThis->svga.pvFIFOExtCmdParam;
            AssertLogRelMsgBreak(RT_VALID_PTR(pSSM), ("pSSM=%p\n", pSSM));
            vmsvga3dSaveExec(pThis, pSSM);
# endif
            break;
        }

        case VMSVGA_FIFO_EXTCMD_LOADSTATE:
        {
            Log(("vmsvgaFIFOLoop: VMSVGA_FIFO_EXTCMD_LOADSTATE.\n"));
# ifdef VBOX_WITH_VMSVGA3D
            PVMSVGA_STATE_LOAD pLoadState = (PVMSVGA_STATE_LOAD)pThis->svga.pvFIFOExtCmdParam;
            AssertLogRelMsgBreak(RT_VALID_PTR(pLoadState), ("pLoadState=%p\n", pLoadState));
            vmsvga3dLoadExec(pThis, pLoadState->pSSM, pLoadState->uVersion, pLoadState->uPass);
# endif
            break;
        }

        case VMSVGA_FIFO_EXTCMD_UPDATE_SURFACE_HEAP_BUFFERS:
        {
# ifdef VBOX_WITH_VMSVGA3D
            uint32_t sid = (uint32_t)(uintptr_t)pThis->svga.pvFIFOExtCmdParam;
            Log(("vmsvgaFIFOLoop: VMSVGA_FIFO_EXTCMD_UPDATE_SURFACE_HEAP_BUFFERS sid=%#x\n", sid));
            vmsvga3dUpdateHeapBuffersForSurfaces(pThis, sid);
# endif
            break;
        }


        default:
            AssertLogRelMsgFailed(("uExtCmd=%#x pvFIFOExtCmdParam=%p\n", uExtCmd, pThis->svga.pvFIFOExtCmdParam));
            break;
    }

    /*
     * Signal the end of the external command.
     */
    pThis->svga.pvFIFOExtCmdParam = NULL;
    pThis->svga.u8FIFOExtCommand  = VMSVGA_FIFO_EXTCMD_NONE;
    ASMMemoryFence(); /* paranoia^2 */
    int rc = RTSemEventSignal(pThis->svga.FIFOExtCmdSem);
    AssertLogRelRC(rc);
}

/**
 * Worker for vmsvgaR3Destruct, vmsvgaR3Reset, vmsvgaR3Save and vmsvgaR3Load for
 * doing a job on the FIFO thread (even when it's officially suspended).
 *
 * @returns VBox status code (fully asserted).
 * @param   pThis           VGA device instance data.
 * @param   uExtCmd         The command to execute on the FIFO thread.
 * @param   pvParam         Pointer to command parameters.
 * @param   cMsWait         The time to wait for the command, given in
 *                          milliseconds.
 */
static int vmsvgaR3RunExtCmdOnFifoThread(PVGASTATE pThis, uint8_t uExtCmd, void *pvParam, RTMSINTERVAL cMsWait)
{
    Assert(cMsWait >= RT_MS_1SEC * 5);
    AssertLogRelMsg(pThis->svga.u8FIFOExtCommand == VMSVGA_FIFO_EXTCMD_NONE,
                    ("old=%d new=%d\n", pThis->svga.u8FIFOExtCommand, uExtCmd));

    int rc;
    PPDMTHREAD      pThread  = pThis->svga.pFIFOIOThread;
    PDMTHREADSTATE  enmState = pThread->enmState;
    if (enmState == PDMTHREADSTATE_SUSPENDED)
    {
        /*
         * The thread is suspended, we have to temporarily wake it up so it can
         * perform the task.
         * (We ASSUME not racing code here, both wrt thread state and ext commands.)
         */
        Log(("vmsvgaR3RunExtCmdOnFifoThread: uExtCmd=%d enmState=SUSPENDED\n", uExtCmd));
        /* Post the request. */
        pThis->svga.fFifoExtCommandWakeup = true;
        pThis->svga.pvFIFOExtCmdParam     = pvParam;
        pThis->svga.u8FIFOExtCommand      = uExtCmd;
        ASMMemoryFence(); /* paranoia^3 */

        /* Resume the thread. */
        rc = PDMR3ThreadResume(pThread);
        AssertLogRelRC(rc);
        if (RT_SUCCESS(rc))
        {
            /* Wait. Take care in case the semaphore was already posted (same as below). */
            rc = RTSemEventWait(pThis->svga.FIFOExtCmdSem, cMsWait);
            if (   rc == VINF_SUCCESS
                && pThis->svga.u8FIFOExtCommand == uExtCmd)
                rc = RTSemEventWait(pThis->svga.FIFOExtCmdSem, cMsWait);
            AssertLogRelMsg(pThis->svga.u8FIFOExtCommand != uExtCmd || RT_FAILURE_NP(rc),
                            ("%#x %Rrc\n", pThis->svga.u8FIFOExtCommand, rc));

            /* suspend the thread */
            pThis->svga.fFifoExtCommandWakeup = false;
            int rc2 = PDMR3ThreadSuspend(pThread);
            AssertLogRelRC(rc2);
            if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                rc = rc2;
        }
        pThis->svga.fFifoExtCommandWakeup = false;
        pThis->svga.pvFIFOExtCmdParam     = NULL;
    }
    else if (enmState == PDMTHREADSTATE_RUNNING)
    {
        /*
         * The thread is running, should only happen during reset and vmsvga3dsfc.
         * We ASSUME not racing code here, both wrt thread state and ext commands.
         */
        Log(("vmsvgaR3RunExtCmdOnFifoThread: uExtCmd=%d enmState=RUNNING\n", uExtCmd));
        Assert(uExtCmd == VMSVGA_FIFO_EXTCMD_RESET || uExtCmd == VMSVGA_FIFO_EXTCMD_UPDATE_SURFACE_HEAP_BUFFERS);

        /* Post the request. */
        pThis->svga.pvFIFOExtCmdParam = pvParam;
        pThis->svga.u8FIFOExtCommand  = uExtCmd;
        ASMMemoryFence(); /* paranoia^2 */
        rc = SUPSemEventSignal(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem);
        AssertLogRelRC(rc);

        /* Wait. Take care in case the semaphore was already posted (same as above). */
        rc = RTSemEventWait(pThis->svga.FIFOExtCmdSem, cMsWait);
        if (   rc == VINF_SUCCESS
            && pThis->svga.u8FIFOExtCommand == uExtCmd)
            rc = RTSemEventWait(pThis->svga.FIFOExtCmdSem, cMsWait); /* it was already posted, retry the wait. */
        AssertLogRelMsg(pThis->svga.u8FIFOExtCommand != uExtCmd || RT_FAILURE_NP(rc),
                        ("%#x %Rrc\n", pThis->svga.u8FIFOExtCommand, rc));

        pThis->svga.pvFIFOExtCmdParam = NULL;
    }
    else
    {
        /*
         * Something is wrong with the thread!
         */
        AssertLogRelMsgFailed(("uExtCmd=%d enmState=%d\n", uExtCmd, enmState));
        rc = VERR_INVALID_STATE;
    }
    return rc;
}


/**
 * Marks the FIFO non-busy, notifying any waiting EMTs.
 *
 * @param   pThis           The VGA state.
 * @param   pSVGAState      Pointer to the ring-3 only SVGA state data.
 * @param   offFifoMin      The start byte offset of the command FIFO.
 */
static void vmsvgaFifoSetNotBusy(PVGASTATE pThis, PVMSVGAR3STATE pSVGAState, uint32_t offFifoMin)
{
    ASMAtomicAndU32(&pThis->svga.fBusy, ~VMSVGA_BUSY_F_FIFO);
    if (VMSVGA_IS_VALID_FIFO_REG(SVGA_FIFO_BUSY, offFifoMin))
        vmsvgaSafeFifoBusyRegUpdate(pThis, pThis->svga.fBusy != 0);

    /* Wake up any waiting EMTs. */
    if (pSVGAState->cBusyDelayedEmts > 0)
    {
#ifdef VMSVGA_USE_EMT_HALT_CODE
        PVM pVM = PDMDevHlpGetVM(pThis->pDevInsR3);
        VMCPUID idCpu = VMCpuSetFindLastPresentInternal(&pSVGAState->BusyDelayedEmts);
        if (idCpu != NIL_VMCPUID)
        {
            VMR3NotifyCpuDeviceReady(pVM, idCpu);
            while (idCpu-- > 0)
                if (VMCPUSET_IS_PRESENT(&pSVGAState->BusyDelayedEmts, idCpu))
                    VMR3NotifyCpuDeviceReady(pVM, idCpu);
        }
#else
        int rc2 = RTSemEventMultiSignal(pSVGAState->hBusyDelayedEmts);
        AssertRC(rc2);
#endif
    }
}

/**
 * Reads (more) payload into the command buffer.
 *
 * @returns pbBounceBuf on success
 * @retval  (void *)1 if the thread was requested to stop.
 * @retval  NULL on FIFO error.
 *
 * @param   cbPayloadReq    The number of bytes of payload requested.
 * @param   pFIFO           The FIFO.
 * @param   offCurrentCmd   The FIFO byte offset of the current command.
 * @param   offFifoMin      The start byte offset of the command FIFO.
 * @param   offFifoMax      The end byte offset of the command FIFO.
 * @param   pbBounceBuf     The bounch buffer. Same size as the entire FIFO, so
 *                          always sufficient size.
 * @param   pcbAlreadyRead  How much payload we've already read into the bounce
 *                          buffer. (We will NEVER re-read anything.)
 * @param   pThread         The calling PDM thread handle.
 * @param   pThis           The VGA state.
 * @param   pSVGAState      Pointer to the ring-3 only SVGA state data. For
 *                          statistics collection.
 */
static void *vmsvgaFIFOGetCmdPayload(uint32_t cbPayloadReq, uint32_t RT_UNTRUSTED_VOLATILE_GUEST *pFIFO,
                                     uint32_t offCurrentCmd, uint32_t offFifoMin, uint32_t offFifoMax,
                                     uint8_t *pbBounceBuf, uint32_t *pcbAlreadyRead,
                                     PPDMTHREAD pThread, PVGASTATE pThis, PVMSVGAR3STATE pSVGAState)
{
    Assert(pbBounceBuf);
    Assert(pcbAlreadyRead);
    Assert(offFifoMin < offFifoMax);
    Assert(offCurrentCmd >= offFifoMin && offCurrentCmd < offFifoMax);
    Assert(offFifoMax <= pThis->svga.cbFIFO);

    /*
     * Check if the requested payload size has already been satisfied                                                                                                .
     *                                                                                                                                                       .
     * When called to read more, the caller is responsible for making sure the                                                                               .
     * new command size (cbRequsted) never is smaller than what has already                                                                                  .
     * been read.
     */
    uint32_t cbAlreadyRead = *pcbAlreadyRead;
    if (cbPayloadReq <= cbAlreadyRead)
    {
        AssertLogRelReturn(cbPayloadReq == cbAlreadyRead, NULL);
        return pbBounceBuf;
    }

    /*
     * Commands bigger than the fifo buffer are invalid.
     */
    uint32_t const cbFifoCmd = offFifoMax - offFifoMin;
    AssertMsgReturnStmt(cbPayloadReq <= cbFifoCmd, ("cbPayloadReq=%#x cbFifoCmd=%#x\n", cbPayloadReq, cbFifoCmd),
                        STAM_REL_COUNTER_INC(&pSVGAState->StatFifoErrors),
                        NULL);

    /*
     * Move offCurrentCmd past the command dword.
     */
    offCurrentCmd += sizeof(uint32_t);
    if (offCurrentCmd >= offFifoMax)
        offCurrentCmd = offFifoMin;

    /*
     * Do we have sufficient payload data available already?
     * The host should not read beyond [SVGA_FIFO_NEXT_CMD], therefore '>=' in the condition below.
     */
    uint32_t cbAfter, cbBefore;
    uint32_t offNextCmd = pFIFO[SVGA_FIFO_NEXT_CMD];
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    if (offNextCmd >= offCurrentCmd)
    {
        if (RT_LIKELY(offNextCmd < offFifoMax))
            cbAfter = offNextCmd - offCurrentCmd;
        else
        {
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoErrors);
            LogRelMax(16, ("vmsvgaFIFOGetCmdPayload: Invalid offNextCmd=%#x (offFifoMin=%#x offFifoMax=%#x)\n",
                           offNextCmd, offFifoMin, offFifoMax));
            cbAfter = offFifoMax - offCurrentCmd;
        }
        cbBefore = 0;
    }
    else
    {
        cbAfter  = offFifoMax - offCurrentCmd;
        if (offNextCmd >= offFifoMin)
            cbBefore = offNextCmd - offFifoMin;
        else
        {
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoErrors);
            LogRelMax(16, ("vmsvgaFIFOGetCmdPayload: Invalid offNextCmd=%#x (offFifoMin=%#x offFifoMax=%#x)\n",
                           offNextCmd, offFifoMin, offFifoMax));
            cbBefore = 0;
        }
    }
    if (cbAfter + cbBefore < cbPayloadReq)
    {
        /*
         * Insufficient, must wait for it to arrive.
         */
/** @todo Should clear the busy flag here to maybe encourage the guest to wake us up. */
        STAM_REL_PROFILE_START(&pSVGAState->StatFifoStalls, Stall);
        for (uint32_t i = 0;; i++)
        {
            if (pThread->enmState != PDMTHREADSTATE_RUNNING)
            {
                STAM_REL_PROFILE_STOP(&pSVGAState->StatFifoStalls, Stall);
                return (void *)(uintptr_t)1;
            }
            Log(("Guest still copying (%x vs %x) current %x next %x stop %x loop %u; sleep a bit\n",
                 cbPayloadReq, cbAfter + cbBefore, offCurrentCmd, offNextCmd, pFIFO[SVGA_FIFO_STOP], i));

            SUPSemEventWaitNoResume(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem, i < 16 ? 1 : 2);

            offNextCmd = pFIFO[SVGA_FIFO_NEXT_CMD];
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
            if (offNextCmd >= offCurrentCmd)
            {
                cbAfter = RT_MIN(offNextCmd, offFifoMax) - offCurrentCmd;
                cbBefore = 0;
            }
            else
            {
                cbAfter  = offFifoMax - offCurrentCmd;
                cbBefore = RT_MAX(offNextCmd, offFifoMin) - offFifoMin;
            }

            if (cbAfter + cbBefore >= cbPayloadReq)
                break;
        }
        STAM_REL_PROFILE_STOP(&pSVGAState->StatFifoStalls, Stall);
    }

    /*
     * Copy out the memory and update what pcbAlreadyRead points to.
     */
    if (cbAfter >= cbPayloadReq)
        memcpy(pbBounceBuf + cbAlreadyRead,
               (uint8_t *)pFIFO + offCurrentCmd + cbAlreadyRead,
               cbPayloadReq - cbAlreadyRead);
    else
    {
        LogFlow(("Split data buffer at %x (%u-%u)\n", offCurrentCmd, cbAfter, cbBefore));
        if (cbAlreadyRead < cbAfter)
        {
            memcpy(pbBounceBuf + cbAlreadyRead,
                   (uint8_t *)pFIFO + offCurrentCmd + cbAlreadyRead,
                   cbAfter - cbAlreadyRead);
            cbAlreadyRead = cbAfter;
        }
        memcpy(pbBounceBuf + cbAlreadyRead,
               (uint8_t *)pFIFO + offFifoMin + cbAlreadyRead - cbAfter,
               cbPayloadReq - cbAlreadyRead);
    }
    *pcbAlreadyRead = cbPayloadReq;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    return pbBounceBuf;
}

/* The async FIFO handling thread. */
static DECLCALLBACK(int) vmsvgaFIFOLoop(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    PVGASTATE       pThis = (PVGASTATE)pThread->pvUser;
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;
    int             rc;

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    /*
     * Special mode where we only execute an external command and the go back
     * to being suspended.  Currently, all ext cmds ends up here, with the reset
     * one also being eligble for runtime execution further down as well.
     */
    if (pThis->svga.fFifoExtCommandWakeup)
    {
        vmsvgaR3FifoHandleExtCmd(pThis);
        while (pThread->enmState == PDMTHREADSTATE_RUNNING)
            if (pThis->svga.u8FIFOExtCommand == VMSVGA_FIFO_EXTCMD_NONE)
                SUPSemEventWaitNoResume(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem, RT_MS_1MIN);
            else
                vmsvgaR3FifoHandleExtCmd(pThis);
        return VINF_SUCCESS;
    }


    /*
     * Signal the semaphore to make sure we don't wait for 250ms after a
     * suspend & resume scenario (see vmsvgaFIFOGetCmdPayload).
     */
    SUPSemEventSignal(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem);

    /*
     * Allocate a bounce buffer for command we get from the FIFO.
     * (All code must return via the end of the function to free this buffer.)
     */
    uint8_t *pbBounceBuf = (uint8_t *)RTMemAllocZ(pThis->svga.cbFIFO);
    AssertReturn(pbBounceBuf, VERR_NO_MEMORY);

    /*
     * Polling/sleep interval config.
     *
     * We wait for an a short interval if the guest has recently given us work
     * to do, but the interval increases the longer we're kept idle.  With the
     * current parameters we'll be at a 64ms poll interval after 1 idle second,
     * at 90ms after 2 seconds, and reach the max 250ms interval after about
     * 16 seconds.
     */
    RTMSINTERVAL const  cMsMinSleep = 16;
    RTMSINTERVAL const  cMsIncSleep = 2;
    RTMSINTERVAL const  cMsMaxSleep = 250;
    RTMSINTERVAL        cMsSleep    = cMsMaxSleep;

    /*
     * The FIFO loop.
     */
    LogFlow(("vmsvgaFIFOLoop: started loop\n"));
    bool fBadOrDisabledFifo = false;
    uint32_t RT_UNTRUSTED_VOLATILE_GUEST * const pFIFO = pThis->svga.pFIFOR3;
    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
# if defined(RT_OS_DARWIN) && defined(VBOX_WITH_VMSVGA3D)
        /*
         * Should service the run loop every so often.
         */
        if (pThis->svga.f3DEnabled)
            vmsvga3dCocoaServiceRunLoop();
# endif

        /*
         * Unless there's already work pending, go to sleep for a short while.
         * (See polling/sleep interval config above.)
         */
        if (   fBadOrDisabledFifo
            || pFIFO[SVGA_FIFO_NEXT_CMD] == pFIFO[SVGA_FIFO_STOP])
        {
            rc = SUPSemEventWaitNoResume(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem, cMsSleep);
            AssertBreak(RT_SUCCESS(rc) || rc == VERR_TIMEOUT || rc == VERR_INTERRUPTED);
            if (pThread->enmState != PDMTHREADSTATE_RUNNING)
            {
                LogFlow(("vmsvgaFIFOLoop: thread state %x\n", pThread->enmState));
                break;
            }
        }
        else
            rc = VINF_SUCCESS;
        fBadOrDisabledFifo = false;
        if (rc == VERR_TIMEOUT)
        {
            if (pFIFO[SVGA_FIFO_NEXT_CMD] == pFIFO[SVGA_FIFO_STOP])
            {
                cMsSleep = RT_MIN(cMsSleep + cMsIncSleep, cMsMaxSleep);
                continue;
            }
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoTodoTimeout);

            Log(("vmsvgaFIFOLoop: timeout\n"));
        }
        else if (pFIFO[SVGA_FIFO_NEXT_CMD] != pFIFO[SVGA_FIFO_STOP])
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoTodoWoken);
        cMsSleep = cMsMinSleep;

        Log(("vmsvgaFIFOLoop: enabled=%d configured=%d busy=%d\n", pThis->svga.fEnabled, pThis->svga.fConfigured, pThis->svga.pFIFOR3[SVGA_FIFO_BUSY]));
        Log(("vmsvgaFIFOLoop: min  %x max  %x\n", pFIFO[SVGA_FIFO_MIN], pFIFO[SVGA_FIFO_MAX]));
        Log(("vmsvgaFIFOLoop: next %x stop %x\n", pFIFO[SVGA_FIFO_NEXT_CMD], pFIFO[SVGA_FIFO_STOP]));

        /*
         * Handle external commands (currently only reset).
         */
        if (pThis->svga.u8FIFOExtCommand != VMSVGA_FIFO_EXTCMD_NONE)
        {
            vmsvgaR3FifoHandleExtCmd(pThis);
            continue;
        }

        /*
         * The device must be enabled and configured.
         */
        if (   !pThis->svga.fEnabled
            || !pThis->svga.fConfigured)
        {
            vmsvgaFifoSetNotBusy(pThis, pSVGAState, pFIFO[SVGA_FIFO_MIN]);
            fBadOrDisabledFifo = true;
            continue;
        }

        /*
         * Get and check the min/max values.  We ASSUME that they will remain
         * unchanged while we process requests.  A further ASSUMPTION is that
         * the guest won't mess with SVGA_FIFO_NEXT_CMD while we're busy, so
         * we don't read it back while in the loop.
         */
        uint32_t const offFifoMin    = pFIFO[SVGA_FIFO_MIN];
        uint32_t const offFifoMax    = pFIFO[SVGA_FIFO_MAX];
        uint32_t       offCurrentCmd = pFIFO[SVGA_FIFO_STOP];
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
        if (RT_UNLIKELY(   !VMSVGA_IS_VALID_FIFO_REG(SVGA_FIFO_STOP, offFifoMin)
                        || offFifoMax <= offFifoMin
                        || offFifoMax > pThis->svga.cbFIFO
                        || (offFifoMax & 3) != 0
                        || (offFifoMin & 3) != 0
                        || offCurrentCmd < offFifoMin
                        || offCurrentCmd > offFifoMax))
        {
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoErrors);
            LogRelMax(8, ("vmsvgaFIFOLoop: Bad fifo: min=%#x stop=%#x max=%#x\n", offFifoMin, offCurrentCmd, offFifoMax));
            vmsvgaFifoSetNotBusy(pThis, pSVGAState, offFifoMin);
            fBadOrDisabledFifo = true;
            continue;
        }
        RT_UNTRUSTED_VALIDATED_FENCE();
        if (RT_UNLIKELY(offCurrentCmd & 3))
        {
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoErrors);
            LogRelMax(8, ("vmsvgaFIFOLoop: Misaligned offCurrentCmd=%#x?\n", offCurrentCmd));
            offCurrentCmd = ~UINT32_C(3);
        }

/** @def VMSVGAFIFO_GET_CMD_BUFFER_BREAK
 * Macro for shortening calls to vmsvgaFIFOGetCmdPayload.
 *
 * Will break out of the switch on failure.
 * Will restart and quit the loop if the thread was requested to stop.
 *
 * @param   a_PtrVar        Request variable pointer.
 * @param   a_Type          Request typedef (not pointer) for casting.
 * @param   a_cbPayloadReq  How much payload to fetch.
 * @remarks Accesses a bunch of variables in the current scope!
 */
# define VMSVGAFIFO_GET_CMD_BUFFER_BREAK(a_PtrVar, a_Type, a_cbPayloadReq) \
            if (1) { \
                (a_PtrVar) = (a_Type *)vmsvgaFIFOGetCmdPayload((a_cbPayloadReq), pFIFO, offCurrentCmd, offFifoMin, offFifoMax, \
                                                               pbBounceBuf, &cbPayload, pThread, pThis, pSVGAState); \
                if (RT_UNLIKELY((uintptr_t)(a_PtrVar) < 2)) { if ((uintptr_t)(a_PtrVar) == 1) continue; break; } \
                RT_UNTRUSTED_NONVOLATILE_COPY_FENCE(); \
            } else do {} while (0)
/** @def VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK
 * Macro for shortening calls to vmsvgaFIFOGetCmdPayload for refetching the
 * buffer after figuring out the actual command size.
 *
 * Will break out of the switch on failure.
 *
 * @param   a_PtrVar        Request variable pointer.
 * @param   a_Type          Request typedef (not pointer) for casting.
 * @param   a_cbPayloadReq  How much payload to fetch.
 * @remarks Accesses a bunch of variables in the current scope!
 */
# define VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(a_PtrVar, a_Type, a_cbPayloadReq) \
            if (1) { \
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(a_PtrVar, a_Type, a_cbPayloadReq); \
            } else do {} while (0)

        /*
         * Mark the FIFO as busy.
         */
        ASMAtomicWriteU32(&pThis->svga.fBusy, VMSVGA_BUSY_F_FIFO);
        if (VMSVGA_IS_VALID_FIFO_REG(SVGA_FIFO_BUSY, offFifoMin))
            ASMAtomicWriteU32(&pFIFO[SVGA_FIFO_BUSY], true);

        /*
         * Execute all queued FIFO commands.
         * Quit if pending external command or changes in the thread state.
         */
        bool fDone = false;
        while (   !(fDone = (pFIFO[SVGA_FIFO_NEXT_CMD] == offCurrentCmd))
               && pThread->enmState == PDMTHREADSTATE_RUNNING)
        {
            uint32_t cbPayload = 0;
            uint32_t u32IrqStatus = 0;

            Assert(offCurrentCmd < offFifoMax && offCurrentCmd >= offFifoMin);

            /* First check any pending actions. */
            if (   ASMBitTestAndClear(&pThis->svga.u32ActionFlags, VMSVGA_ACTION_CHANGEMODE_BIT)
                && pThis->svga.p3dState != NULL)
# ifdef VBOX_WITH_VMSVGA3D
                vmsvga3dChangeMode(pThis);
# else
                {/*nothing*/}
# endif
            /* Check for pending external commands (reset). */
            if (pThis->svga.u8FIFOExtCommand != VMSVGA_FIFO_EXTCMD_NONE)
                break;

            /*
             * Process the command.
             */
            SVGAFifoCmdId const enmCmdId = (SVGAFifoCmdId)pFIFO[offCurrentCmd / sizeof(uint32_t)];
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
            LogFlow(("vmsvgaFIFOLoop: FIFO command (iCmd=0x%x) %s 0x%x\n",
                     offCurrentCmd / sizeof(uint32_t), vmsvgaFIFOCmdToString(enmCmdId), enmCmdId));
            switch (enmCmdId)
            {
            case SVGA_CMD_INVALID_CMD:
                /* Nothing to do. */
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdInvalidCmd);
                break;

            case SVGA_CMD_FENCE:
            {
                SVGAFifoCmdFence *pCmdFence;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmdFence, SVGAFifoCmdFence, sizeof(*pCmdFence));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdFence);
                if (VMSVGA_IS_VALID_FIFO_REG(SVGA_FIFO_FENCE, offFifoMin))
                {
                    Log(("vmsvgaFIFOLoop: SVGA_CMD_FENCE %x\n", pCmdFence->fence));
                    pFIFO[SVGA_FIFO_FENCE] = pCmdFence->fence;

                    if (pThis->svga.u32IrqMask & SVGA_IRQFLAG_ANY_FENCE)
                    {
                        Log(("vmsvgaFIFOLoop: any fence irq\n"));
                        u32IrqStatus |= SVGA_IRQFLAG_ANY_FENCE;
                    }
                    else
                    if (    VMSVGA_IS_VALID_FIFO_REG(SVGA_FIFO_FENCE_GOAL, offFifoMin)
                        &&  (pThis->svga.u32IrqMask & SVGA_IRQFLAG_FENCE_GOAL)
                        &&  pFIFO[SVGA_FIFO_FENCE_GOAL] == pCmdFence->fence)
                    {
                        Log(("vmsvgaFIFOLoop: fence goal reached irq (fence=%x)\n", pCmdFence->fence));
                        u32IrqStatus |= SVGA_IRQFLAG_FENCE_GOAL;
                    }
                }
                else
                    Log(("SVGA_CMD_FENCE is bogus when offFifoMin is %#x!\n", offFifoMin));
                break;
            }
            case SVGA_CMD_UPDATE:
            case SVGA_CMD_UPDATE_VERBOSE:
            {
                SVGAFifoCmdUpdate *pUpdate;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pUpdate, SVGAFifoCmdUpdate, sizeof(*pUpdate));
                if (enmCmdId == SVGA_CMD_UPDATE)
                    STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdUpdate);
                else
                    STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdUpdateVerbose);
                Log(("vmsvgaFIFOLoop: UPDATE (%d,%d)(%d,%d)\n", pUpdate->x, pUpdate->y, pUpdate->width, pUpdate->height));
                vgaR3UpdateDisplay(pThis, pUpdate->x, pUpdate->y, pUpdate->width, pUpdate->height);
                break;
            }

            case SVGA_CMD_DEFINE_CURSOR:
            {
                /* Followed by bitmap data. */
                SVGAFifoCmdDefineCursor *pCursor;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCursor, SVGAFifoCmdDefineCursor, sizeof(*pCursor));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineCursor);

                Log(("vmsvgaFIFOLoop: CURSOR id=%d size (%d,%d) hotspot (%d,%d) andMaskDepth=%d xorMaskDepth=%d\n",
                     pCursor->id, pCursor->width, pCursor->height, pCursor->hotspotX, pCursor->hotspotY,
                     pCursor->andMaskDepth, pCursor->xorMaskDepth));
                AssertBreak(pCursor->height < 2048 && pCursor->width < 2048);
                AssertBreak(pCursor->andMaskDepth <= 32);
                AssertBreak(pCursor->xorMaskDepth <= 32);
                RT_UNTRUSTED_VALIDATED_FENCE();

                uint32_t cbAndLine = RT_ALIGN_32(pCursor->width * (pCursor->andMaskDepth + (pCursor->andMaskDepth == 15)), 32) / 8;
                uint32_t cbAndMask = cbAndLine * pCursor->height;
                uint32_t cbXorLine = RT_ALIGN_32(pCursor->width * (pCursor->xorMaskDepth + (pCursor->xorMaskDepth == 15)), 32) / 8;
                uint32_t cbXorMask = cbXorLine * pCursor->height;
                VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(pCursor, SVGAFifoCmdDefineCursor, sizeof(*pCursor) + cbAndMask + cbXorMask);

                vmsvgaR3CmdDefineCursor(pThis, pSVGAState, pCursor, (uint8_t const *)(pCursor + 1), cbAndLine,
                                        (uint8_t const *)(pCursor + 1) + cbAndMask, cbXorLine);
                break;
            }

            case SVGA_CMD_DEFINE_ALPHA_CURSOR:
            {
                /* Followed by bitmap data. */
                uint32_t cbCursorShape, cbAndMask;
                uint8_t *pCursorCopy;
                uint32_t cbCmd;

                SVGAFifoCmdDefineAlphaCursor *pCursor;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCursor, SVGAFifoCmdDefineAlphaCursor, sizeof(*pCursor));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineAlphaCursor);

                Log(("vmsvgaFIFOLoop: ALPHA_CURSOR id=%d size (%d,%d) hotspot (%d,%d)\n", pCursor->id, pCursor->width, pCursor->height, pCursor->hotspotX, pCursor->hotspotY));

                /* Check against a reasonable upper limit to prevent integer overflows in the sanity checks below. */
                AssertBreak(pCursor->height < 2048 && pCursor->width < 2048);
                RT_UNTRUSTED_VALIDATED_FENCE();

                /* Refetch the bitmap data as well. */
                cbCmd = sizeof(SVGAFifoCmdDefineAlphaCursor) + pCursor->width * pCursor->height * sizeof(uint32_t) /* 32-bit BRGA format */;
                VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(pCursor, SVGAFifoCmdDefineAlphaCursor, cbCmd);
                /** @todo Would be more efficient to copy the data straight into pCursorCopy (memcpy below). */

                /* The mouse pointer interface always expects an AND mask followed by the color data (XOR mask). */
                cbAndMask     = (pCursor->width + 7) / 8 * pCursor->height;                         /* size of the AND mask */
                cbAndMask     = ((cbAndMask + 3) & ~3);                                             /* + gap for alignment */
                cbCursorShape = cbAndMask + pCursor->width * sizeof(uint32_t) * pCursor->height;    /* + size of the XOR mask (32-bit BRGA format) */

                pCursorCopy = (uint8_t *)RTMemAlloc(cbCursorShape);
                AssertBreak(pCursorCopy);

                /* Transparency is defined by the alpha bytes, so make the whole bitmap visible. */
                memset(pCursorCopy, 0xff, cbAndMask);
                /* Colour data */
                memcpy(pCursorCopy + cbAndMask, (pCursor + 1), pCursor->width * pCursor->height * sizeof(uint32_t));

                vmsvgaR3InstallNewCursor(pThis, pSVGAState, true /*fAlpha*/, pCursor->hotspotX, pCursor->hotspotY,
                                         pCursor->width, pCursor->height, pCursorCopy, cbCursorShape);
                break;
            }

            case SVGA_CMD_ESCAPE:
            {
                /* Followed by nsize bytes of data. */
                SVGAFifoCmdEscape *pEscape;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pEscape, SVGAFifoCmdEscape, sizeof(*pEscape));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdEscape);

                /* Refetch the command buffer with the variable data; undo size increase (ugly) */
                AssertBreak(pEscape->size < pThis->svga.cbFIFO);
                RT_UNTRUSTED_VALIDATED_FENCE();
                uint32_t cbCmd = sizeof(SVGAFifoCmdEscape) + pEscape->size;
                VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(pEscape, SVGAFifoCmdEscape, cbCmd);

                if (pEscape->nsid == SVGA_ESCAPE_NSID_VMWARE)
                {
                    AssertBreak(pEscape->size >= sizeof(uint32_t));
                    RT_UNTRUSTED_VALIDATED_FENCE();
                    uint32_t cmd = *(uint32_t *)(pEscape + 1);
                    Log(("vmsvgaFIFOLoop: ESCAPE (%x %x) VMWARE cmd=%x\n", pEscape->nsid, pEscape->size, cmd));

                    switch (cmd)
                    {
                        case SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS:
                        {
                            SVGAEscapeVideoSetRegs *pVideoCmd = (SVGAEscapeVideoSetRegs *)(pEscape + 1);
                            AssertBreak(pEscape->size >= sizeof(pVideoCmd->header));
                            uint32_t cRegs = (pEscape->size - sizeof(pVideoCmd->header)) / sizeof(pVideoCmd->items[0]);

                            Log(("SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS: stream %x\n", pVideoCmd->header.streamId));
                            for (uint32_t iReg = 0; iReg < cRegs; iReg++)
                                Log(("SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS: reg %x val %x\n", pVideoCmd->items[iReg].registerId, pVideoCmd->items[iReg].value));

                            RT_NOREF_PV(pVideoCmd);
                            break;

                        }

                        case SVGA_ESCAPE_VMWARE_VIDEO_FLUSH:
                        {
                            SVGAEscapeVideoFlush *pVideoCmd = (SVGAEscapeVideoFlush *)(pEscape + 1);
                            AssertBreak(pEscape->size >= sizeof(*pVideoCmd));
                            Log(("SVGA_ESCAPE_VMWARE_VIDEO_FLUSH: stream %x\n", pVideoCmd->streamId));
                            RT_NOREF_PV(pVideoCmd);
                            break;
                        }

                        default:
                            Log(("SVGA_CMD_ESCAPE: Unknown vmware escape: %x\n", cmd));
                            break;
                    }
                }
                else
                    Log(("vmsvgaFIFOLoop: ESCAPE %x %x\n", pEscape->nsid, pEscape->size));

                break;
            }
# ifdef VBOX_WITH_VMSVGA3D
            case SVGA_CMD_DEFINE_GMR2:
            {
                SVGAFifoCmdDefineGMR2 *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdDefineGMR2, sizeof(*pCmd));
                Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_GMR2 id=%x %x pages\n", pCmd->gmrId, pCmd->numPages));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineGmr2);

                /* Validate current GMR id. */
                AssertBreak(pCmd->gmrId < pThis->svga.cGMR);
                AssertBreak(pCmd->numPages <= VMSVGA_MAX_GMR_PAGES);
                RT_UNTRUSTED_VALIDATED_FENCE();

                if (!pCmd->numPages)
                {
                    STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineGmr2Free);
                    vmsvgaGMRFree(pThis, pCmd->gmrId);
                }
                else
                {
                    PGMR pGMR = &pSVGAState->paGMR[pCmd->gmrId];
                    if (pGMR->cMaxPages)
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineGmr2Modify);

                    /* Not sure if we should always free the descriptor, but for simplicity
                       we do so if the new size is smaller than the current. */
                    /** @todo always free the descriptor in SVGA_CMD_DEFINE_GMR2? */
                    if (pGMR->cbTotal / X86_PAGE_SIZE > pGMR->cMaxPages)
                        vmsvgaGMRFree(pThis, pCmd->gmrId);

                    pGMR->cMaxPages = pCmd->numPages;
                    /* The rest is done by the REMAP_GMR2 command. */
                }
                break;
            }

            case SVGA_CMD_REMAP_GMR2:
            {
                /* Followed by page descriptors or guest ptr. */
                SVGAFifoCmdRemapGMR2 *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdRemapGMR2, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdRemapGmr2);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_REMAP_GMR2 id=%x flags=%x offset=%x npages=%x\n", pCmd->gmrId, pCmd->flags, pCmd->offsetPages, pCmd->numPages));
                AssertBreak(pCmd->gmrId < pThis->svga.cGMR);
                RT_UNTRUSTED_VALIDATED_FENCE();

                /* Calculate the size of what comes after next and fetch it. */
                uint32_t cbCmd = sizeof(SVGAFifoCmdRemapGMR2);
                if (pCmd->flags & SVGA_REMAP_GMR2_VIA_GMR)
                    cbCmd += sizeof(SVGAGuestPtr);
                else
                {
                    uint32_t const cbPageDesc = (pCmd->flags & SVGA_REMAP_GMR2_PPN64) ? sizeof(uint64_t) : sizeof(uint32_t);
                    if (pCmd->flags & SVGA_REMAP_GMR2_SINGLE_PPN)
                    {
                        cbCmd         += cbPageDesc;
                        pCmd->numPages = 1;
                    }
                    else
                    {
                        AssertBreak(pCmd->numPages <= pThis->svga.cbFIFO / cbPageDesc);
                        cbCmd += cbPageDesc * pCmd->numPages;
                    }
                }
                VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdRemapGMR2, cbCmd);

                /* Validate current GMR id and size. */
                AssertBreak(pCmd->gmrId < pThis->svga.cGMR);
                RT_UNTRUSTED_VALIDATED_FENCE();
                PGMR pGMR = &pSVGAState->paGMR[pCmd->gmrId];
                AssertBreak(   (uint64_t)pCmd->offsetPages + pCmd->numPages
                            <= RT_MIN(pGMR->cMaxPages, RT_MIN(VMSVGA_MAX_GMR_PAGES, UINT32_MAX / X86_PAGE_SIZE)));
                AssertBreak(!pCmd->offsetPages || pGMR->paDesc); /** @todo */

                if (pCmd->numPages == 0)
                    break;

                /* Calc new total page count so we can use it instead of cMaxPages for allocations below. */
                uint32_t const cNewTotalPages = RT_MAX(pGMR->cbTotal >> X86_PAGE_SHIFT, pCmd->offsetPages + pCmd->numPages);

                /*
                 * We flatten the existing descriptors into a page array, overwrite the
                 * pages specified in this command and then recompress the descriptor.
                 */
                /** @todo Optimize the GMR remap algorithm! */

                /* Save the old page descriptors as an array of page frame numbers (address >> X86_PAGE_SHIFT) */
                uint64_t *paNewPage64 = NULL;
                if (pGMR->paDesc)
                {
                    STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdRemapGmr2Modify);

                    paNewPage64 = (uint64_t *)RTMemAllocZ(cNewTotalPages * sizeof(uint64_t));
                    AssertBreak(paNewPage64);

                    uint32_t idxPage = 0;
                    for (uint32_t i = 0; i < pGMR->numDescriptors; i++)
                        for (uint32_t j = 0; j < pGMR->paDesc[i].numPages; j++)
                            paNewPage64[idxPage++] = (pGMR->paDesc[i].GCPhys + j * X86_PAGE_SIZE) >> X86_PAGE_SHIFT;
                    AssertBreakStmt(idxPage == pGMR->cbTotal >> X86_PAGE_SHIFT, RTMemFree(paNewPage64));
                    RT_UNTRUSTED_VALIDATED_FENCE();
                }

                /* Free the old GMR if present. */
                if (pGMR->paDesc)
                    RTMemFree(pGMR->paDesc);

                /* Allocate the maximum amount possible (everything non-continuous) */
                PVMSVGAGMRDESCRIPTOR paDescs;
                pGMR->paDesc = paDescs = (PVMSVGAGMRDESCRIPTOR)RTMemAllocZ(cNewTotalPages * sizeof(VMSVGAGMRDESCRIPTOR));
                AssertBreakStmt(paDescs, RTMemFree(paNewPage64));

                if (pCmd->flags & SVGA_REMAP_GMR2_VIA_GMR)
                {
                    /** @todo */
                    AssertFailed();
                    pGMR->numDescriptors = 0;
                }
                else
                {
                    uint32_t  *paPages32 = (uint32_t *)(pCmd + 1);
                    uint64_t  *paPages64 = (uint64_t *)(pCmd + 1);
                    bool       fGCPhys64 = RT_BOOL(pCmd->flags & SVGA_REMAP_GMR2_PPN64);

                    if (paNewPage64)
                    {
                        /* Overwrite the old page array with the new page values. */
                        if (fGCPhys64)
                            for (uint32_t i = pCmd->offsetPages; i < pCmd->offsetPages + pCmd->numPages; i++)
                                paNewPage64[i] = paPages64[i - pCmd->offsetPages];
                        else
                            for (uint32_t i = pCmd->offsetPages; i < pCmd->offsetPages + pCmd->numPages; i++)
                                paNewPage64[i] = paPages32[i - pCmd->offsetPages];

                        /* Use the updated page array instead of the command data. */
                        fGCPhys64      = true;
                        paPages64      = paNewPage64;
                        pCmd->numPages = cNewTotalPages;
                    }

                    /* The first page. */
                    /** @todo The 0x00000FFFFFFFFFFF mask limits to 44 bits and should not be
                     *        applied to paNewPage64. */
                    RTGCPHYS GCPhys;
                    if (fGCPhys64)
                        GCPhys = (paPages64[0] << X86_PAGE_SHIFT) & UINT64_C(0x00000FFFFFFFFFFF); /* Seeing rubbish in the top bits with certain linux guests. */
                    else
                        GCPhys = (RTGCPHYS)paPages32[0] << PAGE_SHIFT;
                    paDescs[0].GCPhys    = GCPhys;
                    paDescs[0].numPages  = 1;

                    /* Subsequent pages. */
                    uint32_t iDescriptor = 0;
                    for (uint32_t i = 1; i < pCmd->numPages; i++)
                    {
                        if (pCmd->flags & SVGA_REMAP_GMR2_PPN64)
                            GCPhys = (paPages64[i] << X86_PAGE_SHIFT) & UINT64_C(0x00000FFFFFFFFFFF); /* Seeing rubbish in the top bits with certain linux guests. */
                        else
                            GCPhys = (RTGCPHYS)paPages32[i] << X86_PAGE_SHIFT;

                        /* Continuous physical memory? */
                        if (GCPhys == paDescs[iDescriptor].GCPhys + paDescs[iDescriptor].numPages * X86_PAGE_SIZE)
                        {
                            Assert(paDescs[iDescriptor].numPages);
                            paDescs[iDescriptor].numPages++;
                            LogFlow(("Page %x GCPhys=%RGp successor\n", i, GCPhys));
                        }
                        else
                        {
                            iDescriptor++;
                            paDescs[iDescriptor].GCPhys   = GCPhys;
                            paDescs[iDescriptor].numPages = 1;
                            LogFlow(("Page %x GCPhys=%RGp\n", i, paDescs[iDescriptor].GCPhys));
                        }
                    }

                    pGMR->cbTotal = cNewTotalPages << X86_PAGE_SHIFT;
                    LogFlow(("Nr of descriptors %x; cbTotal=%#x\n", iDescriptor + 1, cNewTotalPages));
                    pGMR->numDescriptors = iDescriptor + 1;
                }

                if (paNewPage64)
                    RTMemFree(paNewPage64);

#  ifdef DEBUG_GMR_ACCESS
                VMR3ReqCallWaitU(PDMDevHlpGetUVM(pThis->pDevInsR3), VMCPUID_ANY, (PFNRT)vmsvgaRegisterGMR, 2, pThis->pDevInsR3, pCmd->gmrId);
#  endif
                break;
            }
# endif // VBOX_WITH_VMSVGA3D
            case SVGA_CMD_DEFINE_SCREEN:
            {
                /* Note! The size of this command is specified by the guest and depends on capabilities. */
                Assert(!(pThis->svga.pFIFOR3[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_SCREEN_OBJECT));
                SVGAFifoCmdDefineScreen *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdDefineScreen, sizeof(pCmd->screen.structSize));
                RT_BZERO(&pCmd->screen.id, sizeof(*pCmd) - RT_OFFSETOF(SVGAFifoCmdDefineScreen, screen.structSize));
                VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdDefineScreen, RT_MAX(sizeof(pCmd->screen.structSize), pCmd->screen.structSize));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineScreen);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_SCREEN id=%x flags=%x size=(%d,%d) root=(%d,%d)\n", pCmd->screen.id, pCmd->screen.flags, pCmd->screen.size.width, pCmd->screen.size.height, pCmd->screen.root.x, pCmd->screen.root.y));
                if (pCmd->screen.flags & SVGA_SCREEN_HAS_ROOT)
                    Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_SCREEN flags SVGA_SCREEN_HAS_ROOT\n"));
                if (pCmd->screen.flags & SVGA_SCREEN_IS_PRIMARY)
                    Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_SCREEN flags SVGA_SCREEN_IS_PRIMARY\n"));
                if (pCmd->screen.flags & SVGA_SCREEN_FULLSCREEN_HINT)
                    Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_SCREEN flags SVGA_SCREEN_FULLSCREEN_HINT\n"));
                if (pCmd->screen.flags & SVGA_SCREEN_DEACTIVATE )
                    Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_SCREEN flags SVGA_SCREEN_DEACTIVATE \n"));
                if (pCmd->screen.flags & SVGA_SCREEN_BLANKING)
                    Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_SCREEN flags SVGA_SCREEN_BLANKING\n"));

                RT_UNTRUSTED_VALIDATED_FENCE();

                /** @todo multi monitor support and screen object capabilities.   */
                pThis->svga.uWidth  = pCmd->screen.size.width;
                pThis->svga.uHeight = pCmd->screen.size.height;
                vmsvgaChangeMode(pThis);
                break;
            }

            case SVGA_CMD_DESTROY_SCREEN:
            {
                SVGAFifoCmdDestroyScreen *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdDestroyScreen, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDestroyScreen);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_DESTROY_SCREEN id=%x\n", pCmd->screenId));
                break;
            }
# ifdef VBOX_WITH_VMSVGA3D
            case SVGA_CMD_DEFINE_GMRFB:
            {
                SVGAFifoCmdDefineGMRFB *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdDefineGMRFB, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdDefineGmrFb);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_DEFINE_GMRFB gmr=%x offset=%x bytesPerLine=%x bpp=%d color depth=%d\n", pCmd->ptr.gmrId, pCmd->ptr.offset, pCmd->bytesPerLine, pCmd->format.s.bitsPerPixel, pCmd->format.s.colorDepth));
                pSVGAState->GMRFB.ptr          = pCmd->ptr;
                pSVGAState->GMRFB.bytesPerLine = pCmd->bytesPerLine;
                pSVGAState->GMRFB.format       = pCmd->format;
                break;
            }

            case SVGA_CMD_BLIT_GMRFB_TO_SCREEN:
            {
                uint32_t width, height;
                SVGAFifoCmdBlitGMRFBToScreen *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdBlitGMRFBToScreen, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdBlitGmrFbToScreen);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_BLIT_GMRFB_TO_SCREEN src=(%d,%d) dest id=%d (%d,%d)(%d,%d)\n", pCmd->srcOrigin.x, pCmd->srcOrigin.y, pCmd->destScreenId, pCmd->destRect.left, pCmd->destRect.top, pCmd->destRect.right, pCmd->destRect.bottom));

                /** @todo Support GMRFB.format.s.bitsPerPixel != pThis->svga.uBpp   */
                AssertBreak(pSVGAState->GMRFB.format.s.bitsPerPixel == pThis->svga.uBpp);
                AssertBreak(pCmd->destScreenId == 0);

                if (pCmd->destRect.left < 0)
                    pCmd->destRect.left = 0;
                if (pCmd->destRect.top < 0)
                    pCmd->destRect.top = 0;
                if (pCmd->destRect.right < 0)
                    pCmd->destRect.right = 0;
                if (pCmd->destRect.bottom < 0)
                    pCmd->destRect.bottom = 0;

                width  = pCmd->destRect.right - pCmd->destRect.left;
                height = pCmd->destRect.bottom - pCmd->destRect.top;

                if (    width == 0
                    ||  height == 0)
                    break;  /* Nothing to do. */

                /* Clip to screen dimensions. */
                if (width > pThis->svga.uWidth)
                    width = pThis->svga.uWidth;
                if (height > pThis->svga.uHeight)
                    height = pThis->svga.uHeight;

                /* srcOrigin */
                AssertBreak(pSVGAState->GMRFB.bytesPerLine != 0);
                AssertBreak(pSVGAState->GMRFB.format.s.bitsPerPixel != 0);

                const uint32_t cScanlines = pThis->vram_size / pSVGAState->GMRFB.bytesPerLine;
                AssertBreak(pCmd->srcOrigin.y < (int32_t)cScanlines);

                AssertBreak(pCmd->srcOrigin.x < (int32_t)(pSVGAState->GMRFB.bytesPerLine / ((pSVGAState->GMRFB.format.s.bitsPerPixel + 7) / 8)));

                unsigned offsetSource = (pCmd->srcOrigin.x * pSVGAState->GMRFB.format.s.bitsPerPixel) / 8 + pSVGAState->GMRFB.bytesPerLine * pCmd->srcOrigin.y;
                unsigned offsetDest   = (pCmd->destRect.left * RT_ALIGN(pThis->svga.uBpp, 8)) / 8 + pThis->svga.cbScanline * pCmd->destRect.top;
                unsigned cbCopyWidth  = (width * RT_ALIGN(pThis->svga.uBpp, 8)) / 8;

                AssertBreak(offsetDest < pThis->vram_size);

                RT_UNTRUSTED_VALIDATED_FENCE();

                rc = vmsvgaGMRTransfer(pThis, SVGA3D_WRITE_HOST_VRAM, pThis->CTX_SUFF(vram_ptr) + offsetDest, pThis->svga.cbScanline, pSVGAState->GMRFB.ptr, offsetSource, pSVGAState->GMRFB.bytesPerLine, cbCopyWidth, height);
                AssertRC(rc);
                vgaR3UpdateDisplay(pThis, pCmd->destRect.left, pCmd->destRect.top, pCmd->destRect.right - pCmd->destRect.left, pCmd->destRect.bottom - pCmd->destRect.top);
                break;
            }

            case SVGA_CMD_BLIT_SCREEN_TO_GMRFB:
            {
                SVGAFifoCmdBlitScreenToGMRFB *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdBlitScreenToGMRFB, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdBlitScreentoGmrFb);

                /* Note! This can fetch 3d render results as well!! */
                Log(("vmsvgaFIFOLoop: SVGA_CMD_BLIT_SCREEN_TO_GMRFB dest=(%d,%d) src id=%d (%d,%d)(%d,%d)\n", pCmd->destOrigin.x, pCmd->destOrigin.y, pCmd->srcScreenId, pCmd->srcRect.left, pCmd->srcRect.top, pCmd->srcRect.right, pCmd->srcRect.bottom));
                AssertFailed();
                break;
            }
# endif // VBOX_WITH_VMSVGA3D
            case SVGA_CMD_ANNOTATION_FILL:
            {
                SVGAFifoCmdAnnotationFill *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdAnnotationFill, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdAnnotationFill);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_ANNOTATION_FILL red=%x green=%x blue=%x\n", pCmd->color.s.r, pCmd->color.s.g, pCmd->color.s.b));
                pSVGAState->colorAnnotation = pCmd->color;
                break;
            }

            case SVGA_CMD_ANNOTATION_COPY:
            {
                SVGAFifoCmdAnnotationCopy *pCmd;
                VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pCmd, SVGAFifoCmdAnnotationCopy, sizeof(*pCmd));
                STAM_REL_COUNTER_INC(&pSVGAState->StatR3CmdAnnotationCopy);

                Log(("vmsvgaFIFOLoop: SVGA_CMD_ANNOTATION_COPY\n"));
                AssertFailed();
                break;
            }

            /** @todo SVGA_CMD_RECT_COPY  - see with ubuntu */

            default:
# ifdef VBOX_WITH_VMSVGA3D
                if (    (int)enmCmdId >= SVGA_3D_CMD_BASE
                    &&  (int)enmCmdId <  SVGA_3D_CMD_MAX)
                {
                    RT_UNTRUSTED_VALIDATED_FENCE();

                    /* All 3d commands start with a common header, which defines the size of the command. */
                    SVGA3dCmdHeader *pHdr;
                    VMSVGAFIFO_GET_CMD_BUFFER_BREAK(pHdr, SVGA3dCmdHeader, sizeof(*pHdr));
                    AssertBreak(pHdr->size < pThis->svga.cbFIFO);
                    uint32_t cbCmd = sizeof(SVGA3dCmdHeader) + pHdr->size;
                    VMSVGAFIFO_GET_MORE_CMD_BUFFER_BREAK(pHdr, SVGA3dCmdHeader, cbCmd);

/**
 * Check that the 3D command has at least a_cbMin of payload bytes after the
 * header.  Will break out of the switch if it doesn't.
 */
#  define VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(a_cbMin) \
     do { AssertMsgBreak(pHdr->size >= (a_cbMin), ("size=%#x a_cbMin=%#zx\n", pHdr->size, (size_t)(a_cbMin))); \
          RT_UNTRUSTED_VALIDATED_FENCE(); \
     } while (0)
                    switch ((int)enmCmdId)
                    {
                    case SVGA_3D_CMD_SURFACE_DEFINE:
                    {
                        uint32_t                cMipLevels;
                        SVGA3dCmdDefineSurface *pCmd = (SVGA3dCmdDefineSurface *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceDefine);

                        cMipLevels = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dSize);
                        rc = vmsvga3dSurfaceDefine(pThis, pCmd->sid, (uint32_t)pCmd->surfaceFlags, pCmd->format, pCmd->face, 0,
                                                   SVGA3D_TEX_FILTER_NONE, cMipLevels, (SVGA3dSize *)(pCmd + 1));
#  ifdef DEBUG_GMR_ACCESS
                        VMR3ReqCallWaitU(PDMDevHlpGetUVM(pThis->pDevInsR3), VMCPUID_ANY, (PFNRT)vmsvgaResetGMRHandlers, 1, pThis);
#  endif
                        break;
                    }

                    case SVGA_3D_CMD_SURFACE_DEFINE_V2:
                    {
                        uint32_t                   cMipLevels;
                        SVGA3dCmdDefineSurface_v2 *pCmd = (SVGA3dCmdDefineSurface_v2 *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceDefineV2);

                        cMipLevels = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dSize);
                        rc = vmsvga3dSurfaceDefine(pThis, pCmd->sid, pCmd->surfaceFlags, pCmd->format, pCmd->face,
                                                   pCmd->multisampleCount, pCmd->autogenFilter,
                                                   cMipLevels, (SVGA3dSize *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_SURFACE_DESTROY:
                    {
                        SVGA3dCmdDestroySurface *pCmd = (SVGA3dCmdDestroySurface *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceDestroy);
                        rc = vmsvga3dSurfaceDestroy(pThis, pCmd->sid);
                        break;
                    }

                    case SVGA_3D_CMD_SURFACE_COPY:
                    {
                        uint32_t              cCopyBoxes;
                        SVGA3dCmdSurfaceCopy *pCmd = (SVGA3dCmdSurfaceCopy *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceCopy);

                        cCopyBoxes = (pHdr->size - sizeof(pCmd)) / sizeof(SVGA3dCopyBox);
                        rc = vmsvga3dSurfaceCopy(pThis, pCmd->dest, pCmd->src, cCopyBoxes, (SVGA3dCopyBox *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_SURFACE_STRETCHBLT:
                    {
                        SVGA3dCmdSurfaceStretchBlt *pCmd = (SVGA3dCmdSurfaceStretchBlt *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceStretchBlt);

                        rc = vmsvga3dSurfaceStretchBlt(pThis, &pCmd->dest, &pCmd->boxDest, &pCmd->src, &pCmd->boxSrc, pCmd->mode);
                        break;
                    }

                    case SVGA_3D_CMD_SURFACE_DMA:
                    {
                        uint32_t             cCopyBoxes;
                        SVGA3dCmdSurfaceDMA *pCmd = (SVGA3dCmdSurfaceDMA *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceDma);

                        cCopyBoxes = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dCopyBox);
                        STAM_PROFILE_START(&pSVGAState->StatR3Cmd3dSurfaceDmaProf, a);
                        rc = vmsvga3dSurfaceDMA(pThis, pCmd->guest, pCmd->host, pCmd->transfer, cCopyBoxes, (SVGA3dCopyBox *)(pCmd + 1));
                        STAM_PROFILE_STOP(&pSVGAState->StatR3Cmd3dSurfaceDmaProf, a);
                        break;
                    }

                    case SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN:
                    {
                        uint32_t                      cRects;
                        SVGA3dCmdBlitSurfaceToScreen *pCmd = (SVGA3dCmdBlitSurfaceToScreen *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSurfaceScreen);

                        cRects = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGASignedRect);
                        rc = vmsvga3dSurfaceBlitToScreen(pThis, pCmd->destScreenId, pCmd->destRect, pCmd->srcImage, pCmd->srcRect, cRects, (SVGASignedRect *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_CONTEXT_DEFINE:
                    {
                        SVGA3dCmdDefineContext *pCmd = (SVGA3dCmdDefineContext *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dContextDefine);

                        rc = vmsvga3dContextDefine(pThis, pCmd->cid);
                        break;
                    }

                    case SVGA_3D_CMD_CONTEXT_DESTROY:
                    {
                        SVGA3dCmdDestroyContext *pCmd = (SVGA3dCmdDestroyContext *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dContextDestroy);

                        rc = vmsvga3dContextDestroy(pThis, pCmd->cid);
                        break;
                    }

                    case SVGA_3D_CMD_SETTRANSFORM:
                    {
                        SVGA3dCmdSetTransform *pCmd = (SVGA3dCmdSetTransform *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetTransform);

                        rc = vmsvga3dSetTransform(pThis, pCmd->cid, pCmd->type, pCmd->matrix);
                        break;
                    }

                    case SVGA_3D_CMD_SETZRANGE:
                    {
                        SVGA3dCmdSetZRange *pCmd = (SVGA3dCmdSetZRange *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetZRange);

                        rc = vmsvga3dSetZRange(pThis, pCmd->cid, pCmd->zRange);
                        break;
                    }

                    case SVGA_3D_CMD_SETRENDERSTATE:
                    {
                        uint32_t                 cRenderStates;
                        SVGA3dCmdSetRenderState *pCmd = (SVGA3dCmdSetRenderState *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetRenderState);

                        cRenderStates = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dRenderState);
                        rc = vmsvga3dSetRenderState(pThis, pCmd->cid, cRenderStates, (SVGA3dRenderState *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_SETRENDERTARGET:
                    {
                        SVGA3dCmdSetRenderTarget *pCmd = (SVGA3dCmdSetRenderTarget *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetRenderTarget);

                        rc = vmsvga3dSetRenderTarget(pThis, pCmd->cid, pCmd->type, pCmd->target);
                        break;
                    }

                    case SVGA_3D_CMD_SETTEXTURESTATE:
                    {
                        uint32_t                  cTextureStates;
                        SVGA3dCmdSetTextureState *pCmd = (SVGA3dCmdSetTextureState *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetTextureState);

                        cTextureStates = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dTextureState);
                        rc = vmsvga3dSetTextureState(pThis, pCmd->cid, cTextureStates, (SVGA3dTextureState *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_SETMATERIAL:
                    {
                        SVGA3dCmdSetMaterial *pCmd = (SVGA3dCmdSetMaterial *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetMaterial);

                        rc = vmsvga3dSetMaterial(pThis, pCmd->cid, pCmd->face, &pCmd->material);
                        break;
                    }

                    case SVGA_3D_CMD_SETLIGHTDATA:
                    {
                        SVGA3dCmdSetLightData *pCmd = (SVGA3dCmdSetLightData *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetLightData);

                        rc = vmsvga3dSetLightData(pThis, pCmd->cid, pCmd->index, &pCmd->data);
                        break;
                    }

                    case SVGA_3D_CMD_SETLIGHTENABLED:
                    {
                        SVGA3dCmdSetLightEnabled *pCmd = (SVGA3dCmdSetLightEnabled *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetLightEnable);

                        rc = vmsvga3dSetLightEnabled(pThis, pCmd->cid, pCmd->index, pCmd->enabled);
                        break;
                    }

                    case SVGA_3D_CMD_SETVIEWPORT:
                    {
                        SVGA3dCmdSetViewport *pCmd = (SVGA3dCmdSetViewport *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetViewPort);

                        rc = vmsvga3dSetViewPort(pThis, pCmd->cid, &pCmd->rect);
                        break;
                    }

                    case SVGA_3D_CMD_SETCLIPPLANE:
                    {
                        SVGA3dCmdSetClipPlane *pCmd = (SVGA3dCmdSetClipPlane *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetClipPlane);

                        rc = vmsvga3dSetClipPlane(pThis, pCmd->cid, pCmd->index, pCmd->plane);
                        break;
                    }

                    case SVGA_3D_CMD_CLEAR:
                    {
                        SVGA3dCmdClear  *pCmd = (SVGA3dCmdClear *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dClear);

                        uint32_t cRects = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dRect);
                        rc = vmsvga3dCommandClear(pThis, pCmd->cid, pCmd->clearFlag, pCmd->color, pCmd->depth, pCmd->stencil, cRects, (SVGA3dRect *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_PRESENT:
                    case SVGA_3D_CMD_PRESENT_READBACK: /** @todo SVGA_3D_CMD_PRESENT_READBACK isn't quite the same as present... */
                    {
                        SVGA3dCmdPresent *pCmd = (SVGA3dCmdPresent *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        if ((unsigned)enmCmdId == SVGA_3D_CMD_PRESENT)
                            STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dPresent);
                        else
                            STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dPresentReadBack);

                        uint32_t cRects = (pHdr->size - sizeof(*pCmd)) / sizeof(SVGA3dCopyRect);

                        STAM_PROFILE_START(&pSVGAState->StatR3Cmd3dPresentProf, a);
                        rc = vmsvga3dCommandPresent(pThis, pCmd->sid, cRects, (SVGA3dCopyRect *)(pCmd + 1));
                        STAM_PROFILE_STOP(&pSVGAState->StatR3Cmd3dPresentProf, a);
                        break;
                    }

                    case SVGA_3D_CMD_SHADER_DEFINE:
                    {
                        SVGA3dCmdDefineShader *pCmd = (SVGA3dCmdDefineShader *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dShaderDefine);

                        uint32_t cbData = (pHdr->size - sizeof(*pCmd));
                        rc = vmsvga3dShaderDefine(pThis, pCmd->cid, pCmd->shid, pCmd->type, cbData, (uint32_t *)(pCmd + 1));
                        break;
                    }

                    case SVGA_3D_CMD_SHADER_DESTROY:
                    {
                        SVGA3dCmdDestroyShader *pCmd = (SVGA3dCmdDestroyShader *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dShaderDestroy);

                        rc = vmsvga3dShaderDestroy(pThis, pCmd->cid, pCmd->shid, pCmd->type);
                        break;
                    }

                    case SVGA_3D_CMD_SET_SHADER:
                    {
                        SVGA3dCmdSetShader *pCmd = (SVGA3dCmdSetShader *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetShader);

                        rc = vmsvga3dShaderSet(pThis, NULL, pCmd->cid, pCmd->type, pCmd->shid);
                        break;
                    }

                    case SVGA_3D_CMD_SET_SHADER_CONST:
                    {
                        SVGA3dCmdSetShaderConst *pCmd = (SVGA3dCmdSetShaderConst *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetShaderConst);

                        uint32_t cRegisters = (pHdr->size - sizeof(*pCmd)) / sizeof(pCmd->values) + 1;
                        rc = vmsvga3dShaderSetConst(pThis, pCmd->cid, pCmd->reg, pCmd->type, pCmd->ctype, cRegisters, pCmd->values);
                        break;
                    }

                    case SVGA_3D_CMD_DRAW_PRIMITIVES:
                    {
                        SVGA3dCmdDrawPrimitives *pCmd = (SVGA3dCmdDrawPrimitives *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dDrawPrimitives);

                        AssertBreak(pCmd->numRanges <= SVGA3D_MAX_DRAW_PRIMITIVE_RANGES);
                        AssertBreak(pCmd->numVertexDecls <= SVGA3D_MAX_VERTEX_ARRAYS);
                        uint32_t const cbRangesAndVertexDecls = pCmd->numVertexDecls * sizeof(SVGA3dVertexDecl)
                                                              + pCmd->numRanges * sizeof(SVGA3dPrimitiveRange);
                        ASSERT_GUEST_BREAK(cbRangesAndVertexDecls <= pHdr->size - sizeof(*pCmd));

                        uint32_t cVertexDivisor = (pHdr->size - sizeof(*pCmd) - cbRangesAndVertexDecls) / sizeof(uint32_t);
                        AssertBreak(!cVertexDivisor || cVertexDivisor == pCmd->numVertexDecls);

                        RT_UNTRUSTED_VALIDATED_FENCE();

                        SVGA3dVertexDecl     *pVertexDecl    = (SVGA3dVertexDecl *)(pCmd + 1);
                        SVGA3dPrimitiveRange *pNumRange      = (SVGA3dPrimitiveRange *)&pVertexDecl[pCmd->numVertexDecls];
                        SVGA3dVertexDivisor  *pVertexDivisor = cVertexDivisor ? (SVGA3dVertexDivisor *)&pNumRange[pCmd->numRanges] : NULL;

                        STAM_PROFILE_START(&pSVGAState->StatR3Cmd3dDrawPrimitivesProf, a);
                        rc = vmsvga3dDrawPrimitives(pThis, pCmd->cid, pCmd->numVertexDecls, pVertexDecl, pCmd->numRanges,
                                                    pNumRange, cVertexDivisor, pVertexDivisor);
                        STAM_PROFILE_STOP(&pSVGAState->StatR3Cmd3dDrawPrimitivesProf, a);
                        break;
                    }

                    case SVGA_3D_CMD_SETSCISSORRECT:
                    {
                        SVGA3dCmdSetScissorRect *pCmd = (SVGA3dCmdSetScissorRect *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dSetScissorRect);

                        rc = vmsvga3dSetScissorRect(pThis, pCmd->cid, &pCmd->rect);
                        break;
                    }

                    case SVGA_3D_CMD_BEGIN_QUERY:
                    {
                        SVGA3dCmdBeginQuery *pCmd = (SVGA3dCmdBeginQuery *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dBeginQuery);

                        rc = vmsvga3dQueryBegin(pThis, pCmd->cid, pCmd->type);
                        break;
                    }

                    case SVGA_3D_CMD_END_QUERY:
                    {
                        SVGA3dCmdEndQuery *pCmd = (SVGA3dCmdEndQuery *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dEndQuery);

                        rc = vmsvga3dQueryEnd(pThis, pCmd->cid, pCmd->type, pCmd->guestResult);
                        break;
                    }

                    case SVGA_3D_CMD_WAIT_FOR_QUERY:
                    {
                        SVGA3dCmdWaitForQuery *pCmd = (SVGA3dCmdWaitForQuery *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dWaitForQuery);

                        rc = vmsvga3dQueryWait(pThis, pCmd->cid, pCmd->type, pCmd->guestResult);
                        break;
                    }

                    case SVGA_3D_CMD_GENERATE_MIPMAPS:
                    {
                        SVGA3dCmdGenerateMipmaps *pCmd = (SVGA3dCmdGenerateMipmaps *)(pHdr + 1);
                        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dGenerateMipmaps);

                        rc = vmsvga3dGenerateMipmaps(pThis, pCmd->sid, pCmd->filter);
                        break;
                    }

                    case SVGA_3D_CMD_ACTIVATE_SURFACE:
                        /* context id + surface id? */
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dActivateSurface);
                        break;
                    case SVGA_3D_CMD_DEACTIVATE_SURFACE:
                        /* context id + surface id? */
                        STAM_REL_COUNTER_INC(&pSVGAState->StatR3Cmd3dDeactivateSurface);
                        break;

                    default:
                        STAM_REL_COUNTER_INC(&pSVGAState->StatFifoUnkCmds);
                        AssertMsgFailed(("enmCmdId=%d\n", enmCmdId));
                        break;
                    }
                }
                else
# endif // VBOX_WITH_VMSVGA3D
                {
                    STAM_REL_COUNTER_INC(&pSVGAState->StatFifoUnkCmds);
                    AssertMsgFailed(("enmCmdId=%d\n", enmCmdId));
                }
            }

            /* Go to the next slot */
            Assert(cbPayload + sizeof(uint32_t) <= offFifoMax - offFifoMin);
            offCurrentCmd += RT_ALIGN_32(cbPayload + sizeof(uint32_t), sizeof(uint32_t));
            if (offCurrentCmd >= offFifoMax)
            {
                offCurrentCmd -= offFifoMax - offFifoMin;
                Assert(offCurrentCmd >= offFifoMin);
                Assert(offCurrentCmd <  offFifoMax);
            }
            ASMAtomicWriteU32(&pFIFO[SVGA_FIFO_STOP], offCurrentCmd);
            STAM_REL_COUNTER_INC(&pSVGAState->StatFifoCommands);

            /*
             * Raise IRQ if required.  Must enter the critical section here
             * before making final decisions here, otherwise cubebench and
             * others may end up waiting forever.
             */
            if (   u32IrqStatus
                || (pThis->svga.u32IrqMask & SVGA_IRQFLAG_FIFO_PROGRESS))
            {
                int rc2 = PDMCritSectEnter(&pThis->CritSect, VERR_IGNORED);
                AssertRC(rc2);

                /* FIFO progress might trigger an interrupt. */
                if (pThis->svga.u32IrqMask & SVGA_IRQFLAG_FIFO_PROGRESS)
                {
                    Log(("vmsvgaFIFOLoop: fifo progress irq\n"));
                    u32IrqStatus |= SVGA_IRQFLAG_FIFO_PROGRESS;
                }

                /* Unmasked IRQ pending? */
                if (pThis->svga.u32IrqMask & u32IrqStatus)
                {
                    Log(("vmsvgaFIFOLoop: Trigger interrupt with status %x\n", u32IrqStatus));
                    ASMAtomicOrU32(&pThis->svga.u32IrqStatus, u32IrqStatus);
                    PDMDevHlpPCISetIrq(pDevIns, 0, 1);
                }

                PDMCritSectLeave(&pThis->CritSect);
            }
        }

        /* If really done, clear the busy flag. */
        if (fDone)
        {
            Log(("vmsvgaFIFOLoop: emptied the FIFO next=%x stop=%x\n", pFIFO[SVGA_FIFO_NEXT_CMD], offCurrentCmd));
            vmsvgaFifoSetNotBusy(pThis, pSVGAState, offFifoMin);
        }
    }

    /*
     * Free the bounce buffer. (There are no returns above!)
     */
    RTMemFree(pbBounceBuf);

    return VINF_SUCCESS;
}

/**
 * Free the specified GMR
 *
 * @param   pThis           VGA device instance data.
 * @param   idGMR           GMR id
 */
void vmsvgaGMRFree(PVGASTATE pThis, uint32_t idGMR)
{
    PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;

    /* Free the old descriptor if present. */
    PGMR pGMR = &pSVGAState->paGMR[idGMR];
    if (   pGMR->numDescriptors
        || pGMR->paDesc /* needed till we implement SVGA_REMAP_GMR2_VIA_GMR */)
    {
# ifdef DEBUG_GMR_ACCESS
        VMR3ReqCallWaitU(PDMDevHlpGetUVM(pThis->pDevInsR3), VMCPUID_ANY, (PFNRT)vmsvgaDeregisterGMR, 2, pThis->pDevInsR3, idGMR);
# endif

        Assert(pGMR->paDesc);
        RTMemFree(pGMR->paDesc);
        pGMR->paDesc         = NULL;
        pGMR->numDescriptors = 0;
        pGMR->cbTotal        = 0;
        pGMR->cMaxPages      = 0;
    }
    Assert(!pGMR->cMaxPages);
    Assert(!pGMR->cbTotal);
}

/**
 * Copy from a GMR to host memory or vice versa
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   enmTransferType Transfer type (read/write)
 * @param   pbDst           Host destination pointer
 * @param   cbDestPitch     Destination buffer pitch
 * @param   src             GMR description
 * @param   offSrc          Source buffer offset
 * @param   cbSrcPitch      Source buffer pitch
 * @param   cbWidth         Source width in bytes
 * @param   cHeight         Source height
 */
int vmsvgaGMRTransfer(PVGASTATE pThis, const SVGA3dTransferType enmTransferType, uint8_t *pbDst, int32_t cbDestPitch,
                      SVGAGuestPtr src, uint32_t offSrc, int32_t cbSrcPitch, uint32_t cbWidth, uint32_t cHeight)
{
    PVMSVGAR3STATE          pSVGAState = pThis->svga.pSvgaR3State;
    PGMR                    pGMR;
    int                     rc;
    PVMSVGAGMRDESCRIPTOR    pDesc;
    unsigned                offDesc = 0;

    Log(("vmsvgaGMRTransfer: gmr=%x offset=%x pitch=%d cbWidth=%d cHeight=%d; src offset=%d src pitch=%d\n",
         src.gmrId, src.offset, cbDestPitch, cbWidth, cHeight, offSrc, cbSrcPitch));
    Assert(cbWidth && cHeight);

    const uint32_t cbGmrScanline = cbSrcPitch > 0 ? cbSrcPitch : -cbSrcPitch;

    uint32_t cbGmrTotal; /* The GMR size in bytes. */
    if (src.gmrId == SVGA_GMR_FRAMEBUFFER)
    {
        pGMR = NULL;
        cbGmrTotal = pThis->vram_size;
    }
    else
    {
        AssertReturn(src.gmrId < pThis->svga.cGMR, VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();
        pGMR = &pSVGAState->paGMR[src.gmrId];
        cbGmrTotal = pGMR->cbTotal;
    }

    /* Check GMR parameters */
    AssertMsgReturn(src.offset < cbGmrTotal,
                    ("src.gmrId=%#x src.offset=%#x offSrc=%#x cbSrcPitch=%#x cHeight=%#x cbWidth=%#x cbGmrTotal=%#x\n",
                     src.gmrId, src.offset, offSrc, cbSrcPitch, cHeight, cbWidth, cbGmrTotal),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(offSrc < cbGmrTotal - src.offset,
                    ("src.gmrId=%#x src.offset=%#x offSrc=%#x cbSrcPitch=%#x cHeight=%#x cbWidth=%#x cbGmrTotal=%#x\n",
                     src.gmrId, src.offset, offSrc, cbSrcPitch, cHeight, cbWidth, cbGmrTotal),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(cbGmrScanline != 0,
                    ("src.gmrId=%#x src.offset=%#x offSrc=%#x cbSrcPitch=%#x cHeight=%#x cbWidth=%#x cbGmrTotal=%#x\n",
                     src.gmrId, src.offset, offSrc, cbSrcPitch, cHeight, cbWidth, cbGmrTotal),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(cbWidth <= cbGmrScanline,
                    ("src.gmrId=%#x src.offset=%#x offSrc=%#x cbSrcPitch=%#x cHeight=%#x cbWidth=%#x cbGmrTotal=%#x\n",
                     src.gmrId, src.offset, offSrc, cbSrcPitch, cHeight, cbWidth, cbGmrTotal),
                    VERR_INVALID_PARAMETER);

    offSrc += src.offset; /* Actual offset in the GMR, where the first scanline will be copied. */

    AssertMsgReturn(cbWidth <= cbGmrTotal - offSrc,
                    ("src.gmrId=%#x src.offset=%#x offSrc=%#x cbSrcPitch=%#x cHeight=%#x cbWidth=%#x cbGmrTotal=%#x\n",
                     src.gmrId, src.offset, offSrc, cbSrcPitch, cHeight, cbWidth, cbGmrTotal),
                    VERR_INVALID_PARAMETER);

    uint32_t cbGmrLeft = cbSrcPitch > 0 ? cbGmrTotal - offSrc : offSrc + cbWidth;

    uint32_t cGmrScanlines = cbGmrLeft / cbGmrScanline;
    uint32_t cbLastScanline = cbGmrLeft - cGmrScanlines * cbGmrScanline; /* Slack space. */
    if (cbWidth <= cbLastScanline)
        ++cGmrScanlines;

    if (cHeight > cGmrScanlines)
        cHeight = cGmrScanlines;

    AssertMsgReturn(cHeight > 0,
                    ("src.gmrId=%#x src.offset=%#x offSrc=%#x cbSrcPitch=%#x cHeight=%#x cbWidth=%#x cbGmrTotal=%#x\n",
                     src.gmrId, src.offset, offSrc, cbSrcPitch, cHeight, cbWidth, cbGmrTotal),
                    VERR_INVALID_PARAMETER);

    RT_UNTRUSTED_VALIDATED_FENCE();

    /* Shortcut for the framebuffer. */
    if (src.gmrId == SVGA_GMR_FRAMEBUFFER)
    {
        uint8_t *pSrc  = pThis->CTX_SUFF(vram_ptr) + offSrc;

        if (enmTransferType == SVGA3D_READ_HOST_VRAM)
        {
            /* switch src & dest */
            uint8_t *pTemp      = pbDst;
            int32_t cbTempPitch = cbDestPitch;

            pbDst = pSrc;
            pSrc  = pTemp;

            cbDestPitch = cbSrcPitch;
            cbSrcPitch  = cbTempPitch;
        }

        if (   pThis->svga.cbScanline == (uint32_t)cbDestPitch
            && cbWidth                == (uint32_t)cbDestPitch
            && cbSrcPitch             == cbDestPitch)
        {
            memcpy(pbDst, pSrc, cbWidth * cHeight);
        }
        else
        {
            for(uint32_t i = 0; i < cHeight; i++)
            {
                memcpy(pbDst, pSrc, cbWidth);

                pbDst += cbDestPitch;
                pSrc  += cbSrcPitch;
            }
        }
        return VINF_SUCCESS;
    }

    AssertPtrReturn(pGMR, VERR_INVALID_PARAMETER);
    pDesc = pGMR->paDesc;

    for (uint32_t i = 0; i < cHeight; i++)
    {
        uint32_t cbCurrentWidth = cbWidth;
        uint32_t offCurrent     = offSrc;
        uint8_t *pCurrentDest   = pbDst;

        /* Find the right descriptor */
        while (offDesc + pDesc->numPages * PAGE_SIZE <= offCurrent)
        {
            offDesc += pDesc->numPages * PAGE_SIZE;
            AssertReturn(offDesc < pGMR->cbTotal, VERR_INTERNAL_ERROR); /* overflow protection */
            pDesc++;
        }

        while (cbCurrentWidth)
        {
            uint32_t cbToCopy;

            if (offCurrent + cbCurrentWidth <= offDesc + pDesc->numPages * PAGE_SIZE)
            {
                cbToCopy = cbCurrentWidth;
            }
            else
            {
                cbToCopy = (offDesc + pDesc->numPages * PAGE_SIZE - offCurrent);
                AssertReturn(cbToCopy <= cbCurrentWidth, VERR_INVALID_PARAMETER);
            }

            LogFlow(("vmsvgaGMRTransfer: %s phys=%RGp\n", (enmTransferType == SVGA3D_WRITE_HOST_VRAM) ? "READ" : "WRITE", pDesc->GCPhys + offCurrent - offDesc));

            if (enmTransferType == SVGA3D_WRITE_HOST_VRAM)
                rc = PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), pDesc->GCPhys + offCurrent - offDesc, pCurrentDest, cbToCopy);
            else
                rc = PDMDevHlpPhysWrite(pThis->CTX_SUFF(pDevIns), pDesc->GCPhys + offCurrent - offDesc, pCurrentDest, cbToCopy);
            AssertRCBreak(rc);

            cbCurrentWidth -= cbToCopy;
            offCurrent     += cbToCopy;
            pCurrentDest   += cbToCopy;

            /* Go to the next descriptor if there's anything left. */
            if (cbCurrentWidth)
            {
                offDesc += pDesc->numPages * PAGE_SIZE;
                pDesc++;
            }
        }

        offSrc  += cbSrcPitch;
        pbDst   += cbDestPitch;
    }

    return VINF_SUCCESS;
}

/**
 * Unblock the FIFO I/O thread so it can respond to a state change.
 *
 * @returns VBox status code.
 * @param   pDevIns     The VGA device instance.
 * @param   pThread     The send thread.
 */
static DECLCALLBACK(int) vmsvgaFIFOLoopWakeUp(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    RT_NOREF(pDevIns);
    PVGASTATE pThis = (PVGASTATE)pThread->pvUser;
    Log(("vmsvgaFIFOLoopWakeUp\n"));
    return SUPSemEventSignal(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem);
}

/**
 * Enables or disables dirty page tracking for the framebuffer
 *
 * @param   pThis           VGA device instance data.
 * @param   fTraces         Enable/disable traces
 */
static void vmsvgaSetTraces(PVGASTATE pThis, bool fTraces)
{
    if (    (!pThis->svga.fConfigured || !pThis->svga.fEnabled)
        &&  !fTraces)
    {
        //Assert(pThis->svga.fTraces);
        Log(("vmsvgaSetTraces: *not* allowed to disable dirty page tracking when the device is in legacy mode.\n"));
        return;
    }

    pThis->svga.fTraces = fTraces;
    if (pThis->svga.fTraces)
    {
        unsigned cbFrameBuffer = pThis->vram_size;

        Log(("vmsvgaSetTraces: enable dirty page handling for the frame buffer only (%x bytes)\n", 0));
        if (pThis->svga.uHeight != VMSVGA_VAL_UNINITIALIZED)
        {
#ifndef DEBUG_bird /* BB-10.3.1 triggers this as it initializes everything to zero. Better just ignore it. */
            Assert(pThis->svga.cbScanline);
#endif
            /* Hardware enabled; return real framebuffer size .*/
            cbFrameBuffer = (uint32_t)pThis->svga.uHeight * pThis->svga.cbScanline;
            cbFrameBuffer = RT_ALIGN(cbFrameBuffer, PAGE_SIZE);
        }

        if (!pThis->svga.fVRAMTracking)
        {
            Log(("vmsvgaSetTraces: enable frame buffer dirty page tracking. (%x bytes; vram %x)\n", cbFrameBuffer, pThis->vram_size));
            vgaR3RegisterVRAMHandler(pThis, cbFrameBuffer);
            pThis->svga.fVRAMTracking = true;
        }
    }
    else
    {
        if (pThis->svga.fVRAMTracking)
        {
            Log(("vmsvgaSetTraces: disable frame buffer dirty page tracking\n"));
            vgaR3UnregisterVRAMHandler(pThis);
            pThis->svga.fVRAMTracking = false;
        }
    }
}

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
DECLCALLBACK(int) vmsvgaR3IORegionMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                      RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    int         rc;
    PVGASTATE   pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);

    Log(("vgasvgaR3IORegionMap: iRegion=%d GCPhysAddress=%RGp cb=%RGp enmType=%d\n", iRegion, GCPhysAddress, cb, enmType));
    if (enmType == PCI_ADDRESS_SPACE_IO)
    {
        AssertReturn(iRegion == 0, VERR_INTERNAL_ERROR);
        rc = PDMDevHlpIOPortRegister(pDevIns, (RTIOPORT)GCPhysAddress, cb, 0,
                                     vmsvgaIOWrite, vmsvgaIORead, NULL /* OutStr */, NULL /* InStr */, "VMSVGA");
        if (RT_FAILURE(rc))
            return rc;
        if (pThis->fR0Enabled)
        {
            rc = PDMDevHlpIOPortRegisterR0(pDevIns, (RTIOPORT)GCPhysAddress, cb, 0,
                                           "vmsvgaIOWrite", "vmsvgaIORead", NULL, NULL, "VMSVGA");
            if (RT_FAILURE(rc))
                return rc;
        }
        if (pThis->fGCEnabled)
        {
            rc = PDMDevHlpIOPortRegisterRC(pDevIns, (RTIOPORT)GCPhysAddress, cb, 0,
                                           "vmsvgaIOWrite", "vmsvgaIORead", NULL, NULL, "VMSVGA");
            if (RT_FAILURE(rc))
                return rc;
        }

        pThis->svga.BasePort = GCPhysAddress;
        Log(("vmsvgaR3IORegionMap: base port = %x\n", pThis->svga.BasePort));
    }
    else
    {
        AssertReturn(iRegion == 2 && enmType == PCI_ADDRESS_SPACE_MEM, VERR_INTERNAL_ERROR);
        if (GCPhysAddress != NIL_RTGCPHYS)
        {
            /*
             * Mapping the FIFO RAM.
             */
            AssertLogRelMsg(cb == pThis->svga.cbFIFO, ("cb=%#RGp cbFIFO=%#x\n", cb, pThis->svga.cbFIFO));
            rc = PDMDevHlpMMIOExMap(pDevIns, pPciDev, iRegion, GCPhysAddress);
            AssertRC(rc);

# ifdef DEBUG_FIFO_ACCESS
            if (RT_SUCCESS(rc))
            {
                rc = PGMHandlerPhysicalRegister(PDMDevHlpGetVM(pDevIns), GCPhysAddress, GCPhysAddress + (pThis->svga.cbFIFO - 1),
                                                pThis->svga.hFifoAccessHandlerType, pThis, NIL_RTR0PTR, NIL_RTRCPTR,
                                                "VMSVGA FIFO");
                AssertRC(rc);
            }
# endif
            if (RT_SUCCESS(rc))
            {
                pThis->svga.GCPhysFIFO = GCPhysAddress;
                Log(("vmsvgaR3IORegionMap: GCPhysFIFO=%RGp cbFIFO=%#x\n", GCPhysAddress, pThis->svga.cbFIFO));
            }
        }
        else
        {
            Assert(pThis->svga.GCPhysFIFO);
# ifdef DEBUG_FIFO_ACCESS
            rc = PGMHandlerPhysicalDeregister(PDMDevHlpGetVM(pDevIns), pThis->svga.GCPhysFIFO);
            AssertRC(rc);
# endif
            pThis->svga.GCPhysFIFO = 0;
        }

    }
    return VINF_SUCCESS;
}

# ifdef VBOX_WITH_VMSVGA3D

/**
 * Used by vmsvga3dInfoSurfaceWorker to make the FIFO thread to save one or all
 * surfaces to VMSVGA3DMIPMAPLEVEL::pSurfaceData heap buffers.
 *
 * @param   pThis               The VGA device instance data.
 * @param   sid                 Either UINT32_MAX or the ID of a specific
 *                              surface.  If UINT32_MAX is used, all surfaces
 *                              are processed.
 */
void vmsvga3dSurfaceUpdateHeapBuffersOnFifoThread(PVGASTATE pThis, uint32_t sid)
{
    vmsvgaR3RunExtCmdOnFifoThread(pThis, VMSVGA_FIFO_EXTCMD_UPDATE_SURFACE_HEAP_BUFFERS, (void *)(uintptr_t)sid,
                                  sid == UINT32_MAX ? 10 * RT_MS_1SEC : RT_MS_1MIN);
}


/**
 * @callback_method_impl{FNDBGFHANDLERDEV, "vmsvga3dsfc"}
 */
DECLCALLBACK(void) vmsvgaR3Info3dSurface(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    /* There might be a specific context ID at the start of the
       arguments, if not show all contexts. */
    uint32_t cid = UINT32_MAX;
    if (pszArgs)
        pszArgs = RTStrStripL(pszArgs);
    if (pszArgs && RT_C_IS_DIGIT(*pszArgs))
        cid = RTStrToUInt32(pszArgs);

    /* Verbose or terse display, we default to verbose. */
    bool fVerbose = true;
    if (RTStrIStr(pszArgs, "terse"))
        fVerbose = false;

    /* The size of the ascii art (x direction, y is 3/4 of x). */
    uint32_t cxAscii = 80;
    if (RTStrIStr(pszArgs, "gigantic"))
        cxAscii = 300;
    else if (RTStrIStr(pszArgs, "huge"))
        cxAscii = 180;
    else if (RTStrIStr(pszArgs, "big"))
        cxAscii = 132;
    else if (RTStrIStr(pszArgs, "normal"))
        cxAscii = 80;
    else if (RTStrIStr(pszArgs, "medium"))
        cxAscii = 64;
    else if (RTStrIStr(pszArgs, "small"))
        cxAscii = 48;
    else if (RTStrIStr(pszArgs, "tiny"))
        cxAscii = 24;

    /* Y invert the image when producing the ASCII art. */
    bool fInvY = false;
    if (RTStrIStr(pszArgs, "invy"))
        fInvY = true;

    vmsvga3dInfoSurfaceWorker(PDMINS_2_DATA(pDevIns, PVGASTATE), pHlp, cid, fVerbose, cxAscii, fInvY);
}


/**
 * @callback_method_impl{FNDBGFHANDLERDEV, "vmsvga3dctx"}
 */
DECLCALLBACK(void) vmsvgaR3Info3dContext(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    /* There might be a specific surface ID at the start of the
       arguments, if not show all contexts. */
    uint32_t sid = UINT32_MAX;
    if (pszArgs)
        pszArgs = RTStrStripL(pszArgs);
    if (pszArgs && RT_C_IS_DIGIT(*pszArgs))
        sid = RTStrToUInt32(pszArgs);

    /* Verbose or terse display, we default to verbose. */
    bool fVerbose = true;
    if (RTStrIStr(pszArgs, "terse"))
        fVerbose = false;

    vmsvga3dInfoContextWorker(PDMINS_2_DATA(pDevIns, PVGASTATE), pHlp, sid, fVerbose);
}

# endif /* VBOX_WITH_VMSVGA3D */

/**
 * @callback_method_impl{FNDBGFHANDLERDEV, "vmsvga"}
 */
static DECLCALLBACK(void) vmsvgaR3Info(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pszArgs);
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;

    pHlp->pfnPrintf(pHlp, "Extension enabled:  %RTbool\n", pThis->svga.fEnabled);
    pHlp->pfnPrintf(pHlp, "Configured:         %RTbool\n", pThis->svga.fConfigured);
    pHlp->pfnPrintf(pHlp, "Base I/O port:      %#x\n", pThis->svga.BasePort);
    pHlp->pfnPrintf(pHlp, "FIFO address:       %RGp\n", pThis->svga.GCPhysFIFO);
    pHlp->pfnPrintf(pHlp, "FIFO size:          %u (%#x)\n", pThis->svga.cbFIFO, pThis->svga.cbFIFO);
    pHlp->pfnPrintf(pHlp, "FIFO external cmd:  %#x\n", pThis->svga.u8FIFOExtCommand);
    pHlp->pfnPrintf(pHlp, "FIFO extcmd wakeup: %u\n", pThis->svga.fFifoExtCommandWakeup);
    pHlp->pfnPrintf(pHlp, "Busy:               %#x\n", pThis->svga.fBusy);
    pHlp->pfnPrintf(pHlp, "Traces:             %RTbool (effective: %RTbool)\n", pThis->svga.fTraces, pThis->svga.fVRAMTracking);
    pHlp->pfnPrintf(pHlp, "Guest ID:           %#x (%d)\n", pThis->svga.u32GuestId, pThis->svga.u32GuestId);
    pHlp->pfnPrintf(pHlp, "IRQ status:         %#x\n", pThis->svga.u32IrqStatus);
    pHlp->pfnPrintf(pHlp, "IRQ mask:           %#x\n", pThis->svga.u32IrqMask);
    pHlp->pfnPrintf(pHlp, "Pitch lock:         %#x\n", pThis->svga.u32PitchLock);
    pHlp->pfnPrintf(pHlp, "Current GMR ID:     %#x\n", pThis->svga.u32CurrentGMRId);
    pHlp->pfnPrintf(pHlp, "Capabilites reg:    %#x\n", pThis->svga.u32RegCaps);
    pHlp->pfnPrintf(pHlp, "Index reg:          %#x\n", pThis->svga.u32IndexReg);
    pHlp->pfnPrintf(pHlp, "Action flags:       %#x\n", pThis->svga.u32ActionFlags);
    pHlp->pfnPrintf(pHlp, "Max display size:   %ux%u\n", pThis->svga.u32MaxWidth, pThis->svga.u32MaxHeight);
    pHlp->pfnPrintf(pHlp, "Display size:       %ux%u %ubpp\n", pThis->svga.uWidth, pThis->svga.uHeight, pThis->svga.uBpp);
    pHlp->pfnPrintf(pHlp, "Scanline:           %u (%#x)\n", pThis->svga.cbScanline, pThis->svga.cbScanline);
    pHlp->pfnPrintf(pHlp, "Viewport position:  %ux%u\n", pThis->svga.viewport.x, pThis->svga.viewport.y);
    pHlp->pfnPrintf(pHlp, "Viewport size:      %ux%u\n", pThis->svga.viewport.cx, pThis->svga.viewport.cy);

    pHlp->pfnPrintf(pHlp, "Cursor active:      %RTbool\n", pSVGAState->Cursor.fActive);
    pHlp->pfnPrintf(pHlp, "Cursor hotspot:     %ux%u\n", pSVGAState->Cursor.xHotspot, pSVGAState->Cursor.yHotspot);
    pHlp->pfnPrintf(pHlp, "Cursor size:        %ux%u\n", pSVGAState->Cursor.width, pSVGAState->Cursor.height);
    pHlp->pfnPrintf(pHlp, "Cursor byte size:   %u (%#x)\n", pSVGAState->Cursor.cbData, pSVGAState->Cursor.cbData);

# ifdef VBOX_WITH_VMSVGA3D
    pHlp->pfnPrintf(pHlp, "3D enabled:         %RTbool\n", pThis->svga.f3DEnabled);
    pHlp->pfnPrintf(pHlp, "Host windows ID:    %#RX64\n", pThis->svga.u64HostWindowId);
    if (pThis->svga.u64HostWindowId != 0)
        vmsvga3dInfoHostWindow(pHlp, pThis->svga.u64HostWindowId);
# endif
}


/**
 * @copydoc FNSSMDEVLOADEXEC
 */
int vmsvgaLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    RT_NOREF(uPass);
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;
    int             rc;

    /* Load our part of the VGAState */
    rc = SSMR3GetStructEx(pSSM, &pThis->svga, sizeof(pThis->svga), 0, g_aVGAStateSVGAFields, NULL);
    AssertRCReturn(rc, rc);

    /* Load the VGA framebuffer. */
    AssertCompile(VMSVGA_VGA_FB_BACKUP_SIZE >= _32K);
    uint32_t cbVgaFramebuffer = _32K;
    if (uVersion >= VGA_SAVEDSTATE_VERSION_VMSVGA_VGA_FB_FIX)
    {
        rc = SSMR3GetU32(pSSM, &cbVgaFramebuffer);
        AssertRCReturn(rc, rc);
        AssertLogRelMsgReturn(cbVgaFramebuffer <= _4M && cbVgaFramebuffer >= _32K && RT_IS_POWER_OF_TWO(cbVgaFramebuffer),
                              ("cbVgaFramebuffer=%#x - expected 32KB..4MB, power of two\n", cbVgaFramebuffer),
                              VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
        AssertCompile(VMSVGA_VGA_FB_BACKUP_SIZE <= _4M);
        AssertCompile(RT_IS_POWER_OF_TWO(VMSVGA_VGA_FB_BACKUP_SIZE));
    }
    rc = SSMR3GetMem(pSSM, pThis->svga.pbVgaFrameBufferR3, RT_MIN(cbVgaFramebuffer, VMSVGA_VGA_FB_BACKUP_SIZE));
    AssertRCReturn(rc, rc);
    if (cbVgaFramebuffer > VMSVGA_VGA_FB_BACKUP_SIZE)
        SSMR3Skip(pSSM, cbVgaFramebuffer - VMSVGA_VGA_FB_BACKUP_SIZE);
    else if (cbVgaFramebuffer < VMSVGA_VGA_FB_BACKUP_SIZE)
        RT_BZERO(&pThis->svga.pbVgaFrameBufferR3[cbVgaFramebuffer], VMSVGA_VGA_FB_BACKUP_SIZE - cbVgaFramebuffer);

    /* Load the VMSVGA state. */
    rc = SSMR3GetStructEx(pSSM, pSVGAState, sizeof(*pSVGAState), 0, g_aVMSVGAR3STATEFields, NULL);
    AssertRCReturn(rc, rc);

    /* Load the active cursor bitmaps. */
    if (pSVGAState->Cursor.fActive)
    {
        pSVGAState->Cursor.pData = RTMemAlloc(pSVGAState->Cursor.cbData);
        AssertReturn(pSVGAState->Cursor.pData, VERR_NO_MEMORY);

        rc = SSMR3GetMem(pSSM, pSVGAState->Cursor.pData, pSVGAState->Cursor.cbData);
        AssertRCReturn(rc, rc);
    }

    /* Load the GMR state. */
    uint32_t cGMR = 256; /* Hardcoded in previous saved state versions. */
    if (uVersion >= VGA_SAVEDSTATE_VERSION_VMSVGA_GMR_COUNT)
    {
        rc = SSMR3GetU32(pSSM, &cGMR);
        AssertRCReturn(rc, rc);
        /* Numbers of GMRs was never less than 256. 1MB is a large arbitrary limit. */
        AssertLogRelMsgReturn(cGMR <= _1M && cGMR >= 256,
                              ("cGMR=%#x - expected 256B..1MB\n", cGMR),
                              VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    }

    if (pThis->svga.cGMR != cGMR)
    {
        /* Reallocate GMR array. */
        Assert(pSVGAState->paGMR != NULL);
        RTMemFree(pSVGAState->paGMR);
        pSVGAState->paGMR = (PGMR)RTMemAllocZ(cGMR * sizeof(GMR));
        AssertReturn(pSVGAState->paGMR, VERR_NO_MEMORY);
        pThis->svga.cGMR = cGMR;
    }

    for (uint32_t i = 0; i < cGMR; ++i)
    {
        PGMR pGMR = &pSVGAState->paGMR[i];

        rc = SSMR3GetStructEx(pSSM, pGMR, sizeof(*pGMR), 0, g_aGMRFields, NULL);
        AssertRCReturn(rc, rc);

        if (pGMR->numDescriptors)
        {
            Assert(pGMR->cMaxPages || pGMR->cbTotal);
            pGMR->paDesc = (PVMSVGAGMRDESCRIPTOR)RTMemAllocZ(pGMR->numDescriptors * sizeof(VMSVGAGMRDESCRIPTOR));
            AssertReturn(pGMR->paDesc, VERR_NO_MEMORY);

            for (uint32_t j = 0; j < pGMR->numDescriptors; ++j)
            {
                rc = SSMR3GetStructEx(pSSM, &pGMR->paDesc[j], sizeof(pGMR->paDesc[j]), 0, g_aVMSVGAGMRDESCRIPTORFields, NULL);
                AssertRCReturn(rc, rc);
            }
        }
    }

# ifdef VBOX_WITH_VMSVGA3D
    if (pThis->svga.f3DEnabled)
    {
#  ifdef RT_OS_DARWIN  /** @todo r=bird: this is normally done on the EMT, so for DARWIN we do that when loading saved state too now. See DevVGA-SVGA3d-shared.h. */
        vmsvga3dPowerOn(pThis);
#  endif

        VMSVGA_STATE_LOAD LoadState;
        LoadState.pSSM     = pSSM;
        LoadState.uVersion = uVersion;
        LoadState.uPass    = uPass;
        rc = vmsvgaR3RunExtCmdOnFifoThread(pThis, VMSVGA_FIFO_EXTCMD_LOADSTATE, &LoadState, RT_INDEFINITE_WAIT);
        AssertLogRelRCReturn(rc, rc);
    }
# endif

    return VINF_SUCCESS;
}

/**
 * Reinit the video mode after the state has been loaded.
 */
int vmsvgaLoadDone(PPDMDEVINS pDevIns)
{
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;

    pThis->last_bpp = VMSVGA_VAL_UNINITIALIZED;   /* force mode reset */
    vmsvgaChangeMode(pThis);

    /* Set the active cursor. */
    if (pSVGAState->Cursor.fActive)
    {
        int rc;

        rc = pThis->pDrv->pfnVBVAMousePointerShape(pThis->pDrv,
                                                   true,
                                                   true,
                                                   pSVGAState->Cursor.xHotspot,
                                                   pSVGAState->Cursor.yHotspot,
                                                   pSVGAState->Cursor.width,
                                                   pSVGAState->Cursor.height,
                                                   pSVGAState->Cursor.pData);
        AssertRC(rc);
    }
    return VINF_SUCCESS;
}

/**
 * @copydoc FNSSMDEVSAVEEXEC
 */
int vmsvgaSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;
    int             rc;

    /* Save our part of the VGAState */
    rc = SSMR3PutStructEx(pSSM, &pThis->svga, sizeof(pThis->svga), 0, g_aVGAStateSVGAFields, NULL);
    AssertLogRelRCReturn(rc, rc);

    /* Save the framebuffer backup. */
    rc = SSMR3PutU32(pSSM, VMSVGA_VGA_FB_BACKUP_SIZE);
    rc = SSMR3PutMem(pSSM, pThis->svga.pbVgaFrameBufferR3, VMSVGA_VGA_FB_BACKUP_SIZE);
    AssertLogRelRCReturn(rc, rc);

    /* Save the VMSVGA state. */
    rc = SSMR3PutStructEx(pSSM, pSVGAState, sizeof(*pSVGAState), 0, g_aVMSVGAR3STATEFields, NULL);
    AssertLogRelRCReturn(rc, rc);

    /* Save the active cursor bitmaps. */
    if (pSVGAState->Cursor.fActive)
    {
        rc = SSMR3PutMem(pSSM, pSVGAState->Cursor.pData, pSVGAState->Cursor.cbData);
        AssertLogRelRCReturn(rc, rc);
    }

    /* Save the GMR state */
    rc = SSMR3PutU32(pSSM, pThis->svga.cGMR);
    AssertLogRelRCReturn(rc, rc);
    for (uint32_t i = 0; i < pThis->svga.cGMR; ++i)
    {
        PGMR pGMR = &pSVGAState->paGMR[i];

        rc = SSMR3PutStructEx(pSSM, pGMR, sizeof(*pGMR), 0, g_aGMRFields, NULL);
        AssertLogRelRCReturn(rc, rc);

        for (uint32_t j = 0; j < pGMR->numDescriptors; ++j)
        {
            rc = SSMR3PutStructEx(pSSM, &pGMR->paDesc[j], sizeof(pGMR->paDesc[j]), 0, g_aVMSVGAGMRDESCRIPTORFields, NULL);
            AssertLogRelRCReturn(rc, rc);
        }
    }

# ifdef VBOX_WITH_VMSVGA3D
    /*
     * Must save the 3d state in the FIFO thread.
     */
    if (pThis->svga.f3DEnabled)
    {
        rc = vmsvgaR3RunExtCmdOnFifoThread(pThis, VMSVGA_FIFO_EXTCMD_SAVESTATE, pSSM, RT_INDEFINITE_WAIT);
        AssertLogRelRCReturn(rc, rc);
    }
# endif
    return VINF_SUCCESS;
}

/**
 * Resets the SVGA hardware state
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 */
int vmsvgaReset(PPDMDEVINS pDevIns)
{
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState = pThis->svga.pSvgaR3State;

    /* Reset before init? */
    if (!pSVGAState)
        return VINF_SUCCESS;

    Log(("vmsvgaReset\n"));


    /* Reset the FIFO processing as well as the 3d state (if we have one). */
    pThis->svga.pFIFOR3[SVGA_FIFO_NEXT_CMD] = pThis->svga.pFIFOR3[SVGA_FIFO_STOP] = 0; /** @todo should probably let the FIFO thread do this ... */
    int rc = vmsvgaR3RunExtCmdOnFifoThread(pThis, VMSVGA_FIFO_EXTCMD_RESET, NULL /*pvParam*/, 10000 /*ms*/);

    /* Reset other stuff. */
    pThis->svga.cScratchRegion = VMSVGA_SCRATCH_SIZE;
    RT_ZERO(pThis->svga.au32ScratchRegion);
    RT_ZERO(*pThis->svga.pSvgaR3State);
    RT_BZERO(pThis->svga.pbVgaFrameBufferR3, VMSVGA_VGA_FB_BACKUP_SIZE);

    /* Register caps. */
    pThis->svga.u32RegCaps = SVGA_CAP_GMR | SVGA_CAP_GMR2 | SVGA_CAP_CURSOR | SVGA_CAP_CURSOR_BYPASS_2 | SVGA_CAP_EXTENDED_FIFO | SVGA_CAP_IRQMASK | SVGA_CAP_PITCHLOCK | SVGA_CAP_TRACES | SVGA_CAP_SCREEN_OBJECT_2 | SVGA_CAP_ALPHA_CURSOR;
# ifdef VBOX_WITH_VMSVGA3D
    pThis->svga.u32RegCaps |= SVGA_CAP_3D;
# endif

    /* Setup FIFO capabilities. */
    pThis->svga.pFIFOR3[SVGA_FIFO_CAPABILITIES] = SVGA_FIFO_CAP_FENCE | SVGA_FIFO_CAP_CURSOR_BYPASS_3 | SVGA_FIFO_CAP_GMR2 | SVGA_FIFO_CAP_3D_HWVERSION_REVISED | SVGA_FIFO_CAP_SCREEN_OBJECT_2;

    /* Valid with SVGA_FIFO_CAP_SCREEN_OBJECT_2 */
    pThis->svga.pFIFOR3[SVGA_FIFO_CURSOR_SCREEN_ID] = SVGA_ID_INVALID;

    /* VRAM tracking is enabled by default during bootup. */
    pThis->svga.fVRAMTracking = true;
    pThis->svga.fEnabled      = false;

    /* Invalidate current settings. */
    pThis->svga.uWidth     = VMSVGA_VAL_UNINITIALIZED;
    pThis->svga.uHeight    = VMSVGA_VAL_UNINITIALIZED;
    pThis->svga.uBpp       = VMSVGA_VAL_UNINITIALIZED;
    pThis->svga.cbScanline = 0;

    return rc;
}

/**
 * Cleans up the SVGA hardware state
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 */
int vmsvgaDestruct(PPDMDEVINS pDevIns)
{
    PVGASTATE pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);

    /*
     * Ask the FIFO thread to terminate the 3d state and then terminate it.
     */
    if (pThis->svga.pFIFOIOThread)
    {
        int rc = vmsvgaR3RunExtCmdOnFifoThread(pThis, VMSVGA_FIFO_EXTCMD_TERMINATE, NULL /*pvParam*/, 30000 /*ms*/);
        AssertLogRelRC(rc);

        rc = PDMR3ThreadDestroy(pThis->svga.pFIFOIOThread, NULL);
        AssertLogRelRC(rc);
        pThis->svga.pFIFOIOThread = NULL;
    }

    /*
     * Destroy the special SVGA state.
     */
    PVMSVGAR3STATE pSVGAState = pThis->svga.pSvgaR3State;
    if (pSVGAState)
    {
# ifndef VMSVGA_USE_EMT_HALT_CODE
        if (pSVGAState->hBusyDelayedEmts != NIL_RTSEMEVENTMULTI)
        {
            RTSemEventMultiDestroy(pSVGAState->hBusyDelayedEmts);
            pSVGAState->hBusyDelayedEmts = NIL_RTSEMEVENT;
        }
# endif
        if (pSVGAState->Cursor.fActive)
            RTMemFree(pSVGAState->Cursor.pData);

        if (pSVGAState->paGMR)
        {
            for (unsigned i = 0; i < pThis->svga.cGMR; ++i)
                if (pSVGAState->paGMR[i].paDesc)
                    RTMemFree(pSVGAState->paGMR[i].paDesc);

            RTMemFree(pSVGAState->paGMR);
            pSVGAState->paGMR = NULL;
        }

        RTMemFree(pSVGAState);
        pThis->svga.pSvgaR3State = NULL;
    }

    /*
     * Free our resources residing in the VGA state.
     */
    if (pThis->svga.pbVgaFrameBufferR3)
    {
        RTMemFree(pThis->svga.pbVgaFrameBufferR3);
        pThis->svga.pbVgaFrameBufferR3 = NULL;
    }
    if (pThis->svga.FIFOExtCmdSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(pThis->svga.FIFOExtCmdSem);
        pThis->svga.FIFOExtCmdSem = NIL_RTSEMEVENT;
    }
    if (pThis->svga.FIFORequestSem != NIL_SUPSEMEVENT)
    {
        SUPSemEventClose(pThis->svga.pSupDrvSession, pThis->svga.FIFORequestSem);
        pThis->svga.FIFORequestSem = NIL_SUPSEMEVENT;
    }

    return VINF_SUCCESS;
}

/**
 * Initialize the SVGA hardware state
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 */
int vmsvgaInit(PPDMDEVINS pDevIns)
{
    PVGASTATE       pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    PVMSVGAR3STATE  pSVGAState;
    PVM             pVM   = PDMDevHlpGetVM(pDevIns);
    int             rc;

    pThis->svga.cScratchRegion = VMSVGA_SCRATCH_SIZE;
    memset(pThis->svga.au32ScratchRegion, 0, sizeof(pThis->svga.au32ScratchRegion));

    pThis->svga.pSvgaR3State = (PVMSVGAR3STATE)RTMemAllocZ(sizeof(VMSVGAR3STATE));
    AssertReturn(pThis->svga.pSvgaR3State, VERR_NO_MEMORY);
    pSVGAState = pThis->svga.pSvgaR3State;

    pThis->svga.cGMR = VMSVGA_MAX_GMR_IDS;
    pSVGAState->paGMR = (PGMR)RTMemAllocZ(pThis->svga.cGMR * sizeof(GMR));
    AssertReturn(pSVGAState->paGMR, VERR_NO_MEMORY);

    /* Necessary for creating a backup of the text mode frame buffer when switching into svga mode. */
    pThis->svga.pbVgaFrameBufferR3 = (uint8_t *)RTMemAllocZ(VMSVGA_VGA_FB_BACKUP_SIZE);
    AssertReturn(pThis->svga.pbVgaFrameBufferR3, VERR_NO_MEMORY);

    /* Create event semaphore. */
    pThis->svga.pSupDrvSession = PDMDevHlpGetSupDrvSession(pDevIns);

    rc = SUPSemEventCreate(pThis->svga.pSupDrvSession, &pThis->svga.FIFORequestSem);
    if (RT_FAILURE(rc))
    {
        Log(("%s: Failed to create event semaphore for FIFO handling.\n", __FUNCTION__));
        return rc;
    }

    /* Create event semaphore. */
    rc = RTSemEventCreate(&pThis->svga.FIFOExtCmdSem);
    if (RT_FAILURE(rc))
    {
        Log(("%s: Failed to create event semaphore for external fifo cmd handling.\n", __FUNCTION__));
        return rc;
    }

# ifndef VMSVGA_USE_EMT_HALT_CODE
    /* Create semaphore for delaying EMTs wait for the FIFO to stop being busy. */
    rc = RTSemEventMultiCreate(&pSVGAState->hBusyDelayedEmts);
    AssertRCReturn(rc, rc);
# endif

    /* Register caps. */
    pThis->svga.u32RegCaps = SVGA_CAP_GMR | SVGA_CAP_GMR2 | SVGA_CAP_CURSOR | SVGA_CAP_CURSOR_BYPASS_2 | SVGA_CAP_EXTENDED_FIFO | SVGA_CAP_IRQMASK | SVGA_CAP_PITCHLOCK | SVGA_CAP_TRACES | SVGA_CAP_SCREEN_OBJECT_2 | SVGA_CAP_ALPHA_CURSOR;
# ifdef VBOX_WITH_VMSVGA3D
    pThis->svga.u32RegCaps |= SVGA_CAP_3D;
# endif

    /* Setup FIFO capabilities. */
    pThis->svga.pFIFOR3[SVGA_FIFO_CAPABILITIES] = SVGA_FIFO_CAP_FENCE | SVGA_FIFO_CAP_CURSOR_BYPASS_3 | SVGA_FIFO_CAP_GMR2 | SVGA_FIFO_CAP_3D_HWVERSION_REVISED | SVGA_FIFO_CAP_SCREEN_OBJECT_2;

    /* Valid with SVGA_FIFO_CAP_SCREEN_OBJECT_2 */
    pThis->svga.pFIFOR3[SVGA_FIFO_CURSOR_SCREEN_ID] = SVGA_ID_INVALID;

    pThis->svga.pFIFOR3[SVGA_FIFO_3D_HWVERSION] = pThis->svga.pFIFOR3[SVGA_FIFO_3D_HWVERSION_REVISED] = 0;    /* no 3d available. */
# ifdef VBOX_WITH_VMSVGA3D
    if (pThis->svga.f3DEnabled)
    {
        rc = vmsvga3dInit(pThis);
        if (RT_FAILURE(rc))
        {
            LogRel(("VMSVGA3d: 3D support disabled! (vmsvga3dInit -> %Rrc)\n", rc));
            pThis->svga.f3DEnabled = false;
        }
    }
# endif
    /* VRAM tracking is enabled by default during bootup. */
    pThis->svga.fVRAMTracking = true;

    /* Invalidate current settings. */
    pThis->svga.uWidth     = VMSVGA_VAL_UNINITIALIZED;
    pThis->svga.uHeight    = VMSVGA_VAL_UNINITIALIZED;
    pThis->svga.uBpp       = VMSVGA_VAL_UNINITIALIZED;
    pThis->svga.cbScanline = 0;

    pThis->svga.u32MaxWidth  = VBE_DISPI_MAX_YRES;
    pThis->svga.u32MaxHeight = VBE_DISPI_MAX_XRES;
    while (pThis->svga.u32MaxWidth * pThis->svga.u32MaxHeight * 4 /* 32 bpp */ > pThis->vram_size)
    {
        pThis->svga.u32MaxWidth  -= 256;
        pThis->svga.u32MaxHeight -= 256;
    }
    Log(("VMSVGA: Maximum size (%d,%d)\n", pThis->svga.u32MaxWidth, pThis->svga.u32MaxHeight));

# ifdef DEBUG_GMR_ACCESS
    /* Register the GMR access handler type. */
    rc = PGMR3HandlerPhysicalTypeRegister(PDMDevHlpGetVM(pThis->pDevInsR3), PGMPHYSHANDLERKIND_WRITE,
                                          vmsvgaR3GMRAccessHandler,
                                          NULL, NULL, NULL,
                                          NULL, NULL, NULL,
                                          "VMSVGA GMR", &pThis->svga.hGmrAccessHandlerType);
    AssertRCReturn(rc, rc);
# endif
# ifdef DEBUG_FIFO_ACCESS
    rc = PGMR3HandlerPhysicalTypeRegister(PDMDevHlpGetVM(pThis->pDevInsR3), PGMPHYSHANDLERKIND_ALL,
                                          vmsvgaR3FIFOAccessHandler,
                                          NULL, NULL, NULL,
                                          NULL, NULL, NULL,
                                          "VMSVGA FIFO", &pThis->svga.hFifoAccessHandlerType);
    AssertRCReturn(rc, rc);
#endif

    /* Create the async IO thread. */
    rc = PDMDevHlpThreadCreate(pDevIns, &pThis->svga.pFIFOIOThread, pThis, vmsvgaFIFOLoop, vmsvgaFIFOLoopWakeUp, 0,
                                RTTHREADTYPE_IO, "VMSVGA FIFO");
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("%s: Async IO Thread creation for FIFO handling failed rc=%d\n", __FUNCTION__, rc));
        return rc;
    }

    /*
     * Statistics.
     */
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dActivateSurface,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dActivateSurface",    STAMUNIT_OCCURENCES, "SVGA_3D_CMD_ACTIVATE_SURFACE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dBeginQuery,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dBeginQuery",         STAMUNIT_OCCURENCES, "SVGA_3D_CMD_BEGIN_QUERY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dClear,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dClear",              STAMUNIT_OCCURENCES, "SVGA_3D_CMD_CLEAR");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dContextDefine,        STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dContextDefine",      STAMUNIT_OCCURENCES, "SVGA_3D_CMD_CONTEXT_DEFINE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dContextDestroy,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dContextDestroy",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_CONTEXT_DESTROY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dDeactivateSurface,    STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dDeactivateSurface",  STAMUNIT_OCCURENCES, "SVGA_3D_CMD_DEACTIVATE_SURFACE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dDrawPrimitives,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dDrawPrimitives",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_DRAW_PRIMITIVES");
    STAM_REG(pVM,     &pSVGAState->StatR3Cmd3dDrawPrimitivesProf,   STAMTYPE_PROFILE, "/Devices/VMSVGA/Cmd/3dDrawPrimitivesProf", STAMUNIT_TICKS_PER_CALL, "Profiling of SVGA_3D_CMD_DRAW_PRIMITIVES.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dEndQuery,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dEndQuery",           STAMUNIT_OCCURENCES, "SVGA_3D_CMD_END_QUERY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dGenerateMipmaps,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dGenerateMipmaps",    STAMUNIT_OCCURENCES, "SVGA_3D_CMD_GENERATE_MIPMAPS");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dPresent,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dPresent",            STAMUNIT_OCCURENCES, "SVGA_3D_CMD_PRESENT");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dPresentReadBack,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dPresentReadBack",    STAMUNIT_OCCURENCES, "SVGA_3D_CMD_PRESENT_READBACK");
    STAM_REG(pVM,     &pSVGAState->StatR3Cmd3dPresentProf,          STAMTYPE_PROFILE, "/Devices/VMSVGA/Cmd/3dPresentProfBoth",    STAMUNIT_TICKS_PER_CALL, "Profiling of SVGA_3D_CMD_PRESENT and SVGA_3D_CMD_PRESENT_READBACK.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetClipPlane,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetClipPlane",       STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETCLIPPLANE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetLightData,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetLightData",       STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETLIGHTDATA");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetLightEnable,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetLightEnable",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETLIGHTENABLE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetMaterial,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetMaterial",        STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETMATERIAL");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetRenderState,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetRenderState",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETRENDERSTATE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetRenderTarget,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetRenderTarget",    STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETRENDERTARGET");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetScissorRect,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetScissorRect",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETSCISSORRECT");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetShader,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetShader",          STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SET_SHADER");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetShaderConst,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetShaderConst",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SET_SHADER_CONST");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetTextureState,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetTextureState",    STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETTEXTURESTATE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetTransform,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetTransform",       STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETTRANSFORM");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetViewPort,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetViewPort",        STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETVIEWPORT");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSetZRange,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSetZRange",          STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SETZRANGE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dShaderDefine,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dShaderDefine",       STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SHADER_DEFINE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dShaderDestroy,        STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dShaderDestroy",      STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SHADER_DESTROY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceCopy,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceCopy",        STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_COPY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceDefine,        STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceDefine",      STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_DEFINE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceDefineV2,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceDefineV2",    STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_DEFINE_V2");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceDestroy,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceDestroy",     STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_DESTROY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceDma,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceDma",         STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_DMA");
    STAM_REG(pVM,     &pSVGAState->StatR3Cmd3dSurfaceDmaProf,       STAMTYPE_PROFILE, "/Devices/VMSVGA/Cmd/3dSurfaceDmaProf",     STAMUNIT_TICKS_PER_CALL, "Profiling of SVGA_3D_CMD_SURFACE_DMA.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceScreen,        STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceScreen",      STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_SCREEN");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dSurfaceStretchBlt,    STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dSurfaceStretchBlt",  STAMUNIT_OCCURENCES, "SVGA_3D_CMD_SURFACE_STRETCHBLT");
    STAM_REL_REG(pVM, &pSVGAState->StatR3Cmd3dWaitForQuery,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/3dWaitForQuery",       STAMUNIT_OCCURENCES, "SVGA_3D_CMD_WAIT_FOR_QUERY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdAnnotationCopy,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/AnnotationCopy",       STAMUNIT_OCCURENCES, "SVGA_CMD_ANNOTATION_COPY");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdAnnotationFill,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/AnnotationFill",       STAMUNIT_OCCURENCES, "SVGA_CMD_ANNOTATION_FILL");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdBlitGmrFbToScreen,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/BlitGmrFbToScreen",    STAMUNIT_OCCURENCES, "SVGA_CMD_BLIT_GMRFB_TO_SCREEN");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdBlitScreentoGmrFb,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/BlitScreentoGmrFb",    STAMUNIT_OCCURENCES, "SVGA_CMD_BLIT_SCREEN_TO_GMRFB");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineAlphaCursor,      STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineAlphaCursor",    STAMUNIT_OCCURENCES, "SVGA_CMD_DEFINE_ALPHA_CURSOR");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineCursor,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineCursor",         STAMUNIT_OCCURENCES, "SVGA_CMD_DEFINE_CURSOR");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineGmr2,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineGmr2",           STAMUNIT_OCCURENCES, "SVGA_CMD_DEFINE_GMR2");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineGmr2Free,         STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineGmr2/Free",      STAMUNIT_OCCURENCES, "Number of SVGA_CMD_DEFINE_GMR2 commands that only frees.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineGmr2Modify,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineGmr2/Modify",    STAMUNIT_OCCURENCES, "Number of SVGA_CMD_DEFINE_GMR2 commands that redefines a non-free GMR.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineGmrFb,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineGmrFb",          STAMUNIT_OCCURENCES, "SVGA_CMD_DEFINE_GMRFB");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDefineScreen,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DefineScreen",         STAMUNIT_OCCURENCES, "SVGA_CMD_DEFINE_SCREEN");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdDestroyScreen,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/DestroyScreen",        STAMUNIT_OCCURENCES, "SVGA_CMD_DESTROY_SCREEN");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdEscape,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/Escape",               STAMUNIT_OCCURENCES, "SVGA_CMD_ESCAPE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdFence,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/Fence",                STAMUNIT_OCCURENCES, "SVGA_CMD_FENCE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdInvalidCmd,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/InvalidCmd",           STAMUNIT_OCCURENCES, "SVGA_CMD_INVALID_CMD");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdRemapGmr2,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/RemapGmr2",            STAMUNIT_OCCURENCES, "SVGA_CMD_REMAP_GMR2.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdRemapGmr2Modify,        STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/RemapGmr2/Modify",     STAMUNIT_OCCURENCES, "Number of SVGA_CMD_REMAP_GMR2 commands that modifies rather than complete the definition of a GMR.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdUpdate,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/Update",               STAMUNIT_OCCURENCES, "SVGA_CMD_UPATE");
    STAM_REL_REG(pVM, &pSVGAState->StatR3CmdUpdateVerbose,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Cmd/UpdateVerbose",        STAMUNIT_OCCURENCES, "SVGA_CMD_UPDATE_VERBOSE");

    STAM_REL_REG(pVM, &pSVGAState->StatR3RegConfigDoneWr,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/ConfigDoneWrite",            STAMUNIT_OCCURENCES, "SVGA_REG_CONFIG_DONE writes");
    STAM_REL_REG(pVM, &pSVGAState->StatR3RegGmrDescriptorWr,        STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrDescriptorWrite",         STAMUNIT_OCCURENCES, "SVGA_REG_GMR_DESCRIPTOR writes");
    STAM_REL_REG(pVM, &pSVGAState->StatR3RegGmrDescriptorWrErrors,  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrDescriptorWrite/Errors",  STAMUNIT_OCCURENCES, "Number of erroneous SVGA_REG_GMR_DESCRIPTOR commands.");
    STAM_REL_REG(pVM, &pSVGAState->StatR3RegGmrDescriptorWrFree,    STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrDescriptorWrite/Free",    STAMUNIT_OCCURENCES, "Number of SVGA_REG_GMR_DESCRIPTOR commands only freeing the GMR.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegBitsPerPixelWr,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/BitsPerPixelWrite",          STAMUNIT_OCCURENCES, "SVGA_REG_BITS_PER_PIXEL writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegBusyWr,                   STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/BusyWrite",                  STAMUNIT_OCCURENCES, "SVGA_REG_BUSY writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegCursorXxxxWr,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/CursorXxxxWrite",            STAMUNIT_OCCURENCES, "SVGA_REG_CURSOR_XXXX writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDepthWr,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DepthWrite",                 STAMUNIT_OCCURENCES, "SVGA_REG_DEPTH writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayHeightWr,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayHeightWrite",         STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_HEIGHT writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayIdWr,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayIdWrite",             STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_ID writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayIsPrimaryWr,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayIsPrimaryWrite",      STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_IS_PRIMARY writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayPositionXWr,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayPositionXWrite",      STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_POSITION_X writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayPositionYWr,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayPositionYWrite",      STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_POSITION_Y writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayWidthWr,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayWidthWrite",          STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_WIDTH writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegEnableWr,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/EnableWrite",                STAMUNIT_OCCURENCES, "SVGA_REG_ENABLE writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGmrIdWr,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrIdWrite",                 STAMUNIT_OCCURENCES, "SVGA_REG_GMR_ID writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGuestIdWr,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GuestIdWrite",               STAMUNIT_OCCURENCES, "SVGA_REG_GUEST_ID writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegHeightWr,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/HeightWrite",                STAMUNIT_OCCURENCES, "SVGA_REG_HEIGHT writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegIdWr,                     STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/IdWrite",                    STAMUNIT_OCCURENCES, "SVGA_REG_ID writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegIrqMaskWr,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/IrqMaskWrite",               STAMUNIT_OCCURENCES, "SVGA_REG_IRQMASK writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegNumDisplaysWr,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/NumDisplaysWrite",           STAMUNIT_OCCURENCES, "SVGA_REG_NUM_DISPLAYS writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegNumGuestDisplaysWr,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/NumGuestDisplaysWrite",      STAMUNIT_OCCURENCES, "SVGA_REG_NUM_GUEST_DISPLAYS writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegPaletteWr,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/PaletteWrite",               STAMUNIT_OCCURENCES, "SVGA_PALETTE_XXXX writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegPitchLockWr,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/PitchLockWrite",             STAMUNIT_OCCURENCES, "SVGA_REG_PITCHLOCK writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegPseudoColorWr,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/PseudoColorWrite",           STAMUNIT_OCCURENCES, "SVGA_REG_PSEUDOCOLOR writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegReadOnlyWr,               STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/ReadOnlyWrite",              STAMUNIT_OCCURENCES, "Read-only SVGA_REG_XXXX writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegScratchWr,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/ScratchWrite",               STAMUNIT_OCCURENCES, "SVGA_REG_SCRATCH_XXXX writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegSyncWr,                   STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/SyncWrite",                  STAMUNIT_OCCURENCES, "SVGA_REG_SYNC writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegTopWr,                    STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/TopWrite",                   STAMUNIT_OCCURENCES, "SVGA_REG_TOP writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegTracesWr,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/TracesWrite",                STAMUNIT_OCCURENCES, "SVGA_REG_TRACES writes.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegUnknownWr,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/UnknownWrite",               STAMUNIT_OCCURENCES, "Writes to unknown register.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegWidthWr,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/WidthWrite",                 STAMUNIT_OCCURENCES, "SVGA_REG_WIDTH writes.");

    STAM_REL_REG(pVM, &pThis->svga.StatRegBitsPerPixelRd,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/BitsPerPixelRead",           STAMUNIT_OCCURENCES, "SVGA_REG_BITS_PER_PIXEL reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegBlueMaskRd,               STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/BlueMaskRead",               STAMUNIT_OCCURENCES, "SVGA_REG_BLUE_MASK reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegBusyRd,                   STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/BusyRead",                   STAMUNIT_OCCURENCES, "SVGA_REG_BUSY reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegBytesPerLineRd,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/BytesPerLineRead",           STAMUNIT_OCCURENCES, "SVGA_REG_BYTES_PER_LINE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegCapabilitesRd,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/CapabilitesRead",            STAMUNIT_OCCURENCES, "SVGA_REG_CAPABILITIES reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegConfigDoneRd,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/ConfigDoneRead",             STAMUNIT_OCCURENCES, "SVGA_REG_CONFIG_DONE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegCursorXxxxRd,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/CursorXxxxRead",             STAMUNIT_OCCURENCES, "SVGA_REG_CURSOR_XXXX reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDepthRd,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DepthRead",                  STAMUNIT_OCCURENCES, "SVGA_REG_DEPTH reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayHeightRd,          STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayHeightRead",          STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_HEIGHT reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayIdRd,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayIdRead",              STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_ID reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayIsPrimaryRd,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayIsPrimaryRead",       STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_IS_PRIMARY reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayPositionXRd,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayPositionXRead",       STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_POSITION_X reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayPositionYRd,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayPositionYRead",       STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_POSITION_Y reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegDisplayWidthRd,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/DisplayWidthRead",           STAMUNIT_OCCURENCES, "SVGA_REG_DISPLAY_WIDTH reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegEnableRd,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/EnableRead",                 STAMUNIT_OCCURENCES, "SVGA_REG_ENABLE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegFbOffsetRd,               STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/FbOffsetRead",               STAMUNIT_OCCURENCES, "SVGA_REG_FB_OFFSET reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegFbSizeRd,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/FbSizeRead",                 STAMUNIT_OCCURENCES, "SVGA_REG_FB_SIZE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegFbStartRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/FbStartRead",                STAMUNIT_OCCURENCES, "SVGA_REG_FB_START reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGmrIdRd,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrIdRead",                  STAMUNIT_OCCURENCES, "SVGA_REG_GMR_ID reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGmrMaxDescriptorLengthRd, STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrMaxDescriptorLengthRead", STAMUNIT_OCCURENCES, "SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGmrMaxIdsRd,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrMaxIdsRead",              STAMUNIT_OCCURENCES, "SVGA_REG_GMR_MAX_IDS reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGmrsMaxPagesRd,           STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GmrsMaxPagesRead",           STAMUNIT_OCCURENCES, "SVGA_REG_GMRS_MAX_PAGES reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGreenMaskRd,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GreenMaskRead",              STAMUNIT_OCCURENCES, "SVGA_REG_GREEN_MASK reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegGuestIdRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/GuestIdRead",                STAMUNIT_OCCURENCES, "SVGA_REG_GUEST_ID reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegHeightRd,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/HeightRead",                 STAMUNIT_OCCURENCES, "SVGA_REG_HEIGHT reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegHostBitsPerPixelRd,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/HostBitsPerPixelRead",       STAMUNIT_OCCURENCES, "SVGA_REG_HOST_BITS_PER_PIXEL reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegIdRd,                     STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/IdRead",                     STAMUNIT_OCCURENCES, "SVGA_REG_ID reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegIrqMaskRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/IrqMaskRead",                STAMUNIT_OCCURENCES, "SVGA_REG_IRQ_MASK reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegMaxHeightRd,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/MaxHeightRead",              STAMUNIT_OCCURENCES, "SVGA_REG_MAX_HEIGHT reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegMaxWidthRd,               STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/MaxWidthRead",               STAMUNIT_OCCURENCES, "SVGA_REG_MAX_WIDTH reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegMemorySizeRd,             STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/MemorySizeRead",             STAMUNIT_OCCURENCES, "SVGA_REG_MEMORY_SIZE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegMemRegsRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/MemRegsRead",                STAMUNIT_OCCURENCES, "SVGA_REG_MEM_REGS reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegMemSizeRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/MemSizeRead",                STAMUNIT_OCCURENCES, "SVGA_REG_MEM_SIZE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegMemStartRd,               STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/MemStartRead",               STAMUNIT_OCCURENCES, "SVGA_REG_MEM_START reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegNumDisplaysRd,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/NumDisplaysRead",            STAMUNIT_OCCURENCES, "SVGA_REG_NUM_DISPLAYS reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegNumGuestDisplaysRd,       STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/NumGuestDisplaysRead",       STAMUNIT_OCCURENCES, "SVGA_REG_NUM_GUEST_DISPLAYS reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegPaletteRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/PaletteRead",                STAMUNIT_OCCURENCES, "SVGA_REG_PLAETTE_XXXX reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegPitchLockRd,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/PitchLockRead",              STAMUNIT_OCCURENCES, "SVGA_REG_PITCHLOCK reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegPsuedoColorRd,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/PsuedoColorRead",            STAMUNIT_OCCURENCES, "SVGA_REG_PSEUDOCOLOR reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegRedMaskRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/RedMaskRead",                STAMUNIT_OCCURENCES, "SVGA_REG_RED_MASK reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegScratchRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/ScratchRead",                STAMUNIT_OCCURENCES, "SVGA_REG_SCRATCH reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegScratchSizeRd,            STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/ScratchSizeRead",            STAMUNIT_OCCURENCES, "SVGA_REG_SCRATCH_SIZE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegSyncRd,                   STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/SyncRead",                   STAMUNIT_OCCURENCES, "SVGA_REG_SYNC reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegTopRd,                    STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/TopRead",                    STAMUNIT_OCCURENCES, "SVGA_REG_TOP reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegTracesRd,                 STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/TracesRead",                 STAMUNIT_OCCURENCES, "SVGA_REG_TRACES reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegUnknownRd,                STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/UnknownRead",                STAMUNIT_OCCURENCES, "SVGA_REG_UNKNOWN reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegVramSizeRd,               STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/VramSizeRead",               STAMUNIT_OCCURENCES, "SVGA_REG_VRAM_SIZE reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegWidthRd,                  STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/WidthRead",                  STAMUNIT_OCCURENCES, "SVGA_REG_WIDTH reads.");
    STAM_REL_REG(pVM, &pThis->svga.StatRegWriteOnlyRd,              STAMTYPE_COUNTER, "/Devices/VMSVGA/Reg/WriteOnlyRead",              STAMUNIT_OCCURENCES, "Write-only SVGA_REG_XXXX reads.");

    STAM_REL_REG(pVM, &pSVGAState->StatBusyDelayEmts,   STAMTYPE_PROFILE, "/Devices/VMSVGA/EmtDelayOnBusyFifo",  STAMUNIT_TICKS_PER_CALL, "Time we've delayed EMTs because of busy FIFO thread.");
    STAM_REL_REG(pVM, &pSVGAState->StatFifoCommands,    STAMTYPE_COUNTER, "/Devices/VMSVGA/FifoCommands",  STAMUNIT_OCCURENCES, "FIFO command counter.");
    STAM_REL_REG(pVM, &pSVGAState->StatFifoErrors,      STAMTYPE_COUNTER, "/Devices/VMSVGA/FifoErrors",  STAMUNIT_OCCURENCES, "FIFO error counter.");
    STAM_REL_REG(pVM, &pSVGAState->StatFifoUnkCmds,     STAMTYPE_COUNTER, "/Devices/VMSVGA/FifoUnknownCommands",  STAMUNIT_OCCURENCES, "FIFO unknown command counter.");
    STAM_REL_REG(pVM, &pSVGAState->StatFifoTodoTimeout, STAMTYPE_COUNTER, "/Devices/VMSVGA/FifoTodoTimeout",  STAMUNIT_OCCURENCES, "Number of times we discovered pending work after a wait timeout.");
    STAM_REL_REG(pVM, &pSVGAState->StatFifoTodoWoken,   STAMTYPE_COUNTER, "/Devices/VMSVGA/FifoTodoWoken",  STAMUNIT_OCCURENCES, "Number of times we discovered pending work after being woken up.");
    STAM_REL_REG(pVM, &pSVGAState->StatFifoStalls,      STAMTYPE_PROFILE, "/Devices/VMSVGA/FifoStalls",  STAMUNIT_TICKS_PER_CALL, "Profiling of FIFO stalls (waiting for guest to finish copying data).");

    /*
     * Info handlers.
     */
    PDMDevHlpDBGFInfoRegister(pDevIns, "vmsvga", "Basic VMSVGA device state details", vmsvgaR3Info);
# ifdef VBOX_WITH_VMSVGA3D
    PDMDevHlpDBGFInfoRegister(pDevIns, "vmsvga3dctx", "VMSVGA 3d context details. Accepts 'terse'.", vmsvgaR3Info3dContext);
    PDMDevHlpDBGFInfoRegister(pDevIns, "vmsvga3dsfc",
                              "VMSVGA 3d surface details. "
                              "Accepts 'terse', 'invy', and one of 'tiny', 'medium', 'normal', 'big', 'huge', or 'gigantic'.",
                              vmsvgaR3Info3dSurface);
# endif

    return VINF_SUCCESS;
}

# ifdef VBOX_WITH_VMSVGA3D
/** Names for the vmsvga 3d capabilities, prefixed with format type hint char. */
static const char * const g_apszVmSvgaDevCapNames[] =
{
    "x3D",                           /* = 0 */
    "xMAX_LIGHTS",
    "xMAX_TEXTURES",
    "xMAX_CLIP_PLANES",
    "xVERTEX_SHADER_VERSION",
    "xVERTEX_SHADER",
    "xFRAGMENT_SHADER_VERSION",
    "xFRAGMENT_SHADER",
    "xMAX_RENDER_TARGETS",
    "xS23E8_TEXTURES",
    "xS10E5_TEXTURES",
    "xMAX_FIXED_VERTEXBLEND",
    "xD16_BUFFER_FORMAT",
    "xD24S8_BUFFER_FORMAT",
    "xD24X8_BUFFER_FORMAT",
    "xQUERY_TYPES",
    "xTEXTURE_GRADIENT_SAMPLING",
    "rMAX_POINT_SIZE",
    "xMAX_SHADER_TEXTURES",
    "xMAX_TEXTURE_WIDTH",
    "xMAX_TEXTURE_HEIGHT",
    "xMAX_VOLUME_EXTENT",
    "xMAX_TEXTURE_REPEAT",
    "xMAX_TEXTURE_ASPECT_RATIO",
    "xMAX_TEXTURE_ANISOTROPY",
    "xMAX_PRIMITIVE_COUNT",
    "xMAX_VERTEX_INDEX",
    "xMAX_VERTEX_SHADER_INSTRUCTIONS",
    "xMAX_FRAGMENT_SHADER_INSTRUCTIONS",
    "xMAX_VERTEX_SHADER_TEMPS",
    "xMAX_FRAGMENT_SHADER_TEMPS",
    "xTEXTURE_OPS",
    "xSURFACEFMT_X8R8G8B8",
    "xSURFACEFMT_A8R8G8B8",
    "xSURFACEFMT_A2R10G10B10",
    "xSURFACEFMT_X1R5G5B5",
    "xSURFACEFMT_A1R5G5B5",
    "xSURFACEFMT_A4R4G4B4",
    "xSURFACEFMT_R5G6B5",
    "xSURFACEFMT_LUMINANCE16",
    "xSURFACEFMT_LUMINANCE8_ALPHA8",
    "xSURFACEFMT_ALPHA8",
    "xSURFACEFMT_LUMINANCE8",
    "xSURFACEFMT_Z_D16",
    "xSURFACEFMT_Z_D24S8",
    "xSURFACEFMT_Z_D24X8",
    "xSURFACEFMT_DXT1",
    "xSURFACEFMT_DXT2",
    "xSURFACEFMT_DXT3",
    "xSURFACEFMT_DXT4",
    "xSURFACEFMT_DXT5",
    "xSURFACEFMT_BUMPX8L8V8U8",
    "xSURFACEFMT_A2W10V10U10",
    "xSURFACEFMT_BUMPU8V8",
    "xSURFACEFMT_Q8W8V8U8",
    "xSURFACEFMT_CxV8U8",
    "xSURFACEFMT_R_S10E5",
    "xSURFACEFMT_R_S23E8",
    "xSURFACEFMT_RG_S10E5",
    "xSURFACEFMT_RG_S23E8",
    "xSURFACEFMT_ARGB_S10E5",
    "xSURFACEFMT_ARGB_S23E8",
    "xMISSING62",
    "xMAX_VERTEX_SHADER_TEXTURES",
    "xMAX_SIMULTANEOUS_RENDER_TARGETS",
    "xSURFACEFMT_V16U16",
    "xSURFACEFMT_G16R16",
    "xSURFACEFMT_A16B16G16R16",
    "xSURFACEFMT_UYVY",
    "xSURFACEFMT_YUY2",
    "xMULTISAMPLE_NONMASKABLESAMPLES",
    "xMULTISAMPLE_MASKABLESAMPLES",
    "xALPHATOCOVERAGE",
    "xSUPERSAMPLE",
    "xAUTOGENMIPMAPS",
    "xSURFACEFMT_NV12",
    "xSURFACEFMT_AYUV",
    "xMAX_CONTEXT_IDS",
    "xMAX_SURFACE_IDS",
    "xSURFACEFMT_Z_DF16",
    "xSURFACEFMT_Z_DF24",
    "xSURFACEFMT_Z_D24S8_INT",
    "xSURFACEFMT_BC4_UNORM",
    "xSURFACEFMT_BC5_UNORM", /* 83 */
};
# endif


/**
 * Power On notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 *
 * @remarks Caller enters the device critical section.
 */
DECLCALLBACK(void) vmsvgaR3PowerOn(PPDMDEVINS pDevIns)
{
# ifdef VBOX_WITH_VMSVGA3D
    PVGASTATE pThis = PDMINS_2_DATA(pDevIns, PVGASTATE);
    if (pThis->svga.f3DEnabled)
    {
        int rc = vmsvga3dPowerOn(pThis);

        if (RT_SUCCESS(rc))
        {
            bool              fSavedBuffering = RTLogRelSetBuffering(true);
            SVGA3dCapsRecord *pCaps;
            SVGA3dCapPair    *pData;
            uint32_t          idxCap  = 0;

            /* 3d hardware version; latest and greatest */
            pThis->svga.pFIFOR3[SVGA_FIFO_3D_HWVERSION_REVISED] = SVGA3D_HWVERSION_CURRENT;
            pThis->svga.pFIFOR3[SVGA_FIFO_3D_HWVERSION]         = SVGA3D_HWVERSION_CURRENT;

            pCaps = (SVGA3dCapsRecord *)&pThis->svga.pFIFOR3[SVGA_FIFO_3D_CAPS];
            pCaps->header.type   = SVGA3DCAPS_RECORD_DEVCAPS;
            pData = (SVGA3dCapPair *)&pCaps->data;

            /* Fill out all 3d capabilities. */
            for (unsigned i = 0; i < SVGA3D_DEVCAP_MAX; i++)
            {
                uint32_t val = 0;

                rc = vmsvga3dQueryCaps(pThis, i, &val);
                if (RT_SUCCESS(rc))
                {
                    pData[idxCap][0] = i;
                    pData[idxCap][1] = val;
                    idxCap++;
                    if (g_apszVmSvgaDevCapNames[i][0] == 'x')
                        LogRel(("VMSVGA3d: cap[%u]=%#010x {%s}\n", i, val, &g_apszVmSvgaDevCapNames[i][1]));
                    else
                        LogRel(("VMSVGA3d: cap[%u]=%d.%04u {%s}\n", i, (int)*(float *)&val, (unsigned)(*(float *)&val * 10000) % 10000,
                                &g_apszVmSvgaDevCapNames[i][1]));
                }
                else
                    LogRel(("VMSVGA3d: cap[%u]=failed rc=%Rrc! {%s}\n", i, rc, &g_apszVmSvgaDevCapNames[i][1]));
            }
            pCaps->header.length = (sizeof(pCaps->header) + idxCap * sizeof(SVGA3dCapPair)) / sizeof(uint32_t);
            pCaps = (SVGA3dCapsRecord *)((uint32_t *)pCaps + pCaps->header.length);

            /* Mark end of record array. */
            pCaps->header.length = 0;

            RTLogRelSetBuffering(fSavedBuffering);
        }
    }
# else  /* !VBOX_WITH_VMSVGA3D */
    RT_NOREF(pDevIns);
# endif /* !VBOX_WITH_VMSVGA3D */
}

#endif /* IN_RING3 */

