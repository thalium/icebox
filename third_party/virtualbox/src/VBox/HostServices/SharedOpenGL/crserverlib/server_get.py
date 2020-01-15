# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys

import apiutil


apiutil.CopyrightC()

print("""
#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"
""")

max_components = {
    'GetClipPlane': 4,
    'GetCombinerStageParameterfvNV': 4,
    'GetCombinerStageParameterivNV': 4,
    'GetCombinerOutputParameterfvNV': 4,
    'GetCombinerOutputParameterivNV': 4,
    'GetCombinerInputParameterfvNV': 4,
    'GetCombinerInputParameterivNV': 4,
    'GetFinalCombinerInputParameterfvNV': 4,
    'GetFinalCombinerInputParameterivNV': 4,
    'GetLightfv': 4,
    'GetLightiv': 4,
    'GetMaterialfv': 4, 
    'GetMaterialiv': 4, 
    'GetPolygonStipple': 32*32/8,
    'GetTexEnvfv': 4,
    'GetTexEnviv': 4,
    'GetTexGendv': 4,
    'GetTexGenfv': 4,
    'GetTexGeniv': 4,
    'GetTexLevelParameterfv': 1,
    'GetTexLevelParameteriv': 1,
    'GetTexParameterfv': 4,
    'GetTexParameteriv': 4,
    'GetProgramParameterdvNV': 4,
    'GetProgramParameterfvNV': 4,
    'GetProgramivNV': 1,
    'GetTrackMatrixivNV': 24,
    'GetVertexAttribPointervNV': 1,
    'GetVertexAttribdvNV': 4,
    'GetVertexAttribfvNV': 4,
    'GetVertexAttribivNV': 4,
    'GetFenceivNV': 1,
    'GetVertexAttribdvARB': 4,
    'GetVertexAttribfvARB': 4,
    'GetVertexAttribivARB': 4,
    'GetVertexAttribPointervARB': 1,
    'GetProgramNamedParameterdvNV': 4, 
    'GetProgramNamedParameterfvNV': 4,
    'GetProgramLocalParameterdvARB': 4, 
    'GetProgramLocalParameterfvARB': 4, 
    'GetProgramEnvParameterdvARB': 4,
    'GetProgramEnvParameterfvARB': 4,
    'GetProgramivARB': 1,
    'AreProgramsResidentNV': 1,
    'GetBufferParameterivARB': 1,
    'GetBufferPointervARB': 1,
    'GetQueryObjectivARB' : 1,
    'GetQueryObjectuivARB' : 1,
    'GetQueryivARB' : 1,
    'GetProgramiv' : 1,
    'GetShaderiv' : 1,
    'GetObjectParameterfvARB': 1,
    'GetObjectParameterivARB': 1,
    'GetRenderbufferParameterivEXT': 1,
    'GetFramebufferAttachmentParameterivEXT': 1
}

no_pnames = [
    'GetClipPlane',
    'GetPolygonStipple',
    'GetProgramLocalParameterdvARB',
    'GetProgramLocalParameterfvARB',
    'GetProgramNamedParameterdvNV',
    'GetProgramNamedParameterfvNV',
    'GetProgramNamedParameterdvNV',
    'GetProgramNamedParameterfvNV',
    'GetProgramEnvParameterdvARB',
    'GetProgramEnvParameterfvARB',
    'GetProgramivARB',
    'AreProgramsResidentNV',
    'GetProgramiv',
    'GetShaderiv',
    'GetObjectParameterfvARB',
    'GetObjectParameterivARB',
    'GetRenderbufferParameterivEXT',
    'GetFramebufferAttachmentParameterivEXT'
];

convert_bufferid = [
    'GetVertexAttribdvARB',
    'GetVertexAttribdvNV',
    'GetVertexAttribfvARB',
    'GetVertexAttribfvNV',
    'GetVertexAttribivARB',
    'GetVertexAttribivNV'
];

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")
for func_name in keys:
    #(return_type, arg_names, arg_types) = gl_mapping[func_name]
    if ("get" in apiutil.Properties(func_name) and
        apiutil.ReturnType(func_name) == "void" and
        not apiutil.FindSpecial( "server", func_name )):

        params = apiutil.Parameters(func_name)

        print('void SERVER_DISPATCH_APIENTRY crServerDispatch%s(%s)' % (func_name, apiutil.MakeDeclarationString( params ) ))
        print('{')

        lastParam = params[-1]
        assert apiutil.IsPointer(lastParam[1])
        local_argtype = apiutil.PointerType(lastParam[1])
        local_argname = 'local_%s' % lastParam[0]

        if not func_name in no_pnames:
            print('\tunsigned int cComponents = 0;');
        print('\t%s %s[%d] = { 0 };' % ( local_argtype, local_argname, max_components[func_name] ))
        print('\t(void) %s;' % lastParam[0])

        params[-1] = (local_argname, local_argtype, 0)

        print('\tcr_server.head_spu->dispatch_table.%s(%s);' % ( func_name, apiutil.MakeCallString(params) ))

        if func_name in convert_bufferid:
            print('\tif (pname==GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB){')
            print('\t\tlocal_params[0]=(%s)crStateBufferHWIDtoID(&cr_server.StateTracker, (GLint)local_params[0]);' % (local_argtype))
            print('\t}')

        if func_name in no_pnames:
            print('\tcrServerReturnValue(&(%s[0]), %d*sizeof(%s));' % (local_argname, max_components[func_name], local_argtype ))
        else:
            print('\tcComponents = RT_MIN(crStateHlpComponentsCount(pname), RT_ELEMENTS(%s));' % local_argname)
            print('\tcrServerReturnValue(&(%s[0]), cComponents*sizeof(%s));' % (local_argname, local_argtype ))
        print ('}\n')
