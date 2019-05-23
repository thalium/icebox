/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"
#include "cr_error.h"
#include "cr_mem.h"

/**
 * \mainpage state_tracker 
 *
 * \section StateTrackerIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The state_tracker module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 *
 */
void crStateAttribInit (CRAttribState *a) 
{
    int i;
    a->attribStackDepth = 0;
    a->accumBufferStackDepth = 0;
    a->colorBufferStackDepth = 0;
    a->currentStackDepth = 0;
    a->depthBufferStackDepth = 0;
    a->enableStackDepth = 0;
    for ( i = 0 ; i < CR_MAX_ATTRIB_STACK_DEPTH ; i++)
    {
        a->enableStack[i].clip = NULL;
        a->enableStack[i].light = NULL;
        a->lightingStack[i].light = NULL;
        a->transformStack[i].clip = NULL;
        a->transformStack[i].clipPlane = NULL;
    }
    a->evalStackDepth = 0;
    a->fogStackDepth = 0;
    a->lightingStackDepth = 0;
    a->lineStackDepth = 0;
    a->listStackDepth = 0;
    a->pixelModeStackDepth = 0;
    a->pointStackDepth = 0;
    a->polygonStackDepth = 0;
    a->polygonStippleStackDepth = 0;
    a->scissorStackDepth = 0;
    a->stencilBufferStackDepth = 0;
    a->textureStackDepth = 0;
    a->transformStackDepth = 0;
    a->viewportStackDepth = 0;
}

/*@todo check if NV rect needed too*/
static void
copy_texunit(CRTextureUnit *dest, const CRTextureUnit *src)
{
    dest->enabled1D = src->enabled1D;
    dest->enabled2D = src->enabled2D;
    dest->enabled3D = src->enabled3D;
    dest->enabledCubeMap = src->enabledCubeMap;
    dest->envMode = src->envMode;
    dest->envColor = src->envColor;
    dest->textureGen = src->textureGen;
    dest->objSCoeff = src->objSCoeff;
    dest->objTCoeff = src->objTCoeff;
    dest->objRCoeff = src->objRCoeff;
    dest->objQCoeff = src->objQCoeff;
    dest->eyeSCoeff = src->eyeSCoeff;
    dest->eyeTCoeff = src->eyeTCoeff;
    dest->eyeRCoeff = src->eyeRCoeff;
    dest->eyeQCoeff = src->eyeQCoeff;
    dest->gen = src->gen;
    dest->currentTexture1D = src->currentTexture1D;
    dest->currentTexture2D = src->currentTexture2D;
    dest->currentTexture3D = src->currentTexture3D;
    dest->currentTextureCubeMap = src->currentTextureCubeMap;
}

static void
copy_texobj(CRTextureObj *dest, CRTextureObj *src, GLboolean copyName)
{
    if (copyName)
    {
        dest->id = src->id;
        dest->hwid = crStateGetTextureObjHWID(src);
    }

    dest->borderColor = src->borderColor;
    dest->wrapS = src->wrapS;
    dest->wrapT = src->wrapT;
    dest->minFilter = src->minFilter;
    dest->magFilter = src->magFilter;
#ifdef CR_OPENGL_VERSION_1_2
    dest->priority = src->priority;
    dest->wrapR = src->wrapR;
    dest->minLod = src->minLod;
    dest->maxLod = src->maxLod;
    dest->baseLevel = src->baseLevel;
    dest->maxLevel = src->maxLevel;
#endif
#ifdef CR_EXT_texture_filter_anisotropic
    dest->maxAnisotropy = src->maxAnisotropy;
#endif
}

void STATE_APIENTRY crStatePushAttrib(GLbitfield mask)
{
    CRContext *g = GetCurrentContext();
    CRAttribState *a = &(g->attrib);
    CRStateBits *sb = GetCurrentBits();
    CRAttribBits *ab = &(sb->attrib);
    unsigned int i;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glPushAttrib called in Begin/End");
        return;
    }

    if (a->attribStackDepth == CR_MAX_ATTRIB_STACK_DEPTH - 1)
    {
        crStateError(__LINE__, __FILE__, GL_STACK_OVERFLOW, "glPushAttrib called with a full stack!" );
        return;
    }

    FLUSH();

    a->pushMaskStack[a->attribStackDepth++] = mask;

    if (mask & GL_ACCUM_BUFFER_BIT)
    {
        a->accumBufferStack[a->accumBufferStackDepth].accumClearValue = g->buffer.accumClearValue;
        a->accumBufferStackDepth++;
    }
    if (mask & GL_COLOR_BUFFER_BIT)
    {
        a->colorBufferStack[a->colorBufferStackDepth].alphaTest = g->buffer.alphaTest;
        a->colorBufferStack[a->colorBufferStackDepth].alphaTestFunc = g->buffer.alphaTestFunc;
        a->colorBufferStack[a->colorBufferStackDepth].alphaTestRef = g->buffer.alphaTestRef;
        a->colorBufferStack[a->colorBufferStackDepth].blend = g->buffer.blend;
        a->colorBufferStack[a->colorBufferStackDepth].blendSrcRGB = g->buffer.blendSrcRGB;
        a->colorBufferStack[a->colorBufferStackDepth].blendDstRGB = g->buffer.blendDstRGB;
#if defined(CR_EXT_blend_func_separate)
        a->colorBufferStack[a->colorBufferStackDepth].blendSrcA = g->buffer.blendSrcA;
        a->colorBufferStack[a->colorBufferStackDepth].blendDstA = g->buffer.blendDstA;
#endif
#ifdef CR_EXT_blend_color
        a->colorBufferStack[a->colorBufferStackDepth].blendColor = g->buffer.blendColor;
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
        a->colorBufferStack[a->colorBufferStackDepth].blendEquation = g->buffer.blendEquation;
#endif
        a->colorBufferStack[a->colorBufferStackDepth].dither = g->buffer.dither;
        a->colorBufferStack[a->colorBufferStackDepth].drawBuffer = g->buffer.drawBuffer;
        a->colorBufferStack[a->colorBufferStackDepth].logicOp = g->buffer.logicOp;
        a->colorBufferStack[a->colorBufferStackDepth].indexLogicOp = g->buffer.indexLogicOp;
        a->colorBufferStack[a->colorBufferStackDepth].logicOpMode = g->buffer.logicOpMode;
        a->colorBufferStack[a->colorBufferStackDepth].colorClearValue = g->buffer.colorClearValue;
        a->colorBufferStack[a->colorBufferStackDepth].indexClearValue = g->buffer.indexClearValue;
        a->colorBufferStack[a->colorBufferStackDepth].colorWriteMask = g->buffer.colorWriteMask;
        a->colorBufferStack[a->colorBufferStackDepth].indexWriteMask = g->buffer.indexWriteMask;
        a->colorBufferStackDepth++;
    }
    if (mask & GL_CURRENT_BIT)
    {
        for (i = 0 ; i < CR_MAX_VERTEX_ATTRIBS ; i++)
        {
            COPY_4V(a->currentStack[a->currentStackDepth].attrib[i] , g->current.vertexAttrib[i]);
            COPY_4V(a->currentStack[a->currentStackDepth].rasterAttrib[i] , g->current.rasterAttrib[i]);
        }
        a->currentStack[a->currentStackDepth].rasterValid = g->current.rasterValid;
        a->currentStack[a->currentStackDepth].edgeFlag = g->current.edgeFlag;
        a->currentStack[a->currentStackDepth].colorIndex = g->current.colorIndex;
        a->currentStackDepth++;
    }
    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        a->depthBufferStack[a->depthBufferStackDepth].depthTest = g->buffer.depthTest;
        a->depthBufferStack[a->depthBufferStackDepth].depthFunc = g->buffer.depthFunc;
        a->depthBufferStack[a->depthBufferStackDepth].depthClearValue = g->buffer.depthClearValue;
        a->depthBufferStack[a->depthBufferStackDepth].depthMask = g->buffer.depthMask;
        a->depthBufferStackDepth++;
    }
    if (mask & GL_ENABLE_BIT)
    {
        if (a->enableStack[a->enableStackDepth].clip == NULL)
        {
            a->enableStack[a->enableStackDepth].clip = (GLboolean *) crCalloc( g->limits.maxClipPlanes * sizeof( GLboolean ));
        }
        if (a->enableStack[a->enableStackDepth].light == NULL)
        {
            a->enableStack[a->enableStackDepth].light = (GLboolean *) crCalloc( g->limits.maxLights * sizeof( GLboolean ));
        }
        a->enableStack[a->enableStackDepth].alphaTest = g->buffer.alphaTest;
        a->enableStack[a->enableStackDepth].autoNormal = g->eval.autoNormal;
        a->enableStack[a->enableStackDepth].blend = g->buffer.blend;
        for (i = 0 ; i < g->limits.maxClipPlanes ; i++)
        {
            a->enableStack[a->enableStackDepth].clip[i] = g->transform.clip[i];
        }
        a->enableStack[a->enableStackDepth].colorMaterial = g->lighting.colorMaterial;
        a->enableStack[a->enableStackDepth].cullFace = g->polygon.cullFace;
        a->enableStack[a->enableStackDepth].depthTest = g->buffer.depthTest;
        a->enableStack[a->enableStackDepth].dither = g->buffer.dither;
        a->enableStack[a->enableStackDepth].fog = g->fog.enable;
        for (i = 0 ; i < g->limits.maxLights ; i++)
        {
            a->enableStack[a->enableStackDepth].light[i] = g->lighting.light[i].enable;
        }
        a->enableStack[a->enableStackDepth].lighting = g->lighting.lighting;
        a->enableStack[a->enableStackDepth].lineSmooth = g->line.lineSmooth;
        a->enableStack[a->enableStackDepth].lineStipple = g->line.lineStipple;
        a->enableStack[a->enableStackDepth].logicOp = g->buffer.logicOp;
        a->enableStack[a->enableStackDepth].indexLogicOp = g->buffer.indexLogicOp;
        for (i = 0 ; i < GLEVAL_TOT ; i++)
        {
            a->enableStack[a->enableStackDepth].map1[i] = g->eval.enable1D[i];
            a->enableStack[a->enableStackDepth].map2[i] = g->eval.enable2D[i];
        }
        a->enableStack[a->enableStackDepth].normalize = g->transform.normalize;
        a->enableStack[a->enableStackDepth].pointSmooth = g->point.pointSmooth;
#if CR_ARB_point_sprite
        a->enableStack[a->enableStackDepth].pointSprite = g->point.pointSprite;
        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++)
            a->enableStack[a->enableStackDepth].coordReplacement[i] = g->point.coordReplacement[i];
