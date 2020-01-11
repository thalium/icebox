#!/usr/common/bin/python

# apiutil.py
#
# This file defines a bunch of utility functions for OpenGL API code
# generation.

from __future__ import print_function
import sys, string, re


#======================================================================

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



#======================================================================

class APIFunction:
	"""Class to represent a GL API function (name, return type,
	parameters, etc)."""
	def __init__(self):
		self.name = ''
		self.returnType = ''
		self.category = ''
		self.offset = -1
		self.alias = ''
		self.vectoralias = ''
		self.params = []
		self.paramlist = []
		self.paramvec = []
		self.paramaction = []
		self.paramprop = []
		self.paramset = []
		self.props = []
		self.chromium = []
		self.chrelopcode = -1



def ProcessSpecFile(filename, userFunc):
	"""Open the named API spec file and call userFunc(record) for each record
	processed."""
	specFile = open(filename, "r")
	if not specFile:
		print("Error: couldn't open %s file!" % filename)
		sys.exit()

	record = APIFunction()

	for line in specFile.readlines():

		# split line into tokens
		tokens = line.split()

		if len(tokens) > 0 and line[0] != '#':

			if tokens[0] == 'name':
				if record.name != '':
					# process the function now
					userFunc(record)
					# reset the record
					record = APIFunction()

				record.name = tokens[1]

			elif tokens[0] == 'return':
				record.returnType = ' '.join(tokens[1:])
			
			elif tokens[0] == 'param':
				name = tokens[1]
				type = ' '.join(tokens[2:])
				vecSize = 0
				record.params.append((name, type, vecSize))

			elif tokens[0] == 'paramprop':
				name = tokens[1]
				str = tokens[2:]
				enums = []
				for i in range(len(str)):
					enums.append(str[i]) 
				record.paramprop.append((name, enums))

			elif tokens[0] == 'paramlist':
				name = tokens[1]
				str = tokens[2:]
				list = []
				for i in range(len(str)):
					list.append(str[i]) 
				record.paramlist.append((name,list))

			elif tokens[0] == 'paramvec':
				name = tokens[1]
				str = tokens[2:]
				vec = []
				for i in range(len(str)):
					vec.append(str[i]) 
				record.paramvec.append((name,vec))

			elif tokens[0] == 'paramset':
				line = tokens[1:]
				result = []
				for i in range(len(line)):
					tset = line[i]
					if tset == '[':
						nlist = []
					elif tset == ']':
						result.append(nlist)
						nlist = []
					else:
						nlist.append(tset)
				if result != []:
					record.paramset.append(result)

			elif tokens[0] == 'paramaction':
				name = tokens[1]
				str = tokens[2:]
				list = []
				for i in range(len(str)):
					list.append(str[i]) 
				record.paramaction.append((name,list))

			elif tokens[0] == 'category':
				record.category = tokens[1]

			elif tokens[0] == 'offset':
				if tokens[1] == '?':
					record.offset = -2
				else:
					record.offset = int(tokens[1])

			elif tokens[0] == 'alias':
				record.alias = tokens[1]

			elif tokens[0] == 'vectoralias':
				record.vectoralias = tokens[1]

			elif tokens[0] == 'props':
				record.props = tokens[1:]

			elif tokens[0] == 'chromium':
				record.chromium = tokens[1:]

			elif tokens[0] == 'vector':
				vecName = tokens[1]
				vecSize = int(tokens[2])
				for i in range(len(record.params)):
					(name, type, oldSize) = record.params[i]
					if name == vecName:
						record.params[i] = (name, type, vecSize)
						break

			elif tokens[0] == 'chrelopcode':
				record.chrelopcode = int(tokens[1])

			else:
				print('Invalid token %s after function %s' % (tokens[0], record.name))
			#endif
		#endif
	#endfor
	specFile.close()
#enddef





# Dictionary [name] of APIFunction:
__FunctionDict = {}

# Dictionary [name] of name
__VectorVersion = {}

# Reverse mapping of function name aliases
__ReverseAliases = {}


def AddFunction(record):
	assert record.name not in __FunctionDict
	#if not "omit" in record.chromium:
	__FunctionDict[record.name] = record



