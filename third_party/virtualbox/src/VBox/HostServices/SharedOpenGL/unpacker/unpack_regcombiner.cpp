/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackExtendCombinerParameterfvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, GLfloat);

    GLenum pname = READ_DATA(pState,  sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState,  sizeof( int ) + 8, GLfloat );

    switch (pname)
    {
        case GL_CONSTANT_COLOR0_NV:
        case GL_CONSTANT_COLOR1_NV:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLfloat);
            break;
        case GL_NUM_GENERAL_COMBINERS_NV:
        case GL_COLOR_SUM_CLAMP_NV:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLfloat);
            break;
        default:
            crError("crUnpackExtendCombinerParameterfvNV: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->CombinerParameterfvNV( pname, params );
    /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}

void crUnpackExtendCombinerParameterivNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, GLint);

    GLenum pname = READ_DATA(pState,  sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER(pState,  sizeof( int ) + 8, GLint );

    switch (pname)
    {
        case GL_CONSTANT_COLOR0_NV:
        case GL_CONSTANT_COLOR1_NV:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLint);
            break;
        case GL_NUM_GENERAL_COMBINERS_NV:
        case GL_COLOR_SUM_CLAMP_NV:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 1, GLint);
            break;
        default:
            crError("crUnpackExtendCombinerParameterivNV: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->CombinerParameterivNV( pname, params );
    /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}

void crUnpackExtendCombinerStageParameterfvNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, GLenum);

    GLenum stage = READ_DATA(pState,  sizeof( int ) + 4, GLenum );
    GLenum pname = READ_DATA(pState,  sizeof( int ) + 8, GLenum );
    GLfloat *params = DATA_POINTER(pState,  sizeof( int ) + 12, GLfloat );

    switch (pname)
    {
        case GL_CONSTANT_COLOR0_NV:
        case GL_CONSTANT_COLOR1_NV:
            CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, 4, GLint);
            break;
        default:
            crError("crUnpackExtendCombinerStageParameterfvNV: Invalid pname (%#x) passed!\n", pname);
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
    }

    pState->pDispatchTbl->CombinerStageParameterfvNV( stage, pname, params );
    /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}
