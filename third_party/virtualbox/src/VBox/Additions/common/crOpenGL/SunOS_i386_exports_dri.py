"""
Copyright (C) 2009-2017 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
"""

from __future__ import print_function
import sys

import apiutil


def GenerateEntrypoints():

    #apiutil.CopyrightC()

    # Get sorted list of dispatched functions.
    # The order is very important - it must match cr_opcodes.h
    # and spu_dispatch_table.h
    print('%include "iprt/asmdefs.mac"')
    print("")
    print("%ifdef RT_ARCH_AMD64")
    print("extern glim")
    print("%else ; X86")
    print("extern glim")
    print("%endif")
    print("")

    keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

    for index in range(len(keys)):
        func_name = keys[index]
        if apiutil.Category(func_name) == "Chromium":
            continue
        if apiutil.Category(func_name) == "VBox":
            continue

        print("BEGINPROC_EXPORTED cr_gl%s" % func_name)
        print("%ifdef RT_ARCH_AMD64")
        print("\tjmp \t[glim+%d wrt rip wrt ..gotpcrel]" % (8*index))
        print("%else ; X86")
        print("\tjmp \t[glim+%d wrt ..gotpc]" % (4*index))
        print("%endif")
        print("ENDPROC cr_gl%s" % func_name)
        print("")


    print(';')
    print('; Aliases')
    print(';')

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
            # this dict lookup should never fail (raise an exception)!
            index = keys.index(alias)
            print("BEGINPROC_EXPORTED cr_gl%s" % func_name)
            print("%ifdef RT_ARCH_AMD64")
            print("\tjmp \t[glim+%d wrt rip wrt ..gotpcrel]" % (8*index))
            print("%else ; X86")
            print("\tjmp \t[glim+%d wrt ..gotpc]" % (4*index))
            print("%endif")
            print("ENDPROC cr_gl%s" % func_name)
            print("")


    print(';')
    print('; No-op stubs')
    print(';')

    # Now generate no-op stub functions
    for func_name in allkeys:
        if "stub" in apiutil.ChromiumProps(func_name):
            print("BEGINPROC_EXPORTED cr_gl%s" % func_name)
            print("\tleave")
            print("\tret")
            print("ENDPROC cr_gl%s" % func_name)
            print("")


GenerateEntrypoints()

