/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_pixeldata.h"
#include "cr_error.h"
#include "cr_string.h"
#include "cr_version.h"
#include "cr_glstate.h"

void PACK_APIENTRY
crPackTexImage1D(GLenum target, GLint level,
                 GLint internalformat, GLsizei width, GLint border,
                 GLenum format, GLenum type, const GLvoid * pixels,
                 const CRPixelPackState * unpackstate)
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    packet_length =
        sizeof(target) +
        sizeof(level) +
        sizeof(internalformat) +
        sizeof(width) +
        sizeof(border) + sizeof(format) + sizeof(type) + sizeof(int) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += crImageSize(format, type, width, 1);
    }

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, target);
    WRITE_DATA(4, GLint, level);
    WRITE_DATA(8, GLint, internalformat);
    WRITE_DATA(12, GLsizei, width);
    WRITE_DATA(16, GLint, border);
    WRITE_DATA(20, GLenum, format);
    WRITE_DATA(24, GLenum, type);
    WRITE_DATA(28, int, noimagedata);
    WRITE_DATA(32, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        crPixelCopy1D((void *) (data_ptr + 36), format, type,
                      pixels, format, type, width, unpackstate);
    }

    crHugePacket(CR_TEXIMAGE1D_OPCODE, data_ptr);
    crPackFree( data_ptr );
}

void PACK_APIENTRY
crPackTexImage2D(GLenum target, GLint level,
                 GLint internalformat, GLsizei width, GLsizei height,
                 GLint border, GLenum format, GLenum type,
                 const GLvoid * pixels, const CRPixelPackState * unpackstate)
{
    unsigned char *data_ptr;
    int packet_length;
    const int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);
    const int is_distrib = ((type == GL_TRUE) || (type == GL_FALSE));
    int distrib_buf_len = 0;

    packet_length =
        sizeof(target) +
        sizeof(level) +
        sizeof(internalformat) +
        sizeof(width) +
        sizeof(height) +
        sizeof(border) + sizeof(format) + sizeof(type) + sizeof(int) + sizeof(GLint);

    if (!noimagedata)
    {
        if (is_distrib)
        {
            /* Pixels is a zero-terminated filename, followed by the usual image
             * data if type == GL_TRUE.
             * Also note that the image data can't have any unusual pixel packing
             * parameters.
             */
            CRASSERT(format == GL_RGB);
            distrib_buf_len = crStrlen(pixels) + 1 +
                ((type == GL_TRUE) ? width * height * 3 : 0);
            packet_length += distrib_buf_len;
        }
        else
        {
            packet_length += crImageSize(format, type, width, height);
        }
    }

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, target);
    WRITE_DATA(4, GLint, level);
    WRITE_DATA(8, GLint, internalformat);
    WRITE_DATA(12, GLsizei, width);
    WRITE_DATA(16, GLsizei, height);
    WRITE_DATA(20, GLint, border);
    WRITE_DATA(24, GLenum, format);
    WRITE_DATA(28, GLenum, type);
    WRITE_DATA(32, int, noimagedata);
    WRITE_DATA(36, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        if (is_distrib)
        {
            crMemcpy((void *) (data_ptr + 40), pixels, distrib_buf_len);
        }
        else
        {
            crPixelCopy2D(width, height,
                                        (void *) (data_ptr + 40), /* dest image addr */
                                        format, type,             /* dest image format/type */
                                        NULL,   /* dst packing (use default params) */
                                        pixels,       /* src image addr */
                                        format, type, /* src image format/type */
                                        unpackstate);   /* src packing */
        }
    }

    crHugePacket(CR_TEXIMAGE2D_OPCODE, data_ptr);
    crPackFree( data_ptr );
}

