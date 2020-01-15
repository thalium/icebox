/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_version.h"
#include "state/cr_limits.h"
#include "cr_glstate.h"

/*Convert from GLint to GLfloat in [-1.f,1.f]*/
#define CRP_I2F_NORM(i)  ((2.f*((GLint)(i))+1.f) * (1.f/4294967294.f))
/*Convert from GLshort to GLfloat in [-1.f,1.f]*/
#define CRP_S2F_NORM(s)  ((2.f*((GLshort)(s))+1.f) * (1.f/65535.f))
/*Convert from GLbyte to GLfloat in [-1.f,1.f]*/
#define CRP_B2F_NORM(b)  ((2.f*((GLbyte)(b))+1.f) * (1.f/255.f))
/*Convert from GLuint to GLfloat in [0.f,1.f]*/
#define CRP_UI2F_NORM(i) ((GLfloat)(i) * (1.f/4294967295.f))
/*Convert from GLushort to GLfloat in [0.f,1.f]*/
#define CRP_US2F_NORM(s) ((GLfloat)(s) * (1.f/65535.f))
/*Convert from GLubyte to GLfloat in [0.f,1.f]*/
#define CRP_UB2F_NORM(b) ((GLfloat)(b) * (1.f/255.f))

static void crPackVertexAttrib(const CRVertexArrays *array, unsigned int attr, GLint index)
{
    unsigned char *p = array->a[attr].p + index * array->a[attr].stride;

#ifdef DEBUG_misha
    Assert(index >= 0);
#endif

#ifdef CR_ARB_vertex_buffer_object
    if (array->a[attr].buffer && array->a[attr].buffer->data)
    {
        Assert(((uintptr_t)p) < array->a[attr].buffer->size);
        p = (unsigned char *)(array->a[attr].buffer->data) + (uintptr_t)p;
    }
#endif

    if (!p)
    {
        crWarning("crPackVertexAttrib: NULL ptr!");
        return;
    }

    switch (array->a[attr].type)
    {
        case GL_SHORT:
        {
            GLshort *sPtr = (GLshort*) p;
            switch (array->a[attr].size)
            {
                case 1:
                    if (array->a[attr].normalized)
                        crPackVertexAttrib1fARB(attr, CRP_S2F_NORM(sPtr[0]));
                    else
                        crPackVertexAttrib1svARB(attr, sPtr);
                    break;
                case 2:
                    if (array->a[attr].normalized)
                        crPackVertexAttrib2fARB(attr, CRP_S2F_NORM(sPtr[0]), CRP_S2F_NORM(sPtr[1]));
                    else
                        crPackVertexAttrib2svARB(attr, sPtr);
                    break;
                case 3:
                    if (array->a[attr].normalized)
                        crPackVertexAttrib3fARB(attr, CRP_S2F_NORM(sPtr[0]), CRP_S2F_NORM(sPtr[1]), CRP_S2F_NORM(sPtr[2]));
                    else
                        crPackVertexAttrib3svARB(attr, sPtr);
                    break;
                case 4:
                    if (array->a[attr].normalized)
                        crPackVertexAttrib4NsvARB(attr, sPtr);
                    else
                        crPackVertexAttrib4svARB(attr, sPtr);
                    break;
            }
            break;
        }
        case GL_UNSIGNED_SHORT:
        {
            GLushort *usPtr = (GLushort*) p;
            if (array->a[attr].normalized)
            {
                switch (array->a[attr].size)
                {
                    case 1:
                        crPackVertexAttrib1fARB(attr, CRP_US2F_NORM(usPtr[0]));
                        break;
                    case 2:
                        crPackVertexAttrib2fARB(attr, CRP_US2F_NORM(usPtr[0]), CRP_US2F_NORM(usPtr[1]));
                        break;
                    case 3:
                        crPackVertexAttrib3fARB(attr, CRP_US2F_NORM(usPtr[0]), CRP_US2F_NORM(usPtr[1]), CRP_US2F_NORM(usPtr[2]));
                        break;
                    case 4:
                        crPackVertexAttrib4NusvARB(attr, usPtr);
                        break;
                }
            }
            else
            {
                GLushort usv[4];
                switch (array->a[attr].size)
                {
                    case 4:
                        crPackVertexAttrib4usvARB(attr, usPtr);
                        break;
                    case 3: usv[2] = usPtr[2]; RT_FALL_THRU();
                    case 2: usv[1] = usPtr[1]; RT_FALL_THRU();
                    case 1:
                        usv[0] = usPtr[0];
                        crPackVertexAttrib4usvARB(attr, usv);
                        break;
                }
            }
            break;
        }
        case GL_INT:
        {
            GLint *iPtr = (GLint*) p;
            if (array->a[attr].normalized)
            {
                switch (array->a[attr].size)
                {
                    case 1:
                        crPackVertexAttrib1fARB(attr, CRP_I2F_NORM(iPtr[0]));
                        break;
                    case 2:
                        crPackVertexAttrib2fARB(attr, CRP_I2F_NORM(iPtr[0]), CRP_I2F_NORM(iPtr[1]));
                        break;
                    case 3:
                        crPackVertexAttrib3fARB(attr, CRP_I2F_NORM(iPtr[0]), CRP_I2F_NORM(iPtr[1]), CRP_I2F_NORM(iPtr[2]));
                        break;
                    case 4:
                        crPackVertexAttrib4NivARB(attr, iPtr);
                        break;
                }
            }
            else
            {
                GLint iv[4];
                switch (array->a[attr].size)
                {
                    case 4:
                        crPackVertexAttrib4ivARB(attr, iPtr);
                        break;
                    case 3: iv[2] = iPtr[2]; RT_FALL_THRU();
                    case 2: iv[1] = iPtr[1]; RT_FALL_THRU();
                    case 1:
                        iv[0] = iPtr[0];
                        crPackVertexAttrib4ivARB(attr, iv);
                        break;
                }
            }
            break;
        }
        case GL_UNSIGNED_INT:
        {
            GLuint *uiPtr = (GLuint*) p;
            if (array->a[attr].normalized)
            {
                switch (array->a[attr].size)
                {
                    case 1:
                        crPackVertexAttrib1fARB(attr, CRP_UI2F_NORM(uiPtr[0]));
                        break;
                    case 2:
                        crPackVertexAttrib2fARB(attr, CRP_UI2F_NORM(uiPtr[0]), CRP_UI2F_NORM(uiPtr[1]));
                        break;
                    case 3:
                        crPackVertexAttrib3fARB(attr, CRP_UI2F_NORM(uiPtr[0]), CRP_UI2F_NORM(uiPtr[1]), CRP_UI2F_NORM(uiPtr[2]));
                        break;
                    case 4:
                        crPackVertexAttrib4NuivARB(attr, uiPtr);
                        break;
                }
            }
            else
            {
                GLuint uiv[4];
                switch (array->a[attr].size)
                {
                    case 4:
                        crPackVertexAttrib4uivARB(attr, uiPtr);
                        break;
                    case 3: uiv[2] = uiPtr[2]; RT_FALL_THRU();
                    case 2: uiv[1] = uiPtr[1]; RT_FALL_THRU();
                    case 1:
                        uiv[0] = uiPtr[0];
                        crPackVertexAttrib4uivARB(attr, uiv);
                        break;
                }
            }
            break;
        }
        case GL_FLOAT:
            switch (array->a[attr].size)
            {
                case 1: crPackVertexAttrib1fvARB(attr, (GLfloat *)p); break;
                case 2: crPackVertexAttrib2fvARB(attr, (GLfloat *)p); break;
                case 3: crPackVertexAttrib3fvARB(attr, (GLfloat *)p); break;
                case 4: crPackVertexAttrib4fvARB(attr, (GLfloat *)p); break;
            }
            break;
        case GL_DOUBLE:
            switch (array->a[attr].size)
            {
                case 1: crPackVertexAttrib1dvARB(attr, (GLdouble *)p); break;
                case 2: crPackVertexAttrib2dvARB(attr, (GLdouble *)p); break;
                case 3: crPackVertexAttrib3dvARB(attr, (GLdouble *)p); break;
                case 4: crPackVertexAttrib4dvARB(attr, (GLdouble *)p); break;
            }
            break;
        case GL_BYTE:
        {
            GLbyte *bPtr = (GLbyte*) p;
            if (array->a[attr].normalized)
            {
                switch (array->a[attr].size)
                {
                    case 1:
                        crPackVertexAttrib1fARB(attr, CRP_B2F_NORM(bPtr[0]));
                        break;
                    case 2:
                        crPackVertexAttrib2fARB(attr, CRP_B2F_NORM(bPtr[0]), CRP_B2F_NORM(bPtr[1]));
                        break;
                    case 3:
                        crPackVertexAttrib3fARB(attr, CRP_B2F_NORM(bPtr[0]), CRP_B2F_NORM(bPtr[1]), CRP_B2F_NORM(bPtr[2]));
                        break;
                    case 4:
                        crPackVertexAttrib4NbvARB(attr, bPtr);
                        break;
                }
            }
            else
            {
                GLbyte bv[4];
                switch (array->a[attr].size)
                {
                    case 4:
                        crPackVertexAttrib4bvARB(attr, bPtr);
                        break;
                    case 3: bv[2] = bPtr[2]; RT_FALL_THRU();
                    case 2: bv[1] = bPtr[1]; RT_FALL_THRU();
                    case 1:
                        bv[0] = bPtr[0];
                        crPackVertexAttrib4bvARB(attr, bv);
                        break;
                }
            }
            break;
        }
        case GL_UNSIGNED_BYTE:
        {
            GLubyte *ubPtr = (GLubyte*) p;
            if (array->a[attr].normalized)
            {
                switch (array->a[attr].size)
                {
                    case 1:
                        crPackVertexAttrib1fARB(attr, CRP_UB2F_NORM(ubPtr[0]));
                        break;
                    case 2:
                        crPackVertexAttrib2fARB(attr, CRP_UB2F_NORM(ubPtr[0]), CRP_UB2F_NORM(ubPtr[1]));
                        break;
                    case 3:
                        crPackVertexAttrib3fARB(attr, CRP_UB2F_NORM(ubPtr[0]), CRP_UB2F_NORM(ubPtr[1]), CRP_UB2F_NORM(ubPtr[2]));
                        break;
                    case 4:
                        crPackVertexAttrib4NubvARB(attr, ubPtr);
                        break;
                }
            }
            else
            {
                GLubyte ubv[4];
                switch (array->a[attr].size)
                {
                    case 4:
                        crPackVertexAttrib4ubvARB(attr, ubPtr);
                        break;
                    case 3: ubv[2] = ubPtr[2]; RT_FALL_THRU();
                    case 2: ubv[1] = ubPtr[1]; RT_FALL_THRU();
                    case 1:
                        ubv[0] = ubPtr[0];
                        crPackVertexAttrib4ubvARB(attr, ubv);
                        break;
                }
            }
            break;
        }
        default:
            crWarning("Bad datatype for vertex attribute [%d] array: 0x%x\n",
                       attr, array->a[attr].type);
    }
}

