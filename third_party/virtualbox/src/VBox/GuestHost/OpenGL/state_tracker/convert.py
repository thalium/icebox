# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys

# Two different paths to the packer and opengl_stub directories since this
# script will be called from both cr/state_tracker/ and cr/spu/tilesort/.
sys.path.append( '../packer' )
sys.path.append( '../../packer' )
sys.path.append( '../glapi_parser' )
sys.path.append( '../../glapi_parser' )
from pack_currenttypes import *
import apiutil

apiutil.CopyrightC()

print('''
#include "state/cr_statetypes.h"

static double __read_double( const void *src )
{
    const unsigned int *ui = (const unsigned int *) src;
    double d;
    ((unsigned int *) &d)[0] = ui[0];
    ((unsigned int *) &d)[1] = ui[1];
    return d;
}
''')

for k in sorted(gltypes.keys()):
	for i in range(1,5):
		print('static void __convert_%s%d (GLfloat *dst, const %s *src) {' % (k,i,gltypes[k]['type']))
		if k == 'd':
		  for j in range(i-1):
		    print('\t*dst++ = (GLfloat) __read_double(src++);')
		  print('\t*dst = (GLfloat) __read_double(src);')
		else:
		  for j in range(i-1):
		    print('\t*dst++ = (GLfloat) *src++;')
		  print('\t*dst = (GLfloat) *src;')
		print('}\n')

scale = {
	'ub' : 'CR_MAXUBYTE',
	'b'  : 'CR_MAXBYTE',
	'us' : 'CR_MAXUSHORT',
	's'  : 'CR_MAXSHORT',
	'ui' : 'CR_MAXUINT',
	'i'  : 'CR_MAXINT',
	'f'  : '',
	'd'  : ''
}

for k in sorted(gltypes.keys()):
	if k != 'f' and k != 'd' and k != 'l':
		if k[0:1] == "N":
			k2 = k[1:]
		else:
			k2 = k
		for i in range(1,5):
			print('static void __convert_rescale_%s%d (GLfloat *dst, const %s *src) {' % (k,i,gltypes[k2]['type']))
			for j in range(i-1):
				print('\t*dst++ = ((GLfloat) *src++) / %s;' % scale[k2])
			print('\t*dst = ((GLfloat) *src) / %s;' % scale[k2])
			print('}\n')

print('''

static void __convert_boolean (GLboolean *dst, const GLboolean *src) {
	*dst = *src;
}
''')
