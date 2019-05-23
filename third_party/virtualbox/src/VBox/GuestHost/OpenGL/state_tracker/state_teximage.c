/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */


#include "state.h"
#include "state/cr_statetypes.h"
#include "state/cr_texture.h"
#include "cr_pixeldata.h"
#include "cr_string.h"
#include "cr_mem.h"
#include "cr_version.h"
#include "state_internals.h"

#ifndef IN_GUEST
/*# define CR_DUMP_TEXTURES_2D*/
#endif

#ifdef CR_DUMP_TEXTURES_2D
static int _tnum = 0;

#pragma pack(1)
typedef struct tgaheader_tag
{
    char  idlen;

    char  colormap;

    char  imagetype;

    short cm_index;
    short cm_len;
    char  cm_entrysize;

    short x, y, w, h;
    char  depth;
    char  imagedesc;
    
} tgaheader_t;
#pragma pack()

static crDumpTGA(short w, short h, char* data)
{
    char fname[200];
    tgaheader_t header;
    FILE *out;

    if (!w || !h) return;

    sprintf(fname, "tex%i.tga", _tnum++);
    out = fopen(fname, "w");
    if (!out) crError("can't create %s!", fname);

    header.idlen = 0;
    header.colormap = 0;
    header.imagetype = 2;
    header.cm_index = 0;
    header.cm_len = 0;
    header.cm_entrysize = 0;
    header.x = 0;
    header.y = 0;
    header.w = w;
    header.h = h;
    header.depth = 32;
    header.imagedesc = 0x08;
    fwrite(&header, sizeof(header), 1, out);

    fwrite(data, w*h*4, 1, out);

    fclose(out);
}
#endif


static int
bitcount(int value)
{
    int bits = 0;
    for (; value > 0; value >>= 1) {
        if (value & 0x1)
            bits++;
    }
    return bits;
}


/**
 * Return 1 if the target is a proxy target, 0 otherwise.
 */
static GLboolean
IsProxyTarget(GLenum target)
{
    return (target == GL_PROXY_TEXTURE_1D ||
                    target == GL_PROXY_TEXTURE_2D ||
                    target == GL_PROXY_TEXTURE_3D ||
                    target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
                    target == GL_PROXY_TEXTURE_CUBE_MAP);
}


/**
 * Test if a texture width, height or depth is legal.
 * It must be true that 0 <= size <= max.
 * Furthermore, if the ARB_texture_non_power_of_two extension isn't
 * present, size must also be a power of two.
 */
static GLboolean
isLegalSize(CRContext *g, GLsizei size, GLsizei max)
{
    if (size < 0 || size > max)
        return GL_FALSE;
    if (!g->extensions.ARB_texture_non_power_of_two) {
        /* check for power of two */
        if (size > 0 && bitcount(size) != 1)
            return GL_FALSE;
    }
    return GL_TRUE;
}


/**
 * Return the max legal texture level parameter for the given target.
 */
static GLint
MaxTextureLevel(CRContext *g, GLenum target)
{
    CRTextureState *t = &(g->texture);
    switch (target) {
        case GL_TEXTURE_1D:
        case GL_PROXY_TEXTURE_1D:
        case GL_TEXTURE_2D:
        case GL_PROXY_TEXTURE_2D:
            return t->maxLevel;
        case GL_TEXTURE_3D:
        case GL_PROXY_TEXTURE_3D:
            return t->max3DLevel;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
        case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
            return t->maxCubeMapLevel;
        case GL_TEXTURE_RECTANGLE_NV:
        case GL_PROXY_TEXTURE_RECTANGLE_NV:
            return t->maxRectLevel;
        default:
            return 0;
    }
}


/**
 * If GL_GENERATE_MIPMAP_SGIS is true and we modify the base level texture
 * image we have to finish the mipmap.
 * All we really have to do is fill in the width/height/format/etc for the
 * remaining image levels.  The image data doesn't matter here - the back-
 * end OpenGL library will fill those in.
 */
static void
generate_mipmap(CRTextureObj *tobj, GLenum target)
{
    CRTextureLevel *levels;
    GLint level, width, height, depth;

        if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
            target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
          levels = tobj->level[target - GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB];
        }
        else {
          levels = tobj->level[0];
        }

    width = levels[tobj->baseLevel].width;
    height = levels[tobj->baseLevel].height;
    depth = levels[tobj->baseLevel].depth;

    for (level = tobj->baseLevel + 1; level <= tobj->maxLevel; level++)
    {
        if (width > 1)
            width /= 2;
        if (height > 1)
            height /= 2;
        if (depth > 1)
            depth /= 2;
        levels[level].width = width;
        levels[level].height = height;
        levels[level].depth = depth;
        levels[level].internalFormat = levels[tobj->baseLevel].internalFormat;
        levels[level].format = levels[tobj->baseLevel].format;
        levels[level].type = levels[tobj->baseLevel].type;
#ifdef CR_ARB_texture_compression
        levels[level].compressed = levels[tobj->baseLevel].compressed;
#endif
        levels[level].texFormat = levels[tobj->baseLevel].texFormat;
        if (width == 1 && height == 1 && depth == 1)
            break;
    }

    /* Set this flag so when we do the state diff, we enable GENERATE_MIPMAP
     * prior to calling diff.TexImage().
     */
    levels[tobj->baseLevel].generateMipmap = GL_TRUE;
}


