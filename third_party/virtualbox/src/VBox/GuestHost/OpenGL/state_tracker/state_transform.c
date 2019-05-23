/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_bits.h"
#include "cr_mem.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

#ifdef WINDOWS
#pragma warning( disable : 4514 )  
#endif

/*
 * This used to be a macro.
 */
static INLINE void
LOADMATRIX( const CRmatrix *a )
{
    if (a->m00 == 1.0F && a->m01 == 0.0F && a->m02 == 0.0F && a->m03 == 0.0F &&
            a->m10 == 0.0F && a->m11 == 1.0F && a->m12 == 0.0F && a->m13 == 0.0F &&
            a->m20 == 0.0F && a->m21 == 0.0F && a->m22 == 1.0F && a->m23 == 0.0F &&
            a->m30 == 0.0F && a->m31 == 0.0F && a->m32 == 0.0F && a->m33 == 1.0F) {
        diff_api.LoadIdentity();
    }
    else {
        GLfloat f[16];
        f[0] = a->m00;  f[1] = a->m01;  f[2] = a->m02;  f[3] = a->m03;
        f[4] = a->m10;  f[5] = a->m11;  f[6] = a->m12;  f[7] = a->m13;
        f[8] = a->m20;  f[9] = a->m21;  f[10] = a->m22; f[11] = a->m23;
        f[12] = a->m30; f[13] = a->m31; f[14] = a->m32; f[15] = a->m33;
        diff_api.LoadMatrixf(f);
    }
}


static void _math_transposef( GLfloat to[16], const GLfloat from[16] )
{
   to[0] = from[0];
   to[1] = from[4];
   to[2] = from[8];
   to[3] = from[12];
   to[4] = from[1];
   to[5] = from[5];
   to[6] = from[9];
   to[7] = from[13];
   to[8] = from[2];
   to[9] = from[6];
   to[10] = from[10];
   to[11] = from[14];
   to[12] = from[3];
   to[13] = from[7];
   to[14] = from[11];
   to[15] = from[15];
}

static void _math_transposed( GLdouble to[16], const GLdouble from[16] )
{
    to[0] = from[0];
    to[1] = from[4];
    to[2] = from[8];
    to[3] = from[12];
    to[4] = from[1];
    to[5] = from[5];
    to[6] = from[9];
    to[7] = from[13];
    to[8] = from[2];
    to[9] = from[6];
    to[10] = from[10];
    to[11] = from[14];
    to[12] = from[3];
    to[13] = from[7];
    to[14] = from[11];
    to[15] = from[15];
}


void
crStateInitMatrixStack(CRMatrixStack *stack, int maxDepth)
{
    stack->maxDepth = maxDepth;
    stack->depth = 0;
    stack->stack = crAlloc(maxDepth * sizeof(CRmatrix));
    crMatrixInit(&stack->stack[0]);
    stack->top = stack->stack;
}

static void
free_matrix_stack_data(CRMatrixStack *stack)
{
    crFree(stack->stack);
}

void crStateTransformDestroy(CRContext *ctx)
{
    CRTransformState *t = &ctx->transform;
    unsigned int i;

    free_matrix_stack_data(&(t->modelViewStack));
    free_matrix_stack_data(&(t->projectionStack));
    free_matrix_stack_data(&(t->colorStack));
    for (i = 0 ; i < ctx->limits.maxTextureUnits; i++)
        free_matrix_stack_data(&(t->textureStack[i]));
    for (i = 0; i < CR_MAX_PROGRAM_MATRICES; i++)
        free_matrix_stack_data(&(t->programStack[i]));

    crFree( t->clipPlane );
    crFree( t->clip );
}

void crStateTransformInit(CRContext *ctx)
{
    CRLimitsState *limits = &ctx->limits;
    CRTransformState *t = &ctx->transform;
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);
    unsigned int i;

    t->matrixMode = GL_MODELVIEW;
    RESET(tb->matrixMode, ctx->bitid);

    crStateInitMatrixStack(&t->modelViewStack, CR_MAX_MODELVIEW_STACK_DEPTH);
    crStateInitMatrixStack(&t->projectionStack, CR_MAX_PROJECTION_STACK_DEPTH);
    crStateInitMatrixStack(&t->colorStack, CR_MAX_COLOR_STACK_DEPTH);
    for (i = 0 ; i < limits->maxTextureUnits ; i++)
        crStateInitMatrixStack(&t->textureStack[i], CR_MAX_TEXTURE_STACK_DEPTH);
    for (i = 0 ; i < CR_MAX_PROGRAM_MATRICES ; i++)
        crStateInitMatrixStack(&t->programStack[i], CR_MAX_PROGRAM_MATRIX_STACK_DEPTH);
    t->currentStack = &(t->modelViewStack);

    /* dirty bits */
    RESET(tb->modelviewMatrix, ctx->bitid);
    RESET(tb->projectionMatrix, ctx->bitid);
    RESET(tb->colorMatrix, ctx->bitid);
    RESET(tb->textureMatrix, ctx->bitid);
    RESET(tb->programMatrix, ctx->bitid);
    tb->currentMatrix = tb->modelviewMatrix;

    t->normalize = GL_FALSE;
    RESET(tb->enable, ctx->bitid);

    t->clipPlane = (GLvectord *) crCalloc (sizeof (GLvectord) * CR_MAX_CLIP_PLANES);
    t->clip = (GLboolean *) crCalloc (sizeof (GLboolean) * CR_MAX_CLIP_PLANES);
    for (i = 0; i < CR_MAX_CLIP_PLANES; i++) 
    {
        t->clipPlane[i].x = 0.0f;
        t->clipPlane[i].y = 0.0f;
        t->clipPlane[i].z = 0.0f;
        t->clipPlane[i].w = 0.0f;
        t->clip[i] = GL_FALSE;
    }
    RESET(tb->clipPlane, ctx->bitid);

