/* $Id: DevVGA-SVGA3d.h $ */
/** @file
 * DevVMWare - VMWare SVGA device - 3D part.
 */

/*
 * Copyright (C) 2013-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___DEVVMWARE3D_H___
#define ___DEVVMWARE3D_H___

#include "vmsvga/svga_reg.h"
#include "vmsvga/svga3d_reg.h"
#include "vmsvga/svga_escape.h"
#include "vmsvga/svga_overlay.h"


/** Arbitrary limit */
#define SVGA3D_MAX_SHADER_IDS                   0x800
/** D3D allows up to 8 texture stages. */
#define SVGA3D_MAX_TEXTURE_STAGE                8
/** Arbitrary upper limit; seen 8 so far. */
#define SVGA3D_MAX_LIGHTS                       32
/** Arbitrary upper limit; 2GB enough for 32768x16384*4. */
#define SVGA3D_MAX_SURFACE_MEM_SIZE             0x80000000


/**@def FLOAT_FMT_STR
 * Format string bits to go with FLOAT_FMT_ARGS. */
#define FLOAT_FMT_STR                           "%d.%06d"
/** @def FLOAT_FMT_ARGS
 * Format arguments for a float value, corresponding to FLOAT_FMT_STR.
 * @param   r       The floating point value to format.  */
#define FLOAT_FMT_ARGS(r)                       (int)(r), ((unsigned)((r) * 1000000) % 1000000U)


/* DevVGA-SVGA.cpp: */
void vmsvgaGMRFree(PVGASTATE pThis, uint32_t idGMR);
int  vmsvgaGMRTransfer(PVGASTATE pThis, const SVGA3dTransferType enmTransferType, uint8_t *pDest, int32_t cbDestPitch,
                       SVGAGuestPtr src, uint32_t offSrc, int32_t cbSrcPitch, uint32_t cbWidth, uint32_t cHeight);
void vmsvga3dSurfaceUpdateHeapBuffersOnFifoThread(PVGASTATE pThis, uint32_t sid);