#if defined( GL_EXT_texture3D )
void PACK_APIENTRY 
crPackTexImage3DEXT(GLenum target, GLint level,
                    GLenum internalformat,
                    GLsizei width, GLsizei height, GLsizei depth, GLint border,
                    GLenum format, GLenum type, const GLvoid *pixels,
                    const CRPixelPackState *unpackstate )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);
    int is_distrib = ( (type == GL_TRUE) || (type == GL_FALSE) ) ;
    int distrib_buf_len = 0;
    int tex_size = 0;

    packet_length =
        sizeof( target ) +
        sizeof( level ) +
        sizeof( internalformat ) +
        sizeof( width ) +
        sizeof( height ) +
        sizeof( depth ) +
        sizeof( border ) +
        sizeof( format ) +
        sizeof( type ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        if ( is_distrib )
        {
            distrib_buf_len = crStrlen( pixels ) + 1 +
                ( (type == GL_TRUE) ? width*height*3 : 0 ) ;
            packet_length += distrib_buf_len ;
        }
        else
        {
            tex_size = crTextureSize( format, type, width, height, depth );
            packet_length += tex_size;
        }
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, target );
    WRITE_DATA( 4, GLint, level );
    WRITE_DATA( 8, GLint, internalformat );
    WRITE_DATA( 12, GLsizei, width );
    WRITE_DATA( 16, GLsizei, height );
    WRITE_DATA( 20, GLsizei, depth );
    WRITE_DATA( 24, GLint, border );
    WRITE_DATA( 28, GLenum, format );
    WRITE_DATA( 32, GLenum, type );
    WRITE_DATA( 36, int, noimagedata );
    WRITE_DATA( 40, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        if ( is_distrib )
        {
            crMemcpy( (void*)(data_ptr + 44), pixels, distrib_buf_len ) ;
        }
        else
        {               
            crPixelCopy3D( width, height, depth,
                                         (void *)(data_ptr + 44), format, type, NULL,
                                         pixels, format, type, unpackstate );
        }
    }

    crHugePacket( CR_TEXIMAGE3DEXT_OPCODE, data_ptr );
    crPackFree( data_ptr );
}
#endif /* GL_EXT_texture3D */

#ifdef CR_OPENGL_VERSION_1_2
void PACK_APIENTRY 
crPackTexImage3D(GLenum target, GLint level,
                 GLint internalformat,
                 GLsizei width, GLsizei height,
                 GLsizei depth, GLint border,
                 GLenum format, GLenum type,
                 const GLvoid *pixels,
                 const CRPixelPackState *unpackstate )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);
    int is_distrib = ( (type == GL_TRUE) || (type == GL_FALSE) ) ;
    int distrib_buf_len = 0;
    int tex_size = 0;

    packet_length =
        sizeof( target ) +
        sizeof( level ) +
        sizeof( internalformat ) +
        sizeof( width ) +
        sizeof( height ) +
        sizeof( depth ) +
        sizeof( border ) +
        sizeof( format ) +
        sizeof( type ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        if ( is_distrib )
        {
            distrib_buf_len = crStrlen( pixels ) + 1 +
                ( (type == GL_TRUE) ? width*height*3 : 0 ) ;
            packet_length += distrib_buf_len ;
        }
        else
        {
            tex_size = crTextureSize( format, type, width, height, depth );
            packet_length += tex_size;
        }
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, target );
    WRITE_DATA( 4, GLint, level );
    WRITE_DATA( 8, GLint, internalformat );
    WRITE_DATA( 12, GLsizei, width );
    WRITE_DATA( 16, GLsizei, height );
    WRITE_DATA( 20, GLsizei, depth );
    WRITE_DATA( 24, GLint, border );
    WRITE_DATA( 28, GLenum, format );
    WRITE_DATA( 32, GLenum, type );
    WRITE_DATA( 36, int, noimagedata );
    WRITE_DATA( 40, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        if ( is_distrib )
        {
            crMemcpy( (void*)(data_ptr + 44), pixels, distrib_buf_len ) ;
        }
        else
        {               
            crPixelCopy3D( width, height, depth,
                                         (void *)(data_ptr + 44), format, type, NULL,
                                         pixels, format, type, unpackstate );
        }
    }

    crHugePacket( CR_TEXIMAGE3D_OPCODE, data_ptr );
    crPackFree( data_ptr );
}
#endif /* CR_OPENGL_VERSION_1_2 */


