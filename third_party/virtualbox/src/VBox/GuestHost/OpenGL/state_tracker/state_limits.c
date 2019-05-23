
/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_extstring.h"

#ifdef WINDOWS
#pragma warning( disable : 4127 )
#endif


/* This is a debug helper function. */
void crStateLimitsPrint (const CRLimitsState *l)
{
	fprintf(stderr, "----------- OpenGL limits ----------------\n");
	fprintf(stderr, "GL_MAX_TEXTURE_UNITS = %d\n", (int) l->maxTextureUnits);
	fprintf(stderr, "GL_MAX_TEXTURE_SIZE = %d\n", (int) l->maxTextureSize);
	fprintf(stderr, "GL_MAX_3D_TEXTURE_SIZE = %d\n", (int) l->max3DTextureSize);
	fprintf(stderr, "GL_MAX_CUBE_MAP_TEXTURE_SIZE = %d\n", (int) l->maxCubeMapTextureSize);
	fprintf(stderr, "GL_MAX_TEXTURE_ANISOTROPY = %f\n", l->maxTextureAnisotropy);
	fprintf(stderr, "GL_MAX_LIGHTS = %d\n", (int) l->maxLights);
	fprintf(stderr, "GL_MAX_CLIP_PLANES = %d\n", (int) l->maxClipPlanes);
	fprintf(stderr, "GL_MAX_ATTRIB_STACK_DEPTH = %d\n", (int) l->maxClientAttribStackDepth);
	fprintf(stderr, "GL_MAX_PROJECTION_STACK_DEPTH = %d\n", (int) l->maxProjectionStackDepth);
	fprintf(stderr, "GL_MAX_MODELVIEW_STACK_DEPTH = %d\n", (int) l->maxModelviewStackDepth);
	fprintf(stderr, "GL_MAX_TEXTURE_STACK_DEPTH = %d\n", (int) l->maxTextureStackDepth);
	fprintf(stderr, "GL_MAX_COLOR_STACK_DEPTH = %d\n", (int) l->maxColorStackDepth);
	fprintf(stderr, "GL_MAX_ATTRIB_STACK_DEPTH = %d\n", (int) l->maxAttribStackDepth);
	fprintf(stderr, "GL_MAX_ATTRIB_STACK_DEPTH = %d\n", (int) l->maxClientAttribStackDepth);
	fprintf(stderr, "GL_MAX_NAME_STACK_DEPTH = %d\n", (int) l->maxNameStackDepth);
	fprintf(stderr, "GL_MAX_ELEMENTS_INDICES = %d\n", (int) l->maxElementsIndices);
	fprintf(stderr, "GL_MAX_ELEMENTS_VERTICES = %d\n", (int) l->maxElementsVertices);
	fprintf(stderr, "GL_MAX_EVAL_ORDER = %d\n", (int) l->maxEvalOrder);
	fprintf(stderr, "GL_MAX_LIST_NESTING = %d\n", (int) l->maxListNesting);
	fprintf(stderr, "GL_MAX_PIXEL_MAP_TABLE = %d\n", (int) l->maxPixelMapTable);
	fprintf(stderr, "GL_MAX_VIEWPORT_DIMS = %d %d\n",
		(int) l->maxViewportDims[0], (int) l->maxViewportDims[1]);
	fprintf(stderr, "GL_SUBPIXEL_BITS = %d\n", (int) l->subpixelBits);
	fprintf(stderr, "GL_ALIASED_POINT_SIZE_RANGE = %f .. %f\n",
		l->aliasedPointSizeRange[0], l->aliasedPointSizeRange[1]);
	fprintf(stderr, "GL_SMOOTH_POINT_SIZE_RANGE = %f .. %f\n",
		l->aliasedPointSizeRange[0], l->aliasedPointSizeRange[1]);
	fprintf(stderr, "GL_POINT_SIZE_GRANULARITY = %f\n", l->pointSizeGranularity);
	fprintf(stderr, "GL_ALIASED_LINE_WIDTH_RANGE = %f .. %f\n",
		l->aliasedLineWidthRange[0], l->aliasedLineWidthRange[1]);
	fprintf(stderr, "GL_SMOOTH_LINE_WIDTH_RANGE = %f .. %f\n",
		l->smoothLineWidthRange[0], l->smoothLineWidthRange[1]);
	fprintf(stderr, "GL_LINE_WIDTH_GRANULARITY = %f\n", l->lineWidthGranularity);
	fprintf(stderr, "GL_MAX_GENERAL_COMBINERS_NV = %d\n", (int) l->maxGeneralCombiners);
	fprintf(stderr, "GL_EXTENSIONS = %s\n", (const char *) l->extensions);
	fprintf(stderr, "------------------------------------------\n");
}


