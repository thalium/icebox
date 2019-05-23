/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_TEXTURE_H
#define CR_STATE_TEXTURE_H

#include "cr_hash.h"
#include "state/cr_statetypes.h"
#include "state/cr_limits.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tells state tracker to rely on diff_api to store/load texture images
 * and avoid host memory allocation.
 */
#define CR_STATE_NO_TEXTURE_IMAGE_STORE

#if defined(CR_ARB_pixel_buffer_object) && !defined(CR_STATE_NO_TEXTURE_IMAGE_STORE)
#error CR_ARB_pixel_buffer_object not supported without CR_STATE_NO_TEXTURE_IMAGE_STORE
#endif

#define CR_MAX_MIPMAP_LEVELS 20

typedef struct {
    GLubyte redbits;
    GLubyte greenbits;
    GLubyte bluebits;
    GLubyte alphabits;
    GLubyte luminancebits;
    GLubyte intensitybits;
    GLubyte indexbits;
} CRTextureFormat;

typedef struct {
    GLubyte *img;
    int bytes;
    GLint width;  /* width, height, depth includes the border */
    GLint height;
    GLint depth;
    GLint internalFormat;
    GLint border;
    GLenum format;
    GLenum type;
    int bytesPerPixel;
#if CR_ARB_texture_compression
    GLboolean compressed;
#endif
    GLboolean generateMipmap;
    const CRTextureFormat *texFormat;

    CRbitvalue dirty[CR_MAX_BITARRAY];
} CRTextureLevel;

