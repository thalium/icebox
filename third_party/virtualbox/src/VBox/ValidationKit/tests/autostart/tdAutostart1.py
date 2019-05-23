#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
AUtostart testcase using.
"""

__copyright__ = \
"""
Copyright (C) 2013-2017 Oracle Corporation

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
__version__ = "$Id: tdAutostart1.py $"


# Standard Python imports.
import os;
import sys;
import re;
import array;

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

class VBoxManageStdOutWrapper(object):
    """ Parser for VBoxManage list runningvms """
    def __init__(self):
        self.sVmRunning = '';

    def read(self, cb):
        """file.read"""
        _ = cb;
        return "";

    def write(self, sText):
        """VBoxManage stdout write"""
        if isinstance(sText, array.array):
            sText = sText.tostring();

        asLines = sText.splitlines();
        for sLine in asLines:
            sLine = sLine.strip();

                # Extract the value
            idxVmNameStart = sLine.find('"');
            if idxVmNameStart == -1:
                raise Exception('VBoxManageStdOutWrapper: Invalid output');

            idxVmNameStart += 1;
            idxVmNameEnd = idxVmNameStart;
            while sLine[idxVmNameEnd] != '"':
                idxVmNameEnd += 1;

            self.sVmRunning = sLine[idxVmNameStart:idxVmNameEnd];
            reporter.log('Logging: ' + self.sVmRunning);

        return None;

class tdAutostartOs(object):
    """
    Base autostart helper class to provide common methods.
    """

    def _findFile(self, sRegExp, sTestBuildDir):
        """
        Returns a filepath based on the given regex and path to look into
        or None if no matching file is found.
        """

        oRegExp = re.compile(sRegExp);

        asFiles = os.listdir(sTestBuildDir);

        for sFile in asFiles:
            if oRegExp.match(os.path.basename(sFile)) and os.path.exists(sTestBuildDir + '/' + sFile):
                return sTestBuildDir + '/' + sFile;

        reporter.error('Failed to find a file matching "%s" in %s.' % (sRegExp, sTestBuildDir));
        return None;

    def _createAutostartCfg(self, sDefaultPolicy = 'allow', asUserAllow = (), asUserDeny = ()):
        """
        Creates a autostart config for VirtualBox
        """

        sVBoxCfg = 'default_policy=' + sDefaultPolicy + '\n';

        for sUserAllow in asUserAllow:
            sVBoxCfg = sVBoxCfg + sUserAllow + ' = {\n allow = true\n }\n';

        for sUserDeny in asUserDeny:
            sVBoxCfg = sVBoxCfg + sUserDeny + ' = {\n allow = false\n }\n';

        return sVBoxCfg;

