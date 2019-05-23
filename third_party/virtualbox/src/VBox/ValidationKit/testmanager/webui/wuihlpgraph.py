# -*- coding: utf-8 -*-
# $Id: wuihlpgraph.py $

"""
Test Manager Web-UI - Graph Helpers.
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


class WuiHlpGraphDataTable(object): # pylint: disable=R0903
    """
    Data table container.
    """

    class Row(object): # pylint: disable=R0903
        """A row."""
        def __init__(self, sGroup, aoValues, asValues = None):
            self.sName    = sGroup;
            self.aoValues = aoValues;
            if asValues is None:
                self.asValues = [str(oVal) for oVal in aoValues];
            else:
                assert len(asValues) == len(aoValues);
                self.asValues = asValues;

    def __init__(self, sGroupLable, asMemberLabels):
        self.aoTable = [ WuiHlpGraphDataTable.Row(sGroupLable, asMemberLabels), ];

    def addRow(self, sGroup, aoValues, asValues = None):
        """Adds a row to the data table."""
        self.aoTable.append(WuiHlpGraphDataTable.Row(sGroup, aoValues, asValues));
        return True;

    def getGroupCount(self):
        """Gets the number of data groups (rows)."""
        return len(self.aoTable) - 1;


class WuiHlpGraphDataTableEx(object): # pylint: disable=R0903
    """
    Data container for an table/graph with optional error bars on the Y values.
    """

    class DataSeries(object): # pylint: disable=R0903
        """
        A data series.

        The aoXValues, aoYValues and aoYErrorBars are parallel arrays, making a
        series of (X,Y,Y-err-above-delta,Y-err-below-delta) points.

        The error bars are optional.
        """
        def __init__(self, sName, aoXValues, aoYValues, asHtmlTooltips = None, aoYErrorBarBelow = None, aoYErrorBarAbove = None):
            self.sName            = sName;
            self.aoXValues        = aoXValues;
            self.aoYValues        = aoYValues;
            self.asHtmlTooltips   = asHtmlTooltips;
            self.aoYErrorBarBelow = aoYErrorBarBelow;
            self.aoYErrorBarAbove = aoYErrorBarAbove;

    def __init__(self, sXUnit, sYUnit):
        self.sXUnit   = sXUnit;
        self.sYUnit   = sYUnit;
        self.aoSeries = [];

    def addDataSeries(self, sName, aoXValues, aoYValues, asHtmlTooltips = None, aoYErrorBarBelow = None, aoYErrorBarAbove = None):
        """Adds an data series to the table."""
        self.aoSeries.append(WuiHlpGraphDataTableEx.DataSeries(sName, aoXValues, aoYValues, asHtmlTooltips,
                                                               aoYErrorBarBelow, aoYErrorBarAbove));
        return True;

    def getDataSeriesCount(self):
        """Gets the number of data series."""
        return len(self.aoSeries);


#
# Dynamically choose implementation.
#
if True: # pylint: disable=W0125
    from testmanager.webui import wuihlpgraphgooglechart        as GraphImplementation;
else:
    try:
        import matplotlib; # pylint: disable=W0611,F0401,import-error,wrong-import-order
        from testmanager.webui import wuihlpgraphmatplotlib     as GraphImplementation;
    except:
        from testmanager.webui import wuihlpgraphsimple         as GraphImplementation;

# pylint: disable=C0103
WuiHlpBarGraph              = GraphImplementation.WuiHlpBarGraph;
WuiHlpLineGraph             = GraphImplementation.WuiHlpLineGraph;
WuiHlpLineGraphErrorbarY    = GraphImplementation.WuiHlpLineGraphErrorbarY;

