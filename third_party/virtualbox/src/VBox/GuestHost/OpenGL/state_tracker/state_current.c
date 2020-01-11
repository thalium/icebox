/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "state.h"
#include "state_internals.h"

/*
 * Note: regardless of GL_NV_vertex_program, we store all per-vertex
 * attributes in an array now, instead of specially named attributes
 * like color, normal, texcoord, etc.
 */


void crStateCurrentInit( CRContext *ctx )
{
    CRCurrentState *c = &ctx->current;
    CRStateBits *sb = GetCurrentBits(ctx->pStateTracker);
    CRCurrentBits *cb = &(sb->current);
    static const GLfloat default_normal[4]         = {0.0f, 0.0f, 1.0f, 1.0f};
    static const GLfloat default_color[4]          = {1.0f, 1.0f, 1.0f, 1.0f};
    static const GLfloat default_secondaryColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    static const GLfloat default_attrib[4]         = {0.0f, 0.0f, 0.0f, 1.0f};
    unsigned int i;

    /*
     * initialize all vertex attributes to <0,0,0,1> for starters
     */
    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++) {
        COPY_4V(c->vertexAttrib[i], default_attrib);
        COPY_4V(c->vertexAttribPre[i], default_attrib);
    }
    /* now re-do the exceptions */
    COPY_4V(c->vertexAttrib[VERT_ATTRIB_COLOR0], default_color);
    COPY_4V(c->vertexAttrib[VERT_ATTRIB_COLOR1], default_secondaryColor);
    COPY_4V(c->vertexAttrib[VERT_ATTRIB_NORMAL], default_normal);

    c->rasterIndex =  1.0f;
    c->colorIndex = c->colorIndexPre = 1.0;
    c->edgeFlag = c->edgeFlagPre = GL_TRUE;

    /* Set the "pre" values and raster position attributes */
    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++) {
        COPY_4V(c->vertexAttribPre[i], c->vertexAttrib[i]);
        COPY_4V(c->rasterAttrib[i],    c->vertexAttrib[i]);
        COPY_4V(c->rasterAttribPre[i], c->vertexAttrib[i]);
    }

    c->rasterValid = GL_TRUE;

    c->inBeginEnd = GL_FALSE;
    c->beginEndNum = 0;
    /*c->beginEndMax = cfg->beginend_max;*/
    c->mode = 0x10; /* Undefined Mode */
    c->flushOnEnd = 0;

    c->current = 0; /* picked up by crStateSetCurrentPointers() */

    /* init dirty bits */
    RESET(cb->dirty, ctx->bitid);
    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++) {
        RESET(cb->vertexAttrib[i], ctx->bitid);
    }
    RESET(cb->edgeFlag, ctx->bitid);
    RESET(cb->colorIndex, ctx->bitid);
    RESET(cb->rasterPos, ctx->bitid);
}

void STATE_APIENTRY crStateColor3f(PCRStateTracker pState, GLfloat r, GLfloat g, GLfloat b )
{
    crStateColor4f(pState, r, g, b, 1.0F);
}

void STATE_APIENTRY crStateColor3fv(PCRStateTracker pState, const GLfloat *color )
{
    crStateColor4f(pState, color[0], color[1], color[2], 1.0F );
}

void STATE_APIENTRY crStateColor4f(PCRStateTracker pState, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
    CRContext *g = GetCurrentContext(pState);
    CRCurrentState *c = &(g->current);
    CRStateBits *sb = GetCurrentBits(pState);
    CRCurrentBits *cb = &(sb->current);

    FLUSH();

    c->vertexAttrib[VERT_ATTRIB_COLOR0][0] = red;
    c->vertexAttrib[VERT_ATTRIB_COLOR0][1] = green;
    c->vertexAttrib[VERT_ATTRIB_COLOR0][2] = blue;
    c->vertexAttrib[VERT_ATTRIB_COLOR0][3] = alpha;

    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->vertexAttrib[VERT_ATTRIB_COLOR0], g->neg_bitid);
}