#endif
        a->enableStack[a->enableStackDepth].polygonOffsetLine = g->polygon.polygonOffsetLine;
        a->enableStack[a->enableStackDepth].polygonOffsetFill = g->polygon.polygonOffsetFill;
        a->enableStack[a->enableStackDepth].polygonOffsetPoint = g->polygon.polygonOffsetPoint;
        a->enableStack[a->enableStackDepth].polygonSmooth = g->polygon.polygonSmooth;
        a->enableStack[a->enableStackDepth].polygonStipple = g->polygon.polygonStipple;
#ifdef CR_OPENGL_VERSION_1_2
        a->enableStack[a->enableStackDepth].rescaleNormals = g->transform.rescaleNormals;
#endif
        a->enableStack[a->enableStackDepth].scissorTest = g->viewport.scissorTest;
        a->enableStack[a->enableStackDepth].stencilTest = g->stencil.stencilTest;
        for (i = 0 ; i < g->limits.maxTextureUnits; i++)
        {
            a->enableStack[a->enableStackDepth].texture1D[i] = g->texture.unit[i].enabled1D;
            a->enableStack[a->enableStackDepth].texture2D[i] = g->texture.unit[i].enabled2D;
#ifdef CR_OPENGL_VERSION_1_2
            a->enableStack[a->enableStackDepth].texture3D[i] = g->texture.unit[i].enabled3D;
#endif
#ifdef CR_ARB_texture_cube_map
            a->enableStack[a->enableStackDepth].textureCubeMap[i] = g->texture.unit[i].enabledCubeMap;
#endif
#ifdef CR_NV_texture_rectangle
            a->enableStack[a->enableStackDepth].textureRect[i] = g->texture.unit[i].enabledRect;
#endif
            a->enableStack[a->enableStackDepth].textureGenS[i] = g->texture.unit[i].textureGen.s;
            a->enableStack[a->enableStackDepth].textureGenT[i] = g->texture.unit[i].textureGen.t;
            a->enableStack[a->enableStackDepth].textureGenR[i] = g->texture.unit[i].textureGen.r;
            a->enableStack[a->enableStackDepth].textureGenQ[i] = g->texture.unit[i].textureGen.q;
        }
        a->enableStackDepth++;
    }
    if (mask & GL_EVAL_BIT)
    {
        for (i = 0 ; i < GLEVAL_TOT ; i++)
        {
            int size1 = g->eval.eval1D[i].order * gleval_sizes[i] * 
                    sizeof (GLfloat);
            int size2 = g->eval.eval2D[i].uorder * g->eval.eval2D[i].vorder *
                    gleval_sizes[i] * sizeof (GLfloat);
            a->evalStack[a->evalStackDepth].enable1D[i] = g->eval.enable1D[i];
            a->evalStack[a->evalStackDepth].enable2D[i] = g->eval.enable2D[i];
            a->evalStack[a->evalStackDepth].eval1D[i].u1 = g->eval.eval1D[i].u1;
            a->evalStack[a->evalStackDepth].eval1D[i].u2 = g->eval.eval1D[i].u2;
            a->evalStack[a->evalStackDepth].eval1D[i].order = g->eval.eval1D[i].order;
            a->evalStack[a->evalStackDepth].eval1D[i].coeff = (GLfloat*)crCalloc(size1);
            crMemcpy(a->evalStack[a->evalStackDepth].eval1D[i].coeff, g->eval.eval1D[i].coeff, size1);
            a->evalStack[a->evalStackDepth].eval2D[i].u1 = g->eval.eval2D[i].u1;
            a->evalStack[a->evalStackDepth].eval2D[i].u2 = g->eval.eval2D[i].u2;
            a->evalStack[a->evalStackDepth].eval2D[i].v1 = g->eval.eval2D[i].v1;
            a->evalStack[a->evalStackDepth].eval2D[i].v2 = g->eval.eval2D[i].v2;
            a->evalStack[a->evalStackDepth].eval2D[i].uorder = g->eval.eval2D[i].uorder;
            a->evalStack[a->evalStackDepth].eval2D[i].vorder = g->eval.eval2D[i].vorder;
            a->evalStack[a->evalStackDepth].eval2D[i].coeff = (GLfloat*)crCalloc(size2);
            crMemcpy(a->evalStack[a->evalStackDepth].eval2D[i].coeff, g->eval.eval2D[i].coeff, size2);
        }
        a->evalStack[a->evalStackDepth].autoNormal = g->eval.autoNormal;
        a->evalStack[a->evalStackDepth].un1D = g->eval.un1D;
        a->evalStack[a->evalStackDepth].u11D = g->eval.u11D;
        a->evalStack[a->evalStackDepth].u21D = g->eval.u21D;
        a->evalStack[a->evalStackDepth].un2D = g->eval.un2D;
        a->evalStack[a->evalStackDepth].u12D = g->eval.u12D;
        a->evalStack[a->evalStackDepth].u22D = g->eval.u22D;
        a->evalStack[a->evalStackDepth].vn2D = g->eval.vn2D;
        a->evalStack[a->evalStackDepth].v12D = g->eval.v12D;
        a->evalStack[a->evalStackDepth].v22D = g->eval.v22D;
        a->evalStackDepth++;
    }
    if (mask & GL_FOG_BIT)
    {
        a->fogStack[a->fogStackDepth].enable = g->fog.enable;
        a->fogStack[a->fogStackDepth].color = g->fog.color;
        a->fogStack[a->fogStackDepth].density = g->fog.density;
        a->fogStack[a->fogStackDepth].start = g->fog.start;
        a->fogStack[a->fogStackDepth].end = g->fog.end;
        a->fogStack[a->fogStackDepth].index = g->fog.index;
        a->fogStack[a->fogStackDepth].mode = g->fog.mode;
        a->fogStackDepth++;
    }
    if (mask & GL_HINT_BIT)
    {
        a->hintStack[a->hintStackDepth].perspectiveCorrection = g->hint.perspectiveCorrection;
        a->hintStack[a->hintStackDepth].pointSmooth = g->hint.pointSmooth;
        a->hintStack[a->hintStackDepth].lineSmooth = g->hint.lineSmooth;
        a->hintStack[a->hintStackDepth].polygonSmooth = g->hint.polygonSmooth;
        a->hintStack[a->hintStackDepth].fog = g->hint.fog;
#ifdef CR_EXT_clip_volume_hint
        a->hintStack[a->hintStackDepth].clipVolumeClipping = g->hint.clipVolumeClipping;
#endif
#ifdef CR_ARB_texture_compression
        a->hintStack[a->hintStackDepth].textureCompression = g->hint.textureCompression;
#endif
#ifdef CR_SGIS_generate_mipmap
        a->hintStack[a->hintStackDepth].generateMipmap = g->hint.generateMipmap;
#endif
        a->hintStackDepth++;
    }
    if (mask & GL_LIGHTING_BIT)
    {
        if (a->lightingStack[a->lightingStackDepth].light == NULL)
        {
            a->lightingStack[a->lightingStackDepth].light = (CRLight *) crCalloc( g->limits.maxLights * sizeof( CRLight ));
        }
        a->lightingStack[a->lightingStackDepth].lightModelAmbient = g->lighting.lightModelAmbient;
        a->lightingStack[a->lightingStackDepth].lightModelLocalViewer = g->lighting.lightModelLocalViewer;
        a->lightingStack[a->lightingStackDepth].lightModelTwoSide = g->lighting.lightModelTwoSide;
#if defined(CR_EXT_separate_specular_color) || defined(CR_OPENGL_VERSION_1_2)
        a->lightingStack[a->lightingStackDepth].lightModelColorControlEXT = g->lighting.lightModelColorControlEXT;
#endif
        a->lightingStack[a->lightingStackDepth].lighting = g->lighting.lighting;
        a->lightingStack[a->lightingStackDepth].colorMaterial = g->lighting.colorMaterial;
        a->lightingStack[a->lightingStackDepth].colorMaterialMode = g->lighting.colorMaterialMode;
        a->lightingStack[a->lightingStackDepth].colorMaterialFace = g->lighting.colorMaterialFace;
        for (i = 0 ; i < g->limits.maxLights; i++)
        {
            a->lightingStack[a->lightingStackDepth].light[i].enable = g->lighting.light[i].enable;
            a->lightingStack[a->lightingStackDepth].light[i].ambient = g->lighting.light[i].ambient;
            a->lightingStack[a->lightingStackDepth].light[i].diffuse = g->lighting.light[i].diffuse;
            a->lightingStack[a->lightingStackDepth].light[i].specular = g->lighting.light[i].specular;
            a->lightingStack[a->lightingStackDepth].light[i].spotDirection = g->lighting.light[i].spotDirection;
            a->lightingStack[a->lightingStackDepth].light[i].position = g->lighting.light[i].position;
            a->lightingStack[a->lightingStackDepth].light[i].spotExponent = g->lighting.light[i].spotExponent;
            a->lightingStack[a->lightingStackDepth].light[i].spotCutoff = g->lighting.light[i].spotCutoff;
            a->lightingStack[a->lightingStackDepth].light[i].constantAttenuation = g->lighting.light[i].constantAttenuation;
            a->lightingStack[a->lightingStackDepth].light[i].linearAttenuation = g->lighting.light[i].linearAttenuation;
            a->lightingStack[a->lightingStackDepth].light[i].quadraticAttenuation = g->lighting.light[i].quadraticAttenuation;
        }
        for (i = 0 ; i < 2 ; i++)
        {
            a->lightingStack[a->lightingStackDepth].ambient[i] = g->lighting.ambient[i];
            a->lightingStack[a->lightingStackDepth].diffuse[i] = g->lighting.diffuse[i];
            a->lightingStack[a->lightingStackDepth].specular[i] = g->lighting.specular[i];
            a->lightingStack[a->lightingStackDepth].emission[i] = g->lighting.emission[i];
            a->lightingStack[a->lightingStackDepth].shininess[i] = g->lighting.shininess[i];
            a->lightingStack[a->lightingStackDepth].indexes[i][0] = g->lighting.indexes[i][0];
            a->lightingStack[a->lightingStackDepth].indexes[i][1] = g->lighting.indexes[i][1];
            a->lightingStack[a->lightingStackDepth].indexes[i][2] = g->lighting.indexes[i][2];
        }
        a->lightingStack[a->lightingStackDepth].shadeModel = g->lighting.shadeModel;
        a->lightingStackDepth++;
    }
    if (mask & GL_LINE_BIT)
    {
        a->lineStack[a->lineStackDepth].lineSmooth = g->line.lineSmooth;
        a->lineStack[a->lineStackDepth].lineStipple = g->line.lineStipple;
        a->lineStack[a->lineStackDepth].pattern = g->line.pattern;
        a->lineStack[a->lineStackDepth].repeat = g->line.repeat;
        a->lineStack[a->lineStackDepth].width = g->line.width;
        a->lineStackDepth++;
    }
    if (mask & GL_LIST_BIT)
    {
        a->listStack[a->listStackDepth].base = g->lists.base;
        a->listStackDepth++;
    }
    if (mask & GL_PIXEL_MODE_BIT)
    {
        a->pixelModeStack[a->pixelModeStackDepth].bias = g->pixel.bias;
        a->pixelModeStack[a->pixelModeStackDepth].scale = g->pixel.scale;
        a->pixelModeStack[a->pixelModeStackDepth].indexOffset = g->pixel.indexOffset;
        a->pixelModeStack[a->pixelModeStackDepth].indexShift = g->pixel.indexShift;
        a->pixelModeStack[a->pixelModeStackDepth].mapColor = g->pixel.mapColor;
        a->pixelModeStack[a->pixelModeStackDepth].mapStencil = g->pixel.mapStencil;
        a->pixelModeStack[a->pixelModeStackDepth].xZoom = g->pixel.xZoom;
        a->pixelModeStack[a->pixelModeStackDepth].yZoom = g->pixel.yZoom;
        a->pixelModeStack[a->pixelModeStackDepth].readBuffer = g->buffer.readBuffer;
        a->pixelModeStackDepth++;
    }
    if (mask & GL_POINT_BIT)
    {
        a->pointStack[a->pointStackDepth].pointSmooth = g->point.pointSmooth;
#if CR_ARB_point_sprite
        a->pointStack[a->pointStackDepth].pointSprite = g->point.pointSprite;
        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++)
            a->pointStack[a->enableStackDepth].coordReplacement[i] = g->point.coordReplacement[i];
