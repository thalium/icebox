/* $Id: DevVGA-SVGA3d-ogl.cpp $ */
/** @file
 * DevVMWare - VMWare SVGA device
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
/* Enable to disassemble defined shaders. (Windows host only) */
#if defined(RT_OS_WINDOWS) && defined(DEBUG) && 0 /* Disabled as we don't have the DirectX SDK avaible atm. */
# define DUMP_SHADER_DISASSEMBLY
#endif
#ifdef DEBUG_bird
//# define RTMEM_WRAP_TO_EF_APIS
#endif
#define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#include <VBox/vmm/pdmdev.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/vmm/pgm.h>

#include <iprt/assert.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>
#include <iprt/mem.h>

#include <VBoxVideo.h> /* required by DevVGA.h */

/* should go BEFORE any other DevVGA include to make all DevVGA.h config defines be visible */
#include "DevVGA.h"

#include "DevVGA-SVGA.h"
#include "DevVGA-SVGA3d.h"
#include "DevVGA-SVGA3d-internal.h"

#ifdef DUMP_SHADER_DISASSEMBLY
# include <d3dx9shader.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <float.h>  /* FLT_MIN */


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifndef VBOX_VMSVGA3D_DEFAULT_OGL_PROFILE
# define VBOX_VMSVGA3D_DEFAULT_OGL_PROFILE 1.0
#endif

#ifdef RT_OS_WINDOWS
# define OGLGETPROCADDRESS      wglGetProcAddress

#elif defined(RT_OS_DARWIN)
# include <dlfcn.h>
# define OGLGETPROCADDRESS      MyNSGLGetProcAddress
/** Resolves an OpenGL symbol.  */
static void *MyNSGLGetProcAddress(const char *pszSymbol)
{
    /* Another copy in shaderapi.c. */
    static void *s_pvImage = NULL;
    if (s_pvImage == NULL)
        s_pvImage = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);
    return s_pvImage ? dlsym(s_pvImage, pszSymbol) : NULL;
}

#else
# define OGLGETPROCADDRESS(x)   glXGetProcAddress((const GLubyte *)x)
#endif

/* Invert y-coordinate for OpenGL's bottom left origin. */
#define D3D_TO_OGL_Y_COORD(ptrSurface, y_coordinate)                (ptrSurface->pMipmapLevels[0].size.height - (y_coordinate))
#define D3D_TO_OGL_Y_COORD_MIPLEVEL(ptrMipLevel, y_coordinate)      (ptrMipLevel->size.height - (y_coordinate))

//#define MANUAL_FLIP_SURFACE_DATA
/* Enable to render the result of DrawPrimitive in a seperate window. */
//#define DEBUG_GFX_WINDOW


/**
 * Macro for doing something and then checking for errors during initialization.
 * Uses AssertLogRelMsg.
 */
#define VMSVGA3D_INIT_CHECKED(a_Expr) \
    do \
    { \
        a_Expr; \
        GLenum iGlError = glGetError(); \
        AssertLogRelMsg(iGlError == GL_NO_ERROR, ("VMSVGA3d: %s -> %#x\n", #a_Expr, iGlError)); \
    } while (0)

/**
 * Macro for doing something and then checking for errors during initialization,
 * doing the same in the other context when enabled.
 *
 * This will try both profiles in dual profile builds.  Caller must be in the
 * default context.
 *
 * Uses AssertLogRelMsg to indicate trouble.
 */
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
# define VMSVGA3D_INIT_CHECKED_BOTH(a_pState, a_pContext, a_pOtherCtx, a_Expr) \
    do \
    { \
        for (uint32_t i = 0; i < 64; i++) if (glGetError() == GL_NO_ERROR) break; Assert(glGetError() == GL_NO_ERROR); \
        a_Expr; \
        GLenum iGlError = glGetError(); \
        if (iGlError != GL_NO_ERROR) \
        { \
            VMSVGA3D_SET_CURRENT_CONTEXT(a_pState, a_pOtherCtx); \
            for (uint32_t i = 0; i < 64; i++) if (glGetError() == GL_NO_ERROR) break; Assert(glGetError() == GL_NO_ERROR); \
            a_Expr; \
            GLenum iGlError2 = glGetError(); \
            AssertLogRelMsg(iGlError2 == GL_NO_ERROR, ("VMSVGA3d: %s -> %#x / %#x\n", #a_Expr, iGlError, iGlError2)); \
            VMSVGA3D_SET_CURRENT_CONTEXT(a_pState, a_pContext); \
        } \
    } while (0)
#else
# define VMSVGA3D_INIT_CHECKED_BOTH(a_pState, a_pContext, a_pOtherCtx, a_Expr) VMSVGA3D_INIT_CHECKED(a_Expr)
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/* Define the default light parameters as specified by MSDN. */
/** @todo move out; fetched from Wine */
const SVGA3dLightData vmsvga3d_default_light =
{
    SVGA3D_LIGHTTYPE_DIRECTIONAL,   /* type */
    false,                          /* inWorldSpace */
    { 1.0f, 1.0f, 1.0f, 0.0f },     /* diffuse r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f },     /* specular r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f },     /* ambient r,g,b,a, */
    { 0.0f, 0.0f, 0.0f },           /* position x,y,z */
    { 0.0f, 0.0f, 1.0f },           /* direction x,y,z */
    0.0f,                           /* range */
    0.0f,                           /* falloff */
    0.0f, 0.0f, 0.0f,               /* attenuation 0,1,2 */
    0.0f,                           /* theta */
    0.0f                            /* phi */
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int  vmsvga3dContextDestroyOgl(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext, uint32_t cid);
static void vmsvgaColor2GLFloatArray(uint32_t color, GLfloat *pRed, GLfloat *pGreen, GLfloat *pBlue, GLfloat *pAlpha);

/* Generated by VBoxDef2LazyLoad from the VBoxSVGA3D.def and VBoxSVGA3DObjC.def files. */
extern "C" int ExplicitlyLoadVBoxSVGA3D(bool fResolveAllImports, PRTERRINFO pErrInfo);


/**
 * Checks if the given OpenGL extension is supported.
 *
 * @returns true if supported, false if not.
 * @param   pState              The VMSVGA3d state.
 * @param   rsMinGLVersion      The OpenGL version that introduced this feature
 *                              into the core.
 * @param   pszWantedExtension  The name of the OpenGL extension we want padded
 *                              with one space at each end.
 * @remarks Init time only.
 */
static bool vmsvga3dCheckGLExtension(PVMSVGA3DSTATE pState, float rsMinGLVersion, const char *pszWantedExtension)
{
    RT_NOREF(rsMinGLVersion);
    /* check padding. */
    Assert(pszWantedExtension[0] == ' ');
    Assert(pszWantedExtension[1] != ' ');
    Assert(strchr(&pszWantedExtension[1], ' ') + 1 == strchr(pszWantedExtension, '\0'));

    /* Look it up. */
    bool fRet = false;
    if (strstr(pState->pszExtensions, pszWantedExtension))
        fRet = true;

    /* Temporarily.  Later start if (rsMinGLVersion != 0.0 && fActualGLVersion >= rsMinGLVersion) return true; */
#ifdef RT_OS_DARWIN
    AssertMsg(   rsMinGLVersion == 0.0
              || fRet == (pState->rsGLVersion >= rsMinGLVersion)
              || VBOX_VMSVGA3D_DEFAULT_OGL_PROFILE == 2.1,
              ("%s actual:%d min:%d fRet=%d\n",
               pszWantedExtension, (int)(pState->rsGLVersion * 10), (int)(rsMinGLVersion * 10), fRet));
#else
    AssertMsg(rsMinGLVersion == 0.0 || fRet == (pState->rsGLVersion >= rsMinGLVersion),
              ("%s actual:%d min:%d fRet=%d\n",
               pszWantedExtension, (int)(pState->rsGLVersion * 10), (int)(rsMinGLVersion * 10), fRet));
#endif
    return fRet;
}


/**
 * Outputs GL_EXTENSIONS list to the release log.
 */
static void vmsvga3dLogRelExtensions(const char *pszPrefix, const char *pszExtensions)
{
    /* OpenGL 3.0 interface (glGetString(GL_EXTENSIONS) return NULL). */
    bool fBuffered = RTLogRelSetBuffering(true);

    /*
     * Determin the column widths first.
     */
    size_t   acchWidths[4] = { 1, 1, 1, 1 };
    uint32_t i;
    const char *psz = pszExtensions;
    for (i = 0; ; i++)
    {
        while (*psz == ' ')
            psz++;
        if (!*psz)
            break;

        const char *pszEnd = strchr(psz, ' ');
        AssertBreak(pszEnd);
        size_t cch = pszEnd - psz;

        uint32_t iColumn = i % RT_ELEMENTS(acchWidths);
        if (acchWidths[iColumn] < cch)
            acchWidths[iColumn] = cch;

        psz = pszEnd;
    }

    /*
     * Output it.
     */
    LogRel(("VMSVGA3d: %sOpenGL extensions (%d):", pszPrefix, i));
    psz = pszExtensions;
    for (i = 0; ; i++)
    {
        while (*psz == ' ')
            psz++;
        if (!*psz)
            break;

        const char *pszEnd = strchr(psz, ' ');
        AssertBreak(pszEnd);
        size_t cch = pszEnd - psz;

        uint32_t iColumn = i % RT_ELEMENTS(acchWidths);
        if (iColumn == 0)
            LogRel(("\nVMSVGA3d:  %-*.*s", acchWidths[iColumn], cch, psz));
        else if (iColumn != RT_ELEMENTS(acchWidths) - 1)
            LogRel((" %-*.*s", acchWidths[iColumn], cch, psz));
        else
            LogRel((" %.*s", cch, psz));

        psz = pszEnd;
    }

    RTLogRelSetBuffering(fBuffered);
    LogRel(("\n"));
}

/**
 * Gathers the GL_EXTENSIONS list, storing it as a space padded list at
 * @a ppszExtensions.
 *
 * @returns VINF_SUCCESS or VERR_NO_STR_MEMORY
 * @param   ppszExtensions      Pointer to the string pointer. Free with RTStrFree.
 * @param   fGLProfileVersion   The OpenGL profile version.
 */
static int vmsvga3dGatherExtensions(char **ppszExtensions, float fGLProfileVersion)
{
    int rc;
    *ppszExtensions = NULL;

    /*
     * Try the old glGetString interface first.
     */
    const char *pszExtensions = (const char *)glGetString(GL_EXTENSIONS);
    if (pszExtensions)
    {
        rc = RTStrAAppendExN(ppszExtensions, 3, " ", (size_t)1, pszExtensions, RTSTR_MAX, " ", (size_t)1);
        AssertLogRelRCReturn(rc, rc);
    }
    else
    {
        /*
         * The new interface where each extension string is retrieved separately.
         * Note! Cannot use VMSVGA3D_INIT_CHECKED_GL_GET_INTEGER_VALUE here because
         *       the above GL_EXTENSIONS error lingers on darwin. sucks.
         */
#ifndef GL_NUM_EXTENSIONS
# define GL_NUM_EXTENSIONS 0x821D
#endif
        GLint cExtensions = 1024;
        glGetIntegerv(GL_NUM_EXTENSIONS, &cExtensions);
        Assert(cExtensions != 1024);

        PFNGLGETSTRINGIPROC pfnGlGetStringi = (PFNGLGETSTRINGIPROC)OGLGETPROCADDRESS("glGetStringi");
        AssertLogRelReturn(pfnGlGetStringi, VERR_NOT_SUPPORTED);

        rc = RTStrAAppend(ppszExtensions, " ");
        for (GLint i = 0; RT_SUCCESS(rc) && i < cExtensions; i++)
        {
            const char *pszExt = (const char *)pfnGlGetStringi(GL_EXTENSIONS, i);
            if (pszExt)
                rc = RTStrAAppendExN(ppszExtensions, 2, pfnGlGetStringi(GL_EXTENSIONS, i), RTSTR_MAX, " ", (size_t)1);
        }
        AssertRCReturn(rc, rc);
    }

#if 1
    /*
     * Add extensions promoted into the core OpenGL profile.
     */
    static const struct
    {
        float fGLVersion;
        const char *pszzExtensions;
    } s_aPromotedExtensions[] =
    {
        {
            1.1f,
            " GL_EXT_vertex_array \0"
            " GL_EXT_polygon_offset \0"
            " GL_EXT_blend_logic_op \0"
            " GL_EXT_texture \0"
            " GL_EXT_copy_texture \0"
            " GL_EXT_subtexture \0"
            " GL_EXT_texture_object \0"
            " GL_ARB_framebuffer_object \0"
            " GL_ARB_map_buffer_range \0"
            " GL_ARB_vertex_array_object \0"
            "\0"
        },
        {
            1.2f,
            " EXT_texture3D \0"
            " EXT_bgra \0"
            " EXT_packed_pixels \0"
            " EXT_rescale_normal \0"
            " EXT_separate_specular_color \0"
            " SGIS_texture_edge_clamp \0"
            " SGIS_texture_lod \0"
            " EXT_draw_range_elements \0"
            "\0"
        },
        {
            1.3f,
            " GL_ARB_texture_compression \0"
            " GL_ARB_texture_cube_map \0"
            " GL_ARB_multisample \0"
            " GL_ARB_multitexture \0"
            " GL_ARB_texture_env_add \0"
            " GL_ARB_texture_env_combine \0"
            " GL_ARB_texture_env_dot3 \0"
            " GL_ARB_texture_border_clamp \0"
            " GL_ARB_transpose_matrix \0"
            "\0"
        },
        {
            1.5f,
            " GL_SGIS_generate_mipmap \0"
            /*" GL_NV_blend_equare \0"*/
            " GL_ARB_depth_texture \0"
            " GL_ARB_shadow \0"
            " GL_EXT_fog_coord \0"
            " GL_EXT_multi_draw_arrays \0"
            " GL_ARB_point_parameters \0"
            " GL_EXT_secondary_color \0"
            " GL_EXT_blend_func_separate \0"
            " GL_EXT_stencil_wrap \0"
            " GL_ARB_texture_env_crossbar \0"
            " GL_EXT_texture_lod_bias \0"
            " GL_ARB_texture_mirrored_repeat \0"
            " GL_ARB_window_pos \0"
            "\0"
        },
        {
            1.6f,
            " GL_ARB_vertex_buffer_object \0"
            " GL_ARB_occlusion_query \0"
            " GL_EXT_shadow_funcs \0"
        },
        {
            2.0f,
            " GL_ARB_shader_objects \0" /*??*/
            " GL_ARB_vertex_shader \0" /*??*/
            " GL_ARB_fragment_shader \0" /*??*/
            " GL_ARB_shading_language_100 \0" /*??*/
            " GL_ARB_draw_buffers \0"
            " GL_ARB_texture_non_power_of_two \0"
            " GL_ARB_point_sprite \0"
            " GL_ATI_separate_stencil \0"
            " GL_EXT_stencil_two_side \0"
            "\0"
        },
        {
            2.1f,
            " GL_ARB_pixel_buffer_object \0"
            " GL_EXT_texture_sRGB \0"
            "\0"
        },
        {
            3.0f,
            " GL_ARB_framebuffer_object \0"
            " GL_ARB_map_buffer_range \0"
            " GL_ARB_vertex_array_object \0"
            "\0"
        },
        {
            3.1f,
            " GL_ARB_copy_buffer \0"
            " GL_ARB_uniform_buffer_object \0"
            "\0"
        },
        {
            3.2f,
            " GL_ARB_vertex_array_bgra \0"
            " GL_ARB_draw_elements_base_vertex \0"
            " GL_ARB_fragment_coord_conventions \0"
            " GL_ARB_provoking_vertex \0"
            " GL_ARB_seamless_cube_map \0"
            " GL_ARB_texture_multisample \0"
            " GL_ARB_depth_clamp \0"
            " GL_ARB_sync \0"
            " GL_ARB_geometry_shader4 \0" /*??*/
            "\0"
        },
        {
            3.3f,
            " GL_ARB_blend_func_extended \0"
            " GL_ARB_sampler_objects \0"
            " GL_ARB_explicit_attrib_location \0"
            " GL_ARB_occlusion_query2 \0"
            " GL_ARB_shader_bit_encoding \0"
            " GL_ARB_texture_rgb10_a2ui \0"
            " GL_ARB_texture_swizzle \0"
            " GL_ARB_timer_query \0"
            " GL_ARB_vertex_type_2_10_10_10_rev \0"
            "\0"
        },
        {
            4.0f,
            " GL_ARB_texture_query_lod \0"
            " GL_ARB_draw_indirect \0"
            " GL_ARB_gpu_shader5 \0"
            " GL_ARB_gpu_shader_fp64 \0"
            " GL_ARB_shader_subroutine \0"
            " GL_ARB_tessellation_shader \0"
            " GL_ARB_texture_buffer_object_rgb32 \0"
            " GL_ARB_texture_cube_map_array \0"
            " GL_ARB_texture_gather \0"
            " GL_ARB_transform_feedback2 \0"
            " GL_ARB_transform_feedback3 \0"
            "\0"
        },
        {
            4.1f,
            " GL_ARB_ES2_compatibility \0"
            " GL_ARB_get_program_binary \0"
            " GL_ARB_separate_shader_objects \0"
            " GL_ARB_shader_precision \0"
            " GL_ARB_vertex_attrib_64bit \0"
            " GL_ARB_viewport_array \0"
            "\0"
        }
    };

    uint32_t cPromoted = 0;
    for (uint32_t i = 0; i < RT_ELEMENTS(s_aPromotedExtensions) && s_aPromotedExtensions[i].fGLVersion <= fGLProfileVersion; i++)
    {
        const char *pszExt = s_aPromotedExtensions[i].pszzExtensions;
        while (*pszExt)
        {
# ifdef VBOX_STRICT
            size_t cchExt = strlen(pszExt);
            Assert(cchExt > 3);
            Assert(pszExt[0] == ' ');
            Assert(pszExt[1] != ' ');
            Assert(pszExt[cchExt - 2] != ' ');
            Assert(pszExt[cchExt - 1] == ' ');
# endif

            if (strstr(*ppszExtensions, pszExt) == NULL)
            {
                if (cPromoted++ == 0)
                {
                    rc = RTStrAAppend(ppszExtensions, " <promoted-extensions:> <promoted-extensions:> <promoted-extensions:> ");
                    AssertRCReturn(rc, rc);
                }

                rc = RTStrAAppend(ppszExtensions, pszExt);
                AssertRCReturn(rc, rc);
            }

            pszExt = strchr(pszExt, '\0') + 1;
        }
    }
#endif

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VBOXVMSVGASHADERIF,pfnSwitchInitProfile}
 */
static DECLCALLBACK(void) vmsvga3dShaderIfSwitchInitProfile(PVBOXVMSVGASHADERIF pThis, bool fOtherProfile)
{
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    PVMSVGA3DSTATE pState = RT_FROM_MEMBER(pThis, VMSVGA3DSTATE, ShaderIf);
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pState->papContexts[fOtherProfile ? 2 : 1]);
#else
    NOREF(pThis);
    NOREF(fOtherProfile);
#endif
}


/**
 * @interface_method_impl{VBOXVMSVGASHADERIF,pfnGetNextExtension}
 */
static DECLCALLBACK(bool) vmsvga3dShaderIfGetNextExtension(PVBOXVMSVGASHADERIF pThis, void **ppvEnumCtx,
                                                           char *pszBuf, size_t cbBuf, bool fOtherProfile)
{
    PVMSVGA3DSTATE pState = RT_FROM_MEMBER(pThis, VMSVGA3DSTATE, ShaderIf);
    const char    *pszCur = *ppvEnumCtx ? (const char *)*ppvEnumCtx
                          : fOtherProfile ? pState->pszOtherExtensions : pState->pszExtensions;
    while (*pszCur == ' ')
        pszCur++;
    if (!*pszCur)
        return false;

    const char *pszEnd = strchr(pszCur, ' ');
    AssertReturn(pszEnd, false);
    size_t cch = pszEnd - pszCur;
    if (cch < cbBuf)
    {
        memcpy(pszBuf, pszCur, cch);
        pszBuf[cch] = '\0';
    }
    else if (cbBuf > 0)
    {
        memcpy(pszBuf, "<overflow>", RT_MIN(sizeof("<overflow>"), cbBuf));
        pszBuf[cbBuf - 1] = '\0';
    }

    *ppvEnumCtx = (void *)pszEnd;
    return true;
}


/**
 * Initializes the VMSVGA3D state during VGA device construction.
 *
 * Failure are generally not fatal, 3D support will just be disabled.
 *
 * @returns VBox status code.
 * @param   pThis   The VGA device state where svga.p3dState will be modified.
 */
int vmsvga3dInit(PVGASTATE pThis)
{
    AssertCompile(GL_TRUE == 1);
    AssertCompile(GL_FALSE == 0);

    /*
     * Load and resolve imports from the external shared libraries.
     */
    RTERRINFOSTATIC ErrInfo;
    int rc = ExplicitlyLoadVBoxSVGA3D(true /*fResolveAllImports*/, RTErrInfoInitStatic(&ErrInfo));
    if (RT_FAILURE(rc))
    {
        LogRel(("VMSVGA3d: Error loading VBoxSVGA3D and resolving necessary functions: %Rrc - %s\n", rc, ErrInfo.Core.pszMsg));
        return rc;
    }
#ifdef RT_OS_DARWIN
    rc = ExplicitlyLoadVBoxSVGA3DObjC(true /*fResolveAllImports*/, RTErrInfoInitStatic(&ErrInfo));
    if (RT_FAILURE(rc))
    {
        LogRel(("VMSVGA3d: Error loading VBoxSVGA3DObjC and resolving necessary functions: %Rrc - %s\n", rc, ErrInfo.Core.pszMsg));
        return rc;
    }
#endif

    /*
     * Allocate the state.
     */
    pThis->svga.p3dState = (PVMSVGA3DSTATE)RTMemAllocZ(sizeof(VMSVGA3DSTATE));
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);

#ifdef RT_OS_WINDOWS
    /* Create event semaphore and async IO thread. */
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    rc = RTSemEventCreate(&pState->WndRequestSem);
    if (RT_SUCCESS(rc))
    {
        rc = RTThreadCreate(&pState->pWindowThread, vmsvga3dWindowThread, pState->WndRequestSem, 0, RTTHREADTYPE_GUI, 0,
                            "VMSVGA3DWND");
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;

        /* bail out. */
        LogRel(("VMSVGA3d: RTThreadCreate failed: %Rrc\n", rc));
        RTSemEventDestroy(pState->WndRequestSem);
    }
    else
        LogRel(("VMSVGA3d: RTSemEventCreate failed: %Rrc\n", rc));
    RTMemFree(pThis->svga.p3dState);
    pThis->svga.p3dState = NULL;
    return rc;
#else
    return VINF_SUCCESS;
#endif
}

/* We must delay window creation until the PowerOn phase. Init is too early and will cause failures. */
int vmsvga3dPowerOn(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);
    PVMSVGA3DCONTEXT pContext;
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    PVMSVGA3DCONTEXT pOtherCtx;
#endif
    int              rc;

    if (pState->rsGLVersion != 0.0)
        return VINF_SUCCESS;    /* already initialized (load state) */

    /*
     * OpenGL function calls aren't possible without a valid current context, so create a fake one here.
     */
    rc = vmsvga3dContextDefineOgl(pThis, 1, VMSVGA3D_DEF_CTX_F_INIT);
    AssertRCReturn(rc, rc);

    pContext = pState->papContexts[1];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    LogRel(("VMSVGA3d: OpenGL version: %s\n"
            "VMSVGA3d: OpenGL Vendor: %s\n"
            "VMSVGA3d: OpenGL Renderer: %s\n"
            "VMSVGA3d: OpenGL shader language version: %s\n",
            glGetString(GL_VERSION), glGetString(GL_VENDOR), glGetString(GL_RENDERER),
            glGetString(GL_SHADING_LANGUAGE_VERSION)));

    rc = vmsvga3dGatherExtensions(&pState->pszExtensions, VBOX_VMSVGA3D_DEFAULT_OGL_PROFILE);
    AssertRCReturn(rc, rc);
    vmsvga3dLogRelExtensions("", pState->pszExtensions);

    pState->rsGLVersion = atof((const char *)glGetString(GL_VERSION));


#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    /*
     * Get the extension list for the alternative profile so we can better
     * figure out the shader model and stuff.
     */
    rc = vmsvga3dContextDefineOgl(pThis, 2, VMSVGA3D_DEF_CTX_F_INIT | VMSVGA3D_DEF_CTX_F_OTHER_PROFILE);
    AssertLogRelRCReturn(rc, rc);
    pContext = pState->papContexts[1]; /* Array may have been reallocated. */

    pOtherCtx = pState->papContexts[2];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pOtherCtx);

    LogRel(("VMSVGA3d: Alternative OpenGL version: %s\n"
            "VMSVGA3d: Alternative OpenGL Vendor: %s\n"
            "VMSVGA3d: Alternative OpenGL Renderer: %s\n"
            "VMSVGA3d: Alternative OpenGL shader language version: %s\n",
            glGetString(GL_VERSION), glGetString(GL_VENDOR), glGetString(GL_RENDERER),
            glGetString(GL_SHADING_LANGUAGE_VERSION)));

    rc = vmsvga3dGatherExtensions(&pState->pszOtherExtensions, VBOX_VMSVGA3D_OTHER_OGL_PROFILE);
    AssertRCReturn(rc, rc);
    vmsvga3dLogRelExtensions("Alternative ", pState->pszOtherExtensions);

    pState->rsOtherGLVersion = atof((const char *)glGetString(GL_VERSION));

    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
#else
    pState->pszOtherExtensions = (char *)"";
    pState->rsOtherGLVersion = pState->rsGLVersion;
#endif


    if (vmsvga3dCheckGLExtension(pState, 3.0f, " GL_ARB_framebuffer_object "))
    {
        pState->ext.glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)OGLGETPROCADDRESS("glIsRenderbuffer");
        pState->ext.glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)OGLGETPROCADDRESS("glBindRenderbuffer");
        pState->ext.glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)OGLGETPROCADDRESS("glDeleteRenderbuffers");
        pState->ext.glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)OGLGETPROCADDRESS("glGenRenderbuffers");
        pState->ext.glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)OGLGETPROCADDRESS("glRenderbufferStorage");
        pState->ext.glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)OGLGETPROCADDRESS("glGetRenderbufferParameteriv");
        pState->ext.glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)OGLGETPROCADDRESS("glIsFramebuffer");
        pState->ext.glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)OGLGETPROCADDRESS("glBindFramebuffer");
        pState->ext.glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)OGLGETPROCADDRESS("glDeleteFramebuffers");
        pState->ext.glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)OGLGETPROCADDRESS("glGenFramebuffers");
        pState->ext.glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)OGLGETPROCADDRESS("glCheckFramebufferStatus");
        pState->ext.glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)OGLGETPROCADDRESS("glFramebufferTexture1D");
        pState->ext.glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)OGLGETPROCADDRESS("glFramebufferTexture2D");
        pState->ext.glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)OGLGETPROCADDRESS("glFramebufferTexture3D");
        pState->ext.glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)OGLGETPROCADDRESS("glFramebufferRenderbuffer");
        pState->ext.glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)OGLGETPROCADDRESS("glGetFramebufferAttachmentParameteriv");
        pState->ext.glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)OGLGETPROCADDRESS("glGenerateMipmap");
        pState->ext.glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)OGLGETPROCADDRESS("glBlitFramebuffer");
        pState->ext.glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)OGLGETPROCADDRESS("glRenderbufferStorageMultisample");
        pState->ext.glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)OGLGETPROCADDRESS("glFramebufferTextureLayer");
    }
    pState->ext.glPointParameterf           = (PFNGLPOINTPARAMETERFPROC)OGLGETPROCADDRESS("glPointParameterf");
    AssertMsgReturn(pState->ext.glPointParameterf, ("glPointParameterf missing"), VERR_NOT_IMPLEMENTED);
