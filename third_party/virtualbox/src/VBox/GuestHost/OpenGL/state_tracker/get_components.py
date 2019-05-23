# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function

num_components = {
	'GL_AMBIENT' : 4, 
	'GL_DIFFUSE' : 4,
	'GL_SPECULAR' : 4,
	'GL_POSITION' : 4,
	'GL_SPOT_DIRECTION' : 3,
	'GL_SPOT_EXPONENT' : 1, 
	'GL_SPOT_CUTOFF' : 1, 
	'GL_CONSTANT_ATTENUATION' : 1, 
	'GL_LINEAR_ATTENUATION' : 1, 
	'GL_QUADRATIC_ATTENUATION' : 1, 
	'GL_EMISSION' : 4, 
	'GL_SHININESS' : 1, 
	'GL_COLOR_INDEXES' : 3, 
	'GL_TEXTURE_ENV_MODE' : 1,
	'GL_TEXTURE_ENV_COLOR' : 4, 
	'GL_TEXTURE_GEN_MODE' : 1, 
	'GL_OBJECT_PLANE' : 4, 
	'GL_EYE_PLANE' : 4, 
	'GL_TEXTURE_MAG_FILTER' : 1,
	'GL_TEXTURE_MIN_FILTER' : 1, 
	'GL_TEXTURE_WRAP_S' : 1, 
	'GL_TEXTURE_WRAP_T' : 1, 
	'GL_TEXTURE_BORDER_COLOR' : 4,
	'GL_TEXTURE_WIDTH': 1,
	'GL_TEXTURE_HEIGHT': 1,
	'GL_TEXTURE_DEPTH': 1,
	# 'GL_TEXTURE_INTERNAL_FORMAT': 1,  THIS CONFLICTS WITH GL_TEXTURE_COMPONENTS!
	'GL_TEXTURE_BORDER': 1,
	'GL_TEXTURE_RED_SIZE': 1,
	'GL_TEXTURE_GREEN_SIZE': 1,
	'GL_TEXTURE_BLUE_SIZE': 1,
	'GL_TEXTURE_ALPHA_SIZE': 1,
	'GL_TEXTURE_LUMINANCE_SIZE': 1,
	'GL_TEXTURE_INTENSITY_SIZE': 1,
	'GL_TEXTURE_COMPONENTS': 1,
	'GL_TEXTURE_RESIDENT': 1
}

