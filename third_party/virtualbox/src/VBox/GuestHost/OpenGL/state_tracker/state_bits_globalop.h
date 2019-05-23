/* $Id: state_bits_globalop.h $ */

/** @file
 * Global State bits operation
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/cdefs.h>

#include <cr_version.h>

#ifndef CRSTATE_BITS_OP
# error "CRSTATE_BITS_OP must be defined!"
#endif

#define _CRSTATE_BITS_OP_SIZEOF(_val) CRSTATE_BITS_OP(_val, RT_SIZEOFMEMB(CRStateBits, _val))

#ifndef CRSTATE_BITS_OP_VERSION
# define CRSTATE_BITS_OP_VERSION SHCROGL_SSM_VERSION
#endif

do {
int i;
#ifdef _CRSTATE_BITS_OP_STENCIL_V_33
# error "_CRSTATE_BITS_OP_STENCIL_V_33 must no be defined!"
#endif
#if CRSTATE_BITS_OP_VERSION < SHCROGL_SSM_VERSION_WITH_FIXED_STENCIL
# define _CRSTATE_BITS_OP_STENCIL_V_33
#endif

#ifdef _CRSTATE_BITS_OP_STENCIL_V_33
# ifndef CRSTATE_BITS_OP_STENCIL_OP_V_33
#  error "CRSTATE_BITS_OP_STENCIL_OP_V_33 undefined!"
# endif
# ifndef CRSTATE_BITS_OP_STENCIL_FUNC_V_33
#  error "CRSTATE_BITS_OP_STENCIL_FUNC_V_33 undefined!"
# endif
#endif

_CRSTATE_BITS_OP_SIZEOF(attrib.dirty);

_CRSTATE_BITS_OP_SIZEOF(buffer.dirty);
_CRSTATE_BITS_OP_SIZEOF(buffer.enable);
_CRSTATE_BITS_OP_SIZEOF(buffer.alphaFunc);
_CRSTATE_BITS_OP_SIZEOF(buffer.depthFunc);
_CRSTATE_BITS_OP_SIZEOF(buffer.blendFunc);
_CRSTATE_BITS_OP_SIZEOF(buffer.logicOp);
_CRSTATE_BITS_OP_SIZEOF(buffer.indexLogicOp);
_CRSTATE_BITS_OP_SIZEOF(buffer.drawBuffer);
_CRSTATE_BITS_OP_SIZEOF(buffer.readBuffer);
_CRSTATE_BITS_OP_SIZEOF(buffer.indexMask);
_CRSTATE_BITS_OP_SIZEOF(buffer.colorWriteMask);
_CRSTATE_BITS_OP_SIZEOF(buffer.clearColor);
_CRSTATE_BITS_OP_SIZEOF(buffer.clearIndex);
_CRSTATE_BITS_OP_SIZEOF(buffer.clearDepth);
_CRSTATE_BITS_OP_SIZEOF(buffer.clearAccum);
_CRSTATE_BITS_OP_SIZEOF(buffer.depthMask);
#ifdef CR_EXT_blend_color
_CRSTATE_BITS_OP_SIZEOF(buffer.blendColor);
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
_CRSTATE_BITS_OP_SIZEOF(buffer.blendEquation);
#endif
#if defined(CR_EXT_blend_func_separate)
_CRSTATE_BITS_OP_SIZEOF(buffer.blendFuncSeparate);
#endif

#ifdef CR_ARB_vertex_buffer_object
_CRSTATE_BITS_OP_SIZEOF(bufferobject.dirty);
_CRSTATE_BITS_OP_SIZEOF(bufferobject.arrayBinding);
_CRSTATE_BITS_OP_SIZEOF(bufferobject.elementsBinding);
# ifdef CR_ARB_pixel_buffer_object
_CRSTATE_BITS_OP_SIZEOF(bufferobject.packBinding);
_CRSTATE_BITS_OP_SIZEOF(bufferobject.unpackBinding);
# endif
#endif

_CRSTATE_BITS_OP_SIZEOF(client.dirty);
_CRSTATE_BITS_OP_SIZEOF(client.pack);
_CRSTATE_BITS_OP_SIZEOF(client.unpack);
_CRSTATE_BITS_OP_SIZEOF(client.enableClientState);
_CRSTATE_BITS_OP_SIZEOF(client.clientPointer);
CRSTATE_BITS_OP(client.v, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
CRSTATE_BITS_OP(client.n, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
CRSTATE_BITS_OP(client.c, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
CRSTATE_BITS_OP(client.i, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
CRSTATE_BITS_OP(client.e, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
CRSTATE_BITS_OP(client.s, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
CRSTATE_BITS_OP(client.f, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
for (i=0; i<CR_MAX_TEXTURE_UNITS; i++)
{
    CRSTATE_BITS_OP(client.t[i], GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
}
#ifdef CR_NV_vertex_program
for (i=0; i<CR_MAX_VERTEX_ATTRIBS; i++)
{
    CRSTATE_BITS_OP(client.a[i], GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
}
#endif

_CRSTATE_BITS_OP_SIZEOF(current.dirty);
for (i=0; i<CR_MAX_VERTEX_ATTRIBS; i++)
{
    _CRSTATE_BITS_OP_SIZEOF(current.vertexAttrib[i]);
}
_CRSTATE_BITS_OP_SIZEOF(current.edgeFlag);
_CRSTATE_BITS_OP_SIZEOF(current.colorIndex);
_CRSTATE_BITS_OP_SIZEOF(current.rasterPos);


_CRSTATE_BITS_OP_SIZEOF(eval.dirty);
for (i=0; i<GLEVAL_TOT; i++)
{
    _CRSTATE_BITS_OP_SIZEOF(eval.eval1D[i]);
    _CRSTATE_BITS_OP_SIZEOF(eval.eval2D[i]);
    _CRSTATE_BITS_OP_SIZEOF(eval.enable1D[i]);
    _CRSTATE_BITS_OP_SIZEOF(eval.enable2D[i]);
}
_CRSTATE_BITS_OP_SIZEOF(eval.enable);
_CRSTATE_BITS_OP_SIZEOF(eval.grid1D);
_CRSTATE_BITS_OP_SIZEOF(eval.grid2D);
#ifdef CR_NV_vertex_program
        /*@todo Those seems to be unused?
_CRSTATE_BITS_OP_SIZEOF(eval.enableAttrib1D);
_CRSTATE_BITS_OP_SIZEOF(eval.enableAttrib2D);
        */
