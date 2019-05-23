/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "unpack_extend.h"
#include "unpacker.h"
#include "cr_glstate.h"
/**
 * \mainpage Unpacker 
 *
 * \section UnpackerIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The unpacker module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */

void crUnpackExtendVertexPointer(void)
{
    GLint size = READ_DATA( 8, GLint );
    GLenum type = READ_DATA( 12, GLenum );
    GLsizei stride = READ_DATA( 16, GLsizei );
    GLintptrARB pointer = (GLintptrARB) READ_DATA( 20, GLuint );
    cr_unpackDispatch.VertexPointer( size, type, stride, (void *) pointer );
}

void crUnpackExtendTexCoordPointer(void)
{
    GLint size = READ_DATA( 8, GLint );
    GLenum type = READ_DATA( 12, GLenum );
    GLsizei stride = READ_DATA( 16, GLsizei );
    GLintptrARB pointer = READ_DATA( 20, GLuint );
    cr_unpackDispatch.TexCoordPointer( size, type, stride, (void *) pointer );
}

void crUnpackExtendNormalPointer(void)
{
    GLenum type = READ_DATA( 8, GLenum );
    GLsizei stride = READ_DATA( 12, GLsizei );
    GLintptrARB pointer = READ_DATA( 16, GLuint );
    cr_unpackDispatch.NormalPointer( type, stride, (void *) pointer );
}

void crUnpackExtendIndexPointer(void)
{
    GLenum type = READ_DATA( 8, GLenum );
    GLsizei stride = READ_DATA( 12, GLsizei );
    GLintptrARB pointer = READ_DATA( 16, GLuint );
    cr_unpackDispatch.IndexPointer( type, stride, (void *) pointer );
}

void crUnpackExtendEdgeFlagPointer(void)
{
    GLsizei stride = READ_DATA( 8, GLsizei );
    GLintptrARB pointer = READ_DATA( 12, GLuint );
    cr_unpackDispatch.EdgeFlagPointer( stride, (void *) pointer );
}

void crUnpackExtendColorPointer(void)
{
    GLint size = READ_DATA( 8, GLint );
    GLenum type = READ_DATA( 12, GLenum );
    GLsizei stride = READ_DATA( 16, GLsizei );
    GLintptrARB pointer = READ_DATA( 20, GLuint );
    cr_unpackDispatch.ColorPointer( size, type, stride, (void *) pointer );
}

void crUnpackExtendFogCoordPointerEXT(void)
{
    GLenum type = READ_DATA( 8, GLenum );
    GLsizei stride = READ_DATA( 12, GLsizei );
    GLintptrARB pointer = READ_DATA( 16, GLuint );
    cr_unpackDispatch.FogCoordPointerEXT( type, stride, (void *) pointer );
}

void crUnpackExtendSecondaryColorPointerEXT(void)
{
    GLint size = READ_DATA( 8, GLint );
    GLenum type = READ_DATA( 12, GLenum );
    GLsizei stride = READ_DATA( 16, GLsizei );
    GLintptrARB pointer = READ_DATA( 20, GLuint );
    cr_unpackDispatch.SecondaryColorPointerEXT( size, type, stride, (void *) pointer );
}

void crUnpackExtendVertexAttribPointerARB(void)
{
    GLuint index = READ_DATA( 8, GLuint);
    GLint size = READ_DATA( 12, GLint );
    GLenum type = READ_DATA( 16, GLenum );
    GLboolean normalized = READ_DATA( 20, GLboolean );
    GLsizei stride = READ_DATA( 24, GLsizei );
    GLintptrARB pointer = READ_DATA( 28, GLuint );
    cr_unpackDispatch.VertexAttribPointerARB( index, size, type, normalized, stride, (void *) pointer );
}