#endif
        a->pointStack[a->pointStackDepth].pointSize = g->point.pointSize;
        a->pointStackDepth++;
    }
    if (mask & GL_POLYGON_BIT)
    {
        a->polygonStack[a->polygonStackDepth].cullFace = g->polygon.cullFace;
        a->polygonStack[a->polygonStackDepth].cullFaceMode = g->polygon.cullFaceMode;
        a->polygonStack[a->polygonStackDepth].frontFace = g->polygon.frontFace;
        a->polygonStack[a->polygonStackDepth].frontMode = g->polygon.frontMode;
        a->polygonStack[a->polygonStackDepth].backMode = g->polygon.backMode;
        a->polygonStack[a->polygonStackDepth].polygonSmooth = g->polygon.polygonSmooth;
        a->polygonStack[a->polygonStackDepth].polygonStipple = g->polygon.polygonStipple;
        a->polygonStack[a->polygonStackDepth].polygonOffsetFill = g->polygon.polygonOffsetFill;
        a->polygonStack[a->polygonStackDepth].polygonOffsetLine = g->polygon.polygonOffsetLine;
        a->polygonStack[a->polygonStackDepth].polygonOffsetPoint = g->polygon.polygonOffsetPoint;
        a->polygonStack[a->polygonStackDepth].offsetFactor = g->polygon.offsetFactor;
        a->polygonStack[a->polygonStackDepth].offsetUnits = g->polygon.offsetUnits;
        a->polygonStackDepth++;
    }
    if (mask & GL_POLYGON_STIPPLE_BIT)
    {
        crMemcpy( a->polygonStippleStack[a->polygonStippleStackDepth].pattern, g->polygon.stipple, 32*sizeof(GLint) );
        a->polygonStippleStackDepth++;
    }
    if (mask & GL_SCISSOR_BIT)
    {
        a->scissorStack[a->scissorStackDepth].scissorTest = g->viewport.scissorTest;
        a->scissorStack[a->scissorStackDepth].scissorX = g->viewport.scissorX;
        a->scissorStack[a->scissorStackDepth].scissorY = g->viewport.scissorY;
        a->scissorStack[a->scissorStackDepth].scissorW = g->viewport.scissorW;
        a->scissorStack[a->scissorStackDepth].scissorH = g->viewport.scissorH;
        a->scissorStackDepth++;
    }
    if (mask & GL_STENCIL_BUFFER_BIT)
    {
        a->stencilBufferStack[a->stencilBufferStackDepth].stencilTest = g->stencil.stencilTest;
        a->stencilBufferStack[a->stencilBufferStackDepth].clearValue = g->stencil.clearValue;
        a->stencilBufferStack[a->stencilBufferStackDepth].writeMask = g->stencil.writeMask;
        for (i = 0; i < CRSTATE_STENCIL_BUFFER_COUNT; ++i)
        {
            a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].func = g->stencil.buffers[i].func;
            a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].mask = g->stencil.buffers[i].mask;
            a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].ref = g->stencil.buffers[i].ref;
            a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].fail = g->stencil.buffers[i].fail;
            a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].passDepthFail = g->stencil.buffers[i].passDepthFail;
            a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].passDepthPass = g->stencil.buffers[i].passDepthPass;
        }
        a->stencilBufferStackDepth++;
    }
    if (mask & GL_TEXTURE_BIT)
    {
        CRTextureStack *tState = a->textureStack + a->textureStackDepth;
        tState->curTextureUnit = g->texture.curTextureUnit;
        for (i = 0 ; i < g->limits.maxTextureUnits ; i++)
        {
            /* per-unit state */
            copy_texunit(&tState->unit[i], &g->texture.unit[i]);
            /* texture object state */
            copy_texobj(&tState->unit[i].Saved1D, g->texture.unit[i].currentTexture1D, GL_TRUE);
            copy_texobj(&tState->unit[i].Saved2D, g->texture.unit[i].currentTexture2D, GL_TRUE);
#ifdef CR_OPENGL_VERSION_1_2
            copy_texobj(&tState->unit[i].Saved3D, g->texture.unit[i].currentTexture3D, GL_TRUE);
#endif
#ifdef CR_ARB_texture_cube_map
            copy_texobj(&tState->unit[i].SavedCubeMap, g->texture.unit[i].currentTextureCubeMap, GL_TRUE);
#endif
#ifdef CR_NV_texture_rectangle
            copy_texobj(&tState->unit[i].SavedRect, g->texture.unit[i].currentTextureRect, GL_TRUE);
#endif
        }
        a->textureStackDepth++;
    }
    if (mask & GL_TRANSFORM_BIT)
    {
        if (a->transformStack[a->transformStackDepth].clip == NULL)
        {
            a->transformStack[a->transformStackDepth].clip = (GLboolean *) crCalloc( g->limits.maxClipPlanes * sizeof( GLboolean ));
        }
        if (a->transformStack[a->transformStackDepth].clipPlane == NULL)
        {
            a->transformStack[a->transformStackDepth].clipPlane = (GLvectord *) crCalloc( g->limits.maxClipPlanes * sizeof( GLvectord ));
        }
        a->transformStack[a->transformStackDepth].matrixMode = g->transform.matrixMode;
        for (i = 0 ; i < g->limits.maxClipPlanes ; i++)
        {
            a->transformStack[a->transformStackDepth].clip[i] = g->transform.clip[i];
            a->transformStack[a->transformStackDepth].clipPlane[i] = g->transform.clipPlane[i];
        }
        a->transformStack[a->transformStackDepth].normalize = g->transform.normalize;
#ifdef CR_OPENGL_VERSION_1_2
        a->transformStack[a->transformStackDepth].rescaleNormals = g->transform.rescaleNormals;
#endif
        a->transformStackDepth++;
    }
    if (mask & GL_VIEWPORT_BIT)
    {
        a->viewportStack[a->viewportStackDepth].viewportX = g->viewport.viewportX;
        a->viewportStack[a->viewportStackDepth].viewportY = g->viewport.viewportY;
        a->viewportStack[a->viewportStackDepth].viewportW = g->viewport.viewportW;
        a->viewportStack[a->viewportStackDepth].viewportH = g->viewport.viewportH;
        a->viewportStack[a->viewportStackDepth].nearClip = g->viewport.nearClip;
        a->viewportStack[a->viewportStackDepth].farClip = g->viewport.farClip;
        a->viewportStackDepth++;
    }

    DIRTY(ab->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePopAttrib(void) 
{
    CRContext *g = GetCurrentContext();
    CRAttribState *a = &(g->attrib);
    CRStateBits *sb = GetCurrentBits();
    CRAttribBits *ab = &(sb->attrib);
    CRbitvalue mask;
    unsigned int i;

    if (g->current.inBeginEnd)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glPopAttrib called in Begin/End");
        return;
    }

    if (a->attribStackDepth == 0)
    {
        crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty stack!" );
        return;
    }

    FLUSH();

    mask = a->pushMaskStack[--a->attribStackDepth];

    if (mask & GL_ACCUM_BUFFER_BIT)
    {
        if (a->accumBufferStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty accum buffer stack!" );
            return;
        }
        a->accumBufferStackDepth--;
        g->buffer.accumClearValue = a->accumBufferStack[a->accumBufferStackDepth].accumClearValue;
        DIRTY(sb->buffer.dirty, g->neg_bitid);
        DIRTY(sb->buffer.clearAccum, g->neg_bitid);
    }
    if (mask & GL_COLOR_BUFFER_BIT)
    {
        if (a->colorBufferStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty color buffer stack!" );
            return;
        }
        a->colorBufferStackDepth--;
        g->buffer.alphaTest = a->colorBufferStack[a->colorBufferStackDepth].alphaTest;
        g->buffer.alphaTestFunc = a->colorBufferStack[a->colorBufferStackDepth].alphaTestFunc;
        g->buffer.alphaTestRef = a->colorBufferStack[a->colorBufferStackDepth].alphaTestRef;
        g->buffer.blend = a->colorBufferStack[a->colorBufferStackDepth].blend;
        g->buffer.blendSrcRGB = a->colorBufferStack[a->colorBufferStackDepth].blendSrcRGB;
        g->buffer.blendDstRGB = a->colorBufferStack[a->colorBufferStackDepth].blendDstRGB;
#if defined(CR_EXT_blend_func_separate)
        g->buffer.blendSrcA = a->colorBufferStack[a->colorBufferStackDepth].blendSrcA;
        g->buffer.blendDstA = a->colorBufferStack[a->colorBufferStackDepth].blendDstA;
#endif
#ifdef CR_EXT_blend_color
        g->buffer.blendColor = a->colorBufferStack[a->colorBufferStackDepth].blendColor;
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
        g->buffer.blendEquation = a->colorBufferStack[a->colorBufferStackDepth].blendEquation;
#endif
        g->buffer.dither = a->colorBufferStack[a->colorBufferStackDepth].dither;
        g->buffer.drawBuffer = a->colorBufferStack[a->colorBufferStackDepth].drawBuffer;
        g->buffer.logicOp = a->colorBufferStack[a->colorBufferStackDepth].logicOp;
        g->buffer.indexLogicOp = a->colorBufferStack[a->colorBufferStackDepth].indexLogicOp;
        g->buffer.logicOpMode = a->colorBufferStack[a->colorBufferStackDepth].logicOpMode;
        g->buffer.colorClearValue = a->colorBufferStack[a->colorBufferStackDepth].colorClearValue;
        g->buffer.indexClearValue = a->colorBufferStack[a->colorBufferStackDepth].indexClearValue;
        g->buffer.colorWriteMask = a->colorBufferStack[a->colorBufferStackDepth].colorWriteMask;
        g->buffer.indexWriteMask = a->colorBufferStack[a->colorBufferStackDepth].indexWriteMask;
        DIRTY(sb->buffer.dirty, g->neg_bitid);
        DIRTY(sb->buffer.enable, g->neg_bitid);
        DIRTY(sb->buffer.alphaFunc, g->neg_bitid);
        DIRTY(sb->buffer.blendFunc, g->neg_bitid);
#ifdef CR_EXT_blend_color
        DIRTY(sb->buffer.blendColor, g->neg_bitid);
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
        DIRTY(sb->buffer.blendEquation, g->neg_bitid);
#endif
        DIRTY(sb->buffer.drawBuffer, g->neg_bitid);
        DIRTY(sb->buffer.logicOp, g->neg_bitid);
        DIRTY(sb->buffer.indexLogicOp, g->neg_bitid);
        DIRTY(sb->buffer.clearColor, g->neg_bitid);
        DIRTY(sb->buffer.clearIndex, g->neg_bitid);
        DIRTY(sb->buffer.colorWriteMask, g->neg_bitid);
        DIRTY(sb->buffer.indexMask, g->neg_bitid);
    }
    if (mask & GL_CURRENT_BIT)
    {
        if (a->currentStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty current stack!" );
            return;
        }
        a->currentStackDepth--;
        for (i = 0 ; i < CR_MAX_VERTEX_ATTRIBS ; i++)
        {
            COPY_4V(g->current.vertexAttrib[i], a->currentStack[a->currentStackDepth].attrib[i]);
            COPY_4V(g->current.rasterAttrib[i],  a->currentStack[a->currentStackDepth].rasterAttrib[i]);
            DIRTY(sb->current.vertexAttrib[i], g->neg_bitid);
        }
        g->current.rasterValid = a->currentStack[a->currentStackDepth].rasterValid;
        g->current.edgeFlag = a->currentStack[a->currentStackDepth].edgeFlag;
        g->current.colorIndex = a->currentStack[a->currentStackDepth].colorIndex;
        DIRTY(sb->current.dirty, g->neg_bitid);
        DIRTY(sb->current.edgeFlag, g->neg_bitid);
        DIRTY(sb->current.colorIndex, g->neg_bitid);
        DIRTY(sb->current.rasterPos, g->neg_bitid);
    }
    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        if (a->depthBufferStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty depth buffer stack!" );
            return;
        }
        a->depthBufferStackDepth--;
        g->buffer.depthTest = a->depthBufferStack[a->depthBufferStackDepth].depthTest;
        g->buffer.depthFunc = a->depthBufferStack[a->depthBufferStackDepth].depthFunc;
        g->buffer.depthClearValue = a->depthBufferStack[a->depthBufferStackDepth].depthClearValue;
        g->buffer.depthMask = a->depthBufferStack[a->depthBufferStackDepth].depthMask;
        DIRTY(sb->buffer.dirty, g->neg_bitid);
        DIRTY(sb->buffer.enable, g->neg_bitid);
        DIRTY(sb->buffer.depthFunc, g->neg_bitid);
        DIRTY(sb->buffer.clearDepth, g->neg_bitid);
        DIRTY(sb->buffer.depthMask, g->neg_bitid);
    }
    if (mask & GL_ENABLE_BIT)
    {
        if (a->enableStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty enable stack!" );
            return;
        }
        a->enableStackDepth--;
        g->buffer.alphaTest = a->enableStack[a->enableStackDepth].alphaTest;
        g->eval.autoNormal = a->enableStack[a->enableStackDepth].autoNormal;
        g->buffer.blend = a->enableStack[a->enableStackDepth].blend;
        for (i = 0 ; i < g->limits.maxClipPlanes ; i++)
        {
            g->transform.clip[i] = a->enableStack[a->enableStackDepth].clip[i];
        }
        g->lighting.colorMaterial = a->enableStack[a->enableStackDepth].colorMaterial;
        g->polygon.cullFace = a->enableStack[a->enableStackDepth].cullFace;
        g->buffer.depthTest = a->enableStack[a->enableStackDepth].depthTest;
        g->buffer.dither = a->enableStack[a->enableStackDepth].dither;
        g->fog.enable = a->enableStack[a->enableStackDepth].fog;
        for (i = 0 ; i < g->limits.maxLights ; i++)
        {
            g->lighting.light[i].enable = a->enableStack[a->enableStackDepth].light[i];
        }
        g->lighting.lighting = a->enableStack[a->enableStackDepth].lighting;
        g->line.lineSmooth = a->enableStack[a->enableStackDepth].lineSmooth;
        g->line.lineStipple = a->enableStack[a->enableStackDepth].lineStipple;
        g->buffer.logicOp = a->enableStack[a->enableStackDepth].logicOp;
        g->buffer.indexLogicOp = a->enableStack[a->enableStackDepth].indexLogicOp;
        for (i = 0 ; i < GLEVAL_TOT ; i++)
        {
            g->eval.enable1D[i] = a->enableStack[a->enableStackDepth].map1[i];
            g->eval.enable2D[i] = a->enableStack[a->enableStackDepth].map2[i];
        }
        g->transform.normalize = a->enableStack[a->enableStackDepth].normalize;
        g->point.pointSmooth = a->enableStack[a->enableStackDepth].pointSmooth;
#ifdef CR_ARB_point_sprite
        g->point.pointSprite = a->enableStack[a->enableStackDepth].pointSprite;
        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++)
            g->point.coordReplacement[i] = a->enableStack[a->enableStackDepth].coordReplacement[i];