/**
 * Given a texture target and level, return pointers to the corresponding
 * texture object and texture image level.
 */
void
crStateGetTextureObjectAndImage(CRContext *g, GLenum texTarget, GLint level,
                                                                CRTextureObj **obj, CRTextureLevel **img)
{
    CRTextureState *t = &(g->texture);
    CRTextureUnit *unit = t->unit + t->curTextureUnit;

    switch (texTarget) {
        case GL_TEXTURE_1D:
            *obj = unit->currentTexture1D;
            *img = unit->currentTexture1D->level[0] + level;
            return;
        case GL_PROXY_TEXTURE_1D:
            *obj = &(t->proxy1D);
            *img = t->proxy1D.level[0] + level;
            return;
        case GL_TEXTURE_2D:
            *obj = unit->currentTexture2D;
            *img = unit->currentTexture2D->level[0] + level;
            return;
        case GL_PROXY_TEXTURE_2D:
            *obj = &(t->proxy2D);
            *img = t->proxy2D.level[0] + level;
            return;
        case GL_TEXTURE_3D:
            *obj = unit->currentTexture3D;
            *img = unit->currentTexture3D->level[0] + level;
            return;
        case GL_PROXY_TEXTURE_3D:
            *obj = &(t->proxy3D);
            *img = t->proxy3D.level[0] + level;
            return;
        default:
             /* fall-through */
            ;
    }

#ifdef CR_NV_texture_rectangle 
    if (g->extensions.NV_texture_rectangle) {
        switch (texTarget) {
            case GL_PROXY_TEXTURE_RECTANGLE_NV:
                *obj = &(t->proxyRect);
                *img = t->proxyRect.level[0] + level;
                return;
            case GL_TEXTURE_RECTANGLE_NV:
                *obj = unit->currentTextureRect;
                *img = unit->currentTextureRect->level[0] + level;
                return;
            default:
             /* fall-through */
             ;
        }
    }
#endif

#ifdef CR_ARB_texture_cube_map
    if (g->extensions.ARB_texture_cube_map) {
        switch (texTarget) {
            case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
                *obj = &(t->proxyCubeMap);
                *img = t->proxyCubeMap.level[0] + level;
                return;
            case GL_TEXTURE_CUBE_MAP_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = NULL;
                return;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = unit->currentTextureCubeMap->level[0] + level;
                return;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = unit->currentTextureCubeMap->level[1] + level;
                return;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = unit->currentTextureCubeMap->level[2] + level;
                return;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = unit->currentTextureCubeMap->level[3] + level;
                return;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = unit->currentTextureCubeMap->level[4] + level;
                return;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
                *obj = unit->currentTextureCubeMap;
                *img = unit->currentTextureCubeMap->level[5] + level;
                return;
            default:
             /* fall-through */
             ;
        }
    }
#endif

    crWarning("unexpected texTarget 0x%x", texTarget);
    *obj = NULL;
    *img = NULL;
}


/**
 * Do parameter error checking for glTexImagexD functions.
 * We'll call crStateError if we detect any errors, unless it's a proxy target.
 * Return GL_TRUE if any errors, GL_FALSE if no errors.
 */
static GLboolean
ErrorCheckTexImage(GLuint dims, GLenum target, GLint level,
                                     GLsizei width, GLsizei height, GLsizei depth, GLint border)
{
    CRContext *g = GetCurrentContext();

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glTexImage%uD called in Begin/End", dims);
        return GL_TRUE;
    }

    /*
     * Test target
     */
    switch (target)
    {
        case GL_TEXTURE_1D:
        case GL_PROXY_TEXTURE_1D:
            if (dims != 1) {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                         "glTexImage(invalid target=0x%x)", target);
                return GL_TRUE;
            }
            break;
        case GL_TEXTURE_2D:
        case GL_PROXY_TEXTURE_2D:
            if (dims != 2) {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                         "glTexImage(invalid target=0x%x)", target);
                return GL_TRUE;
            }
            break;
        case GL_TEXTURE_3D:
        case GL_PROXY_TEXTURE_3D:
            if (dims != 3) {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                         "glTexImage(invalid target=0x%x)", target);
                return GL_TRUE;
            }
            break;
#ifdef CR_NV_texture_rectangle
        case GL_TEXTURE_RECTANGLE_NV:
        case GL_PROXY_TEXTURE_RECTANGLE_NV:
            if (dims != 2 || !g->extensions.NV_texture_rectangle) {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                         "glTexImage2D(invalid target=0x%x)", target);
                return GL_TRUE;
            }
            break;
