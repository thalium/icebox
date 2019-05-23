/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "cr_mem.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStatePixelInit(CRContext *ctx)
{
    CRPixelState *p = &ctx->pixel;
    CRStateBits *sb = GetCurrentBits();
    CRPixelBits *pb = &(sb->pixel);
    GLcolorf zero_color = {0.0f, 0.0f, 0.0f, 0.0f};
    GLcolorf one_color = {1.0f, 1.0f, 1.0f, 1.0f};

    p->mapColor           = GL_FALSE;
    p->mapStencil         = GL_FALSE;
    p->indexShift         = 0;
    p->indexOffset        = 0;
    p->scale              = one_color;
    p->depthScale         = 1.0f;
    p->bias               = zero_color;
    p->depthBias          = 0.0f;
    p->xZoom              = 1.0f;
    p->yZoom              = 1.0f;
    RESET(pb->transfer, ctx->bitid);
    RESET(pb->zoom, ctx->bitid);

    p->mapStoS[0] = 0;
    p->mapItoI[0] = 0;
    p->mapItoR[0] = 0.0;
    p->mapItoG[0] = 0.0;
    p->mapItoB[0] = 0.0;
    p->mapItoA[0] = 0.0;
    p->mapRtoR[0] = 0.0;
    p->mapGtoG[0] = 0.0;
    p->mapBtoB[0] = 0.0;
    p->mapAtoA[0] = 0.0;

    p->mapItoIsize   = 1;
    p->mapStoSsize   = 1;
    p->mapItoRsize   = 1;
    p->mapItoGsize   = 1;
    p->mapItoBsize   = 1;
    p->mapItoAsize   = 1;
    p->mapRtoRsize   = 1;
    p->mapGtoGsize   = 1;
    p->mapBtoBsize   = 1;
    p->mapAtoAsize   = 1;
    RESET(pb->maps, ctx->bitid);

    RESET(pb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStatePixelTransferi (GLenum pname, GLint param)
{
    crStatePixelTransferf( pname, (GLfloat) param );
}

void STATE_APIENTRY crStatePixelTransferf (GLenum pname, GLfloat param)
{
    CRContext *g = GetCurrentContext();
    CRPixelState *p = &(g->pixel);
    CRStateBits *sb = GetCurrentBits();
    CRPixelBits *pb = &(sb->pixel);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "PixelTransfer{if} called in Begin/End");
        return;
    }

    FLUSH();

    switch( pname )
    {
        case GL_MAP_COLOR:
            p->mapColor = (GLboolean) ((param == 0.0f) ? GL_FALSE : GL_TRUE);
            break;
        case GL_MAP_STENCIL:
            p->mapStencil = (GLboolean) ((param == 0.0f) ? GL_FALSE : GL_TRUE);
            break;
        case GL_INDEX_SHIFT:
            p->indexShift = (GLint) param;
            break;
        case GL_INDEX_OFFSET:
            p->indexOffset = (GLint) param;
            break;
        case GL_RED_SCALE:
            p->scale.r = param;
            break;
        case GL_GREEN_SCALE:
            p->scale.g = param;
            break;
        case GL_BLUE_SCALE:
            p->scale.b = param;
            break;
        case GL_ALPHA_SCALE:
            p->scale.a = param;
            break;
        case GL_DEPTH_SCALE:
            p->depthScale = param;
            break;
        case GL_RED_BIAS:
            p->bias.r = param;
            break;
        case GL_GREEN_BIAS:
            p->bias.g = param;
            break;
        case GL_BLUE_BIAS:
            p->bias.b = param;
            break;
        case GL_ALPHA_BIAS:
            p->bias.a = param;
            break;
        case GL_DEPTH_BIAS:
            p->depthBias = param;
            break;
        default:
            crStateError( __LINE__, __FILE__, GL_INVALID_VALUE, "Unknown glPixelTransfer pname: %d", pname );
            return;
    }
    DIRTY(pb->transfer, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePixelZoom (GLfloat xfactor, GLfloat yfactor) 
{
    CRContext *g = GetCurrentContext();
    CRPixelState *p = &(g->pixel);
    CRStateBits *sb = GetCurrentBits();
    CRPixelBits *pb = &(sb->pixel);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "PixelZoom called in Begin/End");
        return;
    }

    FLUSH();

    p->xZoom = xfactor;
    p->yZoom = yfactor;
    DIRTY(pb->zoom, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateBitmap( GLsizei width, GLsizei height, 
        GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, 
        const GLubyte *bitmap)
{
    CRContext *g = GetCurrentContext();
    CRCurrentState *c = &(g->current);
    CRStateBits *sb = GetCurrentBits();
    CRCurrentBits *cb = &(sb->current);

    (void) xorig;
    (void) yorig;
    (void) bitmap;

    if (g->lists.mode == GL_COMPILE)
        return;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, 
            "Bitmap called in begin/end");
        return;
    }

    if (width < 0 || height < 0)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
            "Bitmap called with neg dims: %dx%d", width, height);
        return;
    }

    if (!c->rasterValid)
    {
        return;
    }

    c->rasterAttrib[VERT_ATTRIB_POS][0] += xmove;
    c->rasterAttrib[VERT_ATTRIB_POS][1] += ymove;
    DIRTY(cb->rasterPos, g->neg_bitid);
    DIRTY(cb->dirty, g->neg_bitid);

    c->rasterAttribPre[VERT_ATTRIB_POS][0] += xmove;
    c->rasterAttribPre[VERT_ATTRIB_POS][1] += ymove;
}
 