#endif
        g->polygon.polygonOffsetLine = a->enableStack[a->enableStackDepth].polygonOffsetLine;
        g->polygon.polygonOffsetFill = a->enableStack[a->enableStackDepth].polygonOffsetFill;
        g->polygon.polygonOffsetPoint = a->enableStack[a->enableStackDepth].polygonOffsetPoint;
        g->polygon.polygonSmooth = a->enableStack[a->enableStackDepth].polygonSmooth;
        g->polygon.polygonStipple = a->enableStack[a->enableStackDepth].polygonStipple;
#ifdef CR_OPENGL_VERSION_1_2
        g->transform.rescaleNormals = a->enableStack[a->enableStackDepth].rescaleNormals;
#endif
        g->viewport.scissorTest = a->enableStack[a->enableStackDepth].scissorTest;
        g->stencil.stencilTest = a->enableStack[a->enableStackDepth].stencilTest;
        for (i = 0 ; i < g->limits.maxTextureUnits; i++)
        {
            g->texture.unit[i].enabled1D = a->enableStack[a->enableStackDepth].texture1D[i];
            g->texture.unit[i].enabled2D = a->enableStack[a->enableStackDepth].texture2D[i];
#ifdef CR_OPENGL_VERSION_1_2
            g->texture.unit[i].enabled3D = a->enableStack[a->enableStackDepth].texture3D[i];
#endif
#ifdef CR_ARB_texture_cube_map
            g->texture.unit[i].enabledCubeMap = a->enableStack[a->enableStackDepth].textureCubeMap[i];
#endif
#ifdef CR_NV_texture_rectangle
            g->texture.unit[i].enabledRect = a->enableStack[a->enableStackDepth].textureRect[i];
#endif
            g->texture.unit[i].textureGen.s = a->enableStack[a->enableStackDepth].textureGenS[i];
            g->texture.unit[i].textureGen.t = a->enableStack[a->enableStackDepth].textureGenT[i];
            g->texture.unit[i].textureGen.r = a->enableStack[a->enableStackDepth].textureGenR[i];
            g->texture.unit[i].textureGen.q = a->enableStack[a->enableStackDepth].textureGenQ[i];
        }
        DIRTY(sb->buffer.dirty, g->neg_bitid);
        DIRTY(sb->eval.dirty, g->neg_bitid);
        DIRTY(sb->transform.dirty, g->neg_bitid);
        DIRTY(sb->lighting.dirty, g->neg_bitid);
        DIRTY(sb->fog.dirty, g->neg_bitid);
        DIRTY(sb->line.dirty, g->neg_bitid);
        DIRTY(sb->polygon.dirty, g->neg_bitid);
        DIRTY(sb->viewport.dirty, g->neg_bitid);
        DIRTY(sb->stencil.dirty, g->neg_bitid);
        DIRTY(sb->texture.dirty, g->neg_bitid);

        DIRTY(sb->buffer.enable, g->neg_bitid);
        DIRTY(sb->eval.enable, g->neg_bitid);
        DIRTY(sb->transform.enable, g->neg_bitid);
        DIRTY(sb->lighting.enable, g->neg_bitid);
        DIRTY(sb->fog.enable, g->neg_bitid);
        DIRTY(sb->line.enable, g->neg_bitid);
        DIRTY(sb->polygon.enable, g->neg_bitid);
        DIRTY(sb->viewport.enable, g->neg_bitid);
        DIRTY(sb->stencil.enable, g->neg_bitid);
        for (i = 0 ; i < g->limits.maxTextureUnits ; i++)
        {
            DIRTY(sb->texture.enable[i], g->neg_bitid);
        }
    }
    if (mask & GL_EVAL_BIT)
    {
        if (a->evalStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty eval stack!" );
            return;
        }
        a->evalStackDepth--;
        for (i = 0 ; i < GLEVAL_TOT ; i++)
        {
            int size1 = a->evalStack[a->evalStackDepth].eval1D[i].order * gleval_sizes[i] * sizeof(GLfloat);
            int size2 = a->evalStack[a->evalStackDepth].eval2D[i].uorder * a->evalStack[a->evalStackDepth].eval2D[i].vorder * gleval_sizes[i] * sizeof (GLfloat);
            g->eval.enable1D[i] = a->evalStack[a->evalStackDepth].enable1D[i];
            g->eval.enable2D[i] = a->evalStack[a->evalStackDepth].enable2D[i];
            g->eval.eval1D[i].u1 = a->evalStack[a->evalStackDepth].eval1D[i].u1;
            g->eval.eval1D[i].u2 = a->evalStack[a->evalStackDepth].eval1D[i].u2;
            g->eval.eval1D[i].order = a->evalStack[a->evalStackDepth].eval1D[i].order;
            crMemcpy((char*)g->eval.eval1D[i].coeff, a->evalStack[a->evalStackDepth].eval1D[i].coeff, size1);
            crFree(a->evalStack[a->evalStackDepth].eval1D[i].coeff);
            a->evalStack[a->evalStackDepth].eval1D[i].coeff = NULL;
            g->eval.eval2D[i].u1 = a->evalStack[a->evalStackDepth].eval2D[i].u1;
            g->eval.eval2D[i].u2 = a->evalStack[a->evalStackDepth].eval2D[i].u2;
            g->eval.eval2D[i].v1 = a->evalStack[a->evalStackDepth].eval2D[i].v1;
            g->eval.eval2D[i].v2 = a->evalStack[a->evalStackDepth].eval2D[i].v2;
            g->eval.eval2D[i].uorder = a->evalStack[a->evalStackDepth].eval2D[i].uorder;
            g->eval.eval2D[i].vorder = a->evalStack[a->evalStackDepth].eval2D[i].vorder;
            crMemcpy((char*)g->eval.eval2D[i].coeff, a->evalStack[a->evalStackDepth].eval2D[i].coeff, size2);
            crFree(a->evalStack[a->evalStackDepth].eval2D[i].coeff);
            a->evalStack[a->evalStackDepth].eval2D[i].coeff = NULL;
        }
        g->eval.autoNormal = a->evalStack[a->evalStackDepth].autoNormal;
        g->eval.un1D = a->evalStack[a->evalStackDepth].un1D;
        g->eval.u11D = a->evalStack[a->evalStackDepth].u11D;
        g->eval.u21D = a->evalStack[a->evalStackDepth].u21D;
        g->eval.un2D = a->evalStack[a->evalStackDepth].un2D;
        g->eval.u12D = a->evalStack[a->evalStackDepth].u12D;
        g->eval.u22D = a->evalStack[a->evalStackDepth].u22D;
        g->eval.vn2D = a->evalStack[a->evalStackDepth].vn2D;
        g->eval.v12D = a->evalStack[a->evalStackDepth].v12D;
        g->eval.v22D = a->evalStack[a->evalStackDepth].v22D;
        for (i = 0; i < GLEVAL_TOT; i++) {
            DIRTY(sb->eval.eval1D[i], g->neg_bitid);
            DIRTY(sb->eval.eval2D[i], g->neg_bitid);
        }
        DIRTY(sb->eval.dirty, g->neg_bitid);
        DIRTY(sb->eval.grid1D, g->neg_bitid);
        DIRTY(sb->eval.grid2D, g->neg_bitid);
        DIRTY(sb->eval.enable, g->neg_bitid);
    }
    if (mask & GL_FOG_BIT)
    {
        if (a->fogStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty fog stack!" );
            return;
        }
        a->fogStackDepth--;
        g->fog.enable = a->fogStack[a->fogStackDepth].enable;
        g->fog.color = a->fogStack[a->fogStackDepth].color;
        g->fog.density = a->fogStack[a->fogStackDepth].density;
        g->fog.start = a->fogStack[a->fogStackDepth].start;
        g->fog.end = a->fogStack[a->fogStackDepth].end;
        g->fog.index = a->fogStack[a->fogStackDepth].index;
        g->fog.mode = a->fogStack[a->fogStackDepth].mode;
        DIRTY(sb->fog.dirty, g->neg_bitid);
        DIRTY(sb->fog.color, g->neg_bitid);
        DIRTY(sb->fog.index, g->neg_bitid);
        DIRTY(sb->fog.density, g->neg_bitid);
        DIRTY(sb->fog.start, g->neg_bitid);
        DIRTY(sb->fog.end, g->neg_bitid);
        DIRTY(sb->fog.mode, g->neg_bitid);
        DIRTY(sb->fog.enable, g->neg_bitid);
    }
    if (mask & GL_HINT_BIT)
    {
        if (a->hintStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty hint stack!" );
            return;
        }
        a->hintStackDepth--;
        g->hint.perspectiveCorrection = a->hintStack[a->hintStackDepth].perspectiveCorrection;
        g->hint.pointSmooth = a->hintStack[a->hintStackDepth].pointSmooth;
        g->hint.lineSmooth = a->hintStack[a->hintStackDepth].lineSmooth;
        g->hint.polygonSmooth = a->hintStack[a->hintStackDepth].polygonSmooth;
        g->hint.fog = a->hintStack[a->hintStackDepth].fog;
        DIRTY(sb->hint.dirty, g->neg_bitid);
        DIRTY(sb->hint.perspectiveCorrection, g->neg_bitid);
        DIRTY(sb->hint.pointSmooth, g->neg_bitid);
        DIRTY(sb->hint.lineSmooth, g->neg_bitid);
        DIRTY(sb->hint.polygonSmooth, g->neg_bitid);
#ifdef CR_EXT_clip_volume_hint
        g->hint.clipVolumeClipping = a->hintStack[a->hintStackDepth].clipVolumeClipping;
        DIRTY(sb->hint.clipVolumeClipping, g->neg_bitid);
#endif
#ifdef CR_ARB_texture_compression
        g->hint.textureCompression = a->hintStack[a->hintStackDepth].textureCompression;
        DIRTY(sb->hint.textureCompression, g->neg_bitid);
#endif
#ifdef CR_SGIS_generate_mipmap
        g->hint.generateMipmap = a->hintStack[a->hintStackDepth].generateMipmap;
        DIRTY(sb->hint.generateMipmap, g->neg_bitid);
#endif
    }
    if (mask & GL_LIGHTING_BIT)
    {
        if (a->lightingStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty lighting stack!" );
            return;
        }
        a->lightingStackDepth--;
        g->lighting.lightModelAmbient = a->lightingStack[a->lightingStackDepth].lightModelAmbient;
        g->lighting.lightModelLocalViewer = a->lightingStack[a->lightingStackDepth].lightModelLocalViewer;
        g->lighting.lightModelTwoSide = a->lightingStack[a->lightingStackDepth].lightModelTwoSide;
#if defined(CR_EXT_separate_specular_color) || defined(CR_OPENGL_VERSION_1_2)
        g->lighting.lightModelColorControlEXT = a->lightingStack[a->lightingStackDepth].lightModelColorControlEXT;
#endif
        g->lighting.lighting = a->lightingStack[a->lightingStackDepth].lighting;
        g->lighting.colorMaterial = a->lightingStack[a->lightingStackDepth].colorMaterial;
        g->lighting.colorMaterialMode = a->lightingStack[a->lightingStackDepth].colorMaterialMode;
        g->lighting.colorMaterialFace = a->lightingStack[a->lightingStackDepth].colorMaterialFace;
        for (i = 0 ; i < g->limits.maxLights; i++)
        {
            g->lighting.light[i].enable = a->lightingStack[a->lightingStackDepth].light[i].enable;
            g->lighting.light[i].ambient = a->lightingStack[a->lightingStackDepth].light[i].ambient;
            g->lighting.light[i].diffuse = a->lightingStack[a->lightingStackDepth].light[i].diffuse;
            g->lighting.light[i].specular = a->lightingStack[a->lightingStackDepth].light[i].specular;
            g->lighting.light[i].spotDirection = a->lightingStack[a->lightingStackDepth].light[i].spotDirection;
            g->lighting.light[i].position = a->lightingStack[a->lightingStackDepth].light[i].position;
            g->lighting.light[i].spotExponent = a->lightingStack[a->lightingStackDepth].light[i].spotExponent;
            g->lighting.light[i].spotCutoff = a->lightingStack[a->lightingStackDepth].light[i].spotCutoff;
            g->lighting.light[i].constantAttenuation = a->lightingStack[a->lightingStackDepth].light[i].constantAttenuation;
            g->lighting.light[i].linearAttenuation = a->lightingStack[a->lightingStackDepth].light[i].linearAttenuation;
            g->lighting.light[i].quadraticAttenuation = a->lightingStack[a->lightingStackDepth].light[i].quadraticAttenuation;
        }
        for (i = 0 ; i < 2 ; i++)
        {
            g->lighting.ambient[i] = a->lightingStack[a->lightingStackDepth].ambient[i];
            g->lighting.diffuse[i] = a->lightingStack[a->lightingStackDepth].diffuse[i];
            g->lighting.specular[i] = a->lightingStack[a->lightingStackDepth].specular[i];
            g->lighting.emission[i] = a->lightingStack[a->lightingStackDepth].emission[i];
            g->lighting.shininess[i] = a->lightingStack[a->lightingStackDepth].shininess[i];
            g->lighting.indexes[i][0] = a->lightingStack[a->lightingStackDepth].indexes[i][0];
            g->lighting.indexes[i][1] = a->lightingStack[a->lightingStackDepth].indexes[i][1];
            g->lighting.indexes[i][2] = a->lightingStack[a->lightingStackDepth].indexes[i][2];
        }
        g->lighting.shadeModel = a->lightingStack[a->lightingStackDepth].shadeModel;
        DIRTY(sb->lighting.dirty, g->neg_bitid);
        DIRTY(sb->lighting.shadeModel, g->neg_bitid);
        DIRTY(sb->lighting.lightModel, g->neg_bitid);
        DIRTY(sb->lighting.material, g->neg_bitid);
        DIRTY(sb->lighting.enable, g->neg_bitid);
        for (i = 0 ; i < g->limits.maxLights; i++)
        {
            DIRTY(sb->lighting.light[i].dirty, g->neg_bitid);
            DIRTY(sb->lighting.light[i].enable, g->neg_bitid);
            DIRTY(sb->lighting.light[i].ambient, g->neg_bitid);
            DIRTY(sb->lighting.light[i].diffuse, g->neg_bitid);
            DIRTY(sb->lighting.light[i].specular, g->neg_bitid);
            DIRTY(sb->lighting.light[i].position, g->neg_bitid);
            DIRTY(sb->lighting.light[i].attenuation, g->neg_bitid);
            DIRTY(sb->lighting.light[i].spot, g->neg_bitid);
        }
    }
    if (mask & GL_LINE_BIT)
    {
        if (a->lineStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty line stack!" );
            return;
        }
        a->lineStackDepth--;
        g->line.lineSmooth = a->lineStack[a->lineStackDepth].lineSmooth;
        g->line.lineStipple = a->lineStack[a->lineStackDepth].lineStipple;
        g->line.pattern = a->lineStack[a->lineStackDepth].pattern;
        g->line.repeat = a->lineStack[a->lineStackDepth].repeat;
        g->line.width = a->lineStack[a->lineStackDepth].width;
        DIRTY(sb->line.dirty, g->neg_bitid);
        DIRTY(sb->line.enable, g->neg_bitid);
        DIRTY(sb->line.width, g->neg_bitid);
        DIRTY(sb->line.stipple, g->neg_bitid);
    }
    if (mask & GL_LIST_BIT)
    {
        if (a->listStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty list stack!" );
            return;
        }
        a->listStackDepth--;
        g->lists.base = a->listStack[a->listStackDepth].base;
        DIRTY(sb->lists.dirty, g->neg_bitid);
    }
    if (mask & GL_PIXEL_MODE_BIT)
    {
        if (a->pixelModeStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty pixel mode stack!" );
            return;
        }
        a->pixelModeStackDepth--;
        g->pixel.bias = a->pixelModeStack[a->pixelModeStackDepth].bias;
        g->pixel.scale = a->pixelModeStack[a->pixelModeStackDepth].scale;
        g->pixel.indexOffset = a->pixelModeStack[a->pixelModeStackDepth].indexOffset;
        g->pixel.indexShift = a->pixelModeStack[a->pixelModeStackDepth].indexShift;
        g->pixel.mapColor = a->pixelModeStack[a->pixelModeStackDepth].mapColor;
        g->pixel.mapStencil = a->pixelModeStack[a->pixelModeStackDepth].mapStencil;
        g->pixel.xZoom = a->pixelModeStack[a->pixelModeStackDepth].xZoom;
        g->pixel.yZoom = a->pixelModeStack[a->pixelModeStackDepth].yZoom;
        g->buffer.readBuffer = a->pixelModeStack[a->pixelModeStackDepth].readBuffer;
        DIRTY(sb->pixel.dirty, g->neg_bitid);
        DIRTY(sb->pixel.transfer, g->neg_bitid);
        DIRTY(sb->pixel.zoom, g->neg_bitid);
        DIRTY(sb->buffer.dirty, g->neg_bitid);
        DIRTY(sb->buffer.readBuffer, g->neg_bitid);
    }
    if (mask & GL_POINT_BIT)
    {
        if (a->pointStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty point stack!" );
            return;
        }
        a->pointStackDepth--;
        g->point.pointSmooth = a->pointStack[a->pointStackDepth].pointSmooth;
        g->point.pointSize = a->pointStack[a->pointStackDepth].pointSize;
#if GL_ARB_point_sprite
        g->point.pointSprite = a->pointStack[a->pointStackDepth].pointSprite;
        DIRTY(sb->point.enableSprite, g->neg_bitid);
        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++) {
            g->point.coordReplacement[i] = a->enableStack[a->enableStackDepth].coordReplacement[i];
            DIRTY(sb->point.coordReplacement[i], g->neg_bitid);
        }
