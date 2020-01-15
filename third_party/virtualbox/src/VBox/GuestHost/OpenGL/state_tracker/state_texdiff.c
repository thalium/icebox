/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state/cr_statetypes.h"
#include "state/cr_texture.h"
#include "cr_hash.h"
#include "cr_string.h"
#include "cr_mem.h"
#include "cr_version.h"
#include "state_internals.h"


#define UNIMPLEMENTED() crStateError(pState, __LINE__,__FILE__,GL_INVALID_OPERATION, "Unimplemented something or other" )


#if 0 /* NOT USED??? */
void crStateTextureObjSwitchCallback( unsigned long key, void *data1, void *data2 )
{
    CRTextureObj *tobj = (CRTextureObj *) data1;
    CRContext *fromCtx = (CRContext *) data2;
    unsigned int i = 0;
    unsigned int j = 0;
    CRbitvalue *bitID = fromCtx->bitid;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (!tobj) return;

    FILLDIRTY(tobj->dirty);
    FILLDIRTY(tobj->imageBit);

    for (i = 0; i < fromCtx->limits.maxTextureUnits; i++)
    {
        int j;

        FILLDIRTY(tobj->paramsBit[i]);
        
        switch (tobj->target)
        {
            case GL_TEXTURE_1D:
            case GL_TEXTURE_2D:
                for (j = 0; j <= fromCtx->texture.maxLevel; j++) 
                {
                    CRTextureLevel *tl = &(tobj->level[j]);
                    FILLDIRTY(tl->dirty);
                }
                break;
#ifdef CR_OPENGL_VERSION_1_2
            case GL_TEXTURE_3D:
#endif
                for (j = 0; j <= fromCtx->texture.max3DLevel; j++) 
                {
                    CRTextureLevel *tl = &(tobj->level[j]);
                    FILLDIRTY(tl->dirty);
                }
                break;
#ifdef CR_ARB_texture_cube_map
            case GL_TEXTURE_CUBE_MAP_ARB:
                for (j = 0; j <= fromCtx->texture.maxCubeMapLevel; j++) 
                {
                    CRTextureLevel *tl;
                    /* Positive X */
                    tl = &(tobj->level[j]);
                    FILLDIRTY(tl->dirty);
                    /* Negative X */
                    tl = &(tobj->negativeXlevel[j]);
                    FILLDIRTY(tl->dirty);
                    /* Positive Y */
                    tl = &(tobj->positiveYlevel[j]);
                    FILLDIRTY(tl->dirty);
                    /* Negative Y */
                    tl = &(tobj->negativeYlevel[j]);
                    FILLDIRTY(tl->dirty);
                    /* Positive Z */
                    tl = &(tobj->positiveZlevel[j]);
                    FILLDIRTY(tl->dirty);
                    /* Negative Z */
                    tl = &(tobj->negativeZlevel[j]);
                    FILLDIRTY(tl->dirty);
                }
                break;
#endif
            default:
                UNIMPLEMENTED();
        }   
    }
}
#endif


