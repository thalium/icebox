# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


from __future__ import print_function
import sys

import apiutil

apiutil.CopyrightC()


print("""#include <stdio.h>
#include "cr_error.h"
#include "cr_spu.h"
#include "state/cr_statetypes.h"

#if defined(WINDOWS)
#define ERROR_APIENTRY __stdcall
#else
#define ERROR_APIENTRY
#endif

#define ERROR_UNUSED(x) ((void)x)""")


keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

for func_name in keys:
	return_type = apiutil.ReturnType(func_name)
	params = apiutil.Parameters(func_name)
	print('\nstatic %s ERROR_APIENTRY error%s(%s)' % (return_type, func_name, apiutil.MakeDeclarationString(params )))
	print('{')
	# Handle the void parameter list
	for (name, type, vecSize) in params:
		print('\tERROR_UNUSED(%s);' % name)
	print('\tcrError("ERROR SPU: Unsupported function gl%s called!");' % func_name)
	if return_type != "void":
		print('\treturn (%s)0;' % return_type)
	print('}')

print('SPUNamedFunctionTable _cr_error_table[] = {')
for index in range(len(keys)):
	func_name = keys[index]
	print('\t{ "%s", (SPUGenericFunction) error%s },' % (func_name, func_name ))
print('\t{ NULL, NULL }')
print('};')