def GetFunctionDict(specFile = ""):
	if not specFile:
		specFile = sys.argv[1]+"/APIspec.txt"
	if len(__FunctionDict) == 0:
		ProcessSpecFile(specFile, AddFunction)
		# Look for vector aliased functions
		for func in __FunctionDict.keys():
			va = __FunctionDict[func].vectoralias
			if va != '':
				__VectorVersion[va] = func
			#endif

			# and look for regular aliases (for glloader)
			a = __FunctionDict[func].alias
			if a:
				if a in __ReverseAliases:
					__ReverseAliases[a].append(func)
				else:
					__ReverseAliases[a] = [func]
	#endif
	return __FunctionDict


def GetAllFunctions(specFile = ""):
	"""Return sorted list of all functions known to Chromium."""
	d = GetFunctionDict(specFile)
	funcs = []
	for func in d.keys():
		rec = d[func]
		if not "omit" in rec.chromium:
			funcs.append(func)
	funcs.sort()
	return funcs
	
def GetAllFunctionsAndOmittedAliases(specFile = ""):
	"""Return sorted list of all functions known to Chromium."""
	d = GetFunctionDict(specFile)
	funcs = []
	for func in d.keys():
		rec = d[func]
		if (not "omit" in rec.chromium or
			rec.alias != ''):
			funcs.append(func)
	funcs.sort()
	return funcs

def GetDispatchedFunctions(specFile = ""):
	"""Return sorted list of all functions handled by SPU dispatch table."""
	d = GetFunctionDict(specFile)
	funcs = []
	for func in d.keys():
		rec = d[func]
		if (not "omit" in rec.chromium and
			not "stub" in rec.chromium and
			rec.alias == ''):
			funcs.append(func)
	funcs.sort()
	return funcs

#======================================================================

def ReturnType(funcName):
	"""Return the C return type of named function.
	Examples: "void" or "const GLubyte *". """
	d = GetFunctionDict()
	return d[funcName].returnType


def Parameters(funcName):
	"""Return list of tuples (name, type, vecSize) of function parameters.
	Example: if funcName=="ClipPlane" return
	[ ("plane", "GLenum", 0), ("equation", "const GLdouble *", 4) ] """
	d = GetFunctionDict()
	return d[funcName].params

def ParamAction(funcName):
	"""Return list of names of actions for testing.
	For PackerTest only."""
	d = GetFunctionDict()
	return d[funcName].paramaction

def ParamList(funcName):
	"""Return list of tuples (name, list of values) of function parameters.
	For PackerTest only."""
	d = GetFunctionDict()
	return d[funcName].paramlist

def ParamVec(funcName):
	"""Return list of tuples (name, vector of values) of function parameters.
	For PackerTest only."""
	d = GetFunctionDict()
	return d[funcName].paramvec

def ParamSet(funcName):
	"""Return list of tuples (name, list of values) of function parameters.
	For PackerTest only."""
	d = GetFunctionDict()
	return d[funcName].paramset


def Properties(funcName):
	"""Return list of properties of the named GL function."""
	d = GetFunctionDict()
	return d[funcName].props

def AllWithProperty(property):
	"""Return list of functions that have the named property."""
	funcs = []
	for funcName in GetDispatchedFunctions():
		if property in Properties(funcName):
			funcs.append(funcName)
	return funcs

def Category(funcName):
	"""Return the category of the named GL function."""
	d = GetFunctionDict()
	return d[funcName].category

def ChromiumProps(funcName):
	"""Return list of Chromium-specific properties of the named GL function."""
	d = GetFunctionDict()
	return d[funcName].chromium
	
def ChromiumRelOpCode(funcName):
	"""Return list of Chromium-specific properties of the named GL function."""
	d = GetFunctionDict()
	return d[funcName].chrelopcode
	

def ParamProps(funcName):
	"""Return list of Parameter-specific properties of the named GL function."""
	d = GetFunctionDict()
	return d[funcName].paramprop

def Alias(funcName):
	"""Return the function that the named function is an alias of.
	Ex: Alias('DrawArraysEXT') = 'DrawArrays'.
	"""
	d = GetFunctionDict()
	return d[funcName].alias


def ReverseAliases(funcName):
	"""Return a list of aliases."""
	d = GetFunctionDict()
	if funcName in __ReverseAliases:
		return sorted(__ReverseAliases[funcName])
	else:
		return []