void crStateTextureSwitch( CRTextureBits *tb, CRbitvalue *bitID, 
                                                     CRContext *fromCtx, CRContext *toCtx )
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRTextureState *from = &(fromCtx->texture);
    const CRTextureState *to = &(toCtx->texture);
    unsigned int i,j;
    glAble able[2];
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    unsigned int activeUnit = (unsigned int) -1;

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    able[0] = pState->diff_api.Disable;
    able[1] = pState->diff_api.Enable;

    for (i = 0; i < fromCtx->limits.maxTextureUnits; i++)
    {
        if (CHECKDIRTY(tb->enable[i], bitID)) 
        {
            if (activeUnit != i) {
                pState->diff_api.ActiveTextureARB( i + GL_TEXTURE0_ARB );
                activeUnit = i;
            }
            if (from->unit[i].enabled1D != to->unit[i].enabled1D) 
            {
                able[to->unit[i].enabled1D](GL_TEXTURE_1D);
                FILLDIRTY(tb->enable[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].enabled2D != to->unit[i].enabled2D) 
            {
                able[to->unit[i].enabled2D](GL_TEXTURE_2D);
                FILLDIRTY(tb->enable[i]);
                FILLDIRTY(tb->dirty);
            }
#ifdef CR_OPENGL_VERSION_1_2
            if (from->unit[i].enabled3D != to->unit[i].enabled3D)
            {
                able[to->unit[i].enabled3D](GL_TEXTURE_3D);
                FILLDIRTY(tb->enable[i]);
                FILLDIRTY(tb->dirty);
            }
#endif
#ifdef CR_ARB_texture_cube_map
            if (fromCtx->extensions.ARB_texture_cube_map &&
                from->unit[i].enabledCubeMap != to->unit[i].enabledCubeMap)
            {
                able[to->unit[i].enabledCubeMap](GL_TEXTURE_CUBE_MAP_ARB);
                FILLDIRTY(tb->enable[i]);
                FILLDIRTY(tb->dirty);
            }
#endif
#ifdef CR_NV_texture_rectangle
            if (fromCtx->extensions.NV_texture_rectangle &&
                from->unit[i].enabledRect != to->unit[i].enabledRect)
            {
                able[to->unit[i].enabledRect](GL_TEXTURE_RECTANGLE_NV);
                FILLDIRTY(tb->enable[i]);
                FILLDIRTY(tb->dirty);
            }
#endif
            if (from->unit[i].textureGen.s != to->unit[i].textureGen.s ||
                    from->unit[i].textureGen.t != to->unit[i].textureGen.t ||
                    from->unit[i].textureGen.r != to->unit[i].textureGen.r ||
                    from->unit[i].textureGen.q != to->unit[i].textureGen.q) 
            {
                able[to->unit[i].textureGen.s](GL_TEXTURE_GEN_S);
                able[to->unit[i].textureGen.t](GL_TEXTURE_GEN_T);
                able[to->unit[i].textureGen.r](GL_TEXTURE_GEN_R);
                able[to->unit[i].textureGen.q](GL_TEXTURE_GEN_Q);
                FILLDIRTY(tb->enable[i]);
                FILLDIRTY(tb->dirty);
            }
            CLEARDIRTY(tb->enable[i], nbitID);
        }

        /* 
        **  A thought on switching with textures:
        **  Since we are only performing a switch
        **  and not a sync, we won't need to 
        **  update individual textures, just
        **  the bindings....
        */

        if (CHECKDIRTY(tb->current[i], bitID)) 
        {
            if (activeUnit != i) {
                pState->diff_api.ActiveTextureARB( i + GL_TEXTURE0_ARB );
                activeUnit = i;
            }
            if (from->unit[i].currentTexture1D->hwid != to->unit[i].currentTexture1D->hwid)
            {
                pState->diff_api.BindTexture(GL_TEXTURE_1D, crStateGetTextureObjHWID(pState, to->unit[i].currentTexture1D));
                FILLDIRTY(tb->current[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].currentTexture2D->hwid != to->unit[i].currentTexture2D->hwid)
            {
                pState->diff_api.BindTexture(GL_TEXTURE_2D, crStateGetTextureObjHWID(pState, to->unit[i].currentTexture2D));
                FILLDIRTY(tb->current[i]);
                FILLDIRTY(tb->dirty);
            }
#ifdef CR_OPENGL_VERSION_1_2
            if (from->unit[i].currentTexture3D->hwid != to->unit[i].currentTexture3D->hwid)
            {
                pState->diff_api.BindTexture(GL_TEXTURE_3D, crStateGetTextureObjHWID(pState, to->unit[i].currentTexture3D));
                FILLDIRTY(tb->current[i]);
                FILLDIRTY(tb->dirty);
            }
#endif
#ifdef CR_ARB_texture_cube_map
            if (fromCtx->extensions.ARB_texture_cube_map &&
                from->unit[i].currentTextureCubeMap->hwid != to->unit[i].currentTextureCubeMap->hwid)
            {
                pState->diff_api.BindTexture(GL_TEXTURE_CUBE_MAP_ARB, crStateGetTextureObjHWID(pState, to->unit[i].currentTextureCubeMap));
                FILLDIRTY(tb->current[i]);
                FILLDIRTY(tb->dirty);
            }
#endif
#ifdef CR_NV_texture_rectangle
            if (fromCtx->extensions.NV_texture_rectangle &&
                from->unit[i].currentTextureRect->hwid != to->unit[i].currentTextureRect->hwid)
            {
                pState->diff_api.BindTexture(GL_TEXTURE_RECTANGLE_NV, crStateGetTextureObjHWID(pState, to->unit[i].currentTextureRect));
                FILLDIRTY(tb->current[i]);
                FILLDIRTY(tb->dirty);
            }
#endif
            CLEARDIRTY(tb->current[i], nbitID);
        }

        if (CHECKDIRTY(tb->objGen[i], bitID)) 
        {
            if (activeUnit != i) {
                pState->diff_api.ActiveTextureARB( i + GL_TEXTURE0_ARB );
                activeUnit = i;
            }
            if (from->unit[i].objSCoeff.x != to->unit[i].objSCoeff.x ||
                  from->unit[i].objSCoeff.y != to->unit[i].objSCoeff.y ||
                  from->unit[i].objSCoeff.z != to->unit[i].objSCoeff.z ||
                  from->unit[i].objSCoeff.w != to->unit[i].objSCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[i].objSCoeff.x;
                f[1] = to->unit[i].objSCoeff.y;
                f[2] = to->unit[i].objSCoeff.z;
                f[3] = to->unit[i].objSCoeff.w;
                pState->diff_api.TexGenfv (GL_S, GL_OBJECT_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->objGen[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].objTCoeff.x != to->unit[i].objTCoeff.x ||
                from->unit[i].objTCoeff.y != to->unit[i].objTCoeff.y ||
                from->unit[i].objTCoeff.z != to->unit[i].objTCoeff.z ||
                from->unit[i].objTCoeff.w != to->unit[i].objTCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].objTCoeff.x;
                f[1] = to->unit[i].objTCoeff.y;
                f[2] = to->unit[i].objTCoeff.z;
                f[3] = to->unit[i].objTCoeff.w;
                pState->diff_api.TexGenfv (GL_T, GL_OBJECT_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->objGen[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].objRCoeff.x != to->unit[i].objRCoeff.x ||
                from->unit[i].objRCoeff.y != to->unit[i].objRCoeff.y ||
                from->unit[i].objRCoeff.z != to->unit[i].objRCoeff.z ||
                from->unit[i].objRCoeff.w != to->unit[i].objRCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].objRCoeff.x;
                f[1] = to->unit[i].objRCoeff.y;
                f[2] = to->unit[i].objRCoeff.z;
                f[3] = to->unit[i].objRCoeff.w;
                pState->diff_api.TexGenfv (GL_R, GL_OBJECT_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->objGen[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].objQCoeff.x != to->unit[i].objQCoeff.x ||
                from->unit[i].objQCoeff.y != to->unit[i].objQCoeff.y ||
                from->unit[i].objQCoeff.z != to->unit[i].objQCoeff.z ||
                from->unit[i].objQCoeff.w != to->unit[i].objQCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].objQCoeff.x;
                f[1] = to->unit[i].objQCoeff.y;
                f[2] = to->unit[i].objQCoeff.z;
                f[3] = to->unit[i].objQCoeff.w;
                pState->diff_api.TexGenfv (GL_Q, GL_OBJECT_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->objGen[i]);
                FILLDIRTY(tb->dirty);
            }
            CLEARDIRTY(tb->objGen[i], nbitID);
        }
        if (CHECKDIRTY(tb->eyeGen[i], bitID)) 
        {
            if (activeUnit != i) {
                pState->diff_api.ActiveTextureARB( i + GL_TEXTURE0_ARB );
                activeUnit = i;
            }
            pState->diff_api.MatrixMode(GL_MODELVIEW);
            pState->diff_api.PushMatrix();
            pState->diff_api.LoadIdentity();
            if (from->unit[i].eyeSCoeff.x != to->unit[i].eyeSCoeff.x ||
                from->unit[i].eyeSCoeff.y != to->unit[i].eyeSCoeff.y ||
                from->unit[i].eyeSCoeff.z != to->unit[i].eyeSCoeff.z ||
                from->unit[i].eyeSCoeff.w != to->unit[i].eyeSCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].eyeSCoeff.x;
                f[1] = to->unit[i].eyeSCoeff.y;
                f[2] = to->unit[i].eyeSCoeff.z;
                f[3] = to->unit[i].eyeSCoeff.w;
                pState->diff_api.TexGenfv (GL_S, GL_EYE_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->eyeGen[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].eyeTCoeff.x != to->unit[i].eyeTCoeff.x ||
                from->unit[i].eyeTCoeff.y != to->unit[i].eyeTCoeff.y ||
                from->unit[i].eyeTCoeff.z != to->unit[i].eyeTCoeff.z ||
                from->unit[i].eyeTCoeff.w != to->unit[i].eyeTCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].eyeTCoeff.x;
                f[1] = to->unit[i].eyeTCoeff.y;
                f[2] = to->unit[i].eyeTCoeff.z;
                f[3] = to->unit[i].eyeTCoeff.w;
                pState->diff_api.TexGenfv (GL_T, GL_EYE_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->eyeGen[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].eyeRCoeff.x != to->unit[i].eyeRCoeff.x ||
                from->unit[i].eyeRCoeff.y != to->unit[i].eyeRCoeff.y ||
                from->unit[i].eyeRCoeff.z != to->unit[i].eyeRCoeff.z ||
                from->unit[i].eyeRCoeff.w != to->unit[i].eyeRCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].eyeRCoeff.x;
                f[1] = to->unit[i].eyeRCoeff.y;
                f[2] = to->unit[i].eyeRCoeff.z;
                f[3] = to->unit[i].eyeRCoeff.w;
                pState->diff_api.TexGenfv (GL_R, GL_EYE_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->eyeGen[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].eyeQCoeff.x != to->unit[i].eyeQCoeff.x ||
                from->unit[i].eyeQCoeff.y != to->unit[i].eyeQCoeff.y ||
                from->unit[i].eyeQCoeff.z != to->unit[i].eyeQCoeff.z ||
                from->unit[i].eyeQCoeff.w != to->unit[i].eyeQCoeff.w) {
                GLfloat f[4];
                f[0] = to->unit[i].eyeQCoeff.x;
                f[1] = to->unit[i].eyeQCoeff.y;
                f[2] = to->unit[i].eyeQCoeff.z;
                f[3] = to->unit[i].eyeQCoeff.w;
                pState->diff_api.TexGenfv (GL_Q, GL_EYE_PLANE, (const GLfloat *) f);
                FILLDIRTY(tb->eyeGen[i]);
                FILLDIRTY(tb->dirty);
            }
            pState->diff_api.PopMatrix();
            CLEARDIRTY(tb->eyeGen[i], nbitID);
        }
        if (CHECKDIRTY(tb->genMode[i], bitID)) 
        {
            if (activeUnit != i) {
                pState->diff_api.ActiveTextureARB( i + GL_TEXTURE0_ARB );
                activeUnit = i;
            }
            if (from->unit[i].gen.s != to->unit[i].gen.s ||
                from->unit[i].gen.t != to->unit[i].gen.t ||
                from->unit[i].gen.r != to->unit[i].gen.r ||
                from->unit[i].gen.q != to->unit[i].gen.q) 
            {
                pState->diff_api.TexGeni (GL_S, GL_TEXTURE_GEN_MODE, to->unit[i].gen.s);
                pState->diff_api.TexGeni (GL_T, GL_TEXTURE_GEN_MODE, to->unit[i].gen.t);
                pState->diff_api.TexGeni (GL_R, GL_TEXTURE_GEN_MODE, to->unit[i].gen.r);
                pState->diff_api.TexGeni (GL_Q, GL_TEXTURE_GEN_MODE, to->unit[i].gen.q);    
                FILLDIRTY(tb->genMode[i]);
                FILLDIRTY(tb->dirty);
            }
            CLEARDIRTY(tb->genMode[i], nbitID);
        }
        CLEARDIRTY(tb->dirty, nbitID);

        /* Texture enviroment */
        if (CHECKDIRTY(tb->envBit[i], bitID)) 
        {
            if (activeUnit != i) {
                pState->diff_api.ActiveTextureARB( i + GL_TEXTURE0_ARB );
                activeUnit = i;
            }
            if (from->unit[i].envMode != to->unit[i].envMode) 
            {
                pState->diff_api.TexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, to->unit[i].envMode);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].envColor.r != to->unit[i].envColor.r ||
                from->unit[i].envColor.g != to->unit[i].envColor.g ||
                from->unit[i].envColor.b != to->unit[i].envColor.b ||
                from->unit[i].envColor.a != to->unit[i].envColor.a) 
            {
                GLfloat f[4];
                f[0] = to->unit[i].envColor.r;
                f[1] = to->unit[i].envColor.g;
                f[2] = to->unit[i].envColor.b;
                f[3] = to->unit[i].envColor.a;
                pState->diff_api.TexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (const GLfloat *) f);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineModeRGB != to->unit[i].combineModeRGB)
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, to->unit[i].combineModeRGB);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineModeA != to->unit[i].combineModeA)
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, to->unit[i].combineModeA);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineSourceRGB[0] != to->unit[i].combineSourceRGB[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, to->unit[i].combineSourceRGB[0]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineSourceRGB[1] != to->unit[i].combineSourceRGB[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, to->unit[i].combineSourceRGB[1]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineSourceRGB[2] != to->unit[i].combineSourceRGB[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, to->unit[i].combineSourceRGB[2]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineSourceA[0] != to->unit[i].combineSourceA[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, to->unit[i].combineSourceA[0]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineSourceA[1] != to->unit[i].combineSourceA[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, to->unit[i].combineSourceA[1]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineSourceA[2] != to->unit[i].combineSourceA[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, to->unit[i].combineSourceA[2]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineOperandRGB[0] != to->unit[i].combineOperandRGB[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, to->unit[i].combineOperandRGB[0]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineOperandRGB[1] != to->unit[i].combineOperandRGB[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, to->unit[i].combineOperandRGB[1]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineOperandRGB[2] != to->unit[i].combineOperandRGB[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, to->unit[i].combineOperandRGB[2]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineOperandA[0] != to->unit[i].combineOperandA[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, to->unit[i].combineOperandA[0]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineOperandA[1] != to->unit[i].combineOperandA[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, to->unit[i].combineOperandA[1]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineOperandA[2] != to->unit[i].combineOperandA[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, to->unit[i].combineOperandA[2]);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineScaleRGB != to->unit[i].combineScaleRGB)
            {
                pState->diff_api.TexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, to->unit[i].combineScaleRGB);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            if (from->unit[i].combineScaleA != to->unit[i].combineScaleA)
            {
                pState->diff_api.TexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, to->unit[i].combineScaleA);
                FILLDIRTY(tb->envBit[i]);
                FILLDIRTY(tb->dirty);
            }
            CLEARDIRTY(tb->envBit[i], nbitID);
        }
    }

    /* Resend texture images */
    if (toCtx->shared->bTexResyncNeeded)
    {
        toCtx->shared->bTexResyncNeeded = GL_FALSE;

        crStateDiffAllTextureObjects(toCtx, bitID, GL_TRUE);
    }

    /* After possible fiddling put them back now */
    if (activeUnit != toCtx->texture.curTextureUnit) {
        pState->diff_api.ActiveTextureARB( toCtx->texture.curTextureUnit + GL_TEXTURE0_ARB );
    }
    pState->diff_api.MatrixMode(toCtx->transform.matrixMode);
}


/* 
 * Check the dirty bits for the specified texture on a given unit to
 * determine if any of its images are dirty.
 * Return:  1 -- dirty, 0 -- clean
 */
int crStateTextureCheckDirtyImages(CRContext *from, CRContext *to, GLenum target, int textureUnit)
{
    CRContext *g         = GetCurrentContext(from->pStateTracker);
    CRTextureState *tsto;
    CRbitvalue *bitID;
    CRTextureObj *tobj   = NULL;
    int maxLevel = 0, i;
    int face, numFaces;

    CRASSERT(to);
    CRASSERT(from);
    CRASSERT(from->pStateTracker == to->pStateTracker);

    tsto = &(to->texture);
    bitID = from->bitid;

    CRASSERT(tsto);

    switch(target)
    {
        case GL_TEXTURE_1D:
            tobj = tsto->unit[textureUnit].currentTexture1D;
            maxLevel = tsto->maxLevel;
            break;
        case GL_TEXTURE_2D:
            tobj = tsto->unit[textureUnit].currentTexture2D;
            maxLevel = tsto->maxLevel;
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_3D:
            tobj = tsto->unit[textureUnit].currentTexture3D;
            maxLevel = tsto->max3DLevel;
            break;
#endif
#ifdef CR_ARB_texture_cube_map
        case GL_TEXTURE_CUBE_MAP:
            if (g->extensions.ARB_texture_cube_map) {
                tobj = tsto->unit[textureUnit].currentTextureCubeMap;
                maxLevel = tsto->maxCubeMapLevel;
            }
            break;
#endif
#ifdef CR_NV_texture_rectangle
        case GL_TEXTURE_RECTANGLE_NV:
            if (g->extensions.NV_texture_rectangle) {
                tobj = tsto->unit[textureUnit].currentTextureRect;
                maxLevel = 1;
            }
            break;
#endif
        default:
            crError("Bad texture target in crStateTextureCheckDirtyImages()");
            return 0;
    }

    if (!tobj)
    {
        return 0;
    }

    numFaces = (target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;

    for (face = 0; face < numFaces; face++) {
        for (i = 0; i < maxLevel; i++) {
            if (CHECKDIRTY(tobj->level[face][i].dirty, bitID))
                return 1;
        }
    }

    return 0;
}


/*
 * Do texture state differencing for the given texture object.
 */
void
crStateTextureObjectDiff(CRContext *fromCtx,
                         const CRbitvalue *bitID, const CRbitvalue *nbitID,
                         CRTextureObj *tobj, GLboolean alwaysDirty)
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRTextureState *from = &(fromCtx->texture);
    glAble able[2];
    int u = 0; /* always use texture unit 0 for diff'ing */
    GLuint hwid = crStateGetTextureObjHWID(pState, tobj);

    if (!hwid)
        return;

    able[0] = pState->diff_api.Disable;
    able[1] = pState->diff_api.Enable;

#if 0
    /* XXX disabling this code fixed Wes Bethel's problem with missing/white
     * textures.  Setting the active texture unit here can screw up the
     * current texture object bindings.
     */
    /* Set active texture unit, and bind this texture object */
    if (from->curTextureUnit != u) {
        pState->diff_api.ActiveTextureARB( u + GL_TEXTURE0_ARB );
        from->curTextureUnit = u;
    }
#endif

    pState->diff_api.BindTexture(tobj->target, hwid);

    if (alwaysDirty || CHECKDIRTY(tobj->paramsBit[u], bitID)) 
    {
        GLfloat f[4];
        f[0] = tobj->borderColor.r;
        f[1] = tobj->borderColor.g;
        f[2] = tobj->borderColor.b;
        f[3] = tobj->borderColor.a;
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_BASE_LEVEL, tobj->baseLevel);
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_MAX_LEVEL, tobj->maxLevel);
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_MIN_FILTER, tobj->minFilter);
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_MAG_FILTER, tobj->magFilter);
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_WRAP_S, tobj->wrapS);
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_WRAP_T, tobj->wrapT);
#ifdef CR_OPENGL_VERSION_1_2
        pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_WRAP_R, tobj->wrapR);
        pState->diff_api.TexParameterf(tobj->target, GL_TEXTURE_PRIORITY, tobj->priority);
#endif
        pState->diff_api.TexParameterfv(tobj->target, GL_TEXTURE_BORDER_COLOR, (const GLfloat *) f);
#ifdef CR_EXT_texture_filter_anisotropic
        if (fromCtx->extensions.EXT_texture_filter_anisotropic) {
            pState->diff_api.TexParameterf(tobj->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, tobj->maxAnisotropy);
        }
#endif
#ifdef CR_ARB_depth_texture
        if (fromCtx->extensions.ARB_depth_texture)
            pState->diff_api.TexParameteri(tobj->target, GL_DEPTH_TEXTURE_MODE_ARB, tobj->depthMode);
#endif
#ifdef CR_ARB_shadow
        if (fromCtx->extensions.ARB_shadow) {
            pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_COMPARE_MODE_ARB, tobj->compareMode);
            pState->diff_api.TexParameteri(tobj->target, GL_TEXTURE_COMPARE_FUNC_ARB, tobj->compareFunc);
        }
