# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

# This script generates the packer.c file from the gl_header.parsed file.

from __future__ import print_function
import sys, string, re

import apiutil



def WriteData( offset, arg_type, arg_name, is_swapped ):
    """Return a string to write a variable to the packing buffer."""
    retval = 9
    if apiutil.IsPointer(arg_type):
        retval = "\tWRITE_NETWORK_POINTER(%d, (void *) %s);" % (offset, arg_name )
    else:   
        if is_swapped:
            if arg_type == "GLfloat" or arg_type == "GLclampf":
                retval = "\tWRITE_DATA(%d, GLuint, SWAPFLOAT(%s));" % (offset, arg_name)
            elif arg_type == "GLdouble" or arg_type == "GLclampd":
                retval = "\tWRITE_SWAPPED_DOUBLE(%d, %s);" % (offset, arg_name)
            elif apiutil.sizeof(arg_type) == 1:
                retval = "\tWRITE_DATA(%d, %s, %s);" % (offset, arg_type, arg_name)
            elif apiutil.sizeof(arg_type) == 2:
                retval = "\tWRITE_DATA(%d, %s, SWAP16(%s));" % (offset, arg_type, arg_name)
            elif apiutil.sizeof(arg_type) == 4:
                retval = "\tWRITE_DATA(%d, %s, SWAP32(%s));" % (offset, arg_type, arg_name)
        else:
            if arg_type == "GLdouble" or arg_type == "GLclampd":
                retval = "\tWRITE_DOUBLE(%d, %s);" % (offset, arg_name)
            else:
                retval = "\tWRITE_DATA(%d, %s, %s);" % (offset, arg_type, arg_name)
    if retval == 9:
        print >>sys.stderr, "no retval for %s %s" % (arg_name, arg_type)
        assert 0
    return retval


def UpdateCurrentPointer( func_name ):
    m = re.search( r"^(Color|Normal)([1234])(ub|b|us|s|ui|i|f|d)$", func_name )
    if m :
        k = m.group(1)
        name = '%s%s' % (k[:1].lower(),k[1:])
        type = m.group(3) + m.group(2)
        print("\tpc->current.c.%s.%s = data_ptr;" % (name,type))
        return

    m = re.search( r"^(SecondaryColor)(3)(ub|b|us|s|ui|i|f|d)EXT$", func_name )
    if m :
        k = m.group(1)
        name = 'secondaryColor'
        type = m.group(3) + m.group(2)
        print("\tpc->current.c.%s.%s = data_ptr;" % (name,type))
        return

    m = re.search( r"^(TexCoord)([1234])(ub|b|us|s|ui|i|f|d)$", func_name )
    if m :
        k = m.group(1)
        name = 'texCoord'
        type = m.group(3) + m.group(2)
        print("\tpc->current.c.%s.%s[0] = data_ptr;" % (name,type))
        return

    m = re.search( r"^(MultiTexCoord)([1234])(ub|b|us|s|ui|i|f|d)ARB$", func_name )
    if m :
        k = m.group(1)
        name = 'texCoord'
        type = m.group(3) + m.group(2)
        print("\tpc->current.c.%s.%s[texture-GL_TEXTURE0_ARB] = data_ptr + 4;" % (name,type))
        return

    m = re.match( r"^(Index)(ub|b|us|s|ui|i|f|d)$", func_name )
    if m :
        k = m.group(1)
        name = 'index'
        type = m.group(2) + "1"
        print("\tpc->current.c.%s.%s = data_ptr;" % (name,type))
        return

    m = re.match( r"^(EdgeFlag)$", func_name )
    if m :
        k = m.group(1)
        name = 'edgeFlag'
        type = "l1"
        print("\tpc->current.c.%s.%s = data_ptr;" % (name,type))
        return

    m = re.match( r"^(FogCoord)(f|d)EXT$", func_name )
    if m :
        k = m.group(1)
        name = 'fogCoord'
        type = m.group(2) + "1"
        print("\tpc->current.c.%s.%s = data_ptr;" % (name,type))
        return


    m = re.search( r"^(VertexAttrib)([1234])N?(ub|b|s|f|d)(NV|ARB)$", func_name )
    if m :
        k = m.group(1)
        name = 'vertexAttrib'
        type = m.group(3) + m.group(2)
        # Add 12 to skip the packet length, opcode and index fields
        print("\tpc->current.c.%s.%s[index] = data_ptr + 4;" % (name,type))
        if m.group(4) == "ARB" or m.group(4) == "NV":
            print("\tpc->current.attribsUsedMask |= (1 << index);")
            print("\tpc->current.changedVertexAttrib |= (1 << index);")
        return



