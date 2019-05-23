/* $Id: cr_blitter.h $ */
/** @file
 * Blitter API.
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

#ifndef ___cr_blitter_h
#define ___cr_blitter_h

#include <iprt/cdefs.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include "cr_vreg.h"
#ifdef IN_VMSVGA3D
# include <iprt/assert.h>
typedef struct TODO_VMSVGA3D_DISPATCH_TABLE SPUDispatchTable;
# include <OpenGL/OpenGL.h>
#else
# include "cr_spu.h"
#endif

/** @todo r=bird: VBOXBLITTERDECL makes no sense. */
#ifndef IN_RING0
# define VBOXBLITTERDECL(_type) DECLEXPORT(_type)
#else
# define VBOXBLITTERDECL(_type) RTDECL(_type)
#endif

RT_C_DECLS_BEGIN
typedef struct CR_BLITTER_IMG
{
    void *pvData;
    GLuint cbData;
    GLenum enmFormat;
    GLuint width, height;
    GLuint bpp;
    GLuint pitch;
} CR_BLITTER_IMG;
typedef CR_BLITTER_IMG *PCR_BLITTER_IMG;
typedef CR_BLITTER_IMG const *PCCR_BLITTER_IMG;

VBOXBLITTERDECL(void) CrMClrFillImgRect(PCR_BLITTER_IMG pDst, PCRTRECT pCopyRect, uint32_t u32Color);
VBOXBLITTERDECL(void) CrMClrFillImg(PCR_BLITTER_IMG pImg, uint32_t cRects, PCRTRECT pRects, uint32_t u32Color);
VBOXBLITTERDECL(void) CrMBltImgRect(PCCR_BLITTER_IMG pSrc, PCRTPOINT pSrcDataPoint, bool fSrcInvert, PCRTRECT pCopyRect,
                                    PCR_BLITTER_IMG pDst);
VBOXBLITTERDECL(void) CrMBltImg(PCCR_BLITTER_IMG pSrc, PCRTPOINT pPos, uint32_t cRects, PCRTRECT pRects, PCR_BLITTER_IMG pDst);
VBOXBLITTERDECL(void) CrMBltImgRectScaled(PCCR_BLITTER_IMG pSrc, PCRTPOINT pPos, bool fSrcInvert, PCRTRECT pCopyRect,
                                          float strX, float strY, PCR_BLITTER_IMG pDst);
VBOXBLITTERDECL(void) CrMBltImgScaled(PCCR_BLITTER_IMG pSrc, PCRTRECTSIZE pSrcRectSize, PCRTRECT pDstRect, uint32_t cRects,
                                      PCRTRECT pRects, PCR_BLITTER_IMG pDst);

/*
 * GLSL Cache
 */
typedef struct CR_GLSL_CACHE
{
    int iGlVersion;
    GLuint uNoAlpha2DProg;
    GLuint uNoAlpha2DRectProg;
    SPUDispatchTable *pDispatch;
} CR_GLSL_CACHE;
typedef CR_GLSL_CACHE *PCR_GLSL_CACHE;
typedef CR_GLSL_CACHE const *PCCR_GLSL_CACHE;

DECLINLINE(void) CrGlslInit(PCR_GLSL_CACHE pCache, SPUDispatchTable *pDispatch)
{
    RT_ZERO(*pCache);
    pCache->pDispatch = pDispatch;
}

DECLINLINE(bool) CrGlslIsInited(PCCR_GLSL_CACHE pCache)
{
    return !!pCache->pDispatch;
}

/* clients should set proper context before calling these funcs */
VBOXBLITTERDECL(bool) CrGlslIsSupported(PCR_GLSL_CACHE pCache);
VBOXBLITTERDECL(int)  CrGlslProgGenAllNoAlpha(PCR_GLSL_CACHE pCache);
VBOXBLITTERDECL(int)  CrGlslProgGenNoAlpha(PCR_GLSL_CACHE pCache, GLenum enmTexTarget);
VBOXBLITTERDECL(int)  CrGlslProgUseGenNoAlpha(PCR_GLSL_CACHE pCache, GLenum enmTexTarget);
VBOXBLITTERDECL(int)  CrGlslProgUseNoAlpha(PCCR_GLSL_CACHE pCache, GLenum enmTexTarget);
VBOXBLITTERDECL(void) CrGlslProgClear(PCCR_GLSL_CACHE pCache);
VBOXBLITTERDECL(bool) CrGlslNeedsCleanup(PCCR_GLSL_CACHE pCache);
VBOXBLITTERDECL(void) CrGlslCleanup(PCR_GLSL_CACHE pCache);
VBOXBLITTERDECL(void) CrGlslTerm(PCR_GLSL_CACHE pCache);

