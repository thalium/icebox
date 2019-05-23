/* $Id: dlm_arrays.c $ */
/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include <stdarg.h>
#include "chromium.h"
#include "cr_dlm.h"
#include "dlm.h"

/*
 * XXX this code is awfully similar to the code in arrayspu.c
 * We should try to write something reusable.
 */

void DLM_APIENTRY crDLMCompileArrayElement (GLint index, CRClientState *c)
{
  unsigned char *p;
  int unit;

  if (c->array.e.enabled)
  {
    crDLMCompileEdgeFlagv(c->array.e.p + index*c->array.e.stride);
  }
  for (unit = 0; unit < CR_MAX_TEXTURE_UNITS; unit++)
  {
    if (c->array.t[unit].enabled)
    {
      p = c->array.t[unit].p + index*c->array.t[unit].stride;
      switch (c->array.t[unit].type)
      {
        case GL_SHORT:
          switch (c->array.t[c->curClientTextureUnit].size)
          {
            case 1: crDLMCompileMultiTexCoord1svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
            case 2: crDLMCompileMultiTexCoord2svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
            case 3: crDLMCompileMultiTexCoord3svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
            case 4: crDLMCompileMultiTexCoord4svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
          }
          break;
        case GL_INT:
          switch (c->array.t[c->curClientTextureUnit].size)
          {
            case 1: crDLMCompileMultiTexCoord1ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
            case 2: crDLMCompileMultiTexCoord2ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
            case 3: crDLMCompileMultiTexCoord3ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
            case 4: crDLMCompileMultiTexCoord4ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
          }
          break;
        case GL_FLOAT:
          switch (c->array.t[c->curClientTextureUnit].size)
          {
            case 1: crDLMCompileMultiTexCoord1fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
            case 2: crDLMCompileMultiTexCoord2fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
            case 3: crDLMCompileMultiTexCoord3fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
            case 4: crDLMCompileMultiTexCoord4fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
          }
          break;
        case GL_DOUBLE:
          switch (c->array.t[c->curClientTextureUnit].size)
          {
            case 1: crDLMCompileMultiTexCoord1dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
            case 2: crDLMCompileMultiTexCoord2dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
            case 3: crDLMCompileMultiTexCoord3dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
            case 4: crDLMCompileMultiTexCoord4dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
          }
          break;
      }
    }
  } /* loop over texture units */

  if (c->array.i.enabled)
  {
    p = c->array.i.p + index*c->array.i.stride;
    switch (c->array.i.type)
    {
      case GL_SHORT: crDLMCompileIndexsv((GLshort *)p); break;
      case GL_INT: crDLMCompileIndexiv((GLint *)p); break;
      case GL_FLOAT: crDLMCompileIndexfv((GLfloat *)p); break;
      case GL_DOUBLE: crDLMCompileIndexdv((GLdouble *)p); break;
    }
  }
  if (c->array.c.enabled)
  {
    p = c->array.c.p + index*c->array.c.stride;
    switch (c->array.c.type)
    {
      case GL_BYTE:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3bv((GLbyte *)p); break;
          case 4: crDLMCompileColor4bv((GLbyte *)p); break;
        }
        break;
      case GL_UNSIGNED_BYTE:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3ubv((GLubyte *)p); break;
          case 4: crDLMCompileColor4ubv((GLubyte *)p); break;
        }
        break;
      case GL_SHORT:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3sv((GLshort *)p); break;
          case 4: crDLMCompileColor4sv((GLshort *)p); break;
        }
        break;
      case GL_UNSIGNED_SHORT:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3usv((GLushort *)p); break;
          case 4: crDLMCompileColor4usv((GLushort *)p); break;
        }
        break;
      case GL_INT:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3iv((GLint *)p); break;
          case 4: crDLMCompileColor4iv((GLint *)p); break;
        }
        break;
      case GL_UNSIGNED_INT:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3uiv((GLuint *)p); break;
          case 4: crDLMCompileColor4uiv((GLuint *)p); break;
        }
        break;
      case GL_FLOAT:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3fv((GLfloat *)p); break;
          case 4: crDLMCompileColor4fv((GLfloat *)p); break;
        }
        break;
      case GL_DOUBLE:
        switch (c->array.c.size)
        {
          case 3: crDLMCompileColor3dv((GLdouble *)p); break;
          case 4: crDLMCompileColor4dv((GLdouble *)p); break;
        }
        break;
    }
  }
  if (c->array.n.enabled)
  {
    p = c->array.n.p + index*c->array.n.stride;
    switch (c->array.n.type)
    {
      case GL_BYTE: crDLMCompileNormal3bv((GLbyte *)p); break;
      case GL_SHORT: crDLMCompileNormal3sv((GLshort *)p); break;
      case GL_INT: crDLMCompileNormal3iv((GLint *)p); break;
      case GL_FLOAT: crDLMCompileNormal3fv((GLfloat *)p); break;
      case GL_DOUBLE: crDLMCompileNormal3dv((GLdouble *)p); break;
    }
  }
