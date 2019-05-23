# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys
curver = sys.version_info[0] + sys.version_info[1]/10.0
if curver < 2.2:
	print("Your python is version %g.  Chromium requires at least"%(curver), file=sys.stderr)
	print("version 2.2.  Please upgrade your python installation.", file=sys.stderr)
	sys.exit(1)

import string;
import re;

def CopyrightC( ):
	print("""/* Copyright (c) 2001, Stanford University
	All rights reserved.

	See the file LICENSE.txt for information on redistributing this software. */
	""")

def CopyrightDef( ):
	print("""; Copyright (c) 2001, Stanford University
	; All rights reserved.
	;
	; See the file LICENSE.txt for information on redistributing this software.
	""")

def DecoderName( glName ):
	return "crUnpack" + glName

def OpcodeName( glName ):
	# This is a bit of a hack.  We want to implement the glVertexAttrib*NV
	# functions in terms of the glVertexAttrib*ARB opcodes.
	m = re.search( "VertexAttrib([1234](ub|b|us|s|ui|i|f|d|)v?)NV", glName )
	if m:
		dataType = m.group(1)
		if dataType == "4ub":
			dataType = "4Nub"
		elif dataType == "4ubv":
			dataType = "4Nubv"
		glName = "VertexAttrib" + dataType + "ARB"
	return "CR_" + string.upper( glName ) + "_OPCODE"

def ExtendedOpcodeName( glName ):
	return "CR_" + string.upper( glName ) + "_EXTEND_OPCODE"

def PackFunction( glName ):
	return "crPack" + glName

def DoPackFunctionMapping( glName ):
	return "__glpack_" + glName

def DoStateFunctionMapping( glName ):
	return "__glstate_" + glName

def DoImmediateMapping( glName ):
	return "__glim_" + glName



# Annotations are a generalization of Specials (below).
# Each line of an annotation file is a set of words.
# The first word is a key; the remainder are all annotations
# for that key.  This is a useful model for grouping keys
# (like GL function names) into overlapping subsets, all in
# a single file.
annotations = {}
def LoadAnnotations(filename):
	table = {}
	try:
		f = open(filename, "r")
	except:
		annotations[filename] = {}
		return {}

	for line in f.readlines():
		line = line.strip()
		if line == "" or line[0] == '#':
			continue
		subtable = {}
		words = line.split()
		for word in words[1:]:
			subtable[word] = 1
		table[words[0]] = subtable

	annotations[filename] = table
	return table

def GetAnnotations( filename, key ):
	table = {}
	try:
		table = annotations[filename]
	except KeyError:
		table = LoadAnnotations(filename)

	try:
		subtable = table[key]
	except KeyError:
		return []

	return sorted(subtable.keys())

def FindAnnotation( filename, key, subkey ):
	table = {}
	try:
		table = annotations[filename]
	except KeyError:
		table = LoadAnnotations(filename)

	try:
	    	subtable = table[key]
	except KeyError:
		return 0

	try:
		return subtable[subkey]
	except KeyError:
		return 0



specials = {}

def LoadSpecials( filename ):
	table = {}
	try:
		f = open( filename, "r" )
	except:
		specials[filename] = {}
		return {}

	for line in f.readlines():
		line = string.strip(line)
		if line == "" or line[0] == '#':
			continue
		table[line] = 1

	specials[filename] = table
	return table

def FindSpecial( table_file, glName ):
	table = {}
	filename = table_file + "_special"
	try:
		table = specials[filename]
	except KeyError:
		table = LoadSpecials( filename )

	try:
		if (table[glName] == 1):
			return 1
		else:
			return 0 #should never happen
	except KeyError:
		return 0

def AllSpecials( table_file ):
	table = {}
	filename = table_file + "_special"
	try:
		table = specials[filename]
	except KeyError:
		table = LoadSpecials( filename )

	return sorted(table.keys())

def AllSpecials( table_file ):
	filename = table_file + "_special"

	table = {}
	try:
		table = specials[filename]
	except KeyError:
		table = LoadSpecials(filename)

	return sorted(table.keys())

def NumSpecials( table_file ):
	filename = table_file + "_special"

	table = {}
	try:
		table = specials[filename]
	except KeyError:
		table = LoadSpecials(filename)

	return len(table.keys())

lengths = {}
lengths['GLbyte'] = 1
lengths['GLubyte'] = 1
lengths['GLshort'] = 2
lengths['GLushort'] = 2
lengths['GLint'] = 4
lengths['GLuint'] = 4
lengths['GLfloat'] = 4
lengths['GLclampf'] = 4
lengths['GLdouble'] = 8
lengths['GLclampd'] = 8

lengths['GLenum'] = 4
lengths['GLboolean'] = 1
lengths['GLsizei'] = 4
lengths['GLbitfield'] = 4

lengths['void'] = 0
lengths['int'] = 4

align_types = 1

def FixAlignment( pos, alignment ):
	# if we want double-alignment take word-alignment instead,
	# yes, this is super-lame, but we know what we are doing
	if alignment > 4:
		alignment = 4
	if align_types and alignment and ( pos % alignment ):
		pos += alignment - ( pos % alignment )
	return pos

def WordAlign( pos ):
	return FixAlignment( pos, 4 )

def PointerSize():
	return 8 # Leave room for a 64 bit pointer

def PacketLength( arg_types ):
	len = 0
	for arg in arg_types:
		if string.find( arg, '*') != -1:
			size = PointerSize()
		else:
			temp_arg = re.sub("const ", "", arg)
			size = lengths[temp_arg]
		len = FixAlignment( len, size ) + size
	len = WordAlign( len )
	return len

def InternalArgumentString( arg_names, arg_types ):
	"""Return string of C-style function declaration arguments."""
	output = ''
	for index in range(0,len(arg_names)):
		if len(arg_names) != 1 and arg_names[index] == '':
			continue
		output += arg_types[index]
		if arg_types[index][-1:] != '*' and arg_names[index] != '':
			output += " ";
		output += arg_names[index]
		if index != len(arg_names) - 1:
			output += ", "
	return output

def ArgumentString( arg_names, arg_types ):
	"""Return InternalArgumentString inside parenthesis."""
	output = '( ' + InternalArgumentString(arg_names, arg_types) + ' )'
	return output

def InternalCallString( arg_names ):
	output = ''
	for index in range(0,len(arg_names)):
		output += arg_names[index]
		if arg_names[index] != '' and index != len(arg_names) - 1:
			output += ", "
	return output

def CallString( arg_names ):
	output = '( '
	output += InternalCallString(arg_names)
	output += " )"
	return output

def IsVector ( func_name ) :
	m = re.search( r"^(SecondaryColor|Color|EdgeFlag|EvalCoord|Index|Normal|TexCoord|MultiTexCoord|Vertex|RasterPos|VertexAttrib|FogCoord|WindowPos|ProgramParameter)([1234]?)N?(ub|b|us|s|ui|i|f|d|)v(ARB|EXT|NV)?$", func_name )
	if m :
		if m.group(2) :
			return string.atoi( m.group(2) )
		else:
			return 1
	else:
		return 0