#endif
#ifdef CR_ARB_shadow_ambient
        if (fromCtx->extensions.ARB_shadow_ambient) {
            pState->diff_api.TexParameterf(tobj->target, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, tobj->compareFailValue);
        }
#endif
#ifdef CR_SGIS_generate_mipmap
        if (fromCtx->extensions.SGIS_generate_mipmap) {
            pState->diff_api.TexParameteri(tobj->target, GL_GENERATE_MIPMAP_SGIS, tobj->generateMipmap);
        }
#endif
        if (!alwaysDirty)
            CLEARDIRTY(tobj->paramsBit[u], nbitID);
    }

    /* now, if the texture images are dirty */
    if (alwaysDirty || CHECKDIRTY(tobj->imageBit, bitID)) 
    {
        int lvl;
        int face;

        switch (tobj->target)
        {
            case GL_TEXTURE_1D:
                for (lvl = 0; lvl <= from->maxLevel; lvl++) 
                {
                    CRTextureLevel *tl = &(tobj->level[0][lvl]);
                    if (alwaysDirty || CHECKDIRTY(tl->dirty, bitID)) 
                    {
                        if (tl->generateMipmap) {
                            pState->diff_api.TexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, 1);
                        }
                        if (tl->width)
                        {
                            if (tl->compressed) {
                                pState->diff_api.CompressedTexImage1DARB(GL_TEXTURE_1D, lvl,
                                                                 tl->internalFormat, tl->width,
                                                                 tl->border, tl->bytes, tl->img);
                            }
                            else {
                                /* alignment must be one */
                                pState->diff_api.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                                if (tl->generateMipmap) {
                                    pState->diff_api.TexParameteri(GL_TEXTURE_1D, GL_GENERATE_MIPMAP_SGIS, 1);
                                }
                                pState->diff_api.TexImage1D(GL_TEXTURE_1D, lvl,
                                                    tl->internalFormat,
                                                    tl->width, tl->border,
                                                    tl->format, tl->type, tl->img);
                            }
                        }
                        if (!alwaysDirty)
                        {
                            CLEARDIRTY(tl->dirty, nbitID);
                        }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
                        else
                        {
                            crFree(tl->img);
                            tl->img = NULL;
                        }
#endif
                    }
                }
                break;
            case GL_TEXTURE_2D:
                for (lvl = 0; lvl <= from->maxLevel; lvl++) 
                {
                    CRTextureLevel *tl = &(tobj->level[0][lvl]);
                    if (alwaysDirty || CHECKDIRTY(tl->dirty, bitID)) 
                    {
                        if (tl->generateMipmap) {
                            pState->diff_api.TexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, 1);
                        }

                        if (tl->width && tl->height)
                        {
                            if (tl->compressed) {
                                pState->diff_api.CompressedTexImage2DARB(GL_TEXTURE_2D, lvl,
                                         tl->internalFormat, tl->width,
                                         tl->height, tl->border,
                                         tl->bytes, tl->img);
                            }
                            else {
                                /* alignment must be one */
                                pState->diff_api.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                                pState->diff_api.TexImage2D(GL_TEXTURE_2D, lvl,
                                                    tl->internalFormat,
                                                    tl->width, tl->height, tl->border,
                                                    tl->format, tl->type, tl->img);
                            }
                        }

                        if (!alwaysDirty)
                        {
                            CLEARDIRTY(tl->dirty, nbitID);
                        }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
                        else
                        {
                            crFree(tl->img);
                            tl->img = NULL;
                        }
#endif
                    }
                }
                break;