#endif

_CRSTATE_BITS_OP_SIZEOF(feedback.dirty);
_CRSTATE_BITS_OP_SIZEOF(selection.dirty);

_CRSTATE_BITS_OP_SIZEOF(fog.dirty);
_CRSTATE_BITS_OP_SIZEOF(fog.color);
_CRSTATE_BITS_OP_SIZEOF(fog.index);
_CRSTATE_BITS_OP_SIZEOF(fog.density);
_CRSTATE_BITS_OP_SIZEOF(fog.start);
_CRSTATE_BITS_OP_SIZEOF(fog.end);
_CRSTATE_BITS_OP_SIZEOF(fog.mode);
_CRSTATE_BITS_OP_SIZEOF(fog.enable);
#ifdef CR_NV_fog_distance
_CRSTATE_BITS_OP_SIZEOF(fog.fogDistanceMode);
#endif
#ifdef CR_EXT_fog_coord
_CRSTATE_BITS_OP_SIZEOF(fog.fogCoordinateSource);
#endif

_CRSTATE_BITS_OP_SIZEOF(hint.dirty);
_CRSTATE_BITS_OP_SIZEOF(hint.perspectiveCorrection);
_CRSTATE_BITS_OP_SIZEOF(hint.pointSmooth);
_CRSTATE_BITS_OP_SIZEOF(hint.lineSmooth);
_CRSTATE_BITS_OP_SIZEOF(hint.polygonSmooth);
_CRSTATE_BITS_OP_SIZEOF(hint.fog);
#ifdef CR_EXT_clip_volume_hint
_CRSTATE_BITS_OP_SIZEOF(hint.clipVolumeClipping);

#endif
#ifdef CR_ARB_texture_compression
_CRSTATE_BITS_OP_SIZEOF(hint.textureCompression);
#endif
#ifdef CR_SGIS_generate_mipmap
_CRSTATE_BITS_OP_SIZEOF(hint.generateMipmap);
#endif

