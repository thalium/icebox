# -*- coding: utf-8 -*-
# $Id: utils.py $
# pylint: disable=C0302

"""
Common Utility Functions.
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
__version__ = "$Revision: 118928 $"


# Standard Python imports.
import datetime;
import os;
import platform;
import re;
import stat;
import subprocess;
import sys;
import time;
import traceback;
import unittest;

if sys.platform == 'win32':
    import ctypes;
    import msvcrt;              # pylint: disable=import-error
    import win32api;            # pylint: disable=import-error
    import win32con;            # pylint: disable=import-error
    import win32console;        # pylint: disable=import-error
    import win32file;           # pylint: disable=import-error
    import win32process;        # pylint: disable=import-error
    import winerror;            # pylint: disable=import-error
    import pywintypes;          # pylint: disable=import-error
else:
    import signal;

# Python 3 hacks:
if sys.version_info[0] >= 3:
    unicode = str;  # pylint: disable=redefined-builtin,invalid-name
    xrange = range; # pylint: disable=redefined-builtin,invalid-name
    long = int;     # pylint: disable=redefined-builtin,invalid-name


#
# Host OS and CPU.
#

def getHostOs():
    """
    Gets the host OS name (short).

    See the KBUILD_OSES variable in kBuild/header.kmk for possible return values.
    """
    sPlatform = platform.system();
    if sPlatform in ('Linux', 'Darwin', 'Solaris', 'FreeBSD', 'NetBSD', 'OpenBSD'):
        sPlatform = sPlatform.lower();
    elif sPlatform == 'Windows':
        sPlatform = 'win';
    elif sPlatform == 'SunOS':
        sPlatform = 'solaris';
    else:
        raise Exception('Unsupported platform "%s"' % (sPlatform,));
    return sPlatform;

g_sHostArch = None;

def getHostArch():
    """
    Gets the host CPU architecture.

    See the KBUILD_ARCHES variable in kBuild/header.kmk for possible return values.
    """
    global g_sHostArch;
    if g_sHostArch is None:
        sArch = platform.machine();
        if sArch in ('i386', 'i486', 'i586', 'i686', 'i786', 'i886', 'x86'):
            sArch = 'x86';
        elif sArch in ('AMD64', 'amd64', 'x86_64'):
            sArch = 'amd64';
        elif sArch == 'i86pc': # SunOS
            if platform.architecture()[0] == '64bit':
                sArch = 'amd64';
            else:
                try:
                    sArch = processOutputChecked(['/usr/bin/isainfo', '-n',]);
                except:
                    pass;
                sArch = sArch.strip();
                if sArch != 'amd64':
                    sArch = 'x86';
        else:
            raise Exception('Unsupported architecture/machine "%s"' % (sArch,));
        g_sHostArch = sArch;
    return g_sHostArch;


def getHostOsDotArch():
    """
    Gets the 'os.arch' for the host.
    """
    return '%s.%s' % (getHostOs(), getHostArch());


def isValidOs(sOs):
    """
    Validates the OS name.
    """
    if sOs in ('darwin', 'dos', 'dragonfly', 'freebsd', 'haiku', 'l4', 'linux', 'netbsd', 'nt', 'openbsd', \
               'os2', 'solaris', 'win', 'os-agnostic'):
        return True;
    return False;


def isValidArch(sArch):
    """
    Validates the CPU architecture name.
    """
    if sArch in ('x86', 'amd64', 'sparc32', 'sparc64', 's390', 's390x', 'ppc32', 'ppc64', \
               'mips32', 'mips64', 'ia64', 'hppa32', 'hppa64', 'arm', 'alpha'):
        return True;
    return False;

def isValidOsDotArch(sOsDotArch):
    """
    Validates the 'os.arch' string.
    """

    asParts = sOsDotArch.split('.');
    if asParts.length() != 2:
        return False;
    return isValidOs(asParts[0]) \
       and isValidArch(asParts[1]);

def getHostOsVersion():
    """
    Returns the host OS version.  This is platform.release with additional
    distro indicator on linux.
    """
    sVersion = platform.release();
    sOs = getHostOs();
    if sOs == 'linux':
        sDist = '';
        try:
            # try /etc/lsb-release first to distinguish between Debian and Ubuntu
            oFile = open('/etc/lsb-release');
            for sLine in oFile:
                oMatch = re.search(r'(?:DISTRIB_DESCRIPTION\s*=)\s*"*(.*)"', sLine);
                if oMatch is not None:
                    sDist = oMatch.group(1).strip();
        except:
            pass;
        if sDist:
            sVersion += ' / ' + sDist;
        else:
            asFiles = \
            [
                [ '/etc/debian_version', 'Debian v'],
                [ '/etc/gentoo-release', '' ],
                [ '/etc/oracle-release', '' ],
                [ '/etc/redhat-release', '' ],
                [ '/etc/SuSE-release', '' ],
            ];
            for sFile, sPrefix in asFiles:
                if os.path.isfile(sFile):
                    try:
                        oFile = open(sFile);
                        sLine = oFile.readline();
                        oFile.close();
                    except:
                        continue;
                    sLine = sLine.strip()
                    if sLine:
                        sVersion += ' / ' + sPrefix + sLine;
                    break;

    elif sOs == 'solaris':
        sVersion = platform.version();
        if os.path.isfile('/etc/release'):
            try:
                oFile = open('/etc/release');
                sLast = oFile.readlines()[-1];
                oFile.close();
                sLast = sLast.strip();
                if sLast:
                    sVersion += ' (' + sLast + ')';
            except:
                pass;

    elif sOs == 'darwin':
        sOsxVersion = platform.mac_ver()[0];
        codenames = {"4": "Tiger",
                     "5": "Leopard",
                     "6": "Snow Leopard",
                     "7": "Lion",
                     "8": "Mountain Lion",
                     "9": "Mavericks",
                     "10": "Yosemite",
                     "11": "El Capitan",
                     "12": "Sierra",
                     "13": "High Sierra",
                     "14": "Unknown 14", }
        sVersion += ' / OS X ' + sOsxVersion + ' (' + codenames[sOsxVersion.split('.')[1]] + ')'

    elif sOs == 'win':
        class OSVersionInfoEx(ctypes.Structure):
            """ OSVERSIONEX """
            kaFields = [
                    ('dwOSVersionInfoSize', ctypes.c_ulong),
                    ('dwMajorVersion',      ctypes.c_ulong),
                    ('dwMinorVersion',      ctypes.c_ulong),
                    ('dwBuildNumber',       ctypes.c_ulong),
                    ('dwPlatformId',        ctypes.c_ulong),
                    ('szCSDVersion',        ctypes.c_wchar*128),
                    ('wServicePackMajor',   ctypes.c_ushort),
                    ('wServicePackMinor',   ctypes.c_ushort),
                    ('wSuiteMask',          ctypes.c_ushort),
                    ('wProductType',        ctypes.c_byte),
                    ('wReserved',           ctypes.c_byte)]
            _fields_ = kaFields # pylint: disable=invalid-name

            def __init__(self):
                super(OSVersionInfoEx, self).__init__()
                self.dwOSVersionInfoSize = ctypes.sizeof(self)

        oOsVersion = OSVersionInfoEx()
        rc = ctypes.windll.Ntdll.RtlGetVersion(ctypes.byref(oOsVersion))
        if rc == 0:
            # Python platform.release() is not reliable for newer server releases
            if oOsVersion.wProductType != 1:
                if oOsVersion.dwMajorVersion == 10 and oOsVersion.dwMinorVersion == 0:
                    sVersion = '2016Server';
                elif oOsVersion.dwMajorVersion == 6 and oOsVersion.dwMinorVersion == 3:
                    sVersion = '2012ServerR2';
                elif oOsVersion.dwMajorVersion == 6 and oOsVersion.dwMinorVersion == 2:
                    sVersion = '2012Server';
                elif oOsVersion.dwMajorVersion == 6 and oOsVersion.dwMinorVersion == 1:
                    sVersion = '2008ServerR2';
                elif oOsVersion.dwMajorVersion == 6 and oOsVersion.dwMinorVersion == 0:
                    sVersion = '2008Server';
                elif oOsVersion.dwMajorVersion == 5 and oOsVersion.dwMinorVersion == 2:
                    sVersion = '2003Server';
            sVersion += ' build ' + str(oOsVersion.dwBuildNumber)
            if oOsVersion.wServicePackMajor:
                sVersion += ' SP' + str(oOsVersion.wServicePackMajor)
                if oOsVersion.wServicePackMinor:
                    sVersion += '.' + str(oOsVersion.wServicePackMinor)

    return sVersion;

#
# File system.
#

def openNoInherit(sFile, sMode = 'r'):
    """
    Wrapper around open() that tries it's best to make sure the file isn't
    inherited by child processes.

    This is a best effort thing at the moment as it doesn't synchronizes with
    child process spawning in any way.  Thus it can be subject to races in
    multithreaded programs.
    """

    try:
        from fcntl import FD_CLOEXEC, F_GETFD, F_SETFD, fcntl; # pylint: disable=F0401
    except:
        # On windows, use the 'N' flag introduces in Visual C++ 7.0 or 7.1.
        if getHostOs() == 'win':
            offComma = sMode.find(',');
            if offComma < 0:
                return open(sFile, sMode + 'N');
            return open(sFile, sMode[:offComma] + 'N' + sMode[offComma:]);

        # Just in case.
        return open(sFile, sMode);

    oFile = open(sFile, sMode)
    #try:
    fcntl(oFile, F_SETFD, fcntl(oFile, F_GETFD) | FD_CLOEXEC);
    #except:
    #    pass;
    return oFile;

def openNoDenyDeleteNoInherit(sFile, sMode = 'r'):
    """
    Wrapper around open() that tries it's best to make sure the file isn't
    inherited by child processes.

    This is a best effort thing at the moment as it doesn't synchronizes with
    child process spawning in any way.  Thus it can be subject to races in
    multithreaded programs.
    """

    try:
        from fcntl import FD_CLOEXEC, F_GETFD, F_SETFD, fcntl; # pylint: disable=F0401
    except:
        if getHostOs() == 'win':
            # Need to use CreateFile directly to open the file so we can feed it FILE_SHARE_DELETE.
            fAccess = 0;
            fDisposition = win32file.OPEN_EXISTING;                                                 # pylint: disable=no-member
            if 'r' in sMode or '+' in sMode:
                fAccess |= win32file.GENERIC_READ;                                                  # pylint: disable=no-member
            if 'a' in sMode:
                fAccess |= win32file.GENERIC_WRITE;                                                 # pylint: disable=no-member
                fDisposition = win32file.OPEN_ALWAYS;                                               # pylint: disable=no-member
            elif 'w' in sMode:
                fAccess = win32file.GENERIC_WRITE;                                                  # pylint: disable=no-member
                if '+' in sMode:
                    fDisposition = win32file.OPEN_ALWAYS;                                           # pylint: disable=no-member
                    fAccess |= win32file.GENERIC_READ;                                              # pylint: disable=no-member
                else:
                    fDisposition = win32file.CREATE_ALWAYS;                                         # pylint: disable=no-member
            if not fAccess:
                fAccess |= win32file.GENERIC_READ;                                                  # pylint: disable=no-member
            fSharing = (win32file.FILE_SHARE_READ | win32file.FILE_SHARE_WRITE                      # pylint: disable=no-member
                        | win32file.FILE_SHARE_DELETE);                                             # pylint: disable=no-member
            hFile = win32file.CreateFile(sFile, fAccess, fSharing, None, fDisposition, 0, None);    # pylint: disable=no-member
            if 'a' in sMode:
                win32file.SetFilePointer(hFile, 0, win32file.FILE_END);                             # pylint: disable=no-member

            # Turn the NT handle into a CRT file descriptor.
            hDetachedFile = hFile.Detach();
            if fAccess == win32file.GENERIC_READ:                                                   # pylint: disable=no-member
                fOpen = os.O_RDONLY;
            elif fAccess == win32file.GENERIC_WRITE:                                                # pylint: disable=no-member
                fOpen = os.O_WRONLY;
            else:
                fOpen = os.O_RDWR;
            if 'a' in sMode:
                fOpen |= os.O_APPEND;
            if 'b' in sMode or 't' in sMode:
                fOpen |= os.O_TEXT;                                                                 # pylint: disable=no-member
            fdFile = msvcrt.open_osfhandle(hDetachedFile, fOpen);

            # Tell python to use this handle.
            return os.fdopen(fdFile, sMode);

        # Just in case.
        return open(sFile, sMode);

    oFile = open(sFile, sMode)
    #try:
    fcntl(oFile, F_SETFD, fcntl(oFile, F_GETFD) | FD_CLOEXEC);
    #except:
    #    pass;
    return oFile;

def noxcptReadLink(sPath, sXcptRet):
    """
    No exceptions os.readlink wrapper.
    """
    try:
        sRet = os.readlink(sPath); # pylint: disable=E1101
    except:
        sRet = sXcptRet;
    return sRet;

def readFile(sFile, sMode = 'rb'):
    """
    Reads the entire file.
    """
    oFile = open(sFile, sMode);
    sRet = oFile.read();
    oFile.close();
    return sRet;

def noxcptReadFile(sFile, sXcptRet, sMode = 'rb'):
    """
    No exceptions common.readFile wrapper.
    """
    try:
        sRet = readFile(sFile, sMode);
    except:
        sRet = sXcptRet;
    return sRet;

def noxcptRmDir(sDir, oXcptRet = False):
    """
    No exceptions os.rmdir wrapper.
    """
    oRet = True;
    try:
        os.rmdir(sDir);
    except:
        oRet = oXcptRet;
    return oRet;

def noxcptDeleteFile(sFile, oXcptRet = False):
    """
    No exceptions os.remove wrapper.
    """
    oRet = True;
    try:
        os.remove(sFile);
    except:
        oRet = oXcptRet;
    return oRet;


def dirEnumerateTree(sDir, fnCallback, fIgnoreExceptions = True):
    # type: (string, (string, stat) -> bool) -> bool
    """
    Recursively walks a directory tree, calling fnCallback for each.

    fnCallback takes a full path and stat object (can be None).  It
    returns a boolean value, False stops walking and returns immediately.

    Returns True or False depending on fnCallback.
    Returns None fIgnoreExceptions is True and an exception was raised by listdir.
    """
    def __worker(sCurDir):
        """ Worker for """
        try:
            asNames = os.listdir(sCurDir);
        except:
            if not fIgnoreExceptions:
                raise;
            return None;
        rc = True;
        for sName in asNames:
            if sName not in [ '.', '..' ]:
                sFullName = os.path.join(sCurDir, sName);
                try:    oStat = os.lstat(sFullName);
                except: oStat = None;
                if fnCallback(sFullName, oStat) is False:
                    return False;
                if oStat is not None and stat.S_ISDIR(oStat.st_mode):
                    rc =  __worker(sFullName);
                    if rc is False:
                        break;
        return rc;

    # Ensure unicode path here so listdir also returns unicode on windows.
    ## @todo figure out unicode stuff on non-windows.
    if sys.platform == 'win32':
        sDir = unicode(sDir);
    return __worker(sDir);



def formatFileMode(uMode):
    # type: (int) -> string
    """
    Format a st_mode value 'ls -la' fasion.
    Returns string.
    """
    if   stat.S_ISDIR(uMode):   sMode = 'd';
    elif stat.S_ISREG(uMode):   sMode = '-';
    elif stat.S_ISLNK(uMode):   sMode = 'l';
    elif stat.S_ISFIFO(uMode):  sMode = 'p';
    elif stat.S_ISCHR(uMode):   sMode = 'c';
    elif stat.S_ISBLK(uMode):   sMode = 'b';
    elif stat.S_ISSOCK(uMode):  sMode = 's';
    else:                       sMode = '?';
    ## @todo sticky bits.
    sMode += 'r' if uMode & stat.S_IRUSR else '-';
    sMode += 'w' if uMode & stat.S_IWUSR else '-';
    sMode += 'x' if uMode & stat.S_IXUSR else '-';
    sMode += 'r' if uMode & stat.S_IRGRP else '-';
    sMode += 'w' if uMode & stat.S_IWGRP else '-';
    sMode += 'x' if uMode & stat.S_IXGRP else '-';
    sMode += 'r' if uMode & stat.S_IROTH else '-';
    sMode += 'w' if uMode & stat.S_IWOTH else '-';
    sMode += 'x' if uMode & stat.S_IXOTH else '-';
    sMode += ' ';
    return sMode;


def formatFileStat(oStat):
    # type: (stat) -> string
    """
    Format a stat result 'ls -la' fasion (numeric IDs).
    Returns string.
    """
    return '%s %3s %4s %4s %10s %s' \
          % (formatFileMode(oStat.st_mode), oStat.st_nlink, oStat.st_uid, oStat.st_gid, oStat.st_size,
             time.strftime('%Y-%m-%d %H:%M', time.localtime(oStat.st_mtime)), );

## Good buffer for file operations.
g_cbGoodBufferSize = 256*1024;

## The original shutil.copyfileobj.
g_fnOriginalShCopyFileObj = None;

def __myshutilcopyfileobj(fsrc, fdst, length = g_cbGoodBufferSize):
    """ shutil.copyfileobj with different length default value (16384 is slow with python 2.7 on windows). """
    return g_fnOriginalShCopyFileObj(fsrc, fdst, length);

def __installShUtilHacks(shutil):
    """ Installs the shutil buffer size hacks. """
    global g_fnOriginalShCopyFileObj;
    if g_fnOriginalShCopyFileObj is None:
        g_fnOriginalShCopyFileObj = shutil.copyfileobj;
        shutil.copyfileobj = __myshutilcopyfileobj;
    return True;


def copyFileSimple(sFileSrc, sFileDst):
    """
    Wrapper around shutil.copyfile that simply copies the data of a regular file.
    Raises exception on failure.
    Return True for show.
    """
    import shutil;
    __installShUtilHacks(shutil);
    return shutil.copyfile(sFileSrc, sFileDst);

#
# SubProcess.
#

def _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs):
    """
    If the "executable" is a python script, insert the python interpreter at
    the head of the argument list so that it will work on systems which doesn't
    support hash-bang scripts.
    """

    asArgs = dKeywordArgs.get('args');
    if asArgs is None:
        asArgs = aPositionalArgs[0];

    if asArgs[0].endswith('.py'):
        if sys.executable:
            asArgs.insert(0, sys.executable);
        else:
            asArgs.insert(0, 'python');

        # paranoia...
        if dKeywordArgs.get('args') is not None:
            dKeywordArgs['args'] = asArgs;
        else:
            aPositionalArgs = (asArgs,) + aPositionalArgs[1:];
    return None;

def processPopenSafe(*aPositionalArgs, **dKeywordArgs):
    """
    Wrapper for subprocess.Popen that's Ctrl-C safe on windows.
    """
    if getHostOs() == 'win':
        if dKeywordArgs.get('creationflags', 0) == 0:
            dKeywordArgs['creationflags'] = subprocess.CREATE_NEW_PROCESS_GROUP;
    return subprocess.Popen(*aPositionalArgs, **dKeywordArgs);

def processCall(*aPositionalArgs, **dKeywordArgs):
    """
    Wrapper around subprocess.call to deal with its absence in older
    python versions.
    Returns process exit code (see subprocess.poll).
    """
    assert dKeywordArgs.get('stdout') is None;
    assert dKeywordArgs.get('stderr') is None;
    _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs);
    oProcess = processPopenSafe(*aPositionalArgs, **dKeywordArgs);
    return oProcess.wait();

def processOutputChecked(*aPositionalArgs, **dKeywordArgs):
    """
    Wrapper around subprocess.check_output to deal with its absense in older
    python versions.
    """
    _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs);
    oProcess = processPopenSafe(stdout=subprocess.PIPE, *aPositionalArgs, **dKeywordArgs);

    sOutput, _ = oProcess.communicate();
    iExitCode  = oProcess.poll();

    if iExitCode is not 0:
        asArgs = dKeywordArgs.get('args');
        if asArgs is None:
            asArgs = aPositionalArgs[0];
        print(sOutput);
        raise subprocess.CalledProcessError(iExitCode, asArgs);

    return str(sOutput); # str() make pylint happy.

g_fOldSudo = None;
def _sudoFixArguments(aPositionalArgs, dKeywordArgs, fInitialEnv = True):
    """
    Adds 'sudo' (or similar) to the args parameter, whereever it is.
    """

    # Are we root?
    fIsRoot = True;
    try:
        fIsRoot = os.getuid() == 0; # pylint: disable=E1101
    except:
        pass;

    # If not, prepend sudo (non-interactive, simulate initial login).
    if fIsRoot is not True:
        asArgs = dKeywordArgs.get('args');
        if asArgs is None:
            asArgs = aPositionalArgs[0];

        # Detect old sudo.
        global g_fOldSudo;
        if g_fOldSudo is None:
            try:
                sVersion = processOutputChecked(['sudo', '-V']);
            except:
                sVersion = '1.7.0';
            sVersion = sVersion.strip().split('\n')[0];
            sVersion = sVersion.replace('Sudo version', '').strip();
            g_fOldSudo = len(sVersion) >= 4 \
                     and sVersion[0] == '1' \
                     and sVersion[1] == '.' \
                     and sVersion[2] <= '6' \
                     and sVersion[3] == '.';

        asArgs.insert(0, 'sudo');
        if not g_fOldSudo:
            asArgs.insert(1, '-n');
        if fInitialEnv and not g_fOldSudo:
            asArgs.insert(1, '-i');

        # paranoia...
        if dKeywordArgs.get('args') is not None:
            dKeywordArgs['args'] = asArgs;
        else:
            aPositionalArgs = (asArgs,) + aPositionalArgs[1:];
    return None;


def sudoProcessCall(*aPositionalArgs, **dKeywordArgs):
    """
    sudo (or similar) + subprocess.call
    """
    _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs);
    _sudoFixArguments(aPositionalArgs, dKeywordArgs);
    return processCall(*aPositionalArgs, **dKeywordArgs);

def sudoProcessOutputChecked(*aPositionalArgs, **dKeywordArgs):
    """
    sudo (or similar) + subprocess.check_output.
    """
    _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs);
    _sudoFixArguments(aPositionalArgs, dKeywordArgs);
    return processOutputChecked(*aPositionalArgs, **dKeywordArgs);

def sudoProcessOutputCheckedNoI(*aPositionalArgs, **dKeywordArgs):
    """
    sudo (or similar) + subprocess.check_output, except '-i' isn't used.
    """
    _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs);
    _sudoFixArguments(aPositionalArgs, dKeywordArgs, False);
    return processOutputChecked(*aPositionalArgs, **dKeywordArgs);

def sudoProcessPopen(*aPositionalArgs, **dKeywordArgs):
    """
    sudo (or similar) + processPopenSafe.
    """
    _processFixPythonInterpreter(aPositionalArgs, dKeywordArgs);
    _sudoFixArguments(aPositionalArgs, dKeywordArgs);
    return processPopenSafe(*aPositionalArgs, **dKeywordArgs);


#
# Generic process stuff.
#

def processInterrupt(uPid):
    """
    Sends a SIGINT or equivalent to interrupt the specified process.
    Returns True on success, False on failure.

    On Windows hosts this may not work unless the process happens to be a
    process group leader.
    """
    if sys.platform == 'win32':
        try:
            win32console.GenerateConsoleCtrlEvent(win32con.CTRL_BREAK_EVENT, uPid);             # pylint: disable=no-member
            fRc = True;
        except:
            fRc = False;
    else:
        try:
            os.kill(uPid, signal.SIGINT);
            fRc = True;
        except:
            fRc = False;
    return fRc;

def sendUserSignal1(uPid):
    """
    Sends a SIGUSR1 or equivalent to nudge the process into shutting down
    (VBoxSVC) or something.
    Returns True on success, False on failure or if not supported (win).

    On Windows hosts this may not work unless the process happens to be a
    process group leader.
    """
    if sys.platform == 'win32':
        fRc = False;
    else:
        try:
            os.kill(uPid, signal.SIGUSR1); # pylint: disable=E1101
            fRc = True;
        except:
            fRc = False;
    return fRc;

def processTerminate(uPid):
    """
    Terminates the process in a nice manner (SIGTERM or equivalent).
    Returns True on success, False on failure.
    """
    fRc = False;
    if sys.platform == 'win32':
        try:
            hProcess = win32api.OpenProcess(win32con.PROCESS_TERMINATE, False, uPid);           # pylint: disable=no-member
        except:
            pass;
        else:
            try:
                win32process.TerminateProcess(hProcess, 0x40010004); # DBG_TERMINATE_PROCESS    # pylint: disable=no-member
                fRc = True;
            except:
                pass;
            hProcess.Close(); #win32api.CloseHandle(hProcess)
    else:
        try:
            os.kill(uPid, signal.SIGTERM);
            fRc = True;
        except:
            pass;
    return fRc;

def processKill(uPid):
    """
    Terminates the process with extreme prejudice (SIGKILL).
    Returns True on success, False on failure.
    """
    if sys.platform == 'win32':
        fRc = processTerminate(uPid);
    else:
        try:
            os.kill(uPid, signal.SIGKILL); # pylint: disable=E1101
            fRc = True;
        except:
            fRc = False;
    return fRc;

def processKillWithNameCheck(uPid, sName):
    """
    Like processKill(), but checks if the process name matches before killing
    it.  This is intended for killing using potentially stale pid values.

    Returns True on success, False on failure.
    """

    if processCheckPidAndName(uPid, sName) is not True:
        return False;
    return processKill(uPid);


def processExists(uPid):
    """
    Checks if the specified process exits.
    This will only work if we can signal/open the process.

    Returns True if it positively exists, False otherwise.
    """
    if sys.platform == 'win32':
        fRc = False;
        # We try open the process for waiting since this is generally only forbidden in a very few cases.
        try:
            hProcess = win32api.OpenProcess(win32con.SYNCHRONIZE, False, uPid);     # pylint: disable=no-member
        except pywintypes.error as oXcpt:                                           # pylint: disable=no-member
            if oXcpt.winerror == winerror.ERROR_ACCESS_DENIED:
                fRc = True;
        except Exception as oXcpt:
            pass;
        else:
            hProcess.Close();
            fRc = True;
    else:
        try:
            os.kill(uPid, 0);
            fRc = True;
        except: ## @todo check error code.
            fRc = False;
    return fRc;

def processCheckPidAndName(uPid, sName):
    """
    Checks if a process PID and NAME matches.
    """
    fRc = processExists(uPid);
    if fRc is not True:
        return False;

    if sys.platform == 'win32':
        try:
            from win32com.client import GetObject; # pylint: disable=F0401
            oWmi = GetObject('winmgmts:');
            aoProcesses = oWmi.InstancesOf('Win32_Process');
            for oProcess in aoProcesses:
                if long(oProcess.Properties_("ProcessId").Value) == uPid:
                    sCurName = oProcess.Properties_("Name").Value;
                    #reporter.log2('uPid=%s sName=%s sCurName=%s' % (uPid, sName, sCurName));
                    sName    = sName.lower();
                    sCurName = sCurName.lower();
                    if os.path.basename(sName) == sName:
                        sCurName = os.path.basename(sCurName);

                    if   sCurName == sName \
                      or sCurName + '.exe' == sName \
                      or sCurName == sName  + '.exe':
                        fRc = True;
                    break;
        except:
            #reporter.logXcpt('uPid=%s sName=%s' % (uPid, sName));
            pass;
    else:
        if sys.platform in ('linux2', ):
            asPsCmd = ['/bin/ps',     '-p', '%u' % (uPid,), '-o', 'fname='];
        elif sys.platform in ('sunos5',):
            asPsCmd = ['/usr/bin/ps', '-p', '%u' % (uPid,), '-o', 'fname='];
        elif sys.platform in ('darwin',):
            asPsCmd = ['/bin/ps',     '-p', '%u' % (uPid,), '-o', 'ucomm='];
        else:
            asPsCmd = None;

        if asPsCmd is not None:
            try:
                oPs = subprocess.Popen(asPsCmd, stdout=subprocess.PIPE);
                sCurName = oPs.communicate()[0];
                iExitCode = oPs.wait();
            except:
                #reporter.logXcpt();
                return False;

            # ps fails with non-zero exit code if the pid wasn't found.
            if iExitCode is not 0:
                return False;
            if sCurName is None:
                return False;
            sCurName = sCurName.strip();
            if not sCurName:
                return False;

            if os.path.basename(sName) == sName:
                sCurName = os.path.basename(sCurName);
            elif os.path.basename(sCurName) == sCurName:
                sName = os.path.basename(sName);

            if sCurName != sName:
                return False;

            fRc = True;
    return fRc;

def processGetInfo(uPid, fSudo = False):
    """
    Tries to acquire state information of the given process.

    Returns a string with the information on success or None on failure or
    if the host is not supported.

    Note that the format of the information is host system dependent and will
    likely differ much between different hosts.
    """
    fRc = processExists(uPid);
    if fRc is not True:
        return None;

    sHostOs = getHostOs();
    if sHostOs in [ 'linux',]:
        sGdb = '/usr/bin/gdb';
        if not os.path.isfile(sGdb): sGdb = '/usr/local/bin/gdb';
        if not os.path.isfile(sGdb): sGdb = 'gdb';
        aasCmd = [
            [ sGdb, '-batch',
              '-ex', 'set pagination off',
              '-ex', 'thread apply all bt',
              '-ex', 'info proc mapping',
              '-ex', 'info sharedlibrary',
              '-p', '%u' % (uPid,), ],
        ];
    elif sHostOs == 'darwin':
        # LLDB doesn't work in batch mode when attaching to a process, at least
        # with macOS Sierra (10.12). GDB might not be installed. Use the sample
        # tool instead with a 1 second duration and 1000ms sampling interval to
        # get one stack trace.  For the process mappings use vmmap.
        aasCmd = [
            [ '/usr/bin/sample', '-mayDie', '%u' % (uPid,), '1', '1000', ],
            [ '/usr/bin/vmmap', '%u' % (uPid,), ],
        ];
    elif sHostOs == 'solaris':
        aasCmd = [
            [ '/usr/bin/pstack', '%u' % (uPid,), ],
            [ '/usr/bin/pmap', '%u' % (uPid,), ],
        ];
    else:
        aasCmd = [];

    sInfo = '';
    for asCmd in aasCmd:
        try:
            if fSudo:
                sThisInfo = sudoProcessOutputChecked(asCmd);
            else:
                sThisInfo = processOutputChecked(asCmd);
            if sThisInfo is not None:
                sInfo += sThisInfo;
        except:
            pass;
    if not sInfo:
        sInfo = None;

    return sInfo;


class ProcessInfo(object):
    """Process info."""
    def __init__(self, iPid):
        self.iPid       = iPid;
        self.iParentPid = None;
        self.sImage     = None;
        self.sName      = None;
        self.asArgs     = None;
        self.sCwd       = None;
        self.iGid       = None;
        self.iUid       = None;
        self.iProcGroup = None;
        self.iSessionId = None;

    def loadAll(self):
        """Load all the info."""
        sOs = getHostOs();
        if sOs == 'linux':
            sProc = '/proc/%s/' % (self.iPid,);
            if self.sImage   is None: self.sImage = noxcptReadLink(sProc + 'exe', None);
            if self.sCwd     is None: self.sCwd   = noxcptReadLink(sProc + 'cwd', None);
            if self.asArgs   is None: self.asArgs = noxcptReadFile(sProc + 'cmdline', '').split('\x00');
        #elif sOs == 'solaris': - doesn't work for root processes, suid proces, and other stuff.
        #    sProc = '/proc/%s/' % (self.iPid,);
        #    if self.sImage   is None: self.sImage = noxcptReadLink(sProc + 'path/a.out', None);
        #    if self.sCwd     is None: self.sCwd   = noxcptReadLink(sProc + 'path/cwd', None);
        else:
            pass;
        if self.sName is None and self.sImage is not None:
            self.sName = self.sImage;

    def windowsGrabProcessInfo(self, oProcess):
        """Windows specific loadAll."""
        try:    self.sName      = oProcess.Properties_("Name").Value;
        except: pass;
        try:    self.sImage     = oProcess.Properties_("ExecutablePath").Value;
        except: pass;
        try:    self.asArgs     = [oProcess.Properties_("CommandLine").Value]; ## @todo split it.
        except: pass;
        try:    self.iParentPid = oProcess.Properties_("ParentProcessId").Value;
        except: pass;
        try:    self.iSessionId = oProcess.Properties_("SessionId").Value;
        except: pass;
        if self.sName is None and self.sImage is not None:
            self.sName = self.sImage;

    def getBaseImageName(self):
        """
        Gets the base image name if available, use the process name if not available.
        Returns image/process base name or None.
        """
        sRet = self.sImage if self.sName is None else self.sName;
        if sRet is None:
            self.loadAll();
            sRet = self.sImage if self.sName is None else self.sName;
            if sRet is None:
                if not self.asArgs:
                    return None;
                sRet = self.asArgs[0];
                if not sRet:
                    return None;
        return os.path.basename(sRet);

    def getBaseImageNameNoExeSuff(self):
        """
        Same as getBaseImageName, except any '.exe' or similar suffix is stripped.
        """
        sRet = self.getBaseImageName();
        if sRet is not None and len(sRet) > 4 and sRet[-4] == '.':
            if (sRet[-4:]).lower() in [ '.exe', '.com', '.msc', '.vbs', '.cmd', '.bat' ]:
                sRet = sRet[:-4];
        return sRet;


def processListAll(): # pylint: disable=R0914
    """
    Return a list of ProcessInfo objects for all the processes in the system
    that the current user can see.
    """
    asProcesses = [];

    sOs = getHostOs();
    if sOs == 'win':
        from win32com.client import GetObject; # pylint: disable=F0401
        oWmi = GetObject('winmgmts:');
        aoProcesses = oWmi.InstancesOf('Win32_Process');
        for oProcess in aoProcesses:
            try:
                iPid = int(oProcess.Properties_("ProcessId").Value);
            except:
                continue;
            oMyInfo = ProcessInfo(iPid);
            oMyInfo.windowsGrabProcessInfo(oProcess);
            asProcesses.append(oMyInfo);
        return asProcesses;

    if sOs in [ 'linux', ]:  # Not solaris, ps gets more info than /proc/.
        try:
            asDirs = os.listdir('/proc');
        except:
            asDirs = [];
        for sDir in asDirs:
            if sDir.isdigit():
                asProcesses.append(ProcessInfo(int(sDir),));
        return asProcesses;

    #
    # The other OSes parses the output from the 'ps' utility.
    #
    asPsCmd = [
        '/bin/ps',          # 0
        '-A',               # 1
        '-o', 'pid=',       # 2,3
        '-o', 'ppid=',      # 4,5
        '-o', 'pgid=',      # 6,7
        '-o', 'sid=',       # 8,9
        '-o', 'uid=',       # 10,11
        '-o', 'gid=',       # 12,13
        '-o', 'comm='       # 14,15
    ];

    if sOs == 'darwin':
        assert asPsCmd[9] == 'sid=';
        asPsCmd[9] = 'sess=';
    elif sOs == 'solaris':
        asPsCmd[0] = '/usr/bin/ps';

    try:
        sRaw = processOutputChecked(asPsCmd);
    except:
        return asProcesses;

    for sLine in sRaw.split('\n'):
        sLine = sLine.lstrip();
        if len(sLine) < 7 or not sLine[0].isdigit():
            continue;

        iField   = 0;
        off      = 0;
        aoFields = [None, None, None, None, None, None, None];
        while iField < 7:
            # Eat whitespace.
            while off < len(sLine) and (sLine[off] == ' ' or sLine[off] == '\t'):
                off += 1;

            # Final field / EOL.
            if iField == 6:
                aoFields[6] = sLine[off:];
                break;
            if off >= len(sLine):
                break;

            # Generic field parsing.
            offStart = off;
            off += 1;
            while off < len(sLine) and sLine[off] != ' ' and sLine[off] != '\t':
                off += 1;
            try:
                if iField != 3:
                    aoFields[iField] = int(sLine[offStart:off]);
                else:
                    aoFields[iField] = long(sLine[offStart:off], 16); # sess is a hex address.
            except:
                pass;
            iField += 1;

        if aoFields[0] is not None:
            oMyInfo = ProcessInfo(aoFields[0]);
            oMyInfo.iParentPid = aoFields[1];
            oMyInfo.iProcGroup = aoFields[2];
            oMyInfo.iSessionId = aoFields[3];
            oMyInfo.iUid       = aoFields[4];
            oMyInfo.iGid       = aoFields[5];
            oMyInfo.sName      = aoFields[6];
            asProcesses.append(oMyInfo);

    return asProcesses;


def processCollectCrashInfo(uPid, fnLog, fnCrashFile):
    """
    Looks for information regarding the demise of the given process.
    """
    sOs = getHostOs();
    if sOs == 'darwin':
        #
        # On darwin we look for crash and diagnostic reports.
        #
        asLogDirs = [
            u'/Library/Logs/DiagnosticReports/',
            u'/Library/Logs/CrashReporter/',
            u'~/Library/Logs/DiagnosticReports/',
            u'~/Library/Logs/CrashReporter/',
        ];
        for sDir in asLogDirs:
            sDir = os.path.expanduser(sDir);
            if not os.path.isdir(sDir):
                continue;
            try:
                asDirEntries = os.listdir(sDir);
            except:
                continue;
            for sEntry in asDirEntries:
                # Only interested in .crash files.
                _, sSuff = os.path.splitext(sEntry);
                if sSuff != '.crash':
                    continue;

                # The pid can be found at the end of the first line.
                sFull = os.path.join(sDir, sEntry);
                try:
                    oFile = open(sFull, 'r');
                    sFirstLine = oFile.readline();
                    oFile.close();
                except:
                    continue;
                if len(sFirstLine) <= 4 or sFirstLine[-2] != ']':
                    continue;
                offPid = len(sFirstLine) - 3;
                while offPid > 1 and sFirstLine[offPid - 1].isdigit():
                    offPid -= 1;
                try:    uReportPid = int(sFirstLine[offPid:-2]);
                except: continue;

                # Does the pid we found match?
                if uReportPid == uPid:
                    fnLog('Found crash report for %u: %s' % (uPid, sFull,));
                    fnCrashFile(sFull, False);
    elif sOs == 'win':
        #
        # Getting WER reports would be great, however we have trouble match the
        # PID to those as they seems not to mention it in the brief reports.
        # Instead we'll just look for crash dumps in C:\CrashDumps (our custom
        # location - see the windows readme for the testbox script) and what
        # the MSDN article lists for now.
        #
        # It's been observed on Windows server 2012 that the dump files takes
        # the form: <processimage>.<decimal-pid>.dmp
        #
        asDmpDirs = [
            u'%SystemDrive%/CrashDumps/',                   # Testboxes.
            u'%LOCALAPPDATA%/CrashDumps/',                  # MSDN example.
            u'%WINDIR%/ServiceProfiles/LocalServices/',     # Local and network service.
            u'%WINDIR%/ServiceProfiles/NetworkSerices/',
            u'%WINDIR%/ServiceProfiles/',
            u'%WINDIR%/System32/Config/SystemProfile/',     # System services.
        ];
        sMatchSuffix = '.%u.dmp' % (uPid,);

        for sDir in asDmpDirs:
            sDir = os.path.expandvars(sDir);
            if not os.path.isdir(sDir):
                continue;
            try:
                asDirEntries = os.listdir(sDir);
            except:
                continue;
            for sEntry in asDirEntries:
                if sEntry.endswith(sMatchSuffix):
                    sFull = os.path.join(sDir, sEntry);
                    fnLog('Found crash dump for %u: %s' % (uPid, sFull,));
                    fnCrashFile(sFull, True);

    else:
        pass; ## TODO
    return None;


#
# Time.
#

#
# The following test case shows how time.time() only have ~ms resolution
# on Windows (tested W10) and why it therefore makes sense to try use
# performance counters.
#
# Note! We cannot use time.clock() as the timestamp must be portable across
#       processes.  See timeout testcase problem on win hosts (no logs).
#
#import sys;
#import time;
#from common import utils;
#
#atSeries = [];
#for i in xrange(1,160):
#    if i == 159: time.sleep(10);
#    atSeries.append((utils.timestampNano(), long(time.clock() * 1000000000), long(time.time() * 1000000000)));
#
#tPrev = atSeries[0]
#for tCur in atSeries:
#    print 't1=%+22u, %u' % (tCur[0], tCur[0] - tPrev[0]);
#    print 't2=%+22u, %u' % (tCur[1], tCur[1] - tPrev[1]);
#    print 't3=%+22u, %u' % (tCur[2], tCur[2] - tPrev[2]);
#    print '';
#    tPrev = tCur
#
#print 't1=%u' % (atSeries[-1][0] - atSeries[0][0]);
#print 't2=%u' % (atSeries[-1][1] - atSeries[0][1]);
#print 't3=%u' % (atSeries[-1][2] - atSeries[0][2]);

g_fWinUseWinPerfCounter           = sys.platform == 'win32';
g_fpWinPerfCounterFreq            = None;
g_oFuncwinQueryPerformanceCounter = None;

def _winInitPerfCounter():
    """ Initializes the use of performance counters. """
    global g_fWinUseWinPerfCounter, g_fpWinPerfCounterFreq, g_oFuncwinQueryPerformanceCounter

    uFrequency = ctypes.c_ulonglong(0);
    if ctypes.windll.kernel32.QueryPerformanceFrequency(ctypes.byref(uFrequency)):
        if uFrequency.value >= 1000:
            #print 'uFrequency = %s' % (uFrequency,);
            #print 'type(uFrequency) = %s' % (type(uFrequency),);
            g_fpWinPerfCounterFreq = float(uFrequency.value);

            # Check that querying the counter works too.
            global g_oFuncwinQueryPerformanceCounter
            g_oFuncwinQueryPerformanceCounter = ctypes.windll.kernel32.QueryPerformanceCounter;
            uCurValue = ctypes.c_ulonglong(0);
            if g_oFuncwinQueryPerformanceCounter(ctypes.byref(uCurValue)):
                if uCurValue.value > 0:
                    return True;
    g_fWinUseWinPerfCounter = False;
    return False;

def _winFloatTime():
    """ Gets floating point time on windows. """
    if g_fpWinPerfCounterFreq is not None or _winInitPerfCounter():
        uCurValue = ctypes.c_ulonglong(0);
        if g_oFuncwinQueryPerformanceCounter(ctypes.byref(uCurValue)):
            return float(uCurValue.value) / g_fpWinPerfCounterFreq;
    return time.time();


def timestampNano():
    """
    Gets a nanosecond timestamp.
    """
    if g_fWinUseWinPerfCounter is True:
        return long(_winFloatTime() * 1000000000);
    return long(time.time() * 1000000000);

def timestampMilli():
    """
    Gets a millisecond timestamp.
    """
    if g_fWinUseWinPerfCounter is True:
        return long(_winFloatTime() * 1000);
    return long(time.time() * 1000);

def timestampSecond():
    """
    Gets a second timestamp.
    """
    if g_fWinUseWinPerfCounter is True:
        return long(_winFloatTime());
    return long(time.time());

def getTimePrefix():
    """
    Returns a timestamp prefix, typically used for logging. UTC.
    """
    try:
        oNow = datetime.datetime.utcnow();
        sTs = '%02u:%02u:%02u.%06u' % (oNow.hour, oNow.minute, oNow.second, oNow.microsecond);
    except:
        sTs = 'getTimePrefix-exception';
    return sTs;

def getTimePrefixAndIsoTimestamp():
    """
    Returns current UTC as log prefix and iso timestamp.
    """
    try:
        oNow = datetime.datetime.utcnow();
        sTsPrf = '%02u:%02u:%02u.%06u' % (oNow.hour, oNow.minute, oNow.second, oNow.microsecond);
        sTsIso = formatIsoTimestamp(oNow);
    except:
        sTsPrf = sTsIso = 'getTimePrefix-exception';
    return (sTsPrf, sTsIso);

def formatIsoTimestamp(oNow):
    """Formats the datetime object as an ISO timestamp."""
    assert oNow.tzinfo is None;
    sTs = '%s.%09uZ' % (oNow.strftime('%Y-%m-%dT%H:%M:%S'), oNow.microsecond * 1000);
    return sTs;

def getIsoTimestamp():
    """Returns the current UTC timestamp as a string."""
    return formatIsoTimestamp(datetime.datetime.utcnow());


def getLocalHourOfWeek():
    """ Local hour of week (0 based). """
    oNow = datetime.datetime.now();
    return (oNow.isoweekday() - 1) * 24 + oNow.hour;


def formatIntervalSeconds(cSeconds):
    """ Format a seconds interval into a nice 01h 00m 22s string  """
    # Two simple special cases.
    if cSeconds < 60:
        return '%ss' % (cSeconds,);
    if cSeconds < 3600:
        cMins = cSeconds / 60;
        cSecs = cSeconds % 60;
        if cSecs == 0:
            return '%sm' % (cMins,);
        return '%sm %ss' % (cMins, cSecs,);

    # Generic and a bit slower.
    cDays     = cSeconds / 86400;
    cSeconds %= 86400;
    cHours    = cSeconds / 3600;
    cSeconds %= 3600;
    cMins     = cSeconds / 60;
    cSecs     = cSeconds % 60;
    sRet = '';
    if cDays > 0:
        sRet = '%sd ' % (cDays,);
    if cHours > 0:
        sRet += '%sh ' % (cHours,);
    if cMins > 0:
        sRet += '%sm ' % (cMins,);
    if cSecs > 0:
        sRet += '%ss ' % (cSecs,);
    assert sRet; assert sRet[-1] == ' ';
    return sRet[:-1];

def formatIntervalSeconds2(oSeconds):
    """
    Flexible input version of formatIntervalSeconds for use in WUI forms where
    data is usually already string form.
    """
    if isinstance(oSeconds, (int, long)):
        return formatIntervalSeconds(oSeconds);
    if not isString(oSeconds):
        try:
            lSeconds = long(oSeconds);
        except:
            pass;
        else:
            if lSeconds >= 0:
                return formatIntervalSeconds2(lSeconds);
    return oSeconds;

def parseIntervalSeconds(sString):
    """
    Reverse of formatIntervalSeconds.

    Returns (cSeconds, sError), where sError is None on success.
    """

    # We might given non-strings, just return them without any fuss.
    if not isString(sString):
        if isinstance(sString, (int, long)) or sString is None:
            return (sString, None);
        ## @todo time/date objects?
        return (int(sString), None);

    # Strip it and make sure it's not empty.
    sString = sString.strip();
    if not sString:
        return (0, 'Empty interval string.');

    #
    # Split up the input into a list of 'valueN, unitN, ...'.
    #
    # Don't want to spend too much time trying to make re.split do exactly what
    # I need here, so please forgive the extra pass I'm making here.
    #
    asRawParts = re.split(r'\s*([0-9]+)\s*([^0-9,;]*)[\s,;]*', sString);
    asParts    = [];
    for sPart in asRawParts:
        sPart = sPart.strip();
        if sPart:
            asParts.append(sPart);
    if not asParts:
        return (0, 'Empty interval string or something?');

    #
    # Process them one or two at the time.
    #
    cSeconds   = 0;
    asErrors   = [];
    i          = 0;
    while i < len(asParts):
        sNumber = asParts[i];
        i += 1;
        if sNumber.isdigit():
            iNumber = int(sNumber);

            sUnit = 's';
            if i < len(asParts) and not asParts[i].isdigit():
                sUnit = asParts[i];
                i += 1;

            sUnitLower = sUnit.lower();
            if sUnitLower in [ 's', 'se', 'sec', 'second', 'seconds' ]:
                pass;
            elif sUnitLower in [ 'm', 'mi', 'min', 'minute', 'minutes' ]:
                iNumber *= 60;
            elif sUnitLower in [ 'h', 'ho', 'hou', 'hour', 'hours' ]:
                iNumber *= 3600;
            elif sUnitLower in [ 'd', 'da', 'day', 'days' ]:
                iNumber *= 86400;
            elif sUnitLower in [ 'w', 'week', 'weeks' ]:
                iNumber *= 7 * 86400;
            else:
                asErrors.append('Unknown unit "%s".' % (sUnit,));
            cSeconds += iNumber;
        else:
            asErrors.append('Bad number "%s".' % (sNumber,));
    return (cSeconds, None if not asErrors else ' '.join(asErrors));

def formatIntervalHours(cHours):
    """ Format a hours interval into a nice 1w 2d 1h string. """
    # Simple special cases.
    if cHours < 24:
        return '%sh' % (cHours,);

    # Generic and a bit slower.
    cWeeks    = cHours / (7 * 24);
    cHours   %= 7 * 24;
    cDays     = cHours / 24;
    cHours   %= 24;
    sRet = '';
    if cWeeks > 0:
        sRet = '%sw ' % (cWeeks,);
    if cDays > 0:
        sRet = '%sd ' % (cDays,);
    if cHours > 0:
        sRet += '%sh ' % (cHours,);
    assert sRet; assert sRet[-1] == ' ';
    return sRet[:-1];

def parseIntervalHours(sString):
    """
    Reverse of formatIntervalHours.

    Returns (cHours, sError), where sError is None on success.
    """

    # We might given non-strings, just return them without any fuss.
    if not isString(sString):
        if isinstance(sString, (int, long)) or sString is None:
            return (sString, None);
        ## @todo time/date objects?
        return (int(sString), None);

    # Strip it and make sure it's not empty.
    sString = sString.strip();
    if not sString:
        return (0, 'Empty interval string.');

    #
    # Split up the input into a list of 'valueN, unitN, ...'.
    #
    # Don't want to spend too much time trying to make re.split do exactly what
    # I need here, so please forgive the extra pass I'm making here.
    #
    asRawParts = re.split(r'\s*([0-9]+)\s*([^0-9,;]*)[\s,;]*', sString);
    asParts    = [];
    for sPart in asRawParts:
        sPart = sPart.strip();
        if sPart:
            asParts.append(sPart);
    if not asParts:
        return (0, 'Empty interval string or something?');

    #
    # Process them one or two at the time.
    #
    cHours     = 0;
    asErrors   = [];
    i          = 0;
    while i < len(asParts):
        sNumber = asParts[i];
        i += 1;
        if sNumber.isdigit():
            iNumber = int(sNumber);

            sUnit = 'h';
            if i < len(asParts) and not asParts[i].isdigit():
                sUnit = asParts[i];
                i += 1;

            sUnitLower = sUnit.lower();
            if sUnitLower in [ 'h', 'ho', 'hou', 'hour', 'hours' ]:
                pass;
            elif sUnitLower in [ 'd', 'da', 'day', 'days' ]:
                iNumber *= 24;
            elif sUnitLower in [ 'w', 'week', 'weeks' ]:
                iNumber *= 7 * 24;
            else:
                asErrors.append('Unknown unit "%s".' % (sUnit,));
            cHours += iNumber;
        else:
            asErrors.append('Bad number "%s".' % (sNumber,));
    return (cHours, None if not asErrors else ' '.join(asErrors));


#
# Introspection.
#

def getCallerName(oFrame=None, iFrame=2):
    """
    Returns the name of the caller's caller.
    """
    if oFrame is None:
        try:
            raise Exception();
        except:
            oFrame = sys.exc_info()[2].tb_frame.f_back;
        while iFrame > 1:
            if oFrame is not None:
                oFrame = oFrame.f_back;
            iFrame = iFrame - 1;
    if oFrame is not None:
        sName = '%s:%u' % (oFrame.f_code.co_name, oFrame.f_lineno);
        return sName;
    return "unknown";


def getXcptInfo(cFrames = 1):
    """
    Gets text detailing the exception. (Good for logging.)
    Returns list of info strings.
    """

    #
    # Try get exception info.
    #
    try:
        oType, oValue, oTraceback = sys.exc_info();
    except:
        oType = oValue = oTraceback = None;
    if oType is not None:

        #
        # Try format the info
        #
        asRet = [];
        try:
            try:
                asRet = asRet + traceback.format_exception_only(oType, oValue);
                asTraceBack = traceback.format_tb(oTraceback);
                if cFrames is not None and cFrames <= 1:
                    asRet.append(asTraceBack[-1]);
                else:
                    asRet.append('Traceback:')
                    for iFrame in range(min(cFrames, len(asTraceBack))):
                        asRet.append(asTraceBack[-iFrame - 1]);
                    asRet.append('Stack:')
                    asRet = asRet + traceback.format_stack(oTraceback.tb_frame.f_back, cFrames);
            except:
                asRet.append('internal-error: Hit exception #2! %s' % (traceback.format_exc(),));

            if not asRet:
                asRet.append('No exception info...');
        except:
            asRet.append('internal-error: Hit exception! %s' % (traceback.format_exc(),));
    else:
        asRet = ['Couldn\'t find exception traceback.'];
    return asRet;


def getObjectTypeName(oObject):
    """
    Get the type name of the given object.
    """
    if oObject is None:
        return 'None';

    # Get the type object.
    try:
        oType = type(oObject);
    except:
        return 'type-throws-exception';

    # Python 2.x only: Handle old-style object wrappers.
    if sys.version_info[0] < 3:
        try:
            from types import InstanceType;
            if oType == InstanceType:
                oType = oObject.__class__;
        except:
            pass;

    # Get the name.
    try:
        return oType.__name__;
    except:
        return '__type__-throws-exception';


#
# TestSuite stuff.
#

def isRunningFromCheckout(cScriptDepth = 1):
    """
    Checks if we're running from the SVN checkout or not.
    """

    try:
        sFile = __file__;
        cScriptDepth = 1;
    except:
        sFile = sys.argv[0];

    sDir = os.path.abspath(sFile);
    while cScriptDepth >= 0:
        sDir = os.path.dirname(sDir);
        if    os.path.exists(os.path.join(sDir, 'Makefile.kmk')) \
           or os.path.exists(os.path.join(sDir, 'Makefile.kup')):
            return True;
        cScriptDepth -= 1;

    return False;


#
# Bourne shell argument fun.
#


def argsSplit(sCmdLine):
    """
    Given a bourne shell command line invocation, split it up into arguments
    assuming IFS is space.
    Returns None on syntax error.
    """
    ## @todo bourne shell argument parsing!
    return sCmdLine.split(' ');

def argsGetFirst(sCmdLine):
    """
    Given a bourne shell command line invocation, get return the first argument
    assuming IFS is space.
    Returns None on invalid syntax, otherwise the parsed and unescaped argv[0] string.
    """
    asArgs = argsSplit(sCmdLine);
    if not asArgs:
        return None;

    return asArgs[0];

#
# String helpers.
#

def stricmp(sFirst, sSecond):
    """
    Compares to strings in an case insensitive fashion.

    Python doesn't seem to have any way of doing the correctly, so this is just
    an approximation using lower.
    """
    if sFirst == sSecond:
        return 0;
    sLower1 = sFirst.lower();
    sLower2 = sSecond.lower();
    if sLower1 == sLower2:
        return 0;
    if sLower1 < sLower2:
        return -1;
    return 1;


#
# Misc.
#

def versionCompare(sVer1, sVer2):
    """
    Compares to version strings in a fashion similar to RTStrVersionCompare.
    """

    ## @todo implement me!!

    if sVer1 == sVer2:
        return 0;
    if sVer1 < sVer2:
        return -1;
    return 1;


def formatNumber(lNum, sThousandSep = ' '):
    """
    Formats a decimal number with pretty separators.
    """
    sNum = str(lNum);
    sRet = sNum[-3:];
    off  = len(sNum) - 3;
    while off > 0:
        off -= 3;
        sRet = sNum[(off if off >= 0 else 0):(off + 3)] + sThousandSep + sRet;
    return sRet;


def formatNumberNbsp(lNum):
    """
    Formats a decimal number with pretty separators.
    """
    sRet = formatNumber(lNum);
    return unicode(sRet).replace(' ', u'\u00a0');


def isString(oString):
    """
    Checks if the object is a string object, hiding difference between python 2 and 3.

    Returns True if it's a string of some kind.
    Returns False if not.
    """
    if sys.version_info[0] >= 3:
        return isinstance(oString, str);
    return isinstance(oString, basestring);


def hasNonAsciiCharacters(sText):
    """
    Returns True is specified string has non-ASCII characters, False if ASCII only.
    """
    sTmp = unicode(sText, errors='ignore') if isinstance(sText, str) else sText;
    return not all(ord(ch) < 128 for ch in sTmp);


def chmodPlusX(sFile):
    """
    Makes the specified file or directory executable.
    Returns success indicator, no exceptions.

    Note! Symbolic links are followed and the target will be changed.
    """
    try:
        oStat = os.stat(sFile);
    except:
        return False;
    try:
        os.chmod(sFile, oStat.st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH);
    except:
        return False;
    return True;


def unpackZipFile(sArchive, sDstDir, fnLog, fnError = None, fnFilter = None):
    # type: (string, string, (string) -> None, (string) -> None, (string) -> bool) -> list[string]
    """
    Worker for unpackFile that deals with ZIP files, same function signature.
    """
    import zipfile
    if fnError is None:
        fnError = fnLog;

    fnLog('Unzipping "%s" to "%s"...' % (sArchive, sDstDir));

    # Open it.
    try: oZipFile = zipfile.ZipFile(sArchive, 'r')
    except Exception as oXcpt:
        fnError('Error opening "%s" for unpacking into "%s": %s' % (sArchive, sDstDir, oXcpt,));
        return None;

    # Extract all members.
    asMembers = [];
    try:
        for sMember in oZipFile.namelist():
            if fnFilter is None  or  fnFilter(sMember) is not False:
                if sMember.endswith('/'):
                    os.makedirs(os.path.join(sDstDir, sMember.replace('/', os.path.sep)), 0x1fd); # octal: 0775 (python 3/2)
                else:
                    oZipFile.extract(sMember, sDstDir);
                asMembers.append(os.path.join(sDstDir, sMember.replace('/', os.path.sep)));
    except Exception as oXcpt:
        fnError('Error unpacking "%s" into "%s": %s' % (sArchive, sDstDir, oXcpt));
        asMembers = None;

    # close it.
    try: oZipFile.close();
    except Exception as oXcpt:
        fnError('Error closing "%s" after unpacking into "%s": %s' % (sArchive, sDstDir, oXcpt));
        asMembers = None;

    return asMembers;


## Set if we've replaced tarfile.copyfileobj with __mytarfilecopyfileobj already.
g_fTarCopyFileObjOverriddend = False;

def __mytarfilecopyfileobj(src, dst, length = None, exception = OSError):
    """ tarfile.copyfileobj with different buffer size (16384 is slow on windows). """
    if length is None:
        __myshutilcopyfileobj(src, dst, g_cbGoodBufferSize);
    elif length > 0:
        cFull, cbRemainder = divmod(length, g_cbGoodBufferSize);
        for _ in xrange(cFull):
            abBuffer = src.read(g_cbGoodBufferSize);
            dst.write(abBuffer);
            if len(abBuffer) != g_cbGoodBufferSize:
                raise exception('unexpected end of source file');
        if cbRemainder > 0:
            abBuffer = src.read(cbRemainder);
            dst.write(abBuffer);
            if len(abBuffer) != cbRemainder:
                raise exception('unexpected end of source file');


def unpackTarFile(sArchive, sDstDir, fnLog, fnError = None, fnFilter = None):
    # type: (string, string, (string) -> None, (string) -> None, (string) -> bool) -> list[string]
    """
    Worker for unpackFile that deals with tarballs, same function signature.
    """
    import shutil;
    import tarfile;
    if fnError is None:
        fnError = fnLog;

    fnLog('Untarring "%s" to "%s"...' % (sArchive, sDstDir));

    #
    # Default buffer sizes of 16384 bytes is causing too many syscalls on Windows.
    # 60%+ speedup for python 2.7 and 50%+ speedup for python 3.5, both on windows with PDBs.
    # 20%+ speedup for python 2.7 and 15%+ speedup for python 3.5, both on windows skipping PDBs.
    #
    if True is True:
        __installShUtilHacks(shutil);
        global g_fTarCopyFileObjOverriddend;
        if g_fTarCopyFileObjOverriddend is False:
            g_fTarCopyFileObjOverriddend = True;
            tarfile.copyfileobj = __mytarfilecopyfileobj;

    #
    # Open it.
    #
    # Note! We not using 'r:*' because we cannot allow seeking compressed files!
    #       That's how we got a 13 min unpack time for VBoxAll on windows (hardlinked pdb).
    #
    try: oTarFile = tarfile.open(sArchive, 'r|*', bufsize = g_cbGoodBufferSize);
    except Exception as oXcpt:
        fnError('Error opening "%s" for unpacking into "%s": %s' % (sArchive, sDstDir, oXcpt,));
        return None;

    # Extract all members.
    asMembers = [];
    try:
        for oTarInfo in oTarFile:
            try:
                if fnFilter is None  or  fnFilter(oTarInfo.name) is not False:
                    if oTarInfo.islnk():
                        # Links are trouble, especially on Windows.  We must avoid the falling that will end up seeking
                        # in the compressed tar stream.  So, fall back on shutil.copy2 instead.
                        sLinkFile     = os.path.join(sDstDir, oTarInfo.name.rstrip('/').replace('/', os.path.sep));
                        sLinkTarget   = os.path.join(sDstDir, oTarInfo.linkname.rstrip('/').replace('/', os.path.sep));
                        sParentDir    = os.path.dirname(sLinkFile);
                        try:    os.unlink(sLinkFile);
                        except: pass;
                        if sParentDir and not os.path.exists(sParentDir):
                            os.makedirs(sParentDir);
                        try:    os.link(sLinkTarget, sLinkFile);
                        except: shutil.copy2(sLinkTarget, sLinkFile);
                    else:
                        if oTarInfo.isdir():
                            # Just make sure the user (we) got full access to dirs.  Don't bother getting it 100% right.
                            oTarInfo.mode |= 0x1c0; # (octal: 0700)
                        oTarFile.extract(oTarInfo, sDstDir);
                    asMembers.append(os.path.join(sDstDir, oTarInfo.name.replace('/', os.path.sep)));
            except Exception as oXcpt:
                fnError('Error unpacking "%s" member "%s" into "%s": %s' % (sArchive, oTarInfo.name, sDstDir, oXcpt));
                for sAttr in [ 'name', 'linkname', 'type', 'mode', 'size', 'mtime', 'uid', 'uname', 'gid', 'gname' ]:
                    fnError('Info: %8s=%s' % (sAttr, getattr(oTarInfo, sAttr),));
                for sFn in [ 'isdir', 'isfile', 'islnk', 'issym' ]:
                    fnError('Info: %8s=%s' % (sFn, getattr(oTarInfo, sFn)(),));
                asMembers = None;
                break;
    except Exception as oXcpt:
        fnError('Error unpacking "%s" into "%s": %s' % (sArchive, sDstDir, oXcpt));
        asMembers = None;

    #
    # Finally, close it.
    #
    try: oTarFile.close();
    except Exception as oXcpt:
        fnError('Error closing "%s" after unpacking into "%s": %s' % (sArchive, sDstDir, oXcpt));
        asMembers = None;

    return asMembers;


def unpackFile(sArchive, sDstDir, fnLog, fnError = None, fnFilter = None):
    # type: (string, string, (string) -> None, (string) -> None, (string) -> bool) -> list[string]
    """
    Unpacks the given file if it has a know archive extension, otherwise do
    nothing.

    fnLog & fnError both take a string parameter.

    fnFilter takes a member name (string) and returns True if it's included
    and False if excluded.

    Returns list of the extracted files (full path) on success.
    Returns empty list if not a supported archive format.
    Returns None on failure.  Raises no exceptions.
    """
    sBaseNameLower = os.path.basename(sArchive).lower();

    #
    # Zip file?
    #
    if sBaseNameLower.endswith('.zip'):
        return unpackZipFile(sArchive, sDstDir, fnLog, fnError, fnFilter);

    #
    # Tarball?
    #
    if   sBaseNameLower.endswith('.tar') \
      or sBaseNameLower.endswith('.tar.gz') \
      or sBaseNameLower.endswith('.tgz') \
      or sBaseNameLower.endswith('.tar.bz2'):
        return unpackTarFile(sArchive, sDstDir, fnLog, fnError, fnFilter);

    #
    # Cannot classify it from the name, so just return that to the caller.
    #
    fnLog('Not unpacking "%s".' % (sArchive,));
    return [];


def getDiskUsage(sPath):
    """
    Get free space of a partition that corresponds to specified sPath in MB.

    Returns partition free space value in MB.
    """
    if platform.system() == 'Windows':
        oCTypeFreeSpace = ctypes.c_ulonglong(0);
        ctypes.windll.kernel32.GetDiskFreeSpaceExW(ctypes.c_wchar_p(sPath), None, None,
                                                   ctypes.pointer(oCTypeFreeSpace));
        cbFreeSpace = oCTypeFreeSpace.value;
    else:
        oStats = os.statvfs(sPath); # pylint: disable=E1101
        cbFreeSpace = long(oStats.f_frsize) * oStats.f_bfree;

    # Convert to MB
    cMbFreeSpace = long(cbFreeSpace) / (1024 * 1024);

    return cMbFreeSpace;


#
# Unit testing.
#

# pylint: disable=C0111
class BuildCategoryDataTestCase(unittest.TestCase):
    def testIntervalSeconds(self):
        self.assertEqual(parseIntervalSeconds(formatIntervalSeconds(3600)), (3600, None));
        self.assertEqual(parseIntervalSeconds(formatIntervalSeconds(1209438593)), (1209438593, None));
        self.assertEqual(parseIntervalSeconds('123'), (123, None));
        self.assertEqual(parseIntervalSeconds(123), (123, None));
        self.assertEqual(parseIntervalSeconds(99999999999), (99999999999, None));
        self.assertEqual(parseIntervalSeconds(''), (0, 'Empty interval string.'));
        self.assertEqual(parseIntervalSeconds('1X2'), (3, 'Unknown unit "X".'));
        self.assertEqual(parseIntervalSeconds('1 Y3'), (4, 'Unknown unit "Y".'));
        self.assertEqual(parseIntervalSeconds('1 Z 4'), (5, 'Unknown unit "Z".'));
        self.assertEqual(parseIntervalSeconds('1 hour 2m 5second'), (3725, None));
        self.assertEqual(parseIntervalSeconds('1 hour,2m ; 5second'), (3725, None));

    def testHasNonAsciiChars(self):
        self.assertEqual(hasNonAsciiCharacters(''), False);
        self.assertEqual(hasNonAsciiCharacters('asdfgebASDFKJ@#$)(!@#UNASDFKHB*&$%&)@#(!)@(#!(#$&*#$&%*Y@#$IQWN---00;'), False);
        self.assertEqual(hasNonAsciiCharacters(u'12039889y!@#$%^&*()0-0asjdkfhoiuyweasdfASDFnvV'), False);
        self.assertEqual(hasNonAsciiCharacters(u'\u0079'), False);
        self.assertEqual(hasNonAsciiCharacters(u'\u0080'), True);
        self.assertEqual(hasNonAsciiCharacters(u'\u0081 \u0100'), True);

if __name__ == '__main__':
    unittest.main();
    # not reached.