class tdAutostartOsLinux(tdAutostartOs):
    """
    Autostart support methods for Linux guests.
    """

    def __init__(self, oTestDriver, sTestBuildDir):
        tdAutostartOs.__init__(self);
        self.sTestBuild = self._findFile('^VirtualBox-.*\\.run$', sTestBuildDir);
        self.oTestDriver = oTestDriver;

    def installVirtualBox(self, oSession, oTxsSession):
        """
        Install VirtualBox in the guest.
        """
        fRc = False;

        if self.sTestBuild is not None:
            fRc = self.oTestDriver.txsUploadFile(oSession, oTxsSession, self.sTestBuild, \
                                                 '/tmp/' + os.path.basename(self.sTestBuild), \
                                                 cMsTimeout = 120 * 1000);
            fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Installing VBox', 10 * 1000, \
                                                      '/bin/chmod',
                                                      ('chmod', '755', '/tmp/' + os.path.basename(self.sTestBuild)));
            fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Installing VBox', 240 * 1000, \
                                                      '/tmp/' + os.path.basename(self.sTestBuild));

        return fRc;

    def configureAutostart(self, oSession, oTxsSession, sDefaultPolicy = 'allow',
                           asUserAllow = (), asUserDeny = ()):
        """
        Configures the autostart feature in the guest.
        """

        # Create autostart database directory writeable for everyone
        fRc = self.oTestDriver.txsRunTest(oTxsSession, 'Creating autostart database', 10 * 1000, \
                                          '/bin/mkdir',
                                          ('mkdir', '-m', '1777', '/etc/vbox/autostart.d'));

        # Create /etc/default/virtualbox
        sVBoxCfg =   'VBOXAUTOSTART_CONFIG=/etc/vbox/autostart.cfg\n' \
                   + 'VBOXAUTOSTART_DB=/etc/vbox/autostart.d\n';
        fRc = fRc and self.oTestDriver.txsUploadString(oSession, oTxsSession, sVBoxCfg, '/etc/default/virtualbox');
        fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Setting permissions', 10 * 1000, \
                                                  '/bin/chmod',
                                                  ('chmod', '644', '/etc/default/virtualbox'));

        sVBoxCfg = self._createAutostartCfg(sDefaultPolicy, asUserAllow, asUserDeny);
        fRc = fRc and self.oTestDriver.txsUploadString(oSession, oTxsSession, sVBoxCfg, '/etc/vbox/autostart.cfg');
        fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Setting permissions', 10 * 1000, \
                                                  '/bin/chmod',
                                                  ('chmod', '644', '/etc/vbox/autostart.cfg'));

        return fRc;

    def createUser(self, oTxsSession, sUser):
        """
        Create a new user with the given name
        """

        fRc = self.oTestDriver.txsRunTest(oTxsSession, 'Creating new user', 10 * 1000, \
                                          '/usr/sbin/useradd',
                                          ('useradd', '-m', '-U', sUser));

        return fRc;

    # pylint: disable=R0913

    def runProgAsUser(self, oTxsSession, sTestName, cMsTimeout, sExecName, sAsUser, asArgs = (),
                      oStdIn = '/dev/null', oStdOut = '/dev/null', oStdErr = '/dev/null', oTestPipe = '/dev/null'):
        """
        Runs a program as the specififed user
        """
        asNewArgs = ('sudo', '-u', sAsUser, sExecName) + asArgs;

        fRc = self.oTestDriver.txsRunTestRedirectStd(oTxsSession, sTestName, cMsTimeout, '/usr/bin/sudo',
                                                     asNewArgs, (), "", oStdIn, oStdOut, oStdErr, oTestPipe);

        return fRc;

    # pylint: enable=R0913

    def createTestVM(self, oSession, oTxsSession, sUser, sVmName):
        """
        Create a test VM in the guest and enable autostart.
        """

        _ = oSession;

        fRc = self.runProgAsUser(oTxsSession, 'Configuring autostart database', 10 * 1000, \
                                 '/opt/VirtualBox/VBoxManage', sUser,
                                 ('setproperty', 'autostartdbpath', '/etc/vbox/autostart.d'));
        fRc = fRc and self.runProgAsUser(oTxsSession, 'Create VM ' + sVmName, 10 * 1000, \
                                         '/opt/VirtualBox/VBoxManage', sUser,
                                         ('createvm', '--name', sVmName, '--register'));
        fRc = fRc and self.runProgAsUser(oTxsSession, 'Enabling autostart for test VM', 10 * 1000, \
                                         '/opt/VirtualBox/VBoxManage', sUser,
                                         ('modifyvm', sVmName, '--autostart-enabled', 'on'));

        return fRc;

    def checkForRunningVM(self, oSession, oTxsSession, sUser, sVmName):
        """
        Check for VM running in the guest after autostart.
        """

        _ = oSession;

        oStdOut = VBoxManageStdOutWrapper();
        fRc = self.runProgAsUser(oTxsSession, 'Check for running VM', 20 * 1000, \
                                 '/opt/VirtualBox/VBoxManage', sUser, ('list', 'runningvms'),
                                 '/dev/null', oStdOut, '/dev/null', '/dev/null');

        fRc = fRc is True and oStdOut.sVmRunning == sVmName;

        return fRc;

class tdAutostartOsDarwin(tdAutostartOs):
    """
    Autostart support methods for Darwin guests.
    """

    def __init__(self, sTestBuildDir):
        _ = sTestBuildDir;
        tdAutostartOs.__init__(self);

    def installVirtualBox(self, oSession, oTxsSession):
        """
        Install VirtualBox in the guest.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

    def configureAutostart(self, oSession, oTxsSession):
        """
        Configures the autostart feature in the guest.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

    def createTestVM(self, oSession, oTxsSession):
        """
        Create a test VM in the guest and enable autostart.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

    def checkForRunningVM(self, oSession, oTxsSession):
        """
        Check for VM running in the guest after autostart.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

