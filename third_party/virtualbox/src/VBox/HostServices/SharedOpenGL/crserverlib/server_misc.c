/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "server_dispatch.h"
#include "server.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_pixeldata.h"
#ifdef VBOX_WITH_CRDUMPER
# include "cr_dump.h"
#endif

void SERVER_DISPATCH_APIENTRY crServerDispatchSelectBuffer( GLsizei size, GLuint *buffer )
{
    (void) size;
    (void) buffer;
    crError( "Unsupported network glSelectBuffer call." );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetChromiumParametervCR(GLenum target, GLuint index, GLenum type, GLsizei count, GLvoid *values)
{
    GLubyte local_storage[4096];
    GLint bytes = 0;
    GLint cbType = 1; /* One byte by default. */

    memset(local_storage, 0, sizeof(local_storage));

    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
         cbType = sizeof(GLbyte);
         break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
         cbType = sizeof(GLshort);
         break;
    case GL_INT:
    case GL_UNSIGNED_INT:
         cbType = sizeof(GLint);
         break;
    case GL_FLOAT:
         cbType = sizeof(GLfloat);
         break;
    case GL_DOUBLE:
         cbType = sizeof(GLdouble);
         break;
    default:
         crError("Bad type in crServerDispatchGetChromiumParametervCR");
    }

    if (count < 0) /* 'GLsizei' is usually an 'int'. */
        count = 0;
    else if ((size_t)count > sizeof(local_storage) / cbType)
        count = sizeof(local_storage) / cbType;

    bytes = count * cbType;

    CRASSERT(bytes >= 0);
    CRASSERT(bytes < 4096);

    switch (target)
    {
        case GL_DBG_CHECK_BREAK_CR:
        {
            if (bytes > 0)
            {
                GLubyte *pbRc = local_storage;
                GLuint *puRc = (GLuint *)(bytes >=4 ? local_storage : NULL);
                int rc;
                memset(local_storage, 0, bytes);
                if (cr_server.RcToGuestOnce)
                {
                    rc = cr_server.RcToGuestOnce;
                    cr_server.RcToGuestOnce = 0;
                }
                else
                {
                    rc = cr_server.RcToGuest;
                }
                if (puRc)
                    *puRc = rc;
                else
                    *pbRc = !!rc;
            }
            else
            {
                crWarning("zero bytes for GL_DBG_CHECK_BREAK_CR");
            }
            break;
        }
        case GL_HH_SET_DEFAULT_SHARED_CTX:
            WARN(("Recieved GL_HH_SET_DEFAULT_SHARED_CTX from guest, ignoring"));
            break;
        case GL_HH_SET_CLIENT_CALLOUT:
            WARN(("Recieved GL_HH_SET_CLIENT_CALLOUT from guest, ignoring"));
            break;
        default:
            cr_server.head_spu->dispatch_table.GetChromiumParametervCR( target, index, type, count, local_storage );
            break;
    }

    crServerReturnValue( local_storage, bytes );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchChromiumParametervCR(GLenum target, GLenum type, GLsizei count, const GLvoid *values)
{
    CRMuralInfo *mural = cr_server.curClient->currentMural;
    static int gather_connect_count = 0;

    switch (target) {
        case GL_SHARE_LISTS_CR:
        {
            CRContextInfo *pCtx[2];
            GLint *ai32Values;
            int i;
            if (count != 2)
            {
                WARN(("GL_SHARE_LISTS_CR invalid cound %d", count));
                return;
            }

            if (type != GL_UNSIGNED_INT && type != GL_INT)
            {
                WARN(("GL_SHARE_LISTS_CR invalid type %d", type));
                return;
            }

            ai32Values = (GLint*)values;

            for (i = 0; i < 2; ++i)
            {
                const int32_t val = ai32Values[i];

                if (val == 0)
                {
                    WARN(("GL_SHARE_LISTS_CR invalid value[%d] %d", i, val));
                    return;
                }

                pCtx[i] = (CRContextInfo *) crHashtableSearch(cr_server.contextTable, val);
                if (!pCtx[i])
                {
                    WARN(("GL_SHARE_LISTS_CR invalid pCtx1 for value[%d] %d", i, val));
                    return;
                }

                if (!pCtx[i]->pContext)
                {
                    WARN(("GL_SHARE_LISTS_CR invalid pCtx1 pContext for value[%d] %d", i, val));
                    return;
                }
            }

            crStateShareLists(pCtx[0]->pContext, pCtx[1]->pContext);

            break;
        }

    case GL_SET_MAX_VIEWPORT_CR:
        {
            GLint *maxDims = (GLint *)values;
            cr_server.limits.maxViewportDims[0] = maxDims[0];
            cr_server.limits.maxViewportDims[1] = maxDims[1];
        }
        break;

    case GL_TILE_INFO_CR:
        /* message from tilesort SPU to set new tile bounds */
        {
            GLint numTiles, muralWidth, muralHeight, server, tiles;
            GLint *tileBounds;
            CRASSERT(count >= 4);
            CRASSERT((count - 4) % 4 == 0); /* must be multiple of four */
            CRASSERT(type == GL_INT);
            numTiles = (count - 4) / 4;
            tileBounds = (GLint *) values;
            server = tileBounds[0];
            muralWidth = tileBounds[1];
            muralHeight = tileBounds[2];
            tiles = tileBounds[3];
            CRASSERT(tiles == numTiles);
            tileBounds += 4; /* skip over header values */
            /*crServerNewMuralTiling(mural, muralWidth, muralHeight, numTiles, tileBounds);
            mural->viewportValidated = GL_FALSE;*/
        }
        break;

    case GL_GATHER_DRAWPIXELS_CR:
        if (cr_server.only_swap_once && cr_server.curClient != cr_server.clients[0])
            break;
        cr_server.head_spu->dispatch_table.ChromiumParametervCR( target, type, count, values );
        break;

    case GL_GATHER_CONNECT_CR:
        /*
         * We want the last connect to go through,
         * otherwise we might deadlock in CheckWindowSize()
         * in the readback spu
         */
        gather_connect_count++;
        if (cr_server.only_swap_once && (gather_connect_count != cr_server.numClients))
        {
            break;
        }
        cr_server.head_spu->dispatch_table.ChromiumParametervCR( target, type, count, values );
        gather_connect_count = 0;
        break;

    case GL_SERVER_VIEW_MATRIX_CR:
        /* Set this server's view matrix which will get premultiplied onto the
         * modelview matrix.  For non-planar tilesort and stereo.
         */
        CRASSERT(count == 18);
        CRASSERT(type == GL_FLOAT);
        /* values[0] is the server index. Ignored here but used in tilesort SPU */
        /* values[1] is the left/right eye index (0 or 1) */
        {
            const GLfloat *v = (const GLfloat *) values;
            const int eye = v[1] == 0.0 ? 0 : 1;
            crMatrixInitFromFloats(&cr_server.viewMatrix[eye], v + 2);

            crDebug("Got GL_SERVER_VIEW_MATRIX_CR:\n"
                            "  %f %f %f %f\n"
                            "  %f %f %f %f\n"
                            "  %f %f %f %f\n"
                            "  %f %f %f %f",
                            cr_server.viewMatrix[eye].m00,
                            cr_server.viewMatrix[eye].m10,
                            cr_server.viewMatrix[eye].m20,
                            cr_server.viewMatrix[eye].m30,
                            cr_server.viewMatrix[eye].m01,
                            cr_server.viewMatrix[eye].m11,
                            cr_server.viewMatrix[eye].m21,
                            cr_server.viewMatrix[eye].m31,
                            cr_server.viewMatrix[eye].m02,
                            cr_server.viewMatrix[eye].m12,
                            cr_server.viewMatrix[eye].m22,
                            cr_server.viewMatrix[eye].m32,
                            cr_server.viewMatrix[eye].m03,
                            cr_server.viewMatrix[eye].m13,
                            cr_server.viewMatrix[eye].m23,
                            cr_server.viewMatrix[eye].m33);
        }
        cr_server.viewOverride = GL_TRUE;
        break;

    case GL_SERVER_PROJECTION_MATRIX_CR:
        /* Set this server's projection matrix which will get replace the user's
         * projection matrix.  For non-planar tilesort and stereo.
         */
        CRASSERT(count == 18);
        CRASSERT(type == GL_FLOAT);
        /* values[0] is the server index. Ignored here but used in tilesort SPU */
        /* values[1] is the left/right eye index (0 or 1) */
        {
            const GLfloat *v = (const GLfloat *) values;
            const int eye = v[1] == 0.0 ? 0 : 1;
            crMatrixInitFromFloats(&cr_server.projectionMatrix[eye], v + 2);

            crDebug("Got GL_SERVER_PROJECTION_MATRIX_CR:\n"
                            "  %f %f %f %f\n"
                            "  %f %f %f %f\n"
                            "  %f %f %f %f\n"
                            "  %f %f %f %f",
                            cr_server.projectionMatrix[eye].m00,
                            cr_server.projectionMatrix[eye].m10,
                            cr_server.projectionMatrix[eye].m20,
                            cr_server.projectionMatrix[eye].m30,
                            cr_server.projectionMatrix[eye].m01,
                            cr_server.projectionMatrix[eye].m11,
                            cr_server.projectionMatrix[eye].m21,
                            cr_server.projectionMatrix[eye].m31,
                            cr_server.projectionMatrix[eye].m02,
                            cr_server.projectionMatrix[eye].m12,
                            cr_server.projectionMatrix[eye].m22,
                            cr_server.projectionMatrix[eye].m32,
                            cr_server.projectionMatrix[eye].m03,
                            cr_server.projectionMatrix[eye].m13,
                            cr_server.projectionMatrix[eye].m23,
                            cr_server.projectionMatrix[eye].m33);

            if (cr_server.projectionMatrix[eye].m33 == 0.0f) {
                float x = cr_server.projectionMatrix[eye].m00;
                float y = cr_server.projectionMatrix[eye].m11;
                float a = cr_server.projectionMatrix[eye].m20;
                float b = cr_server.projectionMatrix[eye].m21;
                float c = cr_server.projectionMatrix[eye].m22;
                float d = cr_server.projectionMatrix[eye].m32;
                float znear = -d / (1.0f - c);
                float zfar = (c - 1.0f) * znear / (c + 1.0f);
                float left = znear * (a - 1.0f) / x;
                float right = 2.0f * znear / x + left;
                float bottom = znear * (b - 1.0f) / y;
              float top = 2.0f * znear / y + bottom;
              crDebug("Frustum: left, right, bottom, top, near, far: %f, %f, %f, %f, %f, %f", left, right, bottom, top, znear, zfar);
            }
            else {
                /** @todo Add debug output for orthographic projection*/
            }

        }
        cr_server.projectionOverride = GL_TRUE;
        break;

    case GL_HH_SET_TMPCTX_MAKE_CURRENT:
        /*we should not receive it from the guest! */
        break;

    case GL_HH_SET_CLIENT_CALLOUT:
        WARN(("Recieved GL_HH_SET_CLIENT_CALLOUT from guest, ignoring"));
        break;

    default:
        /* Pass the parameter info to the head SPU */
        cr_server.head_spu->dispatch_table.ChromiumParametervCR( target, type, count, values );
        break;
    }
}


void SERVER_DISPATCH_APIENTRY crServerDispatchChromiumParameteriCR(GLenum target, GLint value)
{
  switch (target) {
    case GL_SHARE_CONTEXT_RESOURCES_CR:
        crStateShareContext(value);
        break;
    case GL_RCUSAGE_TEXTURE_SET_CR:
        crStateSetTextureUsed(value, GL_TRUE);
        break;
    case GL_RCUSAGE_TEXTURE_CLEAR_CR:
        crStateSetTextureUsed(value, GL_FALSE);
        break;
    case GL_PIN_TEXTURE_SET_CR:
        crStatePinTexture(value, GL_TRUE);
        break;
    case GL_PIN_TEXTURE_CLEAR_CR:
        crStatePinTexture(value, GL_FALSE);
        break;
    case GL_SHARED_DISPLAY_LISTS_CR:
        cr_server.sharedDisplayLists = value;
        break;
    case GL_SHARED_TEXTURE_OBJECTS_CR:
        cr_server.sharedTextureObjects = value;
        break;
    case GL_SHARED_PROGRAMS_CR:
        cr_server.sharedPrograms = value;
        break;
    case GL_SERVER_CURRENT_EYE_CR:
        cr_server.currentEye = value ? 1 : 0;
        break;
    case GL_HOST_WND_CREATED_HIDDEN_CR:
        cr_server.bWindowsInitiallyHidden = value ? 1 : 0;
        break;
    case GL_HH_SET_DEFAULT_SHARED_CTX:
        WARN(("Recieved GL_HH_SET_DEFAULT_SHARED_CTX from guest, ignoring"));
        break;
    case GL_HH_RENDERTHREAD_INFORM:
        WARN(("Recieved GL_HH_RENDERTHREAD_INFORM from guest, ignoring"));
        break;
    default:
        /* Pass the parameter info to the head SPU */
        cr_server.head_spu->dispatch_table.ChromiumParameteriCR( target, value );
    }
}


void SERVER_DISPATCH_APIENTRY crServerDispatchChromiumParameterfCR(GLenum target, GLfloat value)
{
  switch (target) {
    case GL_SHARED_DISPLAY_LISTS_CR:
        cr_server.sharedDisplayLists = (int) value;
        break;
    case GL_SHARED_TEXTURE_OBJECTS_CR:
        cr_server.sharedTextureObjects = (int) value;
        break;
    case GL_SHARED_PROGRAMS_CR:
        cr_server.sharedPrograms = (int) value;
        break;
    default:
        /* Pass the parameter info to the head SPU */
        cr_server.head_spu->dispatch_table.ChromiumParameterfCR( target, value );
    }
}

GLint crServerGenerateID(GLint *pCounter)
{
    return (*pCounter)++;
}

/*#define CR_DUMP_BLITS*/

#ifdef CR_DUMP_BLITS
static int blitnum=0;
static int copynum=0;
#endif

# ifdef DEBUG_misha
//# define CR_CHECK_BLITS
#  include <iprt/assert.h>
#  undef CRASSERT /* iprt assert's int3 are inlined that is why are more convenient to use since they can be easily disabled individually */
#  define CRASSERT Assert
# endif


void SERVER_DISPATCH_APIENTRY
crServerDispatchCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    /** @todo pbo/fbo disabled for now as it's slower, check on other gpus*/
    static int siHavePBO = 0;
    static int siHaveFBO = 0;

    if ((target!=GL_TEXTURE_2D) || (height>=0))
    {
        cr_server.head_spu->dispatch_table.CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);

#ifdef CR_DUMP_BLITS
        {
            SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;
            void *img;
            GLint w, h;
            char fname[200];

            copynum++;

            gl->GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
            gl->GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

            img = crAlloc(w*h*4);
            CRASSERT(img);

            gl->GetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, img);
            sprintf(fname, "copy_blit%i_copy_%i.tga", blitnum, copynum);
            crDumpNamedTGA(fname, w, h, img);
            crFree(img);
        }
