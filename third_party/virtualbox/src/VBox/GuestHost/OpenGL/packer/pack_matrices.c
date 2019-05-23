/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"

void PACK_APIENTRY crPackMultMatrixd(const GLdouble *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DOUBLE(0*sizeof(double), m[ 0]);
    WRITE_DOUBLE(1*sizeof(double), m[ 1]);
    WRITE_DOUBLE(2*sizeof(double), m[ 2]);
    WRITE_DOUBLE(3*sizeof(double), m[ 3]);
    WRITE_DOUBLE(4*sizeof(double), m[ 4]);
    WRITE_DOUBLE(5*sizeof(double), m[ 5]);
    WRITE_DOUBLE(6*sizeof(double), m[ 6]);
    WRITE_DOUBLE(7*sizeof(double), m[ 7]);
    WRITE_DOUBLE(8*sizeof(double), m[ 8]);
    WRITE_DOUBLE(9*sizeof(double), m[ 9]);
    WRITE_DOUBLE(10*sizeof(double), m[10]);
    WRITE_DOUBLE(11*sizeof(double), m[11]);
    WRITE_DOUBLE(12*sizeof(double), m[12]);
    WRITE_DOUBLE(13*sizeof(double), m[13]);
    WRITE_DOUBLE(14*sizeof(double), m[14]);
    WRITE_DOUBLE(15*sizeof(double), m[15]);
    WRITE_OPCODE(pc, CR_MULTMATRIXD_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackMultMatrixf(const GLfloat *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m);
    CR_GET_BUFFERED_POINTER(pc, packet_length); 
    WRITE_DATA(0*sizeof(GLfloat), GLfloat, m[ 0]);
    WRITE_DATA(1*sizeof(GLfloat), GLfloat, m[ 1]);
    WRITE_DATA(2*sizeof(GLfloat), GLfloat, m[ 2]);
    WRITE_DATA(3*sizeof(GLfloat), GLfloat, m[ 3]);
    WRITE_DATA(4*sizeof(GLfloat), GLfloat, m[ 4]);
    WRITE_DATA(5*sizeof(GLfloat), GLfloat, m[ 5]);
    WRITE_DATA(6*sizeof(GLfloat), GLfloat, m[ 6]);
    WRITE_DATA(7*sizeof(GLfloat), GLfloat, m[ 7]);
    WRITE_DATA(8*sizeof(GLfloat), GLfloat, m[ 8]);
    WRITE_DATA(9*sizeof(GLfloat), GLfloat, m[ 9]);
    WRITE_DATA(10*sizeof(GLfloat), GLfloat, m[10]);
    WRITE_DATA(11*sizeof(GLfloat), GLfloat, m[11]);
    WRITE_DATA(12*sizeof(GLfloat), GLfloat, m[12]);
    WRITE_DATA(13*sizeof(GLfloat), GLfloat, m[13]);
    WRITE_DATA(14*sizeof(GLfloat), GLfloat, m[14]);
    WRITE_DATA(15*sizeof(GLfloat), GLfloat, m[15]);
    WRITE_OPCODE(pc, CR_MULTMATRIXF_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackLoadMatrixd(const GLdouble *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DOUBLE(0*sizeof(double), m[ 0]);
    WRITE_DOUBLE(1*sizeof(double), m[ 1]);
    WRITE_DOUBLE(2*sizeof(double), m[ 2]);
    WRITE_DOUBLE(3*sizeof(double), m[ 3]);
    WRITE_DOUBLE(4*sizeof(double), m[ 4]);
    WRITE_DOUBLE(5*sizeof(double), m[ 5]);
    WRITE_DOUBLE(6*sizeof(double), m[ 6]);
    WRITE_DOUBLE(7*sizeof(double), m[ 7]);
    WRITE_DOUBLE(8*sizeof(double), m[ 8]);
    WRITE_DOUBLE(9*sizeof(double), m[ 9]);
    WRITE_DOUBLE(10*sizeof(double), m[10]);
    WRITE_DOUBLE(11*sizeof(double), m[11]);
    WRITE_DOUBLE(12*sizeof(double), m[12]);
    WRITE_DOUBLE(13*sizeof(double), m[13]);
    WRITE_DOUBLE(14*sizeof(double), m[14]);
    WRITE_DOUBLE(15*sizeof(double), m[15]);
    WRITE_OPCODE(pc, CR_LOADMATRIXD_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackLoadMatrixf(const GLfloat *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0*sizeof(GLfloat), GLfloat, m[ 0]);
    WRITE_DATA(1*sizeof(GLfloat), GLfloat, m[ 1]);
    WRITE_DATA(2*sizeof(GLfloat), GLfloat, m[ 2]);
    WRITE_DATA(3*sizeof(GLfloat), GLfloat, m[ 3]);
    WRITE_DATA(4*sizeof(GLfloat), GLfloat, m[ 4]);
    WRITE_DATA(5*sizeof(GLfloat), GLfloat, m[ 5]);
    WRITE_DATA(6*sizeof(GLfloat), GLfloat, m[ 6]);
    WRITE_DATA(7*sizeof(GLfloat), GLfloat, m[ 7]);
    WRITE_DATA(8*sizeof(GLfloat), GLfloat, m[ 8]);
    WRITE_DATA(9*sizeof(GLfloat), GLfloat, m[ 9]);
    WRITE_DATA(10*sizeof(GLfloat), GLfloat, m[10]);
    WRITE_DATA(11*sizeof(GLfloat), GLfloat, m[11]);
    WRITE_DATA(12*sizeof(GLfloat), GLfloat, m[12]);
    WRITE_DATA(13*sizeof(GLfloat), GLfloat, m[13]);
    WRITE_DATA(14*sizeof(GLfloat), GLfloat, m[14]);
    WRITE_DATA(15*sizeof(GLfloat), GLfloat, m[15]);
    WRITE_OPCODE(pc, CR_LOADMATRIXF_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackMultTransposeMatrixdARB(const GLdouble *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m) + sizeof(GLint) + sizeof(GLenum);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_MULTTRANSPOSEMATRIXDARB_EXTEND_OPCODE);
    WRITE_DOUBLE(8 + 0*sizeof(double), m[ 0]);
    WRITE_DOUBLE(8 + 1*sizeof(double), m[ 1]);
    WRITE_DOUBLE(8 + 2*sizeof(double), m[ 2]);
    WRITE_DOUBLE(8 + 3*sizeof(double), m[ 3]);
    WRITE_DOUBLE(8 + 4*sizeof(double), m[ 4]);
    WRITE_DOUBLE(8 + 5*sizeof(double), m[ 5]);
    WRITE_DOUBLE(8 + 6*sizeof(double), m[ 6]);
    WRITE_DOUBLE(8 + 7*sizeof(double), m[ 7]);
    WRITE_DOUBLE(8 + 8*sizeof(double), m[ 8]);
    WRITE_DOUBLE(8 + 9*sizeof(double), m[ 9]);
    WRITE_DOUBLE(8 + 10*sizeof(double), m[10]);
    WRITE_DOUBLE(8 + 11*sizeof(double), m[11]);
    WRITE_DOUBLE(8 + 12*sizeof(double), m[12]);
    WRITE_DOUBLE(8 + 13*sizeof(double), m[13]);
    WRITE_DOUBLE(8 + 14*sizeof(double), m[14]);
    WRITE_DOUBLE(8 + 15*sizeof(double), m[15]);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackMultTransposeMatrixfARB(const GLfloat *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m) + sizeof(GLint) + sizeof(GLenum);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_MULTTRANSPOSEMATRIXFARB_EXTEND_OPCODE);
    WRITE_DATA(8 + 0*sizeof(GLfloat), GLfloat, m[ 0]);
    WRITE_DATA(8 + 1*sizeof(GLfloat), GLfloat, m[ 1]);
    WRITE_DATA(8 + 2*sizeof(GLfloat), GLfloat, m[ 2]);
    WRITE_DATA(8 + 3*sizeof(GLfloat), GLfloat, m[ 3]);
    WRITE_DATA(8 + 4*sizeof(GLfloat), GLfloat, m[ 4]);
    WRITE_DATA(8 + 5*sizeof(GLfloat), GLfloat, m[ 5]);
    WRITE_DATA(8 + 6*sizeof(GLfloat), GLfloat, m[ 6]);
    WRITE_DATA(8 + 7*sizeof(GLfloat), GLfloat, m[ 7]);
    WRITE_DATA(8 + 8*sizeof(GLfloat), GLfloat, m[ 8]);
    WRITE_DATA(8 + 9*sizeof(GLfloat), GLfloat, m[ 9]);
    WRITE_DATA(8 + 10*sizeof(GLfloat), GLfloat, m[10]);
    WRITE_DATA(8 + 11*sizeof(GLfloat), GLfloat, m[11]);
    WRITE_DATA(8 + 12*sizeof(GLfloat), GLfloat, m[12]);
    WRITE_DATA(8 + 13*sizeof(GLfloat), GLfloat, m[13]);
    WRITE_DATA(8 + 14*sizeof(GLfloat), GLfloat, m[14]);
    WRITE_DATA(8 + 15*sizeof(GLfloat), GLfloat, m[15]);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackLoadTransposeMatrixdARB(const GLdouble *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m) + sizeof(GLint) + sizeof(GLenum);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_LOADTRANSPOSEMATRIXDARB_EXTEND_OPCODE);
    WRITE_DOUBLE(8 + 0*sizeof(double), m[ 0]);
    WRITE_DOUBLE(8 + 1*sizeof(double), m[ 1]);
    WRITE_DOUBLE(8 + 2*sizeof(double), m[ 2]);
    WRITE_DOUBLE(8 + 3*sizeof(double), m[ 3]);
    WRITE_DOUBLE(8 + 4*sizeof(double), m[ 4]);
    WRITE_DOUBLE(8 + 5*sizeof(double), m[ 5]);
    WRITE_DOUBLE(8 + 6*sizeof(double), m[ 6]);
    WRITE_DOUBLE(8 + 7*sizeof(double), m[ 7]);
    WRITE_DOUBLE(8 + 8*sizeof(double), m[ 8]);
    WRITE_DOUBLE(8 + 9*sizeof(double), m[ 9]);
    WRITE_DOUBLE(8 + 10*sizeof(double), m[10]);
    WRITE_DOUBLE(8 + 11*sizeof(double), m[11]);
    WRITE_DOUBLE(8 + 12*sizeof(double), m[12]);
    WRITE_DOUBLE(8 + 13*sizeof(double), m[13]);
    WRITE_DOUBLE(8 + 14*sizeof(double), m[14]);
    WRITE_DOUBLE(8 + 15*sizeof(double), m[15]);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackLoadTransposeMatrixfARB(const GLfloat *m)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = 16*sizeof(*m) + sizeof(GLint) + sizeof(GLenum);
    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, GLint, packet_length);
    WRITE_DATA(4, GLenum, CR_LOADTRANSPOSEMATRIXFARB_EXTEND_OPCODE);
    WRITE_DATA(8 + 0*sizeof(GLfloat), GLfloat, m[ 0]);
    WRITE_DATA(8 + 1*sizeof(GLfloat), GLfloat, m[ 1]);
    WRITE_DATA(8 + 2*sizeof(GLfloat), GLfloat, m[ 2]);
    WRITE_DATA(8 + 3*sizeof(GLfloat), GLfloat, m[ 3]);
    WRITE_DATA(8 + 4*sizeof(GLfloat), GLfloat, m[ 4]);
    WRITE_DATA(8 + 5*sizeof(GLfloat), GLfloat, m[ 5]);
    WRITE_DATA(8 + 6*sizeof(GLfloat), GLfloat, m[ 6]);
    WRITE_DATA(8 + 7*sizeof(GLfloat), GLfloat, m[ 7]);
    WRITE_DATA(8 + 8*sizeof(GLfloat), GLfloat, m[ 8]);
    WRITE_DATA(8 + 9*sizeof(GLfloat), GLfloat, m[ 9]);
    WRITE_DATA(8 + 10*sizeof(GLfloat), GLfloat, m[10]);
    WRITE_DATA(8 + 11*sizeof(GLfloat), GLfloat, m[11]);
    WRITE_DATA(8 + 12*sizeof(GLfloat), GLfloat, m[12]);
    WRITE_DATA(8 + 13*sizeof(GLfloat), GLfloat, m[13]);
    WRITE_DATA(8 + 14*sizeof(GLfloat), GLfloat, m[14]);
    WRITE_DATA(8 + 15*sizeof(GLfloat), GLfloat, m[15]);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
