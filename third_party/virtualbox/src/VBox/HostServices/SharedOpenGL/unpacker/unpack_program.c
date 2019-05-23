/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_version.h"


void crUnpackExtendProgramParameter4dvNV(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLdouble params[4];
	params[0] = READ_DOUBLE(16);
	params[1] = READ_DOUBLE(24);
	params[2] = READ_DOUBLE(32);
	params[3] = READ_DOUBLE(40);
	cr_unpackDispatch.ProgramParameter4dvNV(target, index, params);
}


void crUnpackExtendProgramParameter4fvNV(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(16, GLfloat);
	params[1] = READ_DATA(20, GLfloat);
	params[2] = READ_DATA(24, GLfloat);
	params[3] = READ_DATA(28, GLfloat);
	cr_unpackDispatch.ProgramParameter4fvNV(target, index, params);
}


void crUnpackExtendProgramParameters4dvNV(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLuint num = READ_DATA(16, GLuint);
    GLdouble *params;

    if (num >= UINT32_MAX / (4 * sizeof(GLdouble)))
    {
        crError("crUnpackExtendProgramParameters4dvNV: parameter 'num' is out of range");
        return;
    }

    params = (GLdouble *)crAlloc(num * 4 * sizeof(GLdouble));

	if (params) {
		GLuint i;
		for (i = 0; i < 4 * num; i++) {
            params[i] = READ_DATA(20 + i * sizeof(GLdouble), GLdouble);
		}
		cr_unpackDispatch.ProgramParameters4dvNV(target, index, num, params);
		crFree(params);
	}
}


void crUnpackExtendProgramParameters4fvNV(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLuint num = READ_DATA(16, GLuint);
    GLfloat *params;

    if (num >= UINT32_MAX / (4 * sizeof(GLfloat)))
    {
        crError("crUnpackExtendProgramParameters4fvNV: parameter 'num' is out of range");
        return;
    }

    params = (GLfloat *)crAlloc(num * 4 * sizeof(GLfloat));

	if (params) {
		GLuint i;
		for (i = 0; i < 4 * num; i++) {
            params[i] = READ_DATA(20 + i * sizeof(GLfloat), GLfloat);
		}
		cr_unpackDispatch.ProgramParameters4fvNV(target, index, num, params);
		crFree(params);
	}
}


void crUnpackExtendAreProgramsResidentNV(void)
{
	GLsizei n = READ_DATA(8, GLsizei);
	const GLuint *programs = DATA_POINTER(12, const GLuint);
	SET_RETURN_PTR(12 + n * sizeof(GLuint));
	SET_WRITEBACK_PTR(20 + n * sizeof(GLuint));
	(void) cr_unpackDispatch.AreProgramsResidentNV(n, programs, NULL);
}


void crUnpackExtendLoadProgramNV(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint id = READ_DATA(12, GLuint);
	GLsizei len = READ_DATA(16, GLsizei);
	GLvoid *program = DATA_POINTER(20, GLvoid);
	cr_unpackDispatch.LoadProgramNV(target, id, len, program);
}