#ifdef CR_OPENGL_VERSION_1_2
    t->rescaleNormals = GL_FALSE;
#endif
#ifdef CR_IBM_rasterpos_clip
    t->rasterPositionUnclipped = GL_FALSE;
#endif

    t->modelViewProjectionValid = 0;

    RESET(tb->dirty, ctx->bitid);
}


/*
 * Compute the product of the modelview and projection matrices
 */
void crStateTransformUpdateTransform(CRTransformState *t) 
{
    const CRmatrix *m1 = t->projectionStack.top;
    const CRmatrix *m2 = t->modelViewStack.top;
    CRmatrix *m = &(t->modelViewProjection);
    crMatrixMultiply(m, m1, m2);
    t->modelViewProjectionValid = 1;
}

void crStateTransformXformPoint(CRTransformState *t, GLvectorf *p) 
{
    /* XXX is this flag really needed?  We may be covering a bug elsewhere */
    if (!t->modelViewProjectionValid)
        crStateTransformUpdateTransform(t);

    crStateTransformXformPointMatrixf(&(t->modelViewProjection), p);
}

void crStateTransformXformPointMatrixf(const CRmatrix *m, GLvectorf *p) 
{
    GLfloat x = p->x;
    GLfloat y = p->y;
    GLfloat z = p->z;
    GLfloat w = p->w;

    p->x = m->m00*x + m->m10*y + m->m20*z + m->m30*w;
    p->y = m->m01*x + m->m11*y + m->m21*z + m->m31*w;
    p->z = m->m02*x + m->m12*y + m->m22*z + m->m32*w;
    p->w = m->m03*x + m->m13*y + m->m23*z + m->m33*w;
}

void crStateTransformXformPointMatrixd(const CRmatrix *m, GLvectord *p)
{
    GLdouble x = p->x;
    GLdouble y = p->y;
    GLdouble z = p->z;
    GLdouble w = p->w;

    p->x = (GLdouble) (m->m00*x + m->m10*y + m->m20*z + m->m30*w);
    p->y = (GLdouble) (m->m01*x + m->m11*y + m->m21*z + m->m31*w);
    p->z = (GLdouble) (m->m02*x + m->m12*y + m->m22*z + m->m32*w);
    p->w = (GLdouble) (m->m03*x + m->m13*y + m->m23*z + m->m33*w);
}

void STATE_APIENTRY crStateClipPlane (GLenum plane, const GLdouble *equation)
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &g->transform;
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);
    GLvectord e;
    unsigned int i;
    CRmatrix inv;

    e.x = equation[0];
    e.y = equation[1];
    e.z = equation[2];
    e.w = equation[3];

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                     "ClipPlane called in begin/end");
        return;
    }

    FLUSH();

    i = plane - GL_CLIP_PLANE0;
    if (i >= g->limits.maxClipPlanes)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                     "ClipPlane called with bad enumerant: %d", plane);
        return;
    }

    crMatrixInvertTranspose(&inv, t->modelViewStack.top);
    crStateTransformXformPointMatrixd (&inv, &e);
    t->clipPlane[i] = e;
    DIRTY(tb->clipPlane, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateMatrixMode(GLenum e) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRTextureState *tex = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "MatrixMode called in begin/end");
        return;
    }

    FLUSH();

    switch (e) 
    {
        case GL_MODELVIEW:
            t->currentStack = &(t->modelViewStack);
            t->matrixMode = GL_MODELVIEW;
            tb->currentMatrix = tb->modelviewMatrix;
            break;
        case GL_PROJECTION:
            t->currentStack = &(t->projectionStack);
            t->matrixMode = GL_PROJECTION;
            tb->currentMatrix = tb->projectionMatrix;
            break;
        case GL_TEXTURE:
            t->currentStack = &(t->textureStack[tex->curTextureUnit]);
            t->matrixMode = GL_TEXTURE;
            tb->currentMatrix = tb->textureMatrix;
            break;
        case GL_COLOR:
            t->currentStack = &(t->colorStack);
            t->matrixMode = GL_COLOR;
            tb->currentMatrix = tb->colorMatrix;
            break;
        case GL_MATRIX0_NV:
        case GL_MATRIX1_NV:
        case GL_MATRIX2_NV:
        case GL_MATRIX3_NV:
        case GL_MATRIX4_NV:
        case GL_MATRIX5_NV:
        case GL_MATRIX6_NV:
        case GL_MATRIX7_NV:
            if (g->extensions.NV_vertex_program) {
                const GLint i = e - GL_MATRIX0_NV;
                t->currentStack = &(t->programStack[i]);
                t->matrixMode = e;
                tb->currentMatrix = tb->programMatrix;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid matrix mode: %d", e);
                return;
            }
            break;
        case GL_MATRIX0_ARB:
        case GL_MATRIX1_ARB:
        case GL_MATRIX2_ARB:
        case GL_MATRIX3_ARB:
        case GL_MATRIX4_ARB:
        case GL_MATRIX5_ARB:
        case GL_MATRIX6_ARB:
        case GL_MATRIX7_ARB:
            /* Note: the NV and ARB program matrices are the same, but
             * use different enumerants.
             */
            if (g->extensions.ARB_vertex_program) {
                const GLint i = e - GL_MATRIX0_ARB;
                t->currentStack = &(t->programStack[i]);
                t->matrixMode = e;
                tb->currentMatrix = tb->programMatrix;
            }
            else {
                 crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid matrix mode: %d", e);
                 return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid matrix mode: %d", e);
            return;
    }
    DIRTY(tb->matrixMode, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);

    CRASSERT(t->currentStack->top == t->currentStack->stack + t->currentStack->depth);
}

