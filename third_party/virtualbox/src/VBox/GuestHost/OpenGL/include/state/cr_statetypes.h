/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_TYPES_H
#define CR_STATE_TYPES_H

#include "chromium.h"
#include "cr_bits.h"
#include "cr_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef GLfloat GLdefault;
#define CR_DEFAULTTYPE_FLOAT

#define CR_MAXBYTE		((GLbyte) 0x7F)
#define CR_MAXUBYTE		((GLubyte) 0xFF)
#define CR_MAXSHORT		((GLshort) 0x7FFF)
#define CR_MAXUSHORT	((GLushort) 0xFFFF)
#define CR_MAXINT			((GLint) 0x7FFFFFFF)
#define CR_MAXUINT		((GLuint) 0xFFFFFFFF)
#define CR_MAXFLOAT		1.0f
#define CR_MAXDOUBLE	1.0

#define CRBITS_LENGTH 32
#define CRBITS_ONES 0xFFFFFFFF
typedef unsigned int CRbitvalue;

typedef struct {
	GLfloat x1, x2, y1, y2;
} CRrectf;

typedef struct {
	GLint x1, y1, x2, y2;
} CRrecti;

#define VECTOR(type, name) typedef struct { type x,y,z,w; } name
#define COLOR(type, name) typedef struct { type r,g,b,a; } name
#define TEXCOORD(type, name) typedef struct { type s,t,r,q; } name

VECTOR(GLdefault,GLvector);
VECTOR(GLenum,GLvectore);
VECTOR(GLubyte,GLvectorub);
VECTOR(GLbyte,GLvectorb);
VECTOR(GLushort,GLvectorus);
VECTOR(GLshort,GLvectors);
VECTOR(GLint,GLvectori);
VECTOR(GLuint,GLvectorui);
/* These two are defined in cr_matrix.h */
/* VECTOR(GLfloat,GLvectorf); */
/* VECTOR(GLdouble,GLvectord); */
COLOR(GLdefault,GLcolor);
COLOR(GLenum,GLcolore);
COLOR(GLubyte,GLcolorub);
COLOR(GLbyte,GLcolorb);
COLOR(GLushort,GLcolorus);
COLOR(GLshort,GLcolors);
COLOR(GLint,GLcolori);
COLOR(GLuint,GLcolorui);
COLOR(GLfloat,GLcolorf);
COLOR(GLdouble,GLcolord);
TEXCOORD(GLdefault,GLtexcoord);
TEXCOORD(GLenum,GLtexcoorde);
TEXCOORD(GLubyte,GLtexcoordub);
TEXCOORD(GLbyte,GLtexcoordb);
TEXCOORD(GLushort,GLtexcoordus);
TEXCOORD(GLshort,GLtexcoords);
TEXCOORD(GLint,GLtexcoordi);
TEXCOORD(GLuint,GLtexcoordui);
TEXCOORD(GLfloat,GLtexcoordf);
TEXCOORD(GLdouble,GLtexcoordd);

#undef VECTOR
#undef COLOR
#undef TEXCOORD

#define COMPARE_VECTOR(a,b)		((a)[0] != (b)[0] || (a)[1] != (b)[1] || (a)[2] != (b)[2] || (a)[3] != (b)[3])
#define COMPARE_TEXCOORD(a,b)	((a)[0] != (b)[0] || (a)[1] != (b)[1] || (a)[2] != (b)[2] || (a)[3] != (b)[3])
#define COMPARE_COLOR(x,y)		((x)[0] != (y)[0] || (x)[1] != (y)[1] || (x)[2] != (y)[2] || (x)[3] != (y)[3])

/* Assign scalers to short vectors: */
#define ASSIGN_2V( V, V0, V1 )	\
do { 				\
    (V)[0] = V0; 			\
    (V)[1] = V1; 			\
} while(0)

#define ASSIGN_3V( V, V0, V1, V2 )	\
do { 				 	\
    (V)[0] = V0; 				\
    (V)[1] = V1; 				\
    (V)[2] = V2; 				\
} while(0)

#define ASSIGN_4V( V, V0, V1, V2, V3 ) 		\
do { 						\
    (V)[0] = V0;					\
    (V)[1] = V1;					\
    (V)[2] = V2;					\
    (V)[3] = V3; 					\
} while(0)


/* Copy short vectors: */
#define COPY_2V( DST, SRC )			\
do {						\
   (DST)[0] = (SRC)[0];				\
   (DST)[1] = (SRC)[1];				\
} while (0)

#define COPY_3V( DST, SRC )			\
do {						\
   (DST)[0] = (SRC)[0];				\
   (DST)[1] = (SRC)[1];				\
   (DST)[2] = (SRC)[2];				\
} while (0)

#define COPY_4V( DST, SRC )			\
do {						\
   (DST)[0] = (SRC)[0];				\
   (DST)[1] = (SRC)[1];				\
   (DST)[2] = (SRC)[2];				\
   (DST)[3] = (SRC)[3];				\
} while (0)

#define COPY_2V_CAST( DST, SRC, CAST )		\
do {						\
   (DST)[0] = (CAST)(SRC)[0];			\
   (DST)[1] = (CAST)(SRC)[1];			\
} while (0)

#define COPY_3V_CAST( DST, SRC, CAST )		\
do {						\
   (DST)[0] = (CAST)(SRC)[0];			\
   (DST)[1] = (CAST)(SRC)[1];			\
   (DST)[2] = (CAST)(SRC)[2];			\
} while (0)

#define COPY_4V_CAST( DST, SRC, CAST )		\
do {						\
   (DST)[0] = (CAST)(SRC)[0];			\
   (DST)[1] = (CAST)(SRC)[1];			\
   (DST)[2] = (CAST)(SRC)[2];			\
   (DST)[3] = (CAST)(SRC)[3];			\
} while (0)

#if defined(__i386__)
#define COPY_4UBV(DST, SRC)			\
do {						\
   *((GLuint*)(DST)) = *((GLuint*)(SRC));	\
} while (0)
#else
/* The GLuint cast might fail if DST or SRC are not dword-aligned (RISC) */
#define COPY_4UBV(DST, SRC)			\
do {						\
   (DST)[0] = (SRC)[0];				\
   (DST)[1] = (SRC)[1];				\
   (DST)[2] = (SRC)[2];				\
   (DST)[3] = (SRC)[3];				\
} while (0)
#endif

#define COPY_2FV( DST, SRC )			\
do {						\
   const GLfloat *_tmp = (SRC);			\
   (DST)[0] = _tmp[0];				\
   (DST)[1] = _tmp[1];				\
} while (0)

#define COPY_3FV( DST, SRC )			\
do {						\
   const GLfloat *_tmp = (SRC);			\
   (DST)[0] = _tmp[0];				\
   (DST)[1] = _tmp[1];				\
   (DST)[2] = _tmp[2];				\
} while (0)

#define COPY_4FV( DST, SRC )			\
do {						\
   const GLfloat *_tmp = (SRC);			\
   (DST)[0] = _tmp[0];				\
   (DST)[1] = _tmp[1];				\
   (DST)[2] = _tmp[2];				\
   (DST)[3] = _tmp[3];				\
} while (0)



#define COPY_SZ_4V(DST, SZ, SRC) 		\
do {						\
   switch (SZ) {				\
   case 4: (DST)[3] = (SRC)[3];			\
   case 3: (DST)[2] = (SRC)[2];			\
   case 2: (DST)[1] = (SRC)[1];			\
   case 1: (DST)[0] = (SRC)[0];			\
   }  						\
} while(0)


#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_TYPES_H */