void crUnpackExtendExecuteProgramNV(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint id = READ_DATA(12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(16, GLfloat);
	params[1] = READ_DATA(20, GLfloat);
	params[2] = READ_DATA(24, GLfloat);
	params[3] = READ_DATA(28, GLfloat);
	cr_unpackDispatch.ExecuteProgramNV(target, id, params);
}

void crUnpackExtendRequestResidentProgramsNV(void)
{
	GLsizei n = READ_DATA(8, GLsizei);
	crError("RequestResidentProgramsNV needs to be special cased!");
	cr_unpackDispatch.RequestResidentProgramsNV(n, NULL);
}


void crUnpackExtendProgramLocalParameter4fvARB(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(16, GLfloat);
	params[1] = READ_DATA(20, GLfloat);
	params[2] = READ_DATA(24, GLfloat);
	params[3] = READ_DATA(28, GLfloat);
	cr_unpackDispatch.ProgramLocalParameter4fvARB(target, index, params);
}


void crUnpackExtendProgramLocalParameter4dvARB(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLdouble params[4];
	params[0] = READ_DOUBLE(16);
	params[1] = READ_DOUBLE(24);
	params[2] = READ_DOUBLE(32);
	params[3] = READ_DOUBLE(40);
	cr_unpackDispatch.ProgramLocalParameter4dvARB(target, index, params);
}



void crUnpackExtendProgramNamedParameter4dvNV(void)
{
	GLuint id = READ_DATA(8, GLuint);
	GLsizei len = READ_DATA(12, GLsizei);
	GLdouble params[4];
	GLubyte *name = crAlloc(len);
	params[0] = READ_DOUBLE(16);
	params[1] = READ_DOUBLE(24);
	params[2] = READ_DOUBLE(32);
	params[3] = READ_DOUBLE(40);
	crMemcpy(name, DATA_POINTER(48, GLubyte), len);
	cr_unpackDispatch.ProgramNamedParameter4dvNV(id, len, name, params);
}

void crUnpackExtendProgramNamedParameter4dNV(void)
{
	GLuint id = READ_DATA(8, GLuint);
	GLsizei len = READ_DATA(12, GLsizei);
	GLdouble params[4];
	GLubyte *name = crAlloc (len);
	params[0] = READ_DOUBLE(16);
	params[1] = READ_DOUBLE(24);
	params[2] = READ_DOUBLE(32);
	params[3] = READ_DOUBLE(40);
	crMemcpy(name, DATA_POINTER(48, GLubyte), len);
	cr_unpackDispatch.ProgramNamedParameter4dNV(id, len, name, params[0], params[1], params[2], params[3]);
}

void crUnpackExtendProgramNamedParameter4fNV(void)
{
	GLenum id = READ_DATA(8, GLuint);
	GLsizei len = READ_DATA(12, GLsizei);
	GLfloat params[4];
	GLubyte *name = crAlloc(len);
	params[0] = READ_DATA(16, GLfloat);
	params[1] = READ_DATA(20, GLfloat);
	params[2] = READ_DATA(24, GLfloat);
	params[3] = READ_DATA(28, GLfloat);
	crMemcpy(name, DATA_POINTER(32,  GLubyte), len);
	cr_unpackDispatch.ProgramNamedParameter4fNV(id, len, name, params[0], params[1], params[2], params[3]);
}

void crUnpackExtendProgramNamedParameter4fvNV(void)
{
	GLenum id = READ_DATA(8, GLuint);
	GLsizei len = READ_DATA(12, GLsizei);
	GLfloat params[4];
	GLubyte *name = crAlloc(len);
	params[0] = READ_DATA(16, GLfloat);
	params[1] = READ_DATA(20, GLfloat);
	params[2] = READ_DATA(24, GLfloat);
	params[3] = READ_DATA(28, GLfloat);
	crMemcpy(name, DATA_POINTER(32, GLubyte), len);
	cr_unpackDispatch.ProgramNamedParameter4fvNV(id, len, name, params);
}

void crUnpackExtendGetProgramNamedParameterdvNV(void)
{
	GLuint id = READ_DATA(8, GLuint);
	GLsizei len = READ_DATA(12, GLsizei);
	const GLubyte *name = DATA_POINTER(16, GLubyte);
	SET_RETURN_PTR(16+len);
	SET_WRITEBACK_PTR(16+len+8);
	cr_unpackDispatch.GetProgramNamedParameterdvNV(id, len, name, NULL);
}

void crUnpackExtendGetProgramNamedParameterfvNV(void)
{
	GLuint id = READ_DATA(8, GLuint);
	GLsizei len = READ_DATA(12, GLsizei);
	const GLubyte *name = DATA_POINTER(16, GLubyte);
	SET_RETURN_PTR(16+len);
	SET_WRITEBACK_PTR(16+len+8);
	cr_unpackDispatch.GetProgramNamedParameterfvNV(id, len, name, NULL);
}

void crUnpackExtendProgramStringARB(void)
{ 
      GLenum target = READ_DATA(8, GLenum);
      GLenum format = READ_DATA(12, GLuint);
      GLsizei len = READ_DATA(16, GLsizei);
      GLvoid *program = DATA_POINTER(20, GLvoid);
      cr_unpackDispatch.ProgramStringARB(target, format, len, program);
}

void crUnpackExtendGetProgramStringARB(void)
{
}

void crUnpackExtendProgramEnvParameter4dvARB(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLdouble params[4];
	params[0] = READ_DOUBLE(16);
	params[1] = READ_DOUBLE(24);
	params[2] = READ_DOUBLE(32);
	params[3] = READ_DOUBLE(40);
	cr_unpackDispatch.ProgramEnvParameter4dvARB(target, index, params);
}

void crUnpackExtendProgramEnvParameter4fvARB(void)
{
	GLenum target = READ_DATA(8, GLenum);
	GLuint index = READ_DATA(12, GLuint);
	GLfloat params[4];
	params[0] = READ_DATA(16, GLfloat);
	params[1] = READ_DATA(20, GLfloat);
	params[2] = READ_DATA(24, GLfloat);
	params[3] = READ_DATA(28, GLfloat);
	cr_unpackDispatch.ProgramEnvParameter4fvARB(target, index, params);
}

void crUnpackExtendDeleteProgramsARB(void)
{
	GLsizei n = READ_DATA(8, GLsizei);
	const GLuint *programs = DATA_POINTER(12, GLuint);
	cr_unpackDispatch.DeleteProgramsARB(n, programs);
}

void crUnpackVertexAttrib4NbvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLbyte *v = DATA_POINTER(4, const GLbyte);
	cr_unpackDispatch.VertexAttrib4NbvARB(index, v);
	INCR_DATA_PTR(8);
}