#endif
        DIRTY(sb->point.dirty, g->neg_bitid);
        DIRTY(sb->point.size, g->neg_bitid);
        DIRTY(sb->point.enableSmooth, g->neg_bitid);
    }
    if (mask & GL_POLYGON_BIT)
    {
        if (a->polygonStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty polygon stack!" );
            return;
        }
        a->polygonStackDepth--;
        g->polygon.cullFace = a->polygonStack[a->polygonStackDepth].cullFace;
        g->polygon.cullFaceMode = a->polygonStack[a->polygonStackDepth].cullFaceMode;
        g->polygon.frontFace = a->polygonStack[a->polygonStackDepth].frontFace;
        g->polygon.frontMode = a->polygonStack[a->polygonStackDepth].frontMode;
        g->polygon.backMode = a->polygonStack[a->polygonStackDepth].backMode;
        g->polygon.polygonSmooth = a->polygonStack[a->polygonStackDepth].polygonSmooth;
        g->polygon.polygonStipple = a->polygonStack[a->polygonStackDepth].polygonStipple;
        g->polygon.polygonOffsetFill = a->polygonStack[a->polygonStackDepth].polygonOffsetFill;
        g->polygon.polygonOffsetLine = a->polygonStack[a->polygonStackDepth].polygonOffsetLine;
        g->polygon.polygonOffsetPoint = a->polygonStack[a->polygonStackDepth].polygonOffsetPoint;
        g->polygon.offsetFactor = a->polygonStack[a->polygonStackDepth].offsetFactor;
        g->polygon.offsetUnits = a->polygonStack[a->polygonStackDepth].offsetUnits;
        DIRTY(sb->polygon.dirty, g->neg_bitid);
        DIRTY(sb->polygon.enable, g->neg_bitid);
        DIRTY(sb->polygon.offset, g->neg_bitid);
        DIRTY(sb->polygon.mode, g->neg_bitid);
        DIRTY(sb->polygon.stipple, g->neg_bitid);
    }
    if (mask & GL_POLYGON_STIPPLE_BIT)
    {
        if (a->polygonStippleStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty polygon stipple stack!" );
            return;
        }
        a->polygonStippleStackDepth--;
        crMemcpy( g->polygon.stipple, a->polygonStippleStack[a->polygonStippleStackDepth].pattern, 32*sizeof(GLint) );
        DIRTY(sb->polygon.dirty, g->neg_bitid);
        DIRTY(sb->polygon.stipple, g->neg_bitid);
    }
    if (mask & GL_SCISSOR_BIT)
    {
        if (a->scissorStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty scissor stack!" );
            return;
        }
        a->scissorStackDepth--;
        g->viewport.scissorTest = a->scissorStack[a->scissorStackDepth].scissorTest;
        g->viewport.scissorX = a->scissorStack[a->scissorStackDepth].scissorX;
        g->viewport.scissorY = a->scissorStack[a->scissorStackDepth].scissorY;
        g->viewport.scissorW = a->scissorStack[a->scissorStackDepth].scissorW;
        g->viewport.scissorH = a->scissorStack[a->scissorStackDepth].scissorH;
        DIRTY(sb->viewport.dirty, g->neg_bitid);
        DIRTY(sb->viewport.enable, g->neg_bitid);
        DIRTY(sb->viewport.s_dims, g->neg_bitid);
    }
    if (mask & GL_STENCIL_BUFFER_BIT)
    {
        if (a->stencilBufferStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty stencil stack!" );
            return;
        }
        a->stencilBufferStackDepth--;
        g->stencil.stencilTest = a->stencilBufferStack[a->stencilBufferStackDepth].stencilTest;
        g->stencil.clearValue = a->stencilBufferStack[a->stencilBufferStackDepth].clearValue;
        g->stencil.writeMask = a->stencilBufferStack[a->stencilBufferStackDepth].writeMask;
        for (i = 0; i < CRSTATE_STENCIL_BUFFER_COUNT; ++i)
        {
            g->stencil.buffers[i].func = a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].func;
            g->stencil.buffers[i].mask = a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].mask;
            g->stencil.buffers[i].ref = a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].ref;
            g->stencil.buffers[i].fail = a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].fail;
            g->stencil.buffers[i].passDepthFail = a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].passDepthFail;
            g->stencil.buffers[i].passDepthPass = a->stencilBufferStack[a->stencilBufferStackDepth].buffers[i].passDepthPass;
        }

        DIRTY(sb->stencil.dirty, g->neg_bitid);
        DIRTY(sb->stencil.enable, g->neg_bitid);
        DIRTY(sb->stencil.clearValue, g->neg_bitid);
        DIRTY(sb->stencil.writeMask, g->neg_bitid);

        for (i = 0; i < CRSTATE_STENCIL_BUFFER_REF_COUNT; ++i)
        {
            DIRTY(sb->stencil.bufferRefs[i].func, g->neg_bitid);
            DIRTY(sb->stencil.bufferRefs[i].op, g->neg_bitid);
        }
    }
    if (mask & GL_TEXTURE_BIT)
    {
        CRTextureStack *tState;
        if (a->textureStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty texture stack!" );
            return;
        }
        a->textureStackDepth--;
        tState = a->textureStack + a->textureStackDepth;

        g->texture.curTextureUnit = tState->curTextureUnit;
        for (i = 0 ; i < g->limits.maxTextureUnits ; i++)
        {
            copy_texunit(&g->texture.unit[i], &tState->unit[i]);
            /* first, restore the bindings! */
            g->texture.unit[i].currentTexture1D = crStateTextureGet(GL_TEXTURE_1D, tState->unit[i].Saved1D.id);
            copy_texobj(g->texture.unit[i].currentTexture1D, &tState->unit[i].Saved1D, GL_FALSE);
            g->texture.unit[i].currentTexture2D = crStateTextureGet(GL_TEXTURE_2D, tState->unit[i].Saved2D.id);
            copy_texobj(g->texture.unit[i].currentTexture2D, &tState->unit[i].Saved2D, GL_FALSE);
#ifdef CR_OPENGL_VERSION_1_2
            g->texture.unit[i].currentTexture3D = crStateTextureGet(GL_TEXTURE_3D, tState->unit[i].Saved3D.id);
            copy_texobj(g->texture.unit[i].currentTexture3D, &tState->unit[i].Saved3D, GL_FALSE);
#endif
#ifdef CR_ARB_texture_cube_map
            g->texture.unit[i].currentTextureCubeMap = crStateTextureGet(GL_TEXTURE_CUBE_MAP_ARB, tState->unit[i].SavedCubeMap.id);
            copy_texobj(g->texture.unit[i].currentTextureCubeMap, &tState->unit[i].SavedCubeMap, GL_FALSE);
#endif
#ifdef CR_NV_texture_rectangle
            g->texture.unit[i].currentTextureRect = crStateTextureGet(GL_TEXTURE_CUBE_MAP_ARB, tState->unit[i].SavedRect.id);
            copy_texobj(g->texture.unit[i].currentTextureRect, &tState->unit[i].SavedRect, GL_FALSE);
#endif
        }
        DIRTY(sb->texture.dirty, g->neg_bitid);
        for (i = 0 ; i < g->limits.maxTextureUnits ; i++)
        {
            DIRTY(sb->texture.enable[i], g->neg_bitid);
            DIRTY(sb->texture.current[i], g->neg_bitid);
            DIRTY(sb->texture.objGen[i], g->neg_bitid);
            DIRTY(sb->texture.eyeGen[i], g->neg_bitid);
            DIRTY(sb->texture.envBit[i], g->neg_bitid);
            DIRTY(sb->texture.genMode[i], g->neg_bitid);
        }

        for (i = 0 ; i < g->limits.maxTextureUnits ; i++)
        {
            DIRTY(g->texture.unit[i].currentTexture1D->dirty, g->neg_bitid);
            DIRTY(g->texture.unit[i].currentTexture2D->dirty, g->neg_bitid);
            DIRTY(g->texture.unit[i].currentTexture3D->dirty, g->neg_bitid);
#ifdef CR_ARB_texture_cube_map
            DIRTY(g->texture.unit[i].currentTextureCubeMap->dirty, g->neg_bitid);
#endif
#ifdef CR_NV_texture_rectangle
            DIRTY(g->texture.unit[i].currentTextureRect->dirty, g->neg_bitid);
#endif
            DIRTY(g->texture.unit[i].currentTexture1D->paramsBit[i], g->neg_bitid);
            DIRTY(g->texture.unit[i].currentTexture2D->paramsBit[i], g->neg_bitid);
            DIRTY(g->texture.unit[i].currentTexture3D->paramsBit[i], g->neg_bitid);
#ifdef CR_ARB_texture_cube_map
            DIRTY(g->texture.unit[i].currentTextureCubeMap->paramsBit[i], g->neg_bitid);
#endif
#ifdef CR_NV_texture_rectangle
            DIRTY(g->texture.unit[i].currentTextureRect->paramsBit[i], g->neg_bitid);
#endif
        }
    }
    if (mask & GL_TRANSFORM_BIT)
    {
        if (a->transformStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty transform stack!" );
            return;
        }
        a->transformStackDepth--;
        g->transform.matrixMode = a->transformStack[a->transformStackDepth].matrixMode;
        crStateMatrixMode(g->transform.matrixMode);
        for (i = 0 ; i < g->limits.maxClipPlanes ; i++)
        {
            g->transform.clip[i] = a->transformStack[a->transformStackDepth].clip[i];
            g->transform.clipPlane[i] = a->transformStack[a->transformStackDepth].clipPlane[i];
        }
        g->transform.normalize = a->transformStack[a->transformStackDepth].normalize;
#ifdef CR_OPENGL_VERSION_1_2
        g->transform.rescaleNormals = a->transformStack[a->transformStackDepth].rescaleNormals;
#endif
        DIRTY(sb->transform.dirty, g->neg_bitid);
        DIRTY(sb->transform.matrixMode, g->neg_bitid);
        DIRTY(sb->transform.clipPlane, g->neg_bitid);
        DIRTY(sb->transform.enable, g->neg_bitid);
    }
    if (mask & GL_VIEWPORT_BIT)
    {
        if (a->viewportStackDepth == 0)
        {
            crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW, "glPopAttrib called with an empty viewport stack!" );
            return;
        }
        a->viewportStackDepth--;
        g->viewport.viewportX = a->viewportStack[a->viewportStackDepth].viewportX;
        g->viewport.viewportY = a->viewportStack[a->viewportStackDepth].viewportY;
        g->viewport.viewportW = a->viewportStack[a->viewportStackDepth].viewportW;
        g->viewport.viewportH = a->viewportStack[a->viewportStackDepth].viewportH;
        g->viewport.nearClip = a->viewportStack[a->viewportStackDepth].nearClip;
        g->viewport.farClip = a->viewportStack[a->viewportStackDepth].farClip;
        DIRTY(sb->viewport.dirty, g->neg_bitid);
        DIRTY(sb->viewport.v_dims, g->neg_bitid);
        DIRTY(sb->viewport.depth, g->neg_bitid);
    }
    DIRTY(ab->dirty, g->neg_bitid);
}

void crStateAttribSwitch( CRAttribBits *bb, CRbitvalue *bitID,
                          CRContext *fromCtx, CRContext *toCtx )
{
    CRAttribState *to = &(toCtx->attrib);
    CRAttribState *from = &(fromCtx->attrib);
    if (to->attribStackDepth != 0 || from->attribStackDepth != 0)
    {
        crWarning( "Trying to switch contexts when the attribute stacks "
                             "weren't empty.  Currently, this is not supported." );
    }
    (void) bb;
    (void) bitID;
}
