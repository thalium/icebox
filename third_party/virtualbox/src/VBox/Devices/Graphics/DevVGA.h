/* $Id: DevVGA.h $ */
/** @file
 * DevVGA - VBox VGA/VESA device, internal header.
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
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * QEMU internal VGA defines.
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/** Use VBE bytewise I/O. Only needed for Windows Longhorn/Vista betas and backwards compatibility. */
#define VBE_BYTEWISE_IO

#ifdef VBOX
/** The default amount of VRAM. */
# define VGA_VRAM_DEFAULT    (_4M)
/** The maximum amount of VRAM. Limited by VBOX_MAX_ALLOC_PAGE_COUNT. */
# define VGA_VRAM_MAX        (256 * _1M)
/** The minimum amount of VRAM. */
# define VGA_VRAM_MIN        (_1M)
#endif

#include <VBoxVideoVBE.h>
#include <VBoxVideoVBEPrivate.h>

#ifdef VBOX_WITH_HGSMI
# include "HGSMI/HGSMIHost.h"
#endif /* VBOX_WITH_HGSMI */
#include "DevVGASavedState.h"

#ifdef VBOX_WITH_VMSVGA
# include "DevVGA-SVGA.h"
#endif

#include <iprt/list.h>

#define MSR_COLOR_EMULATION 0x01
#define MSR_PAGE_SELECT     0x20

#define ST01_V_RETRACE      0x08
#define ST01_DISP_ENABLE    0x01

/* bochs VBE support */
#define CONFIG_BOCHS_VBE

#ifdef CONFIG_BOCHS_VBE

/* Cross reference with <VBoxVideoVBE.h> */
#define VBE_DISPI_INDEX_NB_SAVED        0xb /* Number of saved registers (vbe_regs array) */
#define VBE_DISPI_INDEX_NB              0xc /* Total number of VBE registers */

#define VGA_STATE_COMMON_BOCHS_VBE              \
    uint16_t vbe_index;                         \
    uint16_t vbe_regs[VBE_DISPI_INDEX_NB];      \
    uint16_t alignment[3]; /* pad to 64 bits */ \
    uint32_t vbe_start_addr;                    \
    uint32_t vbe_line_offset;                   \
    uint32_t vbe_bank_max;

#else

#define VGA_STATE_COMMON_BOCHS_VBE

#endif /* !CONFIG_BOCHS_VBE */

#define CH_ATTR_SIZE (160 * 100)
#define VGA_MAX_HEIGHT VBE_DISPI_MAX_YRES

typedef struct vga_retrace_s {
    unsigned    frame_cclks;    /* Character clocks per frame. */
    unsigned    frame_ns;       /* Frame duration in ns. */
    unsigned    cclk_ns;        /* Character clock duration in ns. */
    unsigned    vb_start;       /* Vertical blanking start (scanline). */
    unsigned    vb_end;         /* Vertical blanking end (scanline). */
    unsigned    vb_end_ns;      /* Vertical blanking end time (length) in ns. */
    unsigned    vs_start;       /* Vertical sync start (scanline). */
    unsigned    vs_end;         /* Vertical sync end (scanline). */
    unsigned    vs_start_ns;    /* Vertical sync start time in ns. */
    unsigned    vs_end_ns;      /* Vertical sync end time in ns. */
    unsigned    h_total;        /* Horizontal total (cclks per scanline). */
    unsigned    h_total_ns;     /* Scanline duration in ns. */
    unsigned    hb_start;       /* Horizontal blanking start (cclk). */
    unsigned    hb_end;         /* Horizontal blanking end (cclk). */
    unsigned    hb_end_ns;      /* Horizontal blanking end time (length) in ns. */
    unsigned    v_freq_hz;      /* Vertical refresh rate to emulate. */
} vga_retrace_s;

