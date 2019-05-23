/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_EXTSTRING_H
#define CR_EXTSTRING_H

#include "cr_version.h"

/*
 * This string is the list of OpenGL extensions which Chromium can understand
 * (in the packer, unpacker, state-tracker, etc).
 * In practice, this string will get intersected with what's reported by the
 * rendering SPUs to reflect what we can really offer to client apps.
 *
 * Yes, we want static declarations here to avoid linking problems.
 */
static const char *crExtensions =
#ifdef CR_EXT_texture_compression_s3tc
    "GL_EXT_texture_compression_s3tc "
#endif
#ifdef CR_EXT_draw_range_elements
    "GL_EXT_draw_range_elements "
#endif
#ifdef CR_EXT_framebuffer_object
    "GL_EXT_framebuffer_object "
#endif
#ifdef CR_EXT_compiled_vertex_array
    "GL_EXT_compiled_vertex_array "
#endif
#ifdef CR_ARB_depth_texture
	"GL_ARB_depth_texture "
#endif
#ifdef CR_ARB_fragment_program
	"GL_ARB_fragment_program "
#endif
#ifdef CR_ARB_imaging
	"GL_ARB_imaging "
#endif
#ifdef CR_ARB_multisample
	"GL_ARB_multisample "
#endif
#ifdef CR_ARB_multitexture
	"GL_ARB_multitexture "
#endif
#ifdef CR_ARB_occlusion_query
	"GL_ARB_occlusion_query "
#endif
#ifdef CR_ARB_point_parameters
	"GL_ARB_point_parameters "
#endif
#ifdef CR_ARB_point_sprite
	"GL_ARB_point_sprite "
#endif
#ifdef CR_ARB_shadow
	"GL_ARB_shadow "
#endif
#ifdef CR_ARB_shadow_ambient
	"GL_ARB_shadow_ambient "
#endif
#ifdef CR_ARB_texture_border_clamp
	"GL_ARB_texture_border_clamp "
#endif
#ifdef CR_ARB_texture_compression
	"GL_ARB_texture_compression "
#endif
#ifdef CR_ARB_texture_cube_map
	"GL_ARB_texture_cube_map "
#endif
#ifdef CR_ARB_texture_env_add
	"GL_ARB_texture_env_add "
#endif
#ifdef CR_ARB_texture_env_combine
	"GL_ARB_texture_env_combine GL_EXT_texture_env_combine "
#endif
#ifdef CR_ARB_texture_env_crossbar
	"GL_ARB_texture_env_crossbar "
#endif
#ifdef CR_ARB_texture_env_dot3
	"GL_ARB_texture_env_dot3 GL_EXT_texture_env_dot3 "
#endif
#ifdef CR_ARB_texture_mirrored_repeat
	"GL_ARB_texture_mirrored_repeat GL_IBM_texture_mirrored_repeat "
#endif
#ifdef CR_ATI_texture_mirror_once
	"GL_ATI_texture_mirror_once "
#endif
#ifdef CR_ARB_texture_non_power_of_two
	"GL_ARB_texture_non_power_of_two "
#endif
#ifdef CR_ARB_transpose_matrix
	"GL_ARB_transpose_matrix "
#endif
#ifdef CR_ARB_vertex_buffer_object
	"GL_ARB_vertex_buffer_object "
#endif
#ifdef CR_ARB_pixel_buffer_object
    "GL_ARB_pixel_buffer_object "
#endif
#ifdef CR_ARB_vertex_program
	"GL_ARB_vertex_program "
#endif
#ifdef CR_ARB_window_pos
	"GL_ARB_window_pos "
#endif
#ifdef CR_EXT_blend_color
	"GL_EXT_blend_color "
#endif
#ifdef CR_EXT_blend_minmax
	"GL_EXT_blend_minmax "
#endif
#ifdef CR_EXT_blend_func_separate
	"GL_EXT_blend_func_separate "
#endif
#ifdef CR_EXT_clip_volume_hint
	"GL_EXT_clip_volume_hint "
#endif
#ifdef CR_EXT_blend_logic_op
	"GL_EXT_blend_logic_op "
#endif
#ifdef CR_EXT_blend_subtract
	"GL_EXT_blend_subtract "
#endif
#ifdef CR_EXT_texture_env_add
	"GL_EXT_texture_env_add "
#endif
#ifdef CR_EXT_fog_coord
	"GL_EXT_fog_coord "
#endif
#ifdef CR_EXT_multi_draw_arrays
	"GL_EXT_multi_draw_arrays "
#endif
#ifdef CR_EXT_secondary_color
	"GL_EXT_secondary_color "
#endif
#ifdef CR_EXT_separate_specular_color
	"GL_EXT_separate_specular_color "
#endif
#ifdef CR_EXT_shadow_funcs
	"GL_EXT_shadow_funcs "
#endif
#ifdef CR_EXT_stencil_wrap
	"GL_EXT_stencil_wrap "
#endif
#ifdef CR_EXT_texture_cube_map
	"GL_EXT_texture_cube_map "
#endif
#ifdef CR_EXT_texture_edge_clamp
	"GL_EXT_texture_edge_clamp "
#endif
#ifdef CR_EXT_texture_filter_anisotropic
	"GL_EXT_texture_filter_anisotropic "