/*
 * Expand glArrayElement into crPackVertex/Color/Normal/etc.
 */
void
crPackExpandArrayElement(GLint index, CRClientState *c, const GLfloat *pZva)
{
    unsigned char *p;
    unsigned int unit, attr;
    const CRVertexArrays *array = &(c->array);
    const GLboolean vpEnabled = crStateGetCurrent(g_pStateTracker)->program.vpEnabled;

    /*crDebug("crPackExpandArrayElement(%i)", index);*/

    if (array->n.enabled && !(vpEnabled && array->a[VERT_ATTRIB_NORMAL].enabled))
    {
        p = array->n.p + index * array->n.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->n.buffer && array->n.buffer->data)
        {
            p = (unsigned char *)(array->n.buffer->data) + (uintptr_t)p;
        }
#endif

        switch (array->n.type)
        {
            case GL_BYTE: crPackNormal3bv((GLbyte *)p); break;
            case GL_SHORT: crPackNormal3sv((GLshort *)p); break;
            case GL_INT: crPackNormal3iv((GLint *)p); break;
            case GL_FLOAT: crPackNormal3fv((GLfloat *)p); break;
            case GL_DOUBLE: crPackNormal3dv((GLdouble *)p); break;
            default:
                crWarning("Unhandled: crPackExpandArrayElement, array->n.type 0x%x", array->n.type);
        }
    }

    if (array->c.enabled && !(vpEnabled && array->a[VERT_ATTRIB_COLOR0].enabled))
    {
        p = array->c.p + index * array->c.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->c.buffer && array->c.buffer->data)
        {
            p = (unsigned char *)(array->c.buffer->data) + (uintptr_t)p;
        }
