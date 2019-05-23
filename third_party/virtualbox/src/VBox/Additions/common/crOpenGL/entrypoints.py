# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


"""
This module generates C entrypoints for all the OpenGL functions
and the special Chromium meta/glue functions.
"""


from __future__ import print_function
import sys

import apiutil


def GenerateEntrypoints(hacks = []):
    """Emit code for all the OpenGL/Chromium entrypoints.
    hacks is an optional list of functions which are special cased.
    """

    apiutil.CopyrightC()

    print('#define GL_GLEXT_PROTOTYPES')
    print('#include <stdio.h>')
    print('#include <stdlib.h>')
    print('#include <GL/gl.h>')
    print('#include "chromium.h"')
    print('#include "stub.h"')
    print('#include "dri_glx.h"')
    print('')
    print('#ifdef __GNUC__')
    print('# if (__GNUC__ << 16) + __GNUC_MINOR__ >= 0x40002')
    print('#  pragma GCC diagnostic ignored "-Wunused-parameter"')
    print('# endif')
    print('#endif')


    # Get sorted list of dispatched functions.
    # The order is very important - it must match cr_opcodes.h
    # and spu_dispatch_table.h
    keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

    for index in range(len(keys)):
        func_name = keys[index]
        if apiutil.Category(func_name) == "Chromium":
            # this function is defined in stub.c
            continue

        return_type = apiutil.ReturnType(func_name)
        params = apiutil.Parameters(func_name)

        if func_name in hacks:
            print("/* hacked entrypoint: %s */" % func_name)
            if func_name == "TexImage3D":
                # Pretty common: internalformat is GLenum, not GLint
                print("void glTexImage3D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels )")
                print("{")
                print("\tglim.TexImage3D( target, level, (GLint) internalformat, width, height, depth, border, format, type, pixels );")
                print("}")
            elif func_name == "TexImage2D":
                # Pretty common: internalformat is GLenum, not GLint
                print("void glTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels )")
                print("{")
                print("\tglim.TexImage2D( target, level, (GLint) internalformat, width, height, border, format, type, pixels );")
                print("}")
            elif func_name == "TexImage1D":
                # Pretty common: internalformat is GLenum, not GLint
                print("void glTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels )")
                print("{")
                print("\tglim.TexImage1D( target, level, (GLint) internalformat, width, border, format, type, pixels );")
                print("}")
            elif func_name == "EdgeFlagPointer":
                # second arg is GLboolean instead of GLvoid
                print("void glEdgeFlagPointer( GLsizei stride, const GLboolean *pointer )")
                print("{")
                print("\tglim.EdgeFlagPointer( stride, pointer );")
                print("}")
            elif func_name == "ProgramParameters4fvNV":
                print("void glProgramParameters4fvNV( GLenum target, GLuint index, GLuint num, const GLfloat *params )")
                print("{")
                print("\tglim.ProgramParameters4fvNV( target, index, num, params );")
                print("}")
            elif func_name == "MultiDrawElementsEXT":
                print("void glMultiDrawElementsEXT(GLenum mode, GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount)")
                print("{")
                print("\tglim.MultiDrawElementsEXT(mode, count,type, indices, primcount);")
                print("}")
            elif func_name == "ProgramParameters4dvNV":
                print("void glProgramParameters4dvNV( GLenum target, GLuint index, GLuint num, const GLdouble *params )")
                print("{")
                print("\tglim.ProgramParameters4dvNV( target, index, num, params );")
                print("}")
        else:
            # the usual path
            print("%s VBOXGLTAG(gl%s)(%s);" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
            print("")
            print("%s VBOXGLTAG(gl%s)(%s)" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
            print("{")
            print("\t", end="")
            if return_type != "void":
                print("return ", end=" ")
            print("glim.%s(%s);" % (func_name, apiutil.MakeCallString(params)))
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
            if func_name in hacks:
                print("/* hacked entrypoint: %s */" % func_name)
                if func_name == "MultiDrawArrays":
                    print("void glMultiDrawArrays( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount )")
                    print("{")
                    print("\tglim.MultiDrawArraysEXT( mode, (GLint*)first, (GLsizei*)count, primcount );")
                    print("}")
                elif func_name == "BufferData":
                    print("void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)")
                    print("{")
                    print("\tglim.BufferDataARB(target, size, data, usage);")
                    print("}")
                elif func_name == "BufferSubData":
                    print("void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)")
                    print("{")
                    print("\tglim.BufferSubDataARB(target, offset, size, data);")
                    print("}")
                elif func_name == "GetBufferSubData":
                    print("void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data)")
                    print("{")
                    print("\tglim.GetBufferSubDataARB(target, offset, size, data);")
                    print("}")
            else:
                return_type = apiutil.ReturnType(func_name)
                params = apiutil.Parameters(func_name)
                print("%s VBOXGLTAG(gl%s)(%s);" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
                print("")
                print("%s VBOXGLTAG(gl%s)(%s)" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
                print("{")
                print("\t", end="")
                if return_type != "void":
                    print("return ", end=" ")
                print("glim.%s(%s);" % (alias, apiutil.MakeCallString(params)))
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

            print("%s VBOXGLTAG(gl%s)(%s);" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
            print("")
            print("%s VBOXGLTAG(gl%s)(%s)" % (return_type, func_name, apiutil.MakeDeclarationString(params)))
            print("{")
            if return_type != "void":
                print("return (%s) 0" % return_type)
            print("}")
            print("")

