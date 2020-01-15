# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys

from pack_currenttypes import *
import apiutil

apiutil.CopyrightC()

print('''
#include "state/cr_currentpointers.h"
#include "state.h"

#include <stdio.h>

#ifdef WINDOWS
#pragma warning( disable : 4127 )
#endif

typedef void (*convert_func) (GLfloat *, const unsigned char *);
''')

import convert

for k in sorted(current_fns.keys()):
	name = k
	name = '%s%s' % (k[:1].lower(),k[1:])
	ucname = k.upper()
	num_members = len(current_fns[k]['default']) + 1

	print('#define VPINCH_CONVERT_%s(op,data,dst) \\' % ucname)
	print('{\\')
	print('\tGLfloat vdata[%d] = {' % num_members, end=' ')

## Copy dst data into vdata
	i = 0;
	for defaultvar in current_fns[k]['default']:
		print('%d' % defaultvar, end=' ')
		if i != num_members:
			print(',', end=' ')
		i += 1
	print('};\\')

	print('\tswitch (op) { \\')
	for type in current_fns[k]['types']:
		if type[0:1] == "N":
			normalize = 1
			type = type[1:]
		else:
			normalize = 0
		for size in current_fns[k]['sizes']:
			uctype = type.upper()
			if ucname == 'EDGEFLAG':
				print('\tcase CR_%s_OPCODE: \\' % ucname)
			else:
				print('\tcase CR_%s%d%s_OPCODE: \\' % (ucname,size,uctype))
			
			if (ucname == 'COLOR' or ucname == 'NORMAL' or ucname == 'SECONDARYCOLOR' or normalize) and type != 'f' and type != 'd':
				print('\t\t__convert_rescale_%s%d (vdata, (%s *) (data)); \\' % (type,size,gltypes[type]['type']))
			else:
				print('\t\t__convert_%s%d (vdata, (%s *) (data)); \\' % (type,size,gltypes[type]['type']))
			print('\t\tbreak; \\')

	print('\tdefault: \\')
	print('\t\tcrSimpleError ( "Unknown opcode in VPINCH_CONVERT_%s" ); \\' % ucname)
	print('\t}\\')

	i = 0
	for member in current_fns[k]['members']:
		print('\t(dst).%s = vdata[%d];\\' % (member,i))
		i += 1

	print('}\n')

print('''

void crStateCurrentRecover(PCRStateTracker pState)
{
	const unsigned char *v;
	convert_func convert=NULL;
	CRContext *g = GetCurrentContext(pState);
	CRCurrentState *c = &(g->current);
	CRStateBits *sb = GetCurrentBits(pState);
	CRCurrentBits *cb = &(sb->current);
	static const GLfloat color_default[4]			= {0.0f, 0.0f, 0.0f, 1.0f};
	static const GLfloat secondaryColor_default[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	static const GLfloat texCoord_default[4]	= {0.0f, 0.0f, 0.0f, 1.0f};
	static const GLfloat normal_default[4]		= {0.0f, 0.0f, 0.0f, 1.0f};
	static const GLfloat index_default			= 0.0f;
	static const GLboolean edgeFlag_default		= GL_TRUE;
	static const GLfloat vertexAttrib_default[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	static const GLfloat fogCoord_default = 0.0f;
	GLnormal_p		*normal		= &(c->current->c.normal);
	GLcolor_p		*color		= &(c->current->c.color);
	GLsecondarycolor_p *secondaryColor = &(c->current->c.secondaryColor);
	GLtexcoord_p	*texCoord	= &(c->current->c.texCoord);
	GLindex_p		*index		= &(c->current->c.index);
	GLedgeflag_p	*edgeFlag	= &(c->current->c.edgeFlag);
	GLvertexattrib_p *vertexAttrib = &(c->current->c.vertexAttrib);
	GLfogcoord_p *fogCoord = &(c->current->c.fogCoord);
	unsigned int i;
	CRbitvalue nbitID[CR_MAX_BITARRAY];

	/* 
	 * If the calling SPU hasn't called crStateSetCurrentPointers()
	 * we can't recover anything, so abort now. (i.e. we don't have
	 * a pincher, oh, and just emit the message once).
	 */
	if (!c->current) {
		static int donewarning = 0;
		if (!donewarning)
			crWarning("No pincher, please call crStateSetCurrentPointers() in your SPU");
		donewarning = 1;
		return; /* never get here */
	}

	c->attribsUsedMask = c->current->attribsUsedMask;
	
	/* silence warnings */
	(void) __convert_b1;
	(void) __convert_b2;
	(void) __convert_b3;
	(void) __convert_b4;
	(void) __convert_ui1;
	(void) __convert_ui2;
	(void) __convert_ui3;
	(void) __convert_ui4;
	(void) __convert_l1;
	(void) __convert_l2;
	(void) __convert_l3;
	(void) __convert_l4;
	(void) __convert_us1;
	(void) __convert_us2;
	(void) __convert_us3;
	(void) __convert_us4;
	(void) __convert_ub1;
	(void) __convert_ub2;
	(void) __convert_ub3;
	(void) __convert_ub4;
	(void) __convert_rescale_s1;
	(void) __convert_rescale_s2;
	(void) __convert_rescale_b1;
	(void) __convert_rescale_b2;
	(void) __convert_rescale_ui1;
	(void) __convert_rescale_ui2;
	(void) __convert_rescale_i1;
	(void) __convert_rescale_i2;
	(void) __convert_rescale_us1;
	(void) __convert_rescale_us2;
	(void) __convert_rescale_ub1;
	(void) __convert_rescale_ub2;
	(void) __convert_Ni1;
	(void) __convert_Ni2;
	(void) __convert_Ni3;
	(void) __convert_Ni4;
	(void) __convert_Nb1;
	(void) __convert_Nb2;
	(void) __convert_Nb3;
	(void) __convert_Nb4;
	(void) __convert_Nus1;
	(void) __convert_Nus2;
	(void) __convert_Nus3;
	(void) __convert_Nus4;
	(void) __convert_Nui1;
	(void) __convert_Nui2;
	(void) __convert_Nui3;
	(void) __convert_Nui4;
	(void) __convert_Ns1;
	(void) __convert_Ns2;
	(void) __convert_Ns3;
	(void) __convert_Ns4;
	(void) __convert_Nub1;
	(void) __convert_Nub2;
	(void) __convert_Nub3;
	(void) __convert_Nub4;

	DIRTY(nbitID, g->neg_bitid);

	/* Save pre state */
	for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++) {
		COPY_4V(c->vertexAttribPre[i]  , c->vertexAttrib[i]);
	}
	c->edgeFlagPre = c->edgeFlag;
	c->colorIndexPre = c->colorIndex;

''')