#endif
    }
    else /* negative height, means we have to Yinvert the source pixels while copying */
    {
        SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;

        if (siHavePBO<0)
        {
            const char *ext = (const char*)gl->GetString(GL_EXTENSIONS);
            siHavePBO = crStrstr(ext, "GL_ARB_pixel_buffer_object") ? 1:0;
        }

        if (siHaveFBO<0)
        {
            const char *ext = (const char*)gl->GetString(GL_EXTENSIONS);
            siHaveFBO = crStrstr(ext, "GL_EXT_framebuffer_object") ? 1:0;
        }

        if (siHavePBO==0 && siHaveFBO==0)
        {
#if 1
            GLint dRow, sRow;
            for (dRow=yoffset, sRow=y-height-1; dRow<yoffset-height; dRow++, sRow--)
            {
                gl->CopyTexSubImage2D(target, level, xoffset, dRow, x, sRow, width, 1);
            }
#else
            {
                GLint w, h, i;
                char *img1, *img2, *sPtr, *dPtr;
                CRContext *ctx = crStateGetCurrent();

                w = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->level[0][level].width;
                h = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->level[0][level].height;

                img1 = crAlloc(4*w*h);
                img2 = crAlloc(4*width*(-height));
                CRASSERT(img1 && img2);

                gl->CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, -height);
                gl->GetTexImage(target, level, GL_RGBA, GL_UNSIGNED_BYTE, img1);

                sPtr=img1+4*xoffset+4*w*yoffset;
                dPtr=img2+4*width*(-height-1);

                for (i=0; i<-height; ++i)
                {
                    crMemcpy(dPtr, sPtr, 4*width);
                    sPtr += 4*w;
                    dPtr -= 4*width;
                }

                gl->TexSubImage2D(target, level, xoffset, yoffset, width, -height, GL_RGBA, GL_UNSIGNED_BYTE, img2);

                crFree(img1);
                crFree(img2);
            }
#endif
        }
        else if (siHaveFBO==1) /** @todo more states to set and restore here*/
        {
            GLuint tID, fboID;
            GLenum status;
            CRContext *ctx = crStateGetCurrent();

            gl->GenTextures(1, &tID);
            gl->BindTexture(target, tID);
            gl->CopyTexImage2D(target, level, GL_RGBA, x, y, width, -height, 0);
            gl->GenFramebuffersEXT(1, &fboID);
            gl->BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);
            gl->FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target,
                                        ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->hwid, level);
            status = gl->CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                crWarning("Framebuffer status 0x%x", status);
            }

            gl->Enable(target);
            gl->PushAttrib(GL_VIEWPORT_BIT);
            gl->Viewport(xoffset, yoffset, width, -height);
            gl->MatrixMode(GL_PROJECTION);
            gl->PushMatrix();
            gl->LoadIdentity();
            gl->MatrixMode(GL_MODELVIEW);
	        gl->PushMatrix();
            gl->LoadIdentity();

            gl->Disable(GL_DEPTH_TEST);
            gl->Disable(GL_CULL_FACE);
            gl->Disable(GL_STENCIL_TEST);
            gl->Disable(GL_SCISSOR_TEST);

            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            gl->TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            gl->TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl->TexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl->TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            gl->Begin(GL_QUADS);
                gl->TexCoord2f(0.0f, 1.0f);
                gl->Vertex2f(-1.0, -1.0);

                gl->TexCoord2f(0.0f, 0.0f);
                gl->Vertex2f(-1.0f, 1.0f);

                gl->TexCoord2f(1.0f, 0.0f);
                gl->Vertex2f(1.0f, 1.0f);

                gl->TexCoord2f(1.0f, 1.0f);
                gl->Vertex2f(1.0f, -1.0f);
            gl->End();

            gl->PopMatrix();
            gl->MatrixMode(GL_PROJECTION);
            gl->PopMatrix();
            gl->PopAttrib();

            gl->FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target, 0, level);
            gl->BindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->framebufferobject.drawFB ? ctx->framebufferobject.drawFB->hwid:0);
            gl->BindTexture(target, ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->hwid);
            gl->DeleteFramebuffersEXT(1, &fboID);
            gl->DeleteTextures(1, &tID);

