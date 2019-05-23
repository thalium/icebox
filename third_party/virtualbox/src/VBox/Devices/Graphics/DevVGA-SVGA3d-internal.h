/* $Id: DevVGA-SVGA3d-internal.h $ */
/** @file
 * DevVMWare - VMWare SVGA device - 3D part, internal header.
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

#ifndef ___DevVGA_SVGA3d_internal_h
#define ___DevVGA_SVGA3d_internal_h

/*
 * Assert sane compilation environment.
 */
#ifndef IN_RING3
# error "VMSVGA3D_INCL_INTERNALS is only for ring-3 code"
#endif
#ifdef VMSVGA3D_OPENGL
# ifdef VMSVGA3D_DIRECT3D
#  error "Both VMSVGA3D_DIRECT3D and VMSVGA3D_OPENGL cannot be defined at the same time."
# endif
#elif !defined(VMSVGA3D_DIRECT3D)
# error "Either VMSVGA3D_OPENGL or VMSVGA3D_DIRECT3D must be defined."
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "DevVGA-SVGA3d.h"

#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
# ifdef VMSVGA3D_DIRECT3D
#  include <d3d9.h>
#  include <iprt/avl.h>
# else
#  include <GL/gl.h>
#  include "vmsvga_glext/wglext.h"
# endif