def ReverseAliasesMaxCount():
	"""Returns the maximum number of aliases possible for a function."""
	d = GetFunctionDict()
	return max([len(a) for a in __ReverseAliases.values()])


def NonVectorFunction(funcName):
	"""Return the non-vector version of the given function, or ''.
	For example: NonVectorFunction("Color3fv") = "Color3f"."""
	d = GetFunctionDict()
	return d[funcName].vectoralias


def VectorFunction(funcName):
	"""Return the vector version of the given non-vector-valued function,
	or ''.
	For example: VectorVersion("Color3f") = "Color3fv"."""
	d = GetFunctionDict()
	if funcName in __VectorVersion.keys():
		return __VectorVersion[funcName]
	else:
		return ''


def GetCategoryWrapper(func_name):
	"""Return a C preprocessor token to test in order to wrap code.
	This handles extensions.
	Example: GetTestWrapper("glActiveTextureARB") = "CR_multitexture"
	Example: GetTestWrapper("glBegin") = ""
	"""
	cat = Category(func_name)
	if (cat == "1.0" or
		cat == "1.1" or
		cat == "1.2" or
		cat == "Chromium" or
		cat == "GL_chromium" or
		cat == "VBox"):
		return ''
	elif (cat == '1.3' or
		  cat == '1.4' or
		  cat == '1.5' or
		  cat == '2.0' or
		  cat == '2.1'):
		# i.e. OpenGL 1.3 or 1.4 or 1.5
		return "OPENGL_VERSION_" + cat.replace(".", "_")
	else:
		assert cat != ''
		return cat.replace("GL_", "")


def CanCompile(funcName):
	"""Return 1 if the function can be compiled into display lists, else 0."""
	props = Properties(funcName)
	if ("nolist" in props or
		"get" in props or
		"setclient" in props):
		return 0
	else:
		return 1

def HasChromiumProperty(funcName, propertyList):
	"""Return 1 if the function or any alias has any property in the
	propertyList"""
	for funcAlias in [funcName, NonVectorFunction(funcName), VectorFunction(funcName)]:
		if funcAlias:
			props = ChromiumProps(funcAlias)
			for p in propertyList:
				if p in props:
					return 1
	return 0

def CanPack(funcName):
	"""Return 1 if the function can be packed, else 0."""
	return HasChromiumProperty(funcName, ['pack', 'extpack', 'expandpack'])

def HasPackOpcode(funcName):
	"""Return 1 if the function has a true pack opcode"""
	return HasChromiumProperty(funcName, ['pack', 'extpack'])

def SetsState(funcName):
	"""Return 1 if the function sets server-side state, else 0."""
	props = Properties(funcName)

	# Exceptions.  The first set of these functions *do* have 
	# server-side state-changing  effects, but will be missed 
	# by the general query, because they either render (e.g. 
	# Bitmap) or do not compile into display lists (e.g. all the others).
	# 
	# The second set do *not* have server-side state-changing
	# effects, despite the fact that they do not render
	# and can be compiled.  They are control functions
	# that are not trackable via state.
	if funcName in ['Bitmap', 'DeleteTextures', 'FeedbackBuffer', 
		'RenderMode', 'BindBufferARB', 'DeleteFencesNV']:
		return 1
	elif funcName in ['ExecuteProgramNV']:
		return 0

	# All compilable functions that do not render and that do
	# not set or use client-side state (e.g. DrawArrays, et al.), set
	# server-side state.
	if CanCompile(funcName) and "render" not in props and "useclient" not in props and "setclient" not in props:
		return 1

	# All others don't set server-side state.
	return 0

def SetsClientState(funcName):
	"""Return 1 if the function sets client-side state, else 0."""
	props = Properties(funcName)
	if "setclient" in props:
		return 1
	return 0