#if VBOX_VMSVGA3D_GL_HACK_LEVEL < 0x102
    pState->ext.glBlendColor                = (PFNGLBLENDCOLORPROC)OGLGETPROCADDRESS("glBlendColor");
    AssertMsgReturn(pState->ext.glBlendColor, ("glBlendColor missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glBlendEquation             = (PFNGLBLENDEQUATIONPROC)OGLGETPROCADDRESS("glBlendEquation");
    AssertMsgReturn(pState->ext.glBlendEquation, ("glBlendEquation missing"), VERR_NOT_IMPLEMENTED);
#endif
    pState->ext.glBlendEquationSeparate     = (PFNGLBLENDEQUATIONSEPARATEPROC)OGLGETPROCADDRESS("glBlendEquationSeparate");
    AssertMsgReturn(pState->ext.glBlendEquationSeparate, ("glBlendEquationSeparate missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glBlendFuncSeparate         = (PFNGLBLENDFUNCSEPARATEPROC)OGLGETPROCADDRESS("glBlendFuncSeparate");
    AssertMsgReturn(pState->ext.glBlendFuncSeparate, ("glBlendFuncSeparate missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glStencilOpSeparate         = (PFNGLSTENCILOPSEPARATEPROC)OGLGETPROCADDRESS("glStencilOpSeparate");
    AssertMsgReturn(pState->ext.glStencilOpSeparate, ("glStencilOpSeparate missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glStencilFuncSeparate       = (PFNGLSTENCILFUNCSEPARATEPROC)OGLGETPROCADDRESS("glStencilFuncSeparate");
    AssertMsgReturn(pState->ext.glStencilFuncSeparate, ("glStencilFuncSeparate missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glBindBuffer                = (PFNGLBINDBUFFERPROC)OGLGETPROCADDRESS("glBindBuffer");
    AssertMsgReturn(pState->ext.glBindBuffer, ("glBindBuffer missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glDeleteBuffers             = (PFNGLDELETEBUFFERSPROC)OGLGETPROCADDRESS("glDeleteBuffers");
    AssertMsgReturn(pState->ext.glDeleteBuffers, ("glDeleteBuffers missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glGenBuffers                = (PFNGLGENBUFFERSPROC)OGLGETPROCADDRESS("glGenBuffers");
    AssertMsgReturn(pState->ext.glGenBuffers, ("glGenBuffers missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glBufferData                = (PFNGLBUFFERDATAPROC)OGLGETPROCADDRESS("glBufferData");
    AssertMsgReturn(pState->ext.glBufferData, ("glBufferData missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glMapBuffer                 = (PFNGLMAPBUFFERPROC)OGLGETPROCADDRESS("glMapBuffer");
    AssertMsgReturn(pState->ext.glMapBuffer, ("glMapBuffer missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glUnmapBuffer               = (PFNGLUNMAPBUFFERPROC)OGLGETPROCADDRESS("glUnmapBuffer");
    AssertMsgReturn(pState->ext.glUnmapBuffer, ("glUnmapBuffer missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glEnableVertexAttribArray   = (PFNGLENABLEVERTEXATTRIBARRAYPROC)OGLGETPROCADDRESS("glEnableVertexAttribArray");
    AssertMsgReturn(pState->ext.glEnableVertexAttribArray, ("glEnableVertexAttribArray missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glDisableVertexAttribArray  = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)OGLGETPROCADDRESS("glDisableVertexAttribArray");
    AssertMsgReturn(pState->ext.glDisableVertexAttribArray, ("glDisableVertexAttribArray missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glVertexAttribPointer       = (PFNGLVERTEXATTRIBPOINTERPROC)OGLGETPROCADDRESS("glVertexAttribPointer");
    AssertMsgReturn(pState->ext.glVertexAttribPointer, ("glVertexAttribPointer missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glFogCoordPointer           = (PFNGLFOGCOORDPOINTERPROC)OGLGETPROCADDRESS("glFogCoordPointer");
    AssertMsgReturn(pState->ext.glFogCoordPointer, ("glFogCoordPointer missing"), VERR_NOT_IMPLEMENTED);
    pState->ext.glActiveTexture             = (PFNGLACTIVETEXTUREPROC)OGLGETPROCADDRESS("glActiveTexture");
    AssertMsgReturn(pState->ext.glActiveTexture, ("glActiveTexture missing"), VERR_NOT_IMPLEMENTED);
#if VBOX_VMSVGA3D_GL_HACK_LEVEL < 0x103
    pState->ext.glClientActiveTexture       = (PFNGLCLIENTACTIVETEXTUREPROC)OGLGETPROCADDRESS("glClientActiveTexture");
    AssertMsgReturn(pState->ext.glClientActiveTexture, ("glClientActiveTexture missing"), VERR_NOT_IMPLEMENTED);
#endif
    pState->ext.glGetProgramivARB           = (PFNGLGETPROGRAMIVARBPROC)OGLGETPROCADDRESS("glGetProgramivARB");
    AssertMsgReturn(pState->ext.glGetProgramivARB, ("glGetProgramivARB missing"), VERR_NOT_IMPLEMENTED);

    /* OpenGL 3.2 core */
    if (vmsvga3dCheckGLExtension(pState, 3.2f, " GL_ARB_draw_elements_base_vertex "))
    {
        pState->ext.glDrawElementsBaseVertex          = (PFNGLDRAWELEMENTSBASEVERTEXPROC)OGLGETPROCADDRESS("glDrawElementsBaseVertex");
        pState->ext.glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)OGLGETPROCADDRESS("glDrawElementsInstancedBaseVertex");
    }
    else
        LogRel(("VMSVGA3d: missing extension GL_ARB_draw_elements_base_vertex\n"));

    /* OpenGL 3.2 core */
    if (vmsvga3dCheckGLExtension(pState, 3.2f, " GL_ARB_provoking_vertex "))
    {
        pState->ext.glProvokingVertex                 = (PFNGLPROVOKINGVERTEXPROC)OGLGETPROCADDRESS("glProvokingVertex");
    }
    else
        LogRel(("VMSVGA3d: missing extension GL_ARB_provoking_vertex\n"));

    /* Extension support */
#if defined(RT_OS_DARWIN)
    /** @todo OpenGL version history suggest this, verify...  */
    pState->ext.fEXT_stencil_two_side = vmsvga3dCheckGLExtension(pState, 2.0f, " GL_EXT_stencil_two_side ");
#else
    pState->ext.fEXT_stencil_two_side = vmsvga3dCheckGLExtension(pState, 0.0f, " GL_EXT_stencil_two_side ");
#endif

    /*
     * Initialize the capabilities with sensible defaults.
     */
    pState->caps.maxActiveLights               = 1;
    pState->caps.maxTextureBufferSize          = 65536;
    pState->caps.maxTextures                   = 1;
    pState->caps.maxClipDistances              = 4;
    pState->caps.maxColorAttachments           = 1;
    pState->caps.maxRectangleTextureSize       = 2048;
    pState->caps.maxTextureAnisotropy          = 2;
    pState->caps.maxVertexShaderInstructions   = 1024;
    pState->caps.maxFragmentShaderInstructions = 1024;
    pState->caps.vertexShaderVersion           = SVGA3DVSVERSION_NONE;
    pState->caps.fragmentShaderVersion         = SVGA3DPSVERSION_NONE;
    pState->caps.flPointSize[0]                = 1;
    pState->caps.flPointSize[1]                = 1;

    /*
     * Query capabilities
     */
    VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx, glGetIntegerv(GL_MAX_LIGHTS, &pState->caps.maxActiveLights));
    VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx, glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &pState->caps.maxTextureBufferSize));
    VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx, glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &pState->caps.maxTextures));
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE /* The alternative profile has a higher number here (ati/darwin). */
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pOtherCtx);
    VMSVGA3D_INIT_CHECKED_BOTH(pState, pOtherCtx, pContext, glGetIntegerv(GL_MAX_CLIP_DISTANCES, &pState->caps.maxClipDistances));
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
#else
    VMSVGA3D_INIT_CHECKED(glGetIntegerv(GL_MAX_CLIP_DISTANCES, &pState->caps.maxClipDistances));
#endif
    VMSVGA3D_INIT_CHECKED(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &pState->caps.maxColorAttachments));
    VMSVGA3D_INIT_CHECKED(glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &pState->caps.maxRectangleTextureSize));
    VMSVGA3D_INIT_CHECKED(glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &pState->caps.maxTextureAnisotropy));
    VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx, glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, pState->caps.flPointSize));

    if (pState->ext.glGetProgramivARB)
    {
        VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx,
                                   pState->ext.glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB,
                                                                 &pState->caps.maxFragmentShaderTemps));
        VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx,
                                   pState->ext.glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,
                                                                 &pState->caps.maxFragmentShaderInstructions));
        VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx,
                                   pState->ext.glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB,
                                                                 &pState->caps.maxVertexShaderTemps));
        VMSVGA3D_INIT_CHECKED_BOTH(pState, pContext, pOtherCtx,
                                   pState->ext.glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,
                                                                 &pState->caps.maxVertexShaderInstructions));
    }
    pState->caps.fS3TCSupported = vmsvga3dCheckGLExtension(pState, 0.0f, " GL_EXT_texture_compression_s3tc ");

    /* http://http://www.opengl.org/wiki/Detecting_the_Shader_Model
     * ARB Assembly Language
     * These are done through testing the presence of extensions. You should test them in this order:
     * GL_NV_gpu_program4: SM 4.0 or better.
     * GL_NV_vertex_program3: SM 3.0 or better.
     * GL_ARB_fragment_program: SM 2.0 or better.
     * ATI does not support higher than SM 2.0 functionality in assembly shaders.
     *
     */
    /** @todo distinguish between vertex and pixel shaders??? */
    const char *pszShadingLanguageVersion = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    float v = pszShadingLanguageVersion ? atof(pszShadingLanguageVersion) : 0.0f;
    if (v >= 3.30f)
    {
        pState->caps.vertexShaderVersion   = SVGA3DVSVERSION_40;
        pState->caps.fragmentShaderVersion = SVGA3DPSVERSION_40;
    }
    else
    if (v >= 1.20f)
    {
        pState->caps.vertexShaderVersion   = SVGA3DVSVERSION_20;
        pState->caps.fragmentShaderVersion = SVGA3DPSVERSION_20;
    }
    else
    if (   vmsvga3dCheckGLExtension(pState, 0.0f, " GL_NV_gpu_program4 ")
        || strstr(pState->pszOtherExtensions, " GL_NV_gpu_program4 "))
    {
        pState->caps.vertexShaderVersion   = SVGA3DVSVERSION_40;
        pState->caps.fragmentShaderVersion = SVGA3DPSVERSION_40;
    }
    else
    if (   vmsvga3dCheckGLExtension(pState, 0.0f, " GL_NV_vertex_program3 ")
        || strstr(pState->pszOtherExtensions, " GL_NV_vertex_program3 ")
        || vmsvga3dCheckGLExtension(pState, 0.0f, " GL_ARB_shader_texture_lod ")  /* Wine claims this suggests SM 3.0 support */
        || strstr(pState->pszOtherExtensions, " GL_ARB_shader_texture_lod ")
        )
    {
        pState->caps.vertexShaderVersion   = SVGA3DVSVERSION_30;
        pState->caps.fragmentShaderVersion = SVGA3DPSVERSION_30;
    }
    else
    if (   vmsvga3dCheckGLExtension(pState, 0.0f, " GL_ARB_fragment_program ")
        || strstr(pState->pszOtherExtensions, " GL_ARB_fragment_program "))
    {
        pState->caps.vertexShaderVersion   = SVGA3DVSVERSION_20;
        pState->caps.fragmentShaderVersion = SVGA3DPSVERSION_20;
    }
    else
    {
        LogRel(("VMSVGA3D: WARNING: unknown support for assembly shaders!!\n"));
        pState->caps.vertexShaderVersion   = SVGA3DVSVERSION_11;
        pState->caps.fragmentShaderVersion = SVGA3DPSVERSION_11;
    }

    if (!vmsvga3dCheckGLExtension(pState, 3.2f, " GL_ARB_vertex_array_bgra "))
    {
        /** @todo Intel drivers don't support this extension! */
        LogRel(("VMSVGA3D: WARNING: Missing required extension GL_ARB_vertex_array_bgra (d3dcolor)!!!\n"));
    }
#if 0
   SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND             = 11,
   SVGA3D_DEVCAP_QUERY_TYPES                       = 15,
   SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING         = 16,
   SVGA3D_DEVCAP_MAX_POINT_SIZE                    = 17,
   SVGA3D_DEVCAP_MAX_SHADER_TEXTURES               = 18,
   SVGA3D_DEVCAP_MAX_VOLUME_EXTENT                 = 21,
   SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT                = 22,
   SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO          = 23,
   SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY            = 24,
   SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT               = 25,
   SVGA3D_DEVCAP_MAX_VERTEX_INDEX                  = 26,
   SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS  = 28,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS           = 29,
   SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS         = 30,
   SVGA3D_DEVCAP_TEXTURE_OPS                       = 31,
   SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8               = 32,
   SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8               = 33,
   SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10            = 34,
   SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5               = 35,
   SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5               = 36,
   SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4               = 37,
   SVGA3D_DEVCAP_SURFACEFMT_R5G6B5                 = 38,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16            = 39,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8      = 40,
   SVGA3D_DEVCAP_SURFACEFMT_ALPHA8                 = 41,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8             = 42,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D16                  = 43,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8                = 44,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8                = 45,
   SVGA3D_DEVCAP_SURFACEFMT_DXT1                   = 46,
   SVGA3D_DEVCAP_SURFACEFMT_DXT2                   = 47,
   SVGA3D_DEVCAP_SURFACEFMT_DXT3                   = 48,
   SVGA3D_DEVCAP_SURFACEFMT_DXT4                   = 49,
   SVGA3D_DEVCAP_SURFACEFMT_DXT5                   = 50,
   SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8           = 51,
   SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10            = 52,
   SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8               = 53,
   SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8               = 54,
   SVGA3D_DEVCAP_SURFACEFMT_CxV8U8                 = 55,
   SVGA3D_DEVCAP_SURFACEFMT_R_S10E5                = 56,
   SVGA3D_DEVCAP_SURFACEFMT_R_S23E8                = 57,
   SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5               = 58,
   SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8               = 59,
   SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5             = 60,
   SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8             = 61,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES        = 63,
   SVGA3D_DEVCAP_SURFACEFMT_V16U16                 = 65,
   SVGA3D_DEVCAP_SURFACEFMT_G16R16                 = 66,
   SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16           = 67,
   SVGA3D_DEVCAP_SURFACEFMT_UYVY                   = 68,
   SVGA3D_DEVCAP_SURFACEFMT_YUY2                   = 69,
   SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES    = 70,
   SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES       = 71,
   SVGA3D_DEVCAP_ALPHATOCOVERAGE                   = 72,
   SVGA3D_DEVCAP_SUPERSAMPLE                       = 73,
   SVGA3D_DEVCAP_AUTOGENMIPMAPS                    = 74,
   SVGA3D_DEVCAP_SURFACEFMT_NV12                   = 75,
   SVGA3D_DEVCAP_SURFACEFMT_AYUV                   = 76,
   SVGA3D_DEVCAP_SURFACEFMT_Z_DF16                 = 79,
   SVGA3D_DEVCAP_SURFACEFMT_Z_DF24                 = 80,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT            = 81,
   SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM              = 82,
   SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM              = 83,
#endif

    LogRel(("VMSVGA3d: Capabilities:\n"));
    LogRel(("VMSVGA3d:   maxActiveLights=%-2d       maxTextures=%-2d           maxTextureBufferSize=%d\n",
            pState->caps.maxActiveLights, pState->caps.maxTextures, pState->caps.maxTextureBufferSize));
    LogRel(("VMSVGA3d:   maxClipDistances=%-2d      maxColorAttachments=%-2d   maxClipDistances=%d\n",
            pState->caps.maxClipDistances, pState->caps.maxColorAttachments, pState->caps.maxClipDistances));
    LogRel(("VMSVGA3d:   maxColorAttachments=%-2d   maxTextureAnisotropy=%-2d  maxRectangleTextureSize=%d\n",
            pState->caps.maxColorAttachments, pState->caps.maxTextureAnisotropy, pState->caps.maxRectangleTextureSize));
    LogRel(("VMSVGA3d:   maxVertexShaderTemps=%-2d  maxVertexShaderInstructions=%d maxFragmentShaderInstructions=%d\n",
            pState->caps.maxVertexShaderTemps, pState->caps.maxVertexShaderInstructions, pState->caps.maxFragmentShaderInstructions));
    LogRel(("VMSVGA3d:   maxFragmentShaderTemps=%d flPointSize={%d.%02u, %d.%02u}\n",
            pState->caps.maxFragmentShaderTemps,
            (int)pState->caps.flPointSize[0], (int)(pState->caps.flPointSize[0] * 100) % 100,
            (int)pState->caps.flPointSize[1], (int)(pState->caps.flPointSize[1] * 100) % 100));
    LogRel(("VMSVGA3d:   fragmentShaderVersion=%-2d vertexShaderVersion=%-2d   fS3TCSupported=%d\n",
            pState->caps.fragmentShaderVersion, pState->caps.vertexShaderVersion, pState->caps.fS3TCSupported));


    /* Initialize the shader library. */
    pState->ShaderIf.pfnSwitchInitProfile = vmsvga3dShaderIfSwitchInitProfile;
    pState->ShaderIf.pfnGetNextExtension  = vmsvga3dShaderIfGetNextExtension;
    rc = ShaderInitLib(&pState->ShaderIf);
    AssertRC(rc);

    /* Cleanup */
    rc = vmsvga3dContextDestroy(pThis, 1);
    AssertRC(rc);
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    rc = vmsvga3dContextDestroy(pThis, 2);
    AssertRC(rc);
#endif

    if (   pState->rsGLVersion < 3.0
        && pState->rsOtherGLVersion < 3.0 /* darwin: legacy profile hack */)
    {
        LogRel(("VMSVGA3d: unsupported OpenGL version; minimum is 3.0\n"));
        return VERR_NOT_IMPLEMENTED;
    }
    if (    !pState->ext.glIsRenderbuffer
        ||  !pState->ext.glBindRenderbuffer
        ||  !pState->ext.glDeleteRenderbuffers
        ||  !pState->ext.glGenRenderbuffers
        ||  !pState->ext.glRenderbufferStorage
        ||  !pState->ext.glGetRenderbufferParameteriv
        ||  !pState->ext.glIsFramebuffer
        ||  !pState->ext.glBindFramebuffer
        ||  !pState->ext.glDeleteFramebuffers
        ||  !pState->ext.glGenFramebuffers
        ||  !pState->ext.glCheckFramebufferStatus
        ||  !pState->ext.glFramebufferTexture1D
        ||  !pState->ext.glFramebufferTexture2D
        ||  !pState->ext.glFramebufferTexture3D
        ||  !pState->ext.glFramebufferRenderbuffer
        ||  !pState->ext.glGetFramebufferAttachmentParameteriv
        ||  !pState->ext.glGenerateMipmap
        ||  !pState->ext.glBlitFramebuffer
        ||  !pState->ext.glRenderbufferStorageMultisample
        ||  !pState->ext.glFramebufferTextureLayer)
    {
        LogRel(("VMSVGA3d: missing required OpenGL extension; aborting\n"));
        return VERR_NOT_IMPLEMENTED;
    }

#ifdef DEBUG_DEBUG_GFX_WINDOW_TEST_CONTEXT
    pState->idTestContext = SVGA_ID_INVALID;
#endif
    return VINF_SUCCESS;
}

int vmsvga3dReset(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);

    /* Destroy all leftover surfaces. */
    for (uint32_t i = 0; i < pState->cSurfaces; i++)
    {
        if (pState->papSurfaces[i]->id != SVGA3D_INVALID_ID)
            vmsvga3dSurfaceDestroy(pThis, pState->papSurfaces[i]->id);
    }

    /* Destroy all leftover contexts. */
    for (uint32_t i = 0; i < pState->cContexts; i++)
    {
        if (pState->papContexts[i]->id != SVGA3D_INVALID_ID)
            vmsvga3dContextDestroy(pThis, pState->papContexts[i]->id);
    }

    if (pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
        vmsvga3dContextDestroyOgl(pThis, &pState->SharedCtx, VMSVGA3D_SHARED_CTX_ID);

    return VINF_SUCCESS;
}

int vmsvga3dTerminate(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_WRONG_ORDER);
    int            rc;

    rc = vmsvga3dReset(pThis);
    AssertRCReturn(rc, rc);

    /* Terminate the shader library. */
    rc = ShaderDestroyLib();
    AssertRC(rc);

#ifdef RT_OS_WINDOWS
    /* Terminate the window creation thread. */
    rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_EXIT, 0, 0);
    AssertRCReturn(rc, rc);

    RTSemEventDestroy(pState->WndRequestSem);
#elif defined(RT_OS_DARWIN)

#elif defined(RT_OS_LINUX)
    /* signal to the thread that it is supposed to exit */
    pState->bTerminate = true;
    /* wait for it to terminate */
    rc = RTThreadWait(pState->pWindowThread, 10000, NULL);
    AssertRC(rc);
    XCloseDisplay(pState->display);
#endif

    RTStrFree(pState->pszExtensions);
    pState->pszExtensions = NULL;
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    RTStrFree(pState->pszOtherExtensions);
#endif
    pState->pszOtherExtensions = NULL;

    return VINF_SUCCESS;
}


void vmsvga3dUpdateHostScreenViewport(PVGASTATE pThis, uint32_t idScreen, VMSVGAVIEWPORT const *pOldViewport)
{
    /** @todo Move the visible framebuffer content here, don't wait for the guest to
     *        redraw it. */

#ifdef RT_OS_DARWIN
    RT_NOREF(pOldViewport);
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    if (   pState
        && idScreen == 0
        && pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
    {
        vmsvga3dCocoaViewUpdateViewport(pState->SharedCtx.cocoaView);
    }
#else
    RT_NOREF(pThis, idScreen, pOldViewport);
#endif
}


/**
 * Worker for vmsvga3dQueryCaps that figures out supported operations for a
 * given surface format capability.
 *
 * @returns Supported/indented operations (SVGA3DFORMAT_OP_XXX).
 * @param   idx3dCaps       The SVGA3D_CAPS_XXX value of the surface format.
 *
 * @remarks See fromat_cap_table in svga_format.c (mesa/gallium) for a reference
 *          of implicit guest expectations:
 *              http://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/svga/svga_format.c
 */
static uint32_t vmsvga3dGetSurfaceFormatSupport(uint32_t idx3dCaps)
{
    uint32_t result = 0;

    /** @todo missing:
     *
     * SVGA3DFORMAT_OP_PIXELSIZE
     */

    switch (idx3dCaps)
    {
    case SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
        result |= SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB
               |  SVGA3DFORMAT_OP_CONVERT_TO_ARGB
               |  SVGA3DFORMAT_OP_DISPLAYMODE           /* Should not be set for alpha formats. */
               |  SVGA3DFORMAT_OP_3DACCELERATION;       /* implies OP_DISPLAYMODE */
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
    case SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
        result |=     SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB
                  |   SVGA3DFORMAT_OP_CONVERT_TO_ARGB
                  |   SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
        break;
    }

    /** @todo check hardware caps! */
    switch (idx3dCaps)
    {
    case SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
    case SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
    case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16:
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8:
    case SVGA3D_DEVCAP_SURFACEFMT_ALPHA8:
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8:
        result |= SVGA3DFORMAT_OP_TEXTURE
               |  SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
               |  SVGA3DFORMAT_OP_OFFSCREENPLAIN
               |  SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET
               |  SVGA3DFORMAT_OP_VOLUMETEXTURE
               |  SVGA3DFORMAT_OP_CUBETEXTURE
               |  SVGA3DFORMAT_OP_SRGBREAD
               |  SVGA3DFORMAT_OP_SRGBWRITE;
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Z_D16:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF16:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF24:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT:
        result |= SVGA3DFORMAT_OP_ZSTENCIL
               |  SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH
               |  SVGA3DFORMAT_OP_TEXTURE /* Necessary for Ubuntu Unity */;
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT1:
    case SVGA3D_DEVCAP_SURFACEFMT_DXT3:
    case SVGA3D_DEVCAP_SURFACEFMT_DXT5:
        result |= SVGA3DFORMAT_OP_TEXTURE
               |  SVGA3DFORMAT_OP_VOLUMETEXTURE
               |  SVGA3DFORMAT_OP_CUBETEXTURE
               |  SVGA3DFORMAT_OP_SRGBREAD;
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8:
    case SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10:
    case SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8:
    case SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8:
    case SVGA3D_DEVCAP_SURFACEFMT_CxV8U8:
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_R_S10E5:
    case SVGA3D_DEVCAP_SURFACEFMT_R_S23E8:
    case SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5:
    case SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8:
    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5:
    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8:
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_V16U16:
    case SVGA3D_DEVCAP_SURFACEFMT_G16R16:
    case SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16:

    case SVGA3D_DEVCAP_SURFACEFMT_UYVY:
    case SVGA3D_DEVCAP_SURFACEFMT_YUY2:
    case SVGA3D_DEVCAP_SURFACEFMT_NV12:
    case SVGA3D_DEVCAP_SURFACEFMT_AYUV:
        break;
    }
    Log(("CAPS: %s =\n%s\n", vmsvga3dGetCapString(idx3dCaps), vmsvga3dGet3dFormatString(result)));

    return result;
}

#if 0 /* unused */
static uint32_t vmsvga3dGetDepthFormatSupport(PVMSVGA3DSTATE pState3D, uint32_t idx3dCaps)
{
    RT_NOREF(pState3D, idx3dCaps);

    /** @todo test this somehow */
    uint32_t result = SVGA3DFORMAT_OP_ZSTENCIL | SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH;

    Log(("CAPS: %s =\n%s\n", vmsvga3dGetCapString(idx3dCaps), vmsvga3dGet3dFormatString(result)));
    return result;
}
#endif


int vmsvga3dQueryCaps(PVGASTATE pThis, uint32_t idx3dCaps, uint32_t *pu32Val)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    int       rc = VINF_SUCCESS;

    *pu32Val = 0;

    /*
     * The capabilities access by current (2015-03-01) linux sources (gallium,
     * vmwgfx, xorg-video-vmware) are annotated, caps without xref annotations
     * aren't access.
     */

    switch (idx3dCaps)
    {
    /* Linux: vmwgfx_fifo.c in kmod; only used with SVGA_CAP_GBOBJECTS. */
    case SVGA3D_DEVCAP_3D:
        *pu32Val = 1; /* boolean? */
        break;

    case SVGA3D_DEVCAP_MAX_LIGHTS:
        *pu32Val = pState->caps.maxActiveLights;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURES:
        *pu32Val = pState->caps.maxTextures;
        break;

    case SVGA3D_DEVCAP_MAX_CLIP_PLANES:
        *pu32Val = pState->caps.maxClipDistances;
        break;

    /* Linux: svga_screen.c in gallium; 3.0 or later required. */
    case SVGA3D_DEVCAP_VERTEX_SHADER_VERSION:
        *pu32Val = pState->caps.vertexShaderVersion;
        break;

    case SVGA3D_DEVCAP_VERTEX_SHADER:
        /* boolean? */
        *pu32Val = (pState->caps.vertexShaderVersion != SVGA3DVSVERSION_NONE);
        break;

    /* Linux: svga_screen.c in gallium; 3.0 or later required. */
    case SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION:
        *pu32Val = pState->caps.fragmentShaderVersion;
        break;

    case SVGA3D_DEVCAP_FRAGMENT_SHADER:
        /* boolean? */
        *pu32Val = (pState->caps.fragmentShaderVersion != SVGA3DPSVERSION_NONE);
        break;

    case SVGA3D_DEVCAP_S23E8_TEXTURES:
    case SVGA3D_DEVCAP_S10E5_TEXTURES:
        /* Must be obsolete by now; surface format caps specify the same thing. */
        rc = VERR_INVALID_PARAMETER;
        break;

    case SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND:
        break;

    /*
     *   2. The BUFFER_FORMAT capabilities are deprecated, and they always
     *      return TRUE. Even on physical hardware that does not support
     *      these formats natively, the SVGA3D device will provide an emulation
     *      which should be invisible to the guest OS.
     */
    case SVGA3D_DEVCAP_D16_BUFFER_FORMAT:
    case SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT:
    case SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT:
        *pu32Val = 1;
        break;

    case SVGA3D_DEVCAP_QUERY_TYPES:
        break;

    case SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING:
        break;

    /* Linux: svga_screen.c in gallium; capped at 80.0, default 1.0. */
    case SVGA3D_DEVCAP_MAX_POINT_SIZE:
        AssertCompile(sizeof(uint32_t) == sizeof(float));
        *(float *)pu32Val = pState->caps.flPointSize[1];
        break;

    case SVGA3D_DEVCAP_MAX_SHADER_TEXTURES:
        /** @todo ?? */
        rc = VERR_INVALID_PARAMETER;
        break;

    /* Linux: svga_screen.c in gallium (for PIPE_CAP_MAX_TEXTURE_2D_LEVELS); have default if missing. */
    case SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH:
    case SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT:
        *pu32Val = pState->caps.maxRectangleTextureSize;
        break;

    /* Linux: svga_screen.c in gallium (for PIPE_CAP_MAX_TEXTURE_3D_LEVELS); have default if missing. */
    case SVGA3D_DEVCAP_MAX_VOLUME_EXTENT:
        //*pu32Val = pCaps->MaxVolumeExtent;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT:
        *pu32Val = 32768;   /* hardcoded in Wine */
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO:
        //*pu32Val = pCaps->MaxTextureAspectRatio;
        break;

    /* Linux: svga_screen.c in gallium (for PIPE_CAPF_MAX_TEXTURE_ANISOTROPY); defaults to 4.0. */
    case SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY:
        *pu32Val = pState->caps.maxTextureAnisotropy;
        break;

    case SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT:
    case SVGA3D_DEVCAP_MAX_VERTEX_INDEX:
        *pu32Val =  0xFFFFF; /* hardcoded in Wine */
        break;

    /* Linux: svga_screen.c in gallium (for PIPE_SHADER_VERTEX/PIPE_SHADER_CAP_MAX_INSTRUCTIONS); defaults to 512. */
    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS:
        *pu32Val = pState->caps.maxVertexShaderInstructions;
        break;

    case SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS:
        *pu32Val = pState->caps.maxFragmentShaderInstructions;
        break;

    /* Linux: svga_screen.c in gallium (for PIPE_SHADER_VERTEX/PIPE_SHADER_CAP_MAX_TEMPS); defaults to 32. */
    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS:
        *pu32Val = pState->caps.maxVertexShaderTemps;
        break;

    /* Linux: svga_screen.c in gallium (for PIPE_SHADER_FRAGMENT/PIPE_SHADER_CAP_MAX_TEMPS); defaults to 32. */
    case SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS:
        *pu32Val = pState->caps.maxFragmentShaderTemps;
        break;

    case SVGA3D_DEVCAP_TEXTURE_OPS:
        break;

    case SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES:
        break;

    case SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES:
        break;

    case SVGA3D_DEVCAP_ALPHATOCOVERAGE:
        break;

    case SVGA3D_DEVCAP_SUPERSAMPLE:
        break;

    case SVGA3D_DEVCAP_AUTOGENMIPMAPS:
        //*pu32Val = !!(pCaps->Caps2 & D3DCAPS2_CANAUTOGENMIPMAP);
        break;

    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES:
        break;

    case SVGA3D_DEVCAP_MAX_RENDER_TARGETS:  /** @todo same thing? */
    case SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS:
        *pu32Val = pState->caps.maxColorAttachments;
        break;

    /*
     * This is the maximum number of SVGA context IDs that the guest
     * can define using SVGA_3D_CMD_CONTEXT_DEFINE.
     */
    case SVGA3D_DEVCAP_MAX_CONTEXT_IDS:
        *pu32Val = SVGA3D_MAX_CONTEXT_IDS;
        break;

    /*
     * This is the maximum number of SVGA surface IDs that the guest
     * can define using SVGA_3D_CMD_SURFACE_DEFINE*.
     */
    case SVGA3D_DEVCAP_MAX_SURFACE_IDS:
        *pu32Val = SVGA3D_MAX_SURFACE_IDS;
        break;

#if 0 /* Appeared more recently, not yet implemented. */
   /* Linux: svga_screen.c in gallium; defaults to FALSE. */
   case SVGA3D_DEVCAP_LINE_AA:
       break;
   /* Linux: svga_screen.c in gallium; defaults to FALSE. */
   case SVGA3D_DEVCAP_LINE_STIPPLE:
       break;
   /* Linux: svga_screen.c in gallium; defaults to 1.0. */
   case SVGA3D_DEVCAP_MAX_LINE_WIDTH:
       break;
   /* Linux: svga_screen.c in gallium; defaults to 1.0. */
   case SVGA3D_DEVCAP_MAX_AA_LINE_WIDTH:
       break;
#endif

    /*
     * Supported surface formats.
     * Linux: svga_format.c in gallium, format_cap_table defines implicit expectations.
     */
    case SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
    case SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
    case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16:
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8:
    case SVGA3D_DEVCAP_SURFACEFMT_ALPHA8:
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D16:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF16:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF24:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT:
    case SVGA3D_DEVCAP_SURFACEFMT_DXT1:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(idx3dCaps);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT2:
    case SVGA3D_DEVCAP_SURFACEFMT_DXT4:
        *pu32Val = 0;   /* apparently not supported in OpenGL */
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT3:
    case SVGA3D_DEVCAP_SURFACEFMT_DXT5:
    case SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8:
    case SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10:
    case SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8:
    case SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8:
    case SVGA3D_DEVCAP_SURFACEFMT_CxV8U8:
    case SVGA3D_DEVCAP_SURFACEFMT_R_S10E5:
    case SVGA3D_DEVCAP_SURFACEFMT_R_S23E8:
    case SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5:
    case SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8:
    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5:
    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8:
    case SVGA3D_DEVCAP_SURFACEFMT_V16U16:
    case SVGA3D_DEVCAP_SURFACEFMT_G16R16:
    case SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16:
    case SVGA3D_DEVCAP_SURFACEFMT_UYVY:
    case SVGA3D_DEVCAP_SURFACEFMT_YUY2:
    case SVGA3D_DEVCAP_SURFACEFMT_NV12:
    case SVGA3D_DEVCAP_SURFACEFMT_AYUV:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(idx3dCaps);
        break;

    /* Linux: Not referenced in current sources. */
    case SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM:
    case SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM:
        Log(("CAPS: Unknown CAP %s\n", vmsvga3dGetCapString(idx3dCaps)));
        rc = VERR_INVALID_PARAMETER;
        *pu32Val = 0;
        break;

    default:
        Log(("CAPS: Unexpected CAP %d\n", idx3dCaps));
        rc = VERR_INVALID_PARAMETER;
        break;
    }

    Log(("CAPS: %s - %x\n", vmsvga3dGetCapString(idx3dCaps), *pu32Val));
    return rc;
}

/**
 * Convert SVGA format value to its OpenGL equivalent
 *
 * @remarks  Clues to be had in format_texture_info table (wined3d/utils.c) with
 *           help from wined3dformat_from_d3dformat().
 */
void vmsvga3dSurfaceFormat2OGL(PVMSVGA3DSURFACE pSurface, SVGA3dSurfaceFormat format)
{
    switch (format)
    {
    case SVGA3D_X8R8G8B8:               /* D3DFMT_X8R8G8B8 - WINED3DFMT_B8G8R8X8_UNORM */
        pSurface->internalFormatGL = GL_RGB8;
        pSurface->formatGL = GL_BGRA;
        pSurface->typeGL = GL_UNSIGNED_INT_8_8_8_8_REV;
        break;
    case SVGA3D_A8R8G8B8:               /* D3DFMT_A8R8G8B8 - WINED3DFMT_B8G8R8A8_UNORM */
        pSurface->internalFormatGL = GL_RGBA8;
        pSurface->formatGL = GL_BGRA;
        pSurface->typeGL = GL_UNSIGNED_INT_8_8_8_8_REV;
        break;
    case SVGA3D_R5G6B5:                 /* D3DFMT_R5G6B5 - WINED3DFMT_B5G6R5_UNORM */
        pSurface->internalFormatGL = GL_RGB5;
        pSurface->formatGL = GL_RGB;
        pSurface->typeGL = GL_UNSIGNED_SHORT_5_6_5;
        AssertMsgFailed(("Test me - SVGA3D_R5G6B5\n"));
        break;
    case SVGA3D_X1R5G5B5:               /* D3DFMT_X1R5G5B5 - WINED3DFMT_B5G5R5X1_UNORM */
        pSurface->internalFormatGL = GL_RGB5;
        pSurface->formatGL = GL_BGRA;
        pSurface->typeGL = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        AssertMsgFailed(("Test me - SVGA3D_X1R5G5B5\n"));
        break;
    case SVGA3D_A1R5G5B5:               /* D3DFMT_A1R5G5B5 - WINED3DFMT_B5G5R5A1_UNORM */
        pSurface->internalFormatGL = GL_RGB5_A1;
        pSurface->formatGL = GL_BGRA;
        pSurface->typeGL = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        AssertMsgFailed(("Test me - SVGA3D_A1R5G5B5\n"));
        break;
    case SVGA3D_A4R4G4B4:               /* D3DFMT_A4R4G4B4 - WINED3DFMT_B4G4R4A4_UNORM */
        pSurface->internalFormatGL = GL_RGBA4;
        pSurface->formatGL = GL_BGRA;
        pSurface->typeGL = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        AssertMsgFailed(("Test me - SVGA3D_A4R4G4B4\n"));
        break;

    case SVGA3D_Z_D32:                  /* D3DFMT_D32 - WINED3DFMT_D32_UNORM */
        pSurface->internalFormatGL = GL_DEPTH_COMPONENT32;
        pSurface->formatGL = GL_DEPTH_COMPONENT;
        pSurface->typeGL = GL_UNSIGNED_INT;
        break;
    case SVGA3D_Z_D16:                  /* D3DFMT_D16 - WINED3DFMT_D16_UNORM */
        pSurface->internalFormatGL = GL_DEPTH_COMPONENT16; /** @todo Wine suggests GL_DEPTH_COMPONENT24. */
        pSurface->formatGL = GL_DEPTH_COMPONENT;
        pSurface->typeGL = GL_UNSIGNED_SHORT;
        //AssertMsgFailed(("Test me - SVGA3D_Z_D16\n"));
        break;
    case SVGA3D_Z_D24S8:                /* D3DFMT_D24S8 - WINED3DFMT_D24_UNORM_S8_UINT */
        pSurface->internalFormatGL = GL_DEPTH24_STENCIL8;
        pSurface->formatGL = GL_DEPTH_STENCIL;
        pSurface->typeGL = GL_UNSIGNED_INT;
        break;
    case SVGA3D_Z_D15S1:                /* D3DFMT_D15S1 - WINED3DFMT_S1_UINT_D15_UNORM */
        pSurface->internalFormatGL = GL_DEPTH_COMPONENT16;  /** @todo ??? */
        pSurface->formatGL = GL_DEPTH_STENCIL;
        pSurface->typeGL = GL_UNSIGNED_SHORT;
        /** @todo Wine sources hints at no hw support for this, so test this one! */
        AssertMsgFailed(("Test me - SVGA3D_Z_D15S1\n"));
        break;
    case SVGA3D_Z_D24X8:                /* D3DFMT_D24X8 - WINED3DFMT_X8D24_UNORM */
        pSurface->internalFormatGL = GL_DEPTH_COMPONENT24;
        pSurface->formatGL = GL_DEPTH_COMPONENT;
        pSurface->typeGL = GL_UNSIGNED_INT;
        break;

    /* Advanced D3D9 depth formats. */
    case SVGA3D_Z_DF16:                 /* D3DFMT_DF16? - not supported */
        pSurface->internalFormatGL = GL_DEPTH_COMPONENT16;
        pSurface->formatGL = GL_DEPTH_COMPONENT;
        pSurface->typeGL = GL_FLOAT;
        break;

    case SVGA3D_Z_DF24:                 /* D3DFMT_DF24? - not supported */
        pSurface->internalFormatGL = GL_DEPTH_COMPONENT24;
        pSurface->formatGL = GL_DEPTH_COMPONENT;
        pSurface->typeGL = GL_FLOAT;        /* ??? */
        break;

    case SVGA3D_Z_D24S8_INT:            /* D3DFMT_??? - not supported */
        pSurface->internalFormatGL = GL_DEPTH24_STENCIL8;
        pSurface->formatGL = GL_DEPTH_STENCIL;
        pSurface->typeGL = GL_INT;        /* ??? */
        break;

    case SVGA3D_DXT1:                   /* D3DFMT_DXT1 - WINED3DFMT_DXT1 */
        pSurface->internalFormatGL = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
#if 0
        pSurface->formatGL = GL_RGBA_S3TC;          /* ??? */
        pSurface->typeGL = GL_UNSIGNED_INT;         /* ??? */
#else   /* wine suggests: */
        pSurface->formatGL = GL_RGBA;
        pSurface->typeGL = GL_UNSIGNED_BYTE;
        AssertMsgFailed(("Test me - SVGA3D_DXT1\n"));
#endif
        break;

    case SVGA3D_DXT3:                   /* D3DFMT_DXT3 - WINED3DFMT_DXT3 */
        pSurface->internalFormatGL = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
#if 0 /** @todo this needs testing... */
        pSurface->formatGL = GL_RGBA_S3TC;          /* ??? */
        pSurface->typeGL = GL_UNSIGNED_INT;         /* ??? */
#else   /* wine suggests: */
        pSurface->formatGL = GL_RGBA;
        pSurface->typeGL = GL_UNSIGNED_BYTE;
        AssertMsgFailed(("Test me - SVGA3D_DXT3\n"));
#endif
        break;

    case SVGA3D_DXT5:                   /* D3DFMT_DXT5 - WINED3DFMT_DXT5 */
        pSurface->internalFormatGL = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#if 0 /** @todo this needs testing... */
        pSurface->formatGL = GL_RGBA_S3TC;
        pSurface->typeGL = GL_UNSIGNED_INT;
#else   /* wine suggests: */
        pSurface->formatGL = GL_RGBA;
        pSurface->typeGL = GL_UNSIGNED_BYTE;
        AssertMsgFailed(("Test me - SVGA3D_DXT5\n"));
#endif
        break;

    case SVGA3D_LUMINANCE8:             /* D3DFMT_? - ? */
        pSurface->internalFormatGL = GL_LUMINANCE8_EXT;
        pSurface->formatGL = GL_LUMINANCE;
        pSurface->typeGL = GL_UNSIGNED_BYTE;
        break;

    case SVGA3D_LUMINANCE16:            /* D3DFMT_? - ? */
        pSurface->internalFormatGL = GL_LUMINANCE16_EXT;
        pSurface->formatGL = GL_LUMINANCE;
        pSurface->typeGL = GL_UNSIGNED_SHORT;
        break;

    case SVGA3D_LUMINANCE4_ALPHA4:     /* D3DFMT_? - ? */
        pSurface->internalFormatGL = GL_LUMINANCE4_ALPHA4_EXT;
        pSurface->formatGL = GL_LUMINANCE_ALPHA;
        pSurface->typeGL = GL_UNSIGNED_BYTE;
        break;

    case SVGA3D_LUMINANCE8_ALPHA8:     /* D3DFMT_? - ? */
        pSurface->internalFormatGL = GL_LUMINANCE8_ALPHA8_EXT;
        pSurface->formatGL = GL_LUMINANCE_ALPHA;
        pSurface->typeGL = GL_UNSIGNED_BYTE;    /* unsigned_short causes issues even though this type should be 16-bit */
        break;

    case SVGA3D_ALPHA8:                /* D3DFMT_A8? - WINED3DFMT_A8_UNORM? */
        pSurface->internalFormatGL = GL_ALPHA8_EXT;
        pSurface->formatGL = GL_ALPHA;
        pSurface->typeGL = GL_UNSIGNED_BYTE;
        break;

#if 0

    /* Bump-map formats */
    case SVGA3D_BUMPU8V8:
        return D3DFMT_V8U8;
    case SVGA3D_BUMPL6V5U5:
        return D3DFMT_L6V5U5;
    case SVGA3D_BUMPX8L8V8U8:
        return D3DFMT_X8L8V8U8;
    case SVGA3D_BUMPL8V8U8:
        /* No corresponding D3D9 equivalent. */
        AssertFailedReturn(D3DFMT_UNKNOWN);
    /* signed bump-map formats */
    case SVGA3D_V8U8:
        return D3DFMT_V8U8;
    case SVGA3D_Q8W8V8U8:
        return D3DFMT_Q8W8V8U8;
    case SVGA3D_CxV8U8:
        return D3DFMT_CxV8U8;
    /* mixed bump-map formats */
    case SVGA3D_X8L8V8U8:
        return D3DFMT_X8L8V8U8;
    case SVGA3D_A2W10V10U10:
        return D3DFMT_A2W10V10U10;
#endif

    case SVGA3D_ARGB_S10E5:   /* 16-bit floating-point ARGB */ /* D3DFMT_A16B16G16R16F - WINED3DFMT_R16G16B16A16_FLOAT */
        pSurface->internalFormatGL = GL_RGBA16F;
        pSurface->formatGL = GL_RGBA;
#if 0 /* bird: wine uses half float, sounds correct to me... */
        pSurface->typeGL = GL_FLOAT;
#else
        pSurface->typeGL = GL_HALF_FLOAT;
        AssertMsgFailed(("Test me - SVGA3D_ARGB_S10E5\n"));
#endif
        break;

    case SVGA3D_ARGB_S23E8:   /* 32-bit floating-point ARGB */ /* D3DFMT_A32B32G32R32F - WINED3DFMT_R32G32B32A32_FLOAT */
        pSurface->internalFormatGL = GL_RGBA32F;
        pSurface->formatGL = GL_RGBA;
        pSurface->typeGL = GL_FLOAT;    /* ?? - same as wine, so probably correct */
        break;

    case SVGA3D_A2R10G10B10:            /* D3DFMT_A2R10G10B10 - WINED3DFMT_B10G10R10A2_UNORM */
        pSurface->internalFormatGL = GL_RGB10_A2; /* ?? - same as wine, so probably correct */
#if 0 /* bird: Wine uses GL_BGRA instead of GL_RGBA. */
        pSurface->formatGL = GL_RGBA;
#else
        pSurface->formatGL = GL_BGRA;
#endif
        pSurface->typeGL = GL_UNSIGNED_INT;
        AssertMsgFailed(("Test me - SVGA3D_A2R10G10B10\n"));
        break;


    /* Single- and dual-component floating point formats */
    case SVGA3D_R_S10E5:                /* D3DFMT_R16F - WINED3DFMT_R16_FLOAT */
        pSurface->internalFormatGL = GL_R16F;
        pSurface->formatGL = GL_RED;
#if 0 /* bird: wine uses half float, sounds correct to me... */
        pSurface->typeGL = GL_FLOAT;
#else
        pSurface->typeGL = GL_HALF_FLOAT;
        AssertMsgFailed(("Test me - SVGA3D_R_S10E5\n"));
#endif
        break;
    case SVGA3D_R_S23E8:                /* D3DFMT_R32F - WINED3DFMT_R32_FLOAT */
        pSurface->internalFormatGL = GL_R32F;
        pSurface->formatGL = GL_RG;
        pSurface->typeGL = GL_FLOAT;
        break;
    case SVGA3D_RG_S10E5:               /* D3DFMT_G16R16F - WINED3DFMT_R16G16_FLOAT */
        pSurface->internalFormatGL = GL_RG16F;
        pSurface->formatGL = GL_RG;
#if 0 /* bird: wine uses half float, sounds correct to me... */
        pSurface->typeGL = GL_FLOAT;
#else
        pSurface->typeGL = GL_HALF_FLOAT;
        AssertMsgFailed(("Test me - SVGA3D_RG_S10E5\n"));
#endif
        break;
    case SVGA3D_RG_S23E8:               /* D3DFMT_G32R32F - WINED3DFMT_R32G32_FLOAT */
        pSurface->internalFormatGL = GL_RG32F;
        pSurface->formatGL = GL_RG;
        pSurface->typeGL = GL_FLOAT;
        break;

    /*
     * Any surface can be used as a buffer object, but SVGA3D_BUFFER is
     * the most efficient format to use when creating new surfaces
     * expressly for index or vertex data.
     */
    case SVGA3D_BUFFER:
        pSurface->internalFormatGL = -1;
        pSurface->formatGL = -1;
        pSurface->typeGL = -1;
        break;

#if 0
        return D3DFMT_UNKNOWN;

    case SVGA3D_V16U16:
        return D3DFMT_V16U16;
#endif

    case SVGA3D_G16R16:                 /* D3DFMT_G16R16 - WINED3DFMT_R16G16_UNORM */
        pSurface->internalFormatGL = GL_RG16;
        pSurface->formatGL = GL_RG;
#if 0 /* bird: Wine uses GL_UNSIGNED_SHORT here. */
        pSurface->typeGL = GL_UNSIGNED_INT;
#else
        pSurface->typeGL = GL_UNSIGNED_SHORT;
        AssertMsgFailed(("test me - SVGA3D_G16R16\n"));
#endif
        break;

    case SVGA3D_A16B16G16R16:           /* D3DFMT_A16B16G16R16 - WINED3DFMT_R16G16B16A16_UNORM */
        pSurface->internalFormatGL = GL_RGBA16;
        pSurface->formatGL = GL_RGBA;
#if 0 /* bird: Wine uses GL_UNSIGNED_SHORT here. */
        pSurface->typeGL = GL_UNSIGNED_INT;     /* ??? */
#else
        pSurface->typeGL = GL_UNSIGNED_SHORT;
        AssertMsgFailed(("Test me - SVGA3D_A16B16G16R16\n"));
#endif
        break;

#if 0
    /* Packed Video formats */
    case SVGA3D_UYVY:
        return D3DFMT_UYVY;
    case SVGA3D_YUY2:
        return D3DFMT_YUY2;

    /* Planar video formats */
    case SVGA3D_NV12:
        return (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');

    /* Video format with alpha */
    case SVGA3D_AYUV:
        return (D3DFORMAT)MAKEFOURCC('A', 'Y', 'U', 'V');

    case SVGA3D_BC4_UNORM:
    case SVGA3D_BC5_UNORM:
        /* Unknown; only in DX10 & 11 */
        break;
#endif
    default:
        AssertMsgFailed(("Unsupported format %d\n", format));
        break;
    }
}


#if 0
/**
 * Convert SVGA multi sample count value to its D3D equivalent
 */
D3DMULTISAMPLE_TYPE vmsvga3dMultipeSampleCount2D3D(uint32_t multisampleCount)
{
    AssertCompile(D3DMULTISAMPLE_2_SAMPLES == 2);
    AssertCompile(D3DMULTISAMPLE_16_SAMPLES == 16);

    if (multisampleCount > 16)
        return D3DMULTISAMPLE_NONE;

    /** @todo exact same mapping as d3d? */
    return (D3DMULTISAMPLE_TYPE)multisampleCount;
}
#endif

/**
 * Destroy backend specific surface bits (part of SVGA_3D_CMD_SURFACE_DESTROY).
 *
 * @param   pState              The VMSVGA3d state.
 * @param   pSurface            The surface being destroyed.
 */
void vmsvga3dBackSurfaceDestroy(PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface)
{
    PVMSVGA3DCONTEXT pContext = &pState->SharedCtx;
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    switch (pSurface->flags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK)
    {
    case SVGA3D_SURFACE_CUBEMAP:
        AssertFailed(); /** @todo destroy SVGA3D_SURFACE_CUBEMAP */
        break;

    case SVGA3D_SURFACE_HINT_INDEXBUFFER | SVGA3D_SURFACE_HINT_VERTEXBUFFER:
    case SVGA3D_SURFACE_HINT_INDEXBUFFER:
    case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
        if (pSurface->oglId.buffer != OPENGL_INVALID_ID)
        {
            pState->ext.glDeleteBuffers(1, &pSurface->oglId.buffer);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
        }
        break;

    case SVGA3D_SURFACE_HINT_TEXTURE:
    case SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
        if (pSurface->oglId.texture != OPENGL_INVALID_ID)
        {
            glDeleteTextures(1, &pSurface->oglId.texture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
        }
        break;

    case SVGA3D_SURFACE_HINT_RENDERTARGET:
    case SVGA3D_SURFACE_HINT_DEPTHSTENCIL:
    case SVGA3D_SURFACE_HINT_DEPTHSTENCIL | SVGA3D_SURFACE_HINT_TEXTURE:    /** @todo actual texture surface not supported */
        if (pSurface->oglId.renderbuffer != OPENGL_INVALID_ID)
        {
            pState->ext.glDeleteRenderbuffers(1, &pSurface->oglId.renderbuffer);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
        }
        break;

    default:
        AssertMsg(!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSurface), ("type=%x\n", (pSurface->flags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK)));
        break;
    }
}


int vmsvga3dSurfaceCopy(PVGASTATE pThis, SVGA3dSurfaceImageId dest, SVGA3dSurfaceImageId src, uint32_t cCopyBoxes, SVGA3dCopyBox *pBox)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    uint32_t            sidSrc = src.sid;
    uint32_t            sidDest = dest.sid;
    int                 rc = VINF_SUCCESS;

    AssertReturn(pState, VERR_NO_MEMORY);
    Assert(sidSrc < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sidSrc < pState->cSurfaces && pState->papSurfaces[sidSrc]->id == sidSrc, VERR_INVALID_PARAMETER);
    Assert(sidDest < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sidDest < pState->cSurfaces && pState->papSurfaces[sidDest]->id == sidDest, VERR_INVALID_PARAMETER);

    for (uint32_t i = 0; i < cCopyBoxes; i++)
    {
        SVGA3dBox destBox, srcBox;

        srcBox.x = pBox[i].srcx;
        srcBox.y = pBox[i].srcy;
        srcBox.z = pBox[i].srcz;
        srcBox.w = pBox[i].w;
        srcBox.h = pBox[i].h;
        srcBox.d = pBox[i].d;

        destBox.x = pBox[i].x;
        destBox.y = pBox[i].y;
        destBox.z = pBox[i].z;
        destBox.w = pBox[i].w;
        destBox.h = pBox[i].h;
        destBox.d = pBox[i].d;

        rc = vmsvga3dSurfaceStretchBlt(pThis, &dest, &destBox, &src, &srcBox, SVGA3D_STRETCH_BLT_LINEAR);
        AssertRCReturn(rc, rc);
    }
    return VINF_SUCCESS;
}


/**
 * Save texture unpacking parameters and loads those appropriate for the given
 * surface.
 *
 * @param   pState              The VMSVGA3D state structure.
 * @param   pContext            The active context.
 * @param   pSurface            The surface.
 * @param   pSave               Where to save stuff.
 */
void vmsvga3dOglSetUnpackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                                PVMSVGAPACKPARAMS pSave)
{
    RT_NOREF(pState);

    /*
     * Save (ignore errors, setting the defaults we want and avoids restore).
     */
    pSave->iAlignment = 1;
    VMSVGA3D_ASSERT_GL_CALL(glGetIntegerv(GL_UNPACK_ALIGNMENT, &pSave->iAlignment), pState, pContext);
    pSave->cxRow = 0;
    VMSVGA3D_ASSERT_GL_CALL(glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pSave->cxRow), pState, pContext);

#ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    pSave->cyImage = 0;
    glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &pSave->cyImage);
    Assert(pSave->cyImage == 0);

    pSave->fSwapBytes = GL_FALSE;
    glGetBooleanv(GL_UNPACK_SWAP_BYTES, &pSave->fSwapBytes);
    Assert(pSave->fSwapBytes == GL_FALSE);

    pSave->fLsbFirst = GL_FALSE;
    glGetBooleanv(GL_UNPACK_LSB_FIRST, &pSave->fLsbFirst);
    Assert(pSave->fLsbFirst == GL_FALSE);

    pSave->cSkipRows = 0;
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &pSave->cSkipRows);
    Assert(pSave->cSkipRows == 0);

    pSave->cSkipPixels = 0;
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &pSave->cSkipPixels);
    Assert(pSave->cSkipPixels == 0);

    pSave->cSkipImages = 0;
    glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &pSave->cSkipImages);
    Assert(pSave->cSkipImages == 0);

    VMSVGA3D_CLEAR_GL_ERRORS();
#endif

    /*
     * Setup unpack.
     *
     * Note! We use 1 as alignment here because we currently don't do any
     *       aligning of line pitches anywhere.
     */
    NOREF(pSurface);
    if (pSave->iAlignment != 1)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1), pState, pContext);
    if (pSave->cxRow != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0), pState, pContext);
#ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    if (pSave->cyImage != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0), pState, pContext);
    if (pSave->fSwapBytes != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE), pState, pContext);
    if (pSave->fLsbFirst != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE), pState, pContext);
    if (pSave->cSkipRows != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0), pState, pContext);
    if (pSave->cSkipPixels != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0), pState, pContext);
    if (pSave->cSkipImages != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0), pState, pContext);
#endif
}


/**
 * Restores texture unpacking parameters.
 *
 * @param   pState              The VMSVGA3D state structure.
 * @param   pContext            The active context.
 * @param   pSurface            The surface.
 * @param   pSave               Where stuff was saved.
 */
void vmsvga3dOglRestoreUnpackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                                    PCVMSVGAPACKPARAMS pSave)
{
    RT_NOREF(pState, pSurface);

    if (pSave->iAlignment != 1)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, pSave->iAlignment), pState, pContext);
    if (pSave->cxRow != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, pSave->cxRow), pState, pContext);
#ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    if (pSave->cyImage != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, pSave->cyImage), pState, pContext);
    if (pSave->fSwapBytes != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SWAP_BYTES, pSave->fSwapBytes), pState, pContext);
    if (pSave->fLsbFirst != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_LSB_FIRST, pSave->fLsbFirst), pState, pContext);
    if (pSave->cSkipRows != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SKIP_ROWS, pSave->cSkipRows), pState, pContext);
    if (pSave->cSkipPixels != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, pSave->cSkipPixels), pState, pContext);
    if (pSave->cSkipImages != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_UNPACK_SKIP_IMAGES, pSave->cSkipImages), pState, pContext);
#endif
}


/**
 * Create D3D/OpenGL texture object for the specified surface.
 *
 * Surfaces are created when needed.
 *
 * @param   pState              The VMSVGA3d state.
 * @param   pContext            The context.
 * @param   idAssociatedContext Probably the same as pContext->id.
 * @param   pSurface            The surface to create the texture for.
 */
int vmsvga3dBackCreateTexture(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t idAssociatedContext,
                              PVMSVGA3DSURFACE pSurface)
{
    RT_NOREF(idAssociatedContext);
    GLint activeTexture = 0;
    uint32_t idPrevCtx = pState->idActiveContext;
    pContext = &pState->SharedCtx;
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    glGenTextures(1, &pSurface->oglId.texture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    /** @todo Set the mip map generation filter settings. */

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeTexture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Must bind texture to the current context in order to change it. */
    glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Set the unpacking parameters. */
    VMSVGAPACKPARAMS SavedParams;
    vmsvga3dOglSetUnpackParams(pState, pContext, pSurface, &SavedParams);

    /* Set the mipmap base and max level paramters. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, pSurface->faces[0].numMipLevels);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    if (pSurface->fDirty)
        Log(("vmsvga3dBackCreateTexture: sync dirty texture\n"));

    /* Always allocate and initialize all mipmap levels; non-initialized mipmap levels used as render targets cause failures. */
    for (uint32_t i = 0; i < pSurface->faces[0].numMipLevels; i++)
    {
        /* Allocate and initialize texture memory.  Passing the zero filled pSurfaceData avoids
            exposing random host memory to the guest and helps a with the fedora 21 surface
            corruption issues (launchpad, background, search field, login). */
        if (pSurface->pMipmapLevels[i].fDirty)
            Log(("vmsvga3dBackCreateTexture: sync dirty texture mipmap level %d (pitch %x)\n", i, pSurface->pMipmapLevels[i].cbSurfacePitch));

        glTexImage2D(GL_TEXTURE_2D,
                        i,
                        pSurface->internalFormatGL,
                        pSurface->pMipmapLevels[i].size.width,
                        pSurface->pMipmapLevels[i].size.height,
                        0,
                        pSurface->formatGL,
                        pSurface->typeGL,
                        pSurface->pMipmapLevels[i].pSurfaceData);

        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        pSurface->pMipmapLevels[i].fDirty = false;
    }
    pSurface->fDirty = false;

    /* Restore unpacking parameters. */
    vmsvga3dOglRestoreUnpackParams(pState, pContext, pSurface, &SavedParams);

    /* Restore the old active texture. */
    glBindTexture(GL_TEXTURE_2D, activeTexture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    pSurface->flags              |= SVGA3D_SURFACE_HINT_TEXTURE;

    if (idPrevCtx < pState->cContexts && pState->papContexts[idPrevCtx]->id == idPrevCtx)
        VMSVGA3D_SET_CURRENT_CONTEXT(pState, pState->papContexts[idPrevCtx]);
    return VINF_SUCCESS;
}


/**
 * Backend worker for implementing SVGA_3D_CMD_SURFACE_STRETCHBLT.
 *
 * @returns VBox status code.
 * @param   pThis               The VGA device instance.
 * @param   pState              The VMSVGA3d state.
 * @param   pDstSurface         The destination host surface.
 * @param   uDstMipmap          The destination mipmap level (valid).
 * @param   pDstBox             The destination box.
 * @param   pSrcSurface         The source host surface.
 * @param   uSrcMipmap          The source mimap level (valid).
 * @param   pSrcBox             The source box.
 * @param   enmMode             The strecht blt mode .
 * @param   pContext            The VMSVGA3d context (already current for OGL).
 */
int vmsvga3dBackSurfaceStretchBlt(PVGASTATE pThis, PVMSVGA3DSTATE pState,
                                  PVMSVGA3DSURFACE pDstSurface, uint32_t uDstMipmap, SVGA3dBox const *pDstBox,
                                  PVMSVGA3DSURFACE pSrcSurface, uint32_t uSrcMipmap, SVGA3dBox const *pSrcBox,
                                  SVGA3dStretchBltMode enmMode, PVMSVGA3DCONTEXT pContext)
{
    RT_NOREF(pThis);

    /* Activate the read and draw framebuffer objects. */
    pState->ext.glBindFramebuffer(GL_READ_FRAMEBUFFER, pContext->idReadFramebuffer);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    pState->ext.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pContext->idDrawFramebuffer);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Bind the source and destination objects to the right place. */
    pState->ext.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                       pSrcSurface->oglId.texture, uSrcMipmap);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    pState->ext.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                       pDstSurface->oglId.texture, uDstMipmap);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    Log(("src conv. (%d,%d)(%d,%d); dest conv (%d,%d)(%d,%d)\n", pSrcBox->x, D3D_TO_OGL_Y_COORD(pSrcSurface, pSrcBox->y + pSrcBox->h),
         pSrcBox->x + pSrcBox->w, D3D_TO_OGL_Y_COORD(pSrcSurface, pSrcBox->y), pDstBox->x, D3D_TO_OGL_Y_COORD(pDstSurface, pDstBox->y + pDstBox->h),
         pDstBox->x + pDstBox->w, D3D_TO_OGL_Y_COORD(pDstSurface, pDstBox->y)));

    pState->ext.glBlitFramebuffer(pSrcBox->x,
#ifdef MANUAL_FLIP_SURFACE_DATA
                                  D3D_TO_OGL_Y_COORD(pSrcSurface, pSrcBox->y + pSrcBox->h), /* inclusive */
#else
                                  pSrcBox->y,
#endif
                                  pSrcBox->x + pSrcBox->w,                                  /* exclusive. */
#ifdef MANUAL_FLIP_SURFACE_DATA
                                  D3D_TO_OGL_Y_COORD(pSrcSurface, pSrcBox->y),              /* exclusive */
#else
                                  pSrcBox->y + pSrcBox->h,
#endif
                                  pDstBox->x,
#ifdef MANUAL_FLIP_SURFACE_DATA
                                  D3D_TO_OGL_Y_COORD(pDstSurface, pDstBox->y + pDstBox->h), /* inclusive. */
#else
                                  pDstBox->y,
#endif
                                  pDstBox->x + pDstBox->w,                                  /* exclusive. */
#ifdef MANUAL_FLIP_SURFACE_DATA
                                  D3D_TO_OGL_Y_COORD(pDstSurface, pDstBox->y),              /* exclusive */
#else
                                  pDstBox->y + pDstBox->h,
#endif
                                  GL_COLOR_BUFFER_BIT,
                                  (enmMode == SVGA3D_STRETCH_BLT_POINT) ? GL_NEAREST : GL_LINEAR);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Reset the frame buffer association */
    pState->ext.glBindFramebuffer(GL_FRAMEBUFFER, pContext->idFramebuffer);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    return VINF_SUCCESS;
}

/**
 * Save texture packing parameters and loads those appropriate for the given
 * surface.
 *
 * @param   pState              The VMSVGA3D state structure.
 * @param   pContext            The active context.
 * @param   pSurface            The surface.
 * @param   pSave               Where to save stuff.
 */
void vmsvga3dOglSetPackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                              PVMSVGAPACKPARAMS pSave)
{
    RT_NOREF(pState);
    /*
     * Save (ignore errors, setting the defaults we want and avoids restore).
     */
    pSave->iAlignment = 1;
    VMSVGA3D_ASSERT_GL_CALL(glGetIntegerv(GL_UNPACK_ALIGNMENT, &pSave->iAlignment), pState, pContext);
    pSave->cxRow = 0;
    VMSVGA3D_ASSERT_GL_CALL(glGetIntegerv(GL_UNPACK_ROW_LENGTH, &pSave->cxRow), pState, pContext);

#ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    pSave->cyImage = 0;
    glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &pSave->cyImage);
    Assert(pSave->cyImage == 0);

    pSave->fSwapBytes = GL_FALSE;
    glGetBooleanv(GL_PACK_SWAP_BYTES, &pSave->fSwapBytes);
    Assert(pSave->fSwapBytes == GL_FALSE);

    pSave->fLsbFirst = GL_FALSE;
    glGetBooleanv(GL_PACK_LSB_FIRST, &pSave->fLsbFirst);
    Assert(pSave->fLsbFirst == GL_FALSE);

    pSave->cSkipRows = 0;
    glGetIntegerv(GL_PACK_SKIP_ROWS, &pSave->cSkipRows);
    Assert(pSave->cSkipRows == 0);

    pSave->cSkipPixels = 0;
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &pSave->cSkipPixels);
    Assert(pSave->cSkipPixels == 0);

    pSave->cSkipImages = 0;
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &pSave->cSkipImages);
    Assert(pSave->cSkipImages == 0);

    VMSVGA3D_CLEAR_GL_ERRORS();
#endif

    /*
     * Setup unpack.
     *
     * Note! We use 1 as alignment here because we currently don't do any
     *       aligning of line pitches anywhere.
     */
    NOREF(pSurface);
    if (pSave->iAlignment != 1)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 1), pState, pContext);
    if (pSave->cxRow != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_ROW_LENGTH, 0), pState, pContext);
#ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    if (pSave->cyImage != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0), pState, pContext);
    if (pSave->fSwapBytes != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE), pState, pContext);
    if (pSave->fLsbFirst != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE), pState, pContext);
    if (pSave->cSkipRows != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SKIP_ROWS, 0), pState, pContext);
    if (pSave->cSkipPixels != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SKIP_PIXELS, 0), pState, pContext);
    if (pSave->cSkipImages != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SKIP_IMAGES, 0), pState, pContext);
#endif
}


/**
 * Restores texture packing parameters.
 *
 * @param   pState              The VMSVGA3D state structure.
 * @param   pContext            The active context.
 * @param   pSurface            The surface.
 * @param   pSave               Where stuff was saved.
 */
void vmsvga3dOglRestorePackParams(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface,
                                  PCVMSVGAPACKPARAMS pSave)
{
    RT_NOREF(pState, pSurface);
    if (pSave->iAlignment != 1)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, pSave->iAlignment), pState, pContext);
    if (pSave->cxRow != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_ROW_LENGTH, pSave->cxRow), pState, pContext);
#ifdef VMSVGA3D_PARANOID_TEXTURE_PACKING
    if (pSave->cyImage != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_IMAGE_HEIGHT, pSave->cyImage), pState, pContext);
    if (pSave->fSwapBytes != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SWAP_BYTES, pSave->fSwapBytes), pState, pContext);
    if (pSave->fLsbFirst != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_LSB_FIRST, pSave->fLsbFirst), pState, pContext);
    if (pSave->cSkipRows != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SKIP_ROWS, pSave->cSkipRows), pState, pContext);
    if (pSave->cSkipPixels != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SKIP_PIXELS, pSave->cSkipPixels), pState, pContext);
    if (pSave->cSkipImages != 0)
        VMSVGA3D_ASSERT_GL_CALL(glPixelStorei(GL_PACK_SKIP_IMAGES, pSave->cSkipImages), pState, pContext);
#endif
}


