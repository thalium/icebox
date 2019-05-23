#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: cgiprofiling.py $

"""
Debug - CGI Profiling.
"""

__copyright__ = \
"""
Copyright (C) 2012-2017 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualBox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision: 118412 $"


def profileIt(fnMain, sAppendToElement = 'main', sSort = 'time'):
    """
    Profiles a main() type function call (no parameters, returns int) and
    outputs a hacky HTML section.
    """

    #
    # Execute it.
    #
    import cProfile;
    oProfiler = cProfile.Profile();
    rc = oProfiler.runcall(fnMain);

    #
    # Output HTML to stdout (CGI assumption).
    #
    print('<div id="debug2"><br>\n' # Lazy BR-layouting!!
          '  <h2>Profiler Output</h2>\n'
          '  <pre>');
    try:
        oProfiler.print_stats(sort = sSort);
    except Exception, oXcpt:
        print('<p><pre>%s</pre></p>\n' % (oXcpt,));
    else:
        print('</pre>\n');
    oProfiler = None;
    print('</div>\n');

    #
    # Trick to move the section in under the SQL trace.
    #
    print('<script lang="script/javascript">\n'
          'var oMain = document.getElementById(\'%s\');\n'
          'if (oMain) {\n'
          '    oMain.appendChild(document.getElementById(\'debug2\'));\n'
          '}\n'
          '</script>\n'
          % (sAppendToElement, ) );

    return rc;

