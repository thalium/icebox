/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"

static GLuint __evaluator_components( GLenum target )
{
   switch (target) {
      case GL_MAP1_VERTEX_3:		return 3;
      case GL_MAP1_VERTEX_4:		return 4;
      case GL_MAP1_INDEX:		return 1;
      case GL_MAP1_COLOR_4:		return 4;
      case GL_MAP1_NORMAL:		return 3;
      case GL_MAP1_TEXTURE_COORD_1:	return 1;
      case GL_MAP1_TEXTURE_COORD_2:	return 2;
      case GL_MAP1_TEXTURE_COORD_3:	return 3;
      case GL_MAP1_TEXTURE_COORD_4:	return 4;
      case GL_MAP2_VERTEX_3:		return 3;
      case GL_MAP2_VERTEX_4:		return 4;
      case GL_MAP2_INDEX:		return 1;
      case GL_MAP2_COLOR_4:		return 4;
      case GL_MAP2_NORMAL:		return 3;
      case GL_MAP2_TEXTURE_COORD_1:	return 1;
      case GL_MAP2_TEXTURE_COORD_2:	return 2;
      case GL_MAP2_TEXTURE_COORD_3:	return 3;
      case GL_MAP2_TEXTURE_COORD_4:	return 4;
      default:				return 0;
   }
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetMapdv( GLenum target, GLenum query, GLdouble *v )
{
	GLdouble *coeffs = NULL;
	GLdouble order[2];
	GLdouble domain[4];
	GLdouble *retptr = NULL;
	GLint tempOrder[2];
	int dimension = 0;
	unsigned int size = sizeof(GLdouble);
	int evalcomp = __evaluator_components(target);

	(void) v;

	switch( target )
	{
		case GL_MAP1_COLOR_4:
		case GL_MAP1_INDEX:
		case GL_MAP1_NORMAL:
		case GL_MAP1_TEXTURE_COORD_1:
		case GL_MAP1_TEXTURE_COORD_2:
		case GL_MAP1_TEXTURE_COORD_3:
		case GL_MAP1_TEXTURE_COORD_4:
		case GL_MAP1_VERTEX_3:
		case GL_MAP1_VERTEX_4:
			dimension = 1; 
			break;
		case GL_MAP2_COLOR_4:
		case GL_MAP2_INDEX:
		case GL_MAP2_NORMAL:
		case GL_MAP2_TEXTURE_COORD_1:
		case GL_MAP2_TEXTURE_COORD_2:
		case GL_MAP2_TEXTURE_COORD_3:
		case GL_MAP2_TEXTURE_COORD_4:
		case GL_MAP2_VERTEX_3:
		case GL_MAP2_VERTEX_4: 
			dimension = 2;
			break;
		default:
			crError( "Bad target in crServerDispatchGetMapdv: %d", target );
			break;
	}

	switch(query)
	{
		case GL_ORDER:
			cr_server.head_spu->dispatch_table.GetMapdv( target, query, order );
			retptr = &(order[0]);
			size *= dimension;
			break;
		case GL_DOMAIN:
			cr_server.head_spu->dispatch_table.GetMapdv( target, query, domain );
			retptr = &(domain[0]);
			size *= dimension * 2;
			break;
		case GL_COEFF:
			cr_server.head_spu->dispatch_table.GetMapiv( target, GL_ORDER, tempOrder );
			size *= evalcomp * tempOrder[0];
			if (dimension == 2)
				size *= tempOrder[1];
			coeffs = (GLdouble *) crAlloc( size );
			cr_server.head_spu->dispatch_table.GetMapdv( target, query, coeffs );
			retptr = coeffs;
			break;
		default:
			crError( "Bad query in crServerDispatchGetMapdv: %d", query );
			break;
	}
			
	crServerReturnValue( retptr, size );
	if (query == GL_COEFF)
	{
		crFree(coeffs);
	}
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetMapfv( GLenum target, GLenum query, GLfloat *v )
{
	GLfloat *coeffs = NULL;
	GLfloat order[2];
	GLfloat domain[4];
	GLfloat *retptr = NULL;
	GLint tempOrder[2];
	int dimension = 0;
	unsigned int size = sizeof(GLfloat);
	int evalcomp = __evaluator_components(target);

	(void) v;

	switch( target )
	{
		case GL_MAP1_COLOR_4:
		case GL_MAP1_INDEX:
		case GL_MAP1_NORMAL:
		case GL_MAP1_TEXTURE_COORD_1:
		case GL_MAP1_TEXTURE_COORD_2:
		case GL_MAP1_TEXTURE_COORD_3:
		case GL_MAP1_TEXTURE_COORD_4:
		case GL_MAP1_VERTEX_3:
		case GL_MAP1_VERTEX_4:
			dimension = 1;
			break;
		case GL_MAP2_COLOR_4:
		case GL_MAP2_INDEX:
		case GL_MAP2_NORMAL:
		case GL_MAP2_TEXTURE_COORD_1:
		case GL_MAP2_TEXTURE_COORD_2:
		case GL_MAP2_TEXTURE_COORD_3:
		case GL_MAP2_TEXTURE_COORD_4:
		case GL_MAP2_VERTEX_3:
		case GL_MAP2_VERTEX_4: 
			dimension = 2;
			break;
		default:
			crError( "Bad target in crServerDispatchGetMapfv: %d", target );
			break;
	}

	switch(query)
	{
		case GL_ORDER:
			cr_server.head_spu->dispatch_table.GetMapfv( target, query, order );
			retptr = &(order[0]);
			size *= dimension;
			break;
		case GL_DOMAIN:
			cr_server.head_spu->dispatch_table.GetMapfv( target, query, domain );
			retptr = &(domain[0]);
			size *= dimension * 2;
			break;
		case GL_COEFF:
			cr_server.head_spu->dispatch_table.GetMapiv( target, GL_ORDER, tempOrder );
			size *= evalcomp * tempOrder[0];
			if (dimension == 2)
				size *= tempOrder[1];
			coeffs = (GLfloat *) crAlloc( size );
			cr_server.head_spu->dispatch_table.GetMapfv( target, query, coeffs );
			retptr = coeffs;
			break;
		default:
			crError( "Bad query in crServerDispatchGetMapfv: %d", query );
			break;
	}
			
	crServerReturnValue( retptr, size );
	if (query == GL_COEFF)
	{
		crFree(coeffs);
	}
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetMapiv( GLenum target, GLenum query, GLint *v )
{
	GLint *coeffs = NULL;
	GLint order[2];
	GLint domain[4];
	GLint *retptr = NULL;
	GLint tempOrder[2];
	int dimension = 0;
	unsigned int size = sizeof(GLint);
	int evalcomp = __evaluator_components(target);

	(void) v;

	switch( target )
	{
		case GL_MAP1_COLOR_4:
		case GL_MAP1_INDEX:
		case GL_MAP1_NORMAL:
		case GL_MAP1_TEXTURE_COORD_1:
		case GL_MAP1_TEXTURE_COORD_2:
		case GL_MAP1_TEXTURE_COORD_3:
		case GL_MAP1_TEXTURE_COORD_4:
		case GL_MAP1_VERTEX_3:
		case GL_MAP1_VERTEX_4:
			dimension = 1; 
			break;
		case GL_MAP2_COLOR_4:
		case GL_MAP2_INDEX:
		case GL_MAP2_NORMAL:
		case GL_MAP2_TEXTURE_COORD_1:
		case GL_MAP2_TEXTURE_COORD_2:
		case GL_MAP2_TEXTURE_COORD_3:
		case GL_MAP2_TEXTURE_COORD_4:
		case GL_MAP2_VERTEX_3:
		case GL_MAP2_VERTEX_4: 
			dimension = 2;
			break;
		default:
			crError( "Bad target in crServerDispatchGetMapiv: %d", target );
			break;
	}

	switch(query)
	{
		case GL_ORDER:
			cr_server.head_spu->dispatch_table.GetMapiv( target, query, order );
			retptr = &(order[0]);
			size *= dimension;
			break;
		case GL_DOMAIN:
			cr_server.head_spu->dispatch_table.GetMapiv( target, query, domain );
			retptr = &(domain[0]);
			size *= dimension * 2;
			break;
		case GL_COEFF:
			cr_server.head_spu->dispatch_table.GetMapiv( target, GL_ORDER, tempOrder );
			size *= evalcomp * tempOrder[0];
			if (dimension == 2)
				size *= tempOrder[1];
			coeffs = (GLint *) crAlloc( size );
			cr_server.head_spu->dispatch_table.GetMapiv( target, query, coeffs );
			retptr = coeffs;
			break;
		default:
			crError( "Bad query in crServerDispatchGetMapiv: %d", query );
			break;
	}
			
	crServerReturnValue( retptr, size );
	if (query == GL_COEFF)
	{
		crFree(coeffs);
	}
}

