"""
Copyright (C) 2008-2016 Oracle Corporation

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

import xpcom
import sys
import platform

#
# This code overcomes somewhat unlucky feature of Python, where it searches
# for binaries in the same place as platfom independent modules, while
# rest of Python bindings expect _xpcom to be inside xpcom module
#

_asVBoxPythons = [
    'VBoxPython' + str(sys.version_info[0]) + '_' + str(sys.version_info[1]),
    'VBoxPython' + str(sys.version_info[0]),
    'VBoxPython'
]

# For Python 3.2 and later use the right ABI flag suffix for the module.
if sys.hexversion >= 0x030200f0 and sys.abiflags:
    _asNew = []
    for sCandidate in _asVBoxPythons:
        if sCandidate[-1:].isdigit():
            _asNew.append(sCandidate + sys.abiflags)
        else:
            _asNew.append(sCandidate)
    _asVBoxPythons = _asNew
    del _asNew

# On platforms where we ship both 32-bit and 64-bit API bindings, we have to
# look for the right set if we're a 32-bit process.
if platform.system() in [ 'SunOS', ] and sys.maxsize <= 2**32:
    _asNew = [ sCandidate + '_x86' for sCandidate in _asVBoxPythons ]
    _asNew.extend(_asVBoxPythons)
    _asVBoxPythons = _asNew
    del _asNew

# On Darwin (aka Mac OS X) we know exactly where things are in a normal
# VirtualBox installation.
## @todo Edit this at build time to the actual VBox location set in the make files.
## @todo We know the location for most hardened builds, not just darwin!
if platform.system() == 'Darwin':
    sys.path.append('/Applications/VirtualBox.app/Contents/MacOS')

_oVBoxPythonMod = None
for m in _asVBoxPythons:
    try:
        _oVBoxPythonMod =  __import__(m)
        break
    except Exception as x:
        print('m=%s x=%s' % (m, x))
    #except:
    #    pass

if platform.system() == 'Darwin':
    sys.path.remove('/Applications/VirtualBox.app/Contents/MacOS')

if _oVBoxPythonMod == None:
    raise Exception('Cannot find VBoxPython module (tried: %s)' % (', '.join(_asVBoxPythons),))

sys.modules['xpcom._xpcom'] = _oVBoxPythonMod
xpcom._xpcom = _oVBoxPythonMod