void STATE_APIENTRY crStateLoadIdentity() 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "LoadIdentity called in begin/end");
        return;
    }

    FLUSH();

    crMatrixInit(t->currentStack->top);
    t->modelViewProjectionValid = 0;

    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePopMatrix() 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &g->transform;
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "PopMatrix called in begin/end");
        return;
    }

    FLUSH();

    if (t->currentStack->depth == 0)
    {
        crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "PopMatrix of empty stack.");
        return;
    }

    CRASSERT(t->currentStack->top == t->currentStack->stack + t->currentStack->depth);

    t->currentStack->depth--;
    t->currentStack->top = t->currentStack->stack + t->currentStack->depth;

    t->modelViewProjectionValid = 0;

    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePushMatrix() 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &g->transform;
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "PushMatrix called in begin/end");
        return;
    }

    FLUSH();

    if (t->currentStack->depth + 1 >= t->currentStack->maxDepth)
    {
        crStateError(__LINE__, __FILE__, GL_STACK_OVERFLOW, "PushMatrix pass the end of allocated stack");
        return;
    }

    CRASSERT(t->currentStack->top == t->currentStack->stack + t->currentStack->depth);
    /* Perform the copy */
    *(t->currentStack->top + 1) = *(t->currentStack->top);

    /* Move the stack pointer */
    t->currentStack->depth++;
    t->currentStack->top = t->currentStack->stack + t->currentStack->depth;

    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


/* Load a CRMatrix */
void crStateLoadMatrix(const CRmatrix *m)
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "LoadMatrix called in begin/end");
        return;
    }

    FLUSH();

    CRASSERT(t->currentStack->top == t->currentStack->stack + t->currentStack->depth);
    *t->currentStack->top = *m;
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}



void STATE_APIENTRY crStateLoadMatrixf(const GLfloat *m1) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "LoadMatrixf called in begin/end");
        return;
    }

    FLUSH();

    crMatrixInitFromFloats(t->currentStack->top, m1);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateLoadMatrixd(const GLdouble *m1) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "LoadMatrixd called in begin/end");
        return;
    }

    FLUSH();

    crMatrixInitFromDoubles(t->currentStack->top, m1);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateLoadTransposeMatrixfARB(const GLfloat *m1) 
{
   GLfloat tm[16];
   if (!m1) return;
   _math_transposef(tm, m1);
   crStateLoadMatrixf(tm);
}

void STATE_APIENTRY crStateLoadTransposeMatrixdARB(const GLdouble *m1) 
{
   GLdouble tm[16];
   if (!m1) return;
   _math_transposed(tm, m1);
   crStateLoadMatrixd(tm);
}

/* This code is based on the Pomegranate stuff.
 ** The theory is that by preloading everything,
 ** the compiler could do optimizations that 
 ** were not doable in the array version
 ** I'm not too sure with a PII with 4 registers
 ** that this really helps.
 */ 