#elif defined(RT_OS_DARWIN)
# include <OpenGL/OpenGL.h>
# include <OpenGL/gl3.h>
# include <OpenGL/gl3ext.h>
# define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
# include <OpenGL/gl.h>
# include <OpenGL/glext.h>
# include "DevVGA-SVGA3d-cocoa.h"
/* work around conflicting definition of GLhandleARB in VMware's glext.h */
//#define GL_ARB_shader_objects
// HACK
typedef void (APIENTRYP PFNGLFOGCOORDPOINTERPROC) (GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRYP PFNGLCLIENTACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRYP PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
# define GL_RGBA_S3TC 0x83A2
# define GL_ALPHA8_EXT 0x803c
# define GL_LUMINANCE8_EXT 0x8040
# define GL_LUMINANCE16_EXT 0x8042
# define GL_LUMINANCE4_ALPHA4_EXT 0x8043
# define GL_LUMINANCE8_ALPHA8_EXT 0x8045
# define GL_INT_2_10_10_10_REV 0x8D9F

#else
# include <X11/Xlib.h>
# include <X11/Xatom.h>
# include <GL/gl.h>
# include <GL/glx.h>
# include <GL/glext.h>
# define VBOX_VMSVGA3D_GL_HACK_LEVEL 0x103
#endif

#include "vmsvga/svga3d_shaderdefs.h"
#ifdef VMSVGA3D_OPENGL
# include "vmsvga_glext/glext.h"
# include "shaderlib/shaderlib.h"
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifdef VMSVGA3D_OPENGL
/** OpenGL: Create a dedicated context for handling surfaces in, thus
 *          avoiding orphaned surfaces after context destruction.
 *
 * This cures, for instance, an assertion on fedora 21 that happens in
 * vmsvga3dSurfaceStretchBlt if the login screen and the desktop has different
 * sizes.  The context of the login screen seems to have just been destroyed
 * earlier and I believe the driver/X/whoever is attemting to strech the old
 * screen content onto the new sized screen.
 *
 * @remarks This probably comes at a slight preformance expense, as we currently
 *          switches context when setting up the surface the first time.  Not sure
 *          if we really need to, but as this is an experiment, I'm playing it safe.
 * @remarks The define has been made default, thus should no longer be used.
 */
# define VMSVGA3D_OGL_WITH_SHARED_CTX
/** Fake surface ID for the shared context. */
# define VMSVGA3D_SHARED_CTX_ID        UINT32_C(0xffffeeee)

/** @def VBOX_VMSVGA3D_GL_HACK_LEVEL
 * Turns out that on Linux gl.h may often define the first 2-4 OpenGL versions
 * worth of extensions, but missing out on a function pointer of fifteen.  This
 * causes headache for us when we use the function pointers below.  This hack
 * changes the code to call the known problematic functions directly.
 * The value is ((x)<<16 | (y))  where x and y are taken from the GL_VERSION_x_y.
 */
# ifndef VBOX_VMSVGA3D_GL_HACK_LEVEL
#  define VBOX_VMSVGA3D_GL_HACK_LEVEL   0
# endif

/** Invalid OpenGL ID. */
# define OPENGL_INVALID_ID              0

# define VMSVGA3D_CLEAR_CURRENT_CONTEXT(pState)                          \
    do { (pState)->idActiveContext = OPENGL_INVALID_ID; } while (0)

/** @def VMSVGA3D_SET_CURRENT_CONTEXT
 * Makes sure the @a pContext is the active OpenGL context.
 * @parm    pState      The VMSVGA3d state.
 * @parm    pContext    The new context.
 */
# ifdef RT_OS_WINDOWS
#  define VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext) \
    do {  \
        if ((pState)->idActiveContext != (pContext)->id) \
        { \
            BOOL fMakeCurrentRc = wglMakeCurrent((pContext)->hdc, (pContext)->hglrc); \
            Assert(fMakeCurrentRc == TRUE); RT_NOREF_PV(fMakeCurrentRc); \
            LogFlowFunc(("Changing context: %#x -> %#x\n", (pState)->idActiveContext, (pContext)->id)); \
            (pState)->idActiveContext = (pContext)->id; \
        } \
    } while (0)

# elif defined(RT_OS_DARWIN)
#  define VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext) \
    do {  \
        if ((pState)->idActiveContext != (pContext)->id) \
        { \
            vmsvga3dCocoaViewMakeCurrentContext((pContext)->cocoaView, (pContext)->cocoaContext); \
            LogFlowFunc(("Changing context: %#x -> %#x\n", (pState)->idActiveContext, (pContext)->id)); \
            (pState)->idActiveContext = (pContext)->id; \
        } \
    } while (0)
# else
#  define VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext) \
    do {  \
        if ((pState)->idActiveContext != (pContext)->id) \
        { \
            Bool fMakeCurrentRc = glXMakeCurrent((pState)->display, \
                                                 (pContext)->window, \
                                                 (pContext)->glxContext); \
            Assert(fMakeCurrentRc == True); RT_NOREF_PV(fMakeCurrentRc); \
            LogFlowFunc(("Changing context: %#x -> %#x\n", (pState)->idActiveContext, (pContext)->id)); \
            (pState)->idActiveContext = (pContext)->id; \
        } \
    } while (0)
# endif

/** @def VMSVGA3D_CLEAR_GL_ERRORS
 * Clears all pending OpenGL errors.
 *
 * If I understood this correctly, OpenGL maintains a bitmask internally and
 * glGetError gets the next bit (clearing it) from the bitmap and translates it
 * into a GL_XXX constant value which it then returns.  A single OpenGL call can
 * set more than one bit, and they stick around across calls, from what I
 * understand.
 *
 * So in order to be able to use glGetError to check whether a function
 * succeeded, we need to call glGetError until all error bits have been cleared.
 * This macro does that (in all types of builds).
 *
 * @sa VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_IS_SUCCESS
 */
# define VMSVGA3D_CLEAR_GL_ERRORS() \
    do { \
        if (RT_UNLIKELY(glGetError() != GL_NO_ERROR)) /* predict no errors pending */ \
        { \
            uint32_t iErrorClearingLoopsLeft = 64; \
            while (glGetError() != GL_NO_ERROR && iErrorClearingLoopsLeft > 0) \
                iErrorClearingLoopsLeft--; \
        } \
    } while (0)

/** @def VMSVGA3D_GET_LAST_GL_ERROR
 * Gets the last OpenGL error, stores it in a_pContext->lastError and returns
 * it.
 *
 * @returns Same as glGetError.
 * @param   a_pContext  The context to store the error in.
 *
 * @sa VMSVGA3D_GL_IS_SUCCESS, VMSVGA3D_GL_COMPLAIN
 */
# define VMSVGA3D_GET_GL_ERROR(a_pContext) ((a_pContext)->lastError = glGetError())

/** @def VMSVGA3D_GL_SUCCESS
 * Checks whether VMSVGA3D_GET_LAST_GL_ERROR() return GL_NO_ERROR.
 *
 * Will call glGetError() and store the result in a_pContext->lastError.
 * Will predict GL_NO_ERROR outcome.
 *
 * @returns True on success, false on error.
 * @parm    a_pContext  The context to store the error in.
 *
 * @sa VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_COMPLAIN
 */
# define VMSVGA3D_GL_IS_SUCCESS(a_pContext) RT_LIKELY((((a_pContext)->lastError = glGetError()) == GL_NO_ERROR))

/** @def VMSVGA3D_GL_COMPLAIN
 * Complains about one or more OpenGL errors (first in a_pContext->lastError).
 *
 * Strict builds will trigger an assertion, while other builds will put the
 * first few occurences in the release log.
 *
 * All GL errors will be cleared after invocation.  Assumes lastError
 * is an error, will not check for GL_NO_ERROR.
 *
 * @param   a_pState        The 3D state structure.
 * @param   a_pContext      The context that holds the first error.
 * @param   a_LogRelDetails Argument list for LogRel or similar that describes
 *                          the operation in greater detail.
 *
 * @sa VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_IS_SUCCESS
 */
# ifdef VBOX_STRICT
#  define VMSVGA3D_GL_COMPLAIN(a_pState, a_pContext, a_LogRelDetails) \
    do { \
        AssertMsg((a_pState)->idActiveContext == (a_pContext)->id, \
                  ("idActiveContext=%#x id=%x\n", (a_pState)->idActiveContext, (a_pContext)->id)); \
        RTAssertMsg2Weak a_LogRelDetails; \
        GLenum iNextError; \
        while ((iNextError = glGetError()) != GL_NO_ERROR) \
            RTAssertMsg2Weak("next error: %#x\n", iNextError); \
        AssertMsgFailed(("first error: %#x (idActiveContext=%#x)\n", (a_pContext)->lastError, (a_pContext)->id)); \
    } while (0)
# else
#  define VMSVGA3D_GL_COMPLAIN(a_pState, a_pContext, a_LogRelDetails) \
    do { \
        LogRelMax(32, ("VMSVGA3d: OpenGL error %#x (idActiveContext=%#x) on line %u ", (a_pContext)->lastError, (a_pContext)->id)); \
        GLenum iNextError; \
        while ((iNextError = glGetError()) != GL_NO_ERROR) \
            LogRelMax(32, (" - also error %#x ", iNextError)); \
        LogRelMax(32, a_LogRelDetails); \
    } while (0)
# endif

/** @def VMSVGA3D_GL_GET_AND_COMPLAIN
 * Combination of VMSVGA3D_GET_GL_ERROR and VMSVGA3D_GL_COMPLAIN, assuming that
 * there is a pending error.
 *
 * @param   a_pState    The 3D state structure.
 * @param   a_pContext  The context that holds the first error.
 * @param   a_LogRelDetails Argument list for LogRel or similar that describes
 *                          the operation in greater detail.
 *
 * @sa VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_IS_SUCCESS, VMSVGA3D_GL_COMPLAIN
 */
# define VMSVGA3D_GL_GET_AND_COMPLAIN(a_pState, a_pContext, a_LogRelDetails) \
    do { \
        VMSVGA3D_GET_GL_ERROR(a_pContext); \
        VMSVGA3D_GL_COMPLAIN(a_pState, a_pContext, a_LogRelDetails); \
    } while (0)

/** @def VMSVGA3D_GL_ASSERT_SUCCESS
 * Asserts that VMSVGA3D_GL_IS_SUCCESS is true, complains if not.
 *
 * Uses VMSVGA3D_GL_COMPLAIN for complaining, so check it out wrt to release
 * logging in non-strict builds.
 *
 * @param   a_pState    The 3D state structure.
 * @param   a_pContext  The context that holds the first error.
 * @param   a_LogRelDetails Argument list for LogRel or similar that describes
 *                          the operation in greater detail.
 *
 * @sa VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_IS_SUCCESS, VMSVGA3D_GL_COMPLAIN
 */
# define VMSVGA3D_GL_ASSERT_SUCCESS(a_pState, a_pContext, a_LogRelDetails) \
    if (VMSVGA3D_GL_IS_SUCCESS(a_pContext)) \
    { /* likely */ } \
    else do { \
        VMSVGA3D_GL_COMPLAIN(a_pState, a_pContext, a_LogRelDetails); \
    } while (0)

/** @def VMSVGA3D_ASSERT_GL_CALL_EX
 * Executes the specified OpenGL API call and asserts that it succeeded, variant
 * with extra logging flexibility.
 *
 * ASSUMES no GL errors pending prior to invocation - caller should use
 * VMSVGA3D_CLEAR_GL_ERRORS if uncertain.
 *
 * Uses VMSVGA3D_GL_COMPLAIN for complaining, so check it out wrt to release
 * logging in non-strict builds.
 *
 * @param   a_GlCall    Expression making an OpenGL call.
 * @param   a_pState    The 3D state structure.
 * @param   a_pContext  The context that holds the first error.
 * @param   a_LogRelDetails Argument list for LogRel or similar that describes
 *                          the operation in greater detail.
 *
 * @sa VMSVGA3D_ASSERT_GL_CALL, VMSVGA3D_GL_ASSERT_SUCCESS,
 *     VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_IS_SUCCESS, VMSVGA3D_GL_COMPLAIN
 */
# define VMSVGA3D_ASSERT_GL_CALL_EX(a_GlCall, a_pState, a_pContext, a_LogRelDetails) \
    do { \
        (a_GlCall); \
        VMSVGA3D_GL_ASSERT_SUCCESS(a_pState, a_pContext, a_LogRelDetails); \
    } while (0)

/** @def VMSVGA3D_ASSERT_GL_CALL
 * Executes the specified OpenGL API call and asserts that it succeeded.
 *
 * ASSUMES no GL errors pending prior to invocation - caller should use
 * VMSVGA3D_CLEAR_GL_ERRORS if uncertain.
 *
 * Uses VMSVGA3D_GL_COMPLAIN for complaining, so check it out wrt to release
 * logging in non-strict builds.
 *
 * @param   a_GlCall    Expression making an OpenGL call.
 * @param   a_pState    The 3D state structure.
 * @param   a_pContext  The context that holds the first error.
 *
 * @sa VMSVGA3D_ASSERT_GL_CALL_EX, VMSVGA3D_GL_ASSERT_SUCCESS,
 *     VMSVGA3D_GET_GL_ERROR, VMSVGA3D_GL_IS_SUCCESS, VMSVGA3D_GL_COMPLAIN
 */
# define VMSVGA3D_ASSERT_GL_CALL(a_GlCall, a_pState, a_pContext) \
    VMSVGA3D_ASSERT_GL_CALL_EX(a_GlCall, a_pState, a_pContext, ("%s\n", #a_GlCall))


/** @def VMSVGA3D_CHECK_LAST_ERROR
 * Checks that the last OpenGL error code indicates success.
 *
 * Will assert and return VERR_INTERNAL_ERROR in strict builds, in other
 * builds it will do nothing and is a NOOP.
 *
 * @parm    pState      The VMSVGA3d state.
 * @parm    pContext    The context.
 *
 * @todo    Replace with proper error handling, it's crazy to return
 *          VERR_INTERNAL_ERROR in strict builds and just barge on ahead in
 *          release builds.
 */
# ifdef VBOX_STRICT
#  define VMSVGA3D_CHECK_LAST_ERROR(pState, pContext) do {                   \
    Assert((pState)->idActiveContext == (pContext)->id);                    \
    (pContext)->lastError = glGetError();                                   \
    AssertMsgReturn((pContext)->lastError == GL_NO_ERROR, \
                    ("%s (%d): last error 0x%x\n", __FUNCTION__, __LINE__, (pContext)->lastError), \
                    VERR_INTERNAL_ERROR); \
    } while (0)
# else
#  define VMSVGA3D_CHECK_LAST_ERROR(pState, pContext)                        do { } while (0)
# endif

/** @def VMSVGA3D_CHECK_LAST_ERROR_WARN
 * Checks that the last OpenGL error code indicates success.
 *
 * Will assert in strict builds, otherwise it's a NOOP.
 *
 * @parm    pState      The VMSVGA3d state.
 * @parm    pContext    The new context.
 */
# ifdef VBOX_STRICT
#  define VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext) do {              \
    Assert((pState)->idActiveContext == (pContext)->id);                    \
    (pContext)->lastError = glGetError();                                   \
    AssertMsg((pContext)->lastError == GL_NO_ERROR, ("%s (%d): last error 0x%x\n", __FUNCTION__, __LINE__, (pContext)->lastError)); \
    } while (0)
# else
#  define VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext)                   do { } while (0)
# endif

#endif /* VMSVGA3D_OPENGL */

#ifdef VMSVGA3D_DIRECT3D
/* Enable to use Wine to convert D3D to opengl */
//#define VBOX_VMSVGA3D_WITH_WINE_OPENGL
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Mipmap level.
 */
typedef struct VMSVGA3DMIPMAPLEVEL
{
    /** The mipmap size: width, height and depth. */
    SVGA3dSize              mipmapSize;
    /** Width in blocks: (width + cxBlock - 1) / cxBlock. SSM: not saved, recalculated on load. */
    uint32_t                cBlocksX;
    /** Height in blocks: (height + cyBlock - 1) / cyBlock. SSM: not saved, recalculated on load. */
    uint32_t                cBlocksY;
    /** The scanline/pitch size in bytes: at least cBlocksX * cbBlock. */
    uint32_t                cbSurfacePitch;
    /** The size (in bytes) of the mipmap plane: cbSurfacePitch * cBlocksY */
    uint32_t                cbSurfacePlane;
    /** The size (in bytes) of the mipmap data when using the format the surface was
     *  defined with: cbSurfacePlane * mipmapSize.z */
    uint32_t                cbSurface;
    /** Pointer to the mipmap bytes (cbSurface).  Often NULL.  If the surface has
     * been realized in hardware, this may be outdated. */
    void                   *pSurfaceData;
    /** Set if pvSurfaceData contains data not realized in hardware or pushed to the
     * hardware surface yet. */
    bool                    fDirty;
} VMSVGA3DMIPMAPLEVEL;
/** Pointer to a mipmap level. */
typedef VMSVGA3DMIPMAPLEVEL *PVMSVGA3DMIPMAPLEVEL;


#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGA3DMIPMAPLEVEL structure.
 */
static SSMFIELD const g_aVMSVGA3DMIPMAPLEVELFields[] =
{
    SSMFIELD_ENTRY(                 VMSVGA3DMIPMAPLEVEL, mipmapSize),
    SSMFIELD_ENTRY(                 VMSVGA3DMIPMAPLEVEL, cbSurface),
    SSMFIELD_ENTRY(                 VMSVGA3DMIPMAPLEVEL, cbSurfacePitch),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DMIPMAPLEVEL, pSurfaceData),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DMIPMAPLEVEL, fDirty),
    SSMFIELD_ENTRY_TERM()
};
#endif