#endif
#ifdef CR_ARB_texture_cube_map
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
        case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
            if (dims != 2 || !g->extensions.ARB_texture_cube_map) {
                crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                         "glTexImage2D(invalid target=0x%x)", target);
                return GL_TRUE;
            }
            break;
#endif
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glTexImage%uD(invalid target=0x%x)", dims, target);
            return GL_TRUE;
    }

    /*
     * Test level
     */
    if (level < 0 || level > MaxTextureLevel(g, target)) {
        if (!IsProxyTarget(target))
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glTexImage%uD(level=%d)", dims, level);
        return GL_TRUE;
    }

    /*
     * Test border
     */
    if (border != 0 && border != 1) {
        if (!IsProxyTarget(target))
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glTexImage%uD(border=%d)", dims, border);
        return GL_TRUE;
    }

    if ((target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
             target == GL_TEXTURE_RECTANGLE_NV) && border != 0) {
        if (!IsProxyTarget(target))
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glTexImage2D(border=%d)", border);
        return GL_TRUE;
    }

    /*
     * Test width, height, depth
     */
    if (target == GL_PROXY_TEXTURE_1D || target == GL_TEXTURE_1D) {
        if (!isLegalSize(g, width - 2 * border, g->limits.maxTextureSize)) {
            if (!IsProxyTarget(target))
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glTexImage1D(width=%d)", width);
            return GL_TRUE;
        }
    }
    else if (target == GL_PROXY_TEXTURE_2D || target == GL_TEXTURE_2D) {
        if (!isLegalSize(g, width - 2 * border, g->limits.maxTextureSize) ||
                !isLegalSize(g, height - 2 * border, g->limits.maxTextureSize)) {
            if (!IsProxyTarget(target))
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glTexImage2D(width=%d, height=%d)", width, height);
            return GL_TRUE;
        }
    }
    else if (target == GL_PROXY_TEXTURE_3D || target == GL_TEXTURE_3D) {
        if (!isLegalSize(g, width - 2 * border, g->limits.max3DTextureSize) ||
                !isLegalSize(g, height - 2 * border, g->limits.max3DTextureSize) ||
                !isLegalSize(g, depth - 2 * border, g->limits.max3DTextureSize)) {
            if (!IsProxyTarget(target))
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glTexImage3D(width=%d, height=%d, depth=%d)",
                                         width, height, depth);
            return GL_TRUE;
        }
    }
    else if (target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
                     target == GL_TEXTURE_RECTANGLE_NV) {
        if (width < 0 || width > (int) g->limits.maxRectTextureSize ||
                height < 0 || height > (int) g->limits.maxRectTextureSize) {
            if (!IsProxyTarget(target))
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glTexImage2D(width=%d, height=%d)", width, height);
            return GL_TRUE;
        }
    }
    else {
        /* cube map */
        if (!isLegalSize(g, width - 2*border, g->limits.maxCubeMapTextureSize) ||
                !isLegalSize(g, height - 2*border, g->limits.maxCubeMapTextureSize) ||
                width != height) {
            if (!IsProxyTarget(target))
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glTexImage2D(width=%d, height=%d)", width, height);
            return GL_TRUE;
        }
    }

    /* OK, no errors */
    return GL_FALSE;
}


/**
 * Do error check for glTexSubImage() functions.
 * We'll call crStateError if we detect any errors.
 * Return GL_TRUE if any errors, GL_FALSE if no errors.
 */
static GLboolean
ErrorCheckTexSubImage(GLuint dims, GLenum target, GLint level,
                                            GLint xoffset, GLint yoffset, GLint zoffset,
                                            GLsizei width, GLsizei height, GLsizei depth)
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;
    CRTextureLevel *tl;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glTexSubImage%uD called in Begin/End", dims);
        return GL_TRUE;
    }

    if (dims == 1) {
        if (target != GL_TEXTURE_1D) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glTexSubImage1D(target=0x%x)", target);
            return GL_TRUE;
        }
    }
    else if (dims == 2) {
        if (target != GL_TEXTURE_2D &&
                target != GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
                target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB &&
                target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB &&
                target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB &&
                target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB &&
                target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB &&
                target != GL_TEXTURE_RECTANGLE_NV) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glTexSubImage2D(target=0x%x)", target);
            return GL_TRUE;
        }
    }
    else if (dims == 3) {
        if (target != GL_TEXTURE_3D) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glTexSubImage3D(target=0x%x)", target);
            return GL_TRUE;
        }
    }

    /* test level */
    if (level < 0 || level > MaxTextureLevel(g, target)) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glTexSubImage%uD(level=%d)", dims, level);
        return GL_TRUE;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    if (!tobj || !tl) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glTexSubImage%uD(target or level)", dims);
        return GL_TRUE;
    }

    /* test x/y/zoffset and size */
    if (xoffset < -tl->border || xoffset + width > tl->width) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glTexSubImage%uD(xoffset=%d + width=%d > %d)",
                                 dims, xoffset, width, tl->width);
        return GL_TRUE;
    }
    if (dims > 1 && (yoffset < -tl->border || yoffset + height > tl->height)) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glTexSubImage%uD(yoffset=%d + height=%d > %d)",
                                 dims, yoffset, height, tl->height);
        return GL_TRUE;
    }
    if (dims > 2 && (zoffset < -tl->border || zoffset + depth > tl->depth)) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glTexSubImage%uD(zoffset=%d and/or depth=%d)",
                                 dims, zoffset, depth);
        return GL_TRUE;
    }

    /* OK, no errors */
    return GL_FALSE;
}