#if 0
            {
                GLint dRow, sRow, w, h;
                void *img1, *img2;

                w = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->level[0][level].width;
                h = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D->level[0][level].height;

                img1 = crAlloc(4*w*h);
                img2 = crAlloc(4*w*h);
                CRASSERT(img1 && img2);

                gl->GetTexImage(target, level, GL_BGRA, GL_UNSIGNED_BYTE, img1);


                for (dRow=yoffset, sRow=y-height-1; dRow<yoffset-height; dRow++, sRow--)
                {
                    gl->CopyTexSubImage2D(target, level, xoffset, dRow, x, sRow, width, 1);
                }

                gl->GetTexImage(target, level, GL_BGRA, GL_UNSIGNED_BYTE, img2);

                if (crMemcmp(img1, img2, 4*w*h))
                {
                    crDebug("MISMATCH! (%x, %i, ->%i,%i  <-%i, %i  [%ix%i])", target, level, xoffset, yoffset, x, y, width, height);
                    crDumpTGA(w, h, img1);
                    crDumpTGA(w, h, img2);
                    DebugBreak();
                }
                crFree(img1);
                crFree(img2);
            }
#endif
        }
        else
        {
            GLint dRow;
            GLuint pboId, sRow;
            CRContext *ctx = crStateGetCurrent();

            gl->GenBuffersARB(1, &pboId);
            gl->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboId);
            gl->BufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, -width*height*4, 0, GL_STATIC_COPY_ARB);

#if 1
            gl->ReadPixels(x, y, width, -height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            gl->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, ctx->bufferobject.packBuffer->hwid);

            gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboId);
            for (dRow=yoffset, sRow=-height-1; dRow<yoffset-height; dRow++, sRow--)
            {
                gl->TexSubImage2D(target, level, xoffset, dRow, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void*)((uintptr_t)sRow*width*4));
            }
#else /*few times slower again*/
            for (dRow=0, sRow=y-height-1; dRow<-height; dRow++, sRow--)
            {
                gl->ReadPixels(x, sRow, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void*)((uintptr_t)dRow*width*4));
            }
            gl->BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, ctx->bufferobject.packBuffer->hwid);

            gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboId);
            gl->TexSubImage2D(target, level, xoffset, yoffset, width, -height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#endif

            gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->bufferobject.unpackBuffer->hwid);
            gl->DeleteBuffersARB(1, &pboId);
        }
    }
}

#ifdef CR_CHECK_BLITS
void crDbgFree(void *pvData)
{
    crFree(pvData);
}

void crDbgGetTexImage2D(GLint texTarget, GLint texName, GLvoid **ppvImage, GLint *pw, GLint *ph)
{
    SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;
    GLint ppb, pub, dstw, dsth, otex;
    GLint pa, pr, psp, psr, ua, ur, usp, usr;
    GLvoid *pvImage;
    GLint rfb, dfb, rb, db;

    gl->GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &rfb);
    gl->GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &dfb);
    gl->GetIntegerv(GL_READ_BUFFER, &rb);
    gl->GetIntegerv(GL_DRAW_BUFFER, &db);

    gl->BindFramebufferEXT(GL_READ_FRAMEBUFFER_BINDING_EXT, 0);
    gl->BindFramebufferEXT(GL_DRAW_FRAMEBUFFER_BINDING_EXT, 0);
    gl->ReadBuffer(GL_BACK);
    gl->DrawBuffer(GL_BACK);

    gl->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &ppb);
    gl->GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pub);
    gl->GetIntegerv(GL_TEXTURE_BINDING_2D, &otex);

    gl->GetIntegerv(GL_PACK_ROW_LENGTH, &pr);
    gl->GetIntegerv(GL_PACK_ALIGNMENT, &pa);
    gl->GetIntegerv(GL_PACK_SKIP_PIXELS, &psp);
    gl->GetIntegerv(GL_PACK_SKIP_ROWS, &psr);

    gl->GetIntegerv(GL_UNPACK_ROW_LENGTH, &ur);
    gl->GetIntegerv(GL_UNPACK_ALIGNMENT, &ua);
    gl->GetIntegerv(GL_UNPACK_SKIP_PIXELS, &usp);
    gl->GetIntegerv(GL_UNPACK_SKIP_ROWS, &usr);

    gl->BindTexture(texTarget, texName);
    gl->GetTexLevelParameteriv(texTarget, 0, GL_TEXTURE_WIDTH, &dstw);
    gl->GetTexLevelParameteriv(texTarget, 0, GL_TEXTURE_HEIGHT, &dsth);

    gl->PixelStorei(GL_PACK_ROW_LENGTH, 0);
    gl->PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl->PixelStorei(GL_PACK_SKIP_PIXELS, 0);
    gl->PixelStorei(GL_PACK_SKIP_ROWS, 0);

    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    gl->PixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    gl->BindBufferARB(GL_PIXEL_PACK_BUFFER, 0);
    gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER, 0);

    pvImage = crAlloc(4*dstw*dsth);
    gl->GetTexImage(texTarget, 0, GL_BGRA, GL_UNSIGNED_BYTE, pvImage);

    gl->BindTexture(texTarget, otex);

    gl->PixelStorei(GL_PACK_ROW_LENGTH, pr);
    gl->PixelStorei(GL_PACK_ALIGNMENT, pa);
    gl->PixelStorei(GL_PACK_SKIP_PIXELS, psp);
    gl->PixelStorei(GL_PACK_SKIP_ROWS, psr);

    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, ur);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, ua);
    gl->PixelStorei(GL_UNPACK_SKIP_PIXELS, usp);
    gl->PixelStorei(GL_UNPACK_SKIP_ROWS, usr);

    gl->BindBufferARB(GL_PIXEL_PACK_BUFFER, ppb);
    gl->BindBufferARB(GL_PIXEL_UNPACK_BUFFER, pub);

    gl->BindFramebufferEXT(GL_READ_FRAMEBUFFER_BINDING_EXT, rfb);
    gl->BindFramebufferEXT(GL_DRAW_FRAMEBUFFER_BINDING_EXT, dfb);
    gl->ReadBuffer(rb);
    gl->DrawBuffer(db);

    *ppvImage = pvImage;
    *pw = dstw;
    *ph = dsth;
}

DECLEXPORT(void) crDbgPrint(const char *format, ... )
{
    va_list args;
    static char txt[8092];

    va_start(args, format);
    vsprintf(txt, format, args);
    va_end(args);

    OutputDebugString(txt);
}

void crDbgDumpImage2D(const char* pszDesc, const void *pvData, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch)
{
    crDbgPrint("<?dml?><exec cmd=\"!vbvdbg.ms 0x%p 0n%d 0n%d 0n%d 0n%d\">%s</exec>, ( !vbvdbg.ms 0x%p 0n%d 0n%d 0n%d 0n%d )\n",
            pvData, width, height, bpp, pitch,
            pszDesc,
            pvData, width, height, bpp, pitch);
}

void crDbgDumpTexImage2D(const char* pszDesc, GLint texTarget, GLint texName, GLboolean fBreak)
{
    GLvoid *pvImage;
    GLint w, h;
    crDbgGetTexImage2D(texTarget, texName, &pvImage, &w, &h);
    crDbgPrint("%s target(%d), name(%d), width(%d), height(%d)", pszDesc, texTarget, texName, w, h);
    crDbgDumpImage2D("texture data", pvImage, w, h, 32, (32 * w)/8);
    if (fBreak)
    {
        CRASSERT(0);
    }
    crDbgFree(pvImage);
}
#endif

PCR_BLITTER crServerVBoxBlitterGet()
{
    if (!CrBltIsInitialized(&cr_server.Blitter))
    {
        CR_BLITTER_CONTEXT Ctx;
        int rc;
        CRASSERT(cr_server.MainContextInfo.SpuContext);
        Ctx.Base.id = cr_server.MainContextInfo.SpuContext;
        Ctx.Base.visualBits = cr_server.MainContextInfo.CreateInfo.realVisualBits;
        rc = CrBltInit(&cr_server.Blitter, &Ctx, true, true, NULL, &cr_server.TmpCtxDispatch);
        if (RT_SUCCESS(rc))
        {
            CRASSERT(CrBltIsInitialized(&cr_server.Blitter));
        }
        else
        {
            crWarning("CrBltInit failed, rc %d", rc);
            CRASSERT(!CrBltIsInitialized(&cr_server.Blitter));
            return NULL;
        }
    }

    if (!CrBltMuralGetCurrentInfo(&cr_server.Blitter)->Base.id)
    {
        CRMuralInfo *dummy = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
        CR_BLITTER_WINDOW DummyInfo;
        CRASSERT(dummy);
        crServerVBoxBlitterWinInit(&DummyInfo, dummy);
        CrBltMuralSetCurrentInfo(&cr_server.Blitter, &DummyInfo);
    }

    return &cr_server.Blitter;
}