typedef struct VMSVGATRANSFORMSTATE
{
    bool        fValid;
    float       matrix[16];
} VMSVGATRANSFORMSTATE;
typedef VMSVGATRANSFORMSTATE *PVMSVGATRANSFORMSTATE;

typedef struct VMSVGAMATERIALSTATE
{
    bool            fValid;
    SVGA3dMaterial  material;
} VMSVGAMATERIALSTATE;
typedef VMSVGAMATERIALSTATE *PVMSVGAMATERIALSTATE;

typedef struct VMSVGACLIPPLANESTATE
{
    bool            fValid;
    float           plane[4];
} VMSVGACLIPPLANESTATE;
typedef VMSVGACLIPPLANESTATE *PVMSVGACLIPPLANESTATE;

typedef struct VMSVGALIGHTSTATE
{
    bool            fEnabled;
    bool            fValidData;
    SVGA3dLightData data;
} VMSVGALIGHTSTATE;
typedef VMSVGALIGHTSTATE *PVMSVGALIGHTSTATE;

typedef struct VMSVGASHADERCONST
{
    bool                    fValid;
    SVGA3dShaderConstType   ctype;
    uint32_t                value[4];
} VMSVGASHADERCONST;
typedef VMSVGASHADERCONST *PVMSVGASHADERCONST;