void STATE_APIENTRY
crStateTexImage1D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei width, GLint border, GLenum format,
                                    GLenum type, const GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
#endif
    CRTextureObj *tobj;
    CRTextureLevel *tl;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
    (void)pixels;
#endif

    FLUSH();

    if (ErrorCheckTexImage(1, target, level, width, 1, 1, border)) {
        if (IsProxyTarget(target)) {
            /* clear all state, but don't generate error */
            crStateTextureInitTextureObj(g, &(t->proxy1D), 0, GL_TEXTURE_1D);
        }
        else {
            /* error was already recorded */
        }
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    if (IsProxyTarget(target))
        tl->bytes = 0;
    else
        tl->bytes = crImageSize(format, type, width, 1);

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    if (tl->bytes)
    {
        /* this is not a proxy texture target so alloc storage */
        if (tl->img)
            crFree(tl->img);
        tl->img = (GLubyte *) crAlloc(tl->bytes);
        if (!tl->img)
        {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY,
                         "glTexImage1D out of memory");
            return;
        }
        if (pixels)
            crPixelCopy1D((GLvoid *) tl->img, format, type,
                          pixels, format, type, width, &(c->unpack));
    }
#endif

    tl->width = width;
    tl->height = 1;
    tl->depth = 1;
    tl->format = format;
    tl->border = border;
    tl->internalFormat = internalFormat;
    crStateTextureInitTextureFormat(tl, internalFormat);
    tl->type = type;
    tl->compressed = GL_FALSE;
    if (width)
        tl->bytesPerPixel = tl->bytes / width;
    else
        tl->bytesPerPixel = 0;

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    /* XXX may need to do some fine-tuning here for proxy textures */
    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}

static void crStateNukeMipmaps(CRTextureObj *tobj)
{
    int i, face;

    for (face = 0; face < 6; face++)
    {
        CRTextureLevel *levels = tobj->level[face];

        if (levels) 
        {
            for (i = 0; i < CR_MAX_MIPMAP_LEVELS; i++)
            {
                if (levels[i].img) 
                {
                    crFree(levels[i].img);
                }
                levels[i].img = NULL;
                levels[i].bytes = 0;
                levels[i].internalFormat = GL_ONE;
                levels[i].format = GL_RGBA;
                levels[i].type = GL_UNSIGNED_BYTE;

            }
        }
    }
}

void STATE_APIENTRY
crStateCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj = NULL;
    CRTextureLevel *tl = NULL;
    (void)x; (void)y;
    
    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    crStateNukeMipmaps(tobj);

    tl->bytes = crImageSize(GL_RGBA, GL_UNSIGNED_BYTE, width, height);

    tl->width = width;
    tl->height = height;
    tl->depth = 1;
    tl->format = GL_RGBA;
    tl->internalFormat = internalFormat;
    crStateTextureInitTextureFormat(tl, internalFormat);
    tl->border = border;
    tl->type = GL_UNSIGNED_BYTE;
    tl->compressed = GL_FALSE;
    if (width && height)
    {
        tl->bytesPerPixel = tl->bytes / (width * height);
    }
    else
        tl->bytesPerPixel = 0;

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif
}