PCR_BLITTER crServerVBoxBlitterGetInitialized()
{
    if (CrBltIsInitialized(&cr_server.Blitter))
        return &cr_server.Blitter;
    return NULL;
}


int crServerVBoxBlitterTexInit(CRContext *ctx, CRMuralInfo *mural, PVBOXVR_TEXTURE pTex, GLboolean fDraw)
{
    CRTextureObj *tobj;
    CRFramebufferObjectState *pBuf = &ctx->framebufferobject;
    GLenum enmBuf;
    CRFBOAttachmentPoint *pAp;
    GLuint idx;
    CRTextureLevel *tl;
    CRFramebufferObject *pFBO = fDraw ? pBuf->drawFB : pBuf->readFB;

    if (!pFBO)
    {
        GLuint hwid;

        if (!mural->fRedirected)
        {
            WARN(("mural not redirected!"));
            return VERR_NOT_IMPLEMENTED;
        }

        enmBuf = fDraw ? ctx->buffer.drawBuffer : ctx->buffer.readBuffer;
        switch (enmBuf)
        {
            case GL_BACK:
            case GL_BACK_RIGHT:
            case GL_BACK_LEFT:
                hwid = mural->aidColorTexs[CR_SERVER_FBO_BB_IDX(mural)];
                break;
            case GL_FRONT:
            case GL_FRONT_RIGHT:
            case GL_FRONT_LEFT:
                hwid = mural->aidColorTexs[CR_SERVER_FBO_FB_IDX(mural)];
                break;
            default:
                WARN(("unsupported enum buf %d", enmBuf));
                return VERR_NOT_IMPLEMENTED;
                break;
        }

        if (!hwid)
        {
            crWarning("offscreen render tex hwid is null");
            return VERR_INVALID_STATE;
        }

        pTex->width = mural->width;
        pTex->height = mural->height;
        pTex->target = GL_TEXTURE_2D;
        pTex->hwid = hwid;
        return VINF_SUCCESS;
    }

    enmBuf = fDraw ? pFBO->drawbuffer[0] : pFBO->readbuffer;
    idx = enmBuf - GL_COLOR_ATTACHMENT0_EXT;
    if (idx >= CR_MAX_COLOR_ATTACHMENTS)
    {
        crWarning("idx is invalid %d, using 0", idx);
    }

    pAp = &pFBO->color[idx];

    if (!pAp->name)
    {
        crWarning("no collor draw attachment");
        return VERR_INVALID_STATE;
    }

    if (pAp->level)
    {
        WARN(("non-zero level not implemented"));
        return VERR_NOT_IMPLEMENTED;
    }

    tobj = (CRTextureObj*)crHashtableSearch(ctx->shared->textureTable, pAp->name);
    if (!tobj)
    {
        crWarning("no texture object found for name %d", pAp->name);
        return VERR_INVALID_STATE;
    }

    if (tobj->target != GL_TEXTURE_2D && tobj->target != GL_TEXTURE_RECTANGLE_NV)
    {
        WARN(("non-texture[rect|2d] not implemented"));
        return VERR_NOT_IMPLEMENTED;
    }

    CRASSERT(tobj->hwid);

    tl = tobj->level[0];
    pTex->width = tl->width;
    pTex->height = tl->height;
    pTex->target = tobj->target;
    pTex->hwid = tobj->hwid;

    return VINF_SUCCESS;
}

int crServerVBoxBlitterBlitCurrentCtx(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
        GLbitfield mask, GLenum filter)
{
    PCR_BLITTER pBlitter;
    CR_BLITTER_CONTEXT Ctx;
    CRMuralInfo *mural;
    CRContext *ctx = crStateGetCurrent();
    PVBOXVR_TEXTURE pDrawTex, pReadTex;
    VBOXVR_TEXTURE DrawTex, ReadTex;
    int rc;
    GLuint idDrawFBO, idReadFBO;
    CR_BLITTER_WINDOW BltInfo;

    if (mask != GL_COLOR_BUFFER_BIT)
    {
        WARN(("not supported blit mask %d", mask));
        return VERR_NOT_IMPLEMENTED;
    }

    if (!cr_server.curClient)
    {
        crWarning("no current client");
        return VERR_INVALID_STATE;
    }
    mural = cr_server.curClient->currentMural;
    if (!mural)
    {
        crWarning("no current mural");
        return VERR_INVALID_STATE;
    }

    rc = crServerVBoxBlitterTexInit(ctx, mural, &DrawTex, GL_TRUE);
    if (RT_SUCCESS(rc))
    {
        pDrawTex = &DrawTex;
    }
    else
    {
        crWarning("crServerVBoxBlitterTexInit failed for draw");
        return rc;
    }

    rc = crServerVBoxBlitterTexInit(ctx, mural, &ReadTex, GL_FALSE);
    if (RT_SUCCESS(rc))
    {
        pReadTex = &ReadTex;
    }
    else
    {
//        crWarning("crServerVBoxBlitterTexInit failed for read");
        return rc;
    }

    pBlitter = crServerVBoxBlitterGet();
    if (!pBlitter)
    {
        crWarning("crServerVBoxBlitterGet failed");
        return VERR_GENERAL_FAILURE;
    }

    crServerVBoxBlitterWinInit(&BltInfo, mural);

    crServerVBoxBlitterCtxInit(&Ctx, cr_server.curClient->currentCtxInfo);

    CrBltMuralSetCurrentInfo(pBlitter, &BltInfo);

    idDrawFBO = CR_SERVER_FBO_FOR_IDX(mural, mural->iCurDrawBuffer);
    idReadFBO = CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer);

    crStateSwitchPrepare(NULL, ctx, idDrawFBO, idReadFBO);

    rc = CrBltEnter(pBlitter);
    if (RT_SUCCESS(rc))
    {
        RTRECT ReadRect, DrawRect;
        ReadRect.xLeft = srcX0;
        ReadRect.yTop = srcY0;
        ReadRect.xRight = srcX1;
        ReadRect.yBottom = srcY1;
        DrawRect.xLeft = dstX0;
        DrawRect.yTop = dstY0;
        DrawRect.xRight = dstX1;
        DrawRect.yBottom = dstY1;
        CrBltBlitTexTex(pBlitter, pReadTex, &ReadRect, pDrawTex, &DrawRect, 1, CRBLT_FLAGS_FROM_FILTER(filter));
        CrBltLeave(pBlitter);
    }
    else
    {
        crWarning("CrBltEnter failed rc %d", rc);
    }

    crStateSwitchPostprocess(ctx, NULL, idDrawFBO, idReadFBO);

    return rc;
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                   GLbitfield mask, GLenum filter)
{
    CRContext *ctx = crStateGetCurrent();
    bool fTryBlitter = false;
#ifdef CR_CHECK_BLITS
//    {
        SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;
        GLint rfb=0, dfb=0, dtex=0, dlev=-1, rtex=0, rlev=-1, rb=0, db=0, ppb=0, pub=0, vp[4], otex, dstw, dsth;
        GLint sdtex=0, srtex=0;
        GLenum dStatus, rStatus;

        CRTextureObj *tobj = 0;
        CRTextureLevel *tl = 0;
        GLint id, tuId, pbufId, pbufIdHw, ubufId, ubufIdHw, width, height, depth;

        crDebug("===StateTracker===");
        crDebug("Current TU: %i", ctx->texture.curTextureUnit);

        tobj = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D;
        CRASSERT(tobj);
        tl = &tobj->level[0][0];
        crDebug("Texture %i(hw %i), w=%i, h=%i", tobj->id, tobj->hwid, tl->width, tl->height, tl->depth);

        if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
        {
            pbufId = ctx->bufferobject.packBuffer->hwid;
        }
        else
        {
            pbufId = 0;
        }
        crDebug("Pack BufferId %i", pbufId);

        if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
        {
            ubufId = ctx->bufferobject.unpackBuffer->hwid;
        }
        else
        {
            ubufId = 0;
        }
        crDebug("Unpack BufferId %i", ubufId);

        crDebug("===GPU===");
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_ACTIVE_TEXTURE, &tuId);
        crDebug("Current TU: %i", tuId - GL_TEXTURE0_ARB);
        CRASSERT(tuId - GL_TEXTURE0_ARB == ctx->texture.curTextureUnit);

        cr_server.head_spu->dispatch_table.GetIntegerv(GL_TEXTURE_BINDING_2D, &id);
        cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH, &depth);
        crDebug("Texture: %i, w=%i, h=%i, d=%i", id, width, height, depth);
        CRASSERT(id == tobj->hwid);
        CRASSERT(width == tl->width);
        CRASSERT(height == tl->height);
        CRASSERT(depth == tl->depth);

        cr_server.head_spu->dispatch_table.GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pbufIdHw);
        crDebug("Hw Pack BufferId %i", pbufIdHw);
        CRASSERT(pbufIdHw == pbufId);

        cr_server.head_spu->dispatch_table.GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &ubufIdHw);
        crDebug("Hw Unpack BufferId %i", ubufIdHw);
        CRASSERT(ubufIdHw == ubufId);

        gl->GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &rfb);
        gl->GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &dfb);
        gl->GetIntegerv(GL_READ_BUFFER, &rb);
        gl->GetIntegerv(GL_DRAW_BUFFER, &db);

        gl->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &ppb);
        gl->GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pub);

        gl->GetIntegerv(GL_VIEWPORT, &vp[0]);

        gl->GetIntegerv(GL_TEXTURE_BINDING_2D, &otex);

        gl->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &dtex);
        gl->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT, &dlev);
        dStatus = gl->CheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);

        gl->GetFramebufferAttachmentParameterivEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &rtex);
        gl->GetFramebufferAttachmentParameterivEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT, &rlev);
        rStatus = gl->CheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER_EXT);

        if (dtex)
        {
            CRASSERT(!dlev);
        }

        if (rtex)
        {
            CRASSERT(!rlev);
        }

        if (ctx->framebufferobject.drawFB)
        {
            CRASSERT(dfb);
            CRASSERT(ctx->framebufferobject.drawFB->hwid == dfb);
            CRASSERT(ctx->framebufferobject.drawFB->drawbuffer[0] == db);

            CRASSERT(dStatus==GL_FRAMEBUFFER_COMPLETE_EXT);
            CRASSERT(db==GL_COLOR_ATTACHMENT0_EXT);

            CRASSERT(ctx->framebufferobject.drawFB->color[0].type == GL_TEXTURE);
            CRASSERT(ctx->framebufferobject.drawFB->color[0].level == 0);
            sdtex = ctx->framebufferobject.drawFB->color[0].name;
            sdtex = crStateGetTextureHWID(sdtex);

            CRASSERT(sdtex);
        }
        else
        {
            CRASSERT(!dfb);
        }

        if (ctx->framebufferobject.readFB)
        {
            CRASSERT(rfb);
            CRASSERT(ctx->framebufferobject.readFB->hwid == rfb);

            CRASSERT(rStatus==GL_FRAMEBUFFER_COMPLETE_EXT);

            CRASSERT(ctx->framebufferobject.readFB->color[0].type == GL_TEXTURE);
            CRASSERT(ctx->framebufferobject.readFB->color[0].level == 0);
            srtex = ctx->framebufferobject.readFB->color[0].name;
            srtex = crStateGetTextureHWID(srtex);

            CRASSERT(srtex);
        }
        else
        {
            CRASSERT(!rfb);
        }

        CRASSERT(sdtex == dtex);
        CRASSERT(srtex == rtex);