#endif

        switch (array->c.type)
        {
            case GL_BYTE:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3bv((GLbyte *)p); break;
                    case 4: crPackColor4bv((GLbyte *)p); break;
                }
                break;
            case GL_UNSIGNED_BYTE:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3ubv((GLubyte *)p); break;
                    case 4: crPackColor4ubv((GLubyte *)p); break;
                }
                break;
            case GL_SHORT:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3sv((GLshort *)p); break;
                    case 4: crPackColor4sv((GLshort *)p); break;
                }
                break;
            case GL_UNSIGNED_SHORT:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3usv((GLushort *)p); break;
                    case 4: crPackColor4usv((GLushort *)p); break;
                }
                break;
            case GL_INT:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3iv((GLint *)p); break;
                    case 4: crPackColor4iv((GLint *)p); break;
                }
                break;
            case GL_UNSIGNED_INT:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3uiv((GLuint *)p); break;
                    case 4: crPackColor4uiv((GLuint *)p); break;
                }
                break;
            case GL_FLOAT:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3fv((GLfloat *)p); break;
                    case 4: crPackColor4fv((GLfloat *)p); break;
                }
                break;
            case GL_DOUBLE:
                switch (c->array.c.size)
                {
                    case 3: crPackColor3dv((GLdouble *)p); break;
                    case 4: crPackColor4dv((GLdouble *)p); break;
                }
                break;
            default:
                crWarning("Unhandled: crPackExpandArrayElement, array->c.type 0x%x", array->c.type);
        }
    }