#ifndef VBOX
#define VGA_STATE_COMMON                                                \
    uint8_t *vram_ptr;                                                  \
    unsigned long vram_offset;                                          \
    unsigned int vram_size;                                             \
    uint32_t latch;                                                     \
    uint8_t sr_index;                                                   \
    uint8_t sr[256];                                                    \
    uint8_t gr_index;                                                   \
    uint8_t gr[256];                                                    \
    uint8_t ar_index;                                                   \
    uint8_t ar[21];                                                     \
    int ar_flip_flop;                                                   \
    uint8_t cr_index;                                                   \
    uint8_t cr[256]; /* CRT registers */                                \
    uint8_t msr; /* Misc Output Register */                             \
    uint8_t fcr; /* Feature Control Register */                         \
    uint8_t st00; /* status 0 */                                        \
    uint8_t st01; /* status 1 */                                        \
    uint8_t dac_state;                                                  \
    uint8_t dac_sub_index;                                              \
    uint8_t dac_read_index;                                             \
    uint8_t dac_write_index;                                            \
    uint8_t dac_cache[3]; /* used when writing */                       \
    uint8_t palette[768];                                               \
    int32_t bank_offset;                                                \
    int (*get_bpp)(struct VGAState *s);                                 \
    void (*get_offsets)(struct VGAState *s,                             \
                        uint32_t *pline_offset,                         \
                        uint32_t *pstart_addr,                          \
                        uint32_t *pline_compare);                       \
    void (*get_resolution)(struct VGAState *s,                          \
                        int *pwidth,                                    \
                        int *pheight);                                  \
    VGA_STATE_COMMON_BOCHS_VBE                                          \
    /* display refresh support */                                       \
    DisplayState *ds;                                                   \
    uint32_t font_offsets[2];                                           \
    int graphic_mode;                                                   \
    uint8_t shift_control;                                              \
    uint8_t double_scan;                                                \
    uint32_t line_offset;                                               \
    uint32_t line_compare;                                              \
    uint32_t start_addr;                                                \
    uint32_t plane_updated;                                             \
    uint8_t last_cw, last_ch;                                           \
    uint32_t last_width, last_height; /* in chars or pixels */          \
    uint32_t last_scr_width, last_scr_height; /* in pixels */           \
    uint8_t cursor_start, cursor_end;                                   \
    uint32_t cursor_offset;                                             \
    unsigned int (*rgb_to_pixel)(unsigned int r,                        \
                                 unsigned int g, unsigned b);           \
    /* hardware mouse cursor support */                                 \
    uint32_t invalidated_y_table[VGA_MAX_HEIGHT / 32];                  \
    void (*cursor_invalidate)(struct VGAState *s);                      \
    void (*cursor_draw_line)(struct VGAState *s, uint8_t *d, int y);    \
    /* tell for each page if it has been updated since the last time */ \
    uint32_t last_palette[256];                                         \
    uint32_t last_ch_attr[CH_ATTR_SIZE]; /* XXX: make it dynamic */

#else /* VBOX */

/* bird: Since we've changed types, reordered members, done alignment
         paddings and more, VGA_STATE_COMMON was added directly to the
         struct to make it more readable and easier to handle. */

struct VGAState;
typedef int FNGETBPP(struct VGAState *s);
typedef void FNGETOFFSETS(struct VGAState *s, uint32_t *pline_offset, uint32_t *pstart_addr, uint32_t *pline_compare);
typedef void FNGETRESOLUTION(struct VGAState *s, int *pwidth, int *pheight);
typedef unsigned int FNRGBTOPIXEL(unsigned int r, unsigned int g, unsigned b);
typedef void FNCURSORINVALIDATE(struct VGAState *s);
typedef void FNCURSORDRAWLINE(struct VGAState *s, uint8_t *d, int y);

#endif /* VBOX */

#ifdef VBOX_WITH_VDMA
typedef struct VBOXVDMAHOST *PVBOXVDMAHOST;
#endif

#ifdef VBOX_WITH_VIDEOHWACCEL
#define VBOX_VHWA_MAX_PENDING_COMMANDS 1000