//        crDbgDumpTexImage2D("==> src tex:", GL_TEXTURE_2D, rtex, true);
//        crDbgDumpTexImage2D("==> dst tex:", GL_TEXTURE_2D, dtex, true);

//    }
#endif
#ifdef CR_DUMP_BLITS
    SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;
    GLint rfb=0, dfb=0, dtex=0, dlev=-1, rb=0, db=0, ppb=0, pub=0, vp[4], otex, dstw, dsth;
    GLenum status;
    char fname[200];
    void *img;

    blitnum++;

    crDebug("[%i]BlitFramebufferEXT(%i, %i, %i, %i, %i, %i, %i, %i, %x, %x)", blitnum, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    crDebug("%i, %i <-> %i, %i", srcX1-srcX0, srcY1-srcY0, dstX1-dstX0, dstY1-dstY0);

    gl->GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &rfb);
    gl->GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &dfb);
    gl->GetIntegerv(GL_READ_BUFFER, &rb);
    gl->GetIntegerv(GL_DRAW_BUFFER, &db);

    gl->GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &ppb);
    gl->GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &pub);

    gl->GetIntegerv(GL_VIEWPORT, &vp[0]);

    gl->GetIntegerv(GL_TEXTURE_BINDING_2D, &otex);

    CRASSERT(!rfb && dfb);
    gl->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &dtex);
    gl->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT, &dlev);
    status = gl->CheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);

    CRASSERT(status==GL_FRAMEBUFFER_COMPLETE_EXT
             && db==GL_COLOR_ATTACHMENT0_EXT
             && (rb==GL_FRONT || rb==GL_BACK)
             && !rfb && dfb && dtex && !dlev
             && !ppb && !pub);

    crDebug("Src[rb 0x%x, fbo %i] Dst[db 0x%x, fbo %i(0x%x), tex %i.%i]", rb, rfb, db, dfb, status, dtex, dlev);
    crDebug("Viewport [%i, %i, %i, %i]", vp[0], vp[1], vp[2], vp[3]);

    gl->PixelStorei(GL_PACK_ROW_LENGTH, 0);
    gl->PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl->PixelStorei(GL_PACK_SKIP_PIXELS, 0);
    gl->PixelStorei(GL_PACK_SKIP_ROWS, 0);

    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    gl->PixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    gl->BindTexture(GL_TEXTURE_2D, dtex);
    gl->GetTexLevelParameteriv(GL_TEXTURE_2D, dlev, GL_TEXTURE_WIDTH, &dstw);
    gl->GetTexLevelParameteriv(GL_TEXTURE_2D, dlev, GL_TEXTURE_HEIGHT, &dsth);
    gl->BindTexture(GL_TEXTURE_2D, otex);
    crDebug("Dst is %i, %i", dstw, dsth);

    CRASSERT(vp[2]>=dstw && vp[3]>=dsth);
    img = crAlloc(vp[2]*vp[3]*4);
    CRASSERT(img);

    gl->ReadPixels(0, 0, vp[2], vp[3], GL_BGRA, GL_UNSIGNED_BYTE, img);
    sprintf(fname, "blit%iA_src.tga", blitnum);
    crDumpNamedTGA(fname, vp[2], vp[3], img);

    gl->BindTexture(GL_TEXTURE_2D, dtex);
    gl->GetTexImage(GL_TEXTURE_2D, dlev, GL_BGRA, GL_UNSIGNED_BYTE, img);
    sprintf(fname, "blit%iB_dst.tga", blitnum);
    crDumpNamedTGA(fname, dstw, dsth, img);
    gl->BindTexture(GL_TEXTURE_2D, otex);
#endif

    if (srcY0 > srcY1)
    {
        /* work around Intel driver bug on Linux host  */
        if (1 || dstY0 > dstY1)
        {
            /* use srcY1 < srcY2 && dstY1 < dstY2 whenever possible to avoid GPU driver bugs */
            int32_t tmp = srcY0;
            srcY0 = srcY1;
            srcY1 = tmp;
            tmp = dstY0;
            dstY0 = dstY1;
            dstY1 = tmp;
        }
    }

    if (srcX0 > srcX1)
    {
        if (dstX0 > dstX1)
        {
            /* use srcX1 < srcX2 && dstX1 < dstX2 whenever possible to avoid GPU driver bugs */
            int32_t tmp = srcX0;
            srcX0 = srcX1;
            srcX1 = tmp;
            tmp = dstX0;
            dstX0 = dstX1;
            dstX1 = tmp;
        }
    }

    if (cr_server.fBlitterMode)
    {
        fTryBlitter = true;
    }

    if (fTryBlitter)
    {
        int rc = crServerVBoxBlitterBlitCurrentCtx(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        if (RT_SUCCESS(rc))
            goto my_exit;
    }

    if (ctx->viewport.scissorTest)
        cr_server.head_spu->dispatch_table.Disable(GL_SCISSOR_TEST);

    cr_server.head_spu->dispatch_table.BlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1,
                                                          dstX0, dstY0, dstX1, dstY1,
                                                          mask, filter);

    if (ctx->viewport.scissorTest)
        cr_server.head_spu->dispatch_table.Enable(GL_SCISSOR_TEST);


my_exit:

//#ifdef CR_CHECK_BLITS
//    crDbgDumpTexImage2D("<== src tex:", GL_TEXTURE_2D, rtex, true);
//    crDbgDumpTexImage2D("<== dst tex:", GL_TEXTURE_2D, dtex, true);
//#endif
#ifdef CR_DUMP_BLITS
    gl->BindTexture(GL_TEXTURE_2D, dtex);
    gl->GetTexImage(GL_TEXTURE_2D, dlev, GL_BGRA, GL_UNSIGNED_BYTE, img);
    sprintf(fname, "blit%iC_res.tga", blitnum);
    crDumpNamedTGA(fname, dstw, dsth, img);
    gl->BindTexture(GL_TEXTURE_2D, otex);
    crFree(img);
