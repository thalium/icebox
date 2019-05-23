/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "cr_spu.h"
#include "cr_error.h"
#include "state/cr_limits.h"
#include "arrayspu.h"

ArraySPU array_spu;

#ifdef CHROMIUM_THREADSAFE
CRmutex _ArrayMutex;
#endif

static void ARRAYSPU_APIENTRY arrayspu_ArrayElement( GLint index )
{
    const CRClientState *c = &(crStateGetCurrent()->client);
    const CRVertexArrays *array = &(c->array);
    const GLboolean vpEnabled = crStateGetCurrent()->program.vpEnabled;
    unsigned char *p;
    unsigned int unit, attr;

    if (array->e.enabled)
    {
        p = array->e.p + index * array->e.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->e.buffer && array->e.buffer->data)
        {
            p = (unsigned char *)(array->e.buffer->data) + (unsigned long)p;
        }
#endif

        array_spu.self.EdgeFlagv(p);
    }

    /*
     * Vertex attribute arrays (GL_NV_vertex_program) have priority over
     * the conventional vertex arrays.
     */
    if (vpEnabled)
    {
        for (attr = 1; attr < VERT_ATTRIB_MAX; attr++)
        {
            if (array->a[attr].enabled)
            {
                GLint *iPtr;
                p = array->a[attr].p + index * array->a[attr].stride;

#ifdef CR_ARB_vertex_buffer_object
                if (array->a[attr].buffer && array->a[attr].buffer->data)
                {
                    p = (unsigned char *)(array->a[attr].buffer->data) + (unsigned long)p;
                }
#endif

                switch (array->a[attr].type)
                {
                    case GL_SHORT:
                        switch (array->a[attr].size)
                        {
                            case 1: array_spu.self.VertexAttrib1svARB(attr, (GLshort *)p); break;
                            case 2: array_spu.self.VertexAttrib2svARB(attr, (GLshort *)p); break;
                            case 3: array_spu.self.VertexAttrib3svARB(attr, (GLshort *)p); break;
                            case 4: array_spu.self.VertexAttrib4svARB(attr, (GLshort *)p); break;
                        }
                        break;
                    case GL_INT:
                        iPtr = (GLint *) p;
                        switch (array->a[attr].size)
                        {
                            case 1: array_spu.self.VertexAttrib1fARB(attr, p[0]); break;
                            case 2: array_spu.self.VertexAttrib2fARB(attr, p[0], p[1]); break;
                            case 3: array_spu.self.VertexAttrib3fARB(attr, p[0], p[1], p[2]); break;
                            case 4: array_spu.self.VertexAttrib4fARB(attr, p[0], p[1], p[2], p[3]); break;
                        }
                        break;
                    case GL_FLOAT:
                        switch (array->a[attr].size)
                        {
                            case 1: array_spu.self.VertexAttrib1fvARB(attr, (GLfloat *)p); break;
                            case 2: array_spu.self.VertexAttrib2fvARB(attr, (GLfloat *)p); break;
                            case 3: array_spu.self.VertexAttrib3fvARB(attr, (GLfloat *)p); break;
                            case 4: array_spu.self.VertexAttrib4fvARB(attr, (GLfloat *)p); break;
                        }
                        break;
                    case GL_DOUBLE:
                        switch (array->a[attr].size)
                        {
                            case 1: array_spu.self.VertexAttrib1dvARB(attr, (GLdouble *)p); break;
                            case 2: array_spu.self.VertexAttrib2dvARB(attr, (GLdouble *)p); break;
                            case 3: array_spu.self.VertexAttrib3dvARB(attr, (GLdouble *)p); break;
                            case 4: array_spu.self.VertexAttrib4dvARB(attr, (GLdouble *)p); break;
                        }
                        break;
                    default:
                        crWarning("Bad datatype for vertex attribute [%d] array: 0x%x\n",
                                            attr, array->a[attr].type);
                }
            }
        }
    }

    /* Now do conventional arrays, unless overridden by generic arrays above */
    for (unit = 0 ; unit < crStateGetCurrent()->limits.maxTextureUnits ; unit++)
    {
        if (array->t[unit].enabled && !(vpEnabled && array->a[VERT_ATTRIB_TEX0+unit].enabled))
        {
            p = array->t[unit].p + index * array->t[unit].stride;

#ifdef CR_ARB_vertex_buffer_object
            if (array->t[unit].buffer && array->t[unit].buffer->data)
            {
                p = (unsigned char *)(array->t[unit].buffer->data) + (unsigned long)p;
            }
#endif

            switch (array->t[unit].type)
            {
                case GL_SHORT:
                    switch (array->t[unit].size)
                    {
                        case 1: array_spu.self.MultiTexCoord1svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                        case 2: array_spu.self.MultiTexCoord2svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                        case 3: array_spu.self.MultiTexCoord3svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                        case 4: array_spu.self.MultiTexCoord4svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                    }
                    break;
                case GL_INT:
                    switch (array->t[unit].size)
                    {
                        case 1: array_spu.self.MultiTexCoord1ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                        case 2: array_spu.self.MultiTexCoord2ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                        case 3: array_spu.self.MultiTexCoord3ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                        case 4: array_spu.self.MultiTexCoord4ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                    }
                    break;
                case GL_FLOAT:
                    switch (array->t[unit].size)
                    {
                        case 1: array_spu.self.MultiTexCoord1fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                        case 2: array_spu.self.MultiTexCoord2fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                        case 3: array_spu.self.MultiTexCoord3fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                        case 4: array_spu.self.MultiTexCoord4fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                    }
                    break;
                case GL_DOUBLE:
                    switch (array->t[unit].size)
                    {
                        case 1: array_spu.self.MultiTexCoord1dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                        case 2: array_spu.self.MultiTexCoord2dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                        case 3: array_spu.self.MultiTexCoord3dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                        case 4: array_spu.self.MultiTexCoord4dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                    }
                    break;
            }
        }
    }
    if (array->i.enabled)
    {
        p = array->i.p + index * array->i.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->i.buffer && array->i.buffer->data)
        {
            p = (unsigned char *)(array->i.buffer->data) + (unsigned long)p;
        }