#ifdef CR_OPENGL_VERSION_1_2
            case GL_TEXTURE_3D:
                for (lvl = 0; lvl <= from->max3DLevel; lvl++) 
                {
                    CRTextureLevel *tl = &(tobj->level[0][lvl]);
                    if (alwaysDirty || CHECKDIRTY(tl->dirty, bitID)) 
                    {
                        if (tl->generateMipmap) {
                            pState->diff_api.TexParameteri(GL_TEXTURE_3D, GL_GENERATE_MIPMAP_SGIS, 1);
                        }

                        if (tl->width && tl->height)
                        {
                            if (tl->compressed) {
                                pState->diff_api.CompressedTexImage3DARB(GL_TEXTURE_3D, lvl,
                                                                 tl->internalFormat, tl->width,
                                                                 tl->height, tl->depth,
                                                                 tl->border, tl->bytes, tl->img);
                            }
                            else {
                                /* alignment must be one */
                                pState->diff_api.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                                pState->diff_api.TexImage3D(GL_TEXTURE_3D, lvl,
                                                    tl->internalFormat,
                                                    tl->width, tl->height, tl->depth,
                                                    tl->border, tl->format,
                                                    tl->type, tl->img);
                            }
                        }

                        if (!alwaysDirty)
                        {
                            CLEARDIRTY(tl->dirty, nbitID);
                        }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
                        else
                        {
                            crFree(tl->img);
                            tl->img = NULL;
                        }
#endif
                    }
                }
                break;