typedef struct _VBOX_VHWA_PENDINGCMD
{
    RTLISTNODE Node;
    VBOXVHWACMD RT_UNTRUSTED_VOLATILE_GUEST *pCommand;
} VBOX_VHWA_PENDINGCMD;
#endif


typedef struct VGAState {
#ifndef VBOX
    VGA_STATE_COMMON
#else /* VBOX */
    R3PTRTYPE(uint8_t *) vram_ptrR3;
    R3PTRTYPE(FNGETBPP *) get_bpp;
    R3PTRTYPE(FNGETOFFSETS *) get_offsets;
    R3PTRTYPE(FNGETRESOLUTION *) get_resolution;
    R3PTRTYPE(FNRGBTOPIXEL *) rgb_to_pixel;
    R3PTRTYPE(FNCURSORINVALIDATE *) cursor_invalidate;
    R3PTRTYPE(FNCURSORDRAWLINE *) cursor_draw_line;
    RTR3PTR R3PtrCmnAlignment;
    uint32_t vram_size;
    uint32_t latch;
    uint8_t sr_index;
    uint8_t sr[256];
    uint8_t gr_index;
    uint8_t gr[256];
    uint8_t ar_index;
    uint8_t ar[21];
    int32_t ar_flip_flop;
    uint8_t cr_index;
    uint8_t cr[256]; /* CRT registers */
    uint8_t msr; /* Misc Output Register */
    uint8_t fcr; /* Feature Control Register */
    uint8_t st00; /* status 0 */
    uint8_t st01; /* status 1 */
    uint8_t dac_state;
    uint8_t dac_sub_index;
    uint8_t dac_read_index;
    uint8_t dac_write_index;
    uint8_t dac_cache[3]; /* used when writing */
    uint8_t palette[768];
    int32_t bank_offset;
    VGA_STATE_COMMON_BOCHS_VBE
    /* display refresh support */
    uint32_t font_offsets[2];
    int32_t graphic_mode;
    uint8_t shift_control;
    uint8_t double_scan;
    uint8_t padding1[2];
    uint32_t line_offset;
    uint32_t vga_addr_mask;
    uint32_t padding1a;
    uint32_t line_compare;
    uint32_t start_addr;
    uint32_t plane_updated;
    uint8_t last_cw, last_ch, padding2[2];
    uint32_t last_width, last_height; /* in chars or pixels */
    uint32_t last_scr_width, last_scr_height; /* in pixels */
    uint32_t last_bpp;
    uint8_t cursor_start, cursor_end;
    bool last_cur_blink, last_chr_blink;
    uint32_t cursor_offset;
    /* hardware mouse cursor support */
    uint32_t invalidated_y_table[VGA_MAX_HEIGHT / 32];
    /* tell for each page if it has been updated since the last time */
    uint32_t last_palette[256];
    uint32_t last_ch_attr[CH_ATTR_SIZE]; /* XXX: make it dynamic */

    /** end-of-common-state-marker */
    uint32_t                    u32Marker;

    /** Pointer to the device instance - RC Ptr. */
    PPDMDEVINSRC                pDevInsRC;
    /** Pointer to the GC vram mapping. */
    RCPTRTYPE(uint8_t *)        vram_ptrRC;
    uint32_t                    Padding1;

    /** Pointer to the device instance - R3 Ptr. */
    PPDMDEVINSR3                pDevInsR3;
# ifdef VBOX_WITH_HGSMI
    R3PTRTYPE(PHGSMIINSTANCE)   pHGSMI;
# endif
# ifdef VBOX_WITH_VDMA
    R3PTRTYPE(PVBOXVDMAHOST)    pVdma;
# endif
    /** LUN\#0: The display port base interface. */
    PDMIBASE                    IBase;
    /** LUN\#0: The display port interface. */
    PDMIDISPLAYPORT             IPort;
# if defined(VBOX_WITH_HGSMI) && (defined(VBOX_WITH_VIDEOHWACCEL) || defined(VBOX_WITH_CRHGSMI))
    /** LUN\#0: VBVA callbacks interface */
    PDMIDISPLAYVBVACALLBACKS    IVBVACallbacks;
# else
    RTR3PTR                     Padding2;
# endif
    /** Status LUN: Leds interface. */
    PDMILEDPORTS                ILeds;

    /** Pointer to base interface of the driver. */
    R3PTRTYPE(PPDMIBASE)        pDrvBase;
    /** Pointer to display connector interface of the driver. */
    R3PTRTYPE(PPDMIDISPLAYCONNECTOR) pDrv;

    /** Status LUN: Partner of ILeds. */
    R3PTRTYPE(PPDMILEDCONNECTORS)   pLedsConnector;

    /** Refresh timer handle - HC. */
    PTMTIMERR3                  RefreshTimer;

    /** Pointer to the device instance - R0 Ptr. */
    PPDMDEVINSR0                pDevInsR0;
    /** The R0 vram pointer... */
    R0PTRTYPE(uint8_t *)        vram_ptrR0;

# ifdef VBOX_WITH_VMSVGA
    VMSVGAState                 svga;
# endif

    /** The number of monitors. */
    uint32_t                    cMonitors;
    /** Current refresh timer interval. */
    uint32_t                    cMilliesRefreshInterval;
    /** Bitmap tracking dirty pages. */
    uint32_t                    au32DirtyBitmap[VGA_VRAM_MAX / PAGE_SIZE / 32];

    /** Flag indicating that there are dirty bits. This is used to optimize the handler resetting. */
    bool                        fHasDirtyBits;
    /** LFB was updated flag. */
    bool                        fLFBUpdated;
    /** Indicates if the GC extensions are enabled or not. */
    bool                        fGCEnabled;
    /** Indicates if the R0 extensions are enabled or not. */
    bool                        fR0Enabled;
    /** Flag indicating that the VGA memory in the 0xa0000-0xbffff region has been remapped to allow direct access. */
    bool                        fRemappedVGA;
    /** Whether to render the guest VRAM to the framebuffer memory. False only for some LFB modes. */
    bool                        fRenderVRAM;
# ifdef VBOX_WITH_VMSVGA
    /* Whether the SVGA emulation is enabled or not. */
    bool                        fVMSVGAEnabled;
    bool                        Padding4[1+4];
# else
    bool                        Padding4[2+4];
# endif

    /** Physical access type for the linear frame buffer dirty page tracking. */
    PGMPHYSHANDLERTYPE          hLfbAccessHandlerType;

    /** The physical address the VRAM was assigned. */
    RTGCPHYS                    GCPhysVRAM;
    /** The critical section protect the instance data. */
    PDMCRITSECT                 CritSect;
    /** The PCI device. */
    PDMPCIDEV                   Dev;

    STAMPROFILE                 StatRZMemoryRead;
    STAMPROFILE                 StatR3MemoryRead;
    STAMPROFILE                 StatRZMemoryWrite;
    STAMPROFILE                 StatR3MemoryWrite;
    STAMCOUNTER                 StatMapPage;            /**< Counts IOMMMIOMapMMIO2Page calls.  */
    STAMCOUNTER                 StatUpdateDisp;         /**< Counts vgaPortUpdateDisplay calls.  */

    /* Keep track of ring 0 latched accesses to the VGA MMIO memory. */
    uint64_t                    u64LastLatchedAccess;
    uint32_t                    cLatchAccesses;
    uint16_t                    uMaskLatchAccess;
    uint16_t                    iMask;

# ifdef VBE_BYTEWISE_IO
    /** VBE read/write data/index flags */
    uint8_t                     fReadVBEData;
    uint8_t                     fWriteVBEData;
    uint8_t                     fReadVBEIndex;
    uint8_t                     fWriteVBEIndex;
    /** VBE write data/index one byte buffer */
    uint8_t                     cbWriteVBEData;
    uint8_t                     cbWriteVBEIndex;
    /** VBE Extra Data write address one byte buffer */
    uint8_t                     cbWriteVBEExtraAddress;
    uint8_t                     Padding5;
# endif

    /** Retrace emulation state */
    bool                        fRealRetrace;
    bool                        Padding6[HC_ARCH_BITS == 64 ? 7 : 3];
    vga_retrace_s               retrace_state;

    /** The VBE BIOS extra data. */
    R3PTRTYPE(uint8_t *)        pbVBEExtraData;
    /** The size of the VBE BIOS extra data. */
    uint16_t                    cbVBEExtraData;
    /** The VBE BIOS current memory address. */
    uint16_t                    u16VBEExtraAddress;
    uint16_t                    Padding7[2];

    /** The BIOS logo data. */
    R3PTRTYPE(uint8_t *)        pbLogo;
    /** The name of the logo file. */
    R3PTRTYPE(char *)           pszLogoFile;
    /** Bitmap image data. */
    R3PTRTYPE(uint8_t *)        pbLogoBitmap;
    /** Current logo data offset. */
    uint32_t                    offLogoData;
    /** The size of the BIOS logo data. */
    uint32_t                    cbLogo;
    /** Current logo command. */
    uint16_t                    LogoCommand;
    /** Bitmap width. */
    uint16_t                    cxLogo;
    /** Bitmap height. */
    uint16_t                    cyLogo;
    /** Bitmap planes. */
    uint16_t                    cLogoPlanes;
    /** Bitmap depth. */
    uint16_t                    cLogoBits;
    /** Bitmap compression. */
    uint16_t                    LogoCompression;
    /** Bitmap colors used. */
    uint16_t                    cLogoUsedColors;
    /** Palette size. */
    uint16_t                    cLogoPalEntries;
    /** Clear screen flag. */
    uint8_t                     fLogoClearScreen;
    bool                        fBootMenuInverse;
    uint8_t                     Padding8[6];
    /** Palette data. */
    uint32_t                    au32LogoPalette[256];

    /** The VGA BIOS ROM data. */
    R3PTRTYPE(uint8_t *)        pbVgaBios;
    /** The size of the VGA BIOS ROM. */
    uint64_t                    cbVgaBios;
    /** The name of the VGA BIOS ROM file. */
    R3PTRTYPE(char *)           pszVgaBiosFile;
# if HC_ARCH_BITS == 32
    uint32_t                    Padding9;
# endif

# ifdef VBOX_WITH_HGSMI
    /** Base port in the assigned PCI I/O space. */
    RTIOPORT                    IOPortBase;
#  ifdef VBOX_WITH_WDDM
    uint8_t                     Padding10[2];
    /** Specifies guest driver caps, i.e. whether it can handle IRQs from the
     * adapter, the way it can handle async HGSMI command completion, etc. */
    uint32_t                    fGuestCaps;
    uint32_t                    fScanLineCfg;
    uint32_t                    fHostCursorCapabilities;
#  else
    uint8_t                     Padding11[14];
#  endif

    /** The critical section serializes the HGSMI IRQ setting/clearing. */
    PDMCRITSECT                 CritSectIRQ;
    /** VBVARaiseIRQ flags which were set when the guest was still processing previous IRQ. */
    uint32_t                    fu32PendingGuestFlags;
    uint32_t                    Padding12;
# endif /* VBOX_WITH_HGSMI */

    PDMLED Led3D;

    struct {
        volatile uint32_t cPending;
        uint32_t Padding1;
        union
        {
            RTLISTNODE PendingList;
            /* make sure the structure sized cross different contexts correctly */
            struct
            {
                R3PTRTYPE(void *) dummy1;
                R3PTRTYPE(void *) dummy2;
            } dummy;
        };
    } pendingVhwaCommands;
#endif /* VBOX */
} VGAState;
#ifdef VBOX
/** VGA state. */
typedef VGAState VGASTATE;
/** Pointer to the VGA state. */
typedef VGASTATE *PVGASTATE;
AssertCompileMemberAlignment(VGASTATE, bank_offset, 8);
AssertCompileMemberAlignment(VGASTATE, font_offsets, 8);
AssertCompileMemberAlignment(VGASTATE, last_ch_attr, 8);
AssertCompileMemberAlignment(VGASTATE, u32Marker, 8);
#endif