#endif

        switch (array->i.type)
        {
            case GL_SHORT: array_spu.self.Indexsv((GLshort *)p); break;
            case GL_INT: array_spu.self.Indexiv((GLint *)p); break;
            case GL_FLOAT: array_spu.self.Indexfv((GLfloat *)p); break;
            case GL_DOUBLE: array_spu.self.Indexdv((GLdouble *)p); break;
        }
    }
    if (array->c.enabled && !(vpEnabled && array->a[VERT_ATTRIB_COLOR0].enabled))
    {
        p = array->c.p + index * array->c.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->c.buffer && array->c.buffer->data)
        {
            p = (unsigned char *)(array->c.buffer->data) + (unsigned long)p;
        }
#endif

        switch (array->c.type)
        {
            case GL_BYTE:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3bv((GLbyte *)p); break;
                    case 4: array_spu.self.Color4bv((GLbyte *)p); break;
                }
                break;
            case GL_UNSIGNED_BYTE:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3ubv((GLubyte *)p); break;
                    case 4: array_spu.self.Color4ubv((GLubyte *)p); break;
                }
                break;
            case GL_SHORT:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3sv((GLshort *)p); break;
                    case 4: array_spu.self.Color4sv((GLshort *)p); break;
                }
                break;
            case GL_UNSIGNED_SHORT:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3usv((GLushort *)p); break;
                    case 4: array_spu.self.Color4usv((GLushort *)p); break;
                }
                break;
            case GL_INT:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3iv((GLint *)p); break;
                    case 4: array_spu.self.Color4iv((GLint *)p); break;
                }
                break;
            case GL_UNSIGNED_INT:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3uiv((GLuint *)p); break;
                    case 4: array_spu.self.Color4uiv((GLuint *)p); break;
                }
                break;
            case GL_FLOAT:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3fv((GLfloat *)p); break;
                    case 4: array_spu.self.Color4fv((GLfloat *)p); break;
                }
                break;
            case GL_DOUBLE:
                switch (array->c.size)
                {
                    case 3: array_spu.self.Color3dv((GLdouble *)p); break;
                    case 4: array_spu.self.Color4dv((GLdouble *)p); break;
                }
                break;
        }
    }
    if (array->n.enabled && !(vpEnabled && array->a[VERT_ATTRIB_NORMAL].enabled))
    {
        p = array->n.p + index * array->n.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->n.buffer && array->n.buffer->data)
        {
            p = (unsigned char *)(array->n.buffer->data) + (unsigned long)p;
        }