_CRSTATE_BITS_OP_SIZEOF(lighting.dirty);
_CRSTATE_BITS_OP_SIZEOF(lighting.shadeModel);
_CRSTATE_BITS_OP_SIZEOF(lighting.colorMaterial);
_CRSTATE_BITS_OP_SIZEOF(lighting.lightModel);
_CRSTATE_BITS_OP_SIZEOF(lighting.material);
_CRSTATE_BITS_OP_SIZEOF(lighting.enable);
for (i=0; i<CR_MAX_LIGHTS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].dirty);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].enable);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].ambient);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].diffuse);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].specular);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].position);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].attenuation);
    _CRSTATE_BITS_OP_SIZEOF(lighting.light[i].spot);
}

_CRSTATE_BITS_OP_SIZEOF(line.dirty);
_CRSTATE_BITS_OP_SIZEOF(line.enable);
_CRSTATE_BITS_OP_SIZEOF(line.width);
_CRSTATE_BITS_OP_SIZEOF(line.stipple);

_CRSTATE_BITS_OP_SIZEOF(lists.dirty);
_CRSTATE_BITS_OP_SIZEOF(lists.base);

_CRSTATE_BITS_OP_SIZEOF(multisample.dirty);
_CRSTATE_BITS_OP_SIZEOF(multisample.enable);
_CRSTATE_BITS_OP_SIZEOF(multisample.sampleAlphaToCoverage);
_CRSTATE_BITS_OP_SIZEOF(multisample.sampleAlphaToOne);
_CRSTATE_BITS_OP_SIZEOF(multisample.sampleCoverage);
_CRSTATE_BITS_OP_SIZEOF(multisample.sampleCoverageValue);

#if CR_ARB_occlusion_query
_CRSTATE_BITS_OP_SIZEOF(occlusion.dirty);
#endif

_CRSTATE_BITS_OP_SIZEOF(pixel.dirty);
_CRSTATE_BITS_OP_SIZEOF(pixel.transfer);
_CRSTATE_BITS_OP_SIZEOF(pixel.zoom);
_CRSTATE_BITS_OP_SIZEOF(pixel.maps);

_CRSTATE_BITS_OP_SIZEOF(point.dirty);
_CRSTATE_BITS_OP_SIZEOF(point.enableSmooth);
_CRSTATE_BITS_OP_SIZEOF(point.size);
#ifdef CR_ARB_point_parameters
_CRSTATE_BITS_OP_SIZEOF(point.minSize);
_CRSTATE_BITS_OP_SIZEOF(point.maxSize);
_CRSTATE_BITS_OP_SIZEOF(point.fadeThresholdSize);
_CRSTATE_BITS_OP_SIZEOF(point.distanceAttenuation);
#endif
#ifdef CR_ARB_point_sprite
_CRSTATE_BITS_OP_SIZEOF(point.enableSprite);
for (i=0; i<CR_MAX_TEXTURE_UNITS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(point.coordReplacement[i]);
}
#endif
#if CRSTATE_BITS_OP_VERSION >= SHCROGL_SSM_VERSION_WITH_SPRITE_COORD_ORIGIN
_CRSTATE_BITS_OP_SIZEOF(point.spriteCoordOrigin);
#endif

_CRSTATE_BITS_OP_SIZEOF(polygon.dirty);
_CRSTATE_BITS_OP_SIZEOF(polygon.enable);
_CRSTATE_BITS_OP_SIZEOF(polygon.offset);
_CRSTATE_BITS_OP_SIZEOF(polygon.mode);
_CRSTATE_BITS_OP_SIZEOF(polygon.stipple);

_CRSTATE_BITS_OP_SIZEOF(program.dirty);
_CRSTATE_BITS_OP_SIZEOF(program.vpEnable);
_CRSTATE_BITS_OP_SIZEOF(program.fpEnable);
_CRSTATE_BITS_OP_SIZEOF(program.vpBinding);
_CRSTATE_BITS_OP_SIZEOF(program.fpBinding);
for (i=0; i<CR_MAX_VERTEX_ATTRIBS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(program.vertexAttribArrayEnable[i]);
    _CRSTATE_BITS_OP_SIZEOF(program.map1AttribArrayEnable[i]);
    _CRSTATE_BITS_OP_SIZEOF(program.map2AttribArrayEnable[i]);
}
for (i=0; i<CR_MAX_VERTEX_PROGRAM_ENV_PARAMS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(program.vertexEnvParameter[i]);
}
for (i=0; i<CR_MAX_FRAGMENT_PROGRAM_ENV_PARAMS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(program.fragmentEnvParameter[i]);
}
_CRSTATE_BITS_OP_SIZEOF(program.vertexEnvParameters);
_CRSTATE_BITS_OP_SIZEOF(program.fragmentEnvParameters);
for (i=0; i<CR_MAX_VERTEX_PROGRAM_ENV_PARAMS/4; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(program.trackMatrix[i]);
}