void crStateLimitsDestroy(CRLimitsState *l)
{
	if (l->extensions) {
		crFree((void *) l->extensions);
		l->extensions = NULL;
	}
}


/*
 * Initialize the CRLimitsState object to Chromium's defaults.
 */
void crStateLimitsInit (CRLimitsState *l)
{
	l->maxTextureUnits = CR_MAX_TEXTURE_UNITS;
	l->maxTextureSize = CR_MAX_TEXTURE_SIZE;
	l->max3DTextureSize = CR_MAX_3D_TEXTURE_SIZE;
	l->maxCubeMapTextureSize = CR_MAX_CUBE_TEXTURE_SIZE;
#ifdef CR_NV_texture_rectangle
	l->maxRectTextureSize  = CR_MAX_RECTANGLE_TEXTURE_SIZE;
#endif
	l->maxTextureAnisotropy = CR_MAX_TEXTURE_ANISOTROPY;
	l->maxGeneralCombiners = CR_MAX_GENERAL_COMBINERS;
	l->maxLights = CR_MAX_LIGHTS;
	l->maxClipPlanes = CR_MAX_CLIP_PLANES;
	l->maxClientAttribStackDepth = CR_MAX_ATTRIB_STACK_DEPTH;
	l->maxProjectionStackDepth = CR_MAX_PROJECTION_STACK_DEPTH;
	l->maxModelviewStackDepth = CR_MAX_MODELVIEW_STACK_DEPTH;
	l->maxTextureStackDepth = CR_MAX_TEXTURE_STACK_DEPTH;
	l->maxColorStackDepth = CR_MAX_COLOR_STACK_DEPTH;
	l->maxAttribStackDepth = CR_MAX_ATTRIB_STACK_DEPTH;
	l->maxClientAttribStackDepth = CR_MAX_ATTRIB_STACK_DEPTH;
	l->maxNameStackDepth = CR_MAX_NAME_STACK_DEPTH;
	l->maxElementsIndices = CR_MAX_ELEMENTS_INDICES;
	l->maxElementsVertices = CR_MAX_ELEMENTS_VERTICES;
	l->maxEvalOrder = CR_MAX_EVAL_ORDER;
	l->maxListNesting = CR_MAX_LIST_NESTING;
	l->maxPixelMapTable = CR_MAX_PIXEL_MAP_TABLE;
	l->maxViewportDims[0] = l->maxViewportDims[1] = CR_MAX_VIEWPORT_DIM;
	l->subpixelBits = CR_SUBPIXEL_BITS;
	l->aliasedPointSizeRange[0] = CR_ALIASED_POINT_SIZE_MIN;
	l->aliasedPointSizeRange[1] = CR_ALIASED_POINT_SIZE_MAX;
	l->smoothPointSizeRange[0] = CR_SMOOTH_POINT_SIZE_MIN;
	l->smoothPointSizeRange[1] = CR_SMOOTH_POINT_SIZE_MAX;
	l->pointSizeGranularity = CR_POINT_SIZE_GRANULARITY;
	l->aliasedLineWidthRange[0] = CR_ALIASED_LINE_WIDTH_MIN;
	l->aliasedLineWidthRange[1] = CR_ALIASED_LINE_WIDTH_MAX;
	l->smoothLineWidthRange[0] = CR_SMOOTH_LINE_WIDTH_MIN;
	l->smoothLineWidthRange[1] = CR_SMOOTH_LINE_WIDTH_MAX;
	l->lineWidthGranularity = CR_LINE_WIDTH_GRANULARITY;
#ifdef CR_EXT_texture_lod_bias
	l->maxTextureLodBias = CR_MAX_TEXTURE_LOD_BIAS;
#endif
#ifdef CR_NV_fragment_program
	l->maxTextureCoords = CR_MAX_TEXTURE_COORDS;
	l->maxTextureImageUnits = CR_MAX_TEXTURE_IMAGE_UNITS;
	l->maxFragmentProgramLocalParams = CR_MAX_FRAGMENT_LOCAL_PARAMS;
#endif
#ifdef CR_NV_vertex_program
	l->maxProgramMatrixStackDepth = CR_MAX_PROGRAM_MATRIX_STACK_DEPTH;
	l->maxProgramMatrices = CR_MAX_PROGRAM_MATRICES;
#endif
#ifdef CR_ARB_fragment_program
	l->maxFragmentProgramInstructions = CR_MAX_FRAGMENT_PROGRAM_INSTRUCTIONS;
	l->maxFragmentProgramLocalParams = CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS;
	l->maxFragmentProgramEnvParams = CR_MAX_FRAGMENT_PROGRAM_ENV_PARAMS;
	l->maxFragmentProgramTemps = CR_MAX_FRAGMENT_PROGRAM_TEMPS;
	l->maxFragmentProgramAttribs = CR_MAX_FRAGMENT_PROGRAM_ATTRIBS;
	l->maxFragmentProgramAddressRegs = CR_MAX_FRAGMENT_PROGRAM_ADDRESS_REGS;
	l->maxFragmentProgramAluInstructions = CR_MAX_FRAGMENT_PROGRAM_ALU_INSTRUCTIONS;
	l->maxFragmentProgramTexInstructions = CR_MAX_FRAGMENT_PROGRAM_TEX_INSTRUCTIONS;
	l->maxFragmentProgramTexIndirections = CR_MAX_FRAGMENT_PROGRAM_TEX_INDIRECTIONS;
#endif
#ifdef CR_ARB_vertex_program
	l->maxVertexProgramInstructions = CR_MAX_VERTEX_PROGRAM_INSTRUCTIONS;
	l->maxVertexProgramLocalParams = CR_MAX_VERTEX_PROGRAM_LOCAL_PARAMS;
	l->maxVertexProgramEnvParams = CR_MAX_VERTEX_PROGRAM_ENV_PARAMS;
	l->maxVertexProgramTemps = CR_MAX_VERTEX_PROGRAM_TEMPS;
	l->maxVertexProgramAttribs = CR_MAX_VERTEX_PROGRAM_ATTRIBS;
	l->maxVertexProgramAddressRegs = CR_MAX_VERTEX_PROGRAM_ADDRESS_REGS;
#endif

	l->extensions = (GLubyte *) crStrdup(crExtensions);

	/* These will get properly set in crStateCreateContext() by examining
	 * the visBits bitfield parameter.
	 */
	l->redBits = 0;
	l->greenBits = 0;
	l->blueBits = 0;
	l->alphaBits = 0;
	l->depthBits = 0;
	l->stencilBits = 0;
	l->accumRedBits = 0;
	l->accumGreenBits = 0;
	l->accumBlueBits = 0;
	l->accumAlphaBits = 0;
	l->auxBuffers = 0;
	l->rgbaMode = GL_TRUE;
	l->doubleBuffer = GL_FALSE;
	l->stereo = GL_FALSE;
	l->sampleBuffers = 0;
	l->samples = 0;
	l->level = 0;

	(void) crAppOnlyExtensions; /* silence warning */
}


