# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys;
import string;
import re;

import apiutil

apiutil.CopyrightC()

print("""
#include "cr_debugopcodes.h"
#include <stdio.h>
""")

print("""void crDebugOpcodes( FILE *fp, unsigned char *ptr, unsigned int num_opcodes )
{
\tunsigned int i;
\tfor (i = 0; i < num_opcodes; i++)
\t{
\t\tswitch(*(ptr--))
\t\t{
""")

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")
keys.sort()

for func_name in keys:
	if "pack" in apiutil.ChromiumProps(func_name):
		print('\t\tcase %s:' % apiutil.OpcodeName( func_name ))
		print('\t\t\tfprintf(fp, "%s\\n"); ' % apiutil.OpcodeName( func_name ))
		print( '\t\t\tbreak;')

print("""
\t\t}
\t}
}
""")