/** VBE Extra Data. */
typedef VBEHeader VBEHEADER;
/** Pointer to the VBE Extra Data. */
typedef VBEHEADER *PVBEHEADER;

#if !defined(VBOX) || defined(IN_RING3)
static inline int c6_to_8(int v)
{
    int b;
    v &= 0x3f;
    b = v & 1;
    return (v << 2) | (b << 1) | b;
}
#endif /* !VBOX || IN_RING3 */


#ifdef VBOX_WITH_HGSMI
int      VBVAInit       (PVGASTATE pVGAState);
void     VBVADestroy    (PVGASTATE pVGAState);
int      VBVAUpdateDisplay (PVGASTATE pVGAState);
void     VBVAReset (PVGASTATE pVGAState);
void     VBVAPause (PVGASTATE pVGAState, bool fPause);
void     VBVAOnVBEChanged(PVGASTATE pVGAState);
void     VBVAOnResume(PVGASTATE pThis);

bool VBVAIsPaused(PVGASTATE pVGAState);
bool VBVAIsEnabled(PVGASTATE pVGAState);

void VBVARaiseIrq(PVGASTATE pVGAState, uint32_t fFlags);

int VBVAInfoView(PVGASTATE pVGAState, const VBVAINFOVIEW RT_UNTRUSTED_VOLATILE_HOST *pView);
int VBVAInfoScreen(PVGASTATE pVGAState, const VBVAINFOSCREEN RT_UNTRUSTED_VOLATILE_HOST *pScreen);
int VBVAGetInfoViewAndScreen(PVGASTATE pVGAState, uint32_t u32ViewIndex, VBVAINFOVIEW *pView, VBVAINFOSCREEN *pScreen);