_CRSTATE_BITS_OP_SIZEOF(regcombiner.dirty);
_CRSTATE_BITS_OP_SIZEOF(regcombiner.enable);
_CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerVars);
_CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerColor0);
_CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerColor1);
for (i=0; i<CR_MAX_GENERAL_COMBINERS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerStageColor0[i]);
    _CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerStageColor1[i]);
    _CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerInput[i]);
    _CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerOutput[i]);
}
_CRSTATE_BITS_OP_SIZEOF(regcombiner.regCombinerFinalInput);

_CRSTATE_BITS_OP_SIZEOF(stencil.dirty);
_CRSTATE_BITS_OP_SIZEOF(stencil.enable);
#ifdef _CRSTATE_BITS_OP_STENCIL_V_33
_CRSTATE_BITS_OP_SIZEOF(stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func);
_CRSTATE_BITS_OP_SIZEOF(stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op);
for (i = CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK + 1; i < CRSTATE_STENCIL_BUFFER_REF_COUNT; ++i)
{
    CRSTATE_BITS_OP_STENCIL_FUNC_V_33(i, stencil.bufferRefs[i].func);
    CRSTATE_BITS_OP_STENCIL_OP_V_33(i, stencil.bufferRefs[i].op);
}
_CRSTATE_BITS_OP_SIZEOF(stencil.clearValue);
_CRSTATE_BITS_OP_SIZEOF(stencil.writeMask);
#else
_CRSTATE_BITS_OP_SIZEOF(stencil.enableTwoSideEXT);
_CRSTATE_BITS_OP_SIZEOF(stencil.activeStencilFace);
_CRSTATE_BITS_OP_SIZEOF(stencil.clearValue);
_CRSTATE_BITS_OP_SIZEOF(stencil.writeMask);
for (i = 0; i < CRSTATE_STENCIL_BUFFER_REF_COUNT; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(stencil.bufferRefs[i].func);
    _CRSTATE_BITS_OP_SIZEOF(stencil.bufferRefs[i].op);
}
#endif

_CRSTATE_BITS_OP_SIZEOF(texture.dirty);
for (i=0; i<CR_MAX_TEXTURE_UNITS; ++i)
{
    _CRSTATE_BITS_OP_SIZEOF(texture.enable[i]);
    _CRSTATE_BITS_OP_SIZEOF(texture.current[i]);
    _CRSTATE_BITS_OP_SIZEOF(texture.objGen[i]);
    _CRSTATE_BITS_OP_SIZEOF(texture.eyeGen[i]);
    _CRSTATE_BITS_OP_SIZEOF(texture.genMode[i]);
    _CRSTATE_BITS_OP_SIZEOF(texture.envBit[i]);
}

_CRSTATE_BITS_OP_SIZEOF(transform.dirty);
_CRSTATE_BITS_OP_SIZEOF(transform.matrixMode);
_CRSTATE_BITS_OP_SIZEOF(transform.modelviewMatrix);
_CRSTATE_BITS_OP_SIZEOF(transform.projectionMatrix);
_CRSTATE_BITS_OP_SIZEOF(transform.colorMatrix);
_CRSTATE_BITS_OP_SIZEOF(transform.textureMatrix);
_CRSTATE_BITS_OP_SIZEOF(transform.programMatrix);
_CRSTATE_BITS_OP_SIZEOF(transform.clipPlane);
_CRSTATE_BITS_OP_SIZEOF(transform.enable);
_CRSTATE_BITS_OP_SIZEOF(transform.base);

_CRSTATE_BITS_OP_SIZEOF(viewport.dirty);
_CRSTATE_BITS_OP_SIZEOF(viewport.v_dims);
_CRSTATE_BITS_OP_SIZEOF(viewport.s_dims);
_CRSTATE_BITS_OP_SIZEOF(viewport.enable);
_CRSTATE_BITS_OP_SIZEOF(viewport.depth);

} while (0);

#undef CRSTATE_BITS_OP_VERSION
#undef _CRSTATE_BITS_OP_STENCIL_V_33
