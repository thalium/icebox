# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


from __future__ import print_function
import sys

import apiutil


def GenerateEntrypoints():

	apiutil.CopyrightC()

	print('#include "chromium.h"')
	print('#include "stub.h"')
	print('')
	print('#define NAKED __declspec(naked)')
	print('#define UNUSED(x) ((void)(x))')
	print('')


	# Get sorted list of dispatched functions.
	# The order is very important - it must match cr_opcodes.h
	# and spu_dispatch_table.h
	keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

	for index in range(len(keys)):
		func_name = keys[index]
		if apiutil.Category(func_name) == "Chromium":
			continue
		if apiutil.Category(func_name) == "VBox":
			continue

		return_type = apiutil.ReturnType(func_name)
		params = apiutil.Parameters(func_name)

		print("NAKED %s cr_gl%s(%s)" % (return_type, func_name,
									  apiutil.MakeDeclarationString( params )))
		print("{")
		print("\t__asm jmp [glim.%s]" % func_name)
		for (name, type, vecSize) in params:
			print("\tUNUSED(%s);" % name)
		print("}")
		print("")

	print('/*')
	print('* Aliases')
	print('*/')

	# Now loop over all the functions and take care of any aliases
	allkeys = apiutil.GetAllFunctions(sys.argv[1]+"/APIspec.txt")
	for func_name in allkeys:
		if "omit" in apiutil.ChromiumProps(func_name):
			continue

		if func_name in keys:
			# we already processed this function earlier
			continue

		# alias is the function we're aliasing
		alias = apiutil.Alias(func_name)
		if alias:
			return_type = apiutil.ReturnType(func_name)
			params = apiutil.Parameters(func_name)
			print("NAKED %s cr_gl%s(%s)" % (return_type, func_name,
								apiutil.MakeDeclarationString( params )))
			print("{")
			print("\t__asm jmp [glim.%s]" % alias)
			for (name, type, vecSize) in params:
				print("\tUNUSED(%s);" % name)
			print("}")
			print("")


	print('/*')
	print('* No-op stubs')
	print('*/')

	# Now generate no-op stub functions
	for func_name in allkeys:
		if "stub" in apiutil.ChromiumProps(func_name):
			return_type = apiutil.ReturnType(func_name)
			params = apiutil.Parameters(func_name)
			print("NAKED %s cr_gl%s(%s)" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
			print("{")
			if return_type != "void":
				print("return (%s) 0" % return_type)
			print("}")
			print("")




GenerateEntrypoints()

