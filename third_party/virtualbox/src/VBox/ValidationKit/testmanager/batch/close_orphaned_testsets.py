#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: close_orphaned_testsets.py $
# pylint: disable=C0301

"""
Maintenance tool for closing orphaned testsets.
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
from testmanager.core.db        import TMDatabaseConnection
from testmanager.core.testset   import TestSetLogic;


class CloseOrphanedTestSets(object):
    """
    Finds and closes orphaned testsets.
    """

    def __init__(self):
        """
        Parse command line
        """
        oParser = OptionParser();
        oParser.add_option('-d', '--just-do-it', dest='fJustDoIt', action='store_true',
                           help='Do the database changes.');


        (self.oConfig, _) = oParser.parse_args();


    def main(self):
        """ Main method. """
        oDb = TMDatabaseConnection();

        # Get a list of orphans.
        oLogic = TestSetLogic(oDb);
        aoOrphans = oLogic.fetchOrphaned();
        if aoOrphans:
            # Complete them.
            if self.oConfig.fJustDoIt:
                print 'Completing %u test sets as abandoned:' % (len(aoOrphans),);
                for oTestSet in aoOrphans:
                    print '#%-7u: idTestBox=%-3u tsCreated=%s tsDone=%s' \
                        % (oTestSet.idTestSet, oTestSet.idTestBox, oTestSet.tsCreated, oTestSet.tsDone);
                    oLogic.completeAsAbandoned(oTestSet.idTestSet);
                print 'Committing...';
                oDb.commit();
            else:
                for oTestSet in aoOrphans:
                    print '#%-7u: idTestBox=%-3u tsCreated=%s tsDone=%s' \
                        % (oTestSet.idTestSet, oTestSet.idTestBox, oTestSet.tsCreated, oTestSet.tsDone);
                print 'Not completing any testsets without seeing the --just-do-it option.'
        else:
            print 'No orphaned test sets.\n'
        return 0;


if __name__ == '__main__':
    sys.exit(CloseOrphanedTestSets().main())