void STATE_APIENTRY crStateMultMatrixf(const GLfloat *m1) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);
    CRmatrix *m = t->currentStack->top;

    const GLdefault lm00 = m->m00;  
    const GLdefault lm01 = m->m01;  
    const GLdefault lm02 = m->m02;  
    const GLdefault lm03 = m->m03;  
    const GLdefault lm10 = m->m10;  
    const GLdefault lm11 = m->m11;  
    const GLdefault lm12 = m->m12;  
    const GLdefault lm13 = m->m13;  
    const GLdefault lm20 = m->m20;  
    const GLdefault lm21 = m->m21;  
    const GLdefault lm22 = m->m22;  
    const GLdefault lm23 = m->m23;  
    const GLdefault lm30 = m->m30;  
    const GLdefault lm31 = m->m31;  
    const GLdefault lm32 = m->m32;      
    const GLdefault lm33 = m->m33;      
    const GLdefault rm00 = (GLdefault) m1[0];   
    const GLdefault rm01 = (GLdefault) m1[1];   
    const GLdefault rm02 = (GLdefault) m1[2];   
    const GLdefault rm03 = (GLdefault) m1[3];   
    const GLdefault rm10 = (GLdefault) m1[4];   
    const GLdefault rm11 = (GLdefault) m1[5];       
    const GLdefault rm12 = (GLdefault) m1[6];   
    const GLdefault rm13 = (GLdefault) m1[7];   
    const GLdefault rm20 = (GLdefault) m1[8];   
    const GLdefault rm21 = (GLdefault) m1[9];   
    const GLdefault rm22 = (GLdefault) m1[10];  
    const GLdefault rm23 = (GLdefault) m1[11];  
    const GLdefault rm30 = (GLdefault) m1[12];  
    const GLdefault rm31 = (GLdefault) m1[13];  
    const GLdefault rm32 = (GLdefault) m1[14];  
    const GLdefault rm33 = (GLdefault) m1[15];  

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "MultMatrixf called in begin/end");
        return;
    }

    FLUSH();

    m->m00 = lm00*rm00 + lm10*rm01 + lm20*rm02 + lm30*rm03; 
    m->m01 = lm01*rm00 + lm11*rm01 + lm21*rm02 + lm31*rm03; 
    m->m02 = lm02*rm00 + lm12*rm01 + lm22*rm02 + lm32*rm03; 
    m->m03 = lm03*rm00 + lm13*rm01 + lm23*rm02 + lm33*rm03; 
    m->m10 = lm00*rm10 + lm10*rm11 + lm20*rm12 + lm30*rm13; 
    m->m11 = lm01*rm10 + lm11*rm11 + lm21*rm12 + lm31*rm13; 
    m->m12 = lm02*rm10 + lm12*rm11 + lm22*rm12 + lm32*rm13; 
    m->m13 = lm03*rm10 + lm13*rm11 + lm23*rm12 + lm33*rm13; 
    m->m20 = lm00*rm20 + lm10*rm21 + lm20*rm22 + lm30*rm23; 
    m->m21 = lm01*rm20 + lm11*rm21 + lm21*rm22 + lm31*rm23; 
    m->m22 = lm02*rm20 + lm12*rm21 + lm22*rm22 + lm32*rm23; 
    m->m23 = lm03*rm20 + lm13*rm21 + lm23*rm22 + lm33*rm23; 
    m->m30 = lm00*rm30 + lm10*rm31 + lm20*rm32 + lm30*rm33; 
    m->m31 = lm01*rm30 + lm11*rm31 + lm21*rm32 + lm31*rm33; 
    m->m32 = lm02*rm30 + lm12*rm31 + lm22*rm32 + lm32*rm33; 
    m->m33 = lm03*rm30 + lm13*rm31 + lm23*rm32 + lm33*rm33;

    t->modelViewProjectionValid = 0;

    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateMultMatrixd(const GLdouble *m1) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);
    CRmatrix *m = t->currentStack->top;
    const GLdefault lm00 = m->m00;  
    const GLdefault lm01 = m->m01;  
    const GLdefault lm02 = m->m02;  
    const GLdefault lm03 = m->m03;  
    const GLdefault lm10 = m->m10;  
    const GLdefault lm11 = m->m11;  
    const GLdefault lm12 = m->m12;  
    const GLdefault lm13 = m->m13;  
    const GLdefault lm20 = m->m20;  
    const GLdefault lm21 = m->m21;  
    const GLdefault lm22 = m->m22;  
    const GLdefault lm23 = m->m23;  
    const GLdefault lm30 = m->m30;  
    const GLdefault lm31 = m->m31;  
    const GLdefault lm32 = m->m32;      
    const GLdefault lm33 = m->m33;      
    const GLdefault rm00 = (GLdefault) m1[0];   
    const GLdefault rm01 = (GLdefault) m1[1];   
    const GLdefault rm02 = (GLdefault) m1[2];   
    const GLdefault rm03 = (GLdefault) m1[3];   
    const GLdefault rm10 = (GLdefault) m1[4];   
    const GLdefault rm11 = (GLdefault) m1[5];       
    const GLdefault rm12 = (GLdefault) m1[6];   
    const GLdefault rm13 = (GLdefault) m1[7];   
    const GLdefault rm20 = (GLdefault) m1[8];   
    const GLdefault rm21 = (GLdefault) m1[9];   
    const GLdefault rm22 = (GLdefault) m1[10];  
    const GLdefault rm23 = (GLdefault) m1[11];  
    const GLdefault rm30 = (GLdefault) m1[12];  
    const GLdefault rm31 = (GLdefault) m1[13];  
    const GLdefault rm32 = (GLdefault) m1[14];  
    const GLdefault rm33 = (GLdefault) m1[15];  

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "MultMatrixd called in begin/end");
        return;
    }

    FLUSH();

    m->m00 = lm00*rm00 + lm10*rm01 + lm20*rm02 + lm30*rm03; 
    m->m01 = lm01*rm00 + lm11*rm01 + lm21*rm02 + lm31*rm03; 
    m->m02 = lm02*rm00 + lm12*rm01 + lm22*rm02 + lm32*rm03; 
    m->m03 = lm03*rm00 + lm13*rm01 + lm23*rm02 + lm33*rm03; 
    m->m10 = lm00*rm10 + lm10*rm11 + lm20*rm12 + lm30*rm13; 
    m->m11 = lm01*rm10 + lm11*rm11 + lm21*rm12 + lm31*rm13; 
    m->m12 = lm02*rm10 + lm12*rm11 + lm22*rm12 + lm32*rm13; 
    m->m13 = lm03*rm10 + lm13*rm11 + lm23*rm12 + lm33*rm13; 
    m->m20 = lm00*rm20 + lm10*rm21 + lm20*rm22 + lm30*rm23; 
    m->m21 = lm01*rm20 + lm11*rm21 + lm21*rm22 + lm31*rm23; 
    m->m22 = lm02*rm20 + lm12*rm21 + lm22*rm22 + lm32*rm23; 
    m->m23 = lm03*rm20 + lm13*rm21 + lm23*rm22 + lm33*rm23; 
    m->m30 = lm00*rm30 + lm10*rm31 + lm20*rm32 + lm30*rm33; 
    m->m31 = lm01*rm30 + lm11*rm31 + lm21*rm32 + lm31*rm33; 
    m->m32 = lm02*rm30 + lm12*rm31 + lm22*rm32 + lm32*rm33; 
    m->m33 = lm03*rm30 + lm13*rm31 + lm23*rm32 + lm33*rm33;

    t->modelViewProjectionValid = 0;

    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateMultTransposeMatrixfARB(const GLfloat *m1) 
{
   GLfloat tm[16];
   if (!m1) return;
   _math_transposef(tm, m1);
   crStateMultMatrixf(tm);
}

