#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdStorageSnapshotMerging1.py $

"""
VirtualBox Validation Kit - Storage snapshotting and merging testcase.
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
__version__ = "$Id: tdStorageSnapshotMerging1.py $"


# Standard Python imports.
import os;
import sys;
import uuid;

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

def _ControllerTypeToName(eControllerType):
    """ Translate a controller type to a name. """
    if eControllerType == vboxcon.StorageControllerType_PIIX3 or eControllerType == vboxcon.StorageControllerType_PIIX4:
        sType = "IDE Controller";
    elif eControllerType == vboxcon.StorageControllerType_IntelAhci:
        sType = "SATA Controller";
    elif eControllerType == vboxcon.StorageControllerType_LsiLogicSas:
        sType = "SAS Controller";
    elif eControllerType == vboxcon.StorageControllerType_LsiLogic or eControllerType == vboxcon.StorageControllerType_BusLogic:
        sType = "SCSI Controller";
    else:
        sType = "Storage Controller";
    return sType;

class tdStorageSnapshot(vbox.TestDriver):                                      # pylint: disable=R0902
    """
    Storage benchmark.
    """

    def __init__(self):
        vbox.TestDriver.__init__(self);
        self.asRsrcs           = None;
        self.oGuestToGuestVM   = None;
        self.oGuestToGuestSess = None;
        self.oGuestToGuestTxs  = None;
        self.asTestVMsDef      = ['tst-win7-vhd', 'tst-debian-vhd', 'tst-debian-vdi'];
        self.asTestVMs         = self.asTestVMsDef;
        self.asSkipVMs         = [];
        self.asStorageCtrlsDef = ['AHCI', 'IDE', 'LsiLogicSAS', 'LsiLogic', 'BusLogic'];
        self.asStorageCtrls    = self.asStorageCtrlsDef;
        self.asDiskFormatsDef  = ['VDI', 'VMDK', 'VHD', 'QED', 'Parallels', 'QCOW', 'iSCSI'];
        self.asDiskFormats     = self.asDiskFormatsDef;
        self.sRndData          = os.urandom(100*1024*1024);

    #
    # Overridden methods.
    #
    def showUsage(self):
        rc = vbox.TestDriver.showUsage(self);
        reporter.log('');
        reporter.log('tdStorageSnapshot1 Options:');
        reporter.log('  --storage-ctrls <type1[:type2[:...]]>');
        reporter.log('      Default: %s' % (':'.join(self.asStorageCtrls)));
        reporter.log('  --disk-formats  <type1[:type2[:...]]>');
        reporter.log('      Default: %s' % (':'.join(self.asDiskFormats)));
        reporter.log('  --test-vms      <vm1[:vm2[:...]]>');
        reporter.log('      Test the specified VMs in the given order. Use this to change');
        reporter.log('      the execution order or limit the choice of VMs');
        reporter.log('      Default: %s  (all)' % (':'.join(self.asTestVMsDef)));
        reporter.log('  --skip-vms      <vm1[:vm2[:...]]>');
        reporter.log('      Skip the specified VMs when testing.');
        return rc;

    def parseOption(self, asArgs, iArg):                                        # pylint: disable=R0912,R0915
        if asArgs[iArg] == '--storage-ctrls':
            iArg += 1;
            if iArg >= len(asArgs):
                raise base.InvalidOption('The "--storage-ctrls" takes a colon separated list of Storage controller types');
            self.asStorageCtrls = asArgs[iArg].split(':');
        elif asArgs[iArg] == '--disk-formats':
            iArg += 1;
            if iArg >= len(asArgs): raise base.InvalidOption('The "--disk-formats" takes a colon separated list of disk formats');
            self.asDiskFormats = asArgs[iArg].split(':');
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
            if 'tst-win7-vhd' in self.asTestVMs:
                self.asRsrcs.append('4.2/storage/win7.vhd');

            if 'tst-debian-vhd' in self.asTestVMs:
                self.asRsrcs.append('4.2/storage/debian.vhd');

            if 'tst-debian-vdi' in self.asTestVMs:
                self.asRsrcs.append('4.2/storage/debian.vdi');

        return self.asRsrcs;

    def actionConfig(self):

        # Make sure vboxapi has been imported so we can use the constants.
        if not self.importVBoxApi():
            return False;

        #
        # Configure the VMs we're going to use.
        #

        # Windows VMs
        if 'tst-win7-vhd' in self.asTestVMs:
            oVM = self.createTestVM('tst-win7-vhd', 1, '4.2/storage/win7.vhd', sKind = 'Windows7', fIoApic = True, \
                                    eNic0AttachType = vboxcon.NetworkAttachmentType_NAT, \
                                    eNic0Type = vboxcon.NetworkAdapterType_Am79C973);
            if oVM is None:
                return False;

        # Linux VMs
        if 'tst-debian-vhd' in self.asTestVMs:
            oVM = self.createTestVM('tst-debian-vhd', 1, '4.2/storage/debian.vhd', sKind = 'Debian_64', fIoApic = True, \
                                    eNic0AttachType = vboxcon.NetworkAttachmentType_NAT, \
                                    eNic0Type = vboxcon.NetworkAdapterType_Am79C973);
            if oVM is None:
                return False;

        if 'tst-debian-vdi' in self.asTestVMs:
            oVM = self.createTestVM('tst-debian-vdi', 1, '4.2/storage/debian.vdi', sKind = 'Debian_64', fIoApic = True, \
                                    eNic0AttachType = vboxcon.NetworkAttachmentType_NAT, \
                                    eNic0Type = vboxcon.NetworkAdapterType_Am79C973);
            if oVM is None:
                return False;

        return True;

    def actionExecute(self):
        """
        Execute the testcase.
        """
        fRc = self.test1();
        return fRc;


    #
    # Test execution helpers.
    #

    def test1UploadFile(self, oSession, oTxsSession):
        """
        Uploads a test file to the test machine.
        """
        reporter.testStart('Upload file');

        fRc = self.txsUploadString(oSession, oTxsSession, self.sRndData, '${SCRATCH}/' + str(uuid.uuid4()), \
                                   cMsTimeout = 3600 * 1000);

        reporter.testDone(not fRc);
        return fRc;

    def test1OneCfg(self, sVmName, eStorageController, sDiskFormat):
        """
        Runs the specified VM thru test #1.

        Returns a success indicator on the general test execution. This is not
        the actual test result.
        """
        oVM = self.getVmByName(sVmName);

        # @ŧodo: Implement support for different formats.
        _ = sDiskFormat;

        # Reconfigure the VM
        fRc = True;
        oSession = self.openSession(oVM);
        if oSession is not None:
            # Attach HD
            fRc = oSession.ensureControllerAttached(_ControllerTypeToName(eStorageController));
            fRc = fRc and oSession.setStorageControllerType(eStorageController, _ControllerTypeToName(eStorageController));
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

                # Do a snapshot first.
                oSession.takeSnapshot('Base snapshot');

                for i in range(0, 10):
                    oSession.takeSnapshot('Snapshot ' + str(i));
                    self.test1UploadFile(oSession, oTxsSession);
                    msNow = base.timestampMilli();
                    oSnapshot = oSession.findSnapshot('Snapshot ' + str(i));
                    oSession.deleteSnapshot(oSnapshot.id, cMsTimeout = 60 * 1000);
                    msElapsed = base.timestampMilli() - msNow;
                    reporter.log('Deleting snapshot %d took %d ms' % (i, msElapsed));

                # cleanup.
                self.removeTask(oTxsSession);
                self.terminateVmBySession(oSession)
            else:
                fRc = False;
        return fRc;

    def test1OneVM(self, sVmName):
        """
        Runs one VM thru the various configurations.
        """
        reporter.testStart(sVmName);
        fRc = True;
        for sStorageCtrl in self.asStorageCtrls:
            reporter.testStart(sStorageCtrl);

            if sStorageCtrl == 'AHCI':
                eStorageCtrl = vboxcon.StorageControllerType_IntelAhci;
            elif sStorageCtrl == 'IDE':
                eStorageCtrl = vboxcon.StorageControllerType_PIIX4;
            elif sStorageCtrl == 'LsiLogicSAS':
                eStorageCtrl = vboxcon.StorageControllerType_LsiLogicSas;
            elif sStorageCtrl == 'LsiLogic':
                eStorageCtrl = vboxcon.StorageControllerType_LsiLogic;
            elif sStorageCtrl == 'BusLogic':
                eStorageCtrl = vboxcon.StorageControllerType_BusLogic;
            else:
                eStorageCtrl = None;

            for sDiskFormat in self.asDiskFormats:
                reporter.testStart('%s' % (sDiskFormat));
                self.test1OneCfg(sVmName, eStorageCtrl, sDiskFormat);
                reporter.testDone();
            reporter.testDone();
        reporter.testDone();
        return fRc;

    def test1(self):
        """
        Executes test #1.
        """

        # Loop thru the test VMs.
        for sVM in self.asTestVMs:
            # run test on the VM.
            if not self.test1OneVM(sVM):
                fRc = False;
            else:
                fRc = True;

        return fRc;



if __name__ == '__main__':
    sys.exit(tdStorageSnapshot().main(sys.argv));

