# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

# This file is imported by several other Python scripts

current_fns = {
	'Color': {
		'types': ['b','ub','s','us','i','ui','f','d'],
		'sizes': [3,4],
		'default': [0,0,0,1],
		'members': ['[0]', '[1]', '[2]', '[3]']
	},
	'SecondaryColor': {
		'types': ['b','ub','s','us','i','ui','f','d'],
		'sizes': [3],
		'default': [0,0,0],
		'members': ['[0]', '[1]', '[2]']
	},
	'Normal': {
		'types': ['b','s','i','f','d'],
		'sizes': [3],
		'default': [0,0,0],
		'members': ['[0]', '[1]', '[2]']
	},
	'TexCoord': {
		'types': ['s','i','f','d'],
		'sizes': [1,2,3,4],
		'default': [0,0,0,1],
		'members': ['[0]', '[1]', '[2]', '[3]'],
		'array': 'CR_MAX_TEXTURE_UNITS'
	},
	'EdgeFlag': {
		'types': ['l'],
		'sizes': [1],
		'default': [1],
		'members': ['[0]']
	},
	'Index': {
		'types': ['ub','s','i','f','d'],
		'sizes': [1],
		'default': [0],
		'members': ['[0]']
	},
	'VertexAttrib': {
		'types': ['s','f','d','b','i','ub','ui','us','Nub','Nus','Nui','Nb','Ns','Ni'],
		'sizes': [1,2,3,4],
		'default': [0,0,0,1],
		'members': ['x', 'y', 'z', 'w'],
		'array': 'CR_MAX_VERTEX_ATTRIBS'
	},
	'FogCoord': {
		'types': ['f','d'],
		'sizes': [1],
		'default': [0],
		'members': []
	},
}

current_fns_new = {
    'VertexAttrib': {
        'types': ['s','f','d','b','i','ub','ui','us','Nub','Nus','Nui','Nb','Ns','Ni'],
        'sizes': [1,2,3,4],
        'default': [0,0,0,1],
        'members': ['x', 'y', 'z', 'w'],
        'array': 'CR_MAX_VERTEX_ATTRIBS'
    },
}

current_vtx = {
	'Vertex': {
		'types': ['s','i','f','d'],
		'sizes': [2,3,4],
		'default': [0,0,0,1],
		'members': ['x', 'y', 'z', 'w']
	}
}

gltypes = {
	'l': {
		'type': 'GLboolean',
		'size': 1
	},
	'b': {
		'type': 'GLbyte',
		'size': 1
	},
	'ub': {
		'type': 'GLubyte',
		'size': 1
	},
	's': {
		'type': 'GLshort',
		'size': 2
	},
	'us': {
		'type': 'GLushort',
		'size': 2
	},
	'i': {
		'type': 'GLint',
		'size': 4
	},
	'ui': {
		'type': 'GLuint',
		'size': 4
	},
	'f': {
		'type': 'GLfloat',
		'size': 4
	},
	'd': {
		'type': 'GLdouble',
		'size': 8
	},

	'Nb': {
		'type': 'GLbyte',
		'size': 1
	},
	'Nub': {
		'type': 'GLubyte',
		'size': 1
	},
	'Ns': {
		'type': 'GLshort',
		'size': 2
	},
	'Nus': {
		'type': 'GLushort',
		'size': 2
	},
	'Ni': {
		'type': 'GLint',
		'size': 4
	},
	'Nui': {
		'type': 'GLuint',
		'size': 4
	}
}