/*
 * Given the GL version number returned from a real GL renderer,
 * compute the version number supported by Chromium.
 */
GLfloat crStateComputeVersion(float minVersion)
{
	const GLfloat crVersion = crStrToFloat(CR_OPENGL_VERSION_STRING);
	if (crVersion < minVersion)
		minVersion = crVersion;
	return minVersion;
}


/*
 * <extenions> is an array [n] of GLubyte pointers which contain lists of
 * OpenGL extensions.
 * Compute the intersection of those strings, then append the Chromium
 * extension strings.
 */
GLubyte * crStateMergeExtensions(GLuint n, const GLubyte **extensions)
{
	char *merged, *result;
	GLuint i;

	/* find intersection of all extension strings */
	merged = crStrdup(crExtensions);
	for (i = 0; i < n; i++)
	{
		char *m = crStrIntersect(merged, (const char *) extensions[i]);
		if (merged)
			crFree(merged);
		merged = m;
	}

	/* append Cr extensions */
	result = crStrjoin(merged, crChromiumExtensions);
	crFree(merged);
	return (GLubyte *) result;
}

static GLboolean hasExtension(const char *haystack, const char *needle)
{
	const int needleLen = crStrlen(needle);
	const char *s;

	while (1) {
		s = crStrstr(haystack, needle);
		if (!s)
			return GL_FALSE;
		if (s && (s[needleLen] == ' ' || s[needleLen] == 0))
			return GL_TRUE;
		haystack += needleLen;
	}
}