/*
 * BLITTER
 */
typedef struct CR_BLITTER_BUFFER
{
    GLuint cbBuffer;
    GLvoid *pvBuffer;
} CR_BLITTER_BUFFER;
typedef CR_BLITTER_BUFFER *PCR_BLITTER_BUFFER;
typedef CR_BLITTER_BUFFER const *PCCR_BLITTER_BUFFER;

typedef union CR_BLITTER_FLAGS
{
    struct
    {
        uint32_t Initialized         : 1;
        uint32_t CtxCreated          : 1;
        uint32_t SupportsFBO         : 1;
        uint32_t SupportsPBO         : 1;
        uint32_t CurrentMuralChanged : 1;
        uint32_t LastWasFBODraw      : 1;
        uint32_t ForceDrawBlit       : 1;
        uint32_t ShadersGloal        : 1;
        uint32_t Reserved            : 24;
    };
    uint32_t Value;
} CR_BLITTER_FLAGS;

struct CR_BLITTER;

typedef DECLCALLBACK(int) FNCRBLT_BLITTER(struct CR_BLITTER *pBlitter, PCVBOXVR_TEXTURE pSrc, PCRTRECT paSrcRect,
                                          PCRTRECTSIZE pDstSize, PCRTRECT paDstRect, uint32_t cRects, uint32_t fFlags);
typedef FNCRBLT_BLITTER *PFNCRBLT_BLITTER;

typedef struct CR_BLITTER_SPUITEM
{
    int id;
    GLint visualBits;
} CR_BLITTER_SPUITEM, *PCR_BLITTER_SPUITEM;

typedef struct CR_BLITTER_CONTEXT
{
    CR_BLITTER_SPUITEM Base;
} CR_BLITTER_CONTEXT;
typedef CR_BLITTER_CONTEXT *PCR_BLITTER_CONTEXT;
typedef CR_BLITTER_CONTEXT const *PCCR_BLITTER_CONTEXT;

typedef struct CR_BLITTER_WINDOW
{
    CR_BLITTER_SPUITEM Base;
    GLuint width, height;
} CR_BLITTER_WINDOW;
typedef CR_BLITTER_WINDOW *PCR_BLITTER_WINDOW;
typedef CR_BLITTER_WINDOW const *PCCR_BLITTER_WINDOW;

typedef struct CR_BLITTER
{
    GLuint idFBO;
    CR_BLITTER_FLAGS Flags;
    uint32_t cEnters;
    PFNCRBLT_BLITTER pfnBlt;
    CR_BLITTER_BUFFER Verticies;
    CR_BLITTER_BUFFER Indicies;
    RTRECTSIZE CurrentSetSize;
    CR_BLITTER_WINDOW CurrentMural;
    CR_BLITTER_CONTEXT CtxInfo;
    int32_t i32MakeCurrentUserData;
    SPUDispatchTable *pDispatch;
    PCCR_GLSL_CACHE pGlslCache;
    CR_GLSL_CACHE LocalGlslCache;
} CR_BLITTER;
typedef CR_BLITTER *PCR_BLITTER;
typedef CR_BLITTER const *PCCR_BLITTER;

DECLINLINE(GLboolean) CrBltIsInitialized(PCR_BLITTER pBlitter)
{
    return !!pBlitter->pDispatch;
}

VBOXBLITTERDECL(int)  CrBltInit(PCR_BLITTER pBlitter, PCCR_BLITTER_CONTEXT pCtxBase, bool fCreateNewCtx,
                                bool fForceDrawBlt, PCCR_GLSL_CACHE pShaders, SPUDispatchTable *pDispatch);

VBOXBLITTERDECL(void) CrBltTerm(PCR_BLITTER pBlitter);

VBOXBLITTERDECL(int)  CrBltCleanup(PCR_BLITTER pBlitter);

DECLINLINE(GLboolean) CrBltSupportsTexTex(PCR_BLITTER pBlitter)
{
    return pBlitter->Flags.SupportsFBO;
}