#ifdef CR_EXT_secondary_color
    if (array->s.enabled && !(vpEnabled && array->a[VERT_ATTRIB_COLOR1].enabled))
    {
        p = array->s.p + index * array->s.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->s.buffer && array->s.buffer->data)
        {
            p = (unsigned char *)(array->s.buffer->data) + (uintptr_t)p;
        }
#endif

        switch (array->s.type)
        {
            case GL_BYTE:
                crPackSecondaryColor3bvEXT((GLbyte *)p); break;
            case GL_UNSIGNED_BYTE:
                crPackSecondaryColor3ubvEXT((GLubyte *)p); break;
            case GL_SHORT:
                crPackSecondaryColor3svEXT((GLshort *)p); break;
            case GL_UNSIGNED_SHORT:
                crPackSecondaryColor3usvEXT((GLushort *)p); break;
            case GL_INT:
                crPackSecondaryColor3ivEXT((GLint *)p); break;
            case GL_UNSIGNED_INT:
                crPackSecondaryColor3uivEXT((GLuint *)p); break;
            case GL_FLOAT:
                crPackSecondaryColor3fvEXT((GLfloat *)p); break;
            case GL_DOUBLE:
                crPackSecondaryColor3dvEXT((GLdouble *)p); break;
            default:
                crWarning("Unhandled: crPackExpandArrayElement, array->s.type 0x%x", array->s.type);
        }
    }
#endif /* CR_EXT_secondary_color */


#ifdef CR_EXT_fog_coord
    if (array->f.enabled && !(vpEnabled && array->a[VERT_ATTRIB_FOG].enabled))
    {
        p = array->f.p + index * array->f.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->f.buffer && array->f.buffer->data)
        {
            p = (unsigned char *)(array->f.buffer->data) + (uintptr_t)p;
        }
#endif
        crPackFogCoordfEXT( *((GLfloat *) p) );
    }
