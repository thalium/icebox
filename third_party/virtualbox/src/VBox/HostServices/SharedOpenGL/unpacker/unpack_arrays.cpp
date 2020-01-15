/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "unpack_extend.h"
#include "unpacker.h"


void crUnpackExtendVertexPointer(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20 + sizeof(GLuint) );

    GLint size = READ_DATA(pState, 8, GLint );
    GLenum type = READ_DATA(pState, 12, GLenum );
    GLsizei stride = READ_DATA(pState, 16, GLsizei );
    GLintptrARB pointer = (GLintptrARB) READ_DATA(pState, 20, GLuint );
    pState->pDispatchTbl->VertexPointer( size, type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendTexCoordPointer(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20 + sizeof(GLuint) );

    GLint size = READ_DATA(pState, 8, GLint );
    GLenum type = READ_DATA(pState, 12, GLenum );
    GLsizei stride = READ_DATA(pState, 16, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 20, GLuint );
    pState->pDispatchTbl->TexCoordPointer( size, type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendNormalPointer(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 16 + sizeof(GLuint) );

    GLenum type = READ_DATA(pState, 8, GLenum );
    GLsizei stride = READ_DATA(pState, 12, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 16, GLuint );
    pState->pDispatchTbl->NormalPointer( type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendIndexPointer(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 16 + sizeof(GLuint) );

    GLenum type = READ_DATA(pState, 8, GLenum );
    GLsizei stride = READ_DATA(pState, 12, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 16, GLuint );
    pState->pDispatchTbl->IndexPointer( type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendEdgeFlagPointer(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 12 + sizeof(GLuint) );

    GLsizei stride = READ_DATA(pState, 8, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 12, GLuint );
    pState->pDispatchTbl->EdgeFlagPointer( stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendColorPointer(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20 + sizeof(GLuint) );

    GLint size = READ_DATA(pState, 8, GLint );
    GLenum type = READ_DATA(pState, 12, GLenum );
    GLsizei stride = READ_DATA(pState, 16, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 20, GLuint );
    pState->pDispatchTbl->ColorPointer( size, type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendFogCoordPointerEXT(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 16 + sizeof(GLuint) );

    GLenum type = READ_DATA(pState, 8, GLenum );
    GLsizei stride = READ_DATA(pState, 12, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 16, GLuint );
    pState->pDispatchTbl->FogCoordPointerEXT( type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendSecondaryColorPointerEXT(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 20 + sizeof(GLuint) );

    GLint size = READ_DATA(pState, 8, GLint );
    GLenum type = READ_DATA(pState, 12, GLenum );
    GLsizei stride = READ_DATA(pState, 16, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 20, GLuint );
    pState->pDispatchTbl->SecondaryColorPointerEXT( size, type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendVertexAttribPointerARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 28 + sizeof(GLuint) );

    GLuint index = READ_DATA(pState, 8, GLuint);
    GLint size = READ_DATA(pState, 12, GLint );
    GLenum type = READ_DATA(pState, 16, GLenum );
    GLboolean normalized = READ_DATA(pState, 20, GLboolean );
    GLsizei stride = READ_DATA(pState, 24, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 28, GLuint );
    pState->pDispatchTbl->VertexAttribPointerARB( index, size, type, normalized, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendVertexAttribPointerNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 24 + sizeof(GLuint) );

    GLuint index = READ_DATA(pState, 8, GLuint);
    GLint size = READ_DATA(pState, 12, GLint );
    GLenum type = READ_DATA(pState, 16, GLenum );
    GLsizei stride = READ_DATA(pState, 20, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 24, GLuint );
    pState->pDispatchTbl->VertexAttribPointerNV( index, size, type, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendInterleavedArrays(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 16 + sizeof(GLuint) );

    GLenum format = READ_DATA(pState, 8, GLenum );
    GLsizei stride = READ_DATA(pState, 12, GLsizei );
    GLintptrARB pointer = READ_DATA(pState, 16, GLuint );
    pState->pDispatchTbl->InterleavedArrays( format, stride, (void *) pointer, false /*fRealPtr*/ );
}

void crUnpackExtendDrawElements(PCrUnpackerState pState)
{
#ifdef CR_ARB_vertex_buffer_object
    CHECK_BUFFER_SIZE_STATIC(pState, 28);
#else
    CHECK_BUFFER_SIZE_STATIC(pState, 24);
#endif

    GLenum mode         = READ_DATA(pState, 8, GLenum );
    GLsizei count       = READ_DATA(pState, 12, GLsizei );
    GLenum type         = READ_DATA(pState, 16, GLenum );
    GLintptrARB indices = READ_DATA(pState, 20, GLuint );
    void * indexptr;

    size_t cbElem = 0;
    switch (type)
    {
        case GL_UNSIGNED_BYTE:
            cbElem = sizeof(GLubyte);
            break;
        case GL_UNSIGNED_SHORT:
            cbElem = sizeof(GLubyte);
            break;
        case GL_UNSIGNED_INT:
            cbElem = sizeof(GLubyte);
            break;
        default:
            crError("crUnpackExtendDrawElements: Invalid type (%#x) passed!\n", type);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

#ifdef CR_ARB_vertex_buffer_object
    GLboolean hasidxdata = READ_DATA(pState, 24, GLint);
    indexptr = hasidxdata ? DATA_POINTER(pState, 28, void) : (void*)indices;
    if (hasidxdata)
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_SZ_LAST(pState, indexptr, count, cbElem);
#else
    indexptr = DATA_POINTER(pState, 24, void);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_SZ_LAST(pState, indexptr, count, cbElem);
#endif
    pState->pDispatchTbl->DrawElements(mode, count, type, indexptr);
}

void crUnpackExtendDrawRangeElements(PCrUnpackerState pState)
{
#ifdef CR_ARB_vertex_buffer_object
    CHECK_BUFFER_SIZE_STATIC(pState, 36);
#else
    CHECK_BUFFER_SIZE_STATIC(pState, 32);
#endif

    GLenum mode         = READ_DATA(pState, 8, GLenum );
    GLuint start        = READ_DATA(pState, 12, GLuint );
    GLuint end          = READ_DATA(pState, 16, GLuint );
    GLsizei count       = READ_DATA(pState, 20, GLsizei );
    GLenum type         = READ_DATA(pState, 24, GLenum );
    GLintptrARB indices = READ_DATA(pState, 28, GLuint );
    void * indexptr;

    size_t cbElem = 0;
    switch (type)
    {
        case GL_UNSIGNED_BYTE:
            cbElem = sizeof(GLubyte);
            break;
        case GL_UNSIGNED_SHORT:
            cbElem = sizeof(GLubyte);
            break;
        case GL_UNSIGNED_INT:
            cbElem = sizeof(GLubyte);
            break;
        default:
            crError("crUnpackExtendDrawElements: Invalid type (%#x) passed!\n", type);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }
#ifdef CR_ARB_vertex_buffer_object
    GLboolean hasidxdata = READ_DATA(pState, 32, GLint);
    indexptr = hasidxdata ? DATA_POINTER(pState, 36, void) : (void*)indices;
    if (hasidxdata)
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_SZ_LAST(pState, indexptr, count, cbElem);
#else
    indexptr = DATA_POINTER(pState, 32, void);
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_SZ_LAST(pState, indexptr, count, cbElem);
#endif

    pState->pDispatchTbl->DrawRangeElements(mode, start, end, count, type, indexptr);
}

void crUnpackMultiDrawArraysEXT(PCrUnpackerState pState)
{
    RT_NOREF(pState);
    crError( "Can't decode MultiDrawArraysEXT" );
}

void crUnpackMultiDrawElementsEXT(PCrUnpackerState pState)
{
    RT_NOREF(pState);
    crError( "Can't decode MultiDrawElementsEXT" );
}

static void crUnpackSetClientPointerByIndex(PCrUnpackerState pState, int index, GLint size, 
                                            GLenum type, GLboolean normalized,
                                            GLsizei stride, const GLvoid *pointer, CRClientState *c, int fRealPtr)
{
    /*crDebug("crUnpackSetClientPointerByIndex: %i(s=%i, t=0x%x, n=%i, str=%i) -> %p", index, size, type, normalized, stride, pointer);*/

    if (index<7)
    {
        switch (index)
        {
            case 0:
                pState->pDispatchTbl->VertexPointer(size, type, stride, pointer, fRealPtr);
                break;
            case 1:
                pState->pDispatchTbl->ColorPointer(size, type, stride, pointer, fRealPtr);
                break;
            case 2:
                pState->pDispatchTbl->FogCoordPointerEXT(type, stride, pointer, fRealPtr);
                break;
            case 3:
                pState->pDispatchTbl->SecondaryColorPointerEXT(size, type, stride, pointer, fRealPtr);
                break;
            case 4:
                pState->pDispatchTbl->EdgeFlagPointer(stride, pointer, fRealPtr);
                break;
            case 5:
                pState->pDispatchTbl->IndexPointer(type, stride, pointer, fRealPtr);
                break;
            case 6:
                pState->pDispatchTbl->NormalPointer(type, stride, pointer, fRealPtr);
                break;
        }
    }
    else if (index<(7+CR_MAX_TEXTURE_UNITS))
    {
        int curTexUnit = c->curClientTextureUnit;
        if ((index-7)!=curTexUnit)
        {
            pState->pDispatchTbl->ClientActiveTextureARB(GL_TEXTURE0_ARB+index-7);
        }
        pState->pDispatchTbl->TexCoordPointer(size, type, stride, pointer, fRealPtr);
        if ((index-7)!=curTexUnit)
        {
            pState->pDispatchTbl->ClientActiveTextureARB(GL_TEXTURE0_ARB+curTexUnit);
        }
    }
    else
    {
        pState->pDispatchTbl->VertexAttribPointerARB(index-7-CR_MAX_TEXTURE_UNITS,
                                                 size, type, normalized, stride, pointer, fRealPtr);
    }
}

void crUnpackExtendLockArraysEXT(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, sizeof(int) + 12 + sizeof(int));

    GLint first    = READ_DATA(pState, sizeof(int) + 4, GLint);
    GLint count    = READ_DATA(pState, sizeof(int) + 8, GLint);
    int numenabled = READ_DATA(pState, sizeof(int) + 12, int);

    CRContext *g = crStateGetCurrent(pState->pStateTracker);
    CRClientState *c = &g->client;
    CRClientPointer *cp;
    int i, index, offset;
    uint8_t *data;

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
        CHECK_BUFFER_SIZE_STATIC_LAST(pState, offset, int);
        index = READ_DATA(pState, offset, int);
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

            data = (uint8_t *)crAlloc((first + count) * cp->bytesPerIndex);

            if (data)
            {
                crMemcpy(data + first * cp->bytesPerIndex, DATA_POINTER(pState, offset, GLvoid), count * cp->bytesPerIndex);
                /*crDebug("crUnpackExtendLockArraysEXT: old cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                        index, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
                crUnpackSetClientPointerByIndex(pState, index, cp->size, cp->type, cp->normalized, 0, data, c, 1);
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

    pState->pDispatchTbl->LockArraysEXT(first, count);
}

void crUnpackExtendUnlockArraysEXT(PCrUnpackerState pState)
{
    int i;
    CRContext *g = crStateGetCurrent(pState->pStateTracker);
    CRClientState *c = &g->client;
    CRClientPointer *cp;

    /*crDebug("crUnpackExtendUnlockArraysEXT");*/

    pState->pDispatchTbl->UnlockArraysEXT();

    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        cp = crStateGetClientPointerByIndex(i, &c->array);
        if (cp->enabled)
        {
            /*crDebug("crUnpackExtendUnlockArraysEXT: old cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                    i, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
            unsigned char *prevPtr = cp->prevPtr;
            int fPrevRealPtr = cp->fPrevRealPtr;
            cp->prevPtr = NULL;
            cp->fPrevRealPtr = 0;

            crUnpackSetClientPointerByIndex(pState, i, cp->size, cp->type, cp->normalized, cp->prevStride, prevPtr, c, fPrevRealPtr);
            /*crDebug("crUnpackExtendUnlockArraysEXT: new cp(%i): en/l=%i(%i) p=%p size=%i type=0x%x n=%i str=%i pp=%p pstr=%i",
                    i, cp->enabled, cp->locked, cp->p, cp->size, cp->type, cp->normalized, cp->stride, cp->prevPtr, cp->prevStride);*/
        }
    }
}