#ifdef CR_EXT_secondary_color
  if (c->array.s.enabled)
  {
    p = c->array.s.p + index*c->array.s.stride;
    switch (c->array.s.type)
    {
      case GL_BYTE:
        crDLMCompileSecondaryColor3bvEXT((GLbyte *)p); break;
      case GL_UNSIGNED_BYTE:
        crDLMCompileSecondaryColor3ubvEXT((GLubyte *)p); break;
      case GL_SHORT:
        crDLMCompileSecondaryColor3svEXT((GLshort *)p); break;
      case GL_UNSIGNED_SHORT:
        crDLMCompileSecondaryColor3usvEXT((GLushort *)p); break;
      case GL_INT:
        crDLMCompileSecondaryColor3ivEXT((GLint *)p); break;
      case GL_UNSIGNED_INT:
        crDLMCompileSecondaryColor3uivEXT((GLuint *)p); break;
      case GL_FLOAT:
        crDLMCompileSecondaryColor3fvEXT((GLfloat *)p); break;
      case GL_DOUBLE:
        crDLMCompileSecondaryColor3dvEXT((GLdouble *)p); break;
    }
  }
#endif
  if (c->array.v.enabled)
  {
    p = c->array.v.p + (index*c->array.v.stride);

    switch (c->array.v.type)
    {
      case GL_SHORT:
        switch (c->array.v.size)
        {
          case 2: crDLMCompileVertex2sv((GLshort *)p); break;
          case 3: crDLMCompileVertex3sv((GLshort *)p); break;
          case 4: crDLMCompileVertex4sv((GLshort *)p); break;
        }
        break;
      case GL_INT:
        switch (c->array.v.size)
        {
          case 2: crDLMCompileVertex2iv((GLint *)p); break;
          case 3: crDLMCompileVertex3iv((GLint *)p); break;
          case 4: crDLMCompileVertex4iv((GLint *)p); break;
        }
        break;
      case GL_FLOAT:
        switch (c->array.v.size)
        {
          case 2: crDLMCompileVertex2fv((GLfloat *)p); break;
          case 3: crDLMCompileVertex3fv((GLfloat *)p); break;
          case 4: crDLMCompileVertex4fv((GLfloat *)p); break;
        }
        break;
      case GL_DOUBLE:
        switch (c->array.v.size)
        {
          case 2: crDLMCompileVertex2dv((GLdouble *)p); break;
          case 3: crDLMCompileVertex3dv((GLdouble *)p); break;
          case 4: crDLMCompileVertex4dv((GLdouble *)p); break;
        }
        break;
    }
  }
}