#endif /* CR_EXT_fog_coord */

    for (unit = 0 ; unit < CR_MAX_TEXTURE_UNITS ; unit++)
    {
        if (array->t[unit].enabled && !(vpEnabled && array->a[VERT_ATTRIB_TEX0+unit].enabled))
        {
            p = array->t[unit].p + index * array->t[unit].stride;

#ifdef CR_ARB_vertex_buffer_object
            if (array->t[unit].buffer && array->t[unit].buffer->data)
            {
                p = (unsigned char *)(array->t[unit].buffer->data) + (uintptr_t)p;
            }
#endif

            switch (array->t[unit].type)
            {
                case GL_SHORT:
                    switch (array->t[unit].size)
                    {
                        case 1: crPackMultiTexCoord1svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                        case 2: crPackMultiTexCoord2svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                        case 3: crPackMultiTexCoord3svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                        case 4: crPackMultiTexCoord4svARB(GL_TEXTURE0_ARB + unit, (GLshort *)p); break;
                    }
                    break;
                case GL_INT:
                    switch (array->t[unit].size)
                    {
                        case 1: crPackMultiTexCoord1ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                        case 2: crPackMultiTexCoord2ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                        case 3: crPackMultiTexCoord3ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                        case 4: crPackMultiTexCoord4ivARB(GL_TEXTURE0_ARB + unit, (GLint *)p); break;
                    }
                    break;
                case GL_FLOAT:
                    switch (array->t[unit].size)
                    {
                        case 1: crPackMultiTexCoord1fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                        case 2: crPackMultiTexCoord2fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                        case 3: crPackMultiTexCoord3fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                        case 4: crPackMultiTexCoord4fvARB(GL_TEXTURE0_ARB + unit, (GLfloat *)p); break;
                    }
                    break;
                case GL_DOUBLE:
                    switch (array->t[unit].size)
                    {
                        case 1: crPackMultiTexCoord1dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                        case 2: crPackMultiTexCoord2dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                        case 3: crPackMultiTexCoord3dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                        case 4: crPackMultiTexCoord4dvARB(GL_TEXTURE0_ARB + unit, (GLdouble *)p); break;
                    }
                    break;
                default:
                    crWarning("Unhandled: crPackExpandArrayElement, array->t[%i].type 0x%x", unit, array->t[unit].type);
            }
        }
    }

    if (array->i.enabled)
    {
        p = array->i.p + index * array->i.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->i.buffer && array->i.buffer->data)
        {
            p = (unsigned char *)(array->i.buffer->data) + (uintptr_t)p;
        }
#endif

        switch (array->i.type)
        {
            case GL_SHORT: crPackIndexsv((GLshort *)p); break;
            case GL_INT: crPackIndexiv((GLint *)p); break;
            case GL_FLOAT: crPackIndexfv((GLfloat *)p); break;
            case GL_DOUBLE: crPackIndexdv((GLdouble *)p); break;
            default:
                crWarning("Unhandled: crPackExpandArrayElement, array->i.type 0x%x", array->i.type);
        }
    }

    if (array->e.enabled)
    {
        p = array->e.p + index * array->e.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->e.buffer && array->e.buffer->data)
        {
            p = (unsigned char *)(array->e.buffer->data) + (uintptr_t)p;
        }
#endif

        crPackEdgeFlagv(p);
    }

    for (attr = 1; attr < VERT_ATTRIB_MAX; attr++)
    {
        if (array->a[attr].enabled)
        {
            crPackVertexAttrib(array, attr, index);
        }
    }

    if (array->a[VERT_ATTRIB_POS].enabled)
    {
        crPackVertexAttrib(array, VERT_ATTRIB_POS, index);
    }
    else if (pZva)
    {
        crPackVertexAttrib4fvARB(VERT_ATTRIB_POS, pZva);
    }
    else if (array->v.enabled)
    {
        p = array->v.p + index * array->v.stride;

#ifdef CR_ARB_vertex_buffer_object
        if (array->v.buffer && array->v.buffer->data)
        {
            p = (unsigned char *)(array->v.buffer->data) + (uintptr_t)p;
        }
#endif
        switch (array->v.type)
        {
            case GL_SHORT:
                switch (c->array.v.size)
                {
                    case 2: crPackVertex2sv((GLshort *)p); break;
                    case 3: crPackVertex3sv((GLshort *)p); break;
                    case 4: crPackVertex4sv((GLshort *)p); break;
                }
                break;
            case GL_INT:
                switch (c->array.v.size)
                {
                    case 2: crPackVertex2iv((GLint *)p); break;
                    case 3: crPackVertex3iv((GLint *)p); break;
                    case 4: crPackVertex4iv((GLint *)p); break;
                }
                break;
            case GL_FLOAT:
                switch (c->array.v.size)
                {
                    case 2: crPackVertex2fv((GLfloat *)p); break;
                    case 3: crPackVertex3fv((GLfloat *)p); break;
                    case 4: crPackVertex4fv((GLfloat *)p); break;
                }
                break;
            case GL_DOUBLE:
                switch (c->array.v.size)
                {
                    case 2: crPackVertex2dv((GLdouble *)p); break;
                    case 3: crPackVertex3dv((GLdouble *)p); break;
                    case 4: crPackVertex4dv((GLdouble *)p); break;
                }
                break;
            default:
                crWarning("Unhandled: crPackExpandArrayElement, array->v.type 0x%x", array->v.type);
        }
    }
}