void STATE_APIENTRY crStateMultTransposeMatrixdARB(const GLdouble *m1) 
{
   GLdouble tm[16];
   if (!m1) return;
   _math_transposed(tm, m1);
   crStateMultMatrixd(tm);
}

void STATE_APIENTRY crStateTranslatef(GLfloat x_arg, GLfloat y_arg, GLfloat z_arg) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Translatef called in begin/end");
        return;
    }

    FLUSH();

    crMatrixTranslate(t->currentStack->top, x_arg, y_arg, z_arg);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateTranslated(GLdouble x_arg, GLdouble y_arg, GLdouble z_arg) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Translated called in begin/end");
        return;
    }

    FLUSH();

    crMatrixTranslate(t->currentStack->top, (float)x_arg, (float)y_arg, (float)z_arg);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}   


void STATE_APIENTRY crStateRotatef(GLfloat ang, GLfloat x, GLfloat y, GLfloat z) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Rotatef called in begin/end");
        return;
    }

    FLUSH();

    crMatrixRotate(t->currentStack->top, ang, x, y, z);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateRotated(GLdouble ang, GLdouble x, GLdouble y, GLdouble z) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Rotated called in begin/end");
        return;
    }

    FLUSH();

    crMatrixRotate(t->currentStack->top, (float)ang, (float)x, (float)y, (float)z);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}   

void STATE_APIENTRY crStateScalef (GLfloat x_arg, GLfloat y_arg, GLfloat z_arg) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Scalef called in begin/end");
        return;
    }

    FLUSH();

    crMatrixScale(t->currentStack->top, x_arg, y_arg, z_arg);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateScaled (GLdouble x_arg, GLdouble y_arg, GLdouble z_arg) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Scaled called in begin/end");
        return;
    }

    FLUSH();

    crMatrixScale(t->currentStack->top, (float)x_arg, (float)y_arg, (float)z_arg);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateFrustum(GLdouble left, GLdouble right,
                                                                     GLdouble bottom, GLdouble top, 
                                                                     GLdouble zNear, GLdouble zFar)
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Frustum called in begin/end");
        return;
    }

    FLUSH();

    crMatrixFrustum(t->currentStack->top, (float)left, (float)right, (float)bottom, (float)top, (float)zNear, (float)zFar);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateOrtho(GLdouble left, GLdouble right,
                                                                 GLdouble bottom, GLdouble top,
                                                                 GLdouble zNear, GLdouble zFar)
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &(g->transform);
    CRStateBits *sb = GetCurrentBits();
    CRTransformBits *tb = &(sb->transform);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "Ortho called in begin/end");
        return;
    }

    FLUSH();

    crMatrixOrtho(t->currentStack->top, (float)left, (float)right, (float)bottom, (float)top, (float)zNear, (float)zFar);
    t->modelViewProjectionValid = 0;
    DIRTY(tb->currentMatrix, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateGetClipPlane(GLenum plane, GLdouble *equation) 
{
    CRContext *g = GetCurrentContext();
    CRTransformState *t = &g->transform;
    unsigned int i;
    
    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
            "glGetClipPlane called in begin/end");
        return;
    }

    i = plane - GL_CLIP_PLANE0;
    if (i >= g->limits.maxClipPlanes)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, 
            "GetClipPlane called with bad enumerant: %d", plane);
        return;
    }

    equation[0] = t->clipPlane[i].x;
    equation[1] = t->clipPlane[i].y;
    equation[2] = t->clipPlane[i].z;
    equation[3] = t->clipPlane[i].w;
}