#endif

        switch (array->n.type)
        {
            case GL_BYTE: array_spu.self.Normal3bv((GLbyte *)p); break;
            case GL_SHORT: array_spu.self.Normal3sv((GLshort *)p); break;
            case GL_INT: array_spu.self.Normal3iv((GLint *)p); break;
            case GL_FLOAT: array_spu.self.Normal3fv((GLfloat *)p); break;
            case GL_DOUBLE: array_spu.self.Normal3dv((GLdouble *)p); break;
        }
    }
#ifdef CR_EXT_secondary_color
    if (array->s.enabled && !(vpEnabled && array->a[VERT_ATTRIB_COLOR1].enabled))
    {
        p = array->s.p + index * array->s.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->s.buffer && array->s.buffer->data)
        {
            p = (unsigned char *)(array->s.buffer->data) + (unsigned long)p;
        }
#endif

        switch (array->s.type)
        {
            case GL_BYTE:
                array_spu.self.SecondaryColor3bvEXT((GLbyte *)p); break;
            case GL_UNSIGNED_BYTE:
                array_spu.self.SecondaryColor3ubvEXT((GLubyte *)p); break;
            case GL_SHORT:
                array_spu.self.SecondaryColor3svEXT((GLshort *)p); break;
            case GL_UNSIGNED_SHORT:
                array_spu.self.SecondaryColor3usvEXT((GLushort *)p); break;
            case GL_INT:
                array_spu.self.SecondaryColor3ivEXT((GLint *)p); break;
            case GL_UNSIGNED_INT:
                array_spu.self.SecondaryColor3uivEXT((GLuint *)p); break;
            case GL_FLOAT:
                array_spu.self.SecondaryColor3fvEXT((GLfloat *)p); break;
            case GL_DOUBLE:
                array_spu.self.SecondaryColor3dvEXT((GLdouble *)p); break;
        }
    }
#endif // CR_EXT_secondary_color
#ifdef CR_EXT_fog_coord
    if (array->f.enabled && !(vpEnabled && array->a[VERT_ATTRIB_FOG].enabled))
    {
        p = array->f.p + index * array->f.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->f.buffer && array->f.buffer->data)
        {
            p = (unsigned char *)(array->f.buffer->data) + (unsigned long)p;
        }
#endif

        array_spu.self.FogCoordfEXT( *((GLfloat *) p) );
    }
#endif // CR_EXT_fog_coord

    /* Need to do attrib[0] / vertex position last */
    if (array->a[VERT_ATTRIB_POS].enabled) {
        GLint *iPtr;
        p = array->a[VERT_ATTRIB_POS].p + index * array->a[VERT_ATTRIB_POS].stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->a[VERT_ATTRIB_POS].buffer && array->a[VERT_ATTRIB_POS].buffer->data)
        {
            p = (unsigned char *)(array->a[VERT_ATTRIB_POS].buffer->data) + (unsigned long)p;
        }