#endif
#ifdef CR_NV_texture_rectangle 
            case GL_TEXTURE_RECTANGLE_NV:
                /* only one level */
                for (lvl = 0; lvl <= from->maxRectLevel; lvl++) 
                {
                    CRTextureLevel *tl = &(tobj->level[0][lvl]);
                    if (alwaysDirty || CHECKDIRTY(tl->dirty, bitID))
                    {
                        if (tl->width && tl->height)
                        {
                            if (tl->compressed) {
                                pState->diff_api.CompressedTexImage2DARB(GL_TEXTURE_RECTANGLE_NV, lvl,
                                                                 tl->internalFormat, tl->width,
                                                                 tl->height, tl->border,
                                                                 tl->bytes, tl->img);
                            }
                            else {
                                /* alignment must be one */
                                pState->diff_api.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                                pState->diff_api.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                                pState->diff_api.TexImage2D(GL_TEXTURE_RECTANGLE_NV, lvl,
                                                    tl->internalFormat,
                                                    tl->width, tl->height, tl->border,
                                                    tl->format, tl->type, tl->img);
                            }
                        }

                        if (!alwaysDirty)
                        {
                            CLEARDIRTY(tl->dirty, nbitID);
                        }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
                        else
                        {
                            crFree(tl->img);
                            tl->img = NULL;
                        }
#endif
                    }
                }
                break;
#endif
#ifdef CR_ARB_texture_cube_map
            case GL_TEXTURE_CUBE_MAP_ARB:
                for (face = 0; face < 6; face++)
                {
                    const GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
                    for (lvl = 0; lvl <= from->maxCubeMapLevel; lvl++) 
                    {
                        CRTextureLevel *tl = &(tobj->level[face][lvl]);
                        if (alwaysDirty || CHECKDIRTY(tl->dirty, bitID))
                        {
                            if (tl->generateMipmap) {
                                pState->diff_api.TexParameteri(GL_TEXTURE_CUBE_MAP_ARB,
                                                       GL_GENERATE_MIPMAP_SGIS, 1);
                            }

                            if (tl->width && tl->height)
                            {
                                if (tl->compressed) {
                                    pState->diff_api.CompressedTexImage2DARB(target,
                                                                     lvl, tl->internalFormat,
                                                                     tl->width, tl->height,
                                                                     tl->border, tl->bytes, tl->img);
                                }
                                else {
                                    /* alignment must be one */
                                    pState->diff_api.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                    pState->diff_api.PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                                    pState->diff_api.PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                                    pState->diff_api.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                                    pState->diff_api.TexImage2D(target, lvl,
                                                        tl->internalFormat,
                                                        tl->width, tl->height, tl->border,
                                                        tl->format, tl->type, tl->img);
                                }
                            }

                            if (!alwaysDirty)
                            {
                                CLEARDIRTY(tl->dirty, nbitID);
                            }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
                            else
                            {
                                crFree(tl->img);
                                tl->img = NULL;
                            }
#endif
                        }
                    } /* for lvl */
                }  /* for face */
                break;
#endif
            default:
                UNIMPLEMENTED();

        }   /* switch */
    } /* if (CHECKDIRTY(tobj->imageBit, bitID)) */
}



