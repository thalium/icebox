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

#ifdef DEBUG_misha
#include <iprt/assert.h>
#endif

#define UNUSED(x) ((void) (x))

#define GET_TOBJ(tobj, state, id) \
  tobj = (CRTextureObj *) crHashtableSearch(g->shared->textureTable, id);


void crStateTextureDestroy(CRContext *ctx)
{
    crStateDeleteTextureObjectData(&ctx->texture.base1D);
    crStateDeleteTextureObjectData(&ctx->texture.proxy1D);
    crStateDeleteTextureObjectData(&ctx->texture.base2D);
    crStateDeleteTextureObjectData(&ctx->texture.proxy2D);
#ifdef CR_OPENGL_VERSION_1_2
    crStateDeleteTextureObjectData(&ctx->texture.base3D);
    crStateDeleteTextureObjectData(&ctx->texture.proxy3D);
#endif
#ifdef CR_ARB_texture_cube_map
    crStateDeleteTextureObjectData(&ctx->texture.baseCubeMap);
    crStateDeleteTextureObjectData(&ctx->texture.proxyCubeMap);
#endif
#ifdef CR_NV_texture_rectangle
    crStateDeleteTextureObjectData(&ctx->texture.baseRect);
    crStateDeleteTextureObjectData(&ctx->texture.proxyRect);
#endif
}


void crStateTextureInit(CRContext *ctx)
{
    CRLimitsState *limits = &ctx->limits;
    CRTextureState *t = &ctx->texture;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    unsigned int i;
    unsigned int a;
    GLvectorf zero_vector = {0.0f, 0.0f, 0.0f, 0.0f};
    GLcolorf zero_color = {0.0f, 0.0f, 0.0f, 0.0f};
    GLvectorf x_vector = {1.0f, 0.0f, 0.0f, 0.0f};
    GLvectorf y_vector = {0.0f, 1.0f, 0.0f, 0.0f};

    /* compute max levels from max sizes */
    for (i=0, a=limits->maxTextureSize; a; i++, a=a>>1);
    t->maxLevel = i-1;
    for (i=0, a=limits->max3DTextureSize; a; i++, a=a>>1);
    t->max3DLevel = i-1;
#ifdef CR_ARB_texture_cube_map
    for (i=0, a=limits->maxCubeMapTextureSize; a; i++, a=a>>1);
    t->maxCubeMapLevel = i-1;
#endif
#ifdef CR_NV_texture_rectangle
    for (i=0, a=limits->maxRectTextureSize; a; i++, a=a>>1);
    t->maxRectLevel = i-1;
#endif

    crStateTextureInitTextureObj(ctx, &(t->base1D), 0, GL_TEXTURE_1D);
    crStateTextureInitTextureObj(ctx, &(t->base2D), 0, GL_TEXTURE_2D);
#ifdef CR_OPENGL_VERSION_1_2
    crStateTextureInitTextureObj(ctx, &(t->base3D), 0, GL_TEXTURE_3D);
#endif
#ifdef CR_ARB_texture_cube_map
    crStateTextureInitTextureObj(ctx, &(t->baseCubeMap), 0,
        GL_TEXTURE_CUBE_MAP_ARB);
#endif
#ifdef CR_NV_texture_rectangle
    crStateTextureInitTextureObj(ctx, &(t->baseRect), 0,
        GL_TEXTURE_RECTANGLE_NV);
#endif

    crStateTextureInitTextureObj(ctx, &(t->proxy1D), 0, GL_TEXTURE_1D);
    crStateTextureInitTextureObj(ctx, &(t->proxy2D), 0, GL_TEXTURE_2D);
#ifdef CR_OPENGL_VERSION_1_2
    crStateTextureInitTextureObj(ctx, &(t->proxy3D), 0, GL_TEXTURE_3D);
#endif
#ifdef CR_ARB_texture_cube_map
    crStateTextureInitTextureObj(ctx, &(t->proxyCubeMap), 0,
        GL_TEXTURE_CUBE_MAP_ARB);
#endif
#ifdef CR_NV_texture_rectangle
    crStateTextureInitTextureObj(ctx, &(t->proxyRect), 0,
        GL_TEXTURE_RECTANGLE_NV);
#endif

    t->curTextureUnit = 0;

    /* Per-unit initialization */
    for (i = 0; i < limits->maxTextureUnits; i++)
    {
        t->unit[i].currentTexture1D = &(t->base1D);
        t->unit[i].currentTexture2D = &(t->base2D);
        t->unit[i].currentTexture3D = &(t->base3D);
#ifdef CR_ARB_texture_cube_map
        t->unit[i].currentTextureCubeMap = &(t->baseCubeMap);
#endif
#ifdef CR_NV_texture_rectangle
        t->unit[i].currentTextureRect = &(t->baseRect);
#endif

        t->unit[i].enabled1D = GL_FALSE;
        t->unit[i].enabled2D = GL_FALSE;
        t->unit[i].enabled3D = GL_FALSE;
        t->unit[i].enabledCubeMap = GL_FALSE;
#ifdef CR_NV_texture_rectangle
        t->unit[i].enabledRect = GL_FALSE;
#endif
        t->unit[i].textureGen.s = GL_FALSE;
        t->unit[i].textureGen.t = GL_FALSE;
        t->unit[i].textureGen.r = GL_FALSE;
        t->unit[i].textureGen.q = GL_FALSE;

        t->unit[i].gen.s = GL_EYE_LINEAR;
        t->unit[i].gen.t = GL_EYE_LINEAR;
        t->unit[i].gen.r = GL_EYE_LINEAR;
        t->unit[i].gen.q = GL_EYE_LINEAR;

        t->unit[i].objSCoeff = x_vector;
        t->unit[i].objTCoeff = y_vector;
        t->unit[i].objRCoeff = zero_vector;
        t->unit[i].objQCoeff = zero_vector;

        t->unit[i].eyeSCoeff = x_vector;
        t->unit[i].eyeTCoeff = y_vector;
        t->unit[i].eyeRCoeff = zero_vector;
        t->unit[i].eyeQCoeff = zero_vector;
        t->unit[i].envMode = GL_MODULATE;
        t->unit[i].envColor = zero_color;

        t->unit[i].combineModeRGB = GL_MODULATE;
        t->unit[i].combineModeA = GL_MODULATE;
        t->unit[i].combineSourceRGB[0] = GL_TEXTURE;
        t->unit[i].combineSourceRGB[1] = GL_PREVIOUS_EXT;
        t->unit[i].combineSourceRGB[2] = GL_CONSTANT_EXT;
        t->unit[i].combineSourceA[0] = GL_TEXTURE;
        t->unit[i].combineSourceA[1] = GL_PREVIOUS_EXT;
        t->unit[i].combineSourceA[2] = GL_CONSTANT_EXT;
        t->unit[i].combineOperandRGB[0] = GL_SRC_COLOR;
        t->unit[i].combineOperandRGB[1] = GL_SRC_COLOR;
        t->unit[i].combineOperandRGB[2] = GL_SRC_ALPHA;
        t->unit[i].combineOperandA[0] = GL_SRC_ALPHA;
        t->unit[i].combineOperandA[1] = GL_SRC_ALPHA;
        t->unit[i].combineOperandA[2] = GL_SRC_ALPHA;
        t->unit[i].combineScaleRGB = 1.0F;
        t->unit[i].combineScaleA = 1.0F;
#ifdef CR_EXT_texture_lod_bias
        t->unit[i].lodBias = 0.0F;
#endif
        RESET(tb->enable[i], ctx->bitid);
        RESET(tb->current[i], ctx->bitid);
        RESET(tb->objGen[i], ctx->bitid);
        RESET(tb->eyeGen[i], ctx->bitid);
        RESET(tb->genMode[i], ctx->bitid);
        RESET(tb->envBit[i], ctx->bitid);
    }
    RESET(tb->dirty, ctx->bitid);
}


void
crStateTextureInitTextureObj(CRContext *ctx, CRTextureObj *tobj,
                             GLuint name, GLenum target)
{
    const CRTextureState *t = &(ctx->texture);
    int i, face;

    tobj->borderColor.r = 0.0f;
    tobj->borderColor.g = 0.0f;
    tobj->borderColor.b = 0.0f;
    tobj->borderColor.a = 0.0f;
    tobj->minFilter     = GL_NEAREST_MIPMAP_LINEAR;
    tobj->magFilter     = GL_LINEAR;
    tobj->wrapS         = GL_REPEAT;
    tobj->wrapT         = GL_REPEAT;
#ifdef CR_OPENGL_VERSION_1_2
    tobj->wrapR         = GL_REPEAT;
    tobj->priority      = 1.0f;
    tobj->minLod        = -1000.0;
    tobj->maxLod        = 1000.0;
    tobj->baseLevel     = 0;
    tobj->maxLevel      = t->maxLevel;
#endif
    tobj->target        = target;
    tobj->id            = name;
    tobj->hwid          = 0;

#ifndef IN_GUEST
    crStateGetTextureObjHWID(tobj);
#endif

    CRASSERT(t->maxLevel);

    /* XXX don't always need all six faces */
    for (face = 0; face < 6; face++) {
        /* allocate array of mipmap levels */
        CRASSERT(t->maxLevel < CR_MAX_MIPMAP_LEVELS);
        tobj->level[face] = (CRTextureLevel *)
            crCalloc(sizeof(CRTextureLevel) * CR_MAX_MIPMAP_LEVELS);

        if (!tobj->level[face])
            return; /* out of memory */

        /* init non-zero fields */
        for (i = 0; i <= t->maxLevel; i++) {
            CRTextureLevel *tl = &(tobj->level[face][i]);
            tl->internalFormat = GL_ONE;
            tl->format = GL_RGBA;
            tl->type = GL_UNSIGNED_BYTE;
            crStateTextureInitTextureFormat( tl, tl->internalFormat );
        }
    }

#ifdef CR_EXT_texture_filter_anisotropic
    tobj->maxAnisotropy = 1.0f;
#endif

#ifdef CR_ARB_depth_texture
    tobj->depthMode = GL_LUMINANCE;
#endif

#ifdef CR_ARB_shadow
    tobj->compareMode = GL_NONE;
    tobj->compareFunc = GL_LEQUAL;
#endif

#ifdef CR_ARB_shadow_ambient
    tobj->compareFailValue = 0.0;
#endif

    RESET(tobj->dirty, ctx->bitid);
    RESET(tobj->imageBit, ctx->bitid);
    for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++)
    {
        RESET(tobj->paramsBit[i], ctx->bitid);
    }

    CR_STATE_SHAREDOBJ_USAGE_INIT(tobj);
    CR_STATE_SHAREDOBJ_USAGE_SET(tobj, ctx);
}


/* ================================================================
 * Texture internal formats:
 */