class tdAutostartOsSolaris(tdAutostartOs):
    """
    Autostart support methods for Solaris guests.
    """

    def __init__(self, oTestDriver, sTestBuildDir):
        tdAutostartOs.__init__(self);
        self.sTestBuildDir = sTestBuildDir;
        self.sTestBuild = self._findFile('^VirtualBox-.*-SunOS-.*\\.tar.gz$', sTestBuildDir);
        self.oTestDriver = oTestDriver;

    def installVirtualBox(self, oSession, oTxsSession):
        """
        Install VirtualBox in the guest.
        """
        fRc = False;

        if self.sTestBuild is not None:
            reporter.log('Testing build: %s' % (self.sTestBuild));
            sTestBuildFilename = os.path.basename(self.sTestBuild);

            # Construct the .pkg filename from the tar.gz name.
            oMatch = re.search(r'\d+.\d+.\d+-SunOS-r\d+', sTestBuildFilename);
            if oMatch is not None:
                sPkgFilename = 'VirtualBox-' + oMatch.group() + '.pkg';

                reporter.log('Extracted package filename: %s' % (sPkgFilename));

                fRc = self.oTestDriver.txsUploadFile(oSession, oTxsSession, self.sTestBuild, \
                                                     '${SCRATCH}/' + sTestBuildFilename, \
                                                     cMsTimeout = 120 * 1000);
                fRc = fRc and self.oTestDriver.txsUnpackFile(oSession, oTxsSession, \
                                                             '${SCRATCH}/' + sTestBuildFilename, \
                                                             '${SCRATCH}', cMsTimeout = 120 * 1000);
                fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Installing package', 240 * 1000, \
                                                          '/usr/sbin/pkgadd',
                                                          ('pkgadd', '-d', '${SCRATCH}/' + sPkgFilename, \
                                                           '-n', '-a', '${SCRATCH}/autoresponse', 'SUNWvbox'));
        return fRc;

    def configureAutostart(self, oSession, oTxsSession, sDefaultPolicy = 'allow',
                           asUserAllow = (), asUserDeny = ()):
        """
        Configures the autostart feature in the guest.
        """

        fRc = self.oTestDriver.txsRunTest(oTxsSession, 'Creating /etc/vbox directory', 10 * 1000, \
                                          '/usr/bin/mkdir',
                                          ('mkdir', '-m', '755', '/etc/vbox'));

        sVBoxCfg = self._createAutostartCfg(sDefaultPolicy, asUserAllow, asUserDeny);
        fRc = fRc and self.oTestDriver.txsUploadString(oSession, oTxsSession, sVBoxCfg, '/etc/vbox/autostart.cfg');
        fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Setting permissions', 10 * 1000, \
                                                  '/usr/bin/chmod',
                                                  ('chmod', '644', '/etc/vbox/autostart.cfg'));


        fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Importing the service', 10 * 1000, \
                                                  '/usr/sbin/svccfg',
                                                  ('svccfg', '-s', 'svc:/application/virtualbox/autostart:default', \
                                                   'setprop', 'config/config=/etc/vbox/autostart.cfg'));
        fRc = fRc and self.oTestDriver.txsRunTest(oTxsSession, 'Enabling the service', 10 * 1000, \
                                                  '/usr/sbin/svcadm',
                                                  ('svcadm', 'enable', 'svc:/application/virtualbox/autostart:default'));

        return fRc;

    def createUser(self, oTxsSession, sUser):
        """
        Create a new user with the given name
        """

        fRc = self.oTestDriver.txsRunTest(oTxsSession, 'Creating new user', 10 * 1000, \
                                          '/usr/sbin/useradd',
                                          ('useradd', '-m', '-g', 'staff', sUser));

        return fRc;

    # pylint: disable=R0913

    def runProgAsUser(self, oTxsSession, sTestName, cMsTimeout, sExecName, sAsUser, asArgs = (),
                      oStdIn = '/dev/null', oStdOut = '/dev/null', oStdErr = '/dev/null', oTestPipe = '/dev/null'):
        """
        Runs a program as the specififed user
        """
        asNewArgs = ('sudo', '-u', sAsUser, sExecName) + asArgs;

        fRc = self.oTestDriver.txsRunTestRedirectStd(oTxsSession, sTestName, cMsTimeout, '/usr/bin/sudo',
                                                     asNewArgs, (), "", oStdIn, oStdOut, oStdErr, oTestPipe);

        return fRc;

    # pylint: enable=R0913

    def createTestVM(self, oSession, oTxsSession, sUser, sVmName):
        """
        Create a test VM in the guest and enable autostart.
        """

        _ = oSession;

        fRc = self.runProgAsUser(oTxsSession, 'Create VM ' + sVmName, 10 * 1000, \
                                 '/opt/VirtualBox/VBoxManage', sUser,
                                 ('createvm', '--name', sVmName, '--register'));
        fRc = fRc and self.runProgAsUser(oTxsSession, 'Enabling autostart for test VM', 10 * 1000, \
                                         '/opt/VirtualBox/VBoxManage', sUser,
                                         ('modifyvm', sVmName, '--autostart-enabled', 'on'));

        return fRc;

    def checkForRunningVM(self, oSession, oTxsSession, sUser, sVmName):
        """
        Check for VM running in the guest after autostart.
        """

        _ = oSession;

        oStdOut = VBoxManageStdOutWrapper();
        fRc = self.runProgAsUser(oTxsSession, 'Check for running VM', 20 * 1000, \
                                 '/opt/VirtualBox/VBoxManage', sUser, ('list', 'runningvms'),
                                 '/dev/null', oStdOut, '/dev/null', '/dev/null');

        fRc = fRc is True and oStdOut.sVmRunning == sVmName;

        return fRc;


