#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdAddBasic1.py $

"""
VirtualBox Validation Kit - Additions Basics #1.
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
import os;
import sys;

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from testdriver import reporter;
from testdriver import base;
from testdriver import vbox;
from testdriver import vboxcon;

# Sub test driver imports.
sys.path.append(os.path.dirname(os.path.abspath(__file__))); # For sub-test drivers.
from tdAddGuestCtrl import SubTstDrvAddGuestCtrl;


class tdAddBasic1(vbox.TestDriver):                                         # pylint: disable=R0902
    """
    Additions Basics #1.
    """
    ## @todo
    # - More of the settings stuff can be and need to be generalized!
    #

    def __init__(self):
        vbox.TestDriver.__init__(self);
        self.oTestVmSet = self.oTestVmManager.getStandardVmSet('nat');
        self.asTestsDef = ['guestprops', 'stdguestprops', 'guestcontrol'];
        self.asTests    = self.asTestsDef;

        self.addSubTestDriver(SubTstDrvAddGuestCtrl(self));

    #
    # Overridden methods.
    #
    def showUsage(self):
        rc = vbox.TestDriver.showUsage(self);
        reporter.log('');
        reporter.log('tdAddBasic1 Options:');
        reporter.log('  --tests        <s1[:s2[:]]>');
        reporter.log('      Default: %s  (all)' % (':'.join(self.asTestsDef)));
        reporter.log('  --quick');
        reporter.log('      Same as --virt-modes hwvirt --cpu-counts 1.');
        return rc;

    def parseOption(self, asArgs, iArg):                                        # pylint: disable=R0912,R0915
        if asArgs[iArg] == '--tests':
            iArg += 1;
            if iArg >= len(asArgs): raise base.InvalidOption('The "--tests" takes a colon separated list of tests');
            self.asTests = asArgs[iArg].split(':');
            for s in self.asTests:
                if s not in self.asTestsDef:
                    raise base.InvalidOption('The "--tests" value "%s" is not valid; valid values are: %s' \
                        % (s, ' '.join(self.asTestsDef)));

        elif asArgs[iArg] == '--quick':
            self.parseOption(['--virt-modes', 'hwvirt'], 0);
            self.parseOption(['--cpu-counts', '1'], 0);

        else:
            return vbox.TestDriver.parseOption(self, asArgs, iArg);
        return iArg + 1;

    def actionConfig(self):
        if not self.importVBoxApi(): # So we can use the constant below.
            return False;

        eNic0AttachType = vboxcon.NetworkAttachmentType_NAT;
        sGaIso = self.getGuestAdditionsIso();
        return self.oTestVmSet.actionConfig(self, eNic0AttachType = eNic0AttachType, sDvdImage = sGaIso);

    def actionExecute(self):
        return self.oTestVmSet.actionExecute(self, self.testOneCfg);


    #
    # Test execution helpers.
    #

    def testOneCfg(self, oVM, oTestVm):
        """
        Runs the specified VM thru the tests.

        Returns a success indicator on the general test execution. This is not
        the actual test result.
        """
        fRc = False;

        self.logVmInfo(oVM);
        oSession, oTxsSession = self.startVmAndConnectToTxsViaTcp(oTestVm.sVmName, fCdWait = True, \
                                                                  sFileCdWait = 'AUTORUN.INF');
        if oSession is not None:
            self.addTask(oTxsSession);
            # Do the testing.
            reporter.testStart('Install');
            fRc, oTxsSession = self.testInstallAdditions(oSession, oTxsSession, oTestVm);
            reporter.testDone();
            fSkip = not fRc;

            reporter.testStart('Guest Properties');
            if not fSkip:
                fRc = self.testGuestProperties(oSession, oTxsSession, oTestVm) and fRc;
            reporter.testDone(fSkip);

            reporter.testStart('Guest Control');
            if not fSkip:
                (fRc2, oTxsSession) = self.aoSubTstDrvs[0].testIt(oTestVm, oSession, oTxsSession);
                fRc = fRc2 and fRc;
            reporter.testDone(fSkip);

            ## @todo Save and restore test.

            ## @todo Reset tests.

            ## @todo Final test: Uninstallation.

            # Cleanup.
            self.removeTask(oTxsSession);
            self.terminateVmBySession(oSession)
        return fRc;

    def testInstallAdditions(self, oSession, oTxsSession, oTestVm):
        """
        Tests installing the guest additions
        """
        if oTestVm.isWindows():
            (fRc, oTxsSession) = self.testWindowsInstallAdditions(oSession, oTxsSession, oTestVm);
        else:
            reporter.error('Guest Additions installation not implemented for %s yet! (%s)' % \
                           (oTestVm.sKind, oTestVm.sVmName));
            fRc = False;

        #
        # Verify installation of Guest Additions using commmon bits.
        #
        if fRc is True:
            #
            # Wait for the GAs to come up.
            #

            ## @todo need to signed up for a OnAdditionsStateChanged and wait runlevel to
            #  at least reach Userland.

            #
            # Check if the additions are operational.
            #
            try:    oGuest = oSession.o.console.guest;
            except:
                reporter.errorXcpt('Getting IGuest failed.');
                return (False, oTxsSession);

            # Check the additionsVersion attribute. It must not be empty.
            reporter.testStart('IGuest::additionsVersion');
            fRc = self.testIGuest_additionsVersion(oGuest);
            reporter.testDone();

            reporter.testStart('IGuest::additionsRunLevel');
            self.testIGuest_additionsRunLevel(oGuest, oTestVm);
            reporter.testDone();

            ## @todo test IAdditionsFacilities.

        return (fRc, oTxsSession);

    def testWindowsInstallAdditions(self, oSession, oTxsSession, oTestVm):
        """
        Installs the Windows guest additions using the test execution service.
        Since this involves rebooting the guest, we will have to create a new TXS session.
        """
        asLogFile = [];

        fHaveSetupApiDevLog = False;

        # Delete relevant log files.
        if oTestVm.sKind in ('WindowsNT4',):
            sWinDir = 'C:/WinNT/';
        else:
            sWinDir = 'C:/Windows/';
            asLogFile = [sWinDir+'setupapi.log', sWinDir+'setupact.log', sWinDir+'setuperr.log'];

            # Apply The SetupAPI logging level so that we also get the (most verbose) setupapi.dev.log file.
            ## @todo !!! HACK ALERT !!! Add the value directly into the testing source image. Later.
            fHaveSetupApiDevLog = self.txsRunTest(oTxsSession, 'Enabling setupapi.dev.log', 30 * 1000, \
                'c:\\Windows\\System32\\reg.exe', ('c:\\Windows\\System32\\reg.exe', \
                'add', '"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup"', '/v', 'LogLevel', '/t', 'REG_DWORD', \
                '/d', '0xFF'));

        # On some guests the files in question still can be locked by the OS, so ignore deletion
        # errors from the guest side (e.g. sharing violations) and just continue.
        for sFile in asLogFile:
            self.txsRmFile(oSession, oTxsSession, sFile, 10 * 1000, fIgnoreErrors = True);

        # Install the public signing key.
        if oTestVm.sKind not in ('WindowsNT4', 'Windows2000', 'WindowsXP', 'Windows2003'):
            ## TODO
            pass;

        #
        # The actual install.
        # Enable installing the optional auto-logon modules (VBoxGINA/VBoxCredProv) + (Direct)3D support.
        # Also tell the installer to produce the appropriate log files.
        #
        fRc = self.txsRunTest(oTxsSession, 'VBoxWindowsAdditions.exe', 5 * 60 * 1000, \
            '${CDROM}/VBoxWindowsAdditions.exe', ('${CDROM}/VBoxWindowsAdditions.exe', '/S', '/l', '/with_autologon'));
        ## @todo For testing the installation (D)3D stuff ('/with_d3d') we need to boot up in safe mode.

        #
        # Reboot the VM and reconnect the TXS session.
        #
        if fRc is True:
            (fRc, oTxsSession) = self.txsRebootAndReconnectViaTcp(oSession, oTxsSession, cMsTimeout = 3 * 60000);

            if fRc is True:
                # Add the Windows Guest Additions installer files to the files we want to download
                # from the guest.
                sGuestAddsDir = 'C:/Program Files/Oracle/VirtualBox Guest Additions/';
                asLogFile.append(sGuestAddsDir + 'install.log');
                # Note: There won't be a install_ui.log because of the silent installation.
                asLogFile.append(sGuestAddsDir + 'install_drivers.log');
                asLogFile.append('C:/Windows/setupapi.log');

                # Note: setupapi.dev.log only is available since Windows 2000.
                if fHaveSetupApiDevLog:
                    asLogFile.append('C:/Windows/setupapi.dev.log');

                #
                # Download log files.
                # Ignore errors as all files above might not be present (or in different locations)
                # on different Windows guests.
                #
                self.txsDownloadFiles(oSession, oTxsSession, asLogFile, fIgnoreErrors = True);

        return (fRc, oTxsSession);

    def testIGuest_additionsVersion(self, oGuest):
        """
        Returns False if no version string could be obtained, otherwise True
        even though errors are logged.
        """
        try:
            sVer = oGuest.additionsVersion;
        except:
            reporter.errorXcpt('Getting the additions version failed.');
            return False;
        reporter.log('IGuest::additionsVersion="%s"' % (sVer,));

        if sVer.strip() == '':
            reporter.error('IGuest::additionsVersion is empty.');
            return False;

        if sVer != sVer.strip():
            reporter.error('IGuest::additionsVersion is contains spaces: "%s".' % (sVer,));

        asBits = sVer.split('.');
        if len(asBits) < 3:
            reporter.error('IGuest::additionsVersion does not contain at least tree dot separated fields: "%s" (%d).'
                           % (sVer, len(asBits)));

        ## @todo verify the format.
        return True;

    def testIGuest_additionsRunLevel(self, oGuest, oTestVm):
        """
        Do run level tests.
        """
        if oTestVm.isLoggedOntoDesktop():
            eExpectedRunLevel = vboxcon.AdditionsRunLevelType_Desktop;
        else:
            eExpectedRunLevel = vboxcon.AdditionsRunLevelType_Userland;

        ## @todo Insert wait for the desired run level.
        try:
            iLevel = oGuest.additionsRunLevel;
        except:
            reporter.errorXcpt('Getting the additions run level failed.');
            return False;
        reporter.log('IGuest::additionsRunLevel=%s' % (iLevel,));

        if iLevel != eExpectedRunLevel:
            pass; ## @todo We really need that wait!!
            #reporter.error('Expected runlevel %d, found %d instead' % (eExpectedRunLevel, iLevel));
        return True;


    def testGuestProperties(self, oSession, oTxsSession, oTestVm):
        """
        Test guest properties.
        """
        _ = oSession; _ = oTxsSession; _ = oTestVm;
        return True;

if __name__ == '__main__':
    sys.exit(tdAddBasic1().main(sys.argv));