const CRTextureFormat _texformat_rgba8888 = {
   8,               /* RedBits */
   8,               /* GreenBits */
   8,               /* BlueBits */
   8,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_argb8888 = {
   8,               /* RedBits */
   8,               /* GreenBits */
   8,               /* BlueBits */
   8,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_rgb888 = {
   8,               /* RedBits */
   8,               /* GreenBits */
   8,               /* BlueBits */
   0,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_rgb565 = {
   5,               /* RedBits */
   6,               /* GreenBits */
   5,               /* BlueBits */
   0,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_argb4444 = {
   4,               /* RedBits */
   4,               /* GreenBits */
   4,               /* BlueBits */
   4,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_argb1555 = {
   5,               /* RedBits */
   5,               /* GreenBits */
   5,               /* BlueBits */
   1,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_al88 = {
   0,               /* RedBits */
   0,               /* GreenBits */
   0,               /* BlueBits */
   8,               /* AlphaBits */
   8,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_rgb332 = {
   3,               /* RedBits */
   3,               /* GreenBits */
   2,               /* BlueBits */
   0,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_a8 = {
   0,               /* RedBits */
   0,               /* GreenBits */
   0,               /* BlueBits */
   8,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_l8 = {
   0,               /* RedBits */
   0,               /* GreenBits */
   0,               /* BlueBits */
   0,               /* AlphaBits */
   8,               /* LuminanceBits */
   0,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_i8 = {
   0,               /* RedBits */
   0,               /* GreenBits */
   0,               /* BlueBits */
   0,               /* AlphaBits */
   0,               /* LuminanceBits */
   8,               /* IntensityBits */
   0,               /* IndexBits */
};

const CRTextureFormat _texformat_ci8 = {
   0,               /* RedBits */
   0,               /* GreenBits */
   0,               /* BlueBits */
   0,               /* AlphaBits */
   0,               /* LuminanceBits */
   0,               /* IntensityBits */
   8,               /* IndexBits */
};


/**
 * Given an internal texture format enum or 1, 2, 3, 4 initialize the
 * texture levels texture format.  This basically just indicates the
 * number of red, green, blue, alpha, luminance, etc. bits are used to
 * store the image.
 */
void
crStateTextureInitTextureFormat( CRTextureLevel *tl, GLenum internalFormat )
{
    switch (internalFormat) {
    case 4:
    case GL_RGBA:
    case GL_COMPRESSED_RGBA_ARB:
#ifdef CR_EXT_texture_sRGB
    case GL_SRGB_ALPHA_EXT:
    case GL_SRGB8_ALPHA8_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_EXT:
#endif
        tl->texFormat = &_texformat_rgba8888;
        break;

    case 3:
    case GL_RGB:
    case GL_COMPRESSED_RGB_ARB:
#ifdef CR_EXT_texture_sRGB
    case GL_SRGB_EXT:
    case GL_SRGB8_EXT:
    case GL_COMPRESSED_SRGB_EXT:
#endif
        tl->texFormat = &_texformat_rgb888;
        break;

    case GL_RGBA2:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGBA8:
    case GL_RGB10_A2:
    case GL_RGBA12:
    case GL_RGBA16:
        tl->texFormat = &_texformat_rgba8888;
        break;

    case GL_R3_G3_B2:
        tl->texFormat = &_texformat_rgb332;
        break;
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGB8:
    case GL_RGB10:
    case GL_RGB12:
    case GL_RGB16:
        tl->texFormat = &_texformat_rgb888;
        break;

    case GL_ALPHA:
    case GL_ALPHA4:
    case GL_ALPHA8:
    case GL_ALPHA12:
    case GL_ALPHA16:
    case GL_COMPRESSED_ALPHA_ARB:
        tl->texFormat = &_texformat_a8;
        break;

    case 1:
    case GL_LUMINANCE:
    case GL_LUMINANCE4:
    case GL_LUMINANCE8:
    case GL_LUMINANCE12:
    case GL_LUMINANCE16:
    case GL_COMPRESSED_LUMINANCE_ARB:
#ifdef CR_EXT_texture_sRGB
    case GL_SLUMINANCE_EXT:
    case GL_SLUMINANCE8_EXT:
    case GL_COMPRESSED_SLUMINANCE_EXT:
#endif
        tl->texFormat = &_texformat_l8;
        break;

    case 2:
    case GL_LUMINANCE_ALPHA:
    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE6_ALPHA2:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE12_ALPHA4:
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16:
    case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
#ifdef CR_EXT_texture_sRGB
    case GL_SLUMINANCE_ALPHA_EXT:
    case GL_SLUMINANCE8_ALPHA8_EXT:
    case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
#endif
        tl->texFormat = &_texformat_al88;
        break;

    case GL_INTENSITY:
    case GL_INTENSITY4:
    case GL_INTENSITY8:
    case GL_INTENSITY12:
    case GL_INTENSITY16:
    case GL_COMPRESSED_INTENSITY_ARB:
        tl->texFormat = &_texformat_i8;
        break;

    case GL_COLOR_INDEX:
    case GL_COLOR_INDEX1_EXT:
    case GL_COLOR_INDEX2_EXT:
    case GL_COLOR_INDEX4_EXT:
    case GL_COLOR_INDEX8_EXT:
    case GL_COLOR_INDEX12_EXT:
    case GL_COLOR_INDEX16_EXT:
        tl->texFormat = &_texformat_ci8;
        break;

    default:
        return;
    }
}

#if 0
void crStateTextureInitTexture (GLuint name) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj;

    GET_TOBJ(tobj, name);
    if (!tobj) return;

    crStateTextureInitTextureObj(g, tobj, name, GL_NONE);
}
#endif



/**
 * Return the texture object corresponding to the given target and ID.
 */
CRTextureObj *
crStateTextureGet(GLenum target, GLuint name) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj;

    if (name == 0)
    {
        switch (target) {
        case GL_TEXTURE_1D:
            return &t->base1D;
        case GL_TEXTURE_2D:
            return &t->base2D;
        case GL_TEXTURE_3D:
            return &t->base3D;
#ifdef CR_ARB_texture_cube_map
        case GL_TEXTURE_CUBE_MAP_ARB:
            return &t->baseCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
        case GL_TEXTURE_RECTANGLE_NV:
            return &t->baseRect;
#endif
        default:
            return NULL;
        }
    }

    GET_TOBJ(tobj, g, name);

    return tobj;
}


/*
 * Allocate a new texture object with the given name.
 * Also insert into hash table.
 */
static CRTextureObj *
crStateTextureAllocate_t(CRContext *ctx, GLuint name) 
{
    CRTextureObj *tobj;

    if (!name) 
        return NULL;

    tobj = crCalloc(sizeof(CRTextureObj));
    if (!tobj)
        return NULL;

    crHashtableAdd( ctx->shared->textureTable, name, (void *) tobj );

    crStateTextureInitTextureObj(ctx, tobj, name, GL_NONE);

    return tobj;
}


/**
 * Delete all the data that hangs off a CRTextureObj, but don't
 * delete the texture object itself, since it may not have been
 * dynamically allocated.
 */
void
crStateDeleteTextureObjectData(CRTextureObj *tobj)
{
    int k;
    int face;

    CRASSERT(tobj);

    /* Free the texture images */
    for (face = 0; face < 6; face++) {
        CRTextureLevel *levels = NULL;
        levels = tobj->level[face];
        if (levels) {
            /* free all mipmap levels for this face */
            for (k = 0; k < CR_MAX_MIPMAP_LEVELS; k++) {
                CRTextureLevel *tl = levels + k;
                if (tl->img) {
                    crFree(tl->img);
                     tl->img = NULL;
                     tl->bytes = 0;
                }
            }
            crFree(levels);
        }
        tobj->level[face] = NULL;
    }
}


void
crStateDeleteTextureObject(CRTextureObj *tobj)
{
    crStateDeleteTextureObjectData(tobj);
    crFree(tobj);
}

void crStateRegNames(CRContext *g, CRHashTable *table, GLsizei n, GLuint *names)
{
    GLint i;
    (void)g;
    for (i = 0; i < n; i++)
    {
        if (names[i])
        {
            GLboolean isNewKey = crHashtableAllocRegisterKey(table, names[i]);
            CRASSERT(isNewKey);
        }
        else
            crWarning("RegNames: requested to register a null name");
    }
}

void crStateRegTextures(GLsizei n, GLuint *names)
{
    CRContext *g = GetCurrentContext();
    crStateRegNames(g, g->shared->textureTable, n, names);
}

void crStateGenNames(CRContext *g, CRHashTable *table, GLsizei n, GLuint *names)
{
    GLint start;

    FLUSH();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "crStateGenNames called in Begin/End");
        return;
    }

    if (n < 0)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "Negative n passed to crStateGenNames: %d", n);
        return;
    }

    start = crHashtableAllocKeys(table, n);
    if (start)
    {
        GLint i;
        for (i = 0; i < n; i++)
            names[i] = (GLuint) (start + i);
    }
    else
    {
        crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY, "glGenTextures");
    }
}

void STATE_APIENTRY crStateGenTextures(GLsizei n, GLuint *textures) 
{
    CRContext *g = GetCurrentContext();
    crStateGenNames(g, g->shared->textureTable, n, textures);
}

static void crStateTextureCheckFBOAPs(GLenum target, GLuint texture)
{
    GLuint u;
    CRFBOAttachmentPoint *ap;
    CRContext *g = GetCurrentContext();
    CRFramebufferObjectState *fbo = &g->framebufferobject;
    CRFramebufferObject *pFBO;

    pFBO = GL_READ_FRAMEBUFFER==target ? fbo->readFB : fbo->drawFB;
    if (!pFBO) return;

    for (u=0; u<CR_MAX_COLOR_ATTACHMENTS; ++u)
    {
        ap = &pFBO->color[u];
        if (ap->type==GL_TEXTURE && ap->name==texture)
        {
            crStateFramebufferTexture1DEXT(target, u+GL_COLOR_ATTACHMENT0_EXT, 0, 0, 0);
        }
    }

    ap = &pFBO->depth;
    if (ap->type==GL_TEXTURE && ap->name==texture)
    {
        crStateFramebufferTexture1DEXT(target, GL_DEPTH_ATTACHMENT_EXT, 0, 0, 0);
    }

    ap = &pFBO->stencil;
    if (ap->type==GL_TEXTURE && ap->name==texture)
    {
        crStateFramebufferTexture1DEXT(target, GL_STENCIL_ATTACHMENT_EXT, 0, 0, 0);
    }
}

static void crStateCleanupTextureRefs(CRContext *g, CRTextureObj *tObj)
{
    CRTextureState *t = &(g->texture);
    GLuint u;

    /*
     ** reset back to the base texture.
     */
    for (u = 0; u < g->limits.maxTextureUnits; u++)
    {
        if (tObj == t->unit[u].currentTexture1D)
        {
            t->unit[u].currentTexture1D = &(t->base1D);
        }
        if (tObj == t->unit[u].currentTexture2D)
        {
            t->unit[u].currentTexture2D = &(t->base2D);
        }
#ifdef CR_OPENGL_VERSION_1_2
        if (tObj == t->unit[u].currentTexture3D)
        {
            t->unit[u].currentTexture3D = &(t->base3D);
        }
#endif
#ifdef CR_ARB_texture_cube_map
        if (tObj == t->unit[u].currentTextureCubeMap)
        {
            t->unit[u].currentTextureCubeMap = &(t->baseCubeMap);
        }
#endif
#ifdef CR_NV_texture_rectangle
        if (tObj == t->unit[u].currentTextureRect)
        {
            t->unit[u].currentTextureRect = &(t->baseRect);
        }
#endif

#ifdef CR_EXT_framebuffer_object
        crStateTextureCheckFBOAPs(GL_DRAW_FRAMEBUFFER, tObj->id);
        crStateTextureCheckFBOAPs(GL_READ_FRAMEBUFFER, tObj->id);
#endif
    }
}

void STATE_APIENTRY crStateDeleteTextures(GLsizei n, const GLuint *textures) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    int i;

    FLUSH();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glDeleteTextures called in Begin/End");
        return;
    }

    if (n < 0)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "Negative n passed to glDeleteTextures: %d", n);
        return;
    }

    for (i=0; i<n; i++) 
    {
        GLuint name = textures[i];
        CRTextureObj *tObj;
        if (!name)
            continue;

        GET_TOBJ(tObj, g, name);
        if (tObj)
        {
            GLuint j;

            crStateCleanupTextureRefs(g, tObj);

            CR_STATE_SHAREDOBJ_USAGE_CLEAR(tObj, g);

            CR_STATE_SHAREDOBJ_USAGE_FOREACH_USED_IDX(tObj, j)
            {
                /* saved state version <= SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS does not have usage bits info,
                 * so on restore, we set mark bits as used.
                 * This is why g_pAvailableContexts[j] could be NULL
                 * also g_pAvailableContexts[0] will hold default context, which we should discard */
                CRContext *ctx = g_pAvailableContexts[j];
                if (j && ctx)
                {
                    crStateCleanupTextureRefs(ctx, tObj);
                    CR_STATE_SHAREDOBJ_USAGE_CLEAR(tObj, g);
                }
                else
                    CR_STATE_SHAREDOBJ_USAGE_CLEAR_IDX(tObj, j);
            }

            /* on the host side, ogl texture object is deleted by a separate cr_server.head_spu->dispatch_table.DeleteTextures(n, newTextures);
             * in crServerDispatchDeleteTextures, we just delete a state object here, which crStateDeleteTextureObject does */
            crHashtableDelete(g->shared->textureTable, name, (CRHashtableCallback)crStateDeleteTextureObject);
        }
        else
        {
            /* call crHashtableDelete in any way, to ensure the allocated key is freed */
            Assert(crHashtableIsKeyUsed(g->shared->textureTable, name));
            crHashtableDelete(g->shared->textureTable, name, NULL);
        }
    }

    DIRTY(tb->dirty, g->neg_bitid);
    DIRTY(tb->current[t->curTextureUnit], g->neg_bitid);
}



