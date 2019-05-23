/* $Id: VBoxGLSupportInfo.cpp $ */
/** @file
 * VBox Qt GUI - OpenGL support info used for 2D support detection.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

# ifdef RT_OS_WINDOWS
#  include <iprt/win/windows.h> /* QGLWidget drags in Windows.h; -Wall forces us to use wrapper. */
#  include <iprt/stdint.h>      /* QGLWidget drags in stdint.h; -Wall forces us to use wrapper. */
# endif
# include <QGLWidget>

# include <iprt/assert.h>
# include <iprt/log.h>
# include <iprt/env.h>
# include <iprt/param.h>
# include <iprt/path.h>
# include <iprt/process.h>
# include <iprt/string.h>
# include <iprt/time.h>
# include <iprt/thread.h>

# include <VBox/VBoxGL2D.h>
# include "VBoxFBOverlayCommon.h"
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

#include <iprt/err.h>

#include <QGLContext>


/*****************/

/* functions */

PFNVBOXVHWA_ACTIVE_TEXTURE vboxglActiveTexture = NULL;
PFNVBOXVHWA_MULTI_TEX_COORD2I vboxglMultiTexCoord2i = NULL;
PFNVBOXVHWA_MULTI_TEX_COORD2D vboxglMultiTexCoord2d = NULL;
PFNVBOXVHWA_MULTI_TEX_COORD2F vboxglMultiTexCoord2f = NULL;


PFNVBOXVHWA_CREATE_SHADER   vboxglCreateShader  = NULL;
PFNVBOXVHWA_SHADER_SOURCE   vboxglShaderSource  = NULL;
PFNVBOXVHWA_COMPILE_SHADER  vboxglCompileShader = NULL;
PFNVBOXVHWA_DELETE_SHADER   vboxglDeleteShader  = NULL;

PFNVBOXVHWA_CREATE_PROGRAM  vboxglCreateProgram = NULL;
PFNVBOXVHWA_ATTACH_SHADER   vboxglAttachShader  = NULL;
PFNVBOXVHWA_DETACH_SHADER   vboxglDetachShader  = NULL;
PFNVBOXVHWA_LINK_PROGRAM    vboxglLinkProgram   = NULL;
PFNVBOXVHWA_USE_PROGRAM     vboxglUseProgram    = NULL;
PFNVBOXVHWA_DELETE_PROGRAM  vboxglDeleteProgram = NULL;

PFNVBOXVHWA_IS_SHADER       vboxglIsShader      = NULL;
PFNVBOXVHWA_GET_SHADERIV    vboxglGetShaderiv   = NULL;
PFNVBOXVHWA_IS_PROGRAM      vboxglIsProgram     = NULL;
PFNVBOXVHWA_GET_PROGRAMIV   vboxglGetProgramiv  = NULL;
PFNVBOXVHWA_GET_ATTACHED_SHADERS vboxglGetAttachedShaders = NULL;
PFNVBOXVHWA_GET_SHADER_INFO_LOG  vboxglGetShaderInfoLog   = NULL;
PFNVBOXVHWA_GET_PROGRAM_INFO_LOG vboxglGetProgramInfoLog  = NULL;

PFNVBOXVHWA_GET_UNIFORM_LOCATION vboxglGetUniformLocation = NULL;

PFNVBOXVHWA_UNIFORM1F vboxglUniform1f = NULL;
PFNVBOXVHWA_UNIFORM2F vboxglUniform2f = NULL;
PFNVBOXVHWA_UNIFORM3F vboxglUniform3f = NULL;
PFNVBOXVHWA_UNIFORM4F vboxglUniform4f = NULL;

PFNVBOXVHWA_UNIFORM1I vboxglUniform1i = NULL;
PFNVBOXVHWA_UNIFORM2I vboxglUniform2i = NULL;
PFNVBOXVHWA_UNIFORM3I vboxglUniform3i = NULL;
PFNVBOXVHWA_UNIFORM4I vboxglUniform4i = NULL;

PFNVBOXVHWA_GEN_BUFFERS vboxglGenBuffers = NULL;
PFNVBOXVHWA_DELETE_BUFFERS vboxglDeleteBuffers = NULL;
PFNVBOXVHWA_BIND_BUFFER vboxglBindBuffer = NULL;
PFNVBOXVHWA_BUFFER_DATA vboxglBufferData = NULL;
PFNVBOXVHWA_MAP_BUFFER vboxglMapBuffer = NULL;
PFNVBOXVHWA_UNMAP_BUFFER vboxglUnmapBuffer = NULL;

