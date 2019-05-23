# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

aliases = [
	# GL_ARB_multitexture / OpenGL 1.2.1
	('ActiveTexture', 'ActiveTextureARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('ClientActiveTexture', 'ClientActiveTextureARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1d', 'MultiTexCoord1dARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1dv','MultiTexCoord1dvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1f', 'MultiTexCoord1fARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1fv','MultiTexCoord1fvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1i', 'MultiTexCoord1iARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1iv','MultiTexCoord1ivARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1s', 'MultiTexCoord1sARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord1sv','MultiTexCoord1svARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2d', 'MultiTexCoord2dARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2dv','MultiTexCoord2dvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2f', 'MultiTexCoord2fARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2fv','MultiTexCoord2fvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2i', 'MultiTexCoord2iARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2iv','MultiTexCoord2ivARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2s', 'MultiTexCoord2sARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord2sv','MultiTexCoord2svARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3d', 'MultiTexCoord3dARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3dv','MultiTexCoord3dvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3f', 'MultiTexCoord3fARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3fv','MultiTexCoord3fvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3i', 'MultiTexCoord3iARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3iv','MultiTexCoord3ivARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3s', 'MultiTexCoord3sARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord3sv','MultiTexCoord3svARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4d', 'MultiTexCoord4dARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4dv','MultiTexCoord4dvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4f', 'MultiTexCoord4fARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4fv','MultiTexCoord4fvARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4i', 'MultiTexCoord4iARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4iv','MultiTexCoord4ivARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4s', 'MultiTexCoord4sARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	('MultiTexCoord4sv','MultiTexCoord4svARB', 'ARB_multitexture', 'CR_OPENGL_VERSION_1_2_1'),
	# GL_ARB_transpose_matrix / OpenGL 1.3
	('LoadTransposeMatrixf','LoadTransposeMatrixfARB', 'ARB_transpose_matrix', 'CR_OPENGL_VERSION_1_3'),
	('LoadTransposeMatrixd','LoadTransposeMatrixdARB', 'ARB_transpose_matrix', 'CR_OPENGL_VERSION_1_3'),
	('MultTransposeMatrixf','MultTransposeMatrixfARB', 'ARB_transpose_matrix', 'CR_OPENGL_VERSION_1_3'),
	('MultTransposeMatrixd','MultTransposeMatrixdARB', 'ARB_transpose_matrix', 'CR_OPENGL_VERSION_1_3'),
	# GL_ARB_texture_compression / OpenGL 1.3
	('CompressedTexImage3D', 'CompressedTexImage3DARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	('CompressedTexImage2D', 'CompressedTexImage2DARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	('CompressedTexImage1D', 'CompressedTexImage1DARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	('CompressedTexSubImage3D', 'CompressedTexSubImage3DARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	('CompressedTexSubImage2D', 'CompressedTexSubImage2DARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	('CompressedTexSubImage1D', 'CompressedTexSubImage1DARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	('GetCompressedTexImage', 'GetCompressedTexImageARB', 'ARB_texture_compression', 'CR_OPENGL_VERSION_1_3'),
	# GL_ARB_multisample / OpenGL 1.3
	('SampleCoverage', 'SampleCoverageARB', 'ARB_multisample', 'CR_OPENGL_VERSION_1_3'),
	# GL_ARB_window_pos / OpenGL 1.4
	('WindowPos2d', 'WindowPos2dARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),

	('WindowPos2dv', 'WindowPos2dvARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos2f', 'WindowPos2fARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos2fv', 'WindowPos2fvARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos2i', 'WindowPos2iARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos2iv', 'WindowPos2ivARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos2s', 'WindowPos2sARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos2sv', 'WindowPos2svARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3d', 'WindowPos3dARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3dv', 'WindowPos3dvARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3f', 'WindowPos3fARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3fv', 'WindowPos3fvARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3i', 'WindowPos3iARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3iv', 'WindowPos3ivARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3s', 'WindowPos3sARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	('WindowPos3sv', 'WindowPos3svARB', 'ARB_window_pos', 'CR_OPENGL_VERSION_1_4'),
	# GL_ARB_point_parameters / OpenGL 1.4
	('PointParameterf', 'PointParameterfARB', 'ARB_point_parameters', 'CR_OPENGL_VERSION_1_4'),
	('PointParameterfv', 'PointParameterfvARB', 'ARB_point_parameters', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_blend_color / OpenGL 1.4
	('BlendColor', 'BlendColorEXT', 'EXT_blend_color', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_blend_func_separate / OpenGL 1.4
	('BlendFuncSeparate', 'BlendFuncSeparateEXT', 'EXT_blend_func_separate', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_blend_equation / OpenGL 1.4
	('BlendEquation', 'BlendEquationEXT', 'EXT_blend_equation', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_multi_draw_arrays / OpenGL 1.4
	('MultiDrawArrays', 'MultiDrawArraysEXT', 'EXT_multi_draw_arrays', 'CR_OPENGL_VERSION_1_4'),
	('MultiDrawElements', 'MultiDrawElementsEXT', 'EXT_multi_draw_arrays', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_secondary_color / OpenGL 1.4
	('SecondaryColor3b', 'SecondaryColor3bEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3bv', 'SecondaryColor3bvEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3d', 'SecondaryColor3dEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3dv', 'SecondaryColor3dvEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3f', 'SecondaryColor3fEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3fv', 'SecondaryColor3fvEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3i', 'SecondaryColor3iEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3iv', 'SecondaryColor3ivEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3s', 'SecondaryColor3sEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3sv', 'SecondaryColor3svEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3ub', 'SecondaryColor3ubEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3ubv', 'SecondaryColor3ubvEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3ui', 'SecondaryColor3uiEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3uiv', 'SecondaryColor3uivEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3us', 'SecondaryColor3usEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColor3usv', 'SecondaryColor3usvEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	('SecondaryColorPointer', 'SecondaryColorPointerEXT', 'EXT_secondary_color', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_fog_coord / OpenGL 1.4
	('FogCoordf', 'FogCoordfEXT', 'EXT_fog_coord', 'CR_OPENGL_VERSION_1_4'),
	('FogCoordfv', 'FogCoordfvEXT', 'EXT_fog_coord', 'CR_OPENGL_VERSION_1_4'),
	('FogCoordd', 'FogCoorddEXT', 'EXT_fog_coord', 'CR_OPENGL_VERSION_1_4'),
	('FogCoorddv', 'FogCoorddvEXT', 'EXT_fog_coord', 'CR_OPENGL_VERSION_1_4'),
	('FogCoordPointer', 'FogCoordPointerEXT', 'EXT_fog_coord', 'CR_OPENGL_VERSION_1_4'),
	# GL_EXT_texture_object / OpenGL 1.1
	('AreTexturesResidentEXT', 'AreTexturesResident', 'EXT_texture_object', 'CR_OPENGL_VERSION_1_1'),
	('BindTextureEXT', 'BindTexture', 'EXT_texture_object', 'CR_OPENGL_VERSION_1_1'),
	('DeleteTexturesEXT', 'DeleteTextures', 'EXT_texture_object', 'CR_OPENGL_VERSION_1_1'),
	('GenTexturesEXT', 'GenTextures', 'EXT_texture_object', 'CR_OPENGL_VERSION_1_1'),
	('IsTextureEXT', 'IsTexture', 'EXT_texture_object', 'CR_OPENGL_VERSION_1_1'),
	('PrioritizeTexturesEXT', 'PrioritizeTextures', 'EXT_texture_object', 'CR_OPENGL_VERSION_1_1'),
	# GL_EXT_texture3D / OpenGL 1.2
	('TexSubImage3DEXT', 'TexSubImage3D', 'EXT_texture3D', 'CR_OPENGL_VERSION_1_2'),

	# GL_NV_vertex_program / GL_ARB_vertex_program
	('VertexAttrib1sNV', 'VertexAttrib1sARB', 'ARB_vertex_program', ''),
	('VertexAttrib1svNV', 'VertexAttrib1svARB', 'ARB_vertex_program', ''),
	('VertexAttrib1fNV', 'VertexAttrib1fARB', 'ARB_vertex_program', ''),
	('VertexAttrib1fvNV', 'VertexAttrib1fvARB', 'ARB_vertex_program', ''),
	('VertexAttrib1dNV', 'VertexAttrib1dARB', 'ARB_vertex_program', ''),
	('VertexAttrib1dvNV', 'VertexAttrib1dvARB', 'ARB_vertex_program', ''),
	('VertexAttrib2sNV', 'VertexAttrib2sARB', 'ARB_vertex_program', ''),
	('VertexAttrib2svNV', 'VertexAttrib2svARB', 'ARB_vertex_program', ''),
	('VertexAttrib2fNV', 'VertexAttrib2fARB', 'ARB_vertex_program', ''),
	('VertexAttrib2fvNV', 'VertexAttrib2fvARB', 'ARB_vertex_program', ''),
	('VertexAttrib2dNV', 'VertexAttrib2dARB', 'ARB_vertex_program', ''),
	('VertexAttrib2dvNV', 'VertexAttrib2dvARB', 'ARB_vertex_program', ''),
	('VertexAttrib3sNV', 'VertexAttrib3sARB', 'ARB_vertex_program', ''),
	('VertexAttrib3svNV', 'VertexAttrib3svARB', 'ARB_vertex_program', ''),
	('VertexAttrib3fNV', 'VertexAttrib3fARB', 'ARB_vertex_program', ''),
	('VertexAttrib3fvNV', 'VertexAttrib3fvARB', 'ARB_vertex_program', ''),
	('VertexAttrib3dNV', 'VertexAttrib3dARB', 'ARB_vertex_program', ''),
	('VertexAttrib3dvNV', 'VertexAttrib3dvARB', 'ARB_vertex_program', ''),
	('VertexAttrib4sNV', 'VertexAttrib4sARB', 'ARB_vertex_program', ''),
	('VertexAttrib4svNV', 'VertexAttrib4svARB', 'ARB_vertex_program', ''),
	('VertexAttrib4fNV', 'VertexAttrib4fARB', 'ARB_vertex_program', ''),
	('VertexAttrib4fvNV', 'VertexAttrib4fvARB', 'ARB_vertex_program', ''),
	('VertexAttrib4dNV', 'VertexAttrib4dARB', 'ARB_vertex_program', ''),
	('VertexAttrib4dvNV', 'VertexAttrib4dvARB', 'ARB_vertex_program', ''),
	('VertexAttrib4ubNV', 'VertexAttrib4NubARB', 'ARB_vertex_program', ''),
	('VertexAttrib4ubvNV', 'VertexAttrib4NubvARB', 'ARB_vertex_program', ''),
	('DeleteProgramsNV', 'DeleteProgramsARB', 'ARB_vertex_program', ''),
	('IsProgramNV', 'IsProgramARB', 'ARB_vertex_program', '')
]

def AliasMap( func_name ):
	for (aliased_func_name, real_func_name, ext_define, gl_version) in aliases:
		if real_func_name == func_name:
			return aliased_func_name;
	return None

def ExtDefine( func_name ):
	for (aliased_func_name, real_func_name, ext_define, gl_version) in aliases:
		if real_func_name == func_name:
			return ext_define;
	return None

def GLversion( func_name ):
	for (aliased_func_name, real_func_name, ext_define, gl_version) in aliases:
		if real_func_name == func_name:
			return gl_version;
	return None
