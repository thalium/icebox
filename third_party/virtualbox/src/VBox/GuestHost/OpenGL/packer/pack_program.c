/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

/*
 * Packer functions for GL_NV_vertex_program extension.
 * XXX: Quite a few of these functions are unfinished.
 */


#include "packer.h"
#include "cr_error.h"


void PACK_APIENTRY crPackProgramParameters4dvNV(GLenum target, GLuint index, GLuint num, const GLdouble * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    int packet_length = sizeof(int) + sizeof(target) + sizeof(index) + sizeof(num) + num * 4 * sizeof(GLdouble);

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(sizeof(int) + 0, GLenum, target);
    WRITE_DATA(sizeof(int) + 4, GLuint, index);
    WRITE_DATA(sizeof(int) + 8, GLuint, num);
    crMemcpy(data_ptr + sizeof(int) + 12, params, num * 4 * sizeof(GLdouble));

    WRITE_OPCODE(pc, CR_PROGRAMPARAMETERS4DVNV_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackProgramParameters4fvNV(GLenum target, GLuint index, GLuint num, const GLfloat * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    int packet_length = sizeof(int) + sizeof(target) + sizeof(index) + sizeof(num) + num * 4 * sizeof(GLfloat);

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(sizeof(int) + 0, GLenum, target);
    WRITE_DATA(sizeof(int) + 4, GLuint, index);
    WRITE_DATA(sizeof(int) + 8, GLuint, num);
    crMemcpy(data_ptr + sizeof(int) + 12, params, num * 4 * sizeof(GLfloat));

    WRITE_OPCODE(pc, CR_PROGRAMPARAMETERS4FVNV_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackVertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib1dvARB(index + i, v + i);
}


void PACK_APIENTRY crPackVertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib1fvARB(index + i, v + i);
}


void PACK_APIENTRY crPackVertexAttribs1svNV(GLuint index, GLsizei n, const GLshort *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib1svARB(index + i, v + i);
}


void PACK_APIENTRY crPackVertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib2dvARB(index + i, v + 2 * i);
}

void PACK_APIENTRY crPackVertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib2fvARB(index + i, v + 2 * i);
}

void PACK_APIENTRY crPackVertexAttribs2svNV(GLuint index, GLsizei n, const GLshort *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib2svARB(index + i, v + 2 * i);
}

void PACK_APIENTRY crPackVertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib3dvARB(index + i, v + 3 * i);
}

void PACK_APIENTRY crPackVertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib3fvARB(index + i, v + 3 * i);
}

void PACK_APIENTRY crPackVertexAttribs3svNV(GLuint index, GLsizei n, const GLshort *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib3svARB(index + i, v + 3 * i);
}

void PACK_APIENTRY crPackVertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib4dvARB(index + i, v + 4 * i);
}

void PACK_APIENTRY crPackVertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib4fvARB(index + i, v + 4 * i);
}

void PACK_APIENTRY crPackVertexAttribs4svNV(GLuint index, GLsizei n, const GLshort *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib4svARB(index + i, v + 4 * i);
}

void PACK_APIENTRY crPackVertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte *v)
{
    GLint i;
    /* reverse order so we hit index 0 last (provoking glVertex) */
    for (i = n - 1; i >= 0; i--)
        crPackVertexAttrib4ubvARB(index + i, v + 4 * i);
}