void STATE_APIENTRY crStateColor4fv(PCRStateTracker pState, const GLfloat *color )
{
    crStateColor4f(pState, color[0], color[1], color[2], color[3] );
}

void crStateSetCurrentPointers( CRContext *ctx, CRCurrentStatePointers *current )
{
    CRCurrentState *c = &(ctx->current);
    c->current = current;
}

void crStateResetCurrentPointers( CRCurrentStatePointers *current )
{
    uint32_t attribsUsedMask = current->attribsUsedMask;

    crMemset(current, 0, sizeof (*current));

    current->attribsUsedMask = attribsUsedMask;
}

void STATE_APIENTRY crStateBegin(PCRStateTracker pState, GLenum mode )
{
    CRContext *g = GetCurrentContext(pState);
    CRCurrentState *c = &(g->current);

    if (mode > GL_POLYGON)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Begin called with invalid mode: %d", mode);
        return;
    }

    if (c->inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glBegin called inside Begin/End");
        return;
    }

    c->attribsUsedMask = 0;
    c->inBeginEnd = GL_TRUE;
    c->mode = mode;
    c->beginEndNum++;
}

void STATE_APIENTRY crStateEnd(PCRStateTracker pState)
{
    CRContext *g = GetCurrentContext(pState);
    CRCurrentState *c = &(g->current);

    if (!c->inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glEnd called outside Begin/End" );
        return;
    }

    c->inBeginEnd = GL_FALSE;
}

void crStateCurrentSwitch( CRCurrentBits *c, CRbitvalue *bitID,
                                                     CRContext *fromCtx, CRContext *toCtx )
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    const CRCurrentState *from = &(fromCtx->current);
    const CRCurrentState *to = &(toCtx->current);
    const GLuint maxTextureUnits = fromCtx->limits.maxTextureUnits;
    unsigned int i, j;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (CHECKDIRTY(c->rasterPos, bitID)) {
        if (to->rasterValid) {
          const GLfloat fromX = from->rasterAttrib[VERT_ATTRIB_POS][0];
          const GLfloat fromY = from->rasterAttrib[VERT_ATTRIB_POS][1];
          const GLfloat fromZ = from->rasterAttrib[VERT_ATTRIB_POS][2];
          const GLfloat toX = to->rasterAttrib[VERT_ATTRIB_POS][0];
          const GLfloat toY = to->rasterAttrib[VERT_ATTRIB_POS][1];
          const GLfloat toZ = to->rasterAttrib[VERT_ATTRIB_POS][2];
          if (toX != fromX || toY != fromY || toZ != fromZ) {
                /* Use glWindowPos (which updates raster color) */
                pState->diff_api.WindowPos3fvARB(to->rasterAttrib[VERT_ATTRIB_POS]);
              FILLDIRTY(c->rasterPos);
              FILLDIRTY(c->dirty);
          }
        }
        CLEARDIRTY(c->rasterPos, nbitID);
    }

    /* Vertex Current State Switch Code */

    /* Its important that we don't do a value check here because
    ** current may not actually have the correct values, I think...
    ** We also need to restore the current state tracking pointer
    ** since the packing functions will set it.
    */

    if (CHECKDIRTY(c->colorIndex, bitID)) {
        if (to->colorIndex != from->colorIndex) {
            pState->diff_api.Indexf(to->colorIndex);
            FILLDIRTY(c->colorIndex);
            FILLDIRTY(c->dirty);
        }
        CLEARDIRTY(c->colorIndex, nbitID);
    }

    if (CHECKDIRTY(c->edgeFlag, bitID)) {
        if (to->edgeFlag != from->edgeFlag) {
            pState->diff_api.EdgeFlag(to->edgeFlag);
            FILLDIRTY(c->edgeFlag);
            FILLDIRTY(c->dirty);
        }
        CLEARDIRTY(c->edgeFlag, nbitID);
    }

    /* If using a vertex program, update the generic vertex attributes,
     * which may or may not be aliased with conventional attributes.
     */
