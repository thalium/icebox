/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_error.h" 
#include "cr_mem.h"
#include "cr_pixeldata.h"
#include "server_dispatch.h"
#include "server.h"

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetTexImage(GLenum target, GLint level, GLenum format,
                            GLenum type, GLvoid * pixels)
{
    GLsizei width, height, depth, size;
    GLvoid *buffer = NULL;


#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        /*pixels are actually a pointer to location of 8byte network pointer in hgcm buffer
          regardless of guest/host bitness we're using only 4lower bytes as there're no
          pbo>4gb (yet?)
         */
        pbo_offset = (GLvoid*) ((uintptr_t) *((GLint*)pixels));

        cr_server.head_spu->dispatch_table.GetTexImage(target, level, format, type, pbo_offset);

        return;
    }
#endif

    cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);
    cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH, &depth);

    size = crTextureSize(format, type, width, height, depth);

#if 0
    {
        CRContext *ctx = crStateGetCurrent();
        CRTextureObj *tobj;
        CRTextureLevel *tl;
        GLint id;

        crDebug("GetTexImage: %d, %i, %d, %d", target, level, format, type);
        crDebug("===StateTracker===");
        crDebug("Current TU: %i", ctx->texture.curTextureUnit);

        if (target==GL_TEXTURE_2D)
        {
            tobj = ctx->texture.unit[ctx->texture.curTextureUnit].currentTexture2D;
            CRASSERT(tobj);
            tl = &tobj->level[0][level];
            crDebug("Texture %i(hw %i), w=%i, h=%i", tobj->id, tobj->hwid, tl->width, tl->height, tl->depth);
        }
        else
        {
            crDebug("Not 2D tex");
        }

        crDebug("===GPU===");
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_ACTIVE_TEXTURE, &id);
        crDebug("Current TU: %i", id);
        if (target==GL_TEXTURE_2D)
        {
            cr_server.head_spu->dispatch_table.GetIntegerv(GL_TEXTURE_BINDING_2D, &id);
            crDebug("Texture: %i, w=%i, h=%i, d=%i", id, width, height, depth);
        }
    }
#endif

    if (size && (buffer = crCalloc(size))) {
        /* Note, the other pixel PACK parameters (default values) should
         * be OK at this point.
         */
        cr_server.head_spu->dispatch_table.PixelStorei(GL_PACK_ALIGNMENT, 1);
        cr_server.head_spu->dispatch_table.GetTexImage(target, level, format, type, buffer);
        crServerReturnValue( buffer, size );
        crFree(buffer);
    }
    else {
        /* need to return _something_ to avoid blowing up */
        GLuint dummy = 0;
        crServerReturnValue( (GLvoid *) &dummy, sizeof(dummy) );
    }
}


#if CR_ARB_texture_compression

void SERVER_DISPATCH_APIENTRY
crServerDispatchGetCompressedTexImageARB(GLenum target, GLint level,
                                         GLvoid *img)
{
    GLint size;
    GLvoid *buffer=NULL;

#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        pbo_offset = (GLvoid*) ((uintptr_t) *((GLint*)img));

        cr_server.head_spu->dispatch_table.GetCompressedTexImageARB(target, level, pbo_offset);

        return;
    }
#endif

    cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);

    if (size && (buffer = crCalloc(size))) {
        /* XXX the pixel PACK parameter should be OK at this point */
        cr_server.head_spu->dispatch_table.GetCompressedTexImageARB(target, level, buffer);
        crServerReturnValue( buffer, size );
        crFree(buffer);
    }
    else {
        /* need to return _something_ to avoid blowing up */
        GLuint dummy = 0;
        crServerReturnValue( (GLvoid *) &dummy, sizeof(dummy) );
    }
}

#endif /* CR_ARB_texture_compression */

void SERVER_DISPATCH_APIENTRY crServerDispatchGetPolygonStipple( GLubyte * mask )
{
#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        pbo_offset = (GLubyte*) ((uintptr_t) *((GLint*)mask));

        cr_server.head_spu->dispatch_table.GetPolygonStipple(pbo_offset);
    }
    else
#endif
    {    
        GLubyte local_mask[128];

        memset(local_mask, 0, sizeof(local_mask));

        cr_server.head_spu->dispatch_table.GetPolygonStipple( local_mask );
        crServerReturnValue( &(local_mask[0]), 128*sizeof(GLubyte) );
    }
}