typedef struct {
    GLenum                 target;
    GLuint                 id;
    GLuint                 hwid;

    /* The mipmap levels */
    CRTextureLevel        *level[6];  /* 6 cube faces */

    GLcolorf               borderColor;
    GLenum                 minFilter, magFilter;
    GLenum                 wrapS, wrapT;
#ifdef CR_OPENGL_VERSION_1_2
    GLenum                 wrapR;
    GLfloat                priority;
    GLfloat                minLod;
    GLfloat                maxLod;
    GLint                  baseLevel;
    GLint                  maxLevel;
#endif
#ifdef CR_EXT_texture_filter_anisotropic
    GLfloat                maxAnisotropy;
#endif
#ifdef CR_ARB_depth_texture
    GLenum                 depthMode;
#endif
#ifdef CR_ARB_shadow
    GLenum                 compareMode;
    GLenum                 compareFunc;
#endif
#ifdef CR_ARB_shadow_ambient
    GLfloat                compareFailValue;
#endif
#ifdef CR_SGIS_generate_mipmap
    GLboolean              generateMipmap;
#endif
    GLboolean              pinned; /* <- keep the texture alive if its ctxUsage reaches zero */
    CRbitvalue             dirty[CR_MAX_BITARRAY];
    CRbitvalue             imageBit[CR_MAX_BITARRAY];
    CRbitvalue             paramsBit[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
    /* bitfield representing the object usage. 1 means the object is used by the context with the given bitid */
    CRbitvalue             ctxUsage[CR_MAX_BITARRAY];
} CRTextureObj;

typedef struct {
    CRbitvalue dirty[CR_MAX_BITARRAY];
    CRbitvalue enable[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
    CRbitvalue current[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
    CRbitvalue objGen[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
    CRbitvalue eyeGen[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
    CRbitvalue genMode[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
    /* XXX someday create more bits for texture env state */
    CRbitvalue envBit[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
} CRTextureBits;

typedef struct {
    /* Current texture objects (in terms of glBindTexture and glActiveTexture) */
    CRTextureObj *currentTexture1D;
    CRTextureObj *currentTexture2D;
    CRTextureObj *currentTexture3D;
#ifdef CR_ARB_texture_cube_map
    CRTextureObj *currentTextureCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
    CRTextureObj *currentTextureRect;
#endif

    GLboolean   enabled1D;
    GLboolean   enabled2D;
    GLboolean   enabled3D;
#ifdef CR_ARB_texture_cube_map
    GLboolean   enabledCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
    GLboolean   enabledRect;
#endif
#ifdef CR_EXT_texture_lod_bias
    GLfloat     lodBias;
#endif

    GLenum      envMode;
    GLcolorf    envColor;
    
    /* GL_ARB_texture_env_combine */
    GLenum combineModeRGB;       /* GL_REPLACE, GL_DECAL, GL_ADD, etc. */
    GLenum combineModeA;         /* GL_REPLACE, GL_DECAL, GL_ADD, etc. */
    GLenum combineSourceRGB[3];  /* GL_PRIMARY_COLOR, GL_TEXTURE, etc. */
    GLenum combineSourceA[3];    /* GL_PRIMARY_COLOR, GL_TEXTURE, etc. */
    GLenum combineOperandRGB[3]; /* SRC_COLOR, ONE_MINUS_SRC_COLOR, etc */
    GLenum combineOperandA[3];   /* SRC_ALPHA, ONE_MINUS_SRC_ALPHA, etc */
    GLfloat combineScaleRGB;     /* 1 or 2 or 4 */
    GLfloat combineScaleA;       /* 1 or 2 or 4 */

    GLtexcoordb textureGen;
    GLvectorf   objSCoeff;
    GLvectorf   objTCoeff;
    GLvectorf   objRCoeff;
    GLvectorf   objQCoeff;
    GLvectorf   eyeSCoeff;
    GLvectorf   eyeTCoeff;
    GLvectorf   eyeRCoeff;
    GLvectorf   eyeQCoeff;
    GLtexcoorde gen;

    /* These are only used for glPush/PopAttrib */
    CRTextureObj Saved1D;
    CRTextureObj Saved2D;
    CRTextureObj Saved3D;
#ifdef CR_ARB_texture_cube_map
    CRTextureObj SavedCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
    CRTextureObj SavedRect;
#endif
} CRTextureUnit;

typedef struct {
    /* Default texture objects (name = 0) */
    CRTextureObj base1D;
    CRTextureObj base2D;
    CRTextureObj base3D;
#ifdef CR_ARB_texture_cube_map
    CRTextureObj baseCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
    CRTextureObj baseRect;
#endif
    
    /* Proxy texture objects */
    CRTextureObj proxy1D;
    CRTextureObj proxy2D;
    CRTextureObj proxy3D;
#ifdef CR_ARB_texture_cube_map
    CRTextureObj proxyCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
    CRTextureObj proxyRect;
#endif

    GLuint      curTextureUnit; /* GL_ACTIVE_TEXTURE */

    GLint       maxLevel;  /* number of mipmap levels possible: [0..max] */
    GLint       max3DLevel;
    GLint       maxCubeMapLevel;
    GLint       maxRectLevel;
    
    GLboolean   broadcastTextures; /*@todo what is it for?*/

    /* Per-texture unit state: */
    CRTextureUnit   unit[CR_MAX_TEXTURE_UNITS];
} CRTextureState;

DECLEXPORT(void) crStateTextureInit(CRContext *ctx);
DECLEXPORT(void) crStateTextureDestroy(CRContext *ctx);
DECLEXPORT(void) crStateTextureFree(CRContext *ctx);

DECLEXPORT(void) crStateTextureInitTexture(GLuint name);
DECLEXPORT(CRTextureObj *) crStateTextureAllocate(GLuint name);
    /*void crStateTextureDelete(GLuint name);*/
DECLEXPORT(CRTextureObj *) crStateTextureGet(GLenum target, GLuint textureid);
DECLEXPORT(int) crStateTextureGetSize(GLenum target, GLenum level);
DECLEXPORT(const GLvoid *) crStateTextureGetData(GLenum target, GLenum level);

DECLEXPORT(int) crStateTextureCheckDirtyImages(CRContext *from, CRContext *to, GLenum target, int textureUnit);

DECLEXPORT(void) crStateTextureDiff(CRTextureBits *t, CRbitvalue *bitID,
                                    CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateTextureSwitch(CRTextureBits *t, CRbitvalue *bitID, 
                                      CRContext *fromCtx, CRContext *toCtx);

DECLEXPORT(void) crStateTextureObjectDiff(CRContext *fromCtx,
                                          const CRbitvalue *bitID,
                                          const CRbitvalue *nbitID,
                                          CRTextureObj *tobj, GLboolean alwaysDirty);

DECLEXPORT(void) crStateDiffAllTextureObjects( CRContext *g, CRbitvalue *bitID, GLboolean bForceUpdate );

DECLEXPORT(void) crStateDeleteTextureObjectData(CRTextureObj *tobj);
DECLEXPORT(void) crStateDeleteTextureObject(CRTextureObj *tobj);

DECLEXPORT(GLuint) STATE_APIENTRY crStateTextureHWIDtoID(GLuint hwid);
DECLEXPORT(GLuint) STATE_APIENTRY crStateGetTextureHWID(GLuint id);
DECLEXPORT(GLuint) STATE_APIENTRY crStateGetTextureObjHWID(CRTextureObj *tobj);

void crStateRegTextures(GLsizei n, GLuint *names);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_TEXTURE_H */
