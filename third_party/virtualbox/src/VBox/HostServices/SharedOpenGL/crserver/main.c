/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_server.h"

/**
 * \mainpage Crserver 
 *
 * \section CrserverIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The crserver module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */

int main( int argc, char *argv[] )
{
	return CRServerMain( argc, argv );
}