#if defined(CR_ARB_vertex_program) || defined(CR_NV_vertex_progra)
    if (toCtx->program.vpEnabled &&
            (toCtx->extensions.ARB_vertex_program ||
             (toCtx->extensions.NV_vertex_program))) {
        const unsigned attribsUsedMask = toCtx->current.attribsUsedMask;
        for (i = 1; i < CR_MAX_VERTEX_ATTRIBS; i++) {  /* skip zero */
            if ((attribsUsedMask & (1 << i))
                    && CHECKDIRTY(c->vertexAttrib[i], bitID)) {
                if (COMPARE_VECTOR (from->vertexAttrib[i], to->vertexAttribPre[i])) {
                    pState->diff_api.VertexAttrib4fvARB(i, &(to->vertexAttrib[i][0]));
                    FILLDIRTY(c->vertexAttrib[i]);
                    FILLDIRTY(c->dirty);
                }
                CLEARDIRTY(c->vertexAttrib[i], nbitID);
            }
        }
    }
    /* Fall-through so that attributes which don't have their bit set in the
     * attribsUsedMask get handled via the conventional attribute functions.
     */
#endif

    {
        /* use conventional attribute functions */

        /* NEED TO FIX THIS!!!!!! */
        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR0], bitID)) {
            if (COMPARE_COLOR(from->vertexAttrib[VERT_ATTRIB_COLOR0],to->vertexAttrib[VERT_ATTRIB_COLOR0])) {
                pState->diff_api.Color4fv ((GLfloat *) &(to->vertexAttrib[VERT_ATTRIB_COLOR0]));
                FILLDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR0]);
                FILLDIRTY(c->dirty);
            }
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR0], nbitID);
        }

        /* NEED TO FIX THIS, ALSO?!!!!! */
#ifdef CR_EXT_secondary_color
        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR1], bitID)) {
            if (COMPARE_COLOR(from->vertexAttrib[VERT_ATTRIB_COLOR1],to->vertexAttrib[VERT_ATTRIB_COLOR1])) {
                pState->diff_api.SecondaryColor3fvEXT ((GLfloat *) &(to->vertexAttrib[VERT_ATTRIB_COLOR1]));
                FILLDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR1]);
                FILLDIRTY(c->dirty);
            }
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR1], nbitID);
        }
#endif

        /* NEED TO FIX THIS, ALSO?!!!!! */
#ifdef CR_EXT_fog_coord
        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_FOG], bitID)) {
            if (from->vertexAttrib[VERT_ATTRIB_FOG][0] != to->vertexAttrib[VERT_ATTRIB_FOG][0] ) {
                pState->diff_api.FogCoordfvEXT ((GLfloat *) &(to->vertexAttrib[VERT_ATTRIB_FOG][0] ));
                FILLDIRTY(c->vertexAttrib[VERT_ATTRIB_FOG]);
                FILLDIRTY(c->dirty);
            }
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_FOG], nbitID);
        }
#endif

        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_NORMAL], bitID)) {
            if (COMPARE_VECTOR (from->vertexAttrib[VERT_ATTRIB_NORMAL], to->vertexAttrib[VERT_ATTRIB_NORMAL])) {
                pState->diff_api.Normal3fv ((GLfloat *) &(to->vertexAttrib[VERT_ATTRIB_NORMAL][0]));
                FILLDIRTY(c->vertexAttrib[VERT_ATTRIB_NORMAL]);
                FILLDIRTY(c->dirty);
            }
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_NORMAL], nbitID);
        }

        for (i = 0; i < maxTextureUnits; i++)   {
            if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_TEX0 + i], bitID)) {
                if (COMPARE_TEXCOORD (from->vertexAttrib[VERT_ATTRIB_TEX0 + i], to->vertexAttribPre[VERT_ATTRIB_TEX0 + i])) {
                    pState->diff_api.MultiTexCoord4fvARB (i+GL_TEXTURE0_ARB, (GLfloat *) &(to->vertexAttrib[VERT_ATTRIB_TEX0+ i][0]));
                    FILLDIRTY(c->vertexAttrib[VERT_ATTRIB_TEX0 + i]);
                    FILLDIRTY(c->dirty);
                }
                CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_TEX0 + i], nbitID);
            }
        }
    }

    CLEARDIRTY(c->dirty, nbitID);
}

