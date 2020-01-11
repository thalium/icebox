/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"

void crUnpackMultMatrixd(PCrUnpackerState pState)
{
	GLdouble m[16];
    CHECK_BUFFER_SIZE_STATIC(pState, sizeof(m));
	crMemcpy( m, DATA_POINTER(pState, 0, GLdouble ), sizeof(m) );

	pState->pDispatchTbl->MultMatrixd( m );
	INCR_DATA_PTR(pState, 16*sizeof( GLdouble ) );
}

void crUnpackMultMatrixf(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 16 * sizeof(GLfloat));

	GLfloat *m = DATA_POINTER(pState, 0, GLfloat );

	pState->pDispatchTbl->MultMatrixf( m );
	INCR_DATA_PTR(pState, 16*sizeof( GLfloat ) );
}

void crUnpackLoadMatrixd(PCrUnpackerState pState)
{
	GLdouble m[16];
    CHECK_BUFFER_SIZE_STATIC(pState, sizeof(m));
	crMemcpy( m, DATA_POINTER(pState, 0, GLdouble ), sizeof(m) );

	pState->pDispatchTbl->LoadMatrixd( m );
	INCR_DATA_PTR(pState, 16*sizeof( GLdouble ) );
}

void crUnpackLoadMatrixf(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 16 * sizeof(GLfloat));

	GLfloat *m = DATA_POINTER(pState, 0, GLfloat );

	pState->pDispatchTbl->LoadMatrixf( m );
	INCR_DATA_PTR(pState, 16*sizeof( GLfloat ) );
}

void crUnpackExtendMultTransposeMatrixdARB(PCrUnpackerState pState)
{
	GLdouble m[16];
    CHECK_BUFFER_SIZE_STATIC(pState, 8 + sizeof(m));
	crMemcpy( m, DATA_POINTER(pState, 8, GLdouble ), sizeof(m) );

	pState->pDispatchTbl->MultTransposeMatrixdARB( m );
}

void crUnpackExtendMultTransposeMatrixfARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8 + 16 * sizeof(GLfloat));
	GLfloat *m = DATA_POINTER(pState, 8, GLfloat );

	pState->pDispatchTbl->MultTransposeMatrixfARB( m );
}

void crUnpackExtendLoadTransposeMatrixdARB(PCrUnpackerState pState)
{
	GLdouble m[16];
    CHECK_BUFFER_SIZE_STATIC(pState, 8 + sizeof(m));
	crMemcpy( m, DATA_POINTER(pState, 8, GLdouble ), sizeof(m) );

	pState->pDispatchTbl->LoadTransposeMatrixdARB( m );
}

void crUnpackExtendLoadTransposeMatrixfARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 8 + 16 * sizeof(GLfloat));
	GLfloat *m = DATA_POINTER(pState, 8, GLfloat );

	pState->pDispatchTbl->LoadTransposeMatrixfARB( m );
}
