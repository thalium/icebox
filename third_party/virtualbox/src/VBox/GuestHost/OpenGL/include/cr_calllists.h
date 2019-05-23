/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_CALLLISTS_H
#define CR_CALLLISTS_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern DECLEXPORT(int)
crExpandCallLists(GLsizei n, GLenum type, const GLvoid *lists, 
    GLuint base, void (*callListFunc)(GLuint list));

extern DECLEXPORT(int)
crExpandCallListsWithData(GLsizei n, GLenum type, const GLvoid *lists, 
    GLuint base, void (*callListFunc)(GLuint list, GLuint index, GLvoid *data), GLvoid *data);

#ifdef __cplusplus
}
#endif

#endif /* CR_CALLLISTS_H */
