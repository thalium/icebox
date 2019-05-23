# $Id: dlm_generated.py $
import sys, cPickle, re

sys.path.append( "../glapi_parser" )
import apiutil

# A routine that can create call strings from instance names
def InstanceCallString( params ):
	output = ''
	for index in range(0,len(params)):
		if index > 0:
			output += ", "
		if params[index][0] != '':
			output += 'instance->' + params[index][0]
	return output

def GetPointerType(basetype):
	words = basetype.split()
	if words[0] == 'const':
		words = words[1:]
	if words[-1].endswith('*'):
		words[-1] = words[-1][:-1].strip()
		if words[-1] == '':
			words = words[:-1]
	if words[0] == 'void' or words[0] == 'GLvoid':
		words[0] = 'int'
	return ' '.join(words)


def GetPointerInfo(functionName):
	# We'll keep track of all the parameters that require pointers.
	# They'll require special handling later.
	params = apiutil.Parameters(functionName)
	pointers = []
	pointername=''
	pointerarg=''
	pointertype=''
	pointersize=0
	pointercomment=''

	index = 0
	for (name, type, vecSize) in params:
		# Watch out for the word "const" (which should be ignored)
		# and for types that end in "*" (which are pointers and need
		# special treatment)
		words = type.split()
		if words[-1].endswith('*'):
			pointers.append(index)
		index += 1

	# If any argument was a pointer, we need a special pointer data
	# array.  The pointer data will be stored into this array, and
	# references to the array will be generated as parameters.
	if len(pointers) == 1:
		index = pointers[0]
		pointername = params[index][0]
		pointerarg = pointername + 'Data'
		pointertype = GetPointerType(params[index][1])
		pointersize = params[index][2]
		if pointersize == 0:
			pointersize = "special"
	elif len(pointers) > 1:
		pointerarg = 'data';
		pointertype = GetPointerType(params[pointers[0]][1])
		for index in range(1,len(pointers)):
			if GetPointerType(params[pointers[index]][1]) != pointertype:
				pointertype = 'GLvoid *'

	return (pointers,pointername,pointerarg,pointertype,pointersize,pointercomment)

def wrap_struct(functionName):
	params = apiutil.Parameters(functionName)
	argstring = apiutil.MakeDeclarationString(params)
	extendedArgstring = argstring
	props = apiutil.Properties(functionName)
	if "useclient" in props or "pixelstore" in props:
		extendedArgstring += ", CRClientState *c"

	# We'll keep track of all the parameters that require pointers.
	# They'll require special handling later.
	(pointers, pointername, pointerarg, pointertype, pointersize, pointercomment) = GetPointerInfo(functionName)

	# Start writing the header
	print 'struct instance%s {' % (functionName)
	print '	DLMInstanceList *next;'
	print '	DLMInstanceList *stateNext;'
	print '	int cbInstance;'
	print '	VBoxDLOpCode iVBoxOpCode;'
	print '	void (DLM_APIENTRY *execute)(DLMInstanceList *instance, SPUDispatchTable *dispatchTable);'
	for (name, type, vecSize) in params:
		# Watch out for the word "const" (which should be ignored)
		# and for types that end in "*" (which are pointers and need
		# special treatment)
		words = type.split()
		if words[0] == 'const':
			words = words[1:]
		if words[0] != "void":
			print '	%s %s;' % (' '.join(words), name)

	# If any argument was a pointer, we need a special pointer data
	# array.  The pointer data will be stored into this array, and
	# references to the array will be generated as parameters.
	if len(pointers) == 1:
		if pointersize == None:
			print "	/* Oh no - pointer parameter %s found, but no pointer class specified and can't guess */" % pointername
		else:
			if pointersize == 'special':
				print '	%s %s[1];%s' % (pointertype, pointerarg, pointercomment)
			else:
				print '	%s %s[%s];%s' % (pointertype, pointerarg, pointersize,pointercomment)
	elif len(pointers) > 1:
		print '	%s %s[1];%s' % (pointertype, pointerarg,pointercomment)

	print '};'

	# Pointers only happen with instances
	if len(pointers) > 1 or (len(pointers) == 1 and pointersize == 'special'):
		print 'int crdlm_pointers_%s(struct instance%s *instance, %s);' % (functionName, functionName, extendedArgstring)
		
	# See if the GL function must sometimes allow passthrough even
	# if the display list is open
	if "checklist" in apiutil.ChromiumProps(functionName):
		print 'int crdlm_checklist_%s(%s);' % (functionName, argstring)

	return