for k in sorted(current_fns.keys()):
	print('\t/* %s */' % k)
	print('\tv = NULL;')
	name = '%s%s' % (k[:1].lower(),k[1:])

	indent = ""
	if 'array' in current_fns[k]:
		print('\tfor (i = 0; i < %s; i++)' % current_fns[k]['array'])
		print('\t{')
		indent += "\t"
	for type in current_fns[k]['types']:
		if type[0:1] == "N":
			normalized = 1
			type2 = type[1:]
		else:
			normalized = 0
			type2 = type
		for size in current_fns[k]['sizes']:
			ptr = '%s->%s%d' % (name, type, size )
			if 'array' in current_fns[k]:
				ptr += "[i]"
			print('%s\tif (v < %s)' % (indent, ptr))
			print('%s\t{' % indent)
			print('%s\t\tv = %s;' % (indent, ptr))
			if (k == 'Color' or k == 'Normal' or k == 'SecondaryColor' or normalized) and type != 'f' and type != 'd' and type != 'l':
				print('%s\t\tconvert = (convert_func) __convert_rescale_%s%d;' % (indent,type,size))
			else:
				print('%s\t\tconvert = (convert_func) __convert_%s%d;' % (indent,type,size))
			print('%s\t}' % indent)
	print('')
	print('%s\tif (v != NULL) {' % indent)
	if 'array' in current_fns[k]:
		if k == 'TexCoord':
			print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_TEX0 + i], %s_default);' % (indent,name))
		else:
			print('%s\t\tCOPY_4V(c->%s[i], %s_default);' % (indent,name,name))
	else:
		if k == 'Normal':
			print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_NORMAL], %s_default);' % (indent,name))
		elif k == 'FogCoord':
			print('%s\t\tc->vertexAttrib[VERT_ATTRIB_FOG][0] =  %s_default;' % (indent,name))
		elif k == 'Color':
			print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_COLOR0], %s_default);' % (indent,name))
		elif k == 'SecondaryColor':
			print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_COLOR1],  %s_default);' % (indent,name))
		elif k == 'TexCoord':
			print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_TEX0], %s_default);' % (indent,name))
		elif k == 'Index':
			print('%s\t\tc->colorIndex =  %s_default;' % (indent,name))
		elif k == 'EdgeFlag':
			print('%s\t\tc->edgeFlag =  %s_default;' % (indent,name))
		else:
			print('%s\t\tc->%s = %s_default;' % (indent,name,name))
	if k == 'EdgeFlag':
		print('%s\t\t__convert_boolean (&c->edgeFlag, v);' % (indent))
		dirtyVar = 'cb->edgeFlag'
	elif k == 'Normal':
		print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_NORMAL][0]), v);' % (indent))
		dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_NORMAL]'
	elif k == 'TexCoord':
		print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_TEX0 + i][0]), v);' % (indent))
		dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_TEX0 + i]'
	elif k == 'Color':
		print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_COLOR0][0]), v);' % (indent))
		dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_COLOR0]'
	elif k == 'Index':
		print('%s\t\tconvert(&(c->colorIndex), v);' % (indent))
		dirtyVar = 'cb->colorIndex'
	elif k == 'SecondaryColor':
		print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_COLOR1][0]), v);' % (indent))
		dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_COLOR1]'
	elif k == 'FogCoord':
		print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_FOG][0]), v);' % (indent))
		dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_FOG]'
	elif k == 'VertexAttrib':
		print('%s\t\tconvert(&(c->vertexAttrib[i][0]), v);' % (indent))
		dirtyVar = 'cb->vertexAttrib[i]'
	else:
		assert 0  # should never get here

	print('%s\t\tDIRTY(%s, nbitID);' % (indent, dirtyVar))