DECLINLINE(GLboolean) CrBltIsEntered(PCR_BLITTER pBlitter)
{
    return !!pBlitter->cEnters;
}

DECLINLINE(GLint) CrBltGetVisBits(PCR_BLITTER pBlitter)
{
    return pBlitter->CtxInfo.Base.visualBits;
}


DECLINLINE(GLboolean) CrBltIsEverEntered(PCR_BLITTER pBlitter)
{
    return !!pBlitter->Flags.Initialized;
}

DECLINLINE(void) CrBltSetMakeCurrentUserData(PCR_BLITTER pBlitter, int32_t i32MakeCurrentUserData)
{
    pBlitter->i32MakeCurrentUserData = i32MakeCurrentUserData;
}

VBOXBLITTERDECL(int) CrBltMuralSetCurrentInfo(PCR_BLITTER pBlitter, PCCR_BLITTER_WINDOW pMural);

DECLINLINE(PCCR_BLITTER_WINDOW) CrBltMuralGetCurrentInfo(PCR_BLITTER pBlitter)
{
    return &pBlitter->CurrentMural;
}

VBOXBLITTERDECL(void) CrBltCheckUpdateViewport(PCR_BLITTER pBlitter);

VBOXBLITTERDECL(void) CrBltLeave(PCR_BLITTER pBlitter);
VBOXBLITTERDECL(int) CrBltEnter(PCR_BLITTER pBlitter);
VBOXBLITTERDECL(void) CrBltBlitTexMural(PCR_BLITTER pBlitter, bool fBb, PCVBOXVR_TEXTURE pSrc, PCRTRECT paSrcRects,
                                        PCRTRECT paDstRects, uint32_t cRects, uint32_t fFlags);
VBOXBLITTERDECL(void) CrBltBlitTexTex(PCR_BLITTER pBlitter, PCVBOXVR_TEXTURE pSrc, PCRTRECT pSrcRect, PCVBOXVR_TEXTURE pDst,
                                      PCRTRECT pDstRect, uint32_t cRects, uint32_t fFlags);
VBOXBLITTERDECL(int) CrBltImgGetTex(PCR_BLITTER pBlitter, PCVBOXVR_TEXTURE pSrc, GLenum enmFormat, PCR_BLITTER_IMG pDst);

VBOXBLITTERDECL(int) CrBltImgGetMural(PCR_BLITTER pBlitter, bool fBb, PCR_BLITTER_IMG pDst);
VBOXBLITTERDECL(void) CrBltImgFree(PCR_BLITTER pBlitter, PCR_BLITTER_IMG pDst);
VBOXBLITTERDECL(void) CrBltPresent(PCR_BLITTER pBlitter);
/* */
struct CR_TEXDATA;

typedef DECLCALLBACK(void) FNCRTEXDATA_RELEASED(struct CR_TEXDATA *pTexture);
typedef FNCRTEXDATA_RELEASED *PFNCRTEXDATA_RELEASED;

typedef union CR_TEXDATA_FLAGS
{
    struct
    {
        uint32_t DataValid           : 1;
        uint32_t DataAcquired        : 1;
        uint32_t DataInverted        : 1;
        uint32_t Entered             : 1;
        uint32_t Reserved            : 28;
    };
    uint32_t Value;
} CR_TEXDATA_FLAGS;


typedef struct CR_TEXDATA
{
    VBOXVR_TEXTURE Tex;
    volatile uint32_t cRefs;
    /* fields specific to texture data download */
    uint32_t idInvertTex;
    uint32_t idPBO;
    CR_TEXDATA_FLAGS Flags;
    PCR_BLITTER pBlitter;
    CR_BLITTER_IMG Img;
    /*dtor*/
    PFNCRTEXDATA_RELEASED pfnTextureReleased;
    struct CR_TEXDATA *pScaledCache;
} CR_TEXDATA;
typedef CR_TEXDATA *PCR_TEXDATA;
typedef CR_TEXDATA const *PCCR_TEXDATA;

DECLINLINE(void) CrTdInit(PCR_TEXDATA pTex, PCVBOXVR_TEXTURE pVrTex, PCR_BLITTER pBlitter, PFNCRTEXDATA_RELEASED pfnTextureReleased)
{
    memset(pTex, 0, sizeof (*pTex));
    pTex->Tex = *pVrTex;
    pTex->cRefs = 1;
    pTex->pBlitter = pBlitter;
    pTex->pfnTextureReleased = pfnTextureReleased;
}

