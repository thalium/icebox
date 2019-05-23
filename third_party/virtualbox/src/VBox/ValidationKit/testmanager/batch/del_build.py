#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: del_build.py $
# pylint: disable=C0301

"""
Interface used by the tinderbox server side software to mark build binaries
deleted.
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

# Standard python imports
import sys
import os
from optparse import OptionParser

# Add Test Manager's modules path
g_ksTestManagerDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(g_ksTestManagerDir)

# Test Manager imports
from testmanager.core.db    import TMDatabaseConnection
from testmanager.core.build import BuildLogic


def markBuildsDeleted():
    """
    Marks the builds using the specified binaries as deleted.
    """

    oParser = OptionParser()
    oParser.add_option('-q', '--quiet', dest='fQuiet', action='store_true',
                       help='Quiet execution');

    (oConfig, asArgs) = oParser.parse_args()
    if not asArgs:
        if not oConfig.fQuiet:
            sys.stderr.write('syntax error: No builds binaries specified\n');
        return 1;


    oDb = TMDatabaseConnection()
    oLogic = BuildLogic(oDb)

    for sBuildBin in asArgs:
        try:
            cBuilds = oLogic.markDeletedByBinaries(sBuildBin, fCommit = True)
        except:
            if oConfig.fQuiet:
                sys.exit(1);
            raise;
        else:
            if not oConfig.fQuiet:
                print "del_build.py: Marked %u builds associated with '%s' as deleted." % (cBuilds, sBuildBin,);

    oDb.close()
    return 0;

if __name__ == '__main__':
    sys.exit(markBuildsDeleted())