#endif

        switch (array->a[VERT_ATTRIB_POS].type)
        {
            case GL_SHORT:
                switch (array->a[VERT_ATTRIB_POS].size)
                {
                    case 1: array_spu.self.VertexAttrib1svARB(0, (GLshort *)p); break;
                    case 2: array_spu.self.VertexAttrib2svARB(0, (GLshort *)p); break;
                    case 3: array_spu.self.VertexAttrib3svARB(0, (GLshort *)p); break;
                    case 4: array_spu.self.VertexAttrib4svARB(0, (GLshort *)p); break;
                }
                break;
            case GL_INT:
                iPtr = (GLint *) p;
                switch (array->a[VERT_ATTRIB_POS].size)
                {
                    case 1: array_spu.self.VertexAttrib1fARB(0, p[0]); break;
                    case 2: array_spu.self.VertexAttrib2fARB(0, p[0], p[1]); break;
                    case 3: array_spu.self.VertexAttrib3fARB(0, p[0], p[1], p[2]); break;
                    case 4: array_spu.self.VertexAttrib4fARB(0, p[0], p[1], p[2], p[3]); break;
                }
                break;
            case GL_FLOAT:
                switch (array->a[VERT_ATTRIB_POS].size)
                {
                    case 1: array_spu.self.VertexAttrib1fvARB(0, (GLfloat *)p); break;
                    case 2: array_spu.self.VertexAttrib2fvARB(0, (GLfloat *)p); break;
                    case 3: array_spu.self.VertexAttrib3fvARB(0, (GLfloat *)p); break;
                    case 4: array_spu.self.VertexAttrib4fvARB(0, (GLfloat *)p); break;
                }
                break;
            case GL_DOUBLE:
                switch (array->a[VERT_ATTRIB_POS].size)
                {
                    case 1: array_spu.self.VertexAttrib1dvARB(0, (GLdouble *)p); break;
                    case 2: array_spu.self.VertexAttrib2dvARB(0, (GLdouble *)p); break;
                    case 3: array_spu.self.VertexAttrib3dvARB(0, (GLdouble *)p); break;
                    case 4: array_spu.self.VertexAttrib4dvARB(0, (GLdouble *)p); break;
                }
                break;
            default:
                crWarning("Bad datatype for vertex attribute [0] array: 0x%x\n", array->a[0].type);
        }
    }
    else if (array->v.enabled)
    {
        p = array->v.p + index * array->v.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->v.buffer && array->v.buffer->data)
        {
            p = (unsigned char *)(array->v.buffer->data) + (unsigned long)p;
        }
#endif

        switch (array->v.type)
        {
            case GL_SHORT:
                switch (array->v.size)
                {
                    case 2: array_spu.self.Vertex2sv((GLshort *)p); break;
                    case 3: array_spu.self.Vertex3sv((GLshort *)p); break;
                    case 4: array_spu.self.Vertex4sv((GLshort *)p); break;
                }
                break;
            case GL_INT:
                switch (array->v.size)
                {
                    case 2: array_spu.self.Vertex2iv((GLint *)p); break;
                    case 3: array_spu.self.Vertex3iv((GLint *)p); break;
                    case 4: array_spu.self.Vertex4iv((GLint *)p); break;
                }
                break;
            case GL_FLOAT:
                switch (array->v.size)
                {
                    case 2: array_spu.self.Vertex2fv((GLfloat *)p); break;
                    case 3: array_spu.self.Vertex3fv((GLfloat *)p); break;
                    case 4: array_spu.self.Vertex4fv((GLfloat *)p); break;
                }
                break;
            case GL_DOUBLE:
                switch (array->v.size)
                {
                    case 2: array_spu.self.Vertex2dv((GLdouble *)p); break;
                    case 3: array_spu.self.Vertex3dv((GLdouble *)p); break;
                    case 4: array_spu.self.Vertex4dv((GLdouble *)p); break;
                }
                break;
            default:
                crWarning("Bad datatype for vertex array: 0x%x\n", array->v.type);
        }
    }
}

static void ARRAYSPU_APIENTRY arrayspu_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    int i;

    if (count < 0)
    {
        crError("array_spu.self.DrawArrays passed negative count: %d", count);
    }

    if (mode > GL_POLYGON)
    {
        crError("array_spu.self.DrawArrays called with invalid mode: %d", mode);
    }

    array_spu.self.Begin(mode);
    for (i=0; i<count; i++)
    {
        array_spu.self.ArrayElement(first++);
    }
    array_spu.self.End();
}