#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGASHADERCONST structure.
 */
static SSMFIELD const g_aVMSVGASHADERCONSTFields[] =
{
    SSMFIELD_ENTRY(                 VMSVGASHADERCONST, fValid),
    SSMFIELD_ENTRY(                 VMSVGASHADERCONST, ctype),
    SSMFIELD_ENTRY(                 VMSVGASHADERCONST, value),
    SSMFIELD_ENTRY_TERM()
};
#endif

#ifdef VMSVGA3D_DIRECT3D

/* What kind of Direct3D resource has been created for the VMSVGA3D surface. */
typedef enum VMSVGA3DD3DRESTYPE
{
    VMSVGA3D_D3DRESTYPE_NONE           = 0,
    VMSVGA3D_D3DRESTYPE_SURFACE        = 1,
    VMSVGA3D_D3DRESTYPE_TEXTURE        = 2,
    VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE   = 3,
    VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE = 4,
    VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER  = 5,
    VMSVGA3D_D3DRESTYPE_INDEX_BUFFER   = 6
} VMSVGA3DD3DRESTYPE;

/**
 *
 */
typedef struct
{
    /** Key is context id. */
    AVLU32NODECORE          Core;
    union
    {
        IDirect3DSurface9          *pSurface;
        IDirect3DTexture9          *pTexture;
        IDirect3DCubeTexture9      *pCubeTexture;
        IDirect3DVolumeTexture9    *pVolumeTexture;
    } u;
} VMSVGA3DSHAREDSURFACE;
typedef VMSVGA3DSHAREDSURFACE *PVMSVGA3DSHAREDSURFACE;
#endif /* VMSVGA3D_DIRECT3D  */

/**
 * VMSVGA3d surface.
 */
typedef struct VMSVGA3DSURFACE
{
    uint32_t                id;
#ifdef VMSVGA3D_OPENGL
    uint32_t                idWeakContextAssociation;
#else
    uint32_t                idAssociatedContext;
#endif
    uint32_t                surfaceFlags;
    SVGA3dSurfaceFormat     format;
#ifdef VMSVGA3D_OPENGL
    GLint                   internalFormatGL;
    GLint                   formatGL;
    GLint                   typeGL;
    union
    {
        GLuint              texture;
        GLuint              buffer;
        GLuint              renderbuffer;
    } oglId;
#endif
    SVGA3dSurfaceFace       faces[SVGA3D_MAX_SURFACE_FACES];
    uint32_t                cFaces;
    uint32_t                cMipmapLevels;
    PVMSVGA3DMIPMAPLEVEL    pMipmapLevels;
    uint32_t                multiSampleCount;
    SVGA3dTextureFilter     autogenFilter;
#ifdef VMSVGA3D_DIRECT3D
    D3DFORMAT               formatD3D;
    DWORD                   fUsageD3D;
    D3DMULTISAMPLE_TYPE     multiSampleTypeD3D;
#endif

    uint32_t                cbBlock;        /* block/pixel size in bytes */
    /* Dimensions of the surface block, usually 1x1 except for compressed formats. */
    uint32_t                cxBlock;        /* Block width in pixels. SSM: not saved, recalculated on load. */
    uint32_t                cyBlock;        /* Block height in pixels. SSM: not saved, recalculated on load. */

    /* Dirty state; surface was manually updated. */
    bool                    fDirty;

#ifdef VMSVGA3D_DIRECT3D
    /* Handle for shared objects (currently only textures & render targets). */
    HANDLE                  hSharedObject;
    /** Event query inserted after each GPU operation that updates or uses this surface. */
    IDirect3DQuery9        *pQuery;
    /* The type of actually created D3D resource. */
    VMSVGA3DD3DRESTYPE      enmD3DResType;
    union
    {
        IDirect3DSurface9          *pSurface;
        IDirect3DTexture9          *pTexture;
        IDirect3DCubeTexture9      *pCubeTexture;
        IDirect3DVolumeTexture9    *pVolumeTexture;
        IDirect3DVertexBuffer9     *pVertexBuffer;
        IDirect3DIndexBuffer9      *pIndexBuffer;
    } u;
    union
    {
        IDirect3DTexture9          *pTexture;
        IDirect3DCubeTexture9      *pCubeTexture;
        IDirect3DVolumeTexture9    *pVolumeTexture;
    } bounce;
    /** AVL tree containing VMSVGA3DSHAREDSURFACE structures. */
    AVLU32TREE              pSharedObjectTree;
    bool                    fStencilAsTexture;
#endif
} VMSVGA3DSURFACE;
/** Pointer to a 3d surface. */
typedef VMSVGA3DSURFACE *PVMSVGA3DSURFACE;

#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGA3DSURFACE structure.
 */
static SSMFIELD const g_aVMSVGA3DSURFACEFields[] =
{
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, id),
# ifdef VMSVGA3D_OPENGL
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, idWeakContextAssociation),
# else
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, idAssociatedContext),
# endif
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, surfaceFlags),
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, format),
# ifdef VMSVGA3D_OPENGL
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, internalFormatGL),
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, formatGL),
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, typeGL),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, id),
# endif
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, faces),
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, cFaces),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSURFACE, pMipmapLevels),
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, multiSampleCount),
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, autogenFilter),
# ifdef VMSVGA3D_DIRECT3D
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, format), /** @todo format duplicated. */
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, formatD3D),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, fUsageD3D),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, multiSampleTypeD3D),
# endif
    SSMFIELD_ENTRY(                 VMSVGA3DSURFACE, cbBlock),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, fDirty),
# ifdef VMSVGA3D_DIRECT3D
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSURFACE, hSharedObject),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSURFACE, pQuery),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSURFACE, u.pSurface),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSURFACE, bounce.pTexture),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, pSharedObjectTree),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DSURFACE, fStencilAsTexture),
# endif
    SSMFIELD_ENTRY_TERM()
};
#endif

/** Mask we frequently apply to VMSVGA3DSURFACE::flags for decing what kind
 * of surface we're dealing. */
