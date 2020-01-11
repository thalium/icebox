# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys

import apiutil


apiutil.CopyrightC()

print("""
#include "cr_server.h"
#include "feedbackspu.h"
#include "feedbackspu_proto.h"
""")
custom = ["CreateContext", "VBoxCreateContext", "MakeCurrent", "DestroyContext"]

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

for func_name in keys:
    if apiutil.FindSpecial( "feedback_state", func_name ):
        if func_name in custom:
            continue
        return_type = apiutil.ReturnType(func_name)
        params = apiutil.Parameters(func_name)
        print('%s FEEDBACKSPU_APIENTRY feedbackspu_%s(%s)' % (return_type, func_name, apiutil.MakeDeclarationString(params)))
        print('{')
        if len(params) == 0:
            print('\tcrState%s(&feedback_spu.StateTracker);' % (func_name,))
        else:
            print('\tcrState%s(&feedback_spu.StateTracker, %s);' % (func_name, apiutil.MakeCallString(params)))
        print('')
        print('\tfeedback_spu.super.%s(%s);' % (func_name, apiutil.MakeCallString(params)))
        print('}')
