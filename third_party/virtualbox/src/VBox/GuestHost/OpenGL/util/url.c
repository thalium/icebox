/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "cr_string.h"
#include "cr_url.h"
#include "cr_error.h"

static int is_digit_string( const char *s )
{
     if (!isdigit( (int) *s))
	{
		return 0;
	}

     while (*s && isdigit ( (int) *s))
	{
		s++;
	}

	return ( *s == 0 );
}

int crParseURL( const char *url, char *protocol, char *hostname,
				unsigned short *port, unsigned short default_port )
{
	const char *temp, *temp2;

	/* pull off the protocol */
	temp = crStrstr( url, "://" );
	if ( temp == NULL && protocol != NULL )
	{
		crStrcpy( protocol, "tcpip" );
		temp = url;
	}
	else 
	{
		if (protocol != NULL) {
			int len = temp - url;
			crStrncpy( protocol, url, len );
			protocol[len] = 0;
		}
		temp += 3;
	}

	/* handle a trailing :<digits> to specify the port */

	/* there might be a filename here */
	temp2 = crStrrchr( temp, '/' );
	if ( temp2 == NULL )
	{
		temp2 = crStrrchr( temp, '\\' );
	}
	if ( temp2 == NULL )
	{
		temp2 = temp;
	}

	temp2 = crStrrchr( temp2, ':' );
	if ( temp2 )
	{
		if (hostname != NULL) {
			int len = temp2 - temp;
			crStrncpy( hostname, temp, len );
			hostname[len] = 0;
		}
		temp2++;
		if ( !is_digit_string( temp2 ) )
			goto bad_url;

		if (port != NULL)
			*port = (unsigned short) atoi( temp2 );
	}
	else
	{
		if (hostname != NULL)
			crStrcpy( hostname, temp );
		if (port != NULL)
		*port = default_port;
	}

	return 1;

 bad_url:
	crWarning( "URL: expected <protocol>://"
				   "<destination>[:<port>], what is \"%s\"?", url );
	return 0;
}
