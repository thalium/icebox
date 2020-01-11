/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackLightfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum light = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 8, GLfloat );

    switch (pname)
    {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLfloat);
            break;
        case GL_SPOT_DIRECTION:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 3, GLfloat);
            break;
        case GL_SPOT_EXPONENT:
        case GL_SPOT_CUTOFF:
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLfloat);
            break;
        default:
            crError("crUnpackLightfv: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->Lightfv( light, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackLightiv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum light = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 8, GLint );

    switch (pname)
    {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLint);
            break;
        case GL_SPOT_DIRECTION:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 3, GLint);
            break;
        case GL_SPOT_EXPONENT:
        case GL_SPOT_CUTOFF:
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLint);
            break;
        default:
            crError("crUnpackLightfv: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->Lightiv( light, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackLightModelfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 0, GLenum);

    GLenum pname = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 4, GLfloat );

    switch (pname)
    {
        case GL_LIGHT_MODEL_AMBIENT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLfloat);
            break;
        case GL_LIGHT_MODEL_TWO_SIDE:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLfloat);
            break;
        default:
            crError("crUnpackLightfv: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->LightModelfv( pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackLightModeliv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 0, GLenum);

    GLenum pname = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 4, GLint );

    switch (pname)
    {
        case GL_LIGHT_MODEL_AMBIENT:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLint);
            break;
        case GL_LIGHT_MODEL_TWO_SIDE:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLint);
            break;
        default:
            crError("crUnpackLightfv: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->LightModeliv( pname, params );
    INCR_VAR_PTR(pState);
}