void
crStateCurrentDiff( CRCurrentBits *c, CRbitvalue *bitID,
                    CRContext *fromCtx, CRContext *toCtx )
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRCurrentState *from = &(fromCtx->current);
    const CRCurrentState *to = &(toCtx->current);
    unsigned int i, j;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (CHECKDIRTY(c->rasterPos, bitID)) {
        from->rasterValid = to->rasterValid;
        if (to->rasterValid) {
            const GLfloat fromX = from->rasterAttrib[VERT_ATTRIB_POS][0];
            const GLfloat fromY = from->rasterAttrib[VERT_ATTRIB_POS][1];
            const GLfloat fromZ = from->rasterAttrib[VERT_ATTRIB_POS][2];
            const GLfloat toX = to->rasterAttrib[VERT_ATTRIB_POS][0];
            const GLfloat toY = to->rasterAttrib[VERT_ATTRIB_POS][1];
            const GLfloat toZ = to->rasterAttrib[VERT_ATTRIB_POS][2];
            if (toX != fromX || toY != fromY || toZ != fromZ) {
                /* Use glWindowPos (which updates raster color) */
                pState->diff_api.WindowPos3fvARB(to->rasterAttrib[VERT_ATTRIB_POS]);
                from->rasterAttrib[VERT_ATTRIB_POS][0] = toX;
                from->rasterAttrib[VERT_ATTRIB_POS][1] = toY;
                from->rasterAttrib[VERT_ATTRIB_POS][2] = toZ;
            }
        }
        CLEARDIRTY(c->rasterPos, nbitID);
    }

    /* Vertex Current State Sync Code */
    /* Some things to note here:
    ** 1) Compare is done against the pre value since the
    **    current value includes the geometry info.
    ** 2) Update is done with the current value since
    **    the server will be getting the geometry block
    ** 3) Copy is done outside of the compare to ensure
    **    that it happens.
    */

    /* edge flag */
    if (CHECKDIRTY(c->edgeFlag, bitID)) {
        if (from->edgeFlag != to->edgeFlagPre) {
            pState->diff_api.EdgeFlag (to->edgeFlagPre);
        }
        from->edgeFlag = to->edgeFlag;
        CLEARDIRTY(c->edgeFlag, nbitID);
    }

    /* color index */
    if (CHECKDIRTY(c->colorIndex, bitID)) {
        if (from->colorIndex != to->colorIndexPre) {
            pState->diff_api.Indexf (to->colorIndex);
        }
        from->colorIndex = to->colorIndex;
        CLEARDIRTY(c->colorIndex, nbitID);
    }


    /* If using a vertex program, update the generic vertex attributes,
     * which may or may not be aliased with conventional attributes.
     */
#if defined(CR_ARB_vertex_program) || defined(CR_NV_vertex_progra)
    if (toCtx->program.vpEnabled &&
            (toCtx->extensions.ARB_vertex_program ||
             (toCtx->extensions.NV_vertex_program))) {
        const unsigned attribsUsedMask = toCtx->current.attribsUsedMask;
        for (i = 1; i < CR_MAX_VERTEX_ATTRIBS; i++) {  /* skip zero */
            if ((attribsUsedMask & (1 << i))
                    && CHECKDIRTY(c->vertexAttrib[i], bitID)) {
                if (COMPARE_VECTOR (from->vertexAttrib[i], to->vertexAttribPre[i])) {
                    pState->diff_api.VertexAttrib4fvARB(i, &(to->vertexAttribPre[i][0]));
                }
                COPY_4V(from->vertexAttrib[i] , to->vertexAttrib[i]);
                CLEARDIRTY(c->vertexAttrib[i], nbitID);
            }
        }
    }
    /* Fall-through so that attributes which don't have their bit set in the
     * attribsUsedMask get handled via the conventional attribute functions.
     */