void
crStateTextureDiff( CRTextureBits *tb, CRbitvalue *bitID,
                                        CRContext *fromCtx, CRContext *toCtx )
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRTextureState *from = &(fromCtx->texture);
    CRTextureState *to = &(toCtx->texture);
    unsigned int u, t, j;
    glAble able[2];
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    const GLboolean haveFragProg = fromCtx->extensions.ARB_fragment_program || fromCtx->extensions.NV_fragment_program;

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    able[0] = pState->diff_api.Disable;
    able[1] = pState->diff_api.Enable;

    /* loop over texture units */
    for (u = 0; u < fromCtx->limits.maxTextureUnits; u++)
    {
        CRTextureObj **fromBinding = NULL;
        CRTextureObj *tobj;

        /* take care of enables/disables */
        if (CHECKDIRTY(tb->enable[u], bitID)) 
        {

            /* Activate texture unit u if needed */
            if (fromCtx->texture.curTextureUnit != u) {
                pState->diff_api.ActiveTextureARB( GL_TEXTURE0_ARB + u);
                fromCtx->texture.curTextureUnit = u;
            }

            if (from->unit[u].enabled1D != to->unit[u].enabled1D) 
            {
                able[to->unit[u].enabled1D](GL_TEXTURE_1D);
                from->unit[u].enabled1D = to->unit[u].enabled1D;
            }
            if (from->unit[u].enabled2D != to->unit[u].enabled2D) 
            {
                able[to->unit[u].enabled2D](GL_TEXTURE_2D);
                from->unit[u].enabled2D = to->unit[u].enabled2D;
            }
#ifdef CR_OPENGL_VERSION_1_2
            if (from->unit[u].enabled3D != to->unit[u].enabled3D)
            {
                able[to->unit[u].enabled3D](GL_TEXTURE_3D);
                from->unit[u].enabled3D = to->unit[u].enabled3D;
            }
#endif
#ifdef CR_ARB_texture_cube_map
            if (fromCtx->extensions.ARB_texture_cube_map &&
                from->unit[u].enabledCubeMap != to->unit[u].enabledCubeMap)
            {
                able[to->unit[u].enabledCubeMap](GL_TEXTURE_CUBE_MAP_ARB);
                from->unit[u].enabledCubeMap = to->unit[u].enabledCubeMap;
            }
#endif
#ifdef CR_NV_texture_rectangle
            if (fromCtx->extensions.NV_texture_rectangle &&
                from->unit[u].enabledRect != to->unit[u].enabledRect)
            {
                able[to->unit[u].enabledRect](GL_TEXTURE_RECTANGLE_NV);
                from->unit[u].enabledRect = to->unit[u].enabledRect;
            }
#endif

            /* texgen enables */
            if (from->unit[u].textureGen.s != to->unit[u].textureGen.s ||
                    from->unit[u].textureGen.t != to->unit[u].textureGen.t ||
                    from->unit[u].textureGen.r != to->unit[u].textureGen.r ||
                    from->unit[u].textureGen.q != to->unit[u].textureGen.q) 
            {
                able[to->unit[u].textureGen.s](GL_TEXTURE_GEN_S);
                able[to->unit[u].textureGen.t](GL_TEXTURE_GEN_T);
                able[to->unit[u].textureGen.r](GL_TEXTURE_GEN_R);
                able[to->unit[u].textureGen.q](GL_TEXTURE_GEN_Q);
                from->unit[u].textureGen = to->unit[u].textureGen;
            }
            CLEARDIRTY(tb->enable[u], nbitID);
        }


        /* Texgen coefficients */
        if (CHECKDIRTY(tb->objGen[u], bitID)) 
        {
            if (fromCtx->texture.curTextureUnit != u) {
                pState->diff_api.ActiveTextureARB( u + GL_TEXTURE0_ARB );
                fromCtx->texture.curTextureUnit = u;
            }
            if (from->unit[u].objSCoeff.x != to->unit[u].objSCoeff.x ||
                    from->unit[u].objSCoeff.y != to->unit[u].objSCoeff.y ||
                    from->unit[u].objSCoeff.z != to->unit[u].objSCoeff.z ||
                    from->unit[u].objSCoeff.w != to->unit[u].objSCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].objSCoeff.x;
                f[1] = to->unit[u].objSCoeff.y;
                f[2] = to->unit[u].objSCoeff.z;
                f[3] = to->unit[u].objSCoeff.w;
                pState->diff_api.TexGenfv (GL_S, GL_OBJECT_PLANE, (const GLfloat *) f);
                from->unit[u].objSCoeff = to->unit[u].objSCoeff;
            }
            if (from->unit[u].objTCoeff.x != to->unit[u].objTCoeff.x ||
                    from->unit[u].objTCoeff.y != to->unit[u].objTCoeff.y ||
                    from->unit[u].objTCoeff.z != to->unit[u].objTCoeff.z ||
                    from->unit[u].objTCoeff.w != to->unit[u].objTCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].objTCoeff.x;
                f[1] = to->unit[u].objTCoeff.y;
                f[2] = to->unit[u].objTCoeff.z;
                f[3] = to->unit[u].objTCoeff.w;
                pState->diff_api.TexGenfv (GL_T, GL_OBJECT_PLANE, (const GLfloat *) f);
                from->unit[u].objTCoeff = to->unit[u].objTCoeff;
            }
            if (from->unit[u].objRCoeff.x != to->unit[u].objRCoeff.x ||
                    from->unit[u].objRCoeff.y != to->unit[u].objRCoeff.y ||
                    from->unit[u].objRCoeff.z != to->unit[u].objRCoeff.z ||
                    from->unit[u].objRCoeff.w != to->unit[u].objRCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].objRCoeff.x;
                f[1] = to->unit[u].objRCoeff.y;
                f[2] = to->unit[u].objRCoeff.z;
                f[3] = to->unit[u].objRCoeff.w;
                pState->diff_api.TexGenfv (GL_R, GL_OBJECT_PLANE, (const GLfloat *) f);
                from->unit[u].objRCoeff = to->unit[u].objRCoeff;
            }
            if (from->unit[u].objQCoeff.x != to->unit[u].objQCoeff.x ||
                    from->unit[u].objQCoeff.y != to->unit[u].objQCoeff.y ||
                    from->unit[u].objQCoeff.z != to->unit[u].objQCoeff.z ||
                    from->unit[u].objQCoeff.w != to->unit[u].objQCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].objQCoeff.x;
                f[1] = to->unit[u].objQCoeff.y;
                f[2] = to->unit[u].objQCoeff.z;
                f[3] = to->unit[u].objQCoeff.w;
                pState->diff_api.TexGenfv (GL_Q, GL_OBJECT_PLANE, (const GLfloat *) f);
                from->unit[u].objQCoeff = to->unit[u].objQCoeff;
            }
            CLEARDIRTY(tb->objGen[u], nbitID);
        }
        if (CHECKDIRTY(tb->eyeGen[u], bitID)) 
        {
            if (fromCtx->texture.curTextureUnit != u) {
                pState->diff_api.ActiveTextureARB( u + GL_TEXTURE0_ARB );
                fromCtx->texture.curTextureUnit = u;
            }
            if (fromCtx->transform.matrixMode != GL_MODELVIEW) {
                pState->diff_api.MatrixMode(GL_MODELVIEW);
                fromCtx->transform.matrixMode = GL_MODELVIEW;
            }
            pState->diff_api.PushMatrix();
            pState->diff_api.LoadIdentity();
            if (from->unit[u].eyeSCoeff.x != to->unit[u].eyeSCoeff.x ||
                    from->unit[u].eyeSCoeff.y != to->unit[u].eyeSCoeff.y ||
                    from->unit[u].eyeSCoeff.z != to->unit[u].eyeSCoeff.z ||
                    from->unit[u].eyeSCoeff.w != to->unit[u].eyeSCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].eyeSCoeff.x;
                f[1] = to->unit[u].eyeSCoeff.y;
                f[2] = to->unit[u].eyeSCoeff.z;
                f[3] = to->unit[u].eyeSCoeff.w;
                pState->diff_api.TexGenfv (GL_S, GL_EYE_PLANE, (const GLfloat *) f);
                from->unit[u].eyeSCoeff = to->unit[u].eyeSCoeff;
            }
            if (from->unit[u].eyeTCoeff.x != to->unit[u].eyeTCoeff.x ||
                    from->unit[u].eyeTCoeff.y != to->unit[u].eyeTCoeff.y ||
                    from->unit[u].eyeTCoeff.z != to->unit[u].eyeTCoeff.z ||
                    from->unit[u].eyeTCoeff.w != to->unit[u].eyeTCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].eyeTCoeff.x;
                f[1] = to->unit[u].eyeTCoeff.y;
                f[2] = to->unit[u].eyeTCoeff.z;
                f[3] = to->unit[u].eyeTCoeff.w;
                pState->diff_api.TexGenfv (GL_T, GL_EYE_PLANE, (const GLfloat *) f);
                from->unit[u].eyeTCoeff = to->unit[u].eyeTCoeff;
            }
            if (from->unit[u].eyeRCoeff.x != to->unit[u].eyeRCoeff.x ||
                    from->unit[u].eyeRCoeff.y != to->unit[u].eyeRCoeff.y ||
                    from->unit[u].eyeRCoeff.z != to->unit[u].eyeRCoeff.z ||
                    from->unit[u].eyeRCoeff.w != to->unit[u].eyeRCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].eyeRCoeff.x;
                f[1] = to->unit[u].eyeRCoeff.y;
                f[2] = to->unit[u].eyeRCoeff.z;
                f[3] = to->unit[u].eyeRCoeff.w;
                pState->diff_api.TexGenfv (GL_R, GL_EYE_PLANE, (const GLfloat *) f);
                from->unit[u].eyeRCoeff = to->unit[u].eyeRCoeff;
            }
            if (from->unit[u].eyeQCoeff.x != to->unit[u].eyeQCoeff.x ||
                    from->unit[u].eyeQCoeff.y != to->unit[u].eyeQCoeff.y ||
                    from->unit[u].eyeQCoeff.z != to->unit[u].eyeQCoeff.z ||
                    from->unit[u].eyeQCoeff.w != to->unit[u].eyeQCoeff.w) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].eyeQCoeff.x;
                f[1] = to->unit[u].eyeQCoeff.y;
                f[2] = to->unit[u].eyeQCoeff.z;
                f[3] = to->unit[u].eyeQCoeff.w;
                pState->diff_api.TexGenfv (GL_Q, GL_EYE_PLANE, (const GLfloat *) f);
                from->unit[u].eyeQCoeff = to->unit[u].eyeQCoeff;
            }
            pState->diff_api.PopMatrix();
            CLEARDIRTY(tb->eyeGen[u], nbitID);
        }
        if (CHECKDIRTY(tb->genMode[u], bitID)) 
        {
            if (fromCtx->texture.curTextureUnit != u) {
                pState->diff_api.ActiveTextureARB( u + GL_TEXTURE0_ARB );
                fromCtx->texture.curTextureUnit = u;
            }
            if (from->unit[u].gen.s != to->unit[u].gen.s ||
                    from->unit[u].gen.t != to->unit[u].gen.t ||
                    from->unit[u].gen.r != to->unit[u].gen.r ||
                    from->unit[u].gen.q != to->unit[u].gen.q) 
            {
                pState->diff_api.TexGeni (GL_S, GL_TEXTURE_GEN_MODE, to->unit[u].gen.s);
                pState->diff_api.TexGeni (GL_T, GL_TEXTURE_GEN_MODE, to->unit[u].gen.t);
                pState->diff_api.TexGeni (GL_R, GL_TEXTURE_GEN_MODE, to->unit[u].gen.r);
                pState->diff_api.TexGeni (GL_Q, GL_TEXTURE_GEN_MODE, to->unit[u].gen.q);    
                from->unit[u].gen = to->unit[u].gen;
            }
            CLEARDIRTY(tb->genMode[u], nbitID);
        }

        /* Texture enviroment  */
        if (CHECKDIRTY(tb->envBit[u], bitID)) 
        {
            if (from->unit[u].envMode != to->unit[u].envMode) 
            {
                pState->diff_api.TexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, to->unit[u].envMode);
                from->unit[u].envMode = to->unit[u].envMode;
            }
            if (from->unit[u].envColor.r != to->unit[u].envColor.r ||
                    from->unit[u].envColor.g != to->unit[u].envColor.g ||
                    from->unit[u].envColor.b != to->unit[u].envColor.b ||
                    from->unit[u].envColor.a != to->unit[u].envColor.a) 
            {
                GLfloat f[4];
                f[0] = to->unit[u].envColor.r;
                f[1] = to->unit[u].envColor.g;
                f[2] = to->unit[u].envColor.b;
                f[3] = to->unit[u].envColor.a;
                pState->diff_api.TexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (const GLfloat *) f);
                from->unit[u].envColor = to->unit[u].envColor;
            }
