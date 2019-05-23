#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VirtualBox Installer Wrapper Driver.

This installs VirtualBox, starts a sub driver which does the real testing,
and then uninstall VirtualBox afterwards.  This reduces the complexity of the
other VBox test drivers.
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
import re
#import socket
import tempfile
import time

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from common             import utils, webutils;
from common.constants   import rtexitcode;
from testdriver         import reporter;
from testdriver.base    import TestDriverBase;



class VBoxInstallerTestDriver(TestDriverBase):
    """
    Implementation of a top level test driver.
    """


    ## State file indicating that we've skipped installation.
    ksVar_Skipped = 'vboxinstaller-skipped';


    def __init__(self):
        TestDriverBase.__init__(self);
        self._asSubDriver   = [];   # The sub driver and it's arguments.
        self._asBuildUrls   = [];   # The URLs passed us on the command line.
        self._asBuildFiles  = [];   # The downloaded file names.
        self._fAutoInstallPuelExtPack = True;

    #
    # Base method we override
    #

    def showUsage(self):
        rc = TestDriverBase.showUsage(self);
        #             0         1         2         3         4         5         6         7         8
        #             012345678901234567890123456789012345678901234567890123456789012345678901234567890
        reporter.log('');
        reporter.log('vboxinstaller Options:');
        reporter.log('  --vbox-build    <url[,url2[,..]]>');
        reporter.log('      Comma separated list of URL to file to download and install or/and');
        reporter.log('      unpack.  URLs without a schema are assumed to be files on the');
        reporter.log('      build share and will be copied off it.');
        reporter.log('  --no-puel-extpack');
        reporter.log('      Indicates that the PUEL extension pack should not be installed if found.');
        reporter.log('      The default is to install it if found in the vbox-build.');
        reporter.log('  --');
        reporter.log('      Indicates the end of our parameters and the start of the sub');
        reporter.log('      testdriver and its arguments.');
        return rc;

    def parseOption(self, asArgs, iArg):
        """
        Parse our arguments.
        """
        if asArgs[iArg] == '--':
            # End of our parameters and start of the sub driver invocation.
            iArg = self.requireMoreArgs(1, asArgs, iArg);
            assert not self._asSubDriver;
            self._asSubDriver = asArgs[iArg:];
            self._asSubDriver[0] = self._asSubDriver[0].replace('/', os.path.sep);
            iArg = len(asArgs) - 1;
        elif asArgs[iArg] == '--vbox-build':
            # List of files to copy/download and install.
            iArg = self.requireMoreArgs(1, asArgs, iArg);
            self._asBuildUrls = asArgs[iArg].split(',');
        elif asArgs[iArg] == '--no-puel-extpack':
            self._fAutoInstallPuelExtPack = False;
        elif asArgs[iArg] == '--puel-extpack':
            self._fAutoInstallPuelExtPack = True;
        else:
            return TestDriverBase.parseOption(self, asArgs, iArg);
        return iArg + 1;

    def completeOptions(self):
        #
        # Check that we've got what we need.
        #
        if not self._asBuildUrls:
            reporter.error('No build files specfiied ("--vbox-build file1[,file2[...]]")');
            return False;
        if not self._asSubDriver:
            reporter.error('No sub testdriver specified. (" -- test/stuff/tdStuff1.py args")');
            return False;

        #
        # Construct _asBuildFiles as an array parallel to _asBuildUrls.
        #
        for sUrl in self._asBuildUrls:
            sDstFile = os.path.join(self.sScratchPath, webutils.getFilename(sUrl));
            self._asBuildFiles.append(sDstFile);

        return TestDriverBase.completeOptions(self);

    def actionExtract(self):
        reporter.error('vboxinstall does not support extracting resources, you have to do that using the sub testdriver.');
        return False;

    def actionCleanupBefore(self):
        """
        Kills all VBox process we see.

        This is only supposed to execute on a testbox so we don't need to go
        all complicated wrt other users.
        """
        return self._killAllVBoxProcesses();

    def actionConfig(self):
        """
        Install VBox and pass on the configure request to the sub testdriver.
        """
        fRc = self._installVBox();
        if fRc is None: self._persistentVarSet(self.ksVar_Skipped, 'true');
        else:           self._persistentVarUnset(self.ksVar_Skipped);

        ## @todo vbox.py still has bugs preventing us from invoking it seperately with each action.
        if fRc is True and 'execute' not in self.asActions and 'all' not in self.asActions:
            fRc = self._executeSubDriver([ 'verify', ]);
        if fRc is True and 'execute' not in self.asActions and 'all' not in self.asActions:
            fRc = self._executeSubDriver([ 'config', ]);
        return fRc;

    def actionExecute(self):
        """
        Execute the sub testdriver.
        """
        return self._executeSubDriver(self.asActions);

    def actionCleanupAfter(self):
        """
        Forward this to the sub testdriver, then uninstall VBox.
        """
        fRc = True;
        if 'execute' not in self.asActions and 'all' not in self.asActions:
            fRc = self._executeSubDriver([ 'cleanup-after', ], fMaySkip = False);

        if not self._killAllVBoxProcesses():
            fRc = False;

        if not self._uninstallVBox(self._persistentVarExists(self.ksVar_Skipped)):
            fRc = False;

        if utils.getHostOs() == 'darwin':
            self._darwinUnmountDmg(fIgnoreError = True); # paranoia

        if not TestDriverBase.actionCleanupAfter(self):
            fRc = False;

        return fRc;


    def actionAbort(self):
        """
        Forward this to the sub testdriver first, then wipe all VBox like
        processes, and finally do the pid file processing (again).
        """
        fRc1 = self._executeSubDriver([ 'abort', ], fMaySkip = False);
        fRc2 = self._killAllVBoxProcesses();
        fRc3 = TestDriverBase.actionAbort(self);
        return fRc1 and fRc2 and fRc3;


    #
    # Persistent variables.
    #
    ## @todo integrate into the base driver. Persisten accross scratch wipes?

    def __persistentVarCalcName(self, sVar):
        """Returns the (full) filename for the given persistent variable."""
        assert re.match(r'^[a-zA-Z0-9_-]*$', sVar) is not None;
        return os.path.join(self.sScratchPath, 'persistent-%s.var' % (sVar,));

    def _persistentVarSet(self, sVar, sValue = ''):
        """
        Sets a persistent variable.

        Returns True on success, False + reporter.error on failure.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        sFull = self.__persistentVarCalcName(sVar);
        try:
            oFile = open(sFull, 'w');
            if sValue:
                oFile.write(sValue.encode('utf-8'));
            oFile.close();
        except:
            reporter.errorXcpt('Error creating "%s"' % (sFull,));
            return False;
        return True;

    def _persistentVarUnset(self, sVar):
        """
        Unsets a persistent variable.

        Returns True on success, False + reporter.error on failure.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        sFull = self.__persistentVarCalcName(sVar);
        if os.path.exists(sFull):
            try:
                os.unlink(sFull);
            except:
                reporter.errorXcpt('Error unlinking "%s"' % (sFull,));
                return False;
        return True;

    def _persistentVarExists(self, sVar):
        """
        Checks if a persistent variable exists.

        Returns true/false.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        return os.path.exists(self.__persistentVarCalcName(sVar));

    def _persistentVarGet(self, sVar):
        """
        Gets the value of a persistent variable.

        Returns variable value on success.
        Returns None if the variable doesn't exist or if an
        error (reported) occured.

        May raise exception if the variable name is invalid or something
        unexpected happens.
        """
        sFull = self.__persistentVarCalcName(sVar);
        if not os.path.exists(sFull):
            return None;
        try:
            oFile = open(sFull, 'r');
            sValue = oFile.read().decode('utf-8');
            oFile.close();
        except:
            reporter.errorXcpt('Error creating "%s"' % (sFull,));
            return None;
        return sValue;


    #
    # Helpers.
    #

    def _killAllVBoxProcesses(self):
        """
        Kills all virtual box related processes we find in the system.
        """

        for iIteration in range(22):
            # Gather processes to kill.
            aoTodo = [];
            for oProcess in utils.processListAll():
                sBase = oProcess.getBaseImageNameNoExeSuff();
                if sBase is None:
                    continue;
                sBase = sBase.lower();
                if sBase in [ 'vboxsvc', 'virtualbox', 'virtualboxvm', 'vboxheadless', 'vboxmanage', 'vboxsdl', 'vboxwebsrv',
                              'vboxautostart', 'vboxballoonctrl', 'vboxbfe', 'vboxextpackhelperapp', 'vboxnetdhcp',
                              'vboxnetadpctl', 'vboxtestogl', 'vboxtunctl', 'vboxvmmpreload', 'vboxxpcomipcd', 'vmCreator', ]:
                    aoTodo.append(oProcess);
                if sBase.startswith('virtualbox-') and sBase.endswith('-multiarch.exe'):
                    aoTodo.append(oProcess);
                if iIteration in [0, 21]  and  sBase in [ 'windbg', 'gdb', 'gdb-i386-apple-darwin', ]:
                    reporter.log('Warning: debugger running: %s (%s)' % (oProcess.iPid, sBase,));
            if not aoTodo:
                return True;

            # Kill.
            for oProcess in aoTodo:
                reporter.log('Loop #%d - Killing %s (%s, uid=%s)'
                             % ( iIteration, oProcess.iPid, oProcess.sImage if oProcess.sName is None else oProcess.sName,
                                 oProcess.iUid, ));
                utils.processKill(oProcess.iPid); # No mercy.

            # Check if they're all dead like they should be.
            time.sleep(0.1);
            for oProcess in aoTodo:
                if utils.processExists(oProcess.iPid):
                    time.sleep(2);
                    break;

        return False;

    def _executeSync(self, asArgs, fMaySkip = False):
        """
        Executes a child process synchronously.

        Returns True if the process executed successfully and returned 0.
        Returns None if fMaySkip is true and the child exits with RTEXITCODE_SKIPPED.
        Returns False for all other cases.
        """
        reporter.log('Executing: %s' % (asArgs, ));
        reporter.flushall();
        try:
            iRc = utils.processCall(asArgs, shell = False, close_fds = False);
        except:
            reporter.errorXcpt();
            return False;
        reporter.log('Exit code: %s (%s)' % (iRc, asArgs));
        if fMaySkip and iRc == rtexitcode.RTEXITCODE_SKIPPED:
            return None;
        return iRc is 0;

    def _sudoExecuteSync(self, asArgs):
        """
        Executes a sudo child process synchronously.
        Returns a tuple [True, 0] if the process executed successfully
        and returned 0, otherwise [False, rc] is returned.
        """
        reporter.log('Executing [sudo]: %s' % (asArgs, ));
        reporter.flushall();
        iRc = 0;
        try:
            iRc = utils.sudoProcessCall(asArgs, shell = False, close_fds = False);
        except:
            reporter.errorXcpt();
            return (False, 0);
        reporter.log('Exit code [sudo]: %s (%s)' % (iRc, asArgs));
        return (iRc is 0, iRc);

    def _executeSubDriver(self, asActions, fMaySkip = True):
        """
        Execute the sub testdriver with the specified action.
        """
        asArgs = list(self._asSubDriver)
        asArgs.append('--no-wipe-clean');
        asArgs.extend(asActions);
        return self._executeSync(asArgs, fMaySkip = fMaySkip);

    def _maybeUnpackArchive(self, sMaybeArchive, fNonFatal = False):
        """
        Attempts to unpack the given build file.
        Updates _asBuildFiles.
        Returns True/False. No exceptions.
        """
        def unpackFilter(sMember):
            # type: (string) -> bool
            """ Skips debug info. """
            sLower = sMember.lower();
            if sLower.endswith('.pdb'):
                return False;
            return True;

        asMembers = utils.unpackFile(sMaybeArchive, self.sScratchPath, reporter.log,
                                     reporter.log if fNonFatal else reporter.error,
                                     fnFilter = unpackFilter);
        if asMembers is None:
            return False;
        self._asBuildFiles.extend(asMembers);
        return True;


    def _installVBox(self):
        """
        Download / copy the build files into the scratch area and install them.
        """
        reporter.testStart('Installing VirtualBox');
        reporter.log('CWD=%s' % (os.getcwd(),)); # curious

        #
        # Download the build files.
        #
        for i in range(len(self._asBuildUrls)):
            if webutils.downloadFile(self._asBuildUrls[i], self._asBuildFiles[i],
                                     self.sBuildPath, reporter.log, reporter.log) is not True:
                reporter.testDone(fSkipped = True);
                return None; # Failed to get binaries, probably deleted. Skip the test run.

        #
        # Unpack anything we know what is and append it to the build files
        # list.  This allows us to use VBoxAll*.tar.gz files.
        #
        for sFile in list(self._asBuildFiles):
            if self._maybeUnpackArchive(sFile, fNonFatal = True) is not True:
                reporter.testDone(fSkipped = True);
                return None; # Failed to unpack. Probably local error, like busy
                             # DLLs on windows, no reason for failing the build.

        #
        # Go to system specific installation code.
        #
        sHost = utils.getHostOs()
        if   sHost == 'darwin':     fRc = self._installVBoxOnDarwin();
        elif sHost == 'linux':      fRc = self._installVBoxOnLinux();
        elif sHost == 'solaris':    fRc = self._installVBoxOnSolaris();
        elif sHost == 'win':        fRc = self._installVBoxOnWindows();
        else:
            reporter.error('Unsupported host "%s".' % (sHost,));
        if fRc is False:
            reporter.testFailure('Installation error.');

        #
        # Install the extension pack.
        #
        if fRc is True  and  self._fAutoInstallPuelExtPack:
            fRc = self._installExtPack();
            if fRc is False:
                reporter.testFailure('Extension pack installation error.');

        # Some debugging...
        try:
            cMbFreeSpace = utils.getDiskUsage(self.sScratchPath);
            reporter.log('Disk usage after VBox install: %d MB available at %s' % (cMbFreeSpace, self.sScratchPath,));
        except:
            reporter.logXcpt('Unable to get disk free space. Ignored. Continuing.');

        reporter.testDone();
        return fRc;

    def _uninstallVBox(self, fIgnoreError = False):
        """
        Uninstall VirtualBox.
        """
        reporter.testStart('Uninstalling VirtualBox');

        sHost = utils.getHostOs()
        if   sHost == 'darwin':     fRc = self._uninstallVBoxOnDarwin();
        elif sHost == 'linux':      fRc = self._uninstallVBoxOnLinux();
        elif sHost == 'solaris':    fRc = self._uninstallVBoxOnSolaris();
        elif sHost == 'win':        fRc = self._uninstallVBoxOnWindows(True);
        else:
            reporter.error('Unsupported host "%s".' % (sHost,));
        if fRc is False and not fIgnoreError:
            reporter.testFailure('Uninstallation failed.');

        fRc2 = self._uninstallAllExtPacks();
        if not fRc2 and fRc:
            fRc = fRc2;

        reporter.testDone(fSkipped = (fRc is None));
        return fRc;

    def _findFile(self, sRegExp, fMandatory = False):
        """
        Returns the first build file that matches the given regular expression
        (basename only).

        Returns None if no match was found, logging it as an error if
        fMandatory is set.
        """
        oRegExp = re.compile(sRegExp);

        for sFile in self._asBuildFiles:
            if oRegExp.match(os.path.basename(sFile)) and os.path.exists(sFile):
                return sFile;

        if fMandatory:
            reporter.error('Failed to find a file matching "%s" in %s.' % (sRegExp, self._asBuildFiles,));
        return None;

    def _waitForTestManagerConnectivity(self, cSecTimeout):
        """
        Check and wait for network connectivity to the test manager.

        This is used with the windows installation and uninstallation since
        these usually disrupts network connectivity when installing the filter
        driver.  If we proceed to quickly, we might finish the test at a time
        when we cannot report to the test manager and thus end up with an
        abandonded test error.
        """
        cSecElapsed = 0;
        secStart    = utils.timestampSecond();
        while reporter.checkTestManagerConnection() is False:
            cSecElapsed = utils.timestampSecond() - secStart;
            if cSecElapsed >= cSecTimeout:
                reporter.log('_waitForTestManagerConnectivity: Giving up after %u secs.' % (cSecTimeout,));
                return False;
            time.sleep(2);

        if cSecElapsed > 0:
            reporter.log('_waitForTestManagerConnectivity: Waited %s secs.' % (cSecTimeout,));
        return True;


    #
    # Darwin (Mac OS X).
    #

    def _darwinDmgPath(self):
        """ Returns the path to the DMG mount."""
        return os.path.join(self.sScratchPath, 'DmgMountPoint');

    def _darwinUnmountDmg(self, fIgnoreError):
        """
        Umount any DMG on at the default mount point.
        """
        sMountPath = self._darwinDmgPath();
        if not os.path.exists(sMountPath):
            return True;

        # Unmount.
        fRc = self._executeSync(['hdiutil', 'detach', sMountPath ]);
        if not fRc and not fIgnoreError:
            reporter.error('Failed to unmount DMG at %s' % sMountPath);

        # Remove dir.
        try:
            os.rmdir(sMountPath);
        except:
            if not fIgnoreError:
                reporter.errorXcpt('Failed to remove directory %s' % sMountPath);
        return fRc;

    def _darwinMountDmg(self, sDmg):
        """
        Mount the DMG at the default mount point.
        """
        self._darwinUnmountDmg(fIgnoreError = True)

        sMountPath = self._darwinDmgPath();
        if not os.path.exists(sMountPath):
            try:
                os.mkdir(sMountPath, 0755);
            except:
                reporter.logXcpt();
                return False;

        return self._executeSync(['hdiutil', 'attach', '-readonly', '-mount', 'required', '-mountpoint', sMountPath, sDmg, ]);

    def _installVBoxOnDarwin(self):
        """ Installs VBox on Mac OS X."""
        sDmg = self._findFile('^VirtualBox-.*\\.dmg$');
        if sDmg is None:
            return False;

        # Mount the DMG.
        fRc = self._darwinMountDmg(sDmg);
        if fRc is not True:
            return False;

        # Uninstall any previous vbox version first.
        sUninstaller = os.path.join(self._darwinDmgPath(), 'VirtualBox_Uninstall.tool');
        fRc, _ = self._sudoExecuteSync([sUninstaller, '--unattended',]);
        if fRc is True:

            # Install the package.
            sPkg = os.path.join(self._darwinDmgPath(), 'VirtualBox.pkg');
            fRc, _ = self._sudoExecuteSync(['installer', '-verbose', '-dumplog', '-pkg', sPkg, '-target', '/']);

        # Unmount the DMG and we're done.
        if not self._darwinUnmountDmg(fIgnoreError = False):
            fRc = False;
        return fRc;

    def _uninstallVBoxOnDarwin(self):
        """ Uninstalls VBox on Mac OS X."""

        # Is VirtualBox installed? If not, don't try uninstall it.
        sVBox = self._getVBoxInstallPath(fFailIfNotFound = False);
        if sVBox is None:
            return True;

        # Find the dmg.
        sDmg = self._findFile('^VirtualBox-.*\\.dmg$');
        if sDmg is None:
            return False;
        if not os.path.exists(sDmg):
            return True;

        # Mount the DMG.
        fRc = self._darwinMountDmg(sDmg);
        if fRc is not True:
            return False;

        # Execute the uninstaller.
        sUninstaller = os.path.join(self._darwinDmgPath(), 'VirtualBox_Uninstall.tool');
        fRc, _ = self._sudoExecuteSync([sUninstaller, '--unattended',]);

        # Unmount the DMG and we're done.
        if not self._darwinUnmountDmg(fIgnoreError = False):
            fRc = False;
        return fRc;

    #
    # GNU/Linux
    #

    def _installVBoxOnLinux(self):
        """ Installs VBox on Linux."""
        sRun = self._findFile('^VirtualBox-.*\\.run$');
        if sRun is None:
            return False;
        utils.chmodPlusX(sRun);

        # Install the new one.
        fRc, _ = self._sudoExecuteSync([sRun,]);
        return fRc;

    def _uninstallVBoxOnLinux(self):
        """ Uninstalls VBox on Linux."""

        # Is VirtualBox installed? If not, don't try uninstall it.
        sVBox = self._getVBoxInstallPath(fFailIfNotFound = False);
        if sVBox is None:
            return True;

        # Find the .run file and use it.
        sRun = self._findFile('^VirtualBox-.*\\.run$', fMandatory = False);
        if sRun is not None:
            utils.chmodPlusX(sRun);
            fRc, _ = self._sudoExecuteSync([sRun, 'uninstall']);
            return fRc;

        # Try the installed uninstaller.
        for sUninstaller in [os.path.join(sVBox, 'uninstall.sh'), '/opt/VirtualBox/uninstall.sh', ]:
            if os.path.isfile(sUninstaller):
                reporter.log('Invoking "%s"...' % (sUninstaller,));
                fRc, _ = self._sudoExecuteSync([sUninstaller, 'uninstall']);
                return fRc;

        reporter.log('Did not find any VirtualBox install to uninstall.');
        return True;


    #
    # Solaris
    #

    def _generateAutoResponseOnSolaris(self):
        """
        Generates an autoresponse file on solaris, returning the name.
        None is return on failure.
        """
        sPath = os.path.join(self.sScratchPath, 'SolarisAutoResponse');
        oFile = utils.openNoInherit(sPath, 'wt');
        oFile.write('basedir=default\n'
                    'runlevel=nocheck\n'
                    'conflict=quit\n'
                    'setuid=nocheck\n'
                    'action=nocheck\n'
                    'partial=quit\n'
                    'instance=unique\n'
                    'idepend=quit\n'
                    'rdepend=quit\n'
                    'space=quit\n'
                    'mail=\n');
        oFile.close();
        return sPath;

    def _installVBoxOnSolaris(self):
        """ Installs VBox on Solaris."""
        sPkg = self._findFile('^VirtualBox-.*\\.pkg$', fMandatory = False);
        if sPkg is None:
            sTar = self._findFile('^VirtualBox-.*-SunOS-.*\\.tar.gz$', fMandatory = False);
            if sTar is not None:
                if self._maybeUnpackArchive(sTar) is not True:
                    return False;
        sPkg = self._findFile('^VirtualBox-.*\\.pkg$', fMandatory = True);
        sRsp = self._findFile('^autoresponse$', fMandatory = True);
        if sPkg is None or sRsp is None:
            return False;

        # Uninstall first (ignore result).
        self._uninstallVBoxOnSolaris();

        # Install the new one.
        fRc, _ = self._sudoExecuteSync(['pkgadd', '-d', sPkg, '-n', '-a', sRsp, 'SUNWvbox']);
        return fRc;

    def _uninstallVBoxOnSolaris(self):
        """ Uninstalls VBox on Solaris."""
        reporter.flushall();
        if utils.processCall(['pkginfo', '-q', 'SUNWvbox']) != 0:
            return True;
        sRsp = self._generateAutoResponseOnSolaris();
        fRc, _ = self._sudoExecuteSync(['pkgrm', '-n', '-a', sRsp, 'SUNWvbox']);
        return fRc;

    #
    # Windows
    #

    ## VBox windows services we can query the status of.
    kasWindowsServices = [ 'vboxdrv', 'vboxusbmon', 'vboxnetadp', 'vboxnetflt', 'vboxnetlwf' ];

    def _installVBoxOnWindows(self):
        """ Installs VBox on Windows."""
        sExe = self._findFile('^VirtualBox-.*-(MultiArch|Win).exe$');
        if sExe is None:
            return False;

        # TEMPORARY HACK - START
        # It seems that running the NDIS cleanup script upon uninstallation is not
        # a good idea, so let's run it before installing VirtualBox.
        #sHostName = socket.getfqdn();
        #if    not sHostName.startswith('testboxwin3') \
        #  and not sHostName.startswith('testboxharp2') \
        #  and not sHostName.startswith('wei01-b6ka-3') \
        #  and utils.getHostOsVersion() in ['8', '8.1', '9', '2008Server', '2008ServerR2', '2012Server']:
        #    reporter.log('Peforming extra NDIS cleanup...');
        #    sMagicScript = os.path.abspath(os.path.join(g_ksValidationKitDir, 'testdriver', 'win-vbox-net-uninstall.ps1'));
        #    fRc2, _ = self._sudoExecuteSync(['powershell.exe', '-Command', 'set-executionpolicy unrestricted']);
        #    if not fRc2:
        #        reporter.log('set-executionpolicy failed.');
        #    self._sudoExecuteSync(['powershell.exe', '-Command', 'get-executionpolicy']);
        #    fRc2, _ = self._sudoExecuteSync(['powershell.exe', '-File', sMagicScript]);
        #    if not fRc2:
        #        reporter.log('NDIS cleanup failed.');
        # TEMPORARY HACK - END

        # Uninstall any previous vbox version first.
        fRc = self._uninstallVBoxOnWindows(True);
        if fRc is not True:
            return None; # There shouldn't be anything to uninstall, and if there is, it's not our fault.

        # Install the new one.
        asArgs = [sExe, '-vvvv', '--silent', '--logging'];
        asArgs.extend(['--msiparams', 'REBOOT=ReallySuppress']);
        sVBoxInstallPath = os.environ.get('VBOX_INSTALL_PATH', None);
        if sVBoxInstallPath is not None:
            asArgs.extend(['INSTALLDIR="%s"' % (sVBoxInstallPath,)]);
        fRc2, iRc = self._sudoExecuteSync(asArgs);
        if fRc2 is False:
            if iRc == 3010: # ERROR_SUCCESS_REBOOT_REQUIRED
                reporter.log('Note: Installer required a reboot to complete installation');
                # Optional, don't fail.
            else:
                fRc = False;
        sLogFile = os.path.join(tempfile.gettempdir(), 'VirtualBox', 'VBoxInstallLog.txt');
        if      sLogFile is not None \
            and os.path.isfile(sLogFile):
            reporter.addLogFile(sLogFile, 'log/installer', "Verbose MSI installation log file");
        self._waitForTestManagerConnectivity(30);
        return fRc;

    def _isProcessPresent(self, sName):
        """ Checks whether the named process is present or not. """
        for oProcess in utils.processListAll():
            sBase = oProcess.getBaseImageNameNoExeSuff();
            if sBase is not None and sBase.lower() == sName:
                return True;
        return False;

    def _killProcessesByName(self, sName, sDesc, fChildren = False):
        """ Kills the named process, optionally including children. """
        cKilled = 0;
        aoProcesses = utils.processListAll();
        for oProcess in aoProcesses:
            sBase = oProcess.getBaseImageNameNoExeSuff();
            if sBase is not None and sBase.lower() == sName:
                reporter.log('Killing %s process: %s (%s)' % (sDesc, oProcess.iPid, sBase));
                utils.processKill(oProcess.iPid);
                cKilled += 1;

                if fChildren:
                    for oChild in aoProcesses:
                        if oChild.iParentPid == oProcess.iPid and oChild.iParentPid is not None:
                            reporter.log('Killing %s child process: %s (%s)' % (sDesc, oChild.iPid, sBase));
                            utils.processKill(oChild.iPid);
                            cKilled += 1;
        return cKilled;

    def _terminateProcessesByNameAndArgSubstr(self, sName, sArg, sDesc):
        """
        Terminates the named process using taskkill.exe, if any of its args
        contains the passed string.
        """
        cKilled = 0;
        aoProcesses = utils.processListAll();
        for oProcess in aoProcesses:
            sBase = oProcess.getBaseImageNameNoExeSuff();
            if sBase is not None and sBase.lower() == sName and any(sArg in s for s in oProcess.asArgs):

                reporter.log('Killing %s process: %s (%s)' % (sDesc, oProcess.iPid, sBase));
                self._executeSync(['taskkill.exe', '/pid', '%u' % (oProcess.iPid,)]);
                cKilled += 1;
        return cKilled;

    def _uninstallVBoxOnWindows(self, fIgnoreServices = False):
        """
        Uninstalls VBox on Windows, all installations we find to be on the safe side...
        """

        import win32com.client; # pylint: disable=F0401
        win32com.client.gencache.EnsureModule('{000C1092-0000-0000-C000-000000000046}', 1033, 1, 0);
        oInstaller = win32com.client.Dispatch('WindowsInstaller.Installer',
                                              resultCLSID = '{000C1090-0000-0000-C000-000000000046}')

        # Search installed products for VirtualBox.
        asProdCodes = [];
        for sProdCode in oInstaller.Products:
            try:
                sProdName = oInstaller.ProductInfo(sProdCode, "ProductName");
            except:
                reporter.logXcpt();
                continue;
            #reporter.log('Info: %s=%s' % (sProdCode, sProdName));
            if  sProdName.startswith('Oracle VM VirtualBox') \
             or sProdName.startswith('Sun VirtualBox'):
                asProdCodes.append([sProdCode, sProdName]);

        # Before we start uninstalling anything, just ruthlessly kill any cdb,
        # msiexec, drvinst and some rundll process we might find hanging around.
        if self._isProcessPresent('rundll32'):
            cTimes = 0;
            while cTimes < 3:
                cTimes += 1;
                cKilled = self._terminateProcessesByNameAndArgSubstr('rundll32', 'InstallSecurityPromptRunDllW',
                                                                     'MSI driver installation');
                if cKilled <= 0:
                    break;
                time.sleep(10); # Give related drvinst process a chance to clean up after we killed the verification dialog.

        if self._isProcessPresent('drvinst'):
            time.sleep(15);     # In the hope that it goes away.
            cTimes = 0;
            while cTimes < 4:
                cTimes += 1;
                cKilled = self._killProcessesByName('drvinst', 'MSI driver installation', True);
                if cKilled <= 0:
                    break;
                time.sleep(10); # Give related MSI process a chance to clean up after we killed the driver installer.

        if self._isProcessPresent('msiexec'):
            cTimes = 0;
            while cTimes < 3:
                reporter.log('found running msiexec process, waiting a bit...');
                time.sleep(20)  # In the hope that it goes away.
                if not self._isProcessPresent('msiexec'):
                    break;
                cTimes += 1;
            ## @todo this could also be the msiexec system service, try to detect this case!
            if cTimes >= 6:
                cKilled = self._killProcessesByName('msiexec', 'MSI driver installation');
                if cKilled > 0:
                    time.sleep(16); # fudge.

        # cdb.exe sometimes stays running (from utils.getProcessInfo), blocking
        # the scratch directory. No idea why.
        if self._isProcessPresent('cdb'):
            cTimes = 0;
            while cTimes < 3:
                cKilled = self._killProcessesByName('cdb', 'cdb.exe from getProcessInfo');
                if cKilled <= 0:
                    break;
                time.sleep(2); # fudge.

        # Do the uninstalling.
        fRc = True;
        sLogFile = os.path.join(self.sScratchPath, 'VBoxUninstallLog.txt');
        for sProdCode, sProdName in asProdCodes:
            reporter.log('Uninstalling %s (%s)...' % (sProdName, sProdCode));
            fRc2, iRc = self._sudoExecuteSync(['msiexec', '/uninstall', sProdCode, '/quiet', '/passive', '/norestart',
                                               '/L*v', '%s' % (sLogFile), ]);
            if fRc2 is False:
                if iRc == 3010: # ERROR_SUCCESS_REBOOT_REQUIRED
                    reporter.log('Note: Uninstaller required a reboot to complete uninstallation');
                    # Optional, don't fail.
                else:
                    fRc = False;
            reporter.addLogFile(sLogFile, 'log/uninstaller', "Verbose MSI uninstallation log file");

        self._waitForTestManagerConnectivity(30);
        if fRc is False and os.path.isfile(sLogFile):
            reporter.addLogFile(sLogFile, 'log/uninstaller');

        # Log driver service states (should ls \Driver\VBox* and \Device\VBox*).
        for sService in self.kasWindowsServices:
            fRc2, _ = self._sudoExecuteSync(['sc.exe', 'query', sService]);
            if fIgnoreServices is False and fRc2 is True:
                fRc = False

        return fRc;


    #
    # Extension pack.
    #

    def _getVBoxInstallPath(self, fFailIfNotFound):
        """ Returns the default VBox installation path. """
        sHost = utils.getHostOs();
        if sHost == 'win':
            sProgFiles = os.environ.get('ProgramFiles', 'C:\\Program Files');
            asLocs = [
                os.path.join(sProgFiles, 'Oracle', 'VirtualBox'),
                os.path.join(sProgFiles, 'OracleVM', 'VirtualBox'),
                os.path.join(sProgFiles, 'Sun', 'VirtualBox'),
            ];
        elif sHost == 'linux' or sHost == 'solaris':
            asLocs = [ '/opt/VirtualBox', '/opt/VirtualBox-3.2', '/opt/VirtualBox-3.1', '/opt/VirtualBox-3.0'];
        elif sHost == 'darwin':
            asLocs = [ '/Applications/VirtualBox.app/Contents/MacOS' ];
        else:
            asLocs = [ '/opt/VirtualBox' ];
        if 'VBOX_INSTALL_PATH' in os.environ:
            asLocs.insert(0, os.environ.get('VBOX_INSTALL_PATH', None));

        for sLoc in asLocs:
            if os.path.isdir(sLoc):
                return sLoc;
        if fFailIfNotFound:
            reporter.error('Failed to locate VirtualBox installation: %s' % (asLocs,));
        else:
            reporter.log2('Failed to locate VirtualBox installation: %s' % (asLocs,));
        return None;

    def _installExtPack(self):
        """ Installs the extension pack. """
        sVBox = self._getVBoxInstallPath(fFailIfNotFound = True);
        if sVBox is None:
            return False;
        sExtPackDir = os.path.join(sVBox, 'ExtensionPacks');

        if self._uninstallAllExtPacks() is not True:
            return False;

        sExtPack = self._findFile('Oracle_VM_VirtualBox_Extension_Pack.vbox-extpack');
        if sExtPack is None:
            sExtPack = self._findFile('Oracle_VM_VirtualBox_Extension_Pack.*.vbox-extpack');
        if sExtPack is None:
            return True;

        sDstDir = os.path.join(sExtPackDir, 'Oracle_VM_VirtualBox_Extension_Pack');
        reporter.log('Installing extension pack "%s" to "%s"...' % (sExtPack, sExtPackDir));
        fRc, _ = self._sudoExecuteSync([ self.getBinTool('vts_tar'),
                                         '--extract',
                                         '--verbose',
                                         '--gzip',
                                         '--file',                sExtPack,
                                         '--directory',           sDstDir,
                                         '--file-mode-and-mask',  '0644',
                                         '--file-mode-or-mask',   '0644',
                                         '--dir-mode-and-mask',   '0755',
                                         '--dir-mode-or-mask',    '0755',
                                         '--owner',               '0',
                                         '--group',               '0',
                                       ]);
        return fRc;

    def _uninstallAllExtPacks(self):
        """ Uninstalls all extension packs. """
        sVBox = self._getVBoxInstallPath(fFailIfNotFound = False);
        if sVBox is None:
            return True;

        sExtPackDir = os.path.join(sVBox, 'ExtensionPacks');
        if not os.path.exists(sExtPackDir):
            return True;

        fRc, _ = self._sudoExecuteSync([self.getBinTool('vts_rm'), '-Rfv', '--', sExtPackDir]);
        return fRc;



if __name__ == '__main__':
    sys.exit(VBoxInstallerTestDriver().main(sys.argv));