void
crPackExpandDrawArrays(GLenum mode, GLint first, GLsizei count, CRClientState *c, const GLfloat *pZva)
{
    int i;

    if (count < 0)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_VALUE, "crPackDrawArrays(negative count)");
        return;
    }

    if (mode > GL_POLYGON)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM, "crPackDrawArrays(bad mode)");
        return;
    }

    crPackBegin(mode);
    for (i=0; i<count; i++)
    {
        crPackExpandArrayElement(first + i, c, pZva);
    }
    crPackEnd();
}

static GLsizei crPackElementsIndexSize(GLenum type)
{
    switch (type)
    {
        case GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);
        case GL_UNSIGNED_SHORT:
            return sizeof(GLushort);
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        default:
            crError("Unknown type 0x%x in crPackElementsIndexSize", type);
            return 0;
    }
}

void PACK_APIENTRY
crPackDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    unsigned char *data_ptr, *start_ptr;
    int packet_length = sizeof(int) + sizeof(mode) + sizeof(count) + sizeof(type) + sizeof(GLuint);
    GLsizei indexsize;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObject *elementsBuffer = crStateGetCurrent(g_pStateTracker)->bufferobject.elementsBuffer;
    packet_length += sizeof(GLint);
    if (elementsBuffer && elementsBuffer->id)
    {
        /** @todo not sure it's possible, and not sure what to do*/
        if (!elementsBuffer->data)
        {
            crWarning("crPackDrawElements: trying to use bound but empty elements buffer, ignoring.");
            return;
        }
        indexsize = 0;
    }
    else
#endif
    {
        indexsize = crPackElementsIndexSize(type);
    }

    packet_length += count * indexsize;

    start_ptr = data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA_AI(GLenum, CR_DRAWELEMENTS_EXTEND_OPCODE );
    WRITE_DATA_AI(GLenum, mode );
    WRITE_DATA_AI(GLsizei, count);
    WRITE_DATA_AI(GLenum, type);
    WRITE_DATA_AI(GLuint, (GLuint) ((uintptr_t) indices) );
#ifdef CR_ARB_vertex_buffer_object
    WRITE_DATA_AI(GLint, (GLint)(indexsize>0));
#endif
    if (indexsize>0)
    {
        crMemcpy(data_ptr, indices, count * indexsize);
    }
    crHugePacket(CR_EXTEND_OPCODE, start_ptr);
    crPackFree(start_ptr);
}

void PACK_APIENTRY
crPackDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                        GLenum type, const GLvoid *indices)
{
    unsigned char *data_ptr, *start_ptr;
    int packet_length = sizeof(int) + sizeof(mode) + sizeof(start)
        + sizeof(end) + sizeof(count) + sizeof(type) + sizeof(GLuint);
    GLsizei indexsize;

#ifdef CR_ARB_vertex_buffer_object
    CRBufferObject *elementsBuffer = crStateGetCurrent(g_pStateTracker)->bufferobject.elementsBuffer;
    packet_length += sizeof(GLint);
    if (elementsBuffer && elementsBuffer->id)
    {
        /** @todo not sure it's possible, and not sure what to do*/
        if (!elementsBuffer->data)
        {
            crWarning("crPackDrawElements: trying to use bound but empty elements buffer, ignoring.");
            return;
        }
        indexsize = 0;
    }
    else
#endif
    {
      indexsize = crPackElementsIndexSize(type);
    }

    packet_length += count * indexsize;

    start_ptr = data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA_AI(GLenum, CR_DRAWRANGEELEMENTS_EXTEND_OPCODE);
    WRITE_DATA_AI(GLenum, mode);
    WRITE_DATA_AI(GLuint, start);
    WRITE_DATA_AI(GLuint, end);
    WRITE_DATA_AI(GLsizei, count);
    WRITE_DATA_AI(GLenum, type);
    WRITE_DATA_AI(GLuint, (GLuint) ((uintptr_t) indices));
#ifdef CR_ARB_vertex_buffer_object
    WRITE_DATA_AI(GLint, (GLint) (indexsize>0));
#endif
    if (indexsize>0)
    {
        crMemcpy(data_ptr, indices, count * indexsize);
    }
    crHugePacket(CR_EXTEND_OPCODE, start_ptr);
    crPackFree(start_ptr);
}