#ifdef CR_ARB_texture_env_combine
            if (from->unit[u].combineModeRGB != to->unit[u].combineModeRGB)
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, to->unit[u].combineModeRGB);
                from->unit[u].combineModeRGB = to->unit[u].combineModeRGB;
            }
            if (from->unit[u].combineModeA != to->unit[u].combineModeA)
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, to->unit[u].combineModeA);
                from->unit[u].combineModeA = to->unit[u].combineModeA;
            }
            if (from->unit[u].combineSourceRGB[0] != to->unit[u].combineSourceRGB[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, to->unit[u].combineSourceRGB[0]);
                from->unit[u].combineSourceRGB[0] = to->unit[u].combineSourceRGB[0];
            }
            if (from->unit[u].combineSourceRGB[1] != to->unit[u].combineSourceRGB[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, to->unit[u].combineSourceRGB[1]);
                from->unit[u].combineSourceRGB[1] = to->unit[u].combineSourceRGB[1];
            }
            if (from->unit[u].combineSourceRGB[2] != to->unit[u].combineSourceRGB[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, to->unit[u].combineSourceRGB[2]);
                from->unit[u].combineSourceRGB[2] = to->unit[u].combineSourceRGB[2];
            }
            if (from->unit[u].combineSourceA[0] != to->unit[u].combineSourceA[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, to->unit[u].combineSourceA[0]);
                from->unit[u].combineSourceA[0] = to->unit[u].combineSourceA[0];
            }
            if (from->unit[u].combineSourceA[1] != to->unit[u].combineSourceA[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, to->unit[u].combineSourceA[1]);
                from->unit[u].combineSourceA[1] = to->unit[u].combineSourceA[1];
            }
            if (from->unit[u].combineSourceA[2] != to->unit[u].combineSourceA[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, to->unit[u].combineSourceA[2]);
                from->unit[u].combineSourceA[2] = to->unit[u].combineSourceA[2];
            }
            if (from->unit[u].combineOperandRGB[0] != to->unit[u].combineOperandRGB[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, to->unit[u].combineOperandRGB[0]);
                from->unit[u].combineOperandRGB[0] = to->unit[u].combineOperandRGB[0];
            }
            if (from->unit[u].combineOperandRGB[1] != to->unit[u].combineOperandRGB[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, to->unit[u].combineOperandRGB[1]);
                from->unit[u].combineOperandRGB[1] = to->unit[u].combineOperandRGB[1];
            }
            if (from->unit[u].combineOperandRGB[2] != to->unit[u].combineOperandRGB[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, to->unit[u].combineOperandRGB[2]);
                from->unit[u].combineOperandRGB[2] = to->unit[u].combineOperandRGB[2];
            }
            if (from->unit[u].combineOperandA[0] != to->unit[u].combineOperandA[0])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, to->unit[u].combineOperandA[0]);
                from->unit[u].combineOperandA[0] = to->unit[u].combineOperandA[0];
            }
            if (from->unit[u].combineOperandA[1] != to->unit[u].combineOperandA[1])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, to->unit[u].combineOperandA[1]);
                from->unit[u].combineOperandA[1] = to->unit[u].combineOperandA[1];
            }
            if (from->unit[u].combineOperandA[2] != to->unit[u].combineOperandA[2])
            {
                pState->diff_api.TexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, to->unit[u].combineOperandA[2]);
                from->unit[u].combineOperandA[2] = to->unit[u].combineOperandA[2];
            }
            if (from->unit[u].combineScaleRGB != to->unit[u].combineScaleRGB)
            {
                pState->diff_api.TexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, to->unit[u].combineScaleRGB);
                from->unit[u].combineScaleRGB = to->unit[u].combineScaleRGB;
            }
            if (from->unit[u].combineScaleA != to->unit[u].combineScaleA)
            {
                pState->diff_api.TexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, to->unit[u].combineScaleA);
                from->unit[u].combineScaleA = to->unit[u].combineScaleA;
            }
#endif
#if CR_EXT_texture_lod_bias
            if (from->unit[u].lodBias != to->unit[u].lodBias)
            {
                pState->diff_api.TexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, to->unit[u].lodBias);
                from->unit[u].lodBias = to->unit[u].lodBias;
            }