/**
 * Backend worker for implementing SVGA_3D_CMD_SURFACE_DMA that copies one box.
 *
 * @returns Failure status code or @a rc.
 * @param   pThis               The VGA device instance data.
 * @param   pState              The VMSVGA3d state.
 * @param   pSurface            The host surface.
 * @param   uHostMipmap         The host mipmap level (valid).
 * @param   GuestPtr            The guest pointer.
 * @param   cbSrcPitch          The guest (?) pitch.
 * @param   transfer            The transfer direction.
 * @param   pBox                The box to copy.
 * @param   pContext            The context (for OpenGL).
 * @param   rc                  The current rc for all boxes.
 * @param   iBox                The current box number (for Direct 3D).
 */
int vmsvga3dBackSurfaceDMACopyBox(PVGASTATE pThis, PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface, uint32_t uHostMipmap,
                                  SVGAGuestPtr GuestPtr, uint32_t cbSrcPitch, SVGA3dTransferType transfer,
                                  SVGA3dCopyBox const *pBox, PVMSVGA3DCONTEXT pContext, int rc, int iBox)
{
    RT_NOREF(iBox);
    PVMSVGA3DMIPMAPLEVEL pMipLevel = &pSurface->pMipmapLevels[uHostMipmap];

    switch (pSurface->flags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK)
    {
    case SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
    case SVGA3D_SURFACE_HINT_TEXTURE:
    case SVGA3D_SURFACE_HINT_RENDERTARGET:
    {
        uint32_t cbSurfacePitch;
        uint8_t *pDoubleBuffer, *pBufferStart;
        unsigned uDestOffset = 0;

        pDoubleBuffer = (uint8_t *)RTMemAlloc(pMipLevel->cbSurface);
        AssertReturn(pDoubleBuffer, VERR_NO_MEMORY);

        if (transfer == SVGA3D_READ_HOST_VRAM)
        {
            GLint activeTexture;

            /* Must bind texture to the current context in order to read it. */
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeTexture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            /* Set row length and alignment of the input data. */
            VMSVGAPACKPARAMS SavedParams;
            vmsvga3dOglSetPackParams(pState, pContext, pSurface, &SavedParams);

            glGetTexImage(GL_TEXTURE_2D,
                          uHostMipmap,
                          pSurface->formatGL,
                          pSurface->typeGL,
                          pDoubleBuffer);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            vmsvga3dOglRestorePackParams(pState, pContext, pSurface, &SavedParams);

            /* Restore the old active texture. */
            glBindTexture(GL_TEXTURE_2D, activeTexture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            uDestOffset = pBox->x * pSurface->cbBlock + pBox->y * pMipLevel->cbSurfacePitch;
            AssertReturnStmt(   uDestOffset + pBox->w * pSurface->cbBlock + (pBox->h - 1) * pMipLevel->cbSurfacePitch
                             <= pMipLevel->cbSurface,
                             RTMemFree(pDoubleBuffer),
                             VERR_INTERNAL_ERROR);

            cbSurfacePitch = pMipLevel->cbSurfacePitch;

#ifdef MANUAL_FLIP_SURFACE_DATA
            pBufferStart =   pDoubleBuffer
                           + pBox->x * pSurface->cbBlock
                           + pMipLevel->cbSurface - pBox->y * cbSurfacePitch
                           - cbSurfacePitch;      /* flip image during copy */
#else
            pBufferStart = pDoubleBuffer + uDestOffset;
#endif
        }
        else
        {
            cbSurfacePitch = pBox->w * pSurface->cbBlock;
#ifdef MANUAL_FLIP_SURFACE_DATA
            pBufferStart = pDoubleBuffer + cbSurfacePitch * pBox->h - cbSurfacePitch;      /* flip image during copy */
#else
            pBufferStart = pDoubleBuffer;
#endif
        }

        rc = vmsvgaGMRTransfer(pThis,
                               transfer,
                               pBufferStart,
#ifdef MANUAL_FLIP_SURFACE_DATA
                               -(int32_t)cbSurfacePitch,
#else
                               (int32_t)cbSurfacePitch,
#endif
                               GuestPtr,
                               pBox->srcx * pSurface->cbBlock + pBox->srcy * cbSrcPitch,
                               cbSrcPitch,
                               pBox->w * pSurface->cbBlock,
                               pBox->h);
        AssertRC(rc);

        /* Update the opengl surface data. */
        if (transfer == SVGA3D_WRITE_HOST_VRAM)
        {
            GLint activeTexture = 0;

            glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeTexture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            /* Must bind texture to the current context in order to change it. */
            glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            Log(("vmsvga3dSurfaceDMA: copy texture mipmap level %d (pitch %x)\n", uHostMipmap, pMipLevel->cbSurfacePitch));

            /* Set row length and alignment of the input data. */
            VMSVGAPACKPARAMS SavedParams;
            vmsvga3dOglSetUnpackParams(pState, pContext, pSurface, &SavedParams); /** @todo do we need to set ROW_LENGTH to w here? */

            glTexSubImage2D(GL_TEXTURE_2D,
                            uHostMipmap,
                            pBox->x,
                            pBox->y,
                            pBox->w,
                            pBox->h,
                            pSurface->formatGL,
                            pSurface->typeGL,
                            pDoubleBuffer);

            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            /* Restore old values. */
            vmsvga3dOglRestoreUnpackParams(pState, pContext, pSurface, &SavedParams);

            /* Restore the old active texture. */
            glBindTexture(GL_TEXTURE_2D, activeTexture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
        }

        Log4(("first line:\n%.*Rhxd\n", pBox->w * pSurface->cbBlock, pDoubleBuffer));

        /* Free the double buffer. */
        RTMemFree(pDoubleBuffer);
        break;
    }

    case SVGA3D_SURFACE_HINT_DEPTHSTENCIL:
        AssertFailed(); /** @todo DMA SVGA3D_SURFACE_HINT_DEPTHSTENCIL */
        break;

    case SVGA3D_SURFACE_HINT_VERTEXBUFFER | SVGA3D_SURFACE_HINT_INDEXBUFFER:
    case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
    case SVGA3D_SURFACE_HINT_INDEXBUFFER:
    {
        Assert(pBox->h == 1);

        VMSVGA3D_CLEAR_GL_ERRORS();
        pState->ext.glBindBuffer(GL_ARRAY_BUFFER, pSurface->oglId.buffer);
        if (VMSVGA3D_GL_IS_SUCCESS(pContext))
        {
            GLenum enmGlTransfer = (transfer == SVGA3D_READ_HOST_VRAM) ? GL_READ_ONLY : GL_WRITE_ONLY;
            uint8_t *pbData = (uint8_t *)pState->ext.glMapBuffer(GL_ARRAY_BUFFER, enmGlTransfer);
            if (RT_LIKELY(pbData != NULL))
            {
#if defined(VBOX_STRICT) && defined(RT_OS_DARWIN)
                GLint cbStrictBufSize;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &cbStrictBufSize);
                Assert(VMSVGA3D_GL_IS_SUCCESS(pContext));
                AssertMsg(cbStrictBufSize >= (int32_t)pMipLevel->cbSurface,
                          ("cbStrictBufSize=%#x cbSurface=%#x pContext->id=%#x\n", (uint32_t)cbStrictBufSize, pMipLevel->cbSurface, pContext->id));
#endif

                unsigned offDst = pBox->x * pSurface->cbBlock + pBox->y * pMipLevel->cbSurfacePitch;
                if (RT_LIKELY(   offDst + pBox->w * pSurface->cbBlock  + (pBox->h - 1) * pMipLevel->cbSurfacePitch
                              <= pMipLevel->cbSurface))
                {
                    Log(("Lock %s memory for rectangle (%d,%d)(%d,%d)\n",
                         (pSurface->flags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK) == SVGA3D_SURFACE_HINT_VERTEXBUFFER ? "vertex" :
                           (pSurface->flags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK) == SVGA3D_SURFACE_HINT_INDEXBUFFER ? "index" : "buffer",
                         pBox->x, pBox->y, pBox->x + pBox->w, pBox->y + pBox->h));

                    rc = vmsvgaGMRTransfer(pThis,
                                           transfer,
                                           pbData + offDst,
                                           pMipLevel->cbSurfacePitch,
                                           GuestPtr,
                                           pBox->srcx * pSurface->cbBlock + pBox->srcy * cbSrcPitch,
                                           cbSrcPitch,
                                           pBox->w * pSurface->cbBlock,
                                           pBox->h);
                    AssertRC(rc);

                    Log4(("first line:\n%.*Rhxd\n", cbSrcPitch, pbData));
                }
                else
                {
                    AssertFailed();
                    rc = VERR_INTERNAL_ERROR;
                }

                pState->ext.glUnmapBuffer(GL_ARRAY_BUFFER);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            }
            else
                VMSVGA3D_GL_GET_AND_COMPLAIN(pState, pContext, ("glMapBuffer(GL_ARRAY_BUFFER, %#x) -> NULL\n", enmGlTransfer));
        }
        else
            VMSVGA3D_GL_COMPLAIN(pState, pContext, ("glBindBuffer(GL_ARRAY_BUFFER, %#x)\n", pSurface->oglId.buffer));
        pState->ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        break;
    }

    default:
        AssertFailed();
        break;
    }

    return rc;
}


int vmsvga3dSurfaceBlitToScreen(PVGASTATE pThis, uint32_t dest, SVGASignedRect destRect, SVGA3dSurfaceImageId src, SVGASignedRect srcRect, uint32_t cRects, SVGASignedRect *pRect)
{
    /* Requires SVGA_FIFO_CAP_SCREEN_OBJECT support */
    Log(("vmsvga3dSurfaceBlitToScreen: dest=%d (%d,%d)(%d,%d) surface=%x (face=%d, mipmap=%d) (%d,%d)(%d,%d) cRects=%d\n", dest, destRect.left, destRect.top, destRect.right, destRect.bottom, src.sid, src.face, src.mipmap, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom, cRects));
    for (uint32_t i = 0; i < cRects; i++)
    {
        Log(("vmsvga3dSurfaceBlitToScreen: clipping rect %d (%d,%d)(%d,%d)\n", i, pRect[i].left, pRect[i].top, pRect[i].right, pRect[i].bottom));
    }

    /** @todo Only screen 0 for now. */
    AssertReturn(dest == 0, VERR_INTERNAL_ERROR);
    AssertReturn(src.mipmap == 0 && src.face == 0, VERR_INVALID_PARAMETER);
    /** @todo scaling */
    AssertReturn(destRect.right - destRect.left == srcRect.right - srcRect.left && destRect.bottom - destRect.top == srcRect.bottom - srcRect.top, VERR_INVALID_PARAMETER);

    if (cRects == 0)
    {
        /* easy case; no clipping */
        SVGA3dCopyBox        box;
        SVGA3dGuestImage     dst;

        box.x       = destRect.left;
        box.y       = destRect.top;
        box.z       = 0;
        box.w       = destRect.right - destRect.left;
        box.h       = destRect.bottom - destRect.top;
        box.d       = 1;
        box.srcx    = srcRect.left;
        box.srcy    = srcRect.top;
        box.srcz    = 0;

        dst.ptr.gmrId  = SVGA_GMR_FRAMEBUFFER;
        dst.ptr.offset = 0;
        dst.pitch      = pThis->svga.cbScanline;

        int rc = vmsvga3dSurfaceDMA(pThis, dst, src, SVGA3D_READ_HOST_VRAM, 1, &box);
        AssertRCReturn(rc, rc);

        vgaR3UpdateDisplay(pThis, box.x, box.y, box.w, box.h);
        return VINF_SUCCESS;
    }
    else
    {
        SVGA3dGuestImage dst;
        SVGA3dCopyBox    box;

        box.srcz    = 0;
        box.z       = 0;
        box.d       = 1;

        dst.ptr.gmrId  = SVGA_GMR_FRAMEBUFFER;
        dst.ptr.offset = 0;
        dst.pitch      = pThis->svga.cbScanline;

        /** @todo merge into one SurfaceDMA call */
        for (uint32_t i = 0; i < cRects; i++)
        {
            /* The clipping rectangle is relative to the top-left corner of srcRect & destRect. Adjust here. */
            box.srcx = srcRect.left + pRect[i].left;
            box.srcy = srcRect.top  + pRect[i].top;

            box.x    = pRect[i].left + destRect.left;
            box.y    = pRect[i].top  + destRect.top;
            box.z    = 0;
            box.w    = pRect[i].right - pRect[i].left;
            box.h    = pRect[i].bottom - pRect[i].top;

            int rc = vmsvga3dSurfaceDMA(pThis, dst, src, SVGA3D_READ_HOST_VRAM, 1, &box);
            AssertRCReturn(rc, rc);

            vgaR3UpdateDisplay(pThis, box.x, box.y, box.w, box.h);
        }

        return VINF_SUCCESS;
    }
}

int vmsvga3dGenerateMipmaps(PVGASTATE pThis, uint32_t sid, SVGA3dTextureFilter filter)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    PVMSVGA3DSURFACE    pSurface;
    int                 rc = VINF_SUCCESS;
    PVMSVGA3DCONTEXT    pContext;
    uint32_t            cid;
    GLint               activeTexture = 0;

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
    AssertReturn(sid < pState->cSurfaces && pState->papSurfaces[sid]->id == sid, VERR_INVALID_PARAMETER);

    pSurface = pState->papSurfaces[sid];

    Assert(filter != SVGA3D_TEX_FILTER_FLATCUBIC);
    Assert(filter != SVGA3D_TEX_FILTER_GAUSSIANCUBIC);
    pSurface->autogenFilter = filter;

    Log(("vmsvga3dGenerateMipmaps: sid=%x filter=%d\n", sid, filter));

    cid = SVGA3D_INVALID_ID;
    pContext = &pState->SharedCtx;
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    if (pSurface->oglId.texture == OPENGL_INVALID_ID)
    {
        /* Unknown surface type; turn it into a texture. */
        Log(("vmsvga3dGenerateMipmaps: unknown src surface id=%x type=%d format=%d -> create texture\n", sid, pSurface->flags, pSurface->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pSurface);
        AssertRCReturn(rc, rc);
    }
    else
    {
        /** @todo new filter */
        AssertFailed();
    }

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeTexture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Must bind texture to the current context in order to change it. */
    glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Generate the mip maps. */
    pState->ext.glGenerateMipmap(GL_TEXTURE_2D);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Restore the old texture. */
    glBindTexture(GL_TEXTURE_2D, activeTexture);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    return VINF_SUCCESS;
}

int vmsvga3dCommandPresent(PVGASTATE pThis, uint32_t sid, uint32_t cRects, SVGA3dCopyRect *pRect)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    PVMSVGA3DSURFACE    pSurface;
    PVMSVGA3DCONTEXT    pContext;
    uint32_t            cid;

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
    AssertReturn(sid < pState->cSurfaces && pState->papSurfaces[sid]->id == sid, VERR_INVALID_PARAMETER);

    pSurface = pState->papSurfaces[sid];

    Log(("vmsvga3dCommandPresent: sid=%x cRects=%d\n", sid, cRects));
    for (uint32_t i=0; i < cRects; i++)
        Log(("vmsvga3dCommandPresent: rectangle %d src=(%d,%d) (%d,%d)(%d,%d)\n", i, pRect[i].srcx, pRect[i].srcy, pRect[i].x, pRect[i].y, pRect[i].x + pRect[i].w, pRect[i].y + pRect[i].h));

    pContext = &pState->SharedCtx;
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
    cid = pContext->id;
    VMSVGA3D_CLEAR_GL_ERRORS();

#if 0 /* Can't make sense of this. SVGA3dCopyRect doesn't allow scaling.  non-blit-cube path change to not use it. */
    /*
     * Source surface different size?
     */
    RTRECT2 srcViewPort;
    if (   pSurface->pMipmapLevels[0].size.width  != pThis->svga.uWidth
        || pSurface->pMipmapLevels[0].size.height != pThis->svga.uHeight)
    {
        float xMultiplier = (float)pSurface->pMipmapLevels[0].size.width  / (float)pThis->svga.uWidth;
        float yMultiplier = (float)pSurface->pMipmapLevels[0].size.height / (float)pThis->svga.uHeight;

        LogFlow(("size (%d vs %d, %d vs %d) multiplier (" FLOAT_FMT_STR ", " FLOAT_FMT_STR ")\n",
                 pSurface->pMipmapLevels[0].size.width, pThis->svga.uWidth,
                 pSurface->pMipmapLevels[0].size.height, pThis->svga.uHeight,
                 FLOAT_FMT_ARGS(xMultiplier), FLOAT_FMT_ARGS(yMultiplier) ));

        srcViewPort.x  = (uint32_t)((float)pThis->svga.viewport.x  * xMultiplier);
        srcViewPort.y  = (uint32_t)((float)pThis->svga.viewport.y  * yMultiplier);
        srcViewPort.cx = (uint32_t)((float)pThis->svga.viewport.cx * xMultiplier);
        srcViewPort.cy = (uint32_t)((float)pThis->svga.viewport.cy * yMultiplier);
    }
    else
    {
        srcViewPort.x  = pThis->svga.viewport.x;
        srcViewPort.y  = pThis->svga.viewport.y;
        srcViewPort.cx = pThis->svga.viewport.cx;
        srcViewPort.cy = pThis->svga.viewport.cy;
    }
    RTRECT SrcViewPortRect;
    SrcViewPortRect.xLeft   = srcViewPort.x;
    SrcViewPortRect.xRight  = srcViewPort.x + srcViewPort.cx;
    SrcViewPortRect.yBottom = srcViewPort.y;
    SrcViewPortRect.yTop    = srcViewPort.y + srcViewPort.cy;
#endif


#if 0//ndef RT_OS_DARWIN /* blit-cube fails in this path... */
    /*
     * Note! this path is slightly faster than the glBlitFrameBuffer path below.
     */
    SVGA3dCopyRect rect;
    uint32_t oldVShader, oldPShader;
    GLint oldTextureId;

    if (cRects == 0)
    {
        rect.x = rect.y = rect.srcx = rect.srcy = 0;
        rect.w = pSurface->pMipmapLevels[0].size.width;
        rect.h = pSurface->pMipmapLevels[0].size.height;
        pRect  = &rect;
        cRects = 1;
    }

    //glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

#if 0
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, 0, pSurface->pMipmapLevels[0].size.width, pSurface->pMipmapLevels[0].size.height);
#endif

    VMSVGA3D_ASSERT_GL_CALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureId), pState, pContext);

    oldVShader = pContext->state.shidVertex;
    oldPShader = pContext->state.shidPixel;
    vmsvga3dShaderSet(pThis, pContext, cid, SVGA3D_SHADERTYPE_VS, SVGA_ID_INVALID);
    vmsvga3dShaderSet(pThis, pContext, cid, SVGA3D_SHADERTYPE_PS, SVGA_ID_INVALID);

    /* Flush shader changes. */
    if (pContext->pShaderContext)
        ShaderUpdateState(pContext->pShaderContext, 0);

    /* Activate the read and draw framebuffer objects. */
    VMSVGA3D_ASSERT_GL_CALL(pState->ext.glBindFramebuffer(GL_READ_FRAMEBUFFER, pContext->idReadFramebuffer), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(pState->ext.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 /* back buffer */), pState, pContext);

    VMSVGA3D_ASSERT_GL_CALL(pState->ext.glActiveTexture(GL_TEXTURE0), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glEnable(GL_TEXTURE_2D), pState, pContext);;
    VMSVGA3D_ASSERT_GL_CALL(glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture), pState, pContext);

    VMSVGA3D_ASSERT_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR), pState, pContext);;

#if 0
    VMSVGA3D_ASSERT_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP), pState, pContext);;
    VMSVGA3D_ASSERT_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP), pState, pContext);;
#endif

    /* Reset the transformation matrices. */
    VMSVGA3D_ASSERT_GL_CALL(glMatrixMode(GL_MODELVIEW), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glPushMatrix(), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glLoadIdentity(), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glMatrixMode(GL_PROJECTION), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glPushMatrix(), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glLoadIdentity(), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glScalef(1.0f, -1.0f, 1.0f), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glOrtho(0, pThis->svga.uWidth, pThis->svga.uHeight, 0, 0.0, -1.0), pState, pContext);

    for (uint32_t i = 0; i < cRects; i++)
    {
        float left, right, top, bottom; /* Texture coordinates */
        int   vertexLeft, vertexRight, vertexTop, vertexBottom;

        pRect[i].srcx = RT_MAX(pRect[i].srcx, (uint32_t)RT_MAX(srcViewPort.x, 0));
        pRect[i].srcy = RT_MAX(pRect[i].srcy, (uint32_t)RT_MAX(srcViewPort.y, 0));
        pRect[i].x    = RT_MAX(pRect[i].x, pThis->svga.viewport.x) - pThis->svga.viewport.x;
        pRect[i].y    = RT_MAX(pRect[i].y, pThis->svga.viewport.y) - pThis->svga.viewport.y;
        pRect[i].w    = pThis->svga.viewport.cx;
        pRect[i].h    = pThis->svga.viewport.cy;

        if (    pRect[i].x + pRect[i].w <= pThis->svga.viewport.x
            ||  pThis->svga.viewport.x + pThis->svga.viewport.cx <= pRect[i].x
            ||  pRect[i].y + pRect[i].h <= pThis->svga.viewport.y
            ||  pThis->svga.viewport.y + pThis->svga.viewport.cy <= pRect[i].y)
        {
            /* Intersection is empty; skip */
            continue;
        }

        left   = pRect[i].srcx;
        right  = pRect[i].srcx + pRect[i].w;
        top    = pRect[i].srcy + pRect[i].h;
        bottom = pRect[i].srcy;

        left   /= pSurface->pMipmapLevels[0].size.width;
        right  /= pSurface->pMipmapLevels[0].size.width;
        top    /= pSurface->pMipmapLevels[0].size.height;
        bottom /= pSurface->pMipmapLevels[0].size.height;

        vertexLeft   = pRect[i].x;
        vertexRight  = pRect[i].x + pRect[i].w;
        vertexTop    = ((uint32_t)pThis->svga.uHeight >= pRect[i].y + pRect[i].h) ? pThis->svga.uHeight - pRect[i].y - pRect[i].h : 0;
        vertexBottom = pThis->svga.uHeight - pRect[i].y;

        Log(("view port (%d,%d)(%d,%d)\n", srcViewPort.x, srcViewPort.y, srcViewPort.cx, srcViewPort.cy));
        Log(("vertex (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n", vertexLeft, vertexBottom, vertexLeft, vertexTop, vertexRight, vertexTop, vertexRight, vertexBottom));
        Log(("texture (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n", pRect[i].srcx, pSurface->pMipmapLevels[0].size.height - (pRect[i].srcy + pRect[i].h), pRect[i].srcx, pSurface->pMipmapLevels[0].size.height - pRect[i].srcy, pRect[i].srcx + pRect[i].w, pSurface->pMipmapLevels[0].size.height - pRect[i].srcy, pRect[i].srcx + pRect[i].w, pSurface->pMipmapLevels[0].size.height - (pRect[i].srcy + pRect[i].h)));

        glBegin(GL_QUADS);

        /* bottom left */
        glTexCoord2f(left, bottom);
        glVertex2i(vertexLeft, vertexBottom);

        /* top left */
        glTexCoord2f(left, top);
        glVertex2i(vertexLeft, vertexTop);

        /* top right */
        glTexCoord2f(right, top);
        glVertex2i(vertexRight, vertexTop);

        /* bottom right */
        glTexCoord2f(right, bottom);
        glVertex2i(vertexRight, vertexBottom);

        VMSVGA3D_ASSERT_GL_CALL(glEnd(), pState, pContext);
    }

    /* Restore old settings. */
    VMSVGA3D_ASSERT_GL_CALL(glMatrixMode(GL_PROJECTION), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glPopMatrix(), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glMatrixMode(GL_MODELVIEW), pState, pContext);
    VMSVGA3D_ASSERT_GL_CALL(glPopMatrix(), pState, pContext);

    //VMSVGA3D_ASSERT_GL_CALL(glPopAttrib(), pState, pContext);

    VMSVGA3D_ASSERT_GL_CALL(glBindTexture(GL_TEXTURE_2D, oldTextureId), pState, pContext);
    vmsvga3dShaderSet(pThis, pContext, cid, SVGA3D_SHADERTYPE_VS, oldVShader);
    vmsvga3dShaderSet(pThis, pContext, cid, SVGA3D_SHADERTYPE_PS, oldPShader);

#else
    /*
     * glBlitFramebuffer variant.
     */
    /* Activate the read and draw framebuffer objects. */
    pState->ext.glBindFramebuffer(GL_READ_FRAMEBUFFER, pContext->idReadFramebuffer);
    VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
    pState->ext.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 /* back buffer */);
    VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

    /* Bind the source objects to the right place. */
    pState->ext.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pSurface->oglId.texture, 0 /* level 0 */);
    VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);


    /* Read the destination viewport specs in one go to try avoid some unnecessary update races. */
    VMSVGAVIEWPORT const DstViewport   = pThis->svga.viewport;
    ASMCompilerBarrier(); /* paranoia */
    Assert(DstViewport.yHighWC >= DstViewport.yLowWC);

    /* If there are no recangles specified, just grab a screenful. */
    SVGA3dCopyRect DummyRect;
    if (cRects != 0)
    { /* likely */ }
    else
    {
        /** @todo Find the usecase for this or check what the original device does.
         *        The original code was doing some scaling based on the surface
         *        size... */
# ifdef DEBUG_bird
        AssertMsgFailed(("No rects to present. Who is doing that and what do they actually expect?\n"));
# endif
        DummyRect.x = DummyRect.srcx = 0;
        DummyRect.y = DummyRect.srcy = 0;
        DummyRect.w = pThis->svga.uWidth;
        DummyRect.h = pThis->svga.uHeight;
        cRects = 1;
        pRect  = &DummyRect;
    }

    /*
     * Blit the surface rectangle(s) to the back buffer.
     */
    uint32_t const cxSurface = pSurface->pMipmapLevels[0].size.width;
    uint32_t const cySurface = pSurface->pMipmapLevels[0].size.height;
    for (uint32_t i = 0; i < cRects; i++)
    {
        SVGA3dCopyRect ClippedRect = pRect[i];

        /*
         * Do some sanity checking and limit width and height, all so we
         * don't need to think about wrap-arounds below.
         */
        if (RT_LIKELY(   ClippedRect.w
                      && ClippedRect.x    < VMSVGA_MAX_X
                      && ClippedRect.srcx < VMSVGA_MAX_X
                      && ClippedRect.h
                      && ClippedRect.y    < VMSVGA_MAX_Y
                      && ClippedRect.srcy < VMSVGA_MAX_Y
                         ))
        { /* likely */ }
        else
            continue;

        if (RT_LIKELY(ClippedRect.w < VMSVGA_MAX_X))
        { /* likely */ }
        else
            ClippedRect.w = VMSVGA_MAX_X;
        if (RT_LIKELY(ClippedRect.h < VMSVGA_MAX_Y))
        { /* likely */ }
        else
            ClippedRect.h = VMSVGA_MAX_Y;


        /*
         * Source surface clipping (paranoia). Straight forward.
         */
        if (RT_LIKELY(ClippedRect.srcx < cxSurface))
        { /* likely */ }
        else
            continue;
        if (RT_LIKELY(ClippedRect.srcx + ClippedRect.w <= cxSurface))
        { /* likely */ }
        else
        {
            AssertFailed(); /* remove if annoying. */
            ClippedRect.w = cxSurface - ClippedRect.srcx;
        }

        if (RT_LIKELY(ClippedRect.srcy < cySurface))
        { /* likely */ }
        else
            continue;
        if (RT_LIKELY(ClippedRect.srcy + ClippedRect.h <= cySurface))
        { /* likely */ }
        else
        {
            AssertFailed(); /* remove if annoying. */
            ClippedRect.h = cySurface - ClippedRect.srcy;
        }

        /*
         * Destination viewport clipping - real PITA.
         *
         * We have to take the following into account here:
         *  - The source image is Y inverted.
         *  - The destination framebuffer is in world and not window coordinates,
         *    just like the source surface.  This means working in the first quadrant.
         *  - The viewport is in window coordinate, that is fourth quadrant and
         *    negated Y values.
         *  - The destination framebuffer is not scrolled, so we have to blit
         *    what's visible into the top of the framebuffer.
         *
         *
         *  To illustrate:
         *
         *        source              destination        0123456789
         *     8 ^----------       8 ^----------       0 ----------->
         *     7 |         |       7 |         |       1 |         |
         *     6 |         |       6 | ******* |       2 | ******* |
         *     5 |   ***   |       5 |    *    |       3 |    *    |
         *     4 |    *    |  =>   4 |    *    |  =>   4 |    *    |
         *     3 |    *    |       3 |   ***   |       5 |   ***   |
         *     2 | ******* |       2 |         |       6 |         |
         *     1 |         |       1 |         |       7 |         |
         *     0 ----------->      0 ----------->      8 v----------
         *       0123456789          0123456789          Destination window
         *
         * From the above, it follows that a destination viewport given in
         * window coordinates matches the source exactly when srcy = srcx = 0.
         *
         * Example (Y only):
         *  ySrc        =  0
         *  yDst         =  0
         *  cyCopy      =  9
         *  cyScreen    =  cyCopy
         *  cySurface  >=  cyCopy
         *  yViewport   = 5
         *  cyViewport  = 2  (i.e. '|   ***   |'
         *                         '|         |' )
         *  yWCViewportHi  = cxScreen - yViewport              = 9 - 5 = 4
         *  yWCViewportLow = cxScreen - yViewport - cyViewport = 4 - 2 = 2
         *
         * We can see from the illustration that the final result should be:
         *  SrcRect = (0,7) (11, 5)  (cy=2 from y=5)
         *  DstRect = (0,2) (11, 4)
         *
         * Let's postpone the switching of SrcRect.yBottom/yTop to make it
         * easier to follow:
         *  SrcRect = (0,5) (11, 7)
         *
         * From the top, Y values only:
         *  0. Copy = { .yDst = 0, .ySrc = 0, .cy = 9 }
         *
         *  1. CopyRect.yDst (=0) is lower than yWCViewportLow:
         *      cyAdjust = yWCViewportLow - CopyRect.yDst = 2;
         *      Copy.yDst += cyAdjust = 2;
         *      Copy.ySrc  = unchanged;
         *      Copy.cx   -= cyAdjust = 7;
         *   => Copy = { .yDst = 2, .ySrc = 0, .cy = 7 }
         *
         *  2. CopyRect.yDst + CopyRect.cx (=9) is higher than yWCViewportHi:
         *      cyAdjust = CopyRect.yDst + CopyRect.cx - yWCViewportHi = 9 - 4 = 5
         *      Copy.yDst  = unchanged;
         *      Copy.ySrc += cyAdjust = 5;
         *      Copy.cx   -= cyAdjust = 2;
         *   => Copy = { .yDst = 2, .ySrc = 5, .cy = 2 }
         *
         * Update: On darwin, it turns out that when we call [NSOpenGLContext updates]
         *         when the view is resized, moved and otherwise messed with,
         *         the visible part of the framebuffer is actually the bottom
         *         one.  It's easy to adjust for this, just have to adjust the
         *         destination rectangle such that yBottom is zero.
         */
        /* X - no inversion, so kind of simple. */
        if (ClippedRect.x >= DstViewport.x)
        {
            if (ClippedRect.x + ClippedRect.w <= DstViewport.xRight)
            { /* typical */ }
            else if (ClippedRect.x < DstViewport.xRight)
                ClippedRect.w = DstViewport.xRight - ClippedRect.x;
            else
                continue;
        }
        else
        {
            uint32_t cxAdjust = DstViewport.x - ClippedRect.x;
            if (cxAdjust < ClippedRect.w)
            {
                ClippedRect.w    -= cxAdjust;
                ClippedRect.x    += cxAdjust;
                ClippedRect.srcx += cxAdjust;
            }
            else
                continue;

            if (ClippedRect.x + ClippedRect.w <= DstViewport.xRight)
            { /* typical */ }
            else
                ClippedRect.w = DstViewport.xRight - ClippedRect.x;
        }

        /* Y - complicated, see above. */
        if (ClippedRect.y >= DstViewport.yLowWC)
        {
            if (ClippedRect.y + ClippedRect.h <= DstViewport.yHighWC)
            { /* typical */ }
            else if (ClippedRect.y < DstViewport.yHighWC)
            {
                /* adjustment #2 */
                uint32_t cyAdjust = ClippedRect.y + ClippedRect.h - DstViewport.yHighWC;
                ClippedRect.srcy += cyAdjust;
                ClippedRect.h    -= cyAdjust;
            }
            else
                continue;
        }
        else
        {
            /* adjustment #1 */
            uint32_t cyAdjust = DstViewport.yLowWC - ClippedRect.y;
            if (cyAdjust < ClippedRect.h)
            {
                ClippedRect.y    += cyAdjust;
                ClippedRect.h    -= cyAdjust;
            }
            else
                continue;

            if (ClippedRect.y + ClippedRect.h <= DstViewport.yHighWC)
            { /* typical */ }
            else
            {
                /* adjustment #2 */
                cyAdjust = ClippedRect.y + ClippedRect.h - DstViewport.yHighWC;
                ClippedRect.srcy += cyAdjust;
                ClippedRect.h    -= cyAdjust;
            }
        }

        /* Calc source rectangle with y flipping wrt destination. */
        RTRECT SrcRect;
        SrcRect.xLeft   = ClippedRect.srcx;
        SrcRect.xRight  = ClippedRect.srcx + ClippedRect.w;
        SrcRect.yBottom = ClippedRect.srcy + ClippedRect.h;
        SrcRect.yTop    = ClippedRect.srcy;

        /* Calc destination rectangle. */
        RTRECT DstRect;
        DstRect.xLeft   = ClippedRect.x;
        DstRect.xRight  = ClippedRect.x + ClippedRect.w;
        DstRect.yBottom = ClippedRect.y;
        DstRect.yTop    = ClippedRect.y + ClippedRect.h;

        /* Adjust for viewport. */
        DstRect.xLeft   -= DstViewport.x;
        DstRect.xRight  -= DstViewport.x;
# ifdef RT_OS_DARWIN /* We actually seeing the bottom of the FB, not the top as on windows and X11. */
        DstRect.yTop    -= DstRect.yBottom;
        DstRect.yBottom  = 0;
# else
        DstRect.yBottom += DstViewport.y;
        DstRect.yTop    += DstViewport.y;
# endif

        Log(("SrcRect: (%d,%d)(%d,%d) DstRect: (%d,%d)(%d,%d)\n",
             SrcRect.xLeft, SrcRect.yBottom, SrcRect.xRight, SrcRect.yTop,
             DstRect.xLeft, DstRect.yBottom, DstRect.xRight, DstRect.yTop));
        pState->ext.glBlitFramebuffer(SrcRect.xLeft, SrcRect.yBottom, SrcRect.xRight, SrcRect.yTop,
                                      DstRect.xLeft, DstRect.yBottom, DstRect.xRight, DstRect.yTop,
                                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

#endif

    /*
     * Flip the front and back buffers.
     */
#ifdef RT_OS_WINDOWS
    BOOL fRef = SwapBuffers(pContext->hdc);
    AssertMsg(fRef, ("SwapBuffers failed with %d\n", GetLastError())); NOREF(fRef);
#elif defined(RT_OS_DARWIN)
    vmsvga3dCocoaSwapBuffers(pContext->cocoaView, pContext->cocoaContext);
#else
    /* show the window if not already done */
    if (!pContext->fMapped)
    {
        XMapWindow(pState->display, pContext->window);
        pContext->fMapped = true;
    }
    /* now swap the buffers, i.e. display the rendering result */
    glXSwapBuffers(pState->display, pContext->window);
#endif

    /*
     * Now we can reset the frame buffer association.  Doing it earlier means no
     * output on darwin.
     */
    VMSVGA3D_ASSERT_GL_CALL(pState->ext.glBindFramebuffer(GL_FRAMEBUFFER, pContext->idFramebuffer), pState, pContext);
    return VINF_SUCCESS;
}

#ifdef RT_OS_LINUX
/**
 * X11 event handling thread.
 *
 * @returns VINF_SUCCESS (ignored)
 * @param   hThreadSelf     thread handle
 * @param   pvUser          pointer to pState structure
 */
DECLCALLBACK(int) vmsvga3dXEventThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    PVMSVGA3DSTATE pState = (PVMSVGA3DSTATE)pvUser;
    while (!pState->bTerminate)
    {
        while (XPending(pState->display) > 0)
        {
            XEvent event;
            XNextEvent(pState->display, &event);

            switch (event.type)
            {
                default:
                    break;
            }
        }
        /* sleep for 16ms to not burn too many cycles */
        RTThreadSleep(16);
    }
    return VINF_SUCCESS;
}
#endif // RT_OS_LINUX


