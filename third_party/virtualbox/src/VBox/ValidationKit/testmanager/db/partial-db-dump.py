#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: partial-db-dump.py $
# pylint: disable=C0301

"""
Utility for dumping the last X days of data.
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
import zipfile;
from optparse import OptionParser;
import xml.etree.ElementTree as ET;

# Add Test Manager's modules path
g_ksTestManagerDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksTestManagerDir);

# Test Manager imports
from testmanager.core.db            import TMDatabaseConnection;
from common                         import utils;


class PartialDbDump(object): # pylint: disable=R0903
    """
    Dumps or loads the last X days of database data.

    This is a useful tool when hacking on the test manager locally.  You can get
    a small sample from the last few days from the production test manager server
    without spending hours dumping, downloading, and loading the whole database
    (because it is gigantic).

    """

    def __init__(self):
        """
        Parse command line.
        """

        oParser = OptionParser()
        oParser.add_option('-q', '--quiet', dest = 'fQuiet', action = 'store_true',
                           help = 'Quiet execution');
        oParser.add_option('-f', '--filename', dest = 'sFilename', metavar = '<filename>',
                           default = 'partial-db-dump.zip', help = 'The name of the partial database zip file to write/load.');

        oParser.add_option('-t', '--tmp-file', dest = 'sTempFile', metavar = '<temp-file>',
                           default = '/tmp/tm-partial-db-dump.pgtxt',
                           help = 'Name of temporary file for duping tables. Must be absolute');
        oParser.add_option('--days-to-dump', dest = 'cDays', metavar = '<days>', type = 'int', default = 14,
                           help = 'How many days to dump (counting backward from current date).');
        oParser.add_option('--load-dump-into-database', dest = 'fLoadDumpIntoDatabase', action = 'store_true',
                           default = False, help = 'For loading instead of dumping.');

        (self.oConfig, _) = oParser.parse_args();


    ##
    # Tables dumped in full because they're either needed in full or they normally
    # aren't large enough to bother reducing.
    kasTablesToDumpInFull = [
        'Users',
        'BuildBlacklist',
        'BuildCategories',
        'BuildSources',
        'FailureCategories',
        'FailureReasons',
        'GlobalResources',
        'TestBoxStrTab',
        'Testcases',
        'TestcaseArgs',
        'TestcaseDeps',
        'TestcaseGlobalRsrcDeps',
        'TestGroups',
        'TestGroupMembers',
        'SchedGroups',
        'SchedGroupMembers',            # ?
        'SchedQueues',
        'Builds',                       # ??
        'VcsRevisions',                 # ?
        'TestResultStrTab',             # 36K rows, never mind complicated then.
    ];

    ##
    # Tables where we only dump partial info (the TestResult* tables are rather
    # gigantic).
    kasTablesToPartiallyDump = [
        'TestBoxes',                    # 2016-05-25: ca.  641 MB
        'TestSets',                     # 2016-05-25: ca.  525 MB
        'TestResults',                  # 2016-05-25: ca.   13 GB
        'TestResultFiles',              # 2016-05-25: ca.   87 MB
        'TestResultMsgs',               # 2016-05-25: ca.   29 MB
        'TestResultValues',             # 2016-05-25: ca. 3728 MB
        'TestResultFailures',
        'SystemLog',
    ];

    def _doCopyTo(self, sTable, oZipFile, oDb, sSql, aoArgs = None):
        """ Does one COPY TO job. """
        print 'Dumping %s...' % (sTable,);

        if aoArgs is not None:
            sSql = oDb.formatBindArgs(sSql, aoArgs);

        oFile = open(self.oConfig.sTempFile, 'w');
        oDb.copyExpert(sSql, oFile);
        cRows = oDb.getRowCount();
        oFile.close();
        print '... %s rows.' % (cRows,);

        oZipFile.write(self.oConfig.sTempFile, sTable);
        return True;

    def _doDump(self, oDb):
        """ Does the dumping of the database. """

        oZipFile = zipfile.ZipFile(self.oConfig.sFilename, 'w', zipfile.ZIP_DEFLATED);

        oDb.begin();

        # Dumping full tables is simple.
        for sTable in self.kasTablesToDumpInFull:
            self._doCopyTo(sTable, oZipFile, oDb, 'COPY ' + sTable + ' TO STDOUT WITH (FORMAT TEXT)');

        # Figure out how far back we need to go.
        oDb.execute('SELECT CURRENT_TIMESTAMP - INTERVAL \'%s days\'' % (self.oConfig.cDays,));
        tsEffective = oDb.fetchOne()[0];
        oDb.execute('SELECT CURRENT_TIMESTAMP - INTERVAL \'%s days\'' % (self.oConfig.cDays + 2,));
        tsEffectiveSafe = oDb.fetchOne()[0];
        print 'Going back to:     %s (safe: %s)' % (tsEffective, tsEffectiveSafe);

        # We dump test boxes back to the safe timestamp because the test sets may
        # use slightly dated test box references and we don't wish to have dangling
        # references when loading.
        for sTable in [ 'TestBoxes', ]:
            self._doCopyTo(sTable, oZipFile, oDb,
                           'COPY (SELECT * FROM ' + sTable + ' WHERE tsExpire >= %s) TO STDOUT WITH (FORMAT TEXT)',
                           (tsEffectiveSafe,));

        # The test results needs to start with test sets and then dump everything
        # releated to them.  So, figure the lowest (oldest) test set ID we'll be
        # dumping first.
        oDb.execute('SELECT idTestSet FROM TestSets WHERE tsCreated >= %s', (tsEffective, ));
        idFirstTestSet = 0;
        if oDb.getRowCount() > 0:
            idFirstTestSet = oDb.fetchOne()[0];
        print 'First test set ID: %s' % (idFirstTestSet,);

        oDb.execute('SELECT MAX(idTestSet) FROM TestSets WHERE tsCreated >= %s', (tsEffective, ));
        idLastTestSet = 0;
        if oDb.getRowCount() > 0:
            idLastTestSet = oDb.fetchOne()[0];
        print 'Last test set ID: %s' % (idLastTestSet,);

        oDb.execute('SELECT MAX(idTestResult) FROM TestResults WHERE tsCreated >= %s', (tsEffective, ));
        idLastTestResult = 0;
        if oDb.getRowCount() > 0:
            idLastTestResult = oDb.fetchOne()[0];
        print 'Last test result ID: %s' % (idLastTestResult,);


        # Tables with idTestSet member.
        for sTable in [ 'TestSets', 'TestResults', 'TestResultValues' ]:
            self._doCopyTo(sTable, oZipFile, oDb,
                           'COPY (SELECT *\n'
                           '      FROM   ' + sTable + '\n'
                           '      WHERE  idTestSet    >= %s\n'
                           '         AND idTestSet    <= %s\n'
                           '         AND idTestResult <= %s\n'
                           ') TO STDOUT WITH (FORMAT TEXT)'
                           , ( idFirstTestSet, idLastTestSet, idLastTestResult,));

        # Tables where we have to go via TestResult.
        for sTable in [ 'TestResultFiles', 'TestResultMsgs', 'TestResultFailures' ]:
            self._doCopyTo(sTable, oZipFile, oDb,
                           'COPY (SELECT it.*\n'
                           '      FROM ' + sTable + ' it, TestResults tr\n'
                           '      WHERE  tr.idTestSet    >= %s\n'
                           '         AND tr.idTestSet    <= %s\n'
                           '         AND tr.idTestResult <= %s\n'
                           '         AND tr.tsCreated    >= %s\n' # performance hack.
                           '         AND it.idTestResult  = tr.idTestResult\n'
                           ') TO STDOUT WITH (FORMAT TEXT)'
                           , ( idFirstTestSet, idLastTestSet, idLastTestResult, tsEffective,));

        # Tables which goes exclusively by tsCreated.
        for sTable in [ 'SystemLog', ]:
            self._doCopyTo(sTable, oZipFile, oDb,
                           'COPY (SELECT * FROM ' + sTable + ' WHERE tsCreated >= %s) TO STDOUT WITH (FORMAT TEXT)',
                           (tsEffective,));

        oZipFile.close();
        print "Done!";
        return 0;

    def _doLoad(self, oDb):
        """ Does the loading of the dumped data into the database. """

        oZipFile = zipfile.ZipFile(self.oConfig.sFilename, 'r');

        asTablesInLoadOrder = [
            'Users',
            'BuildBlacklist',
            'BuildCategories',
            'BuildSources',
            'FailureCategories',
            'FailureReasons',
            'GlobalResources',
            'Testcases',
            'TestcaseArgs',
            'TestcaseDeps',
            'TestcaseGlobalRsrcDeps',
            'TestGroups',
            'TestGroupMembers',
            'SchedGroups',
            'TestBoxStrTab',
            'TestBoxes',
            'SchedGroupMembers',
            'SchedQueues',
            'Builds',
            'SystemLog',
            'VcsRevisions',
            'TestResultStrTab',
            'TestSets',
            'TestResults',
            'TestResultFiles',
            'TestResultMsgs',
            'TestResultValues',
            'TestResultFailures',
        ];
        assert len(asTablesInLoadOrder) == len(self.kasTablesToDumpInFull) + len(self.kasTablesToPartiallyDump);

        oDb.begin();
        oDb.execute('SET CONSTRAINTS ALL DEFERRED;');

        print 'Checking if the database looks empty...\n'
        for sTable in asTablesInLoadOrder + [ 'TestBoxStatuses', 'GlobalResourceStatuses' ]:
            oDb.execute('SELECT COUNT(*) FROM ' + sTable);
            cRows = oDb.fetchOne()[0];
            cMaxRows = 0;
            if sTable in [ 'SchedGroups', 'TestBoxStrTab', 'TestResultStrTab', 'Users' ]:    cMaxRows =  1;
            if cRows > cMaxRows:
                print 'error: Table %s has %u rows which is more than %u - refusing to delete and load.' \
                    % (sTable, cRows, cMaxRows,);
                print 'info:  Please drop and recreate the database before loading!'
                return 1;

        print 'Dropping default table content...\n'
        for sTable in [ 'SchedGroups', 'TestBoxStrTab', 'TestResultStrTab', 'Users']:
            oDb.execute('DELETE FROM ' + sTable);

        oDb.execute('ALTER TABLE TestSets DROP CONSTRAINT IF EXISTS TestSets_idTestResult_fkey');

        for sTable in asTablesInLoadOrder:
            print 'Loading %s...' % (sTable,);
            oFile = oZipFile.open(sTable);
            oDb.copyExpert('COPY ' + sTable + ' FROM STDIN WITH (FORMAT TEXT)', oFile);
            cRows = oDb.getRowCount();
            print '... %s rows.' % (cRows,);

        oDb.execute('ALTER TABLE TestSets ADD FOREIGN KEY (idTestResult) REFERENCES TestResults(idTestResult)');
        oDb.commit();

        # Correct sequences.
        atSequences = [
            ( 'UserIdSeq',              'Users',                'uid' ),
            ( 'GlobalResourceIdSeq',    'GlobalResources',      'idGlobalRsrc' ),
            ( 'BuildSourceIdSeq',       'BuildSources',         'idBuildSrc' ),
            ( 'TestCaseIdSeq',          'TestCases',            'idTestCase' ),
            ( 'TestCaseGenIdSeq',       'TestCases',            'idGenTestCase' ),
            ( 'TestCaseArgsIdSeq',      'TestCaseArgs',         'idTestCaseArgs' ),
            ( 'TestCaseArgsGenIdSeq',   'TestCaseArgs',         'idGenTestCaseArgs' ),
            ( 'TestGroupIdSeq',         'TestGroups',           'idTestGroup' ),
            ( 'SchedGroupIdSeq',        'SchedGroups',          'idSchedGroup' ),
            ( 'TestBoxStrTabIdSeq',     'TestBoxStrTab',        'idStr' ),
            ( 'TestBoxIdSeq',           'TestBoxes',            'idTestBox' ),
            ( 'TestBoxGenIdSeq',        'TestBoxes',            'idGenTestBox' ),
            ( 'FailureCategoryIdSeq',   'FailureCategories',    'idFailureCategory' ),
            ( 'FailureReasonIdSeq',     'FailureReasons',       'idFailureReason' ),
            ( 'BuildBlacklistIdSeq',    'BuildBlacklist',       'idBlacklisting' ),
            ( 'BuildCategoryIdSeq',     'BuildCategories',      'idBuildCategory' ),
            ( 'BuildIdSeq',             'Builds',               'idBuild' ),
            ( 'TestResultStrTabIdSeq',  'TestResultStrTab',     'idStr' ),
            ( 'TestResultIdSeq',        'TestResults',          'idTestResult' ),
            ( 'TestResultValueIdSeq',   'TestResultValues',     'idTestResultValue' ),
            ( 'TestResultFileId',       'TestResultFiles',      'idTestResultFile' ),
            ( 'TestResultMsgIdSeq',     'TestResultMsgs',       'idTestResultMsg' ),
            ( 'TestSetIdSeq',           'TestSets',             'idTestSet' ),
            ( 'SchedQueueItemIdSeq',    'SchedQueues',          'idItem' ),
        ];
        for (sSeq, sTab, sCol) in atSequences:
            oDb.execute('SELECT MAX(%s) FROM %s' % (sCol, sTab,));
            idMax = oDb.fetchOne()[0];
            print '%s: idMax=%s' % (sSeq, idMax);
            if idMax is not None:
                oDb.execute('SELECT setval(\'%s\', %s)' % (sSeq, idMax));

        # Last step.
        print 'Analyzing...'
        oDb.execute('ANALYZE');
        oDb.commit();

        print 'Done!'
        return 0;

    def main(self):
        """
        Main function.
        """
        oDb = TMDatabaseConnection();

        if self.oConfig.fLoadDumpIntoDatabase is not True:
            rc = self._doDump(oDb);
        else:
            rc = self._doLoad(oDb);

        oDb.close();
        return 0;

if __name__ == '__main__':
    sys.exit(PartialDbDump().main());