/**
 * Expand glDrawElements into crPackBegin/Vertex/End, etc commands.
 * Note: if mode==999, don't call glBegin/glEnd.
 */
void
crPackExpandDrawElements(GLenum mode, GLsizei count, GLenum type,
                         const GLvoid *indices, CRClientState *c, const GLfloat *pZva)
{
    int i;
    GLubyte *p = (GLubyte *)indices;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObject *elementsBuffer = crStateGetCurrent(g_pStateTracker)->bufferobject.elementsBuffer;
#endif

    if (count < 0)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                "crPackDrawElements(negative count)");
        return;
    }

    if (mode > GL_POLYGON && mode != 999)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "crPackDrawElements(bad mode)");
        return;
    }

    if (type != GL_UNSIGNED_BYTE &&
            type != GL_UNSIGNED_SHORT &&
            type != GL_UNSIGNED_INT)
    {
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                "crPackDrawElements(bad type)");
        return;
    }

#ifdef CR_ARB_vertex_buffer_object
    if (elementsBuffer && elementsBuffer->data)
    {
        p = (unsigned char *)(elementsBuffer->data) + (uintptr_t)p;
    }
#endif

    if (mode != 999)
        crPackBegin(mode);

    /*crDebug("crPackExpandDrawElements mode:0x%x, count:%d, type:0x%x", mode, count, type);*/

    switch (type)
    {
        case GL_UNSIGNED_BYTE:
            for (i=0; i<count; i++)
            {
                crPackExpandArrayElement((GLint) *p++, c, pZva);
            }
            break;
        case GL_UNSIGNED_SHORT:
            for (i=0; i<count; i++)
            {
                crPackExpandArrayElement((GLint) * (GLushort *) p, c, pZva);
                p+=sizeof (GLushort);
            }
            break;
        case GL_UNSIGNED_INT:
            for (i=0; i<count; i++)
            {
                crPackExpandArrayElement((GLint) * (GLuint *) p, c, pZva);
                p+=sizeof (GLuint);
            }
            break;
        default:
            crError( "this can't happen: array_spu.self.DrawElements" );
            break;
    }

    if (mode != 999)
        crPackEnd();
}


/**
 * Convert a glDrawElements command into a sequence of ArrayElement() calls.
 * NOTE: Caller must issue the glBegin/glEnd.
 */
void
crPackUnrollDrawElements(GLsizei count, GLenum type,
                                                 const GLvoid *indices)
{
    int i;

    switch (type) {
    case GL_UNSIGNED_BYTE:
        {
            const GLubyte *p = (const GLubyte *) indices;
            for (i = 0; i < count; i++)
                crPackArrayElement(p[i]);
        }
        break;
    case GL_UNSIGNED_SHORT:
        {
            const GLushort *p = (const GLushort *) indices;
            for (i = 0; i < count; i++)
                crPackArrayElement(p[i]);
        }
        break;
    case GL_UNSIGNED_INT:
        {
            const GLuint *p = (const GLuint *) indices;
            for (i = 0; i < count; i++)
                crPackArrayElement(p[i]);
        }
        break;
    default:
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackUnrollDrawElements(bad type)");
    }
}



/*
 * glDrawRangeElements, expanded into crPackBegin/Vertex/End/etc.
 */
void
crPackExpandDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, CRClientState *c, const GLfloat *pZva)
{
    if (start>end)
    {
        crWarning("crPackExpandDrawRangeElements start>end (%d>%d)", start, end);
        return;
    }

    crPackExpandDrawElements(mode, count, type, indices, c, pZva);
}


#ifdef CR_EXT_multi_draw_arrays
/*
 * Pack real DrawArrays commands.
 */
void PACK_APIENTRY
crPackMultiDrawArraysEXT( GLenum mode, GLint *first, GLsizei *count,
                                                    GLsizei primcount )
{
   GLint i;
   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         crPackDrawArrays(mode, first[i], count[i]);
      }
   }
}


/*
 * Pack with crPackBegin/Vertex/End/etc.
 */
void
crPackExpandMultiDrawArraysEXT( GLenum mode, GLint *first, GLsizei *count,
                                                                GLsizei primcount, CRClientState *c, const GLfloat *pZva )
{
   GLint i;
   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         crPackExpandDrawArrays(mode, first[i], count[i], c, pZva);
      }
   }
}