void DLM_APIENTRY crDLMCompileDrawArrays(GLenum mode, GLint first, GLsizei count, CRClientState *c)
{
  int i;

  if (count < 0)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_VALUE, "DLM DrawArrays(negative count)");
    return;
  }

  if (mode > GL_POLYGON)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_ENUM, "DLM DrawArrays(bad mode)");
    return;
  }

  crDLMCompileBegin(mode);
  for (i=0; i<count; i++)
  {
    crDLMCompileArrayElement(first + i, c);
  }
  crDLMCompileEnd();
}

void DLM_APIENTRY crDLMCompileDrawElements(GLenum mode, GLsizei count,
                                      GLenum type, const GLvoid *indices, CRClientState *c)
{
  int i;
  GLubyte *p = (GLubyte *)indices;

  if (count < 0)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_VALUE, "DLM DrawElements(negative count)");
    return;
  }

  if (mode > GL_POLYGON)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_ENUM, "DLM DrawElements(bad mode)");
    return;
  }

  if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_ENUM, "DLM DrawElements(bad type)");
    return;
  }

  crDLMCompileBegin(mode);
  switch (type)
  {
  case GL_UNSIGNED_BYTE:
    for (i=0; i<count; i++)
    {
      crDLMCompileArrayElement((GLint) *p++, c);
    }
    break;
  case GL_UNSIGNED_SHORT:
    for (i=0; i<count; i++)
    {
      crDLMCompileArrayElement((GLint) * (GLushort *) p, c);
      p+=sizeof (GLushort);
    }
    break;
  case GL_UNSIGNED_INT:
    for (i=0; i<count; i++)
    {
      crDLMCompileArrayElement((GLint) * (GLuint *) p, c);
      p+=sizeof (GLuint);
    }
    break;
  default:
    crError( "this can't happen: DLM DrawElements" );
    break;
  }
  crDLMCompileEnd();
}

void DLM_APIENTRY crDLMCompileDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, 
                                      GLenum type, const GLvoid *indices, CRClientState *c)
{
  int i;
  GLubyte *p = (GLubyte *)indices;

  (void) end;

  if (count < 0)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_VALUE, "DLM DrawRangeElements(negative count)");
    return;
  }

  if (mode > GL_POLYGON)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_ENUM, "DLM DrawRangeElements(bad mode)");
    return;
  }

  if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT)
  {
    crdlmWarning(__LINE__, __FILE__, GL_INVALID_ENUM, "DLM DrawRangeElements(bad type)");
    return;
  }

  crDLMCompileBegin(mode);
  switch (type)
  {
  case GL_UNSIGNED_BYTE:
    for (i=start; i<count; i++)
    {
      crDLMCompileArrayElement((GLint) *p++, c);
    }
    break;
  case GL_UNSIGNED_SHORT:
    for (i=start; i<count; i++)
    {
      crDLMCompileArrayElement((GLint) * (GLushort *) p, c);
      p+=sizeof (GLushort);
    }
    break;
  case GL_UNSIGNED_INT:
    for (i=start; i<count; i++)
    {
      crDLMCompileArrayElement((GLint) * (GLuint *) p, c);
      p+=sizeof (GLuint);
    }
    break;
  default:
    crError( "this can't happen: DLM DrawRangeElements" );
    break;
  }
  crDLMCompileEnd();
}

#ifdef CR_EXT_multi_draw_arrays
void DLM_APIENTRY crDLMCompileMultiDrawArraysEXT( GLenum mode, GLint *first,
                          GLsizei *count, GLsizei primcount, CRClientState *c)
{
   GLint i;

   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         crDLMCompileDrawArrays(mode, first[i], count[i], c);
      }
   }
}


void DLM_APIENTRY crDLMCompileMultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type,
                            const GLvoid **indices, GLsizei primcount, CRClientState *c)
{
   GLint i;

   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         crDLMCompileDrawElements(mode, count[i], type, indices[i], c);
      }
   }
}
#endif /* CR_EXT_multi_draw_arrays */