static void ARRAYSPU_APIENTRY arrayspu_DrawElements(GLenum mode, GLsizei count,
                                                    GLenum type, const GLvoid *indices)
{
    int i;
    GLubyte *p = (GLubyte *)indices;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObject *elementsBuffer = crStateGetCurrent()->bufferobject.elementsBuffer;
#endif

    if (count < 0)
    {
        crError("array_spu.self.DrawElements passed negative count: %d", count);
    }

    if (mode > GL_POLYGON)
    {
        crError("array_spu.self.DrawElements called with invalid mode: %d", mode);
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT)
    {
        crError("array_spu.self.DrawElements called with invalid type: %d", type);
    }

#ifdef CR_ARB_vertex_buffer_object
    if (elementsBuffer && elementsBuffer->data)
    {
        p = (unsigned char *)(elementsBuffer->data) + (unsigned long)p;
    }
#endif
    //crDebug("arrayspu_DrawElements mode:0x%x, count:%d, type:0x%x", mode, count, type);


    array_spu.self.Begin(mode);
    switch (type)
    {
        case GL_UNSIGNED_BYTE:
            for (i=0; i<count; i++)
            {
                array_spu.self.ArrayElement((GLint) *p++);
            }
            break;
        case GL_UNSIGNED_SHORT:
            for (i=0; i<count; i++)
            {
                array_spu.self.ArrayElement((GLint) * (GLushort *) p);
                p+=sizeof (GLushort);
            }
            break;
        case GL_UNSIGNED_INT:
            for (i=0; i<count; i++)
            {
                array_spu.self.ArrayElement((GLint) * (GLuint *) p);
                p+=sizeof (GLuint);
            }
            break;
        default:
            crError( "this can't happen: array_spu.self.DrawElements" );
            break;
    }
    array_spu.self.End();
}

static void ARRAYSPU_APIENTRY arrayspu_DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
    if (start>end)
    {
        crError("array_spu.self.arrayspu_DrawRangeElements start>end (%d>%d)", start, end);
    }

    arrayspu_DrawElements(mode, count, type, indices);
}