void PACK_APIENTRY
crPackDeleteTextures(GLsizei n, const GLuint * textures)
{
    unsigned char *data_ptr;
    int packet_length =
        sizeof( n ) +
        n * sizeof( *textures );

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLsizei, n);
    crMemcpy(data_ptr + 4, textures, n * sizeof(*textures));
    crHugePacket(CR_DELETETEXTURES_OPCODE, data_ptr);
    crPackFree( data_ptr );
}

static void
__handleTexEnvData(GLenum target, GLenum pname, const GLfloat * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int params_length;

    int packet_length =
        sizeof( int ) +
        sizeof( target ) +
        sizeof( pname );

    if (pname == GL_TEXTURE_ENV_COLOR)
    {
        params_length = 4 * sizeof(*params);
    }
    else
    {
        params_length = sizeof(*params);
    }

    packet_length += params_length;

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(sizeof(int) + 0, GLenum, target);
    WRITE_DATA(sizeof(int) + 4, GLenum, pname);
    crMemcpy(data_ptr + sizeof(int) + 8, params, params_length);
}


void PACK_APIENTRY
crPackTexEnvfv(GLenum target, GLenum pname, const GLfloat * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    __handleTexEnvData(target, pname, params);
    WRITE_OPCODE(pc, CR_TEXENVFV_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackTexEnviv(GLenum target, GLenum pname, const GLint * params)
{
    /* floats and ints are the same size, so the packing should be the same */
    CR_GET_PACKER_CONTEXT(pc);
    __handleTexEnvData(target, pname, (const GLfloat *) params);
    WRITE_OPCODE(pc, CR_TEXENVIV_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    crPackTexEnvfv(target, pname, &param);
}

void PACK_APIENTRY
crPackTexEnvi(GLenum target, GLenum pname, GLint param)
{
    crPackTexEnviv(target, pname, &param);
}

void PACK_APIENTRY
crPackPrioritizeTextures(GLsizei n, const GLuint * textures,
                         const GLclampf * priorities)
{
    unsigned char *data_ptr;
    int packet_length =
        sizeof(n) +
        n * sizeof(*textures) +
        n * sizeof(*priorities);

    data_ptr = (unsigned char *) crPackAlloc(packet_length);

    WRITE_DATA(0, GLsizei, n);
    crMemcpy(data_ptr + 4, textures, n * sizeof(*textures));
    crMemcpy(data_ptr + 4 + n * sizeof(*textures),
                 priorities, n * sizeof(*priorities));

    crHugePacket(CR_PRIORITIZETEXTURES_OPCODE, data_ptr);
    crPackFree( data_ptr );
}

static void
__handleTexGenData(GLenum coord, GLenum pname,
                                     int sizeof_param, const GLvoid * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length =
        sizeof(int) + sizeof(coord) + sizeof(pname) + sizeof_param;
    int params_length = sizeof_param;
    if (pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE)
    {
        packet_length += 3 * sizeof_param;
        params_length += 3 * sizeof_param;
    }

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(sizeof(int) + 0, GLenum, coord);
    WRITE_DATA(sizeof(int) + 4, GLenum, pname);
    crMemcpy(data_ptr + sizeof(int) + 8, params, params_length);
}

void PACK_APIENTRY
crPackTexGendv(GLenum coord, GLenum pname, const GLdouble * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    __handleTexGenData(coord, pname, sizeof(*params), params);
    WRITE_OPCODE(pc, CR_TEXGENDV_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackTexGenfv(GLenum coord, GLenum pname, const GLfloat * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    __handleTexGenData(coord, pname, sizeof(*params), params);
    WRITE_OPCODE(pc, CR_TEXGENFV_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackTexGeniv(GLenum coord, GLenum pname, const GLint * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    __handleTexGenData(coord, pname, sizeof(*params), params);
    WRITE_OPCODE(pc, CR_TEXGENIV_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    crPackTexGendv(coord, pname, &param);
}

void PACK_APIENTRY
crPackTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    crPackTexGenfv(coord, pname, &param);
}

void PACK_APIENTRY
crPackTexGeni(GLenum coord, GLenum pname, GLint param)
{
    crPackTexGeniv(coord, pname, &param);
}

static GLboolean
__handleTexParameterData(GLenum target, GLenum pname, const GLfloat * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = sizeof(int) + sizeof(target) + sizeof(pname);
    int num_params = 0;

    switch (pname)
    {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
#ifdef GL_TEXTURE_PRIORITY
    case GL_TEXTURE_PRIORITY:
#endif
      num_params = 1;
        break;
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        num_params = 1;
        break;
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
        num_params = 1;
        break;
    case GL_TEXTURE_BORDER_COLOR:
        num_params = 4;
        break;
#ifdef CR_ARB_shadow
    case GL_TEXTURE_COMPARE_MODE_ARB:
    case GL_TEXTURE_COMPARE_FUNC_ARB:
        num_params = 1;
        break;
#endif
#ifdef CR_ARB_shadow_ambient
    case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
        num_params = 1;
        break;
#endif
#ifdef CR_ARB_depth_texture
    case GL_DEPTH_TEXTURE_MODE_ARB:
        num_params = 1;
        break;
#endif
#ifdef CR_SGIS_generate_mipmap
    case GL_GENERATE_MIPMAP_SGIS:
        num_params = 1;
        break;
#endif
    default:
        num_params = __packTexParameterNumParams(pname);
        if (!num_params)
        {
            __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                    "crPackTexParameter(bad pname)");
            return GL_FALSE;
        }
    }
    packet_length += num_params * sizeof(*params);

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(sizeof(int) + 0, GLenum, target);
    WRITE_DATA(sizeof(int) + 4, GLenum, pname);
    crMemcpy(data_ptr + sizeof(int) + 8, params, num_params * sizeof(*params));
    return GL_TRUE;
}

void PACK_APIENTRY
crPackTexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    if (__handleTexParameterData(target, pname, params))
    {
        WRITE_OPCODE(pc, CR_TEXPARAMETERFV_OPCODE);
        CR_UNLOCK_PACKER_CONTEXT(pc);
    }
}

void PACK_APIENTRY
crPackTexParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    CR_GET_PACKER_CONTEXT(pc);
    if (__handleTexParameterData(target, pname, (GLfloat *) params))
    {
        WRITE_OPCODE(pc, CR_TEXPARAMETERIV_OPCODE);
        CR_UNLOCK_PACKER_CONTEXT(pc);
    }
}

void PACK_APIENTRY
crPackTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    crPackTexParameterfv(target, pname, &param);
}

void PACK_APIENTRY
crPackTexParameteri(GLenum target, GLenum pname, GLint param)
{
    crPackTexParameteriv(target, pname, &param);
}

#ifdef CR_OPENGL_VERSION_1_2
void PACK_APIENTRY
crPackTexSubImage3D(GLenum target, GLint level,
                    GLint xoffset, GLint yoffset, GLint zoffset,
                    GLsizei width, GLsizei height, GLsizei depth,
                    GLenum format, GLenum type, const GLvoid * pixels,
                    const CRPixelPackState * unpackstate)
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    packet_length =
        sizeof(target) +
        sizeof(level) +
        sizeof(xoffset) +
        sizeof(yoffset) +
        sizeof(zoffset) +
        sizeof(width) +
        sizeof(height) +
        sizeof(depth) +
        sizeof(format) +
        sizeof(type) + sizeof(int) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += crTextureSize(format, type, width, height, depth);
    }

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, target);
    WRITE_DATA(4, GLint, level);
    WRITE_DATA(8, GLint, xoffset);
    WRITE_DATA(12, GLint, yoffset);
    WRITE_DATA(16, GLint, zoffset);
    WRITE_DATA(20, GLsizei, width);
    WRITE_DATA(24, GLsizei, height);
    WRITE_DATA(28, GLsizei, depth);
    WRITE_DATA(32, GLenum, format);
    WRITE_DATA(36, GLenum, type);
    WRITE_DATA(40, GLint, noimagedata);
    WRITE_DATA(44, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        crPixelCopy3D(width, height, depth, (GLvoid *) (data_ptr + 48), format, type, NULL, /* dst */
                      pixels, format, type, unpackstate); /* src */
    }

    crHugePacket(CR_TEXSUBIMAGE3D_OPCODE, data_ptr);
    crPackFree( data_ptr );
}
#endif /* CR_OPENGL_VERSION_1_2 */

