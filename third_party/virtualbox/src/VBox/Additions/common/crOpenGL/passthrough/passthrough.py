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
#include "cr_string.h"
#include "cr_spu.h"
#include "passthroughspu.h"
""")

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")


print('SPUNamedFunctionTable _cr_passthrough_table[%d];' % ( len(keys) + 1 ))

print("""
static void __fillin(int offset, char *name, SPUGenericFunction func)
{
	_cr_passthrough_table[offset].name = crStrdup(name);
	_cr_passthrough_table[offset].fn = func;
}

void BuildPassthroughTable( SPU *child )
{""")

for index in range(len(keys)):
	func_name = keys[index]
	print('\t__fillin(%3d, "%s", (SPUGenericFunction) child->dispatch_table.%s);' % (index, func_name, func_name ))
print('\t__fillin(%3d, NULL, NULL);' % len(keys))
print('}')