/* @return host-guest flags that were set on reset
 * this allows the caller to make further cleaning when needed,
 * e.g. reset the IRQ */
uint32_t HGSMIReset(PHGSMIINSTANCE pIns);

# ifdef VBOX_WITH_VIDEOHWACCEL
DECLCALLBACK(int) vbvaVHWACommandCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_GUEST *pCmd);
int vbvaVHWAConstruct(PVGASTATE pVGAState);
int vbvaVHWAReset(PVGASTATE pVGAState);

void vbvaTimerCb(PVGASTATE pVGAState);

int vboxVBVASaveStatePrep(PPDMDEVINS pDevIns);
int vboxVBVASaveStateDone(PPDMDEVINS pDevIns);
# endif

#ifdef VBOX_WITH_HGSMI
#define PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(_pcb) ( (PVGASTATE)((uint8_t *)(_pcb) - RT_OFFSETOF(VGASTATE, IVBVACallbacks)) )
#endif

# ifdef VBOX_WITH_CRHGSMI
DECLCALLBACK(int) vboxVDMACrHgsmiCommandCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface,
                                                      PVBOXVDMACMD_CHROMIUM_CMD pCmd, int rc);
DECLCALLBACK(int) vboxVDMACrHgsmiControlCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface,
                                                      PVBOXVDMACMD_CHROMIUM_CTL pCmd, int rc);