void STATE_APIENTRY
crStateTexImage2D(GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
#endif
    CRTextureObj *tobj = NULL;
    CRTextureLevel *tl = NULL;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    // Distributed textures are not used by VBox
    const int is_distrib = 0; // ((type == GL_TRUE) || (type == GL_FALSE));

    FLUSH();

    /* NOTE: we skip parameter error checking if this is a distributed
     * texture!  The user better provide correct parameters!!!
     */
    if (!is_distrib
            && ErrorCheckTexImage(2, target, level, width, height, 1, border)) {
        if (IsProxyTarget(target)) {
            /* clear all state, but don't generate error */
            crStateTextureInitTextureObj(g, &(t->proxy2D), 0, GL_TEXTURE_2D);
        }
        else {
            /* error was already recorded */
        }
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    if (level==tobj->baseLevel && (tl->width!=width || tl->height!=height))
    {
        crStateNukeMipmaps(tobj);
    }

    /* compute size of image buffer */
    if (is_distrib) {
        tl->bytes = crStrlen((char *) pixels) + 1;
        tl->bytes += crImageSize(format, GL_UNSIGNED_BYTE, width, height);
    }
    else if (IsProxyTarget(target)) {
        tl->bytes = 0;
    }
    else {
        tl->bytes = crImageSize(format, type, width, height);
    }

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    /* allocate the image buffer and fill it */
    if (tl->bytes)
    {
        /* this is not a proxy texture target so alloc storage */
        if (tl->img)
            crFree(tl->img);
        tl->img = (GLubyte *) crAlloc(tl->bytes);
        if (!tl->img)
        {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY,
                         "glTexImage2D out of memory");
            return;
        }
        if (pixels)
        {
            if (is_distrib)
            {
                crMemcpy((void *) tl->img, (void *) pixels, tl->bytes);
            }
            else
            {
                crPixelCopy2D(width, height,
                              (GLvoid *) tl->img, format, type, NULL, /* dst */
                              pixels, format, type, &(c->unpack));    /* src */
            }
        }
    }
#endif

    tl->width = width;
    tl->height = height;
    tl->depth = 1;
    tl->format = format;
    tl->internalFormat = internalFormat;
    crStateTextureInitTextureFormat(tl, internalFormat);
    tl->border = border;
    tl->type = type;
    tl->compressed = GL_FALSE;
    if (width && height)
    {
        if (is_distrib)
            tl->bytesPerPixel = 3; /* only support GL_RGB */
        else
            tl->bytesPerPixel = tl->bytes / (width * height);
    }
    else
        tl->bytesPerPixel = 0;

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    /* XXX may need to do some fine-tuning here for proxy textures */
    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);

#ifdef CR_DUMP_TEXTURES_2D
    if (pixels)
    {
        GLint w,h;
        char *data;

        diff_api.GetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &w);
        diff_api.GetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &h);

        data = crAlloc(w*h*4);
        if (!data) crError("no memory!");
        diff_api.GetTexImage(target, level, GL_RGBA, GL_UNSIGNED_BYTE, data);
        crDumpTGA(w, h, data);
        crFree(data);
    }
#endif
}

#if defined( CR_OPENGL_VERSION_1_2 ) || defined( GL_EXT_texture3D )
void STATE_APIENTRY
crStateTexImage3D(GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLsizei depth, GLint border,
                                    GLenum format, GLenum type, const GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
#endif
    CRTextureObj *tobj = NULL;
    CRTextureLevel *tl = NULL;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    (void)pixels;

    FLUSH();

    if (ErrorCheckTexImage(3, target, level, width, height, depth, border)) {
        if (IsProxyTarget(target)) {
            /* clear all state, but don't generate error */
            crStateTextureInitTextureObj(g, &(t->proxy3D), 0, GL_TEXTURE_3D);
        }
        else {
            /* error was already recorded */
        }
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    if (IsProxyTarget(target))
        tl->bytes = 0;
    else
        tl->bytes = crTextureSize(format, type, width, height, depth);

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    if (tl->bytes)
    {
        /* this is not a proxy texture target so alloc storage */
        if (tl->img)
            crFree(tl->img);
        tl->img = (GLubyte *) crAlloc(tl->bytes);
        if (!tl->img)
        {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY,
                         "glTexImage3D out of memory");
            return;
        }
        if (pixels)
            crPixelCopy3D(width, height, depth, (GLvoid *) (tl->img), format, type,
                          NULL, pixels, format, type, &(c->unpack));
    }
#endif

    tl->internalFormat = internalFormat;
    tl->border = border;
    tl->width = width;
    tl->height = height;
    tl->depth = depth;
    tl->format = format;
    tl->type = type;
    tl->compressed = GL_FALSE;

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    /* XXX may need to do some fine-tuning here for proxy textures */
    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}
#endif /* CR_OPENGL_VERSION_1_2 || GL_EXT_texture3D */


#ifdef GL_EXT_texture3D
void STATE_APIENTRY
crStateTexImage3DEXT(GLenum target, GLint level,
                                         GLenum internalFormat,
                                         GLsizei width, GLsizei height, GLsizei depth,
                                         GLint border, GLenum format, GLenum type,
                                         const GLvoid * pixels)
{
    crStateTexImage3D(target, level, (GLint) internalFormat, width, height,
                                        depth, border, format, type, pixels);
}
#endif /* GL_EXT_texture3D */


void STATE_APIENTRY
crStateTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                         GLsizei width, GLenum format,
                                         GLenum type, const GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
#endif
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    CRTextureUnit *unit = t->unit + t->curTextureUnit;
    CRTextureObj *tobj = unit->currentTexture1D;
    CRTextureLevel *tl = tobj->level[0] + level;
    (void)format; (void)type; (void)pixels;

    FLUSH();

    if (ErrorCheckTexSubImage(1, target, level, xoffset, 0, 0,
                                                        width, 1, 1)) {
        return; /* GL error state already set */
    }

#ifdef DEBUG_misha
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    xoffset += tl->border;

    crPixelCopy1D((void *) (tl->img + xoffset * tl->bytesPerPixel),
                  tl->format, tl->type,
                  pixels, format, type, width, &(c->unpack));