void PACK_APIENTRY
crPackTexSubImage2D(GLenum target, GLint level,
                    GLint xoffset, GLint yoffset, GLsizei width,
                    GLsizei height, GLenum format, GLenum type,
                    const GLvoid * pixels,
                    const CRPixelPackState * unpackstate)
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    packet_length =
        sizeof(target) +
        sizeof(level) +
        sizeof(xoffset) +
        sizeof(yoffset) +
        sizeof(width) +
        sizeof(height) +
        sizeof(format) + sizeof(type) + sizeof(int) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += crImageSize(format, type, width, height);
    }

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, target);
    WRITE_DATA(4, GLint, level);
    WRITE_DATA(8, GLint, xoffset);
    WRITE_DATA(12, GLint, yoffset);
    WRITE_DATA(16, GLsizei, width);
    WRITE_DATA(20, GLsizei, height);
    WRITE_DATA(24, GLenum, format);
    WRITE_DATA(28, GLenum, type);
    WRITE_DATA(32, GLint, noimagedata);
    WRITE_DATA(36, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        crPixelCopy2D(width, height, (GLvoid *) (data_ptr + 40), format, type, NULL,    /* dst */
                      pixels, format, type, unpackstate); /* src */
    }

    crHugePacket(CR_TEXSUBIMAGE2D_OPCODE, data_ptr);
    crPackFree( data_ptr );
}