#endif
#ifdef CR_EXT_texture_lod_bias
	"GL_EXT_texture_lod_bias "
#endif
#ifdef CR_EXT_texture_object
	"GL_EXT_texture_object "
#endif
#ifdef CR_EXT_texture3D
	"GL_EXT_texture3D "
#endif
#ifdef CR_IBM_rasterpos_clip
	"GL_IBM_rasterpos_clip "
#endif
#ifdef CR_NV_fog_distance
	"GL_NV_fog_distance "
#endif
#ifdef CR_NV_fragment_program
	"GL_NV_fragment_program "
#endif
#ifdef CR_NV_fragment_program_option
    "GL_NV_fragment_program_option "
#endif
#ifdef CR_NV_fragment_program2
    "GL_NV_fragment_program2 "
#endif
#ifdef CR_NV_register_combiners
	"GL_NV_register_combiners "
#endif
#ifdef CR_NV_register_combiners2
	"GL_NV_register_combiners2 "
#endif
#ifdef CR_NV_texgen_reflection
	"GL_NV_texgen_reflection "
#endif
#ifdef CR_NV_texture_rectangle
	"GL_NV_texture_rectangle GL_EXT_texture_rectangle GL_ARB_texture_rectangle "
#endif
#ifdef CR_NV_vertex_program
	"GL_NV_vertex_program "
#endif
#ifdef CR_NV_vertex_program1_1
	"GL_NV_vertex_program1_1 "
#endif
#ifdef CR_NV_vertex_program2
	"GL_NV_vertex_program2 "
#endif
#ifdef CR_NV_vertex_program2_option
    "GL_NV_vertex_program2_option "
#endif
#ifdef CR_NV_vertex_program3
    "GL_NV_vertex_program3 "
#endif
#ifdef CR_SGIS_generate_mipmap
	"GL_SGIS_generate_mipmap "
#endif
#ifdef CR_SGIS_texture_border_clamp
	"GL_SGIS_texture_border_clamp "
#endif
#ifdef CR_SGIS_texture_edge_clamp
	"GL_SGIS_texture_edge_clamp "
#endif
#ifdef CR_ARB_shading_language_100
    "GL_ARB_shading_language_100 "
#endif
#ifdef CR_ARB_shader_objects
    "GL_ARB_shader_objects "
#endif
#ifdef CR_ARB_vertex_shader
    "GL_ARB_vertex_shader "
#endif
#ifdef CR_ARB_fragment_shader
    "GL_ARB_fragment_shader "
#endif
#ifdef CR_EXT_texture_sRGB
    "GL_EXT_texture_sRGB "
#endif
#ifdef CR_EXT_framebuffer_blit
    "GL_EXT_framebuffer_blit "
#endif
#ifdef CR_EXT_blend_equation_separate
    "GL_EXT_blend_equation_separate "
#endif
#ifdef CR_EXT_stencil_two_side
    "GL_EXT_stencil_two_side "
#endif
#ifdef CR_GREMEDY_string_marker
    "GL_GREMEDY_string_marker "
#endif
#ifdef CR_ARB_texture_float
    "GL_ARB_texture_float "
#endif
#ifdef CR_ARB_draw_buffers
    "GL_ARB_draw_buffers "
#endif
#ifdef CR_ARB_shader_texture_lod
    "GL_ARB_shader_texture_lod "
#endif

	"";

/*
 * Extensions which are only supported if the render/readback SPU is
 * on the app node (no packing/unpacking/state-tracking support).
 */
static const char *crAppOnlyExtensions =
  "GL_NV_fence " \
  "GL_NV_texture_env_combine4 " \
  "GL_NV_texture_shader " \
  "GL_NV_vertex_array_range "
;


/*
 * Special extensions which are unique to Chromium.
 * We typically append this to the result of glGetString(GL_EXTENSIONS).
 */
static const char *crChromiumExtensions =
#ifdef GL_CR_state_parameter
	"GL_CR_state_parameter "
#endif
#ifdef GL_CR_cursor_position
	"GL_CR_cursor_position "
#endif
#ifdef GL_CR_bounding_box
	"GL_CR_bounding_box "
#endif
#ifdef GL_CR_print_string
	"GL_CR_print_string "
#endif
#ifdef GL_CR_tilesort_info
	"GL_CR_tilesort_info "
#endif
#ifdef GL_CR_client_clear_control
	"GL_CR_client_clear_control "
#endif
#ifdef GL_CR_synchronization
	"GL_CR_synchronization "
#endif
#ifdef GL_CR_head_spu_name
	"GL_CR_head_spu_name "
#endif
#ifdef GL_CR_performance_info
	"GL_CR_performance_info "
#endif
#ifdef GL_CR_window_size
	"GL_CR_window_size "
#endif
#ifdef GL_CR_tile_info
	"GL_CR_tile_info "
#endif
#ifdef GL_CR_saveframe
	"GL_CR_saveframe "
#endif
#ifdef GL_CR_readback_barrier_size
	"GL_CR_readback_barrier_size "
#endif
#ifdef GL_CR_server_id_sharing
	"GL_CR_server_id_sharing "
#endif
#ifdef GL_CR_server_matrix
	"GL_CR_server_matrix "
#endif
#ifdef USE_DMX
	"GL_CR_dmx "
#endif
	"";

#endif /* CR_EXTSTRING_H */

