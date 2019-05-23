#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdTreeDepth1.py $

"""
VirtualBox Validation Kit - Medium and Snapshot Tree Depth Test #1
"""

__copyright__ = \
"""
Copyright (C) 2010-2017 Oracle Corporation

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


# Standard Python imports.
import os
import sys

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0]
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(g_ksValidationKitDir)

# Validation Kit imports.
from testdriver import reporter
from testdriver import vbox
from testdriver import vboxcon


class tdTreeDepth1(vbox.TestDriver):
    """
    Medium and Snapshot Tree Depth Test #1.
    """

    def __init__(self):
        vbox.TestDriver.__init__(self)
        self.asRsrcs            = None


    #
    # Overridden methods.
    #

    def actionConfig(self):
        """
        Import the API.
        """
        if not self.importVBoxApi():
            return False
        return True

    def actionExecute(self):
        """
        Execute the testcase.
        """
        return  self.testMediumTreeDepth() \
            and self.testSnapshotTreeDepth()

    #
    # Test execution helpers.
    #

    def testMediumTreeDepth(self):
        """
        Test medium tree depth.
        """
        reporter.testStart('mediumTreeDepth')

        try:
            oVM = self.createTestVM('test-medium', 1, None, 4)
            assert oVM is not None

            # create chain with 300 disk images (medium tree depth limit)
            fRc = True
            oSession = self.openSession(oVM)
            for i in range(1, 301):
                sHddPath = os.path.join(self.sScratchPath, 'Test' + str(i) + '.vdi')
                if i is 1:
                    oHd = oSession.createBaseHd(sHddPath, cb=1024*1024)
                else:
                    oHd = oSession.createDiffHd(oHd, sHddPath)
                if oHd is None:
                    fRc = False
                    break

            # modify the VM config, attach HDD
            fRc = fRc and oSession.attachHd(sHddPath, sController='SATA Controller', fImmutable=False, fForceResource=False)
            fRc = fRc and oSession.saveSettings()
            fRc = oSession.close() and fRc

            # unregister and re-register to test loading of settings
            sSettingsFile = oVM.settingsFilePath
            reporter.log('unregistering VM')
            oVM.unregister(vboxcon.CleanupMode_DetachAllReturnNone)
            oVBox = self.oVBoxMgr.getVirtualBox()
            reporter.log('opening VM %s, testing config reading' % (sSettingsFile))
            oVM = oVBox.openMachine(sSettingsFile)

            assert fRc is True
        except:
            reporter.errorXcpt()

        return reporter.testDone()[1] == 0

    def testSnapshotTreeDepth(self):
        """
        Test snapshot tree depth.
        """
        reporter.testStart('snapshotTreeDepth')

        try:
            oVM = self.createTestVM('test-snap', 1, None, 4)
            assert oVM is not None

            # modify the VM config, create and attach empty HDD
            oSession = self.openSession(oVM)
            sHddPath = os.path.join(self.sScratchPath, 'TestSnapEmpty.vdi')
            fRc = True
            fRc = fRc and oSession.createAndAttachHd(sHddPath, cb=1024*1024, sController='SATA Controller', fImmutable=False)
            fRc = fRc and oSession.saveSettings()

            # take 250 snapshots (snapshot tree depth limit)
            for i in range(1, 251):
                fRc = fRc and oSession.takeSnapshot('Snapshot ' + str(i))
            fRc = oSession.close() and fRc

            # unregister and re-register to test loading of settings
            sSettingsFile = oVM.settingsFilePath
            reporter.log('unregistering VM')
            oVM.unregister(vboxcon.CleanupMode_DetachAllReturnNone)
            oVBox = self.oVBoxMgr.getVirtualBox()
            reporter.log('opening VM %s, testing config reading' % (sSettingsFile))
            oVM = oVBox.openMachine(sSettingsFile)

            assert fRc is True
        except:
            reporter.errorXcpt()

        return reporter.testDone()[1] == 0


if __name__ == '__main__':
    sys.exit(tdTreeDepth1().main(sys.argv))