#endif
    return;
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDrawBuffer( GLenum mode )
{
    crStateDrawBuffer( mode );

    if (!crStateGetCurrent()->framebufferobject.drawFB)
    {
        if (mode == GL_FRONT || mode == GL_FRONT_LEFT || mode == GL_FRONT_RIGHT)
            cr_server.curClient->currentMural->bFbDraw = GL_TRUE;

        if (crServerIsRedirectedToFBO()
                && cr_server.curClient->currentMural->aidFBOs[0])
        {
            CRMuralInfo *mural = cr_server.curClient->currentMural;
            GLint iBufferNeeded = -1;
            switch (mode)
            {
                case GL_BACK:
                case GL_BACK_LEFT:
                case GL_BACK_RIGHT:
                    mode = GL_COLOR_ATTACHMENT0;
                    iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                    break;
                case GL_FRONT:
                case GL_FRONT_LEFT:
                case GL_FRONT_RIGHT:
                    mode = GL_COLOR_ATTACHMENT0;
                    iBufferNeeded = CR_SERVER_FBO_FB_IDX(mural);
                    break;
                case GL_NONE:
                    crDebug("DrawBuffer: GL_NONE");
                    break;
                case GL_AUX0:
                    crDebug("DrawBuffer: GL_AUX0");
                    break;
                case GL_AUX1:
                    crDebug("DrawBuffer: GL_AUX1");
                    break;
                case GL_AUX2:
                    crDebug("DrawBuffer: GL_AUX2");
                    break;
                case GL_AUX3:
                    crDebug("DrawBuffer: GL_AUX3");
                    break;
                case GL_LEFT:
                    crWarning("DrawBuffer: GL_LEFT not supported properly");
                    mode = GL_COLOR_ATTACHMENT0;
                    iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                    break;
                case GL_RIGHT:
                    crWarning("DrawBuffer: GL_RIGHT not supported properly");
                    mode = GL_COLOR_ATTACHMENT0;
                    iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                    break;
                case GL_FRONT_AND_BACK:
                    crWarning("DrawBuffer: GL_FRONT_AND_BACK not supported properly");
                    mode = GL_COLOR_ATTACHMENT0;
                    iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                    break;
                default:
                    crWarning("DrawBuffer: unexpected mode! 0x%x", mode);
                    iBufferNeeded = mural->iCurDrawBuffer;
                    break;
            }

            if (iBufferNeeded != mural->iCurDrawBuffer)
            {
                mural->iCurDrawBuffer = iBufferNeeded;
                cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_DRAW_FRAMEBUFFER, CR_SERVER_FBO_FOR_IDX(mural, iBufferNeeded));
            }
        }
    }

    cr_server.head_spu->dispatch_table.DrawBuffer( mode );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchDrawBuffers( GLsizei n, const GLenum* bufs )
{
    if (n == 1)
    {
        crServerDispatchDrawBuffer( bufs[0] );
    }
    else
    {
        /** @todo State tracker. */
        cr_server.head_spu->dispatch_table.DrawBuffers( n, bufs );
    }
}

void SERVER_DISPATCH_APIENTRY crServerDispatchReadBuffer( GLenum mode )
{
    crStateReadBuffer( mode );

    if (crServerIsRedirectedToFBO()
            && cr_server.curClient->currentMural->aidFBOs[0]
            && !crStateGetCurrent()->framebufferobject.readFB)
    {
        CRMuralInfo *mural = cr_server.curClient->currentMural;
        GLint iBufferNeeded = -1;
        switch (mode)
        {
            case GL_BACK:
            case GL_BACK_LEFT:
            case GL_BACK_RIGHT:
                mode = GL_COLOR_ATTACHMENT0;
                iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                break;
            case GL_FRONT:
            case GL_FRONT_LEFT:
            case GL_FRONT_RIGHT:
                mode = GL_COLOR_ATTACHMENT0;
                iBufferNeeded = CR_SERVER_FBO_FB_IDX(mural);
                break;
            case GL_NONE:
                crDebug("ReadBuffer: GL_NONE");
                break;
            case GL_AUX0:
                crDebug("ReadBuffer: GL_AUX0");
                break;
            case GL_AUX1:
                crDebug("ReadBuffer: GL_AUX1");
                break;
            case GL_AUX2:
                crDebug("ReadBuffer: GL_AUX2");
                break;
            case GL_AUX3:
                crDebug("ReadBuffer: GL_AUX3");
                break;
            case GL_LEFT:
                crWarning("ReadBuffer: GL_LEFT not supported properly");
                mode = GL_COLOR_ATTACHMENT0;
                iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                break;
            case GL_RIGHT:
                crWarning("ReadBuffer: GL_RIGHT not supported properly");
                mode = GL_COLOR_ATTACHMENT0;
                iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                break;
            case GL_FRONT_AND_BACK:
                crWarning("ReadBuffer: GL_FRONT_AND_BACK not supported properly");
                mode = GL_COLOR_ATTACHMENT0;
                iBufferNeeded = CR_SERVER_FBO_BB_IDX(mural);
                break;
            default:
                crWarning("ReadBuffer: unexpected mode! 0x%x", mode);
                iBufferNeeded = mural->iCurDrawBuffer;
                break;
        }

        Assert(CR_SERVER_FBO_FOR_IDX(mural, mural->iCurReadBuffer));
        if (iBufferNeeded != mural->iCurReadBuffer)
        {
            mural->iCurReadBuffer = iBufferNeeded;
            cr_server.head_spu->dispatch_table.BindFramebufferEXT(GL_READ_FRAMEBUFFER, CR_SERVER_FBO_FOR_IDX(mural, iBufferNeeded));
        }
    }
    cr_server.head_spu->dispatch_table.ReadBuffer( mode );
}

GLenum SERVER_DISPATCH_APIENTRY crServerDispatchGetError( void )
{
    GLenum retval, err;
    CRContext *ctx = crStateGetCurrent();
    retval = ctx->error;

    err = cr_server.head_spu->dispatch_table.GetError();
    if (retval == GL_NO_ERROR)
        retval = err;
    else
        ctx->error = GL_NO_ERROR;

    /* our impl has a single error flag, so we just loop here to reset all error flags to no_error */
    while (err != GL_NO_ERROR)
        err = cr_server.head_spu->dispatch_table.GetError();

    crServerReturnValue( &retval, sizeof(retval) );
    return retval; /* WILL PROBABLY BE IGNORED */
}

void SERVER_DISPATCH_APIENTRY
crServerMakeTmpCtxCurrent( GLint window, GLint nativeWindow, GLint context )
{
    CRContext *pCtx = crStateGetCurrent();
    CRContext *pCurCtx = NULL;
    GLuint idDrawFBO = 0, idReadFBO = 0;
    int fDoPrePostProcess = 0;

    if (pCtx)
    {
        CRMuralInfo *pCurrentMural = cr_server.currentMural;

        pCurCtx = cr_server.currentCtxInfo ? cr_server.currentCtxInfo->pContext : cr_server.MainContextInfo.pContext;
        Assert(pCurCtx == pCtx);

        if (!context)
        {
            if (pCurrentMural)
            {
                Assert(cr_server.currentCtxInfo);
                context = cr_server.currentCtxInfo->SpuContext > 0 ? cr_server.currentCtxInfo->SpuContext : cr_server.MainContextInfo.SpuContext;
                window = pCurrentMural->spuWindow;
            }
            else
            {
                CRMuralInfo * pDummy;
                Assert(!cr_server.currentCtxInfo);
                pDummy = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
                context = cr_server.MainContextInfo.SpuContext;
                window = pDummy->spuWindow;
            }


            fDoPrePostProcess = -1;
        }
        else
        {
            fDoPrePostProcess = 1;
        }

        if (pCurrentMural)
        {
            idDrawFBO = CR_SERVER_FBO_FOR_IDX(pCurrentMural, pCurrentMural->iCurDrawBuffer);
            idReadFBO = CR_SERVER_FBO_FOR_IDX(pCurrentMural, pCurrentMural->iCurReadBuffer);
        }
        else
        {
            idDrawFBO = 0;
            idReadFBO = 0;
        }
    }
    else
    {
        /* this is a GUI thread, so no need to do anything here */
    }

    if (fDoPrePostProcess > 0)
        crStateSwitchPrepare(NULL, pCurCtx, idDrawFBO, idReadFBO);

    cr_server.head_spu->dispatch_table.MakeCurrent( window, nativeWindow, context);

    if (fDoPrePostProcess < 0)
        crStateSwitchPostprocess(pCurCtx, NULL, idDrawFBO, idReadFBO);
}

void crServerInitTmpCtxDispatch()
{
    MakeCurrentFunc_t pfnMakeCurrent;

    crSPUInitDispatchTable(&cr_server.TmpCtxDispatch);
    crSPUCopyDispatchTable(&cr_server.TmpCtxDispatch, &cr_server.head_spu->dispatch_table);
    cr_server.TmpCtxDispatch.MakeCurrent = crServerMakeTmpCtxCurrent;

    pfnMakeCurrent = crServerMakeTmpCtxCurrent;
    cr_server.head_spu->dispatch_table.ChromiumParametervCR(GL_HH_SET_TMPCTX_MAKE_CURRENT, GL_BYTE, sizeof (void*), &pfnMakeCurrent);

}

/* dump stuff */
#ifdef VBOX_WITH_CRSERVER_DUMPER

# ifndef VBOX_WITH_CRDUMPER
#  error "VBOX_WITH_CRDUMPER undefined!"
# endif

/* first four bits are buffer dump config
 * second four bits are texture dump config
 * config flags:
 * 1 - blit on enter
 * 2 - blit on exit
 *
 *
 * Example:
 *
 * 0x03 - dump buffer on enter and exit
 * 0x22 - dump texture and buffer on exit */

int64_t g_CrDbgDumpPid = 0;
unsigned long g_CrDbgDumpEnabled = 0;
unsigned long g_CrDbgDumpDraw = 0
#if 0
        | CR_SERVER_DUMP_F_COMPILE_SHADER
        | CR_SERVER_DUMP_F_LINK_PROGRAM
#endif
        ;