#define UNUSED(x) ((void)(x))

#define CLAMP(x, min, max)  ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

void STATE_APIENTRY crStatePixelMapfv (GLenum map, GLint mapsize, const GLfloat * values)
{
    CRContext *g = GetCurrentContext();
    CRPixelState *p = &(g->pixel);
    CRStateBits *sb = GetCurrentBits();
    CRPixelBits *pb = &(sb->pixel);
    GLint i;
    GLboolean unpackbuffer = crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "PixelMap called in Begin/End");
        return;
    }

    FLUSH();

    if (mapsize < 0 || mapsize > CR_MAX_PIXEL_MAP_TABLE) {
       crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "PixelMap(mapsize)");
       return;
    }

    if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
       /* XXX check that mapsize is a power of two */
    }

    switch (map) {
    case GL_PIXEL_MAP_S_TO_S:
        p->mapStoSsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                p->mapStoS[i] = (GLint) values[i];
            }
        break;
    case GL_PIXEL_MAP_I_TO_I:
        p->mapItoIsize = mapsize;
        if (!unpackbuffer)
        for (i=0;i<mapsize;i++) {
            p->mapItoI[i] = (GLint) values[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_R:
        p->mapItoRsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
                p->mapItoR[i] = val;
            }
        break;
    case GL_PIXEL_MAP_I_TO_G:
        p->mapItoGsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
                p->mapItoG[i] = val;
            }
        break;
    case GL_PIXEL_MAP_I_TO_B:
        p->mapItoBsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
                p->mapItoB[i] = val;
            }
        break;
    case GL_PIXEL_MAP_I_TO_A:
        p->mapItoAsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
                p->mapItoA[i] = val;
            }
        break;
    case GL_PIXEL_MAP_R_TO_R:
        p->mapRtoRsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                p->mapRtoR[i] = CLAMP( values[i], 0.0F, 1.0F );
            }
        break;
    case GL_PIXEL_MAP_G_TO_G:
        p->mapGtoGsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                p->mapGtoG[i] = CLAMP( values[i], 0.0F, 1.0F );
            }
        break;
    case GL_PIXEL_MAP_B_TO_B:
        p->mapBtoBsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                p->mapBtoB[i] = CLAMP( values[i], 0.0F, 1.0F );
            }
        break;
    case GL_PIXEL_MAP_A_TO_A:
        p->mapAtoAsize = mapsize;
        if (!unpackbuffer)
            for (i=0;i<mapsize;i++) {
                p->mapAtoA[i] = CLAMP( values[i], 0.0F, 1.0F );
            }
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "PixelMap(map)");
        return;
    }

    DIRTY(pb->maps, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePixelMapuiv (GLenum map, GLint mapsize, const GLuint * values)
{
    if (mapsize < 0 || mapsize > CR_MAX_PIXEL_MAP_TABLE)
    {
        crError("crStatePixelMapuiv: parameter 'mapsize' is out of range");
        return;
    }

    if (!crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        GLfloat fvalues[CR_MAX_PIXEL_MAP_TABLE];
        GLint i;

        if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
           for (i=0;i<mapsize;i++) {
              fvalues[i] = (GLfloat) values[i];
           }
        }
        else {
           for (i=0;i<mapsize;i++) {
              fvalues[i] = values[i] / 4294967295.0F;
           }
        }
        crStatePixelMapfv(map, mapsize, fvalues);
    }
    else
    {
        crStatePixelMapfv(map, mapsize, (const GLfloat*) values);
    }
}
 