void PACK_APIENTRY crPackExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params)
{
    const int packet_length = 32;
    unsigned char *data_ptr = NULL;
    CR_GET_PACKER_CONTEXT(pc);

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(4, GLenum, CR_EXECUTEPROGRAMNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLenum, target);
    WRITE_DATA(12, GLuint, id);
    WRITE_DATA(16, GLfloat, params[0]);
    WRITE_DATA(20, GLfloat, params[1]);
    WRITE_DATA(24, GLfloat, params[2]);
    WRITE_DATA(28, GLfloat, params[3]);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackLoadProgramNV(GLenum target, GLuint id, GLsizei len, const GLubyte *program)
{
    const int packet_length = 20 + len;
    unsigned char *data_ptr = NULL;
    CR_GET_PACKER_CONTEXT(pc);

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(4, GLenum, CR_LOADPROGRAMNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLenum, target);
    WRITE_DATA(12, GLuint, id);
    WRITE_DATA(16, GLsizei, len);
    crMemcpy((void *) (data_ptr + 20), program, len);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackRequestResidentProgramsNV(GLsizei n, const GLuint *ids)
{
    CR_GET_PACKER_CONTEXT(pc);
    (void) pc;
    (void) n;
    (void) ids;
    /* We're no-op'ing this function for now. */
}


void PACK_APIENTRY crPackProgramNamedParameter4fNV (GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    int packet_length = 32 + len;

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_PROGRAMNAMEDPARAMETER4FNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLuint, id);
    WRITE_DATA(12, GLsizei, len);
    WRITE_DATA(16, GLfloat, x);
    WRITE_DATA(20, GLfloat, y);
    WRITE_DATA(24, GLfloat, z);
    WRITE_DATA(28, GLfloat, w);
    crMemcpy((void *) (data_ptr + 32), name, len);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackProgramNamedParameter4dNV (GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    int packet_length = 48 + len;

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_PROGRAMNAMEDPARAMETER4DNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLenum, id);
    WRITE_DATA(12, GLuint, len);
    WRITE_DOUBLE(16, x);
    WRITE_DOUBLE(24, y);
    WRITE_DOUBLE(32, z);
    WRITE_DOUBLE(40, w);
    crMemcpy((void *) (data_ptr + 48), name, len);

    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackProgramNamedParameter4fvNV (GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v)
{
    crPackProgramNamedParameter4fNV(id, len, name, v[0], v[1], v[2], v[3]);
}

void PACK_APIENTRY crPackProgramNamedParameter4dvNV (GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v)
{
    crPackProgramNamedParameter4dNV(id, len, name, v[0], v[1], v[2], v[3]);
}


void PACK_APIENTRY
crPackAreProgramsResidentNV(GLsizei n, const GLuint * programs,
                                                        GLboolean *residences, GLboolean *return_val,
                                                        int *writeback)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    int packet_length;

    (void) return_val; /* Caller must compute this from residences!!! */

    packet_length = sizeof(int) +   /* packet length */
        sizeof(GLenum) +                        /* extend opcode */
        sizeof(n) +                                 /* num programs */
        n * sizeof(*programs) +         /* programs */
        8 + 8;

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(4, GLenum, CR_AREPROGRAMSRESIDENTNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLsizei, n);
    crMemcpy(data_ptr + 12, programs, n * sizeof(*programs));
    WRITE_NETWORK_POINTER(12 + n * sizeof(*programs),   (void *) residences);
    WRITE_NETWORK_POINTER(20 + n * sizeof(*programs), (void *) writeback);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackGetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte *name, GLfloat *params, int *writeback)
{
    int packet_length = 32 + len;
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_GETPROGRAMNAMEDPARAMETERFVNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLuint, id);
    WRITE_DATA(12, GLsizei, len);
    crMemcpy(data_ptr + 16, name, len);
    WRITE_NETWORK_POINTER(16 + len, (void *) params);
    WRITE_NETWORK_POINTER(16 + len + 8, (void *) writeback);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackGetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte *name, GLdouble *params, int *writeback)
{
    int packet_length = 32 + len;
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_GETPROGRAMNAMEDPARAMETERDVNV_EXTEND_OPCODE);
    WRITE_DATA(8, GLuint, id);
    WRITE_DATA(12, GLsizei, len);
    crMemcpy(data_ptr + 16, name, len);
    WRITE_NETWORK_POINTER(16 + len, (void *) params);
    WRITE_NETWORK_POINTER(16 + len + 8, (void *) writeback);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackDeleteProgramsARB(GLsizei n, const GLuint *ids)
{
    unsigned char *data_ptr = NULL;
    int packet_length = sizeof(GLenum) + sizeof(n) + n * sizeof(*ids);

    if (!ids)
        return;

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, CR_DELETEPROGRAMSARB_EXTEND_OPCODE);
    WRITE_DATA(4, GLsizei, n);
    crMemcpy(data_ptr + 8, ids, n * sizeof(*ids));
    crHugePacket(CR_EXTEND_OPCODE, data_ptr);
    crPackFree(data_ptr);
}


