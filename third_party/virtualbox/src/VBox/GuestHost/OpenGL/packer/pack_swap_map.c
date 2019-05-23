/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_error.h"


/* Note -- for these packets, the ustride and vstride are implicit,
 * and are computed into the packet instead of copied.
 */

static int __gl_Map2NumComponents(GLenum target)
{
    switch(target)
    {
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_3:
        return 3;
    case GL_MAP2_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
        return 4;
    case GL_MAP2_INDEX:
    case GL_MAP2_TEXTURE_COORD_1:
        return 1;
    case GL_MAP2_TEXTURE_COORD_2:
        return 2;
    default:
        return -1;
    }
}

static int __gl_Map1NumComponents(GLenum target)
{
    switch(target)
    {
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_3:
        return 3;
    case GL_MAP1_VERTEX_4:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
        return 4;
    case GL_MAP1_INDEX:
    case GL_MAP1_TEXTURE_COORD_1:
        return 1;
    case GL_MAP1_TEXTURE_COORD_2:
        return 2;
    default:
        return -1;
    }
}

void PACK_APIENTRY crPackMap2dSWAP(GLenum target, GLdouble u1, 
        GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, 
        GLint vstride, GLint vorder, const GLdouble *points)
{
    unsigned char *data_ptr;
    int u,v;
    int comp;
    GLdouble *dest_data, *src_data;
    int packet_length = 
        sizeof(target) + 
        sizeof(u1) +
        sizeof(u2) +
        sizeof(uorder) +
        sizeof(ustride) +
        sizeof(v1) +
        sizeof(v2) + 
        sizeof(vorder) +
        sizeof(vstride);

    int num_components = __gl_Map2NumComponents(target);
    if (num_components < 0)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackMap2d(bad target)");
        return;
    }

    packet_length += num_components*uorder*vorder*sizeof(*points);

    data_ptr = (unsigned char *) crPackAlloc(packet_length);

    WRITE_DATA(0, GLenum, SWAP32(target));
    WRITE_SWAPPED_DOUBLE(4, u1);
    WRITE_SWAPPED_DOUBLE(12, u2);
    WRITE_DATA(20, GLint, SWAP32(num_components));
    WRITE_DATA(24, GLint, SWAP32(uorder));
    WRITE_SWAPPED_DOUBLE(28, v1);
    WRITE_SWAPPED_DOUBLE(36, v2);
    WRITE_DATA(44, GLint, SWAP32(num_components*uorder));
    WRITE_DATA(48, GLint, SWAP32(vorder));

    dest_data = (GLdouble *) (data_ptr + 52);
    src_data = (GLdouble *) points;
    for (v = 0 ; v < vorder ; v++)
    {
        for (u = 0 ; u < uorder ; u++)
        {
            for (comp = 0 ; comp < num_components ; comp++)
            {
                WRITE_SWAPPED_DOUBLE(((unsigned char *) dest_data + comp*sizeof(*points)) - data_ptr, *(src_data + comp));
            }
            dest_data += num_components;
            src_data += ustride;
        }
        src_data += vstride - ustride*uorder;
    }

    crHugePacket(CR_MAP2D_OPCODE, data_ptr);
    crPackFree(data_ptr);
}

