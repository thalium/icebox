/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "cr_process.h"
#include "cr_string.h"
#include "cr_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#ifndef WINDOWS
#include <unistd.h>
# ifdef VBOX
#  include <string.h>
# endif
#else
#pragma warning ( disable : 4127 )
#define snprintf _snprintf
#endif

/*
 * Return the name of the running process.
 * name[0] will be zero if anything goes wrong.
 */
void crGetProcName( char *name, int maxLen )
{
#ifdef WINDOWS
	char command[1000];
	int c = 0;

	*name = 0;
	
	if (!GetModuleFileName( NULL, command, maxLen ))
		return;

	while (1) {
		/* crude mechanism to blank out the backslashes
		 * in the Windows filename and recover the actual
		 * program name to return */
		if (crStrstr(command, "\\")) {
			crStrncpy(name, command+c+1, maxLen);
			command[c] = 32;
			c++;
		}
		else
			break;
	}
#else
#ifdef VBOX
    const char *pszExecName, *pszProgName;
#  ifdef SunOS
    pszExecName = getexecname();
#  elif defined(DARWIN)
    pszExecName = getprogname();
#  else
    extern const char *__progname;
    pszExecName = __progname;
#  endif
    if (!pszExecName)
        pszExecName = "<unknown>";
    pszProgName = strrchr(pszExecName, '/');
    if (pszProgName && *(pszProgName + 1))
        pszProgName++;
    else
        pszProgName = pszExecName;
    strncpy(name, pszProgName, maxLen);
    name[maxLen - 1] = '\0';
# else
	/* Unix:
	 * Call getpid() to get our process ID.
	 * Then use system() to write the output of 'ps' to a temp file.
	 * Read/scan the temp file to map the process ID to process name.
	 * I'd love to find a better solution! (BrianP)
	 */
	FILE *f;
	pid_t pid = getpid();
	char *tmp, command[1000];

	/* init to NULL in case of early return */
	*name = 0;

	/* get a temporary file name */
	tmp = tmpnam(NULL);
	if (!tmp)
		return;
	/* pipe output of ps to temp file */
#ifndef SunOS
	sprintf(command, "ps > %s", tmp);
#else
	sprintf(command, "ps -e -o 'pid tty time comm'> %s", tmp);
#endif
	system(command);

	/* open/scan temp file */
	f = fopen(tmp, "r");
	if (f) {
		char buffer[1000], cmd[1000], *psz, *pname;
		while (!feof(f)) {
			int id;
			fgets(buffer, 999, f);
			sscanf(buffer, "%d %*s %*s %999s", &id, cmd);
			if (id == pid) {
				for (pname=psz=&cmd[0]; *psz!=0; psz++)
				{
					switch (*psz)
					{
						case '/':
						pname = psz+1;
						break;
					}
				}
				crStrncpy(name, pname, maxLen);
				break;
			}
		}
		fclose(f);
	}
	remove(tmp);
# endif
#endif
}


/**
 * Return current process ID number.
 */
CRpid crGetPID(void)
{
#ifdef WINDOWS 
  /*return _getpid();*/
  return GetCurrentProcess();
#else
  return getpid();
#endif
}


#if 0
/* simple test harness */
int main(int argc, char **argv)
{
   char name[100];
   printf("argv[0] = %s\n", argv[0]);

   crGetProcName(name, 100);
   printf("crGetProcName returned %s\n", name);

   return 0;
}
#endif