#endif

    {
        /* use conventional attribute functions */
        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR0], bitID)) {
            if (COMPARE_COLOR(from->vertexAttrib[VERT_ATTRIB_COLOR0],to->vertexAttribPre[VERT_ATTRIB_COLOR0])) {
                pState->diff_api.Color4fv ((GLfloat *) &(to->vertexAttribPre[VERT_ATTRIB_COLOR0]));
            }
            COPY_4V(from->vertexAttrib[VERT_ATTRIB_COLOR0] , to->vertexAttrib[VERT_ATTRIB_COLOR0]);
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR0], nbitID);
        }

#ifdef CR_EXT_secondary_color
        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR1], bitID)) {
            if (COMPARE_COLOR(from->vertexAttrib[VERT_ATTRIB_COLOR1],to->vertexAttribPre[VERT_ATTRIB_COLOR1])) {
                pState->diff_api.SecondaryColor3fvEXT ((GLfloat *) &(to->vertexAttribPre[VERT_ATTRIB_COLOR1]));
            }
            COPY_4V(from->vertexAttrib[VERT_ATTRIB_COLOR1] , to->vertexAttrib[VERT_ATTRIB_COLOR1]);
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_COLOR1], nbitID);
        }
#endif

#ifdef CR_EXT_fog_coord
        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_FOG], bitID)) {
            if (from->vertexAttrib[VERT_ATTRIB_FOG]  != to->vertexAttribPre[VERT_ATTRIB_FOG]) {
                pState->diff_api.FogCoordfvEXT ((GLfloat *) &(to->vertexAttribPre[VERT_ATTRIB_FOG]));
            }
            COPY_4V(from->vertexAttrib[VERT_ATTRIB_FOG]  , to->vertexAttrib[VERT_ATTRIB_FOG]);
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_FOG], nbitID);
        }
#endif

        if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_NORMAL], bitID)) {
            if (COMPARE_VECTOR (from->vertexAttrib[VERT_ATTRIB_NORMAL], to->vertexAttribPre[VERT_ATTRIB_NORMAL])) {
                pState->diff_api.Normal3fv ((GLfloat *) &(to->vertexAttribPre[VERT_ATTRIB_NORMAL]));
            }
            COPY_4V(from->vertexAttrib[VERT_ATTRIB_NORMAL] , to->vertexAttrib[VERT_ATTRIB_NORMAL]);
            CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_NORMAL], nbitID);
        }

        for ( i = 0 ; i < fromCtx->limits.maxTextureUnits ; i++)
        {
            if (CHECKDIRTY(c->vertexAttrib[VERT_ATTRIB_TEX0 + i], bitID)) {
                if (COMPARE_TEXCOORD (from->vertexAttrib[VERT_ATTRIB_TEX0 + i], to->vertexAttribPre[VERT_ATTRIB_TEX0 + i])) {
                    pState->diff_api.MultiTexCoord4fvARB (GL_TEXTURE0_ARB + i, (GLfloat *) &(to->vertexAttribPre[VERT_ATTRIB_TEX0 + i]));
                }
                COPY_4V(from->vertexAttrib[VERT_ATTRIB_TEX0 + i] , to->vertexAttrib[VERT_ATTRIB_TEX0 + i]);
                CLEARDIRTY(c->vertexAttrib[VERT_ATTRIB_TEX0 + i], nbitID);
            }
        }
    }

    CLEARDIRTY(c->dirty, nbitID);
}
