/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"
#include "cr_pixeldata.h"

void crStatePolygonInit(CRContext *ctx)
{
    CRPolygonState *p = &ctx->polygon;
        CRStateBits *sb = GetCurrentBits();
    CRPolygonBits *pb = &(sb->polygon);
    int i;

    p->polygonSmooth = GL_FALSE;
    p->polygonOffsetFill = GL_FALSE;
    p->polygonOffsetLine = GL_FALSE;
    p->polygonOffsetPoint = GL_FALSE;
    p->polygonStipple = GL_FALSE;
    p->cullFace = GL_FALSE;
    RESET(pb->enable, ctx->bitid);

    p->offsetFactor = 0;
    p->offsetUnits = 0;
    RESET(pb->offset, ctx->bitid);

    p->cullFaceMode = GL_BACK;
    p->frontFace = GL_CCW;
    p->frontMode = GL_FILL;
    p->backMode = GL_FILL;
    RESET(pb->mode, ctx->bitid);

    for (i=0; i<32; i++)
        p->stipple[i] = 0xFFFFFFFF;
    RESET(pb->stipple, ctx->bitid);

    RESET(pb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStateCullFace(GLenum mode) 
{
    CRContext *g = GetCurrentContext();
    CRPolygonState *p = &(g->polygon);
        CRStateBits *sb = GetCurrentBits();
    CRPolygonBits *pb = &(sb->polygon);

    if (g->current.inBeginEnd) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "glCullFace called in begin/end");
        return;
    }

    FLUSH();

    if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "glCullFace called with bogus mode: 0x%x", mode);
        return;
    }

    p->cullFaceMode = mode;
    DIRTY(pb->mode, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateFrontFace (GLenum mode) 
{
    CRContext *g = GetCurrentContext();
    CRPolygonState *p = &(g->polygon);
        CRStateBits *sb = GetCurrentBits();
    CRPolygonBits *pb = &(sb->polygon);

    if (g->current.inBeginEnd) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "glFrontFace called in begin/end");
        return;
    }

    FLUSH();

    if (mode != GL_CW && mode != GL_CCW)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "glFrontFace called with bogus mode: 0x%x", mode);
        return;
    }

    p->frontFace = mode;
    DIRTY(pb->mode, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}

void  STATE_APIENTRY crStatePolygonMode (GLenum face, GLenum mode) 
{
    CRContext *g = GetCurrentContext();
    CRPolygonState *p = &(g->polygon);
        CRStateBits *sb = GetCurrentBits();
    CRPolygonBits *pb = &(sb->polygon);

    if (g->current.inBeginEnd) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "glPolygonMode called in begin/end");
        return;
    }

    FLUSH();

    if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "glPolygonMode called with bogus mode: 0x%x", mode);
        return;
    }

    switch (face) {
        case GL_FRONT:
            p->frontMode = mode;
            break;
        case GL_FRONT_AND_BACK:
            p->frontMode = mode;
            RT_FALL_THRU();
        case GL_BACK:
            p->backMode = mode;
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glPolygonMode called with bogus face: 0x%x", face);
            return;
    }
    DIRTY(pb->mode, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePolygonOffset (GLfloat factor, GLfloat units) 
{
    CRContext *g = GetCurrentContext();
    CRPolygonState *p = &(g->polygon);
        CRStateBits *sb = GetCurrentBits();
    CRPolygonBits *pb = &(sb->polygon);

    if (g->current.inBeginEnd) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "glPolygonOffset called in begin/end");
        return;
    }

    FLUSH();

    p->offsetFactor = factor;
    p->offsetUnits = units;

    DIRTY(pb->offset, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePolygonStipple (const GLubyte *p) 
{
    CRContext *g = GetCurrentContext();
    CRPolygonState *poly = &(g->polygon);
        CRStateBits *sb = GetCurrentBits();
    CRPolygonBits *pb = &(sb->polygon);

    if (g->current.inBeginEnd) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "glPolygonStipple called in begin/end");
        return;
    }

    FLUSH();

    if (!p && !crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        crDebug("Void pointer passed to PolygonStipple");
        return;
    }

    /** @todo track mask if buffer is bound?*/
    if (!crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        crMemcpy((char*)poly->stipple, (char*)p, 128);
    }

    DIRTY(pb->dirty, g->neg_bitid);
    DIRTY(pb->stipple, g->neg_bitid);
}

void STATE_APIENTRY crStateGetPolygonStipple( GLubyte *b )
{
    CRContext *g = GetCurrentContext();
    CRPolygonState *poly = &(g->polygon);

    if (g->current.inBeginEnd) 
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                "glGetPolygonStipple called in begin/end");
        return;
    }

    crMemcpy((char*)b, (char*)poly->stipple, 128);
}