static void ARRAYSPU_APIENTRY arrayspu_ColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateColorPointer(size, type, stride, pointer);
    array_spu.child.ColorPointer(size, type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_SecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateSecondaryColorPointerEXT(size, type, stride, pointer);
    array_spu.child.SecondaryColorPointerEXT(size, type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_VertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateVertexPointer(size, type, stride, pointer);
    array_spu.child.VertexPointer(size, type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_TexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateTexCoordPointer(size, type, stride, pointer);
    array_spu.child.TexCoordPointer(size, type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_NormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateNormalPointer(type, stride, pointer);
    array_spu.child.NormalPointer(type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_IndexPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateIndexPointer(type, stride, pointer);
    array_spu.child.IndexPointer(type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_EdgeFlagPointer( GLsizei stride, const GLvoid *pointer )
{
    crStateEdgeFlagPointer(stride, pointer);
    array_spu.child.EdgeFlagPointer(stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_VertexAttribPointerNV( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer )
{
    crStateVertexAttribPointerNV(index, size, type, stride, pointer);
    array_spu.child.VertexAttribPointerNV(index, size, type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_FogCoordPointerEXT( GLenum type, GLsizei stride, const GLvoid *pointer )
{
    crStateFogCoordPointerEXT(type, stride, pointer);
    array_spu.child.FogCoordPointerEXT(type, stride, pointer);
}

static void ARRAYSPU_APIENTRY arrayspu_GetPointerv( GLenum pname, GLvoid **params )
{
    crStateGetPointerv(pname, params);
}

static void ARRAYSPU_APIENTRY arrayspu_EnableClientState( GLenum array )
{
    crStateEnableClientState(array);
    array_spu.child.EnableClientState(array);
}

static void ARRAYSPU_APIENTRY arrayspu_DisableClientState( GLenum array )
{
    crStateDisableClientState(array);
    array_spu.child.DisableClientState(array);
}

static void ARRAYSPU_APIENTRY arrayspu_ClientActiveTextureARB( GLenum texture )
{
    crStateClientActiveTextureARB(texture);
    array_spu.child.ClientActiveTextureARB(texture);
}

static void ARRAYSPU_APIENTRY arrayspu_MultiDrawArraysEXT(GLenum mode, GLint *first, GLsizei *count, GLsizei primcount)
{
    int i;

    if (primcount < 0)
    {
        crError("array_spu.self.MultiDrawArraysEXT passed negative count: %d", primcount);
    }

    if (mode > GL_POLYGON)
    {
        crError("array_spu.self.MultiDrawArraysEXT called with invalid mode: %d", mode);
    }

    for (i = 0; i < primcount; i++)
    {
        array_spu.self.DrawArrays(mode, first[i], count[i]);
    }
}

static void ARRAYSPU_APIENTRY arrayspu_MultiDrawElementsEXT(GLenum mode, GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount)
{
    int i;

    if (primcount < 0)
    {
        crError("array_spu.self.MultiDrawElementsEXT passed negative count: %d", primcount);
    }

    if (mode > GL_POLYGON)
    {
        crError("array_spu.self.MultiDrawElementsEXT called with invalid mode: %d", mode);
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT)
    {
        crError("array_spu.self.MultiDrawElementsEXT called with invalid type: %d", type);
    }

    for (i = 0; i < primcount; i++)
    {
        array_spu.self.DrawElements(mode, count[i], type, indices[i]);
    }
}

/*
 * We need to know when vertex program mode is enabled/disabled
 * in order to handle vertex attribute arrays correctly.
 */
static void ARRAYSPU_APIENTRY arrayspu_Enable(GLenum cap)
{
     if (cap == GL_VERTEX_PROGRAM_NV) {
            crStateGetCurrent()->program.vpEnabled = GL_TRUE;
     }
     array_spu.child.Enable(cap);
}


static void ARRAYSPU_APIENTRY arrayspu_Disable(GLenum cap)
{
     if (cap == GL_VERTEX_PROGRAM_NV) {
            crStateGetCurrent()->program.vpEnabled = GL_FALSE;
     }
     array_spu.child.Disable(cap);
}

/** @todo it's a hack, as GLSL shouldn't blindly reuse this bit from nv_vertex_program*/
static void ARRAYSPU_APIENTRY arrayspu_UseProgram(GLuint program)
{
    crStateGetCurrent()->program.vpEnabled = program>0;
    array_spu.child.UseProgram(program);
}

static void ARRAYSPU_APIENTRY
arrayspu_VertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                                GLboolean normalized, GLsizei stride,
                                const GLvoid *pointer)
{
    crStateVertexAttribPointerARB(index, size, type, normalized, stride, pointer);
    array_spu.child.VertexAttribPointerARB(index, size, type, normalized, stride, pointer);
}


static void ARRAYSPU_APIENTRY
arrayspu_EnableVertexAttribArrayARB(GLuint index)
{
    crStateEnableVertexAttribArrayARB(index);
}


static void ARRAYSPU_APIENTRY
arrayspu_DisableVertexAttribArrayARB(GLuint index)
{
    crStateDisableVertexAttribArrayARB(index);
}


/* We need to implement Push/PopClientAttrib here so that _our_ state
 * tracker gets used.  Also, pass the call onto the next SPU (in case
 * it's the GL_CLIENT_PIXEL_STORE_BIT, etc).
 */
static void ARRAYSPU_APIENTRY
arrayspu_PushClientAttrib( GLbitfield mask )
{
    crStatePushClientAttrib(mask);
    array_spu.child.PushClientAttrib(mask);
}


static void ARRAYSPU_APIENTRY
arrayspu_PopClientAttrib( void )
{
    crStatePopClientAttrib();
    array_spu.child.PopClientAttrib();
}


static void ARRAYSPU_APIENTRY
arrayspu_GenBuffersARB( GLsizei n, GLuint * buffers )
{
    array_spu.child.GenBuffersARB(n, buffers);
}

static void ARRAYSPU_APIENTRY
arrayspu_DeleteBuffersARB( GLsizei n, const GLuint *buffers )
{
    crStateDeleteBuffersARB(n, buffers);
    array_spu.child.DeleteBuffersARB(n, buffers);
}

static void ARRAYSPU_APIENTRY
arrayspu_BindBufferARB( GLenum target, GLuint buffer )
{
    crStateBindBufferARB(target, buffer);
    array_spu.child.BindBufferARB(target, buffer);
}

static GLboolean ARRAYSPU_APIENTRY
arrayspu_IsBufferARB (GLuint buffer)
{
    return array_spu.child.IsBufferARB(buffer);
}

static void ARRAYSPU_APIENTRY
arrayspu_BufferDataARB( GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage )
{
    crStateBufferDataARB(target, size, data, usage);
    array_spu.child.BufferDataARB(target, size, data, usage);
}

static void ARRAYSPU_APIENTRY
arrayspu_BufferSubDataARB( GLenum target, GLintptrARB offset,
                           GLsizeiptrARB size, const GLvoid * data )
{
    crStateBufferSubDataARB(target, offset, size, data);
    array_spu.child.BufferSubDataARB(target, offset, size, data);
}

static void ARRAYSPU_APIENTRY
arrayspu_GetBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data)
{
    crStateGetBufferSubDataARB(target, offset, size, data);
}

static void * ARRAYSPU_APIENTRY
arrayspu_MapBufferARB(GLenum target, GLenum access)
{
    return crStateMapBufferARB(target, access);
}

static GLboolean ARRAYSPU_APIENTRY
arrayspu_UnmapBufferARB(GLenum target)
{
    crStateUnmapBufferARB(target);
    return array_spu.child.UnmapBufferARB(target);
}

static void ARRAYSPU_APIENTRY
arrayspu_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
    crStateGetBufferParameterivARB(target, pname, params);
}

static void ARRAYSPU_APIENTRY
arrayspu_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
    crStateGetBufferPointervARB(target, pname, params);
}

static void ARRAYSPU_APIENTRY
arrayspu_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *p)
{
    crStateInterleavedArrays(format, stride, p);
}

static GLint ARRAYSPU_APIENTRY
arrayspu_CreateContext( const char *dpyName, GLint visual, GLint shareCtx )
{
    GLint ctx, slot;

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&_ArrayMutex);
#endif

    ctx = array_spu.child.CreateContext(dpyName, visual, shareCtx);

    /* find an empty context slot */
    for (slot = 0; slot < array_spu.numContexts; slot++) {
        if (!array_spu.context[slot].clientState) {
            /* found empty slot */
            break;
        }
    }
    if (slot == array_spu.numContexts) {
        array_spu.numContexts++;
    }

    array_spu.context[slot].clientState = crStateCreateContext(NULL, visual, NULL);
    array_spu.context[slot].clientCtx = ctx;
#ifdef CR_ARB_vertex_buffer_object
    array_spu.context[slot].clientState->bufferobject.retainBufferData = GL_TRUE;
#endif

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&_ArrayMutex);
#endif

    return ctx;
}

static void ARRAYSPU_APIENTRY
arrayspu_MakeCurrent( GLint window, GLint nativeWindow, GLint ctx )
{
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&_ArrayMutex);
#endif
    array_spu.child.MakeCurrent(window, nativeWindow, ctx);

    if (ctx) {
        int slot;

        for (slot=0; slot<array_spu.numContexts; ++slot)
            if (array_spu.context[slot].clientCtx == ctx) break;
        CRASSERT(slot < array_spu.numContexts);

        crStateMakeCurrent(array_spu.context[slot].clientState);
    }
    else
    {
        crStateMakeCurrent(NULL);
    }

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&_ArrayMutex);
#endif
}

static void ARRAYSPU_APIENTRY
arrayspu_DestroyContext( GLint ctx )
{
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&_ArrayMutex);
#endif
    array_spu.child.DestroyContext(ctx);

    if (ctx) {
        int slot;

        for (slot=0; slot<array_spu.numContexts; ++slot)
            if (array_spu.context[slot].clientCtx == ctx) break;
        CRASSERT(slot < array_spu.numContexts);

        crStateDestroyContext(array_spu.context[slot].clientState);

        array_spu.context[slot].clientState = NULL;
        array_spu.context[slot].clientCtx = 0;
    }

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&_ArrayMutex);
#endif
}

