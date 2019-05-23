#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdMoveMedium.py

"""
VirtualBox Validation Kit - Medium Move Test
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
__version__ = ""

# Standard Python imports.
import os
import sys

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0]
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(g_ksValidationKitDir)

# Validation Kit imports.
from common     import utils
from testdriver import reporter
from testdriver import vbox
from testdriver import vboxwrappers;

class tdMoveMedium(vbox.TestDriver):
    """
    Medium moving Test.
    """

    # Suffix exclude list.
    suffixes = [
        '.vdi',
    ];

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
        return  self.testMediumMove()

    #
    # Test execution helpers.
    #
    def setLocation(self, sLocation, aListOfAttach):
        for attachment in aListOfAttach:
            try:
                oMedium = attachment.medium;
                reporter.log('Move medium ' + oMedium.name + ' to the ' + sLocation)
            except:
                reporter.errorXcpt('failed to get the medium from the IMediumAttachment "%s"' % (attachment));

            try:
                oProgress = vboxwrappers.ProgressWrapper(oMedium.setLocation(sLocation), self.oVBoxMgr, self, 'move "%s"' % (oMedium.name,));
            except:
                return reporter.errorXcpt('Medium::setLocation("%s") for medium "%s" failed' % (sLocation, oMedium.name,));

            oProgress.wait();
            if oProgress.logResult() is False:
                return False;

    # Test with VDI image
    # move medium to a new location.
    # case 1. Only path without file name
    # case 2. Only path without file name without '\' or '/' at the end
    # case 3. Path with file name
    # case 4. Only file name
    # case 5. Move snapshot

    def testMediumMove(self):
        """
        Test medium moving.
        """
        reporter.testStart('medium moving')

        try:
            oVM = self.createTestVM('test-medium-move', 1, None, 4)
            assert oVM is not None

            # create virtual disk image vdi
            fRc = True
            c = 0
            oSession = self.openSession(oVM)
            for i in self.suffixes:
                sHddPath = os.path.join(self.sScratchPath, 'Test' + str(c) + i)
                oHd = oSession.createBaseHd(sHddPath, cb=1024*1024)
                if oHd is None:
                    fRc = False
                    break

                # attach HDD, IDE controller exists by default, but we use SATA just in case
                sController='SATA Controller'
                fRc = fRc and oSession.attachHd(sHddPath, sController, iPort = c, iDevice = 0, fImmutable=False, fForceResource=False)
                fRc = fRc and oSession.saveSettings()

            #create temporary subdirectory in the current working directory
            sOrigLoc = self.sScratchPath
            sNewLoc = os.path.join(sOrigLoc, 'newLocation/')

            os.makedirs(sNewLoc, 0775);

            aListOfAttach = oVM.getMediumAttachmentsOfController(sController)
            #case 1. Only path without file name
            sLocation = sNewLoc
            self.setLocation(sLocation, aListOfAttach)

            #case 2. Only path without file name without '\' or '/' at the end
            sLocation = sOrigLoc
            self.setLocation(sLocation, aListOfAttach)

            #case 3. Path with file name
            sLocation = sNewLoc + 'newName.vdi'
            self.setLocation(sLocation, aListOfAttach)

            #case 4. Only file name
            sLocation = 'onlyMediumName'
            self.setLocation(sLocation, aListOfAttach)

            #case 5. Move snapshot
            fRc = fRc and oSession.takeSnapshot('Snapshot1')
            sLocation = sOrigLoc
            #get fresh attachments after snapshot
            aListOfAttach = oVM.getMediumAttachmentsOfController(sController)
            self.setLocation(sLocation, aListOfAttach)

            fRc = oSession.close() and fRc

            assert fRc is True
        except:
            reporter.errorXcpt()

        return reporter.testDone()[1] == 0

if __name__ == '__main__':
    sys.exit(tdMoveMedium().main(sys.argv))

