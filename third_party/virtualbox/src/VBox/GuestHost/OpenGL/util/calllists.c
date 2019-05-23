/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_calllists.h"

#define EXPAND(typeEnum, typeCast, increment, value)     \
    case typeEnum:                                       \
      {                                                  \
        const typeCast *array = (const typeCast *) lists; \
        for (i = 0; i < (GLuint)n; i++, array increment) {        \
          (*callListFunc)(base + (GLuint) value); 	 \
        }                                                \
      }                                                  \
      break

#define EXPAND_WITH_DATA(typeEnum, typeCast, increment, value)     \
    case typeEnum:                                       	\
      {                                                  \
        const typeCast *array = (const typeCast *) lists; \
        for (i = 0; i < (GLuint)n; i++, array increment) {\
          (*callListFunc)(base + (GLuint) value, i, data); \
        }                                                 \
      }                                                    \
      break


int
crExpandCallLists(GLsizei n, GLenum type, const GLvoid *lists, 
    GLuint base, void (*callListFunc)(GLuint list))
{
	GLuint i;

	switch (type) {
		EXPAND(GL_BYTE, GLbyte,++,*array);
		EXPAND(GL_UNSIGNED_BYTE, GLubyte,++,*array);
		EXPAND(GL_SHORT, GLshort,++,*array);
		EXPAND(GL_UNSIGNED_SHORT, GLushort,++,*array);
		EXPAND(GL_INT, GLint,++,*array);
		EXPAND(GL_UNSIGNED_INT, GLuint,++,*array);
		EXPAND(GL_FLOAT, GLfloat,++,*array);

		EXPAND(GL_2_BYTES, GLubyte, +=2, 256*array[0] + array[1]);
		EXPAND(GL_3_BYTES, GLubyte, +=3, 256 * (256 * array[0] + array[1]) + array[2]);
		EXPAND(GL_4_BYTES, GLubyte, +=4, 256 * (256 * (256 * array[0] + array[1]) + array[2]) + array[3]);

		default:
			return GL_INVALID_ENUM;
	}

	return GL_NO_ERROR;
}

int
crExpandCallListsWithData(GLsizei n, GLenum type, const GLvoid *lists, 
    GLuint base, void (*callListFunc)(GLuint list, GLuint index, GLvoid *data), GLvoid *data)
{
	GLuint i;

	switch (type) {
		EXPAND_WITH_DATA(GL_BYTE, GLbyte,++,*array);
		EXPAND_WITH_DATA(GL_UNSIGNED_BYTE, GLubyte,++,*array);
		EXPAND_WITH_DATA(GL_SHORT, GLshort,++,*array);
		EXPAND_WITH_DATA(GL_UNSIGNED_SHORT, GLushort,++,*array);
		EXPAND_WITH_DATA(GL_INT, GLint,++,*array);
		EXPAND_WITH_DATA(GL_UNSIGNED_INT, GLuint,++,*array);
		EXPAND_WITH_DATA(GL_FLOAT, GLfloat,++,*array);

		EXPAND_WITH_DATA(GL_2_BYTES, GLubyte, +=2, 256*array[0] + array[1]);
		EXPAND_WITH_DATA(GL_3_BYTES, GLubyte, +=3, 256 * (256 * array[0] + array[1]) + array[2]);
		EXPAND_WITH_DATA(GL_4_BYTES, GLubyte, +=4, 256 * (256 * (256 * array[0] + array[1]) + array[2]) + array[3]);

		default:
			return GL_INVALID_ENUM;
	}

	return GL_NO_ERROR;
}