#define VMSVGA3D_SURFACE_HINT_SWITCH_MASK \
    (   SVGA3D_SURFACE_HINT_INDEXBUFFER  | SVGA3D_SURFACE_HINT_VERTEXBUFFER \
      | SVGA3D_SURFACE_HINT_TEXTURE      | SVGA3D_SURFACE_HINT_RENDERTARGET \
      | SVGA3D_SURFACE_HINT_DEPTHSTENCIL | SVGA3D_SURFACE_CUBEMAP )

/** @def VMSVGA3DSURFACE_HAS_HW_SURFACE
 * Checks whether the surface has a host hardware/library surface.
 * @returns true/false
 * @param   a_pSurface      The VMSVGA3d surface.
 */
#ifdef VMSVGA3D_DIRECT3D
# define VMSVGA3DSURFACE_HAS_HW_SURFACE(a_pSurface) ((a_pSurface)->u.pSurface != NULL)
#else
# define VMSVGA3DSURFACE_HAS_HW_SURFACE(a_pSurface) ((a_pSurface)->oglId.texture != OPENGL_INVALID_ID)
#endif



typedef struct VMSVGA3DSHADER
{
    uint32_t                        id;
    uint32_t                        cid;
    SVGA3dShaderType                type;
    uint32_t                        cbData;
    void                           *pShaderProgram;
    union
    {
#ifdef VMSVGA3D_DIRECT3D
        IDirect3DVertexShader9     *pVertexShader;
        IDirect3DPixelShader9      *pPixelShader;
#else
        void                       *pVertexShader;
        void                       *pPixelShader;
#endif
        void                       *pv;
    } u;
} VMSVGA3DSHADER;
typedef VMSVGA3DSHADER *PVMSVGA3DSHADER;

#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGA3DSHADER structure.
 */
static SSMFIELD const g_aVMSVGA3DSHADERFields[] =
{
    SSMFIELD_ENTRY(                 VMSVGA3DSHADER, id),
    SSMFIELD_ENTRY(                 VMSVGA3DSHADER, cid),
    SSMFIELD_ENTRY(                 VMSVGA3DSHADER, type),
    SSMFIELD_ENTRY(                 VMSVGA3DSHADER, cbData),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSHADER, pShaderProgram),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DSHADER, u.pv),
    SSMFIELD_ENTRY_TERM()
};
#endif

/** @name VMSVGA3D_UPDATE_XXX - ...
 * @{ */
#define VMSVGA3D_UPDATE_SCISSORRECT    RT_BIT_32(0)
#define VMSVGA3D_UPDATE_ZRANGE         RT_BIT_32(1)
#define VMSVGA3D_UPDATE_VIEWPORT       RT_BIT_32(2)
#define VMSVGA3D_UPDATE_VERTEXSHADER   RT_BIT_32(3)
#define VMSVGA3D_UPDATE_PIXELSHADER    RT_BIT_32(4)
#define VMSVGA3D_UPDATE_TRANSFORM      RT_BIT_32(5)
#define VMSVGA3D_UPDATE_MATERIAL       RT_BIT_32(6)
/** @} */

/* Query states. Mostly used for saved state. */
typedef enum VMSVGA3DQUERYSTATE
{
    VMSVGA3DQUERYSTATE_NULL     = 0,  /* Not created. */
    VMSVGA3DQUERYSTATE_SIGNALED = 1,  /* Result obtained. The guest may or may not read the result yet. */
    VMSVGA3DQUERYSTATE_BUILDING = 2,  /* In process of collecting data. */
    VMSVGA3DQUERYSTATE_ISSUED   = 3,  /* Data collected, but result is not yet obtained. */
    VMSVGA3DQUERYSTATE_32BIT    = 0x7fffffff
} VMSVGA3DQUERYSTATE;
AssertCompileSize(VMSVGA3DQUERYSTATE, sizeof(uint32_t));

typedef struct VMSVGA3DQUERY
{
#ifdef VMSVGA3D_DIRECT3D
    IDirect3DQuery9    *pQuery;
#else /* VMSVGA3D_OPENGL */
    GLuint              idQuery;
#endif
    VMSVGA3DQUERYSTATE  enmQueryState;  /* VMSVGA3DQUERYSTATE_*. State is implicitly _NULL if pQuery is NULL. */
    uint32_t            u32QueryResult; /* Generic result. Enough for all VGPU9 queries. */
} VMSVGA3DQUERY;

#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGA3DQUERY structure.
 */
static SSMFIELD const g_aVMSVGA3DQUERYFields[] =
{
#ifdef VMSVGA3D_DIRECT3D
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DQUERY, pQuery),
#else /* VMSVGA3D_OPENGL */
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DQUERY, idQuery),
#endif
    SSMFIELD_ENTRY(                 VMSVGA3DQUERY, enmQueryState),
    SSMFIELD_ENTRY(                 VMSVGA3DQUERY, u32QueryResult),
    SSMFIELD_ENTRY_TERM()
};
#endif

#ifdef VMSVGA3D_DIRECT3D
#define VMSVGA3DQUERY_EXISTS(p) ((p)->pQuery && (p)->enmQueryState != VMSVGA3DQUERYSTATE_NULL)
#else
#define VMSVGA3DQUERY_EXISTS(p) ((p)->idQuery && (p)->enmQueryState != VMSVGA3DQUERYSTATE_NULL)
#endif

/**
 * VMSVGA3d context.
 */