#endif

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type, const GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    CRTextureObj *tobj;
    CRTextureLevel *tl;
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
    GLubyte *subimg = NULL;
    GLubyte *img = NULL;
    GLubyte *src;
    int i;
#endif
    (void)format; (void)type; (void)pixels;

    FLUSH();

    if (ErrorCheckTexSubImage(2, target, level, xoffset, yoffset, 0,
                                                        width, height, 1)) {
        return; /* GL error state already set */
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

#ifdef DEBUG_misha
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    xoffset += tl->border;
    yoffset += tl->border;

    subimg = (GLubyte *) crAlloc(crImageSize(tl->format, tl->type, width, height));

    crPixelCopy2D(width, height, subimg, tl->format, tl->type, NULL,    /* dst */
                  pixels, format, type, &(c->unpack));                  /* src */

    img = tl->img +
        xoffset * tl->bytesPerPixel + yoffset * tl->width * tl->bytesPerPixel;

    src = subimg;

    /* Copy the data into the texture */
    for (i = 0; i < height; i++)
    {
        crMemcpy(img, src, tl->bytesPerPixel * width);
        img += tl->width * tl->bytesPerPixel;
        src += width * tl->bytesPerPixel;
    }

    crFree(subimg);
#endif

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);

#ifdef CR_DUMP_TEXTURES_2D
    {
        GLint w,h;
        char *data;

        diff_api.GetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &w);
        diff_api.GetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &h);

        data = crAlloc(w*h*4);
        if (!data) crError("no memory!");
        diff_api.GetTexImage(target, level, GL_RGBA, GL_UNSIGNED_BYTE, data);
        crDumpTGA(w, h, data);
        crFree(data);
    }
#endif
}

#if defined( CR_OPENGL_VERSION_1_2 )
void STATE_APIENTRY
crStateTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                         GLint zoffset, GLsizei width, GLsizei height,
                                         GLsizei depth, GLenum format, GLenum type,
                                         const GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    CRTextureUnit *unit = t->unit + t->curTextureUnit;
    CRTextureObj *tobj = unit->currentTexture3D;
    CRTextureLevel *tl = tobj->level[0] + level;
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
    GLubyte *subimg = NULL;
    GLubyte *img = NULL;
    GLubyte *src;
    int i;
#endif
    (void)format; (void)type; (void)pixels;

    FLUSH();

    if (ErrorCheckTexSubImage(3, target, level, xoffset, yoffset, zoffset,
                                                        width, height, depth)) {
        return; /* GL error state already set */
    }

#ifdef DEBUG_misha
    CRASSERT(target == GL_TEXTURE_3D);
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    xoffset += tl->border;
    yoffset += tl->border;
    zoffset += tl->border;

    subimg =
        (GLubyte *)
        crAlloc(crTextureSize(tl->format, tl->type, width, height, depth));

    crPixelCopy3D(width, height, depth, subimg, tl->format, tl->type, NULL,
                  pixels, format, type, &(c->unpack));

    img = tl->img + xoffset * tl->bytesPerPixel +
        yoffset * tl->width * tl->bytesPerPixel +
        zoffset * tl->width * tl->height * tl->bytesPerPixel;

    src = subimg;

    /* Copy the data into the texture */
    for (i = 0; i < depth; i++)
    {
        crMemcpy(img, src, tl->bytesPerPixel * width * height);
        img += tl->width * tl->height * tl->bytesPerPixel;
        src += width * height * tl->bytesPerPixel;
    }

    crFree(subimg);
#endif

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}
#endif /* CR_OPENGL_VERSION_1_2 || GL_EXT_texture3D */