void STATE_APIENTRY crStateClientActiveTextureARB( GLenum texture )
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);

    FLUSH();

    if (!g->extensions.ARB_multitexture) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glClientActiveTextureARB not available");
        return;
    }

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glClientActiveTextureARB called in Begin/End");
        return;
    }

    if ( texture < GL_TEXTURE0_ARB ||
             texture >= GL_TEXTURE0_ARB + g->limits.maxTextureUnits)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "crStateClientActiveTexture: unit = %d (max is %d)",
                                 texture, g->limits.maxTextureUnits );
        return;
    }

    c->curClientTextureUnit = texture - GL_TEXTURE0_ARB;

    DIRTY(GetCurrentBits()->client.dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateActiveTextureARB( GLenum texture )
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);

    FLUSH();

    if (!g->extensions.ARB_multitexture) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glActiveTextureARB not available");
        return;
    }

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glActiveTextureARB called in Begin/End");
        return;
    }

    if ( texture < GL_TEXTURE0_ARB || texture >= GL_TEXTURE0_ARB + g->limits.maxTextureUnits)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "Bad texture unit passed to crStateActiveTexture: %d (max is %d)", texture, g->limits.maxTextureUnits );
        return;
    }

    t->curTextureUnit = texture - GL_TEXTURE0_ARB;

    /* update the current matrix pointer, etc. */
    if (g->transform.matrixMode == GL_TEXTURE) {
        crStateMatrixMode(GL_TEXTURE);
    }
}

#ifndef IN_GUEST
# ifdef DEBUG
static uint32_t gDbgNumPinned = 0;
# endif

DECLEXPORT(void) crStatePinTexture(GLuint texture, GLboolean pin)
{
    CRTextureObj * pTobj;
    CRSharedState *pShared = crStateGlobalSharedAcquire();
    if (pShared)
    {
        pTobj = (CRTextureObj*)crHashtableSearch(pShared->textureTable, texture);

        if (pTobj)
        {
# ifdef DEBUG
            if (!pTobj->pinned != !pin)
            {
                if (pin)
                    ++gDbgNumPinned;
                else
                {
                    Assert(gDbgNumPinned);
                    --gDbgNumPinned;
                }
            }
# endif
            pTobj->pinned = !!pin;
            if (!pin)
            {
                if (!CR_STATE_SHAREDOBJ_USAGE_IS_USED(pTobj))
                    crStateOnTextureUsageRelease(pShared, pTobj);
            }
        }
        else
            WARN(("texture %d not defined", texture));

        crStateGlobalSharedRelease();
    }
    else
        WARN(("no global shared"));
}
#endif

DECLEXPORT(void) crStateSetTextureUsed(GLuint texture, GLboolean used)
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;

    if (!texture)
    {
        crWarning("crStateSetTextureUsed: null texture name specified!");
        return;
    }

    GET_TOBJ(tobj, g, texture);
    if (!tobj)
    {
#ifdef IN_GUEST
        if (used)
        {
            tobj = crStateTextureAllocate_t(g, texture);
        }
        else
#endif
        {
            WARN(("crStateSetTextureUsed: failed to fined a HW name for texture(%d)!", texture));
            return;
        }
    }

    if (used)
        CR_STATE_SHAREDOBJ_USAGE_SET(tobj, g);
    else
    {
        CRStateBits *sb = GetCurrentBits();
        CRTextureBits *tb = &(sb->texture);
        CRTextureState *t = &(g->texture);

        crStateCleanupTextureRefs(g, tobj);

        crStateReleaseTexture(g, tobj);

        DIRTY(tb->dirty, g->neg_bitid);
        DIRTY(tb->current[t->curTextureUnit], g->neg_bitid);
    }
}

void STATE_APIENTRY crStateBindTexture(GLenum target, GLuint texture) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);

    FLUSH();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glBindTexture called in Begin/End");
        return;
    }

    /* Special Case name = 0 */
    if (!texture) 
    {
        switch (target) 
        {
            case GL_TEXTURE_1D:
                t->unit[t->curTextureUnit].currentTexture1D = &(t->base1D);
                break;
            case GL_TEXTURE_2D:
                t->unit[t->curTextureUnit].currentTexture2D = &(t->base2D);
                break;
#ifdef CR_OPENGL_VERSION_1_2
            case GL_TEXTURE_3D:
                t->unit[t->curTextureUnit].currentTexture3D = &(t->base3D);
                break;
#endif
#ifdef CR_ARB_texture_cube_map
            case GL_TEXTURE_CUBE_MAP_ARB:
                if (!g->extensions.ARB_texture_cube_map) {
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "Invalid target passed to glBindTexture: %d", target);
                    return;
                }
                t->unit[t->curTextureUnit].currentTextureCubeMap = &(t->baseCubeMap);
                break;
#endif
#ifdef CR_NV_texture_rectangle
            case GL_TEXTURE_RECTANGLE_NV:
                if (!g->extensions.NV_texture_rectangle) {
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "Invalid target passed to glBindTexture: %d", target);
                    return;
                }
                t->unit[t->curTextureUnit].currentTextureRect = &(t->baseRect);
                break;
#endif
            default:
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid target passed to glBindTexture: %d", target);
                return;
        }

        DIRTY(tb->dirty, g->neg_bitid);
        DIRTY(tb->current[t->curTextureUnit], g->neg_bitid);
        return;
    }

    /* texture != 0 */
    /* Get the texture */
    GET_TOBJ(tobj, g, texture);
    if (!tobj)
    {
        Assert(crHashtableIsKeyUsed(g->shared->textureTable, texture));
        tobj = crStateTextureAllocate_t(g, texture);
    }

    CR_STATE_SHAREDOBJ_USAGE_SET(tobj, g);

    /* Check the targets */
    if (tobj->target == GL_NONE)
    {
        /* Target isn't set so set it now.*/
        tobj->target = target;
    }
    else if ((tobj->target != target) 
             && !((target==GL_TEXTURE_RECTANGLE_NV && tobj->target==GL_TEXTURE_2D)
                  ||(target==GL_TEXTURE_2D && tobj->target==GL_TEXTURE_RECTANGLE_NV)))
    {
        crWarning( "You called glBindTexture with a target of 0x%x, but the texture you wanted was target 0x%x [1D: %x 2D: %x 3D: %x cube: %x]", (int) target, (int) tobj->target, GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP );
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "Attempt to bind a texture of different dimensions");
        return;
    }

    /* Set the current texture */
    switch (target) 
    {
        case GL_TEXTURE_1D:
            t->unit[t->curTextureUnit].currentTexture1D = tobj;
            break;
        case GL_TEXTURE_2D:
            t->unit[t->curTextureUnit].currentTexture2D = tobj;
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_3D:
            t->unit[t->curTextureUnit].currentTexture3D = tobj;
            break;
#endif
#ifdef CR_ARB_texture_cube_map
        case GL_TEXTURE_CUBE_MAP_ARB:
            t->unit[t->curTextureUnit].currentTextureCubeMap = tobj;
            break;
#endif
#ifdef CR_NV_texture_rectangle
        case GL_TEXTURE_RECTANGLE_NV:
            t->unit[t->curTextureUnit].currentTextureRect = tobj;
            break;
#endif
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "Invalid target passed to glBindTexture: %d", target);
            return;
    }

    DIRTY(tb->dirty, g->neg_bitid);
    DIRTY(tb->current[t->curTextureUnit], g->neg_bitid);
}


void STATE_APIENTRY
crStateTexParameterfv(GLenum target, GLenum pname, const GLfloat *param) 
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj = NULL;
    CRTextureLevel *tl = NULL;
    GLenum e = (GLenum) *param;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    unsigned int i;

    FLUSH();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "TexParameterfv called in Begin/End");
        return;
    }

    crStateGetTextureObjectAndImage(g, target, 0, &tobj, &tl);
    if (!tobj) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "TexParamterfv(invalid target=0x%x)", target);
        return;
    }

    switch (pname) 
    {
        case GL_TEXTURE_MIN_FILTER:
            if (e != GL_NEAREST &&
                    e != GL_LINEAR &&
                    e != GL_NEAREST_MIPMAP_NEAREST &&
                    e != GL_LINEAR_MIPMAP_NEAREST &&
                    e != GL_NEAREST_MIPMAP_LINEAR &&
                    e != GL_LINEAR_MIPMAP_LINEAR)
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "TexParamterfv: GL_TEXTURE_MIN_FILTER invalid param: %d", e);
                return;
            }
            tobj->minFilter = e;
            break;
        case GL_TEXTURE_MAG_FILTER:
            if (e != GL_NEAREST && e != GL_LINEAR)
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "TexParamterfv: GL_TEXTURE_MAG_FILTER invalid param: %d", e);
                return;
            }
            tobj->magFilter = e;
            break;
        case GL_TEXTURE_WRAP_S:
            if (e == GL_CLAMP || e == GL_REPEAT) {
                tobj->wrapS = e;
            }