/*
 * Examine the context's extension string and set the boolean extension
 * flags accordingly.  This is to be called during context initialization.
 */
void crStateExtensionsInit( CRLimitsState *limits, CRExtensionState *extensions )
{
	/* init all booleans to false */
	crMemZero(extensions, sizeof(CRExtensionState));

	if (hasExtension((const char*)limits->extensions, "GL_ARB_depth_texture"))
		extensions->ARB_depth_texture = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_fragment_program"))
		extensions->ARB_fragment_program = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_imaging"))
		extensions->ARB_imaging = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_multisample"))
		extensions->ARB_multisample = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_multitexture"))
		extensions->ARB_multitexture = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_occlusion_query"))
		extensions->ARB_occlusion_query = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_point_parameters"))
		extensions->ARB_point_parameters = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_point_sprite"))
		extensions->ARB_point_sprite = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_shadow"))
		extensions->ARB_shadow = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_shadow_ambient"))
		extensions->ARB_shadow_ambient = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_border_clamp") ||
		hasExtension((const char*)limits->extensions, "GL_SGIS_texture_border_clamp"))
		extensions->ARB_texture_border_clamp = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_compression"))
		extensions->ARB_texture_compression = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_cube_map") ||
		hasExtension((const char*)limits->extensions, "GL_EXT_texture_cube_map"))
		extensions->ARB_texture_cube_map = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_env_add"))
		extensions->ARB_texture_env_add = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_env_combine") ||
			hasExtension((const char*)limits->extensions, "GL_EXT_texture_env_combine"))
		extensions->ARB_texture_env_combine = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_env_crossbar"))
		extensions->ARB_texture_env_crossbar = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_env_dot3") ||
			hasExtension((const char*)limits->extensions, "GL_EXT_texture_env_dot3"))
		extensions->ARB_texture_env_dot3 = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_mirrored_repeat"))
		extensions->ARB_texture_mirrored_repeat = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ATI_texture_mirror_once"))
		extensions->ATI_texture_mirror_once = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_texture_non_power_of_two"))
		extensions->ARB_texture_non_power_of_two = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_transpose_matrix"))
		extensions->ARB_transpose_matrix = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_vertex_buffer_object"))
		extensions->ARB_vertex_buffer_object = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_pixel_buffer_object"))
		extensions->ARB_pixel_buffer_object = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_vertex_program"))
		extensions->ARB_vertex_program = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_ARB_window_pos"))
		extensions->ARB_window_pos = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_blend_color"))
		extensions->EXT_blend_color= GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_blend_minmax"))
		extensions->EXT_blend_minmax = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_blend_func_separate"))
		extensions->EXT_blend_func_separate = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_blend_logic_op"))
		extensions->EXT_blend_logic_op = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_blend_subtract"))
		extensions->EXT_blend_subtract = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_clip_volume_hint"))
		extensions->EXT_clip_volume_hint = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_fog_coord"))
		extensions->EXT_fog_coord = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_multi_draw_arrays"))
		extensions->EXT_multi_draw_arrays = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_secondary_color"))
		extensions->EXT_secondary_color = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_separate_specular_color"))
		extensions->EXT_separate_specular_color = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_shadow_funcs"))
		extensions->EXT_shadow_funcs = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_stencil_wrap"))
		extensions->EXT_stencil_wrap = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_texture_edge_clamp") ||
		hasExtension((const char*)limits->extensions, "GL_SGIS_texture_edge_clamp"))
		extensions->EXT_texture_edge_clamp = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_texture_filter_anisotropic"))
		extensions->EXT_texture_filter_anisotropic = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_texture_lod_bias"))
		extensions->EXT_texture_lod_bias = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_IBM_rasterpos_clip"))
		extensions->IBM_rasterpos_clip = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_fog_distance"))
		extensions->NV_fog_distance = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_fragment_program"))
		extensions->NV_fragment_program = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_register_combiners"))
		extensions->NV_register_combiners = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_register_combiners2"))
		extensions->NV_register_combiners2 = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_texgen_reflection"))
		extensions->NV_texgen_reflection = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_texture_rectangle")
			|| hasExtension((const char*)limits->extensions, "GL_EXT_texture_rectangle"))
		extensions->NV_texture_rectangle = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_vertex_program"))
		extensions->NV_vertex_program = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_vertex_program1_1"))
		extensions->NV_vertex_program1_1 = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_NV_vertex_program2"))
		extensions->NV_vertex_program2 = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_EXT_texture3D"))
		extensions->EXT_texture3D = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GL_SGIS_generate_mipmap"))
		extensions->SGIS_generate_mipmap = GL_TRUE;

	if (hasExtension((const char*)limits->extensions, "GLX_EXT_texture_from_pixmap"))
		extensions->EXT_texture_from_pixmap = GL_TRUE;

	if (extensions->NV_vertex_program2)
		limits->maxVertexProgramEnvParams = 256;
	else
		limits->maxVertexProgramEnvParams = 96;

	if (extensions->NV_vertex_program || extensions->ARB_vertex_program)
		extensions->any_vertex_program = GL_TRUE;
	if (extensions->NV_fragment_program || extensions->ARB_fragment_program)
		extensions->any_fragment_program = GL_TRUE;
	if (extensions->any_vertex_program || extensions->any_fragment_program)
		extensions->any_program = GL_TRUE;

