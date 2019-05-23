
#include "server_dispatch.h"
#include "server.h"


/**
 * All glWindowPos commands go through here.
 */
static void crServerWindowPos( GLfloat x, GLfloat y, GLfloat z )
{
	crStateWindowPos3fARB(x, y, z);
	cr_server.head_spu->dispatch_table.WindowPos3fARB(x, y, z);
}


void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2dARB( GLdouble x, GLdouble y )
{
	crServerWindowPos((GLfloat) x, (GLfloat) y, 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2dvARB( const GLdouble * v )
{
	crServerWindowPos((GLfloat) v[0], (GLfloat) v[1], 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2fARB( GLfloat x, GLfloat y )
{
	crServerWindowPos(x, y, 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2fvARB( const GLfloat * v )
{
	crServerWindowPos(v[0], v[1], 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2iARB( GLint x, GLint y )
{
	crServerWindowPos((GLfloat)x, (GLfloat)y, 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2ivARB( const GLint * v )
{
	crServerWindowPos((GLfloat)v[0], (GLfloat)v[1], 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2sARB( GLshort x, GLshort y )
{
	crServerWindowPos(x, y, 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos2svARB( const GLshort * v )
{
	crServerWindowPos(v[0], v[1], 0.0F);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3dARB( GLdouble x, GLdouble y, GLdouble z )
{
	crServerWindowPos((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3dvARB( const GLdouble * v )
{
	crServerWindowPos((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3fARB( GLfloat x, GLfloat y, GLfloat z )
{
	crServerWindowPos(x, y, z);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3fvARB( const GLfloat * v )
{
	crServerWindowPos(v[0], v[1], v[2]);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3iARB( GLint x, GLint y, GLint z )
{
	crServerWindowPos((GLfloat)x,(GLfloat)y, (GLfloat)z);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3ivARB( const GLint * v )
{
	crServerWindowPos((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3sARB( GLshort x, GLshort y, GLshort z )
{
	crServerWindowPos(x, y, z);
}

void SERVER_DISPATCH_APIENTRY crServerDispatchWindowPos3svARB( const GLshort * v )
{
	crServerWindowPos(v[0], v[1], v[2]);
}