static void ARRAYSPU_APIENTRY
arrayspu_VBoxAttachThread(void)
{
    crStateVBoxAttachThread();
    array_spu.child.VBoxAttachThread();
}

static void ARRAYSPU_APIENTRY
arrayspu_VBoxDetachThread(void)
{
    crStateVBoxDetachThread();
    array_spu.child.VBoxDetachThread();
}


SPUNamedFunctionTable _cr_array_table[] = {
    { "ArrayElement", (SPUGenericFunction) arrayspu_ArrayElement },
    { "DrawArrays", (SPUGenericFunction) arrayspu_DrawArrays},
    { "DrawElements", (SPUGenericFunction)  arrayspu_DrawElements},
    { "DrawRangeElements", (SPUGenericFunction) arrayspu_DrawRangeElements},
    { "ColorPointer", (SPUGenericFunction) arrayspu_ColorPointer},
    { "SecondaryColorPointerEXT", (SPUGenericFunction) arrayspu_SecondaryColorPointerEXT},
    { "VertexPointer", (SPUGenericFunction) arrayspu_VertexPointer},
    { "TexCoordPointer", (SPUGenericFunction) arrayspu_TexCoordPointer},
    { "NormalPointer", (SPUGenericFunction) arrayspu_NormalPointer},
    { "IndexPointer", (SPUGenericFunction) arrayspu_IndexPointer},
    { "EdgeFlagPointer", (SPUGenericFunction) arrayspu_EdgeFlagPointer},
    { "VertexAttribPointerNV", (SPUGenericFunction) arrayspu_VertexAttribPointerNV},
    { "FogCoordPointerEXT", (SPUGenericFunction) arrayspu_FogCoordPointerEXT},
    { "GetPointerv", (SPUGenericFunction) arrayspu_GetPointerv},
    { "EnableClientState", (SPUGenericFunction) arrayspu_EnableClientState},
    { "DisableClientState", (SPUGenericFunction) arrayspu_DisableClientState},
    { "ClientActiveTextureARB", (SPUGenericFunction) arrayspu_ClientActiveTextureARB },
    { "MultiDrawArraysEXT", (SPUGenericFunction) arrayspu_MultiDrawArraysEXT },
    { "MultiDrawElementsEXT", (SPUGenericFunction) arrayspu_MultiDrawElementsEXT },
    { "Enable", (SPUGenericFunction) arrayspu_Enable },
    { "Disable", (SPUGenericFunction) arrayspu_Disable },
    { "PushClientAttrib", (SPUGenericFunction) arrayspu_PushClientAttrib },
    { "PopClientAttrib", (SPUGenericFunction) arrayspu_PopClientAttrib },
    { "VertexAttribPointerARB", (SPUGenericFunction) arrayspu_VertexAttribPointerARB },
    { "EnableVertexAttribArrayARB", (SPUGenericFunction) arrayspu_EnableVertexAttribArrayARB },
    { "DisableVertexAttribArrayARB", (SPUGenericFunction) arrayspu_DisableVertexAttribArrayARB },
    { "GenBuffersARB", (SPUGenericFunction) arrayspu_GenBuffersARB },
    { "DeleteBuffersARB", (SPUGenericFunction) arrayspu_DeleteBuffersARB },
    { "BindBufferARB", (SPUGenericFunction) arrayspu_BindBufferARB },
    { "IsBufferARB", (SPUGenericFunction) arrayspu_IsBufferARB },
    { "BufferDataARB", (SPUGenericFunction) arrayspu_BufferDataARB },
    { "BufferSubDataARB", (SPUGenericFunction) arrayspu_BufferSubDataARB },
    { "GetBufferSubDataARB", (SPUGenericFunction) arrayspu_GetBufferSubDataARB },
    { "MapBufferARB", (SPUGenericFunction) arrayspu_MapBufferARB },
    { "UnmapBufferARB", (SPUGenericFunction) arrayspu_UnmapBufferARB },
    { "GetBufferParameterivARB", (SPUGenericFunction) arrayspu_GetBufferParameterivARB},
    { "GetBufferPointervARB", (SPUGenericFunction) arrayspu_GetBufferPointervARB},
    { "InterleavedArrays", (SPUGenericFunction) arrayspu_InterleavedArrays},
    { "CreateContext", (SPUGenericFunction) arrayspu_CreateContext},
    { "MakeCurrent", (SPUGenericFunction) arrayspu_MakeCurrent},
    { "DestroyContext", (SPUGenericFunction) arrayspu_DestroyContext},
    { "UseProgram", (SPUGenericFunction) arrayspu_UseProgram},
    { "VBoxAttachThread", (SPUGenericFunction) arrayspu_VBoxAttachThread},
    { "VBoxDetachThread", (SPUGenericFunction) arrayspu_VBoxDetachThread},
    { NULL, NULL }
};