def SetsTrackedState(funcName):
	"""Return 1 if the function sets state that is tracked by
	the state tracker, else 0."""
	# These functions set state, but aren't tracked by the state
	# tracker for various reasons: 
	# - because the state tracker doesn't manage display lists
	#   (e.g. CallList and CallLists)
	# - because the client doesn't have information about what
	#   the server supports, so the function has to go to the
	#   server (e.g. CompressedTexImage calls)
	# - because they require a round-trip to the server (e.g.
	#   the CopyTexImage calls, SetFenceNV, TrackMatrixNV)
	if funcName in [
		'CopyTexImage1D', 'CopyTexImage2D',
		'CopyTexSubImage1D', 'CopyTexSubImage2D', 'CopyTexSubImage3D',
		'CallList', 'CallLists',
		'CompressedTexImage1DARB', 'CompressedTexSubImage1DARB',
		'CompressedTexImage2DARB', 'CompressedTexSubImage2DARB',
		'CompressedTexImage3DARB', 'CompressedTexSubImage3DARB',
		'SetFenceNV'
		]:
		return 0

	# Anything else that affects client-side state is trackable.
	if SetsClientState(funcName):
		return 1

	# Anything else that doesn't set state at all is certainly
	# not trackable.
	if not SetsState(funcName):
		return 0

	# Per-vertex state isn't tracked the way other state is
	# tracked, so it is specifically excluded.
	if "pervertex" in Properties(funcName):
		return 0

	# Everything else is fine
	return 1

def UsesClientState(funcName):
	"""Return 1 if the function uses client-side state, else 0."""
	props = Properties(funcName)
	if "pixelstore" in props or "useclient" in props:
		return 1
	return 0

def IsQuery(funcName):
	"""Return 1 if the function returns information to the user, else 0."""
	props = Properties(funcName)
	if "get" in props:
		return 1
	return 0

def FuncGetsState(funcName):
	"""Return 1 if the function gets GL state, else 0."""
	d = GetFunctionDict()
	props = Properties(funcName)
	if "get" in props:
		return 1
	else:
		return 0

def IsPointer(dataType):
	"""Determine if the datatype is a pointer.  Return 1 or 0."""
	if dataType.find("*") == -1:
		return 0
	else:
		return 1


def PointerType(pointerType):
	"""Return the type of a pointer.
	Ex: PointerType('const GLubyte *') = 'GLubyte'
	"""
	t = pointerType.split(' ')
	if t[0] == "const":
		t[0] = t[1]
	return t[0]




def OpcodeName(funcName):
	"""Return the C token for the opcode for the given function."""
	return "CR_" + funcName.upper() + "_OPCODE"


def ExtendedOpcodeName(funcName):
	"""Return the C token for the extended opcode for the given function."""
	return "CR_" + funcName.upper() + "_EXTEND_OPCODE"




#======================================================================

def _needPointerOrigin(name, type):
	return type == 'const GLvoid *' and (name == 'pointer' or name == 'NULL');

def MakeCallString(params):
	"""Given a list of (name, type, vectorSize) parameters, make a C-style
	formal parameter string.
	Ex return: 'index, x, y, z'.
	"""
	result = ''
	i = 1
	n = len(params)
	for (name, type, vecSize) in params:
		result += name
		if i < n:
			result = result + ', '
		i += 1
	#endfor
	return result
#enddef

def MakeCallStringForDispatcher(params):
	"""Same as MakeCallString, but with 'pointer' origin hack for bugref:9407."""
	strResult = ''
	fFirst    = True;
	for (name, type, vecSize) in params:
		if not fFirst:  strResult += ', ';
		else:           fFirst     = False;
		strResult += name;
		if _needPointerOrigin(name, type):
			if name != 'NULL': strResult += ', fRealPtr';
			else:   		   strResult += ', 0 /*fRealPtr*/';
	return strResult;


def MakeDeclarationString(params):
	"""Given a list of (name, type, vectorSize) parameters, make a C-style
	parameter declaration string.
	Ex return: 'GLuint index, GLfloat x, GLfloat y, GLfloat z'.
	"""
	n = len(params)
	if n == 0:
		return 'void'
	else:
		result = ''
		i = 1
		for (name, type, vecSize) in params:
			result = result + type + ' ' + name
			if i < n:
				result = result + ', '
			i += 1
		#endfor
		return result
	#endif
#enddef

def MakeDeclarationStringForDispatcher(params):
	"""Same as MakeDeclarationString, but with 'pointer' origin hack for bugref:9407."""
	if len(params) == 0:
		return 'void';
	strResult = '';
	fFirst    = True;
	for (name, type, vecSize) in params:
		if not fFirst:  strResult += ', ';
		else:           fFirst     = False;
		strResult = strResult + type + ' ' + name;
		if _needPointerOrigin(name, type):
			strResult = strResult + ' CRVBOX_HOST_ONLY_PARAM(int fRealPtr)';
	return strResult;