def wrap_execute(functionName):

    params = apiutil.Parameters(functionName)
    (pointers, _, pointerarg, _, _, _) = GetPointerInfo(functionName)

    print 'static void execute%s(DLMInstanceList *x, SPUDispatchTable *dispatchTable)' % functionName
    print '{'
    if len(params) > 0:
        print '    struct instance%s *instance = (struct instance%s *)x;' % (functionName, functionName)

    if len(pointers) == 1:
        print '    instance->%s = instance->%s;' % (params[pointers[0]][0], pointerarg)

    print '    if (dispatchTable->%s != NULL)' % (functionName)
    print '        dispatchTable->%s(%s);' % (functionName, InstanceCallString(params))
    print '    else'
    print '        crWarning("DLM warning: execute%s called with NULL dispatch entry");' % (functionName)
    print '}'

# These code snippets isolate the code required to add a given instance
# to the display list correctly.  They are used during generation, to
# generate correct code, and also to create useful utilities.
def AddInstanceToList(pad):
    print '%s/* Add this instance to the current display list. */' % pad
    print '%sinstance->next = NULL;' % pad
    print '%sinstance->stateNext = NULL;' % pad
    print '%sif (!state->currentListInfo->first) {' % pad
    print '%s    state->currentListInfo->first = (DLMInstanceList *)instance;' % pad
    print '%s}' % pad
    print '%selse {' % pad
    print '%s  state->currentListInfo->last->next = (DLMInstanceList *)instance;' % pad
    print '%s}' % pad
    print '%sstate->currentListInfo->last = (DLMInstanceList *)instance;' % pad
    print '%sstate->currentListInfo->numInstances++;' % pad

def AddInstanceToStateList(pad):
    print '%s/* Instances that change state have to be added to the state list as well. */' % pad
    print '%sif (!state->currentListInfo->stateFirst) {' % pad
    print '%s    state->currentListInfo->stateFirst = (DLMInstanceList *)instance;' % pad
    print '%s}' % pad
    print '%selse {' % pad
    print '%s    state->currentListInfo->stateLast->stateNext = (DLMInstanceList *)instance;' % pad
    print '%s}' % pad
    print '%sstate->currentListInfo->stateLast = (DLMInstanceList *)instance;' % pad


# The compile wrapper collects the parameters into a DLMInstanceList
# element, and adds that element to the end of the display list currently
# being compiled.
def wrap_compile(functionName):
    params = apiutil.Parameters(functionName)
    return_type = apiutil.ReturnType(functionName)
    # Make sure the return type is void.  It's nonsensical to compile
    # an element with any other return type.
    if return_type != 'void':
        print '/* Nonsense: DL function %s has a %s return type?!? */' % (functionName, return_type)

    # Define a structure to hold all the parameters.  Note that the
    # top parameters must exactly match the DLMInstanceList structure
    # in include/cr_dlm.h, or everything will break horribly.
    # Start off by getting all the pointer info we could ever use
    # from the parameters
    (pointers, pointername, pointerarg, pointertype, pointersize, pointercomment) = GetPointerInfo(functionName)

    # Finally, the compile wrapper.  This one will diverge strongly
    # depending on whether or not there are pointer parameters. 
    callstring = apiutil.MakeCallString(params)
    argstring = apiutil.MakeDeclarationString(params)
    props = apiutil.Properties(functionName)
    if "useclient" in props or "pixelstore" in props:
        callstring += ", c"
        argstring += ", CRClientState *c"
    print 'void DLM_APIENTRY crDLMCompile%s(%s)' % (functionName, argstring)
    print '{'
    print '    CRDLMContextState *state = CURRENT_STATE();'
    print '    struct instance%s *instance;' % (functionName)

    # The calling SPU is supposed to verify that the element is supposed to be
    # compiled before it is actually compiled; typically, this is done based
    # on whether a glNewList has been executed more recently than a glEndList.
    # But some functions are dual-natured, sometimes being compiled, and sometimes
    # being executed immediately.  We can check for this here.
    if "checklist" in apiutil.ChromiumProps(functionName):
        print '    if (crDLMCheckList%s(%s))' % (functionName, apiutil.MakeCallString(params))
        print '    {'
        print '        crdlm_error(__LINE__, __FILE__, GL_INVALID_OPERATION,'
        print '            "this instance of function %s should not be compiled");' % functionName;
        print '        return;'
        print '    }'

    if len(pointers) > 1 or pointersize == 'special':
        # Pass NULL, to just allocate space
        print '    instance = crCalloc(sizeof(struct instance%s) + crdlm_pointers_%s(NULL, %s));' % (functionName, functionName, callstring)
    else:
        print '    instance = crCalloc(sizeof(struct instance%s));' % (functionName)
    print '    if (!instance)'
    print '    {'
    print '        crdlm_error(__LINE__, __FILE__, GL_OUT_OF_MEMORY,'
    print '            "out of memory adding %s to display list");' % (functionName)
    print '        return;'
    print '    }'

    # Put in the fields that must always exist
    print '    instance->execute = execute%s;' % functionName

    # Apply all the simple (i.e. non-pointer) parameters
    for index in range(len(params)):
        if index not in pointers:
            name = params[index][0]
            print '    instance->%s = %s;' % (name, name)

    # We need to know instance size in bytes in order to save its state later.
    print '    instance->cbInstance = sizeof(struct instance%s);' % functionName

    # Set OPCODE.
    print '    instance->iVBoxOpCode = VBOX_DL_OPCODE_%s;' % functionName

    # If there's a pointer parameter, apply it.
    if len(pointers) == 1:

        print '    if (%s == NULL)' % (params[pointers[0]][0])
        print '        instance->%s = NULL;' % (params[pointers[0]][0])
        print '    else'
        print '        instance->%s = instance->%s;' % (params[pointers[0]][0], pointerarg)

        if pointersize == 'special':
            print '    instance->cbInstance += crdlm_pointers_%s(instance, %s);' % (functionName, callstring)
        else:
            print '    crMemcpy((void *)instance->%s, (void *) %s, %s*sizeof(%s));' % (params[pointers[0]][0], params[pointers[0]][0], pointersize, pointertype)
    elif len(pointers) == 2:
        # this seems to work
        print '    instance->cbInstance += crdlm_pointers_%s(instance, %s);' % (functionName, callstring)
    elif len(pointers) > 2:
        print "#error don't know how to handle pointer parameters for %s" % (functionName)

    # Add the element to the current display list
    AddInstanceToList('    ')
    # If the element is a state-changing element, add it to the current state list
    if apiutil.SetsTrackedState(functionName):
        AddInstanceToStateList('    ')
    print '}'