num_extended_components = {
	'GL_TEXTURE_MAX_ANISOTROPY_EXT': ( 1, 'CR_EXT_texture_filter_anisotropic' ),
	'GL_TEXTURE_WRAP_R': ( 1, 'CR_OPENGL_VERSION_1_2'),
	'GL_TEXTURE_PRIORITY': ( 1, 'CR_OPENGL_VERSION_1_2'),
	'GL_TEXTURE_MIN_LOD': ( 1, 'CR_OPENGL_VERSION_1_2'),
	'GL_TEXTURE_MAX_LOD': ( 1, 'CR_OPENGL_VERSION_1_2'),
	'GL_TEXTURE_BASE_LEVEL': ( 1, 'CR_OPENGL_VERSION_1_2'),
	'GL_TEXTURE_MAX_LEVEL': ( 1, 'CR_OPENGL_VERSION_1_2'),
	'GL_COMBINER_INPUT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_MAPPING_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_COMPONENT_USAGE_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_AB_DOT_PRODUCT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_CD_DOT_PRODUCT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_MUX_SUM_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_SCALE_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_BIAS_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_AB_OUTPUT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_CD_OUTPUT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_SUM_OUTPUT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_INPUT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_INPUT_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_MAPPING_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_COMBINER_COMPONENT_USAGE_NV': ( 1, 'CR_NV_register_combiners'),
	'GL_CONSTANT_COLOR0_NV': ( 4, 'CR_NV_register_combiners'),
	'GL_CONSTANT_COLOR1_NV': ( 4, 'CR_NV_register_combiners'),
	'GL_COMBINE_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_COMBINE_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_SOURCE0_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_SOURCE1_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_SOURCE2_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_SOURCE0_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_SOURCE1_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_SOURCE2_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_OPERAND0_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_OPERAND1_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_OPERAND2_RGB_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_OPERAND0_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_OPERAND1_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_OPERAND2_ALPHA_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_RGB_SCALE_ARB': (1, 'CR_ARB_texture_env_combine'),
	'GL_ALPHA_SCALE': (1, 'CR_ARB_texture_env_combine'),
	'GL_DEPTH_TEXTURE_MODE_ARB': (1, 'CR_ARB_depth_texture'),
	'GL_TEXTURE_DEPTH_SIZE_ARB': (1, 'CR_ARB_depth_texture'),
	'GL_TEXTURE_COMPARE_MODE_ARB': (1, 'CR_ARB_shadow'),
	'GL_TEXTURE_COMPARE_FUNC_ARB': (1, 'CR_ARB_shadow'),
	'GL_TEXTURE_COMPARE_FAIL_VALUE_ARB': (1, 'CR_ARB_shadow_ambient'),
	'GL_GENERATE_MIPMAP_SGIS': (1, 'CR_SGIS_generate_mipmap'),
	'GL_TEXTURE_LOD_BIAS_EXT': (1, 'CR_EXT_texture_lod_bias'),
	'GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB': (1, 'CR_any_vertex_program'),
	'GL_CURRENT_VERTEX_ATTRIB_ARB': (4, 'CR_any_vertex_program'),
	'GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB': (1, 'CR_any_vertex_program'),
	'GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB': (1, 'CR_any_vertex_program'),
	'GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB': (1, 'CR_any_vertex_program'),
	'GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB': (1, 'CR_any_vertex_program'),
	'GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB': (1, 'CR_any_vertex_program'),
	'GL_TRACK_MATRIX_NV': (24, 'CR_any_vertex_program'),
	'GL_TRACK_MATRIX_TRANSFORM_NV': (24, 'CR_any_vertex_program'),
	'GL_BUFFER_SIZE_ARB': (1, 'CR_ARB_vertex_buffer_object'),
	'GL_BUFFER_USAGE_ARB': (1, 'CR_ARB_vertex_buffer_object'),
	'GL_BUFFER_ACCESS_ARB': (1, 'CR_ARB_vertex_buffer_object'),
	'GL_BUFFER_MAPPED_ARB': (1, 'CR_ARB_vertex_buffer_object'),
	'GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB': (1, 'CR_ARB_vertex_buffer_object'),
	'GL_QUERY_COUNTER_BITS_ARB': (1, 'CR_ARB_occlusion_query'),
	'GL_QUERY_RESULT_AVAILABLE_ARB': (1, 'CR_ARB_occlusion_query'),
	'GL_QUERY_RESULT_ARB': (1, 'CR_ARB_occlusion_query'),
	'GL_CURRENT_QUERY_ARB': (1, 'CR_ARB_occlusion_query'),
	'GL_TEXTURE_COMPRESSED_IMAGE_SIZE': (1, 'CR_ARB_texture_compression'),
    'GL_TEXTURE_COMPRESSED': (1, 'CR_ARB_texture_compression'),
	'GL_COORD_REPLACE_ARB': (1, 'CR_ARB_point_sprite'),
}

print("""unsigned int crStateHlpComponentsCount( GLenum pname )
{
	switch( pname )
	{
""")
for comp in sorted(num_components.keys()):
	print('\t\t\tcase %s: return %d;' % (comp,num_components[comp]))

for comp in sorted(num_extended_components.keys()):
	(nc, ifdef) = num_extended_components[comp]
	print('#ifdef %s' % ifdef)
	print('\t\t\tcase %s: return %d;' % (comp,nc))
	print('#endif /* %s */' % ifdef)

print("""
		default:
			crError( "Unknown parameter name in crStateHlpComponentsCount: %d", (int) pname );
			break;
	}
	/* NOTREACHED */
	return 0;
}
""")