/* DevVGA-SVGA3d-ogl.cpp & DevVGA-SVGA3d-win.cpp: */
int vmsvga3dInit(PVGASTATE pThis);
int vmsvga3dPowerOn(PVGASTATE pThis);
int vmsvga3dLoadExec(PVGASTATE pThis, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
int vmsvga3dSaveExec(PVGASTATE pThis, PSSMHANDLE pSSM);
int vmsvga3dTerminate(PVGASTATE pThis);
int vmsvga3dReset(PVGASTATE pThis);
void vmsvga3dUpdateHostScreenViewport(PVGASTATE pThis, uint32_t idScreen, VMSVGAVIEWPORT const *pOldViewport);
int vmsvga3dQueryCaps(PVGASTATE pThis, uint32_t idx3dCaps, uint32_t *pu32Val);

int vmsvga3dSurfaceDefine(PVGASTATE pThis, uint32_t sid, uint32_t surfaceFlags, SVGA3dSurfaceFormat format, SVGA3dSurfaceFace face[SVGA3D_MAX_SURFACE_FACES], uint32_t multisampleCount, SVGA3dTextureFilter autogenFilter, uint32_t cMipLevels, SVGA3dSize *pMipLevelSize);
int vmsvga3dSurfaceDestroy(PVGASTATE pThis, uint32_t sid);
int vmsvga3dSurfaceCopy(PVGASTATE pThis, SVGA3dSurfaceImageId dest, SVGA3dSurfaceImageId src, uint32_t cCopyBoxes, SVGA3dCopyBox *pBox);
int vmsvga3dSurfaceStretchBlt(PVGASTATE pThis, SVGA3dSurfaceImageId const *pDstSfcImg, SVGA3dBox const *pDstBox,
                              SVGA3dSurfaceImageId const *pSrcSfcImg, SVGA3dBox const *pSrcBox, SVGA3dStretchBltMode enmMode);
int vmsvga3dSurfaceDMA(PVGASTATE pThis, SVGA3dGuestImage guest, SVGA3dSurfaceImageId host, SVGA3dTransferType transfer, uint32_t cCopyBoxes, SVGA3dCopyBox *pBoxes);
int vmsvga3dSurfaceBlitToScreen(PVGASTATE pThis, uint32_t dest, SVGASignedRect destRect, SVGA3dSurfaceImageId src, SVGASignedRect srcRect, uint32_t cRects, SVGASignedRect *pRect);

int vmsvga3dContextDefine(PVGASTATE pThis, uint32_t cid);
int vmsvga3dContextDestroy(PVGASTATE pThis, uint32_t cid);

int vmsvga3dChangeMode(PVGASTATE pThis);

int vmsvga3dSetTransform(PVGASTATE pThis, uint32_t cid, SVGA3dTransformType type, float matrix[16]);
int vmsvga3dSetZRange(PVGASTATE pThis, uint32_t cid, SVGA3dZRange zRange);
int vmsvga3dSetRenderState(PVGASTATE pThis, uint32_t cid, uint32_t cRenderStates, SVGA3dRenderState *pRenderState);
int vmsvga3dSetRenderTarget(PVGASTATE pThis, uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId target);
int vmsvga3dSetTextureState(PVGASTATE pThis, uint32_t cid, uint32_t cTextureStates, SVGA3dTextureState *pTextureState);
int vmsvga3dSetMaterial(PVGASTATE pThis, uint32_t cid, SVGA3dFace face, SVGA3dMaterial *pMaterial);
int vmsvga3dSetLightData(PVGASTATE pThis, uint32_t cid, uint32_t index, SVGA3dLightData *pData);
int vmsvga3dSetLightEnabled(PVGASTATE pThis, uint32_t cid, uint32_t index, uint32_t enabled);
int vmsvga3dSetViewPort(PVGASTATE pThis, uint32_t cid, SVGA3dRect *pRect);
int vmsvga3dSetClipPlane(PVGASTATE pThis, uint32_t cid,  uint32_t index, float plane[4]);
int vmsvga3dCommandClear(PVGASTATE pThis, uint32_t cid, SVGA3dClearFlag clearFlag, uint32_t color, float depth, uint32_t stencil, uint32_t cRects, SVGA3dRect *pRect);
int vmsvga3dCommandPresent(PVGASTATE pThis, uint32_t sid, uint32_t cRects, SVGA3dCopyRect *pRect);
int vmsvga3dDrawPrimitives(PVGASTATE pThis, uint32_t cid, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl, uint32_t numRanges, SVGA3dPrimitiveRange *pNumRange, uint32_t cVertexDivisor, SVGA3dVertexDivisor *pVertexDivisor);
int vmsvga3dSetScissorRect(PVGASTATE pThis, uint32_t cid, SVGA3dRect *pRect);
int vmsvga3dGenerateMipmaps(PVGASTATE pThis, uint32_t sid, SVGA3dTextureFilter filter);

int vmsvga3dShaderDefine(PVGASTATE pThis, uint32_t cid, uint32_t shid, SVGA3dShaderType type, uint32_t cbData, uint32_t *pShaderData);
int vmsvga3dShaderDestroy(PVGASTATE pThis, uint32_t cid, uint32_t shid, SVGA3dShaderType type);
int vmsvga3dShaderSet(PVGASTATE pThis, struct VMSVGA3DCONTEXT *pContext, uint32_t cid, SVGA3dShaderType type, uint32_t shid);
int vmsvga3dShaderSetConst(PVGASTATE pThis, uint32_t cid, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype, uint32_t cRegisters, uint32_t *pValues);

int vmsvga3dQueryBegin(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type);
int vmsvga3dQueryEnd(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type, SVGAGuestPtr guestResult);
int vmsvga3dQueryWait(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type, SVGAGuestPtr guestResult);

/* DevVGA-SVGA3d-shared.h: */
#if defined(RT_OS_WINDOWS) && defined(IN_RING3)
# include <iprt/win/windows.h>

# define WM_VMSVGA3D_WAKEUP                     (WM_APP+1)
# define WM_VMSVGA3D_CREATEWINDOW               (WM_APP+2)
# define WM_VMSVGA3D_DESTROYWINDOW              (WM_APP+3)
# define WM_VMSVGA3D_RESIZEWINDOW               (WM_APP+4)
# define WM_VMSVGA3D_EXIT                       (WM_APP+5)

DECLCALLBACK(int) vmsvga3dWindowThread(RTTHREAD ThreadSelf, void *pvUser);
int vmsvga3dSendThreadMessage(RTTHREAD pWindowThread, RTSEMEVENT WndRequestSem, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

void vmsvga3dUpdateHeapBuffersForSurfaces(PVGASTATE pThis, uint32_t sid);
void vmsvga3dInfoContextWorker(PVGASTATE pThis, PCDBGFINFOHLP pHlp, uint32_t cid, bool fVerbose);
void vmsvga3dInfoSurfaceWorker(PVGASTATE pThis, PCDBGFINFOHLP pHlp, uint32_t sid, bool fVerbose, uint32_t cxAscii, bool fInvY);


/* DevVGA-SVGA3d-shared.cpp: */

/**
 * Structure for use with vmsvga3dInfoU32Flags.
 */
typedef struct VMSVGAINFOFLAGS32
{
    /** The flags. */
    uint32_t    fFlags;
    /** The corresponding mnemonic. */
    const char *pszJohnny;
} VMSVGAINFOFLAGS32;
/** Pointer to a read-only flag translation entry. */
typedef VMSVGAINFOFLAGS32 const *PCVMSVGAINFOFLAGS32;
void vmsvga3dInfoU32Flags(PCDBGFINFOHLP pHlp, uint32_t fFlags, const char *pszPrefix, PCVMSVGAINFOFLAGS32 paFlags, uint32_t cFlags);

/**
 * Structure for use with vmsvgaFormatEnumValueEx and vmsvgaFormatEnumValue.
 */
typedef struct VMSVGAINFOENUM
{
    /** The enum value. */
    int32_t     iValue;
    /** The corresponding value name. */
    const char *pszName;
} VMSVGAINFOENUM;
/** Pointer to a read-only enum value translation entry. */
typedef VMSVGAINFOENUM const *PCVMSVGAINFOENUM;
/**
 * Structure for use with vmsvgaFormatEnumValueEx and vmsvgaFormatEnumValue.
 */
typedef struct VMSVGAINFOENUMMAP
{
    /** Pointer to the value mapping array. */
    PCVMSVGAINFOENUM    paValues;
    /** The number of value mappings. */
    size_t              cValues;
    /** The prefix. */
    const char         *pszPrefix;
#ifdef RT_STRICT
    /** Indicates whether we've checked that it's sorted or not. */
    bool               *pfAsserted;
#endif
} VMSVGAINFOENUMMAP;
typedef VMSVGAINFOENUMMAP const *PCVMSVGAINFOENUMMAP;
/** @def VMSVGAINFOENUMMAP_MAKE
 * Macro for defining a VMSVGAINFOENUMMAP, silently dealing with pfAsserted.
 *
 * @param   a_Scope     The scope. RT_NOTHING or static.
 * @param   a_VarName   The variable name for this map.
 * @param   a_aValues   The variable name of the value mapping array.
 * @param   a_pszPrefix The value name prefix.
 */
#ifdef VBOX_STRICT
# define VMSVGAINFOENUMMAP_MAKE(a_Scope, a_VarName, a_aValues, a_pszPrefix) \
    static bool RT_CONCAT(a_VarName,_AssertedSorted) = false; \
    a_Scope VMSVGAINFOENUMMAP const a_VarName = { \
        a_aValues, RT_ELEMENTS(a_aValues), a_pszPrefix, &RT_CONCAT(a_VarName,_AssertedSorted) \
    }
#else
# define VMSVGAINFOENUMMAP_MAKE(a_Scope, a_VarName, a_aValues, a_pszPrefix) \
    a_Scope VMSVGAINFOENUMMAP const a_VarName = { a_aValues, RT_ELEMENTS(a_aValues), a_pszPrefix }
#endif
extern VMSVGAINFOENUMMAP const g_SVGA3dSurfaceFormat2String;
const char *vmsvgaLookupEnum(int32_t iValue, PCVMSVGAINFOENUMMAP pEnumMap);
char *vmsvgaFormatEnumValueEx(char *pszBuffer, size_t cbBuffer, const char *pszName, int32_t iValue,
                              bool fPrefix, PCVMSVGAINFOENUMMAP pEnumMap);
char *vmsvgaFormatEnumValue(char *pszBuffer, size_t cbBuffer, const char *pszName, uint32_t uValue,
                            const char *pszPrefix, const char * const *papszValues, size_t cValues);

/**
 * ASCII "art" scanline printer callback.
 *
 * @param   pszLine         The line to output.
 * @param   pvUser          The user argument.
 */
typedef DECLCALLBACK(void) FMVMSVGAASCIIPRINTLN(const char *pszLine, void *pvUser);
/** Pointer to an ASCII "art" print line callback. */
typedef FMVMSVGAASCIIPRINTLN *PFMVMSVGAASCIIPRINTLN;
void vmsvga3dAsciiPrint(PFMVMSVGAASCIIPRINTLN pfnPrintLine, void *pvUser, void const *pvImage, size_t cbImage,
                        uint32_t cx, uint32_t cy, uint32_t cbScanline, SVGA3dSurfaceFormat enmFormat, bool fInvY,
                        uint32_t cchMaxX, uint32_t cchMaxY);
DECLCALLBACK(void) vmsvga3dAsciiPrintlnInfo(const char *pszLine, void *pvUser);
DECLCALLBACK(void) vmsvga3dAsciiPrintlnLog(const char *pszLine, void *pvUser);

char *vmsvga3dFormatRenderState(char *pszBuffer, size_t cbBuffer, SVGA3dRenderState const *pRenderState);
char *vmsvga3dFormatTextureState(char *pszBuffer, size_t cbBuffer, SVGA3dTextureState const *pTextureState);
void vmsvga3dInfoHostWindow(PCDBGFINFOHLP pHlp, uint64_t idHostWindow);

uint32_t vmsvga3dSurfaceFormatSize(SVGA3dSurfaceFormat format);

#ifdef LOG_ENABLED
const char *vmsvga3dGetCapString(uint32_t idxCap);
const char *vmsvga3dGet3dFormatString(uint32_t format);
const char *vmsvga3dGetRenderStateName(uint32_t state);
const char *vmsvga3dTextureStateToString(SVGA3dTextureStateName textureState);
const char *vmsvgaTransformToString(SVGA3dTransformType type);
const char *vmsvgaDeclUsage2String(SVGA3dDeclUsage usage);
const char *vmsvgaDeclType2String(SVGA3dDeclType type);
const char *vmsvgaDeclMethod2String(SVGA3dDeclMethod method);
const char *vmsvgaSurfaceType2String(SVGA3dSurfaceFormat format);
const char *vmsvga3dPrimitiveType2String(SVGA3dPrimitiveType PrimitiveType);
#endif

#endif  /* !___DEVVMWARE3D_H___ */