#ifdef CR_OPENGL_VERSION_1_2
            else if (e == GL_CLAMP_TO_EDGE) {
                tobj->wrapS = e;
            }
#endif
#ifdef GL_CLAMP_TO_EDGE_EXT
            else if (e == GL_CLAMP_TO_EDGE_EXT && g->extensions.EXT_texture_edge_clamp) {
                tobj->wrapS = e;
            }
#endif
#ifdef CR_ARB_texture_border_clamp
            else if (e == GL_CLAMP_TO_BORDER_ARB && g->extensions.ARB_texture_border_clamp) {
                tobj->wrapS = e;
            }
#endif
#ifdef CR_ARB_texture_mirrored_repeat
            else if (e == GL_MIRRORED_REPEAT_ARB && g->extensions.ARB_texture_mirrored_repeat) {
                tobj->wrapS = e;
            }
#endif
#ifdef CR_ATI_texture_mirror_once
            else if ((e == GL_MIRROR_CLAMP_ATI || e == GL_MIRROR_CLAMP_TO_EDGE_ATI) && g->extensions.ATI_texture_mirror_once) {
                tobj->wrapS = e;
            }
#endif
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "TexParameterfv: GL_TEXTURE_WRAP_S invalid param: 0x%x", e);
                return;
            }
            break;
        case GL_TEXTURE_WRAP_T:
            if (e == GL_CLAMP || e == GL_REPEAT) {
                tobj->wrapT = e;
            }
#ifdef CR_OPENGL_VERSION_1_2
            else if (e == GL_CLAMP_TO_EDGE) {
                tobj->wrapT = e;
            }
#endif
#ifdef GL_CLAMP_TO_EDGE_EXT
            else if (e == GL_CLAMP_TO_EDGE_EXT && g->extensions.EXT_texture_edge_clamp) {
                tobj->wrapT = e;
            }
#endif
#ifdef CR_ARB_texture_border_clamp
            else if (e == GL_CLAMP_TO_BORDER_ARB && g->extensions.ARB_texture_border_clamp) {
                tobj->wrapT = e;
            }
#endif
#ifdef CR_ARB_texture_mirrored_repeat
            else if (e == GL_MIRRORED_REPEAT_ARB && g->extensions.ARB_texture_mirrored_repeat) {
                tobj->wrapT = e;
            }
#endif
#ifdef CR_ATI_texture_mirror_once
            else if ((e == GL_MIRROR_CLAMP_ATI || e == GL_MIRROR_CLAMP_TO_EDGE_ATI) && g->extensions.ATI_texture_mirror_once) {
                tobj->wrapT = e;
            }
#endif
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "TexParameterfv: GL_TEXTURE_WRAP_T invalid param: 0x%x", e);
                return;
            }
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_WRAP_R:
            if (e == GL_CLAMP || e == GL_REPEAT) {
                tobj->wrapR = e;
            }
            else if (e == GL_CLAMP_TO_EDGE) {
                tobj->wrapR = e;
            }
#ifdef GL_CLAMP_TO_EDGE_EXT
            else if (e == GL_CLAMP_TO_EDGE_EXT && g->extensions.EXT_texture_edge_clamp) {
                tobj->wrapR = e;
            }
#endif
#ifdef CR_ARB_texture_border_clamp
            else if (e == GL_CLAMP_TO_BORDER_ARB && g->extensions.ARB_texture_border_clamp) {
                tobj->wrapR = e;
            }
#endif
#ifdef CR_ARB_texture_mirrored_repeat
            else if (e == GL_MIRRORED_REPEAT_ARB && g->extensions.ARB_texture_mirrored_repeat) {
                tobj->wrapR = e;
            }
#endif
#ifdef CR_ATI_texture_mirror_once
            else if ((e == GL_MIRROR_CLAMP_ATI || e == GL_MIRROR_CLAMP_TO_EDGE_ATI) && g->extensions.ATI_texture_mirror_once) {
                tobj->wrapR = e;
            }
#endif
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "TexParameterfv: GL_TEXTURE_WRAP_R invalid param: 0x%x", e);
                return;
            }
            break;
        case GL_TEXTURE_PRIORITY:
            tobj->priority = param[0];
            break;
        case GL_TEXTURE_MIN_LOD:
            tobj->minLod = param[0];
            break;
        case GL_TEXTURE_MAX_LOD:
            tobj->maxLod = param[0];
            break;
        case GL_TEXTURE_BASE_LEVEL:
            if (e < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "TexParameterfv: GL_TEXTURE_BASE_LEVEL invalid param: 0x%x", e);
                return;
            }
            tobj->baseLevel = e;
            break;
        case GL_TEXTURE_MAX_LEVEL:
            if (e < 0.0f) 
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "TexParameterfv: GL_TEXTURE_MAX_LEVEL invalid param: 0x%x", e);
                return;
            }
            tobj->maxLevel = e;
            break;
#endif
        case GL_TEXTURE_BORDER_COLOR:
            tobj->borderColor.r = param[0];
            tobj->borderColor.g = param[1];
            tobj->borderColor.b = param[2];
            tobj->borderColor.a = param[3];
            break;
#ifdef CR_EXT_texture_filter_anisotropic
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (g->extensions.EXT_texture_filter_anisotropic) {
                if (param[0] < 1.0f)
                {
                    crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                        "TexParameterfv: GL_TEXTURE_MAX_ANISOTROPY_EXT called with parameter less than 1: %f", param[0]);
                    return;
                }
                tobj->maxAnisotropy = param[0];
                if (tobj->maxAnisotropy > g->limits.maxTextureAnisotropy)
                {
                    tobj->maxAnisotropy = g->limits.maxTextureAnisotropy;
                }
            }
            break;
#endif
#ifdef CR_ARB_depth_texture
        case GL_DEPTH_TEXTURE_MODE_ARB:
            if (g->extensions.ARB_depth_texture) {
                if (param[0] == GL_LUMINANCE || 
                    param[0] == GL_INTENSITY ||
                    param[0] == GL_ALPHA) {
                    tobj->depthMode = (GLenum) param[0];
                }
                else
                {
                    crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                        "TexParameterfv: GL_DEPTH_TEXTURE_MODE_ARB called with invalid parameter: 0x%x", param[0]);
                    return;
                }
            }
            break;
#endif
#ifdef CR_ARB_shadow
        case GL_TEXTURE_COMPARE_MODE_ARB:
            if (g->extensions.ARB_shadow) {
                if (param[0] == GL_NONE ||
                    param[0] == GL_COMPARE_R_TO_TEXTURE_ARB) {
                    tobj->compareMode = (GLenum) param[0];
                }
                else
                {
                    crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                        "TexParameterfv: GL_TEXTURE_COMPARE_MODE_ARB called with invalid parameter: 0x%x", param[0]);
                    return;
                }
            }
            break;
        case GL_TEXTURE_COMPARE_FUNC_ARB:
            if (g->extensions.ARB_shadow) {
                if (param[0] == GL_LEQUAL ||
                    param[0] == GL_GEQUAL) {
                    tobj->compareFunc = (GLenum) param[0];
                } 
            }
#ifdef CR_EXT_shadow_funcs
            else if (g->extensions.EXT_shadow_funcs) {
                if (param[0] == GL_LEQUAL ||
                    param[0] == GL_GEQUAL ||
                    param[0] == GL_LESS ||
                    param[0] == GL_GREATER ||
                    param[0] == GL_ALWAYS ||
                    param[0] == GL_NEVER ) {
                    tobj->compareFunc = (GLenum) param[0];
                } 
            }
#endif
            else {
                    crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                        "TexParameterfv: GL_TEXTURE_COMPARE_FUNC_ARB called with invalid parameter: 0x%x", param[0]);
                    return;
            }
            break;
#endif
#ifdef CR_ARB_shadow_ambient
        case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
            if (g->extensions.ARB_shadow_ambient) {
                tobj->compareFailValue = param[0];
            }
            break;
#endif
#ifdef CR_SGIS_generate_mipmap
        case GL_GENERATE_MIPMAP_SGIS:
            if (g->extensions.SGIS_generate_mipmap) {
                tobj->generateMipmap = param[0] ? GL_TRUE : GL_FALSE;
            }
            break;
#endif
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "TexParamterfv: Invalid pname: %d", pname);
            return;
    }

    DIRTY(tobj->dirty, g->neg_bitid);
    for (i = 0; i < g->limits.maxTextureUnits; i++)
    {
        DIRTY(tobj->paramsBit[i], g->neg_bitid);
    }
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateTexParameteriv(GLenum target, GLenum pname, const GLint *param) 
{
    GLfloat f_param;
    GLcolor f_color;
    switch (pname) 
    {
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_WRAP_R:
        case GL_TEXTURE_PRIORITY:
        case GL_TEXTURE_MIN_LOD:
        case GL_TEXTURE_MAX_LOD:
        case GL_TEXTURE_BASE_LEVEL:
        case GL_TEXTURE_MAX_LEVEL:
#endif
#ifdef CR_EXT_texture_filter_anisotropic
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
#endif
#ifdef CR_ARB_depth_texture
        case GL_DEPTH_TEXTURE_MODE_ARB:
#endif
#ifdef CR_ARB_shadow
        case GL_TEXTURE_COMPARE_MODE_ARB:
        case GL_TEXTURE_COMPARE_FUNC_ARB:
#endif
#ifdef CR_ARB_shadow_ambinet
        case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
#endif
#ifdef CR_SGIS_generate_mipmap
        case GL_GENERATE_MIPMAP_SGIS:
#endif
            f_param = (GLfloat) (*param);
            crStateTexParameterfv( target, pname, &(f_param) );
            break;
        case GL_TEXTURE_BORDER_COLOR:
            f_color.r = ((GLfloat) param[0])/CR_MAXINT;
            f_color.g = ((GLfloat) param[1])/CR_MAXINT;
            f_color.b = ((GLfloat) param[2])/CR_MAXINT;
            f_color.a = ((GLfloat) param[3])/CR_MAXINT;
            crStateTexParameterfv( target, pname, (const GLfloat *) &(f_color) );
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "TexParamteriv: Invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateTexParameterf(GLenum target, GLenum pname, GLfloat param) 
{
    crStateTexParameterfv( target, pname, &param );
}


void STATE_APIENTRY
crStateTexParameteri(GLenum target, GLenum pname, GLint param) {
    GLfloat f_param = (GLfloat) param;
    crStateTexParameterfv( target, pname, &f_param );
}


void STATE_APIENTRY
crStateTexEnvfv(GLenum target, GLenum pname, const GLfloat *param) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    GLenum e;
    GLcolorf c;
    GLuint stage = 0;

    (void) stage;

    FLUSH();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glTexEnvfv called in begin/end");
        return;
    }

#if CR_EXT_texture_lod_bias
    if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
        if (!g->extensions.EXT_texture_lod_bias || pname != GL_TEXTURE_LOD_BIAS_EXT) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnv");
        }
        else {
            t->unit[t->curTextureUnit].lodBias = *param;
            DIRTY(tb->envBit[t->curTextureUnit], g->neg_bitid);
            DIRTY(tb->dirty, g->neg_bitid);
        }
        return;
    }
    else