/**
 * Create a new 3d context
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   cid             Context id
 * @param   fFlags          VMSVGA3D_DEF_CTX_F_XXX.
 */
int vmsvga3dContextDefineOgl(PVGASTATE pThis, uint32_t cid, uint32_t fFlags)
{
    int                     rc;
    PVMSVGA3DCONTEXT        pContext;
    PVMSVGA3DSTATE          pState = pThis->svga.p3dState;

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(   cid < SVGA3D_MAX_CONTEXT_IDS
                 || (cid == VMSVGA3D_SHARED_CTX_ID && (fFlags & VMSVGA3D_DEF_CTX_F_SHARED_CTX)), VERR_INVALID_PARAMETER);
#if !defined(VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE) || !(defined(RT_OS_DARWIN))
    AssertReturn(!(fFlags & VMSVGA3D_DEF_CTX_F_OTHER_PROFILE), VERR_INTERNAL_ERROR_3);
#endif

    Log(("vmsvga3dContextDefine id %x\n", cid));
#ifdef DEBUG_DEBUG_GFX_WINDOW_TEST_CONTEXT
    if (pState->idTestContext == SVGA_ID_INVALID)
    {
        pState->idTestContext = 207;
        rc = vmsvga3dContextDefine(pThis, pState->idTestContext);
        AssertRCReturn(rc, rc);
    }
#endif

    if (cid == VMSVGA3D_SHARED_CTX_ID)
        pContext = &pState->SharedCtx;
    else
    {
        if (cid >= pState->cContexts)
        {
            /* Grow the array. */
            uint32_t cNew = RT_ALIGN(cid + 15, 16);
            void *pvNew = RTMemRealloc(pState->papContexts, sizeof(pState->papContexts[0]) * cNew);
            AssertReturn(pvNew, VERR_NO_MEMORY);
            pState->papContexts = (PVMSVGA3DCONTEXT *)pvNew;
            while (pState->cContexts < cNew)
            {
                pContext = (PVMSVGA3DCONTEXT)RTMemAllocZ(sizeof(*pContext));
                AssertReturn(pContext, VERR_NO_MEMORY);
                pContext->id = SVGA3D_INVALID_ID;
                pState->papContexts[pState->cContexts++] = pContext;
            }
        }
        /* If one already exists with this id, then destroy it now. */
        if (pState->papContexts[cid]->id != SVGA3D_INVALID_ID)
            vmsvga3dContextDestroy(pThis, cid);

        pContext = pState->papContexts[cid];
    }

    /*
     * Find or create the shared context if needed (necessary for sharing e.g. textures between contexts).
     */
    PVMSVGA3DCONTEXT pSharedCtx = NULL;
    if (!(fFlags & (VMSVGA3D_DEF_CTX_F_INIT | VMSVGA3D_DEF_CTX_F_SHARED_CTX)))
    {
        pSharedCtx = &pState->SharedCtx;
        if (pSharedCtx->id != VMSVGA3D_SHARED_CTX_ID)
        {
            rc = vmsvga3dContextDefineOgl(pThis, VMSVGA3D_SHARED_CTX_ID, VMSVGA3D_DEF_CTX_F_SHARED_CTX);
            AssertLogRelRCReturn(rc, rc);
        }
    }

    /*
     * Initialize the context.
     */
    memset(pContext, 0, sizeof(*pContext));
    pContext->id                = cid;
    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->aSidActiveTexture); i++)
        pContext->aSidActiveTexture[i] = SVGA3D_INVALID_ID;

    pContext->sidRenderTarget   = SVGA3D_INVALID_ID;
    pContext->state.shidVertex  = SVGA3D_INVALID_ID;
    pContext->state.shidPixel   = SVGA3D_INVALID_ID;
    pContext->idFramebuffer     = OPENGL_INVALID_ID;
    pContext->idReadFramebuffer = OPENGL_INVALID_ID;
    pContext->idDrawFramebuffer = OPENGL_INVALID_ID;

    rc = ShaderContextCreate(&pContext->pShaderContext);
    AssertRCReturn(rc, rc);

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aRenderTargets); i++)
        pContext->state.aRenderTargets[i] = SVGA3D_INVALID_ID;

    AssertReturn(pThis->svga.u64HostWindowId, VERR_INTERNAL_ERROR);

#ifdef RT_OS_WINDOWS
    /* Create a context window. */
    CREATESTRUCT cs;
    cs.lpCreateParams   = NULL;
    cs.dwExStyle        = WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT;
# ifdef DEBUG_GFX_WINDOW
    cs.lpszName         = (char *)RTMemAllocZ(256);
    RTStrPrintf((char *)cs.lpszName, 256, "Context %d OpenGL Window", cid);
# else
    cs.lpszName         = NULL;
# endif
    cs.lpszClass        = 0;
# ifdef DEBUG_GFX_WINDOW
    cs.style            = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION;
# else
    cs.style            = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DISABLED | WS_CHILD | WS_VISIBLE;
# endif
    cs.x                = 0;
    cs.y                = 0;
    cs.cx               = pThis->svga.uWidth;
    cs.cy               = pThis->svga.uHeight;
    cs.hwndParent       = (HWND)pThis->svga.u64HostWindowId;
    cs.hMenu            = NULL;
    cs.hInstance        = pState->hInstance;

    rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_CREATEWINDOW, (WPARAM)&pContext->hwnd, (LPARAM)&cs);
    AssertRCReturn(rc, rc);

    pContext->hdc   = GetDC(pContext->hwnd);
    AssertMsgReturn(pContext->hdc, ("GetDC %x failed with %d\n", pContext->hwnd, GetLastError()), VERR_INTERNAL_ERROR);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  /*  size of this pfd */
        1,                              /* version number */
        PFD_DRAW_TO_WINDOW |            /* support window */
        PFD_DOUBLEBUFFER   |            /* support double buffering */
        PFD_SUPPORT_OPENGL,             /* support OpenGL */
        PFD_TYPE_RGBA,                  /* RGBA type */
        24,                             /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,               /* color bits ignored */
        8,                              /* alpha buffer */
        0,                              /* shift bit ignored */
        0,                              /* no accumulation buffer */
        0, 0, 0, 0,                     /* accum bits ignored */
        16,                             /* set depth buffer  */
        16,                             /* set stencil buffer */
        0,                              /* no auxiliary buffer */
        PFD_MAIN_PLANE,                 /* main layer */
        0,                              /* reserved */
        0, 0, 0                         /* layer masks ignored */
    };
    int     pixelFormat;
    BOOL    ret;

    pixelFormat = ChoosePixelFormat(pContext->hdc, &pfd);
    /** @todo is this really necessary?? */
    pixelFormat = ChoosePixelFormat(pContext->hdc, &pfd);
    AssertMsgReturn(pixelFormat != 0, ("ChoosePixelFormat failed with %d\n", GetLastError()), VERR_INTERNAL_ERROR);

    ret = SetPixelFormat(pContext->hdc, pixelFormat, &pfd);
    AssertMsgReturn(ret == TRUE, ("SetPixelFormat failed with %d\n", GetLastError()), VERR_INTERNAL_ERROR);

    pContext->hglrc = wglCreateContext(pContext->hdc);
    AssertMsgReturn(pContext->hglrc, ("wglCreateContext %x failed with %d\n", pContext->hdc, GetLastError()), VERR_INTERNAL_ERROR);

    if (pSharedCtx)
    {
        ret = wglShareLists(pSharedCtx->hglrc, pContext->hglrc);
        AssertMsg(ret == TRUE, ("wglShareLists(%p, %p) failed with %d\n", pSharedCtx->hglrc, pContext->hglrc, GetLastError()));
    }

#elif defined(RT_OS_DARWIN)
    pContext->fOtherProfile = RT_BOOL(fFlags & VMSVGA3D_DEF_CTX_F_OTHER_PROFILE);

    NativeNSOpenGLContextRef pShareContext = pSharedCtx ? pSharedCtx->cocoaContext : NULL;
    NativeNSViewRef          pHostView    = (NativeNSViewRef)pThis->svga.u64HostWindowId;
    vmsvga3dCocoaCreateViewAndContext(&pContext->cocoaView, &pContext->cocoaContext,
                                      pSharedCtx ? NULL : pHostView, /* Only attach one subview, the one we'll present in. */ /** @todo screen objects and stuff. */
                                      pThis->svga.uWidth, pThis->svga.uHeight,
                                      pShareContext, pContext->fOtherProfile);

#else
    Window hostWindow = (Window)pThis->svga.u64HostWindowId;

    if (pState->display == NULL)
    {
        /* get an X display and make sure we have glX 1.3 */
        pState->display = XOpenDisplay(0);
        Assert(pState->display);
        int glxMajor, glxMinor;
        Bool ret = glXQueryVersion(pState->display, &glxMajor, &glxMinor);
        AssertMsgReturn(ret && glxMajor == 1 && glxMinor >= 3, ("glX >=1.3 not present"), VERR_INTERNAL_ERROR);
        /* start our X event handling thread */
        rc = RTThreadCreate(&pState->pWindowThread, vmsvga3dXEventThread, pState, 0, RTTHREADTYPE_GUI, RTTHREADFLAGS_WAITABLE, "VMSVGA3DXEVENT");
        if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("%s: Async IO Thread creation for 3d window handling failed rc=%d\n", __FUNCTION__, rc));
            return rc;
        }
    }
    int attrib[] =
    {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        //GLX_ALPHA_SIZE, 1, this flips the bbos screen
        GLX_DOUBLEBUFFER,
        None
    };
    XVisualInfo *vi = glXChooseVisual(pState->display, DefaultScreen(pState->display), attrib);
    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(pState->display, XDefaultRootWindow(pState->display), vi->visual, AllocNone);
    swa.border_pixel = 0;
    swa.background_pixel = 0;
    swa.event_mask = StructureNotifyMask | ExposureMask;
    unsigned long flags = CWBorderPixel | CWBackPixel | CWColormap | CWEventMask;
    pContext->window = XCreateWindow(pState->display, hostWindow,//XDefaultRootWindow(pState->display),//hostWindow,
                                     0, 0, pThis->svga.uWidth, pThis->svga.uHeight,
                                     0, vi->depth, InputOutput,
                                     vi->visual, flags, &swa);
    AssertMsgReturn(pContext->window, ("XCreateWindow failed"), VERR_INTERNAL_ERROR);
    //uint32_t cardinal_alpha = (uint32_t) (0.5 * (uint32_t)-1); - unused

    /* the window is hidden by default and only mapped when CommandPresent is executed on it */

    GLXContext shareContext = pSharedCtx ? pSharedCtx->glxContext : NULL;
    pContext->glxContext = glXCreateContext(pState->display, vi, shareContext, GL_TRUE);
    AssertMsgReturn(pContext->glxContext, ("glXCreateContext failed"), VERR_INTERNAL_ERROR);
#endif

    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* NULL during the first PowerOn call. */
    if (pState->ext.glGenFramebuffers)
    {
        /* Create a framebuffer object for this context. */
        pState->ext.glGenFramebuffers(1, &pContext->idFramebuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        /* Bind the object to the framebuffer target. */
        pState->ext.glBindFramebuffer(GL_FRAMEBUFFER, pContext->idFramebuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        /* Create read and draw framebuffer objects for this context. */
        pState->ext.glGenFramebuffers(1, &pContext->idReadFramebuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        pState->ext.glGenFramebuffers(1, &pContext->idDrawFramebuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    }
#if 0
    /** @todo move to shader lib!!! */
    /* Clear the screen */
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClearIndex(0);
    glClearDepth(1);
    glClearStencil(0xffff);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    if (pState->ext.glProvokingVertex)
        pState->ext.glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
    /** @todo move to shader lib!!! */
#endif
    return VINF_SUCCESS;
}


/**
 * Create a new 3d context
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   cid             Context id
 */
int vmsvga3dContextDefine(PVGASTATE pThis, uint32_t cid)
{
    return vmsvga3dContextDefineOgl(pThis, cid, 0/*fFlags*/);
}

/**
 * Destroys a 3d context.
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   pContext        The context to destroy.
 * @param   cid             Context id
 */
static int vmsvga3dContextDestroyOgl(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext, uint32_t cid)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(pContext, VERR_INVALID_PARAMETER);
    AssertReturn(pContext->id == cid, VERR_INVALID_PARAMETER);
    Log(("vmsvga3dContextDestroyOgl id %x\n", cid));

    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Destroy all leftover pixel shaders. */
    for (uint32_t i = 0; i < pContext->cPixelShaders; i++)
    {
        if (pContext->paPixelShader[i].id != SVGA3D_INVALID_ID)
            vmsvga3dShaderDestroy(pThis, pContext->paPixelShader[i].cid, pContext->paPixelShader[i].id, pContext->paPixelShader[i].type);
    }
    if (pContext->paPixelShader)
        RTMemFree(pContext->paPixelShader);

    /* Destroy all leftover vertex shaders. */
    for (uint32_t i = 0; i < pContext->cVertexShaders; i++)
    {
        if (pContext->paVertexShader[i].id != SVGA3D_INVALID_ID)
            vmsvga3dShaderDestroy(pThis, pContext->paVertexShader[i].cid, pContext->paVertexShader[i].id, pContext->paVertexShader[i].type);
    }
    if (pContext->paVertexShader)
        RTMemFree(pContext->paVertexShader);

    if (pContext->state.paVertexShaderConst)
        RTMemFree(pContext->state.paVertexShaderConst);
    if (pContext->state.paPixelShaderConst)
        RTMemFree(pContext->state.paPixelShaderConst);

    if (pContext->pShaderContext)
    {
        int rc = ShaderContextDestroy(pContext->pShaderContext);
        AssertRC(rc);
    }

    if (pContext->idFramebuffer != OPENGL_INVALID_ID)
    {
        /* Unbind the object from the framebuffer target. */
        pState->ext.glBindFramebuffer(GL_FRAMEBUFFER, 0 /* back buffer */);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        pState->ext.glDeleteFramebuffers(1, &pContext->idFramebuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        if (pContext->idReadFramebuffer != OPENGL_INVALID_ID)
        {
            pState->ext.glDeleteFramebuffers(1, &pContext->idReadFramebuffer);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }
        if (pContext->idDrawFramebuffer != OPENGL_INVALID_ID)
        {
            pState->ext.glDeleteFramebuffers(1, &pContext->idDrawFramebuffer);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }
    }
#ifdef RT_OS_WINDOWS
    wglMakeCurrent(pContext->hdc, NULL);
    wglDeleteContext(pContext->hglrc);
    ReleaseDC(pContext->hwnd, pContext->hdc);

    /* Destroy the window we've created. */
    int rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_DESTROYWINDOW, (WPARAM)pContext->hwnd, 0);
    AssertRC(rc);
#elif defined(RT_OS_DARWIN)
    vmsvga3dCocoaDestroyViewAndContext(pContext->cocoaView, pContext->cocoaContext);
#elif defined(RT_OS_LINUX)
    glXMakeCurrent(pState->display, None, NULL);
    glXDestroyContext(pState->display, pContext->glxContext);
    XDestroyWindow(pState->display, pContext->window);
#endif

    memset(pContext, 0, sizeof(*pContext));
    pContext->id = SVGA3D_INVALID_ID;

    VMSVGA3D_CLEAR_CURRENT_CONTEXT(pState);
    return VINF_SUCCESS;
}

/**
 * Destroy an existing 3d context
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   cid             Context id
 */
int vmsvga3dContextDestroy(PVGASTATE pThis, uint32_t cid)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_WRONG_ORDER);

    /*
     * Resolve the context and hand it to the common worker function.
     */
    if (   cid < pState->cContexts
        && pState->papContexts[cid]->id == cid)
        return vmsvga3dContextDestroyOgl(pThis, pState->papContexts[cid], cid);

    AssertReturn(cid < SVGA3D_MAX_CONTEXT_IDS, VERR_INVALID_PARAMETER);
    return VINF_SUCCESS;
}

/**
 * Worker for vmsvga3dChangeMode that resizes a context.
 *
 * @param   pThis               The VGA device instance data.
 * @param   pState              The VMSVGA3d state.
 * @param   pContext            The context.
 */
static void vmsvga3dChangeModeOneContext(PVGASTATE pThis, PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext)
{
#ifdef RT_OS_WINDOWS
    /* Resize the window. */
    CREATESTRUCT          cs;
    RT_ZERO(cs);
    cs.cx = pThis->svga.uWidth;
    cs.cy = pThis->svga.uHeight;
    int rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_RESIZEWINDOW, (WPARAM)pContext->hwnd, (LPARAM)&cs);
    AssertRC(rc);

#elif defined(RT_OS_DARWIN)
    RT_NOREF(pState);
    vmsvga3dCocoaViewSetSize(pContext->cocoaView, pThis->svga.uWidth, pThis->svga.uHeight);

#elif defined(RT_OS_LINUX)
    XWindowChanges wc;
    wc.width = pThis->svga.uWidth;
    wc.height = pThis->svga.uHeight;
    XConfigureWindow(pState->display, pContext->window, CWWidth | CWHeight, &wc);
#endif
}

/* Handle resize */
int vmsvga3dChangeMode(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    /* Resize the shared context too. */
    if (pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
        vmsvga3dChangeModeOneContext(pThis, pState, &pState->SharedCtx);

    /* Resize all active contexts. */
    for (uint32_t i = 0; i < pState->cContexts; i++)
    {
        PVMSVGA3DCONTEXT pContext = pState->papContexts[i];
        if (pContext->id != SVGA3D_INVALID_ID)
            vmsvga3dChangeModeOneContext(pThis, pState, pContext);
    }

    return VINF_SUCCESS;
}


int vmsvga3dSetTransform(PVGASTATE pThis, uint32_t cid, SVGA3dTransformType type, float matrix[16])
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    bool                  fModelViewChanged = false;

    Log(("vmsvga3dSetTransform cid=%x %s\n", cid, vmsvgaTransformToString(type)));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetTransform invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Save this matrix for vm state save/restore. */
    pContext->state.aTransformState[type].fValid = true;
    memcpy(pContext->state.aTransformState[type].matrix, matrix, sizeof(pContext->state.aTransformState[type].matrix));
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_TRANSFORM;

    Log(("Matrix [%d %d %d %d]\n", (int)(matrix[0] * 10.0), (int)(matrix[1] * 10.0), (int)(matrix[2] * 10.0), (int)(matrix[3] * 10.0)));
    Log(("       [%d %d %d %d]\n", (int)(matrix[4] * 10.0), (int)(matrix[5] * 10.0), (int)(matrix[6] * 10.0), (int)(matrix[7] * 10.0)));
    Log(("       [%d %d %d %d]\n", (int)(matrix[8] * 10.0), (int)(matrix[9] * 10.0), (int)(matrix[10] * 10.0), (int)(matrix[11] * 10.0)));
    Log(("       [%d %d %d %d]\n", (int)(matrix[12] * 10.0), (int)(matrix[13] * 10.0), (int)(matrix[14] * 10.0), (int)(matrix[15] * 10.0)));

    switch (type)
    {
    case SVGA3D_TRANSFORM_VIEW:
        /* View * World = Model View */
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(matrix);
        if (pContext->state.aTransformState[SVGA3D_TRANSFORM_WORLD].fValid)
            glMultMatrixf(pContext->state.aTransformState[SVGA3D_TRANSFORM_WORLD].matrix);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        fModelViewChanged = true;
        break;

    case SVGA3D_TRANSFORM_PROJECTION:
    {
        int rc = ShaderTransformProjection(pContext->state.RectViewPort.w, pContext->state.RectViewPort.h, matrix, false /* fPretransformed */);
        AssertRCReturn(rc, rc);
        break;
    }

    case SVGA3D_TRANSFORM_TEXTURE0:
        glMatrixMode(GL_TEXTURE);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        glLoadMatrixf(matrix);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        break;

    case SVGA3D_TRANSFORM_TEXTURE1:
    case SVGA3D_TRANSFORM_TEXTURE2:
    case SVGA3D_TRANSFORM_TEXTURE3:
    case SVGA3D_TRANSFORM_TEXTURE4:
    case SVGA3D_TRANSFORM_TEXTURE5:
    case SVGA3D_TRANSFORM_TEXTURE6:
    case SVGA3D_TRANSFORM_TEXTURE7:
        Log(("vmsvga3dSetTransform: unsupported SVGA3D_TRANSFORM_TEXTUREx transform!!\n"));
        return VERR_INVALID_PARAMETER;

    case SVGA3D_TRANSFORM_WORLD:
        /* View * World = Model View */
        glMatrixMode(GL_MODELVIEW);
        if (pContext->state.aTransformState[SVGA3D_TRANSFORM_VIEW].fValid)
            glLoadMatrixf(pContext->state.aTransformState[SVGA3D_TRANSFORM_VIEW].matrix);
        else
            glLoadIdentity();
        glMultMatrixf(matrix);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        fModelViewChanged = true;
        break;

    case SVGA3D_TRANSFORM_WORLD1:
    case SVGA3D_TRANSFORM_WORLD2:
    case SVGA3D_TRANSFORM_WORLD3:
        Log(("vmsvga3dSetTransform: unsupported SVGA3D_TRANSFORM_WORLDx transform!!\n"));
        return VERR_INVALID_PARAMETER;

    default:
        Log(("vmsvga3dSetTransform: unknown type!!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /* Apparently we need to reset the light and clip data after modifying the modelview matrix. */
    if (fModelViewChanged)
    {
        /* Reprogram the clip planes. */
        for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aClipPlane); j++)
        {
            if (pContext->state.aClipPlane[j].fValid == true)
                vmsvga3dSetClipPlane(pThis, cid, j, pContext->state.aClipPlane[j].plane);
        }

        /* Reprogram the light data. */
        for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aLightData); j++)
        {
            if (pContext->state.aLightData[j].fValidData == true)
                vmsvga3dSetLightData(pThis, cid, j, &pContext->state.aLightData[j].data);
        }
    }

    return VINF_SUCCESS;
}

int vmsvga3dSetZRange(PVGASTATE pThis, uint32_t cid, SVGA3dZRange zRange)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetZRange cid=%x min=%d max=%d\n", cid, (uint32_t)(zRange.min * 100.0), (uint32_t)(zRange.max * 100.0)));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetZRange invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    pContext->state.zRange = zRange;
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_ZRANGE;

    if (zRange.min < -1.0)
        zRange.min = -1.0;
    if (zRange.max > 1.0)
        zRange.max = 1.0;

    glDepthRange((GLdouble)zRange.min, (GLdouble)zRange.max);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    return VINF_SUCCESS;
}

/**
 * Convert SVGA blend op value to its OpenGL equivalent
 */
static GLenum vmsvga3dBlendOp2GL(uint32_t blendOp)
{
    switch (blendOp)
    {
    case SVGA3D_BLENDOP_ZERO:
        return GL_ZERO;
    case SVGA3D_BLENDOP_ONE:
        return GL_ONE;
    case SVGA3D_BLENDOP_SRCCOLOR:
        return GL_SRC_COLOR;
    case SVGA3D_BLENDOP_INVSRCCOLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case SVGA3D_BLENDOP_SRCALPHA:
        return GL_SRC_ALPHA;
    case SVGA3D_BLENDOP_INVSRCALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case SVGA3D_BLENDOP_DESTALPHA:
        return GL_DST_ALPHA;
    case SVGA3D_BLENDOP_INVDESTALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case SVGA3D_BLENDOP_DESTCOLOR:
        return GL_DST_COLOR;
    case SVGA3D_BLENDOP_INVDESTCOLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case SVGA3D_BLENDOP_SRCALPHASAT:
        return GL_SRC_ALPHA_SATURATE;
    case SVGA3D_BLENDOP_BLENDFACTOR:
        return GL_CONSTANT_ALPHA;       /** @todo correct?? */
    case SVGA3D_BLENDOP_INVBLENDFACTOR:
        return GL_ONE_MINUS_CONSTANT_ALPHA;       /** @todo correct?? */
    default:
        AssertFailed();
        return GL_ONE;
    }
}

static GLenum vmsvga3dBlendEquation2GL(uint32_t blendEq)
{
    switch (blendEq)
    {
    case SVGA3D_BLENDEQ_ADD:
        return GL_FUNC_ADD;
    case SVGA3D_BLENDEQ_SUBTRACT:
        return GL_FUNC_SUBTRACT;
    case SVGA3D_BLENDEQ_REVSUBTRACT:
        return GL_FUNC_REVERSE_SUBTRACT;
    case SVGA3D_BLENDEQ_MINIMUM:
        return GL_MIN;
    case SVGA3D_BLENDEQ_MAXIMUM:
        return GL_MAX;
    default:
        AssertMsgFailed(("blendEq=%d (%#x)\n", blendEq, blendEq));
        return GL_FUNC_ADD;
    }
}

static GLenum vmsvgaCmpFunc2GL(uint32_t cmpFunc)
{
    switch (cmpFunc)
    {
    case SVGA3D_CMP_NEVER:
        return GL_NEVER;
    case SVGA3D_CMP_LESS:
        return GL_LESS;
    case SVGA3D_CMP_EQUAL:
        return GL_EQUAL;
    case SVGA3D_CMP_LESSEQUAL:
        return GL_LEQUAL;
    case SVGA3D_CMP_GREATER:
        return GL_GREATER;
    case SVGA3D_CMP_NOTEQUAL:
        return GL_NOTEQUAL;
    case SVGA3D_CMP_GREATEREQUAL:
        return GL_GEQUAL;
    case SVGA3D_CMP_ALWAYS:
        return GL_ALWAYS;
    default:
        AssertFailed();
        return GL_LESS;
    }
}

static GLenum vmsvgaStencipOp2GL(uint32_t stencilOp)
{
    switch (stencilOp)
    {
    case SVGA3D_STENCILOP_KEEP:
        return GL_KEEP;
    case SVGA3D_STENCILOP_ZERO:
        return GL_ZERO;
    case SVGA3D_STENCILOP_REPLACE:
        return GL_REPLACE;
    case SVGA3D_STENCILOP_INCRSAT:
        return GL_INCR_WRAP;
    case SVGA3D_STENCILOP_DECRSAT:
        return GL_DECR_WRAP;
    case SVGA3D_STENCILOP_INVERT:
        return GL_INVERT;
    case SVGA3D_STENCILOP_INCR:
        return GL_INCR;
    case SVGA3D_STENCILOP_DECR:
        return GL_DECR;
    default:
        AssertFailed();
        return GL_KEEP;
    }
}