PFNVBOXVHWA_IS_FRAMEBUFFER vboxglIsFramebuffer = NULL;
PFNVBOXVHWA_BIND_FRAMEBUFFER vboxglBindFramebuffer = NULL;
PFNVBOXVHWA_DELETE_FRAMEBUFFERS vboxglDeleteFramebuffers = NULL;
PFNVBOXVHWA_GEN_FRAMEBUFFERS vboxglGenFramebuffers = NULL;
PFNVBOXVHWA_CHECK_FRAMEBUFFER_STATUS vboxglCheckFramebufferStatus = NULL;
PFNVBOXVHWA_FRAMEBUFFER_TEXTURE1D vboxglFramebufferTexture1D = NULL;
PFNVBOXVHWA_FRAMEBUFFER_TEXTURE2D vboxglFramebufferTexture2D = NULL;
PFNVBOXVHWA_FRAMEBUFFER_TEXTURE3D vboxglFramebufferTexture3D = NULL;
PFNVBOXVHWA_GET_FRAMEBUFFER_ATTACHMENT_PARAMETRIV vboxglGetFramebufferAttachmentParameteriv = NULL;

#define VBOXVHWA_GETPROCADDRESS(_c, _t, _n) ((_t)(uintptr_t)(_c).getProcAddress(QString(_n)))

#define VBOXVHWA_PFNINIT_SAME(_c, _t, _v, _rc) \
    do { \
        if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_v)) == NULL) \
        { \
            VBOXQGLLOGREL(("ERROR: '%s' function not found\n", "gl"#_v));\
            AssertBreakpoint(); \
            if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_v"ARB")) == NULL) \
            { \
                VBOXQGLLOGREL(("ERROR: '%s' function not found\n", "gl"#_v"ARB"));\
                AssertBreakpoint(); \
                if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_v"EXT")) == NULL) \
                { \
                    VBOXQGLLOGREL(("ERROR: '%s' function not found\n", "gl"#_v"EXT"));\
                    AssertBreakpoint(); \
                    (_rc)++; \
                } \
            } \
        } \
    }while(0)