#if 0
        | CR_SERVER_DUMP_F_DRAW_BUFF_ENTER
        | CR_SERVER_DUMP_F_DRAW_BUFF_LEAVE
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_UNIFORMS_ENTER
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_ATTRIBS_ENTER
        | CR_SERVER_DUMP_F_DRAW_TEX_ENTER
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_ENTER
        | CR_SERVER_DUMP_F_DRAW_STATE_ENTER
        | CR_SERVER_DUMP_F_SWAPBUFFERS_ENTER
        | CR_SERVER_DUMP_F_DRAWEL
        | CR_SERVER_DUMP_F_SHADER_SOURCE
        ;
#endif
unsigned long g_CrDbgDumpDrawFramesSettings = CR_SERVER_DUMP_F_DRAW_BUFF_ENTER
        | CR_SERVER_DUMP_F_DRAW_BUFF_LEAVE
        | CR_SERVER_DUMP_F_DRAW_TEX_ENTER
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_ENTER
        | CR_SERVER_DUMP_F_COMPILE_SHADER
        | CR_SERVER_DUMP_F_LINK_PROGRAM
        | CR_SERVER_DUMP_F_SWAPBUFFERS_ENTER;
unsigned long g_CrDbgDumpDrawFramesAppliedSettings = 0;
unsigned long g_CrDbgDumpDrawFramesSavedInitSettings = 0;
unsigned long g_CrDbgDumpDrawFramesCount = 0;

uint32_t g_CrDbgDumpDrawCount = 0;
uint32_t g_CrDbgDumpDumpOnCount = 10;
uint32_t g_CrDbgDumpDumpOnCountEnabled = 0;
uint32_t g_CrDbgDumpDumpOnCountPerform = 0;
uint32_t g_CrDbgDumpDrawFlags = CR_SERVER_DUMP_F_COMPILE_SHADER
        | CR_SERVER_DUMP_F_SHADER_SOURCE
        | CR_SERVER_DUMP_F_COMPILE_SHADER
        | CR_SERVER_DUMP_F_LINK_PROGRAM
        | CR_SERVER_DUMP_F_DRAW_BUFF_ENTER
        | CR_SERVER_DUMP_F_DRAW_BUFF_LEAVE
        | CR_SERVER_DUMP_F_DRAW_TEX_ENTER
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_UNIFORMS_ENTER
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_ATTRIBS_ENTER
        | CR_SERVER_DUMP_F_DRAW_PROGRAM_ENTER
        | CR_SERVER_DUMP_F_DRAW_STATE_ENTER
        | CR_SERVER_DUMP_F_SWAPBUFFERS_ENTER
        | CR_SERVER_DUMP_F_DRAWEL
        | CR_SERVER_DUMP_F_TEXPRESENT;

void crServerDumpCheckTerm()
{
    if (!CrBltIsInitialized(&cr_server.RecorderBlitter))
        return;

    CrBltTerm(&cr_server.RecorderBlitter);
}

int crServerDumpCheckInit()
{
    int rc;
    CR_BLITTER_WINDOW BltWin;
    CR_BLITTER_CONTEXT BltCtx;
    CRMuralInfo *pBlitterMural;

    if (!CrBltIsInitialized(&cr_server.RecorderBlitter))
    {
        pBlitterMural = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
        if (!pBlitterMural)
        {
            crWarning("crServerGetDummyMural failed");
            return VERR_GENERAL_FAILURE;
        }

        crServerVBoxBlitterWinInit(&BltWin, pBlitterMural);
        crServerVBoxBlitterCtxInit(&BltCtx, &cr_server.MainContextInfo);

        rc = CrBltInit(&cr_server.RecorderBlitter, &BltCtx, true, true, NULL, &cr_server.TmpCtxDispatch);
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrBltInit failed rc %d", rc);
            return rc;
        }

        rc = CrBltMuralSetCurrentInfo(&cr_server.RecorderBlitter, &BltWin);
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrBltMuralSetCurrentInfo failed rc %d", rc);
            return rc;
        }
    }

#if 0
    crDmpDbgPrintInit(&cr_server.DbgPrintDumper);
    cr_server.pDumper = &cr_server.DbgPrintDumper.Base;
#else
    if (!crDmpHtmlIsInited(&cr_server.HtmlDumper))
    {
        static int cCounter = 0;
//    crDmpHtmlInit(&cr_server.HtmlDumper, "S:\\projects\\virtualbox\\3d\\dumps\\1", "index.html");
        crDmpHtmlInitF(&cr_server.HtmlDumper, "/Users/oracle-mac/vbox/dump/1", "index%d.html", cCounter);
        cr_server.pDumper = &cr_server.HtmlDumper.Base;
        ++cCounter;
    }
#endif

    crRecInit(&cr_server.Recorder, &cr_server.RecorderBlitter, &cr_server.TmpCtxDispatch, cr_server.pDumper);
    return VINF_SUCCESS;
}

void crServerDumpShader(GLint id)
{
    CRContext *ctx = crStateGetCurrent();
    crRecDumpShader(&cr_server.Recorder, ctx, id, 0);
}

void crServerDumpProgram(GLint id)
{
    CRContext *ctx = crStateGetCurrent();
    crRecDumpProgram(&cr_server.Recorder, ctx, id, 0);
}

void crServerDumpCurrentProgram()
{
    CRContext *ctx = crStateGetCurrent();
    crRecDumpCurrentProgram(&cr_server.Recorder, ctx);
}

void crServerDumpRecompileDumpCurrentProgram()
{
    crDmpStrF(cr_server.Recorder.pDumper, "==Dump(1)==");
    crServerRecompileCurrentProgram();
    crServerDumpCurrentProgramUniforms();
    crServerDumpCurrentProgramAttribs();
    crDmpStrF(cr_server.Recorder.pDumper, "Done Dump(1)");
    crServerRecompileCurrentProgram();
    crDmpStrF(cr_server.Recorder.pDumper, "Dump(2)");
    crServerRecompileCurrentProgram();
    crServerDumpCurrentProgramUniforms();
    crServerDumpCurrentProgramAttribs();
    crDmpStrF(cr_server.Recorder.pDumper, "Done Dump(2)");
}

void crServerRecompileCurrentProgram()
{
    CRContext *ctx = crStateGetCurrent();
    crRecRecompileCurrentProgram(&cr_server.Recorder, ctx);
}

void crServerDumpCurrentProgramUniforms()
{
    CRContext *ctx = crStateGetCurrent();
    crDmpStrF(cr_server.Recorder.pDumper, "==Uniforms==");
    crRecDumpCurrentProgramUniforms(&cr_server.Recorder, ctx);
    crDmpStrF(cr_server.Recorder.pDumper, "==Done Uniforms==");
}

void crServerDumpCurrentProgramAttribs()
{
    CRContext *ctx = crStateGetCurrent();
    crDmpStrF(cr_server.Recorder.pDumper, "==Attribs==");
    crRecDumpCurrentProgramAttribs(&cr_server.Recorder, ctx);
    crDmpStrF(cr_server.Recorder.pDumper, "==Done Attribs==");
}

void crServerDumpState()
{
    CRContext *ctx = crStateGetCurrent();
    crRecDumpGlGetState(&cr_server.Recorder, ctx);
    crRecDumpGlEnableState(&cr_server.Recorder, ctx);
}

void crServerDumpDrawel(const char*pszFormat, ...)
{
    CRContext *ctx = crStateGetCurrent();
    va_list pArgList;
    va_start(pArgList, pszFormat);
    crRecDumpVertAttrV(&cr_server.Recorder, ctx, pszFormat, pArgList);
    va_end(pArgList);
}

void crServerDumpDrawelv(GLuint idx, const char*pszElFormat, uint32_t cbEl, const void *pvVal, uint32_t cVal)
{
    CRContext *ctx = crStateGetCurrent();
    crRecDumpVertAttrv(&cr_server.Recorder, ctx, idx, pszElFormat, cbEl, pvVal, cVal);
}

void crServerDumpBuffer(int idx)
{
    CRContextInfo *pCtxInfo = cr_server.currentCtxInfo;
    CRContext *ctx = crStateGetCurrent();
    GLint idFBO;
    GLint idTex;
    VBOXVR_TEXTURE RedirTex;
    int rc = crServerDumpCheckInit();
    idx = idx >= 0 ? idx : crServerMuralFBOIdxFromBufferName(cr_server.currentMural, pCtxInfo->pContext->buffer.drawBuffer);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerDumpCheckInit failed, rc %d", rc);
        return;
    }

    if (idx < 0)
    {
        crWarning("neg idx, unsupported");
        return;
    }

    idFBO = CR_SERVER_FBO_FOR_IDX(cr_server.currentMural, idx);
    idTex = CR_SERVER_FBO_TEX_FOR_IDX(cr_server.currentMural, idx);

    RedirTex.width = cr_server.currentMural->fboWidth;
    RedirTex.height = cr_server.currentMural->fboHeight;
    RedirTex.target = GL_TEXTURE_2D;
    RedirTex.hwid = idTex;

    crRecDumpBuffer(&cr_server.Recorder, ctx, idFBO, idTex ? &RedirTex : NULL);
}