#	if current_fns[k].has_key( 'array' ):
#		print '%s\t\tDIRTY(cb->%s[i], nbitID);' % (indent,name)
#	else:
#		print '%s\t\tDIRTY(cb->%s, nbitID);' % (indent,name)



	print('%s\t\tDIRTY(cb->dirty, nbitID);' % indent)
	print('%s\t}' % indent)
	if 'array' in current_fns[k]:
		print('%s\t%s->ptr[i] = v;' % (indent, name ))
	else:
		print('%s\t%s->ptr = v;' % (indent, name ))
	if 'array' in current_fns[k]:
		print('\t}')
print('}')

print('''

void crStateCurrentRecoverNew(CRContext *g, CRCurrentStatePointers  *current)
{
    const unsigned char *v;
    convert_func convert=NULL;
    CRCurrentState *c;
    CRStateBits *sb;
    CRCurrentBits *cb;

    static const GLfloat vertexAttrib_default[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    GLvertexattrib_p *vertexAttrib = &(current->c.vertexAttrib);

    unsigned int i;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    /* Invalid parameters, do nothing */
    if (!g || !current)
        return;

    c = &(g->current);
    sb = GetCurrentBits(g->pStateTracker);
    cb = &(sb->current);

    /* silence warnings */
    (void) __convert_b1;
    (void) __convert_b2;
    (void) __convert_b3;
    (void) __convert_b4;
    (void) __convert_ui1;
    (void) __convert_ui2;
    (void) __convert_ui3;
    (void) __convert_ui4;
    (void) __convert_l1;
    (void) __convert_l2;
    (void) __convert_l3;
    (void) __convert_l4;
    (void) __convert_us1;
    (void) __convert_us2;
    (void) __convert_us3;
    (void) __convert_us4;
    (void) __convert_ub1;
    (void) __convert_ub2;
    (void) __convert_ub3;
    (void) __convert_ub4;
    (void) __convert_rescale_s1;
    (void) __convert_rescale_s2;
    (void) __convert_rescale_b1;
    (void) __convert_rescale_b2;
    (void) __convert_rescale_ui1;
    (void) __convert_rescale_ui2;
    (void) __convert_rescale_i1;
    (void) __convert_rescale_i2;
    (void) __convert_rescale_us1;
    (void) __convert_rescale_us2;
    (void) __convert_rescale_ub1;
    (void) __convert_rescale_ub2;
    (void) __convert_Ni1;
    (void) __convert_Ni2;
    (void) __convert_Ni3;
    (void) __convert_Ni4;
    (void) __convert_Nb1;
    (void) __convert_Nb2;
    (void) __convert_Nb3;
    (void) __convert_Nb4;
    (void) __convert_Nus1;
    (void) __convert_Nus2;
    (void) __convert_Nus3;
    (void) __convert_Nus4;
    (void) __convert_Nui1;
    (void) __convert_Nui2;
    (void) __convert_Nui3;
    (void) __convert_Nui4;
    (void) __convert_Ns1;
    (void) __convert_Ns2;
    (void) __convert_Ns3;
    (void) __convert_Ns4;
    (void) __convert_Nub1;
    (void) __convert_Nub2;
    (void) __convert_Nub3;
    (void) __convert_Nub4;

    DIRTY(nbitID, g->neg_bitid);

''')