def MakeDeclarationStringWithContext(ctx_macro_prefix, params):
	"""Same as MakeDeclarationString, but adds a context macro
	"""
	
	n = len(params)
	if n == 0:
		return ctx_macro_prefix + '_ARGSINGLEDECL'
	else:
		result = MakeDeclarationString(params)
		return ctx_macro_prefix + '_ARGDECL ' + result
	#endif
#enddef


def MakePrototypeString(params):
	"""Given a list of (name, type, vectorSize) parameters, make a C-style
	parameter prototype string (types only).
	Ex return: 'GLuint, GLfloat, GLfloat, GLfloat'.
	"""
	n = len(params)
	if n == 0:
		return 'void'
	else:
		result = ''
		i = 1
		for (name, type, vecSize) in params:
			result = result + type
			# see if we need a comma separator
			if i < n:
				result = result + ', '
			i += 1
		#endfor
		return result
	#endif
#enddef

def MakePrototypeStringForDispatcher(params):
	"""Same as MakePrototypeString, but with 'pointer' origin hack for bugref:9407."""
	if len(params) == 0:
		return 'void'
	strResult = ''
	fFirst    = True;
	for (name, type, vecSize) in params:
		if not fFirst:  strResult += ', ';
		else:           fFirst     = False;
		strResult = strResult + type
		if _needPointerOrigin(name, type):
			strResult += ' CRVBOX_HOST_ONLY_PARAM(int)';
	return strResult;


#======================================================================
	
__lengths = {
	'GLbyte': 1,
	'GLubyte': 1,
	'GLshort': 2,
	'GLushort': 2,
	'GLint': 4,
	'GLuint': 4,
	'GLfloat': 4,
	'GLclampf': 4,
	'GLdouble': 8,
	'GLclampd': 8,
	'GLenum': 4,
	'GLboolean': 1,
	'GLsizei': 4,
	'GLbitfield': 4,
	'void': 0,  # XXX why?
	'int': 4,
	'GLintptrARB': 4,   # XXX or 8 bytes?
	'GLsizeiptrARB': 4, # XXX or 8 bytes?
	'VBoxGLhandleARB': 4,
	'GLcharARB': 1,
	'uintptr_t': 4
}

def sizeof(type):
	"""Return size of C datatype, in bytes."""
	if not type in __lengths.keys():
		print >>sys.stderr, "%s not in lengths!" % type
	return __lengths[type]


#======================================================================
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

def PacketLength( params ):
	len = 0
	for (name, type, vecSize) in params:
		if IsPointer(type):
			size = PointerSize()
		else:
			assert type.find("const") == -1
			size = sizeof(type)
		len = FixAlignment( len, size ) + size
	len = WordAlign( len )
	return len

#======================================================================

__specials = {}

def LoadSpecials( filename ):
	table = {}
	try:
		f = open( filename, "r" )
	except:
#		try:
		f = open( sys.argv[2]+"/"+filename, "r")
#		except:
#			__specials[filename] = {}
#			print "%s not present" % filename
#			return {}
	
	for line in f.readlines():
		line = line.strip()
		if line == "" or line[0] == '#':
			continue
		table[line] = 1
	
	__specials[filename] = table
	return table


def FindSpecial( table_file, glName ):
	table = {}
	filename = table_file + "_special"
	try:
		table = __specials[filename]
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
		table = __specials[filename]
	except KeyError:
		table = LoadSpecials( filename )
	
	return sorted(table.keys())


def NumSpecials( table_file ):
	filename = table_file + "_special"
	table = {}
	try:
		table = __specials[filename]
	except KeyError:
		table = LoadSpecials(filename)
	return len(table.keys())

def PrintRecord(record):
	argList = MakeDeclarationString(record.params)
	if record.category == "Chromium":
		prefix = "cr"
	else:
		prefix = "gl"
	print('%s %s%s(%s);' % (record.returnType, prefix, record.name, argList ))
	if len(record.props) > 0:
		print('   /* %s */' % string.join(record.props, ' '))

#ProcessSpecFile("APIspec.txt", PrintRecord)