#define VBOXVHWA_PFNINIT(_c, _t, _v, _f,_rc) \
    do { \
        if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_f)) == NULL) \
        { \
            VBOXQGLLOGREL(("ERROR: '%s' function is not found\n", "gl"#_f));\
            AssertBreakpoint(); \
            (_rc)++; \
        } \
    }while(0)

#define VBOXVHWA_PFNINIT_OBJECT_ARB(_c, _t, _v, _rc) \
        do { \
            if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_v"ObjectARB")) == NULL) \
            { \
                VBOXQGLLOGREL(("ERROR: '%s' function is not found\n", "gl"#_v"ObjectARB"));\
                AssertBreakpoint(); \
                (_rc)++; \
            } \
        }while(0)

#define VBOXVHWA_PFNINIT_ARB(_c, _t, _v, _rc) \
        do { \
            if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_v"ARB")) == NULL) \
            { \
                VBOXQGLLOGREL(("ERROR: '%s' function is not found\n", "gl"#_v"ARB"));\
                AssertBreakpoint(); \
                (_rc)++; \
            } \
        }while(0)

#define VBOXVHWA_PFNINIT_EXT(_c, _t, _v, _rc) \
        do { \
            if((vboxgl##_v = VBOXVHWA_GETPROCADDRESS(_c, _t, "gl"#_v"EXT")) == NULL) \
            { \
                VBOXQGLLOGREL(("ERROR: '%s' function is not found\n", "gl"#_v"EXT"));\
                AssertBreakpoint(); \
                (_rc)++; \
            } \
        }while(0)

static int vboxVHWAGlParseSubver(const GLubyte * ver, const GLubyte ** pNext, bool bSpacePrefixAllowed)
{
    int val = 0;

    for(;;++ver)
    {
        if(*ver >= '0' && *ver <= '9')
        {
            if(!val)
            {
                if(*ver == '0')
                    continue;
            }
            else
            {
                val *= 10;
            }
            val += *ver - '0';
        }
        else if(*ver == '.')
        {
            *pNext = ver+1;
            break;
        }
        else if(*ver == '\0')
        {
            *pNext = NULL;
            break;
        }
        else if(*ver == ' ' || *ver == '\t' ||  *ver == 0x0d || *ver == 0x0a)
        {
            if(bSpacePrefixAllowed)
            {
                if(!val)
                {
                    continue;
                }
            }

            /* treat this as the end ov version string */
            *pNext = NULL;
            break;
        }
        else
        {
            Assert(0);
            val = -1;
            break;
        }
    }

    return val;
}

/* static */
int VBoxGLInfo::parseVersion(const GLubyte * ver)
{
    int iVer = vboxVHWAGlParseSubver(ver, &ver, true);
    if(iVer)
    {
        iVer <<= 16;
        if(ver)
        {
            int tmp = vboxVHWAGlParseSubver(ver, &ver, false);
            if(tmp >= 0)
            {
                iVer |= tmp << 8;
                if(ver)
                {
                    tmp = vboxVHWAGlParseSubver(ver, &ver, false);
                    if(tmp >= 0)
                    {
                        iVer |= tmp;
                    }
                    else
                    {
                        Assert(0);
                        iVer = -1;
                    }
                }
            }
            else
            {
                Assert(0);
                iVer = -1;
            }
        }
    }
    return iVer;
}

void VBoxGLInfo::init(const QGLContext * pContext)
{
    if(mInitialized)
        return;

    mInitialized = true;

    if (!QGLFormat::hasOpenGL())
    {
        VBOXQGLLOGREL (("no gl support available\n"));
        return;
    }

//    pContext->makeCurrent();

    const GLubyte * str;
    VBOXQGL_CHECKERR(
            str = glGetString(GL_VERSION);
            );

    if(str)
    {
        VBOXQGLLOGREL (("gl version string: 0%s\n", str));

        mGLVersion = parseVersion (str);
        Assert(mGLVersion > 0);
        if(mGLVersion < 0)
        {
            mGLVersion = 0;
        }
        else
        {
            VBOXQGLLOGREL (("gl version: 0x%x\n", mGLVersion));
            VBOXQGL_CHECKERR(
                    str = glGetString (GL_EXTENSIONS);
                    );

            VBOXQGLLOGREL (("gl extensions: %s\n", str));

            const char * pos = strstr((const char *)str, "GL_ARB_multitexture");
            m_GL_ARB_multitexture = pos != NULL;
            VBOXQGLLOGREL (("GL_ARB_multitexture: %d\n", m_GL_ARB_multitexture));

            pos = strstr((const char *)str, "GL_ARB_shader_objects");
            m_GL_ARB_shader_objects = pos != NULL;
            VBOXQGLLOGREL (("GL_ARB_shader_objects: %d\n", m_GL_ARB_shader_objects));

            pos = strstr((const char *)str, "GL_ARB_fragment_shader");
            m_GL_ARB_fragment_shader = pos != NULL;
            VBOXQGLLOGREL (("GL_ARB_fragment_shader: %d\n", m_GL_ARB_fragment_shader));

            pos = strstr((const char *)str, "GL_ARB_pixel_buffer_object");
            m_GL_ARB_pixel_buffer_object = pos != NULL;
            VBOXQGLLOGREL (("GL_ARB_pixel_buffer_object: %d\n", m_GL_ARB_pixel_buffer_object));

            pos = strstr((const char *)str, "GL_ARB_texture_rectangle");
            m_GL_ARB_texture_rectangle = pos != NULL;
            VBOXQGLLOGREL (("GL_ARB_texture_rectangle: %d\n", m_GL_ARB_texture_rectangle));

            pos = strstr((const char *)str, "GL_EXT_texture_rectangle");
            m_GL_EXT_texture_rectangle = pos != NULL;
            VBOXQGLLOGREL (("GL_EXT_texture_rectangle: %d\n", m_GL_EXT_texture_rectangle));

            pos = strstr((const char *)str, "GL_NV_texture_rectangle");
            m_GL_NV_texture_rectangle = pos != NULL;
            VBOXQGLLOGREL (("GL_NV_texture_rectangle: %d\n", m_GL_NV_texture_rectangle));

            pos = strstr((const char *)str, "GL_ARB_texture_non_power_of_two");
            m_GL_ARB_texture_non_power_of_two = pos != NULL;
            VBOXQGLLOGREL (("GL_ARB_texture_non_power_of_two: %d\n", m_GL_ARB_texture_non_power_of_two));

            pos = strstr((const char *)str, "GL_EXT_framebuffer_object");
            m_GL_EXT_framebuffer_object = pos != NULL;
            VBOXQGLLOGREL (("GL_EXT_framebuffer_object: %d\n", m_GL_EXT_framebuffer_object));


            initExtSupport(*pContext);
        }
    }
    else
    {
        VBOXQGLLOGREL (("failed to make the context current, treating as unsupported\n"));
    }
}

void VBoxGLInfo::initExtSupport(const QGLContext & context)
{
    int rc = 0;
    do
    {
        rc = 0;
        mMultiTexNumSupported = 1; /* default, 1 means not supported */
        if(mGLVersion >= 0x010201) /* ogl >= 1.2.1 */
        {
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_ACTIVE_TEXTURE, ActiveTexture, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_MULTI_TEX_COORD2I, MultiTexCoord2i, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_MULTI_TEX_COORD2D, MultiTexCoord2d, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_MULTI_TEX_COORD2F, MultiTexCoord2f, rc);
        }
        else if(m_GL_ARB_multitexture)
        {
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_ACTIVE_TEXTURE, ActiveTexture, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_MULTI_TEX_COORD2I, MultiTexCoord2i, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_MULTI_TEX_COORD2D, MultiTexCoord2d, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_MULTI_TEX_COORD2F, MultiTexCoord2f, rc);
        }
        else
        {
            break;
        }

        if(RT_FAILURE(rc))
            break;

        GLint maxCoords, maxUnits;
        glGetIntegerv(GL_MAX_TEXTURE_COORDS, &maxCoords);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);

        VBOXQGLLOGREL(("Max Tex Coords (%d), Img Units (%d)\n", maxCoords, maxUnits));
        /* take the minimum of those */
        if(maxUnits < maxCoords)
            maxCoords = maxUnits;
        if(maxUnits < 2)
        {
            VBOXQGLLOGREL(("Max Tex Coord or Img Units < 2 disabling MultiTex support\n"));
            break;
        }

        mMultiTexNumSupported = maxUnits;
    }while(0);


    do
    {
        rc = 0;
        mPBOSupported = false;

        if(m_GL_ARB_pixel_buffer_object)
        {
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_GEN_BUFFERS, GenBuffers, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_DELETE_BUFFERS, DeleteBuffers, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_BIND_BUFFER, BindBuffer, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_BUFFER_DATA, BufferData, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_MAP_BUFFER, MapBuffer, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNMAP_BUFFER, UnmapBuffer, rc);
        }
        else
        {
            break;
        }

        if(RT_FAILURE(rc))
            break;

        mPBOSupported = true;
    } while(0);

    do
    {
        rc = 0;
        mFragmentShaderSupported = false;

        if(mGLVersion >= 0x020000)  /* if ogl >= 2.0*/
        {
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_CREATE_SHADER, CreateShader, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_SHADER_SOURCE, ShaderSource, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_COMPILE_SHADER, CompileShader, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_DELETE_SHADER, DeleteShader, rc);

            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_CREATE_PROGRAM, CreateProgram, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_ATTACH_SHADER, AttachShader, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_DETACH_SHADER, DetachShader, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_LINK_PROGRAM, LinkProgram, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_USE_PROGRAM, UseProgram, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_DELETE_PROGRAM, DeleteProgram, rc);

            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_IS_SHADER, IsShader, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_GET_SHADERIV, GetShaderiv, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_IS_PROGRAM, IsProgram, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_GET_PROGRAMIV, GetProgramiv, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_GET_ATTACHED_SHADERS, GetAttachedShaders,  rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_GET_SHADER_INFO_LOG, GetShaderInfoLog, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_GET_PROGRAM_INFO_LOG, GetProgramInfoLog, rc);

            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_GET_UNIFORM_LOCATION, GetUniformLocation, rc);

            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM1F, Uniform1f, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM2F, Uniform2f, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM3F, Uniform3f, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM4F, Uniform4f, rc);

            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM1I, Uniform1i, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM2I, Uniform2i, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM3I, Uniform3i, rc);
            VBOXVHWA_PFNINIT_SAME(context, PFNVBOXVHWA_UNIFORM4I, Uniform4i, rc);
        }
        else if(m_GL_ARB_shader_objects && m_GL_ARB_fragment_shader)
        {
            VBOXVHWA_PFNINIT_OBJECT_ARB(context, PFNVBOXVHWA_CREATE_SHADER, CreateShader, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_SHADER_SOURCE, ShaderSource, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_COMPILE_SHADER, CompileShader, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_DELETE_SHADER, DeleteShader, DeleteObjectARB, rc);

            VBOXVHWA_PFNINIT_OBJECT_ARB(context, PFNVBOXVHWA_CREATE_PROGRAM, CreateProgram, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_ATTACH_SHADER, AttachShader, AttachObjectARB, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_DETACH_SHADER, DetachShader, DetachObjectARB, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_LINK_PROGRAM, LinkProgram, rc);
            VBOXVHWA_PFNINIT_OBJECT_ARB(context, PFNVBOXVHWA_USE_PROGRAM, UseProgram, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_DELETE_PROGRAM, DeleteProgram, DeleteObjectARB, rc);

        /// @todo    VBOXVHWA_PFNINIT(PFNVBOXVHWA_IS_SHADER, IsShader, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_GET_SHADERIV, GetShaderiv, GetObjectParameterivARB, rc);
        /// @todo    VBOXVHWA_PFNINIT(PFNVBOXVHWA_IS_PROGRAM, IsProgram, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_GET_PROGRAMIV, GetProgramiv, GetObjectParameterivARB, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_GET_ATTACHED_SHADERS, GetAttachedShaders, GetAttachedObjectsARB, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_GET_SHADER_INFO_LOG, GetShaderInfoLog, GetInfoLogARB, rc);
            VBOXVHWA_PFNINIT(context, PFNVBOXVHWA_GET_PROGRAM_INFO_LOG, GetProgramInfoLog, GetInfoLogARB, rc);

            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_GET_UNIFORM_LOCATION, GetUniformLocation, rc);

            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM1F, Uniform1f, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM2F, Uniform2f, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM3F, Uniform3f, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM4F, Uniform4f, rc);

            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM1I, Uniform1i, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM2I, Uniform2i, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM3I, Uniform3i, rc);
            VBOXVHWA_PFNINIT_ARB(context, PFNVBOXVHWA_UNIFORM4I, Uniform4i, rc);
        }
        else
        {
            break;
        }

        if(RT_FAILURE(rc))
            break;

        mFragmentShaderSupported = true;
    } while(0);

    do
    {
        rc = 0;
        mFBOSupported = false;

        if(m_GL_EXT_framebuffer_object)
        {
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_IS_FRAMEBUFFER, IsFramebuffer, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_BIND_FRAMEBUFFER, BindFramebuffer, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_DELETE_FRAMEBUFFERS, DeleteFramebuffers, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_GEN_FRAMEBUFFERS, GenFramebuffers, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_CHECK_FRAMEBUFFER_STATUS, CheckFramebufferStatus, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_FRAMEBUFFER_TEXTURE1D, FramebufferTexture1D, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_FRAMEBUFFER_TEXTURE2D, FramebufferTexture2D, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_FRAMEBUFFER_TEXTURE3D, FramebufferTexture3D, rc);
            VBOXVHWA_PFNINIT_EXT(context, PFNVBOXVHWA_GET_FRAMEBUFFER_ATTACHMENT_PARAMETRIV, GetFramebufferAttachmentParameteriv, rc);
        }
        else
        {
            break;
        }

        if(RT_FAILURE(rc))
            break;

        mFBOSupported = true;
    } while(0);

    if(m_GL_ARB_texture_rectangle || m_GL_EXT_texture_rectangle || m_GL_NV_texture_rectangle)
    {
        mTextureRectangleSupported = true;
    }
    else
    {
        mTextureRectangleSupported = false;
    }

    mTextureNP2Supported = m_GL_ARB_texture_non_power_of_two;
}