void crUnpackExtendVertexAttribPointerNV(void)
{
    GLuint index = READ_DATA( 8, GLuint);
    GLint size = READ_DATA( 12, GLint );
    GLenum type = READ_DATA( 16, GLenum );
    GLsizei stride = READ_DATA( 20, GLsizei );
    GLintptrARB pointer = READ_DATA( 24, GLuint );
    cr_unpackDispatch.VertexAttribPointerNV( index, size, type, stride, (void *) pointer );
}

void crUnpackExtendInterleavedArrays(void)
{
    GLenum format = READ_DATA( 8, GLenum );
    GLsizei stride = READ_DATA( 12, GLsizei );
    GLintptrARB pointer = READ_DATA( 16, GLuint );
    cr_unpackDispatch.InterleavedArrays( format, stride, (void *) pointer );
}

void crUnpackExtendDrawElements(void)
{
    GLenum mode         = READ_DATA( 8, GLenum );
    GLsizei count       = READ_DATA( 12, GLsizei );
    GLenum type         = READ_DATA( 16, GLenum );
    GLintptrARB indices = READ_DATA( 20, GLuint );
    void * indexptr;
#ifdef CR_ARB_vertex_buffer_object
    GLboolean hasidxdata = READ_DATA(24, GLint);
    indexptr = hasidxdata ? DATA_POINTER(28, void) : (void*)indices;
#else
    indexptr = DATA_POINTER(24, void);
#endif
    cr_unpackDispatch.DrawElements(mode, count, type, indexptr);
}

void crUnpackExtendDrawRangeElements(void)
{
    GLenum mode         = READ_DATA( 8, GLenum );
    GLuint start        = READ_DATA( 12, GLuint );
    GLuint end          = READ_DATA( 16, GLuint );
    GLsizei count       = READ_DATA( 20, GLsizei );
    GLenum type         = READ_DATA( 24, GLenum );
    GLintptrARB indices = READ_DATA( 28, GLuint );
    void * indexptr;
#ifdef CR_ARB_vertex_buffer_object
    GLboolean hasidxdata = READ_DATA(32, GLint);
    indexptr = hasidxdata ? DATA_POINTER(36, void) : (void*)indices;
#else
    indexptr = DATA_POINTER(32, void);
#endif
    cr_unpackDispatch.DrawRangeElements(mode, start, end, count, type, indexptr);
}

void crUnpackMultiDrawArraysEXT(void)
{
    crError( "Can't decode MultiDrawArraysEXT" );
}

void crUnpackMultiDrawElementsEXT(void)
{
    crError( "Can't decode MultiDrawElementsEXT" );
}

static void crUnpackSetClientPointerByIndex(int index, GLint size, 
                                            GLenum type, GLboolean normalized,
                                            GLsizei stride, const GLvoid *pointer, CRClientState *c)
{
    /*crDebug("crUnpackSetClientPointerByIndex: %i(s=%i, t=0x%x, n=%i, str=%i) -> %p", index, size, type, normalized, stride, pointer);*/

    if (index<7)
    {
        switch (index)
        {
            case 0:
                cr_unpackDispatch.VertexPointer(size, type, stride, pointer);
                break;
            case 1:
                cr_unpackDispatch.ColorPointer(size, type, stride, pointer);
                break;
            case 2:
                cr_unpackDispatch.FogCoordPointerEXT(type, stride, pointer);
                break;
            case 3:
                cr_unpackDispatch.SecondaryColorPointerEXT(size, type, stride, pointer);
                break;
            case 4:
                cr_unpackDispatch.EdgeFlagPointer(stride, pointer);
                break;
            case 5:
                cr_unpackDispatch.IndexPointer(type, stride, pointer);
                break;
            case 6:
                cr_unpackDispatch.NormalPointer(type, stride, pointer);
                break;
        }
    }
    else if (index<(7+CR_MAX_TEXTURE_UNITS))
    {
        int curTexUnit = c->curClientTextureUnit;
        if ((index-7)!=curTexUnit)
        {
            cr_unpackDispatch.ClientActiveTextureARB(GL_TEXTURE0_ARB+index-7);
        }
        cr_unpackDispatch.TexCoordPointer(size, type, stride, pointer);
        if ((index-7)!=curTexUnit)
        {
            cr_unpackDispatch.ClientActiveTextureARB(GL_TEXTURE0_ARB+curTexUnit);
        }
    }
    else
    {
        cr_unpackDispatch.VertexAttribPointerARB(index-7-CR_MAX_TEXTURE_UNITS,
                                                 size, type, normalized, stride, pointer);
    }
}