for k in sorted(current_fns_new.keys()):
    print('\t/* %s */' % k)
    print('\tif (current->changed%s)' % k)
    print('\t{')
    print('\t\tv=NULL;')
    name = '%s%s' % (k[:1].lower(),k[1:])

    indent = ""
    if 'array' in current_fns_new[k]:
        print('\t\tfor (i = 0; i < %s; i++)' % current_fns_new[k]['array'])
        print('\t\t{')
        indent += "\t\t"
        print('\t\tif (!(current->changed%s & (1 << i))) continue;' % k)
    for type in current_fns_new[k]['types']:
        if type[0:1] == "N":
            normalized = 1
            type2 = type[1:]
        else:
            normalized = 0
            type2 = type
        for size in current_fns_new[k]['sizes']:
            ptr = '%s->%s%d' % (name, type, size )
            if 'array' in current_fns_new[k]:
                ptr += "[i]"
            print('#ifdef DEBUG_misha')
            print('%s\tif (%s)' % (indent, ptr))
            print('%s\t{' % (indent))
            print('%s\t\tuint32_t *pTst = (uint32_t*)(%s);' % (indent, ptr))
            print('%s\t\t--pTst;' % (indent))
            print('%s\t\tAssert((*pTst) == i);' % (indent))
            print('%s\t}' % (indent))
            print('#endif')
            print('%s\tif (v < %s)' % (indent, ptr))
            print('%s\t{' % indent)
            print('%s\t\tv = %s;' % (indent, ptr))
            if (k == 'Color' or k == 'Normal' or k == 'SecondaryColor' or normalized) and type != 'f' and type != 'd' and type != 'l':
                print('%s\t\tconvert = (convert_func) __convert_rescale_%s%d;' % (indent,type,size))
            else:
                print('%s\t\tconvert = (convert_func) __convert_%s%d;' % (indent,type,size))
            print('%s\t}' % indent)
    print('')
    print('%s\tif (v != NULL) {' % indent)
    if 'array' in current_fns_new[k]:
        if k == 'TexCoord':
            print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_TEX0 + i], %s_default);' % (indent,name))
        else:
            print('%s\t\tCOPY_4V(c->%s[i], %s_default);' % (indent,name,name))
    else:
        if k == 'Normal':
            print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_NORMAL], %s_default);' % (indent,name))
        elif k == 'FogCoord':
            print('%s\t\tc->vertexAttrib[VERT_ATTRIB_FOG][0] =  %s_default;' % (indent,name))
        elif k == 'Color':
            print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_COLOR0], %s_default);' % (indent,name))
        elif k == 'SecondaryColor':
            print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_COLOR1],  %s_default);' % (indent,name))
        elif k == 'TexCoord':
            print('%s\t\tCOPY_4V(c->vertexAttrib[VERT_ATTRIB_TEX0], %s_default);' % (indent,name))
        elif k == 'Index':
            print('%s\t\tc->colorIndex =  %s_default;' % (indent,name))
        elif k == 'EdgeFlag':
            print('%s\t\tc->edgeFlag =  %s_default;' % (indent,name))
        else:
            print('%s\t\tc->%s = %s_default;' % (indent,name,name))
    if k == 'EdgeFlag':
        print('%s\t\t__convert_boolean (&c->edgeFlag, v);' % (indent))
        dirtyVar = 'cb->edgeFlag'
    elif k == 'Normal':
        print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_NORMAL][0]), v);' % (indent))
        dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_NORMAL]'
    elif k == 'TexCoord':
        print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_TEX0 + i][0]), v);' % (indent))
        dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_TEX0 + i]'
    elif k == 'Color':
        print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_COLOR0][0]), v);' % (indent))
        dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_COLOR0]'
    elif k == 'Index':
        print('%s\t\tconvert(&(c->colorIndex), v);' % (indent))
        dirtyVar = 'cb->colorIndex'
    elif k == 'SecondaryColor':
        print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_COLOR1][0]), v);' % (indent))
        dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_COLOR1]'
    elif k == 'FogCoord':
        print('%s\t\tconvert(&(c->vertexAttrib[VERT_ATTRIB_FOG][0]), v);' % (indent))
        dirtyVar = 'cb->vertexAttrib[VERT_ATTRIB_FOG]'
    elif k == 'VertexAttrib':
        print('%s\t\tconvert(&(c->vertexAttrib[i][0]), v);' % (indent))
        dirtyVar = 'cb->vertexAttrib[i]'
    else:
        assert 0  # should never get here

    print('%s\t\tDIRTY(%s, nbitID);' % (indent, dirtyVar))

#   if current_fns_new[k].has_key( 'array' ):
#       print '%s\t\tDIRTY(cb->%s[i], nbitID);' % (indent,name)
#   else:
#       print '%s\t\tDIRTY(cb->%s, nbitID);' % (indent,name)



    print('%s\t\tDIRTY(cb->dirty, nbitID);' % indent)
    print('%s\t}' % indent)
    if 'array' in current_fns_new[k]:
        print('%s\t%s->ptr[i] = v;' % (indent, name ))
    else:
        print('%s\t%s->ptr = v;' % (indent, name ))
    if 'array' in current_fns_new[k]:
        print('\t\t}')
        print('\t\tcurrent->changed%s = 0;' % k)
        print('\t}')
print('\tcrStateResetCurrentPointers(current);')
print('}')