typedef struct VMSVGA3DCONTEXT
{
    uint32_t                id;
#ifdef RT_OS_WINDOWS
# ifdef VMSVGA3D_DIRECT3D
#  ifdef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    IDirect3DDevice9       *pDevice;
#  else
    IDirect3DDevice9Ex     *pDevice;
#  endif
# else
    /* Device context of the context window. */
    HDC                     hdc;
    /* OpenGL rendering context handle. */
    HGLRC                   hglrc;
# endif
    /* Device context window handle. */
    HWND                    hwnd;
#elif defined(RT_OS_DARWIN)
    /* OpenGL rendering context */
    NativeNSOpenGLContextRef cocoaContext;
    NativeNSViewRef          cocoaView;
    bool                     fOtherProfile;
#else
    /** XGL rendering context handle */
    GLXContext              glxContext;
    /** Device context window handle */
    Window                  window;
    /** flag whether the window is mapped (=visible) */
    bool                    fMapped;
#endif

#ifdef VMSVGA3D_OPENGL
    /* Framebuffer object associated with this context. */
    GLuint                  idFramebuffer;
    /* Read and draw framebuffer objects for various operations. */
    GLuint                  idReadFramebuffer;
    GLuint                  idDrawFramebuffer;
    /* Last GL error recorded. */
    GLenum                  lastError;
    void                   *pShaderContext;
#endif

    /* Current active render target (if any) */
    uint32_t                sidRenderTarget;
    /* Current selected texture surfaces (if any) */
    uint32_t                aSidActiveTextures[SVGA3D_MAX_SAMPLERS];
    /* Per context pixel and vertex shaders. */
    uint32_t                cPixelShaders;
    PVMSVGA3DSHADER         paPixelShader;
    uint32_t                cVertexShaders;
    PVMSVGA3DSHADER         paVertexShader;
    /* Keep track of the internal state to be able to recreate the context properly (save/restore, window resize). */
    struct
    {
        /** VMSVGA3D_UPDATE_XXX */
        uint32_t                u32UpdateFlags;

        SVGA3dRenderState       aRenderState[SVGA3D_RS_MAX];
        /* aTextureStates contains both TextureStageStates and SamplerStates, therefore [SVGA3D_MAX_SAMPLERS]. */
        SVGA3dTextureState      aTextureStates[SVGA3D_MAX_SAMPLERS][SVGA3D_TS_MAX];
        VMSVGATRANSFORMSTATE    aTransformState[SVGA3D_TRANSFORM_MAX];
        VMSVGAMATERIALSTATE     aMaterial[SVGA3D_FACE_MAX];
        VMSVGACLIPPLANESTATE    aClipPlane[SVGA3D_CLIPPLANE_MAX];
        VMSVGALIGHTSTATE        aLightData[SVGA3D_MAX_LIGHTS];

        uint32_t                aRenderTargets[SVGA3D_RT_MAX];
        SVGA3dRect              RectScissor;
        SVGA3dRect              RectViewPort;
        SVGA3dZRange            zRange;
        uint32_t                shidPixel;
        uint32_t                shidVertex;

        uint32_t                cPixelShaderConst;
        PVMSVGASHADERCONST      paPixelShaderConst;
        uint32_t                cVertexShaderConst;
        PVMSVGASHADERCONST      paVertexShaderConst;
    } state;

    /* Occlusion query. */
    VMSVGA3DQUERY occlusion;
} VMSVGA3DCONTEXT;
/** Pointer to a VMSVGA3d context. */
typedef VMSVGA3DCONTEXT *PVMSVGA3DCONTEXT;

#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGA3DCONTEXT structure.
 */
static SSMFIELD const g_aVMSVGA3DCONTEXTFields[] =
{
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, id),

# ifdef RT_OS_WINDOWS
#  ifdef VMSVGA3D_DIRECT3D
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DCONTEXT, pDevice),
#  else
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, hdc),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, hglrc),
#  endif
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, hwnd),
# elif defined(RT_OS_DARWIN)
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, cocoaContext),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, cocoaView),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, fOtherProfile),
# else
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, glxContext),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, window),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, fMapped),
# endif

#ifdef VMSVGA3D_OPENGL
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, idFramebuffer),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, idReadFramebuffer),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, idDrawFramebuffer),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, lastError),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DCONTEXT, pShaderContext),
#endif

    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, sidRenderTarget),
    SSMFIELD_ENTRY_IGNORE(          VMSVGA3DCONTEXT, aSidActiveTextures),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, cPixelShaders),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DCONTEXT, paPixelShader),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, cVertexShaders),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DCONTEXT, paVertexShader),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.u32UpdateFlags),

    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.aRenderState),
    SSMFIELD_ENTRY_OLD(             state.aTextureStates,
                                    sizeof(SVGA3dTextureState) * /*SVGA3D_MAX_TEXTURE_STAGE=*/ 8 * /*SVGA3D_TS_MAX=*/ 30),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.aTransformState),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.aMaterial),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.aClipPlane),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.aLightData),

    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.aRenderTargets),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.RectScissor),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.RectViewPort),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.zRange),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.shidPixel),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.shidVertex),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.cPixelShaderConst),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DCONTEXT, state.paPixelShaderConst),
    SSMFIELD_ENTRY(                 VMSVGA3DCONTEXT, state.cVertexShaderConst),
    SSMFIELD_ENTRY_IGN_HCPTR(       VMSVGA3DCONTEXT, state.paVertexShaderConst),
    SSMFIELD_ENTRY_TERM()
};
#endif /* VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS */


/**
 * VMSVGA3d state data.
 *
 * Allocated on the heap and pointed to by VMSVGAState::p3dState.
 */
