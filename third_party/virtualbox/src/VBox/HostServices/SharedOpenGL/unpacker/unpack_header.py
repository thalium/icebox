# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys;
import pickle;
import types;
import string;
import re;

sys.path.append( "../opengl_stub" )

import stub_common;

parsed_file = open( "../glapi_parser/gl_header.parsed", "rb" )
gl_mapping = pickle.load( parsed_file )

stub_common.CopyrightC()

print("""#ifndef CR_UNPACKFUNCTIONS_H
#define CR_UNPACKFUNCTIONS_H

#include "cr_unpack.h"
""")

for func_name in sorted(gl_mapping.keys()):
	( return_type, arg_names, arg_types ) = gl_mapping[func_name]
	print('void crUnpack%s(PCrUnpackerState pState);' %( func_name ))
print('void crUnpackExtend(PCrUnpackerState pState);')
print('\n#endif /* CR_UNPACKFUNCTIONS_H */')