/*
 * Pack real DrawElements commands.
 */
void PACK_APIENTRY
crPackMultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type,
                            const GLvoid **indices, GLsizei primcount )
{
   GLint i;
   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         crPackDrawElements(mode, count[i], type, indices[i]);
      }
   }
}


/*
 * Pack with crPackBegin/Vertex/End/etc.
 */
void
crPackExpandMultiDrawElementsEXT( GLenum mode, const GLsizei *count,
                                  GLenum type, const GLvoid **indices,
                                  GLsizei primcount, CRClientState *c, const GLfloat *pZva )
{
   GLint i;
   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         crPackExpandDrawElements(mode, count[i], type, indices[i], c, pZva);
      }
   }
}
#endif /* CR_EXT_multi_draw_arrays */

static int crPack_GetNumEnabledArrays(CRClientState *c, int *size)
{
    int i, count=0;

    *size = 0;

    if (c->array.v.enabled)
    {
        count++;
        *size += c->array.v.bytesPerIndex;
    }

    if (c->array.c.enabled)
    {
        count++;
        *size += c->array.c.bytesPerIndex;
    }

    if (c->array.f.enabled)
    {
        count++;
        *size += c->array.f.bytesPerIndex;
    }

    if (c->array.s.enabled)
    {
        count++;
        *size += c->array.s.bytesPerIndex;
    }

    if (c->array.e.enabled)
    {
        count++;
        *size += c->array.e.bytesPerIndex;
    }

    if (c->array.i.enabled)
    {
        count++;
        *size += c->array.i.bytesPerIndex;
    }

    if (c->array.n.enabled)
    {
        count++;
        *size += c->array.n.bytesPerIndex;
    }

    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        if (c->array.t[i].enabled)
        {
            count++;
            *size += c->array.t[i].bytesPerIndex;
        }
    }

    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++)
    {
        if (c->array.a[i].enabled)
        {
            count++;
            *size += c->array.a[i].bytesPerIndex;
        }
    }

    return count;
}

static void crPackLockClientPointer(GLint first, GLint count, unsigned char **ppData, int index, CRClientState *c)
{
    CRClientPointer *cp;
    unsigned char *data_ptr = *ppData, *cptr;
    GLint i;

    cp = crStateGetClientPointerByIndex(index, &c->array);

    if (cp->enabled)
    {
        if (cp->buffer && cp->buffer->id)
        {
            crWarning("crPackLockClientPointer called when there's VBO enabled!");
        }

        WRITE_DATA_AI(int, index);
        cptr = cp->p + first*cp->stride;
        if (cp->bytesPerIndex==cp->stride)
        {
            crMemcpy(data_ptr, cptr, count*cp->bytesPerIndex);
            data_ptr += count*cp->bytesPerIndex;
        }
        else
        {
            for (i=0; i<count; ++i)
            {
                crMemcpy(data_ptr, cptr, cp->bytesPerIndex);
                data_ptr += cp->bytesPerIndex;
                cptr += cp->stride;
            }
        }
        *ppData = data_ptr;
    }
}

void PACK_APIENTRY crPackLockArraysEXT(GLint first, GLint count)
{
    CRContext *g = crStateGetCurrent(g_pStateTracker);
    CRClientState *c = &g->client;
    unsigned char *data_ptr, *start_ptr;
    int packet_length = sizeof(int); /*extopcode*/
    int vertex_size, i, numenabled;

    packet_length += sizeof(first) + sizeof(count); /*params*/
    numenabled = crPack_GetNumEnabledArrays(c, &vertex_size);
    packet_length += sizeof(int) + numenabled*sizeof(int); /*numenabled + indices*/
    packet_length += vertex_size * count; /*vertices data*/

    start_ptr = data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA_AI(GLenum, CR_LOCKARRAYSEXT_EXTEND_OPCODE );
    WRITE_DATA_AI(GLint, first);
    WRITE_DATA_AI(GLint, count);
    WRITE_DATA_AI(int, numenabled);
    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        crPackLockClientPointer(first, count, &data_ptr, i, c);
    }
    crHugePacket(CR_EXTEND_OPCODE, start_ptr);
    crPackFree(start_ptr);
}
