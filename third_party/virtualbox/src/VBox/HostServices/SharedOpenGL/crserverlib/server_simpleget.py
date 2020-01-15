# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys

import apiutil


apiutil.CopyrightC()

print("""#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"
""")

from get_sizes import *;


funcs = [ 'GetIntegerv', 'GetFloatv', 'GetDoublev', 'GetBooleanv' ]
types = [ 'GLint', 'GLfloat', 'GLdouble', 'GLboolean' ]

for index in range(len(funcs)):
    func_name = funcs[index]
    params = apiutil.Parameters(func_name)
    print('void SERVER_DISPATCH_APIENTRY crServerDispatch%s(%s)' % ( func_name, apiutil.MakeDeclarationString(params)))
    print('{')
    print('\t%s *get_values;' % types[index])
    print('\tint tablesize;')
    print("""
    #ifdef CR_ARB_texture_compression
    if (GL_COMPRESSED_TEXTURE_FORMATS_ARB == pname)
    {
        GLint numtexfmts = 0;
        cr_server.head_spu->dispatch_table.GetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, &numtexfmts);
        tablesize = numtexfmts * sizeof(%s);
    }
    else
    #endif
    {
        tablesize = __numValues( pname ) * sizeof(%s);
    }
    """ % (types[index], types[index]))
    print('\t(void) params;')
    print('\tif (tablesize == 0)')
    print('\t{')
    print('\t\tcrServerReturnValue(NULL, 0);')
    print('\t\treturn;')
    print('\t}')
    print('\tget_values = (%s *) crAlloc( tablesize );' % types[index])
    print('\tcr_server.head_spu->dispatch_table.%s( pname, get_values );' % func_name)
    print("""
    if (GL_TEXTURE_BINDING_1D==pname
        || GL_TEXTURE_BINDING_2D==pname
        || GL_TEXTURE_BINDING_3D==pname
        || GL_TEXTURE_BINDING_RECTANGLE_ARB==pname
        || GL_TEXTURE_BINDING_CUBE_MAP_ARB==pname)
    {
        GLuint texid;
        CRASSERT(tablesize/sizeof(%s)==1);
        texid = (GLuint) *get_values;
        *get_values = (%s) crStateTextureHWIDtoID(&cr_server.StateTracker, texid);
    }
    else if (GL_CURRENT_PROGRAM==pname)
    {
        GLuint programid;
        CRASSERT(tablesize/sizeof(%s)==1);
        programid = (GLuint) *get_values;
        *get_values = (%s) crStateGLSLProgramHWIDtoID(&cr_server.StateTracker, programid);
    }
    else if (GL_FRAMEBUFFER_BINDING_EXT==pname
             ||GL_READ_FRAMEBUFFER_BINDING==pname)
    {
        GLuint fboid;
        CRASSERT(tablesize/sizeof(%s)==1);
        fboid = (GLuint) *get_values;
        if (crServerIsRedirectedToFBO()
            && (fboid==cr_server.curClient->currentMural->aidFBOs[0]
            || fboid==cr_server.curClient->currentMural->aidFBOs[1]))
        {
            fboid = 0;
        }
        else
        {
        	fboid = crStateFBOHWIDtoID(&cr_server.StateTracker, fboid);
        }
        *get_values = (%s) fboid;
    }
    else if (GL_READ_BUFFER==pname)
    {
        if (crServerIsRedirectedToFBO()
            && CR_SERVER_FBO_FOR_IDX(cr_server.curClient->currentMural, cr_server.curClient->currentMural->iCurReadBuffer)
            && !crStateGetCurrent(&cr_server.StateTracker)->framebufferobject.readFB)
        {
            *get_values = (%s) crStateGetCurrent(&cr_server.StateTracker)->buffer.readBuffer;
            Assert(crStateGetCurrent(&cr_server.StateTracker)->buffer.readBuffer == GL_BACK || crStateGetCurrent(&cr_server.StateTracker)->buffer.readBuffer == GL_FRONT);
        }
    }
    else if (GL_DRAW_BUFFER==pname)
    {
        if (crServerIsRedirectedToFBO()
            && CR_SERVER_FBO_FOR_IDX(cr_server.curClient->currentMural, cr_server.curClient->currentMural->iCurDrawBuffer)
            && !crStateGetCurrent(&cr_server.StateTracker)->framebufferobject.drawFB)
        {
            *get_values = (%s) crStateGetCurrent(&cr_server.StateTracker)->buffer.drawBuffer;
            Assert(crStateGetCurrent(&cr_server.StateTracker)->buffer.drawBuffer == GL_BACK || crStateGetCurrent(&cr_server.StateTracker)->buffer.drawBuffer == GL_FRONT);
        }
    }
    else if (GL_RENDERBUFFER_BINDING_EXT==pname)
    {
        GLuint rbid;
        CRASSERT(tablesize/sizeof(%s)==1);
        rbid = (GLuint) *get_values;
        *get_values = (%s) crStateRBOHWIDtoID(&cr_server.StateTracker, rbid);
    }
    else if (GL_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_VERTEX_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_NORMAL_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_COLOR_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_INDEX_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB==pname
             || GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB==pname)
    {
        GLuint bufid;
        CRASSERT(tablesize/sizeof(%s)==1);
        bufid = (GLuint) *get_values;
        *get_values = (%s) crStateBufferHWIDtoID(&cr_server.StateTracker, bufid);
    }
    else if (GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS==pname)
    {
    	if (CR_MAX_TEXTURE_UNITS < (GLuint)*get_values)
    	{
    		*get_values = (%s)CR_MAX_TEXTURE_UNITS;
    	} 
    }
    else if (GL_MAX_VERTEX_ATTRIBS_ARB==pname)
    {
        if (CR_MAX_VERTEX_ATTRIBS < (GLuint)*get_values)
        {
            *get_values = (%s)CR_MAX_VERTEX_ATTRIBS;
        } 
    }
    """ % (types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index], types[index]))
    print('\tcrServerReturnValue( get_values, tablesize );')
    print('\tcrFree(get_values);')
    print('}\n')