void crStateTransformSwitch( CRTransformBits *t, CRbitvalue *bitID, 
                                                         CRContext *fromCtx, CRContext *toCtx )
{
    const GLuint maxTextureUnits = toCtx->limits.maxTextureUnits;
    CRTransformState *from = &(fromCtx->transform);
    CRTransformState *to = &(toCtx->transform);
    GLuint i, j;
    unsigned int checktex = 0;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (CHECKDIRTY(t->enable, bitID))
    {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->normalize != to->normalize) {
            if (to->normalize == GL_TRUE)
                diff_api.Enable(GL_NORMALIZE);
            else
                diff_api.Disable(GL_NORMALIZE);
            FILLDIRTY(t->enable);
            FILLDIRTY(t->dirty);
        }
#ifdef CR_OPENGL_VERSION_1_2
        if (from->rescaleNormals != to->rescaleNormals)
        {
            able[to->rescaleNormals](GL_RESCALE_NORMAL);
            FILLDIRTY(t->enable);
            FILLDIRTY(t->dirty);
        }
#else
        (void) able;
#endif
#ifdef CR_IBM_rasterpos_clip
        if (from->rasterPositionUnclipped != to->rasterPositionUnclipped)
        {
            able[to->rasterPositionUnclipped](GL_RASTER_POSITION_UNCLIPPED_IBM);
            FILLDIRTY(t->enable);
            FILLDIRTY(t->dirty);
        }
#endif
        CLEARDIRTY(t->enable, nbitID);
    }

    if (CHECKDIRTY(t->clipPlane, bitID)) {
        diff_api.MatrixMode(GL_MODELVIEW);
        diff_api.PushMatrix();
        diff_api.LoadIdentity();
        for (i=0; i<CR_MAX_CLIP_PLANES; i++) {
            if (from->clipPlane[i].x != to->clipPlane[i].x ||
                from->clipPlane[i].y != to->clipPlane[i].y ||
                from->clipPlane[i].z != to->clipPlane[i].z ||
                from->clipPlane[i].w != to->clipPlane[i].w) {

                GLdouble cp[4];
                cp[0] = to->clipPlane[i].x;
                cp[1] = to->clipPlane[i].y;
                cp[2] = to->clipPlane[i].z;
                cp[3] = to->clipPlane[i].w;

                diff_api.ClipPlane(GL_CLIP_PLANE0 + i, (const GLdouble *)(cp));

                FILLDIRTY(t->clipPlane);
                FILLDIRTY(t->dirty);
            }
        }
        diff_api.PopMatrix();
        CLEARDIRTY(t->clipPlane, nbitID);
    }

    /* If the matrix stack depths don't match when we're
     * updating the context - we can update the wrong matrix
     * and get some lovely effects!! 
     * So we troll each depth list here and Pop & Push the matrix stack
     * to bring it right up to date before checking the current
     * matrix.
     *
     * - Alan H.
     */
    if ( (from->modelViewStack.depth != to->modelViewStack.depth) ||
          CHECKDIRTY(t->modelviewMatrix, bitID) )
    {
        GLuint td = to->modelViewStack.depth;
        GLuint fd = from->modelViewStack.depth;

        if (td != fd ||
                !crMatrixIsEqual(to->modelViewStack.top, from->modelViewStack.top))
        {
            diff_api.MatrixMode(GL_MODELVIEW);

            if (fd > td)
            {
                for (i = td; i < fd; i++) 
                {
                    diff_api.PopMatrix();
                }
                fd = td;
            }

            for (i = fd; i <= td; i++)
            {
                LOADMATRIX(to->modelViewStack.stack + i);
                FILLDIRTY(t->modelviewMatrix);
                FILLDIRTY(t->dirty);

                /* Don't want to push on the current matrix */
                if (i != to->modelViewStack.depth)
                    diff_api.PushMatrix();
            }
        }
        CLEARDIRTY(t->modelviewMatrix, nbitID);
    }

    /* Projection matrix */
    if ( (from->projectionStack.depth != to->projectionStack.depth) ||
          CHECKDIRTY(t->projectionMatrix, bitID) )
    {
        GLuint td = to->projectionStack.depth;
        GLuint fd = from->projectionStack.depth;

        if (td != fd ||
                !crMatrixIsEqual(to->projectionStack.top, from->projectionStack.top)) {

            diff_api.MatrixMode(GL_PROJECTION);

            if (fd > td)
            {
                for (i = td; i < fd; i++) 
                {
                    diff_api.PopMatrix();
                }
                fd = td;
            }

            for (i = fd; i <= td; i++)
            {
                LOADMATRIX(to->projectionStack.stack + i);
                FILLDIRTY(t->projectionMatrix);
                FILLDIRTY(t->dirty);

                /* Don't want to push on the current matrix */
                if (i != to->projectionStack.depth)
                    diff_api.PushMatrix();
            }
        }
        CLEARDIRTY(t->projectionMatrix, nbitID);
    }

    /* Texture matrices */
    for (i = 0 ; i < maxTextureUnits ; i++)
        if (from->textureStack[i].depth != to->textureStack[i].depth)
            checktex = 1;
    
    if ( checktex || CHECKDIRTY(t->textureMatrix, bitID) )
    {
        for (j = 0 ; j < maxTextureUnits ; j++)
        {
            GLuint td = to->textureStack[j].depth;
            GLuint fd = from->textureStack[j].depth;

            if (td != fd ||
                    !crMatrixIsEqual(to->textureStack[j].top, from->textureStack[j].top))
            {
                diff_api.MatrixMode(GL_TEXTURE);

                if (fd > td)
                {
                    for (i = td; i < fd; i++) 
                    {
                        diff_api.PopMatrix();
                    }
                    fd = td;
                }

                diff_api.ActiveTextureARB( j + GL_TEXTURE0_ARB );
                for (i = fd; i <= td; i++)
                {
                    LOADMATRIX(to->textureStack[j].stack + i);
                    FILLDIRTY(t->textureMatrix);
                    FILLDIRTY(t->dirty);

                    /* Don't want to push on the current matrix */
                    if (i != to->textureStack[j].depth)
                        diff_api.PushMatrix();
                }
            }
        }
        /* Since we were mucking with the active texture unit above set it to the
         * proper value now.  
         */
        diff_api.ActiveTextureARB(GL_TEXTURE0_ARB + toCtx->texture.curTextureUnit);
        CLEARDIRTY(t->textureMatrix, nbitID);
    }

    /* Color matrix */
    if ( (from->colorStack.depth != to->colorStack.depth) ||
          CHECKDIRTY(t->colorMatrix, bitID) )
    {
        GLuint td = to->colorStack.depth;
        GLuint fd = from->colorStack.depth;
        if (td != fd || !crMatrixIsEqual(to->colorStack.top, from->colorStack.top))
        {
            diff_api.MatrixMode(GL_COLOR);

            if (fd > td)
            {
                for (i = td; i < fd; i++) 
                {
                    diff_api.PopMatrix();
                }
                fd = td;
            }

            for (i = fd; i <= td; i++)
            {
                LOADMATRIX(to->colorStack.stack + i);
                FILLDIRTY(t->colorMatrix);
                FILLDIRTY(t->dirty);

                /* Don't want to push on the current matrix */
                if (i != to->colorStack.depth)
                    diff_api.PushMatrix();
            }
        }
        CLEARDIRTY(t->colorMatrix, nbitID);
    }

    to->modelViewProjectionValid = 0;
    CLEARDIRTY(t->dirty, nbitID);

    /* Since we were mucking with the current matrix above 
     * set it to the proper value now.  
     */
    diff_api.MatrixMode(to->matrixMode);

    /* sanity tests */
    CRASSERT(from->modelViewStack.top == from->modelViewStack.stack + from->modelViewStack.depth);
    CRASSERT(from->projectionStack.top == from->projectionStack.stack + from->projectionStack.depth);

}

