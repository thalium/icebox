/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state_internals.h"
#include "state/cr_statetypes.h"



void APIENTRY crStateGenFencesNV(PCRStateTracker pState, GLsizei n, GLuint *fences)
{
    RT_NOREF(pState, n, fences);
}


void APIENTRY crStateDeleteFencesNV(PCRStateTracker pState, GLsizei n, const GLuint *fences)
{
    RT_NOREF(pState, n, fences);
}

void APIENTRY crStateSetFenceNV(PCRStateTracker pState, GLuint fence, GLenum condition)
{
    RT_NOREF(pState, fence, condition);
}

GLboolean APIENTRY crStateTestFenceNV(PCRStateTracker pState, GLuint fence)
{
    RT_NOREF(pState, fence);
    return GL_FALSE;
}

void APIENTRY crStateFinishFenceNV(PCRStateTracker pState, GLuint fence)
{
    RT_NOREF(pState, fence);
}

GLboolean APIENTRY crStateIsFenceNV(PCRStateTracker pState, GLuint fence)
{
    RT_NOREF(pState, fence);
    return GL_FALSE;
}

void APIENTRY crStateGetFenceivNV(PCRStateTracker pState, GLuint fence, GLenum pname, GLint *params)
{
    RT_NOREF(pState, fence, pname, params);
}