def PrintFunc( func_name, params, is_swapped, can_have_pointers ):
    """Emit a packer function."""
    if is_swapped:
        print('void PACK_APIENTRY crPack%sSWAP(%s)' % (func_name, apiutil.MakeDeclarationStringWithContext('CR_PACKER_CONTEXT', params)))
    else:
        print('void PACK_APIENTRY crPack%s(%s)' % (func_name, apiutil.MakeDeclarationStringWithContext('CR_PACKER_CONTEXT', params)))
    print('{')
    print('\tCR_GET_PACKER_CONTEXT(pc);')

    # Save original function name
    orig_func_name = func_name

    # Convert to a non-vector version of the function if possible
    func_name = apiutil.NonVectorFunction( func_name )
    if not func_name:
        func_name = orig_func_name

    # Check if there are any pointer parameters.
    # That's usually a problem so we'll emit an error function.
    nonVecParams = apiutil.Parameters(func_name)
    bail_out = 0
    for (name, type, vecSize) in nonVecParams:
        if apiutil.IsPointer(type) and vecSize == 0 and not can_have_pointers:
            bail_out = 1
    if bail_out:
        for (name, type, vecSize) in nonVecParams:
            print('\t(void)%s;' % (name))
        print('\tcrError ( "%s needs to be special cased %d %d!");' % (func_name, vecSize, can_have_pointers))
        print('\t(void) pc;')
        print('}')
        # XXX we should really abort here
        return

    if "extpack" in apiutil.ChromiumProps(func_name):
        is_extended = 1
    else:
        is_extended = 0


    print("\tunsigned char *data_ptr = NULL;")
    print('\t(void) pc;')
    #if func_name == "Enable" or func_name == "Disable":
    #   print "\tCRASSERT(!pc->buffer.geometry_only); /* sanity check */"

    for index in range(0,len(params)):
        (name, type, vecSize) = params[index]
        if vecSize>0 and func_name!=orig_func_name:
            print("    if (!%s) {" % name)
            # Know the reason for this one, so avoid the spam.
            if orig_func_name != "SecondaryColor3fvEXT":
                print("        crDebug(\"App passed NULL as %s for %s\");" % (name, orig_func_name))
            print("        return;")
            print("    }")

    packet_length = apiutil.PacketLength(nonVecParams)

    if packet_length == 0 and not is_extended:
        print("\tCR_GET_BUFFERED_POINTER_NO_ARGS(pc);")
    elif func_name[:9] == "Translate" or func_name[:5] == "Color":
        # XXX WTF is the purpose of this?
        if is_extended:
            packet_length += 8
        print("\tCR_GET_BUFFERED_POINTER_NO_BEGINEND_FLUSH(pc, %d, GL_TRUE);" % packet_length)
    else:
        if is_extended:
            packet_length += 8
        print("\tCR_GET_BUFFERED_POINTER(pc, %d);" % packet_length)
    UpdateCurrentPointer( func_name )

    if is_extended:
        counter = 8
        print(WriteData( 0, 'GLint', packet_length, is_swapped ))
        print(WriteData( 4, 'GLenum', apiutil.ExtendedOpcodeName( func_name ), is_swapped ))
    else:
        counter = 0

    # Now emit the WRITE_() macros for all parameters
    for index in range(0,len(params)):
        (name, type, vecSize) = params[index]
        # if we're converting a vector-valued function to a non-vector func:
        if vecSize > 0 and func_name != orig_func_name:
            ptrType = apiutil.PointerType(type)
            for i in range(0, vecSize):
                print(WriteData( counter + i * apiutil.sizeof(ptrType),
                                 ptrType, "%s[%d]" % (name, i), is_swapped ))
            # XXX increment counter here?
        else:
            print(WriteData( counter, type, name, is_swapped ))
            if apiutil.IsPointer(type):
                counter += apiutil.PointerSize()
            else:
                counter += apiutil.sizeof(type)

    # finish up
    if is_extended:
        print("\tWRITE_OPCODE(pc, CR_EXTEND_OPCODE);")
    else:
        print("\tWRITE_OPCODE(pc, %s);" % apiutil.OpcodeName( func_name ))

    if "get" in apiutil.Properties(func_name):
        print('\tCR_CMDBLOCK_CHECK_FLUSH(pc);')

    print('\tCR_UNLOCK_PACKER_CONTEXT(pc);')
    print('}\n')


r0_funcs = [ 'ChromiumParameteriCR', 'WindowSize', 'WindowShow', 'WindowPosition' ]


apiutil.CopyrightC()

print("""
/* DO NOT EDIT - THIS FILE GENERATED BY THE packer.py SCRIPT */

/* For each of the OpenGL functions we have a packer function which
 * packs the function's opcode and arguments into a buffer.
 */

#include "packer.h"
#include "cr_opcodes.h"

""")


keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

for func_name in keys:
    if apiutil.FindSpecial( "packer", func_name ):
        continue

    if not apiutil.HasPackOpcode(func_name):
        continue

    pointers_ok = 0

    return_type = apiutil.ReturnType(func_name)
    params = apiutil.Parameters(func_name)

    if return_type != 'void':
        # Yet another gross hack for glGetString
        if return_type.find('*') == -1:
            return_type = return_type + " *"
        params.append(("return_value", return_type, 0))

    if "get" in apiutil.Properties(func_name):
        pointers_ok = 1
        params.append(("writeback", "int *", 0))

    if func_name == 'Writeback':
        pointers_ok = 1

    if not func_name in r0_funcs:
        print('#ifndef IN_RING0')
        
    PrintFunc( func_name, params, 0, pointers_ok )
    PrintFunc( func_name, params, 1, pointers_ok )
    
    if not func_name in r0_funcs:
        print('#endif')
    