void PACK_APIENTRY
crPackTexSubImage1D(GLenum target, GLint level,
                                        GLint xoffset, GLsizei width, GLenum format, GLenum type,
                                        const GLvoid * pixels,
                                        const CRPixelPackState * unpackstate)
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    packet_length =
        sizeof(target) +
        sizeof(level) +
        sizeof(xoffset) +
        sizeof(width) +
        sizeof(format) + sizeof(type) + sizeof(int) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += crImageSize(format, type, width, 1);
    }

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, target);
    WRITE_DATA(4, GLint, level);
    WRITE_DATA(8, GLint, xoffset);
    WRITE_DATA(12, GLsizei, width);
    WRITE_DATA(16, GLenum, format);
    WRITE_DATA(20, GLenum, type);
    WRITE_DATA(24, GLint, noimagedata);
    WRITE_DATA(28, GLint, (GLint)(uintptr_t) pixels);

    if (!noimagedata)
    {
        crPixelCopy1D((GLvoid *) (data_ptr + 32), format, type,
                      pixels, format, type, width, unpackstate);
    }

    crHugePacket(CR_TEXSUBIMAGE1D_OPCODE, data_ptr);
    crPackFree( data_ptr );
}

void PACK_APIENTRY
crPackAreTexturesResident(GLsizei n, const GLuint * textures,
                                                    GLboolean * residences, GLboolean * return_val,
                                                    int *writeback)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length;

    (void) return_val; /* Caller must compute this from residences!!! */

    packet_length = sizeof(int) +   /* packet length */
        sizeof(GLenum) +                        /* extend-o opcode */
        sizeof(n) +                                 /* num_textures */
        n * sizeof(*textures) +         /* textures */
        8 + 8;

    CR_GET_BUFFERED_POINTER(pc, packet_length);
    WRITE_DATA(0, int, packet_length);
    WRITE_DATA(4, GLenum, CR_ARETEXTURESRESIDENT_EXTEND_OPCODE);
    WRITE_DATA(8, GLsizei, n);
    crMemcpy(data_ptr + 12, textures, n * sizeof(*textures));
    WRITE_NETWORK_POINTER(12 + n * sizeof(*textures),   (void *) residences);
    WRITE_NETWORK_POINTER(20 + n * sizeof(*textures), (void *) writeback);
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


/**********************************************************************
 * Texture compression
 */

