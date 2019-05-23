/* $Id: dlm_pointers.h $ */
#include <VBoxUhgsmi.h>

#include "cr_dlm.h"
#include "dlm_generated.h"

#ifndef _DLM_POINTERS_H
#define _DLM_POINTERS_H

extern int crdlm_pointers_Bitmap( struct instanceBitmap *instance, GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap, CRClientState *c);
extern int crdlm_pointers_DrawPixels( struct instanceDrawPixels *instance, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_Fogfv( struct instanceFogfv *instance, GLenum pname, const GLfloat *params );
extern int crdlm_pointers_Fogiv( struct instanceFogiv *instance, GLenum pname, const GLint *params );
extern int crdlm_pointers_LightModelfv( struct instanceLightModelfv *instance, GLenum pname, const GLfloat *params );
extern int crdlm_pointers_LightModeliv( struct instanceLightModeliv *instance, GLenum pname, const GLint *params );
extern int crdlm_pointers_Lightfv( struct instanceLightfv *instance, GLenum light, GLenum pname, const GLfloat *params );
extern int crdlm_pointers_Lightiv( struct instanceLightiv *instance, GLenum light, GLenum pname, const GLint *params );
extern int crdlm_pointers_Map1d( struct instanceMap1d *instance, GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
extern int crdlm_pointers_Map1f( struct instanceMap1f *instance, GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
extern int crdlm_pointers_Map2d( struct instanceMap2d *instance, GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
extern int crdlm_pointers_Map2f( struct instanceMap2f *instance, GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
extern int crdlm_pointers_Materialfv(struct instanceMaterialfv *instance, GLenum face, GLenum pname, const GLfloat *params);
extern int crdlm_pointers_Materialiv(struct instanceMaterialiv *instance, GLenum face, GLenum pname, const GLint *params);
extern int crdlm_pointers_PixelMapfv( struct instancePixelMapfv *instance, GLenum map, GLsizei mapsize, const GLfloat *values );
extern int crdlm_pointers_PixelMapuiv( struct instancePixelMapuiv *instance, GLenum map, GLsizei mapsize, const GLuint *values );
extern int crdlm_pointers_PixelMapusv( struct instancePixelMapusv *instance, GLenum map, GLsizei mapsize, const GLushort *values );
extern int crdlm_pointers_PointParameterfvARB( struct instancePointParameterfvARB *instance, GLenum pname, const GLfloat *params);
extern int crdlm_pointers_PointParameteriv( struct instancePointParameteriv *instance, GLenum pname, const GLint *params);
extern int crdlm_pointers_TexEnvfv( struct instanceTexEnvfv *instance, GLenum target, GLenum pname, const GLfloat *params );
extern int crdlm_pointers_TexEnviv( struct instanceTexEnviv *instance, GLenum target, GLenum pname, const GLint *params );
extern int crdlm_pointers_TexGendv( struct instanceTexGendv *instance, GLenum coord, GLenum pname, const GLdouble *params );
extern int crdlm_pointers_TexGenfv( struct instanceTexGenfv *instance, GLenum coord, GLenum pname, const GLfloat *params );
extern int crdlm_pointers_TexGeniv( struct instanceTexGeniv *instance, GLenum coord, GLenum pname, const GLint *params );
extern int crdlm_pointers_TexImage1D( struct instanceTexImage1D *instance, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_CompressedTexImage1DARB(struct instanceCompressedTexImage1DARB *instance, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imagesize, const GLvoid *data);
extern int crdlm_pointers_TexImage2D( struct instanceTexImage2D *instance, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_CompressedTexImage2DARB(struct instanceCompressedTexImage2DARB *instance, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imagesize, const GLvoid *data);
extern int crdlm_pointers_TexImage3D( struct instanceTexImage3D *instance, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_TexImage3DEXT( struct instanceTexImage3DEXT *instance, GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_CompressedTexImage3DARB(struct instanceCompressedTexImage3DARB *instance, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imagesize, const GLvoid *data);
extern int crdlm_pointers_TexParameterfv( struct instanceTexParameterfv *instance, GLenum target, GLenum pname, const GLfloat *params );
extern int crdlm_pointers_TexParameteriv( struct instanceTexParameteriv *instance, GLenum target, GLenum pname, const GLint *params );
extern int crdlm_pointers_TexSubImage1D( struct instanceTexSubImage1D *instance, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_TexSubImage2D( struct instanceTexSubImage2D *instance, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_TexSubImage3D( struct instanceTexSubImage3D *instance, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels, CRClientState *c );
extern int crdlm_pointers_CompressedTexSubImage1DARB(struct instanceCompressedTexSubImage1DARB *instance, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imagesize, const GLvoid *data);
extern int crdlm_pointers_CompressedTexSubImage2DARB(struct instanceCompressedTexSubImage2DARB *instance, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imagesize, const GLvoid *data);
extern int crdlm_pointers_CompressedTexSubImage3DARB(struct instanceCompressedTexSubImage3DARB *instance, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imagesize, const GLvoid *data);
extern int crdlm_pointers_Rectdv(struct instanceRectdv *instance, const GLdouble *v1, const GLdouble *v2);
extern int crdlm_pointers_Rectfv(struct instanceRectfv *instance, const GLfloat *v1, const GLfloat *v2);
extern int crdlm_pointers_Rectiv(struct instanceRectiv *instance, const GLint *v1, const GLint *v2);
extern int crdlm_pointers_Rectsv(struct instanceRectsv *instance, const GLshort *v1, const GLshort *v2);
extern int crdlm_pointers_PrioritizeTextures(struct instancePrioritizeTextures *instance, GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern int crdlm_pointers_CombinerParameterivNV(struct instanceCombinerParameterivNV *instance, GLenum pname, const GLint *params);
extern int crdlm_pointers_CombinerParameterfvNV(struct instanceCombinerParameterfvNV *instance, GLenum pname, const GLfloat *params);
extern int crdlm_pointers_CombinerStageParameterfvNV(struct instanceCombinerStageParameterfvNV *instance, GLenum stage, GLenum pname, const GLfloat *params);
extern int crdlm_pointers_ExecuteProgramNV(struct instanceExecuteProgramNV *instance, GLenum target, GLuint id, const GLfloat *params);
extern int crdlm_pointers_RequestResidentProgramsNV(struct instanceRequestResidentProgramsNV *instance, GLsizei n, const GLuint *ids);
extern int crdlm_pointers_LoadProgramNV(struct instanceLoadProgramNV *instance, GLenum target, GLuint id, GLsizei len, const GLubyte *program);
extern int crdlm_pointers_ProgramNamedParameter4dNV(struct instanceProgramNamedParameter4dNV *instance, GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern int crdlm_pointers_ProgramNamedParameter4dvNV(struct instanceProgramNamedParameter4dvNV *instance, GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v);
extern int crdlm_pointers_ProgramNamedParameter4fNV(struct instanceProgramNamedParameter4fNV *instance, GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern int crdlm_pointers_ProgramNamedParameter4fvNV(struct instanceProgramNamedParameter4fvNV *instance, GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v);
extern int crdlm_pointers_ProgramStringARB(struct instanceProgramStringARB *instance, GLenum target, GLenum format, GLsizei len, const GLvoid * string);
extern int crdlm_pointers_CallLists(struct instanceCallLists *instance, GLsizei n, GLenum type, const GLvoid *lists );
extern int crdlm_pointers_VertexAttribs1dvNV(struct instanceVertexAttribs1dvNV *instance, GLuint index, GLsizei n, const GLdouble *v);
extern int crdlm_pointers_VertexAttribs1fvNV(struct instanceVertexAttribs1fvNV *instance, GLuint index, GLsizei n, const GLfloat *v);
extern int crdlm_pointers_VertexAttribs1svNV(struct instanceVertexAttribs1svNV *instance, GLuint index, GLsizei n, const GLshort *v);
extern int crdlm_pointers_VertexAttribs2dvNV(struct instanceVertexAttribs2dvNV *instance, GLuint index, GLsizei n, const GLdouble *v);
extern int crdlm_pointers_VertexAttribs2fvNV(struct instanceVertexAttribs2fvNV *instance, GLuint index, GLsizei n, const GLfloat *v);
extern int crdlm_pointers_VertexAttribs2svNV(struct instanceVertexAttribs2svNV *instance, GLuint index, GLsizei n, const GLshort *v);
extern int crdlm_pointers_VertexAttribs3dvNV(struct instanceVertexAttribs3dvNV *instance, GLuint index, GLsizei n, const GLdouble *v);
extern int crdlm_pointers_VertexAttribs3fvNV(struct instanceVertexAttribs3fvNV *instance, GLuint index, GLsizei n, const GLfloat *v);
extern int crdlm_pointers_VertexAttribs3svNV(struct instanceVertexAttribs3svNV *instance, GLuint index, GLsizei n, const GLshort *v);
extern int crdlm_pointers_VertexAttribs4dvNV(struct instanceVertexAttribs4dvNV *instance, GLuint index, GLsizei n, const GLdouble *v);
extern int crdlm_pointers_VertexAttribs4fvNV(struct instanceVertexAttribs4fvNV *instance, GLuint index, GLsizei n, const GLfloat *v);
extern int crdlm_pointers_VertexAttribs4svNV(struct instanceVertexAttribs4svNV *instance, GLuint index, GLsizei n, const GLshort *v);
extern int crdlm_pointers_VertexAttribs4ubvNV(struct instanceVertexAttribs4ubvNV *instance, GLuint index, GLsizei n, const GLubyte *v);
extern int crdlm_pointers_ZPixCR( struct instanceZPixCR *instance, GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum ztype, GLint zparm, GLint length, const GLvoid *pixels, CRClientState *c );

#endif
