#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: vcs_import.py $
# pylint: disable=C0301

"""
Cron job for importing revision history for a repository.
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
import sys;
import os;
from optparse import OptionParser;
import xml.etree.ElementTree as ET;

# Add Test Manager's modules path
g_ksTestManagerDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksTestManagerDir);

# Test Manager imports
from testmanager.core.db            import TMDatabaseConnection;
from testmanager.core.vcsrevisions  import VcsRevisionData, VcsRevisionLogic;
from common                         import utils;

class VcsImport(object): # pylint: disable=R0903
    """
    Imports revision history from a VSC into the Test Manager database.
    """

    def __init__(self):
        """
        Parse command line.
        """

        oParser = OptionParser()
        oParser.add_option('-e', '--extra-option', dest = 'asExtraOptions', action = 'append',
                           help = 'Adds a extra option to the command retrieving the log.');
        oParser.add_option('-f', '--full', dest = 'fFull', action = 'store_true',
                           help = 'Full revision history import.');
        oParser.add_option('-q', '--quiet', dest = 'fQuiet', action = 'store_true',
                           help = 'Quiet execution');
        oParser.add_option('-R', '--repository', dest = 'sRepository', metavar = '<repository>',
                           help = 'Version control repository name.');
        oParser.add_option('-s', '--start-revision', dest = 'iStartRevision', metavar = 'start-revision',
                           type = "int", default = 0,
                           help = 'The revision to start at when doing a full import.');
        oParser.add_option('-t', '--type', dest = 'sType', metavar = '<type>',
                           help = 'The VCS type (default: svn)', choices = [ 'svn', ], default = 'svn');
        oParser.add_option('-u', '--url', dest = 'sUrl', metavar = '<url>',
                           help = 'The VCS URL');

        (self.oConfig, _) = oParser.parse_args();

        # Check command line
        asMissing = [];
        if self.oConfig.sUrl is None:               asMissing.append('--url');
        if self.oConfig.sRepository is None:        asMissing.append('--repository');
        if asMissing:
            sys.stderr.write('syntax error: Missing: %s\n' % (asMissing,));
            sys.exit(1);

        assert self.oConfig.sType == 'svn';

    def main(self):
        """
        Main function.
        """
        oDb = TMDatabaseConnection();
        oLogic = VcsRevisionLogic(oDb);

        # Where to start.
        iStartRev = 0;
        if not self.oConfig.fFull:
            iStartRev = oLogic.getLastRevision(self.oConfig.sRepository);
        if iStartRev == 0:
            iStartRev = self.oConfig.iStartRevision;

        # Construct a command line.
        os.environ['LC_ALL'] = 'en_US.utf-8';
        asArgs = [
            'svn',
            'log',
            '--xml',
            '--revision', str(iStartRev) + ':HEAD',
        ];
        if self.oConfig.asExtraOptions is not None:
            asArgs.extend(self.oConfig.asExtraOptions);
        asArgs.append(self.oConfig.sUrl);
        if not self.oConfig.fQuiet:
            print 'Executing: %s' % (asArgs,);
        sLogXml = utils.processOutputChecked(asArgs);

        # Parse the XML and add the entries to the database.
        oParser = ET.XMLParser(target = ET.TreeBuilder(), encoding = 'utf-8');
        oParser.feed(sLogXml);
        oRoot = oParser.close();

        for oLogEntry in oRoot.findall('logentry'):
            iRevision = int(oLogEntry.get('revision'));
            sAuthor  = oLogEntry.findtext('author').strip();
            sDate    = oLogEntry.findtext('date').strip();
            sMessage = oLogEntry.findtext('msg', '').strip();
            if sMessage == '':
                sMessage = ' ';
            if not self.oConfig.fQuiet:
                print 'sDate=%s iRev=%u sAuthor=%s sMsg[%s]=%s' % (sDate, iRevision, sAuthor, type(sMessage).__name__, sMessage);
            oData = VcsRevisionData().initFromValues(self.oConfig.sRepository, iRevision, sDate, sAuthor, sMessage);
            oLogic.addVcsRevision(oData);
        oDb.commit();

        oDb.close();
        return 0;

if __name__ == '__main__':
    sys.exit(VcsImport().main());