void PACK_APIENTRY crPackCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imagesize, const GLvoid *data )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (data == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    /* All extended opcodes have their first 8 bytes predefined:
     * the first four indicate the packet size, and the next four
     * indicate the actual extended opcode.
     */
    packet_length = 
        sizeof( GLenum) + /* extended opcode */
        sizeof( target ) +
        sizeof( level ) +
        sizeof( internalformat ) +
        sizeof( width ) + 
        sizeof( border ) +
        sizeof( imagesize ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += imagesize;
    }


    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, CR_COMPRESSEDTEXIMAGE1DARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLenum, target );
    WRITE_DATA( 8, GLint, level );
    WRITE_DATA( 12, GLint, internalformat );
    WRITE_DATA( 16, GLsizei, width );
    WRITE_DATA( 20, GLint, border );
    WRITE_DATA( 24, GLsizei, imagesize );
    WRITE_DATA( 28, int, noimagedata );
    WRITE_DATA( 32, GLint, (GLint)(uintptr_t) data);

    if (!noimagedata) {
        crMemcpy( (void *)(data_ptr + 36), (void *)data, imagesize);
    }

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imagesize, const GLvoid *data )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (data == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    /* All extended opcodes have their first 8 bytes predefined:
     * the first four indicate the packet size, and the next four
     * indicate the actual extended opcode.
     */
    packet_length = 
        sizeof( GLenum) + /* extended opcode */
        sizeof( target ) +
        sizeof( level ) +
        sizeof( internalformat ) +
        sizeof( width ) + 
        sizeof( height ) + 
        sizeof( border ) +
        sizeof( imagesize ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += imagesize;
    }

    /*crDebug( "Compressing that shit: %d", level );*/

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, CR_COMPRESSEDTEXIMAGE2DARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLenum, target );
    WRITE_DATA( 8, GLint, level );
    WRITE_DATA( 12, GLint, internalformat );
    WRITE_DATA( 16, GLsizei, width );
    WRITE_DATA( 20, GLsizei, height );
    WRITE_DATA( 24, GLint, border );
    WRITE_DATA( 28, GLsizei, imagesize );
    WRITE_DATA( 32, int, noimagedata );
    WRITE_DATA( 36, GLint, (GLint)(uintptr_t) data);

    if (!noimagedata) {
        crMemcpy( (void *)(data_ptr + 40), (void *)data, imagesize);
    }

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imagesize, const GLvoid *data )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (data == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    /* All extended opcodes have their first 8 bytes predefined:
     * the first four indicate the packet size, and the next four
     * indicate the actual extended opcode.
     */
    packet_length = 
        sizeof( GLenum) + /* extended opcode */
        sizeof( target ) +
        sizeof( level ) +
        sizeof( internalformat ) +
        sizeof( width ) + 
        sizeof( height ) + 
        sizeof( depth ) + 
        sizeof( border ) +
        sizeof( imagesize ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += imagesize;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, CR_COMPRESSEDTEXIMAGE3DARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLenum, target );
    WRITE_DATA( 8, GLint, level );
    WRITE_DATA( 12, GLint, internalformat );
    WRITE_DATA( 16, GLsizei, width );
    WRITE_DATA( 20, GLsizei, height );
    WRITE_DATA( 24, GLsizei, depth );
    WRITE_DATA( 28, GLint, border );
    WRITE_DATA( 32, GLsizei, imagesize );
    WRITE_DATA( 36, int, noimagedata );
    WRITE_DATA( 40, GLint, (GLint)(uintptr_t) data);

    if (!noimagedata) {
        crMemcpy( (void *)(data_ptr + 44), (void *)data, imagesize);
    }

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackCompressedTexSubImage1DARB( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imagesize, const GLvoid *data )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (data == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    /* All extended opcodes have their first 8 bytes predefined:
     * the first four indicate the packet size, and the next four
     * indicate the actual extended opcode.
     */
    packet_length = 
        sizeof( GLenum) + /* extended opcode */
        sizeof( target ) +
        sizeof( level ) +
        sizeof( xoffset ) +
        sizeof( width ) + 
        sizeof( format ) +
        sizeof( imagesize ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += imagesize;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, CR_COMPRESSEDTEXSUBIMAGE1DARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLenum, target );
    WRITE_DATA( 8, GLint, level );
    WRITE_DATA( 12, GLint, xoffset );
    WRITE_DATA( 16, GLsizei, width );
    WRITE_DATA( 20, GLenum, format );
    WRITE_DATA( 24, GLsizei, imagesize );
    WRITE_DATA( 28, int, noimagedata );
    WRITE_DATA( 32, GLint, (GLint)(uintptr_t) data);

    if (!noimagedata) {
        crMemcpy( (void *)(data_ptr + 36), (void *)data, imagesize);
    }

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imagesize, const GLvoid *data )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (data == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    /* All extended opcodes have their first 8 bytes predefined:
     * the first four indicate the packet size, and the next four
     * indicate the actual extended opcode.
     */
    packet_length = 
        sizeof( GLenum) + /* extended opcode */
        sizeof( target ) +
        sizeof( level ) +
        sizeof( xoffset ) +
        sizeof( yoffset ) +
        sizeof( width ) + 
        sizeof( height ) + 
        sizeof( format ) +
        sizeof( imagesize ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += imagesize;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, CR_COMPRESSEDTEXSUBIMAGE2DARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLenum, target );
    WRITE_DATA( 8, GLint, level );
    WRITE_DATA( 12, GLint, xoffset );
    WRITE_DATA( 16, GLint, yoffset );
    WRITE_DATA( 20, GLsizei, width );
    WRITE_DATA( 24, GLsizei, height );
    WRITE_DATA( 28, GLenum, format );
    WRITE_DATA( 32, GLsizei, imagesize );
    WRITE_DATA( 36, int, noimagedata );
    WRITE_DATA( 40, GLint, (GLint)(uintptr_t) data);

    if (!noimagedata) {
        crMemcpy( (void *)(data_ptr + 44), (void *)data, imagesize);
    }

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imagesize, const GLvoid *data )
{
    unsigned char *data_ptr;
    int packet_length;
    int noimagedata = (data == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    /* All extended opcodes have their first 8 bytes predefined:
     * the first four indicate the packet size, and the next four
     * indicate the actual extended opcode.
     */
    packet_length = 
        sizeof( GLenum) + /* extended opcode */
        sizeof( target ) +
        sizeof( level ) +
        sizeof( xoffset ) +
        sizeof( yoffset ) +
        sizeof( zoffset ) +
        sizeof( width ) + 
        sizeof( height ) + 
        sizeof( depth ) + 
        sizeof( format ) +
        sizeof( imagesize ) +
        sizeof( int ) + sizeof(GLint);

    if (!noimagedata)
    {
        packet_length += imagesize;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLenum, CR_COMPRESSEDTEXSUBIMAGE3DARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLenum, target );
    WRITE_DATA( 8, GLint, level );
    WRITE_DATA( 12, GLint, xoffset );
    WRITE_DATA( 16, GLint, yoffset );
    WRITE_DATA( 20, GLint, zoffset );
    WRITE_DATA( 24, GLsizei, width );
    WRITE_DATA( 28, GLsizei, height );
    WRITE_DATA( 32, GLsizei, depth );
    WRITE_DATA( 36, GLenum, format );
    WRITE_DATA( 40, GLsizei, imagesize );
    WRITE_DATA( 44, int, noimagedata );
    WRITE_DATA( 48, GLint, (GLint)(uintptr_t) data);

    if (!noimagedata) {
        crMemcpy( (void *)(data_ptr + 52), (void *)data, imagesize);
    }

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackGetCompressedTexImageARB( GLenum target, GLint level, GLvoid *img, int *writeback )
{
    CR_GET_PACKER_CONTEXT(pc);
    int packet_length = sizeof(int)+sizeof(GLenum)+sizeof(target)+sizeof(level)+2*8;
    unsigned char *data_ptr;
    CR_GET_BUFFERED_POINTER( pc, packet_length );

    WRITE_DATA_AI(int, packet_length);
    WRITE_DATA_AI(GLenum, CR_GETCOMPRESSEDTEXIMAGEARB_EXTEND_OPCODE);
    WRITE_DATA_AI(GLenum, target);
    WRITE_DATA_AI(GLint, level);
    WRITE_NETWORK_POINTER(0, (void *) img );
    WRITE_NETWORK_POINTER(8, (void *) writeback );
    WRITE_OPCODE(pc, CR_EXTEND_OPCODE);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