typedef struct VMSVGA3DSTATE
{
    /** The size of papContexts. */
    uint32_t                cContexts;
    /** The size of papSurfaces. */
    uint32_t                cSurfaces;
    /** Contexts indexed by ID.  Grown as needed. */
    PVMSVGA3DCONTEXT       *papContexts;
    /** Surfaces indexed by ID.  Grown as needed. */
    PVMSVGA3DSURFACE       *papSurfaces;

#ifdef RT_OS_WINDOWS
# ifdef VMSVGA3D_DIRECT3D
#  ifdef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    IDirect3D9             *pD3D9;
#  else
    IDirect3D9Ex           *pD3D9;
#  endif
    D3DCAPS9                caps;
    bool                    fSupportedSurfaceINTZ;
    bool                    fSupportedSurfaceNULL;
# endif
    /** Window Thread. */
    R3PTRTYPE(RTTHREAD)     pWindowThread;
    DWORD                   idWindowThread;
    HMODULE                 hInstance;
    /** Window request semaphore. */
    RTSEMEVENT              WndRequestSem;
#elif defined(RT_OS_DARWIN)
#else
    /* The X display */
    Display                *display;
    R3PTRTYPE(RTTHREAD)     pWindowThread;
    bool                    bTerminate;
#endif

#ifdef VMSVGA3D_OPENGL
    float                   rsGLVersion;
    /* Current active context. */
    uint32_t                idActiveContext;

    struct
    {
        PFNGLISRENDERBUFFERPROC                         glIsRenderbuffer;
        PFNGLBINDRENDERBUFFERPROC                       glBindRenderbuffer;
        PFNGLDELETERENDERBUFFERSPROC                    glDeleteRenderbuffers;
        PFNGLGENRENDERBUFFERSPROC                       glGenRenderbuffers;
        PFNGLRENDERBUFFERSTORAGEPROC                    glRenderbufferStorage;
        PFNGLGETRENDERBUFFERPARAMETERIVPROC             glGetRenderbufferParameteriv;
        PFNGLISFRAMEBUFFERPROC                          glIsFramebuffer;
        PFNGLBINDFRAMEBUFFERPROC                        glBindFramebuffer;
        PFNGLDELETEFRAMEBUFFERSPROC                     glDeleteFramebuffers;
        PFNGLGENFRAMEBUFFERSPROC                        glGenFramebuffers;
        PFNGLCHECKFRAMEBUFFERSTATUSPROC                 glCheckFramebufferStatus;
        PFNGLFRAMEBUFFERTEXTURE1DPROC                   glFramebufferTexture1D;
        PFNGLFRAMEBUFFERTEXTURE2DPROC                   glFramebufferTexture2D;
        PFNGLFRAMEBUFFERTEXTURE3DPROC                   glFramebufferTexture3D;
        PFNGLFRAMEBUFFERRENDERBUFFERPROC                glFramebufferRenderbuffer;
        PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC    glGetFramebufferAttachmentParameteriv;
        PFNGLGENERATEMIPMAPPROC                         glGenerateMipmap;
        PFNGLBLITFRAMEBUFFERPROC                        glBlitFramebuffer;
        PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC         glRenderbufferStorageMultisample;
        PFNGLFRAMEBUFFERTEXTURELAYERPROC                glFramebufferTextureLayer;
        PFNGLPOINTPARAMETERFPROC                        glPointParameterf;
#if VBOX_VMSVGA3D_GL_HACK_LEVEL < 0x102
        PFNGLBLENDCOLORPROC                             glBlendColor;
        PFNGLBLENDEQUATIONPROC                          glBlendEquation;
#endif
        PFNGLBLENDEQUATIONSEPARATEPROC                  glBlendEquationSeparate;
        PFNGLBLENDFUNCSEPARATEPROC                      glBlendFuncSeparate;
        PFNGLSTENCILOPSEPARATEPROC                      glStencilOpSeparate;
        PFNGLSTENCILFUNCSEPARATEPROC                    glStencilFuncSeparate;
        PFNGLBINDBUFFERPROC                             glBindBuffer;
        PFNGLDELETEBUFFERSPROC                          glDeleteBuffers;
        PFNGLGENBUFFERSPROC                             glGenBuffers;
        PFNGLBUFFERDATAPROC                             glBufferData;
        PFNGLMAPBUFFERPROC                              glMapBuffer;
        PFNGLUNMAPBUFFERPROC                            glUnmapBuffer;
        PFNGLENABLEVERTEXATTRIBARRAYPROC                glEnableVertexAttribArray;
        PFNGLDISABLEVERTEXATTRIBARRAYPROC               glDisableVertexAttribArray;
        PFNGLVERTEXATTRIBPOINTERPROC                    glVertexAttribPointer;
        PFNGLFOGCOORDPOINTERPROC                        glFogCoordPointer;
        PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC        glDrawElementsInstancedBaseVertex;
        PFNGLDRAWELEMENTSBASEVERTEXPROC                 glDrawElementsBaseVertex;
        PFNGLACTIVETEXTUREPROC                          glActiveTexture;
#if VBOX_VMSVGA3D_GL_HACK_LEVEL < 0x103
        PFNGLCLIENTACTIVETEXTUREPROC                    glClientActiveTexture;
#endif
        PFNGLGETPROGRAMIVARBPROC                        glGetProgramivARB;
        PFNGLPROVOKINGVERTEXPROC                        glProvokingVertex;
        bool                                            fEXT_stencil_two_side;
        PFNGLGENQUERIESPROC                             glGenQueries;
        PFNGLDELETEQUERIESPROC                          glDeleteQueries;
        PFNGLBEGINQUERYPROC                             glBeginQuery;
        PFNGLENDQUERYPROC                               glEndQuery;
        PFNGLGETQUERYOBJECTUIVPROC                      glGetQueryObjectuiv;
    } ext;

    struct
    {
        GLint                           maxActiveLights;
        GLint                           maxTextureBufferSize;
        GLint                           maxTextures;
        GLint                           maxClipDistances;
        GLint                           maxColorAttachments;
        GLint                           maxRectangleTextureSize;
        GLint                           maxTextureAnisotropy;
        GLint                           maxVertexShaderInstructions;
        GLint                           maxFragmentShaderInstructions;
        GLint                           maxVertexShaderTemps;
        GLint                           maxFragmentShaderTemps;
        GLfloat                         flPointSize[2];
        SVGA3dPixelShaderVersion        fragmentShaderVersion;
        SVGA3dVertexShaderVersion       vertexShaderVersion;
        bool                            fS3TCSupported;
    } caps;

    /** The GL_EXTENSIONS value (space padded) for the default OpenGL profile.
     * Free with RTStrFree. */
    R3PTRTYPE(char *)       pszExtensions;

    /** The GL_EXTENSIONS value (space padded) for the other OpenGL profile.
     * Free with RTStrFree.
     *
     * This is used to detect shader model version since some implementations
     * (darwin) hides extensions that have made it into core and probably a
     * bunch of others when using a OpenGL core profile instead of a legacy one */
    R3PTRTYPE(char *)       pszOtherExtensions;
    /** The version of the other GL profile. */
    float                   rsOtherGLVersion;

    /** Shader talk back interface. */
    VBOXVMSVGASHADERIF      ShaderIf;

# ifdef VMSVGA3D_OPENGL
    /** The shared context. */
    VMSVGA3DCONTEXT         SharedCtx;
# endif
#endif /* VMSVGA3D_OPENGL */
} VMSVGA3DSTATE;

#ifdef VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
/**
 * SSM descriptor table for the VMSVGA3DSTATE structure.
 *
 * @remarks This isn't a complete structure markup, only fields with state.
 */
static SSMFIELD const g_aVMSVGA3DSTATEFields[] =
{
# ifdef VMSVGA3D_OPENGL
    SSMFIELD_ENTRY(                 VMSVGA3DSTATE, rsGLVersion), /** @todo Why are we saving the GL version?? */
# endif
    SSMFIELD_ENTRY(                 VMSVGA3DSTATE, cContexts),
    SSMFIELD_ENTRY(                 VMSVGA3DSTATE, cSurfaces),
    SSMFIELD_ENTRY_TERM()
};
#endif /* VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS */


#ifdef VMSVGA3D_DIRECT3D
D3DFORMAT vmsvga3dSurfaceFormat2D3D(SVGA3dSurfaceFormat format);
D3DMULTISAMPLE_TYPE vmsvga3dMultipeSampleCount2D3D(uint32_t multisampleCount);
DECLCALLBACK(int) vmsvga3dSharedSurfaceDestroyTree(PAVLU32NODECORE pNode, void *pvParam);
int vmsvga3dSurfaceFlush(PVGASTATE pThis, PVMSVGA3DSURFACE pSurface);
#endif /* VMSVGA3D_DIRECT3D */


#ifdef VMSVGA3D_OPENGL
/** Save and setup everything. */
# define VMSVGA3D_PARANOID_TEXTURE_PACKING

/**
 * Saved texture packing parameters (shared by both pack and unpack).
 */