void PACK_APIENTRY  crPackProgramStringARB(GLenum target, GLenum format, GLsizei len, const void *string)
{
    const int packet_length = 20 + len;
    unsigned char *data_ptr = NULL;
    CR_GET_PACKER_CONTEXT(pc);

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(4, GLenum, CR_PROGRAMSTRINGARB_EXTEND_OPCODE);
    WRITE_DATA(8, GLenum, target);
    WRITE_DATA(12, GLuint, format);
    WRITE_DATA(16, GLsizei, len);
    crMemcpy((void *) (data_ptr + 20), string, len);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


/*
 * Can't easily auto-generate these functions since there aren't
 * non-vector versions.
 */

void PACK_APIENTRY crPackVertexAttrib4NbvARB(GLuint index, const GLbyte *v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 8);
    pc->current.c.vertexAttrib.b4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLbyte, v[0]);
    WRITE_DATA(5, GLbyte, v[1]);
    WRITE_DATA(6, GLbyte, v[2]);
    WRITE_DATA(7, GLbyte, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4NBVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4NivARB(GLuint index, const GLint *v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 20);
    pc->current.c.vertexAttrib.i4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLint, v[0]);
    WRITE_DATA(8, GLint, v[1]);
    WRITE_DATA(12, GLint, v[2]);
    WRITE_DATA(16, GLint, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4NIVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4NsvARB(GLuint index, const GLshort *v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 12);
    pc->current.c.vertexAttrib.s4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLshort, v[0]);
    WRITE_DATA(6, GLshort, v[1]);
    WRITE_DATA(8, GLshort, v[2]);
    WRITE_DATA(10, GLshort, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4NSVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4NubvARB(GLuint index, const GLubyte * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 8);
    pc->current.c.vertexAttrib.ub4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLubyte, v[0]);
    WRITE_DATA(5, GLubyte, v[1]);
    WRITE_DATA(6, GLubyte, v[2]);
    WRITE_DATA(7, GLubyte, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4NUBVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4NuivARB(GLuint index, const GLuint * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 20);
    pc->current.c.vertexAttrib.ui4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLuint, v[0]);
    WRITE_DATA(8, GLuint, v[1]);
    WRITE_DATA(12, GLuint, v[2]);
    WRITE_DATA(16, GLuint, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4NUIVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4NusvARB(GLuint index, const GLushort * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 12);
    pc->current.c.vertexAttrib.s4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLushort, v[0]);
    WRITE_DATA(6, GLushort, v[1]);
    WRITE_DATA(8, GLushort, v[2]);
    WRITE_DATA(10, GLushort, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4NUSVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4bvARB(GLuint index, const GLbyte * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 8);
    pc->current.c.vertexAttrib.b4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLbyte, v[0]);
    WRITE_DATA(5, GLbyte, v[1]);
    WRITE_DATA(6, GLbyte, v[2]);
    WRITE_DATA(7, GLbyte, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4BVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4ivARB(GLuint index, const GLint * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 20);
    pc->current.c.vertexAttrib.i4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLint, v[0]);
    WRITE_DATA(8, GLint, v[1]);
    WRITE_DATA(12, GLint, v[2]);
    WRITE_DATA(16, GLint, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4IVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4uivARB(GLuint index, const GLuint * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 20);
    pc->current.c.vertexAttrib.ui4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLuint, v[0]);
    WRITE_DATA(8, GLuint, v[1]);
    WRITE_DATA(12, GLuint, v[2]);
    WRITE_DATA(16, GLuint, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4UIVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttrib4usvARB(GLuint index, const GLushort * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 12);
    pc->current.c.vertexAttrib.s4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLushort, v[0]);
    WRITE_DATA(6, GLushort, v[1]);
    WRITE_DATA(8, GLushort, v[2]);
    WRITE_DATA(10, GLushort, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4USVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackVertexAttrib4ubvARB(GLuint index, const GLubyte * v)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr = NULL;
    CR_GET_BUFFERED_POINTER(pc, 8);
    pc->current.c.vertexAttrib.ub4[index] = data_ptr + 4;
    pc->current.attribsUsedMask |= (1 << index);
    WRITE_DATA(0, GLuint, index);
    WRITE_DATA(4, GLubyte, v[0]);
    WRITE_DATA(5, GLubyte, v[1]);
    WRITE_DATA(6, GLubyte, v[2]);
    WRITE_DATA(7, GLubyte, v[3]);
    WRITE_OPCODE(pc, CR_VERTEXATTRIB4UBVARB_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