void VBoxVHWAInfo::init(const QGLContext * pContext)
{
    if(mInitialized)
        return;

    mInitialized = true;

    mglInfo.init(pContext);

    if(mglInfo.isFragmentShaderSupported() && mglInfo.isTextureRectangleSupported())
    {
        uint32_t num = 0;
        mFourccSupportedList[num++] = FOURCC_AYUV;
        mFourccSupportedList[num++] = FOURCC_UYVY;
        mFourccSupportedList[num++] = FOURCC_YUY2;
        if(mglInfo.getMultiTexNumSupported() >= 4)
        {
            /* YV12 currently requires 3 units (for each color component)
             * + 1 unit for dst texture for color-keying + 3 units for each color component
             * TODO: we could store YV12 data in one texture to eliminate this requirement*/
            mFourccSupportedList[num++] = FOURCC_YV12;
        }

        Assert(num <= VBOXVHWA_NUMFOURCC);
        mFourccSupportedCount = num;
    }
    else
    {
        mFourccSupportedCount = 0;
    }
}

bool VBoxVHWAInfo::isVHWASupported() const
{
    if(mglInfo.getGLVersion() <= 0)
    {
        /* error occurred while gl info initialization */
        VBOXQGLLOGREL(("2D not supported: gl version info not initialized properly\n"));
        return false;
    }

#ifndef DEBUGVHWASTRICT
    /* in case we do not support shaders & multitexturing we can not support dst colorkey,
     * no sense to report Video Acceleration supported */
    if(!mglInfo.isFragmentShaderSupported())
    {
        VBOXQGLLOGREL(("2D not supported: fragment shader unsupported\n"));
        return false;
    }
#endif
    if(mglInfo.getMultiTexNumSupported() < 2)
    {
        VBOXQGLLOGREL(("2D not supported: multitexture unsupported\n"));
        return false;
    }

    /* color conversion now supported only GL_TEXTURE_RECTANGLE
     * in this case only stretching is accelerated
     * report as unsupported, TODO: probably should report as supported for stretch acceleration */
    if(!mglInfo.isTextureRectangleSupported())
    {
        VBOXQGLLOGREL(("2D not supported: texture rectangle unsupported\n"));
        return false;
    }

    VBOXQGLLOGREL(("2D is supported!\n"));
    return true;
}