#endif
            CLEARDIRTY(tb->envBit[u], nbitID);
        }

        /* loop over texture targets */
        for (t = 0; t < 5; t++)
        {
            tobj = NULL;

            switch (t) {
            case 0:
                if (to->unit[u].enabled1D || haveFragProg) {
                    tobj = to->unit[u].currentTexture1D;
                    fromBinding = &(from->unit[u].currentTexture1D);
                }
                break;
            case 1:
                if (to->unit[u].enabled2D || haveFragProg) {
                    tobj = to->unit[u].currentTexture2D;
                    fromBinding = &(from->unit[u].currentTexture2D);
                }
                break;
#ifdef CR_OPENGL_VERSION_1_2
            case 2:
                if (to->unit[u].enabled3D || haveFragProg) {
                    tobj = to->unit[u].currentTexture3D;
                    fromBinding = &(from->unit[u].currentTexture3D);
                }
                break;
#endif
#ifdef CR_ARB_texture_cube_map
            case 3:
                if (fromCtx->extensions.ARB_texture_cube_map &&
                        (to->unit[u].enabledCubeMap || haveFragProg)) {
                    tobj = to->unit[u].currentTextureCubeMap;
                    fromBinding = &(from->unit[u].currentTextureCubeMap);
                }
                break;
#endif
#ifdef CR_NV_texture_rectangle
            case 4:
                if (fromCtx->extensions.NV_texture_rectangle &&
                        (to->unit[u].enabledRect || haveFragProg)) {
                    tobj = to->unit[u].currentTextureRect;
                    fromBinding = &(from->unit[u].currentTextureRect);
                }
                break;
#endif
            default:
                /* maybe don't support cube maps or rects */
                continue;
            }

            if (!tobj) {
                continue; /* with next target */
            }

            /* Activate texture unit u if needed */
            if (fromCtx->texture.curTextureUnit != u) {
                pState->diff_api.ActiveTextureARB( GL_TEXTURE0_ARB + u);
                fromCtx->texture.curTextureUnit = u;
            }

            /* bind this texture if needed */
            if (CHECKDIRTY(tb->current[u], bitID)) 
            {
                if (*fromBinding != tobj)
                {
                    pState->diff_api.BindTexture(tobj->target, crStateGetTextureObjHWID(pState, tobj));
                    *fromBinding = tobj;
                }
            }

            /* now, if the texture object is dirty */
            if (CHECKDIRTY(tobj->dirty, bitID)) 
            {
                 crStateTextureObjectDiff(fromCtx, bitID, nbitID, tobj, GL_FALSE);
            }
            CLEARDIRTY(tobj->dirty, nbitID);

        } /* loop over targets */

        CLEARDIRTY(tb->current[u], nbitID);

    } /* loop over units */

    /* After possible fiddling with the active unit, put it back now */
    if (fromCtx->texture.curTextureUnit != toCtx->texture.curTextureUnit) {
        pState->diff_api.ActiveTextureARB( toCtx->texture.curTextureUnit + GL_TEXTURE0_ARB );
        fromCtx->texture.curTextureUnit = toCtx->texture.curTextureUnit;
    }
    if (fromCtx->transform.matrixMode != toCtx->transform.matrixMode) {
        pState->diff_api.MatrixMode(toCtx->transform.matrixMode);
        fromCtx->transform.matrixMode = toCtx->transform.matrixMode;
    }

    CLEARDIRTY(tb->dirty, nbitID);
}



struct callback_info
{
    CRbitvalue *bitID, *nbitID;
    CRContext *g;
    GLboolean bForceUpdate;
};


/* Called by crHashtableWalk() below */
static void
DiffTextureObjectCallback( unsigned long key, void *texObj , void *cbData)
{
    struct callback_info *info = (struct callback_info *) cbData;
    CRTextureObj *tobj = (CRTextureObj *) texObj;
    (void)key;

    /*
    printf("  Checking %d 0x%x  bitid=0x%x\n",tobj->name, tobj->dirty[0], info->bitID[0]);
    */
    if (info->bForceUpdate || CHECKDIRTY(tobj->dirty, info->bitID)) {
         /*
        printf("  Found Dirty! %d\n", tobj->name);
         */
        crStateTextureObjectDiff(info->g, info->bitID, info->nbitID, tobj, info->bForceUpdate);
        CLEARDIRTY(tobj->dirty, info->nbitID);
    }
}


/*
 * This isn't used right now, but will be used in the future to fix some
 * potential display list problems.  Specifically, if glBindTexture is
 * in a display list, we have to be sure that all outstanding texture object
 * updates are resolved before the list is called.  If we don't, we may
 * wind up binding texture objects that are stale.
 */
void
crStateDiffAllTextureObjects( CRContext *g, CRbitvalue *bitID, GLboolean bForceUpdate )
{
    PCRStateTracker pState = g->pStateTracker;
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    struct callback_info info;
    int j;
    int origUnit, orig1D, orig2D, orig3D, origCube, origRect;

    for (j = 0; j < CR_MAX_BITARRAY; j++)
        nbitID[j] = ~bitID[j];

    info.bitID = bitID;
    info.nbitID = nbitID;
    info.g = g;
    info.bForceUpdate = bForceUpdate;

    /* save current texture bindings */
    origUnit = g->texture.curTextureUnit;
    orig1D = crStateGetTextureObjHWID(pState, g->texture.unit[0].currentTexture1D);
    orig2D = crStateGetTextureObjHWID(pState, g->texture.unit[0].currentTexture2D);
    orig3D = crStateGetTextureObjHWID(pState, g->texture.unit[0].currentTexture3D);
#ifdef CR_ARB_texture_cube_map
    origCube = crStateGetTextureObjHWID(pState, g->texture.unit[0].currentTextureCubeMap);
#endif
#ifdef CR_NV_texture_rectangle
    origRect = crStateGetTextureObjHWID(pState, g->texture.unit[0].currentTextureRect);
#endif

    /* use texture unit 0 for updates */
    pState->diff_api.ActiveTextureARB(GL_TEXTURE0_ARB);

    /* diff all the textures */
    crHashtableWalk(g->shared->textureTable, DiffTextureObjectCallback, (void *) &info);

    /* default ones */
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.base1D, GL_TRUE);
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.base2D, GL_TRUE);
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.base3D, GL_TRUE);
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.proxy1D, GL_TRUE);
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.proxy2D, GL_TRUE);
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.proxy3D, GL_TRUE);
#ifdef CR_ARB_texture_cube_map
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.baseCubeMap, GL_TRUE);
    crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.proxyCubeMap, GL_TRUE);
#endif
#ifdef CR_NV_texture_rectangle
    if (g->extensions.NV_texture_rectangle)
    {
        crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.baseRect, GL_TRUE);
        crStateTextureObjectDiff(g, bitID, nbitID, &g->texture.proxyRect, GL_TRUE);
    }
#endif

    /* restore bindings */
    /* first restore unit 0 bindings the unit 0 is active currently */
    pState->diff_api.BindTexture(GL_TEXTURE_1D, orig1D);
    pState->diff_api.BindTexture(GL_TEXTURE_2D, orig2D);
    pState->diff_api.BindTexture(GL_TEXTURE_3D, orig3D);
#ifdef CR_ARB_texture_cube_map
    pState->diff_api.BindTexture(GL_TEXTURE_CUBE_MAP_ARB, origCube);
#endif
#ifdef CR_NV_texture_rectangle
    pState->diff_api.BindTexture(GL_TEXTURE_RECTANGLE_NV, origRect);
#endif
    /* now restore the proper active unit */
    pState->diff_api.ActiveTextureARB(GL_TEXTURE0_ARB + origUnit);
}