void STATE_APIENTRY
crStateCompressedTexImage1DARB(GLenum target, GLint level,
                                                             GLenum internalFormat, GLsizei width,
                                                             GLint border, GLsizei imageSize,
                                                             const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj;
    CRTextureLevel *tl;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    (void)data;

    FLUSH();

    if (ErrorCheckTexImage(1, target, level, width, 1, 1, border)) {
        if (IsProxyTarget(target)) {
            /* clear all state, but don't generate error */
            crStateTextureInitTextureObj(g, &(t->proxy1D), 0, GL_TEXTURE_1D);
        }
        else {
            /* error was already recorded */
        }
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    if (IsProxyTarget(target))
        tl->bytes = 0;
    else
        tl->bytes = imageSize;

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    if (tl->bytes)
    {
        /* this is not a proxy texture target so alloc storage */
        if (tl->img)
            crFree(tl->img);
        tl->img = (GLubyte *) crAlloc(tl->bytes);
        if (!tl->img)
        {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY,
                         "glTexImage1D out of memory");
            return;
        }
        if (data)
            crMemcpy(tl->img, data, imageSize);
    }
#endif

    tl->width = width;
    tl->height = 1;
    tl->depth = 1;
    tl->border = border;
    tl->format = GL_NONE;
    tl->type = GL_NONE;
    tl->internalFormat = internalFormat;
    crStateTextureInitTextureFormat(tl, internalFormat);
    tl->compressed = GL_TRUE;
    tl->bytesPerPixel = 0; /* n/a */

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateCompressedTexImage2DARB(GLenum target, GLint level,
                               GLenum internalFormat, GLsizei width,
                               GLsizei height, GLint border,
                               GLsizei imageSize, const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj = NULL;
    CRTextureLevel *tl = NULL;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    (void)data;

    FLUSH();

    if (ErrorCheckTexImage(2, target, level, width, height, 1, border)) {
        if (IsProxyTarget(target)) {
            /* clear all state, but don't generate error */
            crStateTextureInitTextureObj(g, &(t->proxy2D), 0, GL_TEXTURE_2D);
        }
        else {
            /* error was already recorded */
        }
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    if (IsProxyTarget(target))
        tl->bytes = 0;
    else
        tl->bytes = imageSize;

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    if (tl->bytes)
    {
        /* this is not a proxy texture target so alloc storage */
        if (tl->img)
            crFree(tl->img);
        tl->img = (GLubyte *) crAlloc(tl->bytes);
        if (!tl->img)
        {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY,
                         "glTexImage2D out of memory");
            return;
        }
        if (data)
            crMemcpy(tl->img, data, imageSize);
    }
#endif

    tl->width = width;
    tl->height = height;
    tl->depth = 1;
    tl->border = border;
    tl->format = GL_NONE;
    tl->type = GL_NONE;
    tl->internalFormat = internalFormat;
    crStateTextureInitTextureFormat(tl, internalFormat);
    tl->compressed = GL_TRUE;
    tl->bytesPerPixel = 0; /* n/a */

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    /* XXX may need to do some fine-tuning here for proxy textures */
    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateCompressedTexImage3DARB(GLenum target, GLint level,
                                                             GLenum internalFormat, GLsizei width,
                                                             GLsizei height, GLsizei depth, GLint border,
                                                             GLsizei imageSize, const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRTextureObj *tobj = NULL;
    CRTextureLevel *tl = NULL;
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    (void)data;

    FLUSH();

    if (ErrorCheckTexImage(3, target, level, width, height, depth, border)) {
        if (IsProxyTarget(target)) {
            /* clear all state, but don't generate error */
            crStateTextureInitTextureObj(g, &(t->proxy3D), 0, GL_TEXTURE_3D);
        }
        else {
            /* error was already recorded */
        }
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    CRASSERT(tobj);
    CRASSERT(tl);

    if (IsProxyTarget(target))
        tl->bytes = 0;
    else
        tl->bytes = imageSize;

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    if (tl->bytes)
    {
        /* this is not a proxy texture target so alloc storage */
        if (tl->img)
            crFree(tl->img);
        tl->img = (GLubyte *) crAlloc(tl->bytes);
        if (!tl->img)
        {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY,
                         "glCompressedTexImage3D out of memory");
            return;
        }
        if (data)
            crMemcpy(tl->img, data, imageSize);
    }
#endif

    tl->width = width;
    tl->height = height;
    tl->depth = depth;
    tl->border = border;
    tl->format = GL_NONE;
    tl->type = GL_NONE;
    tl->internalFormat = internalFormat;
    crStateTextureInitTextureFormat(tl, internalFormat);
    tl->compressed = GL_TRUE;
    tl->bytesPerPixel = 0; /* n/a */

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    /* XXX may need to do some fine-tuning here for proxy textures */
    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                                                    GLsizei width, GLenum format,
                                                                    GLsizei imageSize, const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    CRTextureUnit *unit = t->unit + t->curTextureUnit;
    CRTextureObj *tobj = unit->currentTexture1D;
    CRTextureLevel *tl = tobj->level[0] + level;
    (void)format; (void)imageSize; (void)data;

    FLUSH();

    if (ErrorCheckTexSubImage(1, target, level, xoffset, 0, 0, width, 1, 1)) {
        return; /* GL error state already set */
    }

#ifdef DEBUG_misha
    CRASSERT(target == GL_TEXTURE_1D);
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    xoffset += tl->border;

    if (xoffset == 0 && width == tl->width) {
        /* just memcpy */
        crMemcpy(tl->img, data, imageSize);
    }
    else {
        /* XXX this depends on the exact compression method */
        crWarning("Not implemented part crStateCompressedTexSubImage1DARB");
    }
#endif

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                                                    GLint yoffset, GLsizei width,
                                                                    GLsizei height, GLenum format,
                                                                    GLsizei imageSize, const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    CRTextureUnit *unit = t->unit + t->curTextureUnit;
    CRTextureObj *tobj = unit->currentTexture2D;
    CRTextureLevel *tl = tobj->level[0] + level;
    (void)format; (void)imageSize; (void)data;

    FLUSH();

#ifdef DEBUG_misha
    CRASSERT(target == GL_TEXTURE_2D);
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

    if (ErrorCheckTexSubImage(2, target, level, xoffset, yoffset, 0,
                                                        width, height, 1)) {
        return; /* GL error state already set */
    }

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    xoffset += tl->border;
    yoffset += tl->border;

    if (xoffset == 0 && width == tl->width 
        && yoffset == 0 && height == tl->height) 
    {
        /* just memcpy */
        crMemcpy(tl->img, data, imageSize);
    }
    else {
        /* XXX this depends on the exact compression method */
        crWarning("Not implemented part crStateCompressedTexSubImage2DARB");
    }
#endif

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                                                    GLint yoffset, GLint zoffset, GLsizei width,
                                                                    GLsizei height, GLsizei depth,
                                                                    GLenum format, GLsizei imageSize,
                                                                    const GLvoid * data)
{
    CRContext *g = GetCurrentContext();
    CRTextureState *t = &(g->texture);
    CRStateBits *sb = GetCurrentBits();
    CRTextureBits *tb = &(sb->texture);
    CRTextureUnit *unit = t->unit + t->curTextureUnit;
    CRTextureObj *tobj = unit->currentTexture3D;
    CRTextureLevel *tl = tobj->level[0] + level;
    (void)format; (void)imageSize; (void)data;

    FLUSH();

#ifdef DEBUG_misha
    CRASSERT(target == GL_TEXTURE_3D);
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

    if (ErrorCheckTexSubImage(3, target, level, xoffset, yoffset, zoffset,
                                                        width, height, depth)) {
        return; /* GL error state already set */
    }

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    xoffset += tl->border;
    yoffset += tl->border;
    zoffset += tl->border;

    if (xoffset == 0 && width == tl->width &&
        yoffset == 0 && height == tl->height &&
        zoffset == 0 && depth == tl->depth) {
        /* just memcpy */
        crMemcpy(tl->img, data, imageSize);
    }
    else {
        /* XXX this depends on the exact compression method */
        crWarning("Not implemented part crStateCompressedTexSubImage3DARB");
    }
#endif

#ifdef CR_SGIS_generate_mipmap
    if (level == tobj->baseLevel && tobj->generateMipmap) {
        generate_mipmap(tobj, target);
    }
    else {
        tl->generateMipmap = GL_FALSE;
    }
#endif

    DIRTY(tobj->dirty, g->neg_bitid);
    DIRTY(tobj->imageBit, g->neg_bitid);
    DIRTY(tl->dirty, g->neg_bitid);
    DIRTY(tb->dirty, g->neg_bitid);
}


void STATE_APIENTRY
crStateGetCompressedTexImageARB(GLenum target, GLint level, GLvoid * img)
{
    CRContext *g = GetCurrentContext();
    CRTextureObj *tobj;
    CRTextureLevel *tl;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetCompressedTexImage called in begin/end");
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    if (!tobj || !tl) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetCompressedTexImage(invalid target or level)");
        return;
    }

    if (!tl->compressed) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetCompressedTexImage(not a compressed texture)");
        return;
    }

#ifdef DEBUG_misha
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    crMemcpy(img, tl->img, tl->bytes);
#else
    diff_api.GetCompressedTexImageARB(target, level, img);
#endif
}


