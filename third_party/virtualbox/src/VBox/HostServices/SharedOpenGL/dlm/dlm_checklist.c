/* $Id: dlm_checklist.c $ */
#include "cr_dlm.h"
#include "cr_mem.h"
#include "cr_pixeldata.h"
#include "cr_string.h"
#include "dlm.h"

/*****************************************************************************
 * These helper functions are used for GL functions that are listed in
 * the APIspec.txt file as "checklist", meaning that sometimes they
 * represent functions that can be stored in a display list, and sometimes
 * they represent control functions that must be executed immediately.
 *
 * The calling SPU must use these check functions (or their equivalents)
 * before asking the DLM to compile any elements of these types.
 * They return nonzero (TRUE) if the element goes into a display list.
 */

int DLM_APIENTRY crDLMCheckListTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    return (target != GL_PROXY_TEXTURE_1D);
}

int DLM_APIENTRY crDLMCheckListCompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imagesize, const GLvoid *data)
{
    return (target != GL_PROXY_TEXTURE_1D);
}
int DLM_APIENTRY crDLMCheckListTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    return (target != GL_PROXY_TEXTURE_2D);
}

int DLM_APIENTRY crDLMCheckListCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imagesize, const GLvoid *data)
{
    return (target != GL_PROXY_TEXTURE_2D);
}

int DLM_APIENTRY crDLMCheckListTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    return (target != GL_PROXY_TEXTURE_3D);
}

int DLM_APIENTRY crDLMCheckListTexImage3DEXT(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    return (target != GL_PROXY_TEXTURE_3D);
}

int DLM_APIENTRY crDLMCheckListCompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imagesize, const GLvoid *data)
{
    return (target != GL_PROXY_TEXTURE_3D);
}