void crUnpackExtendLockArraysEXT(void)
{
    GLint first    = READ_DATA(sizeof(int) + 4, GLint);
    GLint count    = READ_DATA(sizeof(int) + 8, GLint);
    int numenabled = READ_DATA(sizeof(int) + 12, int);

    CRContext *g = crStateGetCurrent();
    CRClientState *c = &g->client;
    CRClientPointer *cp;
    int i, index, offset;
    unsigned char *data;
    
    if (first < 0 || count <= 0 || first >= INT32_MAX - count)
    {
        crError("crUnpackExtendLockArraysEXT: first(%i) count(%i), parameters out of range", first, count);
        return;
    }

    if (numenabled <= 0 || numenabled >= CRSTATECLIENT_MAX_VERTEXARRAYS)
    {
        crError("crUnpackExtendLockArraysEXT: numenabled(%i), parameter out of range", numenabled);
        return;
    }

    offset = 2 * sizeof(int) + 12;

    /*crDebug("crUnpackExtendLockArraysEXT(%i, %i) ne=%i", first, count, numenabled);*/

    for (i = 0; i < numenabled; ++i)
    {
        index = READ_DATA(offset, int);
        offset += sizeof(int);

        cp = crStateGetClientPointerByIndex(index, &c->array);

        CRASSERT(cp && cp->enabled && (!cp->buffer || !cp->buffer->id));

        if (cp && cp->bytesPerIndex > 0)
        {
            if (first + count >= INT32_MAX / cp->bytesPerIndex)
            {
                crError("crUnpackExtendLockArraysEXT: first(%i) count(%i) bpi(%i), parameters out of range", first, count, cp->bytesPerIndex);
                return;
            }

            data = crAlloc((first + count) * cp->bytesPerIndex);

            if (data)
            {
                crMemcpy(data + first * cp->bytesPerIndex, DATA_POINTER(offset, GLvoid), count * cp->bytesPerIndex);
                /*crDebug("crUnpackExtendLockArraysEXT: old cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                        index, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
                crUnpackSetClientPointerByIndex(index, cp->size, cp->type, cp->normalized, 0, data, c);
                /*crDebug("crUnpackExtendLockArraysEXT: new cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                        index, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
            }
            else
            {
                crError("crUnpackExtendLockArraysEXT: crAlloc failed");
                return;
            }
        }
        else
        {
            crError("crUnpackExtendLockArraysEXT: wrong CRClientState %i", index);
            return;
        }

        offset += count * cp->bytesPerIndex;
    }

    cr_unpackDispatch.LockArraysEXT(first, count);
}

void crUnpackExtendUnlockArraysEXT(void)
{
    int i;
    CRContext *g = crStateGetCurrent();
    CRClientState *c = &g->client;
    CRClientPointer *cp;

    /*crDebug("crUnpackExtendUnlockArraysEXT");*/

    cr_unpackDispatch.UnlockArraysEXT();

    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        cp = crStateGetClientPointerByIndex(i, &c->array);
        if (cp->enabled)
        {
            /*crDebug("crUnpackExtendUnlockArraysEXT: old cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                    i, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
            crUnpackSetClientPointerByIndex(i, cp->size, cp->type, cp->normalized, cp->prevStride, cp->prevPtr, c);
            /*crDebug("crUnpackExtendUnlockArraysEXT: new cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                    i, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
        }
    }
}