int vmsvga3dSetRenderState(PVGASTATE pThis, uint32_t cid, uint32_t cRenderStates, SVGA3dRenderState *pRenderState)
{
    uint32_t                    val = UINT32_MAX; /* Shut up MSC. */
    PVMSVGA3DCONTEXT            pContext;
    PVMSVGA3DSTATE              pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetRenderState cid=%x cRenderStates=%d\n", cid, cRenderStates));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetRenderState invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    for (unsigned i = 0; i < cRenderStates; i++)
    {
        GLenum enableCap = ~(GLenum)0;
        Log(("vmsvga3dSetRenderState: cid=%x state=%s (%d) val=%x\n", cid, vmsvga3dGetRenderStateName(pRenderState[i].state), pRenderState[i].state, pRenderState[i].uintValue));
        /* Save the render state for vm state saving. */
        if (pRenderState[i].state < SVGA3D_RS_MAX)
            pContext->state.aRenderState[pRenderState[i].state] = pRenderState[i];

        switch (pRenderState[i].state)
        {
        case SVGA3D_RS_ZENABLE:                /* SVGA3dBool */
            enableCap = GL_DEPTH_TEST;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_ZWRITEENABLE:           /* SVGA3dBool */
            glDepthMask(!!pRenderState[i].uintValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_ALPHATESTENABLE:        /* SVGA3dBool */
            enableCap = GL_ALPHA_TEST;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_DITHERENABLE:           /* SVGA3dBool */
            enableCap = GL_DITHER;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGENABLE:              /* SVGA3dBool */
            enableCap = GL_FOG;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SPECULARENABLE:         /* SVGA3dBool */
            Log(("vmsvga3dSetRenderState: WARNING: not applicable.\n"));
            break;

        case SVGA3D_RS_LIGHTINGENABLE:         /* SVGA3dBool */
            enableCap = GL_LIGHTING;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_NORMALIZENORMALS:       /* SVGA3dBool */
            /* not applicable */
            Log(("vmsvga3dSetRenderState: WARNING: not applicable.\n"));
            break;

        case SVGA3D_RS_POINTSPRITEENABLE:      /* SVGA3dBool */
            enableCap = GL_POINT_SPRITE_ARB;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_POINTSIZE:              /* float */
            /** @todo we need to apply scaling for point sizes below the min or above the max; see Wine) */
            if (pRenderState[i].floatValue < pState->caps.flPointSize[0])
                pRenderState[i].floatValue = pState->caps.flPointSize[0];
            if (pRenderState[i].floatValue > pState->caps.flPointSize[1])
                pRenderState[i].floatValue = pState->caps.flPointSize[1];

            glPointSize(pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            Log(("SVGA3D_RS_POINTSIZE: %d\n", (uint32_t) (pRenderState[i].floatValue * 100.0)));
            break;

        case SVGA3D_RS_POINTSIZEMIN:           /* float */
            pState->ext.glPointParameterf(GL_POINT_SIZE_MIN, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            Log(("SVGA3D_RS_POINTSIZEMIN: %d\n", (uint32_t) (pRenderState[i].floatValue * 100.0)));
            break;

        case SVGA3D_RS_POINTSIZEMAX:           /* float */
            pState->ext.glPointParameterf(GL_POINT_SIZE_MAX, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            Log(("SVGA3D_RS_POINTSIZEMAX: %d\n", (uint32_t) (pRenderState[i].floatValue * 100.0)));
            break;

        case SVGA3D_RS_POINTSCALEENABLE:       /* SVGA3dBool */
        case SVGA3D_RS_POINTSCALE_A:           /* float */
        case SVGA3D_RS_POINTSCALE_B:           /* float */
        case SVGA3D_RS_POINTSCALE_C:           /* float */
            Log(("vmsvga3dSetRenderState: WARNING: not applicable.\n"));
            break;

        case SVGA3D_RS_AMBIENT:                /* SVGA3dColor */
        {
            GLfloat color[4]; /* red, green, blue, alpha */

            vmsvgaColor2GLFloatArray(pRenderState[i].uintValue, &color[0], &color[1], &color[2], &color[3]);

            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_CLIPPLANEENABLE:        /* SVGA3dClipPlanes */
        {
            AssertCompile(SVGA3D_CLIPPLANE_MAX == (1 << 5));
            for (uint32_t j = 0; j <= 5; j++)
            {
                if (pRenderState[i].uintValue & RT_BIT(j))
                    glEnable(GL_CLIP_PLANE0 + j);
                else
                    glDisable(GL_CLIP_PLANE0 + j);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            }
            break;
        }

        case SVGA3D_RS_FOGCOLOR:               /* SVGA3dColor */
        {
            GLfloat color[4]; /* red, green, blue, alpha */

            vmsvgaColor2GLFloatArray(pRenderState[i].uintValue, &color[0], &color[1], &color[2], &color[3]);

            glFogfv(GL_FOG_COLOR, color);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_FOGSTART:               /* float */
            glFogf(GL_FOG_START, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_FOGEND:                 /* float */
            glFogf(GL_FOG_END, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_FOGDENSITY:             /* float */
            glFogf(GL_FOG_DENSITY, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_RANGEFOGENABLE:         /* SVGA3dBool */
            glFogi(GL_FOG_COORD_SRC, (pRenderState[i].uintValue) ? GL_FOG_COORD : GL_FRAGMENT_DEPTH);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_FOGMODE:                /* SVGA3dFogMode */
        {
            SVGA3dFogMode mode;
            mode.uintValue = pRenderState[i].uintValue;

            enableCap = GL_FOG_MODE;
            switch (mode.s.function)
            {
            case SVGA3D_FOGFUNC_EXP:
                val = GL_EXP;
                break;
            case SVGA3D_FOGFUNC_EXP2:
                val = GL_EXP2;
                break;
            case SVGA3D_FOGFUNC_LINEAR:
                val = GL_LINEAR;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected fog function %d\n", mode.s.function), VERR_INTERNAL_ERROR);
                break;
            }

            /** @todo how to switch between vertex and pixel fog modes??? */
            Assert(mode.s.type == SVGA3D_FOGTYPE_PIXEL);
#if 0
            /* The fog type determines the render state. */
            switch (mode.s.type)
            {
            case SVGA3D_FOGTYPE_VERTEX:
                renderState = D3DRS_FOGVERTEXMODE;
                break;
            case SVGA3D_FOGTYPE_PIXEL:
                renderState = D3DRS_FOGTABLEMODE;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected fog type %d\n", mode.s.type), VERR_INTERNAL_ERROR);
                break;
            }
#endif

            /* Set the fog base to depth or range. */
            switch (mode.s.base)
            {
            case SVGA3D_FOGBASE_DEPTHBASED:
                glFogi(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_FOGBASE_RANGEBASED:
                glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            default:
                /* ignore */
                AssertMsgFailed(("Unexpected fog base %d\n", mode.s.base));
                break;
            }
            break;
        }

        case SVGA3D_RS_FILLMODE:               /* SVGA3dFillMode */
        {
            SVGA3dFillMode mode;

            mode.uintValue = pRenderState[i].uintValue;

            switch (mode.s.mode)
            {
            case SVGA3D_FILLMODE_POINT:
                val = GL_POINT;
                break;
            case SVGA3D_FILLMODE_LINE:
                val = GL_LINE;
                break;
            case SVGA3D_FILLMODE_FILL:
                val = GL_FILL;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected fill mode %d\n", mode.s.mode), VERR_INTERNAL_ERROR);
                break;
            }
            /* @note only front and back faces */
            Assert(mode.s.face == SVGA3D_FACE_FRONT_BACK);
            glPolygonMode(GL_FRONT_AND_BACK, val);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_SHADEMODE:              /* SVGA3dShadeMode */
            switch (pRenderState[i].uintValue)
            {
            case SVGA3D_SHADEMODE_FLAT:
                val = GL_FLAT;
                break;

            case SVGA3D_SHADEMODE_SMOOTH:
                val = GL_SMOOTH;
                break;

            default:
                AssertMsgFailedReturn(("Unexpected shade mode %d\n", pRenderState[i].uintValue), VERR_INTERNAL_ERROR);
                break;
            }

            glShadeModel(val);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_LINEPATTERN:            /* SVGA3dLinePattern */
            /* No longer supported by d3d; mesagl comments suggest not all backends support it */
            /** @todo */
            Log(("WARNING: SVGA3D_RS_LINEPATTERN %x not supported!!\n", pRenderState[i].uintValue));
            /*
            renderState = D3DRS_LINEPATTERN;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_LINEAA:                 /* SVGA3dBool */
            enableCap = GL_LINE_SMOOTH;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_LINEWIDTH:              /* float */
            glLineWidth(pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_SEPARATEALPHABLENDENABLE: /* SVGA3dBool */
        {
            /* Refresh the blending state based on the new enable setting. */
            SVGA3dRenderState renderstate[2];

            renderstate[0].state     = SVGA3D_RS_SRCBLEND;
            renderstate[0].uintValue = pContext->state.aRenderState[SVGA3D_RS_SRCBLEND].uintValue;
            renderstate[1].state     = SVGA3D_RS_BLENDEQUATION;
            renderstate[1].uintValue = pContext->state.aRenderState[SVGA3D_RS_BLENDEQUATION].uintValue;

            int rc = vmsvga3dSetRenderState(pThis, cid, 2, renderstate);
            AssertRCReturn(rc, rc);

            if (pContext->state.aRenderState[SVGA3D_RS_BLENDENABLE].uintValue != 0)
                continue;   /* ignore if blend is already enabled */
        }  RT_FALL_THRU();

        case SVGA3D_RS_BLENDENABLE:            /* SVGA3dBool */
            enableCap = GL_BLEND;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SRCBLENDALPHA:          /* SVGA3dBlendOp */
        case SVGA3D_RS_DSTBLENDALPHA:          /* SVGA3dBlendOp */
        case SVGA3D_RS_SRCBLEND:               /* SVGA3dBlendOp */
        case SVGA3D_RS_DSTBLEND:               /* SVGA3dBlendOp */
        {
            GLint srcRGB, srcAlpha, dstRGB, dstAlpha;
            GLint blendop = vmsvga3dBlendOp2GL(pRenderState[i].uintValue);

            glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            switch (pRenderState[i].state)
            {
            case SVGA3D_RS_SRCBLEND:
                srcRGB = blendop;
                break;
            case SVGA3D_RS_DSTBLEND:
                dstRGB = blendop;
                break;
            case SVGA3D_RS_SRCBLENDALPHA:
                srcAlpha = blendop;
                break;
            case SVGA3D_RS_DSTBLENDALPHA:
                dstAlpha = blendop;
                break;
            default:
                /* not possible; shut up gcc */
                AssertFailed();
                break;
            }

            if (pContext->state.aRenderState[SVGA3D_RS_SEPARATEALPHABLENDENABLE].uintValue != 0)
                pState->ext.glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
            else
                glBlendFunc(srcRGB, dstRGB);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_BLENDEQUATIONALPHA:     /* SVGA3dBlendEquation */
        case SVGA3D_RS_BLENDEQUATION:          /* SVGA3dBlendEquation */
            if (pContext->state.aRenderState[SVGA3D_RS_SEPARATEALPHABLENDENABLE].uintValue != 0)
                pState->ext.glBlendEquationSeparate(vmsvga3dBlendEquation2GL(pContext->state.aRenderState[SVGA3D_RS_BLENDEQUATION].uintValue),
                                                    vmsvga3dBlendEquation2GL(pContext->state.aRenderState[SVGA3D_RS_BLENDEQUATIONALPHA].uintValue));
            else
            {
#if VBOX_VMSVGA3D_GL_HACK_LEVEL >= 0x102
                glBlendEquation(vmsvga3dBlendEquation2GL(pRenderState[i].uintValue));
#else
                pState->ext.glBlendEquation(vmsvga3dBlendEquation2GL(pRenderState[i].uintValue));
#endif
            }
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_BLENDCOLOR:             /* SVGA3dColor */
        {
            GLfloat red, green, blue, alpha;

            vmsvgaColor2GLFloatArray(pRenderState[i].uintValue, &red, &green, &blue, &alpha);

#if VBOX_VMSVGA3D_GL_HACK_LEVEL >= 0x102
            glBlendColor(red, green, blue, alpha);
#else
            pState->ext.glBlendColor(red, green, blue, alpha);
#endif
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_CULLMODE:               /* SVGA3dFace */
        {
            GLenum mode = GL_BACK;  /* default for OpenGL */

            switch (pRenderState[i].uintValue)
            {
            case SVGA3D_FACE_NONE:
                break;
            case SVGA3D_FACE_FRONT:
                mode = GL_FRONT;
                break;
            case SVGA3D_FACE_BACK:
                mode = GL_BACK;
                break;
            case SVGA3D_FACE_FRONT_BACK:
                mode = GL_FRONT_AND_BACK;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected cull mode %d\n", pRenderState[i].uintValue), VERR_INTERNAL_ERROR);
                break;
            }
            enableCap = GL_CULL_FACE;
            if (pRenderState[i].uintValue != SVGA3D_FACE_NONE)
            {
                glCullFace(mode);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                val = 1;
            }
            else
                val = 0;
            break;
        }

        case SVGA3D_RS_ZFUNC:                  /* SVGA3dCmpFunc */
            glDepthFunc(vmsvgaCmpFunc2GL(pRenderState[i].uintValue));
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_ALPHAFUNC:              /* SVGA3dCmpFunc */
        {
            GLclampf ref;

            glGetFloatv(GL_ALPHA_TEST_REF, &ref);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glAlphaFunc(vmsvgaCmpFunc2GL(pRenderState[i].uintValue), ref);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_ALPHAREF:               /* float (0.0 .. 1.0) */
        {
            GLint func;

            glGetIntegerv(GL_ALPHA_TEST_FUNC, &func);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glAlphaFunc(func, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_STENCILENABLE:          /* SVGA3dBool */
            enableCap = GL_STENCIL_TEST;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILFUNC:            /* SVGA3dCmpFunc */
        case SVGA3D_RS_STENCILREF:             /* uint32_t */
        case SVGA3D_RS_STENCILMASK:            /* uint32_t */
        {
            GLint func, ref;
            GLuint mask;

            glGetIntegerv(GL_STENCIL_FUNC, &func);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_VALUE_MASK, (GLint *)&mask);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_REF, &ref);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            switch (pRenderState[i].state)
            {
            case SVGA3D_RS_STENCILFUNC:            /* SVGA3dCmpFunc */
                func = vmsvgaCmpFunc2GL(pRenderState[i].uintValue);
                break;

            case SVGA3D_RS_STENCILREF:             /* uint32_t */
                ref = pRenderState[i].uintValue;
                break;

            case SVGA3D_RS_STENCILMASK:            /* uint32_t */
                mask = pRenderState[i].uintValue;
                break;

            default:
                /* not possible; shut up gcc */
                AssertFailed();
                break;
            }

            glStencilFunc(func, ref, mask);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_STENCILWRITEMASK:       /* uint32_t */
            glStencilMask(pRenderState[i].uintValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_STENCILFAIL:            /* SVGA3dStencilOp */
        case SVGA3D_RS_STENCILZFAIL:           /* SVGA3dStencilOp */
        case SVGA3D_RS_STENCILPASS:            /* SVGA3dStencilOp */
        {
            GLint sfail, dpfail, dppass;
            GLenum stencilop = vmsvgaStencipOp2GL(pRenderState[i].uintValue);

            glGetIntegerv(GL_STENCIL_FAIL, &sfail);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &dpfail);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &dppass);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            switch (pRenderState[i].state)
            {
            case SVGA3D_RS_STENCILFAIL:            /* SVGA3dStencilOp */
                sfail = stencilop;
                break;
            case SVGA3D_RS_STENCILZFAIL:           /* SVGA3dStencilOp */
                dpfail = stencilop;
                break;
            case SVGA3D_RS_STENCILPASS:            /* SVGA3dStencilOp */
                dppass = stencilop;
                break;
            default:
                /* not possible; shut up gcc */
                AssertFailed();
                break;
            }
            glStencilOp(sfail, dpfail, dppass);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_STENCILENABLE2SIDED:    /* SVGA3dBool */
            /* @note GL_EXT_stencil_two_side required! */
            if (pState->ext.fEXT_stencil_two_side)
            {
                enableCap = GL_STENCIL_TEST_TWO_SIDE_EXT;
                val = pRenderState[i].uintValue;
            }
            else
                Log(("vmsvga3dSetRenderState: WARNING unsupported SVGA3D_RS_STENCILENABLE2SIDED\n"));
            break;

        case SVGA3D_RS_CCWSTENCILFUNC:         /* SVGA3dCmpFunc */
        {
            /** @todo SVGA3D_RS_STENCILFAIL/ZFAIL/PASS for front & back faces
             *        SVGA3D_RS_CCWSTENCILFAIL/ZFAIL/PASS for back faces ??
             */
            GLint ref;
            GLuint mask;

            glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, (GLint *)&mask);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_BACK_REF, &ref);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            pState->ext.glStencilFuncSeparate(GL_BACK, vmsvgaCmpFunc2GL(pRenderState[i].uintValue), ref, mask);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_CCWSTENCILFAIL:         /* SVGA3dStencilOp */
        case SVGA3D_RS_CCWSTENCILZFAIL:        /* SVGA3dStencilOp */
        case SVGA3D_RS_CCWSTENCILPASS:         /* SVGA3dStencilOp */
        {
            /** @todo SVGA3D_RS_STENCILFAIL/ZFAIL/PASS for front & back faces
             *        SVGA3D_RS_CCWSTENCILFAIL/ZFAIL/PASS for back faces ??
             */
            GLint sfail, dpfail, dppass;
            GLenum stencilop = vmsvgaStencipOp2GL(pRenderState[i].uintValue);

            glGetIntegerv(GL_STENCIL_BACK_FAIL, &sfail);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &dpfail);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &dppass);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            switch (pRenderState[i].state)
            {
            case SVGA3D_RS_CCWSTENCILFAIL:         /* SVGA3dStencilOp */
                sfail = stencilop;
                break;
            case SVGA3D_RS_CCWSTENCILZFAIL:        /* SVGA3dStencilOp */
                dpfail = stencilop;
                break;
            case SVGA3D_RS_CCWSTENCILPASS:         /* SVGA3dStencilOp */
                dppass = stencilop;
                break;
            default:
                /* not possible; shut up gcc */
                AssertFailed();
                break;
            }
            pState->ext.glStencilOpSeparate(GL_BACK, sfail, dpfail, dppass);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_ZBIAS:                  /* float */
            /** @todo unknown meaning; depth bias is not identical
            renderState = D3DRS_DEPTHBIAS;
            val = pRenderState[i].uintValue;
            */
            Log(("vmsvga3dSetRenderState: WARNING unsupported SVGA3D_RS_ZBIAS\n"));
            break;

        case SVGA3D_RS_DEPTHBIAS:              /* float */
        {
            GLfloat factor;

            /** @todo not sure if the d3d & ogl definitions are identical. */

            /* Do not change the factor part. */
            glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            glPolygonOffset(factor, pRenderState[i].floatValue);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_SLOPESCALEDEPTHBIAS:    /* float */
        {
            GLfloat units;

            /** @todo not sure if the d3d & ogl definitions are identical. */

            /* Do not change the factor part. */
            glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            glPolygonOffset(pRenderState[i].floatValue, units);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_COLORWRITEENABLE:       /* SVGA3dColorMask */
        {
            GLboolean red, green, blue, alpha;
            SVGA3dColorMask mask;

            mask.uintValue = pRenderState[i].uintValue;

            red     = mask.s.red;
            green   = mask.s.green;
            blue    = mask.s.blue;
            alpha   = mask.s.alpha;

            glColorMask(red, green, blue, alpha);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_RS_COLORWRITEENABLE1:      /* SVGA3dColorMask to D3DCOLORWRITEENABLE_* */
        case SVGA3D_RS_COLORWRITEENABLE2:      /* SVGA3dColorMask to D3DCOLORWRITEENABLE_* */
        case SVGA3D_RS_COLORWRITEENABLE3:      /* SVGA3dColorMask to D3DCOLORWRITEENABLE_* */
            Log(("vmsvga3dSetRenderState: WARNING SVGA3D_RS_COLORWRITEENABLEx not supported!!\n"));
            break;

        case SVGA3D_RS_SCISSORTESTENABLE:      /* SVGA3dBool */
            enableCap = GL_SCISSOR_TEST;
            val = pRenderState[i].uintValue;
            break;

#if 0
        case SVGA3D_RS_DIFFUSEMATERIALSOURCE:  /* SVGA3dVertexMaterial */
            AssertCompile(D3DMCS_COLOR2 == SVGA3D_VERTEXMATERIAL_SPECULAR);
            renderState = D3DRS_DIFFUSEMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SPECULARMATERIALSOURCE: /* SVGA3dVertexMaterial */
            renderState = D3DRS_SPECULARMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_AMBIENTMATERIALSOURCE:  /* SVGA3dVertexMaterial */
            renderState = D3DRS_AMBIENTMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_EMISSIVEMATERIALSOURCE: /* SVGA3dVertexMaterial */
            renderState = D3DRS_EMISSIVEMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;
#endif

        case SVGA3D_RS_WRAP3:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP4:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP5:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP6:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP7:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP8:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP9:                  /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP10:                 /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP11:                 /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP12:                 /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP13:                 /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP14:                 /* SVGA3dWrapFlags */
        case SVGA3D_RS_WRAP15:                 /* SVGA3dWrapFlags */
            Log(("vmsvga3dSetRenderState: WARNING unsupported SVGA3D_WRAPx (x >= 3)\n"));
            break;

        case SVGA3D_RS_LASTPIXEL:              /* SVGA3dBool */
        case SVGA3D_RS_TWEENFACTOR:            /* float */
        case SVGA3D_RS_INDEXEDVERTEXBLENDENABLE: /* SVGA3dBool */
        case SVGA3D_RS_VERTEXBLEND:            /* SVGA3dVertexBlendFlags */
            Log(("vmsvga3dSetRenderState: WARNING not applicable!!\n"));
            break;

        case SVGA3D_RS_MULTISAMPLEANTIALIAS:   /* SVGA3dBool */
            enableCap = GL_MULTISAMPLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_MULTISAMPLEMASK:        /* uint32_t */
        case SVGA3D_RS_ANTIALIASEDLINEENABLE:  /* SVGA3dBool */
            Log(("vmsvga3dSetRenderState: WARNING not applicable??!!\n"));
            break;

        case SVGA3D_RS_COORDINATETYPE:         /* SVGA3dCoordinateType */
            Assert(pRenderState[i].uintValue == SVGA3D_COORDINATE_LEFTHANDED);
            /** @todo setup a view matrix to scale the world space by -1 in the z-direction for right handed coordinates. */
            /*
            renderState = D3DRS_COORDINATETYPE;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_FRONTWINDING:           /* SVGA3dFrontWinding */
            Assert(pRenderState[i].uintValue == SVGA3D_FRONTWINDING_CW);
            /* Invert the selected mode because of y-inversion (?) */
            glFrontFace((pRenderState[i].uintValue != SVGA3D_FRONTWINDING_CW) ? GL_CW : GL_CCW);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RS_OUTPUTGAMMA:            /* float */
            //AssertFailed();
            /*
            D3DRS_SRGBWRITEENABLE ??
            renderState = D3DRS_OUTPUTGAMMA;
            val = pRenderState[i].uintValue;
            */
            break;

#if 0

        case SVGA3D_RS_VERTEXMATERIALENABLE:   /* SVGA3dBool */
            //AssertFailed();
            renderState = D3DRS_INDEXEDVERTEXBLENDENABLE;       /* correct?? */
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_TEXTUREFACTOR:          /* SVGA3dColor */
            renderState = D3DRS_TEXTUREFACTOR;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_LOCALVIEWER:            /* SVGA3dBool */
            renderState = D3DRS_LOCALVIEWER;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_ZVISIBLE:               /* SVGA3dBool */
            AssertFailed();
            /*
            renderState = D3DRS_ZVISIBLE;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_CLIPPING:               /* SVGA3dBool */
            renderState = D3DRS_CLIPPING;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP0:                  /* SVGA3dWrapFlags */
            glTexParameter GL_TEXTURE_WRAP_S
            Assert(SVGA3D_WRAPCOORD_3 == D3DWRAPCOORD_3);
            renderState = D3DRS_WRAP0;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP1:                  /* SVGA3dWrapFlags */
            glTexParameter GL_TEXTURE_WRAP_T
            renderState = D3DRS_WRAP1;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP2:                  /* SVGA3dWrapFlags */
            glTexParameter GL_TEXTURE_WRAP_R
            renderState = D3DRS_WRAP2;
            val = pRenderState[i].uintValue;
            break;


        case SVGA3D_RS_SEPARATEALPHABLENDENABLE: /* SVGA3dBool */
            renderState = D3DRS_SEPARATEALPHABLENDENABLE;
            val = pRenderState[i].uintValue;
            break;


        case SVGA3D_RS_BLENDEQUATIONALPHA:     /* SVGA3dBlendEquation */
            renderState = D3DRS_BLENDOPALPHA;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_TRANSPARENCYANTIALIAS:  /* SVGA3dTransparencyAntialiasType */
            AssertFailed();
            /*
            renderState = D3DRS_TRANSPARENCYANTIALIAS;
            val = pRenderState[i].uintValue;
            */
            break;

#endif
        default:
            AssertFailed();
            break;
        }

        if (enableCap != ~(GLenum)0)
        {
            if (val)
                glEnable(enableCap);
            else
                glDisable(enableCap);
        }
    }

    return VINF_SUCCESS;
}

int vmsvga3dSetRenderTarget(PVGASTATE pThis, uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId target)
{
    PVMSVGA3DCONTEXT            pContext;
    PVMSVGA3DSTATE              pState = pThis->svga.p3dState;
    PVMSVGA3DSURFACE            pRenderTarget;

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(type < SVGA3D_RT_MAX, VERR_INVALID_PARAMETER);
    AssertReturn(target.face == 0, VERR_INVALID_PARAMETER);

    Log(("vmsvga3dSetRenderTarget cid=%x type=%x surface id=%x\n", cid, type, target.sid));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetRenderTarget invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Save for vm state save/restore. */
    pContext->state.aRenderTargets[type] = target.sid;

    if (target.sid == SVGA3D_INVALID_ID)
    {
        /* Disable render target. */
        switch (type)
        {
        case SVGA3D_RT_DEPTH:
        case SVGA3D_RT_STENCIL:
            pState->ext.glFramebufferRenderbuffer(GL_FRAMEBUFFER, (type == SVGA3D_RT_DEPTH) ? GL_DEPTH_ATTACHMENT : GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_RT_COLOR0:
        case SVGA3D_RT_COLOR1:
        case SVGA3D_RT_COLOR2:
        case SVGA3D_RT_COLOR3:
        case SVGA3D_RT_COLOR4:
        case SVGA3D_RT_COLOR5:
        case SVGA3D_RT_COLOR6:
        case SVGA3D_RT_COLOR7:
            pContext->sidRenderTarget = SVGA3D_INVALID_ID;
            pState->ext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + type - SVGA3D_RT_COLOR0, 0, 0, 0);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
        }
        return VINF_SUCCESS;
    }

    AssertReturn(target.sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
    AssertReturn(target.sid < pState->cSurfaces && pState->papSurfaces[target.sid]->id == target.sid, VERR_INVALID_PARAMETER);
    pRenderTarget = pState->papSurfaces[target.sid];

    switch (type)
    {
    case SVGA3D_RT_DEPTH:
    case SVGA3D_RT_STENCIL:
        AssertReturn(target.mipmap == 0, VERR_INVALID_PARAMETER);
        if (pRenderTarget->oglId.texture == OPENGL_INVALID_ID)
        {
            Log(("vmsvga3dSetRenderTarget: create renderbuffer to be used as render target; surface id=%x type=%d format=%d\n", target.sid, pRenderTarget->flags, pRenderTarget->internalFormatGL));
            pContext = &pState->SharedCtx;
            VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

            pState->ext.glGenRenderbuffers(1, &pRenderTarget->oglId.renderbuffer);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            pState->ext.glBindRenderbuffer(GL_RENDERBUFFER, pRenderTarget->oglId.renderbuffer);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            pState->ext.glRenderbufferStorage(GL_RENDERBUFFER,
                                              pRenderTarget->internalFormatGL,
                                              pRenderTarget->pMipmapLevels[0].size.width,
                                              pRenderTarget->pMipmapLevels[0].size.height);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            pState->ext.glBindRenderbuffer(GL_RENDERBUFFER, OPENGL_INVALID_ID);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

            pContext = pState->papContexts[cid];
            VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
            pRenderTarget->idWeakContextAssociation = cid;
        }

        pState->ext.glBindRenderbuffer(GL_RENDERBUFFER, pRenderTarget->oglId.renderbuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        Assert(!pRenderTarget->fDirty);
        AssertReturn(pRenderTarget->oglId.texture != OPENGL_INVALID_ID, VERR_INVALID_PARAMETER);

        pRenderTarget->flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;

        pState->ext.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                              (type == SVGA3D_RT_DEPTH) ? GL_DEPTH_ATTACHMENT : GL_STENCIL_ATTACHMENT,
                                              GL_RENDERBUFFER, pRenderTarget->oglId.renderbuffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        break;

    case SVGA3D_RT_COLOR0:
    case SVGA3D_RT_COLOR1:
    case SVGA3D_RT_COLOR2:
    case SVGA3D_RT_COLOR3:
    case SVGA3D_RT_COLOR4:
    case SVGA3D_RT_COLOR5:
    case SVGA3D_RT_COLOR6:
    case SVGA3D_RT_COLOR7:
    {
        /* A texture surface can be used as a render target to fill it and later on used as a texture. */
        if (pRenderTarget->oglId.texture == OPENGL_INVALID_ID)
        {
            Log(("vmsvga3dSetRenderTarget: create texture to be used as render target; surface id=%x type=%d format=%d -> create texture\n", target.sid, pRenderTarget->flags, pRenderTarget->format));
            int rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pRenderTarget);
            AssertRCReturn(rc, rc);
        }

        AssertReturn(pRenderTarget->oglId.texture != OPENGL_INVALID_ID, VERR_INVALID_PARAMETER);
        Assert(!pRenderTarget->fDirty);

        pRenderTarget->flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;

        pState->ext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + type - SVGA3D_RT_COLOR0, GL_TEXTURE_2D, pRenderTarget->oglId.texture, target.mipmap);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        pContext->sidRenderTarget = target.sid;

#ifdef DEBUG
        GLenum status = pState->ext.glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            Log(("vmsvga3dSetRenderTarget: WARNING: glCheckFramebufferStatus returned %x\n", status));
#endif
        /** @todo use glDrawBuffers too? */
        break;
    }

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    return VINF_SUCCESS;
}

#if 0
/**
 * Convert SVGA texture combiner value to its D3D equivalent
 */
static DWORD vmsvga3dTextureCombiner2D3D(uint32_t value)
{
    switch (value)
    {
    case SVGA3D_TC_DISABLE:
        return D3DTOP_DISABLE;
    case SVGA3D_TC_SELECTARG1:
        return D3DTOP_SELECTARG1;
    case SVGA3D_TC_SELECTARG2:
        return D3DTOP_SELECTARG2;
    case SVGA3D_TC_MODULATE:
        return D3DTOP_MODULATE;
    case SVGA3D_TC_ADD:
        return D3DTOP_ADD;
    case SVGA3D_TC_ADDSIGNED:
        return D3DTOP_ADDSIGNED;
    case SVGA3D_TC_SUBTRACT:
        return D3DTOP_SUBTRACT;
    case SVGA3D_TC_BLENDTEXTUREALPHA:
        return D3DTOP_BLENDTEXTUREALPHA;
    case SVGA3D_TC_BLENDDIFFUSEALPHA:
        return D3DTOP_BLENDDIFFUSEALPHA;
    case SVGA3D_TC_BLENDCURRENTALPHA:
        return D3DTOP_BLENDCURRENTALPHA;
    case SVGA3D_TC_BLENDFACTORALPHA:
        return D3DTOP_BLENDFACTORALPHA;
    case SVGA3D_TC_MODULATE2X:
        return D3DTOP_MODULATE2X;
    case SVGA3D_TC_MODULATE4X:
        return D3DTOP_MODULATE4X;
    case SVGA3D_TC_DSDT:
        AssertFailed(); /** @todo ??? */
        return D3DTOP_DISABLE;
    case SVGA3D_TC_DOTPRODUCT3:
        return D3DTOP_DOTPRODUCT3;
    case SVGA3D_TC_BLENDTEXTUREALPHAPM:
        return D3DTOP_BLENDTEXTUREALPHAPM;
    case SVGA3D_TC_ADDSIGNED2X:
        return D3DTOP_ADDSIGNED2X;
    case SVGA3D_TC_ADDSMOOTH:
        return D3DTOP_ADDSMOOTH;
    case SVGA3D_TC_PREMODULATE:
        return D3DTOP_PREMODULATE;
    case SVGA3D_TC_MODULATEALPHA_ADDCOLOR:
        return D3DTOP_MODULATEALPHA_ADDCOLOR;
    case SVGA3D_TC_MODULATECOLOR_ADDALPHA:
        return D3DTOP_MODULATECOLOR_ADDALPHA;
    case SVGA3D_TC_MODULATEINVALPHA_ADDCOLOR:
        return D3DTOP_MODULATEINVALPHA_ADDCOLOR;
    case SVGA3D_TC_MODULATEINVCOLOR_ADDALPHA:
        return D3DTOP_MODULATEINVCOLOR_ADDALPHA;
    case SVGA3D_TC_BUMPENVMAPLUMINANCE:
        return D3DTOP_BUMPENVMAPLUMINANCE;
    case SVGA3D_TC_MULTIPLYADD:
        return D3DTOP_MULTIPLYADD;
    case SVGA3D_TC_LERP:
        return D3DTOP_LERP;
    default:
        AssertFailed();
        return D3DTOP_DISABLE;
    }
}

/**
 * Convert SVGA texture arg data value to its D3D equivalent
 */
static DWORD vmsvga3dTextureArgData2D3D(uint32_t value)
{
    switch (value)
    {
    case SVGA3D_TA_CONSTANT:
        return D3DTA_CONSTANT;
    case SVGA3D_TA_PREVIOUS:
        return D3DTA_CURRENT;   /* current = previous */
    case SVGA3D_TA_DIFFUSE:
        return D3DTA_DIFFUSE;
    case SVGA3D_TA_TEXTURE:
        return D3DTA_TEXTURE;
    case SVGA3D_TA_SPECULAR:
        return D3DTA_SPECULAR;
    default:
        AssertFailed();
        return 0;
    }
}

/**
 * Convert SVGA texture transform flag value to its D3D equivalent
 */
static DWORD vmsvga3dTextTransformFlags2D3D(uint32_t value)
{
    switch (value)
    {
    case SVGA3D_TEX_TRANSFORM_OFF:
        return D3DTTFF_DISABLE;
    case SVGA3D_TEX_TRANSFORM_S:
        return D3DTTFF_COUNT1;      /** @todo correct? */
    case SVGA3D_TEX_TRANSFORM_T:
        return D3DTTFF_COUNT2;      /** @todo correct? */
    case SVGA3D_TEX_TRANSFORM_R:
        return D3DTTFF_COUNT3;      /** @todo correct? */
    case SVGA3D_TEX_TRANSFORM_Q:
        return D3DTTFF_COUNT4;      /** @todo correct? */
    case SVGA3D_TEX_PROJECTED:
        return D3DTTFF_PROJECTED;
    default:
        AssertFailed();
        return 0;
    }
}
#endif

static GLenum vmsvga3dTextureAddress2OGL(SVGA3dTextureAddress value)
{
    switch (value)
    {
    case SVGA3D_TEX_ADDRESS_WRAP:
        return GL_REPEAT;
    case SVGA3D_TEX_ADDRESS_MIRROR:
        return GL_MIRRORED_REPEAT;
    case SVGA3D_TEX_ADDRESS_CLAMP:
        return GL_CLAMP_TO_EDGE;
    case SVGA3D_TEX_ADDRESS_BORDER:
        return GL_CLAMP_TO_BORDER;
    case SVGA3D_TEX_ADDRESS_MIRRORONCE:
        AssertFailed();
        return GL_CLAMP_TO_EDGE_SGIS; /** @todo correct? */

    case SVGA3D_TEX_ADDRESS_EDGE:
    case SVGA3D_TEX_ADDRESS_INVALID:
    default:
        AssertFailed();
        return GL_REPEAT;   /* default */
    }
}

static GLenum vmsvga3dTextureFilter2OGL(SVGA3dTextureFilter value)
{
    switch (value)
    {
    case SVGA3D_TEX_FILTER_NONE:
    case SVGA3D_TEX_FILTER_LINEAR:
        return GL_LINEAR;
    case SVGA3D_TEX_FILTER_NEAREST:
        return GL_NEAREST;
    case SVGA3D_TEX_FILTER_ANISOTROPIC:
        /** @todo */
    case SVGA3D_TEX_FILTER_FLATCUBIC:       // Deprecated, not implemented
    case SVGA3D_TEX_FILTER_GAUSSIANCUBIC:   // Deprecated, not implemented
    case SVGA3D_TEX_FILTER_PYRAMIDALQUAD:   // Not currently implemented
    case SVGA3D_TEX_FILTER_GAUSSIANQUAD:    // Not currently implemented
    default:
        AssertFailed();
        return GL_LINEAR;   /* default */
    }
}

uint32_t vmsvga3dSVGA3dColor2RGBA(SVGA3dColor value)
{
    /* flip the red and blue bytes */
    uint8_t blue = value & 0xff;
    uint8_t red  = (value >> 16) & 0xff;
    return (value & 0xff00ff00) | red | (blue << 16);
}

int vmsvga3dSetTextureState(PVGASTATE pThis, uint32_t cid, uint32_t cTextureStates, SVGA3dTextureState *pTextureState)
{
    GLenum                      val = ~(GLenum)0; /* Shut up MSC. */
    GLenum                      currentStage = ~(GLenum)0;
    PVMSVGA3DCONTEXT            pContext;
    PVMSVGA3DSTATE              pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetTextureState %x cTextureState=%d\n", cid, cTextureStates));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetTextureState invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    for (unsigned i = 0; i < cTextureStates; i++)
    {
        GLenum textureType = ~(GLenum)0;
#if 0
        GLenum samplerType = ~(GLenum)0;
#endif

        Log(("vmsvga3dSetTextureState: cid=%x stage=%d type=%s (%x) val=%x\n", cid, pTextureState[i].stage, vmsvga3dTextureStateToString(pTextureState[i].name), pTextureState[i].name, pTextureState[i].value));
        /* Record the texture state for vm state saving. */
        if (    pTextureState[i].stage < SVGA3D_MAX_TEXTURE_STAGE
            &&  pTextureState[i].name < SVGA3D_TS_MAX)
        {
            pContext->state.aTextureState[pTextureState[i].stage][pTextureState[i].name] = pTextureState[i];
        }

        /* Active the right texture unit for subsequent texture state changes. */
        if (pTextureState[i].stage != currentStage || i == 0)
        {
            /** @todo Is this the appropriate limit for all kinds of textures?  It is the
             * size of aSidActiveTexture and for binding/unbinding we cannot exceed it. */
            if (pTextureState[i].stage < SVGA3D_MAX_TEXTURE_STAGE)
            {
                pState->ext.glActiveTexture(GL_TEXTURE0 + pTextureState[i].stage);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                currentStage = pTextureState[i].stage;
            }
            else
            {
                AssertMsgFailed(("pTextureState[%d].stage=%#x name=%#x\n", i, pTextureState[i].stage, pTextureState[i].name));
                continue;
            }
        }

        switch (pTextureState[i].name)
        {
        case SVGA3D_TS_BUMPENVMAT00:                /* float */
        case SVGA3D_TS_BUMPENVMAT01:                /* float */
        case SVGA3D_TS_BUMPENVMAT10:                /* float */
        case SVGA3D_TS_BUMPENVMAT11:                /* float */
        case SVGA3D_TS_BUMPENVLSCALE:               /* float */
        case SVGA3D_TS_BUMPENVLOFFSET:              /* float */
            Log(("vmsvga3dSetTextureState: bump mapping texture options not supported!!\n"));
            break;

        case SVGA3D_TS_COLOROP:                     /* SVGA3dTextureCombiner */
        case SVGA3D_TS_COLORARG0:                   /* SVGA3dTextureArgData */
        case SVGA3D_TS_COLORARG1:                   /* SVGA3dTextureArgData */
        case SVGA3D_TS_COLORARG2:                   /* SVGA3dTextureArgData */
        case SVGA3D_TS_ALPHAOP:                     /* SVGA3dTextureCombiner */
        case SVGA3D_TS_ALPHAARG0:                   /* SVGA3dTextureArgData */
        case SVGA3D_TS_ALPHAARG1:                   /* SVGA3dTextureArgData */
        case SVGA3D_TS_ALPHAARG2:                   /* SVGA3dTextureArgData */
            /** @todo not used by MesaGL */
            Log(("vmsvga3dSetTextureState: colorop/alphaop not yet supported!!\n"));
            break;
#if 0

        case SVGA3D_TS_TEXCOORDINDEX:               /* uint32_t */
            textureType = D3DTSS_TEXCOORDINDEX;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_TEXTURETRANSFORMFLAGS:       /* SVGA3dTexTransformFlags */
            textureType = D3DTSS_TEXTURETRANSFORMFLAGS;
            val = vmsvga3dTextTransformFlags2D3D(pTextureState[i].value);
            break;
#endif

        case SVGA3D_TS_BIND_TEXTURE:                /* SVGA3dSurfaceId */
            if (pTextureState[i].value == SVGA3D_INVALID_ID)
            {
                Log(("SVGA3D_TS_BIND_TEXTURE: stage %d, texture surface id=%x replacing=%x\n",
                     currentStage, pTextureState[i].value, pContext->aSidActiveTexture[currentStage]));

                pContext->aSidActiveTexture[currentStage] = SVGA3D_INVALID_ID;
                /* Unselect the currently associated texture. */
                glBindTexture(GL_TEXTURE_2D, 0);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                /* Necessary for the fixed pipeline. */
                glDisable(GL_TEXTURE_2D);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            }
            else
            {
                uint32_t sid = pTextureState[i].value;

                AssertReturn(sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
                AssertReturn(sid < pState->cSurfaces && pState->papSurfaces[sid]->id == sid, VERR_INVALID_PARAMETER);

                PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];

                Log(("SVGA3D_TS_BIND_TEXTURE: stage %d, texture surface id=%x (%d,%d) replacing=%x\n",
                     currentStage, pTextureState[i].value, pSurface->pMipmapLevels[0].size.width,
                     pSurface->pMipmapLevels[0].size.height, pContext->aSidActiveTexture[currentStage]));

                if (pSurface->oglId.texture == OPENGL_INVALID_ID)
                {
                    Log(("CreateTexture (%d,%d) level=%d\n", pSurface->pMipmapLevels[0].size.width, pSurface->pMipmapLevels[0].size.height, pSurface->faces[0].numMipLevels));
                    int rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pSurface);
                    AssertRCReturn(rc, rc);
                }

                glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                /* Necessary for the fixed pipeline. */
                glEnable(GL_TEXTURE_2D);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                if (pContext->aSidActiveTexture[currentStage] != sid)
                {
                    /* Recreate the texture state as glBindTexture resets them all (sigh). */
                    for (uint32_t iStage = 0; iStage < SVGA3D_MAX_TEXTURE_STAGE; iStage++)
                    {
                        for (uint32_t j = 0; j < SVGA3D_TS_MAX; j++)
                        {
                            SVGA3dTextureState *pTextureStateIter = &pContext->state.aTextureState[iStage][j];

                            if (    pTextureStateIter->name != SVGA3D_TS_INVALID
                                &&  pTextureStateIter->name != SVGA3D_TS_BIND_TEXTURE)
                                vmsvga3dSetTextureState(pThis, pContext->id, 1, pTextureStateIter);
                        }
                    }
                }
                pContext->aSidActiveTexture[currentStage] = sid;
            }
            /* Finished; continue with the next one. */
            continue;

        case SVGA3D_TS_ADDRESSW:                    /* SVGA3dTextureAddress */
            textureType = GL_TEXTURE_WRAP_R;    /* R = W */
            val = vmsvga3dTextureAddress2OGL((SVGA3dTextureAddress)pTextureState[i].value);
            break;

        case SVGA3D_TS_ADDRESSU:                    /* SVGA3dTextureAddress */
            textureType = GL_TEXTURE_WRAP_S;    /* S = U */
            val = vmsvga3dTextureAddress2OGL((SVGA3dTextureAddress)pTextureState[i].value);
            break;

        case SVGA3D_TS_ADDRESSV:                    /* SVGA3dTextureAddress */
            textureType = GL_TEXTURE_WRAP_T;    /* T = V */
            val = vmsvga3dTextureAddress2OGL((SVGA3dTextureAddress)pTextureState[i].value);
            break;

        case SVGA3D_TS_MIPFILTER:                   /* SVGA3dTextureFilter */
        case SVGA3D_TS_MINFILTER:                   /* SVGA3dTextureFilter */
        {
            uint32_t mipFilter = pContext->state.aTextureState[currentStage][SVGA3D_TS_MIPFILTER].value;
            uint32_t minFilter = pContext->state.aTextureState[currentStage][SVGA3D_TS_MINFILTER].value;

            /* If SVGA3D_TS_MIPFILTER is set to NONE, then use SVGA3D_TS_MIPFILTER, otherwise SVGA3D_TS_MIPFILTER enables mipmap minification. */
            textureType = GL_TEXTURE_MIN_FILTER;
            if (mipFilter != SVGA3D_TEX_FILTER_NONE)
            {
                if (minFilter == SVGA3D_TEX_FILTER_NEAREST)
                {
                    if (mipFilter == SVGA3D_TEX_FILTER_LINEAR)
                        val = GL_NEAREST_MIPMAP_LINEAR;
                    else
                        val = GL_NEAREST_MIPMAP_NEAREST;
                }
                else
                {
                    if (mipFilter == SVGA3D_TEX_FILTER_LINEAR)
                        val = GL_LINEAR_MIPMAP_LINEAR;
                    else
                        val = GL_LINEAR_MIPMAP_NEAREST;
                }
            }
            else
                val = vmsvga3dTextureFilter2OGL((SVGA3dTextureFilter)minFilter);
            break;
        }

        case SVGA3D_TS_MAGFILTER:                   /* SVGA3dTextureFilter */
            textureType = GL_TEXTURE_MAG_FILTER;
            val = vmsvga3dTextureFilter2OGL((SVGA3dTextureFilter)pTextureState[i].value);
            Assert(val == GL_NEAREST || val == GL_LINEAR);
            break;

        case SVGA3D_TS_BORDERCOLOR:                 /* SVGA3dColor */
        {
            GLfloat color[4]; /* red, green, blue, alpha */

            vmsvgaColor2GLFloatArray(pTextureState[i].value, &color[0], &color[1], &color[2], &color[3]);

            glTexParameterfv(GL_TEXTURE_2D /** @todo flexible type */, GL_TEXTURE_BORDER_COLOR, color);   /* Identical; default 0.0 identical too */
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;
        }

        case SVGA3D_TS_TEXTURE_LOD_BIAS:            /* float */
            glTexParameterf(GL_TEXTURE_2D /** @todo flexible type */, GL_TEXTURE_LOD_BIAS, pTextureState[i].value);   /* Identical; default 0.0 identical too */
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            break;

        case SVGA3D_TS_TEXTURE_MIPMAP_LEVEL:        /* uint32_t */
            textureType = GL_TEXTURE_BASE_LEVEL;
            val = pTextureState[i].value;
            break;

#if 0
        case SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL:   /* uint32_t */
            samplerType = D3DSAMP_MAXANISOTROPY;
            val = pTextureState[i].value;   /* Identical?? */
            break;

        case SVGA3D_TS_GAMMA:                       /* float */
            samplerType = D3DSAMP_SRGBTEXTURE;
            /* Boolean in D3D */
            if (pTextureState[i].floatValue == 1.0f)
                val = FALSE;
            else
                val = TRUE;
            break;
#endif
        /* Internal commands, that don't map directly to the SetTextureStageState API. */
        case SVGA3D_TS_TEXCOORDGEN:                 /* SVGA3dTextureCoordGen */
            AssertFailed();
            break;

        default:
            //AssertFailed();
            break;
        }

        if (textureType != ~(GLenum)0)
        {
            glTexParameteri(GL_TEXTURE_2D /** @todo flexible type */, textureType, val);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }
    }

    return VINF_SUCCESS;
}

int vmsvga3dSetMaterial(PVGASTATE pThis, uint32_t cid, SVGA3dFace face, SVGA3dMaterial *pMaterial)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    GLenum                oglFace;

    Log(("vmsvga3dSetMaterial cid=%x face %d\n", cid, face));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetMaterial invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    switch (face)
    {
    case SVGA3D_FACE_NONE:
    case SVGA3D_FACE_FRONT:
        oglFace = GL_FRONT;
        break;

    case SVGA3D_FACE_BACK:
        oglFace = GL_BACK;
        break;

    case SVGA3D_FACE_FRONT_BACK:
        oglFace = GL_FRONT_AND_BACK;
        break;

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    /* Save for vm state save/restore. */
    pContext->state.aMaterial[face].fValid = true;
    pContext->state.aMaterial[face].material = *pMaterial;
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_MATERIAL;

    glMaterialfv(oglFace, GL_DIFFUSE, pMaterial->diffuse);
    glMaterialfv(oglFace, GL_AMBIENT, pMaterial->ambient);
    glMaterialfv(oglFace, GL_SPECULAR, pMaterial->specular);
    glMaterialfv(oglFace, GL_EMISSION, pMaterial->emissive);
    glMaterialfv(oglFace, GL_SHININESS, &pMaterial->shininess);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    return VINF_SUCCESS;
}

/** @todo Move into separate library as we are using logic from Wine here. */
int vmsvga3dSetLightData(PVGASTATE pThis, uint32_t cid, uint32_t index, SVGA3dLightData *pData)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    float                 QuadAttenuation;

    Log(("vmsvga3dSetLightData cid=%x index=%d type=%d\n", cid, index, pData->type));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetLightData invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Store for vm state save/restore */
    if (index < SVGA3D_MAX_LIGHTS)
    {
        pContext->state.aLightData[index].fValidData = true;
        pContext->state.aLightData[index].data = *pData;
    }
    else
        AssertFailed();

    if (    pData->attenuation0 < 0.0f
        ||  pData->attenuation1 < 0.0f
        ||  pData->attenuation2 < 0.0f)
    {
        Log(("vmsvga3dSetLightData: invalid negative attenuation values!!\n"));
        return VINF_SUCCESS;    /* ignore; could crash the GL driver */
    }

    /* Light settings are affected by the model view in OpenGL, the View transform in direct3d */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(pContext->state.aTransformState[SVGA3D_TRANSFORM_VIEW].matrix);

    glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, pData->diffuse);
    glLightfv(GL_LIGHT0 + index, GL_SPECULAR, pData->specular);
    glLightfv(GL_LIGHT0 + index, GL_AMBIENT, pData->ambient);

    if (pData->range * pData->range >= FLT_MIN)
        QuadAttenuation = 1.4f / (pData->range * pData->range);
    else
        QuadAttenuation = 0.0f;

    switch (pData->type)
    {
    case SVGA3D_LIGHTTYPE_POINT:
    {
        GLfloat position[4];

        position[0] = pData->position[0];
        position[1] = pData->position[1];
        position[2] = pData->position[2];
        position[3] = 1.0f;

        glLightfv(GL_LIGHT0 + index, GL_POSITION, position);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, 180.0f);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        /* Attenuation - Are these right? guessing... */
        glLightf(GL_LIGHT0 + index, GL_CONSTANT_ATTENUATION, pData->attenuation0);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_LINEAR_ATTENUATION, pData->attenuation1);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_QUADRATIC_ATTENUATION, (QuadAttenuation < pData->attenuation2) ? pData->attenuation2 : QuadAttenuation);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        /** @todo range */
        break;
    }

    case SVGA3D_LIGHTTYPE_SPOT1:
    {
        GLfloat exponent;
        GLfloat position[4];
        const GLfloat pi = 4.0f * atanf(1.0f);

        position[0] = pData->position[0];
        position[1] = pData->position[1];
        position[2] = pData->position[2];
        position[3] = 1.0f;

        glLightfv(GL_LIGHT0 + index, GL_POSITION, position);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        position[0] = pData->direction[0];
        position[1] = pData->direction[1];
        position[2] = pData->direction[2];
        position[3] = 1.0f;

        glLightfv(GL_LIGHT0 + index, GL_SPOT_DIRECTION, position);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        /*
         * opengl-ish and d3d-ish spot lights use too different models for the
         * light "intensity" as a function of the angle towards the main light direction,
         * so we only can approximate very roughly.
         * however spot lights are rather rarely used in games (if ever used at all).
         * furthermore if still used, probably nobody pays attention to such details.
         */
        if (pData->falloff == 0)
        {
            /* Falloff = 0 is easy, because d3d's and opengl's spot light equations have the
             * falloff resp. exponent parameter as an exponent, so the spot light lighting
             * will always be 1.0 for both of them, and we don't have to care for the
             * rest of the rather complex calculation
             */
            exponent = 0.0f;
        }
        else
        {
            float rho = pData->theta + (pData->phi - pData->theta) / (2 * pData->falloff);
            if (rho < 0.0001f)
                rho = 0.0001f;
            exponent = -0.3f/log(cos(rho/2));
        }
        if (exponent > 128.0f)
            exponent = 128.0f;

        glLightf(GL_LIGHT0 + index, GL_SPOT_EXPONENT, exponent);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, pData->phi * 90.0 / pi);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        /* Attenuation - Are these right? guessing... */
        glLightf(GL_LIGHT0 + index, GL_CONSTANT_ATTENUATION, pData->attenuation0);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_LINEAR_ATTENUATION, pData->attenuation1);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_QUADRATIC_ATTENUATION, (QuadAttenuation < pData->attenuation2) ? pData->attenuation2 : QuadAttenuation);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        /** @todo range */
        break;
    }

    case SVGA3D_LIGHTTYPE_DIRECTIONAL:
    {
        GLfloat position[4];

        position[0] = -pData->direction[0];
        position[1] = -pData->direction[1];
        position[2] = -pData->direction[2];
        position[3] = 0.0f;

        glLightfv(GL_LIGHT0 + index, GL_POSITION, position); /* Note gl uses w position of 0 for direction! */
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, 180.0f);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

        glLightf(GL_LIGHT0 + index, GL_SPOT_EXPONENT, 0.0f);
        VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
        break;
    }

    case SVGA3D_LIGHTTYPE_SPOT2:
    default:
        Log(("Unsupported light type!!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /* Restore the modelview matrix */
    glPopMatrix();

    return VINF_SUCCESS;
}

int vmsvga3dSetLightEnabled(PVGASTATE pThis, uint32_t cid, uint32_t index, uint32_t enabled)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetLightEnabled cid=%x %d -> %d\n", cid, index, enabled));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetLightEnabled invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Store for vm state save/restore */
    if (index < SVGA3D_MAX_LIGHTS)
        pContext->state.aLightData[index].fEnabled = !!enabled;
    else
        AssertFailed();

    if (enabled)
    {
        /* Load the default settings if none have been set yet. */
        if (!pContext->state.aLightData[index].fValidData)
            vmsvga3dSetLightData(pThis, cid, index, (SVGA3dLightData *)&vmsvga3d_default_light);
        glEnable(GL_LIGHT0 + index);
    }
    else
        glDisable(GL_LIGHT0 + index);

    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    return VINF_SUCCESS;
}

int vmsvga3dSetViewPort(PVGASTATE pThis, uint32_t cid, SVGA3dRect *pRect)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetViewPort cid=%x (%d,%d)(%d,%d)\n", cid, pRect->x, pRect->y, pRect->w, pRect->h));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetViewPort invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Save for vm state save/restore. */
    pContext->state.RectViewPort = *pRect;
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_VIEWPORT;

    /** @todo y-inversion for partial viewport coordinates? */
    glViewport(pRect->x, pRect->y, pRect->w, pRect->h);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Reset the projection matrix as that relies on the viewport setting. */
    if (pContext->state.aTransformState[SVGA3D_TRANSFORM_PROJECTION].fValid == true)
    {
        vmsvga3dSetTransform(pThis, cid, SVGA3D_TRANSFORM_PROJECTION, pContext->state.aTransformState[SVGA3D_TRANSFORM_PROJECTION].matrix);
    }
    else
    {
        float matrix[16];

        /* identity matrix if no matrix set. */
        memset(matrix, 0, sizeof(matrix));
        matrix[0]  = 1.0;
        matrix[5]  = 1.0;
        matrix[10] = 1.0;
        matrix[15] = 1.0;
        vmsvga3dSetTransform(pThis, cid, SVGA3D_TRANSFORM_PROJECTION, matrix);
    }

    return VINF_SUCCESS;
}

int vmsvga3dSetClipPlane(PVGASTATE pThis, uint32_t cid,  uint32_t index, float plane[4])
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    double                oglPlane[4];

    Log(("vmsvga3dSetClipPlane cid=%x %d (%d,%d)(%d,%d)\n", cid, index, (unsigned)(plane[0] * 100.0), (unsigned)(plane[1] * 100.0), (unsigned)(plane[2] * 100.0), (unsigned)(plane[3] * 100.0)));
    AssertReturn(index < SVGA3D_CLIPPLANE_MAX, VERR_INVALID_PARAMETER);

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetClipPlane invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Store for vm state save/restore. */
    pContext->state.aClipPlane[index].fValid = true;
    memcpy(pContext->state.aClipPlane[index].plane, plane, sizeof(pContext->state.aClipPlane[index].plane));

    /** @todo clip plane affected by model view in OpenGL & view in D3D + vertex shader -> not transformed (see Wine; state.c clipplane) */
    oglPlane[0] = (double)plane[0];
    oglPlane[1] = (double)plane[1];
    oglPlane[2] = (double)plane[2];
    oglPlane[3] = (double)plane[3];

    glClipPlane(GL_CLIP_PLANE0 + index, oglPlane);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    return VINF_SUCCESS;
}

int vmsvga3dSetScissorRect(PVGASTATE pThis, uint32_t cid, SVGA3dRect *pRect)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetScissorRect cid=%x (%d,%d)(%d,%d)\n", cid, pRect->x, pRect->y, pRect->w, pRect->h));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetScissorRect invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Store for vm state save/restore. */
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_SCISSORRECT;
    pContext->state.RectScissor = *pRect;

    glScissor(pRect->x, pRect->y, pRect->w, pRect->h);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    return VINF_SUCCESS;
}

static void vmsvgaColor2GLFloatArray(uint32_t color, GLfloat *pRed, GLfloat *pGreen, GLfloat *pBlue, GLfloat *pAlpha)
{
    /* Convert byte color components to float (0-1.0) */
    *pAlpha = (GLfloat)(color >> 24) / 255.0;
    *pRed   = (GLfloat)((color >> 16) & 0xff) / 255.0;
    *pGreen = (GLfloat)((color >> 8) & 0xff) / 255.0;
    *pBlue  = (GLfloat)(color & 0xff) / 255.0;
}

int vmsvga3dCommandClear(PVGASTATE pThis, uint32_t cid, SVGA3dClearFlag clearFlag, uint32_t color, float depth, uint32_t stencil,
                         uint32_t cRects, SVGA3dRect *pRect)
{
    GLbitfield            mask = 0;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    GLboolean             fDepthWriteEnabled = GL_FALSE;

    Log(("vmsvga3dCommandClear cid=%x clearFlag=%x color=%x depth=%d stencil=%x cRects=%d\n", cid, clearFlag, color, (uint32_t)(depth * 100.0), stencil, cRects));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dCommandClear invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    if (clearFlag & SVGA3D_CLEAR_COLOR)
    {
        GLfloat red, green, blue, alpha;

        vmsvgaColor2GLFloatArray(color, &red, &green, &blue, &alpha);

        /* Set the color clear value. */
        glClearColor(red, green, blue, alpha);

        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (clearFlag & SVGA3D_CLEAR_STENCIL)
    {
        /** @todo possibly the same problem as with glDepthMask */
        glClearStencil(stencil);
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    if (clearFlag & SVGA3D_CLEAR_DEPTH)
    {
        glClearDepth((GLdouble)depth);
        mask |= GL_DEPTH_BUFFER_BIT;

        /* glClear will not clear the depth buffer if writing is disabled. */
        glGetBooleanv(GL_DEPTH_WRITEMASK, &fDepthWriteEnabled);
        if (fDepthWriteEnabled == GL_FALSE)
            glDepthMask(GL_TRUE);
    }

    if (cRects)
    {
        /* Save the current scissor test bit and scissor box. */
        glPushAttrib(GL_SCISSOR_BIT);
        glEnable(GL_SCISSOR_TEST);
        for (unsigned i=0; i < cRects; i++)
        {
            Log(("vmsvga3dCommandClear: rect %d (%d,%d)(%d,%d)\n", i, pRect[i].x, pRect[i].y, pRect[i].x + pRect[i].w, pRect[i].y + pRect[i].h));
            glScissor(pRect[i].x, pRect[i].y, pRect[i].w, pRect[i].h);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            glClear(mask);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }
        /* Restore the old scissor test bit and box */
        glPopAttrib();
    }
    else
    {
        glClear(mask);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    }

    /* Restore depth write state. */
    if (    (clearFlag & SVGA3D_CLEAR_DEPTH)
        && fDepthWriteEnabled == GL_FALSE)
        glDepthMask(GL_FALSE);

    return VINF_SUCCESS;
}

/* Convert VMWare vertex declaration to its OpenGL equivalent. */
int vmsvga3dVertexDecl2OGL(SVGA3dVertexArrayIdentity &identity, GLint &size, GLenum &type, GLboolean &normalized)
{
    normalized = GL_FALSE;
    switch (identity.type)
    {
    case SVGA3D_DECLTYPE_FLOAT1:
        size = 1;
        type = GL_FLOAT;
        break;
    case SVGA3D_DECLTYPE_FLOAT2:
        size = 2;
        type = GL_FLOAT;
        break;
    case SVGA3D_DECLTYPE_FLOAT3:
        size = 3;
        type = GL_FLOAT;
        break;
    case SVGA3D_DECLTYPE_FLOAT4:
        size = 4;
        type = GL_FLOAT;
        break;

    case SVGA3D_DECLTYPE_D3DCOLOR:
        size = GL_BGRA;                 /* @note requires GL_ARB_vertex_array_bgra */
        type = GL_UNSIGNED_BYTE;
        normalized = GL_TRUE;   /* glVertexAttribPointer fails otherwise */
        break;

    case SVGA3D_DECLTYPE_UBYTE4N:
        normalized = GL_TRUE;
        RT_FALL_THRU();
    case SVGA3D_DECLTYPE_UBYTE4:
        size = 4;
        type = GL_UNSIGNED_BYTE;
        break;

    case SVGA3D_DECLTYPE_SHORT2N:
        normalized = GL_TRUE;
        RT_FALL_THRU();
    case SVGA3D_DECLTYPE_SHORT2:
        size = 2;
        type = GL_SHORT;
        break;

    case SVGA3D_DECLTYPE_SHORT4N:
        normalized = GL_TRUE;
        RT_FALL_THRU();
    case SVGA3D_DECLTYPE_SHORT4:
        size = 4;
        type = GL_SHORT;
        break;

    case SVGA3D_DECLTYPE_USHORT4N:
        normalized = GL_TRUE;
        size = 4;
        type = GL_UNSIGNED_SHORT;
        break;

    case SVGA3D_DECLTYPE_USHORT2N:
        normalized = GL_TRUE;
        size = 2;
        type = GL_UNSIGNED_SHORT;
        break;

    case SVGA3D_DECLTYPE_UDEC3:
        size = 3;
        type = GL_UNSIGNED_INT_2_10_10_10_REV;    /** @todo correct? */
        break;

    case SVGA3D_DECLTYPE_DEC3N:
        normalized = true;
        size = 3;
        type = GL_INT_2_10_10_10_REV;    /** @todo correct? */
        break;

    case SVGA3D_DECLTYPE_FLOAT16_2:
        size = 2;
        type = GL_HALF_FLOAT;
        break;
    case SVGA3D_DECLTYPE_FLOAT16_4:
        size = 4;
        type = GL_HALF_FLOAT;
        break;
    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    //pVertexElement->Method      = identity.method;
    //pVertexElement->Usage       = identity.usage;

    return VINF_SUCCESS;
}

/* Convert VMWare primitive type to its OpenGL equivalent. */
/* Calculate the vertex count based on the primitive type and nr of primitives. */
int vmsvga3dPrimitiveType2OGL(SVGA3dPrimitiveType PrimitiveType, GLenum *pMode, uint32_t cPrimitiveCount, uint32_t *pcVertices)
{
    switch (PrimitiveType)
    {
    case SVGA3D_PRIMITIVE_TRIANGLELIST:
        *pMode      = GL_TRIANGLES;
        *pcVertices = cPrimitiveCount * 3;
        break;
    case SVGA3D_PRIMITIVE_POINTLIST:
        *pMode = GL_POINTS;
        *pcVertices = cPrimitiveCount;
        break;
    case SVGA3D_PRIMITIVE_LINELIST:
        *pMode = GL_LINES;
        *pcVertices = cPrimitiveCount * 2;
        break;
    case SVGA3D_PRIMITIVE_LINESTRIP:
        *pMode = GL_LINE_STRIP;
        *pcVertices = cPrimitiveCount + 1;
        break;
    case SVGA3D_PRIMITIVE_TRIANGLESTRIP:
        *pMode = GL_TRIANGLE_STRIP;
        *pcVertices = cPrimitiveCount + 2;
        break;
    case SVGA3D_PRIMITIVE_TRIANGLEFAN:
        *pMode = GL_TRIANGLE_FAN;
        *pcVertices = cPrimitiveCount + 2;
        break;
    default:
        return VERR_INVALID_PARAMETER;
    }
    return VINF_SUCCESS;
}

int vmsvga3dResetTransformMatrices(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext)
{
    int rc;

    /* Reset the view matrix (also takes the world matrix into account). */
    if (pContext->state.aTransformState[SVGA3D_TRANSFORM_VIEW].fValid == true)
    {
        rc = vmsvga3dSetTransform(pThis, pContext->id, SVGA3D_TRANSFORM_VIEW, pContext->state.aTransformState[SVGA3D_TRANSFORM_VIEW].matrix);
    }
    else
    {
        float matrix[16];

        /* identity matrix if no matrix set. */
        memset(matrix, 0, sizeof(matrix));
        matrix[0]  = 1.0;
        matrix[5]  = 1.0;
        matrix[10] = 1.0;
        matrix[15] = 1.0;
        rc = vmsvga3dSetTransform(pThis, pContext->id, SVGA3D_TRANSFORM_VIEW, matrix);
    }

    /* Reset the projection matrix. */
    if (pContext->state.aTransformState[SVGA3D_TRANSFORM_PROJECTION].fValid == true)
    {
        rc = vmsvga3dSetTransform(pThis, pContext->id, SVGA3D_TRANSFORM_PROJECTION, pContext->state.aTransformState[SVGA3D_TRANSFORM_PROJECTION].matrix);
    }
    else
    {
        float matrix[16];

        /* identity matrix if no matrix set. */
        memset(matrix, 0, sizeof(matrix));
        matrix[0]  = 1.0;
        matrix[5]  = 1.0;
        matrix[10] = 1.0;
        matrix[15] = 1.0;
        rc = vmsvga3dSetTransform(pThis, pContext->id, SVGA3D_TRANSFORM_PROJECTION, matrix);
    }
    AssertRC(rc);
    return rc;
}

int vmsvga3dDrawPrimitivesProcessVertexDecls(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext, uint32_t iVertexDeclBase, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    unsigned            sidVertex = pVertexDecl[0].array.surfaceId;
    PVMSVGA3DSURFACE    pVertexSurface;

    AssertReturn(sidVertex < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
    AssertReturn(sidVertex < pState->cSurfaces && pState->papSurfaces[sidVertex]->id == sidVertex, VERR_INVALID_PARAMETER);

    pVertexSurface = pState->papSurfaces[sidVertex];
    Log(("vmsvga3dDrawPrimitives: vertex surface %x\n", sidVertex));

    /* Create and/or bind the vertex buffer. */
    if (pVertexSurface->oglId.buffer == OPENGL_INVALID_ID)
    {
        Log(("vmsvga3dDrawPrimitives: create vertex buffer fDirty=%d size=%x bytes\n", pVertexSurface->fDirty, pVertexSurface->pMipmapLevels[0].cbSurface));
        PVMSVGA3DCONTEXT pSavedCtx = pContext;
        pContext = &pState->SharedCtx;
        VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

        pState->ext.glGenBuffers(1, &pVertexSurface->oglId.buffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        pState->ext.glBindBuffer(GL_ARRAY_BUFFER, pVertexSurface->oglId.buffer);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        Assert(pVertexSurface->fDirty);
        /** @todo rethink usage dynamic/static */
        pState->ext.glBufferData(GL_ARRAY_BUFFER, pVertexSurface->pMipmapLevels[0].cbSurface, pVertexSurface->pMipmapLevels[0].pSurfaceData, GL_DYNAMIC_DRAW);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        pVertexSurface->pMipmapLevels[0].fDirty = false;
        pVertexSurface->fDirty = false;

        pVertexSurface->flags |= SVGA3D_SURFACE_HINT_VERTEXBUFFER;

        pState->ext.glBindBuffer(GL_ARRAY_BUFFER, OPENGL_INVALID_ID);
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

        pContext = pSavedCtx;
        VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
    }

    Assert(pVertexSurface->fDirty == false);
    pState->ext.glBindBuffer(GL_ARRAY_BUFFER, pVertexSurface->oglId.buffer);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

    /* Setup the vertex declarations. */
    for (unsigned iVertex = 0; iVertex < numVertexDecls; iVertex++)
    {
        GLint size;
        GLenum type;
        GLboolean normalized;
        GLuint index = iVertexDeclBase + iVertex;

        Log(("vmsvga3dDrawPrimitives: array index %d type=%s (%d) method=%s (%d) usage=%s (%d) usageIndex=%d stride=%d offset=%d\n", index, vmsvgaDeclType2String(pVertexDecl[iVertex].identity.type), pVertexDecl[iVertex].identity.type, vmsvgaDeclMethod2String(pVertexDecl[iVertex].identity.method), pVertexDecl[iVertex].identity.method, vmsvgaDeclUsage2String(pVertexDecl[iVertex].identity.usage), pVertexDecl[iVertex].identity.usage, pVertexDecl[iVertex].identity.usageIndex, pVertexDecl[iVertex].array.stride, pVertexDecl[iVertex].array.offset));

        int rc = vmsvga3dVertexDecl2OGL(pVertexDecl[iVertex].identity, size, type, normalized);
        AssertRCReturn(rc, rc);

        if (pContext->state.shidVertex != SVGA_ID_INVALID)
        {
            /* Use numbered vertex arrays when shaders are active. */
            pState->ext.glEnableVertexAttribArray(index);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            pState->ext.glVertexAttribPointer(index, size, type, normalized, pVertexDecl[iVertex].array.stride,
                                              (const GLvoid *)(uintptr_t)pVertexDecl[iVertex].array.offset);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
            /** @todo case SVGA3D_DECLUSAGE_COLOR: color component order not identical!! test GL_BGRA!!  */
        }
        else
        {
            /* Use the predefined selection of vertex streams for the fixed pipeline. */
            switch (pVertexDecl[iVertex].identity.usage)
            {
            case SVGA3D_DECLUSAGE_POSITIONT:
            case SVGA3D_DECLUSAGE_POSITION:
            {
                glEnableClientState(GL_VERTEX_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                glVertexPointer(size, type, pVertexDecl[iVertex].array.stride,
                                (const GLvoid *)(uintptr_t)pVertexDecl[iVertex].array.offset);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            }
            case SVGA3D_DECLUSAGE_BLENDWEIGHT:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_BLENDINDICES:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_NORMAL:
                glEnableClientState(GL_NORMAL_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                glNormalPointer(type, pVertexDecl[iVertex].array.stride,
                                (const GLvoid *)(uintptr_t)pVertexDecl[iVertex].array.offset);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_PSIZE:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_TEXCOORD:
                /* Specify the affected texture unit. */
#if VBOX_VMSVGA3D_GL_HACK_LEVEL >= 0x103
                glClientActiveTexture(GL_TEXTURE0 + pVertexDecl[iVertex].identity.usageIndex);
#else
                pState->ext.glClientActiveTexture(GL_TEXTURE0 + pVertexDecl[iVertex].identity.usageIndex);
#endif
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                glTexCoordPointer(size, type, pVertexDecl[iVertex].array.stride,
                                  (const GLvoid *)(uintptr_t)pVertexDecl[iVertex].array.offset);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_TANGENT:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_BINORMAL:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_TESSFACTOR:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_COLOR:    /** @todo color component order not identical!! test GL_BGRA!! */
                glEnableClientState(GL_COLOR_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                glColorPointer(size, type, pVertexDecl[iVertex].array.stride,
                               (const GLvoid *)(uintptr_t)pVertexDecl[iVertex].array.offset);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_FOG:
                glEnableClientState(GL_FOG_COORD_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                pState->ext.glFogCoordPointer(type, pVertexDecl[iVertex].array.stride,
                                              (const GLvoid *)(uintptr_t)pVertexDecl[iVertex].array.offset);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_DEPTH:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_SAMPLE:
                AssertFailed();
                break;
            case SVGA3D_DECLUSAGE_MAX: AssertFailed(); break; /* shut up gcc */
            }
        }

#ifdef LOG_ENABLED
        if (pVertexDecl[iVertex].array.stride == 0)
            Log(("vmsvga3dDrawPrimitives: stride == 0! Can be valid\n"));
#endif
    }

    return VINF_SUCCESS;
}

int vmsvga3dDrawPrimitivesCleanupVertexDecls(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext, uint32_t iVertexDeclBase, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;

    /* Clean up the vertex declarations. */
    for (unsigned iVertex = 0; iVertex < numVertexDecls; iVertex++)
    {
        if (pVertexDecl[iVertex].identity.usage == SVGA3D_DECLUSAGE_POSITIONT)
        {
            /* Reset the transformation matrices in case of a switch back from pretransformed mode. */
            Log(("vmsvga3dDrawPrimitivesCleanupVertexDecls: reset world and projection matrices after transformation reset (pre-transformed -> transformed)\n"));
            vmsvga3dResetTransformMatrices(pThis, pContext);
        }

        if (pContext->state.shidVertex != SVGA_ID_INVALID)
        {
            /* Use numbered vertex arrays when shaders are active. */
            pState->ext.glDisableVertexAttribArray(iVertexDeclBase + iVertex);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }
        else
        {
            /* Use the predefined selection of vertex streams for the fixed pipeline. */
            switch (pVertexDecl[iVertex].identity.usage)
            {
            case SVGA3D_DECLUSAGE_POSITION:
            case SVGA3D_DECLUSAGE_POSITIONT:
                glDisableClientState(GL_VERTEX_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_BLENDWEIGHT:
                break;
            case SVGA3D_DECLUSAGE_BLENDINDICES:
                break;
            case SVGA3D_DECLUSAGE_NORMAL:
                glDisableClientState(GL_NORMAL_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_PSIZE:
                break;
            case SVGA3D_DECLUSAGE_TEXCOORD:
                /* Specify the affected texture unit. */
#if VBOX_VMSVGA3D_GL_HACK_LEVEL >= 0x103
                glClientActiveTexture(GL_TEXTURE0 + pVertexDecl[iVertex].identity.usageIndex);
#else
                pState->ext.glClientActiveTexture(GL_TEXTURE0 + pVertexDecl[iVertex].identity.usageIndex);
#endif
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_TANGENT:
                break;
            case SVGA3D_DECLUSAGE_BINORMAL:
                break;
            case SVGA3D_DECLUSAGE_TESSFACTOR:
                break;
            case SVGA3D_DECLUSAGE_COLOR:    /** @todo color component order not identical!! */
                glDisableClientState(GL_COLOR_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_FOG:
                glDisableClientState(GL_FOG_COORD_ARRAY);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                break;
            case SVGA3D_DECLUSAGE_DEPTH:
                break;
            case SVGA3D_DECLUSAGE_SAMPLE:
                break;
            case SVGA3D_DECLUSAGE_MAX: AssertFailed(); break; /* shut up gcc */
            }
        }
    }
    /* Unbind the vertex buffer after usage. */
    pState->ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    return VINF_SUCCESS;
}

int vmsvga3dDrawPrimitives(PVGASTATE pThis, uint32_t cid, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl,
                           uint32_t numRanges, SVGA3dPrimitiveRange *pRange, uint32_t cVertexDivisor,
                           SVGA3dVertexDivisor *pVertexDivisor)
{
    RT_NOREF(pVertexDivisor);
    PVMSVGA3DSTATE               pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_INTERNAL_ERROR);
    PVMSVGA3DCONTEXT             pContext;
    int                          rc = VERR_NOT_IMPLEMENTED;
    uint32_t                     iCurrentVertex;

    Log(("vmsvga3dDrawPrimitives cid=%x numVertexDecls=%d numRanges=%d, cVertexDivisor=%d\n", cid, numVertexDecls, numRanges, cVertexDivisor));

    AssertReturn(numVertexDecls && numVertexDecls <= SVGA3D_MAX_VERTEX_ARRAYS, VERR_INVALID_PARAMETER);
    AssertReturn(numRanges && numRanges <= SVGA3D_MAX_DRAW_PRIMITIVE_RANGES, VERR_INVALID_PARAMETER);
    AssertReturn(!cVertexDivisor || cVertexDivisor == numVertexDecls, VERR_INVALID_PARAMETER);
    /** @todo Non-zero cVertexDivisor */
    Assert(!cVertexDivisor);

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dDrawPrimitives invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    /* Check for pretransformed vertex declarations. */
    for (unsigned iVertex = 0; iVertex < numVertexDecls; iVertex++)
    {
        switch (pVertexDecl[iVertex].identity.usage)
        {
            case SVGA3D_DECLUSAGE_POSITIONT:
                Log(("ShaderSetPositionTransformed: (%d,%d)\n", pContext->state.RectViewPort.w, pContext->state.RectViewPort.h));
                RT_FALL_THRU();
            case SVGA3D_DECLUSAGE_POSITION:
                ShaderSetPositionTransformed(pContext->pShaderContext, pContext->state.RectViewPort.w,
                                             pContext->state.RectViewPort.h,
                                             pVertexDecl[iVertex].identity.usage == SVGA3D_DECLUSAGE_POSITIONT);
                break;
            default:  /* Shut up MSC. */ break;
        }
    }

    /* Flush any shader changes; after (!) checking the vertex declarations to deal with pre-transformed vertices. */
    if (pContext->pShaderContext)
    {
        uint32_t rtHeight = 0;

        if (pContext->sidRenderTarget != SVGA_ID_INVALID)
        {
            PVMSVGA3DSURFACE pRenderTarget = pState->papSurfaces[pContext->sidRenderTarget];
            rtHeight = pRenderTarget->pMipmapLevels[0].size.height;
        }

        ShaderUpdateState(pContext->pShaderContext, rtHeight);
    }

    /* Process all vertex declarations. Each vertex buffer is represented by one stream. */
    iCurrentVertex   = 0;
    while (iCurrentVertex < numVertexDecls)
    {
        uint32_t sidVertex = SVGA_ID_INVALID;
        uint32_t iVertex;

        for (iVertex = iCurrentVertex; iVertex < numVertexDecls; iVertex++)
        {
            if (    sidVertex != SVGA_ID_INVALID
                &&  pVertexDecl[iVertex].array.surfaceId != sidVertex
               )
                break;
            sidVertex = pVertexDecl[iVertex].array.surfaceId;
        }

        rc = vmsvga3dDrawPrimitivesProcessVertexDecls(pThis, pContext, iCurrentVertex, iVertex - iCurrentVertex, &pVertexDecl[iCurrentVertex]);
        AssertRCReturn(rc, rc);

        iCurrentVertex = iVertex;
    }

    /* Now draw the primitives. */
    for (unsigned iPrimitive = 0; iPrimitive < numRanges; iPrimitive++)
    {
        GLenum           modeDraw;
        unsigned         sidIndex  = pRange[iPrimitive].indexArray.surfaceId;
        PVMSVGA3DSURFACE pIndexSurface = NULL;
        unsigned         cVertices;

        Log(("Primitive %d: type %s\n", iPrimitive, vmsvga3dPrimitiveType2String(pRange[iPrimitive].primType)));
        rc = vmsvga3dPrimitiveType2OGL(pRange[iPrimitive].primType, &modeDraw, pRange[iPrimitive].primitiveCount, &cVertices);
        if (RT_FAILURE(rc))
        {
            AssertRC(rc);
            goto internal_error;
        }

        if (sidIndex != SVGA3D_INVALID_ID)
        {
            AssertMsg(pRange[iPrimitive].indexWidth == sizeof(uint32_t) || pRange[iPrimitive].indexWidth == sizeof(uint16_t), ("Unsupported primitive width %d\n", pRange[iPrimitive].indexWidth));

            if (    sidIndex >= SVGA3D_MAX_SURFACE_IDS
                ||  sidIndex >= pState->cSurfaces
                ||  pState->papSurfaces[sidIndex]->id != sidIndex)
            {
                Assert(sidIndex < SVGA3D_MAX_SURFACE_IDS);
                Assert(sidIndex < pState->cSurfaces && pState->papSurfaces[sidIndex]->id == sidIndex);
                rc = VERR_INVALID_PARAMETER;
                goto internal_error;
            }
            pIndexSurface = pState->papSurfaces[sidIndex];
            Log(("vmsvga3dDrawPrimitives: index surface %x\n", sidIndex));

            if (pIndexSurface->oglId.buffer == OPENGL_INVALID_ID)
            {
                Log(("vmsvga3dDrawPrimitives: create index buffer fDirty=%d size=%x bytes\n", pIndexSurface->fDirty, pIndexSurface->pMipmapLevels[0].cbSurface));
                pContext = &pState->SharedCtx;
                VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

                pState->ext.glGenBuffers(1, &pIndexSurface->oglId.buffer);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                pState->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndexSurface->oglId.buffer);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                Assert(pIndexSurface->fDirty);

                /** @todo rethink usage dynamic/static */
                pState->ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER, pIndexSurface->pMipmapLevels[0].cbSurface, pIndexSurface->pMipmapLevels[0].pSurfaceData, GL_DYNAMIC_DRAW);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                pIndexSurface->pMipmapLevels[0].fDirty = false;
                pIndexSurface->fDirty = false;

                pIndexSurface->flags |= SVGA3D_SURFACE_HINT_INDEXBUFFER;

                pState->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OPENGL_INVALID_ID);
                VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                pContext = pState->papContexts[cid];
                VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
            }
            Assert(pIndexSurface->fDirty == false);

            pState->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndexSurface->oglId.buffer);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }

        if (!pIndexSurface)
        {
            /* Render without an index buffer */
            Log(("DrawPrimitive %x cPrimitives=%d cVertices=%d index index bias=%d\n", modeDraw, pRange[iPrimitive].primitiveCount, cVertices, pRange[iPrimitive].indexBias));
            glDrawArrays(modeDraw, pRange[iPrimitive].indexBias, cVertices);
        }
        else
        {
            GLenum indexType;

            Assert(pRange[iPrimitive].indexBias >= 0);  /** @todo  indexBias */
            Assert(pRange[iPrimitive].indexWidth == pRange[iPrimitive].indexArray.stride);

            if (pRange[iPrimitive].indexWidth == sizeof(uint8_t))
            {
                indexType = GL_UNSIGNED_BYTE;
            }
            else
            if (pRange[iPrimitive].indexWidth == sizeof(uint16_t))
            {
                indexType = GL_UNSIGNED_SHORT;
            }
            else
            {
                Assert(pRange[iPrimitive].indexWidth == sizeof(uint32_t));
                indexType = GL_UNSIGNED_INT;
            }

            /* Render with an index buffer */
            Log(("DrawIndexedPrimitive %x cPrimitives=%d cVertices=%d hint.first=%d hint.last=%d index offset=%d primitivecount=%d index width=%d index bias=%d\n", modeDraw, pRange[iPrimitive].primitiveCount, cVertices, pVertexDecl[0].rangeHint.first,  pVertexDecl[0].rangeHint.last,  pRange[iPrimitive].indexArray.offset, pRange[iPrimitive].primitiveCount,  pRange[iPrimitive].indexWidth, pRange[iPrimitive].indexBias));
            if (pRange[iPrimitive].indexBias == 0)
                glDrawElements(modeDraw,
                               cVertices,
                               indexType,
                               (GLvoid *)(uintptr_t)pRange[iPrimitive].indexArray.offset);   /* byte offset in indices buffer */
            else
                pState->ext.glDrawElementsBaseVertex(modeDraw,
                                                     cVertices,
                                                     indexType,
                                                     (GLvoid *)(uintptr_t)pRange[iPrimitive].indexArray.offset, /* byte offset in indices buffer */
                                                     pRange[iPrimitive].indexBias);  /* basevertex */

            /* Unbind the index buffer after usage. */
            pState->ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
        }
        VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
    }

internal_error:

    /* Deactivate the vertex declarations. */
    iCurrentVertex   = 0;
    while (iCurrentVertex < numVertexDecls)
    {
        uint32_t sidVertex = SVGA_ID_INVALID;
        uint32_t iVertex;

        for (iVertex = iCurrentVertex; iVertex < numVertexDecls; iVertex++)
        {
            if (    sidVertex != SVGA_ID_INVALID
                &&  pVertexDecl[iVertex].array.surfaceId != sidVertex
               )
                break;
            sidVertex = pVertexDecl[iVertex].array.surfaceId;
        }

        rc = vmsvga3dDrawPrimitivesCleanupVertexDecls(pThis, pContext, iCurrentVertex, iVertex - iCurrentVertex, &pVertexDecl[iCurrentVertex]);
        AssertRCReturn(rc, rc);

        iCurrentVertex = iVertex;
    }
#ifdef DEBUG
    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->aSidActiveTexture); i++)
    {
        if (pContext->aSidActiveTexture[i] != SVGA3D_INVALID_ID)
        {
            GLint activeTexture = 0;
            GLint activeTextureUnit = 0;

            glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTextureUnit);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
            pState->ext.glActiveTexture(GL_TEXTURE0 + i);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

            glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeTexture);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
            pState->ext.glActiveTexture(activeTextureUnit);
            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

# if 0 /* Aren't we checking whether 'activeTexture' on texture unit 'i' matches what we expected?  This works if only one unit is active, but if both are it _will_ fail for one of them. */
            if (pContext->aSidActiveTexture[activeTextureUnit - GL_TEXTURE0] != SVGA3D_INVALID_ID)
            {
                PVMSVGA3DSURFACE pTexture;
                pTexture = pState->papSurfaces[pContext->aSidActiveTexture[activeTextureUnit - GL_TEXTURE0]];

                AssertMsg(pTexture->oglId.texture == (GLuint)activeTexture, ("%x vs %x unit %d - %d\n", pTexture->oglId.texture, activeTexture, i, activeTextureUnit - GL_TEXTURE0));
            }
# else
            PVMSVGA3DSURFACE pTexture = pState->papSurfaces[pContext->aSidActiveTexture[i]];
            AssertMsg(pTexture->id == pContext->aSidActiveTexture[i], ("%x vs %x\n", pTexture->id, pContext->aSidActiveTexture[i]));
            AssertMsg(pTexture->oglId.texture == (GLuint)activeTexture,
                      ("%x vs %x unit %d (active unit %d) sid=%x\n", pTexture->oglId.texture, activeTexture, i,
                       activeTextureUnit - GL_TEXTURE0, pContext->aSidActiveTexture[i]));
# endif
        }
    }
#endif

#ifdef DEBUG_GFX_WINDOW
    if (pContext->sidRenderTarget)
    {
        SVGA3dCopyRect rect;

        rect.srcx = rect.srcy = rect.x = rect.y = 0;
        rect.w = pContext->state.RectViewPort.w;
        rect.h = pContext->state.RectViewPort.h;
        vmsvga3dCommandPresent(pThis, pContext->sidRenderTarget, 0, NULL);
    }
#endif

    return rc;
}


int vmsvga3dShaderDefine(PVGASTATE pThis, uint32_t cid, uint32_t shid, SVGA3dShaderType type, uint32_t cbData, uint32_t *pShaderData)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSHADER       pShader;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    int                   rc;

    Log(("vmsvga3dShaderDefine cid=%x shid=%x type=%s cbData=%x\n", cid, shid, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", cbData));
    Log3(("shader code:\n%.*Rhxd\n", cbData, pShaderData));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderDefine invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    AssertReturn(shid < SVGA3D_MAX_SHADER_IDS, VERR_INVALID_PARAMETER);
    if (type == SVGA3D_SHADERTYPE_VS)
    {
        if (shid >= pContext->cVertexShaders)
        {
            void *pvNew = RTMemRealloc(pContext->paVertexShader, sizeof(VMSVGA3DSHADER) * (shid + 1));
            AssertReturn(pvNew, VERR_NO_MEMORY);
            pContext->paVertexShader = (PVMSVGA3DSHADER)pvNew;
            memset(&pContext->paVertexShader[pContext->cVertexShaders], 0, sizeof(VMSVGA3DSHADER) * (shid + 1 - pContext->cVertexShaders));
            for (uint32_t i = pContext->cVertexShaders; i < shid + 1; i++)
                pContext->paVertexShader[i].id = SVGA3D_INVALID_ID;
            pContext->cVertexShaders = shid + 1;
        }
        /* If one already exists with this id, then destroy it now. */
        if (pContext->paVertexShader[shid].id != SVGA3D_INVALID_ID)
            vmsvga3dShaderDestroy(pThis, cid, shid, pContext->paVertexShader[shid].type);

        pShader = &pContext->paVertexShader[shid];
    }
    else
    {
        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (shid >= pContext->cPixelShaders)
        {
            void *pvNew = RTMemRealloc(pContext->paPixelShader, sizeof(VMSVGA3DSHADER) * (shid + 1));
            AssertReturn(pvNew, VERR_NO_MEMORY);
            pContext->paPixelShader = (PVMSVGA3DSHADER)pvNew;
            memset(&pContext->paPixelShader[pContext->cPixelShaders], 0, sizeof(VMSVGA3DSHADER) * (shid + 1 - pContext->cPixelShaders));
            for (uint32_t i = pContext->cPixelShaders; i < shid + 1; i++)
                pContext->paPixelShader[i].id = SVGA3D_INVALID_ID;
            pContext->cPixelShaders = shid + 1;
        }
        /* If one already exists with this id, then destroy it now. */
        if (pContext->paPixelShader[shid].id != SVGA3D_INVALID_ID)
            vmsvga3dShaderDestroy(pThis, cid, shid, pContext->paPixelShader[shid].type);

        pShader = &pContext->paPixelShader[shid];
    }

    memset(pShader, 0, sizeof(*pShader));
    pShader->id     = shid;
    pShader->cid    = cid;
    pShader->type   = type;
    pShader->cbData = cbData;
    pShader->pShaderProgram = RTMemAllocZ(cbData);
    AssertReturn(pShader->pShaderProgram, VERR_NO_MEMORY);
    memcpy(pShader->pShaderProgram, pShaderData, cbData);

#ifdef DUMP_SHADER_DISASSEMBLY
    LPD3DXBUFFER pDisassembly;
    HRESULT hr = D3DXDisassembleShader((const DWORD *)pShaderData, FALSE, NULL, &pDisassembly);
    if (hr == D3D_OK)
    {
        Log(("Shader disassembly:\n%s\n", pDisassembly->GetBufferPointer()));
        pDisassembly->Release();
    }
#endif

    switch (type)
    {
    case SVGA3D_SHADERTYPE_VS:
        rc = ShaderCreateVertexShader(pContext->pShaderContext, (const uint32_t *)pShaderData, &pShader->u.pVertexShader);
        break;

    case SVGA3D_SHADERTYPE_PS:
        rc = ShaderCreatePixelShader(pContext->pShaderContext, (const uint32_t *)pShaderData, &pShader->u.pPixelShader);
        break;

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    if (rc != VINF_SUCCESS)
    {
        RTMemFree(pShader->pShaderProgram);
        memset(pShader, 0, sizeof(*pShader));
        pShader->id = SVGA3D_INVALID_ID;
    }

    return rc;
}

int vmsvga3dShaderDestroy(PVGASTATE pThis, uint32_t cid, uint32_t shid, SVGA3dShaderType type)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    PVMSVGA3DSHADER       pShader = NULL;
    int                   rc;

    Log(("vmsvga3dShaderDestroy cid=%x shid=%x type=%s\n", cid, shid, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL"));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderDestroy invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    if (type == SVGA3D_SHADERTYPE_VS)
    {
        if (    shid < pContext->cVertexShaders
            &&  pContext->paVertexShader[shid].id == shid)
        {
            pShader = &pContext->paVertexShader[shid];
            rc = ShaderDestroyVertexShader(pContext->pShaderContext, pShader->u.pVertexShader);
            AssertRC(rc);
        }
    }
    else
    {
        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (    shid < pContext->cPixelShaders
            &&  pContext->paPixelShader[shid].id == shid)
        {
            pShader = &pContext->paPixelShader[shid];
            rc = ShaderDestroyPixelShader(pContext->pShaderContext, pShader->u.pPixelShader);
            AssertRC(rc);
        }
    }

    if (pShader)
    {
        if (pShader->pShaderProgram)
            RTMemFree(pShader->pShaderProgram);
        memset(pShader, 0, sizeof(*pShader));
        pShader->id = SVGA3D_INVALID_ID;
    }
    else
        AssertFailedReturn(VERR_INVALID_PARAMETER);

    return VINF_SUCCESS;
}

int vmsvga3dShaderSet(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext, uint32_t cid, SVGA3dShaderType type, uint32_t shid)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    int                 rc;

    Log(("vmsvga3dShaderSet cid=%x type=%s shid=%d\n", cid, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", shid));

    if (   !pContext
        && cid < pState->cContexts
        && pState->papContexts[cid]->id == cid)
        pContext = pState->papContexts[cid];
    else if (!pContext)
    {
        AssertMsgFailed(("cid=%#x cContexts=%#x\n", cid, pState->cContexts));
        Log(("vmsvga3dShaderSet invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    if (type == SVGA3D_SHADERTYPE_VS)
    {
        /* Save for vm state save/restore. */
        pContext->state.shidVertex = shid;
        pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_VERTEXSHADER;

        if (    shid < pContext->cVertexShaders
            &&  pContext->paVertexShader[shid].id == shid)
        {
            PVMSVGA3DSHADER pShader = &pContext->paVertexShader[shid];
            Assert(type == pShader->type);

            rc = ShaderSetVertexShader(pContext->pShaderContext, pShader->u.pVertexShader);
            AssertRCReturn(rc, rc);
        }
        else
        if (shid == SVGA_ID_INVALID)
        {
            /* Unselect shader. */
            rc = ShaderSetVertexShader(pContext->pShaderContext, NULL);
            AssertRCReturn(rc, rc);
        }
        else
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    else
    {
        /* Save for vm state save/restore. */
        pContext->state.shidPixel = shid;
        pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_PIXELSHADER;

        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (    shid < pContext->cPixelShaders
            &&  pContext->paPixelShader[shid].id == shid)
        {
            PVMSVGA3DSHADER pShader = &pContext->paPixelShader[shid];
            Assert(type == pShader->type);

            rc = ShaderSetPixelShader(pContext->pShaderContext, pShader->u.pPixelShader);
            AssertRCReturn(rc, rc);
        }
        else
        if (shid == SVGA_ID_INVALID)
        {
            /* Unselect shader. */
            rc = ShaderSetPixelShader(pContext->pShaderContext, NULL);
            AssertRCReturn(rc, rc);
        }
        else
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    return VINF_SUCCESS;
}

int vmsvga3dShaderSetConst(PVGASTATE pThis, uint32_t cid, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype, uint32_t cRegisters, uint32_t *pValues)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    int                   rc;

    Log(("vmsvga3dShaderSetConst cid=%x reg=%x type=%s cregs=%d ctype=%x\n", cid, reg, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", cRegisters, ctype));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderSetConst invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

    for (uint32_t i = 0; i < cRegisters; i++)
    {
#ifdef LOG_ENABLED
        switch (ctype)
        {
            case SVGA3D_CONST_TYPE_FLOAT:
            {
                float *pValuesF = (float *)pValues;
                Log(("Constant %d: value=%d-%d-%d-%d\n", reg + i, (int)(pValuesF[i*4 + 0] * 100.0), (int)(pValuesF[i*4 + 1] * 100.0), (int)(pValuesF[i*4 + 2] * 100.0), (int)(pValuesF[i*4 + 3] * 100.0)));
                break;
            }

            case SVGA3D_CONST_TYPE_INT:
                Log(("Constant %d: value=%x-%x-%x-%x\n", reg + i, pValues[i*4 + 0], pValues[i*4 + 1], pValues[i*4 + 2], pValues[i*4 + 3]));
                break;

            case SVGA3D_CONST_TYPE_BOOL:
                Log(("Constant %d: value=%x-%x-%x-%x\n", reg + i, pValues[i*4 + 0], pValues[i*4 + 1], pValues[i*4 + 2], pValues[i*4 + 3]));
                break;
        }
#endif
        vmsvga3dSaveShaderConst(pContext, reg + i, type, ctype, pValues[i*4 + 0], pValues[i*4 + 1], pValues[i*4 + 2], pValues[i*4 + 3]);
    }

    switch (type)
    {
    case SVGA3D_SHADERTYPE_VS:
        switch (ctype)
        {
        case SVGA3D_CONST_TYPE_FLOAT:
            rc = ShaderSetVertexShaderConstantF(pContext->pShaderContext, reg, (const float *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_INT:
            rc = ShaderSetVertexShaderConstantI(pContext->pShaderContext, reg, (const int32_t *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_BOOL:
            rc = ShaderSetVertexShaderConstantB(pContext->pShaderContext, reg, (const uint8_t *)pValues, cRegisters);
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
        }
        AssertRCReturn(rc, rc);
        break;

    case SVGA3D_SHADERTYPE_PS:
        switch (ctype)
        {
        case SVGA3D_CONST_TYPE_FLOAT:
            rc = ShaderSetPixelShaderConstantF(pContext->pShaderContext, reg, (const float *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_INT:
            rc = ShaderSetPixelShaderConstantI(pContext->pShaderContext, reg, (const int32_t *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_BOOL:
            rc = ShaderSetPixelShaderConstantB(pContext->pShaderContext, reg, (const uint8_t *)pValues, cRegisters);
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
        }
        AssertRCReturn(rc, rc);
        break;

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    return VINF_SUCCESS;
}


int vmsvga3dQueryBegin(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type)
{
    RT_NOREF(pThis, cid, type);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

int vmsvga3dQueryEnd(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type, SVGAGuestPtr guestResult)
{
    RT_NOREF(pThis, cid, type, guestResult);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

int vmsvga3dQueryWait(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type, SVGAGuestPtr guestResult)
{
    RT_NOREF(pThis, cid, type, guestResult);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

