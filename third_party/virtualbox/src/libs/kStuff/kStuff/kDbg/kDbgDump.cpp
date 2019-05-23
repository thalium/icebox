/* $Id: kDbgDump.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbgDump - Debug Info Dumper.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kDbg.h>
#include <string.h>
#include <stdio.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** @name Options
 * @{ */
static int g_fGlobalSyms = 1;
static int g_fPrivateSyms = 1;
static int g_fLineNumbers = 0;
/** @} */


/**
 * Dumps one file.
 *
 * @returns main exit status.
 * @param   pszFile     The file to dump (path to it).
 */
static int DumpFile(const char *pszFile)
{
    PKDBGMOD pDbgMod;
    int rc = kDbgModuleOpen(&pDbgMod, pszFile, NULL);
    if (rc)
    {
        printf("kDbgDump: error: kDbgModuleOpen('%s',) failed with rc=%d.\n", pszFile, rc);
        return 1;
    }



    return 0;
}


/**
 * Prints the version number
 * @return 0
 */
static int ShowVersion()
{
    printf("kDbgDump v0.0.1\n");
    return 0;
}


/**
 * Prints the program syntax.
 *
 * @returns 1
 * @param   argv0   The program name.
 */
static int ShowSyntax(const char *argv0)
{
    ShowVersion();
    printf("syntax: %s [options] <files>\n"
           "\n",
           argv0);
    return 1;
}

int main(int argc, char **argv)
{
    int rcRet = 0;

    /*
     * Parse arguments.
     */
    int fArgsDone = 0;
    for (int i = 1; i < argc; i++)
    {
        const char *psz = argv[i];

        if (!fArgsDone && psz[0] == '-' && psz[1])
        {
            /* convert long option to short. */
            if (*++psz == '-')
            {
                psz++;
                if (!*psz) /* -- */
                {
                    fArgsDone = 1;
                    continue;
                }
                if (!strcmp(psz, "line-numbers"))
                    psz = "l";
                else if (!strcmp(psz, "no-line-numbers"))
                    psz = "L";
                else if (!strcmp(psz, "global-syms")    || !strcmp(psz, "public-syms"))
                    psz = "g";
                else if (!strcmp(psz, "no-global-syms") || !strcmp(psz, "no-public-syms"))
                    psz = "G";
                else if (!strcmp(psz, "privat-syms")    || !strcmp(psz, "local-syms"))
                    psz = "p";
                else if (!strcmp(psz, "no-privat-syms") || !strcmp(psz, "no-local-syms"))
                    psz = "P";
                else if (!strcmp(psz, "version"))
                    psz = "v";
                else if (!strcmp(psz, "help"))
                    psz = "h";
                else
                {
                    fprintf(stderr, "%s: syntax error: unknown option '--%s'\n", argv[0], psz);
                    return 1;
                }
            }

            /* eat short options. */
            while (*psz)
                switch (*psz++)
                {
                    case 'l': g_fLineNumbers = 1; break;
                    case 'L': g_fLineNumbers = 0; break;
                    case 'p': g_fPrivateSyms = 1; break;
                    case 'P': g_fPrivateSyms = 0; break;
                    case 'g': g_fGlobalSyms = 1; break;
                    case 'G': g_fGlobalSyms = 0; break;
                    case '?':
                    case 'H':
                    case 'h': return ShowSyntax(argv[0]);
                    case 'v': return ShowVersion();
                    default:
                        fprintf(stderr, "%s: syntax error: unknown option '-%c'.\n", argv[0], psz[-1]);
                        return 1;
                }
        }
        else
        {
            /* Dump does it's own bitching if something goes wrong. */
            int rc = DumpFile(psz);
            if (rc && !rcRet)
                rc = rcRet;
        }
    }

    return rcRet;
}