void crUnpackVertexAttrib4NivARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLint *v = DATA_POINTER(4, const GLint);
	cr_unpackDispatch.VertexAttrib4NivARB(index, v);
	INCR_DATA_PTR(20);
}

void crUnpackVertexAttrib4NsvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLshort *v = DATA_POINTER(4, const GLshort);
	cr_unpackDispatch.VertexAttrib4NsvARB(index, v);
	INCR_DATA_PTR(12);
}

void crUnpackVertexAttrib4NubvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLubyte *v = DATA_POINTER(4, const GLubyte);
	cr_unpackDispatch.VertexAttrib4NubvARB(index, v);
	INCR_DATA_PTR(8);
}

void crUnpackVertexAttrib4NuivARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLuint *v = DATA_POINTER(4, const GLuint);
	cr_unpackDispatch.VertexAttrib4NuivARB(index, v);
	INCR_DATA_PTR(20);
}

void crUnpackVertexAttrib4NusvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLushort *v = DATA_POINTER(4, const GLushort);
	cr_unpackDispatch.VertexAttrib4NusvARB(index, v);
	INCR_DATA_PTR(12);
}

void crUnpackVertexAttrib4bvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLbyte *v = DATA_POINTER(4, const GLbyte);
	cr_unpackDispatch.VertexAttrib4bvARB(index, v);
	INCR_DATA_PTR(8);
}

void crUnpackVertexAttrib4ivARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLint *v = DATA_POINTER(4, const GLint);
	cr_unpackDispatch.VertexAttrib4ivARB(index, v);
	INCR_DATA_PTR(20);
}

void crUnpackVertexAttrib4ubvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLubyte *v = DATA_POINTER(4, const GLubyte);
	cr_unpackDispatch.VertexAttrib4ubvARB(index, v);
	INCR_DATA_PTR(8);
}

void crUnpackVertexAttrib4uivARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLuint *v = DATA_POINTER(4, const GLuint);
	cr_unpackDispatch.VertexAttrib4uivARB(index, v);
	INCR_DATA_PTR(20);
}

void crUnpackVertexAttrib4usvARB(void)
{
	GLuint index = READ_DATA(0, GLuint);
	const GLushort *v = DATA_POINTER(4, const GLushort);
	cr_unpackDispatch.VertexAttrib4usvARB(index, v);
	INCR_DATA_PTR(12);
}
