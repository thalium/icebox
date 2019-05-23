# -*- coding: utf-8 -*-
# $Id: diff.py $

"""
Diff two test sets.
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
__all__     = ['BaselineDiff', ];


def _findBaselineTest(oBaseline, oTest):
    """ Recursively finds the test in oBaseline corresponding to oTest. """
    if oTest.oParent is None:
        return oBaseline;
    oBaseline = _findBaselineTest(oBaseline, oTest.oParent);
    if oBaseline is not None:
        for oBaseTest in oBaseline.aoChildren:
            if oBaseTest.sName == oTest.sName:
                return oBaseTest;
    return None;

def _findBaselineTestValue(oBaseline, oValue):
    """ Finds the baseline value corresponding to oValue. """
    oBaseTest = _findBaselineTest(oBaseline, oValue.oTest);
    if oBaseTest is not None:
        for oBaseValue in oBaseTest.aoValues:
            if oBaseValue.sName == oValue.sName:
                return oBaseValue;
    return None;

def baselineDiff(oTestTree, oBaseline):
    """
    Diffs oTestTree against oBaseline, adding diff info to oTestTree.
    Returns oTestTree on success, None on failure (complained already).
    """

    aoStack  = [];
    aoStack.append((oTestTree, 0));
    while len(aoStack) > 0:
        oCurTest, iChild = aoStack.pop();

        # depth first
        if iChild < len(oCurTest.aoChildren):
            aoStack.append((oCurTest, iChild + 1));
            aoStack.append((oCurTest.aoChildren[iChild], 0));
            continue;

        # do value diff.
        for i in range(len(oCurTest.aoValues)):
            oBaseVal = _findBaselineTestValue(oBaseline, oCurTest.aoValues[i]);
            if oBaseVal is not None:
                try:
                    lBase = long(oBaseVal.sValue);
                    lTest = long(oCurTest.aoValues[i].sValue);
                except:
                    try:
                        if oBaseVal.sValue == oCurTest.aoValues[i].sValue:
                            oCurTest.aoValues[i].sValue += '|';
                        else:
                            oCurTest.aoValues[i].sValue += '|%s' % (oBaseVal.sValue);
                    except:
                        oCurTest.aoValues[i].sValue += '|%s' % (oBaseVal.sValue);
                else:
                    if lTest > lBase:
                        oCurTest.aoValues[i].sValue += '|+%u' % (lTest - lBase);
                    elif lTest < lBase:
                        oCurTest.aoValues[i].sValue += '|-%u' % (lBase - lTest);
                    else:
                        oCurTest.aoValues[i].sValue += '|0';
            else:
                oCurTest.aoValues[i].sValue += '|N/A';

    return oTestTree;