void STATE_APIENTRY crStatePixelMapusv (GLenum map, GLint mapsize, const GLushort * values)
{
    GLfloat fvalues[CR_MAX_PIXEL_MAP_TABLE];
    GLint i;

    if (!crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
           for (i=0;i<mapsize;i++) {
              fvalues[i] = (GLfloat) values[i];
           }
        }
        else {
           for (i=0;i<mapsize;i++) {
              fvalues[i] = values[i] / 65535.0F;
           }
        }
        crStatePixelMapfv(map, mapsize, fvalues);
    }
    else
    {
        crStatePixelMapfv(map, mapsize, (const GLfloat*) values);
    }
}

 
void STATE_APIENTRY crStateGetPixelMapfv (GLenum map, GLfloat * values)
{
    CRContext *g = GetCurrentContext();
    CRPixelState *p = &(g->pixel);
    GLint i;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                     "GetPixelMapfv called in Begin/End");
        return;
    }

    switch (map) {
    case GL_PIXEL_MAP_S_TO_S:
        for (i = 0; i < p->mapStoSsize; i++) {
            values[i] = (GLfloat) p->mapStoS[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_I:
        for (i = 0; i < p->mapItoIsize; i++) {
            values[i] = (GLfloat) p->mapItoI[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_R:
        crMemcpy(values, p->mapItoR, p->mapItoRsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_I_TO_G:
        crMemcpy(values, p->mapItoG, p->mapItoGsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_I_TO_B:
        crMemcpy(values, p->mapItoB, p->mapItoBsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_I_TO_A:
        crMemcpy(values, p->mapItoA, p->mapItoAsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_R_TO_R:
        crMemcpy(values, p->mapRtoR, p->mapRtoRsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_G_TO_G:
        crMemcpy(values, p->mapGtoG, p->mapGtoGsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_B_TO_B:
        crMemcpy(values, p->mapBtoB, p->mapBtoBsize * sizeof(GLfloat));
        break;
    case GL_PIXEL_MAP_A_TO_A:
        crMemcpy(values, p->mapAtoA, p->mapAtoAsize * sizeof(GLfloat));
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "GetPixelMap(map)");
        return;
    }
}
 
void STATE_APIENTRY crStateGetPixelMapuiv (GLenum map, GLuint * values)
{
    CRContext *g = GetCurrentContext();
    const GLfloat maxUint = 4294967295.0F;
    CRPixelState *p = &(g->pixel);
    GLint i;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                     "GetPixelMapuiv called in Begin/End");
        return;
    }

    switch (map) {
    case GL_PIXEL_MAP_S_TO_S:
        for (i = 0; i < p->mapStoSsize; i++) {
            values[i] = p->mapStoS[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_I:
        for (i = 0; i < p->mapItoIsize; i++) {
            values[i] = p->mapItoI[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_R:
        for (i = 0; i < p->mapItoRsize; i++) {
            values[i] = (GLuint) (p->mapItoR[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_I_TO_G:
        for (i = 0; i < p->mapItoGsize; i++) {
            values[i] = (GLuint) (p->mapItoG[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_I_TO_B:
        for (i = 0; i < p->mapItoBsize; i++) {
            values[i] = (GLuint) (p->mapItoB[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_I_TO_A:
        for (i = 0; i < p->mapItoAsize; i++) {
            values[i] = (GLuint) (p->mapItoA[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_R_TO_R:
        for (i = 0; i < p->mapRtoRsize; i++) {
            values[i] = (GLuint) (p->mapRtoR[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_G_TO_G:
        for (i = 0; i < p->mapGtoGsize; i++) {
            values[i] = (GLuint) (p->mapGtoG[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_B_TO_B:
        for (i = 0; i < p->mapBtoBsize; i++) {
            values[i] = (GLuint) (p->mapBtoB[i] * maxUint);
        }
        break;
    case GL_PIXEL_MAP_A_TO_A:
        for (i = 0; i < p->mapAtoAsize; i++) {
            values[i] = (GLuint) (p->mapAtoA[i] * maxUint);
        }
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "GetPixelMapuiv(map)");
        return;
    }
}
 
void STATE_APIENTRY crStateGetPixelMapusv (GLenum map, GLushort * values)
{
    CRContext *g = GetCurrentContext();
    const GLfloat maxUshort = 65535.0F;
    CRPixelState *p = &(g->pixel);
    GLint i;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                     "GetPixelMapusv called in Begin/End");
        return;
    }

    switch (map) {
    case GL_PIXEL_MAP_S_TO_S:
        for (i = 0; i < p->mapStoSsize; i++) {
            values[i] = p->mapStoS[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_I:
        for (i = 0; i < p->mapItoIsize; i++) {
            values[i] = p->mapItoI[i];
        }
        break;
    case GL_PIXEL_MAP_I_TO_R:
        for (i = 0; i < p->mapItoRsize; i++) {
            values[i] = (GLushort) (p->mapItoR[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_I_TO_G:
        for (i = 0; i < p->mapItoGsize; i++) {
            values[i] = (GLushort) (p->mapItoG[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_I_TO_B:
        for (i = 0; i < p->mapItoBsize; i++) {
            values[i] = (GLushort) (p->mapItoB[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_I_TO_A:
        for (i = 0; i < p->mapItoAsize; i++) {
            values[i] = (GLushort) (p->mapItoA[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_R_TO_R:
        for (i = 0; i < p->mapRtoRsize; i++) {
            values[i] = (GLushort) (p->mapRtoR[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_G_TO_G:
        for (i = 0; i < p->mapGtoGsize; i++) {
            values[i] = (GLushort) (p->mapGtoG[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_B_TO_B:
        for (i = 0; i < p->mapBtoBsize; i++) {
            values[i] = (GLushort) (p->mapBtoB[i] * maxUshort);
        }
        break;
    case GL_PIXEL_MAP_A_TO_A:
        for (i = 0; i < p->mapAtoAsize; i++) {
            values[i] = (GLushort) (p->mapAtoA[i] * maxUshort);
        }
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "GetPixelMapusv(map)");
        return;
    }
}

void crStatePixelDiff(CRPixelBits *b, CRbitvalue *bitID,
                      CRContext *fromCtx, CRContext *toCtx)
{
    CRPixelState *from = &(fromCtx->pixel);
    CRPixelState *to = &(toCtx->pixel);
    int j, i;
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    i = 0; /* silence compiler */
    if (CHECKDIRTY(b->transfer, bitID))
    {
        if (from->mapColor != to->mapColor)
        {           
            diff_api.PixelTransferi (GL_MAP_COLOR, to->mapColor);
            from->mapColor = to->mapColor;
        }
        if (from->mapStencil != to->mapStencil)
        {
            diff_api.PixelTransferi (GL_MAP_STENCIL, to->mapStencil);
            from->mapStencil = to->mapStencil;
        }
        if (from->indexOffset != to->indexOffset)
        {
            diff_api.PixelTransferi (GL_INDEX_OFFSET, to->indexOffset);
            from->indexOffset = to->indexOffset;
        }
        if (from->indexShift != to->indexShift)
        {
            diff_api.PixelTransferi (GL_INDEX_SHIFT, to->indexShift);
            from->indexShift = to->indexShift;
        }
        if (from->scale.r != to->scale.r)
        {
            diff_api.PixelTransferf (GL_RED_SCALE, to->scale.r);
            from->scale.r = to->scale.r;
        }
        if (from->scale.g != to->scale.g)
        {
            diff_api.PixelTransferf (GL_GREEN_SCALE, to->scale.g);
            from->scale.g = to->scale.g;
        }
        if (from->scale.b != to->scale.b)
        {
            diff_api.PixelTransferf (GL_BLUE_SCALE, to->scale.b);
            from->scale.b = to->scale.b;
        }
        if (from->scale.a != to->scale.a)
        {
            diff_api.PixelTransferf (GL_ALPHA_SCALE, to->scale.a);
            from->scale.a = to->scale.a;
        }
        if (from->bias.r != to->bias.r)
        {
            diff_api.PixelTransferf (GL_RED_BIAS, to->bias.r);
            from->bias.r = to->bias.r;
        }
        if (from->bias.g != to->bias.g)
        {
            diff_api.PixelTransferf (GL_GREEN_BIAS, to->bias.g);
            from->bias.g = to->bias.g;
        }
        if (from->bias.b != to->bias.b)
        {
            diff_api.PixelTransferf (GL_BLUE_BIAS, to->bias.b);
            from->bias.b = to->bias.b;
        }
        if (from->bias.a != to->bias.a)
        {
            diff_api.PixelTransferf (GL_ALPHA_BIAS, to->bias.a);
            from->bias.a = to->bias.a;
        }
        if (from->depthScale != to->depthScale)
        {
            diff_api.PixelTransferf (GL_DEPTH_SCALE, to->depthScale);
            from->depthScale = to->depthScale;
        }
        if (from->depthBias != to->depthBias)
        {
            diff_api.PixelTransferf (GL_DEPTH_BIAS, to->depthBias);
            from->depthBias = to->depthBias;
        }
        CLEARDIRTY(b->transfer, nbitID);
    }
    if (CHECKDIRTY(b->zoom, bitID))
    {
        if (from->xZoom != to->xZoom ||
            from->yZoom != to->yZoom)
        {
            diff_api.PixelZoom (to->xZoom,
                to->yZoom);
            from->xZoom = to->xZoom;
            from->yZoom = to->yZoom;
        }
        CLEARDIRTY(b->zoom, nbitID);
    }
    if (CHECKDIRTY(b->maps, bitID))
    {
        if (crMemcmp(to->mapStoS, from->mapStoS, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_S_TO_S,to->mapStoSsize,(GLfloat*)to->mapStoS);
        if (crMemcmp(to->mapItoI, from->mapItoI, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_I,to->mapItoIsize,(GLfloat*)to->mapItoI);
        if (crMemcmp(to->mapItoR, from->mapItoR, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_R,to->mapItoRsize,(GLfloat*)to->mapItoR);
        if (crMemcmp(to->mapItoG, from->mapItoG, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_G,to->mapItoGsize,(GLfloat*)to->mapItoG);
        if (crMemcmp(to->mapItoB, from->mapItoB, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_B,to->mapItoBsize,(GLfloat*)to->mapItoB);
        if (crMemcmp(to->mapItoA, from->mapItoA, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_A,to->mapItoAsize,(GLfloat*)to->mapItoA);
        if (crMemcmp(to->mapRtoR, from->mapRtoR, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_R_TO_R,to->mapRtoRsize,(GLfloat*)to->mapRtoR);
        if (crMemcmp(to->mapGtoG, from->mapGtoG, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_G_TO_G,to->mapGtoGsize,(GLfloat*)to->mapGtoG);
        if (crMemcmp(to->mapBtoB, from->mapBtoB, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_B_TO_B,to->mapBtoBsize,(GLfloat*)to->mapBtoB);
        if (crMemcmp(to->mapAtoA, from->mapAtoA, CR_MAX_PIXEL_MAP_TABLE*sizeof(GLfloat)))
            diff_api.PixelMapfv(GL_PIXEL_MAP_A_TO_A,to->mapAtoAsize,(GLfloat*)to->mapAtoA);
        CLEARDIRTY(b->maps, nbitID);
    }
    CLEARDIRTY(b->dirty, nbitID);
}

void crStatePixelSwitch(CRPixelBits *b, CRbitvalue *bitID,
                      CRContext *fromCtx, CRContext *toCtx)
{
    CRPixelState *from = &(fromCtx->pixel);
    CRPixelState *to = &(toCtx->pixel);
    int j, i;
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    i = 0; /* silence compiler */
    if (CHECKDIRTY(b->transfer, bitID))
    {
        if (from->mapColor != to->mapColor)
        {
            diff_api.PixelTransferi (GL_MAP_COLOR, to->mapColor);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->mapStencil != to->mapStencil)
        {
            diff_api.PixelTransferi (GL_MAP_STENCIL, to->mapStencil);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->indexOffset != to->indexOffset)
        {
            diff_api.PixelTransferi (GL_INDEX_OFFSET, to->indexOffset);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->indexShift != to->indexShift)
        {
            diff_api.PixelTransferi (GL_INDEX_SHIFT, to->indexShift);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->scale.r != to->scale.r)
        {
            diff_api.PixelTransferf (GL_RED_SCALE, to->scale.r);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->scale.g != to->scale.g)
        {
            diff_api.PixelTransferf (GL_GREEN_SCALE, to->scale.g);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->scale.b != to->scale.b)
        {
            diff_api.PixelTransferf (GL_BLUE_SCALE, to->scale.b);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->scale.a != to->scale.a)
        {
            diff_api.PixelTransferf (GL_ALPHA_SCALE, to->scale.a);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->bias.r != to->bias.r)
        {
            diff_api.PixelTransferf (GL_RED_BIAS, to->bias.r);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->bias.g != to->bias.g)
        {
            diff_api.PixelTransferf (GL_GREEN_BIAS, to->bias.g);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->bias.b != to->bias.b)
        {
            diff_api.PixelTransferf (GL_BLUE_BIAS, to->bias.b);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->bias.a != to->bias.a)
        {
            diff_api.PixelTransferf (GL_ALPHA_BIAS, to->bias.a);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->depthScale != to->depthScale)
        {
            diff_api.PixelTransferf (GL_DEPTH_SCALE, to->depthScale);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        if (from->depthBias != to->depthBias)
        {
            diff_api.PixelTransferf (GL_DEPTH_BIAS, to->depthBias);
            FILLDIRTY(b->transfer);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->transfer, nbitID);
    }
    if (CHECKDIRTY(b->zoom, bitID))
    {
        if (from->xZoom != to->xZoom ||
            from->yZoom != to->yZoom)
        {
            diff_api.PixelZoom (to->xZoom,
                to->yZoom);
            FILLDIRTY(b->zoom);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->zoom, nbitID);
    }
    if (CHECKDIRTY(b->maps, bitID))
    {
        if (crMemcmp(to->mapStoS, from->mapStoS, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_S_TO_S,to->mapStoSsize,(GLfloat*)to->mapStoS);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapItoI, from->mapItoI, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_I,to->mapItoIsize,(GLfloat*)to->mapItoI);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapItoR, from->mapItoR, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_R,to->mapItoRsize,(GLfloat*)to->mapItoR);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapItoG, from->mapItoG, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_G,to->mapItoGsize,(GLfloat*)to->mapItoG);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapItoB, from->mapItoB, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_B,to->mapItoBsize,(GLfloat*)to->mapItoB);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapItoA, from->mapItoA, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_I_TO_A,to->mapItoAsize,(GLfloat*)to->mapItoA);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapRtoR, from->mapRtoR, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_R_TO_R,to->mapRtoRsize,(GLfloat*)to->mapRtoR);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapGtoG, from->mapGtoG, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_G_TO_G,to->mapGtoGsize,(GLfloat*)to->mapGtoG);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapBtoB, from->mapBtoB, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_B_TO_B,to->mapBtoBsize,(GLfloat*)to->mapBtoB);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        if (crMemcmp(to->mapAtoA, from->mapAtoA, CR_MAX_PIXEL_MAP_TABLE)) {
            diff_api.PixelMapfv(GL_PIXEL_MAP_A_TO_A,to->mapAtoAsize,(GLfloat*)to->mapAtoA);
            FILLDIRTY(b->maps);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->maps, nbitID);
    }
    CLEARDIRTY(b->dirty, nbitID);
}