#endif
#if CR_ARB_point_sprite
    if (target == GL_POINT_SPRITE_ARB) {
        if (!g->extensions.ARB_point_sprite || pname != GL_COORD_REPLACE_ARB) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnv");
        }
        else {
            CRPointBits *pb = &(sb->point);
            g->point.coordReplacement[t->curTextureUnit] = *param ? GL_TRUE : GL_FALSE;
            DIRTY(pb->coordReplacement[t->curTextureUnit], g->neg_bitid);
            DIRTY(pb->dirty, g->neg_bitid);
        }
        return;
    }
    else
#endif
    if (target != GL_TEXTURE_ENV)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glTexEnvfv: target != GL_TEXTURE_ENV: %d", target);
        return;
    }

    switch (pname) 
    {
        case GL_TEXTURE_ENV_MODE:
            e = (GLenum) *param;
            if (e != GL_MODULATE &&
                    e != GL_DECAL &&
                    e != GL_BLEND &&
                    e != GL_ADD &&
                    e != GL_REPLACE &&
                    e != GL_COMBINE_ARB)
            {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "glTexEnvfv: invalid param: %f", *param);
                return;
            }
            t->unit[t->curTextureUnit].envMode = e;
            break;
        case GL_TEXTURE_ENV_COLOR:
            c.r = param[0];
            c.g = param[1];
            c.b = param[2];
            c.a = param[3];
            if (c.r > 1.0f) c.r = 1.0f;
            if (c.g > 1.0f) c.g = 1.0f;
            if (c.b > 1.0f) c.b = 1.0f;
            if (c.a > 1.0f) c.a = 1.0f;
            if (c.r < 0.0f) c.r = 0.0f;
            if (c.g < 0.0f) c.g = 0.0f;
            if (c.b < 0.0f) c.b = 0.0f;
            if (c.a < 0.0f) c.a = 0.0f;
            t->unit[t->curTextureUnit].envColor = c;
            break;

#ifdef CR_ARB_texture_env_combine
        case GL_COMBINE_RGB_ARB:
            e = (GLenum) (GLint) *param;
            if (g->extensions.ARB_texture_env_combine &&
                    (e == GL_REPLACE ||
                     e == GL_MODULATE ||
                     e == GL_ADD ||
                     e == GL_ADD_SIGNED_ARB ||
                     e == GL_INTERPOLATE_ARB ||
                     e == GL_SUBTRACT_ARB)) {
                 t->unit[t->curTextureUnit].combineModeRGB = e;
            }
#ifdef CR_ARB_texture_env_dot3
            else if (g->extensions.ARB_texture_env_dot3 &&
                             (e == GL_DOT3_RGB_ARB ||
                                e == GL_DOT3_RGBA_ARB ||
                                e == GL_DOT3_RGB_EXT ||
                                e == GL_DOT3_RGBA_EXT)) {
                 t->unit[t->curTextureUnit].combineModeRGB = e;
            }
#endif
            else {
                 crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnvfv(param=0x%x", e);
                 return;
            }
            break;
        case GL_COMBINE_ALPHA_EXT:
            e = (GLenum) *param;
            if (g->extensions.ARB_texture_env_combine &&
                    (e == GL_REPLACE ||
                     e == GL_MODULATE ||
                     e == GL_ADD ||
                     e == GL_ADD_SIGNED_ARB ||
                     e == GL_INTERPOLATE_ARB ||
                     e == GL_SUBTRACT_ARB)) {
                 t->unit[t->curTextureUnit].combineModeA = e;
            }
            else {
                 crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnvfv");
                 return;
            }
            break;
        case GL_SOURCE0_RGB_ARB:
        case GL_SOURCE1_RGB_ARB:
        case GL_SOURCE2_RGB_ARB:
            e = (GLenum) *param;
            stage = pname - GL_SOURCE0_RGB_ARB;
            if (g->extensions.ARB_texture_env_combine &&
                    (e == GL_TEXTURE ||
                     e == GL_CONSTANT_ARB ||
                     e == GL_PRIMARY_COLOR_ARB ||
                     e == GL_PREVIOUS_ARB)) {
                t->unit[t->curTextureUnit].combineSourceRGB[stage] = e;
            }
            else if (g->extensions.ARB_texture_env_crossbar &&
                             e >= GL_TEXTURE0_ARB &&
                             e < GL_TEXTURE0_ARB + g->limits.maxTextureUnits) {
                t->unit[t->curTextureUnit].combineSourceRGB[stage] = e;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnvfv");
                return;
            }
            break;
        case GL_SOURCE0_ALPHA_ARB:
        case GL_SOURCE1_ALPHA_ARB:
        case GL_SOURCE2_ALPHA_ARB:
            e = (GLenum) *param;
            stage = pname - GL_SOURCE0_ALPHA_ARB;
            if (g->extensions.ARB_texture_env_combine &&
                    (e == GL_TEXTURE ||
                     e == GL_CONSTANT_ARB ||
                     e == GL_PRIMARY_COLOR_ARB ||
                     e == GL_PREVIOUS_ARB)) {
                t->unit[t->curTextureUnit].combineSourceA[stage] = e;
            }
            else if (g->extensions.ARB_texture_env_crossbar &&
                             e >= GL_TEXTURE0_ARB &&
                             e < GL_TEXTURE0_ARB + g->limits.maxTextureUnits) {
                t->unit[t->curTextureUnit].combineSourceA[stage] = e;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnvfv");
                return;
            }
            break;
        case GL_OPERAND0_RGB_ARB:
        case GL_OPERAND1_RGB_ARB:
        case GL_OPERAND2_RGB_ARB:
            e = (GLenum) *param;
            stage = pname - GL_OPERAND0_RGB_ARB;
            if (g->extensions.ARB_texture_env_combine &&
                    (e == GL_SRC_COLOR ||
                     e == GL_ONE_MINUS_SRC_COLOR ||
                     e == GL_SRC_ALPHA ||
                     e == GL_ONE_MINUS_SRC_ALPHA)) {
                t->unit[t->curTextureUnit].combineOperandRGB[stage] = e;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnvfv");
                return;
            }
            break;
        case GL_OPERAND0_ALPHA_ARB:
        case GL_OPERAND1_ALPHA_ARB:
        case GL_OPERAND2_ALPHA_ARB:
            e = (GLenum) *param;
            stage = pname - GL_OPERAND0_ALPHA_ARB;
            if (g->extensions.ARB_texture_env_combine &&
                    (e == GL_SRC_ALPHA ||
                     e == GL_ONE_MINUS_SRC_ALPHA)) {
                t->unit[t->curTextureUnit].combineOperandA[stage] = e;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glTexEnvfv(param=0x%x)", e);
                return;
            }
            break;
        case GL_RGB_SCALE_ARB:
            if (g->extensions.ARB_texture_env_combine &&
                    (*param == 1.0 ||
                     *param == 2.0 ||
                     *param == 4.0)) {
                t->unit[t->curTextureUnit].combineScaleRGB = *param;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glTexEnvfv");
                return;
            }
            break;
        case GL_ALPHA_SCALE:
            if (g->extensions.ARB_texture_env_combine &&
                    (*param == 1.0 ||
                     *param == 2.0 ||
                     *param == 4.0)) {
                t->unit[t->curTextureUnit].combineScaleA = *param;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glTexEnvfv");
                return;
            }
        break;
#endif /* CR_ARB_texture_env_combine */

        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glTexEnvfv: invalid pname: %d", pname);
            return;
    }

    DIRTY(tb->envBit[t->curTextureUnit], g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateTexEnviv(GLenum target, GLenum pname, const GLint *param) 
{
    GLfloat f_param;
    GLcolor f_color;

    switch (pname) {
        case GL_TEXTURE_ENV_MODE:
            f_param = (GLfloat) (*param);
            crStateTexEnvfv( target, pname, &f_param );
            break;
        case GL_TEXTURE_ENV_COLOR:
            f_color.r = ((GLfloat) param[0]) / CR_MAXINT;
            f_color.g = ((GLfloat) param[1]) / CR_MAXINT;
            f_color.b = ((GLfloat) param[2]) / CR_MAXINT;
            f_color.a = ((GLfloat) param[3]) / CR_MAXINT;
            crStateTexEnvfv( target, pname, (const GLfloat *) &f_color );
            break;
#ifdef CR_ARB_texture_env_combine
        case GL_COMBINE_RGB_ARB:
        case GL_COMBINE_ALPHA_EXT:
        case GL_SOURCE0_RGB_ARB:
        case GL_SOURCE1_RGB_ARB:
        case GL_SOURCE2_RGB_ARB:
        case GL_SOURCE0_ALPHA_ARB:
        case GL_SOURCE1_ALPHA_ARB:
        case GL_SOURCE2_ALPHA_ARB:
        case GL_OPERAND0_RGB_ARB:
        case GL_OPERAND1_RGB_ARB:
        case GL_OPERAND2_RGB_ARB:
        case GL_OPERAND0_ALPHA_ARB:
        case GL_OPERAND1_ALPHA_ARB:
        case GL_OPERAND2_ALPHA_ARB:
        case GL_RGB_SCALE_ARB:
        case GL_ALPHA_SCALE:
            f_param = (GLfloat) (*param);
            crStateTexEnvfv( target, pname, &f_param );
            break;
#endif
#ifdef CR_EXT_texture_lod_bias
        case GL_TEXTURE_LOD_BIAS_EXT:
            f_param = (GLfloat) (*param);
            crStateTexEnvfv( target, pname, &f_param);
            break;
#endif
#ifdef CR_ARB_point_sprite
        case GL_COORD_REPLACE_ARB:
            f_param = (GLfloat) *param;
            crStateTexEnvfv( target, pname, &f_param);
            break;
#endif

        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glTexEnvfv: invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateTexEnvf(GLenum target, GLenum pname, GLfloat param) 
{
    crStateTexEnvfv( target, pname, &param );
}


void STATE_APIENTRY
crStateTexEnvi(GLenum target, GLenum pname, GLint param) 
{
    GLfloat f_param = (GLfloat) param;
    crStateTexEnvfv( target, pname, &f_param );
}


void STATE_APIENTRY
crStateGetTexEnvfv(GLenum target, GLenum pname, GLfloat *param) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__,GL_INVALID_OPERATION,
                    "glGetTexEnvfv called in begin/end");
        return;
    }

#if CR_EXT_texture_lod_bias
    if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
        if (!g->extensions.EXT_texture_lod_bias || pname != GL_TEXTURE_LOD_BIAS_EXT) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnv");
        }
        else {
            *param = t->unit[t->curTextureUnit].lodBias;
        }
        return;
    }
    else
#endif
#if CR_ARB_point_sprite
    if (target == GL_POINT_SPRITE_ARB) {
        if (!g->extensions.ARB_point_sprite || pname != GL_COORD_REPLACE_ARB) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnv");
        }
        else {
            *param = (GLfloat) g->point.coordReplacement[t->curTextureUnit];
        }
        return;
    }
    else
