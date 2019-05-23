/* $Id: server_texture.c $ */
/** @file
 * VBox crOpenGL - teximage functions.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "chromium.h"
#include "cr_error.h"
#include "server_dispatch.h"
#include "server.h"
#include "cr_mem.h"

#define CR_NOTHING()

#define CR_CHECKPTR(name)                                   \
    if (!realptr)                                           \
    {                                                       \
        crWarning(#name " with NULL ptr, ignored!");        \
        return;                                             \
    }

#if !defined(CR_STATE_NO_TEXTURE_IMAGE_STORE)
# define CR_FIXPTR() (uintptr_t) realptr += (uintptr_t) cr_server.head_spu->dispatch_table.MapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_READ_ONLY_ARB)
#else
# define CR_FIXPTR()
#endif

#if defined(CR_ARB_pixel_buffer_object)
# define CR_CHECKBUFFER(name, checkptr)                     \
    if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))   \
    {                                                       \
        CR_FIXPTR();                                        \
    }                                                       \
    else                                                    \
    {                                                       \
        checkptr                                            \
    }
#else
# define CR_CHECKBUFFER(name, checkptr) checkptr
#endif

#if defined(CR_ARB_pixel_buffer_object) && !defined(CR_STATE_NO_TEXTURE_IMAGE_STORE)
# define CR_FINISHBUFFER()                                                                  \
    if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))                                   \
    {                                                                                       \
        if (!cr_server.head_spu->dispatch_table.UnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB)) \
        {                                                                                   \
            crWarning("UnmapBufferARB failed");                                             \
        }                                                                                   \
    }
#else
#define CR_FINISHBUFFER()
#endif

#define CR_FUNC_SUBIMAGE(name, def, call, ptrname)          \
void SERVER_DISPATCH_APIENTRY                               \
crServerDispatch##name def                                  \
{                                                           \
    const GLvoid *realptr = ptrname;                        \
    CR_CHECKBUFFER(name, CR_CHECKPTR(name))                 \
    crState##name call;                                     \
    CR_FINISHBUFFER()                                       \
    realptr = ptrname;                                      \
    cr_server.head_spu->dispatch_table.name call;           \
}

#define CR_FUNC_IMAGE(name, def, call, ptrname)             \
void SERVER_DISPATCH_APIENTRY                               \
crServerDispatch##name def                                  \
{                                                           \
    const GLvoid *realptr = ptrname;                        \
    CR_CHECKBUFFER(name, CR_NOTHING())                      \
    crState##name call;                                     \
    CR_FINISHBUFFER()                                       \
    realptr = ptrname;                                      \
    cr_server.head_spu->dispatch_table.name call;           \
}

#if defined(CR_ARB_texture_compression)
CR_FUNC_SUBIMAGE(CompressedTexSubImage1DARB,
    (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imagesize, const GLvoid * data),
    (target, level, xoffset, width, format, imagesize, realptr), data)

CR_FUNC_SUBIMAGE(CompressedTexSubImage2DARB,
    (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imagesize, const GLvoid * data),
    (target, level, xoffset, yoffset, width, height, format, imagesize, realptr), data)

CR_FUNC_SUBIMAGE(CompressedTexSubImage3DARB,
    (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imagesize, const GLvoid * data),
    (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imagesize, realptr), data)

CR_FUNC_IMAGE(CompressedTexImage1DARB,
    (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLsizei imagesize, const GLvoid * data),
    (target, level, internalFormat, width, border, imagesize, realptr), data)

CR_FUNC_IMAGE(CompressedTexImage2DARB,
    (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imagesize, const GLvoid * data),
    (target, level, internalFormat, width, height, border, imagesize, realptr), data)

CR_FUNC_IMAGE(CompressedTexImage3DARB,
    (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imagesize, const GLvoid * data),
    (target, level, internalFormat, width, height, depth, border, imagesize, realptr), data)
#endif

CR_FUNC_SUBIMAGE(TexSubImage1D,
    (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels),
    (target, level, xoffset, width, format, type, realptr), pixels)

CR_FUNC_SUBIMAGE(TexSubImage2D,
    (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels),
    (target, level, xoffset, yoffset, width, height, format, type, realptr), pixels)

CR_FUNC_SUBIMAGE(TexSubImage3D,
    (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels),
    (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, realptr), pixels)

CR_FUNC_IMAGE(TexImage1D,
    (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels),
    (target, level, internalFormat, width, border, format, type, realptr), pixels)

CR_FUNC_IMAGE(TexImage2D,
    (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels),
    (target, level, internalFormat, width, height, border, format, type, realptr), pixels)

CR_FUNC_IMAGE(TexImage3D,
    (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels),
    (target, level, internalFormat, width, height, depth, border, format, type, realptr), pixels)


void SERVER_DISPATCH_APIENTRY crServerDispatchTexEnvf( GLenum target, GLenum pname, GLfloat param )
{
    crStateTexEnvf( target, pname, param );
    if (GL_POINT_SPRITE != target && pname != GL_COORD_REPLACE)
        CR_GLERR_CHECK(cr_server.head_spu->dispatch_table.TexEnvf( target, pname, param ););
}

void SERVER_DISPATCH_APIENTRY crServerDispatchTexEnvfv( GLenum target, GLenum pname, const GLfloat * params )
{
    crStateTexEnvfv( target, pname, params );
    if (GL_POINT_SPRITE != target && pname != GL_COORD_REPLACE)
        CR_GLERR_CHECK(cr_server.head_spu->dispatch_table.TexEnvfv( target, pname, params ););
}

void SERVER_DISPATCH_APIENTRY crServerDispatchTexEnvi( GLenum target, GLenum pname, GLint param )
{
    crStateTexEnvi( target, pname, param );
    if (GL_POINT_SPRITE != target && pname != GL_COORD_REPLACE)
        CR_GLERR_CHECK(cr_server.head_spu->dispatch_table.TexEnvi( target, pname, param ););
}

void SERVER_DISPATCH_APIENTRY crServerDispatchTexEnviv( GLenum target, GLenum pname, const GLint * params )
{
    crStateTexEnviv( target, pname, params );
    if (GL_POINT_SPRITE != target && pname != GL_COORD_REPLACE)
        CR_GLERR_CHECK(cr_server.head_spu->dispatch_table.TexEnviv( target, pname, params ););
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetTexEnvfv( GLenum target, GLenum pname, GLfloat * params )
{
    GLfloat local_params[4];
    (void) params;
    if (GL_POINT_SPRITE != target && pname != GL_COORD_REPLACE)
        cr_server.head_spu->dispatch_table.GetTexEnvfv( target, pname, local_params );
    else
        crStateGetTexEnvfv( target, pname, local_params );

    crServerReturnValue( &(local_params[0]), crStateHlpComponentsCount(pname)*sizeof (GLfloat) );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetTexEnviv( GLenum target, GLenum pname, GLint * params )
{
    GLint local_params[4];
    (void) params;
    if (GL_POINT_SPRITE != target && pname != GL_COORD_REPLACE)
        cr_server.head_spu->dispatch_table.GetTexEnviv( target, pname, local_params );
    else
        crStateGetTexEnviv( target, pname, local_params );

    crServerReturnValue( &(local_params[0]), crStateHlpComponentsCount(pname)*sizeof (GLint) );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchBindTexture( GLenum target, GLuint texture )
{
    crStateBindTexture( target, texture );
    cr_server.head_spu->dispatch_table.BindTexture(target, crStateGetTextureHWID(texture));
}


void SERVER_DISPATCH_APIENTRY crServerDispatchDeleteTextures( GLsizei n, const GLuint *textures)
{
    GLuint *newTextures;
    GLint i;

    if (n >= UINT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchDeleteTextures: parameter 'n' is out of range");
        return;
    }

    newTextures = (GLuint *)crAlloc(n * sizeof(GLuint));

    if (!newTextures)
    {
        crError("crServerDispatchDeleteTextures: out of memory");
        return;
    }

    for (i = 0; i < n; i++)
    {
        newTextures[i] = crStateGetTextureHWID(textures[i]);
    }

//    for (i = 0; i < n; ++i)
//    {
//        crDebug("DeleteTexture: %d, pid %d, ctx %d", textures[i], (uint32_t)cr_server.curClient->pid, cr_server.currentCtxInfo->pContext->id);
//    }


    crStateDeleteTextures(n, textures);
    cr_server.head_spu->dispatch_table.DeleteTextures(n, newTextures);
    crFree(newTextures);
}


void SERVER_DISPATCH_APIENTRY crServerDispatchPrioritizeTextures( GLsizei n, const GLuint * textures, const GLclampf * priorities )
{
    GLuint *newTextures = (GLuint *) crAlloc(n * sizeof(GLuint));
    GLint i;

    if (!newTextures)
    {
        crError("crServerDispatchDeleteTextures: out of memory");
        return;
    }

    crStatePrioritizeTextures(n, textures, priorities);

    for (i = 0; i < n; i++)
    {
        newTextures[i] = crStateGetTextureHWID(textures[i]);
    }

    cr_server.head_spu->dispatch_table.PrioritizeTextures(n, newTextures, priorities);
    crFree(newTextures);
}


/** @todo will fail for textures loaded from snapshot */
GLboolean SERVER_DISPATCH_APIENTRY crServerDispatchIsTexture( GLuint texture )
{
    GLboolean retval;
    retval = cr_server.head_spu->dispatch_table.IsTexture(crStateGetTextureHWID(texture));
    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}


GLboolean SERVER_DISPATCH_APIENTRY
crServerDispatchAreTexturesResident(GLsizei n, const GLuint *textures,
                                    GLboolean *residences)
{
    GLboolean retval;
    GLsizei i;
    GLboolean *res = (GLboolean *) crAlloc(n * sizeof(GLboolean));
    GLuint *textures2 = (GLuint *) crAlloc(n * sizeof(GLuint));

    (void) residences;

    for (i = 0; i < n; i++)
    {
        textures2[i] = crStateGetTextureHWID(textures[i]);
    }
    retval = cr_server.head_spu->dispatch_table.AreTexturesResident(n, textures2, res);

    crFree(textures2);

    crServerReturnValue(res, n * sizeof(GLboolean));

    crFree(res);

    return retval; /* WILL PROBABLY BE IGNORED */
}