class tdAutostartOsWin(tdAutostartOs):
    """
    Autostart support methods for Windows guests.
    """

    def __init__(self, sTestBuildDir):
        _ = sTestBuildDir;
        tdAutostartOs.__init__(self);
        return;

    def installVirtualBox(self, oSession, oTxsSession):
        """
        Install VirtualBox in the guest.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

    def configureAutostart(self, oSession, oTxsSession):
        """
        Configures the autostart feature in the guest.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

    def createTestVM(self, oSession, oTxsSession):
        """
        Create a test VM in the guest and enable autostart.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

    def checkForRunningVM(self, oSession, oTxsSession):
        """
        Check for VM running in the guest after autostart.
        """
        _ = oSession;
        _ = oTxsSession;
        return False;

class tdAutostart(vbox.TestDriver):                                      # pylint: disable=R0902
    """
    Autostart testcase.
    """

    ksOsLinux   = 'tst-debian'
    ksOsWindows = 'tst-win'
    ksOsDarwin  = 'tst-darwin'
    ksOsSolaris = 'tst-solaris'
    ksOsFreeBSD = 'tst-freebsd'

    def __init__(self):
        vbox.TestDriver.__init__(self);
        self.asRsrcs           = None;
        self.asTestVMsDef      = [self.ksOsLinux, self.ksOsSolaris];
        self.asTestVMs         = self.asTestVMsDef;
        self.asSkipVMs         = [];
        self.sTestBuildDir     = '/home/alexander/Downloads';

    #
    # Overridden methods.
    #
    def showUsage(self):
        rc = vbox.TestDriver.showUsage(self);
        reporter.log('');
        reporter.log('tdAutostart Options:');
        reporter.log('  --test-build-dir <path>');
        reporter.log('      Default: %s' % (self.sTestBuildDir));
        reporter.log('  --test-vms      <vm1[:vm2[:...]]>');
        reporter.log('      Test the specified VMs in the given order. Use this to change');
        reporter.log('      the execution order or limit the choice of VMs');
        reporter.log('      Default: %s  (all)' % (':'.join(self.asTestVMsDef)));
        reporter.log('  --skip-vms      <vm1[:vm2[:...]]>');
        reporter.log('      Skip the specified VMs when testing.');
        return rc;

    def parseOption(self, asArgs, iArg):                                        # pylint: disable=R0912,R0915
        if asArgs[iArg] == '--test-build-dir':
            iArg += 1;
            if iArg >= len(asArgs): raise base.InvalidOption('The "--test-build-dir" takes a path argument');
            self.sTestBuildDir = asArgs[iArg];
        elif asArgs[iArg] == '--test-vms':
            iArg += 1;
            if iArg >= len(asArgs): raise base.InvalidOption('The "--test-vms" takes colon separated list');
            self.asTestVMs = asArgs[iArg].split(':');
            for s in self.asTestVMs:
                if s not in self.asTestVMsDef:
                    raise base.InvalidOption('The "--test-vms" value "%s" is not valid; valid values are: %s' \
                        % (s, ' '.join(self.asTestVMsDef)));
        elif asArgs[iArg] == '--skip-vms':
            iArg += 1;
            if iArg >= len(asArgs): raise base.InvalidOption('The "--skip-vms" takes colon separated list');
            self.asSkipVMs = asArgs[iArg].split(':');
            for s in self.asSkipVMs:
                if s not in self.asTestVMsDef:
                    reporter.log('warning: The "--test-vms" value "%s" does not specify any of our test VMs.' % (s));
        else:
            return vbox.TestDriver.parseOption(self, asArgs, iArg);
        return iArg + 1;

    def completeOptions(self):
        # Remove skipped VMs from the test list.
        for sVM in self.asSkipVMs:
            try:    self.asTestVMs.remove(sVM);
            except: pass;

        return vbox.TestDriver.completeOptions(self);

    def getResourceSet(self):
        # Construct the resource list the first time it's queried.
        if self.asRsrcs is None:
            self.asRsrcs = [];
            if self.ksOsLinux in self.asTestVMs:
                self.asRsrcs.append('4.2/autostart/tst-debian.vdi');
            if self.ksOsSolaris in self.asTestVMs:
                self.asRsrcs.append('4.2/autostart/tst-solaris.vdi');

        return self.asRsrcs;

    def actionConfig(self):

        # Make sure vboxapi has been imported so we can use the constants.
        if not self.importVBoxApi():
            return False;

        #
        # Configure the VMs we're going to use.
        #

        # Linux VMs
        if self.ksOsLinux in self.asTestVMs:
            oVM = self.createTestVM(self.ksOsLinux, 1, '4.2/autostart/tst-debian.vdi', sKind = 'Debian_64', \
                                    fIoApic = True, eNic0AttachType = vboxcon.NetworkAttachmentType_NAT, \
                                    eNic0Type = vboxcon.NetworkAdapterType_Am79C973);
            if oVM is None:
                return False;

        # Solaris VMs
        if self.ksOsSolaris in self.asTestVMs:
            oVM = self.createTestVM(self.ksOsSolaris, 1, '4.2/autostart/tst-solaris.vdi', sKind = 'Solaris_64', \
                                    fIoApic = True, eNic0AttachType = vboxcon.NetworkAttachmentType_NAT, \
                                    sHddControllerType = "SATA Controller");
            if oVM is None:
                return False;

        return True;

    def actionExecute(self):
        """
        Execute the testcase.
        """
        fRc = self.testAutostart();
        return fRc;

    #
    # Test execution helpers.
    #
    def testAutostartRunProgs(self, oSession, oTxsSession, sVmName):
        """
        Test VirtualBoxs autostart feature in a VM.
        """
        reporter.testStart('Autostart ' + sVmName);

        oGuestOsHlp = None;             # type: tdAutostartOs
        if sVmName == self.ksOsLinux:
            oGuestOsHlp = tdAutostartOsLinux(self, self.sTestBuildDir);
        elif sVmName == self.ksOsSolaris:
            oGuestOsHlp = tdAutostartOsSolaris(self, self.sTestBuildDir);   # annoying - pylint: disable=redefined-variable-type
        elif sVmName == self.ksOsDarwin:
            oGuestOsHlp = tdAutostartOsDarwin(self.sTestBuildDir);
        elif sVmName == self.ksOsWindows:
            oGuestOsHlp = tdAutostartOsWin(self.sTestBuildDir);

        sTestUserAllow = 'test1';
        sTestUserDeny = 'test2';
        sTestVmName = 'TestVM';

        if oGuestOsHlp is not None:
            # Create two new users
            fRc = oGuestOsHlp.createUser(oTxsSession, sTestUserAllow);
            fRc = fRc and oGuestOsHlp.createUser(oTxsSession, sTestUserDeny);
            if fRc is True:
                # Install VBox first
                fRc = oGuestOsHlp.installVirtualBox(oSession, oTxsSession);
                if fRc is True:
                    fRc = oGuestOsHlp.configureAutostart(oSession, oTxsSession, 'allow',
                                                         (sTestUserAllow,), (sTestUserDeny,));
                    if fRc is True:
                        # Create a VM with autostart enabled in the guest for both users
                        fRc = oGuestOsHlp.createTestVM(oSession, oTxsSession, sTestUserAllow, sTestVmName);
                        fRc = fRc and oGuestOsHlp.createTestVM(oSession, oTxsSession, sTestUserDeny, sTestVmName);
                        if fRc is True:
                            # Reboot the guest
                            (fRc, oTxsSession) = self.txsRebootAndReconnectViaTcp(oSession, oTxsSession, cMsTimeout = 3 * 60000, \
                                                                                  fNatForwardingForTxs = True);
                            if fRc is True:
                                # Fudge factor - Allow the guest to finish starting up.
                                self.sleep(5);
                                fRc = oGuestOsHlp.checkForRunningVM(oSession, oTxsSession, sTestUserAllow, sTestVmName);
                                if fRc is False:
                                    reporter.error('Test VM is not running inside the guest for allowed user');

                                fRc = oGuestOsHlp.checkForRunningVM(oSession, oTxsSession, sTestUserDeny, sTestVmName);
                                if fRc is True:
                                    reporter.error('Test VM is running inside the guest for denied user');
                            else:
                                reporter.log('Rebooting the guest failed');
                        else:
                            reporter.log('Creating test VM failed');
                    else:
                        reporter.log('Configuring autostart in the guest failed');
                else:
                    reporter.log('Installing VirtualBox in the guest failed');
            else:
                reporter.log('Creating test users failed');
        else:
            reporter.log('Guest OS helper not created for VM %s' % (sVmName));
            fRc = False;

        reporter.testDone(not fRc);
        return fRc;

    def testAutostartOneCfg(self, sVmName):
        """
        Runs the specified VM thru test #1.

        Returns a success indicator on the general test execution. This is not
        the actual test result.
        """
        oVM = self.getVmByName(sVmName);

        # Reconfigure the VM
        fRc = True;
        oSession = self.openSession(oVM);
        if oSession is not None:
            fRc = fRc and oSession.enableVirtEx(True);
            fRc = fRc and oSession.enableNestedPaging(True);
            fRc = fRc and oSession.saveSettings();
            fRc = oSession.close() and fRc and True; # pychecker hack.
            oSession = None;
        else:
            fRc = False;

        # Start up.
        if fRc is True:
            self.logVmInfo(oVM);
            oSession, oTxsSession = self.startVmAndConnectToTxsViaTcp(sVmName, fCdWait = False, fNatForwardingForTxs = True);
            if oSession is not None:
                self.addTask(oTxsSession);

                # Fudge factor - Allow the guest to finish starting up.
                self.sleep(5);

                fRc = self.testAutostartRunProgs(oSession, oTxsSession, sVmName);

                # cleanup.
                self.removeTask(oTxsSession);
                self.terminateVmBySession(oSession)
            else:
                fRc = False;
        return fRc;

    def testAutostartForOneVM(self, sVmName):
        """
        Runs one VM thru the various configurations.
        """
        reporter.testStart(sVmName);
        fRc = True;
        self.testAutostartOneCfg(sVmName);
        reporter.testDone();
        return fRc;

    def testAutostart(self):
        """
        Executes autostart test.
        """

        # Loop thru the test VMs.
        for sVM in self.asTestVMs:
            # run test on the VM.
            if not self.testAutostartForOneVM(sVM):
                fRc = False;
            else:
                fRc = True;

        return fRc;



if __name__ == '__main__':
    sys.exit(tdAutostart().main(sys.argv));