/* static */
bool VBoxVHWAInfo::checkVHWASupport()
{
#if defined(RT_OS_WINDOWS) || defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
    static char pszVBoxPath[RTPATH_MAX];
    const char *papszArgs[] = { NULL, "-test", "2D", NULL};
    int rc;
    RTPROCESS Process;
    RTPROCSTATUS ProcStatus;
    uint64_t StartTS;

    rc = RTPathExecDir(pszVBoxPath, RTPATH_MAX); AssertRCReturn(rc, false);
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    rc = RTPathAppend(pszVBoxPath, RTPATH_MAX, "VBoxTestOGL.exe");
#else
    rc = RTPathAppend(pszVBoxPath, RTPATH_MAX, "VBoxTestOGL");
#endif
    papszArgs[0] = pszVBoxPath;         /* argv[0] */
    AssertRCReturn(rc, false);

    rc = RTProcCreate(pszVBoxPath, papszArgs, RTENV_DEFAULT, 0, &Process);
    if (RT_FAILURE(rc))
    {
        VBOXQGLLOGREL(("2D support test failed: failed to create a test process\n"));
        return false;
    }

    StartTS = RTTimeMilliTS();

    while (1)
    {
        rc = RTProcWait(Process, RTPROCWAIT_FLAGS_NOBLOCK, &ProcStatus);
        if (rc != VERR_PROCESS_RUNNING)
            break;

        if (RTTimeMilliTS() - StartTS > 30*1000 /* 30 sec */)
        {
            RTProcTerminate(Process);
            RTThreadSleep(100);
            RTProcWait(Process, RTPROCWAIT_FLAGS_NOBLOCK, &ProcStatus);
            VBOXQGLLOGREL(("2D support test failed: the test did not complete within 30 sec\n"));
            return false;
        }
        RTThreadSleep(100);
    }

    if (RT_SUCCESS(rc))
    {
        if ((ProcStatus.enmReason==RTPROCEXITREASON_NORMAL) && (ProcStatus.iStatus==0))
        {
            VBOXQGLLOGREL(("2D support test succeeded\n"));
            return true;
        }
    }

    VBOXQGLLOGREL(("2D support test failed: err code (%Rra)\n", rc));

    return false;
#else
    /** @todo test & enable external app approach*/
    VBoxGLTmpContext ctx;
    const QGLContext *pContext = ctx.makeCurrent();
    Assert(pContext);
    if(pContext)
    {
        VBoxVHWAInfo info;
        info.init(pContext);
        return info.isVHWASupported();
    }
    return false;
#endif
}

VBoxGLTmpContext::VBoxGLTmpContext()
{
    if(QGLFormat::hasOpenGL())
    {
        mWidget = new QGLWidget();
    }
    else
    {
        mWidget = NULL;
    }
}

VBoxGLTmpContext::~VBoxGLTmpContext()
{
    if(mWidget)
        delete mWidget;
}

const class QGLContext * VBoxGLTmpContext::makeCurrent()
{
    if(mWidget)
    {
        mWidget->makeCurrent();
        return mWidget->context();
    }
    return NULL;
}