void PACK_APIENTRY crPackMap2fSWAP(GLenum target, GLfloat u1, 
        GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, 
        GLint vstride, GLint vorder, const GLfloat *points)
{
    unsigned char *data_ptr;
    int u,v;
    int comp;
    GLfloat *dest_data, *src_data;
    int packet_length = 
        sizeof(target) + 
        sizeof(u1) +
        sizeof(u2) +
        sizeof(uorder) +
        sizeof(ustride) +
        sizeof(v1) +
        sizeof(v2) + 
        sizeof(vorder) +
        sizeof(vstride);

    int num_components = __gl_Map2NumComponents(target);
    if (num_components < 0)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackMap2f(bad target)");
        return;
    }

    packet_length += num_components*uorder*vorder*sizeof(*points);

    data_ptr = (unsigned char *) crPackAlloc(packet_length);

    WRITE_DATA(0, GLenum, SWAP32(target));
    WRITE_DATA(4, GLuint, SWAPFLOAT(u1));
    WRITE_DATA(8, GLuint, SWAPFLOAT(u2));
    WRITE_DATA(12, GLint, SWAP32(num_components));
    WRITE_DATA(16, GLint, SWAP32(uorder));
    WRITE_DATA(20, GLuint, SWAPFLOAT(v1));
    WRITE_DATA(24, GLuint, SWAPFLOAT(v2));
    WRITE_DATA(28, GLint, SWAP32(num_components*uorder));
    WRITE_DATA(32, GLint, SWAP32(vorder));

    dest_data = (GLfloat *) (data_ptr + 36);
    src_data = (GLfloat *) points;
    for (v = 0 ; v < vorder ; v++)
    {
        for (u = 0 ; u < uorder ; u++)
        {
            for (comp = 0 ; comp < num_components ; comp++)
            {
                WRITE_DATA((unsigned char *) dest_data + comp*sizeof(*points) - data_ptr, GLuint, SWAPFLOAT(*(src_data + comp)));
            }
            dest_data += num_components;
            src_data += ustride;
        }
        src_data += vstride - ustride*uorder;
    }

    crHugePacket(CR_MAP2F_OPCODE, data_ptr);
    crPackFree(data_ptr);
}

void PACK_APIENTRY crPackMap1dSWAP(GLenum target, GLdouble u1,
        GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    unsigned char *data_ptr;
    int packet_length = 
        sizeof(target) + 
        sizeof(u1) +
        sizeof(u2) + 
        sizeof(stride) + 
        sizeof(order);

    int num_components = __gl_Map1NumComponents(target);
    GLdouble *src_data, *dest_data;
    int u;
    int comp;

    if (num_components < 0)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackMap1d(bad target)");
        return;
    }

    packet_length += num_components * order * sizeof(*points);

    data_ptr = (unsigned char *) crPackAlloc(packet_length);

    WRITE_DATA(0, GLenum, SWAP32(target));
    WRITE_SWAPPED_DOUBLE(4, u1);
    WRITE_SWAPPED_DOUBLE(12, u2);
    WRITE_DATA(20, GLint, SWAP32(num_components));
    WRITE_DATA(24, GLint, SWAP32(order));

    dest_data = (GLdouble *) (data_ptr + 28);
    src_data = (GLdouble *) points;
    for (u = 0 ; u < order ; u++)
    {
        for (comp = 0 ; comp < num_components ; comp++)
        {
            WRITE_SWAPPED_DOUBLE((unsigned char *) dest_data + comp*sizeof(*points) - data_ptr, *(src_data + comp));
        }
        dest_data += num_components;
        src_data += stride;
    }

    crHugePacket(CR_MAP1D_OPCODE, data_ptr);
    crPackFree(data_ptr);
}

void PACK_APIENTRY crPackMap1fSWAP(GLenum target, GLfloat u1,
        GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    unsigned char *data_ptr;
    int packet_length = 
        sizeof(target) + 
        sizeof(u1) +
        sizeof(u2) + 
        sizeof(stride) + 
        sizeof(order);

    int num_components = __gl_Map1NumComponents(target);
    GLfloat *src_data, *dest_data;
    int u;
    int comp;

    if (num_components < 0)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackMap1f(bad target)");
        return;
    }

    packet_length += num_components * order * sizeof(*points);

    data_ptr = (unsigned char *) crPackAlloc(packet_length);

    WRITE_DATA(0, GLenum, SWAP32(target));
    WRITE_DATA(4, GLuint, SWAPFLOAT(u1));
    WRITE_DATA(8, GLuint, SWAPFLOAT(u2));
    WRITE_DATA(12, GLint, SWAP32(num_components));
    WRITE_DATA(16, GLint, SWAP32(order));

    dest_data = (GLfloat *) (data_ptr + 20);
    src_data = (GLfloat *) points;
    for (u = 0 ; u < order ; u++)
    {
        for (comp = 0 ; comp < num_components ; comp++)
        {
            WRITE_DATA((unsigned char *) dest_data + comp*sizeof(*points) - data_ptr, GLuint, SWAPFLOAT(*(src_data + comp)));
        }
        dest_data += num_components;
        src_data += stride;
    }

    crHugePacket(CR_MAP1F_OPCODE, data_ptr);
    crPackFree(data_ptr);
}