whichfile=sys.argv[1]
if whichfile == 'headers':
    print """#ifndef _DLM_GENERATED_H
#define _DLM_GENERATED_H

#include <VBoxUhgsmi.h>

/* DO NOT EDIT.  This file is auto-generated by dlm_generated.py. */
"""
else:
    print """#include <stdio.h>
#include "cr_spu.h"
#include "cr_dlm.h"
#include "cr_mem.h"
#include "cr_error.h"
#include "state/cr_statefuncs.h"
#include "dlm.h"
#include "dlm_pointers.h"
#include "dlm_generated.h"

/* DO NOT EDIT.  This file is auto-generated by dlm_generated.py. */
"""

# Add in the "add_to_dl" utility function, which will be used by
# external (i.e. non-generated) functions.  The utility ensures that 
# any external functions that are written for compiling elements
# don't have to be rewritten if the conventions for adding to display
# lists are changed.
print """
void crdlm_add_to_list(
    DLMInstanceList *instance,
    void (*executeFunc)(DLMInstanceList *x, SPUDispatchTable *dispatchTable)"""

if (whichfile == 'headers'):
    print ");"
else:
    print """) {
    CRDLMContextState *state = CURRENT_STATE();
    instance->execute = executeFunc;"""

    # Add in the common code for adding the instance to the display list
    AddInstanceToList("    ")

    print '}'
    print ''

# Now generate the functions that won't use the crdlm_add_to_list utility.
# These all directly add their own instances to the current display list
# themselves, without using the crdlm_add_to_list() function.
keys = apiutil.GetDispatchedFunctions(sys.argv[3]+"/APIspec.txt")
for func_name in keys:
	if apiutil.CanCompile(func_name):
		print "\n/*** %s ***/" % func_name
		# Auto-generate an appropriate DL function.  First, functions
		# that go into the display list but that rely on state will
		# have to have their argument strings expanded, to take pointers 
		# to that appropriate state.
		if whichfile == "headers":
		    wrap_struct(func_name)
		elif not apiutil.FindSpecial("dlm", func_name):
		    wrap_execute(func_name)
		    wrap_compile(func_name)


# Generate mapping between OPCODE and routines to be executed.

if whichfile == "headers":
    # Execute routine prototype needed to add static array of routines.
    print ''
    print 'struct DLMInstanceList;'
    print 'typedef void (*VBoxDLMExecuteFn)(struct DLMInstanceList *instance, SPUDispatchTable *dispatchTable);'
    print ''
    print 'extern VBoxDLMExecuteFn g_VBoxDLMExecuteFns[VBOX_DL_OPCODE_MAX];'
    print ''
else:
    print ''
    print 'VBoxDLMExecuteFn g_VBoxDLMExecuteFns[] = {'

    for func_name in keys:
        if apiutil.CanCompile(func_name) and not apiutil.FindSpecial("dlm", func_name):
            print '    execute%s,' % func_name

    print '};'
    print ''

if whichfile == 'headers':
    print "#endif /* _DLM_GENERATED_H */"