void
crStateTransformDiff( CRTransformBits *t, CRbitvalue *bitID,
                                            CRContext *fromCtx, CRContext *toCtx )
{
    const GLuint maxTextureUnits = toCtx->limits.maxTextureUnits;
    CRTransformState *from = &(fromCtx->transform);
    CRTransformState *to = &(toCtx->transform);
    CRTextureState *textureFrom = &(fromCtx->texture);
    GLuint i, j;
    unsigned int checktex = 0;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (CHECKDIRTY(t->enable, bitID)) {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        for (i=0; i<CR_MAX_CLIP_PLANES; i++) {
            if (from->clip[i] != to->clip[i]) {
                if (to->clip[i] == GL_TRUE)
                    diff_api.Enable(GL_CLIP_PLANE0 + i);
                else
                    diff_api.Disable(GL_CLIP_PLANE0 + i);
                from->clip[i] = to->clip[i];
            }
        }
        if (from->normalize != to->normalize) {
            if (to->normalize == GL_TRUE)
                diff_api.Enable(GL_NORMALIZE);
            else
                diff_api.Disable(GL_NORMALIZE);
            from->normalize = to->normalize;
        }
#ifdef CR_OPENGL_VERSION_1_2
        if (from->rescaleNormals != to->rescaleNormals) {
            able[to->rescaleNormals](GL_RESCALE_NORMAL);
            from->rescaleNormals = to->rescaleNormals;
        }
#else
        (void) able;
#endif
#ifdef CR_IBM_rasterpos_clip
        if (from->rasterPositionUnclipped != to->rasterPositionUnclipped) {
            able[to->rasterPositionUnclipped](GL_RASTER_POSITION_UNCLIPPED_IBM);
            from->rasterPositionUnclipped = to->rasterPositionUnclipped;
        }
#endif

        CLEARDIRTY(t->enable, nbitID);
    }

    if (CHECKDIRTY(t->clipPlane, bitID)) {
        if (from->matrixMode != GL_MODELVIEW) {
            diff_api.MatrixMode(GL_MODELVIEW);
            from->matrixMode = GL_MODELVIEW;
        }
        diff_api.PushMatrix();
        diff_api.LoadIdentity();
        for (i=0; i<CR_MAX_CLIP_PLANES; i++) {
            if (from->clipPlane[i].x != to->clipPlane[i].x ||
                from->clipPlane[i].y != to->clipPlane[i].y ||
                from->clipPlane[i].z != to->clipPlane[i].z ||
                from->clipPlane[i].w != to->clipPlane[i].w) {
                
                GLdouble cp[4];
                cp[0] = to->clipPlane[i].x;
                cp[1] = to->clipPlane[i].y;
                cp[2] = to->clipPlane[i].z;
                cp[3] = to->clipPlane[i].w;

                diff_api.ClipPlane(GL_CLIP_PLANE0 + i, (const GLdouble *)(cp));
                from->clipPlane[i] = to->clipPlane[i];
            }
        }
        diff_api.PopMatrix();
        CLEARDIRTY(t->clipPlane, nbitID);
    }
    
    /* If the matrix stack depths don't match when we're
     * updating the context - we can update the wrong matrix
     * and get some lovely effects!! 
     * So we troll each depth list here and Pop & Push the matrix stack
     * to bring it right up to date before checking the current
     * matrix.
     *
     * - Alan H.
     */
    if ( (from->modelViewStack.depth != to->modelViewStack.depth) ||
          CHECKDIRTY(t->modelviewMatrix, bitID) )
    {
        if (from->matrixMode != GL_MODELVIEW) {
            diff_api.MatrixMode(GL_MODELVIEW);
            from->matrixMode = GL_MODELVIEW;
        }

        if (from->modelViewStack.depth > to->modelViewStack.depth) 
        {
            for (i = to->modelViewStack.depth; i < from->modelViewStack.depth; i++) 
            {
                diff_api.PopMatrix();
            }

            from->modelViewStack.depth = to->modelViewStack.depth;
        }

        for (i = from->modelViewStack.depth; i <= to->modelViewStack.depth; i++)
        {
            LOADMATRIX(to->modelViewStack.stack + i);
            from->modelViewStack.stack[i] = to->modelViewStack.stack[i];

            /* Don't want to push on the current matrix */
            if (i != to->modelViewStack.depth)
                diff_api.PushMatrix();
        }
        from->modelViewStack.depth = to->modelViewStack.depth;
        from->modelViewStack.top = from->modelViewStack.stack + from->modelViewStack.depth;

        CLEARDIRTY(t->modelviewMatrix, nbitID);
    }

    /* Projection matrix */
    if ( (from->projectionStack.depth != to->projectionStack.depth) ||
          CHECKDIRTY(t->projectionMatrix, bitID) )
    {
        if (from->matrixMode != GL_PROJECTION) {
            diff_api.MatrixMode(GL_PROJECTION);
            from->matrixMode = GL_PROJECTION;
        }

        if (from->projectionStack.depth > to->projectionStack.depth) 
        {
            for (i = to->projectionStack.depth; i < from->projectionStack.depth; i++) 
            {
                diff_api.PopMatrix();
            }

            from->projectionStack.depth = to->projectionStack.depth;
        }

        for (i = from->projectionStack.depth; i <= to->projectionStack.depth; i++)
        {
            LOADMATRIX(to->projectionStack.stack + i);
            from->projectionStack.stack[i] = to->projectionStack.stack[i];

            /* Don't want to push on the current matrix */
            if (i != to->projectionStack.depth)
                diff_api.PushMatrix();
        }
        from->projectionStack.depth = to->projectionStack.depth;
        from->projectionStack.top = from->projectionStack.stack + from->projectionStack.depth;

        CLEARDIRTY(t->projectionMatrix, nbitID);
    }

    /* Texture matrices */
    for (i = 0 ; i < maxTextureUnits ; i++)
        if (from->textureStack[i].depth != to->textureStack[i].depth)
            checktex = 1;
    
    if (checktex || CHECKDIRTY(t->textureMatrix, bitID))
    {
        if (from->matrixMode != GL_TEXTURE) {
            diff_api.MatrixMode(GL_TEXTURE);
            from->matrixMode = GL_TEXTURE;
        }
        for (j = 0 ; j < maxTextureUnits; j++)
        {
            if (from->textureStack[j].depth > to->textureStack[j].depth) 
            {
                if (textureFrom->curTextureUnit != j) {
                    diff_api.ActiveTextureARB( j + GL_TEXTURE0_ARB );
                    textureFrom->curTextureUnit = j;
                }
                for (i = to->textureStack[j].depth; i < from->textureStack[j].depth; i++) 
                {
                    diff_api.PopMatrix();
                }
    
                from->textureStack[j].depth = to->textureStack[j].depth;
            }

            for (i = from->textureStack[j].depth; i <= to->textureStack[j].depth; i++)
            {
                if (textureFrom->curTextureUnit != j) {
                    diff_api.ActiveTextureARB( j + GL_TEXTURE0_ARB );
                    textureFrom->curTextureUnit = j;
                }
                LOADMATRIX(to->textureStack[j].stack + i);
                from->textureStack[j].stack[i] = to->textureStack[j].stack[i];

                /* Don't want to push on the current matrix */
                if (i != to->textureStack[j].depth)
                    diff_api.PushMatrix();
            }
            from->textureStack[j].depth = to->textureStack[j].depth;
            from->textureStack[j].top = from->textureStack[j].stack + from->textureStack[j].depth;
        }
        CLEARDIRTY(t->textureMatrix, nbitID);

        /* Restore proper active texture unit */
        diff_api.ActiveTextureARB(GL_TEXTURE0_ARB + toCtx->texture.curTextureUnit);
    }

    /* Color matrix */
    if ( (from->colorStack.depth != to->colorStack.depth) ||
          CHECKDIRTY(t->colorMatrix, bitID) )
    {
        if (from->matrixMode != GL_COLOR) {
            diff_api.MatrixMode(GL_COLOR);
            from->matrixMode = GL_COLOR;
        }

        if (from->colorStack.depth > to->colorStack.depth) 
        {
            for (i = to->colorStack.depth; i < from->colorStack.depth; i++) 
            {
                diff_api.PopMatrix();
            }

            from->colorStack.depth = to->colorStack.depth;
        }

        for (i = to->colorStack.depth; i <= to->colorStack.depth; i++)
        {
            LOADMATRIX(to->colorStack.stack + i);
            from->colorStack.stack[i] = to->colorStack.stack[i];

            /* Don't want to push on the current matrix */
            if (i != to->colorStack.depth)
                diff_api.PushMatrix();
        }
        from->colorStack.depth = to->colorStack.depth;
        from->colorStack.top = from->colorStack.stack + from->colorStack.depth;

        CLEARDIRTY(t->colorMatrix, nbitID);
    }

    to->modelViewProjectionValid = 0;
    CLEARDIRTY(t->dirty, nbitID);

    /* update MatrixMode now */
    if (from->matrixMode != to->matrixMode) {
        diff_api.MatrixMode(to->matrixMode);
        from->matrixMode = to->matrixMode;
    }

    /* sanity tests */
    CRASSERT(from->modelViewStack.top == from->modelViewStack.stack + from->modelViewStack.depth);
    CRASSERT(from->projectionStack.top == from->projectionStack.stack + from->projectionStack.depth);
}