DECLCALLBACK(int) vboxCmdVBVACmdHostCtl(PPDMIDISPLAYVBVACALLBACKS pInterface,
                                        struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd,
                                        PFNCRCTLCOMPLETION pfnCompletion,
                                        void *pvCompletion);
DECLCALLBACK(int) vboxCmdVBVACmdHostCtlSync(PPDMIDISPLAYVBVACALLBACKS pInterface,
                                            struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd);
# endif

int vboxVBVASaveStateExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM);
int vboxVBVALoadStateExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t u32Version);
int vboxVBVALoadStateDone(PPDMDEVINS pDevIns);

DECLCALLBACK(int) vgaUpdateDisplayAll(PVGASTATE pThis, bool fFailOnResize);
DECLCALLBACK(int) vbvaPortSendModeHint(PPDMIDISPLAYPORT pInterface, uint32_t cx,
                                       uint32_t cy, uint32_t cBPP,
                                       uint32_t cDisplay, uint32_t dx,
                                       uint32_t dy, uint32_t fEnabled,
                                       uint32_t fNotifyGuest);
DECLCALLBACK(void) vbvaPortReportHostCursorCapabilities(PPDMIDISPLAYPORT pInterface, uint32_t fCapabilitiesAdded,
                                                        uint32_t fCapabilitiesRemoved);
