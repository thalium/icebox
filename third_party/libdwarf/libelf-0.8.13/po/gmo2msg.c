/*
 * gmo2msg.c - create X/Open message source file for libelf.
 * Copyright (C) 1996 - 2005 Michael Riepe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef lint
static const char rcsid[] = "@(#) $Id: gmo2msg.c,v 1.11 2008/05/23 08:16:46 michael Exp $";
#endif /* lint */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>

#define DOMAIN	"libelf"

static const char *msgs[] = {
#define __err__(a,b)	b,
#include <errors.h>
#undef __err__
};

int
main(int argc, char **argv) {
    char buf[1024], *lang, *progname, *s;
    unsigned i;
    FILE *fp;

    setlocale(LC_ALL, "");

    if (*argv && (progname = strrchr(argv[0], '/'))) {
	progname++;
    }
    else if (!(progname = *argv)) {
	progname = "gmo2msg";
    }

    if (argc <= 1 || !(lang = argv[1])) {
	fprintf(stderr, "Usage: gmo2msg <language>\n");
	exit(1);
    }

    /*
     * Fool gettext...
     */
    unlink(DOMAIN ".mo");
    unlink("LC_MESSAGES");
    unlink(lang);
    sprintf(buf, "%s.gmo", lang);
    if (link(buf, DOMAIN ".mo") == -1) {
	fprintf(stderr, "Cannot link %s to " DOMAIN ".mo\n", buf);
	perror("");
	exit(1);
    }
    symlink(".", "LC_MESSAGES");
    symlink(".", lang);
    textdomain(DOMAIN);
    getcwd(buf, sizeof(buf));
    bindtextdomain(DOMAIN, buf);

    sprintf(buf, "%s.msg", lang);
    unlink(buf);
    if (!(fp = fopen(buf, "w"))) {
	perror(buf);
	exit(1);
    }

    fprintf(fp, "$set 1 Automatically created from %s.gmo by %s\n", lang, progname);

    /*
     * Translate messages.
     */
    setlocale(LC_MESSAGES, lang);
    if ((s = gettext("")) && (s = strdup(s))) {
	s = strtok(s, "\n");
	while (s) {
	    fprintf(fp, "$ %s\n", s);
	    s = strtok(NULL, "\n");
	}
    }
    /*
     * Assume that messages contain printable ASCII characters ONLY.
     * That means no tabs, linefeeds etc.
     */
    for (i = 0; i < sizeof(msgs)/sizeof(*msgs); i++) {
	s = gettext(msgs[i]);
	if (s != msgs[i] && strcmp(s, msgs[i]) != 0) {
	    fprintf(fp, "$ \n$ Original message: %s\n", msgs[i]);
	    fprintf(fp, "%u %s\n", i + 1, s);
	}
    }
    setlocale(LC_MESSAGES, "");

    if (fclose(fp)) {
	perror("writing output file");
	exit(1);
    }

    /*
     * Cleanup.
     */
    unlink(DOMAIN ".mo");
    unlink("LC_MESSAGES");
    unlink(lang);
    exit(0);
}
