/*
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackExtendChromiumParametervCR(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, GLsizei);

    GLenum target = READ_DATA(pState, 8, GLenum );
    GLenum type = READ_DATA(pState, 12, GLenum );
    GLsizei count = READ_DATA(pState, 16, GLsizei );
    GLvoid *values = DATA_POINTER(pState, 20, GLvoid );

    size_t cbValue = 0;
    switch (type)
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            cbValue = sizeof(GLbyte);
            break;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            cbValue = sizeof(GLshort);
            break;
        case GL_INT:
        case GL_UNSIGNED_INT:
            cbValue = sizeof(GLint);
            break;
        case GL_FLOAT:
            cbValue = sizeof(GLfloat);
            break;
        case GL_DOUBLE:
            cbValue = sizeof(GLdouble);
            break;
        default:
            crError("crUnpackExtendChromiumParametervCR: Invalid type (%#x) passed!\n", type);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_SZ_LAST(pState, values, count, cbValue);
    pState->pDispatchTbl->ChromiumParametervCR(target, type, count, values);
}

void crUnpackExtendDeleteQueriesARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 8, GLsizei);

    GLsizei n = READ_DATA(pState, 8, GLsizei );
    const GLuint *ids = DATA_POINTER(pState, 12, GLuint);

    if (n < 0 || n >= (INT32_MAX / 8) / sizeof(GLint) || !DATA_POINTER_CHECK_SIZE(pState, 12 + n * sizeof(GLuint)))
    {
        crError("crUnpackExtendDeleteQueriesARB: parameter 'n' is out of range");
        return;
    }


    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, ids, n, GLuint);
    pState->pDispatchTbl->DeleteQueriesARB(n, ids);
}

void crUnpackExtendGetPolygonStipple(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 16, CRNetworkPointer);

    SET_RETURN_PTR(pState, 8 );
    SET_WRITEBACK_PTR(pState, 16 );
    GLubyte *mask = DATA_POINTER(pState, 8, GLubyte);

    /*
     * This method will write to the set writeback buffer and not to the mask argument, the mask argument is only used
     * for GL_PIXEL_PACK_BUFFER_ARB contexts where it denotes an offset (GLint) which was already enclosed in the
     * buffer verification.
     */
    pState->pDispatchTbl->GetPolygonStipple( mask );
}

void crUnpackExtendGetPixelMapfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 20, CRNetworkPointer);

    GLenum map = READ_DATA(pState, 8, GLenum );

    SET_RETURN_PTR(pState, 12 );
    SET_WRITEBACK_PTR(pState, 20 );
    GLfloat *values = DATA_POINTER(pState, 12, GLfloat);

    /* see crUnpackExtendGetPolygonStipple() for verification notes. */
    pState->pDispatchTbl->GetPixelMapfv( map, values );
}

void crUnpackExtendGetPixelMapuiv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 20, CRNetworkPointer);

    GLenum map = READ_DATA(pState, 8, GLenum );
    GLuint *values;

    SET_RETURN_PTR(pState, 12 );
    SET_WRITEBACK_PTR(pState, 20 );
    values = DATA_POINTER(pState, 12, GLuint);

    /* see crUnpackExtendGetPolygonStipple() for verification notes. */
    pState->pDispatchTbl->GetPixelMapuiv( map, values );
}

void crUnpackExtendGetPixelMapusv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 20, CRNetworkPointer);

    GLenum map = READ_DATA(pState, 8, GLenum );
    GLushort *values;

    SET_RETURN_PTR(pState, 12 );
    SET_WRITEBACK_PTR(pState, 20 );
    values = DATA_POINTER(pState, 12, GLushort);

    /* see crUnpackExtendGetPolygonStipple() for verification notes. */
    pState->pDispatchTbl->GetPixelMapusv( map, values );
}

void crUnpackExtendVBoxTexPresent(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, GLint);

    GLuint texture = READ_DATA(pState, 8, GLuint );
    GLuint cfg = READ_DATA(pState, 12, GLuint );
    GLint xPos = READ_DATA(pState, 16, GLint );
    GLint yPos = READ_DATA(pState, 20, GLint );
    GLint cRects = READ_DATA(pState, 24, GLint );
    GLint *pRects = (GLint *)DATA_POINTER(pState, 28, GLvoid );

    if (cRects <= 0 || cRects >= (INT32_MAX / 8) / (4 * sizeof(GLint)) || !DATA_POINTER_CHECK_SIZE(pState, 28 + cRects * 4 * sizeof(GLint)))
    {
        crError("crUnpackExtendVBoxTexPresent: parameter 'cRects' %d is out of range", cRects);
        return;
    }

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pRects, cRects * 4, GLint); /* Each rect as 4 points. */
    pState->pDispatchTbl->VBoxTexPresent( texture, cfg, xPos, yPos, cRects, pRects );
}