#endif
    if (target != GL_TEXTURE_ENV)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexEnvfv: target != GL_TEXTURE_ENV: %d", target);
        return;
    }

    switch (pname) {
        case GL_TEXTURE_ENV_MODE:
            *param = (GLfloat) t->unit[t->curTextureUnit].envMode;
            break;
        case GL_TEXTURE_ENV_COLOR:
            param[0] = t->unit[t->curTextureUnit].envColor.r;
            param[1] = t->unit[t->curTextureUnit].envColor.g;
            param[2] = t->unit[t->curTextureUnit].envColor.b;
            param[3] = t->unit[t->curTextureUnit].envColor.a;
            break;
        case GL_COMBINE_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineModeRGB;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_COMBINE_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineModeA;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_SOURCE0_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineSourceRGB[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_SOURCE1_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineSourceRGB[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_SOURCE2_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineSourceRGB[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_SOURCE0_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineSourceA[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_SOURCE1_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineSourceA[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_SOURCE2_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineSourceA[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_OPERAND0_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineOperandRGB[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_OPERAND1_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineOperandRGB[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_OPERAND2_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineOperandRGB[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_OPERAND0_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineOperandA[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_OPERAND1_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineOperandA[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_OPERAND2_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLfloat) t->unit[t->curTextureUnit].combineOperandA[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_RGB_SCALE_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = t->unit[t->curTextureUnit].combineScaleRGB;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        case GL_ALPHA_SCALE:
            if (g->extensions.ARB_texture_env_combine) {
                *param = t->unit[t->curTextureUnit].combineScaleA;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glGetTexEnvfv: invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexEnviv(GLenum target, GLenum pname, GLint *param) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__,GL_INVALID_OPERATION,
                    "glGetTexEnviv called in begin/end");
        return;
    }

#if CR_EXT_texture_lod_bias
    if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
        if (!g->extensions.EXT_texture_lod_bias || pname != GL_TEXTURE_LOD_BIAS_EXT) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnv");
        }
        else {
            *param = (GLint) t->unit[t->curTextureUnit].lodBias;
        }
        return;
    }
    else
#endif
#if CR_ARB_point_sprite
    if (target == GL_POINT_SPRITE_ARB) {
        if (!g->extensions.ARB_point_sprite || pname != GL_COORD_REPLACE_ARB) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnv");
        }
        else {
            *param = (GLint) g->point.coordReplacement[t->curTextureUnit];
        }
        return;
    }
    else
#endif
    if (target != GL_TEXTURE_ENV)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexEnviv: target != GL_TEXTURE_ENV: %d", target);
        return;
    }

    switch (pname) {
        case GL_TEXTURE_ENV_MODE:
            *param = (GLint) t->unit[t->curTextureUnit].envMode;
            break;
        case GL_TEXTURE_ENV_COLOR:
            param[0] = (GLint) (t->unit[t->curTextureUnit].envColor.r * CR_MAXINT);
            param[1] = (GLint) (t->unit[t->curTextureUnit].envColor.g * CR_MAXINT);
            param[2] = (GLint) (t->unit[t->curTextureUnit].envColor.b * CR_MAXINT);
            param[3] = (GLint) (t->unit[t->curTextureUnit].envColor.a * CR_MAXINT);
            break;
        case GL_COMBINE_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineModeRGB;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_COMBINE_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineModeA;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_SOURCE0_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineSourceRGB[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_SOURCE1_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineSourceRGB[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_SOURCE2_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineSourceRGB[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_SOURCE0_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineSourceA[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_SOURCE1_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineSourceA[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_SOURCE2_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineSourceA[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_OPERAND0_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineOperandRGB[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_OPERAND1_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineOperandRGB[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_OPERAND2_RGB_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineOperandRGB[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_OPERAND0_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineOperandA[0];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_OPERAND1_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineOperandA[1];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_OPERAND2_ALPHA_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineOperandA[2];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_RGB_SCALE_ARB:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineScaleRGB;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        case GL_ALPHA_SCALE:
            if (g->extensions.ARB_texture_env_combine) {
                *param = (GLint) t->unit[t->curTextureUnit].combineScaleA;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
                return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glGetTexEnviv: invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateTexGendv(GLenum coord, GLenum pname, const GLdouble *param) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTransformState *trans = &(g->transform);
    GLvectorf v;
    GLenum e;
    CRmatrix inv;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);

    FLUSH();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glTexGen called in begin/end");
        return;
    }

    switch (coord) 
    {
        case GL_S:
            switch (pname) 
            {
                case GL_TEXTURE_GEN_MODE:
                    e = (GLenum) *param;
                    if (e == GL_OBJECT_LINEAR ||
                        e == GL_EYE_LINEAR ||
                        e == GL_SPHERE_MAP
#if defined(GL_ARB_texture_cube_map) || defined(GL_EXT_texture_cube_map) || defined(GL_NV_texgen_reflection)
                        || ((e == GL_REFLECTION_MAP_ARB || e == GL_NORMAL_MAP_ARB)
                            && g->extensions.ARB_texture_cube_map)
#endif
                    ) {
                        t->unit[t->curTextureUnit].gen.s = e;
                        DIRTY(tb->genMode[t->curTextureUnit], g->neg_bitid);
                        DIRTY(tb->dirty, g->neg_bitid);
                    }
                    else {
                        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "glTexGendv called with bad param: %lf", *param);
                        return;
                    }
                    break;
                case GL_OBJECT_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    t->unit[t->curTextureUnit].objSCoeff = v;
                    DIRTY(tb->objGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                case GL_EYE_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    crMatrixInvertTranspose(&inv, trans->modelViewStack.top);
                    crStateTransformXformPointMatrixf(&inv, &v);
                    t->unit[t->curTextureUnit].eyeSCoeff = v;
                    DIRTY(tb->eyeGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glTexGendv called with bogus pname: %d", pname);
                    return;
            }
            break;
        case GL_T:
            switch (pname) {
                case GL_TEXTURE_GEN_MODE:
                    e = (GLenum) *param;
                    if (e == GL_OBJECT_LINEAR ||
                        e == GL_EYE_LINEAR ||
                        e == GL_SPHERE_MAP
#if defined(GL_ARB_texture_cube_map) || defined(GL_EXT_texture_cube_map) || defined(GL_NV_texgen_reflection)
                        || ((e == GL_REFLECTION_MAP_ARB || e == GL_NORMAL_MAP_ARB)
                            && g->extensions.ARB_texture_cube_map)
#endif
                    ) {
                        t->unit[t->curTextureUnit].gen.t = e;
                        DIRTY(tb->genMode[t->curTextureUnit], g->neg_bitid);
                        DIRTY(tb->dirty, g->neg_bitid);
                    }
                    else {
                        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "glTexGendv called with bad param: %lf", *param);
                        return;
                    }
                    break;
                case GL_OBJECT_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    t->unit[t->curTextureUnit].objTCoeff = v;
                    DIRTY(tb->objGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                case GL_EYE_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    crMatrixInvertTranspose(&inv, trans->modelViewStack.top);
                    crStateTransformXformPointMatrixf(&inv, &v);
                    t->unit[t->curTextureUnit].eyeTCoeff = v;
                    DIRTY(tb->eyeGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glTexGen called with bogus pname: %d", pname);
                    return;
            }
            break;
        case GL_R:
            switch (pname) {
                case GL_TEXTURE_GEN_MODE:
                    e = (GLenum) *param;
                    if (e == GL_OBJECT_LINEAR ||
                        e == GL_EYE_LINEAR ||
                        e == GL_SPHERE_MAP
#if defined(GL_ARB_texture_cube_map) || defined(GL_EXT_texture_cube_map) || defined(GL_NV_texgen_reflection)
                        || ((e == GL_REFLECTION_MAP_ARB || e == GL_NORMAL_MAP_ARB)
                            && g->extensions.ARB_texture_cube_map)
#endif
                    ) {
                        t->unit[t->curTextureUnit].gen.r = e;
                        DIRTY(tb->genMode[t->curTextureUnit], g->neg_bitid);
                        DIRTY(tb->dirty, g->neg_bitid);
                    }
                    else {
                        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "glTexGen called with bad param: %lf", *param);
                        return;
                    }
                    break;
                case GL_OBJECT_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    t->unit[t->curTextureUnit].objRCoeff = v;
                    DIRTY(tb->objGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                case GL_EYE_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    crMatrixInvertTranspose(&inv, trans->modelViewStack.top);
                    crStateTransformXformPointMatrixf(&inv, &v);
                    t->unit[t->curTextureUnit].eyeRCoeff = v;
                    DIRTY(tb->eyeGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glTexGen called with bogus pname: %d", pname);
                    return;
            }
            break;
        case GL_Q:
            switch (pname) {
                case GL_TEXTURE_GEN_MODE:
                    e = (GLenum) *param;
                    if (e == GL_OBJECT_LINEAR ||
                        e == GL_EYE_LINEAR ||
                        e == GL_SPHERE_MAP
#if defined(GL_ARB_texture_cube_map) || defined(GL_EXT_texture_cube_map) || defined(GL_NV_texgen_reflection)
                        || ((e == GL_REFLECTION_MAP_ARB || e == GL_NORMAL_MAP_ARB)
                            && g->extensions.ARB_texture_cube_map)
#endif
                    ) {
                        t->unit[t->curTextureUnit].gen.q = e;
                        DIRTY(tb->genMode[t->curTextureUnit], g->neg_bitid);
                        DIRTY(tb->dirty, g->neg_bitid);
                    }
                    else {
                        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                            "glTexGen called with bad param: %lf", *param);
                        return;
                    }
                    break;
                case GL_OBJECT_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    t->unit[t->curTextureUnit].objQCoeff = v;
                    DIRTY(tb->objGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                case GL_EYE_PLANE:
                    v.x = (GLfloat) param[0];
                    v.y = (GLfloat) param[1];
                    v.z = (GLfloat) param[2];
                    v.w = (GLfloat) param[3];
                    crMatrixInvertTranspose(&inv, trans->modelViewStack.top);
                    crStateTransformXformPointMatrixf(&inv, &v);
                    t->unit[t->curTextureUnit].eyeQCoeff = v;
                    DIRTY(tb->eyeGen[t->curTextureUnit], g->neg_bitid);
                    DIRTY(tb->dirty, g->neg_bitid);
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glTexGen called with bogus pname: %d", pname);
                    return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glTexGen called with bogus coord: %d", coord);
            return;
    }
}


void STATE_APIENTRY
crStateTexGenfv(GLenum coord, GLenum pname, const GLfloat *param) 
{
    GLdouble d_param;
    GLvectord d_vector;
    switch (pname) 
    {
        case GL_TEXTURE_GEN_MODE:
            d_param = (GLdouble) *param;
            crStateTexGendv( coord, pname, &d_param );
            break;
        case GL_OBJECT_PLANE:
        case GL_EYE_PLANE:
            d_vector.x = (GLdouble) param[0];
            d_vector.y = (GLdouble) param[1];
            d_vector.z = (GLdouble) param[2];
            d_vector.w = (GLdouble) param[3];
            crStateTexGendv( coord, pname, (const double *) &d_vector );
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glTexGen called with bogus pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateTexGeniv(GLenum coord, GLenum pname, const GLint *param) 
{
    GLdouble d_param;
    GLvectord d_vector;
    switch (pname) 
    {
        case GL_TEXTURE_GEN_MODE:
            d_param = (GLdouble) *param;
            crStateTexGendv( coord, pname, &d_param );
            break;
        case GL_OBJECT_PLANE:
        case GL_EYE_PLANE:
            d_vector.x = (GLdouble) param[0];
            d_vector.y = (GLdouble) param[1];
            d_vector.z = (GLdouble) param[2];
            d_vector.w = (GLdouble) param[3];
            crStateTexGendv( coord, pname, (const double *) &d_vector );
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glTexGen called with bogus pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateTexGend (GLenum coord, GLenum pname, GLdouble param) 
{
    crStateTexGendv( coord, pname, &param );
}


void STATE_APIENTRY
crStateTexGenf(GLenum coord, GLenum pname, GLfloat param) 
{
    GLdouble d_param = (GLdouble) param;
    crStateTexGendv( coord, pname, &d_param );
}


void STATE_APIENTRY
crStateTexGeni(GLenum coord, GLenum pname, GLint param) 
{
    GLdouble d_param = (GLdouble) param;
    crStateTexGendv( coord, pname, &d_param );
}


void STATE_APIENTRY
crStateGetTexGendv(GLenum coord, GLenum pname, GLdouble *param) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glGetTexGen called in begin/end");
        return;
    }

    switch (pname) {
        case GL_TEXTURE_GEN_MODE:
            switch (coord) {
                case GL_S:
                    *param = (GLdouble) t->unit[t->curTextureUnit].gen.s;
                    break;
                case GL_T:
                    *param = (GLdouble) t->unit[t->curTextureUnit].gen.t;
                    break;
                case GL_R:
                    *param = (GLdouble) t->unit[t->curTextureUnit].gen.r;
                    break;
                case GL_Q:
                    *param = (GLdouble) t->unit[t->curTextureUnit].gen.q;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGen called with bogus coord: %d", coord);
                    return;
            }
            break;
        case GL_OBJECT_PLANE:
            switch (coord) {
                case GL_S:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].objSCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].objSCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].objSCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].objSCoeff.w;
                    break;
                case GL_T:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].objTCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].objTCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].objTCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].objTCoeff.w;
                    break;
                case GL_R:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].objRCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].objRCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].objRCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].objRCoeff.w;
                    break;
                case GL_Q:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].objQCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].objQCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].objQCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].objQCoeff.w;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGen called with bogus coord: %d", coord);
                    return;
            }
            break;
        case GL_EYE_PLANE:
            switch (coord) {
                case GL_S:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].eyeSCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].eyeSCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].eyeSCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].eyeSCoeff.w;
                    break;
                case GL_T:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].eyeTCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].eyeTCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].eyeTCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].eyeTCoeff.w;
                    break;
                case GL_R:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].eyeRCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].eyeRCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].eyeRCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].eyeRCoeff.w;
                    break;
                case GL_Q:
                    param[0] = (GLdouble) t->unit[t->curTextureUnit].eyeQCoeff.x;
                    param[1] = (GLdouble) t->unit[t->curTextureUnit].eyeQCoeff.y;
                    param[2] = (GLdouble) t->unit[t->curTextureUnit].eyeQCoeff.z;
                    param[3] = (GLdouble) t->unit[t->curTextureUnit].eyeQCoeff.w;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGen called with bogus coord: %d", coord);
                    return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glGetTexGen called with bogus pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexGenfv(GLenum coord, GLenum pname, GLfloat *param)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glGetTexGenfv called in begin/end");
        return;
    }

    switch (pname) {
        case GL_TEXTURE_GEN_MODE:
            switch (coord) {
                case GL_S:
                    *param = (GLfloat) t->unit[t->curTextureUnit].gen.s;
                    break;
                case GL_T:
                    *param = (GLfloat) t->unit[t->curTextureUnit].gen.t;
                    break;
                case GL_R:
                    *param = (GLfloat) t->unit[t->curTextureUnit].gen.r;
                    break;
                case GL_Q:
                    *param = (GLfloat) t->unit[t->curTextureUnit].gen.q;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGenfv called with bogus coord: %d", coord);
                    return;
            }
            break;
        case GL_OBJECT_PLANE:
            switch (coord) {
                case GL_S:
                    param[0] = t->unit[t->curTextureUnit].objSCoeff.x;
                    param[1] = t->unit[t->curTextureUnit].objSCoeff.y;
                    param[2] = t->unit[t->curTextureUnit].objSCoeff.z;
                    param[3] = t->unit[t->curTextureUnit].objSCoeff.w;
                    break;
                case GL_T:
                    param[0] =  t->unit[t->curTextureUnit].objTCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].objTCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].objTCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].objTCoeff.w;
                    break;
                case GL_R:
                    param[0] =  t->unit[t->curTextureUnit].objRCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].objRCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].objRCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].objRCoeff.w;
                    break;
                case GL_Q:
                    param[0] =  t->unit[t->curTextureUnit].objQCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].objQCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].objQCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].objQCoeff.w;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGenfv called with bogus coord: %d", coord);
                    return;
            }
            break;
        case GL_EYE_PLANE:
            switch (coord) {
                case GL_S:
                    param[0] =  t->unit[t->curTextureUnit].eyeSCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].eyeSCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].eyeSCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].eyeSCoeff.w;
                    break;
                case GL_T:
                    param[0] =  t->unit[t->curTextureUnit].eyeTCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].eyeTCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].eyeTCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].eyeTCoeff.w;
                    break;
                case GL_R:
                    param[0] =  t->unit[t->curTextureUnit].eyeRCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].eyeRCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].eyeRCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].eyeRCoeff.w;
                    break;
                case GL_Q:
                    param[0] =  t->unit[t->curTextureUnit].eyeQCoeff.x;
                    param[1] =  t->unit[t->curTextureUnit].eyeQCoeff.y;
                    param[2] =  t->unit[t->curTextureUnit].eyeQCoeff.z;
                    param[3] =  t->unit[t->curTextureUnit].eyeQCoeff.w;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGenfv called with bogus coord: %d", coord);
                    return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glGetTexGenfv called with bogus pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexGeniv(GLenum coord, GLenum pname, GLint *param)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glGetTexGeniv called in begin/end");
        return;
    }

    switch (pname) {
        case GL_TEXTURE_GEN_MODE:
            switch (coord) {
                case GL_S:
                    *param = (GLint) t->unit[t->curTextureUnit].gen.s;
                    break;
                case GL_T:
                    *param = (GLint) t->unit[t->curTextureUnit].gen.t;
                    break;
                case GL_R:
                    *param = (GLint) t->unit[t->curTextureUnit].gen.r;
                    break;
                case GL_Q:
                    *param = (GLint) t->unit[t->curTextureUnit].gen.q;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGeniv called with bogus coord: %d", coord);
                    return;
            }
            break;
        case GL_OBJECT_PLANE:
            switch (coord) {
                case GL_S:
                    param[0] = (GLint) t->unit[t->curTextureUnit].objSCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].objSCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].objSCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].objSCoeff.w;
                    break;
                case GL_T:
                    param[0] = (GLint) t->unit[t->curTextureUnit].objTCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].objTCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].objTCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].objTCoeff.w;
                    break;
                case GL_R:
                    param[0] = (GLint) t->unit[t->curTextureUnit].objRCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].objRCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].objRCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].objRCoeff.w;
                    break;
                case GL_Q:
                    param[0] = (GLint) t->unit[t->curTextureUnit].objQCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].objQCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].objQCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].objQCoeff.w;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGeniv called with bogus coord: %d", coord);
                    return;
            }
            break;
        case GL_EYE_PLANE:
            switch (coord) {
                case GL_S:
                    param[0] = (GLint) t->unit[t->curTextureUnit].eyeSCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].eyeSCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].eyeSCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].eyeSCoeff.w;
                    break;
                case GL_T:
                    param[0] = (GLint) t->unit[t->curTextureUnit].eyeTCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].eyeTCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].eyeTCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].eyeTCoeff.w;
                    break;
                case GL_R:
                    param[0] = (GLint) t->unit[t->curTextureUnit].eyeRCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].eyeRCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].eyeRCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].eyeRCoeff.w;
                    break;
                case GL_Q:
                    param[0] = (GLint) t->unit[t->curTextureUnit].eyeQCoeff.x;
                    param[1] = (GLint) t->unit[t->curTextureUnit].eyeQCoeff.y;
                    param[2] = (GLint) t->unit[t->curTextureUnit].eyeQCoeff.z;
                    param[3] = (GLint) t->unit[t->curTextureUnit].eyeQCoeff.w;
                    break;
                default:
                    crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "glGetTexGeniv called with bogus coord: %d", coord);
                    return;
            }
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                        "glGetTexGen called with bogus pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexLevelParameterfv(GLenum target, GLint level, 
                                                            GLenum pname, GLfloat *params) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj;
    CRTextureLevel *timg;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
            "glGetTexLevelParameterfv called in begin/end");
        return;
    }

    if (level < 0 || level > t->maxLevel)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
            "glGetTexLevelParameterfv: Invalid level: %d", level);
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &timg);
    if (!timg)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
            "GetTexLevelParameterfv: invalid target: 0x%x or level %d",
            target, level);
        return;
    }

    switch (pname) 
    {
        case GL_TEXTURE_WIDTH:
            *params = (GLfloat) timg->width;
            break;
        case GL_TEXTURE_HEIGHT:
            *params = (GLfloat) timg->height;
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_DEPTH:
            *params = (GLfloat) timg->depth;
            break;
#endif
        case GL_TEXTURE_INTERNAL_FORMAT:
            *params = (GLfloat) timg->internalFormat;
            break;
        case GL_TEXTURE_BORDER:
            *params = (GLfloat) timg->border;
            break;
        case GL_TEXTURE_RED_SIZE:
            *params = (GLfloat) timg->texFormat->redbits;
            break;
        case GL_TEXTURE_GREEN_SIZE:
            *params = (GLfloat) timg->texFormat->greenbits;
            break;
        case GL_TEXTURE_BLUE_SIZE:
            *params = (GLfloat) timg->texFormat->bluebits;
            break;
        case GL_TEXTURE_ALPHA_SIZE:
            *params = (GLfloat) timg->texFormat->alphabits;
            break;
        case GL_TEXTURE_INTENSITY_SIZE:
            *params = (GLfloat) timg->texFormat->intensitybits;
            break;
        case GL_TEXTURE_LUMINANCE_SIZE:
            *params = (GLfloat) timg->texFormat->luminancebits;
            break;
#if CR_ARB_texture_compression
        case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
             *params = (GLfloat) timg->bytes;
             break;
        case GL_TEXTURE_COMPRESSED_ARB:
             *params = (GLfloat) timg->compressed;
             break;
#endif
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "GetTexLevelParameterfv: invalid pname: 0x%x",
                pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexLevelParameteriv(GLenum target, GLint level, 
                                                            GLenum pname, GLint *params) 
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj;
    CRTextureLevel *timg;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
            "glGetTexLevelParameteriv called in begin/end");
        return;
    }

    if (level < 0 || level > t->maxLevel)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
            "glGetTexLevelParameteriv: Invalid level: %d", level);
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &timg);
    if (!timg)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
            "GetTexLevelParameteriv: invalid target: 0x%x",
            target);
        return;
    }

    switch (pname) 
    {
        case GL_TEXTURE_WIDTH:
            *params = (GLint) timg->width;
            break;
        case GL_TEXTURE_HEIGHT:
            *params = (GLint) timg->height;
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_DEPTH:
            *params = (GLint) timg->depth;
            break;
#endif
        case GL_TEXTURE_INTERNAL_FORMAT:
            *params = (GLint) timg->internalFormat;
            break;
        case GL_TEXTURE_BORDER:
            *params = (GLint) timg->border;
            break;
        case GL_TEXTURE_RED_SIZE:
            *params = (GLint) timg->texFormat->redbits;
            break;
        case GL_TEXTURE_GREEN_SIZE:
            *params = (GLint) timg->texFormat->greenbits;
            break;
        case GL_TEXTURE_BLUE_SIZE:
            *params = (GLint) timg->texFormat->bluebits;
            break;
        case GL_TEXTURE_ALPHA_SIZE:
            *params = (GLint) timg->texFormat->alphabits;
            break;
        case GL_TEXTURE_INTENSITY_SIZE:
            *params = (GLint) timg->texFormat->intensitybits;
            break;
        case GL_TEXTURE_LUMINANCE_SIZE:
            *params = (GLint) timg->texFormat->luminancebits;
            break;

#if 0
        /* XXX TODO */
        case GL_TEXTURE_DEPTH_SIZE:
            *params = (GLint) timg->texFormat->depthSize;
            break;
#endif
#if CR_ARB_texture_compression
        case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
            *params = (GLint) timg->bytes;
            break;
        case GL_TEXTURE_COMPRESSED_ARB:
            *params = (GLint) timg->compressed;
            break;
#endif
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "GetTexLevelParameteriv: invalid pname: 0x%x",
                pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) 
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;
    CRTextureLevel *tl;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glGetTexParameterfv called in begin/end");
        return;
    }

    crStateGetTextureObjectAndImage(g, target, 0, &tobj, &tl);
    if (!tobj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
            "glGetTexParameterfv: invalid target: 0x%x", target);
        return;
    }

    switch (pname) 
    {
        case GL_TEXTURE_MAG_FILTER:
            *params = (GLfloat) tobj->magFilter;
            break;
        case GL_TEXTURE_MIN_FILTER:
            *params = (GLfloat) tobj->minFilter;
            break;
        case GL_TEXTURE_WRAP_S:
            *params = (GLfloat) tobj->wrapS;
            break;
        case GL_TEXTURE_WRAP_T:
            *params = (GLfloat) tobj->wrapT;
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_WRAP_R:
            *params = (GLfloat) tobj->wrapR;
            break;
        case GL_TEXTURE_PRIORITY:
            *params = (GLfloat) tobj->priority;
            break;
#endif
        case GL_TEXTURE_BORDER_COLOR:
            params[0] = tobj->borderColor.r;
            params[1] = tobj->borderColor.g;
            params[2] = tobj->borderColor.b;
            params[3] = tobj->borderColor.a;
            break;
#ifdef CR_EXT_texture_filter_anisotropic
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (g->extensions.EXT_texture_filter_anisotropic) {
                *params = (GLfloat) tobj->maxAnisotropy;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameterfv: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_ARB_depth_texture
        case GL_DEPTH_TEXTURE_MODE_ARB:
            if (g->extensions.ARB_depth_texture) {
                *params = (GLfloat) tobj->depthMode;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_ARB_shadow
        case GL_TEXTURE_COMPARE_MODE_ARB:
            if (g->extensions.ARB_shadow) {
                *params = (GLfloat) tobj->compareMode;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
        case GL_TEXTURE_COMPARE_FUNC_ARB:
            if (g->extensions.ARB_shadow) {
                *params = (GLfloat) tobj->compareFunc;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_ARB_shadow_ambient
        case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
            if (g->extensions.ARB_shadow_ambient) {
                *params = (GLfloat) tobj->compareFailValue;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_SGIS_generate_mipmap
        case GL_GENERATE_MIPMAP_SGIS:
            if (g->extensions.SGIS_generate_mipmap) {
                *params = (GLfloat) tobj->generateMipmap;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_MIN_LOD:
            *params = (GLfloat) tobj->minLod;
            break;
        case GL_TEXTURE_MAX_LOD:
            *params = (GLfloat) tobj->maxLod;
            break;
        case GL_TEXTURE_BASE_LEVEL:
            *params = (GLfloat) tobj->baseLevel;
            break;
        case GL_TEXTURE_MAX_LEVEL:
            *params = (GLfloat) tobj->maxLevel;
            break;
#endif
#if 0
        case GL_TEXTURE_LOD_BIAS_EXT:
            /* XXX todo */
            *params = (GLfloat) tobj->lodBias;
            break;
#endif
            case GL_TEXTURE_RESIDENT:
            /* XXX todo */
            crWarning("glGetTexParameterfv GL_TEXTURE_RESIDENT is unimplemented");
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                "glGetTexParameterfv: invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStateGetTexParameteriv(GLenum target, GLenum pname, GLint *params) 
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;
    CRTextureLevel *tl;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                    "glGetTexParameter called in begin/end");
        return;
    }

    crStateGetTextureObjectAndImage(g, target, 0, &tobj, &tl);
    if (!tobj)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
            "glGetTexParameteriv: invalid target: 0x%x", target);
        return;
    }

    switch (pname) 
    {
        case GL_TEXTURE_MAG_FILTER:
            *params = (GLint) tobj->magFilter;
            break;
        case GL_TEXTURE_MIN_FILTER:
            *params = (GLint) tobj->minFilter;
            break;
        case GL_TEXTURE_WRAP_S:
            *params = (GLint) tobj->wrapS;
            break;
        case GL_TEXTURE_WRAP_T:
            *params = (GLint) tobj->wrapT;
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_WRAP_R:
            *params = (GLint) tobj->wrapR;
            break;
        case GL_TEXTURE_PRIORITY:
            *params = (GLint) tobj->priority;
            break;
#endif
        case GL_TEXTURE_BORDER_COLOR:
            params[0] = (GLint) (tobj->borderColor.r * CR_MAXINT);
            params[1] = (GLint) (tobj->borderColor.g * CR_MAXINT);
            params[2] = (GLint) (tobj->borderColor.b * CR_MAXINT);
            params[3] = (GLint) (tobj->borderColor.a * CR_MAXINT);
            break;
#ifdef CR_OPENGL_VERSION_1_2
        case GL_TEXTURE_MIN_LOD:
            *params = (GLint) tobj->minLod;
            break;
        case GL_TEXTURE_MAX_LOD:
            *params = (GLint) tobj->maxLod;
            break;
        case GL_TEXTURE_BASE_LEVEL:
            *params = (GLint) tobj->baseLevel;
            break;
        case GL_TEXTURE_MAX_LEVEL:
            *params = (GLint) tobj->maxLevel;
            break;
#endif
#ifdef CR_EXT_texture_filter_anisotropic
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (g->extensions.EXT_texture_filter_anisotropic) {
                *params = (GLint) tobj->maxAnisotropy;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, 
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_ARB_depth_texture
        case GL_DEPTH_TEXTURE_MODE_ARB:
            if (g->extensions.ARB_depth_texture) {
                *params = (GLint) tobj->depthMode;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_ARB_shadow
        case GL_TEXTURE_COMPARE_MODE_ARB:
            if (g->extensions.ARB_shadow) {
                *params = (GLint) tobj->compareMode;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
        case GL_TEXTURE_COMPARE_FUNC_ARB:
            if (g->extensions.ARB_shadow) {
                *params = (GLint) tobj->compareFunc;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_ARB_shadow_ambient
        case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
            if (g->extensions.ARB_shadow_ambient) {
                *params = (GLint) tobj->compareFailValue;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
#ifdef CR_SGIS_generate_mipmap
        case GL_GENERATE_MIPMAP_SGIS:
            if (g->extensions.SGIS_generate_mipmap) {
                *params = (GLint) tobj->generateMipmap;
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                    "glGetTexParameter: invalid pname: 0x%x", pname);
                return;
            }
            break;
#endif
        case GL_TEXTURE_RESIDENT:
            /* XXX todo */
            crWarning("glGetTexParameteriv GL_TEXTURE_RESIDENT is unimplemented");
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, 
                "glGetTexParameter: invalid pname: %d", pname);
            return;
    }
}


void STATE_APIENTRY
crStatePrioritizeTextures(GLsizei n, const GLuint *textures,
                                                    const GLclampf *priorities) 
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;
    GLsizei i;
    UNUSED(priorities);

    for (i = 0; i < n; ++i)
    {
        GLuint tex = textures[i];
        GET_TOBJ(tobj, g, tex);
        if (!tobj)
        {
            Assert(crHashtableIsKeyUsed(g->shared->textureTable, tex));
            tobj = crStateTextureAllocate_t(g, tex);
        }

        /* so far the code just ensures the tex object is created to make
         * the crserverlib code be able to pass it to host ogl */

        /* TODO: store texture priorities in the state data to be able to restore it properly
         * on save state load */
    }

    return;
}


GLboolean STATE_APIENTRY
crStateAreTexturesResident(GLsizei n, const GLuint *textures,
                                                     GLboolean *residences) 
{
    UNUSED(n);
    UNUSED(textures);
    UNUSED(residences);
    /* TODO: */
    return GL_TRUE;
}


GLboolean STATE_APIENTRY
crStateIsTexture(GLuint texture)
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;

    GET_TOBJ(tobj, g, texture);
    return tobj != NULL;
}

static void crStateCheckTextureHWIDCB(unsigned long key, void *data1, void *data2)
{
    CRTextureObj *pTex = (CRTextureObj *) data1;
    crCheckIDHWID_t *pParms = (crCheckIDHWID_t*) data2;
    (void) key;

    if (crStateGetTextureObjHWID(pTex)==pParms->hwid)
        pParms->id = pTex->id;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateTextureHWIDtoID(GLuint hwid)
{
    CRContext *g = GetCurrentContext();
    crCheckIDHWID_t parms;

    parms.id = hwid;
    parms.hwid = hwid;

    crHashtableWalk(g->shared->textureTable, crStateCheckTextureHWIDCB, &parms);
    return parms.id;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateGetTextureHWID(GLuint id)
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;

    GET_TOBJ(tobj, g, id);

#ifdef DEBUG_misha
    if (id)
    {
        Assert(tobj);
    }
    else
    {
        Assert(!tobj);
    }
    if (tobj)
    {
/*        crDebug("tex id(%d), hwid(%d)", tobj->id, tobj->hwid);*/
    }
#endif


    return tobj ? crStateGetTextureObjHWID(tobj) : 0;
}

DECLEXPORT(GLuint) STATE_APIENTRY crStateGetTextureObjHWID(CRTextureObj *tobj)
{
    CRASSERT(tobj);

#ifndef IN_GUEST
    if (tobj->id && !tobj->hwid)
    {
        CRASSERT(diff_api.GenTextures);
        diff_api.GenTextures(1, &tobj->hwid);
#if 0 /*def DEBUG_misha*/
        crDebug("tex id(%d), hwid(%d)", tobj->id, tobj->hwid);
#endif
        CRASSERT(tobj->hwid);
    }
#endif

    return tobj->hwid;
}