DECLCALLBACK(void) vbvaPortReportHostCursorPosition(PPDMIDISPLAYPORT pInterface, uint32_t x, uint32_t y);

# ifdef VBOX_WITH_VDMA
typedef struct VBOXVDMAHOST *PVBOXVDMAHOST;
int vboxVDMAConstruct(PVGASTATE pVGAState, uint32_t cPipeElements);
void vboxVDMADestruct(PVBOXVDMAHOST pVdma);
void vboxVDMAReset(PVBOXVDMAHOST pVdma);
void vboxVDMAControl(PVBOXVDMAHOST pVdma, VBOXVDMA_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd);
void vboxVDMACommand(PVBOXVDMAHOST pVdma, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd);
int vboxVDMASaveStateExecPrep(struct VBOXVDMAHOST *pVdma);
int vboxVDMASaveStateExecDone(struct VBOXVDMAHOST *pVdma);
int vboxVDMASaveStateExecPerform(struct VBOXVDMAHOST *pVdma, PSSMHANDLE pSSM);
int vboxVDMASaveLoadExecPerform(struct VBOXVDMAHOST *pVdma, PSSMHANDLE pSSM, uint32_t u32Version);
int vboxVDMASaveLoadDone(struct VBOXVDMAHOST *pVdma);
# endif /* VBOX_WITH_VDMA */

# ifdef VBOX_WITH_CRHGSMI
int vboxCmdVBVACmdSubmit(PVGASTATE pVGAState);
int vboxCmdVBVACmdFlush(PVGASTATE pVGAState);
int vboxCmdVBVACmdCtl(PVGASTATE pVGAState, VBOXCMDVBVA_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCtl, uint32_t cbCtl);
void vboxCmdVBVATimerRefresh(PVGASTATE pVGAState);
bool vboxCmdVBVAIsEnabled(PVGASTATE pVGAState);
# endif /* VBOX_WITH_CRHGSMI */
#endif /* VBOX_WITH_HGSMI */

# ifdef VBOX_WITH_VMSVGA
int vgaR3RegisterVRAMHandler(PVGASTATE pVGAState, uint64_t cbFrameBuffer);
int vgaR3UnregisterVRAMHandler(PVGASTATE pVGAState);
int vgaR3UpdateDisplay(PVGASTATE pVGAState, unsigned xStart, unsigned yStart, unsigned width, unsigned height);
# endif

#ifndef VBOX
void vga_common_init(VGAState *s, DisplayState *ds, uint8_t *vga_ram_base,
                     unsigned long vga_ram_offset, int vga_ram_size);
uint32_t vga_mem_readb(void *opaque, target_phys_addr_t addr);
void vga_mem_writeb(void *opaque, target_phys_addr_t addr, uint32_t val);
void vga_invalidate_scanlines(VGAState *s, int y1, int y2);

void vga_draw_cursor_line_8(uint8_t *d1, const uint8_t *src1,
                            int poffset, int w,
                            unsigned int color0, unsigned int color1,
                            unsigned int color_xor);
void vga_draw_cursor_line_16(uint8_t *d1, const uint8_t *src1,
                             int poffset, int w,
                             unsigned int color0, unsigned int color1,
                             unsigned int color_xor);
void vga_draw_cursor_line_32(uint8_t *d1, const uint8_t *src1,
                             int poffset, int w,
                             unsigned int color0, unsigned int color1,
                             unsigned int color_xor);

extern const uint8_t sr_mask[8];
extern const uint8_t gr_mask[16];
#endif /* !VBOX */