void crServerDumpTexture(const VBOXVR_TEXTURE *pTex)
{
    CRContextInfo *pCtxInfo = cr_server.currentCtxInfo;
    CR_BLITTER_WINDOW BltWin;
    CR_BLITTER_CONTEXT BltCtx;
    CRContext *ctx = crStateGetCurrent();
    int rc = crServerDumpCheckInit();
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerDumpCheckInit failed, rc %d", rc);
        return;
    }

    crServerVBoxBlitterWinInit(&BltWin, cr_server.currentMural);
    crServerVBoxBlitterCtxInit(&BltCtx, pCtxInfo);

    crRecDumpTextureF(&cr_server.Recorder, pTex, &BltCtx, &BltWin, "Tex (%d x %d), hwid (%d) target %#x", pTex->width, pTex->height, pTex->hwid, pTex->target);
}

void crServerDumpTextures()
{
    CRContextInfo *pCtxInfo = cr_server.currentCtxInfo;
    CRContext *ctx = crStateGetCurrent();
    int rc = crServerDumpCheckInit();
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerDumpCheckInit failed, rc %d", rc);
        return;
    }

    crRecDumpTextures(&cr_server.Recorder, ctx);
}

void crServerDumpFilterOpLeave(unsigned long event, CR_DUMPER *pDumper)
{
    if (CR_SERVER_DUMP_F_DRAW_LEAVE_ALL & event)
    {
        g_CrDbgDumpDumpOnCountPerform = 0;
    }
}

bool crServerDumpFilterOpEnter(unsigned long event, CR_DUMPER *pDumper)
{
    if ((CR_SERVER_DUMP_F_SWAPBUFFERS_ENTER & event)
            || (CR_SERVER_DUMP_F_TEXPRESENT & event))
    {
        if (g_CrDbgDumpDumpOnCountEnabled == 1)
            g_CrDbgDumpDumpOnCountEnabled = 2;
        else if (g_CrDbgDumpDumpOnCountEnabled)
        {
            g_CrDbgDumpDumpOnCountEnabled = 0;
            if (cr_server.pDumper == &cr_server.HtmlDumper.Base)
            {
                crDmpHtmlTerm(&cr_server.HtmlDumper);
                cr_server.pDumper = NULL;
            }
        }

        g_CrDbgDumpDrawCount = 0;
    }
    else if (CR_SERVER_DUMP_F_DRAW_ENTER_ALL & event)
    {
        if (g_CrDbgDumpDumpOnCountEnabled == 2)
        {
            if (g_CrDbgDumpDumpOnCount == g_CrDbgDumpDrawCount)
            {
                g_CrDbgDumpDumpOnCountPerform = 1;
            }
            ++g_CrDbgDumpDrawCount;
        }
    }
    if (g_CrDbgDumpDumpOnCountPerform)
    {
        if (g_CrDbgDumpDrawFlags & event)
            return true;
    }
    return CR_SERVER_DUMP_DEFAULT_FILTER_OP(event);
}

bool crServerDumpFilterDmp(unsigned long event, CR_DUMPER *pDumper)
{
    if (g_CrDbgDumpDumpOnCountPerform)
    {
        if (g_CrDbgDumpDrawFlags & event)
            return true;
    }
    return CR_SERVER_DUMP_DEFAULT_FILTER_DMP(event);
}

void crServerDumpFramesCheck()
{
    if (!g_CrDbgDumpDrawFramesCount)
        return;

    if (!g_CrDbgDumpDrawFramesAppliedSettings)
    {
        if (!g_CrDbgDumpDrawFramesSettings)
        {
            crWarning("g_CrDbgDumpDrawFramesSettings is NULL, bump will not be started");
            g_CrDbgDumpDrawFramesCount = 0;
            return;
        }

        g_CrDbgDumpDrawFramesSavedInitSettings = g_CrDbgDumpDraw;
        g_CrDbgDumpDrawFramesAppliedSettings = g_CrDbgDumpDrawFramesSettings;
        g_CrDbgDumpDraw = g_CrDbgDumpDrawFramesSettings;
        crDmpStrF(cr_server.Recorder.pDumper, "***Starting draw dump for %d frames, settings(0x%x)", g_CrDbgDumpDrawFramesCount, g_CrDbgDumpDraw);
        return;
    }

    --g_CrDbgDumpDrawFramesCount;

    if (!g_CrDbgDumpDrawFramesCount)
    {
        crDmpStrF(cr_server.Recorder.pDumper, "***Stop draw dump");
        g_CrDbgDumpDraw = g_CrDbgDumpDrawFramesSavedInitSettings;
        g_CrDbgDumpDrawFramesAppliedSettings = 0;
    }
}
#endif

GLvoid crServerSpriteCoordReplEnable(GLboolean fEnable)
{
    CRContext *g = crStateGetCurrent();
    CRTextureState *t = &(g->texture);
    GLuint curTextureUnit = t->curTextureUnit;
    GLuint curTextureUnitRestore = curTextureUnit;
    GLuint i;

    for (i = 0; i < g->limits.maxTextureUnits; ++i)
    {
        if (g->point.coordReplacement[i])
        {
            if (i != curTextureUnit)
            {
                curTextureUnit = i;
                cr_server.head_spu->dispatch_table.ActiveTextureARB( i + GL_TEXTURE0_ARB );
            }

            cr_server.head_spu->dispatch_table.TexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, (GLint)fEnable);
        }
    }

    if (curTextureUnit != curTextureUnitRestore)
    {
        cr_server.head_spu->dispatch_table.ActiveTextureARB( curTextureUnitRestore + GL_TEXTURE0_ARB );
    }
}

GLvoid SERVER_DISPATCH_APIENTRY crServerDispatchDrawArrays(GLenum mode, GLint first, GLsizei count)
{
#ifdef DEBUG
    GLenum status = cr_server.head_spu->dispatch_table.CheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
    Assert(GL_FRAMEBUFFER_COMPLETE == status);
#endif
    if (mode == GL_POINTS)
        crServerSpriteCoordReplEnable(GL_TRUE);
    CR_SERVER_DUMP_DRAW_ENTER();
    CR_GLERR_CHECK(cr_server.head_spu->dispatch_table.DrawArrays(mode, first, count););
    CR_SERVER_DUMP_DRAW_LEAVE();
    if (mode == GL_POINTS)
        crServerSpriteCoordReplEnable(GL_FALSE);
}

GLvoid SERVER_DISPATCH_APIENTRY crServerDispatchDrawElements(GLenum mode,  GLsizei count,  GLenum type,  const GLvoid * indices)
{
#ifdef DEBUG
    GLenum status = cr_server.head_spu->dispatch_table.CheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
    Assert(GL_FRAMEBUFFER_COMPLETE == status);
#endif
    if (mode == GL_POINTS)
        crServerSpriteCoordReplEnable(GL_TRUE);
    CR_SERVER_DUMP_DRAW_ENTER();
    CR_GLERR_CHECK(cr_server.head_spu->dispatch_table.DrawElements(mode, count, type, indices););
    CR_SERVER_DUMP_DRAW_LEAVE();
    if (mode == GL_POINTS)
        crServerSpriteCoordReplEnable(GL_FALSE);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchEnd( void )
{
    CRContext *g = crStateGetCurrent();
    GLenum mode = g->current.mode;

    crStateEnd();
    cr_server.head_spu->dispatch_table.End();

    CR_SERVER_DUMP_DRAW_LEAVE();

    if (mode == GL_POINTS)
        crServerSpriteCoordReplEnable(GL_FALSE);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchBegin(GLenum mode)
{
#ifdef DEBUG
    CRContext *ctx = crStateGetCurrent();
    SPUDispatchTable *gl = &cr_server.head_spu->dispatch_table;

    if (ctx->program.vpProgramBinding)
    {
        AssertRelease(ctx->program.currentVertexProgram);

        if (ctx->program.currentVertexProgram->isARBprogram)
        {
            GLint pid=-1;
            gl->GetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &pid);

            if (pid != ctx->program.currentVertexProgram->id)
            {
                crWarning("pid(%d) != ctx->program.currentVertexProgram->id(%d)", pid, ctx->program.currentVertexProgram->id);
            }
            AssertRelease(pid == ctx->program.currentVertexProgram->id);
        }
        else
        {
            GLint pid=-1;

            gl->GetIntegerv(GL_VERTEX_PROGRAM_BINDING_NV, &pid);
            if (pid != ctx->program.currentVertexProgram->id)
            {
                crWarning("pid(%d) != ctx->program.currentVertexProgram->id(%d)", pid, ctx->program.currentVertexProgram->id);
            }
            AssertRelease(pid == ctx->program.currentVertexProgram->id);
        }
    }
    else if (ctx->glsl.activeProgram)
    {
        GLint pid=-1;

        gl->GetIntegerv(GL_CURRENT_PROGRAM, &pid);
        //crDebug("pid %i, state: id %i, hwid %i", pid, ctx->glsl.activeProgram->id, ctx->glsl.activeProgram->hwid);
        if (pid != ctx->glsl.activeProgram->hwid)
        {
            crWarning("pid(%d) != ctx->glsl.activeProgram->hwid(%d)", pid, ctx->glsl.activeProgram->hwid);
        }
        AssertRelease(pid == ctx->glsl.activeProgram->hwid);
    }
#endif

    if (mode == GL_POINTS)
        crServerSpriteCoordReplEnable(GL_TRUE);

    CR_SERVER_DUMP_DRAW_ENTER();

    crStateBegin(mode);
    cr_server.head_spu->dispatch_table.Begin(mode);
}