typedef struct VMSVGAPACKPARAMS
{
    GLint       iAlignment;
    GLint       cxRow;
# ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    GLint       cyImage;
    GLboolean   fSwapBytes;
    GLboolean   fLsbFirst;
    GLint       cSkipRows;
    GLint       cSkipPixels;
    GLint       cSkipImages;
# endif
} VMSVGAPACKPARAMS;
/** Pointer to saved texture packing parameters. */
typedef VMSVGAPACKPARAMS *PVMSVGAPACKPARAMS;
/** Pointer to const saved texture packing parameters. */
typedef VMSVGAPACKPARAMS const *PCVMSVGAPACKPARAMS;

void vmsvga3dOglSetPackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                              PVMSVGAPACKPARAMS pSave);
void vmsvga3dOglRestorePackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                               PCVMSVGAPACKPARAMS pSave);
void vmsvga3dOglSetUnpackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                                PVMSVGAPACKPARAMS pSave);
void vmsvga3dOglRestoreUnpackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                                    PCVMSVGAPACKPARAMS pSave);

/** @name VMSVGA3D_DEF_CTX_F_XXX - vmsvga3dContextDefineOgl flags.
 * @{ */
/** When clear, the  context is created using the default OpenGL profile.
 * When set, it's created using the alternative profile.  The latter is only
 * allowed if the VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE is set.  */
# define VMSVGA3D_DEF_CTX_F_OTHER_PROFILE   RT_BIT_32(0)
/** Defining the shared context.  */
# define VMSVGA3D_DEF_CTX_F_SHARED_CTX      RT_BIT_32(1)
/** Defining the init time context (EMT).  */
# define VMSVGA3D_DEF_CTX_F_INIT            RT_BIT_32(2)
/** @} */
int  vmsvga3dContextDefineOgl(PVGASTATE pThis, uint32_t cid, uint32_t fFlags);
void vmsvga3dSurfaceFormat2OGL(PVMSVGA3DSURFACE pSurface, SVGA3dSurfaceFormat format);

#endif /* VMSVGA3D_OPENGL */


/* DevVGA-SVGA3d-shared.cpp: */
int vmsvga3dSaveShaderConst(PVMSVGA3DCONTEXT pContext, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype,
                            uint32_t val1, uint32_t val2, uint32_t val3, uint32_t val4);



/* Command implementation workers. */
void vmsvga3dBackSurfaceDestroy(PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface);
int  vmsvga3dBackSurfaceStretchBlt(PVGASTATE pThis, PVMSVGA3DSTATE pState,
                                   PVMSVGA3DSURFACE pDstSurface, uint32_t uDstFace, uint32_t uDstMipmap, SVGA3dBox const *pDstBox,
                                   PVMSVGA3DSURFACE pSrcSurface, uint32_t uSrcFace, uint32_t uSrcMipmap, SVGA3dBox const *pSrcBox,
                                   SVGA3dStretchBltMode enmMode, PVMSVGA3DCONTEXT pContext);
int  vmsvga3dBackSurfaceDMACopyBox(PVGASTATE pThis, PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface,
                                   PVMSVGA3DMIPMAPLEVEL pMipLevel, uint32_t uHostFace, uint32_t uHostMipmap,
                                   SVGAGuestPtr GuestPtr, uint32_t cbGuestPitch, SVGA3dTransferType transfer,
                                   SVGA3dCopyBox const *pBox, PVMSVGA3DCONTEXT pContext, int rc, int iBox);

int  vmsvga3dBackCreateTexture(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t idAssociatedContext,
                               PVMSVGA3DSURFACE pSurface);

void vmsvgaClipCopyBox(const SVGA3dSize *pSizeSrc,
                       const SVGA3dSize *pSizeDest,
                       SVGA3dCopyBox *pBox);
void vmsvgaClipBox(const SVGA3dSize *pSize,
                   SVGA3dBox *pBox);

DECLINLINE(int) vmsvga3dContextFromCid(PVMSVGA3DSTATE pState, uint32_t cid, PVMSVGA3DCONTEXT *ppContext)
{
    /** @todo stricter checks for associated context */
    if (   cid >= pState->cContexts
        || pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSurfaceCopy invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }

    *ppContext = pState->papContexts[cid];
    return VINF_SUCCESS;
}

DECLINLINE(int) vmsvga3dSurfaceFromSid(PVMSVGA3DSTATE pState, uint32_t sid, PVMSVGA3DSURFACE *ppSurface)
{
    Assert(sid < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sid < pState->cSurfaces, VERR_INVALID_PARAMETER);
    PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
    AssertReturn(pSurface && pSurface->id == sid, VERR_INVALID_PARAMETER);
    *ppSurface = pSurface;
    return VINF_SUCCESS;
}

DECLINLINE(int) vmsvga3dMipmapLevel(PVMSVGA3DSURFACE pSurface, uint32_t face, uint32_t mipmap,
                                    PVMSVGA3DMIPMAPLEVEL *ppMipmapLevel)
{
    /* Can use faces[0].numMipLevels, because numMipLevels is the same for all faces. */
    const uint32_t numMipLevels = pSurface->faces[0].numMipLevels;

    AssertMsgReturn(face < pSurface->cFaces,
                    ("cFaces %d, face %d\n", pSurface->cFaces, face),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(mipmap < numMipLevels,
                    ("numMipLevels %d, mipmap %d", numMipLevels, mipmap),
                    VERR_INVALID_PARAMETER);

    *ppMipmapLevel = &pSurface->pMipmapLevels[face * numMipLevels + mipmap];
    return VINF_SUCCESS;
}

#ifdef VMSVGA3D_DIRECT3D
DECLINLINE(D3DCUBEMAP_FACES) vmsvga3dCubemapFaceFromIndex(uint32_t iFace)
{
    D3DCUBEMAP_FACES Face;
    switch (iFace)
    {
        case 0: Face = D3DCUBEMAP_FACE_POSITIVE_X; break;
        case 1: Face = D3DCUBEMAP_FACE_NEGATIVE_X; break;
        case 2: Face = D3DCUBEMAP_FACE_POSITIVE_Y; break;
        case 3: Face = D3DCUBEMAP_FACE_NEGATIVE_Y; break;
        case 4: Face = D3DCUBEMAP_FACE_POSITIVE_Z; break;
        default:
        case 5: Face = D3DCUBEMAP_FACE_NEGATIVE_Z; break;
    }
    return Face;
}
#endif /* VMSVGA3D_DIRECT3D */

int vmsvga3dOcclusionQueryCreate(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext);
int vmsvga3dOcclusionQueryDelete(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext);
int vmsvga3dOcclusionQueryBegin(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext);
int vmsvga3dOcclusionQueryEnd(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext);
int vmsvga3dOcclusionQueryGetData(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t *pu32Pixels);

#endif