DECLINLINE(PCVBOXVR_TEXTURE) CrTdTexGet(PCCR_TEXDATA pTex)
{
    return &pTex->Tex;
}

DECLINLINE(PCR_BLITTER) CrTdBlitterGet(PCR_TEXDATA pTex)
{
    return pTex->pBlitter;
}

DECLINLINE(int) CrTdBltEnter(PCR_TEXDATA pTex)
{
    int rc;
    if (pTex->Flags.Entered)
        return VERR_INVALID_STATE;
    rc = CrBltEnter(pTex->pBlitter);
#ifdef IN_VMSVGA3D
    AssertRCReturn(rc, rc);
#else
    if (!RT_SUCCESS(rc))
    {
        WARN(("CrBltEnter failed rc %d", rc));
        return rc;
    }
#endif
    pTex->Flags.Entered = 1;
    return VINF_SUCCESS;
}

DECLINLINE(bool) CrTdBltIsEntered(PCR_TEXDATA pTex)
{
    return pTex->Flags.Entered;
}

DECLINLINE(void) CrTdBltLeave(PCR_TEXDATA pTex)
{
#ifdef IN_VMSVGA3D
    AssertReturnVoid(pTex->Flags.Entered);
#else
    if (!pTex->Flags.Entered)
    {
        WARN(("invalid Blt Leave"));
        return;
    }
#endif

    CrBltLeave(pTex->pBlitter);

    pTex->Flags.Entered = 0;
}

/* the CrTdBltXxx calls are done with the entered blitter */
/** Acquire the texture data, returns the cached data in case it is cached.
 * The data remains cached in the CR_TEXDATA object until it is discarded with
 * CrTdBltDataFree or CrTdBltDataCleanup. */
VBOXBLITTERDECL(int) CrTdBltDataAcquire(PCR_TEXDATA pTex, GLenum enmFormat, bool fInverted, PCCR_BLITTER_IMG *ppImg);

VBOXBLITTERDECL(int) CrTdBltDataAcquireScaled(PCR_TEXDATA pTex, GLenum enmFormat, bool fInverted,
                                              uint32_t width, uint32_t height, PCCR_BLITTER_IMG *ppImg);

VBOXBLITTERDECL(int) CrTdBltDataReleaseScaled(PCR_TEXDATA pTex, PCCR_BLITTER_IMG pImg);

VBOXBLITTERDECL(void) CrTdBltScaleCacheMoveTo(PCR_TEXDATA pTex, PCR_TEXDATA pDstTex);

/** Release the texture data, the data remains cached in the CR_TEXDATA object
 * until it is discarded with CrTdBltDataFree or CrTdBltDataCleanup. */
VBOXBLITTERDECL(int) CrTdBltDataRelease(PCR_TEXDATA pTex);
/** Discard the texture data cached with previous CrTdBltDataAcquire.
 * Must be called wit data released (CrTdBltDataRelease). */
VBOXBLITTERDECL(int) CrTdBltDataFree(PCR_TEXDATA pTex);
VBOXBLITTERDECL(int) CrTdBltDataFreeNe(PCR_TEXDATA pTex);
VBOXBLITTERDECL(void) CrTdBltDataInvalidateNe(PCR_TEXDATA pTex);
/** Does same as CrTdBltDataFree, and in addition cleans up.
 * This is kind of a texture destructor, which clients should call on texture object destruction,
 * e.g. from the PFNCRTEXDATA_RELEASED callback. */
VBOXBLITTERDECL(int) CrTdBltDataCleanup(PCR_TEXDATA pTex);

VBOXBLITTERDECL(int) CrTdBltDataCleanupNe(PCR_TEXDATA pTex);

DECLINLINE(uint32_t) CrTdAddRef(PCR_TEXDATA pTex)
{
    return ASMAtomicIncU32(&pTex->cRefs);
}

DECLINLINE(uint32_t) CrTdRelease(PCR_TEXDATA pTex)
{
    uint32_t cRefs = ASMAtomicDecU32(&pTex->cRefs);
    if (!cRefs)
    {
        if (pTex->pfnTextureReleased)
            pTex->pfnTextureReleased(pTex);
        else
            CrTdBltDataCleanupNe(pTex);
    }

    return cRefs;
}

RT_C_DECLS_END

#endif