#if 0
	/* Now, determine what level of OpenGL we support */
	if (extensions->ARB_multisample &&
			extensions->ARB_multitexture &&
			extensions->ARB_texture_border_clamp &&
			extensions->ARB_texture_compression &&
			extensions->ARB_texture_cube_map &&
			extensions->ARB_texture_env_add &&
			extensions->ARB_texture_env_combine &&
			extensions->ARB_texture_env_dot3) {
		if (extensions->ARB_depth_texture &&
				extensions->ARB_point_parameters &&
				extensions->ARB_shadow &&
				extensions->ARB_texture_env_crossbar &&
				extensions->ARB_texture_mirrored_repeat &&
				extensions->ARB_window_pos &&
				extensions->EXT_blend_color &&
				extensions->EXT_blend_func_separate &&
				extensions->EXT_blend_logic_op &&
				extensions->EXT_blend_minmax &&
				extensions->EXT_blend_subtract &&
				extensions->EXT_fog_coord &&
				extensions->EXT_multi_draw_arrays &&
				extensions->EXT_secondary_color &&
				extensions->EXT_shadow_funcs  &&
				extensions->EXT_stencil_wrap &&
				extensions->SGIS_generate_mipmap) {
			if (extensions->ARB_occlusion_query &&
					extensions->ARB_vertex_buffer_object &&
					extensions->ARB_texture_non_power_of_two &&
					extensions->EXT_shadow_funcs) {
				extensions->version = (const GLubyte *) "1.5 Chromium " CR_VERSION_STRING;
			}
			else {
				extensions->version = (const GLubyte *) "1.4 Chromium " CR_VERSION_STRING;
			}
		}
		else {
			extensions->version = (const GLubyte *) "1.3 Chromium " CR_VERSION_STRING;
		}
	}
	else {
		extensions->version = (const GLubyte *) "1.2 Chromium " CR_VERSION_STRING;
	}
#endif
}


/*
 * Set the GL_EXTENSIONS string for the given context.  We'll make
 * a copy of the given string.
 */
void
crStateSetExtensionString( CRContext *ctx, const GLubyte *extensions )
{
   if (ctx->limits.extensions)
      crFree((void *) ctx->limits.extensions);

   ctx->limits.extensions = (const GLubyte *)crStrdup((const char*)extensions);

   crStateExtensionsInit(&(ctx->limits), &(ctx->extensions));
}