void STATE_APIENTRY
crStateGetTexImage(GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid * pixels)
{
    CRContext *g = GetCurrentContext();
#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClientState *c = &(g->client);
#endif
    CRTextureObj *tobj;
    CRTextureLevel *tl;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetTexImage called in begin/end");
        return;
    }

    crStateGetTextureObjectAndImage(g, target, level, &tobj, &tl);
    if (!tobj || !tl) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetTexImage(invalid target or level)");
        return;
    }

    if (tl->compressed) {
        crWarning("glGetTexImage cannot decompress a compressed texture!");
        return;
    }

#ifdef DEBUG_misha
    CRASSERT(tl->bytes);
    CRASSERT(tl->height);
    CRASSERT(tl->width);
    CRASSERT(tl->depth);
#endif

    switch (format)
    {
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
        case GL_RGB:
        case GL_RGBA:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glGetTexImage called with bogus format: %d", format);
            return;
    }

    switch (type)
    {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
        case GL_UNSIGNED_INT:
        case GL_INT:
        case GL_FLOAT:
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glGetTexImage called with bogus type: %d", type);
            return;
    }

#ifndef CR_STATE_NO_TEXTURE_IMAGE_STORE
#ifdef CR_OPENGL_VERSION_1_2
    if (target == GL_TEXTURE_3D)
    {
        crPixelCopy3D(tl->width, tl->height, tl->depth, (GLvoid *) pixels, format,
                      type, NULL, (tl->img), format, type, &(c->pack));
    }
    else
#endif
    if ((target == GL_TEXTURE_1D) || (target == GL_TEXTURE_2D))
    {
        crPixelCopy2D(tl->width, tl->height, (GLvoid *) pixels, format, type, NULL, /* dst */
                      tl->img, format, type, &(c->pack));                           /* src */
    }
#else
    diff_api.GetTexImage(target, level, format, type, pixels);
#endif
}
